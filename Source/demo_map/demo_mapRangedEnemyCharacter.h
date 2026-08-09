#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "demo_mapCombatTypes.h"
#include "demo_mapEnemyEncounterTypes.h"
#include "demo_mapEnemySkillTypes.h"
#include "demo_mapRangedEnemyCharacter.generated.h"

class Ademo_mapSkillProjectile;
class UStaticMeshComponent;
class UTextRenderComponent;
class UPointLightComponent;
class Udemo_mapFactionComponent;
class UMaterialInstanceDynamic;
class Udemo_mapEnemySkillRuntimeComponent;

UENUM(BlueprintType)
enum class Edemo_mapRangedEnemyState : uint8
{
	Idle,
	Approach,
	Retreat,
	Windup,
	Fire,
	Cooldown,
	Dead
};

/** Timer-driven hostile that maintains range and fires a locked straight projectile. */
UCLASS()
class Ademo_mapRangedEnemyCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	Ademo_mapRangedEnemyCharacter();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	int32 GetMaxHealth() const { return MaxHealth; }
	int32 GetCurrentHealth() const { return CurrentHealth; }
	bool IsDead() const { return State == Edemo_mapRangedEnemyState::Dead; }
	Edemo_mapRangedEnemyState GetRangedState() const { return State; }
	bool HasActiveWindup() const { return State == Edemo_mapRangedEnemyState::Windup; }
	FVector GetLockedDirection() const { return LockedDirection; }
	float GetWindupDuration() const { return AttackWindup; }
	float GetAttackCooldown() const { return AttackCooldown; }
	float GetMovementSpeed() const { return MovementSpeed; }
	float GetRetreatThreshold() const { return RetreatStartRange; }
	float GetPreferredMinRange() const { return FireMinRange; }
	float GetPreferredMaxRange() const { return FireMaxRange; }
	float GetRetreatStartRange() const { return RetreatStartRange; }
	float GetRetreatStopRange() const { return RetreatStopRange; }
	int32 GetRetreatMoveRequestCount() const { return RetreatMoveRequestCount; }
	int32 GetRetreatFallbackCount() const { return RetreatFallbackCount; }
	int32 GetRetreatCandidateRejectCount() const { return RetreatCandidateRejectCount; }
	int32 GetTotalProjectilesFired() const { return TotalProjectilesFired; }
	const Fdemo_mapProjectileSkillParams& GetProjectileParams() const
	{
		return ProjectileParams;
	}
	int32 GetActiveProjectileCount() const;
	Ademo_mapSkillProjectile* GetLastProjectile() const { return LastProjectile.Get(); }
	Udemo_mapEnemySkillRuntimeComponent* GetEnemySkillRuntime() const
	{
		return EnemySkillRuntime;
	}
	bool HasLineOfSightToPlayer() const;
	void SetCombatSuppressed(bool bSuppressed);
	void ResetEnemySkillForNewRun();
	void ConfigureLegacyBehavior();
	bool UsesLegacyRangedBehavior() const { return SkillProfileId.IsNone(); }
	FName GetSkillProfileId() const { return SkillProfileId; }
	static bool IsLegacyRetreatRequired(
		float Distance,
		bool bCommittedRetreat,
		float RetreatStart,
		float RetreatStop)
	{
		return (bCommittedRetreat && Distance < RetreatStop)
			|| Distance < RetreatStart;
	}
	static bool IsLegacySafeRangeReached(float Distance, float RetreatStop)
	{
		return Distance >= RetreatStop;
	}
	static bool IsV2FinalRangedStageComplete(
		float Distance,
		int32 RetreatMoveRequests,
		int32 InitialProjectileCount,
		int32 CurrentProjectileCount,
		bool bHasActiveWindup)
	{
		return Distance >= 650.0f
			&& RetreatMoveRequests >= 1
			&& (CurrentProjectileCount > InitialProjectileCount
				|| bHasActiveWindup);
	}
	FGuid GetLootSourceId() const { return LootSourceId; }
	bool ConfigureEncounter(
		const Fdemo_mapEnemyEncounterIdentity& InIdentity,
		const Fdemo_mapEnemyCombatTuning& InTuning,
		bool bInEnhanced);
	const Fdemo_mapEnemyEncounterIdentity& GetEncounterIdentity() const
	{
		return EncounterIdentity;
	}
	bool IsEnhancedEncounter() const { return bEnhancedEncounter; }

private:
	void UpdateBehavior();
	void BeginWindup(APawn* PlayerPawn);
	void CompleteWindup();
	bool TryBeginBackstepShot(APawn* PlayerPawn, float Distance);
	void HandleBackstepResolve(
		const Fdemo_mapEnemySkillRuntimeSnapshot& Snapshot);
	bool FireTargetedProjectile(APawn* PlayerPawn, const FVector& Direction);
	void FinishFireState();
	void MoveTowardPlayer(APawn* PlayerPawn);
	void MoveAwayFromPlayer(APawn* PlayerPawn);
	bool TrySubmitRetreatMove(APawn* PlayerPawn);
	void HandleRetreatMoveFailure(APawn* PlayerPawn);
	void ResetRetreatFailure();
	void StopMovement();
	void CancelWindup();
	void CancelCombatAndProjectiles();
	void EnterDeadState();
	void DestroyAfterDeath();
	void RefreshPresentation();
	void ShowDamageFeedback();
	void ClearDamageFeedback();
	void DrawWindupFeedback() const;
	APawn* GetPlayerPawn() const;
	bool HasWorldStaticLineOfSight(const APawn* PlayerPawn) const;
	void PruneProjectiles();

	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> VisibleMesh;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> MuzzleMesh;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> Label;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UPointLightComponent> EnemyLight;
	UPROPERTY(VisibleAnywhere) TObjectPtr<Udemo_mapFactionComponent> FactionComponent;
	UPROPERTY(VisibleAnywhere) TObjectPtr<Udemo_mapEnemySkillRuntimeComponent> EnemySkillRuntime;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> VisibleMaterial;
	UPROPERTY() TArray<TWeakObjectPtr<Ademo_mapSkillProjectile>> ActiveProjectiles;
	TWeakObjectPtr<Ademo_mapSkillProjectile> LastProjectile;

	int32 MaxHealth = 3;
	int32 CurrentHealth = 3;
	Edemo_mapRangedEnemyState State = Edemo_mapRangedEnemyState::Idle;
	Fdemo_mapProjectileSkillParams ProjectileParams;
	FVector LockedDirection = FVector::ForwardVector;
	float MovementSpeed = 240.0f;
	float AggroRange = 1500.0f;
	float LoseAggroRange = 2000.0f;
	float RetreatStartRange = 550.0f;
	float RetreatStopRange = 700.0f;
	float FireMinRange = 550.0f;
	float FireMaxRange = 1050.0f;
	float AttackWindup = 0.40f;
	float AttackCooldown = 1.60f;
	float AIUpdateInterval = 0.18f;
	float LastMoveRequestTime = -1000.0f;
	float NextAttackAllowedTime = 0.0f;
	float RetreatFailureStartTime = -1.0f;
	float LastRetreatFallbackTime = -1000.0f;
	int32 TotalProjectilesFired = 0;
	int32 RetreatMoveRequestCount = 0;
	int32 RetreatFallbackCount = 0;
	int32 RetreatCandidateRejectCount = 0;
	bool bCombatSuppressed = false;
	bool bEnhancedEncounter = false;
	Fdemo_mapEnemyEncounterIdentity EncounterIdentity;
	FName SkillProfileId = NAME_None;
	FGuid LootSourceId = FGuid::NewGuid();
#if !UE_BUILD_SHIPPING
	bool bV2DiagLegacyLogged = false;
	bool bV2DiagRetreatLogged = false;
	bool bV2DiagSafeRangeLogged = false;
	bool bV2DiagWindupLogged = false;
	bool bV2DiagWindupCompleteLogged = false;
	bool bV2DiagProjectileLogged = false;
#endif
	FTimerHandle AIUpdateTimer;
	FTimerHandle WindupTimer;
	FTimerHandle FireStateTimer;
	FTimerHandle DestroyTimer;
	FTimerHandle DamageFeedbackTimer;
};
