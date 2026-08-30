#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ShanmenActionOrchestrator.h"

#include "ShanmenWeaponGuard.generated.h"

/** Mutable authoring input for one action-bound ordinary weapon guard. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenWeaponGuardDefinitionCapture
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|WeaponGuard")
	FName ActionDefinitionId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|WeaponGuard")
	FName RuleId = NAME_None;

	/** Authored fraction prevented by ordinary guard; balance remains content-owned. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|WeaponGuard", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float GuardFraction = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|WeaponGuard")
	FGameplayTagContainer RequiredDamageTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|WeaponGuard")
	FGameplayTagContainer BlockedDamageTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|WeaponGuard")
	FGameplayTagContainer RequiredSourceTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|WeaponGuard")
	FGameplayTagContainer BlockedSourceTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|WeaponGuard")
	FGameplayTagContainer RequiredTargetTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|WeaponGuard")
	FGameplayTagContainer BlockedTargetTags;
};

/** Immutable ordinary-guard coverage; perfect timing and resource policy remain external. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenWeaponGuardDefinition
{
	GENERATED_BODY()

public:
	static FName CanonicalActionDefinitionId();
	static bool TryCapture(
		const FShanmenWeaponGuardDefinitionCapture& Capture,
		FShanmenWeaponGuardDefinition& OutDefinition);

	bool IsValid() const;
	FName GetActionDefinitionId() const { return ActionDefinitionId; }
	FName GetRuleId() const { return RuleId; }
	float GetGuardFraction() const { return GuardFraction; }
	const FGameplayTagContainer& GetRequiredDamageTags() const { return RequiredDamageTags; }
	const FGameplayTagContainer& GetBlockedDamageTags() const { return BlockedDamageTags; }
	const FGameplayTagContainer& GetRequiredSourceTags() const { return RequiredSourceTags; }
	const FGameplayTagContainer& GetBlockedSourceTags() const { return BlockedSourceTags; }
	const FGameplayTagContainer& GetRequiredTargetTags() const { return RequiredTargetTags; }
	const FGameplayTagContainer& GetBlockedTargetTags() const { return BlockedTargetTags; }

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|WeaponGuard", meta = (AllowPrivateAccess = "true"))
	FName ActionDefinitionId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|WeaponGuard", meta = (AllowPrivateAccess = "true"))
	FName RuleId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|WeaponGuard", meta = (AllowPrivateAccess = "true"))
	float GuardFraction = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|WeaponGuard", meta = (AllowPrivateAccess = "true"))
	FGameplayTagContainer RequiredDamageTags;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|WeaponGuard", meta = (AllowPrivateAccess = "true"))
	FGameplayTagContainer BlockedDamageTags;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|WeaponGuard", meta = (AllowPrivateAccess = "true"))
	FGameplayTagContainer RequiredSourceTags;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|WeaponGuard", meta = (AllowPrivateAccess = "true"))
	FGameplayTagContainer BlockedSourceTags;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|WeaponGuard", meta = (AllowPrivateAccess = "true"))
	FGameplayTagContainer RequiredTargetTags;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|WeaponGuard", meta = (AllowPrivateAccess = "true"))
	FGameplayTagContainer BlockedTargetTags;
};

/** Immutable proof that one exact weapon action commit opened a guard window. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenWeaponGuardWindowReceipt
{
	GENERATED_BODY()

public:
	bool IsValid() const;
	const FGuid& GetReceiptId() const { return ReceiptId; }
	const FGuid& GetWindowId() const { return WindowId; }
	const FShanmenCombatActionSnapshot& GetAction() const { return Action; }
	const FShanmenWeaponGuardDefinition& GetDefinition() const { return Definition; }
	const FShanmenActionTransitionReceipt& GetCommitTransition() const { return CommitTransition; }

private:
	friend class FShanmenWeaponGuardWindow;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|WeaponGuard", meta = (AllowPrivateAccess = "true"))
	FGuid ReceiptId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|WeaponGuard", meta = (AllowPrivateAccess = "true"))
	FGuid WindowId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|WeaponGuard", meta = (AllowPrivateAccess = "true"))
	FShanmenCombatActionSnapshot Action;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|WeaponGuard", meta = (AllowPrivateAccess = "true"))
	FShanmenWeaponGuardDefinition Definition;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|WeaponGuard", meta = (AllowPrivateAccess = "true"))
	FShanmenActionTransitionReceipt CommitTransition;
};

/** Immutable projection of an active ordinary guard into CombatCore. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenWeaponGuardProjectionReceipt
{
	GENERATED_BODY()

public:
	bool IsValid() const;
	const FGuid& GetProjectionId() const { return ProjectionId; }
	const FShanmenWeaponGuardWindowReceipt& GetWindow() const { return Window; }
	const FShanmenDefenseLayer& GetLayer() const { return Layer; }

private:
	friend class FShanmenWeaponGuardWindow;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|WeaponGuard", meta = (AllowPrivateAccess = "true"))
	FGuid ProjectionId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|WeaponGuard", meta = (AllowPrivateAccess = "true"))
	FShanmenWeaponGuardWindowReceipt Window;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|WeaponGuard", meta = (AllowPrivateAccess = "true"))
	FShanmenDefenseLayer Layer;
};

/**
 * Pure ordinary-guard view over the existing action lifecycle.
 *
 * The action orchestrator is the only window lifetime authority. This object
 * owns no clock, input, resource balance, durability transaction or perfect-
 * guard timing; it projects Guard only while the exact weapon action is Active.
 */
class SHANMENCOMBATRUNTIME_API FShanmenWeaponGuardWindow
{
public:
	static bool TryOpen(
		const FShanmenCombatActionSnapshot& Action,
		const FShanmenWeaponGuardDefinition& Definition,
		const FShanmenActionTransitionReceipt& CommitTransition,
		const FShanmenActionOrchestrator& ActionRuntime,
		FShanmenWeaponGuardWindow& OutWindow,
		FShanmenWeaponGuardWindowReceipt& OutReceipt);

	bool IsValid() const;
	bool IsActiveFor(const FShanmenActionOrchestrator& ActionRuntime) const;
	bool TryProjectDefenseLayer(
		const FShanmenActionOrchestrator& ActionRuntime,
		FShanmenWeaponGuardProjectionReceipt& OutReceipt) const;
	void Reset();

	const FShanmenWeaponGuardWindowReceipt& GetOpenReceipt() const
	{
		return OpenReceipt;
	}

private:
	FShanmenWeaponGuardWindowReceipt OpenReceipt;
	bool bOpen = false;
};
