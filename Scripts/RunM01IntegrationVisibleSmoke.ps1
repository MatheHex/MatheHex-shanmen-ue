param()
$ErrorActionPreference = 'Stop'
$ProjectRoot = Split-Path -Parent $PSScriptRoot
$ProjectFile = Join-Path $ProjectRoot 'demo_map.uproject'
$Editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$EvidenceDirectory = Join-Path $ProjectRoot 'Saved\Automation\Dev.D.UE.0.0.8.P5.0.r0'
$VisualDirectory = Join-Path $EvidenceDirectory 'Visual_Final7'
$Log = Join-Path $EvidenceDirectory 'M01IntegrationVisibleSmoke.log'
New-Item -ItemType Directory -Path $VisualDirectory -Force | Out-Null
foreach ($Path in @($ProjectRoot, $ProjectFile, $Editor)) {
    if (-not (Test-Path -LiteralPath $Path)) { throw "Required path missing: $Path" }
}
$Arguments = @(
    $ProjectFile,
    '/Game/M01/Maps/L_M01_Expedition',
    '-game', '-windowed', '-ResX=1280', '-ResY=720',
    '-M01IntegrationVisibleSmoke', "-M01VisualOutput=$VisualDirectory",
    '-nop4', '-nosplash', '-stdout', '-FullStdOutLogOutput', "-abslog=$Log"
)
$Process = Start-Process -FilePath $Editor -ArgumentList $Arguments -Wait -PassThru
if ($Process.ExitCode -ne 0) { exit $Process.ExitCode }
if (-not (Select-String -LiteralPath $Log -Pattern 'M01_INTEGRATION_VISIBLE_SMOKE: art=PASS theme=DESOLATE_SPIRIT_MINE regions=LOW/MID/HIGH/BOSS hud=PASS' -Quiet)) { exit 1 }
if (-not (Select-String -LiteralPath $Log -Pattern 'M01_INTEGRATION_VISIBLE_SMOKE: PASS theme=1 low_regular=1 high_boss=1 discard_spatial=1 settlement_once=1 new_run_reset=1 duplicates=0 input_lock=0 sources=149 base_value=115500' -Quiet)) { exit 1 }
$Screenshots = @(Get-ChildItem -LiteralPath $VisualDirectory -Filter '*.png' -File)
if ($Screenshots.Count -lt 7) { exit 1 }
exit 0
