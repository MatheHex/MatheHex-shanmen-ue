[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [ValidateSet('CompleteCarry', 'Empty', 'NotEnrolled', 'BridgeFailure', 'PreparedRecovery', 'ActiveSessionConflict', 'OutOfRaidLock')]
    [string]$Scenario,
    [ValidateSet('Single', 'Initial', 'Recovery')]
    [string]$Phase = 'Single',
    [Parameter(Mandatory)]
    [string]$StorageRoot,
    [Parameter(Mandatory)]
    [string]$LogPath,
    [int]$TimeoutSeconds = 120,
    [switch]$NegativeControl
)

$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSScriptRoot 'Shanmen.Foundation.psm1') -Force
$Project = Resolve-ShanmenProject
$projectRoot = $Project.ProjectRoot
$engineEditor = $Project.EditorCmd
$uproject = Join-Path $projectRoot 'demo_map.uproject'
$canonicalRoot = [IO.Path]::GetFullPath($StorageRoot)
$allowedRoot = [IO.Path]::GetFullPath((Join-Path $projectRoot 'Saved\Automation\Dev.D.UE.0.0.9B.P6.0.r3'))

if (-not $canonicalRoot.StartsWith($allowedRoot, [StringComparison]::OrdinalIgnoreCase)) {
    throw "P6r3 wrapper rejected storage root outside the isolated P6ProductStartBridge boundary: $canonicalRoot"
}

New-Item -ItemType Directory -Force -Path $canonicalRoot | Out-Null
$outcomePath = Join-Path $canonicalRoot ("P6r3ProductOutcome.{0}.json" -f $Phase)

if ($NegativeControl) {
    $profileData = Get-ChildItem -LiteralPath $canonicalRoot -Recurse -File -ErrorAction SilentlyContinue |
        Where-Object { $_.Name -notlike 'P6r3ProductOutcome.*.json' }
    if ($profileData) {
        throw 'Negative control root already contains product data; refusing to mask a mutation.'
    }
    [ordered]@{
        task = 'Dev.D.UE.0.0.9B.P6.0.r3'
        scenario = 'NegativeControl'
        phase = $Phase
        outcome = 'FAIL'
        trace_sequence = 0
        detail = 'Explicit wrapper negative control; no Unreal process launched and no P5/P6 profile data written.'
    } | ConvertTo-Json -Compress | Set-Content -LiteralPath $outcomePath -Encoding utf8
    Write-Output "P6R3_WRAPPER scenario=NegativeControl phase=$Phase editor_exit=NA result=FAIL wrapper_exit=2 residual_child=0"
    exit 2
}

if (-not (Test-Path -LiteralPath $engineEditor) -or -not (Test-Path -LiteralPath $uproject)) {
    throw 'Required Unreal editor or project file is unavailable.'
}

$arguments = @(
    $uproject,
    '/Game/M01/Maps/L_M01_Expedition',
    '-game', '-nosplash', '-unattended', '-NoSound',
    '-P6ProductStartBridgeR3Trace',
    ("-P6ProductStartBridgeStorageRoot={0}" -f $canonicalRoot),
    ("-P6ProductStartBridgeCase={0}" -f $Scenario),
    ("-P6ProductStartBridgePhase={0}" -f $Phase),
    ("-abslog={0}" -f $LogPath)
)

$process = Start-Process -FilePath $engineEditor -ArgumentList $arguments -PassThru
$finished = $process.WaitForExit([Math]::Max(1, $TimeoutSeconds) * 1000)
if (-not $finished) {
    Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue
    Write-Output "P6R3_WRAPPER scenario=$Scenario phase=$Phase editor_exit=TIMEOUT result=MISSING wrapper_exit=3 residual_child=0"
    exit 3
}

$editorExit = $process.ExitCode
$residualChild = [bool](Get-Process -Id $process.Id -ErrorAction SilentlyContinue)
$result = $null
try {
    if (Test-Path -LiteralPath $outcomePath) {
        $result = Get-Content -LiteralPath $outcomePath -Raw | ConvertFrom-Json
    }
} catch {
    $result = $null
}

$validResult = $null -ne $result `
    -and $result.task -eq 'Dev.D.UE.0.0.9B.P6.0.r3' `
    -and $result.scenario -eq $Scenario `
    -and $result.phase -eq $Phase `
    -and $result.outcome -eq 'PASS' `
    -and $result.trace_sequence -ge 1

$wrapperExit = if ($editorExit -ne 0) { 4 } elseif ($residualChild) { 5 } elseif (-not $validResult) { 6 } else { 0 }
$resultOutcome = if ($null -eq $result) { 'MISSING' } else { [string]$result.outcome }
Write-Output "P6R3_WRAPPER scenario=$Scenario phase=$Phase editor_exit=$editorExit result=$resultOutcome wrapper_exit=$wrapperExit residual_child=$([int]$residualChild)"
exit $wrapperExit
