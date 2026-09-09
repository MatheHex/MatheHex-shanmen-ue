#include "demo_mapShanmenDivineSenseHUDPresentation.h"

namespace
{
	using EPlacement =
		Edemo_mapShanmenDivineSenseMarkerPlacement;
	using EFeedbackReason =
		Edemo_mapShanmenDivineSenseHUDFeedbackReason;
	using EFeedbackTone =
		Edemo_mapShanmenDivineSenseHUDFeedbackTone;
	using EInputStatus = Edemo_mapShanmenDivineSenseLogicalInputStatus;
	using FPlan = Fdemo_mapShanmenDivineSenseHUDMarkerPlan;
	using FFeedback =
		Fdemo_mapShanmenDivineSenseHUDFeedbackPresentation;

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

bool FFeedback::TryProject(
	const Fdemo_mapShanmenDivineSenseLogicalInputResult& Result,
	const FString& UseKeyLabel,
	FFeedback& OutPresentation)
{
	OutPresentation = FFeedback();
	if (!Result.IsValid() || Result.IsAccepted())
	{
		return false;
	}

	FFeedback Candidate;
	const FString ReadableKey = UseKeyLabel.TrimStartAndEnd();
	switch (Result.Status)
	{
	case EInputStatus::ProductUnavailable:
	{
		const auto& Availability =
			Result.AvailabilityBefore.GetProductAvailability();
		if (!Availability.IsValid())
		{
			return false;
		}
		const auto& Resource =
			Availability.GetSessionAvailability().GetResourceSnapshot();
		const float RequiredSpirit = Availability.GetConfig().GetCost().GetAmount();
		const float AvailableSpirit = Resource.GetAvailableAmount();
		Candidate.Tone = EFeedbackTone::Warning;
		if (RequiredSpirit > AvailableSpirit)
		{
			Candidate.Reason = EFeedbackReason::InsufficientSpirit;
			Candidate.DisplayText = FString::Printf(
				TEXT("DIVINE SENSE · NEED %.0f SPIRIT · %.0f AVAILABLE"),
				RequiredSpirit,
				AvailableSpirit);
		}
		else if (Availability.GetRemainingIntentCapacity() <= 0
			|| !Availability.GetSessionAvailability().HasRouteCapacity())
		{
			Candidate.Reason = EFeedbackReason::PulseLimitReached;
			Candidate.DisplayText =
				TEXT("DIVINE SENSE · PULSE LIMIT REACHED");
		}
		else
		{
			Candidate.Reason = EFeedbackReason::Unavailable;
			Candidate.DisplayText = TEXT("DIVINE SENSE · UNAVAILABLE");
		}
		break;
	}
	case EInputStatus::RetryRequired:
		Candidate.Reason = EFeedbackReason::RetryRequired;
		Candidate.Tone = EFeedbackTone::Warning;
		Candidate.DisplayText = ReadableKey.IsEmpty()
			? TEXT("DIVINE SENSE INTERRUPTED · TRY AGAIN")
			: FString::Printf(
				TEXT("DIVINE SENSE INTERRUPTED · PRESS [%s] TO %s"),
				*ReadableKey,
				Result.bRetryAttempt ? TEXT("TRY AGAIN") : TEXT("RETRY"));
		break;
	case EInputStatus::Busy:
		Candidate.Reason = EFeedbackReason::Busy;
		Candidate.Tone = EFeedbackTone::Warning;
		Candidate.DisplayText = TEXT("DIVINE SENSE · STABILIZING");
		break;
	case EInputStatus::AdapterInactive:
	case EInputStatus::RetryUnavailable:
		Candidate.Reason = EFeedbackReason::Unavailable;
		Candidate.Tone = EFeedbackTone::Warning;
		Candidate.DisplayText = TEXT("DIVINE SENSE · UNAVAILABLE");
		break;
	case EInputStatus::AdapterInvalid:
	case EInputStatus::BindingMismatch:
	case EInputStatus::ProductRejected:
	case EInputStatus::StateDesynchronized:
		Candidate.Reason = EFeedbackReason::Failed;
		Candidate.Tone = EFeedbackTone::Error;
		Candidate.DisplayText = TEXT("DIVINE SENSE · PULSE FAILED · TRY AGAIN");
		break;
	default:
		return false;
	}

	if (!Candidate.IsValid())
	{
		return false;
	}
	OutPresentation = MoveTemp(Candidate);
	return true;
}

bool FFeedback::IsValid() const
{
	return Reason != EFeedbackReason::Invalid
		&& Tone != EFeedbackTone::Invalid
		&& !DisplayText.TrimStartAndEnd().IsEmpty();
}

bool FFeedback::Matches(const FFeedback& Other) const
{
	return IsValid() && Other.IsValid()
		&& Reason == Other.Reason
		&& Tone == Other.Tone
		&& DisplayText == Other.DisplayText;
}
