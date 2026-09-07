#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenPreparationAdapter.h"

#include "Components/BoxComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "ShanmenCombatTags.h"
#include "ShanmenItemRepository.h"
#include "ShanmenItemTags.h"
#include "demo_map0909BSectWarehouseService.h"
#include "demo_mapAttributeComponent.h"
#include "demo_mapCombatRunCoordinator.h"
#include "demo_mapEnemyCharacter.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapItemSubsystem.h"
#include "demo_mapM01EnemyIdentityComponent.h"
#include "demo_mapM01EnemyTypes.h"
#include "demo_mapProfileRepository.h"
#include "demo_mapProfileSessionSubsystem.h"
#include "demo_mapPlayerHealthComponent.h"
#include "demo_mapRewardAffix.h"
#include "demo_mapShanmenControlledWeaponActiveRunRoute.h"
#include "demo_mapShanmenControlledWeaponActor.h"
#include "demo_mapShanmenControlledWeaponRunLifecycle.h"
#include "demo_mapShanmenControlledWeaponWorldLifecycle.h"
#include "demo_mapShanmenControlledWeaponWorldThreatSampler.h"
#include "demo_mapShanmenItemCutover.h"
#include "demo_mapShanmenItemMetadataAdapter.h"
#include "demo_mapShanmenRunLifecycleAdapter.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	FString NewPreparationAdapterRoot(const TCHAR* Label)
	{
		return FPaths::Combine(
			FPaths::ProjectSavedDir(), TEXT("Automation"),
			TEXT("Dev.D.UE.0.0.10.P1.6.r0"), Label,
			FGuid::NewGuid().ToString(EGuidFormats::Digits));
	}

	void RemovePreparationAdapterRoot(const FString& Root)
	{
		IFileManager::Get().DeleteDirectory(*Root, false, true);
	}

	bool ReadBytes(const FString& Path, TArray<uint8>& OutBytes)
	{
		OutBytes.Reset();
		return FFileHelper::LoadFileToArray(OutBytes, *Path);
	}

	const FShanmenItemDefinition* FindAuthorityDefinition(
		const FShanmenItemAuthoritySnapshot& Snapshot,
		FName DefinitionId)
	{
		return Snapshot.Definitions.FindByPredicate([DefinitionId](
			const FShanmenItemDefinition& Definition)
		{
			return Definition.DefinitionId == DefinitionId;
		});
	}

	const Fdemo_mapProfilePreparationStashRow* FindRow(
		const Fdemo_mapProfilePreparationSnapshot& Snapshot,
		const FGuid& ItemId)
	{
		return Snapshot.OrderedPermanentStashRows.FindByPredicate([&ItemId](
			const Fdemo_mapProfilePreparationStashRow& Row)
		{
			return Row.ItemInstanceId == ItemId;
		});
	}

	struct FPreparationAdapterFixture
	{
		FString Root;
		Fdemo_mapProfileStorageContext Storage;
		Fdemo_mapPersistentProfile SeedProfile;
		FGuid TrainingBladeId;
		FGuid FlyingSwordId;
		FGuid HeavyBladeId;
		FGuid ArmorId;
		FGuid DustId;
		FGuid PillOneId;
		FGuid PillTwoId;
		FGuid BackpackId;
		TArray<FGuid> ExtraMaterialIds;
		UGameInstance* GameInstance = nullptr;
		Udemo_mapShanmenItemAuthoritySubsystem* Authority = nullptr;
		Udemo_mapProfileSessionSubsystem* Session = nullptr;
		Fdemo_map0909BSectWarehouseService Warehouse;

		bool Seed(FAutomationTestBase& Test, const TCHAR* Label)
		{
			Root = NewPreparationAdapterRoot(Label);
			Storage = Fdemo_mapProfileStorageContext::ForRoot(Root);
			Fdemo_mapProfileRepository Repository;
			SeedProfile = Repository.CreateFreshProfile();
			for (const Fdemo_mapPersistentItemRecord& Item : SeedProfile.PermanentStash)
			{
				if (Item.ItemDefinitionId == Fdemo_mapItemIds::TrainingBlade)
				{
					TrainingBladeId = Item.ItemInstanceId;
				}
				else if (Item.ItemDefinitionId == Fdemo_mapItemIds::TrainingVest)
				{
					ArmorId = Item.ItemInstanceId;
				}
			}
			Fdemo_mapPersistentItemRecord Heavy;
			HeavyBladeId = Heavy.ItemInstanceId = FGuid::NewGuid();
			Heavy.ItemDefinitionId = Fdemo_mapItemIds::HeavyPracticeBlade;
			Heavy.StackCount = 1;
			Heavy.PersistentDomain = Edemo_mapPersistentDomain::PermanentStash;
			SeedProfile.PermanentStash.Add(Heavy);

			Fdemo_mapPersistentItemRecord FlyingSword;
			FlyingSwordId = FlyingSword.ItemInstanceId = FGuid::NewGuid();
			FlyingSword.ItemDefinitionId =
				Fdemo_mapItemIds::TrainingFlyingSword;
			FlyingSword.StackCount = 1;
			FlyingSword.PersistentDomain =
				Edemo_mapPersistentDomain::PermanentStash;
			SeedProfile.PermanentStash.Add(FlyingSword);

			Fdemo_mapPersistentItemRecord Dust;
			DustId = Dust.ItemInstanceId = FGuid::NewGuid();
			Dust.ItemDefinitionId = Fdemo_mapItemIds::SpiritDust;
			Dust.StackCount = 3;
			Dust.PersistentDomain = Edemo_mapPersistentDomain::PermanentStash;
			SeedProfile.PermanentStash.Add(Dust);

			Fdemo_mapPersistentItemRecord PillOne;
			PillOneId = PillOne.ItemInstanceId = FGuid::NewGuid();
			PillOne.ItemDefinitionId = Fdemo_mapItemIds::HealingPillLevel1;
			PillOne.StackCount = 3;
			PillOne.PersistentDomain = Edemo_mapPersistentDomain::PermanentStash;
			SeedProfile.PermanentStash.Add(PillOne);
			Fdemo_mapPersistentItemRecord PillTwo;
			PillTwoId = PillTwo.ItemInstanceId = FGuid::NewGuid();
			PillTwo.ItemDefinitionId = Fdemo_mapItemIds::HealingPillLevel2;
			PillTwo.StackCount = 2;
			PillTwo.PersistentDomain = Edemo_mapPersistentDomain::PermanentStash;
			SeedProfile.PermanentStash.Add(PillTwo);
			Fdemo_mapPersistentItemRecord Backpack;
			BackpackId = Backpack.ItemInstanceId = FGuid::NewGuid();
			Backpack.ItemDefinitionId = Fdemo_mapItemIds::BackpackLevel1;
			Backpack.StackCount = 1;
			Backpack.PersistentDomain =
				Edemo_mapPersistentDomain::PermanentStash;
			SeedProfile.PermanentStash.Add(Backpack);
			for (int32 Index = 0; Index < 4; ++Index)
			{
				Fdemo_mapPersistentItemRecord Material;
				Material.ItemInstanceId = FGuid::NewGuid();
				Material.ItemDefinitionId = Fdemo_mapItemIds::SpiritWoodLevel1;
				Material.StackCount = Index + 1;
				Material.PersistentDomain =
					Edemo_mapPersistentDomain::PermanentStash;
				ExtraMaterialIds.Add(Material.ItemInstanceId);
				SeedProfile.PermanentStash.Add(Material);
			}
			SeedProfile.PreparationLayout.WeaponItemInstanceId = TrainingBladeId;
			const Fdemo_mapProfileSaveResult Saved =
				Repository.SaveProfile(SeedProfile, Storage);
			if (!TrainingBladeId.IsValid() || !FlyingSwordId.IsValid()
				|| !ArmorId.IsValid()
				|| !Saved.IsSuccess())
			{
				Test.AddError(FString::Printf(
					TEXT("P1.6 seed Profile failed: %s"), *Saved.Diagnostic));
				return false;
			}
			return true;
		}

		bool StartGameInstance(FAutomationTestBase& Test)
		{
			if (!GEngine)
			{
				Test.AddError(TEXT("GEngine is unavailable for the P1.6 fixture."));
				return false;
			}
			GameInstance = NewObject<UGameInstance>(GEngine, NAME_None, RF_Transient);
			if (!GameInstance)
			{
				Test.AddError(TEXT("Could not allocate the P1.6 GameInstance."));
				return false;
			}
			GameInstance->AddToRoot();
			GameInstance->Init();
			Authority = GameInstance->GetSubsystem<
				Udemo_mapShanmenItemAuthoritySubsystem>();
			Session = GameInstance->GetSubsystem<
				Udemo_mapProfileSessionSubsystem>();
			return Authority && Session;
		}

		bool StartAndCutover(FAutomationTestBase& Test, const TCHAR* Label)
		{
			if (!Seed(Test, Label) || !StartGameInstance(Test))
			{
				return false;
			}
			const Fdemo_mapProfileSessionInitializeResult Initialized =
				Session->InitializeSession(Storage);
			Fdemo_map0909BWarehousePresentation Presentation;
			FString Diagnostic;
			if (!Initialized.IsReady()
				|| !Warehouse.OpenForSect(
					Root, Initialized.Snapshot,
					Edemo_map0909BTopState::AtSect,
					Presentation, Diagnostic))
			{
				Test.AddError(FString::Printf(
					TEXT("P1.6 stable source open failed: %s"), *Diagnostic));
				return false;
			}
			const Fdemo_mapShanmenItemCutoverResult Cutover =
				Fdemo_mapShanmenItemCutoverCoordinator::Execute(
					Storage, SeedProfile.ProfileId, *Authority,
					*Session, Warehouse);
			if (!Cutover.IsReady())
			{
				Test.AddError(FString::Printf(
					TEXT("P1.6 cutover failed: %s"), *Cutover.Diagnostic));
				return false;
			}
			return true;
		}

		bool RestartAndBind(FAutomationTestBase& Test)
		{
			Stop();
			if (!StartGameInstance(Test)
				|| !Session->InitializeSession(Storage).IsReady())
			{
				return false;
			}
			const Fdemo_mapShanmenItemAuthorityBindResult Bound =
				Authority->BindExisting(Storage, SeedProfile.ProfileId);
			if (!Bound.IsReady())
			{
				Test.AddError(FString::Printf(
					TEXT("P1.6 restart bind failed: %s"), *Bound.Diagnostic));
				return false;
			}
			return true;
		}

		void Stop()
		{
			if (!GameInstance) return;
			GameInstance->Shutdown();
			Authority = nullptr;
			Session = nullptr;
			GameInstance->RemoveFromRoot();
			GameInstance->MarkAsGarbage();
			GameInstance = nullptr;
			CollectGarbage(RF_NoFlags);
		}

		~FPreparationAdapterFixture()
		{
			Stop();
			if (!Root.IsEmpty()) RemovePreparationAdapterRoot(Root);
		}
	};

	struct FControlledWeaponWorldFixture
	{
		UWorld* World = nullptr;
		APawn* Player = nullptr;
		UBoxComponent* PlayerRoot = nullptr;
		Udemo_mapPlayerHealthComponent* PlayerHealth = nullptr;
		Fdemo_mapCombatRunCoordinator Coordinator;
		Fdemo_mapShanmenControlledWeaponRunHost Host;
		Fdemo_mapShanmenControlledWeaponWorldLifecycle Lifecycle;
		Fdemo_mapShanmenControlledWeaponThreatSampleRouter ThreatRouter;
		Fdemo_mapShanmenControlledWeaponWorldThreatSampler ThreatSampler;
		Fdemo_mapShanmenCombatRunFixedTimeline Timeline;
		FString Diagnostic;

		bool Start(
			FAutomationTestBase& Test,
			UGameInstance* GameInstance,
			const FGuid& ActiveRunId)
		{
			if (!GEngine || !GameInstance || !ActiveRunId.IsValid())
			{
				Test.AddError(TEXT(
					"P21.2 World fixture requires Engine, GameInstance, and Run identity."));
				return false;
			}
			World = NewObject<UWorld>(
				GetTransientPackage(), NAME_None, RF_Transient);
			if (!World)
			{
				Test.AddError(TEXT("P21.2 could not allocate a preview World."));
				return false;
			}
			World->WorldType = EWorldType::GamePreview;
			FWorldContext& Context =
				GEngine->CreateNewWorldContext(EWorldType::GamePreview);
			Context.OwningGameInstance = GameInstance;
			Context.SetCurrentWorld(World);
			World->SetGameInstance(GameInstance);
			World->InitializeNewWorld(
				UWorld::InitializationValues()
					.InitializeScenes(true)
					.AllowAudioPlayback(false)
					.RequiresHitProxies(false)
					.CreatePhysicsScene(true)
					.CreateNavigation(false)
					.CreateAISystem(false)
					.ShouldSimulatePhysics(false)
					.EnableTraceCollision(true)
					.SetTransactional(false)
					.CreateFXSystem(false));

			FActorSpawnParameters Parameters;
			Parameters.ObjectFlags |= RF_Transient;
			Parameters.SpawnCollisionHandlingOverride =
				ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			Player = World->SpawnActor<APawn>(
				APawn::StaticClass(),
				FTransform(FVector(25.0f, -10.0f, 5.0f)),
				Parameters);
			PlayerRoot = Player
				? NewObject<UBoxComponent>(
					Player, TEXT("P212PlayerRoot"), RF_Transient)
				: nullptr;
			PlayerHealth = Player
				? NewObject<Udemo_mapPlayerHealthComponent>(
					Player, TEXT("P212PlayerHealth"), RF_Transient)
				: nullptr;
			if (!Player || !PlayerRoot || !PlayerHealth)
			{
				Test.AddError(TEXT(
					"P21.2 could not construct the canonical player Actor."));
				return false;
			}
			Player->SetRootComponent(PlayerRoot);
			Player->AddInstanceComponent(PlayerRoot);
			Player->AddInstanceComponent(PlayerHealth);
			if (!Coordinator.TryBeginRun(
					ActiveRunId,
					Player,
					PlayerHealth,
					Diagnostic)
				|| !Timeline.TryBegin(ActiveRunId, Diagnostic))
			{
				Test.AddError(FString::Printf(
					TEXT("P21.2 CombatRunCoordinator failed: %s"),
					*Diagnostic));
				return false;
			}
			return true;
		}

		Ademo_mapEnemyCharacter* SpawnRegisteredEnemy(
			FAutomationTestBase& Test,
			const FVector& Location)
		{
			const Fdemo_mapM01EnemyDefinition* Definition = nullptr;
			for (const Fdemo_mapM01EnemyDefinition& Candidate :
				Fdemo_mapM01EnemyConfig::GetDefinitions())
			{
				if (Candidate.Archetype
					== Edemo_mapM01EnemyArchetype::StandardSkirmisher)
				{
					Definition = &Candidate;
					break;
				}
			}
			if (!World || !Definition)
			{
				Test.AddError(TEXT(
					"P21.3 could not resolve the canonical M01 enemy definition."));
				return nullptr;
			}

			FActorSpawnParameters Parameters;
			Parameters.ObjectFlags |= RF_Transient;
			Parameters.SpawnCollisionHandlingOverride =
				ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			Ademo_mapEnemyCharacter* Enemy =
				World->SpawnActor<Ademo_mapEnemyCharacter>(
					Ademo_mapEnemyCharacter::StaticClass(),
					FTransform(Location),
					Parameters);
			Udemo_mapM01EnemyIdentityComponent* Identity = Enemy
				? NewObject<Udemo_mapM01EnemyIdentityComponent>(
					Enemy, TEXT("P213EnemyIdentity"), RF_Transient)
				: nullptr;
			Fdemo_mapEnemyEncounterIdentity Encounter;
			Encounter.EncounterId = Definition->EncounterId;
			Encounter.RouteId = Definition->RouteId;
			Encounter.SpawnMarkerId = Definition->SpawnMarkerId;
			Encounter.LootTableId = Definition->CorpseIdentity;
			Encounter.SkillProfileId = Definition->SkillProfileId;
			if (!Enemy || !Identity)
			{
				Test.AddError(TEXT("P21.3 could not spawn the overlap target."));
				return nullptr;
			}
			Enemy->AddInstanceComponent(Identity);
			Identity->RegisterComponent();
			Enemy->SetCombatSuppressed(true);
			if (!Identity->Configure(*Definition)
				|| !Enemy->ConfigureEncounter(
					Encounter, Definition->Tuning, Definition->IsElite())
				|| !Coordinator.TryRegisterM01Enemy(Enemy, Diagnostic))
			{
				Test.AddError(FString::Printf(
					TEXT("P21.3 could not register the overlap target: %s"),
					*Diagnostic));
				return nullptr;
			}
			World->UpdateWorldComponents(true, false);
			return Enemy;
		}

		int32 CountControlledWeaponActors() const
		{
			if (!World)
			{
				return 0;
			}
			int32 Count = 0;
			for (TActorIterator<Ademo_mapShanmenControlledWeaponActor>
				Iterator(World); Iterator; ++Iterator)
			{
				++Count;
			}
			return Count;
		}

		void Stop()
		{
			if (Coordinator.IsActive())
			{
				const Fdemo_mapShanmenControlledWeaponRunEndResult Ended =
					Fdemo_mapShanmenControlledWeaponRunLifecycle::TryEndRun(
						Host, Coordinator);
				if (Ended.IsEnded())
				{
					Lifecycle.TryEndAfterRun(
						Ended.RunId, Host, Diagnostic);
				}
			}
			Host.Reset();
			Lifecycle.Reset();
			ThreatRouter.Reset();
			ThreatSampler.Reset();
			Timeline.Reset();
			Coordinator.Reset();
			if (World)
			{
				World->DestroyWorld(false);
				if (GEngine)
				{
					GEngine->DestroyWorldContext(World);
				}
				World = nullptr;
			}
			Player = nullptr;
			PlayerRoot = nullptr;
			PlayerHealth = nullptr;
		}

		~FControlledWeaponWorldFixture()
		{
			Stop();
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenPreparationProjectionContractTest,
	"Shanmen.0_0_10.Items.PreparationAdapter.CapabilityAndProjection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenPreparationProjectionContractTest::RunTest(const FString&)
{
	FPreparationAdapterFixture Fixture;
	if (!Fixture.StartAndCutover(*this, TEXT("Projection"))) return false;
	FShanmenItemAuthoritySnapshot AuthoritySnapshot;
	TestTrue(TEXT("Ready authority snapshot is readable"),
		Fixture.Authority->TryCaptureSnapshot(AuthoritySnapshot));
	const FShanmenItemDefinition* Weapon = FindAuthorityDefinition(
		AuthoritySnapshot, Fdemo_mapItemIds::TrainingBlade);
	const FShanmenItemDefinition* Material = FindAuthorityDefinition(
		AuthoritySnapshot, Fdemo_mapItemIds::SpiritDust);
	TestTrue(TEXT("Migration grants equipment only the deployment capability"),
		Weapon
		&& Weapon->Supports(EShanmenItemResourceKind::DeploymentLock)
		&& !Weapon->Supports(EShanmenItemResourceKind::Quantity));
	TestTrue(TEXT("Migration keeps stackables quantity-exclusive"),
		Material
		&& Material->Supports(EShanmenItemResourceKind::Quantity)
		&& !Material->Supports(EShanmenItemResourceKind::DeploymentLock));

	const Fdemo_mapProfilePreparationSnapshot Projection =
		Fixture.Session->GetPreparationSnapshot();
	const Fdemo_mapProfilePreparationStashRow* DustRow =
		FindRow(Projection, Fixture.DustId);
	TestTrue(TEXT("Existing equipment is the no-history migration baseline"),
		Projection.SelectedWeaponId == Fixture.TrainingBladeId
		&& Projection.SaveGeneration == AuthoritySnapshot.AuthorityRevision);
	TestTrue(TEXT("All authority items project and the atomic product Start is enabled"),
		Projection.OrderedPermanentStashRows.Num() == AuthoritySnapshot.Items.Num()
		&& DustRow && DustRow->bMaterialSelectionEligible
		&& Projection.bCanStartRun
		&& Projection.VisibleDiagnostic.Contains(TEXT("原子"))
		&& Projection.OrderedSelectedMaterialIds.IsEmpty()
		&& Projection.HotbarBindings.SlotBindings.Num()
			== Fdemo_mapPersistentPreparationLayout::HotbarSlotCount);
	TestTrue(TEXT("Canonical authority warehouse projects all thirty cells"),
		Projection.WarehouseLayout.bInitialized
		&& Projection.WarehouseLayout.SlotItemInstanceIds.Num()
			== Fdemo_mapPersistentWarehouseLayout::SlotCount);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenPreparationDurableRestartTest,
	"Shanmen.0_0_10.Items.PreparationAdapter.ReplaceClearRestart",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenPreparationDurableRestartTest::RunTest(const FString&)
{
	FPreparationAdapterFixture Fixture;
	if (!Fixture.StartAndCutover(*this, TEXT("Restart"))) return false;
	TArray<uint8> ProfileBefore;
	TestTrue(TEXT("Legacy Profile bytes are captured after cutover"),
		ReadBytes(Fixture.Storage.PrimaryPath(), ProfileBefore));

	const Fdemo_mapProfilePreparationSelectionResult Replace =
		Fixture.Session->SetPreparationEquipment(
			Fdemo_mapItemIds::WeaponSlot, Fixture.HeavyBladeId);
	FShanmenItemAuthoritySnapshot AfterReplace;
	Fixture.Authority->TryCaptureSnapshot(AfterReplace);
	const int32 ReplaceRevision = AfterReplace.AuthorityRevision;
	const Fdemo_mapProfilePreparationSelectionResult Replay =
		Fixture.Session->SetPreparationEquipment(
			Fdemo_mapItemIds::WeaponSlot, Fixture.HeavyBladeId);
	FShanmenItemAuthoritySnapshot AfterReplay;
	Fixture.Authority->TryCaptureSnapshot(AfterReplay);
	TestTrue(TEXT("Replacement is durable and exact replay is mutation-free"),
		Replace.IsAccepted() && Replay.IsAccepted()
		&& Fixture.Session->GetPreparationSnapshot().SelectedWeaponId
			== Fixture.HeavyBladeId
		&& AfterReplay.AuthorityRevision == ReplaceRevision);

	const Fdemo_mapProfilePreparationSelectionResult Clear =
		Fixture.Session->SetPreparationEquipment(
			Fdemo_mapItemIds::WeaponSlot, FGuid());
	const Fdemo_mapProfilePreparationSelectionResult ClearReplay =
		Fixture.Session->SetPreparationEquipment(
			Fdemo_mapItemIds::WeaponSlot, FGuid());
	FShanmenItemAuthoritySnapshot AfterClearReplay;
	Fixture.Authority->TryCaptureSnapshot(AfterClearReplay);
	const int32 ClearRevision = AfterClearReplay.AuthorityRevision;
	const Fdemo_mapProfilePreparationSnapshot ClearedProjection =
		Fixture.Session->GetPreparationSnapshot();
	TestTrue(TEXT("Terminal latest reservation is a durable empty tombstone"),
		Clear.IsAccepted() && ClearReplay.IsAccepted()
		&& !ClearedProjection.SelectedWeaponId.IsValid()
		&& !ClearedProjection.bCanStartRun
		&& ClearedProjection.VisibleDiagnostic.Contains(TEXT("选择")));

	const Fdemo_mapProfilePreparationSelectionResult Reselect =
		Fixture.Session->SetPreparationEquipment(
			Fdemo_mapItemIds::WeaponSlot, Fixture.HeavyBladeId);
	FShanmenItemAuthoritySnapshot BeforeRestart;
	Fixture.Authority->TryCaptureSnapshot(BeforeRestart);
	int32 ActiveWeaponIntents = 0;
	for (const FShanmenItemReservationSnapshot& Reservation :
		BeforeRestart.Reservations)
	{
		ActiveWeaponIntents += Reservation.PurposeId
				== FName(TEXT("Shanmen.Preparation.Weapon"))
			&& Reservation.State == EShanmenItemReservationState::Reserved
			? 1 : 0;
	}
	TestTrue(TEXT("Reselect after tombstone creates one new active intent"),
		Reselect.IsAccepted()
		&& BeforeRestart.AuthorityRevision > ClearRevision
		&& ActiveWeaponIntents == 1);

	if (!Fixture.RestartAndBind(*this)) return false;
	TArray<uint8> ProfileAfter;
	const Fdemo_mapProfilePreparationSnapshot RestartProjection =
		Fixture.Session->GetPreparationSnapshot();
	TestTrue(TEXT("Restart restores selection only from ShanmenItems"),
		RestartProjection.SelectedWeaponId == Fixture.HeavyBladeId
		&& ReadBytes(Fixture.Storage.PrimaryPath(), ProfileAfter)
		&& ProfileAfter == ProfileBefore);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenPreparationFailureFenceTest,
	"Shanmen.0_0_10.Items.PreparationAdapter.RejectionAndRollback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenPreparationFailureFenceTest::RunTest(const FString&)
{
	FPreparationAdapterFixture Fixture;
	if (!Fixture.StartAndCutover(*this, TEXT("FailureFence"))) return false;
	TArray<uint8> ProfileBefore;
	ReadBytes(Fixture.Storage.PrimaryPath(), ProfileBefore);
	FShanmenItemAuthoritySnapshot Before;
	Fixture.Authority->TryCaptureSnapshot(Before);

	const Fdemo_mapProfilePreparationSelectionResult Incompatible =
		Fixture.Session->SetPreparationEquipment(
			Fdemo_mapItemIds::WeaponSlot, Fixture.ArmorId);
	const Fdemo_mapProfilePreparationSelectionResult Material =
		Fixture.Session->SetPreparationMaterial(Fixture.ArmorId, true);
	const Fdemo_mapWarehouseMoveResult WarehouseMove =
		Fixture.Session->MoveWarehouseItem(0, 1);
	FShanmenItemAuthoritySnapshot AfterRejected;
	Fixture.Authority->TryCaptureSnapshot(AfterRejected);
	TestTrue(TEXT("Invalid and not-yet-adapted paths mutate neither authority"),
		Incompatible.Status
			== Edemo_mapProfilePreparationSelectionStatus::EquipmentSlotRejected
		&& Material.Status
			== Edemo_mapProfilePreparationSelectionStatus::MaterialRejected
		&& WarehouseMove.Status == Edemo_mapWarehouseMoveStatus::SessionNotReady
		&& AfterRejected == Before);

	Fixture.Authority->SetInjectedFailureForAutomation(
		EShanmenItemStoreFailureStage::WriteTemp);
	const Fdemo_mapProfilePreparationSelectionResult Failed =
		Fixture.Session->SetPreparationEquipment(
			Fdemo_mapItemIds::WeaponSlot, Fixture.HeavyBladeId);
	FShanmenItemAuthoritySnapshot AfterFailure;
	Fixture.Authority->TryCaptureSnapshot(AfterFailure);
	// The authority test hook is intentionally sticky, unlike a transient OS
	// write error. Remove it before exercising the autonomous retry path.
	Fixture.Authority->SetInjectedFailureForAutomation(
		EShanmenItemStoreFailureStage::None);
	const Fdemo_mapProfilePreparationSelectionResult Retry =
		Fixture.Session->SetPreparationEquipment(
			Fdemo_mapItemIds::WeaponSlot, Fixture.HeavyBladeId);
	TArray<uint8> ProfileAfter;
	TestTrue(TEXT("Persistence failure rolls back and an autonomous retry succeeds"),
		Failed.Status
			== Edemo_mapProfilePreparationSelectionStatus::AuthorityCommandRejected
		&& AfterFailure == Before
		&& Retry.IsAccepted()
		&& Fixture.Session->GetPreparationSnapshot().SelectedWeaponId
			== Fixture.HeavyBladeId);
	TestTrue(TEXT("Every P1.6 item path leaves the retired Profile byte-identical"),
		ReadBytes(Fixture.Storage.PrimaryPath(), ProfileAfter)
		&& ProfileAfter == ProfileBefore);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenPreparationRunInventoryRestartTest,
	"Shanmen.0_0_10.Items.PreparationAdapter.QuantityHotbarRestart",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenPreparationRunInventoryRestartTest::RunTest(const FString&)
{
	FPreparationAdapterFixture Fixture;
	if (!Fixture.StartAndCutover(*this, TEXT("RunInventoryRestart")))
	{
		return false;
	}
	TArray<uint8> ProfileBefore;
	TestTrue(TEXT("Retired Profile bytes captured before P1.7 commands"),
		ReadBytes(Fixture.Storage.PrimaryPath(), ProfileBefore));

	const Fdemo_mapProfilePreparationSelectionResult Dust =
		Fixture.Session->SetPreparationMaterial(Fixture.DustId, true);
	const Fdemo_mapProfilePreparationSelectionResult PillOne =
		Fixture.Session->SetPreparationMaterial(Fixture.PillOneId, true);
	const Fdemo_mapProfilePreparationSelectionResult PillTwo =
		Fixture.Session->SetPreparationMaterial(Fixture.PillTwoId, true);
	const TArray<FGuid> ExpectedOrder =
		{ Fixture.DustId, Fixture.PillOneId, Fixture.PillTwoId };
	TestTrue(TEXT("Three complete stacks reserve in explicit selection order"),
		Dust.IsAccepted() && PillOne.IsAccepted() && PillTwo.IsAccepted()
		&& Fixture.Session->GetPreparationSnapshot()
			.OrderedSelectedMaterialIds == ExpectedOrder);

	FShanmenItemAuthoritySnapshot ReservedSnapshot;
	TestTrue(TEXT("Reserved authority snapshot captured"),
		Fixture.Authority->TryCaptureSnapshot(ReservedSnapshot));
	FShanmenItemRepository Repository;
	EShanmenItemTransactionError LoadError =
		EShanmenItemTransactionError::None;
	TestTrue(TEXT("Reserved authority remains repository-valid"),
		Repository.TryLoadSnapshot(ReservedSnapshot, &LoadError));
	int32 ActivePreparationQuantityReservations = 0;
	for (const FShanmenItemReservationSnapshot& Reservation :
		ReservedSnapshot.Reservations)
	{
		if (Reservation.ResourceKind == EShanmenItemResourceKind::Quantity
			&& Reservation.State == EShanmenItemReservationState::Reserved
			&& Reservation.PurposeId.ToString().StartsWith(
				TEXT("Shanmen.Preparation.RunInventory.r1.O")))
		{
			++ActivePreparationQuantityReservations;
			const FShanmenItemInstance* Item =
				Repository.FindItem(Reservation.ItemInstanceId);
			TestTrue(TEXT("Preparation Quantity reserve freezes one whole stack"),
				Item && Reservation.Amount == Item->Quantity
				&& Repository.GetAvailableResource(
					Item->ItemInstanceId,
					EShanmenItemResourceKind::Quantity) == 0);
		}
	}
	TestEqual(TEXT("Exactly one active Quantity reserve exists per selected stack"),
		ActivePreparationQuantityReservations, 3);

	TestTrue(TEXT("Occupied slot replacement and stable-item movement succeed"),
		Fixture.Session->SetPreparationHotbarSlot(
			2, Fixture.PillOneId).IsAccepted()
		&& Fixture.Session->SetPreparationHotbarSlot(
			2, Fixture.PillTwoId).IsAccepted()
		&& Fixture.Session->SetPreparationHotbarSlot(
			9, Fixture.PillOneId).IsAccepted());
	const Fdemo_mapProfilePreparationSnapshot Bound =
		Fixture.Session->GetPreparationSnapshot();
	TestTrue(TEXT("Hotbar keeps exact nine slots without duplicate bindings"),
		Bound.HotbarBindings.SlotBindings.Num() == 9
		&& Bound.HotbarBindings.SlotBindings[1] == Fixture.PillTwoId
		&& Bound.HotbarBindings.SlotBindings[8] == Fixture.PillOneId
		&& Bound.HotbarBindings.SlotBindings.FilterByPredicate(
			[&Fixture](const FGuid& Id)
			{
				return Id == Fixture.PillOneId;
			}).Num() == 1);

	if (!Fixture.RestartAndBind(*this))
	{
		return false;
	}
	const Fdemo_mapProfilePreparationSnapshot Restarted =
		Fixture.Session->GetPreparationSnapshot();
	TestTrue(TEXT("Restart restores order and Hotbar only from ShanmenItems"),
		Restarted.OrderedSelectedMaterialIds == ExpectedOrder
		&& Restarted.HotbarBindings.SlotBindings[1]
			== Fixture.PillTwoId
		&& Restarted.HotbarBindings.SlotBindings[8]
			== Fixture.PillOneId);
	TestTrue(TEXT("Removing a selected stack clears its encoded Hotbar binding"),
		Fixture.Session->SetPreparationMaterial(
			Fixture.PillOneId, false).IsAccepted()
		&& !Fixture.Session->GetPreparationSnapshot()
			.HotbarBindings.SlotBindings[8].IsValid());
	TArray<uint8> ProfileAfter;
	TestTrue(TEXT("P1.7 selection and Hotbar commands never rewrite retired Profile"),
		ReadBytes(Fixture.Storage.PrimaryPath(), ProfileAfter)
		&& ProfileAfter == ProfileBefore);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenPreparationRunInventoryFenceTest,
	"Shanmen.0_0_10.Items.PreparationAdapter.QuantityCapacityAndFailureFence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenPreparationRunInventoryFenceTest::RunTest(const FString&)
{
	FPreparationAdapterFixture Fixture;
	if (!Fixture.StartAndCutover(*this, TEXT("RunInventoryFence")))
	{
		return false;
	}
	TArray<FGuid> SixStacks =
		{ Fixture.DustId, Fixture.PillOneId, Fixture.PillTwoId };
	SixStacks.Append(Fixture.ExtraMaterialIds.GetData(), 3);
	for (const FGuid& ItemId : SixStacks)
	{
		TestTrue(TEXT("Complete stack fits one of six base cells"),
			Fixture.Session->SetPreparationMaterial(
				ItemId, true).IsAccepted());
	}
	TestTrue(TEXT("Seventh stack fails closed without selected storage"),
		Fixture.Session->SetPreparationMaterial(
			Fixture.ExtraMaterialIds[3], true).Status
			== Edemo_mapProfilePreparationSelectionStatus::SelectionLimitExceeded);
	TestTrue(TEXT("Backpack expands authority-native carried capacity"),
		Fixture.Session->SetPreparationEquipment(
			Fdemo_mapItemIds::BackpackSlot,
			Fixture.BackpackId).IsAccepted()
		&& Fixture.Session->SetPreparationMaterial(
			Fixture.ExtraMaterialIds[3], true).IsAccepted());
	TestTrue(TEXT("Capacity equipment cannot be removed while selected stacks depend on it"),
		Fixture.Session->SetPreparationEquipment(
			Fdemo_mapItemIds::BackpackSlot, FGuid()).Status
			== Edemo_mapProfilePreparationSelectionStatus::SelectionLimitExceeded);
	TestTrue(TEXT("Removing the overflow stack makes backpack clear legal"),
		Fixture.Session->SetPreparationMaterial(
			Fixture.ExtraMaterialIds[3], false).IsAccepted()
		&& Fixture.Session->SetPreparationEquipment(
			Fdemo_mapItemIds::BackpackSlot, FGuid()).IsAccepted());
	TestTrue(TEXT("Material cannot bind to Hotbar"),
		Fixture.Session->SetPreparationHotbarSlot(
			1, Fixture.DustId).Status
			== Edemo_mapProfilePreparationSelectionStatus::MaterialRejected);

	TestTrue(TEXT("Consumable binds before injected persistence failure"),
		Fixture.Session->SetPreparationHotbarSlot(
			1, Fixture.PillOneId).IsAccepted());
	FShanmenItemAuthoritySnapshot BeforeFailure;
	Fixture.Authority->TryCaptureSnapshot(BeforeFailure);
	const FShanmenItemReservationSnapshot* ReservationBefore =
		BeforeFailure.Reservations.FindByPredicate(
			[&Fixture](const FShanmenItemReservationSnapshot& Reservation)
			{
				return Reservation.ItemInstanceId == Fixture.PillOneId
					&& Reservation.ResourceKind
						== EShanmenItemResourceKind::Quantity
					&& Reservation.State
						== EShanmenItemReservationState::Reserved;
			});
	const FGuid ReservationIdBefore = ReservationBefore
		? ReservationBefore->ReservationId : FGuid();
	Fixture.Authority->SetInjectedFailureForAutomation(
		EShanmenItemStoreFailureStage::WriteTemp);
	const Fdemo_mapProfilePreparationSelectionResult Failed =
		Fixture.Session->SetPreparationHotbarSlot(2, Fixture.PillOneId);
	FShanmenItemAuthoritySnapshot AfterFailure;
	Fixture.Authority->TryCaptureSnapshot(AfterFailure);
	Fixture.Authority->SetInjectedFailureForAutomation(
		EShanmenItemStoreFailureStage::None);
	TestTrue(TEXT("Failed atomic metadata amendment rolls back before publishing"),
		!Failed.IsAccepted() && AfterFailure == BeforeFailure
		&& Fixture.Session->GetPreparationSnapshot()
			.HotbarBindings.SlotBindings[0] == Fixture.PillOneId
		&& !Fixture.Session->GetPreparationSnapshot()
			.HotbarBindings.SlotBindings[1].IsValid());
	const bool bRetryAccepted = Fixture.Session->SetPreparationHotbarSlot(
		2, Fixture.PillOneId).IsAccepted();
	FShanmenItemAuthoritySnapshot AfterRetry;
	Fixture.Authority->TryCaptureSnapshot(AfterRetry);
	const FShanmenItemReservationSnapshot* ReservationAfter =
		AfterRetry.Reservations.FindByPredicate(
			[&Fixture](const FShanmenItemReservationSnapshot& Reservation)
			{
				return Reservation.ItemInstanceId == Fixture.PillOneId
					&& Reservation.ResourceKind
						== EShanmenItemResourceKind::Quantity
					&& Reservation.State
						== EShanmenItemReservationState::Reserved;
			});
	TestTrue(TEXT("Autonomous retry atomically amends the same reservation once"),
		bRetryAccepted
		&& ReservationIdBefore.IsValid()
		&& ReservationAfter
		&& ReservationAfter->ReservationId == ReservationIdBefore
		&& AfterRetry.AuthorityRevision == BeforeFailure.AuthorityRevision + 1
		&& !Fixture.Session->GetPreparationSnapshot()
			.HotbarBindings.SlotBindings[0].IsValid()
		&& Fixture.Session->GetPreparationSnapshot()
			.HotbarBindings.SlotBindings[1] == Fixture.PillOneId);
	const Fdemo_mapProfilePreparationSelectionResult ClearAll =
		Fixture.Session->ClearPreparationSelection();
	const Fdemo_mapProfilePreparationSnapshot Cleared =
		Fixture.Session->GetPreparationSnapshot();
	TestTrue(TEXT("Clear All releases Quantity reserves before capacity equipment"),
		ClearAll.IsAccepted()
		&& Cleared.OrderedSelectedMaterialIds.IsEmpty()
		&& !Cleared.HotbarBindings.SlotBindings.ContainsByPredicate(
			[](const FGuid& Id) { return Id.IsValid(); })
		&& !Cleared.SelectedWeaponId.IsValid()
		&& !Cleared.SelectedBackpackId.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenPreparationAtomicCommitTest,
	"Shanmen.0_0_10.Items.PreparationAdapter.AtomicPreparedLoadout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenPreparationAtomicCommitTest::RunTest(const FString&)
{
	FPreparationAdapterFixture Fixture;
	if (!Fixture.StartAndCutover(*this, TEXT("AtomicPreparedLoadout")))
	{
		return false;
	}
	TArray<uint8> ProfileBefore;
	TestTrue(TEXT("Retired Profile bytes captured before P1.8 commit"),
		ReadBytes(Fixture.Storage.PrimaryPath(), ProfileBefore));
	TestTrue(TEXT("Migrated baseline becomes an explicit pending lock"),
		Fixture.Session->SetPreparationEquipment(
			Fdemo_mapItemIds::WeaponSlot,
			Fixture.TrainingBladeId).IsAccepted());
	TestTrue(TEXT("Two complete stacks and one Hotbar binding prepare"),
		Fixture.Session->SetPreparationMaterial(
			Fixture.DustId, true).IsAccepted()
		&& Fixture.Session->SetPreparationMaterial(
			Fixture.PillOneId, true).IsAccepted()
		&& Fixture.Session->SetPreparationHotbarSlot(
			3, Fixture.PillOneId).IsAccepted());

	FShanmenItemAuthoritySnapshot Before;
	FShanmenItemAuthorityDocument DocumentBefore;
	TestTrue(TEXT("Pre-commit authority and generation captured"),
		Fixture.Authority->TryCaptureSnapshot(Before)
		&& Fixture.Authority->TryGetDocument(DocumentBefore));
	Fixture.Authority->SetInjectedFailureForAutomation(
		EShanmenItemStoreFailureStage::WriteTemp);
	const Fdemo_mapShanmenPreparedLoadoutResult Failed =
		Fdemo_mapShanmenPreparationAdapter::CommitPreparedLoadout(
			*Fixture.Authority);
	Fixture.Authority->SetInjectedFailureForAutomation(
		EShanmenItemStoreFailureStage::None);
	FShanmenItemAuthoritySnapshot AfterFailure;
	FShanmenItemAuthorityDocument DocumentAfterFailure;
	TestTrue(TEXT("Failed durable batch publishes no partial item state"),
		!Failed.IsCommitted()
		&& Fixture.Authority->TryCaptureSnapshot(AfterFailure)
		&& AfterFailure == Before
		&& Fixture.Authority->TryGetDocument(DocumentAfterFailure)
		&& DocumentAfterFailure == DocumentBefore);

	const Fdemo_mapShanmenPreparedLoadoutResult Committed =
		Fdemo_mapShanmenPreparationAdapter::CommitPreparedLoadout(
			*Fixture.Authority);
	FShanmenItemAuthoritySnapshot After;
	FShanmenItemAuthorityDocument DocumentAfter;
	TestTrue(TEXT("Prepared loadout commits durably"),
		Committed.IsCommitted()
		&& Committed.Status
			== Edemo_mapShanmenPreparationAdapterStatus::Accepted
		&& Fixture.Authority->TryCaptureSnapshot(After)
		&& Fixture.Authority->TryGetDocument(DocumentAfter));
	TestTrue(TEXT("One batch revision and one document generation publish"),
		After.AuthorityRevision == Before.AuthorityRevision + 1
		&& DocumentAfter.SaveGeneration
			== DocumentBefore.SaveGeneration + 1);
	TestTrue(TEXT("Receipt freezes equipment, order, and Hotbar"),
		Committed.Receipt.WeaponItemInstanceId
			== Fixture.TrainingBladeId
		&& Committed.Receipt.OrderedRunInventoryItemInstanceIds
			== TArray<FGuid>({ Fixture.DustId, Fixture.PillOneId })
		&& Committed.Receipt.HotbarItemInstanceIds[2]
			== Fixture.PillOneId
		&& Committed.Receipt.OrderedLines.Num() == 3);
	const FShanmenItemInstance* Weapon = After.Items.FindByPredicate(
		[&Fixture](const FShanmenItemInstance& Item)
		{
			return Item.ItemInstanceId == Fixture.TrainingBladeId;
		});
	const FShanmenItemInstance* Dust = After.Items.FindByPredicate(
		[&Fixture](const FShanmenItemInstance& Item)
		{
			return Item.ItemInstanceId == Fixture.DustId;
		});
	const FShanmenItemInstance* Pill = After.Items.FindByPredicate(
		[&Fixture](const FShanmenItemInstance& Item)
		{
			return Item.ItemInstanceId == Fixture.PillOneId;
		});
	TestTrue(TEXT("Batch deploys equipment and consumes both full stacks"),
		Weapon && Weapon->State == EShanmenItemInstanceState::Deployed
		&& Dust && Dust->State == EShanmenItemInstanceState::Depleted
		&& Pill && Pill->State == EShanmenItemInstanceState::Depleted);
	TArray<uint8> ProfileAfter;
	TestTrue(TEXT("Atomic preparation never rewrites retired Profile items"),
		ReadBytes(Fixture.Storage.PrimaryPath(), ProfileAfter)
		&& ProfileAfter == ProfileBefore);

	if (!Fixture.RestartAndBind(*this)) return false;
	const Fdemo_mapShanmenPreparedLoadoutResult Replayed =
		Fdemo_mapShanmenPreparationAdapter::CommitPreparedLoadout(
			*Fixture.Authority);
	TestTrue(TEXT("Restart reconstructs the same committed loadout without another write"),
		Replayed.IsCommitted()
		&& Replayed.Status
			== Edemo_mapShanmenPreparationAdapterStatus::NoChange
		&& Replayed.Receipt.BatchRequestId
			== Committed.Receipt.BatchRequestId
		&& Replayed.Receipt.BatchReceiptId
			== Committed.Receipt.BatchReceiptId
		&& Replayed.Receipt.OrderedRunInventoryItemInstanceIds
			== Committed.Receipt.OrderedRunInventoryItemInstanceIds);
	FShanmenItemAuthorityDocument RestartedDocument;
	TestTrue(TEXT("Receipt reconstruction performs no extra persistence"),
		Fixture.Authority->TryGetDocument(RestartedDocument)
		&& RestartedDocument.SaveGeneration
			== DocumentAfter.SaveGeneration);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenPreparationPlainPurposeCompatibilityTest,
	"Shanmen.0_0_10.Items.PreparationAdapter.PlainPurposeReceiptCompatibility",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenPreparationPlainPurposeCompatibilityTest::RunTest(const FString&)
{
	FPreparationAdapterFixture Fixture;
	if (!Fixture.StartAndCutover(*this, TEXT("PlainPurposeCompatibility")))
	{
		return false;
	}

	FShanmenItemAuthoritySnapshot Before;
	if (!Fixture.Authority->TryCaptureSnapshot(Before))
	{
		AddError(TEXT("Could not capture authority before the compatibility reserve."));
		return false;
	}
	const FShanmenItemInstance* Dust = Before.Items.FindByPredicate(
		[&Fixture](const FShanmenItemInstance& Item)
		{
			return Item.ItemInstanceId == Fixture.DustId;
		});
	if (!Dust)
	{
		AddError(TEXT("Compatibility fixture is missing its Spirit Dust stack."));
		return false;
	}

	// P1.7/P1.8 persisted the logical purpose directly, before P1.9 began
	// appending an exact source-cell placement envelope for settlement.
	const FName PlainPurpose(
		TEXT("Shanmen.Preparation.RunInventory.r1.O00000000.H00"));
	FShanmenItemReserveRequest ReserveRequest;
	ReserveRequest.Context.RunId = Dust->RunId;
	ReserveRequest.Context.OwnerId = Dust->OwnerId;
	ReserveRequest.Context.RequestId = FGuid::NewGuid();
	ReserveRequest.Context.Content = Before.Content;
	ReserveRequest.ItemInstanceId = Dust->ItemInstanceId;
	ReserveRequest.ResourceKind = EShanmenItemResourceKind::Quantity;
	ReserveRequest.Amount = Dust->Quantity;
	ReserveRequest.ExpectedItemRevision = Dust->Revision;
	ReserveRequest.PurposeId = PlainPurpose;
	const FShanmenItemDurableCommandResult Reserved =
		Fixture.Authority->ReserveDurable(ReserveRequest);
	TestTrue(TEXT("Legacy plain-purpose Quantity intent remains durable"),
		Reserved.IsCommandSuccess());

	const Fdemo_mapShanmenPreparedLoadoutResult Committed =
		Fdemo_mapShanmenPreparationAdapter::CommitPreparedLoadout(
			*Fixture.Authority);
	const Fdemo_mapShanmenPreparedLoadoutLine* DustLine =
		Committed.Receipt.OrderedLines.FindByPredicate(
			[&Fixture](const Fdemo_mapShanmenPreparedLoadoutLine& Line)
			{
				return Line.ItemInstanceId == Fixture.DustId;
			});
	TestTrue(TEXT("Atomic commit accepts the pre-placement plain purpose"),
		Committed.IsCommitted()
		&& Committed.Receipt.OrderedRunInventoryItemInstanceIds
			== TArray<FGuid>({ Fixture.DustId })
		&& DustLine
		&& DustLine->PurposeId == PlainPurpose
		&& !DustLine->SourceContainerId.IsValid()
		&& DustLine->SourceSlotIndex == INDEX_NONE);

	if (!Fixture.RestartAndBind(*this))
	{
		return false;
	}
	const Fdemo_mapShanmenPreparedLoadoutResult Replayed =
		Fdemo_mapShanmenPreparationAdapter::CommitPreparedLoadout(
			*Fixture.Authority);
	const Fdemo_mapShanmenPreparedLoadoutLine* ReplayedDustLine =
		Replayed.Receipt.OrderedLines.FindByPredicate(
			[&Fixture](const Fdemo_mapShanmenPreparedLoadoutLine& Line)
			{
				return Line.ItemInstanceId == Fixture.DustId;
			});
	TestTrue(TEXT("Restart reconstructs the legacy receipt without another write"),
		Replayed.IsCommitted()
		&& Replayed.Status == Edemo_mapShanmenPreparationAdapterStatus::NoChange
		&& Replayed.Receipt.BatchRequestId == Committed.Receipt.BatchRequestId
		&& ReplayedDustLine
		&& ReplayedDustLine->PurposeId == PlainPurpose
		&& !ReplayedDustLine->SourceContainerId.IsValid()
		&& ReplayedDustLine->SourceSlotIndex == INDEX_NONE);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenPreparedRunLifecycleRestartTest,
	"Shanmen.0_0_10.Items.RunLifecycle.ClaimRestartFinalize",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenPreparedRunLifecycleRestartTest::RunTest(const FString&)
{
	FString DefinitionsError;
	TestTrue(TEXT("Runtime dependency manifest validates before lifecycle handoff"),
		Fdemo_mapItemDefinitions::Validate(&DefinitionsError));
	FPreparationAdapterFixture Fixture;
	if (!Fixture.StartAndCutover(*this, TEXT("RunLifecycle")))
	{
		return false;
	}
	TArray<uint8> ProfileBefore;
	TestTrue(TEXT("Retired Profile captured before P1.9 lifecycle"),
		ReadBytes(Fixture.Storage.PrimaryPath(), ProfileBefore));
	TestTrue(TEXT("Recoverable equipment and complete stacks prepare"),
		Fixture.Session->SetPreparationEquipment(
			Fdemo_mapItemIds::WeaponSlot,
			Fixture.TrainingBladeId).IsAccepted()
		&& Fixture.Session->SetPreparationMaterial(
			Fixture.DustId, true).IsAccepted()
		&& Fixture.Session->SetPreparationMaterial(
			Fixture.PillOneId, true).IsAccepted()
		&& Fixture.Session->SetPreparationHotbarSlot(
			4, Fixture.PillOneId).IsAccepted());

	Udemo_mapItemSubsystem* Runtime =
		Fixture.GameInstance->GetSubsystem<Udemo_mapItemSubsystem>();
	if (!Runtime)
	{
		AddError(TEXT("P1.9 fixture has no Runtime item subsystem."));
		return false;
	}
	Runtime->ResetForAutomation();
	FShanmenItemAuthorityDocument BeforeStartDocument;
	TestTrue(TEXT("Authority generation captured before atomic Run start"),
		Fixture.Authority->TryGetDocument(BeforeStartDocument));
	Runtime->SetPreparedRunFailureAfterMutationForAutomation(1);
	const Fdemo_mapShanmenRunStartResult FailedStart =
		Fdemo_mapShanmenRunLifecycleAdapter::StartPreparedRun(
			*Fixture.Authority, *Runtime);
	AddInfo(FString::Printf(
		TEXT("P1.12 first start status=%d diagnostic=%s start_status=%d start_error=%d runtime_status=%d"),
		static_cast<int32>(FailedStart.Status), *FailedStart.Diagnostic,
		static_cast<int32>(FailedStart.StartCommand.Status),
		static_cast<int32>(FailedStart.StartCommand.Receipt.Error),
		static_cast<int32>(FailedStart.RuntimeResult.Status)));
	FShanmenItemAuthoritySnapshot Claimed;
	FShanmenItemAuthorityDocument AfterStartDocument;
	const bool bCapturedClaimed =
		Fixture.Authority->TryCaptureSnapshot(Claimed)
		&& Fixture.Authority->TryGetDocument(AfterStartDocument);
	TestTrue(TEXT("Runtime failure rolls back transient items but retains one atomic durable start"),
		!FailedStart.IsStarted()
		&& FailedStart.Status
			== Edemo_mapShanmenRunLifecycleStatus::RuntimeMaterializationRejected
		&& FailedStart.ActiveRunId.IsValid()
		&& Runtime->GetRunState() == Edemo_mapRunState::Inactive
		&& Runtime->GetAuthority().GetInstanceSnapshot().IsEmpty()
		&& bCapturedClaimed
		&& AfterStartDocument.SaveGeneration
			== BeforeStartDocument.SaveGeneration + 1);
	int32 AtomicStartCount = 0;
	int32 IntermediateBatchOrClaimCount = 0;
	for (const FShanmenItemProcessedRequestSnapshot& Processed :
		Claimed.ProcessedRequests)
	{
		AtomicStartCount += Processed.Receipt.IsSuccess()
			&& Processed.Receipt.Operation
				== EShanmenItemTransactionOperation::StartPreparedRun ? 1 : 0;
		IntermediateBatchOrClaimCount += Processed.Receipt.IsSuccess()
			&& (Processed.Receipt.Operation
					== EShanmenItemTransactionOperation::CommitBatch
				|| Processed.Receipt.Operation
					== EShanmenItemTransactionOperation::ClaimPreparedRun) ? 1 : 0;
	}
	TestTrue(TEXT("Exactly one start marker survives with no intermediate batch or claim"),
		AtomicStartCount == 1 && IntermediateBatchOrClaimCount == 0);

	const FGuid ActiveRunId = FailedStart.ActiveRunId;
	if (!Fixture.RestartAndBind(*this))
	{
		return false;
	}
	Runtime = Fixture.GameInstance->GetSubsystem<Udemo_mapItemSubsystem>();
	const Fdemo_mapShanmenRunStartResult Resumed = Runtime
		? Fdemo_mapShanmenRunLifecycleAdapter::StartPreparedRun(
			*Fixture.Authority, *Runtime)
		: Fdemo_mapShanmenRunStartResult();
	TestTrue(TEXT("Restart replays claim and materializes the same ActiveRunId"),
		Runtime && Resumed.IsStarted()
		&& Resumed.Status == Edemo_mapShanmenRunLifecycleStatus::Resumed
		&& Resumed.ActiveRunId == ActiveRunId
		&& Runtime->GetRunState() == Edemo_mapRunState::Active
		&& Runtime->GetActiveRunId() == ActiveRunId
		&& Runtime->GetDeployedItemIds().Num() == 3);
	TArray<FGuid> AcquiredLootIds;
	const Fdemo_mapItemOperationResult AcquiredLoot = Runtime
		? Runtime->AddDefinition(
			Fdemo_mapItemIds::TrainingBlade, 1, &AcquiredLootIds)
		: Fdemo_mapItemOperationResult::Failure(
			Edemo_mapItemResultCode::RunNotActive,
			TEXT("Runtime fixture is absent."));
	TestTrue(TEXT("Active Runtime creates one equipment loot identity for metadata import"),
		AcquiredLoot.bSuccess && AcquiredLootIds.Num() == 1
		&& Runtime->IsItemAtRiskInActiveRun(AcquiredLootIds[0]));

	Fdemo_mapSettlementSummary Summary;
	const Fdemo_mapItemOperationResult RuntimeSettlement =
		Runtime->RequestSettlement(
			Edemo_mapRunEndReason::Extraction, Summary);
	TestTrue(TEXT("Runtime emits originals plus the acquired identity"),
		RuntimeSettlement.bSuccess && Summary.bValid
		&& Summary.RuntimeSnapshot.bValid
		&& Summary.RunId == ActiveRunId
		&& Summary.RuntimeSnapshot.OrderedSecuredItems.Num() == 4);
	Fdemo_mapRuntimeSettlementItem* PartiallyConsumedPill =
		Summary.RuntimeSnapshot.OrderedSecuredItems.FindByPredicate(
			[&Fixture](const Fdemo_mapRuntimeSettlementItem& Item)
			{
				return Item.ItemInstanceId == Fixture.PillOneId;
			});
	TestTrue(TEXT("Fixture models one consumed pill in the immutable Runtime handoff"),
		PartiallyConsumedPill != nullptr);
	if (PartiallyConsumedPill)
	{
		PartiallyConsumedPill->StackCount = 2;
	}
	Fdemo_mapSettlementSummary MetadataSummary = Summary;
	Fdemo_mapRuntimeSettlementItem* MetadataLoot =
		AcquiredLootIds.Num() == 1
		? MetadataSummary.RuntimeSnapshot.OrderedSecuredItems.FindByPredicate(
			[&AcquiredLootIds](const Fdemo_mapRuntimeSettlementItem& Item)
			{
				return Item.ItemInstanceId == AcquiredLootIds[0];
			}) : nullptr;
	if (MetadataLoot)
	{
		MetadataLoot->RewardEventKind = Edemo_mapRewardEventKind::Jackpot;
		MetadataLoot->RewardEventId = FGuid(0x51100001, 0, 0, 1);
		MetadataLoot->RewardValueMultiplierBps =
			Fdemo_mapRewardEventRules::JackpotMultiplierBps;
		MetadataLoot->RewardSourceRoleId = TEXT("Test.MetadataSource");
		MetadataLoot->RareRewardEventId = FGuid(0x51100002, 0, 0, 1);
		MetadataLoot->RareRewardPolicyId = TEXT("Reward.Rare.TestPolicy");
		MetadataLoot->RareRewardTierId = TEXT("Reward.Rare.Tier3");
		MetadataLoot->RareRewardBonusValue = 777;
		MetadataLoot->AffixSet.AffixSetEventId =
			FGuid(0x51100003, 0, 0, 1);
		MetadataLoot->AffixSet.AffixPolicyId =
			Fdemo_mapRewardAffixPolicyRegistry::DefaultPolicyId;
		MetadataLoot->AffixSet.Acquisition =
			Edemo_mapRewardAffixAcquisition::Natural;
		Fdemo_mapResolvedRewardAffix& Affix =
			MetadataLoot->AffixSet.Affixes.AddDefaulted_GetRef();
		Affix.AffixId = TEXT("Reward.Affix.Weapon.Power.T2");
		Affix.Tier = Edemo_mapRewardAffixTier::Tier2;
		Affix.ResolvedMagnitudeScaled = 5;
		Affix.ResolvedValue = 120;
	}
	FShanmenItemRewardMetadata ExpectedMetadata;
	FString MetadataDiagnostic;
	TestTrue(TEXT("Product reward metadata maps to one canonical authority value"),
		MetadataLoot
		&& Fdemo_mapShanmenItemMetadataAdapter::FromRuntimeItem(
			*MetadataLoot, ExpectedMetadata, MetadataDiagnostic)
		&& ExpectedMetadata.Affixes.Num() == 1);

	FShanmenItemAuthoritySnapshot BeforeFinalizeFailure;
	Fixture.Authority->TryCaptureSnapshot(BeforeFinalizeFailure);
	Fixture.Authority->SetInjectedFailureForAutomation(
		EShanmenItemStoreFailureStage::WriteTemp);
	const Fdemo_mapShanmenRunFinalizeResult FailedFinalize =
		Fdemo_mapShanmenRunLifecycleAdapter::FinalizeSettlement(
			*Fixture.Authority, MetadataSummary);
	Fixture.Authority->SetInjectedFailureForAutomation(
		EShanmenItemStoreFailureStage::None);
	FShanmenItemAuthoritySnapshot AfterFinalizeFailure;
	Fixture.Authority->TryCaptureSnapshot(AfterFinalizeFailure);
	TestTrue(TEXT("Failed terminal save restores exact active authority"),
		!FailedFinalize.IsFinalized()
		&& AfterFinalizeFailure == BeforeFinalizeFailure);

	const Fdemo_mapShanmenRunFinalizeResult Finalized =
		Fdemo_mapShanmenRunLifecycleAdapter::FinalizeSettlement(
			*Fixture.Authority, MetadataSummary);
	FShanmenItemAuthoritySnapshot Terminal;
	TestTrue(TEXT("Autonomous retry atomically finalizes extraction"),
		Finalized.IsFinalized()
		&& Finalized.Status
			== Edemo_mapShanmenRunLifecycleStatus::Finalized
		&& Fixture.Authority->TryCaptureSnapshot(Terminal));
	const FShanmenItemInstance* Weapon = Terminal.Items.FindByPredicate(
		[&Fixture](const FShanmenItemInstance& Item)
		{
			return Item.ItemInstanceId == Fixture.TrainingBladeId;
		});
	const FShanmenItemInstance* Dust = Terminal.Items.FindByPredicate(
		[&Fixture](const FShanmenItemInstance& Item)
		{
			return Item.ItemInstanceId == Fixture.DustId;
		});
	const FShanmenItemInstance* Pill = Terminal.Items.FindByPredicate(
		[&Fixture](const FShanmenItemInstance& Item)
		{
			return Item.ItemInstanceId == Fixture.PillOneId;
		});
	const FShanmenItemInstance* ImportedLoot =
		AcquiredLootIds.Num() == 1
		? Terminal.Items.FindByPredicate(
			[&AcquiredLootIds](const FShanmenItemInstance& Item)
			{
				return Item.ItemInstanceId == AcquiredLootIds[0];
			}) : nullptr;
	const FShanmenItemContainer* Warehouse =
		Terminal.Containers.FindByPredicate(
			[](const FShanmenItemContainer& Container)
			{
				return Container.ContainerType == FName(TEXT("Warehouse"));
			});
	int32 ReleasedLines = 0;
	int32 FinalizeCount = 0;
	for (const FShanmenItemReservationSnapshot& Reservation :
		Terminal.Reservations)
	{
		ReleasedLines += Reservation.State
			== EShanmenItemReservationState::Released ? 1 : 0;
	}
	for (const FShanmenItemProcessedRequestSnapshot& Processed :
		Terminal.ProcessedRequests)
	{
		FinalizeCount += Processed.Receipt.IsSuccess()
			&& Processed.Receipt.Operation
				== EShanmenItemTransactionOperation::FinalizePreparedRun ? 1 : 0;
	}
	TestTrue(TEXT("Terminal candidate restores exact source cells and releases every line"),
		Weapon && Weapon->State == EShanmenItemInstanceState::Stored
		&& !Weapon->DeploymentReservationId.IsValid()
		&& Dust && Dust->State == EShanmenItemInstanceState::Stored
		&& Dust->Quantity == 3 && Dust->ParentContainerId.IsValid()
		&& Pill && Pill->State == EShanmenItemInstanceState::Stored
		&& Pill->Quantity == 2 && Pill->ParentContainerId.IsValid()
		&& ImportedLoot
		&& ImportedLoot->DefinitionId == Fdemo_mapItemIds::TrainingBlade
		&& ImportedLoot->Quantity == 1
		&& ImportedLoot->RewardMetadata == ExpectedMetadata
		&& ImportedLoot->RewardMetadata.RewardEventKind
			== EShanmenItemRewardEventKind::Jackpot
		&& ImportedLoot->RewardMetadata.RareRewardBonusValue == 777
		&& ImportedLoot->RewardMetadata.Affixes.Num() == 1
		&& ImportedLoot->RewardMetadata.Affixes[0].AffixId
			== FName(TEXT("Reward.Affix.Weapon.Power.T2"))
		&& ImportedLoot->State == EShanmenItemInstanceState::Stored
		&& Warehouse
		&& ImportedLoot->ParentContainerId == Warehouse->ContainerId
		&& Warehouse->Slots.IsValidIndex(ImportedLoot->SlotIndex)
		&& Warehouse->Slots[ImportedLoot->SlotIndex]
			== ImportedLoot->ItemInstanceId
		&& ReleasedLines == 3 && FinalizeCount == 1);
	const Fdemo_mapShanmenRunFinalizeResult ReplayFinalize =
		Fdemo_mapShanmenRunLifecycleAdapter::FinalizeSettlement(
			*Fixture.Authority, MetadataSummary);
	TestTrue(TEXT("Repeated settlement observes terminal marker without another write"),
		ReplayFinalize.IsFinalized()
		&& ReplayFinalize.Status
			== Edemo_mapShanmenRunLifecycleStatus::NoChange);

	TArray<uint8> ProfileAfter;
	TestTrue(TEXT("P1.9 lifecycle never rewrites retired Profile bytes"),
		ReadBytes(Fixture.Storage.PrimaryPath(), ProfileAfter)
		&& ProfileAfter == ProfileBefore);
	if (!Fixture.RestartAndBind(*this))
	{
		return false;
	}
	FShanmenItemAuthoritySnapshot Restarted;
	TestTrue(TEXT("Restart keeps one terminal marker and restored originals"),
		Fixture.Authority->TryCaptureSnapshot(Restarted)
		&& Restarted == Terminal);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenPreparedRunItemUseTest,
	"Shanmen.0_0_10.Items.RunLifecycle.DurableHotbarUse",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenPreparedRunItemUseTest::RunTest(const FString&)
{
	FPreparationAdapterFixture Fixture;
	if (!Fixture.StartAndCutover(*this, TEXT("DurableHotbarUse")))
	{
		return false;
	}
	TestTrue(TEXT("Pill stack and slot are selected through the authority adapter"),
		Fixture.Session->SetPreparationMaterial(
			Fixture.PillOneId, true).IsAccepted()
		&& Fixture.Session->SetPreparationHotbarSlot(
			4, Fixture.PillOneId).IsAccepted());
	Udemo_mapItemSubsystem* Runtime =
		Fixture.GameInstance->GetSubsystem<Udemo_mapItemSubsystem>();
	Udemo_mapAttributeComponent* Attributes =
		NewObject<Udemo_mapAttributeComponent>(GetTransientPackage());
	Udemo_mapPlayerHealthComponent* Health =
		NewObject<Udemo_mapPlayerHealthComponent>(GetTransientPackage());
	if (!Runtime || !Attributes || !Health
		|| !Health->BindAttributeComponent(Attributes, true))
	{
		AddError(TEXT("Durable hotbar fixture could not bind Runtime health."));
		return false;
	}
	Runtime->BindAttributeComponent(Attributes);
	Runtime->BindHealthComponent(Health);
	const Fdemo_mapShanmenRunStartResult Started =
		Fdemo_mapShanmenRunLifecycleAdapter::StartPreparedRun(
			*Fixture.Authority, *Runtime);
	const Fdemo_mapItemInstance* InitialPill =
		Runtime->GetAuthority().FindInstance(Fixture.PillOneId);
	TestTrue(TEXT("Prepared pill materializes with its complete stack and binding"),
		Started.IsStarted() && InitialPill && InitialPill->Quantity == 3
		&& Runtime->GetHotbarBindingSnapshot().SlotBindings[3]
			== Fixture.PillOneId);
	Health->SetCurrentHealthForAutomation(1);

	const Fdemo_mapShanmenRunItemUseResult FailedProjection =
		Fdemo_mapShanmenRunLifecycleAdapter::UsePreparedRunHotbarSlot(
			*Fixture.Authority, *Runtime, 4, true,
			Edemo_mapItemUseFailurePoint::AfterItemMutation);
	const Fdemo_mapItemInstance* AfterFailure =
		Runtime->GetAuthority().FindInstance(Fixture.PillOneId);
	AddInfo(FString::Printf(
		TEXT("P5.0 injected use status=%d preview=%d authority=%d error=%d runtime=%d health=%d/%d stack=%d diagnostic=%s"),
		static_cast<int32>(FailedProjection.Status),
		static_cast<int32>(FailedProjection.Preview.Status),
		static_cast<int32>(FailedProjection.AuthorityCommand.Status),
		static_cast<int32>(FailedProjection.AuthorityCommand.Receipt.Error),
		static_cast<int32>(FailedProjection.RuntimeResult.Status),
		Health->GetCurrentHealth(), Health->GetMaxHealth(),
		AfterFailure ? AfterFailure->Quantity : INDEX_NONE,
		*FailedProjection.Diagnostic));
	TestTrue(TEXT("Runtime failure retains durable receipt but rolls back local effect"),
		FailedProjection.Status
			== Edemo_mapShanmenRunItemUseStatus::RuntimeCommitRejected
		&& FailedProjection.AuthorityCommand.IsCommandSuccess()
		&& AfterFailure && AfterFailure->Quantity == 3
		&& Health->GetCurrentHealth() == 1);

	const Fdemo_mapShanmenRunItemUseResult Retried =
		Fdemo_mapShanmenRunLifecycleAdapter::UsePreparedRunHotbarSlot(
			*Fixture.Authority, *Runtime, 4, true);
	const Fdemo_mapItemInstance* AfterRetry =
		Runtime->GetAuthority().FindInstance(Fixture.PillOneId);
	AddInfo(FString::Printf(
		TEXT("P5.0 retry status=%d preview=%d authority=%d error=%d runtime=%d health=%d/%d stack=%d diagnostic=%s"),
		static_cast<int32>(Retried.Status),
		static_cast<int32>(Retried.Preview.Status),
		static_cast<int32>(Retried.AuthorityCommand.Status),
		static_cast<int32>(Retried.AuthorityCommand.Receipt.Error),
		static_cast<int32>(Retried.RuntimeResult.Status),
		Health->GetCurrentHealth(), Health->GetMaxHealth(),
		AfterRetry ? AfterRetry->Quantity : INDEX_NONE,
		*Retried.Diagnostic));
	TestTrue(TEXT("Exact retry replays authority receipt and commits Runtime once"),
		Retried.IsSuccess()
		&& Retried.AuthorityCommand.Status
			== EShanmenItemDurableCommandStatus::Replayed
		&& AfterRetry && AfterRetry->Quantity == 2
		&& Health->GetCurrentHealth() == 2);
	FShanmenItemAuthoritySnapshot UsedSnapshot;
	int32 DurableUseCount = 0;
	if (Fixture.Authority->TryCaptureSnapshot(UsedSnapshot))
	{
		for (const FShanmenItemProcessedRequestSnapshot& Processed :
			UsedSnapshot.ProcessedRequests)
		{
			DurableUseCount += Processed.Receipt.IsSuccess()
				&& Processed.Receipt.Operation
					== EShanmenItemTransactionOperation::ConsumePreparedRunItem
				? 1 : 0;
		}
	}
	TestEqual(TEXT("Only one durable consume exists after retry"),
		DurableUseCount, 1);

	if (!Fixture.RestartAndBind(*this))
	{
		return false;
	}
	Runtime = Fixture.GameInstance->GetSubsystem<Udemo_mapItemSubsystem>();
	const Fdemo_mapShanmenRunStartResult Resumed = Runtime
		? Fdemo_mapShanmenRunLifecycleAdapter::StartPreparedRun(
			*Fixture.Authority, *Runtime)
		: Fdemo_mapShanmenRunStartResult();
	const Fdemo_mapItemInstance* RecoveredPill = Runtime
		? Runtime->GetAuthority().FindInstance(Fixture.PillOneId) : nullptr;
	TestTrue(TEXT("Restart materialization subtracts durable consumption"),
		Resumed.IsStarted()
		&& Resumed.Status == Edemo_mapShanmenRunLifecycleStatus::Resumed
		&& RecoveredPill && RecoveredPill->Quantity == 2
		&& Runtime->GetHotbarBindingSnapshot().SlotBindings[3]
			== Fixture.PillOneId);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenPreparedRunQuantityIntentRestartProjectionTest,
	"Shanmen.0_0_10.Items.RunLifecycle.QuantityIntentRestartProjection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenPreparedRunQuantityIntentRestartProjectionTest::RunTest(
	const FString&)
{
	FPreparationAdapterFixture Fixture;
	if (!Fixture.StartAndCutover(
		*this, TEXT("QuantityIntentRestartProjection")))
	{
		return false;
	}
	TestTrue(TEXT("Pill stack is selected for the restart projection fixture"),
		Fixture.Session->SetPreparationMaterial(
			Fixture.PillOneId, true).IsAccepted());
	Udemo_mapItemSubsystem* Runtime =
		Fixture.GameInstance->GetSubsystem<Udemo_mapItemSubsystem>();
	const Fdemo_mapShanmenRunStartResult Started = Runtime
		? Fdemo_mapShanmenRunLifecycleAdapter::StartPreparedRun(
			*Fixture.Authority, *Runtime)
		: Fdemo_mapShanmenRunStartResult();
	const Fdemo_mapItemInstance* InitialPill = Runtime
		? Runtime->GetAuthority().FindInstance(Fixture.PillOneId) : nullptr;
	FShanmenItemAuthoritySnapshot BeforePrepare;
	if (!Started.IsStarted() || !Started.RunCorrelation.IsValid()
		|| !InitialPill || InitialPill->Quantity != 3
		|| !Fixture.Authority->TryCaptureSnapshot(BeforePrepare))
	{
		AddError(TEXT("Could not establish the active-Run Quantity fixture."));
		return false;
	}

	const FGuid IntentId(0xD3710201, 0, 0, 1);
	FShanmenItemRunQuantityIntentRequest PrepareRequest;
	PrepareRequest.Context.RunId = Started.RunCorrelation.ScopeId;
	PrepareRequest.Context.OwnerId = Started.RunCorrelation.OwnerId;
	PrepareRequest.Context.RequestId = FGuid(0xD3710202, 0, 0, 1);
	PrepareRequest.Context.Content = BeforePrepare.Content;
	PrepareRequest.ActiveRunId = Started.ActiveRunId;
	PrepareRequest.IntentId = IntentId;
	PrepareRequest.ItemInstanceId = Fixture.PillOneId;
	PrepareRequest.Amount = 1;
	PrepareRequest.ExpectedQuantityBefore = 3;
	PrepareRequest.PurposeId =
		TEXT("Test.RunLifecycle.QuantityIntentRestart.r1");
	const FShanmenItemDurableCommandResult Prepared =
		Fixture.Authority->PreparePreparedRunQuantityIntentDurable(
			PrepareRequest);

	FShanmenItemRunQuantityIntentFinalizeRequest FinalizeRequest;
	FinalizeRequest.Context.RunId = Started.RunCorrelation.ScopeId;
	FinalizeRequest.Context.OwnerId = Started.RunCorrelation.OwnerId;
	FinalizeRequest.Context.RequestId = FGuid(0xD3710203, 0, 0, 1);
	FinalizeRequest.Context.Content = BeforePrepare.Content;
	FinalizeRequest.ActiveRunId = Started.ActiveRunId;
	FinalizeRequest.PrepareRequestId = PrepareRequest.Context.RequestId;
	FinalizeRequest.IntentId = IntentId;
	FinalizeRequest.ItemInstanceId = Fixture.PillOneId;
	FinalizeRequest.bCommit = true;
	const FShanmenItemDurableCommandResult Finalized =
		Prepared.IsCommandSuccess()
			? Fixture.Authority->FinalizePreparedRunQuantityIntentDurable(
				FinalizeRequest)
			: FShanmenItemDurableCommandResult();
	TestTrue(TEXT("Committed Quantity intent records one terminal decrement"),
		Prepared.IsCommandSuccess()
		&& Finalized.IsCommandSuccess()
		&& Finalized.Receipt.Operation
			== EShanmenItemTransactionOperation::FinalizePreparedRunQuantityIntent
		&& Finalized.Receipt.Phase
			== EShanmenItemTransactionPhase::Committed
		&& Finalized.Receipt.ResourceBefore == 3
		&& Finalized.Receipt.ResourceAfter == 2);

	if (!Fixture.RestartAndBind(*this))
	{
		return false;
	}
	Runtime = Fixture.GameInstance->GetSubsystem<Udemo_mapItemSubsystem>();
	const Fdemo_mapShanmenRunStartResult Resumed = Runtime
		? Fdemo_mapShanmenRunLifecycleAdapter::StartPreparedRun(
			*Fixture.Authority, *Runtime)
		: Fdemo_mapShanmenRunStartResult();
	const Fdemo_mapItemInstance* RecoveredPill = Runtime
		? Runtime->GetAuthority().FindInstance(Fixture.PillOneId) : nullptr;
	TestTrue(TEXT("Restart projection preserves committed launch consumption"),
		Resumed.IsStarted()
		&& Resumed.Status == Edemo_mapShanmenRunLifecycleStatus::Resumed
		&& RecoveredPill && RecoveredPill->Quantity == 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenPreparedRunInventoryItemUseTest,
	"Shanmen.0_0_10.Items.RunLifecycle.DurableInventoryUse",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenPreparedRunInventoryItemUseTest::RunTest(const FString&)
{
	FPreparationAdapterFixture Fixture;
	if (!Fixture.StartAndCutover(*this, TEXT("DurableInventoryUse")))
	{
		return false;
	}
	TestTrue(TEXT("Pill stack is prepared without a hotbar binding"),
		Fixture.Session->SetPreparationMaterial(
			Fixture.PillOneId, true).IsAccepted());
	Udemo_mapItemSubsystem* Runtime =
		Fixture.GameInstance->GetSubsystem<Udemo_mapItemSubsystem>();
	Udemo_mapAttributeComponent* Attributes =
		NewObject<Udemo_mapAttributeComponent>(GetTransientPackage());
	Udemo_mapPlayerHealthComponent* Health =
		NewObject<Udemo_mapPlayerHealthComponent>(GetTransientPackage());
	if (!Runtime || !Attributes || !Health
		|| !Health->BindAttributeComponent(Attributes, true))
	{
		AddError(TEXT("Durable inventory fixture could not bind Runtime health."));
		return false;
	}
	Runtime->BindAttributeComponent(Attributes);
	Runtime->BindHealthComponent(Health);
	const Fdemo_mapShanmenRunStartResult Started =
		Fdemo_mapShanmenRunLifecycleAdapter::StartPreparedRun(
			*Fixture.Authority, *Runtime);
	const Fdemo_mapItemInstance* InitialPill =
		Runtime->GetAuthority().FindInstance(Fixture.PillOneId);
	const TArray<FGuid> HotbarBefore =
		Runtime->GetHotbarBindingSnapshot().SlotBindings;
	TestTrue(TEXT("Prepared unbound pill materializes with its complete stack"),
		Started.IsStarted() && InitialPill && InitialPill->Quantity == 3
		&& !HotbarBefore.Contains(Fixture.PillOneId));
	Health->SetCurrentHealthForAutomation(1);

	const Fdemo_mapShanmenRunItemUseResult FailedProjection =
		Fdemo_mapShanmenRunLifecycleAdapter::UsePreparedRunInventoryItem(
			*Fixture.Authority, *Runtime, Fixture.PillOneId, true,
			Edemo_mapItemUseFailurePoint::AfterItemMutation);
	const Fdemo_mapItemInstance* AfterFailure =
		Runtime->GetAuthority().FindInstance(Fixture.PillOneId);
	TestTrue(TEXT("Failed inventory projection retains durable authority only"),
		FailedProjection.Status
			== Edemo_mapShanmenRunItemUseStatus::RuntimeCommitRejected
		&& FailedProjection.AuthorityCommand.Status
			== EShanmenItemDurableCommandStatus::Persisted
		&& AfterFailure && AfterFailure->Quantity == 3
		&& Health->GetCurrentHealth() == 1
		&& Runtime->GetHotbarBindingSnapshot().SlotBindings == HotbarBefore);

	const Fdemo_mapShanmenRunItemUseResult Retried =
		Fdemo_mapShanmenRunLifecycleAdapter::UsePreparedRunInventoryItem(
			*Fixture.Authority, *Runtime, Fixture.PillOneId, true);
	const Fdemo_mapItemInstance* AfterRetry =
		Runtime->GetAuthority().FindInstance(Fixture.PillOneId);
	TestTrue(TEXT("Exact inventory retry replays receipt and commits Runtime once"),
		Retried.IsSuccess()
		&& Retried.AuthorityCommand.Status
			== EShanmenItemDurableCommandStatus::Replayed
		&& AfterRetry && AfterRetry->Quantity == 2
		&& Health->GetCurrentHealth() == 2);
	TestTrue(TEXT("Direct inventory retry never synthesizes a hotbar binding"),
		Runtime->GetHotbarBindingSnapshot().SlotBindings == HotbarBefore
		&& !Retried.RuntimeResult.bBindingCleared);

	FShanmenItemAuthoritySnapshot UsedSnapshot;
	int32 DurableUseCount = 0;
	if (Fixture.Authority->TryCaptureSnapshot(UsedSnapshot))
	{
		for (const FShanmenItemProcessedRequestSnapshot& Processed :
			UsedSnapshot.ProcessedRequests)
		{
			DurableUseCount += Processed.Receipt.IsSuccess()
				&& Processed.Receipt.Operation
					== EShanmenItemTransactionOperation::ConsumePreparedRunItem
				? 1 : 0;
		}
	}
	TestEqual(TEXT("Inventory use emits exactly one durable consume receipt"),
		DurableUseCount, 1);

	FString WidgetSource;
	const bool bReadWidget = FFileHelper::LoadFileToString(
		WidgetSource,
		*FPaths::Combine(
			FPaths::ProjectDir(),
			TEXT("Source/demo_map/demo_mapInventoryWidget.cpp")));
	TestTrue(TEXT("Inventory widget source is readable"), bReadWidget);
	TestTrue(TEXT("Inventory widget routes use through the product manager"),
		WidgetSource.Contains(
			TEXT("Manager->RequestUseInventoryItem(")));
	TestFalse(TEXT("Inventory widget has no Runtime-only item-use bypass"),
		WidgetSource.Contains(
			TEXT("GetItemSubsystem()->UseInventoryItem(")));

	if (!Fixture.RestartAndBind(*this))
	{
		return false;
	}
	Runtime = Fixture.GameInstance->GetSubsystem<Udemo_mapItemSubsystem>();
	const Fdemo_mapShanmenRunStartResult Resumed = Runtime
		? Fdemo_mapShanmenRunLifecycleAdapter::StartPreparedRun(
			*Fixture.Authority, *Runtime)
		: Fdemo_mapShanmenRunStartResult();
	const Fdemo_mapItemInstance* RecoveredPill = Runtime
		? Runtime->GetAuthority().FindInstance(Fixture.PillOneId) : nullptr;
	TestTrue(TEXT("Restart reconstructs decremented quantity without a binding"),
		Resumed.IsStarted()
		&& Resumed.Status == Edemo_mapShanmenRunLifecycleStatus::Resumed
		&& RecoveredPill && RecoveredPill->Quantity == 2
		&& !Runtime->GetHotbarBindingSnapshot().SlotBindings.Contains(
			Fixture.PillOneId));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenPreparedRunDestructiveTerminalTest,
	"Shanmen.0_0_10.Items.RunLifecycle.DeathAndAbandon",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenPreparedRunDestructiveTerminalTest::RunTest(const FString&)
{
	const TArray<Edemo_mapRunEndReason> Reasons =
	{
		Edemo_mapRunEndReason::Death,
		Edemo_mapRunEndReason::Abandon
	};
	for (const Edemo_mapRunEndReason Reason : Reasons)
	{
		FPreparationAdapterFixture Fixture;
		const TCHAR* Label = Reason == Edemo_mapRunEndReason::Death
			? TEXT("RunLifecycleDeath") : TEXT("RunLifecycleAbandon");
		if (!Fixture.StartAndCutover(*this, Label))
		{
			return false;
		}
		TArray<uint8> ProfileBefore;
		TestTrue(TEXT("Retired Profile is captured before destructive terminal"),
			ReadBytes(Fixture.Storage.PrimaryPath(), ProfileBefore));
		TestTrue(TEXT("One equipment identity prepares for destructive terminal"),
			Fixture.Session->SetPreparationEquipment(
				Fdemo_mapItemIds::WeaponSlot,
				Fixture.TrainingBladeId).IsAccepted());
		Udemo_mapItemSubsystem* Runtime =
			Fixture.GameInstance->GetSubsystem<Udemo_mapItemSubsystem>();
		if (!Runtime)
		{
			AddError(TEXT("Destructive terminal fixture has no Runtime authority."));
			return false;
		}
		Runtime->ResetForAutomation();
		const Fdemo_mapShanmenRunStartResult Started =
			Fdemo_mapShanmenRunLifecycleAdapter::StartPreparedRun(
				*Fixture.Authority, *Runtime);
		Fdemo_mapSettlementSummary Summary;
		const Fdemo_mapItemOperationResult Settled = Started.IsStarted()
			? Runtime->RequestSettlement(Reason, Summary)
			: Fdemo_mapItemOperationResult::Failure(
				Edemo_mapItemResultCode::RunNotActive,
				TEXT("Prepared Runtime did not start."));
		TestTrue(TEXT("Destructive Runtime snapshot secures no identity"),
			Started.IsStarted() && Settled.bSuccess
			&& Summary.RuntimeSnapshot.bValid
			&& Summary.RuntimeSnapshot.OrderedSecuredItems.IsEmpty());
		const Fdemo_mapShanmenRunFinalizeResult Finalized =
			Fdemo_mapShanmenRunLifecycleAdapter::FinalizeSettlement(
				*Fixture.Authority, Summary);
		FShanmenItemAuthoritySnapshot Terminal;
		const FShanmenItemInstance* LostWeapon =
			Fixture.Authority->TryCaptureSnapshot(Terminal)
			? Terminal.Items.FindByPredicate(
				[&Fixture](const FShanmenItemInstance& Item)
				{
					return Item.ItemInstanceId == Fixture.TrainingBladeId;
				}) : nullptr;
		const FName ExpectedPurpose = Reason == Edemo_mapRunEndReason::Death
			? FShanmenItemRunLifecyclePurpose::Death()
			: FShanmenItemRunLifecyclePurpose::Abandon();
		TestTrue(TEXT("Destructive terminal publishes one reason-specific tombstone"),
			Finalized.Status == Edemo_mapShanmenRunLifecycleStatus::Finalized
			&& Finalized.FinalizeCommand.Receipt.PurposeId == ExpectedPurpose
			&& LostWeapon
			&& LostWeapon->State == EShanmenItemInstanceState::Destroyed
			&& !LostWeapon->ParentContainerId.IsValid()
			&& !LostWeapon->DeploymentReservationId.IsValid());
		const Fdemo_mapShanmenRunFinalizeResult Replay =
			Fdemo_mapShanmenRunLifecycleAdapter::FinalizeSettlement(
				*Fixture.Authority, Summary);
		TestTrue(TEXT("Destructive terminal replay performs no second mutation"),
			Replay.Status == Edemo_mapShanmenRunLifecycleStatus::NoChange);
		TArray<uint8> ProfileAfter;
		TestTrue(TEXT("Destructive terminal does not rewrite retired Profile"),
			ReadBytes(Fixture.Storage.PrimaryPath(), ProfileAfter)
			&& ProfileAfter == ProfileBefore);
		if (!Fixture.RestartAndBind(*this))
		{
			return false;
		}
		FShanmenItemAuthoritySnapshot Restarted;
		TestTrue(TEXT("Destructive tombstone survives durable restart"),
			Fixture.Authority->TryCaptureSnapshot(Restarted)
			&& Restarted == Terminal);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenCanonicalControlledWeaponActiveRunRouteTest,
	"Shanmen.0_0_10.Product.ControlledWeaponActiveRunRoute.CanonicalCutoverStart",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenCanonicalControlledWeaponActiveRunRouteTest::RunTest(
	const FString&)
{
	FPreparationAdapterFixture Fixture;
	if (!Fixture.StartAndCutover(
			*this, TEXT("CanonicalControlledWeaponActiveRun")))
	{
		return false;
	}
	const Fdemo_mapProfilePreparationSelectionResult Selected =
		Fixture.Session->SetPreparationEquipment(
			Fdemo_mapItemIds::WeaponSlot,
			Fixture.FlyingSwordId);
	Udemo_mapItemSubsystem* Runtime =
		Fixture.GameInstance->GetSubsystem<Udemo_mapItemSubsystem>();
	if (!Selected.IsAccepted() || !Runtime)
	{
		AddError(TEXT(
			"Canonical flying-sword preparation could not reach Runtime."));
		return false;
	}
	Runtime->ResetForAutomation();
	const Fdemo_mapShanmenRunStartResult Started =
		Fdemo_mapShanmenRunLifecycleAdapter::StartPreparedRun(
			*Fixture.Authority, *Runtime);
	if (!Started.IsStarted())
	{
		AddError(FString::Printf(
			TEXT("Canonical flying-sword Run did not start: %s"),
			*Started.Diagnostic));
		return false;
	}
	TestTrue(TEXT("Exact selected flying sword owns the durable Run weapon slot"),
		Started.RunCorrelation.IsValid()
		&& Started.RunCorrelation.WeaponItemInstanceId
			== Fixture.FlyingSwordId
		&& Started.RunCorrelation.OrderedPreparedItemInstanceIds.Contains(
			Fixture.FlyingSwordId));

	APawn* Player = NewObject<APawn>(GetTransientPackage());
	UBoxComponent* PlayerRoot = Player
		? NewObject<UBoxComponent>(Player, TEXT("P211PlayerRoot"))
		: nullptr;
	Udemo_mapPlayerHealthComponent* PlayerHealth = Player
		? NewObject<Udemo_mapPlayerHealthComponent>(
			Player, TEXT("P211PlayerHealth"))
		: nullptr;
	AActor* Weapon = NewObject<AActor>(GetTransientPackage());
	UBoxComponent* WeaponRoot = Weapon
		? NewObject<UBoxComponent>(Weapon, TEXT("P211WeaponRoot"))
		: nullptr;
	if (!Player || !PlayerRoot || !PlayerHealth || !Weapon || !WeaponRoot)
	{
		AddError(TEXT("Canonical active-Run Actors could not be allocated."));
		return false;
	}
	Player->SetRootComponent(PlayerRoot);
	Weapon->SetRootComponent(WeaponRoot);

	Fdemo_mapCombatRunCoordinator Coordinator;
	FString CoordinatorDiagnostic;
	if (!Coordinator.TryBeginRun(
			Started.ActiveRunId,
			Player,
			PlayerHealth,
			CoordinatorDiagnostic))
	{
		AddError(FString::Printf(
			TEXT("Canonical CombatRunCoordinator did not start: %s"),
			*CoordinatorDiagnostic));
		return false;
	}

	Fdemo_mapShanmenControlledWeaponActiveRunIntent Intent;
	Intent.SourceItemInstanceId = Fixture.FlyingSwordId;
	Intent.ActivationSequence = 21;
	Intent.Definition.ActionDefinitionId =
		FShanmenControlledWeaponDefinition::CanonicalActionDefinitionId();
	Intent.Definition.DetectorId =
		TEXT("Detector.ControlledWeapon.P21.1.TrainingFlyingSword");
	Intent.Definition.FormulaId =
		TEXT("Formula.ControlledWeapon.P21.1.Training");
	Intent.Definition.BaseDamage = 10.0f;
	Intent.Definition.ControlPowerCoefficient = 0.25f;
	Intent.Definition.DamageTags.AddTag(
		FShanmenCombatNativeTags::DamagePhysicalSlash());
	Intent.Definition.RequiredTargetTags.AddTag(
		FShanmenCombatNativeTags::TargetLiving());
	Intent.ControlPower = 40.0f;
	Intent.SourceTags.AddTag(FShanmenCombatNativeTags::SourcePlayer());
	Intent.Motion.DirectedSpeed = 400.0f;
	Intent.Motion.OrbitCenterOffset = FVector(0.0, 0.0, 50.0);
	Intent.Motion.OrbitPlaneNormal = FVector::UpVector;
	Intent.Motion.OrbitReferenceAxis = FVector::ForwardVector;
	Intent.Motion.OrbitRadius = 100.0f;
	Intent.Motion.OrbitAngularSpeedRadiansPerSecond = UE_PI * 0.5f;
	Intent.Motion.InitialOrbitPhaseRadians = 0.0f;
	Intent.Motion.MaximumStepSeconds = 0.5f;

	FShanmenItemAuthoritySnapshot AuthorityBefore;
	if (!Fixture.Authority->TryCaptureSnapshot(AuthorityBefore))
	{
		AddError(TEXT("Canonical authority snapshot was unavailable."));
		return false;
	}
	Fdemo_mapShanmenControlledWeaponRunHost Host;
	const Fdemo_mapShanmenControlledWeaponActiveRunResult Result =
		Fdemo_mapShanmenControlledWeaponActiveRunRoute::TryStart(
			Fixture.Authority,
			Runtime,
			Coordinator,
			Host,
			Player,
			Weapon,
			WeaponRoot,
			Intent);
	FShanmenItemAuthoritySnapshot AuthorityAfter;
	const bool bCapturedAfter =
		Fixture.Authority->TryCaptureSnapshot(AuthorityAfter);
	TestTrue(TEXT("One canonical flying-sword instance crosses every boundary"),
		Result.IsStarted()
		&& Result.Preparation.Evidence.ItemDefinitionId
			== Fdemo_mapItemIds::TrainingFlyingSword
		&& Result.Preparation.Evidence.ItemInstanceId
			== Fixture.FlyingSwordId
		&& Result.Preparation.Action.GetRunId() == Started.ActiveRunId
		&& Result.Preparation.Action.GetSourceEntityId()
			== Coordinator.GetPlayerEntityId()
		&& Result.Preparation.Action.GetSourceTags().HasTagExact(
			FShanmenCombatNativeTags::SourcePlayer())
		&& Result.Preparation.Action.GetSourceTags().HasTagExact(
			FShanmenItemNativeTags::CapabilityDeploy())
		&& Result.Preparation.Action.GetSourceTags().HasTagExact(
			FShanmenItemNativeTags::ItemWeaponFlyingSword())
		&& Host.IsValid()
		&& Host.NumBound() == 1
		&& Host.NumOrbiting() == 1
		&& Host.GetOrderedItemInstanceIds()
			== TArray<FGuid>({ Fixture.FlyingSwordId }));
	TestTrue(TEXT("Canonical start is read-only against durable item authority"),
		bCapturedAfter && AuthorityAfter == AuthorityBefore);

	const Fdemo_mapShanmenControlledWeaponActiveRunResult Replay =
		Fdemo_mapShanmenControlledWeaponActiveRunRoute::TryStart(
			Fixture.Authority,
			Runtime,
			Coordinator,
			Host,
			Player,
			Weapon,
			WeaponRoot,
			Intent);
	FShanmenItemAuthoritySnapshot AuthorityAfterReplay;
	const bool bCapturedAfterReplay =
		Fixture.Authority->TryCaptureSnapshot(AuthorityAfterReplay);
	TestTrue(TEXT("Exact replay prepares the same evidence but cannot double bind"),
		Replay.Error
			== Edemo_mapShanmenControlledWeaponActiveRunError::
				AttachmentRejected
		&& Replay.Preparation.IsPrepared()
		&& Replay.Attachment.Error
			== Edemo_mapShanmenControlledWeaponHostAttachError::ItemAlreadyBound
		&& Host.NumBound() == 1
		&& bCapturedAfterReplay
		&& AuthorityAfterReplay == AuthorityBefore);

	const Fdemo_mapShanmenControlledWeaponRunEndResult Ended =
		Fdemo_mapShanmenControlledWeaponRunLifecycle::TryEndRun(
			Host, Coordinator);
	TestTrue(TEXT("Canonical controlled weapon retires before Run identity"),
		Ended.IsEnded()
		&& Host.IsEmpty()
		&& !Coordinator.IsActive());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenCanonicalControlledWeaponWorldLifecycleTest,
	"Shanmen.0_0_10.Product.ControlledWeaponWorldLifecycle.CanonicalTrainingFlyingSword",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenCanonicalControlledWeaponWorldLifecycleTest::RunTest(
	const FString&)
{
	FPreparationAdapterFixture Fixture;
	if (!Fixture.StartAndCutover(
			*this, TEXT("CanonicalControlledWeaponWorldLifecycle")))
	{
		return false;
	}
	const Fdemo_mapProfilePreparationSelectionResult Selected =
		Fixture.Session->SetPreparationEquipment(
			Fdemo_mapItemIds::WeaponSlot,
			Fixture.FlyingSwordId);
	Udemo_mapItemSubsystem* Runtime =
		Fixture.GameInstance->GetSubsystem<Udemo_mapItemSubsystem>();
	if (!Selected.IsAccepted() || !Runtime)
	{
		AddError(TEXT("P21.2 flying-sword selection could not reach Runtime."));
		return false;
	}
	Runtime->ResetForAutomation();
	const Fdemo_mapShanmenRunStartResult Started =
		Fdemo_mapShanmenRunLifecycleAdapter::StartPreparedRun(
			*Fixture.Authority, *Runtime);
	if (!Started.IsStarted()
		|| Started.RunCorrelation.WeaponItemInstanceId
			!= Fixture.FlyingSwordId)
	{
		AddError(FString::Printf(
			TEXT("P21.2 durable flying-sword Run failed: %s"),
			*Started.Diagnostic));
		return false;
	}

	FControlledWeaponWorldFixture WorldFixture;
	if (!WorldFixture.Start(
			*this, Fixture.GameInstance, Started.ActiveRunId))
	{
		return false;
	}
	FShanmenItemAuthoritySnapshot AuthorityBefore;
	if (!Fixture.Authority->TryCaptureSnapshot(AuthorityBefore))
	{
		AddError(TEXT("P21.2 could not capture authority before World start."));
		return false;
	}

	const Fdemo_mapShanmenControlledWeaponWorldStartResult Began =
		WorldFixture.Lifecycle.TryBegin(
			WorldFixture.World,
			Fixture.Authority,
			Runtime,
			WorldFixture.Coordinator,
			WorldFixture.Host,
			WorldFixture.Player,
			1);
	Ademo_mapShanmenControlledWeaponActor* Weapon =
		WorldFixture.Lifecycle.GetWeaponActor();
	const FVector ExpectedInitialLocation =
		WorldFixture.Player->GetActorLocation()
		+ FVector(100.0f, 0.0f, 50.0f);
	TestTrue(TEXT("Canonical Run creates exactly one bound physical flying sword"),
		Began.IsStarted()
		&& WorldFixture.Lifecycle.IsActive()
		&& Weapon
		&& Weapon->IsProductBoundTo(
			Started.ActiveRunId,
			Fixture.FlyingSwordId,
			WorldFixture.Player)
		&& Weapon->GetCollisionComponent()
		&& Weapon->GetCollisionComponent()->GetCollisionEnabled()
			== ECollisionEnabled::QueryOnly
		&& WorldFixture.Host.IsValid()
		&& WorldFixture.Host.NumBound() == 1
		&& WorldFixture.CountControlledWeaponActors() == 1);
	TestTrue(TEXT("Spawn pose is defense-ready before the first frame pump"),
		Weapon
		&& Weapon->GetActorLocation().Equals(
			ExpectedInitialLocation, KINDA_SMALL_NUMBER));
	const TArray<FGuid> RequestedItems = { Fixture.FlyingSwordId };
	Fdemo_mapShanmenControlledWeaponDefenseReadinessBatch Readiness;
	TestTrue(TEXT("Initial physical pose backs exact-item defense readiness"),
		WorldFixture.Host.TryCaptureOrbitDefenseReadinessInOrder(
			RequestedItems, Readiness)
		&& Readiness.IsFullyCaptured()
		&& Readiness.Entries.Num() == 1
		&& Readiness.Entries[0].ItemInstanceId
			== Fixture.FlyingSwordId
		&& WorldFixture.Host.IsOrbitDefenseReadinessCurrent(Readiness));

	FShanmenItemAuthoritySnapshot AuthorityAfterStart;
	TestTrue(TEXT("World projection is read-only against durable item authority"),
		Fixture.Authority->TryCaptureSnapshot(AuthorityAfterStart)
		&& AuthorityAfterStart == AuthorityBefore);
	const Fdemo_mapShanmenControlledWeaponWorldStartResult Duplicate =
		WorldFixture.Lifecycle.TryBegin(
			WorldFixture.World,
			Fixture.Authority,
			Runtime,
			WorldFixture.Coordinator,
			WorldFixture.Host,
			WorldFixture.Player,
			2);
	TestTrue(TEXT("Lifecycle rejects duplicate World publication without spawning"),
		Duplicate.Status
			== Edemo_mapShanmenControlledWeaponWorldStartStatus::LifecycleBusy
		&& WorldFixture.Lifecycle.GetWeaponActor() == Weapon
		&& WorldFixture.Host.NumBound() == 1
		&& WorldFixture.CountControlledWeaponActors() == 1);

	const FVector BeforeOrbit = Weapon
		? Weapon->GetActorLocation() : FVector::ZeroVector;
	const Fdemo_mapShanmenControlledWeaponOrbitFrameResult Orbit =
		WorldFixture.Host.AdvanceOrbitingFrame(0.25f);
	TestTrue(TEXT("Existing frame owner advances the same physical Actor"),
		Orbit.IsAdvanced()
		&& Orbit.Batch.IsFullyAdvanced()
		&& Weapon
		&& !Weapon->GetActorLocation().Equals(
			BeforeOrbit, KINDA_SMALL_NUMBER));

	FString EarlyRetirementDiagnostic;
	TestFalse(TEXT("World Actor cannot retire before logical Host teardown"),
		WorldFixture.Lifecycle.TryEndAfterRun(
			Started.ActiveRunId,
			WorldFixture.Host,
			EarlyRetirementDiagnostic));
	TestTrue(TEXT("Rejected early retirement preserves Actor and Host"),
		WorldFixture.Lifecycle.IsActive()
		&& WorldFixture.Host.NumBound() == 1
		&& WorldFixture.CountControlledWeaponActors() == 1);

	const Fdemo_mapShanmenControlledWeaponRunEndResult Ended =
		Fdemo_mapShanmenControlledWeaponRunLifecycle::TryEndRun(
			WorldFixture.Host, WorldFixture.Coordinator);
	TestTrue(TEXT("Logical Host retires atomically before physical carrier"),
		Ended.IsEnded()
		&& WorldFixture.Host.IsEmpty()
		&& !WorldFixture.Coordinator.IsActive()
		&& WorldFixture.Lifecycle.IsActive()
		&& WorldFixture.CountControlledWeaponActors() == 1);
	FString RetirementDiagnostic;
	TestTrue(TEXT("Physical carrier retires after exact logical Run end"),
		WorldFixture.Lifecycle.TryEndAfterRun(
			Ended.RunId,
			WorldFixture.Host,
			RetirementDiagnostic)
		&& WorldFixture.Lifecycle.IsEmpty()
		&& WorldFixture.CountControlledWeaponActors() == 0
		&& (!::IsValid(Weapon) || Weapon->IsActorBeingDestroyed()));

	FShanmenItemAuthoritySnapshot AuthorityAfterEnd;
	TestTrue(TEXT("World lifecycle never rewrites durable item evidence"),
		Fixture.Authority->TryCaptureSnapshot(AuthorityAfterEnd)
		&& AuthorityAfterEnd == AuthorityBefore);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenControlledWeaponWorldThreatSamplingTest,
	"Shanmen.0_0_10.Product.ControlledWeaponWorldThreatSampler.CanonicalCadenceOverlapAndFences",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenControlledWeaponWorldThreatSamplingTest::RunTest(
	const FString&)
{
	FPreparationAdapterFixture Fixture;
	if (!Fixture.StartAndCutover(
			*this, TEXT("ControlledWeaponWorldThreatSampling")))
	{
		return false;
	}
	const Fdemo_mapProfilePreparationSelectionResult Selected =
		Fixture.Session->SetPreparationEquipment(
			Fdemo_mapItemIds::WeaponSlot,
			Fixture.FlyingSwordId);
	Udemo_mapItemSubsystem* Runtime =
		Fixture.GameInstance->GetSubsystem<Udemo_mapItemSubsystem>();
	if (!Selected.IsAccepted() || !Runtime)
	{
		AddError(TEXT("P21.3 flying-sword selection could not reach Runtime."));
		return false;
	}
	Runtime->ResetForAutomation();
	const Fdemo_mapShanmenRunStartResult Started =
		Fdemo_mapShanmenRunLifecycleAdapter::StartPreparedRun(
			*Fixture.Authority, *Runtime);
	if (!Started.IsStarted())
	{
		AddError(FString::Printf(
			TEXT("P21.3 durable flying-sword Run failed: %s"),
			*Started.Diagnostic));
		return false;
	}

	FControlledWeaponWorldFixture WorldFixture;
	if (!WorldFixture.Start(
			*this, Fixture.GameInstance, Started.ActiveRunId))
	{
		return false;
	}
	const Fdemo_mapShanmenControlledWeaponWorldStartResult Began =
		WorldFixture.Lifecycle.TryBegin(
			WorldFixture.World,
			Fixture.Authority,
			Runtime,
			WorldFixture.Coordinator,
			WorldFixture.Host,
			WorldFixture.Player,
			1);
	FString SamplerDiagnostic;
	if (!Began.IsStarted()
		|| !WorldFixture.ThreatSampler.TryBegin(
			Started.ActiveRunId,
			WorldFixture.Timeline.GetTimelineId(),
			WorldFixture.Lifecycle,
			SamplerDiagnostic))
	{
		AddError(FString::Printf(
			TEXT("P21.3 World threat cadence failed to begin: %s"),
			*SamplerDiagnostic));
		return false;
	}

	Ademo_mapShanmenControlledWeaponActor* Weapon =
		WorldFixture.Lifecycle.GetWeaponActor();
	Ademo_mapEnemyCharacter* Enemy = Weapon
		? WorldFixture.SpawnRegisteredEnemy(
			*this, Weapon->GetActorLocation())
		: nullptr;
	if (!Weapon || !Enemy)
	{
		return false;
	}
	const float VitalityBefore = Enemy->GetCurrentVitality();
	const int32 ImpactsBefore = Enemy->NumCommittedCombatImpacts();
	Fdemo_mapShanmenCombatRunTimelineSample TickZero;
	if (!WorldFixture.Timeline.TryCapture(TickZero))
	{
		AddError(TEXT("P21.3 could not capture timeline tick zero."));
		return false;
	}

	const Fdemo_mapShanmenControlledWeaponWorldThreatSampleResult First =
		WorldFixture.ThreatSampler.TrySample(
			WorldFixture.World,
			TickZero,
			WorldFixture.Lifecycle,
			WorldFixture.Coordinator,
			WorldFixture.Host,
			WorldFixture.ThreatRouter);
	const bool bFirstSampleValid =
		First.IsSampled()
		&& First.ObservedTick == 0
		&& First.ScheduledTick == 0
		&& First.SampleSequence == 0
		&& First.RawOverlapCount >= 1
		&& First.RoutedContactCount == 1
		&& First.IntentId
			== Fdemo_mapShanmenControlledWeaponWorldThreatSampler::MakeIntentId(
				Started.ActiveRunId,
				WorldFixture.Timeline.GetTimelineId(),
				Fixture.FlyingSwordId,
				0,
				0)
		&& WorldFixture.ThreatRouter.GetNextSampleSequence() == 1
		&& WorldFixture.ThreatSampler.NumCommittedSamples() == 1
		&& WorldFixture.ThreatSampler.GetNextScheduledTick() == 3
		&& WorldFixture.Host.NumConsumedThreatPresenceIntents() == 1;
	if (!bFirstSampleValid)
	{
		AddError(FString::Printf(
			TEXT("P21.3 tick-zero audit Status=%d Observed=%lld Scheduled=%lld Sequence=%lld Raw=%d Routed=%d RouteStatus=%d RouteAccepted=%d RouterNext=%lld OwnerCount=%lld OwnerNext=%lld HostConsumed=%d Diagnostic=%s"),
			static_cast<int32>(First.Status),
			static_cast<long long>(First.ObservedTick),
			static_cast<long long>(First.ScheduledTick),
			static_cast<long long>(First.SampleSequence),
			First.RawOverlapCount,
			First.RoutedContactCount,
			static_cast<int32>(First.Route.Status),
			First.Route.IsAccepted() ? 1 : 0,
			static_cast<long long>(
				WorldFixture.ThreatRouter.GetNextSampleSequence()),
			static_cast<long long>(
				WorldFixture.ThreatSampler.NumCommittedSamples()),
			static_cast<long long>(
				WorldFixture.ThreatSampler.GetNextScheduledTick()),
			WorldFixture.Host.NumConsumedThreatPresenceIntents(),
			*First.Diagnostic));
	}
	TestTrue(TEXT("Tick zero samples the physical sword box through P6.22"),
		bFirstSampleValid);
	TestTrue(TEXT("Near-body threat remains a zero-effect observation"),
		Enemy->GetCurrentVitality() == VitalityBefore
		&& Enemy->NumCommittedCombatImpacts() == ImpactsBefore);

	const Fdemo_mapShanmenControlledWeaponWorldThreatSampleResult SameTick =
		WorldFixture.ThreatSampler.TrySample(
			WorldFixture.World,
			TickZero,
			WorldFixture.Lifecycle,
			WorldFixture.Coordinator,
			WorldFixture.Host,
			WorldFixture.ThreatRouter);
	TestTrue(TEXT("Repeated owner calls inside one cadence window are no-ops"),
		SameTick.IsNoOp()
		&& SameTick.Status
			== Edemo_mapShanmenControlledWeaponWorldThreatSampleStatus::NotDue
		&& WorldFixture.ThreatSampler.NumCommittedSamples() == 1
		&& WorldFixture.ThreatRouter.GetNextSampleSequence() == 1);

	int64 AdvancedTicks = 0;
	FString TimelineDiagnostic;
	if (!WorldFixture.Timeline.TryAdvance(
			2.0 / 30.0, AdvancedTicks, TimelineDiagnostic))
	{
		AddError(TimelineDiagnostic);
		return false;
	}
	Fdemo_mapShanmenCombatRunTimelineSample TickTwo;
	WorldFixture.Timeline.TryCapture(TickTwo);
	const Fdemo_mapShanmenControlledWeaponWorldThreatSampleResult BeforeDue =
		WorldFixture.ThreatSampler.TrySample(
			WorldFixture.World,
			TickTwo,
			WorldFixture.Lifecycle,
			WorldFixture.Coordinator,
			WorldFixture.Host,
			WorldFixture.ThreatRouter);
	TestTrue(TEXT("Two canonical ticks remain below the 10 Hz sample cadence"),
		AdvancedTicks == 2
		&& TickTwo.GetCurrentTick() == 2
		&& BeforeDue.IsNoOp()
		&& BeforeDue.Status
			== Edemo_mapShanmenControlledWeaponWorldThreatSampleStatus::NotDue);

	Enemy->SetActorLocation(
		Weapon->GetActorLocation() + FVector(1000.0f, 0.0f, 0.0f),
		false,
		nullptr,
		ETeleportType::TeleportPhysics);
	WorldFixture.World->UpdateWorldComponents(true, false);
	if (!WorldFixture.Timeline.TryAdvance(
			1.0 / 30.0, AdvancedTicks, TimelineDiagnostic))
	{
		AddError(TimelineDiagnostic);
		return false;
	}
	Fdemo_mapShanmenCombatRunTimelineSample TickThree;
	WorldFixture.Timeline.TryCapture(TickThree);
	const Fdemo_mapShanmenControlledWeaponWorldThreatSampleResult Empty =
		WorldFixture.ThreatSampler.TrySample(
			WorldFixture.World,
			TickThree,
			WorldFixture.Lifecycle,
			WorldFixture.Coordinator,
			WorldFixture.Host,
			WorldFixture.ThreatRouter);
	TestTrue(TEXT("A due empty query still advances one auditable sample"),
		TickThree.GetCurrentTick() == 3
		&& Empty.IsSampled()
		&& Empty.SampleSequence == 1
		&& Empty.ScheduledTick == 3
		&& Empty.RoutedContactCount == 0
		&& Empty.Route.Batch.Entries.Num() == 1
		&& Empty.Route.Batch.Entries[0].Finalization.GetConsumption()
			.GetStatus()
				== EShanmenControlledWeaponThreatPresenceConsumeStatus::NoOp
		&& WorldFixture.ThreatSampler.NumCommittedSamples() == 2
		&& WorldFixture.ThreatSampler.GetNextScheduledTick() == 6);

	if (!WorldFixture.Timeline.TryAdvance(
			1.0, AdvancedTicks, TimelineDiagnostic))
	{
		AddError(TimelineDiagnostic);
		return false;
	}
	Fdemo_mapShanmenCombatRunTimelineSample TickThirtyThree;
	WorldFixture.Timeline.TryCapture(TickThirtyThree);
	const Fdemo_mapShanmenControlledWeaponWorldThreatSampleResult BoundedCatchUp =
		WorldFixture.ThreatSampler.TrySample(
			WorldFixture.World,
			TickThirtyThree,
			WorldFixture.Lifecycle,
			WorldFixture.Coordinator,
			WorldFixture.Host,
			WorldFixture.ThreatRouter);
	TestTrue(TEXT("A long frame performs one query and skips stale cadence slots"),
		AdvancedTicks == 30
		&& TickThirtyThree.GetCurrentTick() == 33
		&& BoundedCatchUp.IsSampled()
		&& BoundedCatchUp.SampleSequence == 2
		&& BoundedCatchUp.ScheduledTick == 6
		&& WorldFixture.ThreatSampler.NumCommittedSamples() == 3
		&& WorldFixture.ThreatSampler.GetNextScheduledTick() == 36
		&& WorldFixture.ThreatRouter.GetNextSampleSequence() == 3);

	Fdemo_mapShanmenCombatRunTimelineSample ForeignTimeline;
	Fdemo_mapShanmenCombatRunTimelineSample::TryCapture(
		FGuid(0xF213FFFF, 0, 0, 1),
		36,
		ForeignTimeline);
	const Fdemo_mapShanmenControlledWeaponWorldThreatSampleResult Foreign =
		WorldFixture.ThreatSampler.TrySample(
			WorldFixture.World,
			ForeignTimeline,
			WorldFixture.Lifecycle,
			WorldFixture.Coordinator,
			WorldFixture.Host,
			WorldFixture.ThreatRouter);
	TestTrue(TEXT("A foreign timeline cannot spend the next sample sequence"),
		Foreign.Status
			== Edemo_mapShanmenControlledWeaponWorldThreatSampleStatus::
				TimelineMismatch
		&& WorldFixture.ThreatSampler.NumCommittedSamples() == 3
		&& WorldFixture.ThreatRouter.GetNextSampleSequence() == 3
		&& Enemy->GetCurrentVitality() == VitalityBefore
		&& Enemy->NumCommittedCombatImpacts() == ImpactsBefore);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenControlledWeaponThreatCueTest,
	"Shanmen.0_0_10.Product.ControlledWeaponThreatCue.AcceptedPresenceAndEmptyRelease",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenControlledWeaponThreatCueTest::RunTest(const FString&)
{
	FPreparationAdapterFixture Fixture;
	if (!Fixture.StartAndCutover(
			*this, TEXT("ControlledWeaponThreatCue")))
	{
		return false;
	}
	const Fdemo_mapProfilePreparationSelectionResult Selected =
		Fixture.Session->SetPreparationEquipment(
			Fdemo_mapItemIds::WeaponSlot,
			Fixture.FlyingSwordId);
	Udemo_mapItemSubsystem* Runtime =
		Fixture.GameInstance->GetSubsystem<Udemo_mapItemSubsystem>();
	if (!Selected.IsAccepted() || !Runtime)
	{
		AddError(TEXT("P21.4 flying-sword selection could not reach Runtime."));
		return false;
	}
	Runtime->ResetForAutomation();
	const Fdemo_mapShanmenRunStartResult Started =
		Fdemo_mapShanmenRunLifecycleAdapter::StartPreparedRun(
			*Fixture.Authority, *Runtime);
	if (!Started.IsStarted())
	{
		AddError(FString::Printf(
			TEXT("P21.4 durable flying-sword Run failed: %s"),
			*Started.Diagnostic));
		return false;
	}

	FControlledWeaponWorldFixture WorldFixture;
	if (!WorldFixture.Start(
			*this, Fixture.GameInstance, Started.ActiveRunId))
	{
		return false;
	}
	const Fdemo_mapShanmenControlledWeaponWorldStartResult Began =
		WorldFixture.Lifecycle.TryBegin(
			WorldFixture.World,
			Fixture.Authority,
			Runtime,
			WorldFixture.Coordinator,
			WorldFixture.Host,
			WorldFixture.Player,
			1);
	FString SamplerDiagnostic;
	if (!Began.IsStarted()
		|| !WorldFixture.ThreatSampler.TryBegin(
			Started.ActiveRunId,
			WorldFixture.Timeline.GetTimelineId(),
			WorldFixture.Lifecycle,
			SamplerDiagnostic))
	{
		AddError(FString::Printf(
			TEXT("P21.4 World threat cadence failed to begin: %s"),
			*SamplerDiagnostic));
		return false;
	}

	Ademo_mapShanmenControlledWeaponActor* Weapon =
		WorldFixture.Lifecycle.GetWeaponActor();
	Ademo_mapEnemyCharacter* Enemy = Weapon
		? WorldFixture.SpawnRegisteredEnemy(
			*this, Weapon->GetActorLocation())
		: nullptr;
	if (!Weapon || !Enemy)
	{
		return false;
	}
	const float VitalityBefore = Enemy->GetCurrentVitality();
	const int32 ImpactsBefore = Enemy->NumCommittedCombatImpacts();
	TestTrue(TEXT("Fresh flying sword starts with an idle threat cue"),
		!Weapon->IsThreatPresenceCueActive()
		&& !Weapon->IsThreatPresenceCueVisualActive()
		&& Weapon->GetThreatPresenceCueContactCount() == 0
		&& Weapon->GetLastThreatPresenceCueSampleSequence() == INDEX_NONE
		&& !Weapon->GetLastThreatPresenceCueIntentId().IsValid());

	Fdemo_mapShanmenCombatRunTimelineSample TickZero;
	if (!WorldFixture.Timeline.TryCapture(TickZero))
	{
		AddError(TEXT("P21.4 could not capture timeline tick zero."));
		return false;
	}
	const Fdemo_mapShanmenControlledWeaponWorldThreatSampleResult Presence =
		WorldFixture.ThreatSampler.TrySample(
			WorldFixture.World,
			TickZero,
			WorldFixture.Lifecycle,
			WorldFixture.Coordinator,
			WorldFixture.Host,
			WorldFixture.ThreatRouter);
	TestTrue(TEXT("Accepted nearby presence lights the same physical sword"),
		Presence.IsSampled()
		&& Presence.RoutedContactCount == 1
		&& Weapon->TryPresentThreatPresenceCue(Presence)
		&& Weapon->IsThreatPresenceCueActive()
		&& Weapon->IsThreatPresenceCueVisualActive()
		&& Weapon->GetThreatPresenceCueContactCount() == 1
		&& Weapon->GetLastThreatPresenceCueSampleSequence() == 0
		&& Weapon->GetLastThreatPresenceCueIntentId() == Presence.IntentId);
	TestTrue(TEXT("Exact sample replay is presentation-idempotent"),
		Weapon->TryPresentThreatPresenceCue(Presence)
		&& Weapon->GetLastThreatPresenceCueSampleSequence() == 0
		&& Weapon->GetLastThreatPresenceCueIntentId() == Presence.IntentId
		&& Weapon->GetThreatPresenceCueContactCount() == 1
		&& Weapon->IsThreatPresenceCueVisualActive());

	Fdemo_mapShanmenControlledWeaponWorldThreatSampleResult
		ConflictingCount = Presence;
	ConflictingCount.RawOverlapCount = 2;
	ConflictingCount.RoutedContactCount = 2;
	TestFalse(TEXT("Same-sequence contact-count conflicts fail closed"),
		Weapon->TryPresentThreatPresenceCue(ConflictingCount));
	TestTrue(TEXT("Rejected count conflict preserves the accepted readout"),
		Weapon->GetThreatPresenceCueContactCount() == 1
		&& Weapon->GetLastThreatPresenceCueSampleSequence() == 0
		&& Weapon->GetLastThreatPresenceCueIntentId() == Presence.IntentId
		&& Weapon->IsThreatPresenceCueVisualActive());

	Fdemo_mapShanmenControlledWeaponWorldThreatSampleResult Foreign = Presence;
	Foreign.ItemInstanceId = FGuid(0xF214FFFF, 0, 0, 1);
	TestFalse(TEXT("A foreign item cannot repaint the bound sword"),
		Weapon->TryPresentThreatPresenceCue(Foreign));
	TestTrue(TEXT("Rejected foreign presentation preserves accepted state"),
		Weapon->GetLastThreatPresenceCueSampleSequence() == 0
		&& Weapon->GetLastThreatPresenceCueIntentId() == Presence.IntentId
		&& Weapon->GetThreatPresenceCueContactCount() == 1
		&& Weapon->IsThreatPresenceCueVisualActive());

	Enemy->SetActorLocation(
		Weapon->GetActorLocation() + FVector(1000.0f, 0.0f, 0.0f),
		false,
		nullptr,
		ETeleportType::TeleportPhysics);
	WorldFixture.World->UpdateWorldComponents(true, false);
	int64 AdvancedTicks = 0;
	FString TimelineDiagnostic;
	if (!WorldFixture.Timeline.TryAdvance(
			3.0 / 30.0, AdvancedTicks, TimelineDiagnostic))
	{
		AddError(TimelineDiagnostic);
		return false;
	}
	Fdemo_mapShanmenCombatRunTimelineSample TickThree;
	WorldFixture.Timeline.TryCapture(TickThree);
	const Fdemo_mapShanmenControlledWeaponWorldThreatSampleResult Empty =
		WorldFixture.ThreatSampler.TrySample(
			WorldFixture.World,
			TickThree,
			WorldFixture.Lifecycle,
			WorldFixture.Coordinator,
			WorldFixture.Host,
			WorldFixture.ThreatRouter);
	TestTrue(TEXT("Accepted empty sample releases the visible threat cue"),
		AdvancedTicks == 3
		&& Empty.IsSampled()
		&& Empty.SampleSequence == 1
		&& Empty.RoutedContactCount == 0
		&& Weapon->TryPresentThreatPresenceCue(Empty)
		&& !Weapon->IsThreatPresenceCueActive()
		&& !Weapon->IsThreatPresenceCueVisualActive()
		&& Weapon->GetThreatPresenceCueContactCount() == 0
		&& Weapon->GetLastThreatPresenceCueSampleSequence() == 1
		&& Weapon->GetLastThreatPresenceCueIntentId() == Empty.IntentId);
	TestFalse(TEXT("A stale presence cannot relight the sword"),
		Weapon->TryPresentThreatPresenceCue(Presence));
	TestTrue(TEXT("Threat presentation never mutates enemy combat authority"),
		!Weapon->IsThreatPresenceCueActive()
		&& Weapon->GetThreatPresenceCueContactCount() == 0
		&& Weapon->GetLastThreatPresenceCueSampleSequence() == 1
		&& Enemy->GetCurrentVitality() == VitalityBefore
		&& Enemy->NumCommittedCombatImpacts() == ImpactsBefore);
	return true;
}

#endif
