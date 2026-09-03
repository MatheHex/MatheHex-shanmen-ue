#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ShanmenCombatTypes.h"

#include "ShanmenDivineSenseScan.generated.h"

/** Whether a Divine Sense pulse may reveal evidence behind world occlusion. */
UENUM(BlueprintType)
enum class EShanmenDivineSenseOcclusionPolicy : uint8
{
	VisibleOnly,
	RevealOccluded
};

/** Mutable authored values for one Divine Sense pulse rule. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenDivineSenseDefinitionCapture
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|DivineSense")
	FName ActionDefinitionId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|DivineSense")
	FName ScanRuleId = NAME_None;

	/** Authored range; P19.0 does not freeze a final balance value. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|DivineSense", meta = (ClampMin = "0.0"))
	double Radius = 0.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|DivineSense", meta = (ClampMin = "1"))
	int32 MaximumResults = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|DivineSense")
	EShanmenDivineSenseOcclusionPolicy OcclusionPolicy =
		EShanmenDivineSenseOcclusionPolicy::VisibleOnly;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|DivineSense")
	FGameplayTagContainer RequiredSubjectTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|DivineSense")
	FGameplayTagContainer BlockedSubjectTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|DivineSense")
	bool bRejectSelf = true;
};

/** Immutable, content-owned policy for an omnidirectional Divine Sense pulse. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenDivineSenseDefinition
{
	GENERATED_BODY()

public:
	static FName CanonicalActionDefinitionId();
	static bool TryCapture(
		const FShanmenDivineSenseDefinitionCapture& Capture,
		FShanmenDivineSenseDefinition& OutDefinition);

	bool IsValid() const;
	const FGuid& GetDefinitionId() const { return DefinitionId; }
	FName GetActionDefinitionId() const { return ActionDefinitionId; }
	FName GetScanRuleId() const { return ScanRuleId; }
	double GetRadius() const { return Radius; }
	int32 GetMaximumResults() const { return MaximumResults; }
	EShanmenDivineSenseOcclusionPolicy GetOcclusionPolicy() const
	{
		return OcclusionPolicy;
	}
	const FGameplayTagContainer& GetRequiredSubjectTags() const
	{
		return RequiredSubjectTags;
	}
	const FGameplayTagContainer& GetBlockedSubjectTags() const
	{
		return BlockedSubjectTags;
	}
	bool RejectsSelf() const { return bRejectSelf; }

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|DivineSense", meta = (AllowPrivateAccess = "true"))
	FGuid DefinitionId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|DivineSense", meta = (AllowPrivateAccess = "true"))
	FName ActionDefinitionId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|DivineSense", meta = (AllowPrivateAccess = "true"))
	FName ScanRuleId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|DivineSense", meta = (AllowPrivateAccess = "true"))
	double Radius = 0.0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|DivineSense", meta = (AllowPrivateAccess = "true"))
	int32 MaximumResults = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|DivineSense", meta = (AllowPrivateAccess = "true"))
	EShanmenDivineSenseOcclusionPolicy OcclusionPolicy =
		EShanmenDivineSenseOcclusionPolicy::VisibleOnly;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|DivineSense", meta = (AllowPrivateAccess = "true"))
	FGameplayTagContainer RequiredSubjectTags;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|DivineSense", meta = (AllowPrivateAccess = "true"))
	FGameplayTagContainer BlockedSubjectTags;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|DivineSense", meta = (AllowPrivateAccess = "true"))
	bool bRejectSelf = true;
};

/** Immutable identity and origin for one logical Divine Sense sample. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenDivineSenseScanRequest
{
	GENERATED_BODY()

public:
	static bool TryCapture(
		const FShanmenCombatActionSnapshot& Action,
		const FShanmenDivineSenseDefinition& Definition,
		const FVector& Origin,
		int32 ScanOrdinal,
		FShanmenDivineSenseScanRequest& OutRequest);

	bool IsValid() const;
	const FGuid& GetScanId() const { return ScanId; }
	const FShanmenCombatActionSnapshot& GetAction() const { return Action; }
	const FShanmenDivineSenseDefinition& GetDefinition() const
	{
		return Definition;
	}
	const FVector& GetOrigin() const { return Origin; }
	int32 GetScanOrdinal() const { return ScanOrdinal; }

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|DivineSense", meta = (AllowPrivateAccess = "true"))
	FGuid ScanId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|DivineSense", meta = (AllowPrivateAccess = "true"))
	FShanmenCombatActionSnapshot Action;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|DivineSense", meta = (AllowPrivateAccess = "true"))
	FShanmenDivineSenseDefinition Definition;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|DivineSense", meta = (AllowPrivateAccess = "true"))
	FVector Origin = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|DivineSense", meta = (AllowPrivateAccess = "true"))
	int32 ScanOrdinal = INDEX_NONE;
};

/** Mutable world/authority evidence captured for one scan and one subject. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenDivineSenseObservationCapture
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|DivineSense")
	FGuid SubjectEntityId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|DivineSense")
	FVector WorldLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|DivineSense")
	FGameplayTagContainer SubjectTags;

	/** Supplied by the future world adapter; the pure resolver performs no trace. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|DivineSense")
	bool bHasLineOfSight = false;

	/** Monotonic source-authority revision sampled with this subject evidence. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|DivineSense", meta = (ClampMin = "0"))
	int64 AuthorityRevision = 0;
};

/** Immutable subject evidence bound to one exact Divine Sense scan. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenDivineSenseObservation
{
	GENERATED_BODY()

public:
	static bool TryCapture(
		const FShanmenDivineSenseScanRequest& Request,
		const FShanmenDivineSenseObservationCapture& Capture,
		FShanmenDivineSenseObservation& OutObservation);

	bool IsValid() const;
	const FGuid& GetObservationId() const { return ObservationId; }
	const FGuid& GetScanId() const { return ScanId; }
	const FGuid& GetSubjectEntityId() const { return SubjectEntityId; }
	const FVector& GetWorldLocation() const { return WorldLocation; }
	const FGameplayTagContainer& GetSubjectTags() const { return SubjectTags; }
	bool HasLineOfSight() const { return bHasLineOfSight; }
	int64 GetAuthorityRevision() const { return AuthorityRevision; }

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|DivineSense", meta = (AllowPrivateAccess = "true"))
	FGuid ObservationId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|DivineSense", meta = (AllowPrivateAccess = "true"))
	FGuid ScanId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|DivineSense", meta = (AllowPrivateAccess = "true"))
	FGuid SubjectEntityId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|DivineSense", meta = (AllowPrivateAccess = "true"))
	FVector WorldLocation = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|DivineSense", meta = (AllowPrivateAccess = "true"))
	FGameplayTagContainer SubjectTags;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|DivineSense", meta = (AllowPrivateAccess = "true"))
	bool bHasLineOfSight = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|DivineSense", meta = (AllowPrivateAccess = "true"))
	int64 AuthorityRevision = 0;
};

/** One subject that the configured pulse is allowed to expose to consumers. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenDivineSenseReveal
{
	GENERATED_BODY()

public:
	bool IsValid() const;
	const FGuid& GetRevealId() const { return RevealId; }
	const FShanmenDivineSenseObservation& GetObservation() const
	{
		return Observation;
	}
	double GetDistanceSquared() const { return DistanceSquared; }
	bool WasOccluded() const
	{
		return IsValid() && !Observation.HasLineOfSight();
	}

private:
	friend class FShanmenDivineSenseResolver;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|DivineSense", meta = (AllowPrivateAccess = "true"))
	FGuid RevealId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|DivineSense", meta = (AllowPrivateAccess = "true"))
	FShanmenDivineSenseObservation Observation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|DivineSense", meta = (AllowPrivateAccess = "true"))
	double DistanceSquared = 0.0;
};

/** Consumer-safe result: rejected observations are never exposed by this value. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenDivineSenseScanReceipt
{
	GENERATED_BODY()

public:
	bool IsValid() const;
	const FGuid& GetReceiptId() const { return ReceiptId; }
	const FShanmenDivineSenseScanRequest& GetRequest() const { return Request; }
	const TArray<FShanmenDivineSenseReveal>& GetReveals() const
	{
		return Reveals;
	}
	int32 NumReveals() const { return Reveals.Num(); }

private:
	friend class FShanmenDivineSenseResolver;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|DivineSense", meta = (AllowPrivateAccess = "true"))
	FGuid ReceiptId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|DivineSense", meta = (AllowPrivateAccess = "true"))
	FShanmenDivineSenseScanRequest Request;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|DivineSense", meta = (AllowPrivateAccess = "true"))
	TArray<FShanmenDivineSenseReveal> Reveals;
};

/**
 * Pure deterministic filter over already-sampled subject evidence.
 *
 * It performs no World query, trace, Actor lookup, input read, energy mutation,
 * target mutation, timer, persistence, or presentation work.
 */
class SHANMENCOMBATRUNTIME_API FShanmenDivineSenseResolver
{
public:
	static bool TryResolve(
		const FShanmenDivineSenseScanRequest& Request,
		const TArray<FShanmenDivineSenseObservation>& Observations,
		FShanmenDivineSenseScanReceipt& OutReceipt);
};
