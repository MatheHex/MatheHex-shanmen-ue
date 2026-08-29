#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenControlledWeaponRunHost.h"

/** Frozen, input-device-independent command for exact items in one Run. */
class Fdemo_mapShanmenControlledWeaponRunCommandIntent
{
public:
	static bool TryCapture(
		const FGuid& IntentId,
		const FGuid& RunId,
		EShanmenControlledWeaponCommandKind Kind,
		const TArray<FGuid>& TargetItemInstanceIds,
		const FVector& DesiredDirection,
		Fdemo_mapShanmenControlledWeaponRunCommandIntent& OutIntent);

	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenControlledWeaponRunCommandIntent& Other) const;
	const FGuid& GetIntentId() const { return IntentId; }
	const FGuid& GetRunId() const { return RunId; }
	EShanmenControlledWeaponCommandKind GetKind() const { return Kind; }
	const TArray<FGuid>& GetTargetItemInstanceIds() const
	{
		return TargetItemInstanceIds;
	}
	const FVector& GetDesiredDirection() const { return DesiredDirection; }

private:
	FGuid IntentId;
	FGuid RunId;
	EShanmenControlledWeaponCommandKind Kind =
		EShanmenControlledWeaponCommandKind::Launch;
	TArray<FGuid> TargetItemInstanceIds;
	FVector DesiredDirection = FVector::ZeroVector;
};

/** One stable item entry from an accepted Run command intent. */
struct Fdemo_mapShanmenControlledWeaponRunCommandEntry
{
	FGuid ItemInstanceId;
	int64 ExpectedSequence = INDEX_NONE;
	FShanmenControlledWeaponCommandReceipt Command;
	FShanmenActionTransitionReceipt Recovery;
	FShanmenActionTransitionReceipt Completed;

	bool IsValidFor(EShanmenControlledWeaponCommandKind Kind) const;
};

enum class Edemo_mapShanmenControlledWeaponRunCommandStatus : uint8
{
	Applied,
	Replayed,
	CoordinatorNotReady,
	IntentInvalid,
	RunMismatch,
	HostInvalid,
	RouterInvalid,
	RouterRunMismatch,
	IntentIdConflict,
	TargetNotBound,
	CommandRejected
};

/** Auditable result for one atomic, stable-order multi-item command. */
struct Fdemo_mapShanmenControlledWeaponRunCommandResult
{
	Edemo_mapShanmenControlledWeaponRunCommandStatus Status =
		Edemo_mapShanmenControlledWeaponRunCommandStatus::CoordinatorNotReady;
	FGuid IntentId;
	FGuid RunId;
	EShanmenControlledWeaponCommandKind Kind =
		EShanmenControlledWeaponCommandKind::Launch;
	int32 TargetCount = 0;
	TArray<Fdemo_mapShanmenControlledWeaponRunCommandEntry> Entries;
	FString Diagnostic;

	bool IsAccepted() const;
	bool IsReplay() const
	{
		return Status
			== Edemo_mapShanmenControlledWeaponRunCommandStatus::Replayed;
	}
};

/**
 * Run-scoped intent owner between future input and the P6.5 Host.
 *
 * Input supplies stable intent identity and exact target items, never per-item
 * command sequence. Commands execute on a Host copy in stable item order and
 * commit atomically. Exact replay returns the original receipts; reuse of one
 * IntentId with a different payload fails closed. No key, selection policy,
 * Actor spawn, inventory mutation, or combat authority is owned here.
 */
class Fdemo_mapShanmenControlledWeaponRunCommandRouter
{
public:
	Fdemo_mapShanmenControlledWeaponRunCommandResult TryRoute(
		Fdemo_mapShanmenControlledWeaponRunHost& Host,
		const Fdemo_mapCombatRunCoordinator& Coordinator,
		const Fdemo_mapShanmenControlledWeaponRunCommandIntent& Intent);

	bool IsEmpty() const { return ProcessedIntents.IsEmpty(); }
	bool IsValid() const;
	const FGuid& GetRunId() const { return RunId; }
	int32 NumProcessedIntents() const { return ProcessedIntents.Num(); }
	void Reset();

private:
	struct FProcessedIntent
	{
		Fdemo_mapShanmenControlledWeaponRunCommandIntent Intent;
		Fdemo_mapShanmenControlledWeaponRunCommandResult Result;
	};

	FGuid RunId;
	TMap<FGuid, FProcessedIntent> ProcessedIntents;
};
