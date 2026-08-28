#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ShanmenActionOrchestrator.h"
#include "ShanmenCombatGameplayAbility.h"
#include "ShanmenCombatResolver.h"
#include "ShanmenCombatRuntimeTags.h"

namespace
{
	FShanmenCombatActionSnapshot MakeP30Action()
	{
		FShanmenCombatActionCapture Capture;
		Capture.RunId = FGuid(0x53000001, 0, 0, 1);
		Capture.OwnerId = FGuid(0x53000002, 0, 0, 1);
		Capture.SourceEntityId = FGuid(0x53000003, 0, 0, 1);
		Capture.ActionDefinitionId = TEXT("Combat.Action.Sword.Basic01");
		Capture.Content.Version = TEXT("0.0.10.P3.0");
		Capture.Content.Digest = TEXT("TEST-DIGEST-P3.0");
		Capture.ActivationId = FShanmenCombatIdFactory::MakeActivationId(
			Capture.RunId,
			Capture.SourceEntityId,
			Capture.ActionDefinitionId,
			0);

		FShanmenCombatActionSnapshot Action;
		check(FShanmenCombatActionSnapshot::TryCapture(Capture, Action));
		return Action;
	}

	bool ReceiptsMatch(
		const FShanmenActionTransitionReceipt& A,
		const FShanmenActionTransitionReceipt& B)
	{
		return A.GetActivationId() == B.GetActivationId()
			&& A.GetSequence() == B.GetSequence()
			&& A.GetFromPhase() == B.GetFromPhase()
			&& A.GetToPhase() == B.GetToPhase()
			&& A.GetTerminalReason() == B.GetTerminalReason()
			&& A.CrossedCommitPointNow() == B.CrossedCommitPointNow()
			&& A.HasReachedCommitPoint() == B.HasReachedCommitPoint();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenActionLifecycleTest,
	"Shanmen.0_0_10.CombatRuntime.ActionLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenActionLifecycleTest::RunTest(const FString&)
{
	const FShanmenCombatActionSnapshot Action = MakeP30Action();
	FShanmenActionOrchestrator Runtime;
	FShanmenActionTransitionReceipt Receipt;
	TestTrue(TEXT("Valid frozen action enters Startup"),
		FShanmenActionOrchestrator::TryStart(Action, Runtime, Receipt));
	TestTrue(TEXT("Start receipt is valid"), Receipt.IsValid());
	TestEqual(TEXT("Start sequence is zero"), Receipt.GetSequence(), int64(0));
	TestTrue(TEXT("Idle transitions to Startup"),
		Receipt.GetFromPhase() == EShanmenCombatActionPhase::Idle
			&& Receipt.GetToPhase() == EShanmenCombatActionPhase::Startup);
	TestFalse(TEXT("Startup cannot emit candidates"), Runtime.CanEmitCandidates());
	TestFalse(TEXT("Startup has not committed resources"), Runtime.HasReachedCommitPoint());

	TestFalse(TEXT("Stale expected phase is rejected"),
		Runtime.TryAdvance(EShanmenCombatActionPhase::Active, Receipt));
	TestEqual(TEXT("Rejected transition does not consume sequence"), Runtime.GetNextSequence(), int64(1));
	TestTrue(TEXT("Startup advances to Active"),
		Runtime.TryAdvance(EShanmenCombatActionPhase::Startup, Receipt));
	TestTrue(TEXT("Active entry is the sole commit crossing"),
		Receipt.CrossedCommitPointNow() && Receipt.HasReachedCommitPoint());
	TestTrue(TEXT("Only Active emits candidates"), Runtime.CanEmitCandidates());

	TestTrue(TEXT("Active advances to Recovery"),
		Runtime.TryAdvance(EShanmenCombatActionPhase::Active, Receipt));
	TestFalse(TEXT("Recovery cannot emit candidates"), Runtime.CanEmitCandidates());
	TestTrue(TEXT("Commit fact remains visible in Recovery"), Receipt.HasReachedCommitPoint());
	TestTrue(TEXT("Recovery completes to terminal Idle"),
		Runtime.TryAdvance(EShanmenCombatActionPhase::Recovery, Receipt));
	TestTrue(TEXT("Completion receipt is terminal and valid"),
		Receipt.IsValid()
			&& Receipt.GetTerminalReason() == EShanmenActionTerminalReason::Completed);
	TestTrue(TEXT("Completed runtime is terminal"), Runtime.IsTerminal());
	TestFalse(TEXT("Terminal runtime rejects further transitions"),
		Runtime.TryAdvance(EShanmenCombatActionPhase::Idle, Receipt));
	TestTrue(TEXT("Explicit output reuse safely copies its frozen action before reset"),
		FShanmenActionOrchestrator::TryStart(Runtime.GetAction(), Runtime, Receipt));
	TestTrue(TEXT("Reused output begins a fresh Startup sequence"),
		Runtime.GetPhase() == EShanmenCombatActionPhase::Startup
			&& Receipt.GetSequence() == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenActionCancellationTest,
	"Shanmen.0_0_10.CombatRuntime.CancellationBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenActionCancellationTest::RunTest(const FString&)
{
	FShanmenActionOrchestrator StartupRuntime;
	FShanmenActionTransitionReceipt Receipt;
	TestTrue(TEXT("Startup cancellation fixture begins"),
		FShanmenActionOrchestrator::TryStart(MakeP30Action(), StartupRuntime, Receipt));
	TestTrue(TEXT("Startup may cancel before commit"),
		StartupRuntime.TryCancel(EShanmenCombatActionPhase::Startup, Receipt));
	TestTrue(TEXT("Cancellation preserves uncommitted state"),
		Receipt.GetTerminalReason() == EShanmenActionTerminalReason::Cancelled
			&& !Receipt.HasReachedCommitPoint());

	FShanmenActionOrchestrator ActiveRuntime;
	TestTrue(TEXT("Active interruption fixture begins"),
		FShanmenActionOrchestrator::TryStart(MakeP30Action(), ActiveRuntime, Receipt));
	TestTrue(TEXT("Fixture crosses commit point"),
		ActiveRuntime.TryAdvance(EShanmenCombatActionPhase::Startup, Receipt));
	TestFalse(TEXT("Committed action cannot masquerade as pre-commit cancellation"),
		ActiveRuntime.TryCancel(EShanmenCombatActionPhase::Active, Receipt));
	TestTrue(TEXT("Committed action terminates as interruption"),
		ActiveRuntime.TryInterrupt(EShanmenCombatActionPhase::Active, Receipt));
	TestTrue(TEXT("Interruption receipt preserves commit fact"),
		Receipt.GetTerminalReason() == EShanmenActionTerminalReason::Interrupted
			&& Receipt.HasReachedCommitPoint());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenActionReplayTest,
	"Shanmen.0_0_10.CombatRuntime.DeterministicReplay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenActionReplayTest::RunTest(const FString&)
{
	const FShanmenCombatActionSnapshot Action = MakeP30Action();
	FShanmenActionOrchestrator First;
	FShanmenActionOrchestrator Replay;
	FShanmenActionTransitionReceipt FirstReceipt;
	FShanmenActionTransitionReceipt ReplayReceipt;
	TestTrue(TEXT("First runtime starts"),
		FShanmenActionOrchestrator::TryStart(Action, First, FirstReceipt));
	TestTrue(TEXT("Replay runtime starts"),
		FShanmenActionOrchestrator::TryStart(Action, Replay, ReplayReceipt));
	TestTrue(TEXT("Start receipt replays exactly"), ReceiptsMatch(FirstReceipt, ReplayReceipt));

	const EShanmenCombatActionPhase Phases[] = {
		EShanmenCombatActionPhase::Startup,
		EShanmenCombatActionPhase::Active,
		EShanmenCombatActionPhase::Recovery
	};
	for (const EShanmenCombatActionPhase Phase : Phases)
	{
		TestTrue(TEXT("First runtime advances"), First.TryAdvance(Phase, FirstReceipt));
		TestTrue(TEXT("Replay runtime advances"), Replay.TryAdvance(Phase, ReplayReceipt));
		TestTrue(TEXT("Accepted receipt replays exactly"), ReceiptsMatch(FirstReceipt, ReplayReceipt));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenGameplayAbilityBoundaryTest,
	"Shanmen.0_0_10.CombatRuntime.GameplayAbilityBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenGameplayAbilityBoundaryTest::RunTest(const FString&)
{
	const UClass* AbilityClass = UShanmenCombatGameplayAbility::StaticClass();
	const UShanmenCombatGameplayAbility* AbilityCDO = GetDefault<UShanmenCombatGameplayAbility>();
	TestTrue(TEXT("Base ability is abstract and cannot be granted directly"),
		AbilityClass->HasAnyClassFlags(CLASS_Abstract));
	TestTrue(TEXT("Each activation owns isolated lifecycle state"),
		AbilityCDO->GetInstancingPolicy() == EGameplayAbilityInstancingPolicy::InstancedPerExecution);
	TestTrue(TEXT("GAS asset tag classifies combat action abilities"),
		AbilityCDO->GetAssetTags().HasTagExact(
			FShanmenCombatRuntimeNativeTags::AbilityCombatAction()));
	return true;
}

#endif
