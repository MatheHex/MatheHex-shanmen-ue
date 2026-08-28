#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ShanmenVitalityAuthority.h"
#include "demo_mapCombatVitalityHost.h"
#include "demo_mapEnemyEncounterTypes.h"
#include "demo_mapEnemySkillTypes.h"
#include "demo_mapEnemyCharacter.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;
class UMaterialInstanceDynamic;
class UPointLightComponent;
class Udemo_mapFactionComponent;
class Udemo_mapEnemySkillRuntimeComponent;

UENUM()
enum class Edemo_mapEnemyState : uint8
{
	Idle,
	Chase,
	Attack,
	Dead
};

/** A single minimal NavMesh-driven hostile melee enemy. */
UCLASS()
class Ademo_mapEnemyCharacter : public ACharacter,
	public Idemo_mapCombatVitalityHost
{
	GENERATED_BODY()

public:
	Ademo_mapEnemyCharacter();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Compatibility projections for retained integer UI and mission checks. */
	int32 GetMaxHealth() const { return FMath::CeilToInt(MaximumVitality); }
	int32 GetCurrentHealth() const { return FMath::CeilToInt(CurrentVitality); }
	float GetMaximumVitality() const { return MaximumVitality; }
	float GetCurrentVitality() const { return CurrentVitality; }
	bool IsDead() const { return EnemyState == Edemo_mapEnemyState::Dead; }
	Edemo_mapEnemyState GetEnemyState() const { return EnemyState; }
	float GetAttackCooldown() const { return AttackCooldown; }
	float GetAttackDamage() const { return AttackDamage; }
	Udemo_mapEnemySkillRuntimeComponent* GetEnemySkillRuntime() const
	{
		return EnemySkillRuntime;
	}
	void SetCombatSuppressed(bool bSuppressed);
	void ResetEnemySkillForNewRun();
	FGuid GetLootSourceId() const { return LootSourceId; }
	bool ConfigureEncounter(
		const Fdemo_mapEnemyEncounterIdentity& InIdentity,
		const Fdemo_mapEnemyCombatTuning& InTuning,
		bool bInEnhanced);
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
	const Fdemo_mapEnemyEncounterIdentity& GetEncounterIdentity() const
	{
		return EncounterIdentity;
	}
	float GetMovementSpeed() const { return MovementSpeed; }
	bool IsEnhancedEncounter() const { return bEnhancedEncounter; }

#if WITH_DEV_AUTOMATION_TESTS
	virtual int32 GetPositiveCombatDamageCountForAutomation() const override
	{
		return PositiveCombatDamageCount;
	}
#endif

private:
	void UpdateBehavior();
	void AttackPlayer(APawn* PlayerPawn);
	bool TryBeginDash(APawn* PlayerPawn, float Distance);
	void HandleDashSegment(
		const Fdemo_mapEnemySkillDisplacementSegment& Segment);
	void EnterDeadState();
	void DestroyAfterDeath();
	void ClearAttackFeedback();
	void ClearDamageFeedback();
	void ShowDamageFeedback();
	void RefreshPresentation();
	bool TryCommitVitalityState(
		float NewCurrentVitality,
		float NewMaximumVitality);
	void PublishAppliedDamage(float AppliedDamage);
	APawn* GetPlayerPawn() const;

	UPROPERTY(VisibleAnywhere, Category="Enemy")
	TObjectPtr<UStaticMeshComponent> VisibleMesh;

	UPROPERTY(VisibleAnywhere, Category="Enemy")
	TObjectPtr<UTextRenderComponent> Label;

	UPROPERTY(VisibleAnywhere, Category="Enemy")
	TObjectPtr<UPointLightComponent> EnemyLight;

	UPROPERTY(VisibleAnywhere, Category="Combat")
	TObjectPtr<Udemo_mapFactionComponent> FactionComponent;

	UPROPERTY(VisibleAnywhere, Category="Combat")
	TObjectPtr<Udemo_mapEnemySkillRuntimeComponent> EnemySkillRuntime;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> VisibleMaterial;

	UPROPERTY(VisibleAnywhere, Category="Enemy")
	float MaximumVitality = 3.0f;

	UPROPERTY(VisibleAnywhere, Category="Enemy")
	float CurrentVitality = 3.0f;

	UPROPERTY(VisibleAnywhere, Category="Enemy")
	Edemo_mapEnemyState EnemyState = Edemo_mapEnemyState::Idle;

	float AggroRange = 850.0f;
	float LeashRange = 1400.0f;
	float AttackRange = 135.0f;
	float AttackDamage = 1.0f;
	float AttackCooldown = 1.20f;
	float MovementSpeed = 260.0f;
	float LastAttackTime = -1000.0f;
	float LastMoveRequestTime = -1000.0f;
	bool bCombatSuppressed = false;
	bool bEnhancedEncounter = false;
	Fdemo_mapEnemyEncounterIdentity EncounterIdentity;
	FName SkillProfileId =
		Fdemo_mapEnemySkillProfileIds::StandardMeleeDash;
	FGuid LootSourceId = FGuid::NewGuid();
	FShanmenVitalityCommitLedger CombatVitalityLedger;
#if WITH_DEV_AUTOMATION_TESTS
	int32 PositiveCombatDamageCount = 0;
#endif
	FTimerHandle DestroyTimerHandle;
	FTimerHandle AttackFeedbackTimer;
	FTimerHandle DamageFeedbackTimer;
};
