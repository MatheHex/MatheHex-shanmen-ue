param()
$ErrorActionPreference = 'Stop'
$ProjectRoot = Split-Path -Parent $PSScriptRoot
$ProjectFile = Join-Path $ProjectRoot 'demo_map.uproject'
$Editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$EvidenceDirectory = Join-Path $ProjectRoot 'Saved\Automation\Dev.D.UE.0.0.8.P3.0.r0'
$VisualDirectory = Join-Path $EvidenceDirectory 'Visual'
$Log = Join-Path $EvidenceDirectory 'M01EnemyVisibleSmoke.log'
New-Item -ItemType Directory -Path $VisualDirectory -Force | Out-Null
foreach ($Path in @($ProjectRoot, $ProjectFile, $Editor)) {
    if (-not (Test-Path -LiteralPath $Path)) { throw "Required path missing: $Path" }
}
$Arguments = @(
    $ProjectFile,
    '/Game/M01/Maps/L_M01_Expedition',
    '-game', '-windowed', '-ResX=1280', '-ResY=720',
    '-M01EnemyVisibleSmoke', "-M01VisualOutput=$VisualDirectory",
    '-nop4', '-nosplash', '-stdout', '-FullStdOutLogOutput', "-abslog=$Log"
)
$Process = Start-Process -FilePath $Editor -ArgumentList $Arguments -Wait -PassThru
if ($Process.ExitCode -ne 0) { exit $Process.ExitCode }
if (-not (Select-String -LiteralPath $Log -Pattern 'M01_ENEMY_VISIBLE_SMOKE: PASS' -Quiet)) { exit 1 }
$Screenshots = @(Get-ChildItem -LiteralPath $VisualDirectory -Filter '*.png' -File)
if ($Screenshots.Count -lt 3) { exit 1 }
exit 0

