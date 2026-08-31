#include "ShanmenWeaponGuardArc.h"

#include "ShanmenDeterministicId.h"

namespace
{
	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	FString DoubleBits(double Value)
	{
		uint64 Bits = 0;
		static_assert(sizeof(Bits) == sizeof(Value));
		FMemory::Memcpy(&Bits, &Value, sizeof(Bits));
		return FString::Printf(
			TEXT("%016llX"),
			static_cast<unsigned long long>(Bits));
	}

	double CanonicalZero(double Value)
	{
		return Value == 0.0 ? 0.0 : Value;
	}

	bool IsFiniteVector(const FVector& Value)
	{
		return FMath::IsFinite(Value.X)
			&& FMath::IsFinite(Value.Y)
			&& FMath::IsFinite(Value.Z);
	}

	bool IsCanonicalDirection(const FVector& Direction)
	{
		return IsFiniteVector(Direction)
			&& !Direction.IsNearlyZero()
			&& FMath::IsNearlyEqual(
				Direction.SizeSquared(), 1.0, 1.e-8);
	}

	bool TryCanonicalDirection(
		const FVector& Candidate,
		FVector& OutDirection)
	{
		OutDirection = FVector::ZeroVector;
		if (!IsFiniteVector(Candidate) || Candidate.IsNearlyZero())
		{
			return false;
		}

		OutDirection = Candidate.GetSafeNormal();
		if (!IsCanonicalDirection(OutDirection))
		{
			OutDirection = FVector::ZeroVector;
			return false;
		}
		OutDirection.X = CanonicalZero(OutDirection.X);
		OutDirection.Y = CanonicalZero(OutDirection.Y);
		OutDirection.Z = CanonicalZero(OutDirection.Z);
		return IsCanonicalDirection(OutDirection);
	}

	bool WindowsMatch(
		const FShanmenWeaponGuardWindowReceipt& Left,
		const FShanmenWeaponGuardWindowReceipt& Right)
	{
		return Left.IsValid()
			&& Right.IsValid()
			&& Left.GetWindowId() == Right.GetWindowId()
			&& Left.GetReceiptId() == Right.GetReceiptId();
	}

	bool LayersMatch(
		const FShanmenDefenseLayer& Left,
		const FShanmenDefenseLayer& Right)
	{
		return Left.IsValid()
			&& Right.IsValid()
			&& Left.LayerId == Right.LayerId
			&& Left.RuleId == Right.RuleId
			&& Left.SourceInstanceId == Right.SourceInstanceId
			&& Left.Operation == Right.Operation
			&& Left.Order == Right.Order
			&& Left.Magnitude == Right.Magnitude
			&& Left.bRequiresCommitOnTrigger
				== Right.bRequiresCommitOnTrigger
			&& Left.LayerTags == Right.LayerTags
			&& Left.RequiredDamageTags == Right.RequiredDamageTags
			&& Left.BlockedDamageTags == Right.BlockedDamageTags
			&& Left.RequiredSourceTags == Right.RequiredSourceTags
			&& Left.BlockedSourceTags == Right.BlockedSourceTags
			&& Left.RequiredTargetTags == Right.RequiredTargetTags
			&& Left.BlockedTargetTags == Right.BlockedTargetTags;
	}

	bool IsDefaultLayer(const FShanmenDefenseLayer& Layer)
	{
		return !Layer.LayerId.IsValid()
			&& Layer.RuleId.IsNone()
			&& !Layer.SourceInstanceId.IsValid()
			&& Layer.Operation == EShanmenDefenseOperation::ReduceFraction
			&& Layer.Order == 0
			&& Layer.Magnitude == 0.0f
			&& !Layer.bRequiresCommitOnTrigger
			&& Layer.LayerTags.IsEmpty()
			&& Layer.RequiredDamageTags.IsEmpty()
			&& Layer.BlockedDamageTags.IsEmpty()
			&& Layer.RequiredSourceTags.IsEmpty()
			&& Layer.BlockedSourceTags.IsEmpty()
			&& Layer.RequiredTargetTags.IsEmpty()
			&& Layer.BlockedTargetTags.IsEmpty();
	}

	FGuid MakePolicyId(
		const FShanmenWeaponGuardWindowReceipt& Window,
		FName ArcRuleId,
		double MinimumFacingDot)
	{
		if (!Window.IsValid()
			|| ArcRuleId.IsNone()
			|| !FMath::IsFinite(MinimumFacingDot)
			|| MinimumFacingDot < -1.0
			|| MinimumFacingDot > 1.0)
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Sword.WeaponGuard.ArcPolicy.r1"),
			{
				GuidDigits(Window.GetReceiptId()),
				GuidDigits(Window.GetWindowId()),
				ArcRuleId.ToString(),
				DoubleBits(CanonicalZero(MinimumFacingDot))
			});
	}

	FGuid MakeSampleId(
		const FShanmenHitCandidate& Candidate,
		const FVector& GuardFacing,
		const FVector& DirectionToThreat)
	{
		if (!Candidate.IsValid()
			|| !IsFiniteVector(Candidate.HitLocation)
			|| !IsFiniteVector(Candidate.HitNormal)
			|| !IsCanonicalDirection(GuardFacing)
			|| !IsCanonicalDirection(DirectionToThreat))
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Sword.WeaponGuard.ThreatSample.r1"),
			{
				GuidDigits(Candidate.ActivationId),
				GuidDigits(Candidate.SourceEntityId),
				GuidDigits(Candidate.TargetEntityId),
				Candidate.DetectorId.ToString(),
				FString::FromInt(static_cast<uint8>(Candidate.DetectorKind)),
				FString::FromInt(Candidate.HitOrdinal),
				DoubleBits(Candidate.HitLocation.X),
				DoubleBits(Candidate.HitLocation.Y),
				DoubleBits(Candidate.HitLocation.Z),
				DoubleBits(Candidate.HitNormal.X),
				DoubleBits(Candidate.HitNormal.Y),
				DoubleBits(Candidate.HitNormal.Z),
				DoubleBits(GuardFacing.X),
				DoubleBits(GuardFacing.Y),
				DoubleBits(GuardFacing.Z),
				DoubleBits(DirectionToThreat.X),
				DoubleBits(DirectionToThreat.Y),
				DoubleBits(DirectionToThreat.Z)
			});
	}

	double ComputeAlignmentDot(const FShanmenWeaponGuardThreatSample& Sample)
	{
		if (!Sample.IsValid())
		{
			return 0.0;
		}
		return CanonicalZero(FMath::Clamp(
			FVector::DotProduct(
				Sample.GetGuardFacing(), Sample.GetDirectionToThreat()),
			-1.0,
			1.0));
	}

	EShanmenWeaponGuardArcStatus Classify(
		const FShanmenWeaponGuardArcPolicy& Policy,
		const FShanmenWeaponGuardThreatSample& Sample)
	{
		if (!Policy.IsValid() || !Sample.IsValid())
		{
			return EShanmenWeaponGuardArcStatus::Invalid;
		}
		return ComputeAlignmentDot(Sample) >= Policy.GetMinimumFacingDot()
			? EShanmenWeaponGuardArcStatus::Qualified
			: EShanmenWeaponGuardArcStatus::OutsideArc;
	}

	FGuid MakeEvaluationId(
		const FShanmenWeaponGuardArcPolicy& Policy,
		const FShanmenWeaponGuardTimingProjectionReceipt& TimingProjection,
		const FShanmenWeaponGuardThreatSample& Sample,
		EShanmenWeaponGuardArcStatus Status,
		double Dot,
		const FShanmenDefenseLayer& Layer)
	{
		if (!Policy.IsValid()
			|| !TimingProjection.IsValid()
			|| !Sample.IsValid()
			|| Status == EShanmenWeaponGuardArcStatus::Invalid
			|| !FMath::IsFinite(Dot))
		{
			return FGuid();
		}
		const FString LayerIdentity =
			Status == EShanmenWeaponGuardArcStatus::Qualified
				&& Layer.IsValid()
			? GuidDigits(Layer.LayerId)
			: TEXT("NO_LAYER");
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Sword.WeaponGuard.ArcEvaluation.r1"),
			{
				GuidDigits(Policy.GetPolicyId()),
				GuidDigits(TimingProjection.GetReceiptId()),
				GuidDigits(Sample.GetSampleId()),
				FString::FromInt(static_cast<uint8>(Status)),
				DoubleBits(CanonicalZero(Dot)),
				LayerIdentity
			});
	}
}

bool FShanmenWeaponGuardArcPolicy::TryCapture(
	const FShanmenWeaponGuardWindowReceipt& Window,
	FName ArcRuleId,
	double MinimumFacingDot,
	FShanmenWeaponGuardArcPolicy& OutPolicy)
{
	OutPolicy = FShanmenWeaponGuardArcPolicy();
	FShanmenWeaponGuardArcPolicy Candidate;
	Candidate.Window = Window;
	Candidate.ArcRuleId = ArcRuleId;
	Candidate.MinimumFacingDot = CanonicalZero(MinimumFacingDot);
	Candidate.PolicyId = MakePolicyId(
		Candidate.Window,
		Candidate.ArcRuleId,
		Candidate.MinimumFacingDot);
	if (!Candidate.IsValid())
	{
		return false;
	}
	OutPolicy = Candidate;
	return true;
}

bool FShanmenWeaponGuardArcPolicy::IsValid() const
{
	return PolicyId.IsValid()
		&& Window.IsValid()
		&& !ArcRuleId.IsNone()
		&& FMath::IsFinite(MinimumFacingDot)
		&& MinimumFacingDot >= -1.0
		&& MinimumFacingDot <= 1.0
		&& MinimumFacingDot == CanonicalZero(MinimumFacingDot)
		&& PolicyId == MakePolicyId(
			Window, ArcRuleId, MinimumFacingDot);
}

bool FShanmenWeaponGuardThreatSample::TryCapture(
	const FShanmenHitCandidate& Candidate,
	const FVector& GuardFacing,
	const FVector& DirectionToThreat,
	FShanmenWeaponGuardThreatSample& OutSample)
{
	OutSample = FShanmenWeaponGuardThreatSample();
	FVector CanonicalFacing;
	FVector CanonicalThreat;
	if (!Candidate.IsValid()
		|| !IsFiniteVector(Candidate.HitLocation)
		|| !IsFiniteVector(Candidate.HitNormal)
		|| !TryCanonicalDirection(GuardFacing, CanonicalFacing)
		|| !TryCanonicalDirection(DirectionToThreat, CanonicalThreat))
	{
		return false;
	}

	FShanmenWeaponGuardThreatSample Captured;
	Captured.Candidate = Candidate;
	Captured.GuardFacing = CanonicalFacing;
	Captured.DirectionToThreat = CanonicalThreat;
	Captured.SampleId = MakeSampleId(
		Captured.Candidate,
		Captured.GuardFacing,
		Captured.DirectionToThreat);
	if (!Captured.IsValid())
	{
		return false;
	}
	OutSample = Captured;
	return true;
}

bool FShanmenWeaponGuardThreatSample::IsValid() const
{
	return SampleId.IsValid()
		&& Candidate.IsValid()
		&& IsFiniteVector(Candidate.HitLocation)
		&& IsFiniteVector(Candidate.HitNormal)
		&& IsCanonicalDirection(GuardFacing)
		&& IsCanonicalDirection(DirectionToThreat)
		&& SampleId == MakeSampleId(
			Candidate, GuardFacing, DirectionToThreat);
}

bool FShanmenWeaponGuardArcEvaluation::IsValid() const
{
	if (!EvaluationId.IsValid()
		|| !Policy.IsValid()
		|| !TimingProjection.IsValid()
		|| !Sample.IsValid()
		|| !WindowsMatch(
			Policy.GetWindow(), TimingProjection.GetPolicy().GetWindow())
		|| Sample.GetCandidate().TargetEntityId
			!= Policy.GetWindow().GetAction().GetSourceEntityId()
		|| !FMath::IsFinite(AlignmentDot)
		|| AlignmentDot != ComputeAlignmentDot(Sample)
		|| Status != Classify(Policy, Sample))
	{
		return false;
	}

	const bool bLayerMatches =
		Status == EShanmenWeaponGuardArcStatus::Qualified
			? LayersMatch(Layer, TimingProjection.GetLayer())
			: Status == EShanmenWeaponGuardArcStatus::OutsideArc
				&& IsDefaultLayer(Layer);
	return bLayerMatches
		&& EvaluationId == MakeEvaluationId(
			Policy,
			TimingProjection,
			Sample,
			Status,
			AlignmentDot,
			Layer);
}

bool FShanmenWeaponGuardArcEvaluator::TryEvaluate(
	const FShanmenWeaponGuardWindow& Window,
	const FShanmenActionOrchestrator& ActionRuntime,
	const FShanmenWeaponGuardArcPolicy& Policy,
	const FShanmenWeaponGuardTimingProjectionReceipt& TimingProjection,
	const FShanmenWeaponGuardThreatSample& Sample,
	FShanmenWeaponGuardArcEvaluation& OutEvaluation)
{
	OutEvaluation = FShanmenWeaponGuardArcEvaluation();
	if (!Window.IsActiveFor(ActionRuntime)
		|| !Policy.IsValid()
		|| !TimingProjection.IsValid()
		|| !Sample.IsValid()
		|| !WindowsMatch(Window.GetOpenReceipt(), Policy.GetWindow())
		|| !WindowsMatch(
			Policy.GetWindow(), TimingProjection.GetPolicy().GetWindow())
		|| Sample.GetCandidate().TargetEntityId
			!= Policy.GetWindow().GetAction().GetSourceEntityId())
	{
		return false;
	}

	FShanmenWeaponGuardArcEvaluation Candidate;
	Candidate.Policy = Policy;
	Candidate.TimingProjection = TimingProjection;
	Candidate.Sample = Sample;
	Candidate.AlignmentDot = ComputeAlignmentDot(Candidate.Sample);
	Candidate.Status = Classify(Candidate.Policy, Candidate.Sample);
	if (Candidate.Status == EShanmenWeaponGuardArcStatus::Qualified)
	{
		Candidate.Layer = Candidate.TimingProjection.GetLayer();
	}
	Candidate.EvaluationId = MakeEvaluationId(
		Candidate.Policy,
		Candidate.TimingProjection,
		Candidate.Sample,
		Candidate.Status,
		Candidate.AlignmentDot,
		Candidate.Layer);
	if (!Candidate.IsValid())
	{
		return false;
	}

	OutEvaluation = Candidate;
	return true;
}
