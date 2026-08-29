#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenControlledWeaponRunHost.h"

/** Stable, UObject-free identity for one caller-supplied overlap contact. */
struct Fdemo_mapShanmenControlledWeaponThreatContactIdentity
{
	FGuid TargetEntityId;
	int32 BodyIndex = INDEX_NONE;
	FVector ContactLocation = FVector::ZeroVector;
	FVector ContactNormal = FVector::ZeroVector;
	bool bBlockingHit = false;

	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenControlledWeaponThreatContactIdentity& Other) const;
};

/** Canonical contact fingerprint for one exact item in one sample pulse. */
struct Fdemo_mapShanmenControlledWeaponThreatItemIdentity
{
	FGuid ItemInstanceId;
	TArray<Fdemo_mapShanmenControlledWeaponThreatContactIdentity> Contacts;

	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenControlledWeaponThreatItemIdentity& Other) const;
};

/**
 * Frozen external cadence pulse for one atomic, explicit threat sample.
 *
 * Capture resolves transient world objects to stable EntityIds and canonicalizes
 * item/contact order. The caller still owns when and how overlaps are queried.
 */
class Fdemo_mapShanmenControlledWeaponThreatSampleIntent
{
public:
	static bool TryCapture(
		const FGuid& IntentId,
		const FGuid& RunId,
		int64 SampleSequence,
		const Fdemo_mapCombatRunCoordinator& Coordinator,
		const TArray<Fdemo_mapShanmenControlledWeaponThreatSampleRequest>&
			Requests,
		Fdemo_mapShanmenControlledWeaponThreatSampleIntent& OutIntent);

	bool IsValid() const;
	/** Re-resolves transient contacts to reject a capture/route identity race. */
	bool MatchesCoordinator(
		const Fdemo_mapCombatRunCoordinator& Coordinator) const;
	bool Matches(
		const Fdemo_mapShanmenControlledWeaponThreatSampleIntent& Other) const;
	const FGuid& GetIntentId() const { return IntentId; }
	const FGuid& GetRunId() const { return RunId; }
	int64 GetSampleSequence() const { return SampleSequence; }
	const TArray<Fdemo_mapShanmenControlledWeaponThreatSampleRequest>&
	GetRequests() const
	{
		return Requests;
	}
	const TArray<Fdemo_mapShanmenControlledWeaponThreatItemIdentity>&
	GetItemIdentities() const
	{
		return ItemIdentities;
	}

private:
	FGuid IntentId;
	FGuid RunId;
	int64 SampleSequence = INDEX_NONE;
	TArray<Fdemo_mapShanmenControlledWeaponThreatSampleRequest> Requests;
	TArray<Fdemo_mapShanmenControlledWeaponThreatItemIdentity> ItemIdentities;
};

enum class Edemo_mapShanmenControlledWeaponThreatSampleStatus : uint8
{
	Applied,
	Replayed,
	CoordinatorNotReady,
	IntentInvalid,
	RunMismatch,
	HostInvalid,
	RouterInvalid,
	RouterRunMismatch,
	SequenceStale,
	SequenceGap,
	SequenceExhausted,
	IntentConflict,
	BatchRejected
};

/** Auditable result for one external sample pulse. */
struct Fdemo_mapShanmenControlledWeaponThreatSampleResult
{
	Edemo_mapShanmenControlledWeaponThreatSampleStatus Status =
		Edemo_mapShanmenControlledWeaponThreatSampleStatus::
			CoordinatorNotReady;
	FGuid IntentId;
	FGuid RunId;
	int64 SampleSequence = INDEX_NONE;
	int32 ItemCount = 0;
	Fdemo_mapShanmenControlledWeaponThreatSampleBatch Batch;
	FString Diagnostic;

	bool IsAccepted() const;
	bool IsReplay() const
	{
		return Status
			== Edemo_mapShanmenControlledWeaponThreatSampleStatus::Replayed;
	}
};

/**
 * Bounded Run-scoped bridge between an external cadence owner and the P6.21
 * atomic Host batch. It accepts contiguous explicit sequences, replays only
 * the latest exact pulse, rejects conflicts/stale/gaps, and commits Router plus
 * Host together. It owns no Tick, query frequency, cooldown, or gameplay effect.
 */
class Fdemo_mapShanmenControlledWeaponThreatSampleRouter
{
public:
	Fdemo_mapShanmenControlledWeaponThreatSampleResult TryRoute(
		Fdemo_mapShanmenControlledWeaponRunHost& Host,
		const Fdemo_mapCombatRunCoordinator& Coordinator,
		const Fdemo_mapShanmenControlledWeaponThreatSampleIntent& Intent);

	bool IsEmpty() const { return !LatestSample.IsSet(); }
	bool IsValid() const;
	const FGuid& GetRunId() const { return RunId; }
	int64 GetNextSampleSequence() const { return NextSampleSequence; }
	int64 NumAcceptedSamples() const { return NextSampleSequence; }
	void Reset();

private:
	struct FLatestSample
	{
		FGuid IntentId;
		FGuid RunId;
		int64 SampleSequence = INDEX_NONE;
		TArray<Fdemo_mapShanmenControlledWeaponThreatItemIdentity>
			ItemIdentities;
		Fdemo_mapShanmenControlledWeaponThreatSampleResult Result;

		bool IsValid() const;
		bool Matches(
			const Fdemo_mapShanmenControlledWeaponThreatSampleIntent& Intent) const;
	};

	FGuid RunId;
	int64 NextSampleSequence = 0;
	TOptional<FLatestSample> LatestSample;
};
