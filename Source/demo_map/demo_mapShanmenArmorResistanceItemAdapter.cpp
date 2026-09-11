#include "demo_mapShanmenArmorResistanceItemAdapter.h"

#include "ShanmenCombatTags.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapShanmenItemAuthoritySubsystem.h"
#include "demo_mapShanmenRunLifecycleAdapter.h"

namespace
{
	template <typename TValue>
	const TValue* FindBy(
		const TArray<TValue>& Values,
		TFunctionRef<bool(const TValue&)> Predicate)
	{
		return Values.FindByPredicate(Predicate);
	}

	Fdemo_mapShanmenArmorResistanceItemResult Reject(
		Edemo_mapShanmenArmorResistanceItemStatus Status,
		const TCHAR* Diagnostic)
	{
		Fdemo_mapShanmenArmorResistanceItemResult Result;
		Result.Status = Status;
		Result.Diagnostic = Diagnostic;
		return Result;
	}

	bool IsExactArmorDefinitionBridge(
		const FShanmenItemDefinition& AuthorityDefinition,
		const Fdemo_mapItemDefinition& ProductDefinition)
	{
		return AuthorityDefinition.IsValid()
			&& AuthorityDefinition.DefinitionId
				== ProductDefinition.DefinitionId
			&& AuthorityDefinition.Supports(
				EShanmenItemResourceKind::DeploymentLock)
			&& AuthorityDefinition.MaxStack
				== ProductDefinition.MaxStackSize
			&& AuthorityDefinition.MaxDurability
				== ProductDefinition.MaxDurability
			&& AuthorityDefinition.MaxCharges
				== ProductDefinition.MaxCharges
			&& ProductDefinition.CategoryId
				== Fdemo_mapItemIds::ArmorCategory
			&& ProductDefinition.MaxStackSize == 1
			&& ProductDefinition.EquipmentSlotId
				== Fdemo_mapItemIds::ArmorSlot
			&& ProductDefinition.CompatibleSlotIds.Num() == 1
			&& ProductDefinition.CompatibleSlotIds[0]
				== Fdemo_mapItemIds::ArmorSlot
			&& Fdemo_mapItemDefinitions::IsCurrentContentIdentity(
				ProductDefinition.ContentVersionId,
				ProductDefinition.ContentDigest);
	}

	Fdemo_mapShanmenArmorResistanceItemResult MakeNoArmorResult(
		const FShanmenDefenseSnapshot& BaseDefense)
	{
		Fdemo_mapShanmenArmorResistanceItemResult Result;
		Result.Status =
			Edemo_mapShanmenArmorResistanceItemStatus::NoArmorEquipped;
		Result.Diagnostic =
			TEXT("The durable active Run has no prepared ArmorSlot item.");
		Result.Projection.Status =
			Edemo_mapShanmenArmorResistanceProjectionStatus::NotApplicable;
		Result.Projection.Defense = BaseDefense;
		Result.Projection.Diagnostic = Result.Diagnostic;
		return Result;
	}
}

bool Fdemo_mapShanmenArmorResistanceItemEvidence::IsValid() const
{
	const Fdemo_mapItemDefinition* ProductDefinition =
		Fdemo_mapItemDefinitions::Find(ArmorDefinitionId);
	const FShanmenContentStamp ExpectedAuthorityContent =
		Udemo_mapShanmenItemAuthoritySubsystem::ProductContentStamp();
	return CorrelationId.IsValid()
		&& ActiveRunId.IsValid()
		&& OwnerId.IsValid()
		&& ArmorItemInstanceId.IsValid()
		&& !ArmorDefinitionId.IsNone()
		&& DeploymentReservationId.IsValid()
		&& AuthorityRevision >= 0
		&& ItemRevision > DeploymentItemRevisionAtReserve
		&& DeploymentItemRevisionAtReserve >= 0
		&& AuthorityContent.Version == ExpectedAuthorityContent.Version
		&& AuthorityContent.Digest == ExpectedAuthorityContent.Digest
		&& Fdemo_mapItemDefinitions::IsCurrentContentIdentity(
			CatalogContentVersionId, CatalogContentDigest)
		&& ProductDefinition
		&& ProductDefinition->CategoryId == Fdemo_mapItemIds::ArmorCategory
		&& ProductDefinition->MaxStackSize == 1
		&& ProductDefinition->EquipmentSlotId == Fdemo_mapItemIds::ArmorSlot
		&& ProductDefinition->CompatibleSlotIds.Num() == 1
		&& ProductDefinition->CompatibleSlotIds[0]
			== Fdemo_mapItemIds::ArmorSlot;
}

bool Fdemo_mapShanmenArmorResistanceItemEvidence::operator==(
	const Fdemo_mapShanmenArmorResistanceItemEvidence& Other) const
{
	return CorrelationId == Other.CorrelationId
		&& ActiveRunId == Other.ActiveRunId
		&& OwnerId == Other.OwnerId
		&& ArmorItemInstanceId == Other.ArmorItemInstanceId
		&& ArmorDefinitionId == Other.ArmorDefinitionId
		&& DeploymentReservationId == Other.DeploymentReservationId
		&& AuthorityRevision == Other.AuthorityRevision
		&& ItemRevision == Other.ItemRevision
		&& DeploymentItemRevisionAtReserve
			== Other.DeploymentItemRevisionAtReserve
		&& AuthorityContent.Version == Other.AuthorityContent.Version
		&& AuthorityContent.Digest == Other.AuthorityContent.Digest
		&& CatalogContentVersionId == Other.CatalogContentVersionId
		&& CatalogContentDigest == Other.CatalogContentDigest;
}

bool Fdemo_mapShanmenArmorResistanceItemResult::IsSuccess() const
{
	if (Status == Edemo_mapShanmenArmorResistanceItemStatus::Projected)
	{
		return Evidence.IsValid() && Projection.HasProjection();
	}
	if (Status == Edemo_mapShanmenArmorResistanceItemStatus::NotApplicable)
	{
		return Evidence.IsValid()
			&& Projection.IsSuccess()
			&& Projection.Status
				== Edemo_mapShanmenArmorResistanceProjectionStatus::NotApplicable
			&& Projection.Defense.IsValid()
			&& Projection.ProjectedLayerIds.IsEmpty();
	}
	if (Status == Edemo_mapShanmenArmorResistanceItemStatus::NoArmorEquipped)
	{
		return !Evidence.IsValid()
			&& Projection.IsSuccess()
			&& Projection.Status
				== Edemo_mapShanmenArmorResistanceProjectionStatus::NotApplicable
			&& Projection.Defense.IsValid()
			&& Projection.ProjectedLayerIds.IsEmpty();
	}
	return false;
}

bool Fdemo_mapShanmenArmorResistanceItemResult::HasProjection() const
{
	return Status == Edemo_mapShanmenArmorResistanceItemStatus::Projected
		&& Evidence.IsValid()
		&& Projection.HasProjection();
}

Fdemo_mapShanmenArmorResistanceItemResult
Fdemo_mapShanmenArmorResistanceItemAdapter::ProjectActiveRun(
	const Udemo_mapShanmenItemAuthoritySubsystem& Authority,
	const FGuid& TargetEntityId,
	const FShanmenDefenseSnapshot& BaseDefense)
{
	if (!IsInGameThread()
		|| Authority.GetLifecycleState()
			!= Edemo_mapShanmenItemAuthorityLifecycleState::Ready)
	{
		return Reject(
			Edemo_mapShanmenArmorResistanceItemStatus::AuthorityNotReady,
			TEXT("Armor resistance projection requires the ready item authority on the Game Thread."));
	}

	Fdemo_mapShanmenRunCorrelation Correlation;
	FString Diagnostic;
	if (!Fdemo_mapShanmenRunLifecycleAdapter::TryGetActiveRunCorrelation(
			Authority, Correlation, &Diagnostic))
	{
		Fdemo_mapShanmenArmorResistanceItemResult Result = Reject(
			Edemo_mapShanmenArmorResistanceItemStatus::RunCorrelationInvalid,
			TEXT("Armor resistance projection could not reconstruct the durable active Run."));
		Result.Diagnostic = Diagnostic.IsEmpty()
			? Result.Diagnostic : Diagnostic;
		return Result;
	}

	FShanmenItemAuthoritySnapshot Snapshot;
	if (!Authority.TryCaptureSnapshot(Snapshot))
	{
		return Reject(
			Edemo_mapShanmenArmorResistanceItemStatus::SnapshotUnavailable,
			TEXT("The ready item authority could not provide a read-only snapshot."));
	}
	return ProjectFromEvidence(
		Snapshot, Correlation, TargetEntityId, BaseDefense);
}

Fdemo_mapShanmenArmorResistanceItemResult
Fdemo_mapShanmenArmorResistanceItemAdapter::ProjectFromEvidence(
	const FShanmenItemAuthoritySnapshot& Snapshot,
	const Fdemo_mapShanmenRunCorrelation& Correlation,
	const FGuid& TargetEntityId,
	const FShanmenDefenseSnapshot& BaseDefense)
{
	if (!TargetEntityId.IsValid()
		|| !BaseDefense.IsValid()
		|| !BaseDefense.TargetTags.HasTagExact(
			FShanmenCombatNativeTags::TargetLiving()))
	{
		return Reject(
			Edemo_mapShanmenArmorResistanceItemStatus::InputInvalid,
			TEXT("Armor resistance requires one exact living target and valid base defense."));
	}
	if (!Correlation.IsValid())
	{
		return Reject(
			Edemo_mapShanmenArmorResistanceItemStatus::RunCorrelationInvalid,
			TEXT("Armor resistance requires one valid immutable Run correlation."));
	}
	const FShanmenContentStamp ExpectedAuthorityContent =
		Udemo_mapShanmenItemAuthoritySubsystem::ProductContentStamp();
	if (Snapshot.Content.Version != ExpectedAuthorityContent.Version
		|| Snapshot.Content.Digest != ExpectedAuthorityContent.Digest
		|| Snapshot.AuthorityRevision
			< Correlation.LifecycleAuthorityRevision)
	{
		return Reject(
			Edemo_mapShanmenArmorResistanceItemStatus::SnapshotStale,
			TEXT("Item evidence predates the active-Run lifecycle or has the wrong product content identity."));
	}
	if (!Correlation.ArmorItemInstanceId.IsValid())
	{
		return MakeNoArmorResult(BaseDefense);
	}

	const FGuid ArmorItemId = Correlation.ArmorItemInstanceId;
	const FShanmenItemInstance* Item = FindBy<FShanmenItemInstance>(
		Snapshot.Items,
		[&ArmorItemId](const FShanmenItemInstance& Candidate)
		{
			return Candidate.ItemInstanceId == ArmorItemId;
		});
	if (!Item)
	{
		return Reject(
			Edemo_mapShanmenArmorResistanceItemStatus::ItemNotFound,
			TEXT("The prepared ArmorSlot identity is absent from the current authority snapshot."));
	}
	if (Item->State != EShanmenItemInstanceState::Deployed
		|| Item->OwnerId != Correlation.OwnerId
		|| Item->RunId != Correlation.ScopeId
		|| Item->Quantity != 1
		|| !Item->DeploymentReservationId.IsValid())
	{
		return Reject(
			Edemo_mapShanmenArmorResistanceItemStatus::ItemNotDeployed,
			TEXT("The exact ArmorSlot item is not a live deployed singleton in this Run scope."));
	}

	const FShanmenItemDefinition* AuthorityDefinition =
		FindBy<FShanmenItemDefinition>(
			Snapshot.Definitions,
			[Item](const FShanmenItemDefinition& Candidate)
			{
				return Candidate.DefinitionId == Item->DefinitionId;
			});
	const Fdemo_mapItemDefinition* ProductDefinition =
		Fdemo_mapItemDefinitions::Find(Item->DefinitionId);
	if (!AuthorityDefinition || !ProductDefinition)
	{
		return Reject(
			Edemo_mapShanmenArmorResistanceItemStatus::DefinitionUnavailable,
			TEXT("The exact ArmorSlot definition is absent from an authority or canonical catalog."));
	}
	if (!IsExactArmorDefinitionBridge(
			*AuthorityDefinition, *ProductDefinition))
	{
		return Reject(
			Edemo_mapShanmenArmorResistanceItemStatus::DefinitionMismatch,
			TEXT("The active-Run item and canonical catalog do not agree on one deployable ArmorSlot definition."));
	}

	const FShanmenItemReservationSnapshot* Deployment =
		FindBy<FShanmenItemReservationSnapshot>(
			Snapshot.Reservations,
			[Item](const FShanmenItemReservationSnapshot& Candidate)
			{
				return Candidate.ReservationId
					== Item->DeploymentReservationId;
			});
	if (!Deployment || !Deployment->IsValid()
		|| Deployment->State
			!= EShanmenItemReservationState::Committed
		|| Deployment->ResourceKind
			!= EShanmenItemResourceKind::DeploymentLock
		|| Deployment->ItemInstanceId != Item->ItemInstanceId
		|| Deployment->RunId != Correlation.ScopeId
		|| Deployment->OwnerId != Correlation.OwnerId
		|| Deployment->Amount != 1
		|| Item->Revision <= Deployment->ItemRevisionAtReserve)
	{
		return Reject(
			Edemo_mapShanmenArmorResistanceItemStatus::DeploymentEvidenceInvalid,
			TEXT("The ArmorSlot item has no exact committed DeploymentLock evidence."));
	}

	Fdemo_mapShanmenArmorResistanceItemEvidence Evidence;
	Evidence.CorrelationId = Correlation.CorrelationId;
	Evidence.ActiveRunId = Correlation.ActiveRunId;
	Evidence.OwnerId = Correlation.OwnerId;
	Evidence.ArmorItemInstanceId = Item->ItemInstanceId;
	Evidence.ArmorDefinitionId = Item->DefinitionId;
	Evidence.DeploymentReservationId = Item->DeploymentReservationId;
	Evidence.AuthorityRevision = Snapshot.AuthorityRevision;
	Evidence.ItemRevision = Item->Revision;
	Evidence.DeploymentItemRevisionAtReserve =
		Deployment->ItemRevisionAtReserve;
	Evidence.AuthorityContent = Snapshot.Content;
	Evidence.CatalogContentVersionId =
		Fdemo_mapItemDefinitions::GetContentVersionId();
	Evidence.CatalogContentDigest =
		Fdemo_mapItemDefinitions::GetContentDigest();
	if (!Evidence.IsValid())
	{
		return Reject(
			Edemo_mapShanmenArmorResistanceItemStatus::DefinitionMismatch,
			TEXT("Validated ArmorSlot inputs did not produce durable current-content evidence."));
	}

	Fdemo_mapShanmenArmorResistanceProjectionResult Projection =
		Fdemo_mapShanmenArmorResistanceProjection::TryProject(
			*ProductDefinition,
			Item->ItemInstanceId,
			TargetEntityId,
			BaseDefense);
	if (!Projection.IsSuccess())
	{
		Fdemo_mapShanmenArmorResistanceItemResult Result = Reject(
			Edemo_mapShanmenArmorResistanceItemStatus::ProjectionRejected,
			TEXT("Canonical ArmorSlot evidence was rejected by the pure resistance projection."));
		Result.Diagnostic = Projection.Diagnostic.IsEmpty()
			? Result.Diagnostic : Projection.Diagnostic;
		return Result;
	}

	Fdemo_mapShanmenArmorResistanceItemResult Result;
	Result.Status = Projection.HasProjection()
		? Edemo_mapShanmenArmorResistanceItemStatus::Projected
		: Edemo_mapShanmenArmorResistanceItemStatus::NotApplicable;
	Result.Diagnostic = Projection.Diagnostic;
	Result.Evidence = MoveTemp(Evidence);
	Result.Projection = MoveTemp(Projection);
	if (!Result.IsSuccess())
	{
		return Reject(
			Edemo_mapShanmenArmorResistanceItemStatus::ProjectionRejected,
			TEXT("Armor resistance result failed final cross-boundary invariants."));
	}
	return Result;
}
