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
#include "demo_mapShanmenThrownWeaponArcPreviewPresentation.h"
#include "demo_mapShanmenThrownWeaponArcPreviewPresentationCommand.h"
#include "demo_mapShanmenThrownWeaponArcPreviewPresentationCommandLedger.h"
#include "demo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryCoordinator.h"
#include "demo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryHost.h"
#include "demo_mapShanmenThrownWeaponArcPreviewPresentationDeliverySession.h"
#include "demo_mapShanmenThrownWeaponArcPreviewPresentationSession.h"
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

#endif
