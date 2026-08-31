#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ShanmenBasicSwordExecution.h"
#include "ShanmenCombatResolver.h"
#include "ShanmenCombatTags.h"
#include "ShanmenSwordRhythm.h"

namespace
{
	const FGuid RhythmRunId(
		0xC1200001, 0xC1200002, 0xC1200003, 0xC1200004);
	const FGuid RhythmOwnerId(
		0xC1210001, 0xC1210002, 0xC1210003, 0xC1210004);
	const FGuid RhythmSourceId(
		0xC1220001, 0xC1220002, 0xC1220003, 0xC1220004);
	const FGuid RhythmWeaponId(
		0xC1230001, 0xC1230002, 0xC1230003, 0xC1230004);
	const FGuid AlternateWeaponId(
		0xC1240001, 0xC1240002, 0xC1240003, 0xC1240004);
	const FGuid RhythmTimelineId(
		0xC1250001, 0xC1250002, 0xC1250003, 0xC1250004);
	const FGuid AlternateTimelineId(
		0xC1260001, 0xC1260002, 0xC1260003, 0xC1260004);

	FShanmenCombatActionSnapshot MakeAction(
		uint64 ActivationSequence,
		const FGuid& RunId = RhythmRunId,
		const FGuid& WeaponId = RhythmWeaponId,
		const FString& Digest = TEXT("TEST-DIGEST-P12.0"))
	{
		FShanmenCombatActionCapture Capture;
		Capture.RunId = RunId;
		Capture.OwnerId = RhythmOwnerId;
		Capture.SourceEntityId = RhythmSourceId;
		Capture.SourceItemInstanceId = WeaponId;
		Capture.ActionDefinitionId =
			FShanmenBasicSwordDefinition::CanonicalActionDefinitionId();
		Capture.Content.Version = TEXT("0.0.10.P12.0");
		Capture.Content.Digest = Digest;
		Capture.SourceTags.AddTag(
			FShanmenCombatNativeTags::SourcePlayer());
		Capture.ActivationId = FShanmenCombatIdFactory::MakeActivationId(
			Capture.RunId,
			Capture.SourceEntityId,
			Capture.ActionDefinitionId,
			ActivationSequence);
		FShanmenCombatActionSnapshot Action;
		check(FShanmenCombatActionSnapshot::TryCapture(Capture, Action));
		return Action;
	}

	FShanmenSwordRhythmDefinitionCapture MakeDefinitionCapture(
		int64 OpenTick = 8,
		int64 CloseTick = 13,
		FName RuleId = TEXT("Combat.Style.Sword.Taiji01.LinkWindow.r1"))
	{
		FShanmenSwordRhythmDefinitionCapture Capture;
		Capture.StyleDefinitionId =
			FShanmenSwordRhythmDefinition::CanonicalStyleDefinitionId();
		Capture.RuleId = RuleId;
		Capture.LinkOpenOffsetTicks = OpenTick;
		Capture.LinkCloseOffsetTicks = CloseTick;
		return Capture;
	}

	FShanmenSwordRhythmDefinition MakeDefinition(
		int64 OpenTick = 8,
		int64 CloseTick = 13,
		FName RuleId = TEXT("Combat.Style.Sword.Taiji01.LinkWindow.r1"))
	{
		FShanmenSwordRhythmDefinition Definition;
		check(FShanmenSwordRhythmDefinition::TryCapture(
			MakeDefinitionCapture(OpenTick, CloseTick, RuleId),
			Definition));
		return Definition;
	}

	FShanmenSwordRhythmObservation MakeObservation(
		uint64 ActivationSequence,
		int64 InputTick,
		const FGuid& TimelineId = RhythmTimelineId,
		const FGuid& RunId = RhythmRunId,
		const FGuid& WeaponId = RhythmWeaponId,
		const FString& Digest = TEXT("TEST-DIGEST-P12.0"))
	{
		FShanmenSwordRhythmObservation Observation;
		check(FShanmenSwordRhythmObservation::TryCapture(
			MakeAction(ActivationSequence, RunId, WeaponId, Digest),
			TimelineId,
			InputTick,
			Observation));
		return Observation;
	}

	FShanmenSwordRhythmChain MakeChain(
		const FShanmenSwordRhythmDefinition& Definition = MakeDefinition())
	{
		FShanmenSwordRhythmChain Chain;
		check(FShanmenSwordRhythmChain::TryCreate(Definition, Chain));
		return Chain;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenSwordRhythmDefinitionContractTest,
	"Shanmen.0_0_10.CombatRuntime.SwordRhythm.DefinitionContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenSwordRhythmDefinitionContractTest::RunTest(const FString&)
{
	const FShanmenSwordRhythmDefinition Definition = MakeDefinition();
	TestTrue(TEXT("Canonical Tai Chi definition captures"), Definition.IsValid());
	TestTrue(TEXT("Definition supports the real BasicSword action"),
		Definition.GetSupportedActionDefinitionId()
			== FShanmenBasicSwordDefinition::CanonicalActionDefinitionId());

	FShanmenSwordRhythmDefinition Output = Definition;
	FShanmenSwordRhythmDefinitionCapture Invalid = MakeDefinitionCapture();
	Invalid.StyleDefinitionId = TEXT("Combat.Style.Sword.Unknown");
	TestFalse(TEXT("Noncanonical style fails closed"),
		FShanmenSwordRhythmDefinition::TryCapture(Invalid, Output));
	TestFalse(TEXT("Failed capture clears reusable output"), Output.IsValid());
	Invalid = MakeDefinitionCapture(8, 8);
	TestFalse(TEXT("Empty half-open timing window is rejected"),
		FShanmenSwordRhythmDefinition::TryCapture(Invalid, Output));

	FShanmenSwordRhythmObservation Observation;
	TestFalse(TEXT("Negative caller tick is rejected"),
		FShanmenSwordRhythmObservation::TryCapture(
			MakeAction(1), RhythmTimelineId, -1, Observation));
	TestFalse(TEXT("Rejected observation clears output"), Observation.IsValid());

	FShanmenCombatActionCapture NonSwordCapture;
	NonSwordCapture.RunId = RhythmRunId;
	NonSwordCapture.OwnerId = RhythmOwnerId;
	NonSwordCapture.SourceEntityId = RhythmSourceId;
	NonSwordCapture.SourceItemInstanceId = RhythmWeaponId;
	NonSwordCapture.ActionDefinitionId = TEXT("Combat.Action.Spear.Basic01");
	NonSwordCapture.Content.Version = TEXT("0.0.10.P12.0");
	NonSwordCapture.Content.Digest = TEXT("TEST-DIGEST-P12.0");
	NonSwordCapture.ActivationId = FShanmenCombatIdFactory::MakeActivationId(
		NonSwordCapture.RunId,
		NonSwordCapture.SourceEntityId,
		NonSwordCapture.ActionDefinitionId,
		2);
	FShanmenCombatActionSnapshot NonSword;
	check(FShanmenCombatActionSnapshot::TryCapture(
		NonSwordCapture, NonSword));
	TestFalse(TEXT("Other actions cannot enter the BasicSword chain"),
		FShanmenSwordRhythmObservation::TryCapture(
			NonSword, RhythmTimelineId, 10, Observation));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenSwordRhythmPreciseBoundaryTest,
	"Shanmen.0_0_10.CombatRuntime.SwordRhythm.PreciseBoundaries",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenSwordRhythmPreciseBoundaryTest::RunTest(const FString&)
{
	FShanmenSwordRhythmChain Chain = MakeChain();
	FShanmenSwordRhythmReceipt Receipt;
	TestTrue(TEXT("First observation starts a one-count chain"),
		Chain.TryObserve(MakeObservation(10, 100), Receipt));
	TestTrue(TEXT("Start receipt is self-validating"),
		Receipt.IsValid()
			&& Receipt.GetBand() == EShanmenSwordRhythmBand::Started
			&& Receipt.GetResultingChainCount() == 1
			&& !Receipt.HasPreviousObservation());

	TestTrue(TEXT("Inclusive open boundary links precisely"),
		Chain.TryObserve(MakeObservation(11, 108), Receipt));
	TestTrue(TEXT("Precise link increments to two"),
		Receipt.IsValid()
			&& Receipt.GetBand()
				== EShanmenSwordRhythmBand::PreciseLinked
			&& Receipt.GetPreviousChainCount() == 1
			&& Receipt.GetResultingChainCount() == 2);

	TestTrue(TEXT("Tick before exclusive close remains precise"),
		Chain.TryObserve(MakeObservation(12, 120), Receipt));
	TestTrue(TEXT("Second precise link increments to three"),
		Receipt.GetBand() == EShanmenSwordRhythmBand::PreciseLinked
			&& Receipt.GetResultingChainCount() == 3);

	TestTrue(TEXT("Exclusive close boundary restarts late"),
		Chain.TryObserve(MakeObservation(13, 133), Receipt));
	TestTrue(TEXT("Late receipt records old count and resets to one"),
		Receipt.IsValid()
			&& Receipt.GetBand()
				== EShanmenSwordRhythmBand::RestartedLate
			&& Receipt.GetPreviousChainCount() == 3
			&& Receipt.GetResultingChainCount() == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenSwordRhythmEarlyRestartAnchorTest,
	"Shanmen.0_0_10.CombatRuntime.SwordRhythm.EarlyRestartBecomesAnchor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenSwordRhythmEarlyRestartAnchorTest::RunTest(const FString&)
{
	FShanmenSwordRhythmChain Chain = MakeChain();
	FShanmenSwordRhythmReceipt Receipt;
	check(Chain.TryObserve(MakeObservation(20, 200), Receipt));
	TestTrue(TEXT("Too-early input is accepted as a fresh anchor"),
		Chain.TryObserve(MakeObservation(21, 207), Receipt));
	TestTrue(TEXT("Early receipt resets without discarding audit history"),
		Receipt.IsValid()
			&& Receipt.GetBand()
				== EShanmenSwordRhythmBand::RestartedEarly
			&& Receipt.GetPreviousChainCount() == 1
			&& Receipt.GetResultingChainCount() == 1
			&& Chain.NumRecordedObservations() == 2);
	TestTrue(TEXT("Next input links from the early restart anchor"),
		Chain.TryObserve(MakeObservation(22, 215), Receipt));
	TestTrue(TEXT("New-anchor precise link reaches count two"),
		Receipt.GetBand() == EShanmenSwordRhythmBand::PreciseLinked
			&& Receipt.GetPreviousObservation().GetInputTick() == 207
			&& Receipt.GetResultingChainCount() == 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenSwordRhythmIdempotencyTest,
	"Shanmen.0_0_10.CombatRuntime.SwordRhythm.IdempotentReplay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenSwordRhythmIdempotencyTest::RunTest(const FString&)
{
	FShanmenSwordRhythmChain Chain = MakeChain();
	const FShanmenSwordRhythmObservation First = MakeObservation(30, 300);
	FShanmenSwordRhythmReceipt Original;
	check(Chain.TryObserve(First, Original));
	FShanmenSwordRhythmReceipt Replay;
	TestTrue(TEXT("Exact delivery replay returns the original receipt"),
		Chain.TryObserve(First, Replay));
	TestTrue(TEXT("Replay identity and count are unchanged"),
		Replay.IsValid()
			&& Replay.GetReceiptId() == Original.GetReceiptId()
			&& Chain.NumRecordedObservations() == 1
			&& Chain.GetCurrentChainCount() == 1);

	FShanmenSwordRhythmObservation ConflictingTick;
	check(FShanmenSwordRhythmObservation::TryCapture(
		First.GetAction(), RhythmTimelineId, 301, ConflictingTick));
	TestFalse(TEXT("Same activation with a different observation fails closed"),
		Chain.TryObserve(ConflictingTick, Replay));
	TestFalse(TEXT("Conflict clears reusable receipt output"), Replay.IsValid());
	TestTrue(TEXT("Conflict leaves chain atomically unchanged"),
		Chain.IsValid()
			&& Chain.NumRecordedObservations() == 1
			&& Chain.GetLastObservation().GetObservationId()
				== First.GetObservationId());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenSwordRhythmScopeAtomicityTest,
	"Shanmen.0_0_10.CombatRuntime.SwordRhythm.ScopeAndTickAtomicity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenSwordRhythmScopeAtomicityTest::RunTest(const FString&)
{
	FShanmenSwordRhythmChain Chain = MakeChain();
	FShanmenSwordRhythmReceipt Receipt;
	const FShanmenSwordRhythmObservation First = MakeObservation(40, 400);
	check(Chain.TryObserve(First, Receipt));
	const FGuid OriginalReceiptId = Receipt.GetReceiptId();

	TestFalse(TEXT("A different caller timeline cannot continue the chain"),
		Chain.TryObserve(
			MakeObservation(41, 408, AlternateTimelineId), Receipt));
	TestFalse(TEXT("A different weapon cannot inherit rhythm count"),
		Chain.TryObserve(
			MakeObservation(
				42,
				408,
				RhythmTimelineId,
				RhythmRunId,
				AlternateWeaponId),
			Receipt));
	TestFalse(TEXT("A different run cannot inherit rhythm count"),
		Chain.TryObserve(
			MakeObservation(
				43,
				408,
				RhythmTimelineId,
				FGuid(0xC1270001, 0, 0, 1)),
			Receipt));
	TestFalse(TEXT("Content identity drift cannot continue the chain"),
		Chain.TryObserve(
			MakeObservation(
				44,
				408,
				RhythmTimelineId,
				RhythmRunId,
				RhythmWeaponId,
				TEXT("OTHER-DIGEST")),
			Receipt));
	TestFalse(TEXT("Regressed tick is rejected instead of underflowing"),
		Chain.TryObserve(MakeObservation(45, 399), Receipt));
	TestTrue(TEXT("Every rejected scope sample leaves the first anchor intact"),
		Chain.IsValid()
			&& Chain.NumRecordedObservations() == 1
			&& Chain.GetCurrentChainCount() == 1
			&& Chain.GetLastObservation().GetObservationId()
				== First.GetObservationId());

	FShanmenSwordRhythmReceipt FirstReplay;
	TestTrue(TEXT("Original anchor remains replayable after all rejections"),
		Chain.TryObserve(First, FirstReplay));
	TestTrue(TEXT("Original immutable receipt is preserved"),
		FirstReplay.GetReceiptId() == OriginalReceiptId);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenSwordRhythmDeterminismTest,
	"Shanmen.0_0_10.CombatRuntime.SwordRhythm.DeterministicReplayAndIdentity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenSwordRhythmDeterminismTest::RunTest(const FString&)
{
	const FShanmenSwordRhythmDefinition Definition = MakeDefinition();
	const FShanmenSwordRhythmDefinition SameDefinition = MakeDefinition();
	const FShanmenSwordRhythmDefinition OtherDefinition = MakeDefinition(
		8, 14, TEXT("Combat.Style.Sword.Taiji01.LinkWindow.r2"));
	TestTrue(TEXT("Equal content yields equal deterministic DefinitionId"),
		Definition.GetDefinitionId() == SameDefinition.GetDefinitionId());
	TestTrue(TEXT("Timing/rule content separates DefinitionId"),
		Definition.GetDefinitionId() != OtherDefinition.GetDefinitionId());

	FShanmenSwordRhythmChain Left = MakeChain(Definition);
	FShanmenSwordRhythmChain Right = MakeChain(SameDefinition);
	FShanmenSwordRhythmReceipt LeftReceipt;
	FShanmenSwordRhythmReceipt RightReceipt;
	for (int32 Index = 0; Index < 3; ++Index)
	{
		const FShanmenSwordRhythmObservation Observation =
			MakeObservation(50 + Index, 500 + Index * 8);
		TestTrue(TEXT("Left deterministic replay accepts observation"),
			Left.TryObserve(Observation, LeftReceipt));
		TestTrue(TEXT("Right deterministic replay accepts observation"),
			Right.TryObserve(Observation, RightReceipt));
		TestTrue(TEXT("Equivalent history yields identical receipt identity"),
			LeftReceipt.GetReceiptId() == RightReceipt.GetReceiptId());
	}
	TestTrue(TEXT("Equivalent histories end in identical valid state"),
		Left.IsValid()
			&& Right.IsValid()
			&& Left.GetCurrentChainCount() == 3
			&& Right.GetCurrentChainCount() == 3);

	Left.Reset();
	TestFalse(TEXT("Reset removes definition and invalidates default chain"),
		Left.IsValid());
	TestEqual(TEXT("Reset removes all retained observations"),
		Left.NumRecordedObservations(), 0);
	return true;
}

#endif
