#include "demo_mapShanmenControlledWeaponThreatReadoutPresentation.h"

namespace
{
	using EPlacement =
		Edemo_mapShanmenControlledWeaponThreatReadoutPlacement;
	using FPlan =
		Fdemo_mapShanmenControlledWeaponThreatReadoutPlan;
	using FStyle =
		Fdemo_mapShanmenControlledWeaponThreatReadoutStyle;
	using EFlightPhase =
		Edemo_mapShanmenControlledWeaponFlightPhase;

	bool IsFiniteVector(const FVector2D& Value)
	{
		return FMath::IsFinite(Value.X) && FMath::IsFinite(Value.Y);
	}

	bool IsFiniteColor(const FLinearColor& Value)
	{
		return FMath::IsFinite(Value.R)
			&& FMath::IsFinite(Value.G)
			&& FMath::IsFinite(Value.B)
			&& FMath::IsFinite(Value.A);
	}

	bool IsVisiblePhase(const EFlightPhase Phase)
	{
		return Phase == EFlightPhase::Orbiting
			|| Phase == EFlightPhase::Directed
			|| Phase == EFlightPhase::Returning
			|| Phase == EFlightPhase::Redeployed;
	}

	FString BuildText(
		const EPlacement Placement,
		const EFlightPhase Phase,
		const int32 ContactCount)
	{
		FString Text;
		switch (Phase)
		{
		case EFlightPhase::Orbiting:
			Text = TEXT("飞剑 · 环绕待命");
			break;
		case EFlightPhase::Directed:
			Text = TEXT("飞剑 · 御剑出击");
			break;
		case EFlightPhase::Returning:
			Text = TEXT("飞剑 · 返航");
			break;
		case EFlightPhase::Redeployed:
			Text = TEXT("飞剑 · 已归位");
			break;
		default:
			return FString();
		}

		if (Placement == EPlacement::ViewportFallback)
		{
			Text += TEXT(" · 屏外");
		}
		else if (Placement != EPlacement::WorldTracked)
		{
			return FString();
		}
		if (ContactCount > 0)
		{
			Text += FString::Printf(
				TEXT(" · 近身目标 %d"), ContactCount);
		}
		return Text;
	}

	bool IsInsideViewport(
		const FVector2D& CanvasSize,
		const FVector2D& Position)
	{
		return IsFiniteVector(Position)
			&& Position.X >= 0.0 && Position.X <= CanvasSize.X
			&& Position.Y >= 0.0 && Position.Y <= CanvasSize.Y;
	}
}

bool FStyle::IsValid() const
{
	return IsFiniteVector(PanelSize)
		&& IsFiniteVector(TextInset)
		&& PanelSize.X > 0.0 && PanelSize.Y > 0.0
		&& TextInset.X >= 0.0 && TextInset.Y >= 0.0
		&& FMath::IsFinite(WorldVerticalLift) && WorldVerticalLift >= 0.0
		&& FMath::IsFinite(HorizontalMargin) && HorizontalMargin >= 0.0
		&& FMath::IsFinite(TopMargin) && TopMargin >= 0.0
		&& FMath::IsFinite(BottomMargin) && BottomMargin >= 0.0
		&& FMath::IsFinite(TextScale) && TextScale > 0.0
		&& IsFiniteColor(PanelColor)
		&& IsFiniteColor(TextColor)
		&& TextInset.X < PanelSize.X
		&& TextInset.Y < PanelSize.Y;
}

bool FPlan::TryPlan(
	const FVector2D& InCanvasSize,
	const EFlightPhase InPhase,
	const int32 InContactCount,
	const bool bProjectionSucceeded,
	const FVector2D& ProjectedScreenPosition,
	const FStyle& InStyle,
	FPlan& OutPlan)
{
	OutPlan = FPlan();
	if (!IsFiniteVector(InCanvasSize)
		|| InCanvasSize.X <= 0.0 || InCanvasSize.Y <= 0.0
		|| !IsVisiblePhase(InPhase)
		|| InContactCount < 0 || !InStyle.IsValid())
	{
		return false;
	}

	const double MinimumPanelX = InStyle.HorizontalMargin;
	const double MaximumPanelX = InCanvasSize.X
		- InStyle.HorizontalMargin - InStyle.PanelSize.X;
	const double MinimumPanelY = InStyle.TopMargin;
	const double MaximumPanelY = InCanvasSize.Y
		- InStyle.BottomMargin - InStyle.PanelSize.Y;
	if (MaximumPanelX < MinimumPanelX || MaximumPanelY < MinimumPanelY)
	{
		return false;
	}

	FPlan Candidate;
	Candidate.CanvasSize = InCanvasSize;
	Candidate.Phase = InPhase;
	Candidate.ContactCount = InContactCount;
	Candidate.Style = InStyle;
	const bool bWorldTracked = bProjectionSucceeded
		&& IsInsideViewport(InCanvasSize, ProjectedScreenPosition);
	Candidate.Placement = bWorldTracked
		? EPlacement::WorldTracked
		: EPlacement::ViewportFallback;
	Candidate.PanelPosition = bWorldTracked
		? FVector2D(
			FMath::Clamp(
				ProjectedScreenPosition.X - InStyle.PanelSize.X * 0.5,
				MinimumPanelX,
				MaximumPanelX),
			FMath::Clamp(
				ProjectedScreenPosition.Y - InStyle.WorldVerticalLift,
				MinimumPanelY,
				MaximumPanelY))
		: FVector2D(
			FMath::Clamp(
				(InCanvasSize.X - InStyle.PanelSize.X) * 0.5,
				MinimumPanelX,
				MaximumPanelX),
			MinimumPanelY);
	Candidate.Text = BuildText(
		Candidate.Placement, InPhase, InContactCount);
	if (!Candidate.IsValid())
	{
		return false;
	}

	OutPlan = Candidate;
	return true;
}

bool FPlan::IsValid() const
{
	if ((Placement != EPlacement::WorldTracked
			&& Placement != EPlacement::ViewportFallback)
		|| !IsFiniteVector(CanvasSize)
		|| CanvasSize.X <= 0.0 || CanvasSize.Y <= 0.0
		|| !IsVisiblePhase(Phase)
		|| ContactCount < 0 || !Style.IsValid()
		|| !IsFiniteVector(PanelPosition)
		|| Text != BuildText(Placement, Phase, ContactCount))
	{
		return false;
	}

	const double MaximumPanelX = CanvasSize.X
		- Style.HorizontalMargin - Style.PanelSize.X;
	const double MaximumPanelY = CanvasSize.Y
		- Style.BottomMargin - Style.PanelSize.Y;
	return PanelPosition.X >= Style.HorizontalMargin
		&& PanelPosition.X <= MaximumPanelX
		&& PanelPosition.Y >= Style.TopMargin
		&& PanelPosition.Y <= MaximumPanelY;
}

bool FPlan::Matches(const FPlan& Other) const
{
	return IsValid() && Other.IsValid()
		&& Placement == Other.Placement
		&& CanvasSize == Other.CanvasSize
		&& PanelPosition == Other.PanelPosition
		&& Phase == Other.Phase
		&& ContactCount == Other.ContactCount
		&& Text == Other.Text
		&& Style.PanelSize == Other.Style.PanelSize
		&& Style.TextInset == Other.Style.TextInset
		&& Style.WorldVerticalLift == Other.Style.WorldVerticalLift
		&& Style.HorizontalMargin == Other.Style.HorizontalMargin
		&& Style.TopMargin == Other.Style.TopMargin
		&& Style.BottomMargin == Other.Style.BottomMargin
		&& Style.TextScale == Other.Style.TextScale
		&& Style.PanelColor == Other.Style.PanelColor
		&& Style.TextColor == Other.Style.TextColor;
}
