#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenThrownWeaponArcPreviewPresentationConsumerAdapter.h"

enum class
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRecreationDisposition
	: uint8
{
	Invalid,
	InputRejected,
	ConsumerMismatch,
	FreshEmpty,
	ExactVisible,
	EmptyNeedsRehydrate,
	ResidualVisible,
	ForeignVisible,
	ConflictingVisible
};

enum class
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRecreationAction
	: uint8
{
	Invalid,
	Inspect,
	BindFresh,
	AdoptExact,
	ClearToEmpty
};

enum class
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRecreationOutcome
	: uint8
{
	Invalid,
	Rejected,
	Inspected,
	Authorized
};

/**
 * Immutable permission for one exact renderer-surface lifecycle action.
 *
 * A permit binds the expected Run, consumer and both cursor snapshots. It is
 * not a mutation receipt and must be rechecked immediately before a future
 * executor binds, adopts or clears the surface.
 */
class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRecreationPermit
{
public:
	bool IsValid() const;
	bool IsBindingPermit() const;
	bool IsCleanupPermit() const;
	bool MatchesSnapshot(
		const FGuid& ExpectedRunId,
		FName ConsumerDefinitionId,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState&
			ExpectedSurfaceCursor,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState&
			ObservedSurfaceCursor) const;

	const FGuid& GetPermitId() const { return PermitId; }
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRecreationAction
	GetAction() const
	{
		return Action;
	}
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRecreationDisposition
	GetDisposition() const
	{
		return Disposition;
	}
	const FGuid& GetRunId() const { return RunId; }
	FName GetConsumerDefinitionId() const { return ConsumerDefinitionId; }
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
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRecreationPolicy;

	FGuid PermitId;
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRecreationAction
		Action =
			Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRecreationAction::
				Invalid;
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRecreationDisposition
		Disposition =
			Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRecreationDisposition::
				Invalid;
	FGuid RunId;
	FName ConsumerDefinitionId = NAME_None;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState
		ExpectedSurfaceCursor;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState
		ObservedSurfaceCursor;
};

/** Self-validating decision for one immutable recreation-policy request. */
class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRecreationResult
{
public:
	bool IsValid() const { return bValidated; }
	bool IsAccepted() const;
	bool IsInspected() const;
	bool IsAuthorized() const;
	bool IsRejected() const;
	bool CanBindFresh() const;
	bool CanAdoptExact() const;
	bool NeedsRehydrate() const;
	bool RequiresExplicitCleanup() const;
	bool HasPermit() const;

	const FGuid& GetDecisionId() const { return DecisionId; }
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRecreationOutcome
	GetOutcome() const
	{
		return Outcome;
	}
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRecreationAction
	GetRequestedAction() const
	{
		return RequestedAction;
	}
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRecreationDisposition
	GetDisposition() const
	{
		return Disposition;
	}
	const FString& GetDiagnostic() const { return Diagnostic; }
	const FGuid& GetRunId() const { return RunId; }
	FName GetExpectedConsumerDefinitionId() const
	{
		return ExpectedConsumerDefinitionId;
	}
	FName GetObservedConsumerDefinitionId() const
	{
		return ObservedConsumerDefinitionId;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState&
	GetAuthoritativeCursor() const
	{
		return AuthoritativeCursor;
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
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRecreationPermit&
	GetPermit() const
	{
		return Permit;
	}

private:
	friend class
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRecreationPolicy;
	bool Validate() const;

	FGuid DecisionId;
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRecreationOutcome
		Outcome =
			Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRecreationOutcome::
				Invalid;
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRecreationAction
		RequestedAction =
			Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRecreationAction::
				Invalid;
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRecreationDisposition
		Disposition =
			Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRecreationDisposition::
				Invalid;
	FString Diagnostic;
	FGuid RunId;
	FName ExpectedConsumerDefinitionId = NAME_None;
	FName ObservedConsumerDefinitionId = NAME_None;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState AuthoritativeCursor;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState
		ExpectedSurfaceCursor;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState
		ObservedSurfaceCursor;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRecreationPermit
		Permit;
	bool bValidated = false;
};

/**
 * Pure policy for a renderer surface observed after recreation or startup.
 *
 * The policy never mutates or binds a surface. It classifies the observed
 * cursor against the Host-authoritative cursor and issues at most one exact
 * permit. Empty-to-visible rehydration deliberately remains unauthorized
 * until a separate attested restore executor exists.
 */
class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRecreationPolicy
{
public:
	static
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRecreationResult
	Evaluate(
		const FGuid& RunId,
		FName ExpectedConsumerDefinitionId,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState&
			AuthoritativeCursor,
		FName ObservedConsumerDefinitionId,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState&
			ObservedSurfaceCursor,
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRecreationAction
			RequestedAction);
};
