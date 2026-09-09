#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenThrownWeaponInputAdapter.h"

#include "demo_map0909BSectWarehouseService.h"
#include "demo_mapAttributeComponent.h"
#include "demo_mapCombatRunCoordinator.h"
#include "demo_mapGameMode.h"
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
	FString NewThrownInputRoot(const TCHAR* Label)
	{
		return FPaths::Combine(
			FPaths::ProjectSavedDir(),
			TEXT("Automation"),
			TEXT("Dev.D.UE.0.0.10.P20.9.r0"),
			Label,
			FGuid::NewGuid().ToString(EGuidFormats::Digits));
	}

	struct FThrownInputFixture
	{
		FString Root;
		Fdemo_mapProfileStorageContext Storage;
		Fdemo_mapPersistentProfile SeedProfile;
		FGuid TrainingBladeId;
		FGuid HealingPillId;
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
		Fdemo_mapShanmenThrownWeaponInputAdapter Adapter;

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
			Root = NewThrownInputRoot(Label);
			Storage = Fdemo_mapProfileStorageContext::ForRoot(Root);
			Fdemo_mapProfileRepository Repository;
			SeedProfile = Repository.CreateFreshProfile();
			for (const Fdemo_mapPersistentItemRecord& Item :
				SeedProfile.PermanentStash)
			{
				if (Item.ItemDefinitionId == Fdemo_mapItemIds::TrainingBlade)
				{
					TrainingBladeId = Item.ItemInstanceId;
				}
			}
			auto AddConsumable = [this](
				const FName DefinitionId,
				const int32 Quantity,
				FGuid& OutId)
			{
				Fdemo_mapPersistentItemRecord Item;
				OutId = Item.ItemInstanceId = FGuid::NewGuid();
				Item.ItemDefinitionId = DefinitionId;
				Item.StackCount = Quantity;
				Item.PersistentDomain =
					Edemo_mapPersistentDomain::PermanentStash;
				SeedProfile.PermanentStash.Add(Item);
			};
			AddConsumable(Fdemo_mapItemIds::HealingPillLevel1, 2, HealingPillId);
			AddConsumable(
				Fdemo_mapItemIds::TrainingThrowingKnife, 3, ThrowingKnifeId);
			SeedProfile.PreparationLayout.WeaponItemInstanceId = TrainingBladeId;
			const Fdemo_mapProfileSaveResult Saved =
				Repository.SaveProfile(SeedProfile, Storage);
			if (!TrainingBladeId.IsValid() || !HealingPillId.IsValid()
				|| !ThrowingKnifeId.IsValid() || !Saved.IsSuccess() || !GEngine)
			{
				Test.AddError(TEXT("P20.9 could not seed isolated hotbar content."));
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
					TEXT("P20.9 stable migration source failed: %s"),
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
					HealingPillId, true).IsAccepted()
				|| !ProfileSession->SetPreparationMaterial(
					ThrowingKnifeId, true).IsAccepted()
				|| !ProfileSession->SetPreparationHotbarSlot(
					1, HealingPillId).IsAccepted()
				|| !ProfileSession->SetPreparationHotbarSlot(
					2, ThrowingKnifeId).IsAccepted())
			{
				Test.AddError(FString::Printf(
					TEXT("P20.9 authority preparation failed: %s"),
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
				|| Correlation.HotbarItemInstanceIds[0] != HealingPillId
				|| Correlation.HotbarItemInstanceIds[1] != ThrowingKnifeId)
			{
				Test.AddError(FString::Printf(
					TEXT("P20.9 durable active Run failed: %s"),
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
					Source, TEXT("P209PlayerHealth"))
				: nullptr;
			Attributes = Source
				? NewObject<Udemo_mapAttributeComponent>(
					Source, TEXT("P209PlayerAttributes"))
				: nullptr;
			if (Source && Attributes)
			{
				Source->AddInstanceComponent(Attributes);
				Source->SetActorLocation(FVector(100.0, 200.0, 30.0));
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
					TEXT("P20.9 product lifecycle failed: %s"),
					*Diagnostic));
				return false;
			}
			return true;
		}

		Fdemo_mapShanmenThrownWeaponInputResult Route(
			const int32 Slot,
			AActor* RequestedSource,
			int32& AimSampleCount,
			const FVector Aim)
		{
			return Adapter.RouteHotbarInput(
				Authority,
				Lifecycle,
				Coordinator,
				World,
				Ademo_mapShanmenThrownWeaponProjectile::StaticClass(),
				RequestedSource,
				Slot,
				[&AimSampleCount, Aim]()
				{
					++AimSampleCount;
					return Aim;
				},
				[this]()
				{
					return Fdemo_mapShanmenPlayerActionGateResult::
						FromArbitration(
							Coordinator.TryAuthorizePlayerAction(
								Edemo_mapShanmenPlayerActionKind::
									ThrownWeapon,
								Fdemo_mapShanmenPlayerActionOccupancySnapshot()));
				});
		}

		Fdemo_mapShanmenThrownWeaponInputResult RouteArc(
			const int32 Slot,
			AActor* RequestedSource,
			int32& TargetSampleCount,
			const FVector Target,
			int32& ApexSampleCount,
			const double ApexClearance,
			int32& AuthorizationCount)
		{
			return Adapter.RouteArcHotbarInput(
				Authority,
				Lifecycle,
				Coordinator,
				World,
				Ademo_mapShanmenThrownWeaponProjectile::StaticClass(),
				RequestedSource,
				Slot,
				[&TargetSampleCount, Target]()
				{
					++TargetSampleCount;
					return Target;
				},
				[&ApexSampleCount, ApexClearance]()
				{
					++ApexSampleCount;
					return ApexClearance;
				},
				[this, &AuthorizationCount]()
				{
					++AuthorizationCount;
					return Fdemo_mapShanmenPlayerActionGateResult::
						FromArbitration(
							Coordinator.TryAuthorizePlayerAction(
								Edemo_mapShanmenPlayerActionKind::
									ThrownWeapon,
								Fdemo_mapShanmenPlayerActionOccupancySnapshot()));
				});
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

		~FThrownInputFixture()
		{
			Stop();
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponInputIdentityTest,
	"Shanmen.0_0_10.Product.ThrownWeaponInputAdapter.DeterministicIdentity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponInputIdentityTest::RunTest(const FString&)
{
	const FGuid CorrelationId = FGuid::NewGuid();
	const FGuid RunId = FGuid::NewGuid();
	const FGuid ItemId = FGuid::NewGuid();
	const FGuid First =
		Fdemo_mapShanmenThrownWeaponInputAdapter::MakeSelectionId(
			CorrelationId, RunId, ItemId, 2, 17, 4);
	const FGuid Replay =
		Fdemo_mapShanmenThrownWeaponInputAdapter::MakeSelectionId(
			CorrelationId, RunId, ItemId, 2, 17, 4);
	const FGuid Next =
		Fdemo_mapShanmenThrownWeaponInputAdapter::MakeSelectionId(
			CorrelationId, RunId, ItemId, 2, 17, 5);
	TestTrue(TEXT("Canonical input evidence reproduces one stable SelectionId"),
		First.IsValid() && First == Replay && First != Next);
	TestFalse(TEXT("Invalid event identity fails closed"),
		Fdemo_mapShanmenThrownWeaponInputAdapter::MakeSelectionId(
			CorrelationId, RunId, ItemId, 0, 17, 4).IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponHandReleaseOriginTest,
	"Shanmen.0_0_10.Product.ThrownWeaponInputAdapter.HandReleaseOrigin",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponHandReleaseOriginTest::RunTest(const FString&)
{
	const FVector Location(100.0, 200.0, 30.0);
	const FVector FacingForward =
		Fdemo_mapShanmenThrownWeaponInputAdapter::MakeLaunchOrigin(
			FTransform(FRotator::ZeroRotator, Location));
	const FVector FacingRight =
		Fdemo_mapShanmenThrownWeaponInputAdapter::MakeLaunchOrigin(
			FTransform(FRotator(0.0, 90.0, 0.0), Location));
	TestTrue(TEXT("Forward-facing release is ahead, right, and above the source"),
		FacingForward.Equals(FVector(155.0, 228.0, 80.0)));
	TestTrue(TEXT("Hand offset rotates with source yaw while height stays world-up"),
		FacingRight.Equals(FVector(72.0, 255.0, 80.0)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponInputRoutingTest,
	"Shanmen.0_0_10.Product.ThrownWeaponInputAdapter.TypedRouteAndPassThrough",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponInputRoutingTest::RunTest(const FString&)
{
	FThrownInputFixture Fixture;
	if (!Fixture.Start(*this, TEXT("Routing")))
	{
		return false;
	}
	FShanmenItemAuthoritySnapshot Before;
	Fixture.Authority->TryCaptureSnapshot(Before);
	int32 AimSamples = 0;
	const Fdemo_mapShanmenThrownWeaponInputResult Pill = Fixture.Route(
		1, Fixture.Source, AimSamples, FVector::ForwardVector);
	const Fdemo_mapShanmenThrownWeaponInputResult Empty = Fixture.Route(
		3, Fixture.Source, AimSamples, FVector::ForwardVector);
	FShanmenItemAuthoritySnapshot AfterPassThrough;
	Fixture.Authority->TryCaptureSnapshot(AfterPassThrough);
	TestTrue(TEXT("Ordinary and empty slots preserve the existing path without aim sampling"),
		Pill.ShouldPassThrough()
		&& Pill.ItemInstanceId == Fixture.HealingPillId
		&& Empty.ShouldPassThrough()
		&& AimSamples == 0
		&& Fixture.Adapter.GetNextSelectionOrdinal() == 1
		&& AfterPassThrough == Before);

	const FGuid ExpectedSelectionId =
		Fdemo_mapShanmenThrownWeaponInputAdapter::MakeSelectionId(
			Fixture.Correlation.CorrelationId,
			Fixture.Correlation.ActiveRunId,
			Fixture.ThrowingKnifeId,
			2,
			Before.AuthorityRevision,
			1);
	const Fdemo_mapShanmenThrownWeaponInputResult Thrown = Fixture.Route(
		2, Fixture.Source, AimSamples, FVector::ForwardVector);
	FShanmenItemAuthoritySnapshot AfterThrown;
	Fixture.Authority->TryCaptureSnapshot(AfterThrown);
	const FVector ExpectedOrigin = Fixture.Source->GetActorLocation()
		+ Fixture.Source->GetActorForwardVector() * 55.0
		+ Fixture.Source->GetActorRightVector() * 28.0
		+ FVector::UpVector * 50.0;
	const Fdemo_mapShanmenThrownWeaponRunCommandIntent* Command =
		Fixture.Lifecycle.FindCapturedCommand(Thrown.SelectionId);
	TestTrue(TEXT("Typed thrown slot alone samples aim and enters the product lifecycle"),
		Thrown.IsAccepted()
		&& Thrown.bAimSampled
		&& Thrown.SelectionOrdinal == 1
		&& Thrown.SelectionId == ExpectedSelectionId
		&& Thrown.ItemInstanceId == Fixture.ThrowingKnifeId
		&& Command
		&& Command->GetOrigin().Equals(ExpectedOrigin)
		&& AimSamples == 1
		&& Fixture.Adapter.GetNextSelectionOrdinal() == 2
		&& AfterThrown.AuthorityRevision == Before.AuthorityRevision + 2);
	TestTrue(TEXT("Input sequence resets only at the explicit lifecycle boundary"),
		Fixture.Lifecycle.TryInterruptFlight());
	Fixture.Adapter.Reset();
	TestEqual(TEXT("Explicit reset returns the transient ordinal to one"),
		Fixture.Adapter.GetNextSelectionOrdinal(), uint64(1));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcInputRoutingTest,
	"Shanmen.0_0_10.Product.ThrownWeaponInputAdapter.ArcTypedRouteAndPassThrough",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcInputRoutingTest::RunTest(const FString&)
{
	FThrownInputFixture Fixture;
	if (!Fixture.Start(
			*this,
			TEXT("ArcRouting"),
			Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::BallisticArc))
	{
		return false;
	}
	FShanmenItemAuthoritySnapshot Before;
	Fixture.Authority->TryCaptureSnapshot(Before);
	int32 TargetSamples = 0;
	int32 ApexSamples = 0;
	int32 AuthorizationCount = 0;
	const FVector Target(640.0, 250.0, 70.0);
	const double ApexClearance = 180.0;
	const Fdemo_mapShanmenThrownWeaponInputResult Pill = Fixture.RouteArc(
		1,
		Fixture.Source,
		TargetSamples,
		Target,
		ApexSamples,
		ApexClearance,
		AuthorizationCount);
	const Fdemo_mapShanmenThrownWeaponInputResult Empty = Fixture.RouteArc(
		3,
		Fixture.Source,
		TargetSamples,
		Target,
		ApexSamples,
		ApexClearance,
		AuthorizationCount);
	FShanmenItemAuthoritySnapshot AfterPassThrough;
	Fixture.Authority->TryCaptureSnapshot(AfterPassThrough);
	TestTrue(TEXT("Arc route preserves non-thrown and empty hotbar ownership"),
		Pill.ShouldPassThrough()
		&& Pill.ItemInstanceId == Fixture.HealingPillId
		&& Empty.ShouldPassThrough()
		&& TargetSamples == 0
		&& ApexSamples == 0
		&& AuthorizationCount == 0
		&& Fixture.Adapter.GetNextSelectionOrdinal() == 1
		&& AfterPassThrough == Before);

	const FGuid ExpectedSelectionId =
		Fdemo_mapShanmenThrownWeaponInputAdapter::MakeSelectionId(
			Fixture.Correlation.CorrelationId,
			Fixture.Correlation.ActiveRunId,
			Fixture.ThrowingKnifeId,
			2,
			Before.AuthorityRevision,
			1);
	const Fdemo_mapShanmenThrownWeaponInputResult Arc = Fixture.RouteArc(
		2,
		Fixture.Source,
		TargetSamples,
		Target,
		ApexSamples,
		ApexClearance,
		AuthorizationCount);
	FShanmenItemAuthoritySnapshot AfterArc;
	Fixture.Authority->TryCaptureSnapshot(AfterArc);
	const Fdemo_mapShanmenThrownWeaponRunCommandIntent* Command =
		Fixture.Lifecycle.FindCapturedCommand(Arc.SelectionId);
	const FShanmenThrownWeaponArcRequest* ArcRequest = Command
		? &Command->GetArcPlan().GetRequest() : nullptr;
	const FVector ExpectedOrigin = Fixture.Source->GetActorLocation()
		+ Fixture.Source->GetActorForwardVector() * 55.0
		+ Fixture.Source->GetActorRightVector() * 28.0
		+ FVector::UpVector * 50.0;
	TestTrue(TEXT("Arc input samples one geometry and enters the typed lifecycle"),
		Arc.IsAccepted()
		&& Arc.TrajectoryKind
			== Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::BallisticArc
		&& !Arc.bAimSampled
		&& Arc.bTargetSampled
		&& Arc.bApexClearanceSampled
		&& Arc.SelectionOrdinal == 1
		&& Arc.SelectionId == ExpectedSelectionId
		&& Arc.ItemInstanceId == Fixture.ThrowingKnifeId
		&& Arc.Session.Product.ActivationSequence == 1
		&& Command
		&& Command->GetTrajectoryKind()
			== Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::BallisticArc
		&& ArcRequest
		&& ArcRequest->GetOrigin().Equals(ExpectedOrigin)
		&& ArcRequest->GetTarget() == Target
		&& ArcRequest->GetApexClearance() == ApexClearance
		&& ArcRequest->GetTechniqueTier()
			== EShanmenThrownWeaponTechniqueTier::Intermediate
		&& ArcRequest->GetGravityMagnitude() == 980.0
		&& ArcRequest->GetMaximumLaunchSpeed() == 900.0
		&& ArcRequest->GetMaximumFlightTime() == 4.0
		&& Fixture.Lifecycle.GetTrajectoryKind()
			== Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::BallisticArc);
	TestTrue(TEXT("Accepted Arc samples and authorizes exactly once"),
		TargetSamples == 1
		&& ApexSamples == 1
		&& AuthorizationCount == 1
		&& Fixture.Adapter.GetNextSelectionOrdinal() == 2
		&& AfterArc.AuthorityRevision == Before.AuthorityRevision + 2
		&& Fixture.Coordinator
			.GetNextPlayerThrownWeaponActivationSequence() == 2);
	FString Diagnostic;
	TestTrue(TEXT("Lifecycle owns Arc flight teardown after input routing"),
		Fixture.Lifecycle.TryEnd(Diagnostic)
		&& Fixture.Lifecycle.IsEmpty()
		&& Fixture.Coordinator.IsReady());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcInputPlanRejectionTest,
	"Shanmen.0_0_10.Product.ThrownWeaponInputAdapter.ArcPlanRejection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcInputPlanRejectionTest::RunTest(const FString&)
{
	FThrownInputFixture Fixture;
	if (!Fixture.Start(
			*this,
			TEXT("ArcPlanReject"),
			Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::BallisticArc))
	{
		return false;
	}
	FShanmenItemAuthoritySnapshot Before;
	Fixture.Authority->TryCaptureSnapshot(Before);
	int32 TargetSamples = 0;
	int32 ApexSamples = 0;
	int32 AuthorizationCount = 0;
	const Fdemo_mapShanmenThrownWeaponInputResult Rejected =
		Fixture.RouteArc(
			2,
			Fixture.Source,
			TargetSamples,
			FVector(100100.0, 200.0, 80.0),
			ApexSamples,
			160.0,
			AuthorizationCount);
	FShanmenItemAuthoritySnapshot After;
	Fixture.Authority->TryCaptureSnapshot(After);
	TestTrue(TEXT("Unreachable Arc consumes one authorized input identity"),
		Rejected.Status
			== Edemo_mapShanmenThrownWeaponInputStatus::ProductRejected
		&& Rejected.TrajectoryKind
			== Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::BallisticArc
		&& Rejected.ActionGate.IsAuthorized()
		&& Rejected.Session.Status
			== Edemo_mapShanmenThrownWeaponSessionStatus::ProductRejected
		&& Rejected.Session.Product.Status
			== Edemo_mapShanmenThrownWeaponProductStatus::ArcPlanRejected
		&& Rejected.Session.Product.HasCapturedAction()
		&& Rejected.SelectionOrdinal == 1
		&& Rejected.SelectionId.IsValid()
		&& TargetSamples == 1
		&& ApexSamples == 1
		&& AuthorizationCount == 1
		&& Fixture.Adapter.GetNextSelectionOrdinal() == 2
		&& Fixture.Coordinator
			.GetNextPlayerThrownWeaponActivationSequence() == 2);
	TestTrue(TEXT("Planning rejection performs no item or Host side effect"),
		After == Before
		&& !Fixture.Lifecycle.FindCapturedCommand(Rejected.SelectionId)
		&& Fixture.Lifecycle.GetHostState()
			== Edemo_mapShanmenThrownWeaponHostState::Empty
		&& Fixture.Lifecycle.NumCapturedSelections() == 1
		&& Fixture.Lifecycle.IsValid());
	FString Diagnostic;
	TestTrue(TEXT("Rejected Arc input leaves no hidden recovery work"),
		Fixture.Lifecycle.TryEnd(Diagnostic)
		&& Fixture.Lifecycle.IsEmpty()
		&& Fixture.Coordinator.IsReady());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcInputFailClosedTest,
	"Shanmen.0_0_10.Product.ThrownWeaponInputAdapter.ArcFailClosedGeometryAndActionConflict",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcInputFailClosedTest::RunTest(const FString&)
{
	FThrownInputFixture Fixture;
	if (!Fixture.Start(
			*this,
			TEXT("ArcFailClosed"),
			Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::BallisticArc))
	{
		return false;
	}
	FShanmenItemAuthoritySnapshot Before;
	Fixture.Authority->TryCaptureSnapshot(Before);
	int32 TargetSamples = 0;
	int32 ApexSamples = 0;
	int32 AuthorizationCount = 0;
	const auto Route = [&](
		const FVector Target,
		const double ApexClearance,
		const bool bConflict)
	{
		return Fixture.Adapter.RouteArcHotbarInput(
			Fixture.Authority,
			Fixture.Lifecycle,
			Fixture.Coordinator,
			Fixture.World,
			Ademo_mapShanmenThrownWeaponProjectile::StaticClass(),
			Fixture.Source,
			2,
			[&TargetSamples, Target]()
			{
				++TargetSamples;
				return Target;
			},
			[&ApexSamples, ApexClearance]()
			{
				++ApexSamples;
				return ApexClearance;
			},
			[&Fixture, &AuthorizationCount, bConflict]()
			{
				++AuthorizationCount;
				Fdemo_mapShanmenPlayerActionOccupancySnapshot Occupancy;
				if (bConflict)
				{
					Occupancy.TryRegisterClaim(
						Edemo_mapShanmenPlayerActionKind::SpiritEvasion,
						FGuid(0xA1162001, 0, 0, 1),
						Edemo_mapShanmenPlayerActionClaimPreemption::None);
				}
				return Fdemo_mapShanmenPlayerActionGateResult::FromArbitration(
					Fixture.Coordinator.TryAuthorizePlayerAction(
						Edemo_mapShanmenPlayerActionKind::ThrownWeapon,
						Occupancy));
			});
	};
	const FVector Origin = Fixture.Source->GetActorLocation()
		+ Fixture.Source->GetActorForwardVector() * 55.0
		+ Fixture.Source->GetActorRightVector() * 28.0
		+ FVector::UpVector * 50.0;
	const FVector ValidTarget(640.0, 250.0, 70.0);
	const Fdemo_mapShanmenThrownWeaponInputResult InvalidTarget =
		Route(Origin, 160.0, false);
	TestTrue(TEXT("Invalid Arc target stops before apex and authorization"),
		InvalidTarget.Status
			== Edemo_mapShanmenThrownWeaponInputStatus::TargetUnavailable
		&& InvalidTarget.bTargetSampled
		&& !InvalidTarget.bApexClearanceSampled
		&& !InvalidTarget.bAimSampled
		&& TargetSamples == 1
		&& ApexSamples == 0
		&& AuthorizationCount == 0
		&& Fixture.Adapter.GetNextSelectionOrdinal() == 1);

	const Fdemo_mapShanmenThrownWeaponInputResult InvalidApex =
		Route(ValidTarget, 0.0, false);
	TestTrue(TEXT("Invalid Arc apex stops before identity and authorization"),
		InvalidApex.Status
			== Edemo_mapShanmenThrownWeaponInputStatus::ApexClearanceUnavailable
		&& InvalidApex.bTargetSampled
		&& InvalidApex.bApexClearanceSampled
		&& TargetSamples == 2
		&& ApexSamples == 1
		&& AuthorizationCount == 0
		&& Fixture.Adapter.GetNextSelectionOrdinal() == 1);

	const Fdemo_mapShanmenThrownWeaponInputResult Conflict =
		Route(ValidTarget, 160.0, true);
	FShanmenItemAuthoritySnapshot After;
	Fixture.Authority->TryCaptureSnapshot(After);
	TestTrue(TEXT("Arc action conflict records geometry but consumes no event"),
		Conflict.Status
			== Edemo_mapShanmenThrownWeaponInputStatus::ActionConflict
		&& Conflict.ActionGate.IsValid()
		&& !Conflict.ActionGate.IsAuthorized()
		&& Conflict.SelectionId.IsValid()
		&& Conflict.SelectionOrdinal == 1
		&& Conflict.bTargetSampled
		&& Conflict.bApexClearanceSampled
		&& TargetSamples == 3
		&& ApexSamples == 2
		&& AuthorizationCount == 1
		&& Fixture.Adapter.GetNextSelectionOrdinal() == 1
		&& Fixture.Coordinator
			.GetNextPlayerThrownWeaponActivationSequence() == 1);
	TestTrue(TEXT("Arc pre-product failures preserve every downstream authority"),
		After == Before
		&& Fixture.Lifecycle.GetHostState()
			== Edemo_mapShanmenThrownWeaponHostState::Empty
		&& Fixture.Lifecycle.NumCapturedSelections() == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponInputFailClosedTest,
	"Shanmen.0_0_10.Product.ThrownWeaponInputAdapter.FailClosedBeforeSampling",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponInputFailClosedTest::RunTest(const FString&)
{
	FThrownInputFixture Fixture;
	if (!Fixture.Start(*this, TEXT("FailClosed")))
	{
		return false;
	}
	FShanmenItemAuthoritySnapshot Before;
	Fixture.Authority->TryCaptureSnapshot(Before);
	int32 AimSamples = 0;
	const Fdemo_mapShanmenThrownWeaponInputResult InvalidSlot = Fixture.Route(
		0, Fixture.Source, AimSamples, FVector::ForwardVector);
	const Fdemo_mapShanmenThrownWeaponInputResult ForeignSource = Fixture.Route(
		2, Fixture.OtherSource, AimSamples, FVector::ForwardVector);
	Fdemo_mapShanmenThrownWeaponProductLifecycle EmptyLifecycle;
	Fdemo_mapShanmenThrownWeaponInputAdapter EmptyAdapter;
	const Fdemo_mapShanmenThrownWeaponInputResult Unbound =
		EmptyAdapter.RouteHotbarInput(
			Fixture.Authority,
			EmptyLifecycle,
			Fixture.Coordinator,
			Fixture.World,
			Ademo_mapShanmenThrownWeaponProjectile::StaticClass(),
			Fixture.Source,
			2,
			[&AimSamples]()
			{
				++AimSamples;
				return FVector::ForwardVector;
			},
			[&Fixture]()
			{
				return Fdemo_mapShanmenPlayerActionGateResult::
					FromArbitration(
						Fixture.Coordinator.TryAuthorizePlayerAction(
							Edemo_mapShanmenPlayerActionKind::ThrownWeapon,
							Fdemo_mapShanmenPlayerActionOccupancySnapshot()));
			});
	int32 ArcTargetSamples = 0;
	int32 ArcApexSamples = 0;
	int32 ArcAuthorizationCount = 0;
	const Fdemo_mapShanmenThrownWeaponInputResult ArcMismatch =
		Fixture.Adapter.RouteArcHotbarInput(
			Fixture.Authority,
			Fixture.Lifecycle,
			Fixture.Coordinator,
			Fixture.World,
			Ademo_mapShanmenThrownWeaponProjectile::StaticClass(),
			Fixture.Source,
			2,
			[&ArcTargetSamples]()
			{
				++ArcTargetSamples;
				return FVector(640.0, 250.0, 70.0);
			},
			[&ArcApexSamples]()
			{
				++ArcApexSamples;
				return 160.0;
			},
			[&Fixture, &ArcAuthorizationCount]()
			{
				++ArcAuthorizationCount;
				return Fdemo_mapShanmenPlayerActionGateResult::
					FromArbitration(
						Fixture.Coordinator.TryAuthorizePlayerAction(
							Edemo_mapShanmenPlayerActionKind::ThrownWeapon,
							Fdemo_mapShanmenPlayerActionOccupancySnapshot()));
			});
	TestTrue(TEXT("Invalid, foreign, unbound, and policy mismatches reject before sampling"),
		InvalidSlot.Status
			== Edemo_mapShanmenThrownWeaponInputStatus::InvalidSlot
		&& ForeignSource.Status
			== Edemo_mapShanmenThrownWeaponInputStatus::SourceUnavailable
		&& Unbound.Status
			== Edemo_mapShanmenThrownWeaponInputStatus::ProductRunMismatch
		&& ArcMismatch.Status
			== Edemo_mapShanmenThrownWeaponInputStatus::ProductTrajectoryMismatch
		&& ArcMismatch.TrajectoryKind
			== Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::BallisticArc
		&& !ArcMismatch.bTargetSampled
		&& !ArcMismatch.bApexClearanceSampled
		&& AimSamples == 0
		&& ArcTargetSamples == 0
		&& ArcApexSamples == 0
		&& ArcAuthorizationCount == 0
		&& Fixture.Adapter.GetNextSelectionOrdinal() == 1);

	const Fdemo_mapShanmenThrownWeaponInputResult InvalidAim = Fixture.Route(
		2, Fixture.Source, AimSamples, FVector::ZeroVector);
	FShanmenItemAuthoritySnapshot After;
	Fixture.Authority->TryCaptureSnapshot(After);
	TestTrue(TEXT("Invalid sampled aim consumes neither identity nor authority"),
		InvalidAim.Status
			== Edemo_mapShanmenThrownWeaponInputStatus::AimUnavailable
		&& InvalidAim.bAimSampled
		&& AimSamples == 1
		&& Fixture.Adapter.GetNextSelectionOrdinal() == 1
		&& After == Before);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponInputActionConflictTest,
	"Shanmen.0_0_10.Product.ThrownWeaponInputAdapter.ActionConflict",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponInputActionConflictTest::RunTest(const FString&)
{
	FThrownInputFixture Fixture;
	if (!Fixture.Start(*this, TEXT("ActionConflict")))
	{
		return false;
	}
	FShanmenItemAuthoritySnapshot Before;
	Fixture.Authority->TryCaptureSnapshot(Before);
	int32 AimSamples = 0;
	int32 AuthorizationCount = 0;
	const Fdemo_mapShanmenThrownWeaponInputResult Rejected =
		Fixture.Adapter.RouteHotbarInput(
			Fixture.Authority,
			Fixture.Lifecycle,
			Fixture.Coordinator,
			Fixture.World,
			Ademo_mapShanmenThrownWeaponProjectile::StaticClass(),
			Fixture.Source,
			2,
			[&AimSamples]()
			{
				++AimSamples;
				return FVector::ForwardVector;
			},
			[&Fixture, &AuthorizationCount]()
			{
				++AuthorizationCount;
				Fdemo_mapShanmenPlayerActionOccupancySnapshot Occupancy;
				Occupancy.TryRegisterClaim(
					Edemo_mapShanmenPlayerActionKind::SpiritEvasion,
					FGuid(0xA1161001, 0, 0, 1),
					Edemo_mapShanmenPlayerActionClaimPreemption::None);
				return Fdemo_mapShanmenPlayerActionGateResult::
					FromArbitration(
						Fixture.Coordinator.TryAuthorizePlayerAction(
							Edemo_mapShanmenPlayerActionKind::ThrownWeapon,
							Occupancy));
			});
	FShanmenItemAuthoritySnapshot After;
	Fixture.Authority->TryCaptureSnapshot(After);
	TestTrue(TEXT("typed conflict is handled with deterministic gate evidence"),
		Rejected.Status
			== Edemo_mapShanmenThrownWeaponInputStatus::ActionConflict
			&& Rejected.ActionGate.IsValid()
			&& !Rejected.ActionGate.IsAuthorized()
			&& Rejected.SelectionId.IsValid());
	TestTrue(TEXT("classification samples and authorizes exactly once"),
		AimSamples == 1 && AuthorizationCount == 1);
	TestTrue(TEXT("conflict mutates neither selection nor item authority"),
		Fixture.Adapter.GetNextSelectionOrdinal() == 1
			&& Fixture.Lifecycle.GetHostState()
				== Edemo_mapShanmenThrownWeaponHostState::Empty
			&& After == Before);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponGameModeTrajectoryConfigurationTest,
	"Shanmen.0_0_10.Product.ThrownWeaponInputAdapter.GameModeTrajectoryConfiguration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponGameModeTrajectoryConfigurationTest::RunTest(
	const FString&)
{
	Ademo_mapGameMode* GameMode = NewObject<Ademo_mapGameMode>(
		GetTransientPackage(), NAME_None, RF_Transient);
	if (!TestNotNull(TEXT("Transient GameMode exists"), GameMode))
	{
		return false;
	}
	TestTrue(TEXT("Compatibility default remains Straight"),
		GameMode->GetConfiguredThrownWeaponTrajectoryKind()
			== Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::Straight
			&& GameMode->GetThrownWeaponInputChoiceState().IsValid()
			&& GameMode->GetThrownWeaponInputChoiceState().GetRevision() == 0);
	const FGuid InitialStateId =
		GameMode->GetThrownWeaponInputChoiceState().GetStateId();

	FString Diagnostic = TEXT("stale");
	TestFalse(TEXT("Invalid trajectory is rejected"),
		GameMode->TryConfigureThrownWeaponTrajectory(
			Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::Invalid,
			Diagnostic));
	TestTrue(TEXT("Invalid request is diagnostic and non-mutating"),
		!Diagnostic.IsEmpty()
			&& GameMode->GetConfiguredThrownWeaponTrajectoryKind()
				== Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::Straight
			&& GameMode->GetThrownWeaponInputChoiceState().GetStateId()
				== InitialStateId);

	Fdemo_mapShanmenThrownWeaponInputChoiceCommand Arc;
	if (!TestTrue(TEXT("Typed Arc selection captures at GameMode revision"),
		Fdemo_mapShanmenThrownWeaponInputChoiceCommand::
			TryCaptureTrajectorySelection(
				0,
				Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::BallisticArc,
				Arc)))
	{
		return false;
	}
	const Fdemo_mapShanmenThrownWeaponInputChoiceSessionResult Applied =
		GameMode->SubmitThrownWeaponInputChoiceCommand(Arc);
	TestTrue(TEXT("Typed Arc selection becomes the sole GameMode read model"),
		Applied.IsSuccess()
			&& Applied.DidChange()
			&& GameMode->GetConfiguredThrownWeaponTrajectoryKind()
				== Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::BallisticArc
			&& GameMode->GetThrownWeaponInputChoiceState().GetRevision() == 1
			&& GameMode->GetThrownWeaponInputChoiceState().GetLastCommandId()
				== Arc.GetCommandId()
			&& Applied.State.Matches(
				GameMode->GetThrownWeaponInputChoiceState()));
	const Fdemo_mapShanmenThrownWeaponInputChoiceSessionResult Replay =
		GameMode->SubmitThrownWeaponInputChoiceCommand(Arc);
	TestTrue(TEXT("Exact typed retry is idempotent"),
		Replay.IsSuccess()
			&& Replay.IsReplay()
			&& GameMode->GetThrownWeaponInputChoiceState().GetRevision() == 1);
	TestTrue(TEXT("Compatibility Arc selection is a fresh no-op"),
		GameMode->TryConfigureThrownWeaponTrajectory(
			Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::BallisticArc,
			Diagnostic)
			&& Diagnostic.IsEmpty()
			&& GameMode->GetThrownWeaponInputChoiceState().GetRevision() == 1);
	TestTrue(TEXT("Straight can be restored while composition is empty"),
		GameMode->TryConfigureThrownWeaponTrajectory(
			Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::Straight,
			Diagnostic)
			&& GameMode->GetConfiguredThrownWeaponTrajectoryKind()
				== Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::Straight
			&& GameMode->GetThrownWeaponInputChoiceState().GetRevision() == 2
			&& GameMode->GetThrownWeaponInputChoiceState().GetStateId()
				!= InitialStateId);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponGameModeArcRouteFailClosedTest,
	"Shanmen.0_0_10.Product.ThrownWeaponInputAdapter.GameModeArcRouteFailClosed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponGameModeArcRouteFailClosedTest::RunTest(
	const FString&)
{
	Ademo_mapGameMode* GameMode = NewObject<Ademo_mapGameMode>(
		GetTransientPackage(), NAME_None, RF_Transient);
	if (!TestNotNull(TEXT("Transient GameMode exists"), GameMode))
	{
		return false;
	}
	FString Diagnostic;
	if (!TestTrue(TEXT("Arc composition can be selected"),
		GameMode->TryConfigureThrownWeaponTrajectory(
			Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::BallisticArc,
			Diagnostic)))
	{
		return false;
	}
	int32 TargetSamples = 0;
	int32 ApexSamples = 0;
	const Fdemo_mapShanmenThrownWeaponInputResult Result =
		GameMode->RouteThrownWeaponArcHotbarInput(
			2,
			nullptr,
			[&TargetSamples]()
			{
				++TargetSamples;
				return FVector(1000.0, 0.0, 0.0);
			},
			[&ApexSamples]()
			{
				++ApexSamples;
				return 250.0;
			});
	TestTrue(TEXT("Missing GameInstance authority preserves hotbar pass-through"),
		Result.Status
			== Edemo_mapShanmenThrownWeaponInputStatus::PassThrough);
	TestTrue(TEXT("Unclaimed composition samples no target or apex"),
		TargetSamples == 0 && ApexSamples == 0);
	return true;
}

#endif
