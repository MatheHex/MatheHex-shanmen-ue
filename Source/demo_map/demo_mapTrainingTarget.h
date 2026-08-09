#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "demo_mapTrainingTarget.generated.h"

class UStaticMeshComponent;
class UBoxComponent;
class Udemo_mapFactionComponent;
class UTextRenderComponent;
class UMaterialInstanceDynamic;

/** A minimal runtime-only damage target used to validate the T4 melee attack. */
UCLASS()
class Ademo_mapTrainingTarget : public AActor
{
	GENERATED_BODY()

public:
	Ademo_mapTrainingTarget();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	int32 GetHealth() const { return Health; }
	bool WasDestroyedByDamage() const { return bDestroyedByDamage; }

private:
	void ClearDamageFeedback();
	void FinishDeath();
	void RefreshPresentation();
	UPROPERTY(VisibleAnywhere, Category = "Training Target")
	TObjectPtr<UStaticMeshComponent> TargetMesh;

	UPROPERTY(VisibleAnywhere, Category = "Training Target")
	TObjectPtr<UBoxComponent> ProjectileContact;

	UPROPERTY(VisibleAnywhere, Category = "Combat")
	TObjectPtr<Udemo_mapFactionComponent> FactionComponent;

	UPROPERTY(VisibleAnywhere, Category = "Training Target")
	TObjectPtr<UTextRenderComponent> StatusText;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> TargetMaterial;

	UPROPERTY(VisibleAnywhere, Category = "Training Target")
	int32 Health = 2;

	bool bDestroyedByDamage = false;
	FTimerHandle DamageFeedbackTimer;
	FTimerHandle DeathTimer;
};
