#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
串口数据读取工具 - 用于读取单片机主动发送的数据
"""

import serial
import sys
import time

def read_serial_data(port='COM8', baudrate=115200, timeout=1):
    """读取串口数据"""
    try:
        ser = serial.Serial(port, baudrate, timeout=timeout)
        print(f"已连接到 {port}，波特率 {baudrate}")
        print("等待数据... (按 Ctrl+C 退出)\n")
        
        while True:
            if ser.in_waiting > 0:
                data = ser.read(ser.in_waiting)
                try:
                    text = data.decode('utf-8')
                    print(text, end='')
                except UnicodeDecodeError:
                    print(f"[原始数据: {data.hex()}]")
            time.sleep(0.1)
            
    except serial.SerialException as e:
        print(f"串口错误: {e}")
    except KeyboardInterrupt:
        print("\n已停止读取")
    finally:
        if 'ser' in locals() and ser.is_open:
            ser.close()
            print("串口已关闭")

def read_once(port='COM8', baudrate=115200, timeout=2, size=500):
    """单次读取串口数据"""
    try:
        ser = serial.Serial(port, baudrate, timeout=timeout)
        data = ser.read(size)
        ser.close()
        
        if data:
            try:
                return data.decode('utf-8')
            except UnicodeDecodeError:
                return f"[原始数据: {data.hex()}]"
        return ""
    except serial.SerialException as e:
        return f"串口错误: {e}"

if __name__ == "__main__":
    import argparse
    
    parser = argparse.ArgumentParser(description='串口数据读取工具')
    parser.add_argument('--port', default='COM8', help='串口号 (默认: COM8)')
    parser.add_argument('--baud', type=int, default=115200, help='波特率 (默认: 115200)')
    parser.add_argument('--once', action='store_true', help='只读取一次')
    
    args = parser.parse_args()
    
    if args.once:
        result = read_once(args.port, args.baud)
        print(result)
    else:
        read_serial_data(args.port, args.baud)
