#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "demo_mapShanmenSpiritEvasionProductHost.h"
#include "demo_mapShanmenSpiritEvasionComponent.generated.h"

enum class Edemo_mapShanmenSpiritEvasionComponentStartError : uint8
{
	None,
	ComponentBusy,
	OwnerUnavailable,
	WorldUnavailable,
	HostRejected
};

/** Product-component result that preserves the exact P10.5 host start proof. */
struct Fdemo_mapShanmenSpiritEvasionComponentStartResult
{
	Edemo_mapShanmenSpiritEvasionComponentStartError Error =
		Edemo_mapShanmenSpiritEvasionComponentStartError::OwnerUnavailable;
	Fdemo_mapShanmenSpiritEvasionHostStartResult HostStart;

	bool IsValid() const;
	bool IsSuccess() const;
};

/**
 * Thin Unreal lifecycle bridge for the sole P10.5 Spirit Evasion product host.
 *
 * The component owns no duplicate action or motion state. It reads the owner
 * Character and World time only at the call boundary, enables Tick only while
 * the host is active, and routes EndPlay to the host's owner-end transition.
 * Input/content/resource owners call TryStart with already-frozen snapshots.
 */
UCLASS(ClassGroup=(Gameplay), meta=(BlueprintSpawnableComponent))
class Udemo_mapShanmenSpiritEvasionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	Udemo_mapShanmenSpiritEvasionComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(
		const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	Fdemo_mapShanmenSpiritEvasionComponentStartResult TryStart(
		const FShanmenCombatActionSnapshot& Action,
		const FShanmenSpiritEvasionDefinition& Definition,
		const Fdemo_mapShanmenSpiritEvasionMovementPolicySnapshot& Policy,
		const Fdemo_mapShanmenSpiritEvasionTrajectorySnapshot& Trajectory,
		const FVector& CandidateDirection);

	Fdemo_mapShanmenSpiritEvasionHostStepResult TryCancel();
	Fdemo_mapShanmenSpiritEvasionHostStepResult TryInterrupt();
	Fdemo_mapShanmenSpiritEvasionHostStepResult TryOwnerEnd();
	Fdemo_mapShanmenSpiritEvasionHostStepResult TryFinishRecovery();

	bool HasHost() const { return Host.IsValid(); }
	bool IsActive() const { return Host.IsActive(); }
	bool IsRecovery() const { return Host.IsRecovery(); }
	bool IsTerminal() const { return Host.IsTerminal(); }
	bool RequiresExecutionTick() const { return Host.IsActive(); }
	bool CanStart() const { return !Host.IsValid() || Host.IsTerminal(); }
	const Fdemo_mapShanmenSpiritEvasionProductHost& GetHost() const
	{
		return Host;
	}
	const Fdemo_mapShanmenSpiritEvasionHostStepResult& GetLastStep() const
	{
		return LastStep;
	}

#if WITH_DEV_AUTOMATION_TESTS
	Fdemo_mapShanmenSpiritEvasionComponentStartResult TryStartAtForAutomation(
		const FShanmenCombatActionSnapshot& Action,
		const FShanmenSpiritEvasionDefinition& Definition,
		const Fdemo_mapShanmenSpiritEvasionMovementPolicySnapshot& Policy,
		const Fdemo_mapShanmenSpiritEvasionTrajectorySnapshot& Trajectory,
		const FVector& CandidateDirection,
		double StartTimeSeconds,
		Idemo_mapShanmenSpiritEvasionPreflightPort& PreflightPort);
	Fdemo_mapShanmenSpiritEvasionHostStepResult
	TryAdvanceAtForAutomation(
		double NowSeconds,
		Idemo_mapShanmenSpiritEvasionSegmentExecutionPort& ExecutionPort);
#endif

private:
	Fdemo_mapShanmenSpiritEvasionComponentStartResult TryStartWithPort(
		const FShanmenCombatActionSnapshot& Action,
		const FShanmenSpiritEvasionDefinition& Definition,
		const Fdemo_mapShanmenSpiritEvasionMovementPolicySnapshot& Policy,
		const Fdemo_mapShanmenSpiritEvasionTrajectorySnapshot& Trajectory,
		const FVector& CandidateDirection,
		double StartTimeSeconds,
		Idemo_mapShanmenSpiritEvasionPreflightPort& PreflightPort);
	Fdemo_mapShanmenSpiritEvasionHostStepResult RecordStep(
		const Fdemo_mapShanmenSpiritEvasionHostStepResult& Step);
	void RefreshTickEnabled();

	Fdemo_mapShanmenSpiritEvasionProductHost Host;
	Fdemo_mapShanmenSpiritEvasionHostStepResult LastStep;
};
