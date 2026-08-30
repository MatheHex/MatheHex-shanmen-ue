#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenSpiritEvasionComponent.h"

class ACharacter;
class Fdemo_mapCombatRunCoordinator;

/** Device-independent operations accepted by the Spirit Evasion product seam. */
enum class Edemo_mapShanmenSpiritEvasionCommandKind : uint8
{
	Start,
	Cancel,
	Interrupt,
	FinishRecovery
};

/**
 * Frozen command payload. Start consumes identities and policy selected by
 * upstream authorities; lifecycle signals deliberately carry no second copy.
 */
class Fdemo_mapShanmenSpiritEvasionCommand
{
public:
	static bool TryCaptureStart(
		const FShanmenCombatActionSnapshot& Action,
		const FShanmenSpiritEvasionDefinition& Definition,
		const Fdemo_mapShanmenSpiritEvasionMovementPolicySnapshot& Policy,
		const Fdemo_mapShanmenSpiritEvasionTrajectorySnapshot& Trajectory,
		const FVector& CandidateDirection,
		Fdemo_mapShanmenSpiritEvasionCommand& OutCommand);
	static Fdemo_mapShanmenSpiritEvasionCommand MakeCancel();
	static Fdemo_mapShanmenSpiritEvasionCommand MakeInterrupt();
	static Fdemo_mapShanmenSpiritEvasionCommand MakeFinishRecovery();

	bool IsValid() const;
	Edemo_mapShanmenSpiritEvasionCommandKind GetKind() const { return Kind; }
	const FShanmenCombatActionSnapshot& GetAction() const { return Action; }
	const FShanmenSpiritEvasionDefinition& GetDefinition() const
	{
		return Definition;
	}
	const Fdemo_mapShanmenSpiritEvasionMovementPolicySnapshot& GetPolicy() const
	{
		return Policy;
	}
	const Fdemo_mapShanmenSpiritEvasionTrajectorySnapshot& GetTrajectory() const
	{
		return Trajectory;
	}
	const FVector& GetCandidateDirection() const
	{
		return CandidateDirection;
	}
	const FGuid& GetActivationId() const
	{
		return Action.GetActivationId();
	}

private:
	static Fdemo_mapShanmenSpiritEvasionCommand MakeSignal(
		Edemo_mapShanmenSpiritEvasionCommandKind Kind);

	Edemo_mapShanmenSpiritEvasionCommandKind Kind =
		Edemo_mapShanmenSpiritEvasionCommandKind::Start;
	FShanmenCombatActionSnapshot Action;
	FShanmenSpiritEvasionDefinition Definition;
	Fdemo_mapShanmenSpiritEvasionMovementPolicySnapshot Policy;
	Fdemo_mapShanmenSpiritEvasionTrajectorySnapshot Trajectory;
	FVector CandidateDirection = FVector::ZeroVector;
};

enum class Edemo_mapShanmenSpiritEvasionInstallationStatus : uint8
{
	Installed,
	AlreadyInstalled,
	OwnerUnavailable,
	DuplicateComponents,
	CreationFailed,
	RegistrationFailed
};

/** Result of the idempotent one-component installation boundary. */
struct Fdemo_mapShanmenSpiritEvasionInstallationResult
{
	Edemo_mapShanmenSpiritEvasionInstallationStatus Status =
		Edemo_mapShanmenSpiritEvasionInstallationStatus::OwnerUnavailable;
	Udemo_mapShanmenSpiritEvasionComponent* Component = nullptr;
	int32 ExistingComponentCount = 0;

	bool IsSuccess() const;
};

enum class Edemo_mapShanmenSpiritEvasionCommandStatus : uint8
{
	Applied,
	CommandInvalid,
	ComponentUnavailable,
	ComponentOwnerMismatch,
	CoordinatorNotReady,
	RunMismatch,
	SourceMismatch,
	ComponentRejected
};

/** Typed dispatch result retaining the exact component/host proof. */
struct Fdemo_mapShanmenSpiritEvasionCommandResult
{
	Edemo_mapShanmenSpiritEvasionCommandStatus Status =
		Edemo_mapShanmenSpiritEvasionCommandStatus::CommandInvalid;
	Edemo_mapShanmenSpiritEvasionCommandKind Kind =
		Edemo_mapShanmenSpiritEvasionCommandKind::Start;
	FGuid ActivationId;
	Fdemo_mapShanmenSpiritEvasionComponentStartResult Start;
	Fdemo_mapShanmenSpiritEvasionHostStepResult Step;
	FString Diagnostic;

	bool IsAccepted() const;
};

/**
 * Unique installation and typed dispatch seam for the P10.6 component.
 *
 * Start is admitted only when its frozen action names the active product Run
 * and the registered player entity. Control signals remain available during
 * teardown even after the Run coordinator has closed. The router owns no
 * input binding, command ledger, action identity, content or resource state.
 */
class Fdemo_mapShanmenSpiritEvasionCommandRouter
{
public:
	static Fdemo_mapShanmenSpiritEvasionInstallationResult EnsureInstalled(
		ACharacter* Owner);

	static Fdemo_mapShanmenSpiritEvasionCommandResult TryRoute(
		Udemo_mapShanmenSpiritEvasionComponent* Component,
		const Fdemo_mapCombatRunCoordinator& Coordinator,
		ACharacter* Owner,
		const Fdemo_mapShanmenSpiritEvasionCommand& Command);

#if WITH_DEV_AUTOMATION_TESTS
	static Fdemo_mapShanmenSpiritEvasionCommandResult TryRouteAtForAutomation(
		Udemo_mapShanmenSpiritEvasionComponent* Component,
		const Fdemo_mapCombatRunCoordinator& Coordinator,
		ACharacter* Owner,
		const Fdemo_mapShanmenSpiritEvasionCommand& Command,
		double StartTimeSeconds,
		Idemo_mapShanmenSpiritEvasionPreflightPort& PreflightPort);
#endif
};
