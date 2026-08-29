#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenThrownWeaponProductController.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "ShanmenCombatTags.h"
#include "ShanmenItemAuthorityService.h"
#include "ShanmenItemTags.h"
#include "demo_mapPersistentProfileTypes.h"
#include "demo_mapPlayerHealthComponent.h"
#include "demo_mapShanmenItemAuthoritySubsystem.h"

namespace
{
	const FGuid ProductOwnerId(0xD3750001, 0, 0, 1);
	const FGuid ProductScopeId(0xD3750002, 0, 0, 1);
	const FGuid ProductContainerId(0xD3750003, 0, 0, 1);
	const FGuid ProductItemId(0xD3750004, 0, 0, 1);
	const FGuid ProductOtherItemId(0xD3750005, 0, 0, 1);
	const FGuid ProductMigrationId(0xD3750006, 0, 0, 1);
	const FGuid ProductReserveRequestId(0xD3750007, 0, 0, 1);
	const FGuid ProductStartRequestId(0xD3750008, 0, 0, 1);
	const FGuid ProductSelectionOneId(0xD3750010, 0, 0, 1);
	const FGuid ProductSelectionTwoId(0xD3750011, 0, 0, 1);
	const FName ProductItemDefinitionId(TEXT("Item.Test.ThrownDart.P7.5"));

	FString NewProductRoot(const TCHAR* Label)
	{
		return FPaths::Combine(
			FPaths::ProjectSavedDir(),
			TEXT("Automation"),
			TEXT("Dev.D.UE.0.0.10.P7.5.r0"),
			Label,
			FGuid::NewGuid().ToString(EGuidFormats::Digits));
	}

	FShanmenContentStamp ProductContent()
	{
		FShanmenContentStamp Content;
		Content.Version = TEXT("Shanmen.0.0.10.P7.5");
		Content.Digest = TEXT("P7.5.ThrownWeaponProductController.v1");
		return Content;
	}

	FShanmenOperationContext ProductContext(const FGuid& RequestId)
	{
		FShanmenOperationContext Context;
		Context.RunId = ProductScopeId;
		Context.OwnerId = ProductOwnerId;
		Context.RequestId = RequestId;
		Context.Content = ProductContent();
		return Context;
	}

	FShanmenItemAuthoritySnapshot ProductCandidate()
	{
		FShanmenItemAuthoritySnapshot Snapshot;
		Snapshot.Content = ProductContent();

		FShanmenItemDefinition Definition;
		Definition.DefinitionId = ProductItemDefinitionId;
		Definition.MaxStack = 16;
		Definition.ItemTags.AddTag(
			FShanmenItemNativeTags::CapabilityConsumeQuantity());
		Definition.ItemTags.AddTag(
			FShanmenItemNativeTags::ItemWeaponThrown());
		Snapshot.Definitions.Add(Definition);

		FShanmenItemContainer Container;
		Container.ContainerId = ProductContainerId;
		Container.RunId = ProductScopeId;
		Container.OwnerId = ProductOwnerId;
		Container.ContainerType = TEXT("Container.Test.P7.5.RunInventory");
		Container.Slots.Add(ProductItemId);
		Snapshot.Containers.Add(Container);

		FShanmenItemInstance Item;
		Item.ItemInstanceId = ProductItemId;
		Item.DefinitionId = ProductItemDefinitionId;
		Item.RunId = ProductScopeId;
		Item.OwnerId = ProductOwnerId;
		Item.ParentContainerId = ProductContainerId;
		Item.SlotIndex = 0;
		Item.Quantity = 4;
		Snapshot.Items.Add(Item);
		return Snapshot;
	}

	FShanmenItemMigrationEvidence ProductEvidence()
	{
		FShanmenItemMigrationEvidence Evidence;
		Evidence.MigrationId = ProductMigrationId;
		Evidence.OwnerId = ProductOwnerId;
		Evidence.SourceProfileSchema = 7;
		Evidence.SourceSaveGeneration = 75;
		Evidence.SourceCodeBPersistentRevision = 7;
		Evidence.SourceCodeBRepositoryRevision = 5;
		Evidence.DefinitionCount = 1;
		Evidence.ContainerCount = 1;
		Evidence.ItemCount = 1;
		Evidence.SourceFingerprint = TEXT("P7.5.Product.SourceFixture.v1");
		Evidence.CandidateDigest = TEXT("P7.5.Product.CandidateFixture.v1");
		return Evidence;
	}

	FShanmenThrownWeaponDefinitionCapture ProductDefinitionCapture()
	{
		FShanmenThrownWeaponDefinitionCapture Capture;
		Capture.ActionDefinitionId =
			FShanmenThrownWeaponDefinition::CanonicalActionDefinitionId();
		Capture.DetectorId = TEXT("Detector.ThrownWeapon.P7.5.Product");
		Capture.FormulaId = TEXT("Formula.ThrownWeapon.P7.5.Product");
		Capture.BaseDamage = 9.0f;
		Capture.TechniquePowerCoefficient = 0.5f;
		Capture.LaunchSpeed = 900.0f;
		Capture.DamageTags.AddTag(
			FShanmenCombatNativeTags::DamagePhysicalSlash());
		Capture.RequiredTargetTags.AddTag(
			FShanmenCombatNativeTags::TargetLiving());
		return Capture;
	}

	Fdemo_mapShanmenThrownWeaponProductCapture MakeProductCapture()
	{
		Fdemo_mapShanmenThrownWeaponProductCapture Product;
		check(Fdemo_mapShanmenThrownWeaponProductCapture::TryCapture(
			ProductDefinitionCapture(),
			30.0f,
			FGameplayTagContainer(),
			Product));
		return Product;
	}

	struct FProductFixture
	{
		FString Root;
		UGameInstance* GameInstance = nullptr;
		Udemo_mapShanmenItemAuthoritySubsystem* Authority = nullptr;
		UWorld* World = nullptr;
		APawn* Source = nullptr;
		Udemo_mapPlayerHealthComponent* Health = nullptr;
		Fdemo_mapCombatRunCoordinator Coordinator;
		Fdemo_mapShanmenRunCorrelation Correlation;

		bool Start(FAutomationTestBase& Test, const TCHAR* Label)
		{
			Root = NewProductRoot(Label);
			const FShanmenItemStorageContext Storage =
				FShanmenItemStorageContext::ForRoot(Root, ProductOwnerId);
			{
				FShanmenItemAuthorityService Bootstrap;
				const FShanmenItemAuthorityStartResult StartedAuthority =
					Bootstrap.StartFromAuthorizedMigration(
						Storage,
						FShanmenItemMigrationAuthorization::Explicit(
							ProductMigrationId),
						ProductCandidate(),
						ProductEvidence());
				FShanmenItemReserveRequest Reserve;
				Reserve.Context = ProductContext(ProductReserveRequestId);
				Reserve.ItemInstanceId = ProductItemId;
				Reserve.ResourceKind = EShanmenItemResourceKind::Quantity;
				Reserve.Amount = 4;
				Reserve.ExpectedItemRevision = 0;
				Reserve.PurposeId = TEXT("Prepare.P7.5.ThrownWeapon");
				const FShanmenItemDurableCommandResult Reserved =
					StartedAuthority.IsReady()
					? Bootstrap.ReserveDurable(Reserve)
					: FShanmenItemDurableCommandResult();
				FShanmenItemRunStartRequest StartRun;
				StartRun.Context = ProductContext(ProductStartRequestId);
				if (Reserved.IsCommandSuccess())
				{
					StartRun.ReservationIds.Add(
						Reserved.Receipt.ReservationId);
				}
				const FShanmenItemDurableCommandResult RunStarted =
					StartRun.ReservationIds.IsEmpty()
					? FShanmenItemDurableCommandResult()
					: Bootstrap.StartPreparedRunDurable(StartRun);
				if (!RunStarted.IsCommandSuccess())
				{
					Test.AddError(TEXT("Could not publish the P7.5 active Run."));
					return false;
				}

				Correlation.CorrelationId = FGuid(0xD3750020, 0, 0, 1);
				Correlation.OwnerId = ProductOwnerId;
				Correlation.ScopeId = ProductScopeId;
				Correlation.ActiveRunId = RunStarted.Receipt.ReservationId;
				Correlation.PreparedRequestId = Reserve.Context.RequestId;
				Correlation.PreparedReceiptId = Reserved.Receipt.ReceiptId;
				Correlation.LifecycleRequestId = StartRun.Context.RequestId;
				Correlation.LifecycleReceiptId = RunStarted.Receipt.ReceiptId;
				Correlation.PreparedAuthorityRevision =
					Reserved.Receipt.AuthorityRevision;
				Correlation.LifecycleAuthorityRevision =
					RunStarted.Receipt.AuthorityRevision;
				Correlation.OrderedPreparedItemInstanceIds.Add(ProductItemId);
				Correlation.OrderedRunInventoryItemInstanceIds.Add(
					ProductItemId);
				Correlation.HotbarItemInstanceIds.SetNum(
					Fdemo_mapPersistentPreparationLayout::HotbarSlotCount);
				Correlation.HotbarItemInstanceIds[0] = ProductItemId;
				if (!Correlation.IsValid())
				{
					Test.AddError(TEXT("P7.5 Run correlation is invalid."));
					return false;
				}
			}

			if (!GEngine)
			{
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
			const Fdemo_mapShanmenItemAuthorityBindResult Bound = Authority
				? Authority->BindExisting(
					Fdemo_mapProfileStorageContext::ForRoot(Root),
					ProductOwnerId)
				: Fdemo_mapShanmenItemAuthorityBindResult();
			if (!Authority || !Bound.IsReady())
			{
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
			Health = Source
				? NewObject<Udemo_mapPlayerHealthComponent>(
					Source, TEXT("P75PlayerHealth"))
				: nullptr;
			FString Diagnostic;
			if (!Source || !Health
				|| !Coordinator.TryBeginRun(
					Correlation.ActiveRunId,
					Source,
					Health,
					Diagnostic))
			{
				Test.AddError(FString::Printf(
					TEXT("Could not bind P7.5 combat Run: %s"),
					*Diagnostic));
				return false;
			}
			return true;
		}

		Fdemo_mapShanmenThrownWeaponSelectionIntent MakeSelection(
			const FGuid& SelectionId,
			const FVector& Direction) const
		{
			Fdemo_mapShanmenThrownWeaponSelectionIntent Selection;
			check(Fdemo_mapShanmenThrownWeaponSelectionIntent::TryCapture(
				SelectionId,
				Correlation.ActiveRunId,
				ProductItemId,
				FVector(20.0, 30.0, 60.0),
				Direction,
				1500.0f,
				Selection));
			return Selection;
		}

		FShanmenCombatActionSnapshot MakeAction(uint64 Sequence) const
		{
			FShanmenCombatActionCapture Capture;
			Capture.RunId = Correlation.ActiveRunId;
			Capture.OwnerId = ProductOwnerId;
			Capture.SourceEntityId = Coordinator.GetPlayerEntityId();
			Capture.SourceItemInstanceId = ProductItemId;
			Capture.ActionDefinitionId =
				FShanmenThrownWeaponDefinition::CanonicalActionDefinitionId();
			Capture.Content = ProductContent();
			Capture.SourceTags.AddTag(
				FShanmenCombatNativeTags::SourcePlayer());
			Capture.SourceTags.AddTag(
				FShanmenItemNativeTags::CapabilityConsumeQuantity());
			Capture.SourceTags.AddTag(
				FShanmenItemNativeTags::ItemWeaponThrown());
			Capture.ActivationId = FShanmenCombatIdFactory::MakeActivationId(
				Capture.RunId,
				Capture.SourceEntityId,
				Capture.ActionDefinitionId,
				Sequence);
			FShanmenCombatActionSnapshot Action;
			check(FShanmenCombatActionSnapshot::TryCapture(Capture, Action));
			return Action;
		}

		Fdemo_mapShanmenThrownWeaponRunCommandIntent MakeCommand(
			uint64 Sequence,
			const FVector& Direction) const
		{
			const Fdemo_mapShanmenThrownWeaponProductCapture Product =
				MakeProductCapture();
			Fdemo_mapShanmenThrownWeaponRunCommandIntent Command;
			check(Fdemo_mapShanmenThrownWeaponRunCommandIntent::TryCapture(
				Correlation,
				MakeAction(Sequence),
				Product.GetDefinition(),
				Product.GetOffense(),
				FVector(20.0, 30.0, 60.0),
				Direction,
				1500.0f,
				Command));
			return Command;
		}

		void Stop()
		{
			if (Coordinator.IsActive())
			{
				FString Diagnostic;
				Coordinator.TryEndRun(Correlation.ActiveRunId, Diagnostic);
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
				Health = nullptr;
			}
			if (GameInstance)
			{
				GameInstance->Shutdown();
				Authority = nullptr;
				GameInstance->RemoveFromRoot();
				GameInstance->MarkAsGarbage();
				GameInstance = nullptr;
			}
			if (!Root.IsEmpty())
			{
				IFileManager::Get().DeleteDirectory(*Root, false, true);
				Root.Reset();
			}
		}

		~FProductFixture()
		{
			Stop();
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponProductCaptureTest,
	"Shanmen.0_0_10.Product.ThrownWeaponProductController.CaptureContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponProductCaptureTest::RunTest(const FString&)
{
	const FGuid RunId(0xD3750100, 0, 0, 1);
	Fdemo_mapShanmenThrownWeaponSelectionIntent Forward;
	Fdemo_mapShanmenThrownWeaponSelectionIntent Right;
	TestTrue(TEXT("Selection freezes exact item and canonical direction"),
		Fdemo_mapShanmenThrownWeaponSelectionIntent::TryCapture(
			ProductSelectionOneId,
			RunId,
			ProductItemId,
			FVector::ZeroVector,
			FVector(5.0, 0.0, 0.0),
			1000.0f,
			Forward)
			&& Forward.IsValid()
			&& Forward.GetAimDirection() == FVector::ForwardVector);
	TestTrue(TEXT("Same SelectionId with another trajectory is a conflict"),
		Fdemo_mapShanmenThrownWeaponSelectionIntent::TryCapture(
			ProductSelectionOneId,
			RunId,
			ProductItemId,
			FVector::ZeroVector,
			FVector::RightVector,
			1000.0f,
			Right)
			&& !Forward.Matches(Right));
	Fdemo_mapShanmenThrownWeaponSelectionIntent InvalidSelection;
	TestFalse(TEXT("Selection rejects non-positive range"),
		Fdemo_mapShanmenThrownWeaponSelectionIntent::TryCapture(
			ProductSelectionTwoId,
			RunId,
			ProductItemId,
			FVector::ZeroVector,
			FVector::ForwardVector,
			0.0f,
			InvalidSelection));

	Fdemo_mapShanmenThrownWeaponProductCapture Product;
	TestTrue(TEXT("Definition and character offense freeze outside input"),
		Fdemo_mapShanmenThrownWeaponProductCapture::TryCapture(
			ProductDefinitionCapture(),
			30.0f,
			FGameplayTagContainer(),
			Product)
			&& Product.IsValid()
			&& Product.GetOffense().GetTechniquePower() == 30.0f);
	FShanmenThrownWeaponDefinitionCapture InvalidDefinition =
		ProductDefinitionCapture();
	InvalidDefinition.LaunchSpeed = 0.0f;
	Fdemo_mapShanmenThrownWeaponProductCapture InvalidProduct;
	TestFalse(TEXT("Invalid product values fail before action reservation"),
		Fdemo_mapShanmenThrownWeaponProductCapture::TryCapture(
			InvalidDefinition,
			30.0f,
			FGameplayTagContainer(),
			InvalidProduct));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponProductSubmitTest,
	"Shanmen.0_0_10.Product.ThrownWeaponProductController.SubmitReplayConflict",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponProductSubmitTest::RunTest(const FString&)
{
	FProductFixture Fixture;
	Fdemo_mapShanmenThrownWeaponRunHost Host;
	if (!Fixture.Start(*this, TEXT("Submit")))
	{
		return false;
	}
	Fdemo_mapShanmenThrownWeaponProductController Controller;
	Fdemo_mapShanmenThrownWeaponRunCommandRouter Router;
	const Fdemo_mapShanmenThrownWeaponProductCapture Product =
		MakeProductCapture();
	const Fdemo_mapShanmenThrownWeaponSelectionIntent Selection =
		Fixture.MakeSelection(ProductSelectionOneId, FVector::ForwardVector);
	FShanmenItemAuthoritySnapshot Before;
	Fixture.Authority->TryCaptureSnapshot(Before);
	const Fdemo_mapShanmenThrownWeaponProductResult Applied =
		Controller.TrySubmit(
			Host,
			Router,
			Fixture.World,
			Ademo_mapShanmenThrownWeaponProjectile::StaticClass(),
			*Fixture.Authority,
			Fixture.Coordinator,
			Fixture.Source,
			Fixture.Correlation,
			Selection,
			Product);
	FShanmenItemAuthoritySnapshot After;
	Fixture.Authority->TryCaptureSnapshot(After);
	const Fdemo_mapShanmenThrownWeaponRunCommandIntent* Captured =
		Controller.FindCapturedCommand(ProductSelectionOneId);
	TestTrue(TEXT("Product owner captures and launches sequence one exactly once"),
		Applied.IsAccepted()
			&& !Applied.bReusedSelection
			&& Applied.ActivationSequence == 1
			&& Applied.ActivationId == FShanmenCombatIdFactory::MakeActivationId(
				Fixture.Correlation.ActiveRunId,
				Fixture.Coordinator.GetPlayerEntityId(),
				FShanmenThrownWeaponDefinition::CanonicalActionDefinitionId(),
				1)
			&& Captured
			&& Captured->GetAction().GetSourceTags().HasTagExact(
				FShanmenItemNativeTags::ItemWeaponThrown())
			&& Captured->GetAction().GetSourceTags().HasTagExact(
				FShanmenCombatNativeTags::SourcePlayer())
			&& Fixture.Coordinator
				.GetNextPlayerThrownWeaponActivationSequence() == 2
			&& After.AuthorityRevision == Before.AuthorityRevision + 2
			&& Controller.IsValid());

	Ademo_mapShanmenThrownWeaponProjectile* FirstProjectile =
		Host.GetProjectile();
	const Fdemo_mapShanmenThrownWeaponProductResult Replay =
		Controller.TrySubmit(
			Host,
			Router,
			Fixture.World,
			Ademo_mapShanmenThrownWeaponProjectile::StaticClass(),
			*Fixture.Authority,
			Fixture.Coordinator,
			Fixture.Source,
			Fixture.Correlation,
			Selection,
			Product);
	FShanmenItemAuthoritySnapshot AfterReplay;
	Fixture.Authority->TryCaptureSnapshot(AfterReplay);
	TestTrue(TEXT("Exact selection replay reuses action without I/O or Actor spawn"),
		Replay.IsAccepted()
			&& Replay.bReusedSelection
			&& Replay.Command.IsReplay()
			&& Replay.ActivationSequence == 1
			&& Host.GetProjectile() == FirstProjectile
			&& AfterReplay == After
			&& Fixture.Coordinator
				.GetNextPlayerThrownWeaponActivationSequence() == 2);

	const Fdemo_mapShanmenThrownWeaponSelectionIntent Conflict =
		Fixture.MakeSelection(ProductSelectionOneId, FVector::RightVector);
	const Fdemo_mapShanmenThrownWeaponProductResult Rejected =
		Controller.TrySubmit(
			Host,
			Router,
			Fixture.World,
			Ademo_mapShanmenThrownWeaponProjectile::StaticClass(),
			*Fixture.Authority,
			Fixture.Coordinator,
			Fixture.Source,
			Fixture.Correlation,
			Conflict,
			Product);
	TestTrue(TEXT("Selection conflict consumes no action sequence"),
		Rejected.Status
			== Edemo_mapShanmenThrownWeaponProductStatus::SelectionIdConflict
			&& Fixture.Coordinator
				.GetNextPlayerThrownWeaponActivationSequence() == 2
			&& Controller.NumCapturedSelections() == 1
			&& Router.NumProcessedIntents() == 1);

	Fdemo_mapShanmenThrownWeaponSelectionIntent UnpreparedSelection;
	check(Fdemo_mapShanmenThrownWeaponSelectionIntent::TryCapture(
		ProductSelectionTwoId,
		Fixture.Correlation.ActiveRunId,
		ProductOtherItemId,
		FVector::ZeroVector,
		FVector::ForwardVector,
		1000.0f,
		UnpreparedSelection));
	const Fdemo_mapShanmenThrownWeaponProductResult Unprepared =
		Controller.TrySubmit(
			Host,
			Router,
			Fixture.World,
			Ademo_mapShanmenThrownWeaponProjectile::StaticClass(),
			*Fixture.Authority,
			Fixture.Coordinator,
			Fixture.Source,
			Fixture.Correlation,
			UnpreparedSelection,
			Product);
	TestTrue(TEXT("Unprepared exact item fails before sequence reservation"),
		Unprepared.Status
			== Edemo_mapShanmenThrownWeaponProductStatus::RunMismatch
			&& Fixture.Coordinator
				.GetNextPlayerThrownWeaponActivationSequence() == 2
			&& Controller.NumCapturedSelections() == 1);
	Host.TryInterrupt();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponProductRetryTest,
	"Shanmen.0_0_10.Product.ThrownWeaponProductController.TransientRetrySequence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponProductRetryTest::RunTest(const FString&)
{
	FProductFixture Fixture;
	Fdemo_mapShanmenThrownWeaponRunHost Host;
	if (!Fixture.Start(*this, TEXT("Retry")))
	{
		return false;
	}
	Fdemo_mapShanmenThrownWeaponProductController Controller;
	Fdemo_mapShanmenThrownWeaponRunCommandRouter Router;
	const Fdemo_mapShanmenThrownWeaponProductCapture Product =
		MakeProductCapture();
	const Fdemo_mapShanmenThrownWeaponProductResult First =
		Controller.TrySubmit(
			Host,
			Router,
			Fixture.World,
			Ademo_mapShanmenThrownWeaponProjectile::StaticClass(),
			*Fixture.Authority,
			Fixture.Coordinator,
			Fixture.Source,
			Fixture.Correlation,
			Fixture.MakeSelection(
				ProductSelectionOneId, FVector::ForwardVector),
			Product);
	const Fdemo_mapShanmenThrownWeaponSelectionIntent SecondSelection =
		Fixture.MakeSelection(ProductSelectionTwoId, FVector::RightVector);
	FShanmenItemAuthoritySnapshot BeforeBusy;
	Fixture.Authority->TryCaptureSnapshot(BeforeBusy);
	const Fdemo_mapShanmenThrownWeaponProductResult Busy =
		Controller.TrySubmit(
			Host,
			Router,
			Fixture.World,
			Ademo_mapShanmenThrownWeaponProjectile::StaticClass(),
			*Fixture.Authority,
			Fixture.Coordinator,
			Fixture.Source,
			Fixture.Correlation,
			SecondSelection,
			Product);
	FShanmenItemAuthoritySnapshot AfterBusy;
	Fixture.Authority->TryCaptureSnapshot(AfterBusy);
	TestTrue(TEXT("Host busy preserves sequence two for the same selection"),
		First.IsAccepted()
			&& Busy.Status
				== Edemo_mapShanmenThrownWeaponProductStatus::RouterRejected
			&& Busy.Command.Status
				== Edemo_mapShanmenThrownWeaponRunCommandStatus::HostBusy
			&& !Busy.bReusedSelection
			&& Busy.ActivationSequence == 2
			&& Fixture.Coordinator
				.GetNextPlayerThrownWeaponActivationSequence() == 3
			&& AfterBusy == BeforeBusy
			&& Controller.NumCapturedSelections() == 2);

	if (!Host.TryInterrupt() || !Host.Reset())
	{
		AddError(TEXT("Could not release P7.5 busy Host."));
		return false;
	}
	const Fdemo_mapShanmenThrownWeaponProductResult Retried =
		Controller.TrySubmit(
			Host,
			Router,
			Fixture.World,
			Ademo_mapShanmenThrownWeaponProjectile::StaticClass(),
			*Fixture.Authority,
			Fixture.Coordinator,
			Fixture.Source,
			Fixture.Correlation,
			SecondSelection,
			Product);
	TestTrue(TEXT("Retry uses captured sequence two and does not reserve three"),
		Retried.IsAccepted()
			&& Retried.bReusedSelection
			&& Retried.ActivationSequence == 2
			&& Fixture.Coordinator
				.GetNextPlayerThrownWeaponActivationSequence() == 3
			&& Router.NumProcessedIntents() == 2
			&& Controller.IsValid());
	Host.TryInterrupt();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponProductRecoveryTest,
	"Shanmen.0_0_10.Product.ThrownWeaponProductController.SelectionRecovery",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponProductRecoveryTest::RunTest(const FString&)
{
	FProductFixture Fixture;
	Fdemo_mapShanmenThrownWeaponRunHost Host;
	if (!Fixture.Start(*this, TEXT("Recovery")))
	{
		return false;
	}
	const Fdemo_mapShanmenThrownWeaponProductCapture Product =
		MakeProductCapture();
	Fdemo_mapShanmenThrownWeaponRunCommandRouter OccupancyRouter;
	const Fdemo_mapShanmenThrownWeaponRunCommandResult Occupied =
		OccupancyRouter.TryRoute(
			Host,
			Fixture.World,
			Ademo_mapShanmenThrownWeaponProjectile::StaticClass(),
			*Fixture.Authority,
			Fixture.Coordinator,
			Fixture.Source,
			Fixture.MakeCommand(99, FVector::ForwardVector));
	if (!Occupied.IsAccepted())
	{
		AddError(TEXT("Could not occupy P7.5 recovery Host."));
		return false;
	}

	Fdemo_mapShanmenThrownWeaponProductController Controller;
	Fdemo_mapShanmenThrownWeaponRunCommandRouter Router;
	const Fdemo_mapShanmenThrownWeaponSelectionIntent Selection =
		Fixture.MakeSelection(ProductSelectionOneId, FVector::RightVector);
	const Fdemo_mapShanmenThrownWeaponProductResult CapturedBusy =
		Controller.TrySubmit(
			Host,
			Router,
			Fixture.World,
			Ademo_mapShanmenThrownWeaponProjectile::StaticClass(),
			*Fixture.Authority,
			Fixture.Coordinator,
			Fixture.Source,
			Fixture.Correlation,
			Selection,
			Product);
	const Fdemo_mapShanmenThrownWeaponRunCommandIntent* Captured =
		Controller.FindCapturedCommand(ProductSelectionOneId);
	if (CapturedBusy.Command.Status
			!= Edemo_mapShanmenThrownWeaponRunCommandStatus::HostBusy
		|| !Captured
		|| !Host.TryInterrupt()
		|| !Host.Reset())
	{
		AddError(TEXT("Could not freeze P7.5 recovery selection."));
		return false;
	}

	const Fdemo_mapShanmenThrownWeaponItemResult Preprepared =
		Fdemo_mapShanmenThrownWeaponItemAdapter::PrepareActiveRun(
			*Fixture.Authority,
			Fixture.Correlation,
			Captured->GetAction());
	if (!Preprepared.IsPrepared())
	{
		AddError(TEXT("Could not preprepare P7.5 cancellation recovery."));
		return false;
	}
	Fixture.Authority->SetInjectedFailureForAutomation(
		EShanmenItemStoreFailureStage::WriteTemp);
	const Fdemo_mapShanmenThrownWeaponProductResult Interrupted =
		Controller.TrySubmit(
			Host,
			Router,
			Fixture.World,
			nullptr,
			*Fixture.Authority,
			Fixture.Coordinator,
			Fixture.Source,
			Fixture.Correlation,
			Selection,
			Product);
	TestTrue(TEXT("Cancellation failure remains attached to selection sequence one"),
		Interrupted.Command.RequiresRecovery()
			&& Interrupted.bReusedSelection
			&& Interrupted.ActivationSequence == 1
			&& Fixture.Coordinator
				.GetNextPlayerThrownWeaponActivationSequence() == 2
			&& Host.GetState()
				== Edemo_mapShanmenThrownWeaponHostState::Empty);

	Fixture.Authority->SetInjectedFailureForAutomation(
		EShanmenItemStoreFailureStage::None);
	const Fdemo_mapShanmenThrownWeaponProductResult Recovered =
		Controller.TryRecoverCancellation(
			*Fixture.Authority, Router, Selection);
	TestTrue(TEXT("Selection-only recovery retries cancellation and never launch"),
		Recovered.IsRecoveryApplied()
			&& Recovered.bReusedSelection
			&& Recovered.ActivationSequence == 1
			&& Host.GetState()
				== Edemo_mapShanmenThrownWeaponHostState::Empty
			&& Router.IsValid()
			&& Controller.IsValid());
	return true;
}

#endif
