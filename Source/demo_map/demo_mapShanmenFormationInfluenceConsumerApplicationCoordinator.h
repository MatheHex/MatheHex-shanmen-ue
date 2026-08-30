#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenFormationInfluenceConsumerAttributeAdapter.h"

class Udemo_mapAttributeComponent;

/** Atomic evidence that registry and native attribute state accepted one command. */
struct Fdemo_mapShanmenFormationInfluenceConsumerTransactionReceipt
{
public:
	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenFormationInfluenceConsumerTransactionReceipt& Other)
		const;

	const FGuid& GetReceiptId() const { return ReceiptId; }
	const FGuid& GetCoordinatorId() const { return CoordinatorId; }
	const FGuid& GetSubjectEntityId() const { return SubjectEntityId; }
	const Fdemo_mapShanmenFormationInfluenceConsumerApplicationReceipt&
	GetRegistryReceipt() const
	{
		return RegistryReceipt;
	}
	const Fdemo_mapShanmenFormationInfluenceConsumerAttributeAcknowledgement&
	GetAttributeAcknowledgement() const
	{
		return AttributeAcknowledgement;
	}

private:
	FGuid ReceiptId;
	FGuid CoordinatorId;
	FGuid SubjectEntityId;
	Fdemo_mapShanmenFormationInfluenceConsumerApplicationReceipt RegistryReceipt;
	Fdemo_mapShanmenFormationInfluenceConsumerAttributeAcknowledgement
		AttributeAcknowledgement;

	friend class
		Fdemo_mapShanmenFormationInfluenceConsumerApplicationCoordinator;
};

enum class Edemo_mapShanmenFormationInfluenceConsumerTransactionStatus : uint8
{
	Applied,
	Removed,
	TransactionReplayed,
	CoordinatorInvalid,
	CommandInvalid,
	TargetMismatch,
	ComponentUnavailable,
	RegistryRejected,
	NativeRejected,
	CommitRejected
};

/** One caller-driven atomic transaction across registry and native authority. */
struct Fdemo_mapShanmenFormationInfluenceConsumerTransactionResult
{
	Edemo_mapShanmenFormationInfluenceConsumerTransactionStatus Status =
		Edemo_mapShanmenFormationInfluenceConsumerTransactionStatus::
			CoordinatorInvalid;
	FString Diagnostic;
	bool bCoordinatorStateCommitted = false;
	bool bTransactionReplayed = false;
	Fdemo_mapShanmenFormationInfluenceConsumerApplicationResult Registry;
	Fdemo_mapShanmenFormationInfluenceConsumerAttributeResult Native;
	Fdemo_mapShanmenFormationInfluenceConsumerTransactionReceipt Receipt;

	bool IsSuccess() const;
};

/**
 * Component-bound atomic coordinator for one subject's consumer applications.
 *
 * Commands execute against a candidate registry. The candidate is committed
 * only after the existing attribute authority returns a valid acknowledgement.
 * No pending queue or mirrored active-modifier table is stored. Completed
 * command replay returns its immutable transaction receipt without reapplying
 * a superseded historical Apply.
 */
class Fdemo_mapShanmenFormationInfluenceConsumerApplicationCoordinator
{
public:
	static bool TryCreate(
		const FGuid& RunId,
		const FShanmenContentStamp& Content,
		const FGuid& SubjectEntityId,
		Udemo_mapAttributeComponent* AttributeComponent,
		Fdemo_mapShanmenFormationInfluenceConsumerApplicationCoordinator&
			OutCoordinator);

	Fdemo_mapShanmenFormationInfluenceConsumerTransactionResult Execute(
		const Fdemo_mapShanmenFormationInfluenceConsumerCommand& Command);

	bool IsValid() const { return IsConsistent(); }
	bool IsConsistent() const;
	bool IsDrained() const
	{
		return IsConsistent() && Registry.IsDrained();
	}
	bool HasLiveAttributeComponent() const
	{
		return BoundAttributeComponent.IsValid();
	}
	bool MatchesBinding(
		const FGuid& RequestedSubjectEntityId,
		const Udemo_mapAttributeComponent* RequestedComponent) const;
	const FGuid& GetCoordinatorId() const { return CoordinatorId; }
	const FGuid& GetSubjectEntityId() const { return SubjectEntityId; }
	int32 GetActiveApplicationCount() const
	{
		return Registry.GetActiveApplicationCount();
	}
	int32 GetCompletedTransactionCount() const
	{
		return CompletedTransactions.Num();
	}
	const Fdemo_mapShanmenFormationInfluenceConsumerApplicationRegistry&
	GetRegistry() const
	{
		return Registry;
	}
	bool TryGetCompletedResult(
		const FGuid& CommandId,
		Fdemo_mapShanmenFormationInfluenceConsumerTransactionResult& OutResult)
		const;

private:
	struct FCompletedTransactionRecord
	{
		Fdemo_mapShanmenFormationInfluenceConsumerCommand Command;
		Fdemo_mapShanmenFormationInfluenceConsumerTransactionResult Result;
	};

	const FCompletedTransactionRecord* FindCompletedTransaction(
		const FGuid& CommandId) const;

	FGuid CoordinatorId;
	FGuid SubjectEntityId;
	TWeakObjectPtr<Udemo_mapAttributeComponent> BoundAttributeComponent;
	Fdemo_mapShanmenFormationInfluenceConsumerApplicationRegistry Registry;
	TArray<FCompletedTransactionRecord> CompletedTransactions;
};
