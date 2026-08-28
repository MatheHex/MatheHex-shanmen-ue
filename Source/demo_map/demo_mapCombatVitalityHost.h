#pragma once

#include "CoreMinimal.h"
#include "ShanmenVitalityAuthority.h"
#include "UObject/Interface.h"
#include "demo_mapCombatVitalityHost.generated.h"

/**
 * Product-facing vitality contract for an authored M01 combatant.
 *
 * Implementations retain ownership of their vitality, presentation, death,
 * loot, and mission side effects. The combat coordinator sees only this
 * contract and therefore never branches on a concrete enemy actor class.
 */
UINTERFACE(MinimalAPI)
class Udemo_mapCombatVitalityHost : public UInterface
{
	GENERATED_BODY()
};

class Idemo_mapCombatVitalityHost
{
	GENERATED_BODY()

public:
	virtual bool TryBindCombatEntity(const FGuid& TargetEntityId) = 0;
	virtual bool TryEndCombatEntityBinding(
		const FGuid& ExpectedTargetEntityId) = 0;
	virtual bool IsCombatEntityBound() const = 0;
	virtual const FGuid& GetCombatEntityId() const = 0;
	virtual int64 GetCombatAuthorityRevision() const = 0;
	virtual int32 NumCommittedCombatImpacts() const = 0;
	virtual bool TryCaptureCombatVitalitySnapshot(
		FShanmenTargetVitalitySnapshot& OutSnapshot) const = 0;
	virtual FShanmenVitalityCommitResult CommitCombatImpact(
		const FShanmenVitalityCommitCommand& Command) = 0;

#if WITH_DEV_AUTOMATION_TESTS
	virtual int32 GetPositiveCombatDamageCountForAutomation() const = 0;
#endif
};
