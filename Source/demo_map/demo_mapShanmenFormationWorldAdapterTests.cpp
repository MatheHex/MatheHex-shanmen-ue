#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenFormationWorldAdapter.h"

#include "ShanmenCombatResolver.h"
#include "demo_map0909BSectWarehouseService.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapProfileRepository.h"
#include "demo_mapProfileSessionSubsystem.h"
#include "demo_mapShanmenItemAuthoritySubsystem.h"
#include "demo_mapShanmenItemCutover.h"
#include "demo_mapShanmenPreparationAdapter.h"
#include "demo_mapShanmenRunLifecycleAdapter.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	const FGuid WorldAttemptA(0xF8300001, 0, 0, 1);
	const FGuid WorldAttemptB(0xF8300002, 0, 0, 1);
	const FGuid WorldSourceEntityId(0xF8300010, 0, 0, 1);
	const FName WorldAnchorA(TEXT("Formation.Anchor.WorldDelivery.A"));
	const FName WorldAnchorB(TEXT("Formation.Anchor.WorldDelivery.B"));

	FString NewFormationWorldRoot(const TCHAR* Label)
	{
		return FPaths::Combine(
			FPaths::ProjectSavedDir(), TEXT("Automation"),
			TEXT("Dev.D.UE.0.0.10.P8.3.r0"), Label,
			FGuid::NewGuid().ToString(EGuidFormats::Digits));
	}

	FShanmenFormationDiagramDefinition MakeWorldDiagram()
	{
		FShanmenFormationDiagramCapture Capture;
		Capture.ActionDefinitionId =
			FShanmenFormationDiagramDefinition::CanonicalActionDefinitionId();
		Capture.DiagramDefinitionId =
			TEXT("Formation.Diagram.WorldDelivery.P8.3");
		Capture.ActivationEnergyCost.RuleId =
			TEXT("Formation.ActivationEnergy.WorldDelivery.P8.3");
		Capture.ActivationEnergyCost.ResourceChannel =
			FShanmenFormationDiagramDefinition::
				CanonicalActivationEnergyChannel();
		Capture.ActivationEnergyCost.Amount = 10.0f;
		FShanmenFormationAnchorCapture& First =
			Capture.Anchors.AddDefaulted_GetRef();
		First.Order = 0;
		First.AnchorDefinitionId = WorldAnchorA;
		First.RelativeOffset = FVector(100.0, -50.0, 25.0);
		FShanmenFormationMaterialRequirementCapture& FirstWood =
			First.Requirements.AddDefaulted_GetRef();
		FirstWood.Order = 0;
		FirstWood.MaterialDefinitionId = Fdemo_mapItemIds::SpiritWoodLevel1;
		FirstWood.Quantity = 1;
		FShanmenFormationAnchorCapture& Second =
			Capture.Anchors.AddDefaulted_GetRef();
		Second.Order = 1;
		Second.AnchorDefinitionId = WorldAnchorB;
		Second.RelativeOffset = FVector(100.0, 50.0, 25.0);
		FShanmenFormationMaterialRequirementCapture& SecondWood =
			Second.Requirements.AddDefaulted_GetRef();
		SecondWood.Order = 0;
		SecondWood.MaterialDefinitionId = Fdemo_mapItemIds::SpiritWoodLevel1;
		SecondWood.Quantity = 2;
		FShanmenFormationDiagramDefinition Diagram;
		check(FShanmenFormationDiagramDefinition::TryCapture(
			Capture, Diagram));
		return Diagram;
	}

	int32 CountTaggedActors(UWorld* World, const FName Tag)
	{
		if (!World || Tag.IsNone())
		{
			return 0;
		}
		int32 Count = 0;
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			AActor* Actor = *It;
			if (IsValid(Actor) && !Actor->IsActorBeingDestroyed()
				&& Actor->ActorHasTag(Tag))
			{
				++Count;
			}
		}
		return Count;
	}

	struct FFormationWorldFixture
	{
		FString Root;
		Fdemo_mapProfileStorageContext Storage;
		Fdemo_mapPersistentProfile SeedProfile;
		FGuid WoodAId;
		FGuid WoodBId;
		UGameInstance* GameInstance = nullptr;
		Udemo_mapShanmenItemAuthoritySubsystem* Authority = nullptr;
		Udemo_mapProfileSessionSubsystem* ProfileSession = nullptr;
		UWorld* World = nullptr;
		Fdemo_mapShanmenRunCorrelation Correlation;
		Fdemo_mapShanmenFormationProductSession Session;

		bool Start(FAutomationTestBase& Test, const TCHAR* Label)
		{
			Root = NewFormationWorldRoot(Label);
			Storage = Fdemo_mapProfileStorageContext::ForRoot(Root);
			Fdemo_mapProfileRepository Repository;
			SeedProfile = Repository.CreateFreshProfile();
			Fdemo_mapPersistentItemRecord WoodA;
			WoodAId = WoodA.ItemInstanceId = FGuid::NewGuid();
			WoodA.ItemDefinitionId = Fdemo_mapItemIds::SpiritWoodLevel1;
			WoodA.StackCount = 2;
			WoodA.PersistentDomain =
				Edemo_mapPersistentDomain::PermanentStash;
			SeedProfile.PermanentStash.Add(WoodA);
			Fdemo_mapPersistentItemRecord WoodB = WoodA;
			WoodBId = WoodB.ItemInstanceId = FGuid::NewGuid();
			SeedProfile.PermanentStash.Add(WoodB);
			const Fdemo_mapProfileSaveResult Saved =
				Repository.SaveProfile(SeedProfile, Storage);
			if (!Saved.IsSuccess() || !GEngine)
			{
				Test.AddError(FString::Printf(
					TEXT("P8.3 isolated seed failed: %s"),
					*Saved.Diagnostic));
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
					Root, Initialized.Snapshot,
					Edemo_map0909BTopState::AtSect,
					Presentation, Diagnostic))
			{
				Test.AddError(FString::Printf(
					TEXT("P8.3 migration source failed: %s"),
					*Diagnostic));
				return false;
			}
			const Fdemo_mapShanmenItemCutoverResult Cutover =
				Fdemo_mapShanmenItemCutoverCoordinator::Execute(
					Storage, SeedProfile.ProfileId, *Authority,
					*ProfileSession, Warehouse);
			if (!Cutover.IsReady()
				|| !ProfileSession->SetPreparationMaterial(
					WoodAId, true).IsAccepted()
				|| !ProfileSession->SetPreparationMaterial(
					WoodBId, true).IsAccepted())
			{
				Test.AddError(FString::Printf(
					TEXT("P8.3 preparation failed: %s"),
					*Cutover.Diagnostic));
				return false;
			}
			const Fdemo_mapShanmenPreparedLoadoutResult Started =
				Fdemo_mapShanmenPreparationAdapter::
					StartPreparedLoadout(*Authority);
			if (!Started.IsCommitted()
				|| !Fdemo_mapShanmenRunLifecycleAdapter::
					TryGetActiveRunCorrelation(
						*Authority, Correlation, &Diagnostic))
			{
				Test.AddError(FString::Printf(
					TEXT("P8.3 active Run failed: %s"),
					*Diagnostic));
				return false;
			}

			FShanmenItemAuthoritySnapshot Snapshot;
			if (!Authority->TryCaptureSnapshot(Snapshot))
			{
				return false;
			}
			FShanmenCombatActionCapture ActionCapture;
			ActionCapture.RunId = Correlation.ActiveRunId;
			ActionCapture.OwnerId = Correlation.OwnerId;
			ActionCapture.SourceEntityId = WorldSourceEntityId;
			ActionCapture.ActionDefinitionId =
				FShanmenFormationDiagramDefinition::
					CanonicalActionDefinitionId();
			ActionCapture.Content = Snapshot.Content;
			ActionCapture.ActivationId =
				FShanmenCombatIdFactory::MakeActivationId(
					ActionCapture.RunId,
					ActionCapture.SourceEntityId,
					ActionCapture.ActionDefinitionId,
					83);
			FShanmenCombatActionSnapshot Action;
			FShanmenActionTransitionReceipt Startup;
			FShanmenActionTransitionReceipt Active;
			FShanmenFormationDeploymentReceipt Begin;
			if (!FShanmenCombatActionSnapshot::TryCapture(
					ActionCapture, Action)
				|| !Fdemo_mapShanmenFormationProductSession::TryStart(
					Correlation, Action, MakeWorldDiagram(),
					FVector::ZeroVector, FVector::ForwardVector,
					Session, Startup, Active, Begin))
			{
				Test.AddError(TEXT(
					"P8.3 formation product session failed to start."));
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
			return true;
		}

		bool CommitAnchor(
			FAutomationTestBase& Test,
			const FName AnchorDefinitionId,
			const FGuid& AttemptId)
		{
			const Fdemo_mapShanmenFormationSessionResult Prepared =
				Session.TryPrepareAnchor(
					*Authority, Correlation,
					AnchorDefinitionId, AttemptId);
			const Fdemo_mapShanmenFormationSessionResult Committed =
				Session.TryCommitPreparedAnchor(
					*Authority, Correlation,
					AnchorDefinitionId, AttemptId);
			if (!Prepared.IsSuccess() || !Committed.IsSuccess())
			{
				Test.AddError(FString::Printf(
					TEXT("P8.3 anchor commit failed: %s / %s"),
					*Prepared.Diagnostic, *Committed.Diagnostic));
				return false;
			}
			return true;
		}

		void Stop()
		{
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
				Authority = nullptr;
				ProfileSession = nullptr;
				GameInstance->RemoveFromRoot();
				GameInstance->MarkAsGarbage();
				GameInstance = nullptr;
				CollectGarbage(RF_NoFlags);
			}
			if (!Root.IsEmpty())
			{
				IFileManager::Get().DeleteDirectory(*Root, false, true);
				Root.Reset();
			}
		}

		~FFormationWorldFixture()
		{
			Stop();
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationWorldPlacementReplayTest,
	"Shanmen.0_0_10.Product.FormationWorldDelivery.PlacementReplay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationWorldPlacementReplayTest::RunTest(const FString&)
{
	FFormationWorldFixture Fixture;
	if (!Fixture.Start(*this, TEXT("PlacementReplay"))
		|| !Fixture.CommitAnchor(*this, WorldAnchorA, WorldAttemptA))
	{
		return false;
	}
	Fdemo_mapShanmenFormationAnchorPlacementIntent Intent;
	const bool bBuilt =
		Fdemo_mapShanmenFormationWorldAdapter::BuildPlacementIntent(
			Fixture.Session, WorldAnchorA, Intent);
	Fdemo_mapShanmenFormationWorldAdapter Adapter;
	const Fdemo_mapShanmenFormationWorldResult First =
		Adapter.TryPlaceCommittedAnchor(
			Fixture.World, ACharacter::StaticClass(),
			Fixture.Session, WorldAnchorA);
	const Fdemo_mapShanmenFormationWorldResult Replay =
		Adapter.TryPlaceCommittedAnchor(
			Fixture.World, ACharacter::StaticClass(),
			Fixture.Session, WorldAnchorA);
	TestTrue(TEXT("Committed audit freezes one deterministic placement intent"),
		bBuilt && Intent.IsValid()
			&& Intent.WorldLocation.Equals(
				FVector(100.0, -50.0, 25.0), KINDA_SMALL_NUMBER));
	TestTrue(TEXT("First world projection publishes one tagged transient Actor"),
		First.Status == Edemo_mapShanmenFormationWorldStatus::Placed
			&& First.IsPlacementSuccess()
			&& CountTaggedActors(
				Fixture.World, First.PlacementReceipt.PlacementTag) == 1);
	TestTrue(TEXT("Exact replay returns the same Actor and receipt without spawn"),
		Replay.Status == Edemo_mapShanmenFormationWorldStatus::Replayed
			&& Replay.IsPlacementSuccess()
			&& Replay.Actor.Get() == First.Actor.Get()
			&& Replay.PlacementReceipt.ReceiptId
				== First.PlacementReceipt.ReceiptId
			&& Adapter.GetPlacementCount() == 1
			&& Adapter.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationWorldReconstructionConflictTest,
	"Shanmen.0_0_10.Product.FormationWorldDelivery.ReconstructionAndConflict",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationWorldReconstructionConflictTest::RunTest(
	const FString&)
{
	FFormationWorldFixture Fixture;
	if (!Fixture.Start(*this, TEXT("Reconstruction"))
		|| !Fixture.CommitAnchor(*this, WorldAnchorA, WorldAttemptA))
	{
		return false;
	}
	Fdemo_mapShanmenFormationWorldAdapter FirstAdapter;
	const Fdemo_mapShanmenFormationWorldResult First =
		FirstAdapter.TryPlaceCommittedAnchor(
			Fixture.World, ACharacter::StaticClass(),
			Fixture.Session, WorldAnchorA);
	Fdemo_mapShanmenFormationWorldAdapter Reconstructed;
	const Fdemo_mapShanmenFormationWorldResult Adopted =
		Reconstructed.TryPlaceCommittedAnchor(
			Fixture.World, ACharacter::StaticClass(),
			Fixture.Session, WorldAnchorA);
	const Fdemo_mapShanmenFormationWorldResult ClassConflict =
		Reconstructed.TryPlaceCommittedAnchor(
			Fixture.World, APawn::StaticClass(),
			Fixture.Session, WorldAnchorA);
	TestTrue(TEXT("A reconstructed adapter adopts the sole exact tagged Actor"),
		First.IsPlacementSuccess()
			&& Adopted.Status
				== Edemo_mapShanmenFormationWorldStatus::Adopted
			&& Adopted.Actor.Get() == First.Actor.Get()
			&& Adopted.PlacementReceipt.ReceiptId
				== First.PlacementReceipt.ReceiptId);
	TestTrue(TEXT("Actor class cannot create a second identity for one placement"),
		ClassConflict.Status
			== Edemo_mapShanmenFormationWorldStatus::ActorClassConflict
			&& CountTaggedActors(
				Fixture.World, First.PlacementReceipt.PlacementTag) == 1);

	AActor* Duplicate = Fixture.World->SpawnActor<AActor>(
		ACharacter::StaticClass(),
		FTransform(FRotator::ZeroRotator, First.Intent.WorldLocation));
	if (!Duplicate)
	{
		return false;
	}
	Duplicate->Tags.Add(First.PlacementReceipt.PlacementTag);
	Duplicate->Tags.Add(First.PlacementReceipt.DeploymentTag);
	Fdemo_mapShanmenFormationWorldAdapter Third;
	const Fdemo_mapShanmenFormationWorldResult DuplicateResult =
		Third.TryPlaceCommittedAnchor(
			Fixture.World, ACharacter::StaticClass(),
			Fixture.Session, WorldAnchorA);
	TestTrue(TEXT("Duplicate placement tags fail closed without adopting either"),
		DuplicateResult.Status
			== Edemo_mapShanmenFormationWorldStatus::
				DuplicatePlacementActors
			&& Third.GetPlacementCount() == 0
			&& CountTaggedActors(
				Fixture.World, First.PlacementReceipt.PlacementTag) == 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationWorldFailClosedTest,
	"Shanmen.0_0_10.Product.FormationWorldDelivery.FailClosed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationWorldFailClosedTest::RunTest(const FString&)
{
	FFormationWorldFixture Fixture;
	if (!Fixture.Start(*this, TEXT("FailClosed")))
	{
		return false;
	}
	Fdemo_mapShanmenFormationWorldAdapter Adapter;
	const Fdemo_mapShanmenFormationWorldResult Uncommitted =
		Adapter.TryPlaceCommittedAnchor(
			Fixture.World, ACharacter::StaticClass(),
			Fixture.Session, WorldAnchorA);
	if (!Fixture.CommitAnchor(*this, WorldAnchorA, WorldAttemptA))
	{
		return false;
	}
	const Fdemo_mapShanmenFormationWorldResult NoWorld =
		Adapter.TryPlaceCommittedAnchor(
			nullptr, ACharacter::StaticClass(),
			Fixture.Session, WorldAnchorA);
	const Fdemo_mapShanmenFormationWorldResult NoClass =
		Adapter.TryPlaceCommittedAnchor(
			Fixture.World, TSubclassOf<AActor>(),
			Fixture.Session, WorldAnchorA);
	const Fdemo_mapShanmenFormationSessionResult Cancelled =
		Fixture.Session.TryCancel(*Fixture.Authority, Fixture.Correlation);
	const Fdemo_mapShanmenFormationWorldResult TerminalPlacement =
		Adapter.TryPlaceCommittedAnchor(
			Fixture.World, ACharacter::StaticClass(),
			Fixture.Session, WorldAnchorA);
	TestTrue(TEXT("Uncommitted, missing World, and missing class all fail before spawn"),
		Uncommitted.Status
			== Edemo_mapShanmenFormationWorldStatus::AnchorNotCommitted
			&& NoWorld.Status
				== Edemo_mapShanmenFormationWorldStatus::WorldInvalid
			&& NoClass.Status
				== Edemo_mapShanmenFormationWorldStatus::ActorClassInvalid);
	TestTrue(TEXT("Terminal session rejects any late world publication"),
		Cancelled.IsSuccess() && Fixture.Session.IsTerminal()
			&& TerminalPlacement.Status
				== Edemo_mapShanmenFormationWorldStatus::SessionTerminal
			&& Adapter.GetPlacementCount() == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationWorldTerminalTeardownTest,
	"Shanmen.0_0_10.Product.FormationWorldDelivery.TerminalTeardownReplay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationWorldTerminalTeardownTest::RunTest(const FString&)
{
	FFormationWorldFixture Fixture;
	if (!Fixture.Start(*this, TEXT("TerminalTeardown"))
		|| !Fixture.CommitAnchor(*this, WorldAnchorA, WorldAttemptA))
	{
		return false;
	}
	Fdemo_mapShanmenFormationWorldAdapter Publisher;
	const Fdemo_mapShanmenFormationWorldResult First =
		Publisher.TryPlaceCommittedAnchor(
			Fixture.World, ACharacter::StaticClass(),
			Fixture.Session, WorldAnchorA);
	const Fdemo_mapShanmenFormationWorldResult EarlyTeardown =
		Publisher.TryTeardownTerminal(Fixture.World, Fixture.Session);
	if (!Fixture.CommitAnchor(*this, WorldAnchorB, WorldAttemptB))
	{
		return false;
	}
	const Fdemo_mapShanmenFormationWorldResult Second =
		Publisher.TryPlaceCommittedAnchor(
			Fixture.World, ACharacter::StaticClass(),
			Fixture.Session, WorldAnchorB);
	const int32 LiveBeforeEnd = CountTaggedActors(
		Fixture.World, First.PlacementReceipt.DeploymentTag);
	const Fdemo_mapShanmenFormationSessionResult Ended =
		Fixture.Session.TryEnd(Fixture.Correlation);
	Fdemo_mapShanmenFormationWorldAdapter Reconstructed;
	const Fdemo_mapShanmenFormationWorldResult Teardown =
		Reconstructed.TryTeardownTerminal(Fixture.World, Fixture.Session);
	const Fdemo_mapShanmenFormationWorldResult Replay =
		Reconstructed.TryTeardownTerminal(Fixture.World, Fixture.Session);
	const int32 LiveAfterTeardown = CountTaggedActors(
		Fixture.World, First.PlacementReceipt.DeploymentTag);
	TestTrue(TEXT("A non-terminal session cannot remove live formation Actors"),
		First.IsPlacementSuccess()
			&& EarlyTeardown.Status
				== Edemo_mapShanmenFormationWorldStatus::TerminalRequired
			&& LiveBeforeEnd == 2);
	TestTrue(TEXT("Ended session lets a reconstructed adapter remove every deployment Actor"),
		Second.IsPlacementSuccess() && Ended.IsSuccess()
			&& Teardown.Status
				== Edemo_mapShanmenFormationWorldStatus::TeardownComplete
			&& Teardown.IsTeardownSuccess()
			&& Teardown.TeardownReceipt.CommittedAnchorCount == 2
			&& Teardown.TeardownReceipt.RemovedActorCount == 2
			&& LiveAfterTeardown == 0);
	TestTrue(TEXT("Exact terminal cleanup replays one stable receipt"),
		Replay.Status
			== Edemo_mapShanmenFormationWorldStatus::TeardownReplayed
			&& Replay.IsTeardownSuccess()
			&& Replay.TeardownReceipt.ReceiptId
				== Teardown.TeardownReceipt.ReceiptId
			&& Reconstructed.IsTeardownComplete()
			&& Reconstructed.IsValid());
	return true;
}

#endif
