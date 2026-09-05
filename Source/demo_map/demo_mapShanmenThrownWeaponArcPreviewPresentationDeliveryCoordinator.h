#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenThrownWeaponArcPreviewPresentationCommandLedger.h"

enum class
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationPortResponseOutcome
	: uint8
{
	Invalid,
	Applied,
	Rejected
};

/**
 * Immutable response from one renderer-neutral presentation port call.
 *
 * It attests only to the command observed by the port. It deliberately does
 * not prove that a widget, renderer, component or visible frame existed.
 */
class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationPortResponse
{
public:
	static bool TryCreate(
		const FGuid& CommandId,
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationPortResponseOutcome
			Outcome,
		FName OutcomeCode,
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationPortResponse&
			OutResponse,
		FString& OutDiagnostic);

	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationPortResponse&
			Other) const;
	bool IsApplied() const;
	bool IsRejected() const;

	const FGuid& GetResponseId() const { return ResponseId; }
	const FGuid& GetCommandId() const { return CommandId; }
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationPortResponseOutcome
	GetOutcome() const
	{
		return Outcome;
	}
	FName GetOutcomeCode() const { return OutcomeCode; }

private:
	FGuid ResponseId;
	FGuid CommandId;
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationPortResponseOutcome
		Outcome =
			Edemo_mapShanmenThrownWeaponArcPreviewPresentationPortResponseOutcome::
				Invalid;
	FName OutcomeCode = NAME_None;
};

/**
 * Narrow renderer-neutral capability consumed by the delivery coordinator.
 *
 * Production adapters may eventually forward to a HUD or renderer. The port
 * contract itself owns no World, Actor, widget, component, retry or ledger.
 */
class Idemo_mapShanmenThrownWeaponArcPreviewPresentationPort
{
public:
	virtual ~Idemo_mapShanmenThrownWeaponArcPreviewPresentationPort() = default;
	virtual FName GetConsumerDefinitionId() const = 0;
	virtual Fdemo_mapShanmenThrownWeaponArcPreviewPresentationPortResponse Apply(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommand& Command)
		= 0;
};

enum class
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryStatus
	: uint8
{
	Invalid,
	CommandInvalid,
	LedgerInvalid,
	LedgerInactive,
	RunMismatch,
	PortConsumerInvalid,
	ConsumerMismatch,
	CursorMismatch,
	ApplicationReplayed,
	RejectionReplayed,
	PortApplied,
	PortRejected,
	PortResponseRejected,
	LedgerCommitRejected,
	InvariantViolation
};

/** Self-validating outcome for one bounded presentation delivery request. */
class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryResult
{
public:
	bool IsValid() const;
	bool IsAccepted() const;
	bool WasApplied() const;
	bool WasRejected() const;
	bool DidCallPort() const;
	bool IsReplay() const;

	Edemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryStatus
	GetStatus() const
	{
		return Status;
	}
	const FString& GetDiagnostic() const { return Diagnostic; }
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommand&
	GetCommand() const
	{
		return Command;
	}
	FName GetConsumerDefinitionId() const { return ConsumerDefinitionId; }
	int32 GetPortCallCount() const { return PortCallCount; }
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationPortResponse&
	GetPortResponse() const
	{
		return PortResponse;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandReceipt&
	GetReceipt() const
	{
		return Receipt;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandLedgerResult&
	GetLedgerResult() const
	{
		return LedgerResult;
	}

private:
	friend class
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryCoordinator;

	Edemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryStatus Status =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryStatus::
			Invalid;
	FString Diagnostic;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommand Command;
	FName ConsumerDefinitionId = NAME_None;
	int32 PortCallCount = 0;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationPortResponse PortResponse;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandReceipt Receipt;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandLedgerResult
		LedgerResult;
};

/**
 * Stateless single-attempt bridge from one command to one consumer ledger.
 *
 * A command already carrying Applied or Rejected evidence is replayed without
 * invoking the port. A new in-order command invokes Apply exactly once, seals
 * the response as a receipt and atomically commits a candidate ledger copy.
 */
class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryCoordinator
{
public:
	static Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryResult
	Deliver(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommand& Command,
		Idemo_mapShanmenThrownWeaponArcPreviewPresentationPort& Port,
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandLedger& Ledger);

private:
	static Fdemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryResult
	MakeResult(
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryStatus Status,
		const TCHAR* Diagnostic,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommand& Command,
		FName ConsumerDefinitionId = NAME_None,
		int32 PortCallCount = 0,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationPortResponse&
			PortResponse =
				Fdemo_mapShanmenThrownWeaponArcPreviewPresentationPortResponse(),
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandReceipt&
			Receipt =
				Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandReceipt(),
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandLedgerResult&
			LedgerResult =
				Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommandLedgerResult());
};
