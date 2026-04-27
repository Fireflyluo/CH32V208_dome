param(
  [string]$OutExe = "build/impact_displacement_windows_smoke.exe"
)

$ErrorActionPreference = "Stop"

$repo = Resolve-Path (Join-Path $PSScriptRoot "..")
Set-Location $repo

$srcTest = "test/impact_displacement_windows_smoke.c"
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

& $OutExe
