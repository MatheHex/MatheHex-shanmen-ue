#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ShanmenActionOrchestrator.h"

#include "ShanmenSpiritEvasion.generated.h"

/** Mutable authoring input for one short, action-bound spirit-evasion window. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenSpiritEvasionDefinitionCapture
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|SpiritEvasion")
	FName ActionDefinitionId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|SpiritEvasion")
	FName RuleId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|SpiritEvasion")
	FGameplayTagContainer RequiredDamageTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|SpiritEvasion")
	FGameplayTagContainer BlockedDamageTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|SpiritEvasion")
	FGameplayTagContainer RequiredSourceTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|SpiritEvasion")
	FGameplayTagContainer BlockedSourceTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|SpiritEvasion")
	FGameplayTagContainer RequiredTargetTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|SpiritEvasion")
	FGameplayTagContainer BlockedTargetTags;
};

/** Immutable coverage rules; distance, duration and resource cost remain external. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenSpiritEvasionDefinition
{
	GENERATED_BODY()

public:
	static FName CanonicalActionDefinitionId();
	static bool TryCapture(
		const FShanmenSpiritEvasionDefinitionCapture& Capture,
		FShanmenSpiritEvasionDefinition& OutDefinition);

	bool IsValid() const;
	FName GetActionDefinitionId() const { return ActionDefinitionId; }
	FName GetRuleId() const { return RuleId; }
	const FGameplayTagContainer& GetRequiredDamageTags() const { return RequiredDamageTags; }
	const FGameplayTagContainer& GetBlockedDamageTags() const { return BlockedDamageTags; }
	const FGameplayTagContainer& GetRequiredSourceTags() const { return RequiredSourceTags; }
	const FGameplayTagContainer& GetBlockedSourceTags() const { return BlockedSourceTags; }
	const FGameplayTagContainer& GetRequiredTargetTags() const { return RequiredTargetTags; }
	const FGameplayTagContainer& GetBlockedTargetTags() const { return BlockedTargetTags; }

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritEvasion", meta = (AllowPrivateAccess = "true"))
	FName ActionDefinitionId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritEvasion", meta = (AllowPrivateAccess = "true"))
	FName RuleId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritEvasion", meta = (AllowPrivateAccess = "true"))
	FGameplayTagContainer RequiredDamageTags;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritEvasion", meta = (AllowPrivateAccess = "true"))
	FGameplayTagContainer BlockedDamageTags;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritEvasion", meta = (AllowPrivateAccess = "true"))
	FGameplayTagContainer RequiredSourceTags;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritEvasion", meta = (AllowPrivateAccess = "true"))
	FGameplayTagContainer BlockedSourceTags;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritEvasion", meta = (AllowPrivateAccess = "true"))
	FGameplayTagContainer RequiredTargetTags;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritEvasion", meta = (AllowPrivateAccess = "true"))
	FGameplayTagContainer BlockedTargetTags;
};

/** Immutable proof that one exact action commit opened an evasion window. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenSpiritEvasionWindowReceipt
{
	GENERATED_BODY()

public:
	bool IsValid() const;
	const FGuid& GetReceiptId() const { return ReceiptId; }
	const FGuid& GetWindowId() const { return WindowId; }
	const FShanmenCombatActionSnapshot& GetAction() const { return Action; }
	const FShanmenSpiritEvasionDefinition& GetDefinition() const { return Definition; }
	const FShanmenActionTransitionReceipt& GetCommitTransition() const { return CommitTransition; }

private:
	friend class FShanmenSpiritEvasionWindow;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritEvasion", meta = (AllowPrivateAccess = "true"))
	FGuid ReceiptId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritEvasion", meta = (AllowPrivateAccess = "true"))
	FGuid WindowId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritEvasion", meta = (AllowPrivateAccess = "true"))
	FShanmenCombatActionSnapshot Action;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritEvasion", meta = (AllowPrivateAccess = "true"))
	FShanmenSpiritEvasionDefinition Definition;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritEvasion", meta = (AllowPrivateAccess = "true"))
	FShanmenActionTransitionReceipt CommitTransition;
};

/** Immutable projection of an open evasion window into CombatCore. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenSpiritEvasionProjectionReceipt
{
	GENERATED_BODY()

public:
	bool IsValid() const;
	const FGuid& GetProjectionId() const { return ProjectionId; }
	const FShanmenSpiritEvasionWindowReceipt& GetWindow() const { return Window; }
	const FShanmenDefenseLayer& GetLayer() const { return Layer; }

private:
	friend class FShanmenSpiritEvasionWindow;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritEvasion", meta = (AllowPrivateAccess = "true"))
	FGuid ProjectionId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritEvasion", meta = (AllowPrivateAccess = "true"))
	FShanmenSpiritEvasionWindowReceipt Window;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritEvasion", meta = (AllowPrivateAccess = "true"))
	FShanmenDefenseLayer Layer;
};

/**
 * Pure active-defense view over the existing action lifecycle.
 *
 * The action orchestrator remains the only window lifetime authority. This
 * object owns no clock, movement, collision, timer, resource balance or RNG;
 * it can project Evade only while the exact committed action is still Active.
 */
class SHANMENCOMBATRUNTIME_API FShanmenSpiritEvasionWindow
{
public:
	static bool TryOpen(
		const FShanmenCombatActionSnapshot& Action,
		const FShanmenSpiritEvasionDefinition& Definition,
		const FShanmenActionTransitionReceipt& CommitTransition,
		const FShanmenActionOrchestrator& ActionRuntime,
		FShanmenSpiritEvasionWindow& OutWindow,
		FShanmenSpiritEvasionWindowReceipt& OutReceipt);

	bool IsValid() const;
	bool TryProjectDefenseLayer(
		const FShanmenActionOrchestrator& ActionRuntime,
		FShanmenSpiritEvasionProjectionReceipt& OutReceipt) const;
	void Reset();

	const FShanmenSpiritEvasionWindowReceipt& GetOpenReceipt() const
	{
		return OpenReceipt;
	}

private:
	FShanmenSpiritEvasionWindowReceipt OpenReceipt;
	bool bOpen = false;
};
