#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenFormationInfluenceIntentPlanner.h"

#include "Misc/AutomationTest.h"
#include "ShanmenDeterministicId.h"

namespace
{
	const FGuid InfluenceRunId(0xF8B00001, 0, 0, 1);
	const FGuid InfluenceOwnerId(0xF8B00002, 0, 0, 1);
	const FGuid InfluenceSourceId(0xF8B00003, 0, 0, 1);

	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	FShanmenContentStamp InfluenceContent(const int32 Variant)
	{
		FShanmenContentStamp Content;
		Content.Version = TEXT("0.0.10.P8.11");
		Content.Digest = FString::Printf(
			TEXT("formation-influence-intents-r%d"), Variant);
		return Content;
	}

	Fdemo_mapShanmenFormationAnchorPlacementReceipt MakeInfluenceAnchor(
		const int32 Variant,
		const int32 Ordinal,
		const FVector& WorldLocation)
	{
		Fdemo_mapShanmenFormationAnchorPlacementReceipt Receipt;
		Fdemo_mapShanmenFormationAnchorPlacementIntent& Intent = Receipt.Intent;
		Intent.RunId = InfluenceRunId;
		Intent.OwnerId = InfluenceOwnerId;
		Intent.DeploymentId = FGuid(0xF8B00100 + Variant, 0, 0, 1);
		Intent.AnchorDefinitionId = FName(*FString::Printf(
			TEXT("Formation.Anchor.Influence.%d.%02d"), Variant, Ordinal));
		Intent.AnchorInstanceId =
			FGuid(0xF8B01000 + Variant * 0x100 + Ordinal, 0, 0, 1);
		Intent.WorldLocation = WorldLocation;
		Intent.AttemptId =
			FGuid(0xF8B02000 + Variant * 0x100 + Ordinal, 0, 0, 1);
		Intent.FulfillmentId =
			FGuid(0xF8B03000 + Variant * 0x100 + Ordinal, 0, 0, 1);
		Intent.DeploymentReceiptId =
			FGuid(0xF8B04000 + Variant * 0x100 + Ordinal, 0, 0, 1);
		Intent.AuthorityRevision = Variant * 100 + Ordinal;
		Intent.Content = InfluenceContent(Variant);
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

	Fdemo_mapShanmenFormationAreaSnapshot MakeInfluenceArea(
		const int32 Variant = 1)
	{
		const Fdemo_mapShanmenFormationAreaBuildResult Built =
			Fdemo_mapShanmenFormationAreaProvider::BuildArea({
				MakeInfluenceAnchor(Variant, 1, FVector(0.0, 0.0, 10.0)),
				MakeInfluenceAnchor(Variant, 2, FVector(100.0, 0.0, 20.0)),
				MakeInfluenceAnchor(Variant, 3, FVector(100.0, 100.0, 30.0)),
				MakeInfluenceAnchor(Variant, 4, FVector(0.0, 100.0, 40.0))
			});
		check(Built.IsSuccess());
		return Built.Area;
	}

	FGuid InfluenceSubject(const int32 Index)
	{
		return FGuid(0xF8B05000 + Index, 0, 0, 1);
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

	Fdemo_mapShanmenFormationCoverageReceipt MakeInfluenceCoverage(
		const Fdemo_mapShanmenFormationAreaSnapshot& Area,
		const TArray<FGuid>& Subjects,
		const TArray<Edemo_mapShanmenFormationAreaRelation>& Relations,
		const int32 SlotOffset)
	{
		check(Subjects.Num() == Relations.Num());
		TArray<Fdemo_mapShanmenFormationAreaQuery> Queries;
		Queries.Reserve(Subjects.Num());
		for (int32 Index = 0; Index < Subjects.Num(); ++Index)
		{
			Queries.Add({ Subjects[Index],
				PointForRelation(Relations[Index], SlotOffset + Index) });
		}
		const Fdemo_mapShanmenFormationCoverageResult Coverage =
			Fdemo_mapShanmenFormationAreaProvider::EvaluateCoverage(
				Area, Queries);
		check(Coverage.IsSuccess());
		return Coverage.Receipt;
	}

	Fdemo_mapShanmenFormationCoverageTransitionReceipt MakeInfluenceTransition(
		const Fdemo_mapShanmenFormationAreaSnapshot& Area,
		const TArray<FGuid>& Subjects,
		const TArray<Edemo_mapShanmenFormationAreaRelation>& Previous,
		const TArray<Edemo_mapShanmenFormationAreaRelation>& Current)
	{
		const Fdemo_mapShanmenFormationCoverageTransitionResult Reduced =
			Fdemo_mapShanmenFormationCoverageTransitionReducer::Reduce(
				MakeInfluenceCoverage(Area, Subjects, Previous, 0),
				MakeInfluenceCoverage(Area, Subjects, Current, 20));
		check(Reduced.IsSuccess());
		return Reduced.Receipt;
	}

	Fdemo_mapShanmenFormationInfluencePolicy MakeInfluencePolicy(
		const Fdemo_mapShanmenFormationAreaSnapshot& Area,
		const TCHAR* InfluenceId = TEXT("Formation.Influence.Test.Guard"))
	{
		Fdemo_mapShanmenFormationInfluencePolicy Policy;
		Policy.PolicyDefinitionId = TEXT("Formation.Policy.Test.CoverageDelta");
		Policy.InfluenceDefinitionId = FName(InfluenceId);
		Policy.Content = Area.Content;
		check(Policy.IsValid());
		return Policy;
	}

	Fdemo_mapShanmenFormationCoverageTransitionReceipt MakeDeltaTransition(
		const Fdemo_mapShanmenFormationAreaSnapshot& Area)
	{
		return MakeInfluenceTransition(
			Area,
			{ InfluenceSubject(1), InfluenceSubject(2),
				InfluenceSubject(3), InfluenceSubject(4) },
			{ Edemo_mapShanmenFormationAreaRelation::Outside,
				Edemo_mapShanmenFormationAreaRelation::Inside,
				Edemo_mapShanmenFormationAreaRelation::Boundary,
				Edemo_mapShanmenFormationAreaRelation::Outside },
			{ Edemo_mapShanmenFormationAreaRelation::Inside,
				Edemo_mapShanmenFormationAreaRelation::Outside,
				Edemo_mapShanmenFormationAreaRelation::Inside,
				Edemo_mapShanmenFormationAreaRelation::Outside });
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationInfluenceDeltaPlanTest,
	"Shanmen.0_0_10.Product.FormationInfluenceIntents.DeltaPlan",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationInfluenceDeltaPlanTest::RunTest(const FString&)
{
	const Fdemo_mapShanmenFormationAreaSnapshot Area = MakeInfluenceArea();
	const Fdemo_mapShanmenFormationCoverageTransitionReceipt Transition =
		MakeDeltaTransition(Area);
	const Fdemo_mapShanmenFormationInfluencePlanResult Planned =
		Fdemo_mapShanmenFormationInfluenceIntentPlanner::PlanTransition(
			Area, InfluenceSourceId, MakeInfluencePolicy(Area), Transition);
	if (!TestTrue(TEXT("Mixed transition produces a valid batch"),
		Planned.IsSuccess()))
	{
		return false;
	}
	TestEqual(TEXT("Only Entered and Left emit intents"),
		Planned.Batch.Intents.Num(), 2);
	TestEqual(TEXT("One Apply intent"), Planned.Batch.ApplyCount, 1);
	TestEqual(TEXT("One Remove intent"), Planned.Batch.RemoveCount, 1);
	TestEqual(TEXT("Canonical first subject entered"),
		Planned.Batch.Intents[0].SubjectEntityId, InfluenceSubject(1));
	TestEqual(TEXT("Entered maps to Apply"),
		Planned.Batch.Intents[0].Operation,
		Edemo_mapShanmenFormationInfluenceOperation::Apply);
	TestEqual(TEXT("Canonical second subject left"),
		Planned.Batch.Intents[1].SubjectEntityId, InfluenceSubject(2));
	TestEqual(TEXT("Left maps to Remove"),
		Planned.Batch.Intents[1].Operation,
		Edemo_mapShanmenFormationInfluenceOperation::Remove);
	TestEqual(TEXT("Apply cause is exact transition fact"),
		Planned.Batch.Intents[0].CauseId, Transition.Facts[0].FactId);
	TestEqual(TEXT("Remove cause is exact transition fact"),
		Planned.Batch.Intents[1].CauseId, Transition.Facts[1].FactId);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationInfluenceReplayNoOpTest,
	"Shanmen.0_0_10.Product.FormationInfluenceIntents.CanonicalReplayAndNoOp",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationInfluenceReplayNoOpTest::RunTest(const FString&)
{
	const Fdemo_mapShanmenFormationAreaSnapshot Area = MakeInfluenceArea();
	const Fdemo_mapShanmenFormationCoverageTransitionReceipt Delta =
		MakeDeltaTransition(Area);
	const Fdemo_mapShanmenFormationInfluencePolicy Policy =
		MakeInfluencePolicy(Area);
	const auto First =
		Fdemo_mapShanmenFormationInfluenceIntentPlanner::PlanTransition(
			Area, InfluenceSourceId, Policy, Delta);
	const auto Replay =
		Fdemo_mapShanmenFormationInfluenceIntentPlanner::PlanTransition(
			Area, InfluenceSourceId, Policy, Delta);
	const auto OtherInfluence =
		Fdemo_mapShanmenFormationInfluenceIntentPlanner::PlanTransition(
			Area, InfluenceSourceId,
			MakeInfluencePolicy(
				Area, TEXT("Formation.Influence.Test.SpiritShield")),
			Delta);
	const Fdemo_mapShanmenFormationCoverageTransitionReceipt NoOpTransition =
		MakeInfluenceTransition(
			Area, { InfluenceSubject(10), InfluenceSubject(11) },
			{ Edemo_mapShanmenFormationAreaRelation::Inside,
				Edemo_mapShanmenFormationAreaRelation::Outside },
			{ Edemo_mapShanmenFormationAreaRelation::Boundary,
				Edemo_mapShanmenFormationAreaRelation::Outside });
	const auto NoOp =
		Fdemo_mapShanmenFormationInfluenceIntentPlanner::PlanTransition(
			Area, InfluenceSourceId, Policy, NoOpTransition);

	TestTrue(TEXT("Exact replay and alternate policy both validate"),
		First.IsSuccess() && Replay.IsSuccess()
			&& OtherInfluence.IsSuccess());
	TestEqual(TEXT("Exact replay keeps batch identity"),
		First.Batch.BatchId, Replay.Batch.BatchId);
	TestNotEqual(TEXT("Influence definition participates in identity"),
		First.Batch.BatchId, OtherInfluence.Batch.BatchId);
	TestTrue(TEXT("Unchanged relations publish a sealed no-op batch"),
		NoOp.IsSuccess() && NoOp.Batch.IsNoOp()
			&& NoOp.Batch.ApplyCount == 0
			&& NoOp.Batch.RemoveCount == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationInfluenceInputFenceTest,
	"Shanmen.0_0_10.Product.FormationInfluenceIntents.InputFences",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationInfluenceInputFenceTest::RunTest(const FString&)
{
	const Fdemo_mapShanmenFormationAreaSnapshot Area = MakeInfluenceArea();
	const Fdemo_mapShanmenFormationCoverageTransitionReceipt Transition =
		MakeDeltaTransition(Area);
	const Fdemo_mapShanmenFormationInfluencePolicy Policy =
		MakeInfluencePolicy(Area);
	Fdemo_mapShanmenFormationAreaSnapshot InvalidArea = Area;
	InvalidArea.AreaId.Invalidate();
	Fdemo_mapShanmenFormationInfluencePolicy InvalidPolicy = Policy;
	InvalidPolicy.PolicyDefinitionId = NAME_None;
	Fdemo_mapShanmenFormationCoverageTransitionReceipt InvalidTransition =
		Transition;
	InvalidTransition.ReceiptId.Invalidate();
	const Fdemo_mapShanmenFormationAreaSnapshot OtherArea =
		MakeInfluenceArea(2);
	Fdemo_mapShanmenFormationInfluencePolicy WrongContent = Policy;
	WrongContent.Content.Digest = TEXT("formation-influence-intents-foreign");

	const auto AreaRejected =
		Fdemo_mapShanmenFormationInfluenceIntentPlanner::PlanTransition(
			InvalidArea, InfluenceSourceId, Policy, Transition);
	const auto SourceRejected =
		Fdemo_mapShanmenFormationInfluenceIntentPlanner::PlanTransition(
			Area, FGuid(), Policy, Transition);
	const auto PolicyRejected =
		Fdemo_mapShanmenFormationInfluenceIntentPlanner::PlanTransition(
			Area, InfluenceSourceId, InvalidPolicy, Transition);
	const auto TransitionRejected =
		Fdemo_mapShanmenFormationInfluenceIntentPlanner::PlanTransition(
			Area, InfluenceSourceId, Policy, InvalidTransition);
	const auto ScopeRejected =
		Fdemo_mapShanmenFormationInfluenceIntentPlanner::PlanTransition(
			OtherArea, InfluenceSourceId,
			MakeInfluencePolicy(OtherArea), Transition);
	const auto ContentRejected =
		Fdemo_mapShanmenFormationInfluenceIntentPlanner::PlanTransition(
			Area, InfluenceSourceId, WrongContent, Transition);

	TestEqual(TEXT("Invalid Area rejected first"), AreaRejected.Status,
		Edemo_mapShanmenFormationInfluencePlanStatus::AreaInvalid);
	TestEqual(TEXT("Missing source rejected"), SourceRejected.Status,
		Edemo_mapShanmenFormationInfluencePlanStatus::SourceInvalid);
	TestEqual(TEXT("Malformed policy rejected"), PolicyRejected.Status,
		Edemo_mapShanmenFormationInfluencePlanStatus::PolicyInvalid);
	TestEqual(TEXT("Malformed transition rejected"), TransitionRejected.Status,
		Edemo_mapShanmenFormationInfluencePlanStatus::TransitionInvalid);
	TestEqual(TEXT("Foreign Area rejected"), ScopeRejected.Status,
		Edemo_mapShanmenFormationInfluencePlanStatus::AreaMismatch);
	TestEqual(TEXT("Content mismatch rejected"), ContentRejected.Status,
		Edemo_mapShanmenFormationInfluencePlanStatus::ContentMismatch);
	TestFalse(TEXT("Rejected inputs publish no valid batch"),
		ContentRejected.Batch.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationInfluenceMutationSealTest,
	"Shanmen.0_0_10.Product.FormationInfluenceIntents.MutationSeal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationInfluenceMutationSealTest::RunTest(const FString&)
{
	const Fdemo_mapShanmenFormationAreaSnapshot Area = MakeInfluenceArea();
	const auto Planned =
		Fdemo_mapShanmenFormationInfluenceIntentPlanner::PlanTransition(
			Area, InfluenceSourceId, MakeInfluencePolicy(Area),
			MakeDeltaTransition(Area));
	if (!TestTrue(TEXT("Mutation fixture plans"), Planned.IsSuccess()))
	{
		return false;
	}

	auto CountDrift = Planned.Batch;
	++CountDrift.ApplyCount;
	auto OperationDrift = Planned.Batch;
	OperationDrift.Intents[0].Operation =
		Edemo_mapShanmenFormationInfluenceOperation::Remove;
	auto CauseDrift = Planned.Batch;
	CauseDrift.Intents[0].CauseId.Invalidate();
	auto OrderDrift = Planned.Batch;
	OrderDrift.Intents.Swap(0, 1);
	auto PolicyDrift = Planned.Batch;
	PolicyDrift.Policy.InfluenceDefinitionId =
		TEXT("Formation.Influence.Test.Drift");
	auto EvidenceDrift = Planned.Batch;
	EvidenceDrift.Transition.Facts[0].FactId.Invalidate();
	auto BatchIdDrift = Planned.Batch;
	BatchIdDrift.BatchId.Invalidate();
	auto StandaloneIntentDrift = Planned.Batch.Intents[0];
	StandaloneIntentDrift.SubjectEntityId = InfluenceSubject(99);

	TestFalse(TEXT("Count drift breaks batch seal"), CountDrift.IsValid());
	TestFalse(TEXT("Operation drift breaks intent seal"),
		OperationDrift.IsValid());
	TestFalse(TEXT("Cause drift breaks intent seal"), CauseDrift.IsValid());
	TestFalse(TEXT("Canonical intent order is sealed"), OrderDrift.IsValid());
	TestFalse(TEXT("Policy drift breaks batch seal"), PolicyDrift.IsValid());
	TestFalse(TEXT("Nested transition drift rejected"),
		EvidenceDrift.IsValid());
	TestFalse(TEXT("Batch identity drift rejected"), BatchIdDrift.IsValid());
	TestFalse(TEXT("Standalone intent identity is deterministic"),
		StandaloneIntentDrift.IsValid());
	TestTrue(TEXT("Original batch remains valid"), Planned.Batch.IsValid());
	return true;
}

#endif
