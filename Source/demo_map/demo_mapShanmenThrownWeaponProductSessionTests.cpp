#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenThrownWeaponProductSession.h"

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
#include "demo_mapAttributeComponent.h"
#include "demo_mapAttributeDefinitions.h"
#include "demo_mapPersistentProfileTypes.h"
#include "demo_mapPlayerHealthComponent.h"
#include "demo_mapShanmenItemAuthoritySubsystem.h"

namespace
{
	const FGuid SessionOwnerId(0xD3760001, 0, 0, 1);
	const FGuid SessionScopeId(0xD3760002, 0, 0, 1);
	const FGuid SessionContainerId(0xD3760003, 0, 0, 1);
	const FGuid SessionItemOneId(0xD3760004, 0, 0, 1);
	const FGuid SessionItemTwoId(0xD3760005, 0, 0, 1);
	const FGuid SessionMigrationId(0xD3760006, 0, 0, 1);
	const FGuid SessionReserveOneId(0xD3760007, 0, 0, 1);
	const FGuid SessionReserveTwoId(0xD3760008, 0, 0, 1);
	const FGuid SessionStartRequestId(0xD3760009, 0, 0, 1);
	const FGuid SessionSelectionOneId(0xD3760010, 0, 0, 1);
	const FGuid SessionSelectionTwoId(0xD3760011, 0, 0, 1);
	const FName SessionItemDefinitionId(
		TEXT("Item.Test.ThrownDart.P7.6"));

	FString NewSessionRoot(const TCHAR* Label)
	{
		return FPaths::Combine(
			FPaths::ProjectSavedDir(),
			TEXT("Automation"),
			TEXT("Dev.D.UE.0.0.10.P7.6.r0"),
			Label,
			FGuid::NewGuid().ToString(EGuidFormats::Digits));
	}

	FShanmenContentStamp SessionContent()
	{
		FShanmenContentStamp Content;
		Content.Version = TEXT("Shanmen.0.0.10.P7.6");
		Content.Digest = TEXT("P7.6.ThrownWeaponProductSession.v1");
		return Content;
	}

	FShanmenOperationContext SessionContext(const FGuid& RequestId)
	{
		FShanmenOperationContext Context;
		Context.RunId = SessionScopeId;
		Context.OwnerId = SessionOwnerId;
		Context.RequestId = RequestId;
		Context.Content = SessionContent();
		return Context;
	}

	FShanmenItemAuthoritySnapshot SessionCandidate()
	{
		FShanmenItemAuthoritySnapshot Snapshot;
		Snapshot.Content = SessionContent();

		FShanmenItemDefinition Definition;
		Definition.DefinitionId = SessionItemDefinitionId;
		Definition.MaxStack = 16;
		Definition.ItemTags.AddTag(
			FShanmenItemNativeTags::CapabilityConsumeQuantity());
		Definition.ItemTags.AddTag(
			FShanmenItemNativeTags::ItemWeaponThrown());
		Snapshot.Definitions.Add(Definition);

		FShanmenItemContainer Container;
		Container.ContainerId = SessionContainerId;
		Container.RunId = SessionScopeId;
		Container.OwnerId = SessionOwnerId;
		Container.ContainerType =
			TEXT("Container.Test.P7.6.RunInventory");
		Container.Slots = { SessionItemOneId, SessionItemTwoId };
		Snapshot.Containers.Add(Container);

		for (int32 Index = 0; Index < 2; ++Index)
		{
			FShanmenItemInstance Item;
			Item.ItemInstanceId = Index == 0
				? SessionItemOneId : SessionItemTwoId;
			Item.DefinitionId = SessionItemDefinitionId;
			Item.RunId = SessionScopeId;
			Item.OwnerId = SessionOwnerId;
			Item.ParentContainerId = SessionContainerId;
			Item.SlotIndex = Index;
			Item.Quantity = 3;
			Snapshot.Items.Add(Item);
		}
		return Snapshot;
	}

	FShanmenItemMigrationEvidence SessionEvidence()
	{
		FShanmenItemMigrationEvidence Evidence;
		Evidence.MigrationId = SessionMigrationId;
		Evidence.OwnerId = SessionOwnerId;
		Evidence.SourceProfileSchema = 7;
		Evidence.SourceSaveGeneration = 76;
		Evidence.SourceCodeBPersistentRevision = 7;
		Evidence.SourceCodeBRepositoryRevision = 6;
		Evidence.DefinitionCount = 1;
		Evidence.ContainerCount = 1;
		Evidence.ItemCount = 2;
		Evidence.SourceFingerprint =
			TEXT("P7.6.Session.SourceFixture.v1");
		Evidence.CandidateDigest =
			TEXT("P7.6.Session.CandidateFixture.v1");
		return Evidence;
	}

	FShanmenThrownWeaponDefinitionCapture SessionDefinitionCapture()
	{
		FShanmenThrownWeaponDefinitionCapture Capture;
		Capture.ActionDefinitionId =
			FShanmenThrownWeaponDefinition::CanonicalActionDefinitionId();
		Capture.DetectorId = TEXT("Detector.ThrownWeapon.P7.6.Session");
		Capture.FormulaId = TEXT("Formula.ThrownWeapon.P7.6.Session");
		Capture.BaseDamage = 8.0f;
		Capture.TechniquePowerCoefficient = 0.75f;
		Capture.LaunchSpeed = 960.0f;
		Capture.DamageTags.AddTag(
			FShanmenCombatNativeTags::DamagePhysicalSlash());
		Capture.RequiredTargetTags.AddTag(
			FShanmenCombatNativeTags::TargetLiving());
		return Capture;
	}

	Fdemo_mapShanmenThrownWeaponSessionConfig MakeSessionConfig()
	{
		FGameplayTagContainer Tags;
		Tags.AddTag(FShanmenCombatNativeTags::SourcePlayer());
		Fdemo_mapShanmenThrownWeaponSessionConfig Config;
		check(Fdemo_mapShanmenThrownWeaponSessionConfig::TryCapture(
			SessionDefinitionCapture(), Tags, Config));
		return Config;
	}

	struct FSessionFixture
	{
		FString Root;
		UGameInstance* GameInstance = nullptr;
		Udemo_mapShanmenItemAuthoritySubsystem* Authority = nullptr;
		UWorld* World = nullptr;
		APawn* Source = nullptr;
		Udemo_mapPlayerHealthComponent* Health = nullptr;
		Udemo_mapAttributeComponent* Attributes = nullptr;
		Fdemo_mapModifierHandle AttackHandle;
		Fdemo_mapCombatRunCoordinator Coordinator;
		Fdemo_mapShanmenRunCorrelation Correlation;
		Fdemo_mapShanmenThrownWeaponSessionConfig Config;
		Fdemo_mapShanmenThrownWeaponProductSession Session;

		bool Start(FAutomationTestBase& Test, const TCHAR* Label)
		{
			Root = NewSessionRoot(Label);
			const FShanmenItemStorageContext Storage =
				FShanmenItemStorageContext::ForRoot(Root, SessionOwnerId);
			{
				FShanmenItemAuthorityService Bootstrap;
				const FShanmenItemAuthorityStartResult Started =
					Bootstrap.StartFromAuthorizedMigration(
						Storage,
						FShanmenItemMigrationAuthorization::Explicit(
							SessionMigrationId),
						SessionCandidate(),
						SessionEvidence());
				FShanmenItemReserveRequest ReserveOne;
				ReserveOne.Context = SessionContext(SessionReserveOneId);
				ReserveOne.ItemInstanceId = SessionItemOneId;
				ReserveOne.ResourceKind = EShanmenItemResourceKind::Quantity;
				ReserveOne.Amount = 3;
				ReserveOne.ExpectedItemRevision = 0;
				ReserveOne.PurposeId = TEXT("Prepare.P7.6.ThrownWeapon.One");
				const FShanmenItemDurableCommandResult ReservedOne =
					Started.IsReady()
						? Bootstrap.ReserveDurable(ReserveOne)
						: FShanmenItemDurableCommandResult();

				FShanmenItemReserveRequest ReserveTwo;
				ReserveTwo.Context = SessionContext(SessionReserveTwoId);
				ReserveTwo.ItemInstanceId = SessionItemTwoId;
				ReserveTwo.ResourceKind = EShanmenItemResourceKind::Quantity;
				ReserveTwo.Amount = 3;
				ReserveTwo.ExpectedItemRevision = 0;
				ReserveTwo.PurposeId = TEXT("Prepare.P7.6.ThrownWeapon.Two");
				const FShanmenItemDurableCommandResult ReservedTwo =
					ReservedOne.IsCommandSuccess()
						? Bootstrap.ReserveDurable(ReserveTwo)
						: FShanmenItemDurableCommandResult();

				FShanmenItemRunStartRequest StartRun;
				StartRun.Context = SessionContext(SessionStartRequestId);
				if (ReservedOne.IsCommandSuccess()
					&& ReservedTwo.IsCommandSuccess())
				{
					StartRun.ReservationIds = {
						ReservedOne.Receipt.ReservationId,
						ReservedTwo.Receipt.ReservationId };
				}
				const FShanmenItemDurableCommandResult RunStarted =
					StartRun.ReservationIds.Num() == 2
						? Bootstrap.StartPreparedRunDurable(StartRun)
						: FShanmenItemDurableCommandResult();
				if (!RunStarted.IsCommandSuccess())
				{
					Test.AddError(TEXT("Could not publish the P7.6 active Run."));
					return false;
				}

				Correlation.CorrelationId = FGuid(0xD3760020, 0, 0, 1);
				Correlation.OwnerId = SessionOwnerId;
				Correlation.ScopeId = SessionScopeId;
				Correlation.ActiveRunId = RunStarted.Receipt.ReservationId;
				Correlation.PreparedRequestId = ReserveOne.Context.RequestId;
				Correlation.PreparedReceiptId = ReservedOne.Receipt.ReceiptId;
				Correlation.LifecycleRequestId = StartRun.Context.RequestId;
				Correlation.LifecycleReceiptId = RunStarted.Receipt.ReceiptId;
				Correlation.PreparedAuthorityRevision =
					ReservedTwo.Receipt.AuthorityRevision;
				Correlation.LifecycleAuthorityRevision =
					RunStarted.Receipt.AuthorityRevision;
				Correlation.OrderedPreparedItemInstanceIds = {
					SessionItemOneId, SessionItemTwoId };
				Correlation.OrderedRunInventoryItemInstanceIds = {
					SessionItemOneId, SessionItemTwoId };
				Correlation.HotbarItemInstanceIds.SetNum(
					Fdemo_mapPersistentPreparationLayout::HotbarSlotCount);
				Correlation.HotbarItemInstanceIds[0] = SessionItemOneId;
				Correlation.HotbarItemInstanceIds[8] = SessionItemTwoId;
				if (!Correlation.IsValid())
				{
					Test.AddError(TEXT("P7.6 Run correlation is invalid."));
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
					SessionOwnerId)
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
					Source, TEXT("P76PlayerHealth"))
				: nullptr;
			Attributes = Source
				? NewObject<Udemo_mapAttributeComponent>(
					Source, TEXT("P76PlayerAttributes"))
				: nullptr;
			if (Source && Attributes)
			{
				Source->AddInstanceComponent(Attributes);
			}
			FString Diagnostic;
			Config = MakeSessionConfig();
			if (!Source || !Health || !Attributes
				|| !Coordinator.TryBeginRun(
					Correlation.ActiveRunId,
					Source,
					Health,
					Diagnostic)
				|| !Session.TryBegin(
					Correlation, *Source, Config, Diagnostic))
			{
				Test.AddError(FString::Printf(
					TEXT("Could not bind P7.6 product session: %s"),
					*Diagnostic));
				return false;
			}
			return true;
		}

		Fdemo_mapShanmenThrownWeaponHotbarIntent MakeIntent(
			const FGuid& SelectionId,
			int32 SlotNumber,
			const FVector& Direction) const
		{
			Fdemo_mapShanmenThrownWeaponHotbarIntent Intent;
			check(Fdemo_mapShanmenThrownWeaponHotbarIntent::TryCapture(
				SelectionId,
				SlotNumber,
				FVector(25.0, 35.0, 65.0),
				Direction,
				1600.0f,
				Intent));
			return Intent;
		}

		bool SetAttackPower(float RequestedPower)
		{
			if (!Attributes || RequestedPower < 1.0f)
			{
				return false;
			}
			if (AttackHandle.IsValid()
				&& !Attributes->RemoveModifier(AttackHandle))
			{
				return false;
			}
			AttackHandle = Fdemo_mapModifierHandle();
			if (FMath::IsNearlyEqual(RequestedPower, 1.0f))
			{
				return true;
			}
			Fdemo_mapModifierSpec Modifier;
			Modifier.SourceId = TEXT("P7.6.Session.AttackPower");
			Modifier.AttributeId = Fdemo_mapAttributeIds::AttackPower;
			Modifier.Operation = Edemo_mapModifierOperation::Add;
			Modifier.Value = RequestedPower - 1.0f;
			return Attributes->AddModifier(Modifier, AttackHandle);
		}

		void Stop()
		{
			if (Session.IsActive())
			{
				if (Session.GetHostState()
					== Edemo_mapShanmenThrownWeaponHostState::InFlight)
				{
					Session.TryInterruptFlight();
				}
				FString Diagnostic;
				Session.TryEnd(Diagnostic);
			}
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
				Attributes = nullptr;
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

		~FSessionFixture()
		{
			Stop();
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponSessionContractTest,
	"Shanmen.0_0_10.Product.ThrownWeaponProductSession.ContractAndBinding",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponSessionContractTest::RunTest(const FString&)
{
	Fdemo_mapShanmenThrownWeaponHotbarIntent Forward;
	Fdemo_mapShanmenThrownWeaponHotbarIntent OtherSlot;
	TestTrue(TEXT("Hotbar intent canonicalizes one-based slot trajectory"),
		Fdemo_mapShanmenThrownWeaponHotbarIntent::TryCapture(
			SessionSelectionOneId,
			1,
			FVector::ZeroVector,
			FVector(5.0, 0.0, 0.0),
			1200.0f,
			Forward)
			&& Forward.IsValid()
			&& Forward.GetAimDirection() == FVector::ForwardVector);
	TestTrue(TEXT("Same SelectionId with another slot is a conflict"),
		Fdemo_mapShanmenThrownWeaponHotbarIntent::TryCapture(
			SessionSelectionOneId,
			9,
			FVector::ZeroVector,
			FVector::ForwardVector,
			1200.0f,
			OtherSlot)
			&& !Forward.Matches(OtherSlot));
	Fdemo_mapShanmenThrownWeaponHotbarIntent Invalid;
	TestFalse(TEXT("Slot zero is outside the product contract"),
		Fdemo_mapShanmenThrownWeaponHotbarIntent::TryCapture(
			SessionSelectionTwoId,
			0,
			FVector::ZeroVector,
			FVector::ForwardVector,
			1200.0f,
			Invalid));
	TestFalse(TEXT("Slot ten is outside the product contract"),
		Fdemo_mapShanmenThrownWeaponHotbarIntent::TryCapture(
			SessionSelectionTwoId,
			10,
			FVector::ZeroVector,
			FVector::ForwardVector,
			1200.0f,
			Invalid));

	FSessionFixture Fixture;
	if (!Fixture.Start(*this, TEXT("Contract")))
	{
		return false;
	}
	FString Diagnostic;
	TestTrue(TEXT("Exact session bind is idempotent"),
		Fixture.Session.TryBegin(
			Fixture.Correlation,
			*Fixture.Source,
			Fixture.Config,
			Diagnostic)
			&& Fixture.Session.IsValid());
	const Fdemo_mapShanmenThrownWeaponSessionResult Empty =
		Fixture.Session.TrySubmitHotbar(
			Fixture.World,
			Ademo_mapShanmenThrownWeaponProjectile::StaticClass(),
			*Fixture.Authority,
			Fixture.Coordinator,
			Fixture.MakeIntent(
				SessionSelectionOneId, 2, FVector::ForwardVector));
	TestTrue(TEXT("Frozen empty slot performs no action or capture"),
		Empty.Status
			== Edemo_mapShanmenThrownWeaponSessionStatus::HotbarSlotEmpty
			&& Fixture.Session.NumCapturedSelections() == 0
			&& Fixture.Coordinator
				.GetNextPlayerThrownWeaponActivationSequence() == 1);
	TestTrue(TEXT("Empty session ends cleanly"),
		Fixture.Session.TryEnd(Diagnostic)
			&& !Fixture.Session.IsActive()
			&& Fixture.Session.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponSessionFreezeReplayTest,
	"Shanmen.0_0_10.Product.ThrownWeaponProductSession.ResolveFreezeReplayConflict",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponSessionFreezeReplayTest::RunTest(const FString&)
{
	FSessionFixture Fixture;
	if (!Fixture.Start(*this, TEXT("FreezeReplay"))
		|| !Fixture.SetAttackPower(6.0f))
	{
		return false;
	}
	const Fdemo_mapShanmenThrownWeaponHotbarIntent Intent =
		Fixture.MakeIntent(
			SessionSelectionOneId, 1, FVector::ForwardVector);
	FShanmenItemAuthoritySnapshot Before;
	Fixture.Authority->TryCaptureSnapshot(Before);
	const Fdemo_mapShanmenThrownWeaponSessionResult Applied =
		Fixture.Session.TrySubmitHotbar(
			Fixture.World,
			Ademo_mapShanmenThrownWeaponProjectile::StaticClass(),
			*Fixture.Authority,
			Fixture.Coordinator,
			Intent);
	FShanmenItemAuthoritySnapshot After;
	Fixture.Authority->TryCaptureSnapshot(After);
	const Fdemo_mapShanmenThrownWeaponRunCommandIntent* Command =
		Fixture.Session.FindCapturedCommand(SessionSelectionOneId);
	TestTrue(TEXT("Hotbar slot resolves exact item and freezes final AttackPower"),
		Applied.IsAccepted()
			&& !Applied.bReusedSelection
			&& Applied.ItemInstanceId == SessionItemOneId
			&& Applied.TechniquePower == 6.0f
			&& Command
			&& Command->GetOffense().GetTechniquePower() == 6.0f
			&& Command->GetAction().GetSourceItemInstanceId()
				== SessionItemOneId
			&& Applied.Product.ActivationSequence == 1
			&& After.AuthorityRevision == Before.AuthorityRevision + 2
			&& Fixture.Session.IsValid());

	Ademo_mapShanmenThrownWeaponProjectile* FirstProjectile =
		Applied.IsAccepted()
			? Applied.Product.Command.HostStart.Spawn.Projectile.Get()
			: nullptr;
	if (!Fixture.SetAttackPower(10.0f))
	{
		return false;
	}
	const Fdemo_mapShanmenThrownWeaponSessionResult Replay =
		Fixture.Session.TrySubmitHotbar(
			Fixture.World,
			Ademo_mapShanmenThrownWeaponProjectile::StaticClass(),
			*Fixture.Authority,
			Fixture.Coordinator,
			Intent);
	FShanmenItemAuthoritySnapshot AfterReplay;
	Fixture.Authority->TryCaptureSnapshot(AfterReplay);
	TestTrue(TEXT("Exact replay retains first stat snapshot and physical Actor"),
		Replay.IsAccepted()
			&& Replay.bReusedSelection
			&& Replay.Product.Command.IsReplay()
			&& Replay.TechniquePower == 6.0f
			&& Replay.Product.Command.HostStart.Spawn.Projectile.Get()
				== FirstProjectile
			&& AfterReplay == After
			&& Fixture.Coordinator
				.GetNextPlayerThrownWeaponActivationSequence() == 2);

	const Fdemo_mapShanmenThrownWeaponSessionResult Conflict =
		Fixture.Session.TrySubmitHotbar(
			Fixture.World,
			Ademo_mapShanmenThrownWeaponProjectile::StaticClass(),
			*Fixture.Authority,
			Fixture.Coordinator,
			Fixture.MakeIntent(
				SessionSelectionOneId, 9, FVector::ForwardVector));
	TestTrue(TEXT("Conflicting slot cannot consume a sequence"),
		Conflict.Status
			== Edemo_mapShanmenThrownWeaponSessionStatus::SelectionIdConflict
			&& Fixture.Session.NumCapturedSelections() == 1
			&& Fixture.Coordinator
				.GetNextPlayerThrownWeaponActivationSequence() == 2);

	FString Diagnostic;
	TestTrue(TEXT("In-flight work blocks session teardown"),
		!Fixture.Session.TryEnd(Diagnostic));
	TestTrue(TEXT("Explicit terminal then end succeeds"),
		Fixture.Session.TryInterruptFlight()
			&& Fixture.Session.GetTerminalReceipt().Kind
				== Edemo_mapShanmenThrownWeaponTerminalKind::Interrupted
			&& Fixture.Session.TryEnd(Diagnostic));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponSessionBusyRetryTest,
	"Shanmen.0_0_10.Product.ThrownWeaponProductSession.BusyRetryTerminalRollover",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponSessionBusyRetryTest::RunTest(const FString&)
{
	FSessionFixture Fixture;
	if (!Fixture.Start(*this, TEXT("BusyRetry"))
		|| !Fixture.SetAttackPower(3.0f))
	{
		return false;
	}
	const Fdemo_mapShanmenThrownWeaponSessionResult First =
		Fixture.Session.TrySubmitHotbar(
			Fixture.World,
			Ademo_mapShanmenThrownWeaponProjectile::StaticClass(),
			*Fixture.Authority,
			Fixture.Coordinator,
			Fixture.MakeIntent(
				SessionSelectionOneId, 1, FVector::ForwardVector));
	if (!First.IsAccepted() || !Fixture.SetAttackPower(8.0f))
	{
		return false;
	}
	const Fdemo_mapShanmenThrownWeaponHotbarIntent SecondIntent =
		Fixture.MakeIntent(
			SessionSelectionTwoId, 9, FVector::RightVector);
	const Fdemo_mapShanmenThrownWeaponSessionResult Busy =
		Fixture.Session.TrySubmitHotbar(
			Fixture.World,
			Ademo_mapShanmenThrownWeaponProjectile::StaticClass(),
			*Fixture.Authority,
			Fixture.Coordinator,
			SecondIntent);
	TestTrue(TEXT("Busy Host freezes slot nine as sequence two"),
		Busy.Status
			== Edemo_mapShanmenThrownWeaponSessionStatus::ProductRejected
			&& Busy.Product.Command.Status
				== Edemo_mapShanmenThrownWeaponRunCommandStatus::HostBusy
			&& Busy.ItemInstanceId == SessionItemTwoId
			&& Busy.TechniquePower == 8.0f
			&& Busy.Product.ActivationSequence == 2
			&& Fixture.Coordinator
				.GetNextPlayerThrownWeaponActivationSequence() == 3);

	if (!Fixture.SetAttackPower(13.0f)
		|| !Fixture.Session.TryInterruptFlight())
	{
		return false;
	}
	const Fdemo_mapShanmenThrownWeaponSessionResult Retried =
		Fixture.Session.TrySubmitHotbar(
			Fixture.World,
			Ademo_mapShanmenThrownWeaponProjectile::StaticClass(),
			*Fixture.Authority,
			Fixture.Coordinator,
			SecondIntent);
	const Fdemo_mapShanmenThrownWeaponRunCommandIntent* SecondCommand =
		Fixture.Session.FindCapturedCommand(SessionSelectionTwoId);
	TestTrue(TEXT("Terminal rollover reuses sequence and first-attempt stats"),
		Retried.IsAccepted()
			&& Retried.bReusedSelection
			&& Retried.TechniquePower == 8.0f
			&& Retried.Product.ActivationSequence == 2
			&& SecondCommand
			&& SecondCommand->GetOffense().GetTechniquePower() == 8.0f
			&& Fixture.Coordinator
				.GetNextPlayerThrownWeaponActivationSequence() == 3
			&& Fixture.Session.IsValid());
	FString Diagnostic;
	TestTrue(TEXT("Second flight can terminate and close the session"),
		Fixture.Session.TryInterruptFlight()
			&& Fixture.Session.TryEnd(Diagnostic));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponSessionRecoveryTest,
	"Shanmen.0_0_10.Product.ThrownWeaponProductSession.RecoveryEndGate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponSessionRecoveryTest::RunTest(const FString&)
{
	FSessionFixture Fixture;
	if (!Fixture.Start(*this, TEXT("Recovery")))
	{
		return false;
	}
	const Fdemo_mapShanmenThrownWeaponSessionResult Occupied =
		Fixture.Session.TrySubmitHotbar(
			Fixture.World,
			Ademo_mapShanmenThrownWeaponProjectile::StaticClass(),
			*Fixture.Authority,
			Fixture.Coordinator,
			Fixture.MakeIntent(
				SessionSelectionOneId, 1, FVector::ForwardVector));
	if (!Occupied.IsAccepted())
	{
		AddError(TEXT("Could not occupy the P7.6 recovery Host."));
		return false;
	}

	const Fdemo_mapShanmenThrownWeaponHotbarIntent Intent =
		Fixture.MakeIntent(
			SessionSelectionTwoId, 9, FVector::RightVector);
	const Fdemo_mapShanmenThrownWeaponSessionResult CapturedBusy =
		Fixture.Session.TrySubmitHotbar(
			Fixture.World,
			Ademo_mapShanmenThrownWeaponProjectile::StaticClass(),
			*Fixture.Authority,
			Fixture.Coordinator,
			Intent);
	const Fdemo_mapShanmenThrownWeaponRunCommandIntent* Captured =
		Fixture.Session.FindCapturedCommand(SessionSelectionTwoId);
	if (CapturedBusy.Product.Command.Status
			!= Edemo_mapShanmenThrownWeaponRunCommandStatus::HostBusy
		|| !Captured
		|| !Fixture.Session.TryInterruptFlight())
	{
		AddError(TEXT("Could not freeze the P7.6 recovery selection."));
		return false;
	}
	const Fdemo_mapShanmenThrownWeaponItemResult Preprepared =
		Fdemo_mapShanmenThrownWeaponItemAdapter::PrepareActiveRun(
			*Fixture.Authority,
			Fixture.Correlation,
			Captured->GetAction());
	if (!Preprepared.IsPrepared())
	{
		AddError(TEXT("Could not preprepare P7.6 cancellation recovery."));
		return false;
	}
	Fixture.Authority->SetInjectedFailureForAutomation(
		EShanmenItemStoreFailureStage::WriteTemp);
	const Fdemo_mapShanmenThrownWeaponSessionResult Interrupted =
		Fixture.Session.TrySubmitHotbar(
			Fixture.World,
			nullptr,
			*Fixture.Authority,
			Fixture.Coordinator,
			Intent);
	TestTrue(TEXT("Failed cancellation remains owned by its hotbar selection"),
		Interrupted.Status
			== Edemo_mapShanmenThrownWeaponSessionStatus::ProductRejected
			&& Interrupted.RequiresRecovery()
			&& Interrupted.bReusedSelection
			&& Interrupted.Product.ActivationSequence == 2
			&& Fixture.Coordinator
				.GetNextPlayerThrownWeaponActivationSequence() == 3
			&& Fixture.Session.GetHostState()
				== Edemo_mapShanmenThrownWeaponHostState::Empty
			&& Fixture.Session.IsValid());

	FString Diagnostic;
	TestTrue(TEXT("Unresolved durable recovery blocks session end"),
		!Fixture.Session.TryEnd(Diagnostic));
	Fixture.Authority->SetInjectedFailureForAutomation(
		EShanmenItemStoreFailureStage::None);
	const Fdemo_mapShanmenThrownWeaponSessionResult Recovered =
		Fixture.Session.TryRecoverCancellation(
			*Fixture.Authority, Intent);
	TestTrue(TEXT("Session recovery cancels only and preserves empty Host"),
		Recovered.IsRecoveryApplied()
			&& Recovered.bReusedSelection
			&& Recovered.Product.ActivationSequence == 2
			&& Fixture.Session.GetHostState()
				== Edemo_mapShanmenThrownWeaponHostState::Empty
			&& Fixture.Session.IsValid());
	TestTrue(TEXT("Recovered session may now end"),
		Fixture.Session.TryEnd(Diagnostic)
			&& !Fixture.Session.IsActive()
			&& Fixture.Session.IsValid());
	return true;
}

#endif
