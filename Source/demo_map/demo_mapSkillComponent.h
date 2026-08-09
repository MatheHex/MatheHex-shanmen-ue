#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "demo_mapCombatTypes.h"
#include "demo_mapSkillComponent.generated.h"

class Ademo_mapSkillProjectile;

/** Owns V2-B player skill parameters, cooldowns, targeting, damage queries and projectile spawning. */
UCLASS(ClassGroup=(Gameplay), meta=(BlueprintSpawnableComponent))
class Udemo_mapSkillComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	Udemo_mapSkillComponent();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	const Fdemo_mapCircleSkillParams& GetCircleParams() const { return CircleParams; }
	const Fdemo_mapConeSkillParams& GetConeParams() const { return ConeParams; }
	const Fdemo_mapProjectileSkillParams& GetProjectileParams() const { return ProjectileParams; }

	bool ToggleGroundCircleTargeting();
	bool BeginGroundCircleTargeting();
	void CancelGroundCircleTargeting();
	void UpdateGroundTargetPreview(bool bHasGroundPoint, const FVector& GroundPoint);
	void SetAutomationGroundTargetPreview(bool bHasGroundPoint, const FVector& GroundPoint);
	bool ConfirmGroundCircle();
	bool TryCastGroundCircleAt(const FVector& GroundPoint, bool bHasGroundPoint = true);
	bool TryCastSelfSector(const FVector& AimDirection);
	Ademo_mapSkillProjectile* TryFireStraightProjectile(const FVector& AimDirection);
	Ademo_mapSkillProjectile* SpawnProjectile(const FVector& AimDirection);
	void CancelAllSkillState();

	bool IsGroundCircleTargeting() const { return bGroundCircleTargeting; }
	bool HasValidGroundPoint() const { return bPreviewHasGroundPoint; }
	bool IsGroundPointInRange() const { return bPreviewHasGroundPoint && bPreviewInRange; }
	bool IsPreviewExternallyControlled() const { return bExternalPreviewControl; }
	FVector GetPreviewGroundPoint() const { return PreviewGroundPoint; }
	bool IsCircleReady() const;
	bool IsConeReady() const;
	bool IsProjectileReady() const;
	float GetCircleCooldownRemaining() const;
	float GetConeCooldownRemaining() const;
	float GetProjectileCooldownRemaining() const;
	Ademo_mapSkillProjectile* GetLastSpawnedProjectile() const { return LastSpawnedProjectile.Get(); }

private:
	bool CanUseSkills() const;
	float GetWorldTime() const;
	int32 ApplyCircleDamage(const FVector& Center, float DamageSnapshot);
	int32 ApplySectorDamage(const FVector& Direction, float DamageSnapshot);
	void DrawTargetingPreview() const;
	void DrawCircleCastVisual(const FVector& Center) const;
	void DrawSectorCastVisual(const FVector& Direction) const;
	void DrawHitFeedback(const AActor* Target, const FColor& Color) const;
	void PruneProjectiles();

	UPROPERTY(EditDefaultsOnly, Category="Skills")
	Fdemo_mapCircleSkillParams CircleParams;

	UPROPERTY(EditDefaultsOnly, Category="Skills")
	Fdemo_mapConeSkillParams ConeParams;

	UPROPERTY(EditDefaultsOnly, Category="Skills")
	Fdemo_mapProjectileSkillParams ProjectileParams;

	UPROPERTY()
	TArray<TWeakObjectPtr<Ademo_mapSkillProjectile>> ActiveProjectiles;

	TWeakObjectPtr<Ademo_mapSkillProjectile> LastSpawnedProjectile;
	float CircleReadyTime = 0.0f;
	float ConeReadyTime = 0.0f;
	float ProjectileReadyTime = 0.0f;
	bool bGroundCircleTargeting = false;
	bool bPreviewHasGroundPoint = false;
	bool bPreviewInRange = false;
	bool bExternalPreviewControl = false;
	FVector PreviewGroundPoint = FVector::ZeroVector;
};
