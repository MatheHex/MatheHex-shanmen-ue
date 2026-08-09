#include "demo_mapRuntimeContainer.h"
#include "demo_mapItemAuthority.h"
#include "demo_mapItemDefinitions.h"

bool Fdemo_mapRuntimeContainerAuthority::Initialize(
	FGuid InContainerId,
	FGuid InOwningRunId,
	Edemo_mapRuntimeContainerKind InKind,
	const TArray<Fdemo_mapRuntimeContainerResolvedSeedEntry>& ResolvedEntries,
	FString& OutDiagnostic,
	bool bInPlayerDepositAllowed)
{
	OutDiagnostic.Reset();
	if (bInitialized || !InContainerId.IsValid() || !InOwningRunId.IsValid())
	{
		OutDiagnostic = TEXT("Runtime Container requires fresh, valid Container and Run identities.");
		return false;
	}

	const TArray<Edemo_mapRuntimeContainerSection> SectionOrder =
		Fdemo_mapSearchContainerPrototypeConfig::GetSectionOrder(InKind);
	TSet<FGuid> ItemIds;
	TSet<FString> OccupiedSlots;
	TArray<Fdemo_mapRuntimeContainerEntryRecord> NewEntries;
	for (const Fdemo_mapRuntimeContainerResolvedSeedEntry& Seed : ResolvedEntries)
	{
		const int32 Capacity = Fdemo_mapSearchContainerPrototypeConfig::GetSectionCapacity(
			InKind,
			Seed.Section);
		if (!SectionOrder.Contains(Seed.Section)
			|| Seed.SlotIndex < 0
			|| Seed.SlotIndex >= Capacity
			|| !Seed.ItemInstanceId.IsValid()
			|| ItemIds.Contains(Seed.ItemInstanceId)
			|| Seed.DefinitionId.IsNone()
			|| Seed.StackCount <= 0)
		{
			OutDiagnostic = TEXT("Runtime Container seed contains an invalid section, slot, item identity, or stack.");
			return false;
		}
		const FString SlotKey = FString::Printf(
			TEXT("%d:%d"),
			static_cast<int32>(Seed.Section),
			Seed.SlotIndex);
		if (OccupiedSlots.Contains(SlotKey))
		{
			OutDiagnostic = TEXT("Runtime Container seed contains a duplicate occupied slot.");
			return false;
		}
		ItemIds.Add(Seed.ItemInstanceId);
		OccupiedSlots.Add(SlotKey);

		Fdemo_mapRuntimeContainerEntryRecord Entry;
		do
		{
			Entry.EntryId = FGuid::NewGuid();
		}
		while (!Entry.EntryId.IsValid()
			|| NewEntries.ContainsByPredicate(
				[&Entry](const Fdemo_mapRuntimeContainerEntryRecord& Existing)
				{
					return Existing.EntryId == Entry.EntryId;
				}));
		Entry.Section = Seed.Section;
		Entry.SlotIndex = Seed.SlotIndex;
		Entry.InternalItemInstanceId = Seed.ItemInstanceId;
		Entry.ExpectedDefinitionId = Seed.DefinitionId;
		Entry.ExpectedStackCount = Seed.StackCount;
		Entry.SearchDurationSeconds = Seed.SearchDurationSeconds;
		Entry.State = Edemo_mapRuntimeContainerEntryState::Hidden;
		NewEntries.Add(Entry);
	}
	NewEntries.Sort(
		[&SectionOrder](
			const Fdemo_mapRuntimeContainerEntryRecord& A,
			const Fdemo_mapRuntimeContainerEntryRecord& B)
		{
			const int32 SectionA = SectionOrder.IndexOfByKey(A.Section);
			const int32 SectionB = SectionOrder.IndexOfByKey(B.Section);
			return SectionA == SectionB
				? A.SlotIndex < B.SlotIndex
				: SectionA < SectionB;
		});

	ContainerId = InContainerId;
	OwningRunId = InOwningRunId;
	Kind = InKind;
	State = Edemo_mapRuntimeContainerState::Closed;
	ActiveAction = Edemo_mapRuntimeContainerActionKind::None;
	ActiveEntryId.Invalidate();
	Revision = 0;
	Entries = MoveTemp(NewEntries);
	bPlayerDepositAllowed = bInPlayerDepositAllowed;
	bInitialized = true;
	return true;
}

Fdemo_mapRuntimeContainerAuthorityState
Fdemo_mapRuntimeContainerAuthority::CaptureState() const
{
	Fdemo_mapRuntimeContainerAuthorityState Result;
	Result.ContainerId = ContainerId;
	Result.OwningRunId = OwningRunId;
	Result.Kind = Kind;
	Result.State = State;
	Result.ActiveAction = ActiveAction;
	Result.ActiveEntryId = ActiveEntryId;
	Result.Revision = Revision;
	Result.Entries = Entries;
	Result.bPlayerDepositAllowed = bPlayerDepositAllowed;
	Result.bInitialized = bInitialized;
	return Result;
}

Fdemo_mapRuntimeContainerResult
Fdemo_mapRuntimeContainerAuthority::CommitPlayerDropState(
	const Fdemo_mapRuntimeContainerAuthorityState& InState,
	int32 ExpectedRevision,
	FGuid RelatedEntryId,
	FGuid RelatedItemId)
{
	if (!bInitialized
		|| ExpectedRevision != Revision
		|| InState.ContainerId != ContainerId
		|| InState.OwningRunId != OwningRunId
		|| InState.Kind != Kind
		|| InState.bPlayerDepositAllowed != bPlayerDepositAllowed
		|| !InState.bInitialized
		|| InState.State != State
		|| InState.ActiveAction != ActiveAction
		|| InState.ActiveEntryId != ActiveEntryId)
	{
		return Fdemo_mapRuntimeContainerResult::Failure(
			Edemo_mapRuntimeContainerResultCode::StaleRevision,
			TEXT("Runtime Container drag state is stale or targets a different Container."),
			ContainerId,
			Revision,
			RelatedEntryId);
	}
	if (State != Edemo_mapRuntimeContainerState::Opened
		|| ActiveAction != Edemo_mapRuntimeContainerActionKind::None)
	{
		return Fdemo_mapRuntimeContainerResult::Failure(
			Edemo_mapRuntimeContainerResultCode::InvalidState,
			TEXT("Runtime Container drag requires an idle opened Container."),
			ContainerId,
			Revision,
			RelatedEntryId);
	}

	const TArray<Edemo_mapRuntimeContainerSection> SectionOrder =
		Fdemo_mapSearchContainerPrototypeConfig::GetSectionOrder(Kind);
	TSet<FGuid> EntryIds;
	TSet<FGuid> OccupiedItemIds;
	TSet<FString> OccupiedSlots;
	for (const Fdemo_mapRuntimeContainerEntryRecord& Entry : InState.Entries)
	{
		const int32 Capacity =
			Fdemo_mapSearchContainerPrototypeConfig::GetSectionCapacity(
				Kind,
				Entry.Section);
		const FString SlotKey = FString::Printf(
			TEXT("%d:%d"),
			static_cast<int32>(Entry.Section),
			Entry.SlotIndex);
		if (!Entry.EntryId.IsValid()
			|| !Entry.InternalItemInstanceId.IsValid()
			|| Entry.ExpectedDefinitionId.IsNone()
			|| Entry.ExpectedStackCount <= 0
			|| !SectionOrder.Contains(Entry.Section)
			|| Entry.SlotIndex < 0
			|| Entry.SlotIndex >= Capacity
			|| EntryIds.Contains(Entry.EntryId)
			|| OccupiedSlots.Contains(SlotKey)
			|| (Entry.State != Edemo_mapRuntimeContainerEntryState::Taken
				&& OccupiedItemIds.Contains(Entry.InternalItemInstanceId)))
		{
			return Fdemo_mapRuntimeContainerResult::Failure(
				Edemo_mapRuntimeContainerResultCode::InvalidEntry,
				TEXT("Runtime Container drag produced an invalid entry projection."),
				ContainerId,
				Revision,
				RelatedEntryId);
		}
		EntryIds.Add(Entry.EntryId);
		OccupiedSlots.Add(SlotKey);
		if (Entry.State != Edemo_mapRuntimeContainerEntryState::Taken)
		{
			OccupiedItemIds.Add(Entry.InternalItemInstanceId);
		}
	}

	const int32 Before = Revision;
	Entries = InState.Entries;
	Entries.Sort(
		[&SectionOrder](
			const Fdemo_mapRuntimeContainerEntryRecord& A,
			const Fdemo_mapRuntimeContainerEntryRecord& B)
		{
			const int32 SectionA = SectionOrder.IndexOfByKey(A.Section);
			const int32 SectionB = SectionOrder.IndexOfByKey(B.Section);
			return SectionA == SectionB
				? A.SlotIndex < B.SlotIndex
				: SectionA < SectionB;
		});
	++Revision;
	return Fdemo_mapRuntimeContainerResult::Success(
		ContainerId,
		Before,
		Revision,
		RelatedEntryId,
		RelatedItemId);
}

Fdemo_mapRuntimeContainerResult Fdemo_mapRuntimeContainerAuthority::ValidateIntent(
	const Fdemo_mapRuntimeContainerIntent& Intent,
	bool bPlayerAlive,
	bool bInRange) const
{
	if (!bInitialized)
	{
		return Fdemo_mapRuntimeContainerResult::Failure(
			Edemo_mapRuntimeContainerResultCode::NotInitialized,
			TEXT("Runtime Container is not initialized."));
	}
	if (Intent.ExpectedRunId != OwningRunId)
	{
		return Fdemo_mapRuntimeContainerResult::Failure(
			Edemo_mapRuntimeContainerResultCode::InvalidRun,
			TEXT("Runtime Container intent targets the wrong ActiveRun."),
			ContainerId,
			Revision,
			Intent.EntryId);
	}
	if (Intent.ContainerId != ContainerId)
	{
		return Fdemo_mapRuntimeContainerResult::Failure(
			Edemo_mapRuntimeContainerResultCode::InvalidContainer,
			TEXT("Runtime Container intent targets an unknown ContainerId."),
			ContainerId,
			Revision,
			Intent.EntryId);
	}
	if (Intent.ExpectedRevision != Revision)
	{
		return Fdemo_mapRuntimeContainerResult::Failure(
			Edemo_mapRuntimeContainerResultCode::StaleRevision,
			TEXT("Runtime Container intent carries a stale Revision."),
			ContainerId,
			Revision,
			Intent.EntryId);
	}
	if (!bPlayerAlive)
	{
		return Fdemo_mapRuntimeContainerResult::Failure(
			Edemo_mapRuntimeContainerResultCode::PlayerUnavailable,
			TEXT("Runtime Container intent requires a living player."),
			ContainerId,
			Revision,
			Intent.EntryId);
	}
	if (!bInRange)
	{
		return Fdemo_mapRuntimeContainerResult::Failure(
			Edemo_mapRuntimeContainerResultCode::OutOfRange,
			TEXT("Runtime Container intent is outside interaction range."),
			ContainerId,
			Revision,
			Intent.EntryId);
	}
	return Fdemo_mapRuntimeContainerResult::Success(ContainerId, Revision, Revision, Intent.EntryId);
}

Fdemo_mapRuntimeContainerResult Fdemo_mapRuntimeContainerAuthority::SubmitIntent(
	const Fdemo_mapRuntimeContainerIntent& Intent,
	bool bPlayerAlive,
	bool bInRange,
	TFunctionRef<Fdemo_mapItemOperationResult(FGuid ItemInstanceId)> TransferWholeItem)
{
	const Fdemo_mapRuntimeContainerResult Validation = ValidateIntent(
		Intent,
		bPlayerAlive,
		bInRange);
	if (!Validation.bSuccess)
	{
		return Validation;
	}

	switch (Intent.Action)
	{
	case Edemo_mapRuntimeContainerActionKind::BeginOpen:
		return BeginOpen();
	case Edemo_mapRuntimeContainerActionKind::CancelOpen:
		return IsOpening()
			? CancelActiveAction(TEXT("Opening was cancelled."))
			: Fdemo_mapRuntimeContainerResult::Failure(
				Edemo_mapRuntimeContainerResultCode::NoOp,
				TEXT("No Opening action is active."),
				ContainerId,
				Revision);
	case Edemo_mapRuntimeContainerActionKind::BeginSearch:
		return BeginSearch(Intent.EntryId);
	case Edemo_mapRuntimeContainerActionKind::CancelSearch:
	case Edemo_mapRuntimeContainerActionKind::Close:
		return IsSearching()
			? CancelActiveAction(TEXT("Searching was cancelled."))
			: Fdemo_mapRuntimeContainerResult::Success(
				ContainerId,
				Revision,
				Revision,
				Intent.EntryId);
	case Edemo_mapRuntimeContainerActionKind::Take:
		return Take(Intent.EntryId, TransferWholeItem);
	default:
		return Fdemo_mapRuntimeContainerResult::Failure(
			Edemo_mapRuntimeContainerResultCode::InvalidState,
			TEXT("Runtime Container intent has no legal action."),
			ContainerId,
			Revision,
			Intent.EntryId);
	}
}

Fdemo_mapRuntimeContainerResult Fdemo_mapRuntimeContainerAuthority::BeginOpen()
{
	if (ActiveAction != Edemo_mapRuntimeContainerActionKind::None)
	{
		return Fdemo_mapRuntimeContainerResult::Failure(
			Edemo_mapRuntimeContainerResultCode::ConcurrentAction,
			TEXT("Another Runtime Container action is already active."),
			ContainerId,
			Revision);
	}
	if (State == Edemo_mapRuntimeContainerState::Opened)
	{
		return Fdemo_mapRuntimeContainerResult::Failure(
			Edemo_mapRuntimeContainerResultCode::NoOp,
			TEXT("Runtime Container is already Opened."),
			ContainerId,
			Revision);
	}
	if (State != Edemo_mapRuntimeContainerState::Closed)
	{
		return Fdemo_mapRuntimeContainerResult::Failure(
			Edemo_mapRuntimeContainerResultCode::InvalidState,
			TEXT("Runtime Container cannot begin Opening from its current state."),
			ContainerId,
			Revision);
	}
	const int32 Before = Revision;
	State = Edemo_mapRuntimeContainerState::Opening;
	ActiveAction = Edemo_mapRuntimeContainerActionKind::BeginOpen;
	++Revision;
	return Fdemo_mapRuntimeContainerResult::Success(ContainerId, Before, Revision);
}

Fdemo_mapRuntimeContainerResult Fdemo_mapRuntimeContainerAuthority::BeginSearch(FGuid EntryId)
{
	if (ActiveAction != Edemo_mapRuntimeContainerActionKind::None)
	{
		return Fdemo_mapRuntimeContainerResult::Failure(
			Edemo_mapRuntimeContainerResultCode::ConcurrentAction,
			TEXT("Only one Opening or Searching action may be active."),
			ContainerId,
			Revision,
			EntryId);
	}
	if (State != Edemo_mapRuntimeContainerState::Opened)
	{
		return Fdemo_mapRuntimeContainerResult::Failure(
			Edemo_mapRuntimeContainerResultCode::InvalidState,
			TEXT("Entry search requires an Opened Runtime Container."),
			ContainerId,
			Revision,
			EntryId);
	}
	Fdemo_mapRuntimeContainerEntryRecord* Entry = FindMutableEntry(EntryId);
	if (!Entry)
	{
		return Fdemo_mapRuntimeContainerResult::Failure(
			Edemo_mapRuntimeContainerResultCode::InvalidEntry,
			TEXT("Runtime Container EntryId was not found."),
			ContainerId,
			Revision,
			EntryId);
	}
	if (Entry->State == Edemo_mapRuntimeContainerEntryState::Identified
		|| Entry->State == Edemo_mapRuntimeContainerEntryState::Taken)
	{
		return Fdemo_mapRuntimeContainerResult::Failure(
			Edemo_mapRuntimeContainerResultCode::NoOp,
			TEXT("Identified or Taken entries do not search again."),
			ContainerId,
			Revision,
			EntryId);
	}
	if (Entry->State != Edemo_mapRuntimeContainerEntryState::Hidden
		|| Entry->SearchDurationSeconds <= 0.0f)
	{
		return Fdemo_mapRuntimeContainerResult::Failure(
			Edemo_mapRuntimeContainerResultCode::InvalidState,
			TEXT("Runtime Container entry cannot begin Searching."),
			ContainerId,
			Revision,
			EntryId);
	}
	const int32 Before = Revision;
	Entry->State = Edemo_mapRuntimeContainerEntryState::Searching;
	ActiveAction = Edemo_mapRuntimeContainerActionKind::BeginSearch;
	ActiveEntryId = EntryId;
	++Revision;
	return Fdemo_mapRuntimeContainerResult::Success(ContainerId, Before, Revision, EntryId);
}

Fdemo_mapRuntimeContainerResult Fdemo_mapRuntimeContainerAuthority::CompleteActiveAction()
{
	if (!bInitialized || ActiveAction == Edemo_mapRuntimeContainerActionKind::None)
	{
		return Fdemo_mapRuntimeContainerResult::Failure(
			Edemo_mapRuntimeContainerResultCode::NoOp,
			TEXT("Runtime Container has no action to complete."),
			ContainerId,
			Revision,
			ActiveEntryId);
	}

	const int32 Before = Revision;
	if (ActiveAction == Edemo_mapRuntimeContainerActionKind::BeginOpen)
	{
		if (State != Edemo_mapRuntimeContainerState::Opening)
		{
			return Fdemo_mapRuntimeContainerResult::Failure(
				Edemo_mapRuntimeContainerResultCode::InvalidState,
				TEXT("Opening completion found an invalid Container state."),
				ContainerId,
				Revision);
		}
		State = Edemo_mapRuntimeContainerState::Opened;
		ActiveAction = Edemo_mapRuntimeContainerActionKind::None;
		++Revision;
		if (Kind == Edemo_mapRuntimeContainerKind::Corpse)
		{
			for (Fdemo_mapRuntimeContainerEntryRecord& Entry : Entries)
			{
				if (Entry.Section == Edemo_mapRuntimeContainerSection::Equipment
					&& Entry.State == Edemo_mapRuntimeContainerEntryState::Hidden)
				{
					Entry.State = Edemo_mapRuntimeContainerEntryState::Identified;
					++Revision;
				}
			}
		}
		ActiveEntryId.Invalidate();
		return Fdemo_mapRuntimeContainerResult::Success(ContainerId, Before, Revision);
	}

	if (ActiveAction == Edemo_mapRuntimeContainerActionKind::BeginSearch)
	{
		Fdemo_mapRuntimeContainerEntryRecord* Entry = FindMutableEntry(ActiveEntryId);
		if (!Entry || Entry->State != Edemo_mapRuntimeContainerEntryState::Searching)
		{
			return Fdemo_mapRuntimeContainerResult::Failure(
				Edemo_mapRuntimeContainerResultCode::InvalidState,
				TEXT("Searching completion found an invalid Entry state."),
				ContainerId,
				Revision,
				ActiveEntryId);
		}
		const FGuid CompletedEntryId = ActiveEntryId;
		Entry->State = Edemo_mapRuntimeContainerEntryState::Identified;
		ActiveAction = Edemo_mapRuntimeContainerActionKind::None;
		ActiveEntryId.Invalidate();
		++Revision;
		return Fdemo_mapRuntimeContainerResult::Success(
			ContainerId,
			Before,
			Revision,
			CompletedEntryId,
			Entry->InternalItemInstanceId);
	}
	return Fdemo_mapRuntimeContainerResult::Failure(
		Edemo_mapRuntimeContainerResultCode::InvalidState,
		TEXT("Runtime Container active action cannot complete."),
		ContainerId,
		Revision,
		ActiveEntryId);
}

Fdemo_mapRuntimeContainerResult Fdemo_mapRuntimeContainerAuthority::CancelActiveAction(
	const FString& Diagnostic)
{
	if (!IsActionActive())
	{
		return Fdemo_mapRuntimeContainerResult::Success(
			ContainerId,
			Revision,
			Revision);
	}
	const int32 Before = Revision;
	const FGuid CancelledEntryId = ActiveEntryId;
	if (IsOpening())
	{
		State = Edemo_mapRuntimeContainerState::Closed;
	}
	else if (IsSearching())
	{
		Fdemo_mapRuntimeContainerEntryRecord* Entry = FindMutableEntry(ActiveEntryId);
		if (Entry && Entry->State == Edemo_mapRuntimeContainerEntryState::Searching)
		{
			Entry->State = Edemo_mapRuntimeContainerEntryState::Hidden;
		}
	}
	ActiveAction = Edemo_mapRuntimeContainerActionKind::None;
	ActiveEntryId.Invalidate();
	++Revision;
	Fdemo_mapRuntimeContainerResult Result = Fdemo_mapRuntimeContainerResult::Success(
		ContainerId,
		Before,
		Revision,
		CancelledEntryId);
	Result.Diagnostic = Diagnostic;
	return Result;
}

Fdemo_mapRuntimeContainerResult Fdemo_mapRuntimeContainerAuthority::Take(
	FGuid EntryId,
	TFunctionRef<Fdemo_mapItemOperationResult(FGuid ItemInstanceId)> TransferWholeItem)
{
	if (State != Edemo_mapRuntimeContainerState::Opened
		|| ActiveAction != Edemo_mapRuntimeContainerActionKind::None)
	{
		return Fdemo_mapRuntimeContainerResult::Failure(
			Edemo_mapRuntimeContainerResultCode::InvalidState,
			TEXT("Take requires an idle, Opened Runtime Container."),
			ContainerId,
			Revision,
			EntryId);
	}
	Fdemo_mapRuntimeContainerEntryRecord* Entry = FindMutableEntry(EntryId);
	if (!Entry)
	{
		return Fdemo_mapRuntimeContainerResult::Failure(
			Edemo_mapRuntimeContainerResultCode::InvalidEntry,
			TEXT("Take targets an unknown Runtime Container Entry."),
			ContainerId,
			Revision,
			EntryId);
	}
	if (Entry->State != Edemo_mapRuntimeContainerEntryState::Identified)
	{
		return Fdemo_mapRuntimeContainerResult::Failure(
			Entry->State == Edemo_mapRuntimeContainerEntryState::Taken
				? Edemo_mapRuntimeContainerResultCode::NoOp
				: Edemo_mapRuntimeContainerResultCode::InvalidState,
			TEXT("Take accepts only an Identified complete ItemInstance."),
			ContainerId,
			Revision,
			EntryId);
	}
	const FGuid ItemInstanceId = Entry->InternalItemInstanceId;
	const Fdemo_mapItemOperationResult Transfer = TransferWholeItem(ItemInstanceId);
	if (!Transfer.bSuccess)
	{
		return Fdemo_mapRuntimeContainerResult::Failure(
			Edemo_mapRuntimeContainerResultCode::ItemAuthorityRejected,
			Transfer.Diagnostic,
			ContainerId,
			Revision,
			EntryId);
	}
	const int32 Before = Revision;
	Entry->State = Edemo_mapRuntimeContainerEntryState::Taken;
	++Revision;
	return Fdemo_mapRuntimeContainerResult::Success(
		ContainerId,
		Before,
		Revision,
		EntryId,
		ItemInstanceId);
}

Fdemo_mapRuntimeContainerSnapshot Fdemo_mapRuntimeContainerAuthority::BuildSnapshot(
	const Fdemo_mapItemAuthority& ItemAuthority,
	float ActionProgress01,
	int32 InventoryUsedSlots,
	int32 InventoryCapacity,
	const FString& Diagnostic) const
{
	Fdemo_mapRuntimeContainerSnapshot Snapshot;
	Snapshot.ContainerId = ContainerId;
	Snapshot.OwningRunId = OwningRunId;
	Snapshot.Kind = Kind;
	Snapshot.State = State;
	Snapshot.ActiveAction = ActiveAction;
	Snapshot.Revision = Revision;
	Snapshot.ActionProgress01 = FMath::Clamp(ActionProgress01, 0.0f, 1.0f);
	Snapshot.InventoryUsedSlots = InventoryUsedSlots;
	Snapshot.InventoryCapacity = InventoryCapacity;
	Snapshot.bPlayerDepositAllowed = bPlayerDepositAllowed;
	Snapshot.bEmpty = IsEmpty();
	Snapshot.Diagnostic = Diagnostic;

	for (Edemo_mapRuntimeContainerSection Section :
		Fdemo_mapSearchContainerPrototypeConfig::GetSectionOrder(Kind))
	{
		Fdemo_mapRuntimeContainerSectionSnapshot SectionSnapshot;
		SectionSnapshot.Section = Section;
		SectionSnapshot.Capacity =
			Fdemo_mapSearchContainerPrototypeConfig::GetSectionCapacity(Kind, Section);
		for (const Fdemo_mapRuntimeContainerEntryRecord& Entry : Entries)
		{
			if (Entry.Section != Section)
			{
				continue;
			}
			Fdemo_mapRuntimeContainerEntrySnapshot EntrySnapshot;
			EntrySnapshot.EntryId = Entry.EntryId;
			EntrySnapshot.Section = Entry.Section;
			EntrySnapshot.SlotIndex = Entry.SlotIndex;
			EntrySnapshot.State = Entry.State;
			EntrySnapshot.Progress01 =
				Entry.State == Edemo_mapRuntimeContainerEntryState::Searching
					? Snapshot.ActionProgress01
					: (Entry.State == Edemo_mapRuntimeContainerEntryState::Identified
						|| Entry.State == Edemo_mapRuntimeContainerEntryState::Taken
							? 1.0f
							: 0.0f);
			EntrySnapshot.bOccupied =
				Entry.State != Edemo_mapRuntimeContainerEntryState::Taken;
			EntrySnapshot.bCanSearch =
				State == Edemo_mapRuntimeContainerState::Opened
				&& ActiveAction == Edemo_mapRuntimeContainerActionKind::None
				&& Entry.State == Edemo_mapRuntimeContainerEntryState::Hidden;
			EntrySnapshot.bCanTake =
				State == Edemo_mapRuntimeContainerState::Opened
				&& ActiveAction == Edemo_mapRuntimeContainerActionKind::None
				&& Entry.State == Edemo_mapRuntimeContainerEntryState::Identified;

			// Hidden and Searching deliberately expose no identity, definition, name,
			// category, level, price, or stack.
			if (Entry.State == Edemo_mapRuntimeContainerEntryState::Identified)
			{
				const Fdemo_mapItemInstance* Item =
					ItemAuthority.FindInstance(Entry.InternalItemInstanceId);
				const Fdemo_mapItemDefinition* Definition =
					Item ? Fdemo_mapItemDefinitions::Find(Item->DefinitionId) : nullptr;
				if (Item
					&& Definition
					&& Item->DefinitionId == Entry.ExpectedDefinitionId
					&& Item->Quantity == Entry.ExpectedStackCount)
				{
					EntrySnapshot.ItemInstanceId = Item->InstanceId;
					EntrySnapshot.DefinitionId = Definition->DefinitionId;
					EntrySnapshot.DisplayName = Definition->DisplayName;
					EntrySnapshot.CategoryId = Definition->CategoryId;
					EntrySnapshot.Level = Definition->Level;
					EntrySnapshot.StackCount = Item->Quantity;
					EntrySnapshot.UnitSellPrice = Definition->SellPrice;
					EntrySnapshot.RewardEventKind =
						Item->RewardEventKind;
					EntrySnapshot.RewardEventId =
						Item->RewardEventId;
					EntrySnapshot.RewardValueMultiplierBps =
						Item->RewardValueMultiplierBps;
					EntrySnapshot.RewardSourceRoleId =
						Item->RewardSourceRoleId;
					EntrySnapshot.RareRewardEventId =
						Item->RareRewardEventId;
					EntrySnapshot.RareRewardPolicyId =
						Item->RareRewardPolicyId;
					EntrySnapshot.RareRewardTierId =
						Item->RareRewardTierId;
					EntrySnapshot.RareRewardBonusValue =
						Item->RareRewardBonusValue;
					EntrySnapshot.AffixSet = Item->AffixSet;
					Fdemo_mapItemSellValueRules::TryCompute(
						Item->DefinitionId,
						Item->Quantity,
						Item->RewardValueMultiplierBps,
						Item->AffixSet.TotalResolvedValue(),
						Item->RareRewardBonusValue,
						EntrySnapshot.EffectiveStackSellValue);
				}
				else
				{
					EntrySnapshot.bCanTake = false;
				}
			}
			SectionSnapshot.OrderedOccupiedEntries.Add(MoveTemp(EntrySnapshot));
		}
		Snapshot.Sections.Add(MoveTemp(SectionSnapshot));
	}
	return Snapshot;
}

int32 Fdemo_mapRuntimeContainerAuthority::CleanupUnclaimed(
	TFunctionRef<Fdemo_mapItemOperationResult(FGuid ItemInstanceId)> DestroyContainerItem)
{
	CancelActiveAction(TEXT("Runtime Container cleanup cancelled its active action."));
	int32 DestroyedCount = 0;
	for (Fdemo_mapRuntimeContainerEntryRecord& Entry : Entries)
	{
		if (Entry.State == Edemo_mapRuntimeContainerEntryState::Taken)
		{
			continue;
		}
		const Fdemo_mapItemOperationResult Result =
			DestroyContainerItem(Entry.InternalItemInstanceId);
		if (Result.bSuccess
			|| Result.Code == Edemo_mapItemResultCode::InstanceNotFound
			|| Result.Code == Edemo_mapItemResultCode::AlreadyDestroyed)
		{
			Entry.State = Edemo_mapRuntimeContainerEntryState::Taken;
			++DestroyedCount;
		}
	}
	return DestroyedCount;
}

bool Fdemo_mapRuntimeContainerAuthority::IsEmpty() const
{
	return Entries.IsEmpty()
		|| Entries.ContainsByPredicate(
			[](const Fdemo_mapRuntimeContainerEntryRecord& Entry)
			{
				return Entry.State != Edemo_mapRuntimeContainerEntryState::Taken;
			}) == false;
}

float Fdemo_mapRuntimeContainerAuthority::GetActiveActionDuration() const
{
	if (IsOpening())
	{
		return Fdemo_mapSearchContainerPrototypeConfig::GetOpenSeconds(Kind);
	}
	if (IsSearching())
	{
		const Fdemo_mapRuntimeContainerEntryRecord* Entry = FindEntry(ActiveEntryId);
		return Entry ? Entry->SearchDurationSeconds : 0.0f;
	}
	return 0.0f;
}

const Fdemo_mapRuntimeContainerEntryRecord* Fdemo_mapRuntimeContainerAuthority::FindEntry(
	FGuid EntryId) const
{
	return Entries.FindByPredicate(
		[EntryId](const Fdemo_mapRuntimeContainerEntryRecord& Entry)
		{
			return Entry.EntryId == EntryId;
		});
}

Fdemo_mapRuntimeContainerEntryRecord* Fdemo_mapRuntimeContainerAuthority::FindMutableEntry(
	FGuid EntryId)
{
	return Entries.FindByPredicate(
		[EntryId](const Fdemo_mapRuntimeContainerEntryRecord& Entry)
		{
			return Entry.EntryId == EntryId;
		});
}
