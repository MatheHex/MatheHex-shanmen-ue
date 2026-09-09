#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "Misc/AutomationTest.h"

#include "demo_mapGameMode.h"
#include "demo_mapInputActionRegistry.h"
#include "demo_mapInputBindingSettings.h"
#include "demo_mapPlayerController.h"
#include "demo_mapPlayerHealthComponent.h"
#include "Engine/Engine.h"
#include "Engine/EngineBaseTypes.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
	constexpr EAutomationTestFlags SwordQiPhysicalInputFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;
	const FGuid SwordQiPhysicalRun(0x24000001, 0, 0, 1);

	struct FScopedSwordQiInputConfig
	{
		FString Path;
		bool bExisted = false;
		TArray<uint8> Bytes;

		FScopedSwordQiInputConfig()
		{
			Path = Fdemo_mapInputBindingSettings::Get().GetConfigPath();
			bExisted = IFileManager::Get().FileExists(*Path);
			if (bExisted)
			{
				FFileHelper::LoadFileToArray(Bytes, *Path);
			}
		}

		~FScopedSwordQiInputConfig()
		{
			if (bExisted)
			{
				FFileHelper::SaveArrayToFile(Bytes, *Path);
			}
			else
			{
				IFileManager::Get().Delete(*Path, false, true);
			}
			Fdemo_mapInputBindingSettings::Get().Load();
		}
	};

	struct FSwordQiPhysicalWorldFixture
	{
		UGameInstance* GameInstance = nullptr;
		UWorld* World = nullptr;
		Ademo_mapGameMode* GameMode = nullptr;
		Ademo_mapPlayerController* Controller = nullptr;
		ACharacter* Character = nullptr;
		Udemo_mapPlayerHealthComponent* Health = nullptr;

		explicit FSwordQiPhysicalWorldFixture(const bool bCreateGameMode)
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
			if (bCreateGameMode && World->SetGameMode(FURL()))
			{
				GameMode = Cast<Ademo_mapGameMode>(World->GetAuthGameMode());
			}

			FActorSpawnParameters Parameters;
			Parameters.ObjectFlags |= RF_Transient;
			Parameters.SpawnCollisionHandlingOverride =
				ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			Controller = World->SpawnActor<Ademo_mapPlayerController>(
				Ademo_mapPlayerController::StaticClass(),
				FTransform::Identity,
				Parameters);
			Character = World->SpawnActor<ACharacter>(
				ACharacter::StaticClass(),
				FTransform::Identity,
				Parameters);
			Health = Character
				? NewObject<Udemo_mapPlayerHealthComponent>(
					Character,
					TEXT("P240SwordQiPhysicalHealth"),
					RF_Transient)
				: nullptr;
			if (Controller && Character)
			{
				Controller->Possess(Character);
				Controller->InitInputSystem();
				Controller->SetAutomationAimDirection(FVector::ForwardVector);
			}
		}

		~FSwordQiPhysicalWorldFixture()
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

		bool IsReady(const bool bExpectGameMode) const
		{
			return Controller && Character && Health
				&& Controller->HasPlayerInputForAutomation()
				&& Controller->HasInputComponentForAutomation()
				&& (!bExpectGameMode || GameMode != nullptr);
		}
	};

	FString MakeVersionNineConfig(const bool bOccupyB)
	{
		FString Text = TEXT("[ShanmenInputBindings]\nVersion=9\n");
		for (const Fdemo_mapInputActionDefinition& Action :
			Fdemo_mapInputActionRegistry::GetExactDefaultActions())
		{
			if (Action.ActionId == Fdemo_mapInputActionIds::SwordQi)
			{
				continue;
			}
			const FKey Key = bOccupyB
				&& Action.ActionId == Fdemo_mapInputActionIds::Interact
					? EKeys::B
					: Action.DefaultKey;
			Text += FString::Printf(
				TEXT("%s=%s\n"),
				*Action.ActionId.ToString(),
				*Key.GetFName().ToString());
		}
		return Text;
	}

	Fdemo_mapShanmenSwordQiInputResult MakeHostBusyInput(
		const FGuid& EventId)
	{
		Fdemo_mapShanmenSwordQiInputSample Sample;
		check(Fdemo_mapShanmenSwordQiInputSample::TryCapture(
			FVector(10.0, 20.0, 50.0),
			FVector::ForwardVector,
			Sample));
		Fdemo_mapShanmenSwordQiInputResult Input;
		Input.Status = Edemo_mapShanmenSwordQiInputStatus::ProductRejected;
		Input.bSpatialSampled = true;
		Input.bProductRouteInvoked = true;
		Input.InputEventId = EventId;
		Input.RunId = SwordQiPhysicalRun;
		Input.IntentId = Fdemo_mapShanmenSwordQiInputAdapter::MakeIntentId(
			SwordQiPhysicalRun,
			EventId);
		Input.Sample = Sample;
		Input.Product.Status =
			Edemo_mapShanmenSwordQiControllerStatus::RouteRejected;
		Input.Product.Route.Status =
			Edemo_mapShanmenSwordQiProductRouteStatus::HostBusy;
		Input.Diagnostic = TEXT("Synthetic active carrier fence.");
		return Input;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordQiPhysicalRegistryTest,
	"Shanmen.0_0_10.Product.SwordQiPhysicalInput.RegistryDefault",
	SwordQiPhysicalInputFlags)

bool Fdemo_mapSwordQiPhysicalRegistryTest::RunTest(const FString&)
{
	const Fdemo_mapInputActionDefinition* Action =
		Fdemo_mapInputActionRegistry::Find(Fdemo_mapInputActionIds::SwordQi);
	TestTrue(TEXT("registry is exact after adding physical Sword Qi"),
		Fdemo_mapInputActionRegistry::ValidateExactDefaults()
			&& Fdemo_mapInputActionRegistry::GetExactDefaultActions().Num()
				== 32);
	TestTrue(TEXT("Sword Qi owns a conflict-free press-only B default"),
		Action
			&& Action->DefaultKey == EKeys::B
			&& !Action->bRequiresReleasedEvent
			&& Action->DisplayLabel.Contains(TEXT("剑气")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordQiPhysicalMigrationTest,
	"Shanmen.0_0_10.Product.SwordQiPhysicalInput.VersionNineMigration",
	SwordQiPhysicalInputFlags)

bool Fdemo_mapSwordQiPhysicalMigrationTest::RunTest(const FString&)
{
	FScopedSwordQiInputConfig Config;
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(Config.Path), true);
	TestTrue(TEXT("version nine fixture written"),
		FFileHelper::SaveStringToFile(
			MakeVersionNineConfig(false), *Config.Path));
	const Fdemo_mapInputBindingResult Free =
		Fdemo_mapInputBindingSettings::Get().Load();
	TestTrue(TEXT("free B becomes the migrated Sword Qi binding"),
		Free.IsSuccess()
			&& Fdemo_mapInputBindingSettings::Get().GetBindings().Num() == 32
			&& Fdemo_mapInputBindingSettings::Get().GetKey(
				Fdemo_mapInputActionIds::SwordQi) == EKeys::B);

	TestTrue(TEXT("occupied-B fixture written"),
		FFileHelper::SaveStringToFile(
			MakeVersionNineConfig(true), *Config.Path));
	const Fdemo_mapInputBindingResult Conflict =
		Fdemo_mapInputBindingSettings::Get().Load();
	FString Diagnostic;
	TestTrue(TEXT("migration preserves B override and allocates free G"),
		Conflict.IsSuccess()
			&& Fdemo_mapInputBindingSettings::Get().GetKey(
				Fdemo_mapInputActionIds::Interact) == EKeys::B
			&& Fdemo_mapInputBindingSettings::Get().GetKey(
				Fdemo_mapInputActionIds::SwordQi) == EKeys::G
			&& Fdemo_mapInputBindingSettings::ValidateBindings(
				Fdemo_mapInputBindingSettings::Get().GetBindings(), Diagnostic));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordQiPhysicalPressTest,
	"Shanmen.0_0_10.Product.SwordQiPhysicalInput.PressOnlyBinding",
	SwordQiPhysicalInputFlags)

bool Fdemo_mapSwordQiPhysicalPressTest::RunTest(const FString&)
{
	FScopedSwordQiInputConfig Config;
	Fdemo_mapInputBindingSettings::Get().RestoreDefaults();
	FSwordQiPhysicalWorldFixture Fixture(false);
	TestTrue(TEXT("physical Sword Qi fixture is ready"),
		Fixture.IsReady(false));
	if (!Fixture.IsReady(false))
	{
		return false;
	}
	const uint64 Before = Fixture.Controller
		->GetSwordQiInputInvocationCountForAutomation();
	TestTrue(TEXT("B press reaches the Sword Qi handler"),
		Fixture.Controller->DispatchAutomationKeyPressed(EKeys::B));
	TestTrue(TEXT("one press invokes once and preserves missing GameMode fence"),
		Fixture.Controller->GetSwordQiInputInvocationCountForAutomation()
				== Before + 1
			&& Fixture.Controller
				->GetLastSwordQiInputResultForAutomation().Status
					== Edemo_mapShanmenSwordQiAvailabilityCommandStatus::
						ProjectionUnavailable
			&& Fixture.Controller->IsSwordQiInputFeedbackActive());
	Fixture.Controller->DispatchAutomationKeyReleased(EKeys::B);
	TestEqual(TEXT("release owns no duplicate Sword Qi binding"),
		Fixture.Controller->GetSwordQiInputInvocationCountForAutomation(),
		Before + 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordQiPhysicalRemapAndLockTest,
	"Shanmen.0_0_10.Product.SwordQiPhysicalInput.LiveRemapAndLock",
	SwordQiPhysicalInputFlags)

bool Fdemo_mapSwordQiPhysicalRemapAndLockTest::RunTest(const FString&)
{
	FScopedSwordQiInputConfig Config;
	Fdemo_mapInputBindingSettings::Get().RestoreDefaults();
	FSwordQiPhysicalWorldFixture Fixture(false);
	TestTrue(TEXT("Sword Qi remap fixture is ready"), Fixture.IsReady(false));
	if (!Fixture.IsReady(false))
	{
		return false;
	}
	const Fdemo_mapInputBindingResult Remap =
		Fdemo_mapInputBindingSettings::Get().ApplyOverrideWithSwap(
			Fdemo_mapInputActionIds::SwordQi,
			EKeys::Y);
	Fixture.Controller->RebuildProductInputBindings();
	const uint64 Before = Fixture.Controller
		->GetSwordQiInputInvocationCountForAutomation();
	Fixture.Controller->DispatchAutomationKey(EKeys::B);
	Fixture.Controller->DispatchAutomationKey(EKeys::Y);
	TestTrue(TEXT("live remap detaches B and activates Y exactly once"),
		Remap.IsSuccess()
			&& Fdemo_mapInputBindingSettings::Get().GetKey(
				Fdemo_mapInputActionIds::SwordQi) == EKeys::Y
			&& Fixture.Controller
				->GetSwordQiInputInvocationCountForAutomation() == Before + 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordQiPhysicalIssueRetryTest,
	"Shanmen.0_0_10.Product.SwordQiPhysicalInput.IssueRetryAndLock",
	SwordQiPhysicalInputFlags)

bool Fdemo_mapSwordQiPhysicalIssueRetryTest::RunTest(const FString&)
{
	FScopedSwordQiInputConfig Config;
	Fdemo_mapInputBindingSettings::Get().RestoreDefaults();
	FSwordQiPhysicalWorldFixture Fixture(true);
	TestTrue(TEXT("Sword Qi routed fixture is ready"), Fixture.IsReady(true));
	if (!Fixture.IsReady(true))
	{
		return false;
	}
	FString Diagnostic;
	TestTrue(TEXT("combat coordinator and command owner bind one fixture Run"),
		Fixture.GameMode->CombatRunCoordinator.TryBeginRun(
			SwordQiPhysicalRun,
			Fixture.Character,
			Fixture.Health,
			Diagnostic)
			&& Fixture.GameMode->SwordQiCommandEventOwner.TryBegin(
				SwordQiPhysicalRun,
				Diagnostic));
	if (!Fixture.GameMode->CombatRunCoordinator.IsReady()
		|| !Fixture.GameMode->SwordQiCommandEventOwner.IsActive())
	{
		AddError(Diagnostic);
		return false;
	}

	const uint64 Before = Fixture.Controller
		->GetSwordQiInputInvocationCountForAutomation();
	Fixture.Controller->DispatchAutomationKey(EKeys::B);
	const Fdemo_mapShanmenSwordQiAvailabilityCommandResult Issue =
		Fixture.Controller->GetLastSwordQiInputResultForAutomation();
	TestTrue(TEXT("ready press dispatches Issue through P18.10"),
		Issue.IsDispatched()
			&& Issue.Kind
				== Edemo_mapShanmenSwordQiAvailabilityCommandKind::Issue
			&& Issue.Before.CanIssue()
			&& Issue.After.CanIssue()
			&& Issue.CommandEvent.Status
				== Edemo_mapShanmenSwordQiCommandEventStatus::InputRejected
			&& Issue.CommandEvent.Input.Status
				== Edemo_mapShanmenSwordQiInputStatus::ProductRouteUnavailable
			&& !Issue.CommandEvent.bEventCommitted);

	const Fdemo_mapShanmenSwordQiCommandEventResult Pending =
		Fixture.GameMode->SwordQiCommandEventOwner.TryIssue(
			[](const FGuid& EventId)
			{
				return MakeHostBusyInput(EventId);
			});
	TestTrue(TEXT("fixture retains one immutable HostBusy request"),
		Pending.bPendingRetryStored
			&& Fixture.GameMode->SwordQiCommandEventOwner.HasPendingRetry());
	Fixture.Controller->DispatchAutomationKey(EKeys::B);
	const Fdemo_mapShanmenSwordQiAvailabilityCommandResult Retry =
		Fixture.Controller->GetLastSwordQiInputResultForAutomation();
	TestTrue(TEXT("next press retries the frozen request without resampling"),
		Retry.IsDispatched()
			&& Retry.Kind
				== Edemo_mapShanmenSwordQiAvailabilityCommandKind::Retry
			&& Retry.CommandEvent.bPendingRetryAttempt
			&& Retry.CommandEvent.Request.Matches(Pending.Request)
			&& Retry.CommandEvent.Input.Status
				== Edemo_mapShanmenSwordQiInputStatus::ProductRouteUnavailable
			&& Retry.CommandEvent.bPendingRetryStored
			&& Retry.After.CanRetry()
			&& Retry.Before.GetProjectionId()
				== Retry.After.GetProjectionId()
			&& Fixture.GameMode->SwordQiCommandEventOwner.HasPendingRetry());

	Fdemo_mapShanmenSwordQiPendingRetryCancellation Cancellation;
	TestTrue(TEXT("fixture returns to issue-ready through the existing cancel route"),
		Fixture.GameMode->SwordQiCommandEventOwner.TryCancelPending(
			Cancellation,
			Diagnostic));

	Fixture.Controller->BeginSettlementInputLock(nullptr);
	Fixture.Controller->DispatchAutomationKey(EKeys::B);
	const Fdemo_mapShanmenSwordQiAvailabilityCommandResult Locked =
		Fixture.Controller->GetLastSwordQiInputResultForAutomation();
	TestTrue(TEXT("physical pulse reaches the shared UI lock without consuming identity"),
		Locked.IsDispatched()
			&& Locked.Kind
				== Edemo_mapShanmenSwordQiAvailabilityCommandKind::Issue
			&& Locked.CommandEvent.Input.Status
				== Edemo_mapShanmenSwordQiInputStatus::GameplayBlocked
			&& !Locked.CommandEvent.bEventCommitted
			&& Locked.Before.GetProjectionId()
				== Locked.After.GetProjectionId()
			&& Fixture.Controller
				->GetSwordQiInputInvocationCountForAutomation() == Before + 3);

	Fixture.GameMode->SwordQiCommandEventOwner.Reset();
	Fixture.GameMode->CombatRunCoordinator.TryEndRun(
		SwordQiPhysicalRun,
		Diagnostic);
	return true;
}

#endif
