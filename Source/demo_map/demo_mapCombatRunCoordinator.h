#pragma once

#include "CoreMinimal.h"
#include "ShanmenBasicSwordExecution.h"
#include "ShanmenVitalityAuthority.h"
#include "ShanmenWorldEntityRegistry.h"
#include "demo_mapCombatVitalityHost.h"

class AActor;
class APawn;
class UPrimitiveComponent;
class Udemo_mapPlayerHealthComponent;
struct Fdemo_mapM01EnemyDefinition;
struct FHitResult;

/** Product-bound failures that occur before or around a canonical vitality commit. */
enum class Edemo_mapCombatImpactDeliveryError : uint8
{
	None,
	CoordinatorNotReady,
	InvalidImpactReceipt,
	RunMismatch,
	SourceMismatch,
	TargetMismatch,
	TargetNotRegistered,
	TargetNotVitalityBound,
	CommandConstructionFailed,
	CommitRejected
};

/** One product delivery attempt. The nested commit result remains the authority receipt. */
struct Fdemo_mapCombatImpactDeliveryResult
{
	Edemo_mapCombatImpactDeliveryError Error =
		Edemo_mapCombatImpactDeliveryError::CoordinatorNotReady;
	FShanmenVitalityCommitResult CommitResult;

	bool IsSuccess() const
	{
		return Error == Edemo_mapCombatImpactDeliveryError::None
			&& CommitResult.IsSuccess();
	}
};

/** Product execution failures before a BasicSword action can close normally. */
enum class Edemo_mapBasicSwordProductExecutionError : uint8
{
	None,
	CoordinatorNotReady,
	InvalidSourceItem,
	InvalidOffense,
	ActionConstructionFailed,
	RuntimeStartFailed,
	DefinitionConstructionFailed,
	ExecutionConstructionFailed,
	EmissionStartFailed,
	DeliveryRejected,
	EmissionEndFailed,
	RuntimeCompletionFailed
};

/** Auditable summary for one real player-input BasicSword trajectory sample. */
struct Fdemo_mapBasicSwordProductExecutionResult
{
	Edemo_mapBasicSwordProductExecutionError Error =
		Edemo_mapBasicSwordProductExecutionError::CoordinatorNotReady;
	FGuid ActivationId;
	int32 WorldContactCount = 0;
	int32 ResolvedCandidateCount = 0;
	int32 DeliveredImpactCount = 0;
	int32 CommittedImpactCount = 0;
	int32 AlreadyCommittedImpactCount = 0;

	bool IsExecuted() const
	{
		return Error == Edemo_mapBasicSwordProductExecutionError::None
			&& ActivationId.IsValid();
	}

	bool AppliedDamage() const
	{
		return IsExecuted() && CommittedImpactCount > 0;
	}
};

/**
 * Shared product bridge for one authority Run.
 *
 * The persistent player and every authored M01 enemy alias enter one World
 * Entity Registry. Player identity uses the fixed primary-player tuple. M01
 * identity uses only the authored SpawnMarkerId and ordinal zero; transient
 * actor addresses, object names, random loot ids, and spawn callback order
 * never participate. Every authored M01 product host binds the same canonical
 * vitality-host contract and ledger.
 */
class Fdemo_mapCombatRunCoordinator
{
public:
	static FName PlayerSpawnSourceId();
	static FGuid MakeM01EnemyEntityId(
		const FGuid& RunId,
		const Fdemo_mapM01EnemyDefinition& Definition);

	bool TryBeginRun(
		const FGuid& RunId,
		APawn* PlayerPawn,
		Udemo_mapPlayerHealthComponent* PlayerHealth,
		FString& OutDiagnostic);
	/** Registers the actor and collision root from its configured M01 identity. */
	bool TryRegisterM01Enemy(AActor* EnemyActor, FString& OutDiagnostic);
	bool TryEndRun(const FGuid& ExpectedRunId, FString& OutDiagnostic);
	void Reset();

	bool IsReady() const;
	bool IsActive() const { return EntityRegistry.GetRunId().IsValid(); }
	const FGuid& GetRunId() const { return EntityRegistry.GetRunId(); }
	const FGuid& GetPlayerEntityId() const { return PlayerEntityId; }
	int32 NumRegisteredM01Enemies() const { return M01EnemyBindings.Num(); }
	int32 NumVitalityBoundM01Enemies() const;
	const FShanmenWorldEntityRegistry& GetEntityRegistry() const
	{
		return EntityRegistry;
	}

	Fdemo_mapCombatImpactDeliveryResult DeliverBasicSwordImpactToPlayer(
		const FShanmenBasicSwordImpactReceipt& Impact);
	Fdemo_mapCombatImpactDeliveryResult DeliverBasicSwordImpactToM01Enemy(
		const FShanmenBasicSwordImpactReceipt& Impact,
		AActor* TargetEnemy);
	/**
	 * Executes one complete player BasicSword action from an already sampled UE
	 * trajectory. Every accepted contact resolves through this Run's Registry;
	 * canonical vitality delivery is the only mutable damage path.
	 */
	Fdemo_mapBasicSwordProductExecutionResult ExecutePlayerBasicSwordSweep(
		const FGuid& SourceItemInstanceId,
		float AttackPower,
		const TArray<FHitResult>& WorldHits);
	uint64 GetNextPlayerBasicSwordActivationSequence() const
	{
		return NextPlayerBasicSwordActivationSequence;
	}

private:
	struct FM01EnemyBinding
	{
		FName SpawnMarkerId = NAME_None;
		TWeakObjectPtr<AActor> Actor;
		TWeakObjectPtr<UPrimitiveComponent> CollisionRoot;
	};

	FShanmenWorldEntityRegistry EntityRegistry;
	FGuid PlayerEntityId;
	TWeakObjectPtr<APawn> BoundPlayerPawn;
	TWeakObjectPtr<Udemo_mapPlayerHealthComponent> BoundPlayerHealth;
	TWeakObjectPtr<UPrimitiveComponent> BoundPlayerRoot;
	TMap<FGuid, FM01EnemyBinding> M01EnemyBindings;
	uint64 NextPlayerBasicSwordActivationSequence = 1;
};
