param(
  [string]$OutExe = "build/impact_displacement_windows_cli.exe",
  [string]$Csv = "build/uplink_accel_event_window_com8_run2.csv",
  [uint16]$Pre = 30,
  [uint16]$Rate = 0,
  [uint16]$MinMs = 80,
  [uint16]$MaxMs = 20000,
  [uint16]$ReleaseThr = 80,
  [uint16]$ReleaseCount = 5,
  [uint16]$MaxDtMs = 500,
  [uint16]$GravEmaMs = 0,
  [uint32]$EventId = 1
)

$ErrorActionPreference = "Stop"

$repo = Resolve-Path (Join-Path $PSScriptRoot "..")
Set-Location $repo

$srcTest = "test/impact_displacement_windows_cli.c"
$srcAlgo = "lib/impact_displacement/src/impact_displacement.c"
$outDir = Split-Path -Parent $OutExe
if (-not (Test-Path $outDir)) {
  New-Item -ItemType Directory -Path $outDir | Out-Null
}

function Invoke-GccLike([string]$cc) {
  & $cc $srcTest $srcAlgo `
    -Ilib/impact_displacement/inc `
    -std=c11 -Wall -Wextra -O2 -o $OutExe -lm
}

if (Get-Command gcc -ErrorAction SilentlyContinue) {
  Invoke-GccLike "gcc"
} elseif (Get-Command clang -ErrorAction SilentlyContinue) {
  Invoke-GccLike "clang"
} elseif (Get-Command cl -ErrorAction SilentlyContinue) {
  & cl /nologo /Ilib/impact_displacement/inc /std:c11 /W4 /O2 /Fe:$OutExe `
    $srcTest $srcAlgo
} else {
  throw "No C compiler found (gcc/clang/cl)."
}

& $OutExe $Csv `
  --pre $Pre `
  --rate $Rate `
  --min-ms $MinMs `
  --max-ms $MaxMs `
  --release-thr $ReleaseThr `
  --release-count $ReleaseCount `
  --max-dt-ms $MaxDtMs `
  --grav-ema-ms $GravEmaMs `
  --event-id $EventId
