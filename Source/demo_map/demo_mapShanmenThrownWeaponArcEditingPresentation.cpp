#include "demo_mapShanmenThrownWeaponArcEditingPresentation.h"

namespace
{
	using ECapability =
		Edemo_mapShanmenThrownWeaponInputChoiceInteractionCapability;
	using ETrajectory =
		Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind;

	double CanonicalDisplayZero(const double Value)
	{
		return FMath::Abs(Value) < 0.0005 ? 0.0 : Value;
	}

	FString MakeTargetDisplayText(
		const bool bHasTarget,
		const FVector2D& Target)
	{
		if (!bHasTarget)
		{
			return TEXT("ARC TARGET: UNSET  |  SET AVAILABLE");
		}
		return FString::Printf(
			TEXT("ARC TARGET: X %+.2f  Y %+.2f  |  SET / CLEAR AVAILABLE"),
			CanonicalDisplayZero(Target.X),
			CanonicalDisplayZero(Target.Y));
	}

	FString MakeApexDisplayText(
		const double Apex,
		const bool bCanIncrease,
		const bool bCanDecrease)
	{
		const FString Adjustment = bCanIncrease && bCanDecrease
			? TEXT("+/-")
			: bCanIncrease
				? TEXT("+")
				: bCanDecrease
					? TEXT("-")
					: TEXT("NONE");
		return FString::Printf(
			TEXT("ARC APEX: %+.2f  |  ADJUST: %s"),
			CanonicalDisplayZero(Apex),
			*Adjustment);
	}
}

bool Fdemo_mapShanmenThrownWeaponArcEditingPresentation::TryProject(
	const Fdemo_mapShanmenThrownWeaponInputChoiceInteractionReadResult& Read,
	Fdemo_mapShanmenThrownWeaponArcEditingPresentation& OutPresentation)
{
	OutPresentation =
		Fdemo_mapShanmenThrownWeaponArcEditingPresentation();
	if (!Read.IsProjected())
	{
		return false;
	}

	const auto& ReadModel = Read.GetReadModel();
	if (ReadModel.GetTrajectoryKind() != ETrajectory::BallisticArc
		|| !ReadModel.HasCapability(ECapability::SelectStraightTrajectory)
		|| !ReadModel.HasCapability(ECapability::SetArcTargetIntent))
	{
		return false;
	}

	Fdemo_mapShanmenThrownWeaponArcEditingPresentation Candidate;
	Candidate.bHasArcTargetIntent = ReadModel.HasArcTargetIntent();
	Candidate.ArcTargetIntent = ReadModel.GetArcTargetIntent();
	Candidate.ArcTargetIntent.X = Candidate.ArcTargetIntent.X == 0.0
		? 0.0 : Candidate.ArcTargetIntent.X;
	Candidate.ArcTargetIntent.Y = Candidate.ArcTargetIntent.Y == 0.0
		? 0.0 : Candidate.ArcTargetIntent.Y;
	Candidate.ArcApexAdjustment = ReadModel.GetArcApexAdjustment() == 0.0
		? 0.0 : ReadModel.GetArcApexAdjustment();
	Candidate.bCanSetArcTargetIntent = true;
	Candidate.bCanIncreaseArcApex =
		ReadModel.HasCapability(ECapability::IncreaseArcApex);
	Candidate.bCanDecreaseArcApex =
		ReadModel.HasCapability(ECapability::DecreaseArcApex);
	Candidate.bCanClearArcTargetIntent =
		ReadModel.HasCapability(ECapability::ClearArcTargetIntent);
	Candidate.TargetDisplayText = MakeTargetDisplayText(
		Candidate.bHasArcTargetIntent,
		Candidate.ArcTargetIntent);
	Candidate.ApexDisplayText = MakeApexDisplayText(
		Candidate.ArcApexAdjustment,
		Candidate.bCanIncreaseArcApex,
		Candidate.bCanDecreaseArcApex);
	if (!Candidate.IsValid())
	{
		return false;
	}

	OutPresentation = MoveTemp(Candidate);
	return true;
}

bool Fdemo_mapShanmenThrownWeaponArcEditingPresentation::IsValid() const
{
	if (!FMath::IsFinite(ArcTargetIntent.X)
		|| !FMath::IsFinite(ArcTargetIntent.Y)
		|| !FMath::IsFinite(ArcApexAdjustment)
		|| ArcApexAdjustment < -1.0 || ArcApexAdjustment > 1.0
		|| !bCanSetArcTargetIntent
		|| bCanIncreaseArcApex != (ArcApexAdjustment < 1.0)
		|| bCanDecreaseArcApex != (ArcApexAdjustment > -1.0)
		|| bCanClearArcTargetIntent != bHasArcTargetIntent)
	{
		return false;
	}
	if (bHasArcTargetIntent)
	{
		if (ArcTargetIntent.IsZero()
			|| ArcTargetIntent.SizeSquared() > 1.0 + 1.0e-6)
		{
			return false;
		}
	}
	else if (!ArcTargetIntent.IsZero())
	{
		return false;
	}
	return TargetDisplayText == MakeTargetDisplayText(
			bHasArcTargetIntent, ArcTargetIntent)
		&& ApexDisplayText == MakeApexDisplayText(
			ArcApexAdjustment,
			bCanIncreaseArcApex,
			bCanDecreaseArcApex);
}
