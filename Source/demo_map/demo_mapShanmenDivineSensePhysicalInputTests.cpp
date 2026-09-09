#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/BoxComponent.h"
#include "demo_mapEnemyCharacter.h"
#include "demo_mapGameMode.h"
#include "demo_mapInputActionRegistry.h"
#include "demo_mapInputBindingSettings.h"
#include "demo_mapM01EnemyIdentityComponent.h"
#include "demo_mapM01EnemyTypes.h"
#include "demo_mapM01Marker.h"
#include "demo_mapPlayerController.h"
#include "demo_mapPlayerHealthComponent.h"
#include "demo_mapShanmenDivineSenseHUDPresentation.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
	constexpr EAutomationTestFlags DivineSensePhysicalInputFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;

	struct FScopedDivineSenseInputConfig
	{
		FString Path;
		bool bExisted = false;
		TArray<uint8> Bytes;

		FScopedDivineSenseInputConfig()
		{
			Path = Fdemo_mapInputBindingSettings::Get().GetConfigPath();
			bExisted = IFileManager::Get().FileExists(*Path);
			if (bExisted)
			{
				FFileHelper::LoadFileToArray(Bytes, *Path);
			}
		}

		~FScopedDivineSenseInputConfig()
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

	struct FDivineSensePhysicalWorldFixture
	{
		UWorld* World = nullptr;
		Ademo_mapPlayerController* Controller = nullptr;
		ACharacter* Character = nullptr;

		FDivineSensePhysicalWorldFixture()
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

		~FDivineSensePhysicalWorldFixture()
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

	const Fdemo_mapM01EnemyDefinition* FindDivineSenseMeleeDefinition()
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

	struct FDivineSenseProductWorldFixture
	{
		UGameInstance* GameInstance = nullptr;
		UPackage* WorldPackage = nullptr;
		UWorld* World = nullptr;
		Ademo_mapGameMode* GameMode = nullptr;
		Ademo_mapPlayerController* Controller = nullptr;
		ACharacter* Character = nullptr;
		Udemo_mapPlayerHealthComponent* Health = nullptr;
		Ademo_mapM01Marker* RootMarker = nullptr;
		Ademo_mapEnemyCharacter* Enemy = nullptr;
		Udemo_mapM01EnemyIdentityComponent* EnemyIdentity = nullptr;
		const Fdemo_mapM01EnemyDefinition* EnemyDefinition = nullptr;

		FDivineSenseProductWorldFixture()
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

			const FString WorldPackageName = FString::Printf(
				TEXT("/Temp/P230_%s_L_M01_Expedition"),
				*FGuid::NewGuid().ToString(EGuidFormats::Digits));
			WorldPackage = CreatePackage(*WorldPackageName);
			World = NewObject<UWorld>(
				WorldPackage,
				TEXT("L_M01_Expedition"),
				RF_Transient);
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

			FActorSpawnParameters Parameters;
			Parameters.ObjectFlags |= RF_Transient;
			Parameters.SpawnCollisionHandlingOverride =
				ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			GameMode = Cast<Ademo_mapGameMode>(World->GetAuthGameMode());
			Controller = World->SpawnActor<Ademo_mapPlayerController>(
				Ademo_mapPlayerController::StaticClass(),
				FTransform::Identity,
				Parameters);
			Character = World->SpawnActor<ACharacter>(
				ACharacter::StaticClass(),
				FTransform::Identity,
				Parameters);
			RootMarker = World->SpawnActor<Ademo_mapM01Marker>(
				Ademo_mapM01Marker::StaticClass(),
				FTransform::Identity,
				Parameters);
			Enemy = World->SpawnActor<Ademo_mapEnemyCharacter>(
				Ademo_mapEnemyCharacter::StaticClass(),
				FTransform(FVector(300.0, 0.0, 0.0)),
				Parameters);
			EnemyDefinition = FindDivineSenseMeleeDefinition();
			if (!GameMode || !Controller || !Character || !RootMarker
				|| !Enemy || !EnemyDefinition)
			{
				return;
			}
			// This headless World does not run the normal login path, so make
			// the locally spawned controller discoverable to product GameMode.
			World->AddController(Controller);
			Controller->Possess(Character);
			RootMarker->ConfigureRoot();
			Health = NewObject<Udemo_mapPlayerHealthComponent>(
				Character,
				TEXT("P230PlayerHealth"),
				RF_Transient);
			EnemyIdentity = NewObject<Udemo_mapM01EnemyIdentityComponent>(
				Enemy,
				TEXT("P230M01AuthoredIdentity"),
				RF_Transient);
			if (!Health || !EnemyIdentity)
			{
				return;
			}
			Character->AddInstanceComponent(Health);
			Enemy->AddInstanceComponent(EnemyIdentity);
			Fdemo_mapEnemyEncounterIdentity LegacyIdentity;
			LegacyIdentity.EncounterId = EnemyDefinition->EncounterId;
			LegacyIdentity.RouteId = EnemyDefinition->RouteId;
			LegacyIdentity.SpawnMarkerId = EnemyDefinition->SpawnMarkerId;
			LegacyIdentity.LootTableId = EnemyDefinition->CorpseIdentity;
			LegacyIdentity.SkillProfileId = EnemyDefinition->SkillProfileId;
			if (!EnemyIdentity->Configure(*EnemyDefinition)
				|| !Enemy->ConfigureEncounter(
					LegacyIdentity,
					EnemyDefinition->Tuning,
					EnemyDefinition->IsElite()))
			{
				EnemyIdentity = nullptr;
			}
		}

		~FDivineSenseProductWorldFixture()
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
			// The temporary package is unrooted and is collected together with
			// the destroyed World. Drop our observer before that collection.
			WorldPackage = nullptr;
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
			return GameMode && Controller && Character && Health
				&& RootMarker && Enemy && EnemyIdentity && EnemyDefinition;
		}
	};

	FString MakeVersionEightConfig(const bool bOccupyV)
	{
		FString Text = TEXT("[ShanmenInputBindings]\nVersion=8\n");
		for (const Fdemo_mapInputActionDefinition& Action :
			Fdemo_mapInputActionRegistry::GetExactDefaultActions())
		{
			if (Action.ActionId == Fdemo_mapInputActionIds::DivineSense)
			{
				continue;
			}
			const FKey Key = bOccupyV
				&& Action.ActionId == Fdemo_mapInputActionIds::Interact
					? EKeys::V
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
	Fdemo_mapDivineSensePhysicalRegistryTest,
	"Shanmen.0_0_10.Product.DivineSensePhysicalInput.RegistryDefault",
	DivineSensePhysicalInputFlags)

bool Fdemo_mapDivineSensePhysicalRegistryTest::RunTest(const FString&)
{
	const Fdemo_mapInputActionDefinition* Action =
		Fdemo_mapInputActionRegistry::Find(
			Fdemo_mapInputActionIds::DivineSense);
	TestTrue(TEXT("registry is exact after adding physical Divine Sense"),
		Fdemo_mapInputActionRegistry::ValidateExactDefaults()
			&& Fdemo_mapInputActionRegistry::GetExactDefaultActions().Num()
				== 31);
	TestTrue(TEXT("Divine Sense owns a conflict-free press-only V default"),
		Action && Action->DefaultKey == EKeys::V
			&& !Action->bRequiresReleasedEvent
			&& Action->DisplayLabel.Contains(TEXT("神识")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapDivineSensePhysicalMigrationTest,
	"Shanmen.0_0_10.Product.DivineSensePhysicalInput.VersionEightMigration",
	DivineSensePhysicalInputFlags)

bool Fdemo_mapDivineSensePhysicalMigrationTest::RunTest(const FString&)
{
	FScopedDivineSenseInputConfig Config;
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(Config.Path), true);
	TestTrue(TEXT("version eight fixture written"),
		FFileHelper::SaveStringToFile(
			MakeVersionEightConfig(false), *Config.Path));
	const Fdemo_mapInputBindingResult Free =
		Fdemo_mapInputBindingSettings::Get().Load();
	TestTrue(TEXT("free V becomes the migrated Divine Sense binding"),
		Free.IsSuccess()
			&& Fdemo_mapInputBindingSettings::Get().GetBindings().Num() == 31
			&& Fdemo_mapInputBindingSettings::Get().GetKey(
				Fdemo_mapInputActionIds::DivineSense) == EKeys::V);

	TestTrue(TEXT("occupied-V fixture written"),
		FFileHelper::SaveStringToFile(
			MakeVersionEightConfig(true), *Config.Path));
	const Fdemo_mapInputBindingResult Conflict =
		Fdemo_mapInputBindingSettings::Get().Load();
	FString Diagnostic;
	TestTrue(TEXT("migration preserves V override and allocates free G"),
		Conflict.IsSuccess()
			&& Fdemo_mapInputBindingSettings::Get().GetKey(
				Fdemo_mapInputActionIds::Interact) == EKeys::V
			&& Fdemo_mapInputBindingSettings::Get().GetKey(
				Fdemo_mapInputActionIds::DivineSense) == EKeys::G
			&& Fdemo_mapInputBindingSettings::ValidateBindings(
				Fdemo_mapInputBindingSettings::Get().GetBindings(), Diagnostic));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapDivineSensePhysicalPressTest,
	"Shanmen.0_0_10.Product.DivineSensePhysicalInput.PressOnlyBinding",
	DivineSensePhysicalInputFlags)

bool Fdemo_mapDivineSensePhysicalPressTest::RunTest(const FString&)
{
	FScopedDivineSenseInputConfig Config;
	Fdemo_mapInputBindingSettings::Get().RestoreDefaults();
	FDivineSensePhysicalWorldFixture Fixture;
	TestTrue(TEXT("physical Divine Sense fixture is ready"),
		Fixture.IsReady());
	if (!Fixture.IsReady())
	{
		return false;
	}
	const uint64 Before = Fixture.Controller
		->GetDivineSenseInputInvocationCountForAutomation();
	TestTrue(TEXT("V press reaches the Divine Sense handler"),
		Fixture.Controller->DispatchAutomationKeyPressed(EKeys::V));
	TestTrue(TEXT("one press invokes once and preserves missing GameMode fence"),
		Fixture.Controller->GetDivineSenseInputInvocationCountForAutomation()
				== Before + 1
			&& Fixture.Controller
				->GetLastDivineSenseInputResultForAutomation().Status
					== Edemo_mapShanmenDivineSenseLogicalInputStatus::
						AdapterInactive);
	Fdemo_mapShanmenDivineSenseHUDFeedbackPresentation Feedback;
	TestTrue(TEXT("a rejected physical press opens concise HUD feedback"),
		Fixture.Controller->IsDivineSenseInputFeedbackActive()
			&& Fdemo_mapShanmenDivineSenseHUDFeedbackPresentation::TryProject(
				Fixture.Controller->GetLatestDivineSenseInputResult(),
				TEXT("V"),
				Feedback)
			&& Feedback.GetReason()
				== Edemo_mapShanmenDivineSenseHUDFeedbackReason::Unavailable);
	Fixture.Controller->DispatchAutomationKeyReleased(EKeys::V);
	TestEqual(TEXT("release owns no duplicate Divine Sense binding"),
		Fixture.Controller->GetDivineSenseInputInvocationCountForAutomation(),
		Before + 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapDivineSensePhysicalRemapAndLockTest,
	"Shanmen.0_0_10.Product.DivineSensePhysicalInput.LiveRemapAndLock",
	DivineSensePhysicalInputFlags)

bool Fdemo_mapDivineSensePhysicalRemapAndLockTest::RunTest(const FString&)
{
	FScopedDivineSenseInputConfig Config;
	Fdemo_mapInputBindingSettings::Get().RestoreDefaults();
	FDivineSensePhysicalWorldFixture Fixture;
	TestTrue(TEXT("physical Divine Sense remap fixture is ready"),
		Fixture.IsReady());
	if (!Fixture.IsReady())
	{
		return false;
	}
	const Fdemo_mapInputBindingResult Remap =
		Fdemo_mapInputBindingSettings::Get().ApplyOverrideWithSwap(
			Fdemo_mapInputActionIds::DivineSense,
			EKeys::Z);
	Fixture.Controller->RebuildProductInputBindings();
	const uint64 Before = Fixture.Controller
		->GetDivineSenseInputInvocationCountForAutomation();
	Fixture.Controller->DispatchAutomationKey(EKeys::V);
	Fixture.Controller->DispatchAutomationKey(EKeys::Z);
	TestTrue(TEXT("live remap detaches V and activates Z exactly once"),
		Remap.IsSuccess()
			&& Fdemo_mapInputBindingSettings::Get().GetKey(
				Fdemo_mapInputActionIds::DivineSense) == EKeys::Z
			&& Fixture.Controller
				->GetDivineSenseInputInvocationCountForAutomation()
					== Before + 1);

	Fixture.Controller->BeginSettlementInputLock(nullptr);
	Fixture.Controller->DispatchAutomationKey(EKeys::Z);
	const Fdemo_mapShanmenDivineSenseLogicalInputResult& Locked =
		Fixture.Controller->GetLastDivineSenseInputResultForAutomation();
	TestTrue(TEXT("remapped input reaches the shared gameplay lock"),
		Fixture.Controller->GetDivineSenseInputInvocationCountForAutomation()
				== Before + 2
			&& Locked.IsValid()
			&& Locked.Status
				== Edemo_mapShanmenDivineSenseLogicalInputStatus::AdapterInactive
			&& Locked.Diagnostic.Contains(TEXT("blocked")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapDivineSenseGameModeLifecycleTest,
	"Shanmen.0_0_10.Product.DivineSensePhysicalInput.GameModeLifecycle",
	DivineSensePhysicalInputFlags)

bool Fdemo_mapDivineSenseGameModeLifecycleTest::RunTest(const FString&)
{
	FDivineSenseProductWorldFixture Fixture;
	TestTrue(TEXT("P23.0 product world fixture is ready"), Fixture.IsReady());
	if (!Fixture.IsReady())
	{
		return false;
	}

	const FGuid RunId(0x23000001, 0, 0, 1);
	FString Diagnostic;
	int32 M01RootCount = 0;
	for (TActorIterator<Ademo_mapM01Marker> It(Fixture.World); It; ++It)
	{
		if (It->GetMarkerType() == Edemo_mapM01MarkerType::MapRoot
			&& It->GetStableId() == TEXT("M01"))
		{
			++M01RootCount;
		}
	}
	TestTrue(*FString::Printf(TEXT("transient map keeps M01 suffix: %s"),
		*Fixture.World->GetMapName()),
		Fixture.World->GetMapName().EndsWith(TEXT("L_M01_Expedition")));
	TestEqual(TEXT("transient map has one authored M01 root"),
		M01RootCount, 1);
	TestTrue(TEXT("GameMode accepts the fully authored transient M01 map"),
		Fixture.GameMode->IsM01ExpeditionMap());
	Fixture.GameMode->bM01EnemyContentActive = true;
	Fixture.GameMode->M01EnemyActors.Add(Fixture.Enemy);
	const bool bRunReady =
		Fixture.GameMode->CombatRunCoordinator.TryBeginRun(
			RunId,
			Fixture.Character,
			Fixture.Health,
			Diagnostic)
		&& Fixture.GameMode->CombatRunCoordinator.TryRegisterM01Enemy(
			Fixture.Enemy,
			Diagnostic)
		&& Fixture.GameMode->TryBeginDivineSenseProductRun(Diagnostic);
	TestTrue(TEXT("M01 Run owns one ready Divine Sense product chain"),
		bRunReady);
	if (!bRunReady)
	{
		AddError(Diagnostic);
		Fixture.GameMode->DivineSenseLogicalInputAdapter.Reset();
		Fixture.GameMode->DivineSenseProductController.Reset();
		Fixture.GameMode->CombatRunCoordinator.Reset();
		return false;
	}

	const Fdemo_mapShanmenDivineSenseLogicalInputResult Pulse =
		Fixture.Controller->RouteDivineSenseInput();
	const FShanmenDivineSenseScanReceipt& Receipt =
		Fixture.GameMode->GetLatestDivineSenseReceipt();
	TestTrue(*FString::Printf(
		TEXT("one live V-route pulse reveals the registered living enemy: %s"),
		*Pulse.Diagnostic),
		Pulse.IsAccepted()
			&& Receipt.IsValid()
			&& Receipt.NumReveals() == 1
			&& Receipt.GetReveals()[0].GetObservation().GetSubjectEntityId()
				== Fixture.Enemy->GetCombatEntityId()
			&& !Receipt.GetReveals()[0].WasOccluded());
	TestTrue(TEXT("accepted pulse spends ten prototype SpiritEnergy and opens HUD reveal"),
		FMath::IsNearlyEqual(
			Fixture.GameMode->GetDivineSenseSpiritEnergy(), 90.0f)
			&& FMath::IsNearlyEqual(
				Fixture.GameMode->GetDivineSenseMaximumSpiritEnergy(), 100.0f)
			&& Fixture.GameMode->IsDivineSenseRevealActive());
	TestFalse(TEXT("accepted reveal needs no competing rejection banner"),
		Fixture.Controller->IsDivineSenseInputFeedbackActive());

	bool bRemainingPulsesAccepted = true;
	for (int32 PulseIndex = 1; PulseIndex < 10; ++PulseIndex)
	{
		bRemainingPulsesAccepted = bRemainingPulsesAccepted
			&& Fixture.Controller->RouteDivineSenseInput().IsAccepted();
	}
	TestTrue(TEXT("the remaining nine canonical pulses are accepted"),
		bRemainingPulsesAccepted
			&& FMath::IsNearlyZero(
				Fixture.GameMode->GetDivineSenseSpiritEnergy()));
	const Fdemo_mapShanmenDivineSenseLogicalInputResult Exhausted =
		Fixture.Controller->RouteDivineSenseInput();
	Fdemo_mapShanmenDivineSenseHUDFeedbackPresentation ExhaustedFeedback;
	TestTrue(TEXT("an eleventh pulse explains the exact SpiritEnergy shortage"),
		Exhausted.IsValid()
			&& Exhausted.Status
				== Edemo_mapShanmenDivineSenseLogicalInputStatus::
					ProductUnavailable
			&& Fixture.Controller->IsDivineSenseInputFeedbackActive()
			&& Fdemo_mapShanmenDivineSenseHUDFeedbackPresentation::TryProject(
				Exhausted,
				TEXT("V"),
				ExhaustedFeedback)
			&& ExhaustedFeedback.GetReason()
				== Edemo_mapShanmenDivineSenseHUDFeedbackReason::
					InsufficientSpirit
			&& ExhaustedFeedback.GetTone()
				== Edemo_mapShanmenDivineSenseHUDFeedbackTone::Warning
			&& ExhaustedFeedback.GetDisplayText().Contains(
				TEXT("NEED 10 SPIRIT · 0 AVAILABLE")));

	int32 PulseCount = 0;
	const bool bDivineSenseReleased =
		Fixture.GameMode->ReleaseDivineSenseProductRun(
			TEXT("P23.0.Automation"), PulseCount);
	const bool bRunReleased =
		Fixture.GameMode->CombatRunCoordinator.TryEndRun(RunId, Diagnostic);
	TestTrue(TEXT("teardown reports ten pulses and clears product plus reveal state"),
		bDivineSenseReleased
			&& bRunReleased
			&& PulseCount == 10
			&& Fixture.GameMode->DivineSenseProductController.IsEmpty()
			&& Fixture.GameMode->DivineSenseLogicalInputAdapter.IsEmpty()
			&& !Fixture.GameMode->GetLatestDivineSenseReceipt().IsValid()
			&& !Fixture.GameMode->IsDivineSenseRevealActive()
			&& !Fixture.Enemy->IsCombatEntityBound());
	return true;
}

#endif
