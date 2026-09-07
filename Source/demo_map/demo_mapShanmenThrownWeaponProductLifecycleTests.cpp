#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenThrownWeaponProductLifecycle.h"

#include "ShanmenCombatTags.h"
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
#include "demo_mapShanmenThrownWeaponArcPreviewProductBridge.h"
#include "demo_mapShanmenThrownWeaponArcPreLaunchPreviewContext.h"
#include "demo_mapShanmenThrownWeaponArcPreviewMainHUDRendererAdapter.h"
#include "demo_mapShanmenThrownWeaponArcPreviewMainHUDRuntimeBinding.h"
#include "demo_mapShanmenThrownWeaponArcPreviewPresentation.h"
#include "demo_mapShanmenThrownWeaponArcPreviewPresentationCommand.h"
#include "demo_mapShanmenThrownWeaponArcPreviewPresentationCommandLedger.h"
#include "demo_mapShanmenThrownWeaponArcPreviewPresentationCompositionOwner.h"
#include "demo_mapShanmenThrownWeaponArcPreviewPresentationConsumerAdapter.h"
#include "demo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryCoordinator.h"
#include "demo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryHost.h"
#include "demo_mapShanmenThrownWeaponArcPreviewPresentationDeliverySession.h"
#include "demo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoff.h"
#include "demo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecovery.h"
#include "demo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryAdmissionSession.h"
#include "demo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundle.h"
#include "demo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorage.h"
#include "demo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAuthority.h"
#include "demo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadEnvelope.h"
#include "demo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadStorage.h"
#include "demo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionSession.h"
#include "demo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournal.h"
#include "demo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryMultiGenerationCycleProof.h"
#include "demo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryNextGenerationRotationSession.h"
#include "demo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryTerminalAdoptionSession.h"
#include "demo_mapShanmenThrownWeaponArcPreviewPresentationSession.h"
#include "demo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceLifecycleExecutor.h"
#include "demo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceOwnershipTransition.h"
#include "demo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRecreationPolicy.h"
#include "demo_mapShanmenThrownWeaponArcPreviewUpdateCoordinator.h"
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
	FString NewThrownLifecycleRoot(const TCHAR* Label)
	{
		return FPaths::Combine(
			FPaths::ProjectSavedDir(),
			TEXT("Automation"),
			TEXT("Dev.D.UE.0.0.10.P7.8.r0"),
			Label,
			FGuid::NewGuid().ToString(EGuidFormats::Digits));
	}

	struct FThrownLifecycleFixture
	{
		FString Root;
		Fdemo_mapProfileStorageContext Storage;
		Fdemo_mapPersistentProfile SeedProfile;
		FGuid TrainingBladeId;
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

		bool Start(FAutomationTestBase& Test, const TCHAR* Label)
		{
			return Start(
				Test,
				Label,
				Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::Straight);
		}

		bool Start(
			FAutomationTestBase& Test,
			const TCHAR* Label,
			Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind TrajectoryKind)
		{
			Root = NewThrownLifecycleRoot(Label);
			Storage = Fdemo_mapProfileStorageContext::ForRoot(Root);
			Fdemo_mapProfileRepository Repository;
			SeedProfile = Repository.CreateFreshProfile();
			for (const Fdemo_mapPersistentItemRecord& Item :
				SeedProfile.PermanentStash)
			{
				if (Item.ItemDefinitionId
					== Fdemo_mapItemIds::TrainingBlade)
				{
					TrainingBladeId = Item.ItemInstanceId;
				}
			}
			Fdemo_mapPersistentItemRecord Knife;
			ThrowingKnifeId = Knife.ItemInstanceId = FGuid::NewGuid();
			Knife.ItemDefinitionId =
				Fdemo_mapItemIds::TrainingThrowingKnife;
			Knife.StackCount = 3;
			Knife.PersistentDomain =
				Edemo_mapPersistentDomain::PermanentStash;
			SeedProfile.PermanentStash.Add(Knife);
			SeedProfile.PreparationLayout.WeaponItemInstanceId =
				TrainingBladeId;
			const Fdemo_mapProfileSaveResult Saved =
				Repository.SaveProfile(SeedProfile, Storage);
			if (!TrainingBladeId.IsValid()
				|| !ThrowingKnifeId.IsValid()
				|| !Saved.IsSuccess()
				|| !GEngine)
			{
				Test.AddError(TEXT("P7.8 could not seed isolated product content."));
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
					TEXT("P7.8 stable migration source failed: %s"),
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
					ThrowingKnifeId, true).IsAccepted()
				|| !ProfileSession->SetPreparationHotbarSlot(
					2, ThrowingKnifeId).IsAccepted())
			{
				Test.AddError(FString::Printf(
					TEXT("P7.8 authority preparation failed: %s"),
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
				|| Correlation.HotbarItemInstanceIds[1]
					!= ThrowingKnifeId)
			{
				Test.AddError(FString::Printf(
					TEXT("P7.8 durable active Run failed: %s"),
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
					Source, TEXT("P78PlayerHealth"))
				: nullptr;
			Attributes = Source
				? NewObject<Udemo_mapAttributeComponent>(
					Source, TEXT("P78PlayerAttributes"))
				: nullptr;
			if (Source && Attributes)
			{
				Source->AddInstanceComponent(Attributes);
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
					TrajectoryKind,
					Diagnostic))
			{
				Test.AddError(FString::Printf(
					TEXT("P7.8 product lifecycle failed: %s"),
					*Diagnostic));
				return false;
			}
			return true;
		}

		Fdemo_mapShanmenThrownWeaponHotbarIntent MakeIntent(
			const FGuid& SelectionId,
			const FVector& Aim = FVector::ForwardVector) const
		{
			Fdemo_mapShanmenThrownWeaponHotbarIntent Intent;
			check(Fdemo_mapShanmenThrownWeaponHotbarIntent::TryCapture(
				SelectionId,
				2,
				FVector(40.0, 10.0, 75.0),
				Aim,
				1400.0f,
				Intent));
			return Intent;
		}

		Fdemo_mapShanmenThrownWeaponHotbarIntent MakeArcIntent(
			const FGuid& SelectionId,
			const FVector& Target,
			double ApexClearance = 160.0) const
		{
			Fdemo_mapShanmenThrownWeaponHotbarIntent Intent;
			check(Fdemo_mapShanmenThrownWeaponHotbarIntent::TryCaptureArc(
				SelectionId,
				2,
				FVector(40.0, 10.0, 75.0),
				Target,
				ApexClearance,
				Intent));
			return Intent;
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

		~FThrownLifecycleFixture()
		{
			Stop();
		}
	};

	Fdemo_mapShanmenThrownWeaponInputChoiceState MakeArcPreviewChoice(
		const bool bApplyApexAdjustment = true)
	{
		using ETrajectory =
			Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind;
		Fdemo_mapShanmenThrownWeaponInputChoiceState State =
			Fdemo_mapShanmenThrownWeaponInputChoiceState::CreateInitial();
		Fdemo_mapShanmenThrownWeaponInputChoiceCommand Command;
		check(Fdemo_mapShanmenThrownWeaponInputChoiceCommand::
			TryCaptureTrajectorySelection(
				State.GetRevision(), ETrajectory::BallisticArc, Command));
		auto Reduced =
			Fdemo_mapShanmenThrownWeaponInputChoiceReducer::Reduce(
				State, Command);
		check(Reduced.DidChange());
		State = Reduced.State;
		check(Fdemo_mapShanmenThrownWeaponInputChoiceCommand::
			TryCaptureArcTargetIntent(
				State.GetRevision(), FVector2D(0.0, -1.0), Command));
		Reduced = Fdemo_mapShanmenThrownWeaponInputChoiceReducer::Reduce(
			State, Command);
		check(Reduced.DidChange());
		State = Reduced.State;
		if (bApplyApexAdjustment)
		{
			check(Fdemo_mapShanmenThrownWeaponInputChoiceCommand::
				TryCaptureArcApexAdjustment(
					State.GetRevision(), -1.0, Command));
			Reduced = Fdemo_mapShanmenThrownWeaponInputChoiceReducer::Reduce(
				State, Command);
			check(Reduced.DidChange());
			State = Reduced.State;
		}
		return State;
	}

	Fdemo_mapShanmenThrownWeaponArcChoicePolicy MakeArcPreviewChoicePolicy()
	{
		Fdemo_mapShanmenThrownWeaponArcChoicePolicy Policy;
		check(Fdemo_mapShanmenThrownWeaponArcChoicePolicy::TryCapture(
			400.0, 1200.0, 300.0, 100.0, 500.0, Policy));
		return Policy;
	}

	Fdemo_mapShanmenThrownWeaponArcChoiceBasis MakeArcPreviewBasis()
	{
		Fdemo_mapShanmenThrownWeaponArcChoiceBasis Basis;
		check(Fdemo_mapShanmenThrownWeaponArcChoiceBasis::TryCapture(
			FVector(100.0, 200.0, 50.0),
			FVector::ForwardVector,
			FVector::RightVector,
			Basis));
		return Basis;
	}

	Fdemo_mapShanmenThrownWeaponInputChoiceState ClearArcPreviewChoice(
		const Fdemo_mapShanmenThrownWeaponInputChoiceState& Previous)
	{
		Fdemo_mapShanmenThrownWeaponInputChoiceCommand Command;
		check(Fdemo_mapShanmenThrownWeaponInputChoiceCommand::
			TryCaptureArcTargetClear(Previous.GetRevision(), Command));
		const auto Reduced =
			Fdemo_mapShanmenThrownWeaponInputChoiceReducer::Reduce(
				Previous, Command);
		check(Reduced.DidChange());
		return Reduced.State;
	}

	Fdemo_mapShanmenThrownWeaponInputChoiceState RetargetArcPreviewChoice(
		const Fdemo_mapShanmenThrownWeaponInputChoiceState& Previous)
	{
		Fdemo_mapShanmenThrownWeaponInputChoiceCommand Command;
		check(Fdemo_mapShanmenThrownWeaponInputChoiceCommand::
			TryCaptureArcTargetIntent(
				Previous.GetRevision(), FVector2D(0.0, -1.0), Command));
		const auto Reduced =
			Fdemo_mapShanmenThrownWeaponInputChoiceReducer::Reduce(
				Previous, Command);
		check(Reduced.DidChange());
		return Reduced.State;
	}

	Fdemo_mapShanmenThrownWeaponInputChoiceState
	MakeUnreachableArcPreviewChoice()
	{
		using ETrajectory =
			Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind;
		Fdemo_mapShanmenThrownWeaponInputChoiceState State =
			Fdemo_mapShanmenThrownWeaponInputChoiceState::CreateInitial();
		Fdemo_mapShanmenThrownWeaponInputChoiceCommand Command;
		check(Fdemo_mapShanmenThrownWeaponInputChoiceCommand::
			TryCaptureTrajectorySelection(
				State.GetRevision(), ETrajectory::BallisticArc, Command));
		auto Reduced =
			Fdemo_mapShanmenThrownWeaponInputChoiceReducer::Reduce(
				State, Command);
		check(Reduced.DidChange());
		State = Reduced.State;
		check(Fdemo_mapShanmenThrownWeaponInputChoiceCommand::
			TryCaptureArcTargetIntent(
				State.GetRevision(), FVector2D(0.5, 0.5), Command));
		Reduced = Fdemo_mapShanmenThrownWeaponInputChoiceReducer::Reduce(
			State, Command);
		check(Reduced.DidChange());
		State = Reduced.State;
		check(Fdemo_mapShanmenThrownWeaponInputChoiceCommand::
			TryCaptureArcApexAdjustment(
				State.GetRevision(), 0.25, Command));
		Reduced = Fdemo_mapShanmenThrownWeaponInputChoiceReducer::Reduce(
			State, Command);
		check(Reduced.DidChange());
		return Reduced.State;
	}

	bool StartArcPreviewPresentationSession(
		FAutomationTestBase& Test,
		const TCHAR* Label,
		FThrownLifecycleFixture& Fixture,
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSession& Session)
	{
		if (!Fixture.Start(
				Test,
				Label,
				Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::
					BallisticArc))
		{
			return false;
		}
		FString Diagnostic;
		if (!Session.TryBegin(Fixture.Correlation.ActiveRunId, Diagnostic))
		{
			Test.AddError(Diagnostic);
			return false;
		}
		return true;
	}

	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSessionResult
	UpdateArcPreviewPresentationSession(
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSession& Session,
		const Fdemo_mapShanmenThrownWeaponInputChoiceState& Choice,
		const FThrownLifecycleFixture& Fixture)
	{
		return Session.TryUpdate(
			2,
			Choice,
			MakeArcPreviewChoicePolicy(),
			8,
			MakeArcPreviewBasis(),
			Fixture.Lifecycle,
			Fixture.Coordinator);
	}

	Fdemo_mapShanmenThrownWeaponInputChoiceState MakeLaterNonPreviewChoice(
		const Fdemo_mapShanmenThrownWeaponInputChoiceState& Previous)
	{
		Fdemo_mapShanmenThrownWeaponInputChoiceCommand Command;
		check(Fdemo_mapShanmenThrownWeaponInputChoiceCommand::
			TryCaptureTrajectorySelection(
				Previous.GetRevision(),
				Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::Straight,
				Command));
		const auto Reduced =
			Fdemo_mapShanmenThrownWeaponInputChoiceReducer::Reduce(
				Previous, Command);
		check(Reduced.DidChange());
		return Reduced.State;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponProductLifecycleBindingTest,
	"Shanmen.0_0_10.Product.ThrownWeaponProductLifecycle.ContentAndBinding",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponProductLifecycleBindingTest::RunTest(
	const FString&)
{
	Fdemo_mapShanmenThrownWeaponSessionConfig Config;
	FString Diagnostic;
	const bool bCaptured =
		Fdemo_mapShanmenThrownWeaponProductLifecycle::
			TryCaptureTrainingThrowingKnifeConfig(Config, Diagnostic);
	const FShanmenThrownWeaponDefinitionCapture& Definition =
		Config.GetDefinition();
	TestTrue(TEXT("Real product content freezes one valid straight-throw policy"),
		bCaptured && Config.IsValid()
		&& Config.GetTrajectoryKind()
			== Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::Straight
		&& Definition.ActionDefinitionId
			== FShanmenThrownWeaponDefinition::CanonicalActionDefinitionId()
		&& Definition.DetectorId
			== FName(TEXT("Detector.ThrownWeapon.TrainingThrowingKnife.Straight"))
		&& Definition.FormulaId
			== FName(TEXT("Combat.Formula.ThrownWeapon.TrainingThrowingKnife.r1"))
		&& Definition.BaseDamage == 12.0f
		&& Definition.TechniquePowerCoefficient == 0.3f
		&& Definition.LaunchSpeed == 900.0f
		&& Definition.DamageTags.HasTagExact(
			FShanmenCombatNativeTags::DamagePhysicalSlash())
		&& Definition.RequiredTargetTags.HasTagExact(
			FShanmenCombatNativeTags::TargetLiving())
		&& Definition.bRejectSelf);

	Fdemo_mapShanmenThrownWeaponSessionConfig ArcConfig;
	const bool bArcCaptured =
		Fdemo_mapShanmenThrownWeaponProductLifecycle::
			TryCaptureTrainingThrowingKnifeConfig(
				Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::BallisticArc,
				ArcConfig,
				Diagnostic);
	const FShanmenThrownWeaponDefinitionCapture& ArcDefinition =
		ArcConfig.GetDefinition();
	TestTrue(TEXT("Lifecycle owns one canonical Arc policy for the same product"),
		bArcCaptured
		&& ArcConfig.IsValid()
		&& ArcConfig.GetTrajectoryKind()
			== Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::BallisticArc
		&& ArcDefinition.ActionDefinitionId
			== FShanmenThrownWeaponDefinition::ArcActionDefinitionId()
		&& ArcDefinition.DetectorId
			== FName(TEXT("Detector.ThrownWeapon.TrainingThrowingKnife.Arc"))
		&& ArcDefinition.FormulaId == Definition.FormulaId
		&& ArcDefinition.BaseDamage == Definition.BaseDamage
		&& ArcDefinition.TechniquePowerCoefficient
			== Definition.TechniquePowerCoefficient
		&& ArcDefinition.LaunchSpeed == Definition.LaunchSpeed
		&& ArcDefinition.DamageTags == Definition.DamageTags
		&& ArcDefinition.RequiredTargetTags == Definition.RequiredTargetTags
		&& ArcDefinition.bRejectSelf
		&& ArcConfig.GetArcPolicy().GetTechniqueTier()
			== EShanmenThrownWeaponTechniqueTier::Intermediate
		&& ArcConfig.GetArcPolicy().GetGravityMagnitude() == 980.0
		&& ArcConfig.GetArcPolicy().GetMaximumFlightTime() == 4.0
		&& !Config.Matches(ArcConfig));
	Fdemo_mapShanmenThrownWeaponSessionConfig InvalidConfig = ArcConfig;
	TestFalse(TEXT("Lifecycle rejects an unspecified trajectory and clears output"),
		Fdemo_mapShanmenThrownWeaponProductLifecycle::
			TryCaptureTrainingThrowingKnifeConfig(
				Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::Invalid,
				InvalidConfig,
				Diagnostic));
	TestFalse(TEXT("Rejected lifecycle content leaves no injectable config"),
		InvalidConfig.IsValid());

	FThrownLifecycleFixture Fixture;
	if (!Fixture.Start(*this, TEXT("Binding")))
	{
		return false;
	}
	TestTrue(TEXT("Lifecycle binds one durable Run without copying its identity"),
		Fixture.Lifecycle.IsActive()
		&& Fixture.Lifecycle.IsValid()
		&& Fixture.Lifecycle.GetRunId()
			== Fixture.Correlation.ActiveRunId);
	TestTrue(TEXT("Exact begin replay is idempotent"),
		Fixture.Lifecycle.TryBegin(
			*Fixture.Authority,
			*Fixture.Source,
			Fixture.Coordinator,
			Diagnostic));
	TestTrue(TEXT("Explicit Straight begin matches the compatibility entry"),
		Fixture.Lifecycle.TryBegin(
			*Fixture.Authority,
			*Fixture.Source,
			Fixture.Coordinator,
			Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::Straight,
			Diagnostic));
	TestFalse(TEXT("An active Straight lifecycle cannot switch to Arc policy"),
		Fixture.Lifecycle.TryBegin(
			*Fixture.Authority,
			*Fixture.Source,
			Fixture.Coordinator,
			Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::BallisticArc,
			Diagnostic));
	TestFalse(TEXT("Another source Actor cannot take over the live lifecycle"),
		Fixture.Lifecycle.TryBegin(
			*Fixture.Authority,
			*Fixture.OtherSource,
			Fixture.Coordinator,
			Diagnostic));
	TestTrue(TEXT("Rejected source switch preserves the original binding"),
		Fixture.Lifecycle.IsActive()
		&& Fixture.Lifecycle.IsValid()
		&& Fixture.Lifecycle.GetRunId()
			== Fixture.Coordinator.GetRunId());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponProductLifecycleRoutingTest,
	"Shanmen.0_0_10.Product.ThrownWeaponProductLifecycle.HotbarRouteReplayAndEnd",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponProductLifecycleRoutingTest::RunTest(
	const FString&)
{
	FThrownLifecycleFixture Fixture;
	if (!Fixture.Start(*this, TEXT("Routing")))
	{
		return false;
	}
	FShanmenItemAuthoritySnapshot Before;
	Fixture.Authority->TryCaptureSnapshot(Before);
	const FGuid SelectionId = FGuid::NewGuid();
	const Fdemo_mapShanmenThrownWeaponHotbarIntent Intent =
		Fixture.MakeIntent(SelectionId);
	const Fdemo_mapShanmenThrownWeaponSessionResult First =
		Fixture.Lifecycle.TrySubmitHotbar(
			Fixture.World,
			Ademo_mapShanmenThrownWeaponProjectile::StaticClass(),
			Fixture.Coordinator,
			Intent);
	FShanmenItemAuthoritySnapshot After;
	Fixture.Authority->TryCaptureSnapshot(After);
	const Fdemo_mapShanmenThrownWeaponSessionResult Replay =
		Fixture.Lifecycle.TrySubmitHotbar(
			Fixture.World,
			Ademo_mapShanmenThrownWeaponProjectile::StaticClass(),
			Fixture.Coordinator,
			Intent);
	FShanmenItemAuthoritySnapshot AfterReplay;
	Fixture.Authority->TryCaptureSnapshot(AfterReplay);
	TestTrue(TEXT("Device-independent hotbar intent launches the exact product item"),
		First.IsAccepted()
		&& First.ItemInstanceId == Fixture.ThrowingKnifeId
		&& First.RunId == Fixture.Correlation.ActiveRunId
		&& Fixture.Lifecycle.GetHostState()
			== Edemo_mapShanmenThrownWeaponHostState::InFlight
		&& After.AuthorityRevision == Before.AuthorityRevision + 2);
	TestTrue(TEXT("Exact SelectionId replay is idempotent and mutation-free"),
		Replay.IsAccepted()
		&& Replay.bReusedSelection
		&& Replay.Product.Command.IsReplay()
		&& AfterReplay == After
		&& Fixture.Lifecycle.NumCapturedSelections() == 1);

	const Fdemo_mapShanmenThrownWeaponHotbarIntent Conflict =
		Fixture.MakeIntent(SelectionId, FVector::RightVector);
	const Fdemo_mapShanmenThrownWeaponSessionResult ConflictResult =
		Fixture.Lifecycle.TrySubmitHotbar(
			Fixture.World,
			Ademo_mapShanmenThrownWeaponProjectile::StaticClass(),
			Fixture.Coordinator,
			Conflict);
	TestTrue(TEXT("SelectionId payload conflict fails closed"),
		ConflictResult.Status
			== Edemo_mapShanmenThrownWeaponSessionStatus::SelectionIdConflict
		&& Fixture.Lifecycle.NumCapturedSelections() == 1);

	FString Diagnostic;
	TestTrue(TEXT("Run owner interrupts flight before lifecycle teardown"),
		Fixture.Lifecycle.TryEnd(Diagnostic)
		&& Fixture.Lifecycle.IsEmpty()
		&& Fixture.Coordinator.IsReady());
	TestTrue(TEXT("Empty lifecycle end replay is harmless"),
		Fixture.Lifecycle.TryEnd(Diagnostic));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcProductLifecycleRoutingTest,
	"Shanmen.0_0_10.Product.ThrownWeaponProductLifecycle.ArcHotbarRouteReplayAndEnd",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcProductLifecycleRoutingTest::RunTest(
	const FString&)
{
	FThrownLifecycleFixture Fixture;
	if (!Fixture.Start(
			*this,
			TEXT("ArcRouting"),
			Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::BallisticArc))
	{
		return false;
	}
	FShanmenItemAuthoritySnapshot Before;
	Fixture.Authority->TryCaptureSnapshot(Before);
	const FGuid SelectionId = FGuid::NewGuid();
	const FVector Target(640.0, 110.0, 70.0);
	const Fdemo_mapShanmenThrownWeaponHotbarIntent Intent =
		Fixture.MakeArcIntent(SelectionId, Target);
	const Fdemo_mapShanmenThrownWeaponSessionResult First =
		Fixture.Lifecycle.TrySubmitHotbar(
			Fixture.World,
			Ademo_mapShanmenThrownWeaponProjectile::StaticClass(),
			Fixture.Coordinator,
			Intent);
	FShanmenItemAuthoritySnapshot After;
	Fixture.Authority->TryCaptureSnapshot(After);
	const Fdemo_mapShanmenThrownWeaponRunCommandIntent* Command =
		Fixture.Lifecycle.FindCapturedCommand(SelectionId);
	const FShanmenThrownWeaponArcRequest* ArcRequest = Command
		? &Command->GetArcPlan().GetRequest() : nullptr;
	Ademo_mapShanmenThrownWeaponProjectile* FirstProjectile =
		First.IsAccepted()
			? First.Product.Command.HostStart.Spawn.Projectile.Get()
			: nullptr;
	TestTrue(TEXT("Arc lifecycle binds canonical content to the exact Run item"),
		First.IsAccepted()
		&& First.ItemInstanceId == Fixture.ThrowingKnifeId
		&& First.RunId == Fixture.Correlation.ActiveRunId
		&& First.Product.ActivationSequence == 1
		&& Command
		&& Command->GetTrajectoryKind()
			== Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::BallisticArc
		&& Command->GetAction().GetActionDefinitionId()
			== FShanmenThrownWeaponDefinition::ArcActionDefinitionId()
		&& Command->GetAction().GetSourceItemInstanceId()
			== Fixture.ThrowingKnifeId
		&& ArcRequest
		&& ArcRequest->GetOrigin() == Intent.GetOrigin()
		&& ArcRequest->GetTarget() == Target
		&& ArcRequest->GetApexClearance() == Intent.GetApexClearance()
		&& ArcRequest->GetTechniqueTier()
			== EShanmenThrownWeaponTechniqueTier::Intermediate
		&& ArcRequest->GetGravityMagnitude() == 980.0
		&& ArcRequest->GetMaximumLaunchSpeed() == 900.0
		&& ArcRequest->GetMaximumFlightTime() == 4.0
		&& FirstProjectile
		&& Fixture.Lifecycle.GetHostState()
			== Edemo_mapShanmenThrownWeaponHostState::InFlight
		&& After.AuthorityRevision == Before.AuthorityRevision + 2
		&& Fixture.Coordinator
			.GetNextPlayerThrownWeaponActivationSequence() == 2
		&& Fixture.Lifecycle.IsValid());

	const Fdemo_mapShanmenThrownWeaponSessionResult Replay =
		Fixture.Lifecycle.TrySubmitHotbar(
			Fixture.World,
			Ademo_mapShanmenThrownWeaponProjectile::StaticClass(),
			Fixture.Coordinator,
			Intent);
	FShanmenItemAuthoritySnapshot AfterReplay;
	Fixture.Authority->TryCaptureSnapshot(AfterReplay);
	TestTrue(TEXT("Exact Arc lifecycle replay performs no new I/O or spawn"),
		Replay.IsAccepted()
		&& Replay.bReusedSelection
		&& Replay.Product.Command.IsReplay()
		&& Replay.Product.ActivationId == First.Product.ActivationId
		&& Replay.Product.ActivationSequence == 1
		&& Replay.Product.Command.HostStart.Spawn.Projectile.Get()
			== FirstProjectile
		&& AfterReplay == After
		&& Fixture.Lifecycle.NumCapturedSelections() == 1
		&& Fixture.Coordinator
			.GetNextPlayerThrownWeaponActivationSequence() == 2);

	const Fdemo_mapShanmenThrownWeaponSessionResult Conflict =
		Fixture.Lifecycle.TrySubmitHotbar(
			Fixture.World,
			Ademo_mapShanmenThrownWeaponProjectile::StaticClass(),
			Fixture.Coordinator,
			Fixture.MakeArcIntent(
				SelectionId,
				FVector(740.0, 110.0, 70.0)));
	TestTrue(TEXT("Lifecycle preserves SelectionId geometry identity"),
		Conflict.Status
			== Edemo_mapShanmenThrownWeaponSessionStatus::SelectionIdConflict
		&& Fixture.Lifecycle.NumCapturedSelections() == 1
		&& Fixture.Coordinator
			.GetNextPlayerThrownWeaponActivationSequence() == 2);

	FString Diagnostic;
	TestTrue(TEXT("Lifecycle owns Arc interruption before empty teardown"),
		Fixture.Lifecycle.TryEnd(Diagnostic)
		&& Fixture.Lifecycle.IsEmpty()
		&& Fixture.Coordinator.IsReady());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcProductLifecyclePlanRejectionTest,
	"Shanmen.0_0_10.Product.ThrownWeaponProductLifecycle.ArcPlanRejectionReplayAndEnd",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcProductLifecyclePlanRejectionTest::RunTest(
	const FString&)
{
	FThrownLifecycleFixture Fixture;
	if (!Fixture.Start(
			*this,
			TEXT("ArcPlanReject"),
			Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::BallisticArc))
	{
		return false;
	}
	const FGuid SelectionId = FGuid::NewGuid();
	const Fdemo_mapShanmenThrownWeaponHotbarIntent Intent =
		Fixture.MakeArcIntent(
			SelectionId,
			FVector(100040.0, 10.0, 75.0));
	FShanmenItemAuthoritySnapshot Before;
	Fixture.Authority->TryCaptureSnapshot(Before);
	const Fdemo_mapShanmenThrownWeaponSessionResult Rejected =
		Fixture.Lifecycle.TrySubmitHotbar(
			Fixture.World,
			Ademo_mapShanmenThrownWeaponProjectile::StaticClass(),
			Fixture.Coordinator,
			Intent);
	FShanmenItemAuthoritySnapshot After;
	Fixture.Authority->TryCaptureSnapshot(After);
	TestTrue(TEXT("Unreachable lifecycle Arc freezes identity without side effects"),
		Rejected.Status
			== Edemo_mapShanmenThrownWeaponSessionStatus::ProductRejected
		&& Rejected.Product.Status
			== Edemo_mapShanmenThrownWeaponProductStatus::ArcPlanRejected
		&& Rejected.Product.HasCapturedAction()
		&& !Rejected.bReusedSelection
		&& Rejected.Product.ActivationSequence == 1
		&& Before == After
		&& !Fixture.Lifecycle.FindCapturedCommand(SelectionId)
		&& Fixture.Lifecycle.GetHostState()
			== Edemo_mapShanmenThrownWeaponHostState::Empty
		&& Fixture.Lifecycle.NumCapturedSelections() == 1
		&& Fixture.Coordinator
			.GetNextPlayerThrownWeaponActivationSequence() == 2
		&& Fixture.Lifecycle.IsValid());

	const Fdemo_mapShanmenThrownWeaponSessionResult Replay =
		Fixture.Lifecycle.TrySubmitHotbar(
			Fixture.World,
			Ademo_mapShanmenThrownWeaponProjectile::StaticClass(),
			Fixture.Coordinator,
			Intent);
	FShanmenItemAuthoritySnapshot AfterReplay;
	Fixture.Authority->TryCaptureSnapshot(AfterReplay);
	TestTrue(TEXT("Unreachable lifecycle Arc replay reuses rejection identity"),
		Replay.Status
			== Edemo_mapShanmenThrownWeaponSessionStatus::ProductRejected
		&& Replay.Product.Status
			== Edemo_mapShanmenThrownWeaponProductStatus::ArcPlanRejected
		&& Replay.bReusedSelection
		&& Replay.Product.bReusedSelection
		&& Replay.Product.ActivationId == Rejected.Product.ActivationId
		&& Replay.Product.ActivationSequence == 1
		&& AfterReplay == After
		&& Fixture.Coordinator
			.GetNextPlayerThrownWeaponActivationSequence() == 2
		&& Fixture.Lifecycle.IsValid());

	const Fdemo_mapShanmenThrownWeaponSessionResult Conflict =
		Fixture.Lifecycle.TrySubmitHotbar(
			Fixture.World,
			Ademo_mapShanmenThrownWeaponProjectile::StaticClass(),
			Fixture.Coordinator,
			Fixture.MakeArcIntent(
				SelectionId,
				FVector(110040.0, 10.0, 75.0)));
	TestTrue(TEXT("Rejected Arc identity still rejects another geometry"),
		Conflict.Status
			== Edemo_mapShanmenThrownWeaponSessionStatus::SelectionIdConflict
		&& Fixture.Coordinator
			.GetNextPlayerThrownWeaponActivationSequence() == 2);

	FString Diagnostic;
	TestTrue(TEXT("Plan rejection leaves lifecycle free of hidden recovery"),
		Fixture.Lifecycle.TryEnd(Diagnostic)
		&& Fixture.Lifecycle.IsEmpty()
		&& Fixture.Coordinator.IsReady());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponProductLifecycleFailClosedTest,
	"Shanmen.0_0_10.Product.ThrownWeaponProductLifecycle.FailClosedBoundaries",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponProductLifecycleFailClosedTest::RunTest(
	const FString&)
{
	FThrownLifecycleFixture Fixture;
	if (!Fixture.Start(*this, TEXT("FailClosed")))
	{
		return false;
	}
	Fdemo_mapCombatRunCoordinator EmptyCoordinator;
	const Fdemo_mapShanmenThrownWeaponHotbarIntent Intent =
		Fixture.MakeIntent(FGuid::NewGuid());
	FShanmenItemAuthoritySnapshot Before;
	Fixture.Authority->TryCaptureSnapshot(Before);
	const Fdemo_mapShanmenThrownWeaponSessionResult Mismatch =
		Fixture.Lifecycle.TrySubmitHotbar(
			Fixture.World,
			Ademo_mapShanmenThrownWeaponProjectile::StaticClass(),
			EmptyCoordinator,
			Intent);
	FShanmenItemAuthoritySnapshot AfterMismatch;
	Fixture.Authority->TryCaptureSnapshot(AfterMismatch);
	TestTrue(TEXT("Routing never borrows an inactive or foreign coordinator"),
		Mismatch.Status
			== Edemo_mapShanmenThrownWeaponSessionStatus::RunMismatch
		&& AfterMismatch == Before
		&& Fixture.Lifecycle.NumCapturedSelections() == 0);
	const Fdemo_mapShanmenThrownWeaponSessionResult TrajectoryMismatch =
		Fixture.Lifecycle.TrySubmitHotbar(
			Fixture.World,
			Ademo_mapShanmenThrownWeaponProjectile::StaticClass(),
			Fixture.Coordinator,
			Fixture.MakeArcIntent(
				FGuid::NewGuid(),
				FVector(640.0, 110.0, 70.0)));
	FShanmenItemAuthoritySnapshot AfterTrajectoryMismatch;
	Fixture.Authority->TryCaptureSnapshot(AfterTrajectoryMismatch);
	TestTrue(TEXT("Straight lifecycle rejects Arc intent before product work"),
		TrajectoryMismatch.Status
			== Edemo_mapShanmenThrownWeaponSessionStatus::TrajectoryMismatch
		&& AfterTrajectoryMismatch == Before
		&& Fixture.Lifecycle.NumCapturedSelections() == 0
		&& Fixture.Coordinator
			.GetNextPlayerThrownWeaponActivationSequence() == 1);

	Fdemo_mapShanmenThrownWeaponProductLifecycle EmptyLifecycle;
	const Fdemo_mapShanmenThrownWeaponSessionResult Inactive =
		EmptyLifecycle.TrySubmitHotbar(
			Fixture.World,
			Ademo_mapShanmenThrownWeaponProjectile::StaticClass(),
			Fixture.Coordinator,
			Intent);
	FShanmenItemAuthoritySnapshot AfterInactive;
	Fixture.Authority->TryCaptureSnapshot(AfterInactive);
	TestTrue(TEXT("Unbound product lifecycle rejects without mutation"),
		Inactive.Status
			== Edemo_mapShanmenThrownWeaponSessionStatus::SessionInactive
		&& AfterInactive == Before
		&& EmptyLifecycle.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewProductBridgeCanonicalTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewProductBridge.CanonicalReadOnlyCapture",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewProductBridgeCanonicalTest::RunTest(
	const FString&)
{
	FThrownLifecycleFixture Fixture;
	if (!Fixture.Start(
			*this,
			TEXT("ArcPreviewProductBridgeCanonical"),
			Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::BallisticArc))
	{
		return false;
	}
	FShanmenItemAuthoritySnapshot Before;
	Fixture.Authority->TryCaptureSnapshot(Before);
	const uint64 SequenceBefore = Fixture.Coordinator
		.GetNextPlayerThrownWeaponActivationSequence();
	const auto Choice = MakeArcPreviewChoice();
	const auto Policy = MakeArcPreviewChoicePolicy();
	const auto Result =
		Fdemo_mapShanmenThrownWeaponArcPreviewProductBridge::Capture(
			2, Choice, Policy, 8, Fixture.Lifecycle, Fixture.Coordinator);
	FShanmenItemAuthoritySnapshot After;
	Fixture.Authority->TryCaptureSnapshot(After);
	TestTrue(TEXT("live product state enters one self-validating preview"),
		Result.IsCaptured()
			&& Result.GetLifecycleRunId() == Fixture.Correlation.ActiveRunId
			&& Result.GetCoordinatorRunId() == Fixture.Correlation.ActiveRunId
			&& Result.GetPlayerEntityId()
				== Fixture.Coordinator.GetPlayerEntityId()
			&& Result.GetSourceItemInstanceId() == Fixture.ThrowingKnifeId
			&& Result.GetSequenceReadCount() == 2
			&& Result.GetSequenceBefore() == SequenceBefore
			&& Result.GetSequenceAfter() == SequenceBefore);
	TestTrue(TEXT("preview bridge leaves real product and inventory untouched"),
		Before == After
			&& Fixture.Coordinator
				.GetNextPlayerThrownWeaponActivationSequence() == SequenceBefore
			&& Fixture.Lifecycle.GetHostState()
				== Edemo_mapShanmenThrownWeaponHostState::Empty
			&& Fixture.Lifecycle.NumCapturedSelections() == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewProductBridgeReplayTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewProductBridge.DeterministicReplay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewProductBridgeReplayTest::RunTest(
	const FString&)
{
	FThrownLifecycleFixture Fixture;
	if (!Fixture.Start(
			*this,
			TEXT("ArcPreviewProductBridgeReplay"),
			Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::BallisticArc))
	{
		return false;
	}
	FShanmenItemAuthoritySnapshot Before;
	Fixture.Authority->TryCaptureSnapshot(Before);
	const auto Choice = MakeArcPreviewChoice();
	const auto Policy = MakeArcPreviewChoicePolicy();
	const auto First =
		Fdemo_mapShanmenThrownWeaponArcPreviewProductBridge::Capture(
			2, Choice, Policy, 8, Fixture.Lifecycle, Fixture.Coordinator);
	const auto Replay =
		Fdemo_mapShanmenThrownWeaponArcPreviewProductBridge::Capture(
			2, Choice, Policy, 8, Fixture.Lifecycle, Fixture.Coordinator);
	FShanmenItemAuthoritySnapshot After;
	Fixture.Authority->TryCaptureSnapshot(After);
	TestTrue(TEXT("equal live reads replay exact preview evidence"),
		First.Matches(Replay)
			&& First.GetPreviewRequestId() == Replay.GetPreviewRequestId()
			&& First.GetCapture().GetPreviewActivationId()
				== Replay.GetCapture().GetPreviewActivationId());
	TestTrue(TEXT("replay still consumes no sequence or item state"),
		Before == After
			&& Fixture.Coordinator
				.GetNextPlayerThrownWeaponActivationSequence() == 1
			&& Fixture.Lifecycle.NumCapturedSelections() == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewProductBridgeRevisionTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewProductBridge.ChoiceRevisionIsolation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewProductBridgeRevisionTest::RunTest(
	const FString&)
{
	FThrownLifecycleFixture Fixture;
	if (!Fixture.Start(
			*this,
			TEXT("ArcPreviewProductBridgeRevision"),
			Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::BallisticArc))
	{
		return false;
	}
	const auto Policy = MakeArcPreviewChoicePolicy();
	const auto Initial =
		Fdemo_mapShanmenThrownWeaponArcPreviewProductBridge::Capture(
			2,
			MakeArcPreviewChoice(false),
			Policy,
			8,
			Fixture.Lifecycle,
			Fixture.Coordinator);
	const auto Revised =
		Fdemo_mapShanmenThrownWeaponArcPreviewProductBridge::Capture(
			2,
			MakeArcPreviewChoice(true),
			Policy,
			8,
			Fixture.Lifecycle,
			Fixture.Coordinator);
	TestTrue(TEXT("choice revision changes only preview-domain identity"),
		Initial.IsCaptured()
			&& Revised.IsCaptured()
			&& Initial.GetChoiceState().GetRevision() + 1
				== Revised.GetChoiceState().GetRevision()
			&& Initial.GetPreviewRequestId() != Revised.GetPreviewRequestId()
			&& Initial.GetCapture().GetPreviewActivationId()
				!= Revised.GetCapture().GetPreviewActivationId()
			&& Initial.GetCapture().GetProspectiveRealActivationId()
				== Revised.GetCapture().GetProspectiveRealActivationId()
			&& Fixture.Coordinator
				.GetNextPlayerThrownWeaponActivationSequence() == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewProductBridgeInputFenceTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewProductBridge.InputAndChoiceFences",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewProductBridgeInputFenceTest::RunTest(
	const FString&)
{
	FThrownLifecycleFixture Fixture;
	if (!Fixture.Start(
			*this,
			TEXT("ArcPreviewProductBridgeInputFence"),
			Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::BallisticArc))
	{
		return false;
	}
	using EBridgeStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewProductBridgeStatus;
	FShanmenItemAuthoritySnapshot Before;
	Fixture.Authority->TryCaptureSnapshot(Before);
	const auto InvalidSlot =
		Fdemo_mapShanmenThrownWeaponArcPreviewProductBridge::Capture(
			0,
			MakeArcPreviewChoice(),
			MakeArcPreviewChoicePolicy(),
			8,
			Fixture.Lifecycle,
			Fixture.Coordinator);
	const auto StraightChoice =
		Fdemo_mapShanmenThrownWeaponArcPreviewProductBridge::Capture(
			2,
			Fdemo_mapShanmenThrownWeaponInputChoiceState::CreateInitial(),
			MakeArcPreviewChoicePolicy(),
			8,
			Fixture.Lifecycle,
			Fixture.Coordinator);
	FShanmenItemAuthoritySnapshot After;
	Fixture.Authority->TryCaptureSnapshot(After);
	TestTrue(TEXT("invalid input and non-Arc choice reject before product work"),
		InvalidSlot.GetStatus() == EBridgeStatus::InputRejected
			&& StraightChoice.GetStatus() == EBridgeStatus::ChoiceRejected
			&& InvalidSlot.GetSequenceReadCount() == 0
			&& StraightChoice.GetSequenceReadCount() == 0
			&& Before == After
			&& Fixture.Coordinator
				.GetNextPlayerThrownWeaponActivationSequence() == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewProductBridgeProductFenceTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewProductBridge.ProductFences",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewProductBridgeProductFenceTest::RunTest(
	const FString&)
{
	using EBridgeStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewProductBridgeStatus;
	FThrownLifecycleFixture ArcFixture;
	if (!ArcFixture.Start(
			*this,
			TEXT("ArcPreviewProductBridgeProductFence"),
			Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::BallisticArc))
	{
		return false;
	}
	const auto Choice = MakeArcPreviewChoice();
	const auto Policy = MakeArcPreviewChoicePolicy();
	const auto EmptySlot =
		Fdemo_mapShanmenThrownWeaponArcPreviewProductBridge::Capture(
			1,
			Choice,
			Policy,
			8,
			ArcFixture.Lifecycle,
			ArcFixture.Coordinator);
	Fdemo_mapShanmenThrownWeaponProductLifecycle EmptyLifecycle;
	const auto Inactive =
		Fdemo_mapShanmenThrownWeaponArcPreviewProductBridge::Capture(
			2,
			Choice,
			Policy,
			8,
			EmptyLifecycle,
			ArcFixture.Coordinator);
	TestTrue(TEXT("empty hotbar and inactive lifecycle fail closed"),
		EmptySlot.GetStatus() == EBridgeStatus::ProductUnavailable
			&& Inactive.GetStatus() == EBridgeStatus::ProductUnavailable
			&& EmptySlot.GetSequenceReadCount() == 0
			&& Inactive.GetSequenceReadCount() == 0
			&& ArcFixture.Coordinator
				.GetNextPlayerThrownWeaponActivationSequence() == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewProductBridgeRunFenceTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewProductBridge.RunFence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewProductBridgeRunFenceTest::RunTest(
	const FString&)
{
	using EBridgeStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewProductBridgeStatus;
	FThrownLifecycleFixture Fixture;
	if (!Fixture.Start(
			*this,
			TEXT("ArcPreviewProductBridgeRunFence"),
			Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::BallisticArc))
	{
		return false;
	}
	FShanmenItemAuthoritySnapshot Before;
	Fixture.Authority->TryCaptureSnapshot(Before);
	Fdemo_mapCombatRunCoordinator EmptyCoordinator;
	const auto Rejected =
		Fdemo_mapShanmenThrownWeaponArcPreviewProductBridge::Capture(
			2,
			MakeArcPreviewChoice(),
			MakeArcPreviewChoicePolicy(),
			8,
			Fixture.Lifecycle,
			EmptyCoordinator);
	FShanmenItemAuthoritySnapshot After;
	Fixture.Authority->TryCaptureSnapshot(After);
	TestTrue(TEXT("inactive coordinator rejects after immutable product read"),
		Rejected.GetStatus() == EBridgeStatus::RunUnavailable
			&& Rejected.GetLifecycleRunId() == Fixture.Correlation.ActiveRunId
			&& !Rejected.GetCoordinatorRunId().IsValid()
			&& Rejected.GetSequenceReadCount() == 0
			&& Before == After
			&& Fixture.Coordinator
				.GetNextPlayerThrownWeaponActivationSequence() == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewProductBridgeCompositionTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewProductBridge.CompositionCompatibility",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewProductBridgeCompositionTest::RunTest(
	const FString&)
{
	FThrownLifecycleFixture Fixture;
	if (!Fixture.Start(
			*this,
			TEXT("ArcPreviewProductBridgeComposition"),
			Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::BallisticArc))
	{
		return false;
	}
	const auto Choice = MakeArcPreviewChoice();
	const auto Bridge =
		Fdemo_mapShanmenThrownWeaponArcPreviewProductBridge::Capture(
			2,
			Choice,
			MakeArcPreviewChoicePolicy(),
			8,
			Fixture.Lifecycle,
			Fixture.Coordinator);
	const auto Basis = MakeArcPreviewBasis();
	const auto Preview =
		Fdemo_mapShanmenThrownWeaponArcPreviewComposition::Compose(
			Bridge.GetCapture().GetConfiguration(),
			[&Choice]() { return Choice; },
			[&Basis]() { return Basis; });
	TestTrue(TEXT("live product bridge captures canonical preview inputs"),
		Bridge.IsCaptured());
	TestTrue(TEXT("live product bridge output composes into pure geometry"),
		Preview.IsComposed()
			&& Preview.GetConfiguration().GetAction().GetActivationId()
				== Bridge.GetCapture().GetPreviewActivationId()
			&& Preview.GetProjection().GetChoiceRevision()
				== Bridge.GetChoiceState().GetRevision()
			&& Fixture.Coordinator
				.GetNextPlayerThrownWeaponActivationSequence() == 1
			&& Fixture.Lifecycle.NumCapturedSelections() == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewPresentationVisibleTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentation.VisibleSnapshot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewPresentationVisibleTest::RunTest(
	const FString&)
{
	FThrownLifecycleFixture Fixture;
	if (!Fixture.Start(
			*this,
			TEXT("ArcPreviewPresentationVisible"),
			Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::BallisticArc))
	{
		return false;
	}
	FShanmenItemAuthoritySnapshot Before;
	Fixture.Authority->TryCaptureSnapshot(Before);
	const uint64 SequenceBefore = Fixture.Coordinator
		.GetNextPlayerThrownWeaponActivationSequence();
	const auto Choice = MakeArcPreviewChoice(false);
	const auto Bridge =
		Fdemo_mapShanmenThrownWeaponArcPreviewProductBridge::Capture(
			2,
			Choice,
			MakeArcPreviewChoicePolicy(),
			8,
			Fixture.Lifecycle,
			Fixture.Coordinator);
	const auto Projected =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationProjector::Project(
			Bridge, MakeArcPreviewBasis());
	FShanmenItemAuthoritySnapshot After;
	Fixture.Authority->TryCaptureSnapshot(After);

	const auto& State = Projected.GetState();
	TestTrue(TEXT("captured product preview projects one visible snapshot"),
		Projected.IsProjected()
			&& Projected.GetCompositionCount() == 1
			&& State.IsVisible()
			&& State.GetRunId() == Bridge.GetLifecycleRunId()
			&& State.GetPlayerEntityId() == Bridge.GetPlayerEntityId()
			&& State.GetSourceItemInstanceId()
				== Bridge.GetSourceItemInstanceId()
			&& State.GetSourceProductRequestId()
				== Bridge.GetPreviewRequestId()
			&& State.GetSourcePreviewActivationId()
				== Bridge.GetCapture().GetPreviewActivationId()
			&& State.GetChoiceState().Matches(Choice)
			&& State.NumSegments() == 8
			&& State.GetSourcePreview().GetPositions().Num() == 9);
	bool bSegmentsMatch = State.NumSegments() == 8;
	for (int32 Index = 0; bSegmentsMatch && Index < State.NumSegments(); ++Index)
	{
		const auto& Segment = State.GetSegments()[Index];
		bSegmentsMatch = Segment.IsValid()
			&& Segment.GetIndex() == Index
			&& Segment.GetStart()
				== State.GetSourcePreview().GetPositions()[Index]
			&& Segment.GetEnd()
				== State.GetSourcePreview().GetPositions()[Index + 1];
	}
	TestTrue(TEXT("line segments exactly cover the sampled path"),
		bSegmentsMatch
			&& State.GetApexPosition()
				== State.GetSourcePreview().GetApexPosition()
			&& State.GetPlannedLandingPosition()
				== State.GetSourcePreview().GetPlannedLandingPosition());
	TestTrue(TEXT("presentation projection leaves product authority untouched"),
		Before == After
			&& Fixture.Coordinator
				.GetNextPlayerThrownWeaponActivationSequence()
				== SequenceBefore
			&& Fixture.Lifecycle.GetHostState()
				== Edemo_mapShanmenThrownWeaponHostState::Empty
			&& Fixture.Lifecycle.NumCapturedSelections() == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewPresentationReplayTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentation.DeterministicDuplicate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewPresentationReplayTest::RunTest(
	const FString&)
{
	FThrownLifecycleFixture Fixture;
	if (!Fixture.Start(
			*this,
			TEXT("ArcPreviewPresentationReplay"),
			Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::BallisticArc))
	{
		return false;
	}
	const auto Bridge =
		Fdemo_mapShanmenThrownWeaponArcPreviewProductBridge::Capture(
			2,
			MakeArcPreviewChoice(false),
			MakeArcPreviewChoicePolicy(),
			8,
			Fixture.Lifecycle,
			Fixture.Coordinator);
	const auto Basis = MakeArcPreviewBasis();
	const auto First =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationProjector::Project(
			Bridge, Basis);
	const auto Replay =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationProjector::Project(
			Bridge, Basis);
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState Empty;
	const auto Installed =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationReducer::Replace(
			Empty, First.GetState());
	const auto Duplicate =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationReducer::Replace(
			Installed.GetState(), Replay.GetState());
	TestTrue(TEXT("equal inputs reproduce exact renderer-neutral evidence"),
		First.IsProjected()
			&& Replay.IsProjected()
			&& First.GetState().Matches(Replay.GetState())
			&& Installed.IsReplaced()
			&& Duplicate.IsDuplicate()
			&& Duplicate.GetState().Matches(Installed.GetState()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewPresentationReplaceTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentation.NewerRevisionReplace",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewPresentationReplaceTest::RunTest(
	const FString&)
{
	FThrownLifecycleFixture Fixture;
	if (!Fixture.Start(
			*this,
			TEXT("ArcPreviewPresentationReplace"),
			Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::BallisticArc))
	{
		return false;
	}
	const auto Policy = MakeArcPreviewChoicePolicy();
	const auto Basis = MakeArcPreviewBasis();
	const auto InitialChoice = MakeArcPreviewChoice(false);
	const auto RevisedChoice = MakeArcPreviewChoice(true);
	const auto InitialBridge =
		Fdemo_mapShanmenThrownWeaponArcPreviewProductBridge::Capture(
			2, InitialChoice, Policy, 8, Fixture.Lifecycle, Fixture.Coordinator);
	const auto RevisedBridge =
		Fdemo_mapShanmenThrownWeaponArcPreviewProductBridge::Capture(
			2, RevisedChoice, Policy, 8, Fixture.Lifecycle, Fixture.Coordinator);
	const auto Initial =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationProjector::Project(
			InitialBridge, Basis);
	const auto Revised =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationProjector::Project(
			RevisedBridge, Basis);
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState Empty;
	const auto Installed =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationReducer::Replace(
			Empty, Initial.GetState());
	const auto Replaced =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationReducer::Replace(
			Installed.GetState(), Revised.GetState());
	TestTrue(TEXT("newer choice revision atomically replaces visible geometry"),
		Initial.IsProjected()
			&& Revised.IsProjected()
			&& Replaced.IsReplaced()
			&& Replaced.GetState().GetChoiceRevision()
				== Installed.GetState().GetChoiceRevision() + 1
			&& Replaced.GetState().GetPresentationStateId()
				!= Installed.GetState().GetPresentationStateId()
			&& Replaced.GetState().GetSourcePreview().GetPreviewId()
				!= Installed.GetState().GetSourcePreview().GetPreviewId()
			&& Fixture.Coordinator
				.GetNextPlayerThrownWeaponActivationSequence() == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewPresentationClearTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentation.ClearTombstone",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewPresentationClearTest::RunTest(
	const FString&)
{
	FThrownLifecycleFixture Fixture;
	if (!Fixture.Start(
			*this,
			TEXT("ArcPreviewPresentationClear"),
			Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::BallisticArc))
	{
		return false;
	}
	FShanmenItemAuthoritySnapshot Before;
	Fixture.Authority->TryCaptureSnapshot(Before);
	const auto Choice = MakeArcPreviewChoice(false);
	const auto Bridge =
		Fdemo_mapShanmenThrownWeaponArcPreviewProductBridge::Capture(
			2,
			Choice,
			MakeArcPreviewChoicePolicy(),
			8,
			Fixture.Lifecycle,
			Fixture.Coordinator);
	const auto Projected =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationProjector::Project(
			Bridge, MakeArcPreviewBasis());
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState Empty;
	const auto Installed =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationReducer::Replace(
			Empty, Projected.GetState());
	const auto ClearedChoice = ClearArcPreviewChoice(Choice);
	const auto Cleared =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationReducer::Clear(
			Installed.GetState(), ClearedChoice);
	const auto Stale =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationReducer::Replace(
			Cleared.GetState(), Installed.GetState());
	FShanmenItemAuthoritySnapshot After;
	Fixture.Authority->TryCaptureSnapshot(After);
	using EReduce =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationReduceStatus;
	TestTrue(TEXT("clear emits a newer hidden tombstone without geometry"),
		Cleared.IsCleared()
			&& Cleared.GetState().GetChoiceRevision()
				== Installed.GetState().GetChoiceRevision() + 1
			&& Cleared.GetState().NumSegments() == 0
			&& !Cleared.GetState().GetSourceProductRequestId().IsValid()
			&& !Cleared.GetState().GetSourcePreviewActivationId().IsValid()
			&& !Cleared.GetState().GetSourcePreview().IsValid());
	TestTrue(TEXT("hidden revision fences stale asynchronous geometry"),
		Stale.GetStatus() == EReduce::StaleRevision
			&& Stale.GetState().IsEmpty()
			&& Before == After
			&& Fixture.Coordinator
				.GetNextPlayerThrownWeaponActivationSequence() == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewPresentationRetargetTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentation.RetargetAfterClear",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewPresentationRetargetTest::RunTest(
	const FString&)
{
	FThrownLifecycleFixture Fixture;
	if (!Fixture.Start(
			*this,
			TEXT("ArcPreviewPresentationRetarget"),
			Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::BallisticArc))
	{
		return false;
	}
	const auto Policy = MakeArcPreviewChoicePolicy();
	const auto Basis = MakeArcPreviewBasis();
	const auto InitialChoice = MakeArcPreviewChoice(false);
	const auto InitialBridge =
		Fdemo_mapShanmenThrownWeaponArcPreviewProductBridge::Capture(
			2,
			InitialChoice,
			Policy,
			8,
			Fixture.Lifecycle,
			Fixture.Coordinator);
	const auto Initial =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationProjector::Project(
			InitialBridge, Basis);
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState Empty;
	const auto Installed =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationReducer::Replace(
			Empty, Initial.GetState());
	const auto ClearedChoice = ClearArcPreviewChoice(InitialChoice);
	const auto Hidden =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationReducer::Clear(
			Installed.GetState(), ClearedChoice);
	const auto RetargetedChoice = RetargetArcPreviewChoice(ClearedChoice);
	const auto RetargetedBridge =
		Fdemo_mapShanmenThrownWeaponArcPreviewProductBridge::Capture(
			2,
			RetargetedChoice,
			Policy,
			8,
			Fixture.Lifecycle,
			Fixture.Coordinator);
	const auto Retargeted =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationProjector::Project(
			RetargetedBridge, Basis);
	const auto Replaced =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationReducer::Replace(
			Hidden.GetState(), Retargeted.GetState());
	TestTrue(TEXT("new target after clear installs only its newest geometry"),
		Hidden.IsCleared()
			&& Retargeted.IsProjected()
			&& Replaced.IsReplaced()
			&& Replaced.GetState().IsVisible()
			&& Replaced.GetState().GetChoiceRevision()
				== Hidden.GetState().GetChoiceRevision() + 1
			&& Replaced.GetState().NumSegments() == 8);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewPresentationScopeTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentation.ScopeFence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewPresentationScopeTest::RunTest(
	const FString&)
{
	FThrownLifecycleFixture FirstFixture;
	FThrownLifecycleFixture OtherFixture;
	if (!FirstFixture.Start(
			*this,
			TEXT("ArcPreviewPresentationScopeA"),
			Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::BallisticArc)
		|| !OtherFixture.Start(
			*this,
			TEXT("ArcPreviewPresentationScopeB"),
			Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::BallisticArc))
	{
		return false;
	}
	const auto Choice = MakeArcPreviewChoice(false);
	const auto Policy = MakeArcPreviewChoicePolicy();
	const auto Basis = MakeArcPreviewBasis();
	const auto FirstBridge =
		Fdemo_mapShanmenThrownWeaponArcPreviewProductBridge::Capture(
			2,
			Choice,
			Policy,
			8,
			FirstFixture.Lifecycle,
			FirstFixture.Coordinator);
	const auto OtherBridge =
		Fdemo_mapShanmenThrownWeaponArcPreviewProductBridge::Capture(
			2,
			Choice,
			Policy,
			8,
			OtherFixture.Lifecycle,
			OtherFixture.Coordinator);
	const auto First =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationProjector::Project(
			FirstBridge, Basis);
	const auto Other =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationProjector::Project(
			OtherBridge, Basis);
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState Empty;
	const auto Installed =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationReducer::Replace(
			Empty, First.GetState());
	const auto Rejected =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationReducer::Replace(
			Installed.GetState(), Other.GetState());
	using EReduce =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationReduceStatus;
	TestTrue(TEXT("consumer refuses geometry from another Run or item scope"),
		First.IsProjected()
			&& Other.IsProjected()
			&& Rejected.GetStatus() == EReduce::IdentityMismatch
			&& Rejected.GetState().IsEmpty()
			&& FirstFixture.Coordinator
				.GetNextPlayerThrownWeaponActivationSequence() == 1
			&& OtherFixture.Coordinator
				.GetNextPlayerThrownWeaponActivationSequence() == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewPresentationSourceFenceTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentation.SourceFences",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewPresentationSourceFenceTest::RunTest(
	const FString&)
{
	using EProject =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationProjectStatus;
	Fdemo_mapShanmenThrownWeaponArcPreviewProductBridgeResult EmptyBridge;
	const auto MissingBridge =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationProjector::Project(
			EmptyBridge, MakeArcPreviewBasis());

	FThrownLifecycleFixture Fixture;
	if (!Fixture.Start(
			*this,
			TEXT("ArcPreviewPresentationSourceFence"),
			Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::BallisticArc))
	{
		return false;
	}
	const auto ValidBridge =
		Fdemo_mapShanmenThrownWeaponArcPreviewProductBridge::Capture(
			2,
			MakeArcPreviewChoice(false),
			MakeArcPreviewChoicePolicy(),
			8,
			Fixture.Lifecycle,
			Fixture.Coordinator);
	Fdemo_mapShanmenThrownWeaponArcChoiceBasis EmptyBasis;
	const auto MissingBasis =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationProjector::Project(
			ValidBridge, EmptyBasis);
	const auto UnreachableBridge =
		Fdemo_mapShanmenThrownWeaponArcPreviewProductBridge::Capture(
			2,
			MakeUnreachableArcPreviewChoice(),
			MakeArcPreviewChoicePolicy(),
			8,
			Fixture.Lifecycle,
			Fixture.Coordinator);
	const auto Unreachable =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationProjector::Project(
			UnreachableBridge, MakeArcPreviewBasis());
	TestTrue(TEXT("invalid source stages fail closed with typed status"),
		MissingBridge.GetStatus() == EProject::BridgeRejected
			&& MissingBridge.GetCompositionCount() == 0
			&& MissingBridge.GetState().IsEmpty()
			&& MissingBasis.GetStatus() == EProject::BasisRejected
			&& MissingBasis.GetCompositionCount() == 0
			&& MissingBasis.GetState().IsEmpty()
			&& UnreachableBridge.IsCaptured()
			&& Unreachable.GetStatus() == EProject::CompositionRejected
			&& Unreachable.GetCompositionCount() == 1
			&& Unreachable.GetState().IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewPresentationClearFenceTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentation.ClearFences",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewPresentationClearFenceTest::RunTest(
	const FString&)
{
	using EReduce =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationReduceStatus;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState Empty;
	const auto Choice = MakeArcPreviewChoice(false);
	const auto NoPrevious =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationReducer::Clear(
			Empty, ClearArcPreviewChoice(Choice));

	FThrownLifecycleFixture Fixture;
	if (!Fixture.Start(
			*this,
			TEXT("ArcPreviewPresentationClearFence"),
			Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::BallisticArc))
	{
		return false;
	}
	const auto Bridge =
		Fdemo_mapShanmenThrownWeaponArcPreviewProductBridge::Capture(
			2,
			Choice,
			MakeArcPreviewChoicePolicy(),
			8,
			Fixture.Lifecycle,
			Fixture.Coordinator);
	const auto Projected =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationProjector::Project(
			Bridge, MakeArcPreviewBasis());
	const auto Installed =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationReducer::Replace(
			Empty, Projected.GetState());
	Fdemo_mapShanmenThrownWeaponInputChoiceState InvalidChoice;
	const auto Invalid =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationReducer::Clear(
			Installed.GetState(), InvalidChoice);
	const auto SameRevision =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationReducer::Clear(
			Installed.GetState(), Choice);
	const auto NewerVisible =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationReducer::Clear(
			Installed.GetState(), MakeArcPreviewChoice(true));
	const auto Cleared =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationReducer::Clear(
			Installed.GetState(), ClearArcPreviewChoice(Choice));
	const auto Duplicate =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationReducer::Clear(
			Cleared.GetState(), Cleared.GetState().GetChoiceState());
	TestTrue(TEXT("clear requires a newer non-preview choice and prior state"),
		NoPrevious.GetStatus() == EReduce::PreviousStateRequired
			&& Invalid.GetStatus() == EReduce::ChoiceRejected
			&& SameRevision.GetStatus() == EReduce::RevisionConflict
			&& NewerVisible.GetStatus() == EReduce::ClearNotRequired
			&& Cleared.IsCleared()
			&& Duplicate.IsDuplicate()
			&& Duplicate.GetState().Matches(Cleared.GetState()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewUpdateVisibleInstallTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewUpdateCoordinator.VisibleInstall",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewUpdateVisibleInstallTest::RunTest(
	const FString&)
{
	using EUpdate =
		Edemo_mapShanmenThrownWeaponArcPreviewUpdateStatus;
	FThrownLifecycleFixture Fixture;
	if (!Fixture.Start(
			*this,
			TEXT("ArcPreviewUpdateVisible"),
			Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::BallisticArc))
	{
		return false;
	}
	FShanmenItemAuthoritySnapshot Before;
	Fixture.Authority->TryCaptureSnapshot(Before);
	const uint64 SequenceBefore = Fixture.Coordinator
		.GetNextPlayerThrownWeaponActivationSequence();
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState Empty;
	const auto Choice = MakeArcPreviewChoice(false);
	const auto Updated =
		Fdemo_mapShanmenThrownWeaponArcPreviewUpdateCoordinator::Update(
			2,
			Choice,
			MakeArcPreviewChoicePolicy(),
			8,
			MakeArcPreviewBasis(),
			Empty,
			Fixture.Lifecycle,
			Fixture.Coordinator);
	FShanmenItemAuthoritySnapshot After;
	Fixture.Authority->TryCaptureSnapshot(After);
	TestTrue(TEXT("one call installs one live renderer-neutral snapshot"),
		Updated.IsValid()
			&& Updated.IsCompleted()
			&& Updated.DidChange()
			&& !Updated.IsNoChange()
			&& Updated.GetStatus() == EUpdate::Replaced
			&& Updated.GetCaptureCount() == 1
			&& Updated.GetProjectCount() == 1
			&& Updated.GetReduceCount() == 1
			&& Updated.GetBridge().IsCaptured()
			&& Updated.GetProjection().IsProjected()
			&& Updated.GetReduction().IsReplaced()
			&& Updated.GetPreviousState().IsEmpty()
			&& Updated.GetChoiceState().Matches(Choice)
			&& Updated.GetState().IsVisible()
			&& Updated.GetState().NumSegments() == 8);
	TestTrue(TEXT("bounded update leaves every product authority unchanged"),
		Before == After
			&& Fixture.Coordinator
				.GetNextPlayerThrownWeaponActivationSequence()
				== SequenceBefore
			&& Fixture.Lifecycle.GetHostState()
				== Edemo_mapShanmenThrownWeaponHostState::Empty
			&& Fixture.Lifecycle.NumCapturedSelections() == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewUpdateDuplicateTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewUpdateCoordinator.DeterministicDuplicate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewUpdateDuplicateTest::RunTest(
	const FString&)
{
	using EUpdate =
		Edemo_mapShanmenThrownWeaponArcPreviewUpdateStatus;
	FThrownLifecycleFixture Fixture;
	if (!Fixture.Start(
			*this,
			TEXT("ArcPreviewUpdateDuplicate"),
			Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::BallisticArc))
	{
		return false;
	}
	const auto Choice = MakeArcPreviewChoice(false);
	const auto Policy = MakeArcPreviewChoicePolicy();
	const auto Basis = MakeArcPreviewBasis();
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState Empty;
	const auto First =
		Fdemo_mapShanmenThrownWeaponArcPreviewUpdateCoordinator::Update(
			2, Choice, Policy, 8, Basis, Empty,
			Fixture.Lifecycle, Fixture.Coordinator);
	const auto Replay =
		Fdemo_mapShanmenThrownWeaponArcPreviewUpdateCoordinator::Update(
			2, Choice, Policy, 8, Basis, First.GetState(),
			Fixture.Lifecycle, Fixture.Coordinator);
	TestTrue(TEXT("identical update is a deterministic bounded duplicate"),
		First.GetStatus() == EUpdate::Replaced
			&& Replay.IsValid()
			&& Replay.IsCompleted()
			&& Replay.IsNoChange()
			&& !Replay.DidChange()
			&& Replay.GetStatus() == EUpdate::Duplicate
			&& Replay.GetCaptureCount() == 1
			&& Replay.GetProjectCount() == 1
			&& Replay.GetReduceCount() == 1
			&& Replay.GetReduction().IsDuplicate()
			&& Replay.GetState().Matches(First.GetState())
			&& Fixture.Coordinator
				.GetNextPlayerThrownWeaponActivationSequence() == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewUpdateRevisionReplaceTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewUpdateCoordinator.NewerRevisionReplace",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewUpdateRevisionReplaceTest::RunTest(
	const FString&)
{
	using EUpdate =
		Edemo_mapShanmenThrownWeaponArcPreviewUpdateStatus;
	FThrownLifecycleFixture Fixture;
	if (!Fixture.Start(
			*this,
			TEXT("ArcPreviewUpdateRevision"),
			Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::BallisticArc))
	{
		return false;
	}
	const auto Policy = MakeArcPreviewChoicePolicy();
	const auto Basis = MakeArcPreviewBasis();
	const auto InitialChoice = MakeArcPreviewChoice(false);
	const auto RevisedChoice = MakeArcPreviewChoice(true);
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState Empty;
	const auto Initial =
		Fdemo_mapShanmenThrownWeaponArcPreviewUpdateCoordinator::Update(
			2, InitialChoice, Policy, 8, Basis, Empty,
			Fixture.Lifecycle, Fixture.Coordinator);
	const auto Revised =
		Fdemo_mapShanmenThrownWeaponArcPreviewUpdateCoordinator::Update(
			2, RevisedChoice, Policy, 8, Basis, Initial.GetState(),
			Fixture.Lifecycle, Fixture.Coordinator);
	TestTrue(TEXT("one newer choice revision replaces the current snapshot"),
		Initial.GetStatus() == EUpdate::Replaced
			&& Revised.GetStatus() == EUpdate::Replaced
			&& Revised.IsValid()
			&& Revised.DidChange()
			&& Revised.GetState().GetChoiceRevision()
				== Initial.GetState().GetChoiceRevision() + 1
			&& Revised.GetState().GetPresentationStateId()
				!= Initial.GetState().GetPresentationStateId()
			&& Revised.GetState().GetSourcePreview().GetPreviewId()
				!= Initial.GetState().GetSourcePreview().GetPreviewId()
			&& Fixture.Coordinator
				.GetNextPlayerThrownWeaponActivationSequence() == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewUpdateClearWithoutProductTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewUpdateCoordinator.ClearWithoutProduct",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewUpdateClearWithoutProductTest::RunTest(
	const FString&)
{
	using EUpdate =
		Edemo_mapShanmenThrownWeaponArcPreviewUpdateStatus;
	Fdemo_mapShanmenThrownWeaponProductLifecycle EmptyLifecycle;
	Fdemo_mapCombatRunCoordinator EmptyCoordinator;
	Fdemo_mapShanmenThrownWeaponArcChoicePolicy EmptyPolicy;
	Fdemo_mapShanmenThrownWeaponArcChoiceBasis EmptyBasis;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState EmptyState;
	const auto NoPreview =
		Fdemo_mapShanmenThrownWeaponArcPreviewUpdateCoordinator::Update(
			0,
			Fdemo_mapShanmenThrownWeaponInputChoiceState::CreateInitial(),
			EmptyPolicy,
			0,
			EmptyBasis,
			EmptyState,
			EmptyLifecycle,
			EmptyCoordinator);
	TestTrue(TEXT("empty consumer needs no product read or presentation work"),
		NoPreview.IsValid()
			&& NoPreview.IsCompleted()
			&& NoPreview.IsNoChange()
			&& NoPreview.GetStatus() == EUpdate::NoPresentationRequired
			&& NoPreview.GetCaptureCount() == 0
			&& NoPreview.GetProjectCount() == 0
			&& NoPreview.GetReduceCount() == 0
			&& NoPreview.GetState().IsEmpty());

	FThrownLifecycleFixture Fixture;
	if (!Fixture.Start(
			*this,
			TEXT("ArcPreviewUpdateClear"),
			Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::BallisticArc))
	{
		return false;
	}
	const auto Choice = MakeArcPreviewChoice(false);
	const auto Visible =
		Fdemo_mapShanmenThrownWeaponArcPreviewUpdateCoordinator::Update(
			2,
			Choice,
			MakeArcPreviewChoicePolicy(),
			8,
			MakeArcPreviewBasis(),
			EmptyState,
			Fixture.Lifecycle,
			Fixture.Coordinator);
	const auto ClearChoice = ClearArcPreviewChoice(Choice);
	Fixture.Stop();
	const auto Cleared =
		Fdemo_mapShanmenThrownWeaponArcPreviewUpdateCoordinator::Update(
			0,
			ClearChoice,
			EmptyPolicy,
			0,
			EmptyBasis,
			Visible.GetState(),
			Fixture.Lifecycle,
			Fixture.Coordinator);
	TestTrue(TEXT("clear remains available after product and Run shutdown"),
		Visible.GetStatus() == EUpdate::Replaced
			&& !Fixture.Lifecycle.IsActive()
			&& !Fixture.Coordinator.IsActive()
			&& Cleared.IsValid()
			&& Cleared.GetStatus() == EUpdate::Cleared
			&& Cleared.DidChange()
			&& Cleared.GetCaptureCount() == 0
			&& Cleared.GetProjectCount() == 0
			&& Cleared.GetReduceCount() == 1
			&& Cleared.GetState().IsHidden()
			&& Cleared.GetState().NumSegments() == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewUpdateRetargetTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewUpdateCoordinator.RetargetAfterClear",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewUpdateRetargetTest::RunTest(
	const FString&)
{
	using EUpdate =
		Edemo_mapShanmenThrownWeaponArcPreviewUpdateStatus;
	FThrownLifecycleFixture Fixture;
	if (!Fixture.Start(
			*this,
			TEXT("ArcPreviewUpdateRetarget"),
			Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::BallisticArc))
	{
		return false;
	}
	const auto Policy = MakeArcPreviewChoicePolicy();
	const auto Basis = MakeArcPreviewBasis();
	const auto InitialChoice = MakeArcPreviewChoice(false);
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState Empty;
	const auto Initial =
		Fdemo_mapShanmenThrownWeaponArcPreviewUpdateCoordinator::Update(
			2, InitialChoice, Policy, 8, Basis, Empty,
			Fixture.Lifecycle, Fixture.Coordinator);
	const auto ClearChoice = ClearArcPreviewChoice(InitialChoice);
	const auto Cleared =
		Fdemo_mapShanmenThrownWeaponArcPreviewUpdateCoordinator::Update(
			2, ClearChoice, Policy, 8, Basis, Initial.GetState(),
			Fixture.Lifecycle, Fixture.Coordinator);
	const auto RetargetedChoice = RetargetArcPreviewChoice(ClearChoice);
	const auto Retargeted =
		Fdemo_mapShanmenThrownWeaponArcPreviewUpdateCoordinator::Update(
			2, RetargetedChoice, Policy, 8, Basis, Cleared.GetState(),
			Fixture.Lifecycle, Fixture.Coordinator);
	TestTrue(TEXT("clear tombstone accepts only the subsequent retarget"),
		Initial.GetStatus() == EUpdate::Replaced
			&& Cleared.GetStatus() == EUpdate::Cleared
			&& Cleared.GetState().IsHidden()
			&& Retargeted.GetStatus() == EUpdate::Replaced
			&& Retargeted.GetState().IsVisible()
			&& Retargeted.GetState().GetChoiceRevision()
				== Cleared.GetState().GetChoiceRevision() + 1
			&& Retargeted.GetState().NumSegments() == 8);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewUpdateStageFenceTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewUpdateCoordinator.StageFences",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewUpdateStageFenceTest::RunTest(
	const FString&)
{
	using EUpdate =
		Edemo_mapShanmenThrownWeaponArcPreviewUpdateStatus;
	using EBridge =
		Edemo_mapShanmenThrownWeaponArcPreviewProductBridgeStatus;
	using EProject =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationProjectStatus;
	FThrownLifecycleFixture Fixture;
	if (!Fixture.Start(
			*this,
			TEXT("ArcPreviewUpdateStageFence"),
			Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::BallisticArc))
	{
		return false;
	}
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState EmptyState;
	Fdemo_mapShanmenThrownWeaponInputChoiceState InvalidChoice;
	Fdemo_mapShanmenThrownWeaponArcChoicePolicy EmptyPolicy;
	Fdemo_mapShanmenThrownWeaponArcChoiceBasis EmptyBasis;
	const auto Choice = MakeArcPreviewChoice(false);
	const auto Policy = MakeArcPreviewChoicePolicy();
	const auto Basis = MakeArcPreviewBasis();
	const auto BadChoice =
		Fdemo_mapShanmenThrownWeaponArcPreviewUpdateCoordinator::Update(
			2, InvalidChoice, Policy, 8, Basis, EmptyState,
			Fixture.Lifecycle, Fixture.Coordinator);
	const auto BadCapture =
		Fdemo_mapShanmenThrownWeaponArcPreviewUpdateCoordinator::Update(
			0, Choice, Policy, 8, Basis, EmptyState,
			Fixture.Lifecycle, Fixture.Coordinator);
	const auto BadProject =
		Fdemo_mapShanmenThrownWeaponArcPreviewUpdateCoordinator::Update(
			2, Choice, Policy, 8, EmptyBasis, EmptyState,
			Fixture.Lifecycle, Fixture.Coordinator);
	TestTrue(TEXT("each rejected stage stops every later stage"),
		BadChoice.IsValid()
			&& BadChoice.GetStatus() == EUpdate::ChoiceRejected
			&& BadChoice.GetCaptureCount() == 0
			&& BadChoice.GetProjectCount() == 0
			&& BadChoice.GetReduceCount() == 0
			&& BadCapture.IsValid()
			&& BadCapture.GetStatus() == EUpdate::CaptureRejected
			&& BadCapture.GetBridge().GetStatus() == EBridge::InputRejected
			&& BadCapture.GetCaptureCount() == 1
			&& BadCapture.GetProjectCount() == 0
			&& BadCapture.GetReduceCount() == 0
			&& BadProject.IsValid()
			&& BadProject.GetStatus() == EUpdate::ProjectRejected
			&& BadProject.GetProjection().GetStatus()
				== EProject::BasisRejected
			&& BadProject.GetCaptureCount() == 1
			&& BadProject.GetProjectCount() == 1
			&& BadProject.GetReduceCount() == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewUpdateRevisionFenceTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewUpdateCoordinator.RevisionFences",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewUpdateRevisionFenceTest::RunTest(
	const FString&)
{
	using EUpdate =
		Edemo_mapShanmenThrownWeaponArcPreviewUpdateStatus;
	using EReduce =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationReduceStatus;
	FThrownLifecycleFixture Fixture;
	if (!Fixture.Start(
			*this,
			TEXT("ArcPreviewUpdateRevisionFence"),
			Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::BallisticArc))
	{
		return false;
	}
	const auto Policy = MakeArcPreviewChoicePolicy();
	const auto InitialChoice = MakeArcPreviewChoice(false);
	const auto RevisedChoice = MakeArcPreviewChoice(true);
	const auto FirstBasis = MakeArcPreviewBasis();
	Fdemo_mapShanmenThrownWeaponArcChoiceBasis ShiftedBasis;
	check(Fdemo_mapShanmenThrownWeaponArcChoiceBasis::TryCapture(
		FVector(125.0, 200.0, 50.0),
		FVector::ForwardVector,
		FVector::RightVector,
		ShiftedBasis));
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState Empty;
	const auto Initial =
		Fdemo_mapShanmenThrownWeaponArcPreviewUpdateCoordinator::Update(
			2, InitialChoice, Policy, 8, FirstBasis, Empty,
			Fixture.Lifecycle, Fixture.Coordinator);
	const auto GeometryConflict =
		Fdemo_mapShanmenThrownWeaponArcPreviewUpdateCoordinator::Update(
			2, InitialChoice, Policy, 8, ShiftedBasis, Initial.GetState(),
			Fixture.Lifecycle, Fixture.Coordinator);
	const auto Revised =
		Fdemo_mapShanmenThrownWeaponArcPreviewUpdateCoordinator::Update(
			2, RevisedChoice, Policy, 8, FirstBasis, Initial.GetState(),
			Fixture.Lifecycle, Fixture.Coordinator);
	const auto Stale =
		Fdemo_mapShanmenThrownWeaponArcPreviewUpdateCoordinator::Update(
			2, InitialChoice, Policy, 8, FirstBasis, Revised.GetState(),
			Fixture.Lifecycle, Fixture.Coordinator);
	TestTrue(TEXT("equal-revision drift and older async work fail closed"),
		Initial.GetStatus() == EUpdate::Replaced
			&& GeometryConflict.IsValid()
			&& GeometryConflict.GetStatus() == EUpdate::ReduceRejected
			&& GeometryConflict.GetReduction().GetStatus()
				== EReduce::RevisionConflict
			&& GeometryConflict.GetState().IsEmpty()
			&& Revised.GetStatus() == EUpdate::Replaced
			&& Stale.IsValid()
			&& Stale.GetStatus() == EUpdate::ReduceRejected
			&& Stale.GetReduction().GetStatus() == EReduce::StaleRevision
			&& Stale.GetState().IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewUpdateScopeFenceTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewUpdateCoordinator.ScopeFence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewUpdateScopeFenceTest::RunTest(
	const FString&)
{
	using EUpdate =
		Edemo_mapShanmenThrownWeaponArcPreviewUpdateStatus;
	using EReduce =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationReduceStatus;
	FThrownLifecycleFixture FirstFixture;
	FThrownLifecycleFixture ForeignFixture;
	if (!FirstFixture.Start(
			*this,
			TEXT("ArcPreviewUpdateScopeA"),
			Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::BallisticArc)
		|| !ForeignFixture.Start(
			*this,
			TEXT("ArcPreviewUpdateScopeB"),
			Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::BallisticArc))
	{
		return false;
	}
	const auto Choice = MakeArcPreviewChoice(false);
	const auto Policy = MakeArcPreviewChoicePolicy();
	const auto Basis = MakeArcPreviewBasis();
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState Empty;
	const auto First =
		Fdemo_mapShanmenThrownWeaponArcPreviewUpdateCoordinator::Update(
			2, Choice, Policy, 8, Basis, Empty,
			FirstFixture.Lifecycle, FirstFixture.Coordinator);
	const auto Foreign =
		Fdemo_mapShanmenThrownWeaponArcPreviewUpdateCoordinator::Update(
			2, Choice, Policy, 8, Basis, First.GetState(),
			ForeignFixture.Lifecycle, ForeignFixture.Coordinator);
	TestTrue(TEXT("one consumer state cannot cross Run player or item scope"),
		First.GetStatus() == EUpdate::Replaced
			&& Foreign.IsValid()
			&& Foreign.GetStatus() == EUpdate::ReduceRejected
			&& Foreign.GetCaptureCount() == 1
			&& Foreign.GetProjectCount() == 1
			&& Foreign.GetReduceCount() == 1
			&& Foreign.GetReduction().GetStatus()
				== EReduce::IdentityMismatch
			&& Foreign.GetState().IsEmpty()
			&& FirstFixture.Coordinator
				.GetNextPlayerThrownWeaponActivationSequence() == 1
			&& ForeignFixture.Coordinator
				.GetNextPlayerThrownWeaponActivationSequence() == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewPresentationSessionRunBindingTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationSession.RunBinding",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewPresentationSessionRunBindingTest::
	RunTest(const FString&)
{
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSession Session;
	const FGuid FirstRun = FGuid::NewGuid();
	const FGuid OtherRun = FGuid::NewGuid();
	FString Diagnostic;
	TestTrue(TEXT("default presentation consumer is valid and empty"),
		Session.IsValid() && Session.IsEmpty() && !Session.IsActive());
	TestFalse(TEXT("invalid Run identity cannot bind presentation state"),
		Session.TryBegin(FGuid(), Diagnostic));
	TestTrue(TEXT("one valid Run binds an empty consumer"),
		Session.TryBegin(FirstRun, Diagnostic)
			&& Session.IsValid()
			&& Session.IsActive()
			&& !Session.IsEmpty()
			&& Session.GetRunId() == FirstRun
			&& Session.GetState().IsEmpty());
	TestTrue(TEXT("exact Run begin is idempotent"),
		Session.TryBegin(FirstRun, Diagnostic)
			&& Session.GetRunId() == FirstRun);
	TestFalse(TEXT("active consumer rejects silent Run rotation"),
		Session.TryBegin(OtherRun, Diagnostic));
	TestTrue(TEXT("exact Run end returns the consumer to empty state"),
		Session.TryEnd(FirstRun, Diagnostic)
			&& Session.IsEmpty()
			&& Session.GetState().IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewPresentationSessionVisibleCommitTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationSession.VisibleCommit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewPresentationSessionVisibleCommitTest::
	RunTest(const FString&)
{
	using ESession =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationSessionStatus;
	using EUpdate = Edemo_mapShanmenThrownWeaponArcPreviewUpdateStatus;
	FThrownLifecycleFixture Fixture;
	if (!Fixture.Start(
			*this,
			TEXT("ArcPreviewPresentationSessionVisible"),
			Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::BallisticArc))
	{
		return false;
	}
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSession Session;
	FString Diagnostic;
	if (!Session.TryBegin(Fixture.Correlation.ActiveRunId, Diagnostic))
	{
		AddError(Diagnostic);
		return false;
	}
	FShanmenItemAuthoritySnapshot Before;
	Fixture.Authority->TryCaptureSnapshot(Before);
	const uint64 SequenceBefore = Fixture.Coordinator
		.GetNextPlayerThrownWeaponActivationSequence();
	const auto Result = Session.TryUpdate(
		2,
		MakeArcPreviewChoice(false),
		MakeArcPreviewChoicePolicy(),
		8,
		MakeArcPreviewBasis(),
		Fixture.Lifecycle,
		Fixture.Coordinator);
	FShanmenItemAuthoritySnapshot After;
	Fixture.Authority->TryCaptureSnapshot(After);
	TestTrue(TEXT("session atomically adopts one completed bounded update"),
		Result.IsValid()
			&& Result.IsAccepted()
			&& Result.DidChange()
			&& !Result.IsNoChange()
			&& Result.GetStatus() == ESession::Applied
			&& Result.GetCoordinatorCallCount() == 1
			&& Result.GetUpdate().GetStatus() == EUpdate::Replaced
			&& Result.GetPreviousState().IsEmpty()
			&& Result.GetState().IsVisible()
			&& Session.GetState().Matches(Result.GetState())
			&& Session.GetState().GetRunId()
				== Fixture.Correlation.ActiveRunId);
	TestTrue(TEXT("presentation commit leaves product authority unchanged"),
		Before == After
			&& Fixture.Coordinator
				.GetNextPlayerThrownWeaponActivationSequence()
				== SequenceBefore
			&& Fixture.Lifecycle.GetHostState()
				== Edemo_mapShanmenThrownWeaponHostState::Empty
			&& Fixture.Lifecycle.NumCapturedSelections() == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewPresentationSessionRevisionTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationSession.DuplicateAndRevision",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewPresentationSessionRevisionTest::RunTest(
	const FString&)
{
	using ESession =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationSessionStatus;
	FThrownLifecycleFixture Fixture;
	if (!Fixture.Start(
			*this,
			TEXT("ArcPreviewPresentationSessionRevision"),
			Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::BallisticArc))
	{
		return false;
	}
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSession Session;
	FString Diagnostic;
	check(Session.TryBegin(Fixture.Correlation.ActiveRunId, Diagnostic));
	const auto InitialChoice = MakeArcPreviewChoice(false);
	const auto RevisedChoice = MakeArcPreviewChoice(true);
	const auto Policy = MakeArcPreviewChoicePolicy();
	const auto Basis = MakeArcPreviewBasis();
	const auto Initial = Session.TryUpdate(
		2, InitialChoice, Policy, 8, Basis,
		Fixture.Lifecycle, Fixture.Coordinator);
	const auto InitialState = Session.GetState();
	const auto Duplicate = Session.TryUpdate(
		2, InitialChoice, Policy, 8, Basis,
		Fixture.Lifecycle, Fixture.Coordinator);
	const auto Revised = Session.TryUpdate(
		2, RevisedChoice, Policy, 8, Basis,
		Fixture.Lifecycle, Fixture.Coordinator);
	TestTrue(TEXT("session preserves duplicate and strict revision semantics"),
		Initial.GetStatus() == ESession::Applied
			&& Duplicate.IsAccepted()
			&& Duplicate.IsNoChange()
			&& Duplicate.GetStatus() == ESession::NoChange
			&& Duplicate.GetState().Matches(InitialState)
			&& Revised.IsAccepted()
			&& Revised.DidChange()
			&& Revised.GetState().GetChoiceRevision()
				== InitialState.GetChoiceRevision() + 1
			&& Revised.GetState().GetPresentationStateId()
				!= InitialState.GetPresentationStateId()
			&& Session.GetState().Matches(Revised.GetState()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewPresentationSessionNoPreviewTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationSession.NoPreviewWithoutProduct",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewPresentationSessionNoPreviewTest::
	RunTest(const FString&)
{
	using ESession =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationSessionStatus;
	using EUpdate = Edemo_mapShanmenThrownWeaponArcPreviewUpdateStatus;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSession Session;
	FString Diagnostic;
	check(Session.TryBegin(FGuid::NewGuid(), Diagnostic));
	Fdemo_mapShanmenThrownWeaponProductLifecycle EmptyLifecycle;
	Fdemo_mapCombatRunCoordinator EmptyCoordinator;
	Fdemo_mapShanmenThrownWeaponArcChoicePolicy EmptyPolicy;
	Fdemo_mapShanmenThrownWeaponArcChoiceBasis EmptyBasis;
	const auto Result = Session.TryUpdate(
		0,
		Fdemo_mapShanmenThrownWeaponInputChoiceState::CreateInitial(),
		EmptyPolicy,
		0,
		EmptyBasis,
		EmptyLifecycle,
		EmptyCoordinator);
	TestTrue(TEXT("empty non-preview consumer needs no product source"),
		Result.IsValid()
			&& Result.IsAccepted()
			&& Result.IsNoChange()
			&& Result.GetStatus() == ESession::NoChange
			&& Result.GetCoordinatorCallCount() == 1
			&& Result.GetUpdate().GetStatus()
				== EUpdate::NoPresentationRequired
			&& Result.GetUpdate().GetCaptureCount() == 0
			&& Result.GetUpdate().GetProjectCount() == 0
			&& Result.GetUpdate().GetReduceCount() == 0
			&& Session.GetState().IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewPresentationSessionShutdownClearTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationSession.ClearAfterShutdown",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewPresentationSessionShutdownClearTest::
	RunTest(const FString&)
{
	using ESession =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationSessionStatus;
	using EUpdate = Edemo_mapShanmenThrownWeaponArcPreviewUpdateStatus;
	FThrownLifecycleFixture Fixture;
	if (!Fixture.Start(
			*this,
			TEXT("ArcPreviewPresentationSessionShutdownClear"),
			Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::BallisticArc))
	{
		return false;
	}
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSession Session;
	FString Diagnostic;
	check(Session.TryBegin(Fixture.Correlation.ActiveRunId, Diagnostic));
	const auto InitialChoice = MakeArcPreviewChoice(false);
	const auto Visible = Session.TryUpdate(
		2,
		InitialChoice,
		MakeArcPreviewChoicePolicy(),
		8,
		MakeArcPreviewBasis(),
		Fixture.Lifecycle,
		Fixture.Coordinator);
	const auto ClearChoice = ClearArcPreviewChoice(InitialChoice);
	Fixture.Stop();
	Fdemo_mapShanmenThrownWeaponArcChoicePolicy EmptyPolicy;
	Fdemo_mapShanmenThrownWeaponArcChoiceBasis EmptyBasis;
	const auto Cleared = Session.TryUpdate(
		0, ClearChoice, EmptyPolicy, 0, EmptyBasis,
		Fixture.Lifecycle, Fixture.Coordinator);
	const auto Duplicate = Session.TryUpdate(
		0, ClearChoice, EmptyPolicy, 0, EmptyBasis,
		Fixture.Lifecycle, Fixture.Coordinator);
	TestTrue(TEXT("session clears and deduplicates after product shutdown"),
		Visible.GetStatus() == ESession::Applied
			&& !Fixture.Lifecycle.IsActive()
			&& !Fixture.Coordinator.IsActive()
			&& Cleared.IsAccepted()
			&& Cleared.DidChange()
			&& Cleared.GetUpdate().GetStatus() == EUpdate::Cleared
			&& Cleared.GetUpdate().GetCaptureCount() == 0
			&& Cleared.GetUpdate().GetProjectCount() == 0
			&& Cleared.GetUpdate().GetReduceCount() == 1
			&& Session.GetState().IsHidden()
			&& Duplicate.IsNoChange()
			&& Duplicate.GetUpdate().GetStatus() == EUpdate::Duplicate
			&& Duplicate.GetState().Matches(Session.GetState()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewPresentationSessionAtomicRejectTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationSession.RejectionAtomicity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewPresentationSessionAtomicRejectTest::
	RunTest(const FString&)
{
	using ESession =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationSessionStatus;
	using EUpdate = Edemo_mapShanmenThrownWeaponArcPreviewUpdateStatus;
	FThrownLifecycleFixture Fixture;
	if (!Fixture.Start(
			*this,
			TEXT("ArcPreviewPresentationSessionAtomicReject"),
			Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::BallisticArc))
	{
		return false;
	}
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSession Session;
	FString Diagnostic;
	check(Session.TryBegin(Fixture.Correlation.ActiveRunId, Diagnostic));
	const auto Policy = MakeArcPreviewChoicePolicy();
	const auto Basis = MakeArcPreviewBasis();
	const auto Initial = Session.TryUpdate(
		2, MakeArcPreviewChoice(false), Policy, 8, Basis,
		Fixture.Lifecycle, Fixture.Coordinator);
	const auto StableState = Session.GetState();
	Fdemo_mapShanmenThrownWeaponInputChoiceState InvalidChoice;
	const auto Invalid = Session.TryUpdate(
		2, InvalidChoice, Policy, 8, Basis,
		Fixture.Lifecycle, Fixture.Coordinator);
	const auto BadCapture = Session.TryUpdate(
		0, MakeArcPreviewChoice(true), Policy, 8, Basis,
		Fixture.Lifecycle, Fixture.Coordinator);
	const auto Recovered = Session.TryUpdate(
		2, MakeArcPreviewChoice(true), Policy, 8, Basis,
		Fixture.Lifecycle, Fixture.Coordinator);
	TestTrue(TEXT("rejected updates cannot poison consumer-owned state"),
		Initial.IsAccepted()
			&& Invalid.IsValid()
			&& Invalid.GetStatus() == ESession::UpdateRejected
			&& Invalid.GetCoordinatorCallCount() == 1
			&& Invalid.GetUpdate().GetStatus() == EUpdate::ChoiceRejected
			&& Invalid.GetState().Matches(StableState)
			&& BadCapture.IsValid()
			&& BadCapture.GetStatus() == ESession::UpdateRejected
			&& BadCapture.GetUpdate().GetStatus() == EUpdate::CaptureRejected
			&& BadCapture.GetState().Matches(StableState)
			&& Recovered.IsAccepted()
			&& Recovered.DidChange()
			&& Session.GetState().Matches(Recovered.GetState()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewPresentationSessionRunFenceTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationSession.RunSourceFences",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewPresentationSessionRunFenceTest::RunTest(
	const FString&)
{
	using ESession =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationSessionStatus;
	FThrownLifecycleFixture FirstFixture;
	FThrownLifecycleFixture ForeignFixture;
	if (!FirstFixture.Start(
			*this,
			TEXT("ArcPreviewPresentationSessionRunFenceA"),
			Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::BallisticArc)
		|| !ForeignFixture.Start(
			*this,
			TEXT("ArcPreviewPresentationSessionRunFenceB"),
			Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::BallisticArc))
	{
		return false;
	}
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSession Session;
	FString Diagnostic;
	check(Session.TryBegin(FirstFixture.Correlation.ActiveRunId, Diagnostic));
	const auto Policy = MakeArcPreviewChoicePolicy();
	const auto Basis = MakeArcPreviewBasis();
	const auto Initial = Session.TryUpdate(
		2, MakeArcPreviewChoice(false), Policy, 8, Basis,
		FirstFixture.Lifecycle, FirstFixture.Coordinator);
	const auto StableState = Session.GetState();
	const auto Foreign = Session.TryUpdate(
		2, MakeArcPreviewChoice(true), Policy, 8, Basis,
		ForeignFixture.Lifecycle, ForeignFixture.Coordinator);
	FirstFixture.Stop();
	const auto Unavailable = Session.TryUpdate(
		2, MakeArcPreviewChoice(true), Policy, 8, Basis,
		FirstFixture.Lifecycle, FirstFixture.Coordinator);
	TestTrue(TEXT("visible path rejects foreign or unavailable Run sources"),
		Initial.IsAccepted()
			&& Foreign.IsValid()
			&& Foreign.GetStatus() == ESession::RunMismatch
			&& Foreign.GetCoordinatorCallCount() == 0
			&& Foreign.GetState().Matches(StableState)
			&& Unavailable.IsValid()
			&& Unavailable.GetStatus() == ESession::ProductUnavailable
			&& Unavailable.GetCoordinatorCallCount() == 0
			&& Unavailable.GetState().Matches(StableState)
			&& Session.GetState().Matches(StableState));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewPresentationSessionRotationTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationSession.EndResetRotation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewPresentationSessionRotationTest::RunTest(
	const FString&)
{
	using ESession =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationSessionStatus;
	FThrownLifecycleFixture FirstFixture;
	FThrownLifecycleFixture NextFixture;
	if (!FirstFixture.Start(
			*this,
			TEXT("ArcPreviewPresentationSessionRotationA"),
			Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::BallisticArc)
		|| !NextFixture.Start(
			*this,
			TEXT("ArcPreviewPresentationSessionRotationB"),
			Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::BallisticArc))
	{
		return false;
	}
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSession Session;
	FString Diagnostic;
	check(Session.TryBegin(FirstFixture.Correlation.ActiveRunId, Diagnostic));
	const auto First = Session.TryUpdate(
		2,
		MakeArcPreviewChoice(false),
		MakeArcPreviewChoicePolicy(),
		8,
		MakeArcPreviewBasis(),
		FirstFixture.Lifecycle,
		FirstFixture.Coordinator);
	const FGuid OldStateId = Session.GetState().GetPresentationStateId();
	TestFalse(TEXT("mismatched teardown preserves active presentation"),
		Session.TryEnd(NextFixture.Correlation.ActiveRunId, Diagnostic));
	TestTrue(TEXT("exact teardown drops every old Run presentation value"),
		First.IsAccepted()
			&& Session.GetState().GetPresentationStateId() == OldStateId
			&& Session.TryEnd(FirstFixture.Correlation.ActiveRunId, Diagnostic)
			&& Session.IsEmpty()
			&& Session.GetState().IsEmpty());
	check(Session.TryBegin(NextFixture.Correlation.ActiveRunId, Diagnostic));
	const auto Next = Session.TryUpdate(
		2,
		MakeArcPreviewChoice(false),
		MakeArcPreviewChoicePolicy(),
		8,
		MakeArcPreviewBasis(),
		NextFixture.Lifecycle,
		NextFixture.Coordinator);
	TestTrue(TEXT("new Run starts from empty state even at the same choice revision"),
		Next.IsAccepted()
			&& Next.GetStatus() == ESession::Applied
			&& Next.GetPreviousState().IsEmpty()
			&& Next.GetState().GetRunId()
				== NextFixture.Correlation.ActiveRunId
			&& Next.GetState().GetPresentationStateId() != OldStateId);
	Session.Reset();
	const auto Inactive = Session.TryUpdate(
		2,
		MakeArcPreviewChoice(false),
		MakeArcPreviewChoicePolicy(),
		8,
		MakeArcPreviewBasis(),
		NextFixture.Lifecycle,
		NextFixture.Coordinator);
	TestTrue(TEXT("explicit reset drops scope and blocks updates until begin"),
		Session.IsEmpty()
			&& Inactive.IsValid()
			&& Inactive.GetStatus() == ESession::SessionInactive
			&& Inactive.GetCoordinatorCallCount() == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewPresentationCommandRejectionTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationCommand.Rejection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewPresentationCommandRejectionTest::RunTest(
	const FString&)
{
	using EProject =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationCommandProjectStatus;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSessionResult InvalidSource;
	const auto Invalid =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandProjector::
			Project(InvalidSource);

	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSession InactiveSession;
	Fdemo_mapShanmenThrownWeaponProductLifecycle EmptyLifecycle;
	Fdemo_mapCombatRunCoordinator EmptyCoordinator;
	Fdemo_mapShanmenThrownWeaponArcChoicePolicy EmptyPolicy;
	Fdemo_mapShanmenThrownWeaponArcChoiceBasis EmptyBasis;
	const auto RejectedSession = InactiveSession.TryUpdate(
		0,
		Fdemo_mapShanmenThrownWeaponInputChoiceState::CreateInitial(),
		EmptyPolicy,
		0,
		EmptyBasis,
		EmptyLifecycle,
		EmptyCoordinator);
	const auto Rejected =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandProjector::
			Project(RejectedSession);
	TestTrue(TEXT("invalid and rejected Session results emit no command"),
		Invalid.IsValid()
			&& !Invalid.IsProjected()
			&& Invalid.GetStatus() == EProject::SessionResultInvalid
			&& !Invalid.GetCommand().IsValid()
			&& RejectedSession.IsValid()
			&& !RejectedSession.IsAccepted()
			&& Rejected.IsValid()
			&& !Rejected.IsProjected()
			&& Rejected.GetStatus() == EProject::SessionUpdateRejected
			&& !Rejected.GetCommand().IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewPresentationCommandShowTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationCommand.Show",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewPresentationCommandShowTest::RunTest(
	const FString&)
{
	using ECommand =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationCommandKind;
	FThrownLifecycleFixture Fixture;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSession Session;
	if (!StartArcPreviewPresentationSession(
			*this, TEXT("ArcPreviewPresentationCommandShow"), Fixture, Session))
	{
		return false;
	}
	const auto SessionResult = UpdateArcPreviewPresentationSession(
		Session, MakeArcPreviewChoice(false), Fixture);
	const auto Projected =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandProjector::
			Project(SessionResult);
	const auto Replay =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandProjector::
			Project(SessionResult);
	const auto& Command = Projected.GetCommand();
	TestTrue(TEXT("first visible state projects one deterministic Show"),
		SessionResult.IsAccepted()
			&& SessionResult.DidChange()
			&& Projected.IsProjected()
			&& Command.IsValid()
			&& Command.GetKind() == ECommand::Show
			&& Command.IsShow()
			&& Command.RequiresRenderMutation()
			&& Command.GetRunId() == Fixture.Correlation.ActiveRunId
			&& Command.GetPreviousState().IsEmpty()
			&& Command.GetState().IsVisible()
			&& Replay.IsProjected()
			&& Command.Matches(Replay.GetCommand())
			&& Command.GetCommandId()
				== Replay.GetCommand().GetCommandId());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewPresentationCommandReplaceTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationCommand.Replace",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewPresentationCommandReplaceTest::RunTest(
	const FString&)
{
	using ECommand =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationCommandKind;
	FThrownLifecycleFixture Fixture;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSession Session;
	if (!StartArcPreviewPresentationSession(
			*this, TEXT("ArcPreviewPresentationCommandReplace"), Fixture, Session))
	{
		return false;
	}
	const auto First = UpdateArcPreviewPresentationSession(
		Session, MakeArcPreviewChoice(false), Fixture);
	const auto Revised = UpdateArcPreviewPresentationSession(
		Session, MakeArcPreviewChoice(true), Fixture);
	const auto Projected =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandProjector::
			Project(Revised);
	const auto& Command = Projected.GetCommand();
	TestTrue(TEXT("newer visible geometry projects Replace"),
		First.IsAccepted()
			&& Revised.IsAccepted()
			&& Revised.DidChange()
			&& Projected.IsProjected()
			&& Command.GetKind() == ECommand::Replace
			&& Command.IsReplace()
			&& Command.RequiresRenderMutation()
			&& Command.GetPreviousState().IsVisible()
			&& Command.GetState().IsVisible()
			&& Command.GetState().GetChoiceRevision()
				> Command.GetPreviousState().GetChoiceRevision()
			&& Command.GetState().GetPresentationStateId()
				!= Command.GetPreviousState().GetPresentationStateId());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewPresentationCommandHideTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationCommand.Hide",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewPresentationCommandHideTest::RunTest(
	const FString&)
{
	using ECommand =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationCommandKind;
	FThrownLifecycleFixture Fixture;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSession Session;
	if (!StartArcPreviewPresentationSession(
			*this, TEXT("ArcPreviewPresentationCommandHide"), Fixture, Session))
	{
		return false;
	}
	const auto Choice = MakeArcPreviewChoice(false);
	const auto Visible =
		UpdateArcPreviewPresentationSession(Session, Choice, Fixture);
	const auto Cleared = UpdateArcPreviewPresentationSession(
		Session, ClearArcPreviewChoice(Choice), Fixture);
	const auto Projected =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandProjector::
			Project(Cleared);
	const auto& Command = Projected.GetCommand();
	TestTrue(TEXT("visible-to-hidden transition projects Hide"),
		Visible.IsAccepted()
			&& Cleared.IsAccepted()
			&& Cleared.DidChange()
			&& Projected.IsProjected()
			&& Command.GetKind() == ECommand::Hide
			&& Command.IsHide()
			&& Command.RequiresRenderMutation()
			&& Command.GetPreviousState().IsVisible()
			&& Command.GetState().IsHidden());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewPresentationCommandEmptyNoOpTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationCommand.EmptyNoOp",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewPresentationCommandEmptyNoOpTest::RunTest(
	const FString&)
{
	using ECommand =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationCommandKind;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSession Session;
	const FGuid RunId = FGuid::NewGuid();
	FString Diagnostic;
	check(Session.TryBegin(RunId, Diagnostic));
	Fdemo_mapShanmenThrownWeaponProductLifecycle EmptyLifecycle;
	Fdemo_mapCombatRunCoordinator EmptyCoordinator;
	Fdemo_mapShanmenThrownWeaponArcChoicePolicy EmptyPolicy;
	Fdemo_mapShanmenThrownWeaponArcChoiceBasis EmptyBasis;
	const auto SessionResult = Session.TryUpdate(
		0,
		Fdemo_mapShanmenThrownWeaponInputChoiceState::CreateInitial(),
		EmptyPolicy,
		0,
		EmptyBasis,
		EmptyLifecycle,
		EmptyCoordinator);
	const auto Projected =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandProjector::
			Project(SessionResult);
	const auto& Command = Projected.GetCommand();
	TestTrue(TEXT("accepted empty update projects an auditable NoOp"),
		SessionResult.IsAccepted()
			&& SessionResult.IsNoChange()
			&& Projected.IsProjected()
			&& Command.GetKind() == ECommand::NoOp
			&& Command.IsNoOp()
			&& !Command.RequiresRenderMutation()
			&& Command.GetRunId() == RunId
			&& Command.GetPreviousState().IsEmpty()
			&& Command.GetState().IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewPresentationCommandDuplicateNoOpTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationCommand.DuplicateNoOp",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewPresentationCommandDuplicateNoOpTest::
	RunTest(const FString&)
{
	using ECommand =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationCommandKind;
	FThrownLifecycleFixture Fixture;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSession Session;
	if (!StartArcPreviewPresentationSession(
			*this, TEXT("ArcPreviewPresentationCommandDuplicate"), Fixture, Session))
	{
		return false;
	}
	const auto Choice = MakeArcPreviewChoice(false);
	const auto First =
		UpdateArcPreviewPresentationSession(Session, Choice, Fixture);
	const auto Duplicate =
		UpdateArcPreviewPresentationSession(Session, Choice, Fixture);
	const auto Projected =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandProjector::
			Project(Duplicate);
	const auto& Command = Projected.GetCommand();
	TestTrue(TEXT("duplicate visible snapshot projects NoOp"),
		First.IsAccepted()
			&& Duplicate.IsAccepted()
			&& Duplicate.IsNoChange()
			&& Projected.IsProjected()
			&& Command.GetKind() == ECommand::NoOp
			&& Command.IsNoOp()
			&& !Command.RequiresRenderMutation()
			&& Command.GetPreviousState().IsVisible()
			&& Command.GetPreviousState().Matches(Command.GetState()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewPresentationCommandHiddenAdvanceTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationCommand.HiddenAdvanceNoOp",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewPresentationCommandHiddenAdvanceTest::
	RunTest(const FString&)
{
	using ECommand =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationCommandKind;
	FThrownLifecycleFixture Fixture;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSession Session;
	if (!StartArcPreviewPresentationSession(
			*this, TEXT("ArcPreviewPresentationCommandHidden"), Fixture, Session))
	{
		return false;
	}
	const auto VisibleChoice = MakeArcPreviewChoice(false);
	const auto Visible = UpdateArcPreviewPresentationSession(
		Session, VisibleChoice, Fixture);
	const auto ClearChoice = ClearArcPreviewChoice(VisibleChoice);
	const auto Hidden =
		UpdateArcPreviewPresentationSession(Session, ClearChoice, Fixture);
	const auto Advanced = UpdateArcPreviewPresentationSession(
		Session, MakeLaterNonPreviewChoice(ClearChoice), Fixture);
	const auto Projected =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandProjector::
			Project(Advanced);
	const auto& Command = Projected.GetCommand();
	TestTrue(TEXT("hidden tombstone revision advance is visual NoOp"),
		Visible.IsAccepted()
			&& Hidden.IsAccepted()
			&& Hidden.GetState().IsHidden()
			&& Advanced.IsAccepted()
			&& Advanced.DidChange()
			&& Projected.IsProjected()
			&& Command.GetKind() == ECommand::NoOp
			&& Command.IsNoOp()
			&& !Command.RequiresRenderMutation()
			&& Command.GetPreviousState().IsHidden()
			&& Command.GetState().IsHidden()
			&& Command.GetState().GetChoiceRevision()
				> Command.GetPreviousState().GetChoiceRevision()
			&& Command.GetState().GetPresentationStateId()
				!= Command.GetPreviousState().GetPresentationStateId());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewPresentationCommandRunIdentityTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationCommand.RunIdentity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewPresentationCommandRunIdentityTest::
	RunTest(const FString&)
{
	FThrownLifecycleFixture FirstFixture;
	FThrownLifecycleFixture OtherFixture;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSession FirstSession;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSession OtherSession;
	if (!StartArcPreviewPresentationSession(
			*this,
			TEXT("ArcPreviewPresentationCommandRunA"),
			FirstFixture,
			FirstSession)
		|| !StartArcPreviewPresentationSession(
			*this,
			TEXT("ArcPreviewPresentationCommandRunB"),
			OtherFixture,
			OtherSession))
	{
		return false;
	}
	const auto Choice = MakeArcPreviewChoice(false);
	const auto First =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandProjector::
			Project(UpdateArcPreviewPresentationSession(
				FirstSession, Choice, FirstFixture));
	const auto Other =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandProjector::
			Project(UpdateArcPreviewPresentationSession(
				OtherSession, Choice, OtherFixture));
	TestTrue(TEXT("equivalent geometry in different Runs has distinct commands"),
		First.IsProjected()
			&& Other.IsProjected()
			&& First.GetCommand().IsShow()
			&& Other.GetCommand().IsShow()
			&& First.GetCommand().GetRunId()
				!= Other.GetCommand().GetRunId()
			&& First.GetCommand().GetCommandId()
				!= Other.GetCommand().GetCommandId()
			&& !First.GetCommand().Matches(Other.GetCommand()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewCommandLedgerScopeTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationCommandLedger.ScopeAndTeardown",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewCommandLedgerScopeTest::RunTest(
	const FString&)
{
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandLedger Ledger;
	const FGuid FirstRun = FGuid::NewGuid();
	const FGuid OtherRun = FGuid::NewGuid();
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	const FName OtherConsumer(TEXT("Renderer.ArcPreview.Spectator.r1"));
	FString Diagnostic;
	TestTrue(TEXT("default command ledger is valid and empty"),
		Ledger.IsValid() && Ledger.IsEmpty() && !Ledger.IsActive());
	TestFalse(TEXT("invalid Run cannot bind the command ledger"),
		Ledger.TryBegin(FGuid(), Consumer, Diagnostic));
	TestFalse(TEXT("missing consumer cannot bind the command ledger"),
		Ledger.TryBegin(FirstRun, NAME_None, Diagnostic));
	TestTrue(TEXT("one exact Run and consumer bind an empty cursor"),
		Ledger.TryBegin(FirstRun, Consumer, Diagnostic)
			&& Ledger.IsValid() && Ledger.IsActive()
			&& Ledger.GetLedgerId().IsValid()
			&& Ledger.GetRunId() == FirstRun
			&& Ledger.GetConsumerDefinitionId() == Consumer
			&& Ledger.GetCursorState().IsEmpty()
			&& Ledger.NumAppliedCommands() == 0
			&& Ledger.NumRejectedCommands() == 0);
	TestTrue(TEXT("exact begin replay is idempotent"),
		Ledger.TryBegin(FirstRun, Consumer, Diagnostic));
	TestFalse(TEXT("active ledger rejects Run rotation"),
		Ledger.TryBegin(OtherRun, Consumer, Diagnostic));
	TestFalse(TEXT("active ledger rejects consumer rotation"),
		Ledger.TryBegin(FirstRun, OtherConsumer, Diagnostic));
	TestFalse(TEXT("foreign Run cannot end the ledger"),
		Ledger.TryEnd(OtherRun, Diagnostic));
	TestTrue(TEXT("exact Run end drops all consumer scope"),
		Ledger.TryEnd(FirstRun, Diagnostic)
			&& Ledger.IsEmpty() && Ledger.IsValid()
			&& !Ledger.GetLedgerId().IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewCommandReceiptTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationCommandLedger.ReceiptDeterminism",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewCommandReceiptTest::RunTest(
	const FString&)
{
	using EOutcome =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationCommandReceiptOutcome;
	using FReceipt =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandReceipt;
	FThrownLifecycleFixture Fixture;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSession Session;
	if (!StartArcPreviewPresentationSession(
			*this, TEXT("ArcPreviewCommandReceipt"), Fixture, Session))
	{
		return false;
	}
	const auto Projected =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandProjector::
			Project(UpdateArcPreviewPresentationSession(
				Session, MakeArcPreviewChoice(false), Fixture));
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	const FName AppliedCode(TEXT("Renderer.Applied"));
	FReceipt First;
	FReceipt Replay;
	FReceipt Rejected;
	FReceipt Invalid;
	FString Diagnostic;
	TestTrue(TEXT("same command outcome and consumer seal identically"),
		Projected.IsProjected()
			&& FReceipt::TryCreate(
				Projected.GetCommand(), Consumer, EOutcome::Applied,
				AppliedCode, First, Diagnostic)
			&& FReceipt::TryCreate(
				Projected.GetCommand(), Consumer, EOutcome::Applied,
				AppliedCode, Replay, Diagnostic)
			&& First.IsApplied() && First.Matches(Replay));
	TestTrue(TEXT("rejected outcome remains distinct immutable evidence"),
		FReceipt::TryCreate(
			Projected.GetCommand(), Consumer, EOutcome::Rejected,
			FName(TEXT("Renderer.PortUnavailable")), Rejected, Diagnostic)
			&& Rejected.IsRejected()
			&& Rejected.GetReceiptId() != First.GetReceiptId()
			&& !Rejected.Matches(First));
	TestFalse(TEXT("invalid command cannot produce a receipt"),
		FReceipt::TryCreate(
			Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommand(),
			Consumer, EOutcome::Applied, AppliedCode, Invalid, Diagnostic));
	TestFalse(TEXT("missing consumer cannot produce a receipt"),
		FReceipt::TryCreate(
			Projected.GetCommand(), NAME_None, EOutcome::Applied,
			AppliedCode, Invalid, Diagnostic));
	TestFalse(TEXT("missing stable outcome code cannot produce a receipt"),
		FReceipt::TryCreate(
			Projected.GetCommand(), Consumer, EOutcome::Applied,
			NAME_None, Invalid, Diagnostic));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewCommandLedgerShowTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationCommandLedger.ShowApplication",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewCommandLedgerShowTest::RunTest(
	const FString&)
{
	using EOutcome =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationCommandReceiptOutcome;
	using EStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationCommandLedgerStatus;
	using FReceipt =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandReceipt;
	FThrownLifecycleFixture Fixture;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSession Session;
	if (!StartArcPreviewPresentationSession(
			*this, TEXT("ArcPreviewCommandLedgerShow"), Fixture, Session))
	{
		return false;
	}
	const auto Projected =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandProjector::
			Project(UpdateArcPreviewPresentationSession(
				Session, MakeArcPreviewChoice(false), Fixture));
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FReceipt Receipt;
	FString Diagnostic;
	check(FReceipt::TryCreate(
		Projected.GetCommand(), Consumer, EOutcome::Applied,
		FName(TEXT("Renderer.Applied")), Receipt, Diagnostic));
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandLedger Ledger;
	check(Ledger.TryBegin(Fixture.Correlation.ActiveRunId, Consumer, Diagnostic));
	const auto Result = Ledger.Record(Receipt);
	FReceipt Stored;
	TestTrue(TEXT("Show application advances the consumer cursor exactly once"),
		Projected.GetCommand().IsShow()
			&& Result.IsValid() && Result.IsAccepted()
			&& Result.DidAdvanceCursor() && !Result.IsReplay()
			&& Result.GetStatus() == EStatus::ApplicationRecorded
			&& Result.GetPreviousAppliedCount() == 0
			&& Result.GetAppliedCount() == 1
			&& Result.GetPreviousCursorState().IsEmpty()
			&& Result.GetCursorState().Matches(
				Projected.GetCommand().GetState())
			&& Ledger.GetCursorState().Matches(
				Projected.GetCommand().GetState())
			&& Ledger.GetLastAppliedCommandId()
				== Projected.GetCommand().GetCommandId()
			&& Ledger.HasAppliedCommand(Receipt.GetCommandId())
			&& !Ledger.HasRejectedCommand(Receipt.GetCommandId())
			&& Ledger.TryGetAppliedReceipt(Receipt.GetCommandId(), Stored)
			&& Stored.Matches(Receipt));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewCommandLedgerRecoveryTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationCommandLedger.RejectionRecovery",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewCommandLedgerRecoveryTest::RunTest(
	const FString&)
{
	using EOutcome =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationCommandReceiptOutcome;
	using EStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationCommandLedgerStatus;
	using FReceipt =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandReceipt;
	FThrownLifecycleFixture Fixture;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSession Session;
	if (!StartArcPreviewPresentationSession(
			*this, TEXT("ArcPreviewCommandLedgerRecovery"), Fixture, Session))
	{
		return false;
	}
	const auto Projected =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandProjector::
			Project(UpdateArcPreviewPresentationSession(
				Session, MakeArcPreviewChoice(false), Fixture));
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FReceipt RejectedReceipt;
	FReceipt AppliedReceipt;
	FString Diagnostic;
	check(FReceipt::TryCreate(
		Projected.GetCommand(), Consumer, EOutcome::Rejected,
		FName(TEXT("Renderer.PortUnavailable")),
		RejectedReceipt, Diagnostic));
	check(FReceipt::TryCreate(
		Projected.GetCommand(), Consumer, EOutcome::Applied,
		FName(TEXT("Renderer.AppliedAfterRetry")),
		AppliedReceipt, Diagnostic));
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandLedger Ledger;
	check(Ledger.TryBegin(Fixture.Correlation.ActiveRunId, Consumer, Diagnostic));
	const auto Rejected = Ledger.Record(RejectedReceipt);
	const auto Replayed = Ledger.Record(RejectedReceipt);
	const auto Recovered = Ledger.Record(AppliedReceipt);
	const auto OldRejectionReplay = Ledger.Record(RejectedReceipt);
	FReceipt StoredRejected;
	FReceipt StoredApplied;
	TestTrue(TEXT("rejection is auditable but never advances the cursor"),
		Rejected.IsAccepted() && !Rejected.DidAdvanceCursor()
			&& Rejected.GetStatus() == EStatus::RejectionRecorded
			&& Rejected.GetAppliedCount() == 0
			&& Rejected.GetRejectedCount() == 1
			&& Rejected.GetCursorState().IsEmpty()
			&& Replayed.IsReplay()
			&& Replayed.GetStatus() == EStatus::RejectionReplayed);
	TestTrue(TEXT("later Applied evidence recovers and advances exactly once"),
		Recovered.IsAccepted() && Recovered.DidAdvanceCursor()
			&& Recovered.GetStatus() == EStatus::ApplicationRecovered
			&& Recovered.GetAppliedCount() == 1
			&& Recovered.GetRejectedCount() == 1
			&& Ledger.GetCursorState().Matches(
				Projected.GetCommand().GetState())
			&& Ledger.HasRejectedCommand(RejectedReceipt.GetCommandId())
			&& Ledger.HasAppliedCommand(AppliedReceipt.GetCommandId())
			&& Ledger.TryGetRejectedReceipt(
				RejectedReceipt.GetCommandId(), StoredRejected)
			&& Ledger.TryGetAppliedReceipt(
				AppliedReceipt.GetCommandId(), StoredApplied)
			&& StoredRejected.Matches(RejectedReceipt)
			&& StoredApplied.Matches(AppliedReceipt));
	TestTrue(TEXT("historical exact rejection remains idempotent after recovery"),
		OldRejectionReplay.IsAccepted()
			&& OldRejectionReplay.IsReplay()
			&& OldRejectionReplay.GetStatus()
				== EStatus::RejectionReplayed
			&& OldRejectionReplay.GetAppliedCount() == 1
			&& OldRejectionReplay.GetRejectedCount() == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewCommandLedgerReplayConflictTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationCommandLedger.ReplayAndConflict",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewCommandLedgerReplayConflictTest::RunTest(
	const FString&)
{
	using EOutcome =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationCommandReceiptOutcome;
	using EStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationCommandLedgerStatus;
	using FReceipt =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandReceipt;
	FThrownLifecycleFixture Fixture;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSession Session;
	if (!StartArcPreviewPresentationSession(
			*this, TEXT("ArcPreviewCommandLedgerReplay"), Fixture, Session))
	{
		return false;
	}
	const auto Projected =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandProjector::
			Project(UpdateArcPreviewPresentationSession(
				Session, MakeArcPreviewChoice(false), Fixture));
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FReceipt Applied;
	FReceipt AlternateApplied;
	FReceipt LateRejected;
	FString Diagnostic;
	check(FReceipt::TryCreate(
		Projected.GetCommand(), Consumer, EOutcome::Applied,
		FName(TEXT("Renderer.Applied")), Applied, Diagnostic));
	check(FReceipt::TryCreate(
		Projected.GetCommand(), Consumer, EOutcome::Applied,
		FName(TEXT("Renderer.AlternateApplied")),
		AlternateApplied, Diagnostic));
	check(FReceipt::TryCreate(
		Projected.GetCommand(), Consumer, EOutcome::Rejected,
		FName(TEXT("Renderer.LateFailure")), LateRejected, Diagnostic));
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandLedger Ledger;
	check(Ledger.TryBegin(Fixture.Correlation.ActiveRunId, Consumer, Diagnostic));
	const auto AppliedResult = Ledger.Record(Applied);
	const auto Replay = Ledger.Record(Applied);
	const auto Alternate = Ledger.Record(AlternateApplied);
	const auto RejectedAfterApply = Ledger.Record(LateRejected);
	TestTrue(TEXT("exact Applied replay never advances twice"),
		AppliedResult.DidAdvanceCursor()
			&& Replay.IsAccepted() && Replay.IsReplay()
			&& !Replay.DidAdvanceCursor()
			&& Replay.GetStatus() == EStatus::ApplicationReplayed
			&& Replay.GetAppliedCount() == 1);
	TestTrue(TEXT("conflicting evidence fails closed after application"),
		Alternate.IsValid() && !Alternate.IsAccepted()
			&& Alternate.GetStatus() == EStatus::ReceiptConflict
			&& RejectedAfterApply.IsValid()
			&& !RejectedAfterApply.IsAccepted()
			&& RejectedAfterApply.GetStatus() == EStatus::ReceiptConflict
			&& Ledger.NumAppliedCommands() == 1
			&& Ledger.NumRejectedCommands() == 0
			&& Ledger.GetCursorState().Matches(
				Projected.GetCommand().GetState()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewCommandLedgerOrderingTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationCommandLedger.OrderAndReplace",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewCommandLedgerOrderingTest::RunTest(
	const FString&)
{
	using EOutcome =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationCommandReceiptOutcome;
	using EStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationCommandLedgerStatus;
	using FReceipt =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandReceipt;
	FThrownLifecycleFixture Fixture;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSession Session;
	if (!StartArcPreviewPresentationSession(
			*this, TEXT("ArcPreviewCommandLedgerOrder"), Fixture, Session))
	{
		return false;
	}
	const auto Show =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandProjector::
			Project(UpdateArcPreviewPresentationSession(
				Session, MakeArcPreviewChoice(false), Fixture));
	const auto Replace =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandProjector::
			Project(UpdateArcPreviewPresentationSession(
				Session, MakeArcPreviewChoice(true), Fixture));
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FReceipt ShowReceipt;
	FReceipt ReplaceReceipt;
	FString Diagnostic;
	check(FReceipt::TryCreate(
		Show.GetCommand(), Consumer, EOutcome::Applied,
		FName(TEXT("Renderer.ShowApplied")), ShowReceipt, Diagnostic));
	check(FReceipt::TryCreate(
		Replace.GetCommand(), Consumer, EOutcome::Applied,
		FName(TEXT("Renderer.ReplaceApplied")),
		ReplaceReceipt, Diagnostic));
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandLedger Ledger;
	check(Ledger.TryBegin(Fixture.Correlation.ActiveRunId, Consumer, Diagnostic));
	const auto EarlyReplace = Ledger.Record(ReplaceReceipt);
	const auto ShowApplied = Ledger.Record(ShowReceipt);
	const auto ReplaceApplied = Ledger.Record(ReplaceReceipt);
	TestTrue(TEXT("future Replace cannot skip the current cursor"),
		Show.GetCommand().IsShow() && Replace.GetCommand().IsReplace()
			&& EarlyReplace.IsValid() && !EarlyReplace.IsAccepted()
			&& EarlyReplace.GetStatus() == EStatus::CursorMismatch
			&& EarlyReplace.GetAppliedCount() == 0
			&& EarlyReplace.GetCursorState().IsEmpty());
	TestTrue(TEXT("contiguous Show then Replace forms one applied chain"),
		ShowApplied.DidAdvanceCursor()
			&& ReplaceApplied.DidAdvanceCursor()
			&& ReplaceApplied.GetStatus() == EStatus::ApplicationRecorded
			&& Ledger.IsValid() && Ledger.NumAppliedCommands() == 2
			&& Ledger.GetLastAppliedCommandId()
				== Replace.GetCommand().GetCommandId()
			&& Ledger.GetCursorState().Matches(
				Replace.GetCommand().GetState()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewCommandLedgerHideNoOpTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationCommandLedger.HideAndNoOp",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewCommandLedgerHideNoOpTest::RunTest(
	const FString&)
{
	using EOutcome =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationCommandReceiptOutcome;
	using FReceipt =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandReceipt;
	FThrownLifecycleFixture Fixture;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSession Session;
	if (!StartArcPreviewPresentationSession(
			*this, TEXT("ArcPreviewCommandLedgerHideNoOp"), Fixture, Session))
	{
		return false;
	}
	const auto VisibleChoice = MakeArcPreviewChoice(false);
	const auto Show =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandProjector::
			Project(UpdateArcPreviewPresentationSession(
				Session, VisibleChoice, Fixture));
	const auto ClearChoice = ClearArcPreviewChoice(VisibleChoice);
	const auto Hide =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandProjector::
			Project(UpdateArcPreviewPresentationSession(
				Session, ClearChoice, Fixture));
	const auto NoOp =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandProjector::
			Project(UpdateArcPreviewPresentationSession(
				Session, MakeLaterNonPreviewChoice(ClearChoice), Fixture));
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FReceipt ShowReceipt;
	FReceipt HideReceipt;
	FReceipt NoOpReceipt;
	FString Diagnostic;
	check(FReceipt::TryCreate(
		Show.GetCommand(), Consumer, EOutcome::Applied,
		FName(TEXT("Renderer.ShowApplied")), ShowReceipt, Diagnostic));
	check(FReceipt::TryCreate(
		Hide.GetCommand(), Consumer, EOutcome::Applied,
		FName(TEXT("Renderer.HideApplied")), HideReceipt, Diagnostic));
	check(FReceipt::TryCreate(
		NoOp.GetCommand(), Consumer, EOutcome::Applied,
		FName(TEXT("Renderer.NoOpObserved")), NoOpReceipt, Diagnostic));
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandLedger Ledger;
	check(Ledger.TryBegin(Fixture.Correlation.ActiveRunId, Consumer, Diagnostic));
	const auto ShowApplied = Ledger.Record(ShowReceipt);
	const auto HideApplied = Ledger.Record(HideReceipt);
	const auto NoOpApplied = Ledger.Record(NoOpReceipt);
	TestTrue(TEXT("Show Hide and hidden NoOp advance one contiguous cursor"),
		Show.GetCommand().IsShow() && Hide.GetCommand().IsHide()
			&& NoOp.GetCommand().IsNoOp()
			&& !NoOp.GetCommand().RequiresRenderMutation()
			&& ShowApplied.DidAdvanceCursor()
			&& HideApplied.DidAdvanceCursor()
			&& NoOpApplied.DidAdvanceCursor()
			&& Ledger.NumAppliedCommands() == 3
			&& Ledger.GetLastAppliedCommandId()
				== NoOp.GetCommand().GetCommandId()
			&& Ledger.GetCursorState().IsHidden()
			&& Ledger.GetCursorState().Matches(
				NoOp.GetCommand().GetState()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewCommandLedgerRotationTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationCommandLedger.RunConsumerRotation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewCommandLedgerRotationTest::RunTest(
	const FString&)
{
	using EOutcome =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationCommandReceiptOutcome;
	using EStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationCommandLedgerStatus;
	using FReceipt =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandReceipt;
	FThrownLifecycleFixture FirstFixture;
	FThrownLifecycleFixture NextFixture;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSession FirstSession;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSession NextSession;
	if (!StartArcPreviewPresentationSession(
			*this, TEXT("ArcPreviewCommandLedgerRunA"),
			FirstFixture, FirstSession)
		|| !StartArcPreviewPresentationSession(
			*this, TEXT("ArcPreviewCommandLedgerRunB"),
			NextFixture, NextSession))
	{
		return false;
	}
	const auto First =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandProjector::
			Project(UpdateArcPreviewPresentationSession(
				FirstSession, MakeArcPreviewChoice(false), FirstFixture));
	const auto Next =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandProjector::
			Project(UpdateArcPreviewPresentationSession(
				NextSession, MakeArcPreviewChoice(false), NextFixture));
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	const FName OtherConsumer(TEXT("Renderer.ArcPreview.Spectator.r1"));
	FReceipt FirstReceipt;
	FReceipt WrongConsumerReceipt;
	FReceipt NextReceipt;
	FString Diagnostic;
	check(FReceipt::TryCreate(
		First.GetCommand(), Consumer, EOutcome::Applied,
		FName(TEXT("Renderer.Applied")), FirstReceipt, Diagnostic));
	check(FReceipt::TryCreate(
		First.GetCommand(), OtherConsumer, EOutcome::Applied,
		FName(TEXT("Renderer.Applied")),
		WrongConsumerReceipt, Diagnostic));
	check(FReceipt::TryCreate(
		Next.GetCommand(), Consumer, EOutcome::Applied,
		FName(TEXT("Renderer.Applied")), NextReceipt, Diagnostic));
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandLedger Ledger;
	check(Ledger.TryBegin(
		FirstFixture.Correlation.ActiveRunId, Consumer, Diagnostic));
	const auto WrongConsumer = Ledger.Record(WrongConsumerReceipt);
	const auto WrongRun = Ledger.Record(NextReceipt);
	const auto FirstApplied = Ledger.Record(FirstReceipt);
	TestTrue(TEXT("foreign consumer and Run receipts cannot mutate the ledger"),
		WrongConsumer.IsValid() && !WrongConsumer.IsAccepted()
			&& WrongConsumer.GetStatus() == EStatus::ConsumerMismatch
			&& WrongRun.IsValid() && !WrongRun.IsAccepted()
			&& WrongRun.GetStatus() == EStatus::RunMismatch
			&& FirstApplied.DidAdvanceCursor()
			&& Ledger.NumAppliedCommands() == 1);
	const FGuid OldCommandId = First.GetCommand().GetCommandId();
	TestTrue(TEXT("exact teardown permits an isolated next Run"),
		Ledger.TryEnd(
			FirstFixture.Correlation.ActiveRunId, Diagnostic)
			&& Ledger.TryBegin(
				NextFixture.Correlation.ActiveRunId, Consumer, Diagnostic)
			&& !Ledger.HasAppliedCommand(OldCommandId));
	const auto NextApplied = Ledger.Record(NextReceipt);
	TestTrue(TEXT("next Run starts from an empty independent cursor"),
		NextApplied.DidAdvanceCursor()
			&& NextApplied.GetStatus() == EStatus::ApplicationRecorded
			&& NextApplied.GetPreviousCursorState().IsEmpty()
			&& Ledger.NumAppliedCommands() == 1
			&& Ledger.GetRunId() == NextFixture.Correlation.ActiveRunId
			&& Ledger.GetLastAppliedCommandId()
				== Next.GetCommand().GetCommandId()
			&& OldCommandId != Next.GetCommand().GetCommandId());
	Ledger.Reset();
	TestTrue(TEXT("explicit reset drops all scope and cursor state"),
		Ledger.IsEmpty() && Ledger.IsValid());
	return true;
}

namespace
{
	class FFakeArcPreviewPresentationPort final
		: public Idemo_mapShanmenThrownWeaponArcPreviewPresentationPort
	{
	public:
		enum class EMode : uint8
		{
			Applied,
			Rejected,
			InvalidResponse,
			StaleResponse
		};

		explicit FFakeArcPreviewPresentationPort(
			const FName InConsumerDefinitionId,
			const EMode InMode = EMode::Applied)
			: ConsumerDefinitionId(InConsumerDefinitionId)
			, Mode(InMode)
		{
		}

		virtual FName GetConsumerDefinitionId() const override
		{
			return ConsumerDefinitionId;
		}

		virtual
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationPortResponse Apply(
			const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommand&
				Command) override
		{
			using EOutcome =
				Edemo_mapShanmenThrownWeaponArcPreviewPresentationPortResponseOutcome;
			using FResponse =
				Fdemo_mapShanmenThrownWeaponArcPreviewPresentationPortResponse;
			++ApplyCount;
			LastCommandId = Command.GetCommandId();
			if (Mode == EMode::InvalidResponse)
			{
				return FResponse();
			}

			const FGuid ResponseCommandId = Mode == EMode::StaleResponse
				? FGuid(
					0xF3800001, 0xF3800002, 0xF3800003, 0xF3800004)
				: Command.GetCommandId();
			const EOutcome Outcome = Mode == EMode::Rejected
				? EOutcome::Rejected
				: EOutcome::Applied;
			const FName OutcomeCode = Mode == EMode::Rejected
				? FName(TEXT("Renderer.ArcPreview.FakeRejected"))
				: FName(TEXT("Renderer.ArcPreview.FakeApplied"));
			FResponse Response;
			FString Diagnostic;
			check(FResponse::TryCreate(
				ResponseCommandId,
				Outcome,
				OutcomeCode,
				Response,
				Diagnostic));
			return Response;
		}

		void SetConsumerDefinitionId(const FName Value)
		{
			ConsumerDefinitionId = Value;
		}

		void SetMode(const EMode Value)
		{
			Mode = Value;
		}

		int32 ApplyCount = 0;
		FGuid LastCommandId;

	private:
		FName ConsumerDefinitionId = NAME_None;
		EMode Mode = EMode::Applied;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewPortResponseTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationDeliveryCoordinator.PortResponseDeterminism",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewPortResponseTest::RunTest(
	const FString&)
{
	using EOutcome =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationPortResponseOutcome;
	using FResponse =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationPortResponse;
	const FGuid CommandId(
		0xF3810001, 0xF3810002, 0xF3810003, 0xF3810004);
	const FName AppliedCode(TEXT("Renderer.ArcPreview.FakeApplied"));
	FResponse First;
	FResponse Replay;
	FResponse Rejected;
	FResponse Invalid;
	FString Diagnostic;
	TestTrue(TEXT("same port evidence produces one deterministic response"),
		FResponse::TryCreate(
			CommandId, EOutcome::Applied, AppliedCode, First, Diagnostic)
			&& FResponse::TryCreate(
				CommandId, EOutcome::Applied, AppliedCode, Replay, Diagnostic)
			&& First.IsApplied() && First.Matches(Replay));
	TestTrue(TEXT("Rejected is distinct self-validating response evidence"),
		FResponse::TryCreate(
			CommandId,
			EOutcome::Rejected,
			FName(TEXT("Renderer.ArcPreview.FakeRejected")),
			Rejected,
			Diagnostic)
			&& Rejected.IsRejected()
			&& Rejected.GetResponseId() != First.GetResponseId()
			&& !Rejected.Matches(First));
	TestFalse(TEXT("response rejects an invalid command identity"),
		FResponse::TryCreate(
			FGuid(), EOutcome::Applied, AppliedCode, Invalid, Diagnostic));
	TestFalse(TEXT("response rejects an invalid outcome"),
		FResponse::TryCreate(
			CommandId, EOutcome::Invalid, AppliedCode, Invalid, Diagnostic));
	TestFalse(TEXT("response rejects a missing stable outcome code"),
		FResponse::TryCreate(
			CommandId, EOutcome::Applied, NAME_None, Invalid, Diagnostic));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewDeliveryPreflightTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationDeliveryCoordinator.PreflightNoPortCall",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewDeliveryPreflightTest::RunTest(
	const FString&)
{
	using EStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryStatus;
	using FCoordinator =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryCoordinator;
	using FLedger =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandLedger;
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture Fixture;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSession Session;
	if (!StartArcPreviewPresentationSession(
			*this, TEXT("ArcPreviewDeliveryPreflight"), Fixture, Session))
	{
		return false;
	}
	const auto Show =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandProjector::
			Project(UpdateArcPreviewPresentationSession(
				Session, MakeArcPreviewChoice(false), Fixture));
	const auto Replace =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandProjector::
			Project(UpdateArcPreviewPresentationSession(
				Session, MakeArcPreviewChoice(true), Fixture));
	check(Show.IsProjected() && Replace.IsProjected());
	FFakeArcPreviewPresentationPort Port(Consumer);
	FLedger Ledger;
	const auto InvalidCommand = FCoordinator::Deliver(
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommand(),
		Port,
		Ledger);
	const auto Inactive = FCoordinator::Deliver(
		Show.GetCommand(), Port, Ledger);
	FString Diagnostic;
	check(Ledger.TryBegin(FGuid::NewGuid(), Consumer, Diagnostic));
	const auto WrongRun = FCoordinator::Deliver(
		Show.GetCommand(), Port, Ledger);
	Ledger.Reset();
	check(Ledger.TryBegin(
		Fixture.Correlation.ActiveRunId, Consumer, Diagnostic));
	Port.SetConsumerDefinitionId(NAME_None);
	const auto MissingConsumer = FCoordinator::Deliver(
		Show.GetCommand(), Port, Ledger);
	Port.SetConsumerDefinitionId(
		FName(TEXT("Renderer.ArcPreview.Spectator.r1")));
	const auto WrongConsumer = FCoordinator::Deliver(
		Show.GetCommand(), Port, Ledger);
	Port.SetConsumerDefinitionId(Consumer);
	const auto WrongCursor = FCoordinator::Deliver(
		Replace.GetCommand(), Port, Ledger);
	TestTrue(TEXT("all preflight failures are typed valid results"),
		InvalidCommand.IsValid()
			&& InvalidCommand.GetStatus() == EStatus::CommandInvalid
			&& Inactive.IsValid()
			&& Inactive.GetStatus() == EStatus::LedgerInactive
			&& WrongRun.IsValid()
			&& WrongRun.GetStatus() == EStatus::RunMismatch
			&& MissingConsumer.IsValid()
			&& MissingConsumer.GetStatus()
				== EStatus::PortConsumerInvalid
			&& WrongConsumer.IsValid()
			&& WrongConsumer.GetStatus() == EStatus::ConsumerMismatch
			&& WrongCursor.IsValid()
			&& WrongCursor.GetStatus() == EStatus::CursorMismatch);
	TestTrue(TEXT("preflight rejection never invokes the presentation port"),
		Port.ApplyCount == 0
			&& !InvalidCommand.DidCallPort()
			&& !Inactive.DidCallPort()
			&& !WrongRun.DidCallPort()
			&& !MissingConsumer.DidCallPort()
			&& !WrongConsumer.DidCallPort()
			&& !WrongCursor.DidCallPort()
			&& Ledger.NumAppliedCommands() == 0
			&& Ledger.NumRejectedCommands() == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewDeliveryAppliedTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationDeliveryCoordinator.AppliedExactlyOnce",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewDeliveryAppliedTest::RunTest(
	const FString&)
{
	using EStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryStatus;
	using FCoordinator =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryCoordinator;
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture Fixture;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSession Session;
	if (!StartArcPreviewPresentationSession(
			*this, TEXT("ArcPreviewDeliveryApplied"), Fixture, Session))
	{
		return false;
	}
	const auto Show =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandProjector::
			Project(UpdateArcPreviewPresentationSession(
				Session, MakeArcPreviewChoice(false), Fixture));
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandLedger Ledger;
	FString Diagnostic;
	check(Ledger.TryBegin(
		Fixture.Correlation.ActiveRunId, Consumer, Diagnostic));
	FFakeArcPreviewPresentationPort Port(Consumer);
	const auto First = FCoordinator::Deliver(
		Show.GetCommand(), Port, Ledger);
	const auto Replay = FCoordinator::Deliver(
		Show.GetCommand(), Port, Ledger);
	TestTrue(TEXT("new command is applied and committed through one port call"),
		First.IsValid() && First.IsAccepted() && First.WasApplied()
			&& First.DidCallPort() && !First.IsReplay()
			&& First.GetStatus() == EStatus::PortApplied
			&& First.GetPortCallCount() == 1
			&& First.GetPortResponse().IsApplied()
			&& First.GetReceipt().IsApplied()
			&& First.GetLedgerResult().DidAdvanceCursor()
			&& Ledger.NumAppliedCommands() == 1
			&& Ledger.GetCursorState().Matches(
				Show.GetCommand().GetState()));
	TestTrue(TEXT("applied command replay never invokes the port twice"),
		Replay.IsValid() && Replay.IsAccepted() && Replay.WasApplied()
			&& Replay.IsReplay() && !Replay.DidCallPort()
			&& Replay.GetStatus() == EStatus::ApplicationReplayed
			&& Port.ApplyCount == 1
			&& Port.LastCommandId == Show.GetCommand().GetCommandId()
			&& Ledger.NumAppliedCommands() == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewDeliveryRejectedTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationDeliveryCoordinator.RejectedExactlyOnce",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewDeliveryRejectedTest::RunTest(
	const FString&)
{
	using EStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryStatus;
	using FCoordinator =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryCoordinator;
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture Fixture;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSession Session;
	if (!StartArcPreviewPresentationSession(
			*this, TEXT("ArcPreviewDeliveryRejected"), Fixture, Session))
	{
		return false;
	}
	const auto Show =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandProjector::
			Project(UpdateArcPreviewPresentationSession(
				Session, MakeArcPreviewChoice(false), Fixture));
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandLedger Ledger;
	FString Diagnostic;
	check(Ledger.TryBegin(
		Fixture.Correlation.ActiveRunId, Consumer, Diagnostic));
	FFakeArcPreviewPresentationPort Port(
		Consumer, FFakeArcPreviewPresentationPort::EMode::Rejected);
	const auto First = FCoordinator::Deliver(
		Show.GetCommand(), Port, Ledger);
	const auto Replay = FCoordinator::Deliver(
		Show.GetCommand(), Port, Ledger);
	TestTrue(TEXT("port rejection is terminal evidence without cursor advance"),
		First.IsValid() && First.IsAccepted() && First.WasRejected()
			&& First.DidCallPort() && !First.WasApplied()
			&& First.GetStatus() == EStatus::PortRejected
			&& First.GetPortResponse().IsRejected()
			&& First.GetReceipt().IsRejected()
			&& !First.GetLedgerResult().DidAdvanceCursor()
			&& Ledger.NumAppliedCommands() == 0
			&& Ledger.NumRejectedCommands() == 1
			&& Ledger.GetCursorState().IsEmpty());
	TestTrue(TEXT("rejected command replay never retries the port"),
		Replay.IsValid() && Replay.IsAccepted() && Replay.WasRejected()
			&& Replay.IsReplay() && !Replay.DidCallPort()
			&& Replay.GetStatus() == EStatus::RejectionReplayed
			&& Port.ApplyCount == 1
			&& Ledger.NumRejectedCommands() == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewDeliveryInvalidResponseTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationDeliveryCoordinator.InvalidResponseFailsClosed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewDeliveryInvalidResponseTest::RunTest(
	const FString&)
{
	using EStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryStatus;
	using FCoordinator =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryCoordinator;
	using FLedger =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandLedger;
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture Fixture;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSession Session;
	if (!StartArcPreviewPresentationSession(
			*this, TEXT("ArcPreviewDeliveryInvalidResponse"), Fixture, Session))
	{
		return false;
	}
	const auto Show =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandProjector::
			Project(UpdateArcPreviewPresentationSession(
				Session, MakeArcPreviewChoice(false), Fixture));
	FString Diagnostic;
	FLedger InvalidLedger;
	check(InvalidLedger.TryBegin(
		Fixture.Correlation.ActiveRunId, Consumer, Diagnostic));
	FFakeArcPreviewPresentationPort InvalidPort(
		Consumer, FFakeArcPreviewPresentationPort::EMode::InvalidResponse);
	const auto Invalid = FCoordinator::Deliver(
		Show.GetCommand(), InvalidPort, InvalidLedger);
	const auto InvalidReplay = FCoordinator::Deliver(
		Show.GetCommand(), InvalidPort, InvalidLedger);
	FLedger StaleLedger;
	check(StaleLedger.TryBegin(
		Fixture.Correlation.ActiveRunId, Consumer, Diagnostic));
	FFakeArcPreviewPresentationPort StalePort(
		Consumer, FFakeArcPreviewPresentationPort::EMode::StaleResponse);
	const auto Stale = FCoordinator::Deliver(
		Show.GetCommand(), StalePort, StaleLedger);
	TestTrue(TEXT("missing response becomes one stable terminal rejection"),
		Invalid.IsValid() && Invalid.IsAccepted()
			&& Invalid.WasRejected() && Invalid.DidCallPort()
			&& Invalid.GetStatus() == EStatus::PortResponseRejected
			&& !Invalid.GetPortResponse().IsValid()
			&& Invalid.GetReceipt().IsRejected()
			&& InvalidLedger.NumRejectedCommands() == 1
			&& InvalidReplay.IsReplay()
			&& !InvalidReplay.DidCallPort()
			&& InvalidPort.ApplyCount == 1);
	TestTrue(TEXT("stale response identity also fails closed after one call"),
		Stale.IsValid() && Stale.IsAccepted()
			&& Stale.WasRejected() && Stale.DidCallPort()
			&& Stale.GetStatus() == EStatus::PortResponseRejected
			&& Stale.GetPortResponse().IsValid()
			&& Stale.GetPortResponse().GetCommandId()
				!= Show.GetCommand().GetCommandId()
			&& Stale.GetReceipt().IsRejected()
			&& StaleLedger.NumRejectedCommands() == 1
			&& StalePort.ApplyCount == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewDeliveryOrderingTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationDeliveryCoordinator.OrderedShowReplace",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewDeliveryOrderingTest::RunTest(
	const FString&)
{
	using EStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryStatus;
	using FCoordinator =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryCoordinator;
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture Fixture;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSession Session;
	if (!StartArcPreviewPresentationSession(
			*this, TEXT("ArcPreviewDeliveryOrdering"), Fixture, Session))
	{
		return false;
	}
	const auto Show =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandProjector::
			Project(UpdateArcPreviewPresentationSession(
				Session, MakeArcPreviewChoice(false), Fixture));
	const auto Replace =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandProjector::
			Project(UpdateArcPreviewPresentationSession(
				Session, MakeArcPreviewChoice(true), Fixture));
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandLedger Ledger;
	FString Diagnostic;
	check(Ledger.TryBegin(
		Fixture.Correlation.ActiveRunId, Consumer, Diagnostic));
	FFakeArcPreviewPresentationPort Port(Consumer);
	const auto EarlyReplace = FCoordinator::Deliver(
		Replace.GetCommand(), Port, Ledger);
	const auto AppliedShow = FCoordinator::Deliver(
		Show.GetCommand(), Port, Ledger);
	const auto AppliedReplace = FCoordinator::Deliver(
		Replace.GetCommand(), Port, Ledger);
	TestTrue(TEXT("future Replace is rejected before any port side effect"),
		EarlyReplace.IsValid()
			&& EarlyReplace.GetStatus() == EStatus::CursorMismatch
			&& !EarlyReplace.IsAccepted()
			&& !EarlyReplace.DidCallPort()
			&& Port.ApplyCount == 2);
	TestTrue(TEXT("contiguous Show then Replace each call the port once"),
		AppliedShow.WasApplied() && AppliedShow.DidCallPort()
			&& AppliedReplace.WasApplied()
			&& AppliedReplace.DidCallPort()
			&& AppliedReplace.GetStatus() == EStatus::PortApplied
			&& Ledger.NumAppliedCommands() == 2
			&& Ledger.GetCursorState().Matches(
				Replace.GetCommand().GetState()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewDeliveryHideNoOpTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationDeliveryCoordinator.HideAndNoOp",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewDeliveryHideNoOpTest::RunTest(
	const FString&)
{
	using FCoordinator =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryCoordinator;
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture Fixture;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSession Session;
	if (!StartArcPreviewPresentationSession(
			*this, TEXT("ArcPreviewDeliveryHideNoOp"), Fixture, Session))
	{
		return false;
	}
	const auto VisibleChoice = MakeArcPreviewChoice(false);
	const auto Show =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandProjector::
			Project(UpdateArcPreviewPresentationSession(
				Session, VisibleChoice, Fixture));
	const auto ClearChoice = ClearArcPreviewChoice(VisibleChoice);
	const auto Hide =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandProjector::
			Project(UpdateArcPreviewPresentationSession(
				Session, ClearChoice, Fixture));
	const auto NoOp =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandProjector::
			Project(UpdateArcPreviewPresentationSession(
				Session, MakeLaterNonPreviewChoice(ClearChoice), Fixture));
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandLedger Ledger;
	FString Diagnostic;
	check(Ledger.TryBegin(
		Fixture.Correlation.ActiveRunId, Consumer, Diagnostic));
	FFakeArcPreviewPresentationPort Port(Consumer);
	const auto AppliedShow = FCoordinator::Deliver(
		Show.GetCommand(), Port, Ledger);
	const auto AppliedHide = FCoordinator::Deliver(
		Hide.GetCommand(), Port, Ledger);
	const auto AppliedNoOp = FCoordinator::Deliver(
		NoOp.GetCommand(), Port, Ledger);
	TestTrue(TEXT("Show Hide and NoOp are all observed in exact order"),
		AppliedShow.WasApplied() && AppliedHide.WasApplied()
			&& AppliedNoOp.WasApplied()
			&& Show.GetCommand().RequiresRenderMutation()
			&& Hide.GetCommand().RequiresRenderMutation()
			&& !NoOp.GetCommand().RequiresRenderMutation()
			&& AppliedNoOp.DidCallPort()
			&& Port.ApplyCount == 3
			&& Ledger.NumAppliedCommands() == 3
			&& Ledger.GetLastAppliedCommandId()
				== NoOp.GetCommand().GetCommandId()
			&& Ledger.GetCursorState().Matches(
				NoOp.GetCommand().GetState()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewDeliveryRecoveryReplayTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationDeliveryCoordinator.ExternalRecoveryReplay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewDeliveryRecoveryReplayTest::RunTest(
	const FString&)
{
	using EDelivery =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryStatus;
	using ELedger =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationCommandLedgerStatus;
	using EReceipt =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationCommandReceiptOutcome;
	using FCoordinator =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryCoordinator;
	using FReceipt =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandReceipt;
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture Fixture;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSession Session;
	if (!StartArcPreviewPresentationSession(
			*this, TEXT("ArcPreviewDeliveryRecovery"), Fixture, Session))
	{
		return false;
	}
	const auto Show =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandProjector::
			Project(UpdateArcPreviewPresentationSession(
				Session, MakeArcPreviewChoice(false), Fixture));
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandLedger Ledger;
	FString Diagnostic;
	check(Ledger.TryBegin(
		Fixture.Correlation.ActiveRunId, Consumer, Diagnostic));
	FFakeArcPreviewPresentationPort RejectingPort(
		Consumer, FFakeArcPreviewPresentationPort::EMode::Rejected);
	const auto Rejected = FCoordinator::Deliver(
		Show.GetCommand(), RejectingPort, Ledger);
	FReceipt ExternalApplied;
	check(FReceipt::TryCreate(
		Show.GetCommand(),
		Consumer,
		EReceipt::Applied,
		FName(TEXT("Renderer.ArcPreview.ExternallyRecovered")),
		ExternalApplied,
		Diagnostic));
	const auto Recovered = Ledger.Record(ExternalApplied);
	FFakeArcPreviewPresentationPort UnusedPort(Consumer);
	const auto Replay = FCoordinator::Deliver(
		Show.GetCommand(), UnusedPort, Ledger);
	TestTrue(TEXT("direct P20.37 recovery remains compatible"),
		Rejected.WasRejected()
			&& Rejected.GetStatus() == EDelivery::PortRejected
			&& Recovered.IsAccepted() && Recovered.DidAdvanceCursor()
			&& Recovered.GetStatus() == ELedger::ApplicationRecovered
			&& Ledger.HasRejectedCommand(Show.GetCommand().GetCommandId())
			&& Ledger.HasAppliedCommand(Show.GetCommand().GetCommandId()));
	TestTrue(TEXT("coordinator sees recovered application and never retries port"),
		Replay.IsValid() && Replay.IsAccepted() && Replay.WasApplied()
			&& Replay.IsReplay() && !Replay.DidCallPort()
			&& Replay.GetStatus() == EDelivery::ApplicationReplayed
			&& UnusedPort.ApplyCount == 0
			&& RejectingPort.ApplyCount == 1
			&& Ledger.NumAppliedCommands() == 1
			&& Ledger.NumRejectedCommands() == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewDeliverySessionLifecycleTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationDeliverySession.Lifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewDeliverySessionLifecycleTest::RunTest(
	const FString&)
{
	using FDeliverySession =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliverySession;
	const FGuid RunId(
		0xF3900001, 0xF3900002, 0xF3900003, 0xF3900004);
	const FGuid NextRunId(
		0xF3900011, 0xF3900012, 0xF3900013, 0xF3900014);
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	const FName OtherConsumer(TEXT("Renderer.ArcPreview.Spectator.r1"));
	FDeliverySession Session;
	FString Diagnostic;
	TestFalse(TEXT("begin rejects a missing Run"),
		Session.TryBegin(FGuid(), Consumer, Diagnostic));
	TestFalse(TEXT("begin rejects a missing consumer"),
		Session.TryBegin(RunId, NAME_None, Diagnostic));
	TestTrue(TEXT("begin creates one private Run-scoped ledger"),
		Session.TryBegin(RunId, Consumer, Diagnostic)
			&& Session.IsValid() && Session.IsActive()
			&& Session.GetRunId() == RunId
			&& Session.GetConsumerDefinitionId() == Consumer
			&& Session.GetLedgerId().IsValid()
			&& Session.GetLedger().IsActive()
			&& Session.NumAppliedCommands() == 0
			&& Session.NumRejectedCommands() == 0
			&& Session.CanEnd());
	const FGuid FirstLedgerId = Session.GetLedgerId();
	TestTrue(TEXT("exact begin replay is idempotent"),
		Session.TryBegin(RunId, Consumer, Diagnostic)
			&& Session.GetLedgerId() == FirstLedgerId);
	TestFalse(TEXT("active Session rejects Run rotation"),
		Session.TryBegin(NextRunId, Consumer, Diagnostic));
	TestFalse(TEXT("active Session rejects consumer rotation"),
		Session.TryBegin(RunId, OtherConsumer, Diagnostic));
	TestFalse(TEXT("foreign Run cannot close the Session"),
		Session.TryEnd(NextRunId, Diagnostic));
	TestTrue(TEXT("empty cursor permits exact graceful end"),
		Session.TryEnd(RunId, Diagnostic)
			&& Session.IsEmpty() && Session.IsValid()
			&& !Session.GetLedgerId().IsValid());
	TestTrue(TEXT("a later Run receives an isolated new ledger"),
		Session.TryBegin(NextRunId, Consumer, Diagnostic)
			&& Session.GetLedgerId().IsValid()
			&& Session.GetLedgerId() != FirstLedgerId
			&& Session.GetCursorState().IsEmpty()
			&& Session.TryEnd(NextRunId, Diagnostic));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewDeliverySessionPreflightTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationDeliverySession.Preflight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewDeliverySessionPreflightTest::RunTest(
	const FString&)
{
	using ECoordinator =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryStatus;
	using ESession =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationDeliverySessionStatus;
	using FDeliverySession =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliverySession;
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture Fixture;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSession Presentation;
	if (!StartArcPreviewPresentationSession(
			*this, TEXT("ArcPreviewDeliverySessionPreflight"),
			Fixture, Presentation))
	{
		return false;
	}
	const auto Show =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandProjector::
			Project(UpdateArcPreviewPresentationSession(
				Presentation, MakeArcPreviewChoice(false), Fixture));
	const auto Replace =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandProjector::
			Project(UpdateArcPreviewPresentationSession(
				Presentation, MakeArcPreviewChoice(true), Fixture));
	check(Show.IsProjected() && Replace.IsProjected());
	FFakeArcPreviewPresentationPort Port(Consumer);
	FDeliverySession Session;
	const auto Inactive = Session.TryDeliver(Show.GetCommand(), Port);
	FString Diagnostic;
	check(Session.TryBegin(
		Fixture.Correlation.ActiveRunId, Consumer, Diagnostic));
	const auto Invalid = Session.TryDeliver(
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommand(), Port);

	FThrownLifecycleFixture ForeignFixture;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSession ForeignPresentation;
	if (!StartArcPreviewPresentationSession(
			*this, TEXT("ArcPreviewDeliverySessionForeignRun"),
			ForeignFixture, ForeignPresentation))
	{
		return false;
	}
	const auto ForeignShow =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandProjector::
			Project(UpdateArcPreviewPresentationSession(
				ForeignPresentation,
				MakeArcPreviewChoice(false),
				ForeignFixture));
	check(ForeignShow.IsProjected());
	const auto WrongRun = Session.TryDeliver(
		ForeignShow.GetCommand(), Port);
	Port.SetConsumerDefinitionId(
		FName(TEXT("Renderer.ArcPreview.Spectator.r1")));
	const auto WrongConsumer = Session.TryDeliver(Show.GetCommand(), Port);
	Port.SetConsumerDefinitionId(Consumer);
	const auto WrongCursor = Session.TryDeliver(Replace.GetCommand(), Port);
	TestTrue(TEXT("Session preflight is typed and coordinator-free"),
		Inactive.IsValid()
			&& Inactive.GetStatus() == ESession::SessionInactive
			&& !Inactive.DidCallCoordinator()
			&& Invalid.IsValid()
			&& Invalid.GetStatus() == ESession::CommandInvalid
			&& !Invalid.DidCallCoordinator()
			&& WrongRun.IsValid()
			&& WrongRun.GetStatus() == ESession::RunMismatch
			&& !WrongRun.DidCallCoordinator());
	TestTrue(TEXT("consumer and cursor checks delegate once but never call port"),
		WrongConsumer.IsValid()
			&& WrongConsumer.GetStatus() == ESession::DeliveryRejected
			&& WrongConsumer.DidCallCoordinator()
			&& WrongConsumer.GetDelivery().GetStatus()
				== ECoordinator::ConsumerMismatch
			&& WrongCursor.IsValid()
			&& WrongCursor.GetStatus() == ESession::DeliveryRejected
			&& WrongCursor.DidCallCoordinator()
			&& WrongCursor.GetDelivery().GetStatus()
				== ECoordinator::CursorMismatch
			&& Port.ApplyCount == 0
			&& Session.NumAppliedCommands() == 0
			&& Session.NumRejectedCommands() == 0
			&& Session.GetCursorState().IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewDeliverySessionOrderingTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationDeliverySession.OrderedReplay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewDeliverySessionOrderingTest::RunTest(
	const FString&)
{
	using ESession =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationDeliverySessionStatus;
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture Fixture;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSession Presentation;
	if (!StartArcPreviewPresentationSession(
			*this, TEXT("ArcPreviewDeliverySessionOrdering"),
			Fixture, Presentation))
	{
		return false;
	}
	const auto Show =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandProjector::
			Project(UpdateArcPreviewPresentationSession(
				Presentation, MakeArcPreviewChoice(false), Fixture));
	const auto Replace =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandProjector::
			Project(UpdateArcPreviewPresentationSession(
				Presentation, MakeArcPreviewChoice(true), Fixture));
	check(Show.IsProjected() && Replace.IsProjected());
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliverySession Session;
	FString Diagnostic;
	check(Session.TryBegin(
		Fixture.Correlation.ActiveRunId, Consumer, Diagnostic));
	FFakeArcPreviewPresentationPort Port(Consumer);
	const auto AppliedShow = Session.TryDeliver(Show.GetCommand(), Port);
	const auto ReplayedShow = Session.TryDeliver(Show.GetCommand(), Port);
	const auto AppliedReplace =
		Session.TryDeliver(Replace.GetCommand(), Port);
	TestTrue(TEXT("Show and Replace advance one private serialized cursor"),
		AppliedShow.IsValid() && AppliedShow.IsAccepted()
			&& AppliedShow.WasApplied() && !AppliedShow.IsReplay()
			&& AppliedShow.GetStatus() == ESession::Applied
			&& AppliedShow.DidCallCoordinator()
			&& AppliedShow.DidCallPort()
			&& AppliedShow.DidAdvanceCursor()
			&& AppliedReplace.IsValid() && AppliedReplace.WasApplied()
			&& AppliedReplace.GetStatus() == ESession::Applied
			&& Session.NumAppliedCommands() == 2
			&& Session.GetCursorState().Matches(
				Replace.GetCommand().GetState()));
	TestTrue(TEXT("exact replay delegates once without a second port call"),
		ReplayedShow.IsValid() && ReplayedShow.IsAccepted()
			&& ReplayedShow.WasApplied() && ReplayedShow.IsReplay()
			&& ReplayedShow.GetStatus() == ESession::ApplicationReplayed
			&& ReplayedShow.DidCallCoordinator()
			&& !ReplayedShow.DidCallPort()
			&& !ReplayedShow.DidAdvanceCursor()
			&& Port.ApplyCount == 2);
	TestFalse(TEXT("visible cursor cannot be discarded by graceful end"),
		Session.TryEnd(Fixture.Correlation.ActiveRunId, Diagnostic));
	TestTrue(TEXT("failed visible end preserves the active exact ledger"),
		Session.IsActive() && Session.IsValid()
			&& Session.GetCursorState().IsVisible()
			&& Session.NumAppliedCommands() == 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewDeliverySessionSafeEndTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationDeliverySession.HiddenEnd",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewDeliverySessionSafeEndTest::RunTest(
	const FString&)
{
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture Fixture;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSession Presentation;
	if (!StartArcPreviewPresentationSession(
			*this, TEXT("ArcPreviewDeliverySessionSafeEnd"),
			Fixture, Presentation))
	{
		return false;
	}
	const auto VisibleChoice = MakeArcPreviewChoice(false);
	const auto Show =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandProjector::
			Project(UpdateArcPreviewPresentationSession(
				Presentation, VisibleChoice, Fixture));
	const auto ClearChoice = ClearArcPreviewChoice(VisibleChoice);
	const auto Hide =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandProjector::
			Project(UpdateArcPreviewPresentationSession(
				Presentation, ClearChoice, Fixture));
	const auto NoOp =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandProjector::
			Project(UpdateArcPreviewPresentationSession(
				Presentation,
				MakeLaterNonPreviewChoice(ClearChoice),
				Fixture));
	check(Show.IsProjected() && Hide.IsProjected() && NoOp.IsProjected());
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliverySession Session;
	FString Diagnostic;
	check(Session.TryBegin(
		Fixture.Correlation.ActiveRunId, Consumer, Diagnostic));
	FFakeArcPreviewPresentationPort Port(Consumer);
	const auto AppliedShow = Session.TryDeliver(Show.GetCommand(), Port);
	const auto AppliedHide = Session.TryDeliver(Hide.GetCommand(), Port);
	const auto AppliedNoOp = Session.TryDeliver(NoOp.GetCommand(), Port);
	TestTrue(TEXT("Hide and hidden NoOp retain a safely closed cursor"),
		AppliedShow.WasApplied() && AppliedHide.WasApplied()
			&& AppliedNoOp.WasApplied()
			&& AppliedHide.DidAdvanceCursor()
			&& AppliedNoOp.DidAdvanceCursor()
			&& Session.GetCursorState().IsHidden()
			&& Session.NumAppliedCommands() == 3
			&& Port.ApplyCount == 3
			&& Session.CanEnd());
	TestTrue(TEXT("hidden cursor permits atomic ledger and Session teardown"),
		Session.TryEnd(Fixture.Correlation.ActiveRunId, Diagnostic)
			&& Session.IsEmpty() && Session.IsValid()
			&& Session.GetCursorState().IsEmpty()
			&& Session.NumAppliedCommands() == 0
			&& Session.NumRejectedCommands() == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewDeliverySessionRejectedHideTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationDeliverySession.RejectedHideBlocksEnd",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewDeliverySessionRejectedHideTest::RunTest(
	const FString&)
{
	using ESession =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationDeliverySessionStatus;
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture Fixture;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSession Presentation;
	if (!StartArcPreviewPresentationSession(
			*this, TEXT("ArcPreviewDeliverySessionRejectedHide"),
			Fixture, Presentation))
	{
		return false;
	}
	const auto VisibleChoice = MakeArcPreviewChoice(false);
	const auto Show =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandProjector::
			Project(UpdateArcPreviewPresentationSession(
				Presentation, VisibleChoice, Fixture));
	const auto Hide =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandProjector::
			Project(UpdateArcPreviewPresentationSession(
				Presentation,
				ClearArcPreviewChoice(VisibleChoice),
				Fixture));
	check(Show.IsProjected() && Hide.IsProjected());
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliverySession Session;
	FString Diagnostic;
	check(Session.TryBegin(
		Fixture.Correlation.ActiveRunId, Consumer, Diagnostic));
	FFakeArcPreviewPresentationPort Port(Consumer);
	const auto AppliedShow = Session.TryDeliver(Show.GetCommand(), Port);
	Port.SetMode(FFakeArcPreviewPresentationPort::EMode::Rejected);
	const auto RejectedHide = Session.TryDeliver(Hide.GetCommand(), Port);
	Port.SetMode(FFakeArcPreviewPresentationPort::EMode::Applied);
	const auto ReplayedRejection =
		Session.TryDeliver(Hide.GetCommand(), Port);
	TestTrue(TEXT("rejected Hide is terminal without clearing visible cursor"),
		AppliedShow.WasApplied()
			&& RejectedHide.IsValid() && RejectedHide.IsAccepted()
			&& RejectedHide.WasRejected() && !RejectedHide.IsReplay()
			&& RejectedHide.GetStatus() == ESession::Rejected
			&& !RejectedHide.DidAdvanceCursor()
			&& Session.GetCursorState().IsVisible()
			&& Session.NumAppliedCommands() == 1
			&& Session.NumRejectedCommands() == 1);
	TestTrue(TEXT("same rejected Hide never retries a newly healthy port"),
		ReplayedRejection.IsValid()
			&& ReplayedRejection.WasRejected()
			&& ReplayedRejection.IsReplay()
			&& ReplayedRejection.GetStatus()
				== ESession::RejectionReplayed
			&& !ReplayedRejection.DidCallPort()
			&& Port.ApplyCount == 2);
	TestFalse(TEXT("rejected Hide cannot authorize graceful teardown"),
		Session.TryEnd(Fixture.Correlation.ActiveRunId, Diagnostic));
	TestTrue(TEXT("blocked end preserves rejection evidence and visible cursor"),
		Session.IsValid() && Session.IsActive()
			&& Session.GetLedger().HasRejectedCommand(
				Hide.GetCommand().GetCommandId())
			&& !Session.CanEnd());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewDeliverySessionEmptyRejectionTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationDeliverySession.EmptyCursorRejectionEnd",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewDeliverySessionEmptyRejectionTest::RunTest(
	const FString&)
{
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture Fixture;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSession Presentation;
	if (!StartArcPreviewPresentationSession(
			*this, TEXT("ArcPreviewDeliverySessionEmptyRejection"),
			Fixture, Presentation))
	{
		return false;
	}
	const auto Show =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandProjector::
			Project(UpdateArcPreviewPresentationSession(
				Presentation, MakeArcPreviewChoice(false), Fixture));
	check(Show.IsProjected());
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliverySession Session;
	FString Diagnostic;
	check(Session.TryBegin(
		Fixture.Correlation.ActiveRunId, Consumer, Diagnostic));
	FFakeArcPreviewPresentationPort Port(
		Consumer, FFakeArcPreviewPresentationPort::EMode::InvalidResponse);
	const auto RejectedShow = Session.TryDeliver(Show.GetCommand(), Port);
	TestTrue(TEXT("invalid first Show response leaves consumer cursor empty"),
		RejectedShow.IsValid() && RejectedShow.IsAccepted()
			&& RejectedShow.WasRejected()
			&& RejectedShow.DidCallCoordinator()
			&& RejectedShow.DidCallPort()
			&& Session.GetCursorState().IsEmpty()
			&& Session.NumAppliedCommands() == 0
			&& Session.NumRejectedCommands() == 1
			&& Session.CanEnd());
	TestTrue(TEXT("empty cursor can close while dropping only audit memory"),
		Session.TryEnd(Fixture.Correlation.ActiveRunId, Diagnostic)
			&& Session.IsEmpty() && Session.IsValid()
			&& Port.ApplyCount == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewDeliverySessionRecoveryPreflightTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationDeliverySession.RecoveryPreflight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewDeliverySessionRecoveryPreflightTest::
	RunTest(const FString&)
{
	using EOutcome =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationCommandReceiptOutcome;
	using ERecovery =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryRecoveryStatus;
	using FReceipt =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandReceipt;
	using FSession =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliverySession;
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	const FName OtherConsumer(TEXT("Renderer.ArcPreview.Spectator.r1"));
	FThrownLifecycleFixture Fixture;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSession Presentation;
	if (!StartArcPreviewPresentationSession(
			*this, TEXT("ArcPreviewDeliveryRecoveryPreflight"),
			Fixture, Presentation))
	{
		return false;
	}
	const auto Show =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandProjector::
			Project(UpdateArcPreviewPresentationSession(
				Presentation, MakeArcPreviewChoice(false), Fixture));
	check(Show.IsProjected());

	FSession Session;
	const auto Inactive = Session.TryRecoverRejected(FReceipt());
	FString Diagnostic;
	check(Session.TryBegin(
		Fixture.Correlation.ActiveRunId, Consumer, Diagnostic));
	const auto Invalid = Session.TryRecoverRejected(FReceipt());
	FReceipt RejectedReceipt;
	FReceipt AppliedReceipt;
	FReceipt WrongConsumerReceipt;
	check(FReceipt::TryCreate(
		Show.GetCommand(), Consumer, EOutcome::Rejected,
		FName(TEXT("Renderer.ArcPreview.ExternalStillRejected")),
		RejectedReceipt, Diagnostic));
	check(FReceipt::TryCreate(
		Show.GetCommand(), Consumer, EOutcome::Applied,
		FName(TEXT("Renderer.ArcPreview.ExternalAppliedWithoutRejection")),
		AppliedReceipt, Diagnostic));
	check(FReceipt::TryCreate(
		Show.GetCommand(), OtherConsumer, EOutcome::Applied,
		FName(TEXT("Renderer.ArcPreview.ExternalWrongConsumer")),
		WrongConsumerReceipt, Diagnostic));
	const auto WrongOutcome = Session.TryRecoverRejected(RejectedReceipt);
	const auto MissingRejection = Session.TryRecoverRejected(AppliedReceipt);
	const auto WrongConsumer =
		Session.TryRecoverRejected(WrongConsumerReceipt);

	FThrownLifecycleFixture ForeignFixture;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSession
		ForeignPresentation;
	if (!StartArcPreviewPresentationSession(
			*this, TEXT("ArcPreviewDeliveryRecoveryForeignRun"),
			ForeignFixture, ForeignPresentation))
	{
		return false;
	}
	const auto ForeignShow =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandProjector::
			Project(UpdateArcPreviewPresentationSession(
				ForeignPresentation,
				MakeArcPreviewChoice(false),
				ForeignFixture));
	check(ForeignShow.IsProjected());
	FReceipt ForeignReceipt;
	check(FReceipt::TryCreate(
		ForeignShow.GetCommand(), Consumer, EOutcome::Applied,
		FName(TEXT("Renderer.ArcPreview.ExternalForeignRun")),
		ForeignReceipt, Diagnostic));
	const auto WrongRun = Session.TryRecoverRejected(ForeignReceipt);

	TestTrue(TEXT("recovery preflight rejects invalid scope and evidence"),
		Inactive.IsValid()
			&& Inactive.GetStatus() == ERecovery::SessionInactive
			&& !Inactive.DidRecordLedger()
			&& Invalid.IsValid()
			&& Invalid.GetStatus() == ERecovery::ReceiptInvalid
			&& WrongOutcome.IsValid()
			&& WrongOutcome.GetStatus()
				== ERecovery::AppliedReceiptRequired
			&& !WrongOutcome.DidRecordLedger());
	TestTrue(TEXT("recovery requires exact Run consumer and prior rejection"),
		MissingRejection.IsValid()
			&& MissingRejection.GetStatus()
				== ERecovery::RejectionNotFound
			&& WrongConsumer.IsValid()
			&& WrongConsumer.GetStatus() == ERecovery::ConsumerMismatch
			&& WrongRun.IsValid()
			&& WrongRun.GetStatus() == ERecovery::RunMismatch
			&& !MissingRejection.DidRecordLedger()
			&& !WrongConsumer.DidRecordLedger()
			&& !WrongRun.DidRecordLedger());
	TestTrue(TEXT("all recovery preflight failures preserve the private ledger"),
		Session.IsValid() && Session.IsActive()
			&& Session.GetCursorState().IsEmpty()
			&& Session.NumAppliedCommands() == 0
			&& Session.NumRejectedCommands() == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewDeliverySessionHideRecoveryTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationDeliverySession.RejectedHideRecovery",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewDeliverySessionHideRecoveryTest::RunTest(
	const FString&)
{
	using ELedger =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationCommandLedgerStatus;
	using EOutcome =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationCommandReceiptOutcome;
	using ERecovery =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryRecoveryStatus;
	using FReceipt =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandReceipt;
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture Fixture;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSession Presentation;
	if (!StartArcPreviewPresentationSession(
			*this, TEXT("ArcPreviewDeliveryHideRecovery"),
			Fixture, Presentation))
	{
		return false;
	}
	const auto VisibleChoice = MakeArcPreviewChoice(false);
	const auto Show =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandProjector::
			Project(UpdateArcPreviewPresentationSession(
				Presentation, VisibleChoice, Fixture));
	const auto Hide =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandProjector::
			Project(UpdateArcPreviewPresentationSession(
				Presentation,
				ClearArcPreviewChoice(VisibleChoice),
				Fixture));
	check(Show.IsProjected() && Hide.IsProjected());
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliverySession Session;
	FString Diagnostic;
	check(Session.TryBegin(
		Fixture.Correlation.ActiveRunId, Consumer, Diagnostic));
	FFakeArcPreviewPresentationPort Port(Consumer);
	const auto AppliedShow = Session.TryDeliver(Show.GetCommand(), Port);
	Port.SetMode(FFakeArcPreviewPresentationPort::EMode::Rejected);
	const auto RejectedHide = Session.TryDeliver(Hide.GetCommand(), Port);
	FReceipt ExternalApplied;
	check(FReceipt::TryCreate(
		Hide.GetCommand(), Consumer, EOutcome::Applied,
		FName(TEXT("Renderer.ArcPreview.ExternallyAttestedHideApplied")),
		ExternalApplied, Diagnostic));
	const auto Recovered = Session.TryRecoverRejected(ExternalApplied);
	const auto Replayed = Session.TryRecoverRejected(ExternalApplied);

	TestTrue(TEXT("external Applied evidence recovers only the rejected Hide"),
		AppliedShow.WasApplied() && RejectedHide.WasRejected()
			&& Recovered.IsValid() && Recovered.IsAccepted()
			&& Recovered.DidRecover() && !Recovered.IsReplay()
			&& Recovered.DidRecordLedger()
			&& Recovered.DidAdvanceCursor()
			&& Recovered.GetStatus() == ERecovery::Recovered
			&& Recovered.GetLedgerResult().GetStatus()
				== ELedger::ApplicationRecovered
			&& Recovered.GetReceipt().Matches(ExternalApplied)
			&& Recovered.GetPreviousCursorState().IsVisible()
			&& Recovered.GetCursorState().IsHidden()
			&& Recovered.GetPreviousAppliedCount() == 1
			&& Recovered.GetAppliedCount() == 2
			&& Recovered.GetRejectedCount() == 1);
	TestTrue(TEXT("exact external recovery replay is idempotent"),
		Replayed.IsValid() && Replayed.IsAccepted()
			&& !Replayed.DidRecover() && Replayed.IsReplay()
			&& Replayed.DidRecordLedger()
			&& !Replayed.DidAdvanceCursor()
			&& Replayed.GetStatus() == ERecovery::RecoveryReplayed
			&& Replayed.GetLedgerResult().GetStatus()
				== ELedger::ApplicationReplayed
			&& Replayed.GetPreviousAppliedCount() == 2
			&& Replayed.GetAppliedCount() == 2
			&& Port.ApplyCount == 2);
	TestTrue(TEXT("recovered Hidden cursor now permits graceful end"),
		Session.GetCursorState().IsHidden()
			&& Session.NumAppliedCommands() == 2
			&& Session.NumRejectedCommands() == 1
			&& Session.CanEnd()
			&& Session.TryEnd(
				Fixture.Correlation.ActiveRunId, Diagnostic)
			&& Session.IsEmpty() && Session.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewDeliverySessionRecoveryConflictTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationDeliverySession.RecoveryConflict",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewDeliverySessionRecoveryConflictTest::
	RunTest(const FString&)
{
	using ELedger =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationCommandLedgerStatus;
	using EOutcome =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationCommandReceiptOutcome;
	using ERecovery =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryRecoveryStatus;
	using FReceipt =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandReceipt;
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture Fixture;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSession Presentation;
	if (!StartArcPreviewPresentationSession(
			*this, TEXT("ArcPreviewDeliveryRecoveryConflict"),
			Fixture, Presentation))
	{
		return false;
	}
	const auto Show =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandProjector::
			Project(UpdateArcPreviewPresentationSession(
				Presentation, MakeArcPreviewChoice(false), Fixture));
	check(Show.IsProjected());
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliverySession Session;
	FString Diagnostic;
	check(Session.TryBegin(
		Fixture.Correlation.ActiveRunId, Consumer, Diagnostic));
	FFakeArcPreviewPresentationPort Port(
		Consumer, FFakeArcPreviewPresentationPort::EMode::Rejected);
	const auto Rejected = Session.TryDeliver(Show.GetCommand(), Port);
	FReceipt FirstApplied;
	FReceipt ConflictingApplied;
	check(FReceipt::TryCreate(
		Show.GetCommand(), Consumer, EOutcome::Applied,
		FName(TEXT("Renderer.ArcPreview.ExternalRecoveryPrimary")),
		FirstApplied, Diagnostic));
	check(FReceipt::TryCreate(
		Show.GetCommand(), Consumer, EOutcome::Applied,
		FName(TEXT("Renderer.ArcPreview.ExternalRecoveryConflict")),
		ConflictingApplied, Diagnostic));
	const auto Recovered = Session.TryRecoverRejected(FirstApplied);
	const auto Conflict = Session.TryRecoverRejected(ConflictingApplied);
	const auto Replay = Session.TryRecoverRejected(FirstApplied);

	TestTrue(TEXT("first exact recovery advances once"),
		Rejected.WasRejected() && Recovered.DidRecover()
			&& Session.GetCursorState().IsVisible());
	TestTrue(TEXT("different Applied evidence for the same command fails closed"),
		Conflict.IsValid() && !Conflict.IsAccepted()
			&& Conflict.DidRecordLedger()
			&& !Conflict.DidAdvanceCursor()
			&& Conflict.GetStatus() == ERecovery::LedgerRejected
			&& Conflict.GetLedgerResult().GetStatus()
				== ELedger::ReceiptConflict
			&& Conflict.GetPreviousAppliedCount() == 1
			&& Conflict.GetAppliedCount() == 1
			&& Conflict.GetRejectedCount() == 1);
	TestTrue(TEXT("conflict preserves canonical receipt and exact replay"),
		Replay.IsValid() && Replay.IsAccepted() && Replay.IsReplay()
			&& Session.GetLedger().HasRejectedCommand(
				Show.GetCommand().GetCommandId())
			&& Session.GetLedger().HasAppliedCommand(
				Show.GetCommand().GetCommandId())
			&& Session.NumAppliedCommands() == 1
			&& Session.NumRejectedCommands() == 1
			&& Port.ApplyCount == 1
			&& !Session.CanEnd());
	return true;
}

namespace
{
	class FReentrantArcPreviewPresentationPort final
		: public Idemo_mapShanmenThrownWeaponArcPreviewPresentationPort
	{
	public:
		FReentrantArcPreviewPresentationPort(
			const FName InConsumerDefinitionId,
			Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliverySession&
				InSession)
			: ConsumerDefinitionId(InConsumerDefinitionId)
			, Session(InSession)
		{
		}

		virtual FName GetConsumerDefinitionId() const override
		{
			return ConsumerDefinitionId;
		}

		virtual
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationPortResponse Apply(
			const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommand&
				Command) override
		{
			using EOutcome =
				Edemo_mapShanmenThrownWeaponArcPreviewPresentationPortResponseOutcome;
			using EReceiptOutcome =
				Edemo_mapShanmenThrownWeaponArcPreviewPresentationCommandReceiptOutcome;
			using FResponse =
				Fdemo_mapShanmenThrownWeaponArcPreviewPresentationPortResponse;
			using FReceipt =
				Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandReceipt;
			++ApplyCount;
			EndAccepted = Session.TryEnd(Command.GetRunId(), EndDiagnostic);
			ReentrantResult = Session.TryDeliver(Command, *this);
			FReceipt RecoveryReceipt;
			FString RecoveryDiagnostic;
			check(FReceipt::TryCreate(
				Command,
				ConsumerDefinitionId,
				EReceiptOutcome::Applied,
				FName(TEXT("Renderer.ArcPreview.ReentrantRecovery")),
				RecoveryReceipt,
				RecoveryDiagnostic));
			ReentrantRecoveryResult =
				Session.TryRecoverRejected(RecoveryReceipt);
			FResponse Response;
			FString Diagnostic;
			check(FResponse::TryCreate(
				Command.GetCommandId(),
				EOutcome::Applied,
				FName(TEXT("Renderer.ArcPreview.ReentrantOuterApplied")),
				Response,
				Diagnostic));
			return Response;
		}

		int32 ApplyCount = 0;
		bool EndAccepted = true;
		FString EndDiagnostic;
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliverySessionResult
			ReentrantResult;
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryRecoveryResult
			ReentrantRecoveryResult;

	private:
		FName ConsumerDefinitionId = NAME_None;
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliverySession&
			Session;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewDeliverySessionReentrantTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationDeliverySession.ReentrantPortBlocked",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewDeliverySessionReentrantTest::RunTest(
	const FString&)
{
	using ERecovery =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryRecoveryStatus;
	using ESession =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationDeliverySessionStatus;
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture Fixture;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSession Presentation;
	if (!StartArcPreviewPresentationSession(
			*this, TEXT("ArcPreviewDeliverySessionReentrant"),
			Fixture, Presentation))
	{
		return false;
	}
	const auto Show =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandProjector::
			Project(UpdateArcPreviewPresentationSession(
				Presentation, MakeArcPreviewChoice(false), Fixture));
	check(Show.IsProjected());
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliverySession Session;
	FString Diagnostic;
	check(Session.TryBegin(
		Fixture.Correlation.ActiveRunId, Consumer, Diagnostic));
	FReentrantArcPreviewPresentationPort Port(Consumer, Session);
	const auto Outer = Session.TryDeliver(Show.GetCommand(), Port);
	TestTrue(TEXT("outer delivery commits exactly once after guarded callback"),
		Outer.IsValid() && Outer.IsAccepted() && Outer.WasApplied()
			&& Outer.GetStatus() == ESession::Applied
			&& Outer.DidCallCoordinator() && Outer.DidCallPort()
			&& Port.ApplyCount == 1
			&& Session.NumAppliedCommands() == 1
			&& Session.NumRejectedCommands() == 0
			&& Session.GetCursorState().IsVisible());
	TestTrue(TEXT("re-entrant delivery is rejected before recursion"),
		Port.ReentrantResult.IsValid()
			&& !Port.ReentrantResult.IsAccepted()
			&& Port.ReentrantResult.GetStatus()
				== ESession::DeliveryInProgress
			&& !Port.ReentrantResult.DidCallCoordinator()
			&& !Port.ReentrantResult.DidCallPort());
	TestTrue(TEXT("port callback cannot close an in-flight Session"),
		!Port.EndAccepted && !Port.EndDiagnostic.IsEmpty()
			&& !Session.IsDeliveryInProgress()
			&& Session.IsValid());
	TestTrue(TEXT("port callback cannot inject external recovery evidence"),
		Port.ReentrantRecoveryResult.IsValid()
			&& !Port.ReentrantRecoveryResult.IsAccepted()
			&& Port.ReentrantRecoveryResult.GetStatus()
				== ERecovery::SessionBusy
			&& !Port.ReentrantRecoveryResult.DidRecordLedger());
	return true;
}

namespace
{
	bool StartArcPreviewDeliveryHost(
		FAutomationTestBase& Test,
		const TCHAR* Label,
		FThrownLifecycleFixture& Fixture,
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryHost& Host,
		const FName ConsumerDefinitionId)
	{
		if (!Fixture.Start(
				Test,
				Label,
				Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::
					BallisticArc))
		{
			return false;
		}
		FString Diagnostic;
		if (!Host.TryBegin(
				Fixture.Correlation.ActiveRunId,
				ConsumerDefinitionId,
				Diagnostic))
		{
			Test.AddError(Diagnostic);
			return false;
		}
		return true;
	}

	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryHostResult
	UpdateArcPreviewDeliveryHost(
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryHost& Host,
		const Fdemo_mapShanmenThrownWeaponInputChoiceState& Choice,
		const FThrownLifecycleFixture& Fixture,
		Idemo_mapShanmenThrownWeaponArcPreviewPresentationPort& Port)
	{
		return Host.TryUpdate(
			2,
			Choice,
			MakeArcPreviewChoicePolicy(),
			8,
			MakeArcPreviewBasis(),
			Fixture.Lifecycle,
			Fixture.Coordinator,
			Port);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewDeliveryHostLifecycleTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationDeliveryHost.Lifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewDeliveryHostLifecycleTest::RunTest(
	const FString&)
{
	using FHost =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryHost;
	const FGuid RunA(
		0xF4100001, 0xF4100002, 0xF4100003, 0xF4100004);
	const FGuid RunB(
		0xF4100011, 0xF4100012, 0xF4100013, 0xF4100014);
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FHost Host;
	FString Diagnostic;
	TestTrue(TEXT("default Host is valid and safely empty"),
		Host.IsValid() && Host.IsEmpty() && Host.IsSynchronized()
			&& Host.CanEnd()
			&& Host.TryEnd(RunA, Diagnostic));
	TestFalse(TEXT("begin rejects missing consumer identity"),
		Host.TryBegin(RunA, NAME_None, Diagnostic));
	TestTrue(TEXT("begin atomically binds both Sessions"),
		Host.TryBegin(RunA, Consumer, Diagnostic)
			&& Host.IsValid() && Host.IsActive()
			&& Host.GetRunId() == RunA
			&& Host.GetConsumerDefinitionId() == Consumer
			&& Host.GetPresentationSession().GetRunId() == RunA
			&& Host.GetDeliverySession().GetRunId() == RunA
			&& Host.IsSynchronized() && Host.CanEnd());
	TestTrue(TEXT("exact begin replay is idempotent"),
		Host.TryBegin(RunA, Consumer, Diagnostic));
	TestFalse(TEXT("active Host rejects Run rotation"),
		Host.TryBegin(RunB, Consumer, Diagnostic));
	TestTrue(TEXT("empty synchronized Host ends both Sessions"),
		Host.TryEnd(RunA, Diagnostic)
			&& Host.IsEmpty() && Host.IsValid()
			&& Host.GetPresentationSession().IsEmpty()
			&& Host.GetDeliverySession().IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewDeliveryHostOrderedTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationDeliveryHost.OrderedAppliedReplay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewDeliveryHostOrderedTest::RunTest(
	const FString&)
{
	using ECommand =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationCommandKind;
	using EHost =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryHostStatus;
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture Fixture;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryHost Host;
	if (!StartArcPreviewDeliveryHost(
			*this, TEXT("ArcPreviewDeliveryHostOrdered"),
			Fixture, Host, Consumer))
	{
		return false;
	}
	FFakeArcPreviewPresentationPort Port(Consumer);
	const auto InitialChoice = MakeArcPreviewChoice(false);
	const auto RevisedChoice = MakeArcPreviewChoice(true);
	const auto Show = UpdateArcPreviewDeliveryHost(
		Host, InitialChoice, Fixture, Port);
	const auto Replace = UpdateArcPreviewDeliveryHost(
		Host, RevisedChoice, Fixture, Port);
	const auto NoOp = UpdateArcPreviewDeliveryHost(
		Host, RevisedChoice, Fixture, Port);
	const auto Replay = UpdateArcPreviewDeliveryHost(
		Host, RevisedChoice, Fixture, Port);

	TestTrue(TEXT("Show and Replace each freeze one ordered call chain"),
		Show.IsValid() && Show.IsAccepted() && Show.WasApplied()
			&& Show.GetStatus() == EHost::Applied
			&& Show.GetStateUpdateCallCount() == 1
			&& Show.GetProjectionCallCount() == 1
			&& Show.GetDeliveryCallCount() == 1
			&& Show.GetProjection().GetCommand().GetKind()
				== ECommand::Show
			&& Replace.IsValid() && Replace.WasApplied()
			&& Replace.GetProjection().GetCommand().GetKind()
				== ECommand::Replace);
	TestTrue(TEXT("duplicate state emits and records one NoOp"),
		NoOp.IsValid() && NoOp.WasApplied() && !NoOp.IsReplay()
			&& NoOp.GetProjection().GetCommand().GetKind()
				== ECommand::NoOp
			&& Port.ApplyCount == 3);
	TestTrue(TEXT("exact NoOp replay never calls the port twice"),
		Replay.IsValid() && Replay.WasApplied() && Replay.IsReplay()
			&& Replay.GetStatus() == EHost::ApplicationReplayed
			&& !Replay.GetDelivery().DidCallPort()
			&& Port.ApplyCount == 3);
	TestTrue(TEXT("committed state and consumer cursor remain identical"),
		Host.IsValid() && Host.IsSynchronized() && !Host.NeedsRecovery()
			&& Host.GetState().IsVisible()
			&& Host.GetState().Matches(Host.GetCursorState())
			&& Host.GetDeliverySession().NumAppliedCommands() == 3);
	FString Diagnostic;
	TestFalse(TEXT("visible synchronized Host still cannot end"),
		Host.TryEnd(Fixture.Correlation.ActiveRunId, Diagnostic));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewDeliveryHostPreflightTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationDeliveryHost.PreflightAndStateReject",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewDeliveryHostPreflightTest::RunTest(
	const FString&)
{
	using EHost =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryHostStatus;
	using ERecovery =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryHostRecoveryStatus;
	const FGuid RunId(
		0xF4110001, 0xF4110002, 0xF4110003, 0xF4110004);
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryHost Host;
	FFakeArcPreviewPresentationPort Port(Consumer);
	Fdemo_mapShanmenThrownWeaponProductLifecycle Lifecycle;
	Fdemo_mapCombatRunCoordinator Coordinator;
	const auto Choice = MakeArcPreviewChoice(false);
	const auto Inactive = Host.TryUpdate(
		2, Choice, MakeArcPreviewChoicePolicy(), 8, MakeArcPreviewBasis(),
		Lifecycle, Coordinator, Port);
	TestTrue(TEXT("inactive Host rejects before every delegated call"),
		Inactive.IsValid() && !Inactive.IsAccepted()
			&& Inactive.GetStatus() == EHost::HostInactive
			&& Inactive.GetStateUpdateCallCount() == 0
			&& Inactive.GetProjectionCallCount() == 0
			&& Inactive.GetDeliveryCallCount() == 0
			&& Port.ApplyCount == 0);

	FString Diagnostic;
	check(Host.TryBegin(RunId, Consumer, Diagnostic));
	const auto ProductRejected = Host.TryUpdate(
		2, Choice, MakeArcPreviewChoicePolicy(), 8, MakeArcPreviewBasis(),
		Lifecycle, Coordinator, Port);
	TestTrue(TEXT("unavailable product rejects after state update only"),
		ProductRejected.IsValid() && !ProductRejected.IsAccepted()
			&& ProductRejected.GetStatus() == EHost::StateUpdateRejected
			&& ProductRejected.GetStateUpdateCallCount() == 1
			&& ProductRejected.GetProjectionCallCount() == 0
			&& ProductRejected.GetDeliveryCallCount() == 0
			&& Host.IsSynchronized() && Host.GetState().IsEmpty()
			&& Port.ApplyCount == 0);
	const auto NoRecovery = Host.TryRecoverRejected(
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandReceipt());
	TestTrue(TEXT("Host without rejection has no recovery side effect"),
		NoRecovery.IsValid() && !NoRecovery.IsAccepted()
			&& NoRecovery.GetStatus() == ERecovery::NoRecoveryPending
			&& NoRecovery.GetDeliveryRecoveryCallCount() == 0);
	TestTrue(TEXT("preflight-only Host remains safely endable"),
		Host.TryEnd(RunId, Diagnostic) && Host.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewDeliveryHostAtomicRejectTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationDeliveryHost.AtomicDeliveryReject",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewDeliveryHostAtomicRejectTest::RunTest(
	const FString&)
{
	using EHost =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryHostStatus;
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture Fixture;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryHost Host;
	if (!StartArcPreviewDeliveryHost(
			*this, TEXT("ArcPreviewDeliveryHostAtomicReject"),
			Fixture, Host, Consumer))
	{
		return false;
	}
	const auto Choice = MakeArcPreviewChoice(false);
	FFakeArcPreviewPresentationPort ForeignPort(
		FName(TEXT("Renderer.ArcPreview.Foreign.r1")));
	const auto Rejected = UpdateArcPreviewDeliveryHost(
		Host, Choice, Fixture, ForeignPort);
	TestTrue(TEXT("consumer mismatch rejects after one bounded composition"),
		Rejected.IsValid() && !Rejected.IsAccepted()
			&& Rejected.GetStatus() == EHost::DeliveryRejected
			&& Rejected.GetStateUpdateCallCount() == 1
			&& Rejected.GetProjectionCallCount() == 1
			&& Rejected.GetDeliveryCallCount() == 1
			&& !Rejected.GetDelivery().DidCallPort()
			&& ForeignPort.ApplyCount == 0);
	TestTrue(TEXT("rejected candidate leaves both live Sessions untouched"),
		Host.IsValid() && Host.IsSynchronized()
			&& Host.GetState().IsEmpty()
			&& Host.GetCursorState().IsEmpty()
			&& !Host.NeedsRecovery()
			&& Host.GetDeliverySession().NumAppliedCommands() == 0
			&& Host.GetDeliverySession().NumRejectedCommands() == 0);
	FFakeArcPreviewPresentationPort HealthyPort(Consumer);
	const auto Applied = UpdateArcPreviewDeliveryHost(
		Host, Choice, Fixture, HealthyPort);
	TestTrue(TEXT("same update can succeed after atomic preflight rollback"),
		Applied.IsValid() && Applied.WasApplied()
			&& Applied.GetStatus() == EHost::Applied
			&& Host.IsSynchronized() && Host.GetState().IsVisible()
			&& HealthyPort.ApplyCount == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewDeliveryHostRecoveryTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationDeliveryHost.RejectionRecoveryFence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewDeliveryHostRecoveryTest::RunTest(
	const FString&)
{
	using EHost =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryHostStatus;
	using EOutcome =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationCommandReceiptOutcome;
	using ERecovery =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryHostRecoveryStatus;
	using FReceipt =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandReceipt;
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture Fixture;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryHost Host;
	if (!StartArcPreviewDeliveryHost(
			*this, TEXT("ArcPreviewDeliveryHostRecovery"),
			Fixture, Host, Consumer))
	{
		return false;
	}
	FFakeArcPreviewPresentationPort Port(
		Consumer, FFakeArcPreviewPresentationPort::EMode::Rejected);
	const auto InitialChoice = MakeArcPreviewChoice(false);
	const auto Rejected = UpdateArcPreviewDeliveryHost(
		Host, InitialChoice, Fixture, Port);
	TestTrue(TEXT("port rejection commits exact pending recovery evidence"),
		Rejected.IsValid() && Rejected.IsAccepted()
			&& Rejected.WasRejected() && Rejected.NeedsRecovery()
			&& Rejected.GetStatus() == EHost::RejectedPendingRecovery
			&& Host.IsValid() && Host.NeedsRecovery()
			&& Host.GetState().IsVisible()
			&& Host.GetCursorState().IsEmpty()
			&& Host.GetDeliverySession().NumRejectedCommands() == 1);
	const auto Blocked = UpdateArcPreviewDeliveryHost(
		Host, MakeArcPreviewChoice(true), Fixture, Port);
	TestTrue(TEXT("pending rejection fences every newer update before calls"),
		Blocked.IsValid() && !Blocked.IsAccepted()
			&& Blocked.NeedsRecovery()
			&& Blocked.GetStatus() == EHost::RecoveryRequired
			&& Blocked.GetStateUpdateCallCount() == 0
			&& Blocked.GetProjectionCallCount() == 0
			&& Blocked.GetDeliveryCallCount() == 0
			&& Port.ApplyCount == 1);

	FReceipt AppliedReceipt;
	FReceipt ForeignReceipt;
	FString Diagnostic;
	check(FReceipt::TryCreate(
		Host.GetPendingRejectedCommand(),
		FName(TEXT("Renderer.ArcPreview.Foreign.r1")),
		EOutcome::Applied,
		FName(TEXT("Renderer.ArcPreview.ForeignRecovery")),
		ForeignReceipt,
		Diagnostic));
	const auto Mismatch = Host.TryRecoverRejected(ForeignReceipt);
	TestTrue(TEXT("foreign recovery evidence is fenced before ledger access"),
		Mismatch.IsValid() && !Mismatch.IsAccepted()
			&& Mismatch.GetStatus() == ERecovery::ReceiptMismatch
			&& Mismatch.GetDeliveryRecoveryCallCount() == 0
			&& Host.NeedsRecovery());
	check(FReceipt::TryCreate(
		Host.GetPendingRejectedCommand(),
		Consumer,
		EOutcome::Applied,
		FName(TEXT("Renderer.ArcPreview.ExternalRecovery")),
		AppliedReceipt,
		Diagnostic));
	const auto Recovered = Host.TryRecoverRejected(AppliedReceipt);
	const auto Replayed = Host.TryRecoverRejected(AppliedReceipt);
	TestTrue(TEXT("exact Applied evidence reconciles desired state and cursor"),
		Recovered.IsValid() && Recovered.IsAccepted()
			&& Recovered.DidRecover()
			&& Recovered.GetStatus() == ERecovery::Recovered
			&& Recovered.GetDeliveryRecoveryCallCount() == 1
			&& Host.IsValid() && Host.IsSynchronized()
			&& !Host.NeedsRecovery()
			&& Host.GetState().Matches(Host.GetCursorState()));
	TestTrue(TEXT("exact recovery replay is idempotent and port-free"),
		Replayed.IsValid() && Replayed.IsAccepted()
			&& Replayed.IsReplay()
			&& Replayed.GetStatus() == ERecovery::RecoveryReplayed
			&& Port.ApplyCount == 1);
	Port.SetMode(FFakeArcPreviewPresentationPort::EMode::Applied);
	const auto Replace = UpdateArcPreviewDeliveryHost(
		Host, MakeArcPreviewChoice(true), Fixture, Port);
	TestTrue(TEXT("newer update resumes only after cursor reconciliation"),
		Replace.IsValid() && Replace.WasApplied()
			&& Replace.GetStatus() == EHost::Applied
			&& Host.IsSynchronized() && Port.ApplyCount == 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewDeliveryHostHideRecoveryTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationDeliveryHost.HideRecoveryEnd",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewDeliveryHostHideRecoveryTest::RunTest(
	const FString&)
{
	using ECommand =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationCommandKind;
	using EOutcome =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationCommandReceiptOutcome;
	using FReceipt =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandReceipt;
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture Fixture;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryHost Host;
	if (!StartArcPreviewDeliveryHost(
			*this, TEXT("ArcPreviewDeliveryHostHideRecovery"),
			Fixture, Host, Consumer))
	{
		return false;
	}
	FFakeArcPreviewPresentationPort Port(Consumer);
	const auto VisibleChoice = MakeArcPreviewChoice(false);
	const auto Show = UpdateArcPreviewDeliveryHost(
		Host, VisibleChoice, Fixture, Port);
	Port.SetMode(FFakeArcPreviewPresentationPort::EMode::Rejected);
	const auto RejectedHide = UpdateArcPreviewDeliveryHost(
		Host, ClearArcPreviewChoice(VisibleChoice), Fixture, Port);
	FString Diagnostic;
	TestTrue(TEXT("rejected Hide keeps visible cursor and hidden desired state"),
		Show.WasApplied() && RejectedHide.WasRejected()
			&& Host.GetPendingRejectedCommand().GetKind()
				== ECommand::Hide
			&& Host.GetState().IsHidden()
			&& Host.GetCursorState().IsVisible()
			&& !Host.CanEnd());
	TestFalse(TEXT("unreconciled Hide cannot discard renderer state"),
		Host.TryEnd(Fixture.Correlation.ActiveRunId, Diagnostic));

	FReceipt AppliedHide;
	check(FReceipt::TryCreate(
		Host.GetPendingRejectedCommand(),
		Consumer,
		EOutcome::Applied,
		FName(TEXT("Renderer.ArcPreview.ExternalHideRecovery")),
		AppliedHide,
		Diagnostic));
	const auto Recovered = Host.TryRecoverRejected(AppliedHide);
	TestTrue(TEXT("recovered Hide produces one synchronized hidden cursor"),
		Recovered.IsValid() && Recovered.DidRecover()
			&& Host.IsSynchronized() && Host.GetState().IsHidden()
			&& Host.GetCursorState().IsHidden() && Host.CanEnd());
	TestTrue(TEXT("hidden synchronized Host atomically ends both Sessions"),
		Host.TryEnd(Fixture.Correlation.ActiveRunId, Diagnostic)
			&& Host.IsEmpty() && Host.IsValid()
			&& Host.GetState().IsEmpty()
			&& Host.GetCursorState().IsEmpty());
	return true;
}

namespace
{
	class FReentrantArcPreviewPresentationHostPort final
		: public Idemo_mapShanmenThrownWeaponArcPreviewPresentationPort
	{
	public:
		FReentrantArcPreviewPresentationHostPort(
			const FName InConsumerDefinitionId,
			Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryHost&
				InHost,
			const FThrownLifecycleFixture& InFixture)
			: ConsumerDefinitionId(InConsumerDefinitionId)
			, Host(InHost)
			, Fixture(InFixture)
		{
		}

		virtual FName GetConsumerDefinitionId() const override
		{
			return ConsumerDefinitionId;
		}

		virtual
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationPortResponse Apply(
			const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommand&
				Command) override
		{
			using EOutcome =
				Edemo_mapShanmenThrownWeaponArcPreviewPresentationPortResponseOutcome;
			using FResponse =
				Fdemo_mapShanmenThrownWeaponArcPreviewPresentationPortResponse;
			++ApplyCount;
			EndAccepted = Host.TryEnd(Command.GetRunId(), EndDiagnostic);
			ReentrantResult = UpdateArcPreviewDeliveryHost(
				Host, MakeArcPreviewChoice(true), Fixture, *this);
			FResponse Response;
			FString Diagnostic;
			check(FResponse::TryCreate(
				Command.GetCommandId(),
				EOutcome::Applied,
				FName(TEXT("Renderer.ArcPreview.ReentrantHostApplied")),
				Response,
				Diagnostic));
			return Response;
		}

		int32 ApplyCount = 0;
		bool EndAccepted = true;
		FString EndDiagnostic;
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryHostResult
			ReentrantResult;

	private:
		FName ConsumerDefinitionId = NAME_None;
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryHost& Host;
		const FThrownLifecycleFixture& Fixture;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewDeliveryHostReentrantTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationDeliveryHost.ReentrantPortBlocked",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewDeliveryHostReentrantTest::RunTest(
	const FString&)
{
	using EHost =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryHostStatus;
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture Fixture;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryHost Host;
	if (!StartArcPreviewDeliveryHost(
			*this, TEXT("ArcPreviewDeliveryHostReentrant"),
			Fixture, Host, Consumer))
	{
		return false;
	}
	FReentrantArcPreviewPresentationHostPort Port(
		Consumer, Host, Fixture);
	const auto Outer = UpdateArcPreviewDeliveryHost(
		Host, MakeArcPreviewChoice(false), Fixture, Port);
	TestTrue(TEXT("outer Host operation applies once after callback"),
		Outer.IsValid() && Outer.WasApplied()
			&& Port.ApplyCount == 1
			&& Host.IsValid() && Host.IsSynchronized()
			&& !Host.IsOperationInProgress());
	TestTrue(TEXT("port callback cannot re-enter the Host pipeline"),
		Port.ReentrantResult.IsValid()
			&& !Port.ReentrantResult.IsAccepted()
			&& Port.ReentrantResult.GetStatus()
				== EHost::OperationInProgress
			&& Port.ReentrantResult.GetStateUpdateCallCount() == 0
			&& Port.ReentrantResult.GetProjectionCallCount() == 0
			&& Port.ReentrantResult.GetDeliveryCallCount() == 0);
	TestTrue(TEXT("port callback cannot tear down the active Host"),
		!Port.EndAccepted && !Port.EndDiagnostic.IsEmpty()
			&& Host.GetState().IsVisible());
	return true;
}

namespace
{
	using EArcSurfaceCommand =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationCommandKind;
	using EArcSurfaceOutcome =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceResponseOutcome;
	using FArcSurfaceCommand =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommand;
	using FArcSurfaceResponse =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceResponse;
	using FArcSurfaceState =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState;

	struct FArcPreviewSurfaceCommandSet
	{
		FArcSurfaceCommand Show;
		FArcSurfaceCommand Replace;
		FArcSurfaceCommand NoOp;
		FArcSurfaceCommand Hide;
	};

	bool BuildArcPreviewSurfaceCommands(
		FAutomationTestBase& Test,
		const TCHAR* Label,
		FThrownLifecycleFixture& Fixture,
		FArcPreviewSurfaceCommandSet& OutCommands)
	{
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSession Session;
		if (!StartArcPreviewPresentationSession(
				Test, Label, Fixture, Session))
		{
			return false;
		}
		const auto InitialChoice = MakeArcPreviewChoice(false);
		const auto RevisedChoice = MakeArcPreviewChoice(true);
		const auto Show =
			Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandProjector::
				Project(UpdateArcPreviewPresentationSession(
					Session, InitialChoice, Fixture));
		const auto Replace =
			Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandProjector::
				Project(UpdateArcPreviewPresentationSession(
					Session, RevisedChoice, Fixture));
		const auto NoOp =
			Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandProjector::
				Project(UpdateArcPreviewPresentationSession(
					Session, RevisedChoice, Fixture));
		const auto Hide =
			Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandProjector::
				Project(UpdateArcPreviewPresentationSession(
					Session,
					ClearArcPreviewChoice(RevisedChoice),
					Fixture));
		if (!Show.IsProjected() || !Replace.IsProjected()
			|| !NoOp.IsProjected() || !Hide.IsProjected())
		{
			Test.AddError(TEXT(
				"Could not project the complete Arc preview surface command set."));
			return false;
		}
		OutCommands.Show = Show.GetCommand();
		OutCommands.Replace = Replace.GetCommand();
		OutCommands.NoOp = NoOp.GetCommand();
		OutCommands.Hide = Hide.GetCommand();
		return OutCommands.Show.IsShow()
			&& OutCommands.Replace.IsReplace()
			&& OutCommands.NoOp.IsNoOp()
			&& OutCommands.Hide.IsHide();
	}

	FArcSurfaceState SurfaceCursorForCommand(
		const FArcSurfaceCommand& Command)
	{
		return Command.GetState().IsVisible()
			? Command.GetState()
			: FArcSurfaceState();
	}

	class FFakeArcPreviewPresentationSurface final
		: public Idemo_mapShanmenThrownWeaponArcPreviewPresentationSurface
	{
	public:
		enum class EMode : uint8
		{
			Applied,
			Rejected,
			InvalidResponse,
			AppliedWithoutMutation,
			MutatedThenRejected
		};

		explicit FFakeArcPreviewPresentationSurface(
			const FName InConsumerDefinitionId,
			const EMode InMode = EMode::Applied)
			: ConsumerDefinitionId(InConsumerDefinitionId)
			, Mode(InMode)
		{
		}

		virtual FName GetConsumerDefinitionId() const override
		{
			return ConsumerDefinitionId;
		}

		virtual FArcSurfaceState GetSurfaceCursor() const override
		{
			return Cursor;
		}

		virtual FArcSurfaceResponse Show(
			const FArcSurfaceCommand& Command) override
		{
			return ApplyMutation(Command, EArcSurfaceCommand::Show);
		}

		virtual FArcSurfaceResponse Replace(
			const FArcSurfaceCommand& Command) override
		{
			return ApplyMutation(Command, EArcSurfaceCommand::Replace);
		}

		virtual FArcSurfaceResponse Hide(
			const FArcSurfaceCommand& Command) override
		{
			return ApplyMutation(Command, EArcSurfaceCommand::Hide);
		}

		void SetMode(const EMode Value) { Mode = Value; }
		void SetConsumerDefinitionId(const FName Value)
		{
			ConsumerDefinitionId = Value;
		}
		void ForceCursor(const FArcSurfaceState& Value) { Cursor = Value; }

		int32 MutationCallCount = 0;
		TArray<EArcSurfaceCommand> MutationKinds;

	private:
		FArcSurfaceResponse ApplyMutation(
			const FArcSurfaceCommand& Command,
			const EArcSurfaceCommand ExpectedKind)
		{
			++MutationCallCount;
			MutationKinds.Add(ExpectedKind);
			if (!Command.IsValid() || Command.GetKind() != ExpectedKind
				|| Mode == EMode::InvalidResponse)
			{
				return FArcSurfaceResponse();
			}

			const FArcSurfaceState Previous = Cursor;
			const FArcSurfaceState Target =
				SurfaceCursorForCommand(Command);
			EArcSurfaceOutcome Outcome = EArcSurfaceOutcome::Applied;
			FArcSurfaceState ReportedCursor = Target;
			if (Mode == EMode::Rejected)
			{
				Outcome = EArcSurfaceOutcome::Rejected;
				ReportedCursor = Previous;
			}
			else if (Mode == EMode::Applied)
			{
				Cursor = Target;
			}
			else if (Mode == EMode::MutatedThenRejected)
			{
				Cursor = Target;
				Outcome = EArcSurfaceOutcome::Rejected;
				ReportedCursor = Previous;
			}
			// AppliedWithoutMutation deliberately reports Target but leaves Cursor.

			const TCHAR* KindName = ExpectedKind == EArcSurfaceCommand::Show
				? TEXT("Show")
				: ExpectedKind == EArcSurfaceCommand::Replace
					? TEXT("Replace")
					: TEXT("Hide");
			const TCHAR* OutcomeName = Outcome == EArcSurfaceOutcome::Applied
				? TEXT("Applied")
				: TEXT("Rejected");
			FArcSurfaceResponse Response;
			FString Diagnostic;
			check(FArcSurfaceResponse::TryCreate(
				Command,
				Outcome,
				FName(*FString::Printf(
					TEXT("Renderer.ArcPreview.FakeSurface.%s%s"),
					KindName,
					OutcomeName)),
				Previous,
				ReportedCursor,
				Response,
				Diagnostic));
			return Response;
		}

		FName ConsumerDefinitionId = NAME_None;
		EMode Mode = EMode::Applied;
		FArcSurfaceState Cursor;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewSurfaceResponseContractTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationConsumerAdapter.SurfaceResponseContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewSurfaceResponseContractTest::RunTest(
	const FString&)
{
	FThrownLifecycleFixture Fixture;
	FArcPreviewSurfaceCommandSet Commands;
	if (!BuildArcPreviewSurfaceCommands(
			*this, TEXT("ArcPreviewSurfaceResponse"), Fixture, Commands))
	{
		return false;
	}
	FArcSurfaceResponse Applied;
	FArcSurfaceResponse Replay;
	FArcSurfaceResponse Rejected;
	FArcSurfaceResponse Invalid;
	FString Diagnostic;
	const FName AppliedCode(TEXT("Renderer.ArcPreview.Surface.ShowApplied"));
	TestTrue(TEXT("same Show surface evidence is deterministic"),
		FArcSurfaceResponse::TryCreate(
			Commands.Show,
			EArcSurfaceOutcome::Applied,
			AppliedCode,
			FArcSurfaceState(),
			Commands.Show.GetState(),
			Applied,
			Diagnostic)
			&& FArcSurfaceResponse::TryCreate(
				Commands.Show,
				EArcSurfaceOutcome::Applied,
				AppliedCode,
				FArcSurfaceState(),
				Commands.Show.GetState(),
				Replay,
				Diagnostic)
			&& Applied.IsApplied()
			&& Applied.MatchesCommand(Commands.Show)
			&& Applied.GetResponseId() == Replay.GetResponseId());
	TestTrue(TEXT("Rejected Show preserves one empty surface cursor"),
		FArcSurfaceResponse::TryCreate(
			Commands.Show,
			EArcSurfaceOutcome::Rejected,
			FName(TEXT("Renderer.ArcPreview.Surface.ShowRejected")),
			FArcSurfaceState(),
			FArcSurfaceState(),
			Rejected,
			Diagnostic)
			&& Rejected.IsRejected()
			&& Rejected.MatchesCommand(Commands.Show)
			&& Rejected.GetResponseId() != Applied.GetResponseId());
	TestFalse(TEXT("NoOp cannot forge a mutating surface response"),
		FArcSurfaceResponse::TryCreate(
			Commands.NoOp,
			EArcSurfaceOutcome::Applied,
			AppliedCode,
			Commands.NoOp.GetState(),
			Commands.NoOp.GetState(),
			Invalid,
			Diagnostic));
	TestFalse(TEXT("surface response rejects invalid outcome"),
		FArcSurfaceResponse::TryCreate(
			Commands.Show,
			EArcSurfaceOutcome::Invalid,
			AppliedCode,
			FArcSurfaceState(),
			Commands.Show.GetState(),
			Invalid,
			Diagnostic));
	TestFalse(TEXT("surface response rejects a missing outcome code"),
		FArcSurfaceResponse::TryCreate(
			Commands.Show,
			EArcSurfaceOutcome::Applied,
			NAME_None,
			FArcSurfaceState(),
			Commands.Show.GetState(),
			Invalid,
			Diagnostic));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewConsumerAdapterLifecycleTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationConsumerAdapter.LifecycleAndPreflight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewConsumerAdapterLifecycleTest::RunTest(
	const FString&)
{
	using EStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationConsumerAdapterStatus;
	using FAdapter =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationConsumerAdapter;
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture Fixture;
	FArcPreviewSurfaceCommandSet Commands;
	if (!BuildArcPreviewSurfaceCommands(
			*this, TEXT("ArcPreviewConsumerLifecycle"), Fixture, Commands))
	{
		return false;
	}
	FFakeArcPreviewPresentationSurface Surface(Consumer);
	FAdapter Adapter;
	const auto InactiveResponse = Adapter.Apply(Commands.Show);
	TestTrue(TEXT("inactive adapter rejects with typed zero-call evidence"),
		InactiveResponse.IsRejected()
			&& Adapter.GetLastResult().IsValid()
			&& Adapter.GetLastResult().GetStatus()
				== EStatus::AdapterInactive
			&& !Adapter.GetLastResult().DidCallSurface()
			&& Surface.MutationCallCount == 0);

	FString Diagnostic;
	Surface.ForceCursor(Commands.Show.GetState());
	TestFalse(TEXT("visible surface cannot be silently rebound"),
		Adapter.TryBegin(
			Fixture.Correlation.ActiveRunId, Surface, Diagnostic));
	Surface.ForceCursor(FArcSurfaceState());
	TestTrue(TEXT("empty surface binds and exact begin replay is idempotent"),
		Adapter.TryBegin(
			Fixture.Correlation.ActiveRunId, Surface, Diagnostic)
			&& Adapter.TryBegin(
				Fixture.Correlation.ActiveRunId, Surface, Diagnostic)
			&& Adapter.IsValid() && Adapter.IsActive());
	FFakeArcPreviewPresentationSurface OtherSurface(Consumer);
	TestFalse(TEXT("active adapter rejects surface rotation"),
		Adapter.TryBegin(
			Fixture.Correlation.ActiveRunId, OtherSurface, Diagnostic));
	FThrownLifecycleFixture ForeignFixture;
	FArcPreviewSurfaceCommandSet ForeignCommands;
	check(BuildArcPreviewSurfaceCommands(
		*this,
		TEXT("ArcPreviewConsumerForeignRun"),
		ForeignFixture,
		ForeignCommands));
	const auto ForeignRunResponse = Adapter.Apply(ForeignCommands.Show);
	TestTrue(TEXT("foreign Run command rejects before surface dispatch"),
		ForeignRunResponse.IsRejected()
			&& Adapter.GetLastResult().GetStatus() == EStatus::RunMismatch
			&& Surface.MutationCallCount == 0);

	Surface.SetConsumerDefinitionId(
		FName(TEXT("Renderer.ArcPreview.Foreign.r1")));
	const auto InvalidResponse = Adapter.Apply(Commands.Show);
	TestTrue(TEXT("consumer identity drift fails before surface mutation"),
		InvalidResponse.IsRejected()
			&& Adapter.GetLastResult().GetStatus()
				== EStatus::AdapterInvalid
			&& Surface.MutationCallCount == 0);
	Surface.SetConsumerDefinitionId(Consumer);
	Surface.ForceCursor(Commands.Show.GetState());
	const auto CursorResponse = Adapter.Apply(Commands.Show);
	TestTrue(TEXT("surface cursor drift rejects before dispatch"),
		CursorResponse.IsRejected()
			&& Adapter.GetLastResult().GetStatus()
				== EStatus::CursorMismatch
			&& Surface.MutationCallCount == 0);
	Surface.ForceCursor(FArcSurfaceState());
	const auto Applied = Adapter.Apply(Commands.Show);
	TestTrue(TEXT("valid Show makes the surface visible once"),
		Applied.IsApplied() && Adapter.GetLastResult().WasApplied()
			&& Surface.MutationCallCount == 1
			&& Adapter.GetSurfaceCursor().Matches(
				Commands.Show.GetState()));
	TestFalse(TEXT("visible surface prevents adapter teardown"),
		Adapter.TryEnd(Fixture.Correlation.ActiveRunId, Diagnostic));
	Surface.ForceCursor(FArcSurfaceState());
	TestTrue(TEXT("empty exact Run permits adapter teardown"),
		Adapter.TryEnd(Fixture.Correlation.ActiveRunId, Diagnostic)
			&& Adapter.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewConsumerAdapterSequenceTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationConsumerAdapter.ShowReplaceNoOpHide",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewConsumerAdapterSequenceTest::RunTest(
	const FString&)
{
	using FAdapter =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationConsumerAdapter;
	using FDeliverySession =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliverySession;
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture Fixture;
	FArcPreviewSurfaceCommandSet Commands;
	if (!BuildArcPreviewSurfaceCommands(
			*this, TEXT("ArcPreviewConsumerSequence"), Fixture, Commands))
	{
		return false;
	}
	FFakeArcPreviewPresentationSurface Surface(Consumer);
	FAdapter Adapter;
	FDeliverySession Delivery;
	FString Diagnostic;
	check(Adapter.TryBegin(
		Fixture.Correlation.ActiveRunId, Surface, Diagnostic));
	check(Delivery.TryBegin(
		Fixture.Correlation.ActiveRunId, Consumer, Diagnostic));

	const auto Show = Delivery.TryDeliver(Commands.Show, Adapter);
	const auto ShowAdapter = Adapter.GetLastResult();
	const auto Replace = Delivery.TryDeliver(Commands.Replace, Adapter);
	const auto ReplaceAdapter = Adapter.GetLastResult();
	const auto NoOp = Delivery.TryDeliver(Commands.NoOp, Adapter);
	const auto NoOpAdapter = Adapter.GetLastResult();
	const auto Hide = Delivery.TryDeliver(Commands.Hide, Adapter);
	const auto HideAdapter = Adapter.GetLastResult();
	TestTrue(TEXT("Show Replace and Hide each make one typed surface call"),
		Show.WasApplied() && ShowAdapter.WasApplied()
			&& ShowAdapter.GetSurfaceCallCount() == 1
			&& Replace.WasApplied() && ReplaceAdapter.WasApplied()
			&& ReplaceAdapter.GetSurfaceCallCount() == 1
			&& Hide.WasApplied() && HideAdapter.WasApplied()
			&& HideAdapter.GetSurfaceCallCount() == 1
			&& Surface.MutationCallCount == 3);
	TestTrue(TEXT("NoOp advances delivery evidence with zero surface mutation"),
		NoOp.WasApplied() && NoOp.DidCallPort()
			&& NoOpAdapter.WasApplied() && NoOpAdapter.IsNoOp()
			&& !NoOpAdapter.DidCallSurface());
	TestTrue(TEXT("surface dispatch order is exactly Show Replace Hide"),
		Surface.MutationKinds.Num() == 3
			&& Surface.MutationKinds[0] == EArcSurfaceCommand::Show
			&& Surface.MutationKinds[1] == EArcSurfaceCommand::Replace
			&& Surface.MutationKinds[2] == EArcSurfaceCommand::Hide);
	TestTrue(TEXT("Hide normalizes physical surface cursor to empty"),
		Adapter.GetSurfaceCursor().IsEmpty()
			&& Delivery.GetCursorState().IsHidden()
			&& Delivery.NumAppliedCommands() == 4);
	TestTrue(TEXT("empty surface and hidden ledger end independently"),
		Adapter.TryEnd(Fixture.Correlation.ActiveRunId, Diagnostic)
			&& Delivery.TryEnd(
				Fixture.Correlation.ActiveRunId, Diagnostic)
			&& Adapter.IsEmpty() && Delivery.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewConsumerAdapterRejectionTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationConsumerAdapter.SurfaceRejectionReplay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewConsumerAdapterRejectionTest::RunTest(
	const FString&)
{
	using EStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationConsumerAdapterStatus;
	using FAdapter =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationConsumerAdapter;
	using FDeliverySession =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliverySession;
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture Fixture;
	FArcPreviewSurfaceCommandSet Commands;
	if (!BuildArcPreviewSurfaceCommands(
			*this, TEXT("ArcPreviewConsumerRejection"), Fixture, Commands))
	{
		return false;
	}
	FFakeArcPreviewPresentationSurface Surface(
		Consumer,
		FFakeArcPreviewPresentationSurface::EMode::Rejected);
	FAdapter Adapter;
	FDeliverySession Delivery;
	FString Diagnostic;
	check(Adapter.TryBegin(
		Fixture.Correlation.ActiveRunId, Surface, Diagnostic));
	check(Delivery.TryBegin(
		Fixture.Correlation.ActiveRunId, Consumer, Diagnostic));
	const auto Rejected = Delivery.TryDeliver(Commands.Show, Adapter);
	const auto AdapterResult = Adapter.GetLastResult();
	const auto Replay = Delivery.TryDeliver(Commands.Show, Adapter);
	TestTrue(TEXT("surface rejection is sealed without moving either cursor"),
		Rejected.IsValid() && Rejected.WasRejected()
			&& AdapterResult.IsAccepted() && AdapterResult.WasRejected()
			&& AdapterResult.GetStatus() == EStatus::SurfaceRejected
			&& AdapterResult.DidCallSurface()
			&& Adapter.GetSurfaceCursor().IsEmpty()
			&& Delivery.GetCursorState().IsEmpty());
	TestTrue(TEXT("delivery rejection replay never calls the surface twice"),
		Replay.IsValid() && Replay.WasRejected() && Replay.IsReplay()
			&& !Replay.DidCallPort()
			&& Surface.MutationCallCount == 1
			&& Adapter.GetLastResult().GetSurfaceResponse().MatchesCommand(
				Commands.Show));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewConsumerAdapterInvariantTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationConsumerAdapter.SurfaceInvariantFailures",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewConsumerAdapterInvariantTest::RunTest(
	const FString&)
{
	using EMode = FFakeArcPreviewPresentationSurface::EMode;
	using EStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationConsumerAdapterStatus;
	using FAdapter =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationConsumerAdapter;
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture Fixture;
	FArcPreviewSurfaceCommandSet Commands;
	if (!BuildArcPreviewSurfaceCommands(
			*this, TEXT("ArcPreviewConsumerInvariant"), Fixture, Commands))
	{
		return false;
	}
	FString Diagnostic;

	FFakeArcPreviewPresentationSurface InvalidSurface(
		Consumer, EMode::InvalidResponse);
	FAdapter InvalidAdapter;
	check(InvalidAdapter.TryBegin(
		Fixture.Correlation.ActiveRunId, InvalidSurface, Diagnostic));
	const auto Invalid = InvalidAdapter.Apply(Commands.Show);
	TestTrue(TEXT("invalid surface evidence becomes a typed port rejection"),
		Invalid.IsRejected()
			&& InvalidAdapter.GetLastResult().IsValid()
			&& InvalidAdapter.GetLastResult().GetStatus()
				== EStatus::SurfaceResponseInvalid
			&& InvalidSurface.GetSurfaceCursor().IsEmpty());

	FFakeArcPreviewPresentationSurface NoMutationSurface(
		Consumer, EMode::AppliedWithoutMutation);
	FAdapter NoMutationAdapter;
	check(NoMutationAdapter.TryBegin(
		Fixture.Correlation.ActiveRunId, NoMutationSurface, Diagnostic));
	const auto NoMutation = NoMutationAdapter.Apply(Commands.Show);
	TestTrue(TEXT("Applied response without observed mutation fails closed"),
		NoMutation.IsRejected()
			&& NoMutationAdapter.GetLastResult().IsValid()
			&& NoMutationAdapter.GetLastResult().GetStatus()
				== EStatus::SurfaceInvariantViolation
			&& NoMutationSurface.GetSurfaceCursor().IsEmpty());

	FFakeArcPreviewPresentationSurface RejectedMutationSurface(
		Consumer, EMode::MutatedThenRejected);
	FAdapter RejectedMutationAdapter;
	check(RejectedMutationAdapter.TryBegin(
		Fixture.Correlation.ActiveRunId,
		RejectedMutationSurface,
		Diagnostic));
	const auto RejectedMutation =
		RejectedMutationAdapter.Apply(Commands.Show);
	TestTrue(TEXT("Rejected response that mutates the surface fails closed"),
		RejectedMutation.IsRejected()
			&& RejectedMutationAdapter.GetLastResult().IsValid()
			&& RejectedMutationAdapter.GetLastResult().GetStatus()
				== EStatus::SurfaceInvariantViolation
			&& RejectedMutationSurface.GetSurfaceCursor().Matches(
				Commands.Show.GetState())
			&& !RejectedMutationAdapter.CanEnd());
	return true;
}

namespace
{
	class FReentrantArcPreviewPresentationSurface final
		: public Idemo_mapShanmenThrownWeaponArcPreviewPresentationSurface
	{
	public:
		FReentrantArcPreviewPresentationSurface(
			const FName InConsumerDefinitionId,
			Fdemo_mapShanmenThrownWeaponArcPreviewPresentationConsumerAdapter&
				InAdapter)
			: ConsumerDefinitionId(InConsumerDefinitionId)
			, Adapter(InAdapter)
		{
		}

		virtual FName GetConsumerDefinitionId() const override
		{
			return ConsumerDefinitionId;
		}
		virtual FArcSurfaceState GetSurfaceCursor() const override
		{
			return Cursor;
		}
		virtual FArcSurfaceResponse Show(
			const FArcSurfaceCommand& Command) override
		{
			++MutationCallCount;
			EndAccepted = Adapter.TryEnd(
				Command.GetRunId(), EndDiagnostic);
			InnerPortResponse = Adapter.Apply(Command);
			InnerResult = Adapter.GetLastResult();
			const FArcSurfaceState Previous = Cursor;
			Cursor = Command.GetState();
			FArcSurfaceResponse Response;
			FString Diagnostic;
			check(FArcSurfaceResponse::TryCreate(
				Command,
				EArcSurfaceOutcome::Applied,
				FName(TEXT("Renderer.ArcPreview.ReentrantSurface.ShowApplied")),
				Previous,
				Cursor,
				Response,
				Diagnostic));
			return Response;
		}
		virtual FArcSurfaceResponse Replace(
			const FArcSurfaceCommand&) override
		{
			return FArcSurfaceResponse();
		}
		virtual FArcSurfaceResponse Hide(
			const FArcSurfaceCommand&) override
		{
			return FArcSurfaceResponse();
		}

		int32 MutationCallCount = 0;
		bool EndAccepted = true;
		FString EndDiagnostic;
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationPortResponse
			InnerPortResponse;
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationConsumerAdapterResult
			InnerResult;

	private:
		FName ConsumerDefinitionId = NAME_None;
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationConsumerAdapter&
			Adapter;
		FArcSurfaceState Cursor;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewConsumerAdapterReentrantTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationConsumerAdapter.ReentrantSurfaceBlocked",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewConsumerAdapterReentrantTest::RunTest(
	const FString&)
{
	using EStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationConsumerAdapterStatus;
	using FAdapter =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationConsumerAdapter;
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture Fixture;
	FArcPreviewSurfaceCommandSet Commands;
	if (!BuildArcPreviewSurfaceCommands(
			*this, TEXT("ArcPreviewConsumerReentrant"), Fixture, Commands))
	{
		return false;
	}
	FAdapter Adapter;
	FReentrantArcPreviewPresentationSurface Surface(Consumer, Adapter);
	FString Diagnostic;
	check(Adapter.TryBegin(
		Fixture.Correlation.ActiveRunId, Surface, Diagnostic));
	const auto OuterResponse = Adapter.Apply(Commands.Show);
	const auto OuterResult = Adapter.GetLastResult();
	TestTrue(TEXT("outer surface mutation applies exactly once"),
		OuterResponse.IsApplied() && OuterResult.WasApplied()
			&& OuterResult.GetStatus() == EStatus::Applied
			&& Surface.MutationCallCount == 1
			&& Adapter.IsValid() && !Adapter.IsOperationInProgress());
	TestTrue(TEXT("surface callback cannot re-enter adapter Apply"),
		Surface.InnerPortResponse.IsRejected()
			&& Surface.InnerResult.IsValid()
			&& !Surface.InnerResult.IsAccepted()
			&& Surface.InnerResult.GetStatus()
				== EStatus::OperationInProgress
			&& !Surface.InnerResult.DidCallSurface());
	TestTrue(TEXT("surface callback cannot tear down active adapter"),
		!Surface.EndAccepted && !Surface.EndDiagnostic.IsEmpty()
			&& Adapter.GetSurfaceCursor().Matches(
				Commands.Show.GetState()));
	return true;
}

namespace
{
	using FArcCompositionOwner =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCompositionOwner;
	using FArcCompositionUpdate =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCompositionOwnerUpdateResult;

	bool StartArcPreviewCompositionOwner(
		FAutomationTestBase& Test,
		const TCHAR* Label,
		FThrownLifecycleFixture& Fixture,
		FArcCompositionOwner& Owner,
		Idemo_mapShanmenThrownWeaponArcPreviewPresentationSurface& Surface)
	{
		if (!Fixture.Start(
				Test,
				Label,
				Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::
					BallisticArc))
		{
			return false;
		}
		FString Diagnostic;
		if (!Owner.TryBegin(
				Fixture.Correlation.ActiveRunId, Surface, Diagnostic))
		{
			Test.AddError(Diagnostic);
			return false;
		}
		return true;
	}

	FArcCompositionUpdate UpdateArcPreviewCompositionOwner(
		FArcCompositionOwner& Owner,
		const Fdemo_mapShanmenThrownWeaponInputChoiceState& Choice,
		const FThrownLifecycleFixture& Fixture)
	{
		return Owner.TryUpdate(
			2,
			Choice,
			MakeArcPreviewChoicePolicy(),
			8,
			MakeArcPreviewBasis(),
			Fixture.Lifecycle,
			Fixture.Coordinator);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewCompositionOwnerLifecycleTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationCompositionOwner.LifecycleAndPreflight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewCompositionOwnerLifecycleTest::RunTest(
	const FString&)
{
	using EStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationCompositionOwnerUpdateStatus;
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture Fixture;
	FArcPreviewSurfaceCommandSet Commands;
	if (!BuildArcPreviewSurfaceCommands(
			*this, TEXT("ArcPreviewCompositionLifecycle"), Fixture, Commands))
	{
		return false;
	}
	FFakeArcPreviewPresentationSurface Surface(Consumer);
	FArcCompositionOwner Owner;
	FString Diagnostic;
	TestTrue(TEXT("default composition owner is valid and empty"),
		Owner.IsValid() && Owner.IsEmpty());
	Surface.ForceCursor(Commands.Show.GetState());
	TestFalse(TEXT("visible physical surface cannot be silently adopted"),
		Owner.TryBegin(
			Fixture.Correlation.ActiveRunId, Surface, Diagnostic));
	Surface.ForceCursor(FArcSurfaceState());
	TestTrue(TEXT("empty surface binds Host and Adapter atomically"),
		Owner.TryBegin(
			Fixture.Correlation.ActiveRunId, Surface, Diagnostic)
			&& Owner.TryBegin(
				Fixture.Correlation.ActiveRunId, Surface, Diagnostic)
			&& Owner.IsValid() && Owner.IsActive()
			&& Owner.IsSynchronized()
			&& Owner.GetRunId() == Fixture.Correlation.ActiveRunId
			&& Owner.GetConsumerDefinitionId() == Consumer);
	FFakeArcPreviewPresentationSurface OtherSurface(Consumer);
	TestFalse(TEXT("active composition owner rejects surface rotation"),
		Owner.TryBegin(
			Fixture.Correlation.ActiveRunId, OtherSurface, Diagnostic));

	Surface.SetConsumerDefinitionId(
		FName(TEXT("Renderer.ArcPreview.Foreign.r1")));
	const auto ConsumerDrift = UpdateArcPreviewCompositionOwner(
		Owner, MakeArcPreviewChoice(false), Fixture);
	TestTrue(TEXT("consumer drift rejects before Host or Adapter calls"),
		ConsumerDrift.IsValid()
			&& ConsumerDrift.GetStatus() == EStatus::OwnerInvalid
			&& !ConsumerDrift.DidCallHost()
			&& !ConsumerDrift.DidCallAdapter());
	Surface.SetConsumerDefinitionId(Consumer);
	Surface.ForceCursor(Commands.Show.GetState());
	const auto CursorDrift = UpdateArcPreviewCompositionOwner(
		Owner, MakeArcPreviewChoice(false), Fixture);
	TestTrue(TEXT("physical cursor drift rejects before Host mutation"),
		CursorDrift.IsValid()
			&& CursorDrift.GetStatus() == EStatus::OwnerInvalid
			&& !CursorDrift.DidCallHost()
			&& !CursorDrift.DidCallAdapter());
	Surface.ForceCursor(FArcSurfaceState());
	TestFalse(TEXT("foreign Run cannot tear down the active owner"),
		Owner.TryEnd(
			FGuid(0xF4300001, 0xF4300002, 0xF4300003, 0xF4300004),
			Diagnostic));
	TestTrue(TEXT("empty exact Run ends both child scopes atomically"),
		Owner.TryEnd(Fixture.Correlation.ActiveRunId, Diagnostic)
			&& Owner.IsValid() && Owner.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewCompositionOwnerOrderedTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationCompositionOwner.OrderedAppliedReplay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewCompositionOwnerOrderedTest::RunTest(
	const FString&)
{
	using EStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationCompositionOwnerUpdateStatus;
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture Fixture;
	FFakeArcPreviewPresentationSurface Surface(Consumer);
	FArcCompositionOwner Owner;
	if (!StartArcPreviewCompositionOwner(
			*this, TEXT("ArcPreviewCompositionOrdered"),
			Fixture, Owner, Surface))
	{
		return false;
	}
	const auto InitialChoice = MakeArcPreviewChoice(false);
	const auto RevisedChoice = MakeArcPreviewChoice(true);
	const auto Show = UpdateArcPreviewCompositionOwner(
		Owner, InitialChoice, Fixture);
	const auto Replace = UpdateArcPreviewCompositionOwner(
		Owner, RevisedChoice, Fixture);
	const auto NoOp = UpdateArcPreviewCompositionOwner(
		Owner, RevisedChoice, Fixture);
	const auto Replay = UpdateArcPreviewCompositionOwner(
		Owner, RevisedChoice, Fixture);
	const auto Hide = UpdateArcPreviewCompositionOwner(
		Owner, ClearArcPreviewChoice(RevisedChoice), Fixture);

	TestTrue(TEXT("Show and Replace each cross Host and Adapter once"),
		Show.IsValid() && Show.WasApplied()
			&& Show.GetStatus() == EStatus::Applied
			&& Show.DidCallHost() && Show.DidCallAdapter()
			&& Show.GetAdapterResult().DidCallSurface()
			&& Replace.IsValid() && Replace.WasApplied()
			&& Replace.DidCallHost() && Replace.DidCallAdapter()
			&& Replace.GetAdapterResult().DidCallSurface());
	TestTrue(TEXT("NoOp calls Adapter but never mutates surface"),
		NoOp.IsValid() && NoOp.WasApplied()
			&& NoOp.DidCallAdapter()
			&& NoOp.GetAdapterResult().IsNoOp()
			&& !NoOp.GetAdapterResult().DidCallSurface());
	TestTrue(TEXT("exact command replay stays above the Adapter"),
		Replay.IsValid() && Replay.WasApplied() && Replay.IsReplay()
			&& Replay.GetStatus() == EStatus::ApplicationReplayed
			&& Replay.DidCallHost() && !Replay.DidCallAdapter());
	TestTrue(TEXT("Hide reconciles audit Hidden with physical Empty"),
		Hide.IsValid() && Hide.WasApplied()
			&& Hide.DidCallAdapter()
			&& Owner.IsValid() && Owner.IsSynchronized()
			&& Owner.GetState().IsHidden()
			&& Owner.GetHostCursor().IsHidden()
			&& Owner.GetSurfaceCursor().IsEmpty()
			&& Surface.MutationCallCount == 3
			&& Surface.MutationKinds.Num() == 3
			&& Surface.MutationKinds[0] == EArcSurfaceCommand::Show
			&& Surface.MutationKinds[1] == EArcSurfaceCommand::Replace
			&& Surface.MutationKinds[2] == EArcSurfaceCommand::Hide);
	FString Diagnostic;
	TestTrue(TEXT("synchronized Hide permits one owner teardown"),
		Owner.CanEnd()
			&& Owner.TryEnd(Fixture.Correlation.ActiveRunId, Diagnostic)
			&& Owner.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewCompositionOwnerRecoveryTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationCompositionOwner.RejectionRecovery",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewCompositionOwnerRecoveryTest::RunTest(
	const FString&)
{
	using ERecovery =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationCompositionOwnerRecoveryStatus;
	using EUpdate =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationCompositionOwnerUpdateStatus;
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture Fixture;
	FFakeArcPreviewPresentationSurface Surface(
		Consumer,
		FFakeArcPreviewPresentationSurface::EMode::Rejected);
	FArcCompositionOwner Owner;
	if (!StartArcPreviewCompositionOwner(
			*this, TEXT("ArcPreviewCompositionRecovery"),
			Fixture, Owner, Surface))
	{
		return false;
	}
	const auto Choice = MakeArcPreviewChoice(false);
	const auto Rejected = UpdateArcPreviewCompositionOwner(
		Owner, Choice, Fixture);
	TestTrue(TEXT("surface rejection freezes exact Host recovery work"),
		Rejected.IsValid() && Rejected.WasRejected()
			&& Rejected.GetStatus() == EUpdate::RejectedPendingRecovery
			&& Rejected.DidCallHost() && Rejected.DidCallAdapter()
			&& Rejected.GetAdapterResult().WasRejected()
			&& Owner.IsValid() && Owner.NeedsRecovery()
			&& Owner.GetState().IsVisible()
			&& Owner.GetHostCursor().IsEmpty()
			&& Owner.GetSurfaceCursor().IsEmpty());
	const auto Fenced = UpdateArcPreviewCompositionOwner(
		Owner, MakeArcPreviewChoice(true), Fixture);
	TestTrue(TEXT("pending recovery blocks newer update before Adapter"),
		Fenced.IsValid() && !Fenced.IsAccepted()
			&& Fenced.GetStatus() == EUpdate::HostRejected
			&& Fenced.GetHostResult().NeedsRecovery()
			&& Fenced.DidCallHost() && !Fenced.DidCallAdapter());

	const auto RetryRejected = Owner.TryRecoverRejected();
	TestTrue(TEXT("one rejected recovery retry remains bounded and pending"),
		RetryRejected.IsValid() && RetryRejected.WasRejected()
			&& RetryRejected.GetStatus() == ERecovery::AdapterRejected
			&& RetryRejected.DidCallAdapter()
			&& !RetryRejected.DidCallHostRecovery()
			&& Owner.IsValid() && Owner.NeedsRecovery()
			&& Surface.MutationCallCount == 2);
	Surface.SetMode(FFakeArcPreviewPresentationSurface::EMode::Applied);
	const auto Recovered = Owner.TryRecoverRejected();
	TestTrue(TEXT("Applied surface retry is sealed and recorded once"),
		Recovered.IsValid() && Recovered.DidRecover()
			&& Recovered.GetStatus() == ERecovery::Recovered
			&& Recovered.DidCallAdapter()
			&& Recovered.DidCallHostRecovery()
			&& Recovered.GetAppliedReceipt().IsApplied()
			&& Owner.IsValid() && !Owner.NeedsRecovery()
			&& Owner.IsSynchronized()
			&& Owner.GetSurfaceCursor().Matches(Owner.GetHostCursor()));
	const auto NoWork = Owner.TryRecoverRejected();
	TestTrue(TEXT("recovery replay cannot duplicate an already applied effect"),
		NoWork.IsValid() && !NoWork.IsAccepted()
			&& NoWork.GetStatus() == ERecovery::NoRecoveryPending
			&& !NoWork.DidCallAdapter()
			&& !NoWork.DidCallHostRecovery()
			&& Surface.MutationCallCount == 3);
	const auto Hide = UpdateArcPreviewCompositionOwner(
		Owner, ClearArcPreviewChoice(Choice), Fixture);
	FString Diagnostic;
	TestTrue(TEXT("recovered owner can Hide and end normally"),
		Hide.WasApplied() && Owner.CanEnd()
			&& Owner.TryEnd(Fixture.Correlation.ActiveRunId, Diagnostic));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewCompositionOwnerDivergenceTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationCompositionOwner.DivergenceDetected",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewCompositionOwnerDivergenceTest::RunTest(
	const FString&)
{
	using ERecovery =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationCompositionOwnerRecoveryStatus;
	using EUpdate =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationCompositionOwnerUpdateStatus;
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture Fixture;
	FFakeArcPreviewPresentationSurface Surface(
		Consumer,
		FFakeArcPreviewPresentationSurface::EMode::MutatedThenRejected);
	FArcCompositionOwner Owner;
	if (!StartArcPreviewCompositionOwner(
			*this, TEXT("ArcPreviewCompositionDivergence"),
			Fixture, Owner, Surface))
	{
		return false;
	}
	const auto Diverged = UpdateArcPreviewCompositionOwner(
		Owner, MakeArcPreviewChoice(false), Fixture);
	TestTrue(TEXT("mutated Rejected surface cannot masquerade as recovery work"),
		Diverged.IsValid() && !Diverged.IsAccepted()
			&& Diverged.GetStatus() == EUpdate::InvariantViolation
			&& !Diverged.IsOwnerValidAfter()
			&& Owner.GetHost().NeedsRecovery()
			&& Owner.GetHostCursor().IsEmpty()
			&& Owner.GetSurfaceCursor().IsVisible()
			&& !Owner.IsValid());
	const auto Recovery = Owner.TryRecoverRejected();
	TestTrue(TEXT("divergent owner blocks automatic recovery before effects"),
		Recovery.IsValid() && !Recovery.IsAccepted()
			&& Recovery.GetStatus() == ERecovery::OwnerInvalid
			&& !Recovery.DidCallAdapter()
			&& !Recovery.DidCallHostRecovery()
			&& Surface.MutationCallCount == 1);
	return true;
}

namespace
{
	class FReentrantArcPreviewCompositionSurface final
		: public Idemo_mapShanmenThrownWeaponArcPreviewPresentationSurface
	{
	public:
		FReentrantArcPreviewCompositionSurface(
			const FName InConsumerDefinitionId,
			FArcCompositionOwner& InOwner,
			const FThrownLifecycleFixture& InFixture)
			: ConsumerDefinitionId(InConsumerDefinitionId)
			, Owner(InOwner)
			, Fixture(InFixture)
		{
		}

		virtual FName GetConsumerDefinitionId() const override
		{
			return ConsumerDefinitionId;
		}
		virtual FArcSurfaceState GetSurfaceCursor() const override
		{
			return Cursor;
		}
		virtual FArcSurfaceResponse Show(
			const FArcSurfaceCommand& Command) override
		{
			++MutationCallCount;
			InnerUpdate = UpdateArcPreviewCompositionOwner(
				Owner, MakeArcPreviewChoice(true), Fixture);
			InnerRecovery = Owner.TryRecoverRejected();
			EndAccepted = Owner.TryEnd(
				Command.GetRunId(), EndDiagnostic);
			const FArcSurfaceState Previous = Cursor;
			Cursor = Command.GetState();
			FArcSurfaceResponse Response;
			FString Diagnostic;
			check(FArcSurfaceResponse::TryCreate(
				Command,
				EArcSurfaceOutcome::Applied,
				FName(TEXT("Renderer.ArcPreview.CompositionReentrant.ShowApplied")),
				Previous,
				Cursor,
				Response,
				Diagnostic));
			return Response;
		}
		virtual FArcSurfaceResponse Replace(
			const FArcSurfaceCommand&) override
		{
			return FArcSurfaceResponse();
		}
		virtual FArcSurfaceResponse Hide(
			const FArcSurfaceCommand&) override
		{
			return FArcSurfaceResponse();
		}

		int32 MutationCallCount = 0;
		bool EndAccepted = true;
		FString EndDiagnostic;
		FArcCompositionUpdate InnerUpdate;
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCompositionOwnerRecoveryResult
			InnerRecovery;

	private:
		FName ConsumerDefinitionId = NAME_None;
		FArcCompositionOwner& Owner;
		const FThrownLifecycleFixture& Fixture;
		FArcSurfaceState Cursor;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewCompositionOwnerReentrantTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationCompositionOwner.ReentrantSurfaceBlocked",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewCompositionOwnerReentrantTest::RunTest(
	const FString&)
{
	using ERecovery =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationCompositionOwnerRecoveryStatus;
	using EUpdate =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationCompositionOwnerUpdateStatus;
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture Fixture;
	if (!Fixture.Start(
			*this,
			TEXT("ArcPreviewCompositionReentrant"),
			Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::BallisticArc))
	{
		return false;
	}
	FArcCompositionOwner Owner;
	FReentrantArcPreviewCompositionSurface Surface(
		Consumer, Owner, Fixture);
	FString Diagnostic;
	check(Owner.TryBegin(
		Fixture.Correlation.ActiveRunId, Surface, Diagnostic));
	const auto Outer = UpdateArcPreviewCompositionOwner(
		Owner, MakeArcPreviewChoice(false), Fixture);
	TestTrue(TEXT("outer owner update applies exactly once"),
		Outer.IsValid() && Outer.WasApplied()
			&& Surface.MutationCallCount == 1
			&& Owner.IsValid() && !Owner.IsOperationInProgress());
	TestTrue(TEXT("surface callback cannot re-enter owner update"),
		Surface.InnerUpdate.IsValid()
			&& !Surface.InnerUpdate.IsAccepted()
			&& Surface.InnerUpdate.GetStatus()
				== EUpdate::OperationInProgress
			&& !Surface.InnerUpdate.DidCallHost()
			&& !Surface.InnerUpdate.DidCallAdapter());
	TestTrue(TEXT("surface callback cannot enter owner recovery"),
		Surface.InnerRecovery.IsValid()
			&& !Surface.InnerRecovery.IsAccepted()
			&& Surface.InnerRecovery.GetStatus()
				== ERecovery::OperationInProgress
			&& !Surface.InnerRecovery.DidCallAdapter()
			&& !Surface.InnerRecovery.DidCallHostRecovery());
	TestTrue(TEXT("surface callback cannot tear down child scopes"),
		!Surface.EndAccepted && !Surface.EndDiagnostic.IsEmpty()
			&& Owner.IsActive() && Owner.GetSurfaceCursor().IsVisible());
	return true;
}

namespace
{
	using EArcSurfaceRecreationAction =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRecreationAction;
	using EArcSurfaceRecreationDisposition =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRecreationDisposition;
	using EArcSurfaceRecreationOutcome =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRecreationOutcome;
	using FArcSurfaceRecreationPolicy =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRecreationPolicy;
	using FArcSurfaceRecreationResult =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRecreationResult;

	FArcSurfaceRecreationResult EvaluateArcSurfaceRecreation(
		const FThrownLifecycleFixture& Fixture,
		const FName ExpectedConsumer,
		const FArcSurfaceState& AuthoritativeCursor,
		const FName ObservedConsumer,
		const FArcSurfaceState& ObservedCursor,
		const EArcSurfaceRecreationAction Action)
	{
		return FArcSurfaceRecreationPolicy::Evaluate(
			Fixture.Correlation.ActiveRunId,
			ExpectedConsumer,
			AuthoritativeCursor,
			ObservedConsumer,
			ObservedCursor,
			Action);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewSurfaceRecreationClassificationTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationSurfaceRecreationPolicy.ClassificationMatrix",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewSurfaceRecreationClassificationTest::
	RunTest(const FString&)
{
	using EDisposition = EArcSurfaceRecreationDisposition;
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture Fixture;
	FArcPreviewSurfaceCommandSet Commands;
	if (!BuildArcPreviewSurfaceCommands(
			*this, TEXT("ArcPreviewSurfaceRecreationClassification"),
			Fixture, Commands))
	{
		return false;
	}
	FThrownLifecycleFixture ForeignFixture;
	FArcPreviewSurfaceCommandSet ForeignCommands;
	check(BuildArcPreviewSurfaceCommands(
		*this, TEXT("ArcPreviewSurfaceRecreationForeign"),
		ForeignFixture, ForeignCommands));

	const auto Fresh = EvaluateArcSurfaceRecreation(
		Fixture, Consumer, FArcSurfaceState(), Consumer,
		FArcSurfaceState(), EArcSurfaceRecreationAction::Inspect);
	const auto Hidden = EvaluateArcSurfaceRecreation(
		Fixture, Consumer, Commands.Hide.GetState(), Consumer,
		FArcSurfaceState(), EArcSurfaceRecreationAction::Inspect);
	const auto Exact = EvaluateArcSurfaceRecreation(
		Fixture, Consumer, Commands.Show.GetState(), Consumer,
		Commands.Show.GetState(), EArcSurfaceRecreationAction::Inspect);
	const auto Rehydrate = EvaluateArcSurfaceRecreation(
		Fixture, Consumer, Commands.Show.GetState(), Consumer,
		FArcSurfaceState(), EArcSurfaceRecreationAction::Inspect);
	const auto Residual = EvaluateArcSurfaceRecreation(
		Fixture, Consumer, Commands.Hide.GetState(), Consumer,
		Commands.Show.GetState(), EArcSurfaceRecreationAction::Inspect);
	const auto Foreign = EvaluateArcSurfaceRecreation(
		Fixture, Consumer, Commands.Show.GetState(), Consumer,
		ForeignCommands.Show.GetState(),
		EArcSurfaceRecreationAction::Inspect);
	const auto Conflict = EvaluateArcSurfaceRecreation(
		Fixture, Consumer, Commands.Show.GetState(), Consumer,
		Commands.Replace.GetState(), EArcSurfaceRecreationAction::Inspect);
	const auto ConsumerMismatch = EvaluateArcSurfaceRecreation(
		Fixture, Consumer, Commands.Show.GetState(),
		FName(TEXT("Renderer.ArcPreview.Other.r1")),
		Commands.Show.GetState(), EArcSurfaceRecreationAction::Inspect);

	TestTrue(TEXT("empty and hidden authority both normalize to fresh empty"),
		Fresh.IsValid() && Fresh.IsInspected()
			&& Fresh.GetDisposition() == EDisposition::FreshEmpty
			&& Fresh.CanBindFresh()
			&& Hidden.IsValid() && Hidden.IsInspected()
			&& Hidden.GetDisposition() == EDisposition::FreshEmpty
			&& Hidden.GetExpectedSurfaceCursor().IsEmpty());
	TestTrue(TEXT("exact visible surface is the only direct adoption state"),
		Exact.IsValid() && Exact.IsInspected()
			&& Exact.GetDisposition() == EDisposition::ExactVisible
			&& Exact.CanAdoptExact() && !Exact.HasPermit());
	TestTrue(TEXT("empty replacement of visible authority requires rehydrate"),
		Rehydrate.IsValid() && Rehydrate.IsInspected()
			&& Rehydrate.GetDisposition()
				== EDisposition::EmptyNeedsRehydrate
			&& Rehydrate.NeedsRehydrate());
	TestTrue(TEXT("visible conflicts are separated by residual foreign and state"),
		Residual.GetDisposition() == EDisposition::ResidualVisible
			&& Residual.RequiresExplicitCleanup()
			&& Foreign.GetDisposition() == EDisposition::ForeignVisible
			&& Foreign.RequiresExplicitCleanup()
			&& Conflict.GetDisposition() == EDisposition::ConflictingVisible
			&& Conflict.RequiresExplicitCleanup());
	TestTrue(TEXT("consumer mismatch remains inspectable but never bindable"),
		ConsumerMismatch.IsValid() && ConsumerMismatch.IsInspected()
			&& ConsumerMismatch.GetDisposition()
				== EDisposition::ConsumerMismatch
			&& !ConsumerMismatch.CanBindFresh()
			&& !ConsumerMismatch.CanAdoptExact());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewSurfaceRecreationBindingPermitTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationSurfaceRecreationPolicy.BindingPermits",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewSurfaceRecreationBindingPermitTest::
	RunTest(const FString&)
{
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture Fixture;
	FArcPreviewSurfaceCommandSet Commands;
	if (!BuildArcPreviewSurfaceCommands(
			*this, TEXT("ArcPreviewSurfaceRecreationBinding"),
			Fixture, Commands))
	{
		return false;
	}
	const auto Fresh = EvaluateArcSurfaceRecreation(
		Fixture, Consumer, FArcSurfaceState(), Consumer,
		FArcSurfaceState(), EArcSurfaceRecreationAction::BindFresh);
	const auto FreshReplay = EvaluateArcSurfaceRecreation(
		Fixture, Consumer, FArcSurfaceState(), Consumer,
		FArcSurfaceState(), EArcSurfaceRecreationAction::BindFresh);
	const auto Adopt = EvaluateArcSurfaceRecreation(
		Fixture, Consumer, Commands.Show.GetState(), Consumer,
		Commands.Show.GetState(), EArcSurfaceRecreationAction::AdoptExact);
	const auto AdoptReplay = EvaluateArcSurfaceRecreation(
		Fixture, Consumer, Commands.Show.GetState(), Consumer,
		Commands.Show.GetState(), EArcSurfaceRecreationAction::AdoptExact);

	TestTrue(TEXT("fresh empty binding produces one deterministic exact permit"),
		Fresh.IsValid() && Fresh.IsAuthorized() && Fresh.HasPermit()
			&& Fresh.GetPermit().IsBindingPermit()
			&& Fresh.GetDecisionId() == FreshReplay.GetDecisionId()
			&& Fresh.GetPermit().GetPermitId()
				== FreshReplay.GetPermit().GetPermitId()
			&& Fresh.GetPermit().MatchesSnapshot(
				Fixture.Correlation.ActiveRunId,
				Consumer, FArcSurfaceState(), FArcSurfaceState()));
	TestTrue(TEXT("exact visible adoption produces a different bound permit"),
		Adopt.IsValid() && Adopt.IsAuthorized() && Adopt.HasPermit()
			&& Adopt.GetPermit().IsBindingPermit()
			&& Adopt.GetDecisionId() == AdoptReplay.GetDecisionId()
			&& Adopt.GetPermit().GetPermitId()
				== AdoptReplay.GetPermit().GetPermitId()
			&& Adopt.GetPermit().GetPermitId()
				!= Fresh.GetPermit().GetPermitId()
			&& Adopt.GetPermit().MatchesSnapshot(
				Fixture.Correlation.ActiveRunId,
				Consumer,
				Commands.Show.GetState(),
				Commands.Show.GetState()));
	TestFalse(TEXT("adoption permit rejects a changed observed cursor"),
		Adopt.GetPermit().MatchesSnapshot(
			Fixture.Correlation.ActiveRunId,
			Consumer,
			Commands.Show.GetState(),
			Commands.Replace.GetState()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewSurfaceRecreationCleanupPermitTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationSurfaceRecreationPolicy.ExplicitCleanupPermits",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewSurfaceRecreationCleanupPermitTest::
	RunTest(const FString&)
{
	using EDisposition = EArcSurfaceRecreationDisposition;
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture Fixture;
	FArcPreviewSurfaceCommandSet Commands;
	if (!BuildArcPreviewSurfaceCommands(
			*this, TEXT("ArcPreviewSurfaceRecreationCleanup"),
			Fixture, Commands))
	{
		return false;
	}
	FThrownLifecycleFixture ForeignFixture;
	FArcPreviewSurfaceCommandSet ForeignCommands;
	check(BuildArcPreviewSurfaceCommands(
		*this, TEXT("ArcPreviewSurfaceRecreationCleanupForeign"),
		ForeignFixture, ForeignCommands));

	const auto Residual = EvaluateArcSurfaceRecreation(
		Fixture, Consumer, Commands.Hide.GetState(), Consumer,
		Commands.Show.GetState(), EArcSurfaceRecreationAction::ClearToEmpty);
	const auto Foreign = EvaluateArcSurfaceRecreation(
		Fixture, Consumer, Commands.Show.GetState(), Consumer,
		ForeignCommands.Show.GetState(),
		EArcSurfaceRecreationAction::ClearToEmpty);
	const auto Conflict = EvaluateArcSurfaceRecreation(
		Fixture, Consumer, Commands.Show.GetState(), Consumer,
		Commands.Replace.GetState(),
		EArcSurfaceRecreationAction::ClearToEmpty);

	TestTrue(TEXT("all visible conflicts require separately authorized cleanup"),
		Residual.IsAuthorized()
			&& Residual.GetDisposition() == EDisposition::ResidualVisible
			&& Foreign.IsAuthorized()
			&& Foreign.GetDisposition() == EDisposition::ForeignVisible
			&& Conflict.IsAuthorized()
			&& Conflict.GetDisposition() == EDisposition::ConflictingVisible);
	TestTrue(TEXT("cleanup decisions issue exact non-binding permits"),
		Residual.GetPermit().IsCleanupPermit()
			&& Foreign.GetPermit().IsCleanupPermit()
			&& Conflict.GetPermit().IsCleanupPermit()
			&& Residual.GetPermit().GetPermitId()
				!= Foreign.GetPermit().GetPermitId()
			&& Foreign.GetPermit().GetPermitId()
				!= Conflict.GetPermit().GetPermitId());
	TestTrue(TEXT("cleanup permit binds the exact observed visible snapshot"),
		Foreign.GetPermit().MatchesSnapshot(
			Fixture.Correlation.ActiveRunId,
			Consumer,
			Commands.Show.GetState(),
			ForeignCommands.Show.GetState()));
	TestFalse(TEXT("cleanup permit cannot clear a subsequently changed surface"),
		Foreign.GetPermit().MatchesSnapshot(
			Fixture.Correlation.ActiveRunId,
			Consumer,
			Commands.Show.GetState(),
			Commands.Show.GetState()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewSurfaceRecreationRehydrateFenceTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationSurfaceRecreationPolicy.RehydrateFence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewSurfaceRecreationRehydrateFenceTest::
	RunTest(const FString&)
{
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture Fixture;
	FArcPreviewSurfaceCommandSet Commands;
	if (!BuildArcPreviewSurfaceCommands(
			*this, TEXT("ArcPreviewSurfaceRecreationRehydrate"),
			Fixture, Commands))
	{
		return false;
	}
	const auto Inspect = EvaluateArcSurfaceRecreation(
		Fixture, Consumer, Commands.Show.GetState(), Consumer,
		FArcSurfaceState(), EArcSurfaceRecreationAction::Inspect);
	const auto Bind = EvaluateArcSurfaceRecreation(
		Fixture, Consumer, Commands.Show.GetState(), Consumer,
		FArcSurfaceState(), EArcSurfaceRecreationAction::BindFresh);
	const auto Adopt = EvaluateArcSurfaceRecreation(
		Fixture, Consumer, Commands.Show.GetState(), Consumer,
		FArcSurfaceState(), EArcSurfaceRecreationAction::AdoptExact);
	const auto Clear = EvaluateArcSurfaceRecreation(
		Fixture, Consumer, Commands.Show.GetState(), Consumer,
		FArcSurfaceState(), EArcSurfaceRecreationAction::ClearToEmpty);

	TestTrue(TEXT("inspection exposes one explicit rehydrate requirement"),
		Inspect.IsValid() && Inspect.IsInspected()
			&& Inspect.NeedsRehydrate() && !Inspect.HasPermit());
	TestTrue(TEXT("policy cannot reinterpret rehydrate as bind adopt or clear"),
		Bind.IsValid() && Bind.IsRejected() && !Bind.HasPermit()
			&& Adopt.IsValid() && Adopt.IsRejected() && !Adopt.HasPermit()
			&& Clear.IsValid() && Clear.IsRejected() && !Clear.HasPermit());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewSurfaceRecreationInvalidInputTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationSurfaceRecreationPolicy.InvalidInputs",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewSurfaceRecreationInvalidInputTest::
	RunTest(const FString&)
{
	using EDisposition = EArcSurfaceRecreationDisposition;
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture Fixture;
	FArcPreviewSurfaceCommandSet Commands;
	if (!BuildArcPreviewSurfaceCommands(
			*this, TEXT("ArcPreviewSurfaceRecreationInvalid"),
			Fixture, Commands))
	{
		return false;
	}
	FThrownLifecycleFixture ForeignFixture;
	FArcPreviewSurfaceCommandSet ForeignCommands;
	check(BuildArcPreviewSurfaceCommands(
		*this, TEXT("ArcPreviewSurfaceRecreationInvalidForeign"),
		ForeignFixture, ForeignCommands));

	const auto InvalidRun = FArcSurfaceRecreationPolicy::Evaluate(
		FGuid(), Consumer, FArcSurfaceState(), Consumer,
		FArcSurfaceState(), EArcSurfaceRecreationAction::Inspect);
	const auto MissingConsumer = EvaluateArcSurfaceRecreation(
		Fixture, NAME_None, FArcSurfaceState(), Consumer,
		FArcSurfaceState(), EArcSurfaceRecreationAction::Inspect);
	const auto WrongAuthorityRun = EvaluateArcSurfaceRecreation(
		Fixture, Consumer, ForeignCommands.Show.GetState(), Consumer,
		FArcSurfaceState(), EArcSurfaceRecreationAction::Inspect);
	const auto HiddenPhysicalCursor = EvaluateArcSurfaceRecreation(
		Fixture, Consumer, Commands.Show.GetState(), Consumer,
		Commands.Hide.GetState(), EArcSurfaceRecreationAction::Inspect);
	const auto ConsumerMismatchAction = EvaluateArcSurfaceRecreation(
		Fixture, Consumer, Commands.Show.GetState(),
		FName(TEXT("Renderer.ArcPreview.Other.r1")),
		Commands.Show.GetState(), EArcSurfaceRecreationAction::AdoptExact);

	TestTrue(TEXT("invalid Run consumer and authority become typed rejections"),
		InvalidRun.IsValid() && InvalidRun.IsRejected()
			&& InvalidRun.GetDisposition() == EDisposition::InputRejected
			&& MissingConsumer.IsValid() && MissingConsumer.IsRejected()
			&& WrongAuthorityRun.IsValid() && WrongAuthorityRun.IsRejected());
	TestTrue(TEXT("physical surface cannot report a hidden audit cursor"),
		HiddenPhysicalCursor.IsValid() && HiddenPhysicalCursor.IsRejected()
			&& HiddenPhysicalCursor.GetDisposition()
				== EDisposition::InputRejected
			&& !HiddenPhysicalCursor.HasPermit());
	TestTrue(TEXT("consumer mismatch cannot authorize an otherwise exact state"),
		ConsumerMismatchAction.IsValid()
			&& ConsumerMismatchAction.IsRejected()
			&& ConsumerMismatchAction.GetDisposition()
				== EDisposition::ConsumerMismatch
			&& !ConsumerMismatchAction.HasPermit());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewSurfaceRecreationActionFenceTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationSurfaceRecreationPolicy.ActionFence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewSurfaceRecreationActionFenceTest::
	RunTest(const FString&)
{
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture Fixture;
	FArcPreviewSurfaceCommandSet Commands;
	if (!BuildArcPreviewSurfaceCommands(
			*this, TEXT("ArcPreviewSurfaceRecreationActionFence"),
			Fixture, Commands))
	{
		return false;
	}
	const auto AdoptEmpty = EvaluateArcSurfaceRecreation(
		Fixture, Consumer, FArcSurfaceState(), Consumer,
		FArcSurfaceState(), EArcSurfaceRecreationAction::AdoptExact);
	const auto BindVisible = EvaluateArcSurfaceRecreation(
		Fixture, Consumer, Commands.Show.GetState(), Consumer,
		Commands.Show.GetState(), EArcSurfaceRecreationAction::BindFresh);
	const auto ClearExact = EvaluateArcSurfaceRecreation(
		Fixture, Consumer, Commands.Show.GetState(), Consumer,
		Commands.Show.GetState(), EArcSurfaceRecreationAction::ClearToEmpty);
	const auto InvalidAction = EvaluateArcSurfaceRecreation(
		Fixture, Consumer, FArcSurfaceState(), Consumer,
		FArcSurfaceState(), EArcSurfaceRecreationAction::Invalid);
	const auto Inspect = EvaluateArcSurfaceRecreation(
		Fixture, Consumer, FArcSurfaceState(), Consumer,
		FArcSurfaceState(), EArcSurfaceRecreationAction::Inspect);

	TestTrue(TEXT("each lifecycle action is fenced to one disposition"),
		AdoptEmpty.IsValid() && AdoptEmpty.IsRejected()
			&& BindVisible.IsValid() && BindVisible.IsRejected()
			&& ClearExact.IsValid() && ClearExact.IsRejected()
			&& InvalidAction.IsValid() && InvalidAction.IsRejected());
	TestTrue(TEXT("rejected and inspect decisions never leak a permit"),
		!AdoptEmpty.HasPermit() && !BindVisible.HasPermit()
			&& !ClearExact.HasPermit() && !InvalidAction.HasPermit()
			&& Inspect.IsInspected() && !Inspect.HasPermit());
	TestTrue(TEXT("decision identity binds the requested action"),
		AdoptEmpty.GetDecisionId() != Inspect.GetDecisionId()
			&& InvalidAction.GetDecisionId() != Inspect.GetDecisionId());
	return true;
}

namespace
{
	using EArcSurfaceLifecycleExecutorStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceLifecycleExecutorStatus;
	using EArcSurfaceLifecycleReceiptOutcome =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceLifecycleReceiptOutcome;
	using EArcSurfaceLifecycleResponseOutcome =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceLifecycleResponseOutcome;
	using FArcSurfaceLifecycleExecutor =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceLifecycleExecutor;
	using FArcSurfaceLifecycleExecutorResult =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceLifecycleExecutorResult;
	using FArcSurfaceLifecyclePermit =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRecreationPermit;
	using FArcSurfaceLifecycleReceipt =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceLifecycleReceipt;
	using FArcSurfaceLifecycleResponse =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceLifecycleResponse;

	FArcSurfaceLifecyclePermit MakeArcSurfaceLifecyclePermit(
		const FThrownLifecycleFixture& Fixture,
		const FName Consumer,
		const FArcSurfaceState& AuthoritativeCursor,
		const FArcSurfaceState& ObservedCursor,
		const EArcSurfaceRecreationAction Action)
	{
		const auto Result = EvaluateArcSurfaceRecreation(
			Fixture,
			Consumer,
			AuthoritativeCursor,
			Consumer,
			ObservedCursor,
			Action);
		return Result.IsAuthorized()
			? Result.GetPermit()
			: FArcSurfaceLifecyclePermit();
	}

	class FFakeArcPreviewSurfaceLifecycle final
		: public Idemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceLifecycle
	{
	public:
		enum class EMode : uint8
		{
			Applied,
			Rejected,
			InvalidResponse,
			AppliedWithoutMutation,
			MutatedThenRejected
		};

		FFakeArcPreviewSurfaceLifecycle(
			const FName InConsumerDefinitionId,
			const FArcSurfaceState& InCursor,
			const EMode InMode = EMode::Applied)
			: ConsumerDefinitionId(InConsumerDefinitionId)
			, Cursor(InCursor)
			, Mode(InMode)
		{
		}

		virtual FName GetConsumerDefinitionId() const override
		{
			return ConsumerDefinitionId;
		}

		virtual FArcSurfaceState GetSurfaceCursor() const override
		{
			return Cursor;
		}

		virtual FArcSurfaceLifecycleResponse ClearToEmpty(
			const FArcSurfaceLifecyclePermit& Permit) override
		{
			++ClearCallCount;
			if (Mode == EMode::InvalidResponse)
			{
				return FArcSurfaceLifecycleResponse();
			}
			const FArcSurfaceState Previous = Cursor;
			EArcSurfaceLifecycleResponseOutcome Outcome =
				EArcSurfaceLifecycleResponseOutcome::Applied;
			FArcSurfaceState ReportedCursor;
			if (Mode == EMode::Applied)
			{
				Cursor = FArcSurfaceState();
			}
			else if (Mode == EMode::Rejected)
			{
				Outcome = EArcSurfaceLifecycleResponseOutcome::Rejected;
				ReportedCursor = Previous;
			}
			else if (Mode == EMode::AppliedWithoutMutation)
			{
				// Reported empty while the physical cursor stays visible.
			}
			else if (Mode == EMode::MutatedThenRejected)
			{
				Cursor = FArcSurfaceState();
				Outcome = EArcSurfaceLifecycleResponseOutcome::Rejected;
				ReportedCursor = Previous;
			}

			FArcSurfaceLifecycleResponse Response;
			FString Diagnostic;
			check(FArcSurfaceLifecycleResponse::TryCreate(
				Permit,
				Outcome,
				Outcome == EArcSurfaceLifecycleResponseOutcome::Applied
					? FName(TEXT("Renderer.ArcPreview.FakeLifecycle.Cleared"))
					: FName(TEXT("Renderer.ArcPreview.FakeLifecycle.Rejected")),
				Previous,
				ReportedCursor,
				Response,
				Diagnostic));
			return Response;
		}

		void SetMode(const EMode Value) { Mode = Value; }
		void SetConsumerDefinitionId(const FName Value)
		{
			ConsumerDefinitionId = Value;
		}
		void ForceCursor(const FArcSurfaceState& Value) { Cursor = Value; }

		int32 ClearCallCount = 0;

	private:
		FName ConsumerDefinitionId = NAME_None;
		FArcSurfaceState Cursor;
		EMode Mode = EMode::Applied;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewSurfaceLifecycleEvidenceTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationSurfaceLifecycleExecutor.EvidenceContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewSurfaceLifecycleEvidenceTest::RunTest(
	const FString&)
{
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture Fixture;
	FArcPreviewSurfaceCommandSet Commands;
	if (!BuildArcPreviewSurfaceCommands(
			*this, TEXT("ArcPreviewSurfaceLifecycleEvidence"),
			Fixture, Commands))
	{
		return false;
	}
	const auto CleanupPermit = MakeArcSurfaceLifecyclePermit(
		Fixture,
		Consumer,
		Commands.Hide.GetState(),
		Commands.Show.GetState(),
		EArcSurfaceRecreationAction::ClearToEmpty);
	const auto BindingPermit = MakeArcSurfaceLifecyclePermit(
		Fixture,
		Consumer,
		FArcSurfaceState(),
		FArcSurfaceState(),
		EArcSurfaceRecreationAction::BindFresh);
	FArcSurfaceLifecycleResponse Applied;
	FArcSurfaceLifecycleResponse AppliedReplay;
	FArcSurfaceLifecycleResponse Rejected;
	FArcSurfaceLifecycleResponse Invalid;
	FString Diagnostic;
	const FName AppliedCode(TEXT("Renderer.ArcPreview.Lifecycle.Cleared"));
	TestTrue(TEXT("cleanup response identity is deterministic"),
		FArcSurfaceLifecycleResponse::TryCreate(
			CleanupPermit,
			EArcSurfaceLifecycleResponseOutcome::Applied,
			AppliedCode,
			Commands.Show.GetState(),
			FArcSurfaceState(),
			Applied,
			Diagnostic)
			&& FArcSurfaceLifecycleResponse::TryCreate(
				CleanupPermit,
				EArcSurfaceLifecycleResponseOutcome::Applied,
				AppliedCode,
				Commands.Show.GetState(),
				FArcSurfaceState(),
				AppliedReplay,
				Diagnostic)
			&& Applied.IsApplied()
			&& Applied.GetResponseId() == AppliedReplay.GetResponseId());
	TestTrue(TEXT("rejected cleanup response preserves exact visible cursor"),
		FArcSurfaceLifecycleResponse::TryCreate(
			CleanupPermit,
			EArcSurfaceLifecycleResponseOutcome::Rejected,
			FName(TEXT("Renderer.ArcPreview.Lifecycle.Rejected")),
			Commands.Show.GetState(),
			Commands.Show.GetState(),
			Rejected,
			Diagnostic)
			&& Rejected.IsRejected());
	TestFalse(TEXT("binding permit cannot forge a cleanup response"),
		FArcSurfaceLifecycleResponse::TryCreate(
			BindingPermit,
			EArcSurfaceLifecycleResponseOutcome::Applied,
			AppliedCode,
			Commands.Show.GetState(),
			FArcSurfaceState(),
			Invalid,
			Diagnostic));

	FArcSurfaceLifecycleReceipt BindingReceipt;
	FArcSurfaceLifecycleReceipt AppliedReceipt;
	TestTrue(TEXT("binding readiness receipt has zero surface calls"),
		FArcSurfaceLifecycleReceipt::TryCreate(
			BindingPermit,
			EArcSurfaceLifecycleReceiptOutcome::BindingReady,
			0,
			FArcSurfaceLifecycleResponse(),
			FArcSurfaceState(),
			FArcSurfaceState(),
			BindingReceipt,
			Diagnostic)
			&& BindingReceipt.IsBindingReady()
			&& BindingReceipt.MatchesPermit(BindingPermit));
	TestTrue(TEXT("cleanup receipt binds one exact applied response"),
		FArcSurfaceLifecycleReceipt::TryCreate(
			CleanupPermit,
			EArcSurfaceLifecycleReceiptOutcome::CleanupApplied,
			1,
			Applied,
			Commands.Show.GetState(),
			FArcSurfaceState(),
			AppliedReceipt,
			Diagnostic)
			&& AppliedReceipt.WasCleanupApplied()
			&& AppliedReceipt.MatchesPermit(CleanupPermit));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewSurfaceLifecycleBindingTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationSurfaceLifecycleExecutor.BindingReady",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewSurfaceLifecycleBindingTest::RunTest(
	const FString&)
{
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture Fixture;
	FArcPreviewSurfaceCommandSet Commands;
	if (!BuildArcPreviewSurfaceCommands(
			*this, TEXT("ArcPreviewSurfaceLifecycleBinding"),
			Fixture, Commands))
	{
		return false;
	}
	const auto FreshPermit = MakeArcSurfaceLifecyclePermit(
		Fixture, Consumer, FArcSurfaceState(), FArcSurfaceState(),
		EArcSurfaceRecreationAction::BindFresh);
	const auto AdoptPermit = MakeArcSurfaceLifecyclePermit(
		Fixture, Consumer, Commands.Show.GetState(),
		Commands.Show.GetState(), EArcSurfaceRecreationAction::AdoptExact);
	FFakeArcPreviewSurfaceLifecycle FreshSurface(
		Consumer, FArcSurfaceState());
	FFakeArcPreviewSurfaceLifecycle AdoptSurface(
		Consumer, Commands.Show.GetState());
	FArcSurfaceLifecycleExecutor Executor;

	const auto Fresh = Executor.Execute(FreshPermit, FreshSurface);
	const auto Adopt = Executor.Execute(AdoptPermit, AdoptSurface);
	const auto AdoptReplay = Executor.Execute(AdoptPermit, AdoptSurface);
	TestTrue(TEXT("fresh and exact permits become zero-call binding receipts"),
		Fresh.IsValid() && Fresh.IsAccepted() && Fresh.IsBindingReady()
			&& !Fresh.DidCallSurface() && Fresh.HasReceipt()
			&& Adopt.IsValid() && Adopt.IsAccepted()
			&& Adopt.IsBindingReady() && !Adopt.DidCallSurface()
			&& FreshSurface.ClearCallCount == 0
			&& AdoptSurface.ClearCallCount == 0);
	TestTrue(TEXT("binding readiness replay is deterministic and inert"),
		AdoptReplay.IsBindingReady()
			&& Adopt.GetReceipt().GetReceiptId()
				== AdoptReplay.GetReceipt().GetReceiptId()
			&& AdoptSurface.GetSurfaceCursor().Matches(
				Commands.Show.GetState()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewSurfaceLifecycleCleanupTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationSurfaceLifecycleExecutor.CleanupApplied",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewSurfaceLifecycleCleanupTest::RunTest(
	const FString&)
{
	using EStatus = EArcSurfaceLifecycleExecutorStatus;
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture Fixture;
	FArcPreviewSurfaceCommandSet Commands;
	if (!BuildArcPreviewSurfaceCommands(
			*this, TEXT("ArcPreviewSurfaceLifecycleCleanup"),
			Fixture, Commands))
	{
		return false;
	}
	const auto Permit = MakeArcSurfaceLifecyclePermit(
		Fixture,
		Consumer,
		Commands.Hide.GetState(),
		Commands.Show.GetState(),
		EArcSurfaceRecreationAction::ClearToEmpty);
	FFakeArcPreviewSurfaceLifecycle Surface(
		Consumer, Commands.Show.GetState());
	FArcSurfaceLifecycleExecutor Executor;

	const auto Cleared = Executor.Execute(Permit, Surface);
	const auto Replay = Executor.Execute(Permit, Surface);
	TestTrue(TEXT("cleanup permit calls the lifecycle surface exactly once"),
		Cleared.IsValid() && Cleared.IsAccepted() && Cleared.DidClear()
			&& Cleared.DidCallSurface()
			&& Cleared.GetSurfaceCallCount() == 1
			&& Surface.ClearCallCount == 1
			&& Surface.GetSurfaceCursor().IsEmpty());
	TestTrue(TEXT("applied cleanup returns an exact receipt and requires reevaluation"),
		Cleared.HasReceipt()
			&& Cleared.GetReceipt().WasCleanupApplied()
			&& Cleared.GetReceipt().MatchesPermit(Permit)
			&& Cleared.NeedsReevaluation());
	TestTrue(TEXT("old cleanup permit cannot replay after cursor changed"),
		Replay.IsValid() && !Replay.IsAccepted()
			&& Replay.GetStatus() == EStatus::SnapshotMismatch
			&& !Replay.DidCallSurface()
			&& Surface.ClearCallCount == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewSurfaceLifecycleRejectionTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationSurfaceLifecycleExecutor.CleanupRejectedRetry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewSurfaceLifecycleRejectionTest::RunTest(
	const FString&)
{
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture Fixture;
	FArcPreviewSurfaceCommandSet Commands;
	if (!BuildArcPreviewSurfaceCommands(
			*this, TEXT("ArcPreviewSurfaceLifecycleRejected"),
			Fixture, Commands))
	{
		return false;
	}
	const auto Permit = MakeArcSurfaceLifecyclePermit(
		Fixture,
		Consumer,
		Commands.Hide.GetState(),
		Commands.Show.GetState(),
		EArcSurfaceRecreationAction::ClearToEmpty);
	FFakeArcPreviewSurfaceLifecycle Surface(
		Consumer,
		Commands.Show.GetState(),
		FFakeArcPreviewSurfaceLifecycle::EMode::Rejected);
	FArcSurfaceLifecycleExecutor Executor;

	const auto Rejected = Executor.Execute(Permit, Surface);
	TestTrue(TEXT("one rejected call preserves cursor and seals rejection"),
		Rejected.IsValid() && !Rejected.IsAccepted()
			&& Rejected.WasSurfaceRejected()
			&& Rejected.DidCallSurface()
			&& Rejected.HasReceipt()
			&& Rejected.GetReceipt().WasCleanupRejected()
			&& Rejected.CanRetryExactPermit()
			&& Surface.GetSurfaceCursor().Matches(
				Commands.Show.GetState())
			&& Surface.ClearCallCount == 1);
	Surface.SetMode(FFakeArcPreviewSurfaceLifecycle::EMode::Applied);
	const auto Retried = Executor.Execute(Permit, Surface);
	TestTrue(TEXT("caller may make one later retry but executor never loops"),
		Retried.IsValid() && Retried.DidClear()
			&& Retried.GetSurfaceCallCount() == 1
			&& Surface.ClearCallCount == 2
			&& Surface.GetSurfaceCursor().IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewSurfaceLifecycleInvariantTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationSurfaceLifecycleExecutor.InvariantFailures",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewSurfaceLifecycleInvariantTest::RunTest(
	const FString&)
{
	using EMode = FFakeArcPreviewSurfaceLifecycle::EMode;
	using EStatus = EArcSurfaceLifecycleExecutorStatus;
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture Fixture;
	FArcPreviewSurfaceCommandSet Commands;
	if (!BuildArcPreviewSurfaceCommands(
			*this, TEXT("ArcPreviewSurfaceLifecycleInvariant"),
			Fixture, Commands))
	{
		return false;
	}
	const auto Permit = MakeArcSurfaceLifecyclePermit(
		Fixture,
		Consumer,
		Commands.Hide.GetState(),
		Commands.Show.GetState(),
		EArcSurfaceRecreationAction::ClearToEmpty);
	FArcSurfaceLifecycleExecutor Executor;

	FFakeArcPreviewSurfaceLifecycle InvalidSurface(
		Consumer, Commands.Show.GetState(), EMode::InvalidResponse);
	const auto Invalid = Executor.Execute(Permit, InvalidSurface);
	TestTrue(TEXT("invalid response fails closed after one surface call"),
		Invalid.IsValid() && !Invalid.IsAccepted()
			&& Invalid.GetStatus() == EStatus::SurfaceResponseInvalid
			&& Invalid.DidCallSurface() && !Invalid.HasReceipt()
			&& InvalidSurface.ClearCallCount == 1);

	FFakeArcPreviewSurfaceLifecycle NoMutationSurface(
		Consumer, Commands.Show.GetState(), EMode::AppliedWithoutMutation);
	const auto NoMutation = Executor.Execute(Permit, NoMutationSurface);
	TestTrue(TEXT("Applied response without physical clear is detected"),
		NoMutation.IsValid() && !NoMutation.IsAccepted()
			&& NoMutation.GetStatus()
				== EStatus::SurfaceInvariantViolation
			&& NoMutation.DidCallSurface() && !NoMutation.HasReceipt()
			&& NoMutationSurface.GetSurfaceCursor().IsVisible());

	FFakeArcPreviewSurfaceLifecycle RejectedMutationSurface(
		Consumer, Commands.Show.GetState(), EMode::MutatedThenRejected);
	const auto RejectedMutation =
		Executor.Execute(Permit, RejectedMutationSurface);
	TestTrue(TEXT("Rejected response with physical clear is detected"),
		RejectedMutation.IsValid() && !RejectedMutation.IsAccepted()
			&& RejectedMutation.GetStatus()
				== EStatus::SurfaceInvariantViolation
			&& RejectedMutation.DidCallSurface()
			&& !RejectedMutation.HasReceipt()
			&& RejectedMutationSurface.GetSurfaceCursor().IsEmpty());
	return true;
}

namespace
{
	class FReentrantArcPreviewSurfaceLifecycle final
		: public Idemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceLifecycle
	{
	public:
		FReentrantArcPreviewSurfaceLifecycle(
			const FName InConsumerDefinitionId,
			const FArcSurfaceState& InCursor,
			FArcSurfaceLifecycleExecutor& InExecutor)
			: ConsumerDefinitionId(InConsumerDefinitionId)
			, Cursor(InCursor)
			, Executor(InExecutor)
		{
		}

		virtual FName GetConsumerDefinitionId() const override
		{
			return ConsumerDefinitionId;
		}
		virtual FArcSurfaceState GetSurfaceCursor() const override
		{
			return Cursor;
		}
		virtual FArcSurfaceLifecycleResponse ClearToEmpty(
			const FArcSurfaceLifecyclePermit& Permit) override
		{
			++ClearCallCount;
			InnerResult = Executor.Execute(Permit, *this);
			const FArcSurfaceState Previous = Cursor;
			Cursor = FArcSurfaceState();
			FArcSurfaceLifecycleResponse Response;
			FString Diagnostic;
			check(FArcSurfaceLifecycleResponse::TryCreate(
				Permit,
				EArcSurfaceLifecycleResponseOutcome::Applied,
				FName(TEXT("Renderer.ArcPreview.ReentrantLifecycle.Cleared")),
				Previous,
				Cursor,
				Response,
				Diagnostic));
			return Response;
		}

		int32 ClearCallCount = 0;
		FArcSurfaceLifecycleExecutorResult InnerResult;

	private:
		FName ConsumerDefinitionId = NAME_None;
		FArcSurfaceState Cursor;
		FArcSurfaceLifecycleExecutor& Executor;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewSurfaceLifecyclePreflightTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationSurfaceLifecycleExecutor.PreflightAndReentrant",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewSurfaceLifecyclePreflightTest::RunTest(
	const FString&)
{
	using EStatus = EArcSurfaceLifecycleExecutorStatus;
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture Fixture;
	FArcPreviewSurfaceCommandSet Commands;
	if (!BuildArcPreviewSurfaceCommands(
			*this, TEXT("ArcPreviewSurfaceLifecyclePreflight"),
			Fixture, Commands))
	{
		return false;
	}
	const auto Permit = MakeArcSurfaceLifecyclePermit(
		Fixture,
		Consumer,
		Commands.Hide.GetState(),
		Commands.Show.GetState(),
		EArcSurfaceRecreationAction::ClearToEmpty);
	FArcSurfaceLifecycleExecutor Executor;
	FFakeArcPreviewSurfaceLifecycle Surface(
		Consumer, Commands.Show.GetState());

	const auto Invalid = Executor.Execute(
		FArcSurfaceLifecyclePermit(), Surface);
	Surface.SetConsumerDefinitionId(
		FName(TEXT("Renderer.ArcPreview.Other.r1")));
	const auto ConsumerMismatch = Executor.Execute(Permit, Surface);
	Surface.SetConsumerDefinitionId(Consumer);
	Surface.ForceCursor(Commands.Replace.GetState());
	const auto SnapshotMismatch = Executor.Execute(Permit, Surface);
	TestTrue(TEXT("invalid permit consumer and snapshot reject before clear"),
		Invalid.IsValid() && Invalid.GetStatus() == EStatus::PermitInvalid
			&& ConsumerMismatch.IsValid()
			&& ConsumerMismatch.GetStatus() == EStatus::ConsumerMismatch
			&& SnapshotMismatch.IsValid()
			&& SnapshotMismatch.GetStatus() == EStatus::SnapshotMismatch
			&& Surface.ClearCallCount == 0);

	FReentrantArcPreviewSurfaceLifecycle ReentrantSurface(
		Consumer, Commands.Show.GetState(), Executor);
	const auto Outer = Executor.Execute(Permit, ReentrantSurface);
	TestTrue(TEXT("outer cleanup remains one exact applied call"),
		Outer.IsValid() && Outer.DidClear()
			&& ReentrantSurface.ClearCallCount == 1
			&& !Executor.IsOperationInProgress());
	TestTrue(TEXT("surface callback cannot re-enter lifecycle executor"),
		ReentrantSurface.InnerResult.IsValid()
			&& ReentrantSurface.InnerResult.GetStatus()
				== EStatus::OperationInProgress
			&& !ReentrantSurface.InnerResult.DidCallSurface()
			&& !ReentrantSurface.InnerResult.HasReceipt()
			&& ReentrantSurface.InnerResult.GetPreviousSurfaceCursor().IsEmpty()
			&& ReentrantSurface.InnerResult.GetSurfaceCursor().IsEmpty());
	return true;
}

namespace
{
	using EArcSurfaceOwnershipStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceOwnershipTransitionStatus;
	using FArcSurfaceOwnershipRequest =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceOwnershipTransitionRequest;
	using FArcSurfaceOwnershipResult =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceOwnershipTransitionResult;
	using FArcSurfaceOwnershipTicket =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceOwnershipTransitionTicket;
	using FArcSurfaceOwnershipTransition =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceOwnershipTransition;

	bool BuildArcSurfaceOwnershipRequest(
		FAutomationTestBase& Test,
		const FThrownLifecycleFixture& Fixture,
		const FName ConsumerDefinitionId,
		const FArcSurfaceState& AuthoritativeCursor,
		const FGuid& SurfaceInstanceId,
		const EArcSurfaceRecreationAction Action,
		FArcSurfaceOwnershipRequest& OutRequest)
	{
		FString Diagnostic;
		if (!FArcSurfaceOwnershipRequest::TryCreate(
				Fixture.Correlation.ActiveRunId,
				ConsumerDefinitionId,
				AuthoritativeCursor,
				SurfaceInstanceId,
				Action,
				OutRequest,
				Diagnostic))
		{
			Test.AddError(FString::Printf(
				TEXT("Could not create Arc preview ownership request: %s"),
				*Diagnostic));
			return false;
		}
		return true;
	}

	class FFakeArcPreviewSurfaceOwnershipCandidate final
		: public Idemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceOwnershipCandidate
	{
	public:
		enum class EMode : uint8
		{
			Applied,
			Rejected
		};

		FFakeArcPreviewSurfaceOwnershipCandidate(
			const FGuid& InSurfaceInstanceId,
			const FName InConsumerDefinitionId,
			const FArcSurfaceState& InCursor,
			const EMode InMode = EMode::Applied)
			: SurfaceInstanceId(InSurfaceInstanceId)
			, ConsumerDefinitionId(InConsumerDefinitionId)
			, Cursor(InCursor)
			, Mode(InMode)
		{
		}

		virtual FGuid GetSurfaceInstanceId() const override
		{
			++IdentityQueryCount;
			if (bReenterOnFirstIdentityQuery && IdentityQueryCount == 1
				&& ReentrantTransition && ReentrantRequest)
			{
				ReentrantResult = ReentrantTransition->Execute(
					*ReentrantRequest,
					const_cast<FFakeArcPreviewSurfaceOwnershipCandidate&>(*this));
			}
			return bDriftIdentityOnSecondQuery && IdentityQueryCount >= 2
				? DriftSurfaceInstanceId
				: SurfaceInstanceId;
		}

		virtual FName GetConsumerDefinitionId() const override
		{
			++ConsumerQueryCount;
			return ConsumerDefinitionId;
		}

		virtual FArcSurfaceState GetSurfaceCursor() const override
		{
			++CursorQueryCount;
			if (bDriftCursorOnSecondQuery && CursorQueryCount >= 2)
			{
				Cursor = DriftCursor;
			}
			return Cursor;
		}

		virtual FArcSurfaceLifecycleResponse ClearToEmpty(
			const FArcSurfaceLifecyclePermit& Permit) override
		{
			++ClearCallCount;
			const FArcSurfaceState Previous = Cursor;
			const bool bApply = Mode == EMode::Applied;
			if (bApply)
			{
				Cursor = FArcSurfaceState();
			}
			FArcSurfaceLifecycleResponse Response;
			FString Diagnostic;
			check(FArcSurfaceLifecycleResponse::TryCreate(
				Permit,
				bApply
					? EArcSurfaceLifecycleResponseOutcome::Applied
					: EArcSurfaceLifecycleResponseOutcome::Rejected,
				bApply
					? FName(TEXT("Renderer.ArcPreview.Ownership.Cleared"))
					: FName(TEXT("Renderer.ArcPreview.Ownership.Rejected")),
				Previous,
				Cursor,
				Response,
				Diagnostic));
			return Response;
		}

		void SetMode(const EMode Value) { Mode = Value; }
		void SetIdentityDrift(
			const FGuid& Value,
			const bool bEnabled = true)
		{
			DriftSurfaceInstanceId = Value;
			bDriftIdentityOnSecondQuery = bEnabled;
		}
		void SetCursorDrift(
			const FArcSurfaceState& Value,
			const bool bEnabled = true)
		{
			DriftCursor = Value;
			bDriftCursorOnSecondQuery = bEnabled;
		}
		void SetReentrantIdentityCallback(
			FArcSurfaceOwnershipTransition& Transition,
			const FArcSurfaceOwnershipRequest& Request)
		{
			ReentrantTransition = &Transition;
			ReentrantRequest = &Request;
			bReenterOnFirstIdentityQuery = true;
		}

		mutable int32 IdentityQueryCount = 0;
		mutable int32 ConsumerQueryCount = 0;
		mutable int32 CursorQueryCount = 0;
		int32 ClearCallCount = 0;
		mutable FArcSurfaceOwnershipResult ReentrantResult;

	private:
		FGuid SurfaceInstanceId;
		FName ConsumerDefinitionId = NAME_None;
		mutable FArcSurfaceState Cursor;
		EMode Mode = EMode::Applied;
		bool bDriftIdentityOnSecondQuery = false;
		FGuid DriftSurfaceInstanceId;
		bool bDriftCursorOnSecondQuery = false;
		FArcSurfaceState DriftCursor;
		bool bReenterOnFirstIdentityQuery = false;
		FArcSurfaceOwnershipTransition* ReentrantTransition = nullptr;
		const FArcSurfaceOwnershipRequest* ReentrantRequest = nullptr;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewSurfaceOwnershipEvidenceTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationSurfaceOwnershipTransition.EvidenceContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewSurfaceOwnershipEvidenceTest::RunTest(
	const FString&)
{
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	const FGuid SurfaceId(
		0xF4600001, 0xF4600002, 0xF4600003, 0xF4600004);
	FThrownLifecycleFixture Fixture;
	FArcPreviewSurfaceCommandSet Commands;
	if (!BuildArcPreviewSurfaceCommands(
			*this, TEXT("ArcPreviewSurfaceOwnershipEvidence"),
			Fixture, Commands))
	{
		return false;
	}
	FArcSurfaceOwnershipRequest Request;
	FArcSurfaceOwnershipRequest Replay;
	FString Diagnostic;
	TestTrue(TEXT("exact ownership request identity is deterministic"),
		FArcSurfaceOwnershipRequest::TryCreate(
			Fixture.Correlation.ActiveRunId,
			Consumer,
			Commands.Show.GetState(),
			SurfaceId,
			EArcSurfaceRecreationAction::AdoptExact,
			Request,
			Diagnostic)
			&& FArcSurfaceOwnershipRequest::TryCreate(
				Fixture.Correlation.ActiveRunId,
				Consumer,
				Commands.Show.GetState(),
				SurfaceId,
				EArcSurfaceRecreationAction::AdoptExact,
				Replay,
				Diagnostic)
			&& Request.IsValid()
			&& Request.GetRequestId() == Replay.GetRequestId());
	FArcSurfaceOwnershipRequest Invalid;
	TestFalse(TEXT("inspect cannot masquerade as an ownership transition"),
		FArcSurfaceOwnershipRequest::TryCreate(
			Fixture.Correlation.ActiveRunId,
			Consumer,
			Commands.Show.GetState(),
			SurfaceId,
			EArcSurfaceRecreationAction::Inspect,
			Invalid,
			Diagnostic));
	FFakeArcPreviewSurfaceOwnershipCandidate Surface(
		SurfaceId, Consumer, Commands.Show.GetState());
	FArcSurfaceOwnershipTransition Transition;
	const auto InvalidResult = Transition.Execute(Invalid, Surface);
	TestTrue(TEXT("invalid request rejects before every candidate callback"),
		InvalidResult.IsValid()
			&& InvalidResult.GetStatus()
				== EArcSurfaceOwnershipStatus::RequestInvalid
			&& Surface.IdentityQueryCount == 0
			&& Surface.ConsumerQueryCount == 0
			&& Surface.CursorQueryCount == 0
			&& Surface.ClearCallCount == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewSurfaceOwnershipBindFreshTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationSurfaceOwnershipTransition.BindFresh",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewSurfaceOwnershipBindFreshTest::RunTest(
	const FString&)
{
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	const FGuid SurfaceId(
		0xF4610001, 0xF4610002, 0xF4610003, 0xF4610004);
	FThrownLifecycleFixture Fixture;
	FArcPreviewSurfaceCommandSet Commands;
	if (!BuildArcPreviewSurfaceCommands(
			*this, TEXT("ArcPreviewSurfaceOwnershipBindFresh"),
			Fixture, Commands))
	{
		return false;
	}
	FArcSurfaceOwnershipRequest Request;
	if (!BuildArcSurfaceOwnershipRequest(
			*this, Fixture, Consumer, FArcSurfaceState(), SurfaceId,
			EArcSurfaceRecreationAction::BindFresh, Request))
	{
		return false;
	}
	FFakeArcPreviewSurfaceOwnershipCandidate Surface(
		SurfaceId, Consumer, FArcSurfaceState());
	FArcSurfaceOwnershipTransition Transition;
	const auto First = Transition.Execute(Request, Surface);
	const auto Replay = Transition.Execute(Request, Surface);
	TestTrue(TEXT("fresh empty surface receives a zero-mutation ownership ticket"),
		First.IsValid() && First.IsAccepted()
			&& First.DidAuthorizeBinding()
			&& First.HasTransitionTicket()
			&& !First.DidCallSurface()
			&& First.GetIdentityQueryCount() == 2
			&& First.GetPolicySnapshotReadCount() == 1
			&& Surface.ClearCallCount == 0);
	TestTrue(TEXT("ticket binds the exact surface identity consumer and cursor"),
		First.GetTransitionTicket().MatchesCandidateSnapshot(
			SurfaceId, Consumer, FArcSurfaceState())
			&& !First.GetTransitionTicket().MatchesCandidateSnapshot(
				FGuid::NewGuid(), Consumer, FArcSurfaceState()));
	TestTrue(TEXT("unchanged binding evidence replays deterministically"),
		Replay.DidAuthorizeBinding()
			&& Replay.GetTransitionTicket().GetTicketId()
				== First.GetTransitionTicket().GetTicketId());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewSurfaceOwnershipAdoptExactTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationSurfaceOwnershipTransition.AdoptExact",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewSurfaceOwnershipAdoptExactTest::RunTest(
	const FString&)
{
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	const FGuid SurfaceId(
		0xF4620001, 0xF4620002, 0xF4620003, 0xF4620004);
	FThrownLifecycleFixture Fixture;
	FArcPreviewSurfaceCommandSet Commands;
	if (!BuildArcPreviewSurfaceCommands(
			*this, TEXT("ArcPreviewSurfaceOwnershipAdoptExact"),
			Fixture, Commands))
	{
		return false;
	}
	FArcSurfaceOwnershipRequest Request;
	if (!BuildArcSurfaceOwnershipRequest(
			*this, Fixture, Consumer, Commands.Show.GetState(), SurfaceId,
			EArcSurfaceRecreationAction::AdoptExact, Request))
	{
		return false;
	}
	FFakeArcPreviewSurfaceOwnershipCandidate Surface(
		SurfaceId, Consumer, Commands.Show.GetState());
	FArcSurfaceOwnershipTransition Transition;
	const auto Result = Transition.Execute(Request, Surface);
	TestTrue(TEXT("exact visible surface receives an inert adoption ticket"),
		Result.IsValid() && Result.DidAuthorizeBinding()
			&& Result.HasTransitionTicket()
			&& !Result.DidCallSurface()
			&& Surface.ClearCallCount == 0
			&& Result.GetTransitionTicket().GetAction()
				== EArcSurfaceRecreationAction::AdoptExact);
	TestTrue(TEXT("adoption ticket rejects cursor and consumer substitution"),
		Result.GetTransitionTicket().MatchesCandidateSnapshot(
			SurfaceId, Consumer, Commands.Show.GetState())
			&& !Result.GetTransitionTicket().MatchesCandidateSnapshot(
				SurfaceId, Consumer, Commands.Replace.GetState())
			&& !Result.GetTransitionTicket().MatchesCandidateSnapshot(
				SurfaceId,
				FName(TEXT("Renderer.ArcPreview.Other.r1")),
				Commands.Show.GetState()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewSurfaceOwnershipCleanupTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationSurfaceOwnershipTransition.CleanupOutcomes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewSurfaceOwnershipCleanupTest::RunTest(
	const FString&)
{
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	const FGuid AppliedSurfaceId(
		0xF4630001, 0xF4630002, 0xF4630003, 0xF4630004);
	const FGuid RejectedSurfaceId(
		0xF4631001, 0xF4631002, 0xF4631003, 0xF4631004);
	FThrownLifecycleFixture Fixture;
	FArcPreviewSurfaceCommandSet Commands;
	if (!BuildArcPreviewSurfaceCommands(
			*this, TEXT("ArcPreviewSurfaceOwnershipCleanup"),
			Fixture, Commands))
	{
		return false;
	}
	FArcSurfaceOwnershipRequest AppliedRequest;
	FArcSurfaceOwnershipRequest RejectedRequest;
	if (!BuildArcSurfaceOwnershipRequest(
			*this, Fixture, Consumer, Commands.Hide.GetState(),
			AppliedSurfaceId, EArcSurfaceRecreationAction::ClearToEmpty,
			AppliedRequest)
		|| !BuildArcSurfaceOwnershipRequest(
			*this, Fixture, Consumer, Commands.Hide.GetState(),
			RejectedSurfaceId, EArcSurfaceRecreationAction::ClearToEmpty,
			RejectedRequest))
	{
		return false;
	}
	FFakeArcPreviewSurfaceOwnershipCandidate AppliedSurface(
		AppliedSurfaceId, Consumer, Commands.Show.GetState());
	FArcSurfaceOwnershipTransition AppliedTransition;
	const auto Applied = AppliedTransition.Execute(
		AppliedRequest, AppliedSurface);
	const auto StaleReplay = AppliedTransition.Execute(
		AppliedRequest, AppliedSurface);
	TestTrue(TEXT("applied cleanup emits no binding ticket and forces reevaluation"),
		Applied.IsValid() && Applied.IsAccepted() && Applied.DidClear()
			&& Applied.DidCallSurface() && !Applied.HasTransitionTicket()
			&& Applied.NeedsReevaluation()
			&& AppliedSurface.ClearCallCount == 1
			&& StaleReplay.IsValid()
			&& StaleReplay.GetStatus()
				== EArcSurfaceOwnershipStatus::PolicyRejected
			&& AppliedSurface.ClearCallCount == 1);

	FFakeArcPreviewSurfaceOwnershipCandidate RejectedSurface(
		RejectedSurfaceId,
		Consumer,
		Commands.Show.GetState(),
		FFakeArcPreviewSurfaceOwnershipCandidate::EMode::Rejected);
	FArcSurfaceOwnershipTransition RejectedTransition;
	const auto Rejected = RejectedTransition.Execute(
		RejectedRequest, RejectedSurface);
	RejectedSurface.SetMode(
		FFakeArcPreviewSurfaceOwnershipCandidate::EMode::Applied);
	const auto Retried = RejectedTransition.Execute(
		RejectedRequest, RejectedSurface);
	TestTrue(TEXT("rejected cleanup preserves exact bounded retry authority"),
		Rejected.IsValid() && Rejected.WasCleanupRejected()
			&& Rejected.CanRetryExactRequest()
			&& !Rejected.HasTransitionTicket()
			&& Retried.IsValid() && Retried.DidClear()
			&& RejectedSurface.ClearCallCount == 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewSurfaceOwnershipFenceTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationSurfaceOwnershipTransition.IdentityAndPolicyFences",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewSurfaceOwnershipFenceTest::RunTest(
	const FString&)
{
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	const FGuid SurfaceId(
		0xF4640001, 0xF4640002, 0xF4640003, 0xF4640004);
	FThrownLifecycleFixture Fixture;
	FArcPreviewSurfaceCommandSet Commands;
	if (!BuildArcPreviewSurfaceCommands(
			*this, TEXT("ArcPreviewSurfaceOwnershipFence"),
			Fixture, Commands))
	{
		return false;
	}
	FArcSurfaceOwnershipRequest FreshRequest;
	FArcSurfaceOwnershipRequest RehydrateRequest;
	if (!BuildArcSurfaceOwnershipRequest(
			*this, Fixture, Consumer, FArcSurfaceState(), SurfaceId,
			EArcSurfaceRecreationAction::BindFresh, FreshRequest)
		|| !BuildArcSurfaceOwnershipRequest(
			*this, Fixture, Consumer, Commands.Show.GetState(), SurfaceId,
			EArcSurfaceRecreationAction::BindFresh, RehydrateRequest))
	{
		return false;
	}
	FFakeArcPreviewSurfaceOwnershipCandidate WrongIdentity(
		FGuid::NewGuid(), Consumer, FArcSurfaceState());
	FArcSurfaceOwnershipTransition Transition;
	const auto IdentityRejected = Transition.Execute(
		FreshRequest, WrongIdentity);
	TestTrue(TEXT("surface instance mismatch rejects before policy reads"),
		IdentityRejected.IsValid()
			&& IdentityRejected.GetStatus()
				== EArcSurfaceOwnershipStatus::SurfaceIdentityMismatch
			&& IdentityRejected.GetPolicySnapshotReadCount() == 0
			&& WrongIdentity.ConsumerQueryCount == 0
			&& WrongIdentity.CursorQueryCount == 0
			&& WrongIdentity.ClearCallCount == 0);

	FFakeArcPreviewSurfaceOwnershipCandidate EmptySurface(
		SurfaceId, Consumer, FArcSurfaceState());
	const auto RehydrateRejected = Transition.Execute(
		RehydrateRequest, EmptySurface);
	TestTrue(TEXT("empty-to-visible rehydrate remains outside ownership scope"),
		RehydrateRejected.IsValid()
			&& RehydrateRejected.GetStatus()
				== EArcSurfaceOwnershipStatus::PolicyRejected
			&& RehydrateRejected.GetPolicyResult().NeedsRehydrate()
			&& !RehydrateRejected.HasTransitionTicket()
			&& EmptySurface.ClearCallCount == 0);

	FFakeArcPreviewSurfaceOwnershipCandidate DriftedCursor(
		SurfaceId, Consumer, FArcSurfaceState());
	DriftedCursor.SetCursorDrift(Commands.Show.GetState());
	const auto LifecycleRejected = Transition.Execute(
		FreshRequest, DriftedCursor);
	TestTrue(TEXT("cursor drift between policy and lifecycle fails closed"),
		LifecycleRejected.IsValid()
			&& LifecycleRejected.GetStatus()
				== EArcSurfaceOwnershipStatus::LifecycleRejected
			&& LifecycleRejected.GetLifecycleResult().GetStatus()
				== EArcSurfaceLifecycleExecutorStatus::SnapshotMismatch
			&& !LifecycleRejected.HasTransitionTicket()
			&& DriftedCursor.ClearCallCount == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewSurfaceOwnershipReentrantTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationSurfaceOwnershipTransition.IdentityDriftAndReentrant",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewSurfaceOwnershipReentrantTest::RunTest(
	const FString&)
{
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	const FGuid SurfaceId(
		0xF4650001, 0xF4650002, 0xF4650003, 0xF4650004);
	FThrownLifecycleFixture Fixture;
	FArcPreviewSurfaceCommandSet Commands;
	if (!BuildArcPreviewSurfaceCommands(
			*this, TEXT("ArcPreviewSurfaceOwnershipReentrant"),
			Fixture, Commands))
	{
		return false;
	}
	FArcSurfaceOwnershipRequest Request;
	if (!BuildArcSurfaceOwnershipRequest(
			*this, Fixture, Consumer, FArcSurfaceState(), SurfaceId,
			EArcSurfaceRecreationAction::BindFresh, Request))
	{
		return false;
	}
	FFakeArcPreviewSurfaceOwnershipCandidate IdentityDrift(
		SurfaceId, Consumer, FArcSurfaceState());
	IdentityDrift.SetIdentityDrift(FGuid::NewGuid());
	FArcSurfaceOwnershipTransition DriftTransition;
	const auto Drifted = DriftTransition.Execute(Request, IdentityDrift);
	TestTrue(TEXT("identity drift after lifecycle evidence suppresses ticket"),
		Drifted.IsValid()
			&& Drifted.GetStatus()
				== EArcSurfaceOwnershipStatus::SurfaceIdentityDrift
			&& !Drifted.HasTransitionTicket()
			&& !DriftTransition.IsOperationInProgress());

	FFakeArcPreviewSurfaceOwnershipCandidate Reentrant(
		SurfaceId, Consumer, FArcSurfaceState());
	FArcSurfaceOwnershipTransition ReentrantTransition;
	Reentrant.SetReentrantIdentityCallback(ReentrantTransition, Request);
	const auto Outer = ReentrantTransition.Execute(Request, Reentrant);
	TestTrue(TEXT("outer ownership transaction completes after guarded callback"),
		Outer.IsValid() && Outer.DidAuthorizeBinding()
			&& Reentrant.IdentityQueryCount == 2
			&& !ReentrantTransition.IsOperationInProgress());
	TestTrue(TEXT("first identity callback cannot re-enter transaction"),
		Reentrant.ReentrantResult.IsValid()
			&& Reentrant.ReentrantResult.GetStatus()
				== EArcSurfaceOwnershipStatus::OperationInProgress
			&& Reentrant.ReentrantResult.GetIdentityQueryCount() == 0
			&& Reentrant.ReentrantResult.GetPolicySnapshotReadCount() == 0
			&& !Reentrant.ReentrantResult.HasTransitionTicket());
	return true;
}

namespace
{
	using EArcOwnerHandoffStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffStatus;
	using EArcRetirementOutcome =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRetirementOutcome;
	using FArcOwnerHandoff =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoff;
	using FArcOwnerHandoffResult =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffResult;
	using EArcOwnerHandoffRecoveryStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryStatus;
	using FArcOwnerHandoffRecovery =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecovery;
	using FArcOwnerHandoffRecoveryCheckpoint =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpoint;
	using FArcOwnerHandoffRecoveryResult =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryResult;
	using FArcOwnerHandoffRecoveryAdmissionRequest =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryAdmissionRequest;
	using EArcOwnerHandoffRecoveryAdmissionStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryAdmissionStatus;
	using FArcOwnerHandoffRecoveryAdmissionResult =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryAdmissionResult;
	using FArcOwnerHandoffRecoveryAdmissionSession =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryAdmissionSession;
	using FArcOwnerHandoffRecoveryCompletionRequest =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionRequest;
	using FArcOwnerHandoffRecoveryCompletion =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletion;
	using FArcOwnerHandoffRecoveryCompletionCodec =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionCodec;
	using EArcOwnerHandoffRecoveryCompletionDecodeStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionDecodeStatus;
	using FArcOwnerHandoffRecoveryCompletionStorageContext =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionStorageContext;
	using FArcOwnerHandoffRecoveryCompletionStorageAdapter =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionStorageAdapter;
	using EArcOwnerHandoffRecoveryCompletionStorageSaveStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionStorageSaveStatus;
	using EArcOwnerHandoffRecoveryCompletionStorageLoadStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionStorageLoadStatus;
	using FArcOwnerHandoffRecoveryCompletionSession =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionSession;
	using FArcOwnerHandoffRecoveryCompletionSessionResult =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionSessionResult;
	using EArcOwnerHandoffRecoveryCompletionSessionStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionSessionStatus;
	using FArcOwnerHandoffRecoveryTerminalAdoptionRequest =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryTerminalAdoptionRequest;
	using FArcOwnerHandoffRecoveryTerminalAdoption =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryTerminalAdoption;
	using FArcOwnerHandoffRecoveryTerminalAdoptionSession =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryTerminalAdoptionSession;
	using FArcOwnerHandoffRecoveryTerminalAdoptionSessionResult =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryTerminalAdoptionSessionResult;
	using EArcOwnerHandoffRecoveryTerminalAdoptionSessionStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryTerminalAdoptionSessionStatus;
	using FArcOwnerHandoffRecoveryNextGenerationRotationRequest =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryNextGenerationRotationRequest;
	using FArcOwnerHandoffRecoveryNextGenerationRotation =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryNextGenerationRotation;
	using FArcOwnerHandoffRecoveryNextGenerationRotationSession =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryNextGenerationRotationSession;
	using FArcOwnerHandoffRecoveryNextGenerationRotationSessionResult =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryNextGenerationRotationSessionResult;
	using EArcOwnerHandoffRecoveryNextGenerationRotationSessionStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryNextGenerationRotationSessionStatus;
	using FArcOwnerHandoffRecoveryMultiGenerationCycleProof =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryMultiGenerationCycleProof;
	using EArcOwnerHandoffRecoveryJournalKind =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalRecordKind;
	using EArcOwnerHandoffRecoveryJournalDisposition =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalDisposition;
	using EArcOwnerHandoffRecoveryJournalAppendStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalAppendStatus;
	using EArcOwnerHandoffRecoveryJournalDecodeStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalDecodeStatus;
	using FArcOwnerHandoffRecoveryJournalRecord =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalRecord;
	using FArcOwnerHandoffRecoveryJournal =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournal;
	using FArcOwnerHandoffRecoveryJournalCodec =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalCodec;
	using FArcOwnerHandoffRecoveryPayloadEnvelope =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadEnvelope;
	using FArcOwnerHandoffRecoveryPayloadCodec =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadEnvelopeCodec;
	using EArcOwnerHandoffRecoveryPayloadDecodeStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadEnvelopeDecodeStatus;
	using FArcOwnerHandoffRecoveryBundle =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundle;
	using FArcOwnerHandoffRecoveryBundleCodec =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleCodec;
	using EArcOwnerHandoffRecoveryBundleDecodeStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleDecodeStatus;
	using FArcOwnerHandoffRecoveryBundleStorageContext =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorageContext;
	using FArcOwnerHandoffRecoveryBundleStorageAdapter =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorageAdapter;
	using FArcOwnerHandoffRecoveryBundleLocalFileSystem =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleLocalFileSystem;
	using EArcOwnerHandoffRecoveryBundleStorageSaveStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorageSaveStatus;
	using EArcOwnerHandoffRecoveryBundleStorageLoadStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorageLoadStatus;
	using FArcOwnerHandoffRecoveryBundleWatermarkAdvanceRequest =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAdvanceRequest;
	using FArcOwnerHandoffRecoveryBundleWatermarkAdvanceReceipt =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAdvanceReceipt;
	using FArcOwnerHandoffRecoveryBundleWatermarkState =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkState;
	using FArcOwnerHandoffRecoveryBundleWatermarkReadResult =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkReadResult;
	using EArcOwnerHandoffRecoveryBundleWatermarkReadStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkReadStatus;
	using FArcOwnerHandoffRecoveryBundleWatermarkAdvanceResult =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAdvanceResult;
	using EArcOwnerHandoffRecoveryBundleWatermarkAdvanceStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAdvanceStatus;
	using IArcOwnerHandoffRecoveryBundleWatermarkAuthority =
		Idemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAuthority;
	using FArcOwnerHandoffRecoveryBundleWatermarkCommitCoordinator =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkCommitCoordinator;
	using EArcOwnerHandoffRecoveryBundleWatermarkCommitStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkCommitStatus;
	using FArcOwnerHandoffRecoveryPayloadStorageContext =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadStorageContext;
	using FArcOwnerHandoffRecoveryPayloadStorageAdapter =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadStorageAdapter;
	using FArcOwnerHandoffRecoveryPayloadLocalFileSystem =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadLocalFileSystem;
	using IArcOwnerHandoffRecoveryPayloadStorageFileSystem =
		Idemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadStorageFileSystem;
	using EArcOwnerHandoffRecoveryPayloadStorageFileWriteStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadStorageFileWriteStatus;
	using EArcOwnerHandoffRecoveryPayloadStorageFileReadStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadStorageFileReadStatus;
	using EArcOwnerHandoffRecoveryPayloadStorageSaveStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadStorageSaveStatus;
	using EArcOwnerHandoffRecoveryPayloadStorageLoadStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadStorageLoadStatus;
	using FArcRetirementResponse =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRetirementResponse;

	class FFakeArcOwnerHandoffRecoveryPayloadStorageFileSystem final
		: public IArcOwnerHandoffRecoveryPayloadStorageFileSystem
	{
	public:
		enum class EFailure : uint8
		{
			None,
			CreateDirectory,
			DeleteTemporary,
			OpenTemporary,
			WriteTemporary,
			FlushTemporary,
			ReadTemporary,
			CorruptTemporaryRead,
			AtomicReplace,
			ReadPrimary,
			CorruptPrimaryRead
		};

		virtual bool DirectoryExists(const FString&) const override
		{
			++DirectoryQueryCount;
			return bDirectoryExists;
		}

		virtual bool CreateDirectoryTree(const FString&) override
		{
			++CreateDirectoryCount;
			if (Failure == EFailure::CreateDirectory)
			{
				return false;
			}
			bDirectoryExists = true;
			return true;
		}

		virtual bool FileExists(const FString& Path) const override
		{
			++FileExistsQueryCount;
			return Files.Contains(Path);
		}

		virtual bool DeleteFile(const FString& Path) override
		{
			++DeleteCount;
			if (Failure == EFailure::DeleteTemporary)
			{
				return false;
			}
			Files.Remove(Path);
			return true;
		}

		virtual EArcOwnerHandoffRecoveryPayloadStorageFileWriteStatus
		WriteAndFlush(
			const FString& Path,
			const TArray<uint8>& Bytes) override
		{
			++WriteCount;
			if (Failure == EFailure::OpenTemporary)
			{
				return EArcOwnerHandoffRecoveryPayloadStorageFileWriteStatus::
					OpenFailed;
			}
			if (Failure == EFailure::WriteTemporary)
			{
				Files.Add(Path, TArray<uint8>({0x5a}));
				return EArcOwnerHandoffRecoveryPayloadStorageFileWriteStatus::
					WriteFailed;
			}
			Files.Add(Path, Bytes);
			if (Failure == EFailure::FlushTemporary)
			{
				return EArcOwnerHandoffRecoveryPayloadStorageFileWriteStatus::
					FlushFailed;
			}
			return EArcOwnerHandoffRecoveryPayloadStorageFileWriteStatus::
				WrittenAndFlushed;
		}

		virtual EArcOwnerHandoffRecoveryPayloadStorageFileReadStatus
		ReadBounded(
			const FString& Path,
			const int64 MaximumBytes,
			TArray<uint8>& OutBytes,
			int64& OutObservedSize) const override
		{
			++ReadCount;
			const bool bTemporary = Path.EndsWith(TEXT(".tmp"));
			if ((bTemporary && Failure == EFailure::ReadTemporary)
				|| (!bTemporary && Failure == EFailure::ReadPrimary))
			{
				OutBytes.Reset();
				OutObservedSize = INDEX_NONE;
				return EArcOwnerHandoffRecoveryPayloadStorageFileReadStatus::
					Failed;
			}
			const TArray<uint8>* Stored = Files.Find(Path);
			if (!Stored)
			{
				OutBytes.Reset();
				OutObservedSize = INDEX_NONE;
				return EArcOwnerHandoffRecoveryPayloadStorageFileReadStatus::
					Missing;
			}
			OutObservedSize = Stored->Num();
			if (OutObservedSize > MaximumBytes)
			{
				OutBytes.Reset();
				return EArcOwnerHandoffRecoveryPayloadStorageFileReadStatus::
					TooLarge;
			}
			OutBytes = *Stored;
			if (!OutBytes.IsEmpty()
				&& ((bTemporary
						&& Failure == EFailure::CorruptTemporaryRead)
					|| (!bTemporary
						&& Failure == EFailure::CorruptPrimaryRead)))
			{
				OutBytes[0] ^= 0x01;
			}
			return EArcOwnerHandoffRecoveryPayloadStorageFileReadStatus::Read;
		}

		virtual bool AtomicReplace(
			const FString& DestinationPath,
			const FString& SourcePath) override
		{
			++AtomicReplaceCount;
			if (Failure == EFailure::AtomicReplace)
			{
				return false;
			}
			const TArray<uint8>* Source = Files.Find(SourcePath);
			if (!Source)
			{
				return false;
			}
			const TArray<uint8> Copy = *Source;
			Files.Add(DestinationPath, Copy);
			Files.Remove(SourcePath);
			return true;
		}

		void SetFailure(const EFailure Value) { Failure = Value; }
		void SetDirectoryExists(const bool Value)
		{
			bDirectoryExists = Value;
		}
		void SetFile(const FString& Path, const TArray<uint8>& Bytes)
		{
			Files.Add(Path, Bytes);
		}
		bool TryGetFile(const FString& Path, TArray<uint8>& OutBytes) const
		{
			const TArray<uint8>* Value = Files.Find(Path);
			if (!Value)
			{
				OutBytes.Reset();
				return false;
			}
			OutBytes = *Value;
			return true;
		}

		mutable int32 DirectoryQueryCount = 0;
		mutable int32 FileExistsQueryCount = 0;
		int32 CreateDirectoryCount = 0;
		int32 DeleteCount = 0;
		int32 WriteCount = 0;
		mutable int32 ReadCount = 0;
		int32 AtomicReplaceCount = 0;

	private:
		bool bDirectoryExists = false;
		EFailure Failure = EFailure::None;
		TMap<FString, TArray<uint8>> Files;
	};

	class FFakeArcOwnerHandoffRecoveryBundleWatermarkAuthority final
		: public IArcOwnerHandoffRecoveryBundleWatermarkAuthority
	{
	public:
		enum class EReadMode : uint8
		{
			Normal,
			Rejected,
			Unavailable,
			Invalid
		};

		enum class EAdvanceMode : uint8
		{
			Normal,
			Conflict,
			Rejected,
			Unavailable,
			OutcomeUnknownBeforeCommit,
			OutcomeUnknownAfterCommit,
			Invalid
		};

		virtual FArcOwnerHandoffRecoveryBundleWatermarkReadResult Read(
			const FGuid& AuthorityDomainId,
			const FGuid& LineageId) const override
		{
			++ReadCount;
			LastReadAuthorityDomainId = AuthorityDomainId;
			LastReadLineageId = LineageId;
			if (ReadCallback)
			{
				TFunction<void()> Callback = MoveTemp(ReadCallback);
				Callback();
			}
			switch (ReadMode)
			{
			case EReadMode::Rejected:
				return FArcOwnerHandoffRecoveryBundleWatermarkReadResult::
					Failure(
						EArcOwnerHandoffRecoveryBundleWatermarkReadStatus::
							Rejected,
						TEXT("Fake authority rejected the read."));
			case EReadMode::Unavailable:
				return FArcOwnerHandoffRecoveryBundleWatermarkReadResult::
					Failure(
						EArcOwnerHandoffRecoveryBundleWatermarkReadStatus::
							Unavailable,
						TEXT("Fake authority is unavailable."));
			case EReadMode::Invalid:
				return FArcOwnerHandoffRecoveryBundleWatermarkReadResult();
			default:
				return bHasState
					? FArcOwnerHandoffRecoveryBundleWatermarkReadResult::
						Current(
							State,
							TEXT("Fake authority returned current state."))
					: FArcOwnerHandoffRecoveryBundleWatermarkReadResult::
						Missing(
							TEXT("Fake authority has no watermark."));
			}
		}

		virtual FArcOwnerHandoffRecoveryBundleWatermarkAdvanceResult
		CompareAndAdvance(
			const FArcOwnerHandoffRecoveryBundleWatermarkAdvanceRequest&
				Request) override
		{
			++AdvanceCount;
			LastRequest = Request;
			bPreAdvanceProbePassed =
				!PreAdvanceProbe || PreAdvanceProbe();
			if (!Request.IsValid() || !bPreAdvanceProbePassed)
			{
				return FArcOwnerHandoffRecoveryBundleWatermarkAdvanceResult::
					Failure(
						EArcOwnerHandoffRecoveryBundleWatermarkAdvanceStatus::
							Rejected,
						TEXT(
							"Fake authority observed invalid input or missing committed bundle."));
			}
			switch (AdvanceMode)
			{
			case EAdvanceMode::Conflict:
				return FArcOwnerHandoffRecoveryBundleWatermarkAdvanceResult::
					Failure(
						EArcOwnerHandoffRecoveryBundleWatermarkAdvanceStatus::
							Conflict,
						TEXT("Fake compare-and-advance conflict."));
			case EAdvanceMode::Rejected:
				return FArcOwnerHandoffRecoveryBundleWatermarkAdvanceResult::
					Failure(
						EArcOwnerHandoffRecoveryBundleWatermarkAdvanceStatus::
							Rejected,
						TEXT("Fake authority rejected the advance."));
			case EAdvanceMode::Unavailable:
				return FArcOwnerHandoffRecoveryBundleWatermarkAdvanceResult::
					Failure(
						EArcOwnerHandoffRecoveryBundleWatermarkAdvanceStatus::
							Unavailable,
						TEXT("Fake authority could not advance."));
			case EAdvanceMode::OutcomeUnknownBeforeCommit:
				return FArcOwnerHandoffRecoveryBundleWatermarkAdvanceResult::
					Unknown(TEXT(
						"Fake authority outcome is unknown before commit."));
			case EAdvanceMode::Invalid:
				return FArcOwnerHandoffRecoveryBundleWatermarkAdvanceResult();
			default:
				break;
			}

			if (bHasState)
			{
				if (State.MatchesRequest(Request))
				{
					return FArcOwnerHandoffRecoveryBundleWatermarkAdvanceResult::
						Committed(
							EArcOwnerHandoffRecoveryBundleWatermarkAdvanceStatus::
								AlreadyCurrent,
							State.GetLastReceipt(),
							State,
							TEXT(
								"Fake authority replayed the exact advance."));
				}
				if (State.GetAuthorityDomainId()
						!= Request.GetAuthorityDomainId()
					|| State.GetLineageId() != Request.GetLineageId()
					|| State.GetGeneration()
						!= Request.GetExpectedGeneration())
				{
					return FArcOwnerHandoffRecoveryBundleWatermarkAdvanceResult::
						Failure(
							EArcOwnerHandoffRecoveryBundleWatermarkAdvanceStatus::
								Conflict,
							TEXT(
								"Fake authority compare generation did not match."));
				}
			}
			else if (Request.GetExpectedGeneration() != 0)
			{
				return FArcOwnerHandoffRecoveryBundleWatermarkAdvanceResult::
					Failure(
						EArcOwnerHandoffRecoveryBundleWatermarkAdvanceStatus::
							Conflict,
						TEXT(
							"Fake authority expected a missing initial watermark."));
			}

			FArcOwnerHandoffRecoveryBundleWatermarkAdvanceReceipt Receipt;
			FArcOwnerHandoffRecoveryBundleWatermarkState NextState;
			if (!FArcOwnerHandoffRecoveryBundleWatermarkAdvanceReceipt::
					TryCreate(Request, Receipt)
				|| !FArcOwnerHandoffRecoveryBundleWatermarkState::TryCreate(
					Receipt, NextState))
			{
				return FArcOwnerHandoffRecoveryBundleWatermarkAdvanceResult::
					Failure(
						EArcOwnerHandoffRecoveryBundleWatermarkAdvanceStatus::
							Rejected,
						TEXT(
							"Fake authority could not construct committed evidence."));
			}
			State = NextState;
			bHasState = true;
			if (AdvanceMode == EAdvanceMode::OutcomeUnknownAfterCommit)
			{
				return FArcOwnerHandoffRecoveryBundleWatermarkAdvanceResult::
					Unknown(TEXT(
						"Fake authority committed before losing its response."));
			}
			return FArcOwnerHandoffRecoveryBundleWatermarkAdvanceResult::
				Committed(
					EArcOwnerHandoffRecoveryBundleWatermarkAdvanceStatus::
						Advanced,
					Receipt,
					State,
					TEXT("Fake authority advanced."));
		}

		void SetReadMode(const EReadMode Value) { ReadMode = Value; }
		void SetAdvanceMode(const EAdvanceMode Value)
		{
			AdvanceMode = Value;
		}
		void SetPreAdvanceProbe(TFunction<bool()> Value)
		{
			PreAdvanceProbe = MoveTemp(Value);
		}
		void SetReadCallback(TFunction<void()> Value) const
		{
			ReadCallback = MoveTemp(Value);
		}
		void SetState(
			const FArcOwnerHandoffRecoveryBundleWatermarkState& Value)
		{
			State = Value;
			bHasState = Value.IsValid();
		}
		void ClearState()
		{
			State = {};
			bHasState = false;
		}
		bool HasState() const { return bHasState; }
		const FArcOwnerHandoffRecoveryBundleWatermarkState& GetState() const
		{
			return State;
		}

		mutable int32 ReadCount = 0;
		mutable FGuid LastReadAuthorityDomainId;
		mutable FGuid LastReadLineageId;
		int32 AdvanceCount = 0;
		bool bPreAdvanceProbePassed = false;
		FArcOwnerHandoffRecoveryBundleWatermarkAdvanceRequest LastRequest;

	private:
		EReadMode ReadMode = EReadMode::Normal;
		EAdvanceMode AdvanceMode = EAdvanceMode::Normal;
		bool bHasState = false;
		FArcOwnerHandoffRecoveryBundleWatermarkState State;
		TFunction<bool()> PreAdvanceProbe;
		mutable TFunction<void()> ReadCallback;
	};

	class FFakeArcOwnerHandoffRecoveryCompletionAuthority final
		: public IArcOwnerHandoffRecoveryBundleWatermarkAuthority
	{
	public:
		FFakeArcOwnerHandoffRecoveryCompletionAuthority(
			const FGuid& InPendingDomainId,
			const FGuid& InCompletionDomainId)
			: PendingDomainId(InPendingDomainId)
			, CompletionDomainId(InCompletionDomainId)
		{
		}

		virtual FArcOwnerHandoffRecoveryBundleWatermarkReadResult Read(
			const FGuid& AuthorityDomainId,
			const FGuid& LineageId) const override
		{
			if (AuthorityDomainId == PendingDomainId)
			{
				return Pending.Read(AuthorityDomainId, LineageId);
			}
			if (AuthorityDomainId == CompletionDomainId)
			{
				return Completion.Read(AuthorityDomainId, LineageId);
			}
			return FArcOwnerHandoffRecoveryBundleWatermarkReadResult::
				Failure(
					EArcOwnerHandoffRecoveryBundleWatermarkReadStatus::
						Rejected,
					TEXT("Fake completion authority rejected an unknown domain."));
		}

		virtual FArcOwnerHandoffRecoveryBundleWatermarkAdvanceResult
		CompareAndAdvance(
			const FArcOwnerHandoffRecoveryBundleWatermarkAdvanceRequest&
				Request) override
		{
			if (Request.GetAuthorityDomainId() == PendingDomainId)
			{
				return Pending.CompareAndAdvance(Request);
			}
			if (Request.GetAuthorityDomainId() == CompletionDomainId)
			{
				return Completion.CompareAndAdvance(Request);
			}
			return FArcOwnerHandoffRecoveryBundleWatermarkAdvanceResult::
				Failure(
					EArcOwnerHandoffRecoveryBundleWatermarkAdvanceStatus::
						Rejected,
					TEXT("Fake completion authority rejected an unknown domain."));
		}

		FGuid PendingDomainId;
		FGuid CompletionDomainId;
		mutable FFakeArcOwnerHandoffRecoveryBundleWatermarkAuthority Pending;
		mutable FFakeArcOwnerHandoffRecoveryBundleWatermarkAuthority Completion;
	};

	class FFakeArcPreviewHandoffSurface final
		: public Idemo_mapShanmenThrownWeaponArcPreviewPresentationHandoffSurface
	{
	public:
		enum class ERetirementMode : uint8
		{
			Applied,
			Rejected,
			AppliedWithoutMutation,
			InvalidAfterMutation
		};

		FFakeArcPreviewHandoffSurface(
			const FGuid& InSurfaceInstanceId,
			const FName InConsumerDefinitionId,
			const FArcSurfaceState& InCursor = FArcSurfaceState())
			: SurfaceInstanceId(InSurfaceInstanceId)
			, ConsumerDefinitionId(InConsumerDefinitionId)
			, Cursor(InCursor)
		{
		}

		virtual FGuid GetSurfaceInstanceId() const override
		{
			++IdentityQueryCount;
			if (bReenterRecoveryOnNextIdentityQuery
				&& ReentrantRecoveryCoordinator && ReentrantRecoveryOwner
				&& ReentrantRecoveryCheckpoint && ReentrantRecoveryNewSurface)
			{
				bReenterRecoveryOnNextIdentityQuery = false;
				ReentrantRecoveryResult =
					ReentrantRecoveryCoordinator->Execute(
						*ReentrantRecoveryOwner,
						*ReentrantRecoveryCheckpoint,
						*const_cast<FFakeArcPreviewHandoffSurface*>(this),
						*ReentrantRecoveryNewSurface);
			}
			return SurfaceInstanceId;
		}

		virtual FName GetConsumerDefinitionId() const override
		{
			++ConsumerQueryCount;
			return ConsumerDefinitionId;
		}

		virtual FArcSurfaceState GetSurfaceCursor() const override
		{
			++CursorQueryCount;
			return Cursor;
		}

		virtual FArcSurfaceResponse Show(
			const FArcSurfaceCommand& Command) override
		{
			return ApplyCommand(Command, EArcSurfaceCommand::Show);
		}

		virtual FArcSurfaceResponse Replace(
			const FArcSurfaceCommand& Command) override
		{
			return ApplyCommand(Command, EArcSurfaceCommand::Replace);
		}

		virtual FArcSurfaceResponse Hide(
			const FArcSurfaceCommand& Command) override
		{
			return ApplyCommand(Command, EArcSurfaceCommand::Hide);
		}

		virtual FArcSurfaceLifecycleResponse ClearToEmpty(
			const FArcSurfaceLifecyclePermit& Permit) override
		{
			++LifecycleClearCallCount;
			const FArcSurfaceState Previous = Cursor;
			Cursor = FArcSurfaceState();
			FArcSurfaceLifecycleResponse Response;
			FString Diagnostic;
			check(FArcSurfaceLifecycleResponse::TryCreate(
				Permit,
				EArcSurfaceLifecycleResponseOutcome::Applied,
				FName(TEXT("Renderer.ArcPreview.HandoffSurface.Cleared")),
				Previous,
				Cursor,
				Response,
				Diagnostic));
			return Response;
		}

		virtual FArcRetirementResponse RetireForHandoff(
			const FArcSurfaceOwnershipTicket& TransitionTicket) override
		{
			++RetirementCallCount;
			if (ReentrantCoordinator && ReentrantOwner
				&& ReentrantTicket && ReentrantNewSurface)
			{
				ReentrantResult = ReentrantCoordinator->Execute(
					*ReentrantOwner,
					*ReentrantTicket,
					*this,
					*ReentrantNewSurface);
			}

			const FArcSurfaceState Previous = Cursor;
			if (RetirementMode == ERetirementMode::InvalidAfterMutation)
			{
				Cursor = FArcSurfaceState();
				return FArcRetirementResponse();
			}
			const bool bRejected =
				RetirementMode == ERetirementMode::Rejected;
			if (RetirementMode == ERetirementMode::Applied)
			{
				Cursor = FArcSurfaceState();
			}
			const FArcSurfaceState ReportedCursor = bRejected
				? Previous
				: FArcSurfaceState();
			FArcRetirementResponse Response;
			FString Diagnostic;
			check(FArcRetirementResponse::TryCreate(
				TransitionTicket,
				SurfaceInstanceId,
				bRejected
					? EArcRetirementOutcome::Rejected
					: EArcRetirementOutcome::Applied,
				bRejected
					? FName(TEXT("Renderer.ArcPreview.Handoff.RetirementRejected"))
					: FName(TEXT("Renderer.ArcPreview.Handoff.Retired")),
				Previous,
				ReportedCursor,
				Response,
				Diagnostic));
			return Response;
		}

		void ForceCursor(const FArcSurfaceState& Value) { Cursor = Value; }
		void SetConsumerDefinitionId(const FName Value)
		{
			ConsumerDefinitionId = Value;
		}
		void SetRetirementMode(const ERetirementMode Value)
		{
			RetirementMode = Value;
		}
		void SetReentrantRetirement(
			FArcOwnerHandoff& Coordinator,
			FArcCompositionOwner& Owner,
			const FArcSurfaceOwnershipTicket& Ticket,
			FFakeArcPreviewHandoffSurface& NewSurface)
		{
			ReentrantCoordinator = &Coordinator;
			ReentrantOwner = &Owner;
			ReentrantTicket = &Ticket;
			ReentrantNewSurface = &NewSurface;
		}
		void SetReentrantRecovery(
			FArcOwnerHandoffRecovery& Coordinator,
			FArcCompositionOwner& Owner,
			const FArcOwnerHandoffRecoveryCheckpoint& Checkpoint,
			FFakeArcPreviewHandoffSurface& NewSurface)
		{
			ReentrantRecoveryCoordinator = &Coordinator;
			ReentrantRecoveryOwner = &Owner;
			ReentrantRecoveryCheckpoint = &Checkpoint;
			ReentrantRecoveryNewSurface = &NewSurface;
			bReenterRecoveryOnNextIdentityQuery = true;
		}

		mutable int32 IdentityQueryCount = 0;
		mutable int32 ConsumerQueryCount = 0;
		mutable int32 CursorQueryCount = 0;
		int32 MutationCallCount = 0;
		int32 LifecycleClearCallCount = 0;
		int32 RetirementCallCount = 0;
		FArcOwnerHandoffResult ReentrantResult;
		mutable FArcOwnerHandoffRecoveryResult ReentrantRecoveryResult;

	private:
		FArcSurfaceResponse ApplyCommand(
			const FArcSurfaceCommand& Command,
			const EArcSurfaceCommand ExpectedKind)
		{
			++MutationCallCount;
			if (!Command.IsValid() || Command.GetKind() != ExpectedKind)
			{
				return FArcSurfaceResponse();
			}
			const FArcSurfaceState Previous = Cursor;
			Cursor = SurfaceCursorForCommand(Command);
			FArcSurfaceResponse Response;
			FString Diagnostic;
			check(FArcSurfaceResponse::TryCreate(
				Command,
				EArcSurfaceOutcome::Applied,
				FName(TEXT("Renderer.ArcPreview.HandoffSurface.Applied")),
				Previous,
				Cursor,
				Response,
				Diagnostic));
			return Response;
		}

		FGuid SurfaceInstanceId;
		FName ConsumerDefinitionId = NAME_None;
		FArcSurfaceState Cursor;
		ERetirementMode RetirementMode = ERetirementMode::Applied;
		FArcOwnerHandoff* ReentrantCoordinator = nullptr;
		FArcCompositionOwner* ReentrantOwner = nullptr;
		const FArcSurfaceOwnershipTicket* ReentrantTicket = nullptr;
		FFakeArcPreviewHandoffSurface* ReentrantNewSurface = nullptr;
		mutable bool bReenterRecoveryOnNextIdentityQuery = false;
		mutable FArcOwnerHandoffRecovery* ReentrantRecoveryCoordinator = nullptr;
		mutable FArcCompositionOwner* ReentrantRecoveryOwner = nullptr;
		mutable const FArcOwnerHandoffRecoveryCheckpoint*
			ReentrantRecoveryCheckpoint = nullptr;
		mutable FFakeArcPreviewHandoffSurface* ReentrantRecoveryNewSurface =
			nullptr;
	};

	bool BuildArcOwnerHandoffTicket(
		FAutomationTestBase& Test,
		const FArcCompositionOwner& Owner,
		FFakeArcPreviewHandoffSurface& NewSurface,
		const EArcSurfaceRecreationAction Action,
		FArcSurfaceOwnershipTicket& OutTicket)
	{
		FArcSurfaceOwnershipRequest Request;
		FString Diagnostic;
		if (!FArcSurfaceOwnershipRequest::TryCreate(
				Owner.GetRunId(),
				Owner.GetConsumerDefinitionId(),
				Owner.GetHostCursor(),
				NewSurface.GetSurfaceInstanceId(),
				Action,
				Request,
				Diagnostic))
		{
			Test.AddError(Diagnostic);
			return false;
		}
		FArcSurfaceOwnershipTransition Transition;
		const FArcSurfaceOwnershipResult Result =
			Transition.Execute(Request, NewSurface);
		if (!Result.IsValid() || !Result.DidAuthorizeBinding()
			|| !Result.HasTransitionTicket())
		{
			Test.AddError(Result.GetDiagnostic());
			return false;
		}
		OutTicket = Result.GetTransitionTicket();
		return OutTicket.IsValid();
	}

	bool BuildArcOwnerHandoffRecoveryCheckpoint(
		FAutomationTestBase& Test,
		const TCHAR* Label,
		FThrownLifecycleFixture& Fixture,
		FArcCompositionOwner& Owner,
		FFakeArcPreviewHandoffSurface& OldSurface,
		FFakeArcPreviewHandoffSurface& NewSurface,
		FArcOwnerHandoffResult& OutFailedHandoff,
		FArcOwnerHandoffRecoveryCheckpoint& OutCheckpoint)
	{
		if (!StartArcPreviewCompositionOwner(
				Test, Label, Fixture, Owner, OldSurface))
		{
			return false;
		}
		const auto Show = UpdateArcPreviewCompositionOwner(
			Owner, MakeArcPreviewChoice(false), Fixture);
		NewSurface.ForceCursor(Owner.GetSurfaceCursor());
		FArcSurfaceOwnershipTicket Ticket;
		if (!Show.IsValid() || !Show.WasApplied()
			|| !BuildArcOwnerHandoffTicket(
				Test,
				Owner,
				NewSurface,
				EArcSurfaceRecreationAction::AdoptExact,
				Ticket))
		{
			return false;
		}
		OldSurface.SetRetirementMode(
			FFakeArcPreviewHandoffSurface::ERetirementMode::
				InvalidAfterMutation);
		FArcOwnerHandoff Handoff;
		OutFailedHandoff = Handoff.Execute(
			Owner, Ticket, OldSurface, NewSurface);
		FString Diagnostic;
		if (!OutFailedHandoff.IsValid()
			|| !OutFailedHandoff.NeedsManualRecovery()
			|| !FArcOwnerHandoffRecoveryCheckpoint::TryCreate(
				OutFailedHandoff, OutCheckpoint, Diagnostic))
		{
			Test.AddError(
				Diagnostic.IsEmpty()
					? OutFailedHandoff.GetDiagnostic()
					: Diagnostic);
			return false;
		}
		return true;
	}

	bool BuildArcOwnerHandoffRecoveryJournalEvidence(
		FAutomationTestBase& Test,
		const TCHAR* Label,
		FThrownLifecycleFixture& Fixture,
		FArcCompositionOwner& Owner,
		FFakeArcPreviewHandoffSurface& OldSurface,
		FFakeArcPreviewHandoffSurface& NewSurface,
		FArcOwnerHandoffRecoveryCheckpoint& OutCheckpoint,
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryReceipt&
			OutReceipt)
	{
		FArcOwnerHandoffResult Failed;
		if (!BuildArcOwnerHandoffRecoveryCheckpoint(
				Test,
				Label,
				Fixture,
				Owner,
				OldSurface,
				NewSurface,
				Failed,
				OutCheckpoint))
		{
			return false;
		}
		FArcOwnerHandoffRecovery Recovery;
		const FArcOwnerHandoffRecoveryResult Recovered = Recovery.Execute(
			Owner, OutCheckpoint, OldSurface, NewSurface);
		if (!Recovered.IsValid() || !Recovered.WasRecovered()
			|| !Recovered.HasReceipt())
		{
			Test.AddError(Recovered.GetDiagnostic());
			return false;
		}
		OutReceipt = Recovered.GetReceipt();
		return OutReceipt.IsValid()
			&& OutReceipt.MatchesCheckpoint(OutCheckpoint);
	}

	bool BuildArcOwnerHandoffRecoveryPayloadEvidence(
		FAutomationTestBase& Test,
		const TCHAR* Label,
		FThrownLifecycleFixture& Fixture,
		FArcCompositionOwner& Owner,
		FFakeArcPreviewHandoffSurface& OldSurface,
		FFakeArcPreviewHandoffSurface& NewSurface,
		FArcOwnerHandoffRecoveryCheckpoint& OutCheckpoint,
		FArcOwnerHandoffRecoveryJournal& OutJournal,
		FArcOwnerHandoffRecoveryPayloadEnvelope& OutEnvelope,
		TArray<uint8>& OutBytes)
	{
		FArcOwnerHandoffResult Failed;
		if (!BuildArcOwnerHandoffRecoveryCheckpoint(
				Test,
				Label,
				Fixture,
				Owner,
				OldSurface,
				NewSurface,
				Failed,
				OutCheckpoint))
		{
			return false;
		}
		const auto Appended = OutJournal.AppendCheckpoint(OutCheckpoint);
		if (!Appended.DidAppend()
			|| !FArcOwnerHandoffRecoveryPayloadEnvelope::TryWrap(
				OutCheckpoint, OutJournal, OutEnvelope)
			|| !FArcOwnerHandoffRecoveryPayloadCodec::TryEncode(
				OutEnvelope, OutBytes))
		{
			Test.AddError(TEXT(
				"Could not build canonical Arc preview recovery checkpoint payload evidence."));
			return false;
		}
		return true;
	}

	bool BuildArcOwnerHandoffRecoveryPayloadStorageContext(
		FAutomationTestBase& Test,
		const TCHAR* Label,
		const FGuid& JournalId,
		FArcOwnerHandoffRecoveryPayloadStorageContext& OutContext)
	{
		const FString Root = FPaths::ConvertRelativePathToFull(FPaths::Combine(
			FPaths::ProjectSavedDir(),
			TEXT("Automation"),
			TEXT("Dev.D.UE.0.0.10.P20.51.r0"),
			Label,
			FGuid::NewGuid().ToString(EGuidFormats::Digits)));
		FString Diagnostic;
		if (!FArcOwnerHandoffRecoveryPayloadStorageContext::TryCreate(
				Root, JournalId, OutContext, Diagnostic))
		{
			Test.AddError(Diagnostic);
			return false;
		}
		return true;
	}

	bool BuildArcOwnerHandoffRecoveryBundleEvidence(
		FAutomationTestBase& Test,
		const TCHAR* Label,
		FThrownLifecycleFixture& Fixture,
		FArcCompositionOwner& Owner,
		FFakeArcPreviewHandoffSurface& OldSurface,
		FFakeArcPreviewHandoffSurface& NewSurface,
		FArcOwnerHandoffRecoveryCheckpoint& OutCheckpoint,
		FArcOwnerHandoffRecoveryJournal& OutJournal,
		FArcOwnerHandoffRecoveryPayloadEnvelope& OutEnvelope,
		FArcOwnerHandoffRecoveryBundle& OutBundle,
		TArray<uint8>& OutBytes)
	{
		TArray<uint8> PayloadBytes;
		if (!BuildArcOwnerHandoffRecoveryPayloadEvidence(
				Test,
				Label,
				Fixture,
				Owner,
				OldSurface,
				NewSurface,
				OutCheckpoint,
				OutJournal,
				OutEnvelope,
				PayloadBytes)
			|| !FArcOwnerHandoffRecoveryBundle::TryCreate(
				OutJournal, OutEnvelope, OutBundle)
			|| !FArcOwnerHandoffRecoveryBundleCodec::TryEncode(
				OutBundle, OutBytes))
		{
			Test.AddError(TEXT(
				"Could not build canonical Arc preview recovery bundle evidence."));
			return false;
		}
		return true;
	}

	bool BuildArcOwnerHandoffRecoveryBundleStorageContext(
		FAutomationTestBase& Test,
		const TCHAR* Label,
		const FArcOwnerHandoffRecoveryBundle& Bundle,
		FArcOwnerHandoffRecoveryBundleStorageContext& OutContext,
		FGuid& OutLineageId)
	{
		OutLineageId.Invalidate();
		if (!FArcOwnerHandoffRecoveryBundleStorageAdapter::
				TryDeriveLineageId(Bundle, OutLineageId))
		{
			Test.AddError(TEXT(
				"Could not derive the stable recovery bundle lineage identity."));
			return false;
		}
		const FString Root = FPaths::ConvertRelativePathToFull(FPaths::Combine(
			FPaths::ProjectSavedDir(),
			TEXT("Automation"),
			TEXT("Dev.D.UE.0.0.10.P20.53.r0"),
			Label,
			FGuid::NewGuid().ToString(EGuidFormats::Digits)));
		FString Diagnostic;
		if (!FArcOwnerHandoffRecoveryBundleStorageContext::TryCreate(
				Root, OutLineageId, OutContext, Diagnostic))
		{
			Test.AddError(Diagnostic);
			return false;
		}
		return true;
	}

	FGuid ArcOwnerHandoffRecoveryWatermarkDomain()
	{
		return FGuid(
			0xF4540001, 0xF4540002, 0xF4540003, 0xF4540004);
	}

	bool BuildArcOwnerHandoffRecoveryWatermarkState(
		FAutomationTestBase& Test,
		const FGuid& AuthorityDomainId,
		const FGuid& LineageId,
		const int32 PreviousGeneration,
		const int32 CommittedGeneration,
		const FGuid& BundleId,
		FArcOwnerHandoffRecoveryBundleWatermarkAdvanceRequest& OutRequest,
		FArcOwnerHandoffRecoveryBundleWatermarkAdvanceReceipt& OutReceipt,
		FArcOwnerHandoffRecoveryBundleWatermarkState& OutState)
	{
		FString Diagnostic;
		if (!FArcOwnerHandoffRecoveryBundleWatermarkAdvanceRequest::TryCreate(
				AuthorityDomainId,
				LineageId,
				PreviousGeneration,
				CommittedGeneration,
				BundleId,
				OutRequest,
				Diagnostic)
			|| !FArcOwnerHandoffRecoveryBundleWatermarkAdvanceReceipt::
				TryCreate(OutRequest, OutReceipt)
			|| !FArcOwnerHandoffRecoveryBundleWatermarkState::TryCreate(
				OutReceipt, OutState))
		{
			Test.AddError(
				Diagnostic.IsEmpty()
					? TEXT(
						"Could not build trusted recovery watermark evidence.")
					: Diagnostic);
			return false;
		}
		return true;
	}

	bool AdvanceArcOwnerHandoffRecoveryBundleGeneration(
		FAutomationTestBase& Test,
		const TCHAR* Label,
		FArcCompositionOwner& FirstOwner,
		FFakeArcPreviewHandoffSurface& FirstOldSurface,
		FFakeArcPreviewHandoffSurface& FirstNewSurface,
		const FArcOwnerHandoffRecoveryCheckpoint& FirstCheckpoint,
		FArcOwnerHandoffRecoveryJournal& InOutJournal,
		FArcOwnerHandoffRecoveryCheckpoint& OutCheckpoint,
		FArcOwnerHandoffRecoveryBundle& OutBundle,
		TArray<uint8>& OutBytes)
	{
		FArcOwnerHandoffRecovery Recovery;
		const auto Recovered = Recovery.Execute(
			FirstOwner,
			FirstCheckpoint,
			FirstOldSurface,
			FirstNewSurface);
		const auto Committed = Recovered.HasReceipt()
			? InOutJournal.AppendRecoveryReceipt(
				FirstCheckpoint, Recovered.GetReceipt())
			: Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalAppendResult();
		if (!Committed.DidAppend())
		{
			Test.AddError(TEXT(
				"Could not commit the first recovery bundle generation."));
			return false;
		}

		const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
		FThrownLifecycleFixture Fixture;
		FFakeArcPreviewHandoffSurface OldSurface(
			FGuid(0xF4D0A001, 0xF4D0A002, 0xF4D0A003, 0xF4D0A004),
			Consumer);
		FFakeArcPreviewHandoffSurface NewSurface(
			FGuid(0xF4D0B001, 0xF4D0B002, 0xF4D0B003, 0xF4D0B004),
			Consumer);
		FArcCompositionOwner Owner;
		FArcOwnerHandoffResult Failed;
		if (!BuildArcOwnerHandoffRecoveryCheckpoint(
				Test,
				Label,
				Fixture,
				Owner,
				OldSurface,
				NewSurface,
				Failed,
				OutCheckpoint))
		{
			return false;
		}
		const auto Appended = InOutJournal.AppendCheckpoint(OutCheckpoint);
		FArcOwnerHandoffRecoveryPayloadEnvelope Envelope;
		if (!Appended.DidAppend()
			|| !FArcOwnerHandoffRecoveryPayloadEnvelope::TryWrap(
				OutCheckpoint, InOutJournal, Envelope)
			|| !FArcOwnerHandoffRecoveryBundle::TryCreate(
				InOutJournal, Envelope, OutBundle)
			|| !FArcOwnerHandoffRecoveryBundleCodec::TryEncode(
				OutBundle, OutBytes))
		{
			Test.AddError(TEXT(
				"Could not advance canonical recovery bundle evidence to generation two."));
			return false;
		}
		return OutBundle.GetGeneration() == 2;
	}

	struct FArcOwnerHandoffRecoveryAdmissionFixture
	{
		FArcOwnerHandoffRecoveryAdmissionFixture()
			: OldSurface(
				FGuid(0xF4555001, 0xF4555002, 0xF4555003, 0xF4555004),
				Consumer)
			, NewSurface(
				FGuid(0xF4556001, 0xF4556002, 0xF4556003, 0xF4556004),
				Consumer)
		{
		}

		bool Build(FAutomationTestBase& Test, const TCHAR* Label)
		{
			if (!BuildArcOwnerHandoffRecoveryBundleEvidence(
					Test,
					Label,
					Lifecycle,
					Owner,
					OldSurface,
					NewSurface,
					Checkpoint,
					Journal,
					Envelope,
					Bundle,
					Bytes)
				|| !BuildArcOwnerHandoffRecoveryBundleStorageContext(
					Test,
					Label,
					Bundle,
					StorageContext,
					LineageId)
				|| !BuildArcOwnerHandoffRecoveryWatermarkState(
					Test,
					AuthorityDomainId,
					LineageId,
					Bundle.GetGeneration() - 1,
					Bundle.GetGeneration(),
					Bundle.GetBundleId(),
					WatermarkRequest,
					WatermarkReceipt,
					WatermarkState))
			{
				return false;
			}
			FileSystem.SetFile(StorageContext.GetPrimaryPath(), Bytes);
			Authority.SetState(WatermarkState);
			FString Diagnostic;
			if (!FArcOwnerHandoffRecoveryAdmissionRequest::TryCreate(
					AuthorityDomainId,
					LineageId,
					Journal.GetJournalId(),
					Checkpoint.GetCheckpointId(),
					Request,
					Diagnostic))
			{
				Test.AddError(Diagnostic);
				return false;
			}
			return true;
		}

		const FName Consumer = FName(TEXT("Renderer.ArcPreview.MainHUD.r1"));
		const FGuid AuthorityDomainId = ArcOwnerHandoffRecoveryWatermarkDomain();
		FThrownLifecycleFixture Lifecycle;
		FFakeArcPreviewHandoffSurface OldSurface;
		FFakeArcPreviewHandoffSurface NewSurface;
		FArcCompositionOwner Owner;
		FArcOwnerHandoffRecoveryCheckpoint Checkpoint;
		FArcOwnerHandoffRecoveryJournal Journal;
		FArcOwnerHandoffRecoveryPayloadEnvelope Envelope;
		FArcOwnerHandoffRecoveryBundle Bundle;
		TArray<uint8> Bytes;
		FArcOwnerHandoffRecoveryBundleStorageContext StorageContext;
		FGuid LineageId;
		FArcOwnerHandoffRecoveryBundleWatermarkAdvanceRequest WatermarkRequest;
		FArcOwnerHandoffRecoveryBundleWatermarkAdvanceReceipt WatermarkReceipt;
		FArcOwnerHandoffRecoveryBundleWatermarkState WatermarkState;
		FFakeArcOwnerHandoffRecoveryPayloadStorageFileSystem FileSystem;
		FFakeArcOwnerHandoffRecoveryBundleWatermarkAuthority Authority;
		FArcOwnerHandoffRecoveryAdmissionRequest Request;
	};

	FGuid ArcOwnerHandoffRecoveryCompletionWatermarkDomain()
	{
		return FGuid(
			0xF4560001, 0xF4560002, 0xF4560003, 0xF4560004);
	}

	bool BuildArcOwnerHandoffRecoveryCompletionStorageContext(
		FAutomationTestBase& Test,
		const TCHAR* Label,
		const FGuid& LineageId,
		FArcOwnerHandoffRecoveryCompletionStorageContext& OutContext)
	{
		const FString Root = FPaths::ConvertRelativePathToFull(FPaths::Combine(
			FPaths::ProjectSavedDir(),
			TEXT("Automation"),
			TEXT("Dev.D.UE.0.0.10.P20.56.r0"),
			Label,
			FGuid::NewGuid().ToString(EGuidFormats::Digits)));
		FString Diagnostic;
		if (!FArcOwnerHandoffRecoveryCompletionStorageContext::TryCreate(
				Root, LineageId, OutContext, Diagnostic))
		{
			Test.AddError(Diagnostic);
			return false;
		}
		return true;
	}

	struct FArcOwnerHandoffRecoveryCompletionFixture
	{
		FArcOwnerHandoffRecoveryCompletionFixture()
			: Authority(
				ArcOwnerHandoffRecoveryWatermarkDomain(),
				ArcOwnerHandoffRecoveryCompletionWatermarkDomain())
		{
		}

		bool Build(FAutomationTestBase& Test, const TCHAR* Label)
		{
			if (!Admission.Build(Test, Label)
				|| !BuildArcOwnerHandoffRecoveryCompletionStorageContext(
					Test,
					Label,
					Admission.LineageId,
					CompletionStorageContext))
			{
				return false;
			}
			FString Diagnostic;
			if (!FArcOwnerHandoffRecoveryCompletionRequest::TryCreate(
					CompletionAuthorityDomainId,
					Admission.Request,
					Request,
					Diagnostic))
			{
				Test.AddError(Diagnostic);
				return false;
			}
			Authority.Pending.SetState(Admission.WatermarkState);
			Authority.Completion.ClearState();
			return true;
		}

		FArcOwnerHandoffRecoveryCompletionSessionResult Execute(
			FArcOwnerHandoffRecoveryCompletionSession& Session)
		{
			return Session.ExecuteExplicit(
				Request,
				Admission.StorageContext,
				CompletionStorageContext,
				Admission.Journal,
				Admission.Owner,
				Admission.OldSurface,
				Admission.NewSurface,
				Admission.FileSystem,
				Authority);
		}

		const FGuid CompletionAuthorityDomainId =
			ArcOwnerHandoffRecoveryCompletionWatermarkDomain();
		FArcOwnerHandoffRecoveryAdmissionFixture Admission;
		FArcOwnerHandoffRecoveryCompletionStorageContext
			CompletionStorageContext;
		FArcOwnerHandoffRecoveryCompletionRequest Request;
		FFakeArcOwnerHandoffRecoveryCompletionAuthority Authority;
	};

	FGuid ArcOwnerHandoffRecoveryTerminalAdoptionWatermarkDomain()
	{
		return FGuid(
			0xF4570001, 0xF4570002, 0xF4570003, 0xF4570004);
	}

	struct FArcOwnerHandoffRecoveryTerminalAdoptionFixture
	{
		bool Build(FAutomationTestBase& Test, const TCHAR* Label)
		{
			FArcOwnerHandoffRecoveryCompletionSession CompletionSession;
			if (!Completion.Build(Test, Label))
			{
				return false;
			}
			CompletionResult = Completion.Execute(CompletionSession);
			FString Diagnostic;
			if (!CompletionResult.IsSuccess()
				|| !FArcOwnerHandoffRecoveryTerminalAdoptionRequest::TryCreate(
					AdoptionAuthorityDomainId,
					Completion.Request,
					Request,
					Diagnostic))
			{
				Test.AddError(
					Diagnostic.IsEmpty()
						? TEXT(
							"Could not build trusted terminal adoption fixture.")
						: Diagnostic);
				return false;
			}
			AdoptionAuthority.ClearState();
			return true;
		}

		FArcOwnerHandoffRecoveryTerminalAdoptionSessionResult Execute(
			FArcOwnerHandoffRecoveryTerminalAdoptionSession& Session,
			const FArcOwnerHandoffRecoveryJournal& CurrentJournal)
		{
			return Session.ExecuteExplicit(
				Request,
				Completion.CompletionStorageContext,
				CurrentJournal,
				Completion.Admission.FileSystem,
				Completion.Authority,
				AdoptionAuthority);
		}

		FArcOwnerHandoffRecoveryTerminalAdoptionSessionResult Execute(
			FArcOwnerHandoffRecoveryTerminalAdoptionSession& Session)
		{
			return Execute(Session, Completion.Admission.Journal);
		}

		const FGuid AdoptionAuthorityDomainId =
			ArcOwnerHandoffRecoveryTerminalAdoptionWatermarkDomain();
		FArcOwnerHandoffRecoveryCompletionFixture Completion;
		FArcOwnerHandoffRecoveryCompletionSessionResult CompletionResult;
		FArcOwnerHandoffRecoveryTerminalAdoptionRequest Request;
		FFakeArcOwnerHandoffRecoveryBundleWatermarkAuthority
			AdoptionAuthority;
	};

	struct FArcOwnerHandoffRecoveryNextGenerationRotationFixture
	{
		FArcOwnerHandoffRecoveryNextGenerationRotationFixture()
			: NextOldSurface(
				FGuid(0xF4580001, 0xF4580002, 0xF4580003, 0xF4580004),
				Consumer)
			, NextNewSurface(
				FGuid(0xF4581001, 0xF4581002, 0xF4581003, 0xF4581004),
				Consumer)
		{
		}

		bool Build(FAutomationTestBase& Test, const TCHAR* Label)
		{
			if (!Terminal.Build(Test, Label))
			{
				return false;
			}
			FArcOwnerHandoffRecoveryTerminalAdoptionSession AdoptionSession;
			AdoptionResult = Terminal.Execute(AdoptionSession);
			CurrentTerminalJournal = AdoptionResult.GetTerminalJournal();
			const FString CheckpointLabel = FString::Printf(
				TEXT("%s.NextCheckpoint"), Label);
			FString Diagnostic;
			if (!AdoptionResult.IsSuccess()
				|| !BuildArcOwnerHandoffRecoveryCheckpoint(
					Test,
					*CheckpointLabel,
					NextLifecycle,
					NextOwner,
					NextOldSurface,
					NextNewSurface,
					NextFailed,
					NewCheckpoint)
				|| !FArcOwnerHandoffRecoveryNextGenerationRotationRequest::
					TryCreate(
						AdoptionResult.GetAdoption(),
						NewCheckpoint,
						Request,
						Diagnostic))
			{
				Test.AddError(
					Diagnostic.IsEmpty()
						? TEXT(
							"Could not build next-generation recovery rotation fixture.")
						: Diagnostic);
				return false;
			}
			return true;
		}

		FArcOwnerHandoffRecoveryNextGenerationRotationSessionResult Execute(
			FArcOwnerHandoffRecoveryNextGenerationRotationSession& Session,
			const FArcOwnerHandoffRecoveryJournal& CurrentJournal)
		{
			return Session.ExecuteExplicit(
				Request,
				Terminal.Completion.CompletionStorageContext,
				Terminal.Completion.Admission.StorageContext,
				CurrentJournal,
				Terminal.Completion.Admission.FileSystem,
				Terminal.Completion.Authority,
				Terminal.AdoptionAuthority,
				Terminal.Completion.Authority);
		}

		FArcOwnerHandoffRecoveryNextGenerationRotationSessionResult Execute(
			FArcOwnerHandoffRecoveryNextGenerationRotationSession& Session)
		{
			return Execute(Session, CurrentTerminalJournal);
		}

		const FName Consumer = FName(TEXT("Renderer.ArcPreview.MainHUD.r1"));
		FArcOwnerHandoffRecoveryTerminalAdoptionFixture Terminal;
		FArcOwnerHandoffRecoveryTerminalAdoptionSessionResult AdoptionResult;
		FArcOwnerHandoffRecoveryJournal CurrentTerminalJournal;
		FThrownLifecycleFixture NextLifecycle;
		FFakeArcPreviewHandoffSurface NextOldSurface;
		FFakeArcPreviewHandoffSurface NextNewSurface;
		FArcCompositionOwner NextOwner;
		FArcOwnerHandoffResult NextFailed;
		FArcOwnerHandoffRecoveryCheckpoint NewCheckpoint;
		FArcOwnerHandoffRecoveryNextGenerationRotationRequest Request;
	};

	struct FArcOwnerHandoffRecoveryGenerationExecution
	{
		FArcOwnerHandoffRecoveryCheckpoint Checkpoint;
		FArcOwnerHandoffRecoveryNextGenerationRotationRequest RotationRequest;
		FArcOwnerHandoffRecoveryNextGenerationRotationSessionResult
			RotationResult;
		FArcOwnerHandoffRecoveryCompletionRequest CompletionRequest;
		FArcOwnerHandoffRecoveryCompletionSessionResult CompletionResult;
		FArcOwnerHandoffRecoveryTerminalAdoptionRequest AdoptionRequest;
		FArcOwnerHandoffRecoveryTerminalAdoptionSessionResult AdoptionResult;
		FArcOwnerHandoffRecoveryNextGenerationRotationSessionResult
			RotationReplay;
		FArcOwnerHandoffRecoveryCompletionSessionResult CompletionReplay;
		FArcOwnerHandoffRecoveryTerminalAdoptionSessionResult AdoptionReplay;
		bool bRotationReplaySideEffectFree = false;
		bool bCompletionReplaySideEffectFree = false;
		bool bAdoptionReplaySideEffectFree = false;
	};

	/**
	 * Test-only driver that composes the existing P20.55-P20.58 calls. It owns
	 * no additional production authority and keeps every generation bounded to
	 * one rotation, one completion and one terminal adoption.
	 */
	struct FArcOwnerHandoffRecoveryMultiGenerationHarness
	{
		bool Build(FAutomationTestBase& Test, const TCHAR* Label)
		{
			if (!Seed.Build(Test, Label))
			{
				return false;
			}
			FArcOwnerHandoffRecoveryTerminalAdoptionSession AdoptionSession;
			const auto SeedAdoption = Seed.Execute(AdoptionSession);
			if (!SeedAdoption.IsSuccess())
			{
				Test.AddError(SeedAdoption.GetDiagnostic());
				return false;
			}
			CurrentAdoption = SeedAdoption.GetAdoption();
			CurrentTerminalJournal = SeedAdoption.GetTerminalJournal();
			return CurrentAdoption.IsValid()
				&& CurrentAdoption.GetGeneration() == 1
				&& CurrentTerminalJournal.IsValid()
				&& CurrentTerminalJournal.GetRecordCount() == 2;
		}

		bool ExecuteGeneration(
			FAutomationTestBase& Test,
			const TCHAR* Label,
			const bool bExerciseReplay,
			FArcOwnerHandoffRecoveryGenerationExecution& OutExecution)
		{
			OutExecution = {};
			const int32 TargetGeneration =
				CurrentAdoption.GetNextGeneration();
			if (!CurrentAdoption.IsValid() || TargetGeneration <= 0)
			{
				Test.AddError(TEXT(
					"Multi-generation harness has no next generation capacity."));
				return false;
			}

			const uint32 GenerationWord =
				static_cast<uint32>(TargetGeneration);
			const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
			FThrownLifecycleFixture Lifecycle;
			FFakeArcPreviewHandoffSurface OldSurface(
				FGuid(
					0xF4590001,
					0xF4590002,
					0xF4590003,
					GenerationWord),
				Consumer);
			FFakeArcPreviewHandoffSurface NewSurface(
				FGuid(
					0xF4591001,
					0xF4591002,
					0xF4591003,
					GenerationWord),
				Consumer);
			FArcCompositionOwner Owner;
			FArcOwnerHandoffResult Failed;
			const FString GenerationLabel = FString::Printf(
				TEXT("%s.G%d"), Label, TargetGeneration);
			FString Diagnostic;
			if (!BuildArcOwnerHandoffRecoveryCheckpoint(
					Test,
					*GenerationLabel,
					Lifecycle,
					Owner,
					OldSurface,
					NewSurface,
					Failed,
					OutExecution.Checkpoint)
				|| !FArcOwnerHandoffRecoveryNextGenerationRotationRequest::
					TryCreate(
						CurrentAdoption,
						OutExecution.Checkpoint,
						OutExecution.RotationRequest,
						Diagnostic))
			{
				Test.AddError(
					Diagnostic.IsEmpty()
						? TEXT("Could not prepare the next recovery generation.")
						: Diagnostic);
				return false;
			}

			FArcOwnerHandoffRecoveryNextGenerationRotationSession
				RotationSession;
			OutExecution.RotationResult = RotationSession.ExecuteExplicit(
				OutExecution.RotationRequest,
				Seed.Completion.CompletionStorageContext,
				Seed.Completion.Admission.StorageContext,
				CurrentTerminalJournal,
				Seed.Completion.Admission.FileSystem,
				Seed.Completion.Authority,
				Seed.AdoptionAuthority,
				Seed.Completion.Authority);
			if (!OutExecution.RotationResult.IsSuccess())
			{
				Test.AddError(OutExecution.RotationResult.GetDiagnostic());
				return false;
			}

			if (bExerciseReplay)
			{
				const int32 WritesBeforeReplay =
					Seed.Completion.Admission.FileSystem.WriteCount;
				const int32 AdvancesBeforeReplay =
					Seed.Completion.Authority.Pending.AdvanceCount;
				FArcOwnerHandoffRecoveryNextGenerationRotationSession
					ReplaySession;
				OutExecution.RotationReplay =
					ReplaySession.ExecuteExplicit(
						OutExecution.RotationRequest,
						Seed.Completion.CompletionStorageContext,
						Seed.Completion.Admission.StorageContext,
						CurrentTerminalJournal,
						Seed.Completion.Admission.FileSystem,
						Seed.Completion.Authority,
						Seed.AdoptionAuthority,
						Seed.Completion.Authority);
				OutExecution.bRotationReplaySideEffectFree =
					Seed.Completion.Admission.FileSystem.WriteCount
						== WritesBeforeReplay
					&& Seed.Completion.Authority.Pending.AdvanceCount
						== AdvancesBeforeReplay;
			}

			const auto& Rotation =
				OutExecution.RotationResult.GetRotation();
			FArcOwnerHandoffRecoveryAdmissionRequest AdmissionRequest;
			if (!FArcOwnerHandoffRecoveryAdmissionRequest::TryCreate(
					Seed.Completion.Admission.AuthorityDomainId,
					Seed.Completion.Admission.LineageId,
					Rotation.GetPendingJournal().GetJournalId(),
					OutExecution.Checkpoint.GetCheckpointId(),
					AdmissionRequest,
					Diagnostic)
				|| !FArcOwnerHandoffRecoveryCompletionRequest::TryCreate(
					Seed.Completion.CompletionAuthorityDomainId,
					AdmissionRequest,
					OutExecution.CompletionRequest,
					Diagnostic))
			{
				Test.AddError(Diagnostic);
				return false;
			}

			FArcOwnerHandoffRecoveryCompletionSession CompletionSession;
			OutExecution.CompletionResult = CompletionSession.ExecuteExplicit(
				OutExecution.CompletionRequest,
				Seed.Completion.Admission.StorageContext,
				Seed.Completion.CompletionStorageContext,
				Rotation.GetPendingJournal(),
				Owner,
				OldSurface,
				NewSurface,
				Seed.Completion.Admission.FileSystem,
				Seed.Completion.Authority);
			if (!OutExecution.CompletionResult.IsSuccess())
			{
				Test.AddError(OutExecution.CompletionResult.GetDiagnostic());
				return false;
			}

			if (bExerciseReplay)
			{
				const int32 WritesBeforeReplay =
					Seed.Completion.Admission.FileSystem.WriteCount;
				const int32 AdvancesBeforeReplay =
					Seed.Completion.Authority.Completion.AdvanceCount;
				FArcOwnerHandoffRecoveryCompletionSession ReplaySession;
				OutExecution.CompletionReplay =
					ReplaySession.ExecuteExplicit(
						OutExecution.CompletionRequest,
						Seed.Completion.Admission.StorageContext,
						Seed.Completion.CompletionStorageContext,
						Rotation.GetPendingJournal(),
						Owner,
						OldSurface,
						NewSurface,
						Seed.Completion.Admission.FileSystem,
						Seed.Completion.Authority);
				OutExecution.bCompletionReplaySideEffectFree =
					Seed.Completion.Admission.FileSystem.WriteCount
						== WritesBeforeReplay
					&& Seed.Completion.Authority.Completion.AdvanceCount
						== AdvancesBeforeReplay;
			}

			if (!FArcOwnerHandoffRecoveryTerminalAdoptionRequest::TryCreate(
					Seed.AdoptionAuthorityDomainId,
					OutExecution.CompletionRequest,
					OutExecution.AdoptionRequest,
					Diagnostic))
			{
				Test.AddError(Diagnostic);
				return false;
			}
			FArcOwnerHandoffRecoveryTerminalAdoptionSession AdoptionSession;
			OutExecution.AdoptionResult = AdoptionSession.ExecuteExplicit(
				OutExecution.AdoptionRequest,
				Seed.Completion.CompletionStorageContext,
				Rotation.GetPendingJournal(),
				Seed.Completion.Admission.FileSystem,
				Seed.Completion.Authority,
				Seed.AdoptionAuthority);
			if (!OutExecution.AdoptionResult.IsSuccess())
			{
				Test.AddError(OutExecution.AdoptionResult.GetDiagnostic());
				return false;
			}

			if (bExerciseReplay)
			{
				const int32 WritesBeforeReplay =
					Seed.Completion.Admission.FileSystem.WriteCount;
				const int32 AdvancesBeforeReplay =
					Seed.AdoptionAuthority.AdvanceCount;
				FArcOwnerHandoffRecoveryTerminalAdoptionSession ReplaySession;
				OutExecution.AdoptionReplay = ReplaySession.ExecuteExplicit(
					OutExecution.AdoptionRequest,
					Seed.Completion.CompletionStorageContext,
					Rotation.GetPendingJournal(),
					Seed.Completion.Admission.FileSystem,
					Seed.Completion.Authority,
					Seed.AdoptionAuthority);
				OutExecution.bAdoptionReplaySideEffectFree =
					Seed.Completion.Admission.FileSystem.WriteCount
						== WritesBeforeReplay
					&& Seed.AdoptionAuthority.AdvanceCount
						== AdvancesBeforeReplay;
			}

			CurrentAdoption = OutExecution.AdoptionResult.GetAdoption();
			CurrentTerminalJournal =
				OutExecution.AdoptionResult.GetTerminalJournal();
			return CurrentAdoption.IsValid()
				&& CurrentAdoption.GetGeneration() == TargetGeneration
				&& CurrentTerminalJournal.IsValid()
				&& CurrentTerminalJournal.GetRecordCount()
					== TargetGeneration * 2;
		}

		FArcOwnerHandoffRecoveryTerminalAdoptionFixture Seed;
		FArcOwnerHandoffRecoveryTerminalAdoption CurrentAdoption;
		FArcOwnerHandoffRecoveryJournal CurrentTerminalJournal;
	};

	bool OverwriteUint32BigEndian(
		TArray<uint8>& Bytes,
		const int32 Offset,
		const uint32 Value)
	{
		if (Offset < 0 || Offset > Bytes.Num() - 4)
		{
			return false;
		}
		Bytes[Offset] = static_cast<uint8>((Value >> 24) & 0xff);
		Bytes[Offset + 1] = static_cast<uint8>((Value >> 16) & 0xff);
		Bytes[Offset + 2] = static_cast<uint8>((Value >> 8) & 0xff);
		Bytes[Offset + 3] = static_cast<uint8>(Value & 0xff);
		return true;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffEvidenceTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoff.EvidenceContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffEvidenceTest::RunTest(
	const FString&)
{
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture Fixture;
	FFakeArcPreviewHandoffSurface OldSurface(
		FGuid(0xF4700001, 0xF4700002, 0xF4700003, 0xF4700004),
		Consumer);
	FFakeArcPreviewHandoffSurface NewSurface(
		FGuid(0xF4701001, 0xF4701002, 0xF4701003, 0xF4701004),
		Consumer);
	FArcCompositionOwner Owner;
	if (!StartArcPreviewCompositionOwner(
			*this, TEXT("ArcPreviewOwnerHandoffEvidence"),
			Fixture, Owner, OldSurface))
	{
		return false;
	}
	const int32 OldIdentityReads = OldSurface.IdentityQueryCount;
	const int32 NewIdentityReads = NewSurface.IdentityQueryCount;
	FArcOwnerHandoff Coordinator;
	const auto Invalid = Coordinator.Execute(
		Owner, FArcSurfaceOwnershipTicket(), OldSurface, NewSurface);
	TestTrue(TEXT("invalid ticket rejects before every surface callback"),
		Invalid.IsValid()
			&& Invalid.GetStatus() == EArcOwnerHandoffStatus::TicketInvalid
			&& !Invalid.IsAccepted() && !Invalid.DidCallRetirement()
			&& OldSurface.IdentityQueryCount == OldIdentityReads
			&& NewSurface.IdentityQueryCount == NewIdentityReads
			&& OldSurface.RetirementCallCount == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffFreshTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoff.BindFreshCommit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffFreshTest::RunTest(
	const FString&)
{
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture Fixture;
	FFakeArcPreviewHandoffSurface OldSurface(
		FGuid(0xF4710001, 0xF4710002, 0xF4710003, 0xF4710004),
		Consumer);
	FFakeArcPreviewHandoffSurface NewSurface(
		FGuid(0xF4711001, 0xF4711002, 0xF4711003, 0xF4711004),
		Consumer);
	FArcCompositionOwner Owner;
	if (!StartArcPreviewCompositionOwner(
			*this, TEXT("ArcPreviewOwnerHandoffFresh"),
			Fixture, Owner, OldSurface))
	{
		return false;
	}
	FArcSurfaceOwnershipTicket Ticket;
	if (!BuildArcOwnerHandoffTicket(
			*this, Owner, NewSurface,
			EArcSurfaceRecreationAction::BindFresh, Ticket))
	{
		return false;
	}
	FArcOwnerHandoff Coordinator;
	const auto Committed = Coordinator.Execute(
		Owner, Ticket, OldSurface, NewSurface);
	const auto Replay = Coordinator.Execute(
		Owner, Ticket, OldSurface, NewSurface);
	const auto Show = UpdateArcPreviewCompositionOwner(
		Owner, MakeArcPreviewChoice(false), Fixture);
	TestTrue(TEXT("fresh ticket swaps two empty surfaces without retirement"),
		Committed.IsValid() && Committed.WasCommitted()
			&& Committed.HasReceipt()
			&& Committed.GetReceipt().IsFreshBound()
			&& !Committed.DidCallRetirement()
			&& OldSurface.RetirementCallCount == 0
			&& Owner.GetBoundSurfaceInstanceId()
				== Ticket.GetSurfaceInstanceId());
	TestTrue(TEXT("exact consumed ticket replays without mutation"),
		Replay.IsValid() && Replay.IsReplay()
			&& !Replay.DidCallRetirement()
			&& Replay.GetReceipt().GetReceiptId()
				== Committed.GetReceipt().GetReceiptId());
	TestTrue(TEXT("post-handoff Owner dispatch reaches only replacement"),
		Show.IsValid() && Show.WasApplied()
			&& OldSurface.MutationCallCount == 0
			&& NewSurface.MutationCallCount == 1
			&& Owner.IsValid() && Owner.IsSynchronized());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffAdoptTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoff.AdoptExactCommit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffAdoptTest::RunTest(
	const FString&)
{
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture Fixture;
	FFakeArcPreviewHandoffSurface OldSurface(
		FGuid(0xF4720001, 0xF4720002, 0xF4720003, 0xF4720004),
		Consumer);
	FFakeArcPreviewHandoffSurface NewSurface(
		FGuid(0xF4721001, 0xF4721002, 0xF4721003, 0xF4721004),
		Consumer);
	FArcCompositionOwner Owner;
	if (!StartArcPreviewCompositionOwner(
			*this, TEXT("ArcPreviewOwnerHandoffAdopt"),
			Fixture, Owner, OldSurface))
	{
		return false;
	}
	const auto Show = UpdateArcPreviewCompositionOwner(
		Owner, MakeArcPreviewChoice(false), Fixture);
	NewSurface.ForceCursor(Owner.GetSurfaceCursor());
	FArcSurfaceOwnershipTicket Ticket;
	if (!Show.WasApplied()
		|| !BuildArcOwnerHandoffTicket(
			*this, Owner, NewSurface,
			EArcSurfaceRecreationAction::AdoptExact, Ticket))
	{
		return false;
	}
	FArcOwnerHandoff Coordinator;
	const auto Committed = Coordinator.Execute(
		Owner, Ticket, OldSurface, NewSurface);
	const auto Replay = Coordinator.Execute(
		Owner, Ticket, OldSurface, NewSurface);
	const auto Replace = UpdateArcPreviewCompositionOwner(
		Owner, MakeArcPreviewChoice(true), Fixture);
	TestTrue(TEXT("exact adoption retires old visibility once before commit"),
		Committed.IsValid() && Committed.WasCommitted()
			&& Committed.DidCallRetirement()
			&& Committed.GetReceipt().IsExactAdopted()
			&& OldSurface.RetirementCallCount == 1
			&& OldSurface.GetSurfaceCursor().IsEmpty()
			&& Replay.IsReplay()
			&& OldSurface.RetirementCallCount == 1);
	TestTrue(TEXT("visible handoff preserves authority and redirects updates"),
		Replace.IsValid() && Replace.WasApplied()
			&& OldSurface.MutationCallCount == 1
			&& NewSurface.MutationCallCount == 1
			&& Owner.IsValid() && Owner.IsSynchronized());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRetryTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoff.RetirementRejectedRetry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRetryTest::RunTest(
	const FString&)
{
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture Fixture;
	FFakeArcPreviewHandoffSurface OldSurface(
		FGuid(0xF4730001, 0xF4730002, 0xF4730003, 0xF4730004),
		Consumer);
	FFakeArcPreviewHandoffSurface NewSurface(
		FGuid(0xF4731001, 0xF4731002, 0xF4731003, 0xF4731004),
		Consumer);
	FArcCompositionOwner Owner;
	if (!StartArcPreviewCompositionOwner(
			*this, TEXT("ArcPreviewOwnerHandoffRetry"),
			Fixture, Owner, OldSurface))
	{
		return false;
	}
	UpdateArcPreviewCompositionOwner(
		Owner, MakeArcPreviewChoice(false), Fixture);
	NewSurface.ForceCursor(Owner.GetSurfaceCursor());
	FArcSurfaceOwnershipTicket Ticket;
	if (!BuildArcOwnerHandoffTicket(
			*this, Owner, NewSurface,
			EArcSurfaceRecreationAction::AdoptExact, Ticket))
	{
		return false;
	}
	OldSurface.SetRetirementMode(
		FFakeArcPreviewHandoffSurface::ERetirementMode::Rejected);
	FArcOwnerHandoff Coordinator;
	const auto Rejected = Coordinator.Execute(
		Owner, Ticket, OldSurface, NewSurface);
	OldSurface.SetRetirementMode(
		FFakeArcPreviewHandoffSurface::ERetirementMode::Applied);
	const auto Retried = Coordinator.Execute(
		Owner, Ticket, OldSurface, NewSurface);
	TestTrue(TEXT("unchanged retirement rejection preserves exact retry"),
		Rejected.IsValid() && Rejected.WasRetirementRejected()
			&& Rejected.CanRetryExactTicket()
			&& Rejected.IsOwnerValidAfter()
			&& !Rejected.HasReceipt());
	TestTrue(TEXT("one later explicit attempt can consume the same ticket"),
		Retried.IsValid() && Retried.WasCommitted()
			&& OldSurface.RetirementCallCount == 2
			&& Owner.GetLastSurfaceHandoffReceipt().IsExactAdopted());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffFenceTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoff.IdentityAndSnapshotFences",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffFenceTest::RunTest(
	const FString&)
{
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture Fixture;
	FFakeArcPreviewHandoffSurface OldSurface(
		FGuid(0xF4740001, 0xF4740002, 0xF4740003, 0xF4740004),
		Consumer);
	FFakeArcPreviewHandoffSurface WrongOld(
		FGuid(0xF4740101, 0xF4740102, 0xF4740103, 0xF4740104),
		Consumer);
	FFakeArcPreviewHandoffSurface NewSurface(
		FGuid(0xF4741001, 0xF4741002, 0xF4741003, 0xF4741004),
		Consumer);
	FArcCompositionOwner Owner;
	if (!StartArcPreviewCompositionOwner(
			*this, TEXT("ArcPreviewOwnerHandoffFence"),
			Fixture, Owner, OldSurface))
	{
		return false;
	}
	FArcSurfaceOwnershipTicket Ticket;
	if (!BuildArcOwnerHandoffTicket(
			*this, Owner, NewSurface,
			EArcSurfaceRecreationAction::BindFresh, Ticket))
	{
		return false;
	}
	const int32 NewReads = NewSurface.IdentityQueryCount;
	FArcOwnerHandoff Coordinator;
	const auto WrongPointer = Coordinator.Execute(
		Owner, Ticket, WrongOld, NewSurface);
	const bool bWrongPointerReadNothing =
		WrongOld.IdentityQueryCount == 0
		&& NewSurface.IdentityQueryCount == NewReads;
	NewSurface.SetConsumerDefinitionId(
		FName(TEXT("Renderer.ArcPreview.Foreign.r1")));
	const auto DriftedNew = Coordinator.Execute(
		Owner, Ticket, OldSurface, NewSurface);
	TestTrue(TEXT("old pointer mismatch rejects before supplied surface reads"),
		WrongPointer.IsValid()
			&& WrongPointer.GetStatus()
				== EArcOwnerHandoffStatus::OldSurfaceMismatch
			&& bWrongPointerReadNothing
			&& OldSurface.RetirementCallCount == 0);
	TestTrue(TEXT("replacement snapshot substitution cannot retire old surface"),
		DriftedNew.IsValid()
			&& DriftedNew.GetStatus()
				== EArcOwnerHandoffStatus::NewSurfaceMismatch
			&& OldSurface.RetirementCallCount == 0
			&& Owner.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffInvariantTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoff.InvariantAndReentrant",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffInvariantTest::RunTest(
	const FString&)
{
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture BrokenFixture;
	FFakeArcPreviewHandoffSurface BrokenOld(
		FGuid(0xF4750001, 0xF4750002, 0xF4750003, 0xF4750004),
		Consumer);
	FFakeArcPreviewHandoffSurface BrokenNew(
		FGuid(0xF4751001, 0xF4751002, 0xF4751003, 0xF4751004),
		Consumer);
	FArcCompositionOwner BrokenOwner;
	if (!StartArcPreviewCompositionOwner(
			*this, TEXT("ArcPreviewOwnerHandoffBroken"),
			BrokenFixture, BrokenOwner, BrokenOld))
	{
		return false;
	}
	UpdateArcPreviewCompositionOwner(
		BrokenOwner, MakeArcPreviewChoice(false), BrokenFixture);
	BrokenNew.ForceCursor(BrokenOwner.GetSurfaceCursor());
	FArcSurfaceOwnershipTicket BrokenTicket;
	if (!BuildArcOwnerHandoffTicket(
			*this, BrokenOwner, BrokenNew,
			EArcSurfaceRecreationAction::AdoptExact, BrokenTicket))
	{
		return false;
	}
	BrokenOld.SetRetirementMode(
		FFakeArcPreviewHandoffSurface::ERetirementMode::InvalidAfterMutation);
	FArcOwnerHandoff BrokenCoordinator;
	const auto Broken = BrokenCoordinator.Execute(
		BrokenOwner, BrokenTicket, BrokenOld, BrokenNew);
	TestTrue(TEXT("mutation with invalid evidence fails closed for recovery"),
		Broken.IsValid()
			&& Broken.GetStatus()
				== EArcOwnerHandoffStatus::RetirementResponseInvalid
			&& Broken.DidCallRetirement()
			&& Broken.NeedsManualRecovery()
			&& !Broken.IsOwnerValidAfter()
			&& !Broken.HasReceipt());

	FThrownLifecycleFixture ReentrantFixture;
	FFakeArcPreviewHandoffSurface ReentrantOld(
		FGuid(0xF4752001, 0xF4752002, 0xF4752003, 0xF4752004),
		Consumer);
	FFakeArcPreviewHandoffSurface ReentrantNew(
		FGuid(0xF4753001, 0xF4753002, 0xF4753003, 0xF4753004),
		Consumer);
	FArcCompositionOwner ReentrantOwner;
	if (!StartArcPreviewCompositionOwner(
			*this, TEXT("ArcPreviewOwnerHandoffReentrant"),
			ReentrantFixture, ReentrantOwner, ReentrantOld))
	{
		return false;
	}
	UpdateArcPreviewCompositionOwner(
		ReentrantOwner, MakeArcPreviewChoice(false), ReentrantFixture);
	ReentrantNew.ForceCursor(ReentrantOwner.GetSurfaceCursor());
	FArcSurfaceOwnershipTicket ReentrantTicket;
	if (!BuildArcOwnerHandoffTicket(
			*this, ReentrantOwner, ReentrantNew,
			EArcSurfaceRecreationAction::AdoptExact, ReentrantTicket))
	{
		return false;
	}
	FArcOwnerHandoff ReentrantCoordinator;
	ReentrantOld.SetReentrantRetirement(
		ReentrantCoordinator,
		ReentrantOwner,
		ReentrantTicket,
		ReentrantNew);
	const auto Outer = ReentrantCoordinator.Execute(
		ReentrantOwner,
		ReentrantTicket,
		ReentrantOld,
		ReentrantNew);
	TestTrue(TEXT("outer guarded retirement still commits once"),
		Outer.IsValid() && Outer.WasCommitted()
			&& ReentrantOld.RetirementCallCount == 1
			&& ReentrantOwner.IsValid());
	TestTrue(TEXT("retirement callback cannot re-enter Owner handoff"),
		ReentrantOld.ReentrantResult.IsValid()
			&& ReentrantOld.ReentrantResult.GetStatus()
				== EArcOwnerHandoffStatus::OperationInProgress
			&& !ReentrantOld.ReentrantResult.DidCallRetirement()
			&& !ReentrantOld.ReentrantResult.HasReceipt());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryEvidenceTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecovery.EvidenceContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryEvidenceTest::RunTest(
	const FString&)
{
	FArcOwnerHandoffRecoveryCheckpoint InvalidCheckpoint;
	FString InvalidDiagnostic;
	const bool bInvalidCreated =
		FArcOwnerHandoffRecoveryCheckpoint::TryCreate(
			FArcOwnerHandoffResult(),
			InvalidCheckpoint,
			InvalidDiagnostic);
	TestTrue(TEXT("ordinary or invalid handoff evidence cannot become recovery authority"),
		!bInvalidCreated && !InvalidCheckpoint.IsValid()
			&& !InvalidDiagnostic.IsEmpty());

	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture Fixture;
	FFakeArcPreviewHandoffSurface OldSurface(
		FGuid(0xF4800001, 0xF4800002, 0xF4800003, 0xF4800004),
		Consumer);
	FFakeArcPreviewHandoffSurface NewSurface(
		FGuid(0xF4801001, 0xF4801002, 0xF4801003, 0xF4801004),
		Consumer);
	FArcCompositionOwner Owner;
	FArcOwnerHandoffResult Failed;
	FArcOwnerHandoffRecoveryCheckpoint Checkpoint;
	if (!BuildArcOwnerHandoffRecoveryCheckpoint(
			*this,
			TEXT("ArcPreviewOwnerHandoffRecoveryEvidence"),
			Fixture,
			Owner,
			OldSurface,
			NewSurface,
			Failed,
			Checkpoint))
	{
		return false;
	}
	FArcOwnerHandoffRecoveryCheckpoint Duplicate;
	FString Diagnostic;
	const bool bDuplicateCreated =
		FArcOwnerHandoffRecoveryCheckpoint::TryCreate(
			Failed, Duplicate, Diagnostic);
	TestTrue(TEXT("manual-recovery boundary seals deterministic immutable evidence"),
		bDuplicateCreated && Checkpoint.IsValid()
			&& Checkpoint.MatchesFailedHandoff(Failed)
			&& Duplicate.GetCheckpointId() == Checkpoint.GetCheckpointId()
			&& Checkpoint.GetTransitionTicket().GetTicketId()
				== Failed.GetTransitionTicket().GetTicketId()
			&& Checkpoint.GetRetiredSurfaceCursor().IsEmpty()
			&& Checkpoint.GetSurfaceCursor().IsVisible());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryCommitTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecovery.ZeroMutationCommit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryCommitTest::RunTest(
	const FString&)
{
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture Fixture;
	FFakeArcPreviewHandoffSurface OldSurface(
		FGuid(0xF4810001, 0xF4810002, 0xF4810003, 0xF4810004),
		Consumer);
	FFakeArcPreviewHandoffSurface NewSurface(
		FGuid(0xF4811001, 0xF4811002, 0xF4811003, 0xF4811004),
		Consumer);
	FArcCompositionOwner Owner;
	FArcOwnerHandoffResult Failed;
	FArcOwnerHandoffRecoveryCheckpoint Checkpoint;
	if (!BuildArcOwnerHandoffRecoveryCheckpoint(
			*this,
			TEXT("ArcPreviewOwnerHandoffRecoveryCommit"),
			Fixture,
			Owner,
			OldSurface,
			NewSurface,
			Failed,
			Checkpoint))
	{
		return false;
	}
	const int32 OldMutationCalls = OldSurface.MutationCallCount;
	const int32 OldRetirementCalls = OldSurface.RetirementCallCount;
	const int32 NewMutationCalls = NewSurface.MutationCallCount;
	FArcOwnerHandoffRecovery Recovery;
	const auto Recovered = Recovery.Execute(
		Owner, Checkpoint, OldSurface, NewSurface);
	TestTrue(TEXT("fresh dual-surface proof commits only Owner and Adapter pointers"),
		Recovered.IsValid() && Recovered.WasRecovered()
			&& Recovered.HasReceipt() && !Recovered.DidMutateSurface()
			&& Recovered.GetSurfaceSnapshotReadCount() == 2
			&& OldSurface.MutationCallCount == OldMutationCalls
			&& OldSurface.RetirementCallCount == OldRetirementCalls
			&& NewSurface.MutationCallCount == NewMutationCalls
			&& Owner.IsValid() && Owner.IsSynchronized()
			&& !Owner.GetLastSurfaceHandoffReceipt().IsValid()
			&& Owner.GetLastSurfaceHandoffRecoveryReceipt().GetReceiptId()
				== Recovered.GetReceipt().GetReceiptId());

	const auto Replace = UpdateArcPreviewCompositionOwner(
		Owner, MakeArcPreviewChoice(true), Fixture);
	TestTrue(TEXT("post-recovery dispatch reaches only the adopted replacement"),
		Replace.IsValid() && Replace.WasApplied()
			&& OldSurface.MutationCallCount == OldMutationCalls
			&& NewSurface.MutationCallCount == NewMutationCalls + 1
			&& Owner.IsValid() && Owner.IsSynchronized());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryReplayTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecovery.IdempotentReplay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryReplayTest::RunTest(
	const FString&)
{
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture Fixture;
	FFakeArcPreviewHandoffSurface OldSurface(
		FGuid(0xF4820001, 0xF4820002, 0xF4820003, 0xF4820004),
		Consumer);
	FFakeArcPreviewHandoffSurface NewSurface(
		FGuid(0xF4821001, 0xF4821002, 0xF4821003, 0xF4821004),
		Consumer);
	FArcCompositionOwner Owner;
	FArcOwnerHandoffResult Failed;
	FArcOwnerHandoffRecoveryCheckpoint Checkpoint;
	if (!BuildArcOwnerHandoffRecoveryCheckpoint(
			*this,
			TEXT("ArcPreviewOwnerHandoffRecoveryReplay"),
			Fixture,
			Owner,
			OldSurface,
			NewSurface,
			Failed,
			Checkpoint))
	{
		return false;
	}
	FArcOwnerHandoffRecovery Recovery;
	const auto Recovered = Recovery.Execute(
		Owner, Checkpoint, OldSurface, NewSurface);
	const int32 OldIdentityReads = OldSurface.IdentityQueryCount;
	const int32 OldConsumerReads = OldSurface.ConsumerQueryCount;
	const int32 OldCursorReads = OldSurface.CursorQueryCount;
	const int32 OldMutationCalls = OldSurface.MutationCallCount;
	const int32 OldRetirementCalls = OldSurface.RetirementCallCount;
	const auto Replay = Recovery.Execute(
		Owner, Checkpoint, OldSurface, NewSurface);
	TestTrue(TEXT("latest checkpoint replay reads only the current replacement"),
		Recovered.WasRecovered() && Replay.IsValid() && Replay.IsReplay()
			&& Replay.GetSurfaceSnapshotReadCount() == 1
			&& Replay.GetReceipt().GetReceiptId()
				== Recovered.GetReceipt().GetReceiptId()
			&& OldSurface.IdentityQueryCount == OldIdentityReads
			&& OldSurface.ConsumerQueryCount == OldConsumerReads
			&& OldSurface.CursorQueryCount == OldCursorReads
			&& OldSurface.MutationCallCount == OldMutationCalls
			&& OldSurface.RetirementCallCount == OldRetirementCalls
			&& !Replay.DidMutateSurface());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryOldFenceTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecovery.OldSurfaceFences",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryOldFenceTest::RunTest(
	const FString&)
{
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture Fixture;
	FFakeArcPreviewHandoffSurface OldSurface(
		FGuid(0xF4830001, 0xF4830002, 0xF4830003, 0xF4830004),
		Consumer);
	FFakeArcPreviewHandoffSurface WrongOld(
		FGuid(0xF4830101, 0xF4830102, 0xF4830103, 0xF4830104),
		Consumer);
	FFakeArcPreviewHandoffSurface NewSurface(
		FGuid(0xF4831001, 0xF4831002, 0xF4831003, 0xF4831004),
		Consumer);
	FArcCompositionOwner Owner;
	FArcOwnerHandoffResult Failed;
	FArcOwnerHandoffRecoveryCheckpoint Checkpoint;
	if (!BuildArcOwnerHandoffRecoveryCheckpoint(
			*this,
			TEXT("ArcPreviewOwnerHandoffRecoveryOldFence"),
			Fixture,
			Owner,
			OldSurface,
			NewSurface,
			Failed,
			Checkpoint))
	{
		return false;
	}
	const int32 WrongOldIdentityReads = WrongOld.IdentityQueryCount;
	const int32 NewIdentityReads = NewSurface.IdentityQueryCount;
	const int32 NewConsumerReads = NewSurface.ConsumerQueryCount;
	const int32 NewCursorReads = NewSurface.CursorQueryCount;
	FArcOwnerHandoffRecovery Recovery;
	const auto WrongPointer = Recovery.Execute(
		Owner, Checkpoint, WrongOld, NewSurface);
	TestTrue(TEXT("old pointer substitution rejects before either supplied surface is read"),
		WrongPointer.IsValid()
			&& WrongPointer.GetStatus()
				== EArcOwnerHandoffRecoveryStatus::OldSurfaceMismatch
			&& WrongPointer.GetSurfaceSnapshotReadCount() == 0
			&& WrongOld.IdentityQueryCount == WrongOldIdentityReads
			&& NewSurface.IdentityQueryCount == NewIdentityReads);

	OldSurface.ForceCursor(Checkpoint.GetPreviousSurfaceCursor());
	const auto RestoredOld = Recovery.Execute(
		Owner, Checkpoint, OldSurface, NewSurface);
	TestTrue(TEXT("retired old surface must still be exactly Empty before replacement reads"),
		RestoredOld.IsValid()
			&& RestoredOld.GetStatus()
				== EArcOwnerHandoffRecoveryStatus::OldSnapshotMismatch
			&& RestoredOld.GetSurfaceSnapshotReadCount() == 1
			&& NewSurface.IdentityQueryCount == NewIdentityReads
			&& NewSurface.ConsumerQueryCount == NewConsumerReads
			&& NewSurface.CursorQueryCount == NewCursorReads
			&& OldSurface.RetirementCallCount == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoverySnapshotFenceTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecovery.NewAndAuthorityFences",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoverySnapshotFenceTest::RunTest(
	const FString&)
{
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	const FGuid OldId(0xF4840001, 0xF4840002, 0xF4840003, 0xF4840004);
	const FGuid NewId(0xF4841001, 0xF4841002, 0xF4841003, 0xF4841004);
	FThrownLifecycleFixture FixtureA;
	FFakeArcPreviewHandoffSurface OldA(OldId, Consumer);
	FFakeArcPreviewHandoffSurface NewA(NewId, Consumer);
	FArcCompositionOwner OwnerA;
	FArcOwnerHandoffResult FailedA;
	FArcOwnerHandoffRecoveryCheckpoint CheckpointA;
	if (!BuildArcOwnerHandoffRecoveryCheckpoint(
			*this,
			TEXT("ArcPreviewOwnerHandoffRecoverySnapshotA"),
			FixtureA,
			OwnerA,
			OldA,
			NewA,
			FailedA,
			CheckpointA))
	{
		return false;
	}
	NewA.SetConsumerDefinitionId(
		FName(TEXT("Renderer.ArcPreview.Foreign.r1")));
	FArcOwnerHandoffRecovery Recovery;
	const auto NewDrift = Recovery.Execute(
		OwnerA, CheckpointA, OldA, NewA);
	TestTrue(TEXT("replacement snapshot drift cannot commit recovered pointers"),
		NewDrift.IsValid()
			&& NewDrift.GetStatus()
				== EArcOwnerHandoffRecoveryStatus::NewSnapshotMismatch
			&& NewDrift.GetSurfaceSnapshotReadCount() == 2
			&& !NewDrift.HasReceipt() && !OwnerA.IsValid());

	FThrownLifecycleFixture FixtureB;
	FFakeArcPreviewHandoffSurface OldB(OldId, Consumer);
	FFakeArcPreviewHandoffSurface NewB(NewId, Consumer);
	FArcCompositionOwner OwnerB;
	FArcOwnerHandoffResult FailedB;
	FArcOwnerHandoffRecoveryCheckpoint CheckpointB;
	if (!BuildArcOwnerHandoffRecoveryCheckpoint(
			*this,
			TEXT("ArcPreviewOwnerHandoffRecoverySnapshotB"),
			FixtureB,
			OwnerB,
			OldB,
			NewB,
			FailedB,
			CheckpointB))
	{
		return false;
	}
	NewB.ForceCursor(CheckpointA.GetSurfaceCursor());
	const auto ForeignAuthority = Recovery.Execute(
		OwnerB, CheckpointA, OldB, NewB);
	TestTrue(TEXT("matching physical instances cannot substitute another Run authority"),
		OwnerB.GetRunId() != CheckpointA.GetTransitionTicket().GetRunId()
			&& ForeignAuthority.IsValid()
			&& ForeignAuthority.GetStatus()
				== EArcOwnerHandoffRecoveryStatus::AuthoritySnapshotMismatch
			&& ForeignAuthority.GetSurfaceSnapshotReadCount() == 2
			&& !ForeignAuthority.HasReceipt() && !OwnerB.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryReentrantTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecovery.ReentrantAndReceiptRotation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryReentrantTest::RunTest(
	const FString&)
{
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture Fixture;
	FFakeArcPreviewHandoffSurface OldSurface(
		FGuid(0xF4850001, 0xF4850002, 0xF4850003, 0xF4850004),
		Consumer);
	FFakeArcPreviewHandoffSurface NewSurface(
		FGuid(0xF4851001, 0xF4851002, 0xF4851003, 0xF4851004),
		Consumer);
	FArcCompositionOwner Owner;
	FArcOwnerHandoffResult Failed;
	FArcOwnerHandoffRecoveryCheckpoint Checkpoint;
	if (!BuildArcOwnerHandoffRecoveryCheckpoint(
			*this,
			TEXT("ArcPreviewOwnerHandoffRecoveryReentrant"),
			Fixture,
			Owner,
			OldSurface,
			NewSurface,
			Failed,
			Checkpoint))
	{
		return false;
	}
	FArcOwnerHandoffRecovery Recovery;
	OldSurface.SetReentrantRecovery(
		Recovery, Owner, Checkpoint, NewSurface);
	const auto Outer = Recovery.Execute(
		Owner, Checkpoint, OldSurface, NewSurface);
	TestTrue(TEXT("guarded outer recovery still commits one zero-mutation pointer swap"),
		Outer.IsValid() && Outer.WasRecovered() && Owner.IsValid()
			&& OldSurface.RetirementCallCount == 1);
	TestTrue(TEXT("surface query callback cannot re-enter recovery transaction"),
		OldSurface.ReentrantRecoveryResult.IsValid()
			&& OldSurface.ReentrantRecoveryResult.GetStatus()
				== EArcOwnerHandoffRecoveryStatus::OperationInProgress
			&& OldSurface.ReentrantRecoveryResult
				.GetSurfaceSnapshotReadCount() == 0
			&& !OldSurface.ReentrantRecoveryResult.HasReceipt());

	FFakeArcPreviewHandoffSurface ThirdSurface(
		FGuid(0xF4852001, 0xF4852002, 0xF4852003, 0xF4852004),
		Consumer);
	ThirdSurface.ForceCursor(Owner.GetSurfaceCursor());
	FArcSurfaceOwnershipTicket NextTicket;
	if (!BuildArcOwnerHandoffTicket(
			*this,
			Owner,
			ThirdSurface,
			EArcSurfaceRecreationAction::AdoptExact,
			NextTicket))
	{
		return false;
	}
	FArcOwnerHandoff Handoff;
	const auto NextHandoff = Handoff.Execute(
		Owner, NextTicket, NewSurface, ThirdSurface);
	TestTrue(TEXT("later normal handoff replaces rather than coexists with recovery receipt"),
		NextHandoff.IsValid() && NextHandoff.WasCommitted()
			&& Owner.GetLastSurfaceHandoffReceipt().IsExactAdopted()
			&& !Owner.GetLastSurfaceHandoffRecoveryReceipt().IsValid()
			&& Owner.IsValid() && Owner.IsSynchronized());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryJournalEvidenceTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournal.EvidenceContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryJournalEvidenceTest::
RunTest(const FString&)
{
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture Fixture;
	FFakeArcPreviewHandoffSurface OldSurface(
		FGuid(0xF4900001, 0xF4900002, 0xF4900003, 0xF4900004),
		Consumer);
	FFakeArcPreviewHandoffSurface NewSurface(
		FGuid(0xF4901001, 0xF4901002, 0xF4901003, 0xF4901004),
		Consumer);
	FArcCompositionOwner Owner;
	FArcOwnerHandoffRecoveryCheckpoint Checkpoint;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryReceipt
		Receipt;
	if (!BuildArcOwnerHandoffRecoveryJournalEvidence(
			*this,
			TEXT("ArcPreviewOwnerHandoffRecoveryJournalEvidence"),
			Fixture,
			Owner,
			OldSurface,
			NewSurface,
			Checkpoint,
			Receipt))
	{
		return false;
	}

	FArcOwnerHandoffRecoveryJournal Journal;
	const FGuid EmptyJournalId = Journal.GetJournalId();
	const auto PrematureReceipt =
		Journal.AppendRecoveryReceipt(Checkpoint, Receipt);
	const auto InvalidCheckpoint = Journal.AppendCheckpoint(
		FArcOwnerHandoffRecoveryCheckpoint());
	const auto Prepared = Journal.AppendCheckpoint(Checkpoint);
	const auto PreparedReplay = Journal.AppendCheckpoint(Checkpoint);
	FArcOwnerHandoffRecoveryJournalRecord PreparedRecord;
	const bool bReadPrepared = Journal.TryGetLatestRecord(PreparedRecord);
	const auto Committed =
		Journal.AppendRecoveryReceipt(Checkpoint, Receipt);
	const auto CommittedReplay =
		Journal.AppendRecoveryReceipt(Checkpoint, Receipt);
	FArcOwnerHandoffRecoveryJournalRecord CommittedRecord;
	const bool bReadCommitted = Journal.TryGetLatestRecord(CommittedRecord);

	TestTrue(TEXT("empty journal is valid and rejects evidence out of order"),
		EmptyJournalId.IsValid()
			&& PrematureReceipt.IsValid()
			&& PrematureReceipt.GetStatus()
				== EArcOwnerHandoffRecoveryJournalAppendStatus::
					CheckpointNotPending
			&& InvalidCheckpoint.IsValid()
			&& InvalidCheckpoint.GetStatus()
				== EArcOwnerHandoffRecoveryJournalAppendStatus::
					CheckpointInvalid
			&& Journal.IsValid());
	TestTrue(TEXT("checkpoint append is deterministic and idempotent"),
		Prepared.IsValid() && Prepared.DidAppend()
			&& Prepared.GetPreviousRecordCount() == 0
			&& Prepared.GetRecordCount() == 1
			&& PreparedReplay.IsValid() && PreparedReplay.IsReplay()
			&& PreparedReplay.GetRecord().GetRecordId()
				== Prepared.GetRecord().GetRecordId()
			&& bReadPrepared && PreparedRecord.MatchesCheckpoint(Checkpoint)
			&& PreparedRecord.GetKind()
				== EArcOwnerHandoffRecoveryJournalKind::CheckpointPrepared
			&& PreparedRecord.GetSequence() == 0
			&& !PreparedRecord.GetPreviousRecordId().IsValid()
			&& !PreparedRecord.GetRecoveryReceiptId().IsValid());
	TestTrue(TEXT("receipt append closes only the matching pending checkpoint"),
		Committed.IsValid() && Committed.DidAppend()
			&& CommittedReplay.IsValid() && CommittedReplay.IsReplay()
			&& bReadCommitted
			&& CommittedRecord.GetKind()
				== EArcOwnerHandoffRecoveryJournalKind::RecoveryCommitted
			&& CommittedRecord.GetSequence() == 1
			&& CommittedRecord.GetPreviousRecordId()
				== PreparedRecord.GetRecordId()
			&& CommittedRecord.MatchesRecoveryReceipt(
				Checkpoint, Receipt)
			&& Journal.MatchesLatestRecovery(Checkpoint, Receipt)
			&& Journal.GetLatestDisposition()
				== EArcOwnerHandoffRecoveryJournalDisposition::
					RecoveryCommitted
			&& Journal.GetRecordCount() == 2
			&& Journal.GetJournalId() != EmptyJournalId);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryJournalRoundTripTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournal.CanonicalRoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryJournalRoundTripTest::
RunTest(const FString&)
{
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture Fixture;
	FFakeArcPreviewHandoffSurface OldSurface(
		FGuid(0xF4910001, 0xF4910002, 0xF4910003, 0xF4910004),
		Consumer);
	FFakeArcPreviewHandoffSurface NewSurface(
		FGuid(0xF4911001, 0xF4911002, 0xF4911003, 0xF4911004),
		Consumer);
	FArcCompositionOwner Owner;
	FArcOwnerHandoffRecoveryCheckpoint Checkpoint;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryReceipt
		Receipt;
	if (!BuildArcOwnerHandoffRecoveryJournalEvidence(
			*this,
			TEXT("ArcPreviewOwnerHandoffRecoveryJournalRoundTrip"),
			Fixture,
			Owner,
			OldSurface,
			NewSurface,
			Checkpoint,
			Receipt))
	{
		return false;
	}

	FArcOwnerHandoffRecoveryJournal Journal;
	const auto Prepared = Journal.AppendCheckpoint(Checkpoint);
	const auto Committed =
		Journal.AppendRecoveryReceipt(Checkpoint, Receipt);
	TArray<uint8> FirstBytes;
	TArray<uint8> SecondBytes;
	const bool bFirstEncoded =
		FArcOwnerHandoffRecoveryJournalCodec::TryEncode(
			Journal, FirstBytes);
	const bool bSecondEncoded =
		FArcOwnerHandoffRecoveryJournalCodec::TryEncode(
			Journal, SecondBytes);
	const auto Decoded =
		FArcOwnerHandoffRecoveryJournalCodec::Decode(FirstBytes);
	TArray<uint8> ReencodedBytes;
	const bool bReencoded = Decoded.IsSuccess()
		&& FArcOwnerHandoffRecoveryJournalCodec::TryEncode(
			Decoded.GetJournal(), ReencodedBytes);

	FArcOwnerHandoffRecoveryJournal Empty;
	TArray<uint8> EmptyBytes;
	const bool bEmptyEncoded =
		FArcOwnerHandoffRecoveryJournalCodec::TryEncode(
			Empty, EmptyBytes);
	const auto EmptyDecoded =
		FArcOwnerHandoffRecoveryJournalCodec::Decode(EmptyBytes);

	TestTrue(TEXT("current codec emits one exact deterministic byte layout"),
		Prepared.DidAppend() && Committed.DidAppend()
			&& bFirstEncoded && bSecondEncoded
			&& FirstBytes == SecondBytes
			&& FirstBytes.Num()
				== FArcOwnerHandoffRecoveryJournalCodec::HeaderSize()
					+ 2 * FArcOwnerHandoffRecoveryJournalCodec::
						CurrentRecordSize());
	TestTrue(TEXT("current bytes round-trip into the same candidate-bound chain"),
		Decoded.IsSuccess() && !Decoded.WasMigrated()
			&& Decoded.GetStatus()
				== EArcOwnerHandoffRecoveryJournalDecodeStatus::DecodedCurrent
			&& Decoded.GetSourceSchemaVersion()
				== FArcOwnerHandoffRecoveryJournalCodec::
					CurrentSchemaVersion()
			&& Decoded.GetJournal().GetJournalId()
				== Journal.GetJournalId()
			&& Decoded.GetJournal().GetRecordCount() == 2
			&& Decoded.GetJournal().MatchesLatestRecovery(
				Checkpoint, Receipt)
			&& bReencoded && ReencodedBytes == FirstBytes);
	TestTrue(TEXT("empty journal has a canonical current-schema representation"),
		bEmptyEncoded
			&& EmptyBytes.Num()
				== FArcOwnerHandoffRecoveryJournalCodec::HeaderSize()
			&& EmptyDecoded.IsSuccess()
			&& EmptyDecoded.GetJournal().IsValid()
			&& EmptyDecoded.GetJournal().GetRecordCount() == 0
			&& EmptyDecoded.GetJournal().GetJournalId()
				== Empty.GetJournalId());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryJournalCorruptionTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournal.CorruptionAndTruncation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryJournalCorruptionTest::
RunTest(const FString&)
{
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture Fixture;
	FFakeArcPreviewHandoffSurface OldSurface(
		FGuid(0xF4920001, 0xF4920002, 0xF4920003, 0xF4920004),
		Consumer);
	FFakeArcPreviewHandoffSurface NewSurface(
		FGuid(0xF4921001, 0xF4921002, 0xF4921003, 0xF4921004),
		Consumer);
	FArcCompositionOwner Owner;
	FArcOwnerHandoffRecoveryCheckpoint Checkpoint;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryReceipt
		Receipt;
	if (!BuildArcOwnerHandoffRecoveryJournalEvidence(
			*this,
			TEXT("ArcPreviewOwnerHandoffRecoveryJournalCorruption"),
			Fixture,
			Owner,
			OldSurface,
			NewSurface,
			Checkpoint,
			Receipt))
	{
		return false;
	}

	FArcOwnerHandoffRecoveryJournal Journal;
	Journal.AppendCheckpoint(Checkpoint);
	Journal.AppendRecoveryReceipt(Checkpoint, Receipt);
	TArray<uint8> Canonical;
	if (!FArcOwnerHandoffRecoveryJournalCodec::TryEncode(
			Journal, Canonical))
	{
		return false;
	}

	int32 AcceptedCorruptions = 0;
	for (int32 Index = 0; Index < Canonical.Num(); ++Index)
	{
		TArray<uint8> Corrupted = Canonical;
		Corrupted[Index] ^= 0x01;
		if (FArcOwnerHandoffRecoveryJournalCodec::Decode(Corrupted)
			.IsSuccess())
		{
			++AcceptedCorruptions;
		}
	}
	int32 AcceptedTruncations = 0;
	for (int32 Length = 0; Length < Canonical.Num(); ++Length)
	{
		TArray<uint8> Truncated;
		Truncated.Append(Canonical.GetData(), Length);
		if (FArcOwnerHandoffRecoveryJournalCodec::Decode(Truncated)
			.IsSuccess())
		{
			++AcceptedTruncations;
		}
	}
	TArray<uint8> Trailing = Canonical;
	Trailing.Add(0x00);
	const auto TrailingResult =
		FArcOwnerHandoffRecoveryJournalCodec::Decode(Trailing);

	TestEqual(
		TEXT("every one-bit byte corruption is rejected"),
		AcceptedCorruptions,
		0);
	TestEqual(
		TEXT("every strict prefix truncation is rejected"),
		AcceptedTruncations,
		0);
	TestTrue(TEXT("trailing bytes are not silently ignored"),
		!TrailingResult.IsSuccess()
			&& TrailingResult.GetStatus()
				== EArcOwnerHandoffRecoveryJournalDecodeStatus::SizeMismatch);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryJournalSelectionTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournal.LatestSelectionAndConflicts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryJournalSelectionTest::
RunTest(const FString&)
{
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture FixtureA;
	FFakeArcPreviewHandoffSurface OldA(
		FGuid(0xF4930001, 0xF4930002, 0xF4930003, 0xF4930004),
		Consumer);
	FFakeArcPreviewHandoffSurface NewA(
		FGuid(0xF4931001, 0xF4931002, 0xF4931003, 0xF4931004),
		Consumer);
	FArcCompositionOwner OwnerA;
	FArcOwnerHandoffRecoveryCheckpoint CheckpointA;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryReceipt
		ReceiptA;
	if (!BuildArcOwnerHandoffRecoveryJournalEvidence(
			*this,
			TEXT("ArcPreviewOwnerHandoffRecoveryJournalSelectionA"),
			FixtureA,
			OwnerA,
			OldA,
			NewA,
			CheckpointA,
			ReceiptA))
	{
		return false;
	}

	FThrownLifecycleFixture FixtureB;
	FFakeArcPreviewHandoffSurface OldB(
		FGuid(0xF4932001, 0xF4932002, 0xF4932003, 0xF4932004),
		Consumer);
	FFakeArcPreviewHandoffSurface NewB(
		FGuid(0xF4933001, 0xF4933002, 0xF4933003, 0xF4933004),
		Consumer);
	FArcCompositionOwner OwnerB;
	FArcOwnerHandoffRecoveryCheckpoint CheckpointB;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryReceipt
		ReceiptB;
	if (!BuildArcOwnerHandoffRecoveryJournalEvidence(
			*this,
			TEXT("ArcPreviewOwnerHandoffRecoveryJournalSelectionB"),
			FixtureB,
			OwnerB,
			OldB,
			NewB,
			CheckpointB,
			ReceiptB))
	{
		return false;
	}

	FArcOwnerHandoffRecoveryJournal Journal;
	const auto PreparedA = Journal.AppendCheckpoint(CheckpointA);
	const FGuid PendingAId = Journal.GetJournalId();
	const auto PendingConflict = Journal.AppendCheckpoint(CheckpointB);
	const auto CommittedA =
		Journal.AppendRecoveryReceipt(CheckpointA, ReceiptA);
	const auto CompletedReplay = Journal.AppendCheckpoint(CheckpointA);
	const auto ReceiptWithoutPending =
		Journal.AppendRecoveryReceipt(CheckpointB, ReceiptB);
	const auto PreparedB = Journal.AppendCheckpoint(CheckpointB);
	const FGuid PendingBId = Journal.GetJournalId();
	const auto ForeignReceipt =
		Journal.AppendRecoveryReceipt(CheckpointA, ReceiptA);
	const auto CommittedB =
		Journal.AppendRecoveryReceipt(CheckpointB, ReceiptB);

	TestTrue(TEXT("a foreign checkpoint cannot replace the pending tail"),
		PreparedA.DidAppend()
			&& PendingConflict.IsValid()
			&& PendingConflict.GetStatus()
				== EArcOwnerHandoffRecoveryJournalAppendStatus::
					PendingCheckpointConflict
			&& PendingConflict.GetRecordCount() == 1
			&& PendingAId.IsValid());
	TestTrue(TEXT("completed checkpoint replay is idempotent but not reopened"),
		CommittedA.DidAppend()
			&& CompletedReplay.IsReplay()
			&& CompletedReplay.GetRecord().MatchesRecoveryReceipt(
				CheckpointA, ReceiptA)
			&& ReceiptWithoutPending.IsValid()
			&& ReceiptWithoutPending.GetStatus()
				== EArcOwnerHandoffRecoveryJournalAppendStatus::
					CheckpointNotPending
			&& Journal.IsValid());
	TestTrue(TEXT("latest pending checkpoint alone can accept a receipt"),
		PreparedB.DidAppend()
			&& PendingBId.IsValid() && PendingBId != PendingAId
			&& ForeignReceipt.IsValid()
			&& ForeignReceipt.GetStatus()
				== EArcOwnerHandoffRecoveryJournalAppendStatus::
					PendingCheckpointConflict
			&& ForeignReceipt.GetRecordCount() == 3
			&& CommittedB.DidAppend()
			&& Journal.GetRecordCount() == 4
			&& Journal.MatchesLatestRecovery(CheckpointB, ReceiptB)
			&& !Journal.MatchesLatestRecovery(CheckpointA, ReceiptA));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryJournalMigrationTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournal.PreviousSchemaMigration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryJournalMigrationTest::
RunTest(const FString&)
{
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture Fixture;
	FFakeArcPreviewHandoffSurface OldSurface(
		FGuid(0xF4940001, 0xF4940002, 0xF4940003, 0xF4940004),
		Consumer);
	FFakeArcPreviewHandoffSurface NewSurface(
		FGuid(0xF4941001, 0xF4941002, 0xF4941003, 0xF4941004),
		Consumer);
	FArcCompositionOwner Owner;
	FArcOwnerHandoffRecoveryCheckpoint Checkpoint;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryReceipt
		Receipt;
	if (!BuildArcOwnerHandoffRecoveryJournalEvidence(
			*this,
			TEXT("ArcPreviewOwnerHandoffRecoveryJournalMigration"),
			Fixture,
			Owner,
			OldSurface,
			NewSurface,
			Checkpoint,
			Receipt))
	{
		return false;
	}

	FArcOwnerHandoffRecoveryJournal Pending;
	Pending.AppendCheckpoint(Checkpoint);
	TArray<uint8> PreviousBytes;
	const bool bPreviousEncoded =
		FArcOwnerHandoffRecoveryJournalCodec::TryEncodePreviousSchema(
			Pending, PreviousBytes);
	const auto Migrated =
		FArcOwnerHandoffRecoveryJournalCodec::Decode(PreviousBytes);
	TArray<uint8> CurrentBytes;
	const bool bCurrentEncoded = Migrated.IsSuccess()
		&& FArcOwnerHandoffRecoveryJournalCodec::TryEncode(
			Migrated.GetJournal(), CurrentBytes);
	const auto Current =
		FArcOwnerHandoffRecoveryJournalCodec::Decode(CurrentBytes);

	int32 AcceptedPreviousCorruptions = 0;
	for (int32 Index = 0; Index < PreviousBytes.Num(); ++Index)
	{
		TArray<uint8> Corrupted = PreviousBytes;
		Corrupted[Index] ^= 0x01;
		if (FArcOwnerHandoffRecoveryJournalCodec::Decode(Corrupted)
			.IsSuccess())
		{
			++AcceptedPreviousCorruptions;
		}
	}

	FArcOwnerHandoffRecoveryJournal Completed = Pending;
	Completed.AppendRecoveryReceipt(Checkpoint, Receipt);
	TArray<uint8> UnsupportedPrevious;
	const bool bCompletedDowngraded =
		FArcOwnerHandoffRecoveryJournalCodec::TryEncodePreviousSchema(
			Completed, UnsupportedPrevious);

	TestTrue(TEXT("N-1 pending record migrates into the current chain model"),
		bPreviousEncoded
			&& PreviousBytes.Num()
				== FArcOwnerHandoffRecoveryJournalCodec::HeaderSize()
					+ FArcOwnerHandoffRecoveryJournalCodec::
						PreviousRecordSize()
			&& Migrated.IsSuccess() && Migrated.WasMigrated()
			&& Migrated.GetStatus()
				== EArcOwnerHandoffRecoveryJournalDecodeStatus::
					MigratedPrevious
			&& Migrated.GetSourceSchemaVersion()
				== FArcOwnerHandoffRecoveryJournalCodec::
					PreviousSchemaVersion()
			&& Migrated.GetJournal().MatchesLatestCheckpoint(Checkpoint));
	TestTrue(TEXT("migrated journal re-encodes canonically as current schema"),
		bCurrentEncoded
			&& CurrentBytes.Num()
				== FArcOwnerHandoffRecoveryJournalCodec::HeaderSize()
					+ FArcOwnerHandoffRecoveryJournalCodec::
						CurrentRecordSize()
			&& Current.IsSuccess() && !Current.WasMigrated()
			&& Current.GetJournal().GetJournalId()
				== Pending.GetJournalId());
	TestEqual(TEXT("N-1 digest rejects every one-bit byte corruption"),
		AcceptedPreviousCorruptions, 0);
	TestTrue(TEXT("N-1 writer refuses a completed or multi-record shape"),
		!bCompletedDowngraded && UnsupportedPrevious.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryJournalCapacityTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournal.BoundedAppendOnly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryJournalCapacityTest::
RunTest(const FString&)
{
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FArcOwnerHandoffRecoveryJournal Journal;
	TArray<FGuid> AppendedRecordIds;
	for (int32 Cycle = 0;
		Cycle < FArcOwnerHandoffRecoveryJournal::MaxRecordCount() / 2;
		++Cycle)
	{
		FThrownLifecycleFixture Fixture;
		FFakeArcPreviewHandoffSurface OldSurface(
			FGuid::NewGuid(), Consumer);
		FFakeArcPreviewHandoffSurface NewSurface(
			FGuid::NewGuid(), Consumer);
		FArcCompositionOwner Owner;
		FArcOwnerHandoffRecoveryCheckpoint Checkpoint;
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryReceipt
			Receipt;
		const FString Label = FString::Printf(
			TEXT("ArcPreviewOwnerHandoffRecoveryJournalCapacity%d"),
			Cycle);
		if (!BuildArcOwnerHandoffRecoveryJournalEvidence(
				*this,
				*Label,
				Fixture,
				Owner,
				OldSurface,
				NewSurface,
				Checkpoint,
				Receipt))
		{
			return false;
		}
		const auto Prepared = Journal.AppendCheckpoint(Checkpoint);
		const auto Committed =
			Journal.AppendRecoveryReceipt(Checkpoint, Receipt);
		if (!Prepared.DidAppend() || !Committed.DidAppend())
		{
			AddError(TEXT("bounded journal could not append one valid recovery pair"));
			return false;
		}
		AppendedRecordIds.Add(Prepared.GetRecord().GetRecordId());
		AppendedRecordIds.Add(Committed.GetRecord().GetRecordId());
	}

	const FGuid FullJournalId = Journal.GetJournalId();
	FThrownLifecycleFixture OverflowFixture;
	FFakeArcPreviewHandoffSurface OverflowOld(FGuid::NewGuid(), Consumer);
	FFakeArcPreviewHandoffSurface OverflowNew(FGuid::NewGuid(), Consumer);
	FArcCompositionOwner OverflowOwner;
	FArcOwnerHandoffResult OverflowFailed;
	FArcOwnerHandoffRecoveryCheckpoint OverflowCheckpoint;
	if (!BuildArcOwnerHandoffRecoveryCheckpoint(
			*this,
			TEXT("ArcPreviewOwnerHandoffRecoveryJournalOverflow"),
			OverflowFixture,
			OverflowOwner,
			OverflowOld,
			OverflowNew,
			OverflowFailed,
			OverflowCheckpoint))
	{
		return false;
	}
	const auto Overflow = Journal.AppendCheckpoint(OverflowCheckpoint);

	bool bEarlierRecordsUnchanged =
		AppendedRecordIds.Num()
		== FArcOwnerHandoffRecoveryJournal::MaxRecordCount();
	for (int32 Index = 0;
		bEarlierRecordsUnchanged && Index < AppendedRecordIds.Num();
		++Index)
	{
		FArcOwnerHandoffRecoveryJournalRecord Record;
		bEarlierRecordsUnchanged = Journal.TryGetRecordAt(Index, Record)
			&& Record.GetRecordId() == AppendedRecordIds[Index];
	}
	TArray<uint8> FullBytes;
	const bool bFullEncoded =
		FArcOwnerHandoffRecoveryJournalCodec::TryEncode(
			Journal, FullBytes);
	const auto FullDecoded =
		FArcOwnerHandoffRecoveryJournalCodec::Decode(FullBytes);

	TestTrue(TEXT("journal reaches its explicit bound with one intact hash chain"),
		Journal.IsValid()
			&& Journal.GetRecordCount()
				== FArcOwnerHandoffRecoveryJournal::MaxRecordCount()
			&& Journal.GetLatestDisposition()
				== EArcOwnerHandoffRecoveryJournalDisposition::
					RecoveryCommitted
			&& FullJournalId.IsValid()
			&& bEarlierRecordsUnchanged);
	TestTrue(TEXT("capacity rejection neither evicts nor rewrites prior evidence"),
		Overflow.IsValid()
			&& Overflow.GetStatus()
				== EArcOwnerHandoffRecoveryJournalAppendStatus::
					CapacityExceeded
			&& Overflow.GetPreviousRecordCount()
				== FArcOwnerHandoffRecoveryJournal::MaxRecordCount()
			&& Overflow.GetRecordCount()
				== FArcOwnerHandoffRecoveryJournal::MaxRecordCount()
			&& Journal.GetJournalId() == FullJournalId
			&& bEarlierRecordsUnchanged);
	TestTrue(TEXT("maximum-size journal remains canonical and decodable"),
		bFullEncoded
			&& FullBytes.Num()
				== FArcOwnerHandoffRecoveryJournalCodec::HeaderSize()
					+ FArcOwnerHandoffRecoveryJournal::MaxRecordCount()
						* FArcOwnerHandoffRecoveryJournalCodec::
							CurrentRecordSize()
			&& FullDecoded.IsSuccess()
			&& FullDecoded.GetJournal().GetJournalId() == FullJournalId
			&& FullDecoded.GetJournal().GetRecordCount()
				== FArcOwnerHandoffRecoveryJournal::MaxRecordCount());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryPayloadEvidenceTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadEnvelope.EvidenceAndJournalBinding",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryPayloadEvidenceTest::
RunTest(const FString&)
{
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture Fixture;
	FFakeArcPreviewHandoffSurface OldSurface(
		FGuid(0xF4A00001, 0xF4A00002, 0xF4A00003, 0xF4A00004),
		Consumer);
	FFakeArcPreviewHandoffSurface NewSurface(
		FGuid(0xF4A01001, 0xF4A01002, 0xF4A01003, 0xF4A01004),
		Consumer);
	FArcCompositionOwner Owner;
	FArcOwnerHandoffRecoveryCheckpoint Checkpoint;
	FArcOwnerHandoffRecoveryJournal Journal;
	FArcOwnerHandoffRecoveryPayloadEnvelope Envelope;
	TArray<uint8> FirstBytes;
	if (!BuildArcOwnerHandoffRecoveryPayloadEvidence(
			*this,
			TEXT("ArcPreviewRecoveryPayloadEvidence"),
			Fixture,
			Owner,
			OldSurface,
			NewSurface,
			Checkpoint,
			Journal,
			Envelope,
			FirstBytes))
	{
		return false;
	}
	const int32 OldRetirementsBefore = OldSurface.RetirementCallCount;
	const int32 OldMutationsBefore = OldSurface.MutationCallCount;
	const int32 NewMutationsBefore = NewSurface.MutationCallCount;

	FArcOwnerHandoffRecoveryPayloadEnvelope ReplayEnvelope;
	TArray<uint8> ReplayBytes;
	const bool bReplayWrapped =
		FArcOwnerHandoffRecoveryPayloadEnvelope::TryWrap(
			Checkpoint, Journal, ReplayEnvelope);
	const bool bReplayEncoded = bReplayWrapped
		&& FArcOwnerHandoffRecoveryPayloadCodec::TryEncode(
			ReplayEnvelope, ReplayBytes);
	FArcOwnerHandoffRecoveryPayloadEnvelope InvalidEnvelope;
	FArcOwnerHandoffRecoveryJournal EmptyJournal;
	const bool bWrappedAgainstEmpty =
		FArcOwnerHandoffRecoveryPayloadEnvelope::TryWrap(
			Checkpoint, EmptyJournal, InvalidEnvelope);

	TestTrue(TEXT("pending journal seals one deterministic payload envelope"),
		Envelope.IsValid() && Envelope.MatchesJournal(Journal)
			&& Envelope.GetSchemaVersion()
				== FArcOwnerHandoffRecoveryPayloadCodec::
					CurrentSchemaVersion()
			&& Envelope.GetJournalId() == Journal.GetJournalId()
			&& Envelope.GetCheckpoint().GetCheckpointId()
				== Checkpoint.GetCheckpointId()
			&& Envelope.GetPayloadDigest().IsValid()
			&& Envelope.GetEnvelopeId().IsValid());
	TestTrue(TEXT("same evidence emits byte-identical canonical payload"),
		bReplayWrapped && bReplayEncoded
			&& ReplayEnvelope.Matches(Envelope)
			&& ReplayBytes == FirstBytes
			&& FirstBytes.Num()
				> FArcOwnerHandoffRecoveryPayloadCodec::HeaderSize());
	TestTrue(TEXT("payload creation neither broadens scope nor touches surfaces"),
		!bWrappedAgainstEmpty && !InvalidEnvelope.IsValid()
			&& OldSurface.RetirementCallCount == OldRetirementsBefore
			&& OldSurface.MutationCallCount == OldMutationsBefore
			&& NewSurface.MutationCallCount == NewMutationsBefore);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryPayloadRoundTripTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadEnvelope.CanonicalRoundTripAndRecovery",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryPayloadRoundTripTest::
RunTest(const FString&)
{
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture Fixture;
	FFakeArcPreviewHandoffSurface OldSurface(
		FGuid(0xF4A10001, 0xF4A10002, 0xF4A10003, 0xF4A10004),
		Consumer);
	FFakeArcPreviewHandoffSurface NewSurface(
		FGuid(0xF4A11001, 0xF4A11002, 0xF4A11003, 0xF4A11004),
		Consumer);
	FArcCompositionOwner Owner;
	FArcOwnerHandoffRecoveryCheckpoint Checkpoint;
	FArcOwnerHandoffRecoveryJournal Journal;
	FArcOwnerHandoffRecoveryPayloadEnvelope Envelope;
	TArray<uint8> Bytes;
	if (!BuildArcOwnerHandoffRecoveryPayloadEvidence(
			*this,
			TEXT("ArcPreviewRecoveryPayloadRoundTrip"),
			Fixture,
			Owner,
			OldSurface,
			NewSurface,
			Checkpoint,
			Journal,
			Envelope,
			Bytes))
	{
		return false;
	}
	const int32 OldRetirementsBefore = OldSurface.RetirementCallCount;
	const int32 OldMutationsBefore = OldSurface.MutationCallCount;
	const int32 NewMutationsBefore = NewSurface.MutationCallCount;

	const auto Decoded = FArcOwnerHandoffRecoveryPayloadCodec::Decode(Bytes);
	TArray<uint8> Reencoded;
	const bool bReencoded = Decoded.IsSuccess()
		&& FArcOwnerHandoffRecoveryPayloadCodec::TryEncode(
			Decoded.GetEnvelope(), Reencoded);
	FArcOwnerHandoffRecoveryCheckpoint Restored;
	const bool bUnwrapped = Decoded.IsSuccess()
		&& Decoded.GetEnvelope().TryUnwrapForJournal(Journal, Restored);
	FArcOwnerHandoffRecovery Recovery;
	const auto Recovered = bUnwrapped
		? Recovery.Execute(Owner, Restored, OldSurface, NewSurface)
		: FArcOwnerHandoffRecoveryResult();
	const auto Committed =
		Journal.AppendRecoveryReceipt(Restored, Recovered.GetReceipt());
	FArcOwnerHandoffRecoveryCheckpoint Stale;
	const bool bStaleUnwrapped =
		Decoded.GetEnvelope().TryUnwrapForJournal(Journal, Stale);

	TestTrue(TEXT("payload round-trip restores the exact semantic checkpoint"),
		Decoded.IsSuccess()
			&& Decoded.GetStatus()
				== EArcOwnerHandoffRecoveryPayloadDecodeStatus::Decoded
			&& Decoded.GetEnvelope().Matches(Envelope)
			&& bReencoded && Reencoded == Bytes
			&& bUnwrapped && Restored.IsValid()
			&& Restored.GetCheckpointId() == Checkpoint.GetCheckpointId());
	TestTrue(TEXT("restored evidence still passes the live P20.48 authority gate"),
		Recovered.IsValid() && Recovered.WasRecovered()
			&& Recovered.HasReceipt() && !Recovered.DidMutateSurface()
			&& Recovered.GetReceipt().MatchesCheckpoint(Restored)
			&& Owner.IsValid() && Owner.IsSynchronized()
			&& OldSurface.RetirementCallCount == OldRetirementsBefore
			&& OldSurface.MutationCallCount == OldMutationsBefore
			&& NewSurface.MutationCallCount == NewMutationsBefore);
	TestTrue(TEXT("committed journal fences the previously valid payload"),
		Committed.DidAppend()
			&& Journal.GetLatestDisposition()
				== EArcOwnerHandoffRecoveryJournalDisposition::
					RecoveryCommitted
			&& !Decoded.GetEnvelope().MatchesJournal(Journal)
			&& !bStaleUnwrapped && !Stale.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryPayloadCorruptionTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadEnvelope.CorruptionTruncationAndTrailing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryPayloadCorruptionTest::
RunTest(const FString&)
{
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture Fixture;
	FFakeArcPreviewHandoffSurface OldSurface(
		FGuid(0xF4A20001, 0xF4A20002, 0xF4A20003, 0xF4A20004),
		Consumer);
	FFakeArcPreviewHandoffSurface NewSurface(
		FGuid(0xF4A21001, 0xF4A21002, 0xF4A21003, 0xF4A21004),
		Consumer);
	FArcCompositionOwner Owner;
	FArcOwnerHandoffRecoveryCheckpoint Checkpoint;
	FArcOwnerHandoffRecoveryJournal Journal;
	FArcOwnerHandoffRecoveryPayloadEnvelope Envelope;
	TArray<uint8> Canonical;
	if (!BuildArcOwnerHandoffRecoveryPayloadEvidence(
			*this,
			TEXT("ArcPreviewRecoveryPayloadCorruption"),
			Fixture,
			Owner,
			OldSurface,
			NewSurface,
			Checkpoint,
			Journal,
			Envelope,
			Canonical))
	{
		return false;
	}

	int32 AcceptedCorruptions = 0;
	for (int32 Index = 0; Index < Canonical.Num(); ++Index)
	{
		TArray<uint8> Corrupted = Canonical;
		Corrupted[Index] ^= 0x01;
		if (FArcOwnerHandoffRecoveryPayloadCodec::Decode(Corrupted)
			.IsSuccess())
		{
			++AcceptedCorruptions;
		}
	}
	int32 AcceptedTruncations = 0;
	for (int32 Length = 0; Length < Canonical.Num(); ++Length)
	{
		TArray<uint8> Truncated;
		Truncated.Append(Canonical.GetData(), Length);
		if (FArcOwnerHandoffRecoveryPayloadCodec::Decode(Truncated)
			.IsSuccess())
		{
			++AcceptedTruncations;
		}
	}
	TArray<uint8> Trailing = Canonical;
	Trailing.Add(0x00);
	const auto TrailingResult =
		FArcOwnerHandoffRecoveryPayloadCodec::Decode(Trailing);

	TestEqual(TEXT("every one-bit payload-envelope corruption is rejected"),
		AcceptedCorruptions, 0);
	TestEqual(TEXT("every strict payload-envelope prefix is rejected"),
		AcceptedTruncations, 0);
	TestTrue(TEXT("payload-envelope trailing bytes are rejected"),
		!TrailingResult.IsSuccess()
			&& TrailingResult.GetStatus()
				== EArcOwnerHandoffRecoveryPayloadDecodeStatus::SizeMismatch);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryPayloadSemanticTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadEnvelope.SemanticIdentityAndTags",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryPayloadSemanticTest::
RunTest(const FString&)
{
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture Fixture;
	FFakeArcPreviewHandoffSurface OldSurface(
		FGuid(0xF4A30001, 0xF4A30002, 0xF4A30003, 0xF4A30004),
		Consumer);
	FFakeArcPreviewHandoffSurface NewSurface(
		FGuid(0xF4A31001, 0xF4A31002, 0xF4A31003, 0xF4A31004),
		Consumer);
	FArcCompositionOwner Owner;
	FArcOwnerHandoffRecoveryCheckpoint Checkpoint;
	FArcOwnerHandoffRecoveryJournal Journal;
	FArcOwnerHandoffRecoveryPayloadEnvelope Envelope;
	TArray<uint8> Bytes;
	if (!BuildArcOwnerHandoffRecoveryPayloadEvidence(
			*this,
			TEXT("ArcPreviewRecoveryPayloadSemantic"),
			Fixture,
			Owner,
			OldSurface,
			NewSurface,
			Checkpoint,
			Journal,
			Envelope,
			Bytes))
	{
		return false;
	}
	const auto Decoded = FArcOwnerHandoffRecoveryPayloadCodec::Decode(Bytes);
	if (!Decoded.IsSuccess())
	{
		AddError(Decoded.GetDiagnostic());
		return false;
	}
	const auto& OriginalState = Checkpoint.GetSurfaceCursor();
	const auto& RestoredState =
		Decoded.GetEnvelope().GetCheckpoint().GetSurfaceCursor();
	const auto& OriginalRequest =
		OriginalState.GetSourcePreview().GetPlan().GetRequest();
	const auto& RestoredRequest =
		RestoredState.GetSourcePreview().GetPlan().GetRequest();
	const auto& RestoredAction = RestoredRequest.GetAction();

	TestTrue(TEXT("all derived presentation roots are deterministically rebuilt"),
		RestoredState.Matches(OriginalState)
			&& RestoredState.GetChoiceState().Matches(
				OriginalState.GetChoiceState())
			&& RestoredState.GetSourcePreview().Matches(
				OriginalState.GetSourcePreview())
			&& RestoredState.GetSourcePreview().GetPlan().Matches(
				OriginalState.GetSourcePreview().GetPlan())
			&& RestoredRequest.Matches(OriginalRequest));
	TestTrue(TEXT("action metadata and explicit GameplayTags survive the envelope"),
		RestoredAction.GetRunId() == OriginalRequest.GetAction().GetRunId()
			&& RestoredAction.GetOwnerId()
				== OriginalRequest.GetAction().GetOwnerId()
			&& RestoredAction.GetActivationId()
				== OriginalRequest.GetAction().GetActivationId()
			&& RestoredAction.GetActionDefinitionId()
				== OriginalRequest.GetAction().GetActionDefinitionId()
			&& RestoredAction.GetContent().Version
				== OriginalRequest.GetAction().GetContent().Version
			&& RestoredAction.GetContent().Digest
				== OriginalRequest.GetAction().GetContent().Digest
			&& RestoredAction.GetSourceTags()
				== OriginalRequest.GetAction().GetSourceTags()
			&& RestoredAction.GetSourceTags().HasTagExact(
				FShanmenCombatNativeTags::SourcePlayer()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryPayloadBoundsTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadEnvelope.BoundsAndUnknownSchema",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryPayloadBoundsTest::
RunTest(const FString&)
{
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture Fixture;
	FFakeArcPreviewHandoffSurface OldSurface(
		FGuid(0xF4A40001, 0xF4A40002, 0xF4A40003, 0xF4A40004),
		Consumer);
	FFakeArcPreviewHandoffSurface NewSurface(
		FGuid(0xF4A41001, 0xF4A41002, 0xF4A41003, 0xF4A41004),
		Consumer);
	FArcCompositionOwner Owner;
	FArcOwnerHandoffRecoveryCheckpoint Checkpoint;
	FArcOwnerHandoffRecoveryJournal Journal;
	FArcOwnerHandoffRecoveryPayloadEnvelope Envelope;
	TArray<uint8> Canonical;
	if (!BuildArcOwnerHandoffRecoveryPayloadEvidence(
			*this,
			TEXT("ArcPreviewRecoveryPayloadBounds"),
			Fixture,
			Owner,
			OldSurface,
			NewSurface,
			Checkpoint,
			Journal,
			Envelope,
			Canonical))
	{
		return false;
	}

	TArray<uint8> WrongMagic = Canonical;
	WrongMagic[0] ^= 0x01;
	TArray<uint8> UnknownSchema = Canonical;
	UnknownSchema[11] = 0x02;
	TArray<uint8> WrongSize = Canonical;
	WrongSize[12] = WrongSize[13] = WrongSize[14] = WrongSize[15] = 0;
	TArray<uint8> Oversized;
	Oversized.SetNumZeroed(
		FArcOwnerHandoffRecoveryPayloadCodec::MaximumEncodedBytes() + 1);
	TArray<uint8> ShortHeader;
	ShortHeader.Append(
		Canonical.GetData(),
		FArcOwnerHandoffRecoveryPayloadCodec::HeaderSize() - 1);

	const TArray<uint8> EmptyBytes;
	const auto Empty =
		FArcOwnerHandoffRecoveryPayloadCodec::Decode(EmptyBytes);
	const auto Magic =
		FArcOwnerHandoffRecoveryPayloadCodec::Decode(WrongMagic);
	const auto Schema =
		FArcOwnerHandoffRecoveryPayloadCodec::Decode(UnknownSchema);
	const auto Size =
		FArcOwnerHandoffRecoveryPayloadCodec::Decode(WrongSize);
	const auto Over =
		FArcOwnerHandoffRecoveryPayloadCodec::Decode(Oversized);
	const auto Short =
		FArcOwnerHandoffRecoveryPayloadCodec::Decode(ShortHeader);

	TestTrue(TEXT("empty, wrong-magic and unknown-schema inputs fail closed"),
		!Empty.IsSuccess()
			&& Empty.GetStatus()
				== EArcOwnerHandoffRecoveryPayloadDecodeStatus::InputEmpty
			&& !Magic.IsSuccess()
			&& Magic.GetStatus()
				== EArcOwnerHandoffRecoveryPayloadDecodeStatus::MagicMismatch
			&& !Schema.IsSuccess()
			&& Schema.GetStatus()
				== EArcOwnerHandoffRecoveryPayloadDecodeStatus::
					UnsupportedSchema);
	TestTrue(TEXT("declared, minimum and maximum sizes are enforced before allocation"),
		!Size.IsSuccess()
			&& Size.GetStatus()
				== EArcOwnerHandoffRecoveryPayloadDecodeStatus::SizeMismatch
			&& !Over.IsSuccess()
			&& Over.GetStatus()
				== EArcOwnerHandoffRecoveryPayloadDecodeStatus::SizeMismatch
			&& !Short.IsSuccess()
			&& Short.GetStatus()
				== EArcOwnerHandoffRecoveryPayloadDecodeStatus::SizeMismatch);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryPayloadFenceTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadEnvelope.ForeignJournalAndCommittedFence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryPayloadFenceTest::
RunTest(const FString&)
{
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture FixtureA;
	FFakeArcPreviewHandoffSurface OldA(
		FGuid(0xF4A50001, 0xF4A50002, 0xF4A50003, 0xF4A50004),
		Consumer);
	FFakeArcPreviewHandoffSurface NewA(
		FGuid(0xF4A51001, 0xF4A51002, 0xF4A51003, 0xF4A51004),
		Consumer);
	FArcCompositionOwner OwnerA;
	FArcOwnerHandoffRecoveryCheckpoint CheckpointA;
	FArcOwnerHandoffRecoveryJournal JournalA;
	FArcOwnerHandoffRecoveryPayloadEnvelope EnvelopeA;
	TArray<uint8> BytesA;
	if (!BuildArcOwnerHandoffRecoveryPayloadEvidence(
			*this,
			TEXT("ArcPreviewRecoveryPayloadFenceA"),
			FixtureA,
			OwnerA,
			OldA,
			NewA,
			CheckpointA,
			JournalA,
			EnvelopeA,
			BytesA))
	{
		return false;
	}

	FThrownLifecycleFixture FixtureB;
	FFakeArcPreviewHandoffSurface OldB(
		FGuid(0xF4A52001, 0xF4A52002, 0xF4A52003, 0xF4A52004),
		Consumer);
	FFakeArcPreviewHandoffSurface NewB(
		FGuid(0xF4A53001, 0xF4A53002, 0xF4A53003, 0xF4A53004),
		Consumer);
	FArcCompositionOwner OwnerB;
	FArcOwnerHandoffRecoveryCheckpoint CheckpointB;
	FArcOwnerHandoffRecoveryJournal JournalB;
	FArcOwnerHandoffRecoveryPayloadEnvelope EnvelopeB;
	TArray<uint8> BytesB;
	if (!BuildArcOwnerHandoffRecoveryPayloadEvidence(
			*this,
			TEXT("ArcPreviewRecoveryPayloadFenceB"),
			FixtureB,
			OwnerB,
			OldB,
			NewB,
			CheckpointB,
			JournalB,
			EnvelopeB,
			BytesB))
	{
		return false;
	}

	FArcOwnerHandoffRecoveryCheckpoint Foreign;
	const bool bForeignUnwrapped =
		EnvelopeA.TryUnwrapForJournal(JournalB, Foreign);
	FArcOwnerHandoffRecoveryCheckpoint Exact;
	const bool bExactUnwrapped =
		EnvelopeA.TryUnwrapForJournal(JournalA, Exact);
	FArcOwnerHandoffRecovery Recovery;
	const auto Recovered = bExactUnwrapped
		? Recovery.Execute(OwnerA, Exact, OldA, NewA)
		: FArcOwnerHandoffRecoveryResult();
	const auto Committed =
		JournalA.AppendRecoveryReceipt(Exact, Recovered.GetReceipt());
	FArcOwnerHandoffRecoveryCheckpoint Completed;
	const bool bCompletedUnwrapped =
		EnvelopeA.TryUnwrapForJournal(JournalA, Completed);
	const auto StaleDecoded =
		FArcOwnerHandoffRecoveryPayloadCodec::Decode(BytesA);

	TestTrue(TEXT("foreign pending journal cannot authorize another envelope"),
		EnvelopeA.IsValid() && EnvelopeB.IsValid()
			&& EnvelopeA.GetJournalId() != EnvelopeB.GetJournalId()
			&& !bForeignUnwrapped && !Foreign.IsValid());
	TestTrue(TEXT("exact pending journal permits evidence extraction only"),
		bExactUnwrapped && Exact.IsValid()
			&& Exact.GetCheckpointId() == CheckpointA.GetCheckpointId()
			&& Recovered.IsValid() && Recovered.WasRecovered());
	TestTrue(TEXT("valid stale bytes remain evidence but fail the committed gate"),
		Committed.DidAppend() && StaleDecoded.IsSuccess()
			&& !StaleDecoded.GetEnvelope().MatchesJournal(JournalA)
			&& !bCompletedUnwrapped && !Completed.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryPayloadStorageContextTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadStorage.ContextAndJournalFence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryPayloadStorageContextTest::
RunTest(const FString&)
{
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture Fixture;
	FFakeArcPreviewHandoffSurface OldSurface(
		FGuid(0xF4B00001, 0xF4B00002, 0xF4B00003, 0xF4B00004),
		Consumer);
	FFakeArcPreviewHandoffSurface NewSurface(
		FGuid(0xF4B01001, 0xF4B01002, 0xF4B01003, 0xF4B01004),
		Consumer);
	FArcCompositionOwner Owner;
	FArcOwnerHandoffRecoveryCheckpoint Checkpoint;
	FArcOwnerHandoffRecoveryJournal Journal;
	FArcOwnerHandoffRecoveryPayloadEnvelope Envelope;
	TArray<uint8> Bytes;
	if (!BuildArcOwnerHandoffRecoveryPayloadEvidence(
			*this,
			TEXT("ArcPreviewRecoveryPayloadStorageContext"),
			Fixture,
			Owner,
			OldSurface,
			NewSurface,
			Checkpoint,
			Journal,
			Envelope,
			Bytes))
	{
		return false;
	}

	FArcOwnerHandoffRecoveryPayloadStorageContext Context;
	if (!BuildArcOwnerHandoffRecoveryPayloadStorageContext(
			*this,
			TEXT("Context"),
			Journal.GetJournalId(),
			Context))
	{
		return false;
	}
	FString Diagnostic;
	FArcOwnerHandoffRecoveryPayloadStorageContext Relative;
	const bool bAcceptedRelative =
		FArcOwnerHandoffRecoveryPayloadStorageContext::TryCreate(
			TEXT("Relative/CheckpointPayload"),
			Journal.GetJournalId(),
			Relative,
			Diagnostic);
	FArcOwnerHandoffRecoveryPayloadStorageContext EmptyJournal;
	const bool bAcceptedEmptyJournal =
		FArcOwnerHandoffRecoveryPayloadStorageContext::TryCreate(
			Context.GetRootDirectory(),
			FGuid(),
			EmptyJournal,
			Diagnostic);
	const FGuid ForeignJournalId(
		0xF4B02001, 0xF4B02002, 0xF4B02003, 0xF4B02004);
	FArcOwnerHandoffRecoveryPayloadStorageContext ForeignContext;
	const bool bForeignContext =
		FArcOwnerHandoffRecoveryPayloadStorageContext::TryCreate(
			Context.GetRootDirectory(),
			ForeignJournalId,
			ForeignContext,
			Diagnostic);

	FFakeArcOwnerHandoffRecoveryPayloadStorageFileSystem FileSystem;
	FArcOwnerHandoffRecoveryPayloadStorageAdapter Adapter;
	const auto InvalidContext = Adapter.Save(
		FArcOwnerHandoffRecoveryPayloadStorageContext(),
		Envelope,
		FileSystem);
	const auto InvalidEnvelope = Adapter.Save(
		Context,
		FArcOwnerHandoffRecoveryPayloadEnvelope(),
		FileSystem);
	const auto ForeignSlot = Adapter.Save(
		ForeignContext,
		Envelope,
		FileSystem);

	TestTrue(TEXT("context canonicalizes one absolute caller root and journal slot"),
		Context.IsValid()
			&& Context.GetExpectedJournalId() == Journal.GetJournalId()
			&& FPaths::GetPath(Context.GetPrimaryPath())
				== Context.GetStorageDirectory()
			&& FPaths::GetPath(Context.GetTemporaryPath())
				== Context.GetStorageDirectory()
			&& Context.GetPrimaryPath().Contains(
				Journal.GetJournalId().ToString(EGuidFormats::Digits)));
	TestTrue(TEXT("relative root and invalid journal identity fail closed"),
		!bAcceptedRelative && !Relative.IsValid()
			&& !bAcceptedEmptyJournal && !EmptyJournal.IsValid()
			&& bForeignContext && ForeignContext.IsValid());
	TestTrue(TEXT("invalid requests reject before all filesystem callbacks"),
		!InvalidContext.IsSuccess()
			&& InvalidContext.GetStatus()
				== EArcOwnerHandoffRecoveryPayloadStorageSaveStatus::
					ContextRejected
			&& !InvalidEnvelope.IsSuccess()
			&& InvalidEnvelope.GetStatus()
				== EArcOwnerHandoffRecoveryPayloadStorageSaveStatus::
					EnvelopeRejected
			&& !ForeignSlot.IsSuccess()
			&& ForeignSlot.GetStatus()
				== EArcOwnerHandoffRecoveryPayloadStorageSaveStatus::
					JournalSlotMismatch
			&& FileSystem.DirectoryQueryCount == 0
			&& FileSystem.FileExistsQueryCount == 0
			&& FileSystem.CreateDirectoryCount == 0
			&& FileSystem.DeleteCount == 0
			&& FileSystem.WriteCount == 0
			&& FileSystem.ReadCount == 0
			&& FileSystem.AtomicReplaceCount == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryPayloadStorageRoundTripTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadStorage.AtomicRoundTripAndEvidenceOnly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryPayloadStorageRoundTripTest::
RunTest(const FString&)
{
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture Fixture;
	FFakeArcPreviewHandoffSurface OldSurface(
		FGuid(0xF4B10001, 0xF4B10002, 0xF4B10003, 0xF4B10004),
		Consumer);
	FFakeArcPreviewHandoffSurface NewSurface(
		FGuid(0xF4B11001, 0xF4B11002, 0xF4B11003, 0xF4B11004),
		Consumer);
	FArcCompositionOwner Owner;
	FArcOwnerHandoffRecoveryCheckpoint Checkpoint;
	FArcOwnerHandoffRecoveryJournal Journal;
	FArcOwnerHandoffRecoveryPayloadEnvelope Envelope;
	TArray<uint8> Canonical;
	if (!BuildArcOwnerHandoffRecoveryPayloadEvidence(
			*this,
			TEXT("ArcPreviewRecoveryPayloadStorageRoundTrip"),
			Fixture,
			Owner,
			OldSurface,
			NewSurface,
			Checkpoint,
			Journal,
			Envelope,
			Canonical))
	{
		return false;
	}
	FArcOwnerHandoffRecoveryPayloadStorageContext Context;
	if (!BuildArcOwnerHandoffRecoveryPayloadStorageContext(
			*this,
			TEXT("RoundTrip"),
			Journal.GetJournalId(),
			Context))
	{
		return false;
	}

	const int32 OldRetirementsBefore = OldSurface.RetirementCallCount;
	const int32 OldMutationsBefore = OldSurface.MutationCallCount;
	const int32 NewMutationsBefore = NewSurface.MutationCallCount;
	FFakeArcOwnerHandoffRecoveryPayloadStorageFileSystem FileSystem;
	FArcOwnerHandoffRecoveryPayloadStorageAdapter Adapter;
	const auto Saved = Adapter.Save(Context, Envelope, FileSystem);
	const auto Loaded = Adapter.Load(Context, FileSystem);
	TArray<uint8> PrimaryBytes;
	const bool bHasPrimary =
		FileSystem.TryGetFile(Context.GetPrimaryPath(), PrimaryBytes);
	TArray<uint8> TemporaryBytes;
	const bool bHasTemporary =
		FileSystem.TryGetFile(Context.GetTemporaryPath(), TemporaryBytes);
	FArcOwnerHandoffRecoveryCheckpoint ExplicitCheckpoint;
	const bool bExplicitlyUnwrapped = Loaded.IsSuccess()
		&& Loaded.GetEnvelope().TryUnwrapForJournal(
			Journal, ExplicitCheckpoint);

	TestTrue(TEXT("one-shot save publishes only exact canonical bytes"),
		Saved.IsSuccess()
			&& Saved.GetStatus()
				== EArcOwnerHandoffRecoveryPayloadStorageSaveStatus::Saved
			&& Saved.GetEnvelopeId() == Envelope.GetEnvelopeId()
			&& Saved.GetEncodedByteCount() == Canonical.Num()
			&& Saved.DidReplacePrimary()
			&& !Saved.TemporaryFileMayRemain()
			&& bHasPrimary && PrimaryBytes == Canonical
			&& !bHasTemporary);
	TestTrue(TEXT("load returns decoded envelope evidence without implicit authority"),
		Loaded.IsSuccess()
			&& Loaded.GetStatus()
				== EArcOwnerHandoffRecoveryPayloadStorageLoadStatus::Loaded
			&& Loaded.GetObservedByteCount() == Canonical.Num()
			&& Loaded.GetDecodeStatus()
				== EArcOwnerHandoffRecoveryPayloadDecodeStatus::Decoded
			&& Loaded.GetEnvelope().Matches(Envelope)
			&& bExplicitlyUnwrapped && ExplicitCheckpoint.IsValid());
	TestTrue(TEXT("storage calls neither surface nor recovery coordinator"),
		OldSurface.RetirementCallCount == OldRetirementsBefore
			&& OldSurface.MutationCallCount == OldMutationsBefore
			&& NewSurface.MutationCallCount == NewMutationsBefore
			&& FileSystem.CreateDirectoryCount == 1
			&& FileSystem.WriteCount == 1
			&& FileSystem.ReadCount == 3
			&& FileSystem.AtomicReplaceCount == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryPayloadStoragePrecommitTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadStorage.PrecommitFailureAtomicity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryPayloadStoragePrecommitTest::
RunTest(const FString&)
{
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture Fixture;
	FFakeArcPreviewHandoffSurface OldSurface(
		FGuid(0xF4B20001, 0xF4B20002, 0xF4B20003, 0xF4B20004),
		Consumer);
	FFakeArcPreviewHandoffSurface NewSurface(
		FGuid(0xF4B21001, 0xF4B21002, 0xF4B21003, 0xF4B21004),
		Consumer);
	FArcCompositionOwner Owner;
	FArcOwnerHandoffRecoveryCheckpoint Checkpoint;
	FArcOwnerHandoffRecoveryJournal Journal;
	FArcOwnerHandoffRecoveryPayloadEnvelope Envelope;
	TArray<uint8> Canonical;
	if (!BuildArcOwnerHandoffRecoveryPayloadEvidence(
			*this,
			TEXT("ArcPreviewRecoveryPayloadStoragePrecommit"),
			Fixture,
			Owner,
			OldSurface,
			NewSurface,
			Checkpoint,
			Journal,
			Envelope,
			Canonical))
	{
		return false;
	}
	FArcOwnerHandoffRecoveryPayloadStorageContext Context;
	if (!BuildArcOwnerHandoffRecoveryPayloadStorageContext(
			*this,
			TEXT("Precommit"),
			Journal.GetJournalId(),
			Context))
	{
		return false;
	}

	using EFailure =
		FFakeArcOwnerHandoffRecoveryPayloadStorageFileSystem::EFailure;
	struct FFailureCase
	{
		EFailure Failure;
		EArcOwnerHandoffRecoveryPayloadStorageSaveStatus Expected;
		bool bNeedsStaleTemporary = false;
		bool bDirectoryInitiallyExists = true;
	};
	const TArray<FFailureCase> Cases =
	{
		{EFailure::CreateDirectory,
			EArcOwnerHandoffRecoveryPayloadStorageSaveStatus::
				DirectoryCreationFailed,
			false,
			false},
		{EFailure::DeleteTemporary,
			EArcOwnerHandoffRecoveryPayloadStorageSaveStatus::
				StaleTemporaryCleanupFailed,
			true},
		{EFailure::OpenTemporary,
			EArcOwnerHandoffRecoveryPayloadStorageSaveStatus::
				TemporaryOpenFailed},
		{EFailure::WriteTemporary,
			EArcOwnerHandoffRecoveryPayloadStorageSaveStatus::
				TemporaryWriteFailed},
		{EFailure::FlushTemporary,
			EArcOwnerHandoffRecoveryPayloadStorageSaveStatus::
				TemporaryFlushFailed},
		{EFailure::ReadTemporary,
			EArcOwnerHandoffRecoveryPayloadStorageSaveStatus::
				TemporaryReadBackFailed},
		{EFailure::CorruptTemporaryRead,
			EArcOwnerHandoffRecoveryPayloadStorageSaveStatus::
				TemporaryValidationFailed},
		{EFailure::AtomicReplace,
			EArcOwnerHandoffRecoveryPayloadStorageSaveStatus::
				AtomicReplaceFailed}
	};
	const TArray<uint8> ExistingPrimary = {0x11, 0x22, 0x33, 0x44};
	FArcOwnerHandoffRecoveryPayloadStorageAdapter Adapter;
	for (int32 Index = 0; Index < Cases.Num(); ++Index)
	{
		const FFailureCase& FailureCase = Cases[Index];
		FFakeArcOwnerHandoffRecoveryPayloadStorageFileSystem FileSystem;
		FileSystem.SetDirectoryExists(
			FailureCase.bDirectoryInitiallyExists);
		FileSystem.SetFile(Context.GetPrimaryPath(), ExistingPrimary);
		if (FailureCase.bNeedsStaleTemporary)
		{
			FileSystem.SetFile(
				Context.GetTemporaryPath(), TArray<uint8>({0x99}));
		}
		FileSystem.SetFailure(FailureCase.Failure);
		const auto Result = Adapter.Save(Context, Envelope, FileSystem);
		TArray<uint8> PrimaryAfter;
		const bool bHasPrimary = FileSystem.TryGetFile(
			Context.GetPrimaryPath(), PrimaryAfter);

		TestTrue(
			*FString::Printf(TEXT("precommit case %d reports exact stage"), Index),
			!Result.IsSuccess()
				&& Result.GetStatus() == FailureCase.Expected);
		TestTrue(
			*FString::Printf(TEXT("precommit case %d preserves primary"), Index),
			bHasPrimary && PrimaryAfter == ExistingPrimary
				&& !Result.DidReplacePrimary());
		TestTrue(
			*FString::Printf(TEXT("precommit case %d is one-shot"), Index),
			FileSystem.WriteCount <= 1
				&& FileSystem.AtomicReplaceCount <= 1
				&& (FailureCase.Failure != EFailure::AtomicReplace
					|| FileSystem.AtomicReplaceCount == 1));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryPayloadStoragePostcommitTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadStorage.PostcommitOutcomeRequiresLoad",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryPayloadStoragePostcommitTest::
RunTest(const FString&)
{
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture Fixture;
	FFakeArcPreviewHandoffSurface OldSurface(
		FGuid(0xF4B30001, 0xF4B30002, 0xF4B30003, 0xF4B30004),
		Consumer);
	FFakeArcPreviewHandoffSurface NewSurface(
		FGuid(0xF4B31001, 0xF4B31002, 0xF4B31003, 0xF4B31004),
		Consumer);
	FArcCompositionOwner Owner;
	FArcOwnerHandoffRecoveryCheckpoint Checkpoint;
	FArcOwnerHandoffRecoveryJournal Journal;
	FArcOwnerHandoffRecoveryPayloadEnvelope Envelope;
	TArray<uint8> Canonical;
	if (!BuildArcOwnerHandoffRecoveryPayloadEvidence(
			*this,
			TEXT("ArcPreviewRecoveryPayloadStoragePostcommit"),
			Fixture,
			Owner,
			OldSurface,
			NewSurface,
			Checkpoint,
			Journal,
			Envelope,
			Canonical))
	{
		return false;
	}
	FArcOwnerHandoffRecoveryPayloadStorageContext Context;
	if (!BuildArcOwnerHandoffRecoveryPayloadStorageContext(
			*this,
			TEXT("Postcommit"),
			Journal.GetJournalId(),
			Context))
	{
		return false;
	}

	using EFailure =
		FFakeArcOwnerHandoffRecoveryPayloadStorageFileSystem::EFailure;
	struct FFailureCase
	{
		EFailure Failure;
		EArcOwnerHandoffRecoveryPayloadStorageSaveStatus Expected;
	};
	const TArray<FFailureCase> Cases =
	{
		{EFailure::ReadPrimary,
			EArcOwnerHandoffRecoveryPayloadStorageSaveStatus::
				CommittedReadBackFailed},
		{EFailure::CorruptPrimaryRead,
			EArcOwnerHandoffRecoveryPayloadStorageSaveStatus::
				CommittedValidationFailed}
	};
	FArcOwnerHandoffRecoveryPayloadStorageAdapter Adapter;
	for (int32 Index = 0; Index < Cases.Num(); ++Index)
	{
		FFakeArcOwnerHandoffRecoveryPayloadStorageFileSystem FileSystem;
		FileSystem.SetDirectoryExists(true);
		FileSystem.SetFailure(Cases[Index].Failure);
		const auto Result = Adapter.Save(Context, Envelope, FileSystem);
		FileSystem.SetFailure(EFailure::None);
		const auto ResolvedByLoad = Adapter.Load(Context, FileSystem);
		TArray<uint8> PrimaryBytes;
		const bool bHasPrimary = FileSystem.TryGetFile(
			Context.GetPrimaryPath(), PrimaryBytes);

		TestTrue(
			*FString::Printf(TEXT("postcommit case %d exposes indeterminate stage"), Index),
			!Result.IsSuccess()
				&& Result.GetStatus() == Cases[Index].Expected
				&& Result.DidReplacePrimary()
				&& !Result.TemporaryFileMayRemain());
		TestTrue(
			*FString::Printf(TEXT("postcommit case %d is resolved only by explicit load"), Index),
			ResolvedByLoad.IsSuccess()
				&& ResolvedByLoad.GetEnvelope().Matches(Envelope)
				&& bHasPrimary && PrimaryBytes == Canonical);
		TestTrue(
			*FString::Printf(TEXT("postcommit case %d never retries replacement"), Index),
			FileSystem.WriteCount == 1
				&& FileSystem.AtomicReplaceCount == 1);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryPayloadStorageLoadFenceTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadStorage.LoadBoundsDecodeAndJournalFence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryPayloadStorageLoadFenceTest::
RunTest(const FString&)
{
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture Fixture;
	FFakeArcPreviewHandoffSurface OldSurface(
		FGuid(0xF4B40001, 0xF4B40002, 0xF4B40003, 0xF4B40004),
		Consumer);
	FFakeArcPreviewHandoffSurface NewSurface(
		FGuid(0xF4B41001, 0xF4B41002, 0xF4B41003, 0xF4B41004),
		Consumer);
	FArcCompositionOwner Owner;
	FArcOwnerHandoffRecoveryCheckpoint Checkpoint;
	FArcOwnerHandoffRecoveryJournal Journal;
	FArcOwnerHandoffRecoveryPayloadEnvelope Envelope;
	TArray<uint8> Canonical;
	if (!BuildArcOwnerHandoffRecoveryPayloadEvidence(
			*this,
			TEXT("ArcPreviewRecoveryPayloadStorageLoadFence"),
			Fixture,
			Owner,
			OldSurface,
			NewSurface,
			Checkpoint,
			Journal,
			Envelope,
			Canonical))
	{
		return false;
	}
	FArcOwnerHandoffRecoveryPayloadStorageContext Context;
	if (!BuildArcOwnerHandoffRecoveryPayloadStorageContext(
			*this,
			TEXT("LoadFence"),
			Journal.GetJournalId(),
			Context))
	{
		return false;
	}

	FFakeArcOwnerHandoffRecoveryPayloadStorageFileSystem FileSystem;
	FileSystem.SetDirectoryExists(true);
	FArcOwnerHandoffRecoveryPayloadStorageAdapter Adapter;
	const auto Missing = Adapter.Load(Context, FileSystem);
	TArray<uint8> Oversized;
	Oversized.SetNumZeroed(
		FArcOwnerHandoffRecoveryPayloadCodec::MaximumEncodedBytes() + 1);
	FileSystem.SetFile(Context.GetPrimaryPath(), Oversized);
	const auto TooLarge = Adapter.Load(Context, FileSystem);
	FileSystem.SetFile(
		Context.GetPrimaryPath(), TArray<uint8>({0x01, 0x02, 0x03}));
	const auto Corrupt = Adapter.Load(Context, FileSystem);
	FileSystem.SetFile(Context.GetPrimaryPath(), Canonical);
	FileSystem.SetFailure(
		FFakeArcOwnerHandoffRecoveryPayloadStorageFileSystem::EFailure::
			ReadPrimary);
	const auto ReadFailure = Adapter.Load(Context, FileSystem);
	FileSystem.SetFailure(
		FFakeArcOwnerHandoffRecoveryPayloadStorageFileSystem::EFailure::None);

	const FGuid ForeignJournalId(
		0xF4B42001, 0xF4B42002, 0xF4B42003, 0xF4B42004);
	FArcOwnerHandoffRecoveryPayloadStorageContext ForeignContext;
	FString Diagnostic;
	if (!FArcOwnerHandoffRecoveryPayloadStorageContext::TryCreate(
			Context.GetRootDirectory(),
			ForeignJournalId,
			ForeignContext,
			Diagnostic))
	{
		AddError(Diagnostic);
		return false;
	}
	FileSystem.SetFile(ForeignContext.GetPrimaryPath(), Canonical);
	const auto Foreign = Adapter.Load(ForeignContext, FileSystem);

	TestTrue(TEXT("missing, oversized and unreadable primaries classify exactly"),
		!Missing.IsSuccess()
			&& Missing.GetStatus()
				== EArcOwnerHandoffRecoveryPayloadStorageLoadStatus::Missing
			&& !TooLarge.IsSuccess()
			&& TooLarge.GetStatus()
				== EArcOwnerHandoffRecoveryPayloadStorageLoadStatus::SizeRejected
			&& TooLarge.GetObservedByteCount()
				== FArcOwnerHandoffRecoveryPayloadCodec::MaximumEncodedBytes() + 1
			&& !ReadFailure.IsSuccess()
			&& ReadFailure.GetStatus()
				== EArcOwnerHandoffRecoveryPayloadStorageLoadStatus::ReadFailed);
	TestTrue(TEXT("codec corruption remains distinct from filesystem failure"),
		!Corrupt.IsSuccess()
			&& Corrupt.GetStatus()
				== EArcOwnerHandoffRecoveryPayloadStorageLoadStatus::DecodeRejected
			&& Corrupt.GetDecodeStatus()
				!= EArcOwnerHandoffRecoveryPayloadDecodeStatus::Decoded);
	TestTrue(TEXT("valid foreign bytes cannot cross the expected journal slot"),
		!Foreign.IsSuccess()
			&& Foreign.GetStatus()
				== EArcOwnerHandoffRecoveryPayloadStorageLoadStatus::
					JournalSlotMismatch
			&& Foreign.GetDecodeStatus()
				== EArcOwnerHandoffRecoveryPayloadDecodeStatus::Decoded
			&& FileSystem.WriteCount == 0
			&& FileSystem.AtomicReplaceCount == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryPayloadStorageLocalFileTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadStorage.LocalFileAtomicReplaceAndCommittedFence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryPayloadStorageLocalFileTest::
RunTest(const FString&)
{
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture Fixture;
	FFakeArcPreviewHandoffSurface OldSurface(
		FGuid(0xF4B50001, 0xF4B50002, 0xF4B50003, 0xF4B50004),
		Consumer);
	FFakeArcPreviewHandoffSurface NewSurface(
		FGuid(0xF4B51001, 0xF4B51002, 0xF4B51003, 0xF4B51004),
		Consumer);
	FArcCompositionOwner Owner;
	FArcOwnerHandoffRecoveryCheckpoint Checkpoint;
	FArcOwnerHandoffRecoveryJournal Journal;
	FArcOwnerHandoffRecoveryPayloadEnvelope Envelope;
	TArray<uint8> Canonical;
	if (!BuildArcOwnerHandoffRecoveryPayloadEvidence(
			*this,
			TEXT("ArcPreviewRecoveryPayloadStorageLocal"),
			Fixture,
			Owner,
			OldSurface,
			NewSurface,
			Checkpoint,
			Journal,
			Envelope,
			Canonical))
	{
		return false;
	}
	FArcOwnerHandoffRecoveryPayloadStorageContext Context;
	if (!BuildArcOwnerHandoffRecoveryPayloadStorageContext(
			*this,
			TEXT("LocalFile"),
			Journal.GetJournalId(),
			Context))
	{
		return false;
	}
	IFileManager::Get().DeleteDirectory(
		*Context.GetRootDirectory(), false, true);

	const int32 OldRetirementsBefore = OldSurface.RetirementCallCount;
	const int32 OldMutationsBefore = OldSurface.MutationCallCount;
	const int32 NewMutationsBefore = NewSurface.MutationCallCount;
	FArcOwnerHandoffRecoveryPayloadLocalFileSystem FileSystem;
	FArcOwnerHandoffRecoveryPayloadStorageAdapter Adapter;
	const auto FirstSave = Adapter.Save(Context, Envelope, FileSystem);
	const auto FirstLoad = Adapter.Load(Context, FileSystem);
	const auto ReplacementSave = Adapter.Save(Context, Envelope, FileSystem);
	TArray<uint8> PrimaryBytes;
	int64 PrimarySize = INDEX_NONE;
	const auto PrimaryRead = FileSystem.ReadBounded(
		Context.GetPrimaryPath(),
		FArcOwnerHandoffRecoveryPayloadCodec::MaximumEncodedBytes(),
		PrimaryBytes,
		PrimarySize);
	const bool bNoStorageSurfaceCalls =
		OldSurface.RetirementCallCount == OldRetirementsBefore
		&& OldSurface.MutationCallCount == OldMutationsBefore
		&& NewSurface.MutationCallCount == NewMutationsBefore;
	const bool bTemporaryMissingAfterReplace =
		!IFileManager::Get().FileExists(*Context.GetTemporaryPath());

	FArcOwnerHandoffRecoveryCheckpoint Restored;
	const bool bExplicitlyUnwrapped = FirstLoad.IsSuccess()
		&& FirstLoad.GetEnvelope().TryUnwrapForJournal(Journal, Restored);
	FArcOwnerHandoffRecovery Recovery;
	const auto Recovered = bExplicitlyUnwrapped
		? Recovery.Execute(Owner, Restored, OldSurface, NewSurface)
		: FArcOwnerHandoffRecoveryResult();
	const auto Committed = Recovered.HasReceipt()
		? Journal.AppendRecoveryReceipt(Restored, Recovered.GetReceipt())
		: Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalAppendResult();
	const auto HistoricalLoad = Adapter.Load(Context, FileSystem);
	FArcOwnerHandoffRecoveryCheckpoint Stale;
	const bool bStaleUnwrapped = HistoricalLoad.IsSuccess()
		&& HistoricalLoad.GetEnvelope().TryUnwrapForJournal(Journal, Stale);
	const bool bCleanup = IFileManager::Get().DeleteDirectory(
		*Context.GetRootDirectory(), false, true);

	TestTrue(TEXT("real local backend flushes, replaces and verifies exact bytes"),
		FirstSave.IsSuccess() && FirstLoad.IsSuccess()
			&& ReplacementSave.IsSuccess()
			&& PrimaryRead
				== EArcOwnerHandoffRecoveryPayloadStorageFileReadStatus::Read
			&& PrimarySize == Canonical.Num()
			&& PrimaryBytes == Canonical
			&& bTemporaryMissingAfterReplace);
	TestTrue(TEXT("local storage remains evidence-only across journal commit"),
		bNoStorageSurfaceCalls && bExplicitlyUnwrapped
			&& Recovered.IsValid() && Recovered.WasRecovered()
			&& Committed.DidAppend() && HistoricalLoad.IsSuccess()
			&& !bStaleUnwrapped && !Stale.IsValid());
	TestTrue(TEXT("test-owned storage root is cleaned after verification"),
		bCleanup
			&& !IFileManager::Get().DirectoryExists(
				*Context.GetRootDirectory()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryBundleEvidenceTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundle.EvidenceContractAndGeneration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryBundleEvidenceTest::
RunTest(const FString&)
{
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture Fixture;
	FFakeArcPreviewHandoffSurface OldSurface(
		FGuid(0xF4C00001, 0xF4C00002, 0xF4C00003, 0xF4C00004),
		Consumer);
	FFakeArcPreviewHandoffSurface NewSurface(
		FGuid(0xF4C01001, 0xF4C01002, 0xF4C01003, 0xF4C01004),
		Consumer);
	FArcCompositionOwner Owner;
	FArcOwnerHandoffRecoveryCheckpoint Checkpoint;
	FArcOwnerHandoffRecoveryJournal Journal;
	FArcOwnerHandoffRecoveryPayloadEnvelope Envelope;
	FArcOwnerHandoffRecoveryBundle Bundle;
	TArray<uint8> Canonical;
	if (!BuildArcOwnerHandoffRecoveryBundleEvidence(
			*this,
			TEXT("ArcPreviewRecoveryBundleEvidence"),
			Fixture,
			Owner,
			OldSurface,
			NewSurface,
			Checkpoint,
			Journal,
			Envelope,
			Bundle,
			Canonical))
	{
		return false;
	}
	const int32 OldRetirementsBefore = OldSurface.RetirementCallCount;
	const int32 OldMutationsBefore = OldSurface.MutationCallCount;
	const int32 NewMutationsBefore = NewSurface.MutationCallCount;

	FArcOwnerHandoffRecoveryBundle Replay;
	TArray<uint8> ReplayBytes;
	const bool bReplayCreated = FArcOwnerHandoffRecoveryBundle::TryCreate(
		Journal, Envelope, Replay);
	const bool bReplayEncoded = bReplayCreated
		&& FArcOwnerHandoffRecoveryBundleCodec::TryEncode(
			Replay, ReplayBytes);
	FArcOwnerHandoffRecoveryJournal EmptyJournal;
	FArcOwnerHandoffRecoveryBundle Rejected;
	const bool bAcceptedEmpty = FArcOwnerHandoffRecoveryBundle::TryCreate(
		EmptyJournal, Envelope, Rejected);

	TestTrue(TEXT("pending journal and payload form one deterministic generation"),
		Bundle.IsValid()
			&& Bundle.GetSchemaVersion()
				== FArcOwnerHandoffRecoveryBundleCodec::CurrentSchemaVersion()
			&& Bundle.GetGeneration() == 1
			&& Bundle.GetJournal().GetJournalId() == Journal.GetJournalId()
			&& Bundle.GetEnvelope().Matches(Envelope)
			&& Bundle.GetBundleId().IsValid()
			&& Bundle.GetBundleDigest().IsValid());
	TestTrue(TEXT("same evidence emits one byte-identical canonical bundle"),
		bReplayCreated && bReplayEncoded && Replay.Matches(Bundle)
			&& ReplayBytes == Canonical
			&& Canonical.Num()
				> FArcOwnerHandoffRecoveryBundleCodec::HeaderSize()
			&& Canonical.Num()
				<= FArcOwnerHandoffRecoveryBundleCodec::MaximumEncodedBytes());
	TestTrue(TEXT("bundle creation rejects non-pending evidence without side effects"),
		!bAcceptedEmpty && !Rejected.IsValid()
			&& OldSurface.RetirementCallCount == OldRetirementsBefore
			&& OldSurface.MutationCallCount == OldMutationsBefore
			&& NewSurface.MutationCallCount == NewMutationsBefore);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryBundleRoundTripTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundle.CanonicalRoundTripAndCurrentJournalFence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryBundleRoundTripTest::
RunTest(const FString&)
{
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture Fixture;
	FFakeArcPreviewHandoffSurface OldSurface(
		FGuid(0xF4C10001, 0xF4C10002, 0xF4C10003, 0xF4C10004),
		Consumer);
	FFakeArcPreviewHandoffSurface NewSurface(
		FGuid(0xF4C11001, 0xF4C11002, 0xF4C11003, 0xF4C11004),
		Consumer);
	FArcCompositionOwner Owner;
	FArcOwnerHandoffRecoveryCheckpoint Checkpoint;
	FArcOwnerHandoffRecoveryJournal Journal;
	FArcOwnerHandoffRecoveryPayloadEnvelope Envelope;
	FArcOwnerHandoffRecoveryBundle Bundle;
	TArray<uint8> Canonical;
	if (!BuildArcOwnerHandoffRecoveryBundleEvidence(
			*this,
			TEXT("ArcPreviewRecoveryBundleRoundTrip"),
			Fixture,
			Owner,
			OldSurface,
			NewSurface,
			Checkpoint,
			Journal,
			Envelope,
			Bundle,
			Canonical))
	{
		return false;
	}
	const int32 OldRetirementsBefore = OldSurface.RetirementCallCount;
	const int32 OldMutationsBefore = OldSurface.MutationCallCount;
	const int32 NewMutationsBefore = NewSurface.MutationCallCount;

	const auto Decoded = FArcOwnerHandoffRecoveryBundleCodec::Decode(Canonical);
	TArray<uint8> Reencoded;
	const bool bReencoded = Decoded.IsSuccess()
		&& FArcOwnerHandoffRecoveryBundleCodec::TryEncode(
			Decoded.GetBundle(), Reencoded);
	FArcOwnerHandoffRecoveryCheckpoint Restored;
	const bool bCopied = Decoded.IsSuccess()
		&& Decoded.GetBundle().TryCopyPendingCheckpointEvidenceForJournal(
			Journal, 1, Restored);
	FArcOwnerHandoffRecovery Recovery;
	const auto Recovered = bCopied
		? Recovery.Execute(Owner, Restored, OldSurface, NewSurface)
		: FArcOwnerHandoffRecoveryResult();
	const auto Committed = Recovered.HasReceipt()
		? Journal.AppendRecoveryReceipt(Restored, Recovered.GetReceipt())
		: Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalAppendResult();
	FArcOwnerHandoffRecoveryCheckpoint Stale;
	const bool bStaleCopied = Decoded.IsSuccess()
		&& Decoded.GetBundle().TryCopyPendingCheckpointEvidenceForJournal(
			Journal, 1, Stale);

	TestTrue(TEXT("bundle round-trip preserves both canonical evidence sections"),
		Decoded.IsSuccess()
			&& Decoded.GetStatus()
				== EArcOwnerHandoffRecoveryBundleDecodeStatus::Decoded
			&& Decoded.GetJournalDecodeStatus()
				== EArcOwnerHandoffRecoveryJournalDecodeStatus::DecodedCurrent
			&& Decoded.GetPayloadDecodeStatus()
				== EArcOwnerHandoffRecoveryPayloadDecodeStatus::Decoded
			&& Decoded.GetBundle().Matches(Bundle)
			&& bReencoded && Reencoded == Canonical
			&& bCopied && Restored.GetCheckpointId()
				== Checkpoint.GetCheckpointId());
	TestTrue(TEXT("bundle evidence still requires and passes the P20.48 live gate"),
		Recovered.IsValid() && Recovered.WasRecovered()
			&& Recovered.HasReceipt() && !Recovered.DidMutateSurface()
			&& OldSurface.RetirementCallCount == OldRetirementsBefore
			&& OldSurface.MutationCallCount == OldMutationsBefore
			&& NewSurface.MutationCallCount == NewMutationsBefore);
	TestTrue(TEXT("a fresh committed journal fences historical bundle evidence"),
		Committed.DidAppend()
			&& Journal.GetLatestDisposition()
				== EArcOwnerHandoffRecoveryJournalDisposition::RecoveryCommitted
			&& Decoded.GetBundle().IsValid()
			&& !bStaleCopied && !Stale.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryBundleCorruptionTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundle.CorruptionTruncationAndTrailing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryBundleCorruptionTest::
RunTest(const FString&)
{
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture Fixture;
	FFakeArcPreviewHandoffSurface OldSurface(
		FGuid(0xF4C20001, 0xF4C20002, 0xF4C20003, 0xF4C20004),
		Consumer);
	FFakeArcPreviewHandoffSurface NewSurface(
		FGuid(0xF4C21001, 0xF4C21002, 0xF4C21003, 0xF4C21004),
		Consumer);
	FArcCompositionOwner Owner;
	FArcOwnerHandoffRecoveryCheckpoint Checkpoint;
	FArcOwnerHandoffRecoveryJournal Journal;
	FArcOwnerHandoffRecoveryPayloadEnvelope Envelope;
	FArcOwnerHandoffRecoveryBundle Bundle;
	TArray<uint8> Canonical;
	if (!BuildArcOwnerHandoffRecoveryBundleEvidence(
			*this,
			TEXT("ArcPreviewRecoveryBundleCorruption"),
			Fixture,
			Owner,
			OldSurface,
			NewSurface,
			Checkpoint,
			Journal,
			Envelope,
			Bundle,
			Canonical))
	{
		return false;
	}

	int32 AcceptedCorruptions = 0;
	for (int32 Index = 0; Index < Canonical.Num(); ++Index)
	{
		TArray<uint8> Corrupted = Canonical;
		Corrupted[Index] ^= 0x01;
		if (FArcOwnerHandoffRecoveryBundleCodec::Decode(Corrupted).IsSuccess())
		{
			++AcceptedCorruptions;
		}
	}
	int32 AcceptedTruncations = 0;
	for (int32 Length = 0; Length < Canonical.Num(); ++Length)
	{
		TArray<uint8> Truncated;
		Truncated.Append(Canonical.GetData(), Length);
		if (FArcOwnerHandoffRecoveryBundleCodec::Decode(Truncated).IsSuccess())
		{
			++AcceptedTruncations;
		}
	}
	TArray<uint8> Trailing = Canonical;
	Trailing.Add(0x00);
	const auto TrailingResult =
		FArcOwnerHandoffRecoveryBundleCodec::Decode(Trailing);

	TestEqual(TEXT("every one-bit recovery-bundle corruption is rejected"),
		AcceptedCorruptions, 0);
	TestEqual(TEXT("every strict recovery-bundle prefix is rejected"),
		AcceptedTruncations, 0);
	TestTrue(TEXT("recovery-bundle trailing bytes are rejected"),
		!TrailingResult.IsSuccess()
			&& TrailingResult.GetStatus()
				== EArcOwnerHandoffRecoveryBundleDecodeStatus::SizeMismatch);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryBundleBoundsTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundle.HeaderAndSectionBounds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryBundleBoundsTest::
RunTest(const FString&)
{
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture Fixture;
	FFakeArcPreviewHandoffSurface OldSurface(
		FGuid(0xF4C30001, 0xF4C30002, 0xF4C30003, 0xF4C30004),
		Consumer);
	FFakeArcPreviewHandoffSurface NewSurface(
		FGuid(0xF4C31001, 0xF4C31002, 0xF4C31003, 0xF4C31004),
		Consumer);
	FArcCompositionOwner Owner;
	FArcOwnerHandoffRecoveryCheckpoint Checkpoint;
	FArcOwnerHandoffRecoveryJournal Journal;
	FArcOwnerHandoffRecoveryPayloadEnvelope Envelope;
	FArcOwnerHandoffRecoveryBundle Bundle;
	TArray<uint8> Canonical;
	if (!BuildArcOwnerHandoffRecoveryBundleEvidence(
			*this,
			TEXT("ArcPreviewRecoveryBundleBounds"),
			Fixture,
			Owner,
			OldSurface,
			NewSurface,
			Checkpoint,
			Journal,
			Envelope,
			Bundle,
			Canonical))
	{
		return false;
	}

	const auto Empty = FArcOwnerHandoffRecoveryBundleCodec::Decode({});
	TArray<uint8> ShortHeader;
	ShortHeader.SetNumZeroed(
		FArcOwnerHandoffRecoveryBundleCodec::HeaderSize() - 1);
	const auto Short =
		FArcOwnerHandoffRecoveryBundleCodec::Decode(ShortHeader);
	TArray<uint8> BadMagic = Canonical;
	BadMagic[0] ^= 0x01;
	const auto Magic =
		FArcOwnerHandoffRecoveryBundleCodec::Decode(BadMagic);
	TArray<uint8> BadSchema = Canonical;
	const bool bWroteSchema = OverwriteUint32BigEndian(BadSchema, 8, 2);
	const auto Schema =
		FArcOwnerHandoffRecoveryBundleCodec::Decode(BadSchema);
	TArray<uint8> BadGeneration = Canonical;
	const bool bWroteGeneration =
		OverwriteUint32BigEndian(BadGeneration, 16, 0);
	const auto Generation =
		FArcOwnerHandoffRecoveryBundleCodec::Decode(BadGeneration);
	TArray<uint8> BadJournalSize = Canonical;
	const bool bWroteJournalSize = OverwriteUint32BigEndian(
		BadJournalSize,
		20,
		static_cast<uint32>(
			FArcOwnerHandoffRecoveryBundleCodec::MaximumJournalEncodedBytes()
			+ 1));
	const auto JournalSize =
		FArcOwnerHandoffRecoveryBundleCodec::Decode(BadJournalSize);
	TArray<uint8> BadPayloadSize = Canonical;
	const bool bWrotePayloadSize = OverwriteUint32BigEndian(
		BadPayloadSize,
		24,
		static_cast<uint32>(
			FArcOwnerHandoffRecoveryBundleCodec::MaximumPayloadEncodedBytes()
			+ 1));
	const auto PayloadSize =
		FArcOwnerHandoffRecoveryBundleCodec::Decode(BadPayloadSize);
	TArray<uint8> BadDeclaredSize = Canonical;
	const bool bWroteDeclaredSize = OverwriteUint32BigEndian(
		BadDeclaredSize, 12, static_cast<uint32>(Canonical.Num() + 1));
	const auto DeclaredSize =
		FArcOwnerHandoffRecoveryBundleCodec::Decode(BadDeclaredSize);
	TArray<uint8> Oversized;
	Oversized.SetNumZeroed(
		FArcOwnerHandoffRecoveryBundleCodec::MaximumEncodedBytes() + 1);
	const auto TooLarge =
		FArcOwnerHandoffRecoveryBundleCodec::Decode(Oversized);

	TestTrue(TEXT("empty and truncated recovery bundles classify as size failures"),
		!Empty.IsSuccess()
			&& Empty.GetStatus()
				== EArcOwnerHandoffRecoveryBundleDecodeStatus::InputEmpty
			&& !Short.IsSuccess()
			&& Short.GetStatus()
				== EArcOwnerHandoffRecoveryBundleDecodeStatus::SizeMismatch
			&& !TooLarge.IsSuccess()
			&& TooLarge.GetStatus()
				== EArcOwnerHandoffRecoveryBundleDecodeStatus::SizeMismatch);
	TestTrue(TEXT("magic, schema and generation headers fail closed"),
		bWroteSchema && bWroteGeneration
			&& !Magic.IsSuccess()
			&& Magic.GetStatus()
				== EArcOwnerHandoffRecoveryBundleDecodeStatus::MagicMismatch
			&& !Schema.IsSuccess()
			&& Schema.GetStatus()
				== EArcOwnerHandoffRecoveryBundleDecodeStatus::UnsupportedSchema
			&& !Generation.IsSuccess()
			&& Generation.GetStatus()
				== EArcOwnerHandoffRecoveryBundleDecodeStatus::GenerationOutOfRange);
	TestTrue(TEXT("section and declared sizes are independently bounded"),
		bWroteJournalSize && bWrotePayloadSize && bWroteDeclaredSize
			&& !JournalSize.IsSuccess()
			&& JournalSize.GetStatus()
				== EArcOwnerHandoffRecoveryBundleDecodeStatus::SectionSizeOutOfRange
			&& !PayloadSize.IsSuccess()
			&& PayloadSize.GetStatus()
				== EArcOwnerHandoffRecoveryBundleDecodeStatus::SectionSizeOutOfRange
			&& !DeclaredSize.IsSuccess()
			&& DeclaredSize.GetStatus()
				== EArcOwnerHandoffRecoveryBundleDecodeStatus::SizeMismatch);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryBundlePairingTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundle.ForeignPairAndCommittedFence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryBundlePairingTest::
RunTest(const FString&)
{
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture FixtureA;
	FFakeArcPreviewHandoffSurface OldSurfaceA(
		FGuid(0xF4C40001, 0xF4C40002, 0xF4C40003, 0xF4C40004),
		Consumer);
	FFakeArcPreviewHandoffSurface NewSurfaceA(
		FGuid(0xF4C41001, 0xF4C41002, 0xF4C41003, 0xF4C41004),
		Consumer);
	FArcCompositionOwner OwnerA;
	FArcOwnerHandoffRecoveryCheckpoint CheckpointA;
	FArcOwnerHandoffRecoveryJournal JournalA;
	FArcOwnerHandoffRecoveryPayloadEnvelope EnvelopeA;
	FArcOwnerHandoffRecoveryBundle BundleA;
	TArray<uint8> BytesA;
	if (!BuildArcOwnerHandoffRecoveryBundleEvidence(
			*this,
			TEXT("ArcPreviewRecoveryBundlePairA"),
			FixtureA,
			OwnerA,
			OldSurfaceA,
			NewSurfaceA,
			CheckpointA,
			JournalA,
			EnvelopeA,
			BundleA,
			BytesA))
	{
		return false;
	}

	FThrownLifecycleFixture FixtureB;
	FFakeArcPreviewHandoffSurface OldSurfaceB(
		FGuid(0xF4C42001, 0xF4C42002, 0xF4C42003, 0xF4C42004),
		Consumer);
	FFakeArcPreviewHandoffSurface NewSurfaceB(
		FGuid(0xF4C43001, 0xF4C43002, 0xF4C43003, 0xF4C43004),
		Consumer);
	FArcCompositionOwner OwnerB;
	FArcOwnerHandoffRecoveryCheckpoint CheckpointB;
	FArcOwnerHandoffRecoveryJournal JournalB;
	FArcOwnerHandoffRecoveryPayloadEnvelope EnvelopeB;
	FArcOwnerHandoffRecoveryBundle BundleB;
	TArray<uint8> BytesB;
	if (!BuildArcOwnerHandoffRecoveryBundleEvidence(
			*this,
			TEXT("ArcPreviewRecoveryBundlePairB"),
			FixtureB,
			OwnerB,
			OldSurfaceB,
			NewSurfaceB,
			CheckpointB,
			JournalB,
			EnvelopeB,
			BundleB,
			BytesB))
	{
		return false;
	}
	const int32 OldARetirementsBefore = OldSurfaceA.RetirementCallCount;
	const int32 OldAMutationsBefore = OldSurfaceA.MutationCallCount;
	const int32 NewAMutationsBefore = NewSurfaceA.MutationCallCount;

	FArcOwnerHandoffRecoveryBundle ForeignAB;
	FArcOwnerHandoffRecoveryBundle ForeignBA;
	const bool bAcceptedAB = FArcOwnerHandoffRecoveryBundle::TryCreate(
		JournalA, EnvelopeB, ForeignAB);
	const bool bAcceptedBA = FArcOwnerHandoffRecoveryBundle::TryCreate(
		JournalB, EnvelopeA, ForeignBA);
	const auto DecodedA = FArcOwnerHandoffRecoveryBundleCodec::Decode(BytesA);
	FArcOwnerHandoffRecoveryCheckpoint ForeignCheckpoint;
	const bool bForeignCopied = DecodedA.IsSuccess()
		&& DecodedA.GetBundle().TryCopyPendingCheckpointEvidenceForJournal(
			JournalB, 1, ForeignCheckpoint);

	FArcOwnerHandoffRecoveryCheckpoint RestoredA;
	const bool bRestoredA = DecodedA.IsSuccess()
		&& DecodedA.GetBundle().TryCopyPendingCheckpointEvidenceForJournal(
			JournalA, 1, RestoredA);
	FArcOwnerHandoffRecovery Recovery;
	const auto RecoveredA = bRestoredA
		? Recovery.Execute(OwnerA, RestoredA, OldSurfaceA, NewSurfaceA)
		: FArcOwnerHandoffRecoveryResult();
	const auto CommittedA = RecoveredA.HasReceipt()
		? JournalA.AppendRecoveryReceipt(RestoredA, RecoveredA.GetReceipt())
		: Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalAppendResult();
	FArcOwnerHandoffRecoveryCheckpoint StaleCheckpoint;
	const bool bStaleCopied = DecodedA.IsSuccess()
		&& DecodedA.GetBundle().TryCopyPendingCheckpointEvidenceForJournal(
			JournalA, 1, StaleCheckpoint);

	TestTrue(TEXT("foreign journal and payload sections cannot form a bundle"),
		!bAcceptedAB && !bAcceptedBA
			&& !ForeignAB.IsValid() && !ForeignBA.IsValid()
			&& BundleA.GetBundleId() != BundleB.GetBundleId()
			&& BundleA.GetBundleDigest() != BundleB.GetBundleDigest());
	TestTrue(TEXT("current-journal fence rejects foreign and committed evidence"),
		!bForeignCopied && !ForeignCheckpoint.IsValid()
			&& bRestoredA && CommittedA.DidAppend()
			&& !bStaleCopied && !StaleCheckpoint.IsValid());
	TestTrue(TEXT("pairing and decode perform no hidden surface operations"),
		OldSurfaceA.RetirementCallCount == OldARetirementsBefore
			&& OldSurfaceA.MutationCallCount == OldAMutationsBefore
			&& NewSurfaceA.MutationCallCount == NewAMutationsBefore);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryBundleGenerationTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundle.GenerationAdvanceAndRollbackFence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryBundleGenerationTest::
RunTest(const FString&)
{
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture FixtureOne;
	FFakeArcPreviewHandoffSurface OldSurfaceOne(
		FGuid(0xF4C50001, 0xF4C50002, 0xF4C50003, 0xF4C50004),
		Consumer);
	FFakeArcPreviewHandoffSurface NewSurfaceOne(
		FGuid(0xF4C51001, 0xF4C51002, 0xF4C51003, 0xF4C51004),
		Consumer);
	FArcCompositionOwner OwnerOne;
	FArcOwnerHandoffRecoveryCheckpoint CheckpointOne;
	FArcOwnerHandoffRecoveryJournal Journal;
	FArcOwnerHandoffRecoveryPayloadEnvelope EnvelopeOne;
	FArcOwnerHandoffRecoveryBundle BundleOne;
	TArray<uint8> BytesOne;
	if (!BuildArcOwnerHandoffRecoveryBundleEvidence(
			*this,
			TEXT("ArcPreviewRecoveryBundleGenerationOne"),
			FixtureOne,
			OwnerOne,
			OldSurfaceOne,
			NewSurfaceOne,
			CheckpointOne,
			Journal,
			EnvelopeOne,
			BundleOne,
			BytesOne))
	{
		return false;
	}
	const FArcOwnerHandoffRecoveryJournal GenerationOneJournal = Journal;
	FArcOwnerHandoffRecovery Recovery;
	const auto RecoveredOne = Recovery.Execute(
		OwnerOne, CheckpointOne, OldSurfaceOne, NewSurfaceOne);
	const auto CommittedOne = RecoveredOne.HasReceipt()
		? Journal.AppendRecoveryReceipt(
			CheckpointOne, RecoveredOne.GetReceipt())
		: Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalAppendResult();
	if (!CommittedOne.DidAppend())
	{
		AddError(TEXT("Could not close generation one recovery evidence."));
		return false;
	}

	FThrownLifecycleFixture FixtureTwo;
	FFakeArcPreviewHandoffSurface OldSurfaceTwo(
		FGuid(0xF4C52001, 0xF4C52002, 0xF4C52003, 0xF4C52004),
		Consumer);
	FFakeArcPreviewHandoffSurface NewSurfaceTwo(
		FGuid(0xF4C53001, 0xF4C53002, 0xF4C53003, 0xF4C53004),
		Consumer);
	FArcCompositionOwner OwnerTwo;
	FArcOwnerHandoffResult FailedTwo;
	FArcOwnerHandoffRecoveryCheckpoint CheckpointTwo;
	if (!BuildArcOwnerHandoffRecoveryCheckpoint(
			*this,
			TEXT("ArcPreviewRecoveryBundleGenerationTwo"),
			FixtureTwo,
			OwnerTwo,
			OldSurfaceTwo,
			NewSurfaceTwo,
			FailedTwo,
			CheckpointTwo))
	{
		return false;
	}
	const auto AppendedTwo = Journal.AppendCheckpoint(CheckpointTwo);
	FArcOwnerHandoffRecoveryPayloadEnvelope EnvelopeTwo;
	FArcOwnerHandoffRecoveryBundle BundleTwo;
	TArray<uint8> BytesTwo;
	if (!AppendedTwo.DidAppend()
		|| !FArcOwnerHandoffRecoveryPayloadEnvelope::TryWrap(
			CheckpointTwo, Journal, EnvelopeTwo)
		|| !FArcOwnerHandoffRecoveryBundle::TryCreate(
			Journal, EnvelopeTwo, BundleTwo)
		|| !FArcOwnerHandoffRecoveryBundleCodec::TryEncode(
			BundleTwo, BytesTwo))
	{
		AddError(TEXT("Could not create generation two recovery bundle."));
		return false;
	}

	FArcOwnerHandoffRecoveryCheckpoint GenerationOneAllowed;
	const bool bGenerationOneAllowed =
		BundleOne.TryCopyPendingCheckpointEvidenceForJournal(
			GenerationOneJournal, 1, GenerationOneAllowed);
	FArcOwnerHandoffRecoveryCheckpoint RolledBack;
	const bool bRollbackAccepted =
		BundleOne.TryCopyPendingCheckpointEvidenceForJournal(
			GenerationOneJournal, 2, RolledBack);
	FArcOwnerHandoffRecoveryCheckpoint GenerationTwoAllowed;
	const bool bGenerationTwoAllowed =
		BundleTwo.TryCopyPendingCheckpointEvidenceForJournal(
			Journal, 2, GenerationTwoAllowed);
	FArcOwnerHandoffRecoveryCheckpoint InvalidMinimum;
	const bool bInvalidMinimumAccepted =
		BundleTwo.TryCopyPendingCheckpointEvidenceForJournal(
			Journal, 0, InvalidMinimum);
	FArcOwnerHandoffRecoveryCheckpoint FutureMinimum;
	const bool bFutureMinimumAccepted =
		BundleTwo.TryCopyPendingCheckpointEvidenceForJournal(
			Journal, 3, FutureMinimum);

	TestTrue(TEXT("each new pending checkpoint advances the canonical generation"),
		BundleOne.GetGeneration() == 1 && BundleTwo.GetGeneration() == 2
			&& Journal.GetRecordCount() == 3
			&& BytesOne != BytesTwo
			&& BundleOne.GetBundleId() != BundleTwo.GetBundleId()
			&& BundleOne.GetBundleDigest() != BundleTwo.GetBundleDigest());
	TestTrue(TEXT("caller watermark rejects rollback while accepting current generation"),
		bGenerationOneAllowed && GenerationOneAllowed.IsValid()
			&& !bRollbackAccepted && !RolledBack.IsValid()
			&& bGenerationTwoAllowed && GenerationTwoAllowed.IsValid()
			&& GenerationTwoAllowed.GetCheckpointId()
				== CheckpointTwo.GetCheckpointId());
	TestTrue(TEXT("invalid and future minimum generations fail closed"),
		!bInvalidMinimumAccepted && !InvalidMinimum.IsValid()
			&& !bFutureMinimumAccepted && !FutureMinimum.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryBundleStorageLineageTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorage.StableLineageContext",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryBundleStorageLineageTest::
RunTest(const FString&)
{
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture Fixture;
	FFakeArcPreviewHandoffSurface OldSurface(
		FGuid(0xF4D00001, 0xF4D00002, 0xF4D00003, 0xF4D00004),
		Consumer);
	FFakeArcPreviewHandoffSurface NewSurface(
		FGuid(0xF4D01001, 0xF4D01002, 0xF4D01003, 0xF4D01004),
		Consumer);
	FArcCompositionOwner Owner;
	FArcOwnerHandoffRecoveryCheckpoint CheckpointOne;
	FArcOwnerHandoffRecoveryJournal Journal;
	FArcOwnerHandoffRecoveryPayloadEnvelope EnvelopeOne;
	FArcOwnerHandoffRecoveryBundle BundleOne;
	TArray<uint8> BytesOne;
	if (!BuildArcOwnerHandoffRecoveryBundleEvidence(
			*this,
			TEXT("ArcPreviewRecoveryBundleStorageLineageOne"),
			Fixture,
			Owner,
			OldSurface,
			NewSurface,
			CheckpointOne,
			Journal,
			EnvelopeOne,
			BundleOne,
			BytesOne))
	{
		return false;
	}
	const FGuid GenerationOneJournalId = Journal.GetJournalId();
	FGuid LineageOne;
	FArcOwnerHandoffRecoveryBundleStorageContext ContextOne;
	if (!BuildArcOwnerHandoffRecoveryBundleStorageContext(
			*this,
			TEXT("StableLineage"),
			BundleOne,
			ContextOne,
			LineageOne))
	{
		return false;
	}

	FArcOwnerHandoffRecoveryCheckpoint CheckpointTwo;
	FArcOwnerHandoffRecoveryBundle BundleTwo;
	TArray<uint8> BytesTwo;
	if (!AdvanceArcOwnerHandoffRecoveryBundleGeneration(
			*this,
			TEXT("ArcPreviewRecoveryBundleStorageLineageTwo"),
			Owner,
			OldSurface,
			NewSurface,
			CheckpointOne,
			Journal,
			CheckpointTwo,
			BundleTwo,
			BytesTwo))
	{
		return false;
	}
	FGuid LineageTwo;
	const bool bDerivedTwo =
		FArcOwnerHandoffRecoveryBundleStorageAdapter::TryDeriveLineageId(
			BundleTwo, LineageTwo);
	FArcOwnerHandoffRecoveryBundleStorageContext ContextTwo;
	FString Diagnostic;
	const bool bContextTwo = bDerivedTwo
		&& FArcOwnerHandoffRecoveryBundleStorageContext::TryCreate(
			ContextOne.GetRootDirectory(),
			LineageTwo,
			ContextTwo,
			Diagnostic);
	FArcOwnerHandoffRecoveryBundleStorageContext InvalidContext;
	const bool bAcceptedRelative =
		FArcOwnerHandoffRecoveryBundleStorageContext::TryCreate(
			TEXT("Relative/RecoveryBundleRoot"),
			LineageOne,
			InvalidContext,
			Diagnostic);
	const bool bAcceptedInvalidLineage =
		FArcOwnerHandoffRecoveryBundleStorageContext::TryCreate(
			ContextOne.GetRootDirectory(),
			FGuid(),
			InvalidContext,
			Diagnostic);

	TestTrue(TEXT("lineage remains stable while journal and bundle identities advance"),
		bDerivedTwo && LineageOne.IsValid() && LineageTwo == LineageOne
			&& BundleOne.GetGeneration() == 1
			&& BundleTwo.GetGeneration() == 2
			&& GenerationOneJournalId != Journal.GetJournalId()
			&& BundleOne.GetBundleId() != BundleTwo.GetBundleId()
			&& BytesOne != BytesTwo);
	TestTrue(TEXT("all generations resolve to one deterministic same-volume slot"),
		ContextOne.IsValid() && bContextTwo && ContextTwo.IsValid()
			&& ContextOne.GetExpectedLineageId() == LineageOne
			&& ContextOne.GetPrimaryPath() == ContextTwo.GetPrimaryPath()
			&& ContextOne.GetTemporaryPath() == ContextTwo.GetTemporaryPath()
			&& ContextOne.GetPrimaryPath().EndsWith(TEXT(".smarc-bundle"))
			&& ContextOne.GetTemporaryPath().EndsWith(TEXT(".tmp")));
	TestTrue(TEXT("relative roots and invalid external lineage identities fail closed"),
		!bAcceptedRelative && !bAcceptedInvalidLineage
			&& !InvalidContext.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryBundleStorageSaveLoadTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorage.CanonicalSaveLoadAndReplay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryBundleStorageSaveLoadTest::
RunTest(const FString&)
{
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture Fixture;
	FFakeArcPreviewHandoffSurface OldSurface(
		FGuid(0xF4D10001, 0xF4D10002, 0xF4D10003, 0xF4D10004),
		Consumer);
	FFakeArcPreviewHandoffSurface NewSurface(
		FGuid(0xF4D11001, 0xF4D11002, 0xF4D11003, 0xF4D11004),
		Consumer);
	FArcCompositionOwner Owner;
	FArcOwnerHandoffRecoveryCheckpoint Checkpoint;
	FArcOwnerHandoffRecoveryJournal Journal;
	FArcOwnerHandoffRecoveryPayloadEnvelope Envelope;
	FArcOwnerHandoffRecoveryBundle Bundle;
	TArray<uint8> Canonical;
	if (!BuildArcOwnerHandoffRecoveryBundleEvidence(
			*this,
			TEXT("ArcPreviewRecoveryBundleStorageSaveLoad"),
			Fixture,
			Owner,
			OldSurface,
			NewSurface,
			Checkpoint,
			Journal,
			Envelope,
			Bundle,
			Canonical))
	{
		return false;
	}
	FArcOwnerHandoffRecoveryBundleStorageContext Context;
	FGuid LineageId;
	if (!BuildArcOwnerHandoffRecoveryBundleStorageContext(
			*this, TEXT("SaveLoad"), Bundle, Context, LineageId))
	{
		return false;
	}

	FFakeArcOwnerHandoffRecoveryPayloadStorageFileSystem FileSystem;
	FArcOwnerHandoffRecoveryBundleStorageAdapter Adapter;
	const auto Saved = Adapter.Save(Context, Bundle, 1, FileSystem);
	const auto Loaded = Adapter.Load(Context, 1, FileSystem);
	const auto Replay = Adapter.Save(Context, Bundle, 1, FileSystem);
	const int32 ReadsBeforeInvalid = FileSystem.ReadCount;
	const auto InvalidSave = Adapter.Save(Context, Bundle, 0, FileSystem);
	const auto InvalidLoad = Adapter.Load(Context, 9, FileSystem);
	TArray<uint8> StoredBytes;
	const bool bHasPrimary = FileSystem.TryGetFile(
		Context.GetPrimaryPath(), StoredBytes);
	FArcOwnerHandoffRecoveryCheckpoint Restored;
	const bool bRestored = Loaded.IsSuccess()
		&& Loaded.GetBundle().TryCopyPendingCheckpointEvidenceForJournal(
			Journal, 1, Restored);

	TestTrue(TEXT("one canonical bundle is fully flushed, replaced and decoded"),
		Saved.IsSuccess() && !Saved.WasAlreadyCurrent()
			&& Saved.GetStatus()
				== EArcOwnerHandoffRecoveryBundleStorageSaveStatus::Saved
			&& Saved.DidReplacePrimary()
			&& !Saved.TemporaryFileMayRemain()
			&& Saved.GetLineageId() == LineageId
			&& Saved.GetEncodedByteCount() == Canonical.Num()
			&& Loaded.IsSuccess()
			&& Loaded.GetStatus()
				== EArcOwnerHandoffRecoveryBundleStorageLoadStatus::Loaded
			&& Loaded.GetLineageId() == LineageId
			&& Loaded.GetGeneration() == 1
			&& Loaded.GetBundle().Matches(Bundle)
			&& bHasPrimary && StoredBytes == Canonical);
	TestTrue(TEXT("identical replay is idempotent without another write or replace"),
		Replay.IsSuccess() && Replay.WasAlreadyCurrent()
			&& !Replay.DidReplacePrimary()
			&& !Replay.TemporaryFileMayRemain()
			&& FileSystem.WriteCount == 1
			&& FileSystem.AtomicReplaceCount == 1);
	TestTrue(TEXT("external watermark is mandatory and loaded evidence remains explicit"),
		!InvalidSave.IsSuccess()
			&& InvalidSave.GetStatus()
				== EArcOwnerHandoffRecoveryBundleStorageSaveStatus::
					MinimumGenerationRejected
			&& !InvalidLoad.IsSuccess()
			&& InvalidLoad.GetStatus()
				== EArcOwnerHandoffRecoveryBundleStorageLoadStatus::
					MinimumGenerationRejected
			&& FileSystem.ReadCount == ReadsBeforeInvalid
			&& bRestored && Restored.IsValid()
			&& Restored.GetCheckpointId() == Checkpoint.GetCheckpointId());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryBundleStorageGenerationTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorage.GenerationAdvanceAndRollbackFence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryBundleStorageGenerationTest::
RunTest(const FString&)
{
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture Fixture;
	FFakeArcPreviewHandoffSurface OldSurface(
		FGuid(0xF4D20001, 0xF4D20002, 0xF4D20003, 0xF4D20004),
		Consumer);
	FFakeArcPreviewHandoffSurface NewSurface(
		FGuid(0xF4D21001, 0xF4D21002, 0xF4D21003, 0xF4D21004),
		Consumer);
	FArcCompositionOwner Owner;
	FArcOwnerHandoffRecoveryCheckpoint CheckpointOne;
	FArcOwnerHandoffRecoveryJournal Journal;
	FArcOwnerHandoffRecoveryPayloadEnvelope EnvelopeOne;
	FArcOwnerHandoffRecoveryBundle BundleOne;
	TArray<uint8> BytesOne;
	if (!BuildArcOwnerHandoffRecoveryBundleEvidence(
			*this,
			TEXT("ArcPreviewRecoveryBundleStorageGenerationOne"),
			Fixture,
			Owner,
			OldSurface,
			NewSurface,
			CheckpointOne,
			Journal,
			EnvelopeOne,
			BundleOne,
			BytesOne))
	{
		return false;
	}
	FArcOwnerHandoffRecoveryBundleStorageContext Context;
	FGuid LineageOne;
	if (!BuildArcOwnerHandoffRecoveryBundleStorageContext(
			*this, TEXT("Generation"), BundleOne, Context, LineageOne))
	{
		return false;
	}
	FFakeArcOwnerHandoffRecoveryPayloadStorageFileSystem FileSystem;
	FileSystem.SetDirectoryExists(true);
	FArcOwnerHandoffRecoveryBundleStorageAdapter Adapter;
	const auto SavedOne = Adapter.Save(Context, BundleOne, 1, FileSystem);

	FArcOwnerHandoffRecoveryCheckpoint CheckpointTwo;
	FArcOwnerHandoffRecoveryBundle BundleTwo;
	TArray<uint8> BytesTwo;
	if (!AdvanceArcOwnerHandoffRecoveryBundleGeneration(
			*this,
			TEXT("ArcPreviewRecoveryBundleStorageGenerationTwo"),
			Owner,
			OldSurface,
			NewSurface,
			CheckpointOne,
			Journal,
			CheckpointTwo,
			BundleTwo,
			BytesTwo))
	{
		return false;
	}
	FGuid LineageTwo;
	const bool bDerivedTwo =
		FArcOwnerHandoffRecoveryBundleStorageAdapter::TryDeriveLineageId(
			BundleTwo, LineageTwo);
	const auto SavedTwo = Adapter.Save(Context, BundleTwo, 2, FileSystem);
	const auto LoadedTwo = Adapter.Load(Context, 2, FileSystem);
	const int32 WritesBeforeRejectedRollback = FileSystem.WriteCount;
	const int32 ReplacesBeforeRejectedRollback = FileSystem.AtomicReplaceCount;
	const auto BelowWatermark = Adapter.Save(
		Context, BundleOne, 2, FileSystem);
	FileSystem.SetFile(Context.GetPrimaryPath(), BytesOne);
	const auto StaleLoad = Adapter.Load(Context, 2, FileSystem);
	FileSystem.SetFile(Context.GetPrimaryPath(), BytesTwo);
	const auto ExistingNewer = Adapter.Save(
		Context, BundleOne, 1, FileSystem);

	TestTrue(TEXT("new generation advances in place within one stable lineage"),
		SavedOne.IsSuccess() && SavedTwo.IsSuccess()
			&& SavedTwo.DidReplacePrimary()
			&& bDerivedTwo && LineageTwo == LineageOne
			&& LoadedTwo.IsSuccess()
			&& LoadedTwo.GetGeneration() == 2
			&& LoadedTwo.GetBundle().Matches(BundleTwo)
			&& FileSystem.WriteCount == 2
			&& FileSystem.AtomicReplaceCount == 2);
	TestTrue(TEXT("trusted watermark rejects an old candidate before filesystem mutation"),
		!BelowWatermark.IsSuccess()
			&& BelowWatermark.GetStatus()
				== EArcOwnerHandoffRecoveryBundleStorageSaveStatus::
					GenerationBelowWatermark
			&& WritesBeforeRejectedRollback == 2
			&& ReplacesBeforeRejectedRollback == 2);
	TestTrue(TEXT("load and existing-primary fences independently reject rollback"),
		!StaleLoad.IsSuccess()
			&& StaleLoad.GetStatus()
				== EArcOwnerHandoffRecoveryBundleStorageLoadStatus::
					GenerationBelowWatermark
			&& StaleLoad.GetGeneration() == 1
			&& !ExistingNewer.IsSuccess()
			&& ExistingNewer.GetStatus()
				== EArcOwnerHandoffRecoveryBundleStorageSaveStatus::
					ExistingGenerationNewer
			&& FileSystem.WriteCount == WritesBeforeRejectedRollback
			&& FileSystem.AtomicReplaceCount == ReplacesBeforeRejectedRollback);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryBundleStoragePrecommitTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorage.PrecommitFailureAtomicity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryBundleStoragePrecommitTest::
RunTest(const FString&)
{
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture Fixture;
	FFakeArcPreviewHandoffSurface OldSurface(
		FGuid(0xF4D30001, 0xF4D30002, 0xF4D30003, 0xF4D30004),
		Consumer);
	FFakeArcPreviewHandoffSurface NewSurface(
		FGuid(0xF4D31001, 0xF4D31002, 0xF4D31003, 0xF4D31004),
		Consumer);
	FArcCompositionOwner Owner;
	FArcOwnerHandoffRecoveryCheckpoint Checkpoint;
	FArcOwnerHandoffRecoveryJournal Journal;
	FArcOwnerHandoffRecoveryPayloadEnvelope Envelope;
	FArcOwnerHandoffRecoveryBundle Bundle;
	TArray<uint8> Canonical;
	if (!BuildArcOwnerHandoffRecoveryBundleEvidence(
			*this,
			TEXT("ArcPreviewRecoveryBundleStoragePrecommit"),
			Fixture,
			Owner,
			OldSurface,
			NewSurface,
			Checkpoint,
			Journal,
			Envelope,
			Bundle,
			Canonical))
	{
		return false;
	}
	FArcOwnerHandoffRecoveryBundleStorageContext Context;
	FGuid LineageId;
	if (!BuildArcOwnerHandoffRecoveryBundleStorageContext(
			*this, TEXT("Precommit"), Bundle, Context, LineageId))
	{
		return false;
	}

	using EFailure =
		FFakeArcOwnerHandoffRecoveryPayloadStorageFileSystem::EFailure;
	struct FFailureCase
	{
		EFailure Failure;
		EArcOwnerHandoffRecoveryBundleStorageSaveStatus Expected;
		bool bNeedsStaleTemporary = false;
	};
	const TArray<FFailureCase> Cases =
	{
		{EFailure::CreateDirectory,
			EArcOwnerHandoffRecoveryBundleStorageSaveStatus::
				DirectoryCreationFailed},
		{EFailure::DeleteTemporary,
			EArcOwnerHandoffRecoveryBundleStorageSaveStatus::
				StaleTemporaryCleanupFailed,
			true},
		{EFailure::OpenTemporary,
			EArcOwnerHandoffRecoveryBundleStorageSaveStatus::
				TemporaryOpenFailed},
		{EFailure::WriteTemporary,
			EArcOwnerHandoffRecoveryBundleStorageSaveStatus::
				TemporaryWriteFailed},
		{EFailure::FlushTemporary,
			EArcOwnerHandoffRecoveryBundleStorageSaveStatus::
				TemporaryFlushFailed},
		{EFailure::ReadTemporary,
			EArcOwnerHandoffRecoveryBundleStorageSaveStatus::
				TemporaryReadBackFailed},
		{EFailure::CorruptTemporaryRead,
			EArcOwnerHandoffRecoveryBundleStorageSaveStatus::
				TemporaryValidationFailed},
		{EFailure::AtomicReplace,
			EArcOwnerHandoffRecoveryBundleStorageSaveStatus::
				AtomicReplaceFailed}
	};
	FArcOwnerHandoffRecoveryBundleStorageAdapter Adapter;
	for (int32 Index = 0; Index < Cases.Num(); ++Index)
	{
		const FFailureCase& FailureCase = Cases[Index];
		FFakeArcOwnerHandoffRecoveryPayloadStorageFileSystem FileSystem;
		FileSystem.SetDirectoryExists(
			FailureCase.Failure != EFailure::CreateDirectory);
		if (FailureCase.bNeedsStaleTemporary)
		{
			FileSystem.SetFile(
				Context.GetTemporaryPath(), TArray<uint8>({0x99}));
		}
		FileSystem.SetFailure(FailureCase.Failure);
		const auto Result = Adapter.Save(Context, Bundle, 1, FileSystem);
		TArray<uint8> PrimaryBytes;
		const bool bHasPrimary = FileSystem.TryGetFile(
			Context.GetPrimaryPath(), PrimaryBytes);

		TestTrue(
			*FString::Printf(TEXT("precommit case %d classifies exactly"), Index),
			!Result.IsSuccess()
				&& Result.GetStatus() == FailureCase.Expected
				&& !Result.DidReplacePrimary()
				&& !bHasPrimary && PrimaryBytes.IsEmpty());
		TestTrue(
			*FString::Printf(TEXT("precommit case %d is one-shot"), Index),
			FileSystem.WriteCount <= 1
				&& FileSystem.AtomicReplaceCount <= 1
				&& (FailureCase.Failure != EFailure::AtomicReplace
					|| FileSystem.AtomicReplaceCount == 1));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryBundleStorageExistingTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorage.ExistingPrimaryFence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryBundleStorageExistingTest::
RunTest(const FString&)
{
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture FixtureA;
	FFakeArcPreviewHandoffSurface OldA(
		FGuid(0xF4D40001, 0xF4D40002, 0xF4D40003, 0xF4D40004),
		Consumer);
	FFakeArcPreviewHandoffSurface NewA(
		FGuid(0xF4D41001, 0xF4D41002, 0xF4D41003, 0xF4D41004),
		Consumer);
	FArcCompositionOwner OwnerA;
	FArcOwnerHandoffRecoveryCheckpoint CheckpointA;
	FArcOwnerHandoffRecoveryJournal JournalA;
	FArcOwnerHandoffRecoveryPayloadEnvelope EnvelopeA;
	FArcOwnerHandoffRecoveryBundle BundleA;
	TArray<uint8> BytesA;
	if (!BuildArcOwnerHandoffRecoveryBundleEvidence(
			*this,
			TEXT("ArcPreviewRecoveryBundleStorageExistingA"),
			FixtureA,
			OwnerA,
			OldA,
			NewA,
			CheckpointA,
			JournalA,
			EnvelopeA,
			BundleA,
			BytesA))
	{
		return false;
	}
	FArcOwnerHandoffRecoveryBundleStorageContext Context;
	FGuid LineageA;
	if (!BuildArcOwnerHandoffRecoveryBundleStorageContext(
			*this, TEXT("Existing"), BundleA, Context, LineageA))
	{
		return false;
	}

	FThrownLifecycleFixture FixtureB;
	FFakeArcPreviewHandoffSurface OldB(
		FGuid(0xF4D42001, 0xF4D42002, 0xF4D42003, 0xF4D42004),
		Consumer);
	FFakeArcPreviewHandoffSurface NewB(
		FGuid(0xF4D43001, 0xF4D43002, 0xF4D43003, 0xF4D43004),
		Consumer);
	FArcCompositionOwner OwnerB;
	FArcOwnerHandoffRecoveryCheckpoint CheckpointB;
	FArcOwnerHandoffRecoveryJournal JournalB;
	FArcOwnerHandoffRecoveryPayloadEnvelope EnvelopeB;
	FArcOwnerHandoffRecoveryBundle BundleB;
	TArray<uint8> BytesB;
	if (!BuildArcOwnerHandoffRecoveryBundleEvidence(
			*this,
			TEXT("ArcPreviewRecoveryBundleStorageExistingB"),
			FixtureB,
			OwnerB,
			OldB,
			NewB,
			CheckpointB,
			JournalB,
			EnvelopeB,
			BundleB,
			BytesB))
	{
		return false;
	}

	using EFailure =
		FFakeArcOwnerHandoffRecoveryPayloadStorageFileSystem::EFailure;
	struct FExistingCase
	{
		TArray<uint8> Bytes;
		EFailure Failure;
		EArcOwnerHandoffRecoveryBundleStorageSaveStatus Expected;
	};
	TArray<uint8> Oversized;
	Oversized.SetNumZeroed(
		FArcOwnerHandoffRecoveryBundleCodec::MaximumEncodedBytes() + 1);
	const TArray<FExistingCase> Cases =
	{
		{BytesA,
			EFailure::ReadPrimary,
			EArcOwnerHandoffRecoveryBundleStorageSaveStatus::
				ExistingReadFailed},
		{Oversized,
			EFailure::None,
			EArcOwnerHandoffRecoveryBundleStorageSaveStatus::
				ExistingSizeRejected},
		{TArray<uint8>({0x01, 0x02, 0x03}),
			EFailure::None,
			EArcOwnerHandoffRecoveryBundleStorageSaveStatus::
				ExistingDecodeRejected},
		{BytesB,
			EFailure::None,
			EArcOwnerHandoffRecoveryBundleStorageSaveStatus::
				ExistingLineageMismatch}
	};
	FArcOwnerHandoffRecoveryBundleStorageAdapter Adapter;
	for (int32 Index = 0; Index < Cases.Num(); ++Index)
	{
		FFakeArcOwnerHandoffRecoveryPayloadStorageFileSystem FileSystem;
		FileSystem.SetDirectoryExists(true);
		FileSystem.SetFile(Context.GetPrimaryPath(), Cases[Index].Bytes);
		FileSystem.SetFailure(Cases[Index].Failure);
		const auto Result = Adapter.Save(Context, BundleA, 1, FileSystem);
		TArray<uint8> Preserved;
		const bool bPreserved = FileSystem.TryGetFile(
			Context.GetPrimaryPath(), Preserved);

		TestTrue(
			*FString::Printf(TEXT("existing-primary case %d fails closed"), Index),
			!Result.IsSuccess()
				&& Result.GetStatus() == Cases[Index].Expected
				&& !Result.DidReplacePrimary()
				&& bPreserved && Preserved == Cases[Index].Bytes
				&& FileSystem.WriteCount == 0
				&& FileSystem.AtomicReplaceCount == 0);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryBundleStoragePostcommitTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorage.PostcommitOutcomeRequiresLoad",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryBundleStoragePostcommitTest::
RunTest(const FString&)
{
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture Fixture;
	FFakeArcPreviewHandoffSurface OldSurface(
		FGuid(0xF4D50001, 0xF4D50002, 0xF4D50003, 0xF4D50004),
		Consumer);
	FFakeArcPreviewHandoffSurface NewSurface(
		FGuid(0xF4D51001, 0xF4D51002, 0xF4D51003, 0xF4D51004),
		Consumer);
	FArcCompositionOwner Owner;
	FArcOwnerHandoffRecoveryCheckpoint Checkpoint;
	FArcOwnerHandoffRecoveryJournal Journal;
	FArcOwnerHandoffRecoveryPayloadEnvelope Envelope;
	FArcOwnerHandoffRecoveryBundle Bundle;
	TArray<uint8> Canonical;
	if (!BuildArcOwnerHandoffRecoveryBundleEvidence(
			*this,
			TEXT("ArcPreviewRecoveryBundleStoragePostcommit"),
			Fixture,
			Owner,
			OldSurface,
			NewSurface,
			Checkpoint,
			Journal,
			Envelope,
			Bundle,
			Canonical))
	{
		return false;
	}
	FArcOwnerHandoffRecoveryBundleStorageContext Context;
	FGuid LineageId;
	if (!BuildArcOwnerHandoffRecoveryBundleStorageContext(
			*this, TEXT("Postcommit"), Bundle, Context, LineageId))
	{
		return false;
	}

	using EFailure =
		FFakeArcOwnerHandoffRecoveryPayloadStorageFileSystem::EFailure;
	struct FFailureCase
	{
		EFailure Failure;
		EArcOwnerHandoffRecoveryBundleStorageSaveStatus Expected;
	};
	const TArray<FFailureCase> Cases =
	{
		{EFailure::ReadPrimary,
			EArcOwnerHandoffRecoveryBundleStorageSaveStatus::
				CommittedReadBackFailed},
		{EFailure::CorruptPrimaryRead,
			EArcOwnerHandoffRecoveryBundleStorageSaveStatus::
				CommittedValidationFailed}
	};
	FArcOwnerHandoffRecoveryBundleStorageAdapter Adapter;
	for (int32 Index = 0; Index < Cases.Num(); ++Index)
	{
		FFakeArcOwnerHandoffRecoveryPayloadStorageFileSystem FileSystem;
		FileSystem.SetDirectoryExists(true);
		FileSystem.SetFailure(Cases[Index].Failure);
		const auto Result = Adapter.Save(Context, Bundle, 1, FileSystem);
		FileSystem.SetFailure(EFailure::None);
		const auto ResolvedByLoad = Adapter.Load(Context, 1, FileSystem);
		TArray<uint8> PrimaryBytes;
		const bool bHasPrimary = FileSystem.TryGetFile(
			Context.GetPrimaryPath(), PrimaryBytes);

		TestTrue(
			*FString::Printf(TEXT("postcommit case %d exposes indeterminate stage"), Index),
			!Result.IsSuccess()
				&& Result.GetStatus() == Cases[Index].Expected
				&& Result.DidReplacePrimary()
				&& !Result.TemporaryFileMayRemain());
		TestTrue(
			*FString::Printf(TEXT("postcommit case %d resolves only by explicit load"), Index),
			ResolvedByLoad.IsSuccess()
				&& ResolvedByLoad.GetBundle().Matches(Bundle)
				&& bHasPrimary && PrimaryBytes == Canonical
				&& FileSystem.WriteCount == 1
				&& FileSystem.AtomicReplaceCount == 1);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryBundleStorageLoadFenceTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorage.LoadBoundsDecodeLineageAndWatermark",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryBundleStorageLoadFenceTest::
RunTest(const FString&)
{
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture FixtureA;
	FFakeArcPreviewHandoffSurface OldA(
		FGuid(0xF4D60001, 0xF4D60002, 0xF4D60003, 0xF4D60004),
		Consumer);
	FFakeArcPreviewHandoffSurface NewA(
		FGuid(0xF4D61001, 0xF4D61002, 0xF4D61003, 0xF4D61004),
		Consumer);
	FArcCompositionOwner OwnerA;
	FArcOwnerHandoffRecoveryCheckpoint CheckpointA;
	FArcOwnerHandoffRecoveryJournal JournalA;
	FArcOwnerHandoffRecoveryPayloadEnvelope EnvelopeA;
	FArcOwnerHandoffRecoveryBundle BundleA;
	TArray<uint8> BytesA;
	if (!BuildArcOwnerHandoffRecoveryBundleEvidence(
			*this,
			TEXT("ArcPreviewRecoveryBundleStorageLoadA"),
			FixtureA,
			OwnerA,
			OldA,
			NewA,
			CheckpointA,
			JournalA,
			EnvelopeA,
			BundleA,
			BytesA))
	{
		return false;
	}
	FArcOwnerHandoffRecoveryBundleStorageContext Context;
	FGuid LineageA;
	if (!BuildArcOwnerHandoffRecoveryBundleStorageContext(
			*this, TEXT("LoadFence"), BundleA, Context, LineageA))
	{
		return false;
	}

	FThrownLifecycleFixture FixtureB;
	FFakeArcPreviewHandoffSurface OldB(
		FGuid(0xF4D62001, 0xF4D62002, 0xF4D62003, 0xF4D62004),
		Consumer);
	FFakeArcPreviewHandoffSurface NewB(
		FGuid(0xF4D63001, 0xF4D63002, 0xF4D63003, 0xF4D63004),
		Consumer);
	FArcCompositionOwner OwnerB;
	FArcOwnerHandoffRecoveryCheckpoint CheckpointB;
	FArcOwnerHandoffRecoveryJournal JournalB;
	FArcOwnerHandoffRecoveryPayloadEnvelope EnvelopeB;
	FArcOwnerHandoffRecoveryBundle BundleB;
	TArray<uint8> BytesB;
	if (!BuildArcOwnerHandoffRecoveryBundleEvidence(
			*this,
			TEXT("ArcPreviewRecoveryBundleStorageLoadB"),
			FixtureB,
			OwnerB,
			OldB,
			NewB,
			CheckpointB,
			JournalB,
			EnvelopeB,
			BundleB,
			BytesB))
	{
		return false;
	}

	FFakeArcOwnerHandoffRecoveryPayloadStorageFileSystem FileSystem;
	FileSystem.SetDirectoryExists(true);
	FArcOwnerHandoffRecoveryBundleStorageAdapter Adapter;
	const auto Missing = Adapter.Load(Context, 1, FileSystem);
	TArray<uint8> Oversized;
	Oversized.SetNumZeroed(
		FArcOwnerHandoffRecoveryBundleCodec::MaximumEncodedBytes() + 1);
	FileSystem.SetFile(Context.GetPrimaryPath(), Oversized);
	const auto TooLarge = Adapter.Load(Context, 1, FileSystem);
	FileSystem.SetFile(
		Context.GetPrimaryPath(), TArray<uint8>({0x01, 0x02, 0x03}));
	const auto Corrupt = Adapter.Load(Context, 1, FileSystem);
	FileSystem.SetFile(Context.GetPrimaryPath(), BytesA);
	FileSystem.SetFailure(
		FFakeArcOwnerHandoffRecoveryPayloadStorageFileSystem::EFailure::
			ReadPrimary);
	const auto ReadFailure = Adapter.Load(Context, 1, FileSystem);
	FileSystem.SetFailure(
		FFakeArcOwnerHandoffRecoveryPayloadStorageFileSystem::EFailure::None);
	FileSystem.SetFile(Context.GetPrimaryPath(), BytesB);
	const auto Foreign = Adapter.Load(Context, 1, FileSystem);
	FileSystem.SetFile(Context.GetPrimaryPath(), BytesA);
	const auto BelowWatermark = Adapter.Load(Context, 2, FileSystem);

	TestTrue(TEXT("missing, oversized and unreadable primaries classify exactly"),
		!Missing.IsSuccess()
			&& Missing.GetStatus()
				== EArcOwnerHandoffRecoveryBundleStorageLoadStatus::Missing
			&& !TooLarge.IsSuccess()
			&& TooLarge.GetStatus()
				== EArcOwnerHandoffRecoveryBundleStorageLoadStatus::SizeRejected
			&& TooLarge.GetObservedByteCount()
				== FArcOwnerHandoffRecoveryBundleCodec::MaximumEncodedBytes() + 1
			&& !ReadFailure.IsSuccess()
			&& ReadFailure.GetStatus()
				== EArcOwnerHandoffRecoveryBundleStorageLoadStatus::ReadFailed);
	TestTrue(TEXT("codec corruption remains distinct from filesystem failure"),
		!Corrupt.IsSuccess()
			&& Corrupt.GetStatus()
				== EArcOwnerHandoffRecoveryBundleStorageLoadStatus::DecodeRejected
			&& Corrupt.GetDecodeStatus()
				!= EArcOwnerHandoffRecoveryBundleDecodeStatus::Decoded);
	TestTrue(TEXT("foreign lineage and stale generation fail after canonical decode"),
		!Foreign.IsSuccess()
			&& Foreign.GetStatus()
				== EArcOwnerHandoffRecoveryBundleStorageLoadStatus::
					LineageSlotMismatch
			&& Foreign.GetDecodeStatus()
				== EArcOwnerHandoffRecoveryBundleDecodeStatus::Decoded
			&& !BelowWatermark.IsSuccess()
			&& BelowWatermark.GetStatus()
				== EArcOwnerHandoffRecoveryBundleStorageLoadStatus::
					GenerationBelowWatermark
			&& BelowWatermark.GetGeneration() == 1
			&& FileSystem.WriteCount == 0
			&& FileSystem.AtomicReplaceCount == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryBundleStorageLocalFileTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleStorage.LocalFileAtomicGenerationReplace",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryBundleStorageLocalFileTest::
RunTest(const FString&)
{
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture Fixture;
	FFakeArcPreviewHandoffSurface OldSurface(
		FGuid(0xF4D70001, 0xF4D70002, 0xF4D70003, 0xF4D70004),
		Consumer);
	FFakeArcPreviewHandoffSurface NewSurface(
		FGuid(0xF4D71001, 0xF4D71002, 0xF4D71003, 0xF4D71004),
		Consumer);
	FArcCompositionOwner Owner;
	FArcOwnerHandoffRecoveryCheckpoint CheckpointOne;
	FArcOwnerHandoffRecoveryJournal Journal;
	FArcOwnerHandoffRecoveryPayloadEnvelope EnvelopeOne;
	FArcOwnerHandoffRecoveryBundle BundleOne;
	TArray<uint8> BytesOne;
	if (!BuildArcOwnerHandoffRecoveryBundleEvidence(
			*this,
			TEXT("ArcPreviewRecoveryBundleStorageLocalOne"),
			Fixture,
			Owner,
			OldSurface,
			NewSurface,
			CheckpointOne,
			Journal,
			EnvelopeOne,
			BundleOne,
			BytesOne))
	{
		return false;
	}
	FArcOwnerHandoffRecoveryBundleStorageContext Context;
	FGuid LineageId;
	if (!BuildArcOwnerHandoffRecoveryBundleStorageContext(
			*this, TEXT("LocalFile"), BundleOne, Context, LineageId))
	{
		return false;
	}
	IFileManager::Get().DeleteDirectory(
		*Context.GetRootDirectory(), false, true);

	FArcOwnerHandoffRecoveryBundleLocalFileSystem FileSystem;
	FArcOwnerHandoffRecoveryBundleStorageAdapter Adapter;
	const auto SavedOne = Adapter.Save(Context, BundleOne, 1, FileSystem);
	const auto LoadedOne = Adapter.Load(Context, 1, FileSystem);
	const auto ReplayedOne = Adapter.Save(Context, BundleOne, 1, FileSystem);
	FArcOwnerHandoffRecoveryCheckpoint CheckpointTwo;
	FArcOwnerHandoffRecoveryBundle BundleTwo;
	TArray<uint8> BytesTwo;
	if (!AdvanceArcOwnerHandoffRecoveryBundleGeneration(
			*this,
			TEXT("ArcPreviewRecoveryBundleStorageLocalTwo"),
			Owner,
			OldSurface,
			NewSurface,
			CheckpointOne,
			Journal,
			CheckpointTwo,
			BundleTwo,
			BytesTwo))
	{
		return false;
	}
	const int32 OldRetirementsBeforeStorage = OldSurface.RetirementCallCount;
	const int32 OldMutationsBeforeStorage = OldSurface.MutationCallCount;
	const int32 NewMutationsBeforeStorage = NewSurface.MutationCallCount;
	const auto SavedTwo = Adapter.Save(Context, BundleTwo, 2, FileSystem);
	const auto LoadedTwo = Adapter.Load(Context, 2, FileSystem);
	TArray<uint8> PrimaryBytes;
	int64 PrimarySize = INDEX_NONE;
	const auto PrimaryRead = FileSystem.ReadBounded(
		Context.GetPrimaryPath(),
		FArcOwnerHandoffRecoveryBundleCodec::MaximumEncodedBytes(),
		PrimaryBytes,
		PrimarySize);
	const bool bTemporaryMissing =
		!IFileManager::Get().FileExists(*Context.GetTemporaryPath());
	FArcOwnerHandoffRecoveryCheckpoint RestoredTwo;
	const bool bRestoredTwo = LoadedTwo.IsSuccess()
		&& LoadedTwo.GetBundle().TryCopyPendingCheckpointEvidenceForJournal(
			Journal, 2, RestoredTwo);
	const bool bCleanup = IFileManager::Get().DeleteDirectory(
		*Context.GetRootDirectory(), false, true);

	TestTrue(TEXT("local backend atomically keeps one exact newest-generation file"),
		SavedOne.IsSuccess() && LoadedOne.IsSuccess()
			&& ReplayedOne.IsSuccess() && ReplayedOne.WasAlreadyCurrent()
			&& SavedTwo.IsSuccess() && SavedTwo.DidReplacePrimary()
			&& LoadedTwo.IsSuccess()
			&& LoadedTwo.GetGeneration() == 2
			&& LoadedTwo.GetBundle().Matches(BundleTwo)
			&& PrimaryRead
				== EArcOwnerHandoffRecoveryPayloadStorageFileReadStatus::Read
			&& PrimarySize == BytesTwo.Num()
			&& PrimaryBytes == BytesTwo
			&& bTemporaryMissing);
	TestTrue(TEXT("storage remains evidence-only and delegates recovery explicitly"),
		OldSurface.RetirementCallCount == OldRetirementsBeforeStorage
			&& OldSurface.MutationCallCount == OldMutationsBeforeStorage
			&& NewSurface.MutationCallCount == NewMutationsBeforeStorage
			&& bRestoredTwo && RestoredTwo.IsValid()
			&& RestoredTwo.GetCheckpointId() == CheckpointTwo.GetCheckpointId());
	TestTrue(TEXT("test-owned bundle storage root is cleaned after verification"),
		bCleanup
			&& !IFileManager::Get().DirectoryExists(
				*Context.GetRootDirectory()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryBundleWatermarkContractTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAuthority.ContractAndDeterminism",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryBundleWatermarkContractTest::
RunTest(const FString&)
{
	const FGuid Domain = ArcOwnerHandoffRecoveryWatermarkDomain();
	const FGuid Lineage(
		0xF4541001, 0xF4541002, 0xF4541003, 0xF4541004);
	const FGuid BundleOne(
		0xF4542001, 0xF4542002, 0xF4542003, 0xF4542004);
	const FGuid BundleTwo(
		0xF4543001, 0xF4543002, 0xF4543003, 0xF4543004);
	FArcOwnerHandoffRecoveryBundleWatermarkAdvanceRequest RequestOne;
	FArcOwnerHandoffRecoveryBundleWatermarkAdvanceReceipt ReceiptOne;
	FArcOwnerHandoffRecoveryBundleWatermarkState StateOne;
	if (!BuildArcOwnerHandoffRecoveryWatermarkState(
			*this,
			Domain,
			Lineage,
			0,
			1,
			BundleOne,
			RequestOne,
			ReceiptOne,
			StateOne))
	{
		return false;
	}
	FArcOwnerHandoffRecoveryBundleWatermarkAdvanceRequest ReplayRequest;
	FArcOwnerHandoffRecoveryBundleWatermarkAdvanceReceipt ReplayReceipt;
	FArcOwnerHandoffRecoveryBundleWatermarkState ReplayState;
	if (!BuildArcOwnerHandoffRecoveryWatermarkState(
			*this,
			Domain,
			Lineage,
			0,
			1,
			BundleOne,
			ReplayRequest,
			ReplayReceipt,
			ReplayState))
	{
		return false;
	}
	FArcOwnerHandoffRecoveryBundleWatermarkAdvanceRequest RequestTwo;
	FArcOwnerHandoffRecoveryBundleWatermarkAdvanceReceipt ReceiptTwo;
	FArcOwnerHandoffRecoveryBundleWatermarkState StateTwo;
	if (!BuildArcOwnerHandoffRecoveryWatermarkState(
			*this,
			Domain,
			Lineage,
			1,
			2,
			BundleTwo,
			RequestTwo,
			ReceiptTwo,
			StateTwo))
	{
		return false;
	}

	FString Diagnostic;
	FArcOwnerHandoffRecoveryBundleWatermarkAdvanceRequest Invalid;
	const bool bAcceptedSameGeneration =
		FArcOwnerHandoffRecoveryBundleWatermarkAdvanceRequest::TryCreate(
			Domain, Lineage, 1, 1, BundleTwo, Invalid, Diagnostic);
	const bool bAcceptedRewind =
		FArcOwnerHandoffRecoveryBundleWatermarkAdvanceRequest::TryCreate(
			Domain, Lineage, 2, 1, BundleOne, Invalid, Diagnostic);
	const bool bAcceptedGenerationNine =
		FArcOwnerHandoffRecoveryBundleWatermarkAdvanceRequest::TryCreate(
			Domain, Lineage, 7, 9, BundleTwo, Invalid, Diagnostic);
	const bool bAcceptedInvalidDomain =
		FArcOwnerHandoffRecoveryBundleWatermarkAdvanceRequest::TryCreate(
			FGuid(), Lineage, 0, 1, BundleOne, Invalid, Diagnostic);

	const auto Missing =
		FArcOwnerHandoffRecoveryBundleWatermarkReadResult::Missing(
			TEXT("missing"));
	const auto Current =
		FArcOwnerHandoffRecoveryBundleWatermarkReadResult::Current(
			StateTwo, TEXT("current"));
	const auto ReadUnavailable =
		FArcOwnerHandoffRecoveryBundleWatermarkReadResult::Failure(
			EArcOwnerHandoffRecoveryBundleWatermarkReadStatus::Unavailable,
			TEXT("unavailable"));
	const auto Advanced =
		FArcOwnerHandoffRecoveryBundleWatermarkAdvanceResult::Committed(
			EArcOwnerHandoffRecoveryBundleWatermarkAdvanceStatus::Advanced,
			ReceiptTwo,
			StateTwo,
			TEXT("advanced"));
	const auto Conflict =
		FArcOwnerHandoffRecoveryBundleWatermarkAdvanceResult::Failure(
			EArcOwnerHandoffRecoveryBundleWatermarkAdvanceStatus::Conflict,
			TEXT("conflict"));
	const auto Unknown =
		FArcOwnerHandoffRecoveryBundleWatermarkAdvanceResult::Unknown(
			TEXT("unknown"));

	TestTrue(TEXT("request receipt and state identities are replay deterministic"),
		RequestOne.IsValid() && ReceiptOne.IsValid()
			&& StateOne.IsValid()
			&& ReplayRequest.IsValid() && ReplayReceipt.IsValid()
			&& ReplayState.IsValid()
			&& RequestOne.GetRequestId() == ReplayRequest.GetRequestId()
			&& ReceiptOne.GetReceiptId() == ReplayReceipt.GetReceiptId()
			&& ReplayState.MatchesRequest(RequestOne)
			&& StateTwo.MatchesRequest(RequestTwo)
			&& StateTwo.GetGeneration() == 2
			&& StateTwo.GetBundleId() == BundleTwo);
	TestTrue(TEXT("watermark request only permits strict generation advance"),
		!bAcceptedSameGeneration
			&& !bAcceptedRewind
			&& !bAcceptedGenerationNine
			&& !bAcceptedInvalidDomain
			&& !Invalid.IsValid());
	TestTrue(TEXT("authority result values classify current missing and uncertainty"),
		Missing.IsValid()
			&& Missing.GetStatus()
				== EArcOwnerHandoffRecoveryBundleWatermarkReadStatus::Missing
			&& Current.IsValid()
			&& Current.GetState().MatchesRequest(RequestTwo)
			&& ReadUnavailable.IsValid()
			&& Advanced.IsValid() && Advanced.IsSuccess()
			&& Advanced.GetReceipt().Matches(RequestTwo)
			&& Conflict.IsValid() && !Conflict.IsSuccess()
			&& Unknown.IsValid() && !Unknown.IsSuccess()
			&& Unknown.MayHaveAdvanced());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryBundleWatermarkOrderingTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAuthority.BundleBeforeWatermarkAndReplay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryBundleWatermarkOrderingTest::
RunTest(const FString&)
{
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture Fixture;
	FFakeArcPreviewHandoffSurface OldSurface(
		FGuid(0xF4544001, 0xF4544002, 0xF4544003, 0xF4544004),
		Consumer);
	FFakeArcPreviewHandoffSurface NewSurface(
		FGuid(0xF4545001, 0xF4545002, 0xF4545003, 0xF4545004),
		Consumer);
	FArcCompositionOwner Owner;
	FArcOwnerHandoffRecoveryCheckpoint Checkpoint;
	FArcOwnerHandoffRecoveryJournal Journal;
	FArcOwnerHandoffRecoveryPayloadEnvelope Envelope;
	FArcOwnerHandoffRecoveryBundle Bundle;
	TArray<uint8> Bytes;
	if (!BuildArcOwnerHandoffRecoveryBundleEvidence(
			*this,
			TEXT("ArcPreviewRecoveryWatermarkOrdering"),
			Fixture,
			Owner,
			OldSurface,
			NewSurface,
			Checkpoint,
			Journal,
			Envelope,
			Bundle,
			Bytes))
	{
		return false;
	}
	FArcOwnerHandoffRecoveryBundleStorageContext Context;
	FGuid LineageId;
	if (!BuildArcOwnerHandoffRecoveryBundleStorageContext(
			*this,
			TEXT("WatermarkOrdering"),
			Bundle,
			Context,
			LineageId))
	{
		return false;
	}

	FFakeArcOwnerHandoffRecoveryPayloadStorageFileSystem FileSystem;
	FFakeArcOwnerHandoffRecoveryBundleWatermarkAuthority Authority;
	Authority.SetPreAdvanceProbe([&FileSystem, &Context, &Bytes]()
	{
		TArray<uint8> Primary;
		return FileSystem.TryGetFile(Context.GetPrimaryPath(), Primary)
			&& Primary == Bytes;
	});
	FArcOwnerHandoffRecoveryBundleWatermarkCommitCoordinator Coordinator;
	const int32 OldRetirements = OldSurface.RetirementCallCount;
	const int32 OldMutations = OldSurface.MutationCallCount;
	const int32 NewMutations = NewSurface.MutationCallCount;
	const auto Committed = Coordinator.CommitBundleThenAdvance(
		Context,
		Bundle,
		ArcOwnerHandoffRecoveryWatermarkDomain(),
		FileSystem,
		Authority);
	const auto Replayed = Coordinator.CommitBundleThenAdvance(
		Context,
		Bundle,
		ArcOwnerHandoffRecoveryWatermarkDomain(),
		FileSystem,
		Authority);

	TestTrue(TEXT("exact bundle is readable before the one authority advance"),
		Committed.IsSuccess()
			&& Committed.GetStatus()
				== EArcOwnerHandoffRecoveryBundleWatermarkCommitStatus::
					Committed
			&& Committed.WasBundleVerified()
			&& Committed.IsWatermarkCurrent()
			&& Committed.GetPreviousGeneration() == 0
			&& Committed.GetTargetGeneration() == 1
			&& Committed.GetBundleSaveStatus()
				== EArcOwnerHandoffRecoveryBundleStorageSaveStatus::Saved
			&& Committed.GetBundleLoadStatus()
				== EArcOwnerHandoffRecoveryBundleStorageLoadStatus::Loaded
			&& Authority.bPreAdvanceProbePassed
			&& Authority.AdvanceCount == 1
			&& Authority.GetState().MatchesTarget(
				ArcOwnerHandoffRecoveryWatermarkDomain(),
				LineageId,
				1,
				Bundle.GetBundleId()));
	TestTrue(TEXT("committed replay performs no second write replace or advance"),
		Replayed.IsSuccess()
			&& Replayed.GetStatus()
				== EArcOwnerHandoffRecoveryBundleWatermarkCommitStatus::
					AlreadyCommitted
			&& Replayed.GetBundleSaveStatus()
				== EArcOwnerHandoffRecoveryBundleStorageSaveStatus::
					AlreadyCurrent
			&& Replayed.GetReceipt().GetReceiptId()
				== Committed.GetReceipt().GetReceiptId()
			&& FileSystem.WriteCount == 1
			&& FileSystem.AtomicReplaceCount == 1
			&& Authority.ReadCount == 2
			&& Authority.AdvanceCount == 1);
	TestTrue(TEXT("watermark composition has no recovery or surface side effect"),
		OldSurface.RetirementCallCount == OldRetirements
			&& OldSurface.MutationCallCount == OldMutations
			&& NewSurface.MutationCallCount == NewMutations);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryBundleWatermarkGenerationTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAuthority.GenerationAdvanceAndRollbackFence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryBundleWatermarkGenerationTest::
RunTest(const FString&)
{
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture Fixture;
	FFakeArcPreviewHandoffSurface OldSurface(
		FGuid(0xF4546001, 0xF4546002, 0xF4546003, 0xF4546004),
		Consumer);
	FFakeArcPreviewHandoffSurface NewSurface(
		FGuid(0xF4547001, 0xF4547002, 0xF4547003, 0xF4547004),
		Consumer);
	FArcCompositionOwner Owner;
	FArcOwnerHandoffRecoveryCheckpoint CheckpointOne;
	FArcOwnerHandoffRecoveryJournal Journal;
	FArcOwnerHandoffRecoveryPayloadEnvelope Envelope;
	FArcOwnerHandoffRecoveryBundle BundleOne;
	TArray<uint8> BytesOne;
	if (!BuildArcOwnerHandoffRecoveryBundleEvidence(
			*this,
			TEXT("ArcPreviewRecoveryWatermarkGenerationOne"),
			Fixture,
			Owner,
			OldSurface,
			NewSurface,
			CheckpointOne,
			Journal,
			Envelope,
			BundleOne,
			BytesOne))
	{
		return false;
	}
	FArcOwnerHandoffRecoveryBundleStorageContext Context;
	FGuid LineageId;
	if (!BuildArcOwnerHandoffRecoveryBundleStorageContext(
			*this,
			TEXT("WatermarkGeneration"),
			BundleOne,
			Context,
			LineageId))
	{
		return false;
	}

	FFakeArcOwnerHandoffRecoveryPayloadStorageFileSystem FileSystem;
	FFakeArcOwnerHandoffRecoveryBundleWatermarkAuthority Authority;
	FArcOwnerHandoffRecoveryBundleWatermarkCommitCoordinator Coordinator;
	const auto First = Coordinator.CommitBundleThenAdvance(
		Context,
		BundleOne,
		ArcOwnerHandoffRecoveryWatermarkDomain(),
		FileSystem,
		Authority);
	FArcOwnerHandoffRecoveryCheckpoint CheckpointTwo;
	FArcOwnerHandoffRecoveryBundle BundleTwo;
	TArray<uint8> BytesTwo;
	if (!First.IsSuccess()
		|| !AdvanceArcOwnerHandoffRecoveryBundleGeneration(
			*this,
			TEXT("ArcPreviewRecoveryWatermarkGenerationTwo"),
			Owner,
			OldSurface,
			NewSurface,
			CheckpointOne,
			Journal,
			CheckpointTwo,
			BundleTwo,
			BytesTwo))
	{
		return false;
	}
	FGuid LineageTwo;
	if (!FArcOwnerHandoffRecoveryBundleStorageAdapter::TryDeriveLineageId(
			BundleTwo, LineageTwo))
	{
		return false;
	}
	const auto Second = Coordinator.CommitBundleThenAdvance(
		Context,
		BundleTwo,
		ArcOwnerHandoffRecoveryWatermarkDomain(),
		FileSystem,
		Authority);
	const int32 WritesBeforeRollback = FileSystem.WriteCount;
	const int32 ReplacesBeforeRollback = FileSystem.AtomicReplaceCount;
	const int32 AdvancesBeforeRollback = Authority.AdvanceCount;
	const auto Rollback = Coordinator.CommitBundleThenAdvance(
		Context,
		BundleOne,
		ArcOwnerHandoffRecoveryWatermarkDomain(),
		FileSystem,
		Authority);

	TestTrue(TEXT("second generation replaces one slot then advances one watermark"),
		Second.IsSuccess()
			&& Second.GetPreviousGeneration() == 1
			&& Second.GetTargetGeneration() == 2
			&& Second.GetReceipt().GetPreviousGeneration() == 1
			&& Second.GetReceipt().GetCommittedGeneration() == 2
			&& Second.GetState().MatchesTarget(
				ArcOwnerHandoffRecoveryWatermarkDomain(),
				LineageId,
				2,
				BundleTwo.GetBundleId())
			&& LineageTwo == LineageId
			&& FileSystem.WriteCount == 2
			&& FileSystem.AtomicReplaceCount == 2
			&& Authority.AdvanceCount == 2);
	TestTrue(TEXT("trusted newer watermark rejects rollback before storage mutation"),
		!Rollback.IsSuccess()
			&& Rollback.GetStatus()
				== EArcOwnerHandoffRecoveryBundleWatermarkCommitStatus::
					AuthorityAheadOfBundle
			&& WritesBeforeRollback == FileSystem.WriteCount
			&& ReplacesBeforeRollback == FileSystem.AtomicReplaceCount
			&& AdvancesBeforeRollback == Authority.AdvanceCount);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryBundleWatermarkPrecommitTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAuthority.PrecommitFailureNeverAdvances",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryBundleWatermarkPrecommitTest::
RunTest(const FString&)
{
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture Fixture;
	FFakeArcPreviewHandoffSurface OldSurface(
		FGuid(0xF4548001, 0xF4548002, 0xF4548003, 0xF4548004),
		Consumer);
	FFakeArcPreviewHandoffSurface NewSurface(
		FGuid(0xF4549001, 0xF4549002, 0xF4549003, 0xF4549004),
		Consumer);
	FArcCompositionOwner Owner;
	FArcOwnerHandoffRecoveryCheckpoint Checkpoint;
	FArcOwnerHandoffRecoveryJournal Journal;
	FArcOwnerHandoffRecoveryPayloadEnvelope Envelope;
	FArcOwnerHandoffRecoveryBundle Bundle;
	TArray<uint8> Bytes;
	if (!BuildArcOwnerHandoffRecoveryBundleEvidence(
			*this,
			TEXT("ArcPreviewRecoveryWatermarkPrecommit"),
			Fixture,
			Owner,
			OldSurface,
			NewSurface,
			Checkpoint,
			Journal,
			Envelope,
			Bundle,
			Bytes))
	{
		return false;
	}
	FArcOwnerHandoffRecoveryBundleStorageContext Context;
	FGuid LineageId;
	if (!BuildArcOwnerHandoffRecoveryBundleStorageContext(
			*this,
			TEXT("WatermarkPrecommit"),
			Bundle,
			Context,
			LineageId))
	{
		return false;
	}
	FFakeArcOwnerHandoffRecoveryPayloadStorageFileSystem FileSystem;
	FileSystem.SetFailure(
		FFakeArcOwnerHandoffRecoveryPayloadStorageFileSystem::EFailure::
			AtomicReplace);
	FFakeArcOwnerHandoffRecoveryBundleWatermarkAuthority Authority;
	FArcOwnerHandoffRecoveryBundleWatermarkCommitCoordinator Coordinator;
	const auto Result = Coordinator.CommitBundleThenAdvance(
		Context,
		Bundle,
		ArcOwnerHandoffRecoveryWatermarkDomain(),
		FileSystem,
		Authority);
	TArray<uint8> Primary;

	TestTrue(TEXT("precommit storage failure cannot call trusted authority advance"),
		!Result.IsSuccess()
			&& Result.GetStatus()
				== EArcOwnerHandoffRecoveryBundleWatermarkCommitStatus::
					BundleSaveRejected
			&& Result.GetBundleSaveStatus()
				== EArcOwnerHandoffRecoveryBundleStorageSaveStatus::
					AtomicReplaceFailed
			&& !Result.WasBundleVerified()
			&& Authority.ReadCount == 1
			&& Authority.AdvanceCount == 0
			&& !Authority.HasState()
			&& !FileSystem.TryGetFile(Context.GetPrimaryPath(), Primary));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryBundleWatermarkPostcommitTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAuthority.PostcommitVerificationAndRetry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryBundleWatermarkPostcommitTest::
RunTest(const FString&)
{
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture Fixture;
	FFakeArcPreviewHandoffSurface OldSurface(
		FGuid(0xF454A001, 0xF454A002, 0xF454A003, 0xF454A004),
		Consumer);
	FFakeArcPreviewHandoffSurface NewSurface(
		FGuid(0xF454B001, 0xF454B002, 0xF454B003, 0xF454B004),
		Consumer);
	FArcCompositionOwner Owner;
	FArcOwnerHandoffRecoveryCheckpoint Checkpoint;
	FArcOwnerHandoffRecoveryJournal Journal;
	FArcOwnerHandoffRecoveryPayloadEnvelope Envelope;
	FArcOwnerHandoffRecoveryBundle Bundle;
	TArray<uint8> Bytes;
	if (!BuildArcOwnerHandoffRecoveryBundleEvidence(
			*this,
			TEXT("ArcPreviewRecoveryWatermarkPostcommit"),
			Fixture,
			Owner,
			OldSurface,
			NewSurface,
			Checkpoint,
			Journal,
			Envelope,
			Bundle,
			Bytes))
	{
		return false;
	}
	FArcOwnerHandoffRecoveryBundleStorageContext Context;
	FGuid LineageId;
	if (!BuildArcOwnerHandoffRecoveryBundleStorageContext(
			*this,
			TEXT("WatermarkPostcommit"),
			Bundle,
			Context,
			LineageId))
	{
		return false;
	}
	FFakeArcOwnerHandoffRecoveryPayloadStorageFileSystem FileSystem;
	FileSystem.SetFailure(
		FFakeArcOwnerHandoffRecoveryPayloadStorageFileSystem::EFailure::
			ReadPrimary);
	FFakeArcOwnerHandoffRecoveryBundleWatermarkAuthority Authority;
	FArcOwnerHandoffRecoveryBundleWatermarkCommitCoordinator Coordinator;
	const auto Unverified = Coordinator.CommitBundleThenAdvance(
		Context,
		Bundle,
		ArcOwnerHandoffRecoveryWatermarkDomain(),
		FileSystem,
		Authority);
	const int32 AdvancesBeforeRetry = Authority.AdvanceCount;
	FileSystem.SetFailure(
		FFakeArcOwnerHandoffRecoveryPayloadStorageFileSystem::EFailure::None);
	const auto Retried = Coordinator.CommitBundleThenAdvance(
		Context,
		Bundle,
		ArcOwnerHandoffRecoveryWatermarkDomain(),
		FileSystem,
		Authority);
	TArray<uint8> Primary;
	const bool bHasPrimary =
		FileSystem.TryGetFile(Context.GetPrimaryPath(), Primary);

	TestTrue(TEXT("postcommit read ambiguity cannot advance an unverified watermark"),
		!Unverified.IsSuccess()
			&& Unverified.GetStatus()
				== EArcOwnerHandoffRecoveryBundleWatermarkCommitStatus::
					BundleVerificationRejected
			&& Unverified.GetBundleSaveStatus()
				== EArcOwnerHandoffRecoveryBundleStorageSaveStatus::
					CommittedReadBackFailed
			&& Unverified.GetBundleLoadStatus()
				== EArcOwnerHandoffRecoveryBundleStorageLoadStatus::ReadFailed
			&& !Unverified.WasBundleVerified()
			&& AdvancesBeforeRetry == 0
			&& bHasPrimary && Primary == Bytes);
	TestTrue(TEXT("explicit retry verifies existing bytes without a second replace"),
		Retried.IsSuccess()
			&& Retried.GetBundleSaveStatus()
				== EArcOwnerHandoffRecoveryBundleStorageSaveStatus::
					AlreadyCurrent
			&& Retried.WasBundleVerified()
			&& Retried.IsWatermarkCurrent()
			&& FileSystem.WriteCount == 1
			&& FileSystem.AtomicReplaceCount == 1
			&& Authority.GetState().GetGeneration() == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryBundleWatermarkAuthorityFailureTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAuthority.AuthorityFailureLeavesRetryableBundle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryBundleWatermarkAuthorityFailureTest::
RunTest(const FString&)
{
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture Fixture;
	FFakeArcPreviewHandoffSurface OldSurface(
		FGuid(0xF454C001, 0xF454C002, 0xF454C003, 0xF454C004),
		Consumer);
	FFakeArcPreviewHandoffSurface NewSurface(
		FGuid(0xF454D001, 0xF454D002, 0xF454D003, 0xF454D004),
		Consumer);
	FArcCompositionOwner Owner;
	FArcOwnerHandoffRecoveryCheckpoint Checkpoint;
	FArcOwnerHandoffRecoveryJournal Journal;
	FArcOwnerHandoffRecoveryPayloadEnvelope Envelope;
	FArcOwnerHandoffRecoveryBundle Bundle;
	TArray<uint8> Bytes;
	if (!BuildArcOwnerHandoffRecoveryBundleEvidence(
			*this,
			TEXT("ArcPreviewRecoveryWatermarkAuthorityFailure"),
			Fixture,
			Owner,
			OldSurface,
			NewSurface,
			Checkpoint,
			Journal,
			Envelope,
			Bundle,
			Bytes))
	{
		return false;
	}
	FArcOwnerHandoffRecoveryBundleStorageContext Context;
	FGuid LineageId;
	if (!BuildArcOwnerHandoffRecoveryBundleStorageContext(
			*this,
			TEXT("WatermarkAuthorityFailure"),
			Bundle,
			Context,
			LineageId))
	{
		return false;
	}
	FFakeArcOwnerHandoffRecoveryPayloadStorageFileSystem FileSystem;
	FFakeArcOwnerHandoffRecoveryBundleWatermarkAuthority Authority;
	Authority.SetAdvanceMode(
		FFakeArcOwnerHandoffRecoveryBundleWatermarkAuthority::EAdvanceMode::
			Unavailable);
	FArcOwnerHandoffRecoveryBundleWatermarkCommitCoordinator Coordinator;
	const auto Pending = Coordinator.CommitBundleThenAdvance(
		Context,
		Bundle,
		ArcOwnerHandoffRecoveryWatermarkDomain(),
		FileSystem,
		Authority);
	const bool bHadStateWhilePending = Authority.HasState();
	const int32 WritesWhilePending = FileSystem.WriteCount;
	const int32 ReplacesWhilePending = FileSystem.AtomicReplaceCount;
	const int32 AdvanceCallsWhilePending = Authority.AdvanceCount;
	Authority.SetAdvanceMode(
		FFakeArcOwnerHandoffRecoveryBundleWatermarkAuthority::EAdvanceMode::
			Normal);
	const auto Retried = Coordinator.CommitBundleThenAdvance(
		Context,
		Bundle,
		ArcOwnerHandoffRecoveryWatermarkDomain(),
		FileSystem,
		Authority);

	TestTrue(TEXT("unavailable authority leaves verified bundle and old watermark"),
		!Pending.IsSuccess()
			&& Pending.GetStatus()
				== EArcOwnerHandoffRecoveryBundleWatermarkCommitStatus::
					BundleCommittedWatermarkPending
			&& Pending.WasBundleVerified()
			&& !Pending.IsWatermarkCurrent()
			&& !bHadStateWhilePending
			&& AdvanceCallsWhilePending == 1
			&& WritesWhilePending == 1
			&& ReplacesWhilePending == 1);
	TestTrue(TEXT("retry reuses current bundle then advances authority once"),
		Retried.IsSuccess()
			&& Retried.GetBundleSaveStatus()
				== EArcOwnerHandoffRecoveryBundleStorageSaveStatus::
					AlreadyCurrent
			&& Retried.IsWatermarkCurrent()
			&& Authority.ReadCount == 2
			&& Authority.AdvanceCount == 2
			&& Authority.HasState()
			&& FileSystem.WriteCount == 1
			&& FileSystem.AtomicReplaceCount == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryBundleWatermarkUnknownTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAuthority.UnknownOutcomeBoundedRecheck",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryBundleWatermarkUnknownTest::
RunTest(const FString&)
{
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture Fixture;
	FFakeArcPreviewHandoffSurface OldSurface(
		FGuid(0xF454E001, 0xF454E002, 0xF454E003, 0xF454E004),
		Consumer);
	FFakeArcPreviewHandoffSurface NewSurface(
		FGuid(0xF454F001, 0xF454F002, 0xF454F003, 0xF454F004),
		Consumer);
	FArcCompositionOwner Owner;
	FArcOwnerHandoffRecoveryCheckpoint Checkpoint;
	FArcOwnerHandoffRecoveryJournal Journal;
	FArcOwnerHandoffRecoveryPayloadEnvelope Envelope;
	FArcOwnerHandoffRecoveryBundle Bundle;
	TArray<uint8> Bytes;
	if (!BuildArcOwnerHandoffRecoveryBundleEvidence(
			*this,
			TEXT("ArcPreviewRecoveryWatermarkUnknown"),
			Fixture,
			Owner,
			OldSurface,
			NewSurface,
			Checkpoint,
			Journal,
			Envelope,
			Bundle,
			Bytes))
	{
		return false;
	}
	FArcOwnerHandoffRecoveryBundleStorageContext Context;
	FGuid LineageId;
	if (!BuildArcOwnerHandoffRecoveryBundleStorageContext(
			*this,
			TEXT("WatermarkUnknown"),
			Bundle,
			Context,
			LineageId))
	{
		return false;
	}
	FArcOwnerHandoffRecoveryBundleWatermarkCommitCoordinator Coordinator;

	FFakeArcOwnerHandoffRecoveryPayloadStorageFileSystem CommittedFileSystem;
	FFakeArcOwnerHandoffRecoveryBundleWatermarkAuthority CommittedAuthority;
	CommittedAuthority.SetAdvanceMode(
		FFakeArcOwnerHandoffRecoveryBundleWatermarkAuthority::EAdvanceMode::
			OutcomeUnknownAfterCommit);
	const auto Resolved = Coordinator.CommitBundleThenAdvance(
		Context,
		Bundle,
		ArcOwnerHandoffRecoveryWatermarkDomain(),
		CommittedFileSystem,
		CommittedAuthority);

	FFakeArcOwnerHandoffRecoveryPayloadStorageFileSystem PendingFileSystem;
	FFakeArcOwnerHandoffRecoveryBundleWatermarkAuthority PendingAuthority;
	PendingAuthority.SetAdvanceMode(
		FFakeArcOwnerHandoffRecoveryBundleWatermarkAuthority::EAdvanceMode::
			OutcomeUnknownBeforeCommit);
	const auto Unresolved = Coordinator.CommitBundleThenAdvance(
		Context,
		Bundle,
		ArcOwnerHandoffRecoveryWatermarkDomain(),
		PendingFileSystem,
		PendingAuthority);
	PendingAuthority.SetAdvanceMode(
		FFakeArcOwnerHandoffRecoveryBundleWatermarkAuthority::EAdvanceMode::
			Normal);
	const auto Retried = Coordinator.CommitBundleThenAdvance(
		Context,
		Bundle,
		ArcOwnerHandoffRecoveryWatermarkDomain(),
		PendingFileSystem,
		PendingAuthority);

	TestTrue(TEXT("unknown committed outcome is resolved by one authority read"),
		Resolved.IsSuccess()
			&& Resolved.GetStatus()
				== EArcOwnerHandoffRecoveryBundleWatermarkCommitStatus::
					CommittedAfterAuthorityRecheck
			&& Resolved.GetAuthorityAdvanceStatus()
				== EArcOwnerHandoffRecoveryBundleWatermarkAdvanceStatus::
					OutcomeUnknown
			&& Resolved.GetAuthorityRecheckStatus()
				== EArcOwnerHandoffRecoveryBundleWatermarkReadStatus::Current
			&& CommittedAuthority.ReadCount == 2
			&& CommittedAuthority.AdvanceCount == 1
			&& CommittedAuthority.HasState());
	TestTrue(TEXT("unresolved outcome stops after one read and stays retryable"),
		!Unresolved.IsSuccess()
			&& Unresolved.GetStatus()
				== EArcOwnerHandoffRecoveryBundleWatermarkCommitStatus::
					WatermarkOutcomeUnresolved
			&& Unresolved.WasBundleVerified()
			&& PendingAuthority.ReadCount == 3
			&& PendingAuthority.AdvanceCount == 2
			&& Retried.IsSuccess()
			&& Retried.GetBundleSaveStatus()
				== EArcOwnerHandoffRecoveryBundleStorageSaveStatus::
					AlreadyCurrent
			&& PendingFileSystem.WriteCount == 1
			&& PendingFileSystem.AtomicReplaceCount == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryBundleWatermarkFenceTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryBundleWatermarkAuthority.InputAuthorityAndConflictFences",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryBundleWatermarkFenceTest::
RunTest(const FString&)
{
	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture Fixture;
	FFakeArcPreviewHandoffSurface OldSurface(
		FGuid(0xF4550001, 0xF4550002, 0xF4550003, 0xF4550004),
		Consumer);
	FFakeArcPreviewHandoffSurface NewSurface(
		FGuid(0xF4551001, 0xF4551002, 0xF4551003, 0xF4551004),
		Consumer);
	FArcCompositionOwner Owner;
	FArcOwnerHandoffRecoveryCheckpoint Checkpoint;
	FArcOwnerHandoffRecoveryJournal Journal;
	FArcOwnerHandoffRecoveryPayloadEnvelope Envelope;
	FArcOwnerHandoffRecoveryBundle Bundle;
	TArray<uint8> Bytes;
	if (!BuildArcOwnerHandoffRecoveryBundleEvidence(
			*this,
			TEXT("ArcPreviewRecoveryWatermarkFences"),
			Fixture,
			Owner,
			OldSurface,
			NewSurface,
			Checkpoint,
			Journal,
			Envelope,
			Bundle,
			Bytes))
	{
		return false;
	}
	FArcOwnerHandoffRecoveryBundleStorageContext Context;
	FGuid LineageId;
	if (!BuildArcOwnerHandoffRecoveryBundleStorageContext(
			*this,
			TEXT("WatermarkFences"),
			Bundle,
			Context,
			LineageId))
	{
		return false;
	}
	FArcOwnerHandoffRecoveryBundleWatermarkCommitCoordinator Coordinator;

	FFakeArcOwnerHandoffRecoveryPayloadStorageFileSystem InvalidInputFs;
	FFakeArcOwnerHandoffRecoveryBundleWatermarkAuthority InvalidInputAuthority;
	const auto InvalidInput = Coordinator.CommitBundleThenAdvance(
		Context,
		Bundle,
		FGuid(),
		InvalidInputFs,
		InvalidInputAuthority);

	FFakeArcOwnerHandoffRecoveryPayloadStorageFileSystem UnavailableFs;
	FFakeArcOwnerHandoffRecoveryBundleWatermarkAuthority UnavailableAuthority;
	UnavailableAuthority.SetReadMode(
		FFakeArcOwnerHandoffRecoveryBundleWatermarkAuthority::EReadMode::
			Unavailable);
	const auto Unavailable = Coordinator.CommitBundleThenAdvance(
		Context,
		Bundle,
		ArcOwnerHandoffRecoveryWatermarkDomain(),
		UnavailableFs,
		UnavailableAuthority);

	FFakeArcOwnerHandoffRecoveryPayloadStorageFileSystem InvalidReadFs;
	FFakeArcOwnerHandoffRecoveryBundleWatermarkAuthority InvalidReadAuthority;
	InvalidReadAuthority.SetReadMode(
		FFakeArcOwnerHandoffRecoveryBundleWatermarkAuthority::EReadMode::
			Invalid);
	const auto InvalidRead = Coordinator.CommitBundleThenAdvance(
		Context,
		Bundle,
		ArcOwnerHandoffRecoveryWatermarkDomain(),
		InvalidReadFs,
		InvalidReadAuthority);

	FArcOwnerHandoffRecoveryBundleWatermarkAdvanceRequest ForeignRequest;
	FArcOwnerHandoffRecoveryBundleWatermarkAdvanceReceipt ForeignReceipt;
	FArcOwnerHandoffRecoveryBundleWatermarkState ForeignState;
	if (!BuildArcOwnerHandoffRecoveryWatermarkState(
			*this,
			FGuid(0xF4552001, 0xF4552002, 0xF4552003, 0xF4552004),
			LineageId,
			0,
			1,
			Bundle.GetBundleId(),
			ForeignRequest,
			ForeignReceipt,
			ForeignState))
	{
		return false;
	}
	FFakeArcOwnerHandoffRecoveryPayloadStorageFileSystem ForeignFs;
	FFakeArcOwnerHandoffRecoveryBundleWatermarkAuthority ForeignAuthority;
	ForeignAuthority.SetState(ForeignState);
	const auto Foreign = Coordinator.CommitBundleThenAdvance(
		Context,
		Bundle,
		ArcOwnerHandoffRecoveryWatermarkDomain(),
		ForeignFs,
		ForeignAuthority);

	FArcOwnerHandoffRecoveryBundleWatermarkAdvanceRequest AheadRequest;
	FArcOwnerHandoffRecoveryBundleWatermarkAdvanceReceipt AheadReceipt;
	FArcOwnerHandoffRecoveryBundleWatermarkState AheadState;
	if (!BuildArcOwnerHandoffRecoveryWatermarkState(
			*this,
			ArcOwnerHandoffRecoveryWatermarkDomain(),
			LineageId,
			0,
			2,
			Bundle.GetBundleId(),
			AheadRequest,
			AheadReceipt,
			AheadState))
	{
		return false;
	}
	FFakeArcOwnerHandoffRecoveryPayloadStorageFileSystem AheadFs;
	FFakeArcOwnerHandoffRecoveryBundleWatermarkAuthority AheadAuthority;
	AheadAuthority.SetState(AheadState);
	const auto Ahead = Coordinator.CommitBundleThenAdvance(
		Context,
		Bundle,
		ArcOwnerHandoffRecoveryWatermarkDomain(),
		AheadFs,
		AheadAuthority);

	FArcOwnerHandoffRecoveryBundleWatermarkAdvanceRequest ConflictRequest;
	FArcOwnerHandoffRecoveryBundleWatermarkAdvanceReceipt ConflictReceipt;
	FArcOwnerHandoffRecoveryBundleWatermarkState ConflictState;
	if (!BuildArcOwnerHandoffRecoveryWatermarkState(
			*this,
			ArcOwnerHandoffRecoveryWatermarkDomain(),
			LineageId,
			0,
			1,
			FGuid(0xF4553001, 0xF4553002, 0xF4553003, 0xF4553004),
			ConflictRequest,
			ConflictReceipt,
			ConflictState))
	{
		return false;
	}
	FFakeArcOwnerHandoffRecoveryPayloadStorageFileSystem ConflictFs;
	FFakeArcOwnerHandoffRecoveryBundleWatermarkAuthority ConflictAuthority;
	ConflictAuthority.SetState(ConflictState);
	const auto Conflict = Coordinator.CommitBundleThenAdvance(
		Context,
		Bundle,
		ArcOwnerHandoffRecoveryWatermarkDomain(),
		ConflictFs,
		ConflictAuthority);

	FFakeArcOwnerHandoffRecoveryPayloadStorageFileSystem InvalidAdvanceFs;
	FFakeArcOwnerHandoffRecoveryBundleWatermarkAuthority InvalidAdvanceAuthority;
	InvalidAdvanceAuthority.SetAdvanceMode(
		FFakeArcOwnerHandoffRecoveryBundleWatermarkAuthority::EAdvanceMode::
			Invalid);
	const auto InvalidAdvance = Coordinator.CommitBundleThenAdvance(
		Context,
		Bundle,
		ArcOwnerHandoffRecoveryWatermarkDomain(),
		InvalidAdvanceFs,
		InvalidAdvanceAuthority);

	TestTrue(TEXT("invalid input and failed reads stop before storage"),
		!InvalidInput.IsSuccess()
			&& InvalidInput.GetStatus()
				== EArcOwnerHandoffRecoveryBundleWatermarkCommitStatus::
					InputRejected
			&& InvalidInputAuthority.ReadCount == 0
			&& InvalidInputFs.WriteCount == 0
			&& !Unavailable.IsSuccess()
			&& Unavailable.GetStatus()
				== EArcOwnerHandoffRecoveryBundleWatermarkCommitStatus::
					AuthorityUnavailable
			&& UnavailableFs.WriteCount == 0
			&& !InvalidRead.IsSuccess()
			&& InvalidRead.GetStatus()
				== EArcOwnerHandoffRecoveryBundleWatermarkCommitStatus::
					AuthorityReadRejected
			&& InvalidReadFs.WriteCount == 0);
	TestTrue(TEXT("foreign newer and same-generation conflicts fail closed"),
		!Foreign.IsSuccess()
			&& Foreign.GetStatus()
				== EArcOwnerHandoffRecoveryBundleWatermarkCommitStatus::
					AuthorityStateRejected
			&& ForeignFs.WriteCount == 0
			&& !Ahead.IsSuccess()
			&& Ahead.GetStatus()
				== EArcOwnerHandoffRecoveryBundleWatermarkCommitStatus::
					AuthorityAheadOfBundle
			&& AheadFs.WriteCount == 0
			&& !Conflict.IsSuccess()
			&& Conflict.GetStatus()
				== EArcOwnerHandoffRecoveryBundleWatermarkCommitStatus::
					AuthorityBundleConflict
			&& ConflictFs.WriteCount == 0);
	TestTrue(TEXT("malformed advance success is rejected after exact bundle commit"),
		!InvalidAdvance.IsSuccess()
			&& InvalidAdvance.GetStatus()
				== EArcOwnerHandoffRecoveryBundleWatermarkCommitStatus::
					WatermarkAdvanceRejected
			&& InvalidAdvance.WasBundleVerified()
			&& InvalidAdvanceFs.WriteCount == 1
			&& InvalidAdvanceFs.AtomicReplaceCount == 1
			&& !InvalidAdvanceAuthority.HasState());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryAdmissionContractTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryAdmissionSession.ContractAndDeterminism",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryAdmissionContractTest::
RunTest(const FString&)
{
	const FGuid Domain(0xF4557001, 0xF4557002, 0xF4557003, 0xF4557004);
	const FGuid Lineage(0xF4557101, 0xF4557102, 0xF4557103, 0xF4557104);
	const FGuid Journal(0xF4557201, 0xF4557202, 0xF4557203, 0xF4557204);
	const FGuid Checkpoint(0xF4557301, 0xF4557302, 0xF4557303, 0xF4557304);
	const FGuid OtherCheckpoint(
		0xF4557401, 0xF4557402, 0xF4557403, 0xF4557404);
	FString Diagnostic;
	FArcOwnerHandoffRecoveryAdmissionRequest First;
	FArcOwnerHandoffRecoveryAdmissionRequest Replay;
	FArcOwnerHandoffRecoveryAdmissionRequest Different;
	FArcOwnerHandoffRecoveryAdmissionRequest Rejected;
	const bool bFirst = FArcOwnerHandoffRecoveryAdmissionRequest::TryCreate(
		Domain, Lineage, Journal, Checkpoint, First, Diagnostic);
	const bool bReplay = FArcOwnerHandoffRecoveryAdmissionRequest::TryCreate(
		Domain, Lineage, Journal, Checkpoint, Replay, Diagnostic);
	const bool bDifferent = FArcOwnerHandoffRecoveryAdmissionRequest::TryCreate(
		Domain, Lineage, Journal, OtherCheckpoint, Different, Diagnostic);
	const bool bRejected = FArcOwnerHandoffRecoveryAdmissionRequest::TryCreate(
		FGuid(), Lineage, Journal, Checkpoint, Rejected, Diagnostic);

	TestTrue(TEXT("request identity binds all explicit admission evidence"),
		bFirst && bReplay && bDifferent
			&& First.IsValid() && Replay.IsValid() && Different.IsValid()
			&& First.GetRequestId() == Replay.GetRequestId()
			&& First.GetRequestId() != Different.GetRequestId()
			&& First.GetAuthorityDomainId() == Domain
			&& First.GetLineageId() == Lineage
			&& First.GetExpectedJournalId() == Journal
			&& First.GetExpectedCheckpointId() == Checkpoint);
	TestTrue(TEXT("invalid explicit evidence fails closed"),
		!bRejected && !Rejected.IsValid()
			&& !FArcOwnerHandoffRecoveryAdmissionRequest().IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryAdmissionSuccessTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryAdmissionSession.ExactAdmissionAndRecovery",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryAdmissionSuccessTest::
RunTest(const FString&)
{
	FArcOwnerHandoffRecoveryAdmissionFixture Fixture;
	if (!Fixture.Build(*this, TEXT("RecoveryAdmissionSuccess")))
	{
		return false;
	}
	const int32 OldMutations = Fixture.OldSurface.MutationCallCount;
	const int32 NewMutations = Fixture.NewSurface.MutationCallCount;
	FArcOwnerHandoffRecoveryAdmissionSession Session;
	const auto Result = Session.ExecuteExplicit(
		Fixture.Request,
		Fixture.StorageContext,
		Fixture.Journal,
		Fixture.Owner,
		Fixture.OldSurface,
		Fixture.NewSurface,
		Fixture.FileSystem,
		Fixture.Authority);

	TestTrue(TEXT("exact trusted evidence admits one P20.48 recovery"),
		Result.IsValid() && Result.IsSuccess() && Result.WasRecovered()
			&& !Result.IsReplay() && Result.WasCheckpointAdmitted()
			&& Result.DidInvokeRecovery() && !Result.DidMutateSurface()
			&& Result.GetStatus()
				== EArcOwnerHandoffRecoveryAdmissionStatus::Recovered
			&& Result.GetLoadedGeneration() == Fixture.Bundle.GetGeneration()
			&& Result.GetLoadedBundleId() == Fixture.Bundle.GetBundleId()
			&& Result.GetLoadedJournalId() == Fixture.Journal.GetJournalId()
			&& Result.GetLoadedCheckpointId()
				== Fixture.Checkpoint.GetCheckpointId()
			&& Result.GetRecoveryResult().HasReceipt());
	TestTrue(TEXT("one read and one load are the only admission callbacks"),
		Fixture.Authority.ReadCount == 1
			&& Fixture.Authority.AdvanceCount == 0
			&& Fixture.Authority.LastReadAuthorityDomainId
				== Fixture.AuthorityDomainId
			&& Fixture.Authority.LastReadLineageId == Fixture.LineageId
			&& Fixture.FileSystem.ReadCount == 1
			&& Fixture.FileSystem.WriteCount == 0
			&& Fixture.FileSystem.AtomicReplaceCount == 0);
	TestTrue(TEXT("recovery changes ownership without renderer mutation"),
		Fixture.Owner.IsValid() && Fixture.Owner.IsSynchronized()
			&& Fixture.Owner.GetBoundSurfaceInstanceId()
				== Fixture.NewSurface.GetSurfaceInstanceId()
			&& Fixture.OldSurface.MutationCallCount == OldMutations
			&& Fixture.NewSurface.MutationCallCount == NewMutations
			&& !Session.IsOperationInProgress());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryAdmissionReplayTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryAdmissionSession.IdempotentReplay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryAdmissionReplayTest::
RunTest(const FString&)
{
	FArcOwnerHandoffRecoveryAdmissionFixture Fixture;
	if (!Fixture.Build(*this, TEXT("RecoveryAdmissionReplay")))
	{
		return false;
	}
	const int32 OldMutations = Fixture.OldSurface.MutationCallCount;
	const int32 NewMutations = Fixture.NewSurface.MutationCallCount;
	FArcOwnerHandoffRecoveryAdmissionSession Session;
	const auto First = Session.ExecuteExplicit(
		Fixture.Request, Fixture.StorageContext, Fixture.Journal,
		Fixture.Owner, Fixture.OldSurface, Fixture.NewSurface,
		Fixture.FileSystem, Fixture.Authority);
	const auto Replay = Session.ExecuteExplicit(
		Fixture.Request, Fixture.StorageContext, Fixture.Journal,
		Fixture.Owner, Fixture.OldSurface, Fixture.NewSurface,
		Fixture.FileSystem, Fixture.Authority);

	TestTrue(TEXT("same explicit transaction replays the P20.48 receipt"),
		First.IsValid() && First.WasRecovered()
			&& Replay.IsValid() && Replay.IsSuccess() && Replay.IsReplay()
			&& Replay.GetStatus()
				== EArcOwnerHandoffRecoveryAdmissionStatus::Replayed
			&& First.GetRecoveryResult().HasReceipt()
			&& Replay.GetRecoveryResult().HasReceipt()
			&& First.GetRecoveryResult().GetReceipt().GetReceiptId()
				== Replay.GetRecoveryResult().GetReceipt().GetReceiptId());
	TestTrue(TEXT("replay remains bounded and zero-surface-mutation"),
		Fixture.Authority.ReadCount == 2
			&& Fixture.Authority.AdvanceCount == 0
			&& Fixture.FileSystem.ReadCount == 2
			&& Fixture.FileSystem.WriteCount == 0
			&& Fixture.OldSurface.MutationCallCount == OldMutations
			&& Fixture.NewSurface.MutationCallCount == NewMutations
			&& !Session.IsOperationInProgress());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryAdmissionInputFenceTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryAdmissionSession.RequestAndCurrentJournalFences",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryAdmissionInputFenceTest::
RunTest(const FString&)
{
	FArcOwnerHandoffRecoveryAdmissionFixture Fixture;
	if (!Fixture.Build(*this, TEXT("RecoveryAdmissionInputFences")))
	{
		return false;
	}
	FArcOwnerHandoffRecoveryAdmissionSession Session;
	const FArcOwnerHandoffRecoveryAdmissionRequest InvalidRequest;
	const auto Invalid = Session.ExecuteExplicit(
		InvalidRequest, Fixture.StorageContext, Fixture.Journal,
		Fixture.Owner, Fixture.OldSurface, Fixture.NewSurface,
		Fixture.FileSystem, Fixture.Authority);

	FString Diagnostic;
	FArcOwnerHandoffRecoveryAdmissionRequest CrossLineageRequest;
	check(FArcOwnerHandoffRecoveryAdmissionRequest::TryCreate(
		Fixture.AuthorityDomainId,
		FGuid(0xF4558001, 0xF4558002, 0xF4558003, 0xF4558004),
		Fixture.Journal.GetJournalId(),
		Fixture.Checkpoint.GetCheckpointId(),
		CrossLineageRequest,
		Diagnostic));
	const auto CrossLineage = Session.ExecuteExplicit(
		CrossLineageRequest, Fixture.StorageContext, Fixture.Journal,
		Fixture.Owner, Fixture.OldSurface, Fixture.NewSurface,
		Fixture.FileSystem, Fixture.Authority);

	FArcOwnerHandoffRecoveryAdmissionRequest ForeignJournalRequest;
	check(FArcOwnerHandoffRecoveryAdmissionRequest::TryCreate(
		Fixture.AuthorityDomainId,
		Fixture.LineageId,
		FGuid(0xF4558101, 0xF4558102, 0xF4558103, 0xF4558104),
		Fixture.Checkpoint.GetCheckpointId(),
		ForeignJournalRequest,
		Diagnostic));
	const auto ForeignJournal = Session.ExecuteExplicit(
		ForeignJournalRequest, Fixture.StorageContext, Fixture.Journal,
		Fixture.Owner, Fixture.OldSurface, Fixture.NewSurface,
		Fixture.FileSystem, Fixture.Authority);
	const FArcOwnerHandoffRecoveryJournal EmptyJournal;
	const auto Empty = Session.ExecuteExplicit(
		Fixture.Request, Fixture.StorageContext, EmptyJournal,
		Fixture.Owner, Fixture.OldSurface, Fixture.NewSurface,
		Fixture.FileSystem, Fixture.Authority);

	TestTrue(TEXT("invalid and cross-lineage requests reject before callbacks"),
		Invalid.IsValid()
			&& Invalid.GetStatus()
				== EArcOwnerHandoffRecoveryAdmissionStatus::RequestRejected
			&& CrossLineage.IsValid()
			&& CrossLineage.GetStatus()
				== EArcOwnerHandoffRecoveryAdmissionStatus::RequestRejected);
	TestTrue(TEXT("non-current journals reject before authority and storage"),
		ForeignJournal.IsValid()
			&& ForeignJournal.GetStatus()
				== EArcOwnerHandoffRecoveryAdmissionStatus::CurrentJournalRejected
			&& Empty.IsValid()
			&& Empty.GetStatus()
				== EArcOwnerHandoffRecoveryAdmissionStatus::CurrentJournalRejected
			&& Fixture.Authority.ReadCount == 0
			&& Fixture.FileSystem.ReadCount == 0
			&& !Session.IsOperationInProgress());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryAdmissionAuthorityFenceTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryAdmissionSession.AuthorityFences",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryAdmissionAuthorityFenceTest::
RunTest(const FString&)
{
	FArcOwnerHandoffRecoveryAdmissionFixture Fixture;
	if (!Fixture.Build(*this, TEXT("RecoveryAdmissionAuthorityFences")))
	{
		return false;
	}
	FArcOwnerHandoffRecoveryAdmissionSession Session;
	Fixture.Authority.ClearState();
	const auto Missing = Session.ExecuteExplicit(
		Fixture.Request, Fixture.StorageContext, Fixture.Journal,
		Fixture.Owner, Fixture.OldSurface, Fixture.NewSurface,
		Fixture.FileSystem, Fixture.Authority);
	Fixture.Authority.SetReadMode(
		FFakeArcOwnerHandoffRecoveryBundleWatermarkAuthority::EReadMode::Rejected);
	const auto Rejected = Session.ExecuteExplicit(
		Fixture.Request, Fixture.StorageContext, Fixture.Journal,
		Fixture.Owner, Fixture.OldSurface, Fixture.NewSurface,
		Fixture.FileSystem, Fixture.Authority);
	Fixture.Authority.SetReadMode(
		FFakeArcOwnerHandoffRecoveryBundleWatermarkAuthority::EReadMode::Unavailable);
	const auto Unavailable = Session.ExecuteExplicit(
		Fixture.Request, Fixture.StorageContext, Fixture.Journal,
		Fixture.Owner, Fixture.OldSurface, Fixture.NewSurface,
		Fixture.FileSystem, Fixture.Authority);
	Fixture.Authority.SetReadMode(
		FFakeArcOwnerHandoffRecoveryBundleWatermarkAuthority::EReadMode::Invalid);
	const auto Invalid = Session.ExecuteExplicit(
		Fixture.Request, Fixture.StorageContext, Fixture.Journal,
		Fixture.Owner, Fixture.OldSurface, Fixture.NewSurface,
		Fixture.FileSystem, Fixture.Authority);

	FArcOwnerHandoffRecoveryBundleWatermarkAdvanceRequest ForeignRequest;
	FArcOwnerHandoffRecoveryBundleWatermarkAdvanceReceipt ForeignReceipt;
	FArcOwnerHandoffRecoveryBundleWatermarkState ForeignState;
	if (!BuildArcOwnerHandoffRecoveryWatermarkState(
			*this,
			FGuid(0xF4558201, 0xF4558202, 0xF4558203, 0xF4558204),
			Fixture.LineageId,
			0,
			1,
			Fixture.Bundle.GetBundleId(),
			ForeignRequest,
			ForeignReceipt,
			ForeignState))
	{
		return false;
	}
	Fixture.Authority.SetReadMode(
		FFakeArcOwnerHandoffRecoveryBundleWatermarkAuthority::EReadMode::Normal);
	Fixture.Authority.SetState(ForeignState);
	const auto Foreign = Session.ExecuteExplicit(
		Fixture.Request, Fixture.StorageContext, Fixture.Journal,
		Fixture.Owner, Fixture.OldSurface, Fixture.NewSurface,
		Fixture.FileSystem, Fixture.Authority);

	TestTrue(TEXT("all trusted authority failure modes remain distinct"),
		Missing.IsValid()
			&& Missing.GetStatus()
				== EArcOwnerHandoffRecoveryAdmissionStatus::AuthorityMissing
			&& Rejected.IsValid()
			&& Rejected.GetStatus()
				== EArcOwnerHandoffRecoveryAdmissionStatus::AuthorityReadRejected
			&& Unavailable.IsValid()
			&& Unavailable.GetStatus()
				== EArcOwnerHandoffRecoveryAdmissionStatus::AuthorityUnavailable
			&& Invalid.IsValid()
			&& Invalid.GetStatus()
				== EArcOwnerHandoffRecoveryAdmissionStatus::AuthorityReadRejected
			&& Foreign.IsValid()
			&& Foreign.GetStatus()
				== EArcOwnerHandoffRecoveryAdmissionStatus::AuthorityStateRejected);
	TestTrue(TEXT("authority fences never read storage or execute recovery"),
		Fixture.Authority.ReadCount == 5
			&& Fixture.Authority.AdvanceCount == 0
			&& Fixture.FileSystem.ReadCount == 0
			&& Fixture.FileSystem.WriteCount == 0
			&& !Missing.DidInvokeRecovery()
			&& !Foreign.WasCheckpointAdmitted()
			&& !Session.IsOperationInProgress());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryAdmissionBundleFenceTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryAdmissionSession.BundleAndWatermarkFences",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryAdmissionBundleFenceTest::
RunTest(const FString&)
{
	FArcOwnerHandoffRecoveryAdmissionFixture Fixture;
	if (!Fixture.Build(*this, TEXT("RecoveryAdmissionBundleFences")))
	{
		return false;
	}
	FArcOwnerHandoffRecoveryAdmissionSession Session;
	FFakeArcOwnerHandoffRecoveryPayloadStorageFileSystem MissingFileSystem;
	const auto Missing = Session.ExecuteExplicit(
		Fixture.Request, Fixture.StorageContext, Fixture.Journal,
		Fixture.Owner, Fixture.OldSurface, Fixture.NewSurface,
		MissingFileSystem, Fixture.Authority);

	FFakeArcOwnerHandoffRecoveryPayloadStorageFileSystem CorruptFileSystem;
	TArray<uint8> CorruptBytes = Fixture.Bytes;
	CorruptBytes[0] ^= 0x01;
	CorruptFileSystem.SetFile(
		Fixture.StorageContext.GetPrimaryPath(), CorruptBytes);
	const auto Corrupt = Session.ExecuteExplicit(
		Fixture.Request, Fixture.StorageContext, Fixture.Journal,
		Fixture.Owner, Fixture.OldSurface, Fixture.NewSurface,
		CorruptFileSystem, Fixture.Authority);

	FFakeArcOwnerHandoffRecoveryPayloadStorageFileSystem OversizedFileSystem;
	TArray<uint8> OversizedBytes;
	OversizedBytes.SetNumZeroed(
		FArcOwnerHandoffRecoveryBundleCodec::MaximumEncodedBytes() + 1);
	OversizedFileSystem.SetFile(
		Fixture.StorageContext.GetPrimaryPath(), OversizedBytes);
	const auto Oversized = Session.ExecuteExplicit(
		Fixture.Request, Fixture.StorageContext, Fixture.Journal,
		Fixture.Owner, Fixture.OldSurface, Fixture.NewSurface,
		OversizedFileSystem, Fixture.Authority);

	FArcOwnerHandoffRecoveryBundleWatermarkAdvanceRequest ConflictRequest;
	FArcOwnerHandoffRecoveryBundleWatermarkAdvanceReceipt ConflictReceipt;
	FArcOwnerHandoffRecoveryBundleWatermarkState ConflictState;
	if (!BuildArcOwnerHandoffRecoveryWatermarkState(
			*this,
			Fixture.AuthorityDomainId,
			Fixture.LineageId,
			0,
			1,
			FGuid(0xF4558301, 0xF4558302, 0xF4558303, 0xF4558304),
			ConflictRequest,
			ConflictReceipt,
			ConflictState))
	{
		return false;
	}
	FFakeArcOwnerHandoffRecoveryBundleWatermarkAuthority ConflictAuthority;
	ConflictAuthority.SetState(ConflictState);
	const auto Conflict = Session.ExecuteExplicit(
		Fixture.Request, Fixture.StorageContext, Fixture.Journal,
		Fixture.Owner, Fixture.OldSurface, Fixture.NewSurface,
		Fixture.FileSystem, ConflictAuthority);

	TestTrue(TEXT("missing corrupt and oversized bundle bytes fail closed"),
		Missing.IsValid()
			&& Missing.GetStatus()
				== EArcOwnerHandoffRecoveryAdmissionStatus::BundleLoadRejected
			&& Missing.GetBundleLoadStatus()
				== EArcOwnerHandoffRecoveryBundleStorageLoadStatus::Missing
			&& Corrupt.IsValid()
			&& Corrupt.GetBundleLoadStatus()
				== EArcOwnerHandoffRecoveryBundleStorageLoadStatus::DecodeRejected
			&& Oversized.IsValid()
			&& Oversized.GetBundleLoadStatus()
				== EArcOwnerHandoffRecoveryBundleStorageLoadStatus::SizeRejected);
	TestTrue(TEXT("same generation with foreign bundle identity is fenced"),
		Conflict.IsValid()
			&& Conflict.GetStatus()
				== EArcOwnerHandoffRecoveryAdmissionStatus::BundleWatermarkMismatch
			&& Conflict.GetLoadedGeneration() == 1
			&& Conflict.GetLoadedBundleId() == Fixture.Bundle.GetBundleId()
			&& !Conflict.WasCheckpointAdmitted()
			&& !Conflict.DidInvokeRecovery());
	TestTrue(TEXT("bundle admission failures are read-only"),
		MissingFileSystem.ReadCount == 1
			&& CorruptFileSystem.ReadCount == 1
			&& OversizedFileSystem.ReadCount == 1
			&& Fixture.FileSystem.ReadCount == 1
			&& Fixture.FileSystem.WriteCount == 0
			&& ConflictAuthority.AdvanceCount == 0
			&& !Session.IsOperationInProgress());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryAdmissionEvidenceFenceTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryAdmissionSession.JournalAndCheckpointFences",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryAdmissionEvidenceFenceTest::
RunTest(const FString&)
{
	FArcOwnerHandoffRecoveryAdmissionFixture CheckpointFixture;
	if (!CheckpointFixture.Build(*this, TEXT("RecoveryAdmissionCheckpointFence")))
	{
		return false;
	}
	FString Diagnostic;
	FArcOwnerHandoffRecoveryAdmissionRequest WrongCheckpointRequest;
	check(FArcOwnerHandoffRecoveryAdmissionRequest::TryCreate(
		CheckpointFixture.AuthorityDomainId,
		CheckpointFixture.LineageId,
		CheckpointFixture.Journal.GetJournalId(),
		FGuid(0xF4558401, 0xF4558402, 0xF4558403, 0xF4558404),
		WrongCheckpointRequest,
		Diagnostic));
	FArcOwnerHandoffRecoveryAdmissionSession CheckpointSession;
	const int32 CheckpointOldReads =
		CheckpointFixture.OldSurface.IdentityQueryCount;
	const auto WrongCheckpoint = CheckpointSession.ExecuteExplicit(
		WrongCheckpointRequest,
		CheckpointFixture.StorageContext,
		CheckpointFixture.Journal,
		CheckpointFixture.Owner,
		CheckpointFixture.OldSurface,
		CheckpointFixture.NewSurface,
		CheckpointFixture.FileSystem,
		CheckpointFixture.Authority);

	FArcOwnerHandoffRecoveryAdmissionFixture JournalFixture;
	if (!JournalFixture.Build(*this, TEXT("RecoveryAdmissionJournalFence")))
	{
		return false;
	}
	const FArcOwnerHandoffRecoveryJournal OriginalJournal = JournalFixture.Journal;
	const FArcOwnerHandoffRecoveryAdmissionRequest OriginalRequest =
		JournalFixture.Request;
	FArcOwnerHandoffRecoveryCheckpoint SecondCheckpoint;
	FArcOwnerHandoffRecoveryBundle SecondBundle;
	TArray<uint8> SecondBytes;
	if (!AdvanceArcOwnerHandoffRecoveryBundleGeneration(
			*this,
			TEXT("RecoveryAdmissionJournalFenceGenerationTwo"),
			JournalFixture.Owner,
			JournalFixture.OldSurface,
			JournalFixture.NewSurface,
			JournalFixture.Checkpoint,
			JournalFixture.Journal,
			SecondCheckpoint,
			SecondBundle,
			SecondBytes))
	{
		return false;
	}
	JournalFixture.FileSystem.SetFile(
		JournalFixture.StorageContext.GetPrimaryPath(), SecondBytes);
	FArcOwnerHandoffRecoveryBundleWatermarkAdvanceRequest SecondStateRequest;
	FArcOwnerHandoffRecoveryBundleWatermarkAdvanceReceipt SecondStateReceipt;
	FArcOwnerHandoffRecoveryBundleWatermarkState SecondState;
	if (!BuildArcOwnerHandoffRecoveryWatermarkState(
			*this,
			JournalFixture.AuthorityDomainId,
			JournalFixture.LineageId,
			1,
			2,
			SecondBundle.GetBundleId(),
			SecondStateRequest,
			SecondStateReceipt,
			SecondState))
	{
		return false;
	}
	JournalFixture.Authority.SetState(SecondState);
	const int32 JournalOldReads = JournalFixture.OldSurface.IdentityQueryCount;
	FArcOwnerHandoffRecoveryAdmissionSession JournalSession;
	const auto WrongJournal = JournalSession.ExecuteExplicit(
		OriginalRequest,
		JournalFixture.StorageContext,
		OriginalJournal,
		JournalFixture.Owner,
		JournalFixture.OldSurface,
		JournalFixture.NewSurface,
		JournalFixture.FileSystem,
		JournalFixture.Authority);

	TestTrue(TEXT("requested checkpoint must match canonical pending evidence"),
		WrongCheckpoint.IsValid()
			&& WrongCheckpoint.GetStatus()
				== EArcOwnerHandoffRecoveryAdmissionStatus::CheckpointEvidenceRejected
			&& !WrongCheckpoint.WasCheckpointAdmitted()
			&& !WrongCheckpoint.DidInvokeRecovery()
			&& CheckpointFixture.OldSurface.IdentityQueryCount
				== CheckpointOldReads);
	TestTrue(TEXT("trusted newer bundle cannot replace caller current journal"),
		WrongJournal.IsValid()
			&& WrongJournal.GetStatus()
				== EArcOwnerHandoffRecoveryAdmissionStatus::BundleJournalMismatch
			&& WrongJournal.GetLoadedGeneration() == 2
			&& WrongJournal.GetLoadedBundleId() == SecondBundle.GetBundleId()
			&& !WrongJournal.WasCheckpointAdmitted()
			&& !WrongJournal.DidInvokeRecovery()
			&& JournalFixture.OldSurface.IdentityQueryCount == JournalOldReads);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryAdmissionLiveFenceTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryAdmissionSession.LiveAuthorityAndReentryFences",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryAdmissionLiveFenceTest::
RunTest(const FString&)
{
	FArcOwnerHandoffRecoveryAdmissionFixture DriftFixture;
	if (!DriftFixture.Build(*this, TEXT("RecoveryAdmissionLiveFence")))
	{
		return false;
	}
	FFakeArcPreviewHandoffSurface ForeignOldSurface(
		FGuid(0xF4558501, 0xF4558502, 0xF4558503, 0xF4558504),
		DriftFixture.Consumer,
		DriftFixture.Checkpoint.GetRetiredSurfaceCursor());
	const int32 ForeignMutations = ForeignOldSurface.MutationCallCount;
	const int32 NewMutations = DriftFixture.NewSurface.MutationCallCount;
	FArcOwnerHandoffRecoveryAdmissionSession DriftSession;
	const auto Drifted = DriftSession.ExecuteExplicit(
		DriftFixture.Request,
		DriftFixture.StorageContext,
		DriftFixture.Journal,
		DriftFixture.Owner,
		ForeignOldSurface,
		DriftFixture.NewSurface,
		DriftFixture.FileSystem,
		DriftFixture.Authority);

	FArcOwnerHandoffRecoveryAdmissionFixture ReentrantFixture;
	if (!ReentrantFixture.Build(*this, TEXT("RecoveryAdmissionReentry")))
	{
		return false;
	}
	FArcOwnerHandoffRecoveryAdmissionSession ReentrantSession;
	FArcOwnerHandoffRecoveryAdmissionResult Nested;
	ReentrantFixture.Authority.SetReadCallback([&]()
	{
		Nested = ReentrantSession.ExecuteExplicit(
			ReentrantFixture.Request,
			ReentrantFixture.StorageContext,
			ReentrantFixture.Journal,
			ReentrantFixture.Owner,
			ReentrantFixture.OldSurface,
			ReentrantFixture.NewSurface,
			ReentrantFixture.FileSystem,
			ReentrantFixture.Authority);
	});
	const int32 ReentrantOldMutations =
		ReentrantFixture.OldSurface.MutationCallCount;
	const int32 ReentrantNewMutations =
		ReentrantFixture.NewSurface.MutationCallCount;
	const auto Outer = ReentrantSession.ExecuteExplicit(
		ReentrantFixture.Request,
		ReentrantFixture.StorageContext,
		ReentrantFixture.Journal,
		ReentrantFixture.Owner,
		ReentrantFixture.OldSurface,
		ReentrantFixture.NewSurface,
		ReentrantFixture.FileSystem,
		ReentrantFixture.Authority);

	TestTrue(TEXT("fresh P20.48 authority drift rejects after evidence admission"),
		Drifted.IsValid()
			&& Drifted.GetStatus()
				== EArcOwnerHandoffRecoveryAdmissionStatus::RecoveryRejected
			&& Drifted.WasCheckpointAdmitted()
			&& Drifted.DidInvokeRecovery()
			&& Drifted.GetRecoveryResult().GetStatus()
				== EArcOwnerHandoffRecoveryStatus::OldSurfaceMismatch
			&& !Drifted.DidMutateSurface()
			&& ForeignOldSurface.MutationCallCount == ForeignMutations
			&& DriftFixture.NewSurface.MutationCallCount == NewMutations);
	TestTrue(TEXT("authority callback cannot re-enter the admission session"),
		Nested.IsValid()
			&& Nested.GetStatus()
				== EArcOwnerHandoffRecoveryAdmissionStatus::OperationInProgress
			&& !Nested.WasCheckpointAdmitted()
			&& !Nested.DidInvokeRecovery()
			&& Outer.IsValid() && Outer.WasRecovered()
			&& ReentrantFixture.Authority.ReadCount == 1
			&& ReentrantFixture.Authority.AdvanceCount == 0
			&& ReentrantFixture.FileSystem.ReadCount == 1
			&& ReentrantFixture.OldSurface.MutationCallCount
				== ReentrantOldMutations
			&& ReentrantFixture.NewSurface.MutationCallCount
				== ReentrantNewMutations
			&& !ReentrantSession.IsOperationInProgress());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryCompletionRequestTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionSession.RequestContractAndDeterminism",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryCompletionRequestTest::
RunTest(const FString&)
{
	FArcOwnerHandoffRecoveryCompletionFixture Fixture;
	if (!Fixture.Build(*this, TEXT("RecoveryCompletionRequest")))
	{
		return false;
	}
	FString Diagnostic;
	FArcOwnerHandoffRecoveryCompletionRequest Same;
	FArcOwnerHandoffRecoveryCompletionRequest Other;
	FArcOwnerHandoffRecoveryCompletionRequest InvalidSameDomain;
	const bool bSame = FArcOwnerHandoffRecoveryCompletionRequest::TryCreate(
		Fixture.CompletionAuthorityDomainId,
		Fixture.Admission.Request,
		Same,
		Diagnostic);
	const bool bOther = FArcOwnerHandoffRecoveryCompletionRequest::TryCreate(
		FGuid(0xF4561001, 0xF4561002, 0xF4561003, 0xF4561004),
		Fixture.Admission.Request,
		Other,
		Diagnostic);
	const bool bInvalid = FArcOwnerHandoffRecoveryCompletionRequest::TryCreate(
		Fixture.Admission.AuthorityDomainId,
		Fixture.Admission.Request,
		InvalidSameDomain,
		Diagnostic);

	TestTrue(TEXT("completion request binds a distinct explicit trust domain"),
		Fixture.Request.IsValid()
			&& Fixture.Request.GetCompletionAuthorityDomainId()
				== Fixture.CompletionAuthorityDomainId
			&& Fixture.Request.GetAdmissionRequest().GetRequestId()
				== Fixture.Admission.Request.GetRequestId());
	TestTrue(TEXT("same canonical intent is deterministic and domains separate"),
		bSame && Same.IsValid()
			&& Same.GetRequestId() == Fixture.Request.GetRequestId()
			&& bOther && Other.IsValid()
			&& Other.GetRequestId() != Fixture.Request.GetRequestId()
			&& !bInvalid && !InvalidSameDomain.IsValid());
	TestTrue(TEXT("request construction performs no authority or file callback"),
		Fixture.Authority.Pending.ReadCount == 0
			&& Fixture.Authority.Completion.ReadCount == 0
			&& Fixture.Admission.FileSystem.ReadCount == 0
			&& Fixture.Admission.FileSystem.WriteCount == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryCompletionExactTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionSession.ExactDurableCompletion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryCompletionExactTest::
RunTest(const FString&)
{
	FArcOwnerHandoffRecoveryCompletionFixture Fixture;
	if (!Fixture.Build(*this, TEXT("RecoveryCompletionExact")))
	{
		return false;
	}
	const int32 OldMutations = Fixture.Admission.OldSurface.MutationCallCount;
	const int32 NewMutations = Fixture.Admission.NewSurface.MutationCallCount;
	FArcOwnerHandoffRecoveryCompletionSession Session;
	const auto Result = Fixture.Execute(Session);

	TestTrue(TEXT("exact P20.55 receipt closes one terminal journal copy"),
		Result.IsValid() && Result.IsSuccess() && !Result.IsReplay()
			&& Result.GetStatus()
				== EArcOwnerHandoffRecoveryCompletionSessionStatus::Completed
			&& Result.DidInvokeAdmission()
			&& Result.GetAdmissionResult().WasRecovered()
			&& Result.GetJournalAppendResult().DidAppend()
			&& Result.GetCompletion().IsValid()
			&& Result.GetTerminalJournal().GetLatestDisposition()
				== EArcOwnerHandoffRecoveryJournalDisposition::RecoveryCommitted
			&& Result.GetTerminalJournal().GetRecordCount()
				== Fixture.Admission.Journal.GetRecordCount() + 1
			&& Fixture.Admission.Journal.GetLatestDisposition()
				== EArcOwnerHandoffRecoveryJournalDisposition::CheckpointPending);
	TestTrue(TEXT("completion bytes are verified before a separate watermark advances"),
		Result.WasCompletionVerified()
			&& Result.IsCompletionAuthorityCurrent()
			&& Result.GetCompletionSaveStatus()
				== EArcOwnerHandoffRecoveryCompletionStorageSaveStatus::Saved
			&& Result.GetCompletionLoadStatus()
				== EArcOwnerHandoffRecoveryCompletionStorageLoadStatus::Loaded
			&& Fixture.Authority.Completion.GetState().MatchesTarget(
				Fixture.CompletionAuthorityDomainId,
				Fixture.Admission.LineageId,
				Fixture.Admission.Bundle.GetGeneration(),
				Result.GetCompletion().GetCompletionId()));
	TestTrue(TEXT("bounded completion owns no surface mutation"),
		!Result.DidMutateSurface()
			&& Fixture.Admission.OldSurface.MutationCallCount == OldMutations
			&& Fixture.Admission.NewSurface.MutationCallCount == NewMutations
			&& Fixture.Authority.Completion.ReadCount == 1
			&& Fixture.Authority.Pending.ReadCount == 1
			&& Fixture.Authority.Completion.AdvanceCount == 1
			&& Fixture.Authority.Pending.AdvanceCount == 0
			&& Fixture.Admission.FileSystem.WriteCount == 1
			&& !Session.IsOperationInProgress());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryCompletionCodecTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionSession.CanonicalCodecAndTamperFence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryCompletionCodecTest::
RunTest(const FString&)
{
	FArcOwnerHandoffRecoveryCompletionFixture Fixture;
	if (!Fixture.Build(*this, TEXT("RecoveryCompletionCodec")))
	{
		return false;
	}
	FArcOwnerHandoffRecoveryCompletionSession Session;
	const auto Result = Fixture.Execute(Session);
	TArray<uint8> FirstBytes;
	TArray<uint8> SecondBytes;
	if (!Result.IsSuccess()
		|| !FArcOwnerHandoffRecoveryCompletionCodec::TryEncode(
			Result.GetCompletion(), FirstBytes)
		|| !FArcOwnerHandoffRecoveryCompletionCodec::TryEncode(
			Result.GetCompletion(), SecondBytes))
	{
		return false;
	}
	const auto Decoded =
		FArcOwnerHandoffRecoveryCompletionCodec::Decode(FirstBytes);
	TArray<uint8> Tampered = FirstBytes;
	Tampered[0] ^= 0x01;
	const auto TamperedDecoded =
		FArcOwnerHandoffRecoveryCompletionCodec::Decode(Tampered);
	TArray<uint8> Truncated = FirstBytes;
	Truncated.Pop(EAllowShrinking::No);
	const auto TruncatedDecoded =
		FArcOwnerHandoffRecoveryCompletionCodec::Decode(Truncated);

	TestTrue(TEXT("completion codec round-trips one canonical representation"),
		!FirstBytes.IsEmpty() && FirstBytes == SecondBytes
			&& FirstBytes.Num()
				<= FArcOwnerHandoffRecoveryCompletionCodec::MaximumEncodedBytes()
			&& Decoded.IsSuccess()
			&& Decoded.GetStatus()
				== EArcOwnerHandoffRecoveryCompletionDecodeStatus::Decoded
			&& Decoded.GetCompletion().Matches(Result.GetCompletion()));
	TestTrue(TEXT("magic tamper and truncation fail closed"),
		!TamperedDecoded.IsSuccess()
			&& TamperedDecoded.GetStatus()
				== EArcOwnerHandoffRecoveryCompletionDecodeStatus::MagicMismatch
			&& !TruncatedDecoded.IsSuccess());
	TestTrue(TEXT("terminal evidence is distinct from the pending journal"),
		Decoded.GetCompletion().GetSourceJournal().GetJournalId()
			== Fixture.Admission.Journal.GetJournalId()
			&& Decoded.GetCompletion().GetTerminalJournal().GetJournalId()
				!= Fixture.Admission.Journal.GetJournalId()
			&& Decoded.GetCompletion().GetRecoveryReceiptId()
				== Result.GetAdmissionResult().GetRecoveryResult()
					.GetReceipt().GetReceiptId());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryCompletionReplayTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionSession.TrustedReplaySkipsRecovery",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryCompletionReplayTest::
RunTest(const FString&)
{
	FArcOwnerHandoffRecoveryCompletionFixture Fixture;
	if (!Fixture.Build(*this, TEXT("RecoveryCompletionReplay")))
	{
		return false;
	}
	FArcOwnerHandoffRecoveryCompletionSession FirstSession;
	const auto First = Fixture.Execute(FirstSession);
	if (!First.IsSuccess())
	{
		return false;
	}
	const int32 PendingReads = Fixture.Authority.Pending.ReadCount;
	const int32 Advances = Fixture.Authority.Completion.AdvanceCount;
	const int32 FileReads = Fixture.Admission.FileSystem.ReadCount;
	const int32 OldIdentityReads =
		Fixture.Admission.OldSurface.IdentityQueryCount;
	const int32 NewIdentityReads =
		Fixture.Admission.NewSurface.IdentityQueryCount;
	const int32 OldMutations = Fixture.Admission.OldSurface.MutationCallCount;
	const int32 NewMutations = Fixture.Admission.NewSurface.MutationCallCount;

	FArcOwnerHandoffRecoveryCompletionSession PendingReplaySession;
	const auto PendingReplay = Fixture.Execute(PendingReplaySession);
	FArcOwnerHandoffRecoveryCompletionSession TerminalReplaySession;
	const auto TerminalReplay = TerminalReplaySession.ExecuteExplicit(
		Fixture.Request,
		Fixture.Admission.StorageContext,
		Fixture.CompletionStorageContext,
		First.GetTerminalJournal(),
		Fixture.Admission.Owner,
		Fixture.Admission.OldSurface,
		Fixture.Admission.NewSurface,
		Fixture.Admission.FileSystem,
		Fixture.Authority);

	TestTrue(TEXT("trusted completion replays from pending or terminal caller state"),
		PendingReplay.IsValid() && PendingReplay.IsReplay()
			&& TerminalReplay.IsValid() && TerminalReplay.IsReplay()
			&& PendingReplay.GetCompletion().GetCompletionId()
				== First.GetCompletion().GetCompletionId()
			&& TerminalReplay.GetTerminalJournal().GetJournalId()
				== First.GetTerminalJournal().GetJournalId());
	TestTrue(TEXT("trusted replay never invokes P20.55 or advances authority"),
		!PendingReplay.DidInvokeAdmission()
			&& !TerminalReplay.DidInvokeAdmission()
			&& Fixture.Authority.Pending.ReadCount == PendingReads
			&& Fixture.Authority.Completion.AdvanceCount == Advances
			&& Fixture.Admission.FileSystem.ReadCount == FileReads + 2);
	TestTrue(TEXT("trusted replay never reaches either live surface"),
		Fixture.Admission.OldSurface.IdentityQueryCount == OldIdentityReads
			&& Fixture.Admission.NewSurface.IdentityQueryCount == NewIdentityReads
			&& Fixture.Admission.OldSurface.MutationCallCount == OldMutations
			&& Fixture.Admission.NewSurface.MutationCallCount == NewMutations);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryCompletionPendingRepairTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionSession.DurableEvidenceBeforeAuthorityRepair",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryCompletionPendingRepairTest::
RunTest(const FString&)
{
	FArcOwnerHandoffRecoveryCompletionFixture Fixture;
	if (!Fixture.Build(*this, TEXT("RecoveryCompletionPendingRepair")))
	{
		return false;
	}
	Fixture.Authority.Completion.SetAdvanceMode(
		FFakeArcOwnerHandoffRecoveryBundleWatermarkAuthority::EAdvanceMode::
			Unavailable);
	FArcOwnerHandoffRecoveryCompletionSession FirstSession;
	const auto Pending = Fixture.Execute(FirstSession);
	const int32 WritesAfterPending = Fixture.Admission.FileSystem.WriteCount;
	const int32 OldMutations = Fixture.Admission.OldSurface.MutationCallCount;
	const int32 NewMutations = Fixture.Admission.NewSurface.MutationCallCount;
	Fixture.Authority.Completion.SetAdvanceMode(
		FFakeArcOwnerHandoffRecoveryBundleWatermarkAuthority::EAdvanceMode::
			Normal);
	FArcOwnerHandoffRecoveryCompletionSession RepairSession;
	const auto Repaired = Fixture.Execute(RepairSession);

	TestTrue(TEXT("authority outage leaves verified completion evidence explicit"),
		Pending.IsValid() && !Pending.IsSuccess()
			&& Pending.GetStatus()
				== EArcOwnerHandoffRecoveryCompletionSessionStatus::
					CompletionEvidenceCommittedAuthorityPending
			&& Pending.WasCompletionVerified()
			&& !Pending.IsCompletionAuthorityCurrent());
	TestTrue(TEXT("repair replays P20.48 receipt and reuses exact durable bytes"),
		Repaired.IsValid() && Repaired.IsSuccess()
			&& Repaired.GetAdmissionResult().IsReplay()
			&& Repaired.GetCompletionSaveStatus()
				== EArcOwnerHandoffRecoveryCompletionStorageSaveStatus::AlreadyCurrent
			&& Repaired.GetCompletion().GetCompletionId()
				== Pending.GetCompletion().GetCompletionId()
			&& Fixture.Admission.FileSystem.WriteCount == WritesAfterPending);
	TestTrue(TEXT("repair performs no surface mutation"),
		Fixture.Admission.OldSurface.MutationCallCount == OldMutations
			&& Fixture.Admission.NewSurface.MutationCallCount == NewMutations
			&& Fixture.Authority.Completion.AdvanceCount == 2
			&& Fixture.Authority.Pending.ReadCount == 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryCompletionUnknownTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionSession.UnknownAuthorityOutcomeClosure",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryCompletionUnknownTest::
RunTest(const FString&)
{
	FArcOwnerHandoffRecoveryCompletionFixture CommittedFixture;
	if (!CommittedFixture.Build(*this, TEXT("RecoveryCompletionUnknownCommitted")))
	{
		return false;
	}
	CommittedFixture.Authority.Completion.SetAdvanceMode(
		FFakeArcOwnerHandoffRecoveryBundleWatermarkAuthority::EAdvanceMode::
			OutcomeUnknownAfterCommit);
	FArcOwnerHandoffRecoveryCompletionSession CommittedSession;
	const auto Resolved = CommittedFixture.Execute(CommittedSession);

	FArcOwnerHandoffRecoveryCompletionFixture UnresolvedFixture;
	if (!UnresolvedFixture.Build(*this, TEXT("RecoveryCompletionUnknownMissing")))
	{
		return false;
	}
	UnresolvedFixture.Authority.Completion.SetAdvanceMode(
		FFakeArcOwnerHandoffRecoveryBundleWatermarkAuthority::EAdvanceMode::
			OutcomeUnknownBeforeCommit);
	FArcOwnerHandoffRecoveryCompletionSession UnresolvedSession;
	const auto Unresolved = UnresolvedFixture.Execute(UnresolvedSession);
	UnresolvedFixture.Authority.Completion.SetAdvanceMode(
		FFakeArcOwnerHandoffRecoveryBundleWatermarkAuthority::EAdvanceMode::
			Normal);
	FArcOwnerHandoffRecoveryCompletionSession RetrySession;
	const auto Retried = UnresolvedFixture.Execute(RetrySession);

	TestTrue(TEXT("unknown-after-commit resolves with one exact re-read"),
		Resolved.IsValid() && Resolved.IsSuccess()
			&& Resolved.GetStatus()
				== EArcOwnerHandoffRecoveryCompletionSessionStatus::
					CompletedAfterAuthorityRecheck
			&& Resolved.GetCompletionAuthorityRecheckStatus()
				== EArcOwnerHandoffRecoveryBundleWatermarkReadStatus::Current
			&& CommittedFixture.Authority.Completion.ReadCount == 2);
	TestTrue(TEXT("unknown-before-commit stays unresolved without false success"),
		Unresolved.IsValid() && !Unresolved.IsSuccess()
			&& Unresolved.GetStatus()
				== EArcOwnerHandoffRecoveryCompletionSessionStatus::
					CompletionOutcomeUnresolved
			&& !Unresolved.IsCompletionAuthorityCurrent());
	TestTrue(TEXT("bounded retry closes unresolved evidence without rewriting"),
		Retried.IsValid() && Retried.IsSuccess()
			&& Retried.GetAdmissionResult().IsReplay()
			&& Retried.GetCompletionSaveStatus()
				== EArcOwnerHandoffRecoveryCompletionStorageSaveStatus::AlreadyCurrent
			&& Retried.GetCompletion().GetCompletionId()
				== Unresolved.GetCompletion().GetCompletionId());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryCompletionTrustedFenceTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionSession.TrustedRollbackAndConflictFences",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryCompletionTrustedFenceTest::
RunTest(const FString&)
{
	FArcOwnerHandoffRecoveryCompletionFixture MissingFixture;
	if (!MissingFixture.Build(*this, TEXT("RecoveryCompletionTrustedMissing")))
	{
		return false;
	}
	FArcOwnerHandoffRecoveryCompletionSession CommitSession;
	const auto Committed = MissingFixture.Execute(CommitSession);
	if (!Committed.IsSuccess())
	{
		return false;
	}
	MissingFixture.Admission.FileSystem.DeleteFile(
		MissingFixture.CompletionStorageContext.GetPrimaryPath());
	const int32 PendingReads = MissingFixture.Authority.Pending.ReadCount;
	const int32 OldIdentityReads =
		MissingFixture.Admission.OldSurface.IdentityQueryCount;
	FArcOwnerHandoffRecoveryCompletionSession MissingSession;
	const auto Missing = MissingFixture.Execute(MissingSession);

	FArcOwnerHandoffRecoveryCompletionFixture ConflictFixture;
	if (!ConflictFixture.Build(*this, TEXT("RecoveryCompletionTrustedConflict")))
	{
		return false;
	}
	FArcOwnerHandoffRecoveryCompletionSession ConflictCommitSession;
	const auto ConflictCommitted = ConflictFixture.Execute(ConflictCommitSession);
	FArcOwnerHandoffRecoveryBundleWatermarkAdvanceRequest ForeignRequest;
	FArcOwnerHandoffRecoveryBundleWatermarkAdvanceReceipt ForeignReceipt;
	FArcOwnerHandoffRecoveryBundleWatermarkState ForeignState;
	if (!ConflictCommitted.IsSuccess()
		|| !BuildArcOwnerHandoffRecoveryWatermarkState(
			*this,
			ConflictFixture.CompletionAuthorityDomainId,
			ConflictFixture.Admission.LineageId,
			0,
			ConflictCommitted.GetTargetGeneration(),
			FGuid(0xF4562001, 0xF4562002, 0xF4562003, 0xF4562004),
			ForeignRequest,
			ForeignReceipt,
			ForeignState))
	{
		return false;
	}
	ConflictFixture.Authority.Completion.SetState(ForeignState);
	const int32 ConflictPendingReads =
		ConflictFixture.Authority.Pending.ReadCount;
	FArcOwnerHandoffRecoveryCompletionSession ConflictSession;
	const auto Conflict = ConflictFixture.Execute(ConflictSession);

	TestTrue(TEXT("trusted watermark plus missing bytes closes before recovery"),
		Missing.IsValid()
			&& Missing.GetStatus()
				== EArcOwnerHandoffRecoveryCompletionSessionStatus::
					TrustedCompletionLoadRejected
			&& Missing.GetCompletionLoadStatus()
				== EArcOwnerHandoffRecoveryCompletionStorageLoadStatus::Missing
			&& !Missing.DidInvokeAdmission()
			&& MissingFixture.Authority.Pending.ReadCount == PendingReads
			&& MissingFixture.Admission.OldSurface.IdentityQueryCount
				== OldIdentityReads);
	TestTrue(TEXT("same generation foreign trusted identity also closes"),
		Conflict.IsValid()
			&& Conflict.GetStatus()
				== EArcOwnerHandoffRecoveryCompletionSessionStatus::
					TrustedCompletionMismatch
			&& !Conflict.DidInvokeAdmission()
			&& ConflictFixture.Authority.Pending.ReadCount
				== ConflictPendingReads);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryCompletionInputTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionSession.InputAuthorityAndReentryFences",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryCompletionInputTest::
RunTest(const FString&)
{
	FArcOwnerHandoffRecoveryCompletionFixture Fixture;
	if (!Fixture.Build(*this, TEXT("RecoveryCompletionInputFences")))
	{
		return false;
	}
	FArcOwnerHandoffRecoveryCompletionSession Session;
	const auto Invalid = Session.ExecuteExplicit(
		FArcOwnerHandoffRecoveryCompletionRequest(),
		Fixture.Admission.StorageContext,
		Fixture.CompletionStorageContext,
		Fixture.Admission.Journal,
		Fixture.Admission.Owner,
		Fixture.Admission.OldSurface,
		Fixture.Admission.NewSurface,
		Fixture.Admission.FileSystem,
		Fixture.Authority);
	Fixture.Authority.Completion.SetReadMode(
		FFakeArcOwnerHandoffRecoveryBundleWatermarkAuthority::EReadMode::
			Unavailable);
	const auto Unavailable = Fixture.Execute(Session);
	Fixture.Authority.Completion.SetReadMode(
		FFakeArcOwnerHandoffRecoveryBundleWatermarkAuthority::EReadMode::
			Normal);
	FArcOwnerHandoffRecoveryCompletionSessionResult Nested;
	Fixture.Authority.Completion.SetReadCallback([&]()
	{
		Nested = Fixture.Execute(Session);
	});
	const auto Outer = Fixture.Execute(Session);

	TestTrue(TEXT("invalid input rejects before all external callbacks"),
		Invalid.IsValid()
			&& Invalid.GetStatus()
				== EArcOwnerHandoffRecoveryCompletionSessionStatus::RequestRejected);
	TestTrue(TEXT("completion authority outage closes before pending admission"),
		Unavailable.IsValid()
			&& Unavailable.GetStatus()
				== EArcOwnerHandoffRecoveryCompletionSessionStatus::
					CompletionAuthorityUnavailable
			&& !Unavailable.DidInvokeAdmission());
	TestTrue(TEXT("completion callback cannot re-enter the same Session"),
		Nested.IsValid()
			&& Nested.GetStatus()
				== EArcOwnerHandoffRecoveryCompletionSessionStatus::
					OperationInProgress
			&& !Nested.DidInvokeAdmission()
			&& Outer.IsValid() && Outer.IsSuccess()
			&& Fixture.Authority.Completion.ReadCount == 2
			&& Fixture.Authority.Pending.ReadCount == 1
			&& !Session.IsOperationInProgress());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryCompletionFailureTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCompletionSession.PersistenceAndAdvanceFailureFences",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryCompletionFailureTest::
RunTest(const FString&)
{
	FArcOwnerHandoffRecoveryCompletionFixture SaveFixture;
	if (!SaveFixture.Build(*this, TEXT("RecoveryCompletionSaveFailure")))
	{
		return false;
	}
	SaveFixture.Admission.FileSystem.SetFailure(
		FFakeArcOwnerHandoffRecoveryPayloadStorageFileSystem::EFailure::
			OpenTemporary);
	FArcOwnerHandoffRecoveryCompletionSession SaveSession;
	const auto SaveFailed = SaveFixture.Execute(SaveSession);
	SaveFixture.Admission.FileSystem.SetFailure(
		FFakeArcOwnerHandoffRecoveryPayloadStorageFileSystem::EFailure::None);
	FArcOwnerHandoffRecoveryCompletionSession SaveRepairSession;
	const auto SaveRepaired = SaveFixture.Execute(SaveRepairSession);

	FArcOwnerHandoffRecoveryCompletionFixture AdvanceFixture;
	if (!AdvanceFixture.Build(*this, TEXT("RecoveryCompletionAdvanceFailure")))
	{
		return false;
	}
	AdvanceFixture.Authority.Completion.SetAdvanceMode(
		FFakeArcOwnerHandoffRecoveryBundleWatermarkAuthority::EAdvanceMode::
			Conflict);
	FArcOwnerHandoffRecoveryCompletionSession AdvanceSession;
	const auto AdvanceFailed = AdvanceFixture.Execute(AdvanceSession);

	TestTrue(TEXT("temporary open failure never reaches completion authority"),
		SaveFailed.IsValid() && !SaveFailed.IsSuccess()
			&& SaveFailed.GetStatus()
				== EArcOwnerHandoffRecoveryCompletionSessionStatus::
					CompletionSaveRejected
			&& SaveFailed.GetCompletionSaveStatus()
				== EArcOwnerHandoffRecoveryCompletionStorageSaveStatus::
					TemporaryOpenFailed
			&& SaveFixture.Authority.Completion.AdvanceCount == 1);
	TestTrue(TEXT("a later bounded call repairs save after P20.48 replay"),
		SaveRepaired.IsValid() && SaveRepaired.IsSuccess()
			&& SaveRepaired.GetAdmissionResult().IsReplay()
			&& SaveRepaired.WasCompletionVerified());
	TestTrue(TEXT("compare conflict preserves durable evidence without success"),
		AdvanceFailed.IsValid() && !AdvanceFailed.IsSuccess()
			&& AdvanceFailed.GetStatus()
				== EArcOwnerHandoffRecoveryCompletionSessionStatus::
					CompletionAdvanceConflict
			&& AdvanceFailed.WasCompletionVerified()
			&& !AdvanceFailed.IsCompletionAuthorityCurrent());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryTerminalAdoptionContractTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryTerminalAdoptionSession.RequestContractAndDeterminism",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryTerminalAdoptionContractTest::
RunTest(const FString&)
{
	FArcOwnerHandoffRecoveryTerminalAdoptionFixture Fixture;
	if (!Fixture.Build(*this, TEXT("RecoveryTerminalAdoptionContract")))
	{
		return false;
	}
	FArcOwnerHandoffRecoveryTerminalAdoptionRequest Duplicate;
	FArcOwnerHandoffRecoveryTerminalAdoptionRequest Different;
	FArcOwnerHandoffRecoveryTerminalAdoptionRequest Invalid;
	FString Diagnostic;
	const bool bDuplicate =
		FArcOwnerHandoffRecoveryTerminalAdoptionRequest::TryCreate(
			Fixture.AdoptionAuthorityDomainId,
			Fixture.Completion.Request,
			Duplicate,
			Diagnostic);
	const bool bDifferent =
		FArcOwnerHandoffRecoveryTerminalAdoptionRequest::TryCreate(
			FGuid(0xF4571001, 0xF4571002, 0xF4571003, 0xF4571004),
			Fixture.Completion.Request,
			Different,
			Diagnostic);
	const bool bRejected =
		FArcOwnerHandoffRecoveryTerminalAdoptionRequest::TryCreate(
			Fixture.Completion.CompletionAuthorityDomainId,
			Fixture.Completion.Request,
			Invalid,
			Diagnostic);
	FArcOwnerHandoffRecoveryTerminalAdoption Adoption;
	FArcOwnerHandoffRecoveryTerminalAdoption DuplicateAdoption;
	const bool bAdoption = FArcOwnerHandoffRecoveryTerminalAdoption::TryCreate(
		Fixture.Request,
		Fixture.CompletionResult.GetCompletion(),
		Adoption);
	const bool bDuplicateAdoption =
		FArcOwnerHandoffRecoveryTerminalAdoption::TryCreate(
			Duplicate,
			Fixture.CompletionResult.GetCompletion(),
			DuplicateAdoption);

	TestTrue(TEXT("same terminal adoption intent derives one stable request"),
		bDuplicate && Duplicate.IsValid()
			&& Duplicate.GetRequestId() == Fixture.Request.GetRequestId());
	TestTrue(TEXT("authority domain is part of terminal adoption identity"),
		bDifferent && Different.IsValid()
			&& Different.GetRequestId() != Fixture.Request.GetRequestId());
	TestTrue(TEXT("adoption authority cannot alias completion authority"),
		!bRejected && !Invalid.IsValid());
	TestTrue(TEXT("trusted completion derives stable adoption evidence"),
		bAdoption && bDuplicateAdoption && Adoption.IsValid()
			&& Adoption.MatchesRequest(Fixture.Request)
			&& Adoption.MatchesCompletion(
				Fixture.CompletionResult.GetCompletion())
			&& Adoption.GetAdoptionId()
				== DuplicateAdoption.GetAdoptionId()
			&& Adoption.GetGeneration() == 1
			&& Adoption.GetNextGeneration() == 2
			&& Adoption.HasNextGenerationCapacity());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryTerminalAdoptionExactTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryTerminalAdoptionSession.ExactTrustedAdoption",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryTerminalAdoptionExactTest::
RunTest(const FString&)
{
	FArcOwnerHandoffRecoveryTerminalAdoptionFixture Fixture;
	if (!Fixture.Build(*this, TEXT("RecoveryTerminalAdoptionExact")))
	{
		return false;
	}
	const int32 CompletionReads =
		Fixture.Completion.Authority.Completion.ReadCount;
	const int32 FileReads = Fixture.Completion.Admission.FileSystem.ReadCount;
	const int32 FileWrites = Fixture.Completion.Admission.FileSystem.WriteCount;
	const FGuid SourceJournalId =
		Fixture.Completion.Admission.Journal.GetJournalId();
	const int32 SourceRecordCount =
		Fixture.Completion.Admission.Journal.GetRecordCount();
	FArcOwnerHandoffRecoveryTerminalAdoptionSession Session;
	const auto Result = Fixture.Execute(Session);

	TestTrue(TEXT("exact trusted completion advances caller adoption authority"),
		Result.IsValid() && Result.IsSuccess() && !Result.IsReplay()
			&& Result.GetStatus()
				== EArcOwnerHandoffRecoveryTerminalAdoptionSessionStatus::
					Adopted
			&& Result.IsCompletionTrusted()
			&& Result.IsAdoptionAuthorityCurrent());
	TestTrue(TEXT("adoption exposes the exact terminal journal and next boundary"),
		Result.GetTerminalJournal().GetJournalId()
				== Fixture.CompletionResult.GetTerminalJournal().GetJournalId()
			&& Result.GetAdoption().MatchesCompletion(
				Fixture.CompletionResult.GetCompletion())
			&& Result.GetTargetGeneration() == 1
			&& Result.GetPreviousAdoptionGeneration() == 0
			&& Result.HasNextGenerationCapacity()
			&& Result.GetAdoption().GetNextGeneration() == 2);
	TestTrue(TEXT("adoption performs bounded reads and one authority advance"),
		Fixture.Completion.Authority.Completion.ReadCount
				== CompletionReads + 1
			&& Fixture.AdoptionAuthority.ReadCount == 1
			&& Fixture.AdoptionAuthority.AdvanceCount == 1
			&& Fixture.Completion.Admission.FileSystem.ReadCount
				== FileReads + 1
			&& Fixture.Completion.Admission.FileSystem.WriteCount
				== FileWrites);
	TestTrue(TEXT("caller source journal and completion files remain unchanged"),
		Fixture.Completion.Admission.Journal.GetJournalId()
				== SourceJournalId
			&& Fixture.Completion.Admission.Journal.GetRecordCount()
				== SourceRecordCount
			&& !Session.IsOperationInProgress());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryTerminalAdoptionReplayTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryTerminalAdoptionSession.TrustedReplay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryTerminalAdoptionReplayTest::
RunTest(const FString&)
{
	FArcOwnerHandoffRecoveryTerminalAdoptionFixture Fixture;
	if (!Fixture.Build(*this, TEXT("RecoveryTerminalAdoptionReplay")))
	{
		return false;
	}
	FArcOwnerHandoffRecoveryTerminalAdoptionSession FirstSession;
	const auto First = Fixture.Execute(FirstSession);
	const int32 AdvanceCount = Fixture.AdoptionAuthority.AdvanceCount;
	const int32 CompletionReads =
		Fixture.Completion.Authority.Completion.ReadCount;
	const int32 FileReads = Fixture.Completion.Admission.FileSystem.ReadCount;
	FArcOwnerHandoffRecoveryTerminalAdoptionSession ReplaySession;
	const auto Replay = Fixture.Execute(
		ReplaySession, First.GetTerminalJournal());

	TestTrue(TEXT("trusted terminal journal replay is idempotent"),
		First.IsSuccess() && Replay.IsValid() && Replay.IsSuccess()
			&& Replay.IsReplay()
			&& Replay.GetStatus()
				== EArcOwnerHandoffRecoveryTerminalAdoptionSessionStatus::
					Replayed
			&& Replay.GetAdoption().GetAdoptionId()
				== First.GetAdoption().GetAdoptionId());
	TestTrue(TEXT("replay verifies completion but skips compare-and-advance"),
		Fixture.AdoptionAuthority.AdvanceCount == AdvanceCount
			&& Fixture.AdoptionAuthority.ReadCount == 2
			&& Fixture.Completion.Authority.Completion.ReadCount
				== CompletionReads + 1
			&& Fixture.Completion.Admission.FileSystem.ReadCount
				== FileReads + 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryTerminalAdoptionCompletionFenceTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryTerminalAdoptionSession.CompletionTrustFences",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryTerminalAdoptionCompletionFenceTest::
RunTest(const FString&)
{
	FArcOwnerHandoffRecoveryTerminalAdoptionFixture MissingFixture;
	if (!MissingFixture.Build(*this, TEXT("RecoveryTerminalAdoptionMissing")))
	{
		return false;
	}
	MissingFixture.Completion.Authority.Completion.ClearState();
	FArcOwnerHandoffRecoveryTerminalAdoptionSession MissingSession;
	const auto Missing = MissingFixture.Execute(MissingSession);

	FArcOwnerHandoffRecoveryTerminalAdoptionFixture CorruptFixture;
	if (!CorruptFixture.Build(*this, TEXT("RecoveryTerminalAdoptionCorrupt")))
	{
		return false;
	}
	TArray<uint8> CorruptBytes;
	if (!CorruptFixture.Completion.Admission.FileSystem.TryGetFile(
			CorruptFixture.Completion.CompletionStorageContext.GetPrimaryPath(),
			CorruptBytes)
		|| CorruptBytes.IsEmpty())
	{
		AddError(TEXT("Could not read completion bytes for tamper test."));
		return false;
	}
	CorruptBytes[0] ^= 0x01;
	CorruptFixture.Completion.Admission.FileSystem.SetFile(
		CorruptFixture.Completion.CompletionStorageContext.GetPrimaryPath(),
		CorruptBytes);
	FArcOwnerHandoffRecoveryTerminalAdoptionSession CorruptSession;
	const auto Corrupt = CorruptFixture.Execute(CorruptSession);

	TestTrue(TEXT("ordinary completion file without authority is not trusted"),
		Missing.IsValid() && !Missing.IsSuccess()
			&& Missing.GetStatus()
				== EArcOwnerHandoffRecoveryTerminalAdoptionSessionStatus::
					CompletionAuthorityMissing
			&& MissingFixture.AdoptionAuthority.ReadCount == 0
			&& MissingFixture.AdoptionAuthority.AdvanceCount == 0);
	TestTrue(TEXT("tampered trusted completion fails before adoption authority"),
		Corrupt.IsValid() && !Corrupt.IsSuccess()
			&& Corrupt.GetStatus()
				== EArcOwnerHandoffRecoveryTerminalAdoptionSessionStatus::
					TrustedCompletionLoadRejected
			&& CorruptFixture.AdoptionAuthority.ReadCount == 0
			&& CorruptFixture.AdoptionAuthority.AdvanceCount == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryTerminalAdoptionAuthorityFenceTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryTerminalAdoptionSession.AdoptionAuthorityFences",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryTerminalAdoptionAuthorityFenceTest::
RunTest(const FString&)
{
	FArcOwnerHandoffRecoveryTerminalAdoptionFixture ConflictFixture;
	if (!ConflictFixture.Build(*this, TEXT("RecoveryTerminalAdoptionConflict")))
	{
		return false;
	}
	FArcOwnerHandoffRecoveryBundleWatermarkAdvanceRequest ConflictRequest;
	FArcOwnerHandoffRecoveryBundleWatermarkAdvanceReceipt ConflictReceipt;
	FArcOwnerHandoffRecoveryBundleWatermarkState ConflictState;
	if (!BuildArcOwnerHandoffRecoveryWatermarkState(
			*this,
			ConflictFixture.AdoptionAuthorityDomainId,
			ConflictFixture.Completion.Admission.LineageId,
			0,
			1,
			FGuid(0xF457C001, 0xF457C002, 0xF457C003, 0xF457C004),
			ConflictRequest,
			ConflictReceipt,
			ConflictState))
	{
		return false;
	}
	ConflictFixture.AdoptionAuthority.SetState(ConflictState);
	FArcOwnerHandoffRecoveryTerminalAdoptionSession ConflictSession;
	const auto Conflict = ConflictFixture.Execute(ConflictSession);

	FArcOwnerHandoffRecoveryTerminalAdoptionFixture AheadFixture;
	if (!AheadFixture.Build(*this, TEXT("RecoveryTerminalAdoptionAhead")))
	{
		return false;
	}
	FArcOwnerHandoffRecoveryBundleWatermarkAdvanceRequest AheadRequest;
	FArcOwnerHandoffRecoveryBundleWatermarkAdvanceReceipt AheadReceipt;
	FArcOwnerHandoffRecoveryBundleWatermarkState AheadState;
	if (!BuildArcOwnerHandoffRecoveryWatermarkState(
			*this,
			AheadFixture.AdoptionAuthorityDomainId,
			AheadFixture.Completion.Admission.LineageId,
			0,
			2,
			FGuid(0xF457A001, 0xF457A002, 0xF457A003, 0xF457A004),
			AheadRequest,
			AheadReceipt,
			AheadState))
	{
		return false;
	}
	AheadFixture.AdoptionAuthority.SetState(AheadState);
	FArcOwnerHandoffRecoveryTerminalAdoptionSession AheadSession;
	const auto Ahead = AheadFixture.Execute(AheadSession);

	TestTrue(TEXT("same-generation foreign adoption evidence conflicts"),
		Conflict.IsValid() && !Conflict.IsSuccess()
			&& Conflict.GetStatus()
				== EArcOwnerHandoffRecoveryTerminalAdoptionSessionStatus::
					AdoptionAuthorityConflict
			&& ConflictFixture.AdoptionAuthority.AdvanceCount == 0);
	TestTrue(TEXT("caller adoption authority cannot roll back"),
		Ahead.IsValid() && !Ahead.IsSuccess()
			&& Ahead.GetStatus()
				== EArcOwnerHandoffRecoveryTerminalAdoptionSessionStatus::
					AdoptionAuthorityAhead
			&& AheadFixture.AdoptionAuthority.AdvanceCount == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryTerminalAdoptionUnknownTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryTerminalAdoptionSession.UnknownOutcomeClosure",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryTerminalAdoptionUnknownTest::
RunTest(const FString&)
{
	FArcOwnerHandoffRecoveryTerminalAdoptionFixture CommittedFixture;
	if (!CommittedFixture.Build(
			*this, TEXT("RecoveryTerminalAdoptionUnknownCommitted")))
	{
		return false;
	}
	CommittedFixture.AdoptionAuthority.SetAdvanceMode(
		FFakeArcOwnerHandoffRecoveryBundleWatermarkAuthority::EAdvanceMode::
			OutcomeUnknownAfterCommit);
	FArcOwnerHandoffRecoveryTerminalAdoptionSession CommittedSession;
	const auto Committed = CommittedFixture.Execute(CommittedSession);

	FArcOwnerHandoffRecoveryTerminalAdoptionFixture UnresolvedFixture;
	if (!UnresolvedFixture.Build(
			*this, TEXT("RecoveryTerminalAdoptionUnknownBefore")))
	{
		return false;
	}
	UnresolvedFixture.AdoptionAuthority.SetAdvanceMode(
		FFakeArcOwnerHandoffRecoveryBundleWatermarkAuthority::EAdvanceMode::
			OutcomeUnknownBeforeCommit);
	FArcOwnerHandoffRecoveryTerminalAdoptionSession UnresolvedSession;
	const auto Unresolved = UnresolvedFixture.Execute(UnresolvedSession);

	TestTrue(TEXT("unknown response after commit closes with one exact re-read"),
		Committed.IsValid() && Committed.IsSuccess()
			&& Committed.GetStatus()
				== EArcOwnerHandoffRecoveryTerminalAdoptionSessionStatus::
					AdoptedAfterAuthorityRecheck
			&& CommittedFixture.AdoptionAuthority.ReadCount == 2
			&& CommittedFixture.AdoptionAuthority.AdvanceCount == 1);
	TestTrue(TEXT("unknown response before commit remains unresolved"),
		Unresolved.IsValid() && !Unresolved.IsSuccess()
			&& Unresolved.GetStatus()
				== EArcOwnerHandoffRecoveryTerminalAdoptionSessionStatus::
					AdoptionOutcomeUnresolved
			&& UnresolvedFixture.AdoptionAuthority.ReadCount == 2
			&& UnresolvedFixture.AdoptionAuthority.AdvanceCount == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryTerminalAdoptionFailureTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryTerminalAdoptionSession.AuthorityFailureFences",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryTerminalAdoptionFailureTest::
RunTest(const FString&)
{
	FArcOwnerHandoffRecoveryTerminalAdoptionFixture CompletionFixture;
	if (!CompletionFixture.Build(
			*this, TEXT("RecoveryTerminalAdoptionCompletionUnavailable")))
	{
		return false;
	}
	CompletionFixture.Completion.Authority.Completion.SetReadMode(
		FFakeArcOwnerHandoffRecoveryBundleWatermarkAuthority::EReadMode::
			Unavailable);
	FArcOwnerHandoffRecoveryTerminalAdoptionSession CompletionSession;
	const auto CompletionUnavailable =
		CompletionFixture.Execute(CompletionSession);

	FArcOwnerHandoffRecoveryTerminalAdoptionFixture AdoptionFixture;
	if (!AdoptionFixture.Build(
			*this, TEXT("RecoveryTerminalAdoptionAuthorityUnavailable")))
	{
		return false;
	}
	AdoptionFixture.AdoptionAuthority.SetReadMode(
		FFakeArcOwnerHandoffRecoveryBundleWatermarkAuthority::EReadMode::
			Unavailable);
	FArcOwnerHandoffRecoveryTerminalAdoptionSession AdoptionSession;
	const auto AdoptionUnavailable = AdoptionFixture.Execute(AdoptionSession);

	FArcOwnerHandoffRecoveryTerminalAdoptionFixture AdvanceFixture;
	if (!AdvanceFixture.Build(
			*this, TEXT("RecoveryTerminalAdoptionAdvanceConflict")))
	{
		return false;
	}
	AdvanceFixture.AdoptionAuthority.SetAdvanceMode(
		FFakeArcOwnerHandoffRecoveryBundleWatermarkAuthority::EAdvanceMode::
			Conflict);
	FArcOwnerHandoffRecoveryTerminalAdoptionSession AdvanceSession;
	const auto AdvanceConflict = AdvanceFixture.Execute(AdvanceSession);

	TestTrue(TEXT("completion authority outage closes before file access"),
		CompletionUnavailable.IsValid()
			&& !CompletionUnavailable.IsSuccess()
			&& CompletionUnavailable.GetStatus()
				== EArcOwnerHandoffRecoveryTerminalAdoptionSessionStatus::
					CompletionAuthorityUnavailable
			&& CompletionFixture.AdoptionAuthority.ReadCount == 0);
	TestTrue(TEXT("adoption authority outage preserves trusted evidence"),
		AdoptionUnavailable.IsValid() && !AdoptionUnavailable.IsSuccess()
			&& AdoptionUnavailable.GetStatus()
				== EArcOwnerHandoffRecoveryTerminalAdoptionSessionStatus::
					AdoptionAuthorityUnavailable
			&& AdoptionUnavailable.IsCompletionTrusted()
			&& AdoptionFixture.AdoptionAuthority.AdvanceCount == 0);
	TestTrue(TEXT("adoption CAS conflict never reports caller authority current"),
		AdvanceConflict.IsValid() && !AdvanceConflict.IsSuccess()
			&& AdvanceConflict.GetStatus()
				== EArcOwnerHandoffRecoveryTerminalAdoptionSessionStatus::
					AdoptionAdvanceConflict
			&& !AdvanceConflict.IsAdoptionAuthorityCurrent());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryTerminalAdoptionInputTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryTerminalAdoptionSession.InputAndReentryFences",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryTerminalAdoptionInputTest::
RunTest(const FString&)
{
	FArcOwnerHandoffRecoveryTerminalAdoptionFixture Fixture;
	if (!Fixture.Build(*this, TEXT("RecoveryTerminalAdoptionInput")))
	{
		return false;
	}
	FArcOwnerHandoffRecoveryTerminalAdoptionSession Session;
	const auto Invalid = Session.ExecuteExplicit(
		FArcOwnerHandoffRecoveryTerminalAdoptionRequest(),
		Fixture.Completion.CompletionStorageContext,
		Fixture.Completion.Admission.Journal,
		Fixture.Completion.Admission.FileSystem,
		Fixture.Completion.Authority,
		Fixture.AdoptionAuthority);
	FArcOwnerHandoffRecoveryCompletionStorageContext WrongContext;
	FString Diagnostic;
	const FString WrongRoot = FPaths::ConvertRelativePathToFull(FPaths::Combine(
		FPaths::ProjectSavedDir(),
		TEXT("Automation"),
		TEXT("Dev.D.UE.0.0.10.P20.57.r0"),
		TEXT("WrongLineage"),
		FGuid::NewGuid().ToString(EGuidFormats::Digits)));
	if (!FArcOwnerHandoffRecoveryCompletionStorageContext::TryCreate(
			WrongRoot,
			FGuid(0xF457E001, 0xF457E002, 0xF457E003, 0xF457E004),
			WrongContext,
			Diagnostic))
	{
		AddError(Diagnostic);
		return false;
	}
	const auto WrongLineage = Session.ExecuteExplicit(
		Fixture.Request,
		WrongContext,
		Fixture.Completion.Admission.Journal,
		Fixture.Completion.Admission.FileSystem,
		Fixture.Completion.Authority,
		Fixture.AdoptionAuthority);

	FArcOwnerHandoffRecoveryTerminalAdoptionSessionResult Nested;
	Fixture.AdoptionAuthority.SetReadCallback([&]()
	{
		Nested = Fixture.Execute(Session);
	});
	const auto Outer = Fixture.Execute(Session);

	TestTrue(TEXT("invalid request rejects before authority access"),
		Invalid.IsValid() && !Invalid.IsSuccess()
			&& Invalid.GetStatus()
				== EArcOwnerHandoffRecoveryTerminalAdoptionSessionStatus::
					RequestRejected);
	TestTrue(TEXT("cross-lineage storage rejects before authority access"),
		WrongLineage.IsValid() && !WrongLineage.IsSuccess()
			&& WrongLineage.GetStatus()
				== EArcOwnerHandoffRecoveryTerminalAdoptionSessionStatus::
					RequestRejected);
	TestTrue(TEXT("authority callback cannot re-enter the same Session"),
		Nested.IsValid() && !Nested.IsSuccess()
			&& Nested.GetStatus()
				== EArcOwnerHandoffRecoveryTerminalAdoptionSessionStatus::
					OperationInProgress
			&& Outer.IsValid() && Outer.IsSuccess()
			&& Fixture.AdoptionAuthority.AdvanceCount == 1
			&& !Session.IsOperationInProgress());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryNextGenerationRotationContractTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryNextGenerationRotationSession.RequestContractAndDeterminism",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryNextGenerationRotationContractTest::
RunTest(const FString&)
{
	FArcOwnerHandoffRecoveryNextGenerationRotationFixture Fixture;
	if (!Fixture.Build(*this, TEXT("RecoveryNextGenerationRotationContract")))
	{
		return false;
	}
	FArcOwnerHandoffRecoveryNextGenerationRotationRequest Duplicate;
	FArcOwnerHandoffRecoveryNextGenerationRotationRequest Reused;
	FString Diagnostic;
	const bool bDuplicate =
		FArcOwnerHandoffRecoveryNextGenerationRotationRequest::TryCreate(
			Fixture.AdoptionResult.GetAdoption(),
			Fixture.NewCheckpoint,
			Duplicate,
			Diagnostic);
	const bool bReused =
		FArcOwnerHandoffRecoveryNextGenerationRotationRequest::TryCreate(
			Fixture.AdoptionResult.GetAdoption(),
			Fixture.Terminal.Completion.Admission.Checkpoint,
			Reused,
			Diagnostic);

	TestTrue(TEXT("same adopted terminal and checkpoint derive one request id"),
		bDuplicate && Duplicate.IsValid()
			&& Duplicate.GetRequestId() == Fixture.Request.GetRequestId());
	TestTrue(TEXT("source-generation checkpoint identity cannot be reused"),
		!bReused && !Reused.IsValid());
	TestTrue(TEXT("request exposes the exact G to G+1 boundary"),
		Fixture.Request.IsValid()
			&& Fixture.Request.GetAdoption().GetGeneration() == 1
			&& Fixture.Request.GetAdoption().GetNextGeneration() == 2
			&& Fixture.Request.GetNewCheckpoint().GetCheckpointId()
				== Fixture.NewCheckpoint.GetCheckpointId());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryNextGenerationRotationExactTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryNextGenerationRotationSession.ExactTrustedRotation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryNextGenerationRotationExactTest::
RunTest(const FString&)
{
	FArcOwnerHandoffRecoveryNextGenerationRotationFixture Fixture;
	if (!Fixture.Build(*this, TEXT("RecoveryNextGenerationRotationExact")))
	{
		return false;
	}
	TArray<uint8> CompletionBytesBefore;
	if (!Fixture.Terminal.Completion.Admission.FileSystem.TryGetFile(
			Fixture.Terminal.Completion.CompletionStorageContext.GetPrimaryPath(),
			CompletionBytesBefore))
	{
		AddError(TEXT("Could not capture completion evidence before rotation."));
		return false;
	}
	const FGuid SourceJournalId =
		Fixture.CurrentTerminalJournal.GetJournalId();
	const int32 SourceRecordCount =
		Fixture.CurrentTerminalJournal.GetRecordCount();
	const int32 WritesBefore =
		Fixture.Terminal.Completion.Admission.FileSystem.WriteCount;
	const int32 PendingReadsBefore =
		Fixture.Terminal.Completion.Authority.Pending.ReadCount;
	const int32 PendingAdvancesBefore =
		Fixture.Terminal.Completion.Authority.Pending.AdvanceCount;
	const int32 AdoptionAdvancesBefore =
		Fixture.Terminal.AdoptionAuthority.AdvanceCount;
	FArcOwnerHandoffRecoveryNextGenerationRotationSession Session;
	const auto Result = Fixture.Execute(Session);
	TArray<uint8> CompletionBytesAfter;
	const bool bCompletionPreserved =
		Fixture.Terminal.Completion.Admission.FileSystem.TryGetFile(
			Fixture.Terminal.Completion.CompletionStorageContext.GetPrimaryPath(),
			CompletionBytesAfter)
		&& CompletionBytesAfter == CompletionBytesBefore;

	TestTrue(TEXT("exact trusted evidence rotates one pending generation"),
		Result.IsValid() && Result.IsSuccess() && !Result.IsReplay()
			&& Result.GetStatus()
				== EArcOwnerHandoffRecoveryNextGenerationRotationSessionStatus::
					Rotated
			&& Result.WasCompletionTrusted()
			&& Result.WasAdoptionTrusted()
			&& Result.WasPendingAuthorityAccepted());
	TestTrue(TEXT("rotation is the exact terminal prefix plus one checkpoint"),
		Result.GetRotation().IsValid()
			&& Result.GetRotation().GetSourceGeneration() == 1
			&& Result.GetRotation().GetTargetGeneration() == 2
			&& Result.GetRotation().GetSourceTerminalJournal().GetJournalId()
				== SourceJournalId
			&& Result.GetRotation().GetPendingJournal().GetRecordCount()
				== SourceRecordCount + 1
			&& Result.GetRotation().GetPendingJournal().MatchesLatestCheckpoint(
				Fixture.NewCheckpoint)
			&& Result.GetRotation().GetPendingBundle().GetGeneration() == 2);
	TestTrue(TEXT("rotation performs one durable write and one pending CAS"),
		Fixture.Terminal.Completion.Admission.FileSystem.WriteCount
				== WritesBefore + 1
			&& Fixture.Terminal.Completion.Authority.Pending.ReadCount
				== PendingReadsBefore + 2
			&& Fixture.Terminal.Completion.Authority.Pending.AdvanceCount
				== PendingAdvancesBefore + 1);
	TestTrue(TEXT("source terminal completion and adoption authority stay read-only"),
		Fixture.CurrentTerminalJournal.GetJournalId() == SourceJournalId
			&& Fixture.CurrentTerminalJournal.GetRecordCount()
				== SourceRecordCount
			&& bCompletionPreserved
			&& Fixture.Terminal.AdoptionAuthority.AdvanceCount
				== AdoptionAdvancesBefore
			&& !Result.DidMutateSurface()
			&& !Result.DidWriteCompletionEvidence()
			&& !Result.DidAdvanceAdoptionAuthority()
			&& !Session.IsOperationInProgress());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryNextGenerationRotationReplayTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryNextGenerationRotationSession.TrustedReplay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryNextGenerationRotationReplayTest::
RunTest(const FString&)
{
	FArcOwnerHandoffRecoveryNextGenerationRotationFixture Fixture;
	if (!Fixture.Build(*this, TEXT("RecoveryNextGenerationRotationReplay")))
	{
		return false;
	}
	FArcOwnerHandoffRecoveryNextGenerationRotationSession FirstSession;
	const auto First = Fixture.Execute(FirstSession);
	const int32 WritesAfterFirst =
		Fixture.Terminal.Completion.Admission.FileSystem.WriteCount;
	const int32 AdvancesAfterFirst =
		Fixture.Terminal.Completion.Authority.Pending.AdvanceCount;
	FArcOwnerHandoffRecoveryNextGenerationRotationSession ReplaySession;
	const auto Replay = Fixture.Execute(ReplaySession);

	TestTrue(TEXT("exact rotation replay returns the same durable identity"),
		First.IsSuccess() && Replay.IsValid() && Replay.IsSuccess()
			&& Replay.IsReplay()
			&& Replay.GetStatus()
				== EArcOwnerHandoffRecoveryNextGenerationRotationSessionStatus::
					Replayed
			&& Replay.GetRotation().GetRotationId()
				== First.GetRotation().GetRotationId());
	TestTrue(TEXT("replay performs no second write or authority advance"),
		Fixture.Terminal.Completion.Admission.FileSystem.WriteCount
				== WritesAfterFirst
			&& Fixture.Terminal.Completion.Authority.Pending.AdvanceCount
				== AdvancesAfterFirst
			&& Replay.GetBundleCommitResult().GetStatus()
				== EArcOwnerHandoffRecoveryBundleWatermarkCommitStatus::
					AlreadyCommitted);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryNextGenerationRotationCompletionFenceTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryNextGenerationRotationSession.CompletionTrustFences",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryNextGenerationRotationCompletionFenceTest::
RunTest(const FString&)
{
	FArcOwnerHandoffRecoveryNextGenerationRotationFixture MissingFixture;
	if (!MissingFixture.Build(
			*this, TEXT("RecoveryNextGenerationRotationCompletionMissing")))
	{
		return false;
	}
	MissingFixture.Terminal.Completion.Authority.Completion.ClearState();
	const int32 MissingWrites =
		MissingFixture.Terminal.Completion.Admission.FileSystem.WriteCount;
	FArcOwnerHandoffRecoveryNextGenerationRotationSession MissingSession;
	const auto Missing = MissingFixture.Execute(MissingSession);

	FArcOwnerHandoffRecoveryNextGenerationRotationFixture CorruptFixture;
	if (!CorruptFixture.Build(
			*this, TEXT("RecoveryNextGenerationRotationCompletionCorrupt")))
	{
		return false;
	}
	TArray<uint8> CorruptBytes;
	if (!CorruptFixture.Terminal.Completion.Admission.FileSystem.TryGetFile(
			CorruptFixture.Terminal.Completion.CompletionStorageContext.
				GetPrimaryPath(),
			CorruptBytes)
		|| CorruptBytes.IsEmpty())
	{
		AddError(TEXT("Could not read completion bytes for rotation tamper test."));
		return false;
	}
	CorruptBytes[0] ^= 0x01;
	CorruptFixture.Terminal.Completion.Admission.FileSystem.SetFile(
		CorruptFixture.Terminal.Completion.CompletionStorageContext.
			GetPrimaryPath(),
		CorruptBytes);
	const int32 CorruptWrites =
		CorruptFixture.Terminal.Completion.Admission.FileSystem.WriteCount;
	FArcOwnerHandoffRecoveryNextGenerationRotationSession CorruptSession;
	const auto Corrupt = CorruptFixture.Execute(CorruptSession);

	TestTrue(TEXT("ordinary completion file without authority is rejected"),
		Missing.IsValid() && !Missing.IsSuccess()
			&& Missing.GetStatus()
				== EArcOwnerHandoffRecoveryNextGenerationRotationSessionStatus::
					CompletionAuthorityMissing
			&& MissingFixture.Terminal.AdoptionAuthority.ReadCount == 1
			&& MissingFixture.Terminal.Completion.Authority.Pending.ReadCount
				== 1
			&& MissingFixture.Terminal.Completion.Admission.FileSystem.WriteCount
				== MissingWrites);
	TestTrue(TEXT("tampered trusted completion stops before adoption or pending reads"),
		Corrupt.IsValid() && !Corrupt.IsSuccess()
			&& Corrupt.GetStatus()
				== EArcOwnerHandoffRecoveryNextGenerationRotationSessionStatus::
					TrustedCompletionLoadRejected
			&& CorruptFixture.Terminal.AdoptionAuthority.ReadCount == 1
			&& CorruptFixture.Terminal.Completion.Authority.Pending.ReadCount
				== 1
			&& CorruptFixture.Terminal.Completion.Admission.FileSystem.WriteCount
				== CorruptWrites);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryNextGenerationRotationAdoptionFenceTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryNextGenerationRotationSession.AdoptionTrustFences",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryNextGenerationRotationAdoptionFenceTest::
RunTest(const FString&)
{
	FArcOwnerHandoffRecoveryNextGenerationRotationFixture MissingFixture;
	if (!MissingFixture.Build(
			*this, TEXT("RecoveryNextGenerationRotationAdoptionMissing")))
	{
		return false;
	}
	MissingFixture.Terminal.AdoptionAuthority.ClearState();
	const int32 MissingWrites =
		MissingFixture.Terminal.Completion.Admission.FileSystem.WriteCount;
	FArcOwnerHandoffRecoveryNextGenerationRotationSession MissingSession;
	const auto Missing = MissingFixture.Execute(MissingSession);

	FArcOwnerHandoffRecoveryNextGenerationRotationFixture ConflictFixture;
	if (!ConflictFixture.Build(
			*this, TEXT("RecoveryNextGenerationRotationAdoptionConflict")))
	{
		return false;
	}
	FArcOwnerHandoffRecoveryBundleWatermarkAdvanceRequest ConflictRequest;
	FArcOwnerHandoffRecoveryBundleWatermarkAdvanceReceipt ConflictReceipt;
	FArcOwnerHandoffRecoveryBundleWatermarkState ConflictState;
	if (!BuildArcOwnerHandoffRecoveryWatermarkState(
			*this,
			ConflictFixture.Terminal.AdoptionAuthorityDomainId,
			ConflictFixture.Terminal.Completion.Admission.LineageId,
			0,
			1,
			FGuid(0xF458C001, 0xF458C002, 0xF458C003, 0xF458C004),
			ConflictRequest,
			ConflictReceipt,
			ConflictState))
	{
		return false;
	}
	ConflictFixture.Terminal.AdoptionAuthority.SetState(ConflictState);
	const int32 ConflictWrites =
		ConflictFixture.Terminal.Completion.Admission.FileSystem.WriteCount;
	FArcOwnerHandoffRecoveryNextGenerationRotationSession ConflictSession;
	const auto Conflict = ConflictFixture.Execute(ConflictSession);

	TestTrue(TEXT("missing adoption authority rejects the next generation"),
		Missing.IsValid() && !Missing.IsSuccess()
			&& Missing.GetStatus()
				== EArcOwnerHandoffRecoveryNextGenerationRotationSessionStatus::
					AdoptionAuthorityMissing
			&& Missing.WasCompletionTrusted()
			&& MissingFixture.Terminal.Completion.Authority.Pending.ReadCount
				== 1
			&& MissingFixture.Terminal.Completion.Admission.FileSystem.WriteCount
				== MissingWrites);
	TestTrue(TEXT("same-generation foreign adoption evidence conflicts"),
		Conflict.IsValid() && !Conflict.IsSuccess()
			&& Conflict.GetStatus()
				== EArcOwnerHandoffRecoveryNextGenerationRotationSessionStatus::
					AdoptionAuthorityConflict
			&& Conflict.WasCompletionTrusted()
			&& ConflictFixture.Terminal.Completion.Authority.Pending.ReadCount
				== 1
			&& ConflictFixture.Terminal.Completion.Admission.FileSystem.WriteCount
				== ConflictWrites);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryNextGenerationRotationPendingFenceTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryNextGenerationRotationSession.PendingAuthorityFences",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryNextGenerationRotationPendingFenceTest::
RunTest(const FString&)
{
	FArcOwnerHandoffRecoveryNextGenerationRotationFixture ConflictFixture;
	if (!ConflictFixture.Build(
			*this, TEXT("RecoveryNextGenerationRotationPendingConflict")))
	{
		return false;
	}
	FArcOwnerHandoffRecoveryBundleWatermarkAdvanceRequest ConflictRequest;
	FArcOwnerHandoffRecoveryBundleWatermarkAdvanceReceipt ConflictReceipt;
	FArcOwnerHandoffRecoveryBundleWatermarkState ConflictState;
	if (!BuildArcOwnerHandoffRecoveryWatermarkState(
			*this,
			ConflictFixture.Terminal.Completion.Admission.AuthorityDomainId,
			ConflictFixture.Terminal.Completion.Admission.LineageId,
			0,
			1,
			FGuid(0xF458D001, 0xF458D002, 0xF458D003, 0xF458D004),
			ConflictRequest,
			ConflictReceipt,
			ConflictState))
	{
		return false;
	}
	ConflictFixture.Terminal.Completion.Authority.Pending.SetState(
		ConflictState);
	const int32 ConflictWrites =
		ConflictFixture.Terminal.Completion.Admission.FileSystem.WriteCount;
	FArcOwnerHandoffRecoveryNextGenerationRotationSession ConflictSession;
	const auto Conflict = ConflictFixture.Execute(ConflictSession);

	FArcOwnerHandoffRecoveryNextGenerationRotationFixture AheadFixture;
	if (!AheadFixture.Build(
			*this, TEXT("RecoveryNextGenerationRotationPendingAhead")))
	{
		return false;
	}
	FArcOwnerHandoffRecoveryBundleWatermarkAdvanceRequest AheadRequest;
	FArcOwnerHandoffRecoveryBundleWatermarkAdvanceReceipt AheadReceipt;
	FArcOwnerHandoffRecoveryBundleWatermarkState AheadState;
	if (!BuildArcOwnerHandoffRecoveryWatermarkState(
			*this,
			AheadFixture.Terminal.Completion.Admission.AuthorityDomainId,
			AheadFixture.Terminal.Completion.Admission.LineageId,
			2,
			3,
			FGuid(0xF458E001, 0xF458E002, 0xF458E003, 0xF458E004),
			AheadRequest,
			AheadReceipt,
			AheadState))
	{
		return false;
	}
	AheadFixture.Terminal.Completion.Authority.Pending.SetState(AheadState);
	const int32 AheadWrites =
		AheadFixture.Terminal.Completion.Admission.FileSystem.WriteCount;
	FArcOwnerHandoffRecoveryNextGenerationRotationSession AheadSession;
	const auto Ahead = AheadFixture.Execute(AheadSession);

	TestTrue(TEXT("foreign source-generation pending authority conflicts"),
		Conflict.IsValid() && !Conflict.IsSuccess()
			&& Conflict.GetStatus()
				== EArcOwnerHandoffRecoveryNextGenerationRotationSessionStatus::
					PendingAuthorityConflict
			&& Conflict.WasAdoptionTrusted()
			&& ConflictFixture.Terminal.Completion.Admission.FileSystem.WriteCount
				== ConflictWrites
			&& ConflictFixture.Terminal.Completion.Authority.Pending.AdvanceCount
				== 0);
	TestTrue(TEXT("pending authority ahead of G+1 cannot roll back"),
		Ahead.IsValid() && !Ahead.IsSuccess()
			&& Ahead.GetStatus()
				== EArcOwnerHandoffRecoveryNextGenerationRotationSessionStatus::
					PendingAuthorityAhead
			&& AheadFixture.Terminal.Completion.Admission.FileSystem.WriteCount
				== AheadWrites
			&& AheadFixture.Terminal.Completion.Authority.Pending.AdvanceCount
				== 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryNextGenerationRotationRepairTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryNextGenerationRotationSession.DurableBundleBeforeWatermarkRepair",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryNextGenerationRotationRepairTest::
RunTest(const FString&)
{
	FArcOwnerHandoffRecoveryNextGenerationRotationFixture Fixture;
	if (!Fixture.Build(*this, TEXT("RecoveryNextGenerationRotationRepair")))
	{
		return false;
	}
	Fixture.Terminal.Completion.Authority.Pending.SetAdvanceMode(
		FFakeArcOwnerHandoffRecoveryBundleWatermarkAuthority::EAdvanceMode::
			Unavailable);
	FArcOwnerHandoffRecoveryNextGenerationRotationSession PendingSession;
	const auto Pending = Fixture.Execute(PendingSession);
	const int32 WritesWhilePending =
		Fixture.Terminal.Completion.Admission.FileSystem.WriteCount;
	const FGuid RotationId = Pending.GetRotation().GetRotationId();
	Fixture.Terminal.Completion.Authority.Pending.SetAdvanceMode(
		FFakeArcOwnerHandoffRecoveryBundleWatermarkAuthority::EAdvanceMode::
			Normal);
	FArcOwnerHandoffRecoveryNextGenerationRotationSession RepairSession;
	const auto Repaired = Fixture.Execute(RepairSession);

	TestTrue(TEXT("bundle remains durable when the first pending CAS is unavailable"),
		Pending.IsValid() && !Pending.IsSuccess()
			&& Pending.GetStatus()
				== EArcOwnerHandoffRecoveryNextGenerationRotationSessionStatus::
					BundleCommittedWatermarkPending
			&& Pending.GetRotation().IsValid()
			&& Pending.GetBundleCommitResult().WasBundleVerified());
	TestTrue(TEXT("later bounded call repairs only the pending watermark"),
		Repaired.IsValid() && Repaired.IsSuccess()
			&& Repaired.GetRotation().GetRotationId() == RotationId
			&& Repaired.GetBundleCommitResult().GetBundleSaveStatus()
				== EArcOwnerHandoffRecoveryBundleStorageSaveStatus::
					AlreadyCurrent
			&& Fixture.Terminal.Completion.Admission.FileSystem.WriteCount
				== WritesWhilePending
			&& Fixture.Terminal.Completion.Authority.Pending.AdvanceCount == 2
			&& Fixture.Terminal.AdoptionAuthority.AdvanceCount == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryNextGenerationRotationUnknownTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryNextGenerationRotationSession.UnknownOutcomeClosure",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryNextGenerationRotationUnknownTest::
RunTest(const FString&)
{
	FArcOwnerHandoffRecoveryNextGenerationRotationFixture CommittedFixture;
	if (!CommittedFixture.Build(
			*this, TEXT("RecoveryNextGenerationRotationUnknownCommitted")))
	{
		return false;
	}
	const int32 CommittedReads =
		CommittedFixture.Terminal.Completion.Authority.Pending.ReadCount;
	CommittedFixture.Terminal.Completion.Authority.Pending.SetAdvanceMode(
		FFakeArcOwnerHandoffRecoveryBundleWatermarkAuthority::EAdvanceMode::
			OutcomeUnknownAfterCommit);
	FArcOwnerHandoffRecoveryNextGenerationRotationSession CommittedSession;
	const auto Committed = CommittedFixture.Execute(CommittedSession);

	FArcOwnerHandoffRecoveryNextGenerationRotationFixture UnresolvedFixture;
	if (!UnresolvedFixture.Build(
			*this, TEXT("RecoveryNextGenerationRotationUnknownBefore")))
	{
		return false;
	}
	UnresolvedFixture.Terminal.Completion.Authority.Pending.SetAdvanceMode(
		FFakeArcOwnerHandoffRecoveryBundleWatermarkAuthority::EAdvanceMode::
			OutcomeUnknownBeforeCommit);
	FArcOwnerHandoffRecoveryNextGenerationRotationSession UnresolvedSession;
	const auto Unresolved = UnresolvedFixture.Execute(UnresolvedSession);
	const int32 WritesAfterUnresolved =
		UnresolvedFixture.Terminal.Completion.Admission.FileSystem.WriteCount;
	const FGuid UnresolvedRotationId =
		Unresolved.GetRotation().GetRotationId();
	UnresolvedFixture.Terminal.Completion.Authority.Pending.SetAdvanceMode(
		FFakeArcOwnerHandoffRecoveryBundleWatermarkAuthority::EAdvanceMode::
			Normal);
	FArcOwnerHandoffRecoveryNextGenerationRotationSession RetrySession;
	const auto Retried = UnresolvedFixture.Execute(RetrySession);

	TestTrue(TEXT("unknown response after commit closes with one exact reread"),
		Committed.IsValid() && Committed.IsSuccess()
			&& Committed.GetStatus()
				== EArcOwnerHandoffRecoveryNextGenerationRotationSessionStatus::
					Rotated
			&& Committed.GetBundleCommitResult().GetStatus()
				== EArcOwnerHandoffRecoveryBundleWatermarkCommitStatus::
					CommittedAfterAuthorityRecheck
			&& CommittedFixture.Terminal.Completion.Authority.Pending.ReadCount
				== CommittedReads + 3);
	TestTrue(TEXT("unknown response before commit remains bounded and retryable"),
		Unresolved.IsValid() && !Unresolved.IsSuccess()
			&& Unresolved.GetStatus()
				== EArcOwnerHandoffRecoveryNextGenerationRotationSessionStatus::
					PendingWatermarkOutcomeUnresolved
			&& Retried.IsValid() && Retried.IsSuccess()
			&& Retried.GetRotation().GetRotationId() == UnresolvedRotationId
			&& Retried.GetBundleCommitResult().GetBundleSaveStatus()
				== EArcOwnerHandoffRecoveryBundleStorageSaveStatus::
					AlreadyCurrent
			&& UnresolvedFixture.Terminal.Completion.Admission.FileSystem.
				WriteCount == WritesAfterUnresolved);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryNextGenerationRotationInputTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryNextGenerationRotationSession.InputAndReentryFences",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryNextGenerationRotationInputTest::
RunTest(const FString&)
{
	FArcOwnerHandoffRecoveryNextGenerationRotationFixture Fixture;
	if (!Fixture.Build(*this, TEXT("RecoveryNextGenerationRotationInput")))
	{
		return false;
	}
	FArcOwnerHandoffRecoveryNextGenerationRotationSession Session;
	const auto Invalid = Session.ExecuteExplicit(
		FArcOwnerHandoffRecoveryNextGenerationRotationRequest(),
		Fixture.Terminal.Completion.CompletionStorageContext,
		Fixture.Terminal.Completion.Admission.StorageContext,
		Fixture.CurrentTerminalJournal,
		Fixture.Terminal.Completion.Admission.FileSystem,
		Fixture.Terminal.Completion.Authority,
		Fixture.Terminal.AdoptionAuthority,
		Fixture.Terminal.Completion.Authority);
	const auto NonTerminal = Fixture.Execute(
		Session, Fixture.Terminal.Completion.Admission.Journal);

	FArcOwnerHandoffRecoveryNextGenerationRotationSessionResult Nested;
	Fixture.Terminal.AdoptionAuthority.SetReadCallback([&]()
	{
		Nested = Fixture.Execute(Session);
	});
	const auto Outer = Fixture.Execute(Session);

	TestTrue(TEXT("invalid request rejects before every callback"),
		Invalid.IsValid() && !Invalid.IsSuccess()
			&& Invalid.GetStatus()
				== EArcOwnerHandoffRecoveryNextGenerationRotationSessionStatus::
					RequestRejected);
	TestTrue(TEXT("pending source journal cannot masquerade as adopted terminal"),
		NonTerminal.IsValid() && !NonTerminal.IsSuccess()
			&& NonTerminal.GetStatus()
				== EArcOwnerHandoffRecoveryNextGenerationRotationSessionStatus::
					CurrentJournalRejected);
	TestTrue(TEXT("authority callback cannot re-enter the same rotation Session"),
		Nested.IsValid() && !Nested.IsSuccess()
			&& Nested.GetStatus()
				== EArcOwnerHandoffRecoveryNextGenerationRotationSessionStatus::
					OperationInProgress
			&& Outer.IsValid() && Outer.IsSuccess()
			&& !Session.IsOperationInProgress());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryMultiGenerationCycleExactTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryMultiGenerationCycleProof.ExactG1ToG3Cycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryMultiGenerationCycleExactTest::
RunTest(const FString&)
{
	FArcOwnerHandoffRecoveryMultiGenerationHarness Harness;
	FArcOwnerHandoffRecoveryGenerationExecution GenerationTwo;
	FArcOwnerHandoffRecoveryGenerationExecution GenerationThree;
	if (!Harness.Build(*this, TEXT("RecoveryMultiGenerationExact"))
		|| !Harness.ExecuteGeneration(
			*this,
			TEXT("RecoveryMultiGenerationExact"),
			false,
			GenerationTwo)
		|| !Harness.ExecuteGeneration(
			*this,
			TEXT("RecoveryMultiGenerationExact"),
			false,
			GenerationThree))
	{
		return false;
	}

	FArcOwnerHandoffRecoveryMultiGenerationCycleProof Proof;
	FString Diagnostic;
	const bool bCreated =
		FArcOwnerHandoffRecoveryMultiGenerationCycleProof::TryCreate(
			GenerationTwo.RotationResult.GetRotation(),
			GenerationTwo.CompletionResult.GetCompletion(),
			GenerationTwo.AdoptionResult.GetAdoption(),
			GenerationThree.RotationResult.GetRotation(),
			Proof,
			Diagnostic);

	TestTrue(TEXT("two adjacent rotations produce one valid G1 to G3 proof"),
		bCreated && Proof.IsValid() && Diagnostic.Len() > 0
			&& Proof.GetInitialGeneration() == 1
			&& Proof.GetIntermediateGeneration() == 2
			&& Proof.GetFinalGeneration() == 3);
	TestTrue(TEXT("proof preserves the exact terminal and pending prefixes"),
		Proof.GetFirstRotation().GetSourceTerminalJournal().GetRecordCount()
				== 2
			&& Proof.GetFirstRotation().GetPendingJournal().GetRecordCount()
				== 3
			&& Proof.GetIntermediateCompletion().GetTerminalJournal().
				GetRecordCount() == 4
			&& Proof.GetSecondRotation().GetPendingJournal().GetRecordCount()
				== 5);
	TestTrue(TEXT("all three existing authority domains reach generation three"),
		Harness.Seed.Completion.Authority.Pending.GetState().GetGeneration()
				== 3
			&& Harness.Seed.Completion.Authority.Completion.GetState().
				GetGeneration() == 3
			&& Harness.Seed.AdoptionAuthority.GetState().GetGeneration() == 3
			&& Harness.CurrentTerminalJournal.GetRecordCount() == 6);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryMultiGenerationCycleIdentityTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryMultiGenerationCycleProof.DeterministicIdentityAndCrossChainFence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryMultiGenerationCycleIdentityTest::
RunTest(const FString&)
{
	FArcOwnerHandoffRecoveryMultiGenerationHarness FirstHarness;
	FArcOwnerHandoffRecoveryGenerationExecution FirstTwo;
	FArcOwnerHandoffRecoveryGenerationExecution FirstThree;
	FArcOwnerHandoffRecoveryMultiGenerationHarness OtherHarness;
	FArcOwnerHandoffRecoveryGenerationExecution OtherTwo;
	FArcOwnerHandoffRecoveryGenerationExecution OtherThree;
	if (!FirstHarness.Build(*this, TEXT("RecoveryMultiGenerationIdentityA"))
		|| !FirstHarness.ExecuteGeneration(
			*this,
			TEXT("RecoveryMultiGenerationIdentityA"),
			false,
			FirstTwo)
		|| !FirstHarness.ExecuteGeneration(
			*this,
			TEXT("RecoveryMultiGenerationIdentityA"),
			false,
			FirstThree)
		|| !OtherHarness.Build(*this, TEXT("RecoveryMultiGenerationIdentityB"))
		|| !OtherHarness.ExecuteGeneration(
			*this,
			TEXT("RecoveryMultiGenerationIdentityB"),
			false,
			OtherTwo)
		|| !OtherHarness.ExecuteGeneration(
			*this,
			TEXT("RecoveryMultiGenerationIdentityB"),
			false,
			OtherThree))
	{
		return false;
	}

	FArcOwnerHandoffRecoveryMultiGenerationCycleProof FirstProof;
	FArcOwnerHandoffRecoveryMultiGenerationCycleProof RepeatedProof;
	FArcOwnerHandoffRecoveryMultiGenerationCycleProof CrossChainProof;
	FArcOwnerHandoffRecoveryMultiGenerationCycleProof ReorderedProof;
	FString Diagnostic;
	const bool bFirst =
		FArcOwnerHandoffRecoveryMultiGenerationCycleProof::TryCreate(
			FirstTwo.RotationResult.GetRotation(),
			FirstTwo.CompletionResult.GetCompletion(),
			FirstTwo.AdoptionResult.GetAdoption(),
			FirstThree.RotationResult.GetRotation(),
			FirstProof,
			Diagnostic);
	const bool bRepeated =
		FArcOwnerHandoffRecoveryMultiGenerationCycleProof::TryCreate(
			FirstTwo.RotationResult.GetRotation(),
			FirstTwo.CompletionResult.GetCompletion(),
			FirstTwo.AdoptionResult.GetAdoption(),
			FirstThree.RotationResult.GetRotation(),
			RepeatedProof,
			Diagnostic);
	const bool bCrossChain =
		FArcOwnerHandoffRecoveryMultiGenerationCycleProof::TryCreate(
			FirstTwo.RotationResult.GetRotation(),
			FirstTwo.CompletionResult.GetCompletion(),
			FirstTwo.AdoptionResult.GetAdoption(),
			OtherThree.RotationResult.GetRotation(),
			CrossChainProof,
			Diagnostic);
	const bool bReordered =
		FArcOwnerHandoffRecoveryMultiGenerationCycleProof::TryCreate(
			FirstThree.RotationResult.GetRotation(),
			FirstTwo.CompletionResult.GetCompletion(),
			FirstTwo.AdoptionResult.GetAdoption(),
			FirstTwo.RotationResult.GetRotation(),
			ReorderedProof,
			Diagnostic);

	TestTrue(TEXT("same immutable evidence derives the same proof identity"),
		bFirst && bRepeated && FirstProof.Matches(RepeatedProof)
			&& FirstProof.GetProofId() == RepeatedProof.GetProofId());
	TestTrue(TEXT("foreign lineage and reordered evidence fail closed"),
		!bCrossChain && !CrossChainProof.IsValid()
			&& !bReordered && !ReorderedProof.IsValid()
			&& !FArcOwnerHandoffRecoveryMultiGenerationCycleProof().IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryMultiGenerationCycleReplayTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryMultiGenerationCycleProof.ReplayAtEveryBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryMultiGenerationCycleReplayTest::
RunTest(const FString&)
{
	FArcOwnerHandoffRecoveryMultiGenerationHarness Harness;
	FArcOwnerHandoffRecoveryGenerationExecution GenerationTwo;
	FArcOwnerHandoffRecoveryGenerationExecution GenerationThree;
	if (!Harness.Build(*this, TEXT("RecoveryMultiGenerationReplay"))
		|| !Harness.ExecuteGeneration(
			*this,
			TEXT("RecoveryMultiGenerationReplay"),
			true,
			GenerationTwo)
		|| !Harness.ExecuteGeneration(
			*this,
			TEXT("RecoveryMultiGenerationReplay"),
			true,
			GenerationThree))
	{
		return false;
	}

	auto HasExactReplays = [](const auto& Generation)
	{
		return Generation.RotationReplay.IsReplay()
			&& Generation.RotationReplay.GetStatus()
				== EArcOwnerHandoffRecoveryNextGenerationRotationSessionStatus::
					Replayed
			&& Generation.RotationReplay.GetRotation().GetRotationId()
				== Generation.RotationResult.GetRotation().GetRotationId()
			&& Generation.CompletionReplay.IsReplay()
			&& Generation.CompletionReplay.GetStatus()
				== EArcOwnerHandoffRecoveryCompletionSessionStatus::Replayed
			&& Generation.CompletionReplay.GetCompletion().GetCompletionId()
				== Generation.CompletionResult.GetCompletion().GetCompletionId()
			&& Generation.AdoptionReplay.IsReplay()
			&& Generation.AdoptionReplay.GetStatus()
				== EArcOwnerHandoffRecoveryTerminalAdoptionSessionStatus::
					Replayed
			&& Generation.AdoptionReplay.GetAdoption().GetAdoptionId()
				== Generation.AdoptionResult.GetAdoption().GetAdoptionId();
	};
	auto ReplaysAreSideEffectFree = [](const auto& Generation)
	{
		return Generation.bRotationReplaySideEffectFree
			&& Generation.bCompletionReplaySideEffectFree
			&& Generation.bAdoptionReplaySideEffectFree;
	};

	TestTrue(TEXT("generation two replays every durable boundary exactly"),
		HasExactReplays(GenerationTwo)
			&& ReplaysAreSideEffectFree(GenerationTwo));
	TestTrue(TEXT("generation three replays every durable boundary exactly"),
		HasExactReplays(GenerationThree)
			&& ReplaysAreSideEffectFree(GenerationThree));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryMultiGenerationCycleHistoryTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryMultiGenerationCycleProof.HistoricalCheckpointReuseFence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryMultiGenerationCycleHistoryTest::
RunTest(const FString&)
{
	FArcOwnerHandoffRecoveryMultiGenerationHarness Harness;
	FArcOwnerHandoffRecoveryGenerationExecution GenerationTwo;
	if (!Harness.Build(*this, TEXT("RecoveryMultiGenerationHistory")))
	{
		return false;
	}
	const FArcOwnerHandoffRecoveryCheckpoint HistoricalCheckpoint =
		Harness.Seed.Completion.Admission.Checkpoint;
	if (!Harness.ExecuteGeneration(
			*this,
			TEXT("RecoveryMultiGenerationHistory"),
			false,
			GenerationTwo))
	{
		return false;
	}

	FArcOwnerHandoffRecoveryNextGenerationRotationRequest ImmediateRequest;
	FArcOwnerHandoffRecoveryNextGenerationRotationRequest HistoricalRequest;
	FString Diagnostic;
	const bool bImmediateCreated =
		FArcOwnerHandoffRecoveryNextGenerationRotationRequest::TryCreate(
			Harness.CurrentAdoption,
			GenerationTwo.Checkpoint,
			ImmediateRequest,
			Diagnostic);
	const bool bHistoricalCreated =
		FArcOwnerHandoffRecoveryNextGenerationRotationRequest::TryCreate(
			Harness.CurrentAdoption,
			HistoricalCheckpoint,
			HistoricalRequest,
			Diagnostic);

	const int32 WritesBefore =
		Harness.Seed.Completion.Admission.FileSystem.WriteCount;
	const int32 FileReadsBefore =
		Harness.Seed.Completion.Admission.FileSystem.ReadCount;
	const int32 PendingReadsBefore =
		Harness.Seed.Completion.Authority.Pending.ReadCount;
	const int32 CompletionReadsBefore =
		Harness.Seed.Completion.Authority.Completion.ReadCount;
	const int32 AdoptionReadsBefore =
		Harness.Seed.AdoptionAuthority.ReadCount;
	const int32 PendingAdvancesBefore =
		Harness.Seed.Completion.Authority.Pending.AdvanceCount;
	FArcOwnerHandoffRecoveryNextGenerationRotationSession HistoricalSession;
	const auto Historical = HistoricalSession.ExecuteExplicit(
		HistoricalRequest,
		Harness.Seed.Completion.CompletionStorageContext,
		Harness.Seed.Completion.Admission.StorageContext,
		Harness.CurrentTerminalJournal,
		Harness.Seed.Completion.Admission.FileSystem,
		Harness.Seed.Completion.Authority,
		Harness.Seed.AdoptionAuthority,
		Harness.Seed.Completion.Authority);

	TestTrue(TEXT("immediate predecessor checkpoint is rejected by request identity"),
		!bImmediateCreated && !ImmediateRequest.IsValid());
	TestTrue(TEXT("older checkpoint is rejected by full journal history"),
		bHistoricalCreated && Historical.IsValid() && !Historical.IsSuccess()
			&& Historical.GetStatus()
				== EArcOwnerHandoffRecoveryNextGenerationRotationSessionStatus::
					CheckpointHistoryConflict);
	TestTrue(TEXT("history rejection occurs before storage or authority callbacks"),
		Harness.Seed.Completion.Admission.FileSystem.WriteCount == WritesBefore
			&& Harness.Seed.Completion.Admission.FileSystem.ReadCount
				== FileReadsBefore
			&& Harness.Seed.Completion.Authority.Pending.ReadCount
				== PendingReadsBefore
			&& Harness.Seed.Completion.Authority.Completion.ReadCount
				== CompletionReadsBefore
			&& Harness.Seed.AdoptionAuthority.ReadCount
				== AdoptionReadsBefore
			&& Harness.Seed.Completion.Authority.Pending.AdvanceCount
				== PendingAdvancesBefore);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryMultiGenerationCycleCapacityTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryMultiGenerationCycleProof.MaximumGenerationCapacity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewOwnerHandoffRecoveryMultiGenerationCycleCapacityTest::
RunTest(const FString&)
{
	FArcOwnerHandoffRecoveryMultiGenerationHarness Harness;
	if (!Harness.Build(*this, TEXT("RecoveryMultiGenerationCapacity")))
	{
		return false;
	}

	TArray<FArcOwnerHandoffRecoveryMultiGenerationCycleProof> Proofs;
	FArcOwnerHandoffRecoveryGenerationExecution Previous;
	bool bHasPrevious = false;
	for (int32 TargetGeneration = 2;
		TargetGeneration <= FArcOwnerHandoffRecoveryBundle::MaximumGeneration();
		++TargetGeneration)
	{
		FArcOwnerHandoffRecoveryGenerationExecution Current;
		if (!Harness.ExecuteGeneration(
				*this,
				TEXT("RecoveryMultiGenerationCapacity"),
				false,
				Current))
		{
			return false;
		}
		if (bHasPrevious)
		{
			FArcOwnerHandoffRecoveryMultiGenerationCycleProof Proof;
			FString Diagnostic;
			if (!FArcOwnerHandoffRecoveryMultiGenerationCycleProof::TryCreate(
					Previous.RotationResult.GetRotation(),
					Previous.CompletionResult.GetCompletion(),
					Previous.AdoptionResult.GetAdoption(),
					Current.RotationResult.GetRotation(),
					Proof,
					Diagnostic))
			{
				AddError(Diagnostic);
				return false;
			}
			Proofs.Add(Proof);
		}
		Previous = Current;
		bHasPrevious = true;
	}

	const int32 MaximumGeneration =
		FArcOwnerHandoffRecoveryBundle::MaximumGeneration();
	TestTrue(TEXT("all adjacent cycle proofs reach the bounded final generation"),
		Proofs.Num() == MaximumGeneration - 2
			&& Harness.CurrentAdoption.GetGeneration() == MaximumGeneration
			&& !Harness.CurrentAdoption.HasNextGenerationCapacity()
			&& Harness.CurrentTerminalJournal.GetRecordCount()
				== FArcOwnerHandoffRecoveryJournal::MaxRecordCount()
			&& Harness.CurrentTerminalJournal.GetLatestDisposition()
				== EArcOwnerHandoffRecoveryJournalDisposition::
					RecoveryCommitted);
	TestTrue(TEXT("all canonical authorities stop at the same final generation"),
		Harness.Seed.Completion.Authority.Pending.GetState().GetGeneration()
				== MaximumGeneration
			&& Harness.Seed.Completion.Authority.Completion.GetState().
				GetGeneration() == MaximumGeneration
			&& Harness.Seed.AdoptionAuthority.GetState().GetGeneration()
				== MaximumGeneration);

	const FName Consumer(TEXT("Renderer.ArcPreview.MainHUD.r1"));
	FThrownLifecycleFixture OverflowLifecycle;
	FFakeArcPreviewHandoffSurface OverflowOld(
		FGuid(0xF459F001, 0xF459F002, 0xF459F003, 0xF459F004),
		Consumer);
	FFakeArcPreviewHandoffSurface OverflowNew(
		FGuid(0xF459F101, 0xF459F102, 0xF459F103, 0xF459F104),
		Consumer);
	FArcCompositionOwner OverflowOwner;
	FArcOwnerHandoffResult OverflowFailed;
	FArcOwnerHandoffRecoveryCheckpoint OverflowCheckpoint;
	if (!BuildArcOwnerHandoffRecoveryCheckpoint(
			*this,
			TEXT("RecoveryMultiGenerationCapacity.Overflow"),
			OverflowLifecycle,
			OverflowOwner,
			OverflowOld,
			OverflowNew,
			OverflowFailed,
			OverflowCheckpoint))
	{
		return false;
	}

	FArcOwnerHandoffRecoveryJournal OverflowJournal =
		Harness.CurrentTerminalJournal;
	const FGuid JournalIdBefore = OverflowJournal.GetJournalId();
	const auto Capacity = OverflowJournal.AppendCheckpoint(OverflowCheckpoint);
	FArcOwnerHandoffRecoveryNextGenerationRotationRequest OverflowRequest;
	FString Diagnostic;
	const bool bRequestCreated =
		FArcOwnerHandoffRecoveryNextGenerationRotationRequest::TryCreate(
			Harness.CurrentAdoption,
			OverflowCheckpoint,
			OverflowRequest,
			Diagnostic);

	TestTrue(TEXT("journal capacity rejects the seventeenth record without mutation"),
		Capacity.IsValid() && !Capacity.IsSuccess()
			&& Capacity.GetStatus()
				== EArcOwnerHandoffRecoveryJournalAppendStatus::CapacityExceeded
			&& OverflowJournal.GetRecordCount()
				== FArcOwnerHandoffRecoveryJournal::MaxRecordCount()
			&& OverflowJournal.GetJournalId() == JournalIdBefore);
	TestTrue(TEXT("terminal adoption cannot authorize generation nine"),
		!bRequestCreated && !OverflowRequest.IsValid()
			&& Diagnostic.Len() > 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewMainHUDRendererInitializationTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewMainHUDRendererAdapter.InitializationAndPhysicalIdentity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewMainHUDRendererInitializationTest::
RunTest(const FString&)
{
	using FRenderer =
		Fdemo_mapShanmenThrownWeaponArcPreviewMainHUDRendererAdapter;
	const FGuid SurfaceId(
		0xF4606001, 0xF4606002, 0xF4606003, 0xF4606004);
	const FGuid OtherSurfaceId(
		0xF4606101, 0xF4606102, 0xF4606103, 0xF4606104);
	FRenderer Renderer;
	FString Diagnostic;

	TestTrue(TEXT("default MainHUD renderer is explicitly uninitialized"),
		!Renderer.IsInitialized() && !Renderer.IsValid()
			&& !Renderer.GetSurfaceInstanceId().IsValid()
			&& Renderer.GetConsumerDefinitionId().IsNone()
			&& Renderer.GetSurfaceCursor().IsEmpty());
	TestFalse(TEXT("missing physical surface identity fails closed"),
		Renderer.TryInitialize(FGuid(), Diagnostic));
	TestTrue(TEXT("one explicit physical identity initializes an empty surface"),
		Renderer.TryInitialize(SurfaceId, Diagnostic)
			&& Renderer.IsInitialized() && Renderer.IsValid()
			&& Renderer.GetSurfaceInstanceId() == SurfaceId
			&& Renderer.GetConsumerDefinitionId()
				== FRenderer::StableConsumerDefinitionId()
			&& Renderer.GetSurfaceCursor().IsEmpty());
	TestTrue(TEXT("exact initialization replay is idempotent"),
		Renderer.TryInitialize(SurfaceId, Diagnostic)
			&& Renderer.GetSurfaceInstanceId() == SurfaceId);
	TestFalse(TEXT("initialized surface rejects silent identity rotation"),
		Renderer.TryInitialize(OtherSurfaceId, Diagnostic));
	TestTrue(TEXT("rejected identity rotation leaves the surface unchanged"),
		Renderer.IsValid() && Renderer.GetSurfaceInstanceId() == SurfaceId
			&& Renderer.GetSurfaceCursor().IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewMainHUDRendererDeliveryTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewMainHUDRendererAdapter.DeliverySequence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewMainHUDRendererDeliveryTest::
RunTest(const FString&)
{
	using FConsumer =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationConsumerAdapter;
	using FDelivery =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliverySession;
	using FRenderer =
		Fdemo_mapShanmenThrownWeaponArcPreviewMainHUDRendererAdapter;
	FThrownLifecycleFixture Fixture;
	FArcPreviewSurfaceCommandSet Commands;
	if (!BuildArcPreviewSurfaceCommands(
			*this, TEXT("ArcPreviewMainHUDRendererDelivery"),
			Fixture, Commands))
	{
		return false;
	}

	FRenderer Renderer;
	FConsumer Consumer;
	FDelivery Delivery;
	FString Diagnostic;
	check(Renderer.TryInitialize(
		FGuid(0xF4606201, 0xF4606202, 0xF4606203, 0xF4606204),
		Diagnostic));
	check(Consumer.TryBegin(
		Fixture.Correlation.ActiveRunId, Renderer, Diagnostic));
	check(Delivery.TryBegin(
		Fixture.Correlation.ActiveRunId,
		FRenderer::StableConsumerDefinitionId(),
		Diagnostic));

	const auto Show = Delivery.TryDeliver(Commands.Show, Consumer);
	const auto ShowAdapter = Consumer.GetLastResult();
	const FArcSurfaceState ShowCursor = Renderer.GetSurfaceCursor();
	const auto ShowReplay = Delivery.TryDeliver(Commands.Show, Consumer);
	const auto Replace = Delivery.TryDeliver(Commands.Replace, Consumer);
	const auto ReplaceAdapter = Consumer.GetLastResult();
	const FArcSurfaceState ReplaceCursor = Renderer.GetSurfaceCursor();
	const auto NoOp = Delivery.TryDeliver(Commands.NoOp, Consumer);
	const auto NoOpAdapter = Consumer.GetLastResult();
	const FArcSurfaceState NoOpCursor = Renderer.GetSurfaceCursor();
	const auto Hide = Delivery.TryDeliver(Commands.Hide, Consumer);
	const auto HideAdapter = Consumer.GetLastResult();

	TestTrue(TEXT("Show and Replace apply exact renderer-neutral cursors"),
		Show.WasApplied() && ShowAdapter.WasApplied()
			&& ShowAdapter.DidCallSurface()
			&& ShowCursor.Matches(Commands.Show.GetState())
			&& Replace.WasApplied() && ReplaceAdapter.WasApplied()
			&& ReplaceAdapter.DidCallSurface()
			&& ReplaceCursor.Matches(Commands.Replace.GetState()));
	TestTrue(TEXT("exact Show replay is sealed without another surface call"),
		ShowReplay.IsValid() && ShowReplay.WasApplied()
			&& ShowReplay.IsReplay() && !ShowReplay.DidCallPort());
	TestTrue(TEXT("NoOp advances delivery without mutating the MainHUD surface"),
		NoOp.WasApplied() && NoOpAdapter.WasApplied()
			&& NoOpAdapter.IsNoOp() && !NoOpAdapter.DidCallSurface()
			&& NoOpCursor.Matches(Commands.Replace.GetState()));
	TestTrue(TEXT("Hide applies once and normalizes the physical cursor to empty"),
		Hide.WasApplied() && HideAdapter.WasApplied()
			&& HideAdapter.DidCallSurface()
			&& Renderer.GetSurfaceCursor().IsEmpty()
			&& Delivery.GetCursorState().IsHidden()
			&& Delivery.NumAppliedCommands() == 4);
	TestTrue(TEXT("empty renderer and hidden delivery ledger end independently"),
		Consumer.TryEnd(Fixture.Correlation.ActiveRunId, Diagnostic)
			&& Delivery.TryEnd(
				Fixture.Correlation.ActiveRunId, Diagnostic)
			&& Consumer.IsEmpty() && Delivery.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewMainHUDRendererFenceTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewMainHUDRendererAdapter.DirectCommandFences",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewMainHUDRendererFenceTest::
RunTest(const FString&)
{
	using FRenderer =
		Fdemo_mapShanmenThrownWeaponArcPreviewMainHUDRendererAdapter;
	FThrownLifecycleFixture Fixture;
	FArcPreviewSurfaceCommandSet Commands;
	if (!BuildArcPreviewSurfaceCommands(
			*this, TEXT("ArcPreviewMainHUDRendererFences"),
			Fixture, Commands))
	{
		return false;
	}

	FRenderer Renderer;
	const auto NotReady = Renderer.Show(Commands.Show);
	FString Diagnostic;
	check(Renderer.TryInitialize(
		FGuid(0xF4606301, 0xF4606302, 0xF4606303, 0xF4606304),
		Diagnostic));
	const auto ReplaceBeforeShow = Renderer.Replace(Commands.Replace);
	const auto Show = Renderer.Show(Commands.Show);
	const auto WrongEntrypoint = Renderer.Show(Commands.Replace);
	const FArcSurfaceState AfterWrongEntrypoint = Renderer.GetSurfaceCursor();
	const auto Replace = Renderer.Replace(Commands.Replace);
	const auto StaleShow = Renderer.Show(Commands.Show);
	const FArcSurfaceState AfterStaleShow = Renderer.GetSurfaceCursor();
	const auto InvalidNoOp = Renderer.Replace(Commands.NoOp);
	const FArcSurfaceState AfterInvalidNoOp = Renderer.GetSurfaceCursor();
	const auto Hide = Renderer.Hide(Commands.Hide);

	TestTrue(TEXT("uninitialized renderer returns typed rejection without mutation"),
		NotReady.IsValid() && NotReady.IsRejected()
			&& Renderer.IsValid());
	TestTrue(TEXT("Replace before Show fails closed without a cursor"),
		!ReplaceBeforeShow.IsValid());
	TestTrue(TEXT("correct Show applies one exact visible cursor"),
		Show.IsValid() && Show.IsApplied());
	TestTrue(TEXT("wrong entrypoint is rejected with unchanged typed evidence"),
		WrongEntrypoint.IsValid() && WrongEntrypoint.IsRejected()
			&& AfterWrongEntrypoint.Matches(Commands.Show.GetState()));
	TestTrue(TEXT("correct Replace applies while stale Show cannot rewind it"),
		Replace.IsValid() && Replace.IsApplied()
			&& !StaleShow.IsValid()
			&& AfterStaleShow.Matches(Commands.Replace.GetState()));
	TestTrue(TEXT("NoOp cannot enter a mutating renderer endpoint"),
		!InvalidNoOp.IsValid()
			&& AfterStaleShow.Matches(AfterInvalidNoOp));
	TestTrue(TEXT("correct Hide returns the physical surface to empty"),
		Hide.IsValid() && Hide.IsApplied()
			&& Renderer.GetSurfaceCursor().IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewMainHUDRendererCleanupTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewMainHUDRendererAdapter.ExactCleanupPermit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewMainHUDRendererCleanupTest::
RunTest(const FString&)
{
	using FRenderer =
		Fdemo_mapShanmenThrownWeaponArcPreviewMainHUDRendererAdapter;
	FThrownLifecycleFixture Fixture;
	FArcPreviewSurfaceCommandSet Commands;
	if (!BuildArcPreviewSurfaceCommands(
			*this, TEXT("ArcPreviewMainHUDRendererCleanup"),
			Fixture, Commands))
	{
		return false;
	}

	FRenderer Renderer;
	FString Diagnostic;
	check(Renderer.TryInitialize(
		FGuid(0xF4606401, 0xF4606402, 0xF4606403, 0xF4606404),
		Diagnostic));
	check(Renderer.Show(Commands.Show).IsApplied());
	const auto CleanupDecision = EvaluateArcSurfaceRecreation(
		Fixture,
		FRenderer::StableConsumerDefinitionId(),
		Commands.Hide.GetState(),
		FRenderer::StableConsumerDefinitionId(),
		Commands.Show.GetState(),
		EArcSurfaceRecreationAction::ClearToEmpty);
	const auto BindingDecision = EvaluateArcSurfaceRecreation(
		Fixture,
		FRenderer::StableConsumerDefinitionId(),
		Commands.Show.GetState(),
		FRenderer::StableConsumerDefinitionId(),
		Commands.Show.GetState(),
		EArcSurfaceRecreationAction::AdoptExact);
	check(CleanupDecision.IsAuthorized() && CleanupDecision.HasPermit());
	check(BindingDecision.IsAuthorized() && BindingDecision.HasPermit());

	const auto WrongPermit = Renderer.ClearToEmpty(
		BindingDecision.GetPermit());
	const auto Cleared = Renderer.ClearToEmpty(
		CleanupDecision.GetPermit());
	const auto Replay = Renderer.ClearToEmpty(
		CleanupDecision.GetPermit());
	TestTrue(TEXT("binding permit cannot masquerade as cleanup authority"),
		!WrongPermit.IsValid());
	TestTrue(TEXT("exact cleanup permit emits applied evidence and clears once"),
		Cleared.IsValid() && Cleared.IsApplied()
			&& Cleared.GetPreviousSurfaceCursor().Matches(
				Commands.Show.GetState())
			&& Cleared.GetSurfaceCursor().IsEmpty()
			&& Renderer.GetSurfaceCursor().IsEmpty());
	TestTrue(TEXT("consumed cleanup snapshot cannot clear an empty surface again"),
		!Replay.IsValid() && Renderer.GetSurfaceCursor().IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewMainHUDRendererRetirementTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewMainHUDRendererAdapter.ExactHandoffRetirement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewMainHUDRendererRetirementTest::
RunTest(const FString&)
{
	using FRenderer =
		Fdemo_mapShanmenThrownWeaponArcPreviewMainHUDRendererAdapter;
	FThrownLifecycleFixture Fixture;
	FArcPreviewSurfaceCommandSet Commands;
	if (!BuildArcPreviewSurfaceCommands(
			*this, TEXT("ArcPreviewMainHUDRendererRetirement"),
			Fixture, Commands))
	{
		return false;
	}

	const FGuid OldSurfaceId(
		0xF4606501, 0xF4606502, 0xF4606503, 0xF4606504);
	const FGuid NewSurfaceId(
		0xF4606601, 0xF4606602, 0xF4606603, 0xF4606604);
	FRenderer OldRenderer;
	FRenderer NewRenderer;
	FString Diagnostic;
	check(OldRenderer.TryInitialize(OldSurfaceId, Diagnostic));
	check(NewRenderer.TryInitialize(NewSurfaceId, Diagnostic));
	check(OldRenderer.Show(Commands.Show).IsApplied());
	check(NewRenderer.Show(Commands.Show).IsApplied());

	FArcSurfaceOwnershipRequest Request;
	if (!BuildArcSurfaceOwnershipRequest(
			*this,
			Fixture,
			FRenderer::StableConsumerDefinitionId(),
			Commands.Show.GetState(),
			NewSurfaceId,
			EArcSurfaceRecreationAction::AdoptExact,
			Request))
	{
		return false;
	}
	FArcSurfaceOwnershipTransition Transition;
	const auto Ownership = Transition.Execute(Request, NewRenderer);
	check(Ownership.IsValid() && Ownership.DidAuthorizeBinding()
		&& Ownership.HasTransitionTicket());

	const auto Retired = OldRenderer.RetireForHandoff(
		Ownership.GetTransitionTicket());
	const auto Replay = OldRenderer.RetireForHandoff(
		Ownership.GetTransitionTicket());
	TestTrue(TEXT("new physical surface authorizes exact visible adoption"),
		Ownership.GetTransitionTicket().MatchesCandidateSnapshot(
			NewSurfaceId,
			FRenderer::StableConsumerDefinitionId(),
			Commands.Show.GetState())
			&& NewRenderer.GetSurfaceCursor().Matches(
				Commands.Show.GetState()));
	TestTrue(TEXT("old physical surface retires with typed applied evidence"),
		Retired.IsValid() && Retired.IsApplied()
			&& Retired.GetPreviousSurfaceCursor().Matches(
				Commands.Show.GetState())
			&& Retired.GetSurfaceCursor().IsEmpty()
			&& OldRenderer.GetSurfaceCursor().IsEmpty());
	TestTrue(TEXT("retirement replay cannot mutate the already empty surface"),
		!Replay.IsValid() && OldRenderer.GetSurfaceCursor().IsEmpty()
			&& NewRenderer.GetSurfaceCursor().Matches(
				Commands.Show.GetState()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewMainHUDRuntimeInitializationTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewMainHUDRuntimeBinding.InitializationAndDirectHUDRunBinding",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewMainHUDRuntimeInitializationTest::
RunTest(const FString&)
{
	using FBinding =
		Fdemo_mapShanmenThrownWeaponArcPreviewMainHUDRuntimeBinding;
	using FRenderer =
		Fdemo_mapShanmenThrownWeaponArcPreviewMainHUDRendererAdapter;
	FThrownLifecycleFixture Fixture;
	if (!Fixture.Start(
			*this,
			TEXT("ArcPreviewMainHUDRuntimeInitialization"),
			Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::BallisticArc))
	{
		return false;
	}

	const FGuid FallbackId(
		0xF4610001, 0xF4610002, 0xF4610003, 0xF4610004);
	const FGuid HUDId(
		0xF4610101, 0xF4610102, 0xF4610103, 0xF4610104);
	FBinding Binding;
	FRenderer HUD;
	FString Diagnostic;
	TestTrue(TEXT("default runtime binding is valid but explicitly uninitialized"),
		Binding.IsValid() && !Binding.IsInitialized()
			&& !Binding.IsActive() && !Binding.HasAttachedHUD());
	TestFalse(TEXT("missing fallback identity fails closed"),
		Binding.TryInitialize(FGuid(), Diagnostic));
	check(Binding.TryInitialize(FallbackId, Diagnostic));
	check(HUD.TryInitialize(HUDId, Diagnostic));
	TestTrue(TEXT("HUD can attach before any product Run"),
		Binding.TryAttachHUD(HUD, Diagnostic)
			&& Binding.IsValid() && Binding.HasAttachedHUD()
			&& !Binding.IsActive());
	TestTrue(TEXT("Run begins directly on the already attached HUD"),
		Binding.TryBeginRun(Fixture.Correlation.ActiveRunId, Diagnostic)
			&& Binding.IsValid() && Binding.IsActive()
			&& Binding.IsBoundToHUD()
			&& Binding.GetBoundSurfaceInstanceId() == HUDId
			&& HUD.GetSurfaceCursor().IsEmpty());
	TestTrue(TEXT("empty presentation Run tears down without product mutation"),
		Binding.TryEndRun(
			Fixture.Correlation.ActiveRunId,
			Fixture.Lifecycle,
			Fixture.Coordinator,
			Diagnostic)
			&& Binding.IsValid() && !Binding.IsActive()
			&& Binding.HasAttachedHUD());
	TestTrue(TEXT("inactive HUD detach removes only registration"),
		Binding.TryDetachHUD(HUD, Diagnostic)
			&& Binding.IsValid() && !Binding.HasAttachedHUD());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewMainHUDRuntimeFallbackTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewMainHUDRuntimeBinding.FallbackAndFreshLateAttach",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewMainHUDRuntimeFallbackTest::
RunTest(const FString&)
{
	using FBinding =
		Fdemo_mapShanmenThrownWeaponArcPreviewMainHUDRuntimeBinding;
	using FRenderer =
		Fdemo_mapShanmenThrownWeaponArcPreviewMainHUDRendererAdapter;
	FThrownLifecycleFixture Fixture;
	if (!Fixture.Start(
			*this,
			TEXT("ArcPreviewMainHUDRuntimeFallback"),
			Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::BallisticArc))
	{
		return false;
	}

	const FGuid FallbackId(
		0xF4610201, 0xF4610202, 0xF4610203, 0xF4610204);
	const FGuid HUDId(
		0xF4610301, 0xF4610302, 0xF4610303, 0xF4610304);
	FBinding Binding;
	FRenderer HUD;
	FString Diagnostic;
	check(Binding.TryInitialize(FallbackId, Diagnostic));
	check(HUD.TryInitialize(HUDId, Diagnostic));
	check(Binding.TryBeginRun(Fixture.Correlation.ActiveRunId, Diagnostic));
	TestTrue(TEXT("Run without a HUD binds the permanent empty fallback"),
		Binding.IsValid() && Binding.IsActive()
			&& !Binding.IsBoundToHUD()
			&& Binding.GetBoundSurfaceInstanceId() == FallbackId
			&& Binding.GetFallbackSurface().GetSurfaceCursor().IsEmpty());
	TestTrue(TEXT("late empty HUD uses the existing BindFresh ownership path"),
		Binding.TryAttachHUD(HUD, Diagnostic)
			&& Binding.IsValid() && Binding.IsBoundToHUD()
			&& Binding.GetBoundSurfaceInstanceId() == HUDId
			&& Binding.GetFallbackSurface().GetSurfaceCursor().IsEmpty()
			&& HUD.GetSurfaceCursor().IsEmpty()
			&& Binding.GetOwner().GetLastSurfaceHandoffReceipt()
				.IsFreshBound());
	check(Binding.TryEndRun(
		Fixture.Correlation.ActiveRunId,
		Fixture.Lifecycle,
		Fixture.Coordinator,
		Diagnostic));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewMainHUDRuntimeVisibleRecreationTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewMainHUDRuntimeBinding.VisibleHUDRecreationAndDetach",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewMainHUDRuntimeVisibleRecreationTest::
RunTest(const FString&)
{
	using FBinding =
		Fdemo_mapShanmenThrownWeaponArcPreviewMainHUDRuntimeBinding;
	using FRenderer =
		Fdemo_mapShanmenThrownWeaponArcPreviewMainHUDRendererAdapter;
	using FUpdate =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCompositionOwnerUpdateResult;
	FThrownLifecycleFixture Fixture;
	if (!Fixture.Start(
			*this,
			TEXT("ArcPreviewMainHUDRuntimeVisibleRecreation"),
			Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::BallisticArc))
	{
		return false;
	}

	FBinding Binding;
	FRenderer FirstHUD;
	FRenderer ReplacementHUD;
	FString Diagnostic;
	check(Binding.TryInitialize(
		FGuid(0xF4610401, 0xF4610402, 0xF4610403, 0xF4610404),
		Diagnostic));
	check(FirstHUD.TryInitialize(
		FGuid(0xF4610501, 0xF4610502, 0xF4610503, 0xF4610504),
		Diagnostic));
	check(ReplacementHUD.TryInitialize(
		FGuid(0xF4610601, 0xF4610602, 0xF4610603, 0xF4610604),
		Diagnostic));
	check(Binding.TryAttachHUD(FirstHUD, Diagnostic));
	check(Binding.TryBeginRun(Fixture.Correlation.ActiveRunId, Diagnostic));
	const auto Choice = MakeArcPreviewChoice(false);
	FUpdate ShowResult;
	check(Binding.TryUpdate(
		2,
		Choice,
		MakeArcPreviewChoicePolicy(),
		MakeArcPreviewBasis(),
		Fixture.Lifecycle,
		Fixture.Coordinator,
		ShowResult,
		Diagnostic));
	const FArcSurfaceState Visible = Binding.GetState();
	TestTrue(TEXT("explicit source-basis update publishes one visible cursor"),
		ShowResult.IsAccepted() && Visible.IsVisible()
			&& FirstHUD.GetSurfaceCursor().Matches(Visible));

	TestTrue(TEXT("replacement HUD rehydrates and adopts the exact visible cursor"),
		Binding.TryAttachHUD(ReplacementHUD, Diagnostic)
			&& Binding.IsValid() && Binding.IsBoundToHUD()
			&& ReplacementHUD.GetSurfaceCursor().Matches(Visible)
			&& FirstHUD.GetSurfaceCursor().IsEmpty()
			&& Binding.GetOwner().GetLastSurfaceHandoffReceipt()
				.IsExactAdopted());
	TestTrue(TEXT("destroying the visible HUD first hands ownership to fallback"),
		Binding.TryDetachHUD(ReplacementHUD, Diagnostic)
			&& Binding.IsValid() && Binding.IsActive()
			&& !Binding.IsBoundToHUD() && !Binding.HasAttachedHUD()
			&& ReplacementHUD.GetSurfaceCursor().IsEmpty()
			&& Binding.GetFallbackSurface().GetSurfaceCursor().Matches(
				Visible));
	TestTrue(TEXT("an empty former HUD can be attached as a new generation"),
		Binding.TryAttachHUD(FirstHUD, Diagnostic)
			&& Binding.IsValid() && Binding.IsBoundToHUD()
			&& FirstHUD.GetSurfaceCursor().Matches(Visible)
			&& Binding.GetFallbackSurface().GetSurfaceCursor().IsEmpty());
	check(Binding.TryEndRun(
		Fixture.Correlation.ActiveRunId,
		Fixture.Lifecycle,
		Fixture.Coordinator,
		Diagnostic));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewMainHUDRuntimeReplayTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewMainHUDRuntimeBinding.ExactUpdateReplay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewMainHUDRuntimeReplayTest::
RunTest(const FString&)
{
	using EUpdate =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationCompositionOwnerUpdateStatus;
	using FBinding =
		Fdemo_mapShanmenThrownWeaponArcPreviewMainHUDRuntimeBinding;
	using FUpdate =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCompositionOwnerUpdateResult;
	FThrownLifecycleFixture Fixture;
	if (!Fixture.Start(
			*this,
			TEXT("ArcPreviewMainHUDRuntimeReplay"),
			Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::BallisticArc))
	{
		return false;
	}
	FBinding Binding;
	FString Diagnostic;
	check(Binding.TryInitialize(
		FGuid(0xF4610701, 0xF4610702, 0xF4610703, 0xF4610704),
		Diagnostic));
	check(Binding.TryBeginRun(Fixture.Correlation.ActiveRunId, Diagnostic));
	const auto Choice = MakeArcPreviewChoice();
	const auto Policy = MakeArcPreviewChoicePolicy();
	const auto Basis = MakeArcPreviewBasis();
	FUpdate First;
	FUpdate NoOp;
	FUpdate Replay;
	check(Binding.TryUpdate(
		2, Choice, Policy, Basis, Fixture.Lifecycle, Fixture.Coordinator,
		First, Diagnostic));
	const FArcSurfaceState FirstCursor = Binding.GetBoundSurfaceCursor();
	check(Binding.TryUpdate(
		2, Choice, Policy, Basis, Fixture.Lifecycle, Fixture.Coordinator,
		NoOp, Diagnostic));
	check(Binding.TryUpdate(
		2, Choice, Policy, Basis, Fixture.Lifecycle, Fixture.Coordinator,
		Replay, Diagnostic));
	TestTrue(TEXT("duplicate produces one NoOp then exact replay stays above the surface"),
		First.GetStatus() == EUpdate::Applied
			&& NoOp.GetStatus() == EUpdate::Applied
			&& NoOp.DidCallAdapter()
			&& NoOp.GetAdapterResult().IsNoOp()
			&& !NoOp.GetAdapterResult().DidCallSurface()
			&& Replay.GetStatus() == EUpdate::ApplicationReplayed
			&& Replay.IsReplay()
			&& !Replay.DidCallAdapter()
			&& Binding.GetBoundSurfaceCursor().Matches(FirstCursor));
	check(Binding.TryEndRun(
		Fixture.Correlation.ActiveRunId,
		Fixture.Lifecycle,
		Fixture.Coordinator,
		Diagnostic));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreviewMainHUDRuntimeTeardownFenceTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreviewMainHUDRuntimeBinding.LocalClearAndLifecycleFences",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreviewMainHUDRuntimeTeardownFenceTest::
RunTest(const FString&)
{
	using FBinding =
		Fdemo_mapShanmenThrownWeaponArcPreviewMainHUDRuntimeBinding;
	using FRenderer =
		Fdemo_mapShanmenThrownWeaponArcPreviewMainHUDRendererAdapter;
	using FUpdate =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCompositionOwnerUpdateResult;
	FThrownLifecycleFixture Fixture;
	if (!Fixture.Start(
			*this,
			TEXT("ArcPreviewMainHUDRuntimeTeardownFence"),
			Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::BallisticArc))
	{
		return false;
	}
	FBinding Binding;
	FRenderer HUD;
	FRenderer StaleHUD;
	FString Diagnostic;
	check(Binding.TryInitialize(
		FGuid(0xF4610801, 0xF4610802, 0xF4610803, 0xF4610804),
		Diagnostic));
	check(HUD.TryInitialize(
		FGuid(0xF4610901, 0xF4610902, 0xF4610903, 0xF4610904),
		Diagnostic));
	check(StaleHUD.TryInitialize(
		FGuid(0xF4610A01, 0xF4610A02, 0xF4610A03, 0xF4610A04),
		Diagnostic));
	check(Binding.TryAttachHUD(HUD, Diagnostic));
	check(Binding.TryBeginRun(Fixture.Correlation.ActiveRunId, Diagnostic));
	const auto AuthoritativeChoice = MakeArcPreviewChoice(false);
	FUpdate VisibleResult;
	check(Binding.TryUpdate(
		2,
		AuthoritativeChoice,
		MakeArcPreviewChoicePolicy(),
		MakeArcPreviewBasis(),
		Fixture.Lifecycle,
		Fixture.Coordinator,
		VisibleResult,
		Diagnostic));

	TestFalse(TEXT("foreign Run teardown cannot clear or end the live owner"),
		Binding.TryEndRun(
			FGuid::NewGuid(),
			Fixture.Lifecycle,
			Fixture.Coordinator,
			Diagnostic));
	TestTrue(TEXT("foreign teardown leaves exact visible state intact"),
		Binding.IsValid() && Binding.IsActive()
			&& HUD.GetSurfaceCursor().IsVisible());
	TestTrue(TEXT("stale HUD detach is an idempotent no-op"),
		Binding.TryDetachHUD(StaleHUD, Diagnostic)
			&& Binding.IsBoundToHUD()
			&& HUD.GetSurfaceCursor().IsVisible());
	FUpdate LocalClear;
	TestTrue(TEXT("explicit completion clear hides the surface through a local choice copy"),
		Binding.TryClear(
			AuthoritativeChoice,
			Fixture.Lifecycle,
			Fixture.Coordinator,
			LocalClear,
			Diagnostic)
			&& LocalClear.IsAccepted()
			&& Binding.IsActive()
			&& HUD.GetSurfaceCursor().IsEmpty());
	TestTrue(TEXT("matching teardown ends an already hidden runtime binding"),
		Binding.TryEndRun(
			Fixture.Correlation.ActiveRunId,
			Fixture.Lifecycle,
			Fixture.Coordinator,
			Diagnostic)
			&& Binding.IsValid() && !Binding.IsActive()
			&& HUD.GetSurfaceCursor().IsEmpty());
	TestTrue(TEXT("presentation teardown never mutates caller-owned choice"),
		AuthoritativeChoice.IsValid()
			&& AuthoritativeChoice.HasArcTargetIntent()
			&& AuthoritativeChoice.GetRevision() == 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreLaunchPreviewRuntimeCompositionTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreLaunchPreviewContext.LiveEditCancelAndConfirmation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreLaunchPreviewRuntimeCompositionTest::
RunTest(const FString&)
{
	using EAction =
		Edemo_mapShanmenThrownWeaponArcPreLaunchPressAction;
	using FBinding =
		Fdemo_mapShanmenThrownWeaponArcPreviewMainHUDRuntimeBinding;
	using FUpdate =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCompositionOwnerUpdateResult;
	FThrownLifecycleFixture Fixture;
	if (!Fixture.Start(
			*this,
			TEXT("ArcPreLaunchPreviewRuntimeComposition"),
			Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::BallisticArc))
	{
		return false;
	}

	FBinding Binding;
	Fdemo_mapShanmenThrownWeaponArcPreLaunchPreviewContext Context;
	FString Diagnostic;
	check(Binding.TryInitialize(
		FGuid(0xF4630301, 0xF4630302, 0xF4630303, 0xF4630304),
		Diagnostic));
	check(Binding.TryBeginRun(Fixture.Correlation.ActiveRunId, Diagnostic));
	check(Context.TryBegin(Fixture.Correlation.ActiveRunId, Diagnostic));
	const uint64 InitialActivationSequence =
		Fixture.Coordinator.GetNextPlayerThrownWeaponActivationSequence();
	const int32 InitialCapturedSelections =
		Fixture.Lifecycle.NumCapturedSelections();
	const auto Policy = MakeArcPreviewChoicePolicy();
	const auto Basis = MakeArcPreviewBasis();
	const auto InitialChoice = MakeArcPreviewChoice(false);

	FUpdate InitialPreview;
	check(Binding.TryUpdate(
		2,
		InitialChoice,
		Policy,
		Basis,
		Fixture.Lifecycle,
		Fixture.Coordinator,
		InitialPreview,
		Diagnostic));
	EAction Action = EAction::Invalid;
	check(Context.RouteEligibleHotbarPress(
		Fixture.Correlation.ActiveRunId, 2, Action, Diagnostic));
	TestTrue(TEXT("first hotbar press arms one visible preview without launching"),
		Action == EAction::Armed
			&& Context.GetArmedHotbarSlotNumber() == 2
			&& Binding.GetState().IsVisible()
			&& Fixture.Coordinator.
				GetNextPlayerThrownWeaponActivationSequence()
				== InitialActivationSequence
			&& Fixture.Lifecycle.NumCapturedSelections()
				== InitialCapturedSelections);

	Fdemo_mapShanmenThrownWeaponInputChoiceCommand AdjustCommand;
	check(Fdemo_mapShanmenThrownWeaponInputChoiceCommand::
		TryCaptureArcApexAdjustment(
			InitialChoice.GetRevision(), -1.0, AdjustCommand));
	const auto Adjusted =
		Fdemo_mapShanmenThrownWeaponInputChoiceReducer::Reduce(
			InitialChoice, AdjustCommand);
	check(Adjusted.DidChange());
	FUpdate AdjustedPreview;
	check(Binding.TryUpdate(
		2,
		Adjusted.State,
		Policy,
		Basis,
		Fixture.Lifecycle,
		Fixture.Coordinator,
		AdjustedPreview,
		Diagnostic));
	TestTrue(TEXT("live apex edit replaces only presentation state"),
		AdjustedPreview.IsAccepted()
			&& Binding.GetState().IsVisible()
			&& Binding.GetState().GetChoiceRevision()
				== Adjusted.State.GetRevision()
			&& Fixture.Coordinator.
				GetNextPlayerThrownWeaponActivationSequence()
				== InitialActivationSequence
			&& Fixture.Lifecycle.NumCapturedSelections()
				== InitialCapturedSelections);

	const uint64 ArmedRevision = Context.GetRevision();
	check(Context.RouteEligibleHotbarPress(
		Fixture.Correlation.ActiveRunId, 2, Action, Diagnostic));
	check(Context.TryCompleteConfirmation(
		Fixture.Correlation.ActiveRunId, 2, false, Diagnostic));
	TestTrue(TEXT("rejected confirmation keeps the exact preview correctable"),
		Action == EAction::ConfirmationRequested
			&& Context.GetArmedHotbarSlotNumber() == 2
			&& Context.GetRevision() == ArmedRevision
			&& Binding.GetState().IsVisible());

	const auto ClearedChoice = ClearArcPreviewChoice(Adjusted.State);
	FUpdate ClearedPreview;
	check(Binding.TryUpdate(
		2,
		ClearedChoice,
		Policy,
		Fdemo_mapShanmenThrownWeaponArcChoiceBasis(),
		Fixture.Lifecycle,
		Fixture.Coordinator,
		ClearedPreview,
		Diagnostic));
	check(Context.TryCancel(
		Fixture.Correlation.ActiveRunId, Diagnostic));
	TestTrue(TEXT("target clear hides and disarms without product mutation"),
		!Context.HasArmedHotbarSlot()
			&& Binding.GetState().IsHidden()
			&& Fixture.Coordinator.
				GetNextPlayerThrownWeaponActivationSequence()
				== InitialActivationSequence
			&& Fixture.Lifecycle.NumCapturedSelections()
				== InitialCapturedSelections);

	const auto RetargetedChoice = RetargetArcPreviewChoice(ClearedChoice);
	FUpdate RetargetedPreview;
	check(Binding.TryUpdate(
		2,
		RetargetedChoice,
		Policy,
		Basis,
		Fixture.Lifecycle,
		Fixture.Coordinator,
		RetargetedPreview,
		Diagnostic));
	check(Context.RouteEligibleHotbarPress(
		Fixture.Correlation.ActiveRunId, 2, Action, Diagnostic));
	check(Context.RouteEligibleHotbarPress(
		Fixture.Correlation.ActiveRunId, 2, Action, Diagnostic));
	check(Context.TryCompleteConfirmation(
		Fixture.Correlation.ActiveRunId, 2, true, Diagnostic));
	FUpdate CompletionClear;
	check(Binding.TryClear(
		RetargetedChoice,
		Fixture.Lifecycle,
		Fixture.Coordinator,
		CompletionClear,
		Diagnostic));
	TestTrue(TEXT("accepted confirmation consumes only context and preview"),
		!Context.HasArmedHotbarSlot()
			&& Binding.GetState().IsHidden()
			&& RetargetedChoice.HasArcTargetIntent()
			&& Fixture.Coordinator.
				GetNextPlayerThrownWeaponActivationSequence()
				== InitialActivationSequence
			&& Fixture.Lifecycle.NumCapturedSelections()
				== InitialCapturedSelections);

	check(Binding.TryEndRun(
		Fixture.Correlation.ActiveRunId,
		Fixture.Lifecycle,
		Fixture.Coordinator,
		Diagnostic));
	check(Context.TryEnd(Fixture.Correlation.ActiveRunId, Diagnostic));
	return true;
}

#endif
