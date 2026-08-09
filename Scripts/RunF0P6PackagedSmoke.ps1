param(
    [string]$AttemptId = 'attempt-003',
    [string]$PackageAttemptId = 'attempt-003'
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$TaskId = 'Dev.D.UE.0.0.9.F0.0.r0'
$ProjectRoot = Split-Path -Parent $PSScriptRoot
$TaskRoot = Join-Path $ProjectRoot "Saved\Automation\$TaskId"
$CleanRoot = Join-Path $TaskRoot "PackageBuild\$PackageAttemptId\Package"
$RunRoot = Join-Path $TaskRoot "Smoke\P6Diagnostic-$AttemptId"
$ProductRoot = Join-Path $TaskRoot "ProductSmoke\P6Diagnostic-$AttemptId"
$PackageRoot = Join-Path $RunRoot 'Package'
$ManifestTool = Join-Path $PSScriptRoot 'GetF0CanonicalManifest.ps1'
$Utf8NoBom = [System.Text.UTF8Encoding]::new($false)

if (Test-Path -LiteralPath $RunRoot) { throw "P6 smoke attempt already exists: $RunRoot" }
if (-not (Test-Path -LiteralPath $CleanRoot -PathType Container)) { throw "Clean Package missing: $CleanRoot" }
New-Item -ItemType Directory -Path $RunRoot,$ProductRoot -Force | Out-Null
$Clean = & $ManifestTool -Path $CleanRoot -OutputPath (Join-Path $RunRoot 'clean-package.manifest.txt')
$Copy = Start-Process -FilePath 'robocopy.exe' -ArgumentList @(
    $CleanRoot, $PackageRoot, '/E', '/COPY:DAT', '/DCOPY:DAT', '/R:1', '/W:1',
    '/NFL', '/NDL', '/NP', "/LOG:$(Join-Path $RunRoot 'copy.log')") `
    -WindowStyle Hidden -PassThru -Wait
if ($Copy.ExitCode -gt 7) { throw "P6 smoke copy failed: robocopy $($Copy.ExitCode)" }
$CopySummary = & $ManifestTool -Path $PackageRoot -OutputPath (Join-Path $RunRoot 'pre-run-package.manifest.txt')
if ($CopySummary.Files -ne $Clean.Files -or $CopySummary.Bytes -ne $Clean.Bytes -or $CopySummary.ManifestSha256 -ne $Clean.ManifestSha256) {
    throw 'P6 smoke copy differs from clean Package.'
}

$CaseRoot = Join-Path $ProductRoot 'P6DualLoot'
$UserRoot = Join-Path $CaseRoot 'UserDir'
$VisualRoot = Join-Path $CaseRoot 'Visual'
$LogPath = Join-Path $CaseRoot 'P6DualLoot.log'
New-Item -ItemType Directory -Path $UserRoot,$VisualRoot -Force | Out-Null
$Executable = Join-Path $PackageRoot 'Windows\demo_map\Binaries\Win64\demo_map.exe'
$Started = [DateTimeOffset]::UtcNow
$Process = Start-Process -FilePath $Executable -ArgumentList @(
    '-unattended','-nop4','-nosplash','-nosound','-NoCrashDialog','-RenderOffScreen',
    '-ResX=1600','-ResY=900','-P6DualLootVisibleAcceptance',
    "-P6VisibleOutput=$VisualRoot", "-UserDir=$UserRoot", "-abslog=$LogPath") `
    -WorkingDirectory (Split-Path -Parent $Executable) -WindowStyle Hidden -PassThru
$Completed = $Process.WaitForExit(300000)
$CleanupUsed = $false
if (-not $Completed) {
    $CleanupUsed = $true
    Stop-Process -Id $Process.Id -Force -ErrorAction SilentlyContinue
    $Process.WaitForExit(10000) | Out-Null
}
$Process.Refresh()
$Text = if (Test-Path -LiteralPath $LogPath) { Get-Content -Raw -LiteralPath $LogPath } else { '' }
$SuccessCount = [regex]::Matches($Text, 'P6_DUAL_LOOT_VISIBLE_ACCEPTANCE: PASS\.').Count
$FailureCount = [regex]::Matches($Text, 'P6_DUAL_LOOT_VISIBLE_ACCEPTANCE: FAIL').Count
$FatalCount = [regex]::Matches($Text, '(?im)^\s*Fatal error:|Unhandled Exception|RecoveryRequired').Count
$Residual = @(Get-CimInstance Win32_Process | Where-Object {
    $_.ExecutablePath -and $_.ExecutablePath.StartsWith($PackageRoot, [StringComparison]::OrdinalIgnoreCase)
})
$Result = [ordered]@{
    task_id = $TaskId; attempt_id = $AttemptId; package_attempt_id = $PackageAttemptId
    case_id = 'P6.PointerDualLootAndInputRestore'
    exit_code = if ($Completed) { $Process.ExitCode } else { -1 }
    timeout = -not $Completed; cleanup_used = $CleanupUsed
    success_marker_count = $SuccessCount; failure_marker_count = $FailureCount
    fatal_or_recovery_count = $FatalCount; residual_processes = $Residual.Count
    passed = $Completed -and $Process.ExitCode -eq 0 -and $SuccessCount -eq 1 -and $FailureCount -eq 0 -and $FatalCount -eq 0 -and $Residual.Count -eq 0
    started_utc = $Started.ToString('o'); ended_utc = [DateTimeOffset]::UtcNow.ToString('o')
    log_path = $LogPath; clean_manifest_sha256 = $Clean.ManifestSha256
    smoke_copy_manifest_sha256 = $CopySummary.ManifestSha256
}
[IO.File]::WriteAllText((Join-Path $RunRoot 'results.json'), (($Result | ConvertTo-Json -Depth 12) + "`n"), $Utf8NoBom)
$Result | ConvertTo-Json -Depth 12
if (-not $Result.passed) { exit 1 }
