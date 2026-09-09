#include "demo_mapShanmenThrownWeaponTerminalFeedbackPresentation.h"

namespace
{
	using EFeedback =
		Edemo_mapShanmenThrownWeaponTerminalFeedbackKind;
	using ETerminal = Edemo_mapShanmenThrownWeaponTerminalKind;
	using FPresentation =
		Fdemo_mapShanmenThrownWeaponTerminalFeedbackPresentation;

	EFeedback ProjectKind(const ETerminal Kind)
	{
		switch (Kind)
		{
		case ETerminal::Impact:
			return EFeedback::Impact;
		case ETerminal::BlockingMiss:
			return EFeedback::BlockingMiss;
		case ETerminal::RangeExpired:
			return EFeedback::RangeExpired;
		case ETerminal::FlightTimeExpired:
			return EFeedback::FlightTimeExpired;
		case ETerminal::Interrupted:
			return EFeedback::Interrupted;
		default:
			return EFeedback::Invalid;
		}
	}

	FString BuildDisplayText(
		const EFeedback Kind,
		const float AppliedDamage,
		const bool bDefeatedTarget)
	{
		switch (Kind)
		{
		case EFeedback::Impact:
			if (AppliedDamage <= 0.0f)
			{
				return TEXT("飞刀 · 未造成伤害");
			}
			return FString::Printf(
				TEXT("飞刀 · %s · -%.1f"),
				bDefeatedTarget ? TEXT("击破") : TEXT("命中"),
				static_cast<double>(AppliedDamage));
		case EFeedback::BlockingMiss:
			return TEXT("飞刀 · 命中阻挡");
		case EFeedback::RangeExpired:
			return TEXT("飞刀 · 超出射程");
		case EFeedback::FlightTimeExpired:
			return TEXT("飞刀 · 落空");
		case EFeedback::Interrupted:
			return TEXT("飞刀 · 已中断");
		default:
			return FString();
		}
	}
}

bool FPresentation::TryProject(
	const Fdemo_mapShanmenThrownWeaponTerminalReceipt& Receipt,
	FPresentation& OutPresentation)
{
	OutPresentation = FPresentation();
	if (!Receipt.IsValid())
	{
		return false;
	}

	FPresentation Candidate;
	Candidate.Kind = ProjectKind(Receipt.Kind);
	Candidate.LaunchId = Receipt.LaunchId;
	if (Candidate.Kind == EFeedback::Impact)
	{
		const FShanmenVitalityCommitResult& Commit =
			Receipt.Delivery.Delivery.CommitResult;
		if (!Receipt.Delivery.IsDelivered()
			|| Commit.Status != EShanmenVitalityCommitStatus::Committed
			|| !Commit.Receipt.IsValid())
		{
			return false;
		}
		Candidate.AppliedDamage = Commit.Receipt.GetAppliedDamage();
		Candidate.bDefeatedTarget = Candidate.AppliedDamage > 0.0f
			&& Commit.Receipt.GetVitalityAfter() <= 0.0f;
	}
	Candidate.DisplayText = BuildDisplayText(
		Candidate.Kind,
		Candidate.AppliedDamage,
		Candidate.bDefeatedTarget);
	if (!Candidate.IsValid())
	{
		return false;
	}

	OutPresentation = MoveTemp(Candidate);
	return true;
}

bool FPresentation::IsValid() const
{
	if (Kind == EFeedback::Invalid
		|| !LaunchId.IsValid()
		|| !FMath::IsFinite(AppliedDamage)
		|| AppliedDamage < 0.0f
		|| (bDefeatedTarget && AppliedDamage <= 0.0f))
	{
		return false;
	}
	if (Kind != EFeedback::Impact
		&& (AppliedDamage != 0.0f || bDefeatedTarget))
	{
		return false;
	}
	return !DisplayText.IsEmpty()
		&& DisplayText == DisplayText.TrimStartAndEnd()
		&& DisplayText == BuildDisplayText(
			Kind, AppliedDamage, bDefeatedTarget);
}

bool FPresentation::Matches(const FPresentation& Other) const
{
	return IsValid() && Other.IsValid()
		&& Kind == Other.Kind
		&& LaunchId == Other.LaunchId
		&& AppliedDamage == Other.AppliedDamage
		&& bDefeatedTarget == Other.bDefeatedTarget
		&& DisplayText == Other.DisplayText;
}
