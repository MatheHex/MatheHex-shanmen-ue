#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenFormationCoverageTransitionReducer.h"

#include "Misc/AutomationTest.h"
#include "ShanmenDeterministicId.h"

namespace
{
	const FGuid TransitionRunId(0xF8700001, 0, 0, 1);
	const FGuid TransitionOwnerId(0xF8700002, 0, 0, 1);

	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	FShanmenContentStamp TransitionContent(const int32 Variant)
	{
		FShanmenContentStamp Content;
		Content.Version = TEXT("0.0.10.P8.7");
		Content.Digest = FString::Printf(
			TEXT("formation-coverage-transition-r%d"), Variant);
		return Content;
	}

	Fdemo_mapShanmenFormationAnchorPlacementReceipt MakeTransitionAnchor(
		const int32 Variant,
		const int32 Ordinal,
		const FVector& WorldLocation)
	{
		Fdemo_mapShanmenFormationAnchorPlacementReceipt Receipt;
		Fdemo_mapShanmenFormationAnchorPlacementIntent& Intent = Receipt.Intent;
		Intent.RunId = TransitionRunId;
		Intent.OwnerId = TransitionOwnerId;
		Intent.DeploymentId = FGuid(0xF8700100 + Variant, 0, 0, 1);
		Intent.AnchorDefinitionId = FName(*FString::Printf(
			TEXT("Formation.Anchor.Transition.%d.%02d"), Variant, Ordinal));
		Intent.AnchorInstanceId =
			FGuid(0xF8701000 + Variant * 0x100 + Ordinal, 0, 0, 1);
		Intent.WorldLocation = WorldLocation;
		Intent.AttemptId =
			FGuid(0xF8702000 + Variant * 0x100 + Ordinal, 0, 0, 1);
		Intent.FulfillmentId =
			FGuid(0xF8703000 + Variant * 0x100 + Ordinal, 0, 0, 1);
		Intent.DeploymentReceiptId =
			FGuid(0xF8704000 + Variant * 0x100 + Ordinal, 0, 0, 1);
		Intent.AuthorityRevision = Variant * 100 + Ordinal;
		Intent.Content = TransitionContent(Variant);
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

	Fdemo_mapShanmenFormationAreaSnapshot MakeTransitionArea(
		const int32 Variant = 1)
	{
		const Fdemo_mapShanmenFormationAreaBuildResult Built =
			Fdemo_mapShanmenFormationAreaProvider::BuildArea({
				MakeTransitionAnchor(Variant, 1, FVector(0.0, 0.0, 10.0)),
				MakeTransitionAnchor(Variant, 2, FVector(100.0, 0.0, 20.0)),
				MakeTransitionAnchor(Variant, 3, FVector(100.0, 100.0, 30.0)),
				MakeTransitionAnchor(Variant, 4, FVector(0.0, 100.0, 40.0))
			});
		check(Built.IsSuccess());
		return Built.Area;
	}

	Fdemo_mapShanmenFormationCoverageReceipt MakeCoverage(
		const Fdemo_mapShanmenFormationAreaSnapshot& Area,
		const TArray<Fdemo_mapShanmenFormationAreaQuery>& Queries)
	{
		const Fdemo_mapShanmenFormationCoverageResult Coverage =
			Fdemo_mapShanmenFormationAreaProvider::EvaluateCoverage(Area, Queries);
		check(Coverage.IsSuccess());
		return Coverage.Receipt;
	}

	FVector PointForRelation(
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

	FGuid MatrixSubject(const int32 Index)
	{
		return FGuid(0xF8705000 + Index, 0, 0, 1);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationCoverageTransitionMatrixTest,
	"Shanmen.0_0_10.Product.FormationCoverageTransitions.TransitionMatrix",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationCoverageTransitionMatrixTest::RunTest(const FString&)
{
	const Fdemo_mapShanmenFormationAreaSnapshot Area = MakeTransitionArea();
	const TArray<Edemo_mapShanmenFormationAreaRelation> PreviousRelations = {
		Edemo_mapShanmenFormationAreaRelation::Outside,
		Edemo_mapShanmenFormationAreaRelation::Outside,
		Edemo_mapShanmenFormationAreaRelation::Outside,
		Edemo_mapShanmenFormationAreaRelation::Boundary,
		Edemo_mapShanmenFormationAreaRelation::Boundary,
		Edemo_mapShanmenFormationAreaRelation::Boundary,
		Edemo_mapShanmenFormationAreaRelation::Inside,
		Edemo_mapShanmenFormationAreaRelation::Inside,
		Edemo_mapShanmenFormationAreaRelation::Inside
	};
	const TArray<Edemo_mapShanmenFormationAreaRelation> CurrentRelations = {
		Edemo_mapShanmenFormationAreaRelation::Outside,
		Edemo_mapShanmenFormationAreaRelation::Boundary,
		Edemo_mapShanmenFormationAreaRelation::Inside,
		Edemo_mapShanmenFormationAreaRelation::Outside,
		Edemo_mapShanmenFormationAreaRelation::Boundary,
		Edemo_mapShanmenFormationAreaRelation::Inside,
		Edemo_mapShanmenFormationAreaRelation::Outside,
		Edemo_mapShanmenFormationAreaRelation::Boundary,
		Edemo_mapShanmenFormationAreaRelation::Inside
	};
	TArray<Fdemo_mapShanmenFormationAreaQuery> PreviousQueries;
	TArray<Fdemo_mapShanmenFormationAreaQuery> CurrentQueries;
	for (int32 Index = 0; Index < PreviousRelations.Num(); ++Index)
	{
		PreviousQueries.Add({ MatrixSubject(Index),
			PointForRelation(PreviousRelations[Index], Index) });
		CurrentQueries.Add({ MatrixSubject(Index),
			PointForRelation(CurrentRelations[Index], Index + 20) });
	}
	const Fdemo_mapShanmenFormationCoverageTransitionResult Result =
		Fdemo_mapShanmenFormationCoverageTransitionReducer::Reduce(
			MakeCoverage(Area, PreviousQueries),
			MakeCoverage(Area, CurrentQueries));
	if (!TestTrue(TEXT("All nine relation pairs reduce"), Result.IsSuccess()))
	{
		return false;
	}
	TestEqual(TEXT("Two Outside-to-covered entries"),
		Result.Receipt.EnteredCount, 2);
	TestEqual(TEXT("Four covered-to-covered stays"),
		Result.Receipt.StayedCoveredCount, 4);
	TestEqual(TEXT("Two covered-to-Outside leaves"),
		Result.Receipt.LeftCount, 2);
	TestEqual(TEXT("One Outside-to-Outside fact"),
		Result.Receipt.RemainedOutsideCount, 1);
	TestEqual(TEXT("Only enter and leave are changes"),
		Result.Receipt.GetChangedCount(), 4);
	TestEqual(TEXT("Outside to Boundary enters"), Result.Receipt.Facts[1].Kind,
		Edemo_mapShanmenFormationCoverageTransitionKind::Entered);
	TestEqual(TEXT("Boundary to Inside stays covered"),
		Result.Receipt.Facts[5].Kind,
		Edemo_mapShanmenFormationCoverageTransitionKind::StayedCovered);
	TestEqual(TEXT("Inside to Boundary stays covered"),
		Result.Receipt.Facts[7].Kind,
		Edemo_mapShanmenFormationCoverageTransitionKind::StayedCovered);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationCoverageTransitionReplayTest,
	"Shanmen.0_0_10.Product.FormationCoverageTransitions.CanonicalReplay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationCoverageTransitionReplayTest::RunTest(const FString&)
{
	const Fdemo_mapShanmenFormationAreaSnapshot Area = MakeTransitionArea();
	const FGuid A = MatrixSubject(20);
	const FGuid B = MatrixSubject(21);
	const FGuid C = MatrixSubject(22);
	const FGuid D = MatrixSubject(23);
	const Fdemo_mapShanmenFormationCoverageReceipt Previous = MakeCoverage(
		Area,
		{
			{ D, FVector(-40.0, 50.0, 0.0) },
			{ B, FVector(0.0, 50.0, 0.0) },
			{ A, FVector(-10.0, 50.0, 0.0) },
			{ C, FVector(50.0, 50.0, 0.0) }
		});
	const Fdemo_mapShanmenFormationCoverageReceipt Current = MakeCoverage(
		Area,
		{
			{ C, FVector(110.0, 50.0, 0.0) },
			{ A, FVector(10.0, 50.0, 0.0) },
			{ D, FVector(-50.0, 50.0, 0.0) },
			{ B, FVector(50.0, 50.0, 0.0) }
		});
	const Fdemo_mapShanmenFormationCoverageTransitionResult First =
		Fdemo_mapShanmenFormationCoverageTransitionReducer::Reduce(
			Previous, Current);
	const Fdemo_mapShanmenFormationCoverageTransitionResult Replay =
		Fdemo_mapShanmenFormationCoverageTransitionReducer::Reduce(
			Previous, Current);
	if (!TestTrue(TEXT("Canonical batches reduce and replay"),
		First.IsSuccess() && Replay.IsSuccess()))
	{
		return false;
	}
	TestEqual(TEXT("One fact per canonical subject"), First.Receipt.Facts.Num(), 4);
	TestEqual(TEXT("First fact follows stable subject order"),
		First.Receipt.Facts[0].SubjectEntityId, A);
	TestEqual(TEXT("Input query order cannot alter transition identity"),
		First.Receipt.ReceiptId, Replay.Receipt.ReceiptId);
	TestEqual(TEXT("Coverage evidence is retained"),
		First.Receipt.PreviousCoverage.ReceiptId, Previous.ReceiptId);
	TestEqual(TEXT("Current evidence is retained"),
		First.Receipt.CurrentCoverage.ReceiptId, Current.ReceiptId);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationCoverageTransitionFenceTest,
	"Shanmen.0_0_10.Product.FormationCoverageTransitions.InputFences",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationCoverageTransitionFenceTest::RunTest(const FString&)
{
	const Fdemo_mapShanmenFormationAreaSnapshot Area = MakeTransitionArea();
	const Fdemo_mapShanmenFormationAreaSnapshot OtherArea =
		MakeTransitionArea(2);
	const FGuid A = MatrixSubject(30);
	const FGuid B = MatrixSubject(31);
	const FGuid C = MatrixSubject(32);
	const Fdemo_mapShanmenFormationCoverageReceipt Baseline = MakeCoverage(
		Area, { { A, FVector(10.0, 50.0, 0.0) },
			{ B, FVector(20.0, 50.0, 0.0) } });
	Fdemo_mapShanmenFormationCoverageReceipt InvalidPrevious = Baseline;
	InvalidPrevious.ReceiptId.Invalidate();
	Fdemo_mapShanmenFormationCoverageReceipt InvalidCurrent = Baseline;
	InvalidCurrent.Memberships[0].ReceiptId.Invalidate();
	const Fdemo_mapShanmenFormationCoverageReceipt OtherAreaCoverage =
		MakeCoverage(OtherArea,
			{ { A, FVector(10.0, 50.0, 0.0) },
				{ B, FVector(20.0, 50.0, 0.0) } });
	const Fdemo_mapShanmenFormationCoverageReceipt MissingSubject =
		MakeCoverage(Area, { { A, FVector(10.0, 50.0, 0.0) } });
	const Fdemo_mapShanmenFormationCoverageReceipt ReplacedSubject =
		MakeCoverage(Area, { { A, FVector(10.0, 50.0, 0.0) },
			{ C, FVector(20.0, 50.0, 0.0) } });

	const auto PreviousRejected =
		Fdemo_mapShanmenFormationCoverageTransitionReducer::Reduce(
			InvalidPrevious, Baseline);
	const auto CurrentRejected =
		Fdemo_mapShanmenFormationCoverageTransitionReducer::Reduce(
			Baseline, InvalidCurrent);
	const auto AreaRejected =
		Fdemo_mapShanmenFormationCoverageTransitionReducer::Reduce(
			Baseline, OtherAreaCoverage);
	const auto MissingRejected =
		Fdemo_mapShanmenFormationCoverageTransitionReducer::Reduce(
			Baseline, MissingSubject);
	const auto ReplacedRejected =
		Fdemo_mapShanmenFormationCoverageTransitionReducer::Reduce(
			Baseline, ReplacedSubject);
	TestEqual(TEXT("Invalid previous rejected first"), PreviousRejected.Status,
		Edemo_mapShanmenFormationCoverageTransitionStatus::PreviousInvalid);
	TestEqual(TEXT("Invalid current rejected"), CurrentRejected.Status,
		Edemo_mapShanmenFormationCoverageTransitionStatus::CurrentInvalid);
	TestEqual(TEXT("Area replacement rejected"), AreaRejected.Status,
		Edemo_mapShanmenFormationCoverageTransitionStatus::AreaMismatch);
	TestEqual(TEXT("Missing subject is not inferred as leave"),
		MissingRejected.Status,
		Edemo_mapShanmenFormationCoverageTransitionStatus::SubjectSetMismatch);
	TestEqual(TEXT("Replaced subject set rejected"), ReplacedRejected.Status,
		Edemo_mapShanmenFormationCoverageTransitionStatus::SubjectSetMismatch);
	TestFalse(TEXT("Rejected reduction publishes no receipt"),
		ReplacedRejected.Receipt.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationCoverageTransitionSealTest,
	"Shanmen.0_0_10.Product.FormationCoverageTransitions.MutationSeal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationCoverageTransitionSealTest::RunTest(const FString&)
{
	const Fdemo_mapShanmenFormationAreaSnapshot Area = MakeTransitionArea();
	const FGuid A = MatrixSubject(40);
	const FGuid B = MatrixSubject(41);
	const Fdemo_mapShanmenFormationCoverageTransitionResult Reduced =
		Fdemo_mapShanmenFormationCoverageTransitionReducer::Reduce(
			MakeCoverage(Area,
				{ { A, FVector(-10.0, 50.0, 0.0) },
					{ B, FVector(50.0, 50.0, 0.0) } }),
			MakeCoverage(Area,
				{ { A, FVector(50.0, 50.0, 0.0) },
					{ B, FVector(110.0, 50.0, 0.0) } }));
	if (!TestTrue(TEXT("Mutation fixture reduces"), Reduced.IsSuccess()))
	{
		return false;
	}
	Fdemo_mapShanmenFormationCoverageTransitionReceipt CountDrift =
		Reduced.Receipt;
	++CountDrift.EnteredCount;
	Fdemo_mapShanmenFormationCoverageTransitionReceipt KindDrift =
		Reduced.Receipt;
	KindDrift.Facts[0].Kind =
		Edemo_mapShanmenFormationCoverageTransitionKind::Left;
	Fdemo_mapShanmenFormationCoverageTransitionReceipt FactIdDrift =
		Reduced.Receipt;
	FactIdDrift.Facts[0].FactId.Invalidate();
	Fdemo_mapShanmenFormationCoverageTransitionReceipt OrderDrift =
		Reduced.Receipt;
	OrderDrift.Facts.Swap(0, 1);
	Fdemo_mapShanmenFormationCoverageTransitionReceipt EvidenceDrift =
		Reduced.Receipt;
	EvidenceDrift.CurrentCoverage.Memberships[0].WorldLocation.X += 1.0;

	TestFalse(TEXT("Count drift breaks batch seal"), CountDrift.IsValid());
	TestFalse(TEXT("Kind drift breaks fact seal"), KindDrift.IsValid());
	TestFalse(TEXT("Fact identity drift rejected"), FactIdDrift.IsValid());
	TestFalse(TEXT("Fact order drift rejected"), OrderDrift.IsValid());
	TestFalse(TEXT("Nested evidence drift rejected"), EvidenceDrift.IsValid());
	TestTrue(TEXT("Original receipt remains valid"), Reduced.Receipt.IsValid());
	return true;
}

#endif
