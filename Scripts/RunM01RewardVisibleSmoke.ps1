param([string]$TaskId = 'Manual.M01RewardVisibleSmoke')
$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSScriptRoot 'Shanmen.Foundation.psm1') -Force
$Project = Resolve-ShanmenProject
$ProjectRoot = $Project.ProjectRoot
$ProjectFile = Join-Path $ProjectRoot 'demo_map.uproject'
$Editor = $Project.Editor
$EvidenceDirectory = Join-Path $ProjectRoot ('Saved\Automation\' + (ConvertTo-ShanmenSafeName $TaskId))
$VisualDirectory = Join-Path $EvidenceDirectory 'Visual'
$Log = Join-Path $EvidenceDirectory 'M01RewardVisibleSmoke.log'
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
$Process = Start-Process -FilePath $Editor -ArgumentList $Arguments -Wait -PassThru -WindowStyle Hidden
if ($Process.ExitCode -ne 0) { exit $Process.ExitCode }
if (-not (Select-String -LiteralPath $Log -Pattern 'M01_ENEMY_VISIBLE_SMOKE: PASS sources=149 generated_containers=135 search_same_guid_take=PASS' -Quiet)) { exit 1 }
$Screenshots = @(Get-ChildItem -LiteralPath $VisualDirectory -Filter '*.png' -File)
if ($Screenshots.Count -lt 3) { exit 1 }
exit 0
