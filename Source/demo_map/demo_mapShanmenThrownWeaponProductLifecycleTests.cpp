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

#endif
