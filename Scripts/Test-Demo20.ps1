[CmdletBinding()]
param()
$ErrorActionPreference='Stop'
Import-Module (Join-Path $PSScriptRoot 'Shanmen.Foundation.psm1') -Force
$project=Resolve-ShanmenProject
if (@(Get-Process -Name UnrealEditor,UnrealEditor-Cmd -ErrorAction SilentlyContinue).Count) { throw 'Close existing UE before automation.' }
$userRoot=Join-Path $project.ProjectRoot 'Saved/Demo20/AutomationUser'
New-Item -ItemType Directory -Path $userRoot -Force | Out-Null
# Keep one native group per log, matching the repository coverage gate.
$groups=@('Shanmen.Demo20','Shanmen.0_0_10.CombatCore','Shanmen.0_0_10.CombatRuntime.BasicSword','Shanmen.0_0_10.CombatRuntime.VitalityAuthority','Shanmen.0_0_10.CombatRuntime.VitalityLedger')
$logs=@()
foreach($group in $groups) {
$evidence=New-ShanmenEvidenceContext -Project $project -Action "Automation-$group" -TaskId 'Demo20.S01'
$log=Join-Path $evidence.Root 'UnrealEditor.log'
$arguments=@($project.Uproject,'/Engine/Maps/Entry','-unattended','-nop4','-nosplash','-NullRHI','-NoSound','-FORCELOGFLUSH',"-UserDir=$userRoot","-ExecCmds=Automation RunTests $group",'-TestExit=Automation Test Queue Empty',"-abslog=$log")
$result=Start-ShanmenTrackedProcess -FilePath $project.EditorCmd -ArgumentList $arguments -WorkingDirectory $project.ProjectRoot -Evidence $evidence -Wait -Hidden
$body=Get-Content -LiteralPath $log -Raw
if($result.State.exit_code -ne 0 -or $body -match 'Result=\{Fail\}' -or $body -notmatch 'Result=\{Success\}' -or $body -notmatch '(Automation Test Queue Empty\s+\d+\s+tests performed|TEST COMPLETE\. EXIT CODE: 0)') { throw "Demo20 automation not proven successful: $log" }
"Demo20 automation completed: $log"
Get-FileHash -LiteralPath $log -Algorithm SHA256 | Format-List
$logs+=$log
}
$paths=@(Get-ChildItem -LiteralPath (Join-Path $project.ProjectRoot 'Source/demo_map/Demo20') -File | ForEach-Object { 'Source/demo_map/Demo20/'+$_.Name })+@('Content/Demo20/Maps/L_Demo20_StoneCourt.umap','Content/Demo20/Materials/M_Demo20_Color.uasset')
& (Join-Path $PSScriptRoot 'Test-ShanmenRegressionCoverage.ps1') -ChangedPath $paths -AutomationLogPath $logs
