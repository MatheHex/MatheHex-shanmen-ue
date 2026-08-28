#pragma once

#include "CoreMinimal.h"
#include "ShanmenControlledWeaponExecution.h"
#include "ShanmenItemTypes.h"
#include "demo_mapShanmenRunCorrelation.h"

class Udemo_mapItemSubsystem;
class Udemo_mapShanmenItemAuthoritySubsystem;

enum class Edemo_mapShanmenControlledWeaponPrepareStatus : uint8
{
	Prepared,
	AuthorityNotReady,
	RuntimeRunMismatch,
	RunCorrelationInvalid,
	SnapshotUnavailable,
	SnapshotStale,
	RequestInvalid,
	SourceItemMismatch,
	ItemNotFound,
	ItemNotDeployed,
	DefinitionNotFlyingSword,
	DeploymentEvidenceInvalid,
	CombatCaptureRejected
};

/** Product-authored activation input; item and Run identity remain authority-owned. */
struct Fdemo_mapShanmenControlledWeaponPrepareRequest
{
	FGuid SourceEntityId;
	FGuid SourceItemInstanceId;
	uint64 ActivationSequence = 0;
	FShanmenControlledWeaponDefinitionCapture Definition;
	float ControlPower = 0.0f;
	FGameplayTagContainer SourceTags;

	bool IsValid() const;
};

/** Read-only proof of the exact deployed item accepted for one activation. */
struct Fdemo_mapShanmenControlledWeaponAuthorityEvidence
{
	FGuid CorrelationId;
	FGuid ActiveRunId;
	FGuid OwnerId;
	FGuid ItemInstanceId;
	FName ItemDefinitionId = NAME_None;
	FGuid DeploymentReservationId;
	int32 AuthorityRevision = INDEX_NONE;
	int32 ItemRevision = INDEX_NONE;
	FShanmenContentStamp Content;

	bool IsValid() const;
};

/** Prepared immutable activation; no item, Runtime, Actor, or vitality mutation occurs. */
struct Fdemo_mapShanmenControlledWeaponPrepareResult
{
	Edemo_mapShanmenControlledWeaponPrepareStatus Status =
		Edemo_mapShanmenControlledWeaponPrepareStatus::RequestInvalid;
	FString Diagnostic;
	Fdemo_mapShanmenControlledWeaponAuthorityEvidence Evidence;
	FShanmenCombatActionSnapshot Action;
	FShanmenControlledWeaponDefinition Definition;
	FShanmenControlledWeaponOffenseSnapshot Offense;
	FShanmenControlledWeaponExecution Execution;

	bool IsPrepared() const;
};

/**
 * One-way product gate from durable active-Run item evidence into P6.0.
 *
 * It never grants flying-sword semantics from a mutable slot or legacy name:
 * the exact deployed weapon definition must carry Shanmen.Item.Weapon.FlyingSword.
 */
struct Fdemo_mapShanmenControlledWeaponAdapter
{
	/** Product facade: correlates the ready authority with the active Runtime read-only. */
	static Fdemo_mapShanmenControlledWeaponPrepareResult PrepareActiveRun(
		const Udemo_mapShanmenItemAuthoritySubsystem& Authority,
		const Udemo_mapItemSubsystem& Runtime,
		const Fdemo_mapShanmenControlledWeaponPrepareRequest& Request);

	/** Pure evidence gate used after the product has captured both durable values. */
	static Fdemo_mapShanmenControlledWeaponPrepareResult PrepareFromEvidence(
		const FShanmenItemAuthoritySnapshot& Snapshot,
		const Fdemo_mapShanmenRunCorrelation& Correlation,
		const Fdemo_mapShanmenControlledWeaponPrepareRequest& Request);
};
