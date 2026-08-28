#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "demo_mapCombatTypes.h"
#include "demo_mapSkillProjectile.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;
class UPointLightComponent;
class UPrimitiveComponent;

/** Minimal straight projectile used by the V2-B player skill component. */
UCLASS()
class Ademo_mapSkillProjectile : public AActor
{
	GENERATED_BODY()

public:
	Ademo_mapSkillProjectile();
	void InitializeProjectile(AActor* InSourceActor, const FVector& Direction, const Fdemo_mapProjectileSkillParams& InParams);
	void InitializeProjectileWithLaunchSegment(AActor* InSourceActor, const FVector& Direction, const Fdemo_mapProjectileSkillParams& InParams, const FVector& AttackOrigin);
	void InitializeTargetedProjectile(AActor* InSourceActor, AActor* InIntendedTarget, const FVector& Direction, const Fdemo_mapProjectileSkillParams& InParams, const FLinearColor& InVisualColor);
	void InitializeTargetedEnemyProjectile(
		AActor* InSourceActor,
		AActor* InIntendedTarget,
		const FVector& Direction,
		const Fdemo_mapProjectileSkillParams& InParams,
		const FLinearColor& InVisualColor,
		FName InSkillProfileId,
		uint64 InProjectileSequence);

	float GetConfiguredSpeed() const { return ProjectileParams.Speed; }
	float GetConfiguredWidth() const { return ProjectileParams.Width; }
	float GetConfiguredCollisionRadius() const { return ProjectileParams.CollisionRadius; }
	float GetConfiguredMaxDistance() const { return ProjectileParams.MaxDistance; }
	FVector GetInitialLocation() const { return InitialLocation; }
	bool HasBeenConsumed() const { return bConsumed; }
	AActor* GetIntendedTarget() const { return IntendedTarget.Get(); }
	bool IsIntendedTargetOnly() const { return bIntendedTargetOnly; }
	FName GetSourceSkillProfileId() const { return SourceSkillProfileId; }
	uint64 GetProjectileSequence() const { return ProjectileSequence; }

private:
	UFUNCTION()
	void HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	UFUNCTION()
	void HandleHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit);
	bool HandleProjectileContact(AActor* OtherActor, UPrimitiveComponent* OtherComponent, const FHitResult* HitResult, bool bBlockingHit);
	bool ResolveLaunchSegment(const FVector& AttackOrigin, const FVector& SpawnLocation);
	void ActivateForFlight();
	void ConsumeAt(const FVector& Location, const FColor& Color);
	void MarkConsumed();
	bool IsAlreadyDefeated(const AActor* OtherActor) const;
	void ApplyConfiguration(AActor* InSourceActor, const FVector& Direction, const Fdemo_mapProjectileSkillParams& InParams, const FLinearColor& InVisualColor);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USphereComponent> Collision;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> VisibleMesh;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UProjectileMovementComponent> Movement;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UPointLightComponent> ProjectileLight;

	UPROPERTY()
	TObjectPtr<AActor> SourceActor;
	TWeakObjectPtr<AActor> IntendedTarget;

	Fdemo_mapProjectileSkillParams ProjectileParams;
	FName SourceSkillProfileId = NAME_None;
	uint64 ProjectileSequence = 0;
	FVector InitialLocation = FVector::ZeroVector;
	bool bConsumed = false;
	bool bIntendedTargetOnly = false;
	TSet<TWeakObjectPtr<AActor>> ContactedActors;
	FLinearColor VisualColor = FLinearColor(0.0f, 0.75f, 1.0f);
};
