#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ShanmenBasicSwordExecution.h"
#include "ShanmenCombatResolver.h"
#include "ShanmenCombatTags.h"
#include "ShanmenSpiritEvasion.h"
#include "ShanmenSwordRhythmEvaluation.h"
#include "ShanmenWeaponGuard.h"
#include "ShanmenWeaponPerfectGuard.h"

namespace
{
	constexpr EAutomationTestFlags EvaluationFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;
	const FGuid EvaluationRun(
		0xC1340001, 0xC1340002, 0xC1340003, 0xC1340004);
	const FGuid EvaluationOwner(
		0xC1350001, 0xC1350002, 0xC1350003, 0xC1350004);
	const FGuid EvaluationSource(
		0xC1360001, 0xC1360002, 0xC1360003, 0xC1360004);
	const FGuid EvaluationItem(
		0xC1370001, 0xC1370002, 0xC1370003, 0xC1370004);
	const FGuid EvaluationTimeline(
		0xC1380001, 0xC1380002, 0xC1380003, 0xC1380004);

	FName PreciseEffectId()
	{
		return TEXT("Combat.Style.Sword.Taiji01.Effect.PreciseFlow");
	}

	FName GuardEffectId()
	{
		return TEXT("Combat.Style.Sword.Taiji01.Effect.BorrowedForce");
	}

	FName EvasionEffectId()
	{
		return TEXT("Combat.Style.Sword.Taiji01.Effect.RedirectedMomentum");
	}

	FShanmenContentStamp MakePolicyContent()
	{
		FShanmenContentStamp Content;
		Content.Version = TEXT("0.0.10.P12.9");
		Content.Digest = TEXT("TEST-DIGEST-P12.9-SYMBOLIC-EFFECTS");
		return Content;
	}

	FShanmenCombatActionSnapshot MakeAction(
		FName ActionDefinitionId,
		uint64 Sequence,
		bool bUsesItem)
	{
		FShanmenCombatActionCapture Capture;
		Capture.RunId = EvaluationRun;
		Capture.OwnerId = EvaluationOwner;
		Capture.SourceEntityId = EvaluationSource;
		Capture.SourceItemInstanceId = bUsesItem
			? EvaluationItem
			: FGuid();
		Capture.ActionDefinitionId = ActionDefinitionId;
		Capture.Content.Version = TEXT("0.0.10.P12.9");
		Capture.Content.Digest = TEXT("TEST-DIGEST-P12.9-ACTION");
		Capture.SourceTags.AddTag(FShanmenCombatNativeTags::SourcePlayer());
		Capture.ActivationId = FShanmenCombatIdFactory::MakeActivationId(
			Capture.RunId,
			Capture.SourceEntityId,
			Capture.ActionDefinitionId,
			Sequence);
		FShanmenCombatActionSnapshot Action;
		check(FShanmenCombatActionSnapshot::TryCapture(Capture, Action));
		return Action;
	}

	FShanmenSwordRhythmDefinition MakeRhythmDefinition()
	{
		FShanmenSwordRhythmDefinitionCapture Capture;
		Capture.StyleDefinitionId =
			FShanmenSwordRhythmDefinition::CanonicalStyleDefinitionId();
		Capture.RuleId =
			TEXT("Combat.Style.Sword.Taiji01.LinkWindow.r1");
		Capture.LinkOpenOffsetTicks = 8;
		Capture.LinkCloseOffsetTicks = 13;
		FShanmenSwordRhythmDefinition Definition;
		check(FShanmenSwordRhythmDefinition::TryCapture(
			Capture, Definition));
		return Definition;
	}

	FShanmenSwordRhythmObservation MakeSwordObservation(
		uint64 Sequence,
		int64 Tick)
	{
		FShanmenSwordRhythmObservation Observation;
		check(FShanmenSwordRhythmObservation::TryCapture(
			MakeAction(
				FShanmenBasicSwordDefinition::CanonicalActionDefinitionId(),
				Sequence,
				true),
			EvaluationTimeline,
			Tick,
			Observation));
		return Observation;
	}

	FShanmenSwordRhythmReceipt MakeTargetRhythmReceipt()
	{
		FShanmenSwordRhythmChain Chain;
		check(FShanmenSwordRhythmChain::TryCreate(
			MakeRhythmDefinition(), Chain));
		FShanmenSwordRhythmReceipt Receipt;
		check(Chain.TryObserve(MakeSwordObservation(1400, 400), Receipt));
		check(Receipt.GetBand() == EShanmenSwordRhythmBand::Started);
		return Receipt;
	}

	FShanmenSwordRhythmContribution MakePreciseContribution()
	{
		FShanmenSwordRhythmChain Chain;
		check(FShanmenSwordRhythmChain::TryCreate(
			MakeRhythmDefinition(), Chain));
		FShanmenSwordRhythmReceipt Receipt;
		check(Chain.TryObserve(MakeSwordObservation(1401, 100), Receipt));
		check(Chain.TryObserve(MakeSwordObservation(1402, 108), Receipt));
		check(Receipt.GetBand() == EShanmenSwordRhythmBand::PreciseLinked);
		FShanmenSwordRhythmContribution Contribution;
		check(FShanmenSwordRhythmContribution::TryCapturePreciseSwordLink(
			Receipt, Contribution));
		return Contribution;
	}

	FShanmenWeaponGuardTimingProjectionReceipt MakeGuardProjection()
	{
		const FShanmenCombatActionSnapshot Action = MakeAction(
			FShanmenWeaponGuardDefinition::CanonicalActionDefinitionId(),
			1410,
			true);
		FShanmenWeaponGuardDefinitionCapture DefinitionCapture;
		DefinitionCapture.ActionDefinitionId =
			FShanmenWeaponGuardDefinition::CanonicalActionDefinitionId();
		DefinitionCapture.RuleId = TEXT("Defense.Sword.WeaponGuard01");
		DefinitionCapture.GuardFraction = 0.25f;
		FShanmenWeaponGuardDefinition Definition;
		check(FShanmenWeaponGuardDefinition::TryCapture(
			DefinitionCapture, Definition));
		FShanmenActionOrchestrator Runtime;
		FShanmenActionTransitionReceipt Commit;
		check(FShanmenActionOrchestrator::TryStart(Action, Runtime, Commit));
		check(Runtime.TryAdvance(
			EShanmenCombatActionPhase::Startup, Commit));
		FShanmenWeaponGuardWindow Window;
		FShanmenWeaponGuardWindowReceipt Open;
		check(FShanmenWeaponGuardWindow::TryOpen(
			Action, Definition, Commit, Runtime, Window, Open));
		FShanmenWeaponPerfectGuardPolicy Policy;
		check(FShanmenWeaponPerfectGuardPolicy::TryCapture(
			Open,
			EvaluationTimeline,
			200,
			205,
			TEXT("Defense.Sword.PerfectGuard01"),
			Policy));
		FShanmenWeaponGuardTimelineObservation Observation;
		check(FShanmenWeaponGuardTimelineObservation::TryCapture(
			EvaluationTimeline, 202, Observation));
		FShanmenWeaponGuardTimingProjectionReceipt Projection;
		check(FShanmenWeaponGuardTimingEvaluator::TryProject(
			Window, Runtime, Policy, Observation, Projection));
		return Projection;
	}

	FShanmenSwordRhythmContribution MakeGuardContribution()
	{
		FShanmenSwordRhythmContribution Contribution;
		check(FShanmenSwordRhythmContribution::TryCapturePerfectWeaponGuard(
			MakeGuardProjection(), Contribution));
		return Contribution;
	}

	FShanmenSpiritEvasionProjectionReceipt MakeEvasionProjection()
	{
		const FShanmenCombatActionSnapshot Action = MakeAction(
			FShanmenSpiritEvasionDefinition::CanonicalActionDefinitionId(),
			1420,
			false);
		FShanmenSpiritEvasionDefinitionCapture DefinitionCapture;
		DefinitionCapture.ActionDefinitionId =
			FShanmenSpiritEvasionDefinition::CanonicalActionDefinitionId();
		DefinitionCapture.RuleId = TEXT("Defense.Spell.SpiritEvasion01");
		FShanmenSpiritEvasionDefinition Definition;
		check(FShanmenSpiritEvasionDefinition::TryCapture(
			DefinitionCapture, Definition));
		FShanmenActionOrchestrator Runtime;
		FShanmenActionTransitionReceipt Commit;
		check(FShanmenActionOrchestrator::TryStart(Action, Runtime, Commit));
		check(Runtime.TryAdvance(
			EShanmenCombatActionPhase::Startup, Commit));
		FShanmenSpiritEvasionWindow Window;
		FShanmenSpiritEvasionWindowReceipt Open;
		check(FShanmenSpiritEvasionWindow::TryOpen(
			Action, Definition, Commit, Runtime, Window, Open));
		FShanmenSpiritEvasionProjectionReceipt Projection;
		check(Window.TryProjectDefenseLayer(Runtime, Projection));
		return Projection;
	}

	FShanmenSwordRhythmContribution MakeEvasionContribution()
	{
		FShanmenSwordRhythmContribution Contribution;
		check(FShanmenSwordRhythmContribution::TryCaptureSpiritEvasion(
			MakeEvasionProjection(),
			EvaluationTimeline,
			300,
			Contribution));
		return Contribution;
	}

	FShanmenSwordRhythmEvaluationInput MakeEvaluationInput(bool bWithBinding)
	{
		const FShanmenSwordRhythmReceipt Rhythm = MakeTargetRhythmReceipt();
		FShanmenSwordRhythmContributionBindingReceipt Binding;
		if (bWithBinding)
		{
			FShanmenSwordRhythmContributionBindingScope Scope;
			check(FShanmenSwordRhythmContributionBindingScope::TryCapture(
				EvaluationRun,
				EvaluationOwner,
				EvaluationTimeline,
				Scope));
			FShanmenSwordRhythmContributionBindingLedger Ledger;
			check(FShanmenSwordRhythmContributionBindingLedger::TryCreate(
				Scope, Ledger));
			check(Ledger.TryRecordContribution(MakeEvasionContribution()));
			check(Ledger.TryRecordContribution(MakePreciseContribution()));
			check(Ledger.TryRecordContribution(MakeGuardContribution()));
			check(Ledger.TryObserveBasicSword(
				Rhythm.GetCurrentObservation(), Binding));
			check(Binding.IsValid() && Binding.NumContributions() == 3);
		}
		FShanmenSwordRhythmEvaluationInput Input;
		check(FShanmenSwordRhythmEvaluationInput::TryCapture(
			Rhythm, Binding, Input));
		return Input;
	}

	FShanmenSwordRhythmEvaluationPolicyCapture MakePolicyCapture(
		bool bReverseOrder = false)
	{
		FShanmenSwordRhythmEvaluationPolicyCapture Capture;
		Capture.PolicyDefinitionId =
			TEXT("Combat.Style.Sword.Taiji01.SymbolicEffects.r1");
		Capture.Content = MakePolicyContent();
		FShanmenSwordRhythmEffectSpecificationCapture Precise;
		Precise.ContributionKind =
			EShanmenSwordRhythmContributionKind::PreciseSwordLink;
		Precise.EffectDefinitionId = PreciseEffectId();
		FShanmenSwordRhythmEffectSpecificationCapture Guard;
		Guard.ContributionKind =
			EShanmenSwordRhythmContributionKind::PerfectWeaponGuard;
		Guard.EffectDefinitionId = GuardEffectId();
		FShanmenSwordRhythmEffectSpecificationCapture Evasion;
		Evasion.ContributionKind =
			EShanmenSwordRhythmContributionKind::SpiritEvasion;
		Evasion.EffectDefinitionId = EvasionEffectId();
		Capture.Specifications = bReverseOrder
			? TArray<FShanmenSwordRhythmEffectSpecificationCapture>{
				Precise, Guard, Evasion }
			: TArray<FShanmenSwordRhythmEffectSpecificationCapture>{
				Evasion, Guard, Precise };
		return Capture;
	}

	FShanmenSwordRhythmEvaluationPolicy MakePolicy(bool bReverseOrder = false)
	{
		FShanmenSwordRhythmEvaluationPolicy Policy;
		check(FShanmenSwordRhythmEvaluationPolicy::TryCapture(
			MakePolicyCapture(bReverseOrder), Policy));
		return Policy;
	}

	FName ExpectedEffectDefinition(
		EShanmenSwordRhythmContributionKind Kind)
	{
		switch (Kind)
		{
		case EShanmenSwordRhythmContributionKind::PreciseSwordLink:
			return PreciseEffectId();
		case EShanmenSwordRhythmContributionKind::PerfectWeaponGuard:
			return GuardEffectId();
		case EShanmenSwordRhythmContributionKind::SpiritEvasion:
			return EvasionEffectId();
		default:
			return NAME_None;
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenSwordRhythmEvaluationPolicyTest,
	"Shanmen.0_0_10.CombatRuntime.SwordRhythmEvaluation.PolicyContract",
	EvaluationFlags)

bool FShanmenSwordRhythmEvaluationPolicyTest::RunTest(const FString&)
{
	const FShanmenSwordRhythmEvaluationPolicy Policy = MakePolicy();
	const FShanmenSwordRhythmEvaluationPolicy Reordered = MakePolicy(true);
	TestTrue(TEXT("complete symbolic policy is valid and input-order independent"),
		Policy.IsValid()
			&& Reordered.IsValid()
			&& Policy.GetPolicyId() == Reordered.GetPolicyId()
			&& Policy.GetSpecifications().Num() == 3);
	for (const EShanmenSwordRhythmContributionKind Kind : {
		EShanmenSwordRhythmContributionKind::PreciseSwordLink,
		EShanmenSwordRhythmContributionKind::PerfectWeaponGuard,
		EShanmenSwordRhythmContributionKind::SpiritEvasion })
	{
		FShanmenSwordRhythmEffectSpecification Specification;
		TestTrue(TEXT("every current contribution kind has one authored effect"),
			Policy.TryFindSpecification(Kind, Specification)
				&& Specification.IsValid()
				&& Specification.GetContributionKind() == Kind
				&& Specification.GetEffectDefinitionId()
					== ExpectedEffectDefinition(Kind));
	}

	FShanmenSwordRhythmEvaluationPolicyCapture Missing = MakePolicyCapture();
	Missing.Specifications.RemoveAt(0);
	FShanmenSwordRhythmEvaluationPolicy Output;
	TestFalse(TEXT("policy fails closed when one source kind is unmapped"),
		FShanmenSwordRhythmEvaluationPolicy::TryCapture(Missing, Output));
	TestFalse(TEXT("failed policy capture clears reusable output"),
		Output.IsValid());
	FShanmenSwordRhythmEvaluationPolicyCapture DuplicateKind =
		MakePolicyCapture();
	DuplicateKind.Specifications[0].ContributionKind =
		DuplicateKind.Specifications[1].ContributionKind;
	TestFalse(TEXT("policy rejects duplicate contribution kind"),
		FShanmenSwordRhythmEvaluationPolicy::TryCapture(
			DuplicateKind, Output));
	FShanmenSwordRhythmEvaluationPolicyCapture DuplicateEffect =
		MakePolicyCapture();
	DuplicateEffect.Specifications[0].EffectDefinitionId =
		DuplicateEffect.Specifications[1].EffectDefinitionId;
	TestFalse(TEXT("policy rejects ambiguous duplicate authored effect ID"),
		FShanmenSwordRhythmEvaluationPolicy::TryCapture(
			DuplicateEffect, Output));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenSwordRhythmEvaluationMappingTest,
	"Shanmen.0_0_10.CombatRuntime.SwordRhythmEvaluation.SymbolicMapping",
	EvaluationFlags)

bool FShanmenSwordRhythmEvaluationMappingTest::RunTest(const FString&)
{
	const FShanmenSwordRhythmEvaluationInput Input = MakeEvaluationInput(true);
	const FShanmenSwordRhythmEvaluationPolicy Policy = MakePolicy();
	const FShanmenSwordRhythmEvaluationResult Result =
		FShanmenSwordRhythmEvaluator::Evaluate(Input, Policy);
	TestTrue(TEXT("three proven facts map to three immutable symbolic effects"),
		Result.IsSuccess()
			&& Result.GetStatus()
				== EShanmenSwordRhythmEvaluationStatus::Evaluated
			&& Result.GetReceipt().IsValid()
			&& Result.GetReceipt().NumEffects() == 3
			&& Result.GetReceipt().NumEffects() == Input.NumContributions()
			&& Result.GetReceipt().GetInput().GetInputId()
				== Input.GetInputId()
			&& Result.GetReceipt().GetPolicy().GetPolicyId()
				== Policy.GetPolicyId());
	const TArray<FShanmenSwordRhythmEvaluatedEffect>& Effects =
		Result.GetReceipt().GetEffects();
	for (const FShanmenSwordRhythmEvaluatedEffect& Effect : Effects)
	{
		TestTrue(TEXT("effect retains source, mapping, input and authored identity"),
			Effect.IsValid()
				&& Effect.GetEvaluationInputId() == Input.GetInputId()
				&& Effect.GetSpecification().GetContributionKind()
					== Effect.GetContribution().GetKind()
				&& Effect.GetSpecification().GetEffectDefinitionId()
					== ExpectedEffectDefinition(
						Effect.GetContribution().GetKind()));
	}
	TestTrue(TEXT("binding causal order remains visible in effect order"),
		Effects[0].GetContribution().GetKind()
			== EShanmenSwordRhythmContributionKind::PreciseSwordLink
			&& Effects[1].GetContribution().GetKind()
				== EShanmenSwordRhythmContributionKind::PerfectWeaponGuard
			&& Effects[2].GetContribution().GetKind()
				== EShanmenSwordRhythmContributionKind::SpiritEvasion);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenSwordRhythmEvaluationReplayTest,
	"Shanmen.0_0_10.CombatRuntime.SwordRhythmEvaluation.EmptyAndReplay",
	EvaluationFlags)

bool FShanmenSwordRhythmEvaluationReplayTest::RunTest(const FString&)
{
	const FShanmenSwordRhythmEvaluationPolicy Policy = MakePolicy();
	const FShanmenSwordRhythmEvaluationResult Empty =
		FShanmenSwordRhythmEvaluator::Evaluate(
			MakeEvaluationInput(false), Policy);
	TestTrue(TEXT("legal rhythm without bound evidence yields valid empty receipt"),
		Empty.IsSuccess()
			&& Empty.GetReceipt().IsValid()
			&& Empty.GetReceipt().NumEffects() == 0);

	const FShanmenSwordRhythmEvaluationInput Input = MakeEvaluationInput(true);
	const FShanmenSwordRhythmEvaluationResult First =
		FShanmenSwordRhythmEvaluator::Evaluate(Input, Policy);
	const FShanmenSwordRhythmEvaluationResult Replay =
		FShanmenSwordRhythmEvaluator::Evaluate(Input, MakePolicy(true));
	TestTrue(TEXT("equal evidence and equal authored policy replay exactly"),
		First.IsSuccess()
			&& Replay.IsSuccess()
			&& First.GetReceipt().GetReceiptId()
				== Replay.GetReceipt().GetReceiptId()
			&& First.GetReceipt().GetEffects().Num()
				== Replay.GetReceipt().GetEffects().Num());
	for (int32 Index = 0;
		Index < First.GetReceipt().GetEffects().Num(); ++Index)
	{
		TestEqual(TEXT("replayed effect identity is stable"),
			First.GetReceipt().GetEffects()[Index].GetEffectId(),
			Replay.GetReceipt().GetEffects()[Index].GetEffectId());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenSwordRhythmEvaluationFailClosedTest,
	"Shanmen.0_0_10.CombatRuntime.SwordRhythmEvaluation.FailClosed",
	EvaluationFlags)

bool FShanmenSwordRhythmEvaluationFailClosedTest::RunTest(const FString&)
{
	const FShanmenSwordRhythmEvaluationPolicy Policy = MakePolicy();
	const FShanmenSwordRhythmEvaluationResult InvalidInput =
		FShanmenSwordRhythmEvaluator::Evaluate(
			FShanmenSwordRhythmEvaluationInput(), Policy);
	TestTrue(TEXT("invalid input fails before producing a receipt"),
		!InvalidInput.IsSuccess()
			&& InvalidInput.GetStatus()
				== EShanmenSwordRhythmEvaluationStatus::InputInvalid
			&& !InvalidInput.GetReceipt().IsValid());
	const FShanmenSwordRhythmEvaluationInput Input = MakeEvaluationInput(true);
	const FShanmenSwordRhythmEvaluationResult InvalidPolicy =
		FShanmenSwordRhythmEvaluator::Evaluate(
			Input, FShanmenSwordRhythmEvaluationPolicy());
	TestTrue(TEXT("invalid policy fails before producing a receipt"),
		!InvalidPolicy.IsSuccess()
			&& InvalidPolicy.GetStatus()
				== EShanmenSwordRhythmEvaluationStatus::PolicyInvalid
			&& !InvalidPolicy.GetReceipt().IsValid());

	FShanmenSwordRhythmEffectSpecification GuardSpecification;
	check(Policy.TryFindSpecification(
		EShanmenSwordRhythmContributionKind::PerfectWeaponGuard,
		GuardSpecification));
	FShanmenSwordRhythmEvaluatedEffect Output;
	TestFalse(TEXT("one source kind cannot be mislabeled with another effect"),
		FShanmenSwordRhythmEvaluatedEffect::TryCreate(
			Input,
			GuardSpecification,
			Input.GetContributionBinding().GetContributions()[0],
			Output));
	TestFalse(TEXT("rejected effect mapping clears reusable output"),
		Output.IsValid());
	return true;
}

#endif
