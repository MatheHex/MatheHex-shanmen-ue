#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "demo_mapShanmenControlledWeaponActor.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
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

private:
	friend struct Fdemo_mapShanmenControlledWeaponWorldLifecycle;

	/** Publication is a no-fail final step after the P6 Host accepts the Actor. */
	void ActivateProductCollision();
	void DeactivateProductCollision();

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UBoxComponent> Collision;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Visual;

	UPROPERTY()
	TObjectPtr<AActor> SourceActor;

	UPROPERTY(VisibleAnywhere)
	FGuid RunId;

	UPROPERTY(VisibleAnywhere)
	FGuid ItemInstanceId;
};
