#pragma once

#include "CoreMinimal.h"
#include "ShanmenActionResourceAuthority.h"
#include "ShanmenActionOrchestrator.h"

#include "ShanmenFormationDeployment.generated.h"

/** Pure deployment lifecycle; product Actors and effects remain external. */
UENUM(BlueprintType)
enum class EShanmenFormationDeploymentState : uint8
{
	Uninitialized,
	Planned,
	Deploying,
	Active,
	Cancelled,
	Ended
};

UENUM(BlueprintType)
enum class EShanmenFormationDeploymentEventKind : uint8
{
	Begin,
	CommitAnchor,
	Cancel,
	End
};

/** Mutable authoring input for one material line at one formation anchor. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenFormationMaterialRequirementCapture
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|Formation")
	int32 Order = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|Formation")
	FName MaterialDefinitionId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|Formation", meta = (ClampMin = "1"))
	int32 Quantity = 0;
};

/** Mutable authoring input for one explicitly ordered formation anchor. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenFormationAnchorCapture
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|Formation")
	int32 Order = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|Formation")
	FName AnchorDefinitionId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|Formation")
	FVector RelativeOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|Formation")
	TArray<FShanmenFormationMaterialRequirementCapture> Requirements;
};

/** Mutable authoring input; successful capture produces an immutable diagram. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenFormationDiagramCapture
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|Formation")
	FName ActionDefinitionId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|Formation")
	FName DiagramDefinitionId = NAME_None;

	/**
	 * Authored activation demand on the existing shared SpiritEnergy channel.
	 * The amount and rule identity remain content-owned.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|Formation")
	FShanmenActionResourceCostCapture ActivationEnergyCost;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|Formation")
	TArray<FShanmenFormationAnchorCapture> Anchors;
};

/** Frozen exact material requirement; arrays extend without fixed-field rewrites. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenFormationMaterialRequirement
{
	GENERATED_BODY()

public:
	bool IsValid() const;
	int32 GetOrder() const { return Order; }
	FName GetMaterialDefinitionId() const { return MaterialDefinitionId; }
	int32 GetQuantity() const { return Quantity; }

private:
	friend struct FShanmenFormationDiagramDefinition;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Formation", meta = (AllowPrivateAccess = "true"))
	int32 Order = INDEX_NONE;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Formation", meta = (AllowPrivateAccess = "true"))
	FName MaterialDefinitionId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Formation", meta = (AllowPrivateAccess = "true"))
	int32 Quantity = 0;
};

/** Frozen anchor geometry and ordered material requirements. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenFormationAnchorDefinition
{
	GENERATED_BODY()

public:
	bool IsValid() const;
	int32 GetOrder() const { return Order; }
	FName GetAnchorDefinitionId() const { return AnchorDefinitionId; }
	const FVector& GetRelativeOffset() const { return RelativeOffset; }
	const TArray<FShanmenFormationMaterialRequirement>& GetRequirements() const
	{
		return Requirements;
	}

private:
	friend struct FShanmenFormationDiagramDefinition;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Formation", meta = (AllowPrivateAccess = "true"))
	int32 Order = INDEX_NONE;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Formation", meta = (AllowPrivateAccess = "true"))
	FName AnchorDefinitionId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Formation", meta = (AllowPrivateAccess = "true"))
	FVector RelativeOffset = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Formation", meta = (AllowPrivateAccess = "true"))
	TArray<FShanmenFormationMaterialRequirement> Requirements;
};

/** Immutable knowledge contract for one formation diagram. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenFormationDiagramDefinition
{
	GENERATED_BODY()

public:
	static FName CanonicalActionDefinitionId();
	static FGameplayTag CanonicalActivationEnergyChannel();
	static bool TryCapture(
		const FShanmenFormationDiagramCapture& Capture,
		FShanmenFormationDiagramDefinition& OutDefinition);

	bool IsValid() const;
	FName GetActionDefinitionId() const { return ActionDefinitionId; }
	FName GetDiagramDefinitionId() const { return DiagramDefinitionId; }
	const FShanmenActionResourceCost& GetActivationEnergyCost() const
	{
		return ActivationEnergyCost;
	}
	const TArray<FShanmenFormationAnchorDefinition>& GetAnchors() const
	{
		return Anchors;
	}
	const FShanmenFormationAnchorDefinition* FindAnchor(
		FName AnchorDefinitionId) const;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Formation", meta = (AllowPrivateAccess = "true"))
	FName ActionDefinitionId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Formation", meta = (AllowPrivateAccess = "true"))
	FName DiagramDefinitionId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Formation", meta = (AllowPrivateAccess = "true"))
	FShanmenActionResourceCost ActivationEnergyCost;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Formation", meta = (AllowPrivateAccess = "true"))
	TArray<FShanmenFormationAnchorDefinition> Anchors;
};

/** One exact physical item contribution asserted by a later Items adapter. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenFormationMaterialFulfillmentLine
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|Formation")
	FGuid ItemInstanceId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|Formation")
	FName MaterialDefinitionId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|Formation", meta = (ClampMin = "1"))
	int32 Quantity = 0;

	bool IsValid() const;
};

/**
 * Authority-independent envelope for an already committed material decision.
 * P8.0 validates exact shape only; a later ShanmenItems adapter must produce it.
 */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenFormationAnchorFulfillmentEvidence
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|Formation")
	FGuid FulfillmentId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|Formation")
	FGuid RunId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|Formation")
	FGuid OwnerId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|Formation")
	FGuid DeploymentId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|Formation")
	FName AnchorDefinitionId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|Formation")
	FShanmenContentStamp Content;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|Formation", meta = (ClampMin = "0"))
	int32 AuthorityRevision = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shanmen|Combat|Formation")
	TArray<FShanmenFormationMaterialFulfillmentLine> Lines;

	bool IsValid() const;
};

/** Read-only progress for one exact deployed anchor instance. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenFormationAnchorProgress
{
	GENERATED_BODY()

public:
	FName GetAnchorDefinitionId() const { return AnchorDefinitionId; }
	const FGuid& GetAnchorInstanceId() const { return AnchorInstanceId; }
	const FVector& GetWorldLocation() const { return WorldLocation; }
	bool IsCommitted() const { return bCommitted; }
	const FShanmenFormationAnchorFulfillmentEvidence& GetFulfillment() const
	{
		return Fulfillment;
	}

private:
	friend class FShanmenFormationDeployment;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Formation", meta = (AllowPrivateAccess = "true"))
	FName AnchorDefinitionId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Formation", meta = (AllowPrivateAccess = "true"))
	FGuid AnchorInstanceId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Formation", meta = (AllowPrivateAccess = "true"))
	FVector WorldLocation = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Formation", meta = (AllowPrivateAccess = "true"))
	bool bCommitted = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Formation", meta = (AllowPrivateAccess = "true"))
	FShanmenFormationAnchorFulfillmentEvidence Fulfillment;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Formation", meta = (AllowPrivateAccess = "true"))
	FGuid CommitReceiptId;
};

/** Immutable receipt for one accepted deployment event. */
USTRUCT(BlueprintType)
struct SHANMENCOMBATRUNTIME_API FShanmenFormationDeploymentReceipt
{
	GENERATED_BODY()

public:
	bool IsValid() const;
	const FGuid& GetReceiptId() const { return ReceiptId; }
	const FGuid& GetDeploymentId() const { return DeploymentId; }
	int32 GetSequence() const { return Sequence; }
	EShanmenFormationDeploymentEventKind GetEventKind() const
	{
		return EventKind;
	}
	EShanmenFormationDeploymentState GetStateBefore() const
	{
		return StateBefore;
	}
	EShanmenFormationDeploymentState GetStateAfter() const
	{
		return StateAfter;
	}
	FName GetAnchorDefinitionId() const { return AnchorDefinitionId; }
	const FGuid& GetFulfillmentId() const { return FulfillmentId; }
	int32 GetAuthorityRevision() const { return AuthorityRevision; }
	int32 GetCommittedAnchorCount() const { return CommittedAnchorCount; }

private:
	friend class FShanmenFormationDeployment;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Formation", meta = (AllowPrivateAccess = "true"))
	FGuid ReceiptId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Formation", meta = (AllowPrivateAccess = "true"))
	FGuid DeploymentId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Formation", meta = (AllowPrivateAccess = "true"))
	int32 Sequence = INDEX_NONE;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Formation", meta = (AllowPrivateAccess = "true"))
	EShanmenFormationDeploymentEventKind EventKind =
		EShanmenFormationDeploymentEventKind::Begin;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Formation", meta = (AllowPrivateAccess = "true"))
	EShanmenFormationDeploymentState StateBefore =
		EShanmenFormationDeploymentState::Uninitialized;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Formation", meta = (AllowPrivateAccess = "true"))
	EShanmenFormationDeploymentState StateAfter =
		EShanmenFormationDeploymentState::Uninitialized;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Formation", meta = (AllowPrivateAccess = "true"))
	FName AnchorDefinitionId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Formation", meta = (AllowPrivateAccess = "true"))
	FGuid FulfillmentId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Formation", meta = (AllowPrivateAccess = "true"))
	int32 AuthorityRevision = INDEX_NONE;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|Formation", meta = (AllowPrivateAccess = "true"))
	int32 CommittedAnchorCount = 0;
};

/**
 * Pure deterministic deployment kernel for one formation activation.
 *
 * It owns diagram geometry and progress only. Item reserve/commit, world Actor
 * placement, interaction, cadence, effects, UI, and authored recipes remain
 * external adapters or content.
 */
class SHANMENCOMBATRUNTIME_API FShanmenFormationDeployment
{
public:
	static bool TryCreate(
		const FShanmenCombatActionSnapshot& Action,
		const FShanmenFormationDiagramDefinition& Diagram,
		const FVector& Origin,
		const FVector& Forward,
		FShanmenFormationDeployment& OutDeployment);

	bool IsValid() const;
	bool TryBeginDeployment(
		const FShanmenActionOrchestrator& ActionRuntime,
		FShanmenFormationDeploymentReceipt& OutReceipt);
	bool TryCommitAnchor(
		const FShanmenActionOrchestrator& ActionRuntime,
		const FShanmenFormationAnchorFulfillmentEvidence& Evidence,
		FShanmenFormationDeploymentReceipt& OutReceipt);
	bool TryCancel(
		const FShanmenActionOrchestrator& ActionRuntime,
		FShanmenFormationDeploymentReceipt& OutReceipt);
	bool TryEnd(
		const FShanmenActionOrchestrator& ActionRuntime,
		FShanmenFormationDeploymentReceipt& OutReceipt);
	void Reset();

	const FGuid& GetDeploymentId() const { return DeploymentId; }
	const FShanmenCombatActionSnapshot& GetAction() const { return Action; }
	const FShanmenFormationDiagramDefinition& GetDiagram() const
	{
		return Diagram;
	}
	const FVector& GetOrigin() const { return Origin; }
	const FVector& GetForward() const { return Forward; }
	EShanmenFormationDeploymentState GetState() const { return State; }
	int32 GetCommittedAnchorCount() const { return CommittedAnchorCount; }
	const TArray<FShanmenFormationAnchorProgress>& GetAnchors() const
	{
		return Anchors;
	}
	const TArray<FShanmenFormationDeploymentReceipt>& GetReceipts() const
	{
		return Receipts;
	}

private:
	bool MatchesActionRuntime(
		const FShanmenActionOrchestrator& ActionRuntime) const;
	const FShanmenFormationDeploymentReceipt* FindReceipt(
		const FGuid& ReceiptId) const;
	FShanmenFormationAnchorProgress* FindAnchorProgress(
		FName AnchorDefinitionId);
	bool ValidateFulfillment(
		const FShanmenFormationAnchorDefinition& Anchor,
		const FShanmenFormationAnchorFulfillmentEvidence& Evidence) const;

	FShanmenCombatActionSnapshot Action;
	FShanmenFormationDiagramDefinition Diagram;
	FGuid DeploymentId;
	FVector Origin = FVector::ZeroVector;
	FVector Forward = FVector::ZeroVector;
	TArray<FShanmenFormationAnchorProgress> Anchors;
	TArray<FShanmenFormationDeploymentReceipt> Receipts;
	EShanmenFormationDeploymentState State =
		EShanmenFormationDeploymentState::Uninitialized;
	int32 CommittedAnchorCount = 0;
	int32 NextReceiptSequence = 0;
};
