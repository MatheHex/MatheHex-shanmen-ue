#include "demo_mapShanmenSpiritEvasionComponent.h"

#include "demo_map.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"

namespace
{
	Fdemo_mapShanmenSpiritEvasionComponentStartResult RejectComponentStart(
		Edemo_mapShanmenSpiritEvasionComponentStartError Error,
		const Fdemo_mapShanmenSpiritEvasionHostStartResult& HostStart =
			Fdemo_mapShanmenSpiritEvasionHostStartResult())
	{
		Fdemo_mapShanmenSpiritEvasionComponentStartResult Result;
		Result.Error = Error;
		Result.HostStart = HostStart;
		return Result;
	}
}

bool Fdemo_mapShanmenSpiritEvasionComponentStartResult::IsValid() const
{
	if (Error == Edemo_mapShanmenSpiritEvasionComponentStartError::None)
	{
		return HostStart.IsSuccess();
	}
	if (Error == Edemo_mapShanmenSpiritEvasionComponentStartError::HostRejected)
	{
		return HostStart.IsValid() && !HostStart.IsSuccess();
	}
	return Error
			!= Edemo_mapShanmenSpiritEvasionComponentStartError::None
		&& !HostStart.IsSuccess();
}

bool Fdemo_mapShanmenSpiritEvasionComponentStartResult::IsSuccess() const
{
	return IsValid()
		&& Error == Edemo_mapShanmenSpiritEvasionComponentStartError::None;
}

Udemo_mapShanmenSpiritEvasionComponent::
Udemo_mapShanmenSpiritEvasionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void Udemo_mapShanmenSpiritEvasionComponent::BeginPlay()
{
	Super::BeginPlay();
	RefreshTickEnabled();
}

void Udemo_mapShanmenSpiritEvasionComponent::EndPlay(
	const EEndPlayReason::Type EndPlayReason)
{
	if (Host.IsValid() && !Host.IsTerminal())
	{
		LastStep = Host.TryOwnerEnd();
	}
	SetComponentTickEnabled(false);
	Super::EndPlay(EndPlayReason);
}

void Udemo_mapShanmenSpiritEvasionComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!Host.IsActive())
	{
		RefreshTickEnabled();
		return;
	}

	ACharacter* Character = Cast<ACharacter>(GetOwner());
	UWorld* World = GetWorld();
	if (!IsValid(Character) || World == nullptr || World->bIsTearingDown)
	{
		RecordStep(Host.TryOwnerEnd());
		return;
	}

	const Fdemo_mapShanmenSpiritEvasionHostStepResult Step =
		Host.TryAdvanceCharacter(
			static_cast<double>(World->GetTimeSeconds()),
			Character);
	RecordStep(Step);
	if (!Step.IsSuccess())
	{
		UE_LOG(
			Logdemo_map,
			Warning,
			TEXT("0.0.10 P10.6: Spirit Evasion component tick rejected; error=%d."),
			static_cast<int32>(Step.Error));
	}
}

Fdemo_mapShanmenSpiritEvasionComponentStartResult
Udemo_mapShanmenSpiritEvasionComponent::TryStart(
	const FShanmenCombatActionSnapshot& Action,
	const FShanmenSpiritEvasionDefinition& Definition,
	const Fdemo_mapShanmenSpiritEvasionMovementPolicySnapshot& Policy,
	const Fdemo_mapShanmenSpiritEvasionTrajectorySnapshot& Trajectory,
	const FVector& CandidateDirection)
{
	if (!CanStart())
	{
		return RejectComponentStart(
			Edemo_mapShanmenSpiritEvasionComponentStartError::ComponentBusy);
	}
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!IsValid(Character))
	{
		return RejectComponentStart(
			Edemo_mapShanmenSpiritEvasionComponentStartError::OwnerUnavailable);
	}
	const UWorld* World = GetWorld();
	if (World == nullptr || World->bIsTearingDown)
	{
		return RejectComponentStart(
			Edemo_mapShanmenSpiritEvasionComponentStartError::WorldUnavailable);
	}

	Fdemo_mapShanmenSpiritEvasionCharacterPreflightPort Preflight(Character);
	return TryStartWithPort(
		Action,
		Definition,
		Policy,
		Trajectory,
		CandidateDirection,
		static_cast<double>(World->GetTimeSeconds()),
		Preflight);
}

Fdemo_mapShanmenSpiritEvasionComponentStartResult
Udemo_mapShanmenSpiritEvasionComponent::TryStartWithPort(
	const FShanmenCombatActionSnapshot& Action,
	const FShanmenSpiritEvasionDefinition& Definition,
	const Fdemo_mapShanmenSpiritEvasionMovementPolicySnapshot& Policy,
	const Fdemo_mapShanmenSpiritEvasionTrajectorySnapshot& Trajectory,
	const FVector& CandidateDirection,
	double StartTimeSeconds,
	Idemo_mapShanmenSpiritEvasionPreflightPort& PreflightPort)
{
	if (!CanStart())
	{
		return RejectComponentStart(
			Edemo_mapShanmenSpiritEvasionComponentStartError::ComponentBusy);
	}

	Fdemo_mapShanmenSpiritEvasionProductHost Candidate;
	const Fdemo_mapShanmenSpiritEvasionHostStartResult HostStart =
		Fdemo_mapShanmenSpiritEvasionProductHost::TryStart(
			Action,
			Definition,
			Policy,
			Trajectory,
			CandidateDirection,
			StartTimeSeconds,
			PreflightPort,
			Candidate);
	if (!HostStart.IsSuccess())
	{
		return RejectComponentStart(
			Edemo_mapShanmenSpiritEvasionComponentStartError::HostRejected,
			HostStart);
	}

	Host = MoveTemp(Candidate);
	LastStep = Fdemo_mapShanmenSpiritEvasionHostStepResult();
	RefreshTickEnabled();
	Fdemo_mapShanmenSpiritEvasionComponentStartResult Result;
	Result.Error = Edemo_mapShanmenSpiritEvasionComponentStartError::None;
	Result.HostStart = HostStart;
	return Result;
}

Fdemo_mapShanmenSpiritEvasionHostStepResult
Udemo_mapShanmenSpiritEvasionComponent::RecordStep(
	const Fdemo_mapShanmenSpiritEvasionHostStepResult& Step)
{
	LastStep = Step;
	RefreshTickEnabled();
	return LastStep;
}

void Udemo_mapShanmenSpiritEvasionComponent::RefreshTickEnabled()
{
	SetComponentTickEnabled(RequiresExecutionTick());
}

Fdemo_mapShanmenSpiritEvasionHostStepResult
Udemo_mapShanmenSpiritEvasionComponent::TryCancel()
{
	return RecordStep(Host.TryCancel());
}

Fdemo_mapShanmenSpiritEvasionHostStepResult
Udemo_mapShanmenSpiritEvasionComponent::TryInterrupt()
{
	return RecordStep(Host.TryInterrupt());
}

Fdemo_mapShanmenSpiritEvasionHostStepResult
Udemo_mapShanmenSpiritEvasionComponent::TryOwnerEnd()
{
	return RecordStep(Host.TryOwnerEnd());
}

Fdemo_mapShanmenSpiritEvasionHostStepResult
Udemo_mapShanmenSpiritEvasionComponent::TryFinishRecovery()
{
	return RecordStep(Host.TryFinishRecovery());
}

#if WITH_DEV_AUTOMATION_TESTS
Fdemo_mapShanmenSpiritEvasionComponentStartResult
Udemo_mapShanmenSpiritEvasionComponent::TryStartAtForAutomation(
	const FShanmenCombatActionSnapshot& Action,
	const FShanmenSpiritEvasionDefinition& Definition,
	const Fdemo_mapShanmenSpiritEvasionMovementPolicySnapshot& Policy,
	const Fdemo_mapShanmenSpiritEvasionTrajectorySnapshot& Trajectory,
	const FVector& CandidateDirection,
	double StartTimeSeconds,
	Idemo_mapShanmenSpiritEvasionPreflightPort& PreflightPort)
{
	return TryStartWithPort(
		Action,
		Definition,
		Policy,
		Trajectory,
		CandidateDirection,
		StartTimeSeconds,
		PreflightPort);
}

Fdemo_mapShanmenSpiritEvasionHostStepResult
Udemo_mapShanmenSpiritEvasionComponent::TryAdvanceAtForAutomation(
	double NowSeconds,
	Idemo_mapShanmenSpiritEvasionSegmentExecutionPort& ExecutionPort)
{
	return RecordStep(Host.TryAdvance(NowSeconds, ExecutionPort));
}
#endif
