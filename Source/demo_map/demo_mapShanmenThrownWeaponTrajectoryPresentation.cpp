#include "demo_mapShanmenThrownWeaponTrajectoryPresentation.h"

namespace
{
	using ECapability =
		Edemo_mapShanmenThrownWeaponInputChoiceInteractionCapability;
	using ETrajectory =
		Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind;

	FString MakeDisplayText(
		const FString& ModeLabel,
		const FString& ToggleKeyLabel)
	{
		return FString::Printf(
			TEXT("THROWN TRAJECTORY: %s  |  [%s] Toggle"),
			*ModeLabel,
			*ToggleKeyLabel);
	}
}

bool Fdemo_mapShanmenThrownWeaponTrajectoryPresentation::TryProject(
	const Fdemo_mapShanmenThrownWeaponInputChoiceInteractionReadResult& Read,
	const FString& RawToggleKeyLabel,
	Fdemo_mapShanmenThrownWeaponTrajectoryPresentation& OutPresentation)
{
	OutPresentation =
		Fdemo_mapShanmenThrownWeaponTrajectoryPresentation();
	if (!Read.IsProjected())
	{
		return false;
	}

	const FString CanonicalToggleKeyLabel =
		RawToggleKeyLabel.TrimStartAndEnd();
	if (CanonicalToggleKeyLabel.IsEmpty())
	{
		return false;
	}

	const auto& ReadModel = Read.GetReadModel();
	FString CanonicalModeLabel;
	if (ReadModel.GetTrajectoryKind() == ETrajectory::Straight
		&& ReadModel.HasCapability(
			ECapability::SelectBallisticArcTrajectory))
	{
		CanonicalModeLabel = TEXT("STRAIGHT");
	}
	else if (ReadModel.GetTrajectoryKind() == ETrajectory::BallisticArc
		&& ReadModel.HasCapability(ECapability::SelectStraightTrajectory))
	{
		CanonicalModeLabel = TEXT("BALLISTIC ARC");
	}
	else
	{
		return false;
	}

	Fdemo_mapShanmenThrownWeaponTrajectoryPresentation Candidate;
	Candidate.TrajectoryKind = ReadModel.GetTrajectoryKind();
	Candidate.ModeLabel = MoveTemp(CanonicalModeLabel);
	Candidate.ToggleKeyLabel = CanonicalToggleKeyLabel;
	Candidate.DisplayText = MakeDisplayText(
		Candidate.ModeLabel,
		Candidate.ToggleKeyLabel);
	if (!Candidate.IsValid())
	{
		return false;
	}

	OutPresentation = MoveTemp(Candidate);
	return true;
}

bool Fdemo_mapShanmenThrownWeaponTrajectoryPresentation::IsValid() const
{
	const bool bKnownTrajectory =
		(TrajectoryKind == ETrajectory::Straight
			&& ModeLabel == TEXT("STRAIGHT"))
		|| (TrajectoryKind == ETrajectory::BallisticArc
			&& ModeLabel == TEXT("BALLISTIC ARC"));
	return bKnownTrajectory
		&& !ToggleKeyLabel.IsEmpty()
		&& ToggleKeyLabel == ToggleKeyLabel.TrimStartAndEnd()
		&& DisplayText == MakeDisplayText(ModeLabel, ToggleKeyLabel);
}

bool Fdemo_mapShanmenThrownWeaponTrajectoryPresentation::IsBallisticArc()
	const
{
	return IsValid() && TrajectoryKind == ETrajectory::BallisticArc;
}
