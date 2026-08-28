#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "demo_mapCombatVitalityHost.h"
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
class Ademo_mapM01BossCharacter : public ACharacter,
	public Idemo_mapCombatVitalityHost
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
	int32 GetCurrentHealth() const { return FMath::CeilToInt(CurrentVitality); }
	int32 GetMaxHealth() const { return FMath::CeilToInt(MaximumVitality); }
	float GetCurrentVitality() const { return CurrentVitality; }
	float GetMaximumVitality() const { return MaximumVitality; }
	Edemo_mapM01BossAttack GetLastAttack() const { return LastAttack; }
	int32 GetSweepResolveCount() const { return SweepResolveCount; }
	int32 GetChargeResolveCount() const { return ChargeResolveCount; }
	int32 GetVolleyResolveCount() const { return VolleyResolveCount; }
	uint64 GetNextAttackSequence() const { return NextAttackSequence; }
	uint64 GetActiveAttackSequence() const { return ActiveAttackSequence; }
	void ResetBossAttackForNewRun();
	virtual bool TryBindCombatEntity(
		const FGuid& TargetEntityId) override;
	virtual bool TryEndCombatEntityBinding(
		const FGuid& ExpectedTargetEntityId) override;
	virtual bool IsCombatEntityBound() const override
	{
		return CombatVitalityLedger.IsValid();
	}
	virtual const FGuid& GetCombatEntityId() const override
	{
		return CombatVitalityLedger.GetTargetEntityId();
	}
	virtual int64 GetCombatAuthorityRevision() const override
	{
		return CombatVitalityLedger.GetAuthorityRevision();
	}
	virtual int32 NumCommittedCombatImpacts() const override
	{
		return CombatVitalityLedger.NumCommittedImpacts();
	}
	virtual bool TryCaptureCombatVitalitySnapshot(
		FShanmenTargetVitalitySnapshot& OutSnapshot) const override;
	virtual FShanmenVitalityCommitResult CommitCombatImpact(
		const FShanmenVitalityCommitCommand& Command) override;

#if WITH_DEV_AUTOMATION_TESTS
	virtual int32 GetPositiveCombatDamageCountForAutomation() const override
	{
		return PositiveCombatDamageCount;
	}
#endif

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
	bool TryCommitVitalityState(
		float NewCurrentVitality,
		float NewMaximumVitality);
	void PublishAppliedDamage(float AppliedDamage);
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
	float MaximumVitality = 34.0f;
	float CurrentVitality = 34.0f;
	float MovementSpeed = 235.0f;
	float AttackDamage = 3.0f;
	float AttackCooldown = 2.20f;
	float NextAttackAllowedTime = 0.0f;
	float LastMoveRequestTime = -1000.0f;
	int32 SweepResolveCount = 0;
	int32 ChargeResolveCount = 0;
	int32 VolleyResolveCount = 0;
	uint64 NextAttackSequence = 1;
	uint64 ActiveAttackSequence = 0;
	bool bCombatSuppressed = false;
	bool bDeathCommitted = false;
	FShanmenVitalityCommitLedger CombatVitalityLedger;
#if WITH_DEV_AUTOMATION_TESTS
	int32 PositiveCombatDamageCount = 0;
#endif
	FTimerHandle WindupTimer;
	FTimerHandle RecoveryTimer;
	FTimerHandle DestroyTimer;
	FTimerHandle DamageFeedbackTimer;
};
