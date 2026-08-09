#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
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

	int32 GetMaxHealth() const { return MaxHealth; }
	int32 GetCurrentHealth() const { return CurrentHealth; }
	bool IsDefeated() const { return bIsDefeated; }
	static bool ShouldDodge(float DodgeChance, float NormalizedRoll);
	static int32 ResolveAppliedDamage(float RawDamage, float FlatDamageReduction);
	int32 ApplyIncomingDamage(float RawDamage);
	int32 ApplyHealing(int32 RequestedHealing);
	/** P15 Code A receipt adapter: applies at most once for this pawn/run health lifetime. */
	bool ApplyRestoreHealthReceipt(const FGuid& ReceiptId, int32 RestoreAmount, bool& bOutAlreadyProcessed);
	void RestoreCurrentHealthAfterItemUseRollback(int32 PreviousHealth);
	bool BindAttributeComponent(
		class Udemo_mapAttributeComponent* Attributes,
		bool bInitializeCurrentHealth);

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

	UPROPERTY(VisibleAnywhere, Category="Health")
	int32 MaxHealth = 5;

	UPROPERTY(VisibleAnywhere, Category="Health")
	int32 CurrentHealth = 5;

	UPROPERTY(VisibleAnywhere, Category="Health")
	bool bIsDefeated = false;

	FTimerHandle DamageFeedbackTimer;
	FDelegateHandle AttributeChangedHandle;
	TWeakObjectPtr<class Udemo_mapAttributeComponent> AttributeComponent;
	/** Delivery ids only; Code B remains the sole owner of item and receipt truth. */
	TSet<FGuid> ProcessedRestoreHealthReceiptIds;
#if WITH_DEV_AUTOMATION_TESTS
	int32 PositiveDamageBroadcastCount = 0;
#endif
};
