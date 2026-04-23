# Impact Web UI (MVP)

Minimal PC-side web viewer for real-time accelerometer waveforms.

## What It Does

- Reads text logs from the firmware on a serial port (default `COM8`).
- Parses lines like:
  - `uplink ax=-142 ay=244 az=-951 mg, t=29.08C rh=23.47%`
- Serves a single-page web UI that plots `ax/ay/az` over time.

## Quick Start (Windows)

```powershell
cd tools/impact_webui
python -m pip install -r requirements.txt
python app.py --port COM8 --baud 115200
```

Open:

- `http://127.0.0.1:8000/`

## Notes

- Designed as a simple waveform viewer only (no database, no accounts).
- Works best when firmware outputs accel at `>= 50Hz` (100Hz recommended).

