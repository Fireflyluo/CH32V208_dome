#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
串口数据读取工具 - 用于读取单片机主动发送的数据
"""

import argparse
import time

import serial
import serial.tools.list_ports


def parse_usb_id(value, name):
    text = value.strip().lower()
    if text.startswith("0x"):
        text = text[2:]
    try:
        return int(text, 16)
    except ValueError as exc:
        raise ValueError(f"invalid {name}: {value}") from exc


def resolve_port(auto=False, port='COM9', vid=0x1A86, pid=0xFE0C):
    if not auto:
        return port

    for info in serial.tools.list_ports.comports():
        if info.vid == vid and info.pid == pid:
            print(f"[AUTO] use {info.device} ({vid:04X}:{pid:04X})")
            return info.device

    raise serial.SerialException(f"cannot find usb cdc port by VID:PID {vid:04X}:{pid:04X}")


def decode_bytes(data, encoding='utf-8'):
    """Decode serial bytes with a selectable text encoding."""
    try:
        return data.decode(encoding)
    except (UnicodeDecodeError, LookupError):
        return f"[RAW HEX: {data.hex()}]"


def read_serial_data(port='COM9', baudrate=115200, timeout=1, encoding='utf-8', duration=None):
    """持续读取串口数据，可选自动退出时长。"""
    ser = None
    try:
        ser = serial.Serial(port, baudrate, timeout=timeout)
        print(f"已连接到 {port}，波特率 {baudrate}，编码 {encoding}")
        if duration is None:
            print("等待数据... (按 Ctrl+C 退出)\n")
        else:
            print(f"等待数据... (自动退出: {duration:.1f}s)\n")

        start = time.time()
        while True:
            if ser.in_waiting > 0:
                data = ser.read(ser.in_waiting)
                text = decode_bytes(data, encoding)
                print(text, end='')

            if duration is not None and (time.time() - start) >= duration:
                print("\n[serial_reader] duration reached, exiting.")
                break

            time.sleep(0.1)

    except serial.SerialException as e:
        print(f"串口错误: {e}")
    except KeyboardInterrupt:
        print("\n已停止读取")
    finally:
        if ser is not None and ser.is_open:
            ser.close()
            print("串口已关闭")


def read_once(port='COM9', baudrate=115200, timeout=2, size=500, encoding='utf-8'):
    """单次读取串口数据。"""
    try:
        with serial.Serial(port, baudrate, timeout=timeout) as ser:
            data = ser.read(size)
        if data:
            return decode_bytes(data, encoding)
        return ""
    except serial.SerialException as e:
        return f"串口错误: {e}"


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='串口数据读取工具')
    parser.add_argument('--port', default='COM9', help='串口号 (默认: COM9)')
    parser.add_argument('--auto', action='store_true', help='按 VID/PID 自动识别串口')
    parser.add_argument('--vid', default='1A86', help='USB VID(16进制, 默认: 1A86)')
    parser.add_argument('--pid', default='FE0C', help='USB PID(16进制, 默认: FE0C)')
    parser.add_argument('--baud', type=int, default=115200, help='波特率 (默认: 115200)')
    parser.add_argument('--encoding', default='utf-8', help='文本编码 (默认: utf-8, 可选如 gbk)')
    parser.add_argument('--once', action='store_true', help='只读取一次')
    parser.add_argument('--duration', type=float, default=None, help='连续读取时长(秒)，超时自动退出')

    args = parser.parse_args()
    selected_port = args.port
    try:
        vid = parse_usb_id(args.vid, 'vid')
        pid = parse_usb_id(args.pid, 'pid')
        selected_port = resolve_port(args.auto, args.port, vid, pid)
    except (ValueError, serial.SerialException) as exc:
        print(f"串口错误: {exc}")
        raise SystemExit(2) from exc

    if args.once:
        print(read_once(selected_port, args.baud, encoding=args.encoding))
    else:
        read_serial_data(selected_port, args.baud, encoding=args.encoding, duration=args.duration)
