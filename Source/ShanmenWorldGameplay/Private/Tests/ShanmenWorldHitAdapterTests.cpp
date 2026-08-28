#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ShanmenCombatResolver.h"
#include "ShanmenWorldHitAdapter.h"

#include "Engine/HitResult.h"
#include "Engine/OverlapResult.h"

#include <limits>

namespace
{
	class FFixedEntityResolver final : public IShanmenWorldEntityResolver
	{
	public:
		FGuid EntityId = FGuid(21, 22, 23, 24);
		bool bResolve = true;
		mutable EShanmenWorldContactSource LastContactSource = EShanmenWorldContactSource::Sweep;
		mutable int32 LastBodyIndex = INDEX_NONE;

		virtual bool TryResolveEntityId(
			EShanmenWorldContactSource ContactSource,
			const AActor*,
			const UPrimitiveComponent*,
			int32 BodyIndex,
			FGuid& OutEntityId) const override
		{
			LastContactSource = ContactSource;
			LastBodyIndex = BodyIndex;
			OutEntityId = bResolve ? EntityId : FGuid();
			return bResolve;
		}
	};

	FShanmenCombatActionSnapshot MakeAction()
	{
		FShanmenCombatActionCapture Capture;
		Capture.RunId = FGuid(1, 2, 3, 4);
		Capture.OwnerId = FGuid(5, 6, 7, 8);
		Capture.SourceEntityId = FGuid(9, 10, 11, 12);
		Capture.ActionDefinitionId = TEXT("Combat.Action.Sword.Basic01");
		Capture.Content.Version = TEXT("0.0.10.P2.0");
		Capture.Content.Digest = TEXT("TEST-DIGEST-P2.0");
		Capture.ActivationId = FShanmenCombatIdFactory::MakeActivationId(
			Capture.RunId,
			Capture.SourceEntityId,
			Capture.ActionDefinitionId,
			1);

		FShanmenCombatActionSnapshot Action;
		check(FShanmenCombatActionSnapshot::TryCapture(Capture, Action));
		return Action;
	}

	FShanmenWorldHitContext MakeContext(
		EShanmenHitDetectorKind DetectorKind,
		FName DetectorId,
		int32 HitOrdinal)
	{
		FShanmenWorldHitContext Context;
		check(FShanmenWorldHitContext::TryCreate(
			MakeAction(), DetectorId, DetectorKind, HitOrdinal, Context));
		return Context;
	}

	FHitResult MakeHit(const FVector& Point, const FVector& Normal, int32 Item)
	{
		FHitResult Hit;
		Hit.ImpactPoint = Point;
		Hit.ImpactNormal = Normal;
		Hit.Item = Item;
		return Hit;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenWorldSweepAdapterTest,
	"Shanmen.0_0_10.WorldGameplay.SweepAdapter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenWorldSweepAdapterTest::RunTest(const FString&)
{
	const FShanmenWorldHitContext Context = MakeContext(
		EShanmenHitDetectorKind::WeaponTrajectory,
		TEXT("Detector.Weapon.Main"),
		2);
	const FHitResult Hit = MakeHit(FVector(10.0, 20.0, 30.0), FVector::UpVector, 7);
	FFixedEntityResolver Resolver;
	FShanmenHitCandidate Candidate;

	TestTrue(TEXT("Sweep contact becomes a candidate"),
		FShanmenWorldHitAdapter::TryFromSweep(Context, Hit, Resolver, Candidate));
	TestTrue(TEXT("Candidate is structurally valid"), Candidate.IsValid());
	TestTrue(TEXT("Activation identity comes from the frozen action"),
		Candidate.ActivationId == Context.GetAction().GetActivationId());
	TestTrue(TEXT("Source identity comes from the frozen action"),
		Candidate.SourceEntityId == Context.GetAction().GetSourceEntityId());
	TestTrue(TEXT("Target identity comes only from the injected resolver"),
		Candidate.TargetEntityId == Resolver.EntityId);
	TestEqual(TEXT("Detector identity is preserved"), Candidate.DetectorId, Context.GetDetectorId());
	TestEqual(TEXT("Authoritative hit ordinal is preserved"), Candidate.HitOrdinal, 2);
	TestTrue(TEXT("Sweep evidence location is preserved"), Candidate.HitLocation.Equals(Hit.ImpactPoint));
	TestTrue(TEXT("Sweep source is reported to the resolver"),
		Resolver.LastContactSource == EShanmenWorldContactSource::Sweep);
	TestEqual(TEXT("Sweep body index is reported to the resolver"), Resolver.LastBodyIndex, 7);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenWorldOverlapAdapterTest,
	"Shanmen.0_0_10.WorldGameplay.OverlapAdapter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenWorldOverlapAdapterTest::RunTest(const FString&)
{
	const FShanmenWorldHitContext Context = MakeContext(
		EShanmenHitDetectorKind::PersistentZone,
		TEXT("Detector.Zone.Fire01"),
		0);
	FOverlapResult Overlap;
	Overlap.ItemIndex = 11;
	const FVector ContactLocation(40.0, 50.0, 60.0);
	const FVector ContactNormal = FVector::ForwardVector;
	FFixedEntityResolver Resolver;
	FShanmenHitCandidate Candidate;

	TestTrue(TEXT("Overlap contact becomes the same candidate contract"),
		FShanmenWorldHitAdapter::TryFromOverlap(
			Context, Overlap, ContactLocation, ContactNormal, Resolver, Candidate));
	TestTrue(TEXT("Overlap candidate is structurally valid"), Candidate.IsValid());
	TestTrue(TEXT("Explicit overlap sample location is preserved"), Candidate.HitLocation.Equals(ContactLocation));
	TestTrue(TEXT("Explicit overlap sample normal is preserved"), Candidate.HitNormal.Equals(ContactNormal));
	TestTrue(TEXT("Overlap source is reported to the resolver"),
		Resolver.LastContactSource == EShanmenWorldContactSource::Overlap);
	TestEqual(TEXT("Overlap body index is reported to the resolver"), Resolver.LastBodyIndex, 11);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenWorldProjectileAdapterTest,
	"Shanmen.0_0_10.WorldGameplay.ProjectileAdapter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenWorldProjectileAdapterTest::RunTest(const FString&)
{
	const FShanmenWorldHitContext Context = MakeContext(
		EShanmenHitDetectorKind::Projectile,
		TEXT("Detector.Projectile.Arrow01"),
		3);
	const FHitResult Hit = MakeHit(FVector(70.0, 80.0, 90.0), FVector::BackwardVector, 13);
	FFixedEntityResolver Resolver;
	FShanmenHitCandidate Candidate;

	TestTrue(TEXT("Projectile contact becomes the same candidate contract"),
		FShanmenWorldHitAdapter::TryFromProjectile(Context, Hit, Resolver, Candidate));
	TestTrue(TEXT("Projectile kind is retained"),
		Candidate.DetectorKind == EShanmenHitDetectorKind::Projectile);
	TestTrue(TEXT("Projectile source is reported to the resolver"),
		Resolver.LastContactSource == EShanmenWorldContactSource::Projectile);
	TestEqual(TEXT("Projectile body index is reported to the resolver"), Resolver.LastBodyIndex, 13);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenWorldChannelCompatibilityTest,
	"Shanmen.0_0_10.WorldGameplay.ChannelCompatibility",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenWorldChannelCompatibilityTest::RunTest(const FString&)
{
	TestTrue(TEXT("Weapon trajectories use sweep evidence"), FShanmenWorldHitAdapter::IsCompatible(
		EShanmenWorldContactSource::Sweep, EShanmenHitDetectorKind::WeaponTrajectory));
	TestTrue(TEXT("Persistent zones use overlap evidence"), FShanmenWorldHitAdapter::IsCompatible(
		EShanmenWorldContactSource::Overlap, EShanmenHitDetectorKind::PersistentZone));
	TestTrue(TEXT("Projectile detectors use projectile evidence"), FShanmenWorldHitAdapter::IsCompatible(
		EShanmenWorldContactSource::Projectile, EShanmenHitDetectorKind::Projectile));
	TestFalse(TEXT("A projectile cannot silently enter through the sweep adapter"), FShanmenWorldHitAdapter::IsCompatible(
		EShanmenWorldContactSource::Sweep, EShanmenHitDetectorKind::Projectile));
	TestFalse(TEXT("Rule targeting has no world-contact adapter"), FShanmenWorldHitAdapter::IsCompatible(
		EShanmenWorldContactSource::Overlap, EShanmenHitDetectorKind::TargetedRule));

	FShanmenWorldHitContext InvalidContext;
	TestFalse(TEXT("Targeted rules cannot create a world-hit context"),
		FShanmenWorldHitContext::TryCreate(
			MakeAction(), TEXT("Detector.Rule.Targeted"), EShanmenHitDetectorKind::TargetedRule, 0, InvalidContext));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenWorldFailureIsolationTest,
	"Shanmen.0_0_10.WorldGameplay.FailureIsolation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenWorldFailureIsolationTest::RunTest(const FString&)
{
	const FShanmenWorldHitContext Context = MakeContext(
		EShanmenHitDetectorKind::Shape,
		TEXT("Detector.Shape.Cone01"),
		0);
	FHitResult Hit = MakeHit(FVector::ZeroVector, FVector::UpVector, 0);
	FFixedEntityResolver Resolver;
	Resolver.bResolve = false;
	FShanmenHitCandidate Candidate;
	Candidate.TargetEntityId = FGuid(90, 91, 92, 93);

	TestFalse(TEXT("Unresolved world identity fails closed"),
		FShanmenWorldHitAdapter::TryFromSweep(Context, Hit, Resolver, Candidate));
	TestFalse(TEXT("Failure clears stale output"), Candidate.IsValid());

	Resolver.bResolve = true;
	Hit.ImpactPoint.X = std::numeric_limits<double>::quiet_NaN();
	TestFalse(TEXT("Non-finite world geometry fails closed"),
		FShanmenWorldHitAdapter::TryFromSweep(Context, Hit, Resolver, Candidate));
	TestFalse(TEXT("Geometry failure also clears output"), Candidate.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenWorldCandidateIdentityTest,
	"Shanmen.0_0_10.WorldGameplay.CandidateIdentity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenWorldCandidateIdentityTest::RunTest(const FString&)
{
	const FShanmenWorldHitContext Context = MakeContext(
		EShanmenHitDetectorKind::WeaponTrajectory,
		TEXT("Detector.Weapon.Main"),
		4);
	const FHitResult Hit = MakeHit(FVector(1.0, 2.0, 3.0), FVector::UpVector, 0);
	FFixedEntityResolver Resolver;
	Resolver.EntityId = Context.GetAction().GetSourceEntityId();

	FShanmenHitCandidate First;
	FShanmenHitCandidate Replay;
	TestTrue(TEXT("Self contact remains geometric and is not rejected as target policy"),
		FShanmenWorldHitAdapter::TryFromSweep(Context, Hit, Resolver, First));
	TestTrue(TEXT("Identical evidence can be replayed"),
		FShanmenWorldHitAdapter::TryFromSweep(Context, Hit, Resolver, Replay));

	const FGuid FirstImpactId = FShanmenCombatIdFactory::MakeImpactId(
		Context.GetAction().GetRunId(),
		First.ActivationId,
		First.DetectorId,
		First.TargetEntityId,
		First.HitOrdinal);
	const FGuid ReplayImpactId = FShanmenCombatIdFactory::MakeImpactId(
		Context.GetAction().GetRunId(),
		Replay.ActivationId,
		Replay.DetectorId,
		Replay.TargetEntityId,
		Replay.HitOrdinal);
	TestTrue(TEXT("Stable identity inputs replay the same ImpactId"), FirstImpactId == ReplayImpactId);
	TestTrue(TEXT("Candidate keeps self-target legality for the later policy layer"),
		First.SourceEntityId == First.TargetEntityId);
	return true;
}

#endif
