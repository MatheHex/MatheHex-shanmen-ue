#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ShanmenActionOrchestrator.h"
#include "ShanmenBasicSwordExecution.h"
#include "ShanmenCombatResolver.h"
#include "ShanmenCombatTags.h"
#include "ShanmenVitalityAuthority.h"
#include "demo_mapAttributeComponent.h"
#include "demo_mapAttributeDefinitions.h"
#include "demo_mapPlayerHealthComponent.h"

namespace
{
	const FGuid AdapterRunId(0x54200001, 0, 0, 1);
	const FGuid AdapterOwnerId(0x54200002, 0, 0, 1);
	const FGuid AdapterSourceId(0x54200003, 0, 0, 1);
	const FGuid AdapterTargetId(0x54200004, 0, 0, 1);
	const FGuid OtherTargetId(0x54200005, 0, 0, 1);

	struct FPlayerVitalityFixture
	{
		Udemo_mapAttributeComponent* Attributes = nullptr;
		Udemo_mapPlayerHealthComponent* Health = nullptr;
		bool bReady = false;

		explicit FPlayerVitalityFixture(float MaximumVitality = 10.0f)
		{
			Attributes = NewObject<Udemo_mapAttributeComponent>(GetTransientPackage());
			Health = NewObject<Udemo_mapPlayerHealthComponent>(GetTransientPackage());
			bReady = Attributes != nullptr
				&& Health != nullptr
				&& MaximumVitality >= 5.0f
				&& Attributes->SetBaseValue(
					Fdemo_mapAttributeIds::Primary03,
					MaximumVitality - 5.0f)
				&& Health->BindAttributeComponent(Attributes, true)
				&& Health->TryBindCombatEntity(AdapterTargetId);
		}
	};

	FShanmenCombatActionSnapshot MakeAdapterAction()
	{
		FShanmenCombatActionCapture Capture;
		Capture.RunId = AdapterRunId;
		Capture.OwnerId = AdapterOwnerId;
		Capture.SourceEntityId = AdapterSourceId;
		Capture.SourceItemInstanceId = FGuid(0x54200006, 0, 0, 1);
		Capture.ActionDefinitionId = FShanmenBasicSwordDefinition::CanonicalActionDefinitionId();
		Capture.Content.Version = TEXT("0.0.10.P4.2");
		Capture.Content.Digest = TEXT("TEST-DIGEST-P4.2-PLAYER-VITALITY");
		Capture.SourceTags.AddTag(FShanmenCombatNativeTags::SourcePlayer());
		Capture.ActivationId = FShanmenCombatIdFactory::MakeActivationId(
			Capture.RunId,
			Capture.SourceEntityId,
			Capture.ActionDefinitionId,
			1);

		FShanmenCombatActionSnapshot Action;
		check(FShanmenCombatActionSnapshot::TryCapture(Capture, Action));
		return Action;
	}

	FShanmenImpactRequest MakeAdapterRequest(
		const FShanmenTargetVitalitySnapshot& Vitality,
		float RawDamage,
		int32 HitOrdinal = 0)
	{
		FShanmenImpactRequest Request;
		Request.Action = MakeAdapterAction();
		Request.Candidate.ActivationId = Request.Action.GetActivationId();
		Request.Candidate.SourceEntityId = Request.Action.GetSourceEntityId();
		Request.Candidate.TargetEntityId = AdapterTargetId;
		Request.Candidate.DetectorId = TEXT("Detector.Product.PlayerVitality.Test");
		Request.Candidate.DetectorKind = EShanmenHitDetectorKind::WeaponTrajectory;
		Request.Candidate.HitOrdinal = HitOrdinal;
		Request.Candidate.HitLocation = FVector(10.0, 0.0, 1.0);
		Request.Candidate.HitNormal = FVector::BackwardVector;
		Request.Damage.FormulaId = TEXT("Combat.Formula.Product.PlayerVitality.Test");
		Request.Damage.RawDamage = RawDamage;
		Request.Damage.DamageTags.AddTag(FShanmenCombatNativeTags::DamagePhysicalSlash());
		Request.TargetVitality = Vitality;
		Request.ImpactId = FShanmenCombatIdFactory::MakeImpactId(
			Request.Action.GetRunId(),
			Request.Candidate.ActivationId,
			Request.Candidate.DetectorId,
			Request.Candidate.TargetEntityId,
			Request.Candidate.HitOrdinal);
		check(Request.IsValid());
		return Request;
	}

	FShanmenVitalityCommitCommand MakeAdapterCommand(
		const FShanmenImpactRequest& Request)
	{
		const FShanmenImpactResult Result = FShanmenDefenseResolver::Resolve(Request);
		FShanmenVitalityCommitCommand Command;
		check(FShanmenVitalityCommitCommand::TryCreate(Request, Result, Command));
		return Command;
	}

	FShanmenBasicSwordDefinition MakeSwordDefinition()
	{
		FShanmenBasicSwordDefinitionCapture Capture;
		Capture.ActionDefinitionId = FShanmenBasicSwordDefinition::CanonicalActionDefinitionId();
		Capture.DetectorId = TEXT("Detector.Weapon.Main");
		Capture.FormulaId = TEXT("Combat.Formula.Sword.Basic01.r1");
		Capture.BaseDamage = 20.0f;
		Capture.AttackPowerCoefficient = 0.5f;
		Capture.DamageTags.AddTag(FShanmenCombatNativeTags::DamagePhysicalSlash());
		Capture.RequiredTargetTags.AddTag(FShanmenCombatNativeTags::TargetLiving());
		Capture.bRejectSelf = true;
		FShanmenBasicSwordDefinition Definition;
		check(FShanmenBasicSwordDefinition::TryCapture(Capture, Definition));
		return Definition;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenPlayerVitalityBindingAndLegacyMutationTest,
	"Shanmen.0_0_10.Product.PlayerVitality.BindingAndLegacyMutation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenPlayerVitalityBindingAndLegacyMutationTest::RunTest(const FString&)
{
	FPlayerVitalityFixture Fixture;
	TestTrue(TEXT("Product vitality fixture initializes"), Fixture.bReady);
	if (!Fixture.bReady) return false;
	TestFalse(TEXT("Invalid entity identity is rejected"),
		Fixture.Health->TryBindCombatEntity(FGuid()));
	TestTrue(TEXT("Same stable entity bind is idempotent"),
		Fixture.Health->TryBindCombatEntity(AdapterTargetId));
	TestFalse(TEXT("Live health authority cannot silently change entity"),
		Fixture.Health->TryBindCombatEntity(OtherTargetId));

	FShanmenTargetVitalitySnapshot Snapshot;
	TestTrue(TEXT("Product health exposes the initial combat snapshot"),
		Fixture.Health->TryCaptureCombatVitalitySnapshot(Snapshot));
	TestTrue(TEXT("Initial product snapshot is 10/10 at revision zero"),
		FMath::IsNearlyEqual(Snapshot.CurrentVitality, 10.0f)
			&& FMath::IsNearlyEqual(Snapshot.MaximumVitality, 10.0f)
			&& Snapshot.AuthorityRevision == 0);

	TestEqual(TEXT("Legacy damage still reports its integer result"),
		Fixture.Health->ApplyIncomingDamage(2.0f),
		2);
	TestEqual(TEXT("Legacy damage advances the shared revision"),
		Fixture.Health->GetCombatAuthorityRevision(),
		static_cast<int64>(1));
	TestEqual(TEXT("Legacy healing shares the same mutation gate"),
		Fixture.Health->ApplyHealing(1),
		1);
	TestEqual(TEXT("Healing advances the shared revision"),
		Fixture.Health->GetCombatAuthorityRevision(),
		static_cast<int64>(2));
	TestTrue(TEXT("Max-health attribute mutation reaches the same authority"),
		Fixture.Attributes->SetBaseValue(Fdemo_mapAttributeIds::Primary03, 7.0f));
	TestEqual(TEXT("Max-health mutation advances revision"),
		Fixture.Health->GetCombatAuthorityRevision(),
		static_cast<int64>(3));
	Fixture.Health->RestoreCurrentVitalityAfterItemUseRollback(8.5f);
	TestEqual(TEXT("Item rollback advances revision"),
		Fixture.Health->GetCombatAuthorityRevision(),
		static_cast<int64>(4));
	TestTrue(TEXT("Float vitality remains the sole truth"),
		FMath::IsNearlyEqual(Fixture.Health->GetCurrentVitality(), 8.5f)
			&& FMath::IsNearlyEqual(Fixture.Health->GetMaximumVitality(), 12.0f));
	TestTrue(TEXT("Legacy integer view projects a positive fraction upward"),
		Fixture.Health->GetCurrentHealth() == 9
			&& Fixture.Health->GetMaxHealth() == 12);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenPlayerVitalityImpactExactlyOnceTest,
	"Shanmen.0_0_10.Product.PlayerVitality.ImpactExactlyOnce",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenPlayerVitalityImpactExactlyOnceTest::RunTest(const FString&)
{
	FPlayerVitalityFixture Fixture;
	TestTrue(TEXT("Product vitality fixture initializes"), Fixture.bReady);
	if (!Fixture.bReady) return false;
	FShanmenTargetVitalitySnapshot Snapshot;
	check(Fixture.Health->TryCaptureCombatVitalitySnapshot(Snapshot));
	const FShanmenVitalityCommitCommand Command = MakeAdapterCommand(
		MakeAdapterRequest(Snapshot, 1.5f));

	const FShanmenVitalityCommitResult First = Fixture.Health->CommitCombatImpact(Command);
	TestEqual(TEXT("Canonical product delivery commits"),
		First.Status,
		EShanmenVitalityCommitStatus::Committed);
	TestTrue(TEXT("Product stores exact fractional vitality"),
		FMath::IsNearlyEqual(Fixture.Health->GetCurrentVitality(), 8.5f));
	TestTrue(TEXT("First delivery emits one compatibility signal"),
		Fixture.Health->GetPositiveDamageBroadcastCountForAutomation() == 1
			&& Fixture.Health->GetCurrentHealth() == 9);

	const FShanmenVitalityCommitResult Replay = Fixture.Health->CommitCombatImpact(Command);
	TestEqual(TEXT("Duplicate delivery is an idempotent replay"),
		Replay.Status,
		EShanmenVitalityCommitStatus::AlreadyCommitted);
	TestTrue(TEXT("Replay neither writes nor broadcasts twice"),
		FMath::IsNearlyEqual(Fixture.Health->GetCurrentVitality(), 8.5f)
			&& Fixture.Health->GetCombatAuthorityRevision() == 1
			&& Fixture.Health->NumCommittedCombatImpacts() == 1
			&& Fixture.Health->GetPositiveDamageBroadcastCountForAutomation() == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenPlayerVitalityStaleAfterLegacyMutationTest,
	"Shanmen.0_0_10.Product.PlayerVitality.StaleAfterLegacyMutation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenPlayerVitalityStaleAfterLegacyMutationTest::RunTest(const FString&)
{
	FPlayerVitalityFixture Fixture;
	TestTrue(TEXT("Product vitality fixture initializes"), Fixture.bReady);
	if (!Fixture.bReady) return false;
	FShanmenTargetVitalitySnapshot Original;
	check(Fixture.Health->TryCaptureCombatVitalitySnapshot(Original));
	const FShanmenVitalityCommitCommand Stale = MakeAdapterCommand(
		MakeAdapterRequest(Original, 2.0f));
	check(Fixture.Health->ApplyIncomingDamage(1.0f) == 1);

	const FShanmenVitalityCommitResult Rejected = Fixture.Health->CommitCombatImpact(Stale);
	TestTrue(TEXT("Legacy write makes the captured Impact stale"),
		Rejected.Status == EShanmenVitalityCommitStatus::Rejected
			&& Rejected.Error == EShanmenVitalityCommitError::StaleSnapshot);
	TestTrue(TEXT("Rejected stale command changes no product state or signal"),
		FMath::IsNearlyEqual(Fixture.Health->GetCurrentVitality(), 9.0f)
			&& Fixture.Health->GetCombatAuthorityRevision() == 1
			&& Fixture.Health->GetPositiveDamageBroadcastCountForAutomation() == 1);

	FShanmenTargetVitalitySnapshot Fresh;
	check(Fixture.Health->TryCaptureCombatVitalitySnapshot(Fresh));
	const FShanmenVitalityCommitResult Retried = Fixture.Health->CommitCombatImpact(
		MakeAdapterCommand(MakeAdapterRequest(Fresh, 2.0f)));
	TestTrue(TEXT("Fresh resolution commits after legacy traffic"),
		Retried.Status == EShanmenVitalityCommitStatus::Committed
			&& FMath::IsNearlyEqual(Fixture.Health->GetCurrentVitality(), 7.0f)
			&& Fixture.Health->GetCombatAuthorityRevision() == 2
			&& Fixture.Health->GetPositiveDamageBroadcastCountForAutomation() == 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenPlayerVitalityBasicSwordVerticalSliceTest,
	"Shanmen.0_0_10.Product.PlayerVitality.BasicSwordVerticalSlice",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenPlayerVitalityBasicSwordVerticalSliceTest::RunTest(const FString&)
{
	FPlayerVitalityFixture Fixture(100.0f);
	TestTrue(TEXT("Product vitality fixture initializes"), Fixture.bReady);
	if (!Fixture.bReady) return false;
	const FShanmenCombatActionSnapshot Action = MakeAdapterAction();
	FShanmenActionOrchestrator Runtime;
	FShanmenActionTransitionReceipt Phase;
	check(FShanmenActionOrchestrator::TryStart(Action, Runtime, Phase));
	check(Runtime.TryAdvance(EShanmenCombatActionPhase::Startup, Phase));

	FShanmenBasicSwordOffenseSnapshot Offense;
	check(FShanmenBasicSwordOffenseSnapshot::TryCapture(60.0f, Offense));
	FShanmenBasicSwordExecution Sword;
	check(FShanmenBasicSwordExecution::TryCreate(
		Action,
		MakeSwordDefinition(),
		Offense,
		Sword));
	FShanmenWorldHitContext Context;
	check(Sword.TryBeginEmission(Runtime, Context));

	FShanmenHitCandidate Candidate;
	Candidate.ActivationId = Context.GetAction().GetActivationId();
	Candidate.SourceEntityId = Context.GetAction().GetSourceEntityId();
	Candidate.TargetEntityId = AdapterTargetId;
	Candidate.DetectorId = Context.GetDetectorId();
	Candidate.DetectorKind = Context.GetDetectorKind();
	Candidate.HitOrdinal = Context.GetHitOrdinal();
	Candidate.HitLocation = FVector(100.0, 0.0, 50.0);
	Candidate.HitNormal = FVector::BackwardVector;

	FShanmenDefenseSnapshot Defense;
	Defense.TargetTags.AddTag(FShanmenCombatNativeTags::TargetLiving());
	FShanmenDefenseLayer& Guard = Defense.Layers.AddDefaulted_GetRef();
	Guard.LayerId = FGuid(0x54200007, 0, 0, 1);
	Guard.RuleId = TEXT("Defense.Guard.Product.P4.2");
	Guard.Operation = EShanmenDefenseOperation::ReduceFraction;
	Guard.Order = FShanmenDefenseOrder::Guard;
	Guard.Magnitude = 0.2f;
	Guard.LayerTags.AddTag(FShanmenCombatNativeTags::DefenseGuard());

	FShanmenTargetVitalitySnapshot Vitality;
	check(Fixture.Health->TryCaptureCombatVitalitySnapshot(Vitality));
	FShanmenBasicSwordImpactReceipt Impact;
	TestTrue(TEXT("Product target resolves the BasicSword candidate"),
		Sword.TryResolveCandidate(Runtime, Candidate, Vitality, Defense, Impact));
	TestTrue(TEXT("Sword formula and guard resolve 50 raw to 40 final"),
		FMath::IsNearlyEqual(Impact.GetResult().RawDamage, 50.0f)
			&& FMath::IsNearlyEqual(Impact.GetResult().PreventedDamage, 10.0f)
			&& FMath::IsNearlyEqual(Impact.GetResult().FinalDamage, 40.0f));

	FShanmenVitalityCommitCommand Command;
	check(FShanmenVitalityCommitCommand::TryCreate(
		Impact.GetRequest(),
		Impact.GetResult(),
		Command));
	const FShanmenVitalityCommitResult Committed = Fixture.Health->CommitCombatImpact(Command);
	TestTrue(TEXT("BasicSword mutates the real product host exactly once"),
		Committed.Status == EShanmenVitalityCommitStatus::Committed
			&& FMath::IsNearlyEqual(Fixture.Health->GetCurrentVitality(), 60.0f)
			&& Fixture.Health->GetCombatAuthorityRevision() == 1
			&& Fixture.Health->GetPositiveDamageBroadcastCountForAutomation() == 1);
	TestTrue(TEXT("BasicSword replay cannot double-write product health"),
		Fixture.Health->CommitCombatImpact(Command).Status
				== EShanmenVitalityCommitStatus::AlreadyCommitted
			&& FMath::IsNearlyEqual(Fixture.Health->GetCurrentVitality(), 60.0f)
			&& Fixture.Health->GetCombatAuthorityRevision() == 1
			&& Fixture.Health->GetPositiveDamageBroadcastCountForAutomation() == 1);
	return true;
}

#endif
