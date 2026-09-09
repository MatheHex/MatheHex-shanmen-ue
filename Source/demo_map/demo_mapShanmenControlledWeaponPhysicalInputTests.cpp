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
					== Fdemo_mapInputActionIds::ControlledWeaponLaunchRecall
				|| Action.ActionId
					== Fdemo_mapInputActionIds::ControlledWeaponRedirect)
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

	FString MakeVersionSevenConfig(bool bOccupyNewDefault)
	{
		FString Text = TEXT("[ShanmenInputBindings]\nVersion=7\n");
		for (const Fdemo_mapInputActionDefinition& Action :
			Fdemo_mapInputActionRegistry::GetExactDefaultActions())
		{
			if (Action.ActionId
				== Fdemo_mapInputActionIds::ControlledWeaponRedirect)
			{
				continue;
			}
			const FKey Key = bOccupyNewDefault
				&& Action.ActionId == Fdemo_mapInputActionIds::Interact
				? EKeys::C
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
	const Fdemo_mapInputActionDefinition* Redirect =
		Fdemo_mapInputActionRegistry::Find(
			Fdemo_mapInputActionIds::ControlledWeaponRedirect);
	TestTrue(TEXT("registry is exact after adding flying-sword redirect"),
		Fdemo_mapInputActionRegistry::ValidateExactDefaults()
			&& Fdemo_mapInputActionRegistry::GetExactDefaultActions().Num() == 31);
	TestTrue(TEXT("flying-sword command owns a conflict-free press-only X default"),
		Action
			&& Action->DefaultKey == EKeys::X
			&& !Action->bRequiresReleasedEvent
			&& Action->DisplayLabel.Contains(TEXT("飞剑"))
			&& Action->DisplayLabel.Contains(TEXT("召回")));
	TestTrue(TEXT("redirect owns a separate conflict-free press-only C default"),
		Redirect
			&& Redirect->DefaultKey == EKeys::C
			&& !Redirect->bRequiresReleasedEvent
			&& Redirect->DisplayLabel.Contains(TEXT("改向")));
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
	TestTrue(TEXT("version six receives both later flying-sword defaults"),
		Result.IsSuccess()
			&& Fdemo_mapInputBindingSettings::Get().GetBindings().Num() == 31
			&& Fdemo_mapInputBindingSettings::Get().GetKey(
				Fdemo_mapInputActionIds::ControlledWeaponLaunchRecall)
				== EKeys::X
			&& Fdemo_mapInputBindingSettings::Get().GetKey(
				Fdemo_mapInputActionIds::ControlledWeaponRedirect)
				== EKeys::C);
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
	TestTrue(TEXT("migration preserves old override and allocates both new actions"),
		Result.IsSuccess()
			&& Fdemo_mapInputBindingSettings::Get().GetKey(
				Fdemo_mapInputActionIds::Interact) == EKeys::X
			&& Fdemo_mapInputBindingSettings::Get().GetKey(
				Fdemo_mapInputActionIds::ControlledWeaponLaunchRecall)
				== EKeys::C
			&& Fdemo_mapInputBindingSettings::Get().GetKey(
				Fdemo_mapInputActionIds::ControlledWeaponRedirect)
				== EKeys::G
			&& Fdemo_mapInputBindingSettings::ValidateBindings(
				Fdemo_mapInputBindingSettings::Get().GetBindings(),
				Diagnostic));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapControlledWeaponRedirectPhysicalMigrationTest,
	"Shanmen.0_0_10.Product.ControlledWeaponPhysicalInput.VersionSevenRedirectMigration",
	ControlledWeaponPhysicalInputFlags)

bool Fdemo_mapControlledWeaponRedirectPhysicalMigrationTest::RunTest(
	const FString&)
{
	FScopedControlledWeaponInputConfig Config;
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(Config.Path), true);
	TestTrue(TEXT("version seven input fixture written"),
		FFileHelper::SaveStringToFile(
			MakeVersionSevenConfig(false), *Config.Path));
	const Fdemo_mapInputBindingResult Result =
		Fdemo_mapInputBindingSettings::Get().Load();
	TestTrue(TEXT("missing redirect receives free C without moving old keys"),
		Result.IsSuccess()
			&& Fdemo_mapInputBindingSettings::Get().GetBindings().Num() == 31
			&& Fdemo_mapInputBindingSettings::Get().GetKey(
				Fdemo_mapInputActionIds::ControlledWeaponLaunchRecall)
				== EKeys::X
			&& Fdemo_mapInputBindingSettings::Get().GetKey(
				Fdemo_mapInputActionIds::ControlledWeaponRedirect)
				== EKeys::C);

	TestTrue(TEXT("occupied-C version seven fixture written"),
		FFileHelper::SaveStringToFile(
			MakeVersionSevenConfig(true), *Config.Path));
	const Fdemo_mapInputBindingResult Conflict =
		Fdemo_mapInputBindingSettings::Get().Load();
	TestTrue(TEXT("redirect migration preserves C override and uses free G"),
		Conflict.IsSuccess()
			&& Fdemo_mapInputBindingSettings::Get().GetKey(
				Fdemo_mapInputActionIds::Interact) == EKeys::C
			&& Fdemo_mapInputBindingSettings::Get().GetKey(
				Fdemo_mapInputActionIds::ControlledWeaponRedirect)
				== EKeys::G);
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
	const uint64 RedirectBefore = Fixture.Controller
		->GetControlledWeaponRedirectInputInvocationCountForAutomation();
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
	TestTrue(TEXT("C press reaches the separate redirect handler"),
		Fixture.Controller->DispatchAutomationKeyPressed(EKeys::C));
	TestTrue(TEXT("one redirect press invokes once and preserves route fence"),
		Fixture.Controller
				->GetControlledWeaponRedirectInputInvocationCountForAutomation()
			== RedirectBefore + 1
		&& Fixture.Controller
			->GetLastControlledWeaponRedirectInputResultForAutomation().Status
				== Edemo_mapShanmenControlledWeaponInputStatus::
					ProductRouteUnavailable);
	Fixture.Controller->DispatchAutomationKeyReleased(EKeys::C);
	TestEqual(TEXT("redirect release owns no duplicate binding"),
		Fixture.Controller
			->GetControlledWeaponRedirectInputInvocationCountForAutomation(),
		RedirectBefore + 1);
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
			EKeys::V);
	Fixture.Controller->RebuildProductInputBindings();
	const uint64 Before = Fixture.Controller
		->GetControlledWeaponInputInvocationCountForAutomation();
	Fixture.Controller->DispatchAutomationKey(EKeys::X);
	Fixture.Controller->DispatchAutomationKey(EKeys::V);
	TestTrue(TEXT("live remap detaches X and activates V exactly once"),
		Remap.IsSuccess()
			&& Fdemo_mapInputBindingSettings::Get().GetKey(
				Fdemo_mapInputActionIds::ControlledWeaponLaunchRecall)
				== EKeys::V
			&& Fixture.Controller
				->GetControlledWeaponInputInvocationCountForAutomation()
					== Before + 1);

	Fixture.Controller->BeginSettlementInputLock(nullptr);
	Fixture.Controller->DispatchAutomationKey(EKeys::V);
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapControlledWeaponRedirectPhysicalRemapAndLockTest,
	"Shanmen.0_0_10.Product.ControlledWeaponPhysicalInput.RedirectLiveRemapAndLock",
	ControlledWeaponPhysicalInputFlags)

bool Fdemo_mapControlledWeaponRedirectPhysicalRemapAndLockTest::RunTest(
	const FString&)
{
	FScopedControlledWeaponInputConfig Config;
	Fdemo_mapInputBindingSettings::Get().RestoreDefaults();
	FControlledWeaponPhysicalWorldFixture Fixture;
	TestTrue(TEXT("redirect remap fixture is ready"), Fixture.IsReady());
	if (!Fixture.IsReady())
	{
		return false;
	}
	const Fdemo_mapInputBindingResult Remap =
		Fdemo_mapInputBindingSettings::Get().ApplyOverrideWithSwap(
			Fdemo_mapInputActionIds::ControlledWeaponRedirect,
			EKeys::Z);
	Fixture.Controller->RebuildProductInputBindings();
	const uint64 Before = Fixture.Controller
		->GetControlledWeaponRedirectInputInvocationCountForAutomation();
	Fixture.Controller->DispatchAutomationKey(EKeys::C);
	Fixture.Controller->DispatchAutomationKey(EKeys::Z);
	TestTrue(TEXT("live remap detaches C and activates Z exactly once"),
		Remap.IsSuccess()
			&& Fdemo_mapInputBindingSettings::Get().GetKey(
				Fdemo_mapInputActionIds::ControlledWeaponRedirect)
				== EKeys::Z
			&& Fixture.Controller
				->GetControlledWeaponRedirectInputInvocationCountForAutomation()
					== Before + 1);

	Fixture.Controller->BeginSettlementInputLock(nullptr);
	Fixture.Controller->DispatchAutomationKey(EKeys::Z);
	const Fdemo_mapShanmenControlledWeaponInputResult& Locked =
		Fixture.Controller
			->GetLastControlledWeaponRedirectInputResultForAutomation();
	TestTrue(TEXT("remapped redirect reaches the shared gameplay lock"),
		Fixture.Controller
				->GetControlledWeaponRedirectInputInvocationCountForAutomation()
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
