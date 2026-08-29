#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenFormationWorldCoverageSampler.h"

#include "Components/SceneComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Misc/AutomationTest.h"
#include "ShanmenDeterministicId.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	const FGuid CoverageRunId(0xF8600001, 0, 0, 1);
	const FGuid CoverageOwnerId(0xF8600002, 0, 0, 1);
	const FGuid CoverageDeploymentId(0xF8600003, 0, 0, 1);
	const FGuid CoverageInsideId(0xF8601001, 0, 0, 1);
	const FGuid CoverageBoundaryId(0xF8601002, 0, 0, 1);
	const FGuid CoverageOutsideId(0xF8601003, 0, 0, 1);

	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	FShanmenContentStamp CoverageContent()
	{
		FShanmenContentStamp Content;
		Content.Version = TEXT("0.0.10.P8.6");
		Content.Digest = TEXT("formation-world-coverage-r0");
		return Content;
	}

	Fdemo_mapShanmenFormationAnchorPlacementReceipt MakeCoverageAnchor(
		const int32 Ordinal,
		const FVector& WorldLocation)
	{
		Fdemo_mapShanmenFormationAnchorPlacementReceipt Receipt;
		Fdemo_mapShanmenFormationAnchorPlacementIntent& Intent = Receipt.Intent;
		Intent.RunId = CoverageRunId;
		Intent.OwnerId = CoverageOwnerId;
		Intent.DeploymentId = CoverageDeploymentId;
		Intent.AnchorDefinitionId = FName(*FString::Printf(
			TEXT("Formation.Anchor.WorldCoverage.%02d"), Ordinal));
		Intent.AnchorInstanceId = FGuid(0xF8600100 + Ordinal, 0, 0, 1);
		Intent.WorldLocation = WorldLocation;
		Intent.AttemptId = FGuid(0xF8600200 + Ordinal, 0, 0, 1);
		Intent.FulfillmentId = FGuid(0xF8600300 + Ordinal, 0, 0, 1);
		Intent.DeploymentReceiptId = FGuid(0xF8600400 + Ordinal, 0, 0, 1);
		Intent.AuthorityRevision = 20 + Ordinal;
		Intent.Content = CoverageContent();
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

	Fdemo_mapShanmenFormationAreaSnapshot MakeCoverageArea()
	{
		const Fdemo_mapShanmenFormationAreaBuildResult Built =
			Fdemo_mapShanmenFormationAreaProvider::BuildArea({
				MakeCoverageAnchor(1, FVector(0.0, 0.0, 10.0)),
				MakeCoverageAnchor(2, FVector(100.0, 0.0, 20.0)),
				MakeCoverageAnchor(3, FVector(100.0, 100.0, 30.0)),
				MakeCoverageAnchor(4, FVector(0.0, 100.0, 40.0))
			});
		check(Built.IsSuccess());
		return Built.Area;
	}

	struct FWorldCoverageFixture
	{
		UWorld* World = nullptr;

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
			return true;
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
				AActor::StaticClass(),
				FTransform::Identity,
				Parameters);
			if (!Actor)
			{
				return nullptr;
			}

			// AActor itself has no root component, so its spawn transform cannot
			// represent a live World location.  Give the test actor the smallest
			// possible transform root; registration is unnecessary because this
			// fixture never ticks or renders the component.
			USceneComponent* Root = NewObject<USceneComponent>(
				Actor, TEXT("P86CoverageRoot"), RF_Transient);
			if (!Root)
			{
				World->DestroyActor(Actor, true, true);
				return nullptr;
			}
			Actor->SetRootComponent(Root);
			Root->SetWorldLocation(Location);
			if (!Actor->GetActorLocation().Equals(Location, KINDA_SMALL_NUMBER))
			{
				World->DestroyActor(Actor, true, true);
				return nullptr;
			}
			return Actor;
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

		~FWorldCoverageFixture()
		{
			Stop();
		}
	};

	bool Bind(
		FShanmenWorldEntityRegistry& Registry,
		AActor* Actor,
		const FGuid& EntityId)
	{
		return Registry.BindObject(
			CoverageRunId, Actor, EntityId, INDEX_NONE)
			== EShanmenWorldBindingResult::Bound;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationWorldCoverageCanonicalSampleTest,
	"Shanmen.0_0_10.Product.FormationWorldCoverage.CanonicalSample",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationWorldCoverageCanonicalSampleTest::RunTest(
	const FString&)
{
	FWorldCoverageFixture Fixture;
	FShanmenWorldEntityRegistry Registry;
	if (!Fixture.Start() || !Registry.TryBeginRun(CoverageRunId))
	{
		AddError(TEXT("Could not start P8.6 canonical World fixture."));
		return false;
	}
	AActor* Inside = Fixture.Spawn(FVector(50.0, 50.0, 900.0));
	AActor* Boundary = Fixture.Spawn(FVector(0.0, 75.0, -900.0));
	AActor* Outside = Fixture.Spawn(FVector(-1.0, 50.0, 0.0));
	if (!Inside || !Boundary || !Outside
		|| !Bind(Registry, Inside, CoverageInsideId)
		|| !Bind(Registry, Boundary, CoverageBoundaryId)
		|| !Bind(Registry, Outside, CoverageOutsideId))
	{
		AddError(TEXT("Could not bind P8.6 canonical entities."));
		return false;
	}

	const Fdemo_mapShanmenFormationAreaSnapshot Area = MakeCoverageArea();
	const Fdemo_mapShanmenFormationWorldCoverageResult Sample =
		Fdemo_mapShanmenFormationWorldCoverageSampler::Sample(
			Fixture.World, Area, Registry, { Outside, Boundary, Inside });
	const Fdemo_mapShanmenFormationWorldCoverageResult Replay =
		Fdemo_mapShanmenFormationWorldCoverageSampler::Sample(
			Fixture.World, Area, Registry, { Inside, Outside, Boundary });
	TestTrue(TEXT("Registered Actor subset samples successfully"),
		Sample.IsSuccess() && Replay.IsSuccess());
	TestEqual(TEXT("Canonical entity order starts with Inside"),
		Sample.Queries[0].SubjectEntityId, CoverageInsideId);
	TestEqual(TEXT("Inside count"), Sample.Coverage.Receipt.InsideCount, 1);
	TestEqual(TEXT("Boundary count"),
		Sample.Coverage.Receipt.BoundaryCount, 1);
	TestEqual(TEXT("Outside count"),
		Sample.Coverage.Receipt.OutsideCount, 1);
	TestEqual(TEXT("Source order cannot alter coverage identity"),
		Sample.Coverage.Receipt.ReceiptId,
		Replay.Coverage.Receipt.ReceiptId);
	TestEqual(TEXT("Sampling does not mutate registry"),
		Registry.NumObjectBindings(), 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationWorldCoverageLiveLocationTest,
	"Shanmen.0_0_10.Product.FormationWorldCoverage.LiveLocationReplay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationWorldCoverageLiveLocationTest::RunTest(const FString&)
{
	FWorldCoverageFixture Fixture;
	FShanmenWorldEntityRegistry Registry;
	if (!Fixture.Start() || !Registry.TryBeginRun(CoverageRunId))
	{
		return false;
	}
	AActor* Actor = Fixture.Spawn(FVector(50.0, 50.0, 0.0));
	if (!Actor || !Bind(Registry, Actor, CoverageInsideId))
	{
		return false;
	}
	const Fdemo_mapShanmenFormationAreaSnapshot Area = MakeCoverageArea();
	const Fdemo_mapShanmenFormationWorldCoverageResult Inside =
		Fdemo_mapShanmenFormationWorldCoverageSampler::Sample(
			Fixture.World, Area, Registry, { Actor });
	Actor->SetActorLocation(FVector(150.0, 50.0, 0.0), false, nullptr,
		ETeleportType::TeleportPhysics);
	const Fdemo_mapShanmenFormationWorldCoverageResult Outside =
		Fdemo_mapShanmenFormationWorldCoverageSampler::Sample(
			Fixture.World, Area, Registry, { Actor });
	const Fdemo_mapShanmenFormationWorldCoverageResult Replay =
		Fdemo_mapShanmenFormationWorldCoverageSampler::Sample(
			Fixture.World, Area, Registry, { Actor });

	TestTrue(TEXT("Both live locations sample"),
		Inside.IsSuccess() && Outside.IsSuccess() && Replay.IsSuccess());
	TestEqual(TEXT("First location is inside"),
		Inside.Coverage.Receipt.InsideCount, 1);
	TestEqual(TEXT("Moved location is outside"),
		Outside.Coverage.Receipt.OutsideCount, 1);
	TestNotEqual(TEXT("Movement changes immutable coverage identity"),
		Inside.Coverage.Receipt.ReceiptId,
		Outside.Coverage.Receipt.ReceiptId);
	TestEqual(TEXT("Unchanged exact sample replays identity"),
		Outside.Coverage.Receipt.ReceiptId,
		Replay.Coverage.Receipt.ReceiptId);
	TestEqual(TEXT("Area identity remains immutable"),
		Inside.Coverage.Receipt.AreaId, Area.AreaId);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationWorldCoverageIdentityFenceTest,
	"Shanmen.0_0_10.Product.FormationWorldCoverage.IdentityFences",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationWorldCoverageIdentityFenceTest::RunTest(const FString&)
{
	FWorldCoverageFixture Fixture;
	if (!Fixture.Start())
	{
		return false;
	}
	AActor* First = Fixture.Spawn(FVector(10.0, 10.0, 0.0));
	AActor* Alias = Fixture.Spawn(FVector(20.0, 20.0, 0.0));
	if (!First || !Alias)
	{
		return false;
	}
	const Fdemo_mapShanmenFormationAreaSnapshot Area = MakeCoverageArea();
	FShanmenWorldEntityRegistry Inactive;
	const Fdemo_mapShanmenFormationWorldCoverageResult InactiveResult =
		Fdemo_mapShanmenFormationWorldCoverageSampler::Sample(
			Fixture.World, Area, Inactive, { First });
	FShanmenWorldEntityRegistry WrongRun;
	WrongRun.TryBeginRun(FGuid(0xF8609999, 0, 0, 1));
	const Fdemo_mapShanmenFormationWorldCoverageResult WrongRunResult =
		Fdemo_mapShanmenFormationWorldCoverageSampler::Sample(
			Fixture.World, Area, WrongRun, { First });
	FShanmenWorldEntityRegistry Registry;
	Registry.TryBeginRun(CoverageRunId);
	const Fdemo_mapShanmenFormationWorldCoverageResult Empty =
		Fdemo_mapShanmenFormationWorldCoverageSampler::Sample(
			Fixture.World, Area, Registry, {});
	const Fdemo_mapShanmenFormationWorldCoverageResult Missing =
		Fdemo_mapShanmenFormationWorldCoverageSampler::Sample(
			Fixture.World, Area, Registry, { First });
	const Fdemo_mapShanmenFormationWorldCoverageResult NullActor =
		Fdemo_mapShanmenFormationWorldCoverageSampler::Sample(
			Fixture.World, Area, Registry, { nullptr });
	Bind(Registry, First, CoverageInsideId);
	Bind(Registry, Alias, CoverageInsideId);
	const Fdemo_mapShanmenFormationWorldCoverageResult Duplicate =
		Fdemo_mapShanmenFormationWorldCoverageSampler::Sample(
			Fixture.World, Area, Registry, { First, Alias });

	TestEqual(TEXT("Inactive registry rejected"), InactiveResult.Status,
		Edemo_mapShanmenFormationWorldCoverageStatus::RegistryInactive);
	TestEqual(TEXT("Cross-Run registry rejected"), WrongRunResult.Status,
		Edemo_mapShanmenFormationWorldCoverageStatus::RunMismatch);
	TestEqual(TEXT("Implicit discovery is forbidden"), Empty.Status,
		Edemo_mapShanmenFormationWorldCoverageStatus::SourceSetEmpty);
	TestEqual(TEXT("Unregistered Actor rejected"), Missing.Status,
		Edemo_mapShanmenFormationWorldCoverageStatus::ActorUnregistered);
	TestEqual(TEXT("Null Actor rejected"), NullActor.Status,
		Edemo_mapShanmenFormationWorldCoverageStatus::ActorUnavailable);
	TestEqual(TEXT("Two Actors aliasing one entity rejected"), Duplicate.Status,
		Edemo_mapShanmenFormationWorldCoverageStatus::DuplicateEntity);
	TestFalse(TEXT("Rejected sample publishes no partial coverage"),
		Duplicate.Coverage.IsSuccess());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapFormationWorldCoverageWorldFenceTest,
	"Shanmen.0_0_10.Product.FormationWorldCoverage.WorldAndAreaFences",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapFormationWorldCoverageWorldFenceTest::RunTest(const FString&)
{
	FWorldCoverageFixture FirstWorld;
	FWorldCoverageFixture SecondWorld;
	FShanmenWorldEntityRegistry Registry;
	if (!FirstWorld.Start() || !SecondWorld.Start()
		|| !Registry.TryBeginRun(CoverageRunId))
	{
		return false;
	}
	AActor* Actor = SecondWorld.Spawn(FVector(50.0, 50.0, 0.0));
	if (!Actor || !Bind(Registry, Actor, CoverageInsideId))
	{
		return false;
	}
	const Fdemo_mapShanmenFormationAreaSnapshot Area = MakeCoverageArea();
	Fdemo_mapShanmenFormationAreaSnapshot InvalidArea = Area;
	InvalidArea.AreaId.Invalidate();
	const Fdemo_mapShanmenFormationWorldCoverageResult InvalidAreaResult =
		Fdemo_mapShanmenFormationWorldCoverageSampler::Sample(
			SecondWorld.World, InvalidArea, Registry, { Actor });
	const Fdemo_mapShanmenFormationWorldCoverageResult InvalidWorld =
		Fdemo_mapShanmenFormationWorldCoverageSampler::Sample(
			nullptr, Area, Registry, { Actor });
	const Fdemo_mapShanmenFormationWorldCoverageResult CrossWorld =
		Fdemo_mapShanmenFormationWorldCoverageSampler::Sample(
			FirstWorld.World, Area, Registry, { Actor });
	SecondWorld.World->DestroyActor(Actor, true, true);
	const Fdemo_mapShanmenFormationWorldCoverageResult Destroyed =
		Fdemo_mapShanmenFormationWorldCoverageSampler::Sample(
			SecondWorld.World, Area, Registry, { Actor });

	TestEqual(TEXT("Invalid area rejected first"), InvalidAreaResult.Status,
		Edemo_mapShanmenFormationWorldCoverageStatus::AreaInvalid);
	TestEqual(TEXT("Null World rejected"), InvalidWorld.Status,
		Edemo_mapShanmenFormationWorldCoverageStatus::WorldInvalid);
	TestEqual(TEXT("Cross-World Actor rejected"), CrossWorld.Status,
		Edemo_mapShanmenFormationWorldCoverageStatus::ActorWorldMismatch);
	TestEqual(TEXT("Destroyed Actor rejected"), Destroyed.Status,
		Edemo_mapShanmenFormationWorldCoverageStatus::ActorUnavailable);
	return true;
}

#endif
