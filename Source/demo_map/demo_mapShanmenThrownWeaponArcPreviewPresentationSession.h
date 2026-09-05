#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenThrownWeaponArcPreviewUpdateCoordinator.h"

enum class Edemo_mapShanmenThrownWeaponArcPreviewPresentationSessionStatus
	: uint8
{
	Invalid,
	SessionInactive,
	SessionInvalid,
	ProductUnavailable,
	RunMismatch,
	UpdateRejected,
	StateRejected,
	Applied,
	NoChange
};

/** Immutable audit result for one consumer-owned session update attempt. */
class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSessionResult
{
public:
	bool IsValid() const;
	bool IsAccepted() const;
	bool DidChange() const;
	bool IsNoChange() const;

	Edemo_mapShanmenThrownWeaponArcPreviewPresentationSessionStatus
	GetStatus() const
	{
		return Status;
	}
	const FString& GetDiagnostic() const { return Diagnostic; }
	const FGuid& GetSessionRunId() const { return SessionRunId; }
	int32 GetCoordinatorCallCount() const { return CoordinatorCallCount; }
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState&
	GetPreviousState() const
	{
		return PreviousState;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewUpdateResult& GetUpdate()
		const
	{
		return Update;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState& GetState()
		const
	{
		return State;
	}

private:
	friend class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSession;

	Edemo_mapShanmenThrownWeaponArcPreviewPresentationSessionStatus Status =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationSessionStatus::
			Invalid;
	FString Diagnostic;
	FGuid SessionRunId;
	int32 CoordinatorCallCount = 0;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState PreviousState;
	Fdemo_mapShanmenThrownWeaponArcPreviewUpdateResult Update;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState State;
};

/**
 * In-memory presentation state owned by one renderer or view consumer.
 *
 * The session pins one Run identity, invokes the P20.34 coordinator at most
 * once per TryUpdate, and commits only completed results. End and Reset drop
 * all consumer state so a later Run cannot inherit stale geometry or a hidden
 * tombstone. It owns no product, inventory, World, input, widget or renderer.
 */
class Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSession
{
public:
	bool TryBegin(const FGuid& RunId, FString& OutDiagnostic);

	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSessionResult TryUpdate(
		int32 HotbarSlotNumber,
		const Fdemo_mapShanmenThrownWeaponInputChoiceState& CurrentChoice,
		const Fdemo_mapShanmenThrownWeaponArcChoicePolicy& ChoicePolicy,
		int32 SegmentCount,
		const Fdemo_mapShanmenThrownWeaponArcChoiceBasis& SourceBasis,
		const Fdemo_mapShanmenThrownWeaponProductLifecycle& Lifecycle,
		const Fdemo_mapCombatRunCoordinator& Coordinator);

	bool TryEnd(const FGuid& ExpectedRunId, FString& OutDiagnostic);
	void Reset();

	bool IsValid() const;
	bool IsActive() const { return RunId.IsValid(); }
	bool IsEmpty() const { return IsValid() && !IsActive(); }
	const FGuid& GetRunId() const { return RunId; }
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState& GetState()
		const
	{
		return State;
	}

private:
	static Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSessionResult
	RejectBeforeUpdate(
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationSessionStatus Status,
		const FGuid& SessionRunId,
		const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState& State,
		const TCHAR* Diagnostic);

	FGuid RunId;
	Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState State;
};
