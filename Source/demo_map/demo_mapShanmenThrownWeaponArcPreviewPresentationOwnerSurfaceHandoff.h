#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenThrownWeaponArcPreviewPresentationConsumerAdapter.h"
#include "demo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceOwnershipTransition.h"

class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCompositionOwner;

enum class
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRetirementOutcome
	: uint8
{
	Invalid,
	Applied,
	Rejected
};

/** Immutable renderer attestation for retiring the currently owned surface. */
class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRetirementResponse
{
public:
	static bool TryCreate(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceOwnershipTransitionTicket&
			TransitionTicket,
		const FGuid& RetiredSurfaceInstanceId,
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRetirementOutcome
			Outcome,
		FName OutcomeCode,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState&
			PreviousSurfaceCursor,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState&
			SurfaceCursor,
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRetirementResponse&
			OutResponse,
		FString& OutDiagnostic);

	bool IsValid() const;
	bool MatchesTransitionTicket(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceOwnershipTransitionTicket&
			TransitionTicket) const;
	bool IsApplied() const;
	bool IsRejected() const;

	const FGuid& GetResponseId() const { return ResponseId; }
	const FGuid& GetTransitionTicketId() const
	{
		return TransitionTicketId;
	}
	const FGuid& GetRetiredSurfaceInstanceId() const
	{
		return RetiredSurfaceInstanceId;
	}
	const FGuid& GetReplacementSurfaceInstanceId() const
	{
		return ReplacementSurfaceInstanceId;
	}
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRetirementOutcome
	GetOutcome() const
	{
		return Outcome;
	}
	FName GetOutcomeCode() const { return OutcomeCode; }
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState&
	GetPreviousSurfaceCursor() const
	{
		return PreviousSurfaceCursor;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState&
	GetSurfaceCursor() const
	{
		return SurfaceCursor;
	}

private:
	FGuid ResponseId;
	FGuid TransitionTicketId;
	FGuid RetiredSurfaceInstanceId;
	FGuid ReplacementSurfaceInstanceId;
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRetirementOutcome
		Outcome =
			Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRetirementOutcome::
				Invalid;
	FName OutcomeCode = NAME_None;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState
		PreviousSurfaceCursor;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState SurfaceCursor;
};

/**
 * Concrete surface capability required for a live Owner handoff.
 *
 * The caller must supply the exact, still-live currently bound surface and
 * the exact replacement surface. AdoptExact additionally retires the old
 * visible instance once before pointer commit. BindFresh requires both
 * physical cursors to be empty and performs no retirement mutation.
 */
class Idemo_mapShanmenThrownWeaponArcPreviewPresentationHandoffSurface
	: public Idemo_mapShanmenThrownWeaponArcPreviewPresentationSurface
	, public Idemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceOwnershipCandidate
{
public:
	virtual ~Idemo_mapShanmenThrownWeaponArcPreviewPresentationHandoffSurface()
		= default;
	virtual FName GetConsumerDefinitionId() const override = 0;
	virtual Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState
	GetSurfaceCursor() const override = 0;
	virtual Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRetirementResponse
	RetireForHandoff(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceOwnershipTransitionTicket&
			TransitionTicket) = 0;
};

enum class
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffReceiptOutcome
	: uint8
{
	Invalid,
	FreshBound,
	ExactAdopted
};

/** Immutable receipt for one committed Owner/Adapter pointer handoff. */
class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffReceipt
{
public:
	bool IsValid() const;
	bool MatchesTransitionTicket(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceOwnershipTransitionTicket&
			TransitionTicket) const;
	bool MatchesCurrentBinding(
		const FGuid& RunId,
		FName ConsumerDefinitionId,
		const FGuid& SurfaceInstanceId) const;
	bool IsFreshBound() const;
	bool IsExactAdopted() const;

	const FGuid& GetReceiptId() const { return ReceiptId; }
	const FGuid& GetTransitionTicketId() const
	{
		return TransitionTicketId;
	}
	const FGuid& GetRunId() const { return RunId; }
	FName GetConsumerDefinitionId() const { return ConsumerDefinitionId; }
	const FGuid& GetPreviousSurfaceInstanceId() const
	{
		return PreviousSurfaceInstanceId;
	}
	const FGuid& GetSurfaceInstanceId() const { return SurfaceInstanceId; }
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRecreationAction
	GetAction() const
	{
		return Action;
	}
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffReceiptOutcome
	GetOutcome() const
	{
		return Outcome;
	}
	int32 GetRetirementCallCount() const { return RetirementCallCount; }
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRetirementResponse&
	GetRetirementResponse() const
	{
		return RetirementResponse;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState&
	GetPreviousSurfaceCursor() const
	{
		return PreviousSurfaceCursor;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState&
	GetRetiredSurfaceCursor() const
	{
		return RetiredSurfaceCursor;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState&
	GetSurfaceCursor() const
	{
		return SurfaceCursor;
	}

private:
	friend class
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoff;

	FGuid ReceiptId;
	FGuid TransitionTicketId;
	FGuid RunId;
	FName ConsumerDefinitionId = NAME_None;
	FGuid PreviousSurfaceInstanceId;
	FGuid SurfaceInstanceId;
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRecreationAction
		Action =
			Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRecreationAction::
				Invalid;
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffReceiptOutcome
		Outcome =
			Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffReceiptOutcome::
				Invalid;
	int32 RetirementCallCount = 0;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRetirementResponse
		RetirementResponse;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState
		PreviousSurfaceCursor;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState
		RetiredSurfaceCursor;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState SurfaceCursor;
};

enum class
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffStatus
	: uint8
{
	Invalid,
	OperationInProgress,
	OwnerInactive,
	OwnerInvalid,
	TicketInvalid,
	TicketReplayConflict,
	OldSurfaceMismatch,
	NewSurfaceMismatch,
	SnapshotMismatch,
	RetirementResponseInvalid,
	RetirementRejected,
	RetirementInvariantViolation,
	CommitInvariantViolation,
	Committed,
	Replayed
};

/** Self-validating evidence for one bounded Owner surface handoff attempt. */
class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffResult
{
public:
	bool IsValid() const { return bValidated; }
	bool IsAccepted() const;
	bool WasCommitted() const;
	bool IsReplay() const;
	bool WasRetirementRejected() const;
	bool DidCallRetirement() const;
	bool HasReceipt() const;
	bool CanRetryExactTicket() const;
	bool NeedsManualRecovery() const;

	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffStatus
	GetStatus() const
	{
		return Status;
	}
	const FString& GetDiagnostic() const { return Diagnostic; }
	int32 GetRetirementCallCount() const { return RetirementCallCount; }
	const FGuid& GetPreviousSurfaceInstanceId() const
	{
		return PreviousSurfaceInstanceId;
	}
	const FGuid& GetSurfaceInstanceId() const { return SurfaceInstanceId; }
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRetirementResponse&
	GetRetirementResponse() const
	{
		return RetirementResponse;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffReceipt&
	GetReceipt() const
	{
		return Receipt;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState&
	GetPreviousSurfaceCursor() const
	{
		return PreviousSurfaceCursor;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState&
	GetRetiredSurfaceCursor() const
	{
		return RetiredSurfaceCursor;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState&
	GetSurfaceCursor() const
	{
		return SurfaceCursor;
	}
	bool IsOwnerValidAfter() const { return bOwnerValidAfter; }

private:
	friend class
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoff;
	bool Validate() const;

	Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffStatus
		Status =
			Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffStatus::
				Invalid;
	FString Diagnostic;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceOwnershipTransitionTicket
		TransitionTicket;
	int32 RetirementCallCount = 0;
	FGuid PreviousSurfaceInstanceId;
	FGuid SurfaceInstanceId;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRetirementResponse
		RetirementResponse;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffReceipt
		Receipt;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState
		PreviousSurfaceCursor;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState
		RetiredSurfaceCursor;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState SurfaceCursor;
	bool bOwnerValidAfter = false;
	bool bValidated = false;
};

/**
 * Atomically commits one ticket-authorized replacement into Owner and Adapter.
 *
 * No callback is made until the supplied old surface pointer matches the
 * current binding. AdoptExact retires the old visible surface once; BindFresh
 * swaps two empty surfaces without mutation. Successful tickets are retained
 * by Owner for deterministic replay recognition.
 */
class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoff
{
public:
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffResult
	Execute(
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCompositionOwner&
			Owner,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceOwnershipTransitionTicket&
			TransitionTicket,
		Idemo_mapShanmenThrownWeaponArcPreviewPresentationHandoffSurface&
			ExpectedOldSurface,
		Idemo_mapShanmenThrownWeaponArcPreviewPresentationHandoffSurface&
			NewSurface);

private:
	bool TryMakeReceipt(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceOwnershipTransitionTicket&
			TransitionTicket,
		const FGuid& PreviousSurfaceInstanceId,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRetirementResponse&
			RetirementResponse,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState&
			PreviousSurfaceCursor,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState&
			RetiredSurfaceCursor,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState&
			SurfaceCursor,
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffReceipt&
			OutReceipt) const;

	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffResult
	MakeResult(
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffStatus
			Status,
		const TCHAR* Diagnostic,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceOwnershipTransitionTicket&
			TransitionTicket,
		int32 RetirementCallCount,
		const FGuid& PreviousSurfaceInstanceId,
		const FGuid& SurfaceInstanceId,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRetirementResponse&
			RetirementResponse,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffReceipt&
			Receipt,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState&
			PreviousSurfaceCursor,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState&
			RetiredSurfaceCursor,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState&
			SurfaceCursor,
		bool bOwnerValidAfter) const;
};
