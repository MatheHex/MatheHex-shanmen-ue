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
	constexpr EAutomationTestFlags GuardPhysicalFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;

	struct FScopedGuardInputConfig
	{
		FString Path;
		bool bExisted = false;
		TArray<uint8> Bytes;

		FScopedGuardInputConfig()
		{
			Path = Fdemo_mapInputBindingSettings::Get().GetConfigPath();
			bExisted = IFileManager::Get().FileExists(*Path);
			if (bExisted)
			{
				FFileHelper::LoadFileToArray(Bytes, *Path);
			}
		}

		~FScopedGuardInputConfig()
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

	struct FGuardPhysicalWorldFixture
	{
		UWorld* World = nullptr;
		Ademo_mapPlayerController* Controller = nullptr;
		ACharacter* Character = nullptr;

		FGuardPhysicalWorldFixture()
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

		~FGuardPhysicalWorldFixture()
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

	FString MakeVersionThreeGuardConfig(bool bOccupyGuardDefault)
	{
		FString Text = TEXT("[ShanmenInputBindings]\nVersion=3\n");
		for (const Fdemo_mapInputActionDefinition& Action :
			Fdemo_mapInputActionRegistry::GetExactDefaultActions())
		{
			if (Action.ActionId == Fdemo_mapInputActionIds::WeaponGuard)
			{
				continue;
			}
			const FKey Key = bOccupyGuardDefault
				&& Action.ActionId == Fdemo_mapInputActionIds::Interact
				? EKeys::RightMouseButton
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
	Fdemo_mapWeaponGuardPhysicalRegistryTest,
	"Shanmen.0_0_10.Product.WeaponGuardPhysicalInput.RegistryDefault",
	GuardPhysicalFlags)

bool Fdemo_mapWeaponGuardPhysicalRegistryTest::RunTest(const FString&)
{
	const Fdemo_mapInputActionDefinition* Action =
		Fdemo_mapInputActionRegistry::Find(
			Fdemo_mapInputActionIds::WeaponGuard);
	TestTrue(TEXT("registry remains exact after adding Weapon Guard"),
		Fdemo_mapInputActionRegistry::ValidateExactDefaults()
			&& Fdemo_mapInputActionRegistry::GetExactDefaultActions().Num() == 30);
	TestTrue(TEXT("Weapon Guard owns a conflict-free press/release default"),
		Action
			&& Action->DefaultKey == EKeys::RightMouseButton
			&& Action->bRequiresReleasedEvent
			&& Action->DisplayLabel.Contains(TEXT("格挡")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapWeaponGuardPhysicalMigrationTest,
	"Shanmen.0_0_10.Product.WeaponGuardPhysicalInput.VersionThreeMigration",
	GuardPhysicalFlags)

bool Fdemo_mapWeaponGuardPhysicalMigrationTest::RunTest(const FString&)
{
	FScopedGuardInputConfig Config;
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(Config.Path), true);
	TestTrue(TEXT("version three fixture written"),
		FFileHelper::SaveStringToFile(
			MakeVersionThreeGuardConfig(false), *Config.Path));
	const Fdemo_mapInputBindingResult Result =
		Fdemo_mapInputBindingSettings::Get().Load();
	TestTrue(TEXT("missing Weapon Guard receives its free default"),
		Result.IsSuccess()
			&& Fdemo_mapInputBindingSettings::Get().GetBindings().Num() == 30
			&& Fdemo_mapInputBindingSettings::Get().GetKey(
				Fdemo_mapInputActionIds::WeaponGuard)
				== EKeys::RightMouseButton);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapWeaponGuardPhysicalMigrationConflictTest,
	"Shanmen.0_0_10.Product.WeaponGuardPhysicalInput.MigrationConflict",
	GuardPhysicalFlags)

bool Fdemo_mapWeaponGuardPhysicalMigrationConflictTest::RunTest(
	const FString&)
{
	FScopedGuardInputConfig Config;
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(Config.Path), true);
	TestTrue(TEXT("occupied guard default fixture written"),
		FFileHelper::SaveStringToFile(
			MakeVersionThreeGuardConfig(true), *Config.Path));
	const Fdemo_mapInputBindingResult Result =
		Fdemo_mapInputBindingSettings::Get().Load();
	FString Diagnostic;
	TestTrue(TEXT("migration preserves override and assigns released G key"),
		Result.IsSuccess()
			&& Fdemo_mapInputBindingSettings::Get().GetKey(
				Fdemo_mapInputActionIds::Interact)
				== EKeys::RightMouseButton
			&& Fdemo_mapInputBindingSettings::Get().GetKey(
				Fdemo_mapInputActionIds::WeaponGuard) == EKeys::G
			&& Fdemo_mapInputBindingSettings::ValidateBindings(
				Fdemo_mapInputBindingSettings::Get().GetBindings(),
				Diagnostic));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapWeaponGuardPhysicalPressReleaseTest,
	"Shanmen.0_0_10.Product.WeaponGuardPhysicalInput.PressReleaseBinding",
	GuardPhysicalFlags)

bool Fdemo_mapWeaponGuardPhysicalPressReleaseTest::RunTest(const FString&)
{
	FScopedGuardInputConfig Config;
	Fdemo_mapInputBindingSettings::Get().RestoreDefaults();
	FGuardPhysicalWorldFixture Fixture;
	TestTrue(TEXT("physical guard fixture is ready"), Fixture.IsReady());
	if (!Fixture.IsReady()) return false;
	const uint64 PressBefore =
		Fixture.Controller->GetWeaponGuardPressInvocationCountForAutomation();
	const uint64 ReleaseBefore =
		Fixture.Controller->GetWeaponGuardReleaseInvocationCountForAutomation();
	TestTrue(TEXT("right mouse press dispatches"),
		Fixture.Controller->DispatchAutomationKeyPressed(
			EKeys::RightMouseButton));
	TestEqual(TEXT("press invokes exactly once"),
		Fixture.Controller->GetWeaponGuardPressInvocationCountForAutomation(),
		PressBefore + 1);
	TestEqual(TEXT("missing GameMode fails before timeline sampling"),
		Fixture.Controller->GetLastWeaponGuardInputResultForAutomation().Status,
		Edemo_mapShanmenWeaponGuardInputStatus::ProductRouteUnavailable);
	TestTrue(TEXT("right mouse release dispatches"),
		Fixture.Controller->DispatchAutomationKeyReleased(
			EKeys::RightMouseButton));
	TestEqual(TEXT("release invokes exactly once"),
		Fixture.Controller->GetWeaponGuardReleaseInvocationCountForAutomation(),
		ReleaseBefore + 1);
	TestEqual(TEXT("missing GameMode is typed on release"),
		Fixture.Controller->GetLastWeaponGuardReleaseInputResultForAutomation().Status,
		Edemo_mapShanmenWeaponGuardReleaseInputStatus::ProductRouteUnavailable);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapWeaponGuardPhysicalRemapTest,
	"Shanmen.0_0_10.Product.WeaponGuardPhysicalInput.LiveRemap",
	GuardPhysicalFlags)

bool Fdemo_mapWeaponGuardPhysicalRemapTest::RunTest(const FString&)
{
	FScopedGuardInputConfig Config;
	Fdemo_mapInputBindingSettings::Get().RestoreDefaults();
	FGuardPhysicalWorldFixture Fixture;
	TestTrue(TEXT("physical guard remap fixture is ready"), Fixture.IsReady());
	if (!Fixture.IsReady()) return false;
	const Fdemo_mapInputBindingResult Remap =
		Fdemo_mapInputBindingSettings::Get().ApplyOverrideWithSwap(
			Fdemo_mapInputActionIds::WeaponGuard,
			EKeys::C);
	Fixture.Controller->RebuildProductInputBindings();
	const uint64 PressBefore =
		Fixture.Controller->GetWeaponGuardPressInvocationCountForAutomation();
	const uint64 ReleaseBefore =
		Fixture.Controller->GetWeaponGuardReleaseInvocationCountForAutomation();
	Fixture.Controller->DispatchAutomationKey(EKeys::RightMouseButton);
	TestTrue(TEXT("old right mouse bindings are detached"),
		Fixture.Controller->GetWeaponGuardPressInvocationCountForAutomation()
			== PressBefore
			&& Fixture.Controller->GetWeaponGuardReleaseInvocationCountForAutomation()
				== ReleaseBefore);
	Fixture.Controller->DispatchAutomationKey(EKeys::C);
	TestTrue(TEXT("new key owns both hold edges immediately"),
		Remap.IsSuccess()
			&& Fixture.Controller->GetWeaponGuardPressInvocationCountForAutomation()
				== PressBefore + 1
			&& Fixture.Controller->GetWeaponGuardReleaseInvocationCountForAutomation()
				== ReleaseBefore + 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapWeaponGuardPhysicalInputLockTest,
	"Shanmen.0_0_10.Product.WeaponGuardPhysicalInput.InputLockSafeRelease",
	GuardPhysicalFlags)

bool Fdemo_mapWeaponGuardPhysicalInputLockTest::RunTest(const FString&)
{
	FScopedGuardInputConfig Config;
	Fdemo_mapInputBindingSettings::Get().RestoreDefaults();
	FGuardPhysicalWorldFixture Fixture;
	TestTrue(TEXT("physical guard lock fixture is ready"), Fixture.IsReady());
	if (!Fixture.IsReady()) return false;
	Fixture.Controller->BeginSettlementInputLock(nullptr);
	const uint64 ReleaseBefore =
		Fixture.Controller->GetWeaponGuardReleaseInvocationCountForAutomation();
	Fixture.Controller->DispatchAutomationKeyPressed(EKeys::RightMouseButton);
	TestEqual(TEXT("press reaches shared gameplay fence"),
		Fixture.Controller->GetLastWeaponGuardInputResultForAutomation().Status,
		Edemo_mapShanmenWeaponGuardInputStatus::GameplayBlocked);
	Fixture.Controller->DispatchAutomationKeyReleased(EKeys::RightMouseButton);
	TestEqual(TEXT("release remains bound while gameplay is locked"),
		Fixture.Controller->GetWeaponGuardReleaseInvocationCountForAutomation(),
		ReleaseBefore + 1);
	return true;
}

#endif
