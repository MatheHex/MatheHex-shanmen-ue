#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenFormationCoverageTracker.h"

#include "Misc/AutomationTest.h"
#include "ShanmenDeterministicId.h"

namespace
{
	const FGuid TrackerRunId(0xF8800001, 0, 0, 1);
	const FGuid TrackerOwnerId(0xF8800002, 0, 0, 1);

	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	FShanmenContentStamp TrackerContent(const int32 Variant)
	{
		FShanmenContentStamp Content;
		Content.Version = TEXT("0.0.10.P8.8");
		Content.Digest = FString::Printf(
			TEXT("formation-coverage-tracker-r%d"), Variant);
		return Content;
	}

	Fdemo_mapShanmenFormationAnchorPlacementReceipt MakeTrackerAnchor(
		const int32 Variant,
		const int32 Ordinal,
		const FVector& WorldLocation)
	{
		Fdemo_mapShanmenFormationAnchorPlacementReceipt Receipt;
		Fdemo_mapShanmenFormationAnchorPlacementIntent& Intent = Receipt.Intent;
		Intent.RunId = TrackerRunId;
		Intent.OwnerId = TrackerOwnerId;
		Intent.DeploymentId = FGuid(0xF8800100 + Variant, 0, 0, 1);
		Intent.AnchorDefinitionId = FName(*FString::Printf(
			TEXT("Formation.Anchor.Tracker.%d.%02d"), Variant, Ordinal));
		Intent.AnchorInstanceId =
			FGuid(0xF8801000 + Variant * 0x100 + Ordinal, 0, 0, 1);
		Intent.WorldLocation = WorldLocation;
		Intent.AttemptId =
			FGuid(0xF8802000 + Variant * 0x100 + Ordinal, 0, 0, 1);
		Intent.FulfillmentId =
			FGuid(0xF8803000 + Variant * 0x100 + Ordinal, 0, 0, 1);
		Intent.DeploymentReceiptId =
			FGuid(0xF8804000 + Variant * 0x100 + Ordinal, 0, 0, 1);
		Intent.AuthorityRevision = Variant * 100 + Ordinal;
		Intent.Content = TrackerContent(Variant);
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

	Fdemo_mapShanmenFormationAreaSnapshot MakeTrackerArea(
		const int32 Variant = 1)
	{
		const Fdemo_mapShanmenFormationAreaBuildResult Built =
			Fdemo_mapShanmenFormationAreaProvider::BuildArea({
				MakeTrackerAnchor(Variant, 1, FVector(0.0, 0.0, 10.0)),
				MakeTrackerAnchor(Variant, 2, FVector(100.0, 0.0, 20.0)),
				MakeTrackerAnchor(Variant, 3, FVector(100.0, 100.0, 30.0)),
				MakeTrackerAnchor(Variant, 4, FVector(0.0, 100.0, 40.0))
			});
		check(Built.IsSuccess());
		return Built.Area;
	}

	Fdemo_mapShanmenFormationCoverageReceipt MakeTrackerCoverage(
		const Fdemo_mapShanmenFormationAreaSnapshot& Area,
		const TArray<Fdemo_mapShanmenFormationAreaQuery>& Queries)
	{
		const Fdemo_mapShanmenFormationCoverageResult Coverage =
			Fdemo_mapShanmenFormationAreaProvider::EvaluateCoverage(Area, Queries);
		check(Coverage.IsSuccess());
		return Coverage.Receipt;
	}

	FGuid TrackerSubject(const int32 Index)
	{
		return FGuid(0xF8805000 + Index, 0, 0, 1);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationCoverageTrackerPrimeAdvanceTest,
	"Shanmen.0_0_10.Product.FormationCoverageTracker.PrimeAdvanceReplay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationCoverageTrackerPrimeAdvanceTest::RunTest(const FString&)
{
	const Fdemo_mapShanmenFormationAreaSnapshot Area = MakeTrackerArea();
	const FGuid A = TrackerSubject(1);
	const FGuid B = TrackerSubject(2);
	const Fdemo_mapShanmenFormationCoverageReceipt Baseline =
		MakeTrackerCoverage(Area,
			{ { A, FVector(-10.0, 50.0, 0.0) },
				{ B, FVector(50.0, 50.0, 0.0) } });
	const Fdemo_mapShanmenFormationCoverageReceipt Current =
		MakeTrackerCoverage(Area,
			{ { A, FVector(50.0, 50.0, 0.0) },
				{ B, FVector(110.0, 50.0, 0.0) } });
	Fdemo_mapShanmenFormationCoverageTracker Tracker;
	const auto Primed = Tracker.Prime(Baseline);
	const auto PrimeReplay = Tracker.Prime(Baseline);
	const auto Advanced = Tracker.Advance(Baseline.ReceiptId, Current);
	const auto Replay = Tracker.Advance(Baseline.ReceiptId, Current);
	Fdemo_mapShanmenFormationCoverageReceipt ObservedBaseline;
	const bool bReadBaseline = Tracker.TryGetBaseline(ObservedBaseline);

	TestTrue(TEXT("Prime establishes valid baseline"), Primed.IsSuccess());
	TestEqual(TEXT("Exact Prime replays"), PrimeReplay.Status,
		Edemo_mapShanmenFormationCoverageTrackerStatus::PrimeReplayed);
	TestTrue(TEXT("Advance commits one transition"), Advanced.IsSuccess());
	TestEqual(TEXT("One subject enters"),
		Advanced.Transition.Receipt.EnteredCount, 1);
	TestEqual(TEXT("One subject leaves"),
		Advanced.Transition.Receipt.LeftCount, 1);
	TestEqual(TEXT("Latest exact Advance replays"), Replay.Status,
		Edemo_mapShanmenFormationCoverageTrackerStatus::AdvanceReplayed);
	TestEqual(TEXT("Replay preserves transition identity"),
		Replay.Transition.Receipt.ReceiptId,
		Advanced.Transition.Receipt.ReceiptId);
	TestTrue(TEXT("Tracker remains internally consistent"),
		Tracker.IsConsistent());
	TestTrue(TEXT("Baseline is returned as a value copy"), bReadBaseline);
	TestEqual(TEXT("Baseline moved atomically"),
		ObservedBaseline.ReceiptId, Current.ReceiptId);
	ObservedBaseline.ReceiptId.Invalidate();
	TestTrue(TEXT("Mutating a read copy cannot alter tracker state"),
		Tracker.TryGetBaseline(ObservedBaseline)
			&& ObservedBaseline.ReceiptId == Current.ReceiptId);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationCoverageTrackerAtomicFenceTest,
	"Shanmen.0_0_10.Product.FormationCoverageTracker.AtomicFailureFences",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationCoverageTrackerAtomicFenceTest::RunTest(const FString&)
{
	const Fdemo_mapShanmenFormationAreaSnapshot Area = MakeTrackerArea();
	const Fdemo_mapShanmenFormationAreaSnapshot OtherArea = MakeTrackerArea(2);
	const FGuid A = TrackerSubject(10);
	const FGuid B = TrackerSubject(11);
	const FGuid C = TrackerSubject(12);
	const Fdemo_mapShanmenFormationCoverageReceipt Baseline =
		MakeTrackerCoverage(Area,
			{ { A, FVector(10.0, 50.0, 0.0) },
				{ B, FVector(20.0, 50.0, 0.0) } });
	const Fdemo_mapShanmenFormationCoverageReceipt SubjectMismatch =
		MakeTrackerCoverage(Area,
			{ { A, FVector(10.0, 50.0, 0.0) },
				{ C, FVector(20.0, 50.0, 0.0) } });
	const Fdemo_mapShanmenFormationCoverageReceipt AreaMismatch =
		MakeTrackerCoverage(OtherArea,
			{ { A, FVector(10.0, 50.0, 0.0) },
				{ B, FVector(20.0, 50.0, 0.0) } });
	Fdemo_mapShanmenFormationCoverageReceipt Invalid = Baseline;
	Invalid.ReceiptId.Invalidate();
	Fdemo_mapShanmenFormationCoverageTracker Tracker;
	const auto BeforePrime = Tracker.Advance(Baseline.ReceiptId, Baseline);
	Tracker.Prime(Baseline);
	const auto InvalidExpected = Tracker.Advance(FGuid(), Baseline);
	const auto InvalidCurrent = Tracker.Advance(Baseline.ReceiptId, Invalid);
	const auto Conflict = Tracker.Advance(
		FGuid(0xF8809999, 0, 0, 1), Baseline);
	const auto SubjectRejected = Tracker.Advance(
		Baseline.ReceiptId, SubjectMismatch);
	const auto AreaRejected = Tracker.Advance(
		Baseline.ReceiptId, AreaMismatch);
	Fdemo_mapShanmenFormationCoverageReceipt ObservedBaseline;

	TestEqual(TEXT("Advance before Prime rejected"), BeforePrime.Status,
		Edemo_mapShanmenFormationCoverageTrackerStatus::NotPrimed);
	TestEqual(TEXT("Invalid expected identity rejected"), InvalidExpected.Status,
		Edemo_mapShanmenFormationCoverageTrackerStatus::ExpectedBaselineInvalid);
	TestEqual(TEXT("Invalid current receipt rejected"), InvalidCurrent.Status,
		Edemo_mapShanmenFormationCoverageTrackerStatus::CoverageInvalid);
	TestEqual(TEXT("Stale expected identity rejected"), Conflict.Status,
		Edemo_mapShanmenFormationCoverageTrackerStatus::BaselineConflict);
	TestEqual(TEXT("Subject lifecycle cannot auto-advance"),
		SubjectRejected.Status,
		Edemo_mapShanmenFormationCoverageTrackerStatus::TransitionRejected);
	TestEqual(TEXT("Area lifecycle cannot auto-advance"), AreaRejected.Status,
		Edemo_mapShanmenFormationCoverageTrackerStatus::TransitionRejected);
	TestEqual(TEXT("Every failure preserves baseline"),
		Tracker.TryGetBaseline(ObservedBaseline)
			? ObservedBaseline.ReceiptId : FGuid(), Baseline.ReceiptId);
	TestTrue(TEXT("Failures preserve consistent tracker state"),
		Tracker.IsConsistent());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationCoverageTrackerRebaseResetTest,
	"Shanmen.0_0_10.Product.FormationCoverageTracker.RebaseAndReset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationCoverageTrackerRebaseResetTest::RunTest(const FString&)
{
	const Fdemo_mapShanmenFormationAreaSnapshot FirstArea = MakeTrackerArea();
	const Fdemo_mapShanmenFormationAreaSnapshot SecondArea = MakeTrackerArea(2);
	const FGuid A = TrackerSubject(20);
	const FGuid B = TrackerSubject(21);
	const Fdemo_mapShanmenFormationCoverageReceipt First =
		MakeTrackerCoverage(FirstArea,
			{ { A, FVector(10.0, 50.0, 0.0) } });
	const Fdemo_mapShanmenFormationCoverageReceipt Second =
		MakeTrackerCoverage(SecondArea,
			{ { B, FVector(-10.0, 50.0, 0.0) } });
	const Fdemo_mapShanmenFormationCoverageReceipt Third =
		MakeTrackerCoverage(SecondArea,
			{ { B, FVector(50.0, 50.0, 0.0) } });
	Fdemo_mapShanmenFormationCoverageTracker Tracker;
	const auto EarlyRebase = Tracker.Rebase(Second);
	Tracker.Prime(First);
	const auto Rebased = Tracker.Rebase(Second);
	const auto Advanced = Tracker.Advance(Second.ReceiptId, Third);
	const auto Cleared = Tracker.Reset();
	const auto ClearReplay = Tracker.Reset();
	const auto AfterReset = Tracker.Advance(Third.ReceiptId, Third);
	Fdemo_mapShanmenFormationCoverageReceipt ObservedBaseline;

	TestEqual(TEXT("Rebase requires existing baseline"), EarlyRebase.Status,
		Edemo_mapShanmenFormationCoverageTrackerStatus::NotPrimed);
	TestTrue(TEXT("Explicit cross-Area Rebase succeeds"), Rebased.IsSuccess());
	TestTrue(TEXT("Rebased baseline can Advance"), Advanced.IsSuccess());
	TestTrue(TEXT("Reset clears active state"), Cleared.IsSuccess());
	TestTrue(TEXT("Reset is idempotent"), ClearReplay.IsSuccess());
	TestFalse(TEXT("No baseline remains"), Tracker.IsPrimed());
	TestFalse(TEXT("No baseline snapshot remains"),
		Tracker.TryGetBaseline(ObservedBaseline));
	TestEqual(TEXT("Advance after Reset rejected"), AfterReset.Status,
		Edemo_mapShanmenFormationCoverageTrackerStatus::NotPrimed);
	TestTrue(TEXT("Cleared tracker is consistent"), Tracker.IsConsistent());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationCoverageTrackerSequentialCasTest,
	"Shanmen.0_0_10.Product.FormationCoverageTracker.SequentialCAS",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationCoverageTrackerSequentialCasTest::RunTest(const FString&)
{
	const Fdemo_mapShanmenFormationAreaSnapshot Area = MakeTrackerArea();
	const FGuid A = TrackerSubject(30);
	const Fdemo_mapShanmenFormationCoverageReceipt First =
		MakeTrackerCoverage(Area, { { A, FVector(-10.0, 50.0, 0.0) } });
	const Fdemo_mapShanmenFormationCoverageReceipt Second =
		MakeTrackerCoverage(Area, { { A, FVector(10.0, 50.0, 0.0) } });
	const Fdemo_mapShanmenFormationCoverageReceipt Third =
		MakeTrackerCoverage(Area, { { A, FVector(110.0, 50.0, 0.0) } });
	Fdemo_mapShanmenFormationCoverageTracker Tracker;
	Tracker.Prime(First);
	const auto FirstAdvance = Tracker.Advance(First.ReceiptId, Second);
	const auto SecondAdvance = Tracker.Advance(Second.ReceiptId, Third);
	const auto OldRetry = Tracker.Advance(First.ReceiptId, Second);
	const auto LatestRetry = Tracker.Advance(Second.ReceiptId, Third);
	Fdemo_mapShanmenFormationCoverageReceipt ObservedBaseline;

	TestTrue(TEXT("First CAS Advance succeeds"), FirstAdvance.IsSuccess());
	TestTrue(TEXT("Second CAS Advance succeeds"), SecondAdvance.IsSuccess());
	TestEqual(TEXT("Older retry is outside one-step replay window"),
		OldRetry.Status,
		Edemo_mapShanmenFormationCoverageTrackerStatus::BaselineConflict);
	TestEqual(TEXT("Latest retry remains replayable"), LatestRetry.Status,
		Edemo_mapShanmenFormationCoverageTrackerStatus::AdvanceReplayed);
	TestEqual(TEXT("Latest replay preserves transition identity"),
		LatestRetry.Transition.Receipt.ReceiptId,
		SecondAdvance.Transition.Receipt.ReceiptId);
	TestEqual(TEXT("Sequential baseline ends at third receipt"),
		Tracker.TryGetBaseline(ObservedBaseline)
			? ObservedBaseline.ReceiptId : FGuid(), Third.ReceiptId);
	TestTrue(TEXT("Sequential tracker remains consistent"),
		Tracker.IsConsistent());
	return true;
}

#endif
