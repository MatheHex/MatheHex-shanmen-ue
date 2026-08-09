#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "demo_mapFriendlyUnit.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;
class UPointLightComponent;
class Udemo_mapFactionComponent;

UCLASS()
class Ademo_mapFriendlyUnit : public AActor
{
	GENERATED_BODY()

public:
	Ademo_mapFriendlyUnit();
	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
	int32 GetCurrentHealth() const { return CurrentHealth; }
	int32 GetMaxHealth() const { return MaxHealth; }

private:
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Mesh;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> Label;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UPointLightComponent> GreenLight;
	UPROPERTY(VisibleAnywhere) TObjectPtr<Udemo_mapFactionComponent> FactionComponent;
	UPROPERTY(VisibleAnywhere) int32 MaxHealth = 5;
	UPROPERTY(VisibleAnywhere) int32 CurrentHealth = 5;
};
