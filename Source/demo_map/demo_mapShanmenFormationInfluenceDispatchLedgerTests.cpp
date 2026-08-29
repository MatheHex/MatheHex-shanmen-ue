#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenFormationInfluenceDispatchLedger.h"

#include "Misc/AutomationTest.h"
#include "ShanmenDeterministicId.h"

namespace
{
	const FGuid DispatchRunId(0xF8D00001, 0, 0, 1);
	const FGuid DispatchOwnerId(0xF8D00002, 0, 0, 1);
	const FGuid DispatchSourceId(0xF8D00003, 0, 0, 1);

	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	FShanmenContentStamp DispatchContent(const int32 Variant)
	{
		FShanmenContentStamp Content;
		Content.Version = TEXT("0.0.10.P8.13");
		Content.Digest = FString::Printf(
			TEXT("formation-influence-dispatch-r%d"), Variant);
		return Content;
	}

	Fdemo_mapShanmenFormationAnchorPlacementReceipt MakeDispatchAnchor(
		const int32 Variant,
		const int32 Ordinal,
		const FVector& WorldLocation)
	{
		Fdemo_mapShanmenFormationAnchorPlacementReceipt Receipt;
		auto& Intent = Receipt.Intent;
		Intent.RunId = DispatchRunId;
		Intent.OwnerId = DispatchOwnerId;
		Intent.DeploymentId = FGuid(0xF8D00100 + Variant, 0, 0, 1);
		Intent.AnchorDefinitionId = FName(*FString::Printf(
			TEXT("Formation.Anchor.Dispatch.%d.%02d"), Variant, Ordinal));
		Intent.AnchorInstanceId =
			FGuid(0xF8D01000 + Variant * 0x100 + Ordinal, 0, 0, 1);
		Intent.WorldLocation = WorldLocation;
		Intent.AttemptId =
			FGuid(0xF8D02000 + Variant * 0x100 + Ordinal, 0, 0, 1);
		Intent.FulfillmentId =
			FGuid(0xF8D03000 + Variant * 0x100 + Ordinal, 0, 0, 1);
		Intent.DeploymentReceiptId =
			FGuid(0xF8D04000 + Variant * 0x100 + Ordinal, 0, 0, 1);
		Intent.AuthorityRevision = Variant * 100 + Ordinal;
		Intent.Content = DispatchContent(Variant);
		Intent.PlacementId = FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Formation.WorldPlacement.r1"),
			{
				GuidDigits(Intent.RunId), GuidDigits(Intent.OwnerId),
				GuidDigits(Intent.DeploymentId),
				Intent.AnchorDefinitionId.ToString(),
				GuidDigits(Intent.AnchorInstanceId),
				GuidDigits(Intent.AttemptId),
				GuidDigits(Intent.FulfillmentId),
				GuidDigits(Intent.DeploymentReceiptId),
				FString::FromInt(Intent.AuthorityRevision),
				Intent.Content.Version.ToString(), Intent.Content.Digest
			});
		Receipt.ActorClassPath = TEXT("/Script/Engine.Actor");
		Receipt.PlacementTag =
			Fdemo_mapShanmenFormationWorldAdapter::MakePlacementTag(
				Intent.PlacementId);
		Receipt.DeploymentTag =
			Fdemo_mapShanmenFormationWorldAdapter::MakeDeploymentTag(
				Intent.DeploymentId);
		Receipt.ReceiptId = FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Formation.WorldPlacementReceipt.r1"),
			{ GuidDigits(Intent.PlacementId), Receipt.ActorClassPath });
		check(Receipt.IsValid());
		return Receipt;
	}

	Fdemo_mapShanmenFormationAreaSnapshot MakeDispatchArea(
		const int32 Variant = 1)
	{
		const auto Built = Fdemo_mapShanmenFormationAreaProvider::BuildArea({
			MakeDispatchAnchor(Variant, 1, FVector(0.0, 0.0, 10.0)),
			MakeDispatchAnchor(Variant, 2, FVector(100.0, 0.0, 20.0)),
			MakeDispatchAnchor(Variant, 3, FVector(100.0, 100.0, 30.0)),
			MakeDispatchAnchor(Variant, 4, FVector(0.0, 100.0, 40.0))
		});
		check(Built.IsSuccess());
		return Built.Area;
	}

	FGuid DispatchSubject(const int32 Index)
	{
		return FGuid(0xF8D05000 + Index, 0, 0, 1);
	}

	FVector DispatchPoint(
		const Edemo_mapShanmenFormationAreaRelation Relation,
		const int32 Slot)
	{
		switch (Relation)
		{
		case Edemo_mapShanmenFormationAreaRelation::Inside:
			return FVector(25.0 + Slot, 50.0, Slot);
		case Edemo_mapShanmenFormationAreaRelation::Boundary:
			return FVector(0.0, 25.0 + Slot, Slot);
		case Edemo_mapShanmenFormationAreaRelation::Outside:
		default:
			return FVector(-10.0 - Slot, 50.0, Slot);
		}
	}

	Fdemo_mapShanmenFormationCoverageReceipt MakeDispatchCoverage(
		const Fdemo_mapShanmenFormationAreaSnapshot& Area,
		const TArray<FGuid>& Subjects,
		const TArray<Edemo_mapShanmenFormationAreaRelation>& Relations,
		const int32 SlotOffset = 0)
	{
		check(Subjects.Num() == Relations.Num());
		TArray<Fdemo_mapShanmenFormationAreaQuery> Queries;
		for (int32 Index = 0; Index < Subjects.Num(); ++Index)
		{
			Queries.Add({ Subjects[Index],
				DispatchPoint(Relations[Index], SlotOffset + Index) });
		}
		const auto Coverage =
			Fdemo_mapShanmenFormationAreaProvider::EvaluateCoverage(
				Area, Queries);
		check(Coverage.IsSuccess());
		return Coverage.Receipt;
	}

	Fdemo_mapShanmenFormationInfluencePolicy MakeDispatchPolicy(
		const Fdemo_mapShanmenFormationAreaSnapshot& Area)
	{
		Fdemo_mapShanmenFormationInfluencePolicy Policy;
		Policy.PolicyDefinitionId = TEXT("Formation.Policy.Test.Dispatch");
		Policy.InfluenceDefinitionId = TEXT("Formation.Influence.Test.Dispatch");
		Policy.Content = Area.Content;
		check(Policy.IsValid());
		return Policy;
	}

	Fdemo_mapShanmenFormationInfluenceScope MakeDispatchScope(
		const Fdemo_mapShanmenFormationAreaSnapshot& Area,
		const TArray<FGuid>& Subjects,
		const TArray<Edemo_mapShanmenFormationAreaRelation>& Relations)
	{
		Fdemo_mapShanmenFormationInfluenceScope Scope;
		Scope.Area = Area;
		Scope.Policy = MakeDispatchPolicy(Area);
		Scope.Coverage = MakeDispatchCoverage(Area, Subjects, Relations);
		check(Scope.IsValid());
		return Scope;
	}

	Fdemo_mapShanmenFormationInfluenceReconciliationBatch MakePrimeBatch(
		const Fdemo_mapShanmenFormationAreaSnapshot& Area,
		const TArray<FGuid>& Subjects,
		const TArray<Edemo_mapShanmenFormationAreaRelation>& Relations)
	{
		const auto Planned =
			Fdemo_mapShanmenFormationInfluenceReconciliationPlanner::PlanPrime(
				DispatchSourceId, MakeDispatchScope(Area, Subjects, Relations));
		check(Planned.IsSuccess());
		return Planned.Batch;
	}

	Fdemo_mapShanmenFormationInfluenceTransitionBatch MakeTransitionBatch(
		const Fdemo_mapShanmenFormationAreaSnapshot& Area)
	{
		const TArray<FGuid> Subjects = {
			DispatchSubject(1), DispatchSubject(2), DispatchSubject(4) };
		const auto Reduced =
			Fdemo_mapShanmenFormationCoverageTransitionReducer::Reduce(
				MakeDispatchCoverage(
					Area, Subjects,
					{ Edemo_mapShanmenFormationAreaRelation::Outside,
						Edemo_mapShanmenFormationAreaRelation::Inside,
						Edemo_mapShanmenFormationAreaRelation::Outside }),
				MakeDispatchCoverage(
					Area, Subjects,
					{ Edemo_mapShanmenFormationAreaRelation::Inside,
						Edemo_mapShanmenFormationAreaRelation::Outside,
						Edemo_mapShanmenFormationAreaRelation::Outside },
					20));
		check(Reduced.IsSuccess());
		const auto Planned =
			Fdemo_mapShanmenFormationInfluenceIntentPlanner::PlanTransition(
				Area, DispatchSourceId, MakeDispatchPolicy(Area), Reduced.Receipt);
		check(Planned.IsSuccess());
		return Planned.Batch;
	}

	Fdemo_mapShanmenFormationInfluenceAttemptCommand MakeAttempt(
		const FGuid& IntentId,
		const int32 Ordinal,
		const Edemo_mapShanmenFormationInfluenceAttemptOutcome Outcome)
	{
		Fdemo_mapShanmenFormationInfluenceAttemptCommand Command;
		Command.IntentId = IntentId;
		Command.AttemptId = FGuid(0xF8D06000 + Ordinal, 0, 0, 1);
		Command.ExecutorReceiptId = FGuid(0xF8D07000 + Ordinal, 0, 0, 1);
		Command.Outcome = Outcome;
		check(Command.IsValid());
		return Command;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationInfluenceDispatchAcceptTest,
	"Shanmen.0_0_10.Product.FormationInfluenceDispatch.AcceptSourcesAndReplay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationInfluenceDispatchAcceptTest::RunTest(const FString&)
{
	const auto Area = MakeDispatchArea();
	const auto Transition = MakeTransitionBatch(Area);
	const auto Prime = MakePrimeBatch(
		Area, { DispatchSubject(3) },
		{ Edemo_mapShanmenFormationAreaRelation::Boundary });
	Fdemo_mapShanmenFormationInfluenceDispatchLedger Ledger;

	const auto First = Ledger.Accept(Transition);
	const auto Second = Ledger.Accept(Prime);
	const auto Replay = Ledger.Accept(Transition);
	Fdemo_mapShanmenFormationInfluenceIntent Pending;

	TestTrue(TEXT("Transition source batch is accepted"), First.IsSuccess());
	TestEqual(TEXT("Transition contributes two pending intents"),
		First.PendingIntentCount, 2);
	TestTrue(TEXT("Reconciliation source batch is accepted"),
		Second.IsSuccess());
	TestEqual(TEXT("Both source kinds share one deployment ledger"),
		Second.LedgerId, First.LedgerId);
	TestEqual(TEXT("Second source appends canonical pending order"),
		Second.PendingIntentCount, 3);
	TestEqual(TEXT("Exact source replay is explicit"), Replay.Status,
		Edemo_mapShanmenFormationInfluenceSubmitStatus::Replayed);
	TestEqual(TEXT("Replay never duplicates batches"),
		Ledger.GetAcceptedBatchCount(), 2);
	TestEqual(TEXT("Replay never duplicates intents"),
		Ledger.GetIntentCount(), 3);
	TestTrue(TEXT("First transition intent remains queue head"),
		Ledger.TryPeekNextPending(Pending));
	TestEqual(TEXT("Queue head is immutable source order"),
		Pending.IntentId, Transition.Intents[0].IntentId);
	TestTrue(TEXT("Accepted ledger self-validates"), Ledger.IsConsistent());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationInfluenceDispatchRetryTest,
	"Shanmen.0_0_10.Product.FormationInfluenceDispatch.RetryThenSuccess",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationInfluenceDispatchRetryTest::RunTest(const FString&)
{
	const auto Prime = MakePrimeBatch(
		MakeDispatchArea(),
		{ DispatchSubject(1), DispatchSubject(2) },
		{ Edemo_mapShanmenFormationAreaRelation::Inside,
			Edemo_mapShanmenFormationAreaRelation::Boundary });
	Fdemo_mapShanmenFormationInfluenceDispatchLedger Ledger;
	TestTrue(TEXT("Fixture batch accepted"), Ledger.Accept(Prime).IsSuccess());
	const FGuid IntentId = Prime.Intents[0].IntentId;
	const auto Retry = MakeAttempt(
		IntentId, 1,
		Edemo_mapShanmenFormationInfluenceAttemptOutcome::RetryableFailure);
	const auto RetryRecorded = Ledger.Acknowledge(Retry);
	const auto RetryReplayed = Ledger.Acknowledge(Retry);
	auto Conflict = Retry;
	Conflict.ExecutorReceiptId = FGuid(0xF8D07FFF, 0, 0, 1);
	const auto ConflictResult = Ledger.Acknowledge(Conflict);
	const auto Success = MakeAttempt(
		IntentId, 2,
		Edemo_mapShanmenFormationInfluenceAttemptOutcome::Succeeded);
	const auto Acknowledged = Ledger.Acknowledge(Success);
	const auto SuccessReplayed = Ledger.Acknowledge(Success);
	const auto LateAttempt = Ledger.Acknowledge(MakeAttempt(
		IntentId, 3,
		Edemo_mapShanmenFormationInfluenceAttemptOutcome::Succeeded));
	Fdemo_mapShanmenFormationInfluenceAttemptReceipt SuccessfulReceipt;

	TestEqual(TEXT("Retryable failure remains explicit"), RetryRecorded.Status,
		Edemo_mapShanmenFormationInfluenceAcknowledgeStatus::RetryRecorded);
	TestEqual(TEXT("Retry keeps intent pending"),
		RetryRecorded.PendingIntentCount, 2);
	TestEqual(TEXT("Exact retry replay is idempotent"), RetryReplayed.Status,
		Edemo_mapShanmenFormationInfluenceAcknowledgeStatus::RetryReplayed);
	TestEqual(TEXT("Attempt identity cannot change evidence"),
		ConflictResult.Status,
		Edemo_mapShanmenFormationInfluenceAcknowledgeStatus::AttemptConflict);
	TestEqual(TEXT("Later success acknowledges intent"), Acknowledged.Status,
		Edemo_mapShanmenFormationInfluenceAcknowledgeStatus::Acknowledged);
	TestEqual(TEXT("Success removes one pending intent"),
		Acknowledged.PendingIntentCount, 1);
	TestEqual(TEXT("Exact success replay is idempotent"),
		SuccessReplayed.Status,
		Edemo_mapShanmenFormationInfluenceAcknowledgeStatus::AcknowledgementReplayed);
	TestEqual(TEXT("Acknowledged intent rejects another attempt"),
		LateAttempt.Status,
		Edemo_mapShanmenFormationInfluenceAcknowledgeStatus::AlreadyAcknowledged);
	TestTrue(TEXT("Successful receipt remains queryable"),
		Ledger.TryGetSuccessfulReceipt(IntentId, SuccessfulReceipt));
	TestEqual(TEXT("Queried receipt is exact acknowledgement evidence"),
		SuccessfulReceipt.ReceiptId, Acknowledged.Receipt.ReceiptId);
	TestTrue(TEXT("Retry history remains internally valid"),
		Ledger.IsConsistent());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationInfluenceDispatchNoOpSealTest,
	"Shanmen.0_0_10.Product.FormationInfluenceDispatch.NoOpSealAndClosure",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationInfluenceDispatchNoOpSealTest::RunTest(const FString&)
{
	const auto Area = MakeDispatchArea();
	const auto NoOp = MakePrimeBatch(
		Area, { DispatchSubject(9) },
		{ Edemo_mapShanmenFormationAreaRelation::Outside });
	const auto Later = MakeTransitionBatch(Area);
	Fdemo_mapShanmenFormationInfluenceDispatchLedger Empty;
	Fdemo_mapShanmenFormationInfluenceDispatchLedger Ledger;
	const auto EmptySeal = Empty.Seal();
	const auto Accepted = Ledger.Accept(NoOp);
	const auto Sealed = Ledger.Seal();
	const auto SealReplay = Ledger.Seal();
	const auto BatchReplay = Ledger.Accept(NoOp);
	const auto Closed = Ledger.Accept(Later);

	TestTrue(TEXT("Outside-only plan is a valid no-op"), NoOp.IsNoOp());
	TestEqual(TEXT("Empty ledger cannot be sealed"), EmptySeal.Status,
		Edemo_mapShanmenFormationInfluenceSealStatus::LedgerEmpty);
	TestTrue(TEXT("No-op still establishes auditable source batch"),
		Accepted.IsSuccess());
	TestEqual(TEXT("No-op creates no pending intents"),
		Accepted.PendingIntentCount, 0);
	TestEqual(TEXT("No-op ledger seals"), Sealed.Status,
		Edemo_mapShanmenFormationInfluenceSealStatus::Sealed);
	TestEqual(TEXT("Seal replay keeps identity"),
		SealReplay.SealId, Sealed.SealId);
	TestEqual(TEXT("Exact source replay remains legal after seal"),
		BatchReplay.Status,
		Edemo_mapShanmenFormationInfluenceSubmitStatus::Replayed);
	TestEqual(TEXT("New source batch is fenced after seal"), Closed.Status,
		Edemo_mapShanmenFormationInfluenceSubmitStatus::LedgerClosed);
	TestTrue(TEXT("Sealed no-op remains consistent"), Ledger.IsConsistent());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationInfluenceDispatchDeterminismTest,
	"Shanmen.0_0_10.Product.FormationInfluenceDispatch.DeterministicDrainAndScopeFence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationInfluenceDispatchDeterminismTest::RunTest(
	const FString&)
{
	const auto Prime = MakePrimeBatch(
		MakeDispatchArea(),
		{ DispatchSubject(1), DispatchSubject(2) },
		{ Edemo_mapShanmenFormationAreaRelation::Inside,
			Edemo_mapShanmenFormationAreaRelation::Boundary });
	const auto Foreign = MakePrimeBatch(
		MakeDispatchArea(2), { DispatchSubject(7) },
		{ Edemo_mapShanmenFormationAreaRelation::Inside });
	Fdemo_mapShanmenFormationInfluenceDispatchLedger First;
	Fdemo_mapShanmenFormationInfluenceDispatchLedger Replay;
	TestTrue(TEXT("First ledger accepts source"), First.Accept(Prime).IsSuccess());
	TestTrue(TEXT("Replay ledger accepts source"), Replay.Accept(Prime).IsSuccess());
	const auto ScopeFence = First.Accept(Foreign);
	const auto PendingSeal = First.Seal();
	auto InvalidBatch = Prime;
	InvalidBatch.BatchId.Invalidate();
	const auto InvalidResult = First.Accept(InvalidBatch);

	TestEqual(TEXT("Another deployment is fenced"), ScopeFence.Status,
		Edemo_mapShanmenFormationInfluenceSubmitStatus::ScopeMismatch);
	TestEqual(TEXT("Pending ledger cannot seal"), PendingSeal.Status,
		Edemo_mapShanmenFormationInfluenceSealStatus::PendingIntents);
	TestEqual(TEXT("Invalid source batch is rejected"), InvalidResult.Status,
		Edemo_mapShanmenFormationInfluenceSubmitStatus::BatchInvalid);
	for (int32 Index = 0; Index < Prime.Intents.Num(); ++Index)
	{
		const auto Command = MakeAttempt(
			Prime.Intents[Index].IntentId, 10 + Index,
			Edemo_mapShanmenFormationInfluenceAttemptOutcome::Succeeded);
		TestTrue(TEXT("First ledger accepts deterministic success"),
			First.Acknowledge(Command).IsSuccess());
		TestTrue(TEXT("Replay ledger accepts deterministic success"),
			Replay.Acknowledge(Command).IsSuccess());
	}
	const auto FirstSeal = First.Seal();
	const auto ReplaySeal = Replay.Seal();
	Fdemo_mapShanmenFormationInfluenceAttemptReceipt Receipt;
	TestTrue(TEXT("First drain seals"), FirstSeal.IsSuccess());
	TestTrue(TEXT("Replay drain seals"), ReplaySeal.IsSuccess());
	TestEqual(TEXT("Same scope derives same ledger identity"),
		First.GetLedgerId(), Replay.GetLedgerId());
	TestEqual(TEXT("Same drain derives same seal identity"),
		FirstSeal.SealId, ReplaySeal.SealId);
	TestTrue(TEXT("Successful receipt can be recovered"),
		First.TryGetSuccessfulReceipt(Prime.Intents[0].IntentId, Receipt));
	Receipt.ExecutorReceiptId.Invalidate();
	TestFalse(TEXT("Receipt evidence is tamper-evident"), Receipt.IsValid());
	TestTrue(TEXT("Rejected mutations never corrupt source ledger"),
		First.IsConsistent());
	return true;
}

#endif
