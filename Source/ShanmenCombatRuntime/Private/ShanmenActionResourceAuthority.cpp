#include "ShanmenActionResourceAuthority.h"

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

	bool IsValidResourceState(
		float CurrentAmount,
		float MaximumAmount,
		float ReservedAmount)
	{
		return FMath::IsFinite(CurrentAmount)
			&& FMath::IsFinite(MaximumAmount)
			&& FMath::IsFinite(ReservedAmount)
			&& CurrentAmount >= 0.0f
			&& MaximumAmount >= CurrentAmount
			&& ReservedAmount >= 0.0f
			&& ReservedAmount <= CurrentAmount;
	}

	FGuid MakeCostId(
		FName RuleId,
		const FGameplayTag& ResourceChannel,
		float Amount)
	{
		if (RuleId.IsNone() || !ResourceChannel.IsValid()
			|| !FMath::IsFinite(Amount) || Amount <= 0.0f)
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Combat.ActionResource.Cost.r1"),
			{RuleId.ToString(), ResourceChannel.ToString(), FloatBits(Amount)});
	}

	FGuid MakeSnapshotId(
		const FGuid& OwnerEntityId,
		const FGameplayTag& ResourceChannel,
		int64 AuthorityRevision,
		float CurrentAmount,
		float MaximumAmount,
		float ReservedAmount,
		float AvailableAmount)
	{
		if (!OwnerEntityId.IsValid() || !ResourceChannel.IsValid()
			|| AuthorityRevision < 0
			|| !IsValidResourceState(
				CurrentAmount, MaximumAmount, ReservedAmount)
			|| !FloatsMatchExactly(
				AvailableAmount, CurrentAmount - ReservedAmount))
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Combat.ActionResource.Snapshot.r1"),
			{
				GuidDigits(OwnerEntityId),
				ResourceChannel.ToString(),
				FString::Printf(TEXT("%lld"), AuthorityRevision),
				FloatBits(CurrentAmount),
				FloatBits(MaximumAmount),
				FloatBits(ReservedAmount),
				FloatBits(AvailableAmount)
			});
	}

	bool IsStartupReceipt(
		const FShanmenCombatActionSnapshot& Action,
		const FShanmenActionTransitionReceipt& Receipt)
	{
		return Action.IsValid() && Receipt.IsValid()
			&& Receipt.GetActivationId() == Action.GetActivationId()
			&& Receipt.GetSequence() == 0
			&& Receipt.GetFromPhase() == EShanmenCombatActionPhase::Idle
			&& Receipt.GetToPhase() == EShanmenCombatActionPhase::Startup
			&& Receipt.GetTerminalReason() == EShanmenActionTerminalReason::None
			&& !Receipt.CrossedCommitPointNow()
			&& !Receipt.HasReachedCommitPoint();
	}

	FGuid MakeReservationId(
		const FShanmenCombatActionSnapshot& Action,
		const FGameplayTag& ResourceChannel)
	{
		if (!Action.IsValid() || !ResourceChannel.IsValid())
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Combat.ActionResource.Reservation.r1"),
			{
				GuidDigits(Action.GetActivationId()),
				GuidDigits(Action.GetSourceEntityId()),
				ResourceChannel.ToString()
			});
	}

	FGuid MakeReservationCommandId(
		const FShanmenCombatActionSnapshot& Action,
		const FShanmenActionTransitionReceipt& StartupReceipt,
		const FShanmenActionResourceCost& Cost,
		const FShanmenActionResourceSnapshot& Snapshot)
	{
		if (!Action.IsValid() || !IsStartupReceipt(Action, StartupReceipt)
			|| !Cost.IsValid() || !Snapshot.IsValid())
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Combat.ActionResource.ReservationCommand.r1"),
			{
				GuidDigits(Action.GetActivationId()),
				GuidDigits(Action.GetSourceEntityId()),
				FString::Printf(TEXT("%lld"), StartupReceipt.GetSequence()),
				GuidDigits(Cost.GetCostId()),
				GuidDigits(Snapshot.GetSnapshotId())
			});
	}

	FGuid MakeReservationReceiptId(
		const FShanmenActionResourceReservationRequest& Request,
		int64 RevisionBefore,
		int64 RevisionAfter,
		float CurrentAmount,
		float MaximumAmount,
		float ReservedBefore,
		float ReservedAfter,
		float AvailableBefore,
		float AvailableAfter)
	{
		if (!Request.IsValid())
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Combat.ActionResource.ReservationReceipt.r1"),
			{
				GuidDigits(Request.GetReservationId()),
				GuidDigits(Request.GetCommandId()),
				FString::Printf(TEXT("%lld"), RevisionBefore),
				FString::Printf(TEXT("%lld"), RevisionAfter),
				FloatBits(CurrentAmount),
				FloatBits(MaximumAmount),
				FloatBits(ReservedBefore),
				FloatBits(ReservedAfter),
				FloatBits(AvailableBefore),
				FloatBits(AvailableAfter)
			});
	}

	EShanmenActionResourceDisposition ClassifyTransition(
		const FShanmenActionResourceReservationReceipt& Reservation,
		const FShanmenActionTransitionReceipt& Transition)
	{
		if (!Reservation.IsValid() || !Transition.IsValid()
			|| Transition.GetActivationId()
				!= Reservation.GetRequest().GetAction().GetActivationId())
		{
			return EShanmenActionResourceDisposition::None;
		}
		const bool bCommit =
			Transition.GetFromPhase() == EShanmenCombatActionPhase::Startup
			&& Transition.GetToPhase() == EShanmenCombatActionPhase::Active
			&& Transition.GetTerminalReason() == EShanmenActionTerminalReason::None
			&& Transition.CrossedCommitPointNow()
			&& Transition.HasReachedCommitPoint();
		if (bCommit)
		{
			return EShanmenActionResourceDisposition::Commit;
		}
		const bool bRelease =
			Transition.GetFromPhase() == EShanmenCombatActionPhase::Startup
			&& (Transition.GetToPhase() == EShanmenCombatActionPhase::Cancelled
				|| Transition.GetToPhase() == EShanmenCombatActionPhase::Interrupted)
			&& (Transition.GetTerminalReason() == EShanmenActionTerminalReason::Cancelled
				|| Transition.GetTerminalReason() == EShanmenActionTerminalReason::Interrupted)
			&& !Transition.CrossedCommitPointNow()
			&& !Transition.HasReachedCommitPoint();
		return bRelease ? EShanmenActionResourceDisposition::Release
			: EShanmenActionResourceDisposition::None;
	}

	FGuid MakeFinalizationCommandId(
		const FShanmenActionResourceReservationReceipt& Reservation,
		const FShanmenActionTransitionReceipt& Transition,
		EShanmenActionResourceDisposition Disposition)
	{
		if (!Reservation.IsValid() || !Transition.IsValid()
			|| Disposition == EShanmenActionResourceDisposition::None)
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Combat.ActionResource.FinalizationCommand.r1"),
			{
				GuidDigits(Reservation.GetReservationId()),
				GuidDigits(Reservation.GetReceiptId()),
				GuidDigits(Transition.GetActivationId()),
				FString::Printf(TEXT("%lld"), Transition.GetSequence()),
				FString::FromInt(static_cast<uint8>(Transition.GetFromPhase())),
				FString::FromInt(static_cast<uint8>(Transition.GetToPhase())),
				FString::FromInt(static_cast<uint8>(Transition.GetTerminalReason())),
				FString::FromInt(static_cast<uint8>(Disposition))
			});
	}

	FGuid MakeFinalizationReceiptId(
		const FShanmenActionResourceFinalizationRequest& Request,
		int64 RevisionBefore,
		int64 RevisionAfter,
		float CurrentBefore,
		float CurrentAfter,
		float MaximumAmount,
		float ReservedBefore,
		float ReservedAfter,
		float AvailableBefore,
		float AvailableAfter)
	{
		if (!Request.IsValid())
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Combat.ActionResource.FinalizationReceipt.r1"),
			{
				GuidDigits(Request.GetRequestId()),
				GuidDigits(Request.GetCommandId()),
				FString::Printf(TEXT("%lld"), RevisionBefore),
				FString::Printf(TEXT("%lld"), RevisionAfter),
				FloatBits(CurrentBefore),
				FloatBits(CurrentAfter),
				FloatBits(MaximumAmount),
				FloatBits(ReservedBefore),
				FloatBits(ReservedAfter),
				FloatBits(AvailableBefore),
				FloatBits(AvailableAfter)
			});
	}
}

bool FShanmenActionResourceCost::TryCapture(
	const FShanmenActionResourceCostCapture& Capture,
	FShanmenActionResourceCost& OutCost)
{
	OutCost = FShanmenActionResourceCost();
	FShanmenActionResourceCost Candidate;
	Candidate.RuleId = Capture.RuleId;
	Candidate.ResourceChannel = Capture.ResourceChannel;
	Candidate.Amount = Capture.Amount;
	Candidate.CostId = MakeCostId(
		Capture.RuleId, Capture.ResourceChannel, Capture.Amount);
	if (!Candidate.IsValid())
	{
		return false;
	}
	OutCost = Candidate;
	return true;
}

bool FShanmenActionResourceCost::IsValid() const
{
	return CostId.IsValid() && !RuleId.IsNone()
		&& ResourceChannel.IsValid()
		&& FMath::IsFinite(Amount) && Amount > 0.0f
		&& CostId == MakeCostId(RuleId, ResourceChannel, Amount);
}

bool FShanmenActionResourceSnapshot::IsValid() const
{
	return SnapshotId.IsValid() && OwnerEntityId.IsValid()
		&& ResourceChannel.IsValid() && AuthorityRevision >= 0
		&& IsValidResourceState(CurrentAmount, MaximumAmount, ReservedAmount)
		&& FloatsMatchExactly(
			AvailableAmount, CurrentAmount - ReservedAmount)
		&& SnapshotId == MakeSnapshotId(
			OwnerEntityId,
			ResourceChannel,
			AuthorityRevision,
			CurrentAmount,
			MaximumAmount,
			ReservedAmount,
			AvailableAmount);
}

bool FShanmenActionResourceReservationRequest::TryCreate(
	const FShanmenCombatActionSnapshot& Action,
	const FShanmenActionTransitionReceipt& StartupReceipt,
	const FShanmenActionResourceCost& Cost,
	const FShanmenActionResourceSnapshot& ResourceSnapshot,
	FShanmenActionResourceReservationRequest& OutRequest)
{
	OutRequest = FShanmenActionResourceReservationRequest();
	FShanmenActionResourceReservationRequest Candidate;
	Candidate.Action = Action;
	Candidate.StartupReceipt = StartupReceipt;
	Candidate.Cost = Cost;
	Candidate.ResourceSnapshot = ResourceSnapshot;
	Candidate.ReservationId = MakeReservationId(
		Action, Cost.GetResourceChannel());
	Candidate.CommandId = MakeReservationCommandId(
		Action, StartupReceipt, Cost, ResourceSnapshot);
	if (!Candidate.IsValid())
	{
		return false;
	}
	OutRequest = Candidate;
	return true;
}

bool FShanmenActionResourceReservationRequest::IsValid() const
{
	return ReservationId.IsValid() && CommandId.IsValid()
		&& Action.IsValid() && IsStartupReceipt(Action, StartupReceipt)
		&& Cost.IsValid() && ResourceSnapshot.IsValid()
		&& Action.GetSourceEntityId() == ResourceSnapshot.GetOwnerEntityId()
		&& Cost.GetResourceChannel()
			== ResourceSnapshot.GetResourceChannel()
		&& ReservationId == MakeReservationId(
			Action, Cost.GetResourceChannel())
		&& CommandId == MakeReservationCommandId(
			Action, StartupReceipt, Cost, ResourceSnapshot);
}

bool FShanmenActionResourceReservationReceipt::IsValid() const
{
	const float Amount = Request.GetCost().GetAmount();
	return ReceiptId.IsValid() && Request.IsValid()
		&& AuthorityRevisionBefore >= 0
		&& AuthorityRevisionBefore != MAX_int64
		&& AuthorityRevisionAfter == AuthorityRevisionBefore + 1
		&& IsValidResourceState(
			CurrentAmount, MaximumAmount, ReservedBefore)
		&& IsValidResourceState(
			CurrentAmount, MaximumAmount, ReservedAfter)
		&& FloatsMatchExactly(
			Request.GetResourceSnapshot().GetCurrentAmount(), CurrentAmount)
		&& FloatsMatchExactly(
			Request.GetResourceSnapshot().GetMaximumAmount(), MaximumAmount)
		&& Request.GetResourceSnapshot().GetAuthorityRevision()
			== AuthorityRevisionBefore
		&& FloatsMatchExactly(
			Request.GetResourceSnapshot().GetReservedAmount(), ReservedBefore)
		&& FloatsMatchExactly(ReservedAfter, ReservedBefore + Amount)
		&& FloatsMatchExactly(AvailableBefore, CurrentAmount - ReservedBefore)
		&& FloatsMatchExactly(AvailableAfter, CurrentAmount - ReservedAfter)
		&& AvailableAfter >= 0.0f
		&& ReceiptId == MakeReservationReceiptId(
			Request,
			AuthorityRevisionBefore,
			AuthorityRevisionAfter,
			CurrentAmount,
			MaximumAmount,
			ReservedBefore,
			ReservedAfter,
			AvailableBefore,
			AvailableAfter);
}

bool FShanmenActionResourceFinalizationRequest::TryCreate(
	const FShanmenActionResourceReservationReceipt& Reservation,
	const FShanmenActionTransitionReceipt& Transition,
	FShanmenActionResourceFinalizationRequest& OutRequest)
{
	OutRequest = FShanmenActionResourceFinalizationRequest();
	const EShanmenActionResourceDisposition Disposition =
		ClassifyTransition(Reservation, Transition);
	if (Disposition == EShanmenActionResourceDisposition::None)
	{
		return false;
	}
	FShanmenActionResourceFinalizationRequest Candidate;
	Candidate.RequestId = Reservation.GetReservationId();
	Candidate.Reservation = Reservation;
	Candidate.Transition = Transition;
	Candidate.Disposition = Disposition;
	Candidate.CommandId = MakeFinalizationCommandId(
		Reservation, Transition, Disposition);
	if (!Candidate.IsValid())
	{
		return false;
	}
	OutRequest = Candidate;
	return true;
}

bool FShanmenActionResourceFinalizationRequest::IsValid() const
{
	return RequestId.IsValid() && CommandId.IsValid()
		&& Reservation.IsValid() && Transition.IsValid()
		&& Disposition != EShanmenActionResourceDisposition::None
		&& RequestId == Reservation.GetReservationId()
		&& Disposition == ClassifyTransition(Reservation, Transition)
		&& CommandId == MakeFinalizationCommandId(
			Reservation, Transition, Disposition);
}

bool FShanmenActionResourceFinalizationReceipt::IsValid() const
{
	if (!ReceiptId.IsValid() || !Request.IsValid()
		|| AuthorityRevisionBefore < 0
		|| AuthorityRevisionBefore == MAX_int64
		|| AuthorityRevisionAfter != AuthorityRevisionBefore + 1
		|| !IsValidResourceState(
			CurrentBefore, MaximumAmount, ReservedBefore)
		|| !IsValidResourceState(
			CurrentAfter, MaximumAmount, ReservedAfter))
	{
		return false;
	}
	const float Amount =
		Request.GetReservation().GetRequest().GetCost().GetAmount();
	const bool bReservedDecreased =
		FloatsMatchExactly(ReservedAfter, ReservedBefore - Amount)
		&& ReservedAfter >= 0.0f;
	const bool bDispositionMath = Request.GetDisposition()
		== EShanmenActionResourceDisposition::Commit
		? FloatsMatchExactly(CurrentAfter, CurrentBefore - Amount)
		: Request.GetDisposition() == EShanmenActionResourceDisposition::Release
			&& FloatsMatchExactly(CurrentAfter, CurrentBefore);
	return bReservedDecreased && bDispositionMath
		&& FloatsMatchExactly(AvailableBefore, CurrentBefore - ReservedBefore)
		&& FloatsMatchExactly(AvailableAfter, CurrentAfter - ReservedAfter)
		&& ReceiptId == MakeFinalizationReceiptId(
			Request,
			AuthorityRevisionBefore,
			AuthorityRevisionAfter,
			CurrentBefore,
			CurrentAfter,
			MaximumAmount,
			ReservedBefore,
			ReservedAfter,
			AvailableBefore,
			AvailableAfter);
}

bool FShanmenActionResourceTransactionResult::IsValid() const
{
	switch (Status)
	{
	case EShanmenActionResourceTransactionStatus::Reserved:
	case EShanmenActionResourceTransactionStatus::AlreadyReserved:
		return Error == EShanmenActionResourceTransactionError::None
			&& Reservation.IsValid() && !Finalization.IsValid();
	case EShanmenActionResourceTransactionStatus::Committed:
		return Error == EShanmenActionResourceTransactionError::None
			&& !Reservation.IsValid() && Finalization.IsValid()
			&& Finalization.GetRequest().GetDisposition()
				== EShanmenActionResourceDisposition::Commit;
	case EShanmenActionResourceTransactionStatus::Released:
		return Error == EShanmenActionResourceTransactionError::None
			&& !Reservation.IsValid() && Finalization.IsValid()
			&& Finalization.GetRequest().GetDisposition()
				== EShanmenActionResourceDisposition::Release;
	case EShanmenActionResourceTransactionStatus::AlreadyFinalized:
		return Error == EShanmenActionResourceTransactionError::None
			&& !Reservation.IsValid() && Finalization.IsValid();
	case EShanmenActionResourceTransactionStatus::Rejected:
		return Error != EShanmenActionResourceTransactionError::None
			&& !Reservation.IsValid() && !Finalization.IsValid();
	default:
		return false;
	}
}

bool FShanmenActionResourceTransactionResult::IsSuccess() const
{
	return IsValid()
		&& Status != EShanmenActionResourceTransactionStatus::Rejected;
}

bool FShanmenActionResourceAuthority::TryCreate(
	const FGuid& OwnerEntityId,
	const FGameplayTag& ResourceChannel,
	float CurrentAmount,
	float MaximumAmount,
	int64 AuthorityRevision,
	FShanmenActionResourceAuthority& OutAuthority)
{
	OutAuthority.Reset();
	if (!OwnerEntityId.IsValid() || !ResourceChannel.IsValid()
		|| AuthorityRevision < 0
		|| !IsValidResourceState(CurrentAmount, MaximumAmount, 0.0f))
	{
		return false;
	}
	FShanmenActionResourceAuthority Candidate;
	Candidate.OwnerEntityId = OwnerEntityId;
	Candidate.ResourceChannel = ResourceChannel;
	Candidate.CurrentAmount = CurrentAmount;
	Candidate.MaximumAmount = MaximumAmount;
	Candidate.ReservedAmount = 0.0f;
	Candidate.AuthorityRevision = AuthorityRevision;
	Candidate.bInitialized = true;
	if (!Candidate.IsValid())
	{
		return false;
	}
	OutAuthority = Candidate;
	return true;
}

bool FShanmenActionResourceAuthority::IsValid() const
{
	if (!bInitialized || !OwnerEntityId.IsValid()
		|| !ResourceChannel.IsValid() || AuthorityRevision < 0
		|| !IsValidResourceState(
			CurrentAmount, MaximumAmount, ReservedAmount))
	{
		return false;
	}

	TArray<const FTransactionRecord*> Pending;
	for (const TPair<FGuid, FTransactionRecord>& Pair : Transactions)
	{
		const FTransactionRecord& Record = Pair.Value;
		if (Pair.Key != Record.Reservation.GetReservationId()
			|| !Record.Reservation.IsValid()
			|| Record.ReservationCommandId
				!= Record.Reservation.GetRequest().GetCommandId())
		{
			return false;
		}
		if (Record.Finalization.GetReceiptId().IsValid())
		{
			if (!Record.Finalization.IsValid()
				|| Record.FinalizationCommandId
					!= Record.Finalization.GetRequest().GetCommandId()
				|| Record.Finalization.GetRequest().GetRequestId() != Pair.Key)
			{
				return false;
			}
		}
		else
		{
			if (Record.FinalizationCommandId.IsValid())
			{
				return false;
			}
			Pending.Add(&Record);
		}
	}
	Pending.Sort([](const FTransactionRecord& Left, const FTransactionRecord& Right)
	{
		return Left.Reservation.GetReservationId().ToString(EGuidFormats::Digits)
			< Right.Reservation.GetReservationId().ToString(EGuidFormats::Digits);
	});
	double PendingTotal = 0.0;
	for (const FTransactionRecord* Record : Pending)
	{
		if (!Record)
		{
			return false;
		}
		PendingTotal += Record->Reservation.GetRequest().GetCost().GetAmount();
	}
	return FMath::IsNearlyEqual(
		static_cast<float>(PendingTotal), ReservedAmount);
}

bool FShanmenActionResourceAuthority::TryCaptureSnapshot(
	FShanmenActionResourceSnapshot& OutSnapshot) const
{
	OutSnapshot = FShanmenActionResourceSnapshot();
	if (!IsValid())
	{
		return false;
	}
	OutSnapshot.OwnerEntityId = OwnerEntityId;
	OutSnapshot.ResourceChannel = ResourceChannel;
	OutSnapshot.AuthorityRevision = AuthorityRevision;
	OutSnapshot.CurrentAmount = CurrentAmount;
	OutSnapshot.MaximumAmount = MaximumAmount;
	OutSnapshot.ReservedAmount = ReservedAmount;
	OutSnapshot.AvailableAmount = GetAvailableAmount();
	OutSnapshot.SnapshotId = MakeSnapshotId(
		OwnerEntityId,
		ResourceChannel,
		AuthorityRevision,
		CurrentAmount,
		MaximumAmount,
		ReservedAmount,
		OutSnapshot.AvailableAmount);
	return OutSnapshot.IsValid();
}

FShanmenActionResourceTransactionResult
FShanmenActionResourceAuthority::Reserve(
	const FShanmenActionResourceReservationRequest& Request)
{
	if (!Request.IsValid())
	{
		return Reject(EShanmenActionResourceTransactionError::InvalidRequest);
	}
	if (!IsValid())
	{
		return Reject(EShanmenActionResourceTransactionError::AuthorityNotReady);
	}
	if (Request.GetResourceSnapshot().GetOwnerEntityId() != OwnerEntityId)
	{
		return Reject(EShanmenActionResourceTransactionError::OwnerMismatch);
	}
	if (Request.GetCost().GetResourceChannel() != ResourceChannel
		|| Request.GetResourceSnapshot().GetResourceChannel()
			!= ResourceChannel)
	{
		return Reject(EShanmenActionResourceTransactionError::ChannelMismatch);
	}

	if (const FTransactionRecord* Existing =
		Transactions.Find(Request.GetReservationId()))
	{
		if (Existing->ReservationCommandId == Request.GetCommandId())
		{
			FShanmenActionResourceTransactionResult Result;
			Result.Status =
				EShanmenActionResourceTransactionStatus::AlreadyReserved;
			Result.Reservation = Existing->Reservation;
			return Result;
		}
		return Reject(
			EShanmenActionResourceTransactionError::ReservationConflict);
	}

	const FShanmenActionResourceSnapshot& Snapshot =
		Request.GetResourceSnapshot();
	if (Snapshot.GetAuthorityRevision() != AuthorityRevision
		|| !FloatsMatchExactly(Snapshot.GetCurrentAmount(), CurrentAmount)
		|| !FloatsMatchExactly(Snapshot.GetMaximumAmount(), MaximumAmount)
		|| !FloatsMatchExactly(Snapshot.GetReservedAmount(), ReservedAmount)
		|| !FloatsMatchExactly(Snapshot.GetAvailableAmount(), GetAvailableAmount()))
	{
		return Reject(EShanmenActionResourceTransactionError::StaleSnapshot);
	}
	if (Request.GetCost().GetAmount() > GetAvailableAmount())
	{
		return Reject(
			EShanmenActionResourceTransactionError::InsufficientAvailable);
	}
	if (AuthorityRevision == MAX_int64)
	{
		return Reject(
			EShanmenActionResourceTransactionError::RevisionExhausted);
	}

	const float NewReserved =
		ReservedAmount + Request.GetCost().GetAmount();
	if (!IsValidResourceState(CurrentAmount, MaximumAmount, NewReserved))
	{
		return Reject(
			EShanmenActionResourceTransactionError::StateDesynchronized);
	}

	FShanmenActionResourceReservationReceipt Receipt;
	Receipt.Request = Request;
	Receipt.AuthorityRevisionBefore = AuthorityRevision;
	Receipt.AuthorityRevisionAfter = AuthorityRevision + 1;
	Receipt.CurrentAmount = CurrentAmount;
	Receipt.MaximumAmount = MaximumAmount;
	Receipt.ReservedBefore = ReservedAmount;
	Receipt.ReservedAfter = NewReserved;
	Receipt.AvailableBefore = GetAvailableAmount();
	Receipt.AvailableAfter = CurrentAmount - NewReserved;
	Receipt.ReceiptId = MakeReservationReceiptId(
		Request,
		Receipt.AuthorityRevisionBefore,
		Receipt.AuthorityRevisionAfter,
		Receipt.CurrentAmount,
		Receipt.MaximumAmount,
		Receipt.ReservedBefore,
		Receipt.ReservedAfter,
		Receipt.AvailableBefore,
		Receipt.AvailableAfter);
	if (!Receipt.IsValid())
	{
		return Reject(
			EShanmenActionResourceTransactionError::StateDesynchronized);
	}

	FShanmenActionResourceAuthority Candidate = *this;
	Candidate.ReservedAmount = NewReserved;
	Candidate.AuthorityRevision = Receipt.AuthorityRevisionAfter;
	FTransactionRecord Record;
	Record.ReservationCommandId = Request.GetCommandId();
	Record.Reservation = Receipt;
	Candidate.Transactions.Add(Request.GetReservationId(), Record);
	if (!Candidate.IsValid())
	{
		return Reject(
			EShanmenActionResourceTransactionError::StateDesynchronized);
	}
	*this = MoveTemp(Candidate);

	FShanmenActionResourceTransactionResult Result;
	Result.Status = EShanmenActionResourceTransactionStatus::Reserved;
	Result.Reservation = Receipt;
	return Result;
}

FShanmenActionResourceTransactionResult
FShanmenActionResourceAuthority::Finalize(
	const FShanmenActionResourceFinalizationRequest& Request)
{
	if (!Request.IsValid())
	{
		return Reject(EShanmenActionResourceTransactionError::InvalidRequest);
	}
	if (!IsValid())
	{
		return Reject(EShanmenActionResourceTransactionError::AuthorityNotReady);
	}
	const FShanmenActionResourceReservationRequest& ReservationRequest =
		Request.GetReservation().GetRequest();
	if (ReservationRequest.GetResourceSnapshot().GetOwnerEntityId()
		!= OwnerEntityId)
	{
		return Reject(EShanmenActionResourceTransactionError::OwnerMismatch);
	}
	if (ReservationRequest.GetCost().GetResourceChannel() != ResourceChannel)
	{
		return Reject(EShanmenActionResourceTransactionError::ChannelMismatch);
	}

	FTransactionRecord* Record = Transactions.Find(Request.GetRequestId());
	if (!Record)
	{
		return Reject(
			EShanmenActionResourceTransactionError::ReservationNotFound);
	}
	if (Record->Finalization.IsValid())
	{
		if (Record->FinalizationCommandId == Request.GetCommandId())
		{
			FShanmenActionResourceTransactionResult Result;
			Result.Status =
				EShanmenActionResourceTransactionStatus::AlreadyFinalized;
			Result.Finalization = Record->Finalization;
			return Result;
		}
		return Reject(
			EShanmenActionResourceTransactionError::FinalizationConflict);
	}
	if (Record->Reservation.GetReceiptId()
			!= Request.GetReservation().GetReceiptId()
		|| Record->ReservationCommandId
			!= ReservationRequest.GetCommandId())
	{
		return Reject(
			EShanmenActionResourceTransactionError::ReservationConflict);
	}
	if (Request.GetDisposition() == EShanmenActionResourceDisposition::None)
	{
		return Reject(
			EShanmenActionResourceTransactionError::TransitionMismatch);
	}
	if (AuthorityRevision == MAX_int64)
	{
		return Reject(
			EShanmenActionResourceTransactionError::RevisionExhausted);
	}

	const float Amount = ReservationRequest.GetCost().GetAmount();
	if (ReservedAmount < Amount || CurrentAmount < Amount)
	{
		return Reject(
			EShanmenActionResourceTransactionError::StateDesynchronized);
	}
	const float NewReserved = ReservedAmount - Amount;
	const float NewCurrent = Request.GetDisposition()
		== EShanmenActionResourceDisposition::Commit
		? CurrentAmount - Amount : CurrentAmount;
	if (!IsValidResourceState(NewCurrent, MaximumAmount, NewReserved))
	{
		return Reject(
			EShanmenActionResourceTransactionError::StateDesynchronized);
	}

	FShanmenActionResourceFinalizationReceipt Receipt;
	Receipt.Request = Request;
	Receipt.AuthorityRevisionBefore = AuthorityRevision;
	Receipt.AuthorityRevisionAfter = AuthorityRevision + 1;
	Receipt.CurrentBefore = CurrentAmount;
	Receipt.CurrentAfter = NewCurrent;
	Receipt.MaximumAmount = MaximumAmount;
	Receipt.ReservedBefore = ReservedAmount;
	Receipt.ReservedAfter = NewReserved;
	Receipt.AvailableBefore = GetAvailableAmount();
	Receipt.AvailableAfter = NewCurrent - NewReserved;
	Receipt.ReceiptId = MakeFinalizationReceiptId(
		Request,
		Receipt.AuthorityRevisionBefore,
		Receipt.AuthorityRevisionAfter,
		Receipt.CurrentBefore,
		Receipt.CurrentAfter,
		Receipt.MaximumAmount,
		Receipt.ReservedBefore,
		Receipt.ReservedAfter,
		Receipt.AvailableBefore,
		Receipt.AvailableAfter);
	if (!Receipt.IsValid())
	{
		return Reject(
			EShanmenActionResourceTransactionError::StateDesynchronized);
	}

	FShanmenActionResourceAuthority Candidate = *this;
	Candidate.CurrentAmount = NewCurrent;
	Candidate.ReservedAmount = NewReserved;
	Candidate.AuthorityRevision = Receipt.AuthorityRevisionAfter;
	FTransactionRecord* CandidateRecord =
		Candidate.Transactions.Find(Request.GetRequestId());
	if (!CandidateRecord)
	{
		return Reject(
			EShanmenActionResourceTransactionError::StateDesynchronized);
	}
	CandidateRecord->FinalizationCommandId = Request.GetCommandId();
	CandidateRecord->Finalization = Receipt;
	if (!Candidate.IsValid())
	{
		return Reject(
			EShanmenActionResourceTransactionError::StateDesynchronized);
	}
	*this = MoveTemp(Candidate);

	FShanmenActionResourceTransactionResult Result;
	Result.Status = Request.GetDisposition()
		== EShanmenActionResourceDisposition::Commit
		? EShanmenActionResourceTransactionStatus::Committed
		: EShanmenActionResourceTransactionStatus::Released;
	Result.Finalization = Receipt;
	return Result;
}

void FShanmenActionResourceAuthority::Reset()
{
	*this = FShanmenActionResourceAuthority();
}

int32 FShanmenActionResourceAuthority::NumPendingReservations() const
{
	int32 Count = 0;
	for (const TPair<FGuid, FTransactionRecord>& Pair : Transactions)
	{
		Count += Pair.Value.Finalization.IsValid() ? 0 : 1;
	}
	return Count;
}

FShanmenActionResourceTransactionResult
FShanmenActionResourceAuthority::Reject(
	EShanmenActionResourceTransactionError Error) const
{
	FShanmenActionResourceTransactionResult Result;
	Result.Status = EShanmenActionResourceTransactionStatus::Rejected;
	Result.Error = Error;
	return Result;
}
