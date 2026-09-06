#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRecreationPolicy.h"

enum class
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceLifecycleResponseOutcome
	: uint8
{
	Invalid,
	Applied,
	Rejected
};

/** Immutable renderer attestation for one explicit cleanup permit. */
class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceLifecycleResponse
{
public:
	static bool TryCreate(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRecreationPermit&
			Permit,
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceLifecycleResponseOutcome
			Outcome,
		FName OutcomeCode,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState&
			PreviousSurfaceCursor,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState&
			SurfaceCursor,
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceLifecycleResponse&
			OutResponse,
		FString& OutDiagnostic);

	bool IsValid() const;
	bool MatchesPermit(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRecreationPermit&
			Permit) const;
	bool IsApplied() const;
	bool IsRejected() const;

	const FGuid& GetResponseId() const { return ResponseId; }
	const FGuid& GetPermitId() const { return PermitId; }
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceLifecycleResponseOutcome
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
	FGuid PermitId;
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceLifecycleResponseOutcome
		Outcome =
			Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceLifecycleResponseOutcome::
				Invalid;
	FName OutcomeCode = NAME_None;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState
		PreviousSurfaceCursor;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState SurfaceCursor;
};

/**
 * Narrow capability used only to clear an observed renderer surface.
 *
 * This interface owns no delivery command, Host cursor, retry loop, Owner
 * pointer or rehydrate policy. Implementations must attest Applied/Rejected
 * against the exact cleanup permit supplied by the executor.
 */
class Idemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceLifecycle
{
public:
	virtual ~Idemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceLifecycle()
		= default;
	virtual FName GetConsumerDefinitionId() const = 0;
	virtual Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState
	GetSurfaceCursor() const = 0;
	virtual
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceLifecycleResponse
	ClearToEmpty(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRecreationPermit&
			Permit) = 0;
};

enum class
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceLifecycleReceiptOutcome
	: uint8
{
	Invalid,
	BindingReady,
	CleanupApplied,
	CleanupRejected
};

/** Immutable receipt for one bounded lifecycle-executor decision. */
class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceLifecycleReceipt
{
public:
	static bool TryCreate(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRecreationPermit&
			Permit,
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceLifecycleReceiptOutcome
			Outcome,
		int32 SurfaceCallCount,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceLifecycleResponse&
			SurfaceResponse,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState&
			PreviousSurfaceCursor,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState&
			SurfaceCursor,
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceLifecycleReceipt&
			OutReceipt,
		FString& OutDiagnostic);

	bool IsValid() const;
	bool MatchesPermit(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRecreationPermit&
			Permit) const;
	bool IsBindingReady() const;
	bool WasCleanupApplied() const;
	bool WasCleanupRejected() const;

	const FGuid& GetReceiptId() const { return ReceiptId; }
	const FGuid& GetPermitId() const { return PermitId; }
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceLifecycleReceiptOutcome
	GetOutcome() const
	{
		return Outcome;
	}
	int32 GetSurfaceCallCount() const { return SurfaceCallCount; }
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceLifecycleResponse&
	GetSurfaceResponse() const
	{
		return SurfaceResponse;
	}
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
	FGuid ReceiptId;
	FGuid PermitId;
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceLifecycleReceiptOutcome
		Outcome =
			Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceLifecycleReceiptOutcome::
				Invalid;
	int32 SurfaceCallCount = 0;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceLifecycleResponse
		SurfaceResponse;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState
		PreviousSurfaceCursor;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState SurfaceCursor;
};

enum class
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceLifecycleExecutorStatus
	: uint8
{
	Invalid,
	OperationInProgress,
	PermitInvalid,
	ConsumerMismatch,
	SnapshotMismatch,
	SurfaceResponseInvalid,
	SurfaceInvariantViolation,
	SurfaceRejected,
	BindingReady,
	Cleared
};

/** Self-validating evidence for one bounded permit execution attempt. */
class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceLifecycleExecutorResult
{
public:
	bool IsValid() const { return bValidated; }
	bool IsAccepted() const;
	bool IsBindingReady() const;
	bool DidClear() const;
	bool WasSurfaceRejected() const;
	bool DidCallSurface() const;
	bool HasReceipt() const;
	bool CanRetryExactPermit() const;
	bool NeedsReevaluation() const;

	Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceLifecycleExecutorStatus
	GetStatus() const
	{
		return Status;
	}
	const FString& GetDiagnostic() const { return Diagnostic; }
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRecreationPermit&
	GetPermit() const
	{
		return Permit;
	}
	int32 GetSurfaceCallCount() const { return SurfaceCallCount; }
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceLifecycleResponse&
	GetSurfaceResponse() const
	{
		return SurfaceResponse;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceLifecycleReceipt&
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
	GetSurfaceCursor() const
	{
		return SurfaceCursor;
	}

private:
	friend class
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceLifecycleExecutor;
	bool Validate() const;

	Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceLifecycleExecutorStatus
		Status =
			Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceLifecycleExecutorStatus::
				Invalid;
	FString Diagnostic;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRecreationPermit
		Permit;
	int32 SurfaceCallCount = 0;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceLifecycleResponse
		SurfaceResponse;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceLifecycleReceipt
		Receipt;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState
		PreviousSurfaceCursor;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState SurfaceCursor;
	bool bValidated = false;
};

/**
 * Executes one P20.44 permit with a strict zero-or-one surface-call budget.
 * Binding permits produce receipts without mutation. Cleanup permits call the
 * lifecycle capability once and never retry internally. The executor owns no
 * surface or composition Owner pointer.
 */
class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceLifecycleExecutor
{
public:
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceLifecycleExecutorResult
	Execute(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRecreationPermit&
			Permit,
		Idemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceLifecycle&
			Surface);

	bool IsOperationInProgress() const { return bOperationInProgress; }

private:
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceLifecycleExecutorResult
	MakeResult(
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceLifecycleExecutorStatus
			Status,
		const TCHAR* Diagnostic,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRecreationPermit&
			Permit,
		int32 SurfaceCallCount,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceLifecycleResponse&
			SurfaceResponse,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceLifecycleReceipt&
			Receipt,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState&
			PreviousSurfaceCursor,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState&
			SurfaceCursor) const;

	bool bOperationInProgress = false;
};
