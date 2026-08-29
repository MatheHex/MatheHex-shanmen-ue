#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenFormationInfluenceReconciliationPlanner.h"

#include "Misc/AutomationTest.h"
#include "ShanmenDeterministicId.h"

namespace
{
	const FGuid ReconcileRunId(0xF8C00001, 0, 0, 1);
	const FGuid ReconcileOwnerId(0xF8C00002, 0, 0, 1);
	const FGuid ReconcileSourceId(0xF8C00003, 0, 0, 1);

	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	FShanmenContentStamp ReconcileContent(const int32 Variant)
	{
		FShanmenContentStamp Content;
		Content.Version = TEXT("0.0.10.P8.12");
		Content.Digest = FString::Printf(
			TEXT("formation-influence-reconciliation-r%d"), Variant);
		return Content;
	}

	Fdemo_mapShanmenFormationAnchorPlacementReceipt MakeReconcileAnchor(
		const int32 Variant,
		const int32 Ordinal,
		const FVector& WorldLocation)
	{
		Fdemo_mapShanmenFormationAnchorPlacementReceipt Receipt;
		auto& Intent = Receipt.Intent;
		Intent.RunId = ReconcileRunId;
		Intent.OwnerId = ReconcileOwnerId;
		Intent.DeploymentId = FGuid(0xF8C00100 + Variant, 0, 0, 1);
		Intent.AnchorDefinitionId = FName(*FString::Printf(
			TEXT("Formation.Anchor.Reconcile.%d.%02d"), Variant, Ordinal));
		Intent.AnchorInstanceId =
			FGuid(0xF8C01000 + Variant * 0x100 + Ordinal, 0, 0, 1);
		Intent.WorldLocation = WorldLocation;
		Intent.AttemptId =
			FGuid(0xF8C02000 + Variant * 0x100 + Ordinal, 0, 0, 1);
		Intent.FulfillmentId =
			FGuid(0xF8C03000 + Variant * 0x100 + Ordinal, 0, 0, 1);
		Intent.DeploymentReceiptId =
			FGuid(0xF8C04000 + Variant * 0x100 + Ordinal, 0, 0, 1);
		Intent.AuthorityRevision = Variant * 100 + Ordinal;
		Intent.Content = ReconcileContent(Variant);
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

	Fdemo_mapShanmenFormationAreaSnapshot MakeReconcileArea(
		const int32 Variant = 1,
		const int32 IdentityOffset = 0)
	{
		const Fdemo_mapShanmenFormationAreaBuildResult Built =
			Fdemo_mapShanmenFormationAreaProvider::BuildArea({
				MakeReconcileAnchor(
					Variant, IdentityOffset + 1, FVector(0.0, 0.0, 10.0)),
				MakeReconcileAnchor(
					Variant, IdentityOffset + 2, FVector(100.0, 0.0, 20.0)),
				MakeReconcileAnchor(
					Variant, IdentityOffset + 3, FVector(100.0, 100.0, 30.0)),
				MakeReconcileAnchor(
					Variant, IdentityOffset + 4, FVector(0.0, 100.0, 40.0))
			});
		check(Built.IsSuccess());
		return Built.Area;
	}

	FGuid ReconcileSubject(const int32 Index)
	{
		return FGuid(0xF8C05000 + Index, 0, 0, 1);
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

	Fdemo_mapShanmenFormationInfluenceScope MakeReconcileScope(
		const Fdemo_mapShanmenFormationAreaSnapshot& Area,
		const TArray<FGuid>& Subjects,
		const TArray<Edemo_mapShanmenFormationAreaRelation>& Relations,
		const TCHAR* InfluenceId = TEXT("Formation.Influence.Test.Ward"))
	{
		check(Subjects.Num() == Relations.Num());
		TArray<Fdemo_mapShanmenFormationAreaQuery> Queries;
		for (int32 Index = 0; Index < Subjects.Num(); ++Index)
		{
			Queries.Add({
				Subjects[Index], PointForRelation(Relations[Index], Index) });
		}
		const Fdemo_mapShanmenFormationCoverageResult Coverage =
			Fdemo_mapShanmenFormationAreaProvider::EvaluateCoverage(
				Area, Queries);
		check(Coverage.IsSuccess());

		Fdemo_mapShanmenFormationInfluenceScope Scope;
		Scope.Area = Area;
		Scope.Policy.PolicyDefinitionId =
			TEXT("Formation.Policy.Test.Lifecycle");
		Scope.Policy.InfluenceDefinitionId = FName(InfluenceId);
		Scope.Policy.Content = Area.Content;
		Scope.Coverage = Coverage.Receipt;
		check(Scope.IsValid());
		return Scope;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationInfluenceReconcilePrimeTest,
	"Shanmen.0_0_10.Product.FormationInfluenceReconciliation.PrimeAndReplay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationInfluenceReconcilePrimeTest::RunTest(const FString&)
{
	const auto Scope = MakeReconcileScope(
		MakeReconcileArea(),
		{ ReconcileSubject(1), ReconcileSubject(2), ReconcileSubject(3) },
		{ Edemo_mapShanmenFormationAreaRelation::Inside,
			Edemo_mapShanmenFormationAreaRelation::Boundary,
			Edemo_mapShanmenFormationAreaRelation::Outside });
	const auto First =
		Fdemo_mapShanmenFormationInfluenceReconciliationPlanner::PlanPrime(
			ReconcileSourceId, Scope);
	const auto Replay =
		Fdemo_mapShanmenFormationInfluenceReconciliationPlanner::PlanPrime(
			ReconcileSourceId, Scope);

	TestTrue(TEXT("Prime publishes a valid batch"), First.IsSuccess());
	TestEqual(TEXT("Prime applies every covered subject"),
		First.Batch.ApplyCount, 2);
	TestEqual(TEXT("Prime never removes"), First.Batch.RemoveCount, 0);
	TestEqual(TEXT("Inside subject is first"),
		First.Batch.Intents[0].SubjectEntityId, ReconcileSubject(1));
	TestEqual(TEXT("Boundary subject is covered"),
		First.Batch.Intents[1].SubjectEntityId, ReconcileSubject(2));
	TestEqual(TEXT("Every intent cites reconciliation evidence"),
		First.Batch.Intents[0].CauseId, First.Batch.ReconciliationId);
	TestEqual(TEXT("Exact replay keeps evidence identity"),
		First.Batch.ReconciliationId, Replay.Batch.ReconciliationId);
	TestEqual(TEXT("Exact replay keeps batch identity"),
		First.Batch.BatchId, Replay.Batch.BatchId);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationInfluenceReconcileSameScopeTest,
	"Shanmen.0_0_10.Product.FormationInfluenceReconciliation.SameScopeRebase",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationInfluenceReconcileSameScopeTest::RunTest(
	const FString&)
{
	const auto Area = MakeReconcileArea();
	const auto Previous = MakeReconcileScope(
		Area, { ReconcileSubject(1), ReconcileSubject(2), ReconcileSubject(3) },
		{ Edemo_mapShanmenFormationAreaRelation::Inside,
			Edemo_mapShanmenFormationAreaRelation::Boundary,
			Edemo_mapShanmenFormationAreaRelation::Outside });
	const auto Current = MakeReconcileScope(
		Area, { ReconcileSubject(1), ReconcileSubject(2), ReconcileSubject(4) },
		{ Edemo_mapShanmenFormationAreaRelation::Boundary,
			Edemo_mapShanmenFormationAreaRelation::Outside,
			Edemo_mapShanmenFormationAreaRelation::Inside });
	const auto Planned =
		Fdemo_mapShanmenFormationInfluenceReconciliationPlanner::PlanRebase(
			ReconcileSourceId, Previous, Current);

	TestTrue(TEXT("Same-scope Rebase validates"), Planned.IsSuccess());
	TestEqual(TEXT("Only departed coverage is removed"),
		Planned.Batch.RemoveCount, 1);
	TestEqual(TEXT("Only new coverage is applied"),
		Planned.Batch.ApplyCount, 1);
	TestEqual(TEXT("Removes precede applies"),
		Planned.Batch.Intents[0].Operation,
		Edemo_mapShanmenFormationInfluenceOperation::Remove);
	TestEqual(TEXT("Departed subject removed"),
		Planned.Batch.Intents[0].SubjectEntityId, ReconcileSubject(2));
	TestEqual(TEXT("New subject applied"),
		Planned.Batch.Intents[1].SubjectEntityId, ReconcileSubject(4));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationInfluenceReconcileScopeReplacementTest,
	"Shanmen.0_0_10.Product.FormationInfluenceReconciliation.ScopeReplacement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationInfluenceReconcileScopeReplacementTest::RunTest(
	const FString&)
{
	const auto Previous = MakeReconcileScope(
		MakeReconcileArea(1),
		{ ReconcileSubject(1), ReconcileSubject(2) },
		{ Edemo_mapShanmenFormationAreaRelation::Inside,
			Edemo_mapShanmenFormationAreaRelation::Outside });
	const auto Current = MakeReconcileScope(
		MakeReconcileArea(1, 10),
		{ ReconcileSubject(1), ReconcileSubject(2) },
		{ Edemo_mapShanmenFormationAreaRelation::Inside,
			Edemo_mapShanmenFormationAreaRelation::Boundary });
	const auto Planned =
		Fdemo_mapShanmenFormationInfluenceReconciliationPlanner::PlanRebase(
			ReconcileSourceId, Previous, Current);

	TestTrue(TEXT("Replacement Rebase validates"), Planned.IsSuccess());
	TestEqual(TEXT("Old covered scope is fully removed"),
		Planned.Batch.RemoveCount, 1);
	TestEqual(TEXT("New covered scope is fully applied"),
		Planned.Batch.ApplyCount, 2);
	TestEqual(TEXT("Same subject old scope removed first"),
		Planned.Batch.Intents[0].AreaId, Previous.Area.AreaId);
	TestEqual(TEXT("Same subject new scope reapplied"),
		Planned.Batch.Intents[1].AreaId, Current.Area.AreaId);
	TestNotEqual(TEXT("Old and new intent identities remain distinct"),
		Planned.Batch.Intents[0].IntentId,
		Planned.Batch.Intents[1].IntentId);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationInfluenceReconcileClearTest,
	"Shanmen.0_0_10.Product.FormationInfluenceReconciliation.ResetAndTerminal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationInfluenceReconcileClearTest::RunTest(const FString&)
{
	const auto Scope = MakeReconcileScope(
		MakeReconcileArea(),
		{ ReconcileSubject(1), ReconcileSubject(2), ReconcileSubject(3) },
		{ Edemo_mapShanmenFormationAreaRelation::Inside,
			Edemo_mapShanmenFormationAreaRelation::Boundary,
			Edemo_mapShanmenFormationAreaRelation::Outside });
	const auto Reset =
		Fdemo_mapShanmenFormationInfluenceReconciliationPlanner::PlanClear(
			Edemo_mapShanmenFormationInfluenceReconciliationMode::Reset,
			ReconcileSourceId, Scope);
	const auto Terminal =
		Fdemo_mapShanmenFormationInfluenceReconciliationPlanner::PlanClear(
			Edemo_mapShanmenFormationInfluenceReconciliationMode::Terminal,
			ReconcileSourceId, Scope);
	const auto EmptyScope = MakeReconcileScope(
		MakeReconcileArea(), { ReconcileSubject(9) },
		{ Edemo_mapShanmenFormationAreaRelation::Outside });
	const auto NoOp =
		Fdemo_mapShanmenFormationInfluenceReconciliationPlanner::PlanClear(
			Edemo_mapShanmenFormationInfluenceReconciliationMode::Reset,
			ReconcileSourceId, EmptyScope);

	TestTrue(TEXT("Reset and Terminal both validate"),
		Reset.IsSuccess() && Terminal.IsSuccess());
	TestEqual(TEXT("Reset removes all covered subjects"),
		Reset.Batch.RemoveCount, 2);
	TestEqual(TEXT("Terminal removes all covered subjects"),
		Terminal.Batch.RemoveCount, 2);
	TestNotEqual(TEXT("Reset and Terminal evidence remain distinct"),
		Reset.Batch.ReconciliationId, Terminal.Batch.ReconciliationId);
	TestTrue(TEXT("Clearing an uncovered scope is a sealed no-op"),
		NoOp.IsSuccess() && NoOp.Batch.IsNoOp());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationInfluenceReconcileFenceTest,
	"Shanmen.0_0_10.Product.FormationInfluenceReconciliation.FencesAndSeal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationInfluenceReconcileFenceTest::RunTest(const FString&)
{
	const auto Scope = MakeReconcileScope(
		MakeReconcileArea(),
		{ ReconcileSubject(1), ReconcileSubject(2) },
		{ Edemo_mapShanmenFormationAreaRelation::Inside,
			Edemo_mapShanmenFormationAreaRelation::Boundary });
	auto InvalidScope = Scope;
	InvalidScope.Coverage.ReceiptId.Invalidate();
	auto MismatchedScope = Scope;
	MismatchedScope.Area = MakeReconcileArea(2);
	auto ForeignScope = MakeReconcileScope(
		MakeReconcileArea(2),
		{ ReconcileSubject(1), ReconcileSubject(2) },
		{ Edemo_mapShanmenFormationAreaRelation::Inside,
			Edemo_mapShanmenFormationAreaRelation::Boundary });

	const auto SourceRejected =
		Fdemo_mapShanmenFormationInfluenceReconciliationPlanner::PlanPrime(
			FGuid(), Scope);
	const auto CurrentRejected =
		Fdemo_mapShanmenFormationInfluenceReconciliationPlanner::PlanPrime(
			ReconcileSourceId, InvalidScope);
	const auto PreviousRejected =
		Fdemo_mapShanmenFormationInfluenceReconciliationPlanner::PlanClear(
			Edemo_mapShanmenFormationInfluenceReconciliationMode::Reset,
			ReconcileSourceId, MismatchedScope);
	const auto ModeRejected =
		Fdemo_mapShanmenFormationInfluenceReconciliationPlanner::PlanClear(
			Edemo_mapShanmenFormationInfluenceReconciliationMode::Prime,
			ReconcileSourceId, Scope);
	const auto ScopeRejected =
		Fdemo_mapShanmenFormationInfluenceReconciliationPlanner::PlanRebase(
			ReconcileSourceId, Scope, ForeignScope);
	const auto Planned =
		Fdemo_mapShanmenFormationInfluenceReconciliationPlanner::PlanClear(
			Edemo_mapShanmenFormationInfluenceReconciliationMode::Reset,
			ReconcileSourceId, Scope);
	if (!TestTrue(TEXT("Mutation fixture plans"), Planned.IsSuccess()))
	{
		return false;
	}

	auto CountDrift = Planned.Batch;
	++CountDrift.RemoveCount;
	auto OrderDrift = Planned.Batch;
	OrderDrift.Intents.Swap(0, 1);
	auto EvidenceDrift = Planned.Batch;
	EvidenceDrift.ReconciliationId.Invalidate();
	auto ShapeDrift = Planned.Batch;
	ShapeDrift.Current = Scope;
	auto IntentDrift = Planned.Batch;
	IntentDrift.Intents[0].CauseId.Invalidate();
	auto BatchDrift = Planned.Batch;
	BatchDrift.BatchId.Invalidate();

	TestEqual(TEXT("Missing source rejected"), SourceRejected.Status,
		Edemo_mapShanmenFormationInfluenceReconciliationStatus::SourceInvalid);
	TestEqual(TEXT("Invalid current scope rejected"), CurrentRejected.Status,
		Edemo_mapShanmenFormationInfluenceReconciliationStatus::CurrentScopeInvalid);
	TestEqual(TEXT("Mismatched previous scope rejected"), PreviousRejected.Status,
		Edemo_mapShanmenFormationInfluenceReconciliationStatus::PreviousScopeInvalid);
	TestEqual(TEXT("Clear mode is closed"), ModeRejected.Status,
		Edemo_mapShanmenFormationInfluenceReconciliationStatus::ModeInvalid);
	TestEqual(TEXT("Rebase cannot cross deployment authority"),
		ScopeRejected.Status,
		Edemo_mapShanmenFormationInfluenceReconciliationStatus::ScopeMismatch);
	TestFalse(TEXT("Count drift rejected"), CountDrift.IsValid());
	TestFalse(TEXT("Canonical order drift rejected"), OrderDrift.IsValid());
	TestFalse(TEXT("Evidence identity drift rejected"), EvidenceDrift.IsValid());
	TestFalse(TEXT("Mode/scope shape drift rejected"), ShapeDrift.IsValid());
	TestFalse(TEXT("Nested intent drift rejected"), IntentDrift.IsValid());
	TestFalse(TEXT("Batch identity drift rejected"), BatchDrift.IsValid());
	TestTrue(TEXT("Original batch remains valid"), Planned.Batch.IsValid());
	return true;
}

#endif
