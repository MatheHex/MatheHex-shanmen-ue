#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenSwordRhythmEffectCue.h"

#include "demo_mapShanmenSwordRhythmEffectCueDelivery.generated.h"

/** Immutable identity fence for one presentation consumer inside one Run. */
USTRUCT(BlueprintType)
struct Fdemo_mapShanmenSwordRhythmEffectCueConsumerScope
{
	GENERATED_BODY()

public:
	static bool TryCreate(
		const FGuid& RunId,
		const FGuid& ConsumerId,
		FName ConsumerRoleId,
		Fdemo_mapShanmenSwordRhythmEffectCueConsumerScope& OutScope);

	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenSwordRhythmEffectCueConsumerScope& Other) const;
	const FGuid& GetScopeId() const { return ScopeId; }
	const FGuid& GetRunId() const { return RunId; }
	const FGuid& GetConsumerId() const { return ConsumerId; }
	FName GetConsumerRoleId() const { return ConsumerRoleId; }

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|EffectCue|Delivery", meta = (AllowPrivateAccess = "true"))
	FGuid ScopeId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|EffectCue|Delivery", meta = (AllowPrivateAccess = "true"))
	FGuid RunId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|EffectCue|Delivery", meta = (AllowPrivateAccess = "true"))
	FGuid ConsumerId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|EffectCue|Delivery", meta = (AllowPrivateAccess = "true"))
	FName ConsumerRoleId = NAME_None;
};

/** Immutable preparation evidence. Preparing never advances a consumer cursor. */
USTRUCT(BlueprintType)
struct Fdemo_mapShanmenSwordRhythmEffectCueDeliveryReceipt
{
	GENERATED_BODY()

public:
	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenSwordRhythmEffectCueDeliveryReceipt& Other) const;
	const FGuid& GetDeliveryId() const { return DeliveryId; }
	const Fdemo_mapShanmenSwordRhythmEffectCueConsumerScope& GetScope() const
	{
		return Scope;
	}
	const Fdemo_mapShanmenSwordRhythmEffectCueEvent& GetEvent() const
	{
		return Event;
	}

private:
	friend class Fdemo_mapShanmenSwordRhythmEffectCueDeliveryCursor;

	static bool TryCreate(
		const Fdemo_mapShanmenSwordRhythmEffectCueConsumerScope& Scope,
		const Fdemo_mapShanmenSwordRhythmEffectCueEvent& Event,
		Fdemo_mapShanmenSwordRhythmEffectCueDeliveryReceipt& OutReceipt);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|EffectCue|Delivery", meta = (AllowPrivateAccess = "true"))
	FGuid DeliveryId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|EffectCue|Delivery", meta = (AllowPrivateAccess = "true"))
	Fdemo_mapShanmenSwordRhythmEffectCueConsumerScope Scope;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|EffectCue|Delivery", meta = (AllowPrivateAccess = "true"))
	Fdemo_mapShanmenSwordRhythmEffectCueEvent Event;
};

/** Immutable proof that one exact consumer accepted one exact delivery. */
USTRUCT(BlueprintType)
struct Fdemo_mapShanmenSwordRhythmEffectCueAcknowledgementReceipt
{
	GENERATED_BODY()

public:
	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenSwordRhythmEffectCueAcknowledgementReceipt& Other) const;
	const FGuid& GetAcknowledgementId() const
	{
		return AcknowledgementId;
	}
	const Fdemo_mapShanmenSwordRhythmEffectCueDeliveryReceipt& GetDelivery()
		const
	{
		return Delivery;
	}

private:
	friend class Fdemo_mapShanmenSwordRhythmEffectCueDeliveryCursor;

	static bool TryCreate(
		const Fdemo_mapShanmenSwordRhythmEffectCueDeliveryReceipt& Delivery,
		Fdemo_mapShanmenSwordRhythmEffectCueAcknowledgementReceipt& OutReceipt);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|EffectCue|Delivery", meta = (AllowPrivateAccess = "true"))
	FGuid AcknowledgementId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shanmen|Combat|SwordRhythm|EffectCue|Delivery", meta = (AllowPrivateAccess = "true"))
	Fdemo_mapShanmenSwordRhythmEffectCueDeliveryReceipt Delivery;
};

enum class Edemo_mapShanmenSwordRhythmEffectCuePrepareStatus : uint8
{
	Prepared,
	AlreadyAcknowledged,
	CursorInvalid,
	EventInvalid,
	RunMismatch,
	StaleEvent,
	DeliveryRejected
};

struct Fdemo_mapShanmenSwordRhythmEffectCuePrepareResult
{
	Edemo_mapShanmenSwordRhythmEffectCuePrepareStatus Status =
		Edemo_mapShanmenSwordRhythmEffectCuePrepareStatus::CursorInvalid;
	FString Diagnostic;
	Fdemo_mapShanmenSwordRhythmEffectCueDeliveryReceipt Delivery;

	bool IsPrepared() const
	{
		return Status
			== Edemo_mapShanmenSwordRhythmEffectCuePrepareStatus::Prepared
			&& Delivery.IsValid();
	}
};

enum class Edemo_mapShanmenSwordRhythmEffectCueAcknowledgeStatus : uint8
{
	Acknowledged,
	AcknowledgementReplayed,
	CursorInvalid,
	DeliveryInvalid,
	ScopeMismatch,
	StaleDelivery,
	AcknowledgementRejected
};

struct Fdemo_mapShanmenSwordRhythmEffectCueAcknowledgeResult
{
	Edemo_mapShanmenSwordRhythmEffectCueAcknowledgeStatus Status =
		Edemo_mapShanmenSwordRhythmEffectCueAcknowledgeStatus::CursorInvalid;
	FString Diagnostic;
	Fdemo_mapShanmenSwordRhythmEffectCueAcknowledgementReceipt
		Acknowledgement;

	bool IsAcknowledged() const
	{
		return (Status
				== Edemo_mapShanmenSwordRhythmEffectCueAcknowledgeStatus::
					Acknowledged
			|| Status
				== Edemo_mapShanmenSwordRhythmEffectCueAcknowledgeStatus::
					AcknowledgementReplayed)
			&& Acknowledgement.IsValid();
	}
};

/**
 * Consumer-owned, Run-scoped delivery cursor.
 *
 * Prepare is read-only and retry-safe. Only acknowledgement advances this
 * cursor. ProductSession, the cue event, and sibling consumers own no shared
 * "already played" state.
 */
class Fdemo_mapShanmenSwordRhythmEffectCueDeliveryCursor
{
public:
	static bool TryCreate(
		const Fdemo_mapShanmenSwordRhythmEffectCueConsumerScope& Scope,
		Fdemo_mapShanmenSwordRhythmEffectCueDeliveryCursor& OutCursor);

	bool IsValid() const;
	bool IsEmpty() const { return IsValid() && !bHasAcknowledgement; }
	const Fdemo_mapShanmenSwordRhythmEffectCueConsumerScope& GetScope() const
	{
		return Scope;
	}
	bool TryGetLastAcknowledgement(
		Fdemo_mapShanmenSwordRhythmEffectCueAcknowledgementReceipt&
			OutAcknowledgement) const;

	Fdemo_mapShanmenSwordRhythmEffectCuePrepareResult Prepare(
		const Fdemo_mapShanmenSwordRhythmEffectCueEvent& Event) const;
	Fdemo_mapShanmenSwordRhythmEffectCueAcknowledgeResult Acknowledge(
		const Fdemo_mapShanmenSwordRhythmEffectCueDeliveryReceipt& Delivery);

private:
	Fdemo_mapShanmenSwordRhythmEffectCueConsumerScope Scope;
	bool bHasAcknowledgement = false;
	Fdemo_mapShanmenSwordRhythmEffectCueAcknowledgementReceipt
		LastAcknowledgement;
};
