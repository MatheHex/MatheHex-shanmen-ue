#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ShanmenActionResourceAuthority.h"
#include "ShanmenCombatResolver.h"
#include "ShanmenCombatRuntimeTags.h"

namespace
{
	const FGuid ResourceRunId(
		0xA9300001, 0xA9300002, 0xA9300003, 0xA9300004);
	const FGuid ResourceOwnerId(
		0xA9310001, 0xA9310002, 0xA9310003, 0xA9310004);
	const FGuid ResourceSourceId(
		0xA9320001, 0xA9320002, 0xA9320003, 0xA9320004);
	const FGuid ForeignSourceId(
		0xA9330001, 0xA9330002, 0xA9330003, 0xA9330004);

	FShanmenCombatActionSnapshot MakeResourceAction(
		uint64 ActivationSequence = 93,
		const FGuid& SourceEntityId = ResourceSourceId,
		const FString& Digest = TEXT("TEST-DIGEST-P9.3"))
	{
		FShanmenCombatActionCapture Capture;
		Capture.RunId = ResourceRunId;
		Capture.OwnerId = ResourceOwnerId;
		Capture.SourceEntityId = SourceEntityId;
		Capture.ActionDefinitionId = TEXT("Spell.Test.ResourceBacked");
		Capture.Content.Version = TEXT("0.0.10.P9.3");
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

	void StartResourceAction(
		const FShanmenCombatActionSnapshot& Action,
		FShanmenActionOrchestrator& OutRuntime,
		FShanmenActionTransitionReceipt& OutStartup)
	{
		check(FShanmenActionOrchestrator::TryStart(
			Action, OutRuntime, OutStartup));
	}

	FShanmenActionResourceCost MakeResourceCost(
		float Amount = 25.0f,
		FName RuleId = TEXT("Cost.Spell.Test.ResourceBacked"),
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

	FShanmenActionResourceAuthority MakeResourceAuthority(
		const FGuid& OwnerEntityId = ResourceSourceId,
		FGameplayTag Channel =
			FShanmenCombatRuntimeNativeTags::ResourceSpiritEnergy(),
		float CurrentAmount = 100.0f,
		float MaximumAmount = 120.0f,
		int64 Revision = 0)
	{
		FShanmenActionResourceAuthority Authority;
		check(FShanmenActionResourceAuthority::TryCreate(
			OwnerEntityId,
			Channel,
			CurrentAmount,
			MaximumAmount,
			Revision,
			Authority));
		return Authority;
	}

	FShanmenActionResourceReservationRequest MakeReservationRequest(
		const FShanmenCombatActionSnapshot& Action,
		const FShanmenActionTransitionReceipt& Startup,
		const FShanmenActionResourceCost& Cost,
		const FShanmenActionResourceAuthority& Authority)
	{
		FShanmenActionResourceSnapshot Snapshot;
		check(Authority.TryCaptureSnapshot(Snapshot));
		FShanmenActionResourceReservationRequest Request;
		check(FShanmenActionResourceReservationRequest::TryCreate(
			Action, Startup, Cost, Snapshot, Request));
		return Request;
	}

	FShanmenActionResourceFinalizationRequest MakeFinalizationRequest(
		const FShanmenActionResourceReservationReceipt& Reservation,
		const FShanmenActionTransitionReceipt& Transition)
	{
		FShanmenActionResourceFinalizationRequest Request;
		check(FShanmenActionResourceFinalizationRequest::TryCreate(
			Reservation, Transition, Request));
		return Request;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenActionResourceContractTest,
	"Shanmen.0_0_10.CombatRuntime.ActionResource.ContractAndSnapshot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenActionResourceContractTest::RunTest(const FString&)
{
	const FGameplayTag Channel =
		FShanmenCombatRuntimeNativeTags::ResourceSpiritEnergy();
	TestTrue(TEXT("Spirit energy has one typed native channel"),
		Channel.IsValid()
			&& Channel.MatchesTag(
				FShanmenCombatRuntimeNativeTags::Resource()));

	const FShanmenActionResourceCost Cost = MakeResourceCost();
	const FShanmenActionResourceCost ReplayCost = MakeResourceCost();
	TestTrue(TEXT("Equivalent authored cost has one deterministic identity"),
		Cost.IsValid() && Cost.GetCostId() == ReplayCost.GetCostId()
			&& FMath::IsNearlyEqual(Cost.GetAmount(), 25.0f));

	FShanmenActionResourceCostCapture InvalidCapture;
	InvalidCapture.RuleId = TEXT("Cost.Invalid.Zero");
	InvalidCapture.ResourceChannel = Channel;
	InvalidCapture.Amount = 0.0f;
	FShanmenActionResourceCost InvalidCost;
	TestFalse(TEXT("Free actions omit a cost instead of reserving zero"),
		FShanmenActionResourceCost::TryCapture(
			InvalidCapture, InvalidCost));

	const FShanmenActionResourceAuthority Authority =
		MakeResourceAuthority(ResourceSourceId, Channel, 90.0f, 120.0f, 7);
	FShanmenActionResourceSnapshot Snapshot;
	TestTrue(TEXT("Authority exposes one immutable available-balance snapshot"),
		Authority.TryCaptureSnapshot(Snapshot)
			&& Snapshot.IsValid()
			&& Snapshot.GetAuthorityRevision() == 7
			&& FMath::IsNearlyEqual(Snapshot.GetCurrentAmount(), 90.0f)
			&& FMath::IsNearlyZero(Snapshot.GetReservedAmount())
			&& FMath::IsNearlyEqual(Snapshot.GetAvailableAmount(), 90.0f));

	const FShanmenCombatActionSnapshot Action = MakeResourceAction();
	FShanmenActionOrchestrator FirstRuntime;
	FShanmenActionTransitionReceipt FirstStartup;
	StartResourceAction(Action, FirstRuntime, FirstStartup);
	FShanmenActionOrchestrator ReplayRuntime;
	FShanmenActionTransitionReceipt ReplayStartup;
	StartResourceAction(Action, ReplayRuntime, ReplayStartup);
	FShanmenActionResourceReservationRequest First;
	FShanmenActionResourceReservationRequest Replay;
	check(FShanmenActionResourceReservationRequest::TryCreate(
		Action, FirstStartup, Cost, Snapshot, First));
	check(FShanmenActionResourceReservationRequest::TryCreate(
		Action, ReplayStartup, ReplayCost, Snapshot, Replay));
	TestTrue(TEXT("Equivalent startup evidence reproduces reservation command"),
		First.IsValid() && Replay.IsValid()
			&& First.GetReservationId() == Replay.GetReservationId()
			&& First.GetCommandId() == Replay.GetCommandId());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenActionResourceReserveCommitTest,
	"Shanmen.0_0_10.CombatRuntime.ActionResource.ReserveAndCommit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenActionResourceReserveCommitTest::RunTest(const FString&)
{
	const FShanmenCombatActionSnapshot Action = MakeResourceAction();
	FShanmenActionOrchestrator Runtime;
	FShanmenActionTransitionReceipt Startup;
	StartResourceAction(Action, Runtime, Startup);
	FShanmenActionResourceAuthority Authority = MakeResourceAuthority();
	const FShanmenActionResourceReservationRequest ReserveRequest =
		MakeReservationRequest(Action, Startup, MakeResourceCost(), Authority);
	const FShanmenActionResourceTransactionResult Reserved =
		Authority.Reserve(ReserveRequest);
	TestTrue(TEXT("Startup reservation hides cost from competing actions"),
		Reserved.IsSuccess()
			&& Reserved.Status
				== EShanmenActionResourceTransactionStatus::Reserved
			&& Reserved.Reservation.IsValid()
			&& Authority.GetAuthorityRevision() == 1
			&& FMath::IsNearlyEqual(Authority.GetCurrentAmount(), 100.0f)
			&& FMath::IsNearlyEqual(Authority.GetReservedAmount(), 25.0f)
			&& FMath::IsNearlyEqual(Authority.GetAvailableAmount(), 75.0f));

	FShanmenActionTransitionReceipt CommitTransition;
	check(Runtime.TryAdvance(
		EShanmenCombatActionPhase::Startup, CommitTransition));
	const FShanmenActionResourceTransactionResult Committed =
		Authority.Finalize(MakeFinalizationRequest(
			Reserved.Reservation, CommitTransition));
	TestTrue(TEXT("Exact action commit point consumes the reserved amount once"),
		Committed.IsSuccess()
			&& Committed.Status
				== EShanmenActionResourceTransactionStatus::Committed
			&& Committed.Finalization.IsValid()
			&& Committed.Finalization.GetRequest().GetDisposition()
				== EShanmenActionResourceDisposition::Commit
			&& Authority.GetAuthorityRevision() == 2
			&& FMath::IsNearlyEqual(Authority.GetCurrentAmount(), 75.0f)
			&& FMath::IsNearlyZero(Authority.GetReservedAmount())
			&& FMath::IsNearlyEqual(Authority.GetAvailableAmount(), 75.0f)
			&& Authority.NumPendingReservations() == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenActionResourceCancelReleaseTest,
	"Shanmen.0_0_10.CombatRuntime.ActionResource.CancelReleases",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenActionResourceCancelReleaseTest::RunTest(const FString&)
{
	const FShanmenCombatActionSnapshot Action = MakeResourceAction();
	FShanmenActionOrchestrator Runtime;
	FShanmenActionTransitionReceipt Startup;
	StartResourceAction(Action, Runtime, Startup);
	FShanmenActionResourceAuthority Authority = MakeResourceAuthority();
	const auto Reserved = Authority.Reserve(
		MakeReservationRequest(Action, Startup, MakeResourceCost(), Authority));
	check(Reserved.IsSuccess());

	FShanmenActionTransitionReceipt CancelTransition;
	check(Runtime.TryCancel(
		EShanmenCombatActionPhase::Startup, CancelTransition));
	const auto Released = Authority.Finalize(
		MakeFinalizationRequest(Reserved.Reservation, CancelTransition));
	TestTrue(TEXT("A pre-commit terminal action releases without spending"),
		Released.IsSuccess()
			&& Released.Status
				== EShanmenActionResourceTransactionStatus::Released
			&& Released.Finalization.GetRequest().GetDisposition()
				== EShanmenActionResourceDisposition::Release
			&& FMath::IsNearlyEqual(Authority.GetCurrentAmount(), 100.0f)
			&& FMath::IsNearlyZero(Authority.GetReservedAmount())
			&& FMath::IsNearlyEqual(Authority.GetAvailableAmount(), 100.0f)
			&& Authority.GetAuthorityRevision() == 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenActionResourceReplayConflictTest,
	"Shanmen.0_0_10.CombatRuntime.ActionResource.ReplayAndConflict",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenActionResourceReplayConflictTest::RunTest(const FString&)
{
	const FShanmenCombatActionSnapshot Action = MakeResourceAction();
	FShanmenActionOrchestrator CommitRuntime;
	FShanmenActionTransitionReceipt Startup;
	StartResourceAction(Action, CommitRuntime, Startup);
	FShanmenActionResourceAuthority Authority = MakeResourceAuthority();
	FShanmenActionResourceSnapshot InitialSnapshot;
	check(Authority.TryCaptureSnapshot(InitialSnapshot));
	FShanmenActionResourceReservationRequest Request;
	check(FShanmenActionResourceReservationRequest::TryCreate(
		Action, Startup, MakeResourceCost(), InitialSnapshot, Request));
	const auto FirstReserve = Authority.Reserve(Request);
	const auto ReserveReplay = Authority.Reserve(Request);
	TestTrue(TEXT("Exact reserve replay returns original proof without revision drift"),
		FirstReserve.IsSuccess() && ReserveReplay.IsSuccess()
			&& ReserveReplay.Status
				== EShanmenActionResourceTransactionStatus::AlreadyReserved
			&& ReserveReplay.Reservation.GetReceiptId()
				== FirstReserve.Reservation.GetReceiptId()
			&& Authority.GetAuthorityRevision() == 1);

	FShanmenActionResourceReservationRequest ConflictingRequest;
	check(FShanmenActionResourceReservationRequest::TryCreate(
		Action, Startup, MakeResourceCost(30.0f), InitialSnapshot,
		ConflictingRequest));
	const auto ReserveConflict = Authority.Reserve(ConflictingRequest);
	TestTrue(TEXT("Same action and channel cannot rewrite its reserved cost"),
		ReserveConflict.IsValid() && !ReserveConflict.IsSuccess()
			&& ReserveConflict.Error
				== EShanmenActionResourceTransactionError::ReservationConflict
			&& Authority.GetAuthorityRevision() == 1);

	FShanmenActionTransitionReceipt CommitTransition;
	check(CommitRuntime.TryAdvance(
		EShanmenCombatActionPhase::Startup, CommitTransition));
	const FShanmenActionResourceFinalizationRequest CommitRequest =
		MakeFinalizationRequest(FirstReserve.Reservation, CommitTransition);
	const auto FirstCommit = Authority.Finalize(CommitRequest);
	const auto CommitReplay = Authority.Finalize(CommitRequest);
	TestTrue(TEXT("Exact finalization replay never spends twice"),
		FirstCommit.IsSuccess() && CommitReplay.IsSuccess()
			&& CommitReplay.Status
				== EShanmenActionResourceTransactionStatus::AlreadyFinalized
			&& CommitReplay.Finalization.GetReceiptId()
				== FirstCommit.Finalization.GetReceiptId()
			&& Authority.GetAuthorityRevision() == 2
			&& FMath::IsNearlyEqual(Authority.GetCurrentAmount(), 75.0f));

	FShanmenActionOrchestrator CancelRuntime;
	FShanmenActionTransitionReceipt ReplayStartup;
	StartResourceAction(Action, CancelRuntime, ReplayStartup);
	FShanmenActionTransitionReceipt CancelTransition;
	check(CancelRuntime.TryCancel(
		EShanmenCombatActionPhase::Startup, CancelTransition));
	const auto FinalConflict = Authority.Finalize(
		MakeFinalizationRequest(FirstReserve.Reservation, CancelTransition));
	TestTrue(TEXT("A committed reservation cannot be reclassified as released"),
		FinalConflict.IsValid() && !FinalConflict.IsSuccess()
			&& FinalConflict.Error
				== EShanmenActionResourceTransactionError::FinalizationConflict
			&& FMath::IsNearlyEqual(Authority.GetCurrentAmount(), 75.0f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenActionResourceFailClosedTest,
	"Shanmen.0_0_10.CombatRuntime.ActionResource.FailClosedProofs",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenActionResourceFailClosedTest::RunTest(const FString&)
{
	FShanmenActionResourceAuthority Insufficient =
		MakeResourceAuthority(ResourceSourceId,
			FShanmenCombatRuntimeNativeTags::ResourceSpiritEnergy(),
			10.0f, 10.0f);
	const FShanmenCombatActionSnapshot Action = MakeResourceAction();
	FShanmenActionOrchestrator Runtime;
	FShanmenActionTransitionReceipt Startup;
	StartResourceAction(Action, Runtime, Startup);
	const auto NoCapacity = Insufficient.Reserve(
		MakeReservationRequest(Action, Startup, MakeResourceCost(25.0f),
			Insufficient));
	TestTrue(TEXT("Insufficient available balance is atomic"),
		NoCapacity.IsValid() && !NoCapacity.IsSuccess()
			&& NoCapacity.Error
				== EShanmenActionResourceTransactionError::InsufficientAvailable
			&& Insufficient.GetAuthorityRevision() == 0
			&& FMath::IsNearlyEqual(Insufficient.GetCurrentAmount(), 10.0f)
			&& FMath::IsNearlyZero(Insufficient.GetReservedAmount()));

	FShanmenActionResourceAuthority Authority = MakeResourceAuthority();
	const FShanmenCombatActionSnapshot StaleAction =
		MakeResourceAction(94, ResourceSourceId, TEXT("TEST-DIGEST-P9.3-STALE"));
	FShanmenActionOrchestrator StaleRuntime;
	FShanmenActionTransitionReceipt StaleStartup;
	StartResourceAction(StaleAction, StaleRuntime, StaleStartup);
	const FShanmenActionResourceReservationRequest StaleRequest =
		MakeReservationRequest(
			StaleAction, StaleStartup, MakeResourceCost(10.0f), Authority);
	const auto First = Authority.Reserve(
		MakeReservationRequest(Action, Startup, MakeResourceCost(), Authority));
	check(First.IsSuccess());
	const auto Stale = Authority.Reserve(StaleRequest);
	TestTrue(TEXT("A stale snapshot cannot reserve remaining balance"),
		Stale.IsValid() && !Stale.IsSuccess()
			&& Stale.Error
				== EShanmenActionResourceTransactionError::StaleSnapshot
			&& Authority.GetAuthorityRevision() == 1
			&& Authority.NumPendingReservations() == 1);

	const FGameplayTag Spirit =
		FShanmenCombatRuntimeNativeTags::ResourceSpiritEnergy();
	FShanmenActionResourceAuthority ForeignOwnerAuthority =
		MakeResourceAuthority(ForeignSourceId, Spirit);
	const FShanmenCombatActionSnapshot ForeignAction =
		MakeResourceAction(95, ForeignSourceId,
			TEXT("TEST-DIGEST-P9.3-FOREIGN"));
	FShanmenActionOrchestrator ForeignRuntime;
	FShanmenActionTransitionReceipt ForeignStartup;
	StartResourceAction(ForeignAction, ForeignRuntime, ForeignStartup);
	const auto ForeignRequest = MakeReservationRequest(
		ForeignAction, ForeignStartup, MakeResourceCost(),
		ForeignOwnerAuthority);
	const auto WrongOwner = Authority.Reserve(ForeignRequest);
	TestTrue(TEXT("Foreign owner proof cannot mutate this authority"),
		WrongOwner.IsValid() && !WrongOwner.IsSuccess()
			&& WrongOwner.Error
				== EShanmenActionResourceTransactionError::OwnerMismatch);

	const FGameplayTag Root = FShanmenCombatRuntimeNativeTags::Resource();
	FShanmenActionResourceAuthority RootAuthority =
		MakeResourceAuthority(ResourceSourceId, Root);
	const FShanmenCombatActionSnapshot RootAction =
		MakeResourceAction(96, ResourceSourceId,
			TEXT("TEST-DIGEST-P9.3-CHANNEL"));
	FShanmenActionOrchestrator RootRuntime;
	FShanmenActionTransitionReceipt RootStartup;
	StartResourceAction(RootAction, RootRuntime, RootStartup);
	const auto RootRequest = MakeReservationRequest(
		RootAction,
		RootStartup,
		MakeResourceCost(5.0f, TEXT("Cost.Root"), Root),
		RootAuthority);
	const auto WrongChannel = Authority.Reserve(RootRequest);
	TestTrue(TEXT("A different resource channel cannot alias spirit energy"),
		WrongChannel.IsValid() && !WrongChannel.IsSuccess()
			&& WrongChannel.Error
				== EShanmenActionResourceTransactionError::ChannelMismatch);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenActionResourceMultipleReservationsTest,
	"Shanmen.0_0_10.CombatRuntime.ActionResource.MultipleReservations",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenActionResourceMultipleReservationsTest::RunTest(const FString&)
{
	FShanmenActionResourceAuthority Authority = MakeResourceAuthority();
	const FShanmenCombatActionSnapshot FirstAction = MakeResourceAction(101);
	FShanmenActionOrchestrator FirstRuntime;
	FShanmenActionTransitionReceipt FirstStartup;
	StartResourceAction(FirstAction, FirstRuntime, FirstStartup);
	const auto First = Authority.Reserve(MakeReservationRequest(
		FirstAction, FirstStartup, MakeResourceCost(30.0f), Authority));
	check(First.IsSuccess());

	const FShanmenCombatActionSnapshot SecondAction = MakeResourceAction(
		102, ResourceSourceId, TEXT("TEST-DIGEST-P9.3-SECOND"));
	FShanmenActionOrchestrator SecondRuntime;
	FShanmenActionTransitionReceipt SecondStartup;
	StartResourceAction(SecondAction, SecondRuntime, SecondStartup);
	const auto Second = Authority.Reserve(MakeReservationRequest(
		SecondAction, SecondStartup, MakeResourceCost(40.0f), Authority));
	TestTrue(TEXT("Independent reservations share one available-balance owner"),
		First.IsSuccess() && Second.IsSuccess()
			&& Authority.NumPendingReservations() == 2
			&& Authority.GetAuthorityRevision() == 2
			&& FMath::IsNearlyEqual(Authority.GetReservedAmount(), 70.0f)
			&& FMath::IsNearlyEqual(Authority.GetAvailableAmount(), 30.0f));

	FShanmenActionTransitionReceipt FirstCommit;
	check(FirstRuntime.TryAdvance(
		EShanmenCombatActionPhase::Startup, FirstCommit));
	const auto Committed = Authority.Finalize(
		MakeFinalizationRequest(First.Reservation, FirstCommit));
	FShanmenActionTransitionReceipt SecondCancel;
	check(SecondRuntime.TryCancel(
		EShanmenCombatActionPhase::Startup, SecondCancel));
	const auto Released = Authority.Finalize(
		MakeFinalizationRequest(Second.Reservation, SecondCancel));
	TestTrue(TEXT("Commit and release compose without balance drift"),
		Committed.IsSuccess() && Released.IsSuccess()
			&& Authority.IsValid()
			&& Authority.GetAuthorityRevision() == 4
			&& FMath::IsNearlyEqual(Authority.GetCurrentAmount(), 70.0f)
			&& FMath::IsNearlyZero(Authority.GetReservedAmount())
			&& FMath::IsNearlyEqual(Authority.GetAvailableAmount(), 70.0f)
			&& Authority.NumTransactions() == 2
			&& Authority.NumPendingReservations() == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenActionResourceDeterministicReplayTest,
	"Shanmen.0_0_10.CombatRuntime.ActionResource.DeterministicReplay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenActionResourceDeterministicReplayTest::RunTest(const FString&)
{
	const FShanmenCombatActionSnapshot Action = MakeResourceAction(110);
	FShanmenActionOrchestrator FirstRuntime;
	FShanmenActionTransitionReceipt FirstStartup;
	StartResourceAction(Action, FirstRuntime, FirstStartup);
	FShanmenActionOrchestrator ReplayRuntime;
	FShanmenActionTransitionReceipt ReplayStartup;
	StartResourceAction(Action, ReplayRuntime, ReplayStartup);
	FShanmenActionResourceAuthority FirstAuthority = MakeResourceAuthority();
	FShanmenActionResourceAuthority ReplayAuthority = MakeResourceAuthority();
	const auto FirstReserve = FirstAuthority.Reserve(MakeReservationRequest(
		Action, FirstStartup, MakeResourceCost(), FirstAuthority));
	const auto ReplayReserve = ReplayAuthority.Reserve(MakeReservationRequest(
		Action, ReplayStartup, MakeResourceCost(), ReplayAuthority));
	check(FirstReserve.IsSuccess() && ReplayReserve.IsSuccess());

	FShanmenActionTransitionReceipt FirstCommit;
	FShanmenActionTransitionReceipt ReplayCommit;
	check(FirstRuntime.TryAdvance(
		EShanmenCombatActionPhase::Startup, FirstCommit));
	check(ReplayRuntime.TryAdvance(
		EShanmenCombatActionPhase::Startup, ReplayCommit));
	const auto FirstFinal = FirstAuthority.Finalize(
		MakeFinalizationRequest(FirstReserve.Reservation, FirstCommit));
	const auto ReplayFinal = ReplayAuthority.Finalize(
		MakeFinalizationRequest(ReplayReserve.Reservation, ReplayCommit));
	TestTrue(TEXT("Equivalent frozen inputs reproduce every transaction identity"),
		FirstFinal.IsSuccess() && ReplayFinal.IsSuccess()
			&& FirstReserve.Reservation.GetReservationId()
				== ReplayReserve.Reservation.GetReservationId()
			&& FirstReserve.Reservation.GetReceiptId()
				== ReplayReserve.Reservation.GetReceiptId()
			&& FirstFinal.Finalization.GetRequest().GetCommandId()
				== ReplayFinal.Finalization.GetRequest().GetCommandId()
			&& FirstFinal.Finalization.GetReceiptId()
				== ReplayFinal.Finalization.GetReceiptId()
			&& FMath::IsNearlyEqual(
				FirstAuthority.GetCurrentAmount(),
				ReplayAuthority.GetCurrentAmount())
			&& FirstAuthority.GetAuthorityRevision()
				== ReplayAuthority.GetAuthorityRevision());
	return true;
}

#endif
