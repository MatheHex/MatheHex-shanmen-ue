param()
$ErrorActionPreference = 'Stop'
$ProjectRoot = Split-Path -Parent $PSScriptRoot
$ProjectFile = Join-Path $ProjectRoot 'demo_map.uproject'
$PythonScript = Join-Path $PSScriptRoot 'GenerateV2CombatMap.py'
$EditorCmd = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$Log = Join-Path $ProjectRoot 'Docs\0.2.0版本开发档案\0.2.5.0_MAP_GENERATION.log'
if (-not (Test-Path -LiteralPath $ProjectFile)) { throw "Project file missing: $ProjectFile" }
if (-not (Test-Path -LiteralPath $EditorCmd)) { throw "UnrealEditor-Cmd missing: $EditorCmd" }
& $EditorCmd $ProjectFile "-ExecutePythonScript=$PythonScript" -unattended -nop4 -nosplash -stdout -FullStdOutLogOutput "-abslog=$Log"
exit $LASTEXITCODE
