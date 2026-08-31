#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "demo_mapCombatRunCoordinator.h"
#include "demo_mapPlayerHealthComponent.h"
#include "demo_mapShanmenSwordRhythmEffectCueDelivery.h"
#include "demo_mapShanmenSwordRhythmProductSession.h"

namespace
{
	constexpr EAutomationTestFlags SwordRhythmEffectCueDeliveryFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;
	const FGuid DeliveryRun(
		0xA6F80001, 0xA6F80002, 0xA6F80003, 0xA6F80004);
	const FGuid ForeignRun(
		0xA6F80011, 0xA6F80012, 0xA6F80013, 0xA6F80014);
	const FGuid DeliveryWeapon(
		0xA6F90001, 0xA6F90002, 0xA6F90003, 0xA6F90004);
	const FGuid ForeignWeapon(
		0xA6F90011, 0xA6F90012, 0xA6F90013, 0xA6F90014);
	const FGuid VisualConsumer(
		0xA6FA0001, 0xA6FA0002, 0xA6FA0003, 0xA6FA0004);
	const FGuid AudioConsumer(
		0xA6FB0001, 0xA6FB0002, 0xA6FB0003, 0xA6FB0004);
	const FName VisualRole(TEXT("Presentation.Visual.SwordRhythm"));
	const FName AudioRole(TEXT("Presentation.Audio.SwordRhythm"));

	bool TryMakeScope(
		const FGuid& RunId,
		const FGuid& ConsumerId,
		const FName ConsumerRoleId,
		Fdemo_mapShanmenSwordRhythmEffectCueConsumerScope& OutScope,
		Fdemo_mapShanmenSwordRhythmEffectCueDeliveryCursor& OutCursor)
	{
		return Fdemo_mapShanmenSwordRhythmEffectCueConsumerScope::TryCreate(
				RunId, ConsumerId, ConsumerRoleId, OutScope)
			&& Fdemo_mapShanmenSwordRhythmEffectCueDeliveryCursor::TryCreate(
				OutScope, OutCursor);
	}

	struct FSwordRhythmCueDeliveryFixture
	{
		FGuid RunId;
		FGuid WeaponId;
		APawn* Pawn = nullptr;
		UBoxComponent* CollisionRoot = nullptr;
		Udemo_mapPlayerHealthComponent* Health = nullptr;
		Fdemo_mapCombatRunCoordinator Coordinator;
		Fdemo_mapShanmenCombatRunFixedTimeline Timeline;
		Fdemo_mapShanmenSwordRhythmProductSession Session;
		FString Diagnostic;
		bool bReady = false;

		FSwordRhythmCueDeliveryFixture(
			const FGuid& InRunId,
			const FGuid& InWeaponId)
			: RunId(InRunId)
			, WeaponId(InWeaponId)
		{
			Pawn = NewObject<APawn>(GetTransientPackage());
			CollisionRoot = Pawn
				? NewObject<UBoxComponent>(
					Pawn,
					TEXT("CueDeliveryPlayerCollisionRoot"))
				: nullptr;
			Health = Pawn
				? NewObject<Udemo_mapPlayerHealthComponent>(
					Pawn,
					TEXT("CueDeliveryPlayerHealth"))
				: nullptr;
			if (Pawn && CollisionRoot)
			{
				Pawn->SetRootComponent(CollisionRoot);
			}
			bReady = Pawn && CollisionRoot && Health
				&& Coordinator.TryBeginRun(
					RunId, Pawn, Health, Diagnostic)
				&& Timeline.TryBegin(RunId, Diagnostic)
				&& Session.TryBegin(RunId, Diagnostic);
		}

		bool TryAdvanceTicks(const int64 RequestedTicks)
		{
			int64 AdvancedTicks = 0;
			const double Seconds = static_cast<double>(RequestedTicks)
				/ static_cast<double>(
					Fdemo_mapShanmenCombatRunFixedTimeline::
						CanonicalTicksPerSecond());
			return Timeline.TryAdvance(Seconds, AdvancedTicks, Diagnostic)
				&& AdvancedTicks == RequestedTicks;
		}

		bool TryExecute(
			Fdemo_mapShanmenSwordRhythmEffectCueEvent& OutEvent)
		{
			const auto Action = Coordinator.ExecutePlayerBasicSwordSweep(
				WeaponId,
				1.0f,
				{});
			Fdemo_mapShanmenCombatRunTimelineSample Sample;
			FShanmenSwordRhythmReceipt Receipt;
			if (!Action.IsExecuted()
				|| !Timeline.TryCapture(Sample)
				|| !Session.TryObserveExecutedBasicSword(
					Action, Sample, Receipt, Diagnostic))
			{
				return false;
			}
			const auto Adapted =
				Fdemo_mapShanmenSwordRhythmEffectCueAdapter::Adapt(
					Session.GetPresentationState(),
					Session.GetConfig().GetEffectCuePolicy());
			if (!Adapted.IsAdapted())
			{
				Diagnostic = Adapted.Diagnostic;
				return false;
			}
			OutEvent = Adapted.Event;
			return true;
		}

		bool TryMakeEffectEvent(
			Fdemo_mapShanmenSwordRhythmEffectCueEvent& OutEvent)
		{
			Fdemo_mapShanmenSwordRhythmEffectCueEvent Discarded;
			return TryExecute(Discarded)
				&& TryAdvanceTicks(
					Fdemo_mapShanmenSwordRhythmProductConfig::
						CanonicalLinkOpenOffsetTicks())
				&& TryExecute(Discarded)
				&& TryExecute(OutEvent)
				&& OutEvent.IsValid()
				&& OutEvent.NumCommands() == 2;
		}
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueDeliveryScopeTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueDelivery.ScopeContract",
	SwordRhythmEffectCueDeliveryFlags)

bool Fdemo_mapSwordRhythmCueDeliveryScopeTest::RunTest(const FString&)
{
	Fdemo_mapShanmenSwordRhythmEffectCueConsumerScope First;
	Fdemo_mapShanmenSwordRhythmEffectCueConsumerScope Replay;
	Fdemo_mapShanmenSwordRhythmEffectCueConsumerScope DifferentConsumer;
	Fdemo_mapShanmenSwordRhythmEffectCueConsumerScope DifferentRole;
	Fdemo_mapShanmenSwordRhythmEffectCueConsumerScope DifferentRun;
	TestTrue(TEXT("consumer scope captures deterministic Run identity"),
		Fdemo_mapShanmenSwordRhythmEffectCueConsumerScope::TryCreate(
			DeliveryRun, VisualConsumer, VisualRole, First)
			&& Fdemo_mapShanmenSwordRhythmEffectCueConsumerScope::TryCreate(
				DeliveryRun, VisualConsumer, VisualRole, Replay)
			&& First.Matches(Replay));
	TestTrue(TEXT("Run consumer and role each isolate scope identity"),
		Fdemo_mapShanmenSwordRhythmEffectCueConsumerScope::TryCreate(
				DeliveryRun, AudioConsumer, VisualRole, DifferentConsumer)
			&& Fdemo_mapShanmenSwordRhythmEffectCueConsumerScope::TryCreate(
				DeliveryRun, VisualConsumer, AudioRole, DifferentRole)
			&& Fdemo_mapShanmenSwordRhythmEffectCueConsumerScope::TryCreate(
				ForeignRun, VisualConsumer, VisualRole, DifferentRun)
			&& First.GetScopeId() != DifferentConsumer.GetScopeId()
			&& First.GetScopeId() != DifferentRole.GetScopeId()
			&& First.GetScopeId() != DifferentRun.GetScopeId());
	Fdemo_mapShanmenSwordRhythmEffectCueConsumerScope Rejected;
	TestFalse(TEXT("invalid Run fails closed"),
		Fdemo_mapShanmenSwordRhythmEffectCueConsumerScope::TryCreate(
			FGuid(), VisualConsumer, VisualRole, Rejected));
	TestFalse(TEXT("invalid consumer fails closed"),
		Fdemo_mapShanmenSwordRhythmEffectCueConsumerScope::TryCreate(
			DeliveryRun, FGuid(), VisualRole, Rejected));
	TestFalse(TEXT("missing consumer role fails closed"),
		Fdemo_mapShanmenSwordRhythmEffectCueConsumerScope::TryCreate(
			DeliveryRun, VisualConsumer, NAME_None, Rejected));
	TestFalse(TEXT("failed capture clears output"), Rejected.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueDeliveryIndependentPrepareTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueDelivery.IndependentPrepare",
	SwordRhythmEffectCueDeliveryFlags)

bool Fdemo_mapSwordRhythmCueDeliveryIndependentPrepareTest::RunTest(
	const FString&)
{
	FSwordRhythmCueDeliveryFixture Fixture(DeliveryRun, DeliveryWeapon);
	Fdemo_mapShanmenSwordRhythmEffectCueEvent Event;
	TestTrue(TEXT("real product route creates a source-aware cue event"),
		Fixture.bReady && Fixture.TryMakeEffectEvent(Event));
	if (!Event.IsValid())
	{
		AddError(Fixture.Diagnostic);
		return false;
	}
	Fdemo_mapShanmenSwordRhythmEffectCueConsumerScope VisualScope;
	Fdemo_mapShanmenSwordRhythmEffectCueConsumerScope AudioScope;
	Fdemo_mapShanmenSwordRhythmEffectCueDeliveryCursor VisualCursor;
	Fdemo_mapShanmenSwordRhythmEffectCueDeliveryCursor AudioCursor;
	TestTrue(TEXT("visual and audio consumers own independent cursors"),
		TryMakeScope(
			DeliveryRun,
			VisualConsumer,
			VisualRole,
			VisualScope,
			VisualCursor)
			&& TryMakeScope(
				DeliveryRun,
				AudioConsumer,
				AudioRole,
				AudioScope,
				AudioCursor));

	const auto VisualFirst = VisualCursor.Prepare(Event);
	const auto VisualReplay = VisualCursor.Prepare(Event);
	const auto AudioFirst = AudioCursor.Prepare(Event);
	TestTrue(TEXT("each consumer can prepare the same event independently"),
		VisualFirst.IsPrepared()
			&& VisualReplay.IsPrepared()
			&& AudioFirst.IsPrepared()
			&& VisualFirst.Delivery.GetEvent().Matches(Event)
			&& AudioFirst.Delivery.GetEvent().Matches(Event));
	TestTrue(TEXT("same-consumer preparation is deterministic and retry-safe"),
		VisualFirst.Delivery.Matches(VisualReplay.Delivery)
			&& VisualCursor.IsEmpty());
	TestTrue(TEXT("consumer scope separates delivery identities"),
		VisualFirst.Delivery.GetDeliveryId()
			!= AudioFirst.Delivery.GetDeliveryId()
			&& AudioCursor.IsEmpty());
	TestTrue(TEXT("delivery preserves the immutable typed source commands"),
		VisualFirst.Delivery.GetEvent().NumCommands() == 2
			&& VisualFirst.Delivery.GetEvent().GetCommands()[0].IsValid()
			&& VisualFirst.Delivery.GetEvent().GetCommands()[1].IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueDeliveryAcknowledgeTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueDelivery.AcknowledgeAndOrdering",
	SwordRhythmEffectCueDeliveryFlags)

bool Fdemo_mapSwordRhythmCueDeliveryAcknowledgeTest::RunTest(const FString&)
{
	FSwordRhythmCueDeliveryFixture Fixture(DeliveryRun, DeliveryWeapon);
	Fdemo_mapShanmenSwordRhythmEffectCueEvent FirstEvent;
	TestTrue(TEXT("first effect event is available"),
		Fixture.bReady && Fixture.TryMakeEffectEvent(FirstEvent));
	if (!FirstEvent.IsValid())
	{
		AddError(Fixture.Diagnostic);
		return false;
	}
	Fdemo_mapShanmenSwordRhythmEffectCueConsumerScope Scope;
	Fdemo_mapShanmenSwordRhythmEffectCueDeliveryCursor Cursor;
	TestTrue(TEXT("consumer cursor captures"),
		TryMakeScope(
			DeliveryRun, VisualConsumer, VisualRole, Scope, Cursor));
	const auto Prepared = Cursor.Prepare(FirstEvent);
	const auto Acknowledged = Cursor.Acknowledge(Prepared.Delivery);
	const auto Replay = Cursor.Acknowledge(Prepared.Delivery);
	const auto Already = Cursor.Prepare(FirstEvent);
	TestTrue(TEXT("acknowledgement alone advances the cursor"),
		Prepared.IsPrepared()
			&& Acknowledged.Status
				== Edemo_mapShanmenSwordRhythmEffectCueAcknowledgeStatus::
					Acknowledged
			&& !Cursor.IsEmpty());
	TestTrue(TEXT("exact acknowledgement replay preserves identity"),
		Replay.IsAcknowledged()
			&& Replay.Status
				== Edemo_mapShanmenSwordRhythmEffectCueAcknowledgeStatus::
					AcknowledgementReplayed
			&& Replay.Acknowledgement.Matches(
				Acknowledged.Acknowledgement));
	TestTrue(TEXT("acknowledged event is not prepared again"),
		Already.Status
			== Edemo_mapShanmenSwordRhythmEffectCuePrepareStatus::
				AlreadyAcknowledged
			&& Already.Delivery.Matches(Prepared.Delivery));

	Fdemo_mapShanmenSwordRhythmEffectCueEvent NextEvent;
	TestTrue(TEXT("newer source observation creates a next event"),
		Fixture.TryAdvanceTicks(1)
			&& Fixture.TryExecute(NextEvent)
			&& NextEvent.GetState().GetObservationRevision()
				> FirstEvent.GetState().GetObservationRevision());
	const auto NextPrepared = Cursor.Prepare(NextEvent);
	const auto NextAcknowledged = Cursor.Acknowledge(NextPrepared.Delivery);
	const auto StalePrepare = Cursor.Prepare(FirstEvent);
	const auto StaleAcknowledge = Cursor.Acknowledge(Prepared.Delivery);
	TestTrue(TEXT("strictly newer observation advances"),
		NextPrepared.IsPrepared() && NextAcknowledged.IsAcknowledged());
	TestTrue(TEXT("older observation and delivery fail closed"),
		StalePrepare.Status
				== Edemo_mapShanmenSwordRhythmEffectCuePrepareStatus::StaleEvent
			&& StaleAcknowledge.Status
				== Edemo_mapShanmenSwordRhythmEffectCueAcknowledgeStatus::
					StaleDelivery);
	Fdemo_mapShanmenSwordRhythmEffectCueAcknowledgementReceipt Last;
	TestTrue(TEXT("cursor exposes only its latest immutable acknowledgement"),
		Cursor.TryGetLastAcknowledgement(Last)
			&& Last.Matches(NextAcknowledged.Acknowledgement));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueDeliveryFenceAndTeardownTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueDelivery.FenceAndTeardown",
	SwordRhythmEffectCueDeliveryFlags)

bool Fdemo_mapSwordRhythmCueDeliveryFenceAndTeardownTest::RunTest(
	const FString&)
{
	FSwordRhythmCueDeliveryFixture Fixture(DeliveryRun, DeliveryWeapon);
	FSwordRhythmCueDeliveryFixture Foreign(ForeignRun, ForeignWeapon);
	Fdemo_mapShanmenSwordRhythmEffectCueEvent Event;
	Fdemo_mapShanmenSwordRhythmEffectCueEvent ForeignEvent;
	TestTrue(TEXT("both real Run fixtures create cue events"),
		Fixture.bReady
			&& Foreign.bReady
			&& Fixture.TryMakeEffectEvent(Event)
			&& Foreign.TryMakeEffectEvent(ForeignEvent));
	if (!Event.IsValid() || !ForeignEvent.IsValid())
	{
		AddError(Fixture.Diagnostic + TEXT(" | ") + Foreign.Diagnostic);
		return false;
	}
	Fdemo_mapShanmenSwordRhythmEffectCueConsumerScope VisualScope;
	Fdemo_mapShanmenSwordRhythmEffectCueConsumerScope AudioScope;
	Fdemo_mapShanmenSwordRhythmEffectCueDeliveryCursor VisualCursor;
	Fdemo_mapShanmenSwordRhythmEffectCueDeliveryCursor AudioCursor;
	TestTrue(TEXT("two scoped cursors capture"),
		TryMakeScope(
			DeliveryRun,
			VisualConsumer,
			VisualRole,
			VisualScope,
			VisualCursor)
			&& TryMakeScope(
				DeliveryRun,
				AudioConsumer,
				AudioRole,
				AudioScope,
				AudioCursor));
	const auto VisualDelivery = VisualCursor.Prepare(Event);
	const auto AudioDelivery = AudioCursor.Prepare(Event);
	const auto CrossConsumer =
		VisualCursor.Acknowledge(AudioDelivery.Delivery);
	const auto CrossRun = VisualCursor.Prepare(ForeignEvent);
	const auto Invalid = VisualCursor.Acknowledge(
		Fdemo_mapShanmenSwordRhythmEffectCueDeliveryReceipt());
	TestTrue(TEXT("cross-consumer acknowledgement fails closed"),
		CrossConsumer.Status
			== Edemo_mapShanmenSwordRhythmEffectCueAcknowledgeStatus::
				ScopeMismatch);
	TestTrue(TEXT("cross-Run preparation fails closed"),
		CrossRun.Status
			== Edemo_mapShanmenSwordRhythmEffectCuePrepareStatus::
				RunMismatch);
	TestTrue(TEXT("invalid delivery fails closed without cursor mutation"),
		Invalid.Status
				== Edemo_mapShanmenSwordRhythmEffectCueAcknowledgeStatus::
					DeliveryInvalid
			&& VisualCursor.IsEmpty());

	const auto Accepted = VisualCursor.Acknowledge(VisualDelivery.Delivery);
	const auto StableDelivery = VisualDelivery.Delivery;
	const auto StableAcknowledgement = Accepted.Acknowledgement;
	TestTrue(TEXT("own exact delivery remains accepted"),
		Accepted.IsAcknowledged());
	TestTrue(TEXT("source Session teardown succeeds"),
		Fixture.Session.TryEnd(DeliveryRun, Fixture.Diagnostic)
			&& Fixture.Session.IsEmpty());
	TestTrue(TEXT("immutable delivery acknowledgement and cursor survive teardown"),
		StableDelivery.IsValid()
			&& StableAcknowledgement.IsValid()
			&& VisualCursor.IsValid());
	TestFalse(TEXT("empty source Session cannot recreate a cue event"),
		Fdemo_mapShanmenSwordRhythmEffectCueAdapter::Adapt(
			Fixture.Session.GetPresentationState(),
			Fixture.Session.GetConfig().GetEffectCuePolicy()).IsAdapted());
	return true;
}

#endif
