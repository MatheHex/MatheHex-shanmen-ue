#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "demo_mapCombatTypes.h"
#include "demo_mapM01EnemyTypes.h"
#include "demo_mapM01BossCharacter.generated.h"

class Ademo_mapSkillProjectile;
class UStaticMeshComponent;
class UTextRenderComponent;
class UPointLightComponent;
class Udemo_mapFactionComponent;
class UMaterialInstanceDynamic;

UENUM(BlueprintType)
enum class Edemo_mapM01BossState : uint8
{
	Idle,
	Chase,
	Windup,
	Recovery,
	Dead
};

/** Single-stage M01 Boss with sweep, charge and ranged volley. */
UCLASS()
class Ademo_mapM01BossCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	Ademo_mapM01BossCharacter();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual float TakeDamage(
		float DamageAmount,
		FDamageEvent const& DamageEvent,
		AController* EventInstigator,
		AActor* DamageCauser) override;

	bool ConfigureBoss(const Fdemo_mapM01EnemyDefinition& InDefinition);
	void SetCombatSuppressed(bool bSuppressed);
	bool IsDead() const { return State == Edemo_mapM01BossState::Dead; }
	int32 GetCurrentHealth() const { return CurrentHealth; }
	int32 GetMaxHealth() const { return MaxHealth; }
	Edemo_mapM01BossAttack GetLastAttack() const { return LastAttack; }
	int32 GetSweepResolveCount() const { return SweepResolveCount; }
	int32 GetChargeResolveCount() const { return ChargeResolveCount; }
	int32 GetVolleyResolveCount() const { return VolleyResolveCount; }

private:
	void UpdateBehavior();
	void BeginAttack(Edemo_mapM01BossAttack Attack, APawn* PlayerPawn);
	void ResolveAttack();
	void FinishRecovery();
	void EnterDeadState();
	void DestroyAfterDeath();
	void StopMovement();
	void CancelCombat();
	void RefreshPresentation();
	void ShowDamageFeedback();
	void ClearDamageFeedback();
	void DrawTelegraph(FColor Color, float Duration, float Thickness) const;
	bool HasWorldStaticLineOfSight(const APawn* PlayerPawn) const;
	APawn* GetPlayerPawn() const;
	void PruneProjectiles();

	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> BodyMesh;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> CrownMesh;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> CoreMesh;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> Label;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UPointLightComponent> BossLight;
	UPROPERTY(VisibleAnywhere) TObjectPtr<Udemo_mapFactionComponent> FactionComponent;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> BodyMaterial;
	UPROPERTY() TArray<TWeakObjectPtr<Ademo_mapSkillProjectile>> ActiveProjectiles;

	Fdemo_mapM01EnemyDefinition Definition;
	FVector HomeLocation = FVector::ZeroVector;
	FVector LockedDirection = FVector::ForwardVector;
	Fdemo_mapProjectileSkillParams ProjectileParams;
	Edemo_mapM01BossState State = Edemo_mapM01BossState::Idle;
	Edemo_mapM01BossAttack PendingAttack = Edemo_mapM01BossAttack::Sweep;
	Edemo_mapM01BossAttack LastAttack = Edemo_mapM01BossAttack::Sweep;
	FGuid LootSourceId = FGuid::NewGuid();
	int32 MaxHealth = 34;
	int32 CurrentHealth = 34;
	float MovementSpeed = 235.0f;
	float AttackDamage = 3.0f;
	float AttackCooldown = 2.20f;
	float NextAttackAllowedTime = 0.0f;
	float LastMoveRequestTime = -1000.0f;
	int32 SweepResolveCount = 0;
	int32 ChargeResolveCount = 0;
	int32 VolleyResolveCount = 0;
	bool bCombatSuppressed = false;
	bool bDeathCommitted = false;
	FTimerHandle WindupTimer;
	FTimerHandle RecoveryTimer;
	FTimerHandle DestroyTimer;
	FTimerHandle DamageFeedbackTimer;
};

