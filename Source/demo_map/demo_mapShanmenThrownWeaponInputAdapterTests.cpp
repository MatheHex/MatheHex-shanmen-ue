#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenThrownWeaponInputAdapter.h"

#include "demo_map0909BSectWarehouseService.h"
#include "demo_mapAttributeComponent.h"
#include "demo_mapCombatRunCoordinator.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapPlayerHealthComponent.h"
#include "demo_mapProfileRepository.h"
#include "demo_mapProfileSessionSubsystem.h"
#include "demo_mapShanmenItemAuthoritySubsystem.h"
#include "demo_mapShanmenItemCutover.h"
#include "demo_mapShanmenPreparationAdapter.h"
#include "demo_mapShanmenRunLifecycleAdapter.h"
#include "demo_mapShanmenThrownWeaponProjectile.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	FString NewThrownInputRoot(const TCHAR* Label)
	{
		return FPaths::Combine(
			FPaths::ProjectSavedDir(),
			TEXT("Automation"),
			TEXT("Dev.D.UE.0.0.10.P7.9.r0"),
			Label,
			FGuid::NewGuid().ToString(EGuidFormats::Digits));
	}

	struct FThrownInputFixture
	{
		FString Root;
		Fdemo_mapProfileStorageContext Storage;
		Fdemo_mapPersistentProfile SeedProfile;
		FGuid TrainingBladeId;
		FGuid HealingPillId;
		FGuid ThrowingKnifeId;
		UGameInstance* GameInstance = nullptr;
		Udemo_mapShanmenItemAuthoritySubsystem* Authority = nullptr;
		Udemo_mapProfileSessionSubsystem* ProfileSession = nullptr;
		UWorld* World = nullptr;
		APawn* Source = nullptr;
		APawn* OtherSource = nullptr;
		Udemo_mapPlayerHealthComponent* Health = nullptr;
		Udemo_mapAttributeComponent* Attributes = nullptr;
		Fdemo_mapShanmenRunCorrelation Correlation;
		Fdemo_mapCombatRunCoordinator Coordinator;
		Fdemo_mapShanmenThrownWeaponProductLifecycle Lifecycle;
		Fdemo_mapShanmenThrownWeaponInputAdapter Adapter;

		bool Start(FAutomationTestBase& Test, const TCHAR* Label)
		{
			Root = NewThrownInputRoot(Label);
			Storage = Fdemo_mapProfileStorageContext::ForRoot(Root);
			Fdemo_mapProfileRepository Repository;
			SeedProfile = Repository.CreateFreshProfile();
			for (const Fdemo_mapPersistentItemRecord& Item :
				SeedProfile.PermanentStash)
			{
				if (Item.ItemDefinitionId == Fdemo_mapItemIds::TrainingBlade)
				{
					TrainingBladeId = Item.ItemInstanceId;
				}
			}
			auto AddConsumable = [this](
				const FName DefinitionId,
				const int32 Quantity,
				FGuid& OutId)
			{
				Fdemo_mapPersistentItemRecord Item;
				OutId = Item.ItemInstanceId = FGuid::NewGuid();
				Item.ItemDefinitionId = DefinitionId;
				Item.StackCount = Quantity;
				Item.PersistentDomain =
					Edemo_mapPersistentDomain::PermanentStash;
				SeedProfile.PermanentStash.Add(Item);
			};
			AddConsumable(Fdemo_mapItemIds::HealingPillLevel1, 2, HealingPillId);
			AddConsumable(
				Fdemo_mapItemIds::TrainingThrowingKnife, 3, ThrowingKnifeId);
			SeedProfile.PreparationLayout.WeaponItemInstanceId = TrainingBladeId;
			const Fdemo_mapProfileSaveResult Saved =
				Repository.SaveProfile(SeedProfile, Storage);
			if (!TrainingBladeId.IsValid() || !HealingPillId.IsValid()
				|| !ThrowingKnifeId.IsValid() || !Saved.IsSuccess() || !GEngine)
			{
				Test.AddError(TEXT("P7.9 could not seed isolated hotbar content."));
				return false;
			}

			GameInstance = NewObject<UGameInstance>(
				GEngine, NAME_None, RF_Transient);
			if (!GameInstance)
			{
				return false;
			}
			GameInstance->AddToRoot();
			GameInstance->Init();
			Authority = GameInstance->GetSubsystem<
				Udemo_mapShanmenItemAuthoritySubsystem>();
			ProfileSession = GameInstance->GetSubsystem<
				Udemo_mapProfileSessionSubsystem>();
			if (!Authority || !ProfileSession)
			{
				return false;
			}
			const Fdemo_mapProfileSessionInitializeResult Initialized =
				ProfileSession->InitializeSession(Storage);
			Fdemo_map0909BSectWarehouseService Warehouse;
			Fdemo_map0909BWarehousePresentation Presentation;
			FString Diagnostic;
			if (!Initialized.IsReady()
				|| !Warehouse.OpenForSect(
					Root,
					Initialized.Snapshot,
					Edemo_map0909BTopState::AtSect,
					Presentation,
					Diagnostic))
			{
				Test.AddError(FString::Printf(
					TEXT("P7.9 stable migration source failed: %s"),
					*Diagnostic));
				return false;
			}
			const Fdemo_mapShanmenItemCutoverResult Cutover =
				Fdemo_mapShanmenItemCutoverCoordinator::Execute(
					Storage,
					SeedProfile.ProfileId,
					*Authority,
					*ProfileSession,
					Warehouse);
			if (!Cutover.IsReady()
				|| !ProfileSession->SetPreparationEquipment(
					Fdemo_mapItemIds::WeaponSlot,
					TrainingBladeId).IsAccepted()
				|| !ProfileSession->SetPreparationMaterial(
					HealingPillId, true).IsAccepted()
				|| !ProfileSession->SetPreparationMaterial(
					ThrowingKnifeId, true).IsAccepted()
				|| !ProfileSession->SetPreparationHotbarSlot(
					1, HealingPillId).IsAccepted()
				|| !ProfileSession->SetPreparationHotbarSlot(
					2, ThrowingKnifeId).IsAccepted())
			{
				Test.AddError(FString::Printf(
					TEXT("P7.9 authority preparation failed: %s"),
					*Cutover.Diagnostic));
				return false;
			}
			const Fdemo_mapShanmenPreparedLoadoutResult Started =
				Fdemo_mapShanmenPreparationAdapter::
					StartPreparedLoadout(*Authority);
			if (!Started.IsCommitted()
				|| !Fdemo_mapShanmenRunLifecycleAdapter::
					TryGetActiveRunCorrelation(
						*Authority, Correlation, &Diagnostic)
				|| Correlation.HotbarItemInstanceIds[0] != HealingPillId
				|| Correlation.HotbarItemInstanceIds[1] != ThrowingKnifeId)
			{
				Test.AddError(FString::Printf(
					TEXT("P7.9 durable active Run failed: %s"),
					*Diagnostic));
				return false;
			}

			World = NewObject<UWorld>(
				GetTransientPackage(), NAME_None, RF_Transient);
			if (!World)
			{
				return false;
			}
			World->WorldType = EWorldType::GamePreview;
			FWorldContext& Context =
				GEngine->CreateNewWorldContext(EWorldType::GamePreview);
			Context.OwningGameInstance = GameInstance;
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
			Source = World->SpawnActor<APawn>();
			OtherSource = World->SpawnActor<APawn>();
			Health = Source
				? NewObject<Udemo_mapPlayerHealthComponent>(
					Source, TEXT("P79PlayerHealth"))
				: nullptr;
			Attributes = Source
				? NewObject<Udemo_mapAttributeComponent>(
					Source, TEXT("P79PlayerAttributes"))
				: nullptr;
			if (Source && Attributes)
			{
				Source->AddInstanceComponent(Attributes);
				Source->SetActorLocation(FVector(100.0, 200.0, 30.0));
			}
			if (!Source || !OtherSource || !Health || !Attributes
				|| !Coordinator.TryBeginRun(
					Correlation.ActiveRunId,
					Source,
					Health,
					Diagnostic)
				|| !Lifecycle.TryBegin(
					*Authority,
					*Source,
					Coordinator,
					Diagnostic))
			{
				Test.AddError(FString::Printf(
					TEXT("P7.9 product lifecycle failed: %s"),
					*Diagnostic));
				return false;
			}
			return true;
		}

		Fdemo_mapShanmenThrownWeaponInputResult Route(
			const int32 Slot,
			AActor* RequestedSource,
			int32& AimSampleCount,
			const FVector Aim)
		{
			return Adapter.RouteHotbarInput(
				Authority,
				Lifecycle,
				Coordinator,
				World,
				Ademo_mapShanmenThrownWeaponProjectile::StaticClass(),
				RequestedSource,
				Slot,
				[&AimSampleCount, Aim]()
				{
					++AimSampleCount;
					return Aim;
				},
				[this]()
				{
					return Fdemo_mapShanmenPlayerActionGateResult::
						FromArbitration(
							Coordinator.TryAuthorizePlayerAction(
								Edemo_mapShanmenPlayerActionKind::
									ThrownWeapon,
								Fdemo_mapShanmenPlayerActionOccupancySnapshot()));
				});
		}

		void Stop()
		{
			if (Lifecycle.IsActive())
			{
				FString Diagnostic;
				Lifecycle.TryEnd(Diagnostic);
			}
			if (Coordinator.IsActive())
			{
				FString Diagnostic;
				Coordinator.TryEndRun(
					Correlation.ActiveRunId, Diagnostic);
			}
			if (World)
			{
				World->DestroyWorld(false);
				if (GEngine)
				{
					GEngine->DestroyWorldContext(World);
				}
				World = nullptr;
				Source = nullptr;
				OtherSource = nullptr;
				Health = nullptr;
				Attributes = nullptr;
			}
			if (GameInstance)
			{
				GameInstance->Shutdown();
				Authority = nullptr;
				ProfileSession = nullptr;
				GameInstance->RemoveFromRoot();
				GameInstance->MarkAsGarbage();
				GameInstance = nullptr;
				CollectGarbage(RF_NoFlags);
			}
			if (!Root.IsEmpty())
			{
				IFileManager::Get().DeleteDirectory(
					*Root, false, true);
				Root.Reset();
			}
		}

		~FThrownInputFixture()
		{
			Stop();
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponInputIdentityTest,
	"Shanmen.0_0_10.Product.ThrownWeaponInputAdapter.DeterministicIdentity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponInputIdentityTest::RunTest(const FString&)
{
	const FGuid CorrelationId = FGuid::NewGuid();
	const FGuid RunId = FGuid::NewGuid();
	const FGuid ItemId = FGuid::NewGuid();
	const FGuid First =
		Fdemo_mapShanmenThrownWeaponInputAdapter::MakeSelectionId(
			CorrelationId, RunId, ItemId, 2, 17, 4);
	const FGuid Replay =
		Fdemo_mapShanmenThrownWeaponInputAdapter::MakeSelectionId(
			CorrelationId, RunId, ItemId, 2, 17, 4);
	const FGuid Next =
		Fdemo_mapShanmenThrownWeaponInputAdapter::MakeSelectionId(
			CorrelationId, RunId, ItemId, 2, 17, 5);
	TestTrue(TEXT("Canonical input evidence reproduces one stable SelectionId"),
		First.IsValid() && First == Replay && First != Next);
	TestFalse(TEXT("Invalid event identity fails closed"),
		Fdemo_mapShanmenThrownWeaponInputAdapter::MakeSelectionId(
			CorrelationId, RunId, ItemId, 0, 17, 4).IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponInputRoutingTest,
	"Shanmen.0_0_10.Product.ThrownWeaponInputAdapter.TypedRouteAndPassThrough",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponInputRoutingTest::RunTest(const FString&)
{
	FThrownInputFixture Fixture;
	if (!Fixture.Start(*this, TEXT("Routing")))
	{
		return false;
	}
	FShanmenItemAuthoritySnapshot Before;
	Fixture.Authority->TryCaptureSnapshot(Before);
	int32 AimSamples = 0;
	const Fdemo_mapShanmenThrownWeaponInputResult Pill = Fixture.Route(
		1, Fixture.Source, AimSamples, FVector::ForwardVector);
	const Fdemo_mapShanmenThrownWeaponInputResult Empty = Fixture.Route(
		3, Fixture.Source, AimSamples, FVector::ForwardVector);
	FShanmenItemAuthoritySnapshot AfterPassThrough;
	Fixture.Authority->TryCaptureSnapshot(AfterPassThrough);
	TestTrue(TEXT("Ordinary and empty slots preserve the existing path without aim sampling"),
		Pill.ShouldPassThrough()
		&& Pill.ItemInstanceId == Fixture.HealingPillId
		&& Empty.ShouldPassThrough()
		&& AimSamples == 0
		&& Fixture.Adapter.GetNextSelectionOrdinal() == 1
		&& AfterPassThrough == Before);

	const FGuid ExpectedSelectionId =
		Fdemo_mapShanmenThrownWeaponInputAdapter::MakeSelectionId(
			Fixture.Correlation.CorrelationId,
			Fixture.Correlation.ActiveRunId,
			Fixture.ThrowingKnifeId,
			2,
			Before.AuthorityRevision,
			1);
	const Fdemo_mapShanmenThrownWeaponInputResult Thrown = Fixture.Route(
		2, Fixture.Source, AimSamples, FVector::ForwardVector);
	FShanmenItemAuthoritySnapshot AfterThrown;
	Fixture.Authority->TryCaptureSnapshot(AfterThrown);
	TestTrue(TEXT("Typed thrown slot alone samples aim and enters the product lifecycle"),
		Thrown.IsAccepted()
		&& Thrown.bAimSampled
		&& Thrown.SelectionOrdinal == 1
		&& Thrown.SelectionId == ExpectedSelectionId
		&& Thrown.ItemInstanceId == Fixture.ThrowingKnifeId
		&& AimSamples == 1
		&& Fixture.Adapter.GetNextSelectionOrdinal() == 2
		&& AfterThrown.AuthorityRevision == Before.AuthorityRevision + 2);
	TestTrue(TEXT("Input sequence resets only at the explicit lifecycle boundary"),
		Fixture.Lifecycle.TryInterruptFlight());
	Fixture.Adapter.Reset();
	TestEqual(TEXT("Explicit reset returns the transient ordinal to one"),
		Fixture.Adapter.GetNextSelectionOrdinal(), uint64(1));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponInputFailClosedTest,
	"Shanmen.0_0_10.Product.ThrownWeaponInputAdapter.FailClosedBeforeSampling",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponInputFailClosedTest::RunTest(const FString&)
{
	FThrownInputFixture Fixture;
	if (!Fixture.Start(*this, TEXT("FailClosed")))
	{
		return false;
	}
	FShanmenItemAuthoritySnapshot Before;
	Fixture.Authority->TryCaptureSnapshot(Before);
	int32 AimSamples = 0;
	const Fdemo_mapShanmenThrownWeaponInputResult InvalidSlot = Fixture.Route(
		0, Fixture.Source, AimSamples, FVector::ForwardVector);
	const Fdemo_mapShanmenThrownWeaponInputResult ForeignSource = Fixture.Route(
		2, Fixture.OtherSource, AimSamples, FVector::ForwardVector);
	Fdemo_mapShanmenThrownWeaponProductLifecycle EmptyLifecycle;
	Fdemo_mapShanmenThrownWeaponInputAdapter EmptyAdapter;
	const Fdemo_mapShanmenThrownWeaponInputResult Unbound =
		EmptyAdapter.RouteHotbarInput(
			Fixture.Authority,
			EmptyLifecycle,
			Fixture.Coordinator,
			Fixture.World,
			Ademo_mapShanmenThrownWeaponProjectile::StaticClass(),
			Fixture.Source,
			2,
			[&AimSamples]()
			{
				++AimSamples;
				return FVector::ForwardVector;
			},
			[&Fixture]()
			{
				return Fdemo_mapShanmenPlayerActionGateResult::
					FromArbitration(
						Fixture.Coordinator.TryAuthorizePlayerAction(
							Edemo_mapShanmenPlayerActionKind::ThrownWeapon,
							Fdemo_mapShanmenPlayerActionOccupancySnapshot()));
			});
	TestTrue(TEXT("Invalid, foreign, and unbound requests reject before aim sampling"),
		InvalidSlot.Status
			== Edemo_mapShanmenThrownWeaponInputStatus::InvalidSlot
		&& ForeignSource.Status
			== Edemo_mapShanmenThrownWeaponInputStatus::SourceUnavailable
		&& Unbound.Status
			== Edemo_mapShanmenThrownWeaponInputStatus::ProductRunMismatch
		&& AimSamples == 0);

	const Fdemo_mapShanmenThrownWeaponInputResult InvalidAim = Fixture.Route(
		2, Fixture.Source, AimSamples, FVector::ZeroVector);
	FShanmenItemAuthoritySnapshot After;
	Fixture.Authority->TryCaptureSnapshot(After);
	TestTrue(TEXT("Invalid sampled aim consumes neither identity nor authority"),
		InvalidAim.Status
			== Edemo_mapShanmenThrownWeaponInputStatus::AimUnavailable
		&& InvalidAim.bAimSampled
		&& AimSamples == 1
		&& Fixture.Adapter.GetNextSelectionOrdinal() == 1
		&& After == Before);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponInputActionConflictTest,
	"Shanmen.0_0_10.Product.ThrownWeaponInputAdapter.ActionConflict",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponInputActionConflictTest::RunTest(const FString&)
{
	FThrownInputFixture Fixture;
	if (!Fixture.Start(*this, TEXT("ActionConflict")))
	{
		return false;
	}
	FShanmenItemAuthoritySnapshot Before;
	Fixture.Authority->TryCaptureSnapshot(Before);
	int32 AimSamples = 0;
	int32 AuthorizationCount = 0;
	const Fdemo_mapShanmenThrownWeaponInputResult Rejected =
		Fixture.Adapter.RouteHotbarInput(
			Fixture.Authority,
			Fixture.Lifecycle,
			Fixture.Coordinator,
			Fixture.World,
			Ademo_mapShanmenThrownWeaponProjectile::StaticClass(),
			Fixture.Source,
			2,
			[&AimSamples]()
			{
				++AimSamples;
				return FVector::ForwardVector;
			},
			[&Fixture, &AuthorizationCount]()
			{
				++AuthorizationCount;
				Fdemo_mapShanmenPlayerActionOccupancySnapshot Occupancy;
				Occupancy.TryRegisterClaim(
					Edemo_mapShanmenPlayerActionKind::SpiritEvasion,
					FGuid(0xA1161001, 0, 0, 1),
					Edemo_mapShanmenPlayerActionClaimPreemption::None);
				return Fdemo_mapShanmenPlayerActionGateResult::
					FromArbitration(
						Fixture.Coordinator.TryAuthorizePlayerAction(
							Edemo_mapShanmenPlayerActionKind::ThrownWeapon,
							Occupancy));
			});
	FShanmenItemAuthoritySnapshot After;
	Fixture.Authority->TryCaptureSnapshot(After);
	TestTrue(TEXT("typed conflict is handled with deterministic gate evidence"),
		Rejected.Status
			== Edemo_mapShanmenThrownWeaponInputStatus::ActionConflict
			&& Rejected.ActionGate.IsValid()
			&& !Rejected.ActionGate.IsAuthorized()
			&& Rejected.SelectionId.IsValid());
	TestTrue(TEXT("classification samples and authorizes exactly once"),
		AimSamples == 1 && AuthorizationCount == 1);
	TestTrue(TEXT("conflict mutates neither selection nor item authority"),
		Fixture.Adapter.GetNextSelectionOrdinal() == 1
			&& Fixture.Lifecycle.GetHostState()
				== Edemo_mapShanmenThrownWeaponHostState::Empty
			&& After == Before);
	return true;
}

#endif
