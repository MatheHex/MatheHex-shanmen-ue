param()
$ErrorActionPreference = 'Stop'
$ProjectRoot = Split-Path -Parent $PSScriptRoot
$ProjectFile = Join-Path $ProjectRoot 'demo_map.uproject'
$PythonScript = Join-Path $PSScriptRoot 'GenerateM01ExpeditionMap.py'
$EditorCmd = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$EvidenceDirectory = Join-Path $ProjectRoot 'Saved\Automation\Dev.D.UE.0.0.8.P2.0.r0'
$Log = Join-Path $EvidenceDirectory 'M01MapGeneration.log'
New-Item -ItemType Directory -Path $EvidenceDirectory -Force | Out-Null
foreach ($Path in @($ProjectRoot, $ProjectFile, $PythonScript, $EditorCmd)) {
    if (-not (Test-Path -LiteralPath $Path)) { throw "Required path missing: $Path" }
}
& $EditorCmd $ProjectFile "-ExecutePythonScript=$PythonScript" -unattended -nop4 -nosplash -stdout -FullStdOutLogOutput "-abslog=$Log"
if (Select-String -LiteralPath $Log -Pattern 'M01_MAP_GENERATION: FAIL|Python script executed with errors' -Quiet) { exit 1 }
exit $LASTEXITCODE
