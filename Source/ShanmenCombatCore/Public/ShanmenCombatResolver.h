#pragma once

#include "CoreMinimal.h"
#include "ShanmenCombatTypes.h"

/** Canonical identities shared by every future hit detector. */
struct SHANMENCOMBATCORE_API FShanmenCombatIdFactory
{
	static FGuid MakeActivationId(
		const FGuid& RunId,
		const FGuid& SourceEntityId,
		FName ActionDefinitionId,
		uint64 ActivationSequence);

	static FGuid MakeImpactId(
		const FGuid& RunId,
		const FGuid& ActivationId,
		FName DetectorId,
		const FGuid& TargetEntityId,
		int32 HitOrdinal);
};

/** Per-run idempotency gate. Geometry callbacks may repeat; an ImpactId may not. */
class SHANMENCOMBATCORE_API FShanmenImpactLedger
{
public:
	bool TryAccept(const FGuid& ImpactId);
	bool Contains(const FGuid& ImpactId) const;
	int32 Num() const;
	void Reset();

private:
	TSet<FGuid> AcceptedImpactIds;
};

/** Pure, deterministic defense ordering with no Actor, World, inventory, or RNG access. */
struct SHANMENCOMBATCORE_API FShanmenDefenseResolver
{
	static FShanmenImpactResult Resolve(const FShanmenImpactRequest& Request);
};
