#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ShanmenBasicSwordExecution.h"
#include "ShanmenCombatResolver.h"
#include "ShanmenCombatTags.h"
#include "ShanmenSwordRhythmContribution.h"

namespace
{
	constexpr EAutomationTestFlags ContributionFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;
	const FGuid ContributionRun(
		0xC1280001, 0xC1280002, 0xC1280003, 0xC1280004);
	const FGuid ContributionOwner(
		0xC1290001, 0xC1290002, 0xC1290003, 0xC1290004);
	const FGuid ContributionSource(
		0xC12A0001, 0xC12A0002, 0xC12A0003, 0xC12A0004);
	const FGuid ContributionItem(
		0xC12B0001, 0xC12B0002, 0xC12B0003, 0xC12B0004);
	const FGuid ContributionTimeline(
		0xC12C0001, 0xC12C0002, 0xC12C0003, 0xC12C0004);

	FShanmenCombatActionSnapshot MakeAction(
		FName ActionDefinitionId,
		uint64 Sequence,
		bool bUsesItem)
	{
		FShanmenCombatActionCapture Capture;
		Capture.RunId = ContributionRun;
		Capture.OwnerId = ContributionOwner;
		Capture.SourceEntityId = ContributionSource;
		Capture.SourceItemInstanceId = bUsesItem
			? ContributionItem
			: FGuid();
		Capture.ActionDefinitionId = ActionDefinitionId;
		Capture.Content.Version = TEXT("0.0.10.P12.5");
		Capture.Content.Digest = TEXT("TEST-DIGEST-P12.5");
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

	FShanmenSwordRhythmReceipt MakePreciseLinkReceipt()
	{
		FShanmenSwordRhythmDefinitionCapture Capture;
		Capture.StyleDefinitionId =
			FShanmenSwordRhythmDefinition::CanonicalStyleDefinitionId();
		Capture.RuleId = TEXT("Combat.Style.Sword.Taiji01.LinkWindow.r1");
		Capture.LinkOpenOffsetTicks = 8;
		Capture.LinkCloseOffsetTicks = 13;
		FShanmenSwordRhythmDefinition Definition;
		check(FShanmenSwordRhythmDefinition::TryCapture(Capture, Definition));
		FShanmenSwordRhythmChain Chain;
		check(FShanmenSwordRhythmChain::TryCreate(Definition, Chain));
		FShanmenSwordRhythmObservation First;
		FShanmenSwordRhythmObservation Second;
		check(FShanmenSwordRhythmObservation::TryCapture(
			MakeAction(
				FShanmenBasicSwordDefinition::CanonicalActionDefinitionId(),
				1250,
				true),
			ContributionTimeline,
			100,
			First));
		check(FShanmenSwordRhythmObservation::TryCapture(
			MakeAction(
				FShanmenBasicSwordDefinition::CanonicalActionDefinitionId(),
				1251,
				true),
			ContributionTimeline,
			108,
			Second));
		FShanmenSwordRhythmReceipt Receipt;
		check(Chain.TryObserve(First, Receipt));
		check(Chain.TryObserve(Second, Receipt));
		check(Receipt.GetBand() == EShanmenSwordRhythmBand::PreciseLinked);
		return Receipt;
	}

	void MakeGuardProjection(
		int64 ObservedTick,
		FShanmenWeaponGuardTimingProjectionReceipt& OutReceipt)
	{
		const FShanmenCombatActionSnapshot Action = MakeAction(
			FShanmenWeaponGuardDefinition::CanonicalActionDefinitionId(),
			1260,
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
			ContributionTimeline,
			200,
			205,
			TEXT("Defense.Sword.PerfectGuard01"),
			Policy));
		FShanmenWeaponGuardTimelineObservation Observation;
		check(FShanmenWeaponGuardTimelineObservation::TryCapture(
			ContributionTimeline, ObservedTick, Observation));
		check(FShanmenWeaponGuardTimingEvaluator::TryProject(
			Window, Runtime, Policy, Observation, OutReceipt));
	}

	FShanmenSpiritEvasionProjectionReceipt MakeEvasionProjection()
	{
		const FShanmenCombatActionSnapshot Action = MakeAction(
			FShanmenSpiritEvasionDefinition::CanonicalActionDefinitionId(),
			1270,
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
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenSwordRhythmContributionSourceTest,
	"Shanmen.0_0_10.CombatRuntime.SwordRhythmContribution.EligibleSources",
	ContributionFlags)

bool FShanmenSwordRhythmContributionSourceTest::RunTest(const FString&)
{
	const FShanmenSwordRhythmReceipt LinkReceipt = MakePreciseLinkReceipt();
	FShanmenWeaponGuardTimingProjectionReceipt GuardReceipt;
	MakeGuardProjection(202, GuardReceipt);
	const FShanmenSpiritEvasionProjectionReceipt EvasionReceipt =
		MakeEvasionProjection();
	FShanmenSwordRhythmContribution Link;
	FShanmenSwordRhythmContribution Guard;
	FShanmenSwordRhythmContribution Evasion;
	TestTrue(TEXT("precise sword link becomes immutable contribution evidence"),
		FShanmenSwordRhythmContribution::TryCapturePreciseSwordLink(
			LinkReceipt, Link)
			&& Link.IsValid()
			&& Link.GetKind()
				== EShanmenSwordRhythmContributionKind::PreciseSwordLink
			&& Link.GetObservedTick() == 108);
	TestTrue(TEXT("perfect guard becomes immutable contribution evidence"),
		FShanmenSwordRhythmContribution::TryCapturePerfectWeaponGuard(
			GuardReceipt, Guard)
			&& Guard.IsValid()
			&& Guard.GetKind()
				== EShanmenSwordRhythmContributionKind::PerfectWeaponGuard
			&& Guard.GetObservedTick() == 202);
	TestTrue(TEXT("successful evasion projection binds caller timeline evidence"),
		FShanmenSwordRhythmContribution::TryCaptureSpiritEvasion(
			EvasionReceipt,
			ContributionTimeline,
			300,
			Evasion)
			&& Evasion.IsValid()
			&& Evasion.GetKind()
				== EShanmenSwordRhythmContributionKind::SpiritEvasion
			&& Evasion.GetObservedTick() == 300);
	TestTrue(TEXT("different source facts retain different identities"),
		Link.GetContributionId() != Guard.GetContributionId()
			&& Guard.GetContributionId() != Evasion.GetContributionId()
			&& Link.GetContributionId() != Evasion.GetContributionId());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenSwordRhythmContributionEligibilityTest,
	"Shanmen.0_0_10.CombatRuntime.SwordRhythmContribution.FailClosedEligibility",
	ContributionFlags)

bool FShanmenSwordRhythmContributionEligibilityTest::RunTest(const FString&)
{
	FShanmenSwordRhythmDefinitionCapture DefinitionCapture;
	DefinitionCapture.StyleDefinitionId =
		FShanmenSwordRhythmDefinition::CanonicalStyleDefinitionId();
	DefinitionCapture.RuleId = TEXT("Combat.Style.Sword.Taiji01.LinkWindow.r1");
	DefinitionCapture.LinkOpenOffsetTicks = 8;
	DefinitionCapture.LinkCloseOffsetTicks = 13;
	FShanmenSwordRhythmDefinition Definition;
	check(FShanmenSwordRhythmDefinition::TryCapture(
		DefinitionCapture, Definition));
	FShanmenSwordRhythmChain Chain;
	check(FShanmenSwordRhythmChain::TryCreate(Definition, Chain));
	FShanmenSwordRhythmObservation First;
	check(FShanmenSwordRhythmObservation::TryCapture(
		MakeAction(
			FShanmenBasicSwordDefinition::CanonicalActionDefinitionId(),
			1280,
			true),
		ContributionTimeline,
		400,
		First));
	FShanmenSwordRhythmReceipt Started;
	check(Chain.TryObserve(First, Started));

	FShanmenWeaponGuardTimingProjectionReceipt Ordinary;
	MakeGuardProjection(205, Ordinary);
	FShanmenSwordRhythmContribution Output;
	TestFalse(TEXT("chain start is not mislabeled as a precise contribution"),
		FShanmenSwordRhythmContribution::TryCapturePreciseSwordLink(
			Started, Output));
	TestFalse(TEXT("rejected precise-link capture clears reusable output"),
		Output.IsValid());
	TestFalse(TEXT("ordinary guard is not mislabeled as perfect"),
		FShanmenSwordRhythmContribution::TryCapturePerfectWeaponGuard(
			Ordinary, Output));
	TestFalse(TEXT("ordinary guard rejection keeps output empty"),
		Output.IsValid());
	TestFalse(TEXT("evasion requires an explicit valid timeline identity"),
		FShanmenSwordRhythmContribution::TryCaptureSpiritEvasion(
			MakeEvasionProjection(), FGuid(), 500, Output));
	TestFalse(TEXT("evasion rejects negative caller ticks"),
		FShanmenSwordRhythmContribution::TryCaptureSpiritEvasion(
			MakeEvasionProjection(), ContributionTimeline, -1, Output));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenSwordRhythmContributionReplayTest,
	"Shanmen.0_0_10.CombatRuntime.SwordRhythmContribution.DeterministicReplay",
	ContributionFlags)

bool FShanmenSwordRhythmContributionReplayTest::RunTest(const FString&)
{
	const FShanmenSpiritEvasionProjectionReceipt Projection =
		MakeEvasionProjection();
	FShanmenSwordRhythmContribution First;
	FShanmenSwordRhythmContribution Replay;
	FShanmenSwordRhythmContribution Later;
	TestTrue(TEXT("equal frozen evidence captures repeatedly"),
		FShanmenSwordRhythmContribution::TryCaptureSpiritEvasion(
			Projection, ContributionTimeline, 600, First)
			&& FShanmenSwordRhythmContribution::TryCaptureSpiritEvasion(
				Projection, ContributionTimeline, 600, Replay));
	TestTrue(TEXT("equal frozen evidence replays the same contribution ID"),
		First.GetContributionId() == Replay.GetContributionId()
			&& First.GetSourceReceiptId() == Projection.GetProjectionId()
			&& First.GetAction().GetRunId() == ContributionRun);
	TestTrue(TEXT("a different observed tick creates a distinct fact"),
		FShanmenSwordRhythmContribution::TryCaptureSpiritEvasion(
			Projection, ContributionTimeline, 601, Later)
			&& Later.GetContributionId() != First.GetContributionId());
	return true;
}

#endif
