#pragma once

#include "CoreMinimal.h"
#include "ShanmenSpiritShieldActionCoordinator.h"
#include "ShanmenSpiritShieldCapacityAuthority.h"
#include "ShanmenSpiritShieldDeadlineGate.h"

#include "ShanmenSpiritShieldSession.generated.h"

/** Immutable caller-owned timeline plan captured before shield activation. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenSpiritShieldSchedule
{
	GENERATED_BODY()

public:
	static bool TryCapture(
		const FGuid& TimelineId,
		int64 StartTick,
		int64 DeadlineTick,
		FShanmenSpiritShieldSchedule& OutSchedule);

	bool IsValid() const;
	const FGuid& GetScheduleId() const { return ScheduleId; }
	const FGuid& GetTimelineId() const { return TimelineId; }
	int64 GetStartTick() const { return StartTick; }
	int64 GetDeadlineTick() const { return DeadlineTick; }

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield", meta = (AllowPrivateAccess = "true"))
	FGuid ScheduleId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield", meta = (AllowPrivateAccess = "true"))
	FGuid TimelineId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield", meta = (AllowPrivateAccess = "true"))
	int64 StartTick = INDEX_NONE;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield", meta = (AllowPrivateAccess = "true"))
	int64 DeadlineTick = INDEX_NONE;
};

UENUM(BlueprintType)
enum class EShanmenSpiritShieldDeadlineClosureStatus : uint8
{
	Invalid,
	Closed,
	AlreadyClosed,
	Rejected
};

UENUM(BlueprintType)
enum class EShanmenSpiritShieldDeadlineClosureError : uint8
{
	None,
	InvalidObservation,
	SessionNotReady,
	DeadlineRejected,
	ActionClosureRejected,
	StateDesynchronized
};

/** Atomic proof that one deadline observation closed both shield and action. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenSpiritShieldDeadlineClosureResult
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield")
	EShanmenSpiritShieldDeadlineClosureStatus Status =
		EShanmenSpiritShieldDeadlineClosureStatus::Invalid;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield")
	EShanmenSpiritShieldDeadlineClosureError Error =
		EShanmenSpiritShieldDeadlineClosureError::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield")
	EShanmenSpiritShieldDeadlineError DeadlineError =
		EShanmenSpiritShieldDeadlineError::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield")
	EShanmenSpiritShieldActionError ActionError =
		EShanmenSpiritShieldActionError::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield")
	FShanmenSpiritShieldDeadlineElapsedReceipt DeadlineReceipt;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield")
	FShanmenSpiritShieldActionClosureReceipt ClosureReceipt;

	bool IsValid() const;
	bool IsSuccess() const;
};

/**
 * Aggregate root for one complete resource-backed short spirit-shield session.
 *
 * It composes the existing action/resource coordinator, shield-capacity
 * authority and deadline gate without replacing any of them. Every operation
 * is evaluated against a candidate copy before state escapes, so product code
 * cannot publish a half-configured activation or a deadline-only closure.
 * The product layer still owns the real SpiritEnergy balance and monotonic
 * timeline; this session accepts only their typed authority and observations.
 */
class SHANMENCOMBATRUNTIME_API FShanmenSpiritShieldSession
{
public:
	static FShanmenSpiritShieldActionResult Begin(
		const FShanmenCombatActionSnapshot& Action,
		const FShanmenSpiritShieldDefinition& Definition,
		const FShanmenActionResourceCost& Cost,
		const FShanmenSpiritShieldSchedule& Schedule,
		FShanmenActionResourceAuthority& ResourceAuthority,
		FShanmenSpiritShieldSession& OutSession);

	bool IsValid() const;
	FShanmenSpiritShieldActionResult Commit(
		FShanmenActionResourceAuthority& ResourceAuthority);
	FShanmenSpiritShieldActionResult Abort(
		EShanmenActionTerminalReason Reason,
		FShanmenActionResourceAuthority& ResourceAuthority);
	bool TryProjectDefenseLayer(
		FShanmenSpiritShieldProjectionReceipt& OutProjection);
	FShanmenSpiritShieldCapacityCommitResult CommitCapacity(
		const FShanmenSpiritShieldCapacityCommitCommand& Command);
	FShanmenSpiritShieldDeadlineClosureResult ObserveDeadline(
		const FShanmenSpiritShieldTimelineObservation& Observation);
	FShanmenSpiritShieldActionResult Close(
		EShanmenSpiritShieldDeactivationReason Reason);
	void Reset();

	const FShanmenSpiritShieldSchedule& GetSchedule() const
	{
		return Schedule;
	}
	const FShanmenSpiritShieldActionCoordinator& GetActionCoordinator() const
	{
		return ActionCoordinator;
	}
	const FShanmenSpiritShieldCapacityAuthority& GetCapacityAuthority() const
	{
		return CapacityAuthority;
	}
	const FShanmenSpiritShieldDeadlineContract& GetDeadlineContract() const
	{
		return DeadlineContract;
	}
	const FShanmenSpiritShieldDeadlineGate& GetDeadlineGate() const
	{
		return DeadlineGate;
	}
	bool HasActivatedAuthorities() const;

private:
	FShanmenSpiritShieldSchedule Schedule;
	FShanmenSpiritShieldActionCoordinator ActionCoordinator;
	FShanmenSpiritShieldCapacityAuthority CapacityAuthority;
	FShanmenSpiritShieldDeadlineContract DeadlineContract;
	FShanmenSpiritShieldDeadlineGate DeadlineGate;
	bool bInitialized = false;
};
