#pragma once

#include "CoreMinimal.h"

enum class Edemo_mapShanmenThrownWeaponMainHUDCombatHintLayoutMode : uint8
{
	Invalid,
	Standard,
	Compact
};

/**
 * Pure viewport layout for one complete P20.65 combat-hint stack.
 *
 * The policy owns no text, stack, Canvas, widget, font, input, World, Actor or
 * mutable UI state. It either places every requested line or fails closed.
 */
class Fdemo_mapShanmenThrownWeaponMainHUDCombatHintLayoutPlan
{
public:
	static constexpr int32 MaximumLineCount = 5;

	static bool TryPlan(
		const FVector2D& CanvasSize,
		int32 LineCount,
		Fdemo_mapShanmenThrownWeaponMainHUDCombatHintLayoutPlan& OutPlan);

	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenThrownWeaponMainHUDCombatHintLayoutPlan& Other)
		const;
	bool TryGetLinePosition(int32 LineIndex, FVector2D& OutPosition) const;
	bool IsCompact() const;
	Edemo_mapShanmenThrownWeaponMainHUDCombatHintLayoutMode GetMode() const
	{
		return Mode;
	}
	const FVector2D& GetCanvasSize() const { return CanvasSize; }
	int32 GetLineCount() const { return LineCount; }
	double GetLeftMargin() const { return LeftMargin; }
	double GetBottomAnchor() const { return BottomAnchor; }
	double GetLineSpacing() const { return LineSpacing; }
	double GetScaleMultiplier() const { return ScaleMultiplier; }

private:
	Edemo_mapShanmenThrownWeaponMainHUDCombatHintLayoutMode Mode =
		Edemo_mapShanmenThrownWeaponMainHUDCombatHintLayoutMode::Invalid;
	FVector2D CanvasSize = FVector2D::ZeroVector;
	int32 LineCount = 0;
	double LeftMargin = 0.0;
	double BottomAnchor = 0.0;
	double LineSpacing = 0.0;
	double ScaleMultiplier = 0.0;
};
