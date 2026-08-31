#include "demo_mapShanmenPlayerActionArbitration.h"

#include "ShanmenCombatResolver.h"

namespace
{
	bool IsValidAction(Edemo_mapShanmenPlayerActionKind Action)
	{
		switch (Action)
		{
		case Edemo_mapShanmenPlayerActionKind::BasicSword:
		case Edemo_mapShanmenPlayerActionKind::ThrownWeapon:
		case Edemo_mapShanmenPlayerActionKind::SpiritEvasion:
		case Edemo_mapShanmenPlayerActionKind::WeaponGuard:
			return true;
		default:
			return false;
		}
	}

	bool IsPersistentOccupant(Edemo_mapShanmenPlayerActionKind Action)
	{
		return Action == Edemo_mapShanmenPlayerActionKind::ThrownWeapon
			|| Action == Edemo_mapShanmenPlayerActionKind::SpiritEvasion
			|| Action == Edemo_mapShanmenPlayerActionKind::WeaponGuard;
	}

	Fdemo_mapShanmenPlayerActionArbitrationReceipt RejectWithoutIdentity(
		Edemo_mapShanmenPlayerActionArbitrationError Error,
		Edemo_mapShanmenPlayerActionKind RequestedAction,
		const TCHAR* Diagnostic)
	{
		Fdemo_mapShanmenPlayerActionArbitrationReceipt Result;
		Result.Error = Error;
		Result.RequestedAction = RequestedAction;
		Result.Diagnostic = Diagnostic;
		return Result;
	}

	bool HasCommandIdentity(
		const Fdemo_mapShanmenPlayerActionArbitrationReceipt& Receipt)
	{
		return Receipt.CommandSequence > 0
			&& Receipt.RunId.IsValid()
			&& Receipt.PlayerEntityId.IsValid()
			&& Receipt.CommandId.IsValid()
			&& Receipt.CommandId == FShanmenCombatIdFactory::MakeActivationId(
				Receipt.RunId,
				Receipt.PlayerEntityId,
				Fdemo_mapShanmenPlayerActionArbitrationPolicy::
					CanonicalCommandDefinitionId(),
				Receipt.CommandSequence);
	}
}

bool Fdemo_mapShanmenPlayerActionClaim::TryCreate(
	const Edemo_mapShanmenPlayerActionKind OwningAction,
	const FGuid& OwnerId,
	const Edemo_mapShanmenPlayerActionClaimPreemption Preemption,
	Fdemo_mapShanmenPlayerActionClaim& OutClaim)
{
	Fdemo_mapShanmenPlayerActionClaim Candidate;
	Candidate.OwningAction = OwningAction;
	Candidate.OwnerId = OwnerId;
	Candidate.Preemption = Preemption;
	if (!Candidate.IsValid())
	{
		return false;
	}
	OutClaim = Candidate;
	return true;
}

bool Fdemo_mapShanmenPlayerActionClaim::IsValid() const
{
	if (!IsPersistentOccupant(OwningAction) || !OwnerId.IsValid())
	{
		return false;
	}
	if (OwningAction == Edemo_mapShanmenPlayerActionKind::WeaponGuard)
	{
		return Preemption
			== Edemo_mapShanmenPlayerActionClaimPreemption::ExactOwner;
	}
	return Preemption == Edemo_mapShanmenPlayerActionClaimPreemption::None;
}

bool Fdemo_mapShanmenPlayerActionClaim::RequiresExactOwnerPreemption() const
{
	return IsValid()
		&& Preemption
			== Edemo_mapShanmenPlayerActionClaimPreemption::ExactOwner;
}

bool Fdemo_mapShanmenPlayerActionOccupancySnapshot::TryRegisterClaim(
	const Edemo_mapShanmenPlayerActionKind OwningAction,
	const FGuid& OwnerId,
	const Edemo_mapShanmenPlayerActionClaimPreemption Preemption)
{
	Fdemo_mapShanmenPlayerActionClaim Claim;
	if (!bProjectionValid
		|| !Fdemo_mapShanmenPlayerActionClaim::TryCreate(
			OwningAction,
			OwnerId,
			Preemption,
			Claim))
	{
		bProjectionValid = false;
		return false;
	}
	for (const Fdemo_mapShanmenPlayerActionClaim& Existing : Claims)
	{
		if (Existing.OwningAction == OwningAction)
		{
			bProjectionValid = false;
			return false;
		}
	}
	Claims.Add(Claim);
	return true;
}

void Fdemo_mapShanmenPlayerActionOccupancySnapshot::Invalidate()
{
	bProjectionValid = false;
}

bool Fdemo_mapShanmenPlayerActionOccupancySnapshot::IsValid() const
{
	if (!bProjectionValid)
	{
		return false;
	}
	for (int32 Index = 0; Index < Claims.Num(); ++Index)
	{
		if (!Claims[Index].IsValid())
		{
			return false;
		}
		for (int32 OtherIndex = Index + 1;
			OtherIndex < Claims.Num();
			++OtherIndex)
		{
			if (Claims[Index].OwningAction == Claims[OtherIndex].OwningAction)
			{
				return false;
			}
		}
	}
	return true;
}

int32 Fdemo_mapShanmenPlayerActionOccupancySnapshot::
	NumOccupiedProducts() const
{
	return Claims.Num();
}

const Fdemo_mapShanmenPlayerActionClaim*
Fdemo_mapShanmenPlayerActionOccupancySnapshot::GetSoleClaim() const
{
	return IsValid() && Claims.Num() == 1 ? &Claims[0] : nullptr;
}

bool Fdemo_mapShanmenPlayerActionArbitrationReceipt::IsValid() const
{
	const bool bHasIdentity = HasCommandIdentity(*this);
	const bool bHasOccupant = IsPersistentOccupant(OccupyingAction)
		&& OccupyingOwnerId.IsValid();
	const bool bHasNoOccupant = OccupyingAction
			== Edemo_mapShanmenPlayerActionKind::None
		&& !OccupyingOwnerId.IsValid();
	if (Status == Edemo_mapShanmenPlayerActionArbitrationStatus::Rejected)
	{
		if (Error == Edemo_mapShanmenPlayerActionArbitrationError::None
			|| (!bHasOccupant && !bHasNoOccupant))
		{
			return false;
		}
		if (Error == Edemo_mapShanmenPlayerActionArbitrationError::
			MultipleActiveProducts)
		{
			return IsValidAction(RequestedAction)
				&& bHasIdentity
				&& bHasNoOccupant;
		}
		if (Error == Edemo_mapShanmenPlayerActionArbitrationError::
			ConflictingProductActive)
		{
			return IsValidAction(RequestedAction)
				&& bHasIdentity
				&& bHasOccupant
				&& OccupyingAction
					!= Edemo_mapShanmenPlayerActionKind::WeaponGuard;
		}
		return !bHasIdentity
				&& CommandSequence == 0
				&& !RunId.IsValid()
				&& !PlayerEntityId.IsValid()
				&& !CommandId.IsValid()
				&& bHasNoOccupant;
	}
	if (Error != Edemo_mapShanmenPlayerActionArbitrationError::None
		|| !IsValidAction(RequestedAction)
		|| !bHasIdentity)
	{
		return false;
	}
	if (Status
		== Edemo_mapShanmenPlayerActionArbitrationStatus::Granted)
	{
		return bHasNoOccupant;
	}
	if (OccupyingAction != Edemo_mapShanmenPlayerActionKind::WeaponGuard
		|| !OccupyingOwnerId.IsValid())
	{
		return false;
	}
	if (Status
		== Edemo_mapShanmenPlayerActionArbitrationStatus::AlreadyActive)
	{
		return RequestedAction
			== Edemo_mapShanmenPlayerActionKind::WeaponGuard;
	}
	return Status
			== Edemo_mapShanmenPlayerActionArbitrationStatus::
				WeaponGuardPreemptionRequired
		&& RequestedAction
			!= Edemo_mapShanmenPlayerActionKind::WeaponGuard;
}

bool Fdemo_mapShanmenPlayerActionArbitrationReceipt::IsAuthorized() const
{
	return IsValid()
		&& Status != Edemo_mapShanmenPlayerActionArbitrationStatus::Rejected;
}

bool Fdemo_mapShanmenPlayerActionArbitrationReceipt::
	RequiresWeaponGuardPreemption() const
{
	return IsAuthorized()
		&& Status == Edemo_mapShanmenPlayerActionArbitrationStatus::
			WeaponGuardPreemptionRequired
		&& OccupyingAction == Edemo_mapShanmenPlayerActionKind::WeaponGuard
		&& OccupyingOwnerId.IsValid();
}

Fdemo_mapShanmenPlayerActionGateResult
Fdemo_mapShanmenPlayerActionGateResult::FromArbitration(
	const Fdemo_mapShanmenPlayerActionArbitrationReceipt& Receipt)
{
	Fdemo_mapShanmenPlayerActionGateResult Result;
	Result.Arbitration = Receipt;
	if (!Receipt.IsValid())
	{
		Result.Error = Edemo_mapShanmenPlayerActionGateError::
			StateDesynchronized;
		Result.Diagnostic = TEXT("Player-action arbitration receipt is invalid.");
	}
	else if (!Receipt.IsAuthorized())
	{
		Result.Diagnostic = Receipt.Diagnostic;
	}
	else if (Receipt.RequiresWeaponGuardPreemption())
	{
		Result.Error = Edemo_mapShanmenPlayerActionGateError::
			StateDesynchronized;
		Result.Diagnostic =
			TEXT("Player-action route omitted required weapon-guard preemption.");
	}
	else
	{
		Result.Status = Edemo_mapShanmenPlayerActionGateStatus::Authorized;
		Result.Error = Edemo_mapShanmenPlayerActionGateError::None;
		Result.Diagnostic = TEXT("Player action owns the sole product lane.");
	}
	return Result;
}

Fdemo_mapShanmenPlayerActionGateResult
Fdemo_mapShanmenPlayerActionGateResult::FromGuardPreemption(
	const Fdemo_mapShanmenPlayerActionArbitrationReceipt& Receipt,
	const FGuid& RetiredHostId)
{
	Fdemo_mapShanmenPlayerActionGateResult Result;
	Result.Arbitration = Receipt;
	Result.RetiredWeaponGuardHostId = RetiredHostId;
	if (Receipt.IsValid()
		&& Receipt.RequiresWeaponGuardPreemption()
		&& RetiredHostId == Receipt.OccupyingOwnerId)
	{
		Result.Status =
			Edemo_mapShanmenPlayerActionGateStatus::WeaponGuardPreempted;
		Result.Error = Edemo_mapShanmenPlayerActionGateError::None;
		Result.Diagnostic =
			TEXT("Player action retired the exact active weapon-guard Host.");
	}
	else
	{
		Result.RetiredWeaponGuardHostId.Invalidate();
		Result.Error = Edemo_mapShanmenPlayerActionGateError::
			StateDesynchronized;
		Result.Diagnostic =
			TEXT("Weapon-guard preemption did not match its arbitration receipt.");
	}
	return Result;
}

Fdemo_mapShanmenPlayerActionGateResult
Fdemo_mapShanmenPlayerActionGateResult::RejectGuardPreemption(
	const Fdemo_mapShanmenPlayerActionArbitrationReceipt& Receipt,
	const TCHAR* Diagnostic)
{
	Fdemo_mapShanmenPlayerActionGateResult Result;
	Result.Arbitration = Receipt;
	Result.Error = Edemo_mapShanmenPlayerActionGateError::
		WeaponGuardPreemptionRejected;
	Result.Diagnostic = Diagnostic;
	return Result;
}

bool Fdemo_mapShanmenPlayerActionGateResult::IsValid() const
{
	if (!Arbitration.IsValid())
	{
		return false;
	}
	switch (Status)
	{
	case Edemo_mapShanmenPlayerActionGateStatus::Authorized:
		return Error == Edemo_mapShanmenPlayerActionGateError::None
			&& Arbitration.IsAuthorized()
			&& !Arbitration.RequiresWeaponGuardPreemption()
			&& !RetiredWeaponGuardHostId.IsValid();
	case Edemo_mapShanmenPlayerActionGateStatus::WeaponGuardPreempted:
		return Error == Edemo_mapShanmenPlayerActionGateError::None
			&& Arbitration.RequiresWeaponGuardPreemption()
			&& RetiredWeaponGuardHostId.IsValid()
			&& RetiredWeaponGuardHostId == Arbitration.OccupyingOwnerId;
	case Edemo_mapShanmenPlayerActionGateStatus::Rejected:
		return Error != Edemo_mapShanmenPlayerActionGateError::None
			&& !RetiredWeaponGuardHostId.IsValid();
	default:
		return false;
	}
}

bool Fdemo_mapShanmenPlayerActionGateResult::IsAuthorized() const
{
	return IsValid()
		&& Status != Edemo_mapShanmenPlayerActionGateStatus::Rejected;
}

FName Fdemo_mapShanmenPlayerActionArbitrationPolicy::
	CanonicalCommandDefinitionId()
{
	return TEXT("Action.Player.ExclusiveLane.Arbitration.r1");
}

Fdemo_mapShanmenPlayerActionArbitrationReceipt
Fdemo_mapShanmenPlayerActionArbitrationPolicy::Evaluate(
	const FGuid& RunId,
	const FGuid& PlayerEntityId,
	const uint64 CommandSequence,
	const Edemo_mapShanmenPlayerActionKind RequestedAction,
	const Fdemo_mapShanmenPlayerActionOccupancySnapshot& Occupancy)
{
	if (!RunId.IsValid() || !PlayerEntityId.IsValid()
		|| CommandSequence == 0 || CommandSequence == MAX_uint64)
	{
		return RejectWithoutIdentity(
			Edemo_mapShanmenPlayerActionArbitrationError::
				IdentityConstructionFailed,
			RequestedAction,
			TEXT("Player-action arbitration requires valid Run identity and sequence."));
	}
	if (!IsValidAction(RequestedAction))
	{
		return RejectWithoutIdentity(
			Edemo_mapShanmenPlayerActionArbitrationError::
				InvalidRequestedAction,
			RequestedAction,
			TEXT("Player-action arbitration rejected an unknown action kind."));
	}
	if (!Occupancy.IsValid())
	{
		return RejectWithoutIdentity(
			Edemo_mapShanmenPlayerActionArbitrationError::
				InvalidOccupancySnapshot,
			RequestedAction,
			TEXT("Player-action occupancy snapshot is structurally invalid."));
	}

	Fdemo_mapShanmenPlayerActionArbitrationReceipt Result;
	Result.RequestedAction = RequestedAction;
	Result.CommandSequence = CommandSequence;
	Result.RunId = RunId;
	Result.PlayerEntityId = PlayerEntityId;
	Result.CommandId = FShanmenCombatIdFactory::MakeActivationId(
		RunId,
		PlayerEntityId,
		CanonicalCommandDefinitionId(),
		CommandSequence);
	if (!Result.CommandId.IsValid())
	{
		return RejectWithoutIdentity(
			Edemo_mapShanmenPlayerActionArbitrationError::
				IdentityConstructionFailed,
			RequestedAction,
			TEXT("Player-action arbitration identity failed closed."));
	}

	if (Occupancy.NumOccupiedProducts() > 1)
	{
		Result.Error = Edemo_mapShanmenPlayerActionArbitrationError::
			MultipleActiveProducts;
		Result.Diagnostic =
			TEXT("Multiple product Hosts already occupy the player action lane.");
		return Result;
	}

	const Fdemo_mapShanmenPlayerActionClaim* Claim =
		Occupancy.GetSoleClaim();
	if (Claim != nullptr)
	{
		Result.OccupyingAction = Claim->OwningAction;
		Result.OccupyingOwnerId = Claim->OwnerId;
		if (Claim->OwningAction
			== Edemo_mapShanmenPlayerActionKind::WeaponGuard)
		{
			if (!Claim->RequiresExactOwnerPreemption())
			{
				return RejectWithoutIdentity(
					Edemo_mapShanmenPlayerActionArbitrationError::
						InvalidOccupancySnapshot,
					RequestedAction,
					TEXT("Weapon-guard claim omitted exact-owner preemption policy."));
			}
			if (RequestedAction
				== Edemo_mapShanmenPlayerActionKind::WeaponGuard)
			{
				Result.Status =
					Edemo_mapShanmenPlayerActionArbitrationStatus::
						AlreadyActive;
				Result.Error =
					Edemo_mapShanmenPlayerActionArbitrationError::None;
				Result.Diagnostic =
					TEXT("The exact weapon-guard Host already owns the action lane.");
			}
			else
			{
				Result.Status =
					Edemo_mapShanmenPlayerActionArbitrationStatus::
						WeaponGuardPreemptionRequired;
				Result.Error =
					Edemo_mapShanmenPlayerActionArbitrationError::None;
				Result.Diagnostic =
					TEXT("The requested action must retire the exact weapon-guard Host first.");
			}
		}
		else
		{
			Result.Error = Edemo_mapShanmenPlayerActionArbitrationError::
				ConflictingProductActive;
			Result.Diagnostic = Claim->OwningAction
					== Edemo_mapShanmenPlayerActionKind::ThrownWeapon
				? TEXT("An in-flight thrown weapon owns the player action lane.")
				: TEXT("A nonterminal Spirit Evasion owns the player action lane.");
		}
		return Result;
	}

	Result.Status = Edemo_mapShanmenPlayerActionArbitrationStatus::Granted;
	Result.Error = Edemo_mapShanmenPlayerActionArbitrationError::None;
	Result.Diagnostic = TEXT("The player action lane is unoccupied.");
	return Result;
}
