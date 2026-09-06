#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenThrownWeaponArcPreviewPresentationDeliveryCoordinator.h"

class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoff;
class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecovery;

enum class
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceResponseOutcome
	: uint8
{
	Invalid,
	Applied,
	Rejected
};

/**
 * Immutable evidence returned by one renderer-facing surface mutation.
 *
 * A surface cursor contains only the currently visible presentation State:
 * Empty means that no Arc preview is visible. Hidden audit States remain in
 * the delivery ledger and are deliberately normalized to an empty surface.
 */
class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceResponse
{
public:
	static bool TryCreate(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommand& Command,
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceResponseOutcome
			Outcome,
		FName OutcomeCode,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState&
			PreviousSurfaceCursor,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState&
			SurfaceCursor,
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceResponse&
			OutResponse,
		FString& OutDiagnostic);

	bool IsValid() const;
	bool MatchesCommand(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommand& Command)
		const;
	bool IsApplied() const;
	bool IsRejected() const;

	const FGuid& GetResponseId() const { return ResponseId; }
	const FGuid& GetCommandId() const { return CommandId; }
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationCommandKind GetCommandKind()
		const
	{
		return CommandKind;
	}
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceResponseOutcome
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
	FGuid CommandId;
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationCommandKind CommandKind =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationCommandKind::Invalid;
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceResponseOutcome
		Outcome =
			Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceResponseOutcome::
				Invalid;
	FName OutcomeCode = NAME_None;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState
		PreviousSurfaceCursor;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState SurfaceCursor;
};

/**
 * Narrow renderer-facing surface capability.
 *
 * Show, Replace and Hide are separate operations so adapters cannot silently
 * reinterpret command kinds. NoOp is intentionally absent: the consumer
 * adapter validates its cursor and acknowledges NoOp without a surface call.
 * Implementations own no delivery ledger, retry loop or product authority.
 */
class Idemo_mapShanmenThrownWeaponArcPreviewPresentationSurface
{
public:
	virtual ~Idemo_mapShanmenThrownWeaponArcPreviewPresentationSurface() =
		default;
	virtual FName GetConsumerDefinitionId() const = 0;
	virtual Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState
	GetSurfaceCursor() const = 0;
	virtual Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceResponse
	Show(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommand& Command)
		= 0;
	virtual Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceResponse
	Replace(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommand& Command)
		= 0;
	virtual Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceResponse
	Hide(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommand& Command)
		= 0;
};

enum class Edemo_mapShanmenThrownWeaponArcPreviewPresentationConsumerAdapterStatus
	: uint8
{
	Invalid,
	AdapterInactive,
	AdapterInvalid,
	OperationInProgress,
	CommandInvalid,
	RunMismatch,
	CursorMismatch,
	SurfaceResponseInvalid,
	SurfaceRejected,
	SurfaceInvariantViolation,
	Applied,
	NoOpApplied
};

/** Self-validating evidence for one port-to-surface adapter attempt. */
class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationConsumerAdapterResult
{
public:
	bool IsValid() const;
	bool IsAccepted() const;
	bool WasApplied() const;
	bool WasRejected() const;
	bool DidCallSurface() const;
	bool IsNoOp() const;

	Edemo_mapShanmenThrownWeaponArcPreviewPresentationConsumerAdapterStatus
	GetStatus() const
	{
		return Status;
	}
	const FString& GetDiagnostic() const { return Diagnostic; }
	const FGuid& GetRunId() const { return RunId; }
	FName GetConsumerDefinitionId() const { return ConsumerDefinitionId; }
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommand& GetCommand()
		const
	{
		return Command;
	}
	int32 GetSurfaceCallCount() const { return SurfaceCallCount; }
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceResponse&
	GetSurfaceResponse() const
	{
		return SurfaceResponse;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationPortResponse&
	GetPortResponse() const
	{
		return PortResponse;
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
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationConsumerAdapter;

	Edemo_mapShanmenThrownWeaponArcPreviewPresentationConsumerAdapterStatus
		Status =
			Edemo_mapShanmenThrownWeaponArcPreviewPresentationConsumerAdapterStatus::
				Invalid;
	FString Diagnostic;
	FGuid RunId;
	FName ConsumerDefinitionId = NAME_None;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommand Command;
	int32 SurfaceCallCount = 0;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceResponse
		SurfaceResponse;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationPortResponse PortResponse;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState
		PreviousSurfaceCursor;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState SurfaceCursor;
};

/**
 * Run-scoped renderer-facing implementation of the delivery port.
 *
 * The adapter is the only owner of surface dispatch policy. It normalizes
 * hidden presentation States to an empty renderer cursor, maps each mutating
 * command to exactly one typed surface call and maps NoOp to zero calls.
 * The bound surface is non-owning and must outlive the active adapter scope.
 */
class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationConsumerAdapter final
	: public Idemo_mapShanmenThrownWeaponArcPreviewPresentationPort
{
public:
	bool TryBegin(
		const FGuid& RunId,
		Idemo_mapShanmenThrownWeaponArcPreviewPresentationSurface& Surface,
		FString& OutDiagnostic);
	bool TryEnd(const FGuid& ExpectedRunId, FString& OutDiagnostic);

	virtual FName GetConsumerDefinitionId() const override
	{
		return ConsumerDefinitionId;
	}
	virtual Fdemo_mapShanmenThrownWeaponArcPreviewPresentationPortResponse Apply(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommand& Command)
		override;

	bool IsValid() const;
	bool IsActive() const { return Surface != nullptr; }
	bool IsEmpty() const { return IsValid() && !IsActive(); }
	bool IsOperationInProgress() const { return bOperationInProgress; }
	bool CanEnd() const;
	const FGuid& GetRunId() const { return RunId; }
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState GetSurfaceCursor()
		const;
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationConsumerAdapterResult&
	GetLastResult() const
	{
		return LastResult;
	}

private:
	friend class
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoff;
	friend class
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecovery;

	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationConsumerAdapterResult
	MakeResult(
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationConsumerAdapterStatus
			Status,
		const TCHAR* Diagnostic,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationCommand& Command,
		int32 SurfaceCallCount,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceResponse&
			SurfaceResponse,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationPortResponse&
			PortResponse,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState&
			PreviousSurfaceCursor,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState&
			SurfaceCursor) const;

	FGuid RunId;
	FName ConsumerDefinitionId = NAME_None;
	Idemo_mapShanmenThrownWeaponArcPreviewPresentationSurface* Surface = nullptr;
	bool bOperationInProgress = false;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationConsumerAdapterResult
		LastResult;
};
