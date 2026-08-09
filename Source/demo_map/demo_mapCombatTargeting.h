#pragma once

#include "CoreMinimal.h"
#include "demo_mapCombatTypes.h"

class Fdemo_mapCombatTargeting
{
public:
	static bool TryGetFaction(const AActor* Actor, Edemo_mapFaction& OutFaction);
	static Edemo_mapTargetRelation ResolveRelation(const AActor* SourceActor, const AActor* TargetActor);
	static bool CanAffect(const AActor* SourceActor, const AActor* TargetActor, const Fdemo_mapTargetFilter& Filter);
};
