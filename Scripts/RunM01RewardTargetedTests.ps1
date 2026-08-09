param([string]$TaskId = 'Manual.M01RewardTargeted')
$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSScriptRoot 'Shanmen.Foundation.psm1') -Force
$Project = Resolve-ShanmenProject
$ProjectRoot = $Project.ProjectRoot
$ProjectFile = Join-Path $ProjectRoot 'demo_map.uproject'
$EditorCmd = $Project.EditorCmd
$EvidenceDirectory = Join-Path $ProjectRoot ('Saved\Automation\' + (ConvertTo-ShanmenSafeName $TaskId))
$Log = Join-Path $EvidenceDirectory 'M01RewardTargetedTests.log'
New-Item -ItemType Directory -Path $EvidenceDirectory -Force | Out-Null
foreach ($Path in @($ProjectRoot, $ProjectFile, $EditorCmd)) {
    if (-not (Test-Path -LiteralPath $Path)) { throw "Required path missing: $Path" }
}
& $EditorCmd $ProjectFile '/Game/M01/Maps/L_M01_Expedition' `
    -unattended -nop4 -nosplash -nullrhi -nosound `
    '-ExecCmds=Automation RunTests demo_map.M01.Reward;Quit' `
    '-TestExit=Automation Test Queue Empty' -stdout -FullStdOutLogOutput `
    "-abslog=$Log"
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
if (Select-String -LiteralPath $Log -Pattern 'Result=Failed|Automation Test Failed' -Quiet) { exit 1 }
$Passed = @(Select-String -LiteralPath $Log -Pattern 'Test Completed\. Result=\{Success\}.*Path=\{demo_map\.M01\.Reward\.' -AllMatches)
if ($Passed.Count -ne 12) { exit 1 }
if (-not (Select-String -LiteralPath $Log -Pattern '\*\*\*\* TEST COMPLETE\. EXIT CODE: 0 \*\*\*\*' -Quiet)) { exit 1 }
exit 0
