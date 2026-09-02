#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenMeridianShockTreatmentRecoveryIntent.h"

#include "demo_mapItemDefinitions.h"
#include "Misc/AutomationTest.h"

namespace
{
	constexpr EAutomationTestFlags RecoveryIntentFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;

	const FGuid RecoveryIntentRunId(0xC1670001, 0, 0, 1);
	const FGuid RecoveryIntentTargetId(0xC1670001, 0, 0, 2);
	const FGuid RecoveryIntentItemId(0xC1670001, 0, 0, 3);

	bool MakeSourceValues(
		const int64 Revision,
		const int64 Tick,
		Fdemo_mapShanmenCombatConditionTreatmentIntent& OutIntent,
		Fdemo_mapShanmenCombatRunTimelineSample& OutSample)
	{
		const FGuid TimelineId =
			Fdemo_mapShanmenCombatRunFixedTimeline::MakeTimelineId(
				RecoveryIntentRunId);
		return Fdemo_mapShanmenCombatConditionTreatmentIntent::TryCapture(
			RecoveryIntentRunId,
			RecoveryIntentTargetId,
			TimelineId,
			RecoveryIntentItemId,
			Fdemo_mapItemIds::MeridianStabilizingPillLevel1,
			Revision,
			OutIntent)
			&& Fdemo_mapShanmenCombatRunTimelineSample::TryCapture(
				TimelineId,
				Tick,
				OutSample);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapMeridianShockRecoveryIntentRoundTripTest,
	"Shanmen.0_0_10.Product.MeridianShockTreatment.RecoveryIntent.CodecRoundTrip",
	RecoveryIntentFlags)

bool Fdemo_mapMeridianShockRecoveryIntentRoundTripTest::RunTest(
	const FString&)
{
	Fdemo_mapShanmenCombatConditionTreatmentIntent SourceIntent;
	Fdemo_mapShanmenCombatRunTimelineSample SourceSample;
	if (!MakeSourceValues(7, 42, SourceIntent, SourceSample))
	{
		AddError(TEXT("Could not create P16.7 recovery-intent source values."));
		return false;
	}

	Fdemo_mapShanmenMeridianShockTreatmentRecoveryIntent First;
	Fdemo_mapShanmenMeridianShockTreatmentRecoveryIntent Replayed;
	FString Encoded;
	const bool bCaptured =
		Fdemo_mapShanmenMeridianShockTreatmentRecoveryIntent::TryCapture(
			SourceIntent,
			SourceSample,
			First)
		&& Fdemo_mapShanmenMeridianShockTreatmentRecoveryIntent::TryCapture(
			SourceIntent,
			SourceSample,
			Replayed)
		&& First.TryEncode(Encoded);
	TestTrue(TEXT("same source captures one deterministic write-ahead intent"),
		bCaptured
			&& First.Matches(Replayed)
			&& First.GetIntentId() == Replayed.GetIntentId()
			&& !Encoded.IsEmpty());

	Fdemo_mapShanmenMeridianShockTreatmentRecoveryIntent Decoded;
	Fdemo_mapShanmenCombatConditionTreatmentIntent RestoredIntent;
	Fdemo_mapShanmenCombatRunTimelineSample RestoredSample;
	const bool bRoundTrip =
		Fdemo_mapShanmenMeridianShockTreatmentRecoveryIntent::TryDecode(
			Encoded,
			Decoded)
		&& Decoded.Matches(First)
		&& Decoded.TryRestore(RestoredIntent, RestoredSample);
	TestTrue(TEXT("strict codec restores the exact treatment and timeline sample"),
		bRoundTrip
			&& RestoredIntent.GetTreatmentId()
				== SourceIntent.GetTreatmentId()
			&& RestoredIntent.GetExpectedConditionRevision() == 7
			&& RestoredSample.GetSampleId() == SourceSample.GetSampleId()
			&& RestoredSample.GetCurrentTick() == 42);
	AddInfo(FString::Printf(
		TEXT("P16.7 recovery-intent id=%s treatment=%s tick=%lld revision=%lld"),
		*First.GetIntentId().ToString(EGuidFormats::Digits),
		*First.GetTreatmentId().ToString(EGuidFormats::Digits),
		static_cast<long long>(First.GetTreatmentTick()),
		static_cast<long long>(First.GetExpectedConditionRevision())));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapMeridianShockRecoveryIntentIdentityFenceTest,
	"Shanmen.0_0_10.Product.MeridianShockTreatment.RecoveryIntent.IdentityFence",
	RecoveryIntentFlags)

bool Fdemo_mapMeridianShockRecoveryIntentIdentityFenceTest::RunTest(
	const FString&)
{
	Fdemo_mapShanmenCombatConditionTreatmentIntent SourceIntent;
	Fdemo_mapShanmenCombatRunTimelineSample SourceSample;
	Fdemo_mapShanmenMeridianShockTreatmentRecoveryIntent Captured;
	FString Encoded;
	if (!MakeSourceValues(9, 64, SourceIntent, SourceSample)
		|| !Fdemo_mapShanmenMeridianShockTreatmentRecoveryIntent::TryCapture(
			SourceIntent,
			SourceSample,
			Captured)
		|| !Captured.TryEncode(Encoded))
	{
		AddError(TEXT("Could not create P16.7 identity-fence fixture."));
		return false;
	}

	TArray<FString> Parts;
	Encoded.ParseIntoArray(Parts, TEXT("|"), false);
	Fdemo_mapShanmenMeridianShockTreatmentRecoveryIntent Rejected = Captured;
	bool bRejectsTampering = Parts.Num() == 13;
	if (bRejectsTampering)
	{
		TArray<FString> FutureSchema = Parts;
		FutureSchema[1] = TEXT("2");
		bRejectsTampering =
			!Fdemo_mapShanmenMeridianShockTreatmentRecoveryIntent::TryDecode(
				FString::Join(FutureSchema, TEXT("|")),
				Rejected)
			&& !Rejected.IsValid();

		TArray<FString> WrongIntegrityId = Parts;
		WrongIntegrityId[2] =
			FGuid(0xC16700FF, 0, 0, 1).ToString(EGuidFormats::Digits);
		bRejectsTampering = bRejectsTampering
			&& !Fdemo_mapShanmenMeridianShockTreatmentRecoveryIntent::TryDecode(
				FString::Join(WrongIntegrityId, TEXT("|")),
				Rejected);

		TArray<FString> WrongSampleId = Parts;
		WrongSampleId[7] =
			FGuid(0xC16700FF, 0, 0, 2).ToString(EGuidFormats::Digits);
		bRejectsTampering = bRejectsTampering
			&& !Fdemo_mapShanmenMeridianShockTreatmentRecoveryIntent::TryDecode(
				FString::Join(WrongSampleId, TEXT("|")),
				Rejected);

		TArray<FString> NonCanonicalTick = Parts;
		NonCanonicalTick[11] = TEXT("064");
		bRejectsTampering = bRejectsTampering
			&& !Fdemo_mapShanmenMeridianShockTreatmentRecoveryIntent::TryDecode(
				FString::Join(NonCanonicalTick, TEXT("|")),
				Rejected);

		TArray<FString> ExtraPart = Parts;
		ExtraPart.Add(TEXT("unexpected"));
		bRejectsTampering = bRejectsTampering
			&& !Fdemo_mapShanmenMeridianShockTreatmentRecoveryIntent::TryDecode(
				FString::Join(ExtraPart, TEXT("|")),
				Rejected);
	}
	TestTrue(TEXT("codec rejects future, non-canonical and tampered identity"),
		bRejectsTampering);

	const FGuid ForeignRunId(0xC1670010, 0, 0, 1);
	const FGuid ForeignTimelineId =
		Fdemo_mapShanmenCombatRunFixedTimeline::MakeTimelineId(ForeignRunId);
	Fdemo_mapShanmenCombatRunTimelineSample ForeignSample;
	Fdemo_mapShanmenCombatRunTimelineSample::TryCapture(
		ForeignTimelineId,
		64,
		ForeignSample);
	Fdemo_mapShanmenMeridianShockTreatmentRecoveryIntent Mismatched;
	TestFalse(TEXT("capture rejects a timeline sample from another Run"),
		Fdemo_mapShanmenMeridianShockTreatmentRecoveryIntent::TryCapture(
			SourceIntent,
			ForeignSample,
			Mismatched));

	const FGuid NonCanonicalTimelineId(0xC1670020, 0, 0, 1);
	Fdemo_mapShanmenCombatConditionTreatmentIntent NonCanonicalIntent;
	Fdemo_mapShanmenCombatRunTimelineSample NonCanonicalSample;
	const bool bBuiltNonCanonical =
		Fdemo_mapShanmenCombatConditionTreatmentIntent::TryCapture(
			RecoveryIntentRunId,
			RecoveryIntentTargetId,
			NonCanonicalTimelineId,
			RecoveryIntentItemId,
			Fdemo_mapItemIds::MeridianStabilizingPillLevel1,
			9,
			NonCanonicalIntent)
		&& Fdemo_mapShanmenCombatRunTimelineSample::TryCapture(
			NonCanonicalTimelineId,
			64,
			NonCanonicalSample);
	TestTrue(TEXT("fixture exposes a structurally valid non-canonical timeline"),
		bBuiltNonCanonical);
	TestFalse(TEXT("write-ahead intent rejects a non-canonical Run timeline"),
		Fdemo_mapShanmenMeridianShockTreatmentRecoveryIntent::TryCapture(
			NonCanonicalIntent,
			NonCanonicalSample,
			Mismatched));

	Fdemo_mapShanmenCombatConditionTreatmentIntent ExhaustedIntent;
	Fdemo_mapShanmenCombatRunTimelineSample ExhaustedSample;
	const bool bBuiltExhausted = MakeSourceValues(
		MAX_int64,
		64,
		ExhaustedIntent,
		ExhaustedSample);
	TestTrue(TEXT("source intent permits the boundary used by the fence test"),
		bBuiltExhausted);
	TestFalse(TEXT("write-ahead intent rejects a non-incrementable revision"),
		Fdemo_mapShanmenMeridianShockTreatmentRecoveryIntent::TryCapture(
			ExhaustedIntent,
			ExhaustedSample,
			Mismatched));
	return true;
}

#endif
