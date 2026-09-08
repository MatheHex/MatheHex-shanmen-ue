#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "demo_mapInputActionRegistry.h"
#include "demo_mapInputBindingSettings.h"
#include "demo_mapPlayerController.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
	constexpr EAutomationTestFlags ControlledWeaponPhysicalInputFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;

	struct FScopedControlledWeaponInputConfig
	{
		FString Path;
		bool bExisted = false;
		TArray<uint8> Bytes;

		FScopedControlledWeaponInputConfig()
		{
			Path = Fdemo_mapInputBindingSettings::Get().GetConfigPath();
			bExisted = IFileManager::Get().FileExists(*Path);
			if (bExisted)
			{
				FFileHelper::LoadFileToArray(Bytes, *Path);
			}
		}

		~FScopedControlledWeaponInputConfig()
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

	struct FControlledWeaponPhysicalWorldFixture
	{
		UWorld* World = nullptr;
		Ademo_mapPlayerController* Controller = nullptr;
		ACharacter* Character = nullptr;

		FControlledWeaponPhysicalWorldFixture()
		{
			World = NewObject<UWorld>(
				GetTransientPackage(), NAME_None, RF_Transient);
			if (!World || !GEngine)
			{
				return;
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
			Controller = World->SpawnActor<Ademo_mapPlayerController>();
			Character = World->SpawnActor<ACharacter>();
			if (Controller && Character)
			{
				Controller->Possess(Character);
				Controller->InitInputSystem();
			}
		}

		~FControlledWeaponPhysicalWorldFixture()
		{
			if (Controller && !Controller->IsActorBeingDestroyed())
			{
				Controller->UnPossess();
			}
			if (World)
			{
				World->DestroyWorld(false);
				GEngine->DestroyWorldContext(World);
			}
		}

		bool IsReady() const
		{
			return Controller
				&& Character
				&& Controller->HasPlayerInputForAutomation()
				&& Controller->HasInputComponentForAutomation();
		}
	};

	FString MakeVersionSixConfig(bool bOccupyNewDefault)
	{
		FString Text = TEXT("[ShanmenInputBindings]\nVersion=6\n");
		for (const Fdemo_mapInputActionDefinition& Action :
			Fdemo_mapInputActionRegistry::GetExactDefaultActions())
		{
			if (Action.ActionId
				== Fdemo_mapInputActionIds::ControlledWeaponLaunchRecall)
			{
				continue;
			}
			const FKey Key = bOccupyNewDefault
				&& Action.ActionId == Fdemo_mapInputActionIds::Interact
				? EKeys::X
				: Action.DefaultKey;
			Text += FString::Printf(
				TEXT("%s=%s\n"),
				*Action.ActionId.ToString(),
				*Key.GetFName().ToString());
		}
		return Text;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapControlledWeaponPhysicalRegistryTest,
	"Shanmen.0_0_10.Product.ControlledWeaponPhysicalInput.RegistryDefault",
	ControlledWeaponPhysicalInputFlags)

bool Fdemo_mapControlledWeaponPhysicalRegistryTest::RunTest(const FString&)
{
	const Fdemo_mapInputActionDefinition* Action =
		Fdemo_mapInputActionRegistry::Find(
			Fdemo_mapInputActionIds::ControlledWeaponLaunchRecall);
	TestTrue(TEXT("registry is exact after adding the flying-sword command"),
		Fdemo_mapInputActionRegistry::ValidateExactDefaults()
			&& Fdemo_mapInputActionRegistry::GetExactDefaultActions().Num() == 29);
	TestTrue(TEXT("flying-sword command owns a conflict-free press-only X default"),
		Action
			&& Action->DefaultKey == EKeys::X
			&& !Action->bRequiresReleasedEvent
			&& Action->DisplayLabel.Contains(TEXT("飞剑"))
			&& Action->DisplayLabel.Contains(TEXT("召回")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapControlledWeaponPhysicalMigrationTest,
	"Shanmen.0_0_10.Product.ControlledWeaponPhysicalInput.VersionSixMigration",
	ControlledWeaponPhysicalInputFlags)

bool Fdemo_mapControlledWeaponPhysicalMigrationTest::RunTest(const FString&)
{
	FScopedControlledWeaponInputConfig Config;
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(Config.Path), true);
	TestTrue(TEXT("version six input fixture written"),
		FFileHelper::SaveStringToFile(
			MakeVersionSixConfig(false), *Config.Path));
	const Fdemo_mapInputBindingResult Result =
		Fdemo_mapInputBindingSettings::Get().Load();
	TestTrue(TEXT("missing flying-sword command receives free X default"),
		Result.IsSuccess()
			&& Fdemo_mapInputBindingSettings::Get().GetBindings().Num() == 29
			&& Fdemo_mapInputBindingSettings::Get().GetKey(
				Fdemo_mapInputActionIds::ControlledWeaponLaunchRecall)
				== EKeys::X);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapControlledWeaponPhysicalMigrationConflictTest,
	"Shanmen.0_0_10.Product.ControlledWeaponPhysicalInput.MigrationConflict",
	ControlledWeaponPhysicalInputFlags)

bool Fdemo_mapControlledWeaponPhysicalMigrationConflictTest::RunTest(
	const FString&)
{
	FScopedControlledWeaponInputConfig Config;
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(Config.Path), true);
	TestTrue(TEXT("occupied-X input fixture written"),
		FFileHelper::SaveStringToFile(
			MakeVersionSixConfig(true), *Config.Path));
	const Fdemo_mapInputBindingResult Result =
		Fdemo_mapInputBindingSettings::Get().Load();
	FString Diagnostic;
	TestTrue(TEXT("migration preserves old override and uses the free G key"),
		Result.IsSuccess()
			&& Fdemo_mapInputBindingSettings::Get().GetKey(
				Fdemo_mapInputActionIds::Interact) == EKeys::X
			&& Fdemo_mapInputBindingSettings::Get().GetKey(
				Fdemo_mapInputActionIds::ControlledWeaponLaunchRecall)
				== EKeys::G
			&& Fdemo_mapInputBindingSettings::ValidateBindings(
				Fdemo_mapInputBindingSettings::Get().GetBindings(),
				Diagnostic));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapControlledWeaponPhysicalPressTest,
	"Shanmen.0_0_10.Product.ControlledWeaponPhysicalInput.PressOnlyBinding",
	ControlledWeaponPhysicalInputFlags)

bool Fdemo_mapControlledWeaponPhysicalPressTest::RunTest(const FString&)
{
	FScopedControlledWeaponInputConfig Config;
	Fdemo_mapInputBindingSettings::Get().RestoreDefaults();
	FControlledWeaponPhysicalWorldFixture Fixture;
	TestTrue(TEXT("physical flying-sword input fixture is ready"),
		Fixture.IsReady());
	if (!Fixture.IsReady())
	{
		return false;
	}
	const uint64 Before = Fixture.Controller
		->GetControlledWeaponInputInvocationCountForAutomation();
	TestTrue(TEXT("X press reaches the product handler"),
		Fixture.Controller->DispatchAutomationKeyPressed(EKeys::X));
	TestTrue(TEXT("one press invokes once and preserves missing GameMode fence"),
		Fixture.Controller->GetControlledWeaponInputInvocationCountForAutomation()
			== Before + 1
		&& Fixture.Controller->GetLastControlledWeaponInputResultForAutomation()
			.Status
				== Edemo_mapShanmenControlledWeaponInputStatus::
					ProductRouteUnavailable);
	Fixture.Controller->DispatchAutomationKeyReleased(EKeys::X);
	TestEqual(TEXT("release owns no duplicate binding"),
		Fixture.Controller->GetControlledWeaponInputInvocationCountForAutomation(),
		Before + 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapControlledWeaponPhysicalRemapAndLockTest,
	"Shanmen.0_0_10.Product.ControlledWeaponPhysicalInput.LiveRemapAndLock",
	ControlledWeaponPhysicalInputFlags)

bool Fdemo_mapControlledWeaponPhysicalRemapAndLockTest::RunTest(
	const FString&)
{
	FScopedControlledWeaponInputConfig Config;
	Fdemo_mapInputBindingSettings::Get().RestoreDefaults();
	FControlledWeaponPhysicalWorldFixture Fixture;
	TestTrue(TEXT("physical remap fixture is ready"), Fixture.IsReady());
	if (!Fixture.IsReady())
	{
		return false;
	}
	const Fdemo_mapInputBindingResult Remap =
		Fdemo_mapInputBindingSettings::Get().ApplyOverrideWithSwap(
			Fdemo_mapInputActionIds::ControlledWeaponLaunchRecall,
			EKeys::C);
	Fixture.Controller->RebuildProductInputBindings();
	const uint64 Before = Fixture.Controller
		->GetControlledWeaponInputInvocationCountForAutomation();
	Fixture.Controller->DispatchAutomationKey(EKeys::X);
	Fixture.Controller->DispatchAutomationKey(EKeys::C);
	TestTrue(TEXT("live remap detaches X and activates C exactly once"),
		Remap.IsSuccess()
			&& Fdemo_mapInputBindingSettings::Get().GetKey(
				Fdemo_mapInputActionIds::ControlledWeaponLaunchRecall)
				== EKeys::C
			&& Fixture.Controller
				->GetControlledWeaponInputInvocationCountForAutomation()
					== Before + 1);

	Fixture.Controller->BeginSettlementInputLock(nullptr);
	Fixture.Controller->DispatchAutomationKey(EKeys::C);
	const Fdemo_mapShanmenControlledWeaponInputResult& Locked =
		Fixture.Controller->GetLastControlledWeaponInputResultForAutomation();
	TestTrue(TEXT("remapped press reaches the shared gameplay lock"),
		Fixture.Controller->GetControlledWeaponInputInvocationCountForAutomation()
			== Before + 2
		&& Locked.Status
			== Edemo_mapShanmenControlledWeaponInputStatus::GameplayBlocked
		&& !Locked.bCanonicalReadInvoked
		&& !Locked.bIntentIdCreated
		&& !Locked.bDirectionSampled
		&& !Locked.bProductRouteInvoked);
	return true;
}

#endif
