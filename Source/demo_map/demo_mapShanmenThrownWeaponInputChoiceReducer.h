#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenThrownWeaponRunCommandRouter.h"

/** One platform-neutral edit to the consumer-owned thrown-weapon choice. */
enum class Edemo_mapShanmenThrownWeaponInputChoiceCommandKind : uint8
{
	Invalid,
	SelectTrajectory,
	SetArcTargetIntent,
	AdjustArcApex,
	ClearArcTargetIntent
};

/**
 * Immutable, canonically normalized input command.
 *
 * Target intent is a two-axis value in the closed unit disc. Apex adjustment
 * is a signed normalized delta in [-1, 1]. ExpectedRevision makes an exact
 * command safe to replay without applying one physical input twice.
 */
class Fdemo_mapShanmenThrownWeaponInputChoiceCommand
{
public:
	static bool TryCaptureTrajectorySelection(
		uint64 ExpectedRevision,
		Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind TrajectoryKind,
		Fdemo_mapShanmenThrownWeaponInputChoiceCommand& OutCommand);
	static bool TryCaptureArcTargetIntent(
		uint64 ExpectedRevision,
		const FVector2D& RawTargetIntent,
		Fdemo_mapShanmenThrownWeaponInputChoiceCommand& OutCommand);
	static bool TryCaptureArcApexAdjustment(
		uint64 ExpectedRevision,
		double RawNormalizedDelta,
		Fdemo_mapShanmenThrownWeaponInputChoiceCommand& OutCommand);
	static bool TryCaptureArcTargetClear(
		uint64 ExpectedRevision,
		Fdemo_mapShanmenThrownWeaponInputChoiceCommand& OutCommand);

	bool IsValid() const;
	const FGuid& GetCommandId() const { return CommandId; }
	uint64 GetExpectedRevision() const { return ExpectedRevision; }
	Edemo_mapShanmenThrownWeaponInputChoiceCommandKind GetKind() const
	{
		return Kind;
	}
	Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind
	GetTrajectoryKind() const { return TrajectoryKind; }
	const FVector2D& GetArcTargetIntent() const { return ArcTargetIntent; }
	double GetArcApexAdjustmentDelta() const
	{
		return ArcApexAdjustmentDelta;
	}

private:
	FGuid CommandId;
	uint64 ExpectedRevision = MAX_uint64;
	Edemo_mapShanmenThrownWeaponInputChoiceCommandKind Kind =
		Edemo_mapShanmenThrownWeaponInputChoiceCommandKind::Invalid;
	Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind TrajectoryKind =
		Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::Invalid;
	FVector2D ArcTargetIntent = FVector2D::ZeroVector;
	double ArcApexAdjustmentDelta = 0.0;
};

/** Immutable consumer-local choice state; only the stateless reducer advances it. */
class Fdemo_mapShanmenThrownWeaponInputChoiceState
{
public:
	static Fdemo_mapShanmenThrownWeaponInputChoiceState CreateInitial();
	static bool TryRehydrate(
		const FGuid& ExpectedStateId,
		const FGuid& LastCommandId,
		uint64 Revision,
		Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind TrajectoryKind,
		bool bHasArcTargetIntent,
		const FVector2D& ArcTargetIntent,
		double ArcApexAdjustment,
		Fdemo_mapShanmenThrownWeaponInputChoiceState& OutState);

	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenThrownWeaponInputChoiceState& Other) const;
	const FGuid& GetStateId() const { return StateId; }
	const FGuid& GetLastCommandId() const { return LastCommandId; }
	uint64 GetRevision() const { return Revision; }
	Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind
	GetTrajectoryKind() const { return TrajectoryKind; }
	bool HasArcTargetIntent() const { return bHasArcTargetIntent; }
	const FVector2D& GetArcTargetIntent() const { return ArcTargetIntent; }
	double GetArcApexAdjustment() const { return ArcApexAdjustment; }

private:
	friend class Fdemo_mapShanmenThrownWeaponInputChoiceReducer;

	FGuid StateId;
	FGuid LastCommandId;
	uint64 Revision = MAX_uint64;
	Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind TrajectoryKind =
		Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::Invalid;
	bool bHasArcTargetIntent = false;
	FVector2D ArcTargetIntent = FVector2D::ZeroVector;
	double ArcApexAdjustment = 0.0;
};

enum class Edemo_mapShanmenThrownWeaponInputChoiceReduceStatus : uint8
{
	Reduced,
	NoChange,
	Replay,
	StateInvalid,
	CommandInvalid,
	RevisionMismatch,
	RevisionExhausted,
	ModeMismatch,
	StateRejected
};

struct Fdemo_mapShanmenThrownWeaponInputChoiceReduceResult
{
	Edemo_mapShanmenThrownWeaponInputChoiceReduceStatus Status =
		Edemo_mapShanmenThrownWeaponInputChoiceReduceStatus::StateInvalid;
	FString Diagnostic;
	Fdemo_mapShanmenThrownWeaponInputChoiceState State;

	bool IsSuccess() const;
	bool DidChange() const
	{
		return Status
			== Edemo_mapShanmenThrownWeaponInputChoiceReduceStatus::Reduced;
	}
	bool IsReplay() const
	{
		return Status
			== Edemo_mapShanmenThrownWeaponInputChoiceReduceStatus::Replay;
	}
};

/** Pure reducer; the caller owns the previous state and accepts only success. */
class Fdemo_mapShanmenThrownWeaponInputChoiceReducer
{
public:
	static Fdemo_mapShanmenThrownWeaponInputChoiceReduceResult Reduce(
		const Fdemo_mapShanmenThrownWeaponInputChoiceState& PreviousState,
		const Fdemo_mapShanmenThrownWeaponInputChoiceCommand& Command);
};
