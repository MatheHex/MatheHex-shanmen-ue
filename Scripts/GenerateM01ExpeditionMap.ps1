param([string]$TaskId = 'Manual.M01ExpeditionMapGeneration')
$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSScriptRoot 'Shanmen.Foundation.psm1') -Force
$Project = Resolve-ShanmenProject
$ProjectRoot = $Project.ProjectRoot
$ProjectFile = Join-Path $ProjectRoot 'demo_map.uproject'
$PythonScript = Join-Path $PSScriptRoot 'GenerateM01ExpeditionMap.py'
$EditorCmd = $Project.EditorCmd
$EvidenceDirectory = Join-Path $ProjectRoot ('Saved\Automation\' + (ConvertTo-ShanmenSafeName $TaskId))
$Log = Join-Path $EvidenceDirectory 'M01MapGeneration.log'
New-Item -ItemType Directory -Path $EvidenceDirectory -Force | Out-Null
foreach ($Path in @($ProjectRoot, $ProjectFile, $PythonScript, $EditorCmd)) {
    if (-not (Test-Path -LiteralPath $Path)) { throw "Required path missing: $Path" }
}
& $EditorCmd $ProjectFile "-ExecutePythonScript=$PythonScript" -unattended -nop4 -nosplash -stdout -FullStdOutLogOutput "-abslog=$Log"
if (Select-String -LiteralPath $Log -Pattern 'M01_MAP_GENERATION: FAIL|Python script executed with errors' -Quiet) { exit 1 }
exit $LASTEXITCODE
