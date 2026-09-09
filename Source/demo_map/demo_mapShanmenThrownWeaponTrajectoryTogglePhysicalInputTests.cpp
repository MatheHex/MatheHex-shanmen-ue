#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "Misc/AutomationTest.h"

#include "demo_mapGameMode.h"
#include "demo_mapInputActionRegistry.h"
#include "demo_mapInputBindingSettings.h"
#include "demo_mapPlayerController.h"
#include "demo_mapShanmenThrownWeaponInputChoiceReducer.h"

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
	constexpr EAutomationTestFlags TrajectoryToggleFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;
	using ETrajectory =
		Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind;

	struct FScopedTrajectoryToggleInputConfig
	{
		FString Path;
		bool bExisted = false;
		TArray<uint8> Bytes;

		FScopedTrajectoryToggleInputConfig()
		{
			Path = Fdemo_mapInputBindingSettings::Get().GetConfigPath();
			bExisted = IFileManager::Get().FileExists(*Path);
			if (bExisted)
			{
				FFileHelper::LoadFileToArray(Bytes, *Path);
			}
		}

		~FScopedTrajectoryToggleInputConfig()
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

	FString MakeVersionFourConfig(const bool bOccupyToggleDefault)
	{
		FString Text = TEXT("[ShanmenInputBindings]\nVersion=4\n");
		for (const Fdemo_mapInputActionDefinition& Action :
			Fdemo_mapInputActionRegistry::GetExactDefaultActions())
		{
			if (Action.ActionId
				== Fdemo_mapInputActionIds::ThrownWeaponTrajectoryToggle)
			{
				continue;
			}
			const FKey Key = bOccupyToggleDefault
				&& Action.ActionId == Fdemo_mapInputActionIds::Interact
					? EKeys::T
					: Action.DefaultKey;
			Text += FString::Printf(
				TEXT("%s=%s\n"),
				*Action.ActionId.ToString(),
				*Key.GetFName().ToString());
		}
		return Text;
	}

	Fdemo_mapShanmenThrownWeaponInputChoiceState Apply(
		const Fdemo_mapShanmenThrownWeaponInputChoiceState& State,
		const Fdemo_mapShanmenThrownWeaponInputChoiceCommand& Command)
	{
		const Fdemo_mapShanmenThrownWeaponInputChoiceReduceResult Result =
			Fdemo_mapShanmenThrownWeaponInputChoiceReducer::Reduce(
				State, Command);
		check(Result.IsSuccess());
		return Result.State;
	}

	Fdemo_mapShanmenThrownWeaponInputChoiceInteractionReadModel Project(
		const Fdemo_mapShanmenThrownWeaponInputChoiceState& State)
	{
		Fdemo_mapShanmenThrownWeaponInputChoiceInteractionReadModel Model;
		check(Fdemo_mapShanmenThrownWeaponInputChoiceInteractionReadModel::
			TryProject(State, Model));
		return Model;
	}

	struct FTrajectoryTogglePhysicalWorldFixture
	{
		UGameInstance* GameInstance = nullptr;
		UWorld* World = nullptr;
		Ademo_mapGameMode* GameMode = nullptr;
		Ademo_mapPlayerController* Controller = nullptr;
		ACharacter* Character = nullptr;
		bool bGameplayRestored = false;

		FTrajectoryTogglePhysicalWorldFixture()
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

		~FTrajectoryTogglePhysicalWorldFixture()
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
			return GameMode && Controller && Character && bGameplayRestored
				&& Controller->HasPlayerInputForAutomation()
				&& Controller->HasInputComponentForAutomation();
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponTrajectoryToggleRegistryTest,
	"Shanmen.0_0_10.Product.ThrownWeaponTrajectoryTogglePhysicalInput.RegistryDefault",
	TrajectoryToggleFlags)

bool Fdemo_mapThrownWeaponTrajectoryToggleRegistryTest::RunTest(
	const FString&)
{
	const Fdemo_mapInputActionDefinition* Action =
		Fdemo_mapInputActionRegistry::Find(
			Fdemo_mapInputActionIds::ThrownWeaponTrajectoryToggle);
	TestTrue(TEXT("unified registry remains exact with trajectory toggle"),
		Fdemo_mapInputActionRegistry::ValidateExactDefaults()
			&& Fdemo_mapInputActionRegistry::GetExactDefaultActions().Num() == 31);
	TestTrue(TEXT("trajectory toggle owns a conflict-free press-only default"),
		Action
			&& Action->DefaultKey == EKeys::T
			&& !Action->bRequiresReleasedEvent
			&& Action->DisplayLabel.Contains(TEXT("轨迹")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponTrajectoryToggleCaptureTest,
	"Shanmen.0_0_10.Product.ThrownWeaponTrajectoryTogglePhysicalInput.LogicalToggleCapture",
	TrajectoryToggleFlags)

bool Fdemo_mapThrownWeaponTrajectoryToggleCaptureTest::RunTest(
	const FString&)
{
	const Fdemo_mapShanmenThrownWeaponInputChoiceState Initial =
		Fdemo_mapShanmenThrownWeaponInputChoiceState::CreateInitial();
	const Fdemo_mapShanmenThrownWeaponInputChoiceInteractionReadModel Straight =
		Project(Initial);
	Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest ToArc;
	if (!TestTrue(TEXT("Straight projection captures one Arc request"),
		Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest::
			TryCaptureTrajectoryToggle(Straight, ToArc)))
	{
		return false;
	}
	TestTrue(TEXT("Straight toggle preserves visible identity and selects Arc"),
		ToArc.IsValid()
			&& ToArc.GetExpectedReadModelId() == Straight.GetReadModelId()
			&& ToArc.GetIntent().GetTrajectoryKind()
				== ETrajectory::BallisticArc);

	Fdemo_mapShanmenThrownWeaponInputChoiceCommand ArcCommand;
	check(ToArc.GetIntent().TryCaptureCommand(0, ArcCommand));
	const Fdemo_mapShanmenThrownWeaponInputChoiceState ArcState =
		Apply(Initial, ArcCommand);
	const Fdemo_mapShanmenThrownWeaponInputChoiceInteractionReadModel Arc =
		Project(ArcState);
	Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest ToStraight;
	TestTrue(TEXT("Arc projection captures one Straight request"),
		Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest::
			TryCaptureTrajectoryToggle(Arc, ToStraight));
	TestTrue(TEXT("Arc toggle preserves visible identity and selects Straight"),
		ToStraight.IsValid()
			&& ToStraight.GetExpectedReadModelId() == Arc.GetReadModelId()
			&& ToStraight.GetIntent().GetTrajectoryKind()
				== ETrajectory::Straight);

	Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest Reusable =
		ToStraight;
	TestFalse(TEXT("invalid projection fails closed"),
		Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest::
			TryCaptureTrajectoryToggle(
				Fdemo_mapShanmenThrownWeaponInputChoiceInteractionReadModel(),
				Reusable));
	TestFalse(TEXT("failed capture clears reusable output"), Reusable.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponTrajectoryToggleMigrationTest,
	"Shanmen.0_0_10.Product.ThrownWeaponTrajectoryTogglePhysicalInput.VersionFourMigration",
	TrajectoryToggleFlags)

bool Fdemo_mapThrownWeaponTrajectoryToggleMigrationTest::RunTest(
	const FString&)
{
	FScopedTrajectoryToggleInputConfig Config;
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(Config.Path), true);
	TestTrue(TEXT("version four input fixture written"),
		FFileHelper::SaveStringToFile(
			MakeVersionFourConfig(false), *Config.Path));
	const Fdemo_mapInputBindingResult Result =
		Fdemo_mapInputBindingSettings::Get().Load();
	TestTrue(TEXT("missing toggle receives its conflict-free T default"),
		Result.IsSuccess()
			&& Fdemo_mapInputBindingSettings::Get().GetBindings().Num() == 31
			&& Fdemo_mapInputBindingSettings::Get().GetKey(
				Fdemo_mapInputActionIds::ThrownWeaponTrajectoryToggle)
				== EKeys::T);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponTrajectoryToggleMigrationConflictTest,
	"Shanmen.0_0_10.Product.ThrownWeaponTrajectoryTogglePhysicalInput.MigrationConflict",
	TrajectoryToggleFlags)

bool Fdemo_mapThrownWeaponTrajectoryToggleMigrationConflictTest::RunTest(
	const FString&)
{
	FScopedTrajectoryToggleInputConfig Config;
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(Config.Path), true);
	TestTrue(TEXT("occupied T migration fixture written"),
		FFileHelper::SaveStringToFile(
			MakeVersionFourConfig(true), *Config.Path));
	const Fdemo_mapInputBindingResult Result =
		Fdemo_mapInputBindingSettings::Get().Load();
	FString Diagnostic;
	TestTrue(TEXT("migration preserves old T override and assigns free G"),
		Result.IsSuccess()
			&& Fdemo_mapInputBindingSettings::Get().GetKey(
				Fdemo_mapInputActionIds::Interact) == EKeys::T
			&& Fdemo_mapInputBindingSettings::Get().GetKey(
				Fdemo_mapInputActionIds::ThrownWeaponTrajectoryToggle)
				== EKeys::G
			&& Fdemo_mapInputBindingSettings::ValidateBindings(
				Fdemo_mapInputBindingSettings::Get().GetBindings(), Diagnostic));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponTrajectoryToggleRoundTripTest,
	"Shanmen.0_0_10.Product.ThrownWeaponTrajectoryTogglePhysicalInput.PressRoundTrip",
	TrajectoryToggleFlags)

bool Fdemo_mapThrownWeaponTrajectoryToggleRoundTripTest::RunTest(
	const FString&)
{
	FScopedTrajectoryToggleInputConfig Config;
	Fdemo_mapInputBindingSettings::Get().RestoreDefaults();
	FTrajectoryTogglePhysicalWorldFixture Fixture;
	if (!TestTrue(TEXT("trajectory toggle world fixture is ready"),
		Fixture.IsReady()))
	{
		return false;
	}
	const uint64 Before = Fixture.Controller->
		GetThrownWeaponTrajectoryToggleInvocationCountForAutomation();
	TestTrue(TEXT("T press dispatches through the unified binding"),
		Fixture.Controller->DispatchAutomationKeyPressed(EKeys::T));
	const auto& FirstRead = Fixture.Controller->
		GetLastThrownWeaponTrajectoryToggleReadForAutomation();
	const auto& FirstRequest = Fixture.Controller->
		GetLastThrownWeaponTrajectoryToggleRequestForAutomation();
	const auto& FirstResult = Fixture.Controller->
		GetLastThrownWeaponTrajectoryToggleResultForAutomation();
	TestTrue(TEXT("first press reads Straight and routes one accepted Arc request"),
		Fixture.Controller->
			GetThrownWeaponTrajectoryToggleInvocationCountForAutomation()
				== Before + 1
			&& FirstRead.IsProjected()
			&& FirstRead.GetReadModel().GetTrajectoryKind()
				== ETrajectory::Straight
			&& FirstRequest.IsValid()
			&& FirstRequest.GetIntent().GetTrajectoryKind()
				== ETrajectory::BallisticArc
			&& FirstResult.IsAccepted()
			&& FirstResult.GetInteractionReadCount() == 1
			&& FirstResult.GetIntentRouteCount() == 1
			&& Fixture.GameMode->GetThrownWeaponInputChoiceState()
				.GetTrajectoryKind() == ETrajectory::BallisticArc
			&& Fixture.GameMode->GetThrownWeaponInputChoiceState()
				.GetRevision() == 1);

	Fixture.Controller->DispatchAutomationKeyReleased(EKeys::T);
	TestEqual(TEXT("release owns no duplicate toggle binding"),
		Fixture.Controller->
			GetThrownWeaponTrajectoryToggleInvocationCountForAutomation(),
		Before + 1);
	TestTrue(TEXT("second T press dispatches"),
		Fixture.Controller->DispatchAutomationKeyPressed(EKeys::T));
	const auto& SecondRead = Fixture.Controller->
		GetLastThrownWeaponTrajectoryToggleReadForAutomation();
	const auto& SecondRequest = Fixture.Controller->
		GetLastThrownWeaponTrajectoryToggleRequestForAutomation();
	const auto& SecondResult = Fixture.Controller->
		GetLastThrownWeaponTrajectoryToggleResultForAutomation();
	TestTrue(TEXT("second press reads Arc and returns to Straight exactly once"),
		Fixture.Controller->
			GetThrownWeaponTrajectoryToggleInvocationCountForAutomation()
				== Before + 2
			&& SecondRead.IsProjected()
			&& SecondRead.GetReadModel().GetTrajectoryKind()
				== ETrajectory::BallisticArc
			&& SecondRequest.IsValid()
			&& SecondRequest.GetIntent().GetTrajectoryKind()
				== ETrajectory::Straight
			&& SecondResult.IsAccepted()
			&& Fixture.GameMode->GetThrownWeaponInputChoiceState()
				.GetTrajectoryKind() == ETrajectory::Straight
			&& Fixture.GameMode->GetThrownWeaponInputChoiceState()
				.GetRevision() == 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponTrajectoryToggleRemapTest,
	"Shanmen.0_0_10.Product.ThrownWeaponTrajectoryTogglePhysicalInput.LiveRemap",
	TrajectoryToggleFlags)

bool Fdemo_mapThrownWeaponTrajectoryToggleRemapTest::RunTest(
	const FString&)
{
	FScopedTrajectoryToggleInputConfig Config;
	Fdemo_mapInputBindingSettings::Get().RestoreDefaults();
	FTrajectoryTogglePhysicalWorldFixture Fixture;
	if (!TestTrue(TEXT("trajectory remap fixture is ready"), Fixture.IsReady()))
	{
		return false;
	}
	const Fdemo_mapInputBindingResult Remap =
		Fdemo_mapInputBindingSettings::Get().ApplyOverrideWithSwap(
			Fdemo_mapInputActionIds::ThrownWeaponTrajectoryToggle,
			EKeys::C);
	Fixture.Controller->RebuildProductInputBindings();
	const uint64 Before = Fixture.Controller->
		GetThrownWeaponTrajectoryToggleInvocationCountForAutomation();
	Fixture.Controller->DispatchAutomationKey(EKeys::T);
	TestEqual(TEXT("old T binding is detached after rebuild"),
		Fixture.Controller->
			GetThrownWeaponTrajectoryToggleInvocationCountForAutomation(),
		Before);
	Fixture.Controller->DispatchAutomationKey(EKeys::C);
	FString Persisted;
	FFileHelper::LoadFileToString(Persisted, *Config.Path);
	TestTrue(TEXT("new C binding is persisted as version eight and active now"),
		Remap.IsSuccess()
			&& Persisted.Contains(TEXT("Version=9"))
			&& Fdemo_mapInputBindingSettings::Get().GetKey(
				Fdemo_mapInputActionIds::ThrownWeaponTrajectoryToggle)
				== EKeys::C
			&& Fixture.Controller->
				GetThrownWeaponTrajectoryToggleInvocationCountForAutomation()
					== Before + 1
			&& Fixture.Controller->
				GetLastThrownWeaponTrajectoryToggleResultForAutomation()
					.IsAccepted()
			&& Fixture.GameMode->GetConfiguredThrownWeaponTrajectoryKind()
				== ETrajectory::BallisticArc);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponTrajectoryToggleInputLockTest,
	"Shanmen.0_0_10.Product.ThrownWeaponTrajectoryTogglePhysicalInput.InputLock",
	TrajectoryToggleFlags)

bool Fdemo_mapThrownWeaponTrajectoryToggleInputLockTest::RunTest(
	const FString&)
{
	FScopedTrajectoryToggleInputConfig Config;
	Fdemo_mapInputBindingSettings::Get().RestoreDefaults();
	FTrajectoryTogglePhysicalWorldFixture Fixture;
	if (!TestTrue(TEXT("trajectory lock fixture is ready"), Fixture.IsReady()))
	{
		return false;
	}
	Fixture.Controller->BeginSettlementInputLock(nullptr);
	const uint64 Before = Fixture.Controller->
		GetThrownWeaponTrajectoryToggleInvocationCountForAutomation();
	Fixture.Controller->DispatchAutomationKeyPressed(EKeys::T);
	const auto& Result = Fixture.Controller->
		GetLastThrownWeaponTrajectoryToggleResultForAutomation();
	TestTrue(TEXT("locked input reaches the shared downstream gameplay fence"),
		Fixture.Controller->
			GetThrownWeaponTrajectoryToggleInvocationCountForAutomation()
				== Before + 1
			&& Fixture.Controller->
				GetLastThrownWeaponTrajectoryToggleReadForAutomation()
					.IsProjected()
			&& Fixture.Controller->
				GetLastThrownWeaponTrajectoryToggleRequestForAutomation()
					.IsValid()
			&& Result.IsValid()
			&& Result.WasRejectedByIntentRoute()
			&& Fixture.GameMode->GetThrownWeaponInputChoiceState()
				.GetTrajectoryKind() == ETrajectory::Straight
			&& Fixture.GameMode->GetThrownWeaponInputChoiceState()
				.GetRevision() == 0);
	return true;
}

#endif
