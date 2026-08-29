#include "demo_mapShanmenFormationCoverageCoordinator.h"

namespace
{
	Fdemo_mapShanmenFormationCoverageCoordinatorResult MakeResult(
		const Edemo_mapShanmenFormationCoverageCoordinatorStatus Status,
		const TCHAR* Diagnostic,
		const Fdemo_mapShanmenFormationCoverageCommand& Command,
		const Fdemo_mapShanmenFormationWorldCoverageResult& Sample =
			Fdemo_mapShanmenFormationWorldCoverageResult(),
		const Fdemo_mapShanmenFormationCoverageTrackerResult& TrackerResult =
			Fdemo_mapShanmenFormationCoverageTrackerResult())
	{
		Fdemo_mapShanmenFormationCoverageCoordinatorResult Result;
		Result.Status = Status;
		Result.Diagnostic = Diagnostic;
		Result.Command = Command;
		Result.Sample = Sample;
		Result.TrackerResult = TrackerResult;
		return Result;
	}

	bool TrackerStatusMatchesCommand(
		const Edemo_mapShanmenFormationCoverageCommandMode Mode,
		const Edemo_mapShanmenFormationCoverageTrackerStatus Status)
	{
		switch (Mode)
		{
		case Edemo_mapShanmenFormationCoverageCommandMode::Prime:
			return Status == Edemo_mapShanmenFormationCoverageTrackerStatus::Primed
				|| Status
					== Edemo_mapShanmenFormationCoverageTrackerStatus::PrimeReplayed;
		case Edemo_mapShanmenFormationCoverageCommandMode::Advance:
			return Status == Edemo_mapShanmenFormationCoverageTrackerStatus::Advanced
				|| Status
					== Edemo_mapShanmenFormationCoverageTrackerStatus::AdvanceReplayed;
		case Edemo_mapShanmenFormationCoverageCommandMode::Rebase:
			return Status == Edemo_mapShanmenFormationCoverageTrackerStatus::Rebased;
		default:
			return false;
		}
	}
}

bool Fdemo_mapShanmenFormationCoverageCommand::IsValid() const
{
	switch (Mode)
	{
	case Edemo_mapShanmenFormationCoverageCommandMode::Prime:
		return !ExpectedBaselineReceiptId.IsValid();
	case Edemo_mapShanmenFormationCoverageCommandMode::Advance:
	case Edemo_mapShanmenFormationCoverageCommandMode::Rebase:
		return ExpectedBaselineReceiptId.IsValid();
	default:
		return false;
	}
}

Fdemo_mapShanmenFormationCoverageCommand
Fdemo_mapShanmenFormationCoverageCommand::MakePrime()
{
	return Fdemo_mapShanmenFormationCoverageCommand();
}

Fdemo_mapShanmenFormationCoverageCommand
Fdemo_mapShanmenFormationCoverageCommand::MakeAdvance(
	const FGuid& ExpectedBaselineReceiptId)
{
	Fdemo_mapShanmenFormationCoverageCommand Command;
	Command.Mode = Edemo_mapShanmenFormationCoverageCommandMode::Advance;
	Command.ExpectedBaselineReceiptId = ExpectedBaselineReceiptId;
	return Command;
}

Fdemo_mapShanmenFormationCoverageCommand
Fdemo_mapShanmenFormationCoverageCommand::MakeRebase(
	const FGuid& ExpectedBaselineReceiptId)
{
	Fdemo_mapShanmenFormationCoverageCommand Command;
	Command.Mode = Edemo_mapShanmenFormationCoverageCommandMode::Rebase;
	Command.ExpectedBaselineReceiptId = ExpectedBaselineReceiptId;
	return Command;
}

bool Fdemo_mapShanmenFormationCoverageCoordinatorResult::IsSuccess() const
{
	if (Status != Edemo_mapShanmenFormationCoverageCoordinatorStatus::Executed
		|| !Command.IsValid() || !Sample.IsSuccess()
		|| !TrackerResult.IsSuccess()
		|| !TrackerStatusMatchesCommand(Command.Mode, TrackerResult.Status)
		|| TrackerResult.CurrentBaselineReceiptId
			!= Sample.Coverage.Receipt.ReceiptId)
	{
		return false;
	}
	return true;
}

Fdemo_mapShanmenFormationCoverageCoordinatorResult
Fdemo_mapShanmenFormationCoverageCoordinator::Execute(
	UWorld* World,
	const Fdemo_mapShanmenFormationAreaSnapshot& Area,
	const FShanmenWorldEntityRegistry& EntityRegistry,
	const TArray<AActor*>& SourceActors,
	const Fdemo_mapShanmenFormationCoverageCommand& Command,
	Fdemo_mapShanmenFormationCoverageTracker& Tracker)
{
	if (!Command.IsValid())
	{
		return MakeResult(
			Edemo_mapShanmenFormationCoverageCoordinatorStatus::CommandInvalid,
			TEXT("Coverage coordination requires one structurally valid command."),
			Command);
	}
	if (!Tracker.IsConsistent())
	{
		return MakeResult(
			Edemo_mapShanmenFormationCoverageCoordinatorStatus::TrackerInconsistent,
			TEXT("Coverage coordination refuses an internally inconsistent tracker."),
			Command);
	}
	if (Command.Mode != Edemo_mapShanmenFormationCoverageCommandMode::Prime
		&& !Tracker.IsPrimed())
	{
		return MakeResult(
			Edemo_mapShanmenFormationCoverageCoordinatorStatus::TrackerNotPrimed,
			TEXT("Prime must establish a tracker baseline before this command."),
			Command);
	}
	if (Command.Mode == Edemo_mapShanmenFormationCoverageCommandMode::Rebase)
	{
		Fdemo_mapShanmenFormationCoverageReceipt Baseline;
		if (!Tracker.TryGetBaseline(Baseline)
			|| Baseline.ReceiptId != Command.ExpectedBaselineReceiptId)
		{
			return MakeResult(
				Edemo_mapShanmenFormationCoverageCoordinatorStatus::BaselineConflict,
				TEXT("A stale Rebase command cannot replace the current baseline."),
				Command);
		}
	}

	const Fdemo_mapShanmenFormationWorldCoverageResult Sample =
		Fdemo_mapShanmenFormationWorldCoverageSampler::Sample(
			World, Area, EntityRegistry, SourceActors);
	if (!Sample.IsSuccess())
	{
		return MakeResult(
			Edemo_mapShanmenFormationCoverageCoordinatorStatus::SampleRejected,
			TEXT("P8.6 rejected the requested synchronous World sample."),
			Command, Sample);
	}

	Fdemo_mapShanmenFormationCoverageTrackerResult TrackerResult;
	switch (Command.Mode)
	{
	case Edemo_mapShanmenFormationCoverageCommandMode::Prime:
		TrackerResult = Tracker.Prime(Sample.Coverage.Receipt);
		break;
	case Edemo_mapShanmenFormationCoverageCommandMode::Advance:
		TrackerResult = Tracker.Advance(
			Command.ExpectedBaselineReceiptId, Sample.Coverage.Receipt);
		break;
	case Edemo_mapShanmenFormationCoverageCommandMode::Rebase:
		TrackerResult = Tracker.Rebase(Sample.Coverage.Receipt);
		break;
	default:
		return MakeResult(
			Edemo_mapShanmenFormationCoverageCoordinatorStatus::CommandInvalid,
			TEXT("Unknown coverage command mode."), Command, Sample);
	}

	if (!TrackerResult.IsSuccess() || !Tracker.IsConsistent())
	{
		return MakeResult(
			Edemo_mapShanmenFormationCoverageCoordinatorStatus::TrackerRejected,
			TEXT("P8.8 rejected the sampled coverage; tracker authority is unchanged."),
			Command, Sample, TrackerResult);
	}
	return MakeResult(
		Edemo_mapShanmenFormationCoverageCoordinatorStatus::Executed,
		TEXT("One synchronous World sample committed through the requested tracker command."),
		Command, Sample, TrackerResult);
}
