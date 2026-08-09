param(
    [string]$AttemptId = 'attempt-002',
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
$ManifestTool = Join-Path $PSScriptRoot 'GetF0CanonicalManifest.ps1'
$Utf8NoBom = [System.Text.UTF8Encoding]::new($false)

if (Test-Path -LiteralPath $RunRoot) { throw "Startup attempt already exists: $RunRoot" }
if (-not (Test-Path -LiteralPath $CleanRoot -PathType Container)) { throw "Clean Package missing: $CleanRoot" }
New-Item -ItemType Directory -Path $RunRoot -Force | Out-Null
$CleanSummary = & $ManifestTool -Path $CleanRoot -OutputPath (Join-Path $RunRoot 'clean-package.manifest.txt')
$Copy = Start-Process -FilePath 'robocopy.exe' -ArgumentList @(
    $CleanRoot, $PackageRoot, '/E', '/COPY:DAT', '/DCOPY:DAT', '/R:1', '/W:1',
    '/NFL', '/NDL', '/NP', "/LOG:$(Join-Path $RunRoot 'copy.log')") `
    -WindowStyle Hidden -PassThru -Wait
if ($Copy.ExitCode -gt 7) { throw "Startup copy failed: robocopy $($Copy.ExitCode)" }
$CopySummary = & $ManifestTool -Path $PackageRoot -OutputPath (Join-Path $RunRoot 'pre-run-package.manifest.txt')
if ($CopySummary.ManifestSha256 -ne $CleanSummary.ManifestSha256) { throw 'Startup copy differs from clean Package.' }

$OuterExecutable = Join-Path $PackageRoot 'Windows\demo_map.exe'
$ExistingIds = @(Get-CimInstance Win32_Process | Where-Object Name -eq 'demo_map.exe' | Select-Object -ExpandProperty ProcessId)
$Started = [DateTimeOffset]::UtcNow
$Bootstrap = Start-Process -FilePath $OuterExecutable -WorkingDirectory (Split-Path -Parent $OuterExecutable) -PassThru
$WindowProcess = $null
$WindowTitle = ''
$Deadline = [DateTimeOffset]::UtcNow.AddSeconds(60)
do {
    Start-Sleep -Milliseconds 300
    $Candidates = @(Get-Process -Name demo_map -ErrorAction SilentlyContinue | Where-Object {
        $ExistingIds -notcontains $_.Id -and $_.Path -and
        $_.Path.StartsWith($PackageRoot, [StringComparison]::OrdinalIgnoreCase) -and $_.MainWindowHandle -ne 0
    })
    if ($Candidates.Count -gt 0) {
        $WindowProcess = $Candidates[0]
        $WindowTitle = $WindowProcess.MainWindowTitle
        break
    }
} while ([DateTimeOffset]::UtcNow -lt $Deadline)

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
$FatalCount = [regex]::Matches($LogText, '(?im)^\s*Fatal error:|Unhandled Exception|RecoveryRequired').Count
$Passed = $null -ne $WindowProcess -and
    $LogText -match 'Load map complete /Game/M01/Maps/L_M01_Expedition' -and
    $CloseAccepted -and -not $CleanupUsed -and $Residual.Count -eq 0 -and $FatalCount -eq 0
$Result = [ordered]@{
    task_id = $TaskId; attempt_id = $AttemptId; package_attempt_id = $PackageAttemptId
    case_id = 'Visible.DefaultStartupNoArguments'; arguments = @()
    clean_manifest_sha256 = $CleanSummary.ManifestSha256
    startup_copy_manifest_sha256 = $CopySummary.ManifestSha256
    visible_top_level_window = $null -ne $WindowProcess; window_title = $WindowTitle
    default_map_loaded = $LogText -match 'Load map complete /Game/M01/Maps/L_M01_Expedition'
    normal_close_accepted = $CloseAccepted; cleanup_used = $CleanupUsed
    fatal_or_recovery_count = $FatalCount; residual_processes = $Residual.Count
    passed = $Passed; started_utc = $Started.ToString('o')
    ended_utc = [DateTimeOffset]::UtcNow.ToString('o'); log_path = $LogPath
}
[IO.File]::WriteAllText((Join-Path $RunRoot 'results.json'),
    (($Result | ConvertTo-Json -Depth 12) + "`n"), $Utf8NoBom)
$Result | ConvertTo-Json -Depth 8
if (-not $Passed) { exit 1 }
