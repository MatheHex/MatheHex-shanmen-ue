#include "demo_mapShanmenMeridianShockTreatmentRecoveryIntent.h"

#include "ShanmenDeterministicId.h"
#include "demo_mapItemDefinitions.h"

namespace
{
	constexpr int32 EncodedPartCount = 13;
	const TCHAR* EncodingMagic =
		TEXT("SHANMEN_MERIDIAN_TREATMENT_RECOVERY_INTENT");

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

	bool MatchesCanonicalSample(
		const FGuid& TimelineId,
		const FGuid& TimelineSampleId,
		const int64 TreatmentTick)
	{
		Fdemo_mapShanmenCombatRunTimelineSample Sample;
		return Fdemo_mapShanmenCombatRunTimelineSample::TryCapture(
			TimelineId,
			TreatmentTick,
			Sample)
			&& Sample.GetSampleId() == TimelineSampleId;
	}
}

bool Fdemo_mapShanmenMeridianShockTreatmentRecoveryIntent::TryCapture(
	const Fdemo_mapShanmenCombatConditionTreatmentIntent& TreatmentIntent,
	const Fdemo_mapShanmenCombatRunTimelineSample& TimelineSample,
	Fdemo_mapShanmenMeridianShockTreatmentRecoveryIntent& OutIntent)
{
	OutIntent = Fdemo_mapShanmenMeridianShockTreatmentRecoveryIntent();
	if (!TreatmentIntent.IsValid()
		|| !TimelineSample.IsValid()
		|| TimelineSample.GetTimelineId() != TreatmentIntent.GetTimelineId())
	{
		return false;
	}

	Fdemo_mapShanmenMeridianShockTreatmentRecoveryIntent Candidate;
	Candidate.SchemaVersion = CurrentSchemaVersion;
	Candidate.TreatmentId = TreatmentIntent.GetTreatmentId();
	Candidate.RunId = TreatmentIntent.GetRunId();
	Candidate.TargetEntityId = TreatmentIntent.GetTargetEntityId();
	Candidate.TimelineId = TreatmentIntent.GetTimelineId();
	Candidate.TimelineSampleId = TimelineSample.GetSampleId();
	Candidate.ItemInstanceId = TreatmentIntent.GetItemInstanceId();
	Candidate.ItemDefinitionId = TreatmentIntent.GetItemDefinitionId();
	Candidate.ConditionDefinitionId =
		TreatmentIntent.GetConditionDefinitionId();
	Candidate.TreatmentTick = TimelineSample.GetCurrentTick();
	Candidate.ExpectedConditionRevision =
		TreatmentIntent.GetExpectedConditionRevision();
	Candidate.IntentId = MakeIntentId(
		Candidate.SchemaVersion,
		Candidate.TreatmentId,
		Candidate.RunId,
		Candidate.TargetEntityId,
		Candidate.TimelineId,
		Candidate.TimelineSampleId,
		Candidate.ItemInstanceId,
		Candidate.ItemDefinitionId,
		Candidate.ConditionDefinitionId,
		Candidate.TreatmentTick,
		Candidate.ExpectedConditionRevision);
	if (!Candidate.IsValid())
	{
		return false;
	}
	OutIntent = Candidate;
	return true;
}

bool Fdemo_mapShanmenMeridianShockTreatmentRecoveryIntent::TryDecode(
	const FString& Encoded,
	Fdemo_mapShanmenMeridianShockTreatmentRecoveryIntent& OutIntent)
{
	OutIntent = Fdemo_mapShanmenMeridianShockTreatmentRecoveryIntent();
	TArray<FString> Parts;
	Encoded.ParseIntoArray(Parts, TEXT("|"), false);
	if (Parts.Num() != EncodedPartCount
		|| Parts[0] != EncodingMagic
		|| Parts[1] != TEXT("1"))
	{
		return false;
	}

	Fdemo_mapShanmenMeridianShockTreatmentRecoveryIntent Candidate;
	Candidate.SchemaVersion = CurrentSchemaVersion;
	if (!FGuid::ParseExact(Parts[2], EGuidFormats::Digits, Candidate.IntentId)
		|| !FGuid::ParseExact(
			Parts[3], EGuidFormats::Digits, Candidate.TreatmentId)
		|| !FGuid::ParseExact(Parts[4], EGuidFormats::Digits, Candidate.RunId)
		|| !FGuid::ParseExact(
			Parts[5], EGuidFormats::Digits, Candidate.TargetEntityId)
		|| !FGuid::ParseExact(
			Parts[6], EGuidFormats::Digits, Candidate.TimelineId)
		|| !FGuid::ParseExact(
			Parts[7], EGuidFormats::Digits, Candidate.TimelineSampleId)
		|| !FGuid::ParseExact(
			Parts[8], EGuidFormats::Digits, Candidate.ItemInstanceId)
		|| Parts[9]
			!= Fdemo_mapItemIds::MeridianStabilizingPillLevel1.ToString()
		|| Parts[10]
			!= Udemo_mapShanmenCombatConditionComponent::
				MeridianShockDefinitionId().ToString()
		|| !TryParseCanonicalInt64(Parts[11], Candidate.TreatmentTick)
		|| !TryParseCanonicalInt64(
			Parts[12], Candidate.ExpectedConditionRevision))
	{
		return false;
	}
	Candidate.ItemDefinitionId = FName(*Parts[9]);
	Candidate.ConditionDefinitionId = FName(*Parts[10]);
	if (!Candidate.IsValid())
	{
		return false;
	}
	OutIntent = Candidate;
	return true;
}

bool Fdemo_mapShanmenMeridianShockTreatmentRecoveryIntent::TryEncode(
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
			GuidDigits(IntentId),
			GuidDigits(TreatmentId),
			GuidDigits(RunId),
			GuidDigits(TargetEntityId),
			GuidDigits(TimelineId),
			GuidDigits(TimelineSampleId),
			GuidDigits(ItemInstanceId),
			ItemDefinitionId.ToString(),
			ConditionDefinitionId.ToString(),
			CanonicalInt64(TreatmentTick),
			CanonicalInt64(ExpectedConditionRevision)
		};
	OutEncoded = FString::Join(Parts, TEXT("|"));
	return !OutEncoded.IsEmpty();
}

bool Fdemo_mapShanmenMeridianShockTreatmentRecoveryIntent::TryRestore(
	Fdemo_mapShanmenCombatConditionTreatmentIntent& OutIntent,
	Fdemo_mapShanmenCombatRunTimelineSample& OutTimelineSample) const
{
	OutIntent = Fdemo_mapShanmenCombatConditionTreatmentIntent();
	OutTimelineSample = Fdemo_mapShanmenCombatRunTimelineSample();
	if (!IsValid()
		|| !Fdemo_mapShanmenCombatConditionTreatmentIntent::TryCapture(
			RunId,
			TargetEntityId,
			TimelineId,
			ItemInstanceId,
			ItemDefinitionId,
			ExpectedConditionRevision,
			OutIntent)
		|| !Fdemo_mapShanmenCombatRunTimelineSample::TryCapture(
			TimelineId,
			TreatmentTick,
			OutTimelineSample)
		|| OutIntent.GetTreatmentId() != TreatmentId
		|| OutTimelineSample.GetSampleId() != TimelineSampleId)
	{
		OutIntent = Fdemo_mapShanmenCombatConditionTreatmentIntent();
		OutTimelineSample = Fdemo_mapShanmenCombatRunTimelineSample();
		return false;
	}
	return true;
}

bool Fdemo_mapShanmenMeridianShockTreatmentRecoveryIntent::IsValid() const
{
	return SchemaVersion == CurrentSchemaVersion
		&& IntentId.IsValid()
		&& TreatmentId.IsValid()
		&& RunId.IsValid()
		&& TargetEntityId.IsValid()
		&& TimelineId.IsValid()
		&& TimelineSampleId.IsValid()
		&& ItemInstanceId.IsValid()
		&& ItemDefinitionId
			== Fdemo_mapItemIds::MeridianStabilizingPillLevel1
		&& ConditionDefinitionId
			== Udemo_mapShanmenCombatConditionComponent::
				MeridianShockDefinitionId()
		&& TreatmentTick >= 0
		&& ExpectedConditionRevision > 0
		&& ExpectedConditionRevision < MAX_int64
		&& TimelineId
			== Fdemo_mapShanmenCombatRunFixedTimeline::MakeTimelineId(RunId)
		&& MatchesCanonicalSample(
			TimelineId, TimelineSampleId, TreatmentTick)
		&& TreatmentId
			== Udemo_mapShanmenCombatConditionComponent::MakeTreatmentId(
				RunId,
				TargetEntityId,
				TimelineId,
				ItemInstanceId,
				ItemDefinitionId,
				ExpectedConditionRevision)
		&& IntentId == MakeIntentId(
			SchemaVersion,
			TreatmentId,
			RunId,
			TargetEntityId,
			TimelineId,
			TimelineSampleId,
			ItemInstanceId,
			ItemDefinitionId,
			ConditionDefinitionId,
			TreatmentTick,
			ExpectedConditionRevision);
}

bool Fdemo_mapShanmenMeridianShockTreatmentRecoveryIntent::Matches(
	const Fdemo_mapShanmenMeridianShockTreatmentRecoveryIntent& Other) const
{
	return IsValid()
		&& Other.IsValid()
		&& SchemaVersion == Other.SchemaVersion
		&& IntentId == Other.IntentId
		&& TreatmentId == Other.TreatmentId
		&& RunId == Other.RunId
		&& TargetEntityId == Other.TargetEntityId
		&& TimelineId == Other.TimelineId
		&& TimelineSampleId == Other.TimelineSampleId
		&& ItemInstanceId == Other.ItemInstanceId
		&& ItemDefinitionId == Other.ItemDefinitionId
		&& ConditionDefinitionId == Other.ConditionDefinitionId
		&& TreatmentTick == Other.TreatmentTick
		&& ExpectedConditionRevision == Other.ExpectedConditionRevision;
}

FGuid Fdemo_mapShanmenMeridianShockTreatmentRecoveryIntent::MakeIntentId(
	const int32 RequestedSchemaVersion,
	const FGuid& RequestedTreatmentId,
	const FGuid& RequestedRunId,
	const FGuid& RequestedTargetEntityId,
	const FGuid& RequestedTimelineId,
	const FGuid& RequestedTimelineSampleId,
	const FGuid& RequestedItemInstanceId,
	const FName RequestedItemDefinitionId,
	const FName RequestedConditionDefinitionId,
	const int64 RequestedTreatmentTick,
	const int64 RequestedExpectedConditionRevision)
{
	if (RequestedSchemaVersion != CurrentSchemaVersion)
	{
		return FGuid();
	}
	return FShanmenDeterministicId::FromCanonicalParts(
		TEXT("demo_map.Combat.Condition.MeridianShock.TreatmentRecoveryIntent.r1"),
		{
			FString::FromInt(RequestedSchemaVersion),
			GuidDigits(RequestedTreatmentId),
			GuidDigits(RequestedRunId),
			GuidDigits(RequestedTargetEntityId),
			GuidDigits(RequestedTimelineId),
			GuidDigits(RequestedTimelineSampleId),
			GuidDigits(RequestedItemInstanceId),
			RequestedItemDefinitionId.ToString(),
			RequestedConditionDefinitionId.ToString(),
			CanonicalInt64(RequestedTreatmentTick),
			CanonicalInt64(RequestedExpectedConditionRevision)
		});
}
