#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "demo_mapCombatTypes.h"
#include "demo_mapFactionComponent.generated.h"

UCLASS(ClassGroup=(Combat), meta=(BlueprintSpawnableComponent))
class Udemo_mapFactionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	Udemo_mapFactionComponent();
	Edemo_mapFaction GetFaction() const { return Faction; }
	void SetFaction(Edemo_mapFaction InFaction) { Faction = InFaction; }

private:
	UPROPERTY(EditAnywhere, Category="Combat")
	Edemo_mapFaction Faction = Edemo_mapFaction::Neutral;
};
