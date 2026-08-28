#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ShanmenBasicSwordExecution.h"
#include "ShanmenBasicSwordGameplayAbility.h"
#include "ShanmenCombatRuntimeTags.h"
#include "ShanmenCombatTags.h"

namespace
{
	const FGuid SwordRunId(0x53100001, 0, 0, 1);
	const FGuid SwordSourceEntityId(0x53100002, 0, 0, 1);
	const FGuid SwordTargetA(0x53100003, 0, 0, 1);
	const FGuid SwordTargetB(0x53100004, 0, 0, 1);

	FShanmenCombatActionSnapshot MakeSwordAction()
	{
		FShanmenCombatActionCapture Capture;
		Capture.RunId = SwordRunId;
		Capture.OwnerId = FGuid(0x53100005, 0, 0, 1);
		Capture.SourceEntityId = SwordSourceEntityId;
		Capture.SourceItemInstanceId = FGuid(0x53100006, 0, 0, 1);
		Capture.ActionDefinitionId = FShanmenBasicSwordDefinition::CanonicalActionDefinitionId();
		Capture.Content.Version = TEXT("0.0.10.P3.1");
		Capture.Content.Digest = TEXT("TEST-DIGEST-P3.1");
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

	FShanmenBasicSwordOffenseSnapshot MakeSwordOffense(float AttackPower = 60.0f)
	{
		FShanmenBasicSwordOffenseSnapshot Offense;
		check(FShanmenBasicSwordOffenseSnapshot::TryCapture(AttackPower, Offense));
		return Offense;
	}

	FShanmenTargetVitalitySnapshot MakeVitality()
	{
		FShanmenTargetVitalitySnapshot Vitality;
		Vitality.CurrentVitality = 100.0f;
		Vitality.MaximumVitality = 100.0f;
		return Vitality;
	}

	FShanmenDefenseSnapshot MakeLivingDefense()
	{
		FShanmenDefenseSnapshot Defense;
		Defense.TargetTags.AddTag(FShanmenCombatNativeTags::TargetLiving());
		return Defense;
	}

	FShanmenHitCandidate MakeCandidate(
		const FShanmenWorldHitContext& Context,
		const FGuid& TargetEntityId)
	{
		FShanmenHitCandidate Candidate;
		Candidate.ActivationId = Context.GetAction().GetActivationId();
		Candidate.SourceEntityId = Context.GetAction().GetSourceEntityId();
		Candidate.TargetEntityId = TargetEntityId;
		Candidate.DetectorId = Context.GetDetectorId();
		Candidate.DetectorKind = Context.GetDetectorKind();
		Candidate.HitOrdinal = Context.GetHitOrdinal();
		Candidate.HitLocation = FVector(100.0, 20.0, 5.0);
		Candidate.HitNormal = FVector::ForwardVector;
		return Candidate;
	}

	void StartActiveSword(
		FShanmenActionOrchestrator& OutActionRuntime,
		FShanmenBasicSwordExecution& OutSword,
		FShanmenActionTransitionReceipt& OutPhaseReceipt)
	{
		const FShanmenCombatActionSnapshot Action = MakeSwordAction();
		check(FShanmenActionOrchestrator::TryStart(Action, OutActionRuntime, OutPhaseReceipt));
		check(FShanmenBasicSwordExecution::TryCreate(
			Action,
			MakeSwordDefinition(),
			MakeSwordOffense(),
			OutSword));
		check(OutActionRuntime.TryAdvance(EShanmenCombatActionPhase::Startup, OutPhaseReceipt));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenBasicSwordVerticalSliceTest,
	"Shanmen.0_0_10.CombatRuntime.BasicSword.VerticalSlice",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenBasicSwordVerticalSliceTest::RunTest(const FString&)
{
	const FShanmenCombatActionSnapshot Action = MakeSwordAction();
	FShanmenActionOrchestrator ActionRuntime;
	FShanmenActionTransitionReceipt PhaseReceipt;
	TestTrue(TEXT("Sword activation enters Startup"),
		FShanmenActionOrchestrator::TryStart(Action, ActionRuntime, PhaseReceipt));

	FShanmenBasicSwordExecution Sword;
	TestTrue(TEXT("Frozen sword definition and offense prepare execution"),
		FShanmenBasicSwordExecution::TryCreate(
			Action,
			MakeSwordDefinition(),
			MakeSwordOffense(),
			Sword));
	FShanmenWorldHitContext Context;
	TestFalse(TEXT("Startup cannot open a weapon emission"),
		Sword.TryBeginEmission(ActionRuntime, Context));
	TestTrue(TEXT("Startup crosses the P3 commit point"),
		ActionRuntime.TryAdvance(EShanmenCombatActionPhase::Startup, PhaseReceipt));
	TestTrue(TEXT("Active opens the P2.1 weapon emission"),
		Sword.TryBeginEmission(ActionRuntime, Context));

	const FShanmenHitCandidate Candidate = MakeCandidate(Context, SwordTargetA);
	FShanmenBasicSwordImpactReceipt ImpactReceipt;
	TestTrue(TEXT("Living non-self candidate resolves through CombatCore"),
		Sword.TryResolveCandidate(
			ActionRuntime,
			Candidate,
			MakeVitality(),
			MakeLivingDefense(),
			ImpactReceipt));
	TestTrue(TEXT("Impact receipt is valid and conserved"), ImpactReceipt.IsValid());
	TestTrue(TEXT("Formula freezes 20 + 60 * 0.5 = 50 raw damage"),
		FMath::IsNearlyEqual(ImpactReceipt.GetResult().RawDamage, 50.0f));
	TestTrue(TEXT("No defense applies all 50 damage"),
		FMath::IsNearlyEqual(ImpactReceipt.GetResult().FinalDamage, 50.0f));
	TestTrue(TEXT("Formula emits physical slash damage"),
		ImpactReceipt.GetRequest().Damage.DamageTags.HasTagExact(
			FShanmenCombatNativeTags::DamagePhysicalSlash()));
	TestTrue(TEXT("ImpactId is canonical"),
		ImpactReceipt.GetRequest().ImpactId == FShanmenCombatIdFactory::MakeImpactId(
			Action.GetRunId(),
			Candidate.ActivationId,
			Candidate.DetectorId,
			Candidate.TargetEntityId,
			Candidate.HitOrdinal));

	FShanmenBasicSwordImpactReceipt DuplicateOutput;
	TestFalse(TEXT("Repeated physics callback cannot resolve twice"),
		Sword.TryResolveCandidate(
			ActionRuntime,
			Candidate,
			MakeVitality(),
			MakeLivingDefense(),
			DuplicateOutput));
	TestFalse(TEXT("Duplicate failure clears output"), DuplicateOutput.IsValid());
	TestEqual(TEXT("Ledger accepted one Impact"), Sword.NumAcceptedImpacts(), 1);
	TestTrue(TEXT("Active emission closes explicitly"), Sword.TryEndEmission(ActionRuntime));
	TestTrue(TEXT("Active advances to Recovery"),
		ActionRuntime.TryAdvance(EShanmenCombatActionPhase::Active, PhaseReceipt));
	TestTrue(TEXT("Recovery completes"),
		ActionRuntime.TryAdvance(EShanmenCombatActionPhase::Recovery, PhaseReceipt));
	TestFalse(TEXT("Completed action cannot reopen emission"),
		Sword.TryBeginEmission(ActionRuntime, Context));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenBasicSwordTargetPolicyTest,
	"Shanmen.0_0_10.CombatRuntime.BasicSword.TargetPolicyAndOrdinals",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenBasicSwordTargetPolicyTest::RunTest(const FString&)
{
	FShanmenActionOrchestrator ActionRuntime;
	FShanmenBasicSwordExecution Sword;
	FShanmenActionTransitionReceipt PhaseReceipt;
	StartActiveSword(ActionRuntime, Sword, PhaseReceipt);

	FShanmenWorldHitContext FirstContext;
	TestTrue(TEXT("First active emission opens"), Sword.TryBeginEmission(ActionRuntime, FirstContext));
	FShanmenBasicSwordImpactReceipt Output;
	TestFalse(TEXT("Self target is rejected by policy, not geometry"),
		Sword.TryResolveCandidate(
			ActionRuntime,
			MakeCandidate(FirstContext, SwordSourceEntityId),
			MakeVitality(),
			MakeLivingDefense(),
			Output));

	FShanmenDefenseSnapshot MissingLivingTag;
	const FShanmenHitCandidate TargetAFirst = MakeCandidate(FirstContext, SwordTargetA);
	TestFalse(TEXT("Target without required Living tag is rejected"),
		Sword.TryResolveCandidate(
			ActionRuntime,
			TargetAFirst,
			MakeVitality(),
			MissingLivingTag,
			Output));
	TestTrue(TEXT("Policy rejection does not consume the candidate"),
		Sword.TryResolveCandidate(
			ActionRuntime,
			TargetAFirst,
			MakeVitality(),
			MakeLivingDefense(),
			Output));
	const FGuid FirstImpactId = Output.GetResult().ImpactId;
	TestTrue(TEXT("Second target shares ordinal but gets distinct ImpactId"),
		Sword.TryResolveCandidate(
			ActionRuntime,
			MakeCandidate(FirstContext, SwordTargetB),
			MakeVitality(),
			MakeLivingDefense(),
			Output));
	TestTrue(TEXT("Target identity separates same-emission Impacts"),
		FirstImpactId != Output.GetResult().ImpactId);
	TestTrue(TEXT("First emission closes"), Sword.TryEndEmission(ActionRuntime));

	FShanmenWorldHitContext SecondContext;
	TestTrue(TEXT("Second active emission opens"), Sword.TryBeginEmission(ActionRuntime, SecondContext));
	TestEqual(TEXT("Second emission advances ordinal exactly once"), SecondContext.GetHitOrdinal(), 1);
	TestTrue(TEXT("Same target may be hit in a later emission"),
		Sword.TryResolveCandidate(
			ActionRuntime,
			MakeCandidate(SecondContext, SwordTargetA),
			MakeVitality(),
			MakeLivingDefense(),
			Output));
	TestTrue(TEXT("Later emission produces a new ImpactId"), FirstImpactId != Output.GetResult().ImpactId);
	TestEqual(TEXT("Ledger accepted three unique Impacts"), Sword.NumAcceptedImpacts(), 3);
	TestTrue(TEXT("Active action may be interrupted with an emission open"),
		ActionRuntime.TryInterrupt(EShanmenCombatActionPhase::Active, PhaseReceipt));
	TestFalse(TEXT("Terminal phase cannot use the normal active-only close"),
		Sword.TryEndEmission(ActionRuntime));
	Sword.EndEmissionForTermination();
	TestFalse(TEXT("Termination cleanup closes the detector emission"), Sword.IsEmissionActive());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenBasicSwordDefenseReplayTest,
	"Shanmen.0_0_10.CombatRuntime.BasicSword.DefenseReplay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenBasicSwordDefenseReplayTest::RunTest(const FString&)
{
	FShanmenDefenseSnapshot Defense = MakeLivingDefense();
	FShanmenDefenseLayer& Guard = Defense.Layers.AddDefaulted_GetRef();
	Guard.LayerId = FGuid(0x53100007, 0, 0, 1);
	Guard.RuleId = TEXT("Defense.Guard.Test20Percent");
	Guard.Operation = EShanmenDefenseOperation::ReduceFraction;
	Guard.Order = FShanmenDefenseOrder::Guard;
	Guard.Magnitude = 0.2f;
	Guard.LayerTags.AddTag(FShanmenCombatNativeTags::DefenseGuard());
	Guard.RequiredDamageTags.AddTag(FShanmenCombatNativeTags::DamagePhysical());
	TestTrue(TEXT("Defense fixture is valid"), Defense.IsValid());

	FShanmenActionOrchestrator FirstAction;
	FShanmenBasicSwordExecution FirstSword;
	FShanmenActionTransitionReceipt PhaseReceipt;
	StartActiveSword(FirstAction, FirstSword, PhaseReceipt);
	FShanmenWorldHitContext FirstContext;
	TestTrue(TEXT("First replay emission opens"), FirstSword.TryBeginEmission(FirstAction, FirstContext));
	const FShanmenHitCandidate Candidate = MakeCandidate(FirstContext, SwordTargetA);
	FShanmenBasicSwordImpactReceipt FirstReceipt;
	TestTrue(TEXT("First guarded impact resolves"),
		FirstSword.TryResolveCandidate(
			FirstAction,
			Candidate,
			MakeVitality(),
			Defense,
			FirstReceipt));
	TestTrue(TEXT("20 percent guard prevents 10 of 50"),
		FMath::IsNearlyEqual(FirstReceipt.GetResult().PreventedDamage, 10.0f)
			&& FMath::IsNearlyEqual(FirstReceipt.GetResult().FinalDamage, 40.0f));

	FShanmenActionOrchestrator ReplayAction;
	FShanmenBasicSwordExecution ReplaySword;
	StartActiveSword(ReplayAction, ReplaySword, PhaseReceipt);
	FShanmenWorldHitContext ReplayContext;
	TestTrue(TEXT("Replay emission opens"), ReplaySword.TryBeginEmission(ReplayAction, ReplayContext));
	FShanmenBasicSwordImpactReceipt ReplayReceipt;
	TestTrue(TEXT("Replay guarded impact resolves"),
		ReplaySword.TryResolveCandidate(
			ReplayAction,
			MakeCandidate(ReplayContext, SwordTargetA),
			MakeVitality(),
			Defense,
			ReplayReceipt));
	TestTrue(TEXT("Replay preserves canonical ImpactId"),
		FirstReceipt.GetResult().ImpactId == ReplayReceipt.GetResult().ImpactId);
	TestTrue(TEXT("Replay preserves exact damage receipt"),
		FMath::IsNearlyEqual(
			FirstReceipt.GetResult().PreventedDamage,
			ReplayReceipt.GetResult().PreventedDamage)
			&& FMath::IsNearlyEqual(
				FirstReceipt.GetResult().FinalDamage,
				ReplayReceipt.GetResult().FinalDamage));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenBasicSwordAbilityBoundaryTest,
	"Shanmen.0_0_10.CombatRuntime.BasicSword.AbilityBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenBasicSwordAbilityBoundaryTest::RunTest(const FString&)
{
	FShanmenBasicSwordDefinitionCapture InvalidDefinition;
	InvalidDefinition.ActionDefinitionId = TEXT("Combat.Action.Sword.Wrong");
	InvalidDefinition.DetectorId = TEXT("Detector.Weapon.Main");
	InvalidDefinition.FormulaId = TEXT("Combat.Formula.Sword.Basic01.r1");
	InvalidDefinition.BaseDamage = 20.0f;
	InvalidDefinition.AttackPowerCoefficient = 0.5f;
	InvalidDefinition.DamageTags.AddTag(FShanmenCombatNativeTags::DamagePhysicalSlash());
	InvalidDefinition.RequiredTargetTags.AddTag(FShanmenCombatNativeTags::TargetLiving());
	FShanmenBasicSwordDefinition Rejected;
	TestFalse(TEXT("Definition with wrong action identity fails closed"),
		FShanmenBasicSwordDefinition::TryCapture(InvalidDefinition, Rejected));
	TestFalse(TEXT("Default offense is not a captured snapshot"),
		FShanmenBasicSwordOffenseSnapshot().IsValid());

	const UClass* AbilityClass = UShanmenBasicSwordGameplayAbility::StaticClass();
	const UShanmenBasicSwordGameplayAbility* AbilityCDO = GetDefault<UShanmenBasicSwordGameplayAbility>();
	TestTrue(TEXT("Product adapter is still required before ability can be granted"),
		AbilityClass->HasAnyClassFlags(CLASS_Abstract));
	TestTrue(TEXT("Sword ability retains generic combat action tag"),
		AbilityCDO->GetAssetTags().HasTagExact(
			FShanmenCombatRuntimeNativeTags::AbilityCombatAction()));
	TestTrue(TEXT("Sword ability adds exact Basic01 GAS tag"),
		AbilityCDO->GetAssetTags().HasTagExact(
			FShanmenCombatRuntimeNativeTags::AbilityCombatActionSwordBasic01()));
	return true;
}

#endif
