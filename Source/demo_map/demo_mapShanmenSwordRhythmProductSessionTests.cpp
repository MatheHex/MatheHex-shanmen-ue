#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "demo_mapCombatRunCoordinator.h"
#include "demo_mapPlayerHealthComponent.h"
#include "demo_mapShanmenSwordRhythmProductSession.h"

namespace
{
	constexpr EAutomationTestFlags SwordRhythmSessionFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;
	const FGuid SwordRhythmSessionRunA(
		0xA5F40001, 0xA5F40002, 0xA5F40003, 0xA5F40004);
	const FGuid SwordRhythmSessionRunB(
		0xA5F50001, 0xA5F50002, 0xA5F50003, 0xA5F50004);
	const FGuid SwordRhythmSessionWeapon(
		0xA5F60001, 0xA5F60002, 0xA5F60003, 0xA5F60004);

	struct FSwordRhythmSessionFixture
	{
		APawn* Pawn = nullptr;
		UBoxComponent* CollisionRoot = nullptr;
		Udemo_mapPlayerHealthComponent* Health = nullptr;
		Fdemo_mapCombatRunCoordinator Coordinator;
		Fdemo_mapShanmenCombatRunFixedTimeline Timeline;
		FString Diagnostic;
		bool bReady = false;

		explicit FSwordRhythmSessionFixture(const FGuid& RunId)
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
	Fdemo_mapSwordRhythmCanonicalProductConfigTest,
	"Shanmen.0_0_10.Product.SwordRhythmProductSession.CanonicalConfig",
	SwordRhythmSessionFlags)

bool Fdemo_mapSwordRhythmCanonicalProductConfigTest::RunTest(
	const FString&)
{
	Fdemo_mapShanmenSwordRhythmProductConfig First;
	Fdemo_mapShanmenSwordRhythmProductConfig Second;
	TestTrue(TEXT("canonical versioned product configs capture"),
		Fdemo_mapShanmenSwordRhythmProductConfig::TryCreateCanonical(First)
			&& Fdemo_mapShanmenSwordRhythmProductConfig::
				TryCreateCanonical(Second)
			&& First.IsValid()
			&& Second.IsValid());
	TestTrue(TEXT("equivalent content derives one deterministic config ID"),
		First.GetConfigId()
			== Fdemo_mapShanmenSwordRhythmProductConfig::CanonicalConfigId()
			&& Second.GetConfigId() == First.GetConfigId());
	TestTrue(TEXT("initial timing content is explicit at 30 Hz"),
		First.GetContent().Version == TEXT("0.0.10.P12.2")
			&& First.GetContent().Digest
				== TEXT("Shanmen.SwordRhythm.ProductConfig.r1")
			&& First.GetTimelineTicksPerSecond() == 30
			&& First.GetDefinition().GetLinkOpenOffsetTicks() == 8
			&& First.GetDefinition().GetLinkCloseOffsetTicks() == 13
			&& First.GetDefinition().GetRuleId()
				== TEXT("Combat.Style.Sword.Taiji01.BasicLinkWindow.r1"));

	Fdemo_mapShanmenSwordRhythmProductSession Session;
	FString Diagnostic;
	TestTrue(TEXT("Session installs the canonical content for its Run"),
		Session.TryBegin(SwordRhythmSessionRunA, Diagnostic)
			&& Session.IsValid()
			&& !Session.IsEmpty()
			&& Session.GetConfig().GetConfigId() == First.GetConfigId());
	TestTrue(TEXT("same Run begin is idempotent"),
		Session.TryBegin(SwordRhythmSessionRunA, Diagnostic));
	TestFalse(TEXT("second Run is rejected while active"),
		Session.TryBegin(SwordRhythmSessionRunB, Diagnostic));
	TestFalse(TEXT("mismatched teardown is rejected atomically"),
		Session.TryEnd(SwordRhythmSessionRunB, Diagnostic));
	TestTrue(TEXT("matching teardown clears config Host and receipt"),
		Session.TryEnd(SwordRhythmSessionRunA, Diagnostic)
			&& Session.IsEmpty()
			&& Session.NumRecordedObservations() == 0
			&& !Session.GetLastReceipt().IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmProductSessionRealBasicSwordTest,
	"Shanmen.0_0_10.Product.SwordRhythmProductSession.RealBasicSwordLifecycle",
	SwordRhythmSessionFlags)

bool Fdemo_mapSwordRhythmProductSessionRealBasicSwordTest::RunTest(
	const FString&)
{
	FSwordRhythmSessionFixture Fixture(SwordRhythmSessionRunA);
	Fdemo_mapShanmenSwordRhythmProductSession Session;
	TestTrue(TEXT("real product fixture and canonical Session initialize"),
		Fixture.bReady
			&& Session.TryBegin(
				SwordRhythmSessionRunA,
				Fixture.Diagnostic));
	if (!Fixture.bReady || Session.IsEmpty())
	{
		AddError(Fixture.Diagnostic);
		return false;
	}

	Fdemo_mapShanmenCombatRunTimelineSample Sample;
	FShanmenSwordRhythmReceipt Receipt;
	const Fdemo_mapBasicSwordProductExecutionResult NotExecuted;
	Fixture.Timeline.TryCapture(Sample);
	TestFalse(TEXT("non-executed result cannot mutate the Session"),
		Session.TryObserveExecutedBasicSword(
			NotExecuted,
			Sample,
			Receipt,
			Fixture.Diagnostic));
	TestTrue(TEXT("rejection preserves the empty canonical chain"),
		Session.IsValid() && Session.NumRecordedObservations() == 0);

	const Fdemo_mapBasicSwordProductExecutionResult First =
		Fixture.Coordinator.ExecutePlayerBasicSwordSweep(
			SwordRhythmSessionWeapon,
			1.0f,
			{});
	Fixture.Timeline.TryCapture(Sample);
	TestTrue(TEXT("first completed legal miss starts the product rhythm"),
		First.IsExecuted()
			&& !First.AppliedDamage()
			&& Session.TryObserveExecutedBasicSword(
				First,
				Sample,
				Receipt,
				Fixture.Diagnostic)
			&& Receipt.GetBand() == EShanmenSwordRhythmBand::Started
			&& Receipt.GetResultingChainCount() == 1);

	int64 AdvancedTicks = 0;
	const double OpenSeconds =
		static_cast<double>(
			Fdemo_mapShanmenSwordRhythmProductConfig::
				CanonicalLinkOpenOffsetTicks())
		/ static_cast<double>(
			Fdemo_mapShanmenSwordRhythmProductConfig::
				CanonicalTimelineTicksPerSecond());
	TestTrue(TEXT("Run timeline reaches the canonical open boundary"),
		Fixture.Timeline.TryAdvance(
			OpenSeconds,
			AdvancedTicks,
			Fixture.Diagnostic)
			&& AdvancedTicks
				== Fdemo_mapShanmenSwordRhythmProductConfig::
					CanonicalLinkOpenOffsetTicks());

	const Fdemo_mapBasicSwordProductExecutionResult Second =
		Fixture.Coordinator.ExecutePlayerBasicSwordSweep(
			SwordRhythmSessionWeapon,
			1.0f,
			{});
	Fixture.Timeline.TryCapture(Sample);
	FShanmenSwordRhythmReceipt SecondReceipt;
	TestTrue(TEXT("second completed action links at the content boundary"),
		Second.IsExecuted()
			&& Second.ActivationId != First.ActivationId
			&& Session.TryObserveExecutedBasicSword(
				Second,
				Sample,
				SecondReceipt,
				Fixture.Diagnostic)
			&& SecondReceipt.GetBand()
				== EShanmenSwordRhythmBand::PreciseLinked
			&& SecondReceipt.GetResultingChainCount() == 2
			&& !Second.AppliedDamage());
	TestTrue(TEXT("Session exposes the immutable latest receipt"),
		Session.GetLastReceipt().GetReceiptId()
			== SecondReceipt.GetReceiptId()
			&& Session.NumRecordedObservations() == 2);

	FShanmenSwordRhythmReceipt ReplayReceipt;
	TestTrue(TEXT("exact replay remains idempotent through the Session"),
		Session.TryObserveExecutedBasicSword(
			Second,
			Sample,
			ReplayReceipt,
			Fixture.Diagnostic)
			&& ReplayReceipt.GetReceiptId()
				== SecondReceipt.GetReceiptId()
			&& Session.NumRecordedObservations() == 2);
	TestTrue(TEXT("Run teardown clears the complete product Session"),
		Session.TryEnd(SwordRhythmSessionRunA, Fixture.Diagnostic)
			&& Session.IsEmpty());
	return true;
}

#endif
