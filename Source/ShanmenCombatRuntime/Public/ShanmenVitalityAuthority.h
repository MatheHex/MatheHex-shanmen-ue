#pragma once

#include "CoreMinimal.h"
#include "ShanmenCombatTypes.h"

#include "ShanmenVitalityAuthority.generated.h"

UENUM(BlueprintType)
enum class EShanmenVitalityCommitStatus : uint8
{
	Invalid,
	Committed,
	AlreadyCommitted,
	Rejected
};

UENUM(BlueprintType)
enum class EShanmenVitalityCommitError : uint8
{
	None,
	InvalidCommand,
	AuthorityNotReady,
	TargetMismatch,
	StaleSnapshot,
	ImpactConflict,
	RevisionExhausted
};

/** Immutable application intent built only from a canonical resolved Impact. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenVitalityCommitCommand
{
	GENERATED_BODY()

public:
	static bool TryCreate(
		const FShanmenImpactRequest& Request,
		const FShanmenImpactResult& Result,
		FShanmenVitalityCommitCommand& OutCommand);

	bool IsValid() const;
	const FGuid& GetImpactId() const { return ImpactId; }
	const FGuid& GetResolutionId() const { return ResolutionId; }
	const FGuid& GetTargetEntityId() const { return TargetEntityId; }
	int64 GetExpectedAuthorityRevision() const { return ExpectedAuthorityRevision; }
	float GetExpectedCurrentVitality() const { return ExpectedCurrentVitality; }
	float GetExpectedMaximumVitality() const { return ExpectedMaximumVitality; }
	float GetRawDamage() const { return RawDamage; }
	float GetPreventedDamage() const { return PreventedDamage; }
	float GetRequestedDamage() const { return RequestedDamage; }
	EShanmenDefenseOutcome GetDefenseOutcome() const { return DefenseOutcome; }

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Vitality", meta = (AllowPrivateAccess = "true"))
	FGuid ImpactId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Vitality", meta = (AllowPrivateAccess = "true"))
	FGuid ResolutionId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Vitality", meta = (AllowPrivateAccess = "true"))
	FGuid TargetEntityId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Vitality", meta = (AllowPrivateAccess = "true"))
	int64 ExpectedAuthorityRevision = INDEX_NONE;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Vitality", meta = (AllowPrivateAccess = "true"))
	float ExpectedCurrentVitality = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Vitality", meta = (AllowPrivateAccess = "true"))
	float ExpectedMaximumVitality = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Vitality", meta = (AllowPrivateAccess = "true"))
	float RawDamage = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Vitality", meta = (AllowPrivateAccess = "true"))
	float PreventedDamage = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Vitality", meta = (AllowPrivateAccess = "true"))
	float RequestedDamage = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Vitality", meta = (AllowPrivateAccess = "true"))
	EShanmenDefenseOutcome DefenseOutcome = EShanmenDefenseOutcome::Invalid;
};

/** Immutable proof of one successful, revision-checked vitality mutation. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenVitalityCommitReceipt
{
	GENERATED_BODY()

public:
	bool IsValid() const;
	const FGuid& GetImpactId() const { return ImpactId; }
	const FGuid& GetResolutionId() const { return ResolutionId; }
	const FGuid& GetTargetEntityId() const { return TargetEntityId; }
	int64 GetAuthorityRevisionBefore() const { return AuthorityRevisionBefore; }
	int64 GetAuthorityRevisionAfter() const { return AuthorityRevisionAfter; }
	float GetVitalityBefore() const { return VitalityBefore; }
	float GetVitalityAfter() const { return VitalityAfter; }
	float GetMaximumVitality() const { return MaximumVitality; }
	float GetRequestedDamage() const { return RequestedDamage; }
	float GetAppliedDamage() const { return AppliedDamage; }

private:
	friend class FShanmenVitalityAuthority;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Vitality", meta = (AllowPrivateAccess = "true"))
	FGuid ImpactId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Vitality", meta = (AllowPrivateAccess = "true"))
	FGuid ResolutionId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Vitality", meta = (AllowPrivateAccess = "true"))
	FGuid TargetEntityId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Vitality", meta = (AllowPrivateAccess = "true"))
	int64 AuthorityRevisionBefore = INDEX_NONE;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Vitality", meta = (AllowPrivateAccess = "true"))
	int64 AuthorityRevisionAfter = INDEX_NONE;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Vitality", meta = (AllowPrivateAccess = "true"))
	float VitalityBefore = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Vitality", meta = (AllowPrivateAccess = "true"))
	float VitalityAfter = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Vitality", meta = (AllowPrivateAccess = "true"))
	float MaximumVitality = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Vitality", meta = (AllowPrivateAccess = "true"))
	float RequestedDamage = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Vitality", meta = (AllowPrivateAccess = "true"))
	float AppliedDamage = 0.0f;
};

/** Structured command result. Rejections never contain a mutation receipt. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenVitalityCommitResult
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Vitality")
	EShanmenVitalityCommitStatus Status = EShanmenVitalityCommitStatus::Invalid;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Vitality")
	EShanmenVitalityCommitError Error = EShanmenVitalityCommitError::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Vitality")
	FShanmenVitalityCommitReceipt Receipt;

	bool IsValid() const;
	bool IsSuccess() const;
};

/**
 * Deterministic in-memory vitality authority for one stable combat EntityId.
 *
 * Product adapters may project this state to presentation, but must not apply
 * the same Impact through a second health path. Every accepted mutation is
 * revision-checked and idempotent by ImpactId + ResolutionId.
 */
class SHANMENCOMBATRUNTIME_API FShanmenVitalityAuthority
{
public:
	static bool TryCreate(
		const FGuid& TargetEntityId,
		float CurrentVitality,
		float MaximumVitality,
		int64 AuthorityRevision,
		FShanmenVitalityAuthority& OutAuthority);

	bool IsValid() const;
	bool TryCaptureSnapshot(FShanmenTargetVitalitySnapshot& OutSnapshot) const;
	FShanmenVitalityCommitResult Commit(const FShanmenVitalityCommitCommand& Command);
	void Reset();

	const FGuid& GetTargetEntityId() const { return TargetEntityId; }
	float GetCurrentVitality() const { return CurrentVitality; }
	float GetMaximumVitality() const { return MaximumVitality; }
	int64 GetAuthorityRevision() const { return AuthorityRevision; }
	int32 NumCommittedImpacts() const { return ProcessedImpacts.Num(); }

private:
	struct FProcessedImpact
	{
		FGuid ResolutionId;
		FShanmenVitalityCommitReceipt Receipt;
	};

	FShanmenVitalityCommitResult Reject(EShanmenVitalityCommitError Error) const;

	FGuid TargetEntityId;
	float CurrentVitality = 0.0f;
	float MaximumVitality = 0.0f;
	int64 AuthorityRevision = INDEX_NONE;
	bool bInitialized = false;
	TMap<FGuid, FProcessedImpact> ProcessedImpacts;
};
