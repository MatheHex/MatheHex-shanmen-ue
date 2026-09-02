#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShanmenSwordQiExecution.h"
#include "ShanmenWorldHitAdapter.h"

#include "demo_mapShanmenSwordQiProjectile.generated.h"

class UPrimitiveComponent;
class UProjectileMovementComponent;
class USphereComponent;
struct FHitResult;
struct Fdemo_mapShanmenSwordQiWorldAdapter;

/** Product-visible lifecycle for one physical sword-qi carrier. */
enum class Edemo_mapShanmenSwordQiProjectileState : uint8
{
	Empty,
	/** Launch evidence exists, but collision and movement remain inert. */
	Staged,
	InFlight,
	Dissipated
};

class Ademo_mapShanmenSwordQiProjectile;

/** Native contact seam; the carrier never chooses targets or applies damage. */
DECLARE_MULTICAST_DELEGATE_TwoParams(
	Fdemo_mapShanmenSwordQiContact,
	Ademo_mapShanmenSwordQiProjectile&,
	const FHitResult&);

/** Native range/lifespan seam consumed by a later product host. */
DECLARE_MULTICAST_DELEGATE_OneParam(
	Fdemo_mapShanmenSwordQiRangeExpired,
	Ademo_mapShanmenSwordQiProjectile&);

/**
 * Minimal physical carrier for the basic sword-qi contract.
 *
 * Staging is collision-inert so a rejected world binding cannot leave a live
 * projectile behind. Contacts are forwarded as geometry only; this Actor does
 * not own action phases, damage, vitality, item authority, or presentation.
 */
UCLASS()
class Ademo_mapShanmenSwordQiProjectile : public AActor
{
	GENERATED_BODY()

public:
	Ademo_mapShanmenSwordQiProjectile();

	bool TryStageLaunch(
		const FShanmenSwordQiLaunchReceipt& InLaunch,
		const FShanmenWorldHitContext& InContext,
		AActor* InSourceActor);
	bool IsStagedFor(
		const FShanmenSwordQiLaunchReceipt& InLaunch,
		const FShanmenWorldHitContext& InContext) const;
	bool IsInFlightFor(
		const FShanmenSwordQiLaunchReceipt& InLaunch,
		const FShanmenWorldHitContext& InContext) const;
	bool CancelStagedLaunch();

	Edemo_mapShanmenSwordQiProjectileState GetProjectileState() const
	{
		return State;
	}
	const FShanmenSwordQiLaunchReceipt& GetLaunchReceipt() const
	{
		return LaunchReceipt;
	}
	const FShanmenWorldHitContext& GetHitContext() const
	{
		return HitContext;
	}
	USphereComponent* GetCollisionComponent() const { return Collision; }
	UProjectileMovementComponent* GetMovementComponent() const
	{
		return Movement;
	}
	Fdemo_mapShanmenSwordQiContact& OnContact() { return ContactEvent; }
	Fdemo_mapShanmenSwordQiRangeExpired& OnRangeExpired()
	{
		return RangeExpiredEvent;
	}

protected:
	virtual void LifeSpanExpired() override;

private:
	friend struct Fdemo_mapShanmenSwordQiWorldAdapter;

	/** No-fail publication reserved for the copy-on-write world adapter. */
	void ActivateStagedLaunch();
	bool MarkDissipated();

	UFUNCTION()
	void HandleHit(
		UPrimitiveComponent* HitComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		FVector NormalImpulse,
		const FHitResult& Hit);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USphereComponent> Collision;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UProjectileMovementComponent> Movement;

	UPROPERTY()
	TObjectPtr<AActor> SourceActor;

	FShanmenSwordQiLaunchReceipt LaunchReceipt;
	FShanmenWorldHitContext HitContext;
	Fdemo_mapShanmenSwordQiContact ContactEvent;
	Fdemo_mapShanmenSwordQiRangeExpired RangeExpiredEvent;
	Edemo_mapShanmenSwordQiProjectileState State =
		Edemo_mapShanmenSwordQiProjectileState::Empty;
};
