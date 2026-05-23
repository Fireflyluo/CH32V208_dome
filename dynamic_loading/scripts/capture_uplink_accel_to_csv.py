#!/usr/bin/env python3
"""
Capture textual uplink accel logs from serial and save to CSV.

Expected firmware line format:
  uplink ax=-398 ay=100 az=-892 mg, t=29.08C rh=23.47%

Output CSV header (algorithm-compatible):
  timestamp_us,ax_mg,ay_mg,az_mg,flags
"""

from __future__ import annotations

import argparse
import csv
import math
import re
import sys
import time
from pathlib import Path

import serial


UPLINK_RE = re.compile(
    r"uplink\s+ax=([+-]?\d+)\s+ay=([+-]?\d+)\s+az=([+-]?\d+)\s+mg,\s*t=([+-]?\d+(?:\.\d+)?)C\s+rh=([+-]?\d+(?:\.\d+)?)%"
)


def safe_mean(vals: list[float]) -> float:
    return sum(vals) / float(len(vals)) if vals else 0.0


def safe_std(vals: list[float]) -> float:
    if not vals:
        return 0.0
    m = safe_mean(vals)
    return math.sqrt(sum((x - m) ** 2 for x in vals) / float(len(vals)))


def parse_uplink(line: str) -> tuple[int, int, int, float, float] | None:
    m = UPLINK_RE.search(line)
    if not m:
        return None
    return (
        int(m.group(1)),
        int(m.group(2)),
        int(m.group(3)),
        float(m.group(4)),
        float(m.group(5)),
    )


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Capture serial uplink ax/ay/az logs to algorithm CSV"
    )
    parser.add_argument("--port", default="COM8", help="Serial port, default: COM8")
    parser.add_argument("--baud", type=int, default=115200, help="Baud rate")
    parser.add_argument("--timeout", type=float, default=0.10, help="Serial read timeout (s)")
    parser.add_argument(
        "--duration",
        type=float,
        default=15.0,
        help="Capture duration in seconds (default: 15)",
    )
    parser.add_argument(
        "--samples",
        type=int,
        default=0,
        help="Target sample count (0 means unlimited until duration)",
    )
    parser.add_argument(
        "--baseline-samples",
        type=int,
        default=10,
        help="Sample count used for host-side baseline magnitude",
    )
    parser.add_argument(
        "--impact-threshold-mg",
        type=float,
        default=180.0,
        help="Host-side impact flag threshold on |norm-baseline| (mg)",
    )
    parser.add_argument(
        "--out",
        default="",
        help="Output CSV path, default: build/uplink_accel_capture_<ts>.csv",
    )
    args = parser.parse_args()

    out_path = args.out
    if not out_path:
        ts = time.strftime("%Y%m%d_%H%M%S")
        out_path = f"build/uplink_accel_capture_{ts}.csv"
    out_file = Path(out_path)
    out_file.parent.mkdir(parents=True, exist_ok=True)

    rows: list[list[str]] = []
    norm_vals: list[float] = []
    temp_vals: list[float] = []
    rh_vals: list[float] = []
    hint_count = 0
    hint_first_idx = -1

    first_uplink_ns: int | None = None
    begin_ns = time.monotonic_ns()
    stop_after_ns = begin_ns + int(args.duration * 1e9)

    try:
        with serial.Serial(args.port, args.baud, timeout=args.timeout) as ser:
            ser.reset_input_buffer()
            while True:
                now_ns = time.monotonic_ns()
                if now_ns >= stop_after_ns:
                    break
                if args.samples > 0 and len(rows) >= args.samples:
                    break

                raw = ser.readline()
                if not raw:
                    continue
                text = raw.decode("utf-8", errors="ignore").strip()
                parsed = parse_uplink(text)
                if parsed is None:
                    continue

                ax, ay, az, temp_c, rh_pct = parsed
                norm_mg = math.sqrt(float(ax * ax + ay * ay + az * az))
                norm_vals.append(norm_mg)
                temp_vals.append(temp_c)
                rh_vals.append(rh_pct)

                if first_uplink_ns is None:
                    first_uplink_ns = now_ns
                ts_us = int((now_ns - first_uplink_ns) / 1000)

                baseline_n = min(len(norm_vals), max(1, args.baseline_samples))
                baseline = safe_mean(norm_vals[:baseline_n])
                impact_dyn = abs(norm_mg - baseline)
                hint = 1 if (len(norm_vals) > baseline_n and impact_dyn >= args.impact_threshold_mg) else 0
                if hint != 0:
                    hint_count += 1
                    if hint_first_idx < 0:
                        hint_first_idx = len(rows)

                # flags bit definitions belong to sensor/sample quality, not event markers.
                rows.append([str(ts_us), str(ax), str(ay), str(az), "0"])

    except serial.SerialException as e:
        print(f"Serial error: {e}", file=sys.stderr)
        return 2

    with out_file.open("w", newline="", encoding="utf-8") as fp:
        w = csv.writer(fp)
        w.writerow(["timestamp_us", "ax_mg", "ay_mg", "az_mg", "flags"])
        w.writerows(rows)

    if rows:
        duration_s = 0.0
        if first_uplink_ns is not None:
            duration_s = (time.monotonic_ns() - first_uplink_ns) / 1e9
        print(
            f"saved: {out_file.as_posix()}  n={len(rows)} duration={duration_s:.2f}s "
            f"norm_mean={safe_mean(norm_vals):.1f}mg norm_std={safe_std(norm_vals):.1f}mg "
            f"norm_min={min(norm_vals):.1f}mg norm_max={max(norm_vals):.1f}mg "
            f"impact_hints={hint_count} first_hint_idx={hint_first_idx} "
            f"temp_mean={safe_mean(temp_vals):.2f}C rh_mean={safe_mean(rh_vals):.2f}%"
        )
    else:
        print(f"saved: {out_file.as_posix()}  n=0 (no uplink accel lines captured)")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
