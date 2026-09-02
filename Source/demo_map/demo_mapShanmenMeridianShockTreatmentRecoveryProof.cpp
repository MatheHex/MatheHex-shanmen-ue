#include "demo_mapShanmenMeridianShockTreatmentRecoveryProof.h"

#include "ShanmenDeterministicId.h"
#include "demo_mapItemDefinitions.h"

namespace
{
	constexpr int32 EncodedPartCount = 13;
	const TCHAR* EncodingMagic = TEXT("SHANMEN_MERIDIAN_TREATMENT_PROOF");

	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	FString CanonicalInt64(const int64 Value)
	{
		return FString::Printf(TEXT("%lld"), static_cast<long long>(Value));
	}

	bool TryParseCanonicalInt64(const FString& Text, int64& OutValue)
	{
		OutValue = 0;
		return LexTryParseString(OutValue, *Text)
			&& Text == CanonicalInt64(OutValue);
	}
}

bool Fdemo_mapShanmenMeridianShockTreatmentRecoveryProof::TryCapture(
	const Fdemo_mapShanmenCombatConditionTreatmentReceipt& Receipt,
	Fdemo_mapShanmenMeridianShockTreatmentRecoveryProof& OutProof)
{
	OutProof = Fdemo_mapShanmenMeridianShockTreatmentRecoveryProof();
	if (!Receipt.IsValid())
	{
		return false;
	}

	Fdemo_mapShanmenMeridianShockTreatmentRecoveryProof Candidate;
	Candidate.SchemaVersion = CurrentSchemaVersion;
	Candidate.TreatmentId = Receipt.GetTreatmentId();
	Candidate.RunId = Receipt.GetRunId();
	Candidate.TargetEntityId = Receipt.GetTargetEntityId();
	Candidate.TimelineId = Receipt.GetTimelineId();
	Candidate.ItemInstanceId = Receipt.GetItemInstanceId();
	Candidate.ItemDefinitionId = Receipt.GetItemDefinitionId();
	Candidate.ConditionDefinitionId = Receipt.GetConditionDefinitionId();
	Candidate.TreatedAtTick = Receipt.GetTreatedAtTick();
	Candidate.ConditionRevisionBefore =
		Receipt.GetConditionRevisionBefore();
	Candidate.ConditionRevisionAfter =
		Receipt.GetConditionRevisionAfter();
	Candidate.ProofId = MakeProofId(
		Candidate.SchemaVersion,
		Candidate.TreatmentId,
		Candidate.RunId,
		Candidate.TargetEntityId,
		Candidate.TimelineId,
		Candidate.ItemInstanceId,
		Candidate.ItemDefinitionId,
		Candidate.ConditionDefinitionId,
		Candidate.TreatedAtTick,
		Candidate.ConditionRevisionBefore,
		Candidate.ConditionRevisionAfter);
	if (!Candidate.IsValid())
	{
		return false;
	}
	OutProof = Candidate;
	return true;
}

bool Fdemo_mapShanmenMeridianShockTreatmentRecoveryProof::TryDecode(
	const FString& Encoded,
	Fdemo_mapShanmenMeridianShockTreatmentRecoveryProof& OutProof)
{
	OutProof = Fdemo_mapShanmenMeridianShockTreatmentRecoveryProof();
	TArray<FString> Parts;
	Encoded.ParseIntoArray(Parts, TEXT("|"), false);
	if (Parts.Num() != EncodedPartCount
		|| Parts[0] != EncodingMagic
		|| Parts[1] != TEXT("1"))
	{
		return false;
	}

	Fdemo_mapShanmenMeridianShockTreatmentRecoveryProof Candidate;
	Candidate.SchemaVersion = CurrentSchemaVersion;
	if (!FGuid::ParseExact(Parts[2], EGuidFormats::Digits, Candidate.ProofId)
		|| !FGuid::ParseExact(
			Parts[3], EGuidFormats::Digits, Candidate.TreatmentId)
		|| !FGuid::ParseExact(Parts[4], EGuidFormats::Digits, Candidate.RunId)
		|| !FGuid::ParseExact(
			Parts[5], EGuidFormats::Digits, Candidate.TargetEntityId)
		|| !FGuid::ParseExact(
			Parts[6], EGuidFormats::Digits, Candidate.TimelineId)
		|| !FGuid::ParseExact(
			Parts[7], EGuidFormats::Digits, Candidate.ItemInstanceId)
		|| Parts[8]
			!= Fdemo_mapItemIds::MeridianStabilizingPillLevel1.ToString()
		|| Parts[9]
			!= Udemo_mapShanmenCombatConditionComponent::
				MeridianShockDefinitionId().ToString()
		|| !TryParseCanonicalInt64(Parts[10], Candidate.TreatedAtTick)
		|| !TryParseCanonicalInt64(
			Parts[11], Candidate.ConditionRevisionBefore)
		|| !TryParseCanonicalInt64(
			Parts[12], Candidate.ConditionRevisionAfter))
	{
		return false;
	}
	Candidate.ItemDefinitionId = FName(*Parts[8]);
	Candidate.ConditionDefinitionId = FName(*Parts[9]);
	if (!Candidate.IsValid())
	{
		return false;
	}
	OutProof = Candidate;
	return true;
}

bool Fdemo_mapShanmenMeridianShockTreatmentRecoveryProof::TryEncode(
	FString& OutEncoded) const
{
	OutEncoded.Reset();
	if (!IsValid())
	{
		return false;
	}
	const TArray<FString> Parts =
		{
			EncodingMagic,
			TEXT("1"),
			GuidDigits(ProofId),
			GuidDigits(TreatmentId),
			GuidDigits(RunId),
			GuidDigits(TargetEntityId),
			GuidDigits(TimelineId),
			GuidDigits(ItemInstanceId),
			ItemDefinitionId.ToString(),
			ConditionDefinitionId.ToString(),
			CanonicalInt64(TreatedAtTick),
			CanonicalInt64(ConditionRevisionBefore),
			CanonicalInt64(ConditionRevisionAfter)
		};
	OutEncoded = FString::Join(Parts, TEXT("|"));
	return !OutEncoded.IsEmpty();
}

bool Fdemo_mapShanmenMeridianShockTreatmentRecoveryProof::TryRestore(
	Fdemo_mapShanmenCombatConditionTreatmentIntent& OutIntent,
	Fdemo_mapShanmenCombatConditionTreatmentReceipt& OutReceipt) const
{
	OutIntent = Fdemo_mapShanmenCombatConditionTreatmentIntent();
	OutReceipt = Fdemo_mapShanmenCombatConditionTreatmentReceipt();
	if (!IsValid()
		|| !Fdemo_mapShanmenCombatConditionTreatmentIntent::TryCapture(
			RunId,
			TargetEntityId,
			TimelineId,
			ItemInstanceId,
			ItemDefinitionId,
			ConditionRevisionBefore,
			OutIntent))
	{
		return false;
	}

	OutReceipt.TreatmentId = TreatmentId;
	OutReceipt.RunId = RunId;
	OutReceipt.TargetEntityId = TargetEntityId;
	OutReceipt.TimelineId = TimelineId;
	OutReceipt.ItemInstanceId = ItemInstanceId;
	OutReceipt.ItemDefinitionId = ItemDefinitionId;
	OutReceipt.ConditionDefinitionId = ConditionDefinitionId;
	OutReceipt.TreatedAtTick = TreatedAtTick;
	OutReceipt.ConditionRevisionBefore = ConditionRevisionBefore;
	OutReceipt.ConditionRevisionAfter = ConditionRevisionAfter;
	if (!OutReceipt.IsValid() || !OutReceipt.Matches(OutIntent))
	{
		OutIntent = Fdemo_mapShanmenCombatConditionTreatmentIntent();
		OutReceipt = Fdemo_mapShanmenCombatConditionTreatmentReceipt();
		return false;
	}
	return true;
}

bool Fdemo_mapShanmenMeridianShockTreatmentRecoveryProof::IsValid() const
{
	return SchemaVersion == CurrentSchemaVersion
		&& ProofId.IsValid()
		&& TreatmentId.IsValid()
		&& RunId.IsValid()
		&& TargetEntityId.IsValid()
		&& TimelineId.IsValid()
		&& ItemInstanceId.IsValid()
		&& ItemDefinitionId
			== Fdemo_mapItemIds::MeridianStabilizingPillLevel1
		&& ConditionDefinitionId
			== Udemo_mapShanmenCombatConditionComponent::
				MeridianShockDefinitionId()
		&& TreatedAtTick >= 0
		&& ConditionRevisionBefore > 0
		&& ConditionRevisionBefore < MAX_int64
		&& ConditionRevisionAfter == ConditionRevisionBefore + 1
		&& TreatmentId
			== Udemo_mapShanmenCombatConditionComponent::MakeTreatmentId(
				RunId,
				TargetEntityId,
				TimelineId,
				ItemInstanceId,
				ItemDefinitionId,
				ConditionRevisionBefore)
		&& ProofId == MakeProofId(
			SchemaVersion,
			TreatmentId,
			RunId,
			TargetEntityId,
			TimelineId,
			ItemInstanceId,
			ItemDefinitionId,
			ConditionDefinitionId,
			TreatedAtTick,
			ConditionRevisionBefore,
			ConditionRevisionAfter);
}

bool Fdemo_mapShanmenMeridianShockTreatmentRecoveryProof::Matches(
	const Fdemo_mapShanmenMeridianShockTreatmentRecoveryProof& Other) const
{
	return IsValid()
		&& Other.IsValid()
		&& SchemaVersion == Other.SchemaVersion
		&& ProofId == Other.ProofId
		&& TreatmentId == Other.TreatmentId
		&& RunId == Other.RunId
		&& TargetEntityId == Other.TargetEntityId
		&& TimelineId == Other.TimelineId
		&& ItemInstanceId == Other.ItemInstanceId
		&& ItemDefinitionId == Other.ItemDefinitionId
		&& ConditionDefinitionId == Other.ConditionDefinitionId
		&& TreatedAtTick == Other.TreatedAtTick
		&& ConditionRevisionBefore == Other.ConditionRevisionBefore
		&& ConditionRevisionAfter == Other.ConditionRevisionAfter;
}

FGuid Fdemo_mapShanmenMeridianShockTreatmentRecoveryProof::MakeProofId(
	const int32 RequestedSchemaVersion,
	const FGuid& RequestedTreatmentId,
	const FGuid& RequestedRunId,
	const FGuid& RequestedTargetEntityId,
	const FGuid& RequestedTimelineId,
	const FGuid& RequestedItemInstanceId,
	const FName RequestedItemDefinitionId,
	const FName RequestedConditionDefinitionId,
	const int64 RequestedTreatedAtTick,
	const int64 RequestedConditionRevisionBefore,
	const int64 RequestedConditionRevisionAfter)
{
	if (RequestedSchemaVersion != CurrentSchemaVersion)
	{
		return FGuid();
	}
	return FShanmenDeterministicId::FromCanonicalParts(
		TEXT("demo_map.Combat.Condition.MeridianShock.TreatmentProof.r1"),
		{
			FString::FromInt(RequestedSchemaVersion),
			GuidDigits(RequestedTreatmentId),
			GuidDigits(RequestedRunId),
			GuidDigits(RequestedTargetEntityId),
			GuidDigits(RequestedTimelineId),
			GuidDigits(RequestedItemInstanceId),
			RequestedItemDefinitionId.ToString(),
			RequestedConditionDefinitionId.ToString(),
			CanonicalInt64(RequestedTreatedAtTick),
			CanonicalInt64(RequestedConditionRevisionBefore),
			CanonicalInt64(RequestedConditionRevisionAfter)
		});
}
