#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "demo_mapCombatRunCoordinator.h"
#include "demo_mapPlayerHealthComponent.h"
#include "demo_mapShanmenSwordRhythmProductHost.h"

namespace
{
	constexpr EAutomationTestFlags SwordRhythmHostFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;
	const FGuid SwordRhythmRunA(
		0xA5F10001, 0xA5F10002, 0xA5F10003, 0xA5F10004);
	const FGuid SwordRhythmRunB(
		0xA5F20001, 0xA5F20002, 0xA5F20003, 0xA5F20004);
	const FGuid SwordRhythmWeapon(
		0xA5F30001, 0xA5F30002, 0xA5F30003, 0xA5F30004);

	FShanmenSwordRhythmDefinition MakeSwordRhythmDefinition()
	{
		FShanmenSwordRhythmDefinitionCapture Capture;
		Capture.StyleDefinitionId =
			FShanmenSwordRhythmDefinition::CanonicalStyleDefinitionId();
		Capture.RuleId = TEXT("Combat.Style.Sword.Taiji01.BasicLink.r1");
		Capture.LinkOpenOffsetTicks = 8;
		Capture.LinkCloseOffsetTicks = 12;
		FShanmenSwordRhythmDefinition Definition;
		check(FShanmenSwordRhythmDefinition::TryCapture(
			Capture,
			Definition));
		return Definition;
	}

	struct FSwordRhythmProductFixture
	{
		APawn* Pawn = nullptr;
		UBoxComponent* CollisionRoot = nullptr;
		Udemo_mapPlayerHealthComponent* Health = nullptr;
		Fdemo_mapCombatRunCoordinator Coordinator;
		Fdemo_mapShanmenCombatRunFixedTimeline Timeline;
		FString Diagnostic;
		bool bReady = false;

		explicit FSwordRhythmProductFixture(const FGuid& RunId)
		{
			Pawn = NewObject<APawn>(GetTransientPackage());
			CollisionRoot = Pawn
				? NewObject<UBoxComponent>(Pawn, TEXT("PlayerCollisionRoot"))
				: nullptr;
			Health = Pawn
				? NewObject<Udemo_mapPlayerHealthComponent>(
					Pawn,
					TEXT("PlayerHealth"))
				: nullptr;
			if (Pawn && CollisionRoot)
			{
				Pawn->SetRootComponent(CollisionRoot);
			}
			bReady = Pawn && CollisionRoot && Health
				&& Coordinator.TryBeginRun(
					RunId,
					Pawn,
					Health,
					Diagnostic)
				&& Timeline.TryBegin(RunId, Diagnostic);
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmProductHostRealBasicSwordTest,
	"Shanmen.0_0_10.Product.SwordRhythmProductHost.RealBasicSwordBridge",
	SwordRhythmHostFlags)

bool Fdemo_mapSwordRhythmProductHostRealBasicSwordTest::RunTest(
	const FString&)
{
	FSwordRhythmProductFixture Fixture(SwordRhythmRunA);
	Fdemo_mapShanmenSwordRhythmProductHost Host;
	const FShanmenSwordRhythmDefinition Definition =
		MakeSwordRhythmDefinition();
	TestTrue(TEXT("real product fixture and content-owned Host initialize"),
		Fixture.bReady
			&& Host.TryBegin(
				SwordRhythmRunA,
				Definition,
				Fixture.Diagnostic));
	if (!Fixture.bReady || Host.IsEmpty())
	{
		AddError(Fixture.Diagnostic);
		return false;
	}

	const Fdemo_mapBasicSwordProductExecutionResult First =
		Fixture.Coordinator.ExecutePlayerBasicSwordSweep(
			SwordRhythmWeapon,
			1.0f,
			{});
	Fdemo_mapShanmenCombatRunTimelineSample FirstSample;
	FShanmenSwordRhythmReceipt FirstReceipt;
	TestTrue(TEXT("completed legal miss yields an immutable action fact"),
		First.IsExecuted()
			&& !First.AppliedDamage()
			&& First.Action.GetRunId() == SwordRhythmRunA
			&& Fixture.Timeline.TryCapture(FirstSample));
	TestTrue(TEXT("first real BasicSword starts the rhythm chain"),
		Host.TryObserveExecutedBasicSword(
				First,
				FirstSample,
				FirstReceipt,
				Fixture.Diagnostic)
			&& FirstReceipt.GetBand() == EShanmenSwordRhythmBand::Started
			&& FirstReceipt.GetResultingChainCount() == 1);

	int64 AdvancedTicks = 0;
	TestTrue(TEXT("shared Run timeline reaches the authored open boundary"),
		Fixture.Timeline.TryAdvance(
			8.0 / 30.0,
			AdvancedTicks,
			Fixture.Diagnostic)
			&& AdvancedTicks == 8);
	const Fdemo_mapBasicSwordProductExecutionResult Second =
		Fixture.Coordinator.ExecutePlayerBasicSwordSweep(
			SwordRhythmWeapon,
			1.0f,
			{});
	Fdemo_mapShanmenCombatRunTimelineSample SecondSample;
	FShanmenSwordRhythmReceipt SecondReceipt;
	TestTrue(TEXT("second action and exact timeline sample are valid"),
		Second.IsExecuted()
			&& Second.ActivationId != First.ActivationId
			&& Fixture.Timeline.TryCapture(SecondSample));
	TestTrue(TEXT("second real BasicSword links precisely without changing damage"),
		Host.TryObserveExecutedBasicSword(
				Second,
				SecondSample,
				SecondReceipt,
				Fixture.Diagnostic)
			&& SecondReceipt.GetBand()
				== EShanmenSwordRhythmBand::PreciseLinked
			&& SecondReceipt.GetResultingChainCount() == 2
			&& !Second.AppliedDamage());

	FShanmenSwordRhythmReceipt ReplayReceipt;
	TestTrue(TEXT("exact bridge replay is idempotent"),
		Host.TryObserveExecutedBasicSword(
				Second,
				SecondSample,
				ReplayReceipt,
				Fixture.Diagnostic)
			&& ReplayReceipt.GetReceiptId() == SecondReceipt.GetReceiptId()
			&& Host.GetChain().NumRecordedObservations() == 2
			&& Host.GetChain().GetCurrentChainCount() == 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmProductHostFailClosedTest,
	"Shanmen.0_0_10.Product.SwordRhythmProductHost.FailClosedAtomic",
	SwordRhythmHostFlags)

bool Fdemo_mapSwordRhythmProductHostFailClosedTest::RunTest(
	const FString&)
{
	FSwordRhythmProductFixture FixtureA(SwordRhythmRunA);
	FSwordRhythmProductFixture FixtureB(SwordRhythmRunB);
	Fdemo_mapShanmenSwordRhythmProductHost Host;
	const FShanmenSwordRhythmDefinition Definition =
		MakeSwordRhythmDefinition();
	TestTrue(TEXT("fail-closed fixtures initialize"),
		FixtureA.bReady
			&& FixtureB.bReady
			&& Host.TryBegin(
				SwordRhythmRunA,
				Definition,
				FixtureA.Diagnostic));
	if (!FixtureA.bReady || !FixtureB.bReady || Host.IsEmpty())
	{
		AddError(FixtureA.Diagnostic);
		return false;
	}

	const Fdemo_mapBasicSwordProductExecutionResult ExecutedA =
		FixtureA.Coordinator.ExecutePlayerBasicSwordSweep(
			SwordRhythmWeapon,
			1.0f,
			{});
	const Fdemo_mapBasicSwordProductExecutionResult ExecutedB =
		FixtureB.Coordinator.ExecutePlayerBasicSwordSweep(
			SwordRhythmWeapon,
			1.0f,
			{});
	Fdemo_mapShanmenCombatRunTimelineSample SampleA;
	Fdemo_mapShanmenCombatRunTimelineSample SampleB;
	FixtureA.Timeline.TryCapture(SampleA);
	FixtureB.Timeline.TryCapture(SampleB);
	FShanmenSwordRhythmReceipt Receipt;
	FString Diagnostic;
	const Fdemo_mapBasicSwordProductExecutionResult NotExecuted;
	TestFalse(TEXT("non-executed action result is rejected"),
		Host.TryObserveExecutedBasicSword(
			NotExecuted,
			SampleA,
			Receipt,
			Diagnostic));
	TestFalse(TEXT("foreign Run action is rejected"),
		Host.TryObserveExecutedBasicSword(
			ExecutedB,
			SampleA,
			Receipt,
			Diagnostic));
	TestFalse(TEXT("foreign Run timeline sample is rejected"),
		Host.TryObserveExecutedBasicSword(
			ExecutedA,
			SampleB,
			Receipt,
			Diagnostic));

	Fdemo_mapBasicSwordProductExecutionResult Tampered = ExecutedA;
	Tampered.ActivationId = SwordRhythmRunB;
	TestFalse(TEXT("mismatched result identity is not treated as executed"),
		Tampered.IsExecuted());
	TestFalse(TEXT("mismatched result identity is rejected by the Host"),
		Host.TryObserveExecutedBasicSword(
			Tampered,
			SampleA,
			Receipt,
			Diagnostic));
	TestTrue(TEXT("all rejected observations preserve the empty chain"),
		Host.IsValid()
			&& Host.GetChain().NumRecordedObservations() == 0
			&& Host.GetChain().GetCurrentChainCount() == 0);
	TestFalse(TEXT("mismatched teardown is rejected atomically"),
		Host.TryEnd(SwordRhythmRunB, Diagnostic));
	TestTrue(TEXT("matching teardown clears product state"),
		Host.TryEnd(SwordRhythmRunA, Diagnostic)
			&& Host.IsEmpty());
	return true;
}

#endif
