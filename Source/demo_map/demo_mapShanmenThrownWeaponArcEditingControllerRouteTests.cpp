#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "Misc/AutomationTest.h"

#include "demo_mapGameMode.h"
#include "demo_mapPlayerController.h"

#include "Engine/Engine.h"
#include "Engine/EngineBaseTypes.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"

#include <limits>

namespace
{
	constexpr EAutomationTestFlags ArcEditingRouteFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;
	using ERequestStatus =
		Edemo_mapShanmenThrownWeaponInputChoiceInteractionRequestStatus;
	using ETrajectory =
		Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind;
	using FRequestResult =
		Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequestResult;

	struct FArcEditingControllerWorldFixture
	{
		UGameInstance* GameInstance = nullptr;
		UWorld* World = nullptr;
		Ademo_mapGameMode* GameMode = nullptr;
		Ademo_mapPlayerController* Controller = nullptr;
		ACharacter* Character = nullptr;
		bool bGameplayRestored = false;

		FArcEditingControllerWorldFixture()
		{
			if (!GEngine)
			{
				return;
			}
			GameInstance = NewObject<UGameInstance>(
				GEngine, NAME_None, RF_Transient);
			if (!GameInstance)
			{
				return;
			}
			GameInstance->AddToRoot();
			GameInstance->Init();

			World = NewObject<UWorld>(
				GetTransientPackage(), NAME_None, RF_Transient);
			if (!World)
			{
				return;
			}
			World->WorldType = EWorldType::GamePreview;
			FWorldContext& Context =
				GEngine->CreateNewWorldContext(EWorldType::GamePreview);
			Context.OwningGameInstance = GameInstance;
			Context.SetCurrentWorld(World);
			World->SetGameInstance(GameInstance);
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
			if (!World->SetGameMode(FURL()))
			{
				return;
			}
			GameMode = Cast<Ademo_mapGameMode>(World->GetAuthGameMode());
			Controller = World->SpawnActor<Ademo_mapPlayerController>();
			Character = World->SpawnActor<ACharacter>();
			if (GameMode && Controller && Character)
			{
				Controller->Possess(Character);
				Controller->InitInputSystem();
				bGameplayRestored =
					Controller->RestoreGameplayControlForNewRun();
			}
		}

		~FArcEditingControllerWorldFixture()
		{
			if (Controller && !Controller->IsActorBeingDestroyed())
			{
				Controller->UnPossess();
			}
			if (World)
			{
				World->DestroyWorld(false);
				if (GEngine)
				{
					GEngine->DestroyWorldContext(World);
				}
				World = nullptr;
			}
			if (GameInstance)
			{
				GameInstance->Shutdown();
				GameInstance->RemoveFromRoot();
				GameInstance->MarkAsGarbage();
				GameInstance = nullptr;
				CollectGarbage(RF_NoFlags);
			}
		}

		bool IsReady() const
		{
			return GameMode && Controller && Character && bGameplayRestored;
		}

		bool SelectArc() const
		{
			Fdemo_mapShanmenThrownWeaponInputChoiceIntent Intent;
			return Fdemo_mapShanmenThrownWeaponInputChoiceIntent::
				TryCaptureTrajectorySelection(
					ETrajectory::BallisticArc, Intent)
				&& Controller->RouteThrownWeaponInputChoiceIntent(Intent)
					.IsAccepted();
		}
	};

	bool IsRequestInvalid(const FRequestResult& Result)
	{
		return Result.IsValid() && !Result.IsAccepted()
			&& Result.GetStatus() == ERequestStatus::RequestInvalid
			&& !Result.GetRequest().IsValid()
			&& Result.GetInteractionReadCount() == 0
			&& Result.GetIntentRouteCount() == 0;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcEditingControllerTargetTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcEditingControllerRoute.TargetFreshReadRoundTrip",
	ArcEditingRouteFlags)

bool Fdemo_mapThrownWeaponArcEditingControllerTargetTest::RunTest(
	const FString&)
{
	FArcEditingControllerWorldFixture Fixture;
	if (!TestTrue(TEXT("Arc target controller fixture is ready"),
		Fixture.IsReady() && Fixture.SelectArc()))
	{
		return false;
	}

	const FRequestResult First =
		Fixture.Controller->RouteThrownWeaponArcTargetInteraction(
			FVector2D(3.0, 4.0));
	const auto FirstState =
		Fixture.GameMode->GetThrownWeaponInputChoiceState();
	const FRequestResult Repeated =
		Fixture.Controller->RouteThrownWeaponArcTargetInteraction(
			FVector2D(3.0, 4.0));
	const auto& RepeatedState =
		Fixture.GameMode->GetThrownWeaponInputChoiceState();

	TestTrue(TEXT("target entry captures and routes one canonical Arc edit"),
		First.IsAccepted() && First.GetRequest().IsValid()
			&& First.GetInteractionReadCount() == 1
			&& First.GetIntentRouteCount() == 1
			&& FirstState.GetRevision() == 2
			&& FirstState.HasArcTargetIntent()
			&& FirstState.GetArcTargetIntent().Equals(
				FVector2D(0.6, 0.8), 1.0e-6));
	TestTrue(TEXT("repeated target starts from a fresh read and remains a no-op"),
		Repeated.IsAccepted() && Repeated.GetRequest().IsValid()
			&& Repeated.GetInteractionReadCount() == 1
			&& Repeated.GetIntentRouteCount() == 1
			&& First.GetRequest().GetExpectedReadModelId()
				!= Repeated.GetRequest().GetExpectedReadModelId()
			&& RepeatedState.GetRevision() == 2
			&& RepeatedState.HasArcTargetIntent()
			&& RepeatedState.GetArcTargetIntent().Equals(
				FVector2D(0.6, 0.8), 1.0e-6));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcEditingControllerApexTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcEditingControllerRoute.ApexRoundTrip",
	ArcEditingRouteFlags)

bool Fdemo_mapThrownWeaponArcEditingControllerApexTest::RunTest(
	const FString&)
{
	FArcEditingControllerWorldFixture Fixture;
	if (!TestTrue(TEXT("Arc apex controller fixture is ready"),
		Fixture.IsReady() && Fixture.SelectArc()))
	{
		return false;
	}

	const FRequestResult Result =
		Fixture.Controller->RouteThrownWeaponArcApexAdjustmentInteraction(0.75);
	const auto& State = Fixture.GameMode->GetThrownWeaponInputChoiceState();
	TestTrue(TEXT("apex entry captures and routes exactly one Arc edit"),
		Result.IsAccepted() && Result.GetRequest().IsValid()
			&& Result.GetInteractionReadCount() == 1
			&& Result.GetIntentRouteCount() == 1
			&& State.GetRevision() == 2
			&& FMath::IsNearlyEqual(
				State.GetArcApexAdjustment(), 0.75, 1.0e-9));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcEditingControllerClearTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcEditingControllerRoute.ClearRoundTrip",
	ArcEditingRouteFlags)

bool Fdemo_mapThrownWeaponArcEditingControllerClearTest::RunTest(
	const FString&)
{
	FArcEditingControllerWorldFixture Fixture;
	if (!TestTrue(TEXT("Arc clear controller fixture is ready"),
		Fixture.IsReady() && Fixture.SelectArc()))
	{
		return false;
	}
	const FRequestResult Target =
		Fixture.Controller->RouteThrownWeaponArcTargetInteraction(
			FVector2D(1.0, 0.0));
	const FRequestResult Clear =
		Fixture.Controller->RouteThrownWeaponArcTargetClearInteraction();
	const auto& State = Fixture.GameMode->GetThrownWeaponInputChoiceState();

	TestTrue(TEXT("clear entry follows an accepted target and routes once"),
		Target.IsAccepted() && Clear.IsAccepted()
			&& Clear.GetRequest().IsValid()
			&& Clear.GetInteractionReadCount() == 1
			&& Clear.GetIntentRouteCount() == 1
			&& State.GetRevision() == 3
			&& !State.HasArcTargetIntent()
			&& State.GetArcTargetIntent().IsZero());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcEditingControllerCapabilityTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcEditingControllerRoute.CapabilityRejections",
	ArcEditingRouteFlags)

bool Fdemo_mapThrownWeaponArcEditingControllerCapabilityTest::RunTest(
	const FString&)
{
	FArcEditingControllerWorldFixture Fixture;
	if (!TestTrue(TEXT("Arc capability controller fixture is ready"),
		Fixture.IsReady()))
	{
		return false;
	}

	const FRequestResult StraightTarget =
		Fixture.Controller->RouteThrownWeaponArcTargetInteraction(
			FVector2D(1.0, 0.0));
	const FRequestResult StraightApex =
		Fixture.Controller->RouteThrownWeaponArcApexAdjustmentInteraction(0.25);
	const FRequestResult StraightClear =
		Fixture.Controller->RouteThrownWeaponArcTargetClearInteraction();
	const auto StraightState =
		Fixture.GameMode->GetThrownWeaponInputChoiceState();
	TestTrue(TEXT("Straight mode rejects all Arc-only entries before downstream reads"),
		IsRequestInvalid(StraightTarget)
			&& IsRequestInvalid(StraightApex)
			&& IsRequestInvalid(StraightClear)
			&& StraightState.GetTrajectoryKind() == ETrajectory::Straight
			&& StraightState.GetRevision() == 0);

	if (!TestTrue(TEXT("Arc selection is accepted after Straight rejections"),
		Fixture.SelectArc()))
	{
		return false;
	}
	const FRequestResult EmptyClear =
		Fixture.Controller->RouteThrownWeaponArcTargetClearInteraction();
	const auto& ArcState = Fixture.GameMode->GetThrownWeaponInputChoiceState();
	TestTrue(TEXT("clear without a visible target fails closed"),
		IsRequestInvalid(EmptyClear)
			&& ArcState.GetTrajectoryKind() == ETrajectory::BallisticArc
			&& ArcState.GetRevision() == 1
			&& !ArcState.HasArcTargetIntent());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcEditingControllerApexBoundaryTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcEditingControllerRoute.ApexBoundary",
	ArcEditingRouteFlags)

bool Fdemo_mapThrownWeaponArcEditingControllerApexBoundaryTest::RunTest(
	const FString&)
{
	FArcEditingControllerWorldFixture Fixture;
	if (!TestTrue(TEXT("Arc boundary controller fixture is ready"),
		Fixture.IsReady() && Fixture.SelectArc()))
	{
		return false;
	}

	const FRequestResult Maximum =
		Fixture.Controller->RouteThrownWeaponArcApexAdjustmentInteraction(1.0);
	const FRequestResult Overflow =
		Fixture.Controller->RouteThrownWeaponArcApexAdjustmentInteraction(0.25);
	const auto MaximumState =
		Fixture.GameMode->GetThrownWeaponInputChoiceState();
	const FRequestResult Return =
		Fixture.Controller->RouteThrownWeaponArcApexAdjustmentInteraction(-0.25);
	const auto& FinalState = Fixture.GameMode->GetThrownWeaponInputChoiceState();

	TestTrue(TEXT("positive apex limit is accepted and overflow fails closed"),
		Maximum.IsAccepted() && IsRequestInvalid(Overflow)
			&& MaximumState.GetRevision() == 2
			&& FMath::IsNearlyEqual(
				MaximumState.GetArcApexAdjustment(), 1.0, 1.0e-9));
	TestTrue(TEXT("opposite adjustment re-enters the visible apex range"),
		Return.IsAccepted() && Return.GetInteractionReadCount() == 1
			&& Return.GetIntentRouteCount() == 1
			&& FinalState.GetRevision() == 3
			&& FMath::IsNearlyEqual(
				FinalState.GetArcApexAdjustment(), 0.75, 1.0e-9));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcEditingControllerPayloadTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcEditingControllerRoute.InvalidPayloadRejections",
	ArcEditingRouteFlags)

bool Fdemo_mapThrownWeaponArcEditingControllerPayloadTest::RunTest(
	const FString&)
{
	FArcEditingControllerWorldFixture Fixture;
	if (!TestTrue(TEXT("Arc payload controller fixture is ready"),
		Fixture.IsReady() && Fixture.SelectArc()))
	{
		return false;
	}

	const auto Before = Fixture.GameMode->GetThrownWeaponInputChoiceState();
	const FRequestResult ZeroTarget =
		Fixture.Controller->RouteThrownWeaponArcTargetInteraction(
			FVector2D::ZeroVector);
	const FRequestResult ZeroApex =
		Fixture.Controller->RouteThrownWeaponArcApexAdjustmentInteraction(0.0);
	const FRequestResult NaNApex =
		Fixture.Controller->RouteThrownWeaponArcApexAdjustmentInteraction(
			std::numeric_limits<double>::quiet_NaN());
	const auto& After = Fixture.GameMode->GetThrownWeaponInputChoiceState();

	TestTrue(TEXT("invalid payloads become typed request rejections"),
		IsRequestInvalid(ZeroTarget)
			&& IsRequestInvalid(ZeroApex)
			&& IsRequestInvalid(NaNApex));
	TestTrue(TEXT("invalid payloads preserve the authoritative Arc choice"),
		After.Matches(Before) && After.GetRevision() == 1
			&& !After.HasArcTargetIntent()
			&& After.GetArcApexAdjustment() == 0.0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcEditingControllerInputLockTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcEditingControllerRoute.InputLock",
	ArcEditingRouteFlags)

bool Fdemo_mapThrownWeaponArcEditingControllerInputLockTest::RunTest(
	const FString&)
{
	FArcEditingControllerWorldFixture Fixture;
	if (!TestTrue(TEXT("Arc input-lock controller fixture is ready"),
		Fixture.IsReady() && Fixture.SelectArc()))
	{
		return false;
	}
	Fixture.Controller->BeginSettlementInputLock(nullptr);

	const FRequestResult Result =
		Fixture.Controller->RouteThrownWeaponArcTargetInteraction(
			FVector2D(3.0, 4.0));
	const auto& State = Fixture.GameMode->GetThrownWeaponInputChoiceState();
	TestTrue(TEXT("locked input reaches the existing downstream gameplay fence"),
		Result.IsValid() && !Result.IsAccepted()
			&& Result.WasRejectedByIntentRoute()
			&& Result.GetRequest().IsValid()
			&& Result.GetInteractionReadCount() == 1
			&& Result.GetIntentRouteCount() == 1
			&& State.GetTrajectoryKind() == ETrajectory::BallisticArc
			&& State.GetRevision() == 1
			&& !State.HasArcTargetIntent());
	return true;
}

#endif
