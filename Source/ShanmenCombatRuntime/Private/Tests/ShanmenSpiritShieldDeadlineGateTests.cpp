#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ShanmenCombatResolver.h"
#include "ShanmenSpiritShieldCapacityAuthority.h"
#include "ShanmenSpiritShieldDeadlineGate.h"

namespace
{
	const FGuid DeadlineRunId(0xD9200001, 0xD9200002, 0xD9200003, 0xD9200004);
	const FGuid DeadlineOwnerId(0xD9210001, 0xD9210002, 0xD9210003, 0xD9210004);
	const FGuid DeadlineSourceId(0xD9220001, 0xD9220002, 0xD9220003, 0xD9220004);
	const FGuid DeadlineTimelineId(0xD9230001, 0xD9230002, 0xD9230003, 0xD9230004);

	FShanmenCombatActionSnapshot MakeDeadlineAction(
		uint64 ActivationSequence = 92,
		const FString& Digest = TEXT("TEST-DIGEST-P9.2"))
	{
		FShanmenCombatActionCapture Capture;
		Capture.RunId = DeadlineRunId;
		Capture.OwnerId = DeadlineOwnerId;
		Capture.SourceEntityId = DeadlineSourceId;
		Capture.ActionDefinitionId =
			FShanmenSpiritShieldDefinition::CanonicalActionDefinitionId();
		Capture.Content.Version = TEXT("0.0.10.P9.2");
		Capture.Content.Digest = Digest;
		Capture.ActivationId = FShanmenCombatIdFactory::MakeActivationId(
			Capture.RunId,
			Capture.SourceEntityId,
			Capture.ActionDefinitionId,
			ActivationSequence);
		FShanmenCombatActionSnapshot Action;
		check(FShanmenCombatActionSnapshot::TryCapture(Capture, Action));
		return Action;
	}

	FShanmenSpiritShieldDefinition MakeDeadlineDefinition()
	{
		FShanmenSpiritShieldDefinitionCapture Capture;
		Capture.ActionDefinitionId =
			FShanmenSpiritShieldDefinition::CanonicalActionDefinitionId();
		Capture.RuleId = TEXT("Defense.Spell.SpiritShield.DeadlineTest");
		Capture.MaximumCapacity = 40.0f;
		FShanmenSpiritShieldDefinition Definition;
		check(FShanmenSpiritShieldDefinition::TryCapture(
			Capture, Definition));
		return Definition;
	}

	void StartDeadlineShield(
		FShanmenActionOrchestrator& OutActionRuntime,
		FShanmenSpiritShieldRuntime& OutShieldRuntime,
		FShanmenSpiritShieldActivationReceipt& OutActivation,
		uint64 ActivationSequence = 92,
		const FString& Digest = TEXT("TEST-DIGEST-P9.2"))
	{
		const FShanmenCombatActionSnapshot Action =
			MakeDeadlineAction(ActivationSequence, Digest);
		FShanmenActionTransitionReceipt Transition;
		check(FShanmenActionOrchestrator::TryStart(
			Action, OutActionRuntime, Transition));
		check(FShanmenSpiritShieldRuntime::TryPrepare(
			Action, MakeDeadlineDefinition(), OutShieldRuntime));
		check(OutActionRuntime.TryAdvance(
			EShanmenCombatActionPhase::Startup, Transition));
		check(OutShieldRuntime.TryActivate(
			OutActionRuntime, OutActivation));
	}

	FShanmenSpiritShieldDeadlineContract MakeDeadlineContract(
		const FShanmenSpiritShieldActivationReceipt& Activation,
		const FGuid& TimelineId = DeadlineTimelineId,
		int64 StartTick = 100,
		int64 DeadlineTick = 120)
	{
		FShanmenSpiritShieldDeadlineContract Contract;
		check(FShanmenSpiritShieldDeadlineContract::TryCapture(
			Activation, TimelineId, StartTick, DeadlineTick, Contract));
		return Contract;
	}

	FShanmenSpiritShieldTimelineObservation MakeObservation(
		int64 ObservedTick,
		const FGuid& TimelineId = DeadlineTimelineId)
	{
		FShanmenSpiritShieldTimelineObservation Observation;
		check(FShanmenSpiritShieldTimelineObservation::TryCapture(
			TimelineId, ObservedTick, Observation));
		return Observation;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenSpiritShieldDeadlineContractTest,
	"Shanmen.0_0_10.CombatRuntime.SpiritShieldDeadline.Contract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenSpiritShieldDeadlineContractTest::RunTest(const FString&)
{
	FShanmenActionOrchestrator ActionRuntime;
	FShanmenSpiritShieldRuntime ShieldRuntime;
	FShanmenSpiritShieldActivationReceipt Activation;
	StartDeadlineShield(ActionRuntime, ShieldRuntime, Activation);
	const FShanmenSpiritShieldDeadlineContract Contract =
		MakeDeadlineContract(Activation);
	const FShanmenSpiritShieldDeadlineContract Replay =
		MakeDeadlineContract(Activation);
	TestTrue(TEXT("One activation captures an immutable deadline contract"),
		Contract.IsValid()
			&& Contract.GetContractId() == Replay.GetContractId()
			&& Contract.GetStartTick() == 100
			&& Contract.GetDeadlineTick() == 120);

	FShanmenSpiritShieldDeadlineContract Invalid;
	TestFalse(TEXT("A deadline must move forward on a valid timeline"),
		FShanmenSpiritShieldDeadlineContract::TryCapture(
			Activation, DeadlineTimelineId, 120, 120, Invalid));
	TestFalse(TEXT("An anonymous timeline cannot own shield duration"),
		FShanmenSpiritShieldDeadlineContract::TryCapture(
			Activation, FGuid(), 100, 120, Invalid));

	const FShanmenSpiritShieldTimelineObservation Observation =
		MakeObservation(120);
	const FShanmenSpiritShieldTimelineObservation ObservationReplay =
		MakeObservation(120);
	TestTrue(TEXT("Equivalent external samples reproduce one event identity"),
		Observation.IsValid()
			&& Observation.GetObservationId()
				== ObservationReplay.GetObservationId());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenSpiritShieldDeadlineElapseTest,
	"Shanmen.0_0_10.CombatRuntime.SpiritShieldDeadline.EarlyExactAndReplay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenSpiritShieldDeadlineElapseTest::RunTest(const FString&)
{
	FShanmenActionOrchestrator ActionRuntime;
	FShanmenSpiritShieldRuntime ShieldRuntime;
	FShanmenSpiritShieldActivationReceipt Activation;
	StartDeadlineShield(ActionRuntime, ShieldRuntime, Activation);
	FShanmenSpiritShieldDeadlineGate Gate;
	check(FShanmenSpiritShieldDeadlineGate::TryCreate(
		MakeDeadlineContract(Activation), Gate));

	const FShanmenSpiritShieldDeadlineResult Early =
		Gate.TryElapse(ShieldRuntime, MakeObservation(119));
	TestTrue(TEXT("Pre-deadline observations fail closed without mutation"),
		Early.IsValid() && !Early.IsSuccess()
			&& Early.Error
				== EShanmenSpiritShieldDeadlineError::DeadlineNotReached
			&& ShieldRuntime.GetState()
				== EShanmenSpiritShieldState::Active
			&& !Gate.HasElapsed());

	const FShanmenSpiritShieldTimelineObservation Due = MakeObservation(120);
	const FShanmenSpiritShieldDeadlineResult Elapsed =
		Gate.TryElapse(ShieldRuntime, Due);
	TestTrue(TEXT("The exact deadline observation ends the bound shield"),
		Elapsed.IsSuccess()
			&& Elapsed.Status == EShanmenSpiritShieldDeadlineStatus::Elapsed
			&& Elapsed.Receipt.IsValid()
			&& ShieldRuntime.GetState()
				== EShanmenSpiritShieldState::Deactivated
			&& Elapsed.Receipt.GetDeactivation().GetReason()
				== EShanmenSpiritShieldDeactivationReason::DurationElapsed);

	const FShanmenSpiritShieldDeadlineResult Replay =
		Gate.TryElapse(ShieldRuntime, Due);
	TestTrue(TEXT("Exact terminal replay returns the original receipt"),
		Replay.IsSuccess()
			&& Replay.Status
				== EShanmenSpiritShieldDeadlineStatus::AlreadyElapsed
			&& Replay.Receipt.GetReceiptId()
				== Elapsed.Receipt.GetReceiptId());
	const FShanmenSpiritShieldDeadlineResult Conflict =
		Gate.TryElapse(ShieldRuntime, MakeObservation(121));
	TestTrue(TEXT("A second terminal observation cannot rewrite history"),
		Conflict.IsValid() && !Conflict.IsSuccess()
			&& Conflict.Error
				== EShanmenSpiritShieldDeadlineError::DeadlineConflict
			&& Gate.GetElapsedReceipt().GetReceiptId()
				== Elapsed.Receipt.GetReceiptId());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenSpiritShieldDeadlineForeignEvidenceTest,
	"Shanmen.0_0_10.CombatRuntime.SpiritShieldDeadline.ForeignEvidence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenSpiritShieldDeadlineForeignEvidenceTest::RunTest(const FString&)
{
	FShanmenActionOrchestrator ActionRuntime;
	FShanmenSpiritShieldRuntime ShieldRuntime;
	FShanmenSpiritShieldActivationReceipt Activation;
	StartDeadlineShield(ActionRuntime, ShieldRuntime, Activation);
	FShanmenSpiritShieldDeadlineGate Gate;
	check(FShanmenSpiritShieldDeadlineGate::TryCreate(
		MakeDeadlineContract(Activation), Gate));

	const FGuid ForeignTimeline(
		0xD9240001, 0xD9240002, 0xD9240003, 0xD9240004);
	const FShanmenSpiritShieldDeadlineResult WrongTimeline =
		Gate.TryElapse(ShieldRuntime, MakeObservation(120, ForeignTimeline));
	TestTrue(TEXT("A foreign timeline cannot claim deadline completion"),
		WrongTimeline.IsValid() && !WrongTimeline.IsSuccess()
			&& WrongTimeline.Error
				== EShanmenSpiritShieldDeadlineError::TimelineMismatch);

	FShanmenActionOrchestrator ForeignActionRuntime;
	FShanmenSpiritShieldRuntime ForeignShieldRuntime;
	FShanmenSpiritShieldActivationReceipt ForeignActivation;
	StartDeadlineShield(
		ForeignActionRuntime,
		ForeignShieldRuntime,
		ForeignActivation,
		93,
		TEXT("TEST-DIGEST-P9.2-FOREIGN"));
	const FShanmenSpiritShieldDeadlineResult WrongShield =
		Gate.TryElapse(ForeignShieldRuntime, MakeObservation(120));
	TestTrue(TEXT("A deadline contract cannot terminate another shield"),
		WrongShield.IsValid() && !WrongShield.IsSuccess()
			&& WrongShield.Error
				== EShanmenSpiritShieldDeadlineError::ShieldMismatch
			&& ShieldRuntime.GetState() == EShanmenSpiritShieldState::Active
			&& ForeignShieldRuntime.GetState()
				== EShanmenSpiritShieldState::Active);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenSpiritShieldDeadlineLifecycleInterlockTest,
	"Shanmen.0_0_10.CombatRuntime.SpiritShieldDeadline.LifecycleInterlock",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenSpiritShieldDeadlineLifecycleInterlockTest::RunTest(
	const FString&)
{
	FShanmenActionOrchestrator ActionRuntime;
	FShanmenSpiritShieldRuntime ShieldRuntime;
	FShanmenSpiritShieldActivationReceipt Activation;
	StartDeadlineShield(ActionRuntime, ShieldRuntime, Activation);
	FShanmenSpiritShieldDeadlineGate Gate;
	check(FShanmenSpiritShieldDeadlineGate::TryCreate(
		MakeDeadlineContract(Activation), Gate));

	FShanmenSpiritShieldDeactivationReceipt Explicit;
	check(ShieldRuntime.TryDeactivate(
		Activation.GetShieldInstanceId(),
		EShanmenSpiritShieldDeactivationReason::Interrupted,
		Explicit));
	const FShanmenSpiritShieldDeadlineResult Late =
		Gate.TryElapse(ShieldRuntime, MakeObservation(125));
	TestTrue(TEXT("A prior external termination retains its exact reason"),
		Late.IsValid() && !Late.IsSuccess()
			&& Late.Error
				== EShanmenSpiritShieldDeadlineError::ShieldUnavailable
			&& ShieldRuntime.GetDeactivationReceipt().GetReceiptId()
				== Explicit.GetReceiptId()
			&& ShieldRuntime.GetDeactivationReceipt().GetReason()
				== EShanmenSpiritShieldDeactivationReason::Interrupted
			&& !Gate.HasElapsed());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenSpiritShieldDeadlineCapacityCompositionTest,
	"Shanmen.0_0_10.CombatRuntime.SpiritShieldDeadline.CapacityComposition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenSpiritShieldDeadlineCapacityCompositionTest::RunTest(
	const FString&)
{
	FShanmenActionOrchestrator ActionRuntime;
	FShanmenSpiritShieldRuntime ShieldRuntime;
	FShanmenSpiritShieldActivationReceipt Activation;
	StartDeadlineShield(ActionRuntime, ShieldRuntime, Activation);
	FShanmenSpiritShieldCapacityAuthority Capacity;
	check(FShanmenSpiritShieldCapacityAuthority::TryCreate(
		Activation, Capacity));
	FShanmenSpiritShieldProjectionReceipt Projection;
	TestTrue(TEXT("Capacity authority projects before duration termination"),
		Capacity.TryProjectDefenseLayer(ShieldRuntime, Projection)
			&& Projection.IsValid());

	FShanmenSpiritShieldDeadlineGate Gate;
	check(FShanmenSpiritShieldDeadlineGate::TryCreate(
		MakeDeadlineContract(Activation), Gate));
	const FShanmenSpiritShieldDeadlineResult Elapsed =
		Gate.TryElapse(ShieldRuntime, MakeObservation(120));
	FShanmenSpiritShieldProjectionReceipt After;
	TestTrue(TEXT("Duration closes projection without duplicating capacity"),
		Elapsed.IsSuccess()
			&& Capacity.IsValid()
			&& FMath::IsNearlyEqual(Capacity.GetAvailableCapacity(), 40.0f)
			&& !Capacity.TryProjectDefenseLayer(ShieldRuntime, After));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenSpiritShieldDeadlineDeterministicReplayTest,
	"Shanmen.0_0_10.CombatRuntime.SpiritShieldDeadline.DeterministicReplay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenSpiritShieldDeadlineDeterministicReplayTest::RunTest(
	const FString&)
{
	FShanmenActionOrchestrator FirstAction;
	FShanmenSpiritShieldRuntime FirstShield;
	FShanmenSpiritShieldActivationReceipt FirstActivation;
	StartDeadlineShield(FirstAction, FirstShield, FirstActivation);
	FShanmenActionOrchestrator ReplayAction;
	FShanmenSpiritShieldRuntime ReplayShield;
	FShanmenSpiritShieldActivationReceipt ReplayActivation;
	StartDeadlineShield(ReplayAction, ReplayShield, ReplayActivation);

	FShanmenSpiritShieldDeadlineGate FirstGate;
	FShanmenSpiritShieldDeadlineGate ReplayGate;
	check(FShanmenSpiritShieldDeadlineGate::TryCreate(
		MakeDeadlineContract(FirstActivation), FirstGate));
	check(FShanmenSpiritShieldDeadlineGate::TryCreate(
		MakeDeadlineContract(ReplayActivation), ReplayGate));
	const FShanmenSpiritShieldTimelineObservation Observation =
		MakeObservation(120);
	const FShanmenSpiritShieldDeadlineResult First =
		FirstGate.TryElapse(FirstShield, Observation);
	const FShanmenSpiritShieldDeadlineResult Replay =
		ReplayGate.TryElapse(ReplayShield, Observation);
	TestTrue(TEXT("Equivalent frozen input reproduces all deadline evidence"),
		First.IsSuccess() && Replay.IsSuccess()
			&& FirstActivation.GetReceiptId()
				== ReplayActivation.GetReceiptId()
			&& FirstGate.GetContract().GetContractId()
				== ReplayGate.GetContract().GetContractId()
			&& First.Receipt.GetReceiptId()
				== Replay.Receipt.GetReceiptId()
			&& First.Receipt.GetDeactivation().GetReceiptId()
				== Replay.Receipt.GetDeactivation().GetReceiptId());
	return true;
}

#endif
