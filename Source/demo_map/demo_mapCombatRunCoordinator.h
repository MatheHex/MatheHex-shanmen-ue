#pragma once

#include "CoreMinimal.h"
#include "ShanmenBasicSwordExecution.h"
#include "ShanmenVitalityAuthority.h"
#include "ShanmenWorldEntityRegistry.h"

class AActor;
class APawn;
class UPrimitiveComponent;
class Udemo_mapPlayerHealthComponent;
class Ademo_mapEnemyCharacter;
struct Fdemo_mapM01EnemyDefinition;

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

/**
 * Shared product bridge for one authority Run.
 *
 * The persistent player and every authored M01 enemy alias enter one World
 * Entity Registry. Player identity uses the fixed primary-player tuple. M01
 * identity uses only the authored SpawnMarkerId and ordinal zero; transient
 * actor addresses, object names, random loot ids, and spawn callback order
 * never participate. Vitality is bound only for product hosts migrated to the
 * canonical ledger.
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
		Ademo_mapEnemyCharacter* TargetEnemy);

private:
	struct FM01EnemyBinding
	{
		FName SpawnMarkerId = NAME_None;
		TWeakObjectPtr<AActor> Actor;
		TWeakObjectPtr<UPrimitiveComponent> CollisionRoot;
		TWeakObjectPtr<Ademo_mapEnemyCharacter> VitalityHost;
	};

	FShanmenWorldEntityRegistry EntityRegistry;
	FGuid PlayerEntityId;
	TWeakObjectPtr<APawn> BoundPlayerPawn;
	TWeakObjectPtr<Udemo_mapPlayerHealthComponent> BoundPlayerHealth;
	TWeakObjectPtr<UPrimitiveComponent> BoundPlayerRoot;
	TMap<FGuid, FM01EnemyBinding> M01EnemyBindings;
};
