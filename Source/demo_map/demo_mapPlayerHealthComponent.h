#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ShanmenVitalityAuthority.h"
#include "demo_mapAttributeTypes.h"
#include "demo_mapPlayerHealthComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(Fdemo_mapPlayerDefeatedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	Fdemo_mapPlayerDamagedSignature,
	int32,
	AppliedDamage);

/** Runtime player vitality attached to the existing BP_TopDownCharacter. */
UCLASS(ClassGroup=(Gameplay), meta=(BlueprintSpawnableComponent))
class Udemo_mapPlayerHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	Udemo_mapPlayerHealthComponent();
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void BeginPlay() override;

	/** Compatibility projection for the existing integer UI and item surfaces. */
	int32 GetMaxHealth() const { return FMath::CeilToInt(MaximumVitality); }
	/** Positive fractional vitality remains visibly alive in legacy integer UI. */
	int32 GetCurrentHealth() const { return FMath::CeilToInt(CurrentVitality); }
	float GetMaximumVitality() const { return MaximumVitality; }
	float GetCurrentVitality() const { return CurrentVitality; }
	bool IsDefeated() const { return bIsDefeated; }
	static bool ShouldDodge(float DodgeChance, float NormalizedRoll);
	static int32 ResolveAppliedDamage(float RawDamage, float FlatDamageReduction);
	int32 ApplyIncomingDamage(float RawDamage);
	int32 ApplyHealing(int32 RequestedHealing);
	/** P15 Code A receipt adapter: applies at most once for this pawn/run health lifetime. */
	bool ApplyRestoreHealthReceipt(const FGuid& ReceiptId, int32 RestoreAmount, bool& bOutAlreadyProcessed);
	void RestoreCurrentVitalityAfterItemUseRollback(float PreviousVitality);
	bool BindAttributeComponent(
		class Udemo_mapAttributeComponent* Attributes,
		bool bInitializeCurrentHealth);
	/**
	 * Binds the stable World EntityId supplied by the run/spawn registry.
	 * A live component may be rebound only to the same id; no pointer-derived or
	 * random fallback identity is manufactured here.
	 */
	bool TryBindCombatEntity(const FGuid& TargetEntityId);
	/** Releases only the exact Run identity so a persistent Pawn can enter a later Run. */
	bool TryEndCombatEntityBinding(const FGuid& ExpectedTargetEntityId);
	bool IsCombatEntityBound() const { return CombatVitalityLedger.IsValid(); }
	const FGuid& GetCombatEntityId() const { return CombatVitalityLedger.GetTargetEntityId(); }
	int64 GetCombatAuthorityRevision() const { return CombatVitalityLedger.GetAuthorityRevision(); }
	int32 NumCommittedCombatImpacts() const { return CombatVitalityLedger.NumCommittedImpacts(); }
	bool TryCaptureCombatVitalitySnapshot(FShanmenTargetVitalitySnapshot& OutSnapshot) const;
	/**
	 * Captures legacy player avoidance and flat reduction as deterministic,
	 * ordered canonical defense layers for one stable ImpactId.
	 */
	bool TryCaptureCombatDefenseSnapshot(
		const FGuid& ImpactId,
		FShanmenDefenseSnapshot& OutSnapshot) const;
	/** Applies already-resolved final damage without rerunning legacy defense or ApplyDamage. */
	FShanmenVitalityCommitResult CommitCombatImpact(const FShanmenVitalityCommitCommand& Command);

#if !UE_BUILD_SHIPPING
	void SetCurrentHealthForAutomation(int32 NewHealth);
#endif
#if WITH_DEV_AUTOMATION_TESTS
	int32 GetPositiveDamageBroadcastCountForAutomation() const
	{
		return PositiveDamageBroadcastCount;
	}
#endif

	UPROPERTY(BlueprintAssignable, Category="Health")
	Fdemo_mapPlayerDefeatedSignature OnPlayerDefeated;

	UPROPERTY(BlueprintAssignable, Category="Health")
	Fdemo_mapPlayerDamagedSignature OnPlayerDamaged;

private:
	UFUNCTION()
	void HandleOwnerDamaged(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser);
	void EnterDefeatedState();
	void ClearDamageFeedback();
	void HandleAttributeChanged(const Fdemo_mapAttributeChange& Change);
	void BindAttributes();
	void ApplyMaxHealthFromAttributes(bool bInitial);
	bool TryCommitVitalityState(float NewCurrentVitality, float NewMaximumVitality);
	void PublishAppliedDamage(float AppliedDamage);

	/** Sole mutable player-health truth; the combat ledger stores fingerprints only. */
	UPROPERTY(VisibleAnywhere, Category="Health")
	float MaximumVitality = 5.0f;

	UPROPERTY(VisibleAnywhere, Category="Health")
	float CurrentVitality = 5.0f;

	UPROPERTY(VisibleAnywhere, Category="Health")
	bool bIsDefeated = false;

	FTimerHandle DamageFeedbackTimer;
	FDelegateHandle AttributeChangedHandle;
	TWeakObjectPtr<class Udemo_mapAttributeComponent> AttributeComponent;
	FShanmenVitalityCommitLedger CombatVitalityLedger;
	/** Delivery ids only; Code B remains the sole owner of item and receipt truth. */
	TSet<FGuid> ProcessedRestoreHealthReceiptIds;
#if WITH_DEV_AUTOMATION_TESTS
	int32 PositiveDamageBroadcastCount = 0;
#endif
};
