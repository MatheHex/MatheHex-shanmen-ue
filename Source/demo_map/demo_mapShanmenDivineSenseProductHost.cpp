#include "demo_mapShanmenDivineSenseProductHost.h"

#include "ShanmenCombatRuntimeTags.h"
#include "ShanmenDeterministicId.h"

namespace
{
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

	bool FloatsMatchExactly(float Left, float Right)
	{
		return FloatBits(Left) == FloatBits(Right);
	}

	void AppendCanonicalTags(
		TArray<FString>& Parts,
		const FGameplayTagContainer& Container)
	{
		TArray<FGameplayTag> Tags;
		Container.GetGameplayTagArray(Tags);
		Tags.Sort([](const FGameplayTag& Left, const FGameplayTag& Right)
		{
			return Left.GetTagName().LexicalLess(Right.GetTagName());
		});
		Parts.Add(FString::FromInt(Tags.Num()));
		for (const FGameplayTag& Tag : Tags)
		{
			Parts.Add(Tag.ToString());
		}
	}

	bool ActionsMatch(
		const FShanmenCombatActionSnapshot& Left,
		const FShanmenCombatActionSnapshot& Right)
	{
		return Left.IsValid() && Right.IsValid()
			&& Left.GetRunId() == Right.GetRunId()
			&& Left.GetOwnerId() == Right.GetOwnerId()
			&& Left.GetActivationId() == Right.GetActivationId()
			&& Left.GetSourceEntityId() == Right.GetSourceEntityId()
			&& Left.GetSourceItemInstanceId()
				== Right.GetSourceItemInstanceId()
			&& Left.GetActionDefinitionId()
				== Right.GetActionDefinitionId()
			&& Left.GetContent().Version == Right.GetContent().Version
			&& Left.GetContent().Digest == Right.GetContent().Digest
			&& Left.GetSourceTags() == Right.GetSourceTags();
	}

	FGuid MakeCommandId(
		const FShanmenCombatActionSnapshot& Action,
		const FShanmenDivineSenseDefinition& Definition,
		const FShanmenActionResourceCost& Cost,
		int32 ScanOrdinal,
		int32 SubjectActorBudget)
	{
		if (!Action.IsValid() || !Definition.IsValid() || !Cost.IsValid()
			|| Action.GetActionDefinitionId()
				!= FShanmenDivineSenseDefinition::CanonicalActionDefinitionId()
			|| Definition.GetActionDefinitionId()
				!= Action.GetActionDefinitionId()
			|| Cost.GetResourceChannel()
				!= FShanmenCombatRuntimeNativeTags::ResourceSpiritEnergy()
			|| ScanOrdinal < 0 || SubjectActorBudget < 0)
		{
			return FGuid();
		}

		TArray<FString> Parts = {
			GuidDigits(Action.GetRunId()),
			GuidDigits(Action.GetOwnerId()),
			GuidDigits(Action.GetActivationId()),
			GuidDigits(Action.GetSourceEntityId()),
			GuidDigits(Action.GetSourceItemInstanceId()),
			Action.GetActionDefinitionId().ToString(),
			Action.GetContent().Version.ToString(),
			Action.GetContent().Digest,
			GuidDigits(Definition.GetDefinitionId()),
			GuidDigits(Cost.GetCostId()),
			FString::FromInt(ScanOrdinal),
			FString::FromInt(SubjectActorBudget)
		};
		AppendCanonicalTags(Parts, Action.GetSourceTags());
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.Spell.DivineSense.PulseCommand.r1"), Parts);
	}

	FGuid MakeHostId(
		const Fdemo_mapShanmenDivineSensePulseCoordinator& Coordinator,
		const FShanmenActionResourceSnapshot& OpeningResourceSnapshot)
	{
		if (!Coordinator.IsValid() || !OpeningResourceSnapshot.IsValid())
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.Spell.DivineSense.ProductHost.r1"),
			{
				GuidDigits(Coordinator.GetCoordinatorId()),
				GuidDigits(OpeningResourceSnapshot.GetSnapshotId())
			});
	}

	bool SnapshotsMatch(
		const FShanmenActionResourceSnapshot& Left,
		const FShanmenActionResourceSnapshot& Right)
	{
		return Left.IsValid() && Right.IsValid()
			&& Left.GetSnapshotId() == Right.GetSnapshotId();
	}

	bool CommandMatchesReceipt(
		const Fdemo_mapShanmenDivineSensePulseCommand& Command,
		const Fdemo_mapShanmenDivineSensePulseReceipt& Receipt)
	{
		return Command.IsValid() && Receipt.IsValid()
			&& ActionsMatch(Command.GetAction(), Receipt.GetAction())
			&& Command.GetDefinition().GetDefinitionId()
				== Receipt.GetDefinition().GetDefinitionId()
			&& Command.GetCost().GetCostId()
				== Receipt.GetCost().GetCostId()
			&& Command.GetScanOrdinal() == Receipt.GetScanOrdinal()
			&& Command.GetSubjectActorBudget()
				== Receipt.GetSubjectActorBudget();
	}

	Fdemo_mapShanmenDivineSenseHostPulseResult RejectWithoutHost(
		Edemo_mapShanmenDivineSenseHostPulseError Error,
		const TCHAR* Diagnostic)
	{
		Fdemo_mapShanmenDivineSenseHostPulseResult Result;
		Result.Status =
			Edemo_mapShanmenDivineSenseHostPulseStatus::Rejected;
		Result.Error = Error;
		Result.Diagnostic = Diagnostic;
		return Result;
	}

	Fdemo_mapShanmenDivineSenseHostPulseResult RejectWithState(
		Edemo_mapShanmenDivineSenseHostPulseError Error,
		const TCHAR* Diagnostic,
		const FGuid& HostId,
		const FGuid& RunId,
		const Fdemo_mapShanmenDivineSensePulseCommand& Command,
		const FShanmenActionResourceSnapshot& ResourceSnapshot,
		const Fdemo_mapShanmenDivineSensePulseResult* Pulse = nullptr)
	{
		Fdemo_mapShanmenDivineSenseHostPulseResult Result;
		Result.Status =
			Edemo_mapShanmenDivineSenseHostPulseStatus::Rejected;
		Result.Error = Error;
		Result.Diagnostic = Diagnostic;
		Result.HostId = HostId;
		Result.RunId = RunId;
		Result.Command = Command;
		Result.ResourceBefore = ResourceSnapshot;
		Result.ResourceAfter = ResourceSnapshot;
		if (Pulse)
		{
			Result.Pulse = *Pulse;
		}
		return Result;
	}
}

bool Fdemo_mapShanmenDivineSensePulseCommand::TryCapture(
	const FShanmenCombatActionSnapshot& InAction,
	const FShanmenDivineSenseDefinition& InDefinition,
	const FShanmenActionResourceCost& InCost,
	int32 InScanOrdinal,
	int32 InSubjectActorBudget,
	Fdemo_mapShanmenDivineSensePulseCommand& OutCommand)
{
	OutCommand = Fdemo_mapShanmenDivineSensePulseCommand();
	Fdemo_mapShanmenDivineSensePulseCommand Candidate;
	Candidate.Action = InAction;
	Candidate.Definition = InDefinition;
	Candidate.Cost = InCost;
	Candidate.ScanOrdinal = InScanOrdinal;
	Candidate.SubjectActorBudget = InSubjectActorBudget;
	Candidate.CommandId = MakeCommandId(
		InAction,
		InDefinition,
		InCost,
		InScanOrdinal,
		InSubjectActorBudget);
	if (!Candidate.IsValid())
	{
		return false;
	}
	OutCommand = MoveTemp(Candidate);
	return true;
}

bool Fdemo_mapShanmenDivineSensePulseCommand::IsValid() const
{
	return CommandId.IsValid()
		&& CommandId == MakeCommandId(
			Action, Definition, Cost, ScanOrdinal, SubjectActorBudget);
}

bool Fdemo_mapShanmenDivineSenseHostPulseResult::IsValid() const
{
	if (Diagnostic.IsEmpty())
	{
		return false;
	}
	if (Status == Edemo_mapShanmenDivineSenseHostPulseStatus::Rejected)
	{
		if (Error == Edemo_mapShanmenDivineSenseHostPulseError::None)
		{
			return false;
		}
		if (Error == Edemo_mapShanmenDivineSenseHostPulseError::HostNotReady)
		{
			return !HostId.IsValid()
				&& !RunId.IsValid()
				&& !ResourceBefore.IsValid()
				&& !ResourceAfter.IsValid()
				&& !Pulse.IsValid();
		}
		if (!HostId.IsValid() || !RunId.IsValid()
			|| !ResourceBefore.IsValid()
			|| !SnapshotsMatch(ResourceBefore, ResourceAfter))
		{
			return false;
		}
		if (Error == Edemo_mapShanmenDivineSenseHostPulseError::InvalidCommand)
		{
			return !Command.IsValid() && !Pulse.IsValid();
		}
		if (!Command.IsValid())
		{
			return false;
		}
		if (Error == Edemo_mapShanmenDivineSenseHostPulseError::RunMismatch)
		{
			return Command.GetAction().GetRunId() != RunId
				&& !Pulse.IsValid();
		}
		if (Error
			== Edemo_mapShanmenDivineSenseHostPulseError::SourceMismatch)
		{
			return Command.GetAction().GetRunId() == RunId
				&& Command.GetAction().GetSourceEntityId()
					!= ResourceBefore.GetOwnerEntityId()
				&& !Pulse.IsValid();
		}
		if (Command.GetAction().GetRunId() != RunId
			|| Command.GetAction().GetSourceEntityId()
				!= ResourceBefore.GetOwnerEntityId())
		{
			return false;
		}
		if (Error == Edemo_mapShanmenDivineSenseHostPulseError::PulseRejected)
		{
			return Pulse.IsValid() && !Pulse.IsSuccess();
		}
		return !Pulse.IsValid();
	}

	if (Status != Edemo_mapShanmenDivineSenseHostPulseStatus::Applied
		&& Status
			!= Edemo_mapShanmenDivineSenseHostPulseStatus::AlreadyApplied)
	{
		return false;
	}
	if (Error != Edemo_mapShanmenDivineSenseHostPulseError::None
		|| !HostId.IsValid() || !RunId.IsValid() || !Command.IsValid()
		|| !ResourceBefore.IsValid() || !ResourceAfter.IsValid()
		|| !Pulse.IsSuccess()
		|| Command.GetAction().GetRunId() != RunId
		|| Command.GetAction().GetSourceEntityId()
			!= ResourceBefore.GetOwnerEntityId()
		|| !CommandMatchesReceipt(Command, Pulse.Receipt))
	{
		return false;
	}

	if (Status
		== Edemo_mapShanmenDivineSenseHostPulseStatus::AlreadyApplied)
	{
		return Pulse.IsReplay()
			&& SnapshotsMatch(ResourceBefore, ResourceAfter);
	}
	if (Pulse.IsReplay())
	{
		return false;
	}

	const FShanmenActionResourceReservationReceipt& Reservation =
		Pulse.Receipt.GetReservation();
	const FShanmenActionResourceFinalizationReceipt& Commit =
		Pulse.Receipt.GetResourceCommit();
	return ResourceBefore.GetSnapshotId()
			== Reservation.GetRequest().GetResourceSnapshot().GetSnapshotId()
		&& ResourceBefore.GetOwnerEntityId()
			== ResourceAfter.GetOwnerEntityId()
		&& ResourceBefore.GetResourceChannel()
			== ResourceAfter.GetResourceChannel()
		&& FloatsMatchExactly(
			ResourceBefore.GetMaximumAmount(),
			ResourceAfter.GetMaximumAmount())
		&& ResourceBefore.GetAuthorityRevision()
			== Reservation.GetAuthorityRevisionBefore()
		&& ResourceAfter.GetAuthorityRevision()
			== Commit.GetAuthorityRevisionAfter()
		&& ResourceBefore.GetAuthorityRevision() <= MAX_int64 - 2
		&& ResourceAfter.GetAuthorityRevision()
			== ResourceBefore.GetAuthorityRevision() + 2
		&& FloatsMatchExactly(
			ResourceAfter.GetCurrentAmount(), Commit.GetCurrentAfter())
		&& FloatsMatchExactly(
			ResourceAfter.GetMaximumAmount(), Commit.GetMaximumAmount())
		&& FloatsMatchExactly(
			ResourceAfter.GetReservedAmount(), Commit.GetReservedAfter())
		&& FloatsMatchExactly(
			ResourceAfter.GetAvailableAmount(), Commit.GetAvailableAfter());
}

bool Fdemo_mapShanmenDivineSenseHostPulseResult::IsSuccess() const
{
	return IsValid()
		&& (Status == Edemo_mapShanmenDivineSenseHostPulseStatus::Applied
			|| Status
				== Edemo_mapShanmenDivineSenseHostPulseStatus::AlreadyApplied);
}

bool Fdemo_mapShanmenDivineSenseProductHost::TryOpen(
	const FGuid& InRunId,
	const FGuid& SourceEntityId,
	float CurrentSpiritEnergy,
	float MaximumSpiritEnergy,
	int64 AuthorityRevision,
	int32 ProcessedPulseCapacity,
	Fdemo_mapShanmenDivineSenseProductHost& OutHost)
{
	OutHost.Reset();
	if (!InRunId.IsValid() || !SourceEntityId.IsValid()
		|| ProcessedPulseCapacity <= 0
		|| AuthorityRevision < 0
		|| static_cast<int64>(ProcessedPulseCapacity)
			> (MAX_int64 - AuthorityRevision) / 2)
	{
		return false;
	}

	Fdemo_mapShanmenDivineSenseProductHost Candidate;
	if (!FShanmenActionResourceAuthority::TryCreate(
			SourceEntityId,
			FShanmenCombatRuntimeNativeTags::ResourceSpiritEnergy(),
			CurrentSpiritEnergy,
			MaximumSpiritEnergy,
			AuthorityRevision,
			Candidate.ResourceAuthority)
		|| !Candidate.ResourceAuthority.TryCaptureSnapshot(
			Candidate.OpeningResourceSnapshot)
		|| !Fdemo_mapShanmenDivineSensePulseCoordinator::TryCreate(
			InRunId,
			ProcessedPulseCapacity,
			Candidate.Coordinator))
	{
		return false;
	}

	Candidate.HostId = MakeHostId(
		Candidate.Coordinator, Candidate.OpeningResourceSnapshot);
	Candidate.bInitialized = true;
	if (!Candidate.IsValid())
	{
		return false;
	}
	OutHost = MoveTemp(Candidate);
	return true;
}

Fdemo_mapShanmenDivineSenseHostPulseResult
Fdemo_mapShanmenDivineSenseProductHost::ExecutePulse(
	UWorld* World,
	const FShanmenWorldEntityRegistry& EntityRegistry,
	AActor* SourceActor,
	const Fdemo_mapShanmenDivineSensePulseCommand& Command,
	const TArray<AActor*>& SubjectActors,
	const Idemo_mapShanmenDivineSenseWorldEvidenceProvider& EvidenceProvider)
{
	if (!IsValid())
	{
		return RejectWithoutHost(
			Edemo_mapShanmenDivineSenseHostPulseError::HostNotReady,
			TEXT("Divine Sense product host is not ready."));
	}

	FShanmenActionResourceSnapshot Before;
	if (!ResourceAuthority.TryCaptureSnapshot(Before))
	{
		return RejectWithoutHost(
			Edemo_mapShanmenDivineSenseHostPulseError::
				ResourceSnapshotRejected,
			TEXT("Divine Sense host could not capture its resource state."));
	}
	if (!Command.IsValid())
	{
		return RejectWithState(
			Edemo_mapShanmenDivineSenseHostPulseError::InvalidCommand,
			TEXT("Divine Sense host requires one immutable pulse command."),
			HostId,
			GetRunId(),
			Command,
			Before);
	}
	if (Command.GetAction().GetRunId() != GetRunId())
	{
		return RejectWithState(
			Edemo_mapShanmenDivineSenseHostPulseError::RunMismatch,
			TEXT("Divine Sense pulse command belongs to another Run."),
			HostId,
			GetRunId(),
			Command,
			Before);
	}
	if (Command.GetAction().GetSourceEntityId() != GetSourceEntityId())
	{
		return RejectWithState(
			Edemo_mapShanmenDivineSenseHostPulseError::SourceMismatch,
			TEXT("Divine Sense pulse command belongs to another source entity."),
			HostId,
			GetRunId(),
			Command,
			Before);
	}

	Fdemo_mapShanmenDivineSenseProductHost Candidate = *this;
	Fdemo_mapShanmenDivineSensePulseResult Pulse =
		Candidate.Coordinator.Execute(
			World,
			EntityRegistry,
			SourceActor,
			Command.GetAction(),
			Command.GetDefinition(),
			Command.GetCost(),
			Command.GetScanOrdinal(),
			Command.GetSubjectActorBudget(),
			SubjectActors,
			EvidenceProvider,
			Candidate.ResourceAuthority);
	if (!Pulse.IsValid())
	{
		return RejectWithState(
			Edemo_mapShanmenDivineSenseHostPulseError::StateDesynchronized,
			TEXT("Divine Sense pulse coordinator returned invalid evidence."),
			HostId,
			GetRunId(),
			Command,
			Before);
	}
	if (!Pulse.IsSuccess())
	{
		return RejectWithState(
			Edemo_mapShanmenDivineSenseHostPulseError::PulseRejected,
			TEXT("Divine Sense pulse was rejected; host state was unchanged."),
			HostId,
			GetRunId(),
			Command,
			Before,
			&Pulse);
	}

	FShanmenActionResourceSnapshot After;
	if (!Candidate.IsValid()
		|| !Candidate.ResourceAuthority.TryCaptureSnapshot(After))
	{
		return RejectWithState(
			Edemo_mapShanmenDivineSenseHostPulseError::StateDesynchronized,
			TEXT("Divine Sense pulse could not publish a valid host state."),
			HostId,
			GetRunId(),
			Command,
			Before);
	}

	Fdemo_mapShanmenDivineSenseHostPulseResult Result;
	Result.Status = Pulse.IsReplay()
		? Edemo_mapShanmenDivineSenseHostPulseStatus::AlreadyApplied
		: Edemo_mapShanmenDivineSenseHostPulseStatus::Applied;
	Result.Error = Edemo_mapShanmenDivineSenseHostPulseError::None;
	Result.Diagnostic = Pulse.IsReplay()
		? TEXT("Exact Divine Sense host replay returned stored proof without resource or World mutation.")
		: TEXT("Divine Sense host atomically published resource and pulse proof.");
	Result.HostId = HostId;
	Result.RunId = GetRunId();
	Result.Command = Command;
	Result.ResourceBefore = Before;
	Result.ResourceAfter = After;
	Result.Pulse = MoveTemp(Pulse);
	if (!Result.IsValid())
	{
		return RejectWithState(
			Edemo_mapShanmenDivineSenseHostPulseError::StateDesynchronized,
			TEXT("Divine Sense host produced inconsistent product evidence."),
			HostId,
			GetRunId(),
			Command,
			Before);
	}

	*this = MoveTemp(Candidate);
	return Result;
}

bool Fdemo_mapShanmenDivineSenseProductHost::IsValid() const
{
	if (!bInitialized || !HostId.IsValid()
		|| !OpeningResourceSnapshot.IsValid()
		|| !ResourceAuthority.IsValid()
		|| !Coordinator.IsValid()
		|| ResourceAuthority.GetResourceChannel()
			!= FShanmenCombatRuntimeNativeTags::ResourceSpiritEnergy()
		|| OpeningResourceSnapshot.GetOwnerEntityId()
			!= ResourceAuthority.GetOwnerEntityId()
		|| OpeningResourceSnapshot.GetResourceChannel()
			!= ResourceAuthority.GetResourceChannel()
		|| !FloatsMatchExactly(
			OpeningResourceSnapshot.GetMaximumAmount(),
			ResourceAuthority.GetMaximumAmount())
		|| !FloatsMatchExactly(
			OpeningResourceSnapshot.GetReservedAmount(), 0.0f)
		|| ResourceAuthority.NumPendingReservations() != 0
		|| ResourceAuthority.NumTransactions()
			!= Coordinator.NumProcessedPulses()
		|| HostId != MakeHostId(Coordinator, OpeningResourceSnapshot))
		{
		return false;
	}

	const int64 Processed = Coordinator.NumProcessedPulses();
	const int64 OpeningRevision =
		OpeningResourceSnapshot.GetAuthorityRevision();
	return OpeningRevision <= MAX_int64 - (Processed * 2)
		&& ResourceAuthority.GetAuthorityRevision()
			== OpeningRevision + (Processed * 2);
}

void Fdemo_mapShanmenDivineSenseProductHost::Reset()
{
	*this = Fdemo_mapShanmenDivineSenseProductHost();
}

bool Fdemo_mapShanmenDivineSenseProductHost::TryCaptureResourceSnapshot(
	FShanmenActionResourceSnapshot& OutSnapshot) const
{
	OutSnapshot = FShanmenActionResourceSnapshot();
	return IsValid() && ResourceAuthority.TryCaptureSnapshot(OutSnapshot);
}
