#include "demo_mapShanmenThrownWeaponArcPreLaunchGestureFeedbackPresentation.h"

namespace
{
	using ECapability =
		Edemo_mapShanmenThrownWeaponInputChoiceInteractionCapability;
	using EMode =
		Edemo_mapShanmenThrownWeaponArcPreLaunchGestureFeedbackMode;
	using ETrajectory =
		Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind;
	using FPresentation =
		Fdemo_mapShanmenThrownWeaponArcPreLaunchGestureFeedbackPresentation;
	using FReadModel =
		Fdemo_mapShanmenThrownWeaponInputChoiceInteractionReadModel;

	bool IsValidHotbarSlot(const int32 HotbarSlotNumber)
	{
		return HotbarSlotNumber >= 1 && HotbarSlotNumber <= 9;
	}

	FString MakeDisplayText(
		const EMode Mode,
		const int32 HotbarSlotNumber,
		const FString& HotbarKeyLabel)
	{
		if (Mode == EMode::TargetRequired)
		{
			return FString::Printf(
				TEXT("ARC PREVIEW: SLOT %d [%s] ARMED | SET TARGET TO SHOW ARC"),
				HotbarSlotNumber,
				*HotbarKeyLabel);
		}
		if (Mode == EMode::ReadyToConfirm)
		{
			return FString::Printf(
				TEXT("ARC PREVIEW: SLOT %d [%s] ARMED | EDIT TARGET/APEX | PRESS [%s] AGAIN TO THROW"),
				HotbarSlotNumber,
				*HotbarKeyLabel,
				*HotbarKeyLabel);
		}
		return FString();
	}
}

bool FPresentation::TryProject(
	const Fdemo_mapShanmenThrownWeaponArcPreLaunchPreviewContext& Context,
	const Fdemo_mapShanmenThrownWeaponInputChoiceInteractionReadResult&
		ChoiceRead,
	const Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState&
		PhysicalPreviewCursor,
	const FString& RawHotbarKeyLabel,
	FPresentation& OutPresentation)
{
	OutPresentation = FPresentation();
	if (!Context.IsValid() || !Context.IsActive()
		|| !Context.HasArmedHotbarSlot()
		|| !IsValidHotbarSlot(Context.GetArmedHotbarSlotNumber())
		|| Context.GetRevision() == 0
		|| !ChoiceRead.IsProjected())
	{
		return false;
	}

	const FReadModel& Choice = ChoiceRead.GetReadModel();
	if (Choice.GetTrajectoryKind() != ETrajectory::BallisticArc
		|| !Choice.HasCapability(ECapability::SelectStraightTrajectory)
		|| !Choice.HasCapability(ECapability::SetArcTargetIntent))
	{
		return false;
	}
	const FString HotbarKeyLabel = RawHotbarKeyLabel.TrimStartAndEnd();
	if (HotbarKeyLabel.IsEmpty())
	{
		return false;
	}

	EMode Mode = EMode::Invalid;
	if (!Choice.HasArcTargetIntent())
	{
		if (!PhysicalPreviewCursor.IsEmpty())
		{
			return false;
		}
		Mode = EMode::TargetRequired;
	}
	else
	{
		if (!PhysicalPreviewCursor.IsValid()
			|| !PhysicalPreviewCursor.IsVisible()
			|| PhysicalPreviewCursor.GetRunId() != Context.GetRunId())
		{
			return false;
		}
		FReadModel PreviewChoice;
		if (!FReadModel::TryProject(
				PhysicalPreviewCursor.GetChoiceState(), PreviewChoice)
			|| !PreviewChoice.Matches(Choice))
		{
			return false;
		}
		Mode = EMode::ReadyToConfirm;
	}

	FPresentation Candidate;
	Candidate.Mode = Mode;
	Candidate.RunId = Context.GetRunId();
	Candidate.ArmedHotbarSlotNumber =
		Context.GetArmedHotbarSlotNumber();
	Candidate.ContextRevision = Context.GetRevision();
	Candidate.HotbarKeyLabel = HotbarKeyLabel;
	Candidate.DisplayText = MakeDisplayText(
		Candidate.Mode,
		Candidate.ArmedHotbarSlotNumber,
		Candidate.HotbarKeyLabel);
	if (!Candidate.IsValid())
	{
		return false;
	}

	OutPresentation = MoveTemp(Candidate);
	return true;
}

bool FPresentation::IsValid() const
{
	const bool bKnownMode = Mode == EMode::TargetRequired
		|| Mode == EMode::ReadyToConfirm;
	return bKnownMode
		&& RunId.IsValid()
		&& IsValidHotbarSlot(ArmedHotbarSlotNumber)
		&& ContextRevision > 0 && ContextRevision < MAX_uint64
		&& !HotbarKeyLabel.IsEmpty()
		&& HotbarKeyLabel == HotbarKeyLabel.TrimStartAndEnd()
		&& DisplayText == MakeDisplayText(
			Mode, ArmedHotbarSlotNumber, HotbarKeyLabel);
}

bool FPresentation::NeedsArcTarget() const
{
	return IsValid() && Mode == EMode::TargetRequired;
}

bool FPresentation::IsReadyToConfirm() const
{
	return IsValid() && Mode == EMode::ReadyToConfirm;
}
