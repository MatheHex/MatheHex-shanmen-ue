#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenThrownWeaponArcPreviewProductBridge.h"

enum class Edemo_mapShanmenThrownWeaponArcPreviewPresentationMode : uint8
{
	Invalid,
	Hidden,
	Visible
};

/** One immutable renderer-neutral line segment from a sampled Arc preview. */
class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSegment
{
public:
	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSegment& Other)
		const;
	const FGuid& GetSegmentId() const { return SegmentId; }
	const FGuid& GetSourcePreviewId() const { return SourcePreviewId; }
	int32 GetIndex() const { return Index; }
	const FVector& GetStart() const { return Start; }
	const FVector& GetEnd() const { return End; }

private:
	friend class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationProjector;
	friend class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState;

	FGuid SegmentId;
	FGuid SourcePreviewId;
	int32 Index = INDEX_NONE;
	FVector Start = FVector::ZeroVector;
	FVector End = FVector::ZeroVector;
};

/**
 * Consumer-owned renderer-neutral snapshot for one Arc preview scope.
 *
 * Visible snapshots expose immutable line segments, apex, landing and flight
 * time. Hidden snapshots retain only scope and choice revision as a tombstone,
 * so an older asynchronous preview cannot reappear after a clear operation.
 * This value owns no widget, renderer, component, World, Actor, timer, input,
 * product session, coordinator, reservation, projectile or inventory state.
 */
class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState
{
public:
	static bool TryRehydrateVisible(
		const FGuid& ExpectedPresentationStateId,
		const FGuid& RunId,
		const FGuid& PlayerEntityId,
		const FGuid& SourceItemInstanceId,
		const Fdemo_mapShanmenThrownWeaponInputChoiceState& ChoiceState,
		const FGuid& SourceProductRequestId,
		const FGuid& SourcePreviewActivationId,
		const FShanmenThrownWeaponArcPreview& SourcePreview,
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState& OutState);

	bool IsEmpty() const;
	bool IsValid() const;
	bool IsVisible() const;
	bool IsHidden() const;
	bool Matches(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState& Other)
		const;

	const FGuid& GetPresentationStateId() const
	{
		return PresentationStateId;
	}
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationMode GetMode() const
	{
		return Mode;
	}
	const FGuid& GetRunId() const { return RunId; }
	const FGuid& GetPlayerEntityId() const { return PlayerEntityId; }
	const FGuid& GetSourceItemInstanceId() const
	{
		return SourceItemInstanceId;
	}
	const Fdemo_mapShanmenThrownWeaponInputChoiceState& GetChoiceState() const
	{
		return ChoiceState;
	}
	uint64 GetChoiceRevision() const { return ChoiceState.GetRevision(); }
	const FGuid& GetSourceProductRequestId() const
	{
		return SourceProductRequestId;
	}
	const FGuid& GetSourcePreviewActivationId() const
	{
		return SourcePreviewActivationId;
	}
	const FShanmenThrownWeaponArcPreview& GetSourcePreview() const
	{
		return SourcePreview;
	}
	const TArray<Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSegment>&
	GetSegments() const
	{
		return Segments;
	}
	int32 NumSegments() const { return Segments.Num(); }
	const FVector& GetApexPosition() const { return ApexPosition; }
	const FVector& GetPlannedLandingPosition() const
	{
		return PlannedLandingPosition;
	}
	double GetFlightTimeSeconds() const { return FlightTimeSeconds; }

private:
	friend class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationProjector;
	friend class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationReducer;

	FGuid PresentationStateId;
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationMode Mode =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationMode::Invalid;
	FGuid RunId;
	FGuid PlayerEntityId;
	FGuid SourceItemInstanceId;
	Fdemo_mapShanmenThrownWeaponInputChoiceState ChoiceState;
	FGuid SourceProductRequestId;
	FGuid SourcePreviewActivationId;
	FShanmenThrownWeaponArcPreview SourcePreview;
	TArray<Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSegment> Segments;
	FVector ApexPosition = FVector::ZeroVector;
	FVector PlannedLandingPosition = FVector::ZeroVector;
	double FlightTimeSeconds = 0.0;
};

enum class Edemo_mapShanmenThrownWeaponArcPreviewPresentationProjectStatus
	: uint8
{
	Invalid,
	BridgeRejected,
	BasisRejected,
	CompositionRejected,
	IdentityMismatch,
	StateRejected,
	Projected
};

class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationProjectResult
{
public:
	bool IsProjected() const;
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationProjectStatus GetStatus()
		const
	{
		return Status;
	}
	const FString& GetDiagnostic() const { return Diagnostic; }
	int32 GetCompositionCount() const { return CompositionCount; }
	const Fdemo_mapShanmenThrownWeaponArcPreviewCompositionResult&
	GetComposition() const
	{
		return Composition;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState& GetState()
		const
	{
		return State;
	}

private:
	friend class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationProjector;

	Edemo_mapShanmenThrownWeaponArcPreviewPresentationProjectStatus Status =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationProjectStatus::Invalid;
	FString Diagnostic;
	int32 CompositionCount = 0;
	Fdemo_mapShanmenThrownWeaponArcPreviewCompositionResult Composition;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState State;
};

/** Pure P20.32-to-presentation projector; composes geometry exactly once. */
class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationProjector
{
public:
	static Fdemo_mapShanmenThrownWeaponArcPreviewPresentationProjectResult
	Project(
		const Fdemo_mapShanmenThrownWeaponArcPreviewProductBridgeResult& Bridge,
		const Fdemo_mapShanmenThrownWeaponArcChoiceBasis& SourceBasis);

private:
	static Fdemo_mapShanmenThrownWeaponArcPreviewPresentationProjectResult
	Reject(
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationProjectStatus Status,
		const TCHAR* Diagnostic);
};

enum class Edemo_mapShanmenThrownWeaponArcPreviewPresentationReduceStatus
	: uint8
{
	Invalid,
	Replaced,
	Cleared,
	Duplicate,
	CandidateRejected,
	PreviousStateRequired,
	PreviousStateInvalid,
	IdentityMismatch,
	StaleRevision,
	RevisionConflict,
	ChoiceRejected,
	ClearNotRequired,
	StateRejected
};

class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationReduceResult
{
public:
	bool IsApplied() const;
	bool IsReplaced() const;
	bool IsCleared() const;
	bool IsDuplicate() const;
	Edemo_mapShanmenThrownWeaponArcPreviewPresentationReduceStatus GetStatus()
		const
	{
		return Status;
	}
	const FString& GetDiagnostic() const { return Diagnostic; }
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState& GetState()
		const
	{
		return State;
	}

private:
	friend class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationReducer;

	Edemo_mapShanmenThrownWeaponArcPreviewPresentationReduceStatus Status =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationReduceStatus::Invalid;
	FString Diagnostic;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState State;
};

/** Stateless reducer; each presentation consumer owns its previous state. */
class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationReducer
{
public:
	static Fdemo_mapShanmenThrownWeaponArcPreviewPresentationReduceResult
	Replace(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState&
			PreviousState,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState&
			CandidateState);

	static Fdemo_mapShanmenThrownWeaponArcPreviewPresentationReduceResult Clear(
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState&
			PreviousState,
		const Fdemo_mapShanmenThrownWeaponInputChoiceState& CurrentChoice);

private:
	static Fdemo_mapShanmenThrownWeaponArcPreviewPresentationReduceResult
	Reject(
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationReduceStatus Status,
		const TCHAR* Diagnostic);
};
