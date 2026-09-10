#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "Misc/AutomationTest.h"

#include "Components/PrimitiveComponent.h"
#include "demo_mapGameMode.h"
#include "demo_mapAttributeComponent.h"
#include "demo_mapAttributeDefinitions.h"
#include "demo_mapCombatVitalityHost.h"
#include "demo_mapEnemyCharacter.h"
#include "demo_mapInputActionRegistry.h"
#include "demo_mapInputBindingSettings.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapItemSubsystem.h"
#include "demo_mapM01EnemyIdentityComponent.h"
#include "demo_mapM01EnemyTypes.h"
#include "demo_mapPlayerController.h"
#include "demo_mapPlayerHealthComponent.h"
#include "demo_mapShanmenSwordQiProjectile.h"
#include "Engine/Engine.h"
#include "Engine/EngineBaseTypes.h"
#include "Engine/GameInstance.h"
#include "Engine/HitResult.h"
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

	const Fdemo_mapM01EnemyDefinition* FindSwordQiPhysicalTargetDefinition()
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

	Fdemo_mapEnemyEncounterIdentity MakeSwordQiPhysicalTargetIdentity(
		const Fdemo_mapM01EnemyDefinition& Definition)
	{
		Fdemo_mapEnemyEncounterIdentity Identity;
		Identity.EncounterId = Definition.EncounterId;
		Identity.RouteId = Definition.RouteId;
		Identity.SpawnMarkerId = Definition.SpawnMarkerId;
		Identity.LootTableId = Definition.CorpseIdentity;
		Identity.SkillProfileId = Definition.SkillProfileId;
		return Identity;
	}

	FHitResult MakeSwordQiPhysicalEnemyHit(Ademo_mapEnemyCharacter& Enemy)
	{
		UPrimitiveComponent* TargetComponent =
			Cast<UPrimitiveComponent>(Enemy.GetRootComponent());
		FHitResult Hit(
			&Enemy,
			TargetComponent,
			FVector(240.0, 15.0, 55.0),
			FVector::BackwardVector);
		Hit.ImpactPoint = FVector(240.0, 15.0, 55.0);
		Hit.ImpactNormal = FVector::BackwardVector;
		Hit.Item = 0;
		return Hit;
	}

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
				// Preview worlds do not advance the normal Actor initialization
				// pipeline, so make the explicitly spawned product controller
				// discoverable through the same UWorld authority used in runtime.
				World->AddController(Controller);
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
				&& (!bExpectGameMode
					|| World->GetFirstPlayerController() == Controller)
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
	Udemo_mapItemSubsystem* Items = Fixture.GameInstance
		? Fixture.GameInstance->GetSubsystem<Udemo_mapItemSubsystem>()
		: nullptr;
	Udemo_mapAttributeComponent* Attributes = Fixture.Character
		? NewObject<Udemo_mapAttributeComponent>(
			Fixture.Character,
			TEXT("P241SwordQiProductAttributes"),
			RF_Transient)
		: nullptr;
	if (Attributes && !Attributes->IsRegistered())
	{
		Attributes->RegisterComponent();
	}
	TArray<FGuid> AddedSwordIds;
	TestTrue(TEXT("real Runtime and attribute authorities are available"),
		Items && Attributes);
	if (!Items || !Attributes)
	{
		return false;
	}
	Items->ResetForAutomation();
	TestTrue(TEXT("real Runtime begins one active product Run"),
		Items->BindPlayerPawn(Fixture.Character)
			&& Fixture.Health->BindAttributeComponent(Attributes, true)
			&& Items->BindAttributeComponent(Attributes)
			&& Items->BindHealthComponent(Fixture.Health)
			&& Attributes->SetBaseValue(
				Fdemo_mapAttributeIds::Primary01,
				6.0f)
			&& Items->BeginRun().bSuccess
			&& Items->AddDefinition(
				Fdemo_mapItemIds::TrainingBlade,
				1,
				&AddedSwordIds).bSuccess
			&& AddedSwordIds.Num() == 1
			&& Items->Equip(
				AddedSwordIds[0],
				Fdemo_mapItemIds::WeaponSlot).bSuccess);
	if (AddedSwordIds.Num() != 1 || !Items->GetActiveRunId().IsValid())
	{
		return false;
	}
	float EquippedAttackPower = 0.0f;
	TestTrue(TEXT("equipped training sword contributes to final AttackPower"),
		Attributes->GetFinalValue(
			Fdemo_mapAttributeIds::AttackPower,
			EquippedAttackPower)
			&& FMath::IsNearlyEqual(EquippedAttackPower, 8.0f));
	const FGuid ProductRunId = Items->GetActiveRunId();
	Fixture.GameMode->PlayerItemSubsystem = Items;
	Fixture.GameMode->PlayerAttributeComponent = Attributes;
	const Fdemo_mapM01EnemyDefinition* TargetDefinition =
		FindSwordQiPhysicalTargetDefinition();
	FActorSpawnParameters EnemyParameters;
	EnemyParameters.ObjectFlags |= RF_Transient;
	EnemyParameters.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Ademo_mapEnemyCharacter* TargetEnemy = TargetDefinition
		? Fixture.World->SpawnActor<Ademo_mapEnemyCharacter>(
			Ademo_mapEnemyCharacter::StaticClass(),
			FTransform(FVector(240.0, 15.0, 0.0)),
			EnemyParameters)
		: nullptr;
	Udemo_mapM01EnemyIdentityComponent* TargetIdentity = TargetEnemy
		? NewObject<Udemo_mapM01EnemyIdentityComponent>(
			TargetEnemy,
			TEXT("P242SwordQiPhysicalEnemyIdentity"),
			RF_Transient)
		: nullptr;
	TestTrue(TEXT("real M01 target authority is available"),
		TargetDefinition && TargetEnemy && TargetIdentity
			&& Cast<UPrimitiveComponent>(TargetEnemy->GetRootComponent()));
	if (!TargetDefinition || !TargetEnemy || !TargetIdentity
		|| !Cast<UPrimitiveComponent>(TargetEnemy->GetRootComponent()))
	{
		return false;
	}
	TargetEnemy->AddInstanceComponent(TargetIdentity);
	const bool bTargetConfigured =
		TargetIdentity->Configure(*TargetDefinition)
		&& TargetEnemy->ConfigureEncounter(
			MakeSwordQiPhysicalTargetIdentity(*TargetDefinition),
			TargetDefinition->Tuning,
			TargetDefinition->IsElite());
	TestTrue(TEXT("real M01 target owns configured encounter identity"),
		bTargetConfigured);
	if (!bTargetConfigured)
	{
		return false;
	}
	FString Diagnostic;
	TestTrue(TEXT("coordinator, product controller and command owner bind the Runtime Run"),
		Fixture.GameMode->CombatRunCoordinator.TryBeginRun(
			ProductRunId,
			Fixture.Character,
			Fixture.Health,
			Diagnostic)
			&& Fixture.GameMode->SwordQiProductController.TryBegin(
				ProductRunId,
				Diagnostic)
			&& Fixture.GameMode->SwordQiCommandEventOwner.TryBegin(
				ProductRunId,
				Diagnostic)
			&& Fixture.GameMode->CombatRunCoordinator.TryRegisterM01Enemy(
				TargetEnemy,
				Diagnostic));
	if (!Fixture.GameMode->CombatRunCoordinator.IsReady()
		|| !Fixture.GameMode->SwordQiProductController.IsActive()
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
	Ademo_mapShanmenSwordQiProjectile* FirstProjectile =
		Fixture.GameMode->SwordQiProductController.GetSession()
			.GetHost().GetProjectile();
	const FVector FirstOrigin =
		Fixture.Character->GetActorLocation() + FVector(0.0, 0.0, 50.0);
	TestTrue(TEXT("physical Issue launches one authority-backed Sword Qi carrier"),
		Issue.IsDispatched()
			&& Issue.Kind
				== Edemo_mapShanmenSwordQiAvailabilityCommandKind::Issue
			&& Issue.Before.CanIssue()
			&& Issue.After.CanIssue()
			&& Issue.CommandEvent.IsAccepted()
			&& Issue.CommandEvent.bEventCommitted
			&& Issue.CommandEvent.Input.IsAccepted()
			&& Issue.CommandEvent.Input.Product.IsAccepted()
			&& Issue.CommandEvent.Input.Product.Item.Authorization
				.GetSourceItemInstanceId() == AddedSwordIds[0]
			&& FMath::IsNearlyEqual(
				Issue.CommandEvent.Input.Product.AttackPower,
				EquippedAttackPower)
			&& Issue.CommandEvent.Input.Sample.GetOrigin() == FirstOrigin
			&& Issue.CommandEvent.Input.Sample.GetAimDirection().Equals(
				FVector::ForwardVector)
			&& ::IsValid(FirstProjectile)
			&& FirstProjectile->GetProjectileState()
				== Edemo_mapShanmenSwordQiProjectileState::InFlight
			&& FirstProjectile->GetLaunchReceipt().IsValid()
			&& FirstProjectile->GetLaunchReceipt().GetOrigin() == FirstOrigin
			&& FirstProjectile->GetLaunchReceipt().GetDirection().Equals(
				FVector::ForwardVector));
	if (!::IsValid(FirstProjectile))
	{
		return false;
	}

	Fixture.Controller->DispatchAutomationKey(EKeys::B);
	const Fdemo_mapShanmenSwordQiAvailabilityCommandResult Busy =
		Fixture.Controller->GetLastSwordQiInputResultForAutomation();
	TestTrue(TEXT("second physical Issue freezes one real HostBusy request"),
		Busy.IsDispatched()
			&& Busy.Kind
				== Edemo_mapShanmenSwordQiAvailabilityCommandKind::Issue
			&& Busy.CommandEvent.Status
				== Edemo_mapShanmenSwordQiCommandEventStatus::InputRejected
			&& Busy.CommandEvent.bEventCommitted
			&& Busy.CommandEvent.Input.Product.Route.Status
				== Edemo_mapShanmenSwordQiProductRouteStatus::HostBusy
			&& Busy.CommandEvent.bPendingRetryStored
			&& Busy.After.CanRetry()
			&& Fixture.GameMode->SwordQiCommandEventOwner.HasPendingRetry());
	const FGuid FrozenCommandId =
		Busy.CommandEvent.Input.Product.Start.Command.GetCommandId();
	const FVector FrozenOrigin = Busy.CommandEvent.Request.GetSample().GetOrigin();
	const FVector FrozenDirection =
		Busy.CommandEvent.Request.GetSample().GetAimDirection();

	Idemo_mapCombatVitalityHost* TargetVitalityHost =
		Cast<Idemo_mapCombatVitalityHost>(TargetEnemy);
	FShanmenTargetVitalitySnapshot TargetBefore;
	const bool bTargetBeforeCaptured = TargetVitalityHost
		&& TargetVitalityHost->TryCaptureCombatVitalitySnapshot(TargetBefore);
	TestTrue(TEXT("registered M01 target exposes canonical vitality"),
		bTargetBeforeCaptured);
	if (!bTargetBeforeCaptured)
	{
		return false;
	}
	FirstProjectile->OnContact().Broadcast(
		*FirstProjectile,
		MakeSwordQiPhysicalEnemyHit(*TargetEnemy));
	FShanmenTargetVitalitySnapshot TargetAfter;
	const bool bTargetAfterCaptured =
		TargetVitalityHost->TryCaptureCombatVitalitySnapshot(TargetAfter);
	Fdemo_mapShanmenSwordQiTerminalReceipt FirstTerminal;
	TestTrue(TEXT("physical B launch reaches M01 vitality and retires before retry"),
		Fixture.GameMode->SwordQiProductController.TryRetireTerminal(
			FirstTerminal)
			&& FirstTerminal.IsValid()
			&& FirstTerminal.Kind
				== Edemo_mapShanmenSwordQiTerminalKind::Impact
			&& FirstTerminal.Delivery.IsDelivered()
			&& bTargetAfterCaptured
			&& FMath::IsNearlyEqual(
				FirstTerminal.Delivery.GetNewlyCommittedDamage(),
				0.58f,
				KINDA_SMALL_NUMBER)
			&& FMath::IsNearlyEqual(
				TargetBefore.CurrentVitality - TargetAfter.CurrentVitality,
				0.58f,
				KINDA_SMALL_NUMBER)
			&& TargetAfter.AuthorityRevision
				== TargetBefore.AuthorityRevision + 1);
	Fixture.Character->SetActorLocation(
		FVector(400.0, 200.0, 0.0),
		false,
		nullptr,
		ETeleportType::TeleportPhysics);
	Fixture.Controller->SetAutomationAimDirection(FVector::RightVector);
	TestTrue(TEXT("live attributes can move after the busy request freezes"),
		Attributes->SetBaseValue(
			Fdemo_mapAttributeIds::Primary01,
			20.0f));
	Fixture.Controller->DispatchAutomationKey(EKeys::B);
	const Fdemo_mapShanmenSwordQiAvailabilityCommandResult Retry =
		Fixture.Controller->GetLastSwordQiInputResultForAutomation();
	Ademo_mapShanmenSwordQiProjectile* RetryProjectile =
		Fixture.GameMode->SwordQiProductController.GetSession()
			.GetHost().GetProjectile();
	TestTrue(TEXT("next press launches the frozen request without resampling authority"),
		Retry.IsDispatched()
			&& Retry.Kind
				== Edemo_mapShanmenSwordQiAvailabilityCommandKind::Retry
			&& Retry.CommandEvent.IsAccepted()
			&& Retry.CommandEvent.bPendingRetryAttempt
			&& !Retry.CommandEvent.bPendingRetryStored
			&& Retry.CommandEvent.Request.Matches(Busy.CommandEvent.Request)
			&& Retry.CommandEvent.Input.Product.bReusedIntent
			&& !Retry.CommandEvent.Input.Product.Route.IsReplay()
			&& Retry.CommandEvent.Input.Product.AttackPower
				== EquippedAttackPower
			&& Retry.CommandEvent.Input.Product.Start.Command.GetCommandId()
				== FrozenCommandId
			&& Retry.CommandEvent.Input.Sample.GetOrigin() == FrozenOrigin
			&& Retry.CommandEvent.Input.Sample.GetAimDirection().Equals(
				FrozenDirection)
			&& Retry.After.CanIssue()
			&& !Fixture.GameMode->SwordQiCommandEventOwner.HasPendingRetry()
			&& ::IsValid(RetryProjectile)
			&& RetryProjectile != FirstProjectile
			&& RetryProjectile->GetProjectileState()
				== Edemo_mapShanmenSwordQiProjectileState::InFlight
			&& RetryProjectile->GetLaunchReceipt().GetOrigin() == FrozenOrigin
			&& RetryProjectile->GetLaunchReceipt().GetDirection().Equals(
				FrozenDirection));

	Fdemo_mapShanmenSwordQiTerminalReceipt RetryTerminal;
	TestTrue(TEXT("retried carrier retires before the shared UI lock check"),
		Fixture.GameMode->SwordQiProductController.TryInterrupt()
			&& Fixture.GameMode->SwordQiProductController.TryRetireTerminal(
				RetryTerminal)
			&& RetryTerminal.IsValid());

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
			&& Fixture.GameMode->SwordQiCommandEventOwner
				.GetNextEventSequence() == 3
			&& Fixture.Controller
				->GetSwordQiInputInvocationCountForAutomation() == Before + 4);
	Fixture.Controller->EndSettlementInputLock();

	Fdemo_mapShanmenSwordQiCommandEventEndSummary CommandSummary;
	Fdemo_mapShanmenSwordQiControllerEndSummary ProductSummary;
	Fdemo_mapSettlementSummary SettlementSummary;
	TestTrue(TEXT("the complete physical product Run releases cleanly"),
		Fixture.GameMode->SwordQiCommandEventOwner.TryEnd(
			ProductRunId,
			CommandSummary,
			Diagnostic)
			&& CommandSummary.IsValid()
			&& CommandSummary.CommittedEventCount == 2
			&& Fixture.GameMode->SwordQiProductController.TryEnd(
				ProductRunId,
				ProductSummary,
				Diagnostic)
			&& ProductSummary.IsValid()
			&& ProductSummary.CapturedIntentCount == 2
			&& ProductSummary.ProcessedCommandCount == 2
			&& Fixture.GameMode->CombatRunCoordinator.TryEndRun(
				ProductRunId,
				Diagnostic)
			&& Items->RequestSettlement(
				Edemo_mapRunEndReason::Death,
				SettlementSummary).bSuccess);
	return true;
}

#endif
