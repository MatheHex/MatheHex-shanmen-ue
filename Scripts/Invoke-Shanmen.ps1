[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [ValidateSet(
        'Audit',
        'BuildEditor',
        'BuildGame',
        'BuildBoth',
        'OpenEditor',
        'ResolveLatest',
        'LaunchLatest',
        'Package')]
    [string]$Action,

    [string]$TaskId = 'Manual.Foundation',
    [string]$EngineRoot,
    [ValidateSet('DebugGame', 'Development', 'Shipping', 'Test')]
    [string]$Configuration = 'Development',
    [int]$MaxParallelActions = 1,
    [switch]$UseUba,
    [switch]$WaitForExit,
    [switch]$PackagedOnly,
    [string]$StageId = 'Manual.I',
    [string]$ArchiveRoot
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$modulePath = Join-Path $PSScriptRoot 'Shanmen.Foundation.psm1'
Import-Module $modulePath -Force
$project = Resolve-ShanmenProject -EngineRoot $EngineRoot

function Invoke-OneBuild {
    param([ValidateSet('Editor', 'Game')][string]$Target)
    $evidence = New-ShanmenEvidenceContext -Project $project -Action "Build$Target" -TaskId $TaskId
    $buildParameters = @{
        Project = $project
        Target = $Target
        Configuration = $Configuration
        Evidence = $evidence
        MaxParallelActions = $MaxParallelActions
        UseUba = $UseUba
    }
    $result = Invoke-ShanmenBuild @buildParameters
    Write-Host "[$Target] evidence: $($evidence.Root)"
    if ($result.State.exit_code -ne 0) {
        throw "$Target build failed with exit code $($result.State.exit_code)."
    }
}

switch ($Action) {
    'Audit' {
        $auditScript = Join-Path $PSScriptRoot 'Invoke-FoundationAudit.ps1'
        & $auditScript -StageId $StageId -EngineRoot $project.EngineRoot
        exit $LASTEXITCODE
    }

    'BuildEditor' {
        Invoke-OneBuild -Target 'Editor'
    }

    'BuildGame' {
        Invoke-OneBuild -Target 'Game'
    }

    'BuildBoth' {
        Invoke-OneBuild -Target 'Editor'
        Invoke-OneBuild -Target 'Game'
    }

    'OpenEditor' {
        $evidence = New-ShanmenEvidenceContext -Project $project -Action $Action -TaskId $TaskId
        $arguments = @($project.Uproject, "-abslog=$(Join-Path $evidence.Root 'UnrealEditor.log')")
        $startParameters = @{
            FilePath = $project.Editor
            ArgumentList = $arguments
            WorkingDirectory = $project.ProjectRoot
            Evidence = $evidence
            Wait = $WaitForExit
        }
        $result = Start-ShanmenTrackedProcess @startParameters
        Write-Host "Editor PID: $($result.Process.Id)"
        Write-Host "Evidence: $($evidence.Root)"
    }

    'ResolveLatest' {
        $plan = Get-ShanmenLatestLaunchPlan -Project $project -PackagedOnly:$PackagedOnly
        $plan | ConvertTo-Json -Depth 4
    }

    'LaunchLatest' {
        $evidence = New-ShanmenEvidenceContext -Project $project -Action $Action -TaskId $TaskId
        $plan = Get-ShanmenLatestLaunchPlan `
            -Project $project `
            -PackagedOnly:$PackagedOnly `
            -LogPath (Join-Path $evidence.Root 'LatestDemo.EditorGame.log')
        $startParameters = @{
            FilePath = $plan.FilePath
            ArgumentList = @($plan.Arguments)
            WorkingDirectory = $plan.WorkingDirectory
            Evidence = $evidence
            Wait = $WaitForExit
        }
        $result = Start-ShanmenTrackedProcess @startParameters
        if ($plan.Mode -eq 'EDITOR_GAME_FALLBACK') {
            Write-Warning 'Latest_Demo package is absent; launched the canonical Editor -game fallback.'
        }
        Write-Host "Latest mode: $($plan.Mode)"
        Write-Host "Latest PID: $($result.Process.Id)"
        Write-Host "Evidence: $($evidence.Root)"
    }

    'Package' {
        if ([string]::IsNullOrWhiteSpace($ArchiveRoot)) {
            $workspaceRoot = Split-Path -Parent $project.ProjectRoot
            $safeTask = ConvertTo-ShanmenSafeName $TaskId
            $stamp = (Get-Date).ToUniversalTime().ToString('yyyyMMddTHHmmssZ')
            $ArchiveRoot = Join-Path $workspaceRoot "Builds\demo_map\Staging\$safeTask-$stamp"
        }
        $ArchiveRoot = [IO.Path]::GetFullPath($ArchiveRoot)
        $workspaceBuilds = Join-Path (Split-Path -Parent $project.ProjectRoot) 'Builds'
        if (-not (Test-ShanmenPathWithin -Candidate $ArchiveRoot -Parent $workspaceBuilds)) {
            throw "Package archive must stay under $workspaceBuilds"
        }
        if (Test-Path -LiteralPath $ArchiveRoot) {
            throw "Package archive already exists: $ArchiveRoot"
        }
        New-Item -ItemType Directory -Path $ArchiveRoot -Force | Out-Null
        $evidence = New-ShanmenEvidenceContext -Project $project -Action $Action -TaskId $TaskId
        $uatArguments = @(
            'BuildCookRun',
            "-project=$($project.Uproject)",
            '-noP4',
            '-utf8output',
            '-platform=Win64',
            '-clientconfig=Development',
            '-build',
            '-cook',
            '-stage',
            '-package',
            '-pak',
            '-iostore',
            '-archive',
            "-archivedirectory=$ArchiveRoot",
            '-map=/Game/M01/Maps/L_M01_Expedition',
            '-NoZenStore',
            '-utf8output')
        $commandTokens = @('call', (ConvertTo-WindowsCommandLineArgument $project.RunUat)) +
            @($uatArguments | ForEach-Object { ConvertTo-WindowsCommandLineArgument $_ })
        $command = $commandTokens -join ' '
        $startParameters = @{
            FilePath = $env:ComSpec
            ArgumentList = @('/d', '/s', '/c', $command)
            WorkingDirectory = $project.ProjectRoot
            Evidence = $evidence
            Wait = $true
            Hidden = $true
            CaptureOutput = $true
        }
        $result = Start-ShanmenTrackedProcess @startParameters
        if ($result.State.exit_code -ne 0) {
            throw "Package failed with exit code $($result.State.exit_code)."
        }
        Write-Host "Package: $ArchiveRoot"
        Write-Host "Evidence: $($evidence.Root)"
    }
}
