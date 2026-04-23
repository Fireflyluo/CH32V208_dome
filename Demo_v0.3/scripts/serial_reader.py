#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
串口数据读取工具 - 用于读取单片机主动发送的数据
"""

import argparse
import time

import serial


def decode_bytes(data, encoding='utf-8'):
    """Decode serial bytes with a selectable text encoding."""
    try:
        return data.decode(encoding)
    except (UnicodeDecodeError, LookupError):
        return f"[RAW HEX: {data.hex()}]"


def read_serial_data(port='COM8', baudrate=115200, timeout=1, encoding='utf-8', duration=None):
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


def read_once(port='COM8', baudrate=115200, timeout=2, size=500, encoding='utf-8'):
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
    parser.add_argument('--port', default='COM8', help='串口号 (默认: COM8)')
    parser.add_argument('--baud', type=int, default=115200, help='波特率 (默认: 115200)')
    parser.add_argument('--encoding', default='utf-8', help='文本编码 (默认: utf-8, 可选如 gbk)')
    parser.add_argument('--once', action='store_true', help='只读取一次')
    parser.add_argument('--duration', type=float, default=None, help='连续读取时长(秒)，超时自动退出')

    args = parser.parse_args()

    if args.once:
        print(read_once(args.port, args.baud, encoding=args.encoding))
    else:
        read_serial_data(args.port, args.baud, encoding=args.encoding, duration=args.duration)
