#pragma once

#include "CoreMinimal.h"
#include "ShanmenSpiritShield.h"

#include "ShanmenSpiritShieldCapacityAuthority.generated.h"

UENUM(BlueprintType)
enum class EShanmenSpiritShieldCapacityCommitStatus : uint8
{
	Invalid,
	Committed,
	AlreadyCommitted,
	Rejected
};

UENUM(BlueprintType)
enum class EShanmenSpiritShieldCapacityCommitError : uint8
{
	None,
	InvalidCommand,
	AuthorityNotReady,
	ShieldMismatch,
	StaleProjection,
	ImpactConflict,
	CapacityExceeded,
	RevisionExhausted
};

/**
 * Immutable intent to consume the exact shield layer triggered by one
 * canonical CombatCore resolution.
 */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenSpiritShieldCapacityCommitCommand
{
	GENERATED_BODY()

public:
	static bool TryCreate(
		const FShanmenSpiritShieldProjectionReceipt& Projection,
		const FShanmenImpactRequest& Request,
		const FShanmenImpactResult& Result,
		FShanmenSpiritShieldCapacityCommitCommand& OutCommand);

	bool IsValid() const;
	const FGuid& GetCommandId() const { return CommandId; }
	const FGuid& GetResolutionId() const { return ResolutionId; }
	const FGuid& GetImpactId() const { return ImpactId; }
	const FShanmenSpiritShieldProjectionReceipt& GetProjection() const
	{
		return Projection;
	}
	float GetRequestedCapacity() const { return RequestedCapacity; }

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield", meta = (AllowPrivateAccess = "true"))
	FGuid CommandId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield", meta = (AllowPrivateAccess = "true"))
	FGuid ResolutionId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield", meta = (AllowPrivateAccess = "true"))
	FGuid ImpactId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield", meta = (AllowPrivateAccess = "true"))
	FShanmenSpiritShieldProjectionReceipt Projection;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield", meta = (AllowPrivateAccess = "true"))
	float RequestedCapacity = 0.0f;
};

/** Immutable proof of one successful revision-checked capacity consumption. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenSpiritShieldCapacityCommitReceipt
{
	GENERATED_BODY()

public:
	bool IsValid() const;
	const FGuid& GetReceiptId() const { return ReceiptId; }
	const FGuid& GetCommandId() const { return CommandId; }
	const FGuid& GetImpactId() const { return ImpactId; }
	const FGuid& GetProjectionId() const { return ProjectionId; }
	const FGuid& GetShieldInstanceId() const { return ShieldInstanceId; }
	int64 GetAuthorityRevisionBefore() const { return AuthorityRevisionBefore; }
	int64 GetAuthorityRevisionAfter() const { return AuthorityRevisionAfter; }
	float GetCapacityBefore() const { return CapacityBefore; }
	float GetCommittedCapacity() const { return CommittedCapacity; }
	float GetCapacityAfter() const { return CapacityAfter; }
	bool IsDepleted() const { return IsValid() && CapacityAfter == 0.0f; }

private:
	friend class FShanmenSpiritShieldCapacityAuthority;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield", meta = (AllowPrivateAccess = "true"))
	FGuid ReceiptId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield", meta = (AllowPrivateAccess = "true"))
	FGuid CommandId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield", meta = (AllowPrivateAccess = "true"))
	FGuid ImpactId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield", meta = (AllowPrivateAccess = "true"))
	FGuid ProjectionId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield", meta = (AllowPrivateAccess = "true"))
	FGuid ShieldInstanceId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield", meta = (AllowPrivateAccess = "true"))
	int64 AuthorityRevisionBefore = INDEX_NONE;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield", meta = (AllowPrivateAccess = "true"))
	int64 AuthorityRevisionAfter = INDEX_NONE;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield", meta = (AllowPrivateAccess = "true"))
	float CapacityBefore = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield", meta = (AllowPrivateAccess = "true"))
	float CommittedCapacity = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield", meta = (AllowPrivateAccess = "true"))
	float CapacityAfter = 0.0f;
};

/** Structured command result. Rejections never contain a mutation receipt. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenSpiritShieldCapacityCommitResult
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield")
	EShanmenSpiritShieldCapacityCommitStatus Status =
		EShanmenSpiritShieldCapacityCommitStatus::Invalid;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield")
	EShanmenSpiritShieldCapacityCommitError Error =
		EShanmenSpiritShieldCapacityCommitError::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield")
	FShanmenSpiritShieldCapacityCommitReceipt Receipt;

	bool IsValid() const;
	bool IsSuccess() const;
};

/**
 * Pure single-writer capacity authority for one activated spirit shield.
 *
 * It owns only remaining capacity, revision and Impact idempotency. Product
 * code still owns duration, input, energy and explicit shield deactivation.
 */
class SHANMENCOMBATRUNTIME_API FShanmenSpiritShieldCapacityAuthority
{
public:
	static bool TryCreate(
		const FShanmenSpiritShieldActivationReceipt& Activation,
		FShanmenSpiritShieldCapacityAuthority& OutAuthority);

	bool IsValid() const;
	bool TryProjectDefenseLayer(
		FShanmenSpiritShieldRuntime& ShieldRuntime,
		FShanmenSpiritShieldProjectionReceipt& OutProjection) const;
	FShanmenSpiritShieldCapacityCommitResult Commit(
		const FShanmenSpiritShieldCapacityCommitCommand& Command);
	void Reset();

	const FShanmenSpiritShieldActivationReceipt& GetActivation() const
	{
		return Activation;
	}
	float GetAvailableCapacity() const { return AvailableCapacity; }
	float GetMaximumCapacity() const
	{
		return Activation.IsValid()
			? Activation.GetDefinition().GetMaximumCapacity()
			: 0.0f;
	}
	int64 GetAuthorityRevision() const { return AuthorityRevision; }
	int32 NumCommittedImpacts() const { return ProcessedImpacts.Num(); }
	bool IsDepleted() const { return IsValid() && AvailableCapacity == 0.0f; }

private:
	struct FProcessedImpact
	{
		FGuid CommandId;
		FShanmenSpiritShieldCapacityCommitReceipt Receipt;
	};

	FShanmenSpiritShieldCapacityCommitResult Reject(
		EShanmenSpiritShieldCapacityCommitError Error) const;

	FShanmenSpiritShieldActivationReceipt Activation;
	float AvailableCapacity = 0.0f;
	int64 AuthorityRevision = INDEX_NONE;
	bool bInitialized = false;
	TMap<FGuid, FProcessedImpact> ProcessedImpacts;
};
