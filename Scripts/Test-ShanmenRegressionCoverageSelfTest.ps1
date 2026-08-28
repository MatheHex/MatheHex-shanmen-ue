[CmdletBinding()]
param()

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$Validator = Join-Path $PSScriptRoot 'Test-ShanmenRegressionCoverage.ps1'
$Mapping = Join-Path $PSScriptRoot 'ShanmenRegressionMap.json'
$FixtureRoot = Join-Path ([System.IO.Path]::GetTempPath()) (
    'shanmen-regression-coverage-' + [guid]::NewGuid().ToString('N'))
[void](New-Item -ItemType Directory -Path $FixtureRoot)

function New-AutomationLogFixture {
    param(
        [Parameter(Mandatory)][string]$Name,
        [Parameter(Mandatory)][string]$Group,
        [ValidateSet('Success', 'Fail')][string]$Result = 'Success',
        [switch]$OmitQueueEmpty
    )

    $Path = Join-Path $FixtureRoot $Name
    $Lines = [System.Collections.Generic.List[string]]::new()
    $Lines.Add("[2026.08.28-00.00.00:000][  0]Cmd: Automation RunTests $Group")
    $Lines.Add("[2026.08.28-00.00.00:001][  1]LogAutomationController: Display: Test Completed. Result={$Result} Name={Fixture} Path={$Group.Fixture}")
    if (-not $OmitQueueEmpty)
    {
        $Lines.Add('[2026.08.28-00.00.00:002][  2]LogAutomationCommandLine: Display: ...Automation Test Queue Empty 1 tests performed.')
    }
    Set-Content -LiteralPath $Path -Value $Lines -Encoding utf8
    return $Path
}

function Invoke-ExpectedPass {
    param(
        [Parameter(Mandatory)][string]$Name,
        [Parameter(Mandatory)][string[]]$Paths,
        [string[]]$Logs = @()
    )

    try
    {
        $Output = @(& $Validator `
                -ChangedPath $Paths `
                -AutomationLogPath $Logs `
                -MappingPath $Mapping)
        if (-not ($Output -match '^REGRESSION_COVERAGE: PASS'))
        {
            throw 'PASS marker missing'
        }
        Write-Output "SELF_TEST: PASS $Name"
    }
    catch
    {
        throw "SELF_TEST: expected PASS for $Name but failed: $($_.Exception.Message)"
    }
}

function Invoke-ExpectedFail {
    param(
        [Parameter(Mandatory)][string]$Name,
        [Parameter(Mandatory)][string[]]$Paths,
        [string[]]$Logs = @(),
        [Parameter(Mandatory)][string]$ExpectedText
    )

    try
    {
        [void]@(& $Validator `
                -ChangedPath $Paths `
                -AutomationLogPath $Logs `
                -MappingPath $Mapping)
    }
    catch
    {
        if ($_.Exception.Message -notlike "*$ExpectedText*")
        {
            throw "SELF_TEST: $Name failed for the wrong reason: $($_.Exception.Message)"
        }
        Write-Output "SELF_TEST: PASS $Name"
        return
    }
    throw "SELF_TEST: expected failure for $Name"
}

try
{
    $Full = New-AutomationLogFixture `
        -Name 'full.log' `
        -Group 'Shanmen.0_0_10'
    $Enemy = New-AutomationLogFixture `
        -Name 'enemy.log' `
        -Group 'demo_map.EnemySkillFramework'
    $Ranged = New-AutomationLogFixture `
        -Name 'ranged.log' `
        -Group 'demo_map.V2RangedCompatibility'
    $Coordinator = New-AutomationLogFixture `
        -Name 'coordinator.log' `
        -Group 'Shanmen.0_0_10.Product.CombatRunCoordinator'
    $PlayerVitality = New-AutomationLogFixture `
        -Name 'player-vitality.log' `
        -Group 'Shanmen.0_0_10.Product.PlayerVitality'
    $FailedRanged = New-AutomationLogFixture `
        -Name 'ranged-fail.log' `
        -Group 'demo_map.V2RangedCompatibility' `
        -Result Fail
    $NoQueue = New-AutomationLogFixture `
        -Name 'no-queue.log' `
        -Group 'demo_map.V2RangedCompatibility' `
        -OmitQueueEmpty

    Invoke-ExpectedPass `
        -Name 'overlapping rules union and broad suite coverage' `
        -Paths @(
            'Source\ShanmenCombatCore\Private\Resolver.cpp',
            'Source/demo_map/demo_mapSkillProjectile.cpp') `
        -Logs @($Full, $Enemy, $Ranged)

    Invoke-ExpectedPass `
        -Name 'docs and scripts require no product log' `
        -Paths @(
            'Docs/Report/example.md',
            'Scripts/example.ps1')

    Invoke-ExpectedPass `
        -Name 'player health maps to focused vitality plus full regression' `
        -Paths @('Source/demo_map/demo_mapPlayerHealthComponent.cpp') `
        -Logs @($Full, $PlayerVitality)

    Invoke-ExpectedPass `
        -Name 'controlled weapon run host is covered by the full suite' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenControlledWeaponRunHost.cpp') `
        -Logs @($Full)

    Invoke-ExpectedFail `
        -Name 'missing mapped group fails closed' `
        -Paths @('Source/demo_map/demo_mapSkillComponent.cpp') `
        -Logs @($Full) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'failed test evidence is rejected' `
        -Paths @('Source/demo_map/demo_mapSkillProjectile.cpp') `
        -Logs @($Full, $Enemy, $FailedRanged) `
        -ExpectedText 'unhealthy evidence'

    Invoke-ExpectedFail `
        -Name 'queue completion is required' `
        -Paths @('Source/demo_map/demo_mapSkillProjectile.cpp') `
        -Logs @($Full, $Enemy, $NoQueue) `
        -ExpectedText 'queue-empty marker missing'

    Invoke-ExpectedFail `
        -Name 'unknown production path is unmapped' `
        -Paths @('Source/demo_map/demo_mapUnknownAuthority.cpp') `
        -Logs @($Full) `
        -ExpectedText 'unmapped changed paths'

    Invoke-ExpectedFail `
        -Name 'narrow child suite does not satisfy required parent suite' `
        -Paths @('Source/demo_map/demo_mapGameMode.cpp') `
        -Logs @($Coordinator) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'run host cannot use coordinator-only evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenControlledWeaponRunHost.cpp') `
        -Logs @($Coordinator) `
        -ExpectedText 'missing required groups'

    Write-Output 'SELF_TEST: PASS 10/10'
}
finally
{
    if (Test-Path -LiteralPath $FixtureRoot)
    {
        Remove-Item -LiteralPath $FixtureRoot -Recurse -Force
    }
}
