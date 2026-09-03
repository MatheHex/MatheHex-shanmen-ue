#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenDivineSenseCommandRouter.h"

#include "Components/SceneComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Misc/AutomationTest.h"
#include "ShanmenCombatResolver.h"
#include "ShanmenCombatRuntimeTags.h"
#include "ShanmenCombatTags.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	const FGuid RouterRunId(0xD5400001, 0, 0, 1);
	const FGuid OtherRunId(0xD5400002, 0, 0, 1);
	const FGuid RouterOwnerId(0xD5400003, 0, 0, 1);
	const FGuid RouterSourceId(0xD5400004, 0, 0, 1);
	const FGuid RouterSubjectAId(0xD5400010, 0, 0, 1);
	const FGuid RouterSubjectBId(0xD5400011, 0, 0, 1);

	FGameplayTagContainer LivingTags()
	{
		FGameplayTagContainer Tags;
		Tags.AddTag(FShanmenCombatNativeTags::TargetLiving());
		return Tags;
	}

	FShanmenCombatActionSnapshot MakeAction(int32 ActivationSequence)
	{
		FShanmenCombatActionCapture Capture;
		Capture.RunId = RouterRunId;
		Capture.OwnerId = RouterOwnerId;
		Capture.SourceEntityId = RouterSourceId;
		Capture.ActionDefinitionId =
			FShanmenDivineSenseDefinition::CanonicalActionDefinitionId();
		Capture.Content.Version = TEXT("0.0.10.P19.4");
		Capture.Content.Digest = TEXT("TEST-DIGEST-P19.4-ROUTER");
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
		Capture.ScanRuleId = TEXT("Spell.DivineSense.Pulse.CommandRouter01");
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

	FShanmenActionResourceCost MakeCost(float Amount = 10.0f)
	{
		FShanmenActionResourceCostCapture Capture;
		Capture.RuleId = TEXT("Spell.DivineSense.RouterSpiritEnergy01");
		Capture.ResourceChannel =
			FShanmenCombatRuntimeNativeTags::ResourceSpiritEnergy();
		Capture.Amount = Amount;
		FShanmenActionResourceCost Cost;
		check(FShanmenActionResourceCost::TryCapture(Capture, Cost));
		return Cost;
	}

	struct FRouterEvidenceEntry
	{
		FGameplayTagContainer Tags;
		bool bHasLineOfSight = true;
		int64 AuthorityRevision = 0;
	};

	class FRouterEvidenceProvider final
		: public Idemo_mapShanmenDivineSenseWorldEvidenceProvider
	{
	public:
		TMap<FGuid, FRouterEvidenceEntry> Entries;
		FGuid UnavailableSubjectId;
		mutable TArray<FGuid> Calls;

		void Add(const FGuid& EntityId, int64 AuthorityRevision)
		{
			Entries.Add(
				EntityId,
				{ LivingTags(), true, AuthorityRevision });
		}

		virtual bool TryCaptureSubjectEvidence(
			UWorld*,
			const FShanmenDivineSenseScanRequest&,
			const AActor*,
			const AActor*,
			const FGuid& SubjectEntityId,
			const FVector&,
			const FVector&,
			Fdemo_mapShanmenDivineSenseWorldSubjectEvidence&
				OutEvidence) const override
		{
			OutEvidence =
				Fdemo_mapShanmenDivineSenseWorldSubjectEvidence();
			Calls.Add(SubjectEntityId);
			if (SubjectEntityId == UnavailableSubjectId)
			{
				return false;
			}
			const FRouterEvidenceEntry* Entry = Entries.Find(SubjectEntityId);
			if (!Entry)
			{
				return false;
			}
			OutEvidence.SubjectTags = Entry->Tags;
			OutEvidence.bHasLineOfSight = Entry->bHasLineOfSight;
			OutEvidence.AuthorityRevision = Entry->AuthorityRevision;
			return true;
		}
	};

	struct FDivineRouterWorldFixture
	{
		UWorld* World = nullptr;
		FShanmenWorldEntityRegistry Registry;
		AActor* Source = nullptr;
		AActor* SubjectA = nullptr;
		AActor* SubjectB = nullptr;

		AActor* Spawn(const FVector& Location) const
		{
			if (!World)
			{
				return nullptr;
			}
			FActorSpawnParameters Parameters;
			Parameters.ObjectFlags |= RF_Transient;
			Parameters.SpawnCollisionHandlingOverride =
				ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			AActor* Actor = World->SpawnActor<AActor>(
				AActor::StaticClass(), FTransform::Identity, Parameters);
			if (!Actor)
			{
				return nullptr;
			}
			USceneComponent* Root = NewObject<USceneComponent>(
				Actor, NAME_None, RF_Transient);
			if (!Root)
			{
				World->DestroyActor(Actor, true, true);
				return nullptr;
			}
			Actor->SetRootComponent(Root);
			Root->SetWorldLocation(Location);
			return Actor;
		}

		bool Start()
		{
			if (!GEngine)
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

			Source = Spawn(FVector::ZeroVector);
			SubjectA = Spawn(FVector(20.0, 0.0, 0.0));
			SubjectB = Spawn(FVector(30.0, 0.0, 0.0));
			return Source && SubjectA && SubjectB
				&& Registry.TryBeginRun(RouterRunId)
				&& Registry.BindObject(
					RouterRunId, Source, RouterSourceId)
					== EShanmenWorldBindingResult::Bound
				&& Registry.BindObject(
					RouterRunId, SubjectA, RouterSubjectAId)
					== EShanmenWorldBindingResult::Bound
				&& Registry.BindObject(
					RouterRunId, SubjectB, RouterSubjectBId)
					== EShanmenWorldBindingResult::Bound;
		}

		void Stop()
		{
			Registry.Reset();
			if (World)
			{
				World->DestroyWorld(false);
				if (GEngine)
				{
					GEngine->DestroyWorldContext(World);
				}
				World = nullptr;
				Source = nullptr;
				SubjectA = nullptr;
				SubjectB = nullptr;
				CollectGarbage(RF_NoFlags);
			}
		}

		~FDivineRouterWorldFixture()
		{
			Stop();
		}
	};

	bool Open(
		int32 Capacity,
		Fdemo_mapShanmenDivineSenseProductHost& OutHost,
		Fdemo_mapShanmenDivineSenseCommandRouter& OutRouter,
		float Current = 100.0f,
		float Maximum = 100.0f)
	{
		return Fdemo_mapShanmenDivineSenseProductHost::TryOpen(
			RouterRunId,
			RouterSourceId,
			Current,
			Maximum,
			0,
			Capacity,
			OutHost)
			&& Fdemo_mapShanmenDivineSenseCommandRouter::TryCreate(
				OutHost, OutRouter);
	}

	bool Capture(
		const Fdemo_mapShanmenDivineSenseCommandRouter& Router,
		const Fdemo_mapShanmenDivineSenseProductHost& Host,
		const FDivineRouterWorldFixture& Fixture,
		int32 ActivationSequence,
		const TArray<AActor*>& Subjects,
		Fdemo_mapShanmenDivineSenseRouteCommand& OutCommand,
		int32 ScanOrdinal = 0,
		int32 Budget = 4)
	{
		return Router.TryCaptureCommand(
			Host,
			Fixture.Registry,
			MakeAction(ActivationSequence),
			MakeDefinition(),
			MakeCost(),
			ScanOrdinal,
			Budget,
			Subjects,
			OutCommand);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapDivineSenseCommandRouterCaptureAndApplyTest,
	"Shanmen.0_0_10.Product.DivineSenseCommandRouter.CaptureAndApply",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapDivineSenseCommandRouterCaptureAndApplyTest::RunTest(
	const FString&)
{
	FDivineRouterWorldFixture Fixture;
	if (!TestTrue(TEXT("World fixture starts"), Fixture.Start()))
	{
		return false;
	}
	Fdemo_mapShanmenDivineSenseProductHost Host;
	Fdemo_mapShanmenDivineSenseCommandRouter Router;
	if (!TestTrue(TEXT("Host and Router open"), Open(3, Host, Router)))
	{
		return false;
	}
	FRouterEvidenceProvider Provider;
	Provider.Add(RouterSubjectAId, 7);
	Provider.Add(RouterSubjectBId, 8);

	Fdemo_mapShanmenDivineSenseRouteCommand Command;
	Fdemo_mapShanmenDivineSenseRouteCommand Equivalent;
	TestTrue(TEXT("Unordered explicit Actors capture"),
		Capture(
			Router,
			Host,
			Fixture,
			1,
			{ Fixture.SubjectB, Fixture.SubjectA },
			Command));
	TestTrue(TEXT("Equivalent capture succeeds"),
		Capture(
			Router,
			Host,
			Fixture,
			1,
			{ Fixture.SubjectA, Fixture.SubjectB },
			Equivalent));
	TestTrue(TEXT("Captured command is valid"), Command.IsValid());
	TestTrue(TEXT("Equivalent command matches"), Command.Matches(Equivalent));
	TestEqual(TEXT("Subjects freeze in stable order"),
		Command.GetSubjectEntityIds()[0], RouterSubjectAId);
	TestEqual(TEXT("Second stable subject"),
		Command.GetSubjectEntityIds()[1], RouterSubjectBId);
	TestEqual(TEXT("Projection is frozen"),
		Command.GetExpectedResourceSnapshotId(),
		Router.GetCurrentResourceSnapshot().GetSnapshotId());

	const auto Applied = Router.TryRoute(
		Host,
		Fixture.World,
		Fixture.Registry,
		Fixture.Source,
		Command,
		{ Fixture.SubjectB, Fixture.SubjectA },
		Provider);
	TestTrue(TEXT("Route result is valid"), Applied.IsValid());
	TestTrue(TEXT("Route applies"), Applied.IsAccepted());
	TestFalse(TEXT("Initial route is not replay"), Applied.IsReplay());
	TestEqual(TEXT("Applied status"), Applied.Status,
		Edemo_mapShanmenDivineSenseCommandRouteStatus::Applied);
	TestEqual(TEXT("Host spends once"), Host.GetCurrentSpiritEnergy(), 90.0f);
	TestEqual(TEXT("Host records one pulse"), Host.NumProcessedPulses(), 1);
	TestEqual(TEXT("Router records one command"),
		Router.NumProcessedCommands(), 1);
	TestEqual(TEXT("Provider sees both subjects"), Provider.Calls.Num(), 2);
	TestEqual(TEXT("Provider order is canonical"),
		Provider.Calls[0], RouterSubjectAId);
	TestEqual(TEXT("Provider second identity"),
		Provider.Calls[1], RouterSubjectBId);
	TestTrue(TEXT("Router stays consistent"),
		Router.IsConsistentWithHost(Host));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapDivineSenseCommandRouterReplayTest,
	"Shanmen.0_0_10.Product.DivineSenseCommandRouter.ReplayWithoutLiveInputs",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapDivineSenseCommandRouterReplayTest::RunTest(const FString&)
{
	FDivineRouterWorldFixture Fixture;
	if (!Fixture.Start())
	{
		return false;
	}
	Fdemo_mapShanmenDivineSenseProductHost Host;
	Fdemo_mapShanmenDivineSenseCommandRouter Router;
	check(Open(2, Host, Router));
	FRouterEvidenceProvider Provider;
	Provider.Add(RouterSubjectAId, 4);
	Fdemo_mapShanmenDivineSenseRouteCommand Command;
	check(Capture(
		Router, Host, Fixture, 1, { Fixture.SubjectA }, Command));
	const auto Applied = Router.TryRoute(
		Host,
		Fixture.World,
		Fixture.Registry,
		Fixture.Source,
		Command,
		{ Fixture.SubjectA },
		Provider);
	if (!TestTrue(TEXT("Initial route applies"), Applied.IsAccepted()))
	{
		return false;
	}
	const int32 CallsAfterApply = Provider.Calls.Num();
	const FGuid SnapshotAfterApply =
		Router.GetCurrentResourceSnapshot().GetSnapshotId();
	Provider.UnavailableSubjectId = RouterSubjectAId;

	const auto Replay = Router.TryRoute(
		Host,
		nullptr,
		Fixture.Registry,
		nullptr,
		Command,
		{},
		Provider);
	TestTrue(TEXT("Replay result is valid"), Replay.IsValid());
	TestTrue(TEXT("Exact route replays"), Replay.IsAccepted());
	TestTrue(TEXT("Replay status is explicit"), Replay.IsReplay());
	TestEqual(TEXT("Replay returns original receipt"),
		Replay.HostPulse.Pulse.Receipt.GetReceiptId(),
		Applied.HostPulse.Pulse.Receipt.GetReceiptId());
	TestEqual(TEXT("Replay performs no evidence read"),
		Provider.Calls.Num(), CallsAfterApply);
	TestEqual(TEXT("Replay does not spend twice"),
		Host.GetCurrentSpiritEnergy(), 90.0f);
	TestEqual(TEXT("Replay does not add records"),
		Router.NumProcessedCommands(), 1);
	TestEqual(TEXT("Replay keeps current projection"),
		Router.GetCurrentResourceSnapshot().GetSnapshotId(),
		SnapshotAfterApply);

	Fdemo_mapShanmenDivineSenseRouteCommand Recaptured;
	TestTrue(TEXT("Accepted command can be recaptured"),
		Capture(
			Router, Host, Fixture, 1, { Fixture.SubjectA }, Recaptured));
	TestTrue(TEXT("Recapture returns original identity"),
		Command.Matches(Recaptured));
	TestTrue(TEXT("Router and Host remain consistent"),
		Router.IsConsistentWithHost(Host));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapDivineSenseCommandRouterProjectionAndConflictTest,
	"Shanmen.0_0_10.Product.DivineSenseCommandRouter.ProjectionCapacityAndConflict",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapDivineSenseCommandRouterProjectionAndConflictTest::RunTest(
	const FString&)
{
	FDivineRouterWorldFixture Fixture;
	if (!Fixture.Start())
	{
		return false;
	}
	Fdemo_mapShanmenDivineSenseProductHost Host;
	Fdemo_mapShanmenDivineSenseCommandRouter Router;
	check(Open(2, Host, Router));
	FRouterEvidenceProvider Provider;
	Provider.Add(RouterSubjectAId, 1);
	Provider.Add(RouterSubjectBId, 2);

	Fdemo_mapShanmenDivineSenseRouteCommand First;
	Fdemo_mapShanmenDivineSenseRouteCommand StaleSecond;
	Fdemo_mapShanmenDivineSenseRouteCommand Conflict;
	Fdemo_mapShanmenDivineSenseRouteCommand CapacityThird;
	check(Capture(Router, Host, Fixture, 1, { Fixture.SubjectA }, First));
	check(Capture(
		Router, Host, Fixture, 2, { Fixture.SubjectB }, StaleSecond));
	check(Capture(
		Router,
		Host,
		Fixture,
		1,
		{ Fixture.SubjectA },
		Conflict,
		1,
		4));
	check(Capture(
		Router, Host, Fixture, 3, { Fixture.SubjectA }, CapacityThird));
	TestNotEqual(TEXT("Conflicting activation has another route identity"),
		Conflict.GetRouteCommandId(), First.GetRouteCommandId());

	const auto AppliedFirst = Router.TryRoute(
		Host,
		Fixture.World,
		Fixture.Registry,
		Fixture.Source,
		First,
		{ Fixture.SubjectA },
		Provider);
	TestTrue(TEXT("First command applies"), AppliedFirst.IsAccepted());
	const int32 CallsAfterFirst = Provider.Calls.Num();

	const auto ActivationRejected = Router.TryRoute(
		Host,
		nullptr,
		Fixture.Registry,
		nullptr,
		Conflict,
		{},
		Provider);
	TestEqual(TEXT("Activation reuse fails closed"),
		ActivationRejected.Status,
		Edemo_mapShanmenDivineSenseCommandRouteStatus::ActivationConflict);
	TestEqual(TEXT("Conflict is evidence-read free"),
		Provider.Calls.Num(), CallsAfterFirst);

	const auto Stale = Router.TryRoute(
		Host,
		Fixture.World,
		Fixture.Registry,
		Fixture.Source,
		StaleSecond,
		{ Fixture.SubjectB },
		Provider);
	TestEqual(TEXT("Old projection is rejected"), Stale.Status,
		Edemo_mapShanmenDivineSenseCommandRouteStatus::
			ResourceProjectionStale);
	TestEqual(TEXT("Stale route is evidence-read free"),
		Provider.Calls.Num(), CallsAfterFirst);

	Fdemo_mapShanmenDivineSenseRouteCommand FreshSecond;
	TestTrue(TEXT("Second command recaptures current projection"),
		Capture(
			Router, Host, Fixture, 2, { Fixture.SubjectB }, FreshSecond));
	const auto AppliedSecond = Router.TryRoute(
		Host,
		Fixture.World,
		Fixture.Registry,
		Fixture.Source,
		FreshSecond,
		{ Fixture.SubjectB },
		Provider);
	TestTrue(TEXT("Fresh second command applies"), AppliedSecond.IsAccepted());
	const int32 CallsAtCapacity = Provider.Calls.Num();

	const auto Capacity = Router.TryRoute(
		Host,
		nullptr,
		Fixture.Registry,
		nullptr,
		CapacityThird,
		{},
		Provider);
	TestEqual(TEXT("Router capacity wins before stale live input"),
		Capacity.Status,
		Edemo_mapShanmenDivineSenseCommandRouteStatus::
			ProcessedCapacityExceeded);
	TestEqual(TEXT("Capacity path is evidence-read free"),
		Provider.Calls.Num(), CallsAtCapacity);
	TestEqual(TEXT("Two accepted commands spend twice"),
		Host.GetCurrentSpiritEnergy(), 80.0f);
	TestEqual(TEXT("Router reaches exact capacity"),
		Router.NumProcessedCommands(), 2);
	TestTrue(TEXT("Router remains consistent at capacity"),
		Router.IsConsistentWithHost(Host));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapDivineSenseCommandRouterRollbackAndFencesTest,
	"Shanmen.0_0_10.Product.DivineSenseCommandRouter.RollbackAndIdentityFences",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapDivineSenseCommandRouterRollbackAndFencesTest::RunTest(
	const FString&)
{
	FDivineRouterWorldFixture Fixture;
	if (!Fixture.Start())
	{
		return false;
	}
	Fdemo_mapShanmenDivineSenseProductHost Host;
	Fdemo_mapShanmenDivineSenseCommandRouter Router;
	check(Open(3, Host, Router));
	FRouterEvidenceProvider Provider;
	Provider.Add(RouterSubjectAId, 3);
	Provider.Add(RouterSubjectBId, 4);
	Provider.UnavailableSubjectId = RouterSubjectAId;
	Fdemo_mapShanmenDivineSenseRouteCommand First;
	check(Capture(Router, Host, Fixture, 1, { Fixture.SubjectA }, First));

	const auto Rejected = Router.TryRoute(
		Host,
		Fixture.World,
		Fixture.Registry,
		Fixture.Source,
		First,
		{ Fixture.SubjectA },
		Provider);
	TestTrue(TEXT("Host rejection is structured"), Rejected.IsValid());
	TestEqual(TEXT("Host rejection remains visible"), Rejected.Status,
		Edemo_mapShanmenDivineSenseCommandRouteStatus::HostRejected);
	TestEqual(TEXT("World evidence cause remains visible"),
		Rejected.HostPulse.Pulse.WorldFailure.Status,
		Edemo_mapShanmenDivineSenseWorldObservationStatus::
			SubjectEvidenceUnavailable);
	TestEqual(TEXT("Rejected route preserves Host balance"),
		Host.GetCurrentSpiritEnergy(), 100.0f);
	TestEqual(TEXT("Rejected route preserves Router ledger"),
		Router.NumProcessedCommands(), 0);
	TestTrue(TEXT("Rejected state remains consistent"),
		Router.IsConsistentWithHost(Host));

	Provider.UnavailableSubjectId.Invalidate();
	const auto Recovered = Router.TryRoute(
		Host,
		Fixture.World,
		Fixture.Registry,
		Fixture.Source,
		First,
		{ Fixture.SubjectA },
		Provider);
	TestTrue(TEXT("Same frozen command recovers"), Recovered.IsAccepted());
	const int32 CallsAfterRecovery = Provider.Calls.Num();

	Fdemo_mapShanmenDivineSenseRouteCommand Second;
	check(Capture(Router, Host, Fixture, 2, { Fixture.SubjectA }, Second));
	const auto WrongSource = Router.TryRoute(
		Host,
		Fixture.World,
		Fixture.Registry,
		Fixture.SubjectA,
		Second,
		{ Fixture.SubjectA },
		Provider);
	TestEqual(TEXT("Wrong source Actor is rejected"), WrongSource.Status,
		Edemo_mapShanmenDivineSenseCommandRouteStatus::
			SourceActorMismatch);

	const auto WrongSubjects = Router.TryRoute(
		Host,
		Fixture.World,
		Fixture.Registry,
		Fixture.Source,
		Second,
		{ Fixture.SubjectB },
		Provider);
	TestEqual(TEXT("Changed subject set is rejected"), WrongSubjects.Status,
		Edemo_mapShanmenDivineSenseCommandRouteStatus::
			SubjectActorMismatch);
	TestEqual(TEXT("Identity fences avoid evidence reads"),
		Provider.Calls.Num(), CallsAfterRecovery);

	FShanmenWorldEntityRegistry ForeignRegistry;
	check(ForeignRegistry.TryBeginRun(OtherRunId));
	const auto WrongRegistry = Router.TryRoute(
		Host,
		Fixture.World,
		ForeignRegistry,
		Fixture.Source,
		Second,
		{ Fixture.SubjectA },
		Provider);
	TestEqual(TEXT("Foreign registry is rejected"), WrongRegistry.Status,
		Edemo_mapShanmenDivineSenseCommandRouteStatus::RegistryMismatch);

	Fdemo_mapShanmenDivineSenseProductHost OtherHost;
	check(Fdemo_mapShanmenDivineSenseProductHost::TryOpen(
		RouterRunId,
		RouterSourceId,
		50.0f,
		100.0f,
		0,
		3,
		OtherHost));
	const auto WrongHost = Router.TryRoute(
		OtherHost,
		Fixture.World,
		Fixture.Registry,
		Fixture.Source,
		Second,
		{ Fixture.SubjectA },
		Provider);
	TestEqual(TEXT("Foreign Host is rejected"), WrongHost.Status,
		Edemo_mapShanmenDivineSenseCommandRouteStatus::HostMismatch);

	Fdemo_mapShanmenDivineSenseCommandRouter DefaultRouter;
	const auto NotReady = DefaultRouter.TryRoute(
		Host,
		nullptr,
		Fixture.Registry,
		nullptr,
		Second,
		{},
		Provider);
	TestTrue(TEXT("Default Router rejection is structured"),
		NotReady.IsValid());
	TestEqual(TEXT("Default Router is not ready"), NotReady.Status,
		Edemo_mapShanmenDivineSenseCommandRouteStatus::RouterNotReady);

	Fdemo_mapShanmenDivineSenseCommandRouter LateRouter;
	TestFalse(TEXT("Router cannot attach after Host processing"),
		Fdemo_mapShanmenDivineSenseCommandRouter::TryCreate(
			Host, LateRouter));
	TestEqual(TEXT("All fences preserve accepted state"),
		Host.GetCurrentSpiritEnergy(), 90.0f);
	TestEqual(TEXT("All fences preserve Router count"),
		Router.NumProcessedCommands(), 1);
	TestTrue(TEXT("Final state remains consistent"),
		Router.IsConsistentWithHost(Host));
	return true;
}

#endif
