#pragma once

#include "CoreMinimal.h"
#include "ShanmenBasicSwordExecution.h"
#include "ShanmenVitalityAuthority.h"
#include "ShanmenWorldEntityRegistry.h"

class APawn;
class UPrimitiveComponent;
class Udemo_mapPlayerHealthComponent;

/** Product-bound failures that occur before or around the canonical vitality commit. */
enum class Edemo_mapCombatImpactDeliveryError : uint8
{
	None,
	CoordinatorNotReady,
	InvalidImpactReceipt,
	RunMismatch,
	TargetMismatch,
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
 * Run-scoped product bridge for the persistent player Pawn.
 *
 * The coordinator derives identity only from the authority Run and the fixed
 * player spawn tuple, registers every player alias in WorldGameplay, binds the
 * same identity to the sole product vitality host, and delivers already
 * resolved BasicSword impacts without invoking any legacy damage path.
 */
class Fdemo_mapPlayerCombatCoordinator
{
public:
	static FName PlayerSpawnSourceId();

	bool TryBeginRun(
		const FGuid& RunId,
		APawn* PlayerPawn,
		Udemo_mapPlayerHealthComponent* PlayerHealth,
		FString& OutDiagnostic);
	bool TryEndRun(const FGuid& ExpectedRunId, FString& OutDiagnostic);
	void Reset();

	bool IsReady() const;
	bool IsActive() const { return EntityRegistry.GetRunId().IsValid(); }
	const FGuid& GetRunId() const { return EntityRegistry.GetRunId(); }
	const FGuid& GetPlayerEntityId() const { return PlayerEntityId; }
	const FShanmenWorldEntityRegistry& GetEntityRegistry() const
	{
		return EntityRegistry;
	}

	Fdemo_mapCombatImpactDeliveryResult DeliverBasicSwordImpact(
		const FShanmenBasicSwordImpactReceipt& Impact);

private:
	FShanmenWorldEntityRegistry EntityRegistry;
	FGuid PlayerEntityId;
	TWeakObjectPtr<APawn> BoundPlayerPawn;
	TWeakObjectPtr<Udemo_mapPlayerHealthComponent> BoundPlayerHealth;
	TWeakObjectPtr<UPrimitiveComponent> BoundPlayerRoot;
};
