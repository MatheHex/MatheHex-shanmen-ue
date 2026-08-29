#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenFormationAreaProvider.h"

#include "ShanmenDeterministicId.h"
#include "Misc/AutomationTest.h"

#include <limits>

namespace
{
	const FGuid AreaRunId(0xF8500001, 0, 0, 1);
	const FGuid AreaOwnerId(0xF8500002, 0, 0, 1);
	const FGuid AreaDeploymentId(0xF8500003, 0, 0, 1);

	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	FShanmenContentStamp AreaContent()
	{
		FShanmenContentStamp Content;
		Content.Version = TEXT("0.0.10.P8.5");
		Content.Digest = TEXT("formation-area-provider-r0");
		return Content;
	}

	Fdemo_mapShanmenFormationAnchorPlacementReceipt MakePlacedAnchor(
		const int32 Ordinal,
		const FVector& WorldLocation,
		const FGuid& DeploymentId = AreaDeploymentId)
	{
		Fdemo_mapShanmenFormationAnchorPlacementReceipt Receipt;
		Fdemo_mapShanmenFormationAnchorPlacementIntent& Intent = Receipt.Intent;
		Intent.RunId = AreaRunId;
		Intent.OwnerId = AreaOwnerId;
		Intent.DeploymentId = DeploymentId;
		Intent.AnchorDefinitionId = FName(*FString::Printf(
			TEXT("Formation.Anchor.Area.%02d"), Ordinal));
		Intent.AnchorInstanceId = FGuid(0xF8500100 + Ordinal, 0, 0, 1);
		Intent.WorldLocation = WorldLocation;
		Intent.AttemptId = FGuid(0xF8500200 + Ordinal, 0, 0, 1);
		Intent.FulfillmentId = FGuid(0xF8500300 + Ordinal, 0, 0, 1);
		Intent.DeploymentReceiptId = FGuid(0xF8500400 + Ordinal, 0, 0, 1);
		Intent.AuthorityRevision = 10 + Ordinal;
		Intent.Content = AreaContent();
		Intent.PlacementId = FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Formation.WorldPlacement.r1"),
			{
				GuidDigits(Intent.RunId),
				GuidDigits(Intent.OwnerId),
				GuidDigits(Intent.DeploymentId),
				Intent.AnchorDefinitionId.ToString(),
				GuidDigits(Intent.AnchorInstanceId),
				GuidDigits(Intent.AttemptId),
				GuidDigits(Intent.FulfillmentId),
				GuidDigits(Intent.DeploymentReceiptId),
				FString::FromInt(Intent.AuthorityRevision),
				Intent.Content.Version.ToString(),
				Intent.Content.Digest
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

	TArray<Fdemo_mapShanmenFormationAnchorPlacementReceipt> MakeSquare(
		const bool bIncludeInterior = false)
	{
		TArray<Fdemo_mapShanmenFormationAnchorPlacementReceipt> Receipts = {
			MakePlacedAnchor(3, FVector(100.0, 100.0, 30.0)),
			MakePlacedAnchor(1, FVector(0.0, 0.0, 10.0)),
			MakePlacedAnchor(4, FVector(0.0, 100.0, 40.0)),
			MakePlacedAnchor(2, FVector(100.0, 0.0, 20.0))
		};
		if (bIncludeInterior)
		{
			Receipts.Insert(
				MakePlacedAnchor(5, FVector(50.0, 50.0, 500.0)), 1);
		}
		return Receipts;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationAreaDeterministicHullTest,
	"Shanmen.0_0_10.Product.FormationAreaProvider.DeterministicHull",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationAreaDeterministicHullTest::RunTest(const FString&)
{
	TArray<Fdemo_mapShanmenFormationAnchorPlacementReceipt> FirstInput =
		MakeSquare(true);
	TArray<Fdemo_mapShanmenFormationAnchorPlacementReceipt> SecondInput =
		FirstInput;
	Algo::Reverse(SecondInput);
	const Fdemo_mapShanmenFormationAreaBuildResult First =
		Fdemo_mapShanmenFormationAreaProvider::BuildArea(FirstInput);
	const Fdemo_mapShanmenFormationAreaBuildResult Second =
		Fdemo_mapShanmenFormationAreaProvider::BuildArea(SecondInput);

	TestTrue(TEXT("Shuffled valid placements build"),
		First.IsSuccess() && Second.IsSuccess());
	TestEqual(TEXT("Every placed anchor remains in evidence"),
		First.Area.Anchors.Num(), 5);
	TestEqual(TEXT("Interior anchor is not a boundary vertex"),
		First.Area.BoundaryAnchorIndices.Num(), 4);
	TestEqual(TEXT("Input order cannot alter area identity"),
		First.Area.AreaId, Second.Area.AreaId);
	TestTrue(TEXT("Canonical snapshots self-validate"),
		First.Area.IsValid() && Second.Area.IsValid());
	if (First.Area.BoundaryAnchorIndices.Num() == 4)
	{
		const TArray<FVector> Expected = {
			FVector(0.0, 0.0, 10.0),
			FVector(100.0, 0.0, 20.0),
			FVector(100.0, 100.0, 30.0),
			FVector(0.0, 100.0, 40.0)
		};
		for (int32 Index = 0; Index < Expected.Num(); ++Index)
		{
			TestEqual(
				FString::Printf(TEXT("Boundary %d is canonical CCW"), Index),
				First.Area.Anchors[
					First.Area.BoundaryAnchorIndices[Index]].WorldLocation,
				Expected[Index]);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationAreaMembershipCoverageTest,
	"Shanmen.0_0_10.Product.FormationAreaProvider.MembershipCoverage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationAreaMembershipCoverageTest::RunTest(const FString&)
{
	const Fdemo_mapShanmenFormationAreaBuildResult Built =
		Fdemo_mapShanmenFormationAreaProvider::BuildArea(MakeSquare());
	if (!TestTrue(TEXT("Square builds"), Built.IsSuccess()))
	{
		return false;
	}
	const Fdemo_mapShanmenFormationAreaQuery Inside = {
		FGuid(0xF8501001, 0, 0, 1), FVector(50.0, 50.0, 900.0) };
	const Fdemo_mapShanmenFormationAreaQuery Boundary = {
		FGuid(0xF8501002, 0, 0, 1), FVector(0.0, 75.0, -900.0) };
	const Fdemo_mapShanmenFormationAreaQuery Outside = {
		FGuid(0xF8501003, 0, 0, 1), FVector(-1.0, 50.0, 0.0) };
	const Fdemo_mapShanmenFormationMembershipResult InsideResult =
		Fdemo_mapShanmenFormationAreaProvider::EvaluateMembership(
			Built.Area, Inside);
	const Fdemo_mapShanmenFormationMembershipResult BoundaryResult =
		Fdemo_mapShanmenFormationAreaProvider::EvaluateMembership(
			Built.Area, Boundary);
	const Fdemo_mapShanmenFormationMembershipResult OutsideResult =
		Fdemo_mapShanmenFormationAreaProvider::EvaluateMembership(
			Built.Area, Outside);

	TestTrue(TEXT("All memberships produce valid receipts"),
		InsideResult.IsSuccess() && BoundaryResult.IsSuccess()
			&& OutsideResult.IsSuccess());
	TestEqual(TEXT("Inside classified"), InsideResult.Receipt.Relation,
		Edemo_mapShanmenFormationAreaRelation::Inside);
	TestEqual(TEXT("Boundary classified independently"),
		BoundaryResult.Receipt.Relation,
		Edemo_mapShanmenFormationAreaRelation::Boundary);
	TestEqual(TEXT("Outside classified"), OutsideResult.Receipt.Relation,
		Edemo_mapShanmenFormationAreaRelation::Outside);
	TestTrue(TEXT("Boundary and inside are covered"),
		InsideResult.Receipt.IsCovered()
			&& BoundaryResult.Receipt.IsCovered()
			&& !OutsideResult.Receipt.IsCovered());

	const TArray<Fdemo_mapShanmenFormationAreaQuery> Queries = {
		Outside, Inside, Boundary };
	TArray<Fdemo_mapShanmenFormationAreaQuery> Reordered = Queries;
	Algo::Reverse(Reordered);
	const Fdemo_mapShanmenFormationCoverageResult Coverage =
		Fdemo_mapShanmenFormationAreaProvider::EvaluateCoverage(
			Built.Area, Queries);
	const Fdemo_mapShanmenFormationCoverageResult Replay =
		Fdemo_mapShanmenFormationAreaProvider::EvaluateCoverage(
			Built.Area, Reordered);
	TestTrue(TEXT("Coverage and replay self-validate"),
		Coverage.IsSuccess() && Replay.IsSuccess());
	TestEqual(TEXT("Inside count"), Coverage.Receipt.InsideCount, 1);
	TestEqual(TEXT("Boundary count"), Coverage.Receipt.BoundaryCount, 1);
	TestEqual(TEXT("Outside count"), Coverage.Receipt.OutsideCount, 1);
	TestEqual(TEXT("Covered count includes boundary"),
		Coverage.Receipt.GetCoveredCount(), 2);
	TestEqual(TEXT("Query order cannot alter coverage identity"),
		Coverage.Receipt.ReceiptId, Replay.Receipt.ReceiptId);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationAreaFailClosedTest,
	"Shanmen.0_0_10.Product.FormationAreaProvider.FailClosed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationAreaFailClosedTest::RunTest(const FString&)
{
	const Fdemo_mapShanmenFormationAreaBuildResult Empty =
		Fdemo_mapShanmenFormationAreaProvider::BuildArea({});
	TArray<Fdemo_mapShanmenFormationAnchorPlacementReceipt> Two = {
		MakePlacedAnchor(1, FVector(0.0, 0.0, 0.0)),
		MakePlacedAnchor(2, FVector(100.0, 0.0, 0.0)) };
	const Fdemo_mapShanmenFormationAreaBuildResult Insufficient =
		Fdemo_mapShanmenFormationAreaProvider::BuildArea(Two);
	TArray<Fdemo_mapShanmenFormationAnchorPlacementReceipt> Collinear = Two;
	Collinear.Add(MakePlacedAnchor(3, FVector(200.0, 0.0, 0.0)));
	const Fdemo_mapShanmenFormationAreaBuildResult Degenerate =
		Fdemo_mapShanmenFormationAreaProvider::BuildArea(Collinear);
	TArray<Fdemo_mapShanmenFormationAnchorPlacementReceipt> DuplicatePoint = Two;
	DuplicatePoint.Add(MakePlacedAnchor(3, FVector(100.0, 0.0, 50.0)));
	const Fdemo_mapShanmenFormationAreaBuildResult DuplicatePointResult =
		Fdemo_mapShanmenFormationAreaProvider::BuildArea(DuplicatePoint);
	TArray<Fdemo_mapShanmenFormationAnchorPlacementReceipt> DuplicateIdentity =
		MakeSquare();
	DuplicateIdentity[1] = DuplicateIdentity[0];
	const Fdemo_mapShanmenFormationAreaBuildResult DuplicateIdentityResult =
		Fdemo_mapShanmenFormationAreaProvider::BuildArea(DuplicateIdentity);
	TArray<Fdemo_mapShanmenFormationAnchorPlacementReceipt> Mixed = MakeSquare();
	Mixed[0] = MakePlacedAnchor(
		9, FVector(100.0, 100.0, 0.0),
		FGuid(0xF8509999, 0, 0, 1));
	const Fdemo_mapShanmenFormationAreaBuildResult MixedResult =
		Fdemo_mapShanmenFormationAreaProvider::BuildArea(Mixed);
	TArray<Fdemo_mapShanmenFormationAnchorPlacementReceipt> Invalid =
		MakeSquare();
	Invalid[0].ReceiptId = FGuid();
	const Fdemo_mapShanmenFormationAreaBuildResult InvalidResult =
		Fdemo_mapShanmenFormationAreaProvider::BuildArea(Invalid);

	TestEqual(TEXT("Empty set rejected"), Empty.Status,
		Edemo_mapShanmenFormationAreaBuildStatus::EmptyReceiptSet);
	TestEqual(TEXT("Two anchors do not invent an area"), Insufficient.Status,
		Edemo_mapShanmenFormationAreaBuildStatus::InsufficientAnchors);
	TestEqual(TEXT("Collinear anchors rejected"), Degenerate.Status,
		Edemo_mapShanmenFormationAreaBuildStatus::DegenerateGeometry);
	TestEqual(TEXT("Coincident XY anchors rejected"),
		DuplicatePointResult.Status,
		Edemo_mapShanmenFormationAreaBuildStatus::DuplicatePoint);
	TestEqual(TEXT("Duplicate identity rejected"),
		DuplicateIdentityResult.Status,
		Edemo_mapShanmenFormationAreaBuildStatus::DuplicateIdentity);
	TestEqual(TEXT("Mixed deployment rejected"), MixedResult.Status,
		Edemo_mapShanmenFormationAreaBuildStatus::ScopeMismatch);
	TestEqual(TEXT("Invalid placement receipt rejected"), InvalidResult.Status,
		Edemo_mapShanmenFormationAreaBuildStatus::ReceiptInvalid);

	const Fdemo_mapShanmenFormationAreaBuildResult Built =
		Fdemo_mapShanmenFormationAreaProvider::BuildArea(MakeSquare());
	Fdemo_mapShanmenFormationAreaQuery InvalidQuery;
	InvalidQuery.SubjectEntityId = FGuid(0xF8502001, 0, 0, 1);
	InvalidQuery.WorldLocation.X = std::numeric_limits<double>::quiet_NaN();
	const Fdemo_mapShanmenFormationMembershipResult BadMembership =
		Fdemo_mapShanmenFormationAreaProvider::EvaluateMembership(
			Built.Area, InvalidQuery);
	const Fdemo_mapShanmenFormationAreaQuery DuplicateSubject = {
		FGuid(0xF8502002, 0, 0, 1), FVector(10.0, 10.0, 0.0) };
	const Fdemo_mapShanmenFormationCoverageResult DuplicateCoverage =
		Fdemo_mapShanmenFormationAreaProvider::EvaluateCoverage(
			Built.Area, { DuplicateSubject, DuplicateSubject });
	TestEqual(TEXT("Non-finite query rejected"), BadMembership.Status,
		Edemo_mapShanmenFormationMembershipStatus::QueryInvalid);
	TestEqual(TEXT("Duplicate subject coverage rejected"),
		DuplicateCoverage.Status,
		Edemo_mapShanmenFormationCoverageStatus::DuplicateSubject);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationAreaGeometrySealTest,
	"Shanmen.0_0_10.Product.FormationAreaProvider.GeometrySeal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationAreaGeometrySealTest::RunTest(const FString&)
{
	TArray<Fdemo_mapShanmenFormationAnchorPlacementReceipt> Original =
		MakeSquare();
	TArray<Fdemo_mapShanmenFormationAnchorPlacementReceipt> ZChanged =
		Original;
	ZChanged[0] = MakePlacedAnchor(3, FVector(100.0, 100.0, 31.0));
	const Fdemo_mapShanmenFormationAreaBuildResult First =
		Fdemo_mapShanmenFormationAreaProvider::BuildArea(Original);
	const Fdemo_mapShanmenFormationAreaBuildResult Second =
		Fdemo_mapShanmenFormationAreaProvider::BuildArea(ZChanged);
	TestTrue(TEXT("Both exact geometries build"),
		First.IsSuccess() && Second.IsSuccess());
	TestNotEqual(TEXT("Area identity seals exact XYZ geometry"),
		First.Area.AreaId, Second.Area.AreaId);

	Fdemo_mapShanmenFormationAreaSnapshot Mutated = First.Area;
	Mutated.Anchors[0].WorldLocation.X += 1.0;
	TestFalse(TEXT("Post-capture geometry mutation invalidates snapshot"),
		Mutated.IsValid());
	const Fdemo_mapShanmenFormationAreaQuery SameXYDifferentZ = {
		FGuid(0xF8503001, 0, 0, 1), FVector(50.0, 50.0, 100000.0) };
	const Fdemo_mapShanmenFormationMembershipResult Membership =
		Fdemo_mapShanmenFormationAreaProvider::EvaluateMembership(
			First.Area, SameXYDifferentZ);
	TestEqual(TEXT("Membership is explicitly horizontal"),
		Membership.Receipt.Relation,
		Edemo_mapShanmenFormationAreaRelation::Inside);
	return true;
}

#endif
