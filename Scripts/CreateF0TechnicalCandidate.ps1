param(
    [Parameter(Mandatory = $true)][string]$CandidateName,
    [string]$PackageAttemptId = 'attempt-001',
    [string]$TaskId = 'Dev.D.UE.0.0.9B.F0.0.r0'
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$ProjectRoot = Split-Path -Parent $PSScriptRoot
$TaskRoot = Join-Path $ProjectRoot "Saved\Automation\$TaskId"
$CleanRoot = Join-Path $TaskRoot "PackageBuild\$PackageAttemptId\Package"
$CandidateBase = 'C:\AIDev\shanmen-ue\Builds\demo_map\Candidate'
$CandidateRoot = Join-Path $CandidateBase $CandidateName
$EvidenceRoot = Join-Path $TaskRoot "Candidate\$CandidateName"
$ManifestTool = Join-Path $PSScriptRoot 'GetF0CanonicalManifest.ps1'
$Utf8NoBom = [System.Text.UTF8Encoding]::new($false)

if (-not (Test-Path -LiteralPath $CleanRoot -PathType Container)) { throw "Clean Package missing: $CleanRoot" }
if (Test-Path -LiteralPath $CandidateRoot) { throw "Candidate already exists: $CandidateRoot" }
if (Test-Path -LiteralPath $EvidenceRoot) { throw "Candidate evidence already exists: $EvidenceRoot" }
$ResolvedBase = [IO.Path]::GetFullPath($CandidateBase).TrimEnd('\') + '\'
$ResolvedTarget = [IO.Path]::GetFullPath($CandidateRoot).TrimEnd('\') + '\'
if (-not $ResolvedTarget.StartsWith($ResolvedBase, [StringComparison]::OrdinalIgnoreCase)) {
    throw "Candidate target escaped base: $CandidateRoot"
}
New-Item -ItemType Directory -Path $CandidateBase,$EvidenceRoot -Force | Out-Null
$CleanManifestPath = Join-Path $EvidenceRoot 'CleanPackage.manifest.txt'
$CandidateManifestPath = Join-Path $EvidenceRoot 'Candidate.manifest.txt'
$CopyLog = Join-Path $EvidenceRoot 'CandidateCopy.log'
$CleanSummary = & $ManifestTool -Path $CleanRoot -OutputPath $CleanManifestPath
$Copy = Start-Process -FilePath 'robocopy.exe' -ArgumentList @(
    $CleanRoot, $CandidateRoot, '/E', '/COPY:DAT', '/DCOPY:DAT', '/R:1', '/W:1',
    '/NFL', '/NDL', '/NP', "/LOG:$CopyLog") -WindowStyle Hidden -PassThru -Wait
if ($Copy.ExitCode -gt 7) { throw "Candidate copy failed: robocopy $($Copy.ExitCode)" }
$CandidateSummary = & $ManifestTool -Path $CandidateRoot -OutputPath $CandidateManifestPath
$Identical = $CandidateSummary.Files -eq $CleanSummary.Files -and
    $CandidateSummary.Bytes -eq $CleanSummary.Bytes -and
    $CandidateSummary.ManifestSha256 -eq $CleanSummary.ManifestSha256
if (-not $Identical) { throw 'Candidate differs from clean Package.' }
$Summary = [ordered]@{
    task_id = $TaskId; package_attempt_id = $PackageAttemptId
    source_package = $CleanRoot; candidate_name = $CandidateName; candidate_root = $CandidateRoot
    candidate_never_run = $true; files = $CandidateSummary.Files; bytes = $CandidateSummary.Bytes
    manifest_sha256 = $CandidateSummary.ManifestSha256
    clean_manifest_sha256 = $CleanSummary.ManifestSha256
    identical_to_clean_package = $Identical; robocopy_exit_code = $Copy.ExitCode
    created_utc = [DateTimeOffset]::UtcNow.ToString('o')
}
[System.IO.File]::WriteAllText((Join-Path $EvidenceRoot 'Candidate.summary.json'),
    (($Summary | ConvertTo-Json -Depth 8) + "`n"), $Utf8NoBom)
$Summary | ConvertTo-Json -Depth 8
