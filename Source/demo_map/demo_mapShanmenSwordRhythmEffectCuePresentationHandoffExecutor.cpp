#include "demo_mapShanmenSwordRhythmEffectCuePresentationHandoffExecutor.h"

#include "ShanmenDeterministicId.h"

namespace
{
	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	bool IsPresentationChannel(
		const Edemo_mapShanmenSwordRhythmEffectCueChannel Channel)
	{
		return Channel
				== Edemo_mapShanmenSwordRhythmEffectCueChannel::Visual
			|| Channel
				== Edemo_mapShanmenSwordRhythmEffectCueChannel::Audio;
	}

	FGuid MakeHandoffId(
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutorInvocation& Invocation)
	{
		if (!Invocation.IsValid())
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.Combat.SwordRhythm.EffectCuePresentationHandoff.r1"),
			{
				GuidDigits(Invocation.GetInvocationId()),
				GuidDigits(Invocation.GetRoute().GetRouteId()),
				GuidDigits(Invocation.GetRoute().GetDelivery().GetScope().GetRunId()),
				GuidDigits(Invocation.GetAttemptId()),
				FString::FromInt(static_cast<uint8>(
					Invocation.GetRoute().GetChannel()))
			});
	}

	FGuid MakeExecutorReceiptId(
		const Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoff& Handoff)
	{
		if (!Handoff.IsValid())
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.Combat.SwordRhythm.EffectCuePresentationHandoffExecutorReceipt.r1"),
			{
				GuidDigits(Handoff.GetHandoffId()),
				GuidDigits(Handoff.GetInvocation().GetInvocationId()),
				GuidDigits(Handoff.GetInvocation().GetAttemptId()),
				FString::FromInt(static_cast<uint8>(Handoff.GetChannel()))
			});
	}

	Fdemo_mapShanmenSwordRhythmEffectCueExecutorResult Rejected(
		const TCHAR* Diagnostic)
	{
		Fdemo_mapShanmenSwordRhythmEffectCueExecutorResult Result;
		Result.Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutorStatus::Rejected;
		Result.Diagnostic = Diagnostic;
		return Result;
	}

	Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoffConsumeResult
		ConsumeRejected(
			const Edemo_mapShanmenSwordRhythmEffectCuePresentationHandoffConsumeStatus
				Status,
			FString Diagnostic)
	{
		Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoffConsumeResult
			Result;
		Result.Status = Status;
		Result.Diagnostic = MoveTemp(Diagnostic);
		return Result;
	}
}

bool Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoff::TryCreate(
	const Fdemo_mapShanmenSwordRhythmEffectCueExecutorInvocation& InInvocation,
	Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoff& OutHandoff)
{
	OutHandoff =
		Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoff();
	Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoff Candidate;
	Candidate.Invocation = InInvocation;
	Candidate.HandoffId = MakeHandoffId(Candidate.Invocation);
	if (!Candidate.IsValid())
	{
		return false;
	}
	OutHandoff = MoveTemp(Candidate);
	return true;
}

bool Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoff::IsValid() const
{
	return Invocation.IsValid() && IsPresentationChannel(GetChannel())
		&& HandoffId.IsValid() && HandoffId == MakeHandoffId(Invocation);
}

bool Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoff::Matches(
	const Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoff& Other) const
{
	return IsValid() && Other.IsValid() && HandoffId == Other.HandoffId
		&& Invocation.Matches(Other.Invocation);
}

bool Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoffConsumeResult::
	IsSuccess() const
{
	return (Status
				== Edemo_mapShanmenSwordRhythmEffectCuePresentationHandoffConsumeStatus::
					Consumed
			|| Status
				== Edemo_mapShanmenSwordRhythmEffectCuePresentationHandoffConsumeStatus::
					AlreadyConsumed)
		&& Handoff.IsValid();
}

bool Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoffExecutor::
	TryCreate(
		const FGuid& InRunId,
		const Edemo_mapShanmenSwordRhythmEffectCueChannel InChannel,
		Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoffExecutor&
			OutExecutor)
{
	OutExecutor =
		Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoffExecutor();
	if (!InRunId.IsValid() || !IsPresentationChannel(InChannel))
	{
		return false;
	}
	OutExecutor.RunId = InRunId;
	OutExecutor.Channel = InChannel;
	return OutExecutor.IsValid();
}

const Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoffExecutor::
	FAcceptedInvocationRecord*
Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoffExecutor::
	FindAcceptedInvocation(const FGuid& InvocationId) const
{
	return AcceptedInvocations.FindByPredicate(
		[&InvocationId](const FAcceptedInvocationRecord& Record)
		{
			return Record.Handoff.GetInvocation().GetInvocationId()
				== InvocationId;
		});
}

Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoffExecutor::
	FAcceptedInvocationRecord*
Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoffExecutor::
	FindAcceptedHandoff(const FGuid& HandoffId)
{
	return AcceptedInvocations.FindByPredicate(
		[&HandoffId](const FAcceptedInvocationRecord& Record)
		{
			return Record.Handoff.GetHandoffId() == HandoffId;
		});
}

const Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoffExecutor::
	FAcceptedInvocationRecord*
Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoffExecutor::
	FindAcceptedHandoff(const FGuid& HandoffId) const
{
	return AcceptedInvocations.FindByPredicate(
		[&HandoffId](const FAcceptedInvocationRecord& Record)
		{
			return Record.Handoff.GetHandoffId() == HandoffId;
		});
}

bool Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoffExecutor::
	IsValid() const
{
	if (!RunId.IsValid() || !IsPresentationChannel(Channel)
		|| bHasPendingHandoff != PendingHandoffId.IsValid())
	{
		return false;
	}

	TArray<FGuid> InvocationIds;
	TArray<FGuid> HandoffIds;
	int32 PendingMatchCount = 0;
	for (const FAcceptedInvocationRecord& Record : AcceptedInvocations)
	{
		const auto& Handoff = Record.Handoff;
		const auto& Invocation = Handoff.GetInvocation();
		if (!Handoff.IsValid() || !Record.Result.IsSuccess()
			|| !Record.Result.Receipt.Matches(Invocation)
			|| Record.Result.Receipt.GetOutcome()
				!= Edemo_mapShanmenSwordRhythmEffectCueAttemptOutcome::Succeeded
			|| Invocation.GetRoute().GetDelivery().GetScope().GetRunId()
				!= RunId
			|| Handoff.GetChannel() != Channel
			|| InvocationIds.Contains(Invocation.GetInvocationId())
			|| HandoffIds.Contains(Handoff.GetHandoffId()))
		{
			return false;
		}
		InvocationIds.Add(Invocation.GetInvocationId());
		HandoffIds.Add(Handoff.GetHandoffId());
		if (Handoff.GetHandoffId() == PendingHandoffId)
		{
			if (Record.bConsumed)
			{
				return false;
			}
			++PendingMatchCount;
		}
		else if (!Record.bConsumed)
		{
			return false;
		}
	}
	return bHasPendingHandoff ? PendingMatchCount == 1
		: PendingMatchCount == 0;
}

Fdemo_mapShanmenSwordRhythmEffectCueExecutorResult
Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoffExecutor::Execute(
	const Fdemo_mapShanmenSwordRhythmEffectCueExecutorInvocation& Invocation)
{
	if (!IsValid())
	{
		return Rejected(
			TEXT("Presentation handoff executor is internally inconsistent."));
	}
	if (!Invocation.IsValid())
	{
		return Rejected(
			TEXT("Presentation handoff requires one valid cue invocation."));
	}
	if (Invocation.GetRoute().GetDelivery().GetScope().GetRunId() != RunId)
	{
		return Rejected(
			TEXT("Cue invocation belongs to a different combat Run."));
	}
	if (Invocation.GetRoute().GetChannel() != Channel)
	{
		return Rejected(
			TEXT("Cue invocation belongs to a different presentation channel."));
	}
	for (const auto& Command : Invocation.GetCommands())
	{
		if (!Command.IsValid() || Command.GetChannel() != Channel)
		{
			return Rejected(
				TEXT("Cue invocation contains a cross-channel or invalid command."));
		}
	}

	if (const FAcceptedInvocationRecord* Existing =
			FindAcceptedInvocation(Invocation.GetInvocationId()))
	{
		return Existing->Handoff.GetInvocation().Matches(Invocation)
			? Existing->Result
			: Rejected(
				TEXT("Invocation identity conflicts with accepted handoff evidence."));
	}
	if (bHasPendingHandoff)
	{
		return Rejected(
			TEXT("A different presentation handoff remains pending consumption."));
	}

	Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoff Handoff;
	if (!Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoff::TryCreate(
			Invocation, Handoff))
	{
		return Rejected(
			TEXT("Cue invocation could not form immutable handoff evidence."));
	}

	Fdemo_mapShanmenSwordRhythmEffectCueExecutorResult Result;
	if (!Fdemo_mapShanmenSwordRhythmEffectCueExecutorReceipt::TryCreate(
			Invocation,
			MakeExecutorReceiptId(Handoff),
			Edemo_mapShanmenSwordRhythmEffectCueAttemptOutcome::Succeeded,
			Result.Receipt))
	{
		return Rejected(
			TEXT("Presentation handoff could not form executor receipt evidence."));
	}
	Result.Status = Edemo_mapShanmenSwordRhythmEffectCueExecutorStatus::Completed;
	Result.Diagnostic =
		TEXT("Complete cue batch published to the caller-owned presentation outbox.");

	FAcceptedInvocationRecord& Record =
		AcceptedInvocations.AddDefaulted_GetRef();
	Record.Handoff = Handoff;
	Record.Result = Result;
	PendingHandoffId = Handoff.GetHandoffId();
	bHasPendingHandoff = true;
	if (!IsValid())
	{
		AcceptedInvocations.Pop();
		PendingHandoffId.Invalidate();
		bHasPendingHandoff = false;
		return Rejected(
			TEXT("Presentation handoff failed executor self-validation."));
	}
	return Result;
}

bool Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoffExecutor::
	TryGetPendingHandoff(
		Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoff& OutHandoff)
		const
{
	OutHandoff =
		Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoff();
	if (!IsValid() || !bHasPendingHandoff)
	{
		return false;
	}
	const FAcceptedInvocationRecord* Record =
		FindAcceptedHandoff(PendingHandoffId);
	if (!Record || Record->bConsumed)
	{
		return false;
	}
	OutHandoff = Record->Handoff;
	return OutHandoff.IsValid();
}

Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoffConsumeResult
Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoffExecutor::Consume(
	const FGuid& HandoffId)
{
	if (!IsValid())
	{
		return ConsumeRejected(
			Edemo_mapShanmenSwordRhythmEffectCuePresentationHandoffConsumeStatus::
				ExecutorInvalid,
			TEXT("Presentation handoff executor is internally inconsistent."));
	}
	if (!HandoffId.IsValid())
	{
		return ConsumeRejected(
			Edemo_mapShanmenSwordRhythmEffectCuePresentationHandoffConsumeStatus::
				HandoffIdInvalid,
			TEXT("Consumption requires one valid handoff identity."));
	}
	FAcceptedInvocationRecord* Record = FindAcceptedHandoff(HandoffId);
	if (Record && Record->bConsumed)
	{
		Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoffConsumeResult
			Result;
		Result.Status =
			Edemo_mapShanmenSwordRhythmEffectCuePresentationHandoffConsumeStatus::
				AlreadyConsumed;
		Result.Diagnostic =
			TEXT("Presentation handoff consumption replayed from accepted evidence.");
		Result.Handoff = Record->Handoff;
		return Result;
	}
	if (!bHasPendingHandoff)
	{
		return ConsumeRejected(
			Edemo_mapShanmenSwordRhythmEffectCuePresentationHandoffConsumeStatus::
				NoPendingHandoff,
			TEXT("No presentation handoff is pending consumption."));
	}
	if (PendingHandoffId != HandoffId || !Record)
	{
		return ConsumeRejected(
			Edemo_mapShanmenSwordRhythmEffectCuePresentationHandoffConsumeStatus::
				PendingHandoffMismatch,
			TEXT("Requested handoff does not match the pending publication."));
	}

	Record->bConsumed = true;
	PendingHandoffId.Invalidate();
	bHasPendingHandoff = false;
	if (!IsValid())
	{
		Record->bConsumed = false;
		PendingHandoffId = HandoffId;
		bHasPendingHandoff = true;
		return ConsumeRejected(
			Edemo_mapShanmenSwordRhythmEffectCuePresentationHandoffConsumeStatus::
				StateInvalid,
			TEXT("Presentation handoff consumption failed self-validation."));
	}

	Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoffConsumeResult Result;
	Result.Status =
		Edemo_mapShanmenSwordRhythmEffectCuePresentationHandoffConsumeStatus::
			Consumed;
	Result.Diagnostic =
		TEXT("Caller consumed the published presentation handoff.");
	Result.Handoff = Record->Handoff;
	return Result;
}

void Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoffExecutor::Reset()
{
	*this =
		Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoffExecutor();
}
