---
name: ch32-project
description: Repository-specific workflow for this CH32V208 TMOS/BLE/USB project. Use when working in this repository to build, flash, clean, monitor serial debug output, or analyze the firmware according to local conventions in `.vscode/tasks.json`, `scripts/serial_reader.py`, and repository-specific output paths and OpenOCD settings.
---

# Ch32 Project

## Overview

Follow the repository's own workflow instead of inventing a generic one.
Treat `.vscode/tasks.json` as the source of truth for build, flash, and clean behavior.

## Core Workflow

When the user asks to build, flash, clean, or monitor logs, mirror the repository conventions.

- `build`: Run `xmake` from the repository root.
- `flash`: Run the OpenOCD command defined in `.vscode/tasks.json` and program `build/cross/riscv/debug/CH32V208GBU.elf`.
- `clean`: Follow the local clean task behavior: `xmake c`, then remove `build` and `.xmake`.
- Always run `build` and `flash` sequentially: wait for `xmake` to finish successfully before starting OpenOCD.
- Do not run build and flash in parallel; this can program stale artifacts and trigger intermittent verify failures.

Prefer the local task meaning even if another build system also exists in the repository.
This repository contains both `xmake.lua` and `CMakeLists.txt`, but the default operational workflow is the VS Code task set backed by `xmake`.

For serial debug on a connected board:

- Single read smoke test: `python scripts/serial_reader.py --port COM9 --baud 115200 --encoding utf-8 --once`
- Continuous monitor: `python scripts/serial_reader.py --port COM9 --baud 115200 --encoding utf-8`
- Timed monitor (recommended to avoid port leaks): `python scripts/serial_reader.py --port COM9 --baud 115200 --encoding utf-8 --duration 8`
- For non-UTF8 payload experiments: switch `--encoding` (for example `gbk`)

Use `COM9` and `115200` as defaults unless the user specifies a different port or baud rate.

## Task Mapping

Read `.vscode/tasks.json` before assuming the commands have changed.
At the time this skill was created, the task mapping was:

- `build`
  Command: `xmake`
- `flash`
  Command: `E:/APP/MRS2/MounRiver_Studio2/resources/app/resources/win32/components/WCH/OpenOCD/OpenOCD/bin/openocd.exe`
  Arguments:
  `-s .`
  `-f tools/wch-interface.cfg`
  `-c "program build/cross/riscv/debug/CH32V208GBU.elf verify"`
  `-c "reset run"`
  `-c "exit"`
- `clean`
  Behavior: print clean messages, run `xmake c`, then remove `build` and `.xmake`

If `.vscode/tasks.json` changes later, prefer the file over this summary.

## Analysis Conventions

When analyzing this repository, treat the following as non-core/generated or build-like content unless the user explicitly asks for them:

- `build/`
- `.xmake/`
- `.cache/`
- `project/obj/`

Focus analysis on:

- `app/`
- `bsp/`
- `lib/`
- `ble_profile/`
- `sdk/`
- `utils/`
- `scripts/`
- top-level build definitions such as `xmake.lua`, `CMakeLists.txt`, and `.vscode/tasks.json`

## Project Facts

Use these facts as working context for repository-specific help:

- Main MCU family: `CH32V208`
- Architecture/toolchain target: `riscv-wch-elf-`, `rv32imacxw`
- Main app entry: `User/Main.c`
- Primary firmware output: `build/cross/riscv/debug/CH32V208GBU.elf`
- Flash configuration file: `tools/wch-interface.cfg`
- Serial debug script: `scripts/serial_reader.py` (default `COM9`, `115200`)

## Operating Rules

- Use repository-relative paths from the workspace root.
- Prefer the task-defined workflow over ad hoc command variants.
- Treat UTF-8 as the repository text encoding standard.
- Report the important output back to the user after running build/flash/clean because terminal output is not directly visible to them.
- When reading serial logs, provide a short, readable summary of the captured output.
- If a hardware-dependent flash step fails, report whether the failure looks like connection, OpenOCD, or artifact-path related.
- When the user asks for engineering analysis, distinguish between code that is compiled into the target and code that is actually exercised by `app/main.c`.

## Typical Requests

- `build`
- `flash`
- `clean`
- `analyze this project`
- `read COM9 logs`
- `tail serial output`
- `why did flash fail`
- `build this repo the local way`
