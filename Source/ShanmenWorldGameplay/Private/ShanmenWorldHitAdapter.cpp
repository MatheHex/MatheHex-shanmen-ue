#include "ShanmenWorldHitAdapter.h"

#include "Engine/HitResult.h"
#include "Engine/OverlapResult.h"

namespace
{
	bool IsWorldDetectorKind(EShanmenHitDetectorKind DetectorKind)
	{
		switch (DetectorKind)
		{
		case EShanmenHitDetectorKind::WeaponTrajectory:
		case EShanmenHitDetectorKind::Shape:
		case EShanmenHitDetectorKind::Projectile:
		case EShanmenHitDetectorKind::ControlledObject:
		case EShanmenHitDetectorKind::PersistentZone:
			return true;
		case EShanmenHitDetectorKind::TargetedRule:
		default:
			return false;
		}
	}
}

bool FShanmenWorldHitContext::TryCreate(
	const FShanmenCombatActionSnapshot& Action,
	FName DetectorId,
	EShanmenHitDetectorKind DetectorKind,
	int32 HitOrdinal,
	FShanmenWorldHitContext& OutContext)
{
	OutContext = FShanmenWorldHitContext();
	if (!Action.IsValid()
		|| DetectorId.IsNone()
		|| !IsWorldDetectorKind(DetectorKind)
		|| HitOrdinal < 0)
	{
		return false;
	}

	OutContext.Action = Action;
	OutContext.DetectorId = DetectorId;
	OutContext.DetectorKind = DetectorKind;
	OutContext.HitOrdinal = HitOrdinal;
	return true;
}

bool FShanmenWorldHitContext::IsValid() const
{
	return Action.IsValid()
		&& !DetectorId.IsNone()
		&& IsWorldDetectorKind(DetectorKind)
		&& HitOrdinal >= 0;
}

bool FShanmenWorldHitAdapter::TryFromSweep(
	const FShanmenWorldHitContext& Context,
	const FHitResult& Hit,
	const IShanmenWorldEntityResolver& EntityResolver,
	FShanmenHitCandidate& OutCandidate)
{
	return TryBuildCandidate(
		Context,
		EShanmenWorldContactSource::Sweep,
		Hit.GetActor(),
		Hit.GetComponent(),
		Hit.Item,
		Hit.ImpactPoint,
		Hit.ImpactNormal,
		EntityResolver,
		OutCandidate);
}

bool FShanmenWorldHitAdapter::TryFromOverlap(
	const FShanmenWorldHitContext& Context,
	const FOverlapResult& Overlap,
	const FVector& ContactLocation,
	const FVector& ContactNormal,
	const IShanmenWorldEntityResolver& EntityResolver,
	FShanmenHitCandidate& OutCandidate)
{
	return TryBuildCandidate(
		Context,
		EShanmenWorldContactSource::Overlap,
		Overlap.GetActor(),
		Overlap.GetComponent(),
		Overlap.ItemIndex,
		ContactLocation,
		ContactNormal,
		EntityResolver,
		OutCandidate);
}

bool FShanmenWorldHitAdapter::TryFromProjectile(
	const FShanmenWorldHitContext& Context,
	const FHitResult& Hit,
	const IShanmenWorldEntityResolver& EntityResolver,
	FShanmenHitCandidate& OutCandidate)
{
	return TryBuildCandidate(
		Context,
		EShanmenWorldContactSource::Projectile,
		Hit.GetActor(),
		Hit.GetComponent(),
		Hit.Item,
		Hit.ImpactPoint,
		Hit.ImpactNormal,
		EntityResolver,
		OutCandidate);
}

bool FShanmenWorldHitAdapter::IsCompatible(
	EShanmenWorldContactSource ContactSource,
	EShanmenHitDetectorKind DetectorKind)
{
	switch (ContactSource)
	{
	case EShanmenWorldContactSource::Sweep:
		return DetectorKind == EShanmenHitDetectorKind::WeaponTrajectory
			|| DetectorKind == EShanmenHitDetectorKind::Shape
			|| DetectorKind == EShanmenHitDetectorKind::ControlledObject;
	case EShanmenWorldContactSource::Overlap:
		return DetectorKind == EShanmenHitDetectorKind::Shape
			|| DetectorKind == EShanmenHitDetectorKind::ControlledObject
			|| DetectorKind == EShanmenHitDetectorKind::PersistentZone;
	case EShanmenWorldContactSource::Projectile:
		return DetectorKind == EShanmenHitDetectorKind::Projectile;
	default:
		return false;
	}
}

bool FShanmenWorldHitAdapter::TryBuildCandidate(
	const FShanmenWorldHitContext& Context,
	EShanmenWorldContactSource ContactSource,
	const AActor* Actor,
	const UPrimitiveComponent* Component,
	int32 BodyIndex,
	const FVector& ContactLocation,
	const FVector& ContactNormal,
	const IShanmenWorldEntityResolver& EntityResolver,
	FShanmenHitCandidate& OutCandidate)
{
	OutCandidate = FShanmenHitCandidate();
	if (!Context.IsValid()
		|| !IsCompatible(ContactSource, Context.GetDetectorKind())
		|| ContactLocation.ContainsNaN()
		|| ContactNormal.ContainsNaN())
	{
		return false;
	}

	FGuid TargetEntityId;
	if (!EntityResolver.TryResolveEntityId(
			ContactSource,
			Actor,
			Component,
			BodyIndex,
			TargetEntityId)
		|| !TargetEntityId.IsValid())
	{
		return false;
	}

	OutCandidate.ActivationId = Context.GetAction().GetActivationId();
	OutCandidate.SourceEntityId = Context.GetAction().GetSourceEntityId();
	OutCandidate.TargetEntityId = TargetEntityId;
	OutCandidate.DetectorId = Context.GetDetectorId();
	OutCandidate.DetectorKind = Context.GetDetectorKind();
	OutCandidate.HitLocation = ContactLocation;
	OutCandidate.HitNormal = ContactNormal;
	OutCandidate.HitOrdinal = Context.GetHitOrdinal();

	if (!OutCandidate.IsValid())
	{
		OutCandidate = FShanmenHitCandidate();
		return false;
	}
	return true;
}
