#include "demo_mapShanmenMeridianShockTreatmentInputAdapter.h"

#include "ShanmenDeterministicId.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapShanmenItemAuthoritySubsystem.h"
#include "demo_mapShanmenRunLifecycleAdapter.h"

namespace
{
	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	Fdemo_mapShanmenMeridianShockTreatmentInputResult MakeResult(
		const Edemo_mapShanmenMeridianShockTreatmentInputStatus Status,
		const int32 HotbarSlotNumber,
		const TCHAR* Diagnostic)
	{
		Fdemo_mapShanmenMeridianShockTreatmentInputResult Result;
		Result.Status = Status;
		Result.HotbarSlotNumber = HotbarSlotNumber;
		Result.Diagnostic = Diagnostic;
		return Result;
	}
}

FGuid Fdemo_mapShanmenMeridianShockTreatmentInputAdapter::MakeRequestId(
	const FGuid& CorrelationId,
	const FGuid& RunId,
	const FGuid& ItemInstanceId,
	const int32 HotbarSlotNumber,
	const int32 AuthorityRevision,
	const FGuid& TimelineSampleId,
	const uint64 RequestOrdinal)
{
	if (!CorrelationId.IsValid() || !RunId.IsValid()
		|| !ItemInstanceId.IsValid() || !TimelineSampleId.IsValid()
		|| HotbarSlotNumber < 1
		|| HotbarSlotNumber
			> Fdemo_mapPersistentPreparationLayout::HotbarSlotCount
		|| AuthorityRevision < 0 || RequestOrdinal == 0)
	{
		return FGuid();
	}
	return FShanmenDeterministicId::FromCanonicalParts(
		FName(TEXT("demo_map.MeridianShockTreatment.HotbarRequest.r1")),
		{
			GuidDigits(CorrelationId),
			GuidDigits(RunId),
			GuidDigits(ItemInstanceId),
			FString::FromInt(HotbarSlotNumber),
			FString::FromInt(AuthorityRevision),
			GuidDigits(TimelineSampleId),
			FString::Printf(TEXT("%llu"), RequestOrdinal)
		});
}

Fdemo_mapShanmenMeridianShockTreatmentInputResult
Fdemo_mapShanmenMeridianShockTreatmentInputAdapter::RouteHotbarInput(
	Udemo_mapShanmenItemAuthoritySubsystem* Authority,
	Fdemo_mapShanmenMeridianShockTreatmentProductLifecycle& Lifecycle,
	const Fdemo_mapShanmenCombatRunFixedTimeline& Timeline,
	const int32 HotbarSlotNumber)
{
	if (HotbarSlotNumber < 1
		|| HotbarSlotNumber
			> Fdemo_mapPersistentPreparationLayout::HotbarSlotCount)
	{
		return MakeResult(
			Edemo_mapShanmenMeridianShockTreatmentInputStatus::InvalidSlot,
			HotbarSlotNumber,
			TEXT("Treatment input rejected an invalid hotbar slot."));
	}
	if (!Authority
		|| Authority->GetLifecycleState()
			!= Edemo_mapShanmenItemAuthorityLifecycleState::Ready)
	{
		return MakeResult(
			Edemo_mapShanmenMeridianShockTreatmentInputStatus::PassThrough,
			HotbarSlotNumber,
			TEXT("No ready ShanmenItems authority claims this treatment input."));
	}

	Fdemo_mapShanmenRunCorrelation Correlation;
	FString CorrelationDiagnostic;
	if (!Fdemo_mapShanmenRunLifecycleAdapter::TryGetActiveRunCorrelation(
			*Authority, Correlation, &CorrelationDiagnostic))
	{
		return MakeResult(
			Edemo_mapShanmenMeridianShockTreatmentInputStatus::PassThrough,
			HotbarSlotNumber,
			TEXT("No durable active-Run hotbar claims this treatment input."));
	}
	const int32 SlotIndex = HotbarSlotNumber - 1;
	const FGuid ItemId =
		Correlation.HotbarItemInstanceIds.IsValidIndex(SlotIndex)
			? Correlation.HotbarItemInstanceIds[SlotIndex]
			: FGuid();
	if (!ItemId.IsValid())
	{
		Fdemo_mapShanmenMeridianShockTreatmentInputResult Result = MakeResult(
			Edemo_mapShanmenMeridianShockTreatmentInputStatus::PassThrough,
			HotbarSlotNumber,
			TEXT("The durable active-Run hotbar slot is empty."));
		Result.RunId = Correlation.ActiveRunId;
		return Result;
	}

	FShanmenItemAuthoritySnapshot Snapshot;
	if (!Authority->TryCaptureSnapshot(Snapshot))
	{
		return MakeResult(
			Edemo_mapShanmenMeridianShockTreatmentInputStatus::SnapshotUnavailable,
			HotbarSlotNumber,
			TEXT("Typed treatment routing could not capture ShanmenItems authority."));
	}
	const FShanmenContentStamp ProductContent =
		Udemo_mapShanmenItemAuthoritySubsystem::ProductContentStamp();
	if (!Snapshot.Content.IsValid()
		|| Snapshot.Content.Version != ProductContent.Version
		|| Snapshot.Content.Digest != ProductContent.Digest
		|| Snapshot.AuthorityRevision
			< Correlation.LifecycleAuthorityRevision)
	{
		return MakeResult(
			Edemo_mapShanmenMeridianShockTreatmentInputStatus::SnapshotStale,
			HotbarSlotNumber,
			TEXT("Typed treatment routing rejected stale active-Run item evidence."));
	}
	const FShanmenItemInstance* Item = Snapshot.Items.FindByPredicate(
		[&ItemId](const FShanmenItemInstance& Candidate)
		{
			return Candidate.ItemInstanceId == ItemId;
		});
	if (!Item
		|| Item->OwnerId != Correlation.OwnerId
		|| Item->RunId != Correlation.ScopeId
		|| !Correlation.OrderedRunInventoryItemInstanceIds.Contains(ItemId))
	{
		return MakeResult(
			Edemo_mapShanmenMeridianShockTreatmentInputStatus::ItemEvidenceRejected,
			HotbarSlotNumber,
			TEXT("The exact hotbar item is absent from this active Run scope."));
	}
	const FShanmenItemDefinition* AuthorityDefinition =
		Snapshot.Definitions.FindByPredicate(
			[Item](const FShanmenItemDefinition& Candidate)
			{
				return Candidate.DefinitionId == Item->DefinitionId;
			});
	const Fdemo_mapItemDefinition* ProductDefinition =
		Fdemo_mapItemDefinitions::Find(Item->DefinitionId);
	if (!AuthorityDefinition || !AuthorityDefinition->IsValid()
		|| !ProductDefinition
		|| !Fdemo_mapItemDefinitions::IsCurrentContentIdentity(
			ProductDefinition->ContentVersionId,
			ProductDefinition->ContentDigest))
	{
		return MakeResult(
			Edemo_mapShanmenMeridianShockTreatmentInputStatus::ItemEvidenceRejected,
			HotbarSlotNumber,
			TEXT("The exact hotbar item has no current product definition."));
	}
	if (!ProductDefinition->HasGameplaySemantic(
			Edemo_mapItemGameplaySemantic::MeridianShockTreatment))
	{
		Fdemo_mapShanmenMeridianShockTreatmentInputResult Result = MakeResult(
			Edemo_mapShanmenMeridianShockTreatmentInputStatus::PassThrough,
			HotbarSlotNumber,
			TEXT("The exact hotbar item is not a Meridian Shock treatment."));
		Result.RunId = Correlation.ActiveRunId;
		Result.ItemInstanceId = ItemId;
		Result.AuthorityRevision = Snapshot.AuthorityRevision;
		return Result;
	}

	Fdemo_mapShanmenMeridianShockTreatmentInputResult Result;
	Result.HotbarSlotNumber = HotbarSlotNumber;
	Result.RunId = Correlation.ActiveRunId;
	Result.ItemInstanceId = ItemId;
	Result.AuthorityRevision = Snapshot.AuthorityRevision;
	if (!AuthorityDefinition->Supports(EShanmenItemResourceKind::Quantity))
	{
		Result.Status =
			Edemo_mapShanmenMeridianShockTreatmentInputStatus::ItemEvidenceRejected;
		Result.Diagnostic =
			TEXT("Canonical treatment item lacks Quantity authority support.");
		return Result;
	}
	if (!Lifecycle.IsActive() || !Lifecycle.IsValid()
		|| Lifecycle.GetRunId() != Correlation.ActiveRunId
		|| !Timeline.IsActiveForRun(Correlation.ActiveRunId))
	{
		Result.Status =
			Edemo_mapShanmenMeridianShockTreatmentInputStatus::ProductRunMismatch;
		Result.Diagnostic =
			TEXT("Typed treatment requires matching product and timeline Runs.");
		return Result;
	}
	if (NextRequestOrdinal == 0 || NextRequestOrdinal == MAX_uint64)
	{
		Result.Status =
			Edemo_mapShanmenMeridianShockTreatmentInputStatus::RequestSequenceExhausted;
		Result.Diagnostic =
			TEXT("Treatment input sequence exhausted without wrapping identity.");
		return Result;
	}

	Fdemo_mapShanmenCombatRunTimelineSample TimelineSample;
	Result.bTimelineSampled = true;
	if (!Timeline.TryCapture(TimelineSample))
	{
		Result.Status =
			Edemo_mapShanmenMeridianShockTreatmentInputStatus::TimelineUnavailable;
		Result.Diagnostic =
			TEXT("Treatment input could not capture the active Run timeline.");
		return Result;
	}
	Result.RequestOrdinal = NextRequestOrdinal;
	Result.RequestId = MakeRequestId(
		Correlation.CorrelationId,
		Correlation.ActiveRunId,
		ItemId,
		HotbarSlotNumber,
		Snapshot.AuthorityRevision,
		TimelineSample.GetSampleId(),
		NextRequestOrdinal);
	if (!Result.RequestId.IsValid())
	{
		Result.Status =
			Edemo_mapShanmenMeridianShockTreatmentInputStatus::RequestCaptureRejected;
		Result.Diagnostic =
			TEXT("Treatment input could not derive one stable RequestId.");
		return Result;
	}

	++NextRequestOrdinal;
	Result.Route = Lifecycle.TrySubmitHotbar(
		Result.RequestId, ItemId, TimelineSample);
	Result.Status = Result.Route.IsCommitted()
		? Edemo_mapShanmenMeridianShockTreatmentInputStatus::Applied
		: Result.Route.RequiresRecovery()
			? Edemo_mapShanmenMeridianShockTreatmentInputStatus::RecoveryRequired
			: Edemo_mapShanmenMeridianShockTreatmentInputStatus::ProductRejected;
	Result.Diagnostic = Result.Route.Diagnostic;
	return Result;
}
