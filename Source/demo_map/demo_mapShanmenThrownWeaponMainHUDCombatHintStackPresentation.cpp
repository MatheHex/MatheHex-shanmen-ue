#include "demo_mapShanmenThrownWeaponMainHUDCombatHintStackPresentation.h"

namespace
{
	using EKind =
		Edemo_mapShanmenThrownWeaponMainHUDCombatHintKind;
	using EMode =
		Edemo_mapShanmenThrownWeaponMainHUDCombatHintStackMode;
	using ETone =
		Edemo_mapShanmenThrownWeaponMainHUDCombatHintTone;
	using FLine =
		Fdemo_mapShanmenThrownWeaponMainHUDCombatHintLine;
	using FStack =
		Fdemo_mapShanmenThrownWeaponMainHUDCombatHintStackPresentation;

	int32 GetCanonicalRank(const EKind Kind)
	{
		switch (Kind)
		{
		case EKind::TrajectoryMode:
			return 0;
		case EKind::ArcApex:
			return 1;
		case EKind::ArcTarget:
			return 2;
		case EKind::ArcInput:
			return 3;
		case EKind::ArcPreLaunchGesture:
			return 4;
		case EKind::TerminalFeedback:
		case EKind::LaunchRejection:
			return 5;
		default:
			return INDEX_NONE;
		}
	}

	bool IsCanonicalTone(const EKind Kind, const ETone Tone)
	{
		switch (Kind)
		{
		case EKind::TrajectoryMode:
			return Tone == ETone::StraightMode || Tone == ETone::ArcMode;
		case EKind::ArcApex:
			return Tone == ETone::ArcApex;
		case EKind::ArcTarget:
			return Tone == ETone::ArcTarget;
		case EKind::ArcInput:
			return Tone == ETone::ArcInput;
		case EKind::ArcPreLaunchGesture:
			return Tone == ETone::TargetRequired
				|| Tone == ETone::ReadyToConfirm;
		case EKind::TerminalFeedback:
			return Tone == ETone::Impact
				|| Tone == ETone::Defeat
				|| Tone == ETone::NoDamage
				|| Tone == ETone::Blocked
				|| Tone == ETone::Expired
				|| Tone == ETone::Interrupted;
		case EKind::LaunchRejection:
			return Tone == ETone::Blocked;
		default:
			return false;
		}
	}

	ETone ProjectTerminalTone(
		const Fdemo_mapShanmenThrownWeaponTerminalFeedbackPresentation& Terminal)
	{
		using ETerminal =
			Edemo_mapShanmenThrownWeaponTerminalFeedbackKind;
		if (!Terminal.IsValid())
		{
			return ETone::Invalid;
		}
		switch (Terminal.GetKind())
		{
		case ETerminal::Impact:
			return Terminal.GetAppliedDamage() <= 0.0f
				? ETone::NoDamage
				: Terminal.DidDefeatTarget()
					? ETone::Defeat : ETone::Impact;
		case ETerminal::BlockingMiss:
			return ETone::Blocked;
		case ETerminal::RangeExpired:
		case ETerminal::FlightTimeExpired:
			return ETone::Expired;
		case ETerminal::Interrupted:
			return ETone::Interrupted;
		default:
			return ETone::Invalid;
		}
	}
}

bool FLine::IsValid() const
{
	return GetCanonicalRank(Kind) != INDEX_NONE
		&& IsCanonicalTone(Kind, Tone)
		&& !DisplayText.IsEmpty()
		&& DisplayText == DisplayText.TrimStartAndEnd();
}

bool FLine::Matches(const FLine& Other) const
{
	return IsValid() && Other.IsValid()
		&& Kind == Other.Kind
		&& Tone == Other.Tone
		&& DisplayText == Other.DisplayText;
}

bool FStack::TryCompose(
	const Fdemo_mapShanmenThrownWeaponTrajectoryPresentation& Trajectory,
	const Fdemo_mapShanmenThrownWeaponArcEditingPresentation& Arc,
	const Fdemo_mapShanmenThrownWeaponArcEditingInputHintPresentation& ArcInput,
	const Fdemo_mapShanmenThrownWeaponArcPreLaunchGestureFeedbackPresentation&
		Gesture,
	FStack& OutStack)
{
	return TryCompose(
		Trajectory,
		Arc,
		ArcInput,
		Gesture,
		Fdemo_mapShanmenThrownWeaponTerminalFeedbackPresentation(),
		Fdemo_mapShanmenThrownWeaponLaunchRejectionPresentation(),
		OutStack);
}

bool FStack::TryCompose(
	const Fdemo_mapShanmenThrownWeaponTrajectoryPresentation& Trajectory,
	const Fdemo_mapShanmenThrownWeaponArcEditingPresentation& Arc,
	const Fdemo_mapShanmenThrownWeaponArcEditingInputHintPresentation& ArcInput,
	const Fdemo_mapShanmenThrownWeaponArcPreLaunchGestureFeedbackPresentation&
		Gesture,
	const Fdemo_mapShanmenThrownWeaponTerminalFeedbackPresentation& Terminal,
	FStack& OutStack)
{
	return TryCompose(
		Trajectory,
		Arc,
		ArcInput,
		Gesture,
		Terminal,
		Fdemo_mapShanmenThrownWeaponLaunchRejectionPresentation(),
		OutStack);
}

bool FStack::TryCompose(
	const Fdemo_mapShanmenThrownWeaponTrajectoryPresentation& Trajectory,
	const Fdemo_mapShanmenThrownWeaponArcEditingPresentation& Arc,
	const Fdemo_mapShanmenThrownWeaponArcEditingInputHintPresentation& ArcInput,
	const Fdemo_mapShanmenThrownWeaponArcPreLaunchGestureFeedbackPresentation&
		Gesture,
	const Fdemo_mapShanmenThrownWeaponTerminalFeedbackPresentation& Terminal,
	const Fdemo_mapShanmenThrownWeaponLaunchRejectionPresentation&
		LaunchRejection,
	FStack& OutStack)
{
	OutStack = FStack();
	const bool bHasTrajectory = Trajectory.IsValid();
	const bool bHasArc = Arc.IsValid();
	const bool bHasArcInput = ArcInput.IsValid();
	const bool bHasGesture = Gesture.IsValid();
	const bool bHasTerminal = Terminal.IsValid();
	const bool bHasLaunchRejection = LaunchRejection.IsValid();
	if ((!bHasTrajectory && !bHasArc)
		|| (bHasArcInput && !bHasArc)
		|| (bHasGesture && !bHasArc)
		|| (bHasTerminal && bHasLaunchRejection))
	{
		return false;
	}

	if (bHasTrajectory && !Trajectory.IsBallisticArc())
	{
		if (bHasArc || bHasArcInput || bHasGesture)
		{
			return false;
		}
		FStack Candidate;
		Candidate.Mode = EMode::Straight;
		if (!TryAppendLine(
				EKind::TrajectoryMode,
				ETone::StraightMode,
				Trajectory.GetDisplayText(),
				Candidate))
		{
			return false;
		}
		if (bHasTerminal
			&& !TryAppendLine(
				EKind::TerminalFeedback,
				ProjectTerminalTone(Terminal),
				Terminal.GetDisplayText(),
				Candidate))
		{
			return false;
		}
		if (bHasLaunchRejection
			&& !TryAppendLine(
				EKind::LaunchRejection,
				ETone::Blocked,
				LaunchRejection.GetDisplayText(),
				Candidate))
		{
			return false;
		}
		if (!Candidate.IsValid())
		{
			return false;
		}
		OutStack = MoveTemp(Candidate);
		return true;
	}

	FStack Candidate;
	Candidate.Mode = EMode::BallisticArc;
	Candidate.bHasArcPresentation = bHasArc;
	Candidate.bHasArcTargetIntent =
		bHasArc && Arc.HasArcTargetIntent();
	if (bHasTrajectory
		&& !TryAppendLine(
			EKind::TrajectoryMode,
			ETone::ArcMode,
			Trajectory.GetDisplayText(),
			Candidate))
	{
		return false;
	}
	if (bHasArc
		&& (!TryAppendLine(
				EKind::ArcApex,
				ETone::ArcApex,
				Arc.GetApexDisplayText(),
				Candidate)
			|| !TryAppendLine(
				EKind::ArcTarget,
				ETone::ArcTarget,
				Arc.GetTargetDisplayText(),
				Candidate)))
	{
		return false;
	}
	if (bHasArcInput)
	{
		Fdemo_mapShanmenThrownWeaponArcEditingInputHintPresentation Expected;
		if (!Fdemo_mapShanmenThrownWeaponArcEditingInputHintPresentation::
				TryProject(
					Arc,
					ArcInput.GetTargetKeyLabel(),
					ArcInput.GetApexIncreaseKeyLabel(),
					ArcInput.GetApexDecreaseKeyLabel(),
					ArcInput.GetClearKeyLabel(),
					Expected)
			|| Expected.GetDisplayText() != ArcInput.GetDisplayText()
			|| !TryAppendLine(
				EKind::ArcInput,
				ETone::ArcInput,
				ArcInput.GetDisplayText(),
				Candidate))
		{
			return false;
		}
	}
	if (bHasGesture)
	{
		if (Gesture.NeedsArcTarget() == Candidate.bHasArcTargetIntent
			|| Gesture.IsReadyToConfirm()
				!= Candidate.bHasArcTargetIntent
			|| !TryAppendLine(
				EKind::ArcPreLaunchGesture,
				Gesture.IsReadyToConfirm()
					? ETone::ReadyToConfirm
					: ETone::TargetRequired,
				Gesture.GetDisplayText(),
				Candidate))
		{
			return false;
		}
	}
	if (bHasTerminal
		&& !TryAppendLine(
			EKind::TerminalFeedback,
			ProjectTerminalTone(Terminal),
			Terminal.GetDisplayText(),
			Candidate))
	{
		return false;
	}
	if (bHasLaunchRejection
		&& !TryAppendLine(
			EKind::LaunchRejection,
			ETone::Blocked,
			LaunchRejection.GetDisplayText(),
			Candidate))
	{
		return false;
	}
	if (!Candidate.IsValid())
	{
		return false;
	}

	OutStack = MoveTemp(Candidate);
	return true;
}

bool FStack::TryAppendLine(
	const EKind Kind,
	const ETone Tone,
	const FString& DisplayText,
	FStack& Stack)
{
	FLine Line;
	Line.Kind = Kind;
	Line.Tone = Tone;
	Line.DisplayText = DisplayText;
	if (!Line.IsValid())
	{
		return false;
	}
	Stack.Lines.Add(MoveTemp(Line));
	return true;
}

bool FStack::IsValid() const
{
	if ((Mode != EMode::Straight && Mode != EMode::BallisticArc)
		|| Lines.IsEmpty() || Lines.Num() > 6)
	{
		return false;
	}

	bool bHasTrajectory = false;
	bool bHasApex = false;
	bool bHasTarget = false;
	bool bHasInput = false;
	bool bHasGesture = false;
	bool bHasTerminal = false;
	bool bHasLaunchRejection = false;
	ETone TrajectoryTone = ETone::Invalid;
	ETone GestureTone = ETone::Invalid;
	int32 PreviousRank = INDEX_NONE;
	for (const FLine& Line : Lines)
	{
		if (!Line.IsValid())
		{
			return false;
		}
		const int32 Rank = GetCanonicalRank(Line.GetKind());
		if (Rank <= PreviousRank)
		{
			return false;
		}
		PreviousRank = Rank;
		switch (Line.GetKind())
		{
		case EKind::TrajectoryMode:
			bHasTrajectory = true;
			TrajectoryTone = Line.GetTone();
			break;
		case EKind::ArcApex:
			bHasApex = true;
			break;
		case EKind::ArcTarget:
			bHasTarget = true;
			break;
		case EKind::ArcInput:
			bHasInput = true;
			break;
		case EKind::ArcPreLaunchGesture:
			bHasGesture = true;
			GestureTone = Line.GetTone();
			break;
		case EKind::TerminalFeedback:
			bHasTerminal = true;
			break;
		case EKind::LaunchRejection:
			bHasLaunchRejection = true;
			break;
		default:
			return false;
		}
	}

	if (bHasTerminal && bHasLaunchRejection)
	{
		return false;
	}
	const bool bHasOutcome = bHasTerminal || bHasLaunchRejection;
	if (Mode == EMode::Straight)
	{
		return Lines.Num() == (bHasOutcome ? 2 : 1)
			&& bHasTrajectory
			&& TrajectoryTone == ETone::StraightMode
			&& !bHasArcPresentation
			&& !bHasArcTargetIntent;
	}

	const bool bHasArcPair = bHasApex && bHasTarget;
	if (bHasApex != bHasTarget
		|| bHasArcPresentation != bHasArcPair
		|| (!bHasArcPresentation && bHasArcTargetIntent)
		|| (bHasTrajectory && TrajectoryTone != ETone::ArcMode)
		|| (bHasInput && !bHasArcPair)
		|| (bHasGesture && !bHasArcPair))
	{
		return false;
	}
	if (bHasGesture)
	{
		const ETone ExpectedTone = bHasArcTargetIntent
			? ETone::ReadyToConfirm
			: ETone::TargetRequired;
		if (GestureTone != ExpectedTone)
		{
			return false;
		}
	}
	return bHasTrajectory || bHasArcPair;
}

bool FStack::Matches(const FStack& Other) const
{
	if (!IsValid() || !Other.IsValid()
		|| Mode != Other.Mode
		|| bHasArcPresentation != Other.bHasArcPresentation
		|| bHasArcTargetIntent != Other.bHasArcTargetIntent
		|| Lines.Num() != Other.Lines.Num())
	{
		return false;
	}
	for (int32 Index = 0; Index < Lines.Num(); ++Index)
	{
		if (!Lines[Index].Matches(Other.Lines[Index]))
		{
			return false;
		}
	}
	return true;
}

bool FStack::IsBallisticArc() const
{
	return IsValid() && Mode == EMode::BallisticArc;
}
