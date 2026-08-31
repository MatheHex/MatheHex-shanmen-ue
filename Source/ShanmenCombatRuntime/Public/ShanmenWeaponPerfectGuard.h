#pragma once

#include "CoreMinimal.h"
#include "ShanmenWeaponGuard.h"

#include "ShanmenWeaponPerfectGuard.generated.h"

UENUM(BlueprintType)
enum class EShanmenWeaponGuardTimingBand : uint8
{
	Invalid,
	Perfect,
	Ordinary
};

/**
 * Immutable perfect-guard interval bound to one exact ordinary guard window.
 * Ticks belong to a caller-owned monotonic timeline; CombatRuntime owns no clock.
 */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenWeaponPerfectGuardPolicy
{
	GENERATED_BODY()

public:
	static bool TryCapture(
		const FShanmenWeaponGuardWindowReceipt& Window,
		const FGuid& TimelineId,
		int64 ActiveStartTick,
		int64 PerfectEndTick,
		FName PerfectRuleId,
		FShanmenWeaponPerfectGuardPolicy& OutPolicy);

	bool IsValid() const;
	const FGuid& GetPolicyId() const { return PolicyId; }
	const FShanmenWeaponGuardWindowReceipt& GetWindow() const { return Window; }
	const FGuid& GetTimelineId() const { return TimelineId; }
	int64 GetActiveStartTick() const { return ActiveStartTick; }
	int64 GetPerfectEndTick() const { return PerfectEndTick; }
	FName GetPerfectRuleId() const { return PerfectRuleId; }

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|WeaponGuard", meta = (AllowPrivateAccess = "true"))
	FGuid PolicyId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|WeaponGuard", meta = (AllowPrivateAccess = "true"))
	FShanmenWeaponGuardWindowReceipt Window;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|WeaponGuard", meta = (AllowPrivateAccess = "true"))
	FGuid TimelineId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|WeaponGuard", meta = (AllowPrivateAccess = "true"))
	int64 ActiveStartTick = INDEX_NONE;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|WeaponGuard", meta = (AllowPrivateAccess = "true"))
	int64 PerfectEndTick = INDEX_NONE;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|WeaponGuard", meta = (AllowPrivateAccess = "true"))
	FName PerfectRuleId = NAME_None;
};

/** One explicit sample from the same caller-owned monotonic timeline. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenWeaponGuardTimelineObservation
{
	GENERATED_BODY()

public:
	static bool TryCapture(
		const FGuid& TimelineId,
		int64 ObservedTick,
		FShanmenWeaponGuardTimelineObservation& OutObservation);

	bool IsValid() const;
	const FGuid& GetObservationId() const { return ObservationId; }
	const FGuid& GetTimelineId() const { return TimelineId; }
	int64 GetObservedTick() const { return ObservedTick; }

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|WeaponGuard", meta = (AllowPrivateAccess = "true"))
	FGuid ObservationId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|WeaponGuard", meta = (AllowPrivateAccess = "true"))
	FGuid TimelineId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|WeaponGuard", meta = (AllowPrivateAccess = "true"))
	int64 ObservedTick = INDEX_NONE;
};

/** Immutable proof of one mutually exclusive perfect-or-ordinary projection. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenWeaponGuardTimingProjectionReceipt
{
	GENERATED_BODY()

public:
	bool IsValid() const;
	const FGuid& GetReceiptId() const { return ReceiptId; }
	const FShanmenWeaponPerfectGuardPolicy& GetPolicy() const { return Policy; }
	const FShanmenWeaponGuardTimelineObservation& GetObservation() const
	{
		return Observation;
	}
	EShanmenWeaponGuardTimingBand GetBand() const { return Band; }
	const FShanmenDefenseLayer& GetLayer() const { return Layer; }

private:
	friend class FShanmenWeaponGuardTimingEvaluator;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|WeaponGuard", meta = (AllowPrivateAccess = "true"))
	FGuid ReceiptId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|WeaponGuard", meta = (AllowPrivateAccess = "true"))
	FShanmenWeaponPerfectGuardPolicy Policy;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|WeaponGuard", meta = (AllowPrivateAccess = "true"))
	FShanmenWeaponGuardTimelineObservation Observation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|WeaponGuard", meta = (AllowPrivateAccess = "true"))
	FShanmenWeaponGuardProjectionReceipt OrdinaryProjection;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|WeaponGuard", meta = (AllowPrivateAccess = "true"))
	EShanmenWeaponGuardTimingBand Band = EShanmenWeaponGuardTimingBand::Invalid;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|WeaponGuard", meta = (AllowPrivateAccess = "true"))
	FShanmenDefenseLayer Layer;
};

/**
 * Stateless classifier over an existing active weapon-guard window.
 * It owns no Tick, timer, duration, input state, resource or action lifecycle.
 */
class SHANMENCOMBATRUNTIME_API FShanmenWeaponGuardTimingEvaluator
{
public:
	static bool TryProject(
		const FShanmenWeaponGuardWindow& Window,
		const FShanmenActionOrchestrator& ActionRuntime,
		const FShanmenWeaponPerfectGuardPolicy& Policy,
		const FShanmenWeaponGuardTimelineObservation& Observation,
		FShanmenWeaponGuardTimingProjectionReceipt& OutReceipt);
};
