#!/usr/bin/env python3
"""Generate deterministic synthetic impact datasets for PC-side validation."""

from __future__ import annotations

import csv
import math
from pathlib import Path

DT_US = 2500  # 400 Hz

NOISE_X = [-5, -3, -1, 0, 2, 4, 5, 3, 1, -2, -4]
NOISE_Y = [2, 1, 0, -1, -2, -1, 0, 1, 2, 1, 0]
NOISE_Z = [-8, -6, -3, 0, 3, 6, 8, 5, 2, -2, -5]


def write_csv(path: Path, rows: list[tuple[int, int, int, int, int]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="", encoding="utf-8") as fp:
        writer = csv.writer(fp)
        writer.writerow(["timestamp_us", "ax_mg", "ay_mg", "az_mg", "flags"])
        writer.writerows(rows)


def build_ideal() -> list[tuple[int, int, int, int, int]]:
    rows: list[tuple[int, int, int, int, int]] = []
    total = 160
    for i in range(total):
        ts = i * DT_US
        if i < 40:
            ax = 0
        elif i < 80:
            ax = 102
        elif i < 120:
            ax = -102
        else:
            ax = 0
        rows.append((ts, ax, 0, 1000, 0))
    return rows


def build_nonideal() -> list[tuple[int, int, int, int, int]]:
    rows: list[tuple[int, int, int, int, int]] = []
    total = 220
    for i in range(total):
        ts = i * DT_US
        ax_dyn = 0.0
        if 60 <= i < 95:
            p = (i - 60) / 35.0
            ax_dyn += 240.0 * math.sin(math.pi * p)
        elif 95 <= i < 140:
            p = (i - 95) / 45.0
            ax_dyn += -187.0 * math.sin(math.pi * p)
        elif 140 <= i < 180:
            p = i - 140
            decay = 1.0 - (p / 40.0)
            ax_dyn += 35.0 * math.sin(2.0 * math.pi * p / 20.0) * max(decay, 0.0)

        tilt_mg = 0.0
        if 70 <= i < 150:
            tilt_mg = -35.0 * ((i - 70) / 80.0)
        elif i >= 150:
            tilt_mg = -22.0

        ax = int(round(6.0 + ax_dyn + NOISE_X[i % len(NOISE_X)]))
        ay = int(round(-4.0 + 0.12 * ax_dyn + NOISE_Y[i % len(NOISE_Y)]))
        az = int(round(1000.0 + tilt_mg + NOISE_Z[i % len(NOISE_Z)]))
        rows.append((ts, ax, ay, az, 0))
    return rows


def main() -> None:
    out_dir = Path(__file__).resolve().parent
    write_csv(out_dir / "ideal_collision_event.csv", build_ideal())
    write_csv(out_dir / "nonideal_collision_event.csv", build_nonideal())
    print("Generated ideal_collision_event.csv and nonideal_collision_event.csv")


if __name__ == "__main__":
    main()
