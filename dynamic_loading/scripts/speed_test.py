#!/usr/bin/env python3
"""
UART Speed Test — toggles DTR to reset MCU, waits for SpeedTest header,
then sends continuous stream and captures the per-second stats.
"""

import argparse
import sys
import time
import threading

import serial

SEND_BLOCK = 1024


def burst_send(ser: serial.Serial, stop_event: threading.Event):
    """Keep sending data as fast as possible until stop_event is set."""
    payload = b'U' * SEND_BLOCK
    total = 0
    while not stop_event.is_set():
        try:
            n = ser.write(payload)
            total += n
        except serial.SerialTimeoutException:
            pass
    print(f"\n[Sender] Sent {total} bytes total", flush=True)


def main():
    parser = argparse.ArgumentParser(description='UART SpeedTest companion')
    parser.add_argument('--port', default='COM8')
    parser.add_argument('--baud', type=int, default=9600)
    parser.add_argument('--duration', type=float, default=20,
                        help='Seconds to wait for MCU output before quitting')
    parser.add_argument('--no-reset', action='store_true',
                        help='Skip DTR reset (use if already running)')
    args = parser.parse_args()

    ser = serial.Serial(args.port, args.baud, timeout=0.5, rtscts=False)
    print(f"Connected {args.port} @ {args.baud}", flush=True)

    # Reset MCU via DTR toggle (works with WCH-Link serial)
    if not args.no_reset:
        print("Toggling DTR to reset MCU...", flush=True)
        ser.dtr = False
        time.sleep(0.1)
        ser.dtr = True
        time.sleep(0.1)
        ser.dtr = False
        time.sleep(0.5)  # wait for MCU boot + printf init
    else:
        time.sleep(1.0)

    stop_send = threading.Event()
    sender = threading.Thread(target=burst_send, args=(ser, stop_send), daemon=True)

    # Flush any stale data, then start sending
    ser.reset_input_buffer()
    time.sleep(0.2)
    sender.start()

    # Read all output for the given duration
    start = time.time()
    all_output = bytearray()
    try:
        while (time.time() - start) < args.duration:
            data = ser.read(4096)
            if data:
                all_output.extend(data)
                sys.stdout.buffer.write(data)
                sys.stdout.buffer.flush()
            else:
                time.sleep(0.05)
    except KeyboardInterrupt:
        print("\n[Test] Interrupted", flush=True)
    finally:
        stop_send.set()
        sender.join(timeout=2)

    ser.close()
    elapsed = time.time() - start
    text = all_output.decode('utf-8', errors='replace')

    print(f"\n\n=== Summary ===")
    print(f"Duration: {elapsed:.1f}s, Baud: {args.baud}")
    print(f"Total output: {len(all_output)} bytes")


if __name__ == '__main__':
    main()
