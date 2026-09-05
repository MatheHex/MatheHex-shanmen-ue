#include "demo_mapShanmenThrownWeaponArcEditingInputHintPresentation.h"

namespace
{
	bool AreLabelsDistinct(
		const FString& A,
		const FString& B,
		const FString& C,
		const FString& D)
	{
		return !A.Equals(B, ESearchCase::IgnoreCase)
			&& !A.Equals(C, ESearchCase::IgnoreCase)
			&& !A.Equals(D, ESearchCase::IgnoreCase)
			&& !B.Equals(C, ESearchCase::IgnoreCase)
			&& !B.Equals(D, ESearchCase::IgnoreCase)
			&& !C.Equals(D, ESearchCase::IgnoreCase);
	}

	FString MakeDisplayText(
		const FString& TargetKeyLabel,
		const FString& ApexIncreaseKeyLabel,
		const FString& ApexDecreaseKeyLabel,
		const FString& ClearKeyLabel,
		const bool bCanIncreaseArcApex,
		const bool bCanDecreaseArcApex,
		const bool bCanClearArcTargetIntent)
	{
		return FString::Printf(
			TEXT("ARC INPUT: [%s] SET TARGET | [%s] APEX +%s | [%s] APEX -%s | [%s] CLEAR%s"),
			*TargetKeyLabel,
			*ApexIncreaseKeyLabel,
			bCanIncreaseArcApex ? TEXT("") : TEXT(" (LIMIT)"),
			*ApexDecreaseKeyLabel,
			bCanDecreaseArcApex ? TEXT("") : TEXT(" (LIMIT)"),
			*ClearKeyLabel,
			bCanClearArcTargetIntent ? TEXT("") : TEXT(" (NO TARGET)"));
	}
}

bool Fdemo_mapShanmenThrownWeaponArcEditingInputHintPresentation::TryProject(
	const Fdemo_mapShanmenThrownWeaponArcEditingPresentation& ArcPresentation,
	const FString& RawTargetKeyLabel,
	const FString& RawApexIncreaseKeyLabel,
	const FString& RawApexDecreaseKeyLabel,
	const FString& RawClearKeyLabel,
	Fdemo_mapShanmenThrownWeaponArcEditingInputHintPresentation& OutPresentation)
{
	OutPresentation =
		Fdemo_mapShanmenThrownWeaponArcEditingInputHintPresentation();
	if (!ArcPresentation.IsValid()
		|| !ArcPresentation.CanSetArcTargetIntent())
	{
		return false;
	}

	const FString TargetKeyLabel = RawTargetKeyLabel.TrimStartAndEnd();
	const FString ApexIncreaseKeyLabel =
		RawApexIncreaseKeyLabel.TrimStartAndEnd();
	const FString ApexDecreaseKeyLabel =
		RawApexDecreaseKeyLabel.TrimStartAndEnd();
	const FString ClearKeyLabel = RawClearKeyLabel.TrimStartAndEnd();
	if (TargetKeyLabel.IsEmpty()
		|| ApexIncreaseKeyLabel.IsEmpty()
		|| ApexDecreaseKeyLabel.IsEmpty()
		|| ClearKeyLabel.IsEmpty()
		|| !AreLabelsDistinct(
			TargetKeyLabel,
			ApexIncreaseKeyLabel,
			ApexDecreaseKeyLabel,
			ClearKeyLabel))
	{
		return false;
	}

	Fdemo_mapShanmenThrownWeaponArcEditingInputHintPresentation Candidate;
	Candidate.bCanIncreaseArcApex =
		ArcPresentation.CanIncreaseArcApex();
	Candidate.bCanDecreaseArcApex =
		ArcPresentation.CanDecreaseArcApex();
	Candidate.bCanClearArcTargetIntent =
		ArcPresentation.CanClearArcTargetIntent();
	Candidate.TargetKeyLabel = TargetKeyLabel;
	Candidate.ApexIncreaseKeyLabel = ApexIncreaseKeyLabel;
	Candidate.ApexDecreaseKeyLabel = ApexDecreaseKeyLabel;
	Candidate.ClearKeyLabel = ClearKeyLabel;
	Candidate.DisplayText = MakeDisplayText(
		Candidate.TargetKeyLabel,
		Candidate.ApexIncreaseKeyLabel,
		Candidate.ApexDecreaseKeyLabel,
		Candidate.ClearKeyLabel,
		Candidate.bCanIncreaseArcApex,
		Candidate.bCanDecreaseArcApex,
		Candidate.bCanClearArcTargetIntent);
	if (!Candidate.IsValid())
	{
		return false;
	}

	OutPresentation = MoveTemp(Candidate);
	return true;
}

bool Fdemo_mapShanmenThrownWeaponArcEditingInputHintPresentation::IsValid()
	const
{
	return !TargetKeyLabel.IsEmpty()
		&& TargetKeyLabel == TargetKeyLabel.TrimStartAndEnd()
		&& !ApexIncreaseKeyLabel.IsEmpty()
		&& ApexIncreaseKeyLabel == ApexIncreaseKeyLabel.TrimStartAndEnd()
		&& !ApexDecreaseKeyLabel.IsEmpty()
		&& ApexDecreaseKeyLabel == ApexDecreaseKeyLabel.TrimStartAndEnd()
		&& !ClearKeyLabel.IsEmpty()
		&& ClearKeyLabel == ClearKeyLabel.TrimStartAndEnd()
		&& AreLabelsDistinct(
			TargetKeyLabel,
			ApexIncreaseKeyLabel,
			ApexDecreaseKeyLabel,
			ClearKeyLabel)
		&& DisplayText == MakeDisplayText(
			TargetKeyLabel,
			ApexIncreaseKeyLabel,
			ApexDecreaseKeyLabel,
			ClearKeyLabel,
			bCanIncreaseArcApex,
			bCanDecreaseArcApex,
			bCanClearArcTargetIntent);
}
