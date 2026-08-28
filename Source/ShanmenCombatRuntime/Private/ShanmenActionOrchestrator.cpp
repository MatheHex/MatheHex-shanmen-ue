#include "ShanmenActionOrchestrator.h"

namespace
{
	bool IsRunningPhase(EShanmenCombatActionPhase Phase)
	{
		return Phase == EShanmenCombatActionPhase::Startup
			|| Phase == EShanmenCombatActionPhase::Active
			|| Phase == EShanmenCombatActionPhase::Recovery;
	}
}

bool FShanmenActionTransitionReceipt::IsValid() const
{
	if (!ActivationId.IsValid() || Sequence < 0 || FromPhase == ToPhase)
	{
		return false;
	}

	const bool bIsCommitTransition = FromPhase == EShanmenCombatActionPhase::Startup
		&& ToPhase == EShanmenCombatActionPhase::Active;
	if (bCrossedCommitPointNow != bIsCommitTransition
		|| (bCrossedCommitPointNow && !bCommitPointReached))
	{
		return false;
	}

	switch (TerminalReason)
	{
	case EShanmenActionTerminalReason::None:
		if (FromPhase == EShanmenCombatActionPhase::Idle
			&& ToPhase == EShanmenCombatActionPhase::Startup)
		{
			return !bCommitPointReached;
		}
		if (bIsCommitTransition)
		{
			return bCommitPointReached;
		}
		return FromPhase == EShanmenCombatActionPhase::Active
			&& ToPhase == EShanmenCombatActionPhase::Recovery
			&& bCommitPointReached;

	case EShanmenActionTerminalReason::Completed:
		return FromPhase == EShanmenCombatActionPhase::Recovery
			&& ToPhase == EShanmenCombatActionPhase::Idle
			&& bCommitPointReached;

	case EShanmenActionTerminalReason::Cancelled:
		return FromPhase == EShanmenCombatActionPhase::Startup
			&& ToPhase == EShanmenCombatActionPhase::Cancelled
			&& !bCommitPointReached;

	case EShanmenActionTerminalReason::Interrupted:
		return IsRunningPhase(FromPhase)
			&& ToPhase == EShanmenCombatActionPhase::Interrupted
			&& bCommitPointReached == (FromPhase != EShanmenCombatActionPhase::Startup);
	}

	return false;
}

bool FShanmenActionOrchestrator::TryStart(
	const FShanmenCombatActionSnapshot& Action,
	FShanmenActionOrchestrator& OutOrchestrator,
	FShanmenActionTransitionReceipt& OutReceipt)
{
	const FShanmenCombatActionSnapshot FrozenAction = Action;
	OutOrchestrator.Reset();
	OutReceipt = FShanmenActionTransitionReceipt();
	if (!FrozenAction.IsValid())
	{
		return false;
	}

	OutOrchestrator.Action = FrozenAction;
	OutOrchestrator.bStarted = true;
	return OutOrchestrator.EmitTransition(
		EShanmenCombatActionPhase::Startup,
		EShanmenActionTerminalReason::None,
		false,
		OutReceipt);
}

bool FShanmenActionOrchestrator::IsValid() const
{
	if (!bStarted || !Action.IsValid() || NextSequence <= 0)
	{
		return false;
	}

	switch (TerminalReason)
	{
	case EShanmenActionTerminalReason::None:
		return IsRunningPhase(Phase)
			&& bCommitPointReached == (Phase != EShanmenCombatActionPhase::Startup);
	case EShanmenActionTerminalReason::Completed:
		return Phase == EShanmenCombatActionPhase::Idle && bCommitPointReached;
	case EShanmenActionTerminalReason::Cancelled:
		return Phase == EShanmenCombatActionPhase::Cancelled && !bCommitPointReached;
	case EShanmenActionTerminalReason::Interrupted:
		return Phase == EShanmenCombatActionPhase::Interrupted;
	}
	return false;
}

bool FShanmenActionOrchestrator::CanEmitCandidates() const
{
	return IsValid() && !IsTerminal() && Phase == EShanmenCombatActionPhase::Active;
}

bool FShanmenActionOrchestrator::TryAdvance(
	EShanmenCombatActionPhase ExpectedPhase,
	FShanmenActionTransitionReceipt& OutReceipt)
{
	OutReceipt = FShanmenActionTransitionReceipt();
	if (!IsValid() || IsTerminal() || ExpectedPhase != Phase)
	{
		return false;
	}

	switch (Phase)
	{
	case EShanmenCombatActionPhase::Startup:
		return EmitTransition(
			EShanmenCombatActionPhase::Active,
			EShanmenActionTerminalReason::None,
			true,
			OutReceipt);
	case EShanmenCombatActionPhase::Active:
		return EmitTransition(
			EShanmenCombatActionPhase::Recovery,
			EShanmenActionTerminalReason::None,
			false,
			OutReceipt);
	case EShanmenCombatActionPhase::Recovery:
		return EmitTransition(
			EShanmenCombatActionPhase::Idle,
			EShanmenActionTerminalReason::Completed,
			false,
			OutReceipt);
	default:
		return false;
	}
}

bool FShanmenActionOrchestrator::TryCancel(
	EShanmenCombatActionPhase ExpectedPhase,
	FShanmenActionTransitionReceipt& OutReceipt)
{
	OutReceipt = FShanmenActionTransitionReceipt();
	if (!IsValid()
		|| IsTerminal()
		|| ExpectedPhase != Phase
		|| Phase != EShanmenCombatActionPhase::Startup)
	{
		return false;
	}

	return EmitTransition(
		EShanmenCombatActionPhase::Cancelled,
		EShanmenActionTerminalReason::Cancelled,
		false,
		OutReceipt);
}

bool FShanmenActionOrchestrator::TryInterrupt(
	EShanmenCombatActionPhase ExpectedPhase,
	FShanmenActionTransitionReceipt& OutReceipt)
{
	OutReceipt = FShanmenActionTransitionReceipt();
	if (!IsValid()
		|| IsTerminal()
		|| ExpectedPhase != Phase
		|| !IsRunningPhase(Phase))
	{
		return false;
	}

	return EmitTransition(
		EShanmenCombatActionPhase::Interrupted,
		EShanmenActionTerminalReason::Interrupted,
		false,
		OutReceipt);
}

void FShanmenActionOrchestrator::Reset()
{
	*this = FShanmenActionOrchestrator();
}

bool FShanmenActionOrchestrator::EmitTransition(
	EShanmenCombatActionPhase ToPhase,
	EShanmenActionTerminalReason NewTerminalReason,
	bool bCrossCommitPoint,
	FShanmenActionTransitionReceipt& OutReceipt)
{
	const EShanmenCombatActionPhase FromPhase = Phase;
	if (bCrossCommitPoint)
	{
		bCommitPointReached = true;
	}

	Phase = ToPhase;
	TerminalReason = NewTerminalReason;
	OutReceipt.ActivationId = Action.GetActivationId();
	OutReceipt.Sequence = NextSequence++;
	OutReceipt.FromPhase = FromPhase;
	OutReceipt.ToPhase = ToPhase;
	OutReceipt.TerminalReason = NewTerminalReason;
	OutReceipt.bCrossedCommitPointNow = bCrossCommitPoint;
	OutReceipt.bCommitPointReached = bCommitPointReached;
	return OutReceipt.IsValid() && IsValid();
}
