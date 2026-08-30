#include "demo_mapCombatDisplacement.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"

bool Fdemo_mapCombatDisplacement::NormalizePlanarDirection(
	const FVector& Candidate,
	FVector& OutDirection)
{
	OutDirection = FVector(Candidate.X, Candidate.Y, 0.0f);
	return OutDirection.Normalize();
}

bool Fdemo_mapCombatDisplacement::ResolvePlanarDirectionWithFallback(
	const FVector& Primary,
	const FVector& Fallback,
	FVector& OutDirection)
{
	return NormalizePlanarDirection(Primary, OutDirection)
		|| NormalizePlanarDirection(Fallback, OutDirection);
}

float Fdemo_mapCombatDisplacement::ClampPreflightDistance(
	float RequestedDistance,
	float HitDistance,
	bool bBlockingHit,
	float WorldStaticClearance)
{
	if (!FMath::IsFinite(RequestedDistance)
		|| !FMath::IsFinite(HitDistance)
		|| !FMath::IsFinite(WorldStaticClearance)
		|| WorldStaticClearance < 0.0f)
	{
		return 0.0f;
	}
	const float Requested = FMath::Max(0.0f, RequestedDistance);
	if (!bBlockingHit)
	{
		return Requested;
	}
	return FMath::Clamp(
		HitDistance - WorldStaticClearance,
		0.0f,
		Requested);
}

Fdemo_mapCombatDisplacementResult
Fdemo_mapCombatDisplacement::PreflightWorldStatic(
	const ACharacter* Character,
	const FVector& PlanarDirection,
	float RequestedDistance,
	float WorldStaticClearance)
{
	Fdemo_mapCombatDisplacementResult Result;
	if (!FMath::IsFinite(RequestedDistance)
		|| !FMath::IsFinite(WorldStaticClearance)
		|| WorldStaticClearance < 0.0f)
	{
		return Result;
	}
	Result.RequestedDistance = FMath::Max(0.0f, RequestedDistance);
	if (!Character || !Character->GetWorld() || Result.RequestedDistance <= 0.0f)
	{
		return Result;
	}
	FVector Direction;
	if (!NormalizePlanarDirection(PlanarDirection, Direction))
	{
		return Result;
	}
	const UCapsuleComponent* Capsule = Character->GetCapsuleComponent();
	if (!Capsule)
	{
		return Result;
	}
	const FVector Start = Character->GetActorLocation();
	const FVector End = Start + Direction * Result.RequestedDistance;
	FCollisionObjectQueryParams Objects;
	Objects.AddObjectTypesToQuery(ECC_WorldStatic);
	FCollisionQueryParams Params(
		SCENE_QUERY_STAT(CombatDisplacementPreflight), false, Character);
	Params.AddIgnoredActor(Character);
	Result.bBlocked = Character->GetWorld()->SweepSingleByObjectType(
		Result.BlockingHit,
		Start,
		End,
		FQuat::Identity,
		Objects,
		FCollisionShape::MakeCapsule(
			Capsule->GetScaledCapsuleRadius(),
			Capsule->GetScaledCapsuleHalfHeight()),
		Params);
	const float HitDistance = Result.bBlocked
		? FVector::Dist2D(Start, Result.BlockingHit.Location)
		: Result.RequestedDistance;
	Result.ResolvedDistance = ClampPreflightDistance(
		Result.RequestedDistance,
		HitDistance,
		Result.bBlocked,
		WorldStaticClearance);
	return Result;
}

Fdemo_mapCombatDisplacementResult
Fdemo_mapCombatDisplacement::MoveCharacterSwept(
	ACharacter* Character,
	const FVector& PlanarDirection,
	float RequestedDistance)
{
	Fdemo_mapCombatDisplacementResult Result;
	Result.RequestedDistance = FMath::Max(0.0f, RequestedDistance);
	if (!Character || Result.RequestedDistance <= 0.0f)
	{
		return Result;
	}
	FVector Direction;
	if (!NormalizePlanarDirection(PlanarDirection, Direction))
	{
		return Result;
	}
	UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
	if (!Movement || !Movement->UpdatedComponent)
	{
		return Result;
	}
	const FVector Start = Character->GetActorLocation();
	Movement->SafeMoveUpdatedComponent(
		Direction * Result.RequestedDistance,
		Character->GetActorQuat(),
		true,
		Result.BlockingHit);
	Result.ResolvedDistance =
		FVector::Dist2D(Start, Character->GetActorLocation());
	Result.bBlocked = Result.BlockingHit.bBlockingHit
		|| Result.ResolvedDistance + KINDA_SMALL_NUMBER < Result.RequestedDistance;
	return Result;
}
