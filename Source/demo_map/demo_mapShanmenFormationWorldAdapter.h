#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenFormationProductSession.h"

class AActor;
class UWorld;

enum class Edemo_mapShanmenFormationWorldStatus : uint8
{
	Placed,
	Replayed,
	Adopted,
	TeardownComplete,
	TeardownReplayed,
	SessionInvalid,
	SessionTerminal,
	AnchorNotCommitted,
	IntentInvalid,
	WorldInvalid,
	AdapterConflict,
	ActorClassInvalid,
	ActorClassConflict,
	PlacementConflict,
	DuplicatePlacementActors,
	SpawnRejected,
	ActorUnavailable,
	TerminalRequired,
	TeardownRecoveryRequired
};

/** Immutable world-placement input derived only from one committed P8.2 audit. */
struct Fdemo_mapShanmenFormationAnchorPlacementIntent
{
	FGuid PlacementId;
	FGuid RunId;
	FGuid OwnerId;
	FGuid DeploymentId;
	FName AnchorDefinitionId = NAME_None;
	FGuid AnchorInstanceId;
	FVector WorldLocation = FVector::ZeroVector;
	FGuid AttemptId;
	FGuid FulfillmentId;
	FGuid DeploymentReceiptId;
	int32 AuthorityRevision = INDEX_NONE;
	FShanmenContentStamp Content;

	bool IsValid() const;
};

/** Deterministic evidence that one exact placement identity owns one Actor class. */
struct Fdemo_mapShanmenFormationAnchorPlacementReceipt
{
	FGuid ReceiptId;
	Fdemo_mapShanmenFormationAnchorPlacementIntent Intent;
	FString ActorClassPath;
	FName PlacementTag = NAME_None;
	FName DeploymentTag = NAME_None;

	bool IsValid() const;
};

/** Stable terminal cleanup identity; RemovedActorCount is observational only. */
struct Fdemo_mapShanmenFormationWorldTeardownReceipt
{
	FGuid ReceiptId;
	FGuid DeploymentId;
	Edemo_mapShanmenFormationSessionState TerminalState =
		Edemo_mapShanmenFormationSessionState::Empty;
	int32 CommittedAnchorCount = 0;
	int32 RemovedActorCount = 0;

	bool IsValid() const;
};

/** One explicit placement or terminal cleanup result. */
struct Fdemo_mapShanmenFormationWorldResult
{
	Edemo_mapShanmenFormationWorldStatus Status =
		Edemo_mapShanmenFormationWorldStatus::SessionInvalid;
	FString Diagnostic;
	Fdemo_mapShanmenFormationAnchorPlacementIntent Intent;
	Fdemo_mapShanmenFormationAnchorPlacementReceipt PlacementReceipt;
	Fdemo_mapShanmenFormationWorldTeardownReceipt TeardownReceipt;
	TWeakObjectPtr<AActor> Actor;

	bool IsPlacementSuccess() const;
	bool IsTeardownSuccess() const;
};

/**
 * Transient world projection for committed formation anchors.
 *
 * It owns only weak Actor handles and deterministic receipts. Materials,
 * deployment progress, action lifecycle, authored effects, cadence, UI, and
 * persistence remain with their existing authorities.
 */
class Fdemo_mapShanmenFormationWorldAdapter
{
public:
	static bool BuildPlacementIntent(
		const Fdemo_mapShanmenFormationProductSession& Session,
		FName AnchorDefinitionId,
		Fdemo_mapShanmenFormationAnchorPlacementIntent& OutIntent);
	static FName MakePlacementTag(const FGuid& PlacementId);
	static FName MakeDeploymentTag(const FGuid& DeploymentId);

	Fdemo_mapShanmenFormationWorldResult TryPlaceCommittedAnchor(
		UWorld* World,
		TSubclassOf<AActor> ActorClass,
		const Fdemo_mapShanmenFormationProductSession& Session,
		FName AnchorDefinitionId);
	Fdemo_mapShanmenFormationWorldResult TryTeardownTerminal(
		UWorld* World,
		const Fdemo_mapShanmenFormationProductSession& Session);

	bool IsValid() const;
	int32 GetPlacementCount() const { return Placements.Num(); }
	bool IsBoundToWorld(const UWorld* World) const
	{
		return World && BoundWorld.Get() == World;
	}
	const Fdemo_mapShanmenFormationAnchorPlacementReceipt* FindReceipt(
		const FGuid& PlacementId) const;
	bool IsTeardownComplete() const { return bTeardownComplete; }
	const Fdemo_mapShanmenFormationWorldTeardownReceipt& GetTeardownReceipt()
		const
	{
		return TeardownReceipt;
	}

private:
	struct FPlacementRecord
	{
		Fdemo_mapShanmenFormationAnchorPlacementIntent Intent;
		Fdemo_mapShanmenFormationAnchorPlacementReceipt Receipt;
		TSubclassOf<AActor> ActorClass;
		TWeakObjectPtr<AActor> Actor;
		bool bRemoved = false;
	};

	FPlacementRecord* FindRecord(const FGuid& PlacementId);

	FGuid BoundDeploymentId;
	TWeakObjectPtr<UWorld> BoundWorld;
	TArray<FPlacementRecord> Placements;
	Fdemo_mapShanmenFormationWorldTeardownReceipt TeardownReceipt;
	int32 TeardownRemovedActorCount = 0;
	bool bTeardownComplete = false;
};
