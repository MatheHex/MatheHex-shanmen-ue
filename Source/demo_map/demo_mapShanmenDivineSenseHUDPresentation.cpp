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
	using FTacticalSummary =
		Fdemo_mapShanmenDivineSenseHUDTacticalSummary;

	constexpr double MinimumCanvasWidth = 320.0;
	constexpr double MinimumCanvasHeight = 240.0;
	constexpr double DirectionToleranceSquared = 1.0e-8;
	constexpr double UnitTolerance = 1.0e-6;
	constexpr double EdgeTolerance = 1.0e-4;
	constexpr int32 MaximumDeconflictionRings = 8;

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

bool FPlan::TryDeconflict(
	const FPlan& BasePlan,
	const TConstArrayView<FVector2D> OccupiedScreenPositions,
	FPlan& OutPlan)
{
	const FPlan FrozenBasePlan = BasePlan;
	OutPlan = FPlan();
	if (!FrozenBasePlan.IsValid())
	{
		return false;
	}
	for (const FVector2D& OccupiedPosition : OccupiedScreenPositions)
	{
		if (!IsFiniteVector(OccupiedPosition)
			|| !IsInsideSafeArea(
				OccupiedPosition,
				FrozenBasePlan.CanvasSize))
		{
			return false;
		}
	}

	const auto TryCandidate =
		[&](const FVector2D& CandidatePosition) -> bool
		{
			if (!IsInsideSafeArea(CandidatePosition, FrozenBasePlan.CanvasSize))
			{
				return false;
			}
			for (const FVector2D& OccupiedPosition : OccupiedScreenPositions)
			{
				const FVector2D Separation = CandidatePosition - OccupiedPosition;
				if (FMath::Abs(Separation.X)
						< GetMinimumHorizontalMarkerSeparation()
					&& FMath::Abs(Separation.Y)
						< GetMinimumVerticalMarkerSeparation())
				{
					return false;
				}
			}

			FPlan Candidate = FrozenBasePlan;
			Candidate.ScreenPosition = CandidatePosition;
			if (!Candidate.IsValid())
			{
				return false;
			}
			OutPlan = Candidate;
			return true;
		};

	if (TryCandidate(FrozenBasePlan.ScreenPosition))
	{
		return true;
	}

	if (FrozenBasePlan.Placement == EPlacement::ScreenEdge)
	{
		const bool bOnHorizontalEdge = FMath::IsNearlyEqual(
				FrozenBasePlan.ScreenPosition.Y,
				GetTopSafeMargin(),
				EdgeTolerance)
			|| FMath::IsNearlyEqual(
				FrozenBasePlan.ScreenPosition.Y,
				FrozenBasePlan.CanvasSize.Y - GetBottomSafeMargin(),
				EdgeTolerance);
		const FVector2D EdgeTangent = bOnHorizontalEdge
			? FVector2D(1.0, 0.0)
			: FVector2D(0.0, 1.0);
		const double EdgeStep = bOnHorizontalEdge
			? GetMinimumHorizontalMarkerSeparation()
			: GetMinimumVerticalMarkerSeparation();
		for (int32 Ring = 1; Ring <= MaximumDeconflictionRings; ++Ring)
		{
			const FVector2D Offset = EdgeTangent * (EdgeStep * Ring);
			if (TryCandidate(FrozenBasePlan.ScreenPosition - Offset)
				|| TryCandidate(FrozenBasePlan.ScreenPosition + Offset))
			{
				return true;
			}
		}
		return false;
	}

	static const FVector2D Directions[] = {
		FVector2D(0.0, -1.0),
		FVector2D(1.0, 0.0),
		FVector2D(0.0, 1.0),
		FVector2D(-1.0, 0.0),
		FVector2D(-1.0, -1.0),
		FVector2D(1.0, -1.0),
		FVector2D(1.0, 1.0),
		FVector2D(-1.0, 1.0)
	};
	for (int32 Ring = 1; Ring <= MaximumDeconflictionRings; ++Ring)
	{
		for (const FVector2D& Direction : Directions)
		{
			const FVector2D Offset(
				Direction.X * GetMinimumHorizontalMarkerSeparation() * Ring,
				Direction.Y * GetMinimumVerticalMarkerSeparation() * Ring);
			if (TryCandidate(FrozenBasePlan.ScreenPosition + Offset))
			{
				return true;
			}
		}
	}
	return false;
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

bool FTacticalSummary::TryProject(
	const int32 InContactCount,
	const int32 InOccludedContactCount,
	const double InNearestDistanceMeters,
	const float InCurrentSpirit,
	const float InMaximumSpirit,
	FTacticalSummary& OutSummary)
{
	OutSummary = FTacticalSummary();
	if (InContactCount < 0
		|| InOccludedContactCount < 0
		|| InOccludedContactCount > InContactCount
		|| !FMath::IsFinite(InNearestDistanceMeters)
		|| InNearestDistanceMeters < 0.0
		|| (InContactCount == 0
			&& !FMath::IsNearlyZero(InNearestDistanceMeters))
		|| !FMath::IsFinite(InCurrentSpirit)
		|| !FMath::IsFinite(InMaximumSpirit)
		|| InMaximumSpirit <= 0.0f
		|| InCurrentSpirit < 0.0f
		|| InCurrentSpirit > InMaximumSpirit)
	{
		return false;
	}

	FTacticalSummary Candidate;
	Candidate.ContactCount = InContactCount;
	Candidate.OccludedContactCount = InOccludedContactCount;
	Candidate.NearestDistanceMeters = InNearestDistanceMeters;
	Candidate.CurrentSpirit = InCurrentSpirit;
	Candidate.MaximumSpirit = InMaximumSpirit;
	if (InContactCount == 0)
	{
		Candidate.PrimaryText = TEXT("DIVINE SENSE · AREA CLEAR");
		Candidate.SecondaryText = FString::Printf(
			TEXT("SPIRIT %.0f / %.0f"),
			InCurrentSpirit,
			InMaximumSpirit);
	}
	else
	{
		Candidate.PrimaryText = InContactCount == 1
			? FString::Printf(
				TEXT("DIVINE SENSE · 1 CONTACT · %d OCCLUDED"),
				InOccludedContactCount)
			: FString::Printf(
				TEXT("DIVINE SENSE · %d CONTACTS · %d OCCLUDED"),
				InContactCount,
				InOccludedContactCount);
		Candidate.SecondaryText = FString::Printf(
			TEXT("NEAREST %.1fm · SPIRIT %.0f / %.0f"),
			InNearestDistanceMeters,
			InCurrentSpirit,
			InMaximumSpirit);
	}

	if (!Candidate.IsValid())
	{
		return false;
	}
	OutSummary = MoveTemp(Candidate);
	return true;
}

bool FTacticalSummary::IsValid() const
{
	return ContactCount >= 0
		&& OccludedContactCount >= 0
		&& OccludedContactCount <= ContactCount
		&& FMath::IsFinite(NearestDistanceMeters)
		&& NearestDistanceMeters >= 0.0
		&& (ContactCount > 0 || FMath::IsNearlyZero(NearestDistanceMeters))
		&& FMath::IsFinite(CurrentSpirit)
		&& FMath::IsFinite(MaximumSpirit)
		&& MaximumSpirit > 0.0f
		&& CurrentSpirit >= 0.0f
		&& CurrentSpirit <= MaximumSpirit
		&& !PrimaryText.IsEmpty()
		&& !SecondaryText.IsEmpty();
}

bool FTacticalSummary::Matches(const FTacticalSummary& Other) const
{
	return IsValid() && Other.IsValid()
		&& ContactCount == Other.ContactCount
		&& OccludedContactCount == Other.OccludedContactCount
		&& NearestDistanceMeters == Other.NearestDistanceMeters
		&& CurrentSpirit == Other.CurrentSpirit
		&& MaximumSpirit == Other.MaximumSpirit
		&& PrimaryText == Other.PrimaryText
		&& SecondaryText == Other.SecondaryText;
}
