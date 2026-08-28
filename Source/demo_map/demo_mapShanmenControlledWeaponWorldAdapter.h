#pragma once

#include "CoreMinimal.h"
#include "demo_mapCombatRunCoordinator.h"
#include "demo_mapShanmenControlledWeaponSession.h"

struct FHitResult;
struct FOverlapResult;

/** Product-bound failure before a controlled-object contact becomes canonical vitality. */
enum class Edemo_mapShanmenControlledWeaponWorldDeliveryError : uint8
{
	None,
	CoordinatorNotReady,
	SessionNotActive,
	ContextMismatch,
	ContactNotResolved,
	TargetVitalityUnavailable,
	CandidateRejected,
	DeliveryRejected
};

/** Auditable result for one sweep or overlap callback from the physical weapon. */
struct Fdemo_mapShanmenControlledWeaponWorldDeliveryResult
{
	Edemo_mapShanmenControlledWeaponWorldDeliveryError Error =
		Edemo_mapShanmenControlledWeaponWorldDeliveryError::CoordinatorNotReady;
	FGuid TargetEntityId;
	FShanmenControlledWeaponImpactReceipt Impact;
	Fdemo_mapCombatImpactDeliveryResult Delivery;

	bool IsDelivered() const
	{
		return Error
			== Edemo_mapShanmenControlledWeaponWorldDeliveryError::None
			&& TargetEntityId.IsValid()
			&& Impact.IsValid()
			&& Delivery.IsSuccess();
	}

	float GetNewlyCommittedDamage() const
	{
		return IsDelivered()
			&& Delivery.CommitResult.Status
				== EShanmenVitalityCommitStatus::Committed
			? Delivery.CommitResult.Receipt.GetAppliedDamage()
			: 0.0f;
	}
};

/**
 * Converts UE contacts into the frozen controlled-weapon contract and commits
 * them through CombatRunCoordinator. Session state advances only after the
 * vitality authority accepts the same receipt.
 */
class Fdemo_mapShanmenControlledWeaponWorldAdapter
{
public:
	static Fdemo_mapShanmenControlledWeaponWorldDeliveryResult
	ResolveSweepContact(
		Fdemo_mapShanmenControlledWeaponSession& Session,
		Fdemo_mapCombatRunCoordinator& Coordinator,
		const FShanmenWorldHitContext& Context,
		const FHitResult& Hit);

	static Fdemo_mapShanmenControlledWeaponWorldDeliveryResult
	ResolveOverlapContact(
		Fdemo_mapShanmenControlledWeaponSession& Session,
		Fdemo_mapCombatRunCoordinator& Coordinator,
		const FShanmenWorldHitContext& Context,
		const FOverlapResult& Overlap,
		const FVector& ContactLocation,
		const FVector& ContactNormal);

private:
	static bool ContextMatchesSession(
		const Fdemo_mapShanmenControlledWeaponSession& Session,
		const FShanmenWorldHitContext& Context);
	static Fdemo_mapShanmenControlledWeaponWorldDeliveryResult
	ResolveCandidateAndDeliver(
		Fdemo_mapShanmenControlledWeaponSession& Session,
		Fdemo_mapCombatRunCoordinator& Coordinator,
		const FShanmenHitCandidate& Candidate,
		AActor* TargetActor);
};
