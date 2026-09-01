#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "demo_mapShanmenSwordRhythmEffectCuePresentationHandoffExecutor.h"

#include "demo_mapShanmenSwordRhythmEffectCuePresentationAcknowledgement.generated.h"

/** Mutable caller report for one authored presentation batch. */
USTRUCT(BlueprintType)
struct Fdemo_mapShanmenSwordRhythmEffectCuePresentationAcknowledgementCapture
{
	GENERATED_BODY()

	/** Stable authored presenter identity, not an asset path or loaded object. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|SwordRhythm|EffectCue|Presentation")
	FName PresenterDefinitionId = NAME_None;

	/** Command identities completed by the presenter, in handoff order. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|SwordRhythm|EffectCue|Presentation")
	TArray<FGuid> CompletedCommandIds;
};

/**
 * Immutable caller acknowledgement for one complete presentation handoff.
 *
 * This value proves that a caller attested to the exact ordered command batch.
 * It deliberately does not prove that an animation, VFX, or audio asset played.
 */
USTRUCT(BlueprintType)
struct Fdemo_mapShanmenSwordRhythmEffectCuePresentationAcknowledgement
{
	GENERATED_BODY()

public:
	static bool TryCapture(
		const Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoff& Handoff,
		const Fdemo_mapShanmenSwordRhythmEffectCuePresentationAcknowledgementCapture& Capture,
		Fdemo_mapShanmenSwordRhythmEffectCuePresentationAcknowledgement& OutAcknowledgement,
		FString& OutDiagnostic);

	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoff& OtherHandoff) const;
	bool Matches(
		const Fdemo_mapShanmenSwordRhythmEffectCuePresentationAcknowledgement& Other) const;
	const FGuid& GetAcknowledgementId() const { return AcknowledgementId; }
	const Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoff& GetHandoff() const
	{
		return Handoff;
	}
	const FGuid& GetHandoffId() const { return Handoff.GetHandoffId(); }
	Edemo_mapShanmenSwordRhythmEffectCueChannel GetChannel() const
	{
		return Handoff.GetChannel();
	}
	FName GetPresenterDefinitionId() const { return PresenterDefinitionId; }
	const TArray<FGuid>& GetCompletedCommandIds() const
	{
		return CompletedCommandIds;
	}

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|EffectCue|Presentation", meta = (AllowPrivateAccess = "true"))
	FGuid AcknowledgementId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|EffectCue|Presentation", meta = (AllowPrivateAccess = "true"))
	Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoff Handoff;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|EffectCue|Presentation", meta = (AllowPrivateAccess = "true"))
	FName PresenterDefinitionId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|EffectCue|Presentation", meta = (AllowPrivateAccess = "true"))
	TArray<FGuid> CompletedCommandIds;
};

/** Stateless Blueprint adapter for extracting and acknowledging handoff batches. */
UCLASS()
class DEMO_MAP_API Udemo_mapShanmenSwordRhythmEffectCuePresentationAcknowledgementLibrary
	: public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Returns the exact command order a presenter must acknowledge. */
	UFUNCTION(BlueprintPure, Category = "Shanmen|Combat|SwordRhythm|EffectCue|Presentation")
	static TArray<FGuid> GetOrderedCommandIds(
		const Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoff& Handoff);

	/** Seals a complete caller acknowledgement; incomplete or reordered input fails closed. */
	UFUNCTION(BlueprintCallable, Category = "Shanmen|Combat|SwordRhythm|EffectCue|Presentation")
	static bool TryAcknowledge(
		const Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoff& Handoff,
		FName PresenterDefinitionId,
		const TArray<FGuid>& CompletedCommandIds,
		Fdemo_mapShanmenSwordRhythmEffectCuePresentationAcknowledgement& OutAcknowledgement,
		FString& OutDiagnostic);
};
