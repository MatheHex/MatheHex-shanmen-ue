#pragma once

#include "CoreMinimal.h"
#include "ShanmenActionResourceAuthority.h"
#include "ShanmenSpiritShield.h"

#include "ShanmenSpiritShieldActionCoordinator.generated.h"

UENUM(BlueprintType)
enum class EShanmenSpiritShieldActionState : uint8
{
	Uninitialized,
	Reserved,
	Activated,
	Completed,
	Aborted,
	Interrupted
};

UENUM(BlueprintType)
enum class EShanmenSpiritShieldActionStatus : uint8
{
	Invalid,
	Begun,
	AlreadyBegun,
	Activated,
	Completed,
	Interrupted,
	Aborted,
	AlreadyFinalized,
	AlreadyClosed,
	Rejected
};

UENUM(BlueprintType)
enum class EShanmenSpiritShieldActionError : uint8
{
	None,
	InvalidInput,
	CoordinatorNotReady,
	CoordinatorConflict,
	ResourceReservationRejected,
	ActionTransitionRejected,
	ResourceFinalizationRejected,
	ShieldActivationRejected,
	ShieldDeactivationRejected,
	ActionClosureRejected,
	FinalizationConflict,
	ClosureConflict,
	StateDesynchronized
};

UENUM(BlueprintType)
enum class EShanmenSpiritShieldActionOutcome : uint8
{
	None,
	Active,
	Cancelled,
	Interrupted
};

UENUM(BlueprintType)
enum class EShanmenSpiritShieldActionClosureOutcome : uint8
{
	None,
	Completed,
	Interrupted
};

/** Immutable proof that Startup and resource reservation succeeded together. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenSpiritShieldActionStartupReceipt
{
	GENERATED_BODY()

public:
	bool IsValid() const;
	const FGuid& GetReceiptId() const { return ReceiptId; }
	const FGuid& GetSessionId() const { return SessionId; }
	const FShanmenCombatActionSnapshot& GetAction() const { return Action; }
	const FShanmenSpiritShieldDefinition& GetDefinition() const
	{
		return Definition;
	}
	const FShanmenActionTransitionReceipt& GetStartupTransition() const
	{
		return StartupTransition;
	}
	const FShanmenActionResourceReservationReceipt& GetReservation() const
	{
		return Reservation;
	}

private:
	friend class FShanmenSpiritShieldActionCoordinator;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield", meta = (AllowPrivateAccess = "true"))
	FGuid ReceiptId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield", meta = (AllowPrivateAccess = "true"))
	FGuid SessionId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield", meta = (AllowPrivateAccess = "true"))
	FShanmenCombatActionSnapshot Action;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield", meta = (AllowPrivateAccess = "true"))
	FShanmenSpiritShieldDefinition Definition;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield", meta = (AllowPrivateAccess = "true"))
	FShanmenActionTransitionReceipt StartupTransition;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield", meta = (AllowPrivateAccess = "true"))
	FShanmenActionResourceReservationReceipt Reservation;
};

/** Immutable proof of either atomic activation or pre-commit release. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenSpiritShieldActionTerminalReceipt
{
	GENERATED_BODY()

public:
	bool IsValid() const;
	const FGuid& GetReceiptId() const { return ReceiptId; }
	const FShanmenSpiritShieldActionStartupReceipt& GetStartup() const
	{
		return Startup;
	}
	const FShanmenActionTransitionReceipt& GetActionTransition() const
	{
		return ActionTransition;
	}
	const FShanmenActionResourceFinalizationReceipt& GetResourceFinalization() const
	{
		return ResourceFinalization;
	}
	const FShanmenSpiritShieldActivationReceipt& GetShieldActivation() const
	{
		return ShieldActivation;
	}
	EShanmenSpiritShieldActionOutcome GetOutcome() const { return Outcome; }

private:
	friend class FShanmenSpiritShieldActionCoordinator;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield", meta = (AllowPrivateAccess = "true"))
	FGuid ReceiptId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield", meta = (AllowPrivateAccess = "true"))
	FShanmenSpiritShieldActionStartupReceipt Startup;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield", meta = (AllowPrivateAccess = "true"))
	FShanmenActionTransitionReceipt ActionTransition;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield", meta = (AllowPrivateAccess = "true"))
	FShanmenActionResourceFinalizationReceipt ResourceFinalization;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield", meta = (AllowPrivateAccess = "true"))
	FShanmenSpiritShieldActivationReceipt ShieldActivation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield", meta = (AllowPrivateAccess = "true"))
	EShanmenSpiritShieldActionOutcome Outcome =
		EShanmenSpiritShieldActionOutcome::None;
};

/** Immutable proof that an activated shield and its action closed together. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenSpiritShieldActionClosureReceipt
{
	GENERATED_BODY()

public:
	bool IsValid() const;
	const FGuid& GetReceiptId() const { return ReceiptId; }
	const FShanmenSpiritShieldActionTerminalReceipt& GetActivationTerminal() const
	{
		return ActivationTerminal;
	}
	const FShanmenSpiritShieldDeactivationReceipt& GetDeactivation() const
	{
		return Deactivation;
	}
	const FShanmenActionTransitionReceipt& GetExitActiveTransition() const
	{
		return ExitActiveTransition;
	}
	const FShanmenActionTransitionReceipt& GetCompletionTransition() const
	{
		return CompletionTransition;
	}
	EShanmenSpiritShieldActionClosureOutcome GetOutcome() const
	{
		return Outcome;
	}

private:
	friend class FShanmenSpiritShieldActionCoordinator;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield", meta = (AllowPrivateAccess = "true"))
	FGuid ReceiptId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield", meta = (AllowPrivateAccess = "true"))
	FShanmenSpiritShieldActionTerminalReceipt ActivationTerminal;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield", meta = (AllowPrivateAccess = "true"))
	FShanmenSpiritShieldDeactivationReceipt Deactivation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield", meta = (AllowPrivateAccess = "true"))
	FShanmenActionTransitionReceipt ExitActiveTransition;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield", meta = (AllowPrivateAccess = "true"))
	FShanmenActionTransitionReceipt CompletionTransition;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield", meta = (AllowPrivateAccess = "true"))
	EShanmenSpiritShieldActionClosureOutcome Outcome =
		EShanmenSpiritShieldActionClosureOutcome::None;
};

/** Structured operation result; rejected operations carry no proof. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenSpiritShieldActionResult
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield")
	EShanmenSpiritShieldActionStatus Status =
		EShanmenSpiritShieldActionStatus::Invalid;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield")
	EShanmenSpiritShieldActionError Error =
		EShanmenSpiritShieldActionError::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield")
	EShanmenActionResourceTransactionError ResourceError =
		EShanmenActionResourceTransactionError::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield")
	FShanmenSpiritShieldActionStartupReceipt Startup;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield")
	FShanmenSpiritShieldActionTerminalReceipt Terminal;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield")
	FShanmenSpiritShieldActionClosureReceipt Closure;

	bool IsValid() const;
	bool IsSuccess() const;
};

/**
 * Pure composition authority for one resource-backed spirit-shield action.
 *
 * Begin atomically starts the existing action runtime, prepares the existing
 * shield runtime and reserves one typed spirit-energy cost. Commit atomically
 * crosses the existing action commit point, consumes that reservation and
 * activates the shield. A pre-commit cancel or interrupt only releases it.
 */
class SHANMENCOMBATRUNTIME_API FShanmenSpiritShieldActionCoordinator
{
public:
	static FShanmenSpiritShieldActionResult Begin(
		const FShanmenCombatActionSnapshot& Action,
		const FShanmenSpiritShieldDefinition& Definition,
		const FShanmenActionResourceCost& Cost,
		FShanmenActionResourceAuthority& ResourceAuthority,
		FShanmenSpiritShieldActionCoordinator& OutCoordinator);

	bool IsValid() const;
	FShanmenSpiritShieldActionResult Commit(
		FShanmenActionResourceAuthority& ResourceAuthority);
	FShanmenSpiritShieldActionResult Abort(
		EShanmenActionTerminalReason Reason,
		FShanmenActionResourceAuthority& ResourceAuthority);
	/**
	 * Atomically closes an activated shield and its action. DurationElapsed is
	 * accepted only after the deadline gate has already deactivated the shield.
	 */
	FShanmenSpiritShieldActionResult Close(
		EShanmenSpiritShieldDeactivationReason Reason);
	void Reset();

	EShanmenSpiritShieldActionState GetState() const { return State; }
	const FShanmenSpiritShieldActionStartupReceipt& GetStartupReceipt() const
	{
		return StartupReceipt;
	}
	const FShanmenSpiritShieldActionTerminalReceipt& GetTerminalReceipt() const
	{
		return TerminalReceipt;
	}
	const FShanmenSpiritShieldActionClosureReceipt& GetClosureReceipt() const
	{
		return ClosureReceipt;
	}
	const FShanmenActionOrchestrator& GetActionRuntime() const
	{
		return ActionRuntime;
	}
	const FShanmenSpiritShieldRuntime& GetShieldRuntime() const
	{
		return ShieldRuntime;
	}
	FShanmenSpiritShieldRuntime& GetShieldRuntime()
	{
		return ShieldRuntime;
	}

private:
	static FShanmenSpiritShieldActionResult Reject(
		EShanmenSpiritShieldActionError Error,
		EShanmenActionResourceTransactionError ResourceError =
			EShanmenActionResourceTransactionError::None);
	FShanmenSpiritShieldActionResult ReplayTerminal(
		EShanmenSpiritShieldActionOutcome ExpectedOutcome,
		FShanmenActionResourceAuthority& ResourceAuthority) const;

	FShanmenActionOrchestrator ActionRuntime;
	FShanmenSpiritShieldRuntime ShieldRuntime;
	FShanmenSpiritShieldActionStartupReceipt StartupReceipt;
	FShanmenSpiritShieldActionTerminalReceipt TerminalReceipt;
	FShanmenSpiritShieldActionClosureReceipt ClosureReceipt;
	EShanmenSpiritShieldActionState State =
		EShanmenSpiritShieldActionState::Uninitialized;
	bool bInitialized = false;
};
