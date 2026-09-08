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
	constexpr EAutomationTestFlags PhysicalInputFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;

	struct FScopedPhysicalInputConfig
	{
		FString Path;
		bool bExisted = false;
		TArray<uint8> Bytes;

		FScopedPhysicalInputConfig()
		{
			Path = Fdemo_mapInputBindingSettings::Get().GetConfigPath();
			bExisted = IFileManager::Get().FileExists(*Path);
			if (bExisted)
			{
				FFileHelper::LoadFileToArray(Bytes, *Path);
			}
		}

		~FScopedPhysicalInputConfig()
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

	struct FPhysicalInputWorldFixture
	{
		UWorld* World = nullptr;
		Ademo_mapPlayerController* Controller = nullptr;
		ACharacter* Character = nullptr;

		FPhysicalInputWorldFixture()
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

		~FPhysicalInputWorldFixture()
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

	FString MakeVersionTwoConfig(
		bool bOccupySpiritEvasionDefault)
	{
		FString Text = TEXT("[ShanmenInputBindings]\nVersion=2\n");
		for (const Fdemo_mapInputActionDefinition& Action :
			Fdemo_mapInputActionRegistry::GetExactDefaultActions())
		{
			if (Action.ActionId == Fdemo_mapInputActionIds::SpiritEvasion
				|| Action.ActionId == Fdemo_mapInputActionIds::WeaponGuard)
			{
				continue;
			}
			const FKey Key =
				bOccupySpiritEvasionDefault
					&& Action.ActionId == Fdemo_mapInputActionIds::Interact
				? EKeys::SpaceBar
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
	Fdemo_mapSpiritEvasionPhysicalRegistryTest,
	"Shanmen.0_0_10.Product.SpiritEvasionPhysicalInput.RegistryDefault",
	PhysicalInputFlags)

bool Fdemo_mapSpiritEvasionPhysicalRegistryTest::RunTest(
	const FString& Parameters)
{
	const Fdemo_mapInputActionDefinition* Action =
		Fdemo_mapInputActionRegistry::Find(
			Fdemo_mapInputActionIds::SpiritEvasion);
	TestTrue(
		TEXT("registry remains exact after adding Spirit Evasion"),
		Fdemo_mapInputActionRegistry::ValidateExactDefaults()
			&& Fdemo_mapInputActionRegistry::
				GetExactDefaultActions().Num() == 29);
	TestTrue(
		TEXT("Spirit Evasion owns a conflict-free Press-only default"),
		Action
			&& Action->DefaultKey == EKeys::SpaceBar
			&& !Action->bRequiresReleasedEvent
			&& Action->DisplayLabel.Contains(TEXT("闪避")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSpiritEvasionPhysicalMigrationTest,
	"Shanmen.0_0_10.Product.SpiritEvasionPhysicalInput.VersionTwoMigration",
	PhysicalInputFlags)

bool Fdemo_mapSpiritEvasionPhysicalMigrationTest::RunTest(
	const FString& Parameters)
{
	FScopedPhysicalInputConfig Config;
	IFileManager::Get().MakeDirectory(
		*FPaths::GetPath(Config.Path), true);
	TestTrue(
		TEXT("version two fixture written"),
		FFileHelper::SaveStringToFile(
			MakeVersionTwoConfig(false), *Config.Path));
	const Fdemo_mapInputBindingResult Result =
		Fdemo_mapInputBindingSettings::Get().Load();
	TestTrue(
		TEXT("missing Spirit Evasion receives its free default"),
		Result.IsSuccess()
			&& Fdemo_mapInputBindingSettings::Get().GetBindings().Num() == 29
			&& Fdemo_mapInputBindingSettings::Get().GetKey(
				Fdemo_mapInputActionIds::SpiritEvasion) == EKeys::SpaceBar
			&& Fdemo_mapInputBindingSettings::Get().GetKey(
				Fdemo_mapInputActionIds::Interact) == EKeys::G);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSpiritEvasionPhysicalMigrationConflictTest,
	"Shanmen.0_0_10.Product.SpiritEvasionPhysicalInput.MigrationConflict",
	PhysicalInputFlags)

bool Fdemo_mapSpiritEvasionPhysicalMigrationConflictTest::RunTest(
	const FString& Parameters)
{
	FScopedPhysicalInputConfig Config;
	IFileManager::Get().MakeDirectory(
		*FPaths::GetPath(Config.Path), true);
	TestTrue(
		TEXT("occupied-default fixture written"),
		FFileHelper::SaveStringToFile(
			MakeVersionTwoConfig(true), *Config.Path));
	const Fdemo_mapInputBindingResult Result =
		Fdemo_mapInputBindingSettings::Get().Load();
	FString Diagnostic;
	TestTrue(
		TEXT("migration preserves the old override and assigns free keys in registry order"),
		Result.IsSuccess()
			&& Fdemo_mapInputBindingSettings::Get().GetKey(
				Fdemo_mapInputActionIds::Interact) == EKeys::SpaceBar
			&& Fdemo_mapInputBindingSettings::Get().GetKey(
				Fdemo_mapInputActionIds::SpiritEvasion) == EKeys::RightMouseButton
			&& Fdemo_mapInputBindingSettings::Get().GetKey(
				Fdemo_mapInputActionIds::WeaponGuard) == EKeys::G
			&& Fdemo_mapInputBindingSettings::ValidateBindings(
				Fdemo_mapInputBindingSettings::Get().GetBindings(),
				Diagnostic));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSpiritEvasionPhysicalPressTest,
	"Shanmen.0_0_10.Product.SpiritEvasionPhysicalInput.PressOnlyBinding",
	PhysicalInputFlags)

bool Fdemo_mapSpiritEvasionPhysicalPressTest::RunTest(
	const FString& Parameters)
{
	FScopedPhysicalInputConfig Config;
	Fdemo_mapInputBindingSettings::Get().RestoreDefaults();
	FPhysicalInputWorldFixture Fixture;
	TestTrue(TEXT("physical input fixture is ready"), Fixture.IsReady());
	if (!Fixture.IsReady())
	{
		return false;
	}
	const uint64 Before =
		Fixture.Controller->GetSpiritEvasionInputInvocationCountForAutomation();
	TestTrue(
		TEXT("default key press reaches the handler"),
		Fixture.Controller->DispatchAutomationKeyPressed(EKeys::SpaceBar));
	TestEqual(
		TEXT("one press invokes exactly once"),
		Fixture.Controller->GetSpiritEvasionInputInvocationCountForAutomation(),
		Before + 1);
	TestEqual(
		TEXT("missing product route is preserved, not bypassed"),
		Fixture.Controller->GetLastSpiritEvasionInputResultForAutomation().Status,
		Edemo_mapShanmenSpiritEvasionInputStatus::ProductRouteUnavailable);
	Fixture.Controller->DispatchAutomationKeyReleased(EKeys::SpaceBar);
	TestEqual(
		TEXT("release owns no duplicate Spirit Evasion binding"),
		Fixture.Controller->GetSpiritEvasionInputInvocationCountForAutomation(),
		Before + 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSpiritEvasionPhysicalRemapTest,
	"Shanmen.0_0_10.Product.SpiritEvasionPhysicalInput.LiveRemap",
	PhysicalInputFlags)

bool Fdemo_mapSpiritEvasionPhysicalRemapTest::RunTest(
	const FString& Parameters)
{
	FScopedPhysicalInputConfig Config;
	Fdemo_mapInputBindingSettings::Get().RestoreDefaults();
	FPhysicalInputWorldFixture Fixture;
	TestTrue(TEXT("physical remap fixture is ready"), Fixture.IsReady());
	if (!Fixture.IsReady())
	{
		return false;
	}
	const Fdemo_mapInputBindingResult Remap =
		Fdemo_mapInputBindingSettings::Get().ApplyOverrideWithSwap(
			Fdemo_mapInputActionIds::SpiritEvasion,
			EKeys::C);
	Fixture.Controller->RebuildProductInputBindings();
	const uint64 Before =
		Fixture.Controller->GetSpiritEvasionInputInvocationCountForAutomation();
	Fixture.Controller->DispatchAutomationKey(EKeys::SpaceBar);
	TestEqual(
		TEXT("old key is detached after rebuild"),
		Fixture.Controller->GetSpiritEvasionInputInvocationCountForAutomation(),
		Before);
	Fixture.Controller->DispatchAutomationKey(EKeys::C);
	TestTrue(
		TEXT("new key is persisted and immediately active"),
		Remap.IsSuccess()
			&& Fdemo_mapInputBindingSettings::Get().GetKey(
				Fdemo_mapInputActionIds::SpiritEvasion) == EKeys::C
			&& Fixture.Controller->
				GetSpiritEvasionInputInvocationCountForAutomation() == Before + 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSpiritEvasionPhysicalInputLockTest,
	"Shanmen.0_0_10.Product.SpiritEvasionPhysicalInput.InputLock",
	PhysicalInputFlags)

bool Fdemo_mapSpiritEvasionPhysicalInputLockTest::RunTest(
	const FString& Parameters)
{
	FScopedPhysicalInputConfig Config;
	Fdemo_mapInputBindingSettings::Get().RestoreDefaults();
	FPhysicalInputWorldFixture Fixture;
	TestTrue(TEXT("physical lock fixture is ready"), Fixture.IsReady());
	if (!Fixture.IsReady())
	{
		return false;
	}
	Fixture.Controller->BeginSettlementInputLock(nullptr);
	const uint64 Before =
		Fixture.Controller->GetSpiritEvasionInputInvocationCountForAutomation();
	Fixture.Controller->DispatchAutomationKey(EKeys::SpaceBar);
	const Fdemo_mapShanmenSpiritEvasionInputResult& Result =
		Fixture.Controller->GetLastSpiritEvasionInputResultForAutomation();
	TestTrue(
		TEXT("physical press reaches the shared gameplay fence"),
		Fixture.Controller->GetSpiritEvasionInputInvocationCountForAutomation()
			== Before + 1
			&& Result.Status
				== Edemo_mapShanmenSpiritEvasionInputStatus::GameplayBlocked
			&& !Result.bDirectionSampled
			&& !Result.bProductRouteInvoked);
	return true;
}

#endif
