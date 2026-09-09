#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShanmenThrownWeaponExecution.h"
#include "ShanmenWorldHitAdapter.h"

#include "demo_mapShanmenThrownWeaponProjectile.generated.h"

class UProjectileMovementComponent;
class UBoxComponent;
class UMaterialInstanceDynamic;
class UPointLightComponent;
class URotatingMovementComponent;
class USceneComponent;
class UStaticMeshComponent;
struct FHitResult;
struct Fdemo_mapShanmenThrownWeaponWorldAdapter;

/** Product-visible lifecycle of one physical thrown item. */
enum class Edemo_mapShanmenThrownWeaponProjectileState : uint8
{
	Empty,
	/** Frozen launch evidence exists, but collision and movement are still inert. */
	Staged,
	InFlight,
	Spent
};

/** Exact reason why an inert carrier refused one staging request. */
enum class Edemo_mapShanmenThrownWeaponProjectileStageError : uint8
{
	None,
	ContractRejected,
	ReleasePathBlocked
};

class Ademo_mapShanmenThrownWeaponProjectile;

/** Native contact seam consumed by the product host; the Actor never resolves damage. */
DECLARE_MULTICAST_DELEGATE_TwoParams(
	Fdemo_mapShanmenThrownWeaponContact,
	Ademo_mapShanmenThrownWeaponProjectile&,
	const FHitResult&);

/** Native lifecycle seam used instead of a gameplay timer in the Actor. */
DECLARE_MULTICAST_DELEGATE_OneParam(
	Fdemo_mapShanmenThrownWeaponRangeExpired,
	Ademo_mapShanmenThrownWeaponProjectile&);

/**
 * Minimal physical carrier for the shared straight/ballistic launch contract.
 *
 * Staging is deliberately inert. Only an exact durable Quantity commit may
 * call ActivateCommittedLaunch. Geometry is emitted through OnContact; this
 * Actor never chooses a target, mutates inventory, or applies damage itself.
 */
UCLASS()
class Ademo_mapShanmenThrownWeaponProjectile : public AActor
{
	GENERATED_BODY()

public:
	Ademo_mapShanmenThrownWeaponProjectile();

	bool TryStageLaunch(
		const FShanmenThrownWeaponLaunchReceipt& InLaunch,
		const FShanmenWorldHitContext& InContext,
		AActor* InSourceActor,
		Edemo_mapShanmenThrownWeaponProjectileStageError* OutError = nullptr);
	bool IsStagedFor(
		const FShanmenThrownWeaponLaunchReceipt& InLaunch,
		const FShanmenWorldHitContext& InContext) const;
	bool IsInFlightFor(
		const FShanmenThrownWeaponLaunchReceipt& InLaunch,
		const FShanmenWorldHitContext& InContext) const;
	bool CancelStagedLaunch();

	Edemo_mapShanmenThrownWeaponProjectileState GetProjectileState() const
	{
		return State;
	}
	const FShanmenThrownWeaponLaunchReceipt& GetLaunchReceipt() const
	{
		return LaunchReceipt;
	}
	const FShanmenWorldHitContext& GetHitContext() const
	{
		return HitContext;
	}
	UBoxComponent* GetCollisionComponent() const { return Collision; }
	UProjectileMovementComponent* GetMovementComponent() const
	{
		return Movement;
	}
	/** The prototype knife silhouette is visible only after durable publication. */
	bool IsPresentationVisible() const;
	/** Blade body, edge, and grip retain distinct prototype colors. */
	bool HasPresentationMaterialContrast() const;
	/** World-space forward direction of the collisionless presentation pivot. */
	FVector GetPresentationForwardDirection() const;
	/** World-space blade-up direction used to prove presentation-only roll. */
	FVector GetPresentationUpDirection() const;
	/** Roll is active only while the durable projectile is in flight. */
	bool IsPresentationRollActive() const;
	/** Attached readability cue is active only during durable flight. */
	bool IsFlightCueVisible() const;
	/** Actual trajectory-coded color held by the attached flight cue. */
	FLinearColor GetFlightCueColor() const;
	Fdemo_mapShanmenThrownWeaponContact& OnContact() { return ContactEvent; }
	Fdemo_mapShanmenThrownWeaponRangeExpired& OnRangeExpired()
	{
		return RangeExpiredEvent;
	}

protected:
	virtual void PostInitializeComponents() override;
	virtual void LifeSpanExpired() override;

private:
	friend struct Fdemo_mapShanmenThrownWeaponWorldAdapter;

	/** No-fail publication reserved for the durable product adapter. */
	void ActivateCommittedLaunch();
	bool MarkSpent();
	bool IsPresentationGeometryValid() const;
	void SetPresentationVisibility(bool bVisible);
	void RefreshFlightCue();
	void ResetPresentationRoll();

	UFUNCTION()
	void HandleProjectileStop(const FHitResult& Hit);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UBoxComponent> Collision;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UProjectileMovementComponent> Movement;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> PresentationPivot;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Visual;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> BladeEdgeVisual;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> GripVisual;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> BladeMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> BladeEdgeMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> GripMaterial;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<URotatingMovementComponent> VisualRoll;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UPointLightComponent> FlightCueLight;

	UPROPERTY()
	TObjectPtr<AActor> SourceActor;

	FShanmenThrownWeaponLaunchReceipt LaunchReceipt;
	FShanmenWorldHitContext HitContext;
	Fdemo_mapShanmenThrownWeaponContact ContactEvent;
	Fdemo_mapShanmenThrownWeaponRangeExpired RangeExpiredEvent;
	Edemo_mapShanmenThrownWeaponProjectileState State =
		Edemo_mapShanmenThrownWeaponProjectileState::Empty;
};
