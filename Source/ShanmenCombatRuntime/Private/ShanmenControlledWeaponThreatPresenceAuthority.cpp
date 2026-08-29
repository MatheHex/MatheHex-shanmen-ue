#include "ShanmenControlledWeaponThreatPresenceAuthority.h"

namespace
{
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
	return OutAuthority.IsValid();
}

bool FShanmenControlledWeaponThreatPresenceAuthority::IsValid() const
{
	if (!RunId.IsValid()
		|| !SourceEntityId.IsValid()
		|| AuthorityRevision < 0
		|| AuthorityRevision != ProcessedIntents.Num())
	{
		return false;
	}

	TSet<int64> Revisions;
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
	return Revisions.Num() == ProcessedIntents.Num();
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

	FShanmenControlledWeaponThreatPresenceConsumeResult Result;
	Result.RunId = RunId;
	Result.AuthorityRevisionBefore = AuthorityRevision;
	Result.AuthorityRevisionAfter = AuthorityRevision;
	if (Presence.GetIntents().IsEmpty())
	{
		Result.Status =
			EShanmenControlledWeaponThreatPresenceConsumeStatus::NoOp;
		return Result.IsValid()
			? Result
			: Reject(
				EShanmenControlledWeaponThreatPresenceConsumeError::IntentConflict);
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

	FShanmenControlledWeaponThreatPresenceAuthority Candidate = *this;
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

void FShanmenControlledWeaponThreatPresenceAuthority::Reset()
{
	*this = FShanmenControlledWeaponThreatPresenceAuthority();
}

bool FShanmenControlledWeaponThreatPresenceAuthority::Contains(
	const FGuid& IntentId) const
{
	return IntentId.IsValid() && ProcessedIntents.Contains(IntentId);
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
