param(
    [string]$AttemptId = 'attempt-001',
    [string]$TaskId = 'Dev.D.UE.0.0.9B.F0.0.r0'
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
Import-Module (Join-Path $PSScriptRoot 'Shanmen.Foundation.psm1') -Force
$Project = Resolve-ShanmenProject
$ProjectRoot = $Project.ProjectRoot
$TaskRoot = Join-Path $ProjectRoot "Saved\Automation\$TaskId"
$AttemptRoot = Join-Path $TaskRoot "PackageBuild\$AttemptId"
$PackageRoot = Join-Path $AttemptRoot 'Package'
$StdoutPath = Join-Path $AttemptRoot 'BuildCookRun.stdout.log'
$StderrPath = Join-Path $AttemptRoot 'BuildCookRun.stderr.log'
$ResultPath = Join-Path $AttemptRoot 'result.json'
$ManifestPath = Join-Path $AttemptRoot 'package.manifest.txt'
$ManifestTool = Join-Path $PSScriptRoot 'GetF0CanonicalManifest.ps1'
$Uat = $Project.RunUat
$Uproject = Join-Path $ProjectRoot 'demo_map.uproject'
$Utf8NoBom = [System.Text.UTF8Encoding]::new($false)

if (Test-Path -LiteralPath $AttemptRoot) { throw "Attempt already exists: $AttemptRoot" }
foreach ($Required in @($Uat, $Uproject, $ManifestTool)) {
    if (-not (Test-Path -LiteralPath $Required -PathType Leaf)) { throw "Missing required file: $Required" }
}
New-Item -ItemType Directory -Path $AttemptRoot -Force | Out-Null

$Arguments = @(
    'BuildCookRun', "-project=$Uproject", '-noP4', '-utf8output',
    '-platform=Win64', '-clientconfig=Development', '-build', '-cook',
    '-stage', '-package', '-pak', '-iostore', '-archive',
    "-archivedirectory=$PackageRoot",
    '-map=/Game/M01/Maps/L_M01_Expedition',
    '-AdditionalCookerOptions=-SkipZenStore',
    '-ubtargs="-NoUBA -MaxParallelActions=2"')

$Started = [DateTimeOffset]::UtcNow
$Process = Start-Process -FilePath $Uat -ArgumentList $Arguments -WindowStyle Hidden `
    -RedirectStandardOutput $StdoutPath -RedirectStandardError $StderrPath -PassThru -Wait
$Ended = [DateTimeOffset]::UtcNow
$Text = @(
    if (Test-Path -LiteralPath $StdoutPath) { Get-Content -Raw -LiteralPath $StdoutPath }
    if (Test-Path -LiteralPath $StderrPath) { Get-Content -Raw -LiteralPath $StderrPath }
) -join "`n"
$PrimaryExecutable = Join-Path $PackageRoot 'Windows\demo_map.exe'
$GameExecutable = Join-Path $PackageRoot 'Windows\demo_map\Binaries\Win64\demo_map.exe'
$ContainerFiles = @(Get-ChildItem -LiteralPath (Join-Path $PackageRoot 'Windows') -Recurse -File `
    -ErrorAction SilentlyContinue | Where-Object Extension -in @('.pak', '.utoc', '.ucas'))
$FatalCount = [regex]::Matches($Text, '(?im)^\s*Fatal error:|Unhandled Exception').Count
$FailureCount = [regex]::Matches($Text, '(?im)AutomationTool exiting with ExitCode=[1-9]|BUILD FAILED|Cook failed|Stage Failed|Pak failed|Archive failed').Count
$Passed = $Process.ExitCode -eq 0 -and $FatalCount -eq 0 -and $FailureCount -eq 0 -and
    (Test-Path -LiteralPath $PrimaryExecutable -PathType Leaf) -and
    (Test-Path -LiteralPath $GameExecutable -PathType Leaf) -and $ContainerFiles.Count -ge 3

$Summary = $null
if ($Passed) { $Summary = & $ManifestTool -Path $PackageRoot -OutputPath $ManifestPath }
$Result = [ordered]@{
    task_id = $TaskId; attempt_id = $AttemptId
    started_utc = $Started.ToString('o'); ended_utc = $Ended.ToString('o')
    duration_seconds = [math]::Round(($Ended - $Started).TotalSeconds, 3)
    exit_code = $Process.ExitCode; fatal_count = $FatalCount; failure_count = $FailureCount
    build = $Passed; cook = $Passed; stage = $Passed; package = $Passed
    pak = $Passed; iostore = $Passed; archive = $Passed; passed = $Passed
    mode = 'WINDOWS_DEVELOPMENT_SKIP_ZEN_STORE'
    package_root = $PackageRoot; primary_executable = $PrimaryExecutable
    game_executable = $GameExecutable; container_file_count = $ContainerFiles.Count
    package_files = if ($Summary) { $Summary.Files } else { 0 }
    package_bytes = if ($Summary) { $Summary.Bytes } else { 0 }
    package_manifest_sha256 = if ($Summary) { $Summary.ManifestSha256 } else { '' }
    stdout_path = $StdoutPath; stderr_path = $StderrPath
}
[System.IO.File]::WriteAllText($ResultPath, (($Result | ConvertTo-Json -Depth 8) + "`n"), $Utf8NoBom)
$Result | ConvertTo-Json -Depth 8
if (-not $Passed) { exit 1 }
