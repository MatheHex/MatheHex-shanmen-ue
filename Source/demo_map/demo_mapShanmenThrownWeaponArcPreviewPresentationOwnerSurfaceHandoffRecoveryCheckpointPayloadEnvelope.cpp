#include "demo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadEnvelope.h"

#include "ShanmenDeterministicId.h"
#include "ShanmenThrownWeaponArcPlanner.h"
#include "ShanmenThrownWeaponArcPreview.h"

namespace
{
	using FEnvelope =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadEnvelope;
	using FCodec =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadEnvelopeCodec;
	using FDecodeResult =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadEnvelopeDecodeResult;
	using EDecodeStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpointPayloadEnvelopeDecodeStatus;
	using FCheckpoint =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryCheckpoint;
	using FJournal =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournal;
	using EJournalDisposition =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffRecoveryJournalDisposition;
	using FTicket =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceOwnershipTransitionTicket;
	using ESurfaceAction =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationSurfaceRecreationAction;
	using EFailure =
		Edemo_mapShanmenThrownWeaponArcPreviewPresentationOwnerSurfaceHandoffStatus;
	using FState =
		Fdemo_mapShanmenThrownWeaponArcPreviewPresentationState;
	using FChoice = Fdemo_mapShanmenThrownWeaponInputChoiceState;
	using ETrajectory =
		Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind;

	constexpr uint8 PayloadMagic[] =
	{
		'S', 'M', 'A', 'R', 'C', 'P', 'A', 'Y'
	};

	void AppendUint32BigEndian(TArray<uint8>& OutBytes, const uint32 Value)
	{
		OutBytes.Add(static_cast<uint8>((Value >> 24) & 0xff));
		OutBytes.Add(static_cast<uint8>((Value >> 16) & 0xff));
		OutBytes.Add(static_cast<uint8>((Value >> 8) & 0xff));
		OutBytes.Add(static_cast<uint8>(Value & 0xff));
	}

	void AppendUint64BigEndian(TArray<uint8>& OutBytes, const uint64 Value)
	{
		for (int32 Shift = 56; Shift >= 0; Shift -= 8)
		{
			OutBytes.Add(static_cast<uint8>((Value >> Shift) & 0xff));
		}
	}

	void AppendGuid(TArray<uint8>& OutBytes, const FGuid& Value)
	{
		AppendUint32BigEndian(OutBytes, Value.A);
		AppendUint32BigEndian(OutBytes, Value.B);
		AppendUint32BigEndian(OutBytes, Value.C);
		AppendUint32BigEndian(OutBytes, Value.D);
	}

	void AppendDouble(TArray<uint8>& OutBytes, double Value)
	{
		Value = Value == 0.0 ? 0.0 : Value;
		uint64 Bits = 0;
		static_assert(sizeof(Bits) == sizeof(Value));
		FMemory::Memcpy(&Bits, &Value, sizeof(Bits));
		AppendUint64BigEndian(OutBytes, Bits);
	}

	bool HasEmbeddedNull(const FString& Value)
	{
		for (const TCHAR Character : Value)
		{
			if (Character == TEXT('\0'))
			{
				return true;
			}
		}
		return false;
	}

	bool AppendString(TArray<uint8>& OutBytes, const FString& Value)
	{
		if (HasEmbeddedNull(Value))
		{
			return false;
		}
		FTCHARToUTF8 Converted(*Value);
		if (Converted.Length() < 0
			|| Converted.Length() > FCodec::MaximumStringBytes())
		{
			return false;
		}
		AppendUint32BigEndian(
			OutBytes, static_cast<uint32>(Converted.Length()));
		if (Converted.Length() > 0)
		{
			OutBytes.Append(
				reinterpret_cast<const uint8*>(Converted.Get()),
				Converted.Length());
		}
		return true;
	}

	bool TryGetCanonicalSourceTags(
		const FGameplayTagContainer& Container,
		TArray<FString>& OutTags)
	{
		OutTags.Reset();
		TArray<FGameplayTag> Tags;
		Container.GetGameplayTagArray(Tags);
		if (Tags.Num() > FCodec::MaximumSourceTagCount())
		{
			return false;
		}
		OutTags.Reserve(Tags.Num());
		for (const FGameplayTag& Tag : Tags)
		{
			if (!Tag.IsValid())
			{
				return false;
			}
			OutTags.Add(Tag.ToString());
		}
		OutTags.Sort([](const FString& Left, const FString& Right)
		{
			return Left < Right;
		});
		for (int32 Index = 1; Index < OutTags.Num(); ++Index)
		{
			if (OutTags[Index - 1].Compare(OutTags[Index]) >= 0)
			{
				return false;
			}
		}
		return true;
	}

	FString BytesKey(const TArray<uint8>& Bytes)
	{
		static constexpr TCHAR HexDigits[] = TEXT("0123456789ABCDEF");
		FString Result;
		Result.Reserve(Bytes.Num() * 2);
		for (const uint8 Byte : Bytes)
		{
			Result.AppendChar(HexDigits[(Byte >> 4) & 0x0f]);
			Result.AppendChar(HexDigits[Byte & 0x0f]);
		}
		return Result;
	}

	FGuid DerivePayloadDigest(const TArray<uint8>& Payload)
	{
		return Payload.IsEmpty()
			? FGuid()
			: FShanmenDeterministicId::FromCanonicalParts(
				TEXT("demo_map.ShanmenThrownWeapon.ArcPreviewOwnerSurfaceHandoffRecoveryCheckpointPayload.r1"),
				{BytesKey(Payload)});
	}

	FGuid DeriveEnvelopeId(
		const int32 SchemaVersion,
		const FGuid& JournalId,
		const FGuid& CheckpointId,
		const FGuid& PayloadDigest)
	{
		if (SchemaVersion != FCodec::CurrentSchemaVersion()
			|| !JournalId.IsValid() || !CheckpointId.IsValid()
			|| !PayloadDigest.IsValid())
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.ShanmenThrownWeapon.ArcPreviewOwnerSurfaceHandoffRecoveryCheckpointPayloadEnvelope.r1"),
			{
				FString::FromInt(SchemaVersion),
				JournalId.ToString(EGuidFormats::Digits),
				CheckpointId.ToString(EGuidFormats::Digits),
				PayloadDigest.ToString(EGuidFormats::Digits)
			});
	}

	bool BuildCanonicalPayload(
		const FCheckpoint& Checkpoint,
		TArray<uint8>& OutPayload)
	{
		OutPayload.Reset();
		if (!Checkpoint.IsValid())
		{
			return false;
		}
		const FTicket& Ticket = Checkpoint.GetTransitionTicket();
		const FState& State = Checkpoint.GetSurfaceCursor();
		if (!State.IsVisible()
			|| !State.Matches(Checkpoint.GetPreviousSurfaceCursor())
			|| !State.Matches(Ticket.GetExpectedSurfaceCursor())
			|| !State.Matches(Ticket.GetObservedSurfaceCursor())
			|| !Checkpoint.GetRetiredSurfaceCursor().IsEmpty())
		{
			return false;
		}

		const FChoice& Choice = State.GetChoiceState();
		const FShanmenThrownWeaponArcPreview& Preview = State.GetSourcePreview();
		const FShanmenThrownWeaponArcPlan& Plan = Preview.GetPlan();
		const FShanmenThrownWeaponArcRequest& Request = Plan.GetRequest();
		const FShanmenCombatActionSnapshot& Action = Request.GetAction();
		TArray<FString> SourceTags;
		if (!Choice.IsValid() || !Preview.IsValid() || !Plan.IsValid()
			|| !Request.IsValid() || !Action.IsValid()
			|| !TryGetCanonicalSourceTags(Action.GetSourceTags(), SourceTags))
		{
			return false;
		}

		AppendGuid(OutPayload, Checkpoint.GetCheckpointId());
		AppendUint32BigEndian(
			OutPayload,
			static_cast<uint32>(Checkpoint.GetSourceFailureStatus()));
		AppendGuid(OutPayload, Checkpoint.GetSourceRetirementResponseId());
		AppendGuid(OutPayload, Checkpoint.GetPreviousSurfaceInstanceId());
		AppendGuid(OutPayload, Checkpoint.GetSurfaceInstanceId());

		AppendGuid(OutPayload, Ticket.GetTicketId());
		AppendGuid(OutPayload, Ticket.GetRequestId());
		AppendGuid(OutPayload, Ticket.GetPolicyDecisionId());
		AppendGuid(OutPayload, Ticket.GetPermitId());
		AppendGuid(OutPayload, Ticket.GetLifecycleReceiptId());
		AppendGuid(OutPayload, Ticket.GetRunId());
		if (!AppendString(OutPayload, Ticket.GetConsumerDefinitionId().ToString()))
		{
			return false;
		}
		AppendUint32BigEndian(
			OutPayload, static_cast<uint32>(Ticket.GetAction()));

		AppendGuid(OutPayload, State.GetPresentationStateId());
		AppendGuid(OutPayload, State.GetPlayerEntityId());
		AppendGuid(OutPayload, State.GetSourceItemInstanceId());
		AppendGuid(OutPayload, Choice.GetStateId());
		AppendGuid(OutPayload, Choice.GetLastCommandId());
		AppendUint64BigEndian(OutPayload, Choice.GetRevision());
		AppendUint32BigEndian(
			OutPayload, static_cast<uint32>(Choice.GetTrajectoryKind()));
		AppendUint32BigEndian(
			OutPayload, Choice.HasArcTargetIntent() ? 1u : 0u);
		AppendDouble(OutPayload, Choice.GetArcTargetIntent().X);
		AppendDouble(OutPayload, Choice.GetArcTargetIntent().Y);
		AppendDouble(OutPayload, Choice.GetArcApexAdjustment());
		AppendGuid(OutPayload, State.GetSourceProductRequestId());
		AppendGuid(OutPayload, State.GetSourcePreviewActivationId());
		AppendGuid(OutPayload, Preview.GetPreviewId());
		AppendUint32BigEndian(
			OutPayload, static_cast<uint32>(Preview.GetSegmentCount()));
		AppendGuid(OutPayload, Plan.GetPlanId());
		AppendGuid(OutPayload, Request.GetRequestId());

		AppendGuid(OutPayload, Action.GetRunId());
		AppendGuid(OutPayload, Action.GetOwnerId());
		AppendGuid(OutPayload, Action.GetActivationId());
		AppendGuid(OutPayload, Action.GetSourceEntityId());
		AppendGuid(OutPayload, Action.GetSourceItemInstanceId());
		if (!AppendString(OutPayload, Action.GetActionDefinitionId().ToString())
			|| !AppendString(OutPayload, Action.GetContent().Version.ToString())
			|| !AppendString(OutPayload, Action.GetContent().Digest))
		{
			return false;
		}
		AppendUint32BigEndian(
			OutPayload, static_cast<uint32>(SourceTags.Num()));
		for (const FString& SourceTag : SourceTags)
		{
			if (!AppendString(OutPayload, SourceTag))
			{
				return false;
			}
		}

		AppendUint32BigEndian(
			OutPayload, static_cast<uint32>(Request.GetTechniqueTier()));
		AppendDouble(OutPayload, Request.GetOrigin().X);
		AppendDouble(OutPayload, Request.GetOrigin().Y);
		AppendDouble(OutPayload, Request.GetOrigin().Z);
		AppendDouble(OutPayload, Request.GetTarget().X);
		AppendDouble(OutPayload, Request.GetTarget().Y);
		AppendDouble(OutPayload, Request.GetTarget().Z);
		AppendDouble(OutPayload, Request.GetGravityMagnitude());
		AppendDouble(OutPayload, Request.GetApexClearance());
		AppendDouble(OutPayload, Request.GetMaximumLaunchSpeed());
		AppendDouble(OutPayload, Request.GetMaximumFlightTime());
		return !OutPayload.IsEmpty()
			&& OutPayload.Num()
				<= FCodec::MaximumEncodedBytes() - FCodec::HeaderSize();
	}

	class FPayloadReader
	{
	public:
		explicit FPayloadReader(const TArray<uint8>& InBytes)
			: Bytes(InBytes)
		{
		}

		bool ReadUint32(uint32& OutValue)
		{
			OutValue = 0;
			if (!CanRead(4))
			{
				return false;
			}
			OutValue = (static_cast<uint32>(Bytes[Offset]) << 24)
				| (static_cast<uint32>(Bytes[Offset + 1]) << 16)
				| (static_cast<uint32>(Bytes[Offset + 2]) << 8)
				| static_cast<uint32>(Bytes[Offset + 3]);
			Offset += 4;
			return true;
		}

		bool ReadUint64(uint64& OutValue)
		{
			OutValue = 0;
			if (!CanRead(8))
			{
				return false;
			}
			for (int32 Index = 0; Index < 8; ++Index)
			{
				OutValue = (OutValue << 8)
					| static_cast<uint64>(Bytes[Offset + Index]);
			}
			Offset += 8;
			return true;
		}

		bool ReadGuid(FGuid& OutGuid)
		{
			OutGuid.Invalidate();
			uint32 A = 0;
			uint32 B = 0;
			uint32 C = 0;
			uint32 D = 0;
			if (!ReadUint32(A) || !ReadUint32(B)
				|| !ReadUint32(C) || !ReadUint32(D))
			{
				return false;
			}
			OutGuid = FGuid(A, B, C, D);
			return true;
		}

		bool ReadDouble(double& OutValue)
		{
			uint64 Bits = 0;
			if (!ReadUint64(Bits))
			{
				OutValue = 0.0;
				return false;
			}
			static_assert(sizeof(Bits) == sizeof(OutValue));
			FMemory::Memcpy(&OutValue, &Bits, sizeof(Bits));
			return FMath::IsFinite(OutValue)
				&& (OutValue != 0.0 || Bits == 0);
		}

		bool ReadString(FString& OutValue)
		{
			OutValue.Reset();
			uint32 Length = 0;
			if (!ReadUint32(Length)
				|| Length > static_cast<uint32>(FCodec::MaximumStringBytes())
				|| !CanRead(static_cast<int32>(Length)))
			{
				return false;
			}
			for (uint32 Index = 0; Index < Length; ++Index)
			{
				if (Bytes[Offset + static_cast<int32>(Index)] == 0)
				{
					return false;
				}
			}
			const ANSICHAR* Source = reinterpret_cast<const ANSICHAR*>(
				Bytes.GetData() + Offset);
			FUTF8ToTCHAR Converted(Source, static_cast<int32>(Length));
			OutValue = FString(Converted.Length(), Converted.Get());
			FTCHARToUTF8 Canonical(*OutValue);
			if (Canonical.Length() != static_cast<int32>(Length)
				|| (Length > 0
					&& FMemory::Memcmp(
						Canonical.Get(), Source, static_cast<SIZE_T>(Length)) != 0))
			{
				OutValue.Reset();
				return false;
			}
			Offset += static_cast<int32>(Length);
			return true;
		}

		bool IsAtEnd() const { return Offset == Bytes.Num(); }

	private:
		bool CanRead(const int32 Count) const
		{
			return Count >= 0 && Offset >= 0
				&& Offset <= Bytes.Num() - Count;
		}

		const TArray<uint8>& Bytes;
		int32 Offset = 0;
	};

	bool TryReadSourceTags(
		FPayloadReader& Reader,
		FGameplayTagContainer& OutTags)
	{
		OutTags.Reset();
		uint32 Count = 0;
		if (!Reader.ReadUint32(Count)
			|| Count > static_cast<uint32>(FCodec::MaximumSourceTagCount()))
		{
			return false;
		}
		FString Previous;
		for (uint32 Index = 0; Index < Count; ++Index)
		{
			FString Value;
			if (!Reader.ReadString(Value) || Value.IsEmpty()
				|| (Index > 0 && Previous.Compare(Value) >= 0))
			{
				return false;
			}
			const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(
				FName(*Value), false);
			if (!Tag.IsValid() || Tag.ToString() != Value)
			{
				return false;
			}
			OutTags.AddTag(Tag);
			Previous = MoveTemp(Value);
		}
		return true;
	}

	bool TryReadHeaderUint32(
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

	bool TryReadHeaderGuid(
		const TArray<uint8>& Bytes,
		int32& InOutOffset,
		FGuid& OutGuid)
	{
		uint32 A = 0;
		uint32 B = 0;
		uint32 C = 0;
		uint32 D = 0;
		if (!TryReadHeaderUint32(Bytes, InOutOffset, A)
			|| !TryReadHeaderUint32(Bytes, InOutOffset, B)
			|| !TryReadHeaderUint32(Bytes, InOutOffset, C)
			|| !TryReadHeaderUint32(Bytes, InOutOffset, D))
		{
			OutGuid.Invalidate();
			return false;
		}
		OutGuid = FGuid(A, B, C, D);
		return true;
	}
}

bool FEnvelope::TryWrap(
	const FCheckpoint& InCheckpoint,
	const FJournal& Journal,
	FEnvelope& OutEnvelope)
{
	OutEnvelope = FEnvelope();
	if (!InCheckpoint.IsValid() || !Journal.IsValid()
		|| Journal.GetLatestDisposition()
			!= EJournalDisposition::CheckpointPending
		|| !Journal.MatchesLatestCheckpoint(InCheckpoint))
	{
		return false;
	}
	TArray<uint8> Payload;
	if (!BuildCanonicalPayload(InCheckpoint, Payload))
	{
		return false;
	}
	FEnvelope Candidate;
	Candidate.SchemaVersion = FCodec::CurrentSchemaVersion();
	Candidate.JournalId = Journal.GetJournalId();
	Candidate.PayloadDigest = DerivePayloadDigest(Payload);
	Candidate.Checkpoint = InCheckpoint;
	Candidate.EnvelopeId = DeriveEnvelopeId(
		Candidate.SchemaVersion,
		Candidate.JournalId,
		Candidate.Checkpoint.GetCheckpointId(),
		Candidate.PayloadDigest);
	if (!Candidate.IsValid())
	{
		return false;
	}
	OutEnvelope = MoveTemp(Candidate);
	return true;
}

bool FEnvelope::IsValid() const
{
	TArray<uint8> Payload;
	return SchemaVersion == FCodec::CurrentSchemaVersion()
		&& EnvelopeId.IsValid() && JournalId.IsValid()
		&& PayloadDigest.IsValid() && Checkpoint.IsValid()
		&& BuildCanonicalPayload(Checkpoint, Payload)
		&& PayloadDigest == DerivePayloadDigest(Payload)
		&& EnvelopeId == DeriveEnvelopeId(
			SchemaVersion,
			JournalId,
			Checkpoint.GetCheckpointId(),
			PayloadDigest);
}

bool FEnvelope::Matches(const FEnvelope& Other) const
{
	return IsValid() && Other.IsValid()
		&& SchemaVersion == Other.SchemaVersion
		&& EnvelopeId == Other.EnvelopeId
		&& JournalId == Other.JournalId
		&& PayloadDigest == Other.PayloadDigest
		&& Checkpoint.GetCheckpointId()
			== Other.Checkpoint.GetCheckpointId();
}

bool FEnvelope::MatchesJournal(const FJournal& Journal) const
{
	return IsValid() && Journal.IsValid()
		&& Journal.GetLatestDisposition()
			== EJournalDisposition::CheckpointPending
		&& JournalId == Journal.GetJournalId()
		&& Journal.MatchesLatestCheckpoint(Checkpoint);
}

bool FEnvelope::TryUnwrapForJournal(
	const FJournal& Journal,
	FCheckpoint& OutCheckpoint) const
{
	OutCheckpoint = FCheckpoint();
	if (!MatchesJournal(Journal))
	{
		return false;
	}
	OutCheckpoint = Checkpoint;
	return OutCheckpoint.IsValid();
}

bool FDecodeResult::IsSuccess() const
{
	return Status == EDecodeStatus::Decoded
		&& SourceSchemaVersion == FCodec::CurrentSchemaVersion()
		&& !Diagnostic.IsEmpty() && Envelope.IsValid();
}

bool FCodec::TryEncode(
	const FEnvelope& Envelope,
	TArray<uint8>& OutBytes)
{
	OutBytes.Reset();
	TArray<uint8> Payload;
	if (!Envelope.IsValid()
		|| !BuildCanonicalPayload(Envelope.GetCheckpoint(), Payload)
		|| Envelope.GetPayloadDigest() != DerivePayloadDigest(Payload))
	{
		return false;
	}
	const int32 TotalSize = HeaderSize() + Payload.Num();
	if (TotalSize > MaximumEncodedBytes())
	{
		return false;
	}
	OutBytes.Reserve(TotalSize);
	OutBytes.Append(PayloadMagic, UE_ARRAY_COUNT(PayloadMagic));
	AppendUint32BigEndian(
		OutBytes, static_cast<uint32>(Envelope.GetSchemaVersion()));
	AppendUint32BigEndian(OutBytes, static_cast<uint32>(TotalSize));
	AppendGuid(OutBytes, Envelope.GetEnvelopeId());
	AppendGuid(OutBytes, Envelope.GetJournalId());
	AppendGuid(OutBytes, Envelope.GetPayloadDigest());
	OutBytes.Append(Payload);
	return OutBytes.Num() == TotalSize;
}

FDecodeResult FCodec::Decode(const TArray<uint8>& Bytes)
{
	if (Bytes.IsEmpty())
	{
		return MakeDecodeResult(
			EDecodeStatus::InputEmpty,
			TEXT("Arc preview recovery checkpoint payload envelope input is empty."));
	}
	if (Bytes.Num() < HeaderSize() || Bytes.Num() > MaximumEncodedBytes())
	{
		return MakeDecodeResult(
			EDecodeStatus::SizeMismatch,
			TEXT("Arc preview recovery checkpoint payload envelope size is outside the bounded schema."));
	}
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(PayloadMagic); ++Index)
	{
		if (Bytes[Index] != PayloadMagic[Index])
		{
			return MakeDecodeResult(
				EDecodeStatus::MagicMismatch,
				TEXT("Arc preview recovery checkpoint payload envelope magic does not match."));
		}
	}

	int32 HeaderOffset = UE_ARRAY_COUNT(PayloadMagic);
	uint32 Schema = 0;
	uint32 DeclaredSize = 0;
	FGuid ExpectedEnvelopeId;
	FGuid JournalId;
	FGuid ExpectedPayloadDigest;
	if (!TryReadHeaderUint32(Bytes, HeaderOffset, Schema)
		|| !TryReadHeaderUint32(Bytes, HeaderOffset, DeclaredSize)
		|| !TryReadHeaderGuid(Bytes, HeaderOffset, ExpectedEnvelopeId)
		|| !TryReadHeaderGuid(Bytes, HeaderOffset, JournalId)
		|| !TryReadHeaderGuid(Bytes, HeaderOffset, ExpectedPayloadDigest)
		|| HeaderOffset != HeaderSize())
	{
		return MakeDecodeResult(
			EDecodeStatus::SizeMismatch,
			TEXT("Arc preview recovery checkpoint payload envelope header is truncated."));
	}
	if (Schema != static_cast<uint32>(CurrentSchemaVersion()))
	{
		return MakeDecodeResult(
			EDecodeStatus::UnsupportedSchema,
			TEXT("Arc preview recovery checkpoint payload envelope schema is unsupported."),
			static_cast<int32>(Schema));
	}
	if (DeclaredSize != static_cast<uint32>(Bytes.Num()))
	{
		return MakeDecodeResult(
			EDecodeStatus::SizeMismatch,
			TEXT("Arc preview recovery checkpoint payload envelope declared size does not match input."),
			static_cast<int32>(Schema));
	}

	TArray<uint8> Payload;
	Payload.Append(
		Bytes.GetData() + HeaderSize(), Bytes.Num() - HeaderSize());
	if (!ExpectedPayloadDigest.IsValid()
		|| ExpectedPayloadDigest != DerivePayloadDigest(Payload))
	{
		return MakeDecodeResult(
			EDecodeStatus::PayloadDigestMismatch,
			TEXT("Arc preview recovery checkpoint payload envelope digest does not match payload."),
			static_cast<int32>(Schema));
	}

	FPayloadReader Reader(Payload);
	FGuid CheckpointId;
	uint32 FailureValue = 0;
	FGuid RetirementResponseId;
	FGuid PreviousSurfaceInstanceId;
	FGuid SurfaceInstanceId;
	FGuid TicketId;
	FGuid RequestId;
	FGuid PolicyDecisionId;
	FGuid PermitId;
	FGuid LifecycleReceiptId;
	FGuid RunId;
	FString ConsumerDefinition;
	uint32 SurfaceActionValue = 0;
	FGuid PresentationStateId;
	FGuid PlayerEntityId;
	FGuid SourceItemInstanceId;
	FGuid ChoiceStateId;
	FGuid LastCommandId;
	uint64 ChoiceRevision = 0;
	uint32 TrajectoryValue = 0;
	uint32 HasTargetValue = 0;
	double ChoiceTargetX = 0.0;
	double ChoiceTargetY = 0.0;
	double ChoiceApexAdjustment = 0.0;
	FGuid SourceProductRequestId;
	FGuid SourcePreviewActivationId;
	FGuid PreviewId;
	uint32 SegmentCount = 0;
	FGuid PlanId;
	FGuid ArcRequestId;
	FGuid ActionRunId;
	FGuid ActionOwnerId;
	FGuid ActionActivationId;
	FGuid ActionSourceEntityId;
	FGuid ActionSourceItemInstanceId;
	FString ActionDefinition;
	FString ContentVersion;
	FString ContentDigest;
	FGameplayTagContainer SourceTags;
	uint32 TechniqueTierValue = 0;
	double OriginX = 0.0;
	double OriginY = 0.0;
	double OriginZ = 0.0;
	double TargetX = 0.0;
	double TargetY = 0.0;
	double TargetZ = 0.0;
	double Gravity = 0.0;
	double ApexClearance = 0.0;
	double MaximumLaunchSpeed = 0.0;
	double MaximumFlightTime = 0.0;

	const bool bRead =
		Reader.ReadGuid(CheckpointId)
		&& Reader.ReadUint32(FailureValue)
		&& Reader.ReadGuid(RetirementResponseId)
		&& Reader.ReadGuid(PreviousSurfaceInstanceId)
		&& Reader.ReadGuid(SurfaceInstanceId)
		&& Reader.ReadGuid(TicketId)
		&& Reader.ReadGuid(RequestId)
		&& Reader.ReadGuid(PolicyDecisionId)
		&& Reader.ReadGuid(PermitId)
		&& Reader.ReadGuid(LifecycleReceiptId)
		&& Reader.ReadGuid(RunId)
		&& Reader.ReadString(ConsumerDefinition)
		&& Reader.ReadUint32(SurfaceActionValue)
		&& Reader.ReadGuid(PresentationStateId)
		&& Reader.ReadGuid(PlayerEntityId)
		&& Reader.ReadGuid(SourceItemInstanceId)
		&& Reader.ReadGuid(ChoiceStateId)
		&& Reader.ReadGuid(LastCommandId)
		&& Reader.ReadUint64(ChoiceRevision)
		&& Reader.ReadUint32(TrajectoryValue)
		&& Reader.ReadUint32(HasTargetValue)
		&& Reader.ReadDouble(ChoiceTargetX)
		&& Reader.ReadDouble(ChoiceTargetY)
		&& Reader.ReadDouble(ChoiceApexAdjustment)
		&& Reader.ReadGuid(SourceProductRequestId)
		&& Reader.ReadGuid(SourcePreviewActivationId)
		&& Reader.ReadGuid(PreviewId)
		&& Reader.ReadUint32(SegmentCount)
		&& Reader.ReadGuid(PlanId)
		&& Reader.ReadGuid(ArcRequestId)
		&& Reader.ReadGuid(ActionRunId)
		&& Reader.ReadGuid(ActionOwnerId)
		&& Reader.ReadGuid(ActionActivationId)
		&& Reader.ReadGuid(ActionSourceEntityId)
		&& Reader.ReadGuid(ActionSourceItemInstanceId)
		&& Reader.ReadString(ActionDefinition)
		&& Reader.ReadString(ContentVersion)
		&& Reader.ReadString(ContentDigest)
		&& TryReadSourceTags(Reader, SourceTags)
		&& Reader.ReadUint32(TechniqueTierValue)
		&& Reader.ReadDouble(OriginX)
		&& Reader.ReadDouble(OriginY)
		&& Reader.ReadDouble(OriginZ)
		&& Reader.ReadDouble(TargetX)
		&& Reader.ReadDouble(TargetY)
		&& Reader.ReadDouble(TargetZ)
		&& Reader.ReadDouble(Gravity)
		&& Reader.ReadDouble(ApexClearance)
		&& Reader.ReadDouble(MaximumLaunchSpeed)
		&& Reader.ReadDouble(MaximumFlightTime)
		&& Reader.IsAtEnd();
	if (!bRead || HasTargetValue > 1
		|| SegmentCount > static_cast<uint32>(MAX_int32)
		|| ConsumerDefinition.IsEmpty() || ActionDefinition.IsEmpty()
		|| ContentVersion.IsEmpty() || ContentDigest.IsEmpty())
	{
		return MakeDecodeResult(
			EDecodeStatus::PayloadMalformed,
			TEXT("Arc preview recovery checkpoint payload envelope is malformed or non-canonical."),
			static_cast<int32>(Schema));
	}

	FShanmenCombatActionCapture ActionCapture;
	ActionCapture.RunId = ActionRunId;
	ActionCapture.OwnerId = ActionOwnerId;
	ActionCapture.ActivationId = ActionActivationId;
	ActionCapture.SourceEntityId = ActionSourceEntityId;
	ActionCapture.SourceItemInstanceId = ActionSourceItemInstanceId;
	ActionCapture.ActionDefinitionId = FName(*ActionDefinition);
	ActionCapture.Content.Version = FName(*ContentVersion);
	ActionCapture.Content.Digest = ContentDigest;
	ActionCapture.SourceTags = SourceTags;
	FShanmenCombatActionSnapshot Action;
	if (!FShanmenCombatActionSnapshot::TryCapture(ActionCapture, Action))
	{
		return MakeDecodeResult(
			EDecodeStatus::SemanticReconstructionRejected,
			TEXT("Arc preview recovery checkpoint payload action snapshot was rejected."),
			static_cast<int32>(Schema));
	}

	FShanmenThrownWeaponArcRequestCapture ArcCapture;
	ArcCapture.Action = Action;
	ArcCapture.TechniqueTier =
		static_cast<EShanmenThrownWeaponTechniqueTier>(TechniqueTierValue);
	ArcCapture.Origin = FVector(OriginX, OriginY, OriginZ);
	ArcCapture.Target = FVector(TargetX, TargetY, TargetZ);
	ArcCapture.GravityMagnitude = Gravity;
	ArcCapture.ApexClearance = ApexClearance;
	ArcCapture.MaximumLaunchSpeed = MaximumLaunchSpeed;
	ArcCapture.MaximumFlightTime = MaximumFlightTime;
	FShanmenThrownWeaponArcRequest ArcRequest;
	if (!FShanmenThrownWeaponArcRequest::TryCapture(ArcCapture, ArcRequest)
		|| ArcRequest.GetRequestId() != ArcRequestId)
	{
		return MakeDecodeResult(
			EDecodeStatus::SemanticReconstructionRejected,
			TEXT("Arc preview recovery checkpoint payload Arc request identity was rejected."),
			static_cast<int32>(Schema));
	}
	const FShanmenThrownWeaponArcPlanResult Planned =
		FShanmenThrownWeaponArcPlanner::Plan(ArcRequest);
	if (!Planned.IsPlanned() || Planned.Plan.GetPlanId() != PlanId)
	{
		return MakeDecodeResult(
			EDecodeStatus::SemanticReconstructionRejected,
			TEXT("Arc preview recovery checkpoint payload Arc plan identity was rejected."),
			static_cast<int32>(Schema));
	}
	FShanmenThrownWeaponArcPreview Preview;
	if (!FShanmenThrownWeaponArcPreviewSampler::TrySample(
			Planned.Plan, static_cast<int32>(SegmentCount), Preview)
		|| Preview.GetPreviewId() != PreviewId)
	{
		return MakeDecodeResult(
			EDecodeStatus::SemanticReconstructionRejected,
			TEXT("Arc preview recovery checkpoint payload preview identity was rejected."),
			static_cast<int32>(Schema));
	}

	FChoice Choice;
	if (!FChoice::TryRehydrate(
			ChoiceStateId,
			LastCommandId,
			ChoiceRevision,
			static_cast<ETrajectory>(TrajectoryValue),
			HasTargetValue == 1,
			FVector2D(ChoiceTargetX, ChoiceTargetY),
			ChoiceApexAdjustment,
			Choice))
	{
		return MakeDecodeResult(
			EDecodeStatus::SemanticReconstructionRejected,
			TEXT("Arc preview recovery checkpoint payload choice state was rejected."),
			static_cast<int32>(Schema));
	}
	FState State;
	if (!FState::TryRehydrateVisible(
			PresentationStateId,
			RunId,
			PlayerEntityId,
			SourceItemInstanceId,
			Choice,
			SourceProductRequestId,
			SourcePreviewActivationId,
			Preview,
			State))
	{
		return MakeDecodeResult(
			EDecodeStatus::SemanticReconstructionRejected,
			TEXT("Arc preview recovery checkpoint payload presentation state was rejected."),
			static_cast<int32>(Schema));
	}

	FTicket Ticket;
	if (!FTicket::TryRehydrate(
			TicketId,
			RequestId,
			PolicyDecisionId,
			PermitId,
			LifecycleReceiptId,
			RunId,
			FName(*ConsumerDefinition),
			SurfaceInstanceId,
			static_cast<ESurfaceAction>(SurfaceActionValue),
			State,
			State,
			Ticket))
	{
		return MakeDecodeResult(
			EDecodeStatus::SemanticReconstructionRejected,
			TEXT("Arc preview recovery checkpoint payload transition ticket was rejected."),
			static_cast<int32>(Schema));
	}
	FCheckpoint Checkpoint;
	if (!FCheckpoint::TryRehydrate(
			CheckpointId,
			Ticket,
			static_cast<EFailure>(FailureValue),
			RetirementResponseId,
			PreviousSurfaceInstanceId,
			SurfaceInstanceId,
			State,
			FState(),
			State,
			Checkpoint))
	{
		return MakeDecodeResult(
			EDecodeStatus::SemanticReconstructionRejected,
			TEXT("Arc preview recovery checkpoint payload checkpoint was rejected."),
			static_cast<int32>(Schema));
	}

	TArray<uint8> CanonicalPayload;
	if (!BuildCanonicalPayload(Checkpoint, CanonicalPayload)
		|| CanonicalPayload != Payload)
	{
		return MakeDecodeResult(
			EDecodeStatus::PayloadMalformed,
			TEXT("Arc preview recovery checkpoint payload does not use canonical field encoding."),
			static_cast<int32>(Schema));
	}

	FEnvelope Envelope;
	Envelope.SchemaVersion = static_cast<int32>(Schema);
	Envelope.EnvelopeId = ExpectedEnvelopeId;
	Envelope.JournalId = JournalId;
	Envelope.PayloadDigest = ExpectedPayloadDigest;
	Envelope.Checkpoint = MoveTemp(Checkpoint);
	if (!Envelope.IsValid())
	{
		return MakeDecodeResult(
			EDecodeStatus::EnvelopeIdentityMismatch,
			TEXT("Arc preview recovery checkpoint payload envelope identity does not match reconstructed evidence."),
			static_cast<int32>(Schema));
	}
	return MakeDecodeResult(
		EDecodeStatus::Decoded,
		TEXT("Decoded one canonical Arc preview recovery checkpoint payload envelope without authorizing recovery."),
		static_cast<int32>(Schema),
		Envelope);
}

FDecodeResult FCodec::MakeDecodeResult(
	const EDecodeStatus Status,
	const TCHAR* Diagnostic,
	const int32 SourceSchemaVersion,
	const FEnvelope& Envelope)
{
	FDecodeResult Result;
	Result.Status = Status;
	Result.Diagnostic = Diagnostic;
	Result.SourceSchemaVersion = SourceSchemaVersion;
	Result.Envelope = Envelope;
	return Result;
}
