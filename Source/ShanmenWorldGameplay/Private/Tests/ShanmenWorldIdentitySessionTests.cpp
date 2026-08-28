#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ShanmenCombatResolver.h"
#include "ShanmenDetectorEmissionSession.h"
#include "ShanmenWorldEntityRegistry.h"

#include "Components/BoxComponent.h"
#include "Engine/HitResult.h"

namespace
{
	const FGuid RegistryRunId(0x52100001, 0, 0, 1);
	const FGuid ForeignRunId(0x52100002, 0, 0, 1);

	FShanmenCombatActionSnapshot MakeP21Action(const FGuid& RunId = RegistryRunId)
	{
		FShanmenCombatActionCapture Capture;
		Capture.RunId = RunId;
		Capture.OwnerId = FGuid(5, 6, 7, 8);
		Capture.SourceEntityId = FShanmenWorldEntityIdFactory::MakeEntityId(
			RunId, TEXT("Spawn.Player"), 0);
		Capture.ActionDefinitionId = TEXT("Combat.Action.Sword.Basic01");
		Capture.Content.Version = TEXT("0.0.10.P2.1");
		Capture.Content.Digest = TEXT("TEST-DIGEST-P2.1");
		Capture.ActivationId = FShanmenCombatIdFactory::MakeActivationId(
			Capture.RunId,
			Capture.SourceEntityId,
			Capture.ActionDefinitionId,
			1);

		FShanmenCombatActionSnapshot Action;
		check(FShanmenCombatActionSnapshot::TryCapture(Capture, Action));
		return Action;
	}

	FHitResult MakeComponentHit(UPrimitiveComponent* Component, int32 BodyIndex)
	{
		FHitResult Hit(nullptr, Component, FVector(10.0, 20.0, 30.0), FVector::UpVector);
		Hit.ImpactPoint = FVector(10.0, 20.0, 30.0);
		Hit.ImpactNormal = FVector::UpVector;
		Hit.Item = BodyIndex;
		return Hit;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenWorldEntityIdFactoryTest,
	"Shanmen.0_0_10.WorldGameplay.EntityIdFactory",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenWorldEntityIdFactoryTest::RunTest(const FString&)
{
	const FGuid First = FShanmenWorldEntityIdFactory::MakeEntityId(
		RegistryRunId, TEXT("Spawn.Enemy.Wolf"), 3);
	const FGuid Replay = FShanmenWorldEntityIdFactory::MakeEntityId(
		RegistryRunId, TEXT("Spawn.Enemy.Wolf"), 3);
	const FGuid DifferentOrdinal = FShanmenWorldEntityIdFactory::MakeEntityId(
		RegistryRunId, TEXT("Spawn.Enemy.Wolf"), 4);
	const FGuid DifferentRun = FShanmenWorldEntityIdFactory::MakeEntityId(
		ForeignRunId, TEXT("Spawn.Enemy.Wolf"), 3);

	TestTrue(TEXT("Explicit spawn identity produces a valid EntityId"), First.IsValid());
	TestTrue(TEXT("Same Run and spawn tuple replays the EntityId"), First == Replay);
	TestTrue(TEXT("Spawn ordinal participates in identity"), First != DifferentOrdinal);
	TestTrue(TEXT("Run identity namespaces world entities"), First != DifferentRun);
	TestFalse(TEXT("Invalid Run fails closed"),
		FShanmenWorldEntityIdFactory::MakeEntityId(FGuid(), TEXT("Spawn.Enemy.Wolf"), 3).IsValid());
	TestFalse(TEXT("Negative spawn ordinal fails closed"),
		FShanmenWorldEntityIdFactory::MakeEntityId(RegistryRunId, TEXT("Spawn.Enemy.Wolf"), -1).IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenWorldEntityRegistryTest,
	"Shanmen.0_0_10.WorldGameplay.EntityRegistry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenWorldEntityRegistryTest::RunTest(const FString&)
{
	FShanmenWorldEntityRegistry Registry;
	UBoxComponent* WorldObject = NewObject<UBoxComponent>();
	const FGuid EntityId = FShanmenWorldEntityIdFactory::MakeEntityId(
		RegistryRunId, TEXT("Spawn.Prop.Target"), 0);
	const FGuid ConflictingId = FShanmenWorldEntityIdFactory::MakeEntityId(
		RegistryRunId, TEXT("Spawn.Prop.Target"), 1);

	TestTrue(TEXT("Valid Run opens the registry"), Registry.TryBeginRun(RegistryRunId));
	TestTrue(TEXT("Opening the same Run is idempotent"), Registry.TryBeginRun(RegistryRunId));
	TestFalse(TEXT("A live registry cannot silently switch Run"), Registry.TryBeginRun(ForeignRunId));
	TestTrue(TEXT("Spawner binds an explicit EntityId"),
		Registry.BindObject(RegistryRunId, WorldObject, EntityId) == EShanmenWorldBindingResult::Bound);
	TestTrue(TEXT("Identical binding is idempotent"),
		Registry.BindObject(RegistryRunId, WorldObject, EntityId) == EShanmenWorldBindingResult::AlreadyBound);
	TestTrue(TEXT("One world object cannot change EntityId"),
		Registry.BindObject(RegistryRunId, WorldObject, ConflictingId) == EShanmenWorldBindingResult::Conflict);
	TestTrue(TEXT("Foreign Run cannot bind into active registry"),
		Registry.BindObject(ForeignRunId, WorldObject, EntityId) == EShanmenWorldBindingResult::RunMismatch);

	FGuid Resolved;
	TestTrue(TEXT("Exact Run resolves the world object"),
		Registry.TryResolveObject(RegistryRunId, WorldObject, INDEX_NONE, Resolved));
	TestTrue(TEXT("Resolved identity matches injected identity"), Resolved == EntityId);
	TestFalse(TEXT("Foreign Run cannot resolve the object"),
		Registry.TryResolveObject(ForeignRunId, WorldObject, INDEX_NONE, Resolved));
	TestTrue(TEXT("Foreign Run cannot unbind the object"),
		Registry.UnbindObject(ForeignRunId, WorldObject) == EShanmenWorldBindingResult::RunMismatch);
	TestTrue(TEXT("Exact Run removes the object binding"),
		Registry.UnbindObject(RegistryRunId, WorldObject) == EShanmenWorldBindingResult::Removed);
	TestTrue(TEXT("Repeated unbind reports not found"),
		Registry.UnbindObject(RegistryRunId, WorldObject) == EShanmenWorldBindingResult::NotFound);
	TestFalse(TEXT("Removed binding no longer resolves"),
		Registry.TryResolveObject(RegistryRunId, WorldObject, INDEX_NONE, Resolved));
	TestTrue(TEXT("Spawner may explicitly rebind the same live object"),
		Registry.BindObject(RegistryRunId, WorldObject, EntityId) == EShanmenWorldBindingResult::Bound);
	TestFalse(TEXT("Foreign Run cannot end the registry"), Registry.TryEndRun(ForeignRunId));
	TestTrue(TEXT("Exact Run end clears the registry"), Registry.TryEndRun(RegistryRunId));
	TestEqual(TEXT("Run end clears every object binding"), Registry.NumObjectBindings(), 0);
	TestFalse(TEXT("Ended registry no longer resolves identity"),
		Registry.TryResolveObject(RegistryRunId, WorldObject, INDEX_NONE, Resolved));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenWorldRegistryAdapterIntegrationTest,
	"Shanmen.0_0_10.WorldGameplay.RegistryAdapterIntegration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenWorldRegistryAdapterIntegrationTest::RunTest(const FString&)
{
	FShanmenWorldEntityRegistry Registry;
	TestTrue(TEXT("Registry starts for integration Run"), Registry.TryBeginRun(RegistryRunId));

	UBoxComponent* HitComponent = NewObject<UBoxComponent>();
	const FGuid TargetEntityId = FShanmenWorldEntityIdFactory::MakeEntityId(
		RegistryRunId, TEXT("Spawn.Enemy.Wolf"), 0);
	TestTrue(TEXT("Spawner binds exact physics body"),
		Registry.BindObject(RegistryRunId, HitComponent, TargetEntityId, 7)
			== EShanmenWorldBindingResult::Bound);
	TestTrue(TEXT("Same object may add a generic alias only for the same entity"),
		Registry.BindObject(RegistryRunId, HitComponent, TargetEntityId)
			== EShanmenWorldBindingResult::Bound);

	FShanmenDetectorEmissionSession Session;
	TestTrue(TEXT("Detector session starts from frozen action"),
		FShanmenDetectorEmissionSession::TryStart(
			MakeP21Action(),
			TEXT("Detector.Weapon.Main"),
			EShanmenHitDetectorKind::WeaponTrajectory,
			Session));

	FShanmenWorldHitContext Context;
	TestTrue(TEXT("First detector emission opens"), Session.TryBeginEmission(Context));
	const FHitResult Hit = MakeComponentHit(HitComponent, 7);
	FShanmenHitCandidate Candidate;
	TestTrue(TEXT("P2.0 adapter resolves P2.1 registry identity"),
		FShanmenWorldHitAdapter::TryFromSweep(Context, Hit, Registry, Candidate));
	TestTrue(TEXT("Candidate carries the registered target"), Candidate.TargetEntityId == TargetEntityId);
	TestTrue(TEXT("Session accepts first target contact"), Session.TryAcceptCandidate(Candidate));
	TestFalse(TEXT("Duplicate component callback for same target is rejected"),
		Session.TryAcceptCandidate(Candidate));
	TestTrue(TEXT("Emission closes explicitly"), Session.TryEndEmission());

	FShanmenWorldHitContext ForeignContext;
	TestTrue(TEXT("Foreign action can still form a geometric context"),
		FShanmenWorldHitContext::TryCreate(
			MakeP21Action(ForeignRunId),
			TEXT("Detector.Weapon.Main"),
			EShanmenHitDetectorKind::WeaponTrajectory,
			0,
			ForeignContext));
	TestFalse(TEXT("Registry refuses cross-Run adapter resolution"),
		FShanmenWorldHitAdapter::TryFromSweep(ForeignContext, Hit, Registry, Candidate));
	TestFalse(TEXT("Cross-Run failure clears candidate output"), Candidate.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenDetectorEmissionSessionTest,
	"Shanmen.0_0_10.WorldGameplay.DetectorEmissionSession",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenDetectorEmissionSessionTest::RunTest(const FString&)
{
	const FShanmenCombatActionSnapshot Action = MakeP21Action();
	FShanmenDetectorEmissionSession Session;
	TestTrue(TEXT("Valid detector starts a session"),
		FShanmenDetectorEmissionSession::TryStart(
			Action,
			TEXT("Detector.Zone.Fire01"),
			EShanmenHitDetectorKind::PersistentZone,
			Session));

	FShanmenWorldHitContext FirstEmission;
	TestTrue(TEXT("First scheduled emission begins"), Session.TryBeginEmission(FirstEmission));
	TestEqual(TEXT("First emission owns ordinal zero"), FirstEmission.GetHitOrdinal(), 0);
	TestFalse(TEXT("Nested emission is rejected"), Session.TryBeginEmission(FirstEmission));

	FShanmenHitCandidate TargetA;
	TargetA.ActivationId = Action.GetActivationId();
	TargetA.SourceEntityId = Action.GetSourceEntityId();
	TargetA.TargetEntityId = FShanmenWorldEntityIdFactory::MakeEntityId(
		RegistryRunId, TEXT("Spawn.Enemy.Wolf"), 0);
	TargetA.DetectorId = TEXT("Detector.Zone.Fire01");
	TargetA.DetectorKind = EShanmenHitDetectorKind::PersistentZone;
	TargetA.HitOrdinal = 0;

	FShanmenHitCandidate TargetB = TargetA;
	TargetB.TargetEntityId = FShanmenWorldEntityIdFactory::MakeEntityId(
		RegistryRunId, TEXT("Spawn.Enemy.Wolf"), 1);
	TestTrue(TEXT("First target is accepted"), Session.TryAcceptCandidate(TargetA));
	TestTrue(TEXT("Second target shares the same emission ordinal"), Session.TryAcceptCandidate(TargetB));
	TestFalse(TEXT("Repeated target in one emission is deduplicated"), Session.TryAcceptCandidate(TargetA));
	TestEqual(TEXT("Two target identities are retained independent of callback order"),
		Session.NumAcceptedTargets(), 2);
	TestTrue(TEXT("First emission ends"), Session.TryEndEmission());
	TestFalse(TEXT("Candidate outside an emission is rejected"), Session.TryAcceptCandidate(TargetA));

	FShanmenWorldHitContext SecondEmission;
	TestTrue(TEXT("Second scheduled emission begins"), Session.TryBeginEmission(SecondEmission));
	TestEqual(TEXT("Second emission advances exactly once"), SecondEmission.GetHitOrdinal(), 1);
	TestFalse(TEXT("Old ordinal cannot enter the new emission"), Session.TryAcceptCandidate(TargetA));
	TargetA.HitOrdinal = 1;
	TestTrue(TEXT("Same target can be hit once in the next emission"), Session.TryAcceptCandidate(TargetA));
	TestTrue(TEXT("Second emission ends"), Session.TryEndEmission());

	FShanmenDetectorEmissionSession Replay;
	TestTrue(TEXT("Replay session starts from the same frozen inputs"),
		FShanmenDetectorEmissionSession::TryStart(
			Action,
			TEXT("Detector.Zone.Fire01"),
			EShanmenHitDetectorKind::PersistentZone,
			Replay));
	FShanmenWorldHitContext ReplayFirst;
	TestTrue(TEXT("Replay first emission begins"), Replay.TryBeginEmission(ReplayFirst));
	TestEqual(TEXT("Replay begins from the same ordinal"), ReplayFirst.GetHitOrdinal(), 0);
	return true;
}

#endif
