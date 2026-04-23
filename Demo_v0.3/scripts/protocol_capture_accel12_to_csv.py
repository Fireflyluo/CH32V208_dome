#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Capture protocol report frames (i/h) and dump decoded accel12 values to CSV.

Notes:
- Current firmware protocol_task uploads a single 12-bit accel value per sample:
  peak(|ax|,|ay|,|az|) mapped to 0..4095 with a configured full-scale (default 16000mg).
- This script is intended for early-stage link/sensor sanity checks (e.g. static stability).
"""

from __future__ import annotations

import argparse
import csv
import math
import sys
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Optional

import serial


TABLE = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz-_"
REV = {c: i for i, c in enumerate(TABLE)}


def crc24q(data: bytes) -> int:
    poly = 0x1864CFB
    crc = 0
    for b in data:
        crc ^= (b << 16)
        for _ in range(8):
            crc <<= 1
            if crc & 0x1000000:
                crc ^= poly
    return crc & 0xFFFFFF


def dec_u12(s: str) -> int:
    return (REV[s[0]] << 6) | REV[s[1]]


def dec_u18(s: str) -> int:
    return (REV[s[0]] << 12) | (REV[s[1]] << 6) | REV[s[2]]


def dec_u24(s: str) -> int:
    return (REV[s[0]] << 18) | (REV[s[1]] << 12) | (REV[s[2]] << 6) | REV[s[3]]


def dec_u36(s: str) -> int:
    v = 0
    for ch in s:
        v = (v << 6) | REV[ch]
    return v


def enc_u12(v: int) -> str:
    v &= 0xFFF
    return TABLE[(v >> 6) & 0x3F] + TABLE[v & 0x3F]


def enc_u24(v: int) -> str:
    v &= 0xFFFFFF
    return "".join(TABLE[(v >> s) & 0x3F] for s in (18, 12, 6, 0))


def enc_u36(v: int) -> str:
    v &= 0xFFFFFFFFF
    return "".join(TABLE[(v >> s) & 0x3F] for s in (30, 24, 18, 12, 6, 0))


def build_frame(msg_type: str, seq: int, content: str = "") -> bytes:
    seq_c = TABLE[seq & 0x3F]
    body = f"#{msg_type}{seq_c}{content}".encode("ascii")
    crc = enc_u24(crc24q(body))
    return (body.decode("ascii") + crc + "$").encode("ascii")


def parse_frame(frame: str):
    if len(frame) < 7 or frame[0] != "#" or frame[-1] != "$":
        return {"ok": False, "reason": "format"}

    msg_type = frame[1]
    seq_c = frame[2]
    if seq_c not in REV:
        return {"ok": False, "reason": "seq-char"}

    seq = REV[seq_c]
    content = frame[3:-5]
    crc_rx_s = frame[-5:-1]
    if any(c not in REV for c in crc_rx_s):
        return {"ok": False, "reason": "crc-char"}
    crc_rx = dec_u24(crc_rx_s)
    crc_calc = crc24q(frame[:-5].encode("ascii"))
    return {
        "ok": crc_rx == crc_calc,
        "type": msg_type,
        "seq": seq,
        "content": content,
        "content_len": len(content),
        "crc_rx": crc_rx,
        "crc_calc": crc_calc,
    }


def read_one_frame(ser: serial.Serial, timeout_s: float) -> str:
    end_t = time.time() + timeout_s
    buf = bytearray()
    while time.time() < end_t:
        chunk = ser.read(256)
        if not chunk:
            continue
        buf.extend(chunk)
        i = buf.find(b"#")
        if i >= 0:
            j = buf.find(b"$", i + 1)
            if j >= 0:
                return bytes(buf[i : j + 1]).decode("ascii", errors="replace")
    return ""


def read_one_frame_with_retry(
    ser: serial.Serial, timeout_s: float, read_retries: int, retry_gap_s: float
) -> str:
    attempt = 0
    while True:
        frame = read_one_frame(ser, timeout_s)
        if frame:
            return frame
        if attempt >= read_retries:
            return ""
        attempt += 1
        if retry_gap_s > 0:
            time.sleep(retry_gap_s)


def temp_from_12bit(v: int) -> float:
    return (v & 0x0FFF) / 10.0 - 200.0


def hum_from_12bit(v: int) -> float:
    return (v & 0x0FFF) / 10.0


def accel12_to_mg(v: int, full_scale_mg: float) -> float:
    return (v & 0x0FFF) * full_scale_mg / 4095.0


@dataclass
class ReportChunk:
    tag_id: int
    start_hour: int
    start_minute: int
    start_second: int
    chunk_seq: int
    temperature_c: Optional[float]
    humidity_pct: Optional[float]
    accel12: list[int]


def parse_report_content(content: str) -> ReportChunk:
    if len(content) < 12:
        raise ValueError("content too short")

    tag_id = dec_u36(content[0:6])
    start_hour = REV[content[6]]
    start_minute = REV[content[7]]
    start_second = REV[content[8]]
    chunk_seq = dec_u18(content[9:12])

    pos = 12
    temperature_c = None
    humidity_pct = None

    # In current firmware, temp/humidity are included only in the first chunk (chunk_seq==1).
    if chunk_seq == 1 and (pos + 4) <= len(content):
        t12 = dec_u12(content[pos : pos + 2])
        h12 = dec_u12(content[pos + 2 : pos + 4])
        temperature_c = temp_from_12bit(t12)
        humidity_pct = hum_from_12bit(h12)
        pos += 4

    accel12: list[int] = []
    while (pos + 2) <= len(content):
        accel12.append(dec_u12(content[pos : pos + 2]))
        pos += 2

    return ReportChunk(
        tag_id=tag_id,
        start_hour=start_hour,
        start_minute=start_minute,
        start_second=start_second,
        chunk_seq=chunk_seq,
        temperature_c=temperature_c,
        humidity_pct=humidity_pct,
        accel12=accel12,
    )


def build_param_content(args) -> str:
    # Keep format compatible with scripts/protocol_host_tester.py
    return "".join(
        [
            enc_u24(args.t1),
            enc_u24(args.t2),
            enc_u24(args.t3),
            enc_u24(args.t4),
            enc_u12(args.th_high),
            enc_u12(args.th_low),
            enc_u36(args.master_time),
        ]
    )


def safe_mean(xs: list[float]) -> float:
    return sum(xs) / float(len(xs)) if xs else 0.0


def safe_std(xs: list[float]) -> float:
    if not xs:
        return 0.0
    m = safe_mean(xs)
    return math.sqrt(sum((x - m) ** 2 for x in xs) / float(len(xs)))


def main() -> int:
    parser = argparse.ArgumentParser(description="Capture protocol report accel12 to CSV")
    parser.add_argument("--port", default="COM4", help="Serial port, default: COM4")
    parser.add_argument("--baud", type=int, default=115200, help="Baud rate, default: 115200")
    parser.add_argument("--out", default="", help="CSV output path, default: build/accel12_capture_<ts>.csv")
    parser.add_argument("--samples", type=int, default=120, help="Target sample count, default: 120")
    parser.add_argument("--full-scale-mg", type=float, default=16000.0, help="Accel full-scale mg for decoding")

    parser.add_argument("--timeout", type=float, default=3.0, help="Single-frame receive timeout (s)")
    parser.add_argument("--query-interval", type=float, default=0.25, help="Gap between S queries (s)")
    parser.add_argument("--startup-delay", type=float, default=0.5, help="Wait after port open before first TX (s)")
    parser.add_argument("--warmup-queries", type=int, default=1, help="Warmup S count before capture loop")
    parser.add_argument("--read-retries", type=int, default=4, help="Extra read attempts after timeout")
    parser.add_argument("--read-retry-gap", type=float, default=0.10, help="Gap between extra read attempts (s)")
    parser.add_argument("--s-resend-on-timeout", type=int, default=2, help="Max resend count for each S on timeout")
    parser.add_argument("--s-resend-gap", type=float, default=0.10, help="Gap before resend S after timeout (s)")

    parser.add_argument("--send-params", dest="send_params", action="store_true", help="Send one P frame before capture")
    parser.add_argument("--no-send-params", dest="send_params", action="store_false", help="Skip P frame")
    parser.add_argument("--t1", type=int, default=6)
    parser.add_argument("--t2", type=int, default=100)
    parser.add_argument("--t3", type=int, default=15)
    parser.add_argument("--t4", type=int, default=800)
    parser.add_argument("--th-high", type=int, default=384)
    parser.add_argument("--th-low", type=int, default=13)
    parser.add_argument("--master-time", type=int, default=0x123456789)
    parser.set_defaults(send_params=True)
    args = parser.parse_args()

    out_path = args.out
    if not out_path:
        ts = time.strftime("%Y%m%d_%H%M%S")
        out_path = f"build/accel12_capture_{ts}.csv"
    out_file = Path(out_path)
    out_file.parent.mkdir(parents=True, exist_ok=True)

    seq = 0
    rows: list[list[str]] = []
    peak_mg_samples: list[float] = []
    last_temp_c: Optional[float] = None
    last_hum_pct: Optional[float] = None

    try:
        with serial.Serial(args.port, args.baud, timeout=0.05) as ser:
            ser.reset_input_buffer()
            if args.startup_delay > 0:
                time.sleep(args.startup_delay)
            ser.reset_input_buffer()

            if args.send_params:
                p_content = build_param_content(args)
                tx = build_frame("P", seq, p_content)
                seq = (seq + 1) & 0x3F
                ser.write(tx)
                rx = read_one_frame_with_retry(ser, args.timeout, args.read_retries, args.read_retry_gap)
                if not rx:
                    print("RX: <timeout> (ACK q not received)", file=sys.stderr)
                else:
                    p = parse_frame(rx)
                    if not p.get("ok", False) or p.get("type") != "q":
                        print(f"RX: {rx}  [unexpected/invalid]", file=sys.stderr)

            warmup_idx = 0
            while warmup_idx < args.warmup_queries:
                tx = build_frame("S", seq, "")
                seq = (seq + 1) & 0x3F
                ser.write(tx)
                _ = read_one_frame_with_retry(ser, args.timeout, args.read_retries, args.read_retry_gap)
                warmup_idx += 1
                if args.query_interval > 0:
                    time.sleep(args.query_interval)

            sample_index = 0
            while sample_index < args.samples:
                tx = build_frame("S", seq, "")
                seq = (seq + 1) & 0x3F
                ser.write(tx)

                rx = read_one_frame_with_retry(ser, args.timeout, args.read_retries, args.read_retry_gap)
                if not rx:
                    resend_try = 0
                    while resend_try < args.s_resend_on_timeout and not rx:
                        resend_try += 1
                        if args.s_resend_gap > 0:
                            time.sleep(args.s_resend_gap)
                        ser.write(tx)
                        rx = read_one_frame_with_retry(ser, args.timeout, args.read_retries, args.read_retry_gap)
                    if not rx:
                        print("RX: <timeout>", file=sys.stderr)
                        if args.query_interval > 0:
                            time.sleep(args.query_interval)
                        continue

                p = parse_frame(rx)
                if not p.get("ok", False):
                    if args.query_interval > 0:
                        time.sleep(args.query_interval)
                    continue

                msg_type = p["type"]
                if msg_type not in ("i", "h"):
                    if args.query_interval > 0:
                        time.sleep(args.query_interval)
                    continue

                try:
                    chunk = parse_report_content(p["content"])
                except Exception:
                    if args.query_interval > 0:
                        time.sleep(args.query_interval)
                    continue

                if chunk.temperature_c is not None:
                    last_temp_c = chunk.temperature_c
                if chunk.humidity_pct is not None:
                    last_hum_pct = chunk.humidity_pct

                for v12 in chunk.accel12:
                    if sample_index >= args.samples:
                        break
                    peak_mg = accel12_to_mg(v12, args.full_scale_mg)
                    peak_mg_samples.append(peak_mg)

                    ts_us_est = int(sample_index * int(args.t2) * 1000)
                    rows.append(
                        [
                            str(sample_index),
                            str(ts_us_est),
                            f"{peak_mg:.3f}",
                            str(v12 & 0x0FFF),
                            str(chunk.chunk_seq),
                            str(chunk.start_hour),
                            str(chunk.start_minute),
                            str(chunk.start_second),
                            "" if last_temp_c is None else f"{last_temp_c:.2f}",
                            "" if last_hum_pct is None else f"{last_hum_pct:.2f}",
                        ]
                    )
                    sample_index += 1

                if args.query_interval > 0:
                    time.sleep(args.query_interval)

    except serial.SerialException as e:
        print(f"Serial error: {e}", file=sys.stderr)
        return 2

    with out_file.open("w", newline="", encoding="utf-8") as fp:
        w = csv.writer(fp)
        w.writerow(
            [
                "sample_index",
                "timestamp_us_est",
                "peak_mg",
                "accel12",
                "chunk_seq",
                "start_hour",
                "start_minute",
                "start_second",
                "temp_c",
                "hum_pct",
            ]
        )
        w.writerows(rows)

    if peak_mg_samples:
        m = safe_mean(peak_mg_samples)
        s = safe_std(peak_mg_samples)
        print(
            f"saved: {out_file.as_posix()}  "
            f"n={len(peak_mg_samples)} mean={m:.1f}mg std={s:.1f}mg "
            f"min={min(peak_mg_samples):.1f}mg max={max(peak_mg_samples):.1f}mg"
        )
    else:
        print(f"saved: {out_file.as_posix()}  n=0 (no report frames captured)")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
