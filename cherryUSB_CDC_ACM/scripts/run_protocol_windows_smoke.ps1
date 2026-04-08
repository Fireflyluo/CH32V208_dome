param(
  [string]$OutExe = "build/protocol_windows_smoke.exe"
)

$ErrorActionPreference = "Stop"

$repo = Resolve-Path (Join-Path $PSScriptRoot "..")
Set-Location $repo

$srcTest = "test/protocol_windows_smoke.c"
$srcProto = "utils/data_protocol.c"
$outDir = Split-Path -Parent $OutExe
if (-not (Test-Path $outDir)) {
  New-Item -ItemType Directory -Path $outDir | Out-Null
}

function Invoke-GccLike([string]$cc) {
  & $cc $srcTest $srcProto -Iutils -std=c11 -Wall -Wextra -O2 -o $OutExe
}

if (Get-Command gcc -ErrorAction SilentlyContinue) {
  Invoke-GccLike "gcc"
} elseif (Get-Command clang -ErrorAction SilentlyContinue) {
  Invoke-GccLike "clang"
} elseif (Get-Command cl -ErrorAction SilentlyContinue) {
  & cl /nologo /Iutils /std:c11 /W4 /O2 /Fe:$OutExe $srcTest $srcProto
} else {
  throw "No C compiler found (gcc/clang/cl)."
}

& $OutExe
