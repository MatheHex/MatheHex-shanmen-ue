#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ShanmenActionOrchestrator.h"

#include "ShanmenActionResourceAuthority.generated.h"

UENUM(BlueprintType)
enum class EShanmenActionResourceDisposition : uint8
{
	None,
	Commit,
	Release
};

UENUM(BlueprintType)
enum class EShanmenActionResourceTransactionStatus : uint8
{
	Invalid,
	Reserved,
	AlreadyReserved,
	Committed,
	Released,
	AlreadyFinalized,
	Rejected
};

UENUM(BlueprintType)
enum class EShanmenActionResourceTransactionError : uint8
{
	None,
	InvalidRequest,
	AuthorityNotReady,
	OwnerMismatch,
	ChannelMismatch,
	StaleSnapshot,
	InsufficientAvailable,
	ReservationConflict,
	ReservationNotFound,
	FinalizationConflict,
	TransitionMismatch,
	RevisionExhausted,
	StateDesynchronized
};

/** Mutable authoring input; final resource numbers remain content-owned. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenActionResourceCostCapture
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|Resource")
	FName RuleId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|Resource")
	FGameplayTag ResourceChannel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|Resource", meta = (ClampMin = "0.0"))
	float Amount = 0.0f;
};

/** Immutable typed action cost. A free action carries no cost object. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenActionResourceCost
{
	GENERATED_BODY()

public:
	static bool TryCapture(
		const FShanmenActionResourceCostCapture& Capture,
		FShanmenActionResourceCost& OutCost);

	bool IsValid() const;
	const FGuid& GetCostId() const { return CostId; }
	FName GetRuleId() const { return RuleId; }
	FGameplayTag GetResourceChannel() const { return ResourceChannel; }
	float GetAmount() const { return Amount; }

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Resource", meta = (AllowPrivateAccess = "true"))
	FGuid CostId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Resource", meta = (AllowPrivateAccess = "true"))
	FName RuleId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Resource", meta = (AllowPrivateAccess = "true"))
	FGameplayTag ResourceChannel;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Resource", meta = (AllowPrivateAccess = "true"))
	float Amount = 0.0f;
};

/** Immutable resource state sampled before a reservation command is built. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenActionResourceSnapshot
{
	GENERATED_BODY()

public:
	bool IsValid() const;
	const FGuid& GetSnapshotId() const { return SnapshotId; }
	const FGuid& GetOwnerEntityId() const { return OwnerEntityId; }
	FGameplayTag GetResourceChannel() const { return ResourceChannel; }
	int64 GetAuthorityRevision() const { return AuthorityRevision; }
	float GetCurrentAmount() const { return CurrentAmount; }
	float GetMaximumAmount() const { return MaximumAmount; }
	float GetReservedAmount() const { return ReservedAmount; }
	float GetAvailableAmount() const { return AvailableAmount; }

private:
	friend class FShanmenActionResourceAuthority;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Resource", meta = (AllowPrivateAccess = "true"))
	FGuid SnapshotId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Resource", meta = (AllowPrivateAccess = "true"))
	FGuid OwnerEntityId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Resource", meta = (AllowPrivateAccess = "true"))
	FGameplayTag ResourceChannel;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Resource", meta = (AllowPrivateAccess = "true"))
	int64 AuthorityRevision = INDEX_NONE;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Resource", meta = (AllowPrivateAccess = "true"))
	float CurrentAmount = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Resource", meta = (AllowPrivateAccess = "true"))
	float MaximumAmount = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Resource", meta = (AllowPrivateAccess = "true"))
	float ReservedAmount = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Resource", meta = (AllowPrivateAccess = "true"))
	float AvailableAmount = 0.0f;
};

/** Immutable request to reserve one channel before an action commit point. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenActionResourceReservationRequest
{
	GENERATED_BODY()

public:
	static bool TryCreate(
		const FShanmenCombatActionSnapshot& Action,
		const FShanmenActionTransitionReceipt& StartupReceipt,
		const FShanmenActionResourceCost& Cost,
		const FShanmenActionResourceSnapshot& ResourceSnapshot,
		FShanmenActionResourceReservationRequest& OutRequest);

	bool IsValid() const;
	const FGuid& GetReservationId() const { return ReservationId; }
	const FGuid& GetCommandId() const { return CommandId; }
	const FShanmenCombatActionSnapshot& GetAction() const { return Action; }
	const FShanmenActionTransitionReceipt& GetStartupReceipt() const { return StartupReceipt; }
	const FShanmenActionResourceCost& GetCost() const { return Cost; }
	const FShanmenActionResourceSnapshot& GetResourceSnapshot() const { return ResourceSnapshot; }

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Resource", meta = (AllowPrivateAccess = "true"))
	FGuid ReservationId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Resource", meta = (AllowPrivateAccess = "true"))
	FGuid CommandId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Resource", meta = (AllowPrivateAccess = "true"))
	FShanmenCombatActionSnapshot Action;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Resource", meta = (AllowPrivateAccess = "true"))
	FShanmenActionTransitionReceipt StartupReceipt;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Resource", meta = (AllowPrivateAccess = "true"))
	FShanmenActionResourceCost Cost;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Resource", meta = (AllowPrivateAccess = "true"))
	FShanmenActionResourceSnapshot ResourceSnapshot;
};

/** Immutable proof that capacity was hidden from competing actions. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenActionResourceReservationReceipt
{
	GENERATED_BODY()

public:
	bool IsValid() const;
	const FGuid& GetReceiptId() const { return ReceiptId; }
	const FGuid& GetReservationId() const { return Request.GetReservationId(); }
	const FShanmenActionResourceReservationRequest& GetRequest() const { return Request; }
	int64 GetAuthorityRevisionBefore() const { return AuthorityRevisionBefore; }
	int64 GetAuthorityRevisionAfter() const { return AuthorityRevisionAfter; }
	float GetCurrentAmount() const { return CurrentAmount; }
	float GetMaximumAmount() const { return MaximumAmount; }
	float GetReservedBefore() const { return ReservedBefore; }
	float GetReservedAfter() const { return ReservedAfter; }
	float GetAvailableBefore() const { return AvailableBefore; }
	float GetAvailableAfter() const { return AvailableAfter; }

private:
	friend class FShanmenActionResourceAuthority;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Resource", meta = (AllowPrivateAccess = "true"))
	FGuid ReceiptId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Resource", meta = (AllowPrivateAccess = "true"))
	FShanmenActionResourceReservationRequest Request;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Resource", meta = (AllowPrivateAccess = "true"))
	int64 AuthorityRevisionBefore = INDEX_NONE;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Resource", meta = (AllowPrivateAccess = "true"))
	int64 AuthorityRevisionAfter = INDEX_NONE;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Resource", meta = (AllowPrivateAccess = "true"))
	float CurrentAmount = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Resource", meta = (AllowPrivateAccess = "true"))
	float MaximumAmount = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Resource", meta = (AllowPrivateAccess = "true"))
	float ReservedBefore = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Resource", meta = (AllowPrivateAccess = "true"))
	float ReservedAfter = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Resource", meta = (AllowPrivateAccess = "true"))
	float AvailableBefore = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Resource", meta = (AllowPrivateAccess = "true"))
	float AvailableAfter = 0.0f;
};

/** Commit on Startup->Active, or release on a pre-commit terminal transition. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenActionResourceFinalizationRequest
{
	GENERATED_BODY()

public:
	static bool TryCreate(
		const FShanmenActionResourceReservationReceipt& Reservation,
		const FShanmenActionTransitionReceipt& Transition,
		FShanmenActionResourceFinalizationRequest& OutRequest);

	bool IsValid() const;
	const FGuid& GetRequestId() const { return RequestId; }
	const FGuid& GetCommandId() const { return CommandId; }
	const FShanmenActionResourceReservationReceipt& GetReservation() const { return Reservation; }
	const FShanmenActionTransitionReceipt& GetTransition() const { return Transition; }
	EShanmenActionResourceDisposition GetDisposition() const { return Disposition; }

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Resource", meta = (AllowPrivateAccess = "true"))
	FGuid RequestId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Resource", meta = (AllowPrivateAccess = "true"))
	FGuid CommandId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Resource", meta = (AllowPrivateAccess = "true"))
	FShanmenActionResourceReservationReceipt Reservation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Resource", meta = (AllowPrivateAccess = "true"))
	FShanmenActionTransitionReceipt Transition;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Resource", meta = (AllowPrivateAccess = "true"))
	EShanmenActionResourceDisposition Disposition = EShanmenActionResourceDisposition::None;
};

/** Immutable proof of a committed cost or released reservation. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenActionResourceFinalizationReceipt
{
	GENERATED_BODY()

public:
	bool IsValid() const;
	const FGuid& GetReceiptId() const { return ReceiptId; }
	const FShanmenActionResourceFinalizationRequest& GetRequest() const { return Request; }
	int64 GetAuthorityRevisionBefore() const { return AuthorityRevisionBefore; }
	int64 GetAuthorityRevisionAfter() const { return AuthorityRevisionAfter; }
	float GetCurrentBefore() const { return CurrentBefore; }
	float GetCurrentAfter() const { return CurrentAfter; }
	float GetMaximumAmount() const { return MaximumAmount; }
	float GetReservedBefore() const { return ReservedBefore; }
	float GetReservedAfter() const { return ReservedAfter; }
	float GetAvailableBefore() const { return AvailableBefore; }
	float GetAvailableAfter() const { return AvailableAfter; }

private:
	friend class FShanmenActionResourceAuthority;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Resource", meta = (AllowPrivateAccess = "true"))
	FGuid ReceiptId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Resource", meta = (AllowPrivateAccess = "true"))
	FShanmenActionResourceFinalizationRequest Request;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Resource", meta = (AllowPrivateAccess = "true"))
	int64 AuthorityRevisionBefore = INDEX_NONE;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Resource", meta = (AllowPrivateAccess = "true"))
	int64 AuthorityRevisionAfter = INDEX_NONE;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Resource", meta = (AllowPrivateAccess = "true"))
	float CurrentBefore = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Resource", meta = (AllowPrivateAccess = "true"))
	float CurrentAfter = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Resource", meta = (AllowPrivateAccess = "true"))
	float MaximumAmount = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Resource", meta = (AllowPrivateAccess = "true"))
	float ReservedBefore = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Resource", meta = (AllowPrivateAccess = "true"))
	float ReservedAfter = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Resource", meta = (AllowPrivateAccess = "true"))
	float AvailableBefore = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Resource", meta = (AllowPrivateAccess = "true"))
	float AvailableAfter = 0.0f;
};

/** Structured reserve/finalize result. Rejections carry no valid proof. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenActionResourceTransactionResult
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Resource")
	EShanmenActionResourceTransactionStatus Status = EShanmenActionResourceTransactionStatus::Invalid;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Resource")
	EShanmenActionResourceTransactionError Error = EShanmenActionResourceTransactionError::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Resource")
	FShanmenActionResourceReservationReceipt Reservation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Resource")
	FShanmenActionResourceFinalizationReceipt Finalization;

	bool IsValid() const;
	bool IsSuccess() const;
};

/**
 * Pure single-writer authority for one owner and one typed combat resource.
 * Startup reservations hide capacity from competing actions. The exact action
 * commit transition consumes it; a pre-commit terminal transition releases it.
 * Final numeric balance and product projection remain outside action content.
 */
class SHANMENCOMBATRUNTIME_API FShanmenActionResourceAuthority
{
public:
	static bool TryCreate(
		const FGuid& OwnerEntityId,
		const FGameplayTag& ResourceChannel,
		float CurrentAmount,
		float MaximumAmount,
		int64 AuthorityRevision,
		FShanmenActionResourceAuthority& OutAuthority);

	bool IsValid() const;
	bool TryCaptureSnapshot(FShanmenActionResourceSnapshot& OutSnapshot) const;
	FShanmenActionResourceTransactionResult Reserve(
		const FShanmenActionResourceReservationRequest& Request);
	FShanmenActionResourceTransactionResult Finalize(
		const FShanmenActionResourceFinalizationRequest& Request);
	void Reset();

	const FGuid& GetOwnerEntityId() const { return OwnerEntityId; }
	FGameplayTag GetResourceChannel() const { return ResourceChannel; }
	float GetCurrentAmount() const { return CurrentAmount; }
	float GetMaximumAmount() const { return MaximumAmount; }
	float GetReservedAmount() const { return ReservedAmount; }
	float GetAvailableAmount() const { return CurrentAmount - ReservedAmount; }
	int64 GetAuthorityRevision() const { return AuthorityRevision; }
	int32 NumTransactions() const { return Transactions.Num(); }
	int32 NumPendingReservations() const;

private:
	struct FTransactionRecord
	{
		FGuid ReservationCommandId;
		FShanmenActionResourceReservationReceipt Reservation;
		FGuid FinalizationCommandId;
		FShanmenActionResourceFinalizationReceipt Finalization;
	};

	FShanmenActionResourceTransactionResult Reject(
		EShanmenActionResourceTransactionError Error) const;

	FGuid OwnerEntityId;
	FGameplayTag ResourceChannel;
	float CurrentAmount = 0.0f;
	float MaximumAmount = 0.0f;
	float ReservedAmount = 0.0f;
	int64 AuthorityRevision = INDEX_NONE;
	bool bInitialized = false;
	TMap<FGuid, FTransactionRecord> Transactions;
};
