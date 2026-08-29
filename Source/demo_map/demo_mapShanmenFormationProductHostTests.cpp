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
#include "Components/SceneComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "ShanmenWorldEntityRegistry.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	const FGuid HostAttemptA(0xF8400001, 0, 0, 1);
	const FGuid HostAttemptB(0xF8400002, 0, 0, 1);
	const FGuid HostAttemptOther(0xF8400003, 0, 0, 1);
	const FGuid HostAttemptC(0xF8400004, 0, 0, 1);
	const FGuid HostAttemptD(0xF8400005, 0, 0, 1);
	const FGuid HostSourceEntityId(0xF8400010, 0, 0, 1);
	const FGuid HostCoverageSubjectId(0xF8400011, 0, 0, 1);
	const FName HostAnchorA(TEXT("Formation.Anchor.ProductHost.A"));
	const FName HostAnchorB(TEXT("Formation.Anchor.ProductHost.B"));
	const FName HostAnchorC(TEXT("Formation.Anchor.ProductHost.C"));
	const FName HostAnchorD(TEXT("Formation.Anchor.ProductHost.D"));

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

	FShanmenFormationDiagramDefinition MakeHostCoverageDiagram()
	{
		FShanmenFormationDiagramCapture Capture;
		Capture.ActionDefinitionId =
			FShanmenFormationDiagramDefinition::CanonicalActionDefinitionId();
		Capture.DiagramDefinitionId =
			TEXT("Formation.Diagram.ProductHost.P8.10");
		const TArray<FName> AnchorIds = {
			HostAnchorA, HostAnchorB, HostAnchorC, HostAnchorD
		};
		const TArray<FVector> Offsets = {
			FVector(0.0, 0.0, 25.0),
			FVector(100.0, 0.0, 25.0),
			FVector(100.0, 100.0, 25.0),
			FVector(0.0, 100.0, 25.0)
		};
		for (int32 Index = 0; Index < AnchorIds.Num(); ++Index)
		{
			FShanmenFormationAnchorCapture& Anchor =
				Capture.Anchors.AddDefaulted_GetRef();
			Anchor.Order = Index;
			Anchor.AnchorDefinitionId = AnchorIds[Index];
			Anchor.RelativeOffset = Offsets[Index];
			FShanmenFormationMaterialRequirementCapture& Wood =
				Anchor.Requirements.AddDefaulted_GetRef();
			Wood.Order = 0;
			Wood.MaterialDefinitionId = Fdemo_mapItemIds::SpiritWoodLevel1;
			Wood.Quantity = 1;
		}
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

		bool Start(
			FAutomationTestBase& Test,
			const TCHAR* Label,
			const bool bUseCoverageDiagram = false)
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
					Correlation, Action,
					bUseCoverageDiagram
						? MakeHostCoverageDiagram()
						: MakeHostDiagram(),
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

		AActor* SpawnCoverageSubject(const FVector& Location) const
		{
			if (!World)
			{
				return nullptr;
			}
			FActorSpawnParameters Parameters;
			Parameters.ObjectFlags |= RF_Transient;
			Parameters.SpawnCollisionHandlingOverride =
				ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			AActor* Actor = World->SpawnActor<AActor>(
				AActor::StaticClass(), FTransform::Identity, Parameters);
			if (!Actor)
			{
				return nullptr;
			}
			USceneComponent* RootComponent = NewObject<USceneComponent>(
				Actor, TEXT("P810CoverageRoot"), RF_Transient);
			if (!RootComponent)
			{
				World->DestroyActor(Actor, true, true);
				return nullptr;
			}
			Actor->SetRootComponent(RootComponent);
			RootComponent->SetWorldLocation(Location);
			return Actor->GetActorLocation().Equals(
				Location, KINDA_SMALL_NUMBER) ? Actor : nullptr;
		}

		bool CommitAndPlaceCoverageDiagram(FAutomationTestBase& Test)
		{
			const TArray<FName> AnchorIds = {
				HostAnchorA, HostAnchorB, HostAnchorC, HostAnchorD
			};
			const TArray<FGuid> AttemptIds = {
				HostAttemptA, HostAttemptB, HostAttemptC, HostAttemptD
			};
			for (int32 Index = 0; Index < AnchorIds.Num(); ++Index)
			{
				Fdemo_mapShanmenFormationHostResult Commit;
				if (!PrepareAndCommit(
						Test, AnchorIds[Index], AttemptIds[Index], Commit))
				{
					return false;
				}
				const Fdemo_mapShanmenFormationHostResult Placement =
					Host.TryPlaceCommittedAnchor(
						World, ACharacter::StaticClass(), Correlation,
						AnchorIds[Index], AttemptIds[Index]);
				if (!Placement.IsSuccess())
				{
					Test.AddError(FString::Printf(
						TEXT("P8.10 placement failed: %s"),
						*Placement.Diagnostic));
					return false;
				}
			}
			return Host.GetSession().GetState()
				== Edemo_mapShanmenFormationSessionState::Active
				&& !Host.HasPendingPlacement();
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

	Fdemo_mapShanmenFormationInfluencePolicy MakeHostInfluencePolicy(
		const Fdemo_mapShanmenFormationProductHost& Host,
		const TCHAR* InfluenceId = TEXT("Formation.Influence.Test.HostWard"))
	{
		Fdemo_mapShanmenFormationInfluencePolicy Policy;
		Policy.PolicyDefinitionId = TEXT("Formation.Policy.Test.ProductHost");
		Policy.InfluenceDefinitionId = FName(InfluenceId);
		Policy.Content =
			Host.GetSession().GetActionRuntime().GetAction().GetContent();
		check(Policy.IsValid());
		return Policy;
	}

	Fdemo_mapShanmenFormationInfluenceAttemptCommand MakeHostInfluenceAttempt(
		const FGuid& IntentId,
		const int32 Ordinal,
		const Edemo_mapShanmenFormationInfluenceAttemptOutcome Outcome)
	{
		Fdemo_mapShanmenFormationInfluenceAttemptCommand Command;
		Command.IntentId = IntentId;
		Command.AttemptId = FGuid(0xF8410000 + Ordinal, 0, 0, 1);
		Command.ExecutorReceiptId =
			FGuid(0xF8420000 + Ordinal, 0, 0, 1);
		Command.Outcome = Outcome;
		check(Command.IsValid());
		return Command;
	}

	bool AcknowledgeNextHostInfluence(
		FAutomationTestBase& Test,
		FFormationHostFixture& Fixture,
		const int32 Ordinal,
		Fdemo_mapShanmenFormationInfluenceAttemptCommand* OutCommand = nullptr)
	{
		Fdemo_mapShanmenFormationInfluenceIntent Intent;
		if (!Fixture.Host.TryPeekNextInfluenceIntent(Intent))
		{
			Test.AddError(TEXT("P8.14 expected one pending influence intent."));
			return false;
		}
		const auto Command = MakeHostInfluenceAttempt(
			Intent.IntentId, Ordinal,
			Edemo_mapShanmenFormationInfluenceAttemptOutcome::Succeeded);
		const auto Result = Fixture.Host.TryAcknowledgeInfluence(
			Fixture.Correlation, Command);
		if (!Result.IsSuccess())
		{
			Test.AddError(FString::Printf(
				TEXT("P8.14 acknowledgement failed: %s"),
				*Result.Diagnostic));
			return false;
		}
		if (OutCommand)
		{
			*OutCommand = Command;
		}
		return true;
	}
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationHostCoverageOwnershipTest,
	"Shanmen.0_0_10.Product.FormationProductHost.CoverageOwnership",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationHostCoverageOwnershipTest::RunTest(const FString&)
{
	FFormationHostFixture Fixture;
	if (!Fixture.Start(*this, TEXT("CoverageOwnership"), true)
		|| !Fixture.CommitAndPlaceCoverageDiagram(*this))
	{
		return false;
	}

	FShanmenWorldEntityRegistry Registry;
	AActor* Subject = Fixture.SpawnCoverageSubject(
		FVector(50.0, 50.0, 900.0));
	if (!Subject
		|| !Registry.TryBeginRun(Fixture.Correlation.ActiveRunId)
		|| Registry.BindObject(
			Fixture.Correlation.ActiveRunId, Subject,
			HostCoverageSubjectId, INDEX_NONE)
			!= EShanmenWorldBindingResult::Bound)
	{
		AddError(TEXT("Could not bind the P8.10 coverage subject."));
		return false;
	}

	const Fdemo_mapShanmenFormationHostCoverageResult Prime =
		Fixture.Host.TryCoordinateCoverage(
			Fixture.World, Registry, { Subject }, Fixture.Correlation,
			Fdemo_mapShanmenFormationCoverageCommand::MakePrime());
	Fdemo_mapShanmenFormationCoverageReceipt PrimeBaseline;
	const bool bReadPrime =
		Fixture.Host.TryGetCoverageBaseline(PrimeBaseline);
	Subject->SetActorLocation(
		FVector(150.0, 50.0, 900.0), false, nullptr,
		ETeleportType::TeleportPhysics);
	const Fdemo_mapShanmenFormationCoverageCommand AdvanceCommand =
		Fdemo_mapShanmenFormationCoverageCommand::MakeAdvance(
			PrimeBaseline.ReceiptId);
	const Fdemo_mapShanmenFormationHostCoverageResult Advance =
		Fixture.Host.TryCoordinateCoverage(
			Fixture.World, Registry, { Subject }, Fixture.Correlation,
			AdvanceCommand);
	const Fdemo_mapShanmenFormationHostCoverageResult Replay =
		Fixture.Host.TryCoordinateCoverage(
			Fixture.World, Registry, { Subject }, Fixture.Correlation,
			AdvanceCommand);

	TestTrue(TEXT("Host primes only from its internally rebuilt deployment area"),
		Prime.IsSuccess()
			&& Prime.Status
				== Edemo_mapShanmenFormationHostCoverageStatus::Coordinated
			&& Prime.Area.Area.Anchors.Num() == 4
			&& Prime.Area.Area.DeploymentId
				== Fixture.Host.GetSession().GetDeployment().GetDeploymentId()
			&& Prime.Coordination.TrackerResult.Status
				== Edemo_mapShanmenFormationCoverageTrackerStatus::Primed
			&& bReadPrime && Fixture.Host.HasCoverageBaseline());
	TestTrue(TEXT("Owned tracker advances once and preserves transition evidence"),
		Advance.IsSuccess()
			&& Advance.Coordination.TrackerResult.Status
				== Edemo_mapShanmenFormationCoverageTrackerStatus::Advanced
			&& Advance.Coordination.TrackerResult.Transition.IsSuccess()
			&& Advance.Coordination.TrackerResult.Transition.Receipt.LeftCount
				== 1);
	TestTrue(TEXT("Exact host command replays without a second baseline mutation"),
		Replay.IsSuccess()
			&& Replay.Coordination.TrackerResult.Status
				== Edemo_mapShanmenFormationCoverageTrackerStatus::AdvanceReplayed
			&& Replay.Coordination.TrackerResult.CurrentBaselineReceiptId
				== Advance.Coordination.TrackerResult.CurrentBaselineReceiptId);

	const Fdemo_mapShanmenFormationHostCoverageResult Reset =
		Fixture.Host.TryResetCoverage(Fixture.Correlation);
	const Fdemo_mapShanmenFormationHostCoverageResult ResetReplay =
		Fixture.Host.TryResetCoverage(Fixture.Correlation);
	TestTrue(TEXT("Explicit reset clears the sole baseline and replays safely"),
		Reset.IsSuccess()
			&& Reset.Status
				== Edemo_mapShanmenFormationHostCoverageStatus::Reset
			&& ResetReplay.IsSuccess()
			&& ResetReplay.Status
				== Edemo_mapShanmenFormationHostCoverageStatus::ResetReplayed
			&& !Fixture.Host.HasCoverageBaseline()
			&& Fixture.Host.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationHostCoverageLifecycleTest,
	"Shanmen.0_0_10.Product.FormationProductHost.CoverageLifecycleFences",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationHostCoverageLifecycleTest::RunTest(const FString&)
{
	FFormationHostFixture Fixture;
	if (!Fixture.Start(*this, TEXT("CoverageLifecycle"), true))
	{
		return false;
	}
	FShanmenWorldEntityRegistry Registry;
	AActor* Subject = Fixture.SpawnCoverageSubject(
		FVector(50.0, 50.0, 0.0));
	if (!Subject
		|| !Registry.TryBeginRun(Fixture.Correlation.ActiveRunId)
		|| Registry.BindObject(
			Fixture.Correlation.ActiveRunId, Subject,
			HostCoverageSubjectId, INDEX_NONE)
			!= EShanmenWorldBindingResult::Bound)
	{
		return false;
	}

	const Fdemo_mapShanmenFormationHostCoverageResult BeforeActive =
		Fixture.Host.TryCoordinateCoverage(
			Fixture.World, Registry, { Subject }, Fixture.Correlation,
			Fdemo_mapShanmenFormationCoverageCommand::MakePrime());
	if (!Fixture.CommitAndPlaceCoverageDiagram(*this))
	{
		return false;
	}
	Fdemo_mapShanmenRunCorrelation Foreign = Fixture.Correlation;
	Foreign.OwnerId = FGuid(0xF8409999, 0, 0, 1);
	const Fdemo_mapShanmenFormationHostCoverageResult WrongCorrelation =
		Fixture.Host.TryCoordinateCoverage(
			Fixture.World, Registry, { Subject }, Foreign,
			Fdemo_mapShanmenFormationCoverageCommand::MakePrime());
	const Fdemo_mapShanmenFormationHostCoverageResult WrongWorld =
		Fixture.Host.TryCoordinateCoverage(
			nullptr, Registry, { Subject }, Fixture.Correlation,
			Fdemo_mapShanmenFormationCoverageCommand::MakePrime());
	const Fdemo_mapShanmenFormationHostCoverageResult Prime =
		Fixture.Host.TryCoordinateCoverage(
			Fixture.World, Registry, { Subject }, Fixture.Correlation,
			Fdemo_mapShanmenFormationCoverageCommand::MakePrime());

	TestTrue(TEXT("Coverage is fenced by active deployment and exact host scope"),
		BeforeActive.Status
			== Edemo_mapShanmenFormationHostCoverageStatus::SessionNotActive
			&& WrongCorrelation.Status
				== Edemo_mapShanmenFormationHostCoverageStatus::CorrelationMismatch
			&& WrongWorld.Status
				== Edemo_mapShanmenFormationHostCoverageStatus::WorldMismatch
			&& Prime.IsSuccess() && Fixture.Host.HasCoverageBaseline());

	const Fdemo_mapShanmenFormationHostResult Ended =
		Fixture.Host.TryEndAndTeardown(
			Fixture.World, Fixture.Correlation);
	const Fdemo_mapShanmenFormationHostCoverageResult AfterTerminal =
		Fixture.Host.TryCoordinateCoverage(
			Fixture.World, Registry, { Subject }, Fixture.Correlation,
			Fdemo_mapShanmenFormationCoverageCommand::MakePrime());
	const Fdemo_mapShanmenFormationHostCoverageResult TerminalReset =
		Fixture.Host.TryResetCoverage(Fixture.Correlation);
	const Fdemo_mapShanmenFormationHostResult EndReplay =
		Fixture.Host.TryEndAndTeardown(
			Fixture.World, Fixture.Correlation);
	TestTrue(TEXT("Successful terminal teardown atomically clears coverage authority"),
		Ended.IsSuccess() && !Fixture.Host.HasCoverageBaseline()
			&& AfterTerminal.Status
				== Edemo_mapShanmenFormationHostCoverageStatus::SessionTerminal
			&& TerminalReset.Status
				== Edemo_mapShanmenFormationHostCoverageStatus::SessionTerminal
			&& EndReplay.Status
				== Edemo_mapShanmenFormationHostStatus::TeardownReplayed
			&& Fixture.Host.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationHostInfluencePrimeAdvanceTest,
	"Shanmen.0_0_10.Product.FormationInfluenceHost.PrimeAdvanceReplay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationHostInfluencePrimeAdvanceTest::RunTest(
	const FString&)
{
	FFormationHostFixture Fixture;
	if (!Fixture.Start(*this, TEXT("InfluencePrimeAdvance"), true)
		|| !Fixture.CommitAndPlaceCoverageDiagram(*this))
	{
		return false;
	}
	FShanmenWorldEntityRegistry Registry;
	AActor* Subject = Fixture.SpawnCoverageSubject(
		FVector(50.0, 50.0, 900.0));
	if (!Subject
		|| !Registry.TryBeginRun(Fixture.Correlation.ActiveRunId)
		|| Registry.BindObject(
			Fixture.Correlation.ActiveRunId, Subject,
			HostCoverageSubjectId, INDEX_NONE)
			!= EShanmenWorldBindingResult::Bound)
	{
		return false;
	}
	const auto Policy = MakeHostInfluencePolicy(Fixture.Host);
	const auto Prime = Fixture.Host.TryCoordinateInfluence(
		Fixture.World, Registry, { Subject }, Fixture.Correlation,
		Fdemo_mapShanmenFormationCoverageCommand::MakePrime(), Policy);
	const auto PrimeReplay = Fixture.Host.TryCoordinateInfluence(
		Fixture.World, Registry, { Subject }, Fixture.Correlation,
		Fdemo_mapShanmenFormationCoverageCommand::MakePrime(), Policy);
	Fdemo_mapShanmenFormationCoverageReceipt PrimeBaseline;
	const bool bReadPrime =
		Fixture.Host.TryGetCoverageBaseline(PrimeBaseline);
	if (!AcknowledgeNextHostInfluence(*this, Fixture, 1))
	{
		return false;
	}
	Subject->SetActorLocation(
		FVector(150.0, 50.0, 900.0), false, nullptr,
		ETeleportType::TeleportPhysics);
	const auto AdvanceCommand =
		Fdemo_mapShanmenFormationCoverageCommand::MakeAdvance(
			PrimeBaseline.ReceiptId);
	const auto Advance = Fixture.Host.TryCoordinateInfluence(
		Fixture.World, Registry, { Subject }, Fixture.Correlation,
		AdvanceCommand, Policy);
	const auto AdvanceReplay = Fixture.Host.TryCoordinateInfluence(
		Fixture.World, Registry, { Subject }, Fixture.Correlation,
		AdvanceCommand, Policy);
	const auto RawCoverage = Fixture.Host.TryCoordinateCoverage(
		Fixture.World, Registry, { Subject }, Fixture.Correlation,
		AdvanceCommand);

	TestTrue(TEXT("Prime publishes and owns one Apply intent"),
		Prime.IsSuccess()
			&& Prime.Status
				== Edemo_mapShanmenFormationHostInfluenceStatus::Coordinated
			&& Prime.ReconciliationPlan.Batch.ApplyCount == 1
			&& Prime.ReconciliationPlan.Batch.RemoveCount == 0
			&& Prime.Dispatch.PendingIntentCount == 1
			&& Fixture.Host.HasInfluenceAuthority());
	TestTrue(TEXT("Exact Prime replays without another dispatch batch"),
		PrimeReplay.IsSuccess()
			&& PrimeReplay.Status
				== Edemo_mapShanmenFormationHostInfluenceStatus::CoordinateReplayed
			&& PrimeReplay.Dispatch.BatchRecordId
				== Prime.Dispatch.BatchRecordId);
	TestTrue(TEXT("Advance emits the exact Left transition Remove"),
		bReadPrime && Advance.IsSuccess()
			&& Advance.TransitionPlan.Batch.ApplyCount == 0
			&& Advance.TransitionPlan.Batch.RemoveCount == 1
			&& Advance.TransitionPlan.Batch.Intents[0].SubjectEntityId
				== HostCoverageSubjectId
			&& Fixture.Host.GetPendingInfluenceIntentCount() == 1);
	TestTrue(TEXT("Exact Advance replays without queue duplication"),
		AdvanceReplay.IsSuccess()
			&& AdvanceReplay.Status
				== Edemo_mapShanmenFormationHostInfluenceStatus::CoordinateReplayed
			&& Fixture.Host.GetInfluenceLedger().GetAcceptedBatchCount() == 2);
	TestTrue(TEXT("Raw coverage cannot bypass established influence authority"),
		RawCoverage.Status
			== Edemo_mapShanmenFormationHostCoverageStatus::
				InfluenceOrchestrationRequired
			&& Fixture.Host.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationHostInfluenceRebaseTest,
	"Shanmen.0_0_10.Product.FormationInfluenceHost.PolicyRebase",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationHostInfluenceRebaseTest::RunTest(const FString&)
{
	FFormationHostFixture Fixture;
	if (!Fixture.Start(*this, TEXT("InfluenceRebase"), true)
		|| !Fixture.CommitAndPlaceCoverageDiagram(*this))
	{
		return false;
	}
	FShanmenWorldEntityRegistry Registry;
	AActor* Subject = Fixture.SpawnCoverageSubject(
		FVector(50.0, 50.0, 900.0));
	if (!Subject
		|| !Registry.TryBeginRun(Fixture.Correlation.ActiveRunId)
		|| Registry.BindObject(
			Fixture.Correlation.ActiveRunId, Subject,
			HostCoverageSubjectId, INDEX_NONE)
			!= EShanmenWorldBindingResult::Bound)
	{
		return false;
	}
	const auto FirstPolicy = MakeHostInfluencePolicy(Fixture.Host);
	const auto Prime = Fixture.Host.TryCoordinateInfluence(
		Fixture.World, Registry, { Subject }, Fixture.Correlation,
		Fdemo_mapShanmenFormationCoverageCommand::MakePrime(), FirstPolicy);
	Fdemo_mapShanmenFormationCoverageReceipt Baseline;
	if (!Prime.IsSuccess()
		|| !Fixture.Host.TryGetCoverageBaseline(Baseline)
		|| !AcknowledgeNextHostInfluence(*this, Fixture, 20))
	{
		return false;
	}
	const auto ReplacementPolicy = MakeHostInfluencePolicy(
		Fixture.Host, TEXT("Formation.Influence.Test.HostSpiritShield"));
	const auto Rebase = Fixture.Host.TryCoordinateInfluence(
		Fixture.World, Registry, { Subject }, Fixture.Correlation,
		Fdemo_mapShanmenFormationCoverageCommand::MakeRebase(
			Baseline.ReceiptId),
		ReplacementPolicy);
	Fdemo_mapShanmenFormationCoverageReceipt RebasedBaseline;
	const bool bReadRebased =
		Fixture.Host.TryGetCoverageBaseline(RebasedBaseline);
	const int32 BatchCountAfterRebase =
		Fixture.Host.GetInfluenceLedger().GetAcceptedBatchCount();
	const auto PolicyDriftAdvance = Fixture.Host.TryCoordinateInfluence(
		Fixture.World, Registry, { Subject }, Fixture.Correlation,
		Fdemo_mapShanmenFormationCoverageCommand::MakeAdvance(
			RebasedBaseline.ReceiptId),
		FirstPolicy);
	Fdemo_mapShanmenFormationCoverageReceipt AfterRejectedAdvance;
	const bool bReadAfterReject =
		Fixture.Host.TryGetCoverageBaseline(AfterRejectedAdvance);

	TestTrue(TEXT("Policy replacement uses explicit Rebase reconciliation"),
		Rebase.IsSuccess()
			&& Rebase.ReconciliationPlan.Batch.Mode
				== Edemo_mapShanmenFormationInfluenceReconciliationMode::Rebase
			&& Rebase.ReconciliationPlan.Batch.RemoveCount == 1
			&& Rebase.ReconciliationPlan.Batch.ApplyCount == 1
			&& Rebase.Dispatch.PendingIntentCount == 2
			&& bReadRebased);
	TestTrue(TEXT("Old policy cannot drift through Advance"),
		PolicyDriftAdvance.Status
			== Edemo_mapShanmenFormationHostInfluenceStatus::LifecycleConflict
			&& bReadAfterReject
			&& AfterRejectedAdvance.ReceiptId == RebasedBaseline.ReceiptId
			&& Fixture.Host.GetInfluenceLedger().GetAcceptedBatchCount()
				== BatchCountAfterRebase
			&& Fixture.Host.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationHostInfluenceResetTest,
	"Shanmen.0_0_10.Product.FormationInfluenceHost.RetryResetSeal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationHostInfluenceResetTest::RunTest(const FString&)
{
	FFormationHostFixture Fixture;
	if (!Fixture.Start(*this, TEXT("InfluenceReset"), true)
		|| !Fixture.CommitAndPlaceCoverageDiagram(*this))
	{
		return false;
	}
	FShanmenWorldEntityRegistry Registry;
	AActor* Subject = Fixture.SpawnCoverageSubject(
		FVector(50.0, 50.0, 900.0));
	if (!Subject
		|| !Registry.TryBeginRun(Fixture.Correlation.ActiveRunId)
		|| Registry.BindObject(
			Fixture.Correlation.ActiveRunId, Subject,
			HostCoverageSubjectId, INDEX_NONE)
			!= EShanmenWorldBindingResult::Bound)
	{
		return false;
	}
	const auto Prime = Fixture.Host.TryCoordinateInfluence(
		Fixture.World, Registry, { Subject }, Fixture.Correlation,
		Fdemo_mapShanmenFormationCoverageCommand::MakePrime(),
		MakeHostInfluencePolicy(Fixture.Host));
	Fdemo_mapShanmenFormationInfluenceIntent ApplyIntent;
	if (!Prime.IsSuccess()
		|| !Fixture.Host.TryPeekNextInfluenceIntent(ApplyIntent))
	{
		return false;
	}
	const auto RetryCommand = MakeHostInfluenceAttempt(
		ApplyIntent.IntentId, 30,
		Edemo_mapShanmenFormationInfluenceAttemptOutcome::RetryableFailure);
	const auto Retry = Fixture.Host.TryAcknowledgeInfluence(
		Fixture.Correlation, RetryCommand);
	const auto RetryReplay = Fixture.Host.TryAcknowledgeInfluence(
		Fixture.Correlation, RetryCommand);
	const auto SuccessCommand = MakeHostInfluenceAttempt(
		ApplyIntent.IntentId, 31,
		Edemo_mapShanmenFormationInfluenceAttemptOutcome::Succeeded);
	const auto Success = Fixture.Host.TryAcknowledgeInfluence(
		Fixture.Correlation, SuccessCommand);
	const auto Reset = Fixture.Host.TryResetInfluence(Fixture.Correlation);
	const auto ResetReplay =
		Fixture.Host.TryResetInfluence(Fixture.Correlation);
	if (!AcknowledgeNextHostInfluence(*this, Fixture, 32))
	{
		return false;
	}
	const auto Seal = Fixture.Host.TrySealInfluence(Fixture.Correlation);
	const auto SealReplay =
		Fixture.Host.TrySealInfluence(Fixture.Correlation);
	const auto Ended = Fixture.Host.TryEndAndTeardown(
		Fixture.World, Fixture.Correlation);

	TestTrue(TEXT("Retry is retained and exact retry replays"),
		Retry.Status
			== Edemo_mapShanmenFormationHostInfluenceStatus::RetryRecorded
			&& RetryReplay.Status
				== Edemo_mapShanmenFormationHostInfluenceStatus::RetryReplayed
			&& Retry.Acknowledgement.PendingIntentCount == 1);
	TestTrue(TEXT("Later success drains the Apply intent"),
		Success.Status
			== Edemo_mapShanmenFormationHostInfluenceStatus::Acknowledged
			&& Success.Acknowledgement.PendingIntentCount == 0);
	TestTrue(TEXT("Reset publishes one Remove and exact replay is idempotent"),
		Reset.IsSuccess()
			&& Reset.Status
				== Edemo_mapShanmenFormationHostInfluenceStatus::Reset
			&& Reset.ReconciliationPlan.Batch.RemoveCount == 1
			&& ResetReplay.Status
				== Edemo_mapShanmenFormationHostInfluenceStatus::ResetReplayed
			&& !Fixture.Host.HasActiveInfluenceScope()
			&& !Fixture.Host.HasCoverageBaseline());
	TestTrue(TEXT("Drained Reset ledger seals and permits terminal teardown"),
		Seal.Status == Edemo_mapShanmenFormationHostInfluenceStatus::Sealed
			&& SealReplay.Status
				== Edemo_mapShanmenFormationHostInfluenceStatus::SealReplayed
			&& Ended.Status == Edemo_mapShanmenFormationHostStatus::Ended
			&& Fixture.Host.GetInfluenceLedger().IsSealed()
			&& Fixture.Host.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationHostInfluenceTerminalTest,
	"Shanmen.0_0_10.Product.FormationInfluenceHost.TerminalDrainGate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationHostInfluenceTerminalTest::RunTest(const FString&)
{
	FFormationHostFixture Fixture;
	if (!Fixture.Start(*this, TEXT("InfluenceTerminal"), true)
		|| !Fixture.CommitAndPlaceCoverageDiagram(*this))
	{
		return false;
	}
	FShanmenWorldEntityRegistry Registry;
	AActor* Subject = Fixture.SpawnCoverageSubject(
		FVector(50.0, 50.0, 900.0));
	if (!Subject
		|| !Registry.TryBeginRun(Fixture.Correlation.ActiveRunId)
		|| Registry.BindObject(
			Fixture.Correlation.ActiveRunId, Subject,
			HostCoverageSubjectId, INDEX_NONE)
			!= EShanmenWorldBindingResult::Bound)
	{
		return false;
	}
	const auto Prime = Fixture.Host.TryCoordinateInfluence(
		Fixture.World, Registry, { Subject }, Fixture.Correlation,
		Fdemo_mapShanmenFormationCoverageCommand::MakePrime(),
		MakeHostInfluencePolicy(Fixture.Host));
	if (!Prime.IsSuccess()
		|| !AcknowledgeNextHostInfluence(*this, Fixture, 40))
	{
		return false;
	}
	const auto BeforePrepare = Fixture.Host.TryEndAndTeardown(
		Fixture.World, Fixture.Correlation);
	const bool bStayedActiveBeforePrepare =
		Fixture.Host.GetSession().GetState()
			== Edemo_mapShanmenFormationSessionState::Active;
	const auto Prepared =
		Fixture.Host.TryPrepareTerminalInfluence(Fixture.Correlation);
	const auto PreparedReplay =
		Fixture.Host.TryPrepareTerminalInfluence(Fixture.Correlation);
	const auto BeforeDrain = Fixture.Host.TryEndAndTeardown(
		Fixture.World, Fixture.Correlation);
	Fdemo_mapShanmenFormationInfluenceAttemptCommand RemoveCommand;
	if (!AcknowledgeNextHostInfluence(
			*this, Fixture, 41, &RemoveCommand))
	{
		return false;
	}
	const auto BeforeSeal = Fixture.Host.TryEndAndTeardown(
		Fixture.World, Fixture.Correlation);
	const auto Seal = Fixture.Host.TrySealInfluence(Fixture.Correlation);
	const auto Ended = Fixture.Host.TryEndAndTeardown(
		Fixture.World, Fixture.Correlation);
	const auto TerminalReplay =
		Fixture.Host.TryPrepareTerminalInfluence(Fixture.Correlation);
	const auto AcknowledgementReplay =
		Fixture.Host.TryAcknowledgeInfluence(
			Fixture.Correlation, RemoveCommand);
	const auto EndReplay = Fixture.Host.TryEndAndTeardown(
		Fixture.World, Fixture.Correlation);

	TestTrue(TEXT("Active influence blocks teardown before Terminal planning"),
		BeforePrepare.Status
			== Edemo_mapShanmenFormationHostStatus::InfluenceTerminalRequired
			&& bStayedActiveBeforePrepare);
	TestTrue(TEXT("Terminal plan emits one Remove and exact plan replays"),
		Prepared.IsSuccess()
			&& Prepared.ReconciliationPlan.Batch.RemoveCount == 1
			&& PreparedReplay.Status
				== Edemo_mapShanmenFormationHostInfluenceStatus::TerminalReplayed
			&& BeforeDrain.Status
				== Edemo_mapShanmenFormationHostStatus::InfluenceTerminalRequired);
	TestTrue(TEXT("Successful removals still require an explicit ledger seal"),
		BeforeSeal.Status
			== Edemo_mapShanmenFormationHostStatus::InfluenceTerminalRequired
			&& Seal.Status
				== Edemo_mapShanmenFormationHostInfluenceStatus::Sealed);
	TestTrue(TEXT("Sealed terminal evidence permits teardown and all exact replay"),
		Ended.Status == Edemo_mapShanmenFormationHostStatus::Ended
			&& TerminalReplay.Status
				== Edemo_mapShanmenFormationHostInfluenceStatus::TerminalReplayed
			&& AcknowledgementReplay.Status
				== Edemo_mapShanmenFormationHostInfluenceStatus::
					AcknowledgementReplayed
			&& EndReplay.Status
				== Edemo_mapShanmenFormationHostStatus::TeardownReplayed
			&& !Fixture.Host.HasActiveInfluenceScope()
			&& !Fixture.Host.IsInfluenceTerminalPrepared()
			&& Fixture.Host.IsValid());
	return true;
}

#endif
