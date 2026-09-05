#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenThrownWeaponArcPreviewPresentation.h"

class Fdemo_mapCombatRunCoordinator;
class Fdemo_mapShanmenThrownWeaponProductLifecycle;

enum class Edemo_mapShanmenThrownWeaponArcPreviewUpdateStatus : uint8
{
	Invalid,
	ChoiceRejected,
	PreviousStateRejected,
	NoPresentationRequired,
	CaptureRejected,
	ProjectRejected,
	ReduceRejected,
	Replaced,
	Cleared,
	Duplicate
};

/**
 * Immutable audit result for one bounded live-preview presentation update.
 *
 * A visible update may capture, project and reduce exactly once each. A clear
 * update skips product capture and geometry projection so stale visuals can be
 * removed even after the product lifecycle has ended. The result owns no
 * product, inventory, Run, World, input, widget or renderer authority.
 */
class Fdemo_mapShanmenThrownWeaponArcPreviewUpdateResult
{
public:
	bool IsValid() const;
	bool IsCompleted() const;
	bool DidChange() const;
	bool IsNoChange() const;

	Edemo_mapShanmenThrownWeaponArcPreviewUpdateStatus GetStatus() const
	{
		return Status;
	}
	const FString& GetDiagnostic() const { return Diagnostic; }
	int32 GetCaptureCount() const { return CaptureCount; }
	int32 GetProjectCount() const { return ProjectCount; }
	int32 GetReduceCount() const { return ReduceCount; }
	const Fdemo_mapShanmenThrownWeaponInputChoiceState& GetChoiceState() const
	{
		return ChoiceState;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState&
	GetPreviousState() const
	{
		return PreviousState;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewProductBridgeResult&
	GetBridge() const
	{
		return Bridge;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationProjectResult&
	GetProjection() const
	{
		return Projection;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationReduceResult&
	GetReduction() const
	{
		return Reduction;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState& GetState()
		const
	{
		return State;
	}

private:
	friend class Fdemo_mapShanmenThrownWeaponArcPreviewUpdateCoordinator;

	Edemo_mapShanmenThrownWeaponArcPreviewUpdateStatus Status =
		Edemo_mapShanmenThrownWeaponArcPreviewUpdateStatus::Invalid;
	FString Diagnostic;
	int32 CaptureCount = 0;
	int32 ProjectCount = 0;
	int32 ReduceCount = 0;
	Fdemo_mapShanmenThrownWeaponInputChoiceState ChoiceState;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState PreviousState;
	Fdemo_mapShanmenThrownWeaponArcPreviewProductBridgeResult Bridge;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationProjectResult Projection;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationReduceResult Reduction;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState State;
};

/**
 * Stateless synchronous coordinator for one consumer-owned presentation state.
 *
 * Product lifecycle and Run coordinator are read-only. The caller remains the
 * sole owner of PreviousState and decides whether to retain GetState().
 */
class Fdemo_mapShanmenThrownWeaponArcPreviewUpdateCoordinator
{
public:
	static Fdemo_mapShanmenThrownWeaponArcPreviewUpdateResult Update(
		int32 HotbarSlotNumber,
		const Fdemo_mapShanmenThrownWeaponInputChoiceState& CurrentChoice,
		const Fdemo_mapShanmenThrownWeaponArcChoicePolicy& ChoicePolicy,
		int32 SegmentCount,
		const Fdemo_mapShanmenThrownWeaponArcChoiceBasis& SourceBasis,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState&
			PreviousState,
		const Fdemo_mapShanmenThrownWeaponProductLifecycle& Lifecycle,
		const Fdemo_mapCombatRunCoordinator& Coordinator);
};
