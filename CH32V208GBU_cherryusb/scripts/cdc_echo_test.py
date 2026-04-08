#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import argparse
import sys
import time

import serial
import serial.tools.list_ports


def _parse_usb_id(value: str, name: str) -> int:
    text = value.strip().lower()
    if text.startswith("0x"):
        text = text[2:]
    try:
        return int(text, 16)
    except ValueError as exc:
        raise argparse.ArgumentTypeError(f"invalid {name}: {value}") from exc


def _resolve_port(auto: bool, port: str, vid: int, pid: int) -> str:
    if not auto:
        return port

    for info in serial.tools.list_ports.comports():
        if info.vid == vid and info.pid == pid:
            print(f"[AUTO] use {info.device} ({vid:04X}:{pid:04X})")
            return info.device

    raise serial.SerialException(f"cannot find usb cdc port by VID:PID {vid:04X}:{pid:04X}")


def run_echo_test(port: str, baud: int, message: str, timeout: float, retries: int, interval: float) -> int:
    payload = message.encode("utf-8")

    with serial.Serial(port, baudrate=baud, timeout=timeout) as ser:
        ser.reset_input_buffer()
        ser.reset_output_buffer()
        time.sleep(0.2)

        for i in range(1, retries + 1):
            ser.write(payload)
            ser.flush()
            time.sleep(interval)
            rx = ser.read(len(payload))

            if rx == payload:
                print(f"[PASS] echo ok on try {i}: {rx!r}")
                return 0

            print(f"[RETRY] try {i}: rx={rx!r}, expect={payload!r}")

    print("[FAIL] echo test failed")
    return 1


def main() -> int:
    parser = argparse.ArgumentParser(description="CDC ACM echo test")
    parser.add_argument("--port", default="COM9", help="Serial port (default: COM9)")
    parser.add_argument("--auto", action="store_true", help="Auto detect port by VID:PID")
    parser.add_argument("--vid", default="1A86", help="USB VID in hex (default: 1A86)")
    parser.add_argument("--pid", default="FE0C", help="USB PID in hex (default: FE0C)")
    parser.add_argument("--baud", type=int, default=115200, help="Baud rate (default: 115200)")
    parser.add_argument("--message", default="hello_cherryusb", help="ASCII/UTF-8 payload")
    parser.add_argument("--timeout", type=float, default=0.8, help="Read timeout in seconds")
    parser.add_argument("--retries", type=int, default=5, help="Retry count")
    parser.add_argument("--interval", type=float, default=0.05, help="Delay after write in seconds")
    args = parser.parse_args()

    try:
        vid = _parse_usb_id(args.vid, "vid")
        pid = _parse_usb_id(args.pid, "pid")
        port = _resolve_port(args.auto, args.port, vid, pid)
        return run_echo_test(port, args.baud, args.message, args.timeout, args.retries, args.interval)
    except (argparse.ArgumentTypeError, serial.SerialException) as exc:
        print(f"[FAIL] serial error: {exc}")
        return 2


if __name__ == "__main__":
    sys.exit(main())
