param(
    [string]$Version = "0.15.2"
)

$ErrorActionPreference = "Stop"
$ProgressPreference = "SilentlyContinue"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$rootDir = Resolve-Path (Join-Path $scriptDir "..")
$toolsDir = Join-Path $rootDir "tools"
$zigRoot = Join-Path $toolsDir "zig"

if (-not (Test-Path $toolsDir)) {
    New-Item -ItemType Directory -Path $toolsDir | Out-Null
}
if (Test-Path $zigRoot) {
    Remove-Item -Recurse -Force $zigRoot
}
New-Item -ItemType Directory -Path $zigRoot | Out-Null

$zipName = "zig-x86_64-windows-$Version.zip"
$url = "https://ziglang.org/download/$Version/$zipName"
$zipPath = Join-Path $toolsDir $zipName

Write-Host "Downloading: $url"
Invoke-WebRequest -Uri $url -OutFile $zipPath

Write-Host "Extracting to: $zigRoot"
Expand-Archive -Path $zipPath -DestinationPath $zigRoot
Remove-Item -Force $zipPath

$zigExe = Get-ChildItem -Path $zigRoot -Recurse -Filter zig.exe | Select-Object -First 1 -ExpandProperty FullName
if (-not $zigExe) {
    throw "zig.exe not found after extraction"
}

Write-Host "Ready: $zigExe"
Write-Host "Build with:"
Write-Host "powershell -ExecutionPolicy Bypass -File .\\scripts\\build.ps1 -ZigExe `"$zigExe`""

