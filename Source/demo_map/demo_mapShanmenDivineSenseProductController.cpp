#include "demo_mapShanmenDivineSenseProductController.h"

#include "ShanmenCombatRuntimeTags.h"
#include "ShanmenDeterministicId.h"
#include "demo_mapCombatRunCoordinator.h"

namespace
{
	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
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

	FGuid MakeConfigId(
		const FShanmenDivineSenseDefinition& Definition,
		const FShanmenActionResourceCost& Cost,
		int32 PulseCapacity)
	{
		if (!Definition.IsValid() || !Cost.IsValid()
			|| Definition.GetActionDefinitionId()
				!= FShanmenDivineSenseDefinition::CanonicalActionDefinitionId()
			|| Cost.GetResourceChannel()
				!= FShanmenCombatRuntimeNativeTags::ResourceSpiritEnergy()
			|| PulseCapacity <= 0)
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.Spell.DivineSense.ProductConfig.r1"),
			{
				GuidDigits(Definition.GetDefinitionId()),
				GuidDigits(Cost.GetCostId()),
				FString::FromInt(PulseCapacity)
			});
	}

	FGuid MakeControllerId(
		const FGuid& RunId,
		const FGuid& SourceEntityId,
		const FGuid& ConfigId,
		const FGuid& SessionId,
		const FGuid& OpeningResourceSnapshotId)
	{
		if (!RunId.IsValid() || !SourceEntityId.IsValid()
			|| !ConfigId.IsValid() || !SessionId.IsValid()
			|| !OpeningResourceSnapshotId.IsValid())
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.Spell.DivineSense.ProductController.r1"),
			{
				GuidDigits(RunId),
				GuidDigits(SourceEntityId),
				GuidDigits(ConfigId),
				GuidDigits(SessionId),
				GuidDigits(OpeningResourceSnapshotId)
			});
	}

	FGuid MakeAvailabilityId(
		const FGuid& ControllerId,
		const FGuid& ConfigId,
		const FGuid& SessionProjectionId,
		int32 CapturedIntentCount)
	{
		if (!ControllerId.IsValid() || !ConfigId.IsValid()
			|| !SessionProjectionId.IsValid()
			|| CapturedIntentCount < 0)
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.Spell.DivineSense.ProductAvailability.r1"),
			{
				GuidDigits(ControllerId),
				GuidDigits(ConfigId),
				GuidDigits(SessionProjectionId),
				FString::FromInt(CapturedIntentCount)
			});
	}

	FGuid MakeEndReceiptId(
		const FGuid& ControllerId,
		const FGuid& ConfigId,
		const FGuid& RunId,
		const FGuid& SourceEntityId,
		const FGuid& SessionEndReceiptId,
		int32 CapturedIntentCount)
	{
		if (!ControllerId.IsValid() || !ConfigId.IsValid()
			|| !RunId.IsValid() || !SourceEntityId.IsValid()
			|| !SessionEndReceiptId.IsValid()
			|| CapturedIntentCount < 0)
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.Spell.DivineSense.ProductControllerEnd.r1"),
			{
				GuidDigits(ControllerId),
				GuidDigits(ConfigId),
				GuidDigits(RunId),
				GuidDigits(SourceEntityId),
				GuidDigits(SessionEndReceiptId),
				FString::FromInt(CapturedIntentCount)
			});
	}

	bool CommandMatches(
		const Fdemo_mapShanmenDivineSenseRouteCommand& Command,
		const Fdemo_mapShanmenDivineSenseProductIntent& Intent,
		const Fdemo_mapShanmenDivineSenseProductConfig& Config)
	{
		if (!Command.IsValid() || !Intent.IsValid() || !Config.IsValid())
		{
			return false;
		}
		const Fdemo_mapShanmenDivineSensePulseCommand& Pulse =
			Command.GetPulseCommand();
		return Pulse.IsValid()
			&& ActionsMatch(Pulse.GetAction(), Intent.GetAction())
			&& Pulse.GetDefinition().GetDefinitionId()
				== Config.GetDefinition().GetDefinitionId()
			&& Pulse.GetCost().GetCostId() == Config.GetCost().GetCostId()
			&& Pulse.GetScanOrdinal() == Intent.GetScanOrdinal()
			&& Pulse.GetSubjectActorBudget()
				== Intent.GetSubjectActorBudget();
	}

	Fdemo_mapShanmenDivineSenseProductControllerResult RejectSubmit(
		Edemo_mapShanmenDivineSenseProductControllerStatus Status,
		const TCHAR* Diagnostic,
		const FGuid& ControllerId,
		const FGuid& RunId,
		const Fdemo_mapShanmenDivineSenseProductIntent& Intent)
	{
		Fdemo_mapShanmenDivineSenseProductControllerResult Result;
		Result.Status = Status;
		Result.Diagnostic = Diagnostic
			? Diagnostic
			: TEXT("Divine Sense product Controller rejected submission.");
		Result.ControllerId = ControllerId;
		Result.RunId = RunId;
		Result.Intent = Intent;
		return Result;
	}

	Fdemo_mapShanmenDivineSenseProductControllerEndResult RejectEnd(
		Edemo_mapShanmenDivineSenseProductControllerEndStatus Status,
		const TCHAR* Diagnostic)
	{
		Fdemo_mapShanmenDivineSenseProductControllerEndResult Result;
		Result.Status = Status;
		Result.Diagnostic = Diagnostic
			? Diagnostic
			: TEXT("Divine Sense product Controller rejected teardown.");
		return Result;
	}

	Fdemo_mapShanmenSharedSpiritEnergyTransactionResult
	RejectSharedSpiritEnergyTransaction(
		Edemo_mapShanmenSharedSpiritEnergyTransactionError Error,
		const TCHAR* Diagnostic)
	{
		Fdemo_mapShanmenSharedSpiritEnergyTransactionResult Result;
		Result.Status =
			Edemo_mapShanmenSharedSpiritEnergyTransactionStatus::Rejected;
		Result.Error = Error;
		Result.Diagnostic = Diagnostic;
		return Result;
	}
}

bool Fdemo_mapShanmenDivineSenseProductConfig::TryCapture(
	const FShanmenDivineSenseDefinition& InDefinition,
	const FShanmenActionResourceCost& InCost,
	int32 InPulseCapacity,
	Fdemo_mapShanmenDivineSenseProductConfig& OutConfig)
{
	OutConfig = Fdemo_mapShanmenDivineSenseProductConfig();
	Fdemo_mapShanmenDivineSenseProductConfig Candidate;
	Candidate.Definition = InDefinition;
	Candidate.Cost = InCost;
	Candidate.PulseCapacity = InPulseCapacity;
	Candidate.ConfigId = MakeConfigId(
		InDefinition, InCost, InPulseCapacity);
	if (!Candidate.IsValid())
	{
		return false;
	}
	OutConfig = MoveTemp(Candidate);
	return true;
}

bool Fdemo_mapShanmenDivineSenseProductConfig::IsValid() const
{
	return ConfigId.IsValid() && Definition.IsValid() && Cost.IsValid()
		&& PulseCapacity > 0
		&& Definition.GetActionDefinitionId()
			== FShanmenDivineSenseDefinition::CanonicalActionDefinitionId()
		&& Cost.GetResourceChannel()
			== FShanmenCombatRuntimeNativeTags::ResourceSpiritEnergy()
		&& ConfigId == MakeConfigId(Definition, Cost, PulseCapacity);
}

bool Fdemo_mapShanmenDivineSenseProductConfig::Matches(
	const Fdemo_mapShanmenDivineSenseProductConfig& Other) const
{
	return IsValid() && Other.IsValid()
		&& ConfigId == Other.ConfigId
		&& Definition.GetDefinitionId()
			== Other.Definition.GetDefinitionId()
		&& Cost.GetCostId() == Other.Cost.GetCostId()
		&& PulseCapacity == Other.PulseCapacity;
}

bool Fdemo_mapShanmenDivineSenseProductIntent::TryCapture(
	const FGuid& RequestedIntentId,
	const FShanmenCombatActionSnapshot& InAction,
	int32 InScanOrdinal,
	int32 InSubjectActorBudget,
	Fdemo_mapShanmenDivineSenseProductIntent& OutIntent)
{
	OutIntent = Fdemo_mapShanmenDivineSenseProductIntent();
	Fdemo_mapShanmenDivineSenseProductIntent Candidate;
	Candidate.IntentId = RequestedIntentId;
	Candidate.Action = InAction;
	Candidate.ScanOrdinal = InScanOrdinal;
	Candidate.SubjectActorBudget = InSubjectActorBudget;
	if (!Candidate.IsValid())
	{
		return false;
	}
	OutIntent = MoveTemp(Candidate);
	return true;
}

bool Fdemo_mapShanmenDivineSenseProductIntent::IsValid() const
{
	return IntentId.IsValid() && Action.IsValid()
		&& Action.GetRunId().IsValid()
		&& Action.GetOwnerId() == Action.GetSourceEntityId()
		&& Action.GetActionDefinitionId()
			== FShanmenDivineSenseDefinition::CanonicalActionDefinitionId()
		&& ScanOrdinal >= 0 && SubjectActorBudget >= 0;
}

bool Fdemo_mapShanmenDivineSenseProductIntent::Matches(
	const Fdemo_mapShanmenDivineSenseProductIntent& Other) const
{
	return IsValid() && Other.IsValid()
		&& IntentId == Other.IntentId
		&& ActionsMatch(Action, Other.Action)
		&& ScanOrdinal == Other.ScanOrdinal
		&& SubjectActorBudget == Other.SubjectActorBudget;
}

bool Fdemo_mapShanmenDivineSenseProductAvailability::IsValid() const
{
	return AvailabilityId.IsValid() && ControllerId.IsValid()
		&& Config.IsValid() && SessionAvailability.IsValid()
		&& SessionAvailability.GetProcessedCommandCapacity()
			== Config.GetPulseCapacity()
		&& CapturedIntentCount >= 0
		&& CapturedIntentCount <= Config.GetPulseCapacity()
		&& SessionAvailability.GetProcessedCommandCount()
			<= CapturedIntentCount
		&& AvailabilityId == MakeAvailabilityId(
			ControllerId,
			Config.GetConfigId(),
			SessionAvailability.GetProjectionId(),
			CapturedIntentCount);
}

bool Fdemo_mapShanmenDivineSenseProductAvailability::Matches(
	const Fdemo_mapShanmenDivineSenseProductAvailability& Other) const
{
	return IsValid() && Other.IsValid()
		&& AvailabilityId == Other.AvailabilityId
		&& ControllerId == Other.ControllerId
		&& Config.Matches(Other.Config)
		&& SessionAvailability.Matches(Other.SessionAvailability)
		&& CapturedIntentCount == Other.CapturedIntentCount;
}

bool Fdemo_mapShanmenDivineSenseProductAvailability::
	CanCaptureNewIntent() const
{
	return IsValid()
		&& CapturedIntentCount < Config.GetPulseCapacity()
		&& SessionAvailability.CanAfford(Config.GetCost());
}

bool Fdemo_mapShanmenDivineSenseProductControllerResult::IsValid() const
{
	if (Status
			== Edemo_mapShanmenDivineSenseProductControllerStatus::Invalid
		|| Diagnostic.IsEmpty())
	{
		return false;
	}

	const bool bRouteOutcome =
		Status == Edemo_mapShanmenDivineSenseProductControllerStatus::Applied
		|| Status
			== Edemo_mapShanmenDivineSenseProductControllerStatus::
				AlreadyApplied
		|| Status
			== Edemo_mapShanmenDivineSenseProductControllerStatus::
				RouteRejected;
	if (!bRouteOutcome)
	{
		return true;
	}
	if (!ControllerId.IsValid() || !RunId.IsValid()
		|| !Intent.IsValid() || Intent.GetRunId() != RunId
		|| !Command.IsValid() || !AvailabilityBefore.IsValid()
		|| !AvailabilityAfter.IsValid() || !Route.IsValid()
		|| AvailabilityBefore.GetControllerId() != ControllerId
		|| AvailabilityAfter.GetControllerId() != ControllerId
		|| !AvailabilityBefore.GetConfig().Matches(
			AvailabilityAfter.GetConfig())
		|| !CommandMatches(
			Command, Intent, AvailabilityBefore.GetConfig())
		|| !AvailabilityBefore.GetSessionAvailability().Matches(
			Route.AvailabilityBefore)
		|| !AvailabilityAfter.GetSessionAvailability().Matches(
			Route.AvailabilityAfter))
	{
		return false;
	}

	if (Status
		== Edemo_mapShanmenDivineSenseProductControllerStatus::
			RouteRejected)
	{
		return !Route.IsAccepted();
	}
	if (!Route.IsAccepted())
	{
		return false;
	}
	if (Status
		== Edemo_mapShanmenDivineSenseProductControllerStatus::
			AlreadyApplied)
	{
		return bReusedIntent && Route.IsReplay()
			&& AvailabilityBefore.Matches(AvailabilityAfter);
	}
	return !Route.IsReplay();
}

bool Fdemo_mapShanmenDivineSenseProductControllerResult::IsAccepted() const
{
	return IsValid()
		&& (Status
				== Edemo_mapShanmenDivineSenseProductControllerStatus::
					Applied
			|| Status
				== Edemo_mapShanmenDivineSenseProductControllerStatus::
					AlreadyApplied);
}

bool Fdemo_mapShanmenDivineSenseProductControllerEndReceipt::IsValid() const
{
	return ReceiptId.IsValid() && ControllerId.IsValid()
		&& ConfigId.IsValid() && RunId.IsValid()
		&& SourceEntityId.IsValid() && SessionEndReceiptId.IsValid()
		&& CapturedIntentCount >= 0
		&& ReceiptId == MakeEndReceiptId(
			ControllerId,
			ConfigId,
			RunId,
			SourceEntityId,
			SessionEndReceiptId,
			CapturedIntentCount);
}

bool Fdemo_mapShanmenDivineSenseProductControllerEndResult::IsValid() const
{
	if (Status
			== Edemo_mapShanmenDivineSenseProductControllerEndStatus::Invalid
		|| Diagnostic.IsEmpty())
	{
		return false;
	}
	if (Status
			== Edemo_mapShanmenDivineSenseProductControllerEndStatus::Ended
		|| Status
			== Edemo_mapShanmenDivineSenseProductControllerEndStatus::
				AlreadyEnded)
	{
		return SessionEnd.IsSuccess() && Receipt.IsValid()
			&& (Status
					== Edemo_mapShanmenDivineSenseProductControllerEndStatus::
						AlreadyEnded)
				== SessionEnd.IsReplay();
	}
	if (Status
		== Edemo_mapShanmenDivineSenseProductControllerEndStatus::
			SessionRejected)
	{
		return SessionEnd.IsValid() && !SessionEnd.IsSuccess()
			&& !Receipt.IsValid();
	}
	return !SessionEnd.IsValid() && !Receipt.IsValid();
}

bool Fdemo_mapShanmenDivineSenseProductControllerEndResult::IsSuccess() const
{
	return IsValid()
		&& (Status
				== Edemo_mapShanmenDivineSenseProductControllerEndStatus::Ended
			|| Status
				== Edemo_mapShanmenDivineSenseProductControllerEndStatus::
					AlreadyEnded);
}

bool Fdemo_mapShanmenDivineSenseProductController::TryBegin(
	const Fdemo_mapCombatRunCoordinator& Coordinator,
	const FShanmenActionResourceSnapshot& OpeningSpiritEnergy,
	const Fdemo_mapShanmenDivineSenseProductConfig& InConfig,
	FString& OutDiagnostic)
{
	OutDiagnostic.Reset();
	if (!IsValid())
	{
		OutDiagnostic = TEXT("Divine Sense Controller begin requires valid local state.");
		return false;
	}
	if (IsActive())
	{
		if (Coordinator.IsReady()
			&& Coordinator.GetRunId() == RunId
			&& Coordinator.GetPlayerEntityId() == SourceEntityId
			&& Config.Matches(InConfig)
			&& Session.GetOpeningResourceSnapshot().GetSnapshotId()
				== OpeningSpiritEnergy.GetSnapshotId())
		{
			OutDiagnostic = TEXT("Exact Divine Sense Controller begin is already active.");
			return true;
		}
		OutDiagnostic = TEXT("Divine Sense Controller already owns another active binding.");
		return false;
	}
	if (IsEnded())
	{
		OutDiagnostic = TEXT("Ended Divine Sense Controller must be reset before reuse.");
		return false;
	}
	if (!Coordinator.IsReady())
	{
		OutDiagnostic = TEXT("Divine Sense Controller requires one ready Combat Run.");
		return false;
	}
	if (!OpeningSpiritEnergy.IsValid() || !InConfig.IsValid())
	{
		OutDiagnostic = TEXT("Divine Sense Controller requires valid opening resources and frozen config.");
		return false;
	}

	Fdemo_mapShanmenDivineSenseProductController Candidate;
	Candidate.RunId = Coordinator.GetRunId();
	Candidate.SourceEntityId = Coordinator.GetPlayerEntityId();
	Candidate.Config = InConfig;
	if (!Candidate.Session.TryBegin(
			Coordinator,
			OpeningSpiritEnergy,
			InConfig.GetPulseCapacity(),
			OutDiagnostic))
	{
		return false;
	}
	Candidate.ControllerId = MakeControllerId(
		Candidate.RunId,
		Candidate.SourceEntityId,
		Candidate.Config.GetConfigId(),
		Candidate.Session.GetSessionId(),
		Candidate.Session.GetOpeningResourceSnapshot().GetSnapshotId());
	Candidate.State =
		Edemo_mapShanmenDivineSenseProductControllerState::Active;
	if (!Candidate.IsValid()
		|| !Candidate.Session.IsConsistentWithCoordinator(Coordinator))
	{
		OutDiagnostic = TEXT("Divine Sense Controller failed closed after Session binding.");
		return false;
	}

	*this = MoveTemp(Candidate);
	OutDiagnostic = TEXT("Divine Sense Controller froze product config and opened one Run Session.");
	return true;
}

bool Fdemo_mapShanmenDivineSenseProductController::TryCaptureAvailability(
	const Fdemo_mapCombatRunCoordinator& Coordinator,
	Fdemo_mapShanmenDivineSenseProductAvailability& OutAvailability,
	FString& OutDiagnostic) const
{
	return TryBuildAvailability(
		Coordinator, OutAvailability, OutDiagnostic);
}

bool Fdemo_mapShanmenDivineSenseProductController::IsAvailabilityCurrent(
	const Fdemo_mapCombatRunCoordinator& Coordinator,
	const Fdemo_mapShanmenDivineSenseProductAvailability& Availability) const
{
	Fdemo_mapShanmenDivineSenseProductAvailability Current;
	FString Diagnostic;
	return Availability.IsValid()
		&& TryBuildAvailability(Coordinator, Current, Diagnostic)
		&& Current.Matches(Availability);
}

Fdemo_mapShanmenDivineSenseProductControllerResult
Fdemo_mapShanmenDivineSenseProductController::TrySubmit(
	const Fdemo_mapCombatRunCoordinator& Coordinator,
	UWorld* World,
	AActor* SourceActor,
	const Fdemo_mapShanmenDivineSenseProductIntent& Intent,
	const TArray<AActor*>& SubjectActors,
	const Idemo_mapShanmenDivineSenseWorldEvidenceProvider& EvidenceProvider)
{
	if (!IsActive())
	{
		return RejectSubmit(
			Edemo_mapShanmenDivineSenseProductControllerStatus::
				ControllerNotActive,
			TEXT("Divine Sense submission requires one active Controller."),
			ControllerId,
			RunId,
			Intent);
	}
	if (!IsValid())
	{
		return RejectSubmit(
			Edemo_mapShanmenDivineSenseProductControllerStatus::
				ControllerInvalid,
			TEXT("Divine Sense submission rejected invalid Controller state."),
			ControllerId,
			RunId,
			Intent);
	}
	if (!Coordinator.IsReady())
	{
		return RejectSubmit(
			Edemo_mapShanmenDivineSenseProductControllerStatus::
				CoordinatorNotReady,
			TEXT("Divine Sense submission requires one ready Combat Run."),
			ControllerId,
			RunId,
			Intent);
	}
	if (Coordinator.GetRunId() != RunId)
	{
		return RejectSubmit(
			Edemo_mapShanmenDivineSenseProductControllerStatus::RunMismatch,
			TEXT("Divine Sense Controller cannot submit across Runs."),
			ControllerId,
			RunId,
			Intent);
	}
	if (Coordinator.GetPlayerEntityId() != SourceEntityId)
	{
		return RejectSubmit(
			Edemo_mapShanmenDivineSenseProductControllerStatus::SourceMismatch,
			TEXT("Divine Sense Controller cannot submit across player identities."),
			ControllerId,
			RunId,
			Intent);
	}
	if (!Intent.IsValid())
	{
		return RejectSubmit(
			Edemo_mapShanmenDivineSenseProductControllerStatus::IntentInvalid,
			TEXT("Divine Sense requires one valid device-independent Intent."),
			ControllerId,
			RunId,
			Intent);
	}
	if (Intent.GetRunId() != RunId)
	{
		return RejectSubmit(
			Edemo_mapShanmenDivineSenseProductControllerStatus::RunMismatch,
			TEXT("Divine Sense Intent and Controller must name one Run."),
			ControllerId,
			RunId,
			Intent);
	}
	if (Intent.GetAction().GetOwnerId() != SourceEntityId
		|| Intent.GetAction().GetSourceEntityId() != SourceEntityId)
	{
		return RejectSubmit(
			Edemo_mapShanmenDivineSenseProductControllerStatus::SourceMismatch,
			TEXT("Divine Sense Intent must belong to the Controller player."),
			ControllerId,
			RunId,
			Intent);
	}

	Fdemo_mapShanmenDivineSenseProductAvailability Before;
	FString AvailabilityDiagnostic;
	if (!TryBuildAvailability(
			Coordinator, Before, AvailabilityDiagnostic))
	{
		return RejectSubmit(
			Edemo_mapShanmenDivineSenseProductControllerStatus::
				StateDesynchronized,
			TEXT("Divine Sense Controller could not capture pre-submit availability."),
			ControllerId,
			RunId,
			Intent);
	}

	if (const FCapturedIntent* Existing =
		CapturedIntents.Find(Intent.GetIntentId()))
	{
		if (!Existing->Intent.Matches(Intent))
		{
			Fdemo_mapShanmenDivineSenseProductControllerResult Conflict =
				RejectSubmit(
					Edemo_mapShanmenDivineSenseProductControllerStatus::
						IntentIdConflict,
					TEXT("Divine Sense IntentId was reused with another payload."),
					ControllerId,
					RunId,
					Intent);
			Conflict.AvailabilityBefore = Before;
			Conflict.AvailabilityAfter = Before;
			return Conflict;
		}

		Fdemo_mapShanmenDivineSenseProductController Candidate = *this;
		Fdemo_mapShanmenDivineSenseProductControllerResult Result =
			Candidate.RouteCaptured(
				Coordinator,
				World,
				SourceActor,
				SubjectActors,
				EvidenceProvider,
				Candidate.CapturedIntents.FindChecked(
					Intent.GetIntentId()),
				true,
				Before);
		if (!Result.IsValid() || !Candidate.IsValid()
			|| (Result.Status
				!= Edemo_mapShanmenDivineSenseProductControllerStatus::Applied
				&& Result.Status
					!= Edemo_mapShanmenDivineSenseProductControllerStatus::
						AlreadyApplied
				&& Result.Status
					!= Edemo_mapShanmenDivineSenseProductControllerStatus::
						RouteRejected))
		{
			return RejectSubmit(
				Edemo_mapShanmenDivineSenseProductControllerStatus::
					StateDesynchronized,
				TEXT("Divine Sense Controller could not publish a valid retry."),
				ControllerId,
				RunId,
				Intent);
		}
		*this = MoveTemp(Candidate);
		return Result;
	}

	if (CapturedIntents.Num() >= Config.GetPulseCapacity())
	{
		Fdemo_mapShanmenDivineSenseProductControllerResult Exhausted =
			RejectSubmit(
				Edemo_mapShanmenDivineSenseProductControllerStatus::
					IntentCapacityExceeded,
				TEXT("Divine Sense Controller has no capacity for a new Intent."),
				ControllerId,
				RunId,
				Intent);
		Exhausted.AvailabilityBefore = Before;
		Exhausted.AvailabilityAfter = Before;
		return Exhausted;
	}

	Fdemo_mapShanmenDivineSenseProductController Candidate = *this;
	FCapturedIntent Captured;
	Captured.Intent = Intent;
	FString CaptureDiagnostic;
	if (!Candidate.Session.TryCaptureCommand(
			Coordinator,
			Intent.GetAction(),
			Candidate.Config.GetDefinition(),
			Candidate.Config.GetCost(),
			Intent.GetScanOrdinal(),
			Intent.GetSubjectActorBudget(),
			SubjectActors,
			Captured.Command,
			CaptureDiagnostic))
	{
		Fdemo_mapShanmenDivineSenseProductControllerResult Rejected =
			RejectSubmit(
				Edemo_mapShanmenDivineSenseProductControllerStatus::
					CommandCaptureRejected,
				*CaptureDiagnostic,
				ControllerId,
				RunId,
				Intent);
		Rejected.AvailabilityBefore = Before;
		Rejected.AvailabilityAfter = Before;
		return Rejected;
	}
	Candidate.CapturedIntents.Add(
		Intent.GetIntentId(), MoveTemp(Captured));
	Fdemo_mapShanmenDivineSenseProductControllerResult Result =
		Candidate.RouteCaptured(
			Coordinator,
			World,
			SourceActor,
			SubjectActors,
			EvidenceProvider,
			Candidate.CapturedIntents.FindChecked(Intent.GetIntentId()),
			false,
			Before);
	if (!Result.IsValid() || !Candidate.IsValid()
		|| (Result.Status
			!= Edemo_mapShanmenDivineSenseProductControllerStatus::Applied
			&& Result.Status
				!= Edemo_mapShanmenDivineSenseProductControllerStatus::
					RouteRejected))
	{
		return RejectSubmit(
			Edemo_mapShanmenDivineSenseProductControllerStatus::
				StateDesynchronized,
			TEXT("Divine Sense Controller could not publish a valid first submission."),
			ControllerId,
			RunId,
			Intent);
	}
	*this = MoveTemp(Candidate);
	return Result;
}

Fdemo_mapShanmenDivineSenseProductControllerResult
Fdemo_mapShanmenDivineSenseProductController::RouteCaptured(
	const Fdemo_mapCombatRunCoordinator& Coordinator,
	UWorld* World,
	AActor* SourceActor,
	const TArray<AActor*>& SubjectActors,
	const Idemo_mapShanmenDivineSenseWorldEvidenceProvider& EvidenceProvider,
	FCapturedIntent& Captured,
	bool bReusedIntent,
	const Fdemo_mapShanmenDivineSenseProductAvailability& Before)
{
	Captured.LastRoute = Session.TryRoute(
		Coordinator,
		World,
		SourceActor,
		Captured.Command,
		SubjectActors,
		EvidenceProvider);

	Fdemo_mapShanmenDivineSenseProductControllerResult Result;
	Result.bReusedIntent = bReusedIntent;
	Result.ControllerId = ControllerId;
	Result.RunId = RunId;
	Result.Intent = Captured.Intent;
	Result.Command = Captured.Command;
	Result.AvailabilityBefore = Before;
	Result.Route = Captured.LastRoute;
	Result.Diagnostic = Captured.LastRoute.Diagnostic;
	if (!Captured.LastRoute.IsValid())
	{
		Result.Status =
			Edemo_mapShanmenDivineSenseProductControllerStatus::
				StateDesynchronized;
		Result.Diagnostic =
			TEXT("Divine Sense Session returned invalid Controller evidence.");
		return Result;
	}

	FString AvailabilityDiagnostic;
	if (!TryBuildAvailability(
			Coordinator, Result.AvailabilityAfter, AvailabilityDiagnostic))
	{
		Result.Status =
			Edemo_mapShanmenDivineSenseProductControllerStatus::
				StateDesynchronized;
		Result.Diagnostic =
			TEXT("Divine Sense Controller could not capture post-route availability.");
		return Result;
	}

	if (!Captured.LastRoute.IsAccepted())
	{
		Result.Status =
			Edemo_mapShanmenDivineSenseProductControllerStatus::RouteRejected;
		return Result;
	}
	Result.Status = Captured.LastRoute.IsReplay()
		? Edemo_mapShanmenDivineSenseProductControllerStatus::AlreadyApplied
		: Edemo_mapShanmenDivineSenseProductControllerStatus::Applied;
	return Result;
}

Fdemo_mapShanmenSharedSpiritEnergyTransactionResult
Fdemo_mapShanmenDivineSenseProductController::
	ApplySharedSpiritEnergyTransaction(
		const Fdemo_mapCombatRunCoordinator& Coordinator,
		const FGuid& TransactionId,
		const FGuid& CommandId,
		TFunctionRef<bool(FShanmenActionResourceAuthority&)>
			ApplyTransaction)
{
	if (!IsActive() || !IsValid()
		|| !Session.IsConsistentWithCoordinator(Coordinator))
	{
		return RejectSharedSpiritEnergyTransaction(
			Edemo_mapShanmenSharedSpiritEnergyTransactionError::HostNotReady,
			TEXT("Shared SpiritEnergy transaction requires the active Controller Run."));
	}

	Fdemo_mapShanmenDivineSenseProductController Candidate = *this;
	Fdemo_mapShanmenSharedSpiritEnergyTransactionResult Result =
		Candidate.Session.Host.ApplySharedSpiritEnergyTransaction(
			TransactionId, CommandId, ApplyTransaction);
	if (!Result.IsSuccess())
	{
		return Result;
	}
	if (!Candidate.Session.Router.TryRecordSharedSpiritEnergyTransaction(
			Candidate.Session.Host, Result.Receipt)
		|| !Candidate.Session.IsValid()
		|| !Candidate.IsValid())
	{
		return RejectSharedSpiritEnergyTransaction(
			Edemo_mapShanmenSharedSpiritEnergyTransactionError::
				StateDesynchronized,
			TEXT("Shared SpiritEnergy Host proof could not be published into the ordered Router ledger."));
	}

	*this = MoveTemp(Candidate);
	return Result;
}

Fdemo_mapShanmenDivineSenseProductControllerEndResult
Fdemo_mapShanmenDivineSenseProductController::TryEnd(
	const Fdemo_mapCombatRunCoordinator& Coordinator,
	const FGuid& ExpectedControllerId)
{
	if (!IsValid())
	{
		return RejectEnd(
			Edemo_mapShanmenDivineSenseProductControllerEndStatus::
				ControllerInvalid,
			TEXT("Divine Sense Controller teardown rejected invalid local state."));
	}
	if (IsEmpty())
	{
		return RejectEnd(
			Edemo_mapShanmenDivineSenseProductControllerEndStatus::
				ControllerNotActive,
			TEXT("Divine Sense Controller is already empty."));
	}
	if (!ExpectedControllerId.IsValid()
		|| ExpectedControllerId != ControllerId)
	{
		return RejectEnd(
			Edemo_mapShanmenDivineSenseProductControllerEndStatus::
				ControllerMismatch,
			TEXT("Divine Sense Controller teardown requires its exact ControllerId."));
	}

	if (IsEnded())
	{
		Fdemo_mapShanmenDivineSenseProductControllerEndResult Replay =
			RejectEnd(
				Edemo_mapShanmenDivineSenseProductControllerEndStatus::
					AlreadyEnded,
				TEXT("Exact Divine Sense Controller teardown replay returned retained proof."));
		Replay.SessionEnd = Session.TryEnd(
			Coordinator, Session.GetSessionId());
		Replay.Receipt = EndReceipt;
		if (!Replay.IsValid())
		{
			return RejectEnd(
				Edemo_mapShanmenDivineSenseProductControllerEndStatus::
					StateDesynchronized,
				TEXT("Divine Sense Controller retained invalid teardown replay proof."));
		}
		return Replay;
	}

	Fdemo_mapShanmenDivineSenseProductController Candidate = *this;
	Fdemo_mapShanmenDivineSenseSessionEndResult SessionEnd =
		Candidate.Session.TryEnd(
			Coordinator, Candidate.Session.GetSessionId());
	if (!SessionEnd.IsSuccess())
	{
		Fdemo_mapShanmenDivineSenseProductControllerEndResult Rejected =
			RejectEnd(
				Edemo_mapShanmenDivineSenseProductControllerEndStatus::
					SessionRejected,
				TEXT("Divine Sense Controller preserved state after Session teardown rejection."));
		Rejected.SessionEnd = MoveTemp(SessionEnd);
		return Rejected;
	}

	Fdemo_mapShanmenDivineSenseProductControllerEndReceipt Receipt;
	Receipt.ControllerId = Candidate.ControllerId;
	Receipt.ConfigId = Candidate.Config.GetConfigId();
	Receipt.RunId = Candidate.RunId;
	Receipt.SourceEntityId = Candidate.SourceEntityId;
	Receipt.SessionEndReceiptId = SessionEnd.Receipt.GetReceiptId();
	Receipt.CapturedIntentCount = Candidate.CapturedIntents.Num();
	Receipt.ReceiptId = MakeEndReceiptId(
		Receipt.ControllerId,
		Receipt.ConfigId,
		Receipt.RunId,
		Receipt.SourceEntityId,
		Receipt.SessionEndReceiptId,
		Receipt.CapturedIntentCount);
	if (!Receipt.IsValid())
	{
		return RejectEnd(
			Edemo_mapShanmenDivineSenseProductControllerEndStatus::
				StateDesynchronized,
			TEXT("Divine Sense Controller could not create terminal proof."));
	}

	Candidate.EndReceipt = Receipt;
	Candidate.State =
		Edemo_mapShanmenDivineSenseProductControllerState::Ended;
	if (!Candidate.IsValid())
	{
		return RejectEnd(
			Edemo_mapShanmenDivineSenseProductControllerEndStatus::
				StateDesynchronized,
			TEXT("Divine Sense Controller failed terminal-state validation."));
	}

	Fdemo_mapShanmenDivineSenseProductControllerEndResult Result =
		RejectEnd(
			Edemo_mapShanmenDivineSenseProductControllerEndStatus::Ended,
			TEXT("Divine Sense Controller ended with immutable config, Intent and Session proof."));
	Result.SessionEnd = MoveTemp(SessionEnd);
	Result.Receipt = Receipt;
	if (!Result.IsValid())
	{
		return RejectEnd(
			Edemo_mapShanmenDivineSenseProductControllerEndStatus::
				StateDesynchronized,
			TEXT("Divine Sense Controller produced invalid teardown evidence."));
	}
	*this = MoveTemp(Candidate);
	return Result;
}

bool Fdemo_mapShanmenDivineSenseProductController::Reset()
{
	if (!IsValid() || IsActive())
	{
		return false;
	}
	*this = Fdemo_mapShanmenDivineSenseProductController();
	return IsValid() && IsEmpty();
}

bool Fdemo_mapShanmenDivineSenseProductController::IsValid() const
{
	if (IsEmpty())
	{
		return !ControllerId.IsValid() && !RunId.IsValid()
			&& !SourceEntityId.IsValid() && !Config.IsValid()
			&& Session.IsValid() && Session.IsEmpty()
			&& CapturedIntents.IsEmpty() && !EndReceipt.IsValid();
	}
	if (!ControllerId.IsValid() || !RunId.IsValid()
		|| !SourceEntityId.IsValid() || !Config.IsValid()
		|| !Session.IsValid() || Session.IsEmpty()
		|| Session.GetRunId() != RunId
		|| Session.GetSourceEntityId() != SourceEntityId
		|| Session.GetRouter().GetProcessedCommandCapacity()
			!= Config.GetPulseCapacity()
		|| CapturedIntents.Num() > Config.GetPulseCapacity()
		|| Session.GetRouter().NumProcessedCommands()
			> CapturedIntents.Num()
		|| ControllerId != MakeControllerId(
			RunId,
			SourceEntityId,
			Config.GetConfigId(),
			Session.GetSessionId(),
			Session.GetOpeningResourceSnapshot().GetSnapshotId()))
	{
		return false;
	}

	for (const TPair<FGuid, FCapturedIntent>& Pair : CapturedIntents)
	{
		const FCapturedIntent& Captured = Pair.Value;
		if (!Pair.Key.IsValid()
			|| Pair.Key != Captured.Intent.GetIntentId()
			|| !Captured.Intent.IsValid()
			|| Captured.Intent.GetRunId() != RunId
			|| Captured.Intent.GetAction().GetOwnerId()
				!= SourceEntityId
			|| Captured.Intent.GetAction().GetSourceEntityId()
				!= SourceEntityId
			|| !CommandMatches(
				Captured.Command, Captured.Intent, Config)
			|| !Captured.LastRoute.IsValid()
			|| Captured.LastRoute.SessionId != Session.GetSessionId()
			|| !Captured.LastRoute.Route.IsValid()
			|| !Captured.LastRoute.Route.Command.Matches(Captured.Command))
		{
			return false;
		}
	}

	if (IsActive())
	{
		return Session.IsActive() && !EndReceipt.IsValid();
	}
	if (!IsEnded() || !Session.IsEnded() || !EndReceipt.IsValid())
	{
		return false;
	}
	return EndReceipt.GetControllerId() == ControllerId
		&& EndReceipt.GetConfigId() == Config.GetConfigId()
		&& EndReceipt.GetRunId() == RunId
		&& EndReceipt.GetSourceEntityId() == SourceEntityId
		&& EndReceipt.GetSessionEndReceiptId()
			== Session.GetEndReceipt().GetReceiptId()
		&& EndReceipt.GetCapturedIntentCount() == CapturedIntents.Num();
}

const Fdemo_mapShanmenDivineSenseRouteCommand*
Fdemo_mapShanmenDivineSenseProductController::FindCapturedCommand(
	const FGuid& IntentId) const
{
	const FCapturedIntent* Captured = CapturedIntents.Find(IntentId);
	return Captured && IsValid() ? &Captured->Command : nullptr;
}

bool Fdemo_mapShanmenDivineSenseProductController::TryBuildAvailability(
	const Fdemo_mapCombatRunCoordinator& Coordinator,
	Fdemo_mapShanmenDivineSenseProductAvailability& OutAvailability,
	FString& OutDiagnostic) const
{
	OutAvailability = Fdemo_mapShanmenDivineSenseProductAvailability();
	OutDiagnostic.Reset();
	if (!IsActive())
	{
		OutDiagnostic = TEXT("Divine Sense product availability requires an active Controller.");
		return false;
	}
	if (!IsValid())
	{
		OutDiagnostic = TEXT("Divine Sense product availability rejected invalid Controller state.");
		return false;
	}

	Fdemo_mapShanmenDivineSenseAvailabilityProjection SessionAvailability;
	if (!Session.TryCaptureAvailability(
			Coordinator, SessionAvailability, OutDiagnostic))
	{
		return false;
	}

	Fdemo_mapShanmenDivineSenseProductAvailability Candidate;
	Candidate.ControllerId = ControllerId;
	Candidate.Config = Config;
	Candidate.SessionAvailability = SessionAvailability;
	Candidate.CapturedIntentCount = CapturedIntents.Num();
	Candidate.AvailabilityId = MakeAvailabilityId(
		Candidate.ControllerId,
		Candidate.Config.GetConfigId(),
		Candidate.SessionAvailability.GetProjectionId(),
		Candidate.CapturedIntentCount);
	if (!Candidate.IsValid())
	{
		OutDiagnostic = TEXT("Divine Sense Controller could not build valid product availability.");
		return false;
	}
	OutAvailability = MoveTemp(Candidate);
	OutDiagnostic = TEXT("Captured current pointer-free Divine Sense product availability.");
	return true;
}
