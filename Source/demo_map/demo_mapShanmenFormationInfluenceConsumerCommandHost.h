#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenFormationInfluenceConsumerApplicationCoordinator.h"

class Udemo_mapAttributeComponent;

/** Immutable evidence that one subject was bound to this run-scoped Host. */
struct Fdemo_mapShanmenFormationInfluenceConsumerBindingReceipt
{
public:
	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenFormationInfluenceConsumerBindingReceipt& Other)
		const;

	const FGuid& GetBindingId() const { return BindingId; }
	const FGuid& GetHostId() const { return HostId; }
	const FGuid& GetSubjectEntityId() const { return SubjectEntityId; }
	const FGuid& GetCoordinatorId() const { return CoordinatorId; }

private:
	FGuid BindingId;
	FGuid HostId;
	FGuid SubjectEntityId;
	FGuid CoordinatorId;

	friend class Fdemo_mapShanmenFormationInfluenceConsumerCommandHost;
};

enum class Edemo_mapShanmenFormationInfluenceConsumerBindingStatus : uint8
{
	Bound,
	BindingReplayed,
	HostInvalid,
	SubjectInvalid,
	ComponentUnavailable,
	SubjectBindingConflict,
	ComponentBindingConflict,
	StateInvalid
};

/** Result of one explicit, append-only subject/component bind request. */
struct Fdemo_mapShanmenFormationInfluenceConsumerBindingResult
{
	Edemo_mapShanmenFormationInfluenceConsumerBindingStatus Status =
		Edemo_mapShanmenFormationInfluenceConsumerBindingStatus::HostInvalid;
	FString Diagnostic;
	bool bHostStateCommitted = false;
	Fdemo_mapShanmenFormationInfluenceConsumerBindingReceipt Receipt;

	bool IsSuccess() const;
};

enum class Edemo_mapShanmenFormationInfluenceConsumerRouteStatus : uint8
{
	Routed,
	TransactionReplayed,
	HostInvalid,
	CommandInvalid,
	ScopeMismatch,
	SubjectUnbound,
	ComponentUnavailable,
	TransactionRejected,
	StateInvalid
};

/**
 * One routed command result. The nested coordinator transaction remains the
 * sole application/native acknowledgement and replay authority.
 */
struct Fdemo_mapShanmenFormationInfluenceConsumerRouteResult
{
	Edemo_mapShanmenFormationInfluenceConsumerRouteStatus Status =
		Edemo_mapShanmenFormationInfluenceConsumerRouteStatus::HostInvalid;
	FString Diagnostic;
	bool bHostStateChanged = false;
	Fdemo_mapShanmenFormationInfluenceConsumerBindingReceipt Binding;
	Fdemo_mapShanmenFormationInfluenceConsumerTransactionResult Transaction;

	bool IsSuccess() const;
};

/**
 * Run-scoped resolver for exact subject/component consumer bindings.
 *
 * Bindings are append-only for the Host lifetime, including after a subject's
 * coordinator drains. That durable identity fence prevents a completed Apply
 * command from becoming new work after component replacement. The Host owns
 * no duplicate application table, native modifier state, retry queue,
 * scheduled work, persistence, Actor, or World authority.
 */
class Fdemo_mapShanmenFormationInfluenceConsumerCommandHost
{
public:
	static bool TryOpen(
		const FGuid& RunId,
		const FShanmenContentStamp& Content,
		Fdemo_mapShanmenFormationInfluenceConsumerCommandHost& OutHost);

	Fdemo_mapShanmenFormationInfluenceConsumerBindingResult TryBindSubject(
		const FGuid& SubjectEntityId,
		Udemo_mapAttributeComponent* AttributeComponent);
	Fdemo_mapShanmenFormationInfluenceConsumerRouteResult TryRoute(
		const Fdemo_mapShanmenFormationInfluenceConsumerCommand& Command);

	bool TryGetBindingReceipt(
		const FGuid& SubjectEntityId,
		Fdemo_mapShanmenFormationInfluenceConsumerBindingReceipt& OutReceipt)
		const;
	bool HasBinding(const FGuid& SubjectEntityId) const;
	bool HasLiveBinding(const FGuid& SubjectEntityId) const;
	bool IsValid() const { return IsConsistent(); }
	bool IsConsistent() const;
	bool IsDrained() const;
	int32 GetBindingCount() const { return Bindings.Num(); }
	int32 GetActiveApplicationCount() const;
	int32 GetCompletedTransactionCount() const;
	const FGuid& GetHostId() const { return HostId; }
	const FGuid& GetRunId() const { return RunId; }
	const FShanmenContentStamp& GetContent() const { return Content; }

private:
	struct FBindingRecord
	{
		Fdemo_mapShanmenFormationInfluenceConsumerBindingReceipt Receipt;
		Fdemo_mapShanmenFormationInfluenceConsumerApplicationCoordinator
			Coordinator;
	};

	FBindingRecord* FindBinding(const FGuid& SubjectEntityId);
	const FBindingRecord* FindBinding(const FGuid& SubjectEntityId) const;

	FGuid HostId;
	FGuid RunId;
	FShanmenContentStamp Content;
	TArray<FBindingRecord> Bindings;
};
