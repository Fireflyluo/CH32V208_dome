from __future__ import annotations

import argparse
import asyncio
import json
import math
import re
import threading
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Optional

import serial
from fastapi import FastAPI, WebSocket, WebSocketDisconnect
from fastapi.responses import FileResponse
from fastapi.staticfiles import StaticFiles


UPLINK_RE = re.compile(
    r"uplink\s+ax=([+-]?\d+)\s+ay=([+-]?\d+)\s+az=([+-]?\d+)\s+mg,\s*t=([+-]?\d+(?:\.\d+)?)C\s+rh=([+-]?\d+(?:\.\d+)?)%"
)


@dataclass(frozen=True)
class AccelSample:
    t_us: int
    ax_mg: int
    ay_mg: int
    az_mg: int
    norm_mg: float
    temp_c: float
    rh_pct: float


class SerialUplinkReader:
    def __init__(self, port: str, baud: int) -> None:
        self._port = port
        self._baud = baud
        self._stop = threading.Event()
        self._thread: Optional[threading.Thread] = None
        self._loop: Optional[asyncio.AbstractEventLoop] = None
        self._queue: Optional[asyncio.Queue[AccelSample]] = None
        self._first_ns: Optional[int] = None

    def start(self, loop: asyncio.AbstractEventLoop, queue: asyncio.Queue[AccelSample]) -> None:
        self._loop = loop
        self._queue = queue
        self._stop.clear()
        self._thread = threading.Thread(target=self._run, name="serial-uplink-reader", daemon=True)
        self._thread.start()

    def stop(self) -> None:
        self._stop.set()
        t = self._thread
        if t is not None:
            t.join(timeout=1.5)
        self._thread = None

    @staticmethod
    def _parse_line(line: str) -> Optional[tuple[int, int, int, float, float]]:
        m = UPLINK_RE.search(line)
        if not m:
            return None
        return (int(m.group(1)), int(m.group(2)), int(m.group(3)), float(m.group(4)), float(m.group(5)))

    def _emit(self, sample: AccelSample) -> None:
        if self._loop is None or self._queue is None:
            return
        self._loop.call_soon_threadsafe(self._queue.put_nowait, sample)

    def _run(self) -> None:
        try:
            with serial.Serial(self._port, self._baud, timeout=0.10) as ser:
                ser.reset_input_buffer()
                while not self._stop.is_set():
                    raw = ser.readline()
                    if not raw:
                        continue
                    line = raw.decode("utf-8", errors="ignore").strip()
                    parsed = self._parse_line(line)
                    if parsed is None:
                        continue
                    ax, ay, az, temp_c, rh_pct = parsed

                    now_ns = time.monotonic_ns()
                    if self._first_ns is None:
                        self._first_ns = now_ns
                    t_us = int((now_ns - self._first_ns) / 1000)
                    norm_mg = math.sqrt(float(ax * ax + ay * ay + az * az))
                    self._emit(
                        AccelSample(
                            t_us=t_us,
                            ax_mg=ax,
                            ay_mg=ay,
                            az_mg=az,
                            norm_mg=norm_mg,
                            temp_c=temp_c,
                            rh_pct=rh_pct,
                        )
                    )
        except Exception:
            # Keep MVP simple: on serial failure, the UI will show "no data".
            return


def create_app(reader: SerialUplinkReader) -> FastAPI:
    app = FastAPI(title="Impact Web UI", version="0.1")

    static_dir = Path(__file__).parent / "static"
    app.mount("/static", StaticFiles(directory=str(static_dir)), name="static")

    @app.get("/")
    def index() -> FileResponse:
        return FileResponse(str(static_dir / "index.html"))

    sample_queue: asyncio.Queue[AccelSample] = asyncio.Queue(maxsize=5000)
    clients: set[WebSocket] = set()

    @app.on_event("startup")
    async def _startup() -> None:
        loop = asyncio.get_running_loop()
        reader.start(loop, sample_queue)

        async def fanout() -> None:
            while True:
                s = await sample_queue.get()
                if not clients:
                    continue
                msg = json.dumps(
                    {
                        "t_us": s.t_us,
                        "ax_mg": s.ax_mg,
                        "ay_mg": s.ay_mg,
                        "az_mg": s.az_mg,
                        "norm_mg": round(s.norm_mg, 3),
                        "temp_c": s.temp_c,
                        "rh_pct": s.rh_pct,
                    }
                )
                dead: list[WebSocket] = []
                for ws in clients:
                    try:
                        await ws.send_text(msg)
                    except Exception:
                        dead.append(ws)
                for ws in dead:
                    clients.discard(ws)

        asyncio.create_task(fanout())

    @app.on_event("shutdown")
    async def _shutdown() -> None:
        reader.stop()

    @app.websocket("/ws")
    async def ws_endpoint(ws: WebSocket) -> None:
        await ws.accept()
        clients.add(ws)
        try:
            while True:
                # Keep connection alive; ignore incoming messages.
                await ws.receive_text()
        except WebSocketDisconnect:
            clients.discard(ws)
        except Exception:
            clients.discard(ws)

    return app


def main() -> int:
    parser = argparse.ArgumentParser(description="Impact Web UI (serial uplink waveform viewer)")
    parser.add_argument("--port", default="COM8", help="Serial port (default: COM8)")
    parser.add_argument("--baud", type=int, default=115200, help="Baud rate (default: 115200)")
    parser.add_argument("--host", default="127.0.0.1", help="Bind host (default: 127.0.0.1)")
    parser.add_argument("--port-http", type=int, default=8000, help="HTTP port (default: 8000)")
    args = parser.parse_args()

    import uvicorn

    reader = SerialUplinkReader(args.port, args.baud)
    app = create_app(reader)
    uvicorn.run(app, host=args.host, port=args.port_http, log_level="info")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

