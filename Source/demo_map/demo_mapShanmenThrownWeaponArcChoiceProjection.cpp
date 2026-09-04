#include "demo_mapShanmenThrownWeaponArcChoiceProjection.h"

#include "ShanmenDeterministicId.h"

namespace
{
	constexpr double DirectionMagnitudeToleranceSquared = 1.0e-12;
	constexpr double UnitLengthTolerance = 1.0e-6;
	constexpr double OrthogonalityTolerance = 1.0e-6;

	double CanonicalZero(const double Value)
	{
		return Value == 0.0 ? 0.0 : Value;
	}

	FVector CanonicalVector(FVector Value)
	{
		Value.X = CanonicalZero(Value.X);
		Value.Y = CanonicalZero(Value.Y);
		Value.Z = CanonicalZero(Value.Z);
		return Value;
	}

	bool IsFiniteVector(const FVector& Value)
	{
		return FMath::IsFinite(Value.X)
			&& FMath::IsFinite(Value.Y)
			&& FMath::IsFinite(Value.Z);
	}

	FString DoubleBits(double Value)
	{
		Value = CanonicalZero(Value);
		uint64 Bits = 0;
		static_assert(sizeof(Bits) == sizeof(Value));
		FMemory::Memcpy(&Bits, &Value, sizeof(Bits));
		return FString::Printf(TEXT("%016llX"), Bits);
	}

	bool TryNormalizeDirection(const FVector& Raw, FVector& OutDirection)
	{
		OutDirection = FVector::ZeroVector;
		if (!IsFiniteVector(Raw))
		{
			return false;
		}
		const double MagnitudeSquared = Raw.SizeSquared();
		if (!FMath::IsFinite(MagnitudeSquared)
			|| MagnitudeSquared <= DirectionMagnitudeToleranceSquared)
		{
			return false;
		}
		OutDirection = CanonicalVector(Raw / FMath::Sqrt(MagnitudeSquared));
		return IsFiniteVector(OutDirection)
			&& FMath::IsNearlyEqual(
				OutDirection.SizeSquared(), 1.0, UnitLengthTolerance);
	}

	bool HasBasisShape(
		const FVector& Origin,
		const FVector& Forward,
		const FVector& Right)
	{
		return IsFiniteVector(Origin)
			&& IsFiniteVector(Forward)
			&& IsFiniteVector(Right)
			&& FMath::IsNearlyEqual(
				Forward.SizeSquared(), 1.0, UnitLengthTolerance)
			&& FMath::IsNearlyEqual(
				Right.SizeSquared(), 1.0, UnitLengthTolerance)
			&& FMath::Abs(FVector::DotProduct(Forward, Right))
				<= OrthogonalityTolerance;
	}

	FGuid MakeBasisId(
		const FVector& Origin,
		const FVector& Forward,
		const FVector& Right)
	{
		if (!HasBasisShape(Origin, Forward, Right))
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			FName(TEXT("demo_map.ShanmenThrownWeapon.ArcChoiceBasis.r1")),
			{
				DoubleBits(Origin.X), DoubleBits(Origin.Y),
				DoubleBits(Origin.Z), DoubleBits(Forward.X),
				DoubleBits(Forward.Y), DoubleBits(Forward.Z),
				DoubleBits(Right.X), DoubleBits(Right.Y),
				DoubleBits(Right.Z)
			});
	}

	bool HasPolicyShape(
		const double MinimumForwardDistance,
		const double MaximumForwardDistance,
		const double MaximumLateralOffset,
		const double MinimumApexClearance,
		const double MaximumApexClearance)
	{
		return FMath::IsFinite(MinimumForwardDistance)
			&& FMath::IsFinite(MaximumForwardDistance)
			&& FMath::IsFinite(MaximumLateralOffset)
			&& FMath::IsFinite(MinimumApexClearance)
			&& FMath::IsFinite(MaximumApexClearance)
			&& MinimumForwardDistance > 0.0
			&& MaximumForwardDistance >= MinimumForwardDistance
			&& MaximumLateralOffset >= 0.0
			&& MinimumApexClearance > 0.0
			&& MaximumApexClearance >= MinimumApexClearance;
	}

	FGuid MakePolicyId(
		const double MinimumForwardDistance,
		const double MaximumForwardDistance,
		const double MaximumLateralOffset,
		const double MinimumApexClearance,
		const double MaximumApexClearance)
	{
		if (!HasPolicyShape(
				MinimumForwardDistance,
				MaximumForwardDistance,
				MaximumLateralOffset,
				MinimumApexClearance,
				MaximumApexClearance))
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			FName(TEXT("demo_map.ShanmenThrownWeapon.ArcChoicePolicy.r1")),
			{
				DoubleBits(MinimumForwardDistance),
				DoubleBits(MaximumForwardDistance),
				DoubleBits(MaximumLateralOffset),
				DoubleBits(MinimumApexClearance),
				DoubleBits(MaximumApexClearance)
			});
	}

	FGuid MakeProjectionId(
		const Fdemo_mapShanmenThrownWeaponArcChoiceProjectionResult& Result)
	{
		if (!Result.GetChoiceStateId().IsValid()
			|| Result.GetChoiceRevision() == MAX_uint64
			|| !Result.GetBasisId().IsValid()
			|| !Result.GetPolicyId().IsValid()
			|| !IsFiniteVector(Result.GetTarget())
			|| !FMath::IsFinite(Result.GetApexClearance())
			|| Result.GetApexClearance() <= 0.0
			|| !FMath::IsFinite(Result.GetForwardDistance())
			|| Result.GetForwardDistance() <= 0.0
			|| !FMath::IsFinite(Result.GetLateralOffset()))
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			FName(TEXT("demo_map.ShanmenThrownWeapon.ArcChoiceProjection.r1")),
			{
				Result.GetChoiceStateId().ToString(EGuidFormats::Digits),
				FString::Printf(TEXT("%llu"), Result.GetChoiceRevision()),
				Result.GetBasisId().ToString(EGuidFormats::Digits),
				Result.GetPolicyId().ToString(EGuidFormats::Digits),
				DoubleBits(Result.GetTarget().X),
				DoubleBits(Result.GetTarget().Y),
				DoubleBits(Result.GetTarget().Z),
				DoubleBits(Result.GetApexClearance()),
				DoubleBits(Result.GetForwardDistance()),
				DoubleBits(Result.GetLateralOffset())
			});
	}

}

bool Fdemo_mapShanmenThrownWeaponArcChoiceBasis::TryCapture(
	const FVector& InOrigin,
	const FVector& InForward,
	const FVector& InRight,
	Fdemo_mapShanmenThrownWeaponArcChoiceBasis& OutBasis)
{
	OutBasis = Fdemo_mapShanmenThrownWeaponArcChoiceBasis();
	FVector CanonicalForward;
	FVector CanonicalRight;
	if (!IsFiniteVector(InOrigin)
		|| !TryNormalizeDirection(InForward, CanonicalForward)
		|| !TryNormalizeDirection(InRight, CanonicalRight)
		|| FMath::Abs(FVector::DotProduct(CanonicalForward, CanonicalRight))
			> OrthogonalityTolerance)
	{
		return false;
	}
	OutBasis.Origin = CanonicalVector(InOrigin);
	OutBasis.Forward = CanonicalForward;
	OutBasis.Right = CanonicalRight;
	OutBasis.BasisId = MakeBasisId(
		OutBasis.Origin, OutBasis.Forward, OutBasis.Right);
	return OutBasis.IsValid();
}

bool Fdemo_mapShanmenThrownWeaponArcChoiceBasis::IsValid() const
{
	return BasisId.IsValid()
		&& HasBasisShape(Origin, Forward, Right)
		&& BasisId == MakeBasisId(Origin, Forward, Right);
}

bool Fdemo_mapShanmenThrownWeaponArcChoiceBasis::Matches(
	const Fdemo_mapShanmenThrownWeaponArcChoiceBasis& Other) const
{
	return IsValid() && Other.IsValid() && BasisId == Other.BasisId;
}

bool Fdemo_mapShanmenThrownWeaponArcChoicePolicy::TryCapture(
	const double InMinimumForwardDistance,
	const double InMaximumForwardDistance,
	const double InMaximumLateralOffset,
	const double InMinimumApexClearance,
	const double InMaximumApexClearance,
	Fdemo_mapShanmenThrownWeaponArcChoicePolicy& OutPolicy)
{
	OutPolicy = Fdemo_mapShanmenThrownWeaponArcChoicePolicy();
	if (!HasPolicyShape(
			InMinimumForwardDistance,
			InMaximumForwardDistance,
			InMaximumLateralOffset,
			InMinimumApexClearance,
			InMaximumApexClearance))
	{
		return false;
	}
	OutPolicy.MinimumForwardDistance =
		CanonicalZero(InMinimumForwardDistance);
	OutPolicy.MaximumForwardDistance =
		CanonicalZero(InMaximumForwardDistance);
	OutPolicy.MaximumLateralOffset = CanonicalZero(InMaximumLateralOffset);
	OutPolicy.MinimumApexClearance = CanonicalZero(InMinimumApexClearance);
	OutPolicy.MaximumApexClearance = CanonicalZero(InMaximumApexClearance);
	OutPolicy.PolicyId = MakePolicyId(
		OutPolicy.MinimumForwardDistance,
		OutPolicy.MaximumForwardDistance,
		OutPolicy.MaximumLateralOffset,
		OutPolicy.MinimumApexClearance,
		OutPolicy.MaximumApexClearance);
	return OutPolicy.IsValid();
}

bool Fdemo_mapShanmenThrownWeaponArcChoicePolicy::IsValid() const
{
	return PolicyId.IsValid()
		&& HasPolicyShape(
			MinimumForwardDistance,
			MaximumForwardDistance,
			MaximumLateralOffset,
			MinimumApexClearance,
			MaximumApexClearance)
		&& PolicyId == MakePolicyId(
			MinimumForwardDistance,
			MaximumForwardDistance,
			MaximumLateralOffset,
			MinimumApexClearance,
			MaximumApexClearance);
}

bool Fdemo_mapShanmenThrownWeaponArcChoicePolicy::Matches(
	const Fdemo_mapShanmenThrownWeaponArcChoicePolicy& Other) const
{
	return IsValid() && Other.IsValid() && PolicyId == Other.PolicyId;
}

bool Fdemo_mapShanmenThrownWeaponArcChoiceProjectionResult::IsValid() const
{
	return Status
			== Edemo_mapShanmenThrownWeaponArcChoiceProjectionStatus::Projected
		&& !Diagnostic.IsEmpty()
		&& ProjectionId.IsValid()
		&& ProjectionId == MakeProjectionId(*this);
}

bool Fdemo_mapShanmenThrownWeaponArcChoiceProjectionResult::Matches(
	const Fdemo_mapShanmenThrownWeaponArcChoiceProjectionResult& Other) const
{
	return IsValid() && Other.IsValid()
		&& ProjectionId == Other.ProjectionId;
}

Fdemo_mapShanmenThrownWeaponArcChoiceProjectionResult
Fdemo_mapShanmenThrownWeaponArcChoiceProjectionResult::Reject(
	const Edemo_mapShanmenThrownWeaponArcChoiceProjectionStatus InStatus,
	const TCHAR* InDiagnostic)
{
	Fdemo_mapShanmenThrownWeaponArcChoiceProjectionResult Result;
	Result.Status = InStatus;
	Result.Diagnostic = InDiagnostic;
	return Result;
}

Fdemo_mapShanmenThrownWeaponArcChoiceProjectionResult
Fdemo_mapShanmenThrownWeaponArcChoiceProjector::Project(
	const Fdemo_mapShanmenThrownWeaponInputChoiceState& ChoiceState,
	const Fdemo_mapShanmenThrownWeaponArcChoiceBasis& Basis,
	const Fdemo_mapShanmenThrownWeaponArcChoicePolicy& Policy)
{
	if (!ChoiceState.IsValid())
	{
		return Fdemo_mapShanmenThrownWeaponArcChoiceProjectionResult::Reject(
			Edemo_mapShanmenThrownWeaponArcChoiceProjectionStatus::
				ChoiceStateInvalid,
			TEXT("Arc choice projection requires one valid choice state."));
	}
	if (ChoiceState.GetTrajectoryKind()
		!= Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::BallisticArc)
	{
		return Fdemo_mapShanmenThrownWeaponArcChoiceProjectionResult::Reject(
			Edemo_mapShanmenThrownWeaponArcChoiceProjectionStatus::
				TrajectoryNotArc,
			TEXT("Arc choice projection rejects a non-Arc trajectory."));
	}
	if (!ChoiceState.HasArcTargetIntent())
	{
		return Fdemo_mapShanmenThrownWeaponArcChoiceProjectionResult::Reject(
			Edemo_mapShanmenThrownWeaponArcChoiceProjectionStatus::
				TargetIntentMissing,
			TEXT("Arc choice projection requires a normalized target intent."));
	}
	if (!Basis.IsValid())
	{
		return Fdemo_mapShanmenThrownWeaponArcChoiceProjectionResult::Reject(
			Edemo_mapShanmenThrownWeaponArcChoiceProjectionStatus::BasisInvalid,
			TEXT("Arc choice projection rejected the caller-supplied basis."));
	}
	if (!Policy.IsValid())
	{
		return Fdemo_mapShanmenThrownWeaponArcChoiceProjectionResult::Reject(
			Edemo_mapShanmenThrownWeaponArcChoiceProjectionStatus::PolicyInvalid,
			TEXT("Arc choice projection rejected the frozen distance/apex policy."));
	}

	const FVector2D TargetIntent = ChoiceState.GetArcTargetIntent();
	const double ForwardAlpha =
		(FMath::Clamp(TargetIntent.Y, -1.0, 1.0) + 1.0) * 0.5;
	const double ForwardDistance = FMath::Lerp(
		Policy.GetMinimumForwardDistance(),
		Policy.GetMaximumForwardDistance(),
		ForwardAlpha);
	const double LateralOffset =
		FMath::Clamp(TargetIntent.X, -1.0, 1.0)
		* Policy.GetMaximumLateralOffset();
	const double ApexAlpha =
		(FMath::Clamp(ChoiceState.GetArcApexAdjustment(), -1.0, 1.0) + 1.0)
		* 0.5;
	const double ApexClearance = FMath::Lerp(
		Policy.GetMinimumApexClearance(),
		Policy.GetMaximumApexClearance(),
		ApexAlpha);
	const FVector Target = Basis.GetOrigin()
		+ Basis.GetForward() * ForwardDistance
		+ Basis.GetRight() * LateralOffset;
	if (!IsFiniteVector(Target)
		|| !FMath::IsFinite(ForwardDistance) || ForwardDistance <= 0.0
		|| !FMath::IsFinite(LateralOffset)
		|| !FMath::IsFinite(ApexClearance) || ApexClearance <= 0.0)
	{
		return Fdemo_mapShanmenThrownWeaponArcChoiceProjectionResult::Reject(
			Edemo_mapShanmenThrownWeaponArcChoiceProjectionStatus::OutputRejected,
			TEXT("Arc choice projection rejected non-finite derived geometry."));
	}

	Fdemo_mapShanmenThrownWeaponArcChoiceProjectionResult Result;
	Result.Status =
		Edemo_mapShanmenThrownWeaponArcChoiceProjectionStatus::Projected;
	Result.Diagnostic = TEXT("Projected one frozen thrown-weapon Arc choice.");
	Result.ChoiceStateId = ChoiceState.GetStateId();
	Result.ChoiceRevision = ChoiceState.GetRevision();
	Result.BasisId = Basis.GetBasisId();
	Result.PolicyId = Policy.GetPolicyId();
	Result.Target = CanonicalVector(Target);
	Result.ApexClearance = CanonicalZero(ApexClearance);
	Result.ForwardDistance = CanonicalZero(ForwardDistance);
	Result.LateralOffset = CanonicalZero(LateralOffset);
	Result.ProjectionId = MakeProjectionId(Result);
	if (!Result.IsValid())
	{
		return Fdemo_mapShanmenThrownWeaponArcChoiceProjectionResult::Reject(
			Edemo_mapShanmenThrownWeaponArcChoiceProjectionStatus::OutputRejected,
			TEXT("Arc choice projection could not seal derived geometry."));
	}
	return Result;
}
