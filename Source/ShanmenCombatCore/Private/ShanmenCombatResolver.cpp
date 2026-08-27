#include "ShanmenCombatResolver.h"

#include "ShanmenCombatTags.h"
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

	bool MatchesTags(
		const FGameplayTagContainer& Available,
		const FGameplayTagContainer& Required,
		const FGameplayTagContainer& Blocked)
	{
		return Available.HasAll(Required) && !Available.HasAny(Blocked);
	}

	bool LayerComesBefore(const FShanmenDefenseLayer& Left, const FShanmenDefenseLayer& Right)
	{
		if (Left.Order != Right.Order)
		{
			return Left.Order < Right.Order;
		}
		return GuidDigits(Left.LayerId) < GuidDigits(Right.LayerId);
	}
}

bool FShanmenCombatActionCapture::IsValid() const
{
	return RunId.IsValid()
		&& OwnerId.IsValid()
		&& ActivationId.IsValid()
		&& SourceEntityId.IsValid()
		&& !ActionDefinitionId.IsNone()
		&& Content.IsValid();
}

bool FShanmenCombatActionSnapshot::TryCapture(
	const FShanmenCombatActionCapture& Capture,
	FShanmenCombatActionSnapshot& OutSnapshot)
{
	OutSnapshot = FShanmenCombatActionSnapshot();
	if (!Capture.IsValid())
	{
		return false;
	}

	OutSnapshot.RunId = Capture.RunId;
	OutSnapshot.OwnerId = Capture.OwnerId;
	OutSnapshot.ActivationId = Capture.ActivationId;
	OutSnapshot.SourceEntityId = Capture.SourceEntityId;
	OutSnapshot.SourceItemInstanceId = Capture.SourceItemInstanceId;
	OutSnapshot.ActionDefinitionId = Capture.ActionDefinitionId;
	OutSnapshot.Content = Capture.Content;
	OutSnapshot.SourceTags = Capture.SourceTags;
	return true;
}

bool FShanmenCombatActionSnapshot::IsValid() const
{
	return RunId.IsValid()
		&& OwnerId.IsValid()
		&& ActivationId.IsValid()
		&& SourceEntityId.IsValid()
		&& !ActionDefinitionId.IsNone()
		&& Content.IsValid();
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

bool FShanmenDamagePacket::IsValid() const
{
	return !FormulaId.IsNone()
		&& FMath::IsFinite(RawDamage)
		&& RawDamage >= 0.0f
		&& !DamageTags.IsEmpty();
}

bool FShanmenTargetVitalitySnapshot::IsValid() const
{
	return FMath::IsFinite(CurrentVitality)
		&& FMath::IsFinite(MaximumVitality)
		&& CurrentVitality >= 0.0f
		&& MaximumVitality >= CurrentVitality;
}

bool FShanmenDefenseLayer::IsValid() const
{
	if (!LayerId.IsValid() || RuleId.IsNone() || LayerTags.IsEmpty() || !FMath::IsFinite(Magnitude))
	{
		return false;
	}
	if (bRequiresCommitOnTrigger && !SourceInstanceId.IsValid())
	{
		return false;
	}

	switch (Operation)
	{
	case EShanmenDefenseOperation::PreventAll:
		return Magnitude >= 0.0f;
	case EShanmenDefenseOperation::ReduceFraction:
		return IsFraction(Magnitude);
	case EShanmenDefenseOperation::AbsorbPoints:
	case EShanmenDefenseOperation::PreventLethal:
		return Magnitude >= 0.0f;
	default:
		return false;
	}
}

bool FShanmenDefenseLayer::IsApplicable(
	const FGameplayTagContainer& DamageTags,
	const FGameplayTagContainer& SourceTags,
	const FGameplayTagContainer& TargetTags) const
{
	return MatchesTags(DamageTags, RequiredDamageTags, BlockedDamageTags)
		&& MatchesTags(SourceTags, RequiredSourceTags, BlockedSourceTags)
		&& MatchesTags(TargetTags, RequiredTargetTags, BlockedTargetTags);
}

bool FShanmenDefenseSnapshot::IsValid() const
{
	TSet<FGuid> LayerIds;
	for (const FShanmenDefenseLayer& Layer : Layers)
	{
		if (!Layer.IsValid() || LayerIds.Contains(Layer.LayerId))
		{
			return false;
		}
		LayerIds.Add(Layer.LayerId);
	}
	return true;
}

bool FShanmenImpactRequest::IsValid() const
{
	if (!ImpactId.IsValid()
		|| !Action.IsValid()
		|| !Candidate.IsValid()
		|| !Damage.IsValid()
		|| !TargetVitality.IsValid()
		|| !Defense.IsValid()
		|| Action.GetActivationId() != Candidate.ActivationId
		|| Action.GetSourceEntityId() != Candidate.SourceEntityId)
	{
		return false;
	}

	return ImpactId == FShanmenCombatIdFactory::MakeImpactId(
		Action.GetRunId(),
		Candidate.ActivationId,
		Candidate.DetectorId,
		Candidate.TargetEntityId,
		Candidate.HitOrdinal);
}

bool FShanmenImpactResult::IsConserved(float Tolerance) const
{
	return bAccepted
		&& FMath::IsFinite(RawDamage)
		&& FMath::IsFinite(PreventedDamage)
		&& FMath::IsFinite(FinalDamage)
		&& FMath::IsNearlyEqual(RawDamage, PreventedDamage + FinalDamage, Tolerance);
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

bool FShanmenImpactLedger::TryAccept(const FShanmenImpactRequest& Request)
{
	if (!Request.IsValid() || AcceptedImpactIds.Contains(Request.ImpactId))
	{
		return false;
	}

	AcceptedImpactIds.Add(Request.ImpactId);
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
	Result.RawDamage = Request.Damage.RawDamage;

	TArray<int32> OrderedLayerIndices;
	OrderedLayerIndices.Reserve(Request.Defense.Layers.Num());
	for (int32 Index = 0; Index < Request.Defense.Layers.Num(); ++Index)
	{
		OrderedLayerIndices.Add(Index);
	}
	OrderedLayerIndices.Sort([&Request](int32 LeftIndex, int32 RightIndex)
	{
		return LayerComesBefore(Request.Defense.Layers[LeftIndex], Request.Defense.Layers[RightIndex]);
	});

	float RemainingDamage = Request.Damage.RawDamage;
	for (int32 LayerIndex : OrderedLayerIndices)
	{
		if (RemainingDamage <= 0.0f)
		{
			break;
		}

		const FShanmenDefenseLayer& Layer = Request.Defense.Layers[LayerIndex];
		if (!Layer.IsApplicable(
			Request.Damage.DamageTags,
			Request.Action.GetSourceTags(),
			Request.Defense.TargetTags))
		{
			continue;
		}

		float Prevented = 0.0f;
		switch (Layer.Operation)
		{
		case EShanmenDefenseOperation::PreventAll:
			Prevented = RemainingDamage;
			break;
		case EShanmenDefenseOperation::ReduceFraction:
			Prevented = RemainingDamage * Layer.Magnitude;
			break;
		case EShanmenDefenseOperation::AbsorbPoints:
			Prevented = FMath::Min(RemainingDamage, Layer.Magnitude);
			break;
		case EShanmenDefenseOperation::PreventLethal:
		{
			const float AllowedDamage = FMath::Max(0.0f, Request.TargetVitality.CurrentVitality - Layer.Magnitude);
			Prevented = FMath::Max(0.0f, RemainingDamage - AllowedDamage);
			break;
		}
		default:
			break;
		}

		Prevented = FMath::Clamp(Prevented, 0.0f, RemainingDamage);
		if (Prevented <= 0.0f)
		{
			continue;
		}

		RemainingDamage -= Prevented;
		Result.PreventedDamage += Prevented;

		FShanmenDefenseLayerResult& LayerResult = Result.TriggeredLayers.AddDefaulted_GetRef();
		LayerResult.LayerId = Layer.LayerId;
		LayerResult.RuleId = Layer.RuleId;
		LayerResult.SourceInstanceId = Layer.SourceInstanceId;
		LayerResult.Operation = Layer.Operation;
		LayerResult.Order = Layer.Order;
		LayerResult.PreventedDamage = Prevented;
		LayerResult.bRequiresCommit = Layer.bRequiresCommitOnTrigger;
		LayerResult.LayerTags = Layer.LayerTags;
	}

	Result.FinalDamage = FMath::Max(0.0f, RemainingDamage);
	if (Result.PreventedDamage <= 0.0f)
	{
		Result.Outcome = EShanmenDefenseOutcome::Applied;
	}
	else if (Result.FinalDamage > 0.0f)
	{
		Result.Outcome = EShanmenDefenseOutcome::Mitigated;
	}
	else if (!Result.TriggeredLayers.IsEmpty()
		&& Result.TriggeredLayers[0].LayerTags.HasTagExact(FShanmenCombatNativeTags::DefenseEvade()))
	{
		Result.Outcome = EShanmenDefenseOutcome::Evaded;
	}
	else if (!Result.TriggeredLayers.IsEmpty()
		&& Result.TriggeredLayers[0].LayerTags.HasTagExact(FShanmenCombatNativeTags::DefensePerfectGuard()))
	{
		Result.Outcome = EShanmenDefenseOutcome::PerfectGuarded;
	}
	else
	{
		Result.Outcome = EShanmenDefenseOutcome::FullyPrevented;
	}

	return Result;
}
