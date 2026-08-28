#include "ShanmenVitalityAuthority.h"

#include "ShanmenCombatResolver.h"
#include "ShanmenDeterministicId.h"

namespace
{
	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	uint32 FloatValueBits(float Value)
	{
		uint32 Bits = 0;
		static_assert(sizeof(Bits) == sizeof(Value));
		FMemory::Memcpy(&Bits, &Value, sizeof(Bits));
		return Bits;
	}

	FString FloatBits(float Value)
	{
		return FString::Printf(TEXT("%08X"), FloatValueBits(Value));
	}

	bool FloatsMatchExactly(float Left, float Right)
	{
		return FloatValueBits(Left) == FloatValueBits(Right);
	}

	bool IsValidVitalityState(float CurrentVitality, float MaximumVitality)
	{
		return FMath::IsFinite(CurrentVitality)
			&& FMath::IsFinite(MaximumVitality)
			&& CurrentVitality >= 0.0f
			&& MaximumVitality >= CurrentVitality;
	}

	void AppendSortedTags(
		const FGameplayTagContainer& Tags,
		TArray<FString>& InOutParts)
	{
		TArray<FGameplayTag> SortedTags;
		Tags.GetGameplayTagArray(SortedTags);
		SortedTags.Sort([](const FGameplayTag& Left, const FGameplayTag& Right)
		{
			return Left.ToString() < Right.ToString();
		});
		InOutParts.Add(FString::FromInt(SortedTags.Num()));
		for (const FGameplayTag& Tag : SortedTags)
		{
			InOutParts.Add(Tag.ToString());
		}
	}

	bool LayerResultsMatch(
		const FShanmenDefenseLayerResult& Left,
		const FShanmenDefenseLayerResult& Right)
	{
		return Left.LayerId == Right.LayerId
			&& Left.RuleId == Right.RuleId
			&& Left.SourceInstanceId == Right.SourceInstanceId
			&& Left.Operation == Right.Operation
			&& Left.Order == Right.Order
			&& FloatsMatchExactly(Left.PreventedDamage, Right.PreventedDamage)
			&& Left.bRequiresCommit == Right.bRequiresCommit
			&& Left.LayerTags == Right.LayerTags;
	}

	bool ResultsMatch(
		const FShanmenImpactResult& Left,
		const FShanmenImpactResult& Right)
	{
		if (Left.bAccepted != Right.bAccepted
			|| Left.ImpactId != Right.ImpactId
			|| Left.Outcome != Right.Outcome
			|| !FloatsMatchExactly(Left.RawDamage, Right.RawDamage)
			|| !FloatsMatchExactly(Left.PreventedDamage, Right.PreventedDamage)
			|| !FloatsMatchExactly(Left.FinalDamage, Right.FinalDamage)
			|| Left.TriggeredLayers.Num() != Right.TriggeredLayers.Num())
		{
			return false;
		}

		for (int32 Index = 0; Index < Left.TriggeredLayers.Num(); ++Index)
		{
			if (!LayerResultsMatch(Left.TriggeredLayers[Index], Right.TriggeredLayers[Index]))
			{
				return false;
			}
		}
		return true;
	}

	FGuid MakeResolutionId(
		const FShanmenImpactRequest& Request,
		const FShanmenImpactResult& Result)
	{
		TArray<FString> Parts{
			GuidDigits(Request.ImpactId),
			GuidDigits(Request.Action.GetActivationId()),
			GuidDigits(Request.Candidate.TargetEntityId),
			FString::Printf(TEXT("%lld"), static_cast<long long>(Request.TargetVitality.AuthorityRevision)),
			FloatBits(Request.TargetVitality.CurrentVitality),
			FloatBits(Request.TargetVitality.MaximumVitality),
			Request.Action.GetContent().Version.ToString(),
			Request.Action.GetContent().Digest,
			Request.Damage.FormulaId.ToString(),
			FloatBits(Result.RawDamage),
			FloatBits(Result.PreventedDamage),
			FloatBits(Result.FinalDamage),
			FString::FromInt(static_cast<uint8>(Result.Outcome)),
			FString::FromInt(Result.TriggeredLayers.Num())
		};

		AppendSortedTags(Request.Damage.DamageTags, Parts);
		for (const FShanmenDefenseLayerResult& Layer : Result.TriggeredLayers)
		{
			Parts.Add(GuidDigits(Layer.LayerId));
			Parts.Add(Layer.RuleId.ToString());
			Parts.Add(GuidDigits(Layer.SourceInstanceId));
			Parts.Add(FString::FromInt(static_cast<uint8>(Layer.Operation)));
			Parts.Add(FString::FromInt(Layer.Order));
			Parts.Add(FloatBits(Layer.PreventedDamage));
			Parts.Add(Layer.bRequiresCommit ? TEXT("1") : TEXT("0"));
			AppendSortedTags(Layer.LayerTags, Parts);
		}

		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Combat.VitalityCommit.r1"),
			Parts);
	}

}

bool FShanmenVitalityCommitCommand::TryCreate(
	const FShanmenImpactRequest& Request,
	const FShanmenImpactResult& Result,
	FShanmenVitalityCommitCommand& OutCommand)
{
	OutCommand = FShanmenVitalityCommitCommand();
	if (!Request.IsValid()
		|| !Result.bAccepted
		|| Result.ImpactId != Request.ImpactId
		|| !Result.IsConserved()
		|| !ResultsMatch(FShanmenDefenseResolver::Resolve(Request), Result))
	{
		return false;
	}

	OutCommand.ImpactId = Request.ImpactId;
	OutCommand.TargetEntityId = Request.Candidate.TargetEntityId;
	OutCommand.ExpectedAuthorityRevision = Request.TargetVitality.AuthorityRevision;
	OutCommand.ExpectedCurrentVitality = Request.TargetVitality.CurrentVitality;
	OutCommand.ExpectedMaximumVitality = Request.TargetVitality.MaximumVitality;
	OutCommand.RawDamage = Result.RawDamage;
	OutCommand.PreventedDamage = Result.PreventedDamage;
	OutCommand.RequestedDamage = Result.FinalDamage;
	OutCommand.DefenseOutcome = Result.Outcome;
	OutCommand.ResolutionId = MakeResolutionId(Request, Result);
	return OutCommand.IsValid();
}

bool FShanmenVitalityCommitCommand::IsValid() const
{
	return ImpactId.IsValid()
		&& ResolutionId.IsValid()
		&& TargetEntityId.IsValid()
		&& ExpectedAuthorityRevision >= 0
		&& FMath::IsFinite(ExpectedCurrentVitality)
		&& FMath::IsFinite(ExpectedMaximumVitality)
		&& ExpectedCurrentVitality >= 0.0f
		&& ExpectedMaximumVitality >= ExpectedCurrentVitality
		&& FMath::IsFinite(RawDamage)
		&& FMath::IsFinite(PreventedDamage)
		&& FMath::IsFinite(RequestedDamage)
		&& RawDamage >= 0.0f
		&& PreventedDamage >= 0.0f
		&& RequestedDamage >= 0.0f
		&& DefenseOutcome != EShanmenDefenseOutcome::Invalid
		&& FMath::IsNearlyEqual(RawDamage, PreventedDamage + RequestedDamage);
}

bool FShanmenVitalityCommitReceipt::IsValid() const
{
	if (!ImpactId.IsValid()
		|| !ResolutionId.IsValid()
		|| !TargetEntityId.IsValid()
		|| AuthorityRevisionBefore < 0
		|| AuthorityRevisionBefore == MAX_int64
		|| AuthorityRevisionAfter != AuthorityRevisionBefore + 1
		|| !FMath::IsFinite(VitalityBefore)
		|| !FMath::IsFinite(VitalityAfter)
		|| !FMath::IsFinite(MaximumVitality)
		|| !FMath::IsFinite(RequestedDamage)
		|| !FMath::IsFinite(AppliedDamage)
		|| VitalityBefore < 0.0f
		|| VitalityBefore > MaximumVitality
		|| VitalityAfter < 0.0f
		|| VitalityAfter > VitalityBefore
		|| RequestedDamage < 0.0f
		|| AppliedDamage < 0.0f)
	{
		return false;
	}

	return FMath::IsNearlyEqual(AppliedDamage, FMath::Min(VitalityBefore, RequestedDamage))
		&& FMath::IsNearlyEqual(VitalityAfter, VitalityBefore - AppliedDamage);
}

bool FShanmenVitalityCommitResult::IsValid() const
{
	switch (Status)
	{
	case EShanmenVitalityCommitStatus::Committed:
	case EShanmenVitalityCommitStatus::AlreadyCommitted:
		return Error == EShanmenVitalityCommitError::None && Receipt.IsValid();
	case EShanmenVitalityCommitStatus::Rejected:
		return Error != EShanmenVitalityCommitError::None && !Receipt.IsValid();
	default:
		return false;
	}
}

bool FShanmenVitalityCommitResult::IsSuccess() const
{
	return IsValid()
		&& (Status == EShanmenVitalityCommitStatus::Committed
			|| Status == EShanmenVitalityCommitStatus::AlreadyCommitted);
}

bool FShanmenVitalityCommitLedger::TryCreate(
	const FGuid& InTargetEntityId,
	float InCurrentVitality,
	float InMaximumVitality,
	int64 InAuthorityRevision,
	FShanmenVitalityCommitLedger& OutLedger)
{
	OutLedger.Reset();
	if (!InTargetEntityId.IsValid()
		|| !IsValidVitalityState(InCurrentVitality, InMaximumVitality)
		|| InAuthorityRevision < 0)
	{
		return false;
	}

	OutLedger.TargetEntityId = InTargetEntityId;
	OutLedger.AuthorityRevision = InAuthorityRevision;
	OutLedger.CurrentVitalityFingerprint = FloatValueBits(InCurrentVitality);
	OutLedger.MaximumVitalityFingerprint = FloatValueBits(InMaximumVitality);
	OutLedger.bInitialized = true;
	return true;
}

bool FShanmenVitalityCommitLedger::IsValid() const
{
	return bInitialized
		&& TargetEntityId.IsValid()
		&& AuthorityRevision >= 0;
}

bool FShanmenVitalityCommitLedger::IsSynchronized(
	float CurrentVitality,
	float MaximumVitality) const
{
	return IsValid()
		&& IsValidVitalityState(CurrentVitality, MaximumVitality)
		&& CurrentVitalityFingerprint == FloatValueBits(CurrentVitality)
		&& MaximumVitalityFingerprint == FloatValueBits(MaximumVitality);
}

bool FShanmenVitalityCommitLedger::TryCaptureSnapshot(
	float CurrentVitality,
	float MaximumVitality,
	FShanmenTargetVitalitySnapshot& OutSnapshot) const
{
	OutSnapshot = FShanmenTargetVitalitySnapshot();
	if (!IsSynchronized(CurrentVitality, MaximumVitality))
	{
		return false;
	}

	OutSnapshot.CurrentVitality = CurrentVitality;
	OutSnapshot.MaximumVitality = MaximumVitality;
	OutSnapshot.AuthorityRevision = AuthorityRevision;
	return OutSnapshot.IsValid();
}

FShanmenVitalityCommitResult FShanmenVitalityCommitLedger::Commit(
	const FShanmenVitalityCommitCommand& Command,
	float& InOutCurrentVitality,
	float MaximumVitality)
{
	if (!Command.IsValid())
	{
		return Reject(EShanmenVitalityCommitError::InvalidCommand);
	}
	if (!IsValid())
	{
		return Reject(EShanmenVitalityCommitError::AuthorityNotReady);
	}
	if (!IsSynchronized(InOutCurrentVitality, MaximumVitality))
	{
		return Reject(EShanmenVitalityCommitError::StateDesynchronized);
	}

	if (const FProcessedImpact* Existing = ProcessedImpacts.Find(Command.GetImpactId()))
	{
		if (Existing->ResolutionId != Command.GetResolutionId())
		{
			return Reject(EShanmenVitalityCommitError::ImpactConflict);
		}

		FShanmenVitalityCommitResult Result;
		Result.Status = EShanmenVitalityCommitStatus::AlreadyCommitted;
		Result.Receipt = Existing->Receipt;
		return Result;
	}

	if (Command.GetTargetEntityId() != TargetEntityId)
	{
		return Reject(EShanmenVitalityCommitError::TargetMismatch);
	}
	if (Command.GetExpectedAuthorityRevision() != AuthorityRevision
		|| !FloatsMatchExactly(Command.GetExpectedCurrentVitality(), InOutCurrentVitality)
		|| !FloatsMatchExactly(Command.GetExpectedMaximumVitality(), MaximumVitality))
	{
		return Reject(EShanmenVitalityCommitError::StaleSnapshot);
	}
	if (AuthorityRevision == MAX_int64)
	{
		return Reject(EShanmenVitalityCommitError::RevisionExhausted);
	}

	FShanmenVitalityCommitReceipt Receipt;
	Receipt.ImpactId = Command.GetImpactId();
	Receipt.ResolutionId = Command.GetResolutionId();
	Receipt.TargetEntityId = TargetEntityId;
	Receipt.AuthorityRevisionBefore = AuthorityRevision;
	Receipt.AuthorityRevisionAfter = AuthorityRevision + 1;
	Receipt.VitalityBefore = InOutCurrentVitality;
	Receipt.MaximumVitality = MaximumVitality;
	Receipt.RequestedDamage = Command.GetRequestedDamage();
	Receipt.AppliedDamage = FMath::Min(InOutCurrentVitality, Command.GetRequestedDamage());
	Receipt.VitalityAfter = FMath::Max(0.0f, InOutCurrentVitality - Receipt.AppliedDamage);
	if (!Receipt.IsValid())
	{
		return Reject(EShanmenVitalityCommitError::InvalidCommand);
	}

	InOutCurrentVitality = Receipt.VitalityAfter;
	AuthorityRevision = Receipt.AuthorityRevisionAfter;
	CurrentVitalityFingerprint = FloatValueBits(InOutCurrentVitality);
	MaximumVitalityFingerprint = FloatValueBits(MaximumVitality);
	FProcessedImpact& Processed = ProcessedImpacts.Add(Receipt.ImpactId);
	Processed.ResolutionId = Receipt.ResolutionId;
	Processed.Receipt = Receipt;

	FShanmenVitalityCommitResult Result;
	Result.Status = EShanmenVitalityCommitStatus::Committed;
	Result.Receipt = Receipt;
	return Result;
}

bool FShanmenVitalityCommitLedger::TryCommitExternalMutation(
	float& InOutCurrentVitality,
	float& InOutMaximumVitality,
	float NewCurrentVitality,
	float NewMaximumVitality)
{
	if (!IsSynchronized(InOutCurrentVitality, InOutMaximumVitality)
		|| !IsValidVitalityState(NewCurrentVitality, NewMaximumVitality))
	{
		return false;
	}

	const uint32 NewCurrentFingerprint = FloatValueBits(NewCurrentVitality);
	const uint32 NewMaximumFingerprint = FloatValueBits(NewMaximumVitality);
	if (CurrentVitalityFingerprint == NewCurrentFingerprint
		&& MaximumVitalityFingerprint == NewMaximumFingerprint)
	{
		return true;
	}
	if (AuthorityRevision == MAX_int64)
	{
		return false;
	}

	InOutCurrentVitality = NewCurrentVitality;
	InOutMaximumVitality = NewMaximumVitality;
	CurrentVitalityFingerprint = NewCurrentFingerprint;
	MaximumVitalityFingerprint = NewMaximumFingerprint;
	++AuthorityRevision;
	return true;
}

void FShanmenVitalityCommitLedger::Reset()
{
	*this = FShanmenVitalityCommitLedger();
}

FShanmenVitalityCommitResult FShanmenVitalityCommitLedger::Reject(
	EShanmenVitalityCommitError Error) const
{
	FShanmenVitalityCommitResult Result;
	Result.Status = EShanmenVitalityCommitStatus::Rejected;
	Result.Error = Error;
	return Result;
}

bool FShanmenVitalityAuthority::TryCreate(
	const FGuid& InTargetEntityId,
	float InCurrentVitality,
	float InMaximumVitality,
	int64 InAuthorityRevision,
	FShanmenVitalityAuthority& OutAuthority)
{
	OutAuthority.Reset();
	if (!FShanmenVitalityCommitLedger::TryCreate(
		InTargetEntityId,
		InCurrentVitality,
		InMaximumVitality,
		InAuthorityRevision,
		OutAuthority.CommitLedger))
	{
		return false;
	}

	OutAuthority.CurrentVitality = InCurrentVitality;
	OutAuthority.MaximumVitality = InMaximumVitality;
	return true;
}

bool FShanmenVitalityAuthority::IsValid() const
{
	return CommitLedger.IsSynchronized(CurrentVitality, MaximumVitality);
}

bool FShanmenVitalityAuthority::TryCaptureSnapshot(
	FShanmenTargetVitalitySnapshot& OutSnapshot) const
{
	return CommitLedger.TryCaptureSnapshot(
		CurrentVitality,
		MaximumVitality,
		OutSnapshot);
}

FShanmenVitalityCommitResult FShanmenVitalityAuthority::Commit(
	const FShanmenVitalityCommitCommand& Command)
{
	return CommitLedger.Commit(
		Command,
		CurrentVitality,
		MaximumVitality);
}

void FShanmenVitalityAuthority::Reset()
{
	*this = FShanmenVitalityAuthority();
}
