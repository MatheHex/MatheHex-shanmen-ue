#include "ShanmenCombatResolver.h"

#include "ShanmenDeterministicId.h"

namespace
{
	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	bool IsFraction(float Value)
	{
		return FMath::IsFinite(Value) && Value >= 0.0f && Value <= 1.0f;
	}
}

bool FShanmenCombatActionSnapshot::IsValid() const
{
	return RunId.IsValid()
		&& OwnerId.IsValid()
		&& ActivationId.IsValid()
		&& SourceEntityId.IsValid()
		&& !ActionDefinitionId.IsNone()
		&& Content.IsValid()
		&& FMath::IsFinite(BasePower)
		&& BasePower >= 0.0f;
}

bool FShanmenHitCandidate::IsValid() const
{
	return ActivationId.IsValid()
		&& SourceEntityId.IsValid()
		&& TargetEntityId.IsValid()
		&& !DetectorId.IsNone()
		&& HitOrdinal >= 0
		&& !HitLocation.ContainsNaN()
		&& !HitNormal.ContainsNaN();
}

bool FShanmenDefenseSnapshot::IsValid() const
{
	return IsFraction(GuardReductionFraction)
		&& FMath::IsFinite(ShieldPoints)
		&& ShieldPoints >= 0.0f
		&& IsFraction(ArmorResistanceFraction);
}

bool FShanmenImpactRequest::IsValid() const
{
	return ImpactId.IsValid()
		&& Action.IsValid()
		&& Candidate.IsValid()
		&& Defense.IsValid()
		&& Action.ActivationId == Candidate.ActivationId
		&& Action.SourceEntityId == Candidate.SourceEntityId
		&& FMath::IsFinite(IncomingDamage)
		&& IncomingDamage >= 0.0f;
}

FGuid FShanmenCombatIdFactory::MakeActivationId(
	const FGuid& RunId,
	const FGuid& SourceEntityId,
	FName ActionDefinitionId,
	uint64 ActivationSequence)
{
	if (!RunId.IsValid() || !SourceEntityId.IsValid() || ActionDefinitionId.IsNone())
	{
		return FGuid();
	}

	return FShanmenDeterministicId::FromCanonicalParts(
		TEXT("Shanmen.Combat.Activation.r1"),
		{
			GuidDigits(RunId),
			GuidDigits(SourceEntityId),
			ActionDefinitionId.ToString(),
			FString::Printf(TEXT("%llu"), static_cast<unsigned long long>(ActivationSequence))
		});
}

FGuid FShanmenCombatIdFactory::MakeImpactId(
	const FGuid& RunId,
	const FGuid& ActivationId,
	FName DetectorId,
	const FGuid& TargetEntityId,
	int32 HitOrdinal)
{
	if (!RunId.IsValid() || !ActivationId.IsValid() || DetectorId.IsNone() || !TargetEntityId.IsValid() || HitOrdinal < 0)
	{
		return FGuid();
	}

	return FShanmenDeterministicId::FromCanonicalParts(
		TEXT("Shanmen.Combat.Impact.r1"),
		{
			GuidDigits(RunId),
			GuidDigits(ActivationId),
			DetectorId.ToString(),
			GuidDigits(TargetEntityId),
			FString::FromInt(HitOrdinal)
		});
}

bool FShanmenImpactLedger::TryAccept(const FGuid& ImpactId)
{
	if (!ImpactId.IsValid() || AcceptedImpactIds.Contains(ImpactId))
	{
		return false;
	}

	AcceptedImpactIds.Add(ImpactId);
	return true;
}

bool FShanmenImpactLedger::Contains(const FGuid& ImpactId) const
{
	return ImpactId.IsValid() && AcceptedImpactIds.Contains(ImpactId);
}

int32 FShanmenImpactLedger::Num() const
{
	return AcceptedImpactIds.Num();
}

void FShanmenImpactLedger::Reset()
{
	AcceptedImpactIds.Reset();
}

FShanmenImpactResult FShanmenDefenseResolver::Resolve(const FShanmenImpactRequest& Request)
{
	FShanmenImpactResult Result;
	if (!Request.IsValid())
	{
		return Result;
	}

	Result.bAccepted = true;
	Result.ImpactId = Request.ImpactId;
	Result.RawDamage = Request.IncomingDamage;

	if (Request.Defense.bDodgeWindowActive)
	{
		Result.Outcome = EShanmenDefenseOutcome::Evaded;
		return Result;
	}

	if (Request.Defense.bPerfectGuardWindowActive)
	{
		Result.Outcome = EShanmenDefenseOutcome::PerfectGuarded;
		Result.GuardPrevented = Request.IncomingDamage;
		return Result;
	}

	float RemainingDamage = Request.IncomingDamage;
	if (Request.Defense.bGuardActive)
	{
		Result.GuardPrevented = RemainingDamage * Request.Defense.GuardReductionFraction;
		RemainingDamage -= Result.GuardPrevented;
	}

	Result.ShieldAbsorbed = FMath::Min(RemainingDamage, Request.Defense.ShieldPoints);
	RemainingDamage -= Result.ShieldAbsorbed;

	Result.ArmorPrevented = RemainingDamage * Request.Defense.ArmorResistanceFraction;
	RemainingDamage -= Result.ArmorPrevented;
	Result.FinalDamage = FMath::Max(0.0f, RemainingDamage);

	const bool bMitigated = Result.GuardPrevented > 0.0f
		|| Result.ShieldAbsorbed > 0.0f
		|| Result.ArmorPrevented > 0.0f;
	Result.Outcome = bMitigated ? EShanmenDefenseOutcome::Mitigated : EShanmenDefenseOutcome::Applied;
	return Result;
}
