#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenFormationProductHost.h"

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
	const FGuid HostAttemptA(0xF8400001, 0, 0, 1);
	const FGuid HostAttemptB(0xF8400002, 0, 0, 1);
	const FGuid HostAttemptOther(0xF8400003, 0, 0, 1);
	const FGuid HostSourceEntityId(0xF8400010, 0, 0, 1);
	const FName HostAnchorA(TEXT("Formation.Anchor.ProductHost.A"));
	const FName HostAnchorB(TEXT("Formation.Anchor.ProductHost.B"));

	FString NewFormationHostRoot(const TCHAR* Label)
	{
		return FPaths::Combine(
			FPaths::ProjectSavedDir(), TEXT("Automation"),
			TEXT("Dev.D.UE.0.0.10.P8.4.r0"), Label,
			FGuid::NewGuid().ToString(EGuidFormats::Digits));
	}

	FShanmenFormationDiagramDefinition MakeHostDiagram()
	{
		FShanmenFormationDiagramCapture Capture;
		Capture.ActionDefinitionId =
			FShanmenFormationDiagramDefinition::CanonicalActionDefinitionId();
		Capture.DiagramDefinitionId =
			TEXT("Formation.Diagram.ProductHost.P8.4");
		FShanmenFormationAnchorCapture& First =
			Capture.Anchors.AddDefaulted_GetRef();
		First.Order = 0;
		First.AnchorDefinitionId = HostAnchorA;
		First.RelativeOffset = FVector(100.0, -50.0, 25.0);
		FShanmenFormationMaterialRequirementCapture& FirstWood =
			First.Requirements.AddDefaulted_GetRef();
		FirstWood.Order = 0;
		FirstWood.MaterialDefinitionId = Fdemo_mapItemIds::SpiritWoodLevel1;
		FirstWood.Quantity = 1;
		FShanmenFormationAnchorCapture& Second =
			Capture.Anchors.AddDefaulted_GetRef();
		Second.Order = 1;
		Second.AnchorDefinitionId = HostAnchorB;
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

	struct FFormationHostFixture
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
		Fdemo_mapShanmenFormationProductHost Host;

		bool Start(FAutomationTestBase& Test, const TCHAR* Label)
		{
			Root = NewFormationHostRoot(Label);
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
					TEXT("P8.4 isolated seed failed: %s"),
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
					TEXT("P8.4 migration source failed: %s"),
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
					TEXT("P8.4 preparation failed: %s"),
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
					TEXT("P8.4 active Run failed: %s"),
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
			ActionCapture.SourceEntityId = HostSourceEntityId;
			ActionCapture.ActionDefinitionId =
				FShanmenFormationDiagramDefinition::
					CanonicalActionDefinitionId();
			ActionCapture.Content = Snapshot.Content;
			ActionCapture.ActivationId =
				FShanmenCombatIdFactory::MakeActivationId(
					ActionCapture.RunId,
					ActionCapture.SourceEntityId,
					ActionCapture.ActionDefinitionId,
					84);
			FShanmenCombatActionSnapshot Action;
			FShanmenActionTransitionReceipt Startup;
			FShanmenActionTransitionReceipt Active;
			FShanmenFormationDeploymentReceipt Begin;
			if (!FShanmenCombatActionSnapshot::TryCapture(
					ActionCapture, Action)
				|| !Fdemo_mapShanmenFormationProductHost::TryStart(
					Correlation, Action, MakeHostDiagram(),
					FVector::ZeroVector, FVector::ForwardVector,
					Host, Startup, Active, Begin))
			{
				Test.AddError(TEXT(
					"P8.4 formation product host failed to start."));
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

		bool CaptureSnapshot(FShanmenItemAuthoritySnapshot& OutSnapshot) const
		{
			return Authority && Authority->TryCaptureSnapshot(OutSnapshot);
		}

		bool PrepareAndCommit(
			FAutomationTestBase& Test,
			const FName AnchorDefinitionId,
			const FGuid& AttemptId,
			Fdemo_mapShanmenFormationHostResult& OutCommit)
		{
			const Fdemo_mapShanmenFormationHostResult Prepared =
				Host.TryPrepareAnchor(
					*Authority, Correlation,
					AnchorDefinitionId, AttemptId);
			OutCommit = Host.TryCommitPreparedAnchor(
				*Authority, Correlation, AnchorDefinitionId, AttemptId);
			if (!Prepared.IsSuccess() || !OutCommit.IsSuccess())
			{
				Test.AddError(FString::Printf(
					TEXT("P8.4 prepare/commit failed: %s / %s"),
					*Prepared.Diagnostic, *OutCommit.Diagnostic));
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

		~FFormationHostFixture()
		{
			Stop();
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationHostDurableCommitTest,
	"Shanmen.0_0_10.Product.FormationProductHost.DurableCommitReplay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationHostDurableCommitTest::RunTest(const FString&)
{
	FFormationHostFixture Fixture;
	if (!Fixture.Start(*this, TEXT("DurableCommit")))
	{
		return false;
	}
	Fdemo_mapShanmenFormationHostResult Committed;
	if (!Fixture.PrepareAndCommit(
			*this, HostAnchorA, HostAttemptA, Committed))
	{
		return false;
	}
	FShanmenItemAuthoritySnapshot AfterCommit;
	if (!Fixture.CaptureSnapshot(AfterCommit))
	{
		return false;
	}
	const Fdemo_mapShanmenFormationHostResult Replay =
		Fixture.Host.TryCommitPreparedAnchor(
			*Fixture.Authority, Fixture.Correlation,
			HostAnchorA, HostAttemptA);
	FShanmenItemAuthoritySnapshot AfterReplay;
	if (!Fixture.CaptureSnapshot(AfterReplay))
	{
		return false;
	}
	const Fdemo_mapShanmenFormationHostResult Blocked =
		Fixture.Host.TryPrepareAnchor(
			*Fixture.Authority, Fixture.Correlation,
			HostAnchorB, HostAttemptB);
	TestTrue(TEXT("Durable commit publishes the sole canonical placement intent"),
		Committed.Status
			== Edemo_mapShanmenFormationHostStatus::
				CommittedPendingPlacement
			&& Committed.PlacementIntent.IsValid()
			&& Fixture.Host.HasPendingPlacement()
			&& Fixture.Host.IsValid());
	TestTrue(TEXT("Exact commit replay does not consume material twice"),
		Committed.Session.Material.Lines.Num() == 1
			&& Committed.Session.Material.FinalizeReceipts.Num() == 1
			&& Committed.Session.Material.FinalizeReceipts[0].ResourceAfter
				== Committed.Session.Material.Lines[0].ExpectedQuantityBefore
					- Committed.Session.Material.Lines[0].Quantity
			&& AfterCommit == AfterReplay && Replay.Status
				== Edemo_mapShanmenFormationHostStatus::
					CommittedPendingPlacement
			&& Replay.Session.Status
				== Edemo_mapShanmenFormationSessionStatus::Replayed
			&& Replay.Session.Material.Evidence.FulfillmentId
				== Committed.Session.Material.Evidence.FulfillmentId
			&& Replay.PlacementIntent.PlacementId
				== Committed.PlacementIntent.PlacementId);
	TestTrue(TEXT("A different anchor cannot overtake pending World delivery"),
		Blocked.Status
			== Edemo_mapShanmenFormationHostStatus::PlacementPending
			&& Fixture.Host.GetSession().GetAnchorAudits().Num() == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationHostPlacementRecoveryTest,
	"Shanmen.0_0_10.Product.FormationProductHost.PlacementRecoveryBinding",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationHostPlacementRecoveryTest::RunTest(const FString&)
{
	FFormationHostFixture Fixture;
	Fdemo_mapShanmenFormationHostResult Committed;
	if (!Fixture.Start(*this, TEXT("PlacementRecovery"))
		|| !Fixture.PrepareAndCommit(
			*this, HostAnchorA, HostAttemptA, Committed))
	{
		return false;
	}
	const FName PlacementTag =
		Fdemo_mapShanmenFormationWorldAdapter::MakePlacementTag(
			Committed.PlacementIntent.PlacementId);
	const FName DeploymentTag =
		Fdemo_mapShanmenFormationWorldAdapter::MakeDeploymentTag(
			Committed.PlacementIntent.DeploymentId);
	AActor* FirstDuplicate = Fixture.World->SpawnActor<AActor>(
		ACharacter::StaticClass(),
		FTransform(
			FRotator::ZeroRotator,
			Committed.PlacementIntent.WorldLocation));
	AActor* SecondDuplicate = Fixture.World->SpawnActor<AActor>(
		ACharacter::StaticClass(),
		FTransform(
			FRotator::ZeroRotator,
			Committed.PlacementIntent.WorldLocation));
	if (!FirstDuplicate || !SecondDuplicate)
	{
		return false;
	}
	FirstDuplicate->Tags.Add(PlacementTag);
	FirstDuplicate->Tags.Add(DeploymentTag);
	SecondDuplicate->Tags.Add(PlacementTag);
	SecondDuplicate->Tags.Add(DeploymentTag);
	const Fdemo_mapShanmenFormationHostResult Failed =
		Fixture.Host.TryPlaceCommittedAnchor(
			Fixture.World, ACharacter::StaticClass(),
			Fixture.Correlation, HostAnchorA, HostAttemptA);
	const bool bPendingAfterFailure = Fixture.Host.HasPendingPlacement();
	const int32 TaggedAfterFailure = CountTaggedActors(
		Fixture.World, PlacementTag);
	const Fdemo_mapShanmenFormationHostResult Drift =
		Fixture.Host.TryPlaceCommittedAnchor(
			Fixture.World, APawn::StaticClass(),
			Fixture.Correlation, HostAnchorA, HostAttemptA);
	Fixture.World->DestroyActor(SecondDuplicate, true, true);
	const Fdemo_mapShanmenFormationHostResult Recovered =
		Fixture.Host.TryPlaceCommittedAnchor(
			Fixture.World, ACharacter::StaticClass(),
			Fixture.Correlation, HostAnchorA, HostAttemptA);
	const Fdemo_mapShanmenFormationHostResult Replay =
		Fixture.Host.TryPlaceCommittedAnchor(
			Fixture.World, ACharacter::StaticClass(),
			Fixture.Correlation, HostAnchorA, HostAttemptA);
	TestTrue(TEXT("Duplicate-tag failure retains one forward-only placement"),
		Failed.Status
			== Edemo_mapShanmenFormationHostStatus::WorldRejected
			&& Failed.World.Status
				== Edemo_mapShanmenFormationWorldStatus::
					DuplicatePlacementActors
			&& bPendingAfterFailure && TaggedAfterFailure == 2);
	TestTrue(TEXT("Retry cannot drift from its first valid class binding"),
		Drift.Status
			== Edemo_mapShanmenFormationHostStatus::
				PlacementBindingConflict);
	TestTrue(TEXT("Exact retry adopts one survivor then replays it"),
		Recovered.Status == Edemo_mapShanmenFormationHostStatus::Replayed
			&& Recovered.World.Status
				== Edemo_mapShanmenFormationWorldStatus::Adopted
			&& !Fixture.Host.HasPendingPlacement()
			&& Replay.Status
				== Edemo_mapShanmenFormationHostStatus::Replayed
			&& Replay.World.Status
				== Edemo_mapShanmenFormationWorldStatus::Replayed
			&& Replay.World.Actor.Get() == FirstDuplicate
			&& Fixture.Host.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationHostEndTeardownTest,
	"Shanmen.0_0_10.Product.FormationProductHost.EndTeardownReplay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationHostEndTeardownTest::RunTest(const FString&)
{
	FFormationHostFixture Fixture;
	Fdemo_mapShanmenFormationHostResult FirstCommit;
	if (!Fixture.Start(*this, TEXT("EndTeardown"))
		|| !Fixture.PrepareAndCommit(
			*this, HostAnchorA, HostAttemptA, FirstCommit))
	{
		return false;
	}
	const Fdemo_mapShanmenFormationHostResult FirstPlacement =
		Fixture.Host.TryPlaceCommittedAnchor(
			Fixture.World, ACharacter::StaticClass(),
			Fixture.Correlation, HostAnchorA, HostAttemptA);
	Fdemo_mapShanmenFormationHostResult SecondCommit;
	if (!FirstPlacement.IsSuccess()
		|| !Fixture.PrepareAndCommit(
			*this, HostAnchorB, HostAttemptB, SecondCommit))
	{
		return false;
	}
	const Fdemo_mapShanmenFormationHostResult EarlyEnd =
		Fixture.Host.TryEndAndTeardown(
			Fixture.World, Fixture.Correlation);
	const Fdemo_mapShanmenFormationHostResult SecondPlacement =
		Fixture.Host.TryPlaceCommittedAnchor(
			Fixture.World, ACharacter::StaticClass(),
			Fixture.Correlation, HostAnchorB, HostAttemptB);
	const int32 LiveBeforeEnd = CountTaggedActors(
		Fixture.World, FirstPlacement.World.PlacementReceipt.DeploymentTag);
	const Fdemo_mapShanmenFormationHostResult Ended =
		Fixture.Host.TryEndAndTeardown(
			Fixture.World, Fixture.Correlation);
	const Fdemo_mapShanmenFormationHostResult Replay =
		Fixture.Host.TryEndAndTeardown(
			Fixture.World, Fixture.Correlation);
	TestTrue(TEXT("Final placement is a hard gate before action completion"),
		EarlyEnd.Status
			== Edemo_mapShanmenFormationHostStatus::PlacementPending
			&& Fixture.Host.GetSession().GetState()
				== Edemo_mapShanmenFormationSessionState::Ended);
	TestTrue(TEXT("End removes every placed Actor through one terminal path"),
		SecondPlacement.IsSuccess() && LiveBeforeEnd == 2
			&& Ended.Status == Edemo_mapShanmenFormationHostStatus::Ended
			&& Ended.World.IsTeardownSuccess()
			&& Ended.World.TeardownReceipt.CommittedAnchorCount == 2
			&& Ended.World.TeardownReceipt.RemovedActorCount == 2
			&& CountTaggedActors(
				Fixture.World,
				FirstPlacement.World.PlacementReceipt.DeploymentTag) == 0);
	TestTrue(TEXT("Exact terminal call replays stable teardown evidence"),
		Replay.Status
			== Edemo_mapShanmenFormationHostStatus::TeardownReplayed
			&& Replay.World.TeardownReceipt.ReceiptId
				== Ended.World.TeardownReceipt.ReceiptId
			&& Fixture.Host.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationHostCancelBoundaryTest,
	"Shanmen.0_0_10.Product.FormationProductHost.CancelBoundaries",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationHostCancelBoundaryTest::RunTest(const FString&)
{
	FFormationHostFixture PreparedFixture;
	if (!PreparedFixture.Start(*this, TEXT("CancelPrepared")))
	{
		return false;
	}
	const Fdemo_mapShanmenFormationHostResult Prepared =
		PreparedFixture.Host.TryPrepareAnchor(
			*PreparedFixture.Authority, PreparedFixture.Correlation,
			HostAnchorA, HostAttemptA);
	const Fdemo_mapShanmenFormationHostResult CancelPrepared =
		PreparedFixture.Host.TryCancelAndTeardown(
			*PreparedFixture.Authority, PreparedFixture.World,
			PreparedFixture.Correlation);
	FShanmenItemAuthoritySnapshot AfterPreparedCancel;
	if (!PreparedFixture.CaptureSnapshot(AfterPreparedCancel))
	{
		return false;
	}
	TestTrue(TEXT("Pre-commit cancel releases reservation without consumption"),
		Prepared.IsSuccess() && CancelPrepared.IsSuccess()
			&& Prepared.Session.Material.Lines.Num() == 1
			&& CancelPrepared.Session.Material.IsCancelled()
			&& CancelPrepared.Session.Material.FinalizeReceipts.Num() == 1
			&& CancelPrepared.Session.Material.FinalizeReceipts[0].ResourceBefore
				== CancelPrepared.Session.Material.FinalizeReceipts[0].ResourceAfter
			&& PreparedFixture.Host.GetSession().GetState()
				== Edemo_mapShanmenFormationSessionState::Cancelled
			&& PreparedFixture.Host.IsValid());

	FFormationHostFixture CommittedFixture;
	Fdemo_mapShanmenFormationHostResult Committed;
	if (!CommittedFixture.Start(*this, TEXT("CancelCommitted"))
		|| !CommittedFixture.PrepareAndCommit(
			*this, HostAnchorA, HostAttemptOther, Committed))
	{
		return false;
	}
	FShanmenItemAuthoritySnapshot AfterDurableCommit;
	if (!CommittedFixture.CaptureSnapshot(AfterDurableCommit))
	{
		return false;
	}
	const Fdemo_mapShanmenFormationHostResult CancelCommitted =
		CommittedFixture.Host.TryCancelAndTeardown(
			*CommittedFixture.Authority, CommittedFixture.World,
			CommittedFixture.Correlation);
	const Fdemo_mapShanmenFormationHostResult CancelReplay =
		CommittedFixture.Host.TryCancelAndTeardown(
			*CommittedFixture.Authority, CommittedFixture.World,
			CommittedFixture.Correlation);
	FShanmenItemAuthoritySnapshot AfterCancelReplay;
	if (!CommittedFixture.CaptureSnapshot(AfterCancelReplay))
	{
		return false;
	}
	TestTrue(TEXT("Post-commit cancel is forward-only and never refunds material"),
		Committed.Session.Material.Lines.Num() == 1
			&& Committed.Session.Material.FinalizeReceipts.Num() == 1
			&& Committed.Session.Material.FinalizeReceipts[0].ResourceAfter
				== Committed.Session.Material.Lines[0].ExpectedQuantityBefore
					- Committed.Session.Material.Lines[0].Quantity
			&& AfterDurableCommit == AfterCancelReplay
			&& CancelCommitted.IsSuccess()
			&& !CommittedFixture.Host.HasPendingPlacement()
			&& CancelReplay.Status
				== Edemo_mapShanmenFormationHostStatus::TeardownReplayed
			&& CommittedFixture.Host.GetSession().GetState()
				== Edemo_mapShanmenFormationSessionState::Cancelled
			&& CommittedFixture.Host.IsValid());
	return true;
}

#endif
