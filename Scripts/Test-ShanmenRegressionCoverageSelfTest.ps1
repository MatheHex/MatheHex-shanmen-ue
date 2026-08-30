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
    $CombatRuntime = New-AutomationLogFixture `
        -Name 'combat-runtime.log' `
        -Group 'Shanmen.0_0_10.CombatRuntime'
    $ActionResource = New-AutomationLogFixture `
        -Name 'action-resource.log' `
        -Group 'Shanmen.0_0_10.CombatRuntime.ActionResource'
    $ActionLifecycle = New-AutomationLogFixture `
        -Name 'action-lifecycle.log' `
        -Group 'Shanmen.0_0_10.CombatRuntime.ActionLifecycle'
    $SpiritShieldAction = New-AutomationLogFixture `
        -Name 'spirit-shield-action.log' `
        -Group 'Shanmen.0_0_10.CombatRuntime.SpiritShieldAction'
    $SpiritShieldRuntime = New-AutomationLogFixture `
        -Name 'spirit-shield-runtime.log' `
        -Group 'Shanmen.0_0_10.CombatRuntime.SpiritShield'
    $SpiritShieldCapacity = New-AutomationLogFixture `
        -Name 'spirit-shield-capacity.log' `
        -Group 'Shanmen.0_0_10.CombatRuntime.SpiritShieldCapacity'
    $SpiritShieldDeadline = New-AutomationLogFixture `
        -Name 'spirit-shield-deadline.log' `
        -Group 'Shanmen.0_0_10.CombatRuntime.SpiritShieldDeadline'
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
    $FormationInfluenceModifierEvaluator = New-AutomationLogFixture `
        -Name 'formation-influence-modifier-evaluator.log' `
        -Group 'Shanmen.0_0_10.Product.FormationInfluenceModifierEvaluator'
    $FormationInfluenceEvaluationBinding = New-AutomationLogFixture `
        -Name 'formation-influence-evaluation-binding.log' `
        -Group 'Shanmen.0_0_10.Product.FormationInfluenceEvaluationBinding'
    $FormationInfluenceConsumerProjection = New-AutomationLogFixture `
        -Name 'formation-influence-consumer-projection.log' `
        -Group 'Shanmen.0_0_10.Product.FormationInfluenceConsumerProjection'
    $FormationInfluenceConsumerWorldResolution = New-AutomationLogFixture `
        -Name 'formation-influence-consumer-world-resolution.log' `
        -Group 'Shanmen.0_0_10.Product.FormationInfluenceConsumerWorldResolution'
    $FormationInfluenceConsumerRunComposition = New-AutomationLogFixture `
        -Name 'formation-influence-consumer-run-composition.log' `
        -Group 'Shanmen.0_0_10.Product.FormationInfluenceConsumerRunComposition'
    $FormationInfluenceConsumerRegistry = New-AutomationLogFixture `
        -Name 'formation-influence-consumer-registry.log' `
        -Group 'Shanmen.0_0_10.Product.FormationInfluenceConsumerRegistry'
    $FormationInfluenceConsumerAttributeAdapter = New-AutomationLogFixture `
        -Name 'formation-influence-consumer-attribute-adapter.log' `
        -Group 'Shanmen.0_0_10.Product.FormationInfluenceConsumerAttributeAdapter'
    $FormationInfluenceConsumerApplicationCoordinator = New-AutomationLogFixture `
        -Name 'formation-influence-consumer-application-coordinator.log' `
        -Group 'Shanmen.0_0_10.Product.FormationInfluenceConsumerApplicationCoordinator'
    $FormationInfluenceConsumerCommandHost = New-AutomationLogFixture `
        -Name 'formation-influence-consumer-command-host.log' `
        -Group 'Shanmen.0_0_10.Product.FormationInfluenceConsumerCommandHost'
    $FormationInfluenceConsumerProductBridge = New-AutomationLogFixture `
        -Name 'formation-influence-consumer-product-bridge.log' `
        -Group 'Shanmen.0_0_10.Product.FormationInfluenceConsumerProductBridge'
    $FormationInfluenceConsumerProductRuntime = New-AutomationLogFixture `
        -Name 'formation-influence-consumer-product-runtime.log' `
        -Group 'Shanmen.0_0_10.Product.FormationInfluenceConsumerProductRuntime'
    $CombatCore = New-AutomationLogFixture `
        -Name 'combat-core.log' `
        -Group 'Shanmen.0_0_10.CombatCore'
    $Attributes = New-AutomationLogFixture `
        -Name 'attributes.log' `
        -Group 'demo_map.V3.Attributes'
    $V3Items = New-AutomationLogFixture `
        -Name 'v3-items.log' `
        -Group 'demo_map.V3.Items'
    $ItemUseAndArmor = New-AutomationLogFixture `
        -Name 'item-use-and-armor.log' `
        -Group 'demo_map.ItemUseAndArmor'
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
    $FormationInfluenceLifecycleCommandRouter = New-AutomationLogFixture `
        -Name 'formation-influence-lifecycle-command-router.log' `
        -Group 'Shanmen.0_0_10.Product.FormationInfluenceLifecycleCommandRouter'
    $FormationInfluenceLifecycleCommandHost = New-AutomationLogFixture `
        -Name 'formation-influence-lifecycle-command-host.log' `
        -Group 'Shanmen.0_0_10.Product.FormationInfluenceLifecycleCommandHost'
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
        -Name 'spirit shield runtime requires focused lifecycle and CombatCore evidence' `
        -Paths @(
            'Source/ShanmenCombatRuntime/Public/ShanmenSpiritShield.h',
            'Source/ShanmenCombatRuntime/Private/ShanmenSpiritShield.cpp',
            'Source/ShanmenCombatRuntime/Private/Tests/ShanmenSpiritShieldTests.cpp') `
        -Logs @($SpiritShieldRuntime, $CombatRuntime, $CombatCore)

    Invoke-ExpectedPass `
        -Name 'spirit shield capacity authority requires capacity lifecycle and CombatCore evidence' `
        -Paths @(
            'Source/ShanmenCombatRuntime/Public/ShanmenSpiritShieldCapacityAuthority.h',
            'Source/ShanmenCombatRuntime/Private/ShanmenSpiritShieldCapacityAuthority.cpp',
            'Source/ShanmenCombatRuntime/Private/Tests/ShanmenSpiritShieldCapacityAuthorityTests.cpp') `
        -Logs @(
            $SpiritShieldCapacity,
            $SpiritShieldRuntime,
            $CombatRuntime,
            $CombatCore)

    Invoke-ExpectedPass `
        -Name 'spirit shield deadline gate requires deadline lifecycle and capacity composition evidence' `
        -Paths @(
            'Source/ShanmenCombatRuntime/Public/ShanmenSpiritShieldDeadlineGate.h',
            'Source/ShanmenCombatRuntime/Private/ShanmenSpiritShieldDeadlineGate.cpp',
            'Source/ShanmenCombatRuntime/Private/Tests/ShanmenSpiritShieldDeadlineGateTests.cpp') `
        -Logs @(
            $SpiritShieldDeadline,
            $SpiritShieldCapacity,
            $SpiritShieldRuntime,
            $CombatRuntime)

    Invoke-ExpectedPass `
        -Name 'action resource authority requires transaction lifecycle and action transition evidence' `
        -Paths @(
            'Source/ShanmenCombatRuntime/Public/ShanmenActionResourceAuthority.h',
            'Source/ShanmenCombatRuntime/Private/ShanmenActionResourceAuthority.cpp',
            'Source/ShanmenCombatRuntime/Private/Tests/ShanmenActionResourceAuthorityTests.cpp') `
        -Logs @($ActionResource, $ActionLifecycle, $CombatRuntime)

    Invoke-ExpectedPass `
        -Name 'spirit shield action coordinator requires every composed authority contract' `
        -Paths @(
            'Source/ShanmenCombatRuntime/Public/ShanmenSpiritShieldActionCoordinator.h',
            'Source/ShanmenCombatRuntime/Private/ShanmenSpiritShieldActionCoordinator.cpp',
            'Source/ShanmenCombatRuntime/Private/Tests/ShanmenSpiritShieldActionCoordinatorTests.cpp') `
        -Logs @(
            $SpiritShieldAction,
            $ActionResource,
            $ActionLifecycle,
            $SpiritShieldRuntime,
            $SpiritShieldCapacity,
            $SpiritShieldDeadline,
            $CombatRuntime)

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
        -Name 'formation product host is covered by broad full and attribute evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenFormationProductHost.cpp') `
        -Logs @($Full, $Attributes)

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
        -Name 'formation influence modifier evaluator is covered by broad full evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenFormationInfluenceModifierEvaluator.cpp') `
        -Logs @($Full)

    Invoke-ExpectedPass `
        -Name 'combat tags include modifier consumer coverage under broad evidence' `
        -Paths @('Source/ShanmenCombatCore/Private/ShanmenCombatTags.cpp') `
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
        -Name 'formation influence consumer projection maps lease evidence to legacy attributes' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenFormationInfluenceConsumerProjection.cpp') `
        -Logs @($Full, $Attributes)

    Invoke-ExpectedPass `
        -Name 'formation consumer World resolution is covered by broad full and attribute evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenFormationInfluenceConsumerWorldResolution.cpp') `
        -Logs @($Full, $Attributes)

    Invoke-ExpectedPass `
        -Name 'formation consumer Run composition is covered by broad full and attribute evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenFormationInfluenceConsumerRunComposition.cpp') `
        -Logs @($Full, $Attributes)

    Invoke-ExpectedPass `
        -Name 'formation influence consumer registry is covered by broad full and attribute evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenFormationInfluenceConsumerRegistry.cpp') `
        -Logs @($Full, $Attributes)

    Invoke-ExpectedPass `
        -Name 'formation influence consumer attribute adapter is covered by broad full and attribute evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenFormationInfluenceConsumerAttributeAdapter.cpp') `
        -Logs @($Full, $Attributes)

    Invoke-ExpectedPass `
        -Name 'formation influence consumer application coordinator is covered by broad full and attribute evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenFormationInfluenceConsumerApplicationCoordinator.cpp') `
        -Logs @($Full, $Attributes)

    Invoke-ExpectedPass `
        -Name 'formation influence consumer command host is covered by broad full and attribute evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenFormationInfluenceConsumerCommandHost.cpp') `
        -Logs @($Full, $Attributes)

    Invoke-ExpectedPass `
        -Name 'formation influence consumer product bridge is covered by broad full and attribute evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenFormationInfluenceConsumerProductBridge.cpp') `
        -Logs @($Full, $Attributes)

    Invoke-ExpectedPass `
        -Name 'formation influence consumer product runtime is covered by broad full and attribute evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenFormationInfluenceConsumerProductRuntime.cpp') `
        -Logs @($Full, $Attributes)

    Invoke-ExpectedPass `
        -Name 'exact attribute mutation seam requires broad product and focused attribute evidence' `
        -Paths @('Source/demo_map/demo_mapAttributeComponent.cpp') `
        -Logs @($Full, $Attributes, $V3Items, $ItemUseAndArmor)

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
        -Name 'formation influence lifecycle coordinator is covered by broad full and attribute evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenFormationInfluenceLifecycleCoordinator.cpp') `
        -Logs @($Full, $Attributes)

    Invoke-ExpectedPass `
        -Name 'formation influence lifecycle command router is covered by broad full and attribute evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenFormationInfluenceLifecycleCommandRouter.cpp') `
        -Logs @($Full, $Attributes)

    Invoke-ExpectedPass `
        -Name 'formation influence lifecycle command host is covered by broad full and attribute evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenFormationInfluenceLifecycleCommandHost.cpp') `
        -Logs @($Full, $Attributes)

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

    Invoke-ExpectedPass `
        -Name 'combat Run coordinator alias seam is covered by broad full and attribute evidence' `
        -Paths @('Source/demo_map/demo_mapCombatRunCoordinator.cpp') `
        -Logs @($Full, $Attributes)

    Invoke-ExpectedFail `
        -Name 'missing mapped group fails closed' `
        -Paths @('Source/demo_map/demo_mapSkillComponent.cpp') `
        -Logs @($Full) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'combat Run coordinator focus cannot replace registry formation and broad contracts' `
        -Paths @('Source/demo_map/demo_mapCombatRunCoordinator.cpp') `
        -Logs @(
            $Coordinator,
            $FormationInfluenceConsumerWorldResolution,
            $Attributes) `
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
        -Name 'spirit shield focus cannot replace CombatCore resolver evidence' `
        -Paths @(
            'Source/ShanmenCombatRuntime/Private/ShanmenSpiritShield.cpp') `
        -Logs @($SpiritShieldRuntime, $CombatRuntime) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'capacity focus cannot replace shield lifecycle and CombatCore evidence' `
        -Paths @(
            'Source/ShanmenCombatRuntime/Private/ShanmenSpiritShieldCapacityAuthority.cpp') `
        -Logs @($SpiritShieldCapacity, $CombatRuntime) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'deadline focus cannot replace shield lifecycle and capacity composition evidence' `
        -Paths @(
            'Source/ShanmenCombatRuntime/Private/ShanmenSpiritShieldDeadlineGate.cpp') `
        -Logs @($SpiritShieldDeadline) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'action resource focus cannot replace action lifecycle and broad runtime evidence' `
        -Paths @(
            'Source/ShanmenCombatRuntime/Private/ShanmenActionResourceAuthority.cpp') `
        -Logs @($ActionResource) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'shield action focus cannot replace every composed authority contract' `
        -Paths @(
            'Source/ShanmenCombatRuntime/Private/ShanmenSpiritShieldActionCoordinator.cpp') `
        -Logs @($SpiritShieldAction, $ActionResource) `
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
        -Name 'modifier evaluator child evidence cannot replace policy and tag contracts' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenFormationInfluenceModifierEvaluator.cpp') `
        -Logs @($FormationInfluenceModifierEvaluator) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'evaluation binding child evidence cannot replace evaluator and execution contracts' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenFormationInfluenceEvaluationBinding.cpp') `
        -Logs @($FormationInfluenceEvaluationBinding) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'consumer projection child evidence cannot replace lease evaluator attribute and tag contracts' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenFormationInfluenceConsumerProjection.cpp') `
        -Logs @($FormationInfluenceConsumerProjection) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'consumer World resolution child evidence cannot replace registry Host and attribute contracts' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenFormationInfluenceConsumerWorldResolution.cpp') `
        -Logs @($FormationInfluenceConsumerWorldResolution) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'consumer Run composition focus cannot replace Run Host World and attribute contracts' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenFormationInfluenceConsumerRunComposition.cpp') `
        -Logs @($FormationInfluenceConsumerRunComposition) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'consumer registry child evidence cannot replace projection evaluator attribute and tag contracts' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenFormationInfluenceConsumerRegistry.cpp') `
        -Logs @($FormationInfluenceConsumerRegistry) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'consumer attribute adapter child evidence cannot replace registry attribute and broad product contracts' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenFormationInfluenceConsumerAttributeAdapter.cpp') `
        -Logs @($FormationInfluenceConsumerAttributeAdapter, $Attributes) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'consumer application coordinator child evidence cannot replace registry adapter and broad contracts' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenFormationInfluenceConsumerApplicationCoordinator.cpp') `
        -Logs @(
            $FormationInfluenceConsumerApplicationCoordinator,
            $FormationInfluenceConsumerAttributeAdapter,
            $Attributes) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'consumer command host focused evidence cannot replace registry projection and broad contracts' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenFormationInfluenceConsumerCommandHost.cpp') `
        -Logs @(
            $FormationInfluenceConsumerCommandHost,
            $FormationInfluenceConsumerApplicationCoordinator,
            $FormationInfluenceConsumerAttributeAdapter,
            $Attributes) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'consumer product bridge focused evidence cannot replace ProductHost command chain and broad contracts' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenFormationInfluenceConsumerProductBridge.cpp') `
        -Logs @(
            $FormationInfluenceConsumerProductBridge,
            $FormationHost,
            $FormationInfluenceConsumerCommandHost,
            $Attributes) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'consumer product runtime focused evidence cannot replace ProductHost bridge command chain and broad contracts' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenFormationInfluenceConsumerProductRuntime.cpp') `
        -Logs @(
            $FormationInfluenceConsumerProductRuntime,
            $FormationInfluenceConsumerProductBridge,
            $FormationHost,
            $Attributes) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'focused attributes alone cannot cover exact-handle product consumers' `
        -Paths @('Source/demo_map/demo_mapAttributeComponent.cpp') `
        -Logs @($Attributes) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'combat core evidence alone cannot cover tag consumers' `
        -Paths @('Source/ShanmenCombatCore/Public/ShanmenCombatTags.h') `
        -Logs @($CombatCore) `
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
        -Name 'formation lifecycle command router child evidence cannot replace coordinator Host and teardown contracts' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenFormationInfluenceLifecycleCommandRouter.cpp') `
        -Logs @($FormationInfluenceLifecycleCommandRouter) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'formation lifecycle command host child evidence cannot replace Router ProductHost and teardown contracts' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenFormationInfluenceLifecycleCommandHost.cpp') `
        -Logs @($FormationInfluenceLifecycleCommandHost) `
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
