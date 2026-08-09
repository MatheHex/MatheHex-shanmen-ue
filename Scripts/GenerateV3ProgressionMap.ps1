param()
$ErrorActionPreference = 'Stop'
$ProjectRoot = Split-Path -Parent $PSScriptRoot
$ProjectFile = Join-Path $ProjectRoot 'demo_map.uproject'
$PythonScript = Join-Path $PSScriptRoot 'GenerateV3ProgressionMap.py'
$EditorCmd = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$LogDirectory = Join-Path $ProjectRoot 'Saved\Logs\0.3.3.0'
$Log = Join-Path $LogDirectory 'v3_map_generation.log'
New-Item -ItemType Directory -Path $LogDirectory -Force | Out-Null
foreach ($Path in @($ProjectRoot, $ProjectFile, $PythonScript, $EditorCmd)) {
    if (-not (Test-Path -LiteralPath $Path)) { throw "Required path missing: $Path" }
}
& $EditorCmd $ProjectFile "-ExecutePythonScript=$PythonScript" -unattended -nop4 -nosplash -stdout -FullStdOutLogOutput "-abslog=$Log"
exit $LASTEXITCODE
