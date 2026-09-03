#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenDivineSenseProductRoute.h"

#include "Components/BoxComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "Misc/AutomationTest.h"
#include "ShanmenCombatRuntimeTags.h"
#include "ShanmenCombatTags.h"
#include "demo_mapCombatRunCoordinator.h"
#include "demo_mapEnemyCharacter.h"
#include "demo_mapM01EnemyIdentityComponent.h"
#include "demo_mapM01EnemyTypes.h"
#include "demo_mapPlayerHealthComponent.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	const EAutomationTestFlags ProductRouteFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;
	const FGuid RouteRunA(
		0xD5800001, 0xD5800002, 0xD5800003, 0xD5800004);
	const FGuid RouteRunB(
		0xD5810001, 0xD5810002, 0xD5810003, 0xD5810004);
	const FGuid RouteRunC(
		0xD5820001, 0xD5820002, 0xD5820003, 0xD5820004);
	const FGuid RouteRunD(
		0xD5830001, 0xD5830002, 0xD5830003, 0xD5830004);

	const Fdemo_mapM01EnemyDefinition* FindRouteEnemyDefinition()
	{
		for (const Fdemo_mapM01EnemyDefinition& Definition :
			Fdemo_mapM01EnemyConfig::GetDefinitions())
		{
			if (Definition.Archetype
				== Edemo_mapM01EnemyArchetype::StandardSkirmisher)
			{
				return &Definition;
			}
		}
		return nullptr;
	}

	Fdemo_mapEnemyEncounterIdentity MakeRouteEncounterIdentity(
		const Fdemo_mapM01EnemyDefinition& Definition)
	{
		Fdemo_mapEnemyEncounterIdentity Identity;
		Identity.EncounterId = Definition.EncounterId;
		Identity.RouteId = Definition.RouteId;
		Identity.SpawnMarkerId = Definition.SpawnMarkerId;
		Identity.LootTableId = Definition.CorpseIdentity;
		Identity.SkillProfileId = Definition.SkillProfileId;
		return Identity;
	}

	struct FDivineSenseRouteFixture
	{
		UWorld* World = nullptr;
		APawn* Player = nullptr;
		UBoxComponent* PlayerRoot = nullptr;
		Udemo_mapPlayerHealthComponent* Health = nullptr;
		Ademo_mapEnemyCharacter* Enemy = nullptr;
		Udemo_mapM01EnemyIdentityComponent* EnemyIdentity = nullptr;
		Fdemo_mapCombatRunCoordinator Coordinator;
		FString Diagnostic;

		bool Start(const FGuid& RunId)
		{
			const Fdemo_mapM01EnemyDefinition* Definition =
				FindRouteEnemyDefinition();
			if (!GEngine || !RunId.IsValid() || !Definition)
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
			PlayerRoot = Player
				? NewObject<UBoxComponent>(
					Player, TEXT("P198DivineSensePlayerRoot"), RF_Transient)
				: nullptr;
			Health = Player
				? NewObject<Udemo_mapPlayerHealthComponent>(
					Player,
					TEXT("P198DivineSensePlayerHealth"),
					RF_Transient)
				: nullptr;
			if (!Player || !PlayerRoot || !Health)
			{
				return false;
			}
			Player->SetRootComponent(PlayerRoot);
			Player->AddInstanceComponent(PlayerRoot);
			Player->AddInstanceComponent(Health);
			PlayerRoot->SetWorldLocation(FVector::ZeroVector);

			Enemy = World->SpawnActor<Ademo_mapEnemyCharacter>(
				Ademo_mapEnemyCharacter::StaticClass(),
				FTransform(FVector(100.0, 0.0, 0.0)),
				Parameters);
			EnemyIdentity = Enemy
				? NewObject<Udemo_mapM01EnemyIdentityComponent>(
					Enemy,
					TEXT("P198DivineSenseEnemyIdentity"),
					RF_Transient)
				: nullptr;
			if (!Enemy || !EnemyIdentity)
			{
				return false;
			}
			Enemy->AddInstanceComponent(EnemyIdentity);
			return EnemyIdentity->Configure(*Definition)
				&& Enemy->ConfigureEncounter(
					MakeRouteEncounterIdentity(*Definition),
					Definition->Tuning,
					Definition->IsElite())
				&& Coordinator.TryBeginRun(
					RunId, Player, Health, Diagnostic)
				&& Coordinator.TryRegisterM01Enemy(Enemy, Diagnostic);
		}

		AActor* SpawnUnregisteredActor()
		{
			if (!World)
			{
				return nullptr;
			}
			FActorSpawnParameters Parameters;
			Parameters.ObjectFlags |= RF_Transient;
			Parameters.SpawnCollisionHandlingOverride =
				ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			return World->SpawnActor<AActor>(
				AActor::StaticClass(), FTransform::Identity, Parameters);
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
				PlayerRoot = nullptr;
				Health = nullptr;
				Enemy = nullptr;
				EnemyIdentity = nullptr;
				CollectGarbage(RF_NoFlags);
			}
		}

		~FDivineSenseRouteFixture()
		{
			Stop();
		}
	};

	bool MakeOpeningSnapshot(
		const FGuid& OwnerEntityId,
		float CurrentAmount,
		float MaximumAmount,
		FShanmenActionResourceSnapshot& OutSnapshot,
		int64 AuthorityRevision = 0)
	{
		FShanmenActionResourceAuthority Authority;
		return FShanmenActionResourceAuthority::TryCreate(
			OwnerEntityId,
			FShanmenCombatRuntimeNativeTags::ResourceSpiritEnergy(),
			CurrentAmount,
			MaximumAmount,
			AuthorityRevision,
			Authority)
			&& Authority.TryCaptureSnapshot(OutSnapshot);
	}

	bool BeginRoute(
		FDivineSenseRouteFixture& Fixture,
		Fdemo_mapShanmenDivineSenseProductController& Controller,
		float CurrentAmount = 100.0f,
		float MaximumAmount = 100.0f)
	{
		FShanmenActionResourceSnapshot Opening;
		return MakeOpeningSnapshot(
			Fixture.Coordinator.GetPlayerEntityId(),
			CurrentAmount,
			MaximumAmount,
			Opening,
			7)
			&& Fdemo_mapShanmenDivineSenseProductRoute::TryBegin(
				Controller,
				Fixture.Coordinator,
				Opening,
				Fixture.Diagnostic);
	}

	class FRouteEvidenceProvider final
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
			++CallCount;
			OutEvidence =
				Fdemo_mapShanmenDivineSenseWorldSubjectEvidence();
			if (!bAvailable)
			{
				return false;
			}
			OutEvidence.SubjectTags.AddTag(
				FShanmenCombatNativeTags::TargetLiving());
			OutEvidence.bHasLineOfSight = true;
			OutEvidence.AuthorityRevision = 8;
			return true;
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapDivineSenseProductRouteCanonicalBeginTest,
	"Shanmen.0_0_10.Product.DivineSenseProductRoute.CanonicalBegin",
	ProductRouteFlags)

bool Fdemo_mapDivineSenseProductRouteCanonicalBeginTest::RunTest(
	const FString&)
{
	Fdemo_mapCombatRunCoordinator Unready;
	Fdemo_mapShanmenDivineSenseProductController UnreadyController;
	FShanmenActionResourceSnapshot ForeignOpening;
	check(MakeOpeningSnapshot(
		FGuid(0xD5800090, 0, 0, 1), 100.0f, 100.0f, ForeignOpening));
	FString Diagnostic;
	TestFalse(TEXT("unready Run cannot begin product route"),
		Fdemo_mapShanmenDivineSenseProductRoute::TryBegin(
			UnreadyController, Unready, ForeignOpening, Diagnostic));
	TestTrue(TEXT("unready rejection leaves Controller empty"),
		UnreadyController.IsEmpty() && UnreadyController.IsValid());

	FDivineSenseRouteFixture Fixture;
	if (!TestTrue(TEXT("fixture starts"), Fixture.Start(RouteRunA)))
	{
		return false;
	}
	FShanmenActionResourceSnapshot Opening;
	check(MakeOpeningSnapshot(
		Fixture.Coordinator.GetPlayerEntityId(),
		100.0f,
		120.0f,
		Opening,
		9));
	Fdemo_mapShanmenDivineSenseProductController Controller;
	TestTrue(TEXT("route begins canonical Controller"),
		Fdemo_mapShanmenDivineSenseProductRoute::TryBegin(
			Controller,
			Fixture.Coordinator,
			Opening,
			Fixture.Diagnostic));
	const FGuid ControllerId = Controller.GetControllerId();
	TestTrue(TEXT("caller cannot select product config"),
		Controller.IsActive() && Controller.IsValid()
			&& Fdemo_mapShanmenDivineSenseProductAuthority::
				IsCanonicalConfig(Controller.GetConfig()));
	TestTrue(TEXT("exact route begin is idempotent"),
		Fdemo_mapShanmenDivineSenseProductRoute::TryBegin(
			Controller,
			Fixture.Coordinator,
			Opening,
			Fixture.Diagnostic)
			&& Controller.GetControllerId() == ControllerId);
	TestFalse(TEXT("active Controller cannot be reset around route"),
		Controller.Reset());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapDivineSenseProductRouteUseReplayTest,
	"Shanmen.0_0_10.Product.DivineSenseProductRoute.UseAndReplay",
	ProductRouteFlags)

bool Fdemo_mapDivineSenseProductRouteUseReplayTest::RunTest(
	const FString&)
{
	FDivineSenseRouteFixture Fixture;
	if (!Fixture.Start(RouteRunA))
	{
		return false;
	}
	Fdemo_mapShanmenDivineSenseProductController Controller;
	check(BeginRoute(Fixture, Controller));
	FRouteEvidenceProvider Provider;
	const auto Applied = Fdemo_mapShanmenDivineSenseProductRoute::TryUse(
		Controller,
		Fixture.Coordinator,
		Fixture.World,
		Fixture.Player,
		{},
		Provider);
	if (!TestTrue(TEXT("device-independent use applies"),
		Applied.IsAccepted() && !Applied.IsReplay()
			&& !Applied.bReusedAttempt && Applied.HasAttempt()))
	{
		return false;
	}
	const auto Attempt = Applied.Attempt;
	const FGuid CommandId =
		Applied.ControllerResult.Command.GetRouteCommandId();
	TestTrue(TEXT("route issued immutable canonical identity"),
		Attempt.GetPrepared().Reservation.GetActivationSequence() == 1
			&& Attempt.GetConfigId()
				== Fdemo_mapShanmenDivineSenseProductAuthority::
					CanonicalConfigId()
			&& Fixture.Coordinator.
				GetNextPlayerDivineSenseActivationSequence() == 2);
	TestTrue(TEXT("first use mutates product state once"),
		Controller.NumCapturedIntents() == 1
			&& Controller.GetSession().GetHost().GetCurrentSpiritEnergy()
				== 90.0f);

	Provider.bAvailable = false;
	const int32 CallsBeforeReplay = Provider.CallCount;
	const auto Replay = Fdemo_mapShanmenDivineSenseProductRoute::TryRetry(
		Controller,
		Fixture.Coordinator,
		nullptr,
		nullptr,
		Attempt,
		{},
		Provider);
	TestTrue(TEXT("accepted attempt replays without live inputs"),
		Replay.IsAccepted() && Replay.IsReplay()
			&& Replay.bReusedAttempt
			&& Replay.Attempt.Matches(Attempt));
	TestEqual(TEXT("replay performs no provider read"),
		Provider.CallCount, CallsBeforeReplay);
	TestTrue(TEXT("replay consumes neither identity nor resource"),
		Fixture.Coordinator.GetNextPlayerDivineSenseActivationSequence() == 2
			&& Controller.NumCapturedIntents() == 1
			&& Controller.GetSession().GetHost().GetCurrentSpiritEnergy()
				== 90.0f
			&& Replay.ControllerResult.Command.GetRouteCommandId()
				== CommandId);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapDivineSenseProductRoutePreflightTest,
	"Shanmen.0_0_10.Product.DivineSenseProductRoute.PreflightFences",
	ProductRouteFlags)

bool Fdemo_mapDivineSenseProductRoutePreflightTest::RunTest(
	const FString&)
{
	FDivineSenseRouteFixture Fixture;
	FDivineSenseRouteFixture Foreign;
	if (!Fixture.Start(RouteRunA) || !Foreign.Start(RouteRunB))
	{
		return false;
	}
	Fdemo_mapShanmenDivineSenseProductController Controller;
	check(BeginRoute(Fixture, Controller));
	FRouteEvidenceProvider Provider;
	auto IsLiveRejection = [this](
		const TCHAR* Label,
		const Fdemo_mapShanmenDivineSenseProductRouteResult& Result)
	{
		return TestTrue(Label,
			Result.IsValid()
				&& Result.Status
					== Edemo_mapShanmenDivineSenseProductRouteStatus::
						LiveInputRejected
				&& !Result.HasAttempt());
	};

	IsLiveRejection(TEXT("null World rejects"),
		Fdemo_mapShanmenDivineSenseProductRoute::TryUse(
			Controller,
			Fixture.Coordinator,
			nullptr,
			Fixture.Player,
			{},
			Provider));
	IsLiveRejection(TEXT("non-player source rejects"),
		Fdemo_mapShanmenDivineSenseProductRoute::TryUse(
			Controller,
			Fixture.Coordinator,
			Fixture.World,
			Fixture.Enemy,
			{},
			Provider));
	IsLiveRejection(TEXT("canonical self subject rejects"),
		Fdemo_mapShanmenDivineSenseProductRoute::TryUse(
			Controller,
			Fixture.Coordinator,
			Fixture.World,
			Fixture.Player,
			{ Fixture.Player },
			Provider));
	IsLiveRejection(TEXT("duplicate stable subject rejects"),
		Fdemo_mapShanmenDivineSenseProductRoute::TryUse(
			Controller,
			Fixture.Coordinator,
			Fixture.World,
			Fixture.Player,
			{ Fixture.Enemy, Fixture.Enemy },
			Provider));
	AActor* Unregistered = Fixture.SpawnUnregisteredActor();
	IsLiveRejection(TEXT("unregistered subject rejects"),
		Fdemo_mapShanmenDivineSenseProductRoute::TryUse(
			Controller,
			Fixture.Coordinator,
			Fixture.World,
			Fixture.Player,
			{ Unregistered },
			Provider));
	IsLiveRejection(TEXT("foreign World subject rejects"),
		Fdemo_mapShanmenDivineSenseProductRoute::TryUse(
			Controller,
			Fixture.Coordinator,
			Fixture.World,
			Fixture.Player,
			{ Foreign.Enemy },
			Provider));
	TArray<AActor*> Oversized;
	Oversized.Init(
		Fixture.Enemy,
		Fdemo_mapShanmenDivineSenseProductAuthority::
			CanonicalSubjectActorBudget() + 1);
	IsLiveRejection(TEXT("oversized explicit batch rejects"),
		Fdemo_mapShanmenDivineSenseProductRoute::TryUse(
			Controller,
			Fixture.Coordinator,
			Fixture.World,
			Fixture.Player,
			Oversized,
			Provider));
	TestTrue(TEXT("all preflight fences precede identity consumption"),
		Fixture.Coordinator.GetNextPlayerDivineSenseActivationSequence() == 1
			&& Controller.NumCapturedIntents() == 0
			&& Controller.GetSession().GetHost().GetCurrentSpiritEnergy()
				== 100.0f
			&& Provider.CallCount == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapDivineSenseProductRouteRecoveryTest,
	"Shanmen.0_0_10.Product.DivineSenseProductRoute.RejectedRecovery",
	ProductRouteFlags)

bool Fdemo_mapDivineSenseProductRouteRecoveryTest::RunTest(
	const FString&)
{
	FDivineSenseRouteFixture Fixture;
	if (!Fixture.Start(RouteRunA))
	{
		return false;
	}
	Fdemo_mapShanmenDivineSenseProductController Controller;
	check(BeginRoute(Fixture, Controller));
	FRouteEvidenceProvider Provider;
	Provider.bAvailable = false;
	const auto Rejected = Fdemo_mapShanmenDivineSenseProductRoute::TryUse(
		Controller,
		Fixture.Coordinator,
		Fixture.World,
		Fixture.Player,
		{ Fixture.Enemy },
		Provider);
	if (!TestTrue(TEXT("provider failure retains route-issued attempt"),
		Rejected.IsValid() && !Rejected.IsAccepted()
			&& Rejected.Status
				== Edemo_mapShanmenDivineSenseProductRouteStatus::
					ControllerRejected
			&& Rejected.HasAttempt()
			&& Rejected.ControllerResult.Status
				== Edemo_mapShanmenDivineSenseProductControllerStatus::
					RouteRejected))
	{
		return false;
	}
	const FGuid IntentId = Rejected.Attempt.GetIntent().GetIntentId();
	const FGuid CommandId =
		Rejected.ControllerResult.Command.GetRouteCommandId();
	TestTrue(TEXT("one failed live route consumes exactly one identity"),
		Fixture.Coordinator.GetNextPlayerDivineSenseActivationSequence() == 2
			&& Controller.NumCapturedIntents() == 1
			&& Controller.GetSession().GetHost().GetCurrentSpiritEnergy()
				== 100.0f);

	Provider.bAvailable = true;
	const auto Recovered =
		Fdemo_mapShanmenDivineSenseProductRoute::TryRetry(
			Controller,
			Fixture.Coordinator,
			Fixture.World,
			Fixture.Player,
			Rejected.Attempt,
			{ Fixture.Enemy },
			Provider);
	TestTrue(TEXT("same frozen attempt recovers after provider repair"),
		Recovered.IsAccepted() && !Recovered.IsReplay()
			&& Recovered.bReusedAttempt
			&& Recovered.ControllerResult.bReusedIntent
			&& Recovered.Attempt.GetIntent().GetIntentId() == IntentId
			&& Recovered.ControllerResult.Command.GetRouteCommandId()
				== CommandId);
	TestTrue(TEXT("recovery consumes no second identity and commits once"),
		Fixture.Coordinator.GetNextPlayerDivineSenseActivationSequence() == 2
			&& Controller.NumCapturedIntents() == 1
			&& Controller.GetSession().GetHost().GetCurrentSpiritEnergy()
				== 90.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapDivineSenseProductRouteAvailabilityTest,
	"Shanmen.0_0_10.Product.DivineSenseProductRoute.AvailabilityFence",
	ProductRouteFlags)

bool Fdemo_mapDivineSenseProductRouteAvailabilityTest::RunTest(
	const FString&)
{
	{
		FDivineSenseRouteFixture Fixture;
		if (!Fixture.Start(RouteRunC))
		{
			return false;
		}
		Fdemo_mapShanmenDivineSenseProductController Controller;
		check(BeginRoute(Fixture, Controller, 5.0f, 100.0f));
		FRouteEvidenceProvider Provider;
		const auto Unaffordable =
			Fdemo_mapShanmenDivineSenseProductRoute::TryUse(
				Controller,
				Fixture.Coordinator,
				Fixture.World,
				Fixture.Player,
				{},
				Provider);
		TestTrue(TEXT("insufficient SpiritEnergy rejects before identity"),
			Unaffordable.IsValid()
				&& Unaffordable.Status
					== Edemo_mapShanmenDivineSenseProductRouteStatus::
						AvailabilityRejected
				&& Fixture.Coordinator.
					GetNextPlayerDivineSenseActivationSequence() == 1
				&& Controller.NumCapturedIntents() == 0);
	}

	FDivineSenseRouteFixture Fixture;
	if (!Fixture.Start(RouteRunD))
	{
		return false;
	}
	Fdemo_mapShanmenDivineSenseProductController Controller;
	check(BeginRoute(Fixture, Controller, 200.0f, 200.0f));
	FRouteEvidenceProvider Provider;
	for (int32 Index = 0;
		Index < Fdemo_mapShanmenDivineSenseProductAuthority::
			CanonicalPulseCapacity();
		++Index)
	{
		const auto Applied =
			Fdemo_mapShanmenDivineSenseProductRoute::TryUse(
				Controller,
				Fixture.Coordinator,
				Fixture.World,
				Fixture.Player,
				{},
				Provider);
		if (!TestTrue(TEXT("bounded canonical use applies"),
			Applied.IsAccepted()))
		{
			return false;
		}
	}
	const uint64 SequenceAtCapacity =
		Fixture.Coordinator.GetNextPlayerDivineSenseActivationSequence();
	const auto Exhausted =
		Fdemo_mapShanmenDivineSenseProductRoute::TryUse(
			Controller,
			Fixture.Coordinator,
			Fixture.World,
			Fixture.Player,
			{},
			Provider);
	TestTrue(TEXT("Controller capacity rejects before a new reservation"),
		Exhausted.IsValid()
			&& Exhausted.Status
				== Edemo_mapShanmenDivineSenseProductRouteStatus::
					AvailabilityRejected
			&& SequenceAtCapacity
				== static_cast<uint64>(
					Fdemo_mapShanmenDivineSenseProductAuthority::
						CanonicalPulseCapacity() + 1)
			&& Fixture.Coordinator.
				GetNextPlayerDivineSenseActivationSequence()
					== SequenceAtCapacity
			&& Controller.NumCapturedIntents()
				== Fdemo_mapShanmenDivineSenseProductAuthority::
					CanonicalPulseCapacity());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapDivineSenseProductRouteBindingTeardownTest,
	"Shanmen.0_0_10.Product.DivineSenseProductRoute.CrossBindingAndTeardown",
	ProductRouteFlags)

bool Fdemo_mapDivineSenseProductRouteBindingTeardownTest::RunTest(
	const FString&)
{
	FDivineSenseRouteFixture First;
	FDivineSenseRouteFixture Second;
	if (!First.Start(RouteRunA) || !Second.Start(RouteRunB))
	{
		return false;
	}
	Fdemo_mapShanmenDivineSenseProductController FirstController;
	Fdemo_mapShanmenDivineSenseProductController SecondController;
	check(BeginRoute(First, FirstController));
	check(BeginRoute(Second, SecondController));
	FRouteEvidenceProvider Provider;
	const auto Applied = Fdemo_mapShanmenDivineSenseProductRoute::TryUse(
		FirstController,
		First.Coordinator,
		First.World,
		First.Player,
		{},
		Provider);
	if (!Applied.IsAccepted())
	{
		return false;
	}
	const int32 CallsBeforeCrossBinding = Provider.CallCount;
	const auto CrossBinding =
		Fdemo_mapShanmenDivineSenseProductRoute::TryRetry(
			SecondController,
			Second.Coordinator,
			Second.World,
			Second.Player,
			Applied.Attempt,
			{},
			Provider);
	TestTrue(TEXT("attempt cannot cross Controller or Run binding"),
		CrossBinding.IsValid() && !CrossBinding.IsAccepted()
			&& CrossBinding.Status
				== Edemo_mapShanmenDivineSenseProductRouteStatus::
					AttemptRejected
			&& Provider.CallCount == CallsBeforeCrossBinding
			&& Second.Coordinator.
				GetNextPlayerDivineSenseActivationSequence() == 1
			&& SecondController.NumCapturedIntents() == 0);

	const auto WrongRunEnd =
		Fdemo_mapShanmenDivineSenseProductRoute::TryEnd(
			FirstController, Second.Coordinator);
	TestTrue(TEXT("foreign Coordinator cannot close Controller"),
		WrongRunEnd.IsValid() && !WrongRunEnd.IsSuccess()
			&& FirstController.IsActive());
	const auto Ended = Fdemo_mapShanmenDivineSenseProductRoute::TryEnd(
		FirstController, First.Coordinator);
	TestTrue(TEXT("route closes its own Controller identity"),
		Ended.IsSuccess() && !Ended.IsReplay()
			&& FirstController.IsEnded());
	const auto ReplayEnd =
		Fdemo_mapShanmenDivineSenseProductRoute::TryEnd(
			FirstController, First.Coordinator);
	TestTrue(TEXT("exact route teardown replays"),
		ReplayEnd.IsSuccess() && ReplayEnd.IsReplay()
			&& ReplayEnd.Receipt.GetReceiptId()
				== Ended.Receipt.GetReceiptId());
	return true;
}

#endif
