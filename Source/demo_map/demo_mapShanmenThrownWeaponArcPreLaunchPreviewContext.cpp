#include "demo_mapShanmenThrownWeaponArcPreLaunchPreviewContext.h"

namespace
{
	using EAction =
		Edemo_mapShanmenThrownWeaponArcPreLaunchPressAction;
	using EStatus =
		Edemo_mapShanmenThrownWeaponArcPreLaunchHotbarStatus;

	bool IsValidHotbarSlot(const int32 HotbarSlotNumber)
	{
		return HotbarSlotNumber >= 1 && HotbarSlotNumber <= 9;
	}
}

bool Fdemo_mapShanmenThrownWeaponArcPreLaunchPreviewContext::TryBegin(
	const FGuid& RequestedRunId,
	FString& OutDiagnostic)
{
	OutDiagnostic.Reset();
	if (!IsValid() || !RequestedRunId.IsValid())
	{
		OutDiagnostic = TEXT(
			"Arc pre-launch preview context requires valid empty state and Run identity.");
		return false;
	}
	if (IsActive())
	{
		if (RunId == RequestedRunId)
		{
			OutDiagnostic = TEXT(
				"Arc pre-launch preview context already owns this Run.");
			return true;
		}
		OutDiagnostic = TEXT(
			"Arc pre-launch preview context rejects a second active Run.");
		return false;
	}

	RunId = RequestedRunId;
	if (!IsValid())
	{
		Reset();
		OutDiagnostic = TEXT(
			"Arc pre-launch preview context failed Run-begin invariants.");
		return false;
	}
	OutDiagnostic = TEXT(
		"Arc pre-launch preview context bound one empty Run selection.");
	return true;
}

bool Fdemo_mapShanmenThrownWeaponArcPreLaunchPreviewContext::TryEnd(
	const FGuid& ExpectedRunId,
	FString& OutDiagnostic)
{
	OutDiagnostic.Reset();
	if (!IsValid() || !ExpectedRunId.IsValid())
	{
		OutDiagnostic = TEXT(
			"Arc pre-launch preview context end requires valid state and Run identity.");
		return false;
	}
	if (!IsActive())
	{
		OutDiagnostic = TEXT(
			"Arc pre-launch preview context is already empty.");
		return true;
	}
	if (RunId != ExpectedRunId)
	{
		OutDiagnostic = TEXT(
			"Arc pre-launch preview context rejects mismatched Run teardown.");
		return false;
	}

	Reset();
	OutDiagnostic = TEXT(
		"Arc pre-launch preview context ended and discarded its selected slot.");
	return true;
}

bool Fdemo_mapShanmenThrownWeaponArcPreLaunchPreviewContext::
	RouteEligibleHotbarPress(
		const FGuid& ExpectedRunId,
		const int32 HotbarSlotNumber,
		EAction& OutAction,
		FString& OutDiagnostic)
{
	OutAction = EAction::Invalid;
	if (!CanMutate(ExpectedRunId, OutDiagnostic))
	{
		return false;
	}
	if (!IsValidHotbarSlot(HotbarSlotNumber))
	{
		OutDiagnostic = TEXT(
			"Arc pre-launch preview accepts only hotbar slots 1 through 9.");
		return false;
	}
	if (ArmedHotbarSlotNumber == HotbarSlotNumber)
	{
		OutAction = EAction::ConfirmationRequested;
		OutDiagnostic = TEXT(
			"Same-slot Arc press requested launch confirmation without changing selection.");
		return true;
	}
	if (Revision == MAX_uint64)
	{
		OutDiagnostic = TEXT(
			"Arc pre-launch preview context revision is exhausted.");
		return false;
	}

	const bool bHadArmedSlot = HasArmedHotbarSlot();
	ArmedHotbarSlotNumber = HotbarSlotNumber;
	++Revision;
	OutAction = bHadArmedSlot ? EAction::Rearmed : EAction::Armed;
	OutDiagnostic = bHadArmedSlot
		? TEXT("Arc pre-launch preview moved selection to another eligible slot.")
		: TEXT("Arc pre-launch preview armed one eligible slot.");
	return IsValid();
}

bool Fdemo_mapShanmenThrownWeaponArcPreLaunchPreviewContext::TryCancel(
	const FGuid& ExpectedRunId,
	FString& OutDiagnostic)
{
	if (!CanMutate(ExpectedRunId, OutDiagnostic))
	{
		return false;
	}
	if (!HasArmedHotbarSlot())
	{
		OutDiagnostic = TEXT(
			"Arc pre-launch preview cancellation is an empty-state no-op.");
		return true;
	}
	if (Revision == MAX_uint64)
	{
		OutDiagnostic = TEXT(
			"Arc pre-launch preview context revision is exhausted.");
		return false;
	}

	ArmedHotbarSlotNumber = INDEX_NONE;
	++Revision;
	OutDiagnostic = TEXT(
		"Arc pre-launch preview cancellation cleared the selected slot.");
	return IsValid();
}

bool Fdemo_mapShanmenThrownWeaponArcPreLaunchPreviewContext::
	TryCompleteConfirmation(
		const FGuid& ExpectedRunId,
		const int32 HotbarSlotNumber,
		const bool bLaunchAccepted,
		FString& OutDiagnostic)
{
	if (!CanMutate(ExpectedRunId, OutDiagnostic))
	{
		return false;
	}
	if (!IsValidHotbarSlot(HotbarSlotNumber)
		|| ArmedHotbarSlotNumber != HotbarSlotNumber)
	{
		OutDiagnostic = TEXT(
			"Arc pre-launch completion requires the exact armed hotbar slot.");
		return false;
	}
	if (!bLaunchAccepted)
	{
		OutDiagnostic = TEXT(
			"Rejected Arc launch preserved the armed preview for correction and retry.");
		return true;
	}
	if (Revision == MAX_uint64)
	{
		OutDiagnostic = TEXT(
			"Arc pre-launch preview context revision is exhausted.");
		return false;
	}

	ArmedHotbarSlotNumber = INDEX_NONE;
	++Revision;
	OutDiagnostic = TEXT(
		"Accepted Arc launch consumed and cleared the armed preview context.");
	return IsValid();
}

bool Fdemo_mapShanmenThrownWeaponArcPreLaunchPreviewContext::IsValid() const
{
	if (Revision == MAX_uint64)
	{
		return false;
	}
	if (!RunId.IsValid())
	{
		return ArmedHotbarSlotNumber == INDEX_NONE && Revision == 0;
	}
	return ArmedHotbarSlotNumber == INDEX_NONE
		|| IsValidHotbarSlot(ArmedHotbarSlotNumber);
}

void Fdemo_mapShanmenThrownWeaponArcPreLaunchPreviewContext::Reset()
{
	*this = Fdemo_mapShanmenThrownWeaponArcPreLaunchPreviewContext();
}

bool Fdemo_mapShanmenThrownWeaponArcPreLaunchPreviewContext::CanMutate(
	const FGuid& ExpectedRunId,
	FString& OutDiagnostic) const
{
	OutDiagnostic.Reset();
	if (!IsValid() || !IsActive())
	{
		OutDiagnostic = TEXT(
			"Arc pre-launch preview mutation requires one active valid context.");
		return false;
	}
	if (!ExpectedRunId.IsValid() || RunId != ExpectedRunId)
	{
		OutDiagnostic = TEXT(
			"Arc pre-launch preview mutation rejects a mismatched Run.");
		return false;
	}
	return true;
}

bool Fdemo_mapShanmenThrownWeaponArcPreLaunchHotbarResult::IsValid() const
{
	if (Status == EStatus::Invalid || Diagnostic.IsEmpty()
		|| !IsValidHotbarSlot(HotbarSlotNumber)
		|| SourceBasisSampleCount < 0 || SourceBasisSampleCount > 1)
	{
		return false;
	}
	if (Status == EStatus::Rejected)
	{
		return PressAction == EAction::Invalid
			&& ContextRevisionAfter == 0;
	}
	if (Status == EStatus::StraightRouteRequired
		|| Status == EStatus::ArcNonThrownPassThrough)
	{
		return !ItemInstanceId.IsValid()
			&& PressAction == EAction::Invalid
			&& SourceBasisSampleCount == 0
			&& !bPreviewUpdateAttempted
			&& !bPreviewVisibleAfter
			&& ContextRevisionAfter == 0;
	}
	if (!ItemInstanceId.IsValid() || ContextRevisionAfter == 0)
	{
		return false;
	}
	if (Status == EStatus::ArcConfirmationRequested)
	{
		return PressAction == EAction::ConfirmationRequested
			&& SourceBasisSampleCount == 0
			&& !bPreviewUpdateAttempted;
	}
	const bool bActionMatches =
		(Status == EStatus::ArcArmed && PressAction == EAction::Armed)
		|| (Status == EStatus::ArcRearmed && PressAction == EAction::Rearmed);
	return bActionMatches
		&& bPreviewUpdateAttempted
		&& (bPreviewVisibleAfter
			? SourceBasisSampleCount == 1
			: SourceBasisSampleCount == 0);
}

bool Fdemo_mapShanmenThrownWeaponArcPreLaunchHotbarResult::
	ShouldRouteExistingConfirmation() const
{
	return IsValid()
		&& (Status == EStatus::StraightRouteRequired
			|| Status == EStatus::ArcConfirmationRequested);
}

bool Fdemo_mapShanmenThrownWeaponArcPreLaunchHotbarResult::
	ShouldPassThrough() const
{
	return IsValid() && Status == EStatus::ArcNonThrownPassThrough;
}

bool Fdemo_mapShanmenThrownWeaponArcPreLaunchHotbarResult::
	IsConsumedWithoutConfirmation() const
{
	return IsValid()
		&& (Status == EStatus::ArcArmed
			|| Status == EStatus::ArcRearmed
			|| Status == EStatus::Rejected);
}
