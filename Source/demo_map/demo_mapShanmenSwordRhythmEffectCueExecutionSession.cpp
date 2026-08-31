#include "demo_mapShanmenSwordRhythmEffectCueExecutionSession.h"

#include "ShanmenDeterministicId.h"

namespace
{
	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	bool IsOrderedBatch(
		const TArray<Fdemo_mapShanmenSwordRhythmEffectCueEvent>& Events,
		FGuid& OutRunId)
	{
		OutRunId.Invalidate();
		if (Events.IsEmpty())
		{
			return false;
		}
		int32 PreviousRevision = INDEX_NONE;
		for (int32 Index = 0; Index < Events.Num(); ++Index)
		{
			const auto& Event = Events[Index];
			if (!Event.IsValid())
			{
				return false;
			}
			const auto& State = Event.GetState();
			if (Index == 0)
			{
				OutRunId = State.GetRunId();
			}
			if (!OutRunId.IsValid()
				|| State.GetRunId() != OutRunId
				|| (Index > 0
					&& State.GetObservationRevision() <= PreviousRevision))
			{
				OutRunId.Invalidate();
				return false;
			}
			PreviousRevision = State.GetObservationRevision();
		}
		return true;
	}

	FGuid MakeBatchId(
		const FGuid& RunId,
		const FGuid& VisualConsumerId,
		const FGuid& AudioConsumerId,
		const TArray<Fdemo_mapShanmenSwordRhythmEffectCueEvent>& Events)
	{
		FGuid ValidatedRun;
		if (!RunId.IsValid()
			|| !VisualConsumerId.IsValid()
			|| !AudioConsumerId.IsValid()
			|| VisualConsumerId == AudioConsumerId
			|| !IsOrderedBatch(Events, ValidatedRun)
			|| ValidatedRun != RunId)
		{
			return FGuid();
		}
		TArray<FString> Parts;
		Parts.Reserve(4 + Events.Num() * 2);
		Parts.Add(GuidDigits(RunId));
		Parts.Add(GuidDigits(VisualConsumerId));
		Parts.Add(GuidDigits(AudioConsumerId));
		Parts.Add(FString::FromInt(Events.Num()));
		for (const auto& Event : Events)
		{
			Parts.Add(GuidDigits(Event.GetEventId()));
			Parts.Add(FString::FromInt(
				Event.GetState().GetObservationRevision()));
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.Combat.SwordRhythm.EffectCueExecutionSession.r1"),
			Parts);
	}

	int32 FindEventIndex(
		const TArray<Fdemo_mapShanmenSwordRhythmEffectCueEvent>& Events,
		const Fdemo_mapShanmenSwordRhythmEffectCueEvent& Event)
	{
		for (int32 Index = 0; Index < Events.Num(); ++Index)
		{
			if (Events[Index].Matches(Event))
			{
				return Index;
			}
		}
		return INDEX_NONE;
	}

	bool IsConsumerProgressValid(
		const Fdemo_mapShanmenSwordRhythmEffectCueConsumerCoordinator& Consumer,
		const TArray<Fdemo_mapShanmenSwordRhythmEffectCueEvent>& Events,
		const int32 NextEventIndex)
	{
		Fdemo_mapShanmenSwordRhythmEffectCueAcknowledgementReceipt
			Acknowledgement;
		if (!Consumer.TryGetLastAcknowledgement(Acknowledgement))
		{
			return NextEventIndex == 0;
		}
		const int32 AcknowledgedIndex = FindEventIndex(
			Events, Acknowledgement.GetDelivery().GetEvent());
		if (AcknowledgedIndex == INDEX_NONE)
		{
			return false;
		}
		if (NextEventIndex > 0
			&& AcknowledgedIndex == NextEventIndex - 1)
		{
			return true;
		}
		return NextEventIndex < Events.Num()
			&& AcknowledgedIndex == NextEventIndex;
	}

	Fdemo_mapShanmenSwordRhythmEffectCueSessionResult Reject(
		const Edemo_mapShanmenSwordRhythmEffectCueSessionStatus Status,
		FString Diagnostic)
	{
		Fdemo_mapShanmenSwordRhythmEffectCueSessionResult Result;
		Result.Status = Status;
		Result.Diagnostic = MoveTemp(Diagnostic);
		return Result;
	}
}

bool Fdemo_mapShanmenSwordRhythmEffectCueSessionResult::IsSuccess() const
{
	if (TotalEvents <= 0
		|| CompletedEvents < 0
		|| CompletedEvents > TotalEvents)
	{
		return false;
	}
	switch (Status)
	{
	case Edemo_mapShanmenSwordRhythmEffectCueSessionStatus::EventCompleted:
		return Event.IsValid()
			&& EventIndex >= 0
			&& CompletedEvents == EventIndex + 1
			&& CompletedEvents < TotalEvents
			&& Host.IsComplete();
	case Edemo_mapShanmenSwordRhythmEffectCueSessionStatus::BatchCompleted:
		return Event.IsValid()
			&& EventIndex == TotalEvents - 1
			&& CompletedEvents == TotalEvents
			&& Host.IsComplete();
	case Edemo_mapShanmenSwordRhythmEffectCueSessionStatus::AlreadyCompleted:
		return !Event.IsValid()
			&& EventIndex == INDEX_NONE
			&& CompletedEvents == TotalEvents;
	case Edemo_mapShanmenSwordRhythmEffectCueSessionStatus::RetryPending:
		return Event.IsValid()
			&& EventIndex == CompletedEvents
			&& CompletedEvents < TotalEvents
			&& Host.Status
				== Edemo_mapShanmenSwordRhythmEffectCueHostStatus::RetryPending
			&& Host.IsSuccess()
			&& !Host.IsComplete();
	default:
		return false;
	}
}

bool Fdemo_mapShanmenSwordRhythmEffectCueSessionResult::IsBatchComplete() const
{
	return (Status
			== Edemo_mapShanmenSwordRhythmEffectCueSessionStatus::BatchCompleted
		|| Status
			== Edemo_mapShanmenSwordRhythmEffectCueSessionStatus::AlreadyCompleted)
		&& IsSuccess();
}

bool Fdemo_mapShanmenSwordRhythmEffectCueExecutionSession::TryCreate(
	const TArray<Fdemo_mapShanmenSwordRhythmEffectCueEvent>& InEvents,
	const FGuid& VisualConsumerId,
	const FGuid& AudioConsumerId,
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionSession& OutSession)
{
	OutSession = Fdemo_mapShanmenSwordRhythmEffectCueExecutionSession();
	FGuid InRunId;
	if (!IsOrderedBatch(InEvents, InRunId))
	{
		return false;
	}
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionSession Candidate;
	Candidate.RunId = InRunId;
	Candidate.Events = InEvents;
	Candidate.NextEventIndex = 0;
	if (!Fdemo_mapShanmenSwordRhythmEffectCueExecutionHost::TryCreate(
			InRunId,
			VisualConsumerId,
			AudioConsumerId,
			Candidate.Host))
	{
		return false;
	}
	Candidate.BatchId = MakeBatchId(
		InRunId, VisualConsumerId, AudioConsumerId, InEvents);
	if (!Candidate.IsValid())
	{
		return false;
	}
	OutSession = MoveTemp(Candidate);
	return true;
}

Fdemo_mapShanmenSwordRhythmEffectCueSessionResult
Fdemo_mapShanmenSwordRhythmEffectCueExecutionSession::ProcessNext(
	const FGuid& VisualAttemptId,
	const FGuid& AudioAttemptId,
	Idemo_mapShanmenSwordRhythmEffectCueExecutor& VisualExecutor,
	Idemo_mapShanmenSwordRhythmEffectCueExecutor& AudioExecutor)
{
	if (!IsValid())
	{
		return Reject(
			Edemo_mapShanmenSwordRhythmEffectCueSessionStatus::SessionInvalid,
			TEXT("Cue execution session requires one valid ordered batch."));
	}
	if (IsComplete())
	{
		auto Result = Reject(
			Edemo_mapShanmenSwordRhythmEffectCueSessionStatus::AlreadyCompleted,
			TEXT("Cue execution batch is already complete."));
		Result.CompletedEvents = NextEventIndex;
		Result.TotalEvents = Events.Num();
		return Result;
	}
	if (!VisualAttemptId.IsValid() || !AudioAttemptId.IsValid())
	{
		auto Result = Reject(
			Edemo_mapShanmenSwordRhythmEffectCueSessionStatus::AttemptInvalid,
			TEXT("Current event requires both caller-owned attempt identities."));
		Result.EventIndex = NextEventIndex;
		Result.CompletedEvents = NextEventIndex;
		Result.TotalEvents = Events.Num();
		Result.Event = Events[NextEventIndex];
		return Result;
	}

	const auto Before = *this;
	Fdemo_mapShanmenSwordRhythmEffectCueSessionResult Result;
	Result.EventIndex = NextEventIndex;
	Result.CompletedEvents = NextEventIndex;
	Result.TotalEvents = Events.Num();
	Result.Event = Events[NextEventIndex];
	Result.Host = Host.Process(
		Result.Event,
		VisualAttemptId,
		AudioAttemptId,
		VisualExecutor,
		AudioExecutor);
	if (Result.Host.IsComplete())
	{
		++NextEventIndex;
		Result.CompletedEvents = NextEventIndex;
		Result.Status = IsComplete()
			? Edemo_mapShanmenSwordRhythmEffectCueSessionStatus::BatchCompleted
			: Edemo_mapShanmenSwordRhythmEffectCueSessionStatus::EventCompleted;
		Result.Diagnostic = Result.Host.Diagnostic;
	}
	else if (Result.Host.IsSuccess())
	{
		Result.Status =
			Edemo_mapShanmenSwordRhythmEffectCueSessionStatus::RetryPending;
		Result.Diagnostic = Result.Host.Diagnostic;
	}
	else
	{
		Result.Status =
			Edemo_mapShanmenSwordRhythmEffectCueSessionStatus::EventRejected;
		Result.Diagnostic = Result.Host.Diagnostic;
	}
	if (!IsValid())
	{
		*this = Before;
		auto Invalid = Reject(
			Edemo_mapShanmenSwordRhythmEffectCueSessionStatus::StateInvalid,
			TEXT("Cue execution step violated ordered batch progress."));
		Invalid.EventIndex = Result.EventIndex;
		Invalid.CompletedEvents = Before.NextEventIndex;
		Invalid.TotalEvents = Before.Events.Num();
		Invalid.Event = Result.Event;
		return Invalid;
	}
	return Result;
}

bool Fdemo_mapShanmenSwordRhythmEffectCueExecutionSession::
	TryGetCurrentEvent(
		Fdemo_mapShanmenSwordRhythmEffectCueEvent& OutEvent) const
{
	OutEvent = Fdemo_mapShanmenSwordRhythmEffectCueEvent();
	if (!IsValid() || IsComplete())
	{
		return false;
	}
	OutEvent = Events[NextEventIndex];
	return true;
}

bool Fdemo_mapShanmenSwordRhythmEffectCueExecutionSession::TryEnd(
	const FGuid& ExpectedRunId,
	FString& OutDiagnostic)
{
	OutDiagnostic.Reset();
	if (!IsValid())
	{
		OutDiagnostic = TEXT("Cannot end an invalid cue execution session.");
		return false;
	}
	if (!ExpectedRunId.IsValid() || ExpectedRunId != RunId)
	{
		OutDiagnostic = TEXT("Cue execution session end Run identity mismatched.");
		return false;
	}
	if (!IsComplete())
	{
		OutDiagnostic = TEXT("Cannot end cue execution before the batch completes.");
		return false;
	}
	if (!Host.TryEnd(ExpectedRunId, OutDiagnostic))
	{
		return false;
	}
	Reset();
	OutDiagnostic = TEXT("Cue execution session ended and released its batch.");
	return true;
}

void Fdemo_mapShanmenSwordRhythmEffectCueExecutionSession::Reset()
{
	*this = Fdemo_mapShanmenSwordRhythmEffectCueExecutionSession();
}

bool Fdemo_mapShanmenSwordRhythmEffectCueExecutionSession::IsValid() const
{
	FGuid ValidatedRun;
	if (!RunId.IsValid()
		|| !BatchId.IsValid()
		|| !IsOrderedBatch(Events, ValidatedRun)
		|| ValidatedRun != RunId
		|| NextEventIndex < 0
		|| NextEventIndex > Events.Num()
		|| !Host.IsValid()
		|| Host.GetRunId() != RunId)
	{
		return false;
	}
	const auto& VisualScope = Host.GetVisualCoordinator().GetScope();
	const auto& AudioScope = Host.GetAudioCoordinator().GetScope();
	if (BatchId != MakeBatchId(
			RunId,
			VisualScope.GetConsumerId(),
			AudioScope.GetConsumerId(),
			Events))
	{
		return false;
	}
	return IsConsumerProgressValid(
			Host.GetVisualCoordinator(), Events, NextEventIndex)
		&& IsConsumerProgressValid(
			Host.GetAudioCoordinator(), Events, NextEventIndex);
}

bool Fdemo_mapShanmenSwordRhythmEffectCueExecutionSession::IsEmpty() const
{
	return !RunId.IsValid()
		&& !BatchId.IsValid()
		&& Events.IsEmpty()
		&& NextEventIndex == INDEX_NONE
		&& Host.IsEmpty();
}

bool Fdemo_mapShanmenSwordRhythmEffectCueExecutionSession::IsComplete() const
{
	return IsValid() && NextEventIndex == Events.Num();
}
