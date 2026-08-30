#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ShanmenActionOrchestrator.h"

#include "ShanmenSpiritShield.generated.h"

class FShanmenSpiritShieldDeadlineGate;

/** Explicit lifetime for one short-lived spirit shield. */
UENUM(BlueprintType)
enum class EShanmenSpiritShieldState : uint8
{
	Uninitialized,
	Prepared,
	Active,
	Deactivated
};

/** Why the external product owner ended the shield. */
UENUM(BlueprintType)
enum class EShanmenSpiritShieldDeactivationReason : uint8
{
	None,
	Explicit,
	DurationElapsed,
	Interrupted,
	OwnerEnded
};

/** Mutable content input for the first short active spirit-shield action. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenSpiritShieldDefinitionCapture
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|SpiritShield")
	FName ActionDefinitionId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|SpiritShield")
	FName RuleId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|SpiritShield", meta = (ClampMin = "0.0"))
	float MaximumCapacity = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|SpiritShield")
	FGameplayTagContainer RequiredDamageTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|SpiritShield")
	FGameplayTagContainer BlockedDamageTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|SpiritShield")
	FGameplayTagContainer RequiredSourceTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|SpiritShield")
	FGameplayTagContainer BlockedSourceTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|SpiritShield")
	FGameplayTagContainer RequiredTargetTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|SpiritShield")
	FGameplayTagContainer BlockedTargetTags;
};

/** Immutable authored rules for one spirit-shield activation. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenSpiritShieldDefinition
{
	GENERATED_BODY()

public:
	static FName CanonicalActionDefinitionId();
	static bool TryCapture(
		const FShanmenSpiritShieldDefinitionCapture& Capture,
		FShanmenSpiritShieldDefinition& OutDefinition);

	bool IsValid() const;
	FName GetActionDefinitionId() const { return ActionDefinitionId; }
	FName GetRuleId() const { return RuleId; }
	float GetMaximumCapacity() const { return MaximumCapacity; }
	const FGameplayTagContainer& GetRequiredDamageTags() const { return RequiredDamageTags; }
	const FGameplayTagContainer& GetBlockedDamageTags() const { return BlockedDamageTags; }
	const FGameplayTagContainer& GetRequiredSourceTags() const { return RequiredSourceTags; }
	const FGameplayTagContainer& GetBlockedSourceTags() const { return BlockedSourceTags; }
	const FGameplayTagContainer& GetRequiredTargetTags() const { return RequiredTargetTags; }
	const FGameplayTagContainer& GetBlockedTargetTags() const { return BlockedTargetTags; }

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield", meta = (AllowPrivateAccess = "true"))
	FName ActionDefinitionId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield", meta = (AllowPrivateAccess = "true"))
	FName RuleId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield", meta = (AllowPrivateAccess = "true"))
	float MaximumCapacity = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield", meta = (AllowPrivateAccess = "true"))
	FGameplayTagContainer RequiredDamageTags;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield", meta = (AllowPrivateAccess = "true"))
	FGameplayTagContainer BlockedDamageTags;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield", meta = (AllowPrivateAccess = "true"))
	FGameplayTagContainer RequiredSourceTags;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield", meta = (AllowPrivateAccess = "true"))
	FGameplayTagContainer BlockedSourceTags;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield", meta = (AllowPrivateAccess = "true"))
	FGameplayTagContainer RequiredTargetTags;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield", meta = (AllowPrivateAccess = "true"))
	FGameplayTagContainer BlockedTargetTags;
};

/** Immutable proof that one exact committed action activated a shield. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenSpiritShieldActivationReceipt
{
	GENERATED_BODY()

public:
	bool IsValid() const;
	const FGuid& GetReceiptId() const { return ReceiptId; }
	const FGuid& GetShieldInstanceId() const { return ShieldInstanceId; }
	const FShanmenCombatActionSnapshot& GetAction() const { return Action; }
	const FShanmenSpiritShieldDefinition& GetDefinition() const { return Definition; }

private:
	friend class FShanmenSpiritShieldRuntime;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield", meta = (AllowPrivateAccess = "true"))
	FGuid ReceiptId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield", meta = (AllowPrivateAccess = "true"))
	FGuid ShieldInstanceId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield", meta = (AllowPrivateAccess = "true"))
	FShanmenCombatActionSnapshot Action;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield", meta = (AllowPrivateAccess = "true"))
	FShanmenSpiritShieldDefinition Definition;
};

/** Immutable projection of one sampled shield-capacity revision into CombatCore. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenSpiritShieldProjectionReceipt
{
	GENERATED_BODY()

public:
	bool IsValid() const;
	const FGuid& GetProjectionId() const { return ProjectionId; }
	const FShanmenSpiritShieldActivationReceipt& GetActivation() const { return Activation; }
	int64 GetAuthorityRevision() const { return AuthorityRevision; }
	float GetAvailableCapacity() const { return AvailableCapacity; }
	const FShanmenDefenseLayer& GetLayer() const { return Layer; }

private:
	friend class FShanmenSpiritShieldRuntime;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield", meta = (AllowPrivateAccess = "true"))
	FGuid ProjectionId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield", meta = (AllowPrivateAccess = "true"))
	FShanmenSpiritShieldActivationReceipt Activation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield", meta = (AllowPrivateAccess = "true"))
	int64 AuthorityRevision = INDEX_NONE;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield", meta = (AllowPrivateAccess = "true"))
	float AvailableCapacity = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield", meta = (AllowPrivateAccess = "true"))
	FShanmenDefenseLayer Layer;
};

/** Immutable proof that an external owner explicitly ended the shield. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenSpiritShieldDeactivationReceipt
{
	GENERATED_BODY()

public:
	bool IsValid() const;
	const FGuid& GetReceiptId() const { return ReceiptId; }
	const FShanmenSpiritShieldActivationReceipt& GetActivation() const { return Activation; }
	EShanmenSpiritShieldDeactivationReason GetReason() const { return Reason; }

private:
	friend class FShanmenSpiritShieldRuntime;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield", meta = (AllowPrivateAccess = "true"))
	FGuid ReceiptId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield", meta = (AllowPrivateAccess = "true"))
	FShanmenSpiritShieldActivationReceipt Activation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SpiritShield", meta = (AllowPrivateAccess = "true"))
	EShanmenSpiritShieldDeactivationReason Reason = EShanmenSpiritShieldDeactivationReason::None;
};

/**
 * Pure lifetime and projection authority for one short spirit shield.
 *
 * Product code owns time, input, energy and capacity commits. This runtime only
 * accepts explicit lifecycle commands and projects a revisioned defense layer.
 */
class SHANMENCOMBATRUNTIME_API FShanmenSpiritShieldRuntime
{
public:
	static bool TryPrepare(
		const FShanmenCombatActionSnapshot& Action,
		const FShanmenSpiritShieldDefinition& Definition,
		FShanmenSpiritShieldRuntime& OutRuntime);

	bool IsValid() const;
	EShanmenSpiritShieldState GetState() const { return State; }
	const FShanmenCombatActionSnapshot& GetAction() const { return Action; }
	const FShanmenSpiritShieldDefinition& GetDefinition() const { return Definition; }
	const FShanmenSpiritShieldActivationReceipt& GetActivationReceipt() const { return ActivationReceipt; }
	const FShanmenSpiritShieldProjectionReceipt& GetLastProjectionReceipt() const { return LastProjectionReceipt; }
	const FShanmenSpiritShieldDeactivationReceipt& GetDeactivationReceipt() const { return DeactivationReceipt; }

	bool TryActivate(
		const FShanmenActionOrchestrator& ActionRuntime,
		FShanmenSpiritShieldActivationReceipt& OutReceipt);
	bool TryProjectDefenseLayer(
		int64 AuthorityRevision,
		float AvailableCapacity,
		FShanmenSpiritShieldProjectionReceipt& OutReceipt);
	bool TryDeactivate(
		const FGuid& ExpectedShieldInstanceId,
		EShanmenSpiritShieldDeactivationReason Reason,
		FShanmenSpiritShieldDeactivationReceipt& OutReceipt);
	void Reset();

private:
	friend class FShanmenSpiritShieldDeadlineGate;

	bool TryDeactivateForDeadline(
		const FGuid& ExpectedShieldInstanceId,
		FShanmenSpiritShieldDeactivationReceipt& OutReceipt);
	bool TryDeactivateInternal(
		const FGuid& ExpectedShieldInstanceId,
		EShanmenSpiritShieldDeactivationReason Reason,
		FShanmenSpiritShieldDeactivationReceipt& OutReceipt);
	bool MatchesActionRuntime(const FShanmenActionOrchestrator& ActionRuntime) const;

	FShanmenCombatActionSnapshot Action;
	FShanmenSpiritShieldDefinition Definition;
	FShanmenSpiritShieldActivationReceipt ActivationReceipt;
	FShanmenSpiritShieldProjectionReceipt LastProjectionReceipt;
	FShanmenSpiritShieldDeactivationReceipt DeactivationReceipt;
	EShanmenSpiritShieldState State = EShanmenSpiritShieldState::Uninitialized;
};
