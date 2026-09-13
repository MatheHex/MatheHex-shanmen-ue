#include "demo_mapShanmenFormationScatterWorldPublicationRunRoute.h"

#include "ShanmenDeterministicId.h"

namespace
{
	using ERunEvent =
		Edemo_mapShanmenFormationScatterWorldPublicationRunEvent;
	using ERouteStatus =
		Edemo_mapShanmenFormationScatterWorldPublicationRunRouteStatus;
	using ECommandStatus =
		Edemo_mapShanmenFormationScatterWorldPublicationCommandStatus;

	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	bool IsRunEvent(const ERunEvent Event)
	{
		return Event == ERunEvent::Publish
			|| Event == ERunEvent::Cancel
			|| Event == ERunEvent::End;
	}

	const TCHAR* EventName(const ERunEvent Event)
	{
		switch (Event)
		{
		case ERunEvent::Publish:
			return TEXT("Publish");
		case ERunEvent::Cancel:
			return TEXT("Cancel");
		case ERunEvent::End:
			return TEXT("End");
		default:
			return TEXT("Invalid");
		}
	}

	ERouteStatus MapSuccessStatus(const ECommandStatus Status)
	{
		switch (Status)
		{
		case ECommandStatus::Applied:
			return ERouteStatus::Applied;
		case ECommandStatus::Recovered:
			return ERouteStatus::Recovered;
		case ECommandStatus::Replayed:
			return ERouteStatus::Replayed;
		default:
			return ERouteStatus::HostRejected;
		}
	}
}

bool Fdemo_mapShanmenFormationScatterWorldPublicationRunRouteResult::
IsSuccess() const
{
	return (Status == ERouteStatus::Applied
			|| Status == ERouteStatus::Recovered
			|| Status == ERouteStatus::Replayed)
		&& RouteId.IsValid() && RunId.IsValid() && CommandId.IsValid()
		&& IsRunEvent(Event) && Command.IsSuccess()
		&& Command.CommandId == CommandId;
}

bool Fdemo_mapShanmenFormationScatterWorldPublicationRunRouteResult::IsValid()
	const
{
	if (Status == ERouteStatus::Invalid || Diagnostic.IsEmpty())
	{
		return false;
	}
	if (IsSuccess())
	{
		return true;
	}
	if (Status == ERouteStatus::HostRejected)
	{
		return RouteId.IsValid() && RunId.IsValid() && CommandId.IsValid()
			&& IsRunEvent(Event) && Command.IsValid() && !Command.IsSuccess()
			&& Command.CommandId == CommandId;
	}
	return !Command.IsSuccess();
}

FGuid Fdemo_mapShanmenFormationScatterWorldPublicationRunRoute::BuildRouteId(
	const FGuid& InRunId,
	const FGuid& HostId)
{
	if (!InRunId.IsValid() || !HostId.IsValid())
	{
		return FGuid();
	}
	return FShanmenDeterministicId::FromCanonicalParts(
		TEXT("demo_map.Formation.ScatterWorldPublicationRunRoute.r1"),
		{ GuidDigits(InRunId), GuidDigits(HostId) });
}

FGuid Fdemo_mapShanmenFormationScatterWorldPublicationRunRoute::
BuildCommandId(
	const FGuid& InRouteId,
	const ERunEvent Event)
{
	if (!InRouteId.IsValid() || !IsRunEvent(Event))
	{
		return FGuid();
	}
	return FShanmenDeterministicId::FromCanonicalParts(
		TEXT("demo_map.Formation.ScatterWorldPublicationRunCommand.r1"),
		{ GuidDigits(InRouteId), EventName(Event) });
}

bool Fdemo_mapShanmenFormationScatterWorldPublicationRunRoute::
TryCaptureCommand(
	const FGuid& CommandId,
	const ERunEvent Event,
	const Fdemo_mapShanmenFormationScatterWorldPublicationCommandBinding&
		Binding,
	Fdemo_mapShanmenFormationScatterWorldPublicationCommand& OutCommand)
{
	switch (Event)
	{
	case ERunEvent::Publish:
		return Fdemo_mapShanmenFormationScatterWorldPublicationCommand::
			TryCapturePublish(CommandId, Binding, OutCommand);
	case ERunEvent::Cancel:
		return Fdemo_mapShanmenFormationScatterWorldPublicationCommand::
			TryCaptureCancel(CommandId, Binding, OutCommand);
	case ERunEvent::End:
		return Fdemo_mapShanmenFormationScatterWorldPublicationCommand::
			TryCaptureEnd(CommandId, Binding, OutCommand);
	default:
		OutCommand =
			Fdemo_mapShanmenFormationScatterWorldPublicationCommand();
		return false;
	}
}

bool Fdemo_mapShanmenFormationScatterWorldPublicationRunRoute::TryOpen(
	const FGuid& InRunId,
	const Fdemo_mapShanmenFormationScatterWorldPlacementHandoffEvidence&
		HandoffEvidence,
	const TSubclassOf<AActor> ActorClass,
	Fdemo_mapShanmenFormationScatterWorldPublicationRunRoute& OutRoute)
{
	OutRoute.Clear();
	if (!InRunId.IsValid() || !HandoffEvidence.IsValid()
		|| HandoffEvidence.GetDeploymentEvidence()
			.GetResourceEvidence().GetPlan().GetActiveRunId() != InRunId)
	{
		return false;
	}

	Fdemo_mapShanmenFormationScatterWorldPublicationRunRoute Candidate;
	if (!Fdemo_mapShanmenFormationScatterWorldPublicationCommandHost::TryOpen(
			HandoffEvidence, ActorClass, Candidate.Host))
	{
		return false;
	}
	Candidate.RunId = InRunId;
	Candidate.RouteId = BuildRouteId(
		InRunId, Candidate.Host.GetBinding().GetHostId());
	if (!Candidate.IsValid())
	{
		return false;
	}
	OutRoute = MoveTemp(Candidate);
	return true;
}

bool Fdemo_mapShanmenFormationScatterWorldPublicationRunRoute::TryTakeover(
	Fdemo_mapShanmenFormationScatterWorldPublicationRunRoute& Previous,
	Fdemo_mapShanmenFormationScatterWorldPublicationRunRoute& OutRoute)
{
	if (&Previous == &OutRoute || !Previous.IsValid() || !OutRoute.IsEmpty())
	{
		return false;
	}

	Fdemo_mapShanmenFormationScatterWorldPublicationRunRoute Candidate;
	Candidate.RouteId = Previous.RouteId;
	Candidate.RunId = Previous.RunId;
	if (!Fdemo_mapShanmenFormationScatterWorldPublicationCommandHost::
		TryTakeover(Previous.Host, Candidate.Host)
		|| !Candidate.IsValid())
	{
		return false;
	}
	Previous.Clear();
	OutRoute = MoveTemp(Candidate);
	return Previous.IsEmpty() && OutRoute.IsValid();
}

Fdemo_mapShanmenFormationScatterWorldPublicationRunRouteResult
Fdemo_mapShanmenFormationScatterWorldPublicationRunRoute::TryRoute(
	const FGuid& ExpectedRunId,
	UWorld* World,
	const ERunEvent Event)
{
	Fdemo_mapShanmenFormationScatterWorldPublicationRunRouteResult Result;
	Result.RouteId = RouteId;
	Result.RunId = RunId;
	Result.Event = Event;
	if (!IsValid())
	{
		Result.Status = ERouteStatus::RouteInvalid;
		Result.Diagnostic = TEXT("Publication Run event requires one valid route.");
		return Result;
	}
	if (!ExpectedRunId.IsValid() || ExpectedRunId != RunId)
	{
		Result.Status = ERouteStatus::RunMismatch;
		Result.Diagnostic =
			TEXT("Publication Run route rejected a foreign Run identity.");
		return Result;
	}
	if (!IsRunEvent(Event))
	{
		Result.Status = ERouteStatus::EventInvalid;
		Result.Diagnostic =
			TEXT("Publication Run route requires Publish, Cancel or End.");
		return Result;
	}

	Result.CommandId = BuildCommandId(RouteId, Event);
	Fdemo_mapShanmenFormationScatterWorldPublicationCommand Command;
	if (!TryCaptureCommand(
			Result.CommandId, Event, Host.GetBinding(), Command))
	{
		Result.Status = ERouteStatus::CommandCaptureRejected;
		Result.Diagnostic =
			TEXT("Publication Run route could not capture its deterministic command.");
		return Result;
	}

	Result.Command = Host.TrySubmit(World, Command);
	Result.Status = Result.Command.IsSuccess()
		? MapSuccessStatus(Result.Command.Status)
		: ERouteStatus::HostRejected;
	Result.Diagnostic = Result.Command.Diagnostic.IsEmpty()
		? TEXT("Publication command Host returned no diagnostic.")
		: Result.Command.Diagnostic;
	if (!Result.IsValid() || !IsValid())
	{
		Result.Status = ERouteStatus::StateInvalid;
		Result.Diagnostic =
			TEXT("Publication Run route produced inconsistent nested evidence.");
	}
	return Result;
}

Fdemo_mapShanmenFormationScatterWorldPublicationRunRouteResult
Fdemo_mapShanmenFormationScatterWorldPublicationRunRoute::TryPublish(
	const FGuid& ExpectedRunId,
	UWorld* World)
{
	return TryRoute(ExpectedRunId, World, ERunEvent::Publish);
}

Fdemo_mapShanmenFormationScatterWorldPublicationRunRouteResult
Fdemo_mapShanmenFormationScatterWorldPublicationRunRoute::TryCancel(
	const FGuid& ExpectedRunId,
	UWorld* World)
{
	return TryRoute(ExpectedRunId, World, ERunEvent::Cancel);
}

Fdemo_mapShanmenFormationScatterWorldPublicationRunRouteResult
Fdemo_mapShanmenFormationScatterWorldPublicationRunRoute::TryEnd(
	const FGuid& ExpectedRunId,
	UWorld* World)
{
	return TryRoute(ExpectedRunId, World, ERunEvent::End);
}

bool Fdemo_mapShanmenFormationScatterWorldPublicationRunRoute::IsValid() const
{
	if (!RouteId.IsValid() || !RunId.IsValid() || !Host.IsValid()
		|| RouteId != BuildRouteId(
			RunId, Host.GetBinding().GetHostId()))
	{
		return false;
	}
	const auto& Plan = Host.GetSession().GetHandoffEvidence()
		.GetDeploymentEvidence().GetResourceEvidence().GetPlan();
	return Plan.IsValid() && Plan.GetActiveRunId() == RunId;
}

bool Fdemo_mapShanmenFormationScatterWorldPublicationRunRoute::IsEmpty() const
{
	return !RouteId.IsValid() && !RunId.IsValid()
		&& !Host.GetBinding().IsValid()
		&& !Host.GetSession().IsValid()
		&& Host.GetRecordCount() == 0;
}

void Fdemo_mapShanmenFormationScatterWorldPublicationRunRoute::Clear()
{
	RouteId.Invalidate();
	RunId.Invalidate();
	Host = Fdemo_mapShanmenFormationScatterWorldPublicationCommandHost();
}
