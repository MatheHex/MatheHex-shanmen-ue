#pragma once

#include "CoreMinimal.h"
#include "ShanmenSpiritShield.h"

#include "ShanmenSpiritShieldDeadlineGate.generated.h"

UENUM(BlueprintType)
enum class EShanmenSpiritShieldDeadlineStatus : uint8
{
	Invalid,
	Elapsed,
	AlreadyElapsed,
	Rejected
};

UENUM(BlueprintType)
enum class EShanmenSpiritShieldDeadlineError : uint8
{
	None,
	InvalidObservation,
	GateNotReady,
	TimelineMismatch,
	ShieldMismatch,
	DeadlineNotReached,
	DeadlineConflict,
	ShieldUnavailable
};

/**
 * Immutable binding between one activated shield and one caller-owned
 * monotonic timeline. Ticks are opaque to CombatRuntime; only ordering matters.
 */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenSpiritShieldDeadlineContract
{
	GENERATED_BODY()

public:
	static bool TryCapture(
		const FShanmenSpiritShieldActivationReceipt& Activation,
		const FGuid& TimelineId,
		int64 StartTick,
		int64 DeadlineTick,
		FShanmenSpiritShieldDeadlineContract& OutContract);

	bool IsValid() const;
	const FGuid& GetContractId() const { return ContractId; }
	const FShanmenSpiritShieldActivationReceipt& GetActivation() const
	{
		return Activation;
	}
	const FGuid& GetTimelineId() const { return TimelineId; }
	int64 GetStartTick() const { return StartTick; }
	int64 GetDeadlineTick() const { return DeadlineTick; }

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield", meta = (AllowPrivateAccess = "true"))
	FGuid ContractId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield", meta = (AllowPrivateAccess = "true"))
	FShanmenSpiritShieldActivationReceipt Activation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield", meta = (AllowPrivateAccess = "true"))
	FGuid TimelineId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield", meta = (AllowPrivateAccess = "true"))
	int64 StartTick = INDEX_NONE;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield", meta = (AllowPrivateAccess = "true"))
	int64 DeadlineTick = INDEX_NONE;
};

/** One explicit sample from the same caller-owned monotonic timeline. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenSpiritShieldTimelineObservation
{
	GENERATED_BODY()

public:
	static bool TryCapture(
		const FGuid& TimelineId,
		int64 ObservedTick,
		FShanmenSpiritShieldTimelineObservation& OutObservation);

	bool IsValid() const;
	const FGuid& GetObservationId() const { return ObservationId; }
	const FGuid& GetTimelineId() const { return TimelineId; }
	int64 GetObservedTick() const { return ObservedTick; }

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield", meta = (AllowPrivateAccess = "true"))
	FGuid ObservationId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield", meta = (AllowPrivateAccess = "true"))
	FGuid TimelineId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield", meta = (AllowPrivateAccess = "true"))
	int64 ObservedTick = INDEX_NONE;
};

/** Immutable proof that an on-or-after-deadline observation ended the shield. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenSpiritShieldDeadlineElapsedReceipt
{
	GENERATED_BODY()

public:
	bool IsValid() const;
	const FGuid& GetReceiptId() const { return ReceiptId; }
	const FShanmenSpiritShieldDeadlineContract& GetContract() const
	{
		return Contract;
	}
	const FShanmenSpiritShieldTimelineObservation& GetObservation() const
	{
		return Observation;
	}
	const FShanmenSpiritShieldDeactivationReceipt& GetDeactivation() const
	{
		return Deactivation;
	}

private:
	friend class FShanmenSpiritShieldDeadlineGate;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield", meta = (AllowPrivateAccess = "true"))
	FGuid ReceiptId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield", meta = (AllowPrivateAccess = "true"))
	FShanmenSpiritShieldDeadlineContract Contract;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield", meta = (AllowPrivateAccess = "true"))
	FShanmenSpiritShieldTimelineObservation Observation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield", meta = (AllowPrivateAccess = "true"))
	FShanmenSpiritShieldDeactivationReceipt Deactivation;
};

/** Structured result; a rejection never carries a terminal receipt. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenSpiritShieldDeadlineResult
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield")
	EShanmenSpiritShieldDeadlineStatus Status =
		EShanmenSpiritShieldDeadlineStatus::Invalid;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield")
	EShanmenSpiritShieldDeadlineError Error =
		EShanmenSpiritShieldDeadlineError::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield")
	FShanmenSpiritShieldDeadlineElapsedReceipt Receipt;

	bool IsValid() const;
	bool IsSuccess() const;
};

/**
 * Pure gate from an explicit external timeline observation to the existing
 * shield lifecycle. It owns no clock, timer, polling loop, input, or energy.
 */
class SHANMENCOMBATRUNTIME_API FShanmenSpiritShieldDeadlineGate
{
public:
	static bool TryCreate(
		const FShanmenSpiritShieldDeadlineContract& Contract,
		FShanmenSpiritShieldDeadlineGate& OutGate);

	bool IsValid() const;
	FShanmenSpiritShieldDeadlineResult TryElapse(
		FShanmenSpiritShieldRuntime& ShieldRuntime,
		const FShanmenSpiritShieldTimelineObservation& Observation);
	void Reset();

	const FShanmenSpiritShieldDeadlineContract& GetContract() const
	{
		return Contract;
	}
	const FShanmenSpiritShieldDeadlineElapsedReceipt& GetElapsedReceipt() const
	{
		return ElapsedReceipt;
	}
	bool HasElapsed() const { return ElapsedReceipt.IsValid(); }

private:
	FShanmenSpiritShieldDeadlineResult Reject(
		EShanmenSpiritShieldDeadlineError Error) const;

	FShanmenSpiritShieldDeadlineContract Contract;
	FShanmenSpiritShieldDeadlineElapsedReceipt ElapsedReceipt;
	bool bInitialized = false;
};
