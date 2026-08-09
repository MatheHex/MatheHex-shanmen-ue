param([string]$TaskId = 'Manual.M01InitialArtValidation')
$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSScriptRoot 'Shanmen.Foundation.psm1') -Force
$Project = Resolve-ShanmenProject
$ProjectRoot = $Project.ProjectRoot
$ProjectFile = Join-Path $ProjectRoot 'demo_map.uproject'
$PythonScript = Join-Path $PSScriptRoot 'ValidateM01ExpeditionMap.py'
$EditorCmd = $Project.EditorCmd
$EvidenceDirectory = Join-Path $ProjectRoot ('Saved\Automation\' + (ConvertTo-ShanmenSafeName $TaskId))
$Log = Join-Path $EvidenceDirectory 'M01MapCheck.log'
New-Item -ItemType Directory -Path $EvidenceDirectory -Force | Out-Null
foreach ($Path in @($ProjectRoot, $ProjectFile, $PythonScript, $EditorCmd)) {
    if (-not (Test-Path -LiteralPath $Path)) { throw "Required path missing: $Path" }
}
& $EditorCmd $ProjectFile "-ExecutePythonScript=$PythonScript" -unattended -nop4 -nosplash -stdout -FullStdOutLogOutput "-abslog=$Log"
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
if (Select-String -LiteralPath $Log -Pattern 'M01_MAP_VALIDATION: FAIL|Python script executed with errors|MapCheck: Error:' -Quiet) { exit 1 }
if (-not (Select-String -LiteralPath $Log -Pattern 'M01_MAP_VALIDATION: THEME=DESOLATE_SPIRIT_MINE ART_PROPS=40 VISUAL_COLLISION=SEPARATED' -Quiet)) { exit 1 }
if (-not (Select-String -LiteralPath $Log -Pattern 'M01_MAP_VALIDATION: PASS' -Quiet)) { exit 1 }
exit 0
