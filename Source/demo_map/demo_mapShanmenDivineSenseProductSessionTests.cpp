#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenDivineSenseProductSession.h"

#include "Components/SceneComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Misc/AutomationTest.h"
#include "ShanmenCombatResolver.h"
#include "ShanmenCombatRuntimeTags.h"
#include "ShanmenCombatTags.h"
#include "demo_mapCombatRunCoordinator.h"
#include "demo_mapPlayerHealthComponent.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	const FGuid SessionRunId(0xD5500001, 0, 0, 1);
	const FGuid OtherSessionRunId(0xD5500002, 0, 0, 1);
	const FGuid ForeignOwnerId(0xD5500003, 0, 0, 1);

	struct FDivineSenseSessionFixture
	{
		UWorld* World = nullptr;
		APawn* Player = nullptr;
		Udemo_mapPlayerHealthComponent* Health = nullptr;
		Fdemo_mapCombatRunCoordinator Coordinator;
		FString Diagnostic;
		FGuid RunId;

		bool Start(const FGuid& InRunId)
		{
			RunId = InRunId;
			if (!GEngine || !RunId.IsValid())
			{
				return false;
			}
			World = NewObject<UWorld>(
				GetTransientPackage(), NAME_None, RF_Transient);
			if (!World)
			{
				return false;
			}
			World->WorldType = EWorldType::GamePreview;
			FWorldContext& Context =
				GEngine->CreateNewWorldContext(EWorldType::GamePreview);
			Context.SetCurrentWorld(World);
			World->InitializeNewWorld(
				UWorld::InitializationValues()
					.InitializeScenes(false)
					.AllowAudioPlayback(false)
					.RequiresHitProxies(false)
					.CreatePhysicsScene(false)
					.CreateNavigation(false)
					.CreateAISystem(false)
					.ShouldSimulatePhysics(false)
					.EnableTraceCollision(false)
					.SetTransactional(false)
					.CreateFXSystem(false));

			FActorSpawnParameters Parameters;
			Parameters.ObjectFlags |= RF_Transient;
			Parameters.SpawnCollisionHandlingOverride =
				ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			Player = World->SpawnActor<APawn>(
				APawn::StaticClass(), FTransform::Identity, Parameters);
			USceneComponent* Root = Player
				? NewObject<USceneComponent>(
					Player, TEXT("P195DivineSensePlayerRoot"), RF_Transient)
				: nullptr;
			Health = Player
				? NewObject<Udemo_mapPlayerHealthComponent>(
					Player, TEXT("P195DivineSensePlayerHealth"), RF_Transient)
				: nullptr;
			if (!Player || !Root || !Health)
			{
				return false;
			}
			Player->SetRootComponent(Root);
			Root->SetWorldLocation(FVector::ZeroVector);
			return Coordinator.TryBeginRun(
				RunId, Player, Health, Diagnostic);
		}

		void Stop()
		{
			Coordinator.Reset();
			if (World)
			{
				World->DestroyWorld(false);
				if (GEngine)
				{
					GEngine->DestroyWorldContext(World);
				}
				World = nullptr;
				Player = nullptr;
				Health = nullptr;
				CollectGarbage(RF_NoFlags);
			}
		}

		~FDivineSenseSessionFixture()
		{
			Stop();
		}
	};

	bool MakeOpeningSnapshot(
		const FGuid& OwnerEntityId,
		float CurrentAmount,
		float MaximumAmount,
		int64 AuthorityRevision,
		FShanmenActionResourceSnapshot& OutSnapshot,
		FGameplayTag Channel =
			FShanmenCombatRuntimeNativeTags::ResourceSpiritEnergy())
	{
		FShanmenActionResourceAuthority Authority;
		return FShanmenActionResourceAuthority::TryCreate(
			OwnerEntityId,
			Channel,
			CurrentAmount,
			MaximumAmount,
			AuthorityRevision,
			Authority)
			&& Authority.TryCaptureSnapshot(OutSnapshot);
	}

	FShanmenCombatActionSnapshot MakeAction(
		const Fdemo_mapCombatRunCoordinator& Coordinator,
		int32 ActivationSequence)
	{
		FShanmenCombatActionCapture Capture;
		Capture.RunId = Coordinator.GetRunId();
		Capture.OwnerId = Coordinator.GetPlayerEntityId();
		Capture.SourceEntityId = Coordinator.GetPlayerEntityId();
		Capture.ActionDefinitionId =
			FShanmenDivineSenseDefinition::CanonicalActionDefinitionId();
		Capture.Content.Version = TEXT("0.0.10.P19.5");
		Capture.Content.Digest = TEXT("TEST-DIGEST-P19.5-SESSION");
		Capture.SourceTags.AddTag(FShanmenCombatNativeTags::SourcePlayer());
		Capture.ActivationId = FShanmenCombatIdFactory::MakeActivationId(
			Capture.RunId,
			Capture.SourceEntityId,
			Capture.ActionDefinitionId,
			ActivationSequence);

		FShanmenCombatActionSnapshot Action;
		check(FShanmenCombatActionSnapshot::TryCapture(Capture, Action));
		return Action;
	}

	FShanmenDivineSenseDefinition MakeDefinition()
	{
		FShanmenDivineSenseDefinitionCapture Capture;
		Capture.ActionDefinitionId =
			FShanmenDivineSenseDefinition::CanonicalActionDefinitionId();
		Capture.ScanRuleId = TEXT("Spell.DivineSense.ProductSession01");
		Capture.Radius = 100.0;
		Capture.MaximumResults = 4;
		Capture.OcclusionPolicy =
			EShanmenDivineSenseOcclusionPolicy::VisibleOnly;
		Capture.RequiredSubjectTags.AddTag(
			FShanmenCombatNativeTags::TargetLiving());
		Capture.BlockedSubjectTags.AddTag(
			FShanmenCombatNativeTags::SourcePlayer());
		Capture.bRejectSelf = true;

		FShanmenDivineSenseDefinition Definition;
		check(FShanmenDivineSenseDefinition::TryCapture(
			Capture, Definition));
		return Definition;
	}

	FShanmenActionResourceCost MakeCost(float Amount)
	{
		FShanmenActionResourceCostCapture Capture;
		Capture.RuleId = TEXT("Spell.DivineSense.SessionSpiritEnergy01");
		Capture.ResourceChannel =
			FShanmenCombatRuntimeNativeTags::ResourceSpiritEnergy();
		Capture.Amount = Amount;
		FShanmenActionResourceCost Cost;
		check(FShanmenActionResourceCost::TryCapture(Capture, Cost));
		return Cost;
	}

	class FSessionEvidenceProvider final
		: public Idemo_mapShanmenDivineSenseWorldEvidenceProvider
	{
	public:
		bool bAvailable = true;
		mutable int32 CallCount = 0;

		virtual bool TryCaptureSubjectEvidence(
			UWorld*,
			const FShanmenDivineSenseScanRequest&,
			const AActor*,
			const AActor*,
			const FGuid&,
			const FVector&,
			const FVector&,
			Fdemo_mapShanmenDivineSenseWorldSubjectEvidence& OutEvidence)
			const override
		{
			OutEvidence =
				Fdemo_mapShanmenDivineSenseWorldSubjectEvidence();
			++CallCount;
			if (!bAvailable)
			{
				return false;
			}
			OutEvidence.SubjectTags.AddTag(
				FShanmenCombatNativeTags::TargetLiving());
			OutEvidence.bHasLineOfSight = true;
			OutEvidence.AuthorityRevision = 5;
			return true;
		}
	};

	bool BeginSession(
		FDivineSenseSessionFixture& Fixture,
		Fdemo_mapShanmenDivineSenseProductSession& Session,
		int32 Capacity,
		float CurrentAmount = 100.0f,
		float MaximumAmount = 100.0f,
		int64 Revision = 0)
	{
		FShanmenActionResourceSnapshot Opening;
		return MakeOpeningSnapshot(
			Fixture.Coordinator.GetPlayerEntityId(),
			CurrentAmount,
			MaximumAmount,
			Revision,
			Opening)
			&& Session.TryBegin(
				Fixture.Coordinator,
				Opening,
				Capacity,
				Fixture.Diagnostic);
	}

	bool CaptureCommand(
		FDivineSenseSessionFixture& Fixture,
		const Fdemo_mapShanmenDivineSenseProductSession& Session,
		int32 ActivationSequence,
		float CostAmount,
		Fdemo_mapShanmenDivineSenseRouteCommand& OutCommand)
	{
		return Session.TryCaptureCommand(
			Fixture.Coordinator,
			MakeAction(Fixture.Coordinator, ActivationSequence),
			MakeDefinition(),
			MakeCost(CostAmount),
			ActivationSequence - 1,
			1,
			{ Fixture.Player },
			OutCommand,
			Fixture.Diagnostic);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapDivineSenseProductSessionLifecycleAndAvailabilityTest,
	"Shanmen.0_0_10.Product.DivineSenseProductSession.LifecycleAndAvailability",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapDivineSenseProductSessionLifecycleAndAvailabilityTest::RunTest(
	const FString&)
{
	FDivineSenseSessionFixture Fixture;
	if (!TestTrue(TEXT("Combat Run starts"), Fixture.Start(SessionRunId)))
	{
		return false;
	}
	Fdemo_mapShanmenDivineSenseProductSession Session;
	TestTrue(TEXT("Default Session is valid and empty"),
		Session.IsValid() && Session.IsEmpty());

	FShanmenActionResourceSnapshot WrongOwner;
	check(MakeOpeningSnapshot(
		ForeignOwnerId, 100.0f, 120.0f, 7, WrongOwner));
	TestFalse(TEXT("Foreign opening owner is rejected"),
		Session.TryBegin(
			Fixture.Coordinator, WrongOwner, 2, Fixture.Diagnostic));
	TestTrue(TEXT("Rejected begin keeps empty state"),
		Session.IsValid() && Session.IsEmpty());

	FShanmenActionResourceSnapshot Opening;
	check(MakeOpeningSnapshot(
		Fixture.Coordinator.GetPlayerEntityId(),
		100.0f,
		120.0f,
		7,
		Opening));
	TestTrue(TEXT("Session begins from explicit snapshot"),
		Session.TryBegin(
			Fixture.Coordinator, Opening, 2, Fixture.Diagnostic));
	const FGuid SessionId = Session.GetSessionId();
	TestTrue(TEXT("Bound Session is valid and Run-consistent"),
		Session.IsValid() && Session.IsActive()
			&& Session.IsConsistentWithCoordinator(Fixture.Coordinator));
	TestTrue(TEXT("Opening snapshot identity is retained"),
		Session.GetOpeningResourceSnapshot().GetSnapshotId()
			== Opening.GetSnapshotId());
	TestTrue(TEXT("Exact begin is idempotent"),
		Session.TryBegin(
			Fixture.Coordinator, Opening, 2, Fixture.Diagnostic)
			&& Session.GetSessionId() == SessionId);
	TestFalse(TEXT("Active Session cannot reset"), Session.Reset());

	Fdemo_mapShanmenDivineSenseAvailabilityProjection Availability;
	TestTrue(TEXT("Availability projection captures"),
		Session.TryCaptureAvailability(
			Fixture.Coordinator, Availability, Fixture.Diagnostic));
	TestTrue(TEXT("Opening projection is valid and current"),
		Availability.IsValid()
			&& Session.IsAvailabilityCurrent(
				Fixture.Coordinator, Availability));
	TestTrue(TEXT("Projection exposes capacity and affordability"),
		Availability.GetRemainingCommandCapacity() == 2
			&& Availability.CanAfford(MakeCost(100.0f))
			&& !Availability.CanAfford(MakeCost(101.0f)));

	Fdemo_mapShanmenDivineSenseRouteCommand Command;
	check(CaptureCommand(Fixture, Session, 1, 10.0f, Command));
	FSessionEvidenceProvider Provider;
	const auto Routed = Session.TryRoute(
		Fixture.Coordinator,
		Fixture.World,
		Fixture.Player,
		Command,
		{ Fixture.Player },
		Provider);
	TestTrue(TEXT("Session route applies"), Routed.IsAccepted());
	TestTrue(TEXT("First route changes availability identity"),
		!Routed.AvailabilityBefore.Matches(Routed.AvailabilityAfter)
			&& Routed.AvailabilityAfter.GetProcessedCommandCount() == 1
			&& Routed.AvailabilityAfter.GetRemainingCommandCapacity() == 1
			&& Routed.AvailabilityAfter.GetResourceSnapshot()
				.GetCurrentAmount() == 90.0f);
	TestFalse(TEXT("Opening projection becomes stale"),
		Session.IsAvailabilityCurrent(Fixture.Coordinator, Availability));
	TestTrue(TEXT("Post-route projection is current"),
		Session.IsAvailabilityCurrent(
			Fixture.Coordinator, Routed.AvailabilityAfter));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapDivineSenseProductSessionReplayTest,
	"Shanmen.0_0_10.Product.DivineSenseProductSession.ReplayWithoutLiveInputs",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapDivineSenseProductSessionReplayTest::RunTest(const FString&)
{
	FDivineSenseSessionFixture Fixture;
	if (!Fixture.Start(SessionRunId))
	{
		return false;
	}
	Fdemo_mapShanmenDivineSenseProductSession Session;
	check(BeginSession(Fixture, Session, 2));
	Fdemo_mapShanmenDivineSenseRouteCommand Command;
	check(CaptureCommand(Fixture, Session, 1, 10.0f, Command));
	FSessionEvidenceProvider Provider;
	const auto Applied = Session.TryRoute(
		Fixture.Coordinator,
		Fixture.World,
		Fixture.Player,
		Command,
		{ Fixture.Player },
		Provider);
	if (!TestTrue(TEXT("Initial Session route applies"),
		Applied.IsAccepted()))
	{
		return false;
	}
	const int32 CallsAfterApply = Provider.CallCount;
	const FGuid ProjectionAfterApply =
		Applied.AvailabilityAfter.GetProjectionId();
	Provider.bAvailable = false;

	const auto Replay = Session.TryRoute(
		Fixture.Coordinator,
		nullptr,
		nullptr,
		Command,
		{},
		Provider);
	TestTrue(TEXT("Exact Session route replays"),
		Replay.IsAccepted() && Replay.IsReplay());
	TestTrue(TEXT("Replay returns the same nested pulse proof"),
		Replay.Route.HostPulse.Pulse.Receipt.GetReceiptId()
			== Applied.Route.HostPulse.Pulse.Receipt.GetReceiptId());
	TestEqual(TEXT("Replay performs no evidence read"),
		Provider.CallCount, CallsAfterApply);
	TestTrue(TEXT("Replay preserves exact availability"),
		Replay.AvailabilityBefore.Matches(Replay.AvailabilityAfter)
			&& Replay.AvailabilityAfter.GetProjectionId()
				== ProjectionAfterApply
			&& Session.GetHost().GetCurrentSpiritEnergy() == 90.0f
			&& Session.GetRouter().NumProcessedCommands() == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapDivineSenseProductSessionFencesAndRollbackTest,
	"Shanmen.0_0_10.Product.DivineSenseProductSession.FencesAndRollback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapDivineSenseProductSessionFencesAndRollbackTest::RunTest(
	const FString&)
{
	FDivineSenseSessionFixture Fixture;
	FDivineSenseSessionFixture Foreign;
	if (!Fixture.Start(SessionRunId)
		|| !Foreign.Start(OtherSessionRunId))
	{
		return false;
	}
	Fdemo_mapShanmenDivineSenseProductSession Session;
	check(BeginSession(Fixture, Session, 2));
	Fdemo_mapShanmenDivineSenseAvailabilityProjection Opening;
	check(Session.TryCaptureAvailability(
		Fixture.Coordinator, Opening, Fixture.Diagnostic));

	Fdemo_mapShanmenDivineSenseRouteCommand ForeignCommand;
	TestFalse(TEXT("Foreign Run cannot capture a command"),
		Session.TryCaptureCommand(
			Foreign.Coordinator,
			MakeAction(Foreign.Coordinator, 1),
			MakeDefinition(),
			MakeCost(10.0f),
			0,
			1,
			{ Foreign.Player },
			ForeignCommand,
			Fixture.Diagnostic));
	Fdemo_mapShanmenDivineSenseAvailabilityProjection ForeignProjection;
	TestFalse(TEXT("Foreign Run cannot read availability"),
		Session.TryCaptureAvailability(
			Foreign.Coordinator,
			ForeignProjection,
			Fixture.Diagnostic));

	FSessionEvidenceProvider Provider;
	Fdemo_mapShanmenDivineSenseRouteCommand InvalidCommand;
	const auto Invalid = Session.TryRoute(
		Fixture.Coordinator,
		Fixture.World,
		Fixture.Player,
		InvalidCommand,
		{},
		Provider);
	TestTrue(TEXT("Invalid command rejection is structured"),
		Invalid.IsValid()
			&& Invalid.Status
				== Edemo_mapShanmenDivineSenseSessionRouteStatus::
					CommandInvalid);

	Fdemo_mapShanmenDivineSenseRouteCommand Command;
	check(CaptureCommand(Fixture, Session, 1, 10.0f, Command));
	Provider.bAvailable = false;
	const auto Rejected = Session.TryRoute(
		Fixture.Coordinator,
		Fixture.World,
		Fixture.Player,
		Command,
		{ Fixture.Player },
		Provider);
	TestTrue(TEXT("Nested Host rejection remains visible"),
		Rejected.IsValid()
			&& Rejected.Status
				== Edemo_mapShanmenDivineSenseSessionRouteStatus::
					RouteRejected
			&& Rejected.Route.Status
				== Edemo_mapShanmenDivineSenseCommandRouteStatus::
					HostRejected);
	TestTrue(TEXT("Rejected route preserves Session projection"),
		Rejected.AvailabilityBefore.Matches(
			Rejected.AvailabilityAfter)
			&& Session.IsAvailabilityCurrent(
				Fixture.Coordinator, Opening)
			&& Session.GetHost().GetCurrentSpiritEnergy() == 100.0f
			&& Session.GetRouter().NumProcessedCommands() == 0);

	Provider.bAvailable = true;
	const auto Recovered = Session.TryRoute(
		Fixture.Coordinator,
		Fixture.World,
		Fixture.Player,
		Command,
		{ Fixture.Player },
		Provider);
	TestTrue(TEXT("Same frozen command recovers"), Recovered.IsAccepted());
	const int32 CallsAfterRecovery = Provider.CallCount;

	Fdemo_mapShanmenDivineSenseRouteCommand Second;
	check(CaptureCommand(Fixture, Session, 2, 10.0f, Second));
	const auto WrongSource = Session.TryRoute(
		Fixture.Coordinator,
		Fixture.World,
		nullptr,
		Second,
		{ Fixture.Player },
		Provider);
	TestTrue(TEXT("Router source fence remains visible through Session"),
		WrongSource.IsValid()
			&& WrongSource.Status
				== Edemo_mapShanmenDivineSenseSessionRouteStatus::
					RouteRejected
			&& WrongSource.Route.Status
				== Edemo_mapShanmenDivineSenseCommandRouteStatus::
					SourceActorMismatch
			&& Provider.CallCount == CallsAfterRecovery);

	const auto ForeignEnd = Session.TryEnd(
		Foreign.Coordinator, Session.GetSessionId());
	TestTrue(TEXT("Foreign Run cannot end active Session"),
		ForeignEnd.IsValid()
			&& ForeignEnd.Status
				== Edemo_mapShanmenDivineSenseSessionEndStatus::RunMismatch
			&& Session.IsActive());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapDivineSenseProductSessionCapacityAndTeardownTest,
	"Shanmen.0_0_10.Product.DivineSenseProductSession.CapacityAndTeardown",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapDivineSenseProductSessionCapacityAndTeardownTest::RunTest(
	const FString&)
{
	FDivineSenseSessionFixture Fixture;
	if (!Fixture.Start(SessionRunId))
	{
		return false;
	}
	Fdemo_mapShanmenDivineSenseProductSession Session;
	check(BeginSession(Fixture, Session, 1, 10.0f, 20.0f, 3));
	const FGuid SessionId = Session.GetSessionId();
	const FGuid OpeningSnapshotId =
		Session.GetOpeningResourceSnapshot().GetSnapshotId();
	Fdemo_mapShanmenDivineSenseAvailabilityProjection Before;
	check(Session.TryCaptureAvailability(
		Fixture.Coordinator, Before, Fixture.Diagnostic));
	TestTrue(TEXT("Single slot can afford exact remaining energy"),
		Before.CanAfford(MakeCost(10.0f)));

	Fdemo_mapShanmenDivineSenseRouteCommand First;
	check(CaptureCommand(Fixture, Session, 1, 10.0f, First));
	FSessionEvidenceProvider Provider;
	const auto Applied = Session.TryRoute(
		Fixture.Coordinator,
		Fixture.World,
		Fixture.Player,
		First,
		{ Fixture.Player },
		Provider);
	TestTrue(TEXT("Capacity-consuming route applies"), Applied.IsAccepted());
	TestTrue(TEXT("Post-route projection exposes exhaustion"),
		Applied.AvailabilityAfter.GetRemainingCommandCapacity() == 0
			&& !Applied.AvailabilityAfter.HasRouteCapacity()
			&& !Applied.AvailabilityAfter.CanAfford(MakeCost(1.0f)));

	Fdemo_mapShanmenDivineSenseRouteCommand Second;
	TestFalse(TEXT("Exhausted Session cannot capture another command"),
		CaptureCommand(Fixture, Session, 2, 1.0f, Second));
	const auto WrongIdEnd = Session.TryEnd(
		Fixture.Coordinator, FGuid(0xD5500099, 0, 0, 1));
	TestTrue(TEXT("Wrong SessionId cannot tear down"),
		WrongIdEnd.IsValid()
			&& WrongIdEnd.Status
				== Edemo_mapShanmenDivineSenseSessionEndStatus::
					SessionMismatch
			&& Session.IsActive());

	const auto Ended = Session.TryEnd(Fixture.Coordinator, SessionId);
	TestTrue(TEXT("Exact Session teardown succeeds"),
		Ended.IsSuccess() && !Ended.IsReplay()
			&& Session.IsEnded() && Session.IsValid());
	TestTrue(TEXT("Terminal receipt closes the snapshot chain"),
		Ended.Receipt.GetSessionId() == SessionId
			&& Ended.Receipt.GetOpeningResourceSnapshotId()
				== OpeningSnapshotId
			&& Ended.Receipt.GetFinalResourceSnapshotId()
				== Applied.AvailabilityAfter.GetResourceSnapshot()
					.GetSnapshotId()
			&& Ended.Receipt.GetProcessedCommandCount() == 1
			&& Ended.Receipt.GetProcessedCommandCapacity() == 1);

	Fdemo_mapShanmenDivineSenseAvailabilityProjection AfterEnd;
	TestFalse(TEXT("Ended Session no longer projects availability"),
		Session.TryCaptureAvailability(
			Fixture.Coordinator, AfterEnd, Fixture.Diagnostic));
	const auto RouteAfterEnd = Session.TryRoute(
		Fixture.Coordinator,
		nullptr,
		nullptr,
		First,
		{},
		Provider);
	TestTrue(TEXT("Ended Session rejects routing"),
		RouteAfterEnd.IsValid()
			&& RouteAfterEnd.Status
				== Edemo_mapShanmenDivineSenseSessionRouteStatus::
					SessionNotActive);

	const auto EndReplay = Session.TryEnd(Fixture.Coordinator, SessionId);
	TestTrue(TEXT("Exact teardown replays immutable receipt"),
		EndReplay.IsSuccess() && EndReplay.IsReplay()
			&& EndReplay.Receipt.GetReceiptId()
				== Ended.Receipt.GetReceiptId());
	TestTrue(TEXT("Ended Session can reset to reusable empty state"),
		Session.Reset() && Session.IsEmpty() && Session.IsValid());
	TestTrue(TEXT("Combat Run can end after Session teardown"),
		Fixture.Coordinator.TryEndRun(
			SessionRunId, Fixture.Diagnostic));
	return true;
}

#endif
