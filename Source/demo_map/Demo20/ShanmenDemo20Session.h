#pragma once

#include "CoreMinimal.h"
#include "ShanmenVitalityAuthority.h"

enum class EShanmenDemo20Phase : uint8 { Preparation, Active, Extracted, Defeated, Abandoned };

/** Combat checkpoint only; item quantities remain exclusively in ShanmenItems. */
struct FShanmenDemo20CombatCheckpoint
{
	FGuid RunId;
	EShanmenDemo20Phase Phase = EShanmenDemo20Phase::Active;
	uint64 Sequence = 0;
	float Health[4] = {100.f, 78.f, 65.f, 156.f};
	int64 Revisions[4] = {0, 0, 0, 0};
	float Elapsed = 0.f, AttackCooldown = 0.f, EvadeCooldown = 0.f, EvadeWindow = 0.f;
	float SwordDamage = 26.f, ArmorFraction = .12f;
	bool IsValid() const;
};

/** Combat projection shared by practice and checkpointed expeditions; never an Item/Profile authority. */
class FShanmenDemo20Session
{
public:
	static constexpr int32 SentinelCount = 3;
	bool Begin(const FGuid& NewRunId);
	bool BeginExpedition(const FGuid& NewRunId, float SwordDamage, float ArmorFraction);
	bool RestoreExpedition(const FShanmenDemo20CombatCheckpoint& Checkpoint);
	bool CaptureExpedition(FShanmenDemo20CombatCheckpoint& Out) const;
	void Advance(float DeltaSeconds);
	bool StrikeSentinel(int32 Index);
	bool ReceiveSentinelStrike(int32 Index);
	/** Mutates a candidate only. World/item coordination must confirm it before publication. */
	bool TryUseMedicine(FString& Reason);
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
	bool bExpedition = false;
	float ExpeditionSwordDamage = 26.f, ExpeditionArmorFraction = .12f;
};
