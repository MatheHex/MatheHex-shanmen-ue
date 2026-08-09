#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "demo_mapEnemyEncounterTypes.h"
#include "demo_mapHeavyEnemyCharacter.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;
class UPointLightComponent;
class Udemo_mapFactionComponent;
class UMaterialInstanceDynamic;

UENUM(BlueprintType)
enum class Edemo_mapHeavyEnemyState : uint8
{
	Idle,
	Chase,
	Windup,
	Resolve,
	Recovery,
	Dead
};

/** Slow hostile with a locked, telegraphed, single-resolution sector attack. */
UCLASS()
class Ademo_mapHeavyEnemyCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	Ademo_mapHeavyEnemyCharacter();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	int32 GetMaxHealth() const { return MaxHealth; }
	int32 GetCurrentHealth() const { return CurrentHealth; }
	bool IsDead() const { return State == Edemo_mapHeavyEnemyState::Dead; }
	Edemo_mapHeavyEnemyState GetHeavyState() const { return State; }
	bool HasActiveTelegraph() const { return State == Edemo_mapHeavyEnemyState::Windup; }
	FVector GetLockedDirection() const { return LockedDirection; }
	float GetMovementSpeed() const { return MovementSpeed; }
	float GetAttackRange() const { return AttackRange; }
	float GetSectorRadius() const { return SectorRadius; }
	float GetFullAngleDegrees() const { return FullAngleDegrees; }
	float GetWindupDuration() const { return WindupDuration; }
	float GetRecoveryDuration() const { return RecoveryDuration; }
	float GetAttackCooldown() const { return AttackCooldown; }
	int32 GetResolveCount() const { return ResolveCount; }
	void SetCombatSuppressed(bool bSuppressed);
	FGuid GetLootSourceId() const { return LootSourceId; }
	bool ConfigureEncounter(
		const Fdemo_mapEnemyEncounterIdentity& InIdentity,
		const Fdemo_mapEnemyCombatTuning& InTuning);
	const Fdemo_mapEnemyEncounterIdentity& GetEncounterIdentity() const
	{
		return EncounterIdentity;
	}

private:
	void UpdateBehavior();
	void BeginWindup(APawn* PlayerPawn);
	void ResolveAttack();
	void FinishRecovery();
	void StopMovement();
	void CancelPendingAttack();
	void EnterDeadState();
	void DestroyAfterDeath();
	void RefreshPresentation();
	void ShowDamageFeedback();
	void ClearDamageFeedback();
	void DrawSectorFeedback(const FColor& Color, float Duration, float Thickness) const;
	bool HasWorldStaticLineOfSight(const APawn* PlayerPawn) const;
	APawn* GetPlayerPawn() const;

	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> VisibleMesh;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> ShoulderMesh;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> Label;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UPointLightComponent> EnemyLight;
	UPROPERTY(VisibleAnywhere) TObjectPtr<Udemo_mapFactionComponent> FactionComponent;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> VisibleMaterial;

	int32 MaxHealth = 5;
	int32 CurrentHealth = 5;
	Edemo_mapHeavyEnemyState State = Edemo_mapHeavyEnemyState::Idle;
	FVector LockedDirection = FVector::ForwardVector;
	float MovementSpeed = 180.0f;
	float AggroRange = 1100.0f;
	float LoseAggroRange = 1500.0f;
	float AttackRange = 420.0f;
	float SectorRadius = 430.0f;
	float FullAngleDegrees = 100.0f;
	float AttackDamage = 1.0f;
	float WindupDuration = 0.85f;
	float RecoveryDuration = 0.50f;
	float AttackCooldown = 2.40f;
	float VerticalTolerance = 180.0f;
	float AIUpdateInterval = 0.18f;
	float LastMoveRequestTime = -1000.0f;
	float NextAttackAllowedTime = 0.0f;
	int32 ResolveCount = 0;
	bool bCombatSuppressed = false;
	Fdemo_mapEnemyEncounterIdentity EncounterIdentity;
	FGuid LootSourceId = FGuid::NewGuid();
	FTimerHandle AIUpdateTimer;
	FTimerHandle WindupTimer;
	FTimerHandle RecoveryTimer;
	FTimerHandle DestroyTimer;
	FTimerHandle DamageFeedbackTimer;
};
