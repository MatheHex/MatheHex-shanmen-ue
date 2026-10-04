#pragma once

#include "CoreMinimal.h"
#include "ShanmenVitalityAuthority.h"

enum class EShanmenDemo20Phase : uint8 { Preparation, Active, Extracted, Defeated, Abandoned };

/** Disposable arena session, not a Profile/Item authority or a persistent expedition. */
class FShanmenDemo20Session
{
public:
	static constexpr int32 SentinelCount = 3;
	bool Begin(const FGuid& NewRunId);
	void Advance(float DeltaSeconds);
	bool StrikeSentinel(int32 Index);
	bool ReceiveSentinelStrike(int32 Index);
	bool TryEvade();
	void SetGuarding(bool bHeld);
	bool TryExtract();
	void Abandon();
	bool ReturnToPreparation();
	EShanmenDemo20Phase GetPhase() const { return Phase; }
	const FGuid& GetRunId() const { return RunId; }
	float GetHealth(int32 EntityIndex = 0) const;
	int32 NumDefeated() const;
	bool IsGuarding() const { return bGuarding; }
	bool IsEvading() const { return EvadeWindow > 0.f; }
	float GetEvadeCooldown() const { return EvadeCooldown; }
	float GetAttackCooldown() const { return AttackCooldown; }
	float GetElapsed() const { return Elapsed; }
	int32 GetImpactCount() const;

private:
	bool ResolveContact(int32 SourceIndex, int32 TargetIndex, float Damage);
	FGuid RunId;
	EShanmenDemo20Phase Phase = EShanmenDemo20Phase::Preparation;
	TArray<FShanmenVitalityAuthority> Vitalities;
	uint64 Sequence = 0;
	float Elapsed = 0.f;
	float AttackCooldown = 0.f;
	float EvadeWindow = 0.f;
	float EvadeCooldown = 0.f;
	bool bGuarding = false;
};
