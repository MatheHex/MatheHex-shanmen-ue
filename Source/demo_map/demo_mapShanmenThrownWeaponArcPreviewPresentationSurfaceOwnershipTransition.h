#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceLifecycleExecutor.h"

/**
 * Identity-bearing lifecycle capability for one concrete renderer surface.
 *
 * SurfaceInstanceId is immutable for the lifetime of the concrete surface.
 * A recreated renderer surface must expose a new identity even when its
 * consumer and physical cursor happen to match an earlier instance.
 */
class Idemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceOwnershipCandidate
	: public Idemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceLifecycle
{
public:
	virtual ~Idemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceOwnershipCandidate()
		= default;
	virtual FGuid GetSurfaceInstanceId() const = 0;
};

/** Immutable request for one exact surface-ownership transition attempt. */
class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceOwnershipTransitionRequest
{
public:
	static bool TryCreate(
		const FGuid& RunId,
		FName ConsumerDefinitionId,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState&
			AuthoritativeCursor,
		const FGuid& SurfaceInstanceId,
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRecreationAction
			Action,
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceOwnershipTransitionRequest&
			OutRequest,
		FString& OutDiagnostic);

	bool IsValid() const;

	const FGuid& GetRequestId() const { return RequestId; }
	const FGuid& GetRunId() const { return RunId; }
	FName GetConsumerDefinitionId() const { return ConsumerDefinitionId; }
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState&
	GetAuthoritativeCursor() const
	{
		return AuthoritativeCursor;
	}
	const FGuid& GetSurfaceInstanceId() const { return SurfaceInstanceId; }
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRecreationAction
	GetAction() const
	{
		return Action;
	}

private:
	FGuid RequestId;
	FGuid RunId;
	FName ConsumerDefinitionId = NAME_None;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState
		AuthoritativeCursor;
	FGuid SurfaceInstanceId;
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRecreationAction
		Action =
			Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRecreationAction::
				Invalid;
};

/**
 * Immutable authorization to bind one exact physical surface instance.
 *
 * This ticket does not mutate the composition Owner. A later Owner handoff
 * must re-read the candidate identity, consumer and cursor, match this ticket,
 * and consume it under its own one-shot transaction.
 */
class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceOwnershipTransitionTicket
{
public:
	static bool TryRehydrate(
		const FGuid& ExpectedTicketId,
		const FGuid& RequestId,
		const FGuid& PolicyDecisionId,
		const FGuid& PermitId,
		const FGuid& LifecycleReceiptId,
		const FGuid& RunId,
		FName ConsumerDefinitionId,
		const FGuid& SurfaceInstanceId,
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRecreationAction
			Action,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState&
			ExpectedSurfaceCursor,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState&
			ObservedSurfaceCursor,
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceOwnershipTransitionTicket&
			OutTicket);

	bool IsValid() const;
	bool MatchesCandidateSnapshot(
		const FGuid& SurfaceInstanceId,
		FName ConsumerDefinitionId,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState&
			SurfaceCursor) const;

	const FGuid& GetTicketId() const { return TicketId; }
	const FGuid& GetRequestId() const { return RequestId; }
	const FGuid& GetPolicyDecisionId() const { return PolicyDecisionId; }
	const FGuid& GetPermitId() const { return PermitId; }
	const FGuid& GetLifecycleReceiptId() const { return LifecycleReceiptId; }
	const FGuid& GetRunId() const { return RunId; }
	FName GetConsumerDefinitionId() const { return ConsumerDefinitionId; }
	const FGuid& GetSurfaceInstanceId() const { return SurfaceInstanceId; }
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRecreationAction
	GetAction() const
	{
		return Action;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState&
	GetExpectedSurfaceCursor() const
	{
		return ExpectedSurfaceCursor;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState&
	GetObservedSurfaceCursor() const
	{
		return ObservedSurfaceCursor;
	}

private:
	friend class
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceOwnershipTransition;

	FGuid TicketId;
	FGuid RequestId;
	FGuid PolicyDecisionId;
	FGuid PermitId;
	FGuid LifecycleReceiptId;
	FGuid RunId;
	FName ConsumerDefinitionId = NAME_None;
	FGuid SurfaceInstanceId;
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRecreationAction
		Action =
			Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRecreationAction::
				Invalid;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState
		ExpectedSurfaceCursor;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState
		ObservedSurfaceCursor;
};

enum class
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceOwnershipTransitionStatus
	: uint8
{
	Invalid,
	OperationInProgress,
	RequestInvalid,
	SurfaceIdentityMismatch,
	PolicyRejected,
	LifecycleRejected,
	SurfaceIdentityDrift,
	EvidenceInvariantViolation,
	BindingAuthorized,
	CleanupApplied,
	CleanupRejected
};

/** Self-validating evidence for one bounded ownership-transition attempt. */
class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceOwnershipTransitionResult
{
public:
	bool IsValid() const { return bValidated; }
	bool IsAccepted() const;
	bool DidAuthorizeBinding() const;
	bool DidClear() const;
	bool WasCleanupRejected() const;
	bool HasTransitionTicket() const;
	bool DidCallSurface() const;
	bool CanRetryExactRequest() const;
	bool NeedsReevaluation() const;

	Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceOwnershipTransitionStatus
	GetStatus() const
	{
		return Status;
	}
	const FString& GetDiagnostic() const { return Diagnostic; }
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceOwnershipTransitionRequest&
	GetRequest() const
	{
		return Request;
	}
	int32 GetIdentityQueryCount() const { return IdentityQueryCount; }
	int32 GetPolicySnapshotReadCount() const
	{
		return PolicySnapshotReadCount;
	}
	const FGuid& GetInitialSurfaceInstanceId() const
	{
		return InitialSurfaceInstanceId;
	}
	const FGuid& GetFinalSurfaceInstanceId() const
	{
		return FinalSurfaceInstanceId;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRecreationResult&
	GetPolicyResult() const
	{
		return PolicyResult;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceLifecycleExecutorResult&
	GetLifecycleResult() const
	{
		return LifecycleResult;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceOwnershipTransitionTicket&
	GetTransitionTicket() const
	{
		return TransitionTicket;
	}

private:
	friend class
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceOwnershipTransition;
	bool Validate() const;

	Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceOwnershipTransitionStatus
		Status =
			Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceOwnershipTransitionStatus::
				Invalid;
	FString Diagnostic;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceOwnershipTransitionRequest
		Request;
	int32 IdentityQueryCount = 0;
	int32 PolicySnapshotReadCount = 0;
	FGuid InitialSurfaceInstanceId;
	FGuid FinalSurfaceInstanceId;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRecreationResult
		PolicyResult;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceLifecycleExecutorResult
		LifecycleResult;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceOwnershipTransitionTicket
		TransitionTicket;
	bool bValidated = false;
};

/**
 * Evaluates policy and lifecycle evidence against the same concrete surface.
 *
 * The transaction calls no composition Owner and performs no binding. It
 * emits a ticket only for a stable BindFresh/AdoptExact candidate. Cleanup is
 * bounded to the P20.45 executor's one-call budget and always requires a new
 * policy evaluation before any later binding attempt.
 */
class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceOwnershipTransition
{
public:
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceOwnershipTransitionResult
	Execute(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceOwnershipTransitionRequest&
			Request,
		Idemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceOwnershipCandidate&
			Candidate);

	bool IsOperationInProgress() const { return bOperationInProgress; }

private:
	bool TryMakeTicket(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceOwnershipTransitionRequest&
			Request,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRecreationResult&
			PolicyResult,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceLifecycleExecutorResult&
			LifecycleResult,
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceOwnershipTransitionTicket&
			OutTicket) const;

	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceOwnershipTransitionResult
	MakeResult(
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceOwnershipTransitionStatus
			Status,
		const TCHAR* Diagnostic,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceOwnershipTransitionRequest&
			Request,
		int32 IdentityQueryCount,
		int32 PolicySnapshotReadCount,
		const FGuid& InitialSurfaceInstanceId,
		const FGuid& FinalSurfaceInstanceId,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRecreationResult&
			PolicyResult,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceLifecycleExecutorResult&
			LifecycleResult,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceOwnershipTransitionTicket&
			TransitionTicket) const;

	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceLifecycleExecutor
		LifecycleExecutor;
	bool bOperationInProgress = false;
};
