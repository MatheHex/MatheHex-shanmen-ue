[CmdletBinding()]
param(
    [ValidateSet('Play','Editor','GenerateMap','GenerateMaterial','GenerateExpedition')][string]$Action='Play',
    [switch]$Practice,
    [ValidateRange(960,3840)][int]$Width=1280,
    [ValidateRange(540,2160)][int]$Height=720,
    [ValidatePattern('^[A-Za-z0-9_-]{1,64}$')][string]$ProfileName='ExpeditionProfile',
    [ValidateRange(1,1000000000)][int]$TestMoney=1000000
)
$ErrorActionPreference='Stop'
Import-Module (Join-Path $PSScriptRoot 'Shanmen.Foundation.psm1') -Force
$project=Resolve-ShanmenProject
$map=if($Practice -or $Action -eq 'GenerateMap'){'/Game/Demo20/Maps/L_Demo20_StoneCourt'}else{'/Game/Demo20/Maps/L_Demo20_JadePass'}
$mapFile=Join-Path $project.ProjectRoot ($map.Replace('/Game/','Content/')+'.umap')
if (@(Get-Process -Name UnrealEditor,UnrealEditor-Cmd -ErrorAction SilentlyContinue).Count) {
    throw 'An Unreal Editor process is already running. Close your own Editor before launching another instance.'
}
$evidence=New-ShanmenEvidenceContext -Project $project -Action $Action -TaskId 'Demo20.S01'
$isolated=Join-Path $project.ProjectRoot 'Saved/Demo20/User'
New-Item -ItemType Directory -Path $isolated -Force | Out-Null
if ($Action -in @('GenerateMap','GenerateMaterial','GenerateExpedition')) {
    $scriptName=if($Action -eq 'GenerateMap'){'GenerateDemo20Map.py'}elseif($Action -eq 'GenerateExpedition'){'GenerateDemo20Expedition.py'}else{'GenerateDemo20Material.py'}
    $outputFile=if($Action -ne 'GenerateMaterial'){$mapFile}else{Join-Path $project.ProjectRoot 'Content/Demo20/Materials/M_Demo20_Color.uasset'}
    if(Test-Path -LiteralPath $outputFile){throw 'Demo20 asset already exists; generation will not overwrite it.'}
    $arguments=@($project.Uproject,"-ExecutePythonScript=$(Join-Path $PSScriptRoot $scriptName)",'-unattended','-nop4','-nosplash','-NullRHI',"-UserDir=$isolated","-abslog=$(Join-Path $evidence.Root 'UnrealEditor.log')")
    $result=Start-ShanmenTrackedProcess -FilePath $project.EditorCmd -ArgumentList $arguments -WorkingDirectory $project.ProjectRoot -Evidence $evidence -Wait -Hidden
    if($result.State.exit_code -ne 0 -or !(Test-Path -LiteralPath $outputFile)){throw 'Asset generation failed; see evidence log.'}
} else {
    if(!(Test-Path -LiteralPath $mapFile)){throw 'Generate the formal map with -Action GenerateExpedition, or use -Practice for the existing StoneCourt.'}
    $arguments=@($project.Uproject,$map,'-nosplash',"-UserDir=$isolated","-Demo20ProfileName=$ProfileName","-Demo20TestMoney=$TestMoney","-abslog=$(Join-Path $evidence.Root 'UnrealEditor.log')")
    if($Action -eq 'Play'){$arguments+=@('-game','-windowed',"-ResX=$Width","-ResY=$Height",'-ForceRes')}
    $result=Start-ShanmenTrackedProcess -FilePath $project.Editor -ArgumentList $arguments -WorkingDirectory $project.ProjectRoot -Evidence $evidence
    "Demo20 PID: $($result.Process.Id)"
}
"Evidence: $($evidence.Root)"
