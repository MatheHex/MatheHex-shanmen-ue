#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenSwordRhythmEffectCueDelivery.h"

#include "demo_mapShanmenSwordRhythmEffectCueConsumerAttempt.generated.h"

/** Immutable channel projection for one exact consumer delivery. */
USTRUCT(BlueprintType)
struct Fdemo_mapShanmenSwordRhythmEffectCueConsumerRoute
{
	GENERATED_BODY()

public:
	static FName VisualConsumerRoleId();
	static FName AudioConsumerRoleId();
	static bool TryResolveChannel(
		FName ConsumerRoleId,
		Edemo_mapShanmenSwordRhythmEffectCueChannel& OutChannel);
	static bool TryCreate(
		const Fdemo_mapShanmenSwordRhythmEffectCueDeliveryReceipt& Delivery,
		Fdemo_mapShanmenSwordRhythmEffectCueConsumerRoute& OutRoute);

	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenSwordRhythmEffectCueConsumerRoute& Other) const;
	const FGuid& GetRouteId() const { return RouteId; }
	const Fdemo_mapShanmenSwordRhythmEffectCueDeliveryReceipt& GetDelivery()
		const
	{
		return Delivery;
	}
	Edemo_mapShanmenSwordRhythmEffectCueChannel GetChannel() const
	{
		return Channel;
	}
	const TArray<Fdemo_mapShanmenSwordRhythmEffectCueCommand>& GetCommands()
		const
	{
		return Commands;
	}
	int32 NumCommands() const { return Commands.Num(); }

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|EffectCue|Consumer", meta = (AllowPrivateAccess = "true"))
	FGuid RouteId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|EffectCue|Consumer", meta = (AllowPrivateAccess = "true"))
	Fdemo_mapShanmenSwordRhythmEffectCueDeliveryReceipt Delivery;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|EffectCue|Consumer", meta = (AllowPrivateAccess = "true"))
	Edemo_mapShanmenSwordRhythmEffectCueChannel Channel =
		Edemo_mapShanmenSwordRhythmEffectCueChannel::Invalid;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|EffectCue|Consumer", meta = (AllowPrivateAccess = "true"))
	TArray<Fdemo_mapShanmenSwordRhythmEffectCueCommand> Commands;
};

UENUM(BlueprintType)
enum class Edemo_mapShanmenSwordRhythmEffectCueAttemptOutcome : uint8
{
	RetryableFailure,
	Succeeded,
	NoOpSucceeded
};

/** Immutable opaque executor evidence for one route attempt. */
USTRUCT(BlueprintType)
struct Fdemo_mapShanmenSwordRhythmEffectCueAttemptCommand
{
	GENERATED_BODY()

public:
	static bool TryCreate(
		const Fdemo_mapShanmenSwordRhythmEffectCueConsumerRoute& Route,
		const FGuid& AttemptId,
		const FGuid& ExecutorReceiptId,
		Edemo_mapShanmenSwordRhythmEffectCueAttemptOutcome Outcome,
		Fdemo_mapShanmenSwordRhythmEffectCueAttemptCommand& OutCommand);

	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenSwordRhythmEffectCueAttemptCommand& Other) const;
	const FGuid& GetCommandId() const { return CommandId; }
	const Fdemo_mapShanmenSwordRhythmEffectCueConsumerRoute& GetRoute() const
	{
		return Route;
	}
	const FGuid& GetAttemptId() const { return AttemptId; }
	const FGuid& GetExecutorReceiptId() const { return ExecutorReceiptId; }
	Edemo_mapShanmenSwordRhythmEffectCueAttemptOutcome GetOutcome() const
	{
		return Outcome;
	}

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|EffectCue|Consumer", meta = (AllowPrivateAccess = "true"))
	FGuid CommandId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|EffectCue|Consumer", meta = (AllowPrivateAccess = "true"))
	Fdemo_mapShanmenSwordRhythmEffectCueConsumerRoute Route;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|EffectCue|Consumer", meta = (AllowPrivateAccess = "true"))
	FGuid AttemptId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|EffectCue|Consumer", meta = (AllowPrivateAccess = "true"))
	FGuid ExecutorReceiptId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|EffectCue|Consumer", meta = (AllowPrivateAccess = "true"))
	Edemo_mapShanmenSwordRhythmEffectCueAttemptOutcome Outcome =
		Edemo_mapShanmenSwordRhythmEffectCueAttemptOutcome::RetryableFailure;
};

/** Immutable attempt receipt. Only successful receipts contain acknowledgement. */
USTRUCT(BlueprintType)
struct Fdemo_mapShanmenSwordRhythmEffectCueAttemptReceipt
{
	GENERATED_BODY()

public:
	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenSwordRhythmEffectCueAttemptReceipt& Other) const;
	const FGuid& GetReceiptId() const { return ReceiptId; }
	const Fdemo_mapShanmenSwordRhythmEffectCueConsumerRoute& GetRoute() const
	{
		return Route;
	}
	const Fdemo_mapShanmenSwordRhythmEffectCueAttemptCommand& GetCommand() const
	{
		return Command;
	}
	bool HasAcknowledgement() const { return bHasAcknowledgement; }
	const Fdemo_mapShanmenSwordRhythmEffectCueAcknowledgementReceipt&
		GetAcknowledgement() const
	{
		return Acknowledgement;
	}

private:
	friend class Fdemo_mapShanmenSwordRhythmEffectCueConsumerCoordinator;

	static bool TryCreate(
		const Fdemo_mapShanmenSwordRhythmEffectCueConsumerRoute& Route,
		const Fdemo_mapShanmenSwordRhythmEffectCueAttemptCommand& Command,
		const Fdemo_mapShanmenSwordRhythmEffectCueAcknowledgementReceipt*
			Acknowledgement,
		Fdemo_mapShanmenSwordRhythmEffectCueAttemptReceipt& OutReceipt);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|EffectCue|Consumer", meta = (AllowPrivateAccess = "true"))
	FGuid ReceiptId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|EffectCue|Consumer", meta = (AllowPrivateAccess = "true"))
	Fdemo_mapShanmenSwordRhythmEffectCueConsumerRoute Route;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|EffectCue|Consumer", meta = (AllowPrivateAccess = "true"))
	Fdemo_mapShanmenSwordRhythmEffectCueAttemptCommand Command;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|EffectCue|Consumer", meta = (AllowPrivateAccess = "true"))
	bool bHasAcknowledgement = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|EffectCue|Consumer", meta = (AllowPrivateAccess = "true"))
	Fdemo_mapShanmenSwordRhythmEffectCueAcknowledgementReceipt Acknowledgement;
};

enum class Edemo_mapShanmenSwordRhythmEffectCueConsumerPrepareStatus : uint8
{
	Routed,
	AlreadyAcknowledged,
	CoordinatorInvalid,
	EventInvalid,
	RunMismatch,
	StaleEvent,
	RoleUnsupported,
	DeliveryRejected,
	RouteRejected
};

struct Fdemo_mapShanmenSwordRhythmEffectCueConsumerPrepareResult
{
	Edemo_mapShanmenSwordRhythmEffectCueConsumerPrepareStatus Status =
		Edemo_mapShanmenSwordRhythmEffectCueConsumerPrepareStatus::
			CoordinatorInvalid;
	FString Diagnostic;
	Fdemo_mapShanmenSwordRhythmEffectCueConsumerRoute Route;

	bool IsRouted() const
	{
		return Status
				== Edemo_mapShanmenSwordRhythmEffectCueConsumerPrepareStatus::Routed
			&& Route.IsValid();
	}
};

enum class Edemo_mapShanmenSwordRhythmEffectCueConsumerSubmitStatus : uint8
{
	RetryRecorded,
	RetryReplayed,
	Acknowledged,
	AcknowledgementReplayed,
	NoOpAcknowledged,
	NoOpReplayed,
	CoordinatorInvalid,
	RouteInvalid,
	CommandInvalid,
	ScopeMismatch,
	RouteMismatch,
	AttemptConflict,
	AlreadyAcknowledged,
	PendingRoute,
	StaleRoute,
	AcknowledgementRejected,
	StateInvalid
};

struct Fdemo_mapShanmenSwordRhythmEffectCueConsumerSubmitResult
{
	Edemo_mapShanmenSwordRhythmEffectCueConsumerSubmitStatus Status =
		Edemo_mapShanmenSwordRhythmEffectCueConsumerSubmitStatus::
			CoordinatorInvalid;
	FString Diagnostic;
	Fdemo_mapShanmenSwordRhythmEffectCueAttemptReceipt Receipt;

	bool IsSuccess() const;
	bool IsAcknowledged() const;
};

/**
 * Consumer-owned attempt coordinator.
 *
 * It projects only the consumer's canonical channel, records opaque executor
 * outcomes and advances its delivery cursor only after success. It owns no
 * assets, World, UObject, playback, timers, gameplay state or global history.
 */
class Fdemo_mapShanmenSwordRhythmEffectCueConsumerCoordinator
{
public:
	static bool TryCreate(
		const Fdemo_mapShanmenSwordRhythmEffectCueConsumerScope& Scope,
		Fdemo_mapShanmenSwordRhythmEffectCueConsumerCoordinator&
			OutCoordinator);

	bool IsValid() const;
	const Fdemo_mapShanmenSwordRhythmEffectCueConsumerScope& GetScope() const
	{
		return Cursor.GetScope();
	}
	bool IsEmpty() const { return IsValid() && !bHasRouteRecord; }
	bool IsPending() const
	{
		return IsValid() && bHasRouteRecord && !CurrentRoute.bAcknowledged;
	}
	Fdemo_mapShanmenSwordRhythmEffectCueConsumerPrepareResult Prepare(
		const Fdemo_mapShanmenSwordRhythmEffectCueEvent& Event) const;
	Fdemo_mapShanmenSwordRhythmEffectCueConsumerSubmitResult Submit(
		const Fdemo_mapShanmenSwordRhythmEffectCueConsumerRoute& Route,
		const Fdemo_mapShanmenSwordRhythmEffectCueAttemptCommand& Command);
	bool TryGetLatestReceipt(
		Fdemo_mapShanmenSwordRhythmEffectCueAttemptReceipt& OutReceipt) const;
	bool TryGetLastAcknowledgement(
		Fdemo_mapShanmenSwordRhythmEffectCueAcknowledgementReceipt&
			OutAcknowledgement) const;

private:
	struct FRouteRecord
	{
		Fdemo_mapShanmenSwordRhythmEffectCueConsumerRoute Route;
		TArray<Fdemo_mapShanmenSwordRhythmEffectCueAttemptReceipt> Attempts;
		bool bAcknowledged = false;
	};

	Fdemo_mapShanmenSwordRhythmEffectCueDeliveryCursor Cursor;
	bool bHasRouteRecord = false;
	FRouteRecord CurrentRoute;
};
