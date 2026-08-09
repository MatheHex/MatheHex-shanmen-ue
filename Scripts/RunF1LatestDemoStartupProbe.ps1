param(
    [string]$AttemptId = 'attempt-001'
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$TaskId = 'Dev.D.UE.0.0.9.F1.0.r0'
$ProjectRoot = Split-Path -Parent $PSScriptRoot
$TaskRoot = Join-Path $ProjectRoot "Saved\Automation\$TaskId"
$LatestRoot = Join-Path $ProjectRoot 'Latest_Demo'
$RunRoot = Join-Path $TaskRoot "Startup\$AttemptId"
$PackageRoot = Join-Path $RunRoot 'Package'
$ManifestTool = Join-Path $PSScriptRoot 'GetF0CanonicalManifest.ps1'
$Utf8NoBom = [System.Text.UTF8Encoding]::new($false)

if (Test-Path -LiteralPath $RunRoot) { throw "Startup attempt already exists: $RunRoot" }
if (-not (Test-Path -LiteralPath $LatestRoot -PathType Container)) { throw "Latest_Demo missing: $LatestRoot" }
New-Item -ItemType Directory -Path $RunRoot -Force | Out-Null

$LatestSummary = & $ManifestTool -Path $LatestRoot -OutputPath (Join-Path $RunRoot 'latest-before.manifest.txt')
$Copy = Start-Process -FilePath 'robocopy.exe' -ArgumentList @(
    $LatestRoot, $PackageRoot, '/E', '/COPY:DAT', '/DCOPY:DAT', '/R:1', '/W:1',
    '/NFL', '/NDL', '/NP', "/LOG:$(Join-Path $RunRoot 'copy.log')") `
    -WindowStyle Hidden -PassThru -Wait
if ($Copy.ExitCode -gt 7) { throw "Startup copy failed: robocopy $($Copy.ExitCode)" }
$CopySummary = & $ManifestTool -Path $PackageRoot -OutputPath (Join-Path $RunRoot 'startup-copy.manifest.txt')
if ($CopySummary.Files -ne $LatestSummary.Files -or $CopySummary.Bytes -ne $LatestSummary.Bytes -or $CopySummary.ManifestSha256 -ne $LatestSummary.ManifestSha256) {
    throw 'Startup copy differs from Latest_Demo.'
}

$OuterExecutable = Join-Path $PackageRoot 'Windows\demo_map.exe'
if (-not (Test-Path -LiteralPath $OuterExecutable -PathType Leaf)) { throw "Startup executable missing: $OuterExecutable" }
$ExistingIds = @(Get-CimInstance Win32_Process | Where-Object Name -eq 'demo_map.exe' | Select-Object -ExpandProperty ProcessId)
$Started = [DateTimeOffset]::UtcNow
$Bootstrap = Start-Process -FilePath $OuterExecutable -WorkingDirectory (Split-Path -Parent $OuterExecutable) -PassThru
$WindowProcess = $null
$WindowTitle = ''
$WindowDeadline = [DateTimeOffset]::UtcNow.AddSeconds(60)
do {
    Start-Sleep -Milliseconds 300
    $Candidates = @(Get-Process -Name demo_map -ErrorAction SilentlyContinue | Where-Object {
        $ExistingIds -notcontains $_.Id -and $_.Path -and $_.Path.StartsWith($PackageRoot, [StringComparison]::OrdinalIgnoreCase) -and $_.MainWindowHandle -ne 0
    })
    if ($Candidates.Count -gt 0) {
        $WindowProcess = $Candidates[0]
        $WindowTitle = $WindowProcess.MainWindowTitle
        break
    }
} while ([DateTimeOffset]::UtcNow -lt $WindowDeadline)

$LogText = ''
$LogPath = ''
$LogDeadline = [DateTimeOffset]::UtcNow.AddSeconds(35)
do {
    $RecentLogs = @(Get-ChildItem -LiteralPath $PackageRoot -Filter '*.log' -File -Recurse -ErrorAction SilentlyContinue |
        Where-Object LastWriteTimeUtc -ge $Started.UtcDateTime.AddSeconds(-2) |
        Sort-Object LastWriteTimeUtc -Descending)
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
    $Residual = @(Get-CimInstance Win32_Process | Where-Object {
        $_.ExecutablePath -and $_.ExecutablePath.StartsWith($PackageRoot, [StringComparison]::OrdinalIgnoreCase)
    })
} while ($Residual.Count -ne 0 -and [DateTimeOffset]::UtcNow -lt $ExitDeadline)
$CleanupUsed = $false
if ($Residual.Count -ne 0) {
    $CleanupUsed = $true
    foreach ($Owned in $Residual) { Stop-Process -Id $Owned.ProcessId -Force -ErrorAction SilentlyContinue }
    Start-Sleep -Seconds 1
    $Residual = @(Get-CimInstance Win32_Process | Where-Object {
        $_.ExecutablePath -and $_.ExecutablePath.StartsWith($PackageRoot, [StringComparison]::OrdinalIgnoreCase)
    })
}

$LatestAfter = & $ManifestTool -Path $LatestRoot -OutputPath (Join-Path $RunRoot 'latest-after.manifest.txt')
$FatalCount = [regex]::Matches($LogText, '(?im)^\s*Fatal error:|Unhandled Exception|RecoveryRequired').Count
$Passed = $null -ne $WindowProcess -and
    $LogText -match 'Load map complete /Game/M01/Maps/L_M01_Expedition' -and
    $CloseAccepted -and -not $CleanupUsed -and $Residual.Count -eq 0 -and $FatalCount -eq 0 -and
    $LatestAfter.Files -eq $LatestSummary.Files -and $LatestAfter.Bytes -eq $LatestSummary.Bytes -and $LatestAfter.ManifestSha256 -eq $LatestSummary.ManifestSha256
$Result = [ordered]@{
    task_id = $TaskId; attempt_id = $AttemptId; case_id = 'Visible.LatestDemoDefaultStartupNoArguments'; arguments = @()
    latest_before_manifest_sha256 = $LatestSummary.ManifestSha256; startup_copy_manifest_sha256 = $CopySummary.ManifestSha256; latest_after_manifest_sha256 = $LatestAfter.ManifestSha256
    visible_top_level_window = $null -ne $WindowProcess; window_title = $WindowTitle
    default_map_loaded = $LogText -match 'Load map complete /Game/M01/Maps/L_M01_Expedition'
    normal_close_accepted = $CloseAccepted; cleanup_used = $CleanupUsed
    fatal_or_recovery_count = $FatalCount; residual_processes = $Residual.Count
    latest_unchanged = $LatestAfter.Files -eq $LatestSummary.Files -and $LatestAfter.Bytes -eq $LatestSummary.Bytes -and $LatestAfter.ManifestSha256 -eq $LatestSummary.ManifestSha256
    passed = $Passed; started_utc = $Started.ToString('o'); ended_utc = [DateTimeOffset]::UtcNow.ToString('o'); log_path = $LogPath
}
[IO.File]::WriteAllText((Join-Path $RunRoot 'results.json'), (($Result | ConvertTo-Json -Depth 12) + "`n"), $Utf8NoBom)
$Result | ConvertTo-Json -Depth 12
if (-not $Passed) { exit 1 }
