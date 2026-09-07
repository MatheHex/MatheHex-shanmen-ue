#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "demo_mapShanmenControlledWeaponActor.generated.h"

class UBoxComponent;
class UMaterialInstanceDynamic;
class UPointLightComponent;
class UStaticMeshComponent;
struct Fdemo_mapShanmenControlledWeaponWorldThreatSampleResult;
struct Fdemo_mapShanmenControlledWeaponWorldLifecycle;

/**
 * Physical carrier for the canonical training flying sword.
 *
 * Durable item identity and combat state remain owned by ShanmenItems and the
 * P6 Host. This Actor owns only world collision, presentation, and an exact
 * identity binding that lets lifecycle tests detect stale or substituted
 * carriers.
 */
UCLASS()
class Ademo_mapShanmenControlledWeaponActor : public AActor
{
	GENERATED_BODY()

public:
	Ademo_mapShanmenControlledWeaponActor();

	bool TryBindProductIdentity(
		const FGuid& InRunId,
		const FGuid& InItemInstanceId,
		AActor* InSourceActor);
	bool IsProductBound() const;
	bool IsProductBoundTo(
		const FGuid& ExpectedRunId,
		const FGuid& ExpectedItemInstanceId,
		const AActor* ExpectedSourceActor) const;

	const FGuid& GetRunId() const { return RunId; }
	const FGuid& GetItemInstanceId() const { return ItemInstanceId; }
	AActor* GetSourceActor() const { return SourceActor.Get(); }
	UBoxComponent* GetCollisionComponent() const { return Collision; }

	/** Projects only an already-accepted P21.3 sample into local World presentation. */
	bool TryPresentThreatPresenceCue(
		const Fdemo_mapShanmenControlledWeaponWorldThreatSampleResult& Sample);
	/** Fail-safe presentation clear; does not alter the accepted sample watermark. */
	bool TryClearThreatPresenceCue(
		const FGuid& ExpectedRunId,
		const FGuid& ExpectedItemInstanceId);
	bool IsThreatPresenceCueActive() const
	{
		return bThreatPresenceCueActive;
	}
	bool IsThreatPresenceCueVisualActive() const;
	int32 GetThreatPresenceCueContactCount() const
	{
		return ThreatPresenceCueContactCount;
	}
	int64 GetLastThreatPresenceCueSampleSequence() const
	{
		return LastThreatPresenceCueSampleSequence;
	}
	const FGuid& GetLastThreatPresenceCueIntentId() const
	{
		return LastThreatPresenceCueIntentId;
	}

private:
	friend struct Fdemo_mapShanmenControlledWeaponWorldLifecycle;

	/** Publication is a no-fail final step after the P6 Host accepts the Actor. */
	void ActivateProductCollision();
	void DeactivateProductCollision();
	bool IsThreatPresenceCueStateValid() const;
	void RefreshThreatPresenceCue();

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UBoxComponent> Collision;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Visual;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UPointLightComponent> ThreatCueLight;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> VisualMaterial;

	UPROPERTY()
	TObjectPtr<AActor> SourceActor;

	UPROPERTY(VisibleAnywhere)
	FGuid RunId;

	UPROPERTY(VisibleAnywhere)
	FGuid ItemInstanceId;

	UPROPERTY(VisibleAnywhere)
	FGuid LastThreatPresenceCueIntentId;

	UPROPERTY(VisibleAnywhere)
	int64 LastThreatPresenceCueSampleSequence = INDEX_NONE;

	UPROPERTY(VisibleAnywhere)
	int32 ThreatPresenceCueContactCount = 0;

	UPROPERTY(VisibleAnywhere)
	bool bThreatPresenceCueActive = false;
};
