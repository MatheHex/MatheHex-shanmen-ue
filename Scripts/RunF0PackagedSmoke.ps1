param(
    [string]$AttemptId = 'attempt-001',
    [string]$PackageAttemptId = 'attempt-001'
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$TaskId = 'Dev.D.UE.0.0.9.F0.0.r0'
$ProjectRoot = Split-Path -Parent $PSScriptRoot
$TaskRoot = Join-Path $ProjectRoot "Saved\Automation\$TaskId"
$CleanRoot = Join-Path $TaskRoot "PackageBuild\$PackageAttemptId\Package"
$RunRoot = Join-Path $TaskRoot "Smoke\$AttemptId"
$PackageRoot = Join-Path $RunRoot 'Package'
$ProductRoot = Join-Path $TaskRoot "ProductSmoke\$AttemptId"
$ManifestTool = Join-Path $PSScriptRoot 'GetF0CanonicalManifest.ps1'
$Utf8NoBom = [System.Text.UTF8Encoding]::new($false)

if (Test-Path -LiteralPath $RunRoot) { throw "Smoke attempt already exists: $RunRoot" }
if (-not (Test-Path -LiteralPath $CleanRoot -PathType Container)) { throw "Clean Package missing: $CleanRoot" }
$ResolvedTask = [IO.Path]::GetFullPath($TaskRoot).TrimEnd('\') + '\'
$ResolvedRun = [IO.Path]::GetFullPath($RunRoot).TrimEnd('\') + '\'
if (-not $ResolvedRun.StartsWith($ResolvedTask, [StringComparison]::OrdinalIgnoreCase)) {
    throw "Smoke target escaped task root: $RunRoot"
}
New-Item -ItemType Directory -Path $RunRoot,$ProductRoot -Force | Out-Null

$CleanSummary = & $ManifestTool -Path $CleanRoot -OutputPath (Join-Path $RunRoot 'clean-package.manifest.txt')
$Copy = Start-Process -FilePath 'robocopy.exe' -ArgumentList @(
    $CleanRoot, $PackageRoot, '/E', '/COPY:DAT', '/DCOPY:DAT', '/R:1', '/W:1',
    '/NFL', '/NDL', '/NP', "/LOG:$(Join-Path $RunRoot 'copy.log')") `
    -WindowStyle Hidden -PassThru -Wait
if ($Copy.ExitCode -gt 7) { throw "Smoke copy failed: robocopy $($Copy.ExitCode)" }
$CopySummary = & $ManifestTool -Path $PackageRoot -OutputPath (Join-Path $RunRoot 'pre-run-package.manifest.txt')
if ($CopySummary.Files -ne $CleanSummary.Files -or $CopySummary.Bytes -ne $CleanSummary.Bytes -or
    $CopySummary.ManifestSha256 -ne $CleanSummary.ManifestSha256) {
    throw 'Smoke copy differs from clean Package.'
}

$Executable = Join-Path $PackageRoot 'Windows\demo_map\Binaries\Win64\demo_map.exe'
$OuterExecutable = Join-Path $PackageRoot 'Windows\demo_map.exe'
$Results = [Collections.Generic.List[object]]::new()

function Invoke-MarkerCase {
    param(
        [Parameter(Mandatory = $true)][string]$CaseId,
        [Parameter(Mandatory = $true)][string[]]$Arguments,
        [Parameter(Mandatory = $true)][string]$LogPath,
        [Parameter(Mandatory = $true)][string]$SuccessPattern,
        [Parameter(Mandatory = $true)][string]$FailurePattern
    )
    $Parent = Split-Path -Parent $LogPath
    New-Item -ItemType Directory -Path $Parent -Force | Out-Null
    $Started = [DateTimeOffset]::UtcNow
    $Process = Start-Process -FilePath $Executable -ArgumentList $Arguments `
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
    $SuccessCount = [regex]::Matches($Text, $SuccessPattern).Count
    $FailureCount = [regex]::Matches($Text, $FailurePattern).Count
    $FatalCount = [regex]::Matches($Text, '(?im)^\s*Fatal error:|Unhandled Exception|RecoveryRequired').Count
    $Residual = @(Get-CimInstance Win32_Process | Where-Object {
        $_.ExecutablePath -and $_.ExecutablePath.StartsWith($PackageRoot, [StringComparison]::OrdinalIgnoreCase)
    })
    $Exit = if ($Completed) { $Process.ExitCode } else { -1 }
    $Passed = $Completed -and $Exit -eq 0 -and $SuccessCount -eq 1 -and
        $FailureCount -eq 0 -and $FatalCount -eq 0 -and $Residual.Count -eq 0
    $Result = [ordered]@{
        case_id = $CaseId; arguments = $Arguments; exit_code = $Exit
        timeout = -not $Completed; cleanup_used = $CleanupUsed
        success_marker_count = $SuccessCount; failure_marker_count = $FailureCount
        fatal_or_recovery_count = $FatalCount; residual_processes = $Residual.Count
        passed = $Passed; started_utc = $Started.ToString('o')
        ended_utc = [DateTimeOffset]::UtcNow.ToString('o'); log_path = $LogPath
    }
    [IO.File]::WriteAllText((Join-Path $Parent 'result.json'),
        (($Result | ConvertTo-Json -Depth 12) + "`n"), $Utf8NoBom)
    $script:Results.Add([pscustomobject]$Result)
    Write-Host "DONE $CaseId passed=$Passed exit=$Exit marker=$SuccessCount"
    if (-not $Passed) { throw "Packaged smoke failed: $CaseId" }
}

$IntegrationRoot = Join-Path $ProductRoot 'M01Integration'
$IntegrationUser = Join-Path $IntegrationRoot 'UserDir'
$IntegrationVisual = Join-Path $IntegrationRoot 'Visual'
$IntegrationLog = Join-Path $IntegrationRoot 'M01Integration.log'
New-Item -ItemType Directory -Path $IntegrationUser,$IntegrationVisual -Force | Out-Null
$IntegrationArgs = @(
    '-unattended','-nop4','-nosplash','-nosound','-NoCrashDialog','-RenderOffScreen',
    '-ResX=1280','-ResY=720','-M01IntegrationVisibleSmoke',
    "-M01VisualOutput=$IntegrationVisual", "-UserDir=$IntegrationUser", "-abslog=$IntegrationLog")
Invoke-MarkerCase -CaseId 'M01.IntegrationAndThreeExtractions' -Arguments $IntegrationArgs `
    -LogPath $IntegrationLog `
    -SuccessPattern 'M01_INTEGRATION_VISIBLE_SMOKE: PASS theme=1 low_regular=1 high_boss=1 discard_spatial=1 settlement_once=1 new_run_reset=1 duplicates=0 input_lock=0 sources=149 base_value=115500\.' `
    -FailurePattern 'M01_(?:INTEGRATION|EXTRACTION)_VISIBLE_SMOKE: FAIL'

$EnemyRoot = Join-Path $ProductRoot 'M01EnemyReward'
$EnemyUser = Join-Path $EnemyRoot 'UserDir'
$EnemyLog = Join-Path $EnemyRoot 'M01EnemyReward.log'
New-Item -ItemType Directory -Path $EnemyUser -Force | Out-Null
$EnemyArgs = @(
    '-unattended','-nop4','-nosplash','-nullrhi','-nosound','-NoCrashDialog',
    '-M01EnemyVisibleSmoke', "-UserDir=$EnemyUser", "-abslog=$EnemyLog")
Invoke-MarkerCase -CaseId 'M01.EnemySearchBossRewardAndCleanup' -Arguments $EnemyArgs `
    -LogPath $EnemyLog `
    -SuccessPattern 'M01_ENEMY_VISIBLE_SMOKE: PASS sources=149 generated_containers=135 search_same_guid_take=PASS generated_corpse_types=3 boss_equipment=required boss_notify_once=1 boss_exit=unlocked carry_settlement=PASS terminal_cleanup=PASS\.' `
    -FailurePattern 'M01_ENEMY_VISIBLE_SMOKE: FAIL'

$P6Root = Join-Path $ProductRoot 'P6DualLoot'
$P6User = Join-Path $P6Root 'UserDir'
$P6Visual = Join-Path $P6Root 'Visual'
$P6Log = Join-Path $P6Root 'P6DualLoot.log'
New-Item -ItemType Directory -Path $P6User,$P6Visual -Force | Out-Null
$P6Args = @(
    '-unattended','-nop4','-nosplash','-nosound','-NoCrashDialog','-RenderOffScreen',
    '-ResX=1600','-ResY=900','-P6DualLootVisibleAcceptance',
    "-P6VisibleOutput=$P6Visual", "-UserDir=$P6User", "-abslog=$P6Log")
Invoke-MarkerCase -CaseId 'P6.PointerDualLootAndInputRestore' -Arguments $P6Args `
    -LogPath $P6Log `
    -SuccessPattern 'P6_DUAL_LOOT_VISIBLE_ACCEPTANCE: PASS\.' `
    -FailurePattern 'P6_DUAL_LOOT_VISIBLE_ACCEPTANCE: FAIL'

$VisibleRoot = Join-Path $RunRoot 'VisibleDefaultStartup'
New-Item -ItemType Directory -Path $VisibleRoot -Force | Out-Null
$ExistingIds = @(Get-CimInstance Win32_Process | Where-Object Name -eq 'demo_map.exe' | Select-Object -ExpandProperty ProcessId)
$VisibleStarted = [DateTimeOffset]::UtcNow
$Bootstrap = Start-Process -FilePath $OuterExecutable -WorkingDirectory (Split-Path -Parent $OuterExecutable) -PassThru
$WindowProcess = $null
$Deadline = [DateTimeOffset]::UtcNow.AddSeconds(60)
do {
    Start-Sleep -Milliseconds 300
    $Candidates = @(Get-Process -Name demo_map -ErrorAction SilentlyContinue | Where-Object {
        $ExistingIds -notcontains $_.Id -and $_.Path -and
        $_.Path.StartsWith($PackageRoot, [StringComparison]::OrdinalIgnoreCase) -and $_.MainWindowHandle -ne 0
    })
    if ($Candidates.Count -gt 0) { $WindowProcess = $Candidates[0]; break }
} while ([DateTimeOffset]::UtcNow -lt $Deadline)

$LogText = ''
$LogPath = ''
$LocalLogRoot = Join-Path $env:LOCALAPPDATA 'demo_map\Saved\Logs'
$LogDeadline = [DateTimeOffset]::UtcNow.AddSeconds(35)
do {
    $RecentLogs = @(
        Get-ChildItem -LiteralPath $LocalLogRoot -Filter '*.log' -File -ErrorAction SilentlyContinue
        Get-ChildItem -LiteralPath $PackageRoot -Filter '*.log' -File -Recurse -ErrorAction SilentlyContinue
    ) | Where-Object {
        $_.LastWriteTimeUtc -ge $VisibleStarted.UtcDateTime.AddSeconds(-2)
    } | Sort-Object LastWriteTimeUtc -Descending
    if ($RecentLogs.Count -gt 0) {
        $LogPath = $RecentLogs[0].FullName
        try { $LogText = Get-Content -Raw -LiteralPath $LogPath } catch { $LogText = '' }
    }
    if ($LogText -match 'Load map complete /Game/M01/Maps/L_M01_Expedition') { break }
    Start-Sleep -Milliseconds 500
} while ([DateTimeOffset]::UtcNow -lt $LogDeadline)

$CloseAccepted = $false
if ($WindowProcess) { $CloseAccepted = $WindowProcess.CloseMainWindow() }
$ExitDeadline = [DateTimeOffset]::UtcNow.AddSeconds(20)
do {
    Start-Sleep -Milliseconds 250
    $VisibleResidual = @(Get-CimInstance Win32_Process | Where-Object {
        $_.ExecutablePath -and $_.ExecutablePath.StartsWith($PackageRoot, [StringComparison]::OrdinalIgnoreCase)
    })
} while ($VisibleResidual.Count -ne 0 -and [DateTimeOffset]::UtcNow -lt $ExitDeadline)
$CleanupUsed = $false
if ($VisibleResidual.Count -ne 0) {
    $CleanupUsed = $true
    foreach ($Owned in $VisibleResidual) { Stop-Process -Id $Owned.ProcessId -Force -ErrorAction SilentlyContinue }
    Start-Sleep -Seconds 1
    $VisibleResidual = @(Get-CimInstance Win32_Process | Where-Object {
        $_.ExecutablePath -and $_.ExecutablePath.StartsWith($PackageRoot, [StringComparison]::OrdinalIgnoreCase)
    })
}
$VisibleFatal = [regex]::Matches($LogText, '(?im)^\s*Fatal error:|Unhandled Exception|RecoveryRequired').Count
$VisiblePassed = $null -ne $WindowProcess -and
    $LogText -match 'Load map complete /Game/M01/Maps/L_M01_Expedition' -and
    $CloseAccepted -and -not $CleanupUsed -and $VisibleResidual.Count -eq 0 -and $VisibleFatal -eq 0
$VisibleResult = [ordered]@{
    case_id = 'Visible.DefaultStartupNoArguments'; arguments = @()
    visible_top_level_window = $null -ne $WindowProcess
    window_title = if ($WindowProcess) { $WindowProcess.MainWindowTitle } else { '' }
    default_map_loaded = $LogText -match 'Load map complete /Game/M01/Maps/L_M01_Expedition'
    normal_close_accepted = $CloseAccepted; cleanup_used = $CleanupUsed
    fatal_or_recovery_count = $VisibleFatal; residual_processes = $VisibleResidual.Count
    passed = $VisiblePassed; started_utc = $VisibleStarted.ToString('o')
    ended_utc = [DateTimeOffset]::UtcNow.ToString('o'); log_path = $LogPath
}
[IO.File]::WriteAllText((Join-Path $VisibleRoot 'result.json'),
    (($VisibleResult | ConvertTo-Json -Depth 12) + "`n"), $Utf8NoBom)
$Results.Add([pscustomobject]$VisibleResult)
Write-Host "DONE Visible.DefaultStartupNoArguments passed=$VisiblePassed"

$Summary = [ordered]@{
    task_id = $TaskId; attempt_id = $AttemptId; package_attempt_id = $PackageAttemptId
    clean_package = $CleanRoot; clean_files = $CleanSummary.Files; clean_bytes = $CleanSummary.Bytes
    clean_manifest_sha256 = $CleanSummary.ManifestSha256
    smoke_copy_manifest_sha256 = $CopySummary.ManifestSha256
    expected = 4; executed = $Results.Count
    passed_count = @($Results | Where-Object passed).Count
    failed_count = @($Results | Where-Object { -not $_.passed }).Count
    passed = $Results.Count -eq 4 -and @($Results | Where-Object { -not $_.passed }).Count -eq 0
    results = @($Results)
}
[IO.File]::WriteAllText((Join-Path $RunRoot 'results.json'),
    (($Summary | ConvertTo-Json -Depth 20) + "`n"), $Utf8NoBom)
$Summary | ConvertTo-Json -Depth 10
if (-not $Summary.passed) { exit 1 }
