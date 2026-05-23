@echo off
setlocal EnableExtensions EnableDelayedExpansion

set "TEST_DIR=%~dp0"
if "%TEST_DIR:~-1%"=="\" set "TEST_DIR=%TEST_DIR:~0,-1%"
for %%I in ("%TEST_DIR%\..") do set "LIB_DIR=%%~fI"
set "OUT_EXE=%TEST_DIR%\adhoc_sim.exe"
set "SCENARIO=%~1"

if "%SCENARIO%"=="" set "SCENARIO=0"
if /I "%SCENARIO%"=="all" (
    powershell -NoProfile -ExecutionPolicy Bypass -File "%TEST_DIR%\run_all_scenarios.ps1"
    exit /b %ERRORLEVEL%
)

call :run_one %SCENARIO%
set "RET=%ERRORLEVEL%"
if not "%RET%"=="0" (
    echo [ERROR] Scenario %SCENARIO% failed with code %RET%.
    exit /b %RET%
)
echo.
echo Logs: %TEST_DIR%\logs\
exit /b 0

:run_one
set "SC=%~1"
echo === Build (Scenario %SC%) ===
call :build %SC%
if errorlevel 1 exit /b %ERRORLEVEL%

echo === Run (Scenario %SC%) ===
cd /d "%TEST_DIR%"
"%OUT_EXE%"
if errorlevel 1 exit /b %ERRORLEVEL%
exit /b 0

:build
set "SC=%~1"

where gcc >nul 2>&1
if not errorlevel 1 (
    echo [Compiler: MinGW-GCC]
    gcc -Wall -O2 -std=gnu11 -I"%LIB_DIR%\include" -I"%TEST_DIR%" ^
        -DSCENARIO_MODE=%SC%u ^
        "%TEST_DIR%\main.c" ^
        "%TEST_DIR%\channel_win32\adhoc_channel.c" ^
        "%TEST_DIR%\channel_win32\adhoc_link_win32.c" ^
        "%TEST_DIR%\channel_win32\adhoc_logger.c" ^
        "%LIB_DIR%\src\adhoc_crc8.c" ^
        "%LIB_DIR%\src\adhoc_data_plane.c" ^
        "%LIB_DIR%\src\adhoc_frame.c" ^
        "%LIB_DIR%\src\adhoc_node.c" ^
        "%LIB_DIR%\src\adhoc_reply_list.c" ^
        "%LIB_DIR%\src\adhoc_sm.c" ^
        "%LIB_DIR%\src\adhoc_timing.c" ^
        -o "%OUT_EXE%"
    if errorlevel 1 (
        echo [ERROR] GCC build failed.
        exit /b 1
    )
    exit /b 0
)

where cl >nul 2>&1
if not errorlevel 1 (
    echo [Compiler: MSVC]
    cl /nologo /W3 /O2 /MT /D_CRT_SECURE_NO_WARNINGS /utf-8 ^
       /DSCENARIO_MODE=%SC%u ^
       /I"%LIB_DIR%\include" /I"%TEST_DIR%" ^
       "%TEST_DIR%\main.c" ^
       "%TEST_DIR%\channel_win32\adhoc_channel.c" ^
       "%TEST_DIR%\channel_win32\adhoc_link_win32.c" ^
       "%TEST_DIR%\channel_win32\adhoc_logger.c" ^
       "%LIB_DIR%\src\adhoc_crc8.c" ^
       "%LIB_DIR%\src\adhoc_data_plane.c" ^
       "%LIB_DIR%\src\adhoc_frame.c" ^
       "%LIB_DIR%\src\adhoc_node.c" ^
       "%LIB_DIR%\src\adhoc_reply_list.c" ^
       "%LIB_DIR%\src\adhoc_sm.c" ^
       "%LIB_DIR%\src\adhoc_timing.c" ^
       /Fe:"%OUT_EXE%"
    if errorlevel 1 (
        echo [ERROR] MSVC build failed.
        exit /b 1
    )
    exit /b 0
)

echo [ERROR] No compiler found (gcc or cl).
exit /b 1
