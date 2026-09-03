#include "demo_mapShanmenDivineSensePulseCoordinator.h"

#include "ShanmenCombatRuntimeTags.h"
#include "ShanmenDeterministicId.h"

namespace
{
	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	FString TransitionToken(
		const FShanmenActionTransitionReceipt& Transition)
	{
		if (!Transition.IsValid())
		{
			return FString();
		}
		return FString::Printf(
			TEXT("%s:%lld:%u:%u:%u:%u:%u"),
			*GuidDigits(Transition.GetActivationId()),
			Transition.GetSequence(),
			static_cast<uint32>(Transition.GetFromPhase()),
			static_cast<uint32>(Transition.GetToPhase()),
			static_cast<uint32>(Transition.GetTerminalReason()),
			Transition.CrossedCommitPointNow() ? 1 : 0,
			Transition.HasReachedCommitPoint() ? 1 : 0);
	}

	bool ActionsMatch(
		const FShanmenCombatActionSnapshot& Left,
		const FShanmenCombatActionSnapshot& Right)
	{
		return Left.IsValid() && Right.IsValid()
			&& Left.GetRunId() == Right.GetRunId()
			&& Left.GetOwnerId() == Right.GetOwnerId()
			&& Left.GetActivationId() == Right.GetActivationId()
			&& Left.GetSourceEntityId() == Right.GetSourceEntityId()
			&& Left.GetSourceItemInstanceId()
				== Right.GetSourceItemInstanceId()
			&& Left.GetActionDefinitionId()
				== Right.GetActionDefinitionId()
			&& Left.GetContent().Version == Right.GetContent().Version
			&& Left.GetContent().Digest == Right.GetContent().Digest
			&& Left.GetSourceTags() == Right.GetSourceTags();
	}

	bool IsExpectedTransition(
		const FShanmenActionTransitionReceipt& Transition,
		const FGuid& ActivationId,
		int64 Sequence,
		EShanmenCombatActionPhase From,
		EShanmenCombatActionPhase To,
		EShanmenActionTerminalReason TerminalReason,
		bool bCrossedCommitPointNow,
		bool bHasReachedCommitPoint)
	{
		return Transition.IsValid()
			&& Transition.GetActivationId() == ActivationId
			&& Transition.GetSequence() == Sequence
			&& Transition.GetFromPhase() == From
			&& Transition.GetToPhase() == To
			&& Transition.GetTerminalReason() == TerminalReason
			&& Transition.CrossedCommitPointNow()
				== bCrossedCommitPointNow
			&& Transition.HasReachedCommitPoint()
				== bHasReachedCommitPoint;
	}

	FGuid MakeCoordinatorId(
		const FGuid& RunId,
		int32 ProcessedPulseCapacity)
	{
		if (!RunId.IsValid() || ProcessedPulseCapacity <= 0)
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.Spell.DivineSense.PulseCoordinator.r1"),
			{
				GuidDigits(RunId),
				FString::FromInt(ProcessedPulseCapacity)
			});
	}

	FGuid MakePulseReceiptId(
		const Fdemo_mapShanmenDivineSensePulseReceipt& Receipt)
	{
		if (!Receipt.GetCoordinatorId().IsValid()
			|| !Receipt.GetStartup().IsValid()
			|| !Receipt.GetReservation().IsValid()
			|| !Receipt.GetActiveCommit().IsValid()
			|| !Receipt.GetResourceCommit().IsValid()
			|| !Receipt.GetWorldObservation().IsSuccess()
			|| !Receipt.GetRecovery().IsValid()
			|| !Receipt.GetCompletion().IsValid())
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.Spell.DivineSense.PulseReceipt.r1"),
			{
				GuidDigits(Receipt.GetCoordinatorId()),
				GuidDigits(Receipt.GetAction().GetActivationId()),
				GuidDigits(Receipt.GetDefinition().GetDefinitionId()),
				GuidDigits(Receipt.GetCost().GetCostId()),
				GuidDigits(Receipt.GetReservation().GetReceiptId()),
				GuidDigits(Receipt.GetResourceCommit().GetReceiptId()),
				GuidDigits(
					Receipt.GetWorldObservation().Receipt.GetReceiptId()),
				TransitionToken(Receipt.GetStartup()),
				TransitionToken(Receipt.GetActiveCommit()),
				TransitionToken(Receipt.GetRecovery()),
				TransitionToken(Receipt.GetCompletion()),
				FString::FromInt(Receipt.GetSubjectActorBudget()),
				FString::FromInt(Receipt.GetObservedSubjectCount())
			});
	}

	bool InputsMatch(
		const Fdemo_mapShanmenDivineSensePulseReceipt& Existing,
		const FShanmenCombatActionSnapshot& Action,
		const FShanmenDivineSenseDefinition& Definition,
		const FShanmenActionResourceCost& Cost,
		int32 ScanOrdinal,
		int32 SubjectActorBudget)
	{
		return Existing.IsValid()
			&& ActionsMatch(Existing.GetAction(), Action)
			&& Existing.GetDefinition().GetDefinitionId()
				== Definition.GetDefinitionId()
			&& Existing.GetCost().GetCostId() == Cost.GetCostId()
			&& Existing.GetScanOrdinal() == ScanOrdinal
			&& Existing.GetSubjectActorBudget() == SubjectActorBudget;
	}

	bool VerifyResourceReplay(
		const Fdemo_mapShanmenDivineSensePulseReceipt& Existing,
		const FShanmenActionResourceAuthority& ResourceAuthority,
		EShanmenActionResourceTransactionError& OutError)
	{
		OutError = EShanmenActionResourceTransactionError::None;
		FShanmenActionResourceAuthority Candidate = ResourceAuthority;
		const FShanmenActionResourceTransactionResult Reserved =
			Candidate.Reserve(Existing.GetReservation().GetRequest());
		if (!Reserved.IsSuccess()
			|| Reserved.Status
				!= EShanmenActionResourceTransactionStatus::AlreadyReserved
			|| Reserved.Reservation.GetReceiptId()
				!= Existing.GetReservation().GetReceiptId())
		{
			OutError = Reserved.Error
				!= EShanmenActionResourceTransactionError::None
				? Reserved.Error
				: EShanmenActionResourceTransactionError::StateDesynchronized;
			return false;
		}
		const FShanmenActionResourceTransactionResult Finalized =
			Candidate.Finalize(Existing.GetResourceCommit().GetRequest());
		if (!Finalized.IsSuccess()
			|| Finalized.Status
				!= EShanmenActionResourceTransactionStatus::AlreadyFinalized
			|| Finalized.Finalization.GetReceiptId()
				!= Existing.GetResourceCommit().GetReceiptId())
		{
			OutError = Finalized.Error
				!= EShanmenActionResourceTransactionError::None
				? Finalized.Error
				: EShanmenActionResourceTransactionError::StateDesynchronized;
			return false;
		}
		return true;
	}

	Fdemo_mapShanmenDivineSensePulseResult Reject(
		Edemo_mapShanmenDivineSensePulseError Error,
		const TCHAR* Diagnostic,
		EShanmenActionResourceTransactionError ResourceError =
			EShanmenActionResourceTransactionError::None,
		const Fdemo_mapShanmenDivineSenseWorldObservationResult*
			WorldFailure = nullptr)
	{
		Fdemo_mapShanmenDivineSensePulseResult Result;
		Result.Status = Edemo_mapShanmenDivineSensePulseStatus::Rejected;
		Result.Error = Error;
		Result.ResourceError = ResourceError;
		Result.Diagnostic = Diagnostic;
		if (WorldFailure)
		{
			Result.WorldFailure = *WorldFailure;
		}
		return Result;
	}
}

const FShanmenCombatActionSnapshot&
Fdemo_mapShanmenDivineSensePulseReceipt::GetAction() const
{
	return WorldObservation.Receipt.GetRequest().GetAction();
}

const FShanmenDivineSenseDefinition&
Fdemo_mapShanmenDivineSensePulseReceipt::GetDefinition() const
{
	return WorldObservation.Receipt.GetRequest().GetDefinition();
}

const FShanmenActionResourceCost&
Fdemo_mapShanmenDivineSensePulseReceipt::GetCost() const
{
	return Reservation.GetRequest().GetCost();
}

int32 Fdemo_mapShanmenDivineSensePulseReceipt::GetScanOrdinal() const
{
	return WorldObservation.Receipt.GetRequest().GetScanOrdinal();
}

bool Fdemo_mapShanmenDivineSensePulseReceipt::IsValid() const
{
	if (!ReceiptId.IsValid() || !CoordinatorId.IsValid()
		|| !WorldObservation.IsSuccess())
	{
		return false;
	}

	const FShanmenCombatActionSnapshot& Action = GetAction();
	const FGuid& ActivationId = Action.GetActivationId();
	if (!Action.IsValid()
		|| Action.GetActionDefinitionId()
			!= FShanmenDivineSenseDefinition::CanonicalActionDefinitionId()
		|| !GetDefinition().IsValid()
		|| GetDefinition().GetActionDefinitionId()
			!= Action.GetActionDefinitionId()
		|| !GetCost().IsValid()
		|| GetCost().GetResourceChannel()
			!= FShanmenCombatRuntimeNativeTags::ResourceSpiritEnergy()
		|| GetScanOrdinal() < 0
		|| GetSubjectActorBudget() < 0
		|| GetObservedSubjectCount() < 0
		|| GetObservedSubjectCount() > GetSubjectActorBudget())
	{
		return false;
	}

	if (!IsExpectedTransition(
			Startup,
			ActivationId,
			0,
			EShanmenCombatActionPhase::Idle,
			EShanmenCombatActionPhase::Startup,
			EShanmenActionTerminalReason::None,
			false,
			false)
		|| !IsExpectedTransition(
			ActiveCommit,
			ActivationId,
			1,
			EShanmenCombatActionPhase::Startup,
			EShanmenCombatActionPhase::Active,
			EShanmenActionTerminalReason::None,
			true,
			true)
		|| !IsExpectedTransition(
			Recovery,
			ActivationId,
			2,
			EShanmenCombatActionPhase::Active,
			EShanmenCombatActionPhase::Recovery,
			EShanmenActionTerminalReason::None,
			false,
			true)
		|| !IsExpectedTransition(
			Completion,
			ActivationId,
			3,
			EShanmenCombatActionPhase::Recovery,
			EShanmenCombatActionPhase::Idle,
			EShanmenActionTerminalReason::Completed,
			false,
			true))
	{
		return false;
	}

	const FShanmenActionResourceReservationRequest& ReservationRequest =
		Reservation.GetRequest();
	const FShanmenActionResourceFinalizationRequest& FinalizationRequest =
		ResourceCommit.GetRequest();
	return Reservation.IsValid()
		&& ActionsMatch(ReservationRequest.GetAction(), Action)
		&& ReservationRequest.GetStartupReceipt().GetSequence()
			== Startup.GetSequence()
		&& ReservationRequest.GetStartupReceipt().GetActivationId()
			== Startup.GetActivationId()
		&& ReservationRequest.GetCost().GetCostId() == GetCost().GetCostId()
		&& ResourceCommit.IsValid()
		&& FinalizationRequest.GetReservation().GetReceiptId()
			== Reservation.GetReceiptId()
		&& FinalizationRequest.GetTransition().GetActivationId()
			== ActiveCommit.GetActivationId()
		&& FinalizationRequest.GetTransition().GetSequence()
			== ActiveCommit.GetSequence()
		&& FinalizationRequest.GetDisposition()
			== EShanmenActionResourceDisposition::Commit
		&& ReceiptId == MakePulseReceiptId(*this);
}

bool Fdemo_mapShanmenDivineSensePulseResult::IsValid() const
{
	if (Diagnostic.IsEmpty())
	{
		return false;
	}
	if (Status == Edemo_mapShanmenDivineSensePulseStatus::Applied
		|| Status
			== Edemo_mapShanmenDivineSensePulseStatus::AlreadyApplied)
	{
		return Error == Edemo_mapShanmenDivineSensePulseError::None
			&& ResourceError
				== EShanmenActionResourceTransactionError::None
			&& Receipt.IsValid();
	}
	if (Status == Edemo_mapShanmenDivineSensePulseStatus::Rejected)
	{
		return Error != Edemo_mapShanmenDivineSensePulseError::None
			&& !Receipt.IsValid()
			&& (Error
					!= Edemo_mapShanmenDivineSensePulseError::
						WorldObservationRejected
				|| (!WorldFailure.IsSuccess()
					&& !WorldFailure.Diagnostic.IsEmpty()));
	}
	return false;
}

bool Fdemo_mapShanmenDivineSensePulseResult::IsSuccess() const
{
	return IsValid()
		&& (Status == Edemo_mapShanmenDivineSensePulseStatus::Applied
			|| Status
				== Edemo_mapShanmenDivineSensePulseStatus::AlreadyApplied);
}

bool Fdemo_mapShanmenDivineSensePulseCoordinator::TryCreate(
	const FGuid& InRunId,
	int32 InProcessedPulseCapacity,
	Fdemo_mapShanmenDivineSensePulseCoordinator& OutCoordinator)
{
	OutCoordinator.Reset();
	Fdemo_mapShanmenDivineSensePulseCoordinator Candidate;
	Candidate.RunId = InRunId;
	Candidate.ProcessedPulseCapacity = InProcessedPulseCapacity;
	Candidate.CoordinatorId = MakeCoordinatorId(
		InRunId, InProcessedPulseCapacity);
	Candidate.bInitialized = true;
	if (!Candidate.IsValid())
	{
		return false;
	}
	OutCoordinator = MoveTemp(Candidate);
	return true;
}

Fdemo_mapShanmenDivineSensePulseResult
Fdemo_mapShanmenDivineSensePulseCoordinator::Execute(
	UWorld* World,
	const FShanmenWorldEntityRegistry& EntityRegistry,
	AActor* SourceActor,
	const FShanmenCombatActionSnapshot& Action,
	const FShanmenDivineSenseDefinition& Definition,
	const FShanmenActionResourceCost& Cost,
	int32 ScanOrdinal,
	int32 SubjectActorBudget,
	const TArray<AActor*>& SubjectActors,
	const Idemo_mapShanmenDivineSenseWorldEvidenceProvider& EvidenceProvider,
	FShanmenActionResourceAuthority& ResourceAuthority)
{
	if (!IsValid())
	{
		return Reject(
			Edemo_mapShanmenDivineSensePulseError::CoordinatorNotReady,
			TEXT("Divine Sense pulse coordinator is not ready."));
	}
	if (!Action.IsValid() || !Definition.IsValid() || !Cost.IsValid()
		|| Action.GetActionDefinitionId()
			!= FShanmenDivineSenseDefinition::CanonicalActionDefinitionId()
		|| Definition.GetActionDefinitionId()
			!= Action.GetActionDefinitionId()
		|| Cost.GetResourceChannel()
			!= FShanmenCombatRuntimeNativeTags::ResourceSpiritEnergy()
		|| ScanOrdinal < 0 || SubjectActorBudget < 0)
	{
		return Reject(
			Edemo_mapShanmenDivineSensePulseError::InvalidInput,
			TEXT("Divine Sense pulse requires canonical action, definition, SpiritEnergy cost, ordinal, and budget."));
	}
	if (Action.GetRunId() != RunId)
	{
		return Reject(
			Edemo_mapShanmenDivineSensePulseError::RunMismatch,
			TEXT("Divine Sense pulse action belongs to another Run."));
	}

	if (const Fdemo_mapShanmenDivineSensePulseReceipt* Existing =
		ProcessedPulses.Find(Action.GetActivationId()))
	{
		if (!InputsMatch(
				*Existing,
				Action,
				Definition,
				Cost,
				ScanOrdinal,
				SubjectActorBudget))
		{
			return Reject(
				Edemo_mapShanmenDivineSensePulseError::ActivationConflict,
				TEXT("Divine Sense ActivationId was reused with another immutable pulse payload."));
		}
		EShanmenActionResourceTransactionError ReplayError;
		if (!VerifyResourceReplay(
				*Existing, ResourceAuthority, ReplayError))
		{
			return Reject(
				Edemo_mapShanmenDivineSensePulseError::StateDesynchronized,
				TEXT("Stored Divine Sense pulse and resource authority disagree."),
				ReplayError);
		}
		Fdemo_mapShanmenDivineSensePulseResult Replay;
		Replay.Status =
			Edemo_mapShanmenDivineSensePulseStatus::AlreadyApplied;
		Replay.Diagnostic =
			TEXT("Exact Divine Sense pulse replay returned stored proofs without World or provider I/O.");
		Replay.Receipt = *Existing;
		return Replay;
	}

	if (ProcessedPulses.Num() >= ProcessedPulseCapacity)
	{
		return Reject(
			Edemo_mapShanmenDivineSensePulseError::
				ProcessedCapacityExceeded,
			TEXT("Divine Sense processed-pulse capacity is exhausted."));
	}
	if (!ResourceAuthority.IsValid())
	{
		return Reject(
			Edemo_mapShanmenDivineSensePulseError::
				ResourceAuthorityNotReady,
			TEXT("Divine Sense pulse requires one valid resource authority."),
			EShanmenActionResourceTransactionError::AuthorityNotReady);
	}
	if (ResourceAuthority.GetOwnerEntityId()
		!= Action.GetSourceEntityId())
	{
		return Reject(
			Edemo_mapShanmenDivineSensePulseError::ResourceOwnerMismatch,
			TEXT("SpiritEnergy authority must belong to the Divine Sense source entity."),
			EShanmenActionResourceTransactionError::OwnerMismatch);
	}
	if (ResourceAuthority.GetResourceChannel()
		!= Cost.GetResourceChannel())
	{
		return Reject(
			Edemo_mapShanmenDivineSensePulseError::ResourceChannelMismatch,
			TEXT("Divine Sense cost and resource authority channel differ."),
			EShanmenActionResourceTransactionError::ChannelMismatch);
	}

	Fdemo_mapShanmenDivineSensePulseCoordinator CoordinatorCandidate =
		*this;
	FShanmenActionResourceAuthority AuthorityCandidate = ResourceAuthority;
	FShanmenActionOrchestrator ActionRuntime;
	FShanmenActionTransitionReceipt Startup;
	if (!FShanmenActionOrchestrator::TryStart(
			Action, ActionRuntime, Startup))
	{
		return Reject(
			Edemo_mapShanmenDivineSensePulseError::ActionStartRejected,
			TEXT("Action runtime rejected Divine Sense Startup."));
	}

	FShanmenActionResourceSnapshot ResourceSnapshot;
	if (!AuthorityCandidate.TryCaptureSnapshot(ResourceSnapshot))
	{
		return Reject(
			Edemo_mapShanmenDivineSensePulseError::
				ResourceSnapshotRejected,
			TEXT("SpiritEnergy snapshot capture failed."),
			EShanmenActionResourceTransactionError::AuthorityNotReady);
	}
	FShanmenActionResourceReservationRequest ReservationRequest;
	if (!FShanmenActionResourceReservationRequest::TryCreate(
			Action,
			Startup,
			Cost,
			ResourceSnapshot,
			ReservationRequest))
	{
		return Reject(
			Edemo_mapShanmenDivineSensePulseError::
				ReservationRequestRejected,
			TEXT("Divine Sense SpiritEnergy reservation request was invalid."));
	}
	const FShanmenActionResourceTransactionResult Reserved =
		AuthorityCandidate.Reserve(ReservationRequest);
	if (!Reserved.IsSuccess()
		|| Reserved.Status != EShanmenActionResourceTransactionStatus::Reserved)
	{
		return Reject(
			Edemo_mapShanmenDivineSensePulseError::
				ResourceReservationRejected,
			TEXT("SpiritEnergy reservation rejected the Divine Sense pulse."),
			Reserved.Error);
	}

	FShanmenActionTransitionReceipt ActiveCommit;
	if (!ActionRuntime.TryAdvance(
			EShanmenCombatActionPhase::Startup, ActiveCommit))
	{
		return Reject(
			Edemo_mapShanmenDivineSensePulseError::ActionCommitRejected,
			TEXT("Divine Sense action rejected the Startup-to-Active commit."));
	}
	FShanmenActionResourceFinalizationRequest FinalizationRequest;
	if (!FShanmenActionResourceFinalizationRequest::TryCreate(
			Reserved.Reservation,
			ActiveCommit,
			FinalizationRequest))
	{
		return Reject(
			Edemo_mapShanmenDivineSensePulseError::
				FinalizationRequestRejected,
			TEXT("Divine Sense SpiritEnergy commit request was invalid."));
	}
	const FShanmenActionResourceTransactionResult Finalized =
		AuthorityCandidate.Finalize(FinalizationRequest);
	if (!Finalized.IsSuccess()
		|| Finalized.Status
			!= EShanmenActionResourceTransactionStatus::Committed)
	{
		return Reject(
			Edemo_mapShanmenDivineSensePulseError::
				ResourceFinalizationRejected,
			TEXT("SpiritEnergy commit rejected the Divine Sense pulse."),
			Finalized.Error);
	}

	Fdemo_mapShanmenDivineSenseWorldObservationResult WorldObservation =
		Fdemo_mapShanmenDivineSenseWorldObservationAdapter::SampleAndResolve(
			World,
			EntityRegistry,
			SourceActor,
			Action,
			Definition,
			ScanOrdinal,
			SubjectActorBudget,
			SubjectActors,
			EvidenceProvider);
	if (!WorldObservation.IsSuccess())
	{
		return Reject(
			Edemo_mapShanmenDivineSensePulseError::
				WorldObservationRejected,
			TEXT("World observation rejected the staged Divine Sense pulse; SpiritEnergy was not published."),
			EShanmenActionResourceTransactionError::None,
			&WorldObservation);
	}

	FShanmenActionTransitionReceipt Recovery;
	if (!ActionRuntime.TryAdvance(
			EShanmenCombatActionPhase::Active, Recovery))
	{
		return Reject(
			Edemo_mapShanmenDivineSensePulseError::ActionRecoveryRejected,
			TEXT("Divine Sense action could not enter Recovery; staged resource state was discarded."));
	}
	FShanmenActionTransitionReceipt Completion;
	if (!ActionRuntime.TryAdvance(
			EShanmenCombatActionPhase::Recovery, Completion))
	{
		return Reject(
			Edemo_mapShanmenDivineSensePulseError::
				ActionCompletionRejected,
			TEXT("Divine Sense action could not complete; staged resource state was discarded."));
	}

	Fdemo_mapShanmenDivineSensePulseReceipt Receipt;
	Receipt.CoordinatorId = CoordinatorId;
	Receipt.Startup = Startup;
	Receipt.Reservation = Reserved.Reservation;
	Receipt.ActiveCommit = ActiveCommit;
	Receipt.ResourceCommit = Finalized.Finalization;
	Receipt.WorldObservation = MoveTemp(WorldObservation);
	Receipt.Recovery = Recovery;
	Receipt.Completion = Completion;
	Receipt.ReceiptId = MakePulseReceiptId(Receipt);
	if (!Receipt.IsValid())
	{
		return Reject(
			Edemo_mapShanmenDivineSensePulseError::StateDesynchronized,
			TEXT("Divine Sense pulse produced inconsistent staged proofs."));
	}

	CoordinatorCandidate.ProcessedPulses.Add(
		Action.GetActivationId(), Receipt);
	if (!CoordinatorCandidate.IsValid() || !AuthorityCandidate.IsValid())
	{
		return Reject(
			Edemo_mapShanmenDivineSensePulseError::StateDesynchronized,
			TEXT("Divine Sense pulse could not publish a valid atomic state."));
	}

	ResourceAuthority = MoveTemp(AuthorityCandidate);
	*this = MoveTemp(CoordinatorCandidate);
	Fdemo_mapShanmenDivineSensePulseResult Result;
	Result.Status = Edemo_mapShanmenDivineSensePulseStatus::Applied;
	Result.Diagnostic =
		TEXT("Divine Sense action, SpiritEnergy and World receipt committed atomically.");
	Result.Receipt = MoveTemp(Receipt);
	return Result;
}

bool Fdemo_mapShanmenDivineSensePulseCoordinator::IsValid() const
{
	if (!bInitialized || !CoordinatorId.IsValid() || !RunId.IsValid()
		|| ProcessedPulseCapacity <= 0
		|| ProcessedPulses.Num() > ProcessedPulseCapacity
		|| CoordinatorId
			!= MakeCoordinatorId(RunId, ProcessedPulseCapacity))
	{
		return false;
	}
	for (const TPair<FGuid, Fdemo_mapShanmenDivineSensePulseReceipt>& Pair :
		ProcessedPulses)
	{
		if (!Pair.Key.IsValid() || !Pair.Value.IsValid()
			|| Pair.Key != Pair.Value.GetAction().GetActivationId()
			|| Pair.Value.GetAction().GetRunId() != RunId
			|| Pair.Value.GetCoordinatorId() != CoordinatorId)
		{
			return false;
		}
	}
	return true;
}

void Fdemo_mapShanmenDivineSensePulseCoordinator::Reset()
{
	*this = Fdemo_mapShanmenDivineSensePulseCoordinator();
}
