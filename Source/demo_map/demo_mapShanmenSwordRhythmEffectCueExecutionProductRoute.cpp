#include "demo_mapShanmenSwordRhythmEffectCueExecutionProductRoute.h"

namespace
{
	using FProjection =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductProjection;
	using FCreateRequest =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductCreateRequest;

	bool EventsMatchProjections(
		const TArray<Fdemo_mapShanmenSwordRhythmEffectCueEvent>& Events,
		const TArray<FProjection>& Projections)
	{
		if (Events.Num() != Projections.Num())
		{
			return false;
		}
		for (int32 Index = 0; Index < Events.Num(); ++Index)
		{
			if (!Events[Index].Matches(Projections[Index].GetEvent()))
			{
				return false;
			}
		}
		return true;
	}

	bool ProjectionsMatch(
		const TArray<FProjection>& Left,
		const TArray<FProjection>& Right)
	{
		if (Left.Num() != Right.Num())
		{
			return false;
		}
		for (int32 Index = 0; Index < Left.Num(); ++Index)
		{
			if (!Left[Index].Matches(Right[Index]))
			{
				return false;
			}
		}
		return true;
	}

	bool ValidateProjectionBatch(
		const TArray<FProjection>& Projections,
		FGuid& OutRunId,
		FGuid& OutConfigId,
		FGuid& OutCuePolicyId,
		bool& bOutIdentityMismatch,
		bool& bOutOrderInvalid)
	{
		OutRunId.Invalidate();
		OutConfigId.Invalidate();
		OutCuePolicyId.Invalidate();
		bOutIdentityMismatch = false;
		bOutOrderInvalid = false;
		if (Projections.IsEmpty() || !Projections[0].IsValid())
		{
			return false;
		}

		OutRunId = Projections[0].GetRunId();
		OutConfigId = Projections[0].GetConfigId();
		OutCuePolicyId = Projections[0].GetCuePolicyId();
		int32 PreviousRevision = 0;
		for (const FProjection& Projection : Projections)
		{
			if (!Projection.IsValid())
			{
				return false;
			}
			if (Projection.GetRunId() != OutRunId
				|| Projection.GetConfigId() != OutConfigId
				|| Projection.GetCuePolicyId() != OutCuePolicyId)
			{
				bOutIdentityMismatch = true;
				return false;
			}
			if (Projection.GetObservationRevision() <= PreviousRevision)
			{
				bOutOrderInvalid = true;
				return false;
			}
			PreviousRevision = Projection.GetObservationRevision();
		}
		return true;
	}
}

bool Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductProjection::
IsValid() const
{
	return RunId.IsValid()
		&& ConfigId.IsValid()
		&& CuePolicyId.IsValid()
		&& Event.IsValid()
		&& Event.GetState().GetRunId() == RunId
		&& Event.GetState().GetConfigId() == ConfigId
		&& Event.GetState().GetObservationRevision() > 0
		&& Event.GetPolicy().GetPolicyId() == CuePolicyId;
}

bool Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductProjection::
Matches(
	const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductProjection& Other)
	const
{
	return IsValid()
		&& Other.IsValid()
		&& RunId == Other.RunId
		&& ConfigId == Other.ConfigId
		&& CuePolicyId == Other.CuePolicyId
		&& Event.Matches(Other.Event);
}

bool Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductProjection::
MatchesCurrentSession(
	const Fdemo_mapShanmenSwordRhythmProductSession& Session) const
{
	if (!IsValid()
		|| !Session.IsValid()
		|| Session.IsEmpty()
		|| Session.GetRunId() != RunId
		|| Session.GetConfig().GetConfigId() != ConfigId
		|| Session.GetConfig().GetEffectCuePolicy().GetPolicyId()
			!= CuePolicyId
		|| !Session.GetPresentationState().Matches(Event.GetState()))
	{
		return false;
	}
	const auto Adapted = Fdemo_mapShanmenSwordRhythmEffectCueAdapter::Adapt(
		Session.GetPresentationState(),
		Session.GetConfig().GetEffectCuePolicy());
	return Adapted.IsAdapted() && Event.Matches(Adapted.Event);
}

Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductProjectionResult
Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductProjector::CaptureCurrent(
	const Fdemo_mapShanmenSwordRhythmProductSession& Session)
{
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductProjectionResult Result;
	if (!Session.IsValid() || Session.IsEmpty())
	{
		Result.Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionProductProjectionStatus::
				SessionInvalid;
		Result.Diagnostic = TEXT(
			"Product projection requires one valid, observed ProductSession.");
		return Result;
	}

	const auto& Config = Session.GetConfig();
	const auto& State = Session.GetPresentationState();
	const auto& Policy = Config.GetEffectCuePolicy();
	if (!State.IsValid() || !Policy.IsValid())
	{
		Result.Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionProductProjectionStatus::
				PresentationUnavailable;
		Result.Diagnostic = TEXT(
			"ProductSession has no valid presentation/cue-policy pair.");
		return Result;
	}
	if (State.GetRunId() != Session.GetRunId()
		|| State.GetConfigId() != Config.GetConfigId())
	{
		Result.Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionProductProjectionStatus::
				IdentityMismatch;
		Result.Diagnostic = TEXT(
			"ProductSession presentation identity does not match its Run/config.");
		return Result;
	}

	const auto Adapted =
		Fdemo_mapShanmenSwordRhythmEffectCueAdapter::Adapt(State, Policy);
	if (!Adapted.IsAdapted())
	{
		Result.Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionProductProjectionStatus::
				AdapterRejected;
		Result.Diagnostic = Adapted.Diagnostic;
		return Result;
	}

	Result.Projection.RunId = Session.GetRunId();
	Result.Projection.ConfigId = Config.GetConfigId();
	Result.Projection.CuePolicyId = Policy.GetPolicyId();
	Result.Projection.Event = Adapted.Event;
	if (!Result.Projection.IsValid()
		|| !Result.Projection.MatchesCurrentSession(Session))
	{
		Result.Projection =
			Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductProjection();
		Result.Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionProductProjectionStatus::
				ProjectionRejected;
		Result.Diagnostic = TEXT(
			"Captured Product projection failed its identity proof.");
		return Result;
	}

	Result.Status =
		Edemo_mapShanmenSwordRhythmEffectCueExecutionProductProjectionStatus::
			Captured;
	Result.Diagnostic = TEXT(
		"Captured the current immutable ProductSession EffectCue projection.");
	return Result;
}

bool Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductCreateRequest::
IsValid() const
{
	FGuid ExpectedRunId;
	FGuid ExpectedConfigId;
	FGuid ExpectedCuePolicyId;
	bool bIdentityMismatch = false;
	bool bOrderInvalid = false;
	if (!ValidateProjectionBatch(
			Projections,
			ExpectedRunId,
			ExpectedConfigId,
			ExpectedCuePolicyId,
			bIdentityMismatch,
			bOrderInvalid)
		|| ExpectedRunId != RunId
		|| ExpectedConfigId != ConfigId
		|| ExpectedCuePolicyId != CuePolicyId
		|| !Envelope.IsValid()
		|| Envelope.GetSequence() != 0)
	{
		return false;
	}

	const auto& Command = Envelope.GetCommand();
	if (Command.GetKind()
			!= Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandKind::Create
		|| Command.GetRunId() != RunId
		|| !EventsMatchProjections(Command.GetEvents(), Projections))
	{
		return false;
	}

	TArray<Fdemo_mapShanmenSwordRhythmEffectCueEvent> Events;
	Events.Reserve(Projections.Num());
	for (const FProjection& Projection : Projections)
	{
		Events.Add(Projection.GetEvent());
	}
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand ExpectedCommand;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope
		ExpectedEnvelope;
	return Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand::
			TryCaptureCreate(
				Command.GetCommandId(),
				Events,
				Command.GetVisualConsumerId(),
				Command.GetAudioConsumerId(),
				ExpectedCommand)
		&& ExpectedCommand.Matches(Command)
		&& Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope::
			TryCapture(
				Envelope.GetHostId(),
				Envelope.GetSequence(),
				ExpectedCommand,
				ExpectedEnvelope)
		&& ExpectedEnvelope.Matches(Envelope);
}

bool Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductCreateRequest::
Matches(
	const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductCreateRequest&
		Other) const
{
	return IsValid()
		&& Other.IsValid()
		&& RunId == Other.RunId
		&& ConfigId == Other.ConfigId
		&& CuePolicyId == Other.CuePolicyId
		&& ProjectionsMatch(Projections, Other.Projections)
		&& Envelope.Matches(Other.Envelope);
}

Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductCreateCaptureResult
Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductCreateFactory::Capture(
	const FGuid& HostId,
	const int64 Sequence,
	const FGuid& CommandId,
	const TArray<FProjection>& Projections,
	const FGuid& VisualConsumerId,
	const FGuid& AudioConsumerId)
{
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductCreateCaptureResult
		Result;
	if (!HostId.IsValid()
		|| Sequence != 0
		|| !CommandId.IsValid()
		|| !VisualConsumerId.IsValid()
		|| !AudioConsumerId.IsValid()
		|| VisualConsumerId == AudioConsumerId)
	{
		Result.Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionProductCreateCaptureStatus::
				TransportIdentityInvalid;
		Result.Diagnostic = TEXT(
			"Product Create requires valid distinct identities and explicit sequence zero.");
		return Result;
	}

	FGuid RunId;
	FGuid ConfigId;
	FGuid CuePolicyId;
	bool bIdentityMismatch = false;
	bool bOrderInvalid = false;
	if (!ValidateProjectionBatch(
			Projections,
			RunId,
			ConfigId,
			CuePolicyId,
			bIdentityMismatch,
			bOrderInvalid))
	{
		if (bIdentityMismatch)
		{
			Result.Status =
				Edemo_mapShanmenSwordRhythmEffectCueExecutionProductCreateCaptureStatus::
					ProjectionIdentityMismatch;
			Result.Diagnostic = TEXT(
				"Product projections do not share Run/config/cue-policy identity.");
		}
		else if (bOrderInvalid)
		{
			Result.Status =
				Edemo_mapShanmenSwordRhythmEffectCueExecutionProductCreateCaptureStatus::
					ProjectionOrderInvalid;
			Result.Diagnostic = TEXT(
				"Product projections are not strictly revision ordered.");
		}
		else
		{
			Result.Status =
				Edemo_mapShanmenSwordRhythmEffectCueExecutionProductCreateCaptureStatus::
					ProjectionInvalid;
			Result.Diagnostic = TEXT(
				"Product Create requires a non-empty valid projection batch.");
		}
		return Result;
	}

	TArray<Fdemo_mapShanmenSwordRhythmEffectCueEvent> Events;
	Events.Reserve(Projections.Num());
	for (const FProjection& Projection : Projections)
	{
		Events.Add(Projection.GetEvent());
	}
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand Command;
	if (!Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand::
			TryCaptureCreate(
				CommandId,
				Events,
				VisualConsumerId,
				AudioConsumerId,
				Command))
	{
		Result.Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionProductCreateCaptureStatus::
				CommandRejected;
		Result.Diagnostic = TEXT(
			"P12.18 rejected the Product projection batch Create command.");
		return Result;
	}

	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope Envelope;
	if (!Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope::
			TryCapture(HostId, Sequence, Command, Envelope))
	{
		Result.Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionProductCreateCaptureStatus::
				EnvelopeRejected;
		Result.Diagnostic = TEXT(
			"P12.19 rejected the Product Create transport envelope.");
		return Result;
	}

	Result.Request.RunId = RunId;
	Result.Request.ConfigId = ConfigId;
	Result.Request.CuePolicyId = CuePolicyId;
	Result.Request.Projections = Projections;
	Result.Request.Envelope = Envelope;
	if (!Result.Request.IsValid())
	{
		Result.Request = FCreateRequest();
		Result.Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionProductCreateCaptureStatus::
				RequestRejected;
		Result.Diagnostic = TEXT(
			"Captured Product Create request failed self-validation.");
		return Result;
	}

	Result.Status =
		Edemo_mapShanmenSwordRhythmEffectCueExecutionProductCreateCaptureStatus::
			Captured;
	Result.Diagnostic = TEXT(
		"Captured one ordered Product projection batch Create request.");
	return Result;
}

bool Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRouteResult::
IsSuccess() const
{
	const bool bStatusMatchesReplay = bReplay
		? Status
			== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductRouteStatus::
				Replayed
		: Status
			== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductRouteStatus::
				Created;
	return bStatusMatchesReplay
		&& RunId.IsValid()
		&& ConfigId.IsValid()
		&& CuePolicyId.IsValid()
		&& HostId.IsValid()
		&& DispatchId.IsValid()
		&& EventCount > 0
		&& Host.IsSuccess()
		&& Host.IsReplay() == bReplay
		&& Host.RunId == RunId
		&& Host.HostId == HostId
		&& Host.DispatchId == DispatchId;
}

Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRouteResult
Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRoute::TryCreate(
	const FCreateRequest& Request,
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost& Host)
{
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRouteResult Result;
	if (!Request.IsValid())
	{
		Result.Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionProductRouteStatus::
				RequestInvalid;
		Result.Diagnostic = TEXT(
			"Product route rejected an invalid frozen Create request.");
		return Result;
	}

	Result.RunId = Request.GetRunId();
	Result.ConfigId = Request.GetConfigId();
	Result.CuePolicyId = Request.GetCuePolicyId();
	Result.HostId = Request.GetEnvelope().GetHostId();
	Result.DispatchId = Request.GetEnvelope().GetDispatchId();
	Result.EventCount = Request.GetProjections().Num();
	Result.Host = Host.TryRoute(Request.GetEnvelope());
	Result.bReplay = Result.Host.IsReplay();
	if (!Result.Host.IsSuccess())
	{
		Result.Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionProductRouteStatus::
				HostRejected;
		Result.Diagnostic = Result.Host.Diagnostic;
		return Result;
	}
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHostRecord
		CreateRecord;
	if (!Host.IsValid()
		|| Host.GetHostId() != Result.HostId
		|| Host.GetRunId() != Result.RunId
		|| !Host.TryGetRecord(0, CreateRecord)
		|| !CreateRecord.Envelope.Matches(Request.GetEnvelope())
		|| Host.GetRecordCount() < 1
		|| Host.GetNextSequence() < 1
		|| (!Result.bReplay
			&& (Host.GetRecordCount() != 1 || Host.GetNextSequence() != 1)))
	{
		Result.Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionProductRouteStatus::
				StateInvalid;
		Result.Diagnostic = TEXT(
			"P12.19 Host state did not preserve the Product Create identity.");
		return Result;
	}

	Result.Status = Result.bReplay
		? Edemo_mapShanmenSwordRhythmEffectCueExecutionProductRouteStatus::
			Replayed
		: Edemo_mapShanmenSwordRhythmEffectCueExecutionProductRouteStatus::
			Created;
	Result.Diagnostic = Result.bReplay
		? TEXT("Replayed the exact frozen Product Create request.")
		: TEXT("Created one P12.19 Host from frozen Product projections.");
	return Result;
}
