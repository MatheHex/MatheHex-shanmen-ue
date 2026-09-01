#include "demo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpointEnvelopeManifestCodec.h"

namespace
{
	using FEnvelope =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpointEnvelope;
	using FManifest =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpointEnvelopeManifest;

	constexpr uint8 ManifestMagic[] =
	{
		'S', 'M', 'C', 'E', 'V', 'M', '0', '1'
	};

	void AppendUint32BigEndian(TArray<uint8>& OutBytes, const uint32 Value)
	{
		OutBytes.Add(static_cast<uint8>((Value >> 24) & 0xff));
		OutBytes.Add(static_cast<uint8>((Value >> 16) & 0xff));
		OutBytes.Add(static_cast<uint8>((Value >> 8) & 0xff));
		OutBytes.Add(static_cast<uint8>(Value & 0xff));
	}

	void AppendGuid(TArray<uint8>& OutBytes, const FGuid& Value)
	{
		AppendUint32BigEndian(OutBytes, Value.A);
		AppendUint32BigEndian(OutBytes, Value.B);
		AppendUint32BigEndian(OutBytes, Value.C);
		AppendUint32BigEndian(OutBytes, Value.D);
	}

	bool TryReadUint32BigEndian(
		const TArray<uint8>& Bytes,
		int32& InOutOffset,
		uint32& OutValue)
	{
		OutValue = 0;
		if (InOutOffset < 0 || InOutOffset > Bytes.Num() - 4)
		{
			return false;
		}

		OutValue = (static_cast<uint32>(Bytes[InOutOffset]) << 24)
			| (static_cast<uint32>(Bytes[InOutOffset + 1]) << 16)
			| (static_cast<uint32>(Bytes[InOutOffset + 2]) << 8)
			| static_cast<uint32>(Bytes[InOutOffset + 3]);
		InOutOffset += 4;
		return true;
	}

	bool TryReadGuid(
		const TArray<uint8>& Bytes,
		int32& InOutOffset,
		FGuid& OutGuid)
	{
		OutGuid.Invalidate();
		uint32 A = 0;
		uint32 B = 0;
		uint32 C = 0;
		uint32 D = 0;
		if (!TryReadUint32BigEndian(Bytes, InOutOffset, A)
			|| !TryReadUint32BigEndian(Bytes, InOutOffset, B)
			|| !TryReadUint32BigEndian(Bytes, InOutOffset, C)
			|| !TryReadUint32BigEndian(Bytes, InOutOffset, D))
		{
			return false;
		}

		OutGuid = FGuid(A, B, C, D);
		return true;
	}
}

bool FManifest::IsValid() const
{
	return SchemaVersion == FEnvelope::CurrentSchemaVersion()
		&& EnvelopeId.IsValid()
		&& CheckpointId.IsValid()
		&& RecordCount >= 0;
}

bool FManifest::Matches(const FManifest& Other) const
{
	return IsValid()
		&& Other.IsValid()
		&& SchemaVersion == Other.SchemaVersion
		&& EnvelopeId == Other.EnvelopeId
		&& CheckpointId == Other.CheckpointId
		&& RecordCount == Other.RecordCount;
}

bool Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpointEnvelopeManifestCodec::TryEncode(
	const FEnvelope& Envelope,
	TArray<uint8>& OutBytes)
{
	OutBytes.Reset();
	if (!Envelope.IsValid()
		|| !Envelope.GetCheckpointId().IsValid()
		|| Envelope.GetRecordCount() < 0)
	{
		return false;
	}

	OutBytes.Reserve(EncodedSize());
	OutBytes.Append(ManifestMagic, UE_ARRAY_COUNT(ManifestMagic));
	AppendUint32BigEndian(
		OutBytes, static_cast<uint32>(Envelope.GetSchemaVersion()));
	AppendGuid(OutBytes, Envelope.GetEnvelopeId());
	AppendGuid(OutBytes, Envelope.GetCheckpointId());
	AppendUint32BigEndian(
		OutBytes, static_cast<uint32>(Envelope.GetRecordCount()));
	if (OutBytes.Num() != EncodedSize())
	{
		OutBytes.Reset();
		return false;
	}
	return true;
}

bool Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpointEnvelopeManifestCodec::TryDecode(
	const TArray<uint8>& Bytes,
	FManifest& OutManifest)
{
	OutManifest = FManifest();
	if (Bytes.Num() != EncodedSize())
	{
		return false;
	}
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(ManifestMagic); ++Index)
	{
		if (Bytes[Index] != ManifestMagic[Index])
		{
			return false;
		}
	}

	int32 Offset = UE_ARRAY_COUNT(ManifestMagic);
	uint32 SchemaVersion = 0;
	uint32 RecordCount = 0;
	FManifest Candidate;
	if (!TryReadUint32BigEndian(Bytes, Offset, SchemaVersion)
		|| SchemaVersion > static_cast<uint32>(MAX_int32)
		|| !TryReadGuid(Bytes, Offset, Candidate.EnvelopeId)
		|| !TryReadGuid(Bytes, Offset, Candidate.CheckpointId)
		|| !TryReadUint32BigEndian(Bytes, Offset, RecordCount)
		|| RecordCount > static_cast<uint32>(MAX_int32)
		|| Offset != Bytes.Num())
	{
		return false;
	}

	Candidate.SchemaVersion = static_cast<int32>(SchemaVersion);
	Candidate.RecordCount = static_cast<int32>(RecordCount);
	if (!Candidate.IsValid())
	{
		return false;
	}

	OutManifest = Candidate;
	return true;
}

bool Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpointEnvelopeManifestCodec::TryVerify(
	const FManifest& Manifest,
	const FEnvelope& ExpectedEnvelope)
{
	return Manifest.IsValid()
		&& ExpectedEnvelope.IsValid()
		&& Manifest.GetSchemaVersion()
			== ExpectedEnvelope.GetSchemaVersion()
		&& Manifest.GetEnvelopeId() == ExpectedEnvelope.GetEnvelopeId()
		&& Manifest.GetCheckpointId() == ExpectedEnvelope.GetCheckpointId()
		&& Manifest.GetRecordCount() == ExpectedEnvelope.GetRecordCount();
}

bool Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalCheckpointEnvelopeManifestCodec::TryDecodeVerified(
	const TArray<uint8>& Bytes,
	const FEnvelope& ExpectedEnvelope,
	FEnvelope& OutEnvelope)
{
	OutEnvelope = FEnvelope();
	FManifest Manifest;
	if (!TryDecode(Bytes, Manifest)
		|| !TryVerify(Manifest, ExpectedEnvelope))
	{
		return false;
	}

	OutEnvelope = ExpectedEnvelope;
	return true;
}
