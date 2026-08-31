#include "demo_mapShanmenSwordRhythmEffectCueExecutionHost.h"

namespace
{
	Fdemo_mapShanmenSwordRhythmEffectCueHostResult Reject(
		const Edemo_mapShanmenSwordRhythmEffectCueHostStatus Status,
		FString Diagnostic)
	{
		Fdemo_mapShanmenSwordRhythmEffectCueHostResult Result;
		Result.Status = Status;
		Result.Diagnostic = MoveTemp(Diagnostic);
		return Result;
	}

	bool TryMakeCoordinator(
		const FGuid& RunId,
		const FGuid& ConsumerId,
		const FName ConsumerRoleId,
		Fdemo_mapShanmenSwordRhythmEffectCueConsumerCoordinator& OutCoordinator)
	{
		Fdemo_mapShanmenSwordRhythmEffectCueConsumerScope Scope;
		return Fdemo_mapShanmenSwordRhythmEffectCueConsumerScope::TryCreate(
				RunId, ConsumerId, ConsumerRoleId, Scope)
			&& Fdemo_mapShanmenSwordRhythmEffectCueConsumerCoordinator::TryCreate(
				Scope, OutCoordinator);
	}

	FString AggregateDiagnostic(
		const Fdemo_mapShanmenSwordRhythmEffectCueDriverResult& Visual,
		const Fdemo_mapShanmenSwordRhythmEffectCueDriverResult& Audio)
	{
		return FString::Printf(
			TEXT("Visual: %s Audio: %s"),
			Visual.Diagnostic.IsEmpty() ? TEXT("no diagnostic.") : *Visual.Diagnostic,
			Audio.Diagnostic.IsEmpty() ? TEXT("no diagnostic.") : *Audio.Diagnostic);
	}
}

bool Fdemo_mapShanmenSwordRhythmEffectCueHostResult::IsSuccess() const
{
	if (!Visual.IsSuccess() || !Audio.IsSuccess())
	{
		return false;
	}
	const int32 Acknowledged = NumAcknowledgedConsumers();
	switch (Status)
	{
	case Edemo_mapShanmenSwordRhythmEffectCueHostStatus::Completed:
		return Acknowledged == 2;
	case Edemo_mapShanmenSwordRhythmEffectCueHostStatus::RetryPending:
		return Acknowledged < 2;
	default:
		return false;
	}
}

bool Fdemo_mapShanmenSwordRhythmEffectCueHostResult::IsComplete() const
{
	return Status == Edemo_mapShanmenSwordRhythmEffectCueHostStatus::Completed
		&& IsSuccess();
}

int32 Fdemo_mapShanmenSwordRhythmEffectCueHostResult::
	NumAcknowledgedConsumers() const
{
	return (Visual.IsAcknowledged() ? 1 : 0)
		+ (Audio.IsAcknowledged() ? 1 : 0);
}

bool Fdemo_mapShanmenSwordRhythmEffectCueExecutionHost::TryCreate(
	const FGuid& RunId,
	const FGuid& VisualConsumerId,
	const FGuid& AudioConsumerId,
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionHost& OutHost)
{
	OutHost = Fdemo_mapShanmenSwordRhythmEffectCueExecutionHost();
	if (!RunId.IsValid() || !VisualConsumerId.IsValid()
		|| !AudioConsumerId.IsValid() || VisualConsumerId == AudioConsumerId)
	{
		return false;
	}
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionHost Candidate;
	if (!TryMakeCoordinator(
			RunId,
			VisualConsumerId,
			Fdemo_mapShanmenSwordRhythmEffectCueConsumerRoute::
				VisualConsumerRoleId(),
			Candidate.VisualCoordinator)
		|| !TryMakeCoordinator(
			RunId,
			AudioConsumerId,
			Fdemo_mapShanmenSwordRhythmEffectCueConsumerRoute::
				AudioConsumerRoleId(),
			Candidate.AudioCoordinator)
		|| !Candidate.IsValid())
	{
		return false;
	}
	OutHost = MoveTemp(Candidate);
	return true;
}

Fdemo_mapShanmenSwordRhythmEffectCueHostResult
Fdemo_mapShanmenSwordRhythmEffectCueExecutionHost::Process(
	const Fdemo_mapShanmenSwordRhythmEffectCueEvent& Event,
	const FGuid& VisualAttemptId,
	const FGuid& AudioAttemptId,
	Idemo_mapShanmenSwordRhythmEffectCueExecutor& VisualExecutor,
	Idemo_mapShanmenSwordRhythmEffectCueExecutor& AudioExecutor)
{
	if (!IsValid())
	{
		return Reject(
			Edemo_mapShanmenSwordRhythmEffectCueHostStatus::HostInvalid,
			TEXT("Cue execution host requires one valid Run-local pair."));
	}
	if (!Event.IsValid())
	{
		return Reject(
			Edemo_mapShanmenSwordRhythmEffectCueHostStatus::EventInvalid,
			TEXT("Cue execution host requires one immutable valid event."));
	}
	if (!VisualAttemptId.IsValid() || !AudioAttemptId.IsValid())
	{
		return Reject(
			Edemo_mapShanmenSwordRhythmEffectCueHostStatus::AttemptInvalid,
			TEXT("Both consumer attempts require caller-owned identities."));
	}

	const auto Before = *this;
	Fdemo_mapShanmenSwordRhythmEffectCueHostResult Result;
	Result.Visual = Fdemo_mapShanmenSwordRhythmEffectCueExecutionDriver::Process(
		VisualCoordinator, Event, VisualAttemptId, VisualExecutor);
	Result.Audio = Fdemo_mapShanmenSwordRhythmEffectCueExecutionDriver::Process(
		AudioCoordinator, Event, AudioAttemptId, AudioExecutor);
	if (!IsValid())
	{
		*this = Before;
		return Reject(
			Edemo_mapShanmenSwordRhythmEffectCueHostStatus::StateInvalid,
			TEXT("Consumer processing violated the Run-local host invariant."));
	}

	Result.Diagnostic = AggregateDiagnostic(Result.Visual, Result.Audio);
	if (!Result.Visual.IsSuccess() || !Result.Audio.IsSuccess())
	{
		Result.Status =
			Edemo_mapShanmenSwordRhythmEffectCueHostStatus::ConsumerRejected;
		return Result;
	}
	Result.Status = Result.NumAcknowledgedConsumers() == 2
		? Edemo_mapShanmenSwordRhythmEffectCueHostStatus::Completed
		: Edemo_mapShanmenSwordRhythmEffectCueHostStatus::RetryPending;
	return Result;
}

bool Fdemo_mapShanmenSwordRhythmEffectCueExecutionHost::TryEnd(
	const FGuid& ExpectedRunId,
	FString& OutDiagnostic)
{
	OutDiagnostic.Reset();
	if (!IsValid())
	{
		OutDiagnostic = TEXT("Cannot end an invalid cue execution host.");
		return false;
	}
	if (!ExpectedRunId.IsValid() || ExpectedRunId != GetRunId())
	{
		OutDiagnostic = TEXT("Cue execution host end Run identity mismatched.");
		return false;
	}
	Reset();
	OutDiagnostic = TEXT("Cue execution host ended and released consumer state.");
	return true;
}

void Fdemo_mapShanmenSwordRhythmEffectCueExecutionHost::Reset()
{
	*this = Fdemo_mapShanmenSwordRhythmEffectCueExecutionHost();
}

bool Fdemo_mapShanmenSwordRhythmEffectCueExecutionHost::IsValid() const
{
	if (!VisualCoordinator.IsValid() || !AudioCoordinator.IsValid())
	{
		return false;
	}
	const auto& VisualScope = VisualCoordinator.GetScope();
	const auto& AudioScope = AudioCoordinator.GetScope();
	return VisualScope.GetRunId().IsValid()
		&& VisualScope.GetRunId() == AudioScope.GetRunId()
		&& VisualScope.GetConsumerId().IsValid()
		&& AudioScope.GetConsumerId().IsValid()
		&& VisualScope.GetConsumerId() != AudioScope.GetConsumerId()
		&& VisualScope.GetConsumerRoleId()
			== Fdemo_mapShanmenSwordRhythmEffectCueConsumerRoute::
				VisualConsumerRoleId()
		&& AudioScope.GetConsumerRoleId()
			== Fdemo_mapShanmenSwordRhythmEffectCueConsumerRoute::
				AudioConsumerRoleId()
		&& VisualScope.GetScopeId() != AudioScope.GetScopeId();
}

bool Fdemo_mapShanmenSwordRhythmEffectCueExecutionHost::IsEmpty() const
{
	return !VisualCoordinator.IsValid() && !AudioCoordinator.IsValid();
}
