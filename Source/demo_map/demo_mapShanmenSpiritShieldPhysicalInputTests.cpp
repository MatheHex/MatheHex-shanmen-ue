#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "Misc/AutomationTest.h"

#include "demo_mapInputActionRegistry.h"
#include "demo_mapInputBindingSettings.h"
#include "demo_mapPlayerController.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"

namespace
{
	constexpr EAutomationTestFlags ShieldPhysicalFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;

	struct FScopedShieldInputConfig
	{
		FString Path;
		bool bExisted = false;
		TArray<uint8> Bytes;

		FScopedShieldInputConfig()
		{
			Path = Fdemo_mapInputBindingSettings::Get().GetConfigPath();
			bExisted = IFileManager::Get().FileExists(*Path);
			if (bExisted)
			{
				FFileHelper::LoadFileToArray(Bytes, *Path);
			}
		}

		~FScopedShieldInputConfig()
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

	struct FShieldPhysicalWorldFixture
	{
		UWorld* World = nullptr;
		Ademo_mapPlayerController* Controller = nullptr;
		ACharacter* Character = nullptr;

		FShieldPhysicalWorldFixture()
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

		~FShieldPhysicalWorldFixture()
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
			return Controller && Character
				&& Controller->HasPlayerInputForAutomation()
				&& Controller->HasInputComponentForAutomation();
		}
	};

	FString MakeVersionTenConfig(const bool bOccupyH)
	{
		FString Text = TEXT("[ShanmenInputBindings]\nVersion=10\n");
		for (const Fdemo_mapInputActionDefinition& Action :
			Fdemo_mapInputActionRegistry::GetExactDefaultActions())
		{
			if (Action.ActionId == Fdemo_mapInputActionIds::SpiritShield)
			{
				continue;
			}
			const FKey Key = bOccupyH
				&& Action.ActionId == Fdemo_mapInputActionIds::Interact
					? EKeys::H
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
	Fdemo_mapSpiritShieldPhysicalRegistryTest,
	"Shanmen.0_0_10.Product.SpiritShieldPhysicalInput.RegistryDefault",
	ShieldPhysicalFlags)

bool Fdemo_mapSpiritShieldPhysicalRegistryTest::RunTest(const FString&)
{
	const Fdemo_mapInputActionDefinition* Action =
		Fdemo_mapInputActionRegistry::Find(
			Fdemo_mapInputActionIds::SpiritShield);
	TestTrue(TEXT("registry remains exact after adding Spirit Shield"),
		Fdemo_mapInputActionRegistry::ValidateExactDefaults()
			&& Fdemo_mapInputActionRegistry::GetExactDefaultActions().Num()
				== 33);
	TestTrue(TEXT("Spirit Shield owns a conflict-free press default"),
		Action && Action->DefaultKey == EKeys::H
			&& !Action->bRequiresReleasedEvent
			&& Action->DisplayLabel.Contains(TEXT("护盾")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSpiritShieldPhysicalMigrationTest,
	"Shanmen.0_0_10.Product.SpiritShieldPhysicalInput.VersionTenMigration",
	ShieldPhysicalFlags)

bool Fdemo_mapSpiritShieldPhysicalMigrationTest::RunTest(const FString&)
{
	FScopedShieldInputConfig Config;
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(Config.Path), true);
	TestTrue(TEXT("version ten fixture written"),
		FFileHelper::SaveStringToFile(
			MakeVersionTenConfig(false), *Config.Path));
	const Fdemo_mapInputBindingResult Free =
		Fdemo_mapInputBindingSettings::Get().Load();
	TestTrue(TEXT("free H becomes the migrated Spirit Shield binding"),
		Free.IsSuccess()
			&& Fdemo_mapInputBindingSettings::Get().GetBindings().Num() == 33
			&& Fdemo_mapInputBindingSettings::Get().GetKey(
				Fdemo_mapInputActionIds::SpiritShield) == EKeys::H);

	TestTrue(TEXT("occupied-H fixture written"),
		FFileHelper::SaveStringToFile(
			MakeVersionTenConfig(true), *Config.Path));
	const Fdemo_mapInputBindingResult Conflict =
		Fdemo_mapInputBindingSettings::Get().Load();
	FString Diagnostic;
	TestTrue(TEXT("migration preserves H override and allocates free G"),
		Conflict.IsSuccess()
			&& Fdemo_mapInputBindingSettings::Get().GetKey(
				Fdemo_mapInputActionIds::Interact) == EKeys::H
			&& Fdemo_mapInputBindingSettings::Get().GetKey(
				Fdemo_mapInputActionIds::SpiritShield) == EKeys::G
			&& Fdemo_mapInputBindingSettings::ValidateBindings(
				Fdemo_mapInputBindingSettings::Get().GetBindings(), Diagnostic));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSpiritShieldPhysicalPressTest,
	"Shanmen.0_0_10.Product.SpiritShieldPhysicalInput.PressBinding",
	ShieldPhysicalFlags)

bool Fdemo_mapSpiritShieldPhysicalPressTest::RunTest(const FString&)
{
	FScopedShieldInputConfig Config;
	Fdemo_mapInputBindingSettings::Get().RestoreDefaults();
	FShieldPhysicalWorldFixture Fixture;
	TestTrue(TEXT("physical shield fixture is ready"), Fixture.IsReady());
	if (!Fixture.IsReady())
	{
		return false;
	}
	const uint64 Before =
		Fixture.Controller->GetSpiritShieldInputInvocationCountForAutomation();
	TestTrue(TEXT("H press dispatches through PlayerInput"),
		Fixture.Controller->DispatchAutomationKeyPressed(EKeys::H));
	TestEqual(TEXT("physical press invokes exactly one shield route"),
		Fixture.Controller->GetSpiritShieldInputInvocationCountForAutomation(),
		Before + 1);
	const Fdemo_mapShanmenSpiritShieldProductActivationResult& Result =
		Fixture.Controller->GetLastSpiritShieldInputResultForAutomation();
	TestTrue(TEXT("missing GameMode produces a typed readable rejection"),
		Result.IsValid() && !Result.IsAccepted()
			&& Result.Error
				== Edemo_mapShanmenSpiritShieldProductActivationError::
					CoordinatorNotReady);
	TestTrue(TEXT("the latest physical outcome opens the HUD feedback window"),
		Fixture.Controller->IsSpiritShieldInputFeedbackActive()
			&& Fixture.Controller->GetLatestSpiritShieldInputResult().
				Diagnostic == Result.Diagnostic);
	return true;
}

#endif
