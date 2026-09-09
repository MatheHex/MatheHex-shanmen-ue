#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenThrownWeaponArcEditingInputHintPresentation.h"
#include "demo_mapShanmenThrownWeaponArcPreLaunchGestureFeedbackPresentation.h"
#include "demo_mapShanmenThrownWeaponTerminalFeedbackPresentation.h"
#include "demo_mapShanmenThrownWeaponTrajectoryPresentation.h"

enum class Edemo_mapShanmenThrownWeaponMainHUDCombatHintStackMode : uint8
{
	Invalid,
	Straight,
	BallisticArc
};

enum class Edemo_mapShanmenThrownWeaponMainHUDCombatHintKind : uint8
{
	Invalid,
	TrajectoryMode,
	ArcApex,
	ArcTarget,
	ArcInput,
	ArcPreLaunchGesture,
	TerminalFeedback
};

enum class Edemo_mapShanmenThrownWeaponMainHUDCombatHintTone : uint8
{
	Invalid,
	StraightMode,
	ArcMode,
	ArcApex,
	ArcTarget,
	ArcInput,
	TargetRequired,
	ReadyToConfirm,
	Impact,
	Defeat,
	NoDamage,
	Blocked,
	Expired,
	Interrupted
};

/** One immutable, renderer-neutral line in the MainHUD combat hint stack. */
class Fdemo_mapShanmenThrownWeaponMainHUDCombatHintLine
{
public:
	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenThrownWeaponMainHUDCombatHintLine& Other) const;
	Edemo_mapShanmenThrownWeaponMainHUDCombatHintKind GetKind() const
	{
		return Kind;
	}
	Edemo_mapShanmenThrownWeaponMainHUDCombatHintTone GetTone() const
	{
		return Tone;
	}
	const FString& GetDisplayText() const { return DisplayText; }

private:
	friend class Fdemo_mapShanmenThrownWeaponMainHUDCombatHintStackPresentation;

	Edemo_mapShanmenThrownWeaponMainHUDCombatHintKind Kind =
		Edemo_mapShanmenThrownWeaponMainHUDCombatHintKind::Invalid;
	Edemo_mapShanmenThrownWeaponMainHUDCombatHintTone Tone =
		Edemo_mapShanmenThrownWeaponMainHUDCombatHintTone::Invalid;
	FString DisplayText;
};

/**
 * Immutable composition of existing thrown-weapon MainHUD projections.
 *
 * Lines are ordered bottom-to-top: trajectory, apex, target, input, gesture.
 * The composition copies presentation text only. It owns no choice, context,
 * preview, input binding, renderer, World, Actor, widget or mutable UI state.
 */
class Fdemo_mapShanmenThrownWeaponMainHUDCombatHintStackPresentation
{
public:
	static bool TryCompose(
		const Fdemo_mapShanmenThrownWeaponTrajectoryPresentation& Trajectory,
		const Fdemo_mapShanmenThrownWeaponArcEditingPresentation& Arc,
		const Fdemo_mapShanmenThrownWeaponArcEditingInputHintPresentation&
			ArcInput,
		const Fdemo_mapShanmenThrownWeaponArcPreLaunchGestureFeedbackPresentation&
			Gesture,
		Fdemo_mapShanmenThrownWeaponMainHUDCombatHintStackPresentation& OutStack);
	static bool TryCompose(
		const Fdemo_mapShanmenThrownWeaponTrajectoryPresentation& Trajectory,
		const Fdemo_mapShanmenThrownWeaponArcEditingPresentation& Arc,
		const Fdemo_mapShanmenThrownWeaponArcEditingInputHintPresentation&
			ArcInput,
		const Fdemo_mapShanmenThrownWeaponArcPreLaunchGestureFeedbackPresentation&
			Gesture,
		const Fdemo_mapShanmenThrownWeaponTerminalFeedbackPresentation& Terminal,
		Fdemo_mapShanmenThrownWeaponMainHUDCombatHintStackPresentation& OutStack);

	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenThrownWeaponMainHUDCombatHintStackPresentation&
			Other) const;
	bool IsBallisticArc() const;
	bool HasArcPresentation() const { return bHasArcPresentation; }
	bool HasArcTargetIntent() const
	{
		return bHasArcPresentation && bHasArcTargetIntent;
	}
	Edemo_mapShanmenThrownWeaponMainHUDCombatHintStackMode GetMode() const
	{
		return Mode;
	}
	const TArray<Fdemo_mapShanmenThrownWeaponMainHUDCombatHintLine>& GetLines()
		const
	{
		return Lines;
	}
	int32 NumLines() const { return Lines.Num(); }

private:
	static bool TryAppendLine(
		Edemo_mapShanmenThrownWeaponMainHUDCombatHintKind Kind,
		Edemo_mapShanmenThrownWeaponMainHUDCombatHintTone Tone,
		const FString& DisplayText,
		Fdemo_mapShanmenThrownWeaponMainHUDCombatHintStackPresentation& Stack);

	Edemo_mapShanmenThrownWeaponMainHUDCombatHintStackMode Mode =
		Edemo_mapShanmenThrownWeaponMainHUDCombatHintStackMode::Invalid;
	bool bHasArcPresentation = false;
	bool bHasArcTargetIntent = false;
	TArray<Fdemo_mapShanmenThrownWeaponMainHUDCombatHintLine> Lines;
};
