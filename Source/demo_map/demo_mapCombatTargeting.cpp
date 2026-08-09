#include "demo_mapCombatTargeting.h"
#include "demo_mapFactionComponent.h"
#include "GameFramework/Actor.h"

bool Fdemo_mapCombatTargeting::TryGetFaction(const AActor* Actor, Edemo_mapFaction& OutFaction)
{
	const Udemo_mapFactionComponent* Component = IsValid(Actor) ? Actor->FindComponentByClass<Udemo_mapFactionComponent>() : nullptr;
	if (Component == nullptr)
	{
		return false;
	}
	OutFaction = Component->GetFaction();
	return true;
}

Edemo_mapTargetRelation Fdemo_mapCombatTargeting::ResolveRelation(const AActor* SourceActor, const AActor* TargetActor)
{
	if (!IsValid(SourceActor) || !IsValid(TargetActor)) return Edemo_mapTargetRelation::Invalid;
	if (SourceActor == TargetActor) return Edemo_mapTargetRelation::Self;
	Edemo_mapFaction SourceFaction, TargetFaction;
	if (!TryGetFaction(SourceActor, SourceFaction) || !TryGetFaction(TargetActor, TargetFaction)) return Edemo_mapTargetRelation::Invalid;
	if (SourceFaction == Edemo_mapFaction::Neutral || TargetFaction == Edemo_mapFaction::Neutral) return Edemo_mapTargetRelation::Neutral;
	if (SourceFaction == TargetFaction) return Edemo_mapTargetRelation::Friendly;
	const bool bSourceFriendly = SourceFaction == Edemo_mapFaction::Player || SourceFaction == Edemo_mapFaction::Friendly;
	const bool bTargetFriendly = TargetFaction == Edemo_mapFaction::Player || TargetFaction == Edemo_mapFaction::Friendly;
	if (bSourceFriendly && bTargetFriendly) return Edemo_mapTargetRelation::Friendly;
	if ((bSourceFriendly && TargetFaction == Edemo_mapFaction::Hostile) || (SourceFaction == Edemo_mapFaction::Hostile && bTargetFriendly)) return Edemo_mapTargetRelation::Hostile;
	return Edemo_mapTargetRelation::Invalid;
}

bool Fdemo_mapCombatTargeting::CanAffect(const AActor* SourceActor, const AActor* TargetActor, const Fdemo_mapTargetFilter& Filter)
{
	switch (ResolveRelation(SourceActor, TargetActor))
	{
	case Edemo_mapTargetRelation::Self: return Filter.bAffectSelf;
	case Edemo_mapTargetRelation::Friendly: return Filter.bAffectFriendly;
	case Edemo_mapTargetRelation::Hostile: return Filter.bAffectHostile;
	case Edemo_mapTargetRelation::Neutral: return Filter.bAffectNeutral;
	default: return false;
	}
}
