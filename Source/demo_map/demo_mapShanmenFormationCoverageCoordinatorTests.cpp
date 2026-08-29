#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenFormationCoverageCoordinator.h"

#include "Components/SceneComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Misc/AutomationTest.h"
#include "ShanmenDeterministicId.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	const FGuid CoordinatorRunId(0xF8900001, 0, 0, 1);
	const FGuid CoordinatorOwnerId(0xF8900002, 0, 0, 1);

	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	FShanmenContentStamp CoordinatorContent(const int32 Variant)
	{
		FShanmenContentStamp Content;
		Content.Version = TEXT("0.0.10.P8.9");
		Content.Digest = FString::Printf(
			TEXT("formation-coverage-coordinator-r%d"), Variant);
		return Content;
	}

	Fdemo_mapShanmenFormationAnchorPlacementReceipt MakeCoordinatorAnchor(
		const int32 Variant,
		const int32 Ordinal,
		const FVector& WorldLocation)
	{
		Fdemo_mapShanmenFormationAnchorPlacementReceipt Receipt;
		Fdemo_mapShanmenFormationAnchorPlacementIntent& Intent = Receipt.Intent;
		Intent.RunId = CoordinatorRunId;
		Intent.OwnerId = CoordinatorOwnerId;
		Intent.DeploymentId = FGuid(0xF8900100 + Variant, 0, 0, 1);
		Intent.AnchorDefinitionId = FName(*FString::Printf(
			TEXT("Formation.Anchor.CoverageCoordinator.%d.%02d"),
			Variant, Ordinal));
		Intent.AnchorInstanceId =
			FGuid(0xF8901000 + Variant * 0x100 + Ordinal, 0, 0, 1);
		Intent.WorldLocation = WorldLocation;
		Intent.AttemptId =
			FGuid(0xF8902000 + Variant * 0x100 + Ordinal, 0, 0, 1);
		Intent.FulfillmentId =
			FGuid(0xF8903000 + Variant * 0x100 + Ordinal, 0, 0, 1);
		Intent.DeploymentReceiptId =
			FGuid(0xF8904000 + Variant * 0x100 + Ordinal, 0, 0, 1);
		Intent.AuthorityRevision = Variant * 100 + Ordinal;
		Intent.Content = CoordinatorContent(Variant);
		Intent.PlacementId = FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Formation.WorldPlacement.r1"),
			{
				GuidDigits(Intent.RunId), GuidDigits(Intent.OwnerId),
				GuidDigits(Intent.DeploymentId),
				Intent.AnchorDefinitionId.ToString(),
				GuidDigits(Intent.AnchorInstanceId),
				GuidDigits(Intent.AttemptId),
				GuidDigits(Intent.FulfillmentId),
				GuidDigits(Intent.DeploymentReceiptId),
				FString::FromInt(Intent.AuthorityRevision),
				Intent.Content.Version.ToString(), Intent.Content.Digest
			});
		Receipt.ActorClassPath = TEXT("/Script/Engine.Actor");
		Receipt.PlacementTag =
			Fdemo_mapShanmenFormationWorldAdapter::MakePlacementTag(
				Intent.PlacementId);
		Receipt.DeploymentTag =
			Fdemo_mapShanmenFormationWorldAdapter::MakeDeploymentTag(
				Intent.DeploymentId);
		Receipt.ReceiptId = FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Formation.WorldPlacementReceipt.r1"),
			{ GuidDigits(Intent.PlacementId), Receipt.ActorClassPath });
		check(Receipt.IsValid());
		return Receipt;
	}

	Fdemo_mapShanmenFormationAreaSnapshot MakeCoordinatorArea(
		const int32 Variant)
	{
		const Fdemo_mapShanmenFormationAreaBuildResult Built =
			Fdemo_mapShanmenFormationAreaProvider::BuildArea({
				MakeCoordinatorAnchor(Variant, 1, FVector(0.0, 0.0, 10.0)),
				MakeCoordinatorAnchor(Variant, 2, FVector(100.0, 0.0, 20.0)),
				MakeCoordinatorAnchor(Variant, 3, FVector(100.0, 100.0, 30.0)),
				MakeCoordinatorAnchor(Variant, 4, FVector(0.0, 100.0, 40.0))
			});
		check(Built.IsSuccess());
		return Built.Area;
	}

	struct FCoordinatorWorldFixture
	{
		UWorld* World = nullptr;
		FShanmenWorldEntityRegistry Registry;

		bool Start()
		{
			if (!GEngine)
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
			return Registry.TryBeginRun(CoordinatorRunId);
		}

		AActor* Spawn(const FVector& Location) const
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
			USceneComponent* Root = NewObject<USceneComponent>(
				Actor, NAME_None, RF_Transient);
			if (!Root)
			{
				World->DestroyActor(Actor, true, true);
				return nullptr;
			}
			Actor->SetRootComponent(Root);
			Root->SetWorldLocation(Location);
			return Actor->GetActorLocation().Equals(Location, KINDA_SMALL_NUMBER)
				? Actor : nullptr;
		}

		bool Bind(AActor* Actor, const FGuid& EntityId)
		{
			return Registry.BindObject(
				CoordinatorRunId, Actor, EntityId, INDEX_NONE)
				== EShanmenWorldBindingResult::Bound;
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
				CollectGarbage(RF_NoFlags);
			}
		}

		~FCoordinatorWorldFixture()
		{
			Stop();
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationCoverageCoordinatorPrimeAdvanceTest,
	"Shanmen.0_0_10.Product.FormationCoverageCoordinator.PrimeAdvanceReplay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationCoverageCoordinatorPrimeAdvanceTest::RunTest(
	const FString&)
{
	FCoordinatorWorldFixture Fixture;
	Fdemo_mapShanmenFormationCoverageTracker Tracker;
	if (!Fixture.Start())
	{
		return false;
	}
	AActor* Subject = Fixture.Spawn(FVector(50.0, 50.0, 0.0));
	const FGuid SubjectId(0xF8905001, 0, 0, 1);
	if (!Subject || !Fixture.Bind(Subject, SubjectId))
	{
		return false;
	}
	const Fdemo_mapShanmenFormationAreaSnapshot Area = MakeCoordinatorArea(1);
	const auto Primed = Fdemo_mapShanmenFormationCoverageCoordinator::Execute(
		Fixture.World, Area, Fixture.Registry, { Subject },
		Fdemo_mapShanmenFormationCoverageCommand::MakePrime(), Tracker);
	Fdemo_mapShanmenFormationCoverageReceipt Baseline;
	Tracker.TryGetBaseline(Baseline);
	Subject->SetActorLocation(
		FVector(150.0, 50.0, 0.0), false, nullptr,
		ETeleportType::TeleportPhysics);
	const auto Command =
		Fdemo_mapShanmenFormationCoverageCommand::MakeAdvance(
			Baseline.ReceiptId);
	const auto Advanced = Fdemo_mapShanmenFormationCoverageCoordinator::Execute(
		Fixture.World, Area, Fixture.Registry, { Subject }, Command, Tracker);
	const auto Replayed = Fdemo_mapShanmenFormationCoverageCoordinator::Execute(
		Fixture.World, Area, Fixture.Registry, { Subject }, Command, Tracker);
	Fdemo_mapShanmenFormationCoverageCoordinatorResult Tampered = Advanced;
	Tampered.TrackerResult.CurrentBaselineReceiptId.Invalidate();

	TestTrue(TEXT("Prime samples and establishes baseline"), Primed.IsSuccess());
	TestEqual(TEXT("Prime dispatch status"), Primed.TrackerResult.Status,
		Edemo_mapShanmenFormationCoverageTrackerStatus::Primed);
	TestTrue(TEXT("Movement samples and advances"), Advanced.IsSuccess());
	TestEqual(TEXT("Inside to outside emits Leave"),
		Advanced.TrackerResult.Transition.Receipt.LeftCount, 1);
	TestTrue(TEXT("Exact World command replays"), Replayed.IsSuccess());
	TestEqual(TEXT("Replay reaches tracker replay path"),
		Replayed.TrackerResult.Status,
		Edemo_mapShanmenFormationCoverageTrackerStatus::AdvanceReplayed);
	TestEqual(TEXT("Replay preserves transition identity"),
		Replayed.TrackerResult.Transition.Receipt.ReceiptId,
		Advanced.TrackerResult.Transition.Receipt.ReceiptId);
	TestFalse(TEXT("Result identity tampering is rejected"),
		Tampered.IsSuccess());
	TestEqual(TEXT("Coordinator does not mutate registry"),
		Fixture.Registry.NumObjectBindings(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationCoverageCoordinatorRebaseTest,
	"Shanmen.0_0_10.Product.FormationCoverageCoordinator.RebaseCas",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationCoverageCoordinatorRebaseTest::RunTest(const FString&)
{
	FCoordinatorWorldFixture Fixture;
	Fdemo_mapShanmenFormationCoverageTracker Tracker;
	if (!Fixture.Start())
	{
		return false;
	}
	AActor* Subject = Fixture.Spawn(FVector(50.0, 50.0, 0.0));
	if (!Subject
		|| !Fixture.Bind(Subject, FGuid(0xF8905002, 0, 0, 1)))
	{
		return false;
	}
	const Fdemo_mapShanmenFormationAreaSnapshot FirstArea =
		MakeCoordinatorArea(2);
	const Fdemo_mapShanmenFormationAreaSnapshot SecondArea =
		MakeCoordinatorArea(3);
	const auto Primed = Fdemo_mapShanmenFormationCoverageCoordinator::Execute(
		Fixture.World, FirstArea, Fixture.Registry, { Subject },
		Fdemo_mapShanmenFormationCoverageCommand::MakePrime(), Tracker);
	const FGuid FirstBaselineId =
		Primed.TrackerResult.CurrentBaselineReceiptId;
	const auto Rebased = Fdemo_mapShanmenFormationCoverageCoordinator::Execute(
		Fixture.World, SecondArea, Fixture.Registry, { Subject },
		Fdemo_mapShanmenFormationCoverageCommand::MakeRebase(FirstBaselineId),
		Tracker);
	Fdemo_mapShanmenFormationCoverageReceipt CurrentBaseline;
	Tracker.TryGetBaseline(CurrentBaseline);
	const FGuid SecondBaselineId = CurrentBaseline.ReceiptId;
	const auto Stale = Fdemo_mapShanmenFormationCoverageCoordinator::Execute(
		Fixture.World, FirstArea, Fixture.Registry, { Subject },
		Fdemo_mapShanmenFormationCoverageCommand::MakeRebase(FirstBaselineId),
		Tracker);
	Fdemo_mapShanmenFormationCoverageReceipt AfterStale;
	Tracker.TryGetBaseline(AfterStale);
	const auto Returned = Fdemo_mapShanmenFormationCoverageCoordinator::Execute(
		Fixture.World, FirstArea, Fixture.Registry, { Subject },
		Fdemo_mapShanmenFormationCoverageCommand::MakeRebase(SecondBaselineId),
		Tracker);

	TestTrue(TEXT("Initial Prime succeeds"), Primed.IsSuccess());
	TestTrue(TEXT("Explicit cross-Area Rebase succeeds"), Rebased.IsSuccess());
	TestNotEqual(TEXT("Area identity changes coverage baseline"),
		FirstBaselineId, SecondBaselineId);
	TestEqual(TEXT("Stale Rebase rejected before sampling"), Stale.Status,
		Edemo_mapShanmenFormationCoverageCoordinatorStatus::BaselineConflict);
	TestFalse(TEXT("Stale Rebase publishes no World sample"),
		Stale.Sample.IsSuccess());
	TestEqual(TEXT("Stale Rebase cannot change baseline"),
		AfterStale.ReceiptId, SecondBaselineId);
	TestTrue(TEXT("Current expected baseline permits Rebase"),
		Returned.IsSuccess());
	TestEqual(TEXT("Return to first Area restores deterministic coverage ID"),
		Returned.TrackerResult.CurrentBaselineReceiptId, FirstBaselineId);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationCoverageCoordinatorSampleFenceTest,
	"Shanmen.0_0_10.Product.FormationCoverageCoordinator.SampleFailureAtomicity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationCoverageCoordinatorSampleFenceTest::RunTest(
	const FString&)
{
	FCoordinatorWorldFixture Fixture;
	Fdemo_mapShanmenFormationCoverageTracker Tracker;
	if (!Fixture.Start())
	{
		return false;
	}
	AActor* Subject = Fixture.Spawn(FVector(50.0, 50.0, 0.0));
	AActor* Unregistered = Fixture.Spawn(FVector(25.0, 25.0, 0.0));
	if (!Subject || !Unregistered
		|| !Fixture.Bind(Subject, FGuid(0xF8905003, 0, 0, 1)))
	{
		return false;
	}
	const Fdemo_mapShanmenFormationAreaSnapshot Area = MakeCoordinatorArea(4);
	const auto Primed = Fdemo_mapShanmenFormationCoverageCoordinator::Execute(
		Fixture.World, Area, Fixture.Registry, { Subject },
		Fdemo_mapShanmenFormationCoverageCommand::MakePrime(), Tracker);
	const FGuid FirstBaselineId =
		Primed.TrackerResult.CurrentBaselineReceiptId;
	Subject->SetActorLocation(
		FVector(150.0, 50.0, 0.0), false, nullptr,
		ETeleportType::TeleportPhysics);
	const auto AdvanceCommand =
		Fdemo_mapShanmenFormationCoverageCommand::MakeAdvance(FirstBaselineId);
	const auto Advanced = Fdemo_mapShanmenFormationCoverageCoordinator::Execute(
		Fixture.World, Area, Fixture.Registry, { Subject },
		AdvanceCommand, Tracker);
	Fdemo_mapShanmenFormationCoverageReceipt AdvancedBaseline;
	Tracker.TryGetBaseline(AdvancedBaseline);
	const auto Failed = Fdemo_mapShanmenFormationCoverageCoordinator::Execute(
		Fixture.World, Area, Fixture.Registry, { Unregistered },
		Fdemo_mapShanmenFormationCoverageCommand::MakeAdvance(
			AdvancedBaseline.ReceiptId),
		Tracker);
	Fdemo_mapShanmenFormationCoverageReceipt AfterFailure;
	Tracker.TryGetBaseline(AfterFailure);
	const auto Replay = Fdemo_mapShanmenFormationCoverageCoordinator::Execute(
		Fixture.World, Area, Fixture.Registry, { Subject },
		AdvanceCommand, Tracker);

	TestTrue(TEXT("Prime and first Advance succeed"),
		Primed.IsSuccess() && Advanced.IsSuccess());
	TestEqual(TEXT("Unregistered source rejects sample"), Failed.Status,
		Edemo_mapShanmenFormationCoverageCoordinatorStatus::SampleRejected);
	TestEqual(TEXT("Nested sampler gives exact reason"), Failed.Sample.Status,
		Edemo_mapShanmenFormationWorldCoverageStatus::ActorUnregistered);
	TestEqual(TEXT("Sample failure cannot change baseline"),
		AfterFailure.ReceiptId, AdvancedBaseline.ReceiptId);
	TestTrue(TEXT("Sample failure preserves latest replay slot"),
		Replay.IsSuccess());
	TestEqual(TEXT("Old expected is allowed only for exact replay"),
		Replay.TrackerResult.Status,
		Edemo_mapShanmenFormationCoverageTrackerStatus::AdvanceReplayed);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationCoverageCoordinatorCommandFenceTest,
	"Shanmen.0_0_10.Product.FormationCoverageCoordinator.CommandFences",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationCoverageCoordinatorCommandFenceTest::RunTest(
	const FString&)
{
	FCoordinatorWorldFixture Fixture;
	Fdemo_mapShanmenFormationCoverageTracker Tracker;
	if (!Fixture.Start())
	{
		return false;
	}
	AActor* Subject = Fixture.Spawn(FVector(50.0, 50.0, 0.0));
	if (!Subject
		|| !Fixture.Bind(Subject, FGuid(0xF8905004, 0, 0, 1)))
	{
		return false;
	}
	const Fdemo_mapShanmenFormationAreaSnapshot Area = MakeCoordinatorArea(5);
	Fdemo_mapShanmenFormationCoverageCommand InvalidPrime =
		Fdemo_mapShanmenFormationCoverageCommand::MakePrime();
	InvalidPrime.ExpectedBaselineReceiptId = FGuid(0xF8909999, 0, 0, 1);
	Fdemo_mapShanmenFormationCoverageCommand Unknown;
	Unknown.Mode = static_cast<
		Edemo_mapShanmenFormationCoverageCommandMode>(255);
	const auto Invalid = Fdemo_mapShanmenFormationCoverageCoordinator::Execute(
		Fixture.World, Area, Fixture.Registry, { Subject }, InvalidPrime, Tracker);
	const auto UnknownResult =
		Fdemo_mapShanmenFormationCoverageCoordinator::Execute(
			Fixture.World, Area, Fixture.Registry, { Subject }, Unknown, Tracker);
	const FGuid Expected(0xF8909001, 0, 0, 1);
	const auto BeforeAdvance =
		Fdemo_mapShanmenFormationCoverageCoordinator::Execute(
			Fixture.World, Area, Fixture.Registry, { Subject },
			Fdemo_mapShanmenFormationCoverageCommand::MakeAdvance(Expected),
			Tracker);
	const auto BeforeRebase =
		Fdemo_mapShanmenFormationCoverageCoordinator::Execute(
			Fixture.World, Area, Fixture.Registry, { Subject },
			Fdemo_mapShanmenFormationCoverageCommand::MakeRebase(Expected),
			Tracker);
	const auto Primed = Fdemo_mapShanmenFormationCoverageCoordinator::Execute(
		Fixture.World, Area, Fixture.Registry, { Subject },
		Fdemo_mapShanmenFormationCoverageCommand::MakePrime(), Tracker);
	Fdemo_mapShanmenFormationCoverageReceipt Baseline;
	Tracker.TryGetBaseline(Baseline);
	Subject->SetActorLocation(
		FVector(150.0, 50.0, 0.0), false, nullptr,
		ETeleportType::TeleportPhysics);
	const auto SecondPrime =
		Fdemo_mapShanmenFormationCoverageCoordinator::Execute(
			Fixture.World, Area, Fixture.Registry, { Subject },
			Fdemo_mapShanmenFormationCoverageCommand::MakePrime(), Tracker);
	Fdemo_mapShanmenFormationCoverageReceipt AfterSecondPrime;
	Tracker.TryGetBaseline(AfterSecondPrime);

	TestEqual(TEXT("Prime carrying expected baseline rejected"), Invalid.Status,
		Edemo_mapShanmenFormationCoverageCoordinatorStatus::CommandInvalid);
	TestEqual(TEXT("Unknown command mode rejected"), UnknownResult.Status,
		Edemo_mapShanmenFormationCoverageCoordinatorStatus::CommandInvalid);
	TestFalse(TEXT("Invalid commands do not sample"),
		Invalid.Sample.IsSuccess() || UnknownResult.Sample.IsSuccess());
	TestEqual(TEXT("Advance before Prime rejected"), BeforeAdvance.Status,
		Edemo_mapShanmenFormationCoverageCoordinatorStatus::TrackerNotPrimed);
	TestEqual(TEXT("Rebase before Prime rejected"), BeforeRebase.Status,
		Edemo_mapShanmenFormationCoverageCoordinatorStatus::TrackerNotPrimed);
	TestFalse(TEXT("Pre-Prime state errors do not sample"),
		BeforeAdvance.Sample.IsSuccess() || BeforeRebase.Sample.IsSuccess());
	TestTrue(TEXT("Valid Prime succeeds"), Primed.IsSuccess());
	TestEqual(TEXT("Different second Prime rejected by tracker"),
		SecondPrime.Status,
		Edemo_mapShanmenFormationCoverageCoordinatorStatus::TrackerRejected);
	TestEqual(TEXT("Nested tracker reason remains visible"),
		SecondPrime.TrackerResult.Status,
		Edemo_mapShanmenFormationCoverageTrackerStatus::AlreadyPrimed);
	TestEqual(TEXT("Rejected second Prime cannot replace baseline"),
		AfterSecondPrime.ReceiptId, Baseline.ReceiptId);
	TestTrue(TEXT("Tracker remains consistent"), Tracker.IsConsistent());
	return true;
}

#endif
