#include "demo_mapShanmenSwordRhythmEffectCuePresentationRunController.h"

#include "ShanmenDeterministicId.h"

namespace
{
	using FController =
		Fdemo_mapShanmenSwordRhythmEffectCuePresentationRunController;
	using FPrepared =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatch;
	using FSeed =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchPlanSeed;
	using EPublishStatus =
		Edemo_mapShanmenSwordRhythmEffectCuePresentationRunPublishStatus;

	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	FGuid DeriveRunRoleId(const FGuid& RunId, const TCHAR* Role)
	{
		if (!RunId.IsValid() || !Role || !*Role)
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.Combat.SwordRhythm.EffectCuePresentationRunController.r1"),
			{Role, GuidDigits(RunId)});
	}

	bool SeedsMatch(const FSeed& Left, const FSeed& Right)
	{
		return Left.IsValid() && Right.IsValid()
			&& Left.DispatchSeed == Right.DispatchSeed
			&& Left.VisualConsumerScopeId == Right.VisualConsumerScopeId
			&& Left.AudioConsumerScopeId == Right.AudioConsumerScopeId;
	}

	bool PreparedMatches(
		const FPrepared& Prepared,
		const FGuid& RunId,
		const FSeed& Seed)
	{
		return Prepared.IsValid()
			&& Prepared.GetProjection().GetRunId() == RunId
			&& SeedsMatch(Prepared.GetPlan().GetSeed(), Seed);
	}

	Fdemo_mapShanmenSwordRhythmEffectCuePresentationRunPublishResult
		MakePublishResult(
			const EPublishStatus Status,
			FString Diagnostic,
			const int32 Revision,
			const FController& Controller)
	{
		Fdemo_mapShanmenSwordRhythmEffectCuePresentationRunPublishResult Result;
		Result.Status = Status;
		Result.Diagnostic = MoveTemp(Diagnostic);
		Result.ObservationRevision = Revision;
		Result.QueuedDispatchCount = Controller.GetQueuedDispatchCount();
		Result.PublishedDispatchCount =
			Controller.GetPublishedDispatchCount();
		return Result;
	}
}

bool Fdemo_mapShanmenSwordRhythmEffectCuePresentationRunPublishResult::
	IsSuccess() const
{
	return Status
			== Edemo_mapShanmenSwordRhythmEffectCuePresentationRunPublishStatus::
				Published
		|| Status
			== Edemo_mapShanmenSwordRhythmEffectCuePresentationRunPublishStatus::
				Queued
		|| Status
			== Edemo_mapShanmenSwordRhythmEffectCuePresentationRunPublishStatus::
				AlreadyCaptured;
}

Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchPlanSeed
Fdemo_mapShanmenSwordRhythmEffectCuePresentationRunController::MakeSeed(
	const FGuid& InRunId)
{
	FSeed Result;
	Result.DispatchSeed =
		DeriveRunRoleId(InRunId, TEXT("DispatchSeed"));
	Result.VisualConsumerScopeId =
		DeriveRunRoleId(InRunId, TEXT("ConsumerScope.Visual"));
	Result.AudioConsumerScopeId =
		DeriveRunRoleId(InRunId, TEXT("ConsumerScope.Audio"));
	return Result;
}

bool Fdemo_mapShanmenSwordRhythmEffectCuePresentationRunController::
	IsEmpty() const
{
	return !RunId.IsValid()
		&& !Seed.DispatchSeed.IsValid()
		&& !Seed.VisualConsumerScopeId.IsValid()
		&& !Seed.AudioConsumerScopeId.IsValid()
		&& !VisualExecutor.IsValid()
		&& !AudioExecutor.IsValid()
		&& VisualExecutor.GetAcceptedInvocationCount() == 0
		&& AudioExecutor.GetAcceptedInvocationCount() == 0
		&& PendingDispatches.IsEmpty()
		&& LastCapturedRevision == 0
		&& LastPublishedRevision == 0
		&& PublishedDispatchCount == 0;
}

bool Fdemo_mapShanmenSwordRhythmEffectCuePresentationRunController::
	IsValid() const
{
	if (IsEmpty())
	{
		return true;
	}
	const FSeed ExpectedSeed = MakeSeed(RunId);
	if (!RunId.IsValid() || !SeedsMatch(Seed, ExpectedSeed)
		|| !VisualExecutor.IsValid() || !AudioExecutor.IsValid()
		|| VisualExecutor.GetRunId() != RunId
		|| AudioExecutor.GetRunId() != RunId
		|| VisualExecutor.GetChannel()
			!= Edemo_mapShanmenSwordRhythmEffectCueChannel::Visual
		|| AudioExecutor.GetChannel()
			!= Edemo_mapShanmenSwordRhythmEffectCueChannel::Audio
		|| LastCapturedRevision < 0 || LastPublishedRevision < 0
		|| LastPublishedRevision > LastCapturedRevision
		|| PublishedDispatchCount != LastPublishedRevision
		|| PendingDispatches.Num()
			!= LastCapturedRevision - LastPublishedRevision)
	{
		return false;
	}

	int32 ExpectedRevision = LastPublishedRevision + 1;
	for (const FPrepared& Prepared : PendingDispatches)
	{
		if (!PreparedMatches(Prepared, RunId, Seed)
			|| Prepared.GetProjection().GetObservationRevision()
				!= ExpectedRevision)
		{
			return false;
		}
		++ExpectedRevision;
	}
	return ExpectedRevision == LastCapturedRevision + 1;
}

bool Fdemo_mapShanmenSwordRhythmEffectCuePresentationRunController::
	TryBegin(const FGuid& InRunId, FString& OutDiagnostic)
{
	OutDiagnostic.Reset();
	if (!IsEmpty())
	{
		OutDiagnostic = TEXT(
			"Presentation Run controller must be empty before Run binding.");
		return false;
	}
	if (!InRunId.IsValid())
	{
		OutDiagnostic = TEXT(
			"Presentation Run controller requires one valid combat Run identity.");
		return false;
	}

	FController Candidate;
	Candidate.RunId = InRunId;
	Candidate.Seed = MakeSeed(InRunId);
	if (!Candidate.Seed.IsValid()
		|| !Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoffExecutor::
			TryCreate(
				InRunId,
				Edemo_mapShanmenSwordRhythmEffectCueChannel::Visual,
				Candidate.VisualExecutor)
		|| !Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoffExecutor::
			TryCreate(
				InRunId,
				Edemo_mapShanmenSwordRhythmEffectCueChannel::Audio,
				Candidate.AudioExecutor)
		|| !Candidate.IsValid())
	{
		OutDiagnostic = TEXT(
			"Presentation Run controller could not bind deterministic channel executors.");
		return false;
	}
	*this = MoveTemp(Candidate);
	OutDiagnostic = TEXT(
		"Bound one caller-owned SwordRhythm presentation Run controller.");
	return true;
}

Fdemo_mapShanmenSwordRhythmEffectCuePresentationRunController::FPumpResult
Fdemo_mapShanmenSwordRhythmEffectCuePresentationRunController::
	PumpPendingDispatches()
{
	FPumpResult Result;
	if (!IsValid() || IsEmpty())
	{
		Result.Status = EPumpStatus::StateInvalid;
		Result.Diagnostic = TEXT(
			"Presentation dispatch pump requires one valid active Run controller.");
		return Result;
	}
	if (PendingDispatches.IsEmpty())
	{
		Result.Status = EPumpStatus::Idle;
		Result.Diagnostic = TEXT("No prepared presentation dispatch is queued.");
		return Result;
	}
	if (VisualExecutor.HasPendingHandoff()
		|| AudioExecutor.HasPendingHandoff())
	{
		Result.Status = EPumpStatus::Blocked;
		Result.Diagnostic = TEXT(
			"Prepared presentation dispatch remains queued behind caller consumption.");
		return Result;
	}

	const int32 BoundedDispatchCount = PendingDispatches.Num();
	for (int32 Attempt = 0;
		Attempt < BoundedDispatchCount && !PendingDispatches.IsEmpty();
		++Attempt)
	{
		if (VisualExecutor.HasPendingHandoff()
			|| AudioExecutor.HasPendingHandoff())
		{
			break;
		}
		const FPrepared& Prepared = PendingDispatches[0];
		const int32 Revision =
			Prepared.GetProjection().GetObservationRevision();
		if (!PreparedMatches(Prepared, RunId, Seed)
			|| Revision != LastPublishedRevision + 1)
		{
			Result.Status = EPumpStatus::StateInvalid;
			Result.Diagnostic = TEXT(
				"Queued presentation dispatch violated Run or revision ordering.");
			return Result;
		}

		Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost Host;
		const auto Dispatched =
			Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchService::
				TryDispatchPrepared(
					Prepared,
					Host,
					VisualExecutor,
					AudioExecutor);
		if (!Dispatched.IsSuccess())
		{
			Result.Status = EPumpStatus::DispatchIncomplete;
			Result.Diagnostic = Dispatched.Diagnostic.IsEmpty()
				? TEXT(
					"Prepared presentation dispatch did not complete; frozen revision remains queued.")
				: Dispatched.Diagnostic;
			return Result;
		}
		if (!Host.IsValid() || !Host.IsTerminal()
			|| Host.GetRunId() != RunId)
		{
			Result.Status = EPumpStatus::StateInvalid;
			Result.Diagnostic = TEXT(
				"Completed presentation dispatch did not produce its terminal Host proof.");
			return Result;
		}

		PendingDispatches.RemoveAt(0);
		LastPublishedRevision = Revision;
		++PublishedDispatchCount;
		++Result.PublishedCount;
		if (!IsValid())
		{
			Result.Status = EPumpStatus::StateInvalid;
			Result.Diagnostic = TEXT(
				"Presentation Run controller failed post-dispatch self-validation.");
			return Result;
		}
	}

	Result.Status = Result.PublishedCount > 0
		? EPumpStatus::Published
		: PendingDispatches.IsEmpty()
			? EPumpStatus::Idle
			: EPumpStatus::Blocked;
	Result.Diagnostic = PendingDispatches.IsEmpty()
		? TEXT("Published every currently unblocked presentation revision.")
		: TEXT(
			"Published presentation revisions until a handoff required caller consumption.");
	return Result;
}

Fdemo_mapShanmenSwordRhythmEffectCuePresentationRunPublishResult
Fdemo_mapShanmenSwordRhythmEffectCuePresentationRunController::
	TryPublishCurrent(
		const Fdemo_mapShanmenSwordRhythmProductSession& Session)
{
	if (!IsValid() || IsEmpty())
	{
		return MakePublishResult(
			EPublishStatus::ControllerInvalid,
			TEXT("Current presentation publication requires an active valid controller."),
			0,
			*this);
	}
	if (!Session.IsValid() || Session.IsEmpty())
	{
		return MakePublishResult(
			EPublishStatus::SessionInvalid,
			TEXT("Current presentation publication requires an observed ProductSession."),
			0,
			*this);
	}
	if (Session.GetRunId() != RunId)
	{
		return MakePublishResult(
			EPublishStatus::RunMismatch,
			TEXT("ProductSession belongs to a different combat Run."),
			0,
			*this);
	}

	const auto Prepared =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchService::
			PrepareCurrent(Session, Seed);
	if (!Prepared.IsPrepared())
	{
		return MakePublishResult(
			EPublishStatus::PrepareRejected,
			Prepared.Diagnostic,
			0,
			*this);
	}
	const int32 Revision =
		Prepared.Prepared.GetProjection().GetObservationRevision();
	if (Revision == LastCapturedRevision)
	{
		return MakePublishResult(
			EPublishStatus::AlreadyCaptured,
			TEXT("Current presentation revision was already captured."),
			Revision,
			*this);
	}
	if (Revision != LastCapturedRevision + 1)
	{
		return MakePublishResult(
			EPublishStatus::RevisionConflict,
			FString::Printf(
				TEXT("Presentation revision must be contiguous. Expected=%d Actual=%d."),
				LastCapturedRevision + 1,
				Revision),
			Revision,
			*this);
	}

	PendingDispatches.Add(Prepared.Prepared);
	LastCapturedRevision = Revision;
	if (!IsValid())
	{
		PendingDispatches.Pop();
		--LastCapturedRevision;
		return MakePublishResult(
			EPublishStatus::StateInvalid,
			TEXT("Captured presentation revision failed FIFO self-validation."),
			Revision,
			*this);
	}

	const FPumpResult Pump = PumpPendingDispatches();
	if (Pump.Status == EPumpStatus::DispatchIncomplete)
	{
		return MakePublishResult(
			EPublishStatus::DispatchIncomplete,
			Pump.Diagnostic,
			Revision,
			*this);
	}
	if (Pump.Status == EPumpStatus::StateInvalid || !IsValid())
	{
		return MakePublishResult(
			EPublishStatus::StateInvalid,
			Pump.Diagnostic,
			Revision,
			*this);
	}
	return MakePublishResult(
		LastPublishedRevision >= Revision
			? EPublishStatus::Published
			: EPublishStatus::Queued,
		LastPublishedRevision >= Revision
			? TEXT(
				"Current presentation revision reached the caller-owned handoff boundary.")
			: TEXT(
				"Current presentation revision was frozen behind pending handoff consumption."),
		Revision,
		*this);
}

bool Fdemo_mapShanmenSwordRhythmEffectCuePresentationRunController::
	TryGetPendingVisualHandoff(
		Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoff& OutHandoff)
	const
{
	OutHandoff =
		Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoff();
	return IsValid() && !IsEmpty()
		&& VisualExecutor.TryGetPendingHandoff(OutHandoff);
}

bool Fdemo_mapShanmenSwordRhythmEffectCuePresentationRunController::
	TryGetPendingAudioHandoff(
		Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoff& OutHandoff)
	const
{
	OutHandoff =
		Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoff();
	return IsValid() && !IsEmpty()
		&& AudioExecutor.TryGetPendingHandoff(OutHandoff);
}

bool Fdemo_mapShanmenSwordRhythmEffectCuePresentationRunController::
	TryConsumeHandoff(
		Fdemo_mapShanmenSwordRhythmEffectCuePresentationHandoffExecutor&
			Executor,
		const FGuid& HandoffId,
		FString& OutDiagnostic)
{
	OutDiagnostic.Reset();
	if (!IsValid() || IsEmpty())
	{
		OutDiagnostic = TEXT(
			"Presentation handoff consumption requires an active valid controller.");
		return false;
	}
	const auto Consumed = Executor.Consume(HandoffId);
	if (!Consumed.IsSuccess())
	{
		OutDiagnostic = Consumed.Diagnostic;
		return false;
	}
	if (Consumed.Status
		== Edemo_mapShanmenSwordRhythmEffectCuePresentationHandoffConsumeStatus::
			Consumed
		&& !VisualExecutor.HasPendingHandoff()
		&& !AudioExecutor.HasPendingHandoff())
	{
		const FPumpResult Pump = PumpPendingDispatches();
		if (Pump.Status == EPumpStatus::DispatchIncomplete
			|| Pump.Status == EPumpStatus::StateInvalid)
		{
			OutDiagnostic = FString::Printf(
				TEXT("%s NextDispatch=%s"),
				*Consumed.Diagnostic,
				*Pump.Diagnostic);
			return false;
		}
		OutDiagnostic = FString::Printf(
			TEXT("%s PublishedNext=%d Queued=%d."),
			*Consumed.Diagnostic,
			Pump.PublishedCount,
			PendingDispatches.Num());
	}
	else
	{
		OutDiagnostic = Consumed.Diagnostic;
	}
	return IsValid();
}

bool Fdemo_mapShanmenSwordRhythmEffectCuePresentationRunController::
	TryConsumeVisualHandoff(
		const FGuid& HandoffId,
		FString& OutDiagnostic)
{
	return TryConsumeHandoff(VisualExecutor, HandoffId, OutDiagnostic);
}

bool Fdemo_mapShanmenSwordRhythmEffectCuePresentationRunController::
	TryConsumeAudioHandoff(
		const FGuid& HandoffId,
		FString& OutDiagnostic)
{
	return TryConsumeHandoff(AudioExecutor, HandoffId, OutDiagnostic);
}

bool Fdemo_mapShanmenSwordRhythmEffectCuePresentationRunController::
	TryEnd(
		const FGuid& ExpectedRunId,
		Fdemo_mapShanmenSwordRhythmEffectCuePresentationRunEndSummary&
			OutSummary,
		FString& OutDiagnostic)
{
	OutSummary =
		Fdemo_mapShanmenSwordRhythmEffectCuePresentationRunEndSummary();
	OutDiagnostic.Reset();
	if (IsEmpty())
	{
		OutDiagnostic = TEXT(
			"Presentation Run controller was already empty.");
		return ExpectedRunId.IsValid();
	}

	OutSummary.RunId = RunId;
	OutSummary.CapturedDispatchCount = LastCapturedRevision;
	OutSummary.PublishedDispatchCount = PublishedDispatchCount;
	OutSummary.QueuedDispatchCount = PendingDispatches.Num();
	OutSummary.VisualAcceptedInvocationCount =
		VisualExecutor.GetAcceptedInvocationCount();
	OutSummary.AudioAcceptedInvocationCount =
		AudioExecutor.GetAcceptedInvocationCount();
	OutSummary.bVisualPending = VisualExecutor.HasPendingHandoff();
	OutSummary.bAudioPending = AudioExecutor.HasPendingHandoff();
	if (!ExpectedRunId.IsValid() || ExpectedRunId != RunId)
	{
		OutDiagnostic = TEXT(
			"Presentation Run teardown identity does not match the active Run.");
		return false;
	}
	if (!IsValid())
	{
		OutDiagnostic = TEXT(
			"Presentation Run controller was internally inconsistent at teardown.");
		return false;
	}

	Reset();
	OutDiagnostic = TEXT(
		"Released presentation-local queue, handoffs, and replay evidence.");
	return true;
}

void Fdemo_mapShanmenSwordRhythmEffectCuePresentationRunController::Reset()
{
	*this = FController();
}
