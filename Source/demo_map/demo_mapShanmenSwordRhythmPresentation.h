#pragma once

#include "CoreMinimal.h"
#include "ShanmenSwordRhythmEvaluation.h"

#include "demo_mapShanmenSwordRhythmPresentation.generated.h"

class Fdemo_mapShanmenSwordRhythmProductConfig;

/**
 * Immutable, self-validating read model for one accepted sword-rhythm receipt.
 *
 * Presentation and animation code may copy and inspect this value. It owns no
 * input, animation command, timer, damage multiplier or mutable chain state.
 */
USTRUCT(BlueprintType)
struct Fdemo_mapShanmenSwordRhythmPresentationState
{
	GENERATED_BODY()

public:
	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenSwordRhythmPresentationState& Other) const;

	const FGuid& GetPresentationStateId() const
	{
		return PresentationStateId;
	}
	const FGuid& GetRunId() const { return RunId; }
	const FGuid& GetConfigId() const { return ConfigId; }
	FName GetContentVersion() const { return ContentVersion; }
	const FString& GetContentDigest() const { return ContentDigest; }
	const FGuid& GetReceiptId() const { return ReceiptId; }
	const FGuid& GetEvaluationReceiptId() const
	{
		return EvaluationReceiptId;
	}
	const FGuid& GetEvaluationPolicyId() const
	{
		return EvaluationPolicyId;
	}
	const TArray<FName>& GetEffectDefinitionIds() const
	{
		return EffectDefinitionIds;
	}
	int32 NumEffectDefinitions() const
	{
		return EffectDefinitionIds.Num();
	}
	const FGuid& GetActivationId() const { return ActivationId; }
	const FGuid& GetTimelineId() const { return TimelineId; }
	FName GetStyleDefinitionId() const { return StyleDefinitionId; }
	FName GetRuleId() const { return RuleId; }
	int64 GetLinkOpenOffsetTicks() const { return LinkOpenOffsetTicks; }
	int64 GetLinkCloseOffsetTicks() const { return LinkCloseOffsetTicks; }
	int64 GetTimelineTicksPerSecond() const { return TimelineTicksPerSecond; }
	int64 GetPreviousInputTick() const { return PreviousInputTick; }
	int64 GetCurrentInputTick() const { return CurrentInputTick; }
	int32 GetObservationRevision() const { return ObservationRevision; }
	int32 GetPreviousChainCount() const { return PreviousChainCount; }
	int32 GetResultingChainCount() const { return ResultingChainCount; }
	EShanmenSwordRhythmBand GetBand() const { return Band; }

private:
	friend class Fdemo_mapShanmenSwordRhythmPresentationProjector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|Presentation", meta = (AllowPrivateAccess = "true"))
	FGuid PresentationStateId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|Presentation", meta = (AllowPrivateAccess = "true"))
	FGuid RunId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|Presentation", meta = (AllowPrivateAccess = "true"))
	FGuid ConfigId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|Presentation", meta = (AllowPrivateAccess = "true"))
	FName ContentVersion = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|Presentation", meta = (AllowPrivateAccess = "true"))
	FString ContentDigest;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|Presentation", meta = (AllowPrivateAccess = "true"))
	FGuid ReceiptId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|Presentation", meta = (AllowPrivateAccess = "true"))
	FGuid EvaluationReceiptId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|Presentation", meta = (AllowPrivateAccess = "true"))
	FGuid EvaluationPolicyId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|Presentation", meta = (AllowPrivateAccess = "true"))
	TArray<FName> EffectDefinitionIds;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|Presentation", meta = (AllowPrivateAccess = "true"))
	FGuid ActivationId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|Presentation", meta = (AllowPrivateAccess = "true"))
	FGuid TimelineId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|Presentation", meta = (AllowPrivateAccess = "true"))
	FName StyleDefinitionId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|Presentation", meta = (AllowPrivateAccess = "true"))
	FName RuleId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|Presentation", meta = (AllowPrivateAccess = "true"))
	int64 LinkOpenOffsetTicks = INDEX_NONE;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|Presentation", meta = (AllowPrivateAccess = "true"))
	int64 LinkCloseOffsetTicks = INDEX_NONE;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|Presentation", meta = (AllowPrivateAccess = "true"))
	int64 TimelineTicksPerSecond = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|Presentation", meta = (AllowPrivateAccess = "true"))
	int64 PreviousInputTick = INDEX_NONE;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|Presentation", meta = (AllowPrivateAccess = "true"))
	int64 CurrentInputTick = INDEX_NONE;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|Presentation", meta = (AllowPrivateAccess = "true"))
	int32 ObservationRevision = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|Presentation", meta = (AllowPrivateAccess = "true"))
	int32 PreviousChainCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|Presentation", meta = (AllowPrivateAccess = "true"))
	int32 ResultingChainCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|Presentation", meta = (AllowPrivateAccess = "true"))
	EShanmenSwordRhythmBand Band = EShanmenSwordRhythmBand::Invalid;
};

enum class Edemo_mapShanmenSwordRhythmPresentationProjectionStatus : uint8
{
	Projected,
	ConfigInvalid,
	RunInvalid,
	ReceiptInvalid,
	EvaluationReceiptInvalid,
	RevisionInvalid,
	IdentityMismatch,
	ProjectionRejected
};

struct Fdemo_mapShanmenSwordRhythmPresentationProjectionResult
{
	Edemo_mapShanmenSwordRhythmPresentationProjectionStatus Status =
		Edemo_mapShanmenSwordRhythmPresentationProjectionStatus::ConfigInvalid;
	FString Diagnostic;
	Fdemo_mapShanmenSwordRhythmPresentationState State;

	bool IsProjected() const
	{
		return Status
			== Edemo_mapShanmenSwordRhythmPresentationProjectionStatus::Projected
			&& State.IsValid();
	}
};

/** Stateless receipt-to-read-model projection. */
class Fdemo_mapShanmenSwordRhythmPresentationProjector
{
public:
	static Fdemo_mapShanmenSwordRhythmPresentationProjectionResult Project(
		const Fdemo_mapShanmenSwordRhythmProductConfig& Config,
		const FGuid& RunId,
		const FShanmenSwordRhythmReceipt& Receipt,
		const FShanmenSwordRhythmEvaluationReceipt& EvaluationReceipt,
		int32 ObservationRevision);
};
