param([string]$AttemptId = 'attempt-002')

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$TaskId = 'Dev.D.UE.0.0.9.F0.0.r0'
$ProjectRoot = Split-Path -Parent $PSScriptRoot
$AttemptRoot = Join-Path $ProjectRoot "Saved\Automation\$TaskId\Build\$AttemptId"
$BuildBat = 'C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat'
$Uproject = Join-Path $ProjectRoot 'demo_map.uproject'
$Utf8NoBom = [System.Text.UTF8Encoding]::new($false)

if (Test-Path -LiteralPath $AttemptRoot) {
    throw "Build attempt already exists: $AttemptRoot"
}
foreach ($Required in @($BuildBat, $Uproject)) {
    if (-not (Test-Path -LiteralPath $Required -PathType Leaf)) {
        throw "Missing required file: $Required"
    }
}
New-Item -ItemType Directory -Path $AttemptRoot | Out-Null

function Invoke-TargetBuild([string]$Target) {
    $Stem = "$Target-Win64-Development"
    $Stdout = Join-Path $AttemptRoot "$Stem.stdout.log"
    $Stderr = Join-Path $AttemptRoot "$Stem.stderr.log"
    $Started = [DateTimeOffset]::UtcNow
    $Process = Start-Process -FilePath $BuildBat -ArgumentList @(
        $Target, 'Win64', 'Development', $Uproject,
        '-WaitMutex', '-NoUBA', '-MaxParallelActions=2') `
        -WorkingDirectory (Split-Path -Parent $BuildBat) -WindowStyle Hidden `
        -RedirectStandardOutput $Stdout -RedirectStandardError $Stderr -PassThru -Wait
    $Ended = [DateTimeOffset]::UtcNow
    $Result = [ordered]@{
        task_id = $TaskId; attempt_id = $AttemptId; target = $Target
        exit_code = $Process.ExitCode; started_utc = $Started.ToString('o')
        ended_utc = $Ended.ToString('o')
        duration_seconds = [math]::Round(($Ended - $Started).TotalSeconds, 3)
        passed = $Process.ExitCode -eq 0; stdout = $Stdout; stderr = $Stderr
    }
    [IO.File]::WriteAllText(
        (Join-Path $AttemptRoot "$Stem.result.json"),
        (($Result | ConvertTo-Json -Depth 8) + "`n"), $Utf8NoBom)
    if (-not $Result.passed) { throw "Build failed: $Target exit=$($Process.ExitCode)" }
    return [pscustomobject]$Result
}

$Results = @(
    Invoke-TargetBuild 'demo_mapEditor'
    Invoke-TargetBuild 'demo_map'
)
$Results | ConvertTo-Json -Depth 8
