#include "demo_mapShanmenSwordRhythmEffectCueConsumerAttempt.h"

#include "ShanmenDeterministicId.h"

namespace
{
	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	bool IsSuccessful(
		const Edemo_mapShanmenSwordRhythmEffectCueAttemptOutcome Outcome)
	{
		return Outcome
				== Edemo_mapShanmenSwordRhythmEffectCueAttemptOutcome::Succeeded
			|| Outcome
				== Edemo_mapShanmenSwordRhythmEffectCueAttemptOutcome::
					NoOpSucceeded;
	}

	bool IsKnownOutcome(
		const Edemo_mapShanmenSwordRhythmEffectCueAttemptOutcome Outcome)
	{
		return Outcome
				== Edemo_mapShanmenSwordRhythmEffectCueAttemptOutcome::
					RetryableFailure
			|| IsSuccessful(Outcome);
	}

	bool BuildProjection(
		const Fdemo_mapShanmenSwordRhythmEffectCueDeliveryReceipt& Delivery,
		Edemo_mapShanmenSwordRhythmEffectCueChannel& OutChannel,
		TArray<Fdemo_mapShanmenSwordRhythmEffectCueCommand>& OutCommands)
	{
		OutChannel = Edemo_mapShanmenSwordRhythmEffectCueChannel::Invalid;
		OutCommands.Reset();
		if (!Delivery.IsValid()
			|| !Fdemo_mapShanmenSwordRhythmEffectCueConsumerRoute::
				TryResolveChannel(
					Delivery.GetScope().GetConsumerRoleId(), OutChannel))
		{
			return false;
		}
		for (const auto& Command : Delivery.GetEvent().GetCommands())
		{
			if (!Command.IsValid())
			{
				OutCommands.Reset();
				return false;
			}
			if (Command.GetChannel() == OutChannel)
			{
				OutCommands.Add(Command);
			}
		}
		return true;
	}

	FGuid MakeRouteId(
		const Fdemo_mapShanmenSwordRhythmEffectCueDeliveryReceipt& Delivery,
		const Edemo_mapShanmenSwordRhythmEffectCueChannel Channel,
		const TArray<Fdemo_mapShanmenSwordRhythmEffectCueCommand>& Commands)
	{
		if (!Delivery.IsValid()
			|| (Channel
					!= Edemo_mapShanmenSwordRhythmEffectCueChannel::Visual
				&& Channel
					!= Edemo_mapShanmenSwordRhythmEffectCueChannel::Audio))
		{
			return FGuid();
		}
		TArray<FString> Parts = {
			GuidDigits(Delivery.GetDeliveryId()),
			GuidDigits(Delivery.GetScope().GetScopeId()),
			FString::FromInt(static_cast<uint8>(Channel)),
			FString::FromInt(Commands.Num())
		};
		for (const auto& Command : Commands)
		{
			if (!Command.IsValid() || Command.GetChannel() != Channel)
			{
				return FGuid();
			}
			Parts.Add(GuidDigits(Command.GetCommandId()));
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.Combat.SwordRhythm.EffectCueConsumerRoute.r1"),
			Parts);
	}

	FGuid MakeAttemptCommandId(
		const Fdemo_mapShanmenSwordRhythmEffectCueConsumerRoute& Route,
		const FGuid& AttemptId,
		const FGuid& ExecutorReceiptId,
		const Edemo_mapShanmenSwordRhythmEffectCueAttemptOutcome Outcome)
	{
		if (!Route.IsValid() || !AttemptId.IsValid() || !IsKnownOutcome(Outcome))
		{
			return FGuid();
		}
		const bool bNoOp = Outcome
			== Edemo_mapShanmenSwordRhythmEffectCueAttemptOutcome::NoOpSucceeded;
		if (bNoOp != (Route.NumCommands() == 0)
			|| (bNoOp && ExecutorReceiptId.IsValid())
			|| (!bNoOp && !ExecutorReceiptId.IsValid()))
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.Combat.SwordRhythm.EffectCueAttemptCommand.r1"),
			{
				GuidDigits(Route.GetRouteId()),
				GuidDigits(AttemptId),
				bNoOp ? TEXT("NO_EXECUTOR") : GuidDigits(ExecutorReceiptId),
				FString::FromInt(static_cast<uint8>(Outcome)),
				FString::FromInt(Route.NumCommands())
			});
	}

	FGuid MakeAttemptReceiptId(
		const Fdemo_mapShanmenSwordRhythmEffectCueConsumerRoute& Route,
		const Fdemo_mapShanmenSwordRhythmEffectCueAttemptCommand& Command,
		const bool bHasAcknowledgement,
		const Fdemo_mapShanmenSwordRhythmEffectCueAcknowledgementReceipt&
			Acknowledgement)
	{
		if (!Route.IsValid() || !Command.IsValid()
			|| !Command.GetRoute().Matches(Route)
			|| bHasAcknowledgement != IsSuccessful(Command.GetOutcome())
			|| (bHasAcknowledgement
				&& (!Acknowledgement.IsValid()
					|| !Acknowledgement.GetDelivery().Matches(
						Route.GetDelivery())))
			|| (!bHasAcknowledgement && Acknowledgement.IsValid()))
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.Combat.SwordRhythm.EffectCueAttemptReceipt.r1"),
			{
				GuidDigits(Route.GetRouteId()),
				GuidDigits(Command.GetCommandId()),
				bHasAcknowledgement
					? GuidDigits(Acknowledgement.GetAcknowledgementId())
					: TEXT("NO_ACKNOWLEDGEMENT")
			});
	}

	Fdemo_mapShanmenSwordRhythmEffectCueConsumerPrepareResult PrepareResult(
		const Edemo_mapShanmenSwordRhythmEffectCueConsumerPrepareStatus Status,
		const TCHAR* Diagnostic,
		const Fdemo_mapShanmenSwordRhythmEffectCueConsumerRoute& Route = {})
	{
		Fdemo_mapShanmenSwordRhythmEffectCueConsumerPrepareResult Result;
		Result.Status = Status;
		Result.Diagnostic = Diagnostic;
		Result.Route = Route;
		return Result;
	}

	Fdemo_mapShanmenSwordRhythmEffectCueConsumerSubmitResult SubmitResult(
		const Edemo_mapShanmenSwordRhythmEffectCueConsumerSubmitStatus Status,
		const TCHAR* Diagnostic,
		const Fdemo_mapShanmenSwordRhythmEffectCueAttemptReceipt& Receipt = {})
	{
		Fdemo_mapShanmenSwordRhythmEffectCueConsumerSubmitResult Result;
		Result.Status = Status;
		Result.Diagnostic = Diagnostic;
		Result.Receipt = Receipt;
		return Result;
	}
}

FName Fdemo_mapShanmenSwordRhythmEffectCueConsumerRoute::
	VisualConsumerRoleId()
{
	return FName(TEXT("Presentation.Visual.SwordRhythm"));
}

FName Fdemo_mapShanmenSwordRhythmEffectCueConsumerRoute::AudioConsumerRoleId()
{
	return FName(TEXT("Presentation.Audio.SwordRhythm"));
}

bool Fdemo_mapShanmenSwordRhythmEffectCueConsumerRoute::TryResolveChannel(
	const FName ConsumerRoleId,
	Edemo_mapShanmenSwordRhythmEffectCueChannel& OutChannel)
{
	OutChannel = Edemo_mapShanmenSwordRhythmEffectCueChannel::Invalid;
	if (ConsumerRoleId == VisualConsumerRoleId())
	{
		OutChannel = Edemo_mapShanmenSwordRhythmEffectCueChannel::Visual;
		return true;
	}
	if (ConsumerRoleId == AudioConsumerRoleId())
	{
		OutChannel = Edemo_mapShanmenSwordRhythmEffectCueChannel::Audio;
		return true;
	}
	return false;
}

bool Fdemo_mapShanmenSwordRhythmEffectCueConsumerRoute::TryCreate(
	const Fdemo_mapShanmenSwordRhythmEffectCueDeliveryReceipt& InDelivery,
	Fdemo_mapShanmenSwordRhythmEffectCueConsumerRoute& OutRoute)
{
	OutRoute = Fdemo_mapShanmenSwordRhythmEffectCueConsumerRoute();
	Fdemo_mapShanmenSwordRhythmEffectCueConsumerRoute Candidate;
	Candidate.Delivery = InDelivery;
	if (!BuildProjection(
			Candidate.Delivery, Candidate.Channel, Candidate.Commands))
	{
		return false;
	}
	Candidate.RouteId = MakeRouteId(
		Candidate.Delivery, Candidate.Channel, Candidate.Commands);
	if (!Candidate.IsValid())
	{
		return false;
	}
	OutRoute = MoveTemp(Candidate);
	return true;
}

bool Fdemo_mapShanmenSwordRhythmEffectCueConsumerRoute::IsValid() const
{
	Edemo_mapShanmenSwordRhythmEffectCueChannel ExpectedChannel =
		Edemo_mapShanmenSwordRhythmEffectCueChannel::Invalid;
	TArray<Fdemo_mapShanmenSwordRhythmEffectCueCommand> ExpectedCommands;
	if (!RouteId.IsValid()
		|| !BuildProjection(Delivery, ExpectedChannel, ExpectedCommands)
		|| Channel != ExpectedChannel
		|| Commands.Num() != ExpectedCommands.Num())
	{
		return false;
	}
	for (int32 Index = 0; Index < Commands.Num(); ++Index)
	{
		if (!Commands[Index].IsValid()
			|| Commands[Index].GetCommandId()
				!= ExpectedCommands[Index].GetCommandId())
		{
			return false;
		}
	}
	return RouteId == MakeRouteId(Delivery, Channel, Commands);
}

bool Fdemo_mapShanmenSwordRhythmEffectCueConsumerRoute::Matches(
	const Fdemo_mapShanmenSwordRhythmEffectCueConsumerRoute& Other) const
{
	return IsValid() && Other.IsValid() && RouteId == Other.RouteId;
}

bool Fdemo_mapShanmenSwordRhythmEffectCueAttemptCommand::TryCreate(
	const Fdemo_mapShanmenSwordRhythmEffectCueConsumerRoute& InRoute,
	const FGuid& InAttemptId,
	const FGuid& InExecutorReceiptId,
	const Edemo_mapShanmenSwordRhythmEffectCueAttemptOutcome InOutcome,
	Fdemo_mapShanmenSwordRhythmEffectCueAttemptCommand& OutCommand)
{
	OutCommand = Fdemo_mapShanmenSwordRhythmEffectCueAttemptCommand();
	Fdemo_mapShanmenSwordRhythmEffectCueAttemptCommand Candidate;
	Candidate.Route = InRoute;
	Candidate.AttemptId = InAttemptId;
	Candidate.ExecutorReceiptId = InExecutorReceiptId;
	Candidate.Outcome = InOutcome;
	Candidate.CommandId = MakeAttemptCommandId(
		Candidate.Route,
		Candidate.AttemptId,
		Candidate.ExecutorReceiptId,
		Candidate.Outcome);
	if (!Candidate.IsValid())
	{
		return false;
	}
	OutCommand = MoveTemp(Candidate);
	return true;
}

bool Fdemo_mapShanmenSwordRhythmEffectCueAttemptCommand::IsValid() const
{
	return CommandId.IsValid()
		&& CommandId == MakeAttemptCommandId(
			Route, AttemptId, ExecutorReceiptId, Outcome);
}

bool Fdemo_mapShanmenSwordRhythmEffectCueAttemptCommand::Matches(
	const Fdemo_mapShanmenSwordRhythmEffectCueAttemptCommand& Other) const
{
	return IsValid() && Other.IsValid() && CommandId == Other.CommandId;
}

bool Fdemo_mapShanmenSwordRhythmEffectCueAttemptReceipt::TryCreate(
	const Fdemo_mapShanmenSwordRhythmEffectCueConsumerRoute& InRoute,
	const Fdemo_mapShanmenSwordRhythmEffectCueAttemptCommand& InCommand,
	const Fdemo_mapShanmenSwordRhythmEffectCueAcknowledgementReceipt*
		InAcknowledgement,
	Fdemo_mapShanmenSwordRhythmEffectCueAttemptReceipt& OutReceipt)
{
	OutReceipt = Fdemo_mapShanmenSwordRhythmEffectCueAttemptReceipt();
	Fdemo_mapShanmenSwordRhythmEffectCueAttemptReceipt Candidate;
	Candidate.Route = InRoute;
	Candidate.Command = InCommand;
	Candidate.bHasAcknowledgement = InAcknowledgement != nullptr;
	if (InAcknowledgement)
	{
		Candidate.Acknowledgement = *InAcknowledgement;
	}
	Candidate.ReceiptId = MakeAttemptReceiptId(
		Candidate.Route,
		Candidate.Command,
		Candidate.bHasAcknowledgement,
		Candidate.Acknowledgement);
	if (!Candidate.IsValid())
	{
		return false;
	}
	OutReceipt = MoveTemp(Candidate);
	return true;
}

bool Fdemo_mapShanmenSwordRhythmEffectCueAttemptReceipt::IsValid() const
{
	return ReceiptId.IsValid()
		&& ReceiptId == MakeAttemptReceiptId(
			Route, Command, bHasAcknowledgement, Acknowledgement);
}

bool Fdemo_mapShanmenSwordRhythmEffectCueAttemptReceipt::Matches(
	const Fdemo_mapShanmenSwordRhythmEffectCueAttemptReceipt& Other) const
{
	return IsValid() && Other.IsValid() && ReceiptId == Other.ReceiptId;
}

bool Fdemo_mapShanmenSwordRhythmEffectCueConsumerSubmitResult::IsSuccess()
	const
{
	return (Status
				== Edemo_mapShanmenSwordRhythmEffectCueConsumerSubmitStatus::
					RetryRecorded
			|| Status
				== Edemo_mapShanmenSwordRhythmEffectCueConsumerSubmitStatus::
					RetryReplayed
			|| IsAcknowledged())
		&& Receipt.IsValid();
}

bool Fdemo_mapShanmenSwordRhythmEffectCueConsumerSubmitResult::
	IsAcknowledged() const
{
	return (Status
				== Edemo_mapShanmenSwordRhythmEffectCueConsumerSubmitStatus::
					Acknowledged
			|| Status
				== Edemo_mapShanmenSwordRhythmEffectCueConsumerSubmitStatus::
					AcknowledgementReplayed
			|| Status
				== Edemo_mapShanmenSwordRhythmEffectCueConsumerSubmitStatus::
					NoOpAcknowledged
			|| Status
				== Edemo_mapShanmenSwordRhythmEffectCueConsumerSubmitStatus::
					NoOpReplayed)
		&& Receipt.IsValid() && Receipt.HasAcknowledgement();
}

bool Fdemo_mapShanmenSwordRhythmEffectCueConsumerCoordinator::TryCreate(
	const Fdemo_mapShanmenSwordRhythmEffectCueConsumerScope& Scope,
	Fdemo_mapShanmenSwordRhythmEffectCueConsumerCoordinator& OutCoordinator)
{
	OutCoordinator = Fdemo_mapShanmenSwordRhythmEffectCueConsumerCoordinator();
	Fdemo_mapShanmenSwordRhythmEffectCueConsumerCoordinator Candidate;
	if (!Fdemo_mapShanmenSwordRhythmEffectCueDeliveryCursor::TryCreate(
			Scope, Candidate.Cursor)
		|| !Candidate.IsValid())
	{
		return false;
	}
	OutCoordinator = Candidate;
	return true;
}

bool Fdemo_mapShanmenSwordRhythmEffectCueConsumerCoordinator::IsValid() const
{
	if (!Cursor.IsValid())
	{
		return false;
	}
	if (!bHasRouteRecord)
	{
		return !CurrentRoute.Route.IsValid()
			&& CurrentRoute.Attempts.IsEmpty()
			&& !CurrentRoute.bAcknowledged;
	}
	if (!CurrentRoute.Route.IsValid()
		|| !CurrentRoute.Route.GetDelivery().GetScope().Matches(
			Cursor.GetScope()))
	{
		return false;
	}

	TSet<FGuid> AttemptIds;
	int32 SuccessCount = 0;
	const Fdemo_mapShanmenSwordRhythmEffectCueAttemptReceipt*
		SuccessfulReceipt = nullptr;
	for (const auto& Receipt : CurrentRoute.Attempts)
	{
		if (!Receipt.IsValid()
			|| !Receipt.GetRoute().Matches(CurrentRoute.Route)
			|| AttemptIds.Contains(Receipt.GetCommand().GetAttemptId()))
		{
			return false;
		}
		AttemptIds.Add(Receipt.GetCommand().GetAttemptId());
		if (IsSuccessful(Receipt.GetCommand().GetOutcome()))
		{
			++SuccessCount;
			SuccessfulReceipt = &Receipt;
		}
	}
	if (SuccessCount > 1
		|| CurrentRoute.bAcknowledged != (SuccessCount == 1))
	{
		return false;
	}

	Fdemo_mapShanmenSwordRhythmEffectCueAcknowledgementReceipt Last;
	const bool bCursorAcknowledged = Cursor.TryGetLastAcknowledgement(Last);
	if (CurrentRoute.bAcknowledged)
	{
		return bCursorAcknowledged && SuccessfulReceipt
			&& SuccessfulReceipt->HasAcknowledgement()
			&& SuccessfulReceipt->GetAcknowledgement().Matches(Last)
			&& Last.GetDelivery().Matches(CurrentRoute.Route.GetDelivery());
	}
	if (!bCursorAcknowledged)
	{
		return true;
	}
	return CurrentRoute.Route.GetDelivery().GetEvent().GetState().
		GetObservationRevision()
		> Last.GetDelivery().GetEvent().GetState().GetObservationRevision();
}

Fdemo_mapShanmenSwordRhythmEffectCueConsumerPrepareResult
Fdemo_mapShanmenSwordRhythmEffectCueConsumerCoordinator::Prepare(
	const Fdemo_mapShanmenSwordRhythmEffectCueEvent& Event) const
{
	if (!IsValid())
	{
		return PrepareResult(
			Edemo_mapShanmenSwordRhythmEffectCueConsumerPrepareStatus::
				CoordinatorInvalid,
			TEXT("Effect-cue consumer preparation requires a valid coordinator."));
	}
	Edemo_mapShanmenSwordRhythmEffectCueChannel Channel =
		Edemo_mapShanmenSwordRhythmEffectCueChannel::Invalid;
	if (!Fdemo_mapShanmenSwordRhythmEffectCueConsumerRoute::
		TryResolveChannel(Cursor.GetScope().GetConsumerRoleId(), Channel))
	{
		return PrepareResult(
			Edemo_mapShanmenSwordRhythmEffectCueConsumerPrepareStatus::
				RoleUnsupported,
			TEXT("Consumer role does not own a canonical cue channel."));
	}
	const auto Delivery = Cursor.Prepare(Event);
	switch (Delivery.Status)
	{
	case Edemo_mapShanmenSwordRhythmEffectCuePrepareStatus::
		AlreadyAcknowledged:
		return PrepareResult(
			Edemo_mapShanmenSwordRhythmEffectCueConsumerPrepareStatus::
				AlreadyAcknowledged,
			*Delivery.Diagnostic);
	case Edemo_mapShanmenSwordRhythmEffectCuePrepareStatus::EventInvalid:
		return PrepareResult(
			Edemo_mapShanmenSwordRhythmEffectCueConsumerPrepareStatus::
				EventInvalid,
			*Delivery.Diagnostic);
	case Edemo_mapShanmenSwordRhythmEffectCuePrepareStatus::RunMismatch:
		return PrepareResult(
			Edemo_mapShanmenSwordRhythmEffectCueConsumerPrepareStatus::RunMismatch,
			*Delivery.Diagnostic);
	case Edemo_mapShanmenSwordRhythmEffectCuePrepareStatus::StaleEvent:
		return PrepareResult(
			Edemo_mapShanmenSwordRhythmEffectCueConsumerPrepareStatus::StaleEvent,
			*Delivery.Diagnostic);
	case Edemo_mapShanmenSwordRhythmEffectCuePrepareStatus::DeliveryRejected:
		return PrepareResult(
			Edemo_mapShanmenSwordRhythmEffectCueConsumerPrepareStatus::
				DeliveryRejected,
			*Delivery.Diagnostic);
	case Edemo_mapShanmenSwordRhythmEffectCuePrepareStatus::CursorInvalid:
		return PrepareResult(
			Edemo_mapShanmenSwordRhythmEffectCueConsumerPrepareStatus::
				CoordinatorInvalid,
			*Delivery.Diagnostic);
	case Edemo_mapShanmenSwordRhythmEffectCuePrepareStatus::Prepared:
		break;
	default:
		return PrepareResult(
			Edemo_mapShanmenSwordRhythmEffectCueConsumerPrepareStatus::
				DeliveryRejected,
			TEXT("Delivery cursor returned an unknown preparation status."));
	}

	Fdemo_mapShanmenSwordRhythmEffectCueConsumerRoute Route;
	if (!Fdemo_mapShanmenSwordRhythmEffectCueConsumerRoute::TryCreate(
			Delivery.Delivery, Route))
	{
		return PrepareResult(
			Edemo_mapShanmenSwordRhythmEffectCueConsumerPrepareStatus::
				RouteRejected,
			TEXT("Delivery failed canonical consumer-channel projection."));
	}
	return PrepareResult(
		Edemo_mapShanmenSwordRhythmEffectCueConsumerPrepareStatus::Routed,
		TEXT("Consumer route prepared without executing or acknowledging cues."),
		Route);
}

Fdemo_mapShanmenSwordRhythmEffectCueConsumerSubmitResult
Fdemo_mapShanmenSwordRhythmEffectCueConsumerCoordinator::Submit(
	const Fdemo_mapShanmenSwordRhythmEffectCueConsumerRoute& Route,
	const Fdemo_mapShanmenSwordRhythmEffectCueAttemptCommand& Command)
{
	if (!IsValid())
	{
		return SubmitResult(
			Edemo_mapShanmenSwordRhythmEffectCueConsumerSubmitStatus::
				CoordinatorInvalid,
			TEXT("Effect-cue attempt requires a valid coordinator."));
	}
	if (!Route.IsValid())
	{
		return SubmitResult(
			Edemo_mapShanmenSwordRhythmEffectCueConsumerSubmitStatus::RouteInvalid,
			TEXT("Effect-cue attempt requires one valid consumer route."));
	}
	if (!Command.IsValid())
	{
		return SubmitResult(
			Edemo_mapShanmenSwordRhythmEffectCueConsumerSubmitStatus::CommandInvalid,
			TEXT("Effect-cue attempt requires valid opaque executor evidence."));
	}
	if (!Route.GetDelivery().GetScope().Matches(Cursor.GetScope()))
	{
		return SubmitResult(
			Edemo_mapShanmenSwordRhythmEffectCueConsumerSubmitStatus::ScopeMismatch,
			TEXT("Consumer route belongs to a different Run or consumer scope."));
	}
	if (!Command.GetRoute().Matches(Route))
	{
		return SubmitResult(
			Edemo_mapShanmenSwordRhythmEffectCueConsumerSubmitStatus::RouteMismatch,
			TEXT("Attempt evidence belongs to a different consumer route."));
	}

	if (bHasRouteRecord && CurrentRoute.Route.Matches(Route))
	{
		for (const auto& Existing : CurrentRoute.Attempts)
		{
			if (Existing.GetCommand().GetAttemptId()
				!= Command.GetAttemptId())
			{
				continue;
			}
			if (!Existing.GetCommand().Matches(Command))
			{
				return SubmitResult(
					Edemo_mapShanmenSwordRhythmEffectCueConsumerSubmitStatus::
						AttemptConflict,
					TEXT("AttemptId replayed with different executor evidence."));
			}
			if (Command.GetOutcome()
				== Edemo_mapShanmenSwordRhythmEffectCueAttemptOutcome::
					RetryableFailure)
			{
				return SubmitResult(
					Edemo_mapShanmenSwordRhythmEffectCueConsumerSubmitStatus::
						RetryReplayed,
					TEXT("The exact retryable consumer attempt replayed."),
					Existing);
			}
			return SubmitResult(
				Command.GetOutcome()
						== Edemo_mapShanmenSwordRhythmEffectCueAttemptOutcome::
							NoOpSucceeded
					? Edemo_mapShanmenSwordRhythmEffectCueConsumerSubmitStatus::
						NoOpReplayed
					: Edemo_mapShanmenSwordRhythmEffectCueConsumerSubmitStatus::
						AcknowledgementReplayed,
				TEXT("The exact successful consumer attempt replayed."),
				Existing);
		}
		if (CurrentRoute.bAcknowledged)
		{
			return SubmitResult(
				Edemo_mapShanmenSwordRhythmEffectCueConsumerSubmitStatus::
					AlreadyAcknowledged,
				TEXT("Acknowledged route accepts only exact attempt replay."));
		}
	}
	else if (bHasRouteRecord)
	{
		const int32 IncomingRevision = Route.GetDelivery().GetEvent().GetState().
			GetObservationRevision();
		const int32 CurrentRevision = CurrentRoute.Route.GetDelivery().GetEvent().
			GetState().GetObservationRevision();
		if (IncomingRevision <= CurrentRevision)
		{
			return SubmitResult(
				Edemo_mapShanmenSwordRhythmEffectCueConsumerSubmitStatus::StaleRoute,
				TEXT("Consumer route does not advance current route order."));
		}
		if (!CurrentRoute.bAcknowledged)
		{
			return SubmitResult(
				Edemo_mapShanmenSwordRhythmEffectCueConsumerSubmitStatus::PendingRoute,
				TEXT("A retryable earlier route must succeed before a newer route."));
		}
	}

	const auto Before = *this;
	if (!bHasRouteRecord || !CurrentRoute.Route.Matches(Route))
	{
		bHasRouteRecord = true;
		CurrentRoute = FRouteRecord();
		CurrentRoute.Route = Route;
	}

	if (Command.GetOutcome()
		== Edemo_mapShanmenSwordRhythmEffectCueAttemptOutcome::RetryableFailure)
	{
		Fdemo_mapShanmenSwordRhythmEffectCueAttemptReceipt Receipt;
		if (!Fdemo_mapShanmenSwordRhythmEffectCueAttemptReceipt::TryCreate(
				Route, Command, nullptr, Receipt))
		{
			*this = Before;
			return SubmitResult(
				Edemo_mapShanmenSwordRhythmEffectCueConsumerSubmitStatus::StateInvalid,
				TEXT("Retry attempt failed immutable receipt creation."));
		}
		CurrentRoute.Attempts.Add(Receipt);
		if (!IsValid())
		{
			*this = Before;
			return SubmitResult(
				Edemo_mapShanmenSwordRhythmEffectCueConsumerSubmitStatus::StateInvalid,
				TEXT("Retry attempt failed coordinator self-validation."));
		}
		return SubmitResult(
			Edemo_mapShanmenSwordRhythmEffectCueConsumerSubmitStatus::RetryRecorded,
			TEXT("Retryable executor failure recorded; route remains pending."),
			Receipt);
	}

	const auto Acknowledged = Cursor.Acknowledge(Route.GetDelivery());
	if (Acknowledged.Status
		!= Edemo_mapShanmenSwordRhythmEffectCueAcknowledgeStatus::Acknowledged)
	{
		*this = Before;
		return SubmitResult(
			Edemo_mapShanmenSwordRhythmEffectCueConsumerSubmitStatus::
				AcknowledgementRejected,
			TEXT("Successful executor evidence did not advance the delivery cursor."));
	}
	Fdemo_mapShanmenSwordRhythmEffectCueAttemptReceipt Receipt;
	if (!Fdemo_mapShanmenSwordRhythmEffectCueAttemptReceipt::TryCreate(
			Route, Command, &Acknowledged.Acknowledgement, Receipt))
	{
		*this = Before;
		return SubmitResult(
			Edemo_mapShanmenSwordRhythmEffectCueConsumerSubmitStatus::StateInvalid,
			TEXT("Successful attempt failed immutable receipt creation."));
	}
	CurrentRoute.Attempts.Add(Receipt);
	CurrentRoute.bAcknowledged = true;
	if (!IsValid())
	{
		*this = Before;
		return SubmitResult(
			Edemo_mapShanmenSwordRhythmEffectCueConsumerSubmitStatus::StateInvalid,
			TEXT("Successful attempt failed coordinator self-validation."));
	}
	return SubmitResult(
		Command.GetOutcome()
				== Edemo_mapShanmenSwordRhythmEffectCueAttemptOutcome::NoOpSucceeded
			? Edemo_mapShanmenSwordRhythmEffectCueConsumerSubmitStatus::
				NoOpAcknowledged
			: Edemo_mapShanmenSwordRhythmEffectCueConsumerSubmitStatus::
				Acknowledged,
		Command.GetOutcome()
				== Edemo_mapShanmenSwordRhythmEffectCueAttemptOutcome::NoOpSucceeded
			? TEXT("Empty cue route acknowledged explicitly without playback.")
			: TEXT("Executor success acknowledged the consumer cue route."),
		Receipt);
}

bool Fdemo_mapShanmenSwordRhythmEffectCueConsumerCoordinator::
	TryGetLatestReceipt(
		Fdemo_mapShanmenSwordRhythmEffectCueAttemptReceipt& OutReceipt) const
{
	OutReceipt = Fdemo_mapShanmenSwordRhythmEffectCueAttemptReceipt();
	if (!IsValid() || !bHasRouteRecord || CurrentRoute.Attempts.IsEmpty())
	{
		return false;
	}
	OutReceipt = CurrentRoute.Attempts.Last();
	return true;
}

bool Fdemo_mapShanmenSwordRhythmEffectCueConsumerCoordinator::
	TryGetLastAcknowledgement(
		Fdemo_mapShanmenSwordRhythmEffectCueAcknowledgementReceipt&
			OutAcknowledgement) const
{
	OutAcknowledgement =
		Fdemo_mapShanmenSwordRhythmEffectCueAcknowledgementReceipt();
	return IsValid()
		&& Cursor.TryGetLastAcknowledgement(OutAcknowledgement);
}
