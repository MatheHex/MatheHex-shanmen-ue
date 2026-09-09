#include "demo_mapShanmenDivineSenseHUDPresentation.h"

namespace
{
	using EPlacement =
		Edemo_mapShanmenDivineSenseMarkerPlacement;
	using FPlan = Fdemo_mapShanmenDivineSenseHUDMarkerPlan;

	constexpr double MinimumCanvasWidth = 320.0;
	constexpr double MinimumCanvasHeight = 240.0;
	constexpr double DirectionToleranceSquared = 1.0e-8;
	constexpr double UnitTolerance = 1.0e-6;
	constexpr double EdgeTolerance = 1.0e-4;

	bool IsFiniteVector(const FVector2D& Value)
	{
		return FMath::IsFinite(Value.X) && FMath::IsFinite(Value.Y);
	}

	bool IsInsideSafeArea(
		const FVector2D& Position,
		const FVector2D& CanvasSize)
	{
		return Position.X >= FPlan::GetHorizontalSafeMargin()
			&& Position.X
				<= CanvasSize.X - FPlan::GetHorizontalSafeMargin()
			&& Position.Y >= FPlan::GetTopSafeMargin()
			&& Position.Y
				<= CanvasSize.Y - FPlan::GetBottomSafeMargin();
	}

	bool IsOnSafeAreaEdge(
		const FVector2D& Position,
		const FVector2D& CanvasSize)
	{
		return FMath::IsNearlyEqual(
				Position.X,
				FPlan::GetHorizontalSafeMargin(),
				EdgeTolerance)
			|| FMath::IsNearlyEqual(
				Position.X,
				CanvasSize.X - FPlan::GetHorizontalSafeMargin(),
				EdgeTolerance)
			|| FMath::IsNearlyEqual(
				Position.Y,
				FPlan::GetTopSafeMargin(),
				EdgeTolerance)
			|| FMath::IsNearlyEqual(
				Position.Y,
				CanvasSize.Y - FPlan::GetBottomSafeMargin(),
				EdgeTolerance);
	}
}

bool FPlan::TryPlan(
	const FVector2D& InCanvasSize,
	const bool bWorldProjectionSucceeded,
	const FVector2D& ProjectedScreenPosition,
	const FVector2D& CameraRelativeFallbackBearing,
	FPlan& OutPlan)
{
	OutPlan = FPlan();
	if (!IsFiniteVector(InCanvasSize)
		|| InCanvasSize.X < MinimumCanvasWidth
		|| InCanvasSize.Y < MinimumCanvasHeight)
	{
		return false;
	}

	const bool bHasFiniteProjection =
		IsFiniteVector(ProjectedScreenPosition);
	if (bWorldProjectionSucceeded && bHasFiniteProjection
		&& IsInsideSafeArea(ProjectedScreenPosition, InCanvasSize))
	{
		FPlan Candidate;
		Candidate.Placement = EPlacement::WorldProjected;
		Candidate.CanvasSize = InCanvasSize;
		Candidate.ScreenPosition = ProjectedScreenPosition;
		if (!Candidate.IsValid())
		{
			return false;
		}
		OutPlan = Candidate;
		return true;
	}

	const FVector2D CanvasCenter = InCanvasSize * 0.5;
	FVector2D Direction =
		bWorldProjectionSucceeded && bHasFiniteProjection
			? ProjectedScreenPosition - CanvasCenter
			: CameraRelativeFallbackBearing;
	if (!IsFiniteVector(Direction)
		|| Direction.SizeSquared() <= DirectionToleranceSquared)
	{
		return false;
	}
	Direction.Normalize();

	const double AvailableX = Direction.X >= 0.0
		? InCanvasSize.X - GetHorizontalSafeMargin() - CanvasCenter.X
		: CanvasCenter.X - GetHorizontalSafeMargin();
	const double AvailableY = Direction.Y >= 0.0
		? InCanvasSize.Y - GetBottomSafeMargin() - CanvasCenter.Y
		: CanvasCenter.Y - GetTopSafeMargin();
	double EdgeScale = TNumericLimits<double>::Max();
	if (!FMath::IsNearlyZero(Direction.X))
	{
		EdgeScale = FMath::Min(
			EdgeScale,
			AvailableX / FMath::Abs(Direction.X));
	}
	if (!FMath::IsNearlyZero(Direction.Y))
	{
		EdgeScale = FMath::Min(
			EdgeScale,
			AvailableY / FMath::Abs(Direction.Y));
	}
	if (!FMath::IsFinite(EdgeScale) || EdgeScale <= 0.0)
	{
		return false;
	}

	FPlan Candidate;
	Candidate.Placement = EPlacement::ScreenEdge;
	Candidate.CanvasSize = InCanvasSize;
	Candidate.ScreenPosition = CanvasCenter + Direction * EdgeScale;
	Candidate.ScreenPosition.X = FMath::Clamp(
		Candidate.ScreenPosition.X,
		GetHorizontalSafeMargin(),
		InCanvasSize.X - GetHorizontalSafeMargin());
	Candidate.ScreenPosition.Y = FMath::Clamp(
		Candidate.ScreenPosition.Y,
		GetTopSafeMargin(),
		InCanvasSize.Y - GetBottomSafeMargin());
	Candidate.EdgeDirection = Direction;
	if (!Candidate.IsValid())
	{
		return false;
	}

	OutPlan = Candidate;
	return true;
}

bool FPlan::IsValid() const
{
	if (!IsFiniteVector(CanvasSize)
		|| CanvasSize.X < MinimumCanvasWidth
		|| CanvasSize.Y < MinimumCanvasHeight
		|| !IsFiniteVector(ScreenPosition)
		|| !IsFiniteVector(EdgeDirection)
		|| !IsInsideSafeArea(ScreenPosition, CanvasSize))
	{
		return false;
	}

	if (Placement == EPlacement::WorldProjected)
	{
		return EdgeDirection.IsZero();
	}
	if (Placement != EPlacement::ScreenEdge
		|| !IsOnSafeAreaEdge(ScreenPosition, CanvasSize))
	{
		return false;
	}
	return FMath::IsNearlyEqual(
		EdgeDirection.SizeSquared(), 1.0, UnitTolerance);
}

bool FPlan::Matches(const FPlan& Other) const
{
	return IsValid() && Other.IsValid()
		&& Placement == Other.Placement
		&& CanvasSize == Other.CanvasSize
		&& ScreenPosition == Other.ScreenPosition
		&& EdgeDirection == Other.EdgeDirection;
}
