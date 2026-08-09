param([string]$TaskId = 'Manual.M01MapVisibleSmoke')
$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSScriptRoot 'Shanmen.Foundation.psm1') -Force
$Project = Resolve-ShanmenProject
$ProjectRoot = $Project.ProjectRoot
$ProjectFile = Join-Path $ProjectRoot 'demo_map.uproject'
$Editor = $Project.Editor
$EvidenceDirectory = Join-Path $ProjectRoot ('Saved\Automation\' + (ConvertTo-ShanmenSafeName $TaskId))
$VisualDirectory = Join-Path $EvidenceDirectory 'Visual'
$Log = Join-Path $EvidenceDirectory 'M01VisibleSmoke.log'
New-Item -ItemType Directory -Path $VisualDirectory -Force | Out-Null
foreach ($Path in @($ProjectRoot, $ProjectFile, $Editor)) {
    if (-not (Test-Path -LiteralPath $Path)) { throw "Required path missing: $Path" }
}
$Arguments = @(
    $ProjectFile,
    '/Game/M01/Maps/L_M01_Expedition',
    '-game', '-windowed', '-ResX=1280', '-ResY=720',
    '-M01ExtractionVisibleSmoke', "-M01VisualOutput=$VisualDirectory",
    '-nop4', '-nosplash', '-stdout', '-FullStdOutLogOutput', "-abslog=$Log"
)
$Process = Start-Process -FilePath $Editor -ArgumentList $Arguments -Wait -PassThru
if ($Process.ExitCode -ne 0) { exit $Process.ExitCode }
if (-not (Select-String -LiteralPath $Log -Pattern 'M01_EXTRACTION_VISIBLE_SMOKE: PASS regular=1 boss_locked=1 discard_condition_reused=1 return_ready=1 new_run_ready=1 repeated_p1_flows=0' -Quiet)) { exit 1 }
if (-not (Select-String -LiteralPath $Log -Pattern 'M01_MAP_VISIBLE_SMOKE: map=PASS risk_hud=LOW/MID/HIGH collision=PASS content_isolation=PASS' -Quiet)) { exit 1 }
$Screenshots = @(Get-ChildItem -LiteralPath $VisualDirectory -Filter '*.png' -File)
if ($Screenshots.Count -lt 2) { exit 1 }
exit 0
