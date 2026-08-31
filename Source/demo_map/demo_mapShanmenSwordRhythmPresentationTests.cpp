#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "demo_mapCombatRunCoordinator.h"
#include "demo_mapGameMode.h"
#include "demo_mapPlayerHealthComponent.h"
#include "demo_mapShanmenSwordRhythmPresentation.h"
#include "demo_mapShanmenSwordRhythmProductSession.h"

namespace
{
	constexpr EAutomationTestFlags SwordRhythmPresentationFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;
	const FGuid PresentationRunA(
		0xA6F10001, 0xA6F10002, 0xA6F10003, 0xA6F10004);
	const FGuid PresentationRunB(
		0xA6F20001, 0xA6F20002, 0xA6F20003, 0xA6F20004);
	const FGuid PresentationWeapon(
		0xA6F30001, 0xA6F30002, 0xA6F30003, 0xA6F30004);

	struct FSwordRhythmPresentationFixture
	{
		APawn* Pawn = nullptr;
		UBoxComponent* CollisionRoot = nullptr;
		Udemo_mapPlayerHealthComponent* Health = nullptr;
		Fdemo_mapCombatRunCoordinator Coordinator;
		Fdemo_mapShanmenCombatRunFixedTimeline Timeline;
		FString Diagnostic;
		bool bReady = false;

		explicit FSwordRhythmPresentationFixture(const FGuid& RunId)
		{
			Pawn = NewObject<APawn>(GetTransientPackage());
			CollisionRoot = Pawn
				? NewObject<UBoxComponent>(
					Pawn,
					TEXT("PresentationPlayerCollisionRoot"))
				: nullptr;
			Health = Pawn
				? NewObject<Udemo_mapPlayerHealthComponent>(
					Pawn,
					TEXT("PresentationPlayerHealth"))
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
	Fdemo_mapSwordRhythmPresentationProjectionTest,
	"Shanmen.0_0_10.Product.SwordRhythmPresentation.ProjectionAndIdentityFence",
	SwordRhythmPresentationFlags)

bool Fdemo_mapSwordRhythmPresentationProjectionTest::RunTest(
	const FString&)
{
	FSwordRhythmPresentationFixture Fixture(PresentationRunA);
	Fdemo_mapShanmenSwordRhythmProductSession Session;
	TestTrue(TEXT("real product fixture and Session initialize"),
		Fixture.bReady
			&& Session.TryBegin(PresentationRunA, Fixture.Diagnostic));
	if (!Fixture.bReady || Session.IsEmpty())
	{
		AddError(Fixture.Diagnostic);
		return false;
	}

	const Fdemo_mapBasicSwordProductExecutionResult First =
		Fixture.Coordinator.ExecutePlayerBasicSwordSweep(
			PresentationWeapon,
			1.0f,
			{});
	Fdemo_mapShanmenCombatRunTimelineSample Sample;
	FShanmenSwordRhythmReceipt Receipt;
	const bool bCaptured = Fixture.Timeline.TryCapture(Sample);
	const bool bObserved = Session.TryObserveExecutedBasicSword(
		First,
		Sample,
		Receipt,
		Fixture.Diagnostic);
	TestTrue(TEXT("completed BasicSword produces one immutable source receipt"),
		First.IsExecuted() && !First.AppliedDamage()
			&& bCaptured && bObserved && Receipt.IsValid());

	const auto Projection =
		Fdemo_mapShanmenSwordRhythmPresentationProjector::Project(
			Session.GetConfig(),
			PresentationRunA,
			Receipt,
			Session.GetLastEvaluationReceipt(),
			1);
	const auto Reprojection =
		Fdemo_mapShanmenSwordRhythmPresentationProjector::Project(
			Session.GetConfig(),
			PresentationRunA,
			Receipt,
			Session.GetLastEvaluationReceipt(),
			1);
	TestTrue(TEXT("same receipt and revision derive one presentation identity"),
		Projection.IsProjected()
			&& Reprojection.IsProjected()
			&& Projection.State.Matches(Reprojection.State)
			&& Projection.State.Matches(Session.GetPresentationState()));
	TestTrue(TEXT("read model freezes the complete first-action display scope"),
		Projection.State.GetRunId() == PresentationRunA
			&& Projection.State.GetConfigId()
				== Session.GetConfig().GetConfigId()
			&& Projection.State.GetContentVersion()
				== Fdemo_mapShanmenSwordRhythmProductConfig::
					CanonicalContentVersion()
			&& Projection.State.GetReceiptId() == Receipt.GetReceiptId()
			&& Projection.State.GetEvaluationReceiptId()
				== Session.GetLastEvaluationReceipt().GetReceiptId()
			&& Projection.State.GetEvaluationPolicyId()
				== Session.GetConfig().GetEvaluationPolicy().GetPolicyId()
			&& Projection.State.NumEffectDefinitions() == 0
			&& Projection.State.GetActivationId() == First.ActivationId
			&& Projection.State.GetTimelineId() == Sample.GetTimelineId()
			&& Projection.State.GetLinkOpenOffsetTicks() == 8
			&& Projection.State.GetLinkCloseOffsetTicks() == 13
			&& Projection.State.GetTimelineTicksPerSecond() == 30
			&& Projection.State.GetPreviousInputTick() == INDEX_NONE
			&& Projection.State.GetCurrentInputTick() == 0
			&& Projection.State.GetObservationRevision() == 1
			&& Projection.State.GetPreviousChainCount() == 0
			&& Projection.State.GetResultingChainCount() == 1
			&& Projection.State.GetBand()
				== EShanmenSwordRhythmBand::Started);

	const auto BadConfig =
		Fdemo_mapShanmenSwordRhythmPresentationProjector::Project(
			Fdemo_mapShanmenSwordRhythmProductConfig(),
			PresentationRunA,
			Receipt,
			Session.GetLastEvaluationReceipt(),
			1);
	const auto BadRun =
		Fdemo_mapShanmenSwordRhythmPresentationProjector::Project(
			Session.GetConfig(),
			FGuid(),
			Receipt,
			Session.GetLastEvaluationReceipt(),
			1);
	const auto BadReceipt =
		Fdemo_mapShanmenSwordRhythmPresentationProjector::Project(
			Session.GetConfig(),
			PresentationRunA,
			FShanmenSwordRhythmReceipt(),
			Session.GetLastEvaluationReceipt(),
			1);
	const auto BadEvaluationReceipt =
		Fdemo_mapShanmenSwordRhythmPresentationProjector::Project(
			Session.GetConfig(),
			PresentationRunA,
			Receipt,
			FShanmenSwordRhythmEvaluationReceipt(),
			1);
	const auto BadRevision =
		Fdemo_mapShanmenSwordRhythmPresentationProjector::Project(
			Session.GetConfig(),
			PresentationRunA,
			Receipt,
			Session.GetLastEvaluationReceipt(),
			0);
	const auto ForeignRun =
		Fdemo_mapShanmenSwordRhythmPresentationProjector::Project(
			Session.GetConfig(),
			PresentationRunB,
			Receipt,
			Session.GetLastEvaluationReceipt(),
			1);
	TestTrue(TEXT("malformed and cross-Run projections fail with typed status"),
		!BadConfig.IsProjected()
			&& BadConfig.Status
				== Edemo_mapShanmenSwordRhythmPresentationProjectionStatus::
					ConfigInvalid
			&& !BadRun.IsProjected()
			&& BadRun.Status
				== Edemo_mapShanmenSwordRhythmPresentationProjectionStatus::
					RunInvalid
			&& !BadReceipt.IsProjected()
			&& BadReceipt.Status
				== Edemo_mapShanmenSwordRhythmPresentationProjectionStatus::
					ReceiptInvalid
			&& !BadEvaluationReceipt.IsProjected()
			&& BadEvaluationReceipt.Status
				== Edemo_mapShanmenSwordRhythmPresentationProjectionStatus::
					EvaluationReceiptInvalid
			&& !BadRevision.IsProjected()
			&& BadRevision.Status
				== Edemo_mapShanmenSwordRhythmPresentationProjectionStatus::
					RevisionInvalid
			&& !ForeignRun.IsProjected()
			&& ForeignRun.Status
				== Edemo_mapShanmenSwordRhythmPresentationProjectionStatus::
					IdentityMismatch);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmPresentationLifecycleTest,
	"Shanmen.0_0_10.Product.SwordRhythmPresentation.SessionLifecycleReadModel",
	SwordRhythmPresentationFlags)

bool Fdemo_mapSwordRhythmPresentationLifecycleTest::RunTest(
	const FString&)
{
	FSwordRhythmPresentationFixture Fixture(PresentationRunA);
	Fdemo_mapShanmenSwordRhythmProductSession Session;
	TestTrue(TEXT("active Session has no display state before its first action"),
		Fixture.bReady
			&& Session.TryBegin(PresentationRunA, Fixture.Diagnostic)
			&& !Session.GetPresentationState().IsValid());

	const auto First = Fixture.Coordinator.ExecutePlayerBasicSwordSweep(
		PresentationWeapon,
		1.0f,
		{});
	Fdemo_mapShanmenCombatRunTimelineSample FirstSample;
	Fixture.Timeline.TryCapture(FirstSample);
	FShanmenSwordRhythmReceipt FirstReceipt;
	TestTrue(TEXT("first observation atomically commits receipt and read model"),
		Session.TryObserveExecutedBasicSword(
			First,
			FirstSample,
			FirstReceipt,
			Fixture.Diagnostic)
			&& Session.GetLastEvaluationReceipt().IsValid()
			&& Session.GetLastEvaluationReceipt().NumEffects() == 0
			&& Session.GetPresentationState().IsValid());
	const Fdemo_mapShanmenSwordRhythmPresentationState FirstState =
		Session.GetPresentationState();

	FShanmenSwordRhythmReceipt ReplayReceipt;
	TestTrue(TEXT("exact replay preserves presentation identity and revision"),
		Session.TryObserveExecutedBasicSword(
			First,
			FirstSample,
			ReplayReceipt,
			Fixture.Diagnostic)
			&& ReplayReceipt.GetReceiptId() == FirstReceipt.GetReceiptId()
			&& Session.GetPresentationState().Matches(FirstState)
			&& Session.GetPresentationState().GetObservationRevision() == 1
			&& Session.NumRecordedObservations() == 1);

	int64 AdvancedTicks = 0;
	const double OpenSeconds =
		static_cast<double>(
			Fdemo_mapShanmenSwordRhythmProductConfig::
				CanonicalLinkOpenOffsetTicks())
		/ static_cast<double>(
			Fdemo_mapShanmenSwordRhythmProductConfig::
				CanonicalTimelineTicksPerSecond());
	TestTrue(TEXT("canonical Run timeline reaches precise-link boundary"),
		Fixture.Timeline.TryAdvance(
			OpenSeconds,
			AdvancedTicks,
			Fixture.Diagnostic)
			&& AdvancedTicks == 8);

	const auto Second = Fixture.Coordinator.ExecutePlayerBasicSwordSweep(
		PresentationWeapon,
		1.0f,
		{});
	Fdemo_mapShanmenCombatRunTimelineSample SecondSample;
	Fixture.Timeline.TryCapture(SecondSample);
	FShanmenSwordRhythmReceipt SecondReceipt;
	TestTrue(TEXT("second observation advances the read model without damage"),
		Second.IsExecuted() && !Second.AppliedDamage()
			&& Session.TryObserveExecutedBasicSword(
				Second,
				SecondSample,
				SecondReceipt,
				Fixture.Diagnostic));
	const auto& SecondState = Session.GetPresentationState();
	TestTrue(TEXT("latest state exposes precise transition and monotonic revision"),
		SecondState.IsValid()
			&& !SecondState.Matches(FirstState)
			&& SecondState.GetReceiptId() == SecondReceipt.GetReceiptId()
			&& SecondState.GetEvaluationReceiptId()
				== Session.GetLastEvaluationReceipt().GetReceiptId()
			&& SecondState.GetEvaluationPolicyId()
				== Session.GetConfig().GetEvaluationPolicy().GetPolicyId()
			&& SecondState.NumEffectDefinitions() == 0
			&& SecondState.GetPreviousInputTick() == 0
			&& SecondState.GetCurrentInputTick() == 8
			&& SecondState.GetObservationRevision() == 2
			&& SecondState.GetPreviousChainCount() == 1
			&& SecondState.GetResultingChainCount() == 2
			&& SecondState.GetBand()
				== EShanmenSwordRhythmBand::PreciseLinked);

	Ademo_mapGameMode* EmptyGameMode =
		NewObject<Ademo_mapGameMode>(GetTransientPackage());
	TestNotNull(TEXT("GameMode read-model fixture is available"), EmptyGameMode);
	Fdemo_mapShanmenSwordRhythmPresentationState EmptyState;
	const bool bEmptyStateAvailable = EmptyGameMode
		? EmptyGameMode->TryGetSwordRhythmPresentationState(EmptyState)
		: false;
	TestFalse(TEXT("GameMode query fails closed before product state exists"),
		bEmptyStateAvailable);
	TestTrue(TEXT("failed GameMode query returns a canonical empty value"),
		!EmptyState.IsValid());
	TestTrue(TEXT("Run teardown removes presentation state with authority"),
		Session.TryEnd(PresentationRunA, Fixture.Diagnostic)
			&& Session.IsEmpty()
			&& !Session.GetLastEvaluationReceipt().IsValid()
			&& !Session.GetPresentationState().IsValid());
	return true;
}

#endif
