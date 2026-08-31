#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenWeaponGuardWorldAdapter.h"

#include "Components/SceneComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Misc/AutomationTest.h"
#include "ShanmenCombatResolver.h"
#include "ShanmenCombatTags.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	const FGuid GuardWorldRunId(
		0xDD200001, 0xDD200002, 0xDD200003, 0xDD200004);
	const FGuid GuardWorldOwnerId(
		0xDD210001, 0xDD210002, 0xDD210003, 0xDD210004);
	const FGuid GuardWorldDefenderId(
		0xDD220001, 0xDD220002, 0xDD220003, 0xDD220004);
	const FGuid GuardWorldWeaponId(
		0xDD230001, 0xDD230002, 0xDD230003, 0xDD230004);
	const FGuid GuardWorldTimelineId(
		0xDD240001, 0xDD240002, 0xDD240003, 0xDD240004);
	const FGuid GuardWorldThreatId(
		0xDD250001, 0xDD250002, 0xDD250003, 0xDD250004);
	const FGuid GuardWorldForeignId(
		0xDD260001, 0xDD260002, 0xDD260003, 0xDD260004);

	FShanmenCombatActionSnapshot MakeGuardAction()
	{
		FShanmenCombatActionCapture Capture;
		Capture.RunId = GuardWorldRunId;
		Capture.OwnerId = GuardWorldOwnerId;
		Capture.SourceEntityId = GuardWorldDefenderId;
		Capture.SourceItemInstanceId = GuardWorldWeaponId;
		Capture.ActionDefinitionId =
			FShanmenWeaponGuardDefinition::CanonicalActionDefinitionId();
		Capture.Content.Version = TEXT("0.0.10.P11.3");
		Capture.Content.Digest = TEXT("TEST-DIGEST-P11.3-GUARD");
		Capture.SourceTags.AddTag(
			FShanmenCombatNativeTags::SourcePlayer());
		Capture.ActivationId = FShanmenCombatIdFactory::MakeActivationId(
			Capture.RunId,
			Capture.SourceEntityId,
			Capture.ActionDefinitionId,
			1130);
		FShanmenCombatActionSnapshot Action;
		check(FShanmenCombatActionSnapshot::TryCapture(Capture, Action));
		return Action;
	}

	FShanmenWeaponGuardDefinition MakeGuardDefinition()
	{
		FShanmenWeaponGuardDefinitionCapture Capture;
		Capture.ActionDefinitionId =
			FShanmenWeaponGuardDefinition::CanonicalActionDefinitionId();
		Capture.RuleId = TEXT("Defense.Sword.WeaponGuard01");
		Capture.GuardFraction = 0.25f;
		Capture.RequiredDamageTags.AddTag(
			FShanmenCombatNativeTags::DamagePhysical());
		Capture.RequiredTargetTags.AddTag(
			FShanmenCombatNativeTags::TargetLiving());
		FShanmenWeaponGuardDefinition Definition;
		check(FShanmenWeaponGuardDefinition::TryCapture(Capture, Definition));
		return Definition;
	}

	FShanmenHitCandidate MakeCandidate(
		const FGuid& SourceEntityId = GuardWorldThreatId,
		const FGuid& TargetEntityId = GuardWorldDefenderId)
	{
		FShanmenHitCandidate Candidate;
		Candidate.ActivationId = FShanmenCombatIdFactory::MakeActivationId(
			GuardWorldRunId,
			SourceEntityId,
			TEXT("Combat.Action.Test.P11_3Threat"),
			1131);
		Candidate.SourceEntityId = SourceEntityId;
		Candidate.TargetEntityId = TargetEntityId;
		Candidate.DetectorId = TEXT("Detector.Test.P11_3Threat");
		Candidate.DetectorKind = EShanmenHitDetectorKind::WeaponTrajectory;
		Candidate.HitLocation = FVector(100.0, 0.0, 5.0);
		Candidate.HitNormal = FVector::BackwardVector;
		Candidate.HitOrdinal = 0;
		check(Candidate.IsValid());
		return Candidate;
	}

	struct FGuardChain
	{
		FShanmenActionOrchestrator Runtime;
		FShanmenWeaponGuardWindow Window;
		FShanmenWeaponGuardWindowReceipt Open;
		FShanmenWeaponGuardTimingProjectionReceipt Timing;
		FShanmenWeaponGuardArcPolicy Arc;

		bool Start(int64 ObservedTick, double MinimumFacingDot = 0.5)
		{
			FShanmenActionTransitionReceipt Commit;
			const FShanmenCombatActionSnapshot Action = MakeGuardAction();
			if (!FShanmenActionOrchestrator::TryStart(
					Action, Runtime, Commit)
				|| !Runtime.TryAdvance(
					EShanmenCombatActionPhase::Startup, Commit)
				|| !FShanmenWeaponGuardWindow::TryOpen(
					Action,
					MakeGuardDefinition(),
					Commit,
					Runtime,
					Window,
					Open))
			{
				return false;
			}
			FShanmenWeaponPerfectGuardPolicy TimingPolicy;
			FShanmenWeaponGuardTimelineObservation Observation;
			return FShanmenWeaponPerfectGuardPolicy::TryCapture(
					Open,
					GuardWorldTimelineId,
					10,
					15,
					TEXT("Defense.Sword.PerfectGuard01"),
					TimingPolicy)
				&& FShanmenWeaponGuardTimelineObservation::TryCapture(
					GuardWorldTimelineId, ObservedTick, Observation)
				&& FShanmenWeaponGuardTimingEvaluator::TryProject(
					Window, Runtime, TimingPolicy, Observation, Timing)
				&& FShanmenWeaponGuardArcPolicy::TryCapture(
					Open,
					TEXT("Defense.Sword.WeaponGuardArc01"),
					MinimumFacingDot,
					Arc);
		}
	};

	struct FGuardWorld
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

		AActor* Spawn(
			const FVector& Location,
			const FRotator& Rotation = FRotator::ZeroRotator) const
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
			Root->SetWorldLocationAndRotation(
				Location, Rotation.Quaternion());
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

		~FGuardWorld()
		{
			Stop();
		}
	};

	struct FGuardWorldFixture
	{
		FGuardWorld World;
		FShanmenWorldEntityRegistry Registry;
		FGuardChain Chain;
		AActor* Defender = nullptr;
		AActor* Threat = nullptr;

		bool Start(int64 Tick = 10, double Threshold = 0.5)
		{
			if (!World.Start()
				|| !Registry.TryBeginRun(GuardWorldRunId)
				|| !Chain.Start(Tick, Threshold))
			{
				return false;
			}
			Defender = World.Spawn(FVector::ZeroVector);
			Threat = World.Spawn(FVector(100.0, 0.0, 0.0));
			return Defender
				&& Threat
				&& Registry.BindObject(
					GuardWorldRunId,
					Defender,
					GuardWorldDefenderId) ==
					EShanmenWorldBindingResult::Bound
				&& Registry.BindObject(
					GuardWorldRunId,
					Threat,
					GuardWorldThreatId) ==
					EShanmenWorldBindingResult::Bound;
		}

		Fdemo_mapShanmenWeaponGuardWorldResult Evaluate(
			const FShanmenHitCandidate& Candidate = MakeCandidate())
		{
			return Fdemo_mapShanmenWeaponGuardWorldAdapter::Evaluate(
				World.World,
				Registry,
				Defender,
				Threat,
				Chain.Window,
				Chain.Runtime,
				Chain.Arc,
				Chain.Timing,
				Candidate);
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapWeaponGuardWorldQualifiedPerfectTest,
	"Shanmen.0_0_10.Product.WeaponGuardWorldAdapter.QualifiedPerfect",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapWeaponGuardWorldQualifiedPerfectTest::RunTest(const FString&)
{
	FGuardWorldFixture Fixture;
	if (!Fixture.Start())
	{
		AddError(TEXT("Could not start P11.3 perfect World fixture."));
		return false;
	}
	const Fdemo_mapShanmenWeaponGuardWorldResult Result = Fixture.Evaluate();
	TestTrue(TEXT("Registered front threat qualifies perfect guard"),
		Result.IsSuccess()
			&& Result.IsQualified()
			&& Result.RunId == GuardWorldRunId
			&& Result.DefenderEntityId == GuardWorldDefenderId
			&& Result.ThreatEntityId == GuardWorldThreatId
			&& Result.Sample.GetGuardFacing() == FVector::ForwardVector
			&& Result.Sample.GetDirectionToThreat() == FVector::ForwardVector
			&& Result.Evaluation.GetLayer().LayerTags.HasTagExact(
				FShanmenCombatNativeTags::DefensePerfectGuard()));
	TestEqual(TEXT("Sampling does not mutate registry"),
		Fixture.Registry.NumObjectBindings(), 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapWeaponGuardWorldLiveTransformsTest,
	"Shanmen.0_0_10.Product.WeaponGuardWorldAdapter.LiveTransforms",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapWeaponGuardWorldLiveTransformsTest::RunTest(const FString&)
{
	FGuardWorldFixture Fixture;
	if (!Fixture.Start(15))
	{
		return false;
	}
	const Fdemo_mapShanmenWeaponGuardWorldResult Front = Fixture.Evaluate();
	Fixture.Threat->SetActorLocation(
		FVector(-100.0, 0.0, 0.0),
		false,
		nullptr,
		ETeleportType::TeleportPhysics);
	const Fdemo_mapShanmenWeaponGuardWorldResult Rear = Fixture.Evaluate();
	const Fdemo_mapShanmenWeaponGuardWorldResult Replay = Fixture.Evaluate();

	TestTrue(TEXT("Front ordinary layer is preserved"),
		Front.IsQualified()
			&& Front.Evaluation.GetTimingProjection().GetBand()
				== EShanmenWeaponGuardTimingBand::Ordinary
			&& Front.Evaluation.GetLayer().LayerTags.HasTagExact(
				FShanmenCombatNativeTags::DefenseGuard()));
	TestTrue(TEXT("Moved rear threat yields auditable no-layer result"),
		Rear.IsSuccess()
			&& !Rear.IsQualified()
			&& Rear.Evaluation.GetStatus()
				== EShanmenWeaponGuardArcStatus::OutsideArc
			&& !Rear.Evaluation.HasLayer());
	TestNotEqual(TEXT("Live movement changes World receipt"),
		Front.ReceiptId, Rear.ReceiptId);
	TestEqual(TEXT("Unchanged World sample replays exactly"),
		Rear.ReceiptId, Replay.ReceiptId);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapWeaponGuardWorldIdentityFencesTest,
	"Shanmen.0_0_10.Product.WeaponGuardWorldAdapter.IdentityFences",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapWeaponGuardWorldIdentityFencesTest::RunTest(const FString&)
{
	FGuardWorldFixture Fixture;
	if (!Fixture.Start())
	{
		return false;
	}
	TestEqual(TEXT("Foreign candidate target rejected"),
		Fixture.Evaluate(MakeCandidate(
			GuardWorldThreatId, GuardWorldForeignId)).Status,
		Edemo_mapShanmenWeaponGuardWorldStatus::DefenderIdentityMismatch);
	TestEqual(TEXT("Foreign candidate source rejected"),
		Fixture.Evaluate(MakeCandidate(
			GuardWorldForeignId, GuardWorldDefenderId)).Status,
		Edemo_mapShanmenWeaponGuardWorldStatus::ThreatIdentityMismatch);

	Fixture.Registry.UnbindObject(GuardWorldRunId, Fixture.Threat);
	TestEqual(TEXT("Unregistered threat rejected"), Fixture.Evaluate().Status,
		Edemo_mapShanmenWeaponGuardWorldStatus::ThreatUnregistered);
	Fixture.Registry.BindObject(
		GuardWorldRunId, Fixture.Threat, GuardWorldDefenderId);
	const Fdemo_mapShanmenWeaponGuardWorldResult Alias = Fixture.Evaluate();
	TestEqual(TEXT("Actor aliases cannot form self threat"), Alias.Status,
		Edemo_mapShanmenWeaponGuardWorldStatus::IdentityConflict);
	TestFalse(TEXT("Identity rejection publishes no partial receipt"),
		Alias.ReceiptId.IsValid() || Alias.Sample.IsValid()
			|| Alias.Evaluation.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapWeaponGuardWorldEnvironmentFencesTest,
	"Shanmen.0_0_10.Product.WeaponGuardWorldAdapter.EnvironmentFences",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapWeaponGuardWorldEnvironmentFencesTest::RunTest(const FString&)
{
	FGuardWorldFixture Fixture;
	FGuardWorld ForeignWorld;
	if (!Fixture.Start() || !ForeignWorld.Start())
	{
		return false;
	}
	const Fdemo_mapShanmenWeaponGuardWorldResult NullWorld =
		Fdemo_mapShanmenWeaponGuardWorldAdapter::Evaluate(
			nullptr,
			Fixture.Registry,
			Fixture.Defender,
			Fixture.Threat,
			Fixture.Chain.Window,
			Fixture.Chain.Runtime,
			Fixture.Chain.Arc,
			Fixture.Chain.Timing,
			MakeCandidate());
	AActor* ForeignThreat = ForeignWorld.Spawn(FVector(100.0, 0.0, 0.0));
	const Fdemo_mapShanmenWeaponGuardWorldResult CrossWorld =
		Fdemo_mapShanmenWeaponGuardWorldAdapter::Evaluate(
			Fixture.World.World,
			Fixture.Registry,
			Fixture.Defender,
			ForeignThreat,
			Fixture.Chain.Window,
			Fixture.Chain.Runtime,
			Fixture.Chain.Arc,
			Fixture.Chain.Timing,
			MakeCandidate());
	Fixture.Threat->SetActorLocation(FVector::ZeroVector);
	const Fdemo_mapShanmenWeaponGuardWorldResult Coincident =
		Fixture.Evaluate();

	FShanmenWorldEntityRegistry InactiveRegistry;
	const Fdemo_mapShanmenWeaponGuardWorldResult Inactive =
		Fdemo_mapShanmenWeaponGuardWorldAdapter::Evaluate(
			Fixture.World.World,
			InactiveRegistry,
			Fixture.Defender,
			Fixture.Threat,
			Fixture.Chain.Window,
			Fixture.Chain.Runtime,
			Fixture.Chain.Arc,
			Fixture.Chain.Timing,
			MakeCandidate());

	TestEqual(TEXT("Null World rejected"), NullWorld.Status,
		Edemo_mapShanmenWeaponGuardWorldStatus::WorldInvalid);
	TestEqual(TEXT("Cross-World threat rejected"), CrossWorld.Status,
		Edemo_mapShanmenWeaponGuardWorldStatus::ActorWorldMismatch);
	TestEqual(TEXT("Coincident actors have no threat direction"),
		Coincident.Status,
		Edemo_mapShanmenWeaponGuardWorldStatus::DirectionSampleRejected);
	TestEqual(TEXT("Inactive registry rejected"), Inactive.Status,
		Edemo_mapShanmenWeaponGuardWorldStatus::RegistryInactive);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapWeaponGuardWorldLifecycleReplayTest,
	"Shanmen.0_0_10.Product.WeaponGuardWorldAdapter.LifecycleReplay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapWeaponGuardWorldLifecycleReplayTest::RunTest(const FString&)
{
	FGuardWorldFixture Fixture;
	if (!Fixture.Start())
	{
		return false;
	}
	const Fdemo_mapShanmenWeaponGuardWorldResult First = Fixture.Evaluate();
	const Fdemo_mapShanmenWeaponGuardWorldResult Replay = Fixture.Evaluate();
	TestTrue(TEXT("Exact World sample replays deterministic receipt"),
		First.IsSuccess()
			&& Replay.IsSuccess()
			&& First.ReceiptId == Replay.ReceiptId
			&& First.Sample.GetSampleId() == Replay.Sample.GetSampleId()
			&& First.Evaluation.GetEvaluationId()
				== Replay.Evaluation.GetEvaluationId());

	FShanmenActionTransitionReceipt Recovery;
	check(Fixture.Chain.Runtime.TryAdvance(
		EShanmenCombatActionPhase::Active, Recovery));
	const Fdemo_mapShanmenWeaponGuardWorldResult Closed = Fixture.Evaluate();
	TestEqual(TEXT("Recovery closes World direction sampling before publish"),
		Closed.Status,
		Edemo_mapShanmenWeaponGuardWorldStatus::ArcBindingRejected);
	TestFalse(TEXT("Closed lifecycle publishes no partial World receipt"),
		Closed.ReceiptId.IsValid() || Closed.Sample.IsValid()
			|| Closed.Evaluation.IsValid());
	return true;
}

#endif
