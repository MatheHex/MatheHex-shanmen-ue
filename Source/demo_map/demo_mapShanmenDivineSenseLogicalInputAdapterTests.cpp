#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenDivineSenseLogicalInputAdapter.h"

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
	const EAutomationTestFlags LogicalInputFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;
	const FGuid LogicalRunA(
		0xD5900001, 0xD5900002, 0xD5900003, 0xD5900004);
	const FGuid LogicalRunB(
		0xD5910001, 0xD5910002, 0xD5910003, 0xD5910004);
	const FGuid LogicalRunC(
		0xD5920001, 0xD5920002, 0xD5920003, 0xD5920004);

	const Fdemo_mapM01EnemyDefinition* FindLogicalEnemyDefinition()
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

	Fdemo_mapEnemyEncounterIdentity MakeLogicalEncounterIdentity(
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

	struct FDivineSenseLogicalInputFixture
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
				FindLogicalEnemyDefinition();
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
					Player,
					TEXT("P199DivineSensePlayerRoot"),
					RF_Transient)
				: nullptr;
			Health = Player
				? NewObject<Udemo_mapPlayerHealthComponent>(
					Player,
					TEXT("P199DivineSensePlayerHealth"),
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
					TEXT("P199DivineSenseEnemyIdentity"),
					RF_Transient)
				: nullptr;
			if (!Enemy || !EnemyIdentity)
			{
				return false;
			}
			Enemy->AddInstanceComponent(EnemyIdentity);
			return EnemyIdentity->Configure(*Definition)
				&& Enemy->ConfigureEncounter(
					MakeLogicalEncounterIdentity(*Definition),
					Definition->Tuning,
					Definition->IsElite())
				&& Coordinator.TryBeginRun(
					RunId, Player, Health, Diagnostic)
				&& Coordinator.TryRegisterM01Enemy(Enemy, Diagnostic);
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

		~FDivineSenseLogicalInputFixture()
		{
			Stop();
		}
	};

	bool MakeLogicalOpeningSnapshot(
		const FGuid& OwnerEntityId,
		const float CurrentAmount,
		const float MaximumAmount,
		FShanmenActionResourceSnapshot& OutSnapshot)
	{
		FShanmenActionResourceAuthority Authority;
		return FShanmenActionResourceAuthority::TryCreate(
			OwnerEntityId,
			FShanmenCombatRuntimeNativeTags::ResourceSpiritEnergy(),
			CurrentAmount,
			MaximumAmount,
			11,
			Authority)
			&& Authority.TryCaptureSnapshot(OutSnapshot);
	}

	bool BeginLogicalProduct(
		FDivineSenseLogicalInputFixture& Fixture,
		Fdemo_mapShanmenDivineSenseProductController& Controller,
		Fdemo_mapShanmenDivineSenseLogicalInputAdapter& Adapter,
		const float CurrentAmount = 100.0f,
		const float MaximumAmount = 100.0f)
	{
		FShanmenActionResourceSnapshot Opening;
		return MakeLogicalOpeningSnapshot(
			Fixture.Coordinator.GetPlayerEntityId(),
			CurrentAmount,
			MaximumAmount,
			Opening)
			&& Fdemo_mapShanmenDivineSenseProductRoute::TryBegin(
				Controller,
				Fixture.Coordinator,
				Opening,
				Fixture.Diagnostic)
			&& Adapter.TryBegin(
				Controller, Fixture.Coordinator, Fixture.Diagnostic);
	}

	class FLogicalInputEvidenceProvider final
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
			OutEvidence.AuthorityRevision = 12;
			return true;
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapDivineSenseLogicalInputLifecycleTest,
	"Shanmen.0_0_10.Product.DivineSenseLogicalInputAdapter.LifecycleAvailability",
	LogicalInputFlags)

bool Fdemo_mapDivineSenseLogicalInputLifecycleTest::RunTest(
	const FString&)
{
	Fdemo_mapShanmenDivineSenseLogicalInputAdapter Adapter;
	Fdemo_mapShanmenDivineSenseProductController EmptyController;
	Fdemo_mapCombatRunCoordinator EmptyCoordinator;
	FString Diagnostic;
	Fdemo_mapShanmenDivineSenseLogicalInputAvailability Inactive;
	TestTrue(TEXT("canonical empty adapter projects inactive availability"),
		Adapter.IsValid() && Adapter.IsEmpty()
			&& Adapter.TryProjectAvailability(
				EmptyController, EmptyCoordinator, Inactive, Diagnostic)
			&& Inactive.IsValid()
			&& Inactive.GetState()
				== Edemo_mapShanmenDivineSenseLogicalInputAvailabilityState::
					Inactive
			&& !Inactive.CanUse() && !Inactive.CanRetry()
			&& !Inactive.CanCancel());
	const FGuid InactiveId = Inactive.GetProjectionId();

	FDivineSenseLogicalInputFixture Fixture;
	if (!Fixture.Start(LogicalRunA))
	{
		AddError(TEXT("Could not start P19.9 lifecycle fixture."));
		return false;
	}
	Fdemo_mapShanmenDivineSenseProductController Controller;
	if (!BeginLogicalProduct(Fixture, Controller, Adapter))
	{
		AddError(TEXT("Could not bind P19.9 lifecycle fixture."));
		return false;
	}
	Fdemo_mapShanmenDivineSenseLogicalInputAvailability Ready;
	TestTrue(TEXT("active canonical product exposes use as sole operation"),
		Adapter.IsActive() && Adapter.IsValid()
			&& Adapter.TryProjectAvailability(
				Controller, Fixture.Coordinator, Ready, Diagnostic)
			&& Ready.IsValid() && Ready.CanUse()
			&& !Ready.CanRetry() && !Ready.CanCancel()
			&& Ready.GetControllerId() == Controller.GetControllerId()
			&& Ready.GetRunId() == LogicalRunA
			&& Ready.GetProjectionId() != InactiveId);
	Fdemo_mapShanmenDivineSenseLogicalInputAvailability Repeated;
	TestTrue(TEXT("unchanged logical availability has deterministic identity"),
		Adapter.TryProjectAvailability(
			Controller, Fixture.Coordinator, Repeated, Diagnostic)
			&& Repeated.Matches(Ready));
	TestTrue(TEXT("same binding begin is idempotent"),
		Adapter.TryBegin(Controller, Fixture.Coordinator, Diagnostic)
			&& !Adapter.Reset());

	Fdemo_mapShanmenDivineSenseLogicalInputEndSummary Summary;
	TestTrue(TEXT("explicit teardown emits value-only binding evidence"),
		Adapter.TryEnd(
			Controller, Fixture.Coordinator, Summary, Diagnostic)
			&& Summary.IsValid() && !Summary.HadPendingRetry()
			&& Summary.GetControllerId() == Controller.GetControllerId()
			&& Summary.GetRunId() == LogicalRunA
			&& Adapter.IsValid() && Adapter.IsEmpty());
	Fdemo_mapShanmenDivineSenseLogicalInputAvailability Ended;
	TestTrue(TEXT("teardown restores canonical inactive projection"),
		Adapter.TryProjectAvailability(
			Controller, Fixture.Coordinator, Ended, Diagnostic)
			&& Ended.GetProjectionId() == InactiveId);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapDivineSenseLogicalInputUseTest,
	"Shanmen.0_0_10.Product.DivineSenseLogicalInputAdapter.SingleUse",
	LogicalInputFlags)

bool Fdemo_mapDivineSenseLogicalInputUseTest::RunTest(const FString&)
{
	FDivineSenseLogicalInputFixture Fixture;
	if (!Fixture.Start(LogicalRunA))
	{
		AddError(TEXT("Could not start P19.9 use fixture."));
		return false;
	}
	Fdemo_mapShanmenDivineSenseProductController Controller;
	Fdemo_mapShanmenDivineSenseLogicalInputAdapter Adapter;
	if (!BeginLogicalProduct(Fixture, Controller, Adapter))
	{
		AddError(TEXT("Could not bind P19.9 use fixture."));
		return false;
	}
	FLogicalInputEvidenceProvider Provider;
	const auto Applied = Adapter.TryUse(
		Controller,
		Fixture.Coordinator,
		Fixture.World,
		Fixture.Player,
		{ Fixture.Enemy },
		Provider);
	TestTrue(TEXT("one logical event delegates exactly once"),
		Applied.IsAccepted()
			&& Applied.Status
				== Edemo_mapShanmenDivineSenseLogicalInputStatus::Applied
			&& Applied.bProductRouteInvoked
			&& !Applied.bRetryAttempt
			&& !Applied.bPendingRetryStored
			&& Applied.AvailabilityBefore.CanUse()
			&& Applied.AvailabilityAfter.IsValid()
			&& Provider.CallCount == 1
			&& Fixture.Coordinator.
				GetNextPlayerDivineSenseActivationSequence() == 2
			&& Controller.NumCapturedIntents() == 1
			&& Controller.GetSession().GetHost().GetCurrentSpiritEnergy()
				== 90.0f);

	const auto MissingRetry = Adapter.TryRetry(
		Controller,
		Fixture.Coordinator,
		nullptr,
		nullptr,
		{},
		Provider);
	TestTrue(TEXT("retry without a retained attempt is side-effect free"),
		MissingRetry.IsValid()
			&& MissingRetry.Status
				== Edemo_mapShanmenDivineSenseLogicalInputStatus::
					RetryUnavailable
			&& MissingRetry.bRetryAttempt
			&& !MissingRetry.bProductRouteInvoked
			&& MissingRetry.AvailabilityBefore.Matches(
				MissingRetry.AvailabilityAfter)
			&& Provider.CallCount == 1
			&& Fixture.Coordinator.
				GetNextPlayerDivineSenseActivationSequence() == 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapDivineSenseLogicalInputPreflightTest,
	"Shanmen.0_0_10.Product.DivineSenseLogicalInputAdapter.PreflightRejection",
	LogicalInputFlags)

bool Fdemo_mapDivineSenseLogicalInputPreflightTest::RunTest(
	const FString&)
{
	FDivineSenseLogicalInputFixture Fixture;
	if (!Fixture.Start(LogicalRunA))
	{
		AddError(TEXT("Could not start P19.9 preflight fixture."));
		return false;
	}
	Fdemo_mapShanmenDivineSenseProductController Controller;
	Fdemo_mapShanmenDivineSenseLogicalInputAdapter Adapter;
	if (!BeginLogicalProduct(Fixture, Controller, Adapter))
	{
		AddError(TEXT("Could not bind P19.9 preflight fixture."));
		return false;
	}
	FLogicalInputEvidenceProvider Provider;
	const auto Rejected = Adapter.TryUse(
		Controller,
		Fixture.Coordinator,
		nullptr,
		Fixture.Player,
		{ Fixture.Enemy },
		Provider);
	TestTrue(TEXT("preflight rejection never becomes pending work"),
		Rejected.IsValid() && !Rejected.IsAccepted()
			&& Rejected.Status
				== Edemo_mapShanmenDivineSenseLogicalInputStatus::
					ProductRejected
			&& Rejected.bProductRouteInvoked
			&& !Rejected.bPendingRetryStored
			&& Rejected.ProductRoute.Status
				== Edemo_mapShanmenDivineSenseProductRouteStatus::
					LiveInputRejected
			&& Rejected.AvailabilityBefore.Matches(
				Rejected.AvailabilityAfter)
			&& Rejected.AvailabilityAfter.CanUse()
			&& !Adapter.HasPendingRetry()
			&& Provider.CallCount == 0
			&& Fixture.Coordinator.
				GetNextPlayerDivineSenseActivationSequence() == 1
			&& Controller.NumCapturedIntents() == 0);

	const auto Applied = Adapter.TryUse(
		Controller,
		Fixture.Coordinator,
		Fixture.World,
		Fixture.Player,
		{ Fixture.Enemy },
		Provider);
	TestTrue(TEXT("valid next event remains immediately usable"),
		Applied.IsAccepted() && Provider.CallCount == 1
			&& Fixture.Coordinator.
				GetNextPlayerDivineSenseActivationSequence() == 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapDivineSenseLogicalInputBusyRetryTest,
	"Shanmen.0_0_10.Product.DivineSenseLogicalInputAdapter.BusyRetry",
	LogicalInputFlags)

bool Fdemo_mapDivineSenseLogicalInputBusyRetryTest::RunTest(
	const FString&)
{
	FDivineSenseLogicalInputFixture Fixture;
	if (!Fixture.Start(LogicalRunA))
	{
		AddError(TEXT("Could not start P19.9 retry fixture."));
		return false;
	}
	Fdemo_mapShanmenDivineSenseProductController Controller;
	Fdemo_mapShanmenDivineSenseLogicalInputAdapter Adapter;
	if (!BeginLogicalProduct(Fixture, Controller, Adapter))
	{
		AddError(TEXT("Could not bind P19.9 retry fixture."));
		return false;
	}
	FLogicalInputEvidenceProvider Provider;
	Provider.bAvailable = false;
	const auto Rejected = Adapter.TryUse(
		Controller,
		Fixture.Coordinator,
		Fixture.World,
		Fixture.Player,
		{ Fixture.Enemy },
		Provider);
	if (!TestTrue(TEXT("typed provider rejection occupies retry slot"),
		Rejected.IsValid() && Rejected.RequiresRetry()
			&& Rejected.bProductRouteInvoked
			&& Rejected.bPendingRetryStored
			&& Adapter.HasPendingRetry()
			&& Adapter.GetPendingAttempt()
			&& Adapter.GetPendingAttempt()->Matches(
				Rejected.ProductRoute.Attempt)))
	{
		return false;
	}
	const FGuid IntentId =
		Rejected.ProductRoute.Attempt.GetIntent().GetIntentId();
	const FGuid CommandId =
		Rejected.ProductRoute.ControllerResult.Command.GetRouteCommandId();
	const FGuid PendingProjectionId =
		Rejected.AvailabilityAfter.GetProjectionId();
	TestTrue(TEXT("first rejection consumes one identity but no resource"),
		Provider.CallCount == 1
			&& Fixture.Coordinator.
				GetNextPlayerDivineSenseActivationSequence() == 2
			&& Controller.NumCapturedIntents() == 1
			&& Controller.GetSession().GetHost().GetCurrentSpiritEnergy()
				== 100.0f);

	Provider.bAvailable = true;
	const auto Busy = Adapter.TryUse(
		Controller,
		Fixture.Coordinator,
		Fixture.World,
		Fixture.Player,
		{ Fixture.Enemy },
		Provider);
	TestTrue(TEXT("pending retry blocks replacement before product access"),
		Busy.IsValid()
			&& Busy.Status
				== Edemo_mapShanmenDivineSenseLogicalInputStatus::Busy
			&& !Busy.bProductRouteInvoked
			&& Busy.AvailabilityBefore.GetProjectionId()
				== PendingProjectionId
			&& Provider.CallCount == 1
			&& Fixture.Coordinator.
				GetNextPlayerDivineSenseActivationSequence() == 2);

	Provider.bAvailable = false;
	const auto StillRejected = Adapter.TryRetry(
		Controller,
		Fixture.Coordinator,
		Fixture.World,
		Fixture.Player,
		{ Fixture.Enemy },
		Provider);
	TestTrue(TEXT("failed explicit retry retains exact attempt"),
		StillRejected.RequiresRetry()
			&& StillRejected.bRetryAttempt
			&& StillRejected.ProductRoute.bReusedAttempt
			&& StillRejected.ProductRoute.Attempt.GetIntent().GetIntentId()
				== IntentId
			&& StillRejected.ProductRoute.ControllerResult.Command.
				GetRouteCommandId() == CommandId
			&& StillRejected.AvailabilityAfter.GetProjectionId()
				== PendingProjectionId
			&& Provider.CallCount == 2
			&& Fixture.Coordinator.
				GetNextPlayerDivineSenseActivationSequence() == 2);

	Provider.bAvailable = true;
	const auto Recovered = Adapter.TryRetry(
		Controller,
		Fixture.Coordinator,
		Fixture.World,
		Fixture.Player,
		{ Fixture.Enemy },
		Provider);
	TestTrue(TEXT("explicit retry recovers without new identity"),
		Recovered.IsAccepted()
			&& Recovered.Status
				== Edemo_mapShanmenDivineSenseLogicalInputStatus::Applied
			&& Recovered.bRetryAttempt
			&& Recovered.bProductRouteInvoked
			&& !Recovered.bPendingRetryStored
			&& Recovered.ProductRoute.bReusedAttempt
			&& Recovered.ProductRoute.Attempt.GetIntent().GetIntentId()
				== IntentId
			&& Recovered.ProductRoute.ControllerResult.Command.
				GetRouteCommandId() == CommandId
			&& Recovered.AvailabilityAfter.CanUse()
			&& !Adapter.HasPendingRetry()
			&& Provider.CallCount == 3
			&& Fixture.Coordinator.
				GetNextPlayerDivineSenseActivationSequence() == 2
			&& Controller.GetSession().GetHost().GetCurrentSpiritEnergy()
				== 90.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapDivineSenseLogicalInputUnavailableTest,
	"Shanmen.0_0_10.Product.DivineSenseLogicalInputAdapter.AvailabilityFence",
	LogicalInputFlags)

bool Fdemo_mapDivineSenseLogicalInputUnavailableTest::RunTest(
	const FString&)
{
	FDivineSenseLogicalInputFixture Fixture;
	if (!Fixture.Start(LogicalRunC))
	{
		AddError(TEXT("Could not start P19.9 availability fixture."));
		return false;
	}
	Fdemo_mapShanmenDivineSenseProductController Controller;
	Fdemo_mapShanmenDivineSenseLogicalInputAdapter Adapter;
	if (!BeginLogicalProduct(Fixture, Controller, Adapter, 5.0f, 100.0f))
	{
		AddError(TEXT("Could not bind P19.9 availability fixture."));
		return false;
	}
	FLogicalInputEvidenceProvider Provider;
	Fdemo_mapShanmenDivineSenseLogicalInputAvailability Availability;
	TestTrue(TEXT("unaffordable product projects unavailable"),
		Adapter.TryProjectAvailability(
			Controller,
			Fixture.Coordinator,
			Availability,
			Fixture.Diagnostic)
			&& Availability.IsValid()
			&& Availability.GetState()
				== Edemo_mapShanmenDivineSenseLogicalInputAvailabilityState::
					ProductUnavailable
			&& !Availability.CanUse()
			&& !Availability.CanRetry());
	const auto Unavailable = Adapter.TryUse(
		Controller,
		Fixture.Coordinator,
		nullptr,
		nullptr,
		{},
		Provider);
	TestTrue(TEXT("availability fence precedes World and identity work"),
		Unavailable.IsValid()
			&& Unavailable.Status
				== Edemo_mapShanmenDivineSenseLogicalInputStatus::
					ProductUnavailable
			&& !Unavailable.bProductRouteInvoked
			&& Unavailable.AvailabilityBefore.Matches(
				Unavailable.AvailabilityAfter)
			&& Provider.CallCount == 0
			&& Fixture.Coordinator.
				GetNextPlayerDivineSenseActivationSequence() == 1
			&& Controller.NumCapturedIntents() == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapDivineSenseLogicalInputCancelTeardownTest,
	"Shanmen.0_0_10.Product.DivineSenseLogicalInputAdapter.CancelAndTeardown",
	LogicalInputFlags)

bool Fdemo_mapDivineSenseLogicalInputCancelTeardownTest::RunTest(
	const FString&)
{
	FDivineSenseLogicalInputFixture Fixture;
	if (!Fixture.Start(LogicalRunA))
	{
		AddError(TEXT("Could not start P19.9 cancellation fixture."));
		return false;
	}
	Fdemo_mapShanmenDivineSenseProductController Controller;
	Fdemo_mapShanmenDivineSenseLogicalInputAdapter Adapter;
	if (!BeginLogicalProduct(Fixture, Controller, Adapter))
	{
		AddError(TEXT("Could not bind P19.9 cancellation fixture."));
		return false;
	}
	FLogicalInputEvidenceProvider Provider;
	Provider.bAvailable = false;
	const auto FirstPending = Adapter.TryUse(
		Controller,
		Fixture.Coordinator,
		Fixture.World,
		Fixture.Player,
		{ Fixture.Enemy },
		Provider);
	Fdemo_mapShanmenDivineSenseLogicalInputCancellation Cancellation;
	TestTrue(TEXT("explicit cancel returns exact attempt evidence"),
		FirstPending.RequiresRetry()
			&& Adapter.TryCancelPending(
				Cancellation, Fixture.Diagnostic)
			&& Cancellation.IsValid()
			&& Cancellation.GetAttempt().Matches(
				FirstPending.ProductRoute.Attempt)
			&& !Adapter.HasPendingRetry()
			&& Fixture.Coordinator.
				GetNextPlayerDivineSenseActivationSequence() == 2);
	Fdemo_mapShanmenDivineSenseLogicalInputCancellation Missing;
	TestFalse(TEXT("empty retry slot cannot be cancelled twice"),
		Adapter.TryCancelPending(Missing, Fixture.Diagnostic));

	const auto SecondPending = Adapter.TryUse(
		Controller,
		Fixture.Coordinator,
		Fixture.World,
		Fixture.Player,
		{ Fixture.Enemy },
		Provider);
	Fdemo_mapShanmenDivineSenseLogicalInputEndSummary Summary;
	TestTrue(TEXT("teardown audits and clears unconsumed pending attempt"),
		SecondPending.RequiresRetry()
			&& Adapter.TryEnd(
				Controller,
				Fixture.Coordinator,
				Summary,
				Fixture.Diagnostic)
			&& Summary.IsValid() && Summary.HadPendingRetry()
			&& Summary.GetPendingAttempt().Matches(
				SecondPending.ProductRoute.Attempt)
			&& Adapter.IsValid() && Adapter.IsEmpty()
			&& Fixture.Coordinator.
				GetNextPlayerDivineSenseActivationSequence() == 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapDivineSenseLogicalInputBindingTest,
	"Shanmen.0_0_10.Product.DivineSenseLogicalInputAdapter.BindingFences",
	LogicalInputFlags)

bool Fdemo_mapDivineSenseLogicalInputBindingTest::RunTest(const FString&)
{
	FDivineSenseLogicalInputFixture FixtureA;
	FDivineSenseLogicalInputFixture FixtureB;
	if (!FixtureA.Start(LogicalRunA) || !FixtureB.Start(LogicalRunB))
	{
		AddError(TEXT("Could not start P19.9 binding fixtures."));
		return false;
	}
	Fdemo_mapShanmenDivineSenseProductController ControllerA;
	Fdemo_mapShanmenDivineSenseProductController ControllerB;
	Fdemo_mapShanmenDivineSenseLogicalInputAdapter Adapter;
	if (!BeginLogicalProduct(FixtureA, ControllerA, Adapter))
	{
		AddError(TEXT("Could not bind P19.9 primary fixture."));
		return false;
	}
	FShanmenActionResourceSnapshot OpeningB;
	if (!MakeLogicalOpeningSnapshot(
			FixtureB.Coordinator.GetPlayerEntityId(),
			100.0f,
			100.0f,
			OpeningB)
		|| !Fdemo_mapShanmenDivineSenseProductRoute::TryBegin(
			ControllerB,
			FixtureB.Coordinator,
			OpeningB,
			FixtureB.Diagnostic))
	{
		AddError(TEXT("Could not start P19.9 foreign product."));
		return false;
	}
	FLogicalInputEvidenceProvider Provider;
	const auto Foreign = Adapter.TryUse(
		ControllerB,
		FixtureB.Coordinator,
		FixtureB.World,
		FixtureB.Player,
		{ FixtureB.Enemy },
		Provider);
	TestTrue(TEXT("logical adapter cannot cross Controller or Run"),
		Foreign.IsValid()
			&& Foreign.Status
				== Edemo_mapShanmenDivineSenseLogicalInputStatus::
					BindingMismatch
			&& !Foreign.bProductRouteInvoked
			&& Provider.CallCount == 0
			&& FixtureA.Coordinator.
				GetNextPlayerDivineSenseActivationSequence() == 1
			&& FixtureB.Coordinator.
				GetNextPlayerDivineSenseActivationSequence() == 1);

	Provider.bAvailable = false;
	const auto Pending = Adapter.TryUse(
		ControllerA,
		FixtureA.Coordinator,
		FixtureA.World,
		FixtureA.Player,
		{ FixtureA.Enemy },
		Provider);
	const auto ForeignRetry = Adapter.TryRetry(
		ControllerB,
		FixtureB.Coordinator,
		FixtureB.World,
		FixtureB.Player,
		{ FixtureB.Enemy },
		Provider);
	TestTrue(TEXT("foreign retry cannot consume retained attempt"),
		Pending.RequiresRetry()
			&& ForeignRetry.IsValid()
			&& ForeignRetry.Status
				== Edemo_mapShanmenDivineSenseLogicalInputStatus::
					BindingMismatch
			&& ForeignRetry.bRetryAttempt
			&& !ForeignRetry.bProductRouteInvoked
			&& Adapter.HasPendingRetry()
			&& Provider.CallCount == 1
			&& FixtureA.Coordinator.
				GetNextPlayerDivineSenseActivationSequence() == 2
			&& FixtureB.Coordinator.
				GetNextPlayerDivineSenseActivationSequence() == 1);
	Fdemo_mapShanmenDivineSenseLogicalInputEndSummary WrongSummary;
	TestFalse(TEXT("foreign binding cannot teardown adapter"),
		Adapter.TryEnd(
			ControllerB,
			FixtureB.Coordinator,
			WrongSummary,
			FixtureB.Diagnostic));
	return true;
}

#endif
