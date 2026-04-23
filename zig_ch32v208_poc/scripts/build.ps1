param(
    [ValidateSet("debug", "release")]
    [string]$Mode = "debug",
    [string]$WchToolchainRoot = "",
    [ValidateSet("15", "12", "auto")]
    [string]$WchGccVer = "auto",
    [string]$ZigExe = ""
)

$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$rootDir = Resolve-Path (Join-Path $scriptDir "..")
Push-Location $rootDir

try {
    $args = @("f", "-p", "cross", "-a", "riscv", "-m", $Mode, "--wch_gcc_ver=$WchGccVer")
    if ($WchToolchainRoot -ne "") { $args += "--wch_toolchain_root=$WchToolchainRoot" }
    if ($ZigExe -ne "") { $args += "--zig_exe=$ZigExe" }

    & xmake @args
    if ($LASTEXITCODE -ne 0) { throw "xmake configure failed (exit=$LASTEXITCODE)" }

    & xmake
    if ($LASTEXITCODE -ne 0) { throw "xmake build failed (exit=$LASTEXITCODE)" }
}
finally {
    Pop-Location
}

