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
        [switch]$UseUE58Completion,
        [switch]$OmitQueueEmpty
    )

    $Path = Join-Path $FixtureRoot $Name
    $Lines = [System.Collections.Generic.List[string]]::new()
    $Lines.Add("[2026.08.28-00.00.00:000][  0]Cmd: Automation RunTests $Group")
    $Lines.Add("[2026.08.28-00.00.00:001][  1]LogAutomationController: Display: Test Completed. Result={$Result} Name={Fixture} Path={$Group.Fixture}")
    if ($UseUE58Completion)
    {
        $Lines.Add('[2026.08.28-00.00.00:002][  2]LogAutomationCommandLine: Display: **** TEST COMPLETE. EXIT CODE: 0 ****')
    }
    elseif (-not $OmitQueueEmpty)
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
    $FullSystemLoop = New-AutomationLogFixture `
        -Name 'full-system-loop.log' `
        -Group 'demo_map.FullSystemLoop'
    $SearchContainer = New-AutomationLogFixture `
        -Name 'search-container.log' `
        -Group 'demo_map.SearchContainer'
    $FullSystemRegistry = New-AutomationLogFixture `
        -Name 'full-system-registry.log' `
        -Group 'demo_map.FullSystemLoop.41'
    $FullSystemRestore = New-AutomationLogFixture `
        -Name 'full-system-restore.log' `
        -Group 'demo_map.FullSystemLoop.47'
    $P7Integration = New-AutomationLogFixture `
        -Name 'p7-integration.log' `
        -Group 'demo_map.P7Integration'
    $P5RuntimeInterface = New-AutomationLogFixture `
        -Name 'p5-runtime-interface.log' `
        -Group 'demo_map.P5RuntimeInterface.06'
    $InputRestore = New-AutomationLogFixture `
        -Name 'input-restore.log' `
        -Group 'demo_map.InputRestore.32'
    $Coordinator = New-AutomationLogFixture `
        -Name 'coordinator.log' `
        -Group 'Shanmen.0_0_10.Product.CombatRunCoordinator'
    $PlayerActionArbitration = New-AutomationLogFixture `
        -Name 'player-action-arbitration.log' `
        -Group 'Shanmen.0_0_10.Product.PlayerActionArbitration'
    $PlayerVitality = New-AutomationLogFixture `
        -Name 'player-vitality.log' `
        -Group 'Shanmen.0_0_10.Product.PlayerVitality'
    $ItemUse = New-AutomationLogFixture `
        -Name 'item-use.log' `
        -Group 'demo_map.ItemUseAndArmor'
    $SpatialRingSchema = New-AutomationLogFixture `
        -Name 'spatial-ring-schema.log' `
        -Group 'demo_map.P1R6.SpatialRing'
    $ThrownRuntime = New-AutomationLogFixture `
        -Name 'thrown-runtime.log' `
        -Group 'Shanmen.0_0_10.CombatRuntime.ThrownWeapon'
    $CombatRuntime = New-AutomationLogFixture `
        -Name 'combat-runtime.log' `
        -Group 'Shanmen.0_0_10.CombatRuntime'
    $SwordRhythm = New-AutomationLogFixture `
        -Name 'sword-rhythm.log' `
        -Group 'Shanmen.0_0_10.CombatRuntime.SwordRhythm'
    $SwordRhythmContribution = New-AutomationLogFixture `
        -Name 'sword-rhythm-contribution.log' `
        -Group 'Shanmen.0_0_10.CombatRuntime.SwordRhythmContribution'
    $SwordRhythmContributionBinding = New-AutomationLogFixture `
        -Name 'sword-rhythm-contribution-binding.log' `
        -Group 'Shanmen.0_0_10.CombatRuntime.SwordRhythmContributionBinding'
    $SwordRhythmEvaluation = New-AutomationLogFixture `
        -Name 'sword-rhythm-evaluation.log' `
        -Group 'Shanmen.0_0_10.CombatRuntime.SwordRhythmEvaluation'
    $BasicSword = New-AutomationLogFixture `
        -Name 'basic-sword.log' `
        -Group 'Shanmen.0_0_10.CombatRuntime.BasicSword'
    $ActionResource = New-AutomationLogFixture `
        -Name 'action-resource.log' `
        -Group 'Shanmen.0_0_10.CombatRuntime.ActionResource'
    $ActionLifecycle = New-AutomationLogFixture `
        -Name 'action-lifecycle.log' `
        -Group 'Shanmen.0_0_10.CombatRuntime.ActionLifecycle'
    $SpiritShieldAction = New-AutomationLogFixture `
        -Name 'spirit-shield-action.log' `
        -Group 'Shanmen.0_0_10.CombatRuntime.SpiritShieldAction'
    $SpiritShieldSession = New-AutomationLogFixture `
        -Name 'spirit-shield-session.log' `
        -Group 'Shanmen.0_0_10.CombatRuntime.SpiritShieldSession'
    $SpiritEvasion = New-AutomationLogFixture `
        -Name 'spirit-evasion.log' `
        -Group 'Shanmen.0_0_10.CombatRuntime.SpiritEvasion'
    $WeaponGuard = New-AutomationLogFixture `
        -Name 'weapon-guard.log' `
        -Group 'Shanmen.0_0_10.CombatRuntime.WeaponGuard'
    $WeaponPerfectGuard = New-AutomationLogFixture `
        -Name 'weapon-perfect-guard.log' `
        -Group 'Shanmen.0_0_10.CombatRuntime.WeaponPerfectGuard'
    $WeaponGuardArc = New-AutomationLogFixture `
        -Name 'weapon-guard-arc.log' `
        -Group 'Shanmen.0_0_10.CombatRuntime.WeaponGuardArc'
    $WeaponGuardWorldAdapter = New-AutomationLogFixture `
        -Name 'weapon-guard-world-adapter.log' `
        -Group 'Shanmen.0_0_10.Product.WeaponGuardWorldAdapter'
    $WeaponGuardDefenseCoordinator = New-AutomationLogFixture `
        -Name 'weapon-guard-defense-coordinator.log' `
        -Group 'Shanmen.0_0_10.Product.WeaponGuardDefenseCoordinator'
    $WeaponGuardProductHost = New-AutomationLogFixture `
        -Name 'weapon-guard-product-host.log' `
        -Group 'Shanmen.0_0_10.Product.WeaponGuardProductHost'
    $WeaponGuardProductAuthority = New-AutomationLogFixture `
        -Name 'weapon-guard-product-authority.log' `
        -Group 'Shanmen.0_0_10.Product.WeaponGuardProductAuthority'
    $WeaponGuardItemAdapter = New-AutomationLogFixture `
        -Name 'weapon-guard-item-adapter.log' `
        -Group 'Shanmen.0_0_10.Product.WeaponGuardItemAdapter'
    $WeaponGuardProductRoute = New-AutomationLogFixture `
        -Name 'weapon-guard-product-route.log' `
        -Group 'Shanmen.0_0_10.Product.WeaponGuardProductRoute'
    $WeaponGuardInputAdapter = New-AutomationLogFixture `
        -Name 'weapon-guard-input-adapter.log' `
        -Group 'Shanmen.0_0_10.Product.WeaponGuardInputAdapter'
    $CombatRunFixedTimeline = New-AutomationLogFixture `
        -Name 'combat-run-fixed-timeline.log' `
        -Group 'Shanmen.0_0_10.Product.CombatRunFixedTimeline'
    $CombatCondition = New-AutomationLogFixture `
        -Name 'combat-condition.log' `
        -Group 'Shanmen.0_0_10.Product.CombatCondition.MeridianShock'
    $CombatConditionStatus = New-AutomationLogFixture `
        -Name 'combat-condition-status.log' `
        -Group 'Shanmen.0_0_10.Product.CombatCondition.MeridianShock.Status'
    $CombatConditionPresentationEvent = New-AutomationLogFixture `
        -Name 'combat-condition-presentation-event.log' `
        -Group 'Shanmen.0_0_10.Product.CombatCondition.MeridianShock.PresentationEvent'
    $CombatConditionPresentationViewState = New-AutomationLogFixture `
        -Name 'combat-condition-presentation-view-state.log' `
        -Group 'Shanmen.0_0_10.Product.CombatCondition.MeridianShock.PresentationViewState'
    $SwordRhythmProductHost = New-AutomationLogFixture `
        -Name 'sword-rhythm-product-host.log' `
        -Group 'Shanmen.0_0_10.Product.SwordRhythmProductHost'
    $SwordRhythmProductSession = New-AutomationLogFixture `
        -Name 'sword-rhythm-product-session.log' `
        -Group 'Shanmen.0_0_10.Product.SwordRhythmProductSession'
    $SwordRhythmEvaluationRoute = New-AutomationLogFixture `
        -Name 'sword-rhythm-evaluation-route.log' `
        -Group 'Shanmen.0_0_10.Product.SwordRhythmEvaluationRoute'
    $SwordRhythmEffectCue = New-AutomationLogFixture `
        -Name 'sword-rhythm-effect-cue.log' `
        -Group 'Shanmen.0_0_10.Product.SwordRhythmEffectCue'
    $SwordRhythmEffectCueDelivery = New-AutomationLogFixture `
        -Name 'sword-rhythm-effect-cue-delivery.log' `
        -Group 'Shanmen.0_0_10.Product.SwordRhythmEffectCueDelivery'
    $SwordRhythmEffectCueConsumerAttempt = New-AutomationLogFixture `
        -Name 'sword-rhythm-effect-cue-consumer-attempt.log' `
        -Group 'Shanmen.0_0_10.Product.SwordRhythmEffectCueConsumerAttempt'
    $SwordRhythmEffectCueExecutorAdapter = New-AutomationLogFixture `
        -Name 'sword-rhythm-effect-cue-executor-adapter.log' `
        -Group 'Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutorAdapter'
    $SwordRhythmEffectCuePresentationHandoffExecutor = New-AutomationLogFixture `
        -Name 'sword-rhythm-effect-cue-presentation-handoff-executor.log' `
        -Group 'Shanmen.0_0_10.Product.SwordRhythmEffectCuePresentationHandoffExecutor'
    $SwordRhythmEffectCuePresentationAcknowledgement = New-AutomationLogFixture `
        -Name 'sword-rhythm-effect-cue-presentation-acknowledgement.log' `
        -Group 'Shanmen.0_0_10.Product.SwordRhythmEffectCuePresentationAcknowledgement'
    $SwordRhythmEffectCuePresentationRunController = New-AutomationLogFixture `
        -Name 'sword-rhythm-effect-cue-presentation-run-controller.log' `
        -Group 'Shanmen.0_0_10.Product.SwordRhythmEffectCuePresentationRunController'
    $SwordRhythmEffectCueExecutionDriver = New-AutomationLogFixture `
        -Name 'sword-rhythm-effect-cue-execution-driver.log' `
        -Group 'Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionDriver'
    $SwordRhythmEffectCueExecutionHost = New-AutomationLogFixture `
        -Name 'sword-rhythm-effect-cue-execution-host.log' `
        -Group 'Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionHost'
    $SwordRhythmEffectCueExecutionCommandRouter = New-AutomationLogFixture `
        -Name 'sword-rhythm-effect-cue-execution-command-router.log' `
        -Group 'Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionCommandRouter'
    $SwordRhythmEffectCueExecutionCommandHost = New-AutomationLogFixture `
        -Name 'sword-rhythm-effect-cue-execution-command-host.log' `
        -Group 'Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionCommandHost'
    $SwordRhythmEffectCueExecutionProductRoute = New-AutomationLogFixture `
        -Name 'sword-rhythm-effect-cue-execution-product-route.log' `
        -Group 'Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductRoute'
    $SwordRhythmEffectCueExecutionProductTransaction = New-AutomationLogFixture `
        -Name 'sword-rhythm-effect-cue-execution-product-transaction.log' `
        -Group 'Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductTransaction'
    $SwordRhythmEffectCueExecutionProductDispatch = New-AutomationLogFixture `
        -Name 'sword-rhythm-effect-cue-execution-product-dispatch.log' `
        -Group 'Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductDispatch'
    $SwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpointEnvelopeManifestCodec = New-AutomationLogFixture `
        -Name 'sword-rhythm-effect-cue-execution-product-retry-step-command-journal-checkpoint-envelope-manifest-codec.log' `
        -Group 'Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpointEnvelopeManifestCodec'
    $SwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpointEnvelope = New-AutomationLogFixture `
        -Name 'sword-rhythm-effect-cue-execution-product-retry-step-command-journal-checkpoint-envelope.log' `
        -Group 'Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpointEnvelope'
    $SwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpoint = New-AutomationLogFixture `
        -Name 'sword-rhythm-effect-cue-execution-product-retry-step-command-journal-checkpoint.log' `
        -Group 'Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpoint'
    $SwordRhythmEffectCueExecutionProductRetryStepCommandJournal = New-AutomationLogFixture `
        -Name 'sword-rhythm-effect-cue-execution-product-retry-step-command-journal.log' `
        -Group 'Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductRetryStepCommandJournal'
    $SwordRhythmEffectCueExecutionProductRetryStepCommand = New-AutomationLogFixture `
        -Name 'sword-rhythm-effect-cue-execution-product-retry-step-command.log' `
        -Group 'Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductRetryStepCommand'
    $SwordRhythmEffectCueExecutionProductRetryStep = New-AutomationLogFixture `
        -Name 'sword-rhythm-effect-cue-execution-product-retry-step.log' `
        -Group 'Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductRetryStep'
    $SwordRhythmEffectCueExecutionProductRetryDecision = New-AutomationLogFixture `
        -Name 'sword-rhythm-effect-cue-execution-product-retry-decision.log' `
        -Group 'Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductRetryDecision'
    $SwordRhythmEffectCueExecutionProductPreparedRetry = New-AutomationLogFixture `
        -Name 'sword-rhythm-effect-cue-execution-product-prepared-retry.log' `
        -Group 'Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductPreparedRetry'
    $SwordRhythmEffectCueExecutionProductPreparedDispatch = New-AutomationLogFixture `
        -Name 'sword-rhythm-effect-cue-execution-product-prepared-dispatch.log' `
        -Group 'Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductPreparedDispatch'
    $SwordRhythmEffectCueExecutionProductPlannedDispatch = New-AutomationLogFixture `
        -Name 'sword-rhythm-effect-cue-execution-product-planned-dispatch.log' `
        -Group 'Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductPlannedDispatch'
    $SwordRhythmEffectCueExecutionProductDispatchPlan = New-AutomationLogFixture `
        -Name 'sword-rhythm-effect-cue-execution-product-dispatch-plan.log' `
        -Group 'Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductDispatchPlan'
    $SwordRhythmEffectCueExecutionSession = New-AutomationLogFixture `
        -Name 'sword-rhythm-effect-cue-execution-session.log' `
        -Group 'Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionSession'
    $SwordRhythmPresentation = New-AutomationLogFixture `
        -Name 'sword-rhythm-presentation.log' `
        -Group 'Shanmen.0_0_10.Product.SwordRhythmPresentation'
    $SwordRhythmPresentationEvent = New-AutomationLogFixture `
        -Name 'sword-rhythm-presentation-event.log' `
        -Group 'Shanmen.0_0_10.Product.SwordRhythmPresentationEvent'
    $WeaponGuardProductSession = New-AutomationLogFixture `
        -Name 'weapon-guard-product-session.log' `
        -Group 'Shanmen.0_0_10.Product.WeaponGuardProductSession'
    $WeaponGuardImpactRoute = New-AutomationLogFixture `
        -Name 'weapon-guard-impact-route.log' `
        -Group 'Shanmen.0_0_10.Product.WeaponGuardImpactRoute'
    $SkillProjectileDamageRoute = New-AutomationLogFixture `
        -Name 'skill-projectile-damage-route.log' `
        -Group 'Shanmen.0_0_10.Product.SkillProjectileDamageRoute'
    $SpiritEvasionMovement = New-AutomationLogFixture `
        -Name 'spirit-evasion-movement.log' `
        -Group 'Shanmen.0_0_10.CombatRuntime.SpiritEvasionMovement'
    $SpiritEvasionMovementProduct = New-AutomationLogFixture `
        -Name 'spirit-evasion-movement-product.log' `
        -Group 'Shanmen.0_0_10.Product.SpiritEvasionMovementAdapter'
    $SpiritEvasionMotionRuntime = New-AutomationLogFixture `
        -Name 'spirit-evasion-motion-runtime.log' `
        -Group 'Shanmen.0_0_10.Product.SpiritEvasionMotionRuntime'
    $SpiritEvasionActionCoordinator = New-AutomationLogFixture `
        -Name 'spirit-evasion-action-coordinator.log' `
        -Group 'Shanmen.0_0_10.Product.SpiritEvasionActionCoordinator'
    $SpiritEvasionProductHost = New-AutomationLogFixture `
        -Name 'spirit-evasion-product-host.log' `
        -Group 'Shanmen.0_0_10.Product.SpiritEvasionProductHost'
    $SpiritEvasionComponent = New-AutomationLogFixture `
        -Name 'spirit-evasion-component.log' `
        -Group 'Shanmen.0_0_10.Product.SpiritEvasionComponent'
    $SpiritEvasionCommandRouter = New-AutomationLogFixture `
        -Name 'spirit-evasion-command-router.log' `
        -Group 'Shanmen.0_0_10.Product.SpiritEvasionCommandRouter'
    $SpiritEvasionProductAuthority = New-AutomationLogFixture `
        -Name 'spirit-evasion-product-authority.log' `
        -Group 'Shanmen.0_0_10.Product.SpiritEvasionProductAuthority'
    $SpiritEvasionProductRoute = New-AutomationLogFixture `
        -Name 'spirit-evasion-product-route.log' `
        -Group 'Shanmen.0_0_10.Product.SpiritEvasionProductRoute'
    $SpiritEvasionInputAdapter = New-AutomationLogFixture `
        -Name 'spirit-evasion-input-adapter.log' `
        -Group 'Shanmen.0_0_10.Product.SpiritEvasionInputAdapter'
    $SpiritEvasionPhysicalInput = New-AutomationLogFixture `
        -Name 'spirit-evasion-physical-input.log' `
        -Group 'Shanmen.0_0_10.Product.SpiritEvasionPhysicalInput'
    $WorldGameplay = New-AutomationLogFixture `
        -Name 'world-gameplay.log' `
        -Group 'Shanmen.0_0_10.WorldGameplay'
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
    $AutomationRootBoundary = New-AutomationLogFixture `
        -Name 'automation-root-boundary.log' `
        -Group 'demo_map.AutomationRootBoundary'
    $RewardJackpot = New-AutomationLogFixture `
        -Name 'reward-jackpot.log' `
        -Group 'demo_map.RewardJackpot'
    $RewardAffix = New-AutomationLogFixture `
        -Name 'reward-affix.log' `
        -Group 'demo_map.RewardAffix'
    $RewardBossSource = New-AutomationLogFixture `
        -Name 'reward-boss-source.log' `
        -Group 'demo_map.RewardBossSource'
    $RareExtremeValue = New-AutomationLogFixture `
        -Name 'rare-extreme-value.log' `
        -Group 'demo_map.RareExtremeValue'
    $RewardSourceProjection = New-AutomationLogFixture `
        -Name 'reward-source-projection.log' `
        -Group 'demo_map.RewardSourceProjection'
    $FailedRanged = New-AutomationLogFixture `
        -Name 'ranged-fail.log' `
        -Group 'demo_map.V2RangedCompatibility' `
        -Result Fail
    $NoQueue = New-AutomationLogFixture `
        -Name 'no-queue.log' `
        -Group 'demo_map.V2RangedCompatibility' `
        -OmitQueueEmpty
    $UE58Complete = New-AutomationLogFixture `
        -Name 'ue58-complete.log' `
        -Group 'Shanmen.0_0_10' `
        -UseUE58Completion

    Invoke-ExpectedPass `
        -Name 'overlapping rules union and broad suite coverage' `
        -Paths @(
            'Source\ShanmenCombatCore\Private\Resolver.cpp',
            'Source/demo_map/demo_mapSkillProjectile.cpp') `
        -Logs @($Full, $SkillProjectileDamageRoute, $Enemy, $Ranged)

    Invoke-ExpectedPass `
        -Name 'skill projectile damage route requires focus full enemy and ranged evidence' `
        -Paths @(
            'Source/demo_map/demo_mapSkillProjectile.h',
            'Source/demo_map/demo_mapSkillProjectile.cpp',
            'Source/demo_map/demo_mapSkillProjectileDamageRouteTests.cpp') `
        -Logs @($SkillProjectileDamageRoute, $Full, $Enemy, $Ranged)

    Invoke-ExpectedPass `
        -Name 'docs and scripts require no product log' `
        -Paths @(
            'Docs/Report/example.md',
            'Scripts/example.ps1')

    Invoke-ExpectedPass `
        -Name 'UE 5.8 native TEST COMPLETE marker is healthy evidence' `
        -Paths @('Docs/Process/P_PHASE_BASELINE.md') `
        -Logs @($UE58Complete)

    Invoke-ExpectedPass `
        -Name 'formation deployment core is covered by broad full evidence' `
        -Paths @(
            'Source/ShanmenCombatRuntime/Private/ShanmenFormationDeployment.cpp') `
        -Logs @($Full)

    Invoke-ExpectedPass `
        -Name 'sword rhythm requires rhythm BasicSword lifecycle and broad runtime evidence' `
        -Paths @(
            'Source/ShanmenCombatRuntime/Public/ShanmenSwordRhythm.h',
            'Source/ShanmenCombatRuntime/Private/ShanmenSwordRhythm.cpp',
            'Source/ShanmenCombatRuntime/Private/Tests/ShanmenSwordRhythmTests.cpp') `
        -Logs @($SwordRhythm, $BasicSword, $ActionLifecycle, $CombatRuntime)

    Invoke-ExpectedPass `
        -Name 'sword rhythm contribution requires every source contract and broad runtime evidence' `
        -Paths @(
            'Source/ShanmenCombatRuntime/Public/ShanmenSwordRhythmContribution.h',
            'Source/ShanmenCombatRuntime/Private/ShanmenSwordRhythmContribution.cpp',
            'Source/ShanmenCombatRuntime/Private/Tests/ShanmenSwordRhythmContributionTests.cpp') `
        -Logs @(
            $SwordRhythmContribution,
            $SwordRhythm,
            $WeaponPerfectGuard,
            $SpiritEvasion,
            $ActionLifecycle,
            $CombatRuntime)

    Invoke-ExpectedPass `
        -Name 'sword rhythm contribution binding requires source rhythm action and broad runtime evidence' `
        -Paths @(
            'Source/ShanmenCombatRuntime/Public/ShanmenSwordRhythmContributionBinding.h',
            'Source/ShanmenCombatRuntime/Private/ShanmenSwordRhythmContributionBinding.cpp',
            'Source/ShanmenCombatRuntime/Private/Tests/ShanmenSwordRhythmContributionBindingTests.cpp') `
        -Logs @(
            $SwordRhythmContributionBinding,
            $SwordRhythmContribution,
            $SwordRhythm,
            $BasicSword,
            $ActionLifecycle,
            $CombatRuntime)

    Invoke-ExpectedPass `
        -Name 'sword rhythm evaluator requires focused product binding source rhythm and action evidence' `
        -Paths @(
            'Source/ShanmenCombatRuntime/Public/ShanmenSwordRhythmEvaluation.h',
            'Source/ShanmenCombatRuntime/Private/ShanmenSwordRhythmEvaluation.cpp',
            'Source/ShanmenCombatRuntime/Private/Tests/ShanmenSwordRhythmEvaluationTests.cpp') `
        -Logs @(
            $SwordRhythmEvaluation,
            $SwordRhythmProductSession,
            $SwordRhythmContributionBinding,
            $SwordRhythmContribution,
            $SwordRhythm,
            $BasicSword,
            $ActionLifecycle,
            $CombatRuntime)

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
        -Name 'spirit shield session requires every owned authority contract' `
        -Paths @(
            'Source/ShanmenCombatRuntime/Public/ShanmenSpiritShieldSession.h',
            'Source/ShanmenCombatRuntime/Private/ShanmenSpiritShieldSession.cpp',
            'Source/ShanmenCombatRuntime/Private/Tests/ShanmenSpiritShieldSessionTests.cpp') `
        -Logs @(
            $SpiritShieldSession,
            $SpiritShieldAction,
            $ActionResource,
            $ActionLifecycle,
            $SpiritShieldRuntime,
            $SpiritShieldCapacity,
            $SpiritShieldDeadline,
            $CombatRuntime,
            $CombatCore)

    Invoke-ExpectedPass `
        -Name 'spirit evasion requires action lifecycle and resolver evidence' `
        -Paths @(
            'Source/ShanmenCombatRuntime/Public/ShanmenSpiritEvasion.h',
            'Source/ShanmenCombatRuntime/Private/ShanmenSpiritEvasion.cpp',
            'Source/ShanmenCombatRuntime/Private/Tests/ShanmenSpiritEvasionTests.cpp') `
        -Logs @(
            $SpiritEvasion,
            $ActionLifecycle,
            $CombatRuntime,
            $CombatCore)

    Invoke-ExpectedPass `
        -Name 'weapon guard requires action lifecycle and resolver evidence' `
        -Paths @(
            'Source/ShanmenCombatRuntime/Public/ShanmenWeaponGuard.h',
            'Source/ShanmenCombatRuntime/Private/ShanmenWeaponGuard.cpp',
            'Source/ShanmenCombatRuntime/Private/Tests/ShanmenWeaponGuardTests.cpp') `
        -Logs @(
            $WeaponGuard,
            $ActionLifecycle,
            $CombatRuntime,
            $CombatCore)

    Invoke-ExpectedPass `
        -Name 'perfect guard timing requires ordinary window lifecycle and resolver evidence' `
        -Paths @(
            'Source/ShanmenCombatRuntime/Public/ShanmenWeaponPerfectGuard.h',
            'Source/ShanmenCombatRuntime/Private/ShanmenWeaponPerfectGuard.cpp',
            'Source/ShanmenCombatRuntime/Private/Tests/ShanmenWeaponPerfectGuardTests.cpp') `
        -Logs @(
            $WeaponPerfectGuard,
            $WeaponGuard,
            $ActionLifecycle,
            $CombatRuntime,
            $CombatCore)

    Invoke-ExpectedPass `
        -Name 'weapon guard arc requires timing ordinary lifecycle and resolver evidence' `
        -Paths @(
            'Source/ShanmenCombatRuntime/Public/ShanmenWeaponGuardArc.h',
            'Source/ShanmenCombatRuntime/Private/ShanmenWeaponGuardArc.cpp',
            'Source/ShanmenCombatRuntime/Private/Tests/ShanmenWeaponGuardArcTests.cpp') `
        -Logs @(
            $WeaponGuardArc,
            $WeaponPerfectGuard,
            $WeaponGuard,
            $ActionLifecycle,
            $CombatRuntime,
            $CombatCore)

    Invoke-ExpectedPass `
        -Name 'weapon guard World adapter requires full world arc timing and resolver evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenWeaponGuardWorldAdapter.h',
            'Source/demo_map/demo_mapShanmenWeaponGuardWorldAdapter.cpp',
            'Source/demo_map/demo_mapShanmenWeaponGuardWorldAdapterTests.cpp') `
        -Logs @(
            $Full,
            $WeaponGuardWorldAdapter,
            $WeaponGuardArc,
            $WeaponPerfectGuard,
            $WeaponGuard,
            $ActionLifecycle,
            $CombatCore,
            $WorldGameplay)

    Invoke-ExpectedPass `
        -Name 'weapon guard defense coordinator requires full composition world arc timing and resolver evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenWeaponGuardDefenseCoordinator.h',
            'Source/demo_map/demo_mapShanmenWeaponGuardDefenseCoordinator.cpp',
            'Source/demo_map/demo_mapShanmenWeaponGuardDefenseCoordinatorTests.cpp') `
        -Logs @(
            $Full,
            $WeaponGuardDefenseCoordinator,
            $WeaponGuardWorldAdapter,
            $WeaponGuardArc,
            $WeaponPerfectGuard,
            $WeaponGuard,
            $ActionLifecycle,
            $CombatCore,
            $WorldGameplay)

    Invoke-ExpectedPass `
        -Name 'weapon guard product host requires lifecycle composition world arc timing and resolver evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenWeaponGuardProductHost.h',
            'Source/demo_map/demo_mapShanmenWeaponGuardProductHost.cpp',
            'Source/demo_map/demo_mapShanmenWeaponGuardProductHostTests.cpp') `
        -Logs @(
            $Full,
            $WeaponGuardProductHost,
            $WeaponGuardDefenseCoordinator,
            $WeaponGuardWorldAdapter,
            $WeaponGuardArc,
            $WeaponPerfectGuard,
            $WeaponGuard,
            $ActionLifecycle,
            $CombatCore,
            $WorldGameplay)

    Invoke-ExpectedPass `
        -Name 'weapon guard product authority requires config reservation host and Run evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenWeaponGuardProductAuthority.h',
            'Source/demo_map/demo_mapShanmenWeaponGuardProductAuthority.cpp',
            'Source/demo_map/demo_mapShanmenWeaponGuardProductAuthorityTests.cpp') `
        -Logs @(
            $Full,
            $WeaponGuardProductAuthority,
            $WeaponGuardProductHost,
            $WeaponGuardDefenseCoordinator,
            $WeaponGuardWorldAdapter,
            $WeaponGuardArc,
            $WeaponPerfectGuard,
            $WeaponGuard,
            $ActionLifecycle,
            $CombatCore,
            $Coordinator,
            $FormationInfluenceConsumerWorldResolution,
            $WorldGameplay,
            $Attributes)

    Invoke-ExpectedPass `
        -Name 'weapon guard item adapter requires explicit item and product authority evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenWeaponGuardItemAdapter.h',
            'Source/demo_map/demo_mapShanmenWeaponGuardItemAdapter.cpp',
            'Source/demo_map/demo_mapShanmenWeaponGuardItemAdapterTests.cpp') `
        -Logs @(
            $Full,
            $WeaponGuardItemAdapter,
            $WeaponGuardProductAuthority,
            $Legacy)

    Invoke-ExpectedPass `
        -Name 'weapon guard product route requires item authorization product host and Run evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenWeaponGuardProductRoute.h',
            'Source/demo_map/demo_mapShanmenWeaponGuardProductRoute.cpp',
            'Source/demo_map/demo_mapShanmenWeaponGuardProductRouteTests.cpp') `
        -Logs @(
            $Full,
            $WeaponGuardProductRoute,
            $WeaponGuardItemAdapter,
            $WeaponGuardProductAuthority,
            $WeaponGuardProductHost,
            $Coordinator,
            $Legacy)

    Invoke-ExpectedPass `
        -Name 'weapon guard input adapter requires product route item Run and legacy evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenWeaponGuardInputAdapter.h',
            'Source/demo_map/demo_mapShanmenWeaponGuardInputAdapter.cpp',
            'Source/demo_map/demo_mapShanmenWeaponGuardInputAdapterTests.cpp') `
        -Logs @(
            $Full,
            $WeaponGuardInputAdapter,
            $WeaponGuardProductRoute,
            $WeaponGuardItemAdapter,
            $WeaponGuardProductAuthority,
            $WeaponGuardProductHost,
            $Coordinator,
            $Legacy)

    Invoke-ExpectedPass `
        -Name 'weapon guard product session requires route item Host Run and legacy evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenWeaponGuardProductSession.h',
            'Source/demo_map/demo_mapShanmenWeaponGuardProductSession.cpp',
            'Source/demo_map/demo_mapShanmenWeaponGuardProductSessionTests.cpp') `
        -Logs @(
            $Full,
            $WeaponGuardProductSession,
            $WeaponGuardProductRoute,
            $WeaponGuardItemAdapter,
            $WeaponGuardProductAuthority,
            $WeaponGuardProductHost,
            $Coordinator,
            $Legacy)

    Invoke-ExpectedPass `
        -Name 'weapon guard Impact route requires Session World defense Run item and enemy evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenWeaponGuardImpactRouteTests.cpp') `
        -Logs @(
            $Full,
            $WeaponGuardImpactRoute,
            $Legacy)

    Invoke-ExpectedPass `
        -Name 'combat Run fixed timeline requires all Run-clock consumers' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenCombatRunFixedTimeline.h',
            'Source/demo_map/demo_mapShanmenCombatRunFixedTimeline.cpp',
            'Source/demo_map/demo_mapShanmenCombatRunFixedTimelineTests.cpp',
            'Source/demo_map/demo_mapShanmenWeaponGuardFixedTimeline.h',
            'Source/demo_map/demo_mapShanmenWeaponGuardFixedTimeline.cpp',
            'Source/demo_map/demo_mapShanmenWeaponGuardFixedTimelineTests.cpp') `
        -Logs @(
            $Full,
            $CombatRunFixedTimeline,
            $WeaponGuardInputAdapter,
            $WeaponGuardProductSession,
            $WeaponGuardProductRoute,
            $Coordinator,
            $SwordRhythmProductHost,
            $SwordRhythmProductSession,
            $SwordRhythmPresentation,
            $SwordRhythmPresentationEvent)

    Invoke-ExpectedPass `
        -Name 'combat condition requires committed vitality timeline attribute and full evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenCombatConditionComponent.h',
            'Source/demo_map/demo_mapShanmenCombatConditionComponent.cpp',
            'Source/demo_map/demo_mapShanmenCombatConditionComponentTests.cpp') `
        -Logs @(
            $Full,
            $CombatCondition,
            $CombatConditionStatus,
            $CombatConditionPresentationEvent,
            $CombatConditionPresentationViewState,
            $PlayerVitality,
            $CombatRunFixedTimeline,
            $Attributes)

    Invoke-ExpectedPass `
        -Name 'combat condition status requires source timeline attribute and full evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenCombatConditionStatus.h',
            'Source/demo_map/demo_mapShanmenCombatConditionStatus.cpp') `
        -Logs @(
            $Full,
            $CombatCondition,
            $CombatConditionStatus,
            $CombatConditionPresentationEvent,
            $CombatConditionPresentationViewState,
            $CombatRunFixedTimeline,
            $Attributes)

    Invoke-ExpectedPass `
        -Name 'combat condition presentation event requires source status timeline attribute and full evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenCombatConditionPresentationEvent.h',
            'Source/demo_map/demo_mapShanmenCombatConditionPresentationEvent.cpp') `
        -Logs @(
            $Full,
            $CombatCondition,
            $CombatConditionStatus,
            $CombatConditionPresentationEvent,
            $CombatConditionPresentationViewState,
            $CombatRunFixedTimeline,
            $Attributes)

    Invoke-ExpectedPass `
        -Name 'combat condition presentation view requires source event status timeline attribute and full evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenCombatConditionPresentationViewState.h',
            'Source/demo_map/demo_mapShanmenCombatConditionPresentationViewState.cpp') `
        -Logs @(
            $Full,
            $CombatCondition,
            $CombatConditionStatus,
            $CombatConditionPresentationEvent,
            $CombatConditionPresentationViewState,
            $CombatRunFixedTimeline,
            $Attributes)

    Invoke-ExpectedPass `
        -Name 'sword rhythm product Host requires real action timeline and pure runtime evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenSwordRhythmProductHost.h',
            'Source/demo_map/demo_mapShanmenSwordRhythmProductHost.cpp',
            'Source/demo_map/demo_mapShanmenSwordRhythmProductHostTests.cpp') `
        -Logs @(
            $Full,
            $SwordRhythmProductHost,
            $CombatRunFixedTimeline,
            $Coordinator,
            $SwordRhythm,
            $BasicSword,
            $ActionLifecycle)

    Invoke-ExpectedPass `
        -Name 'sword rhythm product Session requires evaluator presentation source Host timeline and runtime evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenSwordRhythmProductSession.h',
            'Source/demo_map/demo_mapShanmenSwordRhythmProductSession.cpp',
            'Source/demo_map/demo_mapShanmenSwordRhythmProductSessionTests.cpp') `
        -Logs @(
            $Full,
            $SwordRhythmProductSession,
            $SwordRhythmEvaluationRoute,
            $SwordRhythmEffectCue,
            $SwordRhythmPresentation,
            $SwordRhythmPresentationEvent,
            $SwordRhythmProductHost,
            $CombatRunFixedTimeline,
            $Coordinator,
            $SwordRhythmEvaluation,
            $SwordRhythmContributionBinding,
            $SwordRhythmContribution,
            $SwordRhythm,
            $WeaponPerfectGuard,
            $SpiritEvasion,
            $BasicSword,
            $ActionLifecycle)

    Invoke-ExpectedPass `
        -Name 'sword rhythm effect cues require product policy read model and runtime evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCue.h',
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCue.cpp',
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueTests.cpp') `
        -Logs @(
            $Full,
            $SwordRhythmEffectCue,
            $SwordRhythmPresentation,
            $SwordRhythmProductSession,
            $SwordRhythmEvaluationRoute,
            $SwordRhythmProductHost,
            $CombatRunFixedTimeline,
            $Coordinator,
            $SwordRhythmEvaluation,
            $SwordRhythm,
            $BasicSword,
            $ActionLifecycle)

    Invoke-ExpectedPass `
        -Name 'sword rhythm cue delivery requires consumer cursor source product and runtime evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueDelivery.h',
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueDelivery.cpp',
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueDeliveryTests.cpp') `
        -Logs @(
            $Full,
            $SwordRhythmEffectCueDelivery,
            $SwordRhythmEffectCue,
            $SwordRhythmPresentation,
            $SwordRhythmProductSession,
            $SwordRhythmEvaluationRoute,
            $SwordRhythmProductHost,
            $CombatRunFixedTimeline,
            $Coordinator,
            $SwordRhythmEvaluation,
            $SwordRhythm,
            $BasicSword,
            $ActionLifecycle)

    Invoke-ExpectedPass `
        -Name 'sword rhythm cue consumer attempts require delivery source product and runtime evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueConsumerAttempt.h',
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueConsumerAttempt.cpp',
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueConsumerAttemptTests.cpp') `
        -Logs @(
            $Full,
            $SwordRhythmEffectCueConsumerAttempt,
            $SwordRhythmEffectCueDelivery,
            $SwordRhythmEffectCue,
            $SwordRhythmPresentation,
            $SwordRhythmProductSession,
            $SwordRhythmEvaluationRoute,
            $SwordRhythmProductHost,
            $CombatRunFixedTimeline,
            $Coordinator,
            $SwordRhythmEvaluation,
            $SwordRhythm,
            $BasicSword,
            $ActionLifecycle)

    Invoke-ExpectedPass `
        -Name 'sword rhythm cue executor adapter requires attempt delivery source and runtime evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueExecutorAdapter.h',
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueExecutorAdapter.cpp',
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueExecutorAdapterTests.cpp') `
        -Logs @(
            $Full,
            $SwordRhythmEffectCueExecutorAdapter,
            $SwordRhythmEffectCueConsumerAttempt,
            $SwordRhythmEffectCueDelivery,
            $SwordRhythmEffectCue,
            $SwordRhythmPresentation,
            $SwordRhythmProductSession,
            $SwordRhythmEvaluationRoute,
            $SwordRhythmProductHost,
            $CombatRunFixedTimeline,
            $Coordinator,
            $SwordRhythmEvaluation,
            $SwordRhythm,
            $BasicSword,
            $ActionLifecycle)

    Invoke-ExpectedPass `
        -Name 'sword rhythm cue presentation handoff executor requires adapter delivery source and runtime evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCuePresentationHandoffExecutor.h',
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCuePresentationHandoffExecutor.cpp',
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCuePresentationHandoffExecutorTests.cpp') `
        -Logs @(
            $Full,
            $SwordRhythmEffectCuePresentationHandoffExecutor,
            $SwordRhythmEffectCueExecutorAdapter,
            $SwordRhythmEffectCueConsumerAttempt,
            $SwordRhythmEffectCueDelivery,
            $SwordRhythmEffectCue,
            $SwordRhythmPresentation,
            $SwordRhythmProductSession,
            $SwordRhythmEvaluationRoute,
            $SwordRhythmProductHost,
            $CombatRunFixedTimeline,
            $Coordinator,
            $SwordRhythmEvaluation,
            $SwordRhythm,
            $BasicSword,
            $ActionLifecycle)

    Invoke-ExpectedPass `
        -Name 'sword rhythm cue presentation acknowledgement requires Run handoff prepared-dispatch and runtime evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCuePresentationAcknowledgement.h',
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCuePresentationAcknowledgement.cpp',
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCuePresentationAcknowledgementTests.cpp') `
        -Logs @(
            $Full,
            $SwordRhythmEffectCuePresentationAcknowledgement,
            $SwordRhythmEffectCuePresentationRunController,
            $SwordRhythmEffectCuePresentationHandoffExecutor,
            $SwordRhythmEffectCueExecutionProductPreparedDispatch,
            $SwordRhythmEffectCueExecutionProductDispatchPlan,
            $SwordRhythmEffectCueExecutionProductDispatch,
            $SwordRhythmEffectCueExecutionProductTransaction,
            $SwordRhythmEffectCueExecutionProductRoute,
            $SwordRhythmEffectCueExecutionCommandHost,
            $SwordRhythmEffectCueExecutionCommandRouter,
            $SwordRhythmEffectCueExecutionSession,
            $SwordRhythmEffectCueExecutionHost,
            $SwordRhythmEffectCueExecutionDriver,
            $SwordRhythmEffectCueExecutorAdapter,
            $SwordRhythmEffectCueConsumerAttempt,
            $SwordRhythmEffectCueDelivery,
            $SwordRhythmEffectCue,
            $SwordRhythmPresentation,
            $SwordRhythmProductSession,
            $SwordRhythmEvaluationRoute,
            $SwordRhythmProductHost,
            $CombatRunFixedTimeline,
            $Coordinator,
            $SwordRhythmEvaluation,
            $SwordRhythm,
            $BasicSword,
            $ActionLifecycle)

    Invoke-ExpectedPass `
        -Name 'sword rhythm cue presentation Run controller requires handoff prepared-dispatch and runtime evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCuePresentationRunController.h',
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCuePresentationRunController.cpp',
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCuePresentationRunControllerTests.cpp') `
        -Logs @(
            $Full,
            $SwordRhythmEffectCuePresentationRunController,
            $SwordRhythmEffectCuePresentationHandoffExecutor,
            $SwordRhythmEffectCueExecutionProductPreparedDispatch,
            $SwordRhythmEffectCueExecutionProductDispatchPlan,
            $SwordRhythmEffectCueExecutionProductDispatch,
            $SwordRhythmEffectCueExecutionProductTransaction,
            $SwordRhythmEffectCueExecutionProductRoute,
            $SwordRhythmEffectCueExecutionCommandHost,
            $SwordRhythmEffectCueExecutionCommandRouter,
            $SwordRhythmEffectCueExecutionSession,
            $SwordRhythmEffectCueExecutionHost,
            $SwordRhythmEffectCueExecutionDriver,
            $SwordRhythmEffectCueExecutorAdapter,
            $SwordRhythmEffectCueConsumerAttempt,
            $SwordRhythmEffectCueDelivery,
            $SwordRhythmEffectCue,
            $SwordRhythmPresentation,
            $SwordRhythmProductSession,
            $SwordRhythmEvaluationRoute,
            $SwordRhythmProductHost,
            $CombatRunFixedTimeline,
            $Coordinator,
            $SwordRhythmEvaluation,
            $SwordRhythm,
            $BasicSword,
            $ActionLifecycle)

    Invoke-ExpectedPass `
        -Name 'sword rhythm cue execution driver requires executor attempt delivery source and runtime evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueExecutionDriver.h',
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueExecutionDriver.cpp',
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueExecutionDriverTests.cpp') `
        -Logs @(
            $Full,
            $SwordRhythmEffectCueExecutionDriver,
            $SwordRhythmEffectCueExecutorAdapter,
            $SwordRhythmEffectCueConsumerAttempt,
            $SwordRhythmEffectCueDelivery,
            $SwordRhythmEffectCue,
            $SwordRhythmPresentation,
            $SwordRhythmProductSession,
            $SwordRhythmEvaluationRoute,
            $SwordRhythmProductHost,
            $CombatRunFixedTimeline,
            $Coordinator,
            $SwordRhythmEvaluation,
            $SwordRhythm,
            $BasicSword,
            $ActionLifecycle)

    Invoke-ExpectedPass `
        -Name 'sword rhythm cue execution host requires dual consumer driver source and runtime evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueExecutionHost.h',
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueExecutionHost.cpp',
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueExecutionHostTests.cpp') `
        -Logs @(
            $Full,
            $SwordRhythmEffectCueExecutionHost,
            $SwordRhythmEffectCueExecutionDriver,
            $SwordRhythmEffectCueExecutorAdapter,
            $SwordRhythmEffectCueConsumerAttempt,
            $SwordRhythmEffectCueDelivery,
            $SwordRhythmEffectCue,
            $SwordRhythmPresentation,
            $SwordRhythmProductSession,
            $SwordRhythmEvaluationRoute,
            $SwordRhythmProductHost,
            $CombatRunFixedTimeline,
            $Coordinator,
            $SwordRhythmEvaluation,
            $SwordRhythm,
            $BasicSword,
            $ActionLifecycle)

    Invoke-ExpectedPass `
        -Name 'sword rhythm cue command Router requires Session Host driver source and runtime evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueExecutionCommandRouter.h',
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueExecutionCommandRouter.cpp',
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueExecutionCommandRouterTests.cpp') `
        -Logs @(
            $Full,
            $SwordRhythmEffectCueExecutionCommandRouter,
            $SwordRhythmEffectCueExecutionSession,
            $SwordRhythmEffectCueExecutionHost,
            $SwordRhythmEffectCueExecutionDriver,
            $SwordRhythmEffectCueExecutorAdapter,
            $SwordRhythmEffectCueConsumerAttempt,
            $SwordRhythmEffectCueDelivery,
            $SwordRhythmEffectCue,
            $SwordRhythmPresentation,
            $SwordRhythmProductSession,
            $SwordRhythmEvaluationRoute,
            $SwordRhythmProductHost,
            $CombatRunFixedTimeline,
            $Coordinator,
            $SwordRhythmEvaluation,
            $SwordRhythm,
            $BasicSword,
            $ActionLifecycle)

    Invoke-ExpectedPass `
        -Name 'sword rhythm cue execution session requires host driver source and runtime evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueExecutionSession.h',
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueExecutionSession.cpp',
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueExecutionSessionTests.cpp') `
        -Logs @(
            $Full,
            $SwordRhythmEffectCueExecutionSession,
            $SwordRhythmEffectCueExecutionHost,
            $SwordRhythmEffectCueExecutionDriver,
            $SwordRhythmEffectCueExecutorAdapter,
            $SwordRhythmEffectCueConsumerAttempt,
            $SwordRhythmEffectCueDelivery,
            $SwordRhythmEffectCue,
            $SwordRhythmPresentation,
            $SwordRhythmProductSession,
            $SwordRhythmEvaluationRoute,
            $SwordRhythmProductHost,
            $CombatRunFixedTimeline,
            $Coordinator,
            $SwordRhythmEvaluation,
            $SwordRhythm,
            $BasicSword,
            $ActionLifecycle)

    Invoke-ExpectedPass `
        -Name 'sword rhythm cue retry envelope manifest codec requires complete codec-to-runtime evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpointEnvelopeManifestCodec.h',
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpointEnvelopeManifestCodec.cpp',
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpointEnvelopeManifestCodecTests.cpp') `
        -Logs @(
            $Full,
            $SwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpointEnvelopeManifestCodec,
            $SwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpointEnvelope,
            $SwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpoint,
            $SwordRhythmEffectCueExecutionProductRetryStepCommandJournal,
            $SwordRhythmEffectCueExecutionProductRetryStepCommand,
            $SwordRhythmEffectCueExecutionProductRetryStep,
            $SwordRhythmEffectCueExecutionProductRetryDecision,
            $SwordRhythmEffectCueExecutionProductPreparedRetry,
            $SwordRhythmEffectCueExecutionProductPreparedDispatch,
            $SwordRhythmEffectCueExecutionProductPlannedDispatch,
            $SwordRhythmEffectCueExecutionProductDispatchPlan,
            $SwordRhythmEffectCueExecutionProductDispatch,
            $SwordRhythmEffectCueExecutionProductTransaction,
            $SwordRhythmEffectCueExecutionProductRoute,
            $SwordRhythmEffectCueExecutionCommandHost,
            $SwordRhythmEffectCueExecutionCommandRouter,
            $SwordRhythmEffectCueExecutionSession,
            $SwordRhythmEffectCueExecutionHost,
            $SwordRhythmEffectCueExecutionDriver,
            $SwordRhythmEffectCueExecutorAdapter,
            $SwordRhythmEffectCueConsumerAttempt,
            $SwordRhythmEffectCueDelivery,
            $SwordRhythmEffectCue,
            $SwordRhythmPresentation,
            $SwordRhythmProductSession,
            $SwordRhythmEvaluationRoute,
            $SwordRhythmProductHost,
            $CombatRunFixedTimeline,
            $Coordinator,
            $SwordRhythmEvaluation,
            $SwordRhythm,
            $BasicSword,
            $ActionLifecycle)

    Invoke-ExpectedPass `
        -Name 'sword rhythm cue retry checkpoint envelope requires complete envelope-to-runtime evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpointEnvelope.h',
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpointEnvelope.cpp',
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpointEnvelopeTests.cpp') `
        -Logs @(
            $Full,
            $SwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpointEnvelope,
            $SwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpoint,
            $SwordRhythmEffectCueExecutionProductRetryStepCommandJournal,
            $SwordRhythmEffectCueExecutionProductRetryStepCommand,
            $SwordRhythmEffectCueExecutionProductRetryStep,
            $SwordRhythmEffectCueExecutionProductRetryDecision,
            $SwordRhythmEffectCueExecutionProductPreparedRetry,
            $SwordRhythmEffectCueExecutionProductPreparedDispatch,
            $SwordRhythmEffectCueExecutionProductPlannedDispatch,
            $SwordRhythmEffectCueExecutionProductDispatchPlan,
            $SwordRhythmEffectCueExecutionProductDispatch,
            $SwordRhythmEffectCueExecutionProductTransaction,
            $SwordRhythmEffectCueExecutionProductRoute,
            $SwordRhythmEffectCueExecutionCommandHost,
            $SwordRhythmEffectCueExecutionCommandRouter,
            $SwordRhythmEffectCueExecutionSession,
            $SwordRhythmEffectCueExecutionHost,
            $SwordRhythmEffectCueExecutionDriver,
            $SwordRhythmEffectCueExecutorAdapter,
            $SwordRhythmEffectCueConsumerAttempt,
            $SwordRhythmEffectCueDelivery,
            $SwordRhythmEffectCue,
            $SwordRhythmPresentation,
            $SwordRhythmProductSession,
            $SwordRhythmEvaluationRoute,
            $SwordRhythmProductHost,
            $CombatRunFixedTimeline,
            $Coordinator,
            $SwordRhythmEvaluation,
            $SwordRhythm,
            $BasicSword,
            $ActionLifecycle)

    Invoke-ExpectedPass `
        -Name 'sword rhythm cue retry journal checkpoint requires complete checkpoint-to-runtime evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpoint.h',
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpoint.cpp',
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpointTests.cpp') `
        -Logs @(
            $Full,
            $SwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpoint,
            $SwordRhythmEffectCueExecutionProductRetryStepCommandJournal,
            $SwordRhythmEffectCueExecutionProductRetryStepCommand,
            $SwordRhythmEffectCueExecutionProductRetryStep,
            $SwordRhythmEffectCueExecutionProductRetryDecision,
            $SwordRhythmEffectCueExecutionProductPreparedRetry,
            $SwordRhythmEffectCueExecutionProductPreparedDispatch,
            $SwordRhythmEffectCueExecutionProductPlannedDispatch,
            $SwordRhythmEffectCueExecutionProductDispatchPlan,
            $SwordRhythmEffectCueExecutionProductDispatch,
            $SwordRhythmEffectCueExecutionProductTransaction,
            $SwordRhythmEffectCueExecutionProductRoute,
            $SwordRhythmEffectCueExecutionCommandHost,
            $SwordRhythmEffectCueExecutionCommandRouter,
            $SwordRhythmEffectCueExecutionSession,
            $SwordRhythmEffectCueExecutionHost,
            $SwordRhythmEffectCueExecutionDriver,
            $SwordRhythmEffectCueExecutorAdapter,
            $SwordRhythmEffectCueConsumerAttempt,
            $SwordRhythmEffectCueDelivery,
            $SwordRhythmEffectCue,
            $SwordRhythmPresentation,
            $SwordRhythmProductSession,
            $SwordRhythmEvaluationRoute,
            $SwordRhythmProductHost,
            $CombatRunFixedTimeline,
            $Coordinator,
            $SwordRhythmEvaluation,
            $SwordRhythm,
            $BasicSword,
            $ActionLifecycle)

    Invoke-ExpectedPass `
        -Name 'sword rhythm cue product retry step command journal requires complete journal-to-runtime evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournal.h',
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournal.cpp',
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalTests.cpp') `
        -Logs @(
            $Full,
            $SwordRhythmEffectCueExecutionProductRetryStepCommandJournal,
            $SwordRhythmEffectCueExecutionProductRetryStepCommand,
            $SwordRhythmEffectCueExecutionProductRetryStep,
            $SwordRhythmEffectCueExecutionProductRetryDecision,
            $SwordRhythmEffectCueExecutionProductPreparedRetry,
            $SwordRhythmEffectCueExecutionProductPreparedDispatch,
            $SwordRhythmEffectCueExecutionProductPlannedDispatch,
            $SwordRhythmEffectCueExecutionProductDispatchPlan,
            $SwordRhythmEffectCueExecutionProductDispatch,
            $SwordRhythmEffectCueExecutionProductTransaction,
            $SwordRhythmEffectCueExecutionProductRoute,
            $SwordRhythmEffectCueExecutionCommandHost,
            $SwordRhythmEffectCueExecutionCommandRouter,
            $SwordRhythmEffectCueExecutionSession,
            $SwordRhythmEffectCueExecutionHost,
            $SwordRhythmEffectCueExecutionDriver,
            $SwordRhythmEffectCueExecutorAdapter,
            $SwordRhythmEffectCueConsumerAttempt,
            $SwordRhythmEffectCueDelivery,
            $SwordRhythmEffectCue,
            $SwordRhythmPresentation,
            $SwordRhythmProductSession,
            $SwordRhythmEvaluationRoute,
            $SwordRhythmProductHost,
            $CombatRunFixedTimeline,
            $Coordinator,
            $SwordRhythmEvaluation,
            $SwordRhythm,
            $BasicSword,
            $ActionLifecycle)

    Invoke-ExpectedPass `
        -Name 'sword rhythm cue product retry step command requires complete command-to-runtime evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommand.h',
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommand.cpp',
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandTests.cpp') `
        -Logs @(
            $Full,
            $SwordRhythmEffectCueExecutionProductRetryStepCommand,
            $SwordRhythmEffectCueExecutionProductRetryStep,
            $SwordRhythmEffectCueExecutionProductRetryDecision,
            $SwordRhythmEffectCueExecutionProductPreparedRetry,
            $SwordRhythmEffectCueExecutionProductPreparedDispatch,
            $SwordRhythmEffectCueExecutionProductPlannedDispatch,
            $SwordRhythmEffectCueExecutionProductDispatchPlan,
            $SwordRhythmEffectCueExecutionProductDispatch,
            $SwordRhythmEffectCueExecutionProductTransaction,
            $SwordRhythmEffectCueExecutionProductRoute,
            $SwordRhythmEffectCueExecutionCommandHost,
            $SwordRhythmEffectCueExecutionCommandRouter,
            $SwordRhythmEffectCueExecutionSession,
            $SwordRhythmEffectCueExecutionHost,
            $SwordRhythmEffectCueExecutionDriver,
            $SwordRhythmEffectCueExecutorAdapter,
            $SwordRhythmEffectCueConsumerAttempt,
            $SwordRhythmEffectCueDelivery,
            $SwordRhythmEffectCue,
            $SwordRhythmPresentation,
            $SwordRhythmProductSession,
            $SwordRhythmEvaluationRoute,
            $SwordRhythmProductHost,
            $CombatRunFixedTimeline,
            $Coordinator,
            $SwordRhythmEvaluation,
            $SwordRhythm,
            $BasicSword,
            $ActionLifecycle)

    Invoke-ExpectedPass `
        -Name 'sword rhythm cue product retry step requires complete step-to-runtime evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStep.h',
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStep.cpp',
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepTests.cpp') `
        -Logs @(
            $Full,
            $SwordRhythmEffectCueExecutionProductRetryStep,
            $SwordRhythmEffectCueExecutionProductRetryDecision,
            $SwordRhythmEffectCueExecutionProductPreparedRetry,
            $SwordRhythmEffectCueExecutionProductPreparedDispatch,
            $SwordRhythmEffectCueExecutionProductPlannedDispatch,
            $SwordRhythmEffectCueExecutionProductDispatchPlan,
            $SwordRhythmEffectCueExecutionProductDispatch,
            $SwordRhythmEffectCueExecutionProductTransaction,
            $SwordRhythmEffectCueExecutionProductRoute,
            $SwordRhythmEffectCueExecutionCommandHost,
            $SwordRhythmEffectCueExecutionCommandRouter,
            $SwordRhythmEffectCueExecutionSession,
            $SwordRhythmEffectCueExecutionHost,
            $SwordRhythmEffectCueExecutionDriver,
            $SwordRhythmEffectCueExecutorAdapter,
            $SwordRhythmEffectCueConsumerAttempt,
            $SwordRhythmEffectCueDelivery,
            $SwordRhythmEffectCue,
            $SwordRhythmPresentation,
            $SwordRhythmProductSession,
            $SwordRhythmEvaluationRoute,
            $SwordRhythmProductHost,
            $CombatRunFixedTimeline,
            $Coordinator,
            $SwordRhythmEvaluation,
            $SwordRhythm,
            $BasicSword,
            $ActionLifecycle)

    Invoke-ExpectedPass `
        -Name 'sword rhythm cue product retry decision requires complete decision-to-runtime evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueExecutionProductRetryDecision.h',
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueExecutionProductRetryDecision.cpp',
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueExecutionProductRetryDecisionTests.cpp') `
        -Logs @(
            $Full,
            $SwordRhythmEffectCueExecutionProductRetryDecision,
            $SwordRhythmEffectCueExecutionProductPreparedRetry,
            $SwordRhythmEffectCueExecutionProductPreparedDispatch,
            $SwordRhythmEffectCueExecutionProductPlannedDispatch,
            $SwordRhythmEffectCueExecutionProductDispatchPlan,
            $SwordRhythmEffectCueExecutionProductDispatch,
            $SwordRhythmEffectCueExecutionProductTransaction,
            $SwordRhythmEffectCueExecutionProductRoute,
            $SwordRhythmEffectCueExecutionCommandHost,
            $SwordRhythmEffectCueExecutionCommandRouter,
            $SwordRhythmEffectCueExecutionSession,
            $SwordRhythmEffectCueExecutionHost,
            $SwordRhythmEffectCueExecutionDriver,
            $SwordRhythmEffectCueExecutorAdapter,
            $SwordRhythmEffectCueConsumerAttempt,
            $SwordRhythmEffectCueDelivery,
            $SwordRhythmEffectCue,
            $SwordRhythmPresentation,
            $SwordRhythmProductSession,
            $SwordRhythmEvaluationRoute,
            $SwordRhythmProductHost,
            $CombatRunFixedTimeline,
            $Coordinator,
            $SwordRhythmEvaluation,
            $SwordRhythm,
            $BasicSword,
            $ActionLifecycle)

    Invoke-ExpectedPass `
        -Name 'sword rhythm cue product prepared retry requires complete continuation-to-runtime evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetry.h',
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetry.cpp',
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryTests.cpp') `
        -Logs @(
            $Full,
            $SwordRhythmEffectCueExecutionProductPreparedRetry,
            $SwordRhythmEffectCueExecutionProductPreparedDispatch,
            $SwordRhythmEffectCueExecutionProductPlannedDispatch,
            $SwordRhythmEffectCueExecutionProductDispatchPlan,
            $SwordRhythmEffectCueExecutionProductDispatch,
            $SwordRhythmEffectCueExecutionProductTransaction,
            $SwordRhythmEffectCueExecutionProductRoute,
            $SwordRhythmEffectCueExecutionCommandHost,
            $SwordRhythmEffectCueExecutionCommandRouter,
            $SwordRhythmEffectCueExecutionSession,
            $SwordRhythmEffectCueExecutionHost,
            $SwordRhythmEffectCueExecutionDriver,
            $SwordRhythmEffectCueExecutorAdapter,
            $SwordRhythmEffectCueConsumerAttempt,
            $SwordRhythmEffectCueDelivery,
            $SwordRhythmEffectCue,
            $SwordRhythmPresentation,
            $SwordRhythmProductSession,
            $SwordRhythmEvaluationRoute,
            $SwordRhythmProductHost,
            $CombatRunFixedTimeline,
            $Coordinator,
            $SwordRhythmEvaluation,
            $SwordRhythm,
            $BasicSword,
            $ActionLifecycle)

    Invoke-ExpectedPass `
        -Name 'sword rhythm cue product prepared dispatch requires complete frozen-to-runtime evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatch.h',
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatch.cpp',
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchTests.cpp') `
        -Logs @(
            $Full,
            $SwordRhythmEffectCueExecutionProductPreparedDispatch,
            $SwordRhythmEffectCueExecutionProductPlannedDispatch,
            $SwordRhythmEffectCueExecutionProductDispatchPlan,
            $SwordRhythmEffectCueExecutionProductDispatch,
            $SwordRhythmEffectCueExecutionProductTransaction,
            $SwordRhythmEffectCueExecutionProductRoute,
            $SwordRhythmEffectCueExecutionCommandHost,
            $SwordRhythmEffectCueExecutionCommandRouter,
            $SwordRhythmEffectCueExecutionSession,
            $SwordRhythmEffectCueExecutionHost,
            $SwordRhythmEffectCueExecutionDriver,
            $SwordRhythmEffectCueExecutorAdapter,
            $SwordRhythmEffectCueConsumerAttempt,
            $SwordRhythmEffectCueDelivery,
            $SwordRhythmEffectCue,
            $SwordRhythmPresentation,
            $SwordRhythmProductSession,
            $SwordRhythmEvaluationRoute,
            $SwordRhythmProductHost,
            $CombatRunFixedTimeline,
            $Coordinator,
            $SwordRhythmEvaluation,
            $SwordRhythm,
            $BasicSword,
            $ActionLifecycle)

    Invoke-ExpectedPass `
        -Name 'sword rhythm cue product planned dispatch requires complete plan-to-runtime evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueExecutionProductPlannedDispatch.h',
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueExecutionProductPlannedDispatch.cpp',
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueExecutionProductPlannedDispatchTests.cpp') `
        -Logs @(
            $Full,
            $SwordRhythmEffectCueExecutionProductPlannedDispatch,
            $SwordRhythmEffectCueExecutionProductPreparedDispatch,
            $SwordRhythmEffectCueExecutionProductDispatchPlan,
            $SwordRhythmEffectCueExecutionProductDispatch,
            $SwordRhythmEffectCueExecutionProductTransaction,
            $SwordRhythmEffectCueExecutionProductRoute,
            $SwordRhythmEffectCueExecutionCommandHost,
            $SwordRhythmEffectCueExecutionCommandRouter,
            $SwordRhythmEffectCueExecutionSession,
            $SwordRhythmEffectCueExecutionHost,
            $SwordRhythmEffectCueExecutionDriver,
            $SwordRhythmEffectCueExecutorAdapter,
            $SwordRhythmEffectCueConsumerAttempt,
            $SwordRhythmEffectCueDelivery,
            $SwordRhythmEffectCue,
            $SwordRhythmPresentation,
            $SwordRhythmProductSession,
            $SwordRhythmEvaluationRoute,
            $SwordRhythmProductHost,
            $CombatRunFixedTimeline,
            $Coordinator,
            $SwordRhythmEvaluation,
            $SwordRhythm,
            $BasicSword,
            $ActionLifecycle)

    Invoke-ExpectedPass `
        -Name 'sword rhythm cue product dispatch plan requires complete deterministic-to-runtime evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchPlan.h',
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchPlan.cpp',
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchPlanTests.cpp') `
        -Logs @(
            $Full,
            $SwordRhythmEffectCueExecutionProductPreparedDispatch,
            $SwordRhythmEffectCueExecutionProductPlannedDispatch,
            $SwordRhythmEffectCueExecutionProductDispatchPlan,
            $SwordRhythmEffectCueExecutionProductDispatch,
            $SwordRhythmEffectCueExecutionProductTransaction,
            $SwordRhythmEffectCueExecutionProductRoute,
            $SwordRhythmEffectCueExecutionCommandHost,
            $SwordRhythmEffectCueExecutionCommandRouter,
            $SwordRhythmEffectCueExecutionSession,
            $SwordRhythmEffectCueExecutionHost,
            $SwordRhythmEffectCueExecutionDriver,
            $SwordRhythmEffectCueExecutorAdapter,
            $SwordRhythmEffectCueConsumerAttempt,
            $SwordRhythmEffectCueDelivery,
            $SwordRhythmEffectCue,
            $SwordRhythmPresentation,
            $SwordRhythmProductSession,
            $SwordRhythmEvaluationRoute,
            $SwordRhythmProductHost,
            $CombatRunFixedTimeline,
            $Coordinator,
            $SwordRhythmEvaluation,
            $SwordRhythm,
            $BasicSword,
            $ActionLifecycle)

    Invoke-ExpectedPass `
        -Name 'sword rhythm cue product dispatch requires complete current-to-transaction evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueExecutionProductDispatch.h',
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueExecutionProductDispatch.cpp',
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchTests.cpp') `
        -Logs @(
            $Full,
            $SwordRhythmEffectCueExecutionProductPlannedDispatch,
            $SwordRhythmEffectCueExecutionProductPreparedDispatch,
            $SwordRhythmEffectCueExecutionProductDispatchPlan,
            $SwordRhythmEffectCueExecutionProductDispatch,
            $SwordRhythmEffectCueExecutionProductTransaction,
            $SwordRhythmEffectCueExecutionProductRoute,
            $SwordRhythmEffectCueExecutionCommandHost,
            $SwordRhythmEffectCueExecutionCommandRouter,
            $SwordRhythmEffectCueExecutionSession,
            $SwordRhythmEffectCueExecutionHost,
            $SwordRhythmEffectCueExecutionDriver,
            $SwordRhythmEffectCueExecutorAdapter,
            $SwordRhythmEffectCueConsumerAttempt,
            $SwordRhythmEffectCueDelivery,
            $SwordRhythmEffectCue,
            $SwordRhythmPresentation,
            $SwordRhythmProductSession,
            $SwordRhythmEvaluationRoute,
            $SwordRhythmProductHost,
            $CombatRunFixedTimeline,
            $Coordinator,
            $SwordRhythmEvaluation,
            $SwordRhythm,
            $BasicSword,
            $ActionLifecycle)

    Invoke-ExpectedPass `
        -Name 'sword rhythm cue product transaction requires complete bounded lifecycle evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueExecutionProductTransaction.h',
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueExecutionProductTransaction.cpp',
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionTests.cpp') `
        -Logs @(
            $Full,
            $SwordRhythmEffectCueExecutionProductTransaction,
            $SwordRhythmEffectCueExecutionProductRoute,
            $SwordRhythmEffectCueExecutionCommandHost,
            $SwordRhythmEffectCueExecutionCommandRouter,
            $SwordRhythmEffectCueExecutionSession,
            $SwordRhythmEffectCueExecutionHost,
            $SwordRhythmEffectCueExecutionDriver,
            $SwordRhythmEffectCueExecutorAdapter,
            $SwordRhythmEffectCueConsumerAttempt,
            $SwordRhythmEffectCueDelivery,
            $SwordRhythmEffectCue,
            $SwordRhythmPresentation,
            $SwordRhythmProductSession,
            $SwordRhythmEvaluationRoute,
            $SwordRhythmProductHost,
            $CombatRunFixedTimeline,
            $Coordinator,
            $SwordRhythmEvaluation,
            $SwordRhythm,
            $BasicSword,
            $ActionLifecycle)

    Invoke-ExpectedPass `
        -Name 'sword rhythm cue product route requires complete source-to-Host evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueExecutionProductRoute.h',
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueExecutionProductRoute.cpp',
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueExecutionProductRouteTests.cpp') `
        -Logs @(
            $Full,
            $SwordRhythmEffectCueExecutionProductRoute,
            $SwordRhythmEffectCueExecutionCommandHost,
            $SwordRhythmEffectCueExecutionCommandRouter,
            $SwordRhythmEffectCueExecutionSession,
            $SwordRhythmEffectCueExecutionHost,
            $SwordRhythmEffectCueExecutionDriver,
            $SwordRhythmEffectCueExecutorAdapter,
            $SwordRhythmEffectCueConsumerAttempt,
            $SwordRhythmEffectCueDelivery,
            $SwordRhythmEffectCue,
            $SwordRhythmPresentation,
            $SwordRhythmProductSession,
            $SwordRhythmEvaluationRoute,
            $SwordRhythmProductHost,
            $CombatRunFixedTimeline,
            $Coordinator,
            $SwordRhythmEvaluation,
            $SwordRhythm,
            $BasicSword,
            $ActionLifecycle)

    Invoke-ExpectedPass `
        -Name 'sword rhythm cue command Host requires Router Session Host driver source and runtime evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueExecutionCommandHost.h',
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueExecutionCommandHost.cpp',
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueExecutionCommandHostTests.cpp') `
        -Logs @(
            $Full,
            $SwordRhythmEffectCueExecutionCommandHost,
            $SwordRhythmEffectCueExecutionCommandRouter,
            $SwordRhythmEffectCueExecutionSession,
            $SwordRhythmEffectCueExecutionHost,
            $SwordRhythmEffectCueExecutionDriver,
            $SwordRhythmEffectCueExecutorAdapter,
            $SwordRhythmEffectCueConsumerAttempt,
            $SwordRhythmEffectCueDelivery,
            $SwordRhythmEffectCue,
            $SwordRhythmPresentation,
            $SwordRhythmProductSession,
            $SwordRhythmEvaluationRoute,
            $SwordRhythmProductHost,
            $CombatRunFixedTimeline,
            $Coordinator,
            $SwordRhythmEvaluation,
            $SwordRhythm,
            $BasicSword,
            $ActionLifecycle)

    Invoke-ExpectedPass `
        -Name 'sword rhythm presentation requires immutable evaluation Session and runtime evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenSwordRhythmPresentation.h',
            'Source/demo_map/demo_mapShanmenSwordRhythmPresentation.cpp',
            'Source/demo_map/demo_mapShanmenSwordRhythmPresentationTests.cpp') `
        -Logs @(
            $Full,
            $SwordRhythmPresentation,
            $SwordRhythmPresentationEvent,
            $SwordRhythmProductSession,
            $SwordRhythmEvaluationRoute,
            $SwordRhythmProductHost,
            $CombatRunFixedTimeline,
            $Coordinator,
            $SwordRhythmEvaluation,
            $SwordRhythm,
            $BasicSword,
            $ActionLifecycle)

    Invoke-ExpectedPass `
        -Name 'sword rhythm presentation event requires read model and runtime evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenSwordRhythmPresentationEvent.h',
            'Source/demo_map/demo_mapShanmenSwordRhythmPresentationEvent.cpp',
            'Source/demo_map/demo_mapShanmenSwordRhythmPresentationEventTests.cpp') `
        -Logs @(
            $Full,
            $SwordRhythmPresentationEvent,
            $SwordRhythmPresentation,
            $SwordRhythmProductSession,
            $SwordRhythmEvaluationRoute,
            $SwordRhythmProductHost,
            $CombatRunFixedTimeline,
            $Coordinator,
            $SwordRhythmEvaluation,
            $SwordRhythm,
            $BasicSword,
            $ActionLifecycle)

    Invoke-ExpectedPass `
        -Name 'spirit evasion movement requires window and lifecycle evidence' `
        -Paths @(
            'Source/ShanmenCombatRuntime/Public/ShanmenSpiritEvasionMovement.h',
            'Source/ShanmenCombatRuntime/Private/ShanmenSpiritEvasionMovement.cpp',
            'Source/ShanmenCombatRuntime/Private/Tests/ShanmenSpiritEvasionMovementTests.cpp') `
        -Logs @(
            $SpiritEvasionMovement,
            $SpiritEvasion,
            $ActionLifecycle,
            $CombatRuntime)

    Invoke-ExpectedPass `
        -Name 'spirit evasion product displacement maps adapter and legacy callers' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenSpiritEvasionMovementAdapter.h',
            'Source/demo_map/demo_mapShanmenSpiritEvasionMovementAdapter.cpp',
            'Source/demo_map/demo_mapShanmenSpiritEvasionMovementAdapterTests.cpp',
            'Source/demo_map/demo_mapCombatDisplacement.h',
            'Source/demo_map/demo_mapCombatDisplacement.cpp',
            'Source/demo_map/demo_mapEnemySkillRuntimeComponent.cpp') `
        -Logs @(
            $Full,
            $SpiritEvasionMovementProduct,
            $SpiritEvasionMovement,
            $Enemy,
            $Ranged)

    Invoke-ExpectedPass `
        -Name 'spirit evasion motion runtime maps plan scheduler and swept authority' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenSpiritEvasionMotionRuntime.h',
            'Source/demo_map/demo_mapShanmenSpiritEvasionMotionRuntime.cpp',
            'Source/demo_map/demo_mapShanmenSpiritEvasionMotionRuntimeTests.cpp') `
        -Logs @(
            $Full,
            $SpiritEvasionMotionRuntime,
            $SpiritEvasionMovementProduct,
            $SpiritEvasionMovement,
            $Enemy,
            $Ranged)

    Invoke-ExpectedPass `
        -Name 'spirit evasion action coordinator maps every owned lifecycle seam' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenSpiritEvasionActionCoordinator.h',
            'Source/demo_map/demo_mapShanmenSpiritEvasionActionCoordinator.cpp',
            'Source/demo_map/demo_mapShanmenSpiritEvasionActionCoordinatorTests.cpp') `
        -Logs @(
            $Full,
            $SpiritEvasionActionCoordinator,
            $SpiritEvasionMotionRuntime,
            $SpiritEvasionMovementProduct,
            $SpiritEvasionMovement,
            $SpiritEvasion,
            $ActionLifecycle,
            $Enemy,
            $Ranged)

    Invoke-ExpectedPass `
        -Name 'spirit evasion product host maps its complete owned chain' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenSpiritEvasionProductHost.h',
            'Source/demo_map/demo_mapShanmenSpiritEvasionProductHost.cpp',
            'Source/demo_map/demo_mapShanmenSpiritEvasionProductHostTests.cpp') `
        -Logs @(
            $Full,
            $SpiritEvasionProductHost,
            $SpiritEvasionActionCoordinator,
            $SpiritEvasionMotionRuntime,
            $SpiritEvasionMovementProduct,
            $SpiritEvasionMovement,
            $SpiritEvasion,
            $ActionLifecycle,
            $Enemy,
            $Ranged)

    Invoke-ExpectedPass `
        -Name 'spirit evasion component maps its host and complete owned chain' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenSpiritEvasionComponent.h',
            'Source/demo_map/demo_mapShanmenSpiritEvasionComponent.cpp',
            'Source/demo_map/demo_mapShanmenSpiritEvasionComponentTests.cpp') `
        -Logs @(
            $Full,
            $SpiritEvasionComponent,
            $SpiritEvasionProductHost,
            $SpiritEvasionActionCoordinator,
            $SpiritEvasionMotionRuntime,
            $SpiritEvasionMovementProduct,
            $SpiritEvasionMovement,
            $SpiritEvasion,
            $ActionLifecycle,
            $Enemy,
            $Ranged)

    Invoke-ExpectedPass `
        -Name 'spirit evasion command route maps install run and complete host chain' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenSpiritEvasionCommandRouter.h',
            'Source/demo_map/demo_mapShanmenSpiritEvasionCommandRouter.cpp',
            'Source/demo_map/demo_mapShanmenSpiritEvasionCommandRouterTests.cpp') `
        -Logs @(
            $Full,
            $SpiritEvasionCommandRouter,
            $SpiritEvasionComponent,
            $SpiritEvasionProductHost,
            $SpiritEvasionActionCoordinator,
            $SpiritEvasionMotionRuntime,
            $SpiritEvasionMovementProduct,
            $SpiritEvasionMovement,
            $SpiritEvasion,
            $ActionLifecycle,
            $Coordinator,
            $WorldGameplay,
            $Enemy,
            $Ranged)

    Invoke-ExpectedPass `
        -Name 'spirit evasion product authority maps reservation config and complete route chain' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenSpiritEvasionProductAuthority.h',
            'Source/demo_map/demo_mapShanmenSpiritEvasionProductAuthority.cpp',
            'Source/demo_map/demo_mapShanmenSpiritEvasionProductAuthorityTests.cpp') `
        -Logs @(
            $Full,
            $SpiritEvasionProductAuthority,
            $SpiritEvasionCommandRouter,
            $SpiritEvasionComponent,
            $SpiritEvasionProductHost,
            $SpiritEvasionActionCoordinator,
            $SpiritEvasionMotionRuntime,
            $SpiritEvasionMovementProduct,
            $SpiritEvasionMovement,
            $SpiritEvasion,
            $ActionLifecycle,
            $Coordinator,
            $FormationInfluenceConsumerWorldResolution,
            $WorldGameplay,
            $Attributes,
            $Enemy,
            $Ranged)

    Invoke-ExpectedPass `
        -Name 'spirit evasion product route maps intent reservation dispatch and complete chain' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenSpiritEvasionProductRoute.h',
            'Source/demo_map/demo_mapShanmenSpiritEvasionProductRoute.cpp',
            'Source/demo_map/demo_mapShanmenSpiritEvasionProductRouteTests.cpp') `
        -Logs @(
            $Full,
            $SpiritEvasionProductRoute,
            $SpiritEvasionProductAuthority,
            $SpiritEvasionCommandRouter,
            $SpiritEvasionComponent,
            $SpiritEvasionProductHost,
            $SpiritEvasionActionCoordinator,
            $SpiritEvasionMotionRuntime,
            $SpiritEvasionMovementProduct,
            $SpiritEvasionMovement,
            $SpiritEvasion,
            $ActionLifecycle,
            $Coordinator,
            $FormationInfluenceConsumerWorldResolution,
            $WorldGameplay,
            $Attributes,
            $Enemy,
            $Ranged)

    Invoke-ExpectedPass `
        -Name 'spirit evasion input adapter maps gameplay gate through the complete product route' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenSpiritEvasionInputAdapter.h',
            'Source/demo_map/demo_mapShanmenSpiritEvasionInputAdapter.cpp',
            'Source/demo_map/demo_mapShanmenSpiritEvasionInputAdapterTests.cpp') `
        -Logs @(
            $Full,
            $SpiritEvasionInputAdapter,
            $SpiritEvasionProductRoute,
            $SpiritEvasionProductAuthority,
            $SpiritEvasionCommandRouter,
            $SpiritEvasionComponent,
            $SpiritEvasionProductHost,
            $SpiritEvasionActionCoordinator,
            $SpiritEvasionMotionRuntime,
            $SpiritEvasionMovementProduct,
            $SpiritEvasionMovement,
            $SpiritEvasion,
            $ActionLifecycle,
            $Coordinator,
            $FormationInfluenceConsumerWorldResolution,
            $WorldGameplay,
            $Attributes,
            $Enemy,
            $Ranged)

    Invoke-ExpectedPass `
        -Name 'unified input changes require physical route and every touched legacy input contract' `
        -Paths @(
            'Source/demo_map/demo_mapInputActionRegistry.h',
            'Source/demo_map/demo_mapInputActionRegistry.cpp',
            'Source/demo_map/demo_mapInputBindingSettings.h',
            'Source/demo_map/demo_mapInputBindingSettings.cpp',
            'Source/demo_map/demo_mapShanmenSpiritEvasionPhysicalInputTests.cpp',
            'Source/demo_map/demo_mapFullSystemLoopTests.cpp',
            'Source/demo_map/demo_mapP7IntegrationTests.cpp',
            'Source/demo_map/demo_mapRuntimeInterfaceSliceTests.cpp',
            'Source/demo_map/demo_mapInputRestoreTests.cpp') `
        -Logs @(
            $Full,
            $SpiritEvasionPhysicalInput,
            $SpiritEvasionInputAdapter,
            $FullSystemLoop,
            $FullSystemRegistry,
            $FullSystemRestore,
            $P7Integration,
            $P5RuntimeInterface,
            $InputRestore,
            $Ranged)

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
        -Name 'spatial ring schema fixture requires its exact migration group' `
        -Paths @(
            'Source/demo_map/demo_mapP1R6SpatialRingTests.cpp') `
        -Logs @($SpatialRingSchema)

    Invoke-ExpectedPass `
        -Name 'legacy full-system fixture requires its own complete suite' `
        -Paths @(
            'Source/demo_map/demo_mapFullSystemLoopTests.cpp') `
        -Logs @(
            $Full,
            $FullSystemLoop,
            $P7Integration,
            $P5RuntimeInterface,
            $InputRestore,
            $Ranged)

    Invoke-ExpectedPass `
        -Name 'legacy search-container fixture requires its exact suite' `
        -Paths @(
            'Source/demo_map/demo_mapSearchContainerTests.cpp') `
        -Logs @($SearchContainer)

    Invoke-ExpectedPass `
        -Name 'automation root boundary changes require their exact safety suite' `
        -Paths @(
            'Source/demo_map/demo_mapAutomationRootBoundary.cpp',
            'Source/demo_map/demo_mapAutomationRootBoundaryTests.cpp') `
        -Logs @($AutomationRootBoundary)

    Invoke-ExpectedPass `
        -Name 'reward projection test support requires every dependent reward suite' `
        -Paths @(
            'Source/demo_map/demo_mapRewardProjectionTestSupport.h',
            'Source/demo_map/demo_mapRewardJackpotTests.cpp',
            'Source/demo_map/demo_mapRewardAffixTests.cpp',
            'Source/demo_map/demo_mapRewardBossSourceTests.cpp',
            'Source/demo_map/demo_mapRewardRareExtremeTests.cpp',
            'Source/demo_map/demo_mapRewardSourceProjectionTests.cpp') `
        -Logs @(
            $RewardJackpot,
            $RewardAffix,
            $RewardBossSource,
            $RareExtremeValue,
            $RewardSourceProjection)

    Invoke-ExpectedPass `
        -Name 'combat Run coordinator alias seam is covered by broad full and attribute evidence' `
        -Paths @('Source/demo_map/demo_mapCombatRunCoordinator.cpp') `
        -Logs @($Full, $Attributes, $Legacy)

    Invoke-ExpectedPass `
        -Name 'player action arbitration maps deterministic policy and broad integration evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenPlayerActionArbitration.cpp',
            'Source/demo_map/demo_mapShanmenPlayerActionArbitrationTests.cpp') `
        -Logs @($Full, $PlayerActionArbitration, $Coordinator)

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
        -Name 'player action arbitration focus cannot replace coordinator and broad integration evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenPlayerActionArbitration.cpp') `
        -Logs @($PlayerActionArbitration) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'failed test evidence is rejected' `
        -Paths @('Source/demo_map/demo_mapSkillProjectile.cpp') `
        -Logs @($Full, $SkillProjectileDamageRoute, $Enemy, $FailedRanged) `
        -ExpectedText 'unhealthy evidence'

    Invoke-ExpectedFail `
        -Name 'queue completion is required' `
        -Paths @('Source/demo_map/demo_mapSkillProjectile.cpp') `
        -Logs @($Full, $SkillProjectileDamageRoute, $Enemy, $NoQueue) `
        -ExpectedText 'terminal completion marker missing'

    Invoke-ExpectedFail `
        -Name 'skill projectile route focus cannot replace broad compatibility evidence' `
        -Paths @(
            'Source/demo_map/demo_mapSkillProjectileDamageRouteTests.cpp') `
        -Logs @($SkillProjectileDamageRoute) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'unknown production path is unmapped' `
        -Paths @('Source/demo_map/demo_mapUnknownAuthority.cpp') `
        -Logs @($Full) `
        -ExpectedText 'unmapped changed paths'

    Invoke-ExpectedFail `
        -Name 'unrelated evidence cannot cover automation root boundary changes' `
        -Paths @(
            'Source/demo_map/demo_mapAutomationRootBoundary.cpp') `
        -Logs @($Full) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'reward projection support cannot omit one dependent reward suite' `
        -Paths @(
            'Source/demo_map/demo_mapRewardProjectionTestSupport.h') `
        -Logs @(
            $RewardJackpot,
            $RewardAffix,
            $RewardBossSource,
            $RareExtremeValue) `
        -ExpectedText 'missing required groups'

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
        -Name 'sword rhythm focus cannot replace BasicSword lifecycle and broad runtime evidence' `
        -Paths @(
            'Source/ShanmenCombatRuntime/Private/ShanmenSwordRhythm.cpp') `
        -Logs @($SwordRhythm) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'sword rhythm contribution focus cannot replace guard evasion lifecycle and broad evidence' `
        -Paths @(
            'Source/ShanmenCombatRuntime/Private/ShanmenSwordRhythmContribution.cpp') `
        -Logs @($SwordRhythmContribution, $SwordRhythm) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'binding focus cannot replace contribution rhythm action and broad evidence' `
        -Paths @(
            'Source/ShanmenCombatRuntime/Private/ShanmenSwordRhythmContributionBinding.cpp') `
        -Logs @($SwordRhythmContributionBinding, $SwordRhythmContribution) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'evaluator focus and product cannot replace binding source and action evidence' `
        -Paths @(
            'Source/ShanmenCombatRuntime/Private/ShanmenSwordRhythmEvaluation.cpp') `
        -Logs @(
            $SwordRhythmEvaluation,
            $SwordRhythmProductSession,
            $SwordRhythmContributionBinding) `
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
        -Name 'shield session focus cannot replace its owned authority contracts' `
        -Paths @(
            'Source/ShanmenCombatRuntime/Private/ShanmenSpiritShieldSession.cpp') `
        -Logs @($SpiritShieldSession, $SpiritShieldAction) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'spirit evasion focus cannot replace lifecycle and resolver evidence' `
        -Paths @(
            'Source/ShanmenCombatRuntime/Private/ShanmenSpiritEvasion.cpp') `
        -Logs @($SpiritEvasion, $ActionLifecycle) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'weapon guard focus cannot replace lifecycle and resolver evidence' `
        -Paths @(
            'Source/ShanmenCombatRuntime/Private/ShanmenWeaponGuard.cpp') `
        -Logs @($WeaponGuard, $ActionLifecycle) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'perfect guard timing focus cannot replace ordinary window lifecycle and resolver evidence' `
        -Paths @(
            'Source/ShanmenCombatRuntime/Private/ShanmenWeaponPerfectGuard.cpp') `
        -Logs @($WeaponPerfectGuard, $WeaponGuard, $ActionLifecycle) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'weapon guard arc focus cannot replace timing ordinary lifecycle and resolver evidence' `
        -Paths @(
            'Source/ShanmenCombatRuntime/Private/ShanmenWeaponGuardArc.cpp') `
        -Logs @(
            $WeaponGuardArc,
            $WeaponPerfectGuard,
            $WeaponGuard,
            $ActionLifecycle) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'weapon guard World focus cannot replace full and World identity evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenWeaponGuardWorldAdapter.cpp') `
        -Logs @(
            $WeaponGuardWorldAdapter,
            $WeaponGuardArc,
            $WeaponPerfectGuard,
            $WeaponGuard,
            $ActionLifecycle,
            $CombatCore) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'weapon guard defense focus cannot replace full and World identity evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenWeaponGuardDefenseCoordinator.cpp') `
        -Logs @(
            $WeaponGuardDefenseCoordinator,
            $WeaponGuardWorldAdapter,
            $WeaponGuardArc,
            $WeaponPerfectGuard,
            $WeaponGuard,
            $ActionLifecycle,
            $CombatCore) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'weapon guard product host focus cannot replace full and World identity evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenWeaponGuardProductHost.cpp') `
        -Logs @(
            $WeaponGuardProductHost,
            $WeaponGuardDefenseCoordinator,
            $WeaponGuardWorldAdapter,
            $WeaponGuardArc,
            $WeaponPerfectGuard,
            $WeaponGuard,
            $ActionLifecycle,
            $CombatCore) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'weapon guard product authority focus cannot replace host Run and legacy evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenWeaponGuardProductAuthority.cpp') `
        -Logs @(
            $WeaponGuardProductAuthority,
            $WeaponGuardProductHost,
            $Coordinator,
            $WeaponGuard,
            $Attributes) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'weapon guard item adapter focus cannot replace catalog and legacy evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenWeaponGuardItemAdapter.cpp') `
        -Logs @(
            $WeaponGuardItemAdapter,
            $WeaponGuardProductAuthority,
            $ItemUseAndArmor) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'weapon guard product route focus cannot replace host and legacy evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenWeaponGuardProductRoute.cpp') `
        -Logs @(
            $WeaponGuardProductRoute,
            $WeaponGuardItemAdapter,
            $WeaponGuardProductAuthority,
            $Coordinator,
            $ItemUseAndArmor) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'weapon guard input adapter focus cannot replace route host and legacy evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenWeaponGuardInputAdapter.cpp') `
        -Logs @(
            $WeaponGuardInputAdapter,
            $WeaponGuardProductRoute,
            $WeaponGuardItemAdapter,
            $Coordinator,
            $ItemUseAndArmor) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'weapon guard product session focus cannot replace route Host and legacy evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenWeaponGuardProductSession.cpp') `
        -Logs @(
            $WeaponGuardProductSession,
            $WeaponGuardProductRoute,
            $WeaponGuardItemAdapter,
            $Coordinator,
            $ItemUseAndArmor) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'weapon guard Impact focus cannot replace World item and enemy evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenWeaponGuardImpactRouteTests.cpp') `
        -Logs @(
            $WeaponGuardImpactRoute,
            $WeaponGuardProductSession,
            $WeaponGuardProductHost,
            $WeaponGuardDefenseCoordinator,
            $WeaponGuardWorldAdapter,
            $Coordinator) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'combat Run timeline focus cannot replace its consumers' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenCombatRunFixedTimeline.cpp') `
        -Logs @($CombatRunFixedTimeline) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'combat condition focus cannot replace vitality timeline attribute and full evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenCombatConditionComponent.cpp') `
        -Logs @($CombatCondition) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'combat condition status focus cannot replace source timeline and attribute evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenCombatConditionStatus.cpp') `
        -Logs @($CombatConditionStatus) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'combat condition presentation event focus cannot replace source status timeline and attribute evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenCombatConditionPresentationEvent.cpp') `
        -Logs @($CombatConditionPresentationEvent) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'combat condition presentation view focus cannot replace source event status timeline and attribute evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenCombatConditionPresentationViewState.cpp') `
        -Logs @($CombatConditionPresentationViewState) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'sword rhythm product Host focus cannot replace action and runtime evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenSwordRhythmProductHost.cpp') `
        -Logs @($SwordRhythmProductHost, $CombatRunFixedTimeline) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'sword rhythm product evaluation focus cannot replace source Host and runtime evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenSwordRhythmProductSession.cpp') `
        -Logs @(
            $SwordRhythmProductSession,
            $SwordRhythmEvaluationRoute,
            $SwordRhythmEffectCue,
            $SwordRhythmPresentation,
            $SwordRhythmPresentationEvent,
            $SwordRhythmProductHost,
            $SwordRhythmEvaluation) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'sword rhythm effect cue focus cannot replace product and runtime evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCue.cpp') `
        -Logs @(
            $SwordRhythmEffectCue,
            $SwordRhythmPresentation,
            $SwordRhythmProductSession,
            $SwordRhythmEvaluationRoute,
            $SwordRhythmProductHost,
            $SwordRhythmEvaluation) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'sword rhythm cue delivery focus cannot replace source product and runtime evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueDelivery.cpp') `
        -Logs @(
            $SwordRhythmEffectCueDelivery,
            $SwordRhythmEffectCue,
            $SwordRhythmPresentation,
            $SwordRhythmProductSession,
            $SwordRhythmEvaluationRoute,
            $SwordRhythmProductHost,
            $SwordRhythmEvaluation) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'sword rhythm cue consumer attempt focus cannot replace delivery source and runtime evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueConsumerAttempt.cpp') `
        -Logs @(
            $SwordRhythmEffectCueConsumerAttempt,
            $SwordRhythmEffectCueDelivery,
            $SwordRhythmEffectCue,
            $SwordRhythmPresentation,
            $SwordRhythmProductSession,
            $SwordRhythmEvaluationRoute,
            $SwordRhythmProductHost,
            $SwordRhythmEvaluation) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'sword rhythm cue executor focus cannot replace attempt delivery and runtime evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueExecutorAdapter.cpp') `
        -Logs @(
            $SwordRhythmEffectCueExecutorAdapter,
            $SwordRhythmEffectCueConsumerAttempt,
            $SwordRhythmEffectCueDelivery,
            $SwordRhythmEffectCue,
            $SwordRhythmPresentation,
            $SwordRhythmProductSession,
            $SwordRhythmEvaluationRoute,
            $SwordRhythmProductHost,
            $SwordRhythmEvaluation) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'sword rhythm cue presentation handoff focus cannot replace adapter delivery and runtime evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCuePresentationHandoffExecutor.cpp') `
        -Logs @(
            $SwordRhythmEffectCuePresentationHandoffExecutor,
            $SwordRhythmEffectCueExecutorAdapter,
            $SwordRhythmEffectCueConsumerAttempt,
            $SwordRhythmEffectCueDelivery,
            $SwordRhythmEffectCue,
            $SwordRhythmPresentation,
            $SwordRhythmProductSession,
            $SwordRhythmEvaluationRoute,
            $SwordRhythmProductHost,
            $SwordRhythmEvaluation) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'sword rhythm cue presentation acknowledgement focus cannot replace Run handoff and runtime evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCuePresentationAcknowledgement.cpp') `
        -Logs @(
            $SwordRhythmEffectCuePresentationAcknowledgement,
            $SwordRhythmEffectCuePresentationRunController,
            $SwordRhythmEffectCuePresentationHandoffExecutor,
            $SwordRhythmEffectCueExecutionProductPreparedDispatch,
            $SwordRhythmEffectCueExecutionProductDispatchPlan,
            $SwordRhythmEffectCueExecutionProductDispatch,
            $SwordRhythmEffectCueExecutionProductTransaction,
            $SwordRhythmEffectCueExecutionProductRoute,
            $SwordRhythmEffectCueExecutionCommandHost,
            $SwordRhythmEffectCueExecutionCommandRouter,
            $SwordRhythmEffectCueExecutionSession,
            $SwordRhythmEffectCueExecutionHost,
            $SwordRhythmEffectCueExecutionDriver,
            $SwordRhythmEffectCueExecutorAdapter,
            $SwordRhythmEffectCueConsumerAttempt,
            $SwordRhythmEffectCueDelivery,
            $SwordRhythmEffectCue,
            $SwordRhythmPresentation,
            $SwordRhythmProductSession,
            $SwordRhythmEvaluationRoute,
            $SwordRhythmProductHost,
            $SwordRhythmEvaluation) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'sword rhythm cue presentation Run controller focus cannot replace prepared-dispatch and runtime evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCuePresentationRunController.cpp') `
        -Logs @(
            $SwordRhythmEffectCuePresentationRunController,
            $SwordRhythmEffectCuePresentationHandoffExecutor,
            $SwordRhythmEffectCueExecutionProductPreparedDispatch,
            $SwordRhythmEffectCueExecutionProductDispatchPlan,
            $SwordRhythmEffectCueExecutionProductDispatch,
            $SwordRhythmEffectCueExecutionProductTransaction,
            $SwordRhythmEffectCueExecutionProductRoute,
            $SwordRhythmEffectCueExecutionCommandHost,
            $SwordRhythmEffectCueExecutionCommandRouter,
            $SwordRhythmEffectCueExecutionSession,
            $SwordRhythmEffectCueExecutionHost,
            $SwordRhythmEffectCueExecutionDriver,
            $SwordRhythmEffectCueExecutorAdapter,
            $SwordRhythmEffectCueConsumerAttempt,
            $SwordRhythmEffectCueDelivery,
            $SwordRhythmEffectCue,
            $SwordRhythmPresentation,
            $SwordRhythmProductSession,
            $SwordRhythmEvaluationRoute,
            $SwordRhythmProductHost,
            $SwordRhythmEvaluation) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'sword rhythm cue driver focus cannot replace executor delivery and runtime evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueExecutionDriver.cpp') `
        -Logs @(
            $SwordRhythmEffectCueExecutionDriver,
            $SwordRhythmEffectCueExecutorAdapter,
            $SwordRhythmEffectCueConsumerAttempt,
            $SwordRhythmEffectCueDelivery,
            $SwordRhythmEffectCue,
            $SwordRhythmPresentation,
            $SwordRhythmProductSession,
            $SwordRhythmEvaluationRoute,
            $SwordRhythmProductHost,
            $SwordRhythmEvaluation) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'sword rhythm cue host focus cannot replace driver delivery and runtime evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueExecutionHost.cpp') `
        -Logs @(
            $SwordRhythmEffectCueExecutionHost,
            $SwordRhythmEffectCueExecutionDriver,
            $SwordRhythmEffectCueExecutorAdapter,
            $SwordRhythmEffectCueConsumerAttempt,
            $SwordRhythmEffectCueDelivery,
            $SwordRhythmEffectCue,
            $SwordRhythmPresentation,
            $SwordRhythmProductSession,
            $SwordRhythmEvaluationRoute,
            $SwordRhythmProductHost,
            $SwordRhythmEvaluation) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'sword rhythm cue command Router focus cannot replace Session Host and runtime evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueExecutionCommandRouter.cpp') `
        -Logs @(
            $SwordRhythmEffectCueExecutionCommandRouter,
            $SwordRhythmEffectCueExecutionSession,
            $SwordRhythmEffectCueExecutionHost,
            $SwordRhythmEffectCueExecutionDriver,
            $SwordRhythmEffectCueExecutorAdapter,
            $SwordRhythmEffectCueConsumerAttempt,
            $SwordRhythmEffectCueDelivery,
            $SwordRhythmEffectCue,
            $SwordRhythmPresentation,
            $SwordRhythmProductSession,
            $SwordRhythmEvaluationRoute,
            $SwordRhythmProductHost,
            $SwordRhythmEvaluation) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'sword rhythm cue session focus cannot replace Host driver and runtime evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueExecutionSession.cpp') `
        -Logs @(
            $SwordRhythmEffectCueExecutionSession,
            $SwordRhythmEffectCueExecutionHost,
            $SwordRhythmEffectCueExecutionDriver,
            $SwordRhythmEffectCueExecutorAdapter,
            $SwordRhythmEffectCueConsumerAttempt,
            $SwordRhythmEffectCueDelivery,
            $SwordRhythmEffectCue,
            $SwordRhythmPresentation,
            $SwordRhythmProductSession,
            $SwordRhythmEvaluationRoute,
            $SwordRhythmProductHost,
            $SwordRhythmEvaluation) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'sword rhythm cue retry envelope manifest codec focus cannot replace codec-to-runtime evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpointEnvelopeManifestCodec.cpp') `
        -Logs @($SwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpointEnvelopeManifestCodec) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'sword rhythm cue retry checkpoint envelope focus cannot replace envelope-to-runtime evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpointEnvelope.cpp') `
        -Logs @($SwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpointEnvelope) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'sword rhythm cue retry journal checkpoint focus cannot replace checkpoint-to-runtime evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpoint.cpp') `
        -Logs @($SwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpoint) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'sword rhythm cue product retry step command journal focus cannot replace journal-to-runtime evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournal.cpp') `
        -Logs @($SwordRhythmEffectCueExecutionProductRetryStepCommandJournal) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'sword rhythm cue product retry step command focus cannot replace command-to-runtime evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommand.cpp') `
        -Logs @($SwordRhythmEffectCueExecutionProductRetryStepCommand) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'sword rhythm cue product retry step focus cannot replace step-to-runtime evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStep.cpp') `
        -Logs @($SwordRhythmEffectCueExecutionProductRetryStep) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'sword rhythm cue product retry decision focus cannot replace decision-to-runtime evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueExecutionProductRetryDecision.cpp') `
        -Logs @($SwordRhythmEffectCueExecutionProductRetryDecision) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'sword rhythm cue product prepared retry focus cannot replace continuation-to-runtime evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetry.cpp') `
        -Logs @($SwordRhythmEffectCueExecutionProductPreparedRetry) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'sword rhythm cue product prepared dispatch focus cannot replace frozen-to-runtime evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatch.cpp') `
        -Logs @($SwordRhythmEffectCueExecutionProductPreparedDispatch) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'sword rhythm cue product planned dispatch focus cannot replace plan-to-runtime evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueExecutionProductPlannedDispatch.cpp') `
        -Logs @($SwordRhythmEffectCueExecutionProductPlannedDispatch) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'sword rhythm cue product dispatch plan focus cannot replace runtime evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchPlan.cpp') `
        -Logs @($SwordRhythmEffectCueExecutionProductDispatchPlan) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'sword rhythm cue product dispatch focus cannot replace current-to-transaction evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueExecutionProductDispatch.cpp') `
        -Logs @($SwordRhythmEffectCueExecutionProductDispatch) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'sword rhythm cue product transaction focus cannot replace lifecycle evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueExecutionProductTransaction.cpp') `
        -Logs @($SwordRhythmEffectCueExecutionProductTransaction) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'sword rhythm cue product route focus cannot replace source-to-Host evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueExecutionProductRoute.cpp') `
        -Logs @(
            $SwordRhythmEffectCueExecutionProductRoute,
            $SwordRhythmEffectCueExecutionCommandHost,
            $SwordRhythmEffectCueExecutionCommandRouter,
            $SwordRhythmEffectCueExecutionSession,
            $SwordRhythmEffectCueExecutionHost,
            $SwordRhythmEffectCueExecutionDriver,
            $SwordRhythmEffectCueExecutorAdapter,
            $SwordRhythmEffectCueConsumerAttempt,
            $SwordRhythmEffectCueDelivery,
            $SwordRhythmEffectCue,
            $SwordRhythmPresentation,
            $SwordRhythmProductSession,
            $SwordRhythmEvaluationRoute,
            $SwordRhythmProductHost,
            $SwordRhythmEvaluation) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'sword rhythm cue command Host focus cannot replace Router Session Host and runtime evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenSwordRhythmEffectCueExecutionCommandHost.cpp') `
        -Logs @(
            $SwordRhythmEffectCueExecutionCommandHost,
            $SwordRhythmEffectCueExecutionCommandRouter,
            $SwordRhythmEffectCueExecutionSession,
            $SwordRhythmEffectCueExecutionHost,
            $SwordRhythmEffectCueExecutionDriver,
            $SwordRhythmEffectCueExecutorAdapter,
            $SwordRhythmEffectCueConsumerAttempt,
            $SwordRhythmEffectCueDelivery,
            $SwordRhythmEffectCue,
            $SwordRhythmPresentation,
            $SwordRhythmProductSession,
            $SwordRhythmEvaluationRoute,
            $SwordRhythmProductHost,
            $SwordRhythmEvaluation) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'sword rhythm presentation evaluation focus cannot replace Host and runtime evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenSwordRhythmPresentation.cpp') `
        -Logs @(
            $SwordRhythmPresentation,
            $SwordRhythmPresentationEvent,
            $SwordRhythmProductSession,
            $SwordRhythmEvaluationRoute,
            $SwordRhythmProductHost,
            $SwordRhythmEvaluation) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'sword rhythm presentation event focus cannot replace read model and runtime evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenSwordRhythmPresentationEvent.cpp') `
        -Logs @(
            $SwordRhythmPresentationEvent,
            $SwordRhythmPresentation,
            $SwordRhythmProductSession,
            $SwordRhythmEvaluationRoute,
            $SwordRhythmProductHost,
            $SwordRhythmEvaluation) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'spirit evasion movement focus cannot replace its window contract' `
        -Paths @(
            'Source/ShanmenCombatRuntime/Private/ShanmenSpiritEvasionMovement.cpp') `
        -Logs @($SpiritEvasionMovement, $ActionLifecycle) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'spirit evasion product focus cannot replace legacy displacement evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenSpiritEvasionMovementAdapter.cpp') `
        -Logs @($SpiritEvasionMovementProduct, $SpiritEvasionMovement) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'spirit evasion motion focus cannot replace adapter and legacy evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenSpiritEvasionMotionRuntime.cpp') `
        -Logs @(
            $SpiritEvasionMotionRuntime,
            $SpiritEvasionMovementProduct,
            $SpiritEvasionMovement) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'spirit evasion coordinator focus cannot replace lifecycle and legacy evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenSpiritEvasionActionCoordinator.cpp') `
        -Logs @(
            $SpiritEvasionActionCoordinator,
            $SpiritEvasionMotionRuntime,
            $SpiritEvasionMovementProduct,
            $SpiritEvasionMovement) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'spirit evasion product host focus cannot replace its composed chain' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenSpiritEvasionProductHost.cpp') `
        -Logs @(
            $SpiritEvasionProductHost,
            $SpiritEvasionActionCoordinator,
            $SpiritEvasionMotionRuntime,
            $SpiritEvasionMovementProduct,
            $SpiritEvasionMovement) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'spirit evasion component focus cannot replace host and chain evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenSpiritEvasionComponent.cpp') `
        -Logs @(
            $SpiritEvasionComponent,
            $SpiritEvasionProductHost,
            $SpiritEvasionActionCoordinator,
            $SpiritEvasionMotionRuntime,
            $SpiritEvasionMovementProduct) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'spirit evasion product authority focus cannot replace reservation route and legacy evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenSpiritEvasionProductAuthority.cpp') `
        -Logs @(
            $SpiritEvasionProductAuthority,
            $SpiritEvasionCommandRouter,
            $Coordinator,
            $Attributes) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'spirit evasion product route focus cannot replace authority host and legacy evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenSpiritEvasionProductRoute.cpp') `
        -Logs @(
            $SpiritEvasionProductRoute,
            $SpiritEvasionProductAuthority,
            $SpiritEvasionCommandRouter,
            $Coordinator,
            $Attributes) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'spirit evasion input focus cannot replace product route and legacy evidence' `
        -Paths @(
            'Source/demo_map/demo_mapShanmenSpiritEvasionInputAdapter.cpp') `
        -Logs @(
            $SpiritEvasionInputAdapter,
            $SpiritEvasionProductRoute,
            $SpiritEvasionProductAuthority,
            $Coordinator,
            $Attributes) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'physical input focus cannot replace registry migration and legacy input suites' `
        -Paths @(
            'Source/demo_map/demo_mapInputActionRegistry.cpp',
            'Source/demo_map/demo_mapShanmenSpiritEvasionPhysicalInputTests.cpp') `
        -Logs @(
            $Full,
            $SpiritEvasionPhysicalInput,
            $SpiritEvasionInputAdapter,
            $Ranged) `
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

    Invoke-ExpectedFail `
        -Name 'spatial ring schema fixture cannot use unrelated full evidence' `
        -Paths @(
            'Source/demo_map/demo_mapP1R6SpatialRingTests.cpp') `
        -Logs @($Full) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'legacy full-system fixture cannot use only new-module evidence' `
        -Paths @(
            'Source/demo_map/demo_mapFullSystemLoopTests.cpp') `
        -Logs @($Full) `
        -ExpectedText 'missing required groups'

    Invoke-ExpectedFail `
        -Name 'legacy search-container fixture cannot use unrelated full evidence' `
        -Paths @(
            'Source/demo_map/demo_mapSearchContainerTests.cpp') `
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
