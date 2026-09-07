#pragma once

#include "CoreMinimal.h"

class Ademo_mapGameMode;

enum class Edemo_mapShanmenThrownWeaponArcPreLaunchPressAction : uint8
{
	Invalid,
	Armed,
	Rearmed,
	ConfirmationRequested
};

/**
 * Run-scoped selection state for the pre-launch Arc preview gesture.
 *
 * This context owns only the selected hotbar slot. It deliberately owns no
 * item, trajectory choice, launch command, actor, World, or presentation
 * state. A same-slot second press requests confirmation without mutating the
 * context; a rejected launch therefore remains correctable and retryable.
 */
class Fdemo_mapShanmenThrownWeaponArcPreLaunchPreviewContext
{
public:
	bool TryBegin(const FGuid& RequestedRunId, FString& OutDiagnostic);
	bool TryEnd(const FGuid& ExpectedRunId, FString& OutDiagnostic);
	bool RouteEligibleHotbarPress(
		const FGuid& ExpectedRunId,
		int32 HotbarSlotNumber,
		Edemo_mapShanmenThrownWeaponArcPreLaunchPressAction& OutAction,
		FString& OutDiagnostic);
	bool TryCancel(const FGuid& ExpectedRunId, FString& OutDiagnostic);
	bool TryCompleteConfirmation(
		const FGuid& ExpectedRunId,
		int32 HotbarSlotNumber,
		bool bLaunchAccepted,
		FString& OutDiagnostic);

	bool IsValid() const;
	bool IsActive() const { return RunId.IsValid(); }
	bool HasArmedHotbarSlot() const { return ArmedHotbarSlotNumber != INDEX_NONE; }
	const FGuid& GetRunId() const { return RunId; }
	int32 GetArmedHotbarSlotNumber() const { return ArmedHotbarSlotNumber; }
	uint64 GetRevision() const { return Revision; }

private:
	void Reset();
	bool CanMutate(const FGuid& ExpectedRunId, FString& OutDiagnostic) const;

	FGuid RunId;
	int32 ArmedHotbarSlotNumber = INDEX_NONE;
	uint64 Revision = 0;
};

enum class Edemo_mapShanmenThrownWeaponArcPreLaunchHotbarStatus : uint8
{
	Invalid,
	StraightRouteRequired,
	ArcNonThrownPassThrough,
	ArcArmed,
	ArcRearmed,
	ArcConfirmationRequested,
	Rejected
};

/** Audit evidence for the GameMode's bounded pre-launch hotbar decision. */
class Fdemo_mapShanmenThrownWeaponArcPreLaunchHotbarResult
{
public:
	bool IsValid() const;
	bool ShouldRouteExistingConfirmation() const;
	bool ShouldPassThrough() const;
	bool IsConsumedWithoutConfirmation() const;
	bool IsArcConfirmationRequested() const
	{
		return Status
			== Edemo_mapShanmenThrownWeaponArcPreLaunchHotbarStatus::
				ArcConfirmationRequested;
	}
	Edemo_mapShanmenThrownWeaponArcPreLaunchHotbarStatus GetStatus() const
	{
		return Status;
	}
	const FString& GetDiagnostic() const { return Diagnostic; }
	int32 GetHotbarSlotNumber() const { return HotbarSlotNumber; }
	const FGuid& GetItemInstanceId() const { return ItemInstanceId; }
	Edemo_mapShanmenThrownWeaponArcPreLaunchPressAction GetPressAction() const
	{
		return PressAction;
	}
	int32 GetSourceBasisSampleCount() const { return SourceBasisSampleCount; }
	bool DidAttemptPreviewUpdate() const { return bPreviewUpdateAttempted; }
	bool IsPreviewVisibleAfter() const { return bPreviewVisibleAfter; }
	uint64 GetContextRevisionAfter() const { return ContextRevisionAfter; }

private:
	friend class Ademo_mapGameMode;

	Edemo_mapShanmenThrownWeaponArcPreLaunchHotbarStatus Status =
		Edemo_mapShanmenThrownWeaponArcPreLaunchHotbarStatus::Invalid;
	FString Diagnostic;
	int32 HotbarSlotNumber = INDEX_NONE;
	FGuid ItemInstanceId;
	Edemo_mapShanmenThrownWeaponArcPreLaunchPressAction PressAction =
		Edemo_mapShanmenThrownWeaponArcPreLaunchPressAction::Invalid;
	int32 SourceBasisSampleCount = 0;
	bool bPreviewUpdateAttempted = false;
	bool bPreviewVisibleAfter = false;
	uint64 ContextRevisionAfter = 0;
};
