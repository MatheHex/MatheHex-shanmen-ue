Set-StrictMode -Version Latest

function ConvertTo-ShanmenSafeName {
    param([Parameter(Mandatory)][string]$Value)
    $safe = $Value -replace '[^A-Za-z0-9._-]', '_'
    if ([string]::IsNullOrWhiteSpace($safe)) { return 'unnamed' }
    return $safe
}

function ConvertTo-WindowsCommandLineArgument {
    param([AllowEmptyString()][string]$Value)

    if ($null -eq $Value -or $Value.Length -eq 0) { return '""' }
    if ($Value -notmatch '[\s"]') { return $Value }

    $builder = New-Object System.Text.StringBuilder
    [void]$builder.Append('"')
    $slashes = 0
    foreach ($character in $Value.ToCharArray()) {
        if ($character -eq '\') {
            $slashes++
            continue
        }
        if ($character -eq '"') {
            [void]$builder.Append(('\' * (($slashes * 2) + 1)))
            [void]$builder.Append('"')
            $slashes = 0
            continue
        }
        if ($slashes -gt 0) {
            [void]$builder.Append(('\' * $slashes))
            $slashes = 0
        }
        [void]$builder.Append($character)
    }
    if ($slashes -gt 0) {
        [void]$builder.Append(('\' * ($slashes * 2)))
    }
    [void]$builder.Append('"')
    return $builder.ToString()
}

function Write-ShanmenJsonAtomic {
    param(
        [Parameter(Mandatory)][string]$Path,
        [Parameter(Mandatory)]$Value
    )

    $directory = Split-Path -Parent $Path
    if (-not [string]::IsNullOrWhiteSpace($directory)) {
        New-Item -ItemType Directory -Path $directory -Force | Out-Null
    }
    $temporary = "$Path.tmp-$([Guid]::NewGuid().ToString('N'))"
    $json = ($Value | ConvertTo-Json -Depth 16) + [Environment]::NewLine
    [IO.File]::WriteAllText(
        $temporary,
        $json,
        (New-Object Text.UTF8Encoding($false)))
    if (Test-Path -LiteralPath $Path -PathType Leaf) {
        $replacementBackup = "$Path.replace-backup-$([Guid]::NewGuid().ToString('N'))"
        [IO.File]::Replace($temporary, $Path, $replacementBackup, $true)
        if (Test-Path -LiteralPath $replacementBackup -PathType Leaf) {
            [IO.File]::Delete($replacementBackup)
        }
    }
    else {
        [IO.File]::Move($temporary, $Path)
    }
}

function Test-ShanmenPathWithin {
    param(
        [Parameter(Mandatory)][string]$Candidate,
        [Parameter(Mandatory)][string]$Parent
    )
    $candidateFull = [IO.Path]::GetFullPath($Candidate).TrimEnd('\', '/')
    $parentFull = [IO.Path]::GetFullPath($Parent).TrimEnd('\', '/')
    return $candidateFull.Equals($parentFull, [StringComparison]::OrdinalIgnoreCase) -or
        $candidateFull.StartsWith(
            $parentFull + [IO.Path]::DirectorySeparatorChar,
            [StringComparison]::OrdinalIgnoreCase)
}

function Resolve-ShanmenProject {
    [CmdletBinding()]
    param(
        [string]$ProjectRoot = (Split-Path -Parent $PSScriptRoot),
        [string]$EngineRoot,
        [switch]$AllowMissingEngine
    )

    $projectRootFull = [IO.Path]::GetFullPath($ProjectRoot).TrimEnd('\', '/')
    $uproject = Join-Path $projectRootFull 'demo_map.uproject'
    if (-not (Test-Path -LiteralPath $uproject -PathType Leaf)) {
        throw "Project descriptor missing: $uproject"
    }

    $descriptor = Get-Content -LiteralPath $uproject -Raw | ConvertFrom-Json
    $association = [string]$descriptor.EngineAssociation
    $candidates = New-Object System.Collections.Generic.List[string]
    if (-not [string]::IsNullOrWhiteSpace($EngineRoot)) { $candidates.Add($EngineRoot) }
    if (-not [string]::IsNullOrWhiteSpace($env:SHANMEN_UE_ROOT)) {
        $candidates.Add($env:SHANMEN_UE_ROOT)
    }

    foreach ($registryPath in @(
        "HKLM:\SOFTWARE\EpicGames\Unreal Engine\$association",
        "HKLM:\SOFTWARE\WOW6432Node\EpicGames\Unreal Engine\$association")) {
        try {
            $installed = (Get-ItemProperty -LiteralPath $registryPath -ErrorAction Stop).InstalledDirectory
            if (-not [string]::IsNullOrWhiteSpace($installed)) { $candidates.Add($installed) }
        }
        catch { }
    }

    if (-not [string]::IsNullOrWhiteSpace($association)) {
        $candidates.Add("C:\Program Files\Epic Games\UE_$association")
    }

    $resolvedEngine = $null
    foreach ($candidate in $candidates) {
        if ([string]::IsNullOrWhiteSpace($candidate)) { continue }
        $full = [IO.Path]::GetFullPath($candidate).TrimEnd('\', '/')
        $editor = Join-Path $full 'Engine\Binaries\Win64\UnrealEditor.exe'
        $build = Join-Path $full 'Engine\Build\BatchFiles\Build.bat'
        $uat = Join-Path $full 'Engine\Build\BatchFiles\RunUAT.bat'
        if ((Test-Path -LiteralPath $editor -PathType Leaf) -and
            (Test-Path -LiteralPath $build -PathType Leaf) -and
            (Test-Path -LiteralPath $uat -PathType Leaf)) {
            $resolvedEngine = $full
            break
        }
    }

    if (-not $resolvedEngine -and -not $AllowMissingEngine) {
        throw "Unable to resolve Unreal Engine $association. Set SHANMEN_UE_ROOT or pass -EngineRoot."
    }

    [pscustomobject]@{
        ProjectRoot = $projectRootFull
        Uproject = $uproject
        EngineAssociation = $association
        EngineRoot = $resolvedEngine
        Editor = if ($resolvedEngine) { Join-Path $resolvedEngine 'Engine\Binaries\Win64\UnrealEditor.exe' } else { $null }
        EditorCmd = if ($resolvedEngine) { Join-Path $resolvedEngine 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe' } else { $null }
        BuildBat = if ($resolvedEngine) { Join-Path $resolvedEngine 'Engine\Build\BatchFiles\Build.bat' } else { $null }
        RunUat = if ($resolvedEngine) { Join-Path $resolvedEngine 'Engine\Build\BatchFiles\RunUAT.bat' } else { $null }
        DefaultMap = '/Game/M01/Maps/L_M01_Expedition?Name=Player'
    }
}

function New-ShanmenEvidenceContext {
    param(
        [Parameter(Mandatory)]$Project,
        [Parameter(Mandatory)][string]$Action,
        [string]$TaskId = 'Manual.Foundation'
    )
    $stamp = (Get-Date).ToUniversalTime().ToString('yyyyMMddTHHmmssfffZ')
    $root = Join-Path $Project.ProjectRoot (
        'Saved\FoundationRuns\{0}\{1}\{2}-{3}' -f
            (ConvertTo-ShanmenSafeName $TaskId),
            (ConvertTo-ShanmenSafeName $Action),
            $stamp,
            [Guid]::NewGuid().ToString('N').Substring(0, 8))
    New-Item -ItemType Directory -Path $root -Force | Out-Null
    [pscustomobject]@{
        Root = $root
        StatePath = Join-Path $root 'run-state.json'
        StdoutPath = Join-Path $root 'stdout.log'
        StderrPath = Join-Path $root 'stderr.log'
    }
}

function Start-ShanmenTrackedProcess {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)][string]$FilePath,
        [string[]]$ArgumentList = @(),
        [Parameter(Mandatory)][string]$WorkingDirectory,
        [Parameter(Mandatory)]$Evidence,
        [switch]$Wait,
        [switch]$Hidden,
        [switch]$CaptureOutput
    )

    if (-not (Test-Path -LiteralPath $FilePath -PathType Leaf)) {
        throw "Executable missing: $FilePath"
    }
    $encodedArguments = @($ArgumentList | ForEach-Object {
        ConvertTo-WindowsCommandLineArgument ([string]$_)
    })
    $startParameters = @{
        FilePath = $FilePath
        ArgumentList = $encodedArguments
        WorkingDirectory = $WorkingDirectory
        PassThru = $true
    }
    if ($Hidden) { $startParameters.WindowStyle = 'Hidden' }
    if ($Wait) { $startParameters.Wait = $true }
    if ($CaptureOutput) {
        $startParameters.RedirectStandardOutput = $Evidence.StdoutPath
        $startParameters.RedirectStandardError = $Evidence.StderrPath
    }

    $startedUtc = (Get-Date).ToUniversalTime().ToString('o')
    $process = Start-Process @startParameters
    $state = [ordered]@{
        schema_version = 1
        state = 'STARTED'
        started_utc = $startedUtc
        completed_utc = $null
        file_path = [IO.Path]::GetFullPath($FilePath)
        arguments = @($ArgumentList)
        working_directory = [IO.Path]::GetFullPath($WorkingDirectory)
        pid = $process.Id
        exit_code = $null
        evidence_root = $Evidence.Root
    }
    Write-ShanmenJsonAtomic -Path $Evidence.StatePath -Value $state

    if ($Wait) {
        $process.Refresh()
        $exitCode = $process.ExitCode
        $state.state = if ($exitCode -eq 0) { 'SUCCEEDED' } else { 'FAILED' }
        $state.completed_utc = (Get-Date).ToUniversalTime().ToString('o')
        $state.exit_code = $exitCode
        Write-ShanmenJsonAtomic -Path $Evidence.StatePath -Value $state
    }
    return [pscustomobject]@{
        Process = $process
        State = $state
        Evidence = $Evidence
    }
}

function Invoke-ShanmenBuild {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)]$Project,
        [Parameter(Mandatory)][ValidateSet('Editor', 'Game')][string]$Target,
        [ValidateSet('DebugGame', 'Development', 'Shipping', 'Test')][string]$Configuration = 'Development',
        [Parameter(Mandatory)]$Evidence,
        [int]$MaxParallelActions = 1,
        [switch]$UseUba
    )

    $targetName = if ($Target -eq 'Editor') { 'demo_mapEditor' } else { 'demo_map' }
    $buildArguments = @(
        $targetName,
        'Win64',
        $Configuration,
        "-Project=$($Project.Uproject)",
        '-WaitMutex',
        '-NoHotReloadFromIDE',
        "-MaxParallelActions=$MaxParallelActions"
    )
    if (-not $UseUba) { $buildArguments += '-NoUBA' }

    # Start the batch entrypoint directly. Wrapping an already quoted command in
    # cmd.exe /s /c causes Start-Process to quote the complete command again,
    # leaving cmd.exe to look for a literal path that begins with a quote.
    $processParameters = @{
        FilePath = $Project.BuildBat
        ArgumentList = $buildArguments
        WorkingDirectory = Split-Path -Parent $Project.BuildBat
        Evidence = $Evidence
        Wait = $true
        Hidden = $true
        CaptureOutput = $true
    }
    return Start-ShanmenTrackedProcess @processParameters
}

function Get-ShanmenLatestPackagedExecutable {
    param([Parameter(Mandatory)]$Project)
    $latestRoot = Join-Path $Project.ProjectRoot 'Latest_Demo'
    foreach ($candidate in @(
        (Join-Path $latestRoot 'Windows\demo_map.exe'),
        (Join-Path $latestRoot 'demo_map.exe'),
        (Join-Path $latestRoot 'Windows\demo_map\Binaries\Win64\demo_map.exe'))) {
        if (Test-Path -LiteralPath $candidate -PathType Leaf) {
            return [IO.Path]::GetFullPath($candidate)
        }
    }
    return $null
}

function Get-ShanmenLatestLaunchPlan {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory)]$Project,
        [switch]$PackagedOnly,
        [string]$LogPath
    )

    $packaged = Get-ShanmenLatestPackagedExecutable -Project $Project
    if ($packaged) {
        return [pscustomobject]@{
            Mode = 'PACKAGED_CANDIDATE'
            FilePath = $packaged
            WorkingDirectory = Split-Path -Parent $packaged
            Arguments = @()
            PackagedArtifactPresent = $true
        }
    }
    if ($PackagedOnly) {
        throw 'No verified packaged Latest_Demo is installed.'
    }

    $arguments = @($Project.Uproject, $Project.DefaultMap, '-game')
    if (-not [string]::IsNullOrWhiteSpace($LogPath)) {
        $arguments += "-abslog=$LogPath"
    }
    return [pscustomobject]@{
        Mode = 'EDITOR_GAME_FALLBACK'
        FilePath = $Project.Editor
        WorkingDirectory = $Project.ProjectRoot
        Arguments = $arguments
        PackagedArtifactPresent = $false
    }
}

Export-ModuleMember -Function @(
    'ConvertTo-ShanmenSafeName',
    'ConvertTo-WindowsCommandLineArgument',
    'Write-ShanmenJsonAtomic',
    'Test-ShanmenPathWithin',
    'Resolve-ShanmenProject',
    'New-ShanmenEvidenceContext',
    'Start-ShanmenTrackedProcess',
    'Invoke-ShanmenBuild',
    'Get-ShanmenLatestPackagedExecutable',
    'Get-ShanmenLatestLaunchPlan'
)
