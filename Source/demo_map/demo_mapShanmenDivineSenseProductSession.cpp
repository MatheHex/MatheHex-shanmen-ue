#include "demo_mapShanmenDivineSenseProductSession.h"

#include "ShanmenCombatRuntimeTags.h"
#include "ShanmenDeterministicId.h"
#include "demo_mapCombatRunCoordinator.h"

namespace
{
	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	bool FloatsMatchExactly(float Left, float Right)
	{
		uint32 LeftBits = 0;
		uint32 RightBits = 0;
		FPlatformMemory::Memcpy(&LeftBits, &Left, sizeof(LeftBits));
		FPlatformMemory::Memcpy(&RightBits, &Right, sizeof(RightBits));
		return LeftBits == RightBits;
	}

	bool SnapshotsMatch(
		const FShanmenActionResourceSnapshot& Left,
		const FShanmenActionResourceSnapshot& Right)
	{
		return Left.IsValid() && Right.IsValid()
			&& Left.GetSnapshotId() == Right.GetSnapshotId();
	}

	FGuid MakeSessionId(
		const FGuid& RunId,
		const FGuid& SourceEntityId,
		const FShanmenActionResourceSnapshot& OpeningSnapshot,
		const FGuid& HostId,
		const FGuid& RouterId)
	{
		if (!RunId.IsValid() || !SourceEntityId.IsValid()
			|| !OpeningSnapshot.IsValid() || !HostId.IsValid()
			|| !RouterId.IsValid())
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.Spell.DivineSense.ProductSession.r1"),
			{
				GuidDigits(RunId),
				GuidDigits(SourceEntityId),
				GuidDigits(OpeningSnapshot.GetSnapshotId()),
				GuidDigits(HostId),
				GuidDigits(RouterId)
			});
	}

	FGuid MakeProjectionId(
		const FGuid& SessionId,
		const FGuid& RunId,
		const FGuid& SourceEntityId,
		const FGuid& HostId,
		const FGuid& RouterId,
		const FShanmenActionResourceSnapshot& ResourceSnapshot,
		int32 ProcessedCommandCount,
		int32 ProcessedCommandCapacity)
	{
		if (!SessionId.IsValid() || !RunId.IsValid()
			|| !SourceEntityId.IsValid() || !HostId.IsValid()
			|| !RouterId.IsValid() || !ResourceSnapshot.IsValid()
			|| ProcessedCommandCount < 0
			|| ProcessedCommandCapacity <= 0
			|| ProcessedCommandCount > ProcessedCommandCapacity)
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.Spell.DivineSense.AvailabilityProjection.r1"),
			{
				GuidDigits(SessionId),
				GuidDigits(RunId),
				GuidDigits(SourceEntityId),
				GuidDigits(HostId),
				GuidDigits(RouterId),
				GuidDigits(ResourceSnapshot.GetSnapshotId()),
				FString::FromInt(ProcessedCommandCount),
				FString::FromInt(ProcessedCommandCapacity)
			});
	}

	FGuid MakeEndReceiptId(
		const FGuid& SessionId,
		const FGuid& RunId,
		const FGuid& SourceEntityId,
		const FGuid& HostId,
		const FGuid& RouterId,
		const FGuid& OpeningResourceSnapshotId,
		const FGuid& FinalResourceSnapshotId,
		int32 ProcessedCommandCount,
		int32 ProcessedCommandCapacity)
	{
		if (!SessionId.IsValid() || !RunId.IsValid()
			|| !SourceEntityId.IsValid() || !HostId.IsValid()
			|| !RouterId.IsValid()
			|| !OpeningResourceSnapshotId.IsValid()
			|| !FinalResourceSnapshotId.IsValid()
			|| ProcessedCommandCount < 0
			|| ProcessedCommandCapacity <= 0
			|| ProcessedCommandCount > ProcessedCommandCapacity)
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.Spell.DivineSense.SessionEndReceipt.r1"),
			{
				GuidDigits(SessionId),
				GuidDigits(RunId),
				GuidDigits(SourceEntityId),
				GuidDigits(HostId),
				GuidDigits(RouterId),
				GuidDigits(OpeningResourceSnapshotId),
				GuidDigits(FinalResourceSnapshotId),
				FString::FromInt(ProcessedCommandCount),
				FString::FromInt(ProcessedCommandCapacity)
			});
	}

	Fdemo_mapShanmenDivineSenseSessionRouteResult MakeRouteResult(
		Edemo_mapShanmenDivineSenseSessionRouteStatus Status,
		const TCHAR* Diagnostic,
		const FGuid& SessionId)
	{
		Fdemo_mapShanmenDivineSenseSessionRouteResult Result;
		Result.Status = Status;
		Result.Diagnostic = Diagnostic
			? Diagnostic
			: TEXT("Divine Sense product Session rejected the route.");
		Result.SessionId = SessionId;
		return Result;
	}

	Fdemo_mapShanmenDivineSenseSessionEndResult MakeEndResult(
		Edemo_mapShanmenDivineSenseSessionEndStatus Status,
		const TCHAR* Diagnostic)
	{
		Fdemo_mapShanmenDivineSenseSessionEndResult Result;
		Result.Status = Status;
		Result.Diagnostic = Diagnostic
			? Diagnostic
			: TEXT("Divine Sense product Session rejected teardown.");
		return Result;
	}
}

bool Fdemo_mapShanmenDivineSenseAvailabilityProjection::IsValid() const
{
	return ProjectionId.IsValid() && SessionId.IsValid()
		&& RunId.IsValid() && SourceEntityId.IsValid()
		&& HostId.IsValid() && RouterId.IsValid()
		&& ResourceSnapshot.IsValid()
		&& ResourceSnapshot.GetOwnerEntityId() == SourceEntityId
		&& ResourceSnapshot.GetResourceChannel()
			== FShanmenCombatRuntimeNativeTags::ResourceSpiritEnergy()
		&& ProcessedCommandCount >= 0
		&& ProcessedCommandCapacity > 0
		&& ProcessedCommandCount <= ProcessedCommandCapacity
		&& ProjectionId == MakeProjectionId(
			SessionId,
			RunId,
			SourceEntityId,
			HostId,
			RouterId,
			ResourceSnapshot,
			ProcessedCommandCount,
			ProcessedCommandCapacity);
}

bool Fdemo_mapShanmenDivineSenseAvailabilityProjection::Matches(
	const Fdemo_mapShanmenDivineSenseAvailabilityProjection& Other) const
{
	return IsValid() && Other.IsValid()
		&& ProjectionId == Other.ProjectionId
		&& SessionId == Other.SessionId
		&& RunId == Other.RunId
		&& SourceEntityId == Other.SourceEntityId
		&& HostId == Other.HostId
		&& RouterId == Other.RouterId
		&& ResourceSnapshot.GetSnapshotId()
			== Other.ResourceSnapshot.GetSnapshotId()
		&& ProcessedCommandCount == Other.ProcessedCommandCount
		&& ProcessedCommandCapacity == Other.ProcessedCommandCapacity;
}

bool Fdemo_mapShanmenDivineSenseAvailabilityProjection::CanAfford(
	const FShanmenActionResourceCost& Cost) const
{
	return HasRouteCapacity() && Cost.IsValid()
		&& Cost.GetResourceChannel()
			== FShanmenCombatRuntimeNativeTags::ResourceSpiritEnergy()
		&& Cost.GetAmount() <= ResourceSnapshot.GetAvailableAmount();
}

bool Fdemo_mapShanmenDivineSenseSessionRouteResult::IsValid() const
{
	if (Status == Edemo_mapShanmenDivineSenseSessionRouteStatus::Invalid
		|| Diagnostic.IsEmpty())
	{
		return false;
	}

	if (Status == Edemo_mapShanmenDivineSenseSessionRouteStatus::Applied
		|| Status
			== Edemo_mapShanmenDivineSenseSessionRouteStatus::AlreadyApplied)
	{
		if (!SessionId.IsValid() || !AvailabilityBefore.IsValid()
			|| !AvailabilityAfter.IsValid()
			|| AvailabilityBefore.GetSessionId() != SessionId
			|| AvailabilityAfter.GetSessionId() != SessionId
			|| !Route.IsAccepted())
		{
			return false;
		}
		if (Status
			== Edemo_mapShanmenDivineSenseSessionRouteStatus::AlreadyApplied)
		{
			return Route.IsReplay()
				&& AvailabilityBefore.Matches(AvailabilityAfter);
		}
		return !Route.IsReplay()
			&& AvailabilityBefore.GetProcessedCommandCount()
				< AvailabilityBefore.GetProcessedCommandCapacity()
			&& AvailabilityAfter.GetProcessedCommandCount()
				== AvailabilityBefore.GetProcessedCommandCount() + 1
			&& AvailabilityBefore.GetProcessedCommandCapacity()
				== AvailabilityAfter.GetProcessedCommandCapacity()
			&& Route.HostPulse.ResourceBefore.GetSnapshotId()
				== AvailabilityBefore.GetResourceSnapshot().GetSnapshotId()
			&& Route.HostPulse.ResourceAfter.GetSnapshotId()
				== AvailabilityAfter.GetResourceSnapshot().GetSnapshotId();
	}

	if (Status
		== Edemo_mapShanmenDivineSenseSessionRouteStatus::RouteRejected)
	{
		return SessionId.IsValid() && AvailabilityBefore.IsValid()
			&& AvailabilityAfter.IsValid()
			&& AvailabilityBefore.Matches(AvailabilityAfter)
			&& Route.IsValid() && !Route.IsAccepted();
	}

	return !Route.IsValid();
}

bool Fdemo_mapShanmenDivineSenseSessionRouteResult::IsAccepted() const
{
	return IsValid()
		&& (Status == Edemo_mapShanmenDivineSenseSessionRouteStatus::Applied
			|| Status
				== Edemo_mapShanmenDivineSenseSessionRouteStatus::
					AlreadyApplied);
}

bool Fdemo_mapShanmenDivineSenseSessionEndReceipt::IsValid() const
{
	return ReceiptId.IsValid() && SessionId.IsValid() && RunId.IsValid()
		&& SourceEntityId.IsValid() && HostId.IsValid() && RouterId.IsValid()
		&& OpeningResourceSnapshotId.IsValid()
		&& FinalResourceSnapshotId.IsValid()
		&& ProcessedCommandCount >= 0
		&& ProcessedCommandCapacity > 0
		&& ProcessedCommandCount <= ProcessedCommandCapacity
		&& ReceiptId == MakeEndReceiptId(
			SessionId,
			RunId,
			SourceEntityId,
			HostId,
			RouterId,
			OpeningResourceSnapshotId,
			FinalResourceSnapshotId,
			ProcessedCommandCount,
			ProcessedCommandCapacity);
}

bool Fdemo_mapShanmenDivineSenseSessionEndResult::IsValid() const
{
	if (Status == Edemo_mapShanmenDivineSenseSessionEndStatus::Invalid
		|| Diagnostic.IsEmpty())
	{
		return false;
	}
	if (Status == Edemo_mapShanmenDivineSenseSessionEndStatus::Ended
		|| Status
			== Edemo_mapShanmenDivineSenseSessionEndStatus::AlreadyEnded)
	{
		return Receipt.IsValid();
	}
	return !Receipt.IsValid();
}

bool Fdemo_mapShanmenDivineSenseSessionEndResult::IsSuccess() const
{
	return IsValid()
		&& (Status == Edemo_mapShanmenDivineSenseSessionEndStatus::Ended
			|| Status
				== Edemo_mapShanmenDivineSenseSessionEndStatus::AlreadyEnded);
}

bool Fdemo_mapShanmenDivineSenseProductSession::TryBegin(
	const Fdemo_mapCombatRunCoordinator& Coordinator,
	const FShanmenActionResourceSnapshot& OpeningSpiritEnergy,
	int32 ProcessedPulseCapacity,
	FString& OutDiagnostic)
{
	OutDiagnostic.Reset();
	if (!IsValid())
	{
		OutDiagnostic = TEXT("Divine Sense Session begin requires valid local state.");
		return false;
	}
	if (IsActive())
	{
		if (IsConsistentWithCoordinator(Coordinator)
			&& OpeningResourceSnapshot.GetSnapshotId()
				== OpeningSpiritEnergy.GetSnapshotId()
			&& Router.GetProcessedCommandCapacity()
				== ProcessedPulseCapacity)
		{
			OutDiagnostic = TEXT("Exact Divine Sense Session begin is already active.");
			return true;
		}
		OutDiagnostic = TEXT("Divine Sense Session already owns another active binding.");
		return false;
	}
	if (IsEnded())
	{
		OutDiagnostic = TEXT("Ended Divine Sense Session must be reset before reuse.");
		return false;
	}
	if (!Coordinator.IsReady())
	{
		OutDiagnostic = TEXT("Divine Sense Session requires one ready Combat Run.");
		return false;
	}
	if (!OpeningSpiritEnergy.IsValid()
		|| ProcessedPulseCapacity <= 0)
	{
		OutDiagnostic = TEXT("Divine Sense Session requires a valid opening snapshot and positive capacity.");
		return false;
	}
	if (OpeningSpiritEnergy.GetOwnerEntityId()
		!= Coordinator.GetPlayerEntityId())
	{
		OutDiagnostic = TEXT("Opening SpiritEnergy snapshot does not belong to the Combat Run player.");
		return false;
	}
	if (OpeningSpiritEnergy.GetResourceChannel()
		!= FShanmenCombatRuntimeNativeTags::ResourceSpiritEnergy())
	{
		OutDiagnostic = TEXT("Divine Sense Session accepts only the SpiritEnergy channel.");
		return false;
	}
	if (!FloatsMatchExactly(
		OpeningSpiritEnergy.GetReservedAmount(), 0.0f))
	{
		OutDiagnostic = TEXT("Opening SpiritEnergy handoff cannot contain reservations.");
		return false;
	}

	Fdemo_mapShanmenDivineSenseProductSession Candidate;
	Candidate.RunId = Coordinator.GetRunId();
	Candidate.SourceEntityId = Coordinator.GetPlayerEntityId();
	Candidate.OpeningResourceSnapshot = OpeningSpiritEnergy;
	if (!Fdemo_mapShanmenDivineSenseProductHost::TryOpen(
			Candidate.RunId,
			Candidate.SourceEntityId,
			OpeningSpiritEnergy.GetCurrentAmount(),
			OpeningSpiritEnergy.GetMaximumAmount(),
			OpeningSpiritEnergy.GetAuthorityRevision(),
			ProcessedPulseCapacity,
			Candidate.Host)
		|| !SnapshotsMatch(
			Candidate.Host.GetOpeningResourceSnapshot(),
			OpeningSpiritEnergy))
	{
		OutDiagnostic = TEXT("Divine Sense Session could not open the exact SpiritEnergy Host.");
		return false;
	}
	if (!Fdemo_mapShanmenDivineSenseCommandRouter::TryCreate(
		Candidate.Host, Candidate.Router))
	{
		OutDiagnostic = TEXT("Divine Sense Session could not create its command Router.");
		return false;
	}
	Candidate.SessionId = MakeSessionId(
		Candidate.RunId,
		Candidate.SourceEntityId,
		Candidate.OpeningResourceSnapshot,
		Candidate.Host.GetHostId(),
		Candidate.Router.GetRouterId());
	Candidate.State = Edemo_mapShanmenDivineSenseProductSessionState::Active;
	if (!Candidate.IsValid()
		|| !Candidate.IsConsistentWithCoordinator(Coordinator))
	{
		OutDiagnostic = TEXT("Divine Sense Session failed closed after Run binding.");
		return false;
	}

	*this = MoveTemp(Candidate);
	OutDiagnostic = TEXT("Divine Sense Session bound Host and Router to the ready Combat Run.");
	return true;
}

bool Fdemo_mapShanmenDivineSenseProductSession::TryCaptureAvailability(
	const Fdemo_mapCombatRunCoordinator& Coordinator,
	Fdemo_mapShanmenDivineSenseAvailabilityProjection& OutProjection,
	FString& OutDiagnostic) const
{
	OutProjection = Fdemo_mapShanmenDivineSenseAvailabilityProjection();
	OutDiagnostic.Reset();
	if (!IsActive())
	{
		OutDiagnostic = TEXT("Divine Sense availability requires an active Session.");
		return false;
	}
	if (!IsValid())
	{
		OutDiagnostic = TEXT("Divine Sense availability rejected invalid Session state.");
		return false;
	}
	if (!Coordinator.IsReady())
	{
		OutDiagnostic = TEXT("Divine Sense availability requires a ready Combat Run.");
		return false;
	}
	if (Coordinator.GetRunId() != RunId)
	{
		OutDiagnostic = TEXT("Divine Sense availability cannot cross Run identity.");
		return false;
	}
	if (Coordinator.GetPlayerEntityId() != SourceEntityId)
	{
		OutDiagnostic = TEXT("Divine Sense availability cannot cross player identity.");
		return false;
	}
	if (!TryBuildAvailability(OutProjection))
	{
		OutDiagnostic = TEXT("Divine Sense Session could not build a valid availability projection.");
		return false;
	}
	OutDiagnostic = TEXT("Captured current read-only Divine Sense availability.");
	return true;
}

bool Fdemo_mapShanmenDivineSenseProductSession::IsAvailabilityCurrent(
	const Fdemo_mapCombatRunCoordinator& Coordinator,
	const Fdemo_mapShanmenDivineSenseAvailabilityProjection& Projection) const
{
	Fdemo_mapShanmenDivineSenseAvailabilityProjection Current;
	FString Diagnostic;
	return Projection.IsValid()
		&& TryCaptureAvailability(Coordinator, Current, Diagnostic)
		&& Current.Matches(Projection);
}

bool Fdemo_mapShanmenDivineSenseProductSession::TryCaptureCommand(
	const Fdemo_mapCombatRunCoordinator& Coordinator,
	const FShanmenCombatActionSnapshot& Action,
	const FShanmenDivineSenseDefinition& Definition,
	const FShanmenActionResourceCost& Cost,
	int32 ScanOrdinal,
	int32 SubjectActorBudget,
	const TArray<AActor*>& SubjectActors,
	Fdemo_mapShanmenDivineSenseRouteCommand& OutCommand,
	FString& OutDiagnostic) const
{
	OutCommand = Fdemo_mapShanmenDivineSenseRouteCommand();
	OutDiagnostic.Reset();
	if (!IsActive() || !IsValid())
	{
		OutDiagnostic = TEXT("Divine Sense command capture requires one valid active Session.");
		return false;
	}
	if (!Coordinator.IsReady())
	{
		OutDiagnostic = TEXT("Divine Sense command capture requires a ready Combat Run.");
		return false;
	}
	if (Coordinator.GetRunId() != RunId)
	{
		OutDiagnostic = TEXT("Divine Sense command capture cannot cross Run identity.");
		return false;
	}
	if (Coordinator.GetPlayerEntityId() != SourceEntityId)
	{
		OutDiagnostic = TEXT("Divine Sense command capture cannot cross player identity.");
		return false;
	}
	if (!Router.TryCaptureCommand(
			Host,
			Coordinator.GetEntityRegistry(),
			Action,
			Definition,
			Cost,
			ScanOrdinal,
			SubjectActorBudget,
			SubjectActors,
			OutCommand))
	{
		OutDiagnostic = Router.NumProcessedCommands()
			>= Router.GetProcessedCommandCapacity()
			? TEXT("Divine Sense command capacity is exhausted.")
			: TEXT("Divine Sense command capture failed its Run, payload, Actor or projection fence.");
		return false;
	}
	OutDiagnostic = TEXT("Captured one immutable Divine Sense Session command.");
	return true;
}

Fdemo_mapShanmenDivineSenseSessionRouteResult
Fdemo_mapShanmenDivineSenseProductSession::TryRoute(
	const Fdemo_mapCombatRunCoordinator& Coordinator,
	UWorld* World,
	AActor* SourceActor,
	const Fdemo_mapShanmenDivineSenseRouteCommand& Command,
	const TArray<AActor*>& SubjectActors,
	const Idemo_mapShanmenDivineSenseWorldEvidenceProvider& EvidenceProvider)
{
	if (!IsActive())
	{
		return MakeRouteResult(
			Edemo_mapShanmenDivineSenseSessionRouteStatus::SessionNotActive,
			TEXT("Divine Sense route requires an active product Session."),
			SessionId);
	}
	if (!IsValid())
	{
		return MakeRouteResult(
			Edemo_mapShanmenDivineSenseSessionRouteStatus::SessionInvalid,
			TEXT("Divine Sense route rejected invalid Session state."),
			SessionId);
	}
	if (!Coordinator.IsReady())
	{
		return MakeRouteResult(
			Edemo_mapShanmenDivineSenseSessionRouteStatus::CoordinatorNotReady,
			TEXT("Divine Sense route requires a ready Combat Run."),
			SessionId);
	}
	if (Coordinator.GetRunId() != RunId)
	{
		return MakeRouteResult(
			Edemo_mapShanmenDivineSenseSessionRouteStatus::RunMismatch,
			TEXT("Divine Sense product Session cannot route across Runs."),
			SessionId);
	}
	if (Coordinator.GetPlayerEntityId() != SourceEntityId)
	{
		return MakeRouteResult(
			Edemo_mapShanmenDivineSenseSessionRouteStatus::SourceMismatch,
			TEXT("Divine Sense product Session cannot route across player identities."),
			SessionId);
	}
	if (!Command.IsValid())
	{
		return MakeRouteResult(
			Edemo_mapShanmenDivineSenseSessionRouteStatus::CommandInvalid,
			TEXT("Divine Sense product Session requires an immutable route command."),
			SessionId);
	}

	Fdemo_mapShanmenDivineSenseAvailabilityProjection Before;
	if (!TryBuildAvailability(Before))
	{
		return MakeRouteResult(
			Edemo_mapShanmenDivineSenseSessionRouteStatus::StateDesynchronized,
			TEXT("Divine Sense Session could not capture its pre-route projection."),
			SessionId);
	}

	Fdemo_mapShanmenDivineSenseProductSession Candidate = *this;
	Fdemo_mapShanmenDivineSenseCommandRouteResult Routed =
		Candidate.Router.TryRoute(
			Candidate.Host,
			World,
			Coordinator.GetEntityRegistry(),
			SourceActor,
			Command,
			SubjectActors,
			EvidenceProvider);
	if (!Routed.IsValid())
	{
		Fdemo_mapShanmenDivineSenseSessionRouteResult Result =
			MakeRouteResult(
				Edemo_mapShanmenDivineSenseSessionRouteStatus::
					StateDesynchronized,
				TEXT("Divine Sense Router returned invalid Session evidence."),
				SessionId);
		Result.AvailabilityBefore = Before;
		return Result;
	}

	if (!Routed.IsAccepted())
	{
		Fdemo_mapShanmenDivineSenseSessionRouteResult Result =
			MakeRouteResult(
				Edemo_mapShanmenDivineSenseSessionRouteStatus::RouteRejected,
				TEXT("Divine Sense Session preserved state after Router rejection."),
				SessionId);
		Result.AvailabilityBefore = Before;
		Result.AvailabilityAfter = Before;
		Result.Route = MoveTemp(Routed);
		return Result;
	}

	Fdemo_mapShanmenDivineSenseAvailabilityProjection After;
	if (!Candidate.IsValid()
		|| !Candidate.IsConsistentWithCoordinator(Coordinator)
		|| !Candidate.TryBuildAvailability(After))
	{
		Fdemo_mapShanmenDivineSenseSessionRouteResult Result =
			MakeRouteResult(
				Edemo_mapShanmenDivineSenseSessionRouteStatus::
					StateDesynchronized,
				TEXT("Divine Sense Session could not publish a valid post-route state."),
				SessionId);
		Result.AvailabilityBefore = Before;
		return Result;
	}

	const bool bReplay = Routed.IsReplay();
	if ((bReplay && !Before.Matches(After))
		|| (!bReplay
			&& (After.GetProcessedCommandCount()
					!= Before.GetProcessedCommandCount() + 1
				|| Routed.HostPulse.ResourceBefore.GetSnapshotId()
					!= Before.GetResourceSnapshot().GetSnapshotId()
				|| Routed.HostPulse.ResourceAfter.GetSnapshotId()
					!= After.GetResourceSnapshot().GetSnapshotId())))
	{
		Fdemo_mapShanmenDivineSenseSessionRouteResult Result =
			MakeRouteResult(
				Edemo_mapShanmenDivineSenseSessionRouteStatus::
					StateDesynchronized,
				TEXT("Divine Sense Session route disagrees with availability evidence."),
				SessionId);
		Result.AvailabilityBefore = Before;
		return Result;
	}

	Fdemo_mapShanmenDivineSenseSessionRouteResult Result =
		MakeRouteResult(
			bReplay
				? Edemo_mapShanmenDivineSenseSessionRouteStatus::AlreadyApplied
				: Edemo_mapShanmenDivineSenseSessionRouteStatus::Applied,
			bReplay
				? TEXT("Exact Divine Sense Session replay returned retained proof without state or live evidence reads.")
				: TEXT("Divine Sense Session atomically published Host, Router and availability state."),
			SessionId);
	Result.AvailabilityBefore = Before;
	Result.AvailabilityAfter = After;
	Result.Route = MoveTemp(Routed);
	if (!Result.IsValid())
	{
		return MakeRouteResult(
			Edemo_mapShanmenDivineSenseSessionRouteStatus::StateDesynchronized,
			TEXT("Divine Sense Session produced inconsistent route proof."),
			SessionId);
	}
	if (!bReplay)
	{
		*this = MoveTemp(Candidate);
	}
	return Result;
}

Fdemo_mapShanmenDivineSenseSessionEndResult
Fdemo_mapShanmenDivineSenseProductSession::TryEnd(
	const Fdemo_mapCombatRunCoordinator& Coordinator,
	const FGuid& ExpectedSessionId)
{
	if (!IsValid())
	{
		return MakeEndResult(
			Edemo_mapShanmenDivineSenseSessionEndStatus::SessionInvalid,
			TEXT("Divine Sense Session teardown rejected invalid local state."));
	}
	if (IsEmpty())
	{
		return MakeEndResult(
			Edemo_mapShanmenDivineSenseSessionEndStatus::SessionNotActive,
			TEXT("Divine Sense Session is already empty."));
	}
	if (!ExpectedSessionId.IsValid() || ExpectedSessionId != SessionId)
	{
		return MakeEndResult(
			Edemo_mapShanmenDivineSenseSessionEndStatus::SessionMismatch,
			TEXT("Divine Sense Session teardown requires its exact SessionId."));
	}
	if (IsEnded())
	{
		Fdemo_mapShanmenDivineSenseSessionEndResult Replay = MakeEndResult(
			Edemo_mapShanmenDivineSenseSessionEndStatus::AlreadyEnded,
			TEXT("Exact Divine Sense Session teardown replay returned retained proof."));
		Replay.Receipt = EndReceipt;
		return Replay;
	}
	if (!Coordinator.IsReady())
	{
		return MakeEndResult(
			Edemo_mapShanmenDivineSenseSessionEndStatus::CoordinatorNotReady,
			TEXT("Active Divine Sense Session must end before its Combat Run."));
	}
	if (Coordinator.GetRunId() != RunId)
	{
		return MakeEndResult(
			Edemo_mapShanmenDivineSenseSessionEndStatus::RunMismatch,
			TEXT("Divine Sense Session teardown cannot cross Run identity."));
	}
	if (Coordinator.GetPlayerEntityId() != SourceEntityId)
	{
		return MakeEndResult(
			Edemo_mapShanmenDivineSenseSessionEndStatus::SourceMismatch,
			TEXT("Divine Sense Session teardown cannot cross player identity."));
	}

	FShanmenActionResourceSnapshot FinalSnapshot;
	if (!Host.TryCaptureResourceSnapshot(FinalSnapshot))
	{
		return MakeEndResult(
			Edemo_mapShanmenDivineSenseSessionEndStatus::StateDesynchronized,
			TEXT("Divine Sense Session could not capture final resource proof."));
	}

	Fdemo_mapShanmenDivineSenseSessionEndReceipt Receipt;
	Receipt.SessionId = SessionId;
	Receipt.RunId = RunId;
	Receipt.SourceEntityId = SourceEntityId;
	Receipt.HostId = Host.GetHostId();
	Receipt.RouterId = Router.GetRouterId();
	Receipt.OpeningResourceSnapshotId =
		OpeningResourceSnapshot.GetSnapshotId();
	Receipt.FinalResourceSnapshotId = FinalSnapshot.GetSnapshotId();
	Receipt.ProcessedCommandCount = Router.NumProcessedCommands();
	Receipt.ProcessedCommandCapacity =
		Router.GetProcessedCommandCapacity();
	Receipt.ReceiptId = MakeEndReceiptId(
		Receipt.SessionId,
		Receipt.RunId,
		Receipt.SourceEntityId,
		Receipt.HostId,
		Receipt.RouterId,
		Receipt.OpeningResourceSnapshotId,
		Receipt.FinalResourceSnapshotId,
		Receipt.ProcessedCommandCount,
		Receipt.ProcessedCommandCapacity);
	if (!Receipt.IsValid())
	{
		return MakeEndResult(
			Edemo_mapShanmenDivineSenseSessionEndStatus::StateDesynchronized,
			TEXT("Divine Sense Session could not create terminal proof."));
	}

	Fdemo_mapShanmenDivineSenseProductSession Candidate = *this;
	Candidate.EndReceipt = Receipt;
	Candidate.State = Edemo_mapShanmenDivineSenseProductSessionState::Ended;
	if (!Candidate.IsValid())
	{
		return MakeEndResult(
			Edemo_mapShanmenDivineSenseSessionEndStatus::StateDesynchronized,
			TEXT("Divine Sense Session failed terminal-state validation."));
	}

	Fdemo_mapShanmenDivineSenseSessionEndResult Result = MakeEndResult(
		Edemo_mapShanmenDivineSenseSessionEndStatus::Ended,
		TEXT("Divine Sense Session ended with immutable Host and Router proof."));
	Result.Receipt = Receipt;
	if (!Result.IsValid())
	{
		return MakeEndResult(
			Edemo_mapShanmenDivineSenseSessionEndStatus::StateDesynchronized,
			TEXT("Divine Sense Session produced invalid teardown evidence."));
	}

	*this = MoveTemp(Candidate);
	return Result;
}

bool Fdemo_mapShanmenDivineSenseProductSession::Reset()
{
	if (!IsValid() || IsActive())
	{
		return false;
	}
	*this = Fdemo_mapShanmenDivineSenseProductSession();
	return IsValid() && IsEmpty();
}

bool Fdemo_mapShanmenDivineSenseProductSession::IsValid() const
{
	if (IsEmpty())
	{
		return !SessionId.IsValid() && !RunId.IsValid()
			&& !SourceEntityId.IsValid()
			&& !OpeningResourceSnapshot.IsValid()
			&& !Host.IsValid() && !Router.IsValid()
			&& !EndReceipt.IsValid();
	}

	if (!SessionId.IsValid() || !RunId.IsValid()
		|| !SourceEntityId.IsValid()
		|| !OpeningResourceSnapshot.IsValid()
		|| !Host.IsValid() || !Router.IsValid()
		|| OpeningResourceSnapshot.GetOwnerEntityId() != SourceEntityId
		|| OpeningResourceSnapshot.GetResourceChannel()
			!= FShanmenCombatRuntimeNativeTags::ResourceSpiritEnergy()
		|| !FloatsMatchExactly(
			OpeningResourceSnapshot.GetReservedAmount(), 0.0f)
		|| Host.GetRunId() != RunId
		|| Host.GetSourceEntityId() != SourceEntityId
		|| !SnapshotsMatch(
			Host.GetOpeningResourceSnapshot(), OpeningResourceSnapshot)
		|| !Router.IsConsistentWithHost(Host)
		|| SessionId != MakeSessionId(
			RunId,
			SourceEntityId,
			OpeningResourceSnapshot,
			Host.GetHostId(),
			Router.GetRouterId()))
	{
		return false;
	}

	if (IsActive())
	{
		return !EndReceipt.IsValid();
	}
	if (!IsEnded() || !EndReceipt.IsValid())
	{
		return false;
	}

	FShanmenActionResourceSnapshot FinalSnapshot;
	return Host.TryCaptureResourceSnapshot(FinalSnapshot)
		&& EndReceipt.GetSessionId() == SessionId
		&& EndReceipt.GetRunId() == RunId
		&& EndReceipt.GetSourceEntityId() == SourceEntityId
		&& EndReceipt.GetHostId() == Host.GetHostId()
		&& EndReceipt.GetRouterId() == Router.GetRouterId()
		&& EndReceipt.GetOpeningResourceSnapshotId()
			== OpeningResourceSnapshot.GetSnapshotId()
		&& EndReceipt.GetFinalResourceSnapshotId()
			== FinalSnapshot.GetSnapshotId()
		&& EndReceipt.GetProcessedCommandCount()
			== Router.NumProcessedCommands()
		&& EndReceipt.GetProcessedCommandCapacity()
			== Router.GetProcessedCommandCapacity();
}

bool Fdemo_mapShanmenDivineSenseProductSession::
	IsConsistentWithCoordinator(
		const Fdemo_mapCombatRunCoordinator& Coordinator) const
{
	return IsActive() && IsValid() && Coordinator.IsReady()
		&& Coordinator.GetRunId() == RunId
		&& Coordinator.GetPlayerEntityId() == SourceEntityId
		&& Coordinator.GetEntityRegistry().GetRunId() == RunId;
}

bool Fdemo_mapShanmenDivineSenseProductSession::TryBuildAvailability(
	Fdemo_mapShanmenDivineSenseAvailabilityProjection& OutProjection) const
{
	OutProjection = Fdemo_mapShanmenDivineSenseAvailabilityProjection();
	if (!IsActive() || !IsValid())
	{
		return false;
	}

	FShanmenActionResourceSnapshot ResourceSnapshot;
	if (!Host.TryCaptureResourceSnapshot(ResourceSnapshot)
		|| !SnapshotsMatch(
			ResourceSnapshot, Router.GetCurrentResourceSnapshot()))
	{
		return false;
	}

	Fdemo_mapShanmenDivineSenseAvailabilityProjection Candidate;
	Candidate.SessionId = SessionId;
	Candidate.RunId = RunId;
	Candidate.SourceEntityId = SourceEntityId;
	Candidate.HostId = Host.GetHostId();
	Candidate.RouterId = Router.GetRouterId();
	Candidate.ResourceSnapshot = ResourceSnapshot;
	Candidate.ProcessedCommandCount = Router.NumProcessedCommands();
	Candidate.ProcessedCommandCapacity =
		Router.GetProcessedCommandCapacity();
	Candidate.ProjectionId = MakeProjectionId(
		Candidate.SessionId,
		Candidate.RunId,
		Candidate.SourceEntityId,
		Candidate.HostId,
		Candidate.RouterId,
		Candidate.ResourceSnapshot,
		Candidate.ProcessedCommandCount,
		Candidate.ProcessedCommandCapacity);
	if (!Candidate.IsValid())
	{
		return false;
	}
	OutProjection = MoveTemp(Candidate);
	return true;
}
