#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ShanmenBasicSwordExecution.h"
#include "ShanmenCombatResolver.h"
#include "ShanmenCombatTags.h"
#include "ShanmenSwordRhythmContributionBinding.h"

namespace
{
	constexpr EAutomationTestFlags BindingFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;
	const FGuid BindingRun(
		0xC12D0001, 0xC12D0002, 0xC12D0003, 0xC12D0004);
	const FGuid BindingOwner(
		0xC12E0001, 0xC12E0002, 0xC12E0003, 0xC12E0004);
	const FGuid BindingSource(
		0xC12F0001, 0xC12F0002, 0xC12F0003, 0xC12F0004);
	const FGuid BindingItem(
		0xC1300001, 0xC1300002, 0xC1300003, 0xC1300004);
	const FGuid BindingTimeline(
		0xC1310001, 0xC1310002, 0xC1310003, 0xC1310004);
	const FGuid AlternateRun(
		0xC1320001, 0xC1320002, 0xC1320003, 0xC1320004);
	const FGuid AlternateTimeline(
		0xC1330001, 0xC1330002, 0xC1330003, 0xC1330004);

	FShanmenCombatActionSnapshot MakeBindingAction(
		uint64 Sequence,
		const FGuid& RunId = BindingRun)
	{
		FShanmenCombatActionCapture Capture;
		Capture.RunId = RunId;
		Capture.OwnerId = BindingOwner;
		Capture.SourceEntityId = BindingSource;
		Capture.SourceItemInstanceId = BindingItem;
		Capture.ActionDefinitionId =
			FShanmenBasicSwordDefinition::CanonicalActionDefinitionId();
		Capture.Content.Version = TEXT("0.0.10.P12.6");
		Capture.Content.Digest = TEXT("TEST-DIGEST-P12.6");
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

	FShanmenSwordRhythmObservation MakeBindingObservation(
		uint64 Sequence,
		int64 InputTick,
		const FGuid& RunId = BindingRun,
		const FGuid& TimelineId = BindingTimeline)
	{
		FShanmenSwordRhythmObservation Observation;
		check(FShanmenSwordRhythmObservation::TryCapture(
			MakeBindingAction(Sequence, RunId),
			TimelineId,
			InputTick,
			Observation));
		return Observation;
	}

	FShanmenSwordRhythmContribution MakePreciseContribution(
		uint64 FirstSequence,
		uint64 SecondSequence,
		int64 FirstTick,
		int64 SecondTick,
		const FGuid& RunId = BindingRun,
		const FGuid& TimelineId = BindingTimeline)
	{
		FShanmenSwordRhythmDefinitionCapture DefinitionCapture;
		DefinitionCapture.StyleDefinitionId =
			FShanmenSwordRhythmDefinition::CanonicalStyleDefinitionId();
		DefinitionCapture.RuleId =
			TEXT("Combat.Style.Sword.Taiji01.LinkWindow.r1");
		DefinitionCapture.LinkOpenOffsetTicks = 8;
		DefinitionCapture.LinkCloseOffsetTicks = 13;
		FShanmenSwordRhythmDefinition Definition;
		check(FShanmenSwordRhythmDefinition::TryCapture(
			DefinitionCapture, Definition));
		FShanmenSwordRhythmChain Chain;
		check(FShanmenSwordRhythmChain::TryCreate(Definition, Chain));
		FShanmenSwordRhythmReceipt Receipt;
		check(Chain.TryObserve(
			MakeBindingObservation(
				FirstSequence, FirstTick, RunId, TimelineId),
			Receipt));
		check(Chain.TryObserve(
			MakeBindingObservation(
				SecondSequence, SecondTick, RunId, TimelineId),
			Receipt));
		check(Receipt.GetBand() == EShanmenSwordRhythmBand::PreciseLinked);
		FShanmenSwordRhythmContribution Contribution;
		check(FShanmenSwordRhythmContribution::TryCapturePreciseSwordLink(
			Receipt, Contribution));
		return Contribution;
	}

	FShanmenSwordRhythmContributionBindingScope MakeBindingScope()
	{
		FShanmenSwordRhythmContributionBindingScope Scope;
		check(FShanmenSwordRhythmContributionBindingScope::TryCapture(
			BindingRun, BindingOwner, BindingTimeline, Scope));
		return Scope;
	}

	FShanmenSwordRhythmContributionBindingLedger MakeBindingLedger()
	{
		FShanmenSwordRhythmContributionBindingLedger Ledger;
		check(FShanmenSwordRhythmContributionBindingLedger::TryCreate(
			MakeBindingScope(), Ledger));
		return Ledger;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenSwordRhythmContributionBindingNextActionTest,
	"Shanmen.0_0_10.CombatRuntime.SwordRhythmContributionBinding.NextAction",
	BindingFlags)

bool FShanmenSwordRhythmContributionBindingNextActionTest::RunTest(
	const FString&)
{
	FShanmenSwordRhythmContributionBindingLedger Ledger = MakeBindingLedger();
	FShanmenSwordRhythmContributionBindingReceipt Receipt;
	TestTrue(TEXT("an empty earlier action is still recorded"),
		Ledger.TryObserveBasicSword(
			MakeBindingObservation(1300, 50), Receipt));
	TestFalse(TEXT("an empty observation emits no fake binding receipt"),
		Receipt.IsValid());

	const FShanmenSwordRhythmContribution Contribution =
		MakePreciseContribution(1301, 1302, 100, 108);
	TestTrue(TEXT("valid in-scope evidence becomes pending"),
		Ledger.TryRecordContribution(Contribution));
	TestTrue(TEXT("the next later BasicSword binds pending evidence once"),
		Ledger.TryObserveBasicSword(
			MakeBindingObservation(1303, 120), Receipt));
	TestTrue(TEXT("binding receipt preserves target and source evidence"),
		Receipt.IsValid()
			&& Receipt.NumContributions() == 1
			&& Receipt.GetContributions()[0].GetContributionId()
				== Contribution.GetContributionId()
			&& Receipt.GetTargetObservation().GetInputTick() == 120);
	TestTrue(TEXT("bound evidence leaves pending exactly once"),
		Ledger.IsValid()
			&& Ledger.NumPending() == 0
			&& Ledger.NumBoundContributions() == 1
			&& Ledger.NumObservedActions() == 2
			&& Ledger.NumBindingReceipts() == 1
			&& Ledger.IsBound(Contribution.GetContributionId()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenSwordRhythmContributionBindingReplayTest,
	"Shanmen.0_0_10.CombatRuntime.SwordRhythmContributionBinding.DeterministicReplay",
	BindingFlags)

bool FShanmenSwordRhythmContributionBindingReplayTest::RunTest(
	const FString&)
{
	const FShanmenSwordRhythmContribution Earlier =
		MakePreciseContribution(1310, 1311, 100, 108);
	const FShanmenSwordRhythmContribution Later =
		MakePreciseContribution(1312, 1313, 200, 208);
	const FShanmenSwordRhythmObservation Target =
		MakeBindingObservation(1314, 220);

	FShanmenSwordRhythmContributionBindingLedger First = MakeBindingLedger();
	TestTrue(TEXT("reverse delivery is accepted before any target"),
		First.TryRecordContribution(Later)
			&& First.TryRecordContribution(Earlier));
	TestTrue(TEXT("exact pending replay is idempotent"),
		First.TryRecordContribution(Earlier)
			&& First.NumPending() == 2);
	FShanmenSwordRhythmContributionBindingReceipt FirstReceipt;
	check(First.TryObserveBasicSword(Target, FirstReceipt));

	FShanmenSwordRhythmContributionBindingLedger Replay = MakeBindingLedger();
	check(Replay.TryRecordContribution(Earlier));
	check(Replay.TryRecordContribution(Later));
	FShanmenSwordRhythmContributionBindingReceipt ReplayReceipt;
	check(Replay.TryObserveBasicSword(Target, ReplayReceipt));
	TestTrue(TEXT("canonical ordering removes insertion-order identity drift"),
		FirstReceipt.IsValid()
			&& ReplayReceipt.IsValid()
			&& FirstReceipt.GetReceiptId() == ReplayReceipt.GetReceiptId()
			&& FirstReceipt.GetContributions()[0].GetContributionId()
				== Earlier.GetContributionId());

	FShanmenSwordRhythmContributionBindingReceipt ExactReplay;
	TestTrue(TEXT("exact target replay returns the original receipt"),
		First.TryObserveBasicSword(Target, ExactReplay)
			&& ExactReplay.GetReceiptId() == FirstReceipt.GetReceiptId()
			&& First.NumBindingReceipts() == 1);
	TestTrue(TEXT("already-bound evidence replays without becoming pending"),
		First.TryRecordContribution(Earlier)
			&& First.NumPending() == 0
			&& First.NumBoundContributions() == 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenSwordRhythmContributionBindingSelfSourceTest,
	"Shanmen.0_0_10.CombatRuntime.SwordRhythmContributionBinding.SourceWaitsForNext",
	BindingFlags)

bool FShanmenSwordRhythmContributionBindingSelfSourceTest::RunTest(
	const FString&)
{
	const FShanmenSwordRhythmContribution Contribution =
		MakePreciseContribution(1320, 1321, 100, 108);
	FShanmenSwordRhythmObservation SourceObservation;
	check(FShanmenSwordRhythmObservation::TryCapture(
		Contribution.GetAction(),
		BindingTimeline,
		Contribution.GetObservedTick(),
		SourceObservation));

	FShanmenSwordRhythmContributionBindingLedger Ledger = MakeBindingLedger();
	check(Ledger.TryRecordContribution(Contribution));
	FShanmenSwordRhythmContributionBindingReceipt Receipt;
	TestTrue(TEXT("source BasicSword itself is observed normally"),
		Ledger.TryObserveBasicSword(SourceObservation, Receipt));
	TestTrue(TEXT("a precise link cannot strengthen its own activation"),
		!Receipt.IsValid()
			&& Ledger.NumPending() == 1
			&& Ledger.ContainsPending(Contribution.GetContributionId()));
	TestTrue(TEXT("the following activation consumes the retained fact"),
		Ledger.TryObserveBasicSword(
			MakeBindingObservation(1322, 116), Receipt)
			&& Receipt.IsValid()
			&& Receipt.NumContributions() == 1
			&& Ledger.NumPending() == 0);

	FShanmenSwordRhythmContributionBindingLedger AlternateOrder =
		MakeBindingLedger();
	FShanmenSwordRhythmContributionBindingReceipt Empty;
	check(AlternateOrder.TryObserveBasicSword(SourceObservation, Empty));
	TestTrue(TEXT("same-tick source evidence may arrive after observation"),
		AlternateOrder.TryRecordContribution(Contribution));
	FShanmenSwordRhythmContributionBindingReceipt AlternateReceipt;
	check(AlternateOrder.TryObserveBasicSword(
		MakeBindingObservation(1322, 116), AlternateReceipt));
	TestTrue(TEXT("both delivery orders bind to the same next action"),
		AlternateReceipt.IsValid()
			&& AlternateReceipt.GetReceiptId() == Receipt.GetReceiptId());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenSwordRhythmContributionBindingFailClosedTest,
	"Shanmen.0_0_10.CombatRuntime.SwordRhythmContributionBinding.FailClosedAtomicity",
	BindingFlags)

bool FShanmenSwordRhythmContributionBindingFailClosedTest::RunTest(
	const FString&)
{
	FShanmenSwordRhythmContributionBindingLedger Ledger = MakeBindingLedger();
	FShanmenSwordRhythmContributionBindingReceipt Output;
	check(Ledger.TryObserveBasicSword(
		MakeBindingObservation(1330, 100), Output));
	const int32 ObservedBefore = Ledger.NumObservedActions();

	TestFalse(TEXT("cross-Run contribution fails closed"),
		Ledger.TryRecordContribution(
			MakePreciseContribution(
				1331, 1332, 108, 116, AlternateRun)));
	TestFalse(TEXT("cross-timeline contribution fails closed"),
		Ledger.TryRecordContribution(
			MakePreciseContribution(
				1333, 1334, 108, 116,
				BindingRun, AlternateTimeline)));
	TestFalse(TEXT("late older evidence cannot backfill an observed action"),
		Ledger.TryRecordContribution(
			MakePreciseContribution(1335, 1336, 80, 88)));
	TestFalse(TEXT("regressed target time fails closed"),
		Ledger.TryObserveBasicSword(
			MakeBindingObservation(1337, 99), Output));
	TestFalse(TEXT("failed observation clears reusable receipt output"),
		Output.IsValid());
	TestTrue(TEXT("rejections leave ledger state unchanged"),
		Ledger.IsValid()
			&& Ledger.NumObservedActions() == ObservedBefore
			&& Ledger.NumPending() == 0
			&& Ledger.NumBoundContributions() == 0);

	const FShanmenSwordRhythmContribution Future =
		MakePreciseContribution(1338, 1339, 192, 200);
	check(Ledger.TryRecordContribution(Future));
	TestTrue(TEXT("an earlier target cannot consume future evidence"),
		Ledger.TryObserveBasicSword(
			MakeBindingObservation(1340, 150), Output)
			&& !Output.IsValid()
			&& Ledger.ContainsPending(Future.GetContributionId()));
	TestTrue(TEXT("the first eligible later target consumes it"),
		Ledger.TryObserveBasicSword(
			MakeBindingObservation(1341, 208), Output)
			&& Output.IsValid()
			&& Ledger.IsBound(Future.GetContributionId()));

	const FShanmenSwordRhythmObservation Existing =
		MakeBindingObservation(1341, 208);
	FShanmenSwordRhythmObservation Conflict;
	check(FShanmenSwordRhythmObservation::TryCapture(
		Existing.GetAction(), BindingTimeline, 209, Conflict));
	const int32 BindingsBeforeConflict = Ledger.NumBindingReceipts();
	TestFalse(TEXT("one activation cannot be replayed with another tick"),
		Ledger.TryObserveBasicSword(Conflict, Output));
	TestTrue(TEXT("conflicting replay is atomic"),
		Ledger.IsValid()
			&& Ledger.NumBindingReceipts() == BindingsBeforeConflict
			&& Ledger.NumPending() == 0);
	return true;
}

#endif
