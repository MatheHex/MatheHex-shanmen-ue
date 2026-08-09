#include "demo_mapM01EnemyIdentityComponent.h"

#include "demo_mapV3ProgressionManager.h"

Udemo_mapM01EnemyIdentityComponent::Udemo_mapM01EnemyIdentityComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool Udemo_mapM01EnemyIdentityComponent::Configure(
	const Fdemo_mapM01EnemyDefinition& InDefinition)
{
	if (!InDefinition.IsValid()) return false;
	Definition = InDefinition;
	return true;
}

bool Udemo_mapM01EnemyIdentityComponent::ProjectCorpse(
	Ademo_mapV3ProgressionManager* Manager,
	FGuid LootSourceId,
	const FVector& DeathLocation,
	const AActor* EnemyActor) const
{
	return Manager && Definition.IsValid()
		&& Manager->HandleM01EnemyDeath(
			Definition.RewardSourceRoleId,
			Definition.CorpseIdentity,
			LootSourceId,
			DeathLocation,
			EnemyActor).bSuccess;
}
