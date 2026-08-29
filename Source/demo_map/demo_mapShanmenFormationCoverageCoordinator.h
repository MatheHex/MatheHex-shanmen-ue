#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenFormationCoverageTracker.h"
#include "demo_mapShanmenFormationWorldCoverageSampler.h"

enum class Edemo_mapShanmenFormationCoverageCommandMode : uint8
{
	Prime,
	Advance,
	Rebase
};

/**
 * One caller-authored command for a synchronous World sample and tracker update.
 * Prime has no expected baseline. Advance and Rebase require the baseline that
 * the caller observed before issuing the command.
 */
struct Fdemo_mapShanmenFormationCoverageCommand
{
	Edemo_mapShanmenFormationCoverageCommandMode Mode =
		Edemo_mapShanmenFormationCoverageCommandMode::Prime;
	FGuid ExpectedBaselineReceiptId;

	bool IsValid() const;

	static Fdemo_mapShanmenFormationCoverageCommand MakePrime();
	static Fdemo_mapShanmenFormationCoverageCommand MakeAdvance(
		const FGuid& ExpectedBaselineReceiptId);
	static Fdemo_mapShanmenFormationCoverageCommand MakeRebase(
		const FGuid& ExpectedBaselineReceiptId);
};

enum class Edemo_mapShanmenFormationCoverageCoordinatorStatus : uint8
{
	Executed,
	CommandInvalid,
	TrackerInconsistent,
	TrackerNotPrimed,
	BaselineConflict,
	SampleRejected,
	TrackerRejected
};

/** One self-contained result from Sample followed by one tracker command. */
struct Fdemo_mapShanmenFormationCoverageCoordinatorResult
{
	Edemo_mapShanmenFormationCoverageCoordinatorStatus Status =
		Edemo_mapShanmenFormationCoverageCoordinatorStatus::CommandInvalid;
	FString Diagnostic;
	Fdemo_mapShanmenFormationCoverageCommand Command;
	Fdemo_mapShanmenFormationWorldCoverageResult Sample;
	Fdemo_mapShanmenFormationCoverageTrackerResult TrackerResult;

	bool IsSuccess() const;
};

/**
 * Stateless product seam that composes P8.6 World sampling with P8.8 tracking.
 *
 * The caller owns World, Area, Registry, Actor subset, Tracker, command cadence,
 * and serialization. The coordinator retains no pointer, discovers no Actor,
 * schedules no Tick/timer, and applies no gameplay effect.
 */
class Fdemo_mapShanmenFormationCoverageCoordinator
{
public:
	static Fdemo_mapShanmenFormationCoverageCoordinatorResult Execute(
		UWorld* World,
		const Fdemo_mapShanmenFormationAreaSnapshot& Area,
		const FShanmenWorldEntityRegistry& EntityRegistry,
		const TArray<AActor*>& SourceActors,
		const Fdemo_mapShanmenFormationCoverageCommand& Command,
		Fdemo_mapShanmenFormationCoverageTracker& Tracker);
};
