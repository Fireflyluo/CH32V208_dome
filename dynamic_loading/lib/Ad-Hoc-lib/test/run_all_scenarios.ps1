param(
    [int]$StartScenario = 0,
    [int]$EndScenario = 13
)

$ErrorActionPreference = "Continue"

if ($StartScenario -lt 0 -or $EndScenario -gt 13 -or $StartScenario -gt $EndScenario) {
    throw "Invalid scenario range: $StartScenario..$EndScenario (valid: 0..13)"
}

$testDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$logsDir = Join-Path $testDir "logs"
if (-not (Test-Path $logsDir)) {
    New-Item -ItemType Directory -Path $logsDir | Out-Null
}

$stamp = Get-Date -Format "yyyyMMdd-HHmmss"
$reportPath = Join-Path $logsDir "regression_summary_$stamp.md"
$results = New-Object System.Collections.Generic.List[object]

Write-Host "=== Ad-Hoc-lib Full Regression (Scenario $StartScenario..$EndScenario) ==="

for ($scenario = $StartScenario; $scenario -le $EndScenario; $scenario++) {
    Write-Host ""
    Write-Host "===== Scenario $scenario ====="

    $scenarioLog = Join-Path $logsDir ("scenario_{0}_run_{1}.log" -f $scenario, $stamp)
    $sw = [System.Diagnostics.Stopwatch]::StartNew()
    $output = & cmd /c "build_and_run.bat $scenario" 2>&1
    $exitCode = $LASTEXITCODE
    $sw.Stop()

    $output | Out-File -FilePath $scenarioLog -Encoding utf8

    $acked = ""
    $retry = ""
    $summaryLine = ($output | Select-String -Pattern "Data Summary:\s*ACKED=(\d+)\s+RETRY_EXHAUSTED=(\d+)" | Select-Object -Last 1)
    if ($summaryLine) {
        $m = [regex]::Match($summaryLine.Line, "ACKED=(\d+)\s+RETRY_EXHAUSTED=(\d+)")
        if ($m.Success) {
            $acked = $m.Groups[1].Value
            $retry = $m.Groups[2].Value
        }
    }

    $result = [PSCustomObject]@{
        scenario = $scenario
        pass = ($exitCode -eq 0)
        exit_code = $exitCode
        elapsed_s = [math]::Round($sw.Elapsed.TotalSeconds, 1)
        acked = $acked
        retry_exhausted = $retry
        log_file = Split-Path -Leaf $scenarioLog
    }
    $results.Add($result)

    if ($exitCode -ne 0) {
        Write-Host "[ERROR] Scenario $scenario failed with code $exitCode."
        break
    }
}

$expectedCount = $EndScenario - $StartScenario + 1
$allPassed = ($results.Count -eq $expectedCount -and ($results | Where-Object { -not $_.pass }).Count -eq 0)

$lines = New-Object System.Collections.Generic.List[string]
$lines.Add("# Ad-Hoc-lib Regression Report")
$lines.Add("")
$lines.Add("- Time: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')")
$lines.Add("- Scenario range: $StartScenario..$EndScenario")
$lines.Add("- Result: $(if ($allPassed) { 'PASS' } else { 'FAIL' })")
$lines.Add("")
$lines.Add("| Scenario | Result | ExitCode | Elapsed(s) | ACKED | RETRY_EXHAUSTED | Log |")
$lines.Add("|---|---|---:|---:|---:|---:|---|")
foreach ($r in $results) {
    $status = if ($r.pass) { "PASS" } else { "FAIL" }
    $lines.Add("| $($r.scenario) | $status | $($r.exit_code) | $($r.elapsed_s) | $($r.acked) | $($r.retry_exhausted) | $($r.log_file) |")
}

$lines | Out-File -FilePath $reportPath -Encoding utf8

Write-Host ""
Write-Host "Report: $reportPath"

if (-not $allPassed) {
    exit 1
}
exit 0
