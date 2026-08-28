#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenControlledWeaponRunHost.h"

/** Product-visible outcome for ending one combat Run with controlled weapons. */
enum class Edemo_mapShanmenControlledWeaponRunEndStatus : uint8
{
	Ended,
	CoordinatorNotActive,
	HostInvalid,
	HostRunMismatch,
	HostInterruptRejected,
	HostRetirementRejected,
	CoordinatorEndRejected
};

/** Auditable result for the ordered Host -> Coordinator teardown transaction. */
struct Fdemo_mapShanmenControlledWeaponRunEndResult
{
	Edemo_mapShanmenControlledWeaponRunEndStatus Status =
		Edemo_mapShanmenControlledWeaponRunEndStatus::CoordinatorNotActive;
	FGuid RunId;
	int32 BoundItemCount = 0;
	int32 ActiveItemCount = 0;
	int32 InterruptedItemCount = 0;
	int32 RetiredItemCount = 0;
	TArray<Fdemo_mapShanmenControlledWeaponHostInterruptReceipt>
		InterruptReceipts;
	FString Diagnostic;

	bool IsEnded() const
	{
		return Status
			== Edemo_mapShanmenControlledWeaponRunEndStatus::Ended
			&& RunId.IsValid()
			&& InterruptedItemCount == InterruptReceipts.Num()
			&& RetiredItemCount == BoundItemCount;
	}
};

/**
 * Ends one product combat Run without orphaning controlled-weapon Sessions.
 *
 * Host interruption and terminal retirement are prepared on a value copy.
 * The real Host is replaced only after the Coordinator accepts the exact Run
 * end. A Host failure never touches Coordinator state, and a Coordinator
 * rejection leaves the real Host unchanged for diagnosis or retry.
 */
class Fdemo_mapShanmenControlledWeaponRunLifecycle
{
public:
	static Fdemo_mapShanmenControlledWeaponRunEndResult TryEndRun(
		Fdemo_mapShanmenControlledWeaponRunHost& Host,
		Fdemo_mapCombatRunCoordinator& Coordinator);
};
