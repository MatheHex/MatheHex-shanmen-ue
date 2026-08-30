#include "ShanmenSpiritShieldActionCoordinator.h"

#include "ShanmenCombatRuntimeTags.h"
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

	void AppendCanonicalTags(
		TArray<FString>& Parts,
		const TCHAR* Label,
		const FGameplayTagContainer& Container)
	{
		TArray<FGameplayTag> Tags;
		Container.GetGameplayTagArray(Tags);
		Tags.Sort([](const FGameplayTag& Left, const FGameplayTag& Right)
		{
			return Left.GetTagName().LexicalLess(Right.GetTagName());
		});
		Parts.Add(Label);
		Parts.Add(FString::FromInt(Tags.Num()));
		for (const FGameplayTag& Tag : Tags)
		{
			Parts.Add(Tag.ToString());
		}
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
			&& Left.GetSourceItemInstanceId() == Right.GetSourceItemInstanceId()
			&& Left.GetActionDefinitionId() == Right.GetActionDefinitionId()
			&& Left.GetContent().Version == Right.GetContent().Version
			&& Left.GetContent().Digest == Right.GetContent().Digest
			&& Left.GetSourceTags() == Right.GetSourceTags();
	}

	bool DefinitionsMatch(
		const FShanmenSpiritShieldDefinition& Left,
		const FShanmenSpiritShieldDefinition& Right)
	{
		return Left.IsValid() && Right.IsValid()
			&& Left.GetActionDefinitionId() == Right.GetActionDefinitionId()
			&& Left.GetRuleId() == Right.GetRuleId()
			&& FloatValueBits(Left.GetMaximumCapacity())
				== FloatValueBits(Right.GetMaximumCapacity())
			&& Left.GetRequiredDamageTags() == Right.GetRequiredDamageTags()
			&& Left.GetBlockedDamageTags() == Right.GetBlockedDamageTags()
			&& Left.GetRequiredSourceTags() == Right.GetRequiredSourceTags()
			&& Left.GetBlockedSourceTags() == Right.GetBlockedSourceTags()
			&& Left.GetRequiredTargetTags() == Right.GetRequiredTargetTags()
			&& Left.GetBlockedTargetTags() == Right.GetBlockedTargetTags();
	}

	bool CostsMatch(
		const FShanmenActionResourceCost& Left,
		const FShanmenActionResourceCost& Right)
	{
		return Left.IsValid() && Right.IsValid()
			&& Left.GetCostId() == Right.GetCostId()
			&& Left.GetRuleId() == Right.GetRuleId()
			&& Left.GetResourceChannel() == Right.GetResourceChannel()
			&& FloatValueBits(Left.GetAmount())
				== FloatValueBits(Right.GetAmount());
	}

	bool TransitionsMatch(
		const FShanmenActionTransitionReceipt& Left,
		const FShanmenActionTransitionReceipt& Right)
	{
		return Left.IsValid() && Right.IsValid()
			&& Left.GetActivationId() == Right.GetActivationId()
			&& Left.GetSequence() == Right.GetSequence()
			&& Left.GetFromPhase() == Right.GetFromPhase()
			&& Left.GetToPhase() == Right.GetToPhase()
			&& Left.GetTerminalReason() == Right.GetTerminalReason()
			&& Left.CrossedCommitPointNow()
				== Right.CrossedCommitPointNow()
			&& Left.HasReachedCommitPoint()
				== Right.HasReachedCommitPoint();
	}

	FGuid MakeDefinitionBindingId(
		const FShanmenSpiritShieldDefinition& Definition)
	{
		if (!Definition.IsValid())
		{
			return FGuid();
		}
		TArray<FString> Parts = {
			Definition.GetActionDefinitionId().ToString(),
			Definition.GetRuleId().ToString(),
			FloatBits(Definition.GetMaximumCapacity())
		};
		AppendCanonicalTags(
			Parts, TEXT("RequiredDamage"),
			Definition.GetRequiredDamageTags());
		AppendCanonicalTags(
			Parts, TEXT("BlockedDamage"),
			Definition.GetBlockedDamageTags());
		AppendCanonicalTags(
			Parts, TEXT("RequiredSource"),
			Definition.GetRequiredSourceTags());
		AppendCanonicalTags(
			Parts, TEXT("BlockedSource"),
			Definition.GetBlockedSourceTags());
		AppendCanonicalTags(
			Parts, TEXT("RequiredTarget"),
			Definition.GetRequiredTargetTags());
		AppendCanonicalTags(
			Parts, TEXT("BlockedTarget"),
			Definition.GetBlockedTargetTags());
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Spell.SpiritShield.Action.DefinitionBinding.r1"),
			Parts);
	}

	FGuid MakeSessionId(
		const FShanmenCombatActionSnapshot& Action,
		const FShanmenSpiritShieldDefinition& Definition,
		const FShanmenActionResourceReservationReceipt& Reservation)
	{
		const FGuid DefinitionBindingId =
			MakeDefinitionBindingId(Definition);
		if (!Action.IsValid() || !DefinitionBindingId.IsValid()
			|| !Reservation.IsValid())
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Spell.SpiritShield.Action.Session.r1"),
			{
				GuidDigits(Action.GetActivationId()),
				GuidDigits(DefinitionBindingId),
				GuidDigits(Reservation.GetReservationId())
			});
	}

	FGuid MakeStartupReceiptId(
		const FGuid& SessionId,
		const FShanmenActionResourceReservationReceipt& Reservation)
	{
		if (!SessionId.IsValid() || !Reservation.IsValid())
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Spell.SpiritShield.Action.StartupReceipt.r1"),
			{
				GuidDigits(SessionId),
				GuidDigits(Reservation.GetReceiptId())
			});
	}

	EShanmenSpiritShieldActionOutcome ClassifyOutcome(
		const FShanmenActionTransitionReceipt& Transition)
	{
		if (!Transition.IsValid()
			|| Transition.GetFromPhase()
				!= EShanmenCombatActionPhase::Startup)
		{
			return EShanmenSpiritShieldActionOutcome::None;
		}
		if (Transition.GetToPhase() == EShanmenCombatActionPhase::Active
			&& Transition.GetTerminalReason()
				== EShanmenActionTerminalReason::None
			&& Transition.CrossedCommitPointNow()
			&& Transition.HasReachedCommitPoint())
		{
			return EShanmenSpiritShieldActionOutcome::Active;
		}
		if (Transition.GetToPhase() == EShanmenCombatActionPhase::Cancelled
			&& Transition.GetTerminalReason()
				== EShanmenActionTerminalReason::Cancelled
			&& !Transition.HasReachedCommitPoint())
		{
			return EShanmenSpiritShieldActionOutcome::Cancelled;
		}
		if (Transition.GetToPhase() == EShanmenCombatActionPhase::Interrupted
			&& Transition.GetTerminalReason()
				== EShanmenActionTerminalReason::Interrupted
			&& !Transition.HasReachedCommitPoint())
		{
			return EShanmenSpiritShieldActionOutcome::Interrupted;
		}
		return EShanmenSpiritShieldActionOutcome::None;
	}

	FGuid MakeTerminalReceiptId(
		const FShanmenSpiritShieldActionStartupReceipt& Startup,
		const FShanmenActionResourceFinalizationReceipt& Finalization,
		const FShanmenSpiritShieldActivationReceipt& Activation,
		EShanmenSpiritShieldActionOutcome Outcome)
	{
		if (!Startup.IsValid() || !Finalization.IsValid()
			|| Outcome == EShanmenSpiritShieldActionOutcome::None)
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Spell.SpiritShield.Action.TerminalReceipt.r1"),
			{
				GuidDigits(Startup.GetReceiptId()),
				GuidDigits(Finalization.GetReceiptId()),
				Activation.IsValid()
					? GuidDigits(Activation.GetReceiptId()) : TEXT("None"),
				FString::FromInt(static_cast<int32>(Outcome))
			});
	}

	bool IsAbortOutcome(EShanmenSpiritShieldActionOutcome Outcome)
	{
		return Outcome == EShanmenSpiritShieldActionOutcome::Cancelled
			|| Outcome == EShanmenSpiritShieldActionOutcome::Interrupted;
	}
}

bool FShanmenSpiritShieldActionStartupReceipt::IsValid() const
{
	if (!ReceiptId.IsValid() || !SessionId.IsValid()
		|| !Action.IsValid() || !Definition.IsValid()
		|| !StartupTransition.IsValid() || !Reservation.IsValid()
		|| Action.GetActionDefinitionId()
			!= Definition.GetActionDefinitionId())
	{
		return false;
	}
	const FShanmenActionResourceReservationRequest& Request =
		Reservation.GetRequest();
	return ActionsMatch(Action, Request.GetAction())
		&& TransitionsMatch(
			StartupTransition, Request.GetStartupReceipt())
		&& Request.GetCost().GetResourceChannel()
			== FShanmenCombatRuntimeNativeTags::ResourceSpiritEnergy()
		&& SessionId == MakeSessionId(Action, Definition, Reservation)
		&& ReceiptId == MakeStartupReceiptId(SessionId, Reservation);
}

bool FShanmenSpiritShieldActionTerminalReceipt::IsValid() const
{
	if (!ReceiptId.IsValid() || !Startup.IsValid()
		|| !ActionTransition.IsValid()
		|| !ResourceFinalization.IsValid()
		|| Outcome == EShanmenSpiritShieldActionOutcome::None
		|| Outcome != ClassifyOutcome(ActionTransition))
	{
		return false;
	}
	const FShanmenActionResourceFinalizationRequest& FinalRequest =
		ResourceFinalization.GetRequest();
	if (!TransitionsMatch(
			ActionTransition, FinalRequest.GetTransition())
		|| FinalRequest.GetReservation().GetReceiptId()
			!= Startup.GetReservation().GetReceiptId())
	{
		return false;
	}

	const bool bActive = Outcome == EShanmenSpiritShieldActionOutcome::Active;
	const bool bDispositionMatches = bActive
		? FinalRequest.GetDisposition()
			== EShanmenActionResourceDisposition::Commit
		: IsAbortOutcome(Outcome)
			&& FinalRequest.GetDisposition()
				== EShanmenActionResourceDisposition::Release;
	const bool bActivationMatches = bActive
		? ShieldActivation.IsValid()
			&& ActionsMatch(
				ShieldActivation.GetAction(), Startup.GetAction())
			&& DefinitionsMatch(
				ShieldActivation.GetDefinition(), Startup.GetDefinition())
		: !ShieldActivation.IsValid();
	return bDispositionMatches && bActivationMatches
		&& ReceiptId == MakeTerminalReceiptId(
			Startup, ResourceFinalization, ShieldActivation, Outcome);
}

bool FShanmenSpiritShieldActionResult::IsValid() const
{
	switch (Status)
	{
	case EShanmenSpiritShieldActionStatus::Begun:
	case EShanmenSpiritShieldActionStatus::AlreadyBegun:
		return Error == EShanmenSpiritShieldActionError::None
			&& ResourceError
				== EShanmenActionResourceTransactionError::None
			&& Startup.IsValid() && !Terminal.IsValid();
	case EShanmenSpiritShieldActionStatus::Activated:
		return Error == EShanmenSpiritShieldActionError::None
			&& ResourceError
				== EShanmenActionResourceTransactionError::None
			&& !Startup.IsValid() && Terminal.IsValid()
			&& Terminal.GetOutcome()
				== EShanmenSpiritShieldActionOutcome::Active;
	case EShanmenSpiritShieldActionStatus::Aborted:
		return Error == EShanmenSpiritShieldActionError::None
			&& ResourceError
				== EShanmenActionResourceTransactionError::None
			&& !Startup.IsValid() && Terminal.IsValid()
			&& IsAbortOutcome(Terminal.GetOutcome());
	case EShanmenSpiritShieldActionStatus::AlreadyFinalized:
		return Error == EShanmenSpiritShieldActionError::None
			&& ResourceError
				== EShanmenActionResourceTransactionError::None
			&& !Startup.IsValid() && Terminal.IsValid();
	case EShanmenSpiritShieldActionStatus::Rejected:
		return Error != EShanmenSpiritShieldActionError::None
			&& !Startup.IsValid() && !Terminal.IsValid();
	default:
		return false;
	}
}

bool FShanmenSpiritShieldActionResult::IsSuccess() const
{
	return IsValid()
		&& Status != EShanmenSpiritShieldActionStatus::Rejected;
}

FShanmenSpiritShieldActionResult
FShanmenSpiritShieldActionCoordinator::Begin(
	const FShanmenCombatActionSnapshot& Action,
	const FShanmenSpiritShieldDefinition& Definition,
	const FShanmenActionResourceCost& Cost,
	FShanmenActionResourceAuthority& ResourceAuthority,
	FShanmenSpiritShieldActionCoordinator& OutCoordinator)
{
	if (!Action.IsValid() || !Definition.IsValid() || !Cost.IsValid()
		|| Action.GetActionDefinitionId()
			!= Definition.GetActionDefinitionId()
		|| Cost.GetResourceChannel()
			!= FShanmenCombatRuntimeNativeTags::ResourceSpiritEnergy())
	{
		return Reject(EShanmenSpiritShieldActionError::InvalidInput);
	}

	if (OutCoordinator.bInitialized)
	{
		if (!OutCoordinator.IsValid())
		{
			return Reject(
				EShanmenSpiritShieldActionError::CoordinatorNotReady);
		}
		if (OutCoordinator.State
				!= EShanmenSpiritShieldActionState::Reserved
			|| !ActionsMatch(
				Action, OutCoordinator.StartupReceipt.GetAction())
			|| !DefinitionsMatch(
				Definition,
				OutCoordinator.StartupReceipt.GetDefinition())
			|| !CostsMatch(
				Cost,
				OutCoordinator.StartupReceipt.GetReservation()
					.GetRequest().GetCost()))
		{
			return Reject(
				EShanmenSpiritShieldActionError::CoordinatorConflict);
		}

		FShanmenActionResourceAuthority AuthorityCandidate =
			ResourceAuthority;
		const FShanmenActionResourceTransactionResult Replay =
			AuthorityCandidate.Reserve(
				OutCoordinator.StartupReceipt.GetReservation()
					.GetRequest());
		if (!Replay.IsSuccess()
			|| Replay.Status
				!= EShanmenActionResourceTransactionStatus::AlreadyReserved
			|| Replay.Reservation.GetReceiptId()
				!= OutCoordinator.StartupReceipt.GetReservation()
					.GetReceiptId())
		{
			return Reject(
				EShanmenSpiritShieldActionError::StateDesynchronized,
				Replay.Error);
		}
		FShanmenSpiritShieldActionResult Result;
		Result.Status = EShanmenSpiritShieldActionStatus::AlreadyBegun;
		Result.Startup = OutCoordinator.StartupReceipt;
		return Result;
	}

	if (!ResourceAuthority.IsValid())
	{
		return Reject(
			EShanmenSpiritShieldActionError::ResourceReservationRejected,
			EShanmenActionResourceTransactionError::AuthorityNotReady);
	}
	if (Action.GetSourceEntityId()
		!= ResourceAuthority.GetOwnerEntityId())
	{
		return Reject(
			EShanmenSpiritShieldActionError::ResourceReservationRejected,
			EShanmenActionResourceTransactionError::OwnerMismatch);
	}
	if (Cost.GetResourceChannel()
		!= ResourceAuthority.GetResourceChannel())
	{
		return Reject(
			EShanmenSpiritShieldActionError::ResourceReservationRejected,
			EShanmenActionResourceTransactionError::ChannelMismatch);
	}

	FShanmenSpiritShieldActionCoordinator Candidate;
	FShanmenActionResourceAuthority AuthorityCandidate = ResourceAuthority;
	FShanmenActionTransitionReceipt StartupTransition;
	if (!FShanmenActionOrchestrator::TryStart(
			Action, Candidate.ActionRuntime, StartupTransition))
	{
		return Reject(
			EShanmenSpiritShieldActionError::ActionTransitionRejected);
	}
	if (!FShanmenSpiritShieldRuntime::TryPrepare(
			Action, Definition, Candidate.ShieldRuntime))
	{
		return Reject(
			EShanmenSpiritShieldActionError::InvalidInput);
	}

	FShanmenActionResourceSnapshot ResourceSnapshot;
	if (!AuthorityCandidate.TryCaptureSnapshot(ResourceSnapshot))
	{
		return Reject(
			EShanmenSpiritShieldActionError::ResourceReservationRejected,
			EShanmenActionResourceTransactionError::AuthorityNotReady);
	}
	FShanmenActionResourceReservationRequest ReservationRequest;
	if (!FShanmenActionResourceReservationRequest::TryCreate(
			Action,
			StartupTransition,
			Cost,
			ResourceSnapshot,
			ReservationRequest))
	{
		return Reject(
			EShanmenSpiritShieldActionError::InvalidInput);
	}
	const FShanmenActionResourceTransactionResult Reserved =
		AuthorityCandidate.Reserve(ReservationRequest);
	if (!Reserved.IsSuccess()
		|| Reserved.Status
			!= EShanmenActionResourceTransactionStatus::Reserved)
	{
		return Reject(
			EShanmenSpiritShieldActionError::ResourceReservationRejected,
			Reserved.Error);
	}

	Candidate.StartupReceipt.Action = Action;
	Candidate.StartupReceipt.Definition = Definition;
	Candidate.StartupReceipt.StartupTransition = StartupTransition;
	Candidate.StartupReceipt.Reservation = Reserved.Reservation;
	Candidate.StartupReceipt.SessionId = MakeSessionId(
		Action, Definition, Reserved.Reservation);
	Candidate.StartupReceipt.ReceiptId = MakeStartupReceiptId(
		Candidate.StartupReceipt.SessionId, Reserved.Reservation);
	Candidate.State = EShanmenSpiritShieldActionState::Reserved;
	Candidate.bInitialized = true;
	if (!Candidate.IsValid())
	{
		return Reject(
			EShanmenSpiritShieldActionError::StateDesynchronized);
	}

	ResourceAuthority = MoveTemp(AuthorityCandidate);
	OutCoordinator = MoveTemp(Candidate);
	FShanmenSpiritShieldActionResult Result;
	Result.Status = EShanmenSpiritShieldActionStatus::Begun;
	Result.Startup = OutCoordinator.StartupReceipt;
	return Result;
}

bool FShanmenSpiritShieldActionCoordinator::IsValid() const
{
	if (!bInitialized || !StartupReceipt.IsValid()
		|| !ActionRuntime.IsValid() || !ShieldRuntime.IsValid()
		|| !ActionsMatch(
			ActionRuntime.GetAction(), StartupReceipt.GetAction())
		|| !ActionsMatch(
			ShieldRuntime.GetAction(), StartupReceipt.GetAction())
		|| !DefinitionsMatch(
			ShieldRuntime.GetDefinition(), StartupReceipt.GetDefinition()))
	{
		return false;
	}

	if (State == EShanmenSpiritShieldActionState::Reserved)
	{
		return !TerminalReceipt.IsValid()
			&& ActionRuntime.GetPhase()
				== EShanmenCombatActionPhase::Startup
			&& !ActionRuntime.IsTerminal()
			&& !ActionRuntime.HasReachedCommitPoint()
			&& ShieldRuntime.GetState()
				== EShanmenSpiritShieldState::Prepared;
	}

	if (!TerminalReceipt.IsValid()
		|| TerminalReceipt.GetStartup().GetReceiptId()
			!= StartupReceipt.GetReceiptId())
	{
		return false;
	}
	if (State == EShanmenSpiritShieldActionState::Activated)
	{
		const FShanmenSpiritShieldActivationReceipt& Activation =
			TerminalReceipt.GetShieldActivation();
		return TerminalReceipt.GetOutcome()
				== EShanmenSpiritShieldActionOutcome::Active
			&& ActionRuntime.GetPhase()
				== EShanmenCombatActionPhase::Active
			&& !ActionRuntime.IsTerminal()
			&& ActionRuntime.HasReachedCommitPoint()
			&& ShieldRuntime.GetState()
				!= EShanmenSpiritShieldState::Prepared
			&& ShieldRuntime.GetActivationReceipt().GetReceiptId()
				== Activation.GetReceiptId();
	}
	if (State == EShanmenSpiritShieldActionState::Aborted)
	{
		const EShanmenSpiritShieldActionOutcome Outcome =
			TerminalReceipt.GetOutcome();
		const EShanmenActionTerminalReason ExpectedReason = Outcome
			== EShanmenSpiritShieldActionOutcome::Cancelled
			? EShanmenActionTerminalReason::Cancelled
			: Outcome == EShanmenSpiritShieldActionOutcome::Interrupted
				? EShanmenActionTerminalReason::Interrupted
				: EShanmenActionTerminalReason::None;
		return ExpectedReason != EShanmenActionTerminalReason::None
			&& ActionRuntime.IsTerminal()
			&& ActionRuntime.GetTerminalReason() == ExpectedReason
			&& !ActionRuntime.HasReachedCommitPoint()
			&& ShieldRuntime.GetState()
				== EShanmenSpiritShieldState::Prepared;
	}
	return false;
}

FShanmenSpiritShieldActionResult
FShanmenSpiritShieldActionCoordinator::Commit(
	FShanmenActionResourceAuthority& ResourceAuthority)
{
	if (!IsValid())
	{
		return Reject(
			EShanmenSpiritShieldActionError::CoordinatorNotReady);
	}
	if (State == EShanmenSpiritShieldActionState::Activated)
	{
		return ReplayTerminal(
			EShanmenSpiritShieldActionOutcome::Active,
			ResourceAuthority);
	}
	if (State != EShanmenSpiritShieldActionState::Reserved)
	{
		return Reject(
			EShanmenSpiritShieldActionError::FinalizationConflict);
	}

	FShanmenSpiritShieldActionCoordinator Candidate = *this;
	FShanmenActionResourceAuthority AuthorityCandidate = ResourceAuthority;
	FShanmenActionTransitionReceipt CommitTransition;
	if (!Candidate.ActionRuntime.TryAdvance(
			EShanmenCombatActionPhase::Startup, CommitTransition))
	{
		return Reject(
			EShanmenSpiritShieldActionError::ActionTransitionRejected);
	}
	FShanmenActionResourceFinalizationRequest FinalizationRequest;
	if (!FShanmenActionResourceFinalizationRequest::TryCreate(
			StartupReceipt.GetReservation(),
			CommitTransition,
			FinalizationRequest))
	{
		return Reject(
			EShanmenSpiritShieldActionError::StateDesynchronized);
	}
	const FShanmenActionResourceTransactionResult Finalized =
		AuthorityCandidate.Finalize(FinalizationRequest);
	if (!Finalized.IsSuccess()
		|| Finalized.Status
			!= EShanmenActionResourceTransactionStatus::Committed)
	{
		return Reject(
			EShanmenSpiritShieldActionError::ResourceFinalizationRejected,
			Finalized.Error);
	}
	FShanmenSpiritShieldActivationReceipt Activation;
	if (!Candidate.ShieldRuntime.TryActivate(
			Candidate.ActionRuntime, Activation))
	{
		return Reject(
			EShanmenSpiritShieldActionError::ShieldActivationRejected);
	}

	Candidate.TerminalReceipt.Startup = StartupReceipt;
	Candidate.TerminalReceipt.ActionTransition = CommitTransition;
	Candidate.TerminalReceipt.ResourceFinalization =
		Finalized.Finalization;
	Candidate.TerminalReceipt.ShieldActivation = Activation;
	Candidate.TerminalReceipt.Outcome =
		EShanmenSpiritShieldActionOutcome::Active;
	Candidate.TerminalReceipt.ReceiptId = MakeTerminalReceiptId(
		Candidate.TerminalReceipt.Startup,
		Candidate.TerminalReceipt.ResourceFinalization,
		Candidate.TerminalReceipt.ShieldActivation,
		Candidate.TerminalReceipt.Outcome);
	Candidate.State = EShanmenSpiritShieldActionState::Activated;
	if (!Candidate.IsValid())
	{
		return Reject(
			EShanmenSpiritShieldActionError::StateDesynchronized);
	}

	ResourceAuthority = MoveTemp(AuthorityCandidate);
	*this = MoveTemp(Candidate);
	FShanmenSpiritShieldActionResult Result;
	Result.Status = EShanmenSpiritShieldActionStatus::Activated;
	Result.Terminal = TerminalReceipt;
	return Result;
}

FShanmenSpiritShieldActionResult
FShanmenSpiritShieldActionCoordinator::Abort(
	EShanmenActionTerminalReason Reason,
	FShanmenActionResourceAuthority& ResourceAuthority)
{
	const EShanmenSpiritShieldActionOutcome ExpectedOutcome = Reason
		== EShanmenActionTerminalReason::Cancelled
		? EShanmenSpiritShieldActionOutcome::Cancelled
		: Reason == EShanmenActionTerminalReason::Interrupted
			? EShanmenSpiritShieldActionOutcome::Interrupted
			: EShanmenSpiritShieldActionOutcome::None;
	if (ExpectedOutcome == EShanmenSpiritShieldActionOutcome::None)
	{
		return Reject(EShanmenSpiritShieldActionError::InvalidInput);
	}
	if (!IsValid())
	{
		return Reject(
			EShanmenSpiritShieldActionError::CoordinatorNotReady);
	}
	if (State == EShanmenSpiritShieldActionState::Aborted)
	{
		if (TerminalReceipt.GetOutcome() != ExpectedOutcome)
		{
			return Reject(
				EShanmenSpiritShieldActionError::FinalizationConflict);
		}
		return ReplayTerminal(ExpectedOutcome, ResourceAuthority);
	}
	if (State != EShanmenSpiritShieldActionState::Reserved)
	{
		return Reject(
			EShanmenSpiritShieldActionError::FinalizationConflict);
	}

	FShanmenSpiritShieldActionCoordinator Candidate = *this;
	FShanmenActionResourceAuthority AuthorityCandidate = ResourceAuthority;
	FShanmenActionTransitionReceipt AbortTransition;
	const bool bTransitioned = Reason
		== EShanmenActionTerminalReason::Cancelled
		? Candidate.ActionRuntime.TryCancel(
			EShanmenCombatActionPhase::Startup, AbortTransition)
		: Candidate.ActionRuntime.TryInterrupt(
			EShanmenCombatActionPhase::Startup, AbortTransition);
	if (!bTransitioned)
	{
		return Reject(
			EShanmenSpiritShieldActionError::ActionTransitionRejected);
	}
	FShanmenActionResourceFinalizationRequest FinalizationRequest;
	if (!FShanmenActionResourceFinalizationRequest::TryCreate(
			StartupReceipt.GetReservation(),
			AbortTransition,
			FinalizationRequest))
	{
		return Reject(
			EShanmenSpiritShieldActionError::StateDesynchronized);
	}
	const FShanmenActionResourceTransactionResult Finalized =
		AuthorityCandidate.Finalize(FinalizationRequest);
	if (!Finalized.IsSuccess()
		|| Finalized.Status
			!= EShanmenActionResourceTransactionStatus::Released)
	{
		return Reject(
			EShanmenSpiritShieldActionError::ResourceFinalizationRejected,
			Finalized.Error);
	}

	Candidate.TerminalReceipt.Startup = StartupReceipt;
	Candidate.TerminalReceipt.ActionTransition = AbortTransition;
	Candidate.TerminalReceipt.ResourceFinalization =
		Finalized.Finalization;
	Candidate.TerminalReceipt.Outcome = ExpectedOutcome;
	Candidate.TerminalReceipt.ReceiptId = MakeTerminalReceiptId(
		Candidate.TerminalReceipt.Startup,
		Candidate.TerminalReceipt.ResourceFinalization,
		Candidate.TerminalReceipt.ShieldActivation,
		Candidate.TerminalReceipt.Outcome);
	Candidate.State = EShanmenSpiritShieldActionState::Aborted;
	if (!Candidate.IsValid())
	{
		return Reject(
			EShanmenSpiritShieldActionError::StateDesynchronized);
	}

	ResourceAuthority = MoveTemp(AuthorityCandidate);
	*this = MoveTemp(Candidate);
	FShanmenSpiritShieldActionResult Result;
	Result.Status = EShanmenSpiritShieldActionStatus::Aborted;
	Result.Terminal = TerminalReceipt;
	return Result;
}

void FShanmenSpiritShieldActionCoordinator::Reset()
{
	*this = FShanmenSpiritShieldActionCoordinator();
}

FShanmenSpiritShieldActionResult
FShanmenSpiritShieldActionCoordinator::Reject(
	EShanmenSpiritShieldActionError Error,
	EShanmenActionResourceTransactionError ResourceError)
{
	FShanmenSpiritShieldActionResult Result;
	Result.Status = EShanmenSpiritShieldActionStatus::Rejected;
	Result.Error = Error;
	Result.ResourceError = ResourceError;
	return Result;
}

FShanmenSpiritShieldActionResult
FShanmenSpiritShieldActionCoordinator::ReplayTerminal(
	EShanmenSpiritShieldActionOutcome ExpectedOutcome,
	FShanmenActionResourceAuthority& ResourceAuthority) const
{
	if (!IsValid() || !TerminalReceipt.IsValid()
		|| TerminalReceipt.GetOutcome() != ExpectedOutcome)
	{
		return Reject(
			EShanmenSpiritShieldActionError::FinalizationConflict);
	}
	FShanmenActionResourceAuthority AuthorityCandidate = ResourceAuthority;
	const FShanmenActionResourceTransactionResult Replay =
		AuthorityCandidate.Finalize(
			TerminalReceipt.GetResourceFinalization().GetRequest());
	if (!Replay.IsSuccess()
		|| Replay.Status
			!= EShanmenActionResourceTransactionStatus::AlreadyFinalized
		|| Replay.Finalization.GetReceiptId()
			!= TerminalReceipt.GetResourceFinalization().GetReceiptId())
	{
		return Reject(
			EShanmenSpiritShieldActionError::StateDesynchronized,
			Replay.Error);
	}
	if (ExpectedOutcome == EShanmenSpiritShieldActionOutcome::Active)
	{
		FShanmenSpiritShieldRuntime ShieldCandidate = ShieldRuntime;
		FShanmenSpiritShieldActivationReceipt Activation;
		if (!ShieldCandidate.TryActivate(ActionRuntime, Activation)
			|| Activation.GetReceiptId()
				!= TerminalReceipt.GetShieldActivation().GetReceiptId())
		{
			return Reject(
				EShanmenSpiritShieldActionError::StateDesynchronized);
		}
	}

	FShanmenSpiritShieldActionResult Result;
	Result.Status = EShanmenSpiritShieldActionStatus::AlreadyFinalized;
	Result.Terminal = TerminalReceipt;
	return Result;
}
