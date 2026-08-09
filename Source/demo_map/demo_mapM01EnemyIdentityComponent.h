#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "demo_mapM01EnemyTypes.h"
#include "demo_mapM01EnemyIdentityComponent.generated.h"

class Ademo_mapV3ProgressionManager;

/** Stable M01 identity carried by combat actor and projected onto its Corpse. */
UCLASS(ClassGroup=(M01), meta=(BlueprintSpawnableComponent))
class Udemo_mapM01EnemyIdentityComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	Udemo_mapM01EnemyIdentityComponent();
	bool Configure(const Fdemo_mapM01EnemyDefinition& InDefinition);
	bool IsConfigured() const { return Definition.IsValid(); }
	const Fdemo_mapM01EnemyDefinition& GetDefinition() const { return Definition; }
	bool ProjectCorpse(
		Ademo_mapV3ProgressionManager* Manager,
		FGuid LootSourceId,
		const FVector& DeathLocation,
		const AActor* EnemyActor) const;

private:
	Fdemo_mapM01EnemyDefinition Definition;
};

