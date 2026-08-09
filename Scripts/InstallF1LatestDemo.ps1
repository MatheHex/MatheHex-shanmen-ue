param(
    [string]$CandidateRoot = 'C:\AIDev\shanmen-ue\Builds\demo_map\Candidate\20260801T110153Z-Dev.D.UE.0.0.8.F0.0.r0'
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$TaskId = 'Dev.D.UE.0.0.8.F1.0.r0'
$ActiveRoot = Split-Path -Parent $PSScriptRoot
$TaskRoot = Join-Path $ActiveRoot "Saved\Automation\$TaskId"
$AttemptRoot = Join-Path $TaskRoot 'LatestSwitch\attempt-001'
$ManifestTool = Join-Path $PSScriptRoot 'GetF0CanonicalManifest.ps1'
$LatestRoot = Join-Path $ActiveRoot 'Latest_Demo'
$BackupParent = Join-Path $ActiveRoot 'Latest_Demo.Backups'
$Timestamp = [DateTime]::UtcNow.ToString('yyyyMMddTHHmmssZ')
$StagingRoot = Join-Path $ActiveRoot ".Latest_Demo.staging-$Timestamp-$TaskId"
$BackupRoot = Join-Path $BackupParent "$Timestamp-pre-$TaskId"
$ExpectedFiles = 48
$ExpectedBytes = 1085427808
$ExpectedManifest = 'B4913B74CBDA064BED68DFEBA409150CA442C49B73F18F029FA7F9478D2F1802'
$Utf8NoBom = [Text.UTF8Encoding]::new($false)

if (-not (Test-Path -LiteralPath $CandidateRoot -PathType Container)) { throw "Candidate missing: $CandidateRoot" }
if (Test-Path -LiteralPath $AttemptRoot) { throw "Attempt already exists: $AttemptRoot" }
if (Test-Path -LiteralPath $StagingRoot) { throw "Staging already exists: $StagingRoot" }
if (Test-Path -LiteralPath $BackupRoot) { throw "Backup already exists: $BackupRoot" }
$ResolvedActive = [IO.Path]::GetFullPath($ActiveRoot).TrimEnd('\') + '\'
foreach ($Target in @($LatestRoot, $BackupParent, $StagingRoot, $BackupRoot)) {
    $ResolvedTarget = [IO.Path]::GetFullPath($Target).TrimEnd('\') + '\'
    if (-not $ResolvedTarget.StartsWith($ResolvedActive, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Target escaped active root: $Target"
    }
}

New-Item -ItemType Directory -Path $AttemptRoot,$BackupParent -Force | Out-Null
$CandidatePre = & $ManifestTool -Path $CandidateRoot -OutputPath (Join-Path $AttemptRoot 'candidate.pre.manifest.txt')
if ($CandidatePre.Files -ne $ExpectedFiles -or $CandidatePre.Bytes -ne $ExpectedBytes -or $CandidatePre.ManifestSha256 -ne $ExpectedManifest) {
    throw "Candidate identity mismatch: $($CandidatePre.Files)/$($CandidatePre.Bytes)/$($CandidatePre.ManifestSha256)"
}

$OldLatest = $null
if (Test-Path -LiteralPath $LatestRoot -PathType Container) {
    $OldLatest = & $ManifestTool -Path $LatestRoot -OutputPath (Join-Path $AttemptRoot 'old-latest.manifest.txt')
}

$Copy = Start-Process -FilePath 'robocopy.exe' -ArgumentList @(
    $CandidateRoot,$StagingRoot,'/E','/COPY:DAT','/DCOPY:DAT','/R:1','/W:1','/XJ','/NP','/NFL','/NDL',
    "/LOG:$(Join-Path $AttemptRoot 'candidate-to-staging.robocopy.log')") -WindowStyle Hidden -PassThru -Wait
if ($Copy.ExitCode -gt 7) { throw "Staging copy failed: robocopy $($Copy.ExitCode)" }
$Staging = & $ManifestTool -Path $StagingRoot -OutputPath (Join-Path $AttemptRoot 'staging.manifest.txt')
if ($Staging.Files -ne $CandidatePre.Files -or $Staging.Bytes -ne $CandidatePre.Bytes -or $Staging.ManifestSha256 -ne $CandidatePre.ManifestSha256) {
    throw 'Staging differs from Candidate.'
}

$OldLatestMoved = $false
try {
    if ($null -ne $OldLatest) {
        Move-Item -LiteralPath $LatestRoot -Destination $BackupRoot
        $OldLatestMoved = $true
    }
    Move-Item -LiteralPath $StagingRoot -Destination $LatestRoot
} catch {
    if ($OldLatestMoved -and -not (Test-Path -LiteralPath $LatestRoot) -and (Test-Path -LiteralPath $BackupRoot)) {
        Move-Item -LiteralPath $BackupRoot -Destination $LatestRoot
    }
    throw
}

$Latest = & $ManifestTool -Path $LatestRoot -OutputPath (Join-Path $AttemptRoot 'latest.product.manifest.txt')
if ($Latest.Files -ne $CandidatePre.Files -or $Latest.Bytes -ne $CandidatePre.Bytes -or $Latest.ManifestSha256 -ne $CandidatePre.ManifestSha256) {
    throw 'Installed Latest_Demo differs from Candidate.'
}

$Backup = $null
if ($OldLatestMoved) {
    $Backup = & $ManifestTool -Path $BackupRoot -OutputPath (Join-Path $AttemptRoot 'backup.manifest.txt')
    if ($Backup.Files -ne $OldLatest.Files -or $Backup.Bytes -ne $OldLatest.Bytes -or $Backup.ManifestSha256 -ne $OldLatest.ManifestSha256) {
        throw 'Backup differs from old Latest_Demo.'
    }
}

$CandidatePost = & $ManifestTool -Path $CandidateRoot -OutputPath (Join-Path $AttemptRoot 'candidate.post.manifest.txt')
if ($CandidatePost.Files -ne $CandidatePre.Files -or $CandidatePost.Bytes -ne $CandidatePre.Bytes -or $CandidatePost.ManifestSha256 -ne $CandidatePre.ManifestSha256) {
    throw 'Candidate changed during installation.'
}

$Executable = Join-Path $LatestRoot 'Windows\demo_map.exe'
if (-not (Test-Path -LiteralPath $Executable -PathType Leaf)) { throw "Executable missing: $Executable" }
$Summary = [ordered]@{
    task_id=$TaskId; timestamp_utc=$Timestamp; candidate_root=$CandidateRoot; candidate_never_run=$true
    candidate_files=$CandidatePre.Files; candidate_bytes=$CandidatePre.Bytes; candidate_manifest_sha256=$CandidatePre.ManifestSha256
    candidate_unchanged=$true; staging_verified=$true; latest_demo=$LatestRoot; executable=$Executable
    latest_product_files=$Latest.Files; latest_product_bytes=$Latest.Bytes; latest_product_manifest_sha256=$Latest.ManifestSha256
    latest_matches_candidate=$true; old_latest_existed=($null -ne $OldLatest)
    old_latest_files=if($OldLatest){$OldLatest.Files}else{$null}; old_latest_bytes=if($OldLatest){$OldLatest.Bytes}else{$null}
    old_latest_manifest_sha256=if($OldLatest){$OldLatest.ManifestSha256}else{$null}
    backup_root=if($OldLatestMoved){$BackupRoot}else{$null}; backup_verified=if($OldLatestMoved){$true}else{$null}
    robocopy_exit_code=$Copy.ExitCode; evidence_root=$AttemptRoot
}
[IO.File]::WriteAllText((Join-Path $AttemptRoot 'LatestSwitch.summary.json'),(($Summary|ConvertTo-Json -Depth 8)+"`n"),$Utf8NoBom)
$Summary | ConvertTo-Json -Depth 8
