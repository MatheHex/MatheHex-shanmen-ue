param()
$ErrorActionPreference = 'Stop'
$ProjectRoot = Split-Path -Parent $PSScriptRoot
$ProjectFile = Join-Path $ProjectRoot 'demo_map.uproject'
$PythonScript = Join-Path $PSScriptRoot 'ValidateM01ExpeditionMap.py'
$EditorCmd = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$EvidenceDirectory = Join-Path $ProjectRoot 'Saved\Automation\Dev.D.UE.0.0.8.P5.0.r0'
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

