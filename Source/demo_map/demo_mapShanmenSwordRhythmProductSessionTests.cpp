#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "ShanmenCombatResolver.h"
#include "ShanmenCombatTags.h"
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

	FShanmenCombatActionSnapshot MakeContributionAction(
		const FGuid& RunId,
		const FGuid& OwnerId,
		FName ActionDefinitionId,
		uint64 Sequence,
		bool bUsesItem)
	{
		FShanmenCombatActionCapture Capture;
		Capture.RunId = RunId;
		Capture.OwnerId = OwnerId;
		Capture.SourceEntityId = OwnerId;
		Capture.SourceItemInstanceId = bUsesItem
			? SwordRhythmSessionWeapon
			: FGuid();
		Capture.ActionDefinitionId = ActionDefinitionId;
		Capture.Content.Version = TEXT("0.0.10.P12.7");
		Capture.Content.Digest =
			TEXT("TEST-SWORD-RHYTHM-PRODUCT-CONTRIBUTION-ROUTE");
		Capture.SourceTags.AddTag(
			FShanmenCombatNativeTags::SourcePlayer());
		Capture.ActivationId = FShanmenCombatIdFactory::MakeActivationId(
			Capture.RunId,
			Capture.SourceEntityId,
			Capture.ActionDefinitionId,
			Sequence);
		FShanmenCombatActionSnapshot Action;
		check(FShanmenCombatActionSnapshot::TryCapture(Capture, Action));
		return Action;
	}

	FShanmenWeaponGuardTimingProjectionReceipt MakeGuardProjection(
		const FGuid& RunId,
		const FGuid& OwnerId,
		int64 ObservedTick)
	{
		const FShanmenCombatActionSnapshot Action =
			MakeContributionAction(
				RunId,
				OwnerId,
				FShanmenWeaponGuardDefinition::
					CanonicalActionDefinitionId(),
				1270,
				true);
		FShanmenWeaponGuardDefinitionCapture DefinitionCapture;
		DefinitionCapture.ActionDefinitionId =
			FShanmenWeaponGuardDefinition::CanonicalActionDefinitionId();
		DefinitionCapture.RuleId =
			TEXT("Defense.Sword.WeaponGuard.ProductRoute.r1");
		DefinitionCapture.GuardFraction = 0.25f;
		FShanmenWeaponGuardDefinition Definition;
		check(FShanmenWeaponGuardDefinition::TryCapture(
			DefinitionCapture,
			Definition));

		FShanmenActionOrchestrator Runtime;
		FShanmenActionTransitionReceipt Transition;
		check(FShanmenActionOrchestrator::TryStart(
			Action,
			Runtime,
			Transition));
		check(Runtime.TryAdvance(
			EShanmenCombatActionPhase::Startup,
			Transition));
		FShanmenWeaponGuardWindow Window;
		FShanmenWeaponGuardWindowReceipt OpenReceipt;
		check(FShanmenWeaponGuardWindow::TryOpen(
			Action,
			Definition,
			Transition,
			Runtime,
			Window,
			OpenReceipt));

		const FGuid TimelineId =
			Fdemo_mapShanmenCombatRunFixedTimeline::MakeTimelineId(RunId);
		FShanmenWeaponPerfectGuardPolicy Policy;
		check(FShanmenWeaponPerfectGuardPolicy::TryCapture(
			OpenReceipt,
			TimelineId,
			0,
			2,
			TEXT("Defense.Sword.PerfectGuard.ProductRoute.r1"),
			Policy));
		FShanmenWeaponGuardTimelineObservation Observation;
		check(FShanmenWeaponGuardTimelineObservation::TryCapture(
			TimelineId,
			ObservedTick,
			Observation));
		FShanmenWeaponGuardTimingProjectionReceipt Projection;
		check(FShanmenWeaponGuardTimingEvaluator::TryProject(
			Window,
			Runtime,
			Policy,
			Observation,
			Projection));
		return Projection;
	}

	FShanmenSpiritEvasionProjectionReceipt MakeEvasionProjection(
		const FGuid& RunId,
		const FGuid& OwnerId)
	{
		const FShanmenCombatActionSnapshot Action =
			MakeContributionAction(
				RunId,
				OwnerId,
				FShanmenSpiritEvasionDefinition::
					CanonicalActionDefinitionId(),
				1280,
				false);
		FShanmenSpiritEvasionDefinitionCapture DefinitionCapture;
		DefinitionCapture.ActionDefinitionId =
			FShanmenSpiritEvasionDefinition::
				CanonicalActionDefinitionId();
		DefinitionCapture.RuleId =
			TEXT("Defense.Spell.SpiritEvasion.ProductRoute.r1");
		FShanmenSpiritEvasionDefinition Definition;
		check(FShanmenSpiritEvasionDefinition::TryCapture(
			DefinitionCapture,
			Definition));

		FShanmenActionOrchestrator Runtime;
		FShanmenActionTransitionReceipt Transition;
		check(FShanmenActionOrchestrator::TryStart(
			Action,
			Runtime,
			Transition));
		check(Runtime.TryAdvance(
			EShanmenCombatActionPhase::Startup,
			Transition));
		FShanmenSpiritEvasionWindow Window;
		FShanmenSpiritEvasionWindowReceipt OpenReceipt;
		check(FShanmenSpiritEvasionWindow::TryOpen(
			Action,
			Definition,
			Transition,
			Runtime,
			Window,
			OpenReceipt));
		FShanmenSpiritEvasionProjectionReceipt Projection;
		check(Window.TryProjectDefenseLayer(Runtime, Projection));
		return Projection;
	}
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

	const FGuid PlayerEntityId =
		Fixture.Coordinator.GetPlayerEntityId();
	const FShanmenWeaponGuardTimingProjectionReceipt PerfectGuard =
		MakeGuardProjection(
			SwordRhythmSessionRunA,
			PlayerEntityId,
			0);
	const FShanmenSpiritEvasionProjectionReceipt SpiritEvasion =
		MakeEvasionProjection(
			SwordRhythmSessionRunA,
			PlayerEntityId);
	FShanmenSwordRhythmContribution GuardContribution;
	FShanmenSwordRhythmContribution EvasionContribution;
	TestTrue(TEXT("real perfect guard and evasion receipts enter the sole product ledger"),
		Session.TryRecordPerfectWeaponGuardContribution(
			PerfectGuard,
			GuardContribution,
			Fixture.Diagnostic)
			&& Session.TryRecordSpiritEvasionContribution(
				SpiritEvasion,
				Sample,
				EvasionContribution,
				Fixture.Diagnostic)
			&& GuardContribution.IsValid()
			&& EvasionContribution.IsValid()
			&& Session.GetContributionBindingLedger().IsValid()
			&& Session.GetContributionBindingLedger().GetScope()
				.GetOwnerId() == PlayerEntityId
			&& Session.GetContributionBindingLedger().NumPending() == 2
			&& Session.GetContributionBindingLedger()
				.NumObservedActions() == 0);
	FShanmenSwordRhythmContribution ReplayGuardContribution;
	TestTrue(TEXT("exact source replay is idempotent before any sword action"),
		Session.TryRecordPerfectWeaponGuardContribution(
			PerfectGuard,
			ReplayGuardContribution,
			Fixture.Diagnostic)
			&& ReplayGuardContribution.GetContributionId()
				== GuardContribution.GetContributionId()
			&& Session.GetContributionBindingLedger().NumPending() == 2);
	const FShanmenWeaponGuardTimingProjectionReceipt OrdinaryGuard =
		MakeGuardProjection(
			SwordRhythmSessionRunA,
			PlayerEntityId,
			2);
	FShanmenSwordRhythmContribution RejectedContribution;
	TestFalse(TEXT("ordinary guard cannot enter the perfect-guard route"),
		Session.TryRecordPerfectWeaponGuardContribution(
			OrdinaryGuard,
			RejectedContribution,
			Fixture.Diagnostic));
	TestTrue(TEXT("rejected source is output-clearing and atomic"),
		!RejectedContribution.IsValid()
			&& Session.GetContributionBindingLedger().NumPending() == 2);
	const FShanmenSpiritEvasionProjectionReceipt ForeignEvasion =
		MakeEvasionProjection(
			SwordRhythmSessionRunB,
			PlayerEntityId);
	TestFalse(TEXT("foreign-Run evidence fails closed without changing the scope"),
		Session.TryRecordSpiritEvasionContribution(
			ForeignEvasion,
			Sample,
			RejectedContribution,
			Fixture.Diagnostic));
	TestTrue(TEXT("foreign rejection preserves both pending contributions"),
		!RejectedContribution.IsValid()
			&& Session.GetContributionBindingLedger().NumPending() == 2);

	const Fdemo_mapBasicSwordProductExecutionResult First =
		Fixture.Coordinator.ExecutePlayerBasicSwordSweep(
			SwordRhythmSessionWeapon,
			1.0f,
			{});
	Fixture.Timeline.TryCapture(Sample);
	FShanmenSwordRhythmContributionBindingReceipt FirstBinding;
	FShanmenSwordRhythmEvaluationInput FirstEvaluation;
	TestTrue(TEXT("first completed legal miss starts the product rhythm"),
		First.IsExecuted()
			&& !First.AppliedDamage()
			&& Session.TryObserveExecutedBasicSword(
				First,
				Sample,
				Receipt,
				FirstBinding,
				FirstEvaluation,
				Fixture.Diagnostic)
			&& Receipt.GetBand() == EShanmenSwordRhythmBand::Started
			&& Receipt.GetResultingChainCount() == 1);
	TestTrue(TEXT("first sword atomically binds both earlier defensive facts"),
		FirstBinding.IsValid()
			&& FirstEvaluation.IsValid()
			&& FirstEvaluation.HasContributionBinding()
			&& FirstEvaluation.NumContributions() == 2
			&& FirstEvaluation.GetContributionBinding().GetReceiptId()
				== FirstBinding.GetReceiptId()
			&& Session.GetLastEvaluationInput().GetInputId()
				== FirstEvaluation.GetInputId()
			&& FirstBinding.NumContributions() == 2
			&& FirstBinding.GetTargetObservation().GetAction()
				.GetActivationId() == First.ActivationId
			&& Session.GetContributionBindingLedger().NumPending() == 0
			&& Session.GetContributionBindingLedger()
				.NumBoundContributions() == 2);

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
	FShanmenSwordRhythmContributionBindingReceipt SecondBinding;
	FShanmenSwordRhythmEvaluationInput SecondEvaluation;
	TestTrue(TEXT("second completed action links at the content boundary"),
		Second.IsExecuted()
			&& Second.ActivationId != First.ActivationId
			&& Session.TryObserveExecutedBasicSword(
				Second,
				Sample,
				SecondReceipt,
				SecondBinding,
				SecondEvaluation,
				Fixture.Diagnostic)
			&& SecondReceipt.GetBand()
				== EShanmenSwordRhythmBand::PreciseLinked
			&& SecondReceipt.GetResultingChainCount() == 2
			&& !Second.AppliedDamage());
	TestTrue(TEXT("Session exposes the immutable latest receipt"),
		Session.GetLastReceipt().GetReceiptId()
			== SecondReceipt.GetReceiptId()
			&& SecondEvaluation.IsValid()
			&& !SecondEvaluation.HasContributionBinding()
			&& SecondEvaluation.NumContributions() == 0
			&& Session.GetLastEvaluationInput().GetInputId()
				== SecondEvaluation.GetInputId()
			&& Session.NumRecordedObservations() == 2
			&& !SecondBinding.IsValid()
			&& Session.GetContributionBindingLedger().NumPending() == 1);

	FShanmenSwordRhythmReceipt ReplayReceipt;
	FShanmenSwordRhythmContributionBindingReceipt ReplayBinding;
	FShanmenSwordRhythmEvaluationInput ReplayEvaluation;
	TestTrue(TEXT("exact replay remains idempotent through the Session"),
		Session.TryObserveExecutedBasicSword(
			Second,
			Sample,
			ReplayReceipt,
			ReplayBinding,
			ReplayEvaluation,
			Fixture.Diagnostic)
			&& ReplayReceipt.GetReceiptId()
				== SecondReceipt.GetReceiptId()
			&& !ReplayBinding.IsValid()
			&& ReplayEvaluation.GetInputId()
				== SecondEvaluation.GetInputId()
			&& Session.NumRecordedObservations() == 2
			&& Session.GetContributionBindingLedger().NumPending() == 1);
	FShanmenSwordRhythmEvaluationInput MismatchedEvaluation;
	TestFalse(TEXT("binding from another target cannot be handed to an evaluator"),
		FShanmenSwordRhythmEvaluationInput::TryCapture(
			SecondReceipt,
			FirstBinding,
			MismatchedEvaluation));
	TestFalse(TEXT("mismatched evaluator handoff clears its output"),
		MismatchedEvaluation.IsValid());

	const Fdemo_mapBasicSwordProductExecutionResult Third =
		Fixture.Coordinator.ExecutePlayerBasicSwordSweep(
			SwordRhythmSessionWeapon,
			1.0f,
			{});
	FShanmenSwordRhythmReceipt ThirdReceipt;
	FShanmenSwordRhythmContributionBindingReceipt ThirdBinding;
	FShanmenSwordRhythmEvaluationInput ThirdEvaluation;
	TestTrue(TEXT("the later sword consumes the precise-link evidence, never its source action"),
		Third.IsExecuted()
			&& Session.TryObserveExecutedBasicSword(
				Third,
				Sample,
				ThirdReceipt,
				ThirdBinding,
				ThirdEvaluation,
				Fixture.Diagnostic)
			&& ThirdBinding.IsValid()
			&& ThirdEvaluation.IsValid()
			&& ThirdEvaluation.NumContributions() == 1
			&& ThirdEvaluation.GetContributionBinding().GetReceiptId()
				== ThirdBinding.GetReceiptId()
			&& ThirdBinding.NumContributions() == 1
			&& ThirdBinding.GetContributions()[0].GetKind()
				== EShanmenSwordRhythmContributionKind::PreciseSwordLink
			&& ThirdBinding.GetContributions()[0].GetAction()
				.GetActivationId() == Second.ActivationId
			&& ThirdBinding.GetTargetObservation().GetAction()
				.GetActivationId() == Third.ActivationId
			&& Session.GetContributionBindingLedger().NumPending() == 0
			&& Session.GetContributionBindingLedger()
				.NumBoundContributions() == 3);
	FShanmenSwordRhythmReceipt ThirdReplayReceipt;
	FShanmenSwordRhythmContributionBindingReceipt ThirdReplayBinding;
	FShanmenSwordRhythmEvaluationInput ThirdReplayEvaluation;
	TestTrue(TEXT("bound target replay returns the same immutable binding receipt"),
		Session.TryObserveExecutedBasicSword(
			Third,
			Sample,
			ThirdReplayReceipt,
			ThirdReplayBinding,
			ThirdReplayEvaluation,
			Fixture.Diagnostic)
			&& ThirdReplayReceipt.GetReceiptId()
				== ThirdReceipt.GetReceiptId()
			&& ThirdReplayBinding.GetReceiptId()
				== ThirdBinding.GetReceiptId()
			&& ThirdReplayEvaluation.GetInputId()
				== ThirdEvaluation.GetInputId()
			&& Session.GetContributionBindingLedger()
				.NumBoundContributions() == 3);
	TestTrue(TEXT("Run teardown clears the complete product Session"),
		Session.TryEnd(SwordRhythmSessionRunA, Fixture.Diagnostic)
			&& Session.IsEmpty()
			&& !Session.GetLastEvaluationInput().IsValid()
			&& !Session.GetContributionBindingLedger().IsValid());
	return true;
}

#endif
