[CmdletBinding()]
param(
    [ValidateSet('Play','Editor','GenerateMap','GenerateMaterial')][string]$Action='Play',
    [ValidateRange(960,3840)][int]$Width=1280,
    [ValidateRange(540,2160)][int]$Height=720
)
$ErrorActionPreference='Stop'
Import-Module (Join-Path $PSScriptRoot 'Shanmen.Foundation.psm1') -Force
$project=Resolve-ShanmenProject
$map='/Game/Demo20/Maps/L_Demo20_StoneCourt'
$mapFile=Join-Path $project.ProjectRoot 'Content/Demo20/Maps/L_Demo20_StoneCourt.umap'
if (@(Get-Process -Name UnrealEditor,UnrealEditor-Cmd -ErrorAction SilentlyContinue).Count) {
    throw 'An Unreal Editor process is already running. Close your own Editor before launching another instance.'
}
$evidence=New-ShanmenEvidenceContext -Project $project -Action $Action -TaskId 'Demo20.S01'
$isolated=Join-Path $project.ProjectRoot 'Saved/Demo20/User'
New-Item -ItemType Directory -Path $isolated -Force | Out-Null
if ($Action -in @('GenerateMap','GenerateMaterial')) {
    $scriptName=if($Action -eq 'GenerateMap'){'GenerateDemo20Map.py'}else{'GenerateDemo20Material.py'}
    $outputFile=if($Action -eq 'GenerateMap'){$mapFile}else{Join-Path $project.ProjectRoot 'Content/Demo20/Materials/M_Demo20_Color.uasset'}
    if(Test-Path -LiteralPath $outputFile){throw 'Demo20 asset already exists; generation will not overwrite it.'}
    $arguments=@($project.Uproject,"-ExecutePythonScript=$(Join-Path $PSScriptRoot $scriptName)",'-unattended','-nop4','-nosplash','-NullRHI',"-UserDir=$isolated","-abslog=$(Join-Path $evidence.Root 'UnrealEditor.log')")
    $result=Start-ShanmenTrackedProcess -FilePath $project.EditorCmd -ArgumentList $arguments -WorkingDirectory $project.ProjectRoot -Evidence $evidence -Wait -Hidden
    if($result.State.exit_code -ne 0 -or !(Test-Path -LiteralPath $outputFile)){throw 'Asset generation failed; see evidence log.'}
} else {
    if(!(Test-Path -LiteralPath $mapFile)){throw 'Generate the new Demo20 map first with -Action GenerateMap.'}
    $arguments=@($project.Uproject,$map,'-nosplash',"-UserDir=$isolated","-abslog=$(Join-Path $evidence.Root 'UnrealEditor.log')")
    if($Action -eq 'Play'){$arguments+=@('-game','-windowed',"-ResX=$Width","-ResY=$Height",'-ForceRes')}
    $result=Start-ShanmenTrackedProcess -FilePath $project.Editor -ArgumentList $arguments -WorkingDirectory $project.ProjectRoot -Evidence $evidence
    "Demo20 PID: $($result.Process.Id)"
}
"Evidence: $($evidence.Root)"
