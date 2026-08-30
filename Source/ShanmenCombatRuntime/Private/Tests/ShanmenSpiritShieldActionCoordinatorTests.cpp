#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ShanmenCombatResolver.h"
#include "ShanmenCombatRuntimeTags.h"
#include "ShanmenCombatTags.h"
#include "ShanmenSpiritShieldActionCoordinator.h"
#include "ShanmenSpiritShieldCapacityAuthority.h"
#include "ShanmenSpiritShieldDeadlineGate.h"

namespace
{
	const FGuid CompositionRunId(
		0xC9400001, 0xC9400002, 0xC9400003, 0xC9400004);
	const FGuid CompositionOwnerId(
		0xC9410001, 0xC9410002, 0xC9410003, 0xC9410004);
	const FGuid CompositionSourceId(
		0xC9420001, 0xC9420002, 0xC9420003, 0xC9420004);
	const FGuid ForeignSourceId(
		0xC9430001, 0xC9430002, 0xC9430003, 0xC9430004);
	const FGuid CompositionTimelineId(
		0xC9440001, 0xC9440002, 0xC9440003, 0xC9440004);

	FShanmenCombatActionSnapshot MakeAction(
		uint64 ActivationSequence = 940,
		const FGuid& SourceEntityId = CompositionSourceId,
		const FString& Digest = TEXT("TEST-DIGEST-P9.4"))
	{
		FShanmenCombatActionCapture Capture;
		Capture.RunId = CompositionRunId;
		Capture.OwnerId = CompositionOwnerId;
		Capture.SourceEntityId = SourceEntityId;
		Capture.ActionDefinitionId =
			FShanmenSpiritShieldDefinition::CanonicalActionDefinitionId();
		Capture.Content.Version = TEXT("0.0.10.P9.4");
		Capture.Content.Digest = Digest;
		Capture.SourceTags.AddTag(
			FShanmenCombatNativeTags::SourcePlayer());
		Capture.ActivationId = FShanmenCombatIdFactory::MakeActivationId(
			Capture.RunId,
			Capture.SourceEntityId,
			Capture.ActionDefinitionId,
			ActivationSequence);
		FShanmenCombatActionSnapshot Action;
		check(FShanmenCombatActionSnapshot::TryCapture(Capture, Action));
		return Action;
	}

	FShanmenSpiritShieldDefinition MakeDefinition(
		FName RuleId = TEXT("Defense.Spell.SpiritShield.P9_4"),
		float Capacity = 40.0f)
	{
		FShanmenSpiritShieldDefinitionCapture Capture;
		Capture.ActionDefinitionId =
			FShanmenSpiritShieldDefinition::CanonicalActionDefinitionId();
		Capture.RuleId = RuleId;
		Capture.MaximumCapacity = Capacity;
		Capture.RequiredDamageTags.AddTag(
			FShanmenCombatNativeTags::DamagePhysical());
		Capture.BlockedDamageTags.AddTag(
			FShanmenCombatNativeTags::DamageMental());
		Capture.RequiredTargetTags.AddTag(
			FShanmenCombatNativeTags::TargetLiving());
		FShanmenSpiritShieldDefinition Definition;
		check(FShanmenSpiritShieldDefinition::TryCapture(
			Capture, Definition));
		return Definition;
	}

	FShanmenActionResourceCost MakeCost(
		float Amount = 20.0f,
		FName RuleId = TEXT("Cost.Spell.SpiritShield.P9_4"),
		FGameplayTag Channel =
			FShanmenCombatRuntimeNativeTags::ResourceSpiritEnergy())
	{
		FShanmenActionResourceCostCapture Capture;
		Capture.RuleId = RuleId;
		Capture.ResourceChannel = Channel;
		Capture.Amount = Amount;
		FShanmenActionResourceCost Cost;
		check(FShanmenActionResourceCost::TryCapture(Capture, Cost));
		return Cost;
	}

	FShanmenActionResourceAuthority MakeAuthority(
		float CurrentAmount = 100.0f,
		float MaximumAmount = 100.0f,
		const FGuid& OwnerEntityId = CompositionSourceId,
		FGameplayTag Channel =
			FShanmenCombatRuntimeNativeTags::ResourceSpiritEnergy())
	{
		FShanmenActionResourceAuthority Authority;
		check(FShanmenActionResourceAuthority::TryCreate(
			OwnerEntityId,
			Channel,
			CurrentAmount,
			MaximumAmount,
			0,
			Authority));
		return Authority;
	}

	FShanmenSpiritShieldActionResult BeginDefault(
		FShanmenActionResourceAuthority& Authority,
		FShanmenSpiritShieldActionCoordinator& Coordinator,
		const FShanmenCombatActionSnapshot& Action = MakeAction(),
		const FShanmenSpiritShieldDefinition& Definition = MakeDefinition(),
		const FShanmenActionResourceCost& Cost = MakeCost())
	{
		return FShanmenSpiritShieldActionCoordinator::Begin(
			Action, Definition, Cost, Authority, Coordinator);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenSpiritShieldActionBeginTest,
	"Shanmen.0_0_10.CombatRuntime.SpiritShieldAction.BeginAndReplay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenSpiritShieldActionBeginTest::RunTest(const FString&)
{
	const FShanmenCombatActionSnapshot Action = MakeAction();
	const FShanmenSpiritShieldDefinition Definition = MakeDefinition();
	const FShanmenActionResourceCost Cost = MakeCost();
	FShanmenActionResourceAuthority Authority = MakeAuthority();
	FShanmenSpiritShieldActionCoordinator Coordinator;
	const FShanmenSpiritShieldActionResult Begun =
		FShanmenSpiritShieldActionCoordinator::Begin(
			Action, Definition, Cost, Authority, Coordinator);
	TestTrue(TEXT("Begin atomically starts, prepares and reserves"),
		Begun.IsSuccess()
			&& Begun.Status == EShanmenSpiritShieldActionStatus::Begun
			&& Begun.Startup.IsValid()
			&& Coordinator.IsValid()
			&& Coordinator.GetState()
				== EShanmenSpiritShieldActionState::Reserved
			&& Coordinator.GetActionRuntime().GetPhase()
				== EShanmenCombatActionPhase::Startup
			&& Coordinator.GetShieldRuntime().GetState()
				== EShanmenSpiritShieldState::Prepared
			&& FMath::IsNearlyEqual(Authority.GetCurrentAmount(), 100.0f)
			&& FMath::IsNearlyEqual(Authority.GetReservedAmount(), 20.0f)
			&& FMath::IsNearlyEqual(Authority.GetAvailableAmount(), 80.0f)
			&& Authority.GetAuthorityRevision() == 1);

	const FGuid StartupReceiptId = Begun.Startup.GetReceiptId();
	const FShanmenSpiritShieldActionResult Replay =
		FShanmenSpiritShieldActionCoordinator::Begin(
			Action, Definition, Cost, Authority, Coordinator);
	TestTrue(TEXT("Exact Begin replay returns the original reservation proof"),
		Replay.IsSuccess()
			&& Replay.Status
				== EShanmenSpiritShieldActionStatus::AlreadyBegun
			&& Replay.Startup.GetReceiptId() == StartupReceiptId
			&& Authority.GetAuthorityRevision() == 1
			&& FMath::IsNearlyEqual(Authority.GetReservedAmount(), 20.0f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenSpiritShieldActionCommitTest,
	"Shanmen.0_0_10.CombatRuntime.SpiritShieldAction.AtomicCommitActivation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenSpiritShieldActionCommitTest::RunTest(const FString&)
{
	FShanmenActionResourceAuthority Authority = MakeAuthority();
	FShanmenSpiritShieldActionCoordinator Coordinator;
	check(BeginDefault(Authority, Coordinator).IsSuccess());
	const FShanmenSpiritShieldActionResult Activated =
		Coordinator.Commit(Authority);
	TestTrue(TEXT("Commit spends once and activates the same action shield"),
		Activated.IsSuccess()
			&& Activated.Status
				== EShanmenSpiritShieldActionStatus::Activated
			&& Activated.Terminal.IsValid()
			&& Activated.Terminal.GetOutcome()
				== EShanmenSpiritShieldActionOutcome::Active
			&& Activated.Terminal.GetResourceFinalization()
				.GetRequest().GetDisposition()
				== EShanmenActionResourceDisposition::Commit
			&& Activated.Terminal.GetShieldActivation().IsValid()
			&& Coordinator.IsValid()
			&& Coordinator.GetState()
				== EShanmenSpiritShieldActionState::Activated
			&& Coordinator.GetActionRuntime().CanEmitCandidates()
			&& Coordinator.GetShieldRuntime().GetState()
				== EShanmenSpiritShieldState::Active
			&& FMath::IsNearlyEqual(Authority.GetCurrentAmount(), 80.0f)
			&& FMath::IsNearlyZero(Authority.GetReservedAmount())
			&& FMath::IsNearlyEqual(Authority.GetAvailableAmount(), 80.0f)
			&& Authority.GetAuthorityRevision() == 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenSpiritShieldActionAbortTest,
	"Shanmen.0_0_10.CombatRuntime.SpiritShieldAction.PreCommitAbortRelease",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenSpiritShieldActionAbortTest::RunTest(const FString&)
{
	FShanmenActionResourceAuthority CancelAuthority = MakeAuthority();
	FShanmenSpiritShieldActionCoordinator CancelCoordinator;
	check(BeginDefault(CancelAuthority, CancelCoordinator).IsSuccess());
	const FShanmenSpiritShieldActionResult Cancelled =
		CancelCoordinator.Abort(
			EShanmenActionTerminalReason::Cancelled, CancelAuthority);
	TestTrue(TEXT("Pre-commit cancellation releases without activating"),
		Cancelled.IsSuccess()
			&& Cancelled.Status
				== EShanmenSpiritShieldActionStatus::Aborted
			&& Cancelled.Terminal.GetOutcome()
				== EShanmenSpiritShieldActionOutcome::Cancelled
			&& Cancelled.Terminal.GetResourceFinalization()
				.GetRequest().GetDisposition()
				== EShanmenActionResourceDisposition::Release
			&& !Cancelled.Terminal.GetShieldActivation().IsValid()
			&& CancelCoordinator.IsValid()
			&& CancelCoordinator.GetShieldRuntime().GetState()
				== EShanmenSpiritShieldState::Prepared
			&& FMath::IsNearlyEqual(CancelAuthority.GetCurrentAmount(), 100.0f)
			&& FMath::IsNearlyZero(CancelAuthority.GetReservedAmount())
			&& CancelAuthority.GetAuthorityRevision() == 2);

	const FShanmenCombatActionSnapshot InterruptedAction = MakeAction(
		941, CompositionSourceId, TEXT("TEST-DIGEST-P9.4-INTERRUPT"));
	FShanmenActionResourceAuthority InterruptAuthority = MakeAuthority();
	FShanmenSpiritShieldActionCoordinator InterruptCoordinator;
	check(BeginDefault(
		InterruptAuthority, InterruptCoordinator, InterruptedAction).IsSuccess());
	const FShanmenSpiritShieldActionResult Interrupted =
		InterruptCoordinator.Abort(
			EShanmenActionTerminalReason::Interrupted, InterruptAuthority);
	TestTrue(TEXT("Pre-commit interruption uses the same release invariant"),
		Interrupted.IsSuccess()
			&& Interrupted.Terminal.GetOutcome()
				== EShanmenSpiritShieldActionOutcome::Interrupted
			&& InterruptCoordinator.GetActionRuntime().GetTerminalReason()
				== EShanmenActionTerminalReason::Interrupted
			&& FMath::IsNearlyEqual(
				InterruptAuthority.GetCurrentAmount(), 100.0f)
			&& FMath::IsNearlyZero(
				InterruptAuthority.GetReservedAmount()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenSpiritShieldActionTerminalReplayTest,
	"Shanmen.0_0_10.CombatRuntime.SpiritShieldAction.TerminalReplayConflict",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenSpiritShieldActionTerminalReplayTest::RunTest(const FString&)
{
	FShanmenActionResourceAuthority Authority = MakeAuthority();
	FShanmenSpiritShieldActionCoordinator Coordinator;
	check(BeginDefault(Authority, Coordinator).IsSuccess());
	const FShanmenSpiritShieldActionResult First =
		Coordinator.Commit(Authority);
	check(First.IsSuccess());
	const FGuid TerminalReceiptId = First.Terminal.GetReceiptId();
	const FShanmenSpiritShieldActionResult Replay =
		Coordinator.Commit(Authority);
	TestTrue(TEXT("Exact terminal replay never spends or activates twice"),
		Replay.IsSuccess()
			&& Replay.Status
				== EShanmenSpiritShieldActionStatus::AlreadyFinalized
			&& Replay.Terminal.GetReceiptId() == TerminalReceiptId
			&& Authority.GetAuthorityRevision() == 2
			&& FMath::IsNearlyEqual(Authority.GetCurrentAmount(), 80.0f));

	const FShanmenSpiritShieldActionResult Conflict = Coordinator.Abort(
		EShanmenActionTerminalReason::Cancelled, Authority);
	TestTrue(TEXT("Activated history cannot be rewritten as cancellation"),
		Conflict.IsValid() && !Conflict.IsSuccess()
			&& Conflict.Error
				== EShanmenSpiritShieldActionError::FinalizationConflict
			&& Coordinator.GetTerminalReceipt().GetReceiptId()
				== TerminalReceiptId
			&& FMath::IsNearlyEqual(Authority.GetCurrentAmount(), 80.0f));

	FShanmenActionResourceAuthority AbortAuthority = MakeAuthority();
	FShanmenSpiritShieldActionCoordinator AbortCoordinator;
	check(BeginDefault(AbortAuthority, AbortCoordinator).IsSuccess());
	const auto FirstAbort = AbortCoordinator.Abort(
		EShanmenActionTerminalReason::Cancelled, AbortAuthority);
	const auto AbortReplay = AbortCoordinator.Abort(
		EShanmenActionTerminalReason::Cancelled, AbortAuthority);
	TestTrue(TEXT("Exact abort replay returns the release proof"),
		FirstAbort.IsSuccess() && AbortReplay.IsSuccess()
			&& AbortReplay.Status
				== EShanmenSpiritShieldActionStatus::AlreadyFinalized
			&& AbortReplay.Terminal.GetReceiptId()
				== FirstAbort.Terminal.GetReceiptId()
			&& AbortAuthority.GetAuthorityRevision() == 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenSpiritShieldActionFailClosedTest,
	"Shanmen.0_0_10.CombatRuntime.SpiritShieldAction.FailClosedAtomicity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenSpiritShieldActionFailClosedTest::RunTest(const FString&)
{
	FShanmenActionResourceAuthority Insufficient = MakeAuthority(10.0f, 10.0f);
	FShanmenSpiritShieldActionCoordinator NoCoordinator;
	const auto NoCapacity = BeginDefault(Insufficient, NoCoordinator);
	TestTrue(TEXT("Insufficient energy leaves every authority untouched"),
		NoCapacity.IsValid() && !NoCapacity.IsSuccess()
			&& NoCapacity.Error
				== EShanmenSpiritShieldActionError::ResourceReservationRejected
			&& NoCapacity.ResourceError
				== EShanmenActionResourceTransactionError::InsufficientAvailable
			&& !NoCoordinator.IsValid()
			&& Insufficient.GetAuthorityRevision() == 0
			&& FMath::IsNearlyEqual(Insufficient.GetCurrentAmount(), 10.0f)
			&& FMath::IsNearlyZero(Insufficient.GetReservedAmount()));

	FShanmenActionResourceAuthority Authority = MakeAuthority();
	FShanmenSpiritShieldActionCoordinator ForeignCoordinator;
	const auto Foreign = BeginDefault(
		Authority,
		ForeignCoordinator,
		MakeAction(942, ForeignSourceId, TEXT("TEST-DIGEST-P9.4-FOREIGN")));
	TestTrue(TEXT("Foreign resource owner fails before action state escapes"),
		Foreign.IsValid() && !Foreign.IsSuccess()
			&& Foreign.ResourceError
				== EShanmenActionResourceTransactionError::OwnerMismatch
			&& !ForeignCoordinator.IsValid()
			&& Authority.GetAuthorityRevision() == 0);

	FShanmenSpiritShieldActionCoordinator Coordinator;
	const FShanmenCombatActionSnapshot Action = MakeAction();
	check(BeginDefault(Authority, Coordinator, Action).IsSuccess());
	FShanmenActionOrchestrator ExternalAction;
	FShanmenActionTransitionReceipt ExternalTransition;
	check(FShanmenActionOrchestrator::TryStart(
		Action, ExternalAction, ExternalTransition));
	check(ExternalAction.TryCancel(
		EShanmenCombatActionPhase::Startup, ExternalTransition));
	FShanmenActionResourceFinalizationRequest ExternalRelease;
	check(FShanmenActionResourceFinalizationRequest::TryCreate(
		Coordinator.GetStartupReceipt().GetReservation(),
		ExternalTransition,
		ExternalRelease));
	check(Authority.Finalize(ExternalRelease).IsSuccess());
	const auto Desynchronized = Coordinator.Commit(Authority);
	TestTrue(TEXT("Externally rewritten resource history cannot half-activate"),
		Desynchronized.IsValid() && !Desynchronized.IsSuccess()
			&& Desynchronized.Error
				== EShanmenSpiritShieldActionError::ResourceFinalizationRejected
			&& Desynchronized.ResourceError
				== EShanmenActionResourceTransactionError::FinalizationConflict
			&& Coordinator.GetState()
				== EShanmenSpiritShieldActionState::Reserved
			&& Coordinator.GetActionRuntime().GetPhase()
				== EShanmenCombatActionPhase::Startup
			&& Coordinator.GetShieldRuntime().GetState()
				== EShanmenSpiritShieldState::Prepared
			&& FMath::IsNearlyEqual(Authority.GetCurrentAmount(), 100.0f)
			&& FMath::IsNearlyZero(Authority.GetReservedAmount()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenSpiritShieldActionCapacityDeadlineTest,
	"Shanmen.0_0_10.CombatRuntime.SpiritShieldAction.CapacityDeadlineComposition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenSpiritShieldActionCapacityDeadlineTest::RunTest(const FString&)
{
	FShanmenActionResourceAuthority ResourceAuthority = MakeAuthority();
	FShanmenSpiritShieldActionCoordinator Coordinator;
	check(BeginDefault(ResourceAuthority, Coordinator).IsSuccess());
	const FShanmenSpiritShieldActionResult Activated =
		Coordinator.Commit(ResourceAuthority);
	check(Activated.IsSuccess());

	FShanmenSpiritShieldCapacityAuthority CapacityAuthority;
	check(FShanmenSpiritShieldCapacityAuthority::TryCreate(
		Activated.Terminal.GetShieldActivation(), CapacityAuthority));
	FShanmenSpiritShieldProjectionReceipt Projection;
	TestTrue(TEXT("Committed composition feeds the existing capacity authority"),
		CapacityAuthority.TryProjectDefenseLayer(
			Coordinator.GetShieldRuntime(), Projection)
			&& Projection.IsValid()
			&& FMath::IsNearlyEqual(
				Projection.GetAvailableCapacity(), 40.0f));

	FShanmenSpiritShieldDeadlineContract Contract;
	check(FShanmenSpiritShieldDeadlineContract::TryCapture(
		Activated.Terminal.GetShieldActivation(),
		CompositionTimelineId,
		100,
		120,
		Contract));
	FShanmenSpiritShieldDeadlineGate Gate;
	check(FShanmenSpiritShieldDeadlineGate::TryCreate(Contract, Gate));
	FShanmenSpiritShieldTimelineObservation Due;
	check(FShanmenSpiritShieldTimelineObservation::TryCapture(
		CompositionTimelineId, 120, Due));
	const FShanmenSpiritShieldDeadlineResult Elapsed =
		Gate.TryElapse(Coordinator.GetShieldRuntime(), Due);
	TestTrue(TEXT("The existing deadline gate may end the activated shield"),
		Elapsed.IsSuccess()
			&& Elapsed.Receipt.IsValid()
			&& Coordinator.GetShieldRuntime().GetState()
				== EShanmenSpiritShieldState::Deactivated
			&& Coordinator.IsValid()
			&& FMath::IsNearlyEqual(
				ResourceAuthority.GetCurrentAmount(), 80.0f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenSpiritShieldActionDeterminismTest,
	"Shanmen.0_0_10.CombatRuntime.SpiritShieldAction.DeterministicReplay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenSpiritShieldActionDeterminismTest::RunTest(const FString&)
{
	const FShanmenCombatActionSnapshot Action = MakeAction(950);
	const FShanmenSpiritShieldDefinition Definition = MakeDefinition();
	const FShanmenActionResourceCost Cost = MakeCost();
	FShanmenActionResourceAuthority FirstAuthority = MakeAuthority();
	FShanmenActionResourceAuthority ReplayAuthority = MakeAuthority();
	FShanmenSpiritShieldActionCoordinator FirstCoordinator;
	FShanmenSpiritShieldActionCoordinator ReplayCoordinator;
	const auto FirstBegin = FShanmenSpiritShieldActionCoordinator::Begin(
		Action, Definition, Cost, FirstAuthority, FirstCoordinator);
	const auto ReplayBegin = FShanmenSpiritShieldActionCoordinator::Begin(
		Action, Definition, Cost, ReplayAuthority, ReplayCoordinator);
	check(FirstBegin.IsSuccess() && ReplayBegin.IsSuccess());
	const auto FirstCommit = FirstCoordinator.Commit(FirstAuthority);
	const auto ReplayCommit = ReplayCoordinator.Commit(ReplayAuthority);
	TestTrue(TEXT("Equivalent frozen composition reproduces all identities"),
		FirstCommit.IsSuccess() && ReplayCommit.IsSuccess()
			&& FirstBegin.Startup.GetSessionId()
				== ReplayBegin.Startup.GetSessionId()
			&& FirstBegin.Startup.GetReceiptId()
				== ReplayBegin.Startup.GetReceiptId()
			&& FirstCommit.Terminal.GetResourceFinalization().GetReceiptId()
				== ReplayCommit.Terminal.GetResourceFinalization().GetReceiptId()
			&& FirstCommit.Terminal.GetShieldActivation().GetReceiptId()
				== ReplayCommit.Terminal.GetShieldActivation().GetReceiptId()
			&& FirstCommit.Terminal.GetReceiptId()
				== ReplayCommit.Terminal.GetReceiptId()
			&& FMath::IsNearlyEqual(
				FirstAuthority.GetCurrentAmount(),
				ReplayAuthority.GetCurrentAmount()));
	return true;
}

#endif
