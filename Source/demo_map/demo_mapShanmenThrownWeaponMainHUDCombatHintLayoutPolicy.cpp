#include "demo_mapShanmenThrownWeaponMainHUDCombatHintLayoutPolicy.h"

namespace
{
	using EMode =
		Edemo_mapShanmenThrownWeaponMainHUDCombatHintLayoutMode;
	using FPlan =
		Fdemo_mapShanmenThrownWeaponMainHUDCombatHintLayoutPlan;

	constexpr double MinimumCanvasWidth = 640.0;
	constexpr double StandardCanvasWidth = 960.0;
	constexpr double MinimumTopMargin = 24.0;
	constexpr double StandardLeftMargin = 28.0;
	constexpr double CompactLeftMargin = 16.0;
	constexpr double CanonicalBottomAnchor = 112.0;
	constexpr double PreferredLineSpacing = 22.5;
	constexpr double MinimumLineSpacing = 16.0;
	constexpr double StandardScaleMultiplier = 1.0;
	constexpr double CompactScaleMultiplier = 0.86;

	bool IsFiniteCanvas(const FVector2D& Size)
	{
		return FMath::IsFinite(Size.X) && FMath::IsFinite(Size.Y);
	}
}

bool FPlan::TryPlan(
	const FVector2D& InCanvasSize,
	const int32 InLineCount,
	FPlan& OutPlan)
{
	OutPlan = FPlan();
	if (!IsFiniteCanvas(InCanvasSize)
		|| InCanvasSize.X < MinimumCanvasWidth
		|| InLineCount < 1 || InLineCount > MaximumLineCount)
	{
		return false;
	}

	const double AvailableSpan =
		InCanvasSize.Y - CanonicalBottomAnchor - MinimumTopMargin;
	if (AvailableSpan < 0.0)
	{
		return false;
	}
	const double Spacing = InLineCount == 1
		? PreferredLineSpacing
		: FMath::Min(
			PreferredLineSpacing,
			AvailableSpan / static_cast<double>(InLineCount - 1));
	if (Spacing < MinimumLineSpacing)
	{
		return false;
	}

	const bool bCompact = InCanvasSize.X < StandardCanvasWidth
		|| Spacing < PreferredLineSpacing;
	FPlan Candidate;
	Candidate.Mode = bCompact ? EMode::Compact : EMode::Standard;
	Candidate.CanvasSize = InCanvasSize;
	Candidate.LineCount = InLineCount;
	Candidate.LeftMargin = bCompact
		? CompactLeftMargin : StandardLeftMargin;
	Candidate.BottomAnchor = CanonicalBottomAnchor;
	Candidate.LineSpacing = Spacing;
	Candidate.ScaleMultiplier = bCompact
		? CompactScaleMultiplier : StandardScaleMultiplier;
	if (!Candidate.IsValid())
	{
		return false;
	}

	OutPlan = Candidate;
	return true;
}

bool FPlan::IsValid() const
{
	if ((Mode != EMode::Standard && Mode != EMode::Compact)
		|| !IsFiniteCanvas(CanvasSize)
		|| CanvasSize.X < MinimumCanvasWidth
		|| LineCount < 1 || LineCount > MaximumLineCount
		|| BottomAnchor != CanonicalBottomAnchor
		|| LineSpacing < MinimumLineSpacing
		|| LineSpacing > PreferredLineSpacing)
	{
		return false;
	}
	const double TopLineY = CanvasSize.Y - BottomAnchor
		- LineSpacing * static_cast<double>(LineCount - 1);
	if (TopLineY < MinimumTopMargin)
	{
		return false;
	}

	const bool bShouldBeCompact = CanvasSize.X < StandardCanvasWidth
		|| LineSpacing < PreferredLineSpacing;
	if (Mode == EMode::Compact)
	{
		return bShouldBeCompact
			&& LeftMargin == CompactLeftMargin
			&& ScaleMultiplier == CompactScaleMultiplier;
	}
	return !bShouldBeCompact
		&& LeftMargin == StandardLeftMargin
		&& ScaleMultiplier == StandardScaleMultiplier;
}

bool FPlan::Matches(const FPlan& Other) const
{
	return IsValid() && Other.IsValid()
		&& Mode == Other.Mode
		&& CanvasSize == Other.CanvasSize
		&& LineCount == Other.LineCount
		&& LeftMargin == Other.LeftMargin
		&& BottomAnchor == Other.BottomAnchor
		&& LineSpacing == Other.LineSpacing
		&& ScaleMultiplier == Other.ScaleMultiplier;
}

bool FPlan::TryGetLinePosition(
	const int32 LineIndex,
	FVector2D& OutPosition) const
{
	OutPosition = FVector2D::ZeroVector;
	if (!IsValid() || LineIndex < 0 || LineIndex >= LineCount)
	{
		return false;
	}
	OutPosition = FVector2D(
		LeftMargin,
		CanvasSize.Y - BottomAnchor - LineSpacing * LineIndex);
	return true;
}

bool FPlan::IsCompact() const
{
	return IsValid() && Mode == EMode::Compact;
}
