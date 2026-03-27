#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Protocol host-side tester.

Features:
1) Optionally send one `P` parameter frame and wait for `q`.
2) Continuously send `S` query frames and print `n/m/i/h` replies.
3) Optionally send one `E` frame to request re-send of last report frame.
"""

import argparse
import sys
import time
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


def enc_u12(v: int) -> str:
    v &= 0xFFF
    return TABLE[(v >> 6) & 0x3F] + TABLE[v & 0x3F]


def enc_u24(v: int) -> str:
    v &= 0xFFFFFF
    return "".join(TABLE[(v >> s) & 0x3F] for s in (18, 12, 6, 0))


def enc_u36(v: int) -> str:
    v &= 0xFFFFFFFFF
    return "".join(TABLE[(v >> s) & 0x3F] for s in (30, 24, 18, 12, 6, 0))


def dec_u24(s: str) -> Optional[int]:
    if len(s) != 4 or any(c not in REV for c in s):
        return None
    return (REV[s[0]] << 18) | (REV[s[1]] << 12) | (REV[s[2]] << 6) | REV[s[3]]


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
    crc_rx = dec_u24(crc_rx_s)
    if crc_rx is None:
        return {"ok": False, "reason": "crc-char"}

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


def build_param_content(args) -> str:
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


def print_frame(prefix: str, frame: str):
    if not frame:
        print(f"{prefix}: <timeout>")
        return

    p = parse_frame(frame)
    if not p["ok"]:
        reason = p.get("reason", "crc")
        print(f"{prefix}: {frame}  [INVALID:{reason}]")
        return

    print(
        f"{prefix}: {frame}  "
        f"[type={p['type']} seq={p['seq']} content_len={p['content_len']} "
        f"crc=0x{p['crc_rx']:06X}]"
    )


def main():
    parser = argparse.ArgumentParser(description="Protocol host tester for P/S/E frames")
    parser.add_argument("--port", default="COM4", help="Serial port, default: COM4")
    parser.add_argument("--baud", type=int, default=115200, help="Baud rate, default: 115200")
    parser.add_argument("--timeout", type=float, default=1.5, help="Single-frame receive timeout (s)")
    parser.add_argument("--query-count", type=int, default=8, help="Number of S queries to send")
    parser.add_argument("--query-interval", type=float, default=0.20, help="Gap between S queries (s)")
    parser.add_argument("--send-params", action="store_true", help="Send one P frame before queries")
    parser.add_argument("--send-repeat", action="store_true", help="Send one E frame after queries")
    parser.add_argument("--t1", type=int, default=1000)
    parser.add_argument("--t2", type=int, default=100)
    parser.add_argument("--t3", type=int, default=3000)
    parser.add_argument("--t4", type=int, default=4000)
    parser.add_argument("--th-high", type=int, default=3000)
    parser.add_argument("--th-low", type=int, default=100)
    parser.add_argument("--master-time", type=int, default=0x123456789)
    args = parser.parse_args()

    seq = 0
    try:
        with serial.Serial(args.port, args.baud, timeout=0.05) as ser:
            ser.reset_input_buffer()
            time.sleep(0.2)

            if args.send_params:
                p_content = build_param_content(args)
                tx = build_frame("P", seq, p_content)
                seq = (seq + 1) & 0x3F
                print_frame("TX-P", tx.decode("ascii"))
                ser.write(tx)
                rx = read_one_frame(ser, args.timeout)
                print_frame("RX", rx)

            for i in range(args.query_count):
                tx = build_frame("S", seq, "")
                seq = (seq + 1) & 0x3F
                print_frame(f"TX-S[{i + 1}]", tx.decode("ascii"))
                ser.write(tx)
                rx = read_one_frame(ser, args.timeout)
                print_frame("RX", rx)
                time.sleep(args.query_interval)

            if args.send_repeat:
                tx = build_frame("E", seq, "")
                seq = (seq + 1) & 0x3F
                print_frame("TX-E", tx.decode("ascii"))
                ser.write(tx)
                rx = read_one_frame(ser, args.timeout)
                print_frame("RX", rx)

    except serial.SerialException as e:
        print(f"Serial error: {e}")
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
