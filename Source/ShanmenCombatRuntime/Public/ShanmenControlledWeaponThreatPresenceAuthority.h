#pragma once

#include "CoreMinimal.h"
#include "ShanmenControlledWeaponExecution.h"

enum class EShanmenControlledWeaponThreatPresenceConsumeStatus : uint8
{
	Invalid,
	Consumed,
	AlreadyConsumed,
	NoOp,
	Rejected
};

enum class EShanmenControlledWeaponThreatPresenceConsumeError : uint8
{
	None,
	ReceiptInvalid,
	AuthorityNotReady,
	RunMismatch,
	SourceMismatch,
	SampleExpired,
	SampleConflict,
	IntentConflict,
	PartialReplayConflict,
	RevisionExhausted
};

/** Immutable audit record for one accepted zero-effect presence intent. */
class SHANMENCOMBATRUNTIME_API
FShanmenControlledWeaponThreatPresenceConsumeReceipt
{
public:
	bool IsValid() const;
	const FShanmenControlledWeaponThreatPresenceIntent& GetIntent() const
	{
		return Intent;
	}
	const FGuid& GetSourceEntityId() const { return SourceEntityId; }
	int64 GetAuthorityRevision() const { return AuthorityRevision; }

private:
	friend class FShanmenControlledWeaponThreatPresenceAuthority;

	FShanmenControlledWeaponThreatPresenceIntent Intent;
	FGuid SourceEntityId;
	int64 AuthorityRevision = INDEX_NONE;
};

/** Atomic batch result. No status in this type applies a gameplay effect. */
class SHANMENCOMBATRUNTIME_API
FShanmenControlledWeaponThreatPresenceConsumeResult
{
public:
	bool IsValid() const;
	bool IsSuccess() const;
	EShanmenControlledWeaponThreatPresenceConsumeStatus GetStatus() const
	{
		return Status;
	}
	EShanmenControlledWeaponThreatPresenceConsumeError GetError() const
	{
		return Error;
	}
	const FGuid& GetRunId() const { return RunId; }
	int64 GetAuthorityRevisionBefore() const
	{
		return AuthorityRevisionBefore;
	}
	int64 GetAuthorityRevisionAfter() const
	{
		return AuthorityRevisionAfter;
	}
	const TArray<FShanmenControlledWeaponThreatPresenceConsumeReceipt>&
	GetReceipts() const
	{
		return Receipts;
	}

private:
	friend class FShanmenControlledWeaponThreatPresenceAuthority;

	EShanmenControlledWeaponThreatPresenceConsumeStatus Status =
		EShanmenControlledWeaponThreatPresenceConsumeStatus::Invalid;
	EShanmenControlledWeaponThreatPresenceConsumeError Error =
		EShanmenControlledWeaponThreatPresenceConsumeError::None;
	FGuid RunId;
	int64 AuthorityRevisionBefore = INDEX_NONE;
	int64 AuthorityRevisionAfter = INDEX_NONE;
	TArray<FShanmenControlledWeaponThreatPresenceConsumeReceipt> Receipts;
};

/**
 * Run-scoped idempotency authority for presence observations.
 *
 * The authority records consumption only. It never owns cadence, duration,
 * target state, damage, control, Actor, World, item, or inventory mutation.
 */
class SHANMENCOMBATRUNTIME_API
FShanmenControlledWeaponThreatPresenceAuthority
{
public:
	static bool TryCreate(
		const FGuid& RunId,
		const FGuid& SourceEntityId,
		FShanmenControlledWeaponThreatPresenceAuthority& OutAuthority);

	bool IsValid() const;
	FShanmenControlledWeaponThreatPresenceConsumeResult Consume(
		const FShanmenControlledWeaponThreatPresenceReceipt& Presence);
	void Reset();

	const FGuid& GetRunId() const { return RunId; }
	const FGuid& GetSourceEntityId() const { return SourceEntityId; }
	int64 GetAuthorityRevision() const { return AuthorityRevision; }
	int64 GetSampleCheckpointRevision() const
	{
		return SampleCheckpointRevision;
	}
	int32 NumTrackedSamples() const { return LatestSamples.Num(); }
	int32 NumRetainedIntents() const { return ProcessedIntents.Num(); }
	/** Lifetime accepted-intent count; retained replay receipts are bounded. */
	int64 NumConsumedIntents() const
	{
		return AuthorityRevision >= 0 ? AuthorityRevision : 0;
	}
	int32 GetLatestSampleOrdinal(const FGuid& SourceItemInstanceId) const;
	bool Contains(const FGuid& IntentId) const;

private:
	struct FSampleCheckpoint
	{
		FGuid SampleId;
		FGuid ActivationId;
		FName DetectorId = NAME_None;
		EShanmenHitDetectorKind DetectorKind =
			EShanmenHitDetectorKind::Shape;
		int32 HitOrdinal = INDEX_NONE;
		TArray<FGuid> IntentIds;
	};

	static bool TryBuildSampleCheckpoint(
		const FShanmenControlledWeaponThreatPresenceReceipt& Presence,
		FSampleCheckpoint& OutCheckpoint);
	bool IsCheckpointValid(
		const FGuid& SourceItemInstanceId,
		const FSampleCheckpoint& Checkpoint) const;
	void PruneCheckpointIntents(
		const FGuid& SourceItemInstanceId);
	FShanmenControlledWeaponThreatPresenceConsumeResult Reject(
		EShanmenControlledWeaponThreatPresenceConsumeError Error) const;

	FGuid RunId;
	FGuid SourceEntityId;
	int64 AuthorityRevision = INDEX_NONE;
	int64 SampleCheckpointRevision = INDEX_NONE;
	TMap<FGuid, FShanmenControlledWeaponThreatPresenceConsumeReceipt>
		ProcessedIntents;
	TMap<FGuid, FSampleCheckpoint> LatestSamples;
};
