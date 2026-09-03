#include "demo_mapShanmenDivineSenseCommandRouter.h"

#include "GameFramework/Actor.h"
#include "ShanmenDeterministicId.h"

namespace
{
	struct FResolvedSubjectActor
	{
		FGuid EntityId;
		AActor* Actor = nullptr;
	};

	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	FString FloatBits(float Value)
	{
		uint32 Bits = 0;
		FPlatformMemory::Memcpy(&Bits, &Value, sizeof(Bits));
		return FString::Printf(TEXT("%08X"), Bits);
	}

	bool GuidLess(const FGuid& Left, const FGuid& Right)
	{
		return GuidDigits(Left) < GuidDigits(Right);
	}

	bool SnapshotsMatch(
		const FShanmenActionResourceSnapshot& Left,
		const FShanmenActionResourceSnapshot& Right)
	{
		return Left.IsValid() && Right.IsValid()
			&& Left.GetSnapshotId() == Right.GetSnapshotId();
	}

	bool SnapshotShapeMatches(
		const FShanmenActionResourceSnapshot& Left,
		const FShanmenActionResourceSnapshot& Right)
	{
		return Left.IsValid() && Right.IsValid()
			&& Left.GetOwnerEntityId() == Right.GetOwnerEntityId()
			&& Left.GetResourceChannel() == Right.GetResourceChannel()
			&& FloatBits(Left.GetMaximumAmount())
				== FloatBits(Right.GetMaximumAmount());
	}

	FGuid MakeRouteCommandId(
		const FGuid& RouterId,
		const FGuid& HostId,
		const FGuid& ResourceSnapshotId,
		const Fdemo_mapShanmenDivineSensePulseCommand& PulseCommand,
		const TArray<FGuid>& SubjectEntityIds)
	{
		if (!RouterId.IsValid() || !HostId.IsValid()
			|| !ResourceSnapshotId.IsValid() || !PulseCommand.IsValid())
		{
			return FGuid();
		}

		TArray<FString> Parts = {
			GuidDigits(RouterId),
			GuidDigits(HostId),
			GuidDigits(ResourceSnapshotId),
			GuidDigits(PulseCommand.GetCommandId()),
			FString::FromInt(SubjectEntityIds.Num())
		};
		for (const FGuid& SubjectEntityId : SubjectEntityIds)
		{
			Parts.Add(GuidDigits(SubjectEntityId));
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.Spell.DivineSense.RouteCommand.r1"), Parts);
	}

	FGuid MakeRouterId(
		const FGuid& HostId,
		const FShanmenActionResourceSnapshot& OpeningSnapshot,
		int32 Capacity)
	{
		if (!HostId.IsValid() || !OpeningSnapshot.IsValid() || Capacity <= 0)
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.Spell.DivineSense.CommandRouter.r1"),
			{
				GuidDigits(HostId),
				GuidDigits(OpeningSnapshot.GetSnapshotId()),
				FString::FromInt(Capacity)
			});
	}

	bool TryResolveSubjects(
		const FShanmenWorldEntityRegistry& EntityRegistry,
		const FGuid& RunId,
		const TArray<AActor*>& SubjectActors,
		TArray<FResolvedSubjectActor>& OutSubjects)
	{
		OutSubjects.Reset();
		OutSubjects.Reserve(SubjectActors.Num());
		TSet<FGuid> UniqueEntityIds;
		UniqueEntityIds.Reserve(SubjectActors.Num());
		for (AActor* SubjectActor : SubjectActors)
		{
			FGuid SubjectEntityId;
			if (!::IsValid(SubjectActor)
				|| SubjectActor->IsActorBeingDestroyed()
				|| !EntityRegistry.TryResolveObject(
					RunId,
					SubjectActor,
					INDEX_NONE,
					SubjectEntityId)
				|| !SubjectEntityId.IsValid()
				|| UniqueEntityIds.Contains(SubjectEntityId))
			{
				OutSubjects.Reset();
				return false;
			}
			UniqueEntityIds.Add(SubjectEntityId);
			OutSubjects.Add({ SubjectEntityId, SubjectActor });
		}
		OutSubjects.Sort([](
			const FResolvedSubjectActor& Left,
			const FResolvedSubjectActor& Right)
		{
			return GuidLess(Left.EntityId, Right.EntityId);
		});
		return true;
	}

	bool SubjectIdsMatch(
		const TArray<FResolvedSubjectActor>& Subjects,
		const TArray<FGuid>& ExpectedIds)
	{
		if (Subjects.Num() != ExpectedIds.Num())
		{
			return false;
		}
		for (int32 Index = 0; Index < Subjects.Num(); ++Index)
		{
			if (Subjects[Index].EntityId != ExpectedIds[Index])
			{
				return false;
			}
		}
		return true;
	}

	Fdemo_mapShanmenDivineSenseCommandRouteResult MakeResult(
		Edemo_mapShanmenDivineSenseCommandRouteStatus Status,
		const TCHAR* Diagnostic,
		const Fdemo_mapShanmenDivineSenseCommandRouter* Router,
		const Fdemo_mapShanmenDivineSenseRouteCommand& Command)
	{
		Fdemo_mapShanmenDivineSenseCommandRouteResult Result;
		Result.Status = Status;
		Result.Diagnostic = Diagnostic;
		Result.Command = Command;
		if (Router && Router->IsValid())
		{
			Result.RouterId = Router->GetRouterId();
			Result.RunId = Router->GetRunId();
			Result.SourceEntityId = Router->GetSourceEntityId();
		}
		return Result;
	}
}

bool Fdemo_mapShanmenDivineSenseRouteCommand::IsValid() const
{
	if (!RouteCommandId.IsValid() || !ExpectedRouterId.IsValid()
		|| !ExpectedHostId.IsValid()
		|| !ExpectedResourceSnapshotId.IsValid()
		|| !PulseCommand.IsValid()
		|| SubjectEntityIds.Num()
			> PulseCommand.GetSubjectActorBudget())
	{
		return false;
	}
	for (int32 Index = 0; Index < SubjectEntityIds.Num(); ++Index)
	{
		if (!SubjectEntityIds[Index].IsValid()
			|| (Index > 0
				&& !GuidLess(
					SubjectEntityIds[Index - 1],
					SubjectEntityIds[Index])))
		{
			return false;
		}
	}
	return RouteCommandId == MakeRouteCommandId(
		ExpectedRouterId,
		ExpectedHostId,
		ExpectedResourceSnapshotId,
		PulseCommand,
		SubjectEntityIds);
}

bool Fdemo_mapShanmenDivineSenseRouteCommand::Matches(
	const Fdemo_mapShanmenDivineSenseRouteCommand& Other) const
{
	return IsValid() && Other.IsValid()
		&& RouteCommandId == Other.RouteCommandId
		&& ExpectedRouterId == Other.ExpectedRouterId
		&& ExpectedHostId == Other.ExpectedHostId
		&& ExpectedResourceSnapshotId
			== Other.ExpectedResourceSnapshotId
		&& PulseCommand.GetCommandId()
			== Other.PulseCommand.GetCommandId()
		&& SubjectEntityIds == Other.SubjectEntityIds;
}

bool Fdemo_mapShanmenDivineSenseCommandRouteResult::IsValid() const
{
	if (Status == Edemo_mapShanmenDivineSenseCommandRouteStatus::Invalid
		|| Diagnostic.IsEmpty())
	{
		return false;
	}
	if (Status
		== Edemo_mapShanmenDivineSenseCommandRouteStatus::RouterNotReady)
	{
		return !RouterId.IsValid() && !HostPulse.IsValid();
	}
	if (!RouterId.IsValid() || !RunId.IsValid()
		|| !SourceEntityId.IsValid())
	{
		return false;
	}
	if (Status
		== Edemo_mapShanmenDivineSenseCommandRouteStatus::CommandInvalid)
	{
		return !Command.IsValid() && !HostPulse.IsValid();
	}
	if (!Command.IsValid()
		|| Command.GetExpectedRouterId() != RouterId
		|| Command.GetPulseCommand().GetAction().GetRunId() != RunId
		|| Command.GetPulseCommand().GetAction().GetSourceEntityId()
			!= SourceEntityId)
	{
		return false;
	}

	if (Status == Edemo_mapShanmenDivineSenseCommandRouteStatus::Applied
		|| Status
			== Edemo_mapShanmenDivineSenseCommandRouteStatus::AlreadyApplied)
	{
		if (!HostPulse.IsSuccess()
			|| HostPulse.HostId != Command.GetExpectedHostId()
			|| HostPulse.Command.GetCommandId()
				!= Command.GetPulseCommand().GetCommandId())
		{
			return false;
		}
		if (Status
			== Edemo_mapShanmenDivineSenseCommandRouteStatus::Applied)
		{
			return !HostPulse.IsReplay()
				&& HostPulse.ResourceBefore.GetSnapshotId()
					== Command.GetExpectedResourceSnapshotId()
				&& HostPulse.Pulse.Receipt.GetObservedSubjectCount()
					== Command.GetSubjectEntityIds().Num();
		}
		return HostPulse.IsReplay()
			&& SnapshotsMatch(
				HostPulse.ResourceBefore, HostPulse.ResourceAfter);
	}

	if (Status
		== Edemo_mapShanmenDivineSenseCommandRouteStatus::HostRejected)
	{
		return HostPulse.IsValid() && !HostPulse.IsSuccess()
			&& HostPulse.HostId == Command.GetExpectedHostId()
			&& HostPulse.Command.GetCommandId()
				== Command.GetPulseCommand().GetCommandId();
	}
	if (Status
		== Edemo_mapShanmenDivineSenseCommandRouteStatus::StateDesynchronized)
	{
		return true;
	}
	return !HostPulse.IsValid();
}

bool Fdemo_mapShanmenDivineSenseCommandRouteResult::IsAccepted() const
{
	return IsValid()
		&& (Status == Edemo_mapShanmenDivineSenseCommandRouteStatus::Applied
			|| Status
				== Edemo_mapShanmenDivineSenseCommandRouteStatus::
					AlreadyApplied);
}

bool Fdemo_mapShanmenDivineSenseCommandRouter::TryCreate(
	const Fdemo_mapShanmenDivineSenseProductHost& Host,
	Fdemo_mapShanmenDivineSenseCommandRouter& OutRouter)
{
	OutRouter.Reset();
	if (!Host.IsValid() || Host.NumProcessedPulses() != 0)
	{
		return false;
	}

	FShanmenActionResourceSnapshot OpeningSnapshot;
	if (!Host.TryCaptureResourceSnapshot(OpeningSnapshot)
		|| !SnapshotsMatch(
			OpeningSnapshot, Host.GetOpeningResourceSnapshot()))
	{
		return false;
	}

	Fdemo_mapShanmenDivineSenseCommandRouter Candidate;
	Candidate.HostId = Host.GetHostId();
	Candidate.RunId = Host.GetRunId();
	Candidate.SourceEntityId = Host.GetSourceEntityId();
	Candidate.OpeningResourceSnapshot = OpeningSnapshot;
	Candidate.CurrentResourceSnapshot = OpeningSnapshot;
	Candidate.ProcessedCommandCapacity = Host.GetProcessedPulseCapacity();
	Candidate.RouterId = MakeRouterId(
		Candidate.HostId,
		Candidate.OpeningResourceSnapshot,
		Candidate.ProcessedCommandCapacity);
	Candidate.bInitialized = true;
	if (!Candidate.IsValid() || !Candidate.IsConsistentWithHost(Host))
	{
		return false;
	}
	OutRouter = MoveTemp(Candidate);
	return true;
}

bool Fdemo_mapShanmenDivineSenseCommandRouter::TryCaptureCommand(
	const Fdemo_mapShanmenDivineSenseProductHost& Host,
	const FShanmenWorldEntityRegistry& EntityRegistry,
	const FShanmenCombatActionSnapshot& Action,
	const FShanmenDivineSenseDefinition& Definition,
	const FShanmenActionResourceCost& Cost,
	int32 ScanOrdinal,
	int32 SubjectActorBudget,
	const TArray<AActor*>& SubjectActors,
	Fdemo_mapShanmenDivineSenseRouteCommand& OutCommand) const
{
	OutCommand = Fdemo_mapShanmenDivineSenseRouteCommand();
	if (!IsConsistentWithHost(Host)
		|| EntityRegistry.GetRunId() != RunId
		|| SubjectActors.Num() > SubjectActorBudget)
	{
		return false;
	}

	Fdemo_mapShanmenDivineSensePulseCommand PulseCommand;
	if (!Fdemo_mapShanmenDivineSensePulseCommand::TryCapture(
			Action,
			Definition,
			Cost,
			ScanOrdinal,
			SubjectActorBudget,
			PulseCommand)
		|| Action.GetRunId() != RunId
		|| Action.GetSourceEntityId() != SourceEntityId)
	{
		return false;
	}

	TArray<FResolvedSubjectActor> ResolvedSubjects;
	if (!TryResolveSubjects(
			EntityRegistry, RunId, SubjectActors, ResolvedSubjects))
	{
		return false;
	}
	TArray<FGuid> SubjectEntityIds;
	SubjectEntityIds.Reserve(ResolvedSubjects.Num());
	for (const FResolvedSubjectActor& Subject : ResolvedSubjects)
	{
		SubjectEntityIds.Add(Subject.EntityId);
	}

	for (const TPair<FGuid, FProcessedCommand>& Pair : ProcessedCommands)
	{
		const Fdemo_mapShanmenDivineSenseRouteCommand& Existing =
			Pair.Value.Command;
		if (Existing.GetPulseCommand().GetAction().GetActivationId()
			== Action.GetActivationId())
		{
			if (Existing.GetPulseCommand().GetCommandId()
					== PulseCommand.GetCommandId()
				&& Existing.GetSubjectEntityIds() == SubjectEntityIds)
			{
				OutCommand = Existing;
				return true;
			}
			return false;
		}
	}

	if (ProcessedCommands.Num() >= ProcessedCommandCapacity)
	{
		return false;
	}

	Fdemo_mapShanmenDivineSenseRouteCommand Candidate;
	Candidate.ExpectedRouterId = RouterId;
	Candidate.ExpectedHostId = HostId;
	Candidate.ExpectedResourceSnapshotId =
		CurrentResourceSnapshot.GetSnapshotId();
	Candidate.PulseCommand = MoveTemp(PulseCommand);
	Candidate.SubjectEntityIds = MoveTemp(SubjectEntityIds);
	Candidate.RouteCommandId = MakeRouteCommandId(
		Candidate.ExpectedRouterId,
		Candidate.ExpectedHostId,
		Candidate.ExpectedResourceSnapshotId,
		Candidate.PulseCommand,
		Candidate.SubjectEntityIds);
	if (!Candidate.IsValid())
	{
		return false;
	}
	OutCommand = MoveTemp(Candidate);
	return true;
}

Fdemo_mapShanmenDivineSenseCommandRouteResult
Fdemo_mapShanmenDivineSenseCommandRouter::TryRoute(
	Fdemo_mapShanmenDivineSenseProductHost& Host,
	UWorld* World,
	const FShanmenWorldEntityRegistry& EntityRegistry,
	AActor* SourceActor,
	const Fdemo_mapShanmenDivineSenseRouteCommand& Command,
	const TArray<AActor*>& SubjectActors,
	const Idemo_mapShanmenDivineSenseWorldEvidenceProvider& EvidenceProvider)
{
	if (!IsValid())
	{
		return MakeResult(
			Edemo_mapShanmenDivineSenseCommandRouteStatus::RouterNotReady,
			TEXT("Divine Sense command router is not ready."),
			nullptr,
			Command);
	}
	if (!Host.IsValid())
	{
		return MakeResult(
			Edemo_mapShanmenDivineSenseCommandRouteStatus::HostNotReady,
			TEXT("Divine Sense command route requires one valid product Host."),
			this,
			Command);
	}
	if (!Command.IsValid())
	{
		return MakeResult(
			Edemo_mapShanmenDivineSenseCommandRouteStatus::CommandInvalid,
			TEXT("Divine Sense route command is invalid."),
			this,
			Command);
	}
	if (Host.GetHostId() != HostId
		|| Command.GetExpectedHostId() != HostId
		|| Command.GetExpectedRouterId() != RouterId)
	{
		return MakeResult(
			Edemo_mapShanmenDivineSenseCommandRouteStatus::HostMismatch,
			TEXT("Divine Sense command does not name this Router and Host."),
			this,
			Command);
	}
	if (Command.GetPulseCommand().GetAction().GetRunId() != RunId
		|| Host.GetRunId() != RunId)
	{
		return MakeResult(
			Edemo_mapShanmenDivineSenseCommandRouteStatus::RunMismatch,
			TEXT("Divine Sense command route crosses Run identity."),
			this,
			Command);
	}
	if (Command.GetPulseCommand().GetAction().GetSourceEntityId()
			!= SourceEntityId
		|| Host.GetSourceEntityId() != SourceEntityId)
	{
		return MakeResult(
			Edemo_mapShanmenDivineSenseCommandRouteStatus::SourceMismatch,
			TEXT("Divine Sense command route crosses source identity."),
			this,
			Command);
	}
	if (!IsConsistentWithHost(Host))
	{
		return MakeResult(
			Edemo_mapShanmenDivineSenseCommandRouteStatus::StateDesynchronized,
			TEXT("Divine Sense Router and Host no longer share one product state."),
			this,
			Command);
	}

	if (const FProcessedCommand* Existing =
		ProcessedCommands.Find(Command.GetRouteCommandId()))
	{
		if (!Existing->Command.Matches(Command))
		{
			return MakeResult(
				Edemo_mapShanmenDivineSenseCommandRouteStatus::
					ActivationConflict,
				TEXT("Divine Sense RouteCommandId was reused with another payload."),
				this,
				Command);
		}

		Fdemo_mapShanmenDivineSenseProductHost HostCandidate = Host;
		Fdemo_mapShanmenDivineSenseCommandRouteResult Replay = MakeResult(
			Edemo_mapShanmenDivineSenseCommandRouteStatus::AlreadyApplied,
			TEXT("Exact Divine Sense route replay returned stored proof without live Actor or evidence reads."),
			this,
			Command);
		Replay.HostPulse = HostCandidate.ExecutePulse(
			nullptr,
			EntityRegistry,
			nullptr,
			Command.GetPulseCommand(),
			{},
			EvidenceProvider);
		if (!Replay.IsValid()
			|| Replay.HostPulse.Pulse.Receipt.GetReceiptId()
				!= Existing->Result.HostPulse.Pulse.Receipt.GetReceiptId()
			|| !IsConsistentWithHost(HostCandidate))
		{
			return MakeResult(
				Edemo_mapShanmenDivineSenseCommandRouteStatus::
					StateDesynchronized,
				TEXT("Stored Divine Sense route and Host replay proof disagree."),
				this,
				Command);
		}
		return Replay;
	}

	for (const TPair<FGuid, FProcessedCommand>& Pair : ProcessedCommands)
	{
		if (Pair.Value.Command.GetPulseCommand().GetAction().GetActivationId()
			== Command.GetPulseCommand().GetAction().GetActivationId())
		{
			return MakeResult(
				Edemo_mapShanmenDivineSenseCommandRouteStatus::
					ActivationConflict,
				TEXT("Divine Sense ActivationId already names another accepted route command."),
				this,
				Command);
		}
	}
	if (ProcessedCommands.Num() >= ProcessedCommandCapacity)
	{
		return MakeResult(
			Edemo_mapShanmenDivineSenseCommandRouteStatus::
				ProcessedCapacityExceeded,
			TEXT("Divine Sense Router processed-command capacity is exhausted."),
			this,
			Command);
	}
	if (Command.GetExpectedResourceSnapshotId()
		!= CurrentResourceSnapshot.GetSnapshotId())
	{
		return MakeResult(
			Edemo_mapShanmenDivineSenseCommandRouteStatus::
				ResourceProjectionStale,
			TEXT("Divine Sense command was captured from a stale resource projection."),
			this,
			Command);
	}
	if (EntityRegistry.GetRunId() != RunId)
	{
		return MakeResult(
			Edemo_mapShanmenDivineSenseCommandRouteStatus::RegistryMismatch,
			TEXT("Divine Sense command requires the Router Run entity registry."),
			this,
			Command);
	}

	FGuid ResolvedSourceEntityId;
	if (!::IsValid(SourceActor) || SourceActor->IsActorBeingDestroyed()
		|| !EntityRegistry.TryResolveObject(
			RunId,
			SourceActor,
			INDEX_NONE,
			ResolvedSourceEntityId)
		|| ResolvedSourceEntityId != SourceEntityId)
	{
		return MakeResult(
			Edemo_mapShanmenDivineSenseCommandRouteStatus::
				SourceActorMismatch,
			TEXT("Divine Sense source Actor does not match the frozen source entity."),
			this,
			Command);
	}

	TArray<FResolvedSubjectActor> ResolvedSubjects;
	if (SubjectActors.Num() != Command.GetSubjectEntityIds().Num()
		|| !TryResolveSubjects(
			EntityRegistry, RunId, SubjectActors, ResolvedSubjects)
		|| !SubjectIdsMatch(
			ResolvedSubjects, Command.GetSubjectEntityIds()))
	{
		return MakeResult(
			Edemo_mapShanmenDivineSenseCommandRouteStatus::
				SubjectActorMismatch,
			TEXT("Divine Sense live Actor batch differs from the frozen canonical entity set."),
			this,
			Command);
	}
	TArray<AActor*> CanonicalSubjectActors;
	CanonicalSubjectActors.Reserve(ResolvedSubjects.Num());
	for (const FResolvedSubjectActor& Subject : ResolvedSubjects)
	{
		CanonicalSubjectActors.Add(Subject.Actor);
	}

	Fdemo_mapShanmenDivineSenseProductHost HostCandidate = Host;
	Fdemo_mapShanmenDivineSenseHostPulseResult HostPulse =
		HostCandidate.ExecutePulse(
			World,
			EntityRegistry,
			SourceActor,
			Command.GetPulseCommand(),
			CanonicalSubjectActors,
			EvidenceProvider);
	if (!HostPulse.IsValid())
	{
		return MakeResult(
			Edemo_mapShanmenDivineSenseCommandRouteStatus::
				StateDesynchronized,
			TEXT("Divine Sense Host returned invalid route evidence."),
			this,
			Command);
	}
	if (!HostPulse.IsSuccess())
	{
		Fdemo_mapShanmenDivineSenseCommandRouteResult Rejected = MakeResult(
			Edemo_mapShanmenDivineSenseCommandRouteStatus::HostRejected,
			TEXT("Divine Sense Host rejected the routed command; Router and Host state were unchanged."),
			this,
			Command);
		Rejected.HostPulse = MoveTemp(HostPulse);
		return Rejected;
	}
	if (HostPulse.IsReplay())
	{
		return MakeResult(
			Edemo_mapShanmenDivineSenseCommandRouteStatus::
				StateDesynchronized,
			TEXT("Unrecorded Divine Sense route unexpectedly resolved as a Host replay."),
			this,
			Command);
	}

	Fdemo_mapShanmenDivineSenseCommandRouteResult Applied = MakeResult(
		Edemo_mapShanmenDivineSenseCommandRouteStatus::Applied,
		TEXT("Divine Sense command atomically published Host and Router proof."),
		this,
		Command);
	Applied.HostPulse = MoveTemp(HostPulse);
	if (!Applied.IsValid())
	{
		return MakeResult(
			Edemo_mapShanmenDivineSenseCommandRouteStatus::
				StateDesynchronized,
			TEXT("Divine Sense route produced inconsistent accepted evidence."),
			this,
			Command);
	}

	Fdemo_mapShanmenDivineSenseCommandRouter RouterCandidate = *this;
	FProcessedCommand Processed;
	Processed.Sequence = RouterCandidate.ProcessedCommands.Num();
	Processed.Command = Command;
	Processed.Result = Applied;
	RouterCandidate.ProcessedCommands.Add(
		Command.GetRouteCommandId(), MoveTemp(Processed));
	RouterCandidate.CurrentResourceSnapshot = Applied.HostPulse.ResourceAfter;
	if (!RouterCandidate.IsValid()
		|| !RouterCandidate.IsConsistentWithHost(HostCandidate))
	{
		return MakeResult(
			Edemo_mapShanmenDivineSenseCommandRouteStatus::
				StateDesynchronized,
			TEXT("Divine Sense Router could not publish a valid replay record."),
			this,
			Command);
	}

	Host = MoveTemp(HostCandidate);
	*this = MoveTemp(RouterCandidate);
	return Applied;
}

bool Fdemo_mapShanmenDivineSenseCommandRouter::IsValid() const
{
	if (!bInitialized || !RouterId.IsValid() || !HostId.IsValid()
		|| !RunId.IsValid() || !SourceEntityId.IsValid()
		|| !OpeningResourceSnapshot.IsValid()
		|| !CurrentResourceSnapshot.IsValid()
		|| ProcessedCommandCapacity <= 0
		|| ProcessedCommands.Num() > ProcessedCommandCapacity
		|| OpeningResourceSnapshot.GetOwnerEntityId() != SourceEntityId
		|| CurrentResourceSnapshot.GetOwnerEntityId() != SourceEntityId
		|| !SnapshotShapeMatches(
			OpeningResourceSnapshot, CurrentResourceSnapshot)
		|| RouterId != MakeRouterId(
			HostId,
			OpeningResourceSnapshot,
			ProcessedCommandCapacity))
	{
		return false;
	}

	const int64 ProcessedCount = ProcessedCommands.Num();
	const int64 OpeningRevision =
		OpeningResourceSnapshot.GetAuthorityRevision();
	if (OpeningRevision > MAX_int64 - (ProcessedCount * 2)
		|| CurrentResourceSnapshot.GetAuthorityRevision()
			!= OpeningRevision + (ProcessedCount * 2))
	{
		return false;
	}

	TArray<const FProcessedCommand*> Ordered;
	Ordered.SetNumZeroed(ProcessedCommands.Num());
	for (const TPair<FGuid, FProcessedCommand>& Pair : ProcessedCommands)
	{
		const FProcessedCommand& Processed = Pair.Value;
		if (Processed.Sequence < 0
			|| Processed.Sequence >= Ordered.Num()
			|| Ordered[Processed.Sequence] != nullptr
			|| Pair.Key != Processed.Command.GetRouteCommandId()
			|| !Processed.Command.IsValid()
			|| Processed.Command.GetExpectedRouterId() != RouterId
			|| Processed.Command.GetExpectedHostId() != HostId
			|| Processed.Command.GetPulseCommand().GetAction().GetRunId()
				!= RunId
			|| Processed.Command.GetPulseCommand().GetAction()
				.GetSourceEntityId() != SourceEntityId
			|| Processed.Result.Status
				!= Edemo_mapShanmenDivineSenseCommandRouteStatus::Applied
			|| !Processed.Result.IsValid()
			|| !Processed.Result.Command.Matches(Processed.Command))
		{
			return false;
		}
		Ordered[Processed.Sequence] = &Processed;
	}

	FGuid ExpectedSnapshotId =
		OpeningResourceSnapshot.GetSnapshotId();
	for (const FProcessedCommand* Processed : Ordered)
	{
		if (!Processed
			|| Processed->Command.GetExpectedResourceSnapshotId()
				!= ExpectedSnapshotId
			|| Processed->Result.HostPulse.ResourceBefore.GetSnapshotId()
				!= ExpectedSnapshotId)
		{
			return false;
		}
		ExpectedSnapshotId =
			Processed->Result.HostPulse.ResourceAfter.GetSnapshotId();
	}
	return ExpectedSnapshotId == CurrentResourceSnapshot.GetSnapshotId();
}

bool Fdemo_mapShanmenDivineSenseCommandRouter::IsConsistentWithHost(
	const Fdemo_mapShanmenDivineSenseProductHost& Host) const
{
	FShanmenActionResourceSnapshot HostSnapshot;
	return IsValid() && Host.IsValid()
		&& Host.GetHostId() == HostId
		&& Host.GetRunId() == RunId
		&& Host.GetSourceEntityId() == SourceEntityId
		&& Host.GetProcessedPulseCapacity() == ProcessedCommandCapacity
		&& Host.NumProcessedPulses() == ProcessedCommands.Num()
		&& SnapshotsMatch(
			Host.GetOpeningResourceSnapshot(), OpeningResourceSnapshot)
		&& Host.TryCaptureResourceSnapshot(HostSnapshot)
		&& SnapshotsMatch(HostSnapshot, CurrentResourceSnapshot);
}

void Fdemo_mapShanmenDivineSenseCommandRouter::Reset()
{
	*this = Fdemo_mapShanmenDivineSenseCommandRouter();
}
