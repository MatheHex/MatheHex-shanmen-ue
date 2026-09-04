#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenThrownWeaponInputChoiceReducer.h"

enum class Edemo_mapShanmenThrownWeaponInputChoiceSessionStatus : uint8
{
	Applied,
	NoChange,
	Replay,
	CombatRunActive,
	ProductLifecycleNotEmpty,
	ReductionRejected
};

/** Result of one consumer-owned input-choice submission. */
struct Fdemo_mapShanmenThrownWeaponInputChoiceSessionResult
{
	Edemo_mapShanmenThrownWeaponInputChoiceSessionStatus Status =
		Edemo_mapShanmenThrownWeaponInputChoiceSessionStatus::
			ReductionRejected;
	Edemo_mapShanmenThrownWeaponInputChoiceReduceStatus ReduceStatus =
		Edemo_mapShanmenThrownWeaponInputChoiceReduceStatus::StateInvalid;
	FString Diagnostic;
	Fdemo_mapShanmenThrownWeaponInputChoiceState State;

	bool IsSuccess() const;
	bool DidChange() const
	{
		return Status
			== Edemo_mapShanmenThrownWeaponInputChoiceSessionStatus::Applied;
	}
	bool IsReplay() const
	{
		return Status
			== Edemo_mapShanmenThrownWeaponInputChoiceSessionStatus::Replay;
	}
};

/**
 * Consumer-owned choice state between combat Runs.
 *
 * The reducer remains the only state transition authority. This session adds
 * product lifetime fences: a command that would change state is rejected while
 * the combat Run is active or the thrown-weapon lifecycle is not empty. Exact
 * replay and fresh no-op acknowledgements remain safe while locked.
 */
class Fdemo_mapShanmenThrownWeaponInputChoiceSession
{
public:
	Fdemo_mapShanmenThrownWeaponInputChoiceSession();

	Fdemo_mapShanmenThrownWeaponInputChoiceSessionResult Submit(
		const Fdemo_mapShanmenThrownWeaponInputChoiceCommand& Command,
		bool bCombatRunActive,
		bool bProductLifecycleEmpty);

	bool IsValid() const { return State.IsValid(); }
	const Fdemo_mapShanmenThrownWeaponInputChoiceState& GetState() const
	{
		return State;
	}
	Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind
	GetTrajectoryKind() const
	{
		return State.GetTrajectoryKind();
	}

private:
	Fdemo_mapShanmenThrownWeaponInputChoiceState State;
};
