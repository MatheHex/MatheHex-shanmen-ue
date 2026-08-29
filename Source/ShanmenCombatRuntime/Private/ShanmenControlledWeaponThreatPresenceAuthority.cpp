#include "ShanmenControlledWeaponThreatPresenceAuthority.h"

#include "ShanmenDeterministicId.h"

namespace
{
	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	FGuid MakeSampleId(
		const FGuid& RunId,
		const FGuid& ActivationId,
		const FGuid& SourceEntityId,
		const FGuid& SourceItemInstanceId,
		FName DetectorId,
		EShanmenHitDetectorKind DetectorKind,
		int32 HitOrdinal)
	{
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.ControlledWeapon.ThreatSample.r1"),
			{
				GuidDigits(RunId),
				GuidDigits(ActivationId),
				GuidDigits(SourceEntityId),
				GuidDigits(SourceItemInstanceId),
				DetectorId.ToString(),
				FString::FromInt(static_cast<int32>(DetectorKind)),
				FString::FromInt(HitOrdinal)
			});
	}

	bool CandidatesMatch(
		const FShanmenHitCandidate& Left,
		const FShanmenHitCandidate& Right)
	{
		return Left.IsValid()
			&& Right.IsValid()
			&& Left.ActivationId == Right.ActivationId
			&& Left.SourceEntityId == Right.SourceEntityId
			&& Left.TargetEntityId == Right.TargetEntityId
			&& Left.DetectorId == Right.DetectorId
			&& Left.DetectorKind == Right.DetectorKind
			&& Left.HitOrdinal == Right.HitOrdinal
			&& Left.HitLocation.Equals(Right.HitLocation)
			&& Left.HitNormal.Equals(Right.HitNormal);
	}

	bool PresenceIntentsMatch(
		const FShanmenControlledWeaponThreatPresenceIntent& Left,
		const FShanmenControlledWeaponThreatPresenceIntent& Right)
	{
		return Left.IsValid()
			&& Right.IsValid()
			&& Left.GetIntentId() == Right.GetIntentId()
			&& Left.GetRunId() == Right.GetRunId()
			&& Left.GetSourceItemInstanceId()
				== Right.GetSourceItemInstanceId()
			&& CandidatesMatch(
				Left.GetCandidate(), Right.GetCandidate());
	}
}

bool FShanmenControlledWeaponThreatPresenceConsumeReceipt::IsValid() const
{
	return Intent.IsValid()
		&& SourceEntityId.IsValid()
		&& Intent.GetCandidate().SourceEntityId == SourceEntityId
		&& AuthorityRevision > 0;
}

bool FShanmenControlledWeaponThreatPresenceConsumeResult::IsValid() const
{
	if (Status
			== EShanmenControlledWeaponThreatPresenceConsumeStatus::Rejected)
	{
		return Error
				!= EShanmenControlledWeaponThreatPresenceConsumeError::None
			&& !RunId.IsValid()
			&& AuthorityRevisionBefore == INDEX_NONE
			&& AuthorityRevisionAfter == INDEX_NONE
			&& Receipts.IsEmpty();
	}
	if (Error != EShanmenControlledWeaponThreatPresenceConsumeError::None
		|| !RunId.IsValid()
		|| AuthorityRevisionBefore < 0
		|| AuthorityRevisionAfter < AuthorityRevisionBefore)
	{
		return false;
	}

	if (Status == EShanmenControlledWeaponThreatPresenceConsumeStatus::NoOp)
	{
		return AuthorityRevisionAfter == AuthorityRevisionBefore
			&& Receipts.IsEmpty();
	}
	if (Status
			!= EShanmenControlledWeaponThreatPresenceConsumeStatus::Consumed
		&& Status
			!= EShanmenControlledWeaponThreatPresenceConsumeStatus::AlreadyConsumed)
	{
		return false;
	}
	if (Receipts.IsEmpty())
	{
		return false;
	}

	TSet<FGuid> IntentIds;
	TSet<int64> Revisions;
	for (int32 Index = 0; Index < Receipts.Num(); ++Index)
	{
		const FShanmenControlledWeaponThreatPresenceConsumeReceipt& Receipt =
			Receipts[Index];
		if (!Receipt.IsValid()
			|| Receipt.GetIntent().GetRunId() != RunId
			|| IntentIds.Contains(Receipt.GetIntent().GetIntentId())
			|| Revisions.Contains(Receipt.GetAuthorityRevision()))
		{
			return false;
		}
		if (Status
				== EShanmenControlledWeaponThreatPresenceConsumeStatus::Consumed
			&& Receipt.GetAuthorityRevision()
				!= AuthorityRevisionBefore + Index + 1)
		{
			return false;
		}
		if (Status
				== EShanmenControlledWeaponThreatPresenceConsumeStatus::AlreadyConsumed
			&& Receipt.GetAuthorityRevision() > AuthorityRevisionAfter)
		{
			return false;
		}
		IntentIds.Add(Receipt.GetIntent().GetIntentId());
		Revisions.Add(Receipt.GetAuthorityRevision());
	}

	return Status
			== EShanmenControlledWeaponThreatPresenceConsumeStatus::Consumed
		? AuthorityRevisionAfter
			== AuthorityRevisionBefore + Receipts.Num()
		: AuthorityRevisionAfter == AuthorityRevisionBefore;
}

bool FShanmenControlledWeaponThreatPresenceConsumeResult::IsSuccess() const
{
	return IsValid()
		&& Status
			!= EShanmenControlledWeaponThreatPresenceConsumeStatus::Rejected;
}

bool FShanmenControlledWeaponThreatPresenceAuthority::TryCreate(
	const FGuid& InRunId,
	const FGuid& InSourceEntityId,
	FShanmenControlledWeaponThreatPresenceAuthority& OutAuthority)
{
	OutAuthority.Reset();
	if (!InRunId.IsValid() || !InSourceEntityId.IsValid())
	{
		return false;
	}
	OutAuthority.RunId = InRunId;
	OutAuthority.SourceEntityId = InSourceEntityId;
	OutAuthority.AuthorityRevision = 0;
	OutAuthority.SampleCheckpointRevision = 0;
	return OutAuthority.IsValid();
}

bool FShanmenControlledWeaponThreatPresenceAuthority::IsValid() const
{
	if (!RunId.IsValid()
		|| !SourceEntityId.IsValid()
		|| AuthorityRevision < 0
		|| SampleCheckpointRevision < 0
		|| SampleCheckpointRevision < LatestSamples.Num()
		|| AuthorityRevision < ProcessedIntents.Num())
	{
		return false;
	}

	TSet<int64> Revisions;
	TSet<FGuid> ReferencedIntentIds;
	for (const TPair<FGuid, FSampleCheckpoint>& Pair : LatestSamples)
	{
		if (!IsCheckpointValid(Pair.Key, Pair.Value))
		{
			return false;
		}
		for (const FGuid& IntentId : Pair.Value.IntentIds)
		{
			if (ReferencedIntentIds.Contains(IntentId))
			{
				return false;
			}
			ReferencedIntentIds.Add(IntentId);
		}
	}

	for (const TPair<FGuid,
		FShanmenControlledWeaponThreatPresenceConsumeReceipt>& Pair :
		ProcessedIntents)
	{
		const FShanmenControlledWeaponThreatPresenceConsumeReceipt& Receipt =
			Pair.Value;
		if (!Pair.Key.IsValid()
			|| !Receipt.IsValid()
			|| Pair.Key != Receipt.GetIntent().GetIntentId()
			|| Receipt.GetIntent().GetRunId() != RunId
			|| Receipt.GetSourceEntityId() != SourceEntityId
			|| Receipt.GetAuthorityRevision() > AuthorityRevision
			|| Revisions.Contains(Receipt.GetAuthorityRevision()))
		{
			return false;
		}
		Revisions.Add(Receipt.GetAuthorityRevision());
	}
	return Revisions.Num() == ProcessedIntents.Num()
		&& ReferencedIntentIds.Num() == ProcessedIntents.Num();
}

FShanmenControlledWeaponThreatPresenceConsumeResult
FShanmenControlledWeaponThreatPresenceAuthority::Consume(
	const FShanmenControlledWeaponThreatPresenceReceipt& Presence)
{
	if (!Presence.IsValid())
	{
		return Reject(
			EShanmenControlledWeaponThreatPresenceConsumeError::ReceiptInvalid);
	}
	if (!IsValid())
	{
		return Reject(
			EShanmenControlledWeaponThreatPresenceConsumeError::AuthorityNotReady);
	}

	const FShanmenCombatActionSnapshot& Action =
		Presence.GetPolicy().GetEmission().GetContext().GetAction();
	if (Action.GetRunId() != RunId)
	{
		return Reject(
			EShanmenControlledWeaponThreatPresenceConsumeError::RunMismatch);
	}
	if (Action.GetSourceEntityId() != SourceEntityId)
	{
		return Reject(
			EShanmenControlledWeaponThreatPresenceConsumeError::SourceMismatch);
	}

	FSampleCheckpoint IncomingCheckpoint;
	if (!TryBuildSampleCheckpoint(Presence, IncomingCheckpoint))
	{
		return Reject(
			EShanmenControlledWeaponThreatPresenceConsumeError::ReceiptInvalid);
	}
	const FGuid& SourceItemInstanceId =
		Action.GetSourceItemInstanceId();
	const FSampleCheckpoint* LatestCheckpoint =
		LatestSamples.Find(SourceItemInstanceId);
	bool bExactLatestSample = false;
	if (LatestCheckpoint)
	{
		if (IncomingCheckpoint.HitOrdinal < LatestCheckpoint->HitOrdinal)
		{
			return Reject(
				EShanmenControlledWeaponThreatPresenceConsumeError::
				SampleExpired);
		}
		if (IncomingCheckpoint.HitOrdinal == LatestCheckpoint->HitOrdinal)
		{
			if (IncomingCheckpoint.SampleId != LatestCheckpoint->SampleId
				|| IncomingCheckpoint.IntentIds
					!= LatestCheckpoint->IntentIds)
			{
				return Reject(
					EShanmenControlledWeaponThreatPresenceConsumeError::
					SampleConflict);
			}
			bExactLatestSample = true;
		}
	}

	FShanmenControlledWeaponThreatPresenceConsumeResult Result;
	Result.RunId = RunId;
	Result.AuthorityRevisionBefore = AuthorityRevision;
	Result.AuthorityRevisionAfter = AuthorityRevision;
	if (Presence.GetIntents().IsEmpty())
	{
		Result.Status =
			EShanmenControlledWeaponThreatPresenceConsumeStatus::NoOp;
		if (!Result.IsValid())
		{
			return Reject(
				EShanmenControlledWeaponThreatPresenceConsumeError::IntentConflict);
		}
		if (bExactLatestSample)
		{
			return Result;
		}
		if (SampleCheckpointRevision == MAX_int64)
		{
			return Reject(
				EShanmenControlledWeaponThreatPresenceConsumeError::
				RevisionExhausted);
		}

		FShanmenControlledWeaponThreatPresenceAuthority Candidate = *this;
		Candidate.PruneCheckpointIntents(SourceItemInstanceId);
		++Candidate.SampleCheckpointRevision;
		Candidate.LatestSamples.Add(
			SourceItemInstanceId, MoveTemp(IncomingCheckpoint));
		if (!Candidate.IsValid())
		{
			return Reject(
				EShanmenControlledWeaponThreatPresenceConsumeError::SampleConflict);
		}
		*this = MoveTemp(Candidate);
		return Result;
	}

	int32 ExistingCount = 0;
	Result.Receipts.Reserve(Presence.GetIntents().Num());
	for (const FShanmenControlledWeaponThreatPresenceIntent& Intent :
		Presence.GetIntents())
	{
		const FShanmenControlledWeaponThreatPresenceConsumeReceipt* Existing =
			ProcessedIntents.Find(Intent.GetIntentId());
		if (!Existing)
		{
			continue;
		}
		if (!PresenceIntentsMatch(Existing->GetIntent(), Intent)
			|| Existing->GetSourceEntityId() != SourceEntityId)
		{
			return Reject(
				EShanmenControlledWeaponThreatPresenceConsumeError::IntentConflict);
		}
		++ExistingCount;
		Result.Receipts.Add(*Existing);
	}

	if (ExistingCount > 0
		&& ExistingCount != Presence.GetIntents().Num())
	{
		return Reject(
			EShanmenControlledWeaponThreatPresenceConsumeError::
			PartialReplayConflict);
	}
	if (ExistingCount == Presence.GetIntents().Num())
	{
		if (!bExactLatestSample)
		{
			return Reject(
				EShanmenControlledWeaponThreatPresenceConsumeError::SampleConflict);
		}
		Result.Status =
			EShanmenControlledWeaponThreatPresenceConsumeStatus::AlreadyConsumed;
		return Result.IsValid()
			? Result
			: Reject(
				EShanmenControlledWeaponThreatPresenceConsumeError::IntentConflict);
	}

	const int64 NewIntentCount = Presence.GetIntents().Num();
	if (AuthorityRevision > MAX_int64 - NewIntentCount)
	{
		return Reject(
			EShanmenControlledWeaponThreatPresenceConsumeError::RevisionExhausted);
	}
	if (SampleCheckpointRevision == MAX_int64)
	{
		return Reject(
			EShanmenControlledWeaponThreatPresenceConsumeError::RevisionExhausted);
	}

	FShanmenControlledWeaponThreatPresenceAuthority Candidate = *this;
	Candidate.PruneCheckpointIntents(SourceItemInstanceId);
	++Candidate.SampleCheckpointRevision;
	Result.Receipts.Reset(Presence.GetIntents().Num());
	for (const FShanmenControlledWeaponThreatPresenceIntent& Intent :
		Presence.GetIntents())
	{
		FShanmenControlledWeaponThreatPresenceConsumeReceipt Receipt;
		Receipt.Intent = Intent;
		Receipt.SourceEntityId = SourceEntityId;
		Receipt.AuthorityRevision = ++Candidate.AuthorityRevision;
		if (!Receipt.IsValid())
		{
			return Reject(
				EShanmenControlledWeaponThreatPresenceConsumeError::IntentConflict);
		}
		Candidate.ProcessedIntents.Add(Intent.GetIntentId(), Receipt);
		Result.Receipts.Add(MoveTemp(Receipt));
	}
	Candidate.LatestSamples.Add(
		SourceItemInstanceId, MoveTemp(IncomingCheckpoint));
	Result.Status =
		EShanmenControlledWeaponThreatPresenceConsumeStatus::Consumed;
	Result.AuthorityRevisionAfter = Candidate.AuthorityRevision;
	if (!Candidate.IsValid() || !Result.IsValid())
	{
		return Reject(
			EShanmenControlledWeaponThreatPresenceConsumeError::IntentConflict);
	}

	*this = MoveTemp(Candidate);
	return Result;
}

bool FShanmenControlledWeaponThreatPresenceAuthority::
TryBuildSampleCheckpoint(
	const FShanmenControlledWeaponThreatPresenceReceipt& Presence,
	FSampleCheckpoint& OutCheckpoint)
{
	OutCheckpoint = FSampleCheckpoint();
	if (!Presence.IsValid())
	{
		return false;
	}

	const FShanmenWorldHitContext& Context =
		Presence.GetPolicy().GetEmission().GetContext();
	const FShanmenCombatActionSnapshot& Action = Context.GetAction();
	OutCheckpoint.ActivationId = Action.GetActivationId();
	OutCheckpoint.DetectorId = Context.GetDetectorId();
	OutCheckpoint.DetectorKind = Context.GetDetectorKind();
	OutCheckpoint.HitOrdinal = Context.GetHitOrdinal();
	OutCheckpoint.SampleId = MakeSampleId(
		Action.GetRunId(),
		OutCheckpoint.ActivationId,
		Action.GetSourceEntityId(),
		Action.GetSourceItemInstanceId(),
		OutCheckpoint.DetectorId,
		OutCheckpoint.DetectorKind,
		OutCheckpoint.HitOrdinal);

	TSet<FGuid> IntentIds;
	OutCheckpoint.IntentIds.Reserve(Presence.GetIntents().Num());
	for (const FShanmenControlledWeaponThreatPresenceIntent& Intent :
		Presence.GetIntents())
	{
		if (!Intent.IsValid()
			|| Intent.GetSourceItemInstanceId()
				!= Action.GetSourceItemInstanceId()
			|| IntentIds.Contains(Intent.GetIntentId()))
		{
			OutCheckpoint = FSampleCheckpoint();
			return false;
		}
		IntentIds.Add(Intent.GetIntentId());
		OutCheckpoint.IntentIds.Add(Intent.GetIntentId());
	}
	return OutCheckpoint.SampleId.IsValid()
		&& OutCheckpoint.ActivationId.IsValid()
		&& !OutCheckpoint.DetectorId.IsNone()
		&& OutCheckpoint.HitOrdinal >= 0;
}

bool FShanmenControlledWeaponThreatPresenceAuthority::IsCheckpointValid(
	const FGuid& SourceItemInstanceId,
	const FSampleCheckpoint& Checkpoint) const
{
	if (!SourceItemInstanceId.IsValid()
		|| !Checkpoint.SampleId.IsValid()
		|| !Checkpoint.ActivationId.IsValid()
		|| Checkpoint.DetectorId.IsNone()
		|| Checkpoint.HitOrdinal < 0
		|| Checkpoint.SampleId != MakeSampleId(
			RunId,
			Checkpoint.ActivationId,
			SourceEntityId,
			SourceItemInstanceId,
			Checkpoint.DetectorId,
			Checkpoint.DetectorKind,
			Checkpoint.HitOrdinal))
	{
		return false;
	}

	TSet<FGuid> IntentIds;
	for (const FGuid& IntentId : Checkpoint.IntentIds)
	{
		const FShanmenControlledWeaponThreatPresenceConsumeReceipt* Receipt =
			ProcessedIntents.Find(IntentId);
		if (!IntentId.IsValid()
			|| IntentIds.Contains(IntentId)
			|| !Receipt
			|| Receipt->GetIntent().GetSourceItemInstanceId()
				!= SourceItemInstanceId
			|| Receipt->GetIntent().GetCandidate().ActivationId
				!= Checkpoint.ActivationId
			|| Receipt->GetIntent().GetCandidate().DetectorId
				!= Checkpoint.DetectorId
			|| Receipt->GetIntent().GetCandidate().DetectorKind
				!= Checkpoint.DetectorKind
			|| Receipt->GetIntent().GetCandidate().HitOrdinal
				!= Checkpoint.HitOrdinal)
		{
			return false;
		}
		IntentIds.Add(IntentId);
	}
	return true;
}

void FShanmenControlledWeaponThreatPresenceAuthority::
PruneCheckpointIntents(const FGuid& SourceItemInstanceId)
{
	const FSampleCheckpoint* Checkpoint =
		LatestSamples.Find(SourceItemInstanceId);
	if (!Checkpoint)
	{
		return;
	}
	for (const FGuid& IntentId : Checkpoint->IntentIds)
	{
		ProcessedIntents.Remove(IntentId);
	}
	LatestSamples.Remove(SourceItemInstanceId);
}

void FShanmenControlledWeaponThreatPresenceAuthority::Reset()
{
	*this = FShanmenControlledWeaponThreatPresenceAuthority();
}

bool FShanmenControlledWeaponThreatPresenceAuthority::Contains(
	const FGuid& IntentId) const
{
	return IntentId.IsValid() && ProcessedIntents.Contains(IntentId);
}

int32 FShanmenControlledWeaponThreatPresenceAuthority::
GetLatestSampleOrdinal(const FGuid& SourceItemInstanceId) const
{
	const FSampleCheckpoint* Checkpoint =
		SourceItemInstanceId.IsValid()
			? LatestSamples.Find(SourceItemInstanceId)
			: nullptr;
	return Checkpoint ? Checkpoint->HitOrdinal : INDEX_NONE;
}

FShanmenControlledWeaponThreatPresenceConsumeResult
FShanmenControlledWeaponThreatPresenceAuthority::Reject(
	EShanmenControlledWeaponThreatPresenceConsumeError Error) const
{
	FShanmenControlledWeaponThreatPresenceConsumeResult Result;
	Result.Status =
		EShanmenControlledWeaponThreatPresenceConsumeStatus::Rejected;
	Result.Error = Error;
	return Result;
}
