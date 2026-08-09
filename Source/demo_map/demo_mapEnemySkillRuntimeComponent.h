#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "demo_mapEnemySkillTypes.h"
#include "demo_mapEnemySkillRuntimeComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(
	Fdemo_mapEnemySkillResolveDelegate,
	const Fdemo_mapEnemySkillRuntimeSnapshot&);
DECLARE_MULTICAST_DELEGATE_OneParam(
	Fdemo_mapEnemySkillSegmentDelegate,
	const Fdemo_mapEnemySkillDisplacementSegment&);

/**
 * Per-actor authority for the only active enemy skill phase, cooldown, direction,
 * and swept displacement.
 */
UCLASS(ClassGroup=(Combat))
class Udemo_mapEnemySkillRuntimeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	Udemo_mapEnemySkillRuntimeComponent();
	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	Fdemo_mapEnemySkillStartResult TryActivate(
		const Fdemo_mapEnemySkillActivationIntent& Intent);
	void MarkFirstLegalHitAndRecover();
	void Cancel(bool bResetCooldown);
	void ResetForNewRun();

	bool IsActive() const
	{
		return Phase != Edemo_mapEnemySkillPhase::Idle;
	}
	bool IsReady() const;
	Fdemo_mapEnemySkillRuntimeSnapshot GetSnapshot() const;
	int32 GetActivationCount() const { return ActivationCount; }
	int32 GetResolveCount() const { return ResolveCount; }
	int32 GetLegalHitCount() const { return LegalHitCount; }
	int32 GetBlockedDisplacementCount() const { return BlockedDisplacementCount; }

	Fdemo_mapEnemySkillResolveDelegate OnResolve;
	Fdemo_mapEnemySkillSegmentDelegate OnDisplacementSegment;

private:
	void EnterDisplacement();
	void ResolveAndRecover();
	void EnterRecovery();
	FVector ResolveDirectionAtEndOfWindup() const;
	double GetNow() const;

	Edemo_mapEnemySkillPhase Phase = Edemo_mapEnemySkillPhase::Idle;
	Fdemo_mapEnemySkillDefinition Definition;
	TWeakObjectPtr<AActor> Target;
	FVector LockedDirection = FVector::ZeroVector;
	double PhaseStartTime = 0.0;
	double CooldownUntil = 0.0;
	float ResolvedPreflightDistance = 0.0f;
	float ResolvedDisplacementDistance = 0.0f;
	uint32 ActivationSerial = 0;
	int32 ActivationCount = 0;
	int32 ResolveCount = 0;
	int32 LegalHitCount = 0;
	int32 BlockedDisplacementCount = 0;
	bool bFirstLegalHitConsumed = false;
	bool bResolveBroadcast = false;
};

