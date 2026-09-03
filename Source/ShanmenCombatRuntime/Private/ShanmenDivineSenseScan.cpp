#include "ShanmenDivineSenseScan.h"

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

	bool TryCanonicalVector(const FVector& Value, FVector& OutValue)
	{
		OutValue = FVector::ZeroVector;
		if (!IsFiniteVector(Value))
		{
			return false;
		}
		OutValue = Value;
		OutValue.X = CanonicalZero(OutValue.X);
		OutValue.Y = CanonicalZero(OutValue.Y);
		OutValue.Z = CanonicalZero(OutValue.Z);
		return IsFiniteVector(OutValue);
	}

	bool IsKnownOcclusionPolicy(
		EShanmenDivineSenseOcclusionPolicy Policy)
	{
		switch (Policy)
		{
		case EShanmenDivineSenseOcclusionPolicy::VisibleOnly:
		case EShanmenDivineSenseOcclusionPolicy::RevealOccluded:
			return true;
		default:
			return false;
		}
	}

	bool TagsConflict(
		const FGameplayTagContainer& Required,
		const FGameplayTagContainer& Blocked)
	{
		TArray<FGameplayTag> RequiredTags;
		TArray<FGameplayTag> BlockedTags;
		Required.GetGameplayTagArray(RequiredTags);
		Blocked.GetGameplayTagArray(BlockedTags);
		for (const FGameplayTag& RequiredTag : RequiredTags)
		{
			for (const FGameplayTag& BlockedTag : BlockedTags)
			{
				if (RequiredTag.MatchesTag(BlockedTag)
					|| BlockedTag.MatchesTag(RequiredTag))
				{
					return true;
				}
			}
		}
		return false;
	}

	bool MatchesTags(
		const FGameplayTagContainer& SubjectTags,
		const FGameplayTagContainer& RequiredTags,
		const FGameplayTagContainer& BlockedTags)
	{
		TArray<FGameplayTag> Required;
		TArray<FGameplayTag> Blocked;
		RequiredTags.GetGameplayTagArray(Required);
		BlockedTags.GetGameplayTagArray(Blocked);
		for (const FGameplayTag& Tag : Required)
		{
			if (!SubjectTags.HasTag(Tag))
			{
				return false;
			}
		}
		for (const FGameplayTag& Tag : Blocked)
		{
			if (SubjectTags.HasTag(Tag))
			{
				return false;
			}
		}
		return true;
	}

	void AppendCanonicalTags(
		TArray<FString>& Parts,
		const TCHAR* Label,
		const FGameplayTagContainer& Container)
	{
		TArray<FGameplayTag> Tags;
		Container.GetGameplayTagArray(Tags);
		Tags.Sort([](const FGameplayTag& Left, const FGameplayTag& Right)
		{
			return Left.GetTagName().LexicalLess(Right.GetTagName());
		});
		Parts.Add(Label);
		Parts.Add(FString::FromInt(Tags.Num()));
		for (const FGameplayTag& Tag : Tags)
		{
			Parts.Add(Tag.ToString());
		}
	}

	void AppendActionIdentity(
		TArray<FString>& Parts,
		const FShanmenCombatActionSnapshot& Action)
	{
		Parts.Append({
			GuidDigits(Action.GetRunId()),
			GuidDigits(Action.GetOwnerId()),
			GuidDigits(Action.GetActivationId()),
			GuidDigits(Action.GetSourceEntityId()),
			GuidDigits(Action.GetSourceItemInstanceId()),
			Action.GetActionDefinitionId().ToString(),
			Action.GetContent().Version.ToString(),
			Action.GetContent().Digest
		});
		AppendCanonicalTags(Parts, TEXT("SourceTags"), Action.GetSourceTags());
	}

	FGuid MakeDefinitionId(
		FName ActionDefinitionId,
		FName ScanRuleId,
		double Radius,
		int32 MaximumResults,
		EShanmenDivineSenseOcclusionPolicy OcclusionPolicy,
		const FGameplayTagContainer& RequiredSubjectTags,
		const FGameplayTagContainer& BlockedSubjectTags,
		bool bRejectSelf)
	{
		const double RadiusSquared = Radius * Radius;
		if (ActionDefinitionId
				!= FShanmenDivineSenseDefinition::CanonicalActionDefinitionId()
			|| ScanRuleId.IsNone()
			|| !FMath::IsFinite(Radius)
			|| Radius <= 0.0
			|| !FMath::IsFinite(RadiusSquared)
			|| MaximumResults <= 0
			|| !IsKnownOcclusionPolicy(OcclusionPolicy)
			|| TagsConflict(RequiredSubjectTags, BlockedSubjectTags))
		{
			return FGuid();
		}

		TArray<FString> Parts = {
			ActionDefinitionId.ToString(),
			ScanRuleId.ToString(),
			DoubleBits(CanonicalZero(Radius)),
			FString::FromInt(MaximumResults),
			FString::FromInt(static_cast<uint8>(OcclusionPolicy)),
			bRejectSelf ? TEXT("REJECT_SELF") : TEXT("ALLOW_SELF")
		};
		AppendCanonicalTags(
			Parts, TEXT("RequiredSubject"), RequiredSubjectTags);
		AppendCanonicalTags(
			Parts, TEXT("BlockedSubject"), BlockedSubjectTags);
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Spell.DivineSense.Definition.r1"), Parts);
	}

	FGuid MakeScanId(
		const FShanmenCombatActionSnapshot& Action,
		const FShanmenDivineSenseDefinition& Definition,
		const FVector& Origin,
		int32 ScanOrdinal)
	{
		if (!Action.IsValid()
			|| !Definition.IsValid()
			|| Action.GetActionDefinitionId()
				!= Definition.GetActionDefinitionId()
			|| !IsFiniteVector(Origin)
			|| ScanOrdinal < 0)
		{
			return FGuid();
		}

		TArray<FString> Parts;
		AppendActionIdentity(Parts, Action);
		Parts.Append({
			GuidDigits(Definition.GetDefinitionId()),
			DoubleBits(CanonicalZero(Origin.X)),
			DoubleBits(CanonicalZero(Origin.Y)),
			DoubleBits(CanonicalZero(Origin.Z)),
			FString::FromInt(ScanOrdinal)
		});
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Spell.DivineSense.Scan.r1"), Parts);
	}

	FGuid MakeObservationId(
		const FGuid& ScanId,
		const FGuid& SubjectEntityId,
		const FVector& WorldLocation,
		const FGameplayTagContainer& SubjectTags,
		bool bHasLineOfSight,
		int64 AuthorityRevision)
	{
		if (!ScanId.IsValid()
			|| !SubjectEntityId.IsValid()
			|| !IsFiniteVector(WorldLocation)
			|| SubjectTags.IsEmpty()
			|| AuthorityRevision < 0)
		{
			return FGuid();
		}

		TArray<FString> Parts = {
			GuidDigits(ScanId),
			GuidDigits(SubjectEntityId),
			DoubleBits(CanonicalZero(WorldLocation.X)),
			DoubleBits(CanonicalZero(WorldLocation.Y)),
			DoubleBits(CanonicalZero(WorldLocation.Z)),
			bHasLineOfSight ? TEXT("VISIBLE") : TEXT("OCCLUDED"),
			FString::Printf(TEXT("%lld"), AuthorityRevision)
		};
		AppendCanonicalTags(Parts, TEXT("SubjectTags"), SubjectTags);
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Spell.DivineSense.Observation.r1"), Parts);
	}

	bool TryDistanceSquared(
		const FShanmenDivineSenseScanRequest& Request,
		const FShanmenDivineSenseObservation& Observation,
		double& OutDistanceSquared)
	{
		OutDistanceSquared = 0.0;
		if (!Request.IsValid()
			|| !Observation.IsValid()
			|| Observation.GetScanId() != Request.GetScanId())
		{
			return false;
		}
		const FVector Delta =
			Observation.GetWorldLocation() - Request.GetOrigin();
		if (!IsFiniteVector(Delta))
		{
			return false;
		}
		OutDistanceSquared = CanonicalZero(Delta.SizeSquared());
		return FMath::IsFinite(OutDistanceSquared)
			&& OutDistanceSquared >= 0.0;
	}

	bool IsRevealable(
		const FShanmenDivineSenseScanRequest& Request,
		const FShanmenDivineSenseObservation& Observation,
		double& OutDistanceSquared)
	{
		OutDistanceSquared = 0.0;
		if (!TryDistanceSquared(Request, Observation, OutDistanceSquared))
		{
			return false;
		}
		const FShanmenDivineSenseDefinition& Definition =
			Request.GetDefinition();
		if (Definition.RejectsSelf()
			&& Observation.GetSubjectEntityId()
				== Request.GetAction().GetSourceEntityId())
		{
			return false;
		}
		const double RadiusSquared =
			Definition.GetRadius() * Definition.GetRadius();
		if (OutDistanceSquared > RadiusSquared
			|| !MatchesTags(
				Observation.GetSubjectTags(),
				Definition.GetRequiredSubjectTags(),
				Definition.GetBlockedSubjectTags()))
		{
			return false;
		}
		return Definition.GetOcclusionPolicy()
				== EShanmenDivineSenseOcclusionPolicy::RevealOccluded
			|| Observation.HasLineOfSight();
	}

	FGuid MakeRevealId(
		const FShanmenDivineSenseObservation& Observation,
		double DistanceSquared)
	{
		if (!Observation.IsValid()
			|| !FMath::IsFinite(DistanceSquared)
			|| DistanceSquared < 0.0)
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Spell.DivineSense.Reveal.r1"),
			{
				GuidDigits(Observation.GetScanId()),
				GuidDigits(Observation.GetObservationId()),
				DoubleBits(CanonicalZero(DistanceSquared))
			});
	}

	bool RevealComesBefore(
		const FShanmenDivineSenseReveal& Left,
		const FShanmenDivineSenseReveal& Right)
	{
		if (Left.GetDistanceSquared() != Right.GetDistanceSquared())
		{
			return Left.GetDistanceSquared() < Right.GetDistanceSquared();
		}
		return GuidDigits(Left.GetObservation().GetSubjectEntityId())
			< GuidDigits(Right.GetObservation().GetSubjectEntityId());
	}

	FGuid MakeReceiptId(
		const FShanmenDivineSenseScanRequest& Request,
		const TArray<FShanmenDivineSenseReveal>& Reveals)
	{
		if (!Request.IsValid())
		{
			return FGuid();
		}
		TArray<FString> Parts = {
			GuidDigits(Request.GetScanId()),
			FString::FromInt(Reveals.Num())
		};
		for (const FShanmenDivineSenseReveal& Reveal : Reveals)
		{
			if (!Reveal.IsValid())
			{
				return FGuid();
			}
			Parts.Add(GuidDigits(Reveal.GetRevealId()));
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Spell.DivineSense.ScanReceipt.r1"), Parts);
	}
}

FName FShanmenDivineSenseDefinition::CanonicalActionDefinitionId()
{
	return TEXT("Combat.Action.Spell.DivineSense.Pulse");
}

bool FShanmenDivineSenseDefinition::TryCapture(
	const FShanmenDivineSenseDefinitionCapture& Capture,
	FShanmenDivineSenseDefinition& OutDefinition)
{
	OutDefinition = FShanmenDivineSenseDefinition();
	FShanmenDivineSenseDefinition Candidate;
	Candidate.ActionDefinitionId = Capture.ActionDefinitionId;
	Candidate.ScanRuleId = Capture.ScanRuleId;
	Candidate.Radius = CanonicalZero(Capture.Radius);
	Candidate.MaximumResults = Capture.MaximumResults;
	Candidate.OcclusionPolicy = Capture.OcclusionPolicy;
	Candidate.RequiredSubjectTags = Capture.RequiredSubjectTags;
	Candidate.BlockedSubjectTags = Capture.BlockedSubjectTags;
	Candidate.bRejectSelf = Capture.bRejectSelf;
	Candidate.DefinitionId = MakeDefinitionId(
		Candidate.ActionDefinitionId,
		Candidate.ScanRuleId,
		Candidate.Radius,
		Candidate.MaximumResults,
		Candidate.OcclusionPolicy,
		Candidate.RequiredSubjectTags,
		Candidate.BlockedSubjectTags,
		Candidate.bRejectSelf);
	if (!Candidate.IsValid())
	{
		return false;
	}
	OutDefinition = Candidate;
	return true;
}

bool FShanmenDivineSenseDefinition::IsValid() const
{
	return DefinitionId.IsValid()
		&& Radius == CanonicalZero(Radius)
		&& DefinitionId == MakeDefinitionId(
			ActionDefinitionId,
			ScanRuleId,
			Radius,
			MaximumResults,
			OcclusionPolicy,
			RequiredSubjectTags,
			BlockedSubjectTags,
			bRejectSelf);
}

bool FShanmenDivineSenseScanRequest::TryCapture(
	const FShanmenCombatActionSnapshot& Action,
	const FShanmenDivineSenseDefinition& Definition,
	const FVector& Origin,
	int32 ScanOrdinal,
	FShanmenDivineSenseScanRequest& OutRequest)
{
	OutRequest = FShanmenDivineSenseScanRequest();
	FVector CanonicalOrigin;
	if (!TryCanonicalVector(Origin, CanonicalOrigin))
	{
		return false;
	}

	FShanmenDivineSenseScanRequest Candidate;
	Candidate.Action = Action;
	Candidate.Definition = Definition;
	Candidate.Origin = CanonicalOrigin;
	Candidate.ScanOrdinal = ScanOrdinal;
	Candidate.ScanId = MakeScanId(
		Candidate.Action,
		Candidate.Definition,
		Candidate.Origin,
		Candidate.ScanOrdinal);
	if (!Candidate.IsValid())
	{
		return false;
	}
	OutRequest = Candidate;
	return true;
}

bool FShanmenDivineSenseScanRequest::IsValid() const
{
	return ScanId.IsValid()
		&& Action.IsValid()
		&& Definition.IsValid()
		&& Action.GetActionDefinitionId()
			== Definition.GetActionDefinitionId()
		&& IsFiniteVector(Origin)
		&& Origin.X == CanonicalZero(Origin.X)
		&& Origin.Y == CanonicalZero(Origin.Y)
		&& Origin.Z == CanonicalZero(Origin.Z)
		&& ScanOrdinal >= 0
		&& ScanId == MakeScanId(
			Action, Definition, Origin, ScanOrdinal);
}

bool FShanmenDivineSenseObservation::TryCapture(
	const FShanmenDivineSenseScanRequest& Request,
	const FShanmenDivineSenseObservationCapture& Capture,
	FShanmenDivineSenseObservation& OutObservation)
{
	OutObservation = FShanmenDivineSenseObservation();
	FVector CanonicalLocation;
	if (!Request.IsValid()
		|| !TryCanonicalVector(Capture.WorldLocation, CanonicalLocation))
	{
		return false;
	}

	FShanmenDivineSenseObservation Candidate;
	Candidate.ScanId = Request.GetScanId();
	Candidate.SubjectEntityId = Capture.SubjectEntityId;
	Candidate.WorldLocation = CanonicalLocation;
	Candidate.SubjectTags = Capture.SubjectTags;
	Candidate.bHasLineOfSight = Capture.bHasLineOfSight;
	Candidate.AuthorityRevision = Capture.AuthorityRevision;
	Candidate.ObservationId = MakeObservationId(
		Candidate.ScanId,
		Candidate.SubjectEntityId,
		Candidate.WorldLocation,
		Candidate.SubjectTags,
		Candidate.bHasLineOfSight,
		Candidate.AuthorityRevision);
	if (!Candidate.IsValid())
	{
		return false;
	}
	OutObservation = Candidate;
	return true;
}

bool FShanmenDivineSenseObservation::IsValid() const
{
	return ObservationId.IsValid()
		&& ScanId.IsValid()
		&& SubjectEntityId.IsValid()
		&& IsFiniteVector(WorldLocation)
		&& WorldLocation.X == CanonicalZero(WorldLocation.X)
		&& WorldLocation.Y == CanonicalZero(WorldLocation.Y)
		&& WorldLocation.Z == CanonicalZero(WorldLocation.Z)
		&& !SubjectTags.IsEmpty()
		&& AuthorityRevision >= 0
		&& ObservationId == MakeObservationId(
			ScanId,
			SubjectEntityId,
			WorldLocation,
			SubjectTags,
			bHasLineOfSight,
			AuthorityRevision);
}

bool FShanmenDivineSenseReveal::IsValid() const
{
	return RevealId.IsValid()
		&& Observation.IsValid()
		&& FMath::IsFinite(DistanceSquared)
		&& DistanceSquared >= 0.0
		&& DistanceSquared == CanonicalZero(DistanceSquared)
		&& RevealId == MakeRevealId(Observation, DistanceSquared);
}

bool FShanmenDivineSenseScanReceipt::IsValid() const
{
	if (!ReceiptId.IsValid()
		|| !Request.IsValid()
		|| Reveals.Num() > Request.GetDefinition().GetMaximumResults())
	{
		return false;
	}

	TSet<FGuid> SubjectIds;
	for (int32 Index = 0; Index < Reveals.Num(); ++Index)
	{
		const FShanmenDivineSenseReveal& Reveal = Reveals[Index];
		double ExpectedDistanceSquared = 0.0;
		if (!Reveal.IsValid()
			|| Reveal.GetObservation().GetScanId() != Request.GetScanId()
			|| SubjectIds.Contains(
				Reveal.GetObservation().GetSubjectEntityId())
			|| !IsRevealable(
				Request, Reveal.GetObservation(), ExpectedDistanceSquared)
			|| Reveal.GetDistanceSquared() != ExpectedDistanceSquared
			|| (Index > 0 && RevealComesBefore(Reveal, Reveals[Index - 1])))
		{
			return false;
		}
		SubjectIds.Add(Reveal.GetObservation().GetSubjectEntityId());
	}
	return ReceiptId == MakeReceiptId(Request, Reveals);
}

bool FShanmenDivineSenseResolver::TryResolve(
	const FShanmenDivineSenseScanRequest& Request,
	const TArray<FShanmenDivineSenseObservation>& Observations,
	FShanmenDivineSenseScanReceipt& OutReceipt)
{
	OutReceipt = FShanmenDivineSenseScanReceipt();
	if (!Request.IsValid())
	{
		return false;
	}

	TMap<FGuid, FGuid> ObservationIdBySubject;
	TArray<FShanmenDivineSenseObservation> UniqueObservations;
	UniqueObservations.Reserve(Observations.Num());
	for (const FShanmenDivineSenseObservation& Observation : Observations)
	{
		if (!Observation.IsValid()
			|| Observation.GetScanId() != Request.GetScanId())
		{
			return false;
		}

		const FGuid* ExistingId = ObservationIdBySubject.Find(
			Observation.GetSubjectEntityId());
		if (ExistingId)
		{
			if (*ExistingId != Observation.GetObservationId())
			{
				return false;
			}
			continue;
		}
		ObservationIdBySubject.Add(
			Observation.GetSubjectEntityId(),
			Observation.GetObservationId());
		UniqueObservations.Add(Observation);
	}

	TArray<FShanmenDivineSenseReveal> Qualified;
	Qualified.Reserve(UniqueObservations.Num());
	for (const FShanmenDivineSenseObservation& Observation :
		UniqueObservations)
	{
		double DistanceSquared = 0.0;
		if (!TryDistanceSquared(Request, Observation, DistanceSquared))
		{
			return false;
		}
		if (!IsRevealable(Request, Observation, DistanceSquared))
		{
			continue;
		}

		FShanmenDivineSenseReveal Reveal;
		Reveal.Observation = Observation;
		Reveal.DistanceSquared = CanonicalZero(DistanceSquared);
		Reveal.RevealId = MakeRevealId(
			Reveal.Observation, Reveal.DistanceSquared);
		if (!Reveal.IsValid())
		{
			return false;
		}
		Qualified.Add(Reveal);
	}

	Qualified.Sort(RevealComesBefore);
	if (Qualified.Num() > Request.GetDefinition().GetMaximumResults())
	{
		Qualified.SetNum(
			Request.GetDefinition().GetMaximumResults(),
			EAllowShrinking::No);
	}

	FShanmenDivineSenseScanReceipt Candidate;
	Candidate.Request = Request;
	Candidate.Reveals = MoveTemp(Qualified);
	Candidate.ReceiptId = MakeReceiptId(
		Candidate.Request, Candidate.Reveals);
	if (!Candidate.IsValid())
	{
		return false;
	}
	OutReceipt = MoveTemp(Candidate);
	return true;
}
