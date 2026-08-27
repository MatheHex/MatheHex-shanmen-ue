#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ShanmenCombatResolver.h"
#include "ShanmenCombatTags.h"
#include "UObject/UnrealType.h"

#include <limits>

namespace
{
	FShanmenDefenseLayer MakeLayer(
		uint32 Id,
		FName RuleId,
		EShanmenDefenseOperation Operation,
		int32 Order,
		float Magnitude,
		const FGameplayTag& LayerTag)
	{
		FShanmenDefenseLayer Layer;
		Layer.LayerId = FGuid(0, 0, 0, Id);
		Layer.RuleId = RuleId;
		Layer.Operation = Operation;
		Layer.Order = Order;
		Layer.Magnitude = Magnitude;
		Layer.LayerTags.AddTag(LayerTag);
		return Layer;
	}

	FShanmenImpactRequest MakeValidRequest(float RawDamage = 100.0f)
	{
		FShanmenCombatActionCapture Capture;
		Capture.RunId = FGuid(1, 2, 3, 4);
		Capture.OwnerId = FGuid(5, 6, 7, 8);
		Capture.SourceEntityId = FGuid(9, 10, 11, 12);
		Capture.ActionDefinitionId = TEXT("Combat.Action.Sword.Basic01");
		Capture.Content.Version = TEXT("0.0.10.P0.1");
		Capture.Content.Digest = TEXT("TEST-DIGEST-P0.1");
		Capture.SourceTags.AddTag(FShanmenCombatNativeTags::SourcePlayer());
		Capture.ActivationId = FShanmenCombatIdFactory::MakeActivationId(
			Capture.RunId,
			Capture.SourceEntityId,
			Capture.ActionDefinitionId,
			1);

		FShanmenImpactRequest Request;
		const bool bCaptured = FShanmenCombatActionSnapshot::TryCapture(Capture, Request.Action);
		check(bCaptured);

		Request.Candidate.ActivationId = Request.Action.GetActivationId();
		Request.Candidate.SourceEntityId = Request.Action.GetSourceEntityId();
		Request.Candidate.TargetEntityId = FGuid(13, 14, 15, 16);
		Request.Candidate.DetectorId = TEXT("Detector.Weapon.Main");
		Request.Candidate.DetectorKind = EShanmenHitDetectorKind::WeaponTrajectory;
		Request.Candidate.HitOrdinal = 0;

		Request.Damage.FormulaId = TEXT("DamageFormula.Test.Constant");
		Request.Damage.RawDamage = RawDamage;
		Request.Damage.DamageTags.AddTag(FShanmenCombatNativeTags::DamagePhysicalSlash());
		Request.TargetVitality.CurrentVitality = 100.0f;
		Request.TargetVitality.MaximumVitality = 100.0f;
		Request.Defense.TargetTags.AddTag(FShanmenCombatNativeTags::TargetLiving());

		Request.ImpactId = FShanmenCombatIdFactory::MakeImpactId(
			Request.Action.GetRunId(),
			Request.Action.GetActivationId(),
			Request.Candidate.DetectorId,
			Request.Candidate.TargetEntityId,
			Request.Candidate.HitOrdinal);
		return Request;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenCombatDeterministicIdentityTest,
	"Shanmen.0_0_10.CombatCore.DeterministicIdentity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenCombatDeterministicIdentityTest::RunTest(const FString&)
{
	const FShanmenImpactRequest Request = MakeValidRequest();
	const FGuid SameImpact = FShanmenCombatIdFactory::MakeImpactId(
		Request.Action.GetRunId(),
		Request.Action.GetActivationId(),
		Request.Candidate.DetectorId,
		Request.Candidate.TargetEntityId,
		Request.Candidate.HitOrdinal);
	const FGuid DifferentOrdinal = FShanmenCombatIdFactory::MakeImpactId(
		Request.Action.GetRunId(),
		Request.Action.GetActivationId(),
		Request.Candidate.DetectorId,
		Request.Candidate.TargetEntityId,
		Request.Candidate.HitOrdinal + 1);

	TestTrue(TEXT("Canonical impact id is valid"), Request.ImpactId.IsValid());
	TestTrue(TEXT("Same canonical values replay the same id"), Request.ImpactId == SameImpact);
	TestTrue(TEXT("Hit ordinal participates in identity"), Request.ImpactId != DifferentOrdinal);

	FShanmenHitCandidate SelfCandidate = Request.Candidate;
	SelfCandidate.TargetEntityId = SelfCandidate.SourceEntityId;
	TestTrue(TEXT("Candidate remains geometric; target policy decides self-target legality"), SelfCandidate.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenCombatImpactLedgerTest,
	"Shanmen.0_0_10.CombatCore.ImpactLedger",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenCombatImpactLedgerTest::RunTest(const FString&)
{
	const FShanmenImpactRequest Request = MakeValidRequest();
	FShanmenImpactLedger Ledger;

	TestTrue(TEXT("First valid impact is accepted"), Ledger.TryAccept(Request));
	TestFalse(TEXT("Replay is rejected"), Ledger.TryAccept(Request));
	TestEqual(TEXT("Only one accepted identity is retained"), Ledger.Num(), 1);
	TestTrue(TEXT("Accepted identity can be queried"), Ledger.Contains(Request.ImpactId));
	Ledger.Reset();
	TestEqual(TEXT("Run boundary clears the ledger"), Ledger.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenCombatImpactIdentityIntegrityTest,
	"Shanmen.0_0_10.CombatCore.ImpactIdentityIntegrity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenCombatImpactIdentityIntegrityTest::RunTest(const FString&)
{
	FShanmenImpactRequest Tampered = MakeValidRequest();
	Tampered.ImpactId = FGuid(90, 91, 92, 93);
	FShanmenImpactLedger Ledger;

	TestFalse(TEXT("Arbitrary ImpactId does not match canonical candidate identity"), Tampered.IsValid());
	TestFalse(TEXT("Ledger cannot accept a structurally mismatched request"), Ledger.TryAccept(Tampered));
	TestFalse(TEXT("Resolver fails closed on mismatched identity"), FShanmenDefenseResolver::Resolve(Tampered).bAccepted);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenCombatDefenseOrderTest,
	"Shanmen.0_0_10.CombatCore.DefenseOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenCombatDefenseOrderTest::RunTest(const FString&)
{
	FShanmenImpactRequest Request = MakeValidRequest(100.0f);

	FShanmenDefenseLayer Armor = MakeLayer(
		3, TEXT("Defense.Armor.Physical"), EShanmenDefenseOperation::ReduceFraction,
		FShanmenDefenseOrder::Resistance, 0.20f, FShanmenCombatNativeTags::DefenseArmor());
	Armor.RequiredDamageTags.AddTag(FShanmenCombatNativeTags::DamagePhysical());

	FShanmenDefenseLayer Guard = MakeLayer(
		1, TEXT("Defense.Guard.Weapon"), EShanmenDefenseOperation::ReduceFraction,
		FShanmenDefenseOrder::Guard, 0.25f, FShanmenCombatNativeTags::DefenseGuard());
	Guard.RequiredSourceTags.AddTag(FShanmenCombatNativeTags::SourcePlayer());
	Guard.RequiredTargetTags.AddTag(FShanmenCombatNativeTags::TargetLiving());

	FShanmenDefenseLayer Shield = MakeLayer(
		2, TEXT("Defense.Shield.Spirit"), EShanmenDefenseOperation::AbsorbPoints,
		FShanmenDefenseOrder::Shield, 10.0f, FShanmenCombatNativeTags::DefenseShield());

	// Deliberately provide layers out of order; the resolver must canonicalize them.
	Request.Defense.Layers = { Armor, Shield, Guard };
	const FShanmenImpactResult Result = FShanmenDefenseResolver::Resolve(Request);

	TestTrue(TEXT("Valid impact is accepted"), Result.bAccepted);
	TestTrue(TEXT("Layered defense reports mitigation"), Result.Outcome == EShanmenDefenseOutcome::Mitigated);
	TestEqual(TEXT("Three layers triggered"), Result.TriggeredLayers.Num(), 3);
	TestEqual(TEXT("Guard resolves first"), Result.TriggeredLayers[0].RuleId, FName(TEXT("Defense.Guard.Weapon")));
	TestTrue(TEXT("Guard prevents 25"), FMath::IsNearlyEqual(Result.TriggeredLayers[0].PreventedDamage, 25.0f));
	TestTrue(TEXT("Shield prevents 10"), FMath::IsNearlyEqual(Result.TriggeredLayers[1].PreventedDamage, 10.0f));
	TestTrue(TEXT("Armor prevents 13"), FMath::IsNearlyEqual(Result.TriggeredLayers[2].PreventedDamage, 13.0f));
	TestTrue(TEXT("Final vitality damage is 52"), FMath::IsNearlyEqual(Result.FinalDamage, 52.0f));
	TestTrue(TEXT("Layer audit conserves all damage"), Result.IsConserved());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenCombatActiveDefensePrecedenceTest,
	"Shanmen.0_0_10.CombatCore.ActiveDefensePrecedence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenCombatActiveDefensePrecedenceTest::RunTest(const FString&)
{
	FShanmenImpactRequest Dodge = MakeValidRequest();
	FShanmenDefenseLayer PerfectGuard = MakeLayer(
		2, TEXT("Defense.PerfectGuard"), EShanmenDefenseOperation::PreventAll,
		FShanmenDefenseOrder::PerfectGuard, 1.0f, FShanmenCombatNativeTags::DefensePerfectGuard());
	FShanmenDefenseLayer Evade = MakeLayer(
		1, TEXT("Defense.Evade"), EShanmenDefenseOperation::PreventAll,
		FShanmenDefenseOrder::Avoidance, 1.0f, FShanmenCombatNativeTags::DefenseEvade());
	Dodge.Defense.Layers = { PerfectGuard, Evade };

	const FShanmenImpactResult DodgeResult = FShanmenDefenseResolver::Resolve(Dodge);
	TestTrue(TEXT("Evade has highest precedence"), DodgeResult.Outcome == EShanmenDefenseOutcome::Evaded);
	TestEqual(TEXT("Only the first full prevention triggers"), DodgeResult.TriggeredLayers.Num(), 1);
	TestTrue(TEXT("Evade records all prevented damage"), FMath::IsNearlyEqual(DodgeResult.PreventedDamage, 100.0f));
	TestTrue(TEXT("Evade path conserves damage"), DodgeResult.IsConserved());

	FShanmenImpactRequest GuardOnly = MakeValidRequest();
	GuardOnly.Defense.Layers.Add(PerfectGuard);
	const FShanmenImpactResult GuardResult = FShanmenDefenseResolver::Resolve(GuardOnly);
	TestTrue(TEXT("Perfect guard remains distinguishable"), GuardResult.Outcome == EShanmenDefenseOutcome::PerfectGuarded);
	TestTrue(TEXT("Perfect guard path conserves damage"), GuardResult.IsConserved());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenCombatDamageTagFilteringTest,
	"Shanmen.0_0_10.CombatCore.DamageTagFiltering",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenCombatDamageTagFilteringTest::RunTest(const FString&)
{
	FShanmenImpactRequest Request = MakeValidRequest();
	FShanmenDefenseLayer SpiritOnly = MakeLayer(
		1, TEXT("Defense.Resistance.Spirit"), EShanmenDefenseOperation::ReduceFraction,
		FShanmenDefenseOrder::Resistance, 0.90f, FShanmenCombatNativeTags::DefenseArmor());
	SpiritOnly.RequiredDamageTags.AddTag(FShanmenCombatNativeTags::DamageSpirit());

	FShanmenDefenseLayer Physical = MakeLayer(
		2, TEXT("Defense.Resistance.Physical"), EShanmenDefenseOperation::ReduceFraction,
		FShanmenDefenseOrder::Resistance, 0.50f, FShanmenCombatNativeTags::DefenseArmor());
	Physical.RequiredDamageTags.AddTag(FShanmenCombatNativeTags::DamagePhysical());
	Request.Defense.Layers = { SpiritOnly, Physical };

	const FShanmenImpactResult Result = FShanmenDefenseResolver::Resolve(Request);
	TestEqual(TEXT("Only matching physical resistance triggers"), Result.TriggeredLayers.Num(), 1);
	TestEqual(TEXT("Physical parent tag matches slash damage"), Result.TriggeredLayers[0].RuleId, FName(TEXT("Defense.Resistance.Physical")));
	TestTrue(TEXT("Filtered resistance leaves 50 damage"), FMath::IsNearlyEqual(Result.FinalDamage, 50.0f));
	TestTrue(TEXT("Filtered result conserves damage"), Result.IsConserved());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenCombatLethalInterceptionTest,
	"Shanmen.0_0_10.CombatCore.LethalInterception",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenCombatLethalInterceptionTest::RunTest(const FString&)
{
	FShanmenImpactRequest Request = MakeValidRequest(100.0f);
	Request.TargetVitality.CurrentVitality = 30.0f;
	Request.TargetVitality.MaximumVitality = 100.0f;

	FShanmenDefenseLayer HeartMirror = MakeLayer(
		1, TEXT("Defense.Artifact.HeartMirror"), EShanmenDefenseOperation::PreventLethal,
		FShanmenDefenseOrder::LethalInterception, 1.0f, FShanmenCombatNativeTags::DefenseLethalIntercept());
	HeartMirror.SourceInstanceId = FGuid(21, 22, 23, 24);
	HeartMirror.bRequiresCommitOnTrigger = true;
	Request.Defense.Layers.Add(HeartMirror);

	const FShanmenImpactResult Result = FShanmenDefenseResolver::Resolve(Request);
	TestTrue(TEXT("Lethal hit is mitigated"), Result.Outcome == EShanmenDefenseOutcome::Mitigated);
	TestTrue(TEXT("Vitality floor leaves exactly one point"), FMath::IsNearlyEqual(Result.FinalDamage, 29.0f));
	TestTrue(TEXT("Artifact records 71 prevented damage"), FMath::IsNearlyEqual(Result.PreventedDamage, 71.0f));
	TestEqual(TEXT("Artifact trigger is auditable"), Result.TriggeredLayers.Num(), 1);
	TestTrue(TEXT("Artifact trigger requests an external item commit"), Result.TriggeredLayers[0].bRequiresCommit);
	TestTrue(TEXT("Artifact source identity is preserved"), Result.TriggeredLayers[0].SourceInstanceId == HeartMirror.SourceInstanceId);
	TestTrue(TEXT("Lethal interception conserves damage"), Result.IsConserved());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenCombatSnapshotReadOnlyTest,
	"Shanmen.0_0_10.CombatCore.ActionSnapshotReadOnly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenCombatSnapshotReadOnlyTest::RunTest(const FString&)
{
	const UScriptStruct* Struct = FShanmenCombatActionSnapshot::StaticStruct();
	int32 PropertyCount = 0;
	for (TFieldIterator<FProperty> It(Struct); It; ++It)
	{
		++PropertyCount;
		TestTrue(*FString::Printf(TEXT("%s is Blueprint-read-only"), *It->GetName()), It->HasAnyPropertyFlags(CPF_BlueprintReadOnly));
		TestTrue(*FString::Printf(TEXT("%s is edit-const"), *It->GetName()), It->HasAnyPropertyFlags(CPF_EditConst));
	}
	TestEqual(TEXT("All eight frozen fields are reflected"), PropertyCount, 8);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenCombatInvalidRequestTest,
	"Shanmen.0_0_10.CombatCore.InvalidRequestFailsClosed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenCombatInvalidRequestTest::RunTest(const FString&)
{
	FShanmenImpactRequest Request = MakeValidRequest();
	Request.Damage.RawDamage = std::numeric_limits<float>::quiet_NaN();
	const FShanmenImpactResult Result = FShanmenDefenseResolver::Resolve(Request);
	TestFalse(TEXT("Invalid numeric input is not accepted"), Result.bAccepted);
	TestTrue(TEXT("Invalid request remains explicit"), Result.Outcome == EShanmenDefenseOutcome::Invalid);
	TestTrue(TEXT("Invalid request cannot apply vitality damage"), FMath::IsNearlyZero(Result.FinalDamage));

	FShanmenImpactRequest MissingCommitSource = MakeValidRequest();
	FShanmenDefenseLayer ConsumableLayer = MakeLayer(
		1, TEXT("Defense.Artifact.Consumable"), EShanmenDefenseOperation::PreventLethal,
		FShanmenDefenseOrder::LethalInterception, 1.0f, FShanmenCombatNativeTags::DefenseLethalIntercept());
	ConsumableLayer.bRequiresCommitOnTrigger = true;
	MissingCommitSource.Defense.Layers.Add(ConsumableLayer);
	TestFalse(TEXT("A commit-triggering layer requires an exact source instance"), MissingCommitSource.IsValid());

	FShanmenImpactRequest DuplicateLayerIdentity = MakeValidRequest();
	const FShanmenDefenseLayer RepeatedLayer = MakeLayer(
		2, TEXT("Defense.Duplicate"), EShanmenDefenseOperation::AbsorbPoints,
		FShanmenDefenseOrder::Shield, 10.0f, FShanmenCombatNativeTags::DefenseShield());
	DuplicateLayerIdentity.Defense.Layers = { RepeatedLayer, RepeatedLayer };
	TestFalse(TEXT("Defense snapshot rejects duplicate layer identities"), DuplicateLayerIdentity.IsValid());
	return true;
}

#endif
