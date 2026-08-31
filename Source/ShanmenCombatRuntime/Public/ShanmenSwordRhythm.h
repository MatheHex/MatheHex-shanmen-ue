#pragma once

#include "CoreMinimal.h"
#include "ShanmenCombatTypes.h"

#include "ShanmenSwordRhythm.generated.h"

/** Mutable content input for one sword-rhythm style. Timing remains content-owned. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenSwordRhythmDefinitionCapture
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|SwordRhythm")
	FName StyleDefinitionId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|SwordRhythm")
	FName RuleId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|SwordRhythm", meta = (ClampMin = "0"))
	int64 LinkOpenOffsetTicks = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|SwordRhythm", meta = (ClampMin = "1"))
	int64 LinkCloseOffsetTicks = 0;
};

/** Frozen rhythm definition for a content-authored BasicSword timing window. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenSwordRhythmDefinition
{
	GENERATED_BODY()

public:
	static FName CanonicalStyleDefinitionId();
	static bool TryCapture(
		const FShanmenSwordRhythmDefinitionCapture& Capture,
		FShanmenSwordRhythmDefinition& OutDefinition);

	bool IsValid() const;
	const FGuid& GetDefinitionId() const { return DefinitionId; }
	FName GetStyleDefinitionId() const { return StyleDefinitionId; }
	FName GetSupportedActionDefinitionId() const
	{
		return SupportedActionDefinitionId;
	}
	FName GetRuleId() const { return RuleId; }
	int64 GetLinkOpenOffsetTicks() const { return LinkOpenOffsetTicks; }
	int64 GetLinkCloseOffsetTicks() const { return LinkCloseOffsetTicks; }

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm", meta = (AllowPrivateAccess = "true"))
	FGuid DefinitionId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm", meta = (AllowPrivateAccess = "true"))
	FName StyleDefinitionId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm", meta = (AllowPrivateAccess = "true"))
	FName SupportedActionDefinitionId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm", meta = (AllowPrivateAccess = "true"))
	FName RuleId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm", meta = (AllowPrivateAccess = "true"))
	int64 LinkOpenOffsetTicks = INDEX_NONE;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm", meta = (AllowPrivateAccess = "true"))
	int64 LinkCloseOffsetTicks = INDEX_NONE;
};

/** One BasicSword activation sampled on a caller-owned monotonic timeline. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenSwordRhythmObservation
{
	GENERATED_BODY()

public:
	static bool TryCapture(
		const FShanmenCombatActionSnapshot& Action,
		const FGuid& TimelineId,
		int64 InputTick,
		FShanmenSwordRhythmObservation& OutObservation);

	bool IsValid() const;
	const FGuid& GetObservationId() const { return ObservationId; }
	const FShanmenCombatActionSnapshot& GetAction() const { return Action; }
	const FGuid& GetTimelineId() const { return TimelineId; }
	int64 GetInputTick() const { return InputTick; }

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm", meta = (AllowPrivateAccess = "true"))
	FGuid ObservationId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm", meta = (AllowPrivateAccess = "true"))
	FShanmenCombatActionSnapshot Action;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm", meta = (AllowPrivateAccess = "true"))
	FGuid TimelineId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm", meta = (AllowPrivateAccess = "true"))
	int64 InputTick = INDEX_NONE;
};

UENUM(BlueprintType)
enum class EShanmenSwordRhythmBand : uint8
{
	Invalid,
	Started,
	PreciseLinked,
	RestartedEarly,
	RestartedLate
};

/** Immutable result of accepting one observation into a sword-rhythm chain. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenSwordRhythmReceipt
{
	GENERATED_BODY()

public:
	bool IsValid() const;
	const FGuid& GetReceiptId() const { return ReceiptId; }
	const FShanmenSwordRhythmDefinition& GetDefinition() const
	{
		return Definition;
	}
	const FShanmenSwordRhythmObservation& GetCurrentObservation() const
	{
		return CurrentObservation;
	}
	const FShanmenSwordRhythmObservation& GetPreviousObservation() const
	{
		return PreviousObservation;
	}
	bool HasPreviousObservation() const
	{
		return PreviousObservation.IsValid();
	}
	int32 GetPreviousChainCount() const { return PreviousChainCount; }
	int32 GetResultingChainCount() const { return ResultingChainCount; }
	EShanmenSwordRhythmBand GetBand() const { return Band; }

private:
	friend class FShanmenSwordRhythmChain;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm", meta = (AllowPrivateAccess = "true"))
	FGuid ReceiptId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm", meta = (AllowPrivateAccess = "true"))
	FShanmenSwordRhythmDefinition Definition;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm", meta = (AllowPrivateAccess = "true"))
	FShanmenSwordRhythmObservation CurrentObservation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm", meta = (AllowPrivateAccess = "true"))
	FShanmenSwordRhythmObservation PreviousObservation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm", meta = (AllowPrivateAccess = "true"))
	int32 PreviousChainCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm", meta = (AllowPrivateAccess = "true"))
	int32 ResultingChainCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm", meta = (AllowPrivateAccess = "true"))
	EShanmenSwordRhythmBand Band = EShanmenSwordRhythmBand::Invalid;
};

/**
 * Deterministic, caller-clocked Tai Chi sword rhythm authority.
 *
 * It owns no World, timer, animation, input binding, damage bonus or RNG. The
 * caller supplies completed BasicSword activations and exact timeline ticks.
 */
class SHANMENCOMBATRUNTIME_API FShanmenSwordRhythmChain
{
public:
	static bool TryCreate(
		const FShanmenSwordRhythmDefinition& Definition,
		FShanmenSwordRhythmChain& OutChain);

	bool IsValid() const;
	bool TryObserve(
		const FShanmenSwordRhythmObservation& Observation,
		FShanmenSwordRhythmReceipt& OutReceipt);
	void Reset();

	bool IsEmpty() const;
	int32 NumRecordedObservations() const { return ReceiptsByActivation.Num(); }
	int32 GetCurrentChainCount() const { return CurrentChainCount; }
	const FShanmenSwordRhythmDefinition& GetDefinition() const
	{
		return Definition;
	}
	const FShanmenSwordRhythmObservation& GetLastObservation() const
	{
		return LastObservation;
	}

private:
	FShanmenSwordRhythmDefinition Definition;
	FShanmenSwordRhythmObservation LastObservation;
	int32 CurrentChainCount = 0;
	TMap<FGuid, FShanmenSwordRhythmReceipt> ReceiptsByActivation;
};
