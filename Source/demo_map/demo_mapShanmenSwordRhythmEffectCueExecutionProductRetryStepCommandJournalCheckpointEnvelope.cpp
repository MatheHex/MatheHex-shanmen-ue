#include "demo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpointEnvelope.h"

#include "ShanmenDeterministicId.h"

namespace
{
	using FCheckpoint =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpoint;
	using FEnvelope =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpointEnvelope;

	FGuid DeriveEnvelopeId(
		const int32 SchemaVersion,
		const FCheckpoint& Checkpoint)
	{
		if (SchemaVersion != FEnvelope::CurrentSchemaVersion()
			|| !Checkpoint.IsValid())
		{
			return FGuid();
		}

		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.Combat.SwordRhythm.EffectCueExecutionProductRetryStepCommandJournalCheckpointEnvelope.r1"),
			{
				FString::FromInt(SchemaVersion),
				Checkpoint.GetCheckpointId().ToString(EGuidFormats::Digits),
				FString::FromInt(Checkpoint.GetRecordCount())
			});
	}
}

bool FEnvelope::TryWrap(
	const int32 InSchemaVersion,
	const FCheckpoint& InCheckpoint,
	FEnvelope& OutEnvelope)
{
	OutEnvelope = FEnvelope();
	if (InSchemaVersion != CurrentSchemaVersion()
		|| !InCheckpoint.IsValid())
	{
		return false;
	}

	OutEnvelope.SchemaVersion = InSchemaVersion;
	OutEnvelope.Checkpoint = InCheckpoint;
	OutEnvelope.EnvelopeId = DeriveEnvelopeId(
		OutEnvelope.SchemaVersion, OutEnvelope.Checkpoint);
	if (!OutEnvelope.IsValid())
	{
		OutEnvelope = FEnvelope();
		return false;
	}
	return true;
}

bool FEnvelope::IsValid() const
{
	return SchemaVersion == CurrentSchemaVersion()
		&& EnvelopeId.IsValid()
		&& Checkpoint.IsValid()
		&& EnvelopeId == DeriveEnvelopeId(SchemaVersion, Checkpoint);
}

bool FEnvelope::Matches(const FEnvelope& Other) const
{
	return IsValid()
		&& Other.IsValid()
		&& SchemaVersion == Other.SchemaVersion
		&& EnvelopeId == Other.EnvelopeId
		&& Checkpoint.Matches(Other.Checkpoint);
}

bool FEnvelope::TryUnwrap(FCheckpoint& OutCheckpoint) const
{
	OutCheckpoint = FCheckpoint();
	if (!IsValid())
	{
		return false;
	}

	OutCheckpoint = Checkpoint;
	return OutCheckpoint.IsValid()
		&& OutCheckpoint.Matches(Checkpoint);
}
