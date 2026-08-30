#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenFormationProductHost.h"
#include "demo_mapShanmenFormationInfluenceExecutorAdapter.h"
#include "demo_mapShanmenFormationInfluenceLeaseExecutor.h"
#include "demo_mapShanmenFormationInfluenceProductRuntime.h"
#include "demo_mapShanmenFormationInfluenceExecutionRouter.h"
#include "demo_mapShanmenFormationInfluenceExecutionService.h"
#include "demo_mapShanmenFormationInfluenceLifecycleCommandHost.h"
#include "demo_mapShanmenFormationInfluenceConsumerWorldResolution.h"
#include "demo_mapShanmenFormationInfluenceConsumerRunComposition.h"
#include "demo_mapShanmenFormationInfluenceLifecycleCoordinator.h"
#include "demo_mapShanmenFormationInfluenceLifecycleCommandRouter.h"
#include "demo_mapShanmenFormationInfluenceConsumerProjection.h"

#include "ShanmenCombatResolver.h"
#include "ShanmenCombatTags.h"
#include "demo_mapAttributeComponent.h"
#include "demo_map0909BSectWarehouseService.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapProfileRepository.h"
#include "demo_mapProfileSessionSubsystem.h"
#include "demo_mapPlayerHealthComponent.h"
#include "demo_mapShanmenItemAuthoritySubsystem.h"
#include "demo_mapShanmenItemCutover.h"
#include "demo_mapShanmenPreparationAdapter.h"
#include "demo_mapShanmenRunLifecycleAdapter.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Components/BoxComponent.h"
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

	Fdemo_mapShanmenFormationInfluenceEvaluationReceipt
	MakeHostEvaluationReceipt(
		const Fdemo_mapShanmenFormationInfluenceIntent& Intent,
		const int32 MagnitudeUnits = 100)
	{
		check(Intent.IsValid());
		Fdemo_mapShanmenFormationInfluencePolicy Policy;
		Policy.PolicyDefinitionId = Intent.PolicyDefinitionId;
		Policy.InfluenceDefinitionId = Intent.InfluenceDefinitionId;
		Policy.Content = Intent.Content;
		Fdemo_mapShanmenFormationInfluenceModifierSpecification Specification;
		check(Fdemo_mapShanmenFormationInfluenceModifierSpecification::TryCreate(
			Policy,
			TEXT("Formation.Modifier.Test.ExecutionBinding"),
			FShanmenCombatNativeTags::InfluenceOffensePower(),
			FGameplayTagContainer(), FGameplayTagContainer(), MagnitudeUnits,
			TEXT("Formation.Stack.Test.ExecutionBinding"),
			Edemo_mapShanmenFormationInfluenceStackPolicy::Additive,
			0, Specification));
		Fdemo_mapShanmenFormationInfluenceEvaluationContext Context;
		Context.RunId = Intent.RunId;
		Context.SubjectEntityId = Intent.SubjectEntityId;
		Context.Channel = FShanmenCombatNativeTags::InfluenceOffensePower();
		Context.Content = Intent.Content;
		const auto Evaluated =
			Fdemo_mapShanmenFormationInfluenceModifierEvaluator::Evaluate(
				Context, { Specification });
		check(Evaluated.IsSuccess());
		return Evaluated.Receipt;
	}

	Fdemo_mapShanmenFormationInfluenceEvaluationBinding
	MakeHostEvaluationBinding(
		const Fdemo_mapShanmenFormationInfluenceIntent& Intent)
	{
		if (Intent.Operation
			== Edemo_mapShanmenFormationInfluenceOperation::Remove)
		{
			return Fdemo_mapShanmenFormationInfluenceEvaluationBinding::
				MakeRemove();
		}
		Fdemo_mapShanmenFormationInfluenceEvaluationBinding Binding;
		check(Fdemo_mapShanmenFormationInfluenceEvaluationBinding::
			TryCaptureApply(MakeHostEvaluationReceipt(Intent), Binding));
		return Binding;
	}

	Fdemo_mapShanmenFormationInfluenceExecutionCommand
	MakeHostExecutionCommand(
		const Fdemo_mapShanmenFormationInfluenceIntent& Intent,
		const int32 Ordinal)
	{
		Fdemo_mapShanmenFormationInfluenceExecutionCommand Command;
		Command.IntentId = Intent.IntentId;
		Command.AttemptId = FGuid(0xF8510000 + Ordinal, 0, 0, 1);
		Command.Evaluation = MakeHostEvaluationBinding(Intent);
		check(Command.IsValid());
		return Command;
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

	Fdemo_mapShanmenFormationInfluenceExecutionRequest
	MakeHostExecutionRequest(
		const Fdemo_mapShanmenFormationInfluenceIntent& Intent,
		const int32 Ordinal)
	{
		Fdemo_mapShanmenFormationInfluenceExecutionRequest Request;
		Request.RequestId = FGuid(0xF8710000 + Ordinal, 0, 0, 1);
		Request.ExpectedIntentId = Intent.IntentId;
		Request.Evaluation = MakeHostEvaluationBinding(Intent);
		check(Request.IsValid());
		return Request;
	}

	Fdemo_mapShanmenFormationInfluenceExecutionRequest
	MakeHostExecutionRequest(
		const FGuid& ExpectedIntentId,
		const int32 Ordinal)
	{
		Fdemo_mapShanmenFormationInfluenceExecutionRequest Request;
		Request.RequestId = FGuid(0xF8710000 + Ordinal, 0, 0, 1);
		Request.ExpectedIntentId = ExpectedIntentId;
		check(Request.IsValid());
		return Request;
	}

	FGuid MakeLifecycleCommandId(const int32 Ordinal)
	{
		return FGuid(0xF8720000 + Ordinal, 0, 0, 1);
	}

	Fdemo_mapShanmenFormationInfluenceLifecycleCommand
	MakeLifecycleStepCommand(
		const Fdemo_mapShanmenRunCorrelation& Correlation,
		const Fdemo_mapShanmenFormationInfluenceExecutionRequest& Request)
	{
		Fdemo_mapShanmenFormationInfluenceLifecycleCommand Command;
		check(Fdemo_mapShanmenFormationInfluenceLifecycleCommand::
			TryCaptureStep(
				Request.RequestId,
				Correlation,
				Request,
				Command));
		return Command;
	}

	Fdemo_mapShanmenFormationInfluenceLifecycleCommand
	MakeLifecycleTerminalCommand(
		const Fdemo_mapShanmenRunCorrelation& Correlation,
		const int32 CommandOrdinal)
	{
		Fdemo_mapShanmenFormationInfluenceLifecycleCommand Command;
		check(Fdemo_mapShanmenFormationInfluenceLifecycleCommand::
			TryCapturePrepareTerminal(
				MakeLifecycleCommandId(CommandOrdinal),
				Correlation,
				Command));
		return Command;
	}

	Fdemo_mapShanmenFormationInfluenceLifecycleCommand
	MakeLifecycleEndCommand(
		const Fdemo_mapShanmenRunCorrelation& Correlation,
		const int32 CommandOrdinal)
	{
		Fdemo_mapShanmenFormationInfluenceLifecycleCommand Command;
		check(Fdemo_mapShanmenFormationInfluenceLifecycleCommand::
			TryCaptureSealAndEnd(
				MakeLifecycleCommandId(CommandOrdinal),
				Correlation,
				Command));
		return Command;
	}

	struct FHostConsumerCommands
	{
		FGuid SubjectEntityId;
		Fdemo_mapShanmenFormationInfluenceConsumerCommand Apply;
		Fdemo_mapShanmenFormationInfluenceConsumerCommand Remove;
		Fdemo_mapShanmenFormationInfluenceConsumerCommandDeliveryResult
			Delivery;
	};

	bool BuildHostConsumerCommands(
		const Fdemo_mapShanmenFormationInfluenceLifecycleCommandHost&
			CommandHost,
		const FGuid& AppliedLifecycleCommandId,
		const Fdemo_mapShanmenFormationInfluenceIntent& ApplyIntent,
		FHostConsumerCommands& OutCommands)
	{
		OutCommands = FHostConsumerCommands();
		Fdemo_mapShanmenFormationInfluenceConsumerDefinition Definition;
		if (!Fdemo_mapShanmenFormationInfluenceConsumerDefinition::
			TryCreateOffensePowerAdditive(
				TEXT("Formation.Consumer.ProductLifecycle.OffensePower"),
				100, 40, ApplyIntent.Content, Definition))
		{
			return false;
		}
		OutCommands.Delivery = CommandHost.TryPrepareConsumerCommands(
			AppliedLifecycleCommandId, Definition);
		if (!OutCommands.Delivery.IsSuccess())
		{
			return false;
		}
		OutCommands.SubjectEntityId =
			OutCommands.Delivery.Delivery.SubjectEntityId;
		OutCommands.Apply = OutCommands.Delivery.Delivery.Apply;
		OutCommands.Remove = OutCommands.Delivery.Delivery.Remove;
		return OutCommands.SubjectEntityId == ApplyIntent.SubjectEntityId;
	}

	bool HostExecutionCommandsMatch(
		const Fdemo_mapShanmenFormationInfluenceExecutionCommand& Left,
		const Fdemo_mapShanmenFormationInfluenceExecutionCommand& Right)
	{
		return Left.IsValid() && Right.IsValid()
			&& Left.IntentId == Right.IntentId
			&& Left.AttemptId == Right.AttemptId
			&& Left.Evaluation.Matches(Right.Evaluation);
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
		Invocation.Evaluation = MakeHostEvaluationBinding(Intent);
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
		Prime.ReconciliationPlan.Batch.Intents[0], 1);
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
	const auto& Intent = Prime.ReconciliationPlan.Batch.Intents[0];
	const auto RetryCommand = MakeHostExecutionCommand(Intent, 10);
	const auto SuccessCommand = MakeHostExecutionCommand(Intent, 11);
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
	const auto& FirstIntent = Prime.ReconciliationPlan.Batch.Intents[0];
	const auto& SecondIntent = Prime.ReconciliationPlan.Batch.Intents[1];
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
		Prime.ReconciliationPlan.Batch.Intents[0], 30);
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
		MakeHostExecutionCommand(RemoveIntent, 31);
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
		Prime.ReconciliationPlan.Batch.Intents[0], 100);
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
			MakeHostExecutionCommand(RemoveIntent, 101), Executor);
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
		ApplyIntent, 111);
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationInfluenceProductRuntimeSingleStepTest,
	"Shanmen.0_0_10.Product.FormationInfluenceProductRuntime.SingleStepAndReplay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationInfluenceProductRuntimeSingleStepTest::RunTest(
	const FString&)
{
	FFormationHostFixture Fixture;
	FShanmenWorldEntityRegistry Registry;
	Fdemo_mapShanmenFormationHostInfluenceResult Prime;
	TArray<AActor*> Subjects;
	if (!PrimeHostExecutorInfluence(
			*this, Fixture, Registry, TEXT("ProductRuntimeSingleStep"),
			2, Prime, Subjects))
	{
		return false;
	}
	Fdemo_mapShanmenFormationInfluenceProductRuntime Runtime;
	const auto OutOfOrderCommand = MakeHostExecutionCommand(
		Prime.ReconciliationPlan.Batch.Intents[1], 200);
	const auto OutOfOrder = Runtime.TryExecuteOne(
		Fixture.Host, Fixture.Correlation, OutOfOrderCommand);
	const bool bUnboundAfterOutOfOrder = !Runtime.IsBound()
		&& Runtime.GetExecutorAttemptCount() == 0
		&& Fixture.Host.GetPendingInfluenceIntentCount() == 2;
	const auto FirstCommand = MakeHostExecutionCommand(
		Prime.ReconciliationPlan.Batch.Intents[0], 201);
	const auto First = Runtime.TryExecuteOne(
		Fixture.Host, Fixture.Correlation, FirstCommand);
	const auto Replay = Runtime.TryExecuteOne(
		Fixture.Host, Fixture.Correlation, FirstCommand);
	const auto SecondCommand = MakeHostExecutionCommand(
		Prime.ReconciliationPlan.Batch.Intents[1], 202);
	const auto Second = Runtime.TryExecuteOne(
		Fixture.Host, Fixture.Correlation, SecondCommand);

	TestTrue(TEXT("Rejected first command leaves the product runtime unbound"),
		!OutOfOrder.IsSuccess()
			&& OutOfOrder.Status
				== Edemo_mapShanmenFormationInfluenceExecutionStatus::
					IntentOutOfOrder
			&& bUnboundAfterOutOfOrder);
	TestTrue(TEXT("Runtime binds once and executes one canonical intent per call"),
		First.IsSuccess() && First.bExecutorInvoked
			&& Replay.IsSuccess() && !Replay.bExecutorInvoked
			&& Second.IsSuccess() && Second.bExecutorInvoked
			&& Runtime.IsBound() && Runtime.IsValid()
			&& Runtime.GetBoundCorrelation() == Fixture.Correlation
			&& Runtime.GetBoundLedgerId()
				== Fixture.Host.GetInfluenceLedger().GetLedgerId()
			&& Runtime.GetExecutorAttemptCount() == 2
			&& Runtime.GetActiveLeaseCount() == 2
			&& Fixture.Host.GetPendingInfluenceIntentCount() == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationInfluenceProductRuntimeBindingFenceTest,
	"Shanmen.0_0_10.Product.FormationInfluenceProductRuntime.BindingAndCorrelationFence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationInfluenceProductRuntimeBindingFenceTest::RunTest(
	const FString&)
{
	FFormationHostFixture FirstFixture;
	FFormationHostFixture OtherFixture;
	FShanmenWorldEntityRegistry FirstRegistry;
	FShanmenWorldEntityRegistry OtherRegistry;
	Fdemo_mapShanmenFormationHostInfluenceResult FirstPrime;
	Fdemo_mapShanmenFormationHostInfluenceResult OtherPrime;
	TArray<AActor*> FirstSubjects;
	TArray<AActor*> OtherSubjects;
	if (!PrimeHostExecutorInfluence(
			*this, FirstFixture, FirstRegistry, TEXT("ProductRuntimeBindingA"),
			1, FirstPrime, FirstSubjects)
		|| !PrimeHostExecutorInfluence(
			*this, OtherFixture, OtherRegistry, TEXT("ProductRuntimeBindingB"),
			1, OtherPrime, OtherSubjects))
	{
		return false;
	}
	Fdemo_mapShanmenFormationInfluenceProductRuntime Runtime;
	const auto FirstCommand = MakeHostExecutionCommand(
		FirstPrime.ReconciliationPlan.Batch.Intents[0], 210);
	const auto First = Runtime.TryExecuteOne(
		FirstFixture.Host, FirstFixture.Correlation, FirstCommand);
	const auto OtherCommand = MakeHostExecutionCommand(
		OtherPrime.ReconciliationPlan.Batch.Intents[0], 211);
	const auto ForeignHost = Runtime.TryExecuteOne(
		OtherFixture.Host, OtherFixture.Correlation, OtherCommand);
	const auto StaleCorrelation = Runtime.TryExecuteOne(
		FirstFixture.Host, OtherFixture.Correlation, FirstCommand);

	TestTrue(TEXT("Runtime rejects a different Host binding without execution"),
		First.IsSuccess() && !ForeignHost.IsSuccess()
			&& !ForeignHost.bExecutorInvoked
			&& ForeignHost.Status
				== Edemo_mapShanmenFormationInfluenceExecutionStatus::StateInvalid
			&& OtherFixture.Host.GetPendingInfluenceIntentCount() == 1);
	TestTrue(TEXT("Runtime rejects stale correlation without mutating its lease"),
		!StaleCorrelation.IsSuccess()
			&& !StaleCorrelation.bExecutorInvoked
			&& StaleCorrelation.Status
				== Edemo_mapShanmenFormationInfluenceExecutionStatus::
					CorrelationMismatch
			&& Runtime.GetActiveLeaseCount() == 1
			&& Runtime.GetExecutorAttemptCount() == 1
			&& Runtime.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationInfluenceProductRuntimeLateAttachTest,
	"Shanmen.0_0_10.Product.FormationInfluenceProductRuntime.LateAttachmentFence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationInfluenceProductRuntimeLateAttachTest::RunTest(
	const FString&)
{
	FFormationHostFixture Fixture;
	FShanmenWorldEntityRegistry Registry;
	Fdemo_mapShanmenFormationHostInfluenceResult Prime;
	TArray<AActor*> Subjects;
	if (!PrimeHostExecutorInfluence(
			*this, Fixture, Registry, TEXT("ProductRuntimeLateAttach"),
			1, Prime, Subjects))
	{
		return false;
	}
	const auto& Intent = Prime.ReconciliationPlan.Batch.Intents[0];
	const auto Manual = MakeHostInfluenceAttempt(
		Intent.IntentId, 220,
		Edemo_mapShanmenFormationInfluenceAttemptOutcome::Succeeded);
	const auto Acknowledged = Fixture.Host.TryAcknowledgeInfluence(
		Fixture.Correlation, Manual);
	Fdemo_mapShanmenFormationInfluenceExecutionCommand ReplayCommand;
	ReplayCommand.IntentId = Intent.IntentId;
	ReplayCommand.AttemptId = Manual.AttemptId;
	ReplayCommand.Evaluation = MakeHostEvaluationBinding(Intent);
	Fdemo_mapShanmenFormationInfluenceProductRuntime Runtime;
	const auto LateAttach = Runtime.TryExecuteOne(
		Fixture.Host, Fixture.Correlation, ReplayCommand);

	TestTrue(TEXT("Unbound runtime rejects Host history with a missing lease"),
		Acknowledged.IsSuccess() && !LateAttach.IsSuccess()
			&& LateAttach.Status
				== Edemo_mapShanmenFormationInfluenceExecutionStatus::StateInvalid
			&& !LateAttach.bExecutorInvoked && !Runtime.IsBound()
			&& Runtime.GetActiveLeaseCount() == 0
			&& Runtime.GetCompletedIntentCount() == 0
			&& Runtime.GetExecutorAttemptCount() == 0
			&& Runtime.IsValid());

	FFormationHostFixture RetryFixture;
	FShanmenWorldEntityRegistry RetryRegistry;
	Fdemo_mapShanmenFormationHostInfluenceResult RetryPrime;
	TArray<AActor*> RetrySubjects;
	if (!PrimeHostExecutorInfluence(
			*this, RetryFixture, RetryRegistry,
			TEXT("ProductRuntimeRetryAttach"), 1,
			RetryPrime, RetrySubjects))
	{
		return false;
	}
	const auto& RetryIntent =
		RetryPrime.ReconciliationPlan.Batch.Intents[0];
	const auto ManualRetry = MakeHostInfluenceAttempt(
		RetryIntent.IntentId, 221,
		Edemo_mapShanmenFormationInfluenceAttemptOutcome::RetryableFailure);
	const auto RetryRecorded = RetryFixture.Host.TryAcknowledgeInfluence(
		RetryFixture.Correlation, ManualRetry);
	Fdemo_mapShanmenFormationInfluenceExecutionCommand RetryReplayCommand;
	RetryReplayCommand.IntentId = RetryIntent.IntentId;
	RetryReplayCommand.AttemptId = ManualRetry.AttemptId;
	RetryReplayCommand.Evaluation = MakeHostEvaluationBinding(RetryIntent);
	Fdemo_mapShanmenFormationInfluenceProductRuntime RetryRuntime;
	const auto RetryReplay = RetryRuntime.TryExecuteOne(
		RetryFixture.Host, RetryFixture.Correlation, RetryReplayCommand);
	const auto RetryRecovered = RetryRuntime.TryExecuteOne(
		RetryFixture.Host, RetryFixture.Correlation,
		MakeHostExecutionCommand(RetryIntent, 222));
	TestTrue(TEXT("Retry-only Host history can bind before semantic mutation"),
		RetryRecorded.IsSuccess() && RetryReplay.IsSuccess()
			&& !RetryReplay.bExecutorInvoked && RetryRecovered.IsSuccess()
			&& RetryRecovered.bExecutorInvoked && RetryRuntime.IsBound()
			&& RetryRuntime.GetExecutorAttemptCount() == 1
			&& RetryRuntime.GetActiveLeaseCount() == 1
			&& RetryFixture.Host.GetPendingInfluenceIntentCount() == 0
			&& RetryRuntime.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationInfluenceProductRuntimeTerminalTest,
	"Shanmen.0_0_10.Product.FormationInfluenceProductRuntime.TerminalDrainReplay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationInfluenceProductRuntimeTerminalTest::RunTest(
	const FString&)
{
	FFormationHostFixture Fixture;
	FShanmenWorldEntityRegistry Registry;
	Fdemo_mapShanmenFormationHostInfluenceResult Prime;
	TArray<AActor*> Subjects;
	if (!PrimeHostExecutorInfluence(
			*this, Fixture, Registry, TEXT("ProductRuntimeTerminal"),
			1, Prime, Subjects))
	{
		return false;
	}
	Fdemo_mapShanmenFormationInfluenceProductRuntime Runtime;
	const auto ApplyCommand = MakeHostExecutionCommand(
		Prime.ReconciliationPlan.Batch.Intents[0], 230);
	const auto Apply = Runtime.TryExecuteOne(
		Fixture.Host, Fixture.Correlation, ApplyCommand);
	const auto Terminal = Fixture.Host.TryPrepareTerminalInfluence(
		Fixture.Correlation);
	Fdemo_mapShanmenFormationInfluenceIntent RemoveIntent;
	if (!Apply.IsSuccess() || !Terminal.IsSuccess()
		|| !Fixture.Host.TryPeekNextInfluenceIntent(RemoveIntent))
	{
		return false;
	}
	const auto RemoveCommand = MakeHostExecutionCommand(
		RemoveIntent, 231);
	const auto Remove = Runtime.TryExecuteOne(
		Fixture.Host, Fixture.Correlation, RemoveCommand);
	const auto Seal = Fixture.Host.TrySealInfluence(Fixture.Correlation);
	const auto End = Fixture.Host.TryEndAndTeardown(
		Fixture.World, Fixture.Correlation);
	const auto RemoveReplay = Runtime.TryExecuteOne(
		Fixture.Host, Fixture.Correlation, RemoveCommand);

	TestTrue(TEXT("Runtime drains Apply and Remove before Host seal"),
		Remove.IsSuccess() && Seal.IsSuccess()
			&& End.Status == Edemo_mapShanmenFormationHostStatus::Ended
			&& Runtime.GetActiveLeaseCount() == 0
			&& Runtime.GetCompletedIntentCount() == 2
			&& Runtime.GetExecutorAttemptCount() == 2);
	TestTrue(TEXT("Sealed exact replay never invokes the owned executor again"),
		RemoveReplay.IsSuccess() && !RemoveReplay.bExecutorInvoked
			&& Runtime.GetExecutorAttemptCount() == 2
			&& Runtime.IsBound() && Runtime.IsValid()
			&& Fixture.Host.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationInfluenceExecutionRouterRouteTest,
	"Shanmen.0_0_10.Product.FormationInfluenceExecutionRouter.RouteExecuteReplay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationInfluenceExecutionRouterRouteTest::RunTest(
	const FString&)
{
	FFormationHostFixture Fixture;
	FShanmenWorldEntityRegistry Registry;
	Fdemo_mapShanmenFormationHostInfluenceResult Prime;
	TArray<AActor*> Subjects;
	if (!PrimeHostExecutorInfluence(
			*this, Fixture, Registry, TEXT("ExecutionRouterRoute"),
			2, Prime, Subjects))
	{
		return false;
	}

	Fdemo_mapShanmenFormationInfluenceExecutionRouter Router;
	Fdemo_mapShanmenFormationInfluenceProductRuntime Runtime;
	const auto FirstRequest = MakeHostExecutionRequest(
		Prime.ReconciliationPlan.Batch.Intents[0], 1);
	const auto FirstRoute = Router.TryRoute(
		Fixture.Host, Fixture.Correlation, FirstRequest);
	const auto FirstRouteReplay = Router.TryRoute(
		Fixture.Host, Fixture.Correlation, FirstRequest);
	Fdemo_mapShanmenFormationInfluenceExecutionCommand StoredFirst;
	const bool bReadFirst = Router.TryGetCommand(
		FirstRequest.RequestId, StoredFirst);
	const auto FirstExecution = Runtime.TryExecuteOne(
		Fixture.Host, Fixture.Correlation, FirstRoute.Command);
	const auto FirstExecutionReplay = Runtime.TryExecuteOne(
		Fixture.Host, Fixture.Correlation, FirstRouteReplay.Command);

	const auto SecondRequest = MakeHostExecutionRequest(
		Prime.ReconciliationPlan.Batch.Intents[1], 2);
	const auto SecondRoute = Router.TryRoute(
		Fixture.Host, Fixture.Correlation, SecondRequest);
	const auto SecondExecution = Runtime.TryExecuteOne(
		Fixture.Host, Fixture.Correlation, SecondRoute.Command);

	TestTrue(TEXT("Router emits and replays one stable command per request"),
		FirstRoute.Status
			== Edemo_mapShanmenFormationInfluenceRouteStatus::Routed
			&& FirstRouteReplay.Status
				== Edemo_mapShanmenFormationInfluenceRouteStatus::
					RequestReplayed
			&& FirstRoute.IsSuccess() && FirstRouteReplay.IsSuccess()
			&& bReadFirst
			&& HostExecutionCommandsMatch(
				FirstRoute.Command, FirstRouteReplay.Command)
			&& HostExecutionCommandsMatch(FirstRoute.Command, StoredFirst));
	TestTrue(TEXT("Two routed commands execute once each in canonical order"),
		FirstExecution.IsSuccess() && FirstExecution.bExecutorInvoked
			&& FirstExecutionReplay.IsSuccess()
			&& !FirstExecutionReplay.bExecutorInvoked
			&& SecondRoute.IsSuccess() && SecondExecution.IsSuccess()
			&& SecondExecution.bExecutorInvoked
			&& FirstRoute.Command.AttemptId
				!= SecondRoute.Command.AttemptId
			&& Router.GetRecordCount() == 2 && Router.IsValid()
			&& Runtime.GetExecutorAttemptCount() == 2
			&& Runtime.GetActiveLeaseCount() == 2
			&& Fixture.Host.GetPendingInfluenceIntentCount() == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationInfluenceExecutionRouterFenceTest,
	"Shanmen.0_0_10.Product.FormationInfluenceExecutionRouter.RequestConflictAndOrderFence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationInfluenceExecutionRouterFenceTest::RunTest(
	const FString&)
{
	FFormationHostFixture Fixture;
	FShanmenWorldEntityRegistry Registry;
	Fdemo_mapShanmenFormationHostInfluenceResult Prime;
	TArray<AActor*> Subjects;
	if (!PrimeHostExecutorInfluence(
			*this, Fixture, Registry, TEXT("ExecutionRouterFence"),
			2, Prime, Subjects))
	{
		return false;
	}

	const auto& FirstIntent = Prime.ReconciliationPlan.Batch.Intents[0];
	const auto& SecondIntent = Prime.ReconciliationPlan.Batch.Intents[1];
	Fdemo_mapShanmenFormationInfluenceExecutionRouter Router;
	const auto OutOfOrder = Router.TryRoute(
		Fixture.Host, Fixture.Correlation,
		MakeHostExecutionRequest(SecondIntent, 10));
	const bool bUnboundAfterOutOfOrder =
		!Router.IsBound() && Router.GetRecordCount() == 0;
	const auto FirstRequest = MakeHostExecutionRequest(FirstIntent, 11);
	const auto FirstRoute = Router.TryRoute(
		Fixture.Host, Fixture.Correlation, FirstRequest);
	auto ConflictingRequest = FirstRequest;
	ConflictingRequest.ExpectedIntentId = SecondIntent.IntentId;
	const auto Conflict = Router.TryRoute(
		Fixture.Host, Fixture.Correlation, ConflictingRequest);
	const auto SecondRequest = MakeHostExecutionRequest(SecondIntent, 12);
	const auto EarlySecond = Router.TryRoute(
		Fixture.Host, Fixture.Correlation, SecondRequest);

	Fdemo_mapShanmenFormationInfluenceProductRuntime Runtime;
	const auto FirstExecution = Runtime.TryExecuteOne(
		Fixture.Host, Fixture.Correlation, FirstRoute.Command);
	const auto SecondRoute = Router.TryRoute(
		Fixture.Host, Fixture.Correlation, SecondRequest);
	const auto SecondExecution = Runtime.TryExecuteOne(
		Fixture.Host, Fixture.Correlation, SecondRoute.Command);

	TestTrue(TEXT("Rejected order does not bind an empty Router"),
		OutOfOrder.Status
			== Edemo_mapShanmenFormationInfluenceRouteStatus::IntentOutOfOrder
			&& !OutOfOrder.IsSuccess() && bUnboundAfterOutOfOrder
			&& FirstRoute.IsSuccess() && Router.IsBound());
	TestTrue(TEXT("Request reuse and premature next intent fail closed"),
		Conflict.Status
			== Edemo_mapShanmenFormationInfluenceRouteStatus::RequestConflict
			&& EarlySecond.Status
				== Edemo_mapShanmenFormationInfluenceRouteStatus::
					IntentOutOfOrder
			&& !Conflict.IsSuccess() && !EarlySecond.IsSuccess());
	TestTrue(TEXT("Advancing Host evidence admits the next unique request"),
		FirstExecution.IsSuccess() && SecondRoute.IsSuccess()
			&& SecondExecution.IsSuccess()
			&& Router.GetRecordCount() == 2 && Router.IsValid()
			&& Runtime.GetActiveLeaseCount() == 2
			&& Fixture.Host.GetPendingInfluenceIntentCount() == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationInfluenceExecutionRouterRecoveryTest,
	"Shanmen.0_0_10.Product.FormationInfluenceExecutionRouter.BindingAndHistoricalRecovery",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationInfluenceExecutionRouterRecoveryTest::RunTest(
	const FString&)
{
	FFormationHostFixture FirstFixture;
	FFormationHostFixture OtherFixture;
	FShanmenWorldEntityRegistry FirstRegistry;
	FShanmenWorldEntityRegistry OtherRegistry;
	Fdemo_mapShanmenFormationHostInfluenceResult FirstPrime;
	Fdemo_mapShanmenFormationHostInfluenceResult OtherPrime;
	TArray<AActor*> FirstSubjects;
	TArray<AActor*> OtherSubjects;
	if (!PrimeHostExecutorInfluence(
			*this, FirstFixture, FirstRegistry,
			TEXT("ExecutionRouterRecoveryA"), 1,
			FirstPrime, FirstSubjects)
		|| !PrimeHostExecutorInfluence(
			*this, OtherFixture, OtherRegistry,
			TEXT("ExecutionRouterRecoveryB"), 1,
			OtherPrime, OtherSubjects))
	{
		return false;
	}

	const auto Request = MakeHostExecutionRequest(
		FirstPrime.ReconciliationPlan.Batch.Intents[0], 20);
	Fdemo_mapShanmenFormationInfluenceExecutionRouter Router;
	Fdemo_mapShanmenFormationInfluenceProductRuntime Runtime;
	const auto InitialRoute = Router.TryRoute(
		FirstFixture.Host, FirstFixture.Correlation, Request);
	const auto InitialExecution = Runtime.TryExecuteOne(
		FirstFixture.Host, FirstFixture.Correlation, InitialRoute.Command);

	Fdemo_mapShanmenFormationInfluenceExecutionRouter RecoveredRouter;
	const auto Recovered = RecoveredRouter.TryRoute(
		FirstFixture.Host, FirstFixture.Correlation, Request);
	const auto HostReplay = Runtime.TryExecuteOne(
		FirstFixture.Host, FirstFixture.Correlation, Recovered.Command);
	const auto ForeignRequest = MakeHostExecutionRequest(
		OtherPrime.ReconciliationPlan.Batch.Intents[0], 21);
	const auto Foreign = Router.TryRoute(
		OtherFixture.Host, OtherFixture.Correlation, ForeignRequest);

	TestTrue(TEXT("Fresh Router reconstructs an issued command from Host evidence"),
		InitialRoute.IsSuccess() && InitialExecution.IsSuccess()
			&& Recovered.Status
				== Edemo_mapShanmenFormationInfluenceRouteStatus::
					HostEvidenceRecovered
			&& Recovered.IsSuccess()
			&& HostExecutionCommandsMatch(
				InitialRoute.Command, Recovered.Command)
			&& HostReplay.IsSuccess() && !HostReplay.bExecutorInvoked
			&& RecoveredRouter.IsBound()
			&& RecoveredRouter.GetRecordCount() == 1);
	TestTrue(TEXT("A bound Router cannot cross into another Host ledger"),
		Foreign.Status
			== Edemo_mapShanmenFormationInfluenceRouteStatus::BindingConflict
			&& !Foreign.IsSuccess()
			&& OtherFixture.Host.GetPendingInfluenceIntentCount() == 1
			&& Router.GetRecordCount() == 1
			&& Runtime.GetExecutorAttemptCount() == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationInfluenceExecutionRouterRetryTest,
	"Shanmen.0_0_10.Product.FormationInfluenceExecutionRouter.RetryAndTerminalDrain",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationInfluenceExecutionRouterRetryTest::RunTest(
	const FString&)
{
	FFormationHostFixture Fixture;
	FShanmenWorldEntityRegistry Registry;
	Fdemo_mapShanmenFormationHostInfluenceResult Prime;
	TArray<AActor*> Subjects;
	if (!PrimeHostExecutorInfluence(
			*this, Fixture, Registry, TEXT("ExecutionRouterRetry"),
			1, Prime, Subjects))
	{
		return false;
	}

	const auto& ApplyIntent = Prime.ReconciliationPlan.Batch.Intents[0];
	Fdemo_mapShanmenFormationInfluenceExecutionRouter Router;
	const auto RetryRequest = MakeHostExecutionRequest(ApplyIntent, 30);
	const auto RetryRoute = Router.TryRoute(
		Fixture.Host, Fixture.Correlation, RetryRequest);
	Fdemo_mapShanmenFormationInfluenceAttemptCommand RetryCommand;
	RetryCommand.IntentId = RetryRoute.Command.IntentId;
	RetryCommand.AttemptId = RetryRoute.Command.AttemptId;
	RetryCommand.ExecutorReceiptId = FGuid(0xF8720001, 0, 0, 1);
	RetryCommand.Outcome =
		Edemo_mapShanmenFormationInfluenceAttemptOutcome::RetryableFailure;
	const auto RetryRecorded = Fixture.Host.TryAcknowledgeInfluence(
		Fixture.Correlation, RetryCommand);
	const auto RetryRouteReplay = Router.TryRoute(
		Fixture.Host, Fixture.Correlation, RetryRequest);

	const auto RecoveryRequest = MakeHostExecutionRequest(ApplyIntent, 31);
	const auto RecoveryRoute = Router.TryRoute(
		Fixture.Host, Fixture.Correlation, RecoveryRequest);
	Fdemo_mapShanmenFormationInfluenceProductRuntime Runtime;
	const auto RecoveryExecution = Runtime.TryExecuteOne(
		Fixture.Host, Fixture.Correlation, RecoveryRoute.Command);
	const auto Terminal = Fixture.Host.TryPrepareTerminalInfluence(
		Fixture.Correlation);
	Fdemo_mapShanmenFormationInfluenceIntent RemoveIntent;
	if (!RetryRoute.IsSuccess() || !RetryRecorded.IsSuccess()
		|| !RecoveryExecution.IsSuccess() || !Terminal.IsSuccess()
		|| !Fixture.Host.TryPeekNextInfluenceIntent(RemoveIntent))
	{
		return false;
	}
	const auto RemoveRequest = MakeHostExecutionRequest(
		RemoveIntent, 32);
	const auto RemoveRoute = Router.TryRoute(
		Fixture.Host, Fixture.Correlation, RemoveRequest);
	const auto RemoveExecution = Runtime.TryExecuteOne(
		Fixture.Host, Fixture.Correlation, RemoveRoute.Command);
	const auto Seal = Fixture.Host.TrySealInfluence(Fixture.Correlation);
	const auto End = Fixture.Host.TryEndAndTeardown(
		Fixture.World, Fixture.Correlation);
	const auto RemoveRouteReplay = Router.TryRoute(
		Fixture.Host, Fixture.Correlation, RemoveRequest);
	Fdemo_mapShanmenFormationInfluenceExecutionRouter SealedRouter;
	const auto SealedRecovery = SealedRouter.TryRoute(
		Fixture.Host, Fixture.Correlation, RemoveRequest);
	const auto SealedExecutionReplay = Runtime.TryExecuteOne(
		Fixture.Host, Fixture.Correlation, SealedRecovery.Command);

	TestTrue(TEXT("Retry keeps the intent pending and exact request replay is stable"),
		RetryRouteReplay.Status
			== Edemo_mapShanmenFormationInfluenceRouteStatus::RequestReplayed
			&& RetryRouteReplay.IsSuccess()
			&& HostExecutionCommandsMatch(
				RetryRoute.Command, RetryRouteReplay.Command)
			&& RecoveryRoute.IsSuccess()
			&& RetryRoute.Command.AttemptId
				!= RecoveryRoute.Command.AttemptId);
	TestTrue(TEXT("New request recovers retry and terminal Remove drains cleanly"),
		RecoveryExecution.IsSuccess() && RemoveRoute.IsSuccess()
			&& RemoveExecution.IsSuccess() && Seal.IsSuccess()
			&& End.Status == Edemo_mapShanmenFormationHostStatus::Ended
			&& RemoveRouteReplay.Status
				== Edemo_mapShanmenFormationInfluenceRouteStatus::
					RequestReplayed
			&& SealedRecovery.Status
				== Edemo_mapShanmenFormationInfluenceRouteStatus::
					HostEvidenceRecovered
			&& SealedExecutionReplay.IsSuccess()
			&& !SealedExecutionReplay.bExecutorInvoked
			&& Router.GetRecordCount() == 3 && Router.IsValid()
			&& SealedRouter.GetRecordCount() == 1
			&& SealedRouter.IsValid()
			&& Runtime.GetExecutorAttemptCount() == 2
			&& Runtime.GetCompletedIntentCount() == 2
			&& Runtime.GetActiveLeaseCount() == 0
			&& Fixture.Host.GetInfluenceLedger().IsSealed());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationInfluenceExecutionServiceSingleStepTest,
	"Shanmen.0_0_10.Product.FormationInfluenceExecutionService.SingleStepAndReplay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationInfluenceExecutionServiceSingleStepTest::RunTest(
	const FString&)
{
	FFormationHostFixture Fixture;
	FShanmenWorldEntityRegistry Registry;
	Fdemo_mapShanmenFormationHostInfluenceResult Prime;
	TArray<AActor*> Subjects;
	if (!PrimeHostExecutorInfluence(
			*this, Fixture, Registry, TEXT("ExecutionServiceSingleStep"),
			2, Prime, Subjects))
	{
		return false;
	}

	Fdemo_mapShanmenFormationInfluenceExecutionService Service;
	const auto FirstRequest = MakeHostExecutionRequest(
		Prime.ReconciliationPlan.Batch.Intents[0], 60);
	const auto First = Service.TryExecuteOne(
		Fixture.Host, Fixture.Correlation, FirstRequest);
	const auto Replay = Service.TryExecuteOne(
		Fixture.Host, Fixture.Correlation, FirstRequest);
	const auto SecondRequest = MakeHostExecutionRequest(
		Prime.ReconciliationPlan.Batch.Intents[1], 61);
	const auto Second = Service.TryExecuteOne(
		Fixture.Host, Fixture.Correlation, SecondRequest);

	TestTrue(TEXT("Service routes and executes one canonical intent per call"),
		First.IsSuccess() && First.Execution.bExecutorInvoked
			&& First.Route.Status
				== Edemo_mapShanmenFormationInfluenceRouteStatus::Routed
			&& Second.IsSuccess() && Second.Execution.bExecutorInvoked
			&& Service.IsBound() && Service.IsValid());
	TestTrue(TEXT("Exact request replay preserves both authority receipts"),
		Replay.IsSuccess() && !Replay.Execution.bExecutorInvoked
			&& Replay.Route.Status
				== Edemo_mapShanmenFormationInfluenceRouteStatus::
					RequestReplayed
			&& Replay.Execution.Status
				== Edemo_mapShanmenFormationInfluenceExecutionStatus::
					AttemptReplayed
			&& HostExecutionCommandsMatch(
				First.Route.Command, Replay.Route.Command));
	TestTrue(TEXT("Replay does not duplicate route or executor state"),
		Service.GetRouteRecordCount() == 2
			&& Service.GetExecutorAttemptCount() == 2
			&& Service.GetActiveLeaseCount() == 2
			&& Fixture.Host.GetPendingInfluenceIntentCount() == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationInfluenceExecutionServiceFenceTest,
	"Shanmen.0_0_10.Product.FormationInfluenceExecutionService.RouteAndBindingFence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationInfluenceExecutionServiceFenceTest::RunTest(
	const FString&)
{
	FFormationHostFixture FirstFixture;
	FFormationHostFixture OtherFixture;
	FShanmenWorldEntityRegistry FirstRegistry;
	FShanmenWorldEntityRegistry OtherRegistry;
	Fdemo_mapShanmenFormationHostInfluenceResult FirstPrime;
	Fdemo_mapShanmenFormationHostInfluenceResult OtherPrime;
	TArray<AActor*> FirstSubjects;
	TArray<AActor*> OtherSubjects;
	if (!PrimeHostExecutorInfluence(
			*this, FirstFixture, FirstRegistry,
			TEXT("ExecutionServiceFenceA"), 2,
			FirstPrime, FirstSubjects)
		|| !PrimeHostExecutorInfluence(
			*this, OtherFixture, OtherRegistry,
			TEXT("ExecutionServiceFenceB"), 1,
			OtherPrime, OtherSubjects))
	{
		return false;
	}

	const auto& FirstIntent = FirstPrime.ReconciliationPlan.Batch.Intents[0];
	const auto& SecondIntent = FirstPrime.ReconciliationPlan.Batch.Intents[1];
	Fdemo_mapShanmenFormationInfluenceExecutionService Service;
	const auto OutOfOrder = Service.TryExecuteOne(
		FirstFixture.Host, FirstFixture.Correlation,
		MakeHostExecutionRequest(SecondIntent, 70));
	const bool bUnboundAfterOrderFence =
		!Service.IsBound() && Service.GetRouteRecordCount() == 0;

	const auto FirstRequest = MakeHostExecutionRequest(FirstIntent, 71);
	const auto First = Service.TryExecuteOne(
		FirstFixture.Host, FirstFixture.Correlation, FirstRequest);
	auto ConflictRequest = FirstRequest;
	ConflictRequest.ExpectedIntentId = SecondIntent.IntentId;
	const auto Conflict = Service.TryExecuteOne(
		FirstFixture.Host, FirstFixture.Correlation, ConflictRequest);
	const auto Foreign = Service.TryExecuteOne(
		OtherFixture.Host, OtherFixture.Correlation,
		MakeHostExecutionRequest(
			OtherPrime.ReconciliationPlan.Batch.Intents[0], 72));
	const auto Second = Service.TryExecuteOne(
		FirstFixture.Host, FirstFixture.Correlation,
		MakeHostExecutionRequest(SecondIntent, 73));

	TestTrue(TEXT("Route rejection leaves an empty Service unbound"),
		OutOfOrder.Status
			== Edemo_mapShanmenFormationInfluenceServiceStatus::RouteRejected
			&& OutOfOrder.Route.Status
				== Edemo_mapShanmenFormationInfluenceRouteStatus::
					IntentOutOfOrder
			&& !OutOfOrder.bServiceStateCommitted
			&& bUnboundAfterOrderFence);
	TestTrue(TEXT("Request conflict and foreign Host fail before execution"),
		First.IsSuccess()
			&& Conflict.Status
				== Edemo_mapShanmenFormationInfluenceServiceStatus::
					RouteRejected
			&& Conflict.Route.Status
				== Edemo_mapShanmenFormationInfluenceRouteStatus::
					RequestConflict
			&& Foreign.Route.Status
				== Edemo_mapShanmenFormationInfluenceRouteStatus::
					BindingConflict
			&& !Conflict.bServiceStateCommitted
			&& !Foreign.bServiceStateCommitted
			&& OtherFixture.Host.GetPendingInfluenceIntentCount() == 1);
	TestTrue(TEXT("Canonical continuation preserves one shared binding"),
		Second.IsSuccess() && Service.IsBound() && Service.IsValid()
			&& Service.GetRouteRecordCount() == 2
			&& Service.GetExecutorAttemptCount() == 2
			&& Service.GetActiveLeaseCount() == 2
			&& FirstFixture.Host.GetPendingInfluenceIntentCount() == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationInfluenceExecutionServiceRecoveryTest,
	"Shanmen.0_0_10.Product.FormationInfluenceExecutionService.LateAttachmentAndRetryRecovery",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationInfluenceExecutionServiceRecoveryTest::RunTest(
	const FString&)
{
	FFormationHostFixture SuccessFixture;
	FShanmenWorldEntityRegistry SuccessRegistry;
	Fdemo_mapShanmenFormationHostInfluenceResult SuccessPrime;
	TArray<AActor*> SuccessSubjects;
	if (!PrimeHostExecutorInfluence(
			*this, SuccessFixture, SuccessRegistry,
			TEXT("ExecutionServiceLateAttach"), 1,
			SuccessPrime, SuccessSubjects))
	{
		return false;
	}
	const auto SuccessRequest = MakeHostExecutionRequest(
		SuccessPrime.ReconciliationPlan.Batch.Intents[0], 80);
	Fdemo_mapShanmenFormationInfluenceExecutionRouter SuccessRouter;
	Fdemo_mapShanmenFormationInfluenceProductRuntime SuccessRuntime;
	const auto SuccessRoute = SuccessRouter.TryRoute(
		SuccessFixture.Host, SuccessFixture.Correlation, SuccessRequest);
	const auto SuccessExecution = SuccessRuntime.TryExecuteOne(
		SuccessFixture.Host, SuccessFixture.Correlation,
		SuccessRoute.Command);
	Fdemo_mapShanmenFormationInfluenceExecutionService LateService;
	const auto LateAttach = LateService.TryExecuteOne(
		SuccessFixture.Host, SuccessFixture.Correlation, SuccessRequest);

	TestTrue(TEXT("Fresh Service cannot attach after successful Host history"),
		SuccessExecution.IsSuccess()
			&& LateAttach.Route.Status
				== Edemo_mapShanmenFormationInfluenceRouteStatus::
					HostEvidenceRecovered
			&& LateAttach.Status
				== Edemo_mapShanmenFormationInfluenceServiceStatus::
					ExecutionRejected
			&& LateAttach.Execution.Status
				== Edemo_mapShanmenFormationInfluenceExecutionStatus::StateInvalid
			&& !LateAttach.bServiceStateCommitted
			&& !LateService.IsBound() && LateService.IsValid()
			&& LateService.GetRouteRecordCount() == 0
			&& LateService.GetExecutorAttemptCount() == 0);

	FFormationHostFixture RetryFixture;
	FShanmenWorldEntityRegistry RetryRegistry;
	Fdemo_mapShanmenFormationHostInfluenceResult RetryPrime;
	TArray<AActor*> RetrySubjects;
	if (!PrimeHostExecutorInfluence(
			*this, RetryFixture, RetryRegistry,
			TEXT("ExecutionServiceRetryAttach"), 1,
			RetryPrime, RetrySubjects))
	{
		return false;
	}
	const auto& RetryIntent = RetryPrime.ReconciliationPlan.Batch.Intents[0];
	const auto RetryRequest = MakeHostExecutionRequest(RetryIntent, 81);
	Fdemo_mapShanmenFormationInfluenceExecutionRouter RetryRouter;
	const auto RetryRoute = RetryRouter.TryRoute(
		RetryFixture.Host, RetryFixture.Correlation, RetryRequest);
	Fdemo_mapShanmenFormationInfluenceAttemptCommand RetryCommand;
	RetryCommand.IntentId = RetryRoute.Command.IntentId;
	RetryCommand.AttemptId = RetryRoute.Command.AttemptId;
	RetryCommand.ExecutorReceiptId = FGuid(0xF8810001, 0, 0, 1);
	RetryCommand.Outcome =
		Edemo_mapShanmenFormationInfluenceAttemptOutcome::RetryableFailure;
	const auto RetryRecorded = RetryFixture.Host.TryAcknowledgeInfluence(
		RetryFixture.Correlation, RetryCommand);

	Fdemo_mapShanmenFormationInfluenceExecutionService RetryService;
	const auto RetryReplay = RetryService.TryExecuteOne(
		RetryFixture.Host, RetryFixture.Correlation, RetryRequest);
	const auto RetryRecovered = RetryService.TryExecuteOne(
		RetryFixture.Host, RetryFixture.Correlation,
		MakeHostExecutionRequest(RetryIntent, 82));

	TestTrue(TEXT("Retry-only Host history safely reconstructs Service state"),
		RetryRecorded.IsSuccess() && RetryReplay.IsSuccess()
			&& RetryReplay.Route.Status
				== Edemo_mapShanmenFormationInfluenceRouteStatus::
					HostEvidenceRecovered
			&& RetryReplay.Execution.Status
				== Edemo_mapShanmenFormationInfluenceExecutionStatus::
					AttemptReplayed
			&& !RetryReplay.Execution.bExecutorInvoked
			&& RetryService.IsBound() && RetryService.IsValid());
	TestTrue(TEXT("A new request recovers retry without duplicate execution"),
		RetryRecovered.IsSuccess()
			&& RetryRecovered.Execution.bExecutorInvoked
			&& RetryService.GetRouteRecordCount() == 2
			&& RetryService.GetExecutorAttemptCount() == 1
			&& RetryService.GetActiveLeaseCount() == 1
			&& RetryFixture.Host.GetPendingInfluenceIntentCount() == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationInfluenceExecutionServiceTerminalTest,
	"Shanmen.0_0_10.Product.FormationInfluenceExecutionService.TerminalDrainAndSealedReplay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationInfluenceExecutionServiceTerminalTest::RunTest(
	const FString&)
{
	FFormationHostFixture Fixture;
	FShanmenWorldEntityRegistry Registry;
	Fdemo_mapShanmenFormationHostInfluenceResult Prime;
	TArray<AActor*> Subjects;
	if (!PrimeHostExecutorInfluence(
			*this, Fixture, Registry, TEXT("ExecutionServiceTerminal"),
			1, Prime, Subjects))
	{
		return false;
	}

	Fdemo_mapShanmenFormationInfluenceExecutionService Service;
	const auto ApplyRequest = MakeHostExecutionRequest(
		Prime.ReconciliationPlan.Batch.Intents[0], 90);
	const auto Apply = Service.TryExecuteOne(
		Fixture.Host, Fixture.Correlation, ApplyRequest);
	const auto Terminal = Fixture.Host.TryPrepareTerminalInfluence(
		Fixture.Correlation);
	Fdemo_mapShanmenFormationInfluenceIntent RemoveIntent;
	if (!Apply.IsSuccess() || !Terminal.IsSuccess()
		|| !Fixture.Host.TryPeekNextInfluenceIntent(RemoveIntent))
	{
		return false;
	}
	const auto RemoveRequest = MakeHostExecutionRequest(
		RemoveIntent, 91);
	const auto Remove = Service.TryExecuteOne(
		Fixture.Host, Fixture.Correlation, RemoveRequest);
	const auto Seal = Fixture.Host.TrySealInfluence(Fixture.Correlation);
	const auto End = Fixture.Host.TryEndAndTeardown(
		Fixture.World, Fixture.Correlation);
	const auto RemoveReplay = Service.TryExecuteOne(
		Fixture.Host, Fixture.Correlation, RemoveRequest);
	Fdemo_mapShanmenFormationInfluenceExecutionService SealedService;
	const auto SealedAttach = SealedService.TryExecuteOne(
		Fixture.Host, Fixture.Correlation, RemoveRequest);

	TestTrue(TEXT("Service drains Apply and Remove before Host seal"),
		Remove.IsSuccess() && Seal.IsSuccess()
			&& End.Status == Edemo_mapShanmenFormationHostStatus::Ended
			&& Service.GetRouteRecordCount() == 2
			&& Service.GetExecutorAttemptCount() == 2
			&& Service.GetCompletedIntentCount() == 2
			&& Service.GetActiveLeaseCount() == 0
			&& Fixture.Host.GetInfluenceLedger().IsSealed());
	TestTrue(TEXT("Bound Service replays a sealed command without execution"),
		RemoveReplay.IsSuccess()
			&& RemoveReplay.Route.Status
				== Edemo_mapShanmenFormationInfluenceRouteStatus::
					RequestReplayed
			&& RemoveReplay.Execution.Status
				== Edemo_mapShanmenFormationInfluenceExecutionStatus::
					AttemptReplayed
			&& !RemoveReplay.Execution.bExecutorInvoked
			&& Service.GetExecutorAttemptCount() == 2
			&& Service.IsValid());
	TestTrue(TEXT("Fresh Service cannot adopt sealed success history"),
		SealedAttach.Route.Status
			== Edemo_mapShanmenFormationInfluenceRouteStatus::
				HostEvidenceRecovered
			&& SealedAttach.Status
				== Edemo_mapShanmenFormationInfluenceServiceStatus::
					ExecutionRejected
			&& !SealedAttach.bServiceStateCommitted
			&& !SealedService.IsBound() && SealedService.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationInfluenceLifecycleCoordinatorExplicitTest,
	"Shanmen.0_0_10.Product.FormationInfluenceLifecycleCoordinator.ExplicitLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationInfluenceLifecycleCoordinatorExplicitTest::RunTest(
	const FString&)
{
	FFormationHostFixture Fixture;
	FShanmenWorldEntityRegistry Registry;
	Fdemo_mapShanmenFormationHostInfluenceResult Prime;
	TArray<AActor*> Subjects;
	if (!PrimeHostExecutorInfluence(
			*this, Fixture, Registry, TEXT("LifecycleCoordinatorExplicit"),
			2, Prime, Subjects))
	{
		return false;
	}

	Fdemo_mapShanmenFormationInfluenceLifecycleCoordinator Coordinator;
	const auto FirstApply = Coordinator.TryExecuteStep(
		Fixture.Host, Fixture.Correlation,
		MakeHostExecutionRequest(
			Prime.ReconciliationPlan.Batch.Intents[0], 100));
	const auto SecondApply = Coordinator.TryExecuteStep(
		Fixture.Host, Fixture.Correlation,
		MakeHostExecutionRequest(
			Prime.ReconciliationPlan.Batch.Intents[1], 101));
	const auto Terminal = Coordinator.TryPrepareTerminal(
		Fixture.Host, Fixture.Correlation);
	Fdemo_mapShanmenFormationInfluenceIntent FirstRemoveIntent;
	if (!FirstApply.IsSuccess() || !SecondApply.IsSuccess()
		|| !Terminal.IsSuccess()
		|| !Fixture.Host.TryPeekNextInfluenceIntent(FirstRemoveIntent))
	{
		return false;
	}
	const auto FirstRemove = Coordinator.TryExecuteStep(
		Fixture.Host, Fixture.Correlation,
		MakeHostExecutionRequest(FirstRemoveIntent, 102));
	Fdemo_mapShanmenFormationInfluenceIntent SecondRemoveIntent;
	if (!FirstRemove.IsSuccess()
		|| !Fixture.Host.TryPeekNextInfluenceIntent(SecondRemoveIntent))
	{
		return false;
	}
	const auto SecondRemove = Coordinator.TryExecuteStep(
		Fixture.Host, Fixture.Correlation,
		MakeHostExecutionRequest(SecondRemoveIntent, 103));
	const auto Completed = Coordinator.TrySealAndEnd(
		Fixture.World, Fixture.Host, Fixture.Correlation);

	TestTrue(TEXT("Caller advances exactly one Apply or Remove per step"),
		FirstApply.Status
			== Edemo_mapShanmenFormationInfluenceLifecycleStatus::StepAccepted
			&& SecondApply.IsSuccess() && FirstRemove.IsSuccess()
			&& SecondRemove.IsSuccess()
			&& Terminal.Status
				== Edemo_mapShanmenFormationInfluenceLifecycleStatus::
					TerminalPrepared
			&& Terminal.TerminalPreparation.ReconciliationPlan.Batch.RemoveCount
				== 2);
	TestTrue(TEXT("Explicit drain permits one seal and terminal teardown"),
		Completed.Status
			== Edemo_mapShanmenFormationInfluenceLifecycleStatus::Completed
			&& Completed.IsSuccess()
			&& Coordinator.GetRouteRecordCount() == 4
			&& Coordinator.GetExecutorAttemptCount() == 4
			&& Coordinator.GetCompletedIntentCount() == 4
			&& Coordinator.GetActiveLeaseCount() == 0
			&& Fixture.Host.GetPendingInfluenceIntentCount() == 0
			&& Fixture.Host.GetInfluenceLedger().IsSealed()
			&& Coordinator.IsBound() && Coordinator.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationInfluenceLifecycleCoordinatorDrainFenceTest,
	"Shanmen.0_0_10.Product.FormationInfluenceLifecycleCoordinator.NoImplicitDrainAndOrderFence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationInfluenceLifecycleCoordinatorDrainFenceTest::RunTest(
	const FString&)
{
	FFormationHostFixture Fixture;
	FShanmenWorldEntityRegistry Registry;
	Fdemo_mapShanmenFormationHostInfluenceResult Prime;
	TArray<AActor*> Subjects;
	if (!PrimeHostExecutorInfluence(
			*this, Fixture, Registry, TEXT("LifecycleCoordinatorDrainFence"),
			2, Prime, Subjects))
	{
		return false;
	}

	Fdemo_mapShanmenFormationInfluenceLifecycleCoordinator Coordinator;
	const auto Terminal = Coordinator.TryPrepareTerminal(
		Fixture.Host, Fixture.Correlation);
	if (!Terminal.IsSuccess()
		|| Terminal.TerminalPreparation.ReconciliationPlan.Batch.Intents.Num()
			!= 2)
	{
		return false;
	}
	const int32 PendingAfterPrepare =
		Fixture.Host.GetPendingInfluenceIntentCount();
	const auto EarlyCompletion = Coordinator.TrySealAndEnd(
		Fixture.World, Fixture.Host, Fixture.Correlation);
	const bool bStayedActiveAfterEarlyCompletion =
		Fixture.Host.GetSession().GetState()
			== Edemo_mapShanmenFormationSessionState::Active;
	const FGuid FirstRemoveIntent =
		Terminal.TerminalPreparation.ReconciliationPlan.Batch.Intents[0].IntentId;
	const FGuid SecondRemoveIntent =
		Terminal.TerminalPreparation.ReconciliationPlan.Batch.Intents[1].IntentId;
	const auto EarlyRemove = Coordinator.TryExecuteStep(
		Fixture.Host, Fixture.Correlation,
		MakeHostExecutionRequest(FirstRemoveIntent, 110));
	const auto FirstApply = Coordinator.TryExecuteStep(
		Fixture.Host, Fixture.Correlation,
		MakeHostExecutionRequest(
			Prime.ReconciliationPlan.Batch.Intents[0], 111));
	const int32 PendingAfterOneStep =
		Fixture.Host.GetPendingInfluenceIntentCount();
	const auto SecondApply = Coordinator.TryExecuteStep(
		Fixture.Host, Fixture.Correlation,
		MakeHostExecutionRequest(
			Prime.ReconciliationPlan.Batch.Intents[1], 112));
	const auto FirstRemove = Coordinator.TryExecuteStep(
		Fixture.Host, Fixture.Correlation,
		MakeHostExecutionRequest(FirstRemoveIntent, 113));
	const auto SecondRemove = Coordinator.TryExecuteStep(
		Fixture.Host, Fixture.Correlation,
		MakeHostExecutionRequest(SecondRemoveIntent, 114));
	const auto Completed = Coordinator.TrySealAndEnd(
		Fixture.World, Fixture.Host, Fixture.Correlation);

	TestTrue(TEXT("Terminal preparation publishes but never drains intents"),
		PendingAfterPrepare == 4
			&& EarlyCompletion.Status
				== Edemo_mapShanmenFormationInfluenceLifecycleStatus::
					SealRejected
			&& !EarlyCompletion.bCoordinatorStateCommitted
			&& bStayedActiveAfterEarlyCompletion);
	TestTrue(TEXT("Out-of-order Remove never skips pending Apply"),
		EarlyRemove.Status
			== Edemo_mapShanmenFormationInfluenceLifecycleStatus::StepRejected
			&& EarlyRemove.Step.Route.Status
				== Edemo_mapShanmenFormationInfluenceRouteStatus::
					IntentOutOfOrder
			&& !EarlyRemove.bCoordinatorStateCommitted
			&& FirstApply.IsSuccess() && PendingAfterOneStep == 3);
	TestTrue(TEXT("Four explicit steps are required before completion"),
		SecondApply.IsSuccess() && FirstRemove.IsSuccess()
			&& SecondRemove.IsSuccess() && Completed.IsSuccess()
			&& Coordinator.GetExecutorAttemptCount() == 4
			&& Coordinator.GetRouteRecordCount() == 4
			&& Fixture.Host.GetPendingInfluenceIntentCount() == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationInfluenceLifecycleCoordinatorBindingTest,
	"Shanmen.0_0_10.Product.FormationInfluenceLifecycleCoordinator.BindingAndLateAttachmentFence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationInfluenceLifecycleCoordinatorBindingTest::RunTest(
	const FString&)
{
	FFormationHostFixture FirstFixture;
	FFormationHostFixture OtherFixture;
	FShanmenWorldEntityRegistry FirstRegistry;
	FShanmenWorldEntityRegistry OtherRegistry;
	Fdemo_mapShanmenFormationHostInfluenceResult FirstPrime;
	Fdemo_mapShanmenFormationHostInfluenceResult OtherPrime;
	TArray<AActor*> FirstSubjects;
	TArray<AActor*> OtherSubjects;
	if (!PrimeHostExecutorInfluence(
			*this, FirstFixture, FirstRegistry,
			TEXT("LifecycleCoordinatorBindingA"), 1,
			FirstPrime, FirstSubjects)
		|| !PrimeHostExecutorInfluence(
			*this, OtherFixture, OtherRegistry,
			TEXT("LifecycleCoordinatorBindingB"), 1,
			OtherPrime, OtherSubjects))
	{
		return false;
	}

	Fdemo_mapShanmenFormationInfluenceLifecycleCoordinator Coordinator;
	const auto Prepared = Coordinator.TryPrepareTerminal(
		FirstFixture.Host, FirstFixture.Correlation);
	const auto PreparedReplay = Coordinator.TryPrepareTerminal(
		FirstFixture.Host, FirstFixture.Correlation);
	const auto Foreign = Coordinator.TryPrepareTerminal(
		OtherFixture.Host, OtherFixture.Correlation);
	const auto Stale = Coordinator.TryExecuteStep(
		FirstFixture.Host, OtherFixture.Correlation,
		MakeHostExecutionRequest(
			FirstPrime.ReconciliationPlan.Batch.Intents[0], 120));

	const auto FirstApply = Coordinator.TryExecuteStep(
		FirstFixture.Host, FirstFixture.Correlation,
		MakeHostExecutionRequest(
			FirstPrime.ReconciliationPlan.Batch.Intents[0], 121));
	Fdemo_mapShanmenFormationInfluenceIntent FirstRemoveIntent;
	if (!FirstApply.IsSuccess()
		|| !FirstFixture.Host.TryPeekNextInfluenceIntent(FirstRemoveIntent))
	{
		return false;
	}
	const auto FirstRemove = Coordinator.TryExecuteStep(
		FirstFixture.Host, FirstFixture.Correlation,
		MakeHostExecutionRequest(FirstRemoveIntent, 122));
	const auto FirstCompleted = Coordinator.TrySealAndEnd(
		FirstFixture.World, FirstFixture.Host, FirstFixture.Correlation);

	const auto OtherRequest = MakeHostExecutionRequest(
		OtherPrime.ReconciliationPlan.Batch.Intents[0], 123);
	Fdemo_mapShanmenFormationInfluenceExecutionService ExternalService;
	const auto ExternalSuccess = ExternalService.TryExecuteOne(
		OtherFixture.Host, OtherFixture.Correlation, OtherRequest);
	Fdemo_mapShanmenFormationInfluenceLifecycleCoordinator LateCoordinator;
	const auto LateAttach = LateCoordinator.TryExecuteStep(
		OtherFixture.Host, OtherFixture.Correlation, OtherRequest);

	TestTrue(TEXT("First lifecycle operation freezes one Host binding"),
		Prepared.IsSuccess()
			&& PreparedReplay.Status
				== Edemo_mapShanmenFormationInfluenceLifecycleStatus::
					TerminalReplayed
			&& Foreign.Status
				== Edemo_mapShanmenFormationInfluenceLifecycleStatus::
					BindingConflict
			&& Stale.Status
				== Edemo_mapShanmenFormationInfluenceLifecycleStatus::
					CorrelationMismatch
			&& !OtherFixture.Host.IsInfluenceTerminalPrepared());
	TestTrue(TEXT("Bound lifecycle completes without crossing Host authority"),
		FirstRemove.IsSuccess() && FirstCompleted.IsSuccess()
			&& Coordinator.GetBoundLedgerId()
				== FirstFixture.Host.GetInfluenceLedger().GetLedgerId()
			&& Coordinator.IsValid());
	TestTrue(TEXT("Fresh Coordinator preserves the Service late-attach fence"),
		ExternalSuccess.IsSuccess()
			&& LateAttach.Status
				== Edemo_mapShanmenFormationInfluenceLifecycleStatus::
					StepRejected
			&& LateAttach.Step.Status
				== Edemo_mapShanmenFormationInfluenceServiceStatus::
					ExecutionRejected
			&& !LateAttach.bCoordinatorStateCommitted
			&& !LateCoordinator.IsBound() && LateCoordinator.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationInfluenceLifecycleCoordinatorRecoveryTest,
	"Shanmen.0_0_10.Product.FormationInfluenceLifecycleCoordinator.ForwardOnlyCompletionRecovery",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationInfluenceLifecycleCoordinatorRecoveryTest::RunTest(
	const FString&)
{
	FFormationHostFixture Fixture;
	FShanmenWorldEntityRegistry Registry;
	Fdemo_mapShanmenFormationHostInfluenceResult Prime;
	TArray<AActor*> Subjects;
	if (!PrimeHostExecutorInfluence(
			*this, Fixture, Registry, TEXT("LifecycleCoordinatorRecovery"),
			1, Prime, Subjects))
	{
		return false;
	}

	Fdemo_mapShanmenFormationInfluenceLifecycleCoordinator Coordinator;
	const auto ApplyRequest = MakeHostExecutionRequest(
		Prime.ReconciliationPlan.Batch.Intents[0], 130);
	const auto Apply = Coordinator.TryExecuteStep(
		Fixture.Host, Fixture.Correlation, ApplyRequest);
	const auto Terminal = Coordinator.TryPrepareTerminal(
		Fixture.Host, Fixture.Correlation);
	Fdemo_mapShanmenFormationInfluenceIntent RemoveIntent;
	if (!Apply.IsSuccess() || !Terminal.IsSuccess()
		|| !Fixture.Host.TryPeekNextInfluenceIntent(RemoveIntent))
	{
		return false;
	}
	const auto RemoveRequest = MakeHostExecutionRequest(
		RemoveIntent, 131);
	const auto Remove = Coordinator.TryExecuteStep(
		Fixture.Host, Fixture.Correlation, RemoveRequest);
	const auto FailedEnd = Coordinator.TrySealAndEnd(
		nullptr, Fixture.Host, Fixture.Correlation);
	const bool bForwardStateRetained =
		Fixture.Host.GetInfluenceLedger().IsSealed()
		&& Fixture.Host.GetSession().GetState()
			== Edemo_mapShanmenFormationSessionState::Ended
		&& Coordinator.IsBound() && Coordinator.IsValid();
	const auto Recovered = Coordinator.TrySealAndEnd(
		Fixture.World, Fixture.Host, Fixture.Correlation);
	const auto Replayed = Coordinator.TrySealAndEnd(
		Fixture.World, Fixture.Host, Fixture.Correlation);
	const auto RemoveReplay = Coordinator.TryExecuteStep(
		Fixture.Host, Fixture.Correlation, RemoveRequest);

	TestTrue(TEXT("Successful seal is retained when World teardown fails"),
		Remove.IsSuccess()
			&& FailedEnd.Status
				== Edemo_mapShanmenFormationInfluenceLifecycleStatus::EndRejected
			&& FailedEnd.Seal.Status
				== Edemo_mapShanmenFormationHostInfluenceStatus::Sealed
			&& FailedEnd.End.Status
				== Edemo_mapShanmenFormationHostStatus::
					TerminalRecoveryRequired
			&& FailedEnd.bCoordinatorStateCommitted
			&& bForwardStateRetained);
	TestTrue(TEXT("Exact retry completes and then replays terminal teardown"),
		Recovered.Status
			== Edemo_mapShanmenFormationInfluenceLifecycleStatus::Completed
			&& Recovered.IsSuccess()
			&& Replayed.Status
				== Edemo_mapShanmenFormationInfluenceLifecycleStatus::
					CompletionReplayed
			&& Replayed.IsSuccess());
	TestTrue(TEXT("Bound semantic state remains replayable after completion"),
		RemoveReplay.IsSuccess()
			&& RemoveReplay.Step.Route.Status
				== Edemo_mapShanmenFormationInfluenceRouteStatus::
					RequestReplayed
			&& RemoveReplay.Step.Execution.Status
				== Edemo_mapShanmenFormationInfluenceExecutionStatus::
					AttemptReplayed
			&& !RemoveReplay.Step.Execution.bExecutorInvoked
			&& Coordinator.GetExecutorAttemptCount() == 2
			&& Coordinator.GetActiveLeaseCount() == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationInfluenceLifecycleCommandRouterExplicitTest,
	"Shanmen.0_0_10.Product.FormationInfluenceLifecycleCommandRouter.TypedLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationInfluenceLifecycleCommandRouterExplicitTest::RunTest(
	const FString&)
{
	FFormationHostFixture Fixture;
	FShanmenWorldEntityRegistry Registry;
	Fdemo_mapShanmenFormationHostInfluenceResult Prime;
	TArray<AActor*> Subjects;
	if (!PrimeHostExecutorInfluence(
			*this, Fixture, Registry, TEXT("LifecycleCommandTyped"),
			2, Prime, Subjects))
	{
		return false;
	}

	Fdemo_mapShanmenFormationInfluenceLifecycleCommandRouter Router;
	const auto FirstApplyCommand = MakeLifecycleStepCommand(
		Fixture.Correlation,
		MakeHostExecutionRequest(
			Prime.ReconciliationPlan.Batch.Intents[0], 200));
	const auto SecondApplyCommand = MakeLifecycleStepCommand(
		Fixture.Correlation,
		MakeHostExecutionRequest(
			Prime.ReconciliationPlan.Batch.Intents[1], 201));
	const auto FirstApply = Router.TryRoute(
		nullptr, Fixture.Host, FirstApplyCommand);
	const auto SecondApply = Router.TryRoute(
		nullptr, Fixture.Host, SecondApplyCommand);
	const auto Terminal = Router.TryRoute(
		nullptr, Fixture.Host,
		MakeLifecycleTerminalCommand(Fixture.Correlation, 202));

	Fdemo_mapShanmenFormationInfluenceIntent FirstRemoveIntent;
	if (!FirstApply.IsSuccess() || !SecondApply.IsSuccess()
		|| !Terminal.IsSuccess()
		|| !Fixture.Host.TryPeekNextInfluenceIntent(FirstRemoveIntent))
	{
		return false;
	}
	const auto FirstRemove = Router.TryRoute(
		nullptr, Fixture.Host,
		MakeLifecycleStepCommand(
			Fixture.Correlation,
			MakeHostExecutionRequest(FirstRemoveIntent, 202)));
	Fdemo_mapShanmenFormationInfluenceIntent SecondRemoveIntent;
	if (!FirstRemove.IsSuccess()
		|| !Fixture.Host.TryPeekNextInfluenceIntent(SecondRemoveIntent))
	{
		return false;
	}
	const auto SecondRemove = Router.TryRoute(
		nullptr, Fixture.Host,
		MakeLifecycleStepCommand(
			Fixture.Correlation,
			MakeHostExecutionRequest(SecondRemoveIntent, 203)));
	const auto Completed = Router.TryRoute(
		Fixture.World, Fixture.Host,
		MakeLifecycleEndCommand(Fixture.Correlation, 205));

	TestTrue(TEXT("Typed commands preserve one-operation routing"),
		FirstApply.IsSuccess() && SecondApply.IsSuccess()
			&& Terminal.IsSuccess() && FirstRemove.IsSuccess()
			&& SecondRemove.IsSuccess() && Completed.IsSuccess()
			&& Router.GetRecordCount() == 6);
	TestTrue(TEXT("Typed lifecycle reaches the same explicit terminal state"),
		Router.IsValid() && Router.IsBound()
			&& Router.GetCoordinator().GetRouteRecordCount() == 4
			&& Router.GetCoordinator().GetExecutorAttemptCount() == 4
			&& Router.GetCoordinator().GetCompletedIntentCount() == 4
			&& Router.GetCoordinator().GetActiveLeaseCount() == 0
			&& Fixture.Host.GetPendingInfluenceIntentCount() == 0
			&& Fixture.Host.GetInfluenceLedger().IsSealed());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationInfluenceLifecycleCommandRouterIdentityTest,
	"Shanmen.0_0_10.Product.FormationInfluenceLifecycleCommandRouter.IdentityConflictAndReplay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationInfluenceLifecycleCommandRouterIdentityTest::RunTest(
	const FString&)
{
	FFormationHostFixture Fixture;
	FFormationHostFixture OtherFixture;
	FShanmenWorldEntityRegistry Registry;
	FShanmenWorldEntityRegistry OtherRegistry;
	Fdemo_mapShanmenFormationHostInfluenceResult Prime;
	Fdemo_mapShanmenFormationHostInfluenceResult OtherPrime;
	TArray<AActor*> Subjects;
	TArray<AActor*> OtherSubjects;
	if (!PrimeHostExecutorInfluence(
			*this, Fixture, Registry, TEXT("LifecycleCommandIdentity"),
			1, Prime, Subjects)
		|| !PrimeHostExecutorInfluence(
			*this, OtherFixture, OtherRegistry,
			TEXT("LifecycleCommandIdentityOther"),
			1, OtherPrime, OtherSubjects))
	{
		return false;
	}

	const auto& Intent = Prime.ReconciliationPlan.Batch.Intents[0];
	const auto Request = MakeHostExecutionRequest(Intent, 210);
	Fdemo_mapShanmenFormationInfluenceLifecycleCommand AliasedStep;
	const bool bAliasedStepCaptured =
		Fdemo_mapShanmenFormationInfluenceLifecycleCommand::TryCaptureStep(
			MakeLifecycleCommandId(299),
			Fixture.Correlation,
			Request,
			AliasedStep);
	const auto Command = MakeLifecycleStepCommand(
		Fixture.Correlation, Request);
	Fdemo_mapShanmenFormationInfluenceLifecycleCommandRouter Router;
	const auto Applied = Router.TryRoute(nullptr, Fixture.Host, Command);
	const int32 AttemptsAfterApply =
		Router.GetCoordinator().GetExecutorAttemptCount();
	const auto Replayed = Router.TryRoute(nullptr, Fixture.Host, Command);
	const auto ForeignReplay = Router.TryRoute(
		nullptr, OtherFixture.Host, Command);

	auto ConflictingRequest = Request;
	ConflictingRequest.ExpectedIntentId = FGuid(0xF8730001, 0, 0, 1);
	check(ConflictingRequest.IsValid());
	const auto PayloadConflict = Router.TryRoute(
		nullptr, Fixture.Host,
		MakeLifecycleStepCommand(Fixture.Correlation, ConflictingRequest));
	Fdemo_mapShanmenFormationInfluenceLifecycleCommand
		ConflictingKindCommand;
	check(Fdemo_mapShanmenFormationInfluenceLifecycleCommand::
		TryCapturePrepareTerminal(
			Request.RequestId,
			Fixture.Correlation,
			ConflictingKindCommand));
	const auto KindConflict = Router.TryRoute(
		nullptr, Fixture.Host, ConflictingKindCommand);
	Fdemo_mapShanmenFormationInfluenceLifecycleCommand InvalidCommand;
	const auto Invalid = Router.TryRoute(
		nullptr, Fixture.Host, InvalidCommand);

	TestTrue(TEXT("Exact command replay never re-enters the Coordinator"),
		Applied.IsSuccess() && Replayed.IsSuccess()
			&& Replayed.IsReplay()
			&& !Replayed.bRouterStateCommitted
			&& Router.GetCoordinator().GetExecutorAttemptCount()
				== AttemptsAfterApply
			&& Router.GetRecordCount() == 1);
	TestTrue(TEXT("Exact replay still requires the bound Host"),
		ForeignReplay.Status
			== Edemo_mapShanmenFormationInfluenceLifecycleCommandStatus::
				LifecycleRejected
			&& ForeignReplay.Lifecycle.Status
				== Edemo_mapShanmenFormationInfluenceLifecycleStatus::
					CorrelationMismatch
			&& !ForeignReplay.IsReplay()
			&& !ForeignReplay.bRouterStateCommitted);
	TestTrue(TEXT("CommandId freezes kind and nested request payload"),
		PayloadConflict.Status
			== Edemo_mapShanmenFormationInfluenceLifecycleCommandStatus::
				CommandIdConflict
			&& KindConflict.Status
				== Edemo_mapShanmenFormationInfluenceLifecycleCommandStatus::
					CommandIdConflict
			&& !PayloadConflict.bRouterStateCommitted
			&& !KindConflict.bRouterStateCommitted);
	TestTrue(TEXT("Invalid command fails before Router mutation"),
		!bAliasedStepCaptured && !AliasedStep.IsValid()
			&& Invalid.Status
			== Edemo_mapShanmenFormationInfluenceLifecycleCommandStatus::
				CommandInvalid
			&& Router.GetRecordCount() == 1 && Router.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationInfluenceLifecycleCommandRouterProgressTest,
	"Shanmen.0_0_10.Product.FormationInfluenceLifecycleCommandRouter.NoImplicitProgress",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationInfluenceLifecycleCommandRouterProgressTest::RunTest(
	const FString&)
{
	FFormationHostFixture Fixture;
	FShanmenWorldEntityRegistry Registry;
	Fdemo_mapShanmenFormationHostInfluenceResult Prime;
	TArray<AActor*> Subjects;
	if (!PrimeHostExecutorInfluence(
			*this, Fixture, Registry, TEXT("LifecycleCommandProgress"),
			1, Prime, Subjects))
	{
		return false;
	}

	Fdemo_mapShanmenFormationInfluenceLifecycleCommandRouter Router;
	const auto TerminalCommand =
		MakeLifecycleTerminalCommand(Fixture.Correlation, 220);
	const auto Terminal = Router.TryRoute(
		nullptr, Fixture.Host, TerminalCommand);
	if (!Terminal.IsSuccess())
	{
		return false;
	}
	const auto TerminalAlias = Router.TryRoute(
		nullptr, Fixture.Host,
		MakeLifecycleTerminalCommand(Fixture.Correlation, 224));
	const int32 PendingAfterTerminal =
		Fixture.Host.GetPendingInfluenceIntentCount();
	const auto EndCommand =
		MakeLifecycleEndCommand(Fixture.Correlation, 221);
	const auto EarlyEnd = Router.TryRoute(
		Fixture.World, Fixture.Host, EndCommand);
	const auto& RemoveIntent = Terminal.Lifecycle.TerminalPreparation.
		ReconciliationPlan.Batch.Intents[0];
	const auto RemoveCommand = MakeLifecycleStepCommand(
		Fixture.Correlation,
		MakeHostExecutionRequest(RemoveIntent, 220));
	const auto EarlyRemove = Router.TryRoute(
		nullptr, Fixture.Host, RemoveCommand);

	const auto Apply = Router.TryRoute(
		nullptr, Fixture.Host,
		MakeLifecycleStepCommand(
			Fixture.Correlation,
			MakeHostExecutionRequest(
				Prime.ReconciliationPlan.Batch.Intents[0], 221)));
	const auto Remove = Router.TryRoute(
		nullptr, Fixture.Host, RemoveCommand);
	const auto Completed = Router.TryRoute(
		Fixture.World, Fixture.Host, EndCommand);

	TestTrue(TEXT("Terminal command publishes but never drains"),
		PendingAfterTerminal == 2
			&& TerminalAlias.Status
				== Edemo_mapShanmenFormationInfluenceLifecycleCommandStatus::
					OperationIdentityConflict
			&& EarlyEnd.Status
				== Edemo_mapShanmenFormationInfluenceLifecycleCommandStatus::
					LifecycleRejected
			&& EarlyEnd.Lifecycle.Status
				== Edemo_mapShanmenFormationInfluenceLifecycleStatus::
					SealRejected
			&& !EarlyEnd.bRouterStateCommitted
			&& Router.GetCoordinator().GetExecutorAttemptCount() == 2);
	TestTrue(TEXT("Out-of-order Remove is not recorded or advanced"),
		EarlyRemove.Status
			== Edemo_mapShanmenFormationInfluenceLifecycleCommandStatus::
				LifecycleRejected
			&& EarlyRemove.Lifecycle.Step.Route.Status
				== Edemo_mapShanmenFormationInfluenceRouteStatus::
					IntentOutOfOrder
			&& !EarlyRemove.bRouterStateCommitted);
	TestTrue(TEXT("Caller may resubmit unchanged precondition-rejected commands"),
		Apply.IsSuccess() && Remove.IsSuccess() && Completed.IsSuccess()
			&& Router.GetRecordCount() == 4
			&& Fixture.Host.GetPendingInfluenceIntentCount() == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationInfluenceLifecycleCommandRouterRecoveryTest,
	"Shanmen.0_0_10.Product.FormationInfluenceLifecycleCommandRouter.ForwardCompletionRecovery",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationInfluenceLifecycleCommandRouterRecoveryTest::RunTest(
	const FString&)
{
	FFormationHostFixture Fixture;
	FShanmenWorldEntityRegistry Registry;
	Fdemo_mapShanmenFormationHostInfluenceResult Prime;
	TArray<AActor*> Subjects;
	if (!PrimeHostExecutorInfluence(
			*this, Fixture, Registry, TEXT("LifecycleCommandRecovery"),
			1, Prime, Subjects))
	{
		return false;
	}

	Fdemo_mapShanmenFormationInfluenceLifecycleCommandRouter Router;
	Fdemo_mapShanmenFormationInfluenceConsumerProductRuntime ConsumerRuntime;
	if (!Fdemo_mapShanmenFormationInfluenceConsumerProductRuntime::TryOpen(
			Fixture.Host, ConsumerRuntime))
	{
		return false;
	}
	const auto Apply = Router.TryRoute(
		nullptr, Fixture.Host,
		MakeLifecycleStepCommand(
			Fixture.Correlation,
			MakeHostExecutionRequest(
				Prime.ReconciliationPlan.Batch.Intents[0], 230)));
	const auto Terminal = Router.TryRoute(
		nullptr, Fixture.Host,
		MakeLifecycleTerminalCommand(Fixture.Correlation, 231));
	Fdemo_mapShanmenFormationInfluenceIntent RemoveIntent;
	if (!Apply.IsSuccess() || !Terminal.IsSuccess()
		|| !Fixture.Host.TryPeekNextInfluenceIntent(RemoveIntent))
	{
		return false;
	}
	const auto Remove = Router.TryRoute(
		nullptr, Fixture.Host,
		MakeLifecycleStepCommand(
			Fixture.Correlation,
			MakeHostExecutionRequest(RemoveIntent, 231)));
	const auto EndCommand =
		MakeLifecycleEndCommand(Fixture.Correlation, 233);
	const auto FailedEnd = Router.TryRouteWithConsumers(
		nullptr, Fixture.Host, ConsumerRuntime, EndCommand);
	const int32 RecordsAfterFailure = Router.GetRecordCount();
	const auto EndAlias = Router.TryRouteWithConsumers(
		Fixture.World, Fixture.Host, ConsumerRuntime,
		MakeLifecycleEndCommand(Fixture.Correlation, 234));
	const auto UnguardedRecovery = Router.TryRoute(
		Fixture.World, Fixture.Host, EndCommand);
	const auto Recovered = Router.TryRouteWithConsumers(
		Fixture.World, Fixture.Host, ConsumerRuntime, EndCommand);
	const auto Replayed = Router.TryRouteWithConsumers(
		Fixture.World, Fixture.Host, ConsumerRuntime, EndCommand);
	const auto Conflict = Router.TryRoute(
		nullptr, Fixture.Host,
		MakeLifecycleTerminalCommand(Fixture.Correlation, 233));

	TestTrue(TEXT("Forward-mutating end rejection locks command identity"),
		Remove.IsSuccess()
			&& FailedEnd.Status
				== Edemo_mapShanmenFormationInfluenceLifecycleCommandStatus::
					LifecycleRejected
			&& FailedEnd.Lifecycle.Status
				== Edemo_mapShanmenFormationInfluenceLifecycleStatus::EndRejected
			&& FailedEnd.bRouterStateCommitted
			&& RecordsAfterFailure == 4
			&& EndAlias.Status
				== Edemo_mapShanmenFormationInfluenceLifecycleCommandStatus::
					OperationIdentityConflict
			&& Router.IsValid());
	TestTrue(TEXT("Consumer-guarded recovery cannot drop its runtime boundary"),
		UnguardedRecovery.Status
			== Edemo_mapShanmenFormationInfluenceLifecycleCommandStatus::
				LifecycleRejected
			&& UnguardedRecovery.Lifecycle.Status
				== Edemo_mapShanmenFormationInfluenceLifecycleStatus::
					ConsumerTeardownRequired
			&& !UnguardedRecovery.bRouterStateCommitted
			&& Router.GetRecordCount() == RecordsAfterFailure);
	TestTrue(TEXT("Exact command explicitly recovers World teardown once"),
		Recovered.IsSuccess() && Recovered.bRecoveryAttempted
			&& Recovered.Lifecycle.bConsumerTeardownChecked
			&& Recovered.Lifecycle.ConsumerTeardown.IsSuccess()
			&& Recovered.bRouterStateCommitted
			&& Replayed.IsSuccess() && Replayed.IsReplay()
			&& !Replayed.bRecoveryAttempted
			&& Router.GetRecordCount() == 4
			&& Fixture.Host.GetInfluenceLedger().IsSealed());
	TestTrue(TEXT("Recovered CommandId remains payload-frozen"),
		Conflict.Status
			== Edemo_mapShanmenFormationInfluenceLifecycleCommandStatus::
				CommandIdConflict
			&& !Conflict.bRouterStateCommitted);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationInfluenceLifecycleCommandHostSubmissionTest,
	"Shanmen.0_0_10.Product.FormationInfluenceLifecycleCommandHost.SubmissionAndReceiptQuery",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationInfluenceLifecycleCommandHostSubmissionTest::RunTest(
	const FString&)
{
	FFormationHostFixture Fixture;
	FShanmenWorldEntityRegistry Registry;
	Fdemo_mapShanmenFormationHostInfluenceResult Prime;
	TArray<AActor*> Subjects;
	if (!PrimeHostExecutorInfluence(
			*this, Fixture, Registry, TEXT("LifecycleCommandHostSubmission"),
			1, Prime, Subjects))
	{
		return false;
	}

	Fdemo_mapShanmenFormationInfluenceLifecycleCommandHost CommandHost;
	if (!Fdemo_mapShanmenFormationInfluenceLifecycleCommandHost::TryOpen(
			Fixture.Host, CommandHost))
	{
		return false;
	}
	const auto Command = MakeLifecycleStepCommand(
		Fixture.Correlation,
		MakeHostExecutionRequest(
			Prime.ReconciliationPlan.Batch.Intents[0], 240));
	const auto First = CommandHost.TrySubmit(
		nullptr, Fixture.Host, Command);
	Fdemo_mapShanmenFormationInfluenceLifecycleCommandRecord Receipt;
	const bool bFound = CommandHost.TryGetReceipt(
		Command.GetCommandId(), Receipt);
	const auto Replay = CommandHost.TrySubmit(
		nullptr, Fixture.Host, Command);
	Fdemo_mapShanmenFormationInfluenceLifecycleCommandRecord ReceiptAfterReplay;
	const bool bFoundAfterReplay = CommandHost.TryGetReceipt(
		Command.GetCommandId(), ReceiptAfterReplay);

	TestTrue(TEXT("CommandHost freezes ProductHost identity without pointer ownership"),
		CommandHost.IsValid()
			&& CommandHost.GetCorrelation() == Fixture.Correlation
			&& CommandHost.GetLedgerId()
				== Fixture.Host.GetInfluenceLedger().GetLedgerId());
	TestTrue(TEXT("Accepted command exposes one frozen durable receipt"),
		First.IsSuccess() && bFound && Receipt.IsValid()
			&& Receipt.Command.Matches(Command)
			&& Receipt.Result.IsSuccess()
			&& !Receipt.Result.IsReplay()
			&& Receipt.Result.bRouterStateCommitted
			&& CommandHost.GetReceiptCount() == 1);
	TestTrue(TEXT("Exact submit replay does not rewrite the queried receipt"),
		Replay.IsSuccess() && Replay.IsReplay()
			&& bFoundAfterReplay && ReceiptAfterReplay.IsValid()
			&& ReceiptAfterReplay.Command.Matches(Receipt.Command)
			&& !ReceiptAfterReplay.Result.IsReplay()
			&& ReceiptAfterReplay.Result.bRouterStateCommitted
			&& CommandHost.GetReceiptCount() == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationInfluenceLifecycleCommandHostBindingTest,
	"Shanmen.0_0_10.Product.FormationInfluenceLifecycleCommandHost.BindingAndForeignHostFence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationInfluenceLifecycleCommandHostBindingTest::RunTest(
	const FString&)
{
	FFormationHostFixture Fixture;
	FFormationHostFixture OtherFixture;
	FShanmenWorldEntityRegistry Registry;
	FShanmenWorldEntityRegistry OtherRegistry;
	Fdemo_mapShanmenFormationHostInfluenceResult Prime;
	Fdemo_mapShanmenFormationHostInfluenceResult OtherPrime;
	TArray<AActor*> Subjects;
	TArray<AActor*> OtherSubjects;
	if (!PrimeHostExecutorInfluence(
			*this, Fixture, Registry, TEXT("LifecycleCommandHostBinding"),
			1, Prime, Subjects)
		|| !PrimeHostExecutorInfluence(
			*this, OtherFixture, OtherRegistry,
			TEXT("LifecycleCommandHostBindingOther"),
			1, OtherPrime, OtherSubjects))
	{
		return false;
	}

	Fdemo_mapShanmenFormationInfluenceLifecycleCommandHost InvalidOpen;
	Fdemo_mapShanmenFormationProductHost EmptyProductHost;
	const bool bOpenedInvalid =
		Fdemo_mapShanmenFormationInfluenceLifecycleCommandHost::TryOpen(
			EmptyProductHost, InvalidOpen);
	Fdemo_mapShanmenFormationInfluenceLifecycleCommandHost CommandHost;
	check(Fdemo_mapShanmenFormationInfluenceLifecycleCommandHost::TryOpen(
		Fixture.Host, CommandHost));
	const auto Command = MakeLifecycleStepCommand(
		Fixture.Correlation,
		MakeHostExecutionRequest(
			Prime.ReconciliationPlan.Batch.Intents[0], 250));
	const auto ForeignBeforeBinding = CommandHost.TrySubmit(
		nullptr, OtherFixture.Host, Command);
	const int32 ReceiptCountAfterForeign = CommandHost.GetReceiptCount();
	const bool bRouterBoundAfterForeign = CommandHost.GetRouter().IsBound();
	Fdemo_mapShanmenFormationInfluenceLifecycleCommand InvalidCommand;
	const auto Invalid = CommandHost.TrySubmit(
		nullptr, Fixture.Host, InvalidCommand);
	const auto Applied = CommandHost.TrySubmit(
		nullptr, Fixture.Host, Command);
	const auto ForeignReplay = CommandHost.TrySubmit(
		nullptr, OtherFixture.Host, Command);
	Fdemo_mapShanmenFormationInfluenceLifecycleCommandRecord Receipt;

	TestTrue(TEXT("Invalid ProductHost cannot open a command session"),
		!bOpenedInvalid && !InvalidOpen.IsValid());
	TestTrue(TEXT("Foreign ProductHost fails before Router binding or mutation"),
		ForeignBeforeBinding.Status
			== Edemo_mapShanmenFormationInfluenceLifecycleCommandStatus::
				LifecycleRejected
			&& ForeignBeforeBinding.Lifecycle.Status
				== Edemo_mapShanmenFormationInfluenceLifecycleStatus::
					CorrelationMismatch
			&& ReceiptCountAfterForeign == 0
			&& !bRouterBoundAfterForeign);
	TestTrue(TEXT("Invalid command preserves Router command semantics"),
		Invalid.Status
			== Edemo_mapShanmenFormationInfluenceLifecycleCommandStatus::
				CommandInvalid
			&& !Invalid.bRouterStateCommitted);
	TestTrue(TEXT("Only the frozen ProductHost can submit or replay"),
		Applied.IsSuccess()
			&& ForeignReplay.Status
				== Edemo_mapShanmenFormationInfluenceLifecycleCommandStatus::
					LifecycleRejected
			&& ForeignReplay.Lifecycle.Status
				== Edemo_mapShanmenFormationInfluenceLifecycleStatus::
					CorrelationMismatch
			&& !ForeignReplay.IsReplay()
			&& CommandHost.TryGetReceipt(Command.GetCommandId(), Receipt)
			&& Receipt.Result.IsSuccess());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationInfluenceLifecycleCommandHostVisibilityTest,
	"Shanmen.0_0_10.Product.FormationInfluenceLifecycleCommandHost.RejectionVisibility",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationInfluenceLifecycleCommandHostVisibilityTest::RunTest(
	const FString&)
{
	FFormationHostFixture Fixture;
	FShanmenWorldEntityRegistry Registry;
	Fdemo_mapShanmenFormationHostInfluenceResult Prime;
	TArray<AActor*> Subjects;
	if (!PrimeHostExecutorInfluence(
			*this, Fixture, Registry, TEXT("LifecycleCommandHostVisibility"),
			1, Prime, Subjects))
	{
		return false;
	}

	Fdemo_mapShanmenFormationInfluenceLifecycleCommandHost CommandHost;
	check(Fdemo_mapShanmenFormationInfluenceLifecycleCommandHost::TryOpen(
		Fixture.Host, CommandHost));
	const auto TerminalCommand =
		MakeLifecycleTerminalCommand(Fixture.Correlation, 260);
	const auto Terminal = CommandHost.TrySubmit(
		nullptr, Fixture.Host, TerminalCommand);
	if (!Terminal.IsSuccess())
	{
		return false;
	}
	const auto EndCommand =
		MakeLifecycleEndCommand(Fixture.Correlation, 261);
	const auto EarlyEnd = CommandHost.TrySubmit(
		Fixture.World, Fixture.Host, EndCommand);
	const auto& RemoveIntent = Terminal.Lifecycle.TerminalPreparation.
		ReconciliationPlan.Batch.Intents[0];
	const auto RemoveCommand = MakeLifecycleStepCommand(
		Fixture.Correlation,
		MakeHostExecutionRequest(RemoveIntent, 260));
	const auto EarlyRemove = CommandHost.TrySubmit(
		nullptr, Fixture.Host, RemoveCommand);
	Fdemo_mapShanmenFormationInfluenceLifecycleCommandRecord RejectedReceipt;
	const bool bEarlyEndQueryable = CommandHost.TryGetReceipt(
		EndCommand.GetCommandId(), RejectedReceipt);
	const bool bEarlyRemoveQueryable = CommandHost.TryGetReceipt(
		RemoveCommand.GetCommandId(), RejectedReceipt);

	const auto ApplyCommand = MakeLifecycleStepCommand(
		Fixture.Correlation,
		MakeHostExecutionRequest(
			Prime.ReconciliationPlan.Batch.Intents[0], 261));
	const auto Apply = CommandHost.TrySubmit(
		nullptr, Fixture.Host, ApplyCommand);
	const auto Remove = CommandHost.TrySubmit(
		nullptr, Fixture.Host, RemoveCommand);
	const auto Ended = CommandHost.TrySubmit(
		Fixture.World, Fixture.Host, EndCommand);
	Fdemo_mapShanmenFormationInfluenceLifecycleCommandRecord EndReceipt;

	TestTrue(TEXT("Ordinary precondition rejection is not a durable receipt"),
		EarlyEnd.Status
			== Edemo_mapShanmenFormationInfluenceLifecycleCommandStatus::
				LifecycleRejected
			&& EarlyRemove.Status
				== Edemo_mapShanmenFormationInfluenceLifecycleCommandStatus::
					LifecycleRejected
			&& !bEarlyEndQueryable && !bEarlyRemoveQueryable);
	TestTrue(TEXT("The same caller commands become queryable after order repair"),
		Apply.IsSuccess() && Remove.IsSuccess() && Ended.IsSuccess()
			&& CommandHost.TryGetReceipt(
				EndCommand.GetCommandId(), EndReceipt)
			&& EndReceipt.Result.IsSuccess()
			&& CommandHost.GetReceiptCount() == 4);
	TestTrue(TEXT("Unknown identity clears and rejects receipt output"),
		!CommandHost.TryGetReceipt(FGuid(0xF8740001, 0, 0, 1),
			RejectedReceipt)
			&& !RejectedReceipt.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationInfluenceLifecycleCommandHostRecoveryTest,
	"Shanmen.0_0_10.Product.FormationInfluenceLifecycleCommandHost.ForwardReceiptUpdate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationInfluenceLifecycleCommandHostRecoveryTest::RunTest(
	const FString&)
{
	FFormationHostFixture Fixture;
	FShanmenWorldEntityRegistry Registry;
	Fdemo_mapShanmenFormationHostInfluenceResult Prime;
	TArray<AActor*> Subjects;
	if (!PrimeHostExecutorInfluence(
			*this, Fixture, Registry, TEXT("LifecycleCommandHostRecovery"),
			1, Prime, Subjects))
	{
		return false;
	}

	Fdemo_mapShanmenFormationInfluenceLifecycleCommandHost CommandHost;
	check(Fdemo_mapShanmenFormationInfluenceLifecycleCommandHost::TryOpen(
		Fixture.Host, CommandHost));
	const auto Apply = CommandHost.TrySubmit(
		nullptr, Fixture.Host,
		MakeLifecycleStepCommand(
			Fixture.Correlation,
			MakeHostExecutionRequest(
				Prime.ReconciliationPlan.Batch.Intents[0], 270)));
	const auto Terminal = CommandHost.TrySubmit(
		nullptr, Fixture.Host,
		MakeLifecycleTerminalCommand(Fixture.Correlation, 271));
	Fdemo_mapShanmenFormationInfluenceIntent RemoveIntent;
	if (!Apply.IsSuccess() || !Terminal.IsSuccess()
		|| !Fixture.Host.TryPeekNextInfluenceIntent(RemoveIntent))
	{
		return false;
	}
	const auto Remove = CommandHost.TrySubmit(
		nullptr, Fixture.Host,
		MakeLifecycleStepCommand(
			Fixture.Correlation,
			MakeHostExecutionRequest(RemoveIntent, 271)));
	const auto EndCommand =
		MakeLifecycleEndCommand(Fixture.Correlation, 272);
	const auto Failed = CommandHost.TrySubmit(
		nullptr, Fixture.Host, EndCommand);
	Fdemo_mapShanmenFormationInfluenceLifecycleCommandRecord FailedReceipt;
	const bool bFoundFailed = CommandHost.TryGetReceipt(
		EndCommand.GetCommandId(), FailedReceipt);
	const auto Recovered = CommandHost.TrySubmit(
		Fixture.World, Fixture.Host, EndCommand);
	Fdemo_mapShanmenFormationInfluenceLifecycleCommandRecord RecoveredReceipt;
	const bool bFoundRecovered = CommandHost.TryGetReceipt(
		EndCommand.GetCommandId(), RecoveredReceipt);
	const auto Replay = CommandHost.TrySubmit(
		Fixture.World, Fixture.Host, EndCommand);

	TestTrue(TEXT("Forward World failure is immediately queryable"),
		Remove.IsSuccess() && Failed.Status
			== Edemo_mapShanmenFormationInfluenceLifecycleCommandStatus::
				LifecycleRejected
			&& Failed.Lifecycle.Status
				== Edemo_mapShanmenFormationInfluenceLifecycleStatus::EndRejected
			&& bFoundFailed && FailedReceipt.IsValid()
			&& FailedReceipt.Result.Lifecycle.Status
				== Edemo_mapShanmenFormationInfluenceLifecycleStatus::EndRejected);
	TestTrue(TEXT("Exact recovery updates the same durable receipt in place"),
		Recovered.IsSuccess() && Recovered.bRecoveryAttempted
			&& bFoundRecovered && RecoveredReceipt.IsValid()
			&& RecoveredReceipt.Result.IsSuccess()
			&& !RecoveredReceipt.Result.bRecoveryAttempted
			&& RecoveredReceipt.Command.Matches(EndCommand)
			&& CommandHost.GetReceiptCount() == 4);
	TestTrue(TEXT("Completed receipt replay remains read-only"),
		Replay.IsSuccess() && Replay.IsReplay()
			&& CommandHost.GetReceiptCount() == 4
			&& CommandHost.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationInfluenceLifecycleCommandHostConsumerFenceTest,
	"Shanmen.0_0_10.Product.FormationInfluenceLifecycleCommandHost.ConsumerTeardownFence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationInfluenceLifecycleCommandHostConsumerFenceTest::
RunTest(const FString&)
{
	FFormationHostFixture Fixture;
	FShanmenWorldEntityRegistry Registry;
	Fdemo_mapShanmenFormationHostInfluenceResult Prime;
	TArray<AActor*> Subjects;
	if (!PrimeHostExecutorInfluence(
			*this, Fixture, Registry,
			TEXT("LifecycleCommandHostConsumerFence"), 1, Prime, Subjects))
	{
		return false;
	}

	Fdemo_mapShanmenFormationInfluenceLifecycleCommandHost CommandHost;
	Fdemo_mapShanmenFormationInfluenceLifecycleCommandHost ForeignCommandHost;
	if (!Fdemo_mapShanmenFormationInfluenceLifecycleCommandHost::TryOpen(
			Fixture.Host, CommandHost)
		|| !Fdemo_mapShanmenFormationInfluenceLifecycleCommandHost::TryOpen(
			Fixture.Host, ForeignCommandHost))
	{
		return false;
	}
	const auto& ApplyIntent = Prime.ReconciliationPlan.Batch.Intents[0];
	const auto Apply = CommandHost.TrySubmit(
		nullptr, Fixture.Host,
		MakeLifecycleStepCommand(
			Fixture.Correlation,
			MakeHostExecutionRequest(ApplyIntent, 280)));
	FHostConsumerCommands ConsumerCommands;
	if (!Apply.IsSuccess()
		|| !BuildHostConsumerCommands(
			CommandHost, Apply.CommandId, ApplyIntent, ConsumerCommands))
	{
		return false;
	}
	const auto DeliveryReplay = CommandHost.TryPrepareConsumerCommands(
		Apply.CommandId, ConsumerCommands.Delivery.Delivery.Definition);
	const auto InvalidDeliveryId = CommandHost.TryPrepareConsumerCommands(
		FGuid(), ConsumerCommands.Delivery.Delivery.Definition);
	const auto MissingDelivery = CommandHost.TryPrepareConsumerCommands(
		FGuid(0xF8400F35, 0, 0, 1),
		ConsumerCommands.Delivery.Delivery.Definition);
	Fdemo_mapShanmenFormationInfluenceConsumerDefinition ForeignDefinition;
	FShanmenContentStamp ForeignContent = ApplyIntent.Content;
	ForeignContent.Digest += TEXT("-foreign");
	if (!Fdemo_mapShanmenFormationInfluenceConsumerDefinition::
		TryCreateOffensePowerAdditive(
			TEXT("Formation.Consumer.ProductLifecycle.Foreign"),
			100, 40, ForeignContent, ForeignDefinition))
	{
		return false;
	}
	const auto ProjectionRejected = CommandHost.TryPrepareConsumerCommands(
		Apply.CommandId, ForeignDefinition);

	TestTrue(TEXT("Lifecycle receipt prepares one immutable consumer delivery"),
		ConsumerCommands.Delivery.IsSuccess()
			&& ConsumerCommands.Delivery.SourceReceipt.Command.GetCommandId()
				== Apply.CommandId
			&& ConsumerCommands.Delivery.Delivery.LifecycleCommandId
				== Apply.CommandId
			&& ConsumerCommands.Delivery.Delivery.SubjectEntityId
				== ApplyIntent.SubjectEntityId
			&& ConsumerCommands.Delivery.Delivery.AuthoritativeLease.LeaseId
				== ConsumerCommands.Apply.GetProjection().GetLease().LeaseId
			&& ConsumerCommands.Delivery.Delivery.Apply.Matches(
				ConsumerCommands.Apply)
			&& ConsumerCommands.Delivery.Delivery.Remove.Matches(
				ConsumerCommands.Remove));
	TestTrue(TEXT("Repeated delivery is read-only and byte-stable by identity"),
		DeliveryReplay.IsSuccess()
			&& DeliveryReplay.Delivery.Apply.Matches(ConsumerCommands.Apply)
			&& DeliveryReplay.Delivery.Remove.Matches(ConsumerCommands.Remove)
			&& CommandHost.GetReceiptCount() == 1
			&& CommandHost.GetConsumerRuntime().GetBindingCount() == 0
			&& CommandHost.GetConsumerRuntime().
				GetActiveApplicationCount() == 0
			&& CommandHost.GetConsumerRuntime().
				GetCompletedTransactionCount() == 0);
	TestTrue(TEXT("Delivery rejects invalid or foreign source evidence"),
		InvalidDeliveryId.Status
			== Edemo_mapShanmenFormationInfluenceConsumerDeliveryStatus::
				LifecycleCommandIdInvalid
			&& MissingDelivery.Status
				== Edemo_mapShanmenFormationInfluenceConsumerDeliveryStatus::
					LifecycleReceiptNotFound
			&& ProjectionRejected.Status
				== Edemo_mapShanmenFormationInfluenceConsumerDeliveryStatus::
					ProjectionRejected
			&& ProjectionRejected.ProjectionAttempt.Status
				== Edemo_mapShanmenFormationInfluenceConsumerProjectionStatus::
					ContentMismatch
			&& CommandHost.GetReceiptCount() == 1);

	Udemo_mapAttributeComponent* Attributes =
		NewObject<Udemo_mapAttributeComponent>();
	Udemo_mapAttributeComponent* ForeignAttributes =
		NewObject<Udemo_mapAttributeComponent>();
	const FGuid ForeignSubjectEntityId(0xF8400F36, 0, 0, 1);
	if (Registry.BindObject(
			Fixture.Correlation.ActiveRunId,
			Attributes,
			ConsumerCommands.SubjectEntityId)
			!= EShanmenWorldBindingResult::Bound
		|| Registry.BindObject(
			Fixture.Correlation.ActiveRunId,
			ForeignAttributes,
			ForeignSubjectEntityId)
			!= EShanmenWorldBindingResult::Bound)
	{
		return false;
	}
	const auto SubjectWorldResolution =
		Fdemo_mapShanmenFormationInfluenceConsumerWorldResolver::Resolve(
			Registry, Fixture.Correlation.ActiveRunId, Attributes);
	const auto ForeignSubjectWorldResolution =
		Fdemo_mapShanmenFormationInfluenceConsumerWorldResolver::Resolve(
			Registry, Fixture.Correlation.ActiveRunId, ForeignAttributes);
	if (!SubjectWorldResolution.IsSuccess()
		|| !ForeignSubjectWorldResolution.IsSuccess())
	{
		return false;
	}
	const auto& SubjectResolution = SubjectWorldResolution.Resolution;
	const auto& ForeignSubjectResolution =
		ForeignSubjectWorldResolution.Resolution;
	const auto ForeignActivation =
		ForeignCommandHost.TryActivateConsumerDelivery(
			Fixture.Host,
			ConsumerCommands.Delivery.Delivery,
			SubjectResolution,
			Attributes);
	const auto InvalidResolution = CommandHost.TryActivateConsumerDelivery(
		Fixture.Host,
		ConsumerCommands.Delivery.Delivery,
		Fdemo_mapShanmenFormationInfluenceConsumerSubjectResolution(),
		Attributes);
	const auto SubjectMismatch = CommandHost.TryActivateConsumerDelivery(
		Fixture.Host,
		ConsumerCommands.Delivery.Delivery,
		ForeignSubjectResolution,
		ForeignAttributes);
	const auto ComponentUnavailable =
		CommandHost.TryActivateConsumerDelivery(
			Fixture.Host,
			ConsumerCommands.Delivery.Delivery,
			SubjectResolution,
			nullptr);
	const auto ComponentMismatch = CommandHost.TryActivateConsumerDelivery(
		Fixture.Host,
		ConsumerCommands.Delivery.Delivery,
		SubjectResolution,
		ForeignAttributes);
	const auto InvalidDelivery = CommandHost.TryActivateConsumerDelivery(
		Fixture.Host,
		Fdemo_mapShanmenFormationInfluenceConsumerCommandDelivery(),
		SubjectResolution,
		Attributes);
	const int32 RejectedBindingCount =
		CommandHost.GetConsumerRuntime().GetBindingCount();
	const int32 RejectedApplicationCount =
		CommandHost.GetConsumerRuntime().GetActiveApplicationCount();
	const auto Activated = CommandHost.TryActivateConsumerDelivery(
		Fixture.Host,
		ConsumerCommands.Delivery.Delivery,
		SubjectResolution,
		Attributes);
	const auto ActivationReplay = CommandHost.TryActivateConsumerDelivery(
		Fixture.Host,
		ConsumerCommands.Delivery.Delivery,
		SubjectResolution,
		Attributes);
	const auto Terminal = CommandHost.TrySubmit(
		nullptr, Fixture.Host,
		MakeLifecycleTerminalCommand(Fixture.Correlation, 281));
	auto ReceiptMismatchedDelivery = ConsumerCommands.Delivery.Delivery;
	ReceiptMismatchedDelivery.LifecycleCommandId = Terminal.CommandId;
	const auto ReceiptMismatch = CommandHost.TryActivateConsumerDelivery(
		Fixture.Host,
		ReceiptMismatchedDelivery,
		SubjectResolution,
		Attributes);
	const auto NonApplyDelivery = CommandHost.TryPrepareConsumerCommands(
		Terminal.CommandId, ConsumerCommands.Delivery.Delivery.Definition);
	Fdemo_mapShanmenFormationInfluenceIntent RemoveIntent;
	if (!Activated.IsSuccess()
		|| !Terminal.IsSuccess()
		|| !Fixture.Host.TryPeekNextInfluenceIntent(RemoveIntent))
	{
		return false;
	}
	const auto RemoveCommand = MakeLifecycleStepCommand(
		Fixture.Correlation,
		MakeHostExecutionRequest(RemoveIntent, 282));
	const auto BlockedRemove = CommandHost.TrySubmit(
		nullptr, Fixture.Host,
		RemoveCommand);
	Fdemo_mapShanmenFormationInfluenceLifecycleCommandRecord
		BlockedRemoveReceipt;
	const bool bBlockedRemoveStored = CommandHost.TryGetReceipt(
		RemoveCommand.GetCommandId(), BlockedRemoveReceipt);

	TestTrue(TEXT("A parallel caller cannot consume another Host's lease"),
		ForeignActivation.Status
			== Edemo_mapShanmenFormationInfluenceConsumerDeliveryApplicationStatus::
				SourceReceiptNotFound
			&& !ForeignActivation.bSourceReceiptChecked
			&& ForeignCommandHost.GetConsumerRuntime().IsDrained());
	TestTrue(TEXT("Subject resolution failures are explicit and side-effect free"),
		InvalidResolution.Status
			== Edemo_mapShanmenFormationInfluenceConsumerDeliveryApplicationStatus::
				SubjectResolutionInvalid
			&& InvalidResolution.bSourceReceiptChecked
			&& InvalidResolution.bSubjectResolutionChecked
			&& SubjectMismatch.Status
				== Edemo_mapShanmenFormationInfluenceConsumerDeliveryApplicationStatus::
					SubjectMismatch
			&& ComponentUnavailable.Status
				== Edemo_mapShanmenFormationInfluenceConsumerDeliveryApplicationStatus::
					AttributeComponentUnavailable
			&& ComponentMismatch.Status
				== Edemo_mapShanmenFormationInfluenceConsumerDeliveryApplicationStatus::
					AttributeComponentMismatch
			&& InvalidDelivery.Status
				== Edemo_mapShanmenFormationInfluenceConsumerDeliveryApplicationStatus::
					DeliveryInvalid
			&& RejectedBindingCount == 0
			&& RejectedApplicationCount == 0);
	TestTrue(TEXT("Delivery application binds exact durable source evidence"),
		ReceiptMismatch.Status
			== Edemo_mapShanmenFormationInfluenceConsumerDeliveryApplicationStatus::
				SourceReceiptRejected
			&& ReceiptMismatch.bSourceReceiptChecked
			&& ReceiptMismatch.SourceReceipt.Command.GetCommandId()
				== Terminal.CommandId);
	TestTrue(TEXT("Native consumer is active under exact lease authority"),
		SubjectWorldResolution.bRegistryChecked
			&& SubjectWorldResolution.ResolvedEntityId
				== ConsumerCommands.SubjectEntityId
			&& Activated.IsSuccess() && Activated.bSourceReceiptChecked
			&& Activated.bSubjectResolutionChecked
			&& Activated.SubjectResolution.ResolutionId
				== SubjectResolution.ResolutionId
			&& Activated.SubjectResolution.AttributeComponentUniqueId
				== Attributes->GetUniqueID()
			&& Activated.Runtime.bLeaseAuthorityChecked
			&& Activated.Runtime.AuthoritativeLease.IsValid()
			&& Activated.Runtime.AuthoritativeLease.LeaseId
				== ConsumerCommands.Apply.GetProjection().GetLease().LeaseId
			&& ActivationReplay.IsSuccess()
			&& ActivationReplay.Status
				== Edemo_mapShanmenFormationInfluenceConsumerDeliveryApplicationStatus::
					ActivationReplayed
			&& ActivationReplay.SubjectResolution.ResolutionId
				== SubjectResolution.ResolutionId
			&& Attributes
			&& Attributes->GetActiveModifierCount() == 1
			&& CommandHost.GetConsumerRuntime().GetBindingCount() == 1
			&& CommandHost.GetConsumerRuntime().
				GetActiveApplicationCount() == 1);
	TestTrue(TEXT("Non-step lifecycle receipts cannot produce consumers"),
		NonApplyDelivery.Status
			== Edemo_mapShanmenFormationInfluenceConsumerDeliveryStatus::
				LifecycleOperationMismatch);
	TestTrue(TEXT("Authoritative Remove waits for explicit native deactivation"),
		BlockedRemove.Status
			== Edemo_mapShanmenFormationInfluenceLifecycleCommandStatus::
				LifecycleRejected
			&& BlockedRemove.Lifecycle.Status
				== Edemo_mapShanmenFormationInfluenceLifecycleStatus::
					ConsumerDeactivateRequired
			&& BlockedRemove.Lifecycle.bConsumerLeaseOrderChecked
			&& BlockedRemove.Lifecycle.ConsumerLeaseId
				== ConsumerCommands.Remove.GetProjection().GetLease().LeaseId
			&& BlockedRemove.Lifecycle.ActiveConsumerApplicationCount == 1
			&& !BlockedRemove.Lifecycle.bCoordinatorStateCommitted
			&& !BlockedRemove.bRouterStateCommitted
			&& !bBlockedRemoveStored
			&& Fixture.Host.TryPeekNextInfluenceIntent(RemoveIntent));

	const auto Deactivated = CommandHost.TryDeactivateConsumerDelivery(
		Fixture.Host, ConsumerCommands.Delivery.Delivery);
	const auto Remove = CommandHost.TrySubmit(
		nullptr, Fixture.Host, RemoveCommand);
	const auto ExpiredDelivery = CommandHost.TryPrepareConsumerCommands(
		Apply.CommandId, ConsumerCommands.Delivery.Delivery.Definition);
	const auto DeactivationReplay =
		CommandHost.TryDeactivateConsumerDelivery(
			Fixture.Host, ConsumerCommands.Delivery.Delivery);
	const auto EndCommand =
		MakeLifecycleEndCommand(Fixture.Correlation, 283);
	const auto BlockedEnd = CommandHost.TrySubmit(
		nullptr, Fixture.Host, EndCommand);
	Fdemo_mapShanmenFormationInfluenceLifecycleCommandRecord Receipt;
	const bool bBlockedEndStored = CommandHost.TryGetReceipt(
		EndCommand.GetCommandId(), Receipt);

	TestTrue(TEXT("Consumer deactivation precedes authoritative Remove"),
		Deactivated.IsSuccess() && Deactivated.bSourceReceiptChecked
			&& !Deactivated.bSubjectResolutionChecked
			&& Deactivated.Runtime.bLeaseAuthorityChecked
			&& Deactivated.Runtime.AuthoritativeLease.IsValid()
			&& Remove.IsSuccess()
			&& DeactivationReplay.IsSuccess()
			&& DeactivationReplay.Status
				== Edemo_mapShanmenFormationInfluenceConsumerDeliveryApplicationStatus::
					DeactivationReplayed
			&& DeactivationReplay.Runtime.bLeaseAuthorityChecked
			&& Attributes->GetActiveModifierCount() == 0
			&& CommandHost.GetConsumerRuntime().IsDrained());
	TestTrue(TEXT("Removed authoritative lease cannot mint new deliveries"),
		ExpiredDelivery.Status
			== Edemo_mapShanmenFormationInfluenceConsumerDeliveryStatus::
				LeaseNotActive);
	TestTrue(TEXT("Drained consumer permits seal and exposes World failure"),
		BlockedEnd.Status
			== Edemo_mapShanmenFormationInfluenceLifecycleCommandStatus::
				LifecycleRejected
			&& BlockedEnd.Lifecycle.Status
				== Edemo_mapShanmenFormationInfluenceLifecycleStatus::
					EndRejected
			&& BlockedEnd.Lifecycle.bConsumerTeardownChecked
			&& BlockedEnd.Lifecycle.ConsumerTeardown.Status
				== Edemo_mapShanmenFormationInfluenceConsumerProductRuntimeStatus::
					TeardownReady
			&& BlockedEnd.Lifecycle.bCoordinatorStateCommitted
			&& BlockedEnd.bRouterStateCommitted
			&& bBlockedEndStored
			&& CommandHost.GetReceiptCount() == 4
			&& Fixture.Host.GetInfluenceLedger().IsSealed()
			&& Fixture.Host.GetSession().IsTerminal());

	const auto FailedWorldEnd = BlockedEnd;
	Fdemo_mapShanmenFormationInfluenceLifecycleCommandRecord FailedReceipt;
	const bool bFailedWorldEndStored = CommandHost.TryGetReceipt(
		EndCommand.GetCommandId(), FailedReceipt);

	TestTrue(TEXT("Drained evidence is durable across forward World failure"),
		FailedWorldEnd.Status
			== Edemo_mapShanmenFormationInfluenceLifecycleCommandStatus::
				LifecycleRejected
			&& FailedWorldEnd.Lifecycle.Status
				== Edemo_mapShanmenFormationInfluenceLifecycleStatus::EndRejected
			&& FailedWorldEnd.Lifecycle.bConsumerTeardownChecked
			&& FailedWorldEnd.Lifecycle.ConsumerTeardown.IsSuccess()
			&& FailedWorldEnd.Lifecycle.bCoordinatorStateCommitted
			&& FailedWorldEnd.bRouterStateCommitted
			&& bFailedWorldEndStored && FailedReceipt.IsValid()
			&& FailedReceipt.Result.Lifecycle.bConsumerTeardownChecked
			&& Fixture.Host.GetInfluenceLedger().IsSealed()
			&& Fixture.Host.GetSession().IsTerminal()
			&& CommandHost.GetReceiptCount() == 4);

	const auto Recovered = CommandHost.TrySubmit(
		Fixture.World, Fixture.Host, EndCommand);
	Fdemo_mapShanmenFormationInfluenceLifecycleCommandRecord RecoveredReceipt;
	const bool bRecoveredStored = CommandHost.TryGetReceipt(
		EndCommand.GetCommandId(), RecoveredReceipt);
	const auto Replay = CommandHost.TrySubmit(
		Fixture.World, Fixture.Host, EndCommand);

	TestTrue(TEXT("Terminal ProductHost can replay the exact drained check"),
		Recovered.IsSuccess() && Recovered.bRecoveryAttempted
			&& Recovered.Lifecycle.bConsumerTeardownChecked
			&& Recovered.Lifecycle.ConsumerTeardown.IsSuccess()
			&& bRecoveredStored && RecoveredReceipt.IsValid()
			&& RecoveredReceipt.Result.IsSuccess()
			&& RecoveredReceipt.Result.Lifecycle.bConsumerTeardownChecked
			&& RecoveredReceipt.Result.Lifecycle.ConsumerTeardown.Status
				== Edemo_mapShanmenFormationInfluenceConsumerProductRuntimeStatus::
					TeardownReady);
	TestTrue(TEXT("Completed guarded end is an immutable read-only replay"),
		Replay.IsSuccess() && Replay.IsReplay()
			&& !Replay.bRecoveryAttempted
			&& CommandHost.GetReceiptCount() == 4
			&& CommandHost.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationInfluenceConsumerRunCompositionTest,
	"Shanmen.0_0_10.Product.FormationInfluenceConsumerRunComposition.CommandHostOwnedLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationInfluenceConsumerRunCompositionTest::RunTest(
	const FString&)
{
	FFormationHostFixture Fixture;
	if (!Fixture.Start(*this, TEXT("ConsumerRunComposition"), true)
		|| !Fixture.CommitAndPlaceCoverageDiagram(*this))
	{
		return false;
	}

	FActorSpawnParameters Parameters;
	Parameters.ObjectFlags |= RF_Transient;
	Parameters.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	APawn* Player = Fixture.World->SpawnActor<APawn>(
		APawn::StaticClass(), FTransform::Identity, Parameters);
	UBoxComponent* PlayerRoot = Player
		? NewObject<UBoxComponent>(
			Player, TEXT("P839PlayerRoot"), RF_Transient)
		: nullptr;
	Udemo_mapPlayerHealthComponent* PlayerHealth = Player
		? NewObject<Udemo_mapPlayerHealthComponent>(
			Player, TEXT("P839PlayerHealth"), RF_Transient)
		: nullptr;
	if (!Player || !PlayerRoot || !PlayerHealth)
	{
		return false;
	}
	Player->SetRootComponent(PlayerRoot);
	Player->AddInstanceComponent(PlayerRoot);
	Player->AddInstanceComponent(PlayerHealth);
	PlayerRoot->SetWorldLocation(FVector(25.0, 25.0, 900.0));

	Fdemo_mapCombatRunCoordinator CombatRun;
	FString CombatDiagnostic;
	if (!CombatRun.TryBeginRun(
			Fixture.Correlation.ActiveRunId,
			Player,
			PlayerHealth,
			CombatDiagnostic))
	{
		AddError(FString::Printf(
			TEXT("P8.41 CombatRun setup failed: %s"),
			*CombatDiagnostic));
		return false;
	}

	TArray<AActor*> Subjects{Player};
	const auto Prime = Fixture.Host.TryCoordinateInfluence(
		Fixture.World,
		CombatRun.GetEntityRegistry(),
		Subjects,
		Fixture.Correlation,
		Fdemo_mapShanmenFormationCoverageCommand::MakePrime(),
		MakeHostInfluencePolicy(Fixture.Host));
	if (!Prime.IsSuccess()
		|| Prime.ReconciliationPlan.Batch.Intents.Num() != 1
		|| Prime.ReconciliationPlan.Batch.Intents[0].SubjectEntityId
			!= CombatRun.GetPlayerEntityId())
	{
		return false;
	}

	Fdemo_mapShanmenFormationInfluenceLifecycleCommandHost CommandHost;
	Fdemo_mapShanmenFormationInfluenceLifecycleCommandHost ForeignCommandHost;
	if (!Fdemo_mapShanmenFormationInfluenceLifecycleCommandHost::TryOpen(
			Fixture.Host, CommandHost)
		|| !Fdemo_mapShanmenFormationInfluenceLifecycleCommandHost::TryOpen(
			Fixture.Host, ForeignCommandHost))
	{
		return false;
	}
	const auto& ApplyIntent = Prime.ReconciliationPlan.Batch.Intents[0];
	const auto Apply = CommandHost.TrySubmit(
		nullptr,
		Fixture.Host,
		MakeLifecycleStepCommand(
			Fixture.Correlation,
			MakeHostExecutionRequest(ApplyIntent, 390)));
	FHostConsumerCommands ConsumerCommands;
	if (!Apply.IsSuccess()
		|| !BuildHostConsumerCommands(
			CommandHost,
			Apply.CommandId,
			ApplyIntent,
			ConsumerCommands))
	{
		return false;
	}

	Udemo_mapAttributeComponent* Attributes =
		NewObject<Udemo_mapAttributeComponent>(
			Player, TEXT("P839Attributes"), RF_Transient);
	Udemo_mapAttributeComponent* ForeignAttributes =
		NewObject<Udemo_mapAttributeComponent>(
			Player, TEXT("P839ForeignAttributes"), RF_Transient);
	USceneComponent* UnregisteredSubject =
		NewObject<USceneComponent>(GetTransientPackage());
	if (!Attributes || !ForeignAttributes || !UnregisteredSubject)
	{
		return false;
	}
	Player->AddInstanceComponent(Attributes);
	Player->AddInstanceComponent(ForeignAttributes);

	Fdemo_mapCombatRunCoordinator InactiveCombatRun;
	const auto Inactive =
		CommandHost.TryActivateConsumerForRun(
			InactiveCombatRun,
			Player,
			Fixture.Host,
			ConsumerCommands.Delivery.Delivery,
			Attributes);
	const int32 BindingCountBeforeRejections =
		CombatRun.GetEntityRegistry().NumObjectBindings();
	const auto MissingSource =
		CommandHost.TryActivateConsumerForRun(
			CombatRun,
			UnregisteredSubject,
			Fixture.Host,
			ConsumerCommands.Delivery.Delivery,
			Attributes);
	TestTrue(TEXT("CommandHost owner rejects invalid alias sources before delivery"),
		Inactive.Status
			== Edemo_mapShanmenFormationInfluenceConsumerRunCompositionStatus::
				AliasRejected
			&& Inactive.Alias.Status
				== Edemo_mapCombatRunEntityAliasStatus::CoordinatorNotReady
			&& !Inactive.bWorldResolutionChecked
			&& !Inactive.bDeliveryAttempted
			&& MissingSource.Status
				== Edemo_mapShanmenFormationInfluenceConsumerRunCompositionStatus::
					AliasRejected
			&& MissingSource.Alias.Status
				== Edemo_mapCombatRunEntityAliasStatus::
					RegisteredObjectNotFound
			&& CombatRun.GetEntityRegistry().NumObjectBindings()
				== BindingCountBeforeRejections
			&& CommandHost.GetConsumerRuntime().IsDrained());

	const auto ForeignHostRejected =
		ForeignCommandHost.TryActivateConsumerForRun(
			CombatRun,
			Player,
			Fixture.Host,
			ConsumerCommands.Delivery.Delivery,
			ForeignAttributes);
	FGuid PersistedForeignAlias;
	const bool bForeignAliasPersisted =
		CombatRun.GetEntityRegistry().TryResolveObject(
			CombatRun.GetRunId(),
			ForeignAttributes,
			INDEX_NONE,
			PersistedForeignAlias);
	TestTrue(TEXT("Valid identity alias survives a downstream Host rejection"),
		ForeignHostRejected.Status
			== Edemo_mapShanmenFormationInfluenceConsumerRunCompositionStatus::
				DeliveryApplicationRejected
			&& ForeignHostRejected.Alias.IsSuccess()
			&& ForeignHostRejected.WorldResolution.IsSuccess()
			&& ForeignHostRejected.bDeliveryAttempted
			&& ForeignHostRejected.Application.Status
				== Edemo_mapShanmenFormationInfluenceConsumerDeliveryApplicationStatus::
					SourceReceiptNotFound
			&& bForeignAliasPersisted
			&& PersistedForeignAlias == CombatRun.GetPlayerEntityId()
			&& ForeignCommandHost.GetConsumerRuntime().IsDrained()
			&& ForeignAttributes->GetActiveModifierCount() == 0);

	const auto Activated =
		CommandHost.TryActivateConsumerForRun(
			CombatRun,
			Player,
			Fixture.Host,
			ConsumerCommands.Delivery.Delivery,
			Attributes);
	const int32 BindingCountBeforeReplay =
		CombatRun.GetEntityRegistry().NumObjectBindings();
	const auto Replay =
		CommandHost.TryActivateConsumerForRun(
			CombatRun,
			Player,
			Fixture.Host,
			ConsumerCommands.Delivery.Delivery,
			Attributes);
	TestTrue(TEXT("CommandHost owner closes the alias-resolution-delivery chain"),
		Activated.IsSuccess()
			&& Activated.Status
				== Edemo_mapShanmenFormationInfluenceConsumerRunCompositionStatus::
					Activated
			&& Activated.Alias.EntityId
				== ConsumerCommands.SubjectEntityId
			&& Activated.WorldResolution.ResolvedEntityId
				== ConsumerCommands.SubjectEntityId
			&& Activated.Application.SubjectResolution.ResolutionId
				== Activated.WorldResolution.Resolution.ResolutionId
			&& Attributes->GetActiveModifierCount() == 1
			&& CommandHost.GetConsumerRuntime().GetBindingCount() == 1
			&& CommandHost.GetConsumerRuntime().
				GetActiveApplicationCount() == 1);
	TestTrue(TEXT("Exact composition replay performs no duplicate mutation"),
		Replay.IsSuccess()
			&& Replay.Status
				== Edemo_mapShanmenFormationInfluenceConsumerRunCompositionStatus::
					ActivationReplayed
			&& Replay.Alias.Status
				== Edemo_mapCombatRunEntityAliasStatus::AlreadyBound
			&& Replay.WorldResolution.Resolution.ResolutionId
				== Activated.WorldResolution.Resolution.ResolutionId
			&& CombatRun.GetEntityRegistry().NumObjectBindings()
				== BindingCountBeforeReplay
			&& Attributes->GetActiveModifierCount() == 1
			&& CommandHost.GetConsumerRuntime().GetBindingCount() == 1);

	auto InvalidActivation = Activated;
	InvalidActivation.RunId.Invalidate();
	const auto InvalidDeactivation =
		CommandHost.TryDeactivateConsumerForRun(
			Fixture.Host,
			InvalidActivation);
	const auto ForeignDeactivation =
		ForeignCommandHost.TryDeactivateConsumerForRun(
			Fixture.Host,
			Activated);
	TestTrue(TEXT("Deactivation rejects invalid evidence and a foreign Host"),
		InvalidDeactivation.Status
			== Edemo_mapShanmenFormationInfluenceConsumerRunDeactivationStatus::
				ActivationEvidenceRejected
			&& InvalidDeactivation.bActivationEvidenceChecked
			&& !InvalidDeactivation.bDeliveryAttempted
			&& ForeignDeactivation.Status
				== Edemo_mapShanmenFormationInfluenceConsumerRunDeactivationStatus::
					DeliveryApplicationRejected
			&& ForeignDeactivation.Application.Status
				== Edemo_mapShanmenFormationInfluenceConsumerDeliveryApplicationStatus::
					SourceReceiptNotFound
			&& Attributes->GetActiveModifierCount() == 1
			&& CommandHost.GetConsumerRuntime().
				GetActiveApplicationCount() == 1
			&& ForeignCommandHost.GetConsumerRuntime().IsDrained());

	const auto Deactivated =
		CommandHost.TryDeactivateConsumerForRun(
			Fixture.Host,
			Activated);
	const int32 CompletedBeforeDeactivationReplay =
		CommandHost.GetConsumerRuntime().GetCompletedTransactionCount();
	const auto DeactivationReplay =
		CommandHost.TryDeactivateConsumerForRun(
			Fixture.Host,
			Activated);
	const FGuid RunId = CombatRun.GetRunId();
	TestTrue(TEXT("Activation receipt removes its exact delivery before Run end"),
		Deactivated.IsSuccess()
			&& Deactivated.Status
				== Edemo_mapShanmenFormationInfluenceConsumerRunDeactivationStatus::
					Deactivated
			&& Deactivated.RunId == Activated.RunId
			&& Deactivated.Activation.Application.Delivery.Matches(
				Deactivated.Application.Delivery)
			&& Attributes->GetActiveModifierCount() == 0
			&& CommandHost.GetConsumerRuntime().IsDrained());
	TestTrue(TEXT("Exact deactivation replay performs no duplicate mutation"),
		DeactivationReplay.IsSuccess()
			&& DeactivationReplay.Status
				== Edemo_mapShanmenFormationInfluenceConsumerRunDeactivationStatus::
					DeactivationReplayed
			&& !DeactivationReplay.Application.Runtime.bRuntimeStateChanged
			&& CommandHost.GetConsumerRuntime().
				GetCompletedTransactionCount()
				== CompletedBeforeDeactivationReplay
			&& Attributes->GetActiveModifierCount() == 0
			&& CommandHost.GetConsumerRuntime().IsDrained());
	TestTrue(TEXT("Combat Run ends only after consumer teardown"),
		CombatRun.TryEndRun(RunId, CombatDiagnostic)
			&& CombatRun.GetEntityRegistry().NumObjectBindings() == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationInfluenceEvaluationBindingLeaseTest,
	"Shanmen.0_0_10.Product.FormationInfluenceEvaluationBinding.ApplyRemoveFreeze",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationInfluenceEvaluationBindingLeaseTest::RunTest(
	const FString&)
{
	FFormationHostFixture Fixture;
	FShanmenWorldEntityRegistry Registry;
	Fdemo_mapShanmenFormationHostInfluenceResult Prime;
	TArray<AActor*> Subjects;
	if (!PrimeHostExecutorInfluence(
			*this, Fixture, Registry, TEXT("EvaluationBindingLease"),
			1, Prime, Subjects))
	{
		return false;
	}

	const auto& Scope = Prime.ReconciliationPlan.Batch.Current.GetValue();
	const auto& ApplyIntent = Prime.ReconciliationPlan.Batch.Intents[0];
	const auto ApplyInvocation = MakeLeaseInvocation(
		Fixture.Host, ApplyIntent, 300);
	const auto FrozenReceipt = ApplyInvocation.Evaluation.GetReceipt();
	Fdemo_mapShanmenFormationInfluenceLeaseExecutor Executor;
	const auto Applied = Executor.Execute(ApplyInvocation);
	Fdemo_mapShanmenFormationInfluenceLeaseKey Key;
	Fdemo_mapShanmenFormationInfluenceLeaseSnapshot Active;
	const bool bReadActive =
		Fdemo_mapShanmenFormationInfluenceLeaseKey::TryFromIntent(
			ApplyIntent, Key)
		&& Executor.TryGetActiveLease(Key, Active);

	const auto RemoveIntent = MakeLeaseIntent(
		Scope, ApplyIntent.SourceEntityId, ApplyIntent.SubjectEntityId,
		Edemo_mapShanmenFormationInfluenceOperation::Remove, 300);
	const auto RemoveInvocation = MakeLeaseInvocation(
		Fixture.Host, RemoveIntent, 301);
	const auto Removed = Executor.Execute(RemoveInvocation);
	Fdemo_mapShanmenFormationInfluenceEvaluationReceipt ApplyHistory;
	Fdemo_mapShanmenFormationInfluenceEvaluationReceipt RemoveHistory;
	const bool bReadApplyHistory =
		Executor.TryGetCompletedEvaluationReceipt(
			ApplyIntent.IntentId, ApplyHistory);
	const bool bReadRemoveHistory =
		Executor.TryGetCompletedEvaluationReceipt(
			RemoveIntent.IntentId, RemoveHistory);

	TestTrue(TEXT("Apply freezes the exact evaluated receipt in its lease"),
		Applied.IsSuccess() && bReadActive && Active.IsValid()
			&& ApplyInvocation.Evaluation.HasReceipt()
			&& Active.EvaluationReceipt.Matches(FrozenReceipt)
			&& Active.LeaseId.IsValid());
	TestTrue(TEXT("Remove carries no resampled receipt and consumes the lease"),
		Removed.IsSuccess() && !RemoveInvocation.Evaluation.HasReceipt()
			&& Executor.GetActiveLeaseCount() == 0
			&& Applied.Receipt.ExecutorReceiptId
				!= Removed.Receipt.ExecutorReceiptId);
	TestTrue(TEXT("Both completion records retain the original Apply evidence"),
		bReadApplyHistory && bReadRemoveHistory
			&& ApplyHistory.Matches(FrozenReceipt)
			&& RemoveHistory.Matches(FrozenReceipt)
			&& Executor.GetCompletedIntentCount() == 2
			&& Executor.GetAttemptCount() == 2
			&& Executor.IsConsistent());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationInfluenceEvaluationBindingRouterTest,
	"Shanmen.0_0_10.Product.FormationInfluenceEvaluationBinding.RequestIdentityFence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationInfluenceEvaluationBindingRouterTest::RunTest(
	const FString&)
{
	FFormationHostFixture Fixture;
	FShanmenWorldEntityRegistry Registry;
	Fdemo_mapShanmenFormationHostInfluenceResult Prime;
	TArray<AActor*> Subjects;
	if (!PrimeHostExecutorInfluence(
			*this, Fixture, Registry, TEXT("EvaluationBindingRouter"),
			2, Prime, Subjects))
	{
		return false;
	}
	const auto& FirstIntent = Prime.ReconciliationPlan.Batch.Intents[0];
	const auto& SecondIntent = Prime.ReconciliationPlan.Batch.Intents[1];
	Fdemo_mapShanmenFormationInfluenceExecutionRouter Router;

	const auto MissingReceipt = Router.TryRoute(
		Fixture.Host, Fixture.Correlation,
		MakeHostExecutionRequest(FirstIntent.IntentId, 310));
	auto WrongSubjectRequest = MakeHostExecutionRequest(FirstIntent, 311);
	WrongSubjectRequest.Evaluation = MakeHostEvaluationBinding(SecondIntent);
	const auto WrongSubject = Router.TryRoute(
		Fixture.Host, Fixture.Correlation, WrongSubjectRequest);
	const bool bUnboundAfterRejected =
		!Router.IsBound() && Router.GetRecordCount() == 0;
	const auto Request = MakeHostExecutionRequest(FirstIntent, 312);
	const auto Routed = Router.TryRoute(
		Fixture.Host, Fixture.Correlation, Request);
	auto ReceiptConflict = Request;
	Fdemo_mapShanmenFormationInfluenceEvaluationBinding OtherBinding;
	check(Fdemo_mapShanmenFormationInfluenceEvaluationBinding::
		TryCaptureApply(
			MakeHostEvaluationReceipt(FirstIntent, 250), OtherBinding));
	ReceiptConflict.Evaluation = OtherBinding;
	const auto Conflict = Router.TryRoute(
		Fixture.Host, Fixture.Correlation, ReceiptConflict);

	TestTrue(TEXT("Apply cannot route without intent-matched evaluation"),
		MissingReceipt.Status
				== Edemo_mapShanmenFormationInfluenceRouteStatus::
					EvaluationMismatch
			&& WrongSubject.Status
				== Edemo_mapShanmenFormationInfluenceRouteStatus::
					EvaluationMismatch
			&& bUnboundAfterRejected);
	TestTrue(TEXT("One valid request freezes receipt identity into its command"),
		Routed.IsSuccess() && Routed.Command.IsValid()
			&& Routed.Command.Evaluation.Matches(Request.Evaluation)
			&& Routed.Command.Evaluation.GetReceiptId()
				== Request.Evaluation.GetReceiptId());
	TestTrue(TEXT("RequestId reuse with another valid receipt is rejected"),
		Conflict.Status
				== Edemo_mapShanmenFormationInfluenceRouteStatus::RequestConflict
			&& !Conflict.IsSuccess()
			&& !ReceiptConflict.Evaluation.Matches(Request.Evaluation)
			&& Router.GetRecordCount() == 1 && Router.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationInfluenceEvaluationBindingTamperTest,
	"Shanmen.0_0_10.Product.FormationInfluenceEvaluationBinding.TamperAndOperationFence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationInfluenceEvaluationBindingTamperTest::RunTest(
	const FString&)
{
	FFormationHostFixture Fixture;
	FShanmenWorldEntityRegistry Registry;
	Fdemo_mapShanmenFormationHostInfluenceResult Prime;
	TArray<AActor*> Subjects;
	if (!PrimeHostExecutorInfluence(
			*this, Fixture, Registry, TEXT("EvaluationBindingTamper"),
			2, Prime, Subjects))
	{
		return false;
	}
	const auto& Scope = Prime.ReconciliationPlan.Batch.Current.GetValue();
	const auto& ApplyIntent = Prime.ReconciliationPlan.Batch.Intents[0];
	const auto& OtherIntent = Prime.ReconciliationPlan.Batch.Intents[1];
	const auto RemoveIntent = MakeLeaseIntent(
		Scope, ApplyIntent.SourceEntityId, ApplyIntent.SubjectEntityId,
		Edemo_mapShanmenFormationInfluenceOperation::Remove, 320);
	const auto Receipt = MakeHostEvaluationReceipt(ApplyIntent);
	const auto ApplyBinding = MakeHostEvaluationBinding(ApplyIntent);
	const auto RemoveBinding =
		Fdemo_mapShanmenFormationInfluenceEvaluationBinding::MakeRemove();

	auto TamperedReceipt = Receipt;
	++TamperedReceipt.FinalMagnitudeUnits;
	Fdemo_mapShanmenFormationInfluenceEvaluationBinding FailedOutput =
		ApplyBinding;
	const bool bCapturedTamper =
		Fdemo_mapShanmenFormationInfluenceEvaluationBinding::TryCaptureApply(
			TamperedReceipt, FailedOutput);
	Fdemo_mapShanmenFormationInfluenceEvaluationContext EmptyContext =
		Receipt.Context;
	const auto EmptyEvaluation =
		Fdemo_mapShanmenFormationInfluenceModifierEvaluator::Evaluate(
			EmptyContext, {});
	const bool bCapturedEmpty =
		Fdemo_mapShanmenFormationInfluenceEvaluationBinding::TryCaptureApply(
			EmptyEvaluation.Receipt, FailedOutput);

	TestTrue(TEXT("Operation shape enforces Apply receipt and empty Remove"),
		ApplyBinding.MatchesIntent(ApplyIntent)
			&& !ApplyBinding.MatchesIntent(RemoveIntent)
			&& RemoveBinding.MatchesIntent(RemoveIntent)
			&& !RemoveBinding.MatchesIntent(ApplyIntent));
	TestTrue(TEXT("Receipt identity cannot cross subject or policy intent"),
		Fdemo_mapShanmenFormationInfluenceEvaluationBinding::
			ReceiptMatchesInfluenceIdentity(Receipt, ApplyIntent)
			&& !Fdemo_mapShanmenFormationInfluenceEvaluationBinding::
				ReceiptMatchesInfluenceIdentity(Receipt, OtherIntent));
	TestTrue(TEXT("Tampered and empty receipts fail closed and clear output"),
		!bCapturedTamper && !bCapturedEmpty
			&& FailedOutput.IsStructurallyValid()
			&& !FailedOutput.HasReceipt()
			&& EmptyEvaluation.IsSuccess()
			&& EmptyEvaluation.Receipt.Decisions.IsEmpty());
	return true;
}

#endif
