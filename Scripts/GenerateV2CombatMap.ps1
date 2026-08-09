param()
$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSScriptRoot 'Shanmen.Foundation.psm1') -Force
$Project = Resolve-ShanmenProject
$ProjectRoot = $Project.ProjectRoot
$ProjectFile = Join-Path $ProjectRoot 'demo_map.uproject'
$PythonScript = Join-Path $PSScriptRoot 'GenerateV2CombatMap.py'
$EditorCmd = $Project.EditorCmd
$Log = Join-Path $ProjectRoot 'Docs\0.2.0版本开发档案\0.2.5.0_MAP_GENERATION.log'
if (-not (Test-Path -LiteralPath $ProjectFile)) { throw "Project file missing: $ProjectFile" }
if (-not (Test-Path -LiteralPath $EditorCmd)) { throw "UnrealEditor-Cmd missing: $EditorCmd" }
& $EditorCmd $ProjectFile "-ExecutePythonScript=$PythonScript" -unattended -nop4 -nosplash -stdout -FullStdOutLogOutput "-abslog=$Log"
exit $LASTEXITCODE
