#include "demo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchPlan.h"

#include "ShanmenDeterministicId.h"

namespace
{
	using FSeed =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchPlanSeed;
	using FProjection =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductProjection;
	using FIdentity =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionCapture;

	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	FGuid DeriveRoleId(
		const TCHAR* Role,
		const FProjection& Projection,
		const FSeed& Seed)
	{
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.Combat.SwordRhythm.EffectCueExecutionProductDispatchPlan.r1"),
			{
				Role,
				GuidDigits(Seed.DispatchSeed),
				GuidDigits(Seed.VisualConsumerScopeId),
				GuidDigits(Seed.AudioConsumerScopeId),
				GuidDigits(Projection.GetRunId()),
				GuidDigits(Projection.GetConfigId()),
				GuidDigits(Projection.GetCuePolicyId()),
				GuidDigits(Projection.GetEvent().GetEventId()),
				FString::FromInt(Projection.GetObservationRevision())
			});
	}

	TArray<FGuid> IdentityValues(const FIdentity& Identity)
	{
		return {
			Identity.HostId,
			Identity.CreateCommandId,
			Identity.ProcessCommandId,
			Identity.EndCommandId,
			Identity.VisualConsumerId,
			Identity.AudioConsumerId,
			Identity.VisualAttemptId,
			Identity.AudioAttemptId
		};
	}

	bool AreAllDistinct(const FIdentity& Identity)
	{
		const TArray<FGuid> Values = IdentityValues(Identity);
		for (int32 Left = 0; Left < Values.Num(); ++Left)
		{
			if (!Values[Left].IsValid())
			{
				return false;
			}
			for (int32 Right = Left + 1; Right < Values.Num(); ++Right)
			{
				if (Values[Left] == Values[Right])
				{
					return false;
				}
			}
		}
		return true;
	}

	bool IdentitiesMatch(const FIdentity& Left, const FIdentity& Right)
	{
		return Left.CreateSequence == Right.CreateSequence
			&& Left.ProcessSequence == Right.ProcessSequence
			&& Left.EndSequence == Right.EndSequence
			&& Left.HostId == Right.HostId
			&& Left.CreateCommandId == Right.CreateCommandId
			&& Left.ProcessCommandId == Right.ProcessCommandId
			&& Left.EndCommandId == Right.EndCommandId
			&& Left.VisualConsumerId == Right.VisualConsumerId
			&& Left.AudioConsumerId == Right.AudioConsumerId
			&& Left.VisualAttemptId == Right.VisualAttemptId
			&& Left.AudioAttemptId == Right.AudioAttemptId;
	}

	bool TryDeriveIdentity(
		const FProjection& Projection,
		const FSeed& Seed,
		FIdentity& OutIdentity)
	{
		OutIdentity = FIdentity();
		if (!Projection.IsValid() || !Seed.IsValid())
		{
			return false;
		}

		OutIdentity.HostId = DeriveRoleId(TEXT("Host"), Projection, Seed);
		OutIdentity.CreateCommandId =
			DeriveRoleId(TEXT("Command.Create"), Projection, Seed);
		OutIdentity.ProcessCommandId =
			DeriveRoleId(TEXT("Command.ProcessNext"), Projection, Seed);
		OutIdentity.EndCommandId =
			DeriveRoleId(TEXT("Command.End"), Projection, Seed);
		OutIdentity.VisualConsumerId =
			DeriveRoleId(TEXT("Consumer.Visual"), Projection, Seed);
		OutIdentity.AudioConsumerId =
			DeriveRoleId(TEXT("Consumer.Audio"), Projection, Seed);
		OutIdentity.VisualAttemptId =
			DeriveRoleId(TEXT("Attempt.Visual"), Projection, Seed);
		OutIdentity.AudioAttemptId =
			DeriveRoleId(TEXT("Attempt.Audio"), Projection, Seed);
		return OutIdentity.IsValid() && AreAllDistinct(OutIdentity);
	}
}

bool Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchPlanSeed::
IsValid() const
{
	return DispatchSeed.IsValid()
		&& VisualConsumerScopeId.IsValid()
		&& AudioConsumerScopeId.IsValid()
		&& VisualConsumerScopeId != AudioConsumerScopeId;
}

bool Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchPlan::
IsValid() const
{
	FIdentity Expected;
	return Seed.IsValid()
		&& Projection.IsValid()
		&& TryDeriveIdentity(Projection, Seed, Expected)
		&& IdentitiesMatch(TransactionIdentity, Expected);
}

bool Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchPlan::
Matches(
	const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchPlan&
		Other) const
{
	return IsValid()
		&& Other.IsValid()
		&& Seed.DispatchSeed == Other.Seed.DispatchSeed
		&& Seed.VisualConsumerScopeId == Other.Seed.VisualConsumerScopeId
		&& Seed.AudioConsumerScopeId == Other.Seed.AudioConsumerScopeId
		&& Projection.Matches(Other.Projection)
		&& IdentitiesMatch(TransactionIdentity, Other.TransactionIdentity);
}

bool Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchPlan::
MatchesProjection(
	const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductProjection& Other)
	const
{
	return IsValid() && Projection.Matches(Other);
}

bool Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchPlan::
MatchesRequest(
	const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionRequest&
		Other) const
{
	if (!IsValid()
		|| !Other.IsValid()
		|| !Projection.Matches(Other.GetProjection()))
	{
		return false;
	}

	const auto& CreateEnvelope = Other.GetCreateRequest().GetEnvelope();
	const auto& CreateCommand = CreateEnvelope.GetCommand();
	const auto& ProcessEnvelope = Other.GetProcessEnvelope();
	const auto& ProcessCommand = ProcessEnvelope.GetCommand();
	const auto& EndEnvelope = Other.GetEndEnvelope();
	const auto& EndCommand = EndEnvelope.GetCommand();
	return CreateEnvelope.GetHostId() == TransactionIdentity.HostId
		&& ProcessEnvelope.GetHostId() == TransactionIdentity.HostId
		&& EndEnvelope.GetHostId() == TransactionIdentity.HostId
		&& CreateEnvelope.GetSequence() == TransactionIdentity.CreateSequence
		&& ProcessEnvelope.GetSequence() == TransactionIdentity.ProcessSequence
		&& EndEnvelope.GetSequence() == TransactionIdentity.EndSequence
		&& CreateCommand.GetCommandId()
			== TransactionIdentity.CreateCommandId
		&& ProcessCommand.GetCommandId()
			== TransactionIdentity.ProcessCommandId
		&& EndCommand.GetCommandId() == TransactionIdentity.EndCommandId
		&& CreateCommand.GetVisualConsumerId()
			== TransactionIdentity.VisualConsumerId
		&& CreateCommand.GetAudioConsumerId()
			== TransactionIdentity.AudioConsumerId
		&& ProcessCommand.GetVisualAttemptId()
			== TransactionIdentity.VisualAttemptId
		&& ProcessCommand.GetAudioAttemptId()
			== TransactionIdentity.AudioAttemptId;
}

bool Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchPlan::
MatchesCurrentSession(
	const Fdemo_mapShanmenSwordRhythmProductSession& Session) const
{
	return IsValid() && Projection.MatchesCurrentSession(Session);
}

Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchPlanCaptureResult
Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchPlanFactory::
Capture(
	const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductProjection&
		Projection,
	const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchPlanSeed&
		Seed)
{
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchPlanCaptureResult
		Result;
	if (!Seed.IsValid())
	{
		Result.Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchPlanCaptureStatus::
				SeedInvalid;
		Result.Diagnostic = TEXT(
			"Dispatch planning requires one valid seed and two distinct consumer scopes.");
		return Result;
	}
	if (!Projection.IsValid())
	{
		Result.Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchPlanCaptureStatus::
				ProjectionInvalid;
		Result.Diagnostic = TEXT(
			"Dispatch planning requires one valid frozen Product projection.");
		return Result;
	}

	Result.Plan.Seed = Seed;
	Result.Plan.Projection = Projection;
	if (!TryDeriveIdentity(
			Projection, Seed, Result.Plan.TransactionIdentity))
	{
		Result.Plan =
			Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchPlan();
		Result.Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchPlanCaptureStatus::
				IdentityDerivationRejected;
		Result.Diagnostic = TEXT(
			"Role-isolated dispatch identity derivation produced invalid or aliased values.");
		return Result;
	}
	if (!Result.Plan.IsValid())
	{
		Result.Plan =
			Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchPlan();
		Result.Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchPlanCaptureStatus::
				PlanRejected;
		Result.Diagnostic = TEXT(
			"Deterministic dispatch plan failed its reconstruction proof.");
		return Result;
	}

	Result.Status =
		Edemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchPlanCaptureStatus::
			Captured;
	Result.Diagnostic = TEXT(
		"Captured one deterministic caller-owned Product dispatch plan.");
	return Result;
}
