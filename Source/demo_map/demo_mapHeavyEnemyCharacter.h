#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "demo_mapCombatVitalityHost.h"
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
class Ademo_mapHeavyEnemyCharacter : public ACharacter,
	public Idemo_mapCombatVitalityHost
{
	GENERATED_BODY()

public:
	Ademo_mapHeavyEnemyCharacter();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	int32 GetMaxHealth() const { return FMath::CeilToInt(MaximumVitality); }
	int32 GetCurrentHealth() const { return FMath::CeilToInt(CurrentVitality); }
	float GetMaximumVitality() const { return MaximumVitality; }
	float GetCurrentVitality() const { return CurrentVitality; }
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
	uint64 GetNextAttackSequence() const { return NextAttackSequence; }
	uint64 GetActiveAttackSequence() const { return ActiveAttackSequence; }
	void SetCombatSuppressed(bool bSuppressed);
	void ResetHeavyAttackForNewRun();
	FGuid GetLootSourceId() const { return LootSourceId; }
	bool ConfigureEncounter(
		const Fdemo_mapEnemyEncounterIdentity& InIdentity,
		const Fdemo_mapEnemyCombatTuning& InTuning);
	const Fdemo_mapEnemyEncounterIdentity& GetEncounterIdentity() const
	{
		return EncounterIdentity;
	}
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
	bool TryCommitVitalityState(
		float NewCurrentVitality,
		float NewMaximumVitality);
	void PublishAppliedDamage(float AppliedDamage);
	void DrawSectorFeedback(const FColor& Color, float Duration, float Thickness) const;
	bool HasWorldStaticLineOfSight(const APawn* PlayerPawn) const;
	APawn* GetPlayerPawn() const;

	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> VisibleMesh;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> ShoulderMesh;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> Label;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UPointLightComponent> EnemyLight;
	UPROPERTY(VisibleAnywhere) TObjectPtr<Udemo_mapFactionComponent> FactionComponent;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> VisibleMaterial;

	float MaximumVitality = 5.0f;
	float CurrentVitality = 5.0f;
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
	uint64 NextAttackSequence = 1;
	uint64 ActiveAttackSequence = 0;
	bool bCombatSuppressed = false;
	Fdemo_mapEnemyEncounterIdentity EncounterIdentity;
	FGuid LootSourceId = FGuid::NewGuid();
	FShanmenVitalityCommitLedger CombatVitalityLedger;
#if WITH_DEV_AUTOMATION_TESTS
	int32 PositiveCombatDamageCount = 0;
#endif
	FTimerHandle AIUpdateTimer;
	FTimerHandle WindupTimer;
	FTimerHandle RecoveryTimer;
	FTimerHandle DestroyTimer;
	FTimerHandle DamageFeedbackTimer;
};
