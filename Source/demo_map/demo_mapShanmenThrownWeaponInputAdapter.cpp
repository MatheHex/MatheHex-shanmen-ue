#include "demo_mapShanmenThrownWeaponInputAdapter.h"

#include "ShanmenDeterministicId.h"
#include "ShanmenItemTags.h"
#include "demo_mapCombatRunCoordinator.h"
#include "demo_mapShanmenItemAuthoritySubsystem.h"
#include "demo_mapShanmenRunLifecycleAdapter.h"

#include "GameFramework/Actor.h"

namespace
{
	constexpr float ThrownWeaponOriginHeight = 50.0f;
	constexpr float ThrownWeaponMaximumDistance = 1400.0f;

	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	Fdemo_mapShanmenThrownWeaponInputResult MakeResult(
		const Edemo_mapShanmenThrownWeaponInputStatus Status,
		const int32 HotbarSlotNumber,
		const TCHAR* Diagnostic)
	{
		Fdemo_mapShanmenThrownWeaponInputResult Result;
		Result.Status = Status;
		Result.HotbarSlotNumber = HotbarSlotNumber;
		Result.Diagnostic = Diagnostic;
		return Result;
	}
}

FGuid Fdemo_mapShanmenThrownWeaponInputAdapter::MakeSelectionId(
	const FGuid& CorrelationId,
	const FGuid& RunId,
	const FGuid& ItemInstanceId,
	const int32 HotbarSlotNumber,
	const int32 AuthorityRevision,
	const uint64 SelectionOrdinal)
{
	if (!CorrelationId.IsValid() || !RunId.IsValid()
		|| !ItemInstanceId.IsValid()
		|| HotbarSlotNumber < 1
		|| HotbarSlotNumber
			> Fdemo_mapPersistentPreparationLayout::HotbarSlotCount
		|| AuthorityRevision < 0 || SelectionOrdinal == 0)
	{
		return FGuid();
	}
	return FShanmenDeterministicId::FromCanonicalParts(
		FName(TEXT("demo_map.ShanmenThrownWeapon.HotbarInputSelection.r1")),
		{
			GuidDigits(CorrelationId),
			GuidDigits(RunId),
			GuidDigits(ItemInstanceId),
			FString::FromInt(HotbarSlotNumber),
			FString::FromInt(AuthorityRevision),
			FString::Printf(TEXT("%llu"), SelectionOrdinal)
		});
}

Fdemo_mapShanmenThrownWeaponInputResult
Fdemo_mapShanmenThrownWeaponInputAdapter::RouteHotbarInput(
	Udemo_mapShanmenItemAuthoritySubsystem* Authority,
	Fdemo_mapShanmenThrownWeaponProductLifecycle& Lifecycle,
	Fdemo_mapCombatRunCoordinator& Coordinator,
	UWorld* World,
	TSubclassOf<Ademo_mapShanmenThrownWeaponProjectile> ProjectileClass,
	AActor* SourceActor,
	const int32 HotbarSlotNumber,
	TFunctionRef<FVector()> SampleAimDirection)
{
	if (HotbarSlotNumber < 1
		|| HotbarSlotNumber
			> Fdemo_mapPersistentPreparationLayout::HotbarSlotCount)
	{
		return MakeResult(
			Edemo_mapShanmenThrownWeaponInputStatus::InvalidSlot,
			HotbarSlotNumber,
			TEXT("Thrown-weapon input rejected an invalid hotbar slot."));
	}
	if (!Authority
		|| Authority->GetLifecycleState()
			!= Edemo_mapShanmenItemAuthorityLifecycleState::Ready)
	{
		return MakeResult(
			Edemo_mapShanmenThrownWeaponInputStatus::PassThrough,
			HotbarSlotNumber,
			TEXT("No ready ShanmenItems authority claims this hotbar input."));
	}

	Fdemo_mapShanmenRunCorrelation Correlation;
	FString CorrelationDiagnostic;
	if (!Fdemo_mapShanmenRunLifecycleAdapter::TryGetActiveRunCorrelation(
			*Authority, Correlation, &CorrelationDiagnostic))
	{
		return MakeResult(
			Edemo_mapShanmenThrownWeaponInputStatus::PassThrough,
			HotbarSlotNumber,
			TEXT("No durable active-Run hotbar claims this input."));
	}
	const int32 SlotIndex = HotbarSlotNumber - 1;
	const FGuid ItemId = Correlation.HotbarItemInstanceIds.IsValidIndex(SlotIndex)
		? Correlation.HotbarItemInstanceIds[SlotIndex]
		: FGuid();
	if (!ItemId.IsValid())
	{
		Fdemo_mapShanmenThrownWeaponInputResult Result = MakeResult(
			Edemo_mapShanmenThrownWeaponInputStatus::PassThrough,
			HotbarSlotNumber,
			TEXT("The durable active-Run hotbar slot is empty."));
		Result.RunId = Correlation.ActiveRunId;
		return Result;
	}

	FShanmenItemAuthoritySnapshot Snapshot;
	if (!Authority->TryCaptureSnapshot(Snapshot))
	{
		return MakeResult(
			Edemo_mapShanmenThrownWeaponInputStatus::SnapshotUnavailable,
			HotbarSlotNumber,
			TEXT("Typed hotbar routing could not capture ShanmenItems authority."));
	}
	if (!Snapshot.Content.IsValid()
		|| Snapshot.AuthorityRevision < Correlation.LifecycleAuthorityRevision)
	{
		return MakeResult(
			Edemo_mapShanmenThrownWeaponInputStatus::SnapshotStale,
			HotbarSlotNumber,
			TEXT("Typed hotbar routing rejected item evidence older than the active Run."));
	}
	const FShanmenItemInstance* Item = Snapshot.Items.FindByPredicate(
		[&ItemId](const FShanmenItemInstance& Candidate)
		{
			return Candidate.ItemInstanceId == ItemId;
		});
	if (!Item
		|| Item->OwnerId != Correlation.OwnerId
		|| Item->RunId != Correlation.ScopeId)
	{
		return MakeResult(
			Edemo_mapShanmenThrownWeaponInputStatus::ItemEvidenceRejected,
			HotbarSlotNumber,
			TEXT("The exact hotbar item is absent from this active Run scope."));
	}
	const FShanmenItemDefinition* Definition =
		Snapshot.Definitions.FindByPredicate(
			[Item](const FShanmenItemDefinition& Candidate)
			{
				return Candidate.DefinitionId == Item->DefinitionId;
			});
	if (!Definition || !Definition->IsValid())
	{
		return MakeResult(
			Edemo_mapShanmenThrownWeaponInputStatus::ItemEvidenceRejected,
			HotbarSlotNumber,
			TEXT("The exact hotbar item has no valid authority definition."));
	}
	if (!Definition->Supports(EShanmenItemResourceKind::Quantity)
		|| !Definition->ItemTags.HasTagExact(
			FShanmenItemNativeTags::ItemWeaponThrown()))
	{
		Fdemo_mapShanmenThrownWeaponInputResult Result = MakeResult(
			Edemo_mapShanmenThrownWeaponInputStatus::PassThrough,
			HotbarSlotNumber,
			TEXT("The exact hotbar item is not a typed thrown weapon."));
		Result.RunId = Correlation.ActiveRunId;
		Result.ItemInstanceId = ItemId;
		Result.AuthorityRevision = Snapshot.AuthorityRevision;
		return Result;
	}

	Fdemo_mapShanmenThrownWeaponInputResult Result;
	Result.HotbarSlotNumber = HotbarSlotNumber;
	Result.RunId = Correlation.ActiveRunId;
	Result.ItemInstanceId = ItemId;
	Result.AuthorityRevision = Snapshot.AuthorityRevision;
	if (!Lifecycle.IsActive()
		|| !Lifecycle.IsValid()
		|| Lifecycle.GetRunId() != Correlation.ActiveRunId
		|| !Coordinator.IsReady()
		|| Coordinator.GetRunId() != Correlation.ActiveRunId)
	{
		Result.Status =
			Edemo_mapShanmenThrownWeaponInputStatus::ProductRunMismatch;
		Result.Diagnostic =
			TEXT("Typed thrown weapon requires the matching active product and combat Run.");
		return Result;
	}
	FGuid SourceEntityId;
	if (!IsValid(SourceActor)
		|| !Coordinator.GetEntityRegistry().TryResolveObject(
			Coordinator.GetRunId(), SourceActor, INDEX_NONE, SourceEntityId)
		|| SourceEntityId != Coordinator.GetPlayerEntityId())
	{
		Result.Status =
			Edemo_mapShanmenThrownWeaponInputStatus::SourceUnavailable;
		Result.Diagnostic =
			TEXT("Typed thrown weapon input requires the canonical player source Actor.");
		return Result;
	}
	if (NextSelectionOrdinal == 0 || NextSelectionOrdinal == MAX_uint64)
	{
		Result.Status =
			Edemo_mapShanmenThrownWeaponInputStatus::SelectionSequenceExhausted;
		Result.Diagnostic =
			TEXT("Thrown-weapon input sequence exhausted without wrapping identity.");
		return Result;
	}

	Result.bAimSampled = true;
	const FVector AimDirection = SampleAimDirection();
	if (!FMath::IsFinite(AimDirection.X)
		|| !FMath::IsFinite(AimDirection.Y)
		|| !FMath::IsFinite(AimDirection.Z)
		|| AimDirection.IsNearlyZero())
	{
		Result.Status =
			Edemo_mapShanmenThrownWeaponInputStatus::AimUnavailable;
		Result.Diagnostic =
			TEXT("Typed thrown weapon input has no finite non-zero aim direction.");
		return Result;
	}
	Result.SelectionOrdinal = NextSelectionOrdinal;
	Result.SelectionId = MakeSelectionId(
		Correlation.CorrelationId,
		Correlation.ActiveRunId,
		ItemId,
		HotbarSlotNumber,
		Snapshot.AuthorityRevision,
		NextSelectionOrdinal);
	if (!Result.SelectionId.IsValid())
	{
		Result.Status =
			Edemo_mapShanmenThrownWeaponInputStatus::IntentCaptureRejected;
		Result.Diagnostic =
			TEXT("Thrown-weapon input could not derive one stable SelectionId.");
		return Result;
	}

	Fdemo_mapShanmenThrownWeaponHotbarIntent Intent;
	const FVector Origin = SourceActor->GetActorLocation()
		+ FVector(0.0, 0.0, ThrownWeaponOriginHeight);
	if (!Fdemo_mapShanmenThrownWeaponHotbarIntent::TryCapture(
			Result.SelectionId,
			HotbarSlotNumber,
			Origin,
			AimDirection,
			ThrownWeaponMaximumDistance,
			Intent))
	{
		Result.Status =
			Edemo_mapShanmenThrownWeaponInputStatus::IntentCaptureRejected;
		Result.Diagnostic =
			TEXT("Sampled thrown-weapon input could not enter the immutable intent contract.");
		return Result;
	}
	++NextSelectionOrdinal;
	Result.Session = Lifecycle.TrySubmitHotbar(
		World, ProjectileClass, Coordinator, Intent);
	Result.Status = Result.Session.IsAccepted()
		? Edemo_mapShanmenThrownWeaponInputStatus::Applied
		: Edemo_mapShanmenThrownWeaponInputStatus::ProductRejected;
	Result.Diagnostic = Result.Session.Diagnostic;
	return Result;
}
