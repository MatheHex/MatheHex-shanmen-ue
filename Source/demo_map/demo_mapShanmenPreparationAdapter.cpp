#include "demo_mapShanmenPreparationAdapter.h"

#include "ShanmenDeterministicId.h"
#include "ShanmenItemRepository.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapShanmenItemAuthoritySubsystem.h"

namespace
{
	struct FPreparationSlotSpec
	{
		FName SlotId;
		FName ContainerType;
		FName PurposeId;
	};

	const TArray<FPreparationSlotSpec>& SlotSpecs()
	{
		static const TArray<FPreparationSlotSpec> Specs =
		{
			{ Fdemo_mapItemIds::WeaponSlot, TEXT("Weapon"), TEXT("Shanmen.Preparation.Weapon") },
			{ Fdemo_mapItemIds::ArmorSlot, TEXT("Armor"), TEXT("Shanmen.Preparation.Armor") },
			{ Fdemo_mapItemIds::AccessorySlot, TEXT("Accessory0"), TEXT("Shanmen.Preparation.Accessory") },
			{ Fdemo_mapItemIds::SpatialRingSlot, TEXT("SpatialRing"), TEXT("Shanmen.Preparation.SpatialRing") },
			{ Fdemo_mapItemIds::BackpackSlot, TEXT("Backpack"), TEXT("Shanmen.Preparation.Backpack") }
		};
		return Specs;
	}

	const FPreparationSlotSpec* FindSlotSpec(FName SlotId)
	{
		return SlotSpecs().FindByPredicate([SlotId](const FPreparationSlotSpec& Spec)
		{
			return Spec.SlotId == SlotId;
		});
	}

	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	bool GuidLess(const FGuid& Left, const FGuid& Right)
	{
		return GuidDigits(Left) < GuidDigits(Right);
	}

	const FShanmenItemDefinition* FindDefinition(
		const FShanmenItemAuthoritySnapshot& Snapshot,
		FName DefinitionId)
	{
		return Snapshot.Definitions.FindByPredicate([DefinitionId](
			const FShanmenItemDefinition& Definition)
		{
			return Definition.DefinitionId == DefinitionId;
		});
	}

	const FShanmenItemInstance* FindItem(
		const FShanmenItemAuthoritySnapshot& Snapshot,
		const FGuid& ItemId)
	{
		return Snapshot.Items.FindByPredicate([&ItemId](
			const FShanmenItemInstance& Item)
		{
			return Item.ItemInstanceId == ItemId;
		});
	}

	int32 ReserveRevision(
		const FShanmenItemAuthoritySnapshot& Snapshot,
		const FShanmenItemReservationSnapshot& Reservation)
	{
		const FShanmenItemProcessedRequestSnapshot* Processed =
			Snapshot.ProcessedRequests.FindByPredicate([&Reservation](
				const FShanmenItemProcessedRequestSnapshot& Candidate)
			{
				return Candidate.RequestId == Reservation.ReserveRequestId;
			});
		return Processed && Processed->Receipt.IsSuccess()
			&& Processed->Receipt.Operation == EShanmenItemTransactionOperation::Reserve
			? Processed->Receipt.AuthorityRevision
			: INDEX_NONE;
	}

	struct FSlotAnalysis
	{
		const FPreparationSlotSpec* Spec = nullptr;
		FGuid BaselineItemId;
		bool bHasHistory = false;
		const FShanmenItemReservationSnapshot* Latest = nullptr;
		int32 LatestReserveRevision = INDEX_NONE;
		FGuid SelectedItemId;
		TArray<const FShanmenItemReservationSnapshot*> ActiveReservations;
		TArray<const FShanmenItemReservationSnapshot*> CommittedReservations;
	};

	bool AnalyzeSlot(
		const FShanmenItemAuthoritySnapshot& Snapshot,
		const FPreparationSlotSpec& Spec,
		FSlotAnalysis& Out,
		FString& OutDiagnostic)
	{
		Out = FSlotAnalysis();
		Out.Spec = &Spec;
		const FShanmenItemContainer* BaselineContainer = nullptr;
		for (const FShanmenItemContainer& Container : Snapshot.Containers)
		{
			if (Container.ContainerType != Spec.ContainerType)
			{
				continue;
			}
			if (BaselineContainer || Container.Slots.Num() != 1)
			{
				OutDiagnostic = FString::Printf(
					TEXT("Preparation slot %s requires exactly one one-cell authority container of type %s."),
					*Spec.SlotId.ToString(), *Spec.ContainerType.ToString());
				return false;
			}
			BaselineContainer = &Container;
		}
		if (!BaselineContainer)
		{
			OutDiagnostic = FString::Printf(
				TEXT("Preparation authority is missing container type %s."),
				*Spec.ContainerType.ToString());
			return false;
		}
		Out.BaselineItemId = BaselineContainer->Slots[0];

		for (const FShanmenItemReservationSnapshot& Reservation : Snapshot.Reservations)
		{
			if (Reservation.ResourceKind != EShanmenItemResourceKind::DeploymentLock
				|| Reservation.PurposeId != Spec.PurposeId)
			{
				continue;
			}
			Out.bHasHistory = true;
			const int32 Revision = ReserveRevision(Snapshot, Reservation);
			if (Revision == INDEX_NONE)
			{
				OutDiagnostic = TEXT("Preparation reservation has no matching successful Reserve receipt.");
				return false;
			}
			if (!Out.Latest || Revision > Out.LatestReserveRevision
				|| (Revision == Out.LatestReserveRevision
					&& GuidLess(Out.Latest->ReservationId, Reservation.ReservationId)))
			{
				Out.Latest = &Reservation;
				Out.LatestReserveRevision = Revision;
			}
			if (Reservation.State == EShanmenItemReservationState::Reserved)
			{
				Out.ActiveReservations.Add(&Reservation);
			}
			else if (Reservation.State == EShanmenItemReservationState::Committed)
			{
				Out.CommittedReservations.Add(&Reservation);
			}
		}

		if (!Out.bHasHistory)
		{
			Out.SelectedItemId = Out.BaselineItemId;
		}
		else if (Out.Latest
			&& (Out.Latest->State == EShanmenItemReservationState::Reserved
				|| Out.Latest->State == EShanmenItemReservationState::Committed))
		{
			Out.SelectedItemId = Out.Latest->ItemInstanceId;
		}
		if (Out.CommittedReservations.Num() > 1
			|| (!Out.CommittedReservations.IsEmpty()
				&& (!Out.Latest
					|| Out.Latest->State
						!= EShanmenItemReservationState::Committed)))
		{
			OutDiagnostic = FString::Printf(
				TEXT("Preparation purpose %s has an ambiguous committed deployment history."),
				*Spec.PurposeId.ToString());
			return false;
		}
		return true;
	}

	bool AnalyzeAllSlots(
		const FShanmenItemAuthoritySnapshot& Snapshot,
		TArray<FSlotAnalysis>& OutSlots,
		FString& OutDiagnostic)
	{
		OutSlots.Reset();
		for (const FPreparationSlotSpec& Spec : SlotSpecs())
		{
			FSlotAnalysis& Slot = OutSlots.AddDefaulted_GetRef();
			if (!AnalyzeSlot(Snapshot, Spec, Slot, OutDiagnostic))
			{
				OutSlots.Reset();
				return false;
			}
		}
		return true;
	}

	FGuid* SelectedField(
		Fdemo_mapShanmenPreparationAuthorityProjection& Projection,
		FName SlotId)
	{
		if (SlotId == Fdemo_mapItemIds::WeaponSlot) return &Projection.SelectedWeaponId;
		if (SlotId == Fdemo_mapItemIds::ArmorSlot) return &Projection.SelectedArmorId;
		if (SlotId == Fdemo_mapItemIds::AccessorySlot) return &Projection.SelectedAccessoryId;
		if (SlotId == Fdemo_mapItemIds::SpatialRingSlot) return &Projection.SelectedSpatialRingId;
		if (SlotId == Fdemo_mapItemIds::BackpackSlot) return &Projection.SelectedBackpackId;
		return nullptr;
	}

	bool IsSelected(
		const Fdemo_mapShanmenPreparationAuthorityProjection& Projection,
		const FGuid& ItemId)
	{
		return Projection.SelectedWeaponId == ItemId
			|| Projection.SelectedArmorId == ItemId
			|| Projection.SelectedAccessoryId == ItemId
			|| Projection.SelectedSpatialRingId == ItemId
			|| Projection.SelectedBackpackId == ItemId;
	}

	Fdemo_mapShanmenPreparationAdapterResult MakeResult(
		Edemo_mapShanmenPreparationAdapterStatus Status,
		const FString& Diagnostic,
		Udemo_mapShanmenItemAuthoritySubsystem* Authority = nullptr)
	{
		Fdemo_mapShanmenPreparationAdapterResult Result;
		Result.Status = Status;
		Result.Diagnostic = Diagnostic;
		if (Authority)
		{
			FShanmenItemAuthoritySnapshot Snapshot;
			FString ProjectionDiagnostic;
			if (Authority->TryCaptureSnapshot(Snapshot))
			{
				Fdemo_mapShanmenPreparationAdapter::BuildProjection(
					Snapshot, Authority->GetBoundOwnerId(), Result.Projection,
					&ProjectionDiagnostic);
			}
		}
		return Result;
	}

	FShanmenOperationContext MakeContext(
		const FShanmenItemAuthoritySnapshot& Snapshot,
		const FGuid& ScopeId,
		const FGuid& OwnerId,
		const FGuid& RequestId)
	{
		FShanmenOperationContext Context;
		Context.RunId = ScopeId;
		Context.OwnerId = OwnerId;
		Context.RequestId = RequestId;
		Context.Content = Snapshot.Content;
		return Context;
	}

	FGuid MakeReserveRequestId(
		const FSlotAnalysis& Slot,
		const FGuid& OwnerId,
		const FGuid& ScopeId,
		const FGuid& ItemId,
		int32 ItemRevision)
	{
		const FString Previous = Slot.Latest
			? GuidDigits(Slot.Latest->ReservationId)
			: FString::Printf(TEXT("baseline:%s"), *GuidDigits(Slot.BaselineItemId));
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Preparation.EquipmentReserve.r1"),
			{
				GuidDigits(OwnerId), GuidDigits(ScopeId),
				Slot.Spec->PurposeId.ToString(), Previous,
				GuidDigits(ItemId), FString::FromInt(ItemRevision)
			});
	}

	FGuid MakeCancelRequestId(const FGuid& ReservationId)
	{
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Preparation.EquipmentCancel.r1"),
			{ GuidDigits(ReservationId) });
	}

	bool CancelReservation(
		Udemo_mapShanmenItemAuthoritySubsystem& Authority,
		const FShanmenItemAuthoritySnapshot& Snapshot,
		const FShanmenItemReservationSnapshot& Reservation,
		FString& OutDiagnostic)
	{
		FShanmenItemReservationActionRequest Request;
		Request.Context = MakeContext(
			Snapshot, Reservation.RunId, Reservation.OwnerId,
			MakeCancelRequestId(Reservation.ReservationId));
		Request.ReservationId = Reservation.ReservationId;
		const FShanmenItemDurableCommandResult Command =
			Authority.CancelDurable(Request);
		if (!Command.IsCommandSuccess())
		{
			OutDiagnostic = FString::Printf(
				TEXT("Authority could not cancel superseded preparation reservation %s: %s"),
				*GuidDigits(Reservation.ReservationId), *Command.Diagnostic);
			return false;
		}
		return true;
	}

	bool CleanupActiveReservations(
		Udemo_mapShanmenItemAuthoritySubsystem& Authority,
		const FShanmenItemAuthoritySnapshot& Snapshot,
		const FSlotAnalysis& Slot,
		const FGuid& KeepReservationId,
		FString& OutDiagnostic)
	{
		TArray<const FShanmenItemReservationSnapshot*> Ordered =
			Slot.ActiveReservations;
		Ordered.Sort([&Snapshot](
			const FShanmenItemReservationSnapshot& Left,
			const FShanmenItemReservationSnapshot& Right)
		{
			const int32 LeftRevision = ReserveRevision(Snapshot, Left);
			const int32 RightRevision = ReserveRevision(Snapshot, Right);
			return LeftRevision == RightRevision
				? GuidLess(Left.ReservationId, Right.ReservationId)
				: LeftRevision < RightRevision;
		});
		for (const FShanmenItemReservationSnapshot* Reservation : Ordered)
		{
			if (Reservation && Reservation->ReservationId != KeepReservationId
				&& !CancelReservation(Authority, Snapshot, *Reservation, OutDiagnostic))
			{
				return false;
			}
		}
		return true;
	}
}

bool Fdemo_mapShanmenPreparationAdapter::BuildProjection(
	const FShanmenItemAuthoritySnapshot& Snapshot,
	const FGuid& ExpectedOwnerId,
	Fdemo_mapShanmenPreparationAuthorityProjection& OutProjection,
	FString* OutDiagnostic)
{
	auto Fail = [&OutProjection, OutDiagnostic](const FString& Diagnostic)
	{
		OutProjection = Fdemo_mapShanmenPreparationAuthorityProjection();
		if (OutDiagnostic) *OutDiagnostic = Diagnostic;
		return false;
	};

	FShanmenItemRepository Validation;
	EShanmenItemTransactionError ValidationError =
		EShanmenItemTransactionError::None;
	if (!ExpectedOwnerId.IsValid()
		|| !Validation.TryLoadSnapshot(Snapshot, &ValidationError))
	{
		return Fail(FString::Printf(
			TEXT("Preparation authority snapshot failed validation (%d)."),
			static_cast<int32>(ValidationError)));
	}

	FGuid ScopeId;
	for (const FShanmenItemContainer& Container : Snapshot.Containers)
	{
		if (Container.OwnerId != ExpectedOwnerId
			|| (ScopeId.IsValid() && Container.RunId != ScopeId))
		{
			return Fail(TEXT("Preparation authority containers do not share the bound owner and scope."));
		}
		ScopeId = Container.RunId;
	}
	if (!ScopeId.IsValid())
	{
		return Fail(TEXT("Preparation authority has no scoped containers."));
	}
	for (const FShanmenItemInstance& Item : Snapshot.Items)
	{
		if (Item.OwnerId != ExpectedOwnerId || Item.RunId != ScopeId)
		{
			return Fail(TEXT("Preparation authority items do not share the bound owner and scope."));
		}
	}

	TArray<FSlotAnalysis> Slots;
	FString Diagnostic;
	if (!AnalyzeAllSlots(Snapshot, Slots, Diagnostic))
	{
		return Fail(Diagnostic);
	}

	OutProjection = Fdemo_mapShanmenPreparationAuthorityProjection();
	OutProjection.OwnerId = ExpectedOwnerId;
	OutProjection.ScopeId = ScopeId;
	OutProjection.AuthorityRevision = Snapshot.AuthorityRevision;
	for (const FSlotAnalysis& Slot : Slots)
	{
		FGuid* Field = SelectedField(OutProjection, Slot.Spec->SlotId);
		if (!Field)
		{
			return Fail(TEXT("Preparation slot projection mapping is incomplete."));
		}
		*Field = Slot.SelectedItemId;
	}

	const FShanmenItemContainer* Warehouse = nullptr;
	for (const FShanmenItemContainer& Container : Snapshot.Containers)
	{
		if (Container.ContainerType != FName(TEXT("Warehouse"))) continue;
		if (Warehouse)
		{
			return Fail(TEXT("Preparation authority has more than one Warehouse container."));
		}
		Warehouse = &Container;
	}
	if (!Warehouse
		|| Warehouse->Slots.Num() != Fdemo_mapPersistentWarehouseLayout::SlotCount)
	{
		return Fail(TEXT("Preparation authority requires one canonical 30-cell Warehouse container."));
	}
	OutProjection.WarehouseLayout.bInitialized = true;
	OutProjection.WarehouseLayout.SlotItemInstanceIds = Warehouse->Slots;

	for (const FShanmenItemInstance& Item : Snapshot.Items)
	{
		if (Item.State == EShanmenItemInstanceState::Depleted)
		{
			continue;
		}
		Fdemo_mapProfilePreparationStashRow Row;
		Row.ItemInstanceId = Item.ItemInstanceId;
		Row.ItemDefinitionId = Item.DefinitionId;
		Row.StackCount = Item.Quantity;
		Row.bSafeInPermanentStash = true;
		Row.bSelected = IsSelected(OutProjection, Item.ItemInstanceId);
		if (const Fdemo_mapItemDefinition* LegacyDefinition =
			Fdemo_mapItemDefinitions::Find(Item.DefinitionId))
		{
			Row.ItemCategoryId = LegacyDefinition->CategoryId;
			Row.CompatibleEquipmentSlotId =
				LegacyDefinition->CompatibleSlotIds.Num() == 1
				? LegacyDefinition->CompatibleSlotIds[0] : NAME_None;
		}
		// P1.6 deliberately adapts equipment only. Quantity reservation and
		// Hotbar/run-start orchestration remain disabled until their own stage.
		Row.bMaterialSelectionEligible = false;
		OutProjection.OrderedRows.Add(MoveTemp(Row));
	}
	OutProjection.OrderedRows.Sort([](
		const Fdemo_mapProfilePreparationStashRow& Left,
		const Fdemo_mapProfilePreparationStashRow& Right)
	{
		return GuidLess(Left.ItemInstanceId, Right.ItemInstanceId);
	});
	OutProjection.Diagnostic =
		TEXT("P1.6 equipment selection is projected from ShanmenItems; material, Hotbar, and run-start adapters remain disabled.");
	if (OutDiagnostic) OutDiagnostic->Reset();
	return true;
}

Fdemo_mapShanmenPreparationAdapterResult
Fdemo_mapShanmenPreparationAdapter::SelectEquipment(
	Udemo_mapShanmenItemAuthoritySubsystem& Authority,
	FName SlotId,
	const FGuid& ItemInstanceId)
{
	if (!IsInGameThread()
		|| Authority.GetLifecycleState()
			!= Edemo_mapShanmenItemAuthorityLifecycleState::Ready)
	{
		return MakeResult(
			Edemo_mapShanmenPreparationAdapterStatus::AuthorityNotReady,
			TEXT("Authority-native preparation requires a Ready ShanmenItems owner."),
			&Authority);
	}
	const FPreparationSlotSpec* Spec = FindSlotSpec(SlotId);
	if (!Spec)
	{
		return MakeResult(
			Edemo_mapShanmenPreparationAdapterStatus::InvalidSlot,
			TEXT("Preparation equipment requires Weapon, Armor, Accessory, SpatialRing, or Backpack."),
			&Authority);
	}

	FShanmenItemAuthoritySnapshot Snapshot;
	if (!Authority.TryCaptureSnapshot(Snapshot))
	{
		return MakeResult(
			Edemo_mapShanmenPreparationAdapterStatus::InvalidAuthority,
			TEXT("ShanmenItems snapshot is unavailable."), &Authority);
	}
	Fdemo_mapShanmenPreparationAuthorityProjection Projection;
	FString Diagnostic;
	if (!BuildProjection(
		Snapshot, Authority.GetBoundOwnerId(), Projection, &Diagnostic))
	{
		return MakeResult(
			Edemo_mapShanmenPreparationAdapterStatus::InvalidAuthority,
			Diagnostic, &Authority);
	}
	TArray<FSlotAnalysis> Slots;
	if (!AnalyzeAllSlots(Snapshot, Slots, Diagnostic))
	{
		return MakeResult(
			Edemo_mapShanmenPreparationAdapterStatus::InvalidAuthority,
			Diagnostic, &Authority);
	}
	FSlotAnalysis* Slot = Slots.FindByPredicate([SlotId](const FSlotAnalysis& Candidate)
	{
		return Candidate.Spec && Candidate.Spec->SlotId == SlotId;
	});
	if (!Slot)
	{
		return MakeResult(
			Edemo_mapShanmenPreparationAdapterStatus::InvalidSlot,
			TEXT("Preparation slot analysis mapping is incomplete."), &Authority);
	}

	if (ItemInstanceId.IsValid())
	{
		const FShanmenItemInstance* Item = FindItem(Snapshot, ItemInstanceId);
		if (!Item || Item->OwnerId != Projection.OwnerId
			|| Item->RunId != Projection.ScopeId
			|| Item->State == EShanmenItemInstanceState::Depleted)
		{
			return MakeResult(
				Edemo_mapShanmenPreparationAdapterStatus::ItemNotFound,
				TEXT("Equipment ItemInstanceId is absent from the bound ShanmenItems scope."),
				&Authority);
		}
		const FShanmenItemDefinition* Definition =
			FindDefinition(Snapshot, Item->DefinitionId);
		const Fdemo_mapItemDefinition* LegacyDefinition =
			Fdemo_mapItemDefinitions::Find(Item->DefinitionId);
		if (!Definition
			|| !Definition->Supports(EShanmenItemResourceKind::DeploymentLock)
			|| !LegacyDefinition || Item->Quantity != 1
			|| !LegacyDefinition->CompatibleSlotIds.Contains(SlotId))
		{
			return MakeResult(
				Edemo_mapShanmenPreparationAdapterStatus::SlotRejected,
				TEXT("Equipment definition, quantity, or immutable slot compatibility rejected the selection."),
				&Authority);
		}
		for (const FSlotAnalysis& Other : Slots)
		{
			if (Other.Spec->SlotId != SlotId
				&& Other.SelectedItemId == ItemInstanceId)
			{
				return MakeResult(
					Edemo_mapShanmenPreparationAdapterStatus::DuplicateSelection,
					TEXT("The same ItemInstanceId cannot occupy two preparation equipment slots."),
					&Authority);
			}
		}
		for (const FShanmenItemReservationSnapshot& Reservation : Snapshot.Reservations)
		{
			if (Reservation.ItemInstanceId == ItemInstanceId
				&& Reservation.ResourceKind == EShanmenItemResourceKind::DeploymentLock
				&& Reservation.PurposeId != Spec->PurposeId
				&& (Reservation.State == EShanmenItemReservationState::Reserved
					|| Reservation.State == EShanmenItemReservationState::Committed))
			{
				return MakeResult(
					Edemo_mapShanmenPreparationAdapterStatus::DuplicateSelection,
					TEXT("The ItemInstanceId already has another active deployment purpose."),
					&Authority);
			}
		}

		if (Slot->SelectedItemId == ItemInstanceId)
		{
			if (!Slot->Latest || Slot->Latest->State == EShanmenItemReservationState::Committed)
			{
				return MakeResult(
					Edemo_mapShanmenPreparationAdapterStatus::NoChange,
					TEXT("Equipment selection already matches the durable authority projection."),
					&Authority);
			}
			if (!CleanupActiveReservations(
				Authority, Snapshot, *Slot, Slot->Latest->ReservationId, Diagnostic))
			{
				return MakeResult(
					Edemo_mapShanmenPreparationAdapterStatus::CleanupPending,
					Diagnostic, &Authority);
			}
			return MakeResult(
				Slot->ActiveReservations.Num() > 1
					? Edemo_mapShanmenPreparationAdapterStatus::Accepted
					: Edemo_mapShanmenPreparationAdapterStatus::NoChange,
				TEXT("Equipment selection is durable and superseded locks are reconciled."),
				&Authority);
		}
		if (!Slot->CommittedReservations.IsEmpty())
		{
			return MakeResult(
				Edemo_mapShanmenPreparationAdapterStatus::DeployedSelectionLocked,
				TEXT("A committed deployment owns this slot and must be released by run settlement."),
				&Authority);
		}

		FShanmenItemReserveRequest Request;
		Request.Context = MakeContext(
			Snapshot, Projection.ScopeId, Projection.OwnerId,
			MakeReserveRequestId(
				*Slot, Projection.OwnerId, Projection.ScopeId,
				ItemInstanceId, Item->Revision));
		Request.ItemInstanceId = ItemInstanceId;
		Request.ResourceKind = EShanmenItemResourceKind::DeploymentLock;
		Request.Amount = 1;
		Request.ExpectedItemRevision = Item->Revision;
		Request.PurposeId = Spec->PurposeId;
		const FShanmenItemDurableCommandResult Reserve =
			Authority.ReserveDurable(Request);
		if (!Reserve.IsCommandSuccess())
		{
			return MakeResult(
				Edemo_mapShanmenPreparationAdapterStatus::CommandRejected,
				FString::Printf(TEXT("Equipment reserve was rejected: %s"),
					*Reserve.Diagnostic), &Authority);
		}
		if (!Authority.TryCaptureSnapshot(Snapshot)
			|| !AnalyzeAllSlots(Snapshot, Slots, Diagnostic))
		{
			return MakeResult(
				Edemo_mapShanmenPreparationAdapterStatus::InvalidAuthority,
				TEXT("Authority could not project the durable equipment reserve."),
				&Authority);
		}
		Slot = Slots.FindByPredicate([SlotId](const FSlotAnalysis& Candidate)
		{
			return Candidate.Spec && Candidate.Spec->SlotId == SlotId;
		});
		if (!Slot || !Slot->Latest
			|| Slot->Latest->ReservationId != Reserve.Receipt.ReservationId)
		{
			return MakeResult(
				Edemo_mapShanmenPreparationAdapterStatus::InvalidAuthority,
				TEXT("Durable reserve did not become the newest slot intent."),
				&Authority);
		}
		if (!CleanupActiveReservations(
			Authority, Snapshot, *Slot, Reserve.Receipt.ReservationId, Diagnostic))
		{
			return MakeResult(
				Edemo_mapShanmenPreparationAdapterStatus::CleanupPending,
				Diagnostic, &Authority);
		}
		return MakeResult(
			Edemo_mapShanmenPreparationAdapterStatus::Accepted,
			TEXT("Authority-native equipment selection committed."), &Authority);
	}

	if (!Slot->CommittedReservations.IsEmpty())
	{
		return MakeResult(
			Edemo_mapShanmenPreparationAdapterStatus::DeployedSelectionLocked,
			TEXT("A committed deployment cannot be cleared before run settlement."),
			&Authority);
	}
	if (!Slot->bHasHistory && !Slot->BaselineItemId.IsValid())
	{
		return MakeResult(
			Edemo_mapShanmenPreparationAdapterStatus::NoChange,
			TEXT("Preparation equipment slot is already empty."), &Authority);
	}

	if (!Slot->bHasHistory)
	{
		const FShanmenItemInstance* Baseline =
			FindItem(Snapshot, Slot->BaselineItemId);
		if (!Baseline)
		{
			return MakeResult(
				Edemo_mapShanmenPreparationAdapterStatus::InvalidAuthority,
				TEXT("Migrated equipment baseline references a missing item."),
				&Authority);
		}
		FShanmenItemReserveRequest ReserveRequest;
		ReserveRequest.Context = MakeContext(
			Snapshot, Projection.ScopeId, Projection.OwnerId,
			MakeReserveRequestId(
				*Slot, Projection.OwnerId, Projection.ScopeId,
				Baseline->ItemInstanceId, Baseline->Revision));
		ReserveRequest.ItemInstanceId = Baseline->ItemInstanceId;
		ReserveRequest.ResourceKind = EShanmenItemResourceKind::DeploymentLock;
		ReserveRequest.Amount = 1;
		ReserveRequest.ExpectedItemRevision = Baseline->Revision;
		ReserveRequest.PurposeId = Spec->PurposeId;
		const FShanmenItemDurableCommandResult Reserve =
			Authority.ReserveDurable(ReserveRequest);
		if (!Reserve.IsCommandSuccess())
		{
			return MakeResult(
				Edemo_mapShanmenPreparationAdapterStatus::CommandRejected,
				FString::Printf(TEXT("Baseline clear tombstone reserve failed: %s"),
					*Reserve.Diagnostic), &Authority);
		}
		if (!Authority.TryCaptureSnapshot(Snapshot))
		{
			return MakeResult(
				Edemo_mapShanmenPreparationAdapterStatus::InvalidAuthority,
				TEXT("Authority snapshot disappeared after baseline reserve."),
				&Authority);
		}
		const FShanmenItemReservationSnapshot* Reserved =
			Snapshot.Reservations.FindByPredicate([&Reserve](
				const FShanmenItemReservationSnapshot& Candidate)
			{
				return Candidate.ReservationId == Reserve.Receipt.ReservationId;
			});
		if (!Reserved || !CancelReservation(
			Authority, Snapshot, *Reserved, Diagnostic))
		{
			return MakeResult(
				Edemo_mapShanmenPreparationAdapterStatus::CleanupPending,
				Diagnostic.IsEmpty()
					? TEXT("Baseline clear tombstone remains reserved; retry will reconcile it.")
					: Diagnostic,
				&Authority);
		}
		return MakeResult(
			Edemo_mapShanmenPreparationAdapterStatus::Accepted,
			TEXT("Migrated equipment baseline cleared with a durable authority tombstone."),
			&Authority);
	}

	if (Slot->Latest
		&& Slot->Latest->State == EShanmenItemReservationState::Reserved
		&& !CancelReservation(Authority, Snapshot, *Slot->Latest, Diagnostic))
	{
		return MakeResult(
			Edemo_mapShanmenPreparationAdapterStatus::CleanupPending,
			Diagnostic, &Authority);
	}
	if (!Authority.TryCaptureSnapshot(Snapshot)
		|| !AnalyzeAllSlots(Snapshot, Slots, Diagnostic))
	{
		return MakeResult(
			Edemo_mapShanmenPreparationAdapterStatus::InvalidAuthority,
			TEXT("Authority could not re-project the cleared equipment slot."),
			&Authority);
	}
	Slot = Slots.FindByPredicate([SlotId](const FSlotAnalysis& Candidate)
	{
		return Candidate.Spec && Candidate.Spec->SlotId == SlotId;
	});
	if (!Slot || !CleanupActiveReservations(
		Authority, Snapshot, *Slot, FGuid(), Diagnostic))
	{
		return MakeResult(
			Edemo_mapShanmenPreparationAdapterStatus::CleanupPending,
			Diagnostic, &Authority);
	}
	return MakeResult(
		Edemo_mapShanmenPreparationAdapterStatus::Accepted,
		TEXT("Authority-native equipment slot cleared."), &Authority);
}
