#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenSwordRhythmPresentation.h"

#include "demo_mapShanmenSwordRhythmEffectCue.generated.h"

/** Presentation-only channel. It carries no gameplay application semantics. */
UENUM(BlueprintType)
enum class Edemo_mapShanmenSwordRhythmEffectCueChannel : uint8
{
	Invalid,
	Visual,
	Audio
};

/** Mutable authored mapping for one symbolic evaluation effect. */
USTRUCT(BlueprintType)
struct Fdemo_mapShanmenSwordRhythmEffectCueBindingCapture
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|SwordRhythm|EffectCue")
	FName EffectDefinitionId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|SwordRhythm|EffectCue")
	FName VisualCueDefinitionId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|SwordRhythm|EffectCue")
	FName AudioCueDefinitionId = NAME_None;
};

/** Immutable authored visual/audio vocabulary for one symbolic effect. */
USTRUCT(BlueprintType)
struct Fdemo_mapShanmenSwordRhythmEffectCueBinding
{
	GENERATED_BODY()

public:
	static bool TryCapture(
		const Fdemo_mapShanmenSwordRhythmEffectCueBindingCapture& Capture,
		const FShanmenContentStamp& Content,
		Fdemo_mapShanmenSwordRhythmEffectCueBinding& OutBinding);

	bool IsValid() const;
	const FGuid& GetBindingId() const { return BindingId; }
	FName GetEffectDefinitionId() const { return EffectDefinitionId; }
	FName GetVisualCueDefinitionId() const
	{
		return VisualCueDefinitionId;
	}
	FName GetAudioCueDefinitionId() const
	{
		return AudioCueDefinitionId;
	}
	const FShanmenContentStamp& GetContent() const { return Content; }

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|EffectCue", meta = (AllowPrivateAccess = "true"))
	FGuid BindingId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|EffectCue", meta = (AllowPrivateAccess = "true"))
	FName EffectDefinitionId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|EffectCue", meta = (AllowPrivateAccess = "true"))
	FName VisualCueDefinitionId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|EffectCue", meta = (AllowPrivateAccess = "true"))
	FName AudioCueDefinitionId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|EffectCue", meta = (AllowPrivateAccess = "true"))
	FShanmenContentStamp Content;
};

/** Mutable authored input for one complete effect-cue policy version. */
USTRUCT(BlueprintType)
struct Fdemo_mapShanmenSwordRhythmEffectCuePolicyCapture
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|SwordRhythm|EffectCue")
	FName PolicyDefinitionId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|SwordRhythm|EffectCue")
	FShanmenContentStamp Content;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|SwordRhythm|EffectCue")
	TArray<Fdemo_mapShanmenSwordRhythmEffectCueBindingCapture> Bindings;
};

/**
 * Immutable non-authoritative mapping from symbolic effects to cue names.
 *
 * Cue names are authored identities only. This policy owns no assets, World,
 * components, playback, magnitude, timing, damage or attribute mutation.
 */
USTRUCT(BlueprintType)
struct Fdemo_mapShanmenSwordRhythmEffectCuePolicy
{
	GENERATED_BODY()

public:
	static bool TryCapture(
		const Fdemo_mapShanmenSwordRhythmEffectCuePolicyCapture& Capture,
		Fdemo_mapShanmenSwordRhythmEffectCuePolicy& OutPolicy);

	bool IsValid() const;
	bool TryFindBinding(
		FName EffectDefinitionId,
		Fdemo_mapShanmenSwordRhythmEffectCueBinding& OutBinding) const;
	const FGuid& GetPolicyId() const { return PolicyId; }
	FName GetPolicyDefinitionId() const { return PolicyDefinitionId; }
	const FShanmenContentStamp& GetContent() const { return Content; }
	const TArray<Fdemo_mapShanmenSwordRhythmEffectCueBinding>& GetBindings()
		const
	{
		return Bindings;
	}

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|EffectCue", meta = (AllowPrivateAccess = "true"))
	FGuid PolicyId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|EffectCue", meta = (AllowPrivateAccess = "true"))
	FName PolicyDefinitionId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|EffectCue", meta = (AllowPrivateAccess = "true"))
	FShanmenContentStamp Content;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|EffectCue", meta = (AllowPrivateAccess = "true"))
	TArray<Fdemo_mapShanmenSwordRhythmEffectCueBinding> Bindings;
};

/** One typed, idempotent instruction for a later presentation consumer. */
USTRUCT(BlueprintType)
struct Fdemo_mapShanmenSwordRhythmEffectCueCommand
{
	GENERATED_BODY()

public:
	static bool TryCreate(
		const Fdemo_mapShanmenSwordRhythmPresentationState& State,
		const Fdemo_mapShanmenSwordRhythmEffectCuePolicy& Policy,
		const Fdemo_mapShanmenSwordRhythmEffectCueBinding& Binding,
		int32 EffectOrdinal,
		Edemo_mapShanmenSwordRhythmEffectCueChannel Channel,
		Fdemo_mapShanmenSwordRhythmEffectCueCommand& OutCommand);

	bool IsValid() const;
	const FGuid& GetCommandId() const { return CommandId; }
	const FGuid& GetPresentationStateId() const
	{
		return PresentationStateId;
	}
	const FGuid& GetEvaluationReceiptId() const
	{
		return EvaluationReceiptId;
	}
	const FGuid& GetCuePolicyId() const { return CuePolicyId; }
	const Fdemo_mapShanmenSwordRhythmEffectCueBinding& GetBinding() const
	{
		return Binding;
	}
	int32 GetEffectOrdinal() const { return EffectOrdinal; }
	Edemo_mapShanmenSwordRhythmEffectCueChannel GetChannel() const
	{
		return Channel;
	}
	FName GetCueDefinitionId() const { return CueDefinitionId; }

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|EffectCue", meta = (AllowPrivateAccess = "true"))
	FGuid CommandId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|EffectCue", meta = (AllowPrivateAccess = "true"))
	FGuid PresentationStateId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|EffectCue", meta = (AllowPrivateAccess = "true"))
	FGuid EvaluationReceiptId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|EffectCue", meta = (AllowPrivateAccess = "true"))
	FGuid CuePolicyId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|EffectCue", meta = (AllowPrivateAccess = "true"))
	Fdemo_mapShanmenSwordRhythmEffectCueBinding Binding;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|EffectCue", meta = (AllowPrivateAccess = "true"))
	int32 EffectOrdinal = INDEX_NONE;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|EffectCue", meta = (AllowPrivateAccess = "true"))
	Edemo_mapShanmenSwordRhythmEffectCueChannel Channel =
		Edemo_mapShanmenSwordRhythmEffectCueChannel::Invalid;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|EffectCue", meta = (AllowPrivateAccess = "true"))
	FName CueDefinitionId = NAME_None;
};

/** Self-validating event derived from one immutable presentation state. */
USTRUCT(BlueprintType)
struct Fdemo_mapShanmenSwordRhythmEffectCueEvent
{
	GENERATED_BODY()

public:
	static bool TryCreate(
		const Fdemo_mapShanmenSwordRhythmPresentationState& State,
		const Fdemo_mapShanmenSwordRhythmEffectCuePolicy& Policy,
		Fdemo_mapShanmenSwordRhythmEffectCueEvent& OutEvent);

	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenSwordRhythmEffectCueEvent& Other) const;
	const FGuid& GetEventId() const { return EventId; }
	const Fdemo_mapShanmenSwordRhythmPresentationState& GetState() const
	{
		return State;
	}
	const Fdemo_mapShanmenSwordRhythmEffectCuePolicy& GetPolicy() const
	{
		return Policy;
	}
	const TArray<Fdemo_mapShanmenSwordRhythmEffectCueCommand>& GetCommands()
		const
	{
		return Commands;
	}
	int32 NumCommands() const { return Commands.Num(); }

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|EffectCue", meta = (AllowPrivateAccess = "true"))
	FGuid EventId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|EffectCue", meta = (AllowPrivateAccess = "true"))
	Fdemo_mapShanmenSwordRhythmPresentationState State;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|EffectCue", meta = (AllowPrivateAccess = "true"))
	Fdemo_mapShanmenSwordRhythmEffectCuePolicy Policy;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|EffectCue", meta = (AllowPrivateAccess = "true"))
	TArray<Fdemo_mapShanmenSwordRhythmEffectCueCommand> Commands;
};

enum class Edemo_mapShanmenSwordRhythmEffectCueAdaptStatus : uint8
{
	Adapted,
	StateInvalid,
	PolicyInvalid,
	EffectUnmapped,
	EventRejected
};

struct Fdemo_mapShanmenSwordRhythmEffectCueAdaptResult
{
	Edemo_mapShanmenSwordRhythmEffectCueAdaptStatus Status =
		Edemo_mapShanmenSwordRhythmEffectCueAdaptStatus::StateInvalid;
	FString Diagnostic;
	Fdemo_mapShanmenSwordRhythmEffectCueEvent Event;

	bool IsAdapted() const
	{
		return Status
			== Edemo_mapShanmenSwordRhythmEffectCueAdaptStatus::Adapted
			&& Event.IsValid();
	}
};

/** Stateless read-model adapter; creates commands but never executes them. */
class Fdemo_mapShanmenSwordRhythmEffectCueAdapter
{
public:
	static Fdemo_mapShanmenSwordRhythmEffectCueAdaptResult Adapt(
		const Fdemo_mapShanmenSwordRhythmPresentationState& State,
		const Fdemo_mapShanmenSwordRhythmEffectCuePolicy& Policy);
};
