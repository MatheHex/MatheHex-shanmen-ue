#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "Misc/AutomationTest.h"

#include "demo_mapGameMode.h"
#include "demo_mapInputActionRegistry.h"
#include "demo_mapInputBindingSettings.h"
#include "demo_mapPlayerController.h"

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
	constexpr EAutomationTestFlags ArcEditingPhysicalInputFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;
	using ERequestStatus =
		Edemo_mapShanmenThrownWeaponInputChoiceInteractionRequestStatus;
	using ETrajectory =
		Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind;

	bool IsArcEditingAction(const FName ActionId)
	{
		return ActionId == Fdemo_mapInputActionIds::ThrownWeaponArcTargetSet
			|| ActionId == Fdemo_mapInputActionIds::ThrownWeaponArcApexIncrease
			|| ActionId == Fdemo_mapInputActionIds::ThrownWeaponArcApexDecrease
			|| ActionId == Fdemo_mapInputActionIds::ThrownWeaponArcTargetClear;
	}

	struct FScopedArcEditingInputConfig
	{
		FString Path;
		bool bExisted = false;
		TArray<uint8> Bytes;

		FScopedArcEditingInputConfig()
		{
			Path = Fdemo_mapInputBindingSettings::Get().GetConfigPath();
			bExisted = IFileManager::Get().FileExists(*Path);
			if (bExisted)
			{
				FFileHelper::LoadFileToArray(Bytes, *Path);
			}
		}

		~FScopedArcEditingInputConfig()
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

	FString MakeVersionFiveConfig(const bool bOccupyTargetDefault)
	{
		FString Text = TEXT("[ShanmenInputBindings]\nVersion=5\n");
		for (const Fdemo_mapInputActionDefinition& Action :
			Fdemo_mapInputActionRegistry::GetExactDefaultActions())
		{
			if (IsArcEditingAction(Action.ActionId))
			{
				continue;
			}
			const FKey Key = bOccupyTargetDefault
				&& Action.ActionId == Fdemo_mapInputActionIds::Interact
					? EKeys::MiddleMouseButton
					: Action.DefaultKey;
			Text += FString::Printf(
				TEXT("%s=%s\n"),
				*Action.ActionId.ToString(),
				*Key.GetFName().ToString());
		}
		return Text;
	}

	struct FArcEditingPhysicalWorldFixture
	{
		UGameInstance* GameInstance = nullptr;
		UWorld* World = nullptr;
		Ademo_mapGameMode* GameMode = nullptr;
		Ademo_mapPlayerController* Controller = nullptr;
		ACharacter* Character = nullptr;
		bool bGameplayRestored = false;

		FArcEditingPhysicalWorldFixture()
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

		~FArcEditingPhysicalWorldFixture()
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

}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcEditingPhysicalRegistryTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcEditingPhysicalInput.RegistryDefaults",
	ArcEditingPhysicalInputFlags)

bool Fdemo_mapThrownWeaponArcEditingPhysicalRegistryTest::RunTest(
	const FString&)
{
	const auto* Target = Fdemo_mapInputActionRegistry::Find(
		Fdemo_mapInputActionIds::ThrownWeaponArcTargetSet);
	const auto* Increase = Fdemo_mapInputActionRegistry::Find(
		Fdemo_mapInputActionIds::ThrownWeaponArcApexIncrease);
	const auto* Decrease = Fdemo_mapInputActionRegistry::Find(
		Fdemo_mapInputActionIds::ThrownWeaponArcApexDecrease);
	const auto* Clear = Fdemo_mapInputActionRegistry::Find(
		Fdemo_mapInputActionIds::ThrownWeaponArcTargetClear);
	TestTrue(TEXT("Arc editing defaults are exact and conflict-free"),
		Fdemo_mapInputActionRegistry::ValidateExactDefaults()
			&& Fdemo_mapInputActionRegistry::GetExactDefaultActions().Num() == 29);
	TestTrue(TEXT("pointer target is a remappable middle-button press"),
		Target && Target->DefaultKey == EKeys::MiddleMouseButton
			&& !Target->bRequiresReleasedEvent
			&& Target->DisplayLabel.Contains(TEXT("目标")));
	TestTrue(TEXT("apex and clear actions use distinct semantic keys"),
		Increase && Increase->DefaultKey == EKeys::RightBracket
			&& !Increase->bRequiresReleasedEvent
			&& Decrease && Decrease->DefaultKey == EKeys::LeftBracket
			&& !Decrease->bRequiresReleasedEvent
			&& Clear && Clear->DefaultKey == EKeys::Delete
			&& !Clear->bRequiresReleasedEvent);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcEditingPhysicalMigrationTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcEditingPhysicalInput.VersionFiveMigration",
	ArcEditingPhysicalInputFlags)

bool Fdemo_mapThrownWeaponArcEditingPhysicalMigrationTest::RunTest(
	const FString&)
{
	FScopedArcEditingInputConfig Config;
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(Config.Path), true);
	TestTrue(TEXT("version five input fixture written"),
		FFileHelper::SaveStringToFile(
			MakeVersionFiveConfig(false), *Config.Path));
	const Fdemo_mapInputBindingResult Result =
		Fdemo_mapInputBindingSettings::Get().Load();
	TestTrue(TEXT("v5 receives all four free v6 Arc defaults"),
		Result.IsSuccess()
			&& Fdemo_mapInputBindingSettings::Get().GetBindings().Num() == 29
			&& Fdemo_mapInputBindingSettings::Get().GetKey(
				Fdemo_mapInputActionIds::ThrownWeaponArcTargetSet)
				== EKeys::MiddleMouseButton
			&& Fdemo_mapInputBindingSettings::Get().GetKey(
				Fdemo_mapInputActionIds::ThrownWeaponArcApexIncrease)
				== EKeys::RightBracket
			&& Fdemo_mapInputBindingSettings::Get().GetKey(
				Fdemo_mapInputActionIds::ThrownWeaponArcApexDecrease)
				== EKeys::LeftBracket
			&& Fdemo_mapInputBindingSettings::Get().GetKey(
				Fdemo_mapInputActionIds::ThrownWeaponArcTargetClear)
				== EKeys::Delete);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcEditingPhysicalMigrationConflictTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcEditingPhysicalInput.VersionFiveConflictMigration",
	ArcEditingPhysicalInputFlags)

bool Fdemo_mapThrownWeaponArcEditingPhysicalMigrationConflictTest::RunTest(
	const FString&)
{
	FScopedArcEditingInputConfig Config;
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(Config.Path), true);
	TestTrue(TEXT("occupied middle-button v5 fixture written"),
		FFileHelper::SaveStringToFile(
			MakeVersionFiveConfig(true), *Config.Path));
	const Fdemo_mapInputBindingResult Result =
		Fdemo_mapInputBindingSettings::Get().Load();
	const auto& Bindings = Fdemo_mapInputBindingSettings::Get().GetBindings();
	FString Diagnostic;
	TestTrue(TEXT("migration preserves the old override and fills unique Arc keys"),
		Result.IsSuccess()
			&& Fdemo_mapInputBindingSettings::Get().GetKey(
				Fdemo_mapInputActionIds::Interact) == EKeys::MiddleMouseButton
			&& Fdemo_mapInputBindingSettings::Get().GetKey(
				Fdemo_mapInputActionIds::ThrownWeaponArcTargetSet)
				!= EKeys::MiddleMouseButton
			&& Fdemo_mapInputBindingSettings::ValidateBindings(
				Bindings, Diagnostic));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcEditingPhysicalTargetTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcEditingPhysicalInput.PointerTargetRoundTrip",
	ArcEditingPhysicalInputFlags)

bool Fdemo_mapThrownWeaponArcEditingPhysicalTargetTest::RunTest(
	const FString&)
{
	FScopedArcEditingInputConfig Config;
	Fdemo_mapInputBindingSettings::Get().RestoreDefaults();
	FArcEditingPhysicalWorldFixture Fixture;
	if (!TestTrue(TEXT("pointer target fixture is ready"),
		Fixture.IsReady() && Fixture.SelectArc()))
	{
		return false;
	}
	Fixture.Controller->SetAutomationAimDirection(FVector(3.0, 4.0, 7.0));
	const uint64 Before = Fixture.Controller->
		GetThrownWeaponArcEditingInputInvocationCountForAutomation();
	const bool bDispatched = Fixture.Controller->DispatchAutomationKeyPressed(
		EKeys::MiddleMouseButton);
	const auto& Result = Fixture.Controller->
		GetLastThrownWeaponArcEditingInputResultForAutomation();
	const auto& State = Fixture.GameMode->GetThrownWeaponInputChoiceState();
	TestTrue(TEXT("middle button routes one normalized existing pointer aim"),
		bDispatched
			&& Fixture.Controller->
				GetThrownWeaponArcEditingInputInvocationCountForAutomation()
					== Before + 1
			&& Result.IsAccepted()
			&& Result.GetInteractionReadCount() == 1
			&& Result.GetIntentRouteCount() == 1
			&& State.GetRevision() == 2
			&& State.HasArcTargetIntent()
			&& State.GetArcTargetIntent().Equals(
				FVector2D(0.6, 0.8), 1.0e-6));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcEditingPhysicalApexTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcEditingPhysicalInput.ApexStepRoundTrip",
	ArcEditingPhysicalInputFlags)

bool Fdemo_mapThrownWeaponArcEditingPhysicalApexTest::RunTest(
	const FString&)
{
	FScopedArcEditingInputConfig Config;
	Fdemo_mapInputBindingSettings::Get().RestoreDefaults();
	FArcEditingPhysicalWorldFixture Fixture;
	if (!TestTrue(TEXT("apex fixture is ready"),
		Fixture.IsReady() && Fixture.SelectArc()))
	{
		return false;
	}
	const uint64 Before = Fixture.Controller->
		GetThrownWeaponArcEditingInputInvocationCountForAutomation();
	const bool bIncrease = Fixture.Controller->DispatchAutomationKeyPressed(
		EKeys::RightBracket);
	const auto Increased = Fixture.GameMode->GetThrownWeaponInputChoiceState();
	const bool bDecrease = Fixture.Controller->DispatchAutomationKeyPressed(
		EKeys::LeftBracket);
	const auto& Restored = Fixture.GameMode->GetThrownWeaponInputChoiceState();
	TestTrue(TEXT("bracket presses route one canonical quarter-step each"),
		bIncrease && bDecrease
			&& Fixture.Controller->
				GetThrownWeaponArcEditingInputInvocationCountForAutomation()
					== Before + 2
			&& Increased.GetRevision() == 2
			&& FMath::IsNearlyEqual(
				Increased.GetArcApexAdjustment(), 0.25, 1.0e-9)
			&& Restored.GetRevision() == 3
			&& FMath::IsNearlyZero(
				Restored.GetArcApexAdjustment(), 1.0e-9));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcEditingPhysicalClearTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcEditingPhysicalInput.ClearRoundTrip",
	ArcEditingPhysicalInputFlags)

bool Fdemo_mapThrownWeaponArcEditingPhysicalClearTest::RunTest(
	const FString&)
{
	FScopedArcEditingInputConfig Config;
	Fdemo_mapInputBindingSettings::Get().RestoreDefaults();
	FArcEditingPhysicalWorldFixture Fixture;
	if (!TestTrue(TEXT("clear fixture is ready"),
		Fixture.IsReady() && Fixture.SelectArc()))
	{
		return false;
	}
	Fixture.Controller->SetAutomationAimDirection(FVector(1.0, 0.0, 0.0));
	const uint64 Before = Fixture.Controller->
		GetThrownWeaponArcEditingInputInvocationCountForAutomation();
	const bool bSet = Fixture.Controller->DispatchAutomationKeyPressed(
		EKeys::MiddleMouseButton);
	const bool bClear = Fixture.Controller->DispatchAutomationKeyPressed(
		EKeys::Delete);
	const auto& Result = Fixture.Controller->
		GetLastThrownWeaponArcEditingInputResultForAutomation();
	const auto& State = Fixture.GameMode->GetThrownWeaponInputChoiceState();
	TestTrue(TEXT("Delete clears the pointer target through the P20.26 route"),
		bSet && bClear
			&& Fixture.Controller->
				GetThrownWeaponArcEditingInputInvocationCountForAutomation()
					== Before + 2
			&& Result.IsAccepted()
			&& State.GetRevision() == 3
			&& !State.HasArcTargetIntent()
			&& State.GetArcTargetIntent().IsZero());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcEditingPhysicalInvalidAimTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcEditingPhysicalInput.InvalidAimFailsClosed",
	ArcEditingPhysicalInputFlags)

bool Fdemo_mapThrownWeaponArcEditingPhysicalInvalidAimTest::RunTest(
	const FString&)
{
	FScopedArcEditingInputConfig Config;
	Fdemo_mapInputBindingSettings::Get().RestoreDefaults();
	FArcEditingPhysicalWorldFixture Fixture;
	if (!TestTrue(TEXT("invalid aim fixture is ready"),
		Fixture.IsReady() && Fixture.SelectArc()))
	{
		return false;
	}
	Fixture.Controller->SetAutomationAimDirection(FVector::ZeroVector);
	const uint64 Before = Fixture.Controller->
		GetThrownWeaponArcEditingInputInvocationCountForAutomation();
	Fixture.Controller->DispatchAutomationKeyPressed(EKeys::MiddleMouseButton);
	const auto& Result = Fixture.Controller->
		GetLastThrownWeaponArcEditingInputResultForAutomation();
	const auto& State = Fixture.GameMode->GetThrownWeaponInputChoiceState();
	TestTrue(TEXT("zero pointer aim becomes one typed request rejection"),
		Fixture.Controller->
			GetThrownWeaponArcEditingInputInvocationCountForAutomation()
				== Before + 1
			&& Result.IsValid() && !Result.IsAccepted()
			&& Result.GetStatus() == ERequestStatus::RequestInvalid
			&& Result.GetInteractionReadCount() == 0
			&& Result.GetIntentRouteCount() == 0
			&& State.GetRevision() == 1
			&& !State.HasArcTargetIntent());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcEditingPhysicalRemapTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcEditingPhysicalInput.LiveRemap",
	ArcEditingPhysicalInputFlags)

bool Fdemo_mapThrownWeaponArcEditingPhysicalRemapTest::RunTest(
	const FString&)
{
	FScopedArcEditingInputConfig Config;
	Fdemo_mapInputBindingSettings::Get().RestoreDefaults();
	FArcEditingPhysicalWorldFixture Fixture;
	if (!TestTrue(TEXT("remap fixture is ready"),
		Fixture.IsReady() && Fixture.SelectArc()))
	{
		return false;
	}
	Fixture.Controller->SetAutomationAimDirection(FVector(0.0, 1.0, 0.0));
	const Fdemo_mapInputBindingResult Remap =
		Fdemo_mapInputBindingSettings::Get().ApplyOverrideWithSwap(
			Fdemo_mapInputActionIds::ThrownWeaponArcTargetSet,
			EKeys::H);
	Fixture.Controller->RebuildProductInputBindings();
	const uint64 Before = Fixture.Controller->
		GetThrownWeaponArcEditingInputInvocationCountForAutomation();
	Fixture.Controller->DispatchAutomationKeyPressed(EKeys::MiddleMouseButton);
	const uint64 AfterOld = Fixture.Controller->
		GetThrownWeaponArcEditingInputInvocationCountForAutomation();
	Fixture.Controller->DispatchAutomationKeyPressed(EKeys::H);
	FString Persisted;
	FFileHelper::LoadFileToString(Persisted, *Config.Path);
	const auto& State = Fixture.GameMode->GetThrownWeaponInputChoiceState();
	TestTrue(TEXT("live rebuild detaches the old target key and activates H"),
		Remap.IsSuccess()
			&& AfterOld == Before
			&& Fixture.Controller->
				GetThrownWeaponArcEditingInputInvocationCountForAutomation()
					== Before + 1
			&& Persisted.Contains(TEXT("Version=7"))
			&& Fdemo_mapInputBindingSettings::Get().GetKey(
				Fdemo_mapInputActionIds::ThrownWeaponArcTargetSet) == EKeys::H
			&& State.GetRevision() == 2
			&& State.HasArcTargetIntent()
			&& State.GetArcTargetIntent().Equals(
				FVector2D(0.0, 1.0), 1.0e-6));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcEditingPhysicalInputLockTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcEditingPhysicalInput.InputLock",
	ArcEditingPhysicalInputFlags)

bool Fdemo_mapThrownWeaponArcEditingPhysicalInputLockTest::RunTest(
	const FString&)
{
	FScopedArcEditingInputConfig Config;
	Fdemo_mapInputBindingSettings::Get().RestoreDefaults();
	FArcEditingPhysicalWorldFixture Fixture;
	if (!TestTrue(TEXT("input lock fixture is ready"),
		Fixture.IsReady() && Fixture.SelectArc()))
	{
		return false;
	}
	Fixture.Controller->SetAutomationAimDirection(FVector(1.0, 0.0, 0.0));
	Fixture.Controller->BeginSettlementInputLock(nullptr);
	const uint64 Before = Fixture.Controller->
		GetThrownWeaponArcEditingInputInvocationCountForAutomation();
	Fixture.Controller->DispatchAutomationKeyPressed(EKeys::MiddleMouseButton);
	const auto& Result = Fixture.Controller->
		GetLastThrownWeaponArcEditingInputResultForAutomation();
	const auto& State = Fixture.GameMode->GetThrownWeaponInputChoiceState();
	TestTrue(TEXT("locked physical input reaches the existing gameplay fence"),
		Fixture.Controller->
			GetThrownWeaponArcEditingInputInvocationCountForAutomation()
				== Before + 1
			&& Result.IsValid() && !Result.IsAccepted()
			&& Result.WasRejectedByIntentRoute()
			&& Result.GetInteractionReadCount() == 1
			&& Result.GetIntentRouteCount() == 1
			&& State.GetRevision() == 1
			&& !State.HasArcTargetIntent());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcEditingPhysicalStraightModeTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcEditingPhysicalInput.StraightModeFailsClosed",
	ArcEditingPhysicalInputFlags)

bool Fdemo_mapThrownWeaponArcEditingPhysicalStraightModeTest::RunTest(
	const FString&)
{
	FScopedArcEditingInputConfig Config;
	Fdemo_mapInputBindingSettings::Get().RestoreDefaults();
	FArcEditingPhysicalWorldFixture Fixture;
	if (!TestTrue(TEXT("Straight-mode fixture is ready"), Fixture.IsReady()))
	{
		return false;
	}
	Fixture.Controller->SetAutomationAimDirection(FVector(1.0, 0.0, 0.0));
	const uint64 Before = Fixture.Controller->
		GetThrownWeaponArcEditingInputInvocationCountForAutomation();
	Fixture.Controller->DispatchAutomationKeyPressed(EKeys::MiddleMouseButton);
	Fixture.Controller->DispatchAutomationKeyPressed(EKeys::RightBracket);
	Fixture.Controller->DispatchAutomationKeyPressed(EKeys::LeftBracket);
	Fixture.Controller->DispatchAutomationKeyPressed(EKeys::Delete);
	const auto& Result = Fixture.Controller->
		GetLastThrownWeaponArcEditingInputResultForAutomation();
	const auto& State = Fixture.GameMode->GetThrownWeaponInputChoiceState();
	TestTrue(TEXT("all four Arc keys fail closed while Straight is selected"),
		Fixture.Controller->
			GetThrownWeaponArcEditingInputInvocationCountForAutomation()
				== Before + 4
			&& Result.IsValid() && !Result.IsAccepted()
			&& Result.GetStatus() == ERequestStatus::RequestInvalid
			&& State.GetTrajectoryKind() == ETrajectory::Straight
			&& State.GetRevision() == 0
			&& !State.HasArcTargetIntent()
			&& State.GetArcApexAdjustment() == 0.0);
	return true;
}

#endif
