[CmdletBinding()]
param()

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$Validator = Join-Path $PSScriptRoot 'Test-ShanmenRegressionCoverage.ps1'
$Mapping = Join-Path $PSScriptRoot 'ShanmenRegressionMap.json'
$FixtureRoot = Join-Path ([System.IO.Path]::GetTempPath()) (
    'shanmen-regression-coverage-' + [guid]::NewGuid().ToString('N'))
[void](New-Item -ItemType Directory -Path $FixtureRoot)
$script:SelfTestPassCount = 0

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
        $script:SelfTestPassCount++
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
        $script:SelfTestPassCount++
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
    $ItemUse = New-AutomationLogFixture `
        -Name 'item-use.log' `
        -Group 'demo_map.ItemUseAndArmor'
    $ThrownRuntime = New-AutomationLogFixture `
        -Name 'thrown-runtime.log' `
        -Group 'Shanmen.0_0_10.CombatRuntime.ThrownWeapon'
    $FormationAdapter = New-AutomationLogFixture `
        -Name 'formation-adapter.log' `
        -Group 'Shanmen.0_0_10.Product.FormationMaterialAdapter'
    $FormationSession = New-AutomationLogFixture `
        -Name 'formation-session.log' `
        -Group 'Shanmen.0_0_10.Product.FormationSession'
    $FormationWorld = New-AutomationLogFixture `
        -Name 'formation-world.log' `
        -Group 'Shanmen.0_0_10.Product.FormationWorldDelivery'
    $FormationHost = New-AutomationLogFixture `
        -Name 'formation-host.log' `
        -Group 'Shanmen.0_0_10.Product.FormationProductHost'
    $FormationInfluenceHost = New-AutomationLogFixture `
        -Name 'formation-influence-host.log' `
        -Group 'Shanmen.0_0_10.Product.FormationInfluenceHost'
    $FormationArea = New-AutomationLogFixture `
        -Name 'formation-area.log' `
        -Group 'Shanmen.0_0_10.Product.FormationAreaProvider'
    $FormationWorldCoverage = New-AutomationLogFixture `
        -Name 'formation-world-coverage.log' `
        -Group 'Shanmen.0_0_10.Product.FormationWorldCoverage'
    $FormationCoverageTransitions = New-AutomationLogFixture `
        -Name 'formation-coverage-transitions.log' `
        -Group 'Shanmen.0_0_10.Product.FormationCoverageTransitions'
    $FormationCoverageTracker = New-AutomationLogFixture `
        -Name 'formation-coverage-tracker.log' `
        -Group 'Shanmen.0_0_10.Product.FormationCoverageTracker'
    $FormationCoverageCoordinator = New-AutomationLogFixture `
        -Name 'formation-coverage-coordinator.log' `
        -Group 'Shanmen.0_0_10.Product.FormationCoverageCoordinator'
    $FormationInfluenceIntents = New-AutomationLogFixture `
        -Name 'formation-influence-intents.log' `
        -Group 'Shanmen.0_0_10.Product.FormationInfluenceIntents'
    $FormationInfluenceReconciliation = New-AutomationLogFixture `
        -Name 'formation-influence-reconciliation.log' `
        -Group 'Shanmen.0_0_10.Product.FormationInfluenceReconciliation'
    $FormationInfluenceDispatch = New-AutomationLogFixture `
        -Name 'formation-influence-dispatch.log' `
        -Group 'Shanmen.0_0_10.Product.FormationInfluenceDispatch'
    $FormationInfluenceExecutor = New-AutomationLogFixture `
        -Name 'formation-influence-executor.log' `
        -Group 'Shanmen.0_0_10.Product.FormationInfluenceExecutor'
    $FormationInfluenceLeaseExecutor = New-AutomationLogFixture `
        -Name 'formation-influence-lease-executor.log' `
        -Group 'Shanmen.0_0_10.Product.FormationInfluenceLeaseExecutor'
    $FormationInfluenceProductRuntime = New-AutomationLogFixture `
        -Name 'formation-influence-product-runtime.log' `
        -Group 'Shanmen.0_0_10.Product.FormationInfluenceProductRuntime'
    $FormationInfluenceExecutionRouter = New-AutomationLogFixture `
        -Name 'formation-influence-execution-router.log' `
        -Group 'Shanmen.0_0_10.Product.FormationInfluenceExecutionRouter'
    $FormationInfluenceExecutionService = New-AutomationLogFixture `
        -Name 'formation-influence-execution-service.log' `
        -Group 'Shanmen.0_0_10.Product.FormationInfluenceExecutionService'
    $FormationInfluenceLifecycleCoordinator = New-AutomationLogFixture `
        -Name 'formation-influence-lifecycle-coordinator.log' `
        -Group 'Shanmen.0_0_10.Product.FormationInfluenceLifecycleCoordinator'
    $Legacy = New-AutomationLogFixture `
        -Name 'legacy.log' `
        -Group 'demo_map'
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
        -Name 'formation deployment core is covered by broad full evidence' `
        -Paths @(
            'Source/ShanmenCombatRuntime/Private/ShanmenFormationDeployment.cpp') `
        -Logs @($Full)

    Invoke-ExpectedPass `
        -Name 'formation material adapter is covered by broad full evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenFormationMaterialAdapter.cpp') `
        -Logs @($Full)

    Invoke-ExpectedPass `
        -Name 'formation product session is covered by broad full evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenFormationProductSession.cpp') `
        -Logs @($Full)

    Invoke-ExpectedPass `
        -Name 'formation world delivery is covered by broad full evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenFormationWorldAdapter.cpp') `
        -Logs @($Full)

    Invoke-ExpectedPass `
        -Name 'formation product host is covered by broad full evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenFormationProductHost.cpp') `
        -Logs @($Full)

    Invoke-ExpectedPass `
        -Name 'formation area provider is covered by broad full evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenFormationAreaProvider.cpp') `
        -Logs @($Full)

    Invoke-ExpectedPass `
        -Name 'formation World coverage is covered by broad full evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenFormationWorldCoverageSampler.cpp') `
        -Logs @($Full)

    Invoke-ExpectedPass `
        -Name 'formation coverage transitions are covered by broad full evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenFormationCoverageTransitionReducer.cpp') `
        -Logs @($Full)

    Invoke-ExpectedPass `
        -Name 'formation coverage tracker is covered by broad full evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenFormationCoverageTracker.cpp') `
        -Logs @($Full)

    Invoke-ExpectedPass `
        -Name 'formation coverage coordinator is covered by broad full evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenFormationCoverageCoordinator.cpp') `
        -Logs @($Full)

    Invoke-ExpectedPass `
        -Name 'formation influence intents are covered by broad full evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenFormationInfluenceIntentPlanner.cpp') `
        -Logs @($Full)

    Invoke-ExpectedPass `
        -Name 'formation influence reconciliation is covered by broad full evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenFormationInfluenceReconciliationPlanner.cpp') `
        -Logs @($Full)

    Invoke-ExpectedPass `
        -Name 'formation influence dispatch is covered by broad full evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenFormationInfluenceDispatchLedger.cpp') `
        -Logs @($Full)

    Invoke-ExpectedPass `
        -Name 'formation influence executor is covered by broad full evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenFormationInfluenceExecutorAdapter.cpp') `
        -Logs @($Full)

    Invoke-ExpectedPass `
        -Name 'formation influence lease executor is covered by broad full evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenFormationInfluenceLeaseExecutor.cpp') `
        -Logs @($Full)

    Invoke-ExpectedPass `
        -Name 'formation influence product runtime is covered by broad full evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenFormationInfluenceProductRuntime.cpp') `
        -Logs @($Full)

    Invoke-ExpectedPass `
        -Name 'formation influence execution router is covered by broad full evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenFormationInfluenceExecutionRouter.cpp') `
        -Logs @($Full)

    Invoke-ExpectedPass `
        -Name 'formation influence execution service is covered by broad full evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenFormationInfluenceExecutionService.cpp') `
        -Logs @($Full)

    Invoke-ExpectedPass `
        -Name 'formation influence lifecycle coordinator is covered by broad full evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenFormationInfluenceLifecycleCoordinator.cpp') `
        -Logs @($Full)

    Invoke-ExpectedPass `
        -Name 'player health maps to focused vitality plus full regression' `
        -Paths @('Source/demo_map/demo_mapPlayerHealthComponent.cpp') `
        -Logs @($Full, $PlayerVitality)

    Invoke-ExpectedPass `
        -Name 'controlled weapon run host is covered by the full suite' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenControlledWeaponRunHost.cpp') `
        -Logs @($Full)

    Invoke-ExpectedPass `
        -Name 'controlled weapon Run lifecycle is covered by the full suite' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenControlledWeaponRunLifecycle.cpp') `
        -Logs @($Full)

    Invoke-ExpectedPass `
        -Name 'controlled weapon Run command router is covered by the full suite' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenControlledWeaponRunCommandRouter.cpp') `
        -Logs @($Full)

    Invoke-ExpectedPass `
        -Name 'controlled weapon threat sample router is covered by the full suite' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenControlledWeaponThreatSampleRouter.cpp') `
        -Logs @($Full)

    Invoke-ExpectedPass `
        -Name 'thrown weapon item adapter maps to product, items, and runtime' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenThrownWeaponItemAdapter.cpp') `
        -Logs @($Full)

    Invoke-ExpectedPass `
        -Name 'thrown weapon world delivery is covered by the full suite' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenThrownWeaponWorldAdapter.cpp',
            'Source/demo_map/demo_mapShanmenThrownWeaponProjectile.cpp') `
        -Logs @($Full)

    Invoke-ExpectedPass `
        -Name 'thrown weapon Run host maps to every owned authority seam' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenThrownWeaponRunHost.cpp') `
        -Logs @($Full)

    Invoke-ExpectedPass `
        -Name 'thrown weapon Run command router maps to every routed authority seam' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenThrownWeaponRunCommandRouter.cpp') `
        -Logs @($Full)

    Invoke-ExpectedPass `
        -Name 'thrown weapon product controller maps selection through every authority seam' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenThrownWeaponProductController.cpp') `
        -Logs @($Full)

    Invoke-ExpectedPass `
        -Name 'thrown weapon product session maps hotbar and combat stat seams' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenThrownWeaponProductSession.cpp') `
        -Logs @($Full, $ItemUse)

    Invoke-ExpectedPass `
        -Name 'thrown weapon product lifecycle maps every owned authority seam' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenThrownWeaponProductLifecycle.cpp') `
        -Logs @($Full, $ItemUse)

    Invoke-ExpectedPass `
        -Name 'thrown weapon input adapter maps typed hotbar and fallback seams' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenThrownWeaponInputAdapter.cpp') `
        -Logs @($Full, $ItemUse)

    Invoke-ExpectedPass `
        -Name 'canonical item catalog requires every direct product consumer' `
        -Paths @(
            'Source/demo_map/demo_mapItemDefinitions.cpp',
            'Source/demo_map/demo_mapWorldInteractionTests.cpp') `
        -Logs @($Full, $Legacy)

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
        -Name 'unrelated thrown runtime evidence cannot cover formation deployment' `
        -Paths @(
            'Source/ShanmenCombatRuntime/Private/ShanmenFormationDeployment.cpp') `
        -Logs @($ThrownRuntime) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'formation adapter child evidence cannot replace items and formation contracts' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenFormationMaterialAdapter.cpp') `
        -Logs @($FormationAdapter) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'formation session child evidence cannot replace owned authority contracts' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenFormationProductSession.cpp') `
        -Logs @($FormationSession) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'formation world child evidence cannot replace session and world contracts' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenFormationWorldAdapter.cpp') `
        -Logs @($FormationWorld) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'formation host focused evidence cannot replace owned authority contracts' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenFormationProductHost.cpp') `
        -Logs @($FormationHost, $FormationInfluenceHost) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'formation area child evidence cannot replace placement and host contracts' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenFormationAreaProvider.cpp') `
        -Logs @($FormationArea) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'formation World coverage child evidence cannot replace area and placement contracts' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenFormationWorldCoverageSampler.cpp') `
        -Logs @($FormationWorldCoverage) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'formation transition child evidence cannot replace coverage and placement contracts' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenFormationCoverageTransitionReducer.cpp') `
        -Logs @($FormationCoverageTransitions) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'formation tracker child evidence cannot replace transition and coverage contracts' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenFormationCoverageTracker.cpp') `
        -Logs @($FormationCoverageTracker) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'formation coordinator child evidence cannot replace tracker and World contracts' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenFormationCoverageCoordinator.cpp') `
        -Logs @($FormationCoverageCoordinator) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'formation influence child evidence cannot replace transition and World contracts' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenFormationInfluenceIntentPlanner.cpp') `
        -Logs @($FormationInfluenceIntents) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'formation reconciliation child evidence cannot replace lifecycle and World contracts' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenFormationInfluenceReconciliationPlanner.cpp') `
        -Logs @($FormationInfluenceReconciliation) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'formation dispatch child evidence cannot replace planners and World contracts' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenFormationInfluenceDispatchLedger.cpp') `
        -Logs @($FormationInfluenceDispatch) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'formation executor child evidence cannot replace Host and ledger contracts' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenFormationInfluenceExecutorAdapter.cpp') `
        -Logs @($FormationInfluenceExecutor) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'formation lease child evidence cannot replace adapter Host and ledger contracts' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenFormationInfluenceLeaseExecutor.cpp') `
        -Logs @($FormationInfluenceLeaseExecutor) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'formation product runtime child evidence cannot replace lease Host and planner contracts' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenFormationInfluenceProductRuntime.cpp') `
        -Logs @($FormationInfluenceProductRuntime) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'formation execution router child evidence cannot replace runtime Host and planner contracts' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenFormationInfluenceExecutionRouter.cpp') `
        -Logs @($FormationInfluenceExecutionRouter) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'formation execution service child evidence cannot replace router runtime and Host contracts' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenFormationInfluenceExecutionService.cpp') `
        -Logs @($FormationInfluenceExecutionService) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'formation lifecycle coordinator child evidence cannot replace service Host and teardown contracts' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenFormationInfluenceLifecycleCoordinator.cpp') `
        -Logs @($FormationInfluenceLifecycleCoordinator) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'run host cannot use coordinator-only evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenControlledWeaponRunHost.cpp') `
        -Logs @($Coordinator) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'Run lifecycle cannot use coordinator-only evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenControlledWeaponRunLifecycle.cpp') `
        -Logs @($Coordinator) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'Run command router cannot use coordinator-only evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenControlledWeaponRunCommandRouter.cpp') `
        -Logs @($Coordinator) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'threat sample router cannot use coordinator-only evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenControlledWeaponThreatSampleRouter.cpp') `
        -Logs @($Coordinator) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'thrown command router cannot use coordinator-only evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenThrownWeaponRunCommandRouter.cpp') `
        -Logs @($Coordinator) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'thrown product controller cannot use coordinator-only evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenThrownWeaponProductController.cpp') `
        -Logs @($Coordinator) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'thrown product session requires legacy combat snapshot evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenThrownWeaponProductSession.cpp') `
        -Logs @($Full) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'thrown product lifecycle requires legacy combat snapshot evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenThrownWeaponProductLifecycle.cpp') `
        -Logs @($Full) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'thrown input adapter requires ordinary hotbar fallback evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenThrownWeaponInputAdapter.cpp') `
        -Logs @($Full) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'canonical item catalog cannot use new-module evidence alone' `
        -Paths @(
            'Source/demo_map/demo_mapItemDefinitions.cpp') `
        -Logs @($Full) `
        -ExpectedText 'missing required groups'

    Write-Output (
        'SELF_TEST: PASS {0}/{0}' -f $script:SelfTestPassCount)
}
finally
{
    if (Test-Path -LiteralPath $FixtureRoot)
    {
        Remove-Item -LiteralPath $FixtureRoot -Recurse -Force
    }
}
