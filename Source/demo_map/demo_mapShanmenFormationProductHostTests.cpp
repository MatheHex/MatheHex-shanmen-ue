#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenFormationProductHost.h"
#include "demo_mapShanmenFormationInfluenceExecutorAdapter.h"
#include "demo_mapShanmenFormationInfluenceLeaseExecutor.h"

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

	Fdemo_mapShanmenFormationInfluenceExecutionCommand
	MakeHostExecutionCommand(
		const FGuid& IntentId,
		const int32 Ordinal)
	{
		Fdemo_mapShanmenFormationInfluenceExecutionCommand Command;
		Command.IntentId = IntentId;
		Command.AttemptId = FGuid(0xF8510000 + Ordinal, 0, 0, 1);
		check(Command.IsValid());
		return Command;
	}

	class FScriptedHostInfluenceExecutor final
		: public Idemo_mapShanmenFormationInfluenceExecutor
	{
	public:
		TArray<Edemo_mapShanmenFormationInfluenceAttemptOutcome> Outcomes;
		int32 InvocationCount = 0;
		bool bRejectNext = false;
		bool bCorruptNextIntent = false;

		virtual Fdemo_mapShanmenFormationInfluenceExecutorResult Execute(
			const Fdemo_mapShanmenFormationInfluenceExecutorInvocation& Invocation)
			override
		{
			++InvocationCount;
			Fdemo_mapShanmenFormationInfluenceExecutorResult Result;
			if (bRejectNext)
			{
				bRejectNext = false;
				Result.Status =
					Edemo_mapShanmenFormationInfluenceExecutorStatus::Rejected;
				Result.Diagnostic = TEXT("Scripted executor rejection.");
				return Result;
			}
			Result.Status =
				Edemo_mapShanmenFormationInfluenceExecutorStatus::Completed;
			Result.Diagnostic = TEXT("Scripted opaque execution receipt.");
			Result.Receipt.LedgerId = Invocation.LedgerId;
			Result.Receipt.IntentId = Invocation.Intent.IntentId;
			Result.Receipt.AttemptId = Invocation.AttemptId;
			Result.Receipt.ExecutorReceiptId =
				FGuid(0xF8520000 + InvocationCount, 0, 0, 1);
			Result.Receipt.Outcome = Outcomes.IsValidIndex(
					InvocationCount - 1)
				? Outcomes[InvocationCount - 1]
				: Edemo_mapShanmenFormationInfluenceAttemptOutcome::Succeeded;
			if (bCorruptNextIntent)
			{
				bCorruptNextIntent = false;
				Result.Receipt.IntentId = FGuid(0xF852FFFF, 0, 0, 1);
			}
			return Result;
		}
	};

	bool PrimeHostExecutorInfluence(
		FAutomationTestBase& Test,
		FFormationHostFixture& Fixture,
		FShanmenWorldEntityRegistry& Registry,
		const TCHAR* Label,
		const int32 SubjectCount,
		Fdemo_mapShanmenFormationHostInfluenceResult& OutPrime,
		TArray<AActor*>& OutSubjects)
	{
		OutSubjects.Reset();
		if (SubjectCount <= 0
			|| !Fixture.Start(Test, Label, true)
			|| !Fixture.CommitAndPlaceCoverageDiagram(Test)
			|| !Registry.TryBeginRun(Fixture.Correlation.ActiveRunId))
		{
			return false;
		}
		for (int32 Index = 0; Index < SubjectCount; ++Index)
		{
			AActor* Subject = Fixture.SpawnCoverageSubject(
				FVector(25.0 + Index * 25.0, 25.0, 900.0));
			const FGuid SubjectId(0xF8530000 + Index, 0, 0, 1);
			if (!Subject
				|| Registry.BindObject(
					Fixture.Correlation.ActiveRunId, Subject,
					SubjectId, INDEX_NONE)
					!= EShanmenWorldBindingResult::Bound)
			{
				return false;
			}
			OutSubjects.Add(Subject);
		}
		OutPrime = Fixture.Host.TryCoordinateInfluence(
			Fixture.World, Registry, OutSubjects, Fixture.Correlation,
			Fdemo_mapShanmenFormationCoverageCommand::MakePrime(),
			MakeHostInfluencePolicy(Fixture.Host));
		return OutPrime.IsSuccess()
			&& OutPrime.ReconciliationPlan.Batch.Intents.Num()
				== SubjectCount;
	}

	Fdemo_mapShanmenFormationInfluenceExecutorInvocation MakeLeaseInvocation(
		const Fdemo_mapShanmenFormationProductHost& Host,
		const Fdemo_mapShanmenFormationInfluenceIntent& Intent,
		const int32 Ordinal)
	{
		Fdemo_mapShanmenFormationInfluenceExecutorInvocation Invocation;
		Invocation.LedgerId = Host.GetInfluenceLedger().GetLedgerId();
		Invocation.Intent = Intent;
		Invocation.AttemptId = FGuid(0xF8610000 + Ordinal, 0, 0, 1);
		check(Invocation.IsValid());
		return Invocation;
	}

	Fdemo_mapShanmenFormationInfluenceIntent MakeLeaseIntent(
		const Fdemo_mapShanmenFormationInfluenceScope& Scope,
		const FGuid& SourceEntityId,
		const FGuid& SubjectEntityId,
		const Edemo_mapShanmenFormationInfluenceOperation Operation,
		const int32 CauseOrdinal)
	{
		const auto Intent = Fdemo_mapShanmenFormationInfluenceIntent::Make(
			Scope.Area, SourceEntityId, Scope.Policy, SubjectEntityId,
			Operation, FGuid(0xF8620000 + CauseOrdinal, 0, 0, 1));
		check(Intent.IsValid());
		return Intent;
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationInfluenceExecutorSuccessReplayTest,
	"Shanmen.0_0_10.Product.FormationInfluenceExecutor.SuccessReplay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationInfluenceExecutorSuccessReplayTest::RunTest(
	const FString&)
{
	FFormationHostFixture Fixture;
	FShanmenWorldEntityRegistry Registry;
	Fdemo_mapShanmenFormationHostInfluenceResult Prime;
	TArray<AActor*> Subjects;
	if (!PrimeHostExecutorInfluence(
			*this, Fixture, Registry, TEXT("ExecutorSuccessReplay"),
			1, Prime, Subjects))
	{
		return false;
	}
	const auto Command = MakeHostExecutionCommand(
		Prime.ReconciliationPlan.Batch.Intents[0].IntentId, 1);
	FScriptedHostInfluenceExecutor Executor;
	const auto First =
		Fdemo_mapShanmenFormationInfluenceExecutorAdapter::TryExecute(
			Fixture.Host, Fixture.Correlation, Command, Executor);
	const auto Replay =
		Fdemo_mapShanmenFormationInfluenceExecutorAdapter::TryExecute(
			Fixture.Host, Fixture.Correlation, Command, Executor);
	Fdemo_mapShanmenFormationInfluenceAttemptReceipt Stored;

	TestTrue(TEXT("Successful execution acknowledges the canonical intent"),
		First.IsSuccess()
			&& First.Status
				== Edemo_mapShanmenFormationInfluenceExecutionStatus::Succeeded
			&& First.bExecutorInvoked
			&& Fixture.Host.GetPendingInfluenceIntentCount() == 0);
	TestTrue(TEXT("Exact success replay does not invoke the executor again"),
		Replay.IsSuccess()
			&& Replay.Status
				== Edemo_mapShanmenFormationInfluenceExecutionStatus::
					AttemptReplayed
			&& !Replay.bExecutorInvoked
			&& Executor.InvocationCount == 1
			&& Replay.Executor.Receipt.ExecutorReceiptId
				== First.Executor.Receipt.ExecutorReceiptId);
	TestTrue(TEXT("Ledger exposes the exact immutable attempt evidence"),
		Fixture.Host.GetInfluenceLedger().TryGetAttemptReceipt(
			Command.IntentId, Command.AttemptId, Stored)
			&& Stored.ExecutorReceiptId
				== First.Executor.Receipt.ExecutorReceiptId
			&& Fixture.Host.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationInfluenceExecutorRetryTest,
	"Shanmen.0_0_10.Product.FormationInfluenceExecutor.RetryThenSuccess",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationInfluenceExecutorRetryTest::RunTest(const FString&)
{
	FFormationHostFixture Fixture;
	FShanmenWorldEntityRegistry Registry;
	Fdemo_mapShanmenFormationHostInfluenceResult Prime;
	TArray<AActor*> Subjects;
	if (!PrimeHostExecutorInfluence(
			*this, Fixture, Registry, TEXT("ExecutorRetry"),
			1, Prime, Subjects))
	{
		return false;
	}
	const FGuid IntentId =
		Prime.ReconciliationPlan.Batch.Intents[0].IntentId;
	const auto RetryCommand = MakeHostExecutionCommand(IntentId, 10);
	const auto SuccessCommand = MakeHostExecutionCommand(IntentId, 11);
	FScriptedHostInfluenceExecutor Executor;
	Executor.Outcomes = {
		Edemo_mapShanmenFormationInfluenceAttemptOutcome::RetryableFailure,
		Edemo_mapShanmenFormationInfluenceAttemptOutcome::Succeeded
	};
	const auto Retry =
		Fdemo_mapShanmenFormationInfluenceExecutorAdapter::TryExecute(
			Fixture.Host, Fixture.Correlation, RetryCommand, Executor);
	const auto RetryReplay =
		Fdemo_mapShanmenFormationInfluenceExecutorAdapter::TryExecute(
			Fixture.Host, Fixture.Correlation, RetryCommand, Executor);
	const auto Success =
		Fdemo_mapShanmenFormationInfluenceExecutorAdapter::TryExecute(
			Fixture.Host, Fixture.Correlation, SuccessCommand, Executor);

	TestTrue(TEXT("Retry remains pending and exact retry is executor-free"),
		Retry.IsSuccess()
			&& Retry.Status
				== Edemo_mapShanmenFormationInfluenceExecutionStatus::RetryRecorded
			&& RetryReplay.Status
				== Edemo_mapShanmenFormationInfluenceExecutionStatus::
					AttemptReplayed
			&& !RetryReplay.bExecutorInvoked
			&& Retry.HostAcknowledgement.Acknowledgement.PendingIntentCount == 1);
	TestTrue(TEXT("A new attempt can later complete the same pending intent"),
		Success.IsSuccess()
			&& Success.Status
				== Edemo_mapShanmenFormationInfluenceExecutionStatus::Succeeded
			&& Executor.InvocationCount == 2
			&& Fixture.Host.GetPendingInfluenceIntentCount() == 0
			&& Fixture.Host.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationInfluenceExecutorFenceTest,
	"Shanmen.0_0_10.Product.FormationInfluenceExecutor.OrderAndEvidenceFence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationInfluenceExecutorFenceTest::RunTest(const FString&)
{
	FFormationHostFixture Fixture;
	FShanmenWorldEntityRegistry Registry;
	Fdemo_mapShanmenFormationHostInfluenceResult Prime;
	TArray<AActor*> Subjects;
	if (!PrimeHostExecutorInfluence(
			*this, Fixture, Registry, TEXT("ExecutorFence"),
			2, Prime, Subjects))
	{
		return false;
	}
	const FGuid FirstIntent =
		Prime.ReconciliationPlan.Batch.Intents[0].IntentId;
	const FGuid SecondIntent =
		Prime.ReconciliationPlan.Batch.Intents[1].IntentId;
	FScriptedHostInfluenceExecutor Executor;
	const auto OutOfOrder =
		Fdemo_mapShanmenFormationInfluenceExecutorAdapter::TryExecute(
			Fixture.Host, Fixture.Correlation,
			MakeHostExecutionCommand(SecondIntent, 20), Executor);
	Executor.bRejectNext = true;
	const auto Rejected =
		Fdemo_mapShanmenFormationInfluenceExecutorAdapter::TryExecute(
			Fixture.Host, Fixture.Correlation,
			MakeHostExecutionCommand(FirstIntent, 21), Executor);
	const int32 PendingAfterReject =
		Fixture.Host.GetPendingInfluenceIntentCount();
	Executor.bCorruptNextIntent = true;
	const auto Corrupt =
		Fdemo_mapShanmenFormationInfluenceExecutorAdapter::TryExecute(
			Fixture.Host, Fixture.Correlation,
			MakeHostExecutionCommand(FirstIntent, 22), Executor);
	const int32 PendingAfterCorrupt =
		Fixture.Host.GetPendingInfluenceIntentCount();
	const auto Recovered =
		Fdemo_mapShanmenFormationInfluenceExecutorAdapter::TryExecute(
			Fixture.Host, Fixture.Correlation,
			MakeHostExecutionCommand(FirstIntent, 22), Executor);

	TestTrue(TEXT("Out-of-order work is rejected before executor invocation"),
		OutOfOrder.Status
			== Edemo_mapShanmenFormationInfluenceExecutionStatus::IntentOutOfOrder
			&& !OutOfOrder.bExecutorInvoked
			&& Rejected.Status
				== Edemo_mapShanmenFormationInfluenceExecutionStatus::
					ExecutorRejected
			&& PendingAfterReject == 2
			&& Corrupt.Status
				== Edemo_mapShanmenFormationInfluenceExecutionStatus::
					ExecutorEvidenceMismatch
			&& PendingAfterCorrupt == 2);
	TestTrue(TEXT("Mismatched evidence cannot acknowledge but exact command may retry"),
		Recovered.IsSuccess()
			&& Recovered.Status
				== Edemo_mapShanmenFormationInfluenceExecutionStatus::Succeeded
			&& Executor.InvocationCount == 3
			&& Fixture.Host.GetPendingInfluenceIntentCount() == 1
			&& Fixture.Host.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationInfluenceExecutorTerminalTest,
	"Shanmen.0_0_10.Product.FormationInfluenceExecutor.TerminalDrainSeal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationInfluenceExecutorTerminalTest::RunTest(const FString&)
{
	FFormationHostFixture Fixture;
	FShanmenWorldEntityRegistry Registry;
	Fdemo_mapShanmenFormationHostInfluenceResult Prime;
	TArray<AActor*> Subjects;
	if (!PrimeHostExecutorInfluence(
			*this, Fixture, Registry, TEXT("ExecutorTerminal"),
			1, Prime, Subjects))
	{
		return false;
	}
	FScriptedHostInfluenceExecutor Executor;
	const auto ApplyCommand = MakeHostExecutionCommand(
		Prime.ReconciliationPlan.Batch.Intents[0].IntentId, 30);
	const auto Apply =
		Fdemo_mapShanmenFormationInfluenceExecutorAdapter::TryExecute(
			Fixture.Host, Fixture.Correlation, ApplyCommand, Executor);
	const auto Terminal =
		Fixture.Host.TryPrepareTerminalInfluence(Fixture.Correlation);
	Fdemo_mapShanmenFormationInfluenceIntent RemoveIntent;
	if (!Apply.IsSuccess() || !Terminal.IsSuccess()
		|| !Fixture.Host.TryPeekNextInfluenceIntent(RemoveIntent))
	{
		return false;
	}
	const auto RemoveCommand =
		MakeHostExecutionCommand(RemoveIntent.IntentId, 31);
	const auto Remove =
		Fdemo_mapShanmenFormationInfluenceExecutorAdapter::TryExecute(
			Fixture.Host, Fixture.Correlation, RemoveCommand, Executor);
	const auto Seal = Fixture.Host.TrySealInfluence(Fixture.Correlation);
	const auto End = Fixture.Host.TryEndAndTeardown(
		Fixture.World, Fixture.Correlation);
	const auto RemoveReplay =
		Fdemo_mapShanmenFormationInfluenceExecutorAdapter::TryExecute(
			Fixture.Host, Fixture.Correlation, RemoveCommand, Executor);

	TestTrue(TEXT("Adapter drains terminal Remove and sealed replay is executor-free"),
		Remove.IsSuccess()
			&& RemoveReplay.Status
				== Edemo_mapShanmenFormationInfluenceExecutionStatus::
					AttemptReplayed
			&& !RemoveReplay.bExecutorInvoked
			&& Executor.InvocationCount == 2);
	TestTrue(TEXT("Drained adapter evidence seals and permits teardown"),
		Seal.IsSuccess()
			&& End.Status == Edemo_mapShanmenFormationHostStatus::Ended
			&& Fixture.Host.GetInfluenceLedger().IsSealed()
			&& Fixture.Host.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationInfluenceLeaseIntegrationTest,
	"Shanmen.0_0_10.Product.FormationInfluenceLeaseExecutor.ApplyRemoveIntegration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationInfluenceLeaseIntegrationTest::RunTest(const FString&)
{
	FFormationHostFixture Fixture;
	FShanmenWorldEntityRegistry Registry;
	Fdemo_mapShanmenFormationHostInfluenceResult Prime;
	TArray<AActor*> Subjects;
	if (!PrimeHostExecutorInfluence(
			*this, Fixture, Registry, TEXT("LeaseIntegration"),
			1, Prime, Subjects))
	{
		return false;
	}
	Fdemo_mapShanmenFormationInfluenceLeaseExecutor Executor;
	const auto ApplyCommand = MakeHostExecutionCommand(
		Prime.ReconciliationPlan.Batch.Intents[0].IntentId, 100);
	const auto Apply =
		Fdemo_mapShanmenFormationInfluenceExecutorAdapter::TryExecute(
			Fixture.Host, Fixture.Correlation, ApplyCommand, Executor);
	const auto Terminal =
		Fixture.Host.TryPrepareTerminalInfluence(Fixture.Correlation);
	Fdemo_mapShanmenFormationInfluenceIntent RemoveIntent;
	if (!Apply.IsSuccess() || !Terminal.IsSuccess()
		|| !Fixture.Host.TryPeekNextInfluenceIntent(RemoveIntent))
	{
		return false;
	}
	const auto Remove =
		Fdemo_mapShanmenFormationInfluenceExecutorAdapter::TryExecute(
			Fixture.Host, Fixture.Correlation,
			MakeHostExecutionCommand(RemoveIntent.IntentId, 101), Executor);
	const auto Seal = Fixture.Host.TrySealInfluence(Fixture.Correlation);
	const auto End = Fixture.Host.TryEndAndTeardown(
		Fixture.World, Fixture.Correlation);

	TestTrue(TEXT("Concrete lease executor applies and removes through the adapter"),
		Apply.IsSuccess() && Remove.IsSuccess()
			&& Executor.GetActiveLeaseCount() == 0
			&& Executor.GetCompletedIntentCount() == 2
			&& Executor.GetAttemptCount() == 2);
	TestTrue(TEXT("Lease drain permits the existing Host seal and teardown"),
		Seal.IsSuccess()
			&& End.Status == Edemo_mapShanmenFormationHostStatus::Ended
			&& Executor.IsConsistent() && Fixture.Host.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationInfluenceLeaseLostAckTest,
	"Shanmen.0_0_10.Product.FormationInfluenceLeaseExecutor.LostAckRecovery",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationInfluenceLeaseLostAckTest::RunTest(const FString&)
{
	FFormationHostFixture Fixture;
	FShanmenWorldEntityRegistry Registry;
	Fdemo_mapShanmenFormationHostInfluenceResult Prime;
	TArray<AActor*> Subjects;
	if (!PrimeHostExecutorInfluence(
			*this, Fixture, Registry, TEXT("LeaseLostAck"),
			1, Prime, Subjects))
	{
		return false;
	}
	const auto& ApplyIntent = Prime.ReconciliationPlan.Batch.Intents[0];
	Fdemo_mapShanmenFormationInfluenceLeaseExecutor Executor;
	const auto FirstInvocation = MakeLeaseInvocation(
		Fixture.Host, ApplyIntent, 110);
	const auto First = Executor.Execute(FirstInvocation);
	const auto ExactReplay = Executor.Execute(FirstInvocation);
	Fdemo_mapShanmenFormationInfluenceExecutorResult StoredFirst;
	const bool bReadStoredFirst = Executor.TryGetAttemptResult(
		FirstInvocation.AttemptId, StoredFirst);
	const auto RecoveryCommand = MakeHostExecutionCommand(
		ApplyIntent.IntentId, 111);
	const auto Recovered =
		Fdemo_mapShanmenFormationInfluenceExecutorAdapter::TryExecute(
			Fixture.Host, Fixture.Correlation, RecoveryCommand, Executor);
	const auto HostReplay =
		Fdemo_mapShanmenFormationInfluenceExecutorAdapter::TryExecute(
			Fixture.Host, Fixture.Correlation, RecoveryCommand, Executor);

	TestTrue(TEXT("Exact executor attempt replays one receipt without new state"),
		First.IsSuccess() && ExactReplay.IsSuccess() && bReadStoredFirst
			&& First.Receipt.ExecutorReceiptId
				== ExactReplay.Receipt.ExecutorReceiptId
			&& First.Receipt.ExecutorReceiptId
				== StoredFirst.Receipt.ExecutorReceiptId
			&& Executor.GetAttemptCount() == 2);
	TestTrue(TEXT("New attempt recovers a lost acknowledgement without duplicate lease"),
		Recovered.IsSuccess() && HostReplay.IsSuccess()
			&& Recovered.bExecutorInvoked && !HostReplay.bExecutorInvoked
			&& Executor.GetActiveLeaseCount() == 1
			&& Executor.GetCompletedIntentCount() == 1
			&& Fixture.Host.GetPendingInfluenceIntentCount() == 0
			&& Executor.IsConsistent());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationInfluenceLeaseConflictTest,
	"Shanmen.0_0_10.Product.FormationInfluenceLeaseExecutor.ConflictAndMissing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationInfluenceLeaseConflictTest::RunTest(const FString&)
{
	FFormationHostFixture Fixture;
	FShanmenWorldEntityRegistry Registry;
	Fdemo_mapShanmenFormationHostInfluenceResult Prime;
	TArray<AActor*> Subjects;
	if (!PrimeHostExecutorInfluence(
			*this, Fixture, Registry, TEXT("LeaseConflict"),
			1, Prime, Subjects))
	{
		return false;
	}
	const auto& Scope = Prime.ReconciliationPlan.Batch.Current.GetValue();
	const auto& ApplyA = Prime.ReconciliationPlan.Batch.Intents[0];
	const auto ApplyB = MakeLeaseIntent(
		Scope, ApplyA.SourceEntityId, ApplyA.SubjectEntityId,
		Edemo_mapShanmenFormationInfluenceOperation::Apply, 120);
	const auto Remove = MakeLeaseIntent(
		Scope, ApplyA.SourceEntityId, ApplyA.SubjectEntityId,
		Edemo_mapShanmenFormationInfluenceOperation::Remove, 121);
	Fdemo_mapShanmenFormationInfluenceLeaseExecutor Executor;
	const auto Missing = Executor.Execute(
		MakeLeaseInvocation(Fixture.Host, Remove, 120));
	const auto MissingReplay = Executor.Execute(
		MakeLeaseInvocation(Fixture.Host, Remove, 120));
	const auto AttemptCollision = Executor.Execute(
		MakeLeaseInvocation(Fixture.Host, ApplyA, 120));
	const auto Applied = Executor.Execute(
		MakeLeaseInvocation(Fixture.Host, ApplyA, 121));
	const auto Conflict = Executor.Execute(
		MakeLeaseInvocation(Fixture.Host, ApplyB, 122));
	const auto Removed = Executor.Execute(
		MakeLeaseInvocation(Fixture.Host, Remove, 123));

	TestTrue(TEXT("Missing Remove rejection is exact-attempt stable"),
		!Missing.IsSuccess() && !MissingReplay.IsSuccess()
			&& !AttemptCollision.IsSuccess()
			&& Executor.GetAttemptCount() == 4);
	TestTrue(TEXT("Different Apply cannot steal an active lease"),
		Applied.IsSuccess() && !Conflict.IsSuccess() && Removed.IsSuccess()
			&& Executor.GetActiveLeaseCount() == 0
			&& Executor.GetCompletedIntentCount() == 2
			&& Executor.IsConsistent());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationInfluenceLeaseStaleRemoveTest,
	"Shanmen.0_0_10.Product.FormationInfluenceLeaseExecutor.StaleRemoveAfterReapply",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationInfluenceLeaseStaleRemoveTest::RunTest(const FString&)
{
	FFormationHostFixture Fixture;
	FShanmenWorldEntityRegistry Registry;
	Fdemo_mapShanmenFormationHostInfluenceResult Prime;
	TArray<AActor*> Subjects;
	if (!PrimeHostExecutorInfluence(
			*this, Fixture, Registry, TEXT("LeaseStaleRemove"),
			1, Prime, Subjects))
	{
		return false;
	}
	const auto& Scope = Prime.ReconciliationPlan.Batch.Current.GetValue();
	const auto& ApplyA = Prime.ReconciliationPlan.Batch.Intents[0];
	const auto RemoveA = MakeLeaseIntent(
		Scope, ApplyA.SourceEntityId, ApplyA.SubjectEntityId,
		Edemo_mapShanmenFormationInfluenceOperation::Remove, 130);
	const auto ApplyB = MakeLeaseIntent(
		Scope, ApplyA.SourceEntityId, ApplyA.SubjectEntityId,
		Edemo_mapShanmenFormationInfluenceOperation::Apply, 131);
	Fdemo_mapShanmenFormationInfluenceLeaseExecutor Executor;
	const auto FirstApply = Executor.Execute(
		MakeLeaseInvocation(Fixture.Host, ApplyA, 130));
	const auto FirstRemove = Executor.Execute(
		MakeLeaseInvocation(Fixture.Host, RemoveA, 131));
	const auto SecondApply = Executor.Execute(
		MakeLeaseInvocation(Fixture.Host, ApplyB, 132));
	const auto StaleRemoveRecovery = Executor.Execute(
		MakeLeaseInvocation(Fixture.Host, RemoveA, 133));
	Fdemo_mapShanmenFormationInfluenceLeaseKey Key;
	Fdemo_mapShanmenFormationInfluenceLeaseSnapshot Active;
	const bool bReadActive =
		Fdemo_mapShanmenFormationInfluenceLeaseKey::TryFromIntent(
			ApplyB, Key)
		&& Executor.TryGetActiveLease(Key, Active);

	TestTrue(TEXT("Completed old Remove recovers without touching a newer lease"),
		FirstApply.IsSuccess() && FirstRemove.IsSuccess()
			&& SecondApply.IsSuccess() && StaleRemoveRecovery.IsSuccess()
			&& bReadActive && Active.ApplyIntentId == ApplyB.IntentId
			&& Executor.GetActiveLeaseCount() == 1);
	TestTrue(TEXT("Reapply history remains internally consistent"),
		Executor.GetCompletedIntentCount() == 3
			&& Executor.GetAttemptCount() == 4
			&& Executor.IsConsistent());
	return true;
}

#endif
