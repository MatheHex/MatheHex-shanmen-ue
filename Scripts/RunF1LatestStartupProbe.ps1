param([string]$AttemptId = 'attempt-001')

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$TaskId = 'Dev.D.UE.0.0.8.F1.0.r0'
$ActiveRoot = Split-Path -Parent $PSScriptRoot
$CandidateRoot = 'C:\AIDev\shanmen-ue\Builds\demo_map\Candidate\20260801T110153Z-Dev.D.UE.0.0.8.F0.0.r0'
$LatestRoot = Join-Path $ActiveRoot 'Latest_Demo'
$RunRoot = Join-Path $ActiveRoot "Saved\Automation\$TaskId\VisibleSmoke\$AttemptId"
$CopyRoot = Join-Path $RunRoot 'IndependentCopy'
$ManifestTool = Join-Path $PSScriptRoot 'GetF0CanonicalManifest.ps1'
$Utf8NoBom = [Text.UTF8Encoding]::new($false)

if (Test-Path -LiteralPath $RunRoot) { throw "Smoke attempt already exists: $RunRoot" }
if (-not (Test-Path -LiteralPath $LatestRoot -PathType Container)) { throw "Latest_Demo missing: $LatestRoot" }
New-Item -ItemType Directory -Path $RunRoot -Force | Out-Null
$CandidatePre = & $ManifestTool -Path $CandidateRoot -OutputPath (Join-Path $RunRoot 'candidate.pre.manifest.txt')
$LatestPre = & $ManifestTool -Path $LatestRoot -OutputPath (Join-Path $RunRoot 'latest.pre.manifest.txt')
if ($LatestPre.Files -ne $CandidatePre.Files -or $LatestPre.Bytes -ne $CandidatePre.Bytes -or $LatestPre.ManifestSha256 -ne $CandidatePre.ManifestSha256) {
    throw 'Latest_Demo differs from Candidate before startup.'
}
$Copy = Start-Process -FilePath 'robocopy.exe' -ArgumentList @(
    $LatestRoot,$CopyRoot,'/E','/COPY:DAT','/DCOPY:DAT','/R:1','/W:1','/XJ','/NP','/NFL','/NDL',
    "/LOG:$(Join-Path $RunRoot 'latest-to-independent-copy.robocopy.log')") -WindowStyle Hidden -PassThru -Wait
if ($Copy.ExitCode -gt 7) { throw "Independent copy failed: robocopy $($Copy.ExitCode)" }
$CopyPre = & $ManifestTool -Path $CopyRoot -OutputPath (Join-Path $RunRoot 'independent-copy.pre.manifest.txt')
if ($CopyPre.Files -ne $CandidatePre.Files -or $CopyPre.Bytes -ne $CandidatePre.Bytes -or $CopyPre.ManifestSha256 -ne $CandidatePre.ManifestSha256) {
    throw 'Independent copy differs from Candidate.'
}

$Executable = Join-Path $CopyRoot 'Windows\demo_map.exe'
$BeforeIds = @(Get-CimInstance Win32_Process | Where-Object Name -eq 'demo_map.exe' | Select-Object -ExpandProperty ProcessId)
$Started = [DateTimeOffset]::UtcNow
$Bootstrap = Start-Process -FilePath $Executable -WorkingDirectory (Split-Path -Parent $Executable) -PassThru
$WindowProcess = $null
$WindowTitle = ''
$Deadline = [DateTimeOffset]::UtcNow.AddSeconds(60)
do {
    Start-Sleep -Milliseconds 250
    $Candidates = @(Get-Process -Name demo_map -ErrorAction SilentlyContinue | Where-Object {
        $BeforeIds -notcontains $_.Id -and $_.Path -and $_.Path.StartsWith($CopyRoot,[StringComparison]::OrdinalIgnoreCase) -and $_.MainWindowHandle -ne 0
    })
    if ($Candidates.Count -gt 0) { $WindowProcess=$Candidates[0]; $WindowTitle=$WindowProcess.MainWindowTitle; break }
} while ([DateTimeOffset]::UtcNow -lt $Deadline)

$LogText=''; $LogPath=''; $DefaultMapLoaded=$false
$LogDeadline=[DateTimeOffset]::UtcNow.AddSeconds(35)
do {
    $Logs=@(Get-ChildItem -LiteralPath $CopyRoot -Filter '*.log' -File -Recurse -ErrorAction SilentlyContinue | Where-Object {
        $_.LastWriteTimeUtc -ge $Started.UtcDateTime.AddSeconds(-2)
    } | Sort-Object LastWriteTimeUtc -Descending)
    if($Logs.Count -gt 0){$LogPath=$Logs[0].FullName; try{$LogText=Get-Content -Raw -LiteralPath $LogPath}catch{$LogText=''}}
    $DefaultMapLoaded=$LogText -match 'Load map complete /Game/M01/Maps/L_M01_Expedition'
    if($DefaultMapLoaded){break}
    Start-Sleep -Milliseconds 500
} while([DateTimeOffset]::UtcNow -lt $LogDeadline)

$CloseAccepted=$false
if($WindowProcess){$CloseAccepted=$WindowProcess.CloseMainWindow()}
$ExitDeadline=[DateTimeOffset]::UtcNow.AddSeconds(20)
do{
    Start-Sleep -Milliseconds 250
    $Residual=@(Get-CimInstance Win32_Process | Where-Object {$_.ExecutablePath -and $_.ExecutablePath.StartsWith($CopyRoot,[StringComparison]::OrdinalIgnoreCase)})
}while($Residual.Count -ne 0 -and [DateTimeOffset]::UtcNow -lt $ExitDeadline)
$CleanupUsed=$false
if($Residual.Count -ne 0){
    $CleanupUsed=$true
    foreach($Owned in $Residual){Stop-Process -Id $Owned.ProcessId -Force -ErrorAction SilentlyContinue}
    Start-Sleep -Seconds 1
    $Residual=@(Get-CimInstance Win32_Process | Where-Object {$_.ExecutablePath -and $_.ExecutablePath.StartsWith($CopyRoot,[StringComparison]::OrdinalIgnoreCase)})
}
$FatalCount=[regex]::Matches($LogText,'(?im)^\s*Fatal error:|Unhandled Exception|RecoveryRequired').Count
$CandidatePost=& $ManifestTool -Path $CandidateRoot -OutputPath (Join-Path $RunRoot 'candidate.post.manifest.txt')
$LatestPost=& $ManifestTool -Path $LatestRoot -OutputPath (Join-Path $RunRoot 'latest.post.manifest.txt')
$CandidateUnchanged=$CandidatePost.Files -eq $CandidatePre.Files -and $CandidatePost.Bytes -eq $CandidatePre.Bytes -and $CandidatePost.ManifestSha256 -eq $CandidatePre.ManifestSha256
$LatestUnchanged=$LatestPost.Files -eq $LatestPre.Files -and $LatestPost.Bytes -eq $LatestPre.Bytes -and $LatestPost.ManifestSha256 -eq $LatestPre.ManifestSha256
$Passed=$null -ne $WindowProcess -and $DefaultMapLoaded -and $CloseAccepted -and -not $CleanupUsed -and $Residual.Count -eq 0 -and $FatalCount -eq 0 -and $CandidateUnchanged -and $LatestUnchanged
$Result=[ordered]@{
    task_id=$TaskId;attempt_id=$AttemptId;case_id='Lightweight.VisibleDefaultStartup';source=$LatestRoot;startup_copy=$CopyRoot
    executable=$Executable;arguments=@();candidate_never_run=$true;candidate_unchanged=$CandidateUnchanged
    latest_demo_never_run=$true;latest_demo_unchanged=$LatestUnchanged;independent_copy_verified=$true
    visible_top_level_window=($null -ne $WindowProcess);window_title=$WindowTitle;default_map_loaded=$DefaultMapLoaded
    normal_close_accepted=$CloseAccepted;fatal_unhandled_or_recovery_count=$FatalCount;cleanup_used=$CleanupUsed
    residual_processes=$Residual.Count;passed=$Passed;started_utc=$Started.ToString('o');ended_utc=[DateTimeOffset]::UtcNow.ToString('o');log_path=$LogPath
}
[IO.File]::WriteAllText((Join-Path $RunRoot 'result.json'),(($Result|ConvertTo-Json -Depth 8)+"`n"),$Utf8NoBom)
$Result|ConvertTo-Json -Depth 8
if(-not $Passed){exit 2}
