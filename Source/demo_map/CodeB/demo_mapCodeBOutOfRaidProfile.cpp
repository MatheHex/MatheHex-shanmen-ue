#include "CodeB/demo_mapCodeBOutOfRaidProfile.h"

#include "demo_mapItemDefinitions.h"
#include "demo_mapPersistentWarehouseTransaction.h"

#include "Dom/JsonObject.h"
#include "HAL/FileManager.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace
{
	using namespace demo_map_code_b;

#if WITH_DEV_AUTOMATION_TESTS
	// This is intentionally process-local.  Production callers cannot request a
	// retry or failure: only the lifecycle product trace can leave a verified
	// Prepared receipt between observer deliveries.
	bool GInterruptAfterRunPreparedReceiptForLifecycleAutomation = false;
#endif

	FString GuidText(const FGuid& Value)
	{
		return Value.IsValid()
			? Value.ToString(EGuidFormats::DigitsWithHyphensLower)
			: FString();
	}

	bool TryGuidText(const FString& Text, FGuid& OutGuid)
	{
		OutGuid.Invalidate();
		return Text.IsEmpty() || FGuid::Parse(Text, OutGuid);
	}

	bool ReadGuid(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field, FGuid& OutGuid, FString& OutError, bool bRequired = false)
	{
		FString Value;
		if (!Object.IsValid() || !Object->TryGetStringField(Field, Value)
			|| !TryGuidText(Value, OutGuid) || (bRequired && !OutGuid.IsValid()))
		{
			OutError = FString::Printf(TEXT("Code B profile field %s is not a valid GUID."), Field);
			return false;
		}
		return true;
	}

	bool ReadInt(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field, int32& OutValue, FString& OutError)
	{
		double Value = 0.0;
		if (!Object.IsValid() || !Object->TryGetNumberField(Field, Value)
			|| !FMath::IsNearlyEqual(Value, FMath::RoundToDouble(Value))
			|| Value < MIN_int32 || Value > MAX_int32)
		{
			OutError = FString::Printf(TEXT("Code B profile field %s is not an integer."), Field);
			return false;
		}
		OutValue = static_cast<int32>(Value);
		return true;
	}

	FString UtcNow()
	{
		return FDateTime::UtcNow().ToIso8601();
	}

	bool BuildP24PriorComposite(
		const FCodeBSnapshot& PlayerSnapshot,
		const FCodeBSnapshot& TargetSnapshot,
		FCodeBSnapshot& OutComposite,
		FString& OutError)
	{
		OutComposite = PlayerSnapshot;
		OutComposite.Revision = FMath::Max(PlayerSnapshot.Revision, TargetSnapshot.Revision);
		for (const TPair<FName, FCodeBItemDefinition>& Pair : TargetSnapshot.Definitions)
		{
			if (const FCodeBItemDefinition* Existing = OutComposite.Definitions.Find(Pair.Key);
				Existing && !(*Existing == Pair.Value))
			{
				OutError = TEXT("P24 split boundary found conflicting player/target definitions.");
				return false;
			}
			OutComposite.Definitions.Add(Pair.Key, Pair.Value);
		}
		for (const TPair<FGuid, FCodeBItemInstance>& Pair : TargetSnapshot.Items)
		{
			if (OutComposite.Items.Contains(Pair.Key))
			{
				OutError = TEXT("P24 split boundary found duplicate player/target item identities.");
				return false;
			}
			OutComposite.Items.Add(Pair.Key, Pair.Value);
		}
		for (const TPair<FGuid, FCodeBContainer>& Pair : TargetSnapshot.Containers)
		{
			if (OutComposite.Containers.Contains(Pair.Key))
			{
				OutError = TEXT("P24 split boundary found duplicate player/target container identities.");
				return false;
			}
			OutComposite.Containers.Add(Pair.Key, Pair.Value);
		}
		return true;
	}

	/**
	 * A cross-graph candidate may contain one new identity only when the entire
	 * delta is structurally identical to P1 ExecuteSplit.  The random identity
	 * value itself is irrelevant; every other field, slot and quantity is proven.
	 */
	bool IsExactP24SplitDelta(
		const FCodeBSnapshot& Prior,
		const FCodeBSnapshot& Candidate,
		FString& OutError)
	{
		if (Prior.Revision == MAX_int32 || Candidate.Revision != Prior.Revision + 1
			|| Candidate.Definitions.Num() != Prior.Definitions.Num()
			|| Candidate.Containers.Num() != Prior.Containers.Num()
			|| Candidate.Items.Num() != Prior.Items.Num() + 1)
		{
			OutError = TEXT("P24 split candidate has a non-Split revision or graph cardinality delta.");
			return false;
		}
		for (const TPair<FName, FCodeBItemDefinition>& Pair : Prior.Definitions)
		{
			const FCodeBItemDefinition* CandidateDefinition = Candidate.Definitions.Find(Pair.Key);
			if (!CandidateDefinition || !(*CandidateDefinition == Pair.Value))
			{
				OutError = TEXT("P24 split candidate mutated an item definition.");
				return false;
			}
		}

		const FCodeBItemInstance* NewItem = nullptr;
		for (const TPair<FGuid, FCodeBItemInstance>& Pair : Candidate.Items)
		{
			if (!Prior.Items.Contains(Pair.Key))
			{
				if (NewItem)
				{
					OutError = TEXT("P24 split candidate created more than one item identity.");
					return false;
				}
				NewItem = &Pair.Value;
			}
		}
		if (!NewItem || !NewItem->ItemId.IsValid() || Prior.Items.Contains(NewItem->ItemId)
			|| NewItem->ChildContainerId.IsValid() || !NewItem->IsPlaced())
		{
			OutError = TEXT("P24 split candidate has no one valid simple created item.");
			return false;
		}

		const FCodeBItemInstance* OriginalSource = nullptr;
		const FCodeBItemInstance* CandidateSource = nullptr;
		for (const TPair<FGuid, FCodeBItemInstance>& Pair : Prior.Items)
		{
			const FCodeBItemInstance* Current = Candidate.Items.Find(Pair.Key);
			if (!Current)
			{
				OutError = TEXT("P24 split candidate removed a pre-existing item.");
				return false;
			}
			if (!(Pair.Value == *Current))
			{
				if (OriginalSource)
				{
					OutError = TEXT("P24 split candidate mutated more than the source stack.");
					return false;
				}
				OriginalSource = &Pair.Value;
				CandidateSource = Current;
			}
		}
		if (!OriginalSource || !CandidateSource || OriginalSource->ChildContainerId.IsValid())
		{
			OutError = TEXT("P24 split candidate has no one simple source stack.");
			return false;
		}
		const FCodeBItemDefinition* Definition = Prior.Definitions.Find(OriginalSource->DefinitionId);
		const FCodeBContainer* SourceContainer = Prior.Containers.Find(OriginalSource->ParentContainerId);
		const FCodeBContainer* TargetContainer = Prior.Containers.Find(NewItem->ParentContainerId);
		if (!Definition || !Definition->bStackable || Definition->MaxStack <= 1
			|| !SourceContainer || SourceContainer->IsEquipment()
			|| !TargetContainer || TargetContainer->IsEquipment()
			|| !SourceContainer->Slots.IsValidIndex(OriginalSource->SlotIndex)
			|| SourceContainer->Slots[OriginalSource->SlotIndex] != OriginalSource->ItemId
			|| !TargetContainer->Slots.IsValidIndex(NewItem->SlotIndex)
			|| TargetContainer->Slots[NewItem->SlotIndex].IsValid()
			|| NewItem->Quantity < 1 || NewItem->Quantity >= OriginalSource->Quantity
			|| NewItem->Quantity > Definition->MaxStack)
		{
			OutError = TEXT("P24 split candidate violates the P1 source, target, stack, or quantity rules.");
			return false;
		}

		FCodeBItemInstance ExpectedSource = *OriginalSource;
		ExpectedSource.Quantity -= NewItem->Quantity;
		FCodeBItemInstance ExpectedNew = *OriginalSource;
		ExpectedNew.ItemId = NewItem->ItemId;
		ExpectedNew.Quantity = NewItem->Quantity;
		ExpectedNew.ParentContainerId = NewItem->ParentContainerId;
		ExpectedNew.SlotIndex = NewItem->SlotIndex;
		if (!(ExpectedSource == *CandidateSource) || !(ExpectedNew == *NewItem))
		{
			OutError = TEXT("P24 split candidate differs from P1's exact source/new-item field delta.");
			return false;
		}
		for (const TPair<FGuid, FCodeBContainer>& Pair : Prior.Containers)
		{
			const FCodeBContainer* Current = Candidate.Containers.Find(Pair.Key);
			if (!Current)
			{
				OutError = TEXT("P24 split candidate removed a pre-existing container.");
				return false;
			}
			FCodeBContainer Expected = Pair.Value;
			if (Pair.Key == NewItem->ParentContainerId)
			{
				Expected.Slots[NewItem->SlotIndex] = NewItem->ItemId;
			}
			if (!(Expected == *Current))
			{
				OutError = TEXT("P24 split candidate changed a container outside the one explicit empty target slot.");
				return false;
			}
		}
		return true;
	}

	bool HasP25MergeQuantityOrRemovalDelta(
		const FCodeBSnapshot& Prior,
		const FCodeBSnapshot& Candidate)
	{
		for (const TPair<FGuid, FCodeBItemInstance>& Pair : Prior.Items)
		{
			const FCodeBItemInstance* Current = Candidate.Items.Find(Pair.Key);
			if (!Current || Current->Quantity != Pair.Value.Quantity)
			{
				return true;
			}
		}
		return false;
	}

	/**
	 * P25 accepts a cross-graph quantity delta only when it is structurally
	 * identical to one P1 ExecuteMerge. No identity is created; partial merge
	 * retains both placements, while a fully consumed source alone is removed.
	 */
	bool IsExactP25MergeDelta(
		const FCodeBSnapshot& Prior,
		const FCodeBSnapshot& Candidate,
		FString& OutError)
	{
		if (Prior.Revision == MAX_int32 || Candidate.Revision != Prior.Revision + 1
			|| Candidate.Definitions.Num() != Prior.Definitions.Num()
			|| Candidate.Containers.Num() != Prior.Containers.Num()
			|| (Candidate.Items.Num() != Prior.Items.Num()
				&& Candidate.Items.Num() != Prior.Items.Num() - 1))
		{
			OutError = TEXT("P25 merge candidate has a non-Merge revision or graph cardinality delta.");
			return false;
		}
		for (const TPair<FName, FCodeBItemDefinition>& Pair : Prior.Definitions)
		{
			const FCodeBItemDefinition* Current = Candidate.Definitions.Find(Pair.Key);
			if (!Current || !(*Current == Pair.Value))
			{
				OutError = TEXT("P25 merge candidate mutated an item definition.");
				return false;
			}
		}
		for (const TPair<FGuid, FCodeBItemInstance>& Pair : Candidate.Items)
		{
			if (!Prior.Items.Contains(Pair.Key))
			{
				OutError = TEXT("P25 merge candidate invented an item identity.");
				return false;
			}
		}

		const FCodeBItemInstance* OriginalSource = nullptr;
		const FCodeBItemInstance* CandidateSource = nullptr;
		const FCodeBItemInstance* OriginalTarget = nullptr;
		const FCodeBItemInstance* CandidateTarget = nullptr;
		for (const TPair<FGuid, FCodeBItemInstance>& Pair : Prior.Items)
		{
			const FCodeBItemInstance* Current = Candidate.Items.Find(Pair.Key);
			if (!Current || Current->Quantity < Pair.Value.Quantity)
			{
				if (OriginalSource)
				{
					OutError = TEXT("P25 merge candidate changed more than one source stack.");
					return false;
				}
				OriginalSource = &Pair.Value;
				CandidateSource = Current;
				continue;
			}
			if (Current->Quantity > Pair.Value.Quantity)
			{
				if (OriginalTarget)
				{
					OutError = TEXT("P25 merge candidate changed more than one target stack.");
					return false;
				}
				OriginalTarget = &Pair.Value;
				CandidateTarget = Current;
				continue;
			}
			if (!(Pair.Value == *Current))
			{
				OutError = TEXT("P25 merge candidate mutated a non-quantity item field.");
				return false;
			}
		}
		if (!OriginalSource || !OriginalTarget || !CandidateTarget
			|| OriginalSource->ItemId == OriginalTarget->ItemId
			|| OriginalSource->DefinitionId != OriginalTarget->DefinitionId
			|| OriginalSource->ChildContainerId.IsValid() || OriginalTarget->ChildContainerId.IsValid())
		{
			OutError = TEXT("P25 merge candidate has no one compatible simple source and target pair.");
			return false;
		}

		const FCodeBItemDefinition* Definition = Prior.Definitions.Find(OriginalSource->DefinitionId);
		const FCodeBContainer* SourceContainer = Prior.Containers.Find(OriginalSource->ParentContainerId);
		const FCodeBContainer* TargetContainer = Prior.Containers.Find(OriginalTarget->ParentContainerId);
		if (!Definition || !Definition->bStackable || Definition->MaxStack <= 1
			|| !SourceContainer || SourceContainer->IsEquipment()
			|| !TargetContainer || TargetContainer->IsEquipment()
			|| !SourceContainer->Slots.IsValidIndex(OriginalSource->SlotIndex)
			|| SourceContainer->Slots[OriginalSource->SlotIndex] != OriginalSource->ItemId
			|| !TargetContainer->Slots.IsValidIndex(OriginalTarget->SlotIndex)
			|| TargetContainer->Slots[OriginalTarget->SlotIndex] != OriginalTarget->ItemId)
		{
			OutError = TEXT("P25 merge candidate violates the P1 stack or storage placement rules.");
			return false;
		}

		const int32 Amount = CandidateTarget->Quantity - OriginalTarget->Quantity;
		const int32 SourceRemainder = OriginalSource->Quantity - Amount;
		if (Amount <= 0 || Amount > OriginalSource->Quantity
			|| CandidateTarget->Quantity > Definition->MaxStack || SourceRemainder < 0)
		{
			OutError = TEXT("P25 merge candidate has an invalid accepted quantity or exceeds MaxStack.");
			return false;
		}
		FCodeBItemInstance ExpectedTarget = *OriginalTarget;
		ExpectedTarget.Quantity += Amount;
		if (!(ExpectedTarget == *CandidateTarget))
		{
			OutError = TEXT("P25 merge target differs from P1's exact quantity-only delta.");
			return false;
		}
		if (SourceRemainder > 0)
		{
			FCodeBItemInstance ExpectedSource = *OriginalSource;
			ExpectedSource.Quantity = SourceRemainder;
			if (!CandidateSource || !(ExpectedSource == *CandidateSource))
			{
				OutError = TEXT("P25 partial merge did not retain the exact source identity and placement.");
				return false;
			}
		}
		else if (CandidateSource)
		{
			OutError = TEXT("P25 full merge retained a zero-quantity source item.");
			return false;
		}

		for (const TPair<FGuid, FCodeBContainer>& Pair : Prior.Containers)
		{
			const FCodeBContainer* Current = Candidate.Containers.Find(Pair.Key);
			if (!Current)
			{
				OutError = TEXT("P25 merge candidate removed a pre-existing container.");
				return false;
			}
			FCodeBContainer Expected = Pair.Value;
			if (SourceRemainder == 0 && Pair.Key == OriginalSource->ParentContainerId)
			{
				Expected.Slots[OriginalSource->SlotIndex] = FGuid();
			}
			if (!(Expected == *Current))
			{
				OutError = TEXT("P25 merge candidate changed a container outside source cleanup.");
				return false;
			}
		}
		return true;
	}

	/**
	 * P28 tightens P25's generic Merge proof with the exact accepted command.
	 * The world source and occupied player target must both retain identity and
	 * placement, no item may be created or deleted, and the two quantities must
	 * change by precisely the explicitly confirmed N.
	 */
	bool IsExactP28WorldPickupMergeDelta(
		const FCodeBSnapshot& Prior,
		const FCodeBSnapshot& Candidate,
		const FCodeBP2Command& AcceptedCommand,
		const FGuid& WorldSourceItemId,
		FString& OutError)
	{
		if (AcceptedCommand.Operation != ECodeBOperation::Merge
			|| AcceptedCommand.Quantity <= 0
			|| AcceptedCommand.ExpectedRevision != Prior.Revision
			|| AcceptedCommand.ItemId != WorldSourceItemId
			|| (AcceptedCommand.SourceContainerId == AcceptedCommand.TargetContainerId
				&& AcceptedCommand.SourceSlot == AcceptedCommand.TargetSlot))
		{
			OutError = TEXT("P28 requires one explicit positive-quantity P1 Merge command from the world source.");
			return false;
		}
		const FCodeBItemInstance* Source = Prior.Items.Find(WorldSourceItemId);
		const FCodeBContainer* SourceContainer = Source
			? Prior.Containers.Find(Source->ParentContainerId) : nullptr;
		const FCodeBContainer* TargetContainer = Prior.Containers.Find(AcceptedCommand.TargetContainerId);
		const FGuid TargetItemId = TargetContainer
			&& TargetContainer->Slots.IsValidIndex(AcceptedCommand.TargetSlot)
			? TargetContainer->Slots[AcceptedCommand.TargetSlot] : FGuid();
		const FCodeBItemInstance* Target = Prior.Items.Find(TargetItemId);
		const FCodeBItemDefinition* Definition = Source
			? Prior.Definitions.Find(Source->DefinitionId) : nullptr;
		if (!Source || !SourceContainer || SourceContainer->IsEquipment()
			|| Source->ParentContainerId != AcceptedCommand.SourceContainerId
			|| Source->SlotIndex != AcceptedCommand.SourceSlot
			|| !SourceContainer->Slots.IsValidIndex(Source->SlotIndex)
			|| SourceContainer->Slots[Source->SlotIndex] != Source->ItemId
			|| !Target || !TargetContainer || TargetContainer->IsEquipment()
			|| Target->ParentContainerId != AcceptedCommand.TargetContainerId
			|| Target->SlotIndex != AcceptedCommand.TargetSlot
			|| Target->ItemId == Source->ItemId
			|| !Definition || !Definition->bStackable || Definition->MaxStack <= 1
			|| Source->DefinitionId != Target->DefinitionId
			|| Source->ChildContainerId.IsValid() || Target->ChildContainerId.IsValid()
			|| Source->Quantity <= AcceptedCommand.Quantity
			|| Target->Quantity <= 0 || Target->Quantity >= Definition->MaxStack
			|| Definition->MaxStack - Target->Quantity < AcceptedCommand.Quantity)
		{
			OutError = TEXT("P28 Merge command violates the exact simple-stack source, occupied target, or available-capacity gate.");
			return false;
		}
		if (Candidate.Items.Num() != Prior.Items.Num()
			|| !IsExactP25MergeDelta(Prior, Candidate, OutError))
		{
			if (OutError.IsEmpty())
			{
				OutError = TEXT("P28 exact partial Merge must retain every existing ItemId.");
			}
			return false;
		}
		const FCodeBItemInstance* CandidateSource = Candidate.Items.Find(Source->ItemId);
		const FCodeBItemInstance* CandidateTarget = Candidate.Items.Find(Target->ItemId);
		FCodeBItemInstance ExpectedSource = *Source;
		ExpectedSource.Quantity -= AcceptedCommand.Quantity;
		FCodeBItemInstance ExpectedTarget = *Target;
		ExpectedTarget.Quantity += AcceptedCommand.Quantity;
		if (!CandidateSource || !CandidateTarget
			|| !(ExpectedSource == *CandidateSource)
			|| !(ExpectedTarget == *CandidateTarget))
		{
			OutError = TEXT("P28 candidate does not contain the exact source -N and target +N quantity-only delta.");
			return false;
		}
		return true;
	}

	/** P16's closed Code B-only content catalog.  It has no Actor, UI, or RNG dependency. */
	struct FCodeBLootProfileCandidate
	{
		FName EntryId;
		FName ItemDefinitionId;
		int32 Weight = 0;
		int32 MinQuantity = 0;
		int32 MaxQuantity = 0;
		int32 TieBreakOrder = INDEX_NONE;
	};

	struct FCodeBLootProfileRollGroup
	{
		FName GroupId;
		int32 SelectionCount = 0;
		bool bOptional = false;
		/** Explicit no-drop weight for an optional deterministic group; zero for guaranteed groups. */
		int32 NoDropWeight = 0;
		/** Explicit spawn weight for an optional deterministic group; zero for guaranteed groups. */
		int32 SpawnWeight = 0;
		int32 GroupOrder = INDEX_NONE;
		TArray<FCodeBLootProfileCandidate> Candidates;
	};

	struct FCodeBLootProfile
	{
		FName LootProfileId;
		FName SourceContainerDefinitionId;
		int32 ProfileVersion = 0;
		FString AlgorithmVersion;
		TArray<FCodeBLootProfileRollGroup> RollGroups;
	};

	struct FCodeBLootProfileRollEntry
	{
		FName ItemDefinitionId;
		int32 Quantity = 0;
		int32 SlotIndex = INDEX_NONE;
		int32 GroupOrder = INDEX_NONE;
		int32 CandidateTieBreakOrder = INDEX_NONE;
	};

	struct FCodeBLootProfileRollResult
	{
		FName LootProfileId;
		int32 ProfileVersion = 0;
		FString ProfileDigest;
		FString AlgorithmVersion;
		FString ResultDigest;
		TArray<FCodeBLootProfileRollEntry> Entries;
	};

	const FCodeBBodyContainerDefinition* FindBodyContainerDefinitionInternal(FName DefinitionId);
	const FCodeBLootProfile* FindLootProfileInternal(FName SourceContainerDefinitionId);
	const FCodeBLootProfile* FindLootProfileByProvenance(
		FName SourceContainerDefinitionId,
		FName LootProfileId,
		int32 LootProfileVersion);
	bool ValidateLootProfileMaterialization(
		const FCodeBLootProfileRollResult& Roll,
		FName SourceContainerDefinitionId,
		int32 SourceCapacity,
		FString& OutError);
	bool BuildDeterministicLootProfileRoll(
		const FGuid& OwnerId,
		const FGuid& RunInstanceId,
		const FGuid& TargetId,
		FName SourceContainerDefinitionId,
		int32 SourceCapacity,
		const FGuid& DeathReceiptId,
		FCodeBLootProfileRollResult& OutRoll,
		FString& OutError);
	bool BuildDeterministicLootProfileRollForProfile(
		const FGuid& OwnerId,
		const FGuid& RunInstanceId,
		const FGuid& TargetId,
		FName SourceContainerDefinitionId,
		int32 SourceCapacity,
		const FGuid& DeathReceiptId,
		const FCodeBLootProfile& Profile,
		FCodeBLootProfileRollResult& OutRoll,
		FString& OutError);
	bool HasLootProfileProvenance(
		FName LootProfileId,
		int32 LootProfileVersion,
		const FString& LootProfileDigest,
		const FString& LootAlgorithmVersion,
		const FString& LootResultDigest);
	bool ValidateLootProfileReceiptProvenance(
		const FGuid& OwnerId,
		const FGuid& RunInstanceId,
		const FGuid& TargetId,
		FName SourceContainerDefinitionId,
		int32 SourceCapacity,
		const FGuid& DeathReceiptId,
		FName LootProfileId,
		int32 LootProfileVersion,
		const FString& LootProfileDigestText,
		const FString& LootAlgorithmVersion,
		const FString& LootResultDigest,
		FCodeBLootProfileRollResult* OutExpectedRoll,
		FString& OutError);
	bool ValidateInitialLootProfileGraph(
		const FGuid& OwnerId,
		const FGuid& RunInstanceId,
		const FGuid& TargetId,
		FName SourceContainerDefinitionId,
		int32 SourceCapacity,
		const FGuid& DeathReceiptId,
		FName LootProfileId,
		int32 LootProfileVersion,
		const FString& LootProfileDigestText,
		const FString& LootAlgorithmVersion,
		const FString& LootResultDigest,
		const FGuid& RootContainerId,
		const FCodeBSnapshot& Snapshot,
		FString& OutError);

	const TCHAR* TerminalStateText(const ECodeBRunInventoryTerminalState State)
	{
		switch (State)
		{
		case ECodeBRunInventoryTerminalState::Extracted: return TEXT("Extracted");
		case ECodeBRunInventoryTerminalState::Dead: return TEXT("Dead");
		case ECodeBRunInventoryTerminalState::RecoveredAbandon: return TEXT("RecoveredAbandon");
		default: return TEXT("Unknown");
		}
	}

	bool TryTerminalState(const FString& Text, ECodeBRunInventoryTerminalState& OutState)
	{
		OutState = ECodeBRunInventoryTerminalState::Unknown;
		if (Text == TEXT("Extracted")) OutState = ECodeBRunInventoryTerminalState::Extracted;
		else if (Text == TEXT("Dead")) OutState = ECodeBRunInventoryTerminalState::Dead;
		else if (Text == TEXT("RecoveredAbandon")) OutState = ECodeBRunInventoryTerminalState::RecoveredAbandon;
		return OutState != ECodeBRunInventoryTerminalState::Unknown;
	}

	bool EquivalentLayout(const FCodeBP2PlayerLayout& Left, const FCodeBP2PlayerLayout& Right)
	{
		return Left.LayoutId == Right.LayoutId
			&& Left.WarehouseContainerId == Right.WarehouseContainerId
			&& Left.BasicContainerId == Right.BasicContainerId
			&& Left.WeaponContainerId == Right.WeaponContainerId
			&& Left.ArmorContainerId == Right.ArmorContainerId
			&& Left.SpatialContainerId == Right.SpatialContainerId
			&& Left.BackpackContainerId == Right.BackpackContainerId
			&& Left.SpatialInternalContainerId == Right.SpatialInternalContainerId
			&& Left.PouchInternalContainerId == Right.PouchInternalContainerId
			&& Left.AccessoryContainerIds == Right.AccessoryContainerIds
			&& Left.bUseConditionalSpatialContainers == Right.bUseConditionalSpatialContainers;
	}

	FString TerminalSnapshotDigest(
		const FCodeBSnapshot& Snapshot,
		const FCodeBP2PlayerLayout& Layout)
	{
		TArray<FString> Pieces;
		Pieces.Add(FString::Printf(TEXT("revision:%d"), Snapshot.Revision));
		Pieces.Add(FString::Printf(TEXT("layout:%s:%s:%s:%s:%s:%s:%s:%s:%s:%d"),
			*Layout.LayoutId.ToString(), *GuidText(Layout.WarehouseContainerId),
			*GuidText(Layout.BasicContainerId), *GuidText(Layout.WeaponContainerId),
			*GuidText(Layout.ArmorContainerId), *GuidText(Layout.SpatialContainerId),
			*GuidText(Layout.BackpackContainerId), *GuidText(Layout.SpatialInternalContainerId),
			*GuidText(Layout.PouchInternalContainerId), Layout.bUseConditionalSpatialContainers ? 1 : 0));
		for (const FGuid& AccessoryId : Layout.AccessoryContainerIds)
		{
			Pieces.Add(FString::Printf(TEXT("accessory:%s"), *GuidText(AccessoryId)));
		}
		for (const TPair<FName, FCodeBItemDefinition>& Pair : Snapshot.Definitions)
		{
			const FCodeBItemDefinition& Definition = Pair.Value;
			Pieces.Add(FString::Printf(TEXT("definition:%s:%d:%d:%d:%d:%d:%d"),
				*Definition.DefinitionId.ToString(), static_cast<int32>(Definition.ItemType),
				Definition.bStackable ? 1 : 0, Definition.MaxStack,
				static_cast<int32>(Definition.EquipSlot), Definition.GridWidth, Definition.GridHeight));
		}
		for (const TPair<FGuid, FCodeBItemInstance>& Pair : Snapshot.Items)
		{
			const FCodeBItemInstance& Item = Pair.Value;
			Pieces.Add(FString::Printf(TEXT("item:%s:%s:%d:%d:%d:%d:%s:%d:%s:%s"),
				*GuidText(Item.ItemId), *Item.DefinitionId.ToString(), Item.Quantity,
				Item.Level, Item.Quality, Item.RandomSeed, *GuidText(Item.ParentContainerId),
				Item.SlotIndex, *GuidText(Item.ChildContainerId), *Item.LegacyAffixDigest));
		}
		for (const TPair<FGuid, FCodeBContainer>& Pair : Snapshot.Containers)
		{
			const FCodeBContainer& Container = Pair.Value;
			TArray<FString> Slots;
			for (const FGuid& ItemId : Container.Slots) Slots.Add(GuidText(ItemId));
			Pieces.Add(FString::Printf(TEXT("container:%s:%s:%d:%d:%s"),
				*GuidText(Container.ContainerId), *Container.ContainerType.ToString(),
				static_cast<int32>(Container.Kind), static_cast<int32>(Container.EquipmentSlot),
				*FString::Join(Slots, TEXT(","))));
		}
		Pieces.Sort();
		return FString::Printf(TEXT("%08X"), FCrc::StrCrc32(*FString::Join(Pieces, TEXT("|"))));
	}

	FString RunInventoryPayloadDigest(const FCodeBRunInventorySession& Session)
	{
		TArray<FString> Pieces;
		Pieces.Add(FString::FromInt(Session.SourceOutOfRaidRevision));
		const auto AddItem = [&Pieces](
			const FGuid& ItemId,
			const FName DefinitionId,
			const int32 Quantity,
			const FGuid& ParentContainerId,
			const int32 SlotIndex,
			const FGuid& ChildContainerId)
		{
			Pieces.Add(FString::Printf(TEXT("%s:%s:%d:%s:%d:%s"),
				*GuidText(ItemId), *DefinitionId.ToString(), Quantity,
				*GuidText(ParentContainerId), SlotIndex, *GuidText(ChildContainerId)));
		};
		if (!Session.Receipt.ImmutablePayloadItems.IsEmpty())
		{
			for (const FCodeBRunInventoryReceiptItem& Item : Session.Receipt.ImmutablePayloadItems)
			{
				AddItem(Item.ItemId, Item.DefinitionId, Item.Quantity, Item.ParentContainerId, Item.SlotIndex, Item.ChildContainerId);
			}
			Pieces.Sort();
			return FString::Printf(TEXT("%08X"), FCrc::StrCrc32(*FString::Join(Pieces, TEXT("|"))));
		}
		for (const FGuid& ItemId : Session.Receipt.MovedItemIds)
		{
			const FCodeBItemInstance* Item = Session.RepositorySnapshot.Items.Find(ItemId);
			if (!Item)
			{
				Pieces.Add(FString::Printf(TEXT("missing:%s"), *GuidText(ItemId)));
				continue;
			}
			AddItem(ItemId, Item->DefinitionId, Item->Quantity, Item->ParentContainerId, Item->SlotIndex, Item->ChildContainerId);
		}
		Pieces.Sort();
		return FString::Printf(TEXT("%08X"), FCrc::StrCrc32(*FString::Join(Pieces, TEXT("|"))));
	}

	bool FreezeRunInventoryPayloadReceipt(FCodeBRunInventorySession& Session, FString& OutError)
	{
		if (!Session.Receipt.ImmutablePayloadItems.IsEmpty())
		{
			return true;
		}
		for (const FGuid& ItemId : Session.Receipt.MovedItemIds)
		{
			const FCodeBItemInstance* Item = Session.RepositorySnapshot.Items.Find(ItemId);
			if (!Item)
			{
				OutError = TEXT("Code B Run inventory cannot freeze a receipt item missing from the active session.");
				return false;
			}
			FCodeBRunInventoryReceiptItem FrozenItem;
			FrozenItem.ItemId = ItemId;
			FrozenItem.DefinitionId = Item->DefinitionId;
			FrozenItem.Quantity = Item->Quantity;
			FrozenItem.ParentContainerId = Item->ParentContainerId;
			FrozenItem.SlotIndex = Item->SlotIndex;
			FrozenItem.ChildContainerId = Item->ChildContainerId;
			Session.Receipt.ImmutablePayloadItems.Add(MoveTemp(FrozenItem));
		}
		Session.Receipt.ImmutablePayloadItems.Sort([](const FCodeBRunInventoryReceiptItem& Left, const FCodeBRunInventoryReceiptItem& Right)
		{
			return Left.ItemId.ToString(EGuidFormats::DigitsWithHyphensLower) < Right.ItemId.ToString(EGuidFormats::DigitsWithHyphensLower);
		});
		return true;
	}

	FGuid StableGuid(const FGuid& Owner, const uint32 Salt)
	{
		return FGuid(
			Owner.A ^ (0xCB500000u + Salt),
			Owner.B ^ (0xB00D0000u + Salt * 17u),
			Owner.C ^ (0x09000000u + Salt * 31u),
			Owner.D ^ (0x0F1E0000u + Salt * 47u));
	}

	FGuid SpatialChildGuid(const FGuid& ItemId)
	{
		return FGuid(ItemId.A ^ 0x5A0C0000u, ItemId.B ^ 0x0000CB05u, ItemId.C ^ 0xB00D0500u, ItemId.D ^ 0x50524F46u);
	}

	FGuid WorldDropGuid(const FGuid& OwnerId, const FGuid& RunInstanceId, const int32 Ordinal)
	{
		const FString Seed = FString::Printf(TEXT("P14.WorldDrop|%s|%s|%d"),
			*GuidText(OwnerId), *GuidText(RunInstanceId), Ordinal);
		return FGuid(
			FCrc::StrCrc32(*(Seed + TEXT("|A"))), FCrc::StrCrc32(*(Seed + TEXT("|B"))),
			FCrc::StrCrc32(*(Seed + TEXT("|C"))), FCrc::StrCrc32(*(Seed + TEXT("|D"))));
	}

	FGuid QuickUseReceiptGuid(const FGuid& OwnerId, const FGuid& RunInstanceId, const int32 Ordinal)
	{
		const FString Seed = FString::Printf(TEXT("P15.QuickUseReceipt|%s|%s|%d"),
			*GuidText(OwnerId), *GuidText(RunInstanceId), Ordinal);
		return FGuid(
			FCrc::StrCrc32(*(Seed + TEXT("|A"))), FCrc::StrCrc32(*(Seed + TEXT("|B"))),
			FCrc::StrCrc32(*(Seed + TEXT("|C"))), FCrc::StrCrc32(*(Seed + TEXT("|D"))));
	}

	const TCHAR* QuickUseReceiptStateText(const ECodeBQuickUseReceiptState State)
	{
		return State == ECodeBQuickUseReceiptState::Acknowledged ? TEXT("Acknowledged") : TEXT("Pending");
	}

	bool TryQuickUseReceiptState(const FString& Text, ECodeBQuickUseReceiptState& OutState)
	{
		if (Text == TEXT("Pending")) { OutState = ECodeBQuickUseReceiptState::Pending; return true; }
		if (Text == TEXT("Acknowledged")) { OutState = ECodeBQuickUseReceiptState::Acknowledged; return true; }
		return false;
	}

	FGuid WorldDropContainerGuid(const FGuid& WorldDropId)
	{
		const FString Seed = FString::Printf(TEXT("P14.WorldContainer|%s"), *GuidText(WorldDropId));
		return FGuid(
			FCrc::StrCrc32(*(Seed + TEXT("|A"))), FCrc::StrCrc32(*(Seed + TEXT("|B"))),
			FCrc::StrCrc32(*(Seed + TEXT("|C"))), FCrc::StrCrc32(*(Seed + TEXT("|D"))));
	}

	bool IsFiniteWorldDropTransform(const FTransform& Transform)
	{
		return !Transform.ContainsNaN()
			&& Transform.GetScale3D().Equals(FVector::OneVector, KINDA_SMALL_NUMBER);
	}

	TSharedRef<FJsonObject> WorldDropJson(const FCodeBWorldDropRecord& Record)
	{
		TSharedRef<FJsonObject> Object = MakeShared<FJsonObject>();
		Object->SetStringField(TEXT("OwnerId"), GuidText(Record.OwnerId));
		Object->SetStringField(TEXT("RunInstanceId"), GuidText(Record.RunInstanceId));
		Object->SetStringField(TEXT("WorldDropId"), GuidText(Record.WorldDropId));
		Object->SetNumberField(TEXT("Ordinal"), Record.Ordinal);
		Object->SetStringField(TEXT("WorldContainerId"), GuidText(Record.WorldContainerId));
		Object->SetStringField(TEXT("ItemId"), GuidText(Record.ItemId));
		Object->SetStringField(TEXT("SpatialChildContainerId"), GuidText(Record.SpatialChildContainerId));
		Object->SetStringField(TEXT("MapRoute"), Record.MapRoute.ToString());
		Object->SetNumberField(TEXT("LocationX"), Record.FloorTransform.GetLocation().X);
		Object->SetNumberField(TEXT("LocationY"), Record.FloorTransform.GetLocation().Y);
		Object->SetNumberField(TEXT("LocationZ"), Record.FloorTransform.GetLocation().Z);
		const FRotator Rotation = Record.FloorTransform.Rotator();
		Object->SetNumberField(TEXT("RotationPitch"), Rotation.Pitch);
		Object->SetNumberField(TEXT("RotationYaw"), Rotation.Yaw);
		Object->SetNumberField(TEXT("RotationRoll"), Rotation.Roll);
		Object->SetStringField(TEXT("ActionState"), TEXT("Available"));
		Object->SetNumberField(TEXT("RecordRevision"), Record.RecordRevision);
		Object->SetStringField(TEXT("Provenance"), Record.Provenance);
		return Object;
	}

	bool JsonToWorldDrop(
		const TSharedPtr<FJsonObject>& Object,
		const bool bRequireP31RegistryIdentity,
		FCodeBWorldDropRecord& OutRecord,
		FString& OutError)
	{
		OutRecord = FCodeBWorldDropRecord();
		FString WorldDropText, WorldContainerText, ItemText, Route, ActionState;
		double X = 0.0, Y = 0.0, Z = 0.0, Pitch = 0.0, Yaw = 0.0, Roll = 0.0;
		if (!Object.IsValid()
			|| !Object->TryGetStringField(TEXT("WorldDropId"), WorldDropText)
			|| !Object->TryGetStringField(TEXT("WorldContainerId"), WorldContainerText)
			|| !Object->TryGetStringField(TEXT("ItemId"), ItemText)
			|| !Object->TryGetStringField(TEXT("MapRoute"), Route)
			|| !Object->TryGetStringField(TEXT("ActionState"), ActionState)
			|| !Object->TryGetNumberField(TEXT("LocationX"), X)
			|| !Object->TryGetNumberField(TEXT("LocationY"), Y)
			|| !Object->TryGetNumberField(TEXT("LocationZ"), Z)
			|| !Object->TryGetNumberField(TEXT("RotationPitch"), Pitch)
			|| !Object->TryGetNumberField(TEXT("RotationYaw"), Yaw)
			|| !Object->TryGetNumberField(TEXT("RotationRoll"), Roll)
			|| !TryGuidText(WorldDropText, OutRecord.WorldDropId) || !OutRecord.WorldDropId.IsValid()
			|| !TryGuidText(WorldContainerText, OutRecord.WorldContainerId) || !OutRecord.WorldContainerId.IsValid()
			|| !TryGuidText(ItemText, OutRecord.ItemId) || !OutRecord.ItemId.IsValid()
			|| Route.IsEmpty() || ActionState != TEXT("Available")
			|| !FMath::IsFinite(X) || !FMath::IsFinite(Y) || !FMath::IsFinite(Z)
			|| !FMath::IsFinite(Pitch) || !FMath::IsFinite(Yaw) || !FMath::IsFinite(Roll))
		{
			OutError = TEXT("Code B P14 world-drop JSON is invalid.");
			return false;
		}
		OutRecord.MapRoute = FName(*Route);
		OutRecord.FloorTransform = FTransform(FRotator(Pitch, Yaw, Roll), FVector(X, Y, Z));
		OutRecord.ActionState = ECodeBWorldDropActionState::Available;
		if (bRequireP31RegistryIdentity)
		{
			FString OwnerText, RunText, ChildText, Provenance;
			double Ordinal = 0.0, RecordRevision = 0.0;
			if (!Object->TryGetStringField(TEXT("OwnerId"), OwnerText)
				|| !Object->TryGetStringField(TEXT("RunInstanceId"), RunText)
				|| !Object->TryGetStringField(TEXT("SpatialChildContainerId"), ChildText)
				|| !Object->TryGetStringField(TEXT("Provenance"), Provenance)
				|| !Object->TryGetNumberField(TEXT("Ordinal"), Ordinal)
				|| !Object->TryGetNumberField(TEXT("RecordRevision"), RecordRevision)
				|| !FMath::IsNearlyEqual(Ordinal, FMath::RoundToDouble(Ordinal))
				|| !FMath::IsNearlyEqual(RecordRevision, FMath::RoundToDouble(RecordRevision))
				|| Ordinal < 1.0 || Ordinal > MAX_int32
				|| RecordRevision < 1.0 || RecordRevision > MAX_int32
				|| !TryGuidText(OwnerText, OutRecord.OwnerId) || !OutRecord.OwnerId.IsValid()
				|| !TryGuidText(RunText, OutRecord.RunInstanceId) || !OutRecord.RunInstanceId.IsValid()
				|| !TryGuidText(ChildText, OutRecord.SpatialChildContainerId)
				|| Provenance.IsEmpty())
			{
				OutError = TEXT("Code B P31 WorldDrop Registry identity JSON is invalid.");
				return false;
			}
			OutRecord.Ordinal = static_cast<int32>(Ordinal);
			OutRecord.RecordRevision = static_cast<int32>(RecordRevision);
			OutRecord.Provenance = MoveTemp(Provenance);
		}
		return IsFiniteWorldDropTransform(OutRecord.FloorTransform);
	}

	bool BuildCanonicalCodeBItemDefinition(const FName DefinitionId, FCodeBItemDefinition& OutDefinition, FString& OutError);
	bool ValidateP19WorldDropClosure(
		const FCodeBSnapshot& Snapshot,
		const FCodeBItemInstance& Parent,
		bool& bOutIsSpatialClosure,
		FString& OutError);
	/** P20 admits the P17 closure only while the BasicCorpse child is still empty. */
	bool ValidateP20BasicCorpseSpatialClosure(
		const FCodeBSnapshot& Snapshot,
		const FCodeBItemInstance& Parent,
		FString& OutError)
	{
		bool bIsSpatialClosure = false;
		if (!ValidateP19WorldDropClosure(Snapshot, Parent, bIsSpatialClosure, OutError))
		{
			return false;
		}
		const FCodeBContainer* Child = Snapshot.Containers.Find(Parent.ChildContainerId);
		if (!bIsSpatialClosure || !Child
			|| Child->Slots.ContainsByPredicate([](const FGuid& ItemId) { return ItemId.IsValid(); }))
		{
			OutError = TEXT("Code B P20 BasicCorpse only admits one formal spatial parent with its canonical empty child container.");
			return false;
		}
		return true;
	}

	bool DiscardP19WorldDropClosure(
		FCodeBSnapshot& InOutSnapshot,
		const FCodeBWorldDropRecord& Record,
		FString& OutError);

	bool WorldDropRecordLess(const FCodeBWorldDropRecord& Left, const FCodeBWorldDropRecord& Right)
	{
		return Left.Ordinal != Right.Ordinal
			? Left.Ordinal < Right.Ordinal
			: GuidText(Left.WorldDropId) < GuidText(Right.WorldDropId);
	}

	void SortWorldDropRegistry(TArray<FCodeBWorldDropRecord>& WorldDrops)
	{
		WorldDrops.Sort([](const FCodeBWorldDropRecord& Left, const FCodeBWorldDropRecord& Right)
		{
			return WorldDropRecordLess(Left, Right);
		});
	}

	bool IsWorldDropClosureUnchanged(
		const FCodeBSnapshot& Before,
		const FCodeBSnapshot& After,
		const FCodeBWorldDropRecord& Record)
	{
		const FCodeBContainer* BeforeWorld = Before.Containers.Find(Record.WorldContainerId);
		const FCodeBContainer* AfterWorld = After.Containers.Find(Record.WorldContainerId);
		const FCodeBItemInstance* BeforeRoot = Before.Items.Find(Record.ItemId);
		const FCodeBItemInstance* AfterRoot = After.Items.Find(Record.ItemId);
		if (!BeforeWorld || !AfterWorld || *BeforeWorld != *AfterWorld
			|| !BeforeRoot || !AfterRoot || *BeforeRoot != *AfterRoot)
		{
			return false;
		}
		if (!Record.SpatialChildContainerId.IsValid()) return true;
		const FCodeBContainer* BeforeChild = Before.Containers.Find(Record.SpatialChildContainerId);
		const FCodeBContainer* AfterChild = After.Containers.Find(Record.SpatialChildContainerId);
		if (!BeforeChild || !AfterChild || *BeforeChild != *AfterChild) return false;
		for (const FGuid& ChildItemId : BeforeChild->Slots)
		{
			if (!ChildItemId.IsValid()) continue;
			const FCodeBItemInstance* BeforeItem = Before.Items.Find(ChildItemId);
			const FCodeBItemInstance* AfterItem = After.Items.Find(ChildItemId);
			if (!BeforeItem || !AfterItem || *BeforeItem != *AfterItem) return false;
		}
		return true;
	}

	bool UpgradeWorldDropRegistry(
		FCodeBRunInventorySession& Session,
		const int32 StoredSchemaVersion,
		FString& OutError)
	{
		if (StoredSchemaVersion < 6)
		{
			// P14's accepted durable shape was a singleton even though its JSON field
			// was array-shaped. P31 promotes that same record without replacing any ID.
			if (Session.WorldDrops.Num() > 1)
			{
				OutError = TEXT("Code B P31 legacy WorldDrop migration found more than one singleton record.");
				return false;
			}
			if (Session.WorldDrops.Num() == 1)
			{
				FCodeBWorldDropRecord& Record = Session.WorldDrops[0];
				const int32 LegacyOrdinal = Session.NextWorldDropOrdinal - 1;
				const FCodeBItemInstance* Root = Session.RepositorySnapshot.Items.Find(Record.ItemId);
				if (LegacyOrdinal < 1
					|| Record.WorldDropId != WorldDropGuid(Session.OwnerId, Session.RunInstanceId, LegacyOrdinal)
					|| !Root)
				{
					OutError = TEXT("Code B P31 cannot recover the exact legacy WorldDrop ordinal/root identity.");
					return false;
				}
				Record.OwnerId = Session.OwnerId;
				Record.RunInstanceId = Session.RunInstanceId;
				Record.Ordinal = LegacyOrdinal;
				Record.SpatialChildContainerId = Root->ChildContainerId;
				Record.RecordRevision = 1;
				Record.Provenance = TEXT("P14.LegacySingleRecord");
			}
		}
		SortWorldDropRegistry(Session.WorldDrops);
		return true;
	}

	bool ValidateWorldDrops(const FCodeBRunInventorySession& Session, FString& OutError)
	{
		if (Session.NextWorldDropOrdinal < 1)
		{
			OutError = TEXT("Code B P14 next world-drop ordinal is invalid.");
			return false;
		}
		TSet<FGuid> WorldDropIds;
		TSet<FGuid> WorldContainerIds;
		TSet<FGuid> WorldItemIds;
		TSet<int32> WorldDropOrdinals;
		for (int32 RecordIndex = 0; RecordIndex < Session.WorldDrops.Num(); ++RecordIndex)
		{
			const FCodeBWorldDropRecord& Record = Session.WorldDrops[RecordIndex];
			const FCodeBContainer* Container = Session.RepositorySnapshot.Containers.Find(Record.WorldContainerId);
			const FCodeBItemInstance* Item = Session.RepositorySnapshot.Items.Find(Record.ItemId);
			if (Record.OwnerId != Session.OwnerId || Record.RunInstanceId != Session.RunInstanceId
				|| !Record.WorldDropId.IsValid() || Record.Ordinal < 1 || Record.Ordinal >= Session.NextWorldDropOrdinal
				|| Record.WorldDropId != WorldDropGuid(Session.OwnerId, Session.RunInstanceId, Record.Ordinal)
				|| !Record.WorldContainerId.IsValid() || !Record.ItemId.IsValid()
				|| Record.WorldContainerId != WorldDropContainerGuid(Record.WorldDropId)
				|| Record.MapRoute.IsNone() || !IsFiniteWorldDropTransform(Record.FloorTransform)
				|| Record.ActionState != ECodeBWorldDropActionState::Available
				|| Record.RecordRevision < 1 || Record.Provenance.IsEmpty()
				|| WorldDropIds.Contains(Record.WorldDropId) || WorldContainerIds.Contains(Record.WorldContainerId)
				|| WorldDropOrdinals.Contains(Record.Ordinal)
				|| WorldItemIds.Contains(Record.ItemId) || !Container || !Item
				|| Container->ContainerType != FName(TEXT("WorldDrop")) || Container->Kind != ECodeBContainerKind::Storage
				|| Container->Slots.Num() != 1 || Container->Slots[0] != Record.ItemId
				|| Item->ParentContainerId != Record.WorldContainerId || Item->SlotIndex != 0
				|| Item->Quantity <= 0)
			{
				OutError = TEXT("Code B P31 WorldDrop Registry record does not match its exact P6/P1 single-root graph.");
				return false;
			}
			if (RecordIndex > 0 && WorldDropRecordLess(Record, Session.WorldDrops[RecordIndex - 1]))
			{
				OutError = TEXT("Code B P31 WorldDrop Registry is not in canonical ordinal/identity order.");
				return false;
			}
			bool bIsSpatialClosure = false;
			if (!ValidateP19WorldDropClosure(Session.RepositorySnapshot, *Item, bIsSpatialClosure, OutError))
			{
				return false;
			}
			if (Record.SpatialChildContainerId != (bIsSpatialClosure ? Item->ChildContainerId : FGuid()))
			{
				OutError = TEXT("Code B P31 WorldDrop record child-closure identity does not match its exact root.");
				return false;
			}
			WorldDropIds.Add(Record.WorldDropId);
			WorldContainerIds.Add(Record.WorldContainerId);
			WorldItemIds.Add(Record.ItemId);
			WorldDropOrdinals.Add(Record.Ordinal);
		}
		for (const FCodeBHotbarBinding& Binding : Session.HotbarBindings.Slots)
		{
			if (Binding.bHasReference && WorldItemIds.Contains(Binding.ItemId))
			{
				OutError = TEXT("Code B P14 ground items cannot retain a P13 hotbar binding.");
				return false;
			}
		}
		return true;
	}

	FCodeBWorldDropProjection WorldDropProjection(const FCodeBRunInventorySession& Session, const FCodeBWorldDropRecord& Record)
	{
		FCodeBWorldDropProjection Result;
		Result.OwnerId = Session.OwnerId;
		Result.RunInstanceId = Session.RunInstanceId;
		Result.WorldDropId = Record.WorldDropId;
		Result.Ordinal = Record.Ordinal;
		Result.WorldContainerId = Record.WorldContainerId;
		Result.ItemId = Record.ItemId;
		Result.SpatialChildContainerId = Record.SpatialChildContainerId;
		if (const FCodeBItemInstance* Item = Session.RepositorySnapshot.Items.Find(Record.ItemId))
		{
			Result.DefinitionId = Item->DefinitionId;
			Result.Quantity = Item->Quantity;
		}
		Result.MapRoute = Record.MapRoute;
		Result.FloorTransform = Record.FloorTransform;
		Result.RecordRevision = Record.RecordRevision;
		Result.P6SnapshotRevision = Session.RepositorySnapshot.Revision;
		return Result;
	}

	bool IsP26PlayerStorageContainer(
		const FCodeBRunInventorySession& Session,
		const FGuid& ContainerId)
	{
		const FCodeBContainer* Container = Session.RepositorySnapshot.Containers.Find(ContainerId);
		if (!Container || Container->IsEquipment()) return false;
		if (ContainerId == Session.Layout.BasicContainerId) return true;

		const auto IsEquippedChild = [&Session, &ContainerId](const FGuid& EquipmentContainerId)
		{
			const FCodeBContainer* Equipment = Session.RepositorySnapshot.Containers.Find(EquipmentContainerId);
			if (!Equipment || !Equipment->IsEquipment() || Equipment->Slots.Num() != 1
				|| !Equipment->Slots[0].IsValid())
			{
				return false;
			}
			const FCodeBItemInstance* Parent = Session.RepositorySnapshot.Items.Find(Equipment->Slots[0]);
			return Parent && Parent->ChildContainerId == ContainerId;
		};
		return IsEquippedChild(Session.Layout.SpatialContainerId)
			|| IsEquippedChild(Session.Layout.BackpackContainerId);
	}

	bool IsP26SimpleStack(
		const FCodeBRunInventorySession& Session,
		const FCodeBItemInstance& Item)
	{
		const FCodeBItemDefinition* Definition = Session.RepositorySnapshot.Definitions.Find(Item.DefinitionId);
		return Definition && Definition->bStackable && Definition->MaxStack > 1
			&& Item.Quantity > 0 && Item.Quantity <= Definition->MaxStack
			&& !Item.ChildContainerId.IsValid();
	}

	/** P32's closed standard-equipment slice: canonical P1 definitions only, never spatial or stack roots. */
	bool IsP32StandardEquipmentRoot(
		const FCodeBRunInventorySession& Session,
		const FCodeBItemInstance& Item)
	{
		const FCodeBItemDefinition* Definition = Session.RepositorySnapshot.Definitions.Find(Item.DefinitionId);
		FCodeBItemDefinition CanonicalDefinition;
		FString Error;
		return Definition
			&& BuildCanonicalCodeBItemDefinition(Item.DefinitionId, CanonicalDefinition, Error)
			&& *Definition == CanonicalDefinition
			&& !Definition->bStackable && Definition->MaxStack == 1
			&& Definition->SpatialContainerSemantic == ECodeBSpatialContainerSemantic::None
			&& Definition->ChildContainerCapacity == 0
			&& (Definition->ItemType == ECodeBItemType::Weapon
				|| Definition->ItemType == ECodeBItemType::Armor
				|| Definition->ItemType == ECodeBItemType::Accessory)
			&& (Definition->EquipSlot == ECodeBEquipSlot::Weapon
				|| Definition->EquipSlot == ECodeBEquipSlot::Armor
				|| Definition->EquipSlot == ECodeBEquipSlot::Accessory)
			&& Item.Quantity == 1 && !Item.ChildContainerId.IsValid();
	}

	bool IsP32ActiveStandardEquipmentContainer(
		const FCodeBRunInventorySession& Session,
		const FGuid& ContainerId,
		const ECodeBEquipSlot EquipSlot)
	{
		const bool bExactLayoutRoot = (EquipSlot == ECodeBEquipSlot::Weapon
				&& ContainerId == Session.Layout.WeaponContainerId)
			|| (EquipSlot == ECodeBEquipSlot::Armor
				&& ContainerId == Session.Layout.ArmorContainerId)
			|| (EquipSlot == ECodeBEquipSlot::Accessory
				&& Session.Layout.AccessoryContainerIds.Contains(ContainerId));
		const FCodeBContainer* Container = Session.RepositorySnapshot.Containers.Find(ContainerId);
		return bExactLayoutRoot && Container && Container->IsEquipment()
			&& Container->EquipmentSlot == EquipSlot && Container->Slots.Num() == 1;
	}

	bool IsP32EquippedStandardRoot(
		const FCodeBRunInventorySession& Session,
		const FCodeBItemInstance& Item,
		const FGuid& ContainerId)
	{
		const FCodeBItemDefinition* Definition = Session.RepositorySnapshot.Definitions.Find(Item.DefinitionId);
		const FCodeBContainer* Container = Session.RepositorySnapshot.Containers.Find(ContainerId);
		return Definition && IsP32StandardEquipmentRoot(Session, Item)
			&& IsP32ActiveStandardEquipmentContainer(Session, ContainerId, Definition->EquipSlot)
			&& Container && Container->Slots[0] == Item.ItemId
			&& Item.ParentContainerId == ContainerId && Item.SlotIndex == 0;
	}

	/**
	 * P29 proves the already accepted shared Ctrl+left P1 command without replaying
	 * it. Only the currently opened world root and BaseQuick/current active P17
	 * child may participate, and the candidate must be the exact one-revision
	 * Move/Merge(0) delta.
	 */
	bool IsExactP29WorldDropQuickTransferDelta(
		const FCodeBRunInventorySession& Session,
		const FCodeBWorldDropRecord& Drop,
		const FCodeBP2Command& AcceptedCommand,
		const FCodeBSnapshot& Candidate,
		bool& bOutRetainWorldRoot,
		FString& OutError)
	{
		bOutRetainWorldRoot = false;
		OutError.Reset();
		const FCodeBSnapshot& Prior = Session.RepositorySnapshot;
		const bool bWorldSource = AcceptedCommand.SourceContainerId == Drop.WorldContainerId;
		const bool bWorldTarget = AcceptedCommand.TargetContainerId == Drop.WorldContainerId;
		if (AcceptedCommand.Intent != ECodeBP2CommandIntent::QuickTransfer
			|| !AcceptedCommand.TransactionId.IsValid()
			|| AcceptedCommand.Quantity != 0
			|| AcceptedCommand.ExpectedRevision != Prior.Revision
			|| Prior.Revision == MAX_int32
			|| bWorldSource == bWorldTarget)
		{
			OutError = TEXT("P29 requires one exact P1 Ctrl QuickTransfer command with one world side and Quantity=0.");
			return false;
		}

		const FCodeBContainer* WorldContainer = Prior.Containers.Find(Drop.WorldContainerId);
		const FCodeBItemInstance* WorldRoot = Prior.Items.Find(Drop.ItemId);
		if (!WorldContainer || WorldContainer->IsEquipment() || WorldContainer->Slots.Num() != 1
			|| WorldContainer->Slots[0] != Drop.ItemId || !WorldRoot
			|| WorldRoot->ParentContainerId != Drop.WorldContainerId || WorldRoot->SlotIndex != 0
			|| !IsP26SimpleStack(Session, *WorldRoot))
		{
			OutError = TEXT("P29 opened WorldDrop no longer contains its exact simple-stack root.");
			return false;
		}

		auto IsExactPlayerContainer = [&Session, &AcceptedCommand](const FGuid& ContainerId, const bool bWorldToPlayer)
		{
			if (ContainerId == Session.Layout.BasicContainerId)
			{
				return !bWorldToPlayer || !AcceptedCommand.QuickTransferActivePlayerContainerId.IsValid();
			}
			return AcceptedCommand.QuickTransferActivePlayerContainerId == ContainerId
				&& IsP26PlayerStorageContainer(Session, ContainerId);
		};

		const FGuid PlayerContainerId = bWorldSource
			? AcceptedCommand.TargetContainerId : AcceptedCommand.SourceContainerId;
		const int32 PlayerSlot = bWorldSource ? AcceptedCommand.TargetSlot : AcceptedCommand.SourceSlot;
		const FCodeBContainer* PlayerContainer = Prior.Containers.Find(PlayerContainerId);
		if (!IsExactPlayerContainer(PlayerContainerId, bWorldSource)
			|| !PlayerContainer || PlayerContainer->IsEquipment()
			|| !PlayerContainer->Slots.IsValidIndex(PlayerSlot))
		{
			OutError = TEXT("P29 player side is not BaseQuick or the exact current active P17 child.");
			return false;
		}

		const FCodeBItemInstance* Source = Prior.Items.Find(AcceptedCommand.ItemId);
		const FCodeBContainer* SourceContainer = Prior.Containers.Find(AcceptedCommand.SourceContainerId);
		const FCodeBContainer* TargetContainer = Prior.Containers.Find(AcceptedCommand.TargetContainerId);
		const FGuid TargetItemId = TargetContainer
			&& TargetContainer->Slots.IsValidIndex(AcceptedCommand.TargetSlot)
			? TargetContainer->Slots[AcceptedCommand.TargetSlot] : FGuid();
		const FCodeBItemInstance* Target = Prior.Items.Find(TargetItemId);
		if (!Source || !SourceContainer || !TargetContainer
			|| SourceContainer->IsEquipment() || TargetContainer->IsEquipment()
			|| Source->ParentContainerId != AcceptedCommand.SourceContainerId
			|| Source->SlotIndex != AcceptedCommand.SourceSlot
			|| !SourceContainer->Slots.IsValidIndex(Source->SlotIndex)
			|| SourceContainer->Slots[Source->SlotIndex] != Source->ItemId
			|| !IsP26SimpleStack(Session, *Source)
			|| (bWorldSource && Source->ItemId != Drop.ItemId)
			|| (bWorldTarget && (AcceptedCommand.TargetSlot != 0 || TargetItemId != Drop.ItemId)))
		{
			OutError = TEXT("P29 command source or exact world-root address is invalid.");
			return false;
		}

		FCodeBSnapshot Expected = Prior;
		Expected.Revision = Prior.Revision + 1;
		FCodeBContainer* ExpectedSourceContainer = Expected.Containers.Find(AcceptedCommand.SourceContainerId);
		FCodeBContainer* ExpectedTargetContainer = Expected.Containers.Find(AcceptedCommand.TargetContainerId);
		FCodeBItemInstance* ExpectedSource = Expected.Items.Find(Source->ItemId);
		if (!ExpectedSourceContainer || !ExpectedTargetContainer || !ExpectedSource)
		{
			OutError = TEXT("P29 could not construct the exact candidate proof.");
			return false;
		}

		if (AcceptedCommand.Operation == ECodeBOperation::Move)
		{
			if (!bWorldSource || TargetItemId.IsValid())
			{
				OutError = TEXT("P29 Move is allowed only from the world root into one explicit empty player cell.");
				return false;
			}
			ExpectedSourceContainer->Slots[AcceptedCommand.SourceSlot].Invalidate();
			ExpectedTargetContainer->Slots[AcceptedCommand.TargetSlot] = Source->ItemId;
			ExpectedSource->ParentContainerId = AcceptedCommand.TargetContainerId;
			ExpectedSource->SlotIndex = AcceptedCommand.TargetSlot;
			bOutRetainWorldRoot = false;
		}
		else if (AcceptedCommand.Operation == ECodeBOperation::Merge)
		{
			const FCodeBItemDefinition* Definition = Prior.Definitions.Find(Source->DefinitionId);
			if (!Target || Target->ItemId == Source->ItemId || !Definition
				|| !Definition->bStackable || Definition->MaxStack <= 1
				|| Target->DefinitionId != Source->DefinitionId
				|| Target->ChildContainerId.IsValid()
				|| Target->Quantity <= 0 || Target->Quantity >= Definition->MaxStack)
			{
				OutError = TEXT("P29 Merge(0) requires one compatible occupied underfull world/player target stack.");
				return false;
			}
			const int32 AcceptedQuantity = FMath::Min(Source->Quantity, Definition->MaxStack - Target->Quantity);
			if (AcceptedQuantity <= 0)
			{
				OutError = TEXT("P29 Merge(0) has no positive compatible capacity.");
				return false;
			}
			FCodeBItemInstance* ExpectedTarget = Expected.Items.Find(Target->ItemId);
			if (!ExpectedTarget)
			{
				OutError = TEXT("P29 merge target disappeared while constructing the proof.");
				return false;
			}
			ExpectedTarget->Quantity += AcceptedQuantity;
			ExpectedSource->Quantity -= AcceptedQuantity;
			if (ExpectedSource->Quantity == 0)
			{
				ExpectedSourceContainer->Slots[AcceptedCommand.SourceSlot].Invalidate();
				Expected.Items.Remove(Source->ItemId);
			}
			bOutRetainWorldRoot = bWorldTarget || Expected.Items.Contains(Drop.ItemId);
		}
		else
		{
			OutError = TEXT("P29 accepts only P1 Move or Merge(0).");
			return false;
		}

		if (Candidate != Expected)
		{
			OutError = TEXT("P29 candidate differs from the exact one-command P1 Ctrl quick-transfer delta.");
			return false;
		}
		return true;
	}

	/**
	 * P30 proves the already accepted P1 Move of one formal P19 closure from the
	 * exact opened WorldDrop root to the first empty BaseQuick cell. The proof is
	 * structural: parent placement is the only allowed delta, so child-container
	 * identity, contents, ordering, capacity, provenance, and every unrelated
	 * graph value remain byte-for-byte represented by snapshot equality.
	 */
	bool IsExactP30WorldDropCompleteGraphQuickTransferDelta(
		const FCodeBRunInventorySession& Session,
		const FCodeBWorldDropRecord& Drop,
		const FCodeBP2Command& AcceptedCommand,
		const FCodeBSnapshot& Candidate,
		FString& OutError)
	{
		OutError.Reset();
		const FCodeBSnapshot& Prior = Session.RepositorySnapshot;
		if (AcceptedCommand.Intent != ECodeBP2CommandIntent::QuickTransfer
			|| AcceptedCommand.Operation != ECodeBOperation::Move
			|| !AcceptedCommand.TransactionId.IsValid()
			|| AcceptedCommand.ItemId != Drop.ItemId
			|| AcceptedCommand.SourceContainerId != Drop.WorldContainerId
			|| AcceptedCommand.SourceSlot != 0
			|| AcceptedCommand.TargetContainerId != Session.Layout.BasicContainerId
			|| AcceptedCommand.TargetSlot < 0
			|| AcceptedCommand.Quantity != 0
			|| AcceptedCommand.ExpectedRevision != Prior.Revision
			|| AcceptedCommand.QuickTransferActivePlayerContainerId.IsValid()
			|| Prior.Revision == MAX_int32
			|| Drop.ActionState != ECodeBWorldDropActionState::Available)
		{
			OutError = TEXT("P30 requires one exact Ctrl QuickTransfer Move from the opened world root to BaseQuick.");
			return false;
		}

		const FCodeBContainer* WorldContainer = Prior.Containers.Find(Drop.WorldContainerId);
		const FCodeBContainer* BaseQuick = Prior.Containers.Find(Session.Layout.BasicContainerId);
		const FCodeBItemInstance* Root = Prior.Items.Find(Drop.ItemId);
		bool bIsSpatialClosure = false;
		if (!WorldContainer || WorldContainer->IsEquipment() || WorldContainer->Slots.Num() != 1
			|| WorldContainer->Slots[0] != Drop.ItemId
			|| !BaseQuick || BaseQuick->IsEquipment()
			|| !BaseQuick->Slots.IsValidIndex(AcceptedCommand.TargetSlot)
			|| BaseQuick->Slots[AcceptedCommand.TargetSlot].IsValid()
			|| !Root || Root->ParentContainerId != Drop.WorldContainerId || Root->SlotIndex != 0
			|| !ValidateP19WorldDropClosure(Prior, *Root, bIsSpatialClosure, OutError)
			|| !bIsSpatialClosure)
		{
			if (OutError.IsEmpty())
			{
				OutError = TEXT("P30 opened source is not one valid formal P19 complete graph, or BaseQuick target is invalid.");
			}
			return false;
		}
		for (int32 SlotIndex = 0; SlotIndex < AcceptedCommand.TargetSlot; ++SlotIndex)
		{
			if (!BaseQuick->Slots[SlotIndex].IsValid())
			{
				OutError = TEXT("P30 target is not the first empty BaseQuick cell in SlotIndex order.");
				return false;
			}
		}

		FCodeBSnapshot Expected = Prior;
		Expected.Revision = Prior.Revision + 1;
		FCodeBContainer* ExpectedWorld = Expected.Containers.Find(Drop.WorldContainerId);
		FCodeBContainer* ExpectedBaseQuick = Expected.Containers.Find(Session.Layout.BasicContainerId);
		FCodeBItemInstance* ExpectedRoot = Expected.Items.Find(Drop.ItemId);
		if (!ExpectedWorld || !ExpectedBaseQuick || !ExpectedRoot)
		{
			OutError = TEXT("P30 could not construct the accepted whole-graph candidate proof.");
			return false;
		}
		ExpectedWorld->Slots[0].Invalidate();
		ExpectedBaseQuick->Slots[AcceptedCommand.TargetSlot] = Drop.ItemId;
		ExpectedRoot->ParentContainerId = Session.Layout.BasicContainerId;
		ExpectedRoot->SlotIndex = AcceptedCommand.TargetSlot;
		if (Candidate != Expected)
		{
			OutError = TEXT("P30 candidate differs from the exact one-command whole-graph Move delta.");
			return false;
		}
		const FCodeBItemInstance* CandidateRoot = Candidate.Items.Find(Drop.ItemId);
		bool bCandidateSpatialClosure = false;
		if (!CandidateRoot || !ValidateP19WorldDropClosure(
			Candidate, *CandidateRoot, bCandidateSpatialClosure, OutError) || !bCandidateSpatialClosure)
		{
			if (OutError.IsEmpty()) OutError = TEXT("P30 accepted candidate no longer contains the exact formal P19 closure.");
			return false;
		}
		return true;
	}

	/**
	 * P34 proves one accepted non-stack standard-equipment Move from the exact
	 * opened P32/P33 world record to the first empty stable BaseQuick slot. The
	 * explicit Quantity=1 distinguishes this whole-root family from P29/P30's
	 * Quantity=0 stack/graph command semantics; P1 Move itself remains unchanged.
	 */
	bool IsExactP34WorldDropStandardEquipmentQuickTransferDelta(
		const FCodeBRunInventorySession& Session,
		const FCodeBWorldDropRecord& Drop,
		const FCodeBP2Command& AcceptedCommand,
		const FCodeBSnapshot& Candidate,
		FString& OutError)
	{
		OutError.Reset();
		const FCodeBSnapshot& Prior = Session.RepositorySnapshot;
		const bool bAcceptedProvenance =
			Drop.Provenance == TEXT("P32.AcceptedGroundDrop.StandardEquipment")
			|| Drop.Provenance == TEXT("P33.AcceptedGroundDrop.BaseQuickStandardEquipment");
		if (AcceptedCommand.Intent != ECodeBP2CommandIntent::QuickTransfer
			|| AcceptedCommand.Operation != ECodeBOperation::Move
			|| !AcceptedCommand.TransactionId.IsValid()
			|| AcceptedCommand.ItemId != Drop.ItemId
			|| AcceptedCommand.SourceContainerId != Drop.WorldContainerId
			|| AcceptedCommand.SourceSlot != 0
			|| AcceptedCommand.TargetContainerId != Session.Layout.BasicContainerId
			|| AcceptedCommand.TargetSlot < 0
			|| AcceptedCommand.Quantity != 1
			|| AcceptedCommand.ExpectedRevision != Prior.Revision
			|| AcceptedCommand.QuickTransferActivePlayerContainerId.IsValid()
			|| Prior.Revision == MAX_int32
			|| Drop.ActionState != ECodeBWorldDropActionState::Available
			|| !bAcceptedProvenance)
		{
			OutError = TEXT("P34 requires one exact Move(1) from an accepted P32/P33 world root to BaseQuick.");
			return false;
		}

		const FCodeBContainer* WorldContainer = Prior.Containers.Find(Drop.WorldContainerId);
		const FCodeBContainer* BaseQuick = Prior.Containers.Find(Session.Layout.BasicContainerId);
		const FCodeBItemInstance* Root = Prior.Items.Find(Drop.ItemId);
		if (!WorldContainer || WorldContainer->IsEquipment() || WorldContainer->Slots.Num() != 1
			|| WorldContainer->Slots[0] != Drop.ItemId
			|| Drop.SpatialChildContainerId.IsValid()
			|| !BaseQuick || BaseQuick->IsEquipment() || BaseQuick->Slots.Num() <= 0
			|| !BaseQuick->Slots.IsValidIndex(AcceptedCommand.TargetSlot)
			|| BaseQuick->Slots[AcceptedCommand.TargetSlot].IsValid()
			|| !Root || Root->ParentContainerId != Drop.WorldContainerId || Root->SlotIndex != 0
			|| !IsP32StandardEquipmentRoot(Session, *Root))
		{
			OutError = TEXT("P34 opened source is not one canonical non-spatial standard root, or BaseQuick target is invalid.");
			return false;
		}
		for (int32 SlotIndex = 0; SlotIndex < AcceptedCommand.TargetSlot; ++SlotIndex)
		{
			if (!BaseQuick->Slots[SlotIndex].IsValid())
			{
				OutError = TEXT("P34 target is not the first empty BaseQuick cell in stable SlotIndex order.");
				return false;
			}
		}

		FCodeBSnapshot Expected = Prior;
		Expected.Revision = Prior.Revision + 1;
		FCodeBContainer* ExpectedWorld = Expected.Containers.Find(Drop.WorldContainerId);
		FCodeBContainer* ExpectedBaseQuick = Expected.Containers.Find(Session.Layout.BasicContainerId);
		FCodeBItemInstance* ExpectedRoot = Expected.Items.Find(Drop.ItemId);
		if (!ExpectedWorld || !ExpectedBaseQuick || !ExpectedRoot)
		{
			OutError = TEXT("P34 could not construct the accepted whole-root candidate proof.");
			return false;
		}
		ExpectedWorld->Slots[0].Invalidate();
		ExpectedBaseQuick->Slots[AcceptedCommand.TargetSlot] = Drop.ItemId;
		ExpectedRoot->ParentContainerId = Session.Layout.BasicContainerId;
		ExpectedRoot->SlotIndex = AcceptedCommand.TargetSlot;
		if (Candidate != Expected)
		{
			OutError = TEXT("P34 candidate differs from the exact one-command standard-equipment Move delta.");
			return false;
		}
		const FCodeBItemInstance* CandidateRoot = Candidate.Items.Find(Drop.ItemId);
		return CandidateRoot && IsP32StandardEquipmentRoot(Session, *CandidateRoot);
	}

	FString LegacyAffixDigest(const Fdemo_mapPersistentItemRecord& Item)
	{
		if (Item.AffixSet.IsEmpty())
		{
			return FString();
		}
		TArray<FString> Pieces;
		Pieces.Add(GuidText(Item.AffixSet.AffixSetEventId));
		Pieces.Add(Item.AffixSet.AffixPolicyId.ToString());
		Pieces.Add(FString::FromInt(static_cast<int32>(Item.AffixSet.Acquisition)));
		for (const Fdemo_mapResolvedRewardAffix& Affix : Item.AffixSet.Affixes)
		{
			Pieces.Add(FString::Printf(TEXT("%s:%d:%d:%lld"), *Affix.AffixId.ToString(), static_cast<int32>(Affix.Tier), Affix.ResolvedMagnitudeScaled, Affix.ResolvedValue));
		}
		Pieces.Sort();
		return FString::Join(Pieces, TEXT("|"));
	}

	FString SourceFingerprint(const Fdemo_mapProfileSessionSnapshot& Snapshot)
	{
		TArray<FString> Pieces;
		Pieces.Add(GuidText(Snapshot.ProfileId));
		Pieces.Add(FString::FromInt(Snapshot.SaveGeneration));
		for (const Fdemo_mapPersistentItemRecord& Item : Snapshot.OrderedPermanentStash)
		{
			Pieces.Add(FString::Printf(TEXT("%s:%s:%d:%d:%s:%s"),
				*GuidText(Item.ItemInstanceId), *Item.ItemDefinitionId.ToString(), Item.StackCount,
				static_cast<int32>(Item.PersistentDomain), *GuidText(Item.LegacySpatialParentItemInstanceId),
				*LegacyAffixDigest(Item)));
		}
		Pieces.Sort();
		return FString::Join(Pieces, TEXT("#"));
	}

	ECodeBItemType CodeBType(const Fdemo_mapItemDefinition& Definition)
	{
		// The legacy definition's category is the durable product semantic.  In
		// particular, old spatial rings are named "Accessory" in a few places,
		// but have their own category and equipment slot.  Do not infer a normal
		// accessory from incidental display names or vice versa.
		if (Definition.CategoryId == Fdemo_mapItemIds::WeaponCategory) return ECodeBItemType::Weapon;
		if (Definition.CategoryId == Fdemo_mapItemIds::ArmorCategory) return ECodeBItemType::Armor;
		if (Definition.CategoryId == Fdemo_mapItemIds::AccessoryCategory) return ECodeBItemType::Accessory;
		if (Definition.CategoryId == Fdemo_mapItemIds::SpatialRingCategory) return ECodeBItemType::SpatialItem;
		if (Definition.CategoryId == Fdemo_mapItemIds::BackpackCategory) return ECodeBItemType::Backpack;
		if (Definition.CategoryId == Fdemo_mapItemIds::MaterialCategory) return ECodeBItemType::Material;
		if (Definition.CategoryId == Fdemo_mapItemIds::ConsumableCategory) return ECodeBItemType::Consumable;

		// Keep a narrow compatibility fallback for older definitions that carried
		// only an equipment slot.  The formal definitions above always take the
		// category path, so this cannot reclassify a recognized live item.
		if (Definition.EquipmentSlotId == Fdemo_mapItemIds::WeaponSlot) return ECodeBItemType::Weapon;
		if (Definition.EquipmentSlotId == Fdemo_mapItemIds::ArmorSlot) return ECodeBItemType::Armor;
		if (Definition.EquipmentSlotId == Fdemo_mapItemIds::AccessorySlot) return ECodeBItemType::Accessory;
		if (Definition.EquipmentSlotId == Fdemo_mapItemIds::SpatialRingSlot) return ECodeBItemType::SpatialItem;
		if (Definition.EquipmentSlotId == Fdemo_mapItemIds::BackpackSlot) return ECodeBItemType::Backpack;
		return ECodeBItemType::Generic;
	}

	ECodeBEquipSlot CodeBEquipSlot(const Fdemo_mapItemDefinition& Definition)
	{
		if (Definition.CategoryId == Fdemo_mapItemIds::WeaponCategory) return ECodeBEquipSlot::Weapon;
		if (Definition.CategoryId == Fdemo_mapItemIds::ArmorCategory) return ECodeBEquipSlot::Armor;
		if (Definition.CategoryId == Fdemo_mapItemIds::AccessoryCategory) return ECodeBEquipSlot::Accessory;
		if (Definition.CategoryId == Fdemo_mapItemIds::SpatialRingCategory) return ECodeBEquipSlot::SpatialItem;
		if (Definition.CategoryId == Fdemo_mapItemIds::BackpackCategory) return ECodeBEquipSlot::Backpack;

		if (Definition.EquipmentSlotId == Fdemo_mapItemIds::WeaponSlot) return ECodeBEquipSlot::Weapon;
		if (Definition.EquipmentSlotId == Fdemo_mapItemIds::ArmorSlot) return ECodeBEquipSlot::Armor;
		if (Definition.EquipmentSlotId == Fdemo_mapItemIds::AccessorySlot) return ECodeBEquipSlot::Accessory;
		if (Definition.EquipmentSlotId == Fdemo_mapItemIds::SpatialRingSlot) return ECodeBEquipSlot::SpatialItem;
		if (Definition.EquipmentSlotId == Fdemo_mapItemIds::BackpackSlot) return ECodeBEquipSlot::Backpack;
		return ECodeBEquipSlot::None;
	}

	const TCHAR* NormalContainerStateText(const ECodeBNormalContainerState State)
	{
		switch (State)
		{
		case ECodeBNormalContainerState::Closed: return TEXT("Closed");
		case ECodeBNormalContainerState::Opening: return TEXT("Opening");
		case ECodeBNormalContainerState::Open: return TEXT("Open");
		case ECodeBNormalContainerState::Interrupted: return TEXT("Interrupted");
		default: return TEXT("Invalid");
		}
	}

	bool TryNormalContainerState(const FString& Text, ECodeBNormalContainerState& OutState)
	{
		OutState = ECodeBNormalContainerState::Closed;
		if (Text == TEXT("Closed")) OutState = ECodeBNormalContainerState::Closed;
		else if (Text == TEXT("Opening")) OutState = ECodeBNormalContainerState::Opening;
		else if (Text == TEXT("Open")) OutState = ECodeBNormalContainerState::Open;
		else if (Text == TEXT("Interrupted")) OutState = ECodeBNormalContainerState::Interrupted;
		else return false;
		return true;
	}

	const TCHAR* NormalContainerRevealStateText(const ECodeBNormalContainerRevealState State)
	{
		switch (State)
		{
		case ECodeBNormalContainerRevealState::Hidden: return TEXT("Hidden");
		case ECodeBNormalContainerRevealState::Searching: return TEXT("Searching");
		case ECodeBNormalContainerRevealState::Revealed: return TEXT("Revealed");
		default: return TEXT("Invalid");
		}
	}

	bool TryNormalContainerRevealState(const FString& Text, ECodeBNormalContainerRevealState& OutState)
	{
		OutState = ECodeBNormalContainerRevealState::Hidden;
		if (Text == TEXT("Hidden")) OutState = ECodeBNormalContainerRevealState::Hidden;
		else if (Text == TEXT("Searching")) OutState = ECodeBNormalContainerRevealState::Searching;
		else if (Text == TEXT("Revealed")) OutState = ECodeBNormalContainerRevealState::Revealed;
		else return false;
		return true;
	}

	const TArray<FCodeBNormalContainerDefinition>& NormalContainerDefinitions()
	{
		static const TArray<FCodeBNormalContainerDefinition> Definitions = []()
		{
			FCodeBNormalContainerDefinition BasicCache;
			BasicCache.DefinitionId = FName(TEXT("CodeB.NormalContainer.BasicCache"));
			BasicCache.ContainerType = FName(TEXT("CodeB.NormalContainer.BasicCache"));
			BasicCache.Capacity = 4;
			BasicCache.ContentRevision = 1;
			BasicCache.ContentPlan = {
				{ Fdemo_mapItemIds::SpiritDust, 3, 0, 0 },
				{ Fdemo_mapItemIds::IronShard, 2, 2, 0 }
			};
			return TArray<FCodeBNormalContainerDefinition> { MoveTemp(BasicCache) };
		}();
		return Definitions;
	}

	const FCodeBNormalContainerDefinition* FindNormalContainerDefinitionInternal(const FName DefinitionId)
	{
		return NormalContainerDefinitions().FindByPredicate([DefinitionId](const FCodeBNormalContainerDefinition& Definition)
		{
			return Definition.DefinitionId == DefinitionId;
		});
	}

	bool BuildCanonicalCodeBItemDefinition(const FName DefinitionId, FCodeBItemDefinition& OutDefinition, FString& OutError)
	{
		const Fdemo_mapItemDefinition* Source = Fdemo_mapItemDefinitions::Find(DefinitionId);
		if (!Source)
		{
			OutError = FString::Printf(TEXT("Code B normal-container content references unknown canonical item definition %s."), *DefinitionId.ToString());
			return false;
		}
		OutDefinition = FCodeBItemDefinition();
		OutDefinition.DefinitionId = DefinitionId;
		OutDefinition.ItemType = CodeBType(*Source);
		OutDefinition.bStackable = Source->MaxStackSize > 1;
		OutDefinition.bQuickUsable = Source->CategoryId == Fdemo_mapItemIds::ConsumableCategory;
		for (const Fdemo_mapItemEffectParameter& Effect : Source->EffectParameters)
		{
			if (OutDefinition.bQuickUsable && Effect.ParameterId == Fdemo_mapItemEffectIds::HealAmount
				&& FMath::IsFinite(Effect.Value) && Effect.Value > 0.0
				&& Effect.Value <= static_cast<double>(MAX_int32)
				&& FMath::IsNearlyEqual(Effect.Value, FMath::RoundToDouble(Effect.Value)))
			{
				OutDefinition.QuickUseEffect = ECodeBQuickUseEffectKind::RestoreHealth;
				OutDefinition.QuickUseRestoreAmount = static_cast<int32>(Effect.Value);
				break;
			}
		}
		OutDefinition.MaxStack = FMath::Max(1, Source->MaxStackSize);
		OutDefinition.EquipSlot = CodeBEquipSlot(*Source);
		OutDefinition.GridWidth = 1;
		OutDefinition.GridHeight = 1;
		if (Source->CategoryId == Fdemo_mapItemIds::SpatialRingCategory)
		{
			const Fdemo_mapSpatialRingCapacityResult Ring =
				Fdemo_mapItemDefinitions::ResolveSpatialRingCapacity(DefinitionId);
			if (!Ring.bSuccess)
			{
				OutError = Ring.Diagnostic;
				return false;
			}
			if (Ring.Capacity > 0)
			{
				OutDefinition.SpatialContainerSemantic = ECodeBSpatialContainerSemantic::QuickRing;
				OutDefinition.ChildContainerCapacity = Ring.Capacity;
			}
		}
		else if (Source->CategoryId == Fdemo_mapItemIds::BackpackCategory)
		{
			const Fdemo_mapSpatialStorageCapacityResult Pouch =
				Fdemo_mapItemDefinitions::ResolveSpatialStorageCapacity(DefinitionId);
			if (!Pouch.bSuccess || Pouch.Capacity <= 0)
			{
				OutError = Pouch.Diagnostic.IsEmpty()
					? TEXT("Canonical spatial storage pouch has no positive capacity.")
					: Pouch.Diagnostic;
				return false;
			}
			OutDefinition.SpatialContainerSemantic = ECodeBSpatialContainerSemantic::StoragePouch;
			OutDefinition.ChildContainerCapacity = Pouch.Capacity;
		}
		return true;
	}

	/** P21's closed Code B catalog slice. These are formal definitions, not Code A inventory instances or Loot. */
	struct FP21BodyEquipmentSlot
	{
		FName Semantic;
		FName ItemDefinitionId;
		ECodeBEquipSlot EquipmentSlot = ECodeBEquipSlot::None;
		FName ContainerType;
	};

	const TArray<FP21BodyEquipmentSlot>& P21BodyEquipmentSlots()
	{
		static const TArray<FP21BodyEquipmentSlot> Slots = {
			{ FName(TEXT("Body.Weapon")), Fdemo_mapItemIds::HeavyPracticeBlade, ECodeBEquipSlot::Weapon, FName(TEXT("CodeB.Body.Weapon")) },
			{ FName(TEXT("Body.ArmorRobe")), Fdemo_mapItemIds::ReinforcedVest, ECodeBEquipSlot::Armor, FName(TEXT("CodeB.Body.ArmorRobe")) },
			{ FName(TEXT("Body.Accessory0")), Fdemo_mapItemIds::EvasionCharm, ECodeBEquipSlot::Accessory, FName(TEXT("CodeB.Body.Accessory0")) }
		};
		return Slots;
	}

	bool IsP21BasicCorpseR3(const FCodeBLootProfile& Profile)
	{
		return Profile.LootProfileId == FName(TEXT("CodeB.LootProfile.BasicCorpse.r3"))
			&& Profile.ProfileVersion == 3
			&& Profile.AlgorithmVersion == TEXT("CodeB.DeterministicWeightedLoot.Crc32.r3");
	}

	const FP21BodyEquipmentSlot* FindP21BodyEquipmentSlot(const FName ItemDefinitionId)
	{
		return P21BodyEquipmentSlots().FindByPredicate([ItemDefinitionId](const FP21BodyEquipmentSlot& Value)
		{
			return Value.ItemDefinitionId == ItemDefinitionId;
		});
	}

	FString P21EquipmentCandidateSetDigest()
	{
		TArray<FString> Pieces;
		for (const FP21BodyEquipmentSlot& Slot : P21BodyEquipmentSlots())
		{
			Pieces.Add(FString::Printf(TEXT("%s:%s:%d"), *Slot.Semantic.ToString(),
				*Slot.ItemDefinitionId.ToString(), static_cast<int32>(Slot.EquipmentSlot)));
		}
		Pieces.Sort();
		return FString::Printf(TEXT("%08X"), FCrc::StrCrc32(*FString::Join(Pieces, TEXT("|"))));
	}

	bool ValidateP21BodyEquipmentCatalog(FString& OutError)
	{
		TSet<FName> Definitions;
		TSet<ECodeBEquipSlot> EquipmentSlots;
		for (const FP21BodyEquipmentSlot& Candidate : P21BodyEquipmentSlots())
		{
			FCodeBItemDefinition Definition;
			if (Candidate.Semantic.IsNone() || Candidate.ItemDefinitionId.IsNone() || Candidate.ContainerType.IsNone()
				|| Candidate.EquipmentSlot == ECodeBEquipSlot::None
				|| Definitions.Contains(Candidate.ItemDefinitionId) || EquipmentSlots.Contains(Candidate.EquipmentSlot)
				|| !BuildCanonicalCodeBItemDefinition(Candidate.ItemDefinitionId, Definition, OutError)
				|| Definition.bStackable || Definition.MaxStack != 1 || Definition.EquipSlot != Candidate.EquipmentSlot
				|| Definition.SpatialContainerSemantic != ECodeBSpatialContainerSemantic::None
				|| Definition.ChildContainerCapacity != 0
				|| (Definition.ItemType != ECodeBItemType::Weapon && Definition.ItemType != ECodeBItemType::Armor
					&& Definition.ItemType != ECodeBItemType::Accessory))
			{
				if (OutError.IsEmpty()) OutError = TEXT("Code B P21 formal equipment candidate catalog is incomplete or incompatible.");
				return false;
			}
			Definitions.Add(Candidate.ItemDefinitionId);
			EquipmentSlots.Add(Candidate.EquipmentSlot);
		}
		return Definitions.Num() == 3 && EquipmentSlots.Num() == 3 && !P21EquipmentCandidateSetDigest().IsEmpty();
	}

	FGuid P21BodyEquipmentContainerGuid(
		const FGuid& BodyTargetId,
		const FName SourceDefinitionId,
		const FName SlotSemantic)
	{
		const FString Seed = FString::Printf(TEXT("P21|body:%s|source:%s|slot:%s"),
			*GuidText(BodyTargetId), *SourceDefinitionId.ToString(), *SlotSemantic.ToString());
		return FGuid(
			FCrc::StrCrc32(*(Seed + TEXT("|A"))), FCrc::StrCrc32(*(Seed + TEXT("|B"))),
			FCrc::StrCrc32(*(Seed + TEXT("|C"))), FCrc::StrCrc32(*(Seed + TEXT("|D"))));
	}

	/** P19 deliberately recognizes only the two formal P17 parents, never a generic child graph. */
	bool ValidateP19WorldDropClosure(
		const FCodeBSnapshot& Snapshot,
		const FCodeBItemInstance& Parent,
		bool& bOutIsSpatialClosure,
		FString& OutError)
	{
		bOutIsSpatialClosure = false;
		const bool bWindTalisman = Parent.DefinitionId == Fdemo_mapItemIds::WindTalisman;
		const bool bBackpackLevel1 = Parent.DefinitionId == Fdemo_mapItemIds::BackpackLevel1;
		if (!Parent.ChildContainerId.IsValid())
		{
			// Keep a pre-P17 P14 record readable as the old simple-root shape. P19
			// never creates this shape for either formal parent (the writer rejects it).
			return true;
		}
		if (!bWindTalisman && !bBackpackLevel1)
		{
			OutError = TEXT("Code B P19 WorldDrop rejects a non-formal child graph.");
			return false;
		}

		FCodeBItemDefinition CanonicalParent;
		if (!BuildCanonicalCodeBItemDefinition(Parent.DefinitionId, CanonicalParent, OutError))
		{
			return false;
		}
		const ECodeBSpatialContainerSemantic ExpectedSemantic = bWindTalisman
			? ECodeBSpatialContainerSemantic::QuickRing
			: ECodeBSpatialContainerSemantic::StoragePouch;
		const FName ExpectedType = bWindTalisman
			? FName(TEXT("CodeB.SpatialChild.QuickRing"))
			: FName(TEXT("CodeB.SpatialChild.StoragePouch"));
		const FGuid ExpectedChildId = SpatialChildGuid(Parent.ItemId);
		const FCodeBContainer* Child = Snapshot.Containers.Find(ExpectedChildId);
		if (CanonicalParent.SpatialContainerSemantic != ExpectedSemantic
			|| CanonicalParent.ChildContainerCapacity < 1
			|| Parent.ChildContainerId != ExpectedChildId
			|| !Child || Child->ContainerType != ExpectedType || Child->IsEquipment()
			|| Child->Slots.Num() != CanonicalParent.ChildContainerCapacity)
		{
			OutError = TEXT("Code B P19 WorldDrop child identity, type, or capacity is not the formal P17 closure.");
			return false;
		}
		for (int32 SlotIndex = 0; SlotIndex < Child->Slots.Num(); ++SlotIndex)
		{
			const FGuid ChildItemId = Child->Slots[SlotIndex];
			if (!ChildItemId.IsValid()) continue;
			const FCodeBItemInstance* ChildItem = Snapshot.Items.Find(ChildItemId);
			FCodeBItemDefinition CanonicalChild;
			if (!ChildItem || ChildItem->ItemId != ChildItemId
				|| ChildItem->ParentContainerId != ExpectedChildId || ChildItem->SlotIndex != SlotIndex
				|| ChildItem->ChildContainerId.IsValid()
				|| !BuildCanonicalCodeBItemDefinition(ChildItem->DefinitionId, CanonicalChild, OutError)
				|| CanonicalChild.SpatialContainerSemantic != ECodeBSpatialContainerSemantic::None
				|| CanonicalChild.ItemType == ECodeBItemType::SpatialItem
				|| CanonicalChild.ItemType == ECodeBItemType::Backpack)
			{
				if (OutError.IsEmpty())
				{
					OutError = TEXT("Code B P19 WorldDrop child closure contains an invalid or nested item.");
				}
				return false;
			}
		}
		bOutIsSpatialClosure = true;
		return true;
	}

	bool DiscardP19WorldDropClosure(
		FCodeBSnapshot& InOutSnapshot,
		const FCodeBWorldDropRecord& Record,
		FString& OutError)
	{
		const FCodeBItemInstance* Parent = InOutSnapshot.Items.Find(Record.ItemId);
		const FCodeBContainer* WorldRoot = InOutSnapshot.Containers.Find(Record.WorldContainerId);
		bool bIsSpatialClosure = false;
		if (!Parent || !WorldRoot || Parent->ParentContainerId != Record.WorldContainerId
			|| Parent->SlotIndex != 0 || WorldRoot->Slots.Num() != 1
			|| WorldRoot->Slots[0] != Record.ItemId
			|| !ValidateP19WorldDropClosure(InOutSnapshot, *Parent, bIsSpatialClosure, OutError))
		{
			if (OutError.IsEmpty())
			{
				OutError = TEXT("Code B P19 cannot discard a malformed WorldDrop closure during P8 closure.");
			}
			return false;
		}
		if (bIsSpatialClosure)
		{
			const FGuid ChildContainerId = Parent->ChildContainerId;
			const FCodeBContainer* Child = InOutSnapshot.Containers.Find(ChildContainerId);
			TArray<FGuid> ChildItemIds;
			if (!Child)
			{
				OutError = TEXT("Code B P19 lost the validated WorldDrop child during P8 closure.");
				return false;
			}
			for (const FGuid& ChildItemId : Child->Slots)
			{
				if (ChildItemId.IsValid()) ChildItemIds.Add(ChildItemId);
			}
			for (const FGuid& ChildItemId : ChildItemIds)
			{
				InOutSnapshot.Items.Remove(ChildItemId);
			}
			InOutSnapshot.Containers.Remove(ChildContainerId);
		}
		InOutSnapshot.Items.Remove(Record.ItemId);
		InOutSnapshot.Containers.Remove(Record.WorldContainerId);
		return true;
	}

	bool EnsureP17SpatialChildrenForSnapshot(
		FCodeBSnapshot& InOutSnapshot,
		FCodeBP2PlayerLayout& InOutLayout,
		bool& bOutChanged,
		FString& OutError)
	{
		bOutChanged = false;
		FCodeBRepository Repository;
		if (!Repository.LoadPersistedSnapshot(InOutSnapshot, &OutError))
		{
			return false;
		}

		FCodeBP2PlayerLayout CandidateLayout = InOutLayout;
		TArray<FGuid> ItemIds;
		InOutSnapshot.Items.GenerateKeyArray(ItemIds);
		for (const FGuid& ItemId : ItemIds)
		{
			const FCodeBItemInstance* Item = Repository.FindItem(ItemId);
			if (!Item || !Fdemo_mapItemDefinitions::Find(Item->DefinitionId))
			{
				continue;
			}

			FCodeBItemDefinition CanonicalDefinition;
			if (!BuildCanonicalCodeBItemDefinition(Item->DefinitionId, CanonicalDefinition, OutError))
			{
				return false;
			}
			if (CanonicalDefinition.SpatialContainerSemantic == ECodeBSpatialContainerSemantic::None)
			{
				continue;
			}

			FGuid ChildContainerId = Item->ChildContainerId;
			if (!ChildContainerId.IsValid())
			{
				const FName ContainerType = CanonicalDefinition.SpatialContainerSemantic == ECodeBSpatialContainerSemantic::QuickRing
					? FName(TEXT("CodeB.SpatialChild.QuickRing"))
					: FName(TEXT("CodeB.SpatialChild.StoragePouch"));
				ChildContainerId = Repository.CreateContainer(
					ContainerType,
					CanonicalDefinition.ChildContainerCapacity,
					ECodeBContainerKind::Storage,
					ECodeBEquipSlot::None,
					&OutError,
					SpatialChildGuid(ItemId));
				if (!ChildContainerId.IsValid()
					|| !Repository.AssociateChildContainer(ItemId, ChildContainerId, &OutError))
				{
					return false;
				}
				bOutChanged = true;
			}

			if (CanonicalDefinition.SpatialContainerSemantic == ECodeBSpatialContainerSemantic::QuickRing
				&& Item->ParentContainerId == CandidateLayout.SpatialContainerId
				&& CandidateLayout.SpatialInternalContainerId != ChildContainerId)
			{
				CandidateLayout.SpatialInternalContainerId = ChildContainerId;
				bOutChanged = true;
			}
			if (CanonicalDefinition.SpatialContainerSemantic == ECodeBSpatialContainerSemantic::StoragePouch
				&& Item->ParentContainerId == CandidateLayout.BackpackContainerId
				&& CandidateLayout.PouchInternalContainerId != ChildContainerId)
			{
				CandidateLayout.PouchInternalContainerId = ChildContainerId;
				bOutChanged = true;
			}
		}

		if (bOutChanged)
		{
			InOutSnapshot = Repository.CaptureSnapshot();
			InOutLayout = MoveTemp(CandidateLayout);
		}
		return true;
	}

	bool EnsureP17SpatialChildrenForRecord(
		FCodeBOutOfRaidInventoryRecord& InOutRecord,
		bool& bOutChanged,
		FString& OutError)
	{
		bOutChanged = false;
		bool bProfileChanged = false;
		if (!EnsureP17SpatialChildrenForSnapshot(
			InOutRecord.RepositorySnapshot, InOutRecord.Layout, bProfileChanged, OutError))
		{
			return false;
		}

		bool bRunChanged = false;
		if (InOutRecord.bHasActiveRunInventorySession
			&& !EnsureP17SpatialChildrenForSnapshot(
				InOutRecord.ActiveRunInventorySession.RepositorySnapshot,
				InOutRecord.ActiveRunInventorySession.Layout,
				bRunChanged,
				OutError))
		{
			return false;
		}
		if (bRunChanged)
		{
			if (InOutRecord.ActiveRunInventorySession.SessionRevision == MAX_int32)
			{
				OutError = TEXT("Code B P17 spatial migration cannot advance the active Run session revision.");
				return false;
			}
			++InOutRecord.ActiveRunInventorySession.SessionRevision;
			InOutRecord.ActiveRunInventorySession.LastCommittedUtc = UtcNow();
			// Pre-freeze P6 receipts derive their digest from the live graph; frozen
			// receipts intentionally retain their immutable origin payload instead.
			InOutRecord.ActiveRunInventorySession.Receipt.PayloadDigest =
				RunInventoryPayloadDigest(InOutRecord.ActiveRunInventorySession);
		}
		bOutChanged = bProfileChanged || bRunChanged;
		return true;
	}

	bool ValidateNormalContainerDefinition(const FCodeBNormalContainerDefinition& Definition, FString& OutError)
	{
		if (Definition.DefinitionId.IsNone() || Definition.ContainerType.IsNone()
			|| Definition.Capacity < 1 || Definition.ContentRevision < 1
			|| Definition.ContentPlan.Num() > Definition.Capacity)
		{
			OutError = TEXT("Code B normal-container definition identity or capacity is invalid.");
			return false;
		}
		TSet<int32> OccupiedSlots;
		for (const FCodeBNormalContainerContentPlanEntry& Entry : Definition.ContentPlan)
		{
			FCodeBItemDefinition ItemDefinition;
			if (Entry.ItemDefinitionId.IsNone() || Entry.Quantity < 1 || Entry.SlotIndex < 0
				|| Entry.SlotIndex >= Definition.Capacity || Entry.ChildContainerCapacity < 0
				|| OccupiedSlots.Contains(Entry.SlotIndex)
				|| !BuildCanonicalCodeBItemDefinition(Entry.ItemDefinitionId, ItemDefinition, OutError))
			{
				if (OutError.IsEmpty()) OutError = TEXT("Code B normal-container item plan is invalid.");
				return false;
			}
			if ((!ItemDefinition.bStackable && Entry.Quantity != 1)
				|| Entry.Quantity > ItemDefinition.MaxStack
				|| (Entry.ChildContainerCapacity > 0
					&& ItemDefinition.ItemType != ECodeBItemType::SpatialItem
					&& ItemDefinition.ItemType != ECodeBItemType::Backpack))
			{
				OutError = TEXT("Code B normal-container item plan violates its canonical item definition.");
				return false;
			}
			OccupiedSlots.Add(Entry.SlotIndex);
		}
		return true;
	}

	FString NormalContainerDefinitionDigest(const FCodeBNormalContainerDefinition& Definition)
	{
		TArray<FString> Pieces;
		Pieces.Add(FString::Printf(TEXT("definition:%s:%s:%d:%d"), *Definition.DefinitionId.ToString(), *Definition.ContainerType.ToString(), Definition.Capacity, Definition.ContentRevision));
		for (const FCodeBNormalContainerContentPlanEntry& Entry : Definition.ContentPlan)
		{
			Pieces.Add(FString::Printf(TEXT("item:%s:%d:%d:%d"), *Entry.ItemDefinitionId.ToString(), Entry.Quantity, Entry.SlotIndex, Entry.ChildContainerCapacity));
		}
		Pieces.Sort();
		return FString::Printf(TEXT("%08X"), FCrc::StrCrc32(*FString::Join(Pieces, TEXT("|"))));
	}

	FGuid StableRunLocalGuid(
		const FGuid& OwnerId,
		const FGuid& RunInstanceId,
		const FGuid& SearchTargetId,
		const FName DefinitionId,
		const int32 ContentRevision,
		const uint32 Salt)
	{
		const FString Seed = FString::Printf(TEXT("%s|%s|%s|%s|%d|%u"),
			*GuidText(OwnerId), *GuidText(RunInstanceId), *GuidText(SearchTargetId),
			*DefinitionId.ToString(), ContentRevision, Salt);
		return FGuid(
			FCrc::StrCrc32(*(Seed + TEXT("|A"))),
			FCrc::StrCrc32(*(Seed + TEXT("|B"))),
			FCrc::StrCrc32(*(Seed + TEXT("|C"))),
			FCrc::StrCrc32(*(Seed + TEXT("|D"))));
	}

	FString NormalContainerMaterializationDigest(const FCodeBRunLocalNormalContainerRecord& Record)
	{
		TArray<FString> Pieces;
		Pieces.Add(FString::Printf(TEXT("identity:%s:%s:%s:%s:%s"),
			*GuidText(Record.OwnerId), *GuidText(Record.RunInstanceId), *GuidText(Record.SearchTargetId),
			*Record.DefinitionId.ToString(), *GuidText(Record.ContainerId)));
		if (HasLootProfileProvenance(Record.Receipt.LootProfileId, Record.Receipt.LootProfileVersion,
			Record.Receipt.LootProfileDigest, Record.Receipt.LootAlgorithmVersion, Record.Receipt.LootResultDigest))
		{
			Pieces.Add(FString::Printf(TEXT("loot:%s:%d:%s:%s:%s"),
				*Record.Receipt.LootProfileId.ToString(), Record.Receipt.LootProfileVersion,
				*Record.Receipt.LootProfileDigest, *Record.Receipt.LootAlgorithmVersion,
				*Record.Receipt.LootResultDigest));
		}
		Pieces.Add(TerminalSnapshotDigest(Record.ContainerSnapshot, FCodeBP2PlayerLayout()));
		for (const FCodeBNormalContainerItemReveal& Reveal : Record.ItemRevealStates)
		{
			Pieces.Add(FString::Printf(TEXT("reveal:%s:%s"), *GuidText(Reveal.ItemId), NormalContainerRevealStateText(Reveal.RevealState)));
		}
		Pieces.Sort();
		return FString::Printf(TEXT("%08X"), FCrc::StrCrc32(*FString::Join(Pieces, TEXT("|"))));
	}

	TSharedRef<FJsonObject> DefinitionJson(const FCodeBItemDefinition& Definition)
	{
		TSharedRef<FJsonObject> Object = MakeShared<FJsonObject>();
		Object->SetStringField(TEXT("Id"), Definition.DefinitionId.ToString());
		Object->SetNumberField(TEXT("Type"), static_cast<int32>(Definition.ItemType));
		Object->SetBoolField(TEXT("Stackable"), Definition.bStackable);
		Object->SetBoolField(TEXT("QuickUsable"), Definition.bQuickUsable);
		Object->SetNumberField(TEXT("QuickUseEffect"), static_cast<int32>(Definition.QuickUseEffect));
		Object->SetNumberField(TEXT("QuickUseRestoreAmount"), Definition.QuickUseRestoreAmount);
		Object->SetNumberField(TEXT("MaxStack"), Definition.MaxStack);
		Object->SetNumberField(TEXT("EquipSlot"), static_cast<int32>(Definition.EquipSlot));
		Object->SetNumberField(TEXT("Width"), Definition.GridWidth);
		Object->SetNumberField(TEXT("Height"), Definition.GridHeight);
		Object->SetNumberField(TEXT("SpatialContainerSemantic"), static_cast<int32>(Definition.SpatialContainerSemantic));
		Object->SetNumberField(TEXT("ChildContainerCapacity"), Definition.ChildContainerCapacity);
		return Object;
	}

	TSharedRef<FJsonObject> ItemJson(const FCodeBItemInstance& Item)
	{
		TSharedRef<FJsonObject> Object = MakeShared<FJsonObject>();
		Object->SetStringField(TEXT("Id"), GuidText(Item.ItemId));
		Object->SetStringField(TEXT("DefinitionId"), Item.DefinitionId.ToString());
		Object->SetNumberField(TEXT("Quantity"), Item.Quantity);
		Object->SetNumberField(TEXT("Level"), Item.Level);
		Object->SetNumberField(TEXT("Quality"), Item.Quality);
		Object->SetNumberField(TEXT("RandomSeed"), Item.RandomSeed);
		Object->SetStringField(TEXT("ParentContainerId"), GuidText(Item.ParentContainerId));
		Object->SetNumberField(TEXT("SlotIndex"), Item.SlotIndex);
		Object->SetStringField(TEXT("ChildContainerId"), GuidText(Item.ChildContainerId));
		Object->SetStringField(TEXT("LegacyAffixDigest"), Item.LegacyAffixDigest);
		return Object;
	}

	TSharedRef<FJsonObject> ContainerJson(const FCodeBContainer& Container)
	{
		TSharedRef<FJsonObject> Object = MakeShared<FJsonObject>();
		Object->SetStringField(TEXT("Id"), GuidText(Container.ContainerId));
		Object->SetStringField(TEXT("Type"), Container.ContainerType.ToString());
		Object->SetNumberField(TEXT("Kind"), static_cast<int32>(Container.Kind));
		Object->SetNumberField(TEXT("EquipmentSlot"), static_cast<int32>(Container.EquipmentSlot));
		TArray<TSharedPtr<FJsonValue>> Slots;
		for (const FGuid& ItemId : Container.Slots)
		{
			Slots.Add(MakeShared<FJsonValueString>(GuidText(ItemId)));
		}
		Object->SetArrayField(TEXT("Slots"), Slots);
		return Object;
	}

	TSharedRef<FJsonObject> LayoutJson(const FCodeBP2PlayerLayout& Layout)
	{
		TSharedRef<FJsonObject> Object = MakeShared<FJsonObject>();
		Object->SetStringField(TEXT("LayoutId"), Layout.LayoutId.ToString());
		Object->SetStringField(TEXT("Warehouse"), GuidText(Layout.WarehouseContainerId));
		Object->SetStringField(TEXT("Basic"), GuidText(Layout.BasicContainerId));
		Object->SetStringField(TEXT("Weapon"), GuidText(Layout.WeaponContainerId));
		Object->SetStringField(TEXT("Armor"), GuidText(Layout.ArmorContainerId));
		Object->SetStringField(TEXT("SpatialRing"), GuidText(Layout.SpatialContainerId));
		Object->SetStringField(TEXT("Backpack"), GuidText(Layout.BackpackContainerId));
		Object->SetStringField(TEXT("QuickSpatial"), GuidText(Layout.SpatialInternalContainerId));
		Object->SetStringField(TEXT("Pouch"), GuidText(Layout.PouchInternalContainerId));
		Object->SetBoolField(TEXT("ConditionalSpatial"), Layout.bUseConditionalSpatialContainers);
		TArray<TSharedPtr<FJsonValue>> Accessories;
		for (const FGuid& Value : Layout.AccessoryContainerIds)
		{
			Accessories.Add(MakeShared<FJsonValueString>(GuidText(Value)));
		}
		Object->SetArrayField(TEXT("Accessories"), Accessories);
		return Object;
	}

	bool JsonToDefinition(const TSharedPtr<FJsonObject>& Object, FCodeBItemDefinition& OutDefinition, FString& OutError)
	{
		FString Id;
		int32 Type, MaxStack, EquipSlot, Width, Height;
		bool bStackable = false;
		bool bQuickUsable = false;
		int32 QuickUseEffectValue = static_cast<int32>(ECodeBQuickUseEffectKind::None);
		int32 QuickUseRestoreAmount = 0;
		int32 SpatialContainerSemanticValue = static_cast<int32>(ECodeBSpatialContainerSemantic::None);
		int32 ChildContainerCapacity = 0;
		if (!Object.IsValid() || !Object->TryGetStringField(TEXT("Id"), Id) || Id.IsEmpty()
			|| !Object->TryGetBoolField(TEXT("Stackable"), bStackable)
			|| !ReadInt(Object, TEXT("Type"), Type, OutError)
			|| !ReadInt(Object, TEXT("MaxStack"), MaxStack, OutError)
			|| !ReadInt(Object, TEXT("EquipSlot"), EquipSlot, OutError)
			|| !ReadInt(Object, TEXT("Width"), Width, OutError)
			|| !ReadInt(Object, TEXT("Height"), Height, OutError))
		{
			if (OutError.IsEmpty()) OutError = TEXT("Code B definition JSON is invalid.");
			return false;
		}
		OutDefinition.DefinitionId = FName(*Id);
		OutDefinition.ItemType = static_cast<ECodeBItemType>(Type);
		OutDefinition.bStackable = bStackable;
		// P13 persists an explicit Code B semantic.  For records written before
		// schema 7, promote only the already-authoritative legacy consumable
		// category; unknown and non-consumable definitions remain false.  The next
		// accepted durable write serializes this as the explicit P13 marker.
		if (!Object->TryGetBoolField(TEXT("QuickUsable"), bQuickUsable))
		{
			if (const Fdemo_mapItemDefinition* LegacyDefinition =
				Fdemo_mapItemDefinitions::Find(OutDefinition.DefinitionId))
			{
				bQuickUsable = LegacyDefinition->CategoryId == Fdemo_mapItemIds::ConsumableCategory;
			}
		}
		OutDefinition.bQuickUsable = bQuickUsable;
		if (Object->HasField(TEXT("QuickUseEffect")) || Object->HasField(TEXT("QuickUseRestoreAmount")))
		{
			if (!ReadInt(Object, TEXT("QuickUseEffect"), QuickUseEffectValue, OutError)
				|| !ReadInt(Object, TEXT("QuickUseRestoreAmount"), QuickUseRestoreAmount, OutError))
			{
				return false;
			}
			OutDefinition.QuickUseEffect = static_cast<ECodeBQuickUseEffectKind>(QuickUseEffectValue);
			OutDefinition.QuickUseRestoreAmount = QuickUseRestoreAmount;
		}
		else if (const Fdemo_mapItemDefinition* LegacyDefinition = Fdemo_mapItemDefinitions::Find(OutDefinition.DefinitionId))
		{
			FCodeBItemDefinition Canonical;
			FString CanonicalError;
			if (BuildCanonicalCodeBItemDefinition(OutDefinition.DefinitionId, Canonical, CanonicalError))
			{
				OutDefinition.QuickUseEffect = Canonical.QuickUseEffect;
				OutDefinition.QuickUseRestoreAmount = Canonical.QuickUseRestoreAmount;
			}
		}
		OutDefinition.MaxStack = MaxStack;
		OutDefinition.EquipSlot = static_cast<ECodeBEquipSlot>(EquipSlot);
		OutDefinition.GridWidth = Width;
		OutDefinition.GridHeight = Height;
		const bool bHasSpatialSemantic = Object->HasField(TEXT("SpatialContainerSemantic"));
		const bool bHasChildCapacity = Object->HasField(TEXT("ChildContainerCapacity"));
		if (bHasSpatialSemantic != bHasChildCapacity
			|| ((bHasSpatialSemantic || bHasChildCapacity)
				&& (!ReadInt(Object, TEXT("SpatialContainerSemantic"), SpatialContainerSemanticValue, OutError)
					|| !ReadInt(Object, TEXT("ChildContainerCapacity"), ChildContainerCapacity, OutError))))
		{
			if (OutError.IsEmpty()) OutError = TEXT("Code B spatial-container definition JSON is incomplete.");
			return false;
		}
		// Historical P5/P6/P8 snapshots intentionally retain no new semantic when
		// these fields are absent. Their existing child graph is then preserved
		// byte-for-byte until an accepted P1 transaction writes it again.
		OutDefinition.SpatialContainerSemantic = static_cast<ECodeBSpatialContainerSemantic>(SpatialContainerSemanticValue);
		OutDefinition.ChildContainerCapacity = ChildContainerCapacity;
		return true;
	}

	bool JsonToItem(const TSharedPtr<FJsonObject>& Object, FCodeBItemInstance& OutItem, FString& OutError)
	{
		FString DefinitionId, Digest;
		int32 Quantity, Level, Quality, RandomSeed, SlotIndex;
		if (!ReadGuid(Object, TEXT("Id"), OutItem.ItemId, OutError, true)
			|| !ReadGuid(Object, TEXT("ParentContainerId"), OutItem.ParentContainerId, OutError)
			|| !ReadGuid(Object, TEXT("ChildContainerId"), OutItem.ChildContainerId, OutError)
			|| !Object->TryGetStringField(TEXT("DefinitionId"), DefinitionId) || DefinitionId.IsEmpty()
			|| !Object->TryGetStringField(TEXT("LegacyAffixDigest"), Digest)
			|| !ReadInt(Object, TEXT("Quantity"), Quantity, OutError)
			|| !ReadInt(Object, TEXT("Level"), Level, OutError)
			|| !ReadInt(Object, TEXT("Quality"), Quality, OutError)
			|| !ReadInt(Object, TEXT("RandomSeed"), RandomSeed, OutError)
			|| !ReadInt(Object, TEXT("SlotIndex"), SlotIndex, OutError))
		{
			if (OutError.IsEmpty()) OutError = TEXT("Code B item JSON is invalid.");
			return false;
		}
		OutItem.DefinitionId = FName(*DefinitionId);
		OutItem.Quantity = Quantity;
		OutItem.Level = Level;
		OutItem.Quality = Quality;
		OutItem.RandomSeed = RandomSeed;
		OutItem.SlotIndex = SlotIndex;
		OutItem.LegacyAffixDigest = Digest;
		return true;
	}

	bool JsonToContainer(const TSharedPtr<FJsonObject>& Object, FCodeBContainer& OutContainer, FString& OutError)
	{
		FString Type;
		int32 Kind, EquipmentSlot;
		const TArray<TSharedPtr<FJsonValue>>* Slots = nullptr;
		if (!ReadGuid(Object, TEXT("Id"), OutContainer.ContainerId, OutError, true)
			|| !Object->TryGetStringField(TEXT("Type"), Type) || Type.IsEmpty()
			|| !ReadInt(Object, TEXT("Kind"), Kind, OutError)
			|| !ReadInt(Object, TEXT("EquipmentSlot"), EquipmentSlot, OutError)
			|| !Object->TryGetArrayField(TEXT("Slots"), Slots) || !Slots)
		{
			if (OutError.IsEmpty()) OutError = TEXT("Code B container JSON is invalid.");
			return false;
		}
		OutContainer.ContainerType = FName(*Type);
		OutContainer.Kind = static_cast<ECodeBContainerKind>(Kind);
		OutContainer.EquipmentSlot = static_cast<ECodeBEquipSlot>(EquipmentSlot);
		for (const TSharedPtr<FJsonValue>& Slot : *Slots)
		{
			FGuid ItemId;
			if (!Slot.IsValid() || Slot->Type != EJson::String || !TryGuidText(Slot->AsString(), ItemId))
			{
				OutError = TEXT("Code B container slot JSON is invalid.");
				return false;
			}
			OutContainer.Slots.Add(ItemId);
		}
		return true;
	}

	bool JsonToLayout(
		const TSharedPtr<FJsonObject>& Object,
		FCodeBP2PlayerLayout& OutLayout,
		FString& OutError,
		const bool bRequireWarehouse = true)
	{
		FString LayoutId;
		const TArray<TSharedPtr<FJsonValue>>* Accessories = nullptr;
		bool bConditional = false;
		if (!Object.IsValid() || !Object->TryGetStringField(TEXT("LayoutId"), LayoutId) || LayoutId.IsEmpty()
			|| !ReadGuid(Object, TEXT("Warehouse"), OutLayout.WarehouseContainerId, OutError, bRequireWarehouse)
			|| !ReadGuid(Object, TEXT("Basic"), OutLayout.BasicContainerId, OutError, true)
			|| !ReadGuid(Object, TEXT("Weapon"), OutLayout.WeaponContainerId, OutError, true)
			|| !ReadGuid(Object, TEXT("Armor"), OutLayout.ArmorContainerId, OutError, true)
			|| !ReadGuid(Object, TEXT("SpatialRing"), OutLayout.SpatialContainerId, OutError, true)
			|| !ReadGuid(Object, TEXT("Backpack"), OutLayout.BackpackContainerId, OutError)
			|| !ReadGuid(Object, TEXT("QuickSpatial"), OutLayout.SpatialInternalContainerId, OutError, true)
			|| !ReadGuid(Object, TEXT("Pouch"), OutLayout.PouchInternalContainerId, OutError, true)
			|| !Object->TryGetBoolField(TEXT("ConditionalSpatial"), bConditional)
			|| !Object->TryGetArrayField(TEXT("Accessories"), Accessories) || !Accessories)
		{
			if (OutError.IsEmpty()) OutError = TEXT("Code B layout JSON is invalid.");
			return false;
		}
		OutLayout.LayoutId = FName(*LayoutId);
		OutLayout.bUseConditionalSpatialContainers = bConditional;
		for (const TSharedPtr<FJsonValue>& Value : *Accessories)
		{
			FGuid Accessory;
			if (!Value.IsValid() || !TryGuidText(Value->AsString(), Accessory) || !Accessory.IsValid())
			{
				OutError = TEXT("Code B layout accessory JSON is invalid.");
				return false;
			}
			OutLayout.AccessoryContainerIds.Add(Accessory);
		}
		return !OutLayout.AccessoryContainerIds.IsEmpty();
	}

	TSharedRef<FJsonObject> SnapshotJson(const FCodeBSnapshot& Snapshot)
	{
		TSharedRef<FJsonObject> Object = MakeShared<FJsonObject>();
		Object->SetNumberField(TEXT("Revision"), Snapshot.Revision);
		TArray<TSharedPtr<FJsonValue>> Definitions;
		for (const TPair<FName, FCodeBItemDefinition>& Pair : Snapshot.Definitions)
		{
			Definitions.Add(MakeShared<FJsonValueObject>(DefinitionJson(Pair.Value)));
		}
		Object->SetArrayField(TEXT("Definitions"), Definitions);
		TArray<TSharedPtr<FJsonValue>> Items;
		for (const TPair<FGuid, FCodeBItemInstance>& Pair : Snapshot.Items)
		{
			Items.Add(MakeShared<FJsonValueObject>(ItemJson(Pair.Value)));
		}
		Object->SetArrayField(TEXT("Items"), Items);
		TArray<TSharedPtr<FJsonValue>> Containers;
		for (const TPair<FGuid, FCodeBContainer>& Pair : Snapshot.Containers)
		{
			Containers.Add(MakeShared<FJsonValueObject>(ContainerJson(Pair.Value)));
		}
		Object->SetArrayField(TEXT("Containers"), Containers);
		return Object;
	}

	bool JsonToSnapshot(
		const TSharedPtr<FJsonObject>& Object,
		FCodeBSnapshot& OutSnapshot,
		FString& OutError)
	{
		OutSnapshot = FCodeBSnapshot();
		const TArray<TSharedPtr<FJsonValue>> *Definitions = nullptr, *Items = nullptr, *Containers = nullptr;
		if (!ReadInt(Object, TEXT("Revision"), OutSnapshot.Revision, OutError)
			|| !Object->TryGetArrayField(TEXT("Definitions"), Definitions)
			|| !Object->TryGetArrayField(TEXT("Items"), Items)
			|| !Object->TryGetArrayField(TEXT("Containers"), Containers)
			|| !Definitions || !Items || !Containers)
		{
			if (OutError.IsEmpty()) OutError = TEXT("Code B snapshot JSON is invalid.");
			return false;
		}
		for (const TSharedPtr<FJsonValue>& Value : *Definitions)
		{
			FCodeBItemDefinition Definition;
			if (!JsonToDefinition(Value.IsValid() ? Value->AsObject() : nullptr, Definition, OutError)) return false;
			OutSnapshot.Definitions.Add(Definition.DefinitionId, Definition);
		}
		for (const TSharedPtr<FJsonValue>& Value : *Items)
		{
			FCodeBItemInstance Item;
			if (!JsonToItem(Value.IsValid() ? Value->AsObject() : nullptr, Item, OutError)) return false;
			OutSnapshot.Items.Add(Item.ItemId, Item);
		}
		for (const TSharedPtr<FJsonValue>& Value : *Containers)
		{
			FCodeBContainer Container;
			if (!JsonToContainer(Value.IsValid() ? Value->AsObject() : nullptr, Container, OutError)) return false;
			OutSnapshot.Containers.Add(Container.ContainerId, Container);
		}
		FCodeBRepository ValidationRepository;
		return ValidationRepository.LoadPersistedSnapshot(OutSnapshot, &OutError);
	}

	bool IsHotbarSlotIndex(const int32 SlotIndex)
	{
		return SlotIndex >= 1 && SlotIndex <= FCodeBHotbarBindings::SlotCount;
	}

	bool ValidateHotbarBindingShape(const FCodeBHotbarBindings& Bindings, FString& OutError)
	{
		if (Bindings.Slots.Num() != FCodeBHotbarBindings::SlotCount)
		{
			OutError = TEXT("Code B P13 hotbar must contain exactly nine logical slots.");
			return false;
		}
		TSet<FGuid> SeenItemIds;
		for (int32 ArrayIndex = 0; ArrayIndex < Bindings.Slots.Num(); ++ArrayIndex)
		{
			const FCodeBHotbarBinding& Slot = Bindings.Slots[ArrayIndex];
			if (Slot.SlotIndex != ArrayIndex + 1
				|| (!Slot.bHasReference && Slot.ItemId.IsValid())
				|| (Slot.bHasReference && (!Slot.ItemId.IsValid() || SeenItemIds.Contains(Slot.ItemId))))
			{
				OutError = TEXT("Code B P13 hotbar binding shape or unique ItemId reference is invalid.");
				return false;
			}
			if (Slot.bHasReference)
			{
				SeenItemIds.Add(Slot.ItemId);
			}
		}
		return true;
	}

	bool IsHotbarItemEligible(
		const FCodeBSnapshot& Snapshot,
		const FCodeBP2PlayerLayout& Layout,
		const FGuid& ItemId)
	{
		if (!ItemId.IsValid() || !Layout.BasicContainerId.IsValid())
		{
			return false;
		}
		const FCodeBItemInstance* Item = Snapshot.Items.Find(ItemId);
		const FCodeBItemDefinition* Definition = Item
			? Snapshot.Definitions.Find(Item->DefinitionId) : nullptr;
		const FCodeBContainer* BaseQuick = Snapshot.Containers.Find(Layout.BasicContainerId);
		return Item && Definition && BaseQuick
			&& Item->Quantity > 0
			&& Definition->bQuickUsable
			&& Item->ParentContainerId == Layout.BasicContainerId
			&& BaseQuick->Slots.IsValidIndex(Item->SlotIndex)
			&& BaseQuick->Slots[Item->SlotIndex] == ItemId;
	}

	bool ReconcileHotbarBindings(
		FCodeBHotbarBindings& Bindings,
		const FCodeBSnapshot& Snapshot,
		const FCodeBP2PlayerLayout& Layout,
		FString& OutError)
	{
		if (Bindings.Slots.Num() == 0)
		{
			Bindings = FCodeBHotbarBindings();
		}
		if (Bindings.Slots.Num() != FCodeBHotbarBindings::SlotCount)
		{
			OutError = TEXT("Code B P13 cannot reconcile a hotbar with a non-nine-slot shape.");
			return false;
		}
		TSet<FGuid> SeenItemIds;
		for (int32 ArrayIndex = 0; ArrayIndex < Bindings.Slots.Num(); ++ArrayIndex)
		{
			FCodeBHotbarBinding& Slot = Bindings.Slots[ArrayIndex];
			if (Slot.SlotIndex != ArrayIndex + 1)
			{
				OutError = TEXT("Code B P13 refuses an unstable hotbar slot index during reconcile.");
				return false;
			}
			if (!Slot.bHasReference || !Slot.ItemId.IsValid()
				|| SeenItemIds.Contains(Slot.ItemId)
				|| !IsHotbarItemEligible(Snapshot, Layout, Slot.ItemId))
			{
				Slot.bHasReference = false;
				Slot.ItemId.Invalidate();
				continue;
			}
			SeenItemIds.Add(Slot.ItemId);
		}
		return ValidateHotbarBindingShape(Bindings, OutError);
	}

	bool BuildHotbarProjection(
		const FCodeBHotbarBindings& Bindings,
		const FCodeBSnapshot& Snapshot,
		const bool bEditable,
		const bool bActiveRunScope,
		const int32 DurableRevision,
		FCodeBHotbarProjection& OutProjection,
		FString& OutError)
	{
		if (!ValidateHotbarBindingShape(Bindings, OutError)) return false;
		OutProjection = FCodeBHotbarProjection();
		OutProjection.bEditable = bEditable;
		OutProjection.bActiveRunScope = bActiveRunScope;
		OutProjection.DurableRevision = DurableRevision;
		OutProjection.Slots.Reserve(FCodeBHotbarBindings::SlotCount);
		for (const FCodeBHotbarBinding& Binding : Bindings.Slots)
		{
			FCodeBHotbarSlotProjection& Slot = OutProjection.Slots.AddDefaulted_GetRef();
			Slot.SlotIndex = Binding.SlotIndex;
			Slot.bHasReference = Binding.bHasReference;
			if (!Binding.bHasReference) continue;
			const FCodeBItemInstance* Item = Snapshot.Items.Find(Binding.ItemId);
			if (!Item)
			{
				OutError = TEXT("Code B P13 cannot project an unresolved hotbar ItemId.");
				return false;
			}
			Slot.ItemId = Binding.ItemId;
			Slot.DefinitionId = Item->DefinitionId;
			Slot.Quantity = Item->Quantity;
		}
		return true;
	}

	bool ValidateHotbarBindingsAgainstSnapshot(
		const FCodeBHotbarBindings& Bindings,
		const FCodeBSnapshot& Snapshot,
		const FCodeBP2PlayerLayout& Layout,
		FString& OutError)
	{
		if (!ValidateHotbarBindingShape(Bindings, OutError)) return false;
		for (const FCodeBHotbarBinding& Binding : Bindings.Slots)
		{
			if (Binding.bHasReference && !IsHotbarItemEligible(Snapshot, Layout, Binding.ItemId))
			{
				OutError = TEXT("Code B P13 durable hotbar contains a dangling or non-BaseQuick reference.");
				return false;
			}
		}
		return true;
	}

	bool ApplyHotbarBind(
		FCodeBHotbarBindings& Bindings,
		const FCodeBSnapshot& Snapshot,
		const FCodeBP2PlayerLayout& Layout,
		const FGuid& ItemId,
		const int32 SlotIndex,
		FString& OutError)
	{
		if (!IsHotbarSlotIndex(SlotIndex) || !IsHotbarItemEligible(Snapshot, Layout, ItemId))
		{
			OutError = TEXT("Code B P13 only binds a current BaseQuick QuickUsable item to slot 1--9.");
			return false;
		}
		if (!ReconcileHotbarBindings(Bindings, Snapshot, Layout, OutError)) return false;
		for (FCodeBHotbarBinding& Existing : Bindings.Slots)
		{
			if (Existing.bHasReference && Existing.ItemId == ItemId)
			{
				Existing.bHasReference = false;
				Existing.ItemId.Invalidate();
			}
		}
		FCodeBHotbarBinding& Target = Bindings.Slots[SlotIndex - 1];
		Target.bHasReference = true;
		Target.ItemId = ItemId;
		return ReconcileHotbarBindings(Bindings, Snapshot, Layout, OutError)
			&& Bindings.Slots[SlotIndex - 1].bHasReference
			&& Bindings.Slots[SlotIndex - 1].ItemId == ItemId;
	}

	bool ApplyHotbarUnbind(FCodeBHotbarBindings& Bindings, const int32 SlotIndex, FString& OutError)
	{
		if (!IsHotbarSlotIndex(SlotIndex) || !ValidateHotbarBindingShape(Bindings, OutError))
		{
			if (OutError.IsEmpty()) OutError = TEXT("Code B P13 only unbinds an explicit slot 1--9.");
			return false;
		}
		FCodeBHotbarBinding& Slot = Bindings.Slots[SlotIndex - 1];
		Slot.bHasReference = false;
		Slot.ItemId.Invalidate();
		return true;
	}

	TArray<TSharedPtr<FJsonValue>> HotbarBindingsJson(const FCodeBHotbarBindings& Bindings)
	{
		TArray<TSharedPtr<FJsonValue>> Values;
		Values.Reserve(Bindings.Slots.Num());
		for (const FCodeBHotbarBinding& Binding : Bindings.Slots)
		{
			TSharedRef<FJsonObject> Object = MakeShared<FJsonObject>();
			Object->SetNumberField(TEXT("SlotIndex"), Binding.SlotIndex);
			Object->SetBoolField(TEXT("HasReference"), Binding.bHasReference);
			if (Binding.bHasReference)
			{
				Object->SetStringField(TEXT("ItemId"), GuidText(Binding.ItemId));
			}
			Values.Add(MakeShared<FJsonValueObject>(Object));
		}
		return Values;
	}

	bool JsonToHotbarBindings(
		const TArray<TSharedPtr<FJsonValue>>& Values,
		FCodeBHotbarBindings& OutBindings,
		FString& OutError)
	{
		OutBindings.Slots.Reset();
		if (Values.Num() != FCodeBHotbarBindings::SlotCount)
		{
			OutError = TEXT("Code B P13 hotbar JSON must contain exactly nine slots.");
			return false;
		}
		for (int32 ArrayIndex = 0; ArrayIndex < Values.Num(); ++ArrayIndex)
		{
			const TSharedPtr<FJsonObject> Object = Values[ArrayIndex].IsValid() ? Values[ArrayIndex]->AsObject() : nullptr;
			FCodeBHotbarBinding& Binding = OutBindings.Slots.AddDefaulted_GetRef();
			if (!ReadInt(Object, TEXT("SlotIndex"), Binding.SlotIndex, OutError)
				|| !Object->TryGetBoolField(TEXT("HasReference"), Binding.bHasReference)
				|| (Binding.bHasReference && !ReadGuid(Object, TEXT("ItemId"), Binding.ItemId, OutError, true))
				|| (!Binding.bHasReference && Object->HasField(TEXT("ItemId"))))
			{
				if (OutError.IsEmpty()) OutError = TEXT("Code B P13 hotbar binding JSON is invalid.");
				return false;
			}
		}
		return ValidateHotbarBindingShape(OutBindings, OutError);
	}

	void WriteLootProfileProvenance(
		const TSharedRef<FJsonObject>& Object,
		const FName LootProfileId,
		const int32 LootProfileVersion,
		const FString& LootProfileDigest,
		const FString& LootAlgorithmVersion,
		const FString& LootResultDigest)
	{
		if (!HasLootProfileProvenance(LootProfileId, LootProfileVersion, LootProfileDigest,
			LootAlgorithmVersion, LootResultDigest))
		{
			return;
		}
		Object->SetStringField(TEXT("LootProfileId"), LootProfileId.ToString());
		Object->SetNumberField(TEXT("LootProfileVersion"), LootProfileVersion);
		Object->SetStringField(TEXT("LootProfileDigest"), LootProfileDigest);
		Object->SetStringField(TEXT("LootAlgorithmVersion"), LootAlgorithmVersion);
		Object->SetStringField(TEXT("LootResultDigest"), LootResultDigest);
	}

	bool ReadLootProfileProvenance(
		const TSharedPtr<FJsonObject>& Object,
		FName& OutLootProfileId,
		int32& OutLootProfileVersion,
		FString& OutLootProfileDigest,
		FString& OutLootAlgorithmVersion,
		FString& OutLootResultDigest,
		FString& OutError)
	{
		OutLootProfileId = NAME_None;
		OutLootProfileVersion = 0;
		OutLootProfileDigest.Reset();
		OutLootAlgorithmVersion.Reset();
		OutLootResultDigest.Reset();
		const bool bHasAnyProvenance = Object.IsValid()
			&& (Object->HasField(TEXT("LootProfileId")) || Object->HasField(TEXT("LootProfileVersion"))
				|| Object->HasField(TEXT("LootProfileDigest")) || Object->HasField(TEXT("LootAlgorithmVersion"))
				|| Object->HasField(TEXT("LootResultDigest")));
		if (!bHasAnyProvenance)
		{
			return true;
		}
		FString LootProfileId;
		double LootProfileVersion = 0.0;
		if (!Object->TryGetStringField(TEXT("LootProfileId"), LootProfileId)
			|| !Object->TryGetNumberField(TEXT("LootProfileVersion"), LootProfileVersion)
			|| !Object->TryGetStringField(TEXT("LootProfileDigest"), OutLootProfileDigest)
			|| !Object->TryGetStringField(TEXT("LootAlgorithmVersion"), OutLootAlgorithmVersion)
			|| !Object->TryGetStringField(TEXT("LootResultDigest"), OutLootResultDigest)
			|| LootProfileId.IsEmpty() || OutLootProfileDigest.IsEmpty()
			|| OutLootAlgorithmVersion.IsEmpty() || OutLootResultDigest.IsEmpty()
			|| !FMath::IsNearlyEqual(LootProfileVersion, FMath::RoundToDouble(LootProfileVersion))
			|| LootProfileVersion < 1.0 || LootProfileVersion > static_cast<double>(MAX_int32))
		{
			OutError = TEXT("Code B P16 Loot Profile provenance JSON is incomplete or invalid.");
			return false;
		}
		OutLootProfileId = FName(*LootProfileId);
		OutLootProfileVersion = static_cast<int32>(LootProfileVersion);
		return true;
	}

	bool HasLootProfileProvenance(
		const FName LootProfileId,
		const int32 LootProfileVersion,
		const FString& LootProfileDigest,
		const FString& LootAlgorithmVersion,
		const FString& LootResultDigest)
	{
		return !LootProfileId.IsNone() || LootProfileVersion != 0 || !LootProfileDigest.IsEmpty()
			|| !LootAlgorithmVersion.IsEmpty() || !LootResultDigest.IsEmpty();
	}

	TSharedRef<FJsonObject> NormalContainerMaterializationReceiptJson(
		const FCodeBNormalContainerMaterializationReceipt& Receipt)
	{
		TSharedRef<FJsonObject> Object = MakeShared<FJsonObject>();
		Object->SetStringField(TEXT("ReceiptId"), GuidText(Receipt.ReceiptId));
		Object->SetStringField(TEXT("OwnerId"), GuidText(Receipt.OwnerId));
		Object->SetStringField(TEXT("RunInstanceId"), GuidText(Receipt.RunInstanceId));
		Object->SetStringField(TEXT("SearchTargetId"), GuidText(Receipt.SearchTargetId));
		Object->SetStringField(TEXT("DefinitionId"), Receipt.DefinitionId.ToString());
		Object->SetStringField(TEXT("ContainerId"), GuidText(Receipt.ContainerId));
		Object->SetNumberField(TEXT("DefinitionContentRevision"), Receipt.DefinitionContentRevision);
		Object->SetStringField(TEXT("DefinitionDigest"), Receipt.DefinitionDigest);
		WriteLootProfileProvenance(Object, Receipt.LootProfileId, Receipt.LootProfileVersion,
			Receipt.LootProfileDigest, Receipt.LootAlgorithmVersion, Receipt.LootResultDigest);
		Object->SetStringField(TEXT("MaterializationDigest"), Receipt.MaterializationDigest);
		Object->SetStringField(TEXT("MaterializedUtc"), Receipt.MaterializedUtc);
		return Object;
	}

	bool JsonToNormalContainerMaterializationReceipt(
		const TSharedPtr<FJsonObject>& Object,
		FCodeBNormalContainerMaterializationReceipt& OutReceipt,
		FString& OutError)
	{
		OutReceipt = FCodeBNormalContainerMaterializationReceipt();
		double ContentRevision = 0.0;
		FString ReceiptId, OwnerId, RunInstanceId, SearchTargetId, DefinitionId, ContainerId;
		if (!Object.IsValid()
			|| !Object->TryGetStringField(TEXT("ReceiptId"), ReceiptId)
			|| !Object->TryGetStringField(TEXT("OwnerId"), OwnerId)
			|| !Object->TryGetStringField(TEXT("RunInstanceId"), RunInstanceId)
			|| !Object->TryGetStringField(TEXT("SearchTargetId"), SearchTargetId)
			|| !Object->TryGetStringField(TEXT("DefinitionId"), DefinitionId)
			|| !Object->TryGetStringField(TEXT("ContainerId"), ContainerId)
			|| !Object->TryGetNumberField(TEXT("DefinitionContentRevision"), ContentRevision)
			|| !Object->TryGetStringField(TEXT("DefinitionDigest"), OutReceipt.DefinitionDigest)
			|| !Object->TryGetStringField(TEXT("MaterializationDigest"), OutReceipt.MaterializationDigest)
			|| !Object->TryGetStringField(TEXT("MaterializedUtc"), OutReceipt.MaterializedUtc)
			|| !FMath::IsNearlyEqual(ContentRevision, FMath::RoundToDouble(ContentRevision))
			|| !TryGuidText(ReceiptId, OutReceipt.ReceiptId) || !OutReceipt.ReceiptId.IsValid()
			|| !TryGuidText(OwnerId, OutReceipt.OwnerId) || !OutReceipt.OwnerId.IsValid()
			|| !TryGuidText(RunInstanceId, OutReceipt.RunInstanceId) || !OutReceipt.RunInstanceId.IsValid()
			|| !TryGuidText(SearchTargetId, OutReceipt.SearchTargetId) || !OutReceipt.SearchTargetId.IsValid()
			|| !TryGuidText(ContainerId, OutReceipt.ContainerId) || !OutReceipt.ContainerId.IsValid()
			|| DefinitionId.IsEmpty() || OutReceipt.DefinitionDigest.IsEmpty()
			|| OutReceipt.MaterializationDigest.IsEmpty() || OutReceipt.MaterializedUtc.IsEmpty())
		{
			OutError = TEXT("Code B normal-container materialization receipt JSON is invalid.");
			return false;
		}
		OutReceipt.DefinitionId = FName(*DefinitionId);
		OutReceipt.DefinitionContentRevision = static_cast<int32>(ContentRevision);
		return OutReceipt.DefinitionContentRevision > 0
			&& ReadLootProfileProvenance(Object, OutReceipt.LootProfileId, OutReceipt.LootProfileVersion,
				OutReceipt.LootProfileDigest, OutReceipt.LootAlgorithmVersion, OutReceipt.LootResultDigest, OutError);
	}

	TSharedRef<FJsonObject> RunLocalNormalContainerJson(const FCodeBRunLocalNormalContainerRecord& Record)
	{
		TSharedRef<FJsonObject> Object = MakeShared<FJsonObject>();
		Object->SetNumberField(TEXT("SchemaVersion"), Record.SchemaVersion);
		Object->SetStringField(TEXT("OwnerId"), GuidText(Record.OwnerId));
		Object->SetStringField(TEXT("RunInstanceId"), GuidText(Record.RunInstanceId));
		Object->SetStringField(TEXT("SearchTargetId"), GuidText(Record.SearchTargetId));
		Object->SetStringField(TEXT("DefinitionId"), Record.DefinitionId.ToString());
		Object->SetStringField(TEXT("ContainerId"), GuidText(Record.ContainerId));
		Object->SetBoolField(TEXT("Materialized"), Record.bMaterialized);
		Object->SetStringField(TEXT("State"), NormalContainerStateText(Record.State));
		Object->SetStringField(TEXT("ActiveActionId"), GuidText(Record.ActiveActionId));
		Object->SetStringField(TEXT("ActiveSearchItemId"), GuidText(Record.ActiveSearchItemId));
		Object->SetNumberField(TEXT("Revision"), Record.Revision);
		Object->SetObjectField(TEXT("Receipt"), NormalContainerMaterializationReceiptJson(Record.Receipt));
		Object->SetObjectField(TEXT("ContainerSnapshot"), SnapshotJson(Record.ContainerSnapshot));
		TArray<TSharedPtr<FJsonValue>> Reveals;
		for (const FCodeBNormalContainerItemReveal& Reveal : Record.ItemRevealStates)
		{
			TSharedRef<FJsonObject> RevealObject = MakeShared<FJsonObject>();
			RevealObject->SetStringField(TEXT("ItemId"), GuidText(Reveal.ItemId));
			RevealObject->SetStringField(TEXT("State"), NormalContainerRevealStateText(Reveal.RevealState));
			Reveals.Add(MakeShared<FJsonValueObject>(RevealObject));
		}
		Object->SetArrayField(TEXT("ItemRevealStates"), Reveals);
		return Object;
	}

	bool JsonToRunLocalNormalContainer(
		const TSharedPtr<FJsonObject>& Object,
		FCodeBRunLocalNormalContainerRecord& OutRecord,
		FString& OutError)
	{
		OutRecord = FCodeBRunLocalNormalContainerRecord();
		double Schema = 0.0, Revision = 0.0;
		FString OwnerId, RunInstanceId, SearchTargetId, DefinitionId, ContainerId, State;
		const TSharedPtr<FJsonObject> *ReceiptObject = nullptr, *SnapshotObject = nullptr;
		const TArray<TSharedPtr<FJsonValue>>* Reveals = nullptr;
		if (!Object.IsValid()
			|| !Object->TryGetNumberField(TEXT("SchemaVersion"), Schema)
			|| !Object->TryGetStringField(TEXT("OwnerId"), OwnerId)
			|| !Object->TryGetStringField(TEXT("RunInstanceId"), RunInstanceId)
			|| !Object->TryGetStringField(TEXT("SearchTargetId"), SearchTargetId)
			|| !Object->TryGetStringField(TEXT("DefinitionId"), DefinitionId)
			|| !Object->TryGetStringField(TEXT("ContainerId"), ContainerId)
			|| !Object->TryGetBoolField(TEXT("Materialized"), OutRecord.bMaterialized)
			|| !Object->TryGetStringField(TEXT("State"), State)
			|| !Object->TryGetNumberField(TEXT("Revision"), Revision)
			|| !Object->TryGetObjectField(TEXT("Receipt"), ReceiptObject)
			|| !Object->TryGetObjectField(TEXT("ContainerSnapshot"), SnapshotObject)
			|| !Object->TryGetArrayField(TEXT("ItemRevealStates"), Reveals)
			|| !FMath::IsNearlyEqual(Schema, FMath::RoundToDouble(Schema))
			|| !FMath::IsNearlyEqual(Revision, FMath::RoundToDouble(Revision))
			|| (static_cast<int32>(Schema) != 1 && static_cast<int32>(Schema) != 2
				&& static_cast<int32>(Schema) != FCodeBRunLocalNormalContainerRecord::CurrentSchemaVersion)
			|| !TryGuidText(OwnerId, OutRecord.OwnerId) || !OutRecord.OwnerId.IsValid()
			|| !TryGuidText(RunInstanceId, OutRecord.RunInstanceId) || !OutRecord.RunInstanceId.IsValid()
			|| !TryGuidText(SearchTargetId, OutRecord.SearchTargetId) || !OutRecord.SearchTargetId.IsValid()
			|| !TryGuidText(ContainerId, OutRecord.ContainerId) || !OutRecord.ContainerId.IsValid()
			|| DefinitionId.IsEmpty() || !TryNormalContainerState(State, OutRecord.State)
			|| !JsonToNormalContainerMaterializationReceipt(*ReceiptObject, OutRecord.Receipt, OutError)
			|| !JsonToSnapshot(*SnapshotObject, OutRecord.ContainerSnapshot, OutError))
		{
			if (OutError.IsEmpty()) OutError = TEXT("Code B Run-local normal-container record JSON is invalid.");
			return false;
		}
		OutRecord.SchemaVersion = FCodeBRunLocalNormalContainerRecord::CurrentSchemaVersion;
		OutRecord.DefinitionId = FName(*DefinitionId);
		OutRecord.Revision = static_cast<int32>(Revision);
		if (static_cast<int32>(Schema) >= 2)
		{
			FString ActiveActionId;
			FString ActiveSearchItemId;
			if (!Object->TryGetStringField(TEXT("ActiveActionId"), ActiveActionId)
				|| !Object->TryGetStringField(TEXT("ActiveSearchItemId"), ActiveSearchItemId)
				|| !TryGuidText(ActiveActionId, OutRecord.ActiveActionId)
				|| !TryGuidText(ActiveSearchItemId, OutRecord.ActiveSearchItemId))
			{
				OutError = TEXT("Code B P10 normal-container action JSON is invalid.");
				return false;
			}
		}
		for (const TSharedPtr<FJsonValue>& Value : *Reveals)
		{
			const TSharedPtr<FJsonObject> RevealObject = Value.IsValid() ? Value->AsObject() : nullptr;
			FString ItemId, RevealState;
			FCodeBNormalContainerItemReveal Reveal;
			if (!RevealObject || !RevealObject->TryGetStringField(TEXT("ItemId"), ItemId)
				|| !RevealObject->TryGetStringField(TEXT("State"), RevealState)
				|| !TryGuidText(ItemId, Reveal.ItemId) || !Reveal.ItemId.IsValid()
				|| !TryNormalContainerRevealState(RevealState, Reveal.RevealState))
			{
				OutError = TEXT("Code B Run-local normal-container reveal JSON is invalid.");
				return false;
			}
			OutRecord.ItemRevealStates.Add(MoveTemp(Reveal));
		}
		return true;
	}

	bool ValidateRunLocalNormalContainerRecord(
		const FCodeBRunLocalNormalContainerRecord& Record,
		FString& OutError)
	{
		if (Record.SchemaVersion != FCodeBRunLocalNormalContainerRecord::CurrentSchemaVersion
			|| !Record.OwnerId.IsValid() || !Record.RunInstanceId.IsValid()
			|| !Record.SearchTargetId.IsValid() || Record.DefinitionId.IsNone()
			|| !Record.ContainerId.IsValid() || !Record.bMaterialized || Record.Revision < 1
			|| Record.Receipt.ReceiptId.IsValid() == false
			|| Record.Receipt.OwnerId != Record.OwnerId
			|| Record.Receipt.RunInstanceId != Record.RunInstanceId
			|| Record.Receipt.SearchTargetId != Record.SearchTargetId
			|| Record.Receipt.DefinitionId != Record.DefinitionId
			|| Record.Receipt.ContainerId != Record.ContainerId
			|| Record.Receipt.DefinitionContentRevision < 1
			|| Record.Receipt.DefinitionDigest.IsEmpty()
			|| Record.Receipt.MaterializationDigest.IsEmpty()
			|| Record.Receipt.MaterializedUtc.IsEmpty())
		{
			OutError = TEXT("Code B Run-local normal-container record identity or receipt is invalid.");
			return false;
		}
		const FCodeBNormalContainerDefinition* Definition = FindNormalContainerDefinitionInternal(Record.DefinitionId);
		if (!Definition || !ValidateNormalContainerDefinition(*Definition, OutError)
			|| Record.Receipt.DefinitionDigest != NormalContainerDefinitionDigest(*Definition)
			|| Record.Receipt.DefinitionContentRevision != Definition->ContentRevision)
		{
			if (OutError.IsEmpty()) OutError = TEXT("Code B Run-local normal-container definition receipt is invalid.");
			return false;
		}
		if (!ValidateLootProfileReceiptProvenance(Record.OwnerId, Record.RunInstanceId, Record.SearchTargetId,
			Record.DefinitionId, Definition->Capacity, FGuid(), Record.Receipt.LootProfileId,
			Record.Receipt.LootProfileVersion, Record.Receipt.LootProfileDigest,
			Record.Receipt.LootAlgorithmVersion, Record.Receipt.LootResultDigest, nullptr, OutError))
		{
			return false;
		}
		FCodeBRepository ValidationRepository;
		if (!ValidationRepository.LoadPersistedSnapshot(Record.ContainerSnapshot, &OutError))
		{
			return false;
		}
		const FCodeBContainer* Root = Record.ContainerSnapshot.Containers.Find(Record.ContainerId);
		if (!Root || Root->ContainerType != Definition->ContainerType
			|| Root->Kind != ECodeBContainerKind::Storage || Root->EquipmentSlot != ECodeBEquipSlot::None
			|| Root->Slots.Num() != Definition->Capacity)
		{
			OutError = TEXT("Code B Run-local normal-container root does not match its definition.");
			return false;
		}
		TSet<FGuid> VisitedContainers;
		TSet<FGuid> VisitedItems;
		TArray<FGuid> PendingContainers;
		PendingContainers.Add(Record.ContainerId);
		while (!PendingContainers.IsEmpty())
		{
			const FGuid CurrentContainerId = PendingContainers.Pop();
			if (VisitedContainers.Contains(CurrentContainerId))
			{
				OutError = TEXT("Code B Run-local normal-container graph contains a container cycle.");
				return false;
			}
			const FCodeBContainer* Container = Record.ContainerSnapshot.Containers.Find(CurrentContainerId);
			if (!Container)
			{
				OutError = TEXT("Code B Run-local normal-container graph is missing a child container.");
				return false;
			}
			VisitedContainers.Add(CurrentContainerId);
			for (const FGuid& ItemId : Container->Slots)
			{
				if (!ItemId.IsValid()) continue;
				const FCodeBItemInstance* Item = Record.ContainerSnapshot.Items.Find(ItemId);
				if (!Item || Item->ParentContainerId != CurrentContainerId || VisitedItems.Contains(ItemId))
				{
					OutError = TEXT("Code B Run-local normal-container graph has duplicate or invalid item placement.");
					return false;
				}
				VisitedItems.Add(ItemId);
				if (Item->ChildContainerId.IsValid())
				{
					PendingContainers.Add(Item->ChildContainerId);
				}
			}
		}
		if (VisitedContainers.Num() != Record.ContainerSnapshot.Containers.Num()
			|| VisitedItems.Num() != Record.ContainerSnapshot.Items.Num())
		{
			OutError = TEXT("Code B Run-local normal-container graph contains an orphan container or item.");
			return false;
		}
		TSet<FGuid> RevealedItems;
		int32 SearchingCount = 0;
		for (const FCodeBNormalContainerItemReveal& Reveal : Record.ItemRevealStates)
		{
			const bool bItemRemainsInContainer = Record.ContainerSnapshot.Items.Contains(Reveal.ItemId);
			if (RevealedItems.Contains(Reveal.ItemId)
				|| (Reveal.RevealState != ECodeBNormalContainerRevealState::Hidden
					&& Reveal.RevealState != ECodeBNormalContainerRevealState::Searching
					&& Reveal.RevealState != ECodeBNormalContainerRevealState::Revealed)
				|| (!bItemRemainsInContainer
					&& Reveal.RevealState != ECodeBNormalContainerRevealState::Revealed))
			{
				OutError = TEXT("Code B Run-local normal-container reveal state is invalid.");
				return false;
			}
			RevealedItems.Add(Reveal.ItemId);
			SearchingCount += Reveal.RevealState == ECodeBNormalContainerRevealState::Searching ? 1 : 0;
		}
		// Every materialized P9 item keeps its reveal record even after it has
		// moved to P6. A later P6-to-container deposit has no reveal record and
		// is intrinsically known; P10's composite-source validation is what proves
		// that it came from this same player's matched P6 snapshot.
		const bool bHasAction = Record.ActiveActionId.IsValid();
		const bool bHasSearchingItem = Record.ActiveSearchItemId.IsValid();
		const bool bOpeningState = Record.State == ECodeBNormalContainerState::Opening;
		const bool bOpenState = Record.State == ECodeBNormalContainerState::Open;
		if ((bOpeningState && (!bHasAction || bHasSearchingItem || SearchingCount != 0))
			|| (!bOpeningState && !bOpenState && (bHasAction || bHasSearchingItem || SearchingCount != 0))
			|| (bOpenState && ((bHasAction != bHasSearchingItem)
				|| (bHasSearchingItem && (SearchingCount != 1
					|| !Record.ContainerSnapshot.Items.Contains(Record.ActiveSearchItemId)))))
			|| (!bOpeningState && !bOpenState && Record.State != ECodeBNormalContainerState::Closed
				&& Record.State != ECodeBNormalContainerState::Interrupted))
		{
			OutError = TEXT("Code B P10 normal-container action state is invalid.");
			return false;
		}
		// P9 persisted the initial graph digest.  It remains directly re-checkable
		// while untouched; P10 intentionally preserves the immutable receipt while
		// later revealed items may leave this Run-local snapshot through one atomic
		// P6+P9 commit.
		if (Record.State == ECodeBNormalContainerState::Closed
			&& !bHasAction && !bHasSearchingItem && SearchingCount == 0)
		{
			bool bAllHidden = true;
			for (const FCodeBNormalContainerItemReveal& Reveal : Record.ItemRevealStates)
			{
				bAllHidden &= Reveal.RevealState == ECodeBNormalContainerRevealState::Hidden;
			}
			if (bAllHidden && Record.Receipt.MaterializationDigest != NormalContainerMaterializationDigest(Record))
			{
				OutError = TEXT("Code B Run-local normal-container initial materialization digest is invalid.");
				return false;
			}
			if (bAllHidden && !ValidateInitialLootProfileGraph(Record.OwnerId, Record.RunInstanceId,
				Record.SearchTargetId, Record.DefinitionId, Definition->Capacity, FGuid(),
				Record.Receipt.LootProfileId, Record.Receipt.LootProfileVersion, Record.Receipt.LootProfileDigest,
				Record.Receipt.LootAlgorithmVersion, Record.Receipt.LootResultDigest, Record.ContainerId,
				Record.ContainerSnapshot, OutError))
			{
				return false;
			}
		}
		return true;
	}

	bool BuildNormalContainerProjection(
		const FCodeBRunLocalNormalContainerRecord& Record,
		FCodeBNormalContainerProjection& OutProjection,
		FString& OutError)
	{
		if (!ValidateRunLocalNormalContainerRecord(Record, OutError))
		{
			return false;
		}
		OutProjection = FCodeBNormalContainerProjection();
		OutProjection.OwnerId = Record.OwnerId;
		OutProjection.RunInstanceId = Record.RunInstanceId;
		OutProjection.SearchTargetId = Record.SearchTargetId;
		OutProjection.DefinitionId = Record.DefinitionId;
		OutProjection.ContainerId = Record.ContainerId;
		OutProjection.State = Record.State;
		OutProjection.ActiveActionId = Record.ActiveActionId;
		OutProjection.ActiveSearchItemId = Record.ActiveSearchItemId;
		OutProjection.Revision = Record.Revision;
		OutProjection.Receipt = Record.Receipt;
		TMap<FGuid, ECodeBNormalContainerRevealState> RevealByItemId;
		for (const FCodeBNormalContainerItemReveal& Reveal : Record.ItemRevealStates)
		{
			RevealByItemId.Add(Reveal.ItemId, Reveal.RevealState);
		}
		for (const TPair<FGuid, FCodeBItemInstance>& Pair : Record.ContainerSnapshot.Items)
		{
			const FCodeBItemInstance& Item = Pair.Value;
			FCodeBNormalContainerItemProjection& ProjectionItem = OutProjection.Items.AddDefaulted_GetRef();
			ProjectionItem.ItemId = Item.ItemId;
			ProjectionItem.DefinitionId = Item.DefinitionId;
			ProjectionItem.Quantity = Item.Quantity;
			ProjectionItem.ParentContainerId = Item.ParentContainerId;
			ProjectionItem.SlotIndex = Item.SlotIndex;
			ProjectionItem.ChildContainerId = Item.ChildContainerId;
			const ECodeBNormalContainerRevealState* RevealState = RevealByItemId.Find(Item.ItemId);
			ProjectionItem.RevealState = RevealState
				? *RevealState
				: ECodeBNormalContainerRevealState::Revealed;
		}
		OutProjection.Items.Sort([](const FCodeBNormalContainerItemProjection& Left, const FCodeBNormalContainerItemProjection& Right)
		{
			return Left.ItemId.ToString(EGuidFormats::DigitsWithHyphensLower) < Right.ItemId.ToString(EGuidFormats::DigitsWithHyphensLower);
		});
		return true;
	}

	const TArray<FCodeBBodyContainerDefinition>& BodyContainerDefinitions()
	{
		static const TArray<FCodeBBodyContainerDefinition> Definitions = []()
		{
			FCodeBBodyContainerDefinition BasicCorpse;
			BasicCorpse.DefinitionId = FName(TEXT("CodeB.BodyContainer.BasicCorpse"));
			BasicCorpse.ContainerType = FName(TEXT("CodeB.BodyContainer.BasicCorpse"));
			BasicCorpse.Capacity = 2;
			BasicCorpse.ContentRevision = 1;
			BasicCorpse.ContentPlan = {
				{ Fdemo_mapItemIds::IronShard, 1, 0, 0 },
				{ Fdemo_mapItemIds::SpiritDust, 1, 1, 0 }
			};
			return TArray<FCodeBBodyContainerDefinition> { MoveTemp(BasicCorpse) };
		}();
		return Definitions;
	}

	const TArray<FCodeBLootProfile>& LootProfiles()
	{
		static const TArray<FCodeBLootProfile> Profiles = []()
		{
			FCodeBLootProfile BasicCacheR1;
			BasicCacheR1.LootProfileId = FName(TEXT("CodeB.LootProfile.BasicCache.r1"));
			BasicCacheR1.SourceContainerDefinitionId = FName(TEXT("CodeB.NormalContainer.BasicCache"));
			BasicCacheR1.ProfileVersion = 1;
			BasicCacheR1.AlgorithmVersion = TEXT("CodeB.DeterministicWeightedLoot.Crc32.r1");
			FCodeBLootProfileRollGroup CacheGuaranteed;
			CacheGuaranteed.GroupId = FName(TEXT("Guaranteed.Main"));
			CacheGuaranteed.SelectionCount = 1;
			CacheGuaranteed.GroupOrder = 0;
			CacheGuaranteed.Candidates = {
				{ FName(TEXT("SpiritDust")), Fdemo_mapItemIds::SpiritDust, 3, 2, 3, 0 },
				{ FName(TEXT("IronShard")), Fdemo_mapItemIds::IronShard, 2, 1, 2, 1 }
			};
			BasicCacheR1.RollGroups = { CacheGuaranteed };

			// P18 only selects this successor for an unmaterialized BasicCache.  The
			// retained r1 profile is selected exclusively by an existing receipt.
			FCodeBLootProfile BasicCacheR2;
			BasicCacheR2.LootProfileId = FName(TEXT("CodeB.LootProfile.BasicCache.r2"));
			BasicCacheR2.SourceContainerDefinitionId = FName(TEXT("CodeB.NormalContainer.BasicCache"));
			BasicCacheR2.ProfileVersion = 2;
			BasicCacheR2.AlgorithmVersion = TEXT("CodeB.DeterministicWeightedLoot.Crc32.r2");
			BasicCacheR2.RollGroups = { CacheGuaranteed };
			FCodeBLootProfileRollGroup SpatialUtility;
			SpatialUtility.GroupId = FName(TEXT("Optional.SpatialUtility"));
			SpatialUtility.SelectionCount = 1;
			SpatialUtility.bOptional = true;
			SpatialUtility.NoDropWeight = 9;
			SpatialUtility.SpawnWeight = 1;
			SpatialUtility.GroupOrder = 1;
			SpatialUtility.Candidates = {
				{ FName(TEXT("WindTalisman")), Fdemo_mapItemIds::WindTalisman, 1, 1, 1, 0 },
				{ FName(TEXT("BackpackLevel1")), Fdemo_mapItemIds::BackpackLevel1, 3, 1, 1, 1 }
			};
			BasicCacheR2.RollGroups.Add(MoveTemp(SpatialUtility));

			FCodeBLootProfile BasicCorpse;
			BasicCorpse.LootProfileId = FName(TEXT("CodeB.LootProfile.BasicCorpse.r1"));
			BasicCorpse.SourceContainerDefinitionId = FName(TEXT("CodeB.BodyContainer.BasicCorpse"));
			BasicCorpse.ProfileVersion = 1;
			BasicCorpse.AlgorithmVersion = TEXT("CodeB.DeterministicWeightedLoot.Crc32.r1");
			FCodeBLootProfileRollGroup CorpseGuaranteed;
			CorpseGuaranteed.GroupId = FName(TEXT("Guaranteed.Main"));
			CorpseGuaranteed.SelectionCount = 1;
			CorpseGuaranteed.GroupOrder = 0;
			CorpseGuaranteed.Candidates = {
				{ FName(TEXT("IronShard")), Fdemo_mapItemIds::IronShard, 3, 1, 2, 0 },
				{ FName(TEXT("SpiritDust")), Fdemo_mapItemIds::SpiritDust, 2, 1, 1, 1 }
			};
			BasicCorpse.RollGroups = { CorpseGuaranteed };

			// P20 selects this successor only while the one BasicCorpse has not yet
			// materialized. Existing P11 receipts continue to resolve their r1 profile.
			FCodeBLootProfile BasicCorpseR2;
			BasicCorpseR2.LootProfileId = FName(TEXT("CodeB.LootProfile.BasicCorpse.r2"));
			BasicCorpseR2.SourceContainerDefinitionId = FName(TEXT("CodeB.BodyContainer.BasicCorpse"));
			BasicCorpseR2.ProfileVersion = 2;
			BasicCorpseR2.AlgorithmVersion = TEXT("CodeB.DeterministicWeightedLoot.Crc32.r2");
			BasicCorpseR2.RollGroups = { CorpseGuaranteed };
			FCodeBLootProfileRollGroup CorpseSpatialUtility;
			CorpseSpatialUtility.GroupId = FName(TEXT("Optional.SpatialUtility"));
			CorpseSpatialUtility.SelectionCount = 1;
			CorpseSpatialUtility.bOptional = true;
			CorpseSpatialUtility.NoDropWeight = 9;
			CorpseSpatialUtility.SpawnWeight = 1;
			CorpseSpatialUtility.GroupOrder = 1;
			CorpseSpatialUtility.Candidates = {
				{ FName(TEXT("WindTalisman")), Fdemo_mapItemIds::WindTalisman, 1, 1, 1, 0 },
				{ FName(TEXT("BackpackLevel1")), Fdemo_mapItemIds::BackpackLevel1, 3, 1, 1, 1 }
			};
			BasicCorpseR2.RollGroups.Add(CorpseSpatialUtility);

			// P21 selects r3 only before the one BasicCorpse is first materialized.
			// Its first two groups intentionally copy r2 without changing weights,
			// candidates, quantity ranges, order, or optional-gate semantics.
			FCodeBLootProfile BasicCorpseR3;
			BasicCorpseR3.LootProfileId = FName(TEXT("CodeB.LootProfile.BasicCorpse.r3"));
			BasicCorpseR3.SourceContainerDefinitionId = FName(TEXT("CodeB.BodyContainer.BasicCorpse"));
			BasicCorpseR3.ProfileVersion = 3;
			BasicCorpseR3.AlgorithmVersion = TEXT("CodeB.DeterministicWeightedLoot.Crc32.r3");
			BasicCorpseR3.RollGroups = { CorpseGuaranteed, CorpseSpatialUtility };
			FCodeBLootProfileRollGroup CorpseEquippedLoadout;
			CorpseEquippedLoadout.GroupId = FName(TEXT("Optional.EquippedLoadout"));
			CorpseEquippedLoadout.SelectionCount = 1;
			CorpseEquippedLoadout.bOptional = true;
			CorpseEquippedLoadout.NoDropWeight = 4;
			CorpseEquippedLoadout.SpawnWeight = 1;
			CorpseEquippedLoadout.GroupOrder = 2;
			CorpseEquippedLoadout.Candidates = {
				{ FName(TEXT("Body.Accessory0")), Fdemo_mapItemIds::EvasionCharm, 1, 1, 1, 0 },
				{ FName(TEXT("Body.ArmorRobe")), Fdemo_mapItemIds::ReinforcedVest, 1, 1, 1, 1 },
				{ FName(TEXT("Body.Weapon")), Fdemo_mapItemIds::HeavyPracticeBlade, 1, 1, 1, 2 }
			};
			BasicCorpseR3.RollGroups.Add(MoveTemp(CorpseEquippedLoadout));
			return TArray<FCodeBLootProfile> { MoveTemp(BasicCacheR1), MoveTemp(BasicCacheR2), MoveTemp(BasicCorpse), MoveTemp(BasicCorpseR2), MoveTemp(BasicCorpseR3) };
		}();
		return Profiles;
	}

	const FCodeBLootProfile* FindLootProfileInternal(const FName SourceContainerDefinitionId)
	{
		const FCodeBLootProfile* Latest = nullptr;
		for (const FCodeBLootProfile& Profile : LootProfiles())
		{
			if (Profile.SourceContainerDefinitionId == SourceContainerDefinitionId
				&& (!Latest || Profile.ProfileVersion > Latest->ProfileVersion))
			{
				Latest = &Profile;
			}
		}
		return Latest;
	}

	const FCodeBLootProfile* FindLootProfileByProvenance(
		const FName SourceContainerDefinitionId,
		const FName LootProfileId,
		const int32 LootProfileVersion)
	{
		return LootProfiles().FindByPredicate([SourceContainerDefinitionId, LootProfileId, LootProfileVersion](const FCodeBLootProfile& Profile)
		{
			return Profile.SourceContainerDefinitionId == SourceContainerDefinitionId
				&& Profile.LootProfileId == LootProfileId
				&& Profile.ProfileVersion == LootProfileVersion;
		});
	}

	FString LootProfileDigest(const FCodeBLootProfile& Profile)
	{
		TArray<FString> Pieces;
		Pieces.Add(FString::Printf(TEXT("profile:%s:%s:%d:%s"), *Profile.LootProfileId.ToString(),
			*Profile.SourceContainerDefinitionId.ToString(), Profile.ProfileVersion, *Profile.AlgorithmVersion));
		TArray<const FCodeBLootProfileRollGroup*> Groups;
		for (const FCodeBLootProfileRollGroup& Group : Profile.RollGroups) Groups.Add(&Group);
		Groups.Sort([](const FCodeBLootProfileRollGroup& Left, const FCodeBLootProfileRollGroup& Right)
		{
			return Left.GroupOrder != Right.GroupOrder
				? Left.GroupOrder < Right.GroupOrder
				: Left.GroupId.LexicalLess(Right.GroupId);
		});
		for (const FCodeBLootProfileRollGroup* Group : Groups)
		{
			// Keep P16's r1 byte-for-byte digest input intact.  Only the r2
			// optional group serializes its explicit no-drop gate into the digest.
			Pieces.Add(Group->NoDropWeight == 0 && Group->SpawnWeight == 0
				? FString::Printf(TEXT("group:%s:%d:%d:%d"), *Group->GroupId.ToString(),
					Group->SelectionCount, Group->bOptional ? 1 : 0, Group->GroupOrder)
				: FString::Printf(TEXT("group:%s:%d:%d:%d:%d:%d"), *Group->GroupId.ToString(),
					Group->SelectionCount, Group->bOptional ? 1 : 0, Group->NoDropWeight, Group->SpawnWeight, Group->GroupOrder));
			TArray<const FCodeBLootProfileCandidate*> Candidates;
			for (const FCodeBLootProfileCandidate& Candidate : Group->Candidates) Candidates.Add(&Candidate);
			Candidates.Sort([](const FCodeBLootProfileCandidate& Left, const FCodeBLootProfileCandidate& Right)
			{
				return Left.TieBreakOrder != Right.TieBreakOrder
					? Left.TieBreakOrder < Right.TieBreakOrder
					: Left.EntryId.LexicalLess(Right.EntryId);
			});
			for (const FCodeBLootProfileCandidate* Candidate : Candidates)
			{
				Pieces.Add(FString::Printf(TEXT("candidate:%s:%s:%d:%d:%d:%d"),
					*Candidate->EntryId.ToString(), *Candidate->ItemDefinitionId.ToString(), Candidate->Weight,
					Candidate->MinQuantity, Candidate->MaxQuantity, Candidate->TieBreakOrder));
			}
		}
		return FString::Printf(TEXT("%08X"), FCrc::StrCrc32(*FString::Join(Pieces, TEXT("|"))));
	}

	bool ValidateLootProfile(const FCodeBLootProfile& Profile, const int32 SourceCapacity, FString& OutError)
	{
		if (Profile.LootProfileId.IsNone() || Profile.SourceContainerDefinitionId.IsNone()
			|| Profile.ProfileVersion < 1 || Profile.AlgorithmVersion.IsEmpty() || SourceCapacity < 1)
		{
			OutError = TEXT("Code B P16 Loot Profile identity is invalid.");
			return false;
		}
		const bool bP21R3 = IsP21BasicCorpseR3(Profile);
		if (bP21R3 && !ValidateP21BodyEquipmentCatalog(OutError))
		{
			return false;
		}
		TSet<FName> GroupIds;
		TSet<int32> GroupOrders;
		int32 OptionalGroupCount = 0;
		int32 MaximumOutputCount = 0;
		for (const FCodeBLootProfileRollGroup& Group : Profile.RollGroups)
		{
			if (Group.GroupId.IsNone() || Group.SelectionCount < 1 || Group.GroupOrder < 0
				|| Group.Candidates.IsEmpty() || Group.SelectionCount > Group.Candidates.Num()
				|| Group.NoDropWeight < 0 || Group.SpawnWeight < 0
				|| (Group.bOptional && (Group.NoDropWeight < 1 || Group.SpawnWeight < 1))
				|| (!Group.bOptional && (Group.NoDropWeight != 0 || Group.SpawnWeight != 0))
				|| GroupIds.Contains(Group.GroupId) || GroupOrders.Contains(Group.GroupOrder))
			{
				OutError = TEXT("Code B P16 Loot Profile roll-group identity or selection count is invalid.");
				return false;
			}
			GroupIds.Add(Group.GroupId);
			GroupOrders.Add(Group.GroupOrder);
			OptionalGroupCount += Group.bOptional ? 1 : 0;
			MaximumOutputCount += Group.GroupId == FName(TEXT("Optional.EquippedLoadout")) ? 0 : Group.SelectionCount;
			TSet<FName> EntryIds;
			TSet<int32> TieBreakOrders;
			for (const FCodeBLootProfileCandidate& Candidate : Group.Candidates)
			{
				FCodeBItemDefinition ItemDefinition;
				if (Candidate.EntryId.IsNone() || Candidate.ItemDefinitionId.IsNone() || Candidate.Weight < 1
					|| Candidate.MinQuantity < 1 || Candidate.MaxQuantity < Candidate.MinQuantity
					|| Candidate.TieBreakOrder < 0 || EntryIds.Contains(Candidate.EntryId)
					|| TieBreakOrders.Contains(Candidate.TieBreakOrder)
					|| !BuildCanonicalCodeBItemDefinition(Candidate.ItemDefinitionId, ItemDefinition, OutError))
				{
					if (OutError.IsEmpty()) OutError = TEXT("Code B P16 Loot Profile candidate is unknown or has invalid identity.");
					return false;
				}
				const bool bSpatialUtilityCandidate = Group.bOptional
					&& Group.GroupId == FName(TEXT("Optional.SpatialUtility"))
					&& (ItemDefinition.ItemType == ECodeBItemType::SpatialItem
						|| ItemDefinition.ItemType == ECodeBItemType::Backpack);
				const bool bMaterialOrConsumable = ItemDefinition.ItemType == ECodeBItemType::Material
					|| ItemDefinition.ItemType == ECodeBItemType::Consumable;
				const FP21BodyEquipmentSlot* EquipmentSlot = bP21R3
					&& Group.GroupId == FName(TEXT("Optional.EquippedLoadout"))
					? FindP21BodyEquipmentSlot(Candidate.ItemDefinitionId) : nullptr;
				const bool bEquipmentCandidate = EquipmentSlot != nullptr;
				if ((!ItemDefinition.bStackable && (Candidate.MinQuantity != 1 || Candidate.MaxQuantity != 1))
					|| Candidate.MaxQuantity > ItemDefinition.MaxStack
					|| (!bSpatialUtilityCandidate && !bEquipmentCandidate && ItemDefinition.EquipSlot != ECodeBEquipSlot::None)
					|| (!bMaterialOrConsumable && !bSpatialUtilityCandidate && !bEquipmentCandidate))
				{
					OutError = TEXT("Code B P16 Loot Profile candidate is invalid or cannot fit its source.");
					return false;
				}
				if (bSpatialUtilityCandidate
					&& (Candidate.MinQuantity != 1 || Candidate.MaxQuantity != 1
						|| ItemDefinition.SpatialContainerSemantic == ECodeBSpatialContainerSemantic::None
						|| ItemDefinition.ChildContainerCapacity < 1))
				{
					OutError = TEXT("Code B P18 spatial-utility candidate is not a canonical empty child-container parent.");
					return false;
				}
				if (bEquipmentCandidate
					&& (Candidate.Weight != 1 || Candidate.MinQuantity != 1 || Candidate.MaxQuantity != 1
						|| ItemDefinition.bStackable || ItemDefinition.MaxStack != 1
						|| ItemDefinition.EquipSlot != EquipmentSlot->EquipmentSlot
						|| ItemDefinition.SpatialContainerSemantic != ECodeBSpatialContainerSemantic::None
						|| ItemDefinition.ChildContainerCapacity != 0))
				{
					OutError = TEXT("Code B P21 equipment candidate is not one formal simple equipment Definition.");
					return false;
				}
				EntryIds.Add(Candidate.EntryId);
				TieBreakOrders.Add(Candidate.TieBreakOrder);
			}
			if (Group.bOptional && Group.GroupId == FName(TEXT("Optional.SpatialUtility"))
				&& (Group.SelectionCount != 1 || Group.NoDropWeight != 9 || Group.SpawnWeight != 1 || Group.Candidates.Num() != 2
					|| Group.Candidates[0].EntryId != FName(TEXT("WindTalisman"))
					|| Group.Candidates[0].ItemDefinitionId != Fdemo_mapItemIds::WindTalisman
					|| Group.Candidates[0].Weight != 1
					|| Group.Candidates[1].EntryId != FName(TEXT("BackpackLevel1"))
					|| Group.Candidates[1].ItemDefinitionId != Fdemo_mapItemIds::BackpackLevel1
					|| Group.Candidates[1].Weight != 3))
			{
				OutError = TEXT("Code B P18 Optional.SpatialUtility configuration is not the closed 9:1 canonical catalog.");
				return false;
			}
			if (Group.bOptional && Group.GroupId == FName(TEXT("Optional.EquippedLoadout"))
				&& (!bP21R3 || Group.SelectionCount != 1 || Group.NoDropWeight != 4 || Group.SpawnWeight != 1
					|| Group.GroupOrder != 2 || Group.Candidates.Num() != 3
					|| Group.Candidates[0].EntryId != FName(TEXT("Body.Accessory0"))
					|| Group.Candidates[0].ItemDefinitionId != Fdemo_mapItemIds::EvasionCharm
					|| Group.Candidates[0].Weight != 1 || Group.Candidates[0].MinQuantity != 1
					|| Group.Candidates[0].MaxQuantity != 1 || Group.Candidates[0].TieBreakOrder != 0
					|| Group.Candidates[1].EntryId != FName(TEXT("Body.ArmorRobe"))
					|| Group.Candidates[1].ItemDefinitionId != Fdemo_mapItemIds::ReinforcedVest
					|| Group.Candidates[1].Weight != 1 || Group.Candidates[1].MinQuantity != 1
					|| Group.Candidates[1].MaxQuantity != 1 || Group.Candidates[1].TieBreakOrder != 1
					|| Group.Candidates[2].EntryId != FName(TEXT("Body.Weapon"))
					|| Group.Candidates[2].ItemDefinitionId != Fdemo_mapItemIds::HeavyPracticeBlade
					|| Group.Candidates[2].Weight != 1 || Group.Candidates[2].MinQuantity != 1
					|| Group.Candidates[2].MaxQuantity != 1 || Group.Candidates[2].TieBreakOrder != 2))
			{
				OutError = TEXT("Code B P21 Optional.EquippedLoadout is not the closed 4:1 formal candidate catalog.");
				return false;
			}
			if (Group.bOptional && Group.GroupId != FName(TEXT("Optional.SpatialUtility"))
				&& Group.GroupId != FName(TEXT("Optional.EquippedLoadout")))
			{
				OutError = TEXT("Code B Loot Profile contains an unsupported optional group.");
				return false;
			}
		}
		if (Profile.RollGroups.IsEmpty() || OptionalGroupCount > (bP21R3 ? 2 : 1) || MaximumOutputCount > SourceCapacity)
		{
			OutError = TEXT("Code B P16 Loot Profile group count exceeds the source capacity or optional-group limit.");
			return false;
		}
		return true;
	}

	bool ValidateLootProfileCatalog(FString& OutError)
	{
		TSet<FName> ProfileIds;
		TSet<FName> SourceDefinitionIds;
		for (const FCodeBLootProfile& Profile : LootProfiles())
		{
			const FCodeBNormalContainerDefinition* NormalDefinition = FindNormalContainerDefinitionInternal(Profile.SourceContainerDefinitionId);
			const FCodeBBodyContainerDefinition* BodyDefinition = FindBodyContainerDefinitionInternal(Profile.SourceContainerDefinitionId);
			const int32 Capacity = NormalDefinition ? NormalDefinition->Capacity : (BodyDefinition ? BodyDefinition->Capacity : 0);
			if (ProfileIds.Contains(Profile.LootProfileId)
				|| !ValidateLootProfile(Profile, Capacity, OutError))
			{
				if (OutError.IsEmpty()) OutError = TEXT("Code B P16 Loot Profile catalog contains duplicate profile or source identities.");
				return false;
			}
			ProfileIds.Add(Profile.LootProfileId);
			SourceDefinitionIds.Add(Profile.SourceContainerDefinitionId);
		}
		return ProfileIds.Num() == 5 && SourceDefinitionIds.Num() == 2
			&& FindLootProfileByProvenance(FName(TEXT("CodeB.NormalContainer.BasicCache")),
				FName(TEXT("CodeB.LootProfile.BasicCache.r1")), 1)
			&& FindLootProfileByProvenance(FName(TEXT("CodeB.NormalContainer.BasicCache")),
				FName(TEXT("CodeB.LootProfile.BasicCache.r2")), 2)
			&& FindLootProfileByProvenance(FName(TEXT("CodeB.BodyContainer.BasicCorpse")),
				FName(TEXT("CodeB.LootProfile.BasicCorpse.r1")), 1)
			&& FindLootProfileByProvenance(FName(TEXT("CodeB.BodyContainer.BasicCorpse")),
				FName(TEXT("CodeB.LootProfile.BasicCorpse.r2")), 2)
			&& FindLootProfileByProvenance(FName(TEXT("CodeB.BodyContainer.BasicCorpse")),
				FName(TEXT("CodeB.LootProfile.BasicCorpse.r3")), 3);
	}

	FString LootRollIdentityText(
		const FGuid& OwnerId,
		const FGuid& RunInstanceId,
		const FGuid& TargetId,
		const FName SourceContainerDefinitionId,
		const FGuid& DeathReceiptId,
		const FCodeBLootProfile& Profile)
	{
		FString Identity = FString::Printf(TEXT("owner:%s|run:%s|target:%s|definition:%s|death:%s|profile:%s|version:%d|digest:%s|algorithm:%s"),
			*GuidText(OwnerId), *GuidText(RunInstanceId), *GuidText(TargetId), *SourceContainerDefinitionId.ToString(),
			*GuidText(DeathReceiptId), *Profile.LootProfileId.ToString(), Profile.ProfileVersion,
			*LootProfileDigest(Profile), *Profile.AlgorithmVersion);
		if (IsP21BasicCorpseR3(Profile))
		{
			Identity += FString::Printf(TEXT("|equipment-candidates:%s"), *P21EquipmentCandidateSetDigest());
		}
		return Identity;
	}

	FGuid StableLootGraphGuid(const FString& LootIdentity, const TCHAR* EntityKind, const int32 StableOrdinal)
	{
		const FString Seed = FString::Printf(TEXT("%s|entity:%s|ordinal:%d"), *LootIdentity, EntityKind, StableOrdinal);
		return FGuid(
			FCrc::StrCrc32(*(Seed + TEXT("|A"))), FCrc::StrCrc32(*(Seed + TEXT("|B"))),
			FCrc::StrCrc32(*(Seed + TEXT("|C"))), FCrc::StrCrc32(*(Seed + TEXT("|D"))));
	}

	bool BuildDeterministicLootProfileRoll(
		const FGuid& OwnerId,
		const FGuid& RunInstanceId,
		const FGuid& TargetId,
		const FName SourceContainerDefinitionId,
		const int32 SourceCapacity,
		const FGuid& DeathReceiptId,
		FCodeBLootProfileRollResult& OutRoll,
		FString& OutError)
	{
		const FCodeBLootProfile* Profile = FindLootProfileInternal(SourceContainerDefinitionId);
		if (!Profile)
		{
			OutRoll = FCodeBLootProfileRollResult();
			OutError = TEXT("Code B P16 cannot resolve the latest valid Loot Profile for this source.");
			return false;
		}
		const bool bP21R3 = IsP21BasicCorpseR3(*Profile);
		if (bP21R3 && !ValidateP21BodyEquipmentCatalog(OutError))
		{
			return false;
		}
		return BuildDeterministicLootProfileRollForProfile(OwnerId, RunInstanceId, TargetId,
			SourceContainerDefinitionId, SourceCapacity, DeathReceiptId, *Profile, OutRoll, OutError);
	}

	bool BuildDeterministicLootProfileRollForProfile(
		const FGuid& OwnerId,
		const FGuid& RunInstanceId,
		const FGuid& TargetId,
		const FName SourceContainerDefinitionId,
		const int32 SourceCapacity,
		const FGuid& DeathReceiptId,
		const FCodeBLootProfile& Profile,
		FCodeBLootProfileRollResult& OutRoll,
		FString& OutError)
	{
		OutRoll = FCodeBLootProfileRollResult();
		if (!OwnerId.IsValid() || !RunInstanceId.IsValid() || !TargetId.IsValid()
			|| Profile.SourceContainerDefinitionId != SourceContainerDefinitionId
			|| !ValidateLootProfileCatalog(OutError) || !ValidateLootProfile(Profile, SourceCapacity, OutError))
		{
			if (OutError.IsEmpty()) OutError = TEXT("Code B P16 cannot resolve one valid Loot Profile for this source.");
			return false;
		}
		const FString Identity = LootRollIdentityText(OwnerId, RunInstanceId, TargetId,
			SourceContainerDefinitionId, DeathReceiptId, Profile);
		OutRoll.LootProfileId = Profile.LootProfileId;
		OutRoll.ProfileVersion = Profile.ProfileVersion;
		OutRoll.ProfileDigest = LootProfileDigest(Profile);
		OutRoll.AlgorithmVersion = Profile.AlgorithmVersion;
		TArray<const FCodeBLootProfileRollGroup*> Groups;
		for (const FCodeBLootProfileRollGroup& Group : Profile.RollGroups) Groups.Add(&Group);
		Groups.Sort([](const FCodeBLootProfileRollGroup& Left, const FCodeBLootProfileRollGroup& Right)
		{
			return Left.GroupOrder != Right.GroupOrder
				? Left.GroupOrder < Right.GroupOrder
				: Left.GroupId.LexicalLess(Right.GroupId);
		});
		for (const FCodeBLootProfileRollGroup* Group : Groups)
		{
			if (Group->bOptional)
			{
				const int64 GateWeight = static_cast<int64>(Group->NoDropWeight) + Group->SpawnWeight;
				if (GateWeight < 1 || GateWeight > MAX_int32)
				{
					OutError = TEXT("Code B P18 Optional.SpatialUtility gate weight is invalid.");
					return false;
				}
				const uint32 GateDraw = FCrc::StrCrc32(*FString::Printf(
					TEXT("%s|optional-gate:%d"), *Identity, Group->GroupOrder));
				if (static_cast<int64>(GateDraw % static_cast<uint32>(GateWeight)) < Group->NoDropWeight)
				{
					continue;
				}
			}
			TArray<const FCodeBLootProfileCandidate*> Remaining;
			for (const FCodeBLootProfileCandidate& Candidate : Group->Candidates) Remaining.Add(&Candidate);
			Remaining.Sort([](const FCodeBLootProfileCandidate& Left, const FCodeBLootProfileCandidate& Right)
			{
				return Left.TieBreakOrder != Right.TieBreakOrder
					? Left.TieBreakOrder < Right.TieBreakOrder
					: Left.EntryId.LexicalLess(Right.EntryId);
			});
			for (int32 SelectionOrdinal = 0; SelectionOrdinal < Group->SelectionCount; ++SelectionOrdinal)
			{
				int64 TotalWeight = 0;
				for (const FCodeBLootProfileCandidate* Candidate : Remaining) TotalWeight += Candidate->Weight;
				if (TotalWeight < 1 || TotalWeight > MAX_int32)
				{
					OutError = TEXT("Code B P16 Loot Profile total candidate weight is invalid.");
					return false;
				}
				const uint32 Draw = FCrc::StrCrc32(*FString::Printf(TEXT("%s|select:%d:%d"), *Identity, Group->GroupOrder, SelectionOrdinal));
				int64 Threshold = static_cast<int64>(Draw % static_cast<uint32>(TotalWeight));
				int32 SelectedIndex = INDEX_NONE;
				for (int32 CandidateIndex = 0; CandidateIndex < Remaining.Num(); ++CandidateIndex)
				{
					if (Threshold < Remaining[CandidateIndex]->Weight)
					{
						SelectedIndex = CandidateIndex;
						break;
					}
					Threshold -= Remaining[CandidateIndex]->Weight;
				}
				if (!Remaining.IsValidIndex(SelectedIndex))
				{
					OutError = TEXT("Code B P16 deterministic weighted selection could not resolve a candidate.");
					return false;
				}
				const FCodeBLootProfileCandidate* Selected = Remaining[SelectedIndex];
				const int32 QuantitySpan = Selected->MaxQuantity - Selected->MinQuantity + 1;
				const uint32 QuantityDraw = FCrc::StrCrc32(*FString::Printf(TEXT("%s|quantity:%d:%d"), *Identity, Group->GroupOrder, SelectionOrdinal));
				FCodeBLootProfileRollEntry& Entry = OutRoll.Entries.AddDefaulted_GetRef();
				Entry.ItemDefinitionId = Selected->ItemDefinitionId;
				Entry.Quantity = Selected->MinQuantity + static_cast<int32>(QuantityDraw % static_cast<uint32>(QuantitySpan));
				Entry.GroupOrder = Group->GroupOrder;
				Entry.CandidateTieBreakOrder = Selected->TieBreakOrder;
				Remaining.RemoveAt(SelectedIndex);
			}
		}
		OutRoll.Entries.Sort([](const FCodeBLootProfileRollEntry& Left, const FCodeBLootProfileRollEntry& Right)
		{
			if (Left.GroupOrder != Right.GroupOrder) return Left.GroupOrder < Right.GroupOrder;
			if (Left.CandidateTieBreakOrder != Right.CandidateTieBreakOrder) return Left.CandidateTieBreakOrder < Right.CandidateTieBreakOrder;
			return Left.ItemDefinitionId.LexicalLess(Right.ItemDefinitionId);
		});
		int32 RootSlotIndex = 0;
		for (FCodeBLootProfileRollEntry& Entry : OutRoll.Entries)
		{
			// P21's optional equipment item is materialized into one fixed corpse
			// equipment container, never into the two-cell material root.
			Entry.SlotIndex = IsP21BasicCorpseR3(Profile) && FindP21BodyEquipmentSlot(Entry.ItemDefinitionId)
				? INDEX_NONE : RootSlotIndex++;
		}
		if (!ValidateLootProfileMaterialization(OutRoll, SourceContainerDefinitionId, SourceCapacity, OutError)) return false;
		TArray<FString> ResultPieces { Identity };
		for (const FCodeBLootProfileRollEntry& Entry : OutRoll.Entries)
		{
			ResultPieces.Add(FString::Printf(TEXT("entry:%d:%d:%s:%d"), Entry.GroupOrder,
				Entry.CandidateTieBreakOrder, *Entry.ItemDefinitionId.ToString(), Entry.Quantity));
		}
		OutRoll.ResultDigest = FString::Printf(TEXT("%08X"), FCrc::StrCrc32(*FString::Join(ResultPieces, TEXT("|"))));
		return true;
	}

	bool ValidateLootProfileMaterialization(
		const FCodeBLootProfileRollResult& Roll,
		const FName SourceContainerDefinitionId,
		const int32 SourceCapacity,
		FString& OutError)
	{
		const FCodeBLootProfile* Profile = FindLootProfileByProvenance(
			SourceContainerDefinitionId, Roll.LootProfileId, Roll.ProfileVersion);
		if (!Profile || !ValidateLootProfile(*Profile, SourceCapacity, OutError)
			|| Roll.LootProfileId != Profile->LootProfileId || Roll.ProfileVersion != Profile->ProfileVersion
			|| Roll.ProfileDigest != LootProfileDigest(*Profile) || Roll.AlgorithmVersion != Profile->AlgorithmVersion
			|| Roll.Entries.IsEmpty())
		{
			if (OutError.IsEmpty()) OutError = TEXT("Code B P16 Loot Profile materialization provenance is invalid.");
			return false;
		}
		const bool bP21R3 = IsP21BasicCorpseR3(*Profile);
		TSet<int32> Slots;
		for (const FCodeBLootProfileRollEntry& Entry : Roll.Entries)
		{
			FCodeBItemDefinition Definition;
			const FP21BodyEquipmentSlot* EquipmentSlot = bP21R3
				? FindP21BodyEquipmentSlot(Entry.ItemDefinitionId) : nullptr;
			if (Entry.ItemDefinitionId.IsNone() || Entry.Quantity < 1
				|| !BuildCanonicalCodeBItemDefinition(Entry.ItemDefinitionId, Definition, OutError)
				|| Entry.Quantity > Definition.MaxStack
				|| (EquipmentSlot
					? (Entry.SlotIndex != INDEX_NONE || Definition.bStackable || Definition.MaxStack != 1
						|| Definition.EquipSlot != EquipmentSlot->EquipmentSlot
						|| Definition.SpatialContainerSemantic != ECodeBSpatialContainerSemantic::None
						|| Definition.ChildContainerCapacity != 0)
					: (Entry.SlotIndex < 0 || Entry.SlotIndex >= SourceCapacity || Slots.Contains(Entry.SlotIndex))))
			{
				if (OutError.IsEmpty()) OutError = TEXT("Code B P16 Loot Profile roll result cannot create a legal P1 item graph.");
				return false;
			}
			if (!EquipmentSlot) Slots.Add(Entry.SlotIndex);
		}
		return true;
	}

	bool ValidateLootProfileReceiptProvenance(
		const FGuid& OwnerId,
		const FGuid& RunInstanceId,
		const FGuid& TargetId,
		const FName SourceContainerDefinitionId,
		const int32 SourceCapacity,
		const FGuid& DeathReceiptId,
		const FName LootProfileId,
		const int32 LootProfileVersion,
		const FString& LootProfileDigestText,
		const FString& LootAlgorithmVersion,
		const FString& LootResultDigest,
		FCodeBLootProfileRollResult* OutExpectedRoll,
		FString& OutError)
	{
		if (!HasLootProfileProvenance(LootProfileId, LootProfileVersion, LootProfileDigestText,
			LootAlgorithmVersion, LootResultDigest))
		{
			return true; // P9/P11 fixed-recipe history remains immutable and valid.
		}
		const FCodeBLootProfile* Profile = FindLootProfileByProvenance(
			SourceContainerDefinitionId, LootProfileId, LootProfileVersion);
		FCodeBLootProfileRollResult ExpectedRoll;
		if (!Profile || !BuildDeterministicLootProfileRollForProfile(OwnerId, RunInstanceId, TargetId,
			SourceContainerDefinitionId, SourceCapacity, DeathReceiptId, *Profile, ExpectedRoll, OutError)
			|| ExpectedRoll.LootProfileId != LootProfileId
			|| ExpectedRoll.ProfileVersion != LootProfileVersion
			|| ExpectedRoll.ProfileDigest != LootProfileDigestText
			|| ExpectedRoll.AlgorithmVersion != LootAlgorithmVersion
			|| ExpectedRoll.ResultDigest != LootResultDigest)
		{
			if (OutError.IsEmpty()) OutError = TEXT("Code B P16 Loot Profile receipt provenance does not match its exact deterministic identity.");
			return false;
		}
		if (OutExpectedRoll) *OutExpectedRoll = MoveTemp(ExpectedRoll);
		return true;
	}

	bool ValidateInitialLootProfileGraph(
		const FGuid& OwnerId,
		const FGuid& RunInstanceId,
		const FGuid& TargetId,
		const FName SourceContainerDefinitionId,
		const int32 SourceCapacity,
		const FGuid& DeathReceiptId,
		const FName LootProfileId,
		const int32 LootProfileVersion,
		const FString& LootProfileDigestText,
		const FString& LootAlgorithmVersion,
		const FString& LootResultDigest,
		const FGuid& RootContainerId,
		const FCodeBSnapshot& Snapshot,
		FString& OutError)
	{
		if (!HasLootProfileProvenance(LootProfileId, LootProfileVersion, LootProfileDigestText,
			LootAlgorithmVersion, LootResultDigest)) return true;
		FCodeBLootProfileRollResult ExpectedRoll;
		if (!ValidateLootProfileReceiptProvenance(OwnerId, RunInstanceId, TargetId,
			SourceContainerDefinitionId, SourceCapacity, DeathReceiptId, LootProfileId, LootProfileVersion,
			LootProfileDigestText, LootAlgorithmVersion, LootResultDigest, &ExpectedRoll, OutError))
		{
			return false;
		}
		const FCodeBLootProfile* Profile = FindLootProfileByProvenance(
			SourceContainerDefinitionId, LootProfileId, LootProfileVersion);
		const FCodeBContainer* Root = Snapshot.Containers.Find(RootContainerId);
		const bool bP21R3 = Profile && IsP21BasicCorpseR3(*Profile);
		if (!Profile || !Root || Snapshot.Items.Num() != ExpectedRoll.Entries.Num()
			|| (bP21R3 && !ValidateP21BodyEquipmentCatalog(OutError)))
		{
			OutError = TEXT("Code B P16 initial Loot Profile graph does not match its deterministic result shape.");
			return false;
		}
		const FString Identity = LootRollIdentityText(OwnerId, RunInstanceId, TargetId,
			SourceContainerDefinitionId, DeathReceiptId, *Profile);
		int32 ExpectedContainerCount = 1 + (bP21R3 ? P21BodyEquipmentSlots().Num() : 0);
		for (int32 EntryIndex = 0; EntryIndex < ExpectedRoll.Entries.Num(); ++EntryIndex)
		{
			const FCodeBLootProfileRollEntry& Entry = ExpectedRoll.Entries[EntryIndex];
			const FGuid ExpectedItemId = StableLootGraphGuid(Identity, TEXT("item"), EntryIndex);
			const FCodeBItemInstance* Item = Snapshot.Items.Find(ExpectedItemId);
			const FP21BodyEquipmentSlot* EquipmentSlot = bP21R3
				? FindP21BodyEquipmentSlot(Entry.ItemDefinitionId) : nullptr;
			const FGuid ExpectedParentId = EquipmentSlot
				? P21BodyEquipmentContainerGuid(TargetId, SourceContainerDefinitionId, EquipmentSlot->Semantic)
				: RootContainerId;
			const int32 ExpectedSlotIndex = EquipmentSlot ? 0 : Entry.SlotIndex;
			const FCodeBContainer* ExpectedParent = Snapshot.Containers.Find(ExpectedParentId);
			FCodeBItemDefinition CanonicalDefinition;
			if (!Item || !ExpectedParent || !ExpectedParent->Slots.IsValidIndex(ExpectedSlotIndex)
				|| ExpectedParent->Slots[ExpectedSlotIndex] != ExpectedItemId
				|| Item->DefinitionId != Entry.ItemDefinitionId || Item->Quantity != Entry.Quantity
				|| Item->ParentContainerId != ExpectedParentId || Item->SlotIndex != ExpectedSlotIndex
				|| !BuildCanonicalCodeBItemDefinition(Entry.ItemDefinitionId, CanonicalDefinition, OutError))
			{
				OutError = TEXT("Code B P16 initial Loot Profile graph differs from its deterministic roll result.");
				return false;
			}
			if (CanonicalDefinition.SpatialContainerSemantic == ECodeBSpatialContainerSemantic::None)
			{
				if (Item->ChildContainerId.IsValid()
					|| (EquipmentSlot && (Entry.SlotIndex != INDEX_NONE || CanonicalDefinition.EquipSlot != EquipmentSlot->EquipmentSlot)))
				{
					OutError = TEXT("Code B P18 simple loot entry unexpectedly owns a child container.");
					return false;
				}
				continue;
			}
			const FGuid ExpectedChildId = SpatialChildGuid(ExpectedItemId);
			const FCodeBContainer* Child = Snapshot.Containers.Find(ExpectedChildId);
			const FName ExpectedChildType = CanonicalDefinition.SpatialContainerSemantic == ECodeBSpatialContainerSemantic::QuickRing
				? FName(TEXT("CodeB.SpatialChild.QuickRing"))
				: FName(TEXT("CodeB.SpatialChild.StoragePouch"));
			if (Item->ChildContainerId != ExpectedChildId || !Child
				|| Child->ContainerType != ExpectedChildType || Child->IsEquipment()
				|| Child->Slots.Num() != CanonicalDefinition.ChildContainerCapacity
				|| Child->Slots.ContainsByPredicate([](const FGuid& Value) { return Value.IsValid(); }))
			{
				OutError = TEXT("Code B P18 spatial loot entry does not own its one canonical empty child container.");
				return false;
			}
			++ExpectedContainerCount;
		}
		if (Snapshot.Containers.Num() != ExpectedContainerCount)
		{
			OutError = TEXT("Code B P18 initial Loot Profile graph has an unexpected root, child, or nested container.");
			return false;
		}
		return true;
	}

	const FCodeBBodyContainerDefinition* FindBodyContainerDefinitionInternal(const FName DefinitionId)
	{
		return BodyContainerDefinitions().FindByPredicate([DefinitionId](const FCodeBBodyContainerDefinition& Definition)
		{
			return Definition.DefinitionId == DefinitionId;
		});
	}

	bool ValidateBodyContainerDefinition(const FCodeBBodyContainerDefinition& Definition, FString& OutError)
	{
		if (Definition.DefinitionId.IsNone() || Definition.ContainerType.IsNone()
			|| Definition.Capacity < 1 || Definition.ContentRevision < 1
			|| Definition.ContentPlan.Num() > Definition.Capacity)
		{
			OutError = TEXT("Code B body-container definition identity or capacity is invalid.");
			return false;
		}
		TSet<int32> OccupiedSlots;
		for (const FCodeBBodyContainerContentPlanEntry& Entry : Definition.ContentPlan)
		{
			FCodeBItemDefinition ItemDefinition;
			if (Entry.ItemDefinitionId.IsNone() || Entry.Quantity < 1 || Entry.SlotIndex < 0
				|| Entry.SlotIndex >= Definition.Capacity || Entry.ChildContainerCapacity < 0
				|| OccupiedSlots.Contains(Entry.SlotIndex)
				|| !BuildCanonicalCodeBItemDefinition(Entry.ItemDefinitionId, ItemDefinition, OutError))
			{
				if (OutError.IsEmpty()) OutError = TEXT("Code B body-container content plan is invalid.");
				return false;
			}
			if ((!ItemDefinition.bStackable && Entry.Quantity != 1)
				|| Entry.Quantity > ItemDefinition.MaxStack
				|| (Entry.ChildContainerCapacity > 0
					&& ItemDefinition.ItemType != ECodeBItemType::SpatialItem
					&& ItemDefinition.ItemType != ECodeBItemType::Backpack))
			{
				OutError = TEXT("Code B body-container item plan violates its canonical item definition.");
				return false;
			}
			OccupiedSlots.Add(Entry.SlotIndex);
		}
		return true;
	}

	FString BodyContainerDefinitionDigest(const FCodeBBodyContainerDefinition& Definition)
	{
		TArray<FString> Pieces;
		Pieces.Add(FString::Printf(TEXT("definition:%s:%s:%d:%d"), *Definition.DefinitionId.ToString(), *Definition.ContainerType.ToString(), Definition.Capacity, Definition.ContentRevision));
		for (const FCodeBBodyContainerContentPlanEntry& Entry : Definition.ContentPlan)
		{
			Pieces.Add(FString::Printf(TEXT("item:%s:%d:%d:%d"), *Entry.ItemDefinitionId.ToString(), Entry.Quantity, Entry.SlotIndex, Entry.ChildContainerCapacity));
		}
		Pieces.Sort();
		return FString::Printf(TEXT("%08X"), FCrc::StrCrc32(*FString::Join(Pieces, TEXT("|"))));
	}

	FGuid StableBodyContainerGuid(
		const FGuid& OwnerId,
		const FGuid& RunInstanceId,
		const FGuid& BodyTargetId,
		const FName DefinitionId,
		const int32 ContentRevision,
		const uint32 Salt)
	{
		const FString Seed = FString::Printf(TEXT("P11|%s|%s|%s|%s|%d|%u"),
			*GuidText(OwnerId), *GuidText(RunInstanceId), *GuidText(BodyTargetId),
			*DefinitionId.ToString(), ContentRevision, Salt);
		return FGuid(
			FCrc::StrCrc32(*(Seed + TEXT("|A"))),
			FCrc::StrCrc32(*(Seed + TEXT("|B"))),
			FCrc::StrCrc32(*(Seed + TEXT("|C"))),
			FCrc::StrCrc32(*(Seed + TEXT("|D"))));
	}

	const TCHAR* BodyContainerStateText(const ECodeBBodyContainerState State)
	{
		switch (State)
		{
		case ECodeBBodyContainerState::BodyMaterialized: return TEXT("BodyMaterialized");
		case ECodeBBodyContainerState::Opening: return TEXT("Opening");
		case ECodeBBodyContainerState::Open: return TEXT("Open");
		case ECodeBBodyContainerState::Interrupted: return TEXT("Interrupted");
		default: return TEXT("Unknown");
		}
	}

	bool TryBodyContainerState(const FString& Text, ECodeBBodyContainerState& OutState)
	{
		if (Text == TEXT("BodyMaterialized")) { OutState = ECodeBBodyContainerState::BodyMaterialized; return true; }
		if (Text == TEXT("Opening")) { OutState = ECodeBBodyContainerState::Opening; return true; }
		if (Text == TEXT("Open")) { OutState = ECodeBBodyContainerState::Open; return true; }
		if (Text == TEXT("Interrupted")) { OutState = ECodeBBodyContainerState::Interrupted; return true; }
		return false;
	}

	const TCHAR* BodyContainerVisibilityText(const ECodeBBodyContainerVisibility Visibility)
	{
		switch (Visibility)
		{
		case ECodeBBodyContainerVisibility::Hidden: return TEXT("Hidden");
		case ECodeBBodyContainerVisibility::Searching: return TEXT("Searching");
		case ECodeBBodyContainerVisibility::Revealed: return TEXT("Revealed");
		default: return TEXT("Unknown");
		}
	}

	bool TryBodyContainerVisibility(const FString& Text, ECodeBBodyContainerVisibility& OutVisibility)
	{
		if (Text == TEXT("Hidden")) { OutVisibility = ECodeBBodyContainerVisibility::Hidden; return true; }
		if (Text == TEXT("Searching")) { OutVisibility = ECodeBBodyContainerVisibility::Searching; return true; }
		if (Text == TEXT("Revealed")) { OutVisibility = ECodeBBodyContainerVisibility::Revealed; return true; }
		return false;
	}

	bool ValidateBodyContainerDeathReceipt(
		const FCodeBBodyContainerDeathReceipt& Receipt,
		const FGuid& OwnerId,
		const FGuid& RunInstanceId,
		const FGuid& BodyTargetId,
		const FName DefinitionId,
		FString& OutError)
	{
		if (!Receipt.DeathReceiptId.IsValid() || Receipt.OwnerId != OwnerId
			|| Receipt.RunInstanceId != RunInstanceId || Receipt.BodyTargetId != BodyTargetId
			|| Receipt.DefinitionId != DefinitionId || Receipt.StaticSpawnIdentity.IsNone()
			|| Receipt.SpawnOrdinal < 0 || Receipt.CommittedUtc.IsEmpty())
		{
			OutError = TEXT("Code B P11 death receipt has an invalid exact identity or static provenance.");
			return false;
		}
		return true;
	}

	FString BodyContainerMaterializationDigest(const FCodeBRunLocalBodyContainerRecord& Record)
	{
		TArray<FString> Pieces;
		Pieces.Add(FString::Printf(TEXT("identity:%s:%s:%s:%s:%s"),
			*GuidText(Record.OwnerId), *GuidText(Record.RunInstanceId), *GuidText(Record.BodyTargetId),
			*Record.DefinitionId.ToString(), *GuidText(Record.ContainerId)));
		Pieces.Add(FString::Printf(TEXT("death:%s:%s:%d"),
			*GuidText(Record.Receipt.DeathReceipt.DeathReceiptId),
			*Record.Receipt.DeathReceipt.StaticSpawnIdentity.ToString(),
			Record.Receipt.DeathReceipt.SpawnOrdinal));
		if (HasLootProfileProvenance(Record.Receipt.LootProfileId, Record.Receipt.LootProfileVersion,
			Record.Receipt.LootProfileDigest, Record.Receipt.LootAlgorithmVersion, Record.Receipt.LootResultDigest))
		{
			Pieces.Add(FString::Printf(TEXT("loot:%s:%d:%s:%s:%s"),
				*Record.Receipt.LootProfileId.ToString(), Record.Receipt.LootProfileVersion,
				*Record.Receipt.LootProfileDigest, *Record.Receipt.LootAlgorithmVersion,
				*Record.Receipt.LootResultDigest));
		}
		if (!Record.Receipt.EquipmentCandidateSetDigest.IsEmpty())
		{
			Pieces.Add(FString::Printf(TEXT("equipment-candidates:%s"), *Record.Receipt.EquipmentCandidateSetDigest));
		}
		Pieces.Add(TerminalSnapshotDigest(Record.ContainerSnapshot, FCodeBP2PlayerLayout()));
		for (const FCodeBBodyContainerItemVisibility& Visibility : Record.ItemVisibilities)
		{
			Pieces.Add(FString::Printf(TEXT("visibility:%s:%s"), *GuidText(Visibility.ItemId), BodyContainerVisibilityText(Visibility.Visibility)));
		}
		Pieces.Sort();
		return FString::Printf(TEXT("%08X"), FCrc::StrCrc32(*FString::Join(Pieces, TEXT("|"))));
	}

	TSharedRef<FJsonObject> BodyContainerDeathReceiptJson(const FCodeBBodyContainerDeathReceipt& Receipt)
	{
		TSharedRef<FJsonObject> Object = MakeShared<FJsonObject>();
		Object->SetStringField(TEXT("DeathReceiptId"), GuidText(Receipt.DeathReceiptId));
		Object->SetStringField(TEXT("OwnerId"), GuidText(Receipt.OwnerId));
		Object->SetStringField(TEXT("RunInstanceId"), GuidText(Receipt.RunInstanceId));
		Object->SetStringField(TEXT("BodyTargetId"), GuidText(Receipt.BodyTargetId));
		Object->SetStringField(TEXT("DefinitionId"), Receipt.DefinitionId.ToString());
		Object->SetStringField(TEXT("StaticSpawnIdentity"), Receipt.StaticSpawnIdentity.ToString());
		Object->SetNumberField(TEXT("SpawnOrdinal"), Receipt.SpawnOrdinal);
		Object->SetStringField(TEXT("CommittedUtc"), Receipt.CommittedUtc);
		return Object;
	}

	bool JsonToBodyContainerDeathReceipt(
		const TSharedPtr<FJsonObject>& Object,
		FCodeBBodyContainerDeathReceipt& OutReceipt,
		FString& OutError)
	{
		OutReceipt = FCodeBBodyContainerDeathReceipt();
		FString DeathReceiptId, OwnerId, RunInstanceId, BodyTargetId, DefinitionId, SpawnIdentity;
		double SpawnOrdinal = 0.0;
		if (!Object.IsValid() || !Object->TryGetStringField(TEXT("DeathReceiptId"), DeathReceiptId)
			|| !Object->TryGetStringField(TEXT("OwnerId"), OwnerId)
			|| !Object->TryGetStringField(TEXT("RunInstanceId"), RunInstanceId)
			|| !Object->TryGetStringField(TEXT("BodyTargetId"), BodyTargetId)
			|| !Object->TryGetStringField(TEXT("DefinitionId"), DefinitionId)
			|| !Object->TryGetStringField(TEXT("StaticSpawnIdentity"), SpawnIdentity)
			|| !Object->TryGetNumberField(TEXT("SpawnOrdinal"), SpawnOrdinal)
			|| !Object->TryGetStringField(TEXT("CommittedUtc"), OutReceipt.CommittedUtc)
			|| !FMath::IsNearlyEqual(SpawnOrdinal, FMath::RoundToDouble(SpawnOrdinal))
			|| !TryGuidText(DeathReceiptId, OutReceipt.DeathReceiptId) || !OutReceipt.DeathReceiptId.IsValid()
			|| !TryGuidText(OwnerId, OutReceipt.OwnerId) || !OutReceipt.OwnerId.IsValid()
			|| !TryGuidText(RunInstanceId, OutReceipt.RunInstanceId) || !OutReceipt.RunInstanceId.IsValid()
			|| !TryGuidText(BodyTargetId, OutReceipt.BodyTargetId) || !OutReceipt.BodyTargetId.IsValid()
			|| DefinitionId.IsEmpty() || SpawnIdentity.IsEmpty())
		{
			OutError = TEXT("Code B P11 death receipt JSON is invalid.");
			return false;
		}
		OutReceipt.DefinitionId = FName(*DefinitionId);
		OutReceipt.StaticSpawnIdentity = FName(*SpawnIdentity);
		OutReceipt.SpawnOrdinal = static_cast<int32>(SpawnOrdinal);
		return true;
	}

	TSharedRef<FJsonObject> BodyContainerMaterializationReceiptJson(const FCodeBBodyContainerMaterializationReceipt& Receipt)
	{
		TSharedRef<FJsonObject> Object = MakeShared<FJsonObject>();
		Object->SetStringField(TEXT("ReceiptId"), GuidText(Receipt.ReceiptId));
		Object->SetStringField(TEXT("OwnerId"), GuidText(Receipt.OwnerId));
		Object->SetStringField(TEXT("RunInstanceId"), GuidText(Receipt.RunInstanceId));
		Object->SetStringField(TEXT("BodyTargetId"), GuidText(Receipt.BodyTargetId));
		Object->SetStringField(TEXT("DefinitionId"), Receipt.DefinitionId.ToString());
		Object->SetStringField(TEXT("ContainerId"), GuidText(Receipt.ContainerId));
		Object->SetNumberField(TEXT("DefinitionContentRevision"), Receipt.DefinitionContentRevision);
		Object->SetStringField(TEXT("DefinitionDigest"), Receipt.DefinitionDigest);
		WriteLootProfileProvenance(Object, Receipt.LootProfileId, Receipt.LootProfileVersion,
			Receipt.LootProfileDigest, Receipt.LootAlgorithmVersion, Receipt.LootResultDigest);
		if (!Receipt.EquipmentCandidateSetDigest.IsEmpty())
		{
			Object->SetStringField(TEXT("EquipmentCandidateSetDigest"), Receipt.EquipmentCandidateSetDigest);
		}
		Object->SetStringField(TEXT("MaterializationDigest"), Receipt.MaterializationDigest);
		Object->SetObjectField(TEXT("DeathReceipt"), BodyContainerDeathReceiptJson(Receipt.DeathReceipt));
		Object->SetStringField(TEXT("MaterializedUtc"), Receipt.MaterializedUtc);
		return Object;
	}

	bool JsonToBodyContainerMaterializationReceipt(
		const TSharedPtr<FJsonObject>& Object,
		FCodeBBodyContainerMaterializationReceipt& OutReceipt,
		FString& OutError)
	{
		OutReceipt = FCodeBBodyContainerMaterializationReceipt();
		FString ReceiptId, OwnerId, RunInstanceId, BodyTargetId, DefinitionId, ContainerId;
		double ContentRevision = 0.0;
		const TSharedPtr<FJsonObject>* DeathReceipt = nullptr;
		if (!Object.IsValid() || !Object->TryGetStringField(TEXT("ReceiptId"), ReceiptId)
			|| !Object->TryGetStringField(TEXT("OwnerId"), OwnerId)
			|| !Object->TryGetStringField(TEXT("RunInstanceId"), RunInstanceId)
			|| !Object->TryGetStringField(TEXT("BodyTargetId"), BodyTargetId)
			|| !Object->TryGetStringField(TEXT("DefinitionId"), DefinitionId)
			|| !Object->TryGetStringField(TEXT("ContainerId"), ContainerId)
			|| !Object->TryGetNumberField(TEXT("DefinitionContentRevision"), ContentRevision)
			|| !Object->TryGetStringField(TEXT("DefinitionDigest"), OutReceipt.DefinitionDigest)
			|| !Object->TryGetStringField(TEXT("MaterializationDigest"), OutReceipt.MaterializationDigest)
			|| !Object->TryGetObjectField(TEXT("DeathReceipt"), DeathReceipt)
			|| !Object->TryGetStringField(TEXT("MaterializedUtc"), OutReceipt.MaterializedUtc)
			|| !FMath::IsNearlyEqual(ContentRevision, FMath::RoundToDouble(ContentRevision))
			|| !TryGuidText(ReceiptId, OutReceipt.ReceiptId) || !OutReceipt.ReceiptId.IsValid()
			|| !TryGuidText(OwnerId, OutReceipt.OwnerId) || !OutReceipt.OwnerId.IsValid()
			|| !TryGuidText(RunInstanceId, OutReceipt.RunInstanceId) || !OutReceipt.RunInstanceId.IsValid()
			|| !TryGuidText(BodyTargetId, OutReceipt.BodyTargetId) || !OutReceipt.BodyTargetId.IsValid()
			|| !TryGuidText(ContainerId, OutReceipt.ContainerId) || !OutReceipt.ContainerId.IsValid()
			|| DefinitionId.IsEmpty()
			|| !JsonToBodyContainerDeathReceipt(*DeathReceipt, OutReceipt.DeathReceipt, OutError))
		{
			if (OutError.IsEmpty()) OutError = TEXT("Code B P11 materialization receipt JSON is invalid.");
			return false;
		}
		OutReceipt.DefinitionId = FName(*DefinitionId);
		OutReceipt.DefinitionContentRevision = static_cast<int32>(ContentRevision);
		if (Object->HasField(TEXT("EquipmentCandidateSetDigest"))
			&& !Object->TryGetStringField(TEXT("EquipmentCandidateSetDigest"), OutReceipt.EquipmentCandidateSetDigest))
		{
			OutError = TEXT("Code B P21 body equipment candidate-set digest JSON is invalid.");
			return false;
		}
		return ReadLootProfileProvenance(Object, OutReceipt.LootProfileId, OutReceipt.LootProfileVersion,
			OutReceipt.LootProfileDigest, OutReceipt.LootAlgorithmVersion, OutReceipt.LootResultDigest, OutError);
	}

	TSharedRef<FJsonObject> RunLocalBodyContainerJson(const FCodeBRunLocalBodyContainerRecord& Record)
	{
		TSharedRef<FJsonObject> Object = MakeShared<FJsonObject>();
		Object->SetNumberField(TEXT("SchemaVersion"), Record.SchemaVersion);
		Object->SetStringField(TEXT("OwnerId"), GuidText(Record.OwnerId));
		Object->SetStringField(TEXT("RunInstanceId"), GuidText(Record.RunInstanceId));
		Object->SetStringField(TEXT("BodyTargetId"), GuidText(Record.BodyTargetId));
		Object->SetStringField(TEXT("DefinitionId"), Record.DefinitionId.ToString());
		Object->SetStringField(TEXT("ContainerId"), GuidText(Record.ContainerId));
		Object->SetBoolField(TEXT("Materialized"), Record.bMaterialized);
		Object->SetStringField(TEXT("State"), BodyContainerStateText(Record.State));
		Object->SetNumberField(TEXT("Revision"), Record.Revision);
		Object->SetStringField(TEXT("ActiveActionId"), GuidText(Record.ActiveActionId));
		Object->SetStringField(TEXT("ActiveSearchItemId"), GuidText(Record.ActiveSearchItemId));
		Object->SetObjectField(TEXT("Receipt"), BodyContainerMaterializationReceiptJson(Record.Receipt));
		Object->SetObjectField(TEXT("ContainerSnapshot"), SnapshotJson(Record.ContainerSnapshot));
		TArray<TSharedPtr<FJsonValue>> Visibilities;
		for (const FCodeBBodyContainerItemVisibility& Visibility : Record.ItemVisibilities)
		{
			TSharedRef<FJsonObject> Item = MakeShared<FJsonObject>();
			Item->SetStringField(TEXT("ItemId"), GuidText(Visibility.ItemId));
			Item->SetStringField(TEXT("Visibility"), BodyContainerVisibilityText(Visibility.Visibility));
			Visibilities.Add(MakeShared<FJsonValueObject>(Item));
		}
		Object->SetArrayField(TEXT("ItemVisibilities"), Visibilities);
		return Object;
	}

	bool JsonToRunLocalBodyContainer(
		const TSharedPtr<FJsonObject>& Object,
		FCodeBRunLocalBodyContainerRecord& OutRecord,
		FString& OutError)
	{
		OutRecord = FCodeBRunLocalBodyContainerRecord();
		double Schema = 0.0, Revision = 0.0;
		FString OwnerId, RunInstanceId, BodyTargetId, DefinitionId, ContainerId, State;
		FString ActiveActionId, ActiveSearchItemId;
		const TSharedPtr<FJsonObject> *ReceiptObject = nullptr, *SnapshotObject = nullptr;
		const TArray<TSharedPtr<FJsonValue>>* Visibilities = nullptr;
		if (!Object.IsValid() || !Object->TryGetNumberField(TEXT("SchemaVersion"), Schema)
			|| !Object->TryGetStringField(TEXT("OwnerId"), OwnerId)
			|| !Object->TryGetStringField(TEXT("RunInstanceId"), RunInstanceId)
			|| !Object->TryGetStringField(TEXT("BodyTargetId"), BodyTargetId)
			|| !Object->TryGetStringField(TEXT("DefinitionId"), DefinitionId)
			|| !Object->TryGetStringField(TEXT("ContainerId"), ContainerId)
			|| !Object->TryGetBoolField(TEXT("Materialized"), OutRecord.bMaterialized)
			|| !Object->TryGetStringField(TEXT("State"), State)
			|| !Object->TryGetNumberField(TEXT("Revision"), Revision)
			|| !Object->TryGetObjectField(TEXT("Receipt"), ReceiptObject)
			|| !Object->TryGetObjectField(TEXT("ContainerSnapshot"), SnapshotObject)
			|| !Object->TryGetArrayField(TEXT("ItemVisibilities"), Visibilities)
			|| !FMath::IsNearlyEqual(Schema, FMath::RoundToDouble(Schema))
			|| !FMath::IsNearlyEqual(Revision, FMath::RoundToDouble(Revision))
			|| (static_cast<int32>(Schema) != 1 && static_cast<int32>(Schema) != 2 && static_cast<int32>(Schema) != 3
				&& static_cast<int32>(Schema) != FCodeBRunLocalBodyContainerRecord::CurrentSchemaVersion)
			|| !TryGuidText(OwnerId, OutRecord.OwnerId) || !OutRecord.OwnerId.IsValid()
			|| !TryGuidText(RunInstanceId, OutRecord.RunInstanceId) || !OutRecord.RunInstanceId.IsValid()
			|| !TryGuidText(BodyTargetId, OutRecord.BodyTargetId) || !OutRecord.BodyTargetId.IsValid()
			|| !TryGuidText(ContainerId, OutRecord.ContainerId) || !OutRecord.ContainerId.IsValid()
			|| DefinitionId.IsEmpty() || !TryBodyContainerState(State, OutRecord.State)
			|| !JsonToBodyContainerMaterializationReceipt(*ReceiptObject, OutRecord.Receipt, OutError)
			|| !JsonToSnapshot(*SnapshotObject, OutRecord.ContainerSnapshot, OutError))
		{
			if (OutError.IsEmpty()) OutError = TEXT("Code B P11 Run-local body-container record JSON is invalid.");
			return false;
		}
		const int32 StoredSchemaVersion = static_cast<int32>(Schema);
		if (StoredSchemaVersion >= 2)
		{
			if (!Object->TryGetStringField(TEXT("ActiveActionId"), ActiveActionId)
				|| !Object->TryGetStringField(TEXT("ActiveSearchItemId"), ActiveSearchItemId)
				|| !TryGuidText(ActiveActionId, OutRecord.ActiveActionId)
				|| !TryGuidText(ActiveSearchItemId, OutRecord.ActiveSearchItemId))
			{
				OutError = TEXT("Code B P12 body-container action JSON is invalid.");
				return false;
			}
		}
		OutRecord.SchemaVersion = FCodeBRunLocalBodyContainerRecord::CurrentSchemaVersion;
		OutRecord.DefinitionId = FName(*DefinitionId);
		OutRecord.Revision = static_cast<int32>(Revision);
		for (const TSharedPtr<FJsonValue>& Value : *Visibilities)
		{
			const TSharedPtr<FJsonObject> Item = Value.IsValid() ? Value->AsObject() : nullptr;
			FString ItemId, VisibilityText;
			FCodeBBodyContainerItemVisibility Visibility;
			if (!Item || !Item->TryGetStringField(TEXT("ItemId"), ItemId)
				|| !Item->TryGetStringField(TEXT("Visibility"), VisibilityText)
				|| !TryGuidText(ItemId, Visibility.ItemId) || !Visibility.ItemId.IsValid()
				|| !TryBodyContainerVisibility(VisibilityText, Visibility.Visibility))
			{
				OutError = TEXT("Code B P11 body-container visibility JSON is invalid.");
				return false;
			}
			OutRecord.ItemVisibilities.Add(MoveTemp(Visibility));
		}
		return true;
	}

	bool ValidateRunLocalBodyContainerRecord(
		const FCodeBRunLocalBodyContainerRecord& Record,
		FString& OutError)
	{
		if (Record.SchemaVersion != FCodeBRunLocalBodyContainerRecord::CurrentSchemaVersion
			|| !Record.OwnerId.IsValid() || !Record.RunInstanceId.IsValid()
			|| !Record.BodyTargetId.IsValid() || Record.DefinitionId.IsNone()
			|| !Record.ContainerId.IsValid() || !Record.bMaterialized || Record.Revision < 1
			|| (Record.State != ECodeBBodyContainerState::BodyMaterialized
				&& Record.State != ECodeBBodyContainerState::Opening
				&& Record.State != ECodeBBodyContainerState::Open
				&& Record.State != ECodeBBodyContainerState::Interrupted)
			|| !Record.Receipt.ReceiptId.IsValid() || Record.Receipt.OwnerId != Record.OwnerId
			|| Record.Receipt.RunInstanceId != Record.RunInstanceId
			|| Record.Receipt.BodyTargetId != Record.BodyTargetId
			|| Record.Receipt.DefinitionId != Record.DefinitionId
			|| Record.Receipt.ContainerId != Record.ContainerId
			|| Record.Receipt.DefinitionContentRevision < 1
			|| Record.Receipt.DefinitionDigest.IsEmpty()
			|| Record.Receipt.MaterializationDigest.IsEmpty()
			|| Record.Receipt.MaterializedUtc.IsEmpty()
			|| !ValidateBodyContainerDeathReceipt(Record.Receipt.DeathReceipt, Record.OwnerId,
				Record.RunInstanceId, Record.BodyTargetId, Record.DefinitionId, OutError))
		{
			if (OutError.IsEmpty()) OutError = TEXT("Code B P11 body-container record identity or receipt is invalid.");
			return false;
		}
		const FCodeBBodyContainerDefinition* Definition = FindBodyContainerDefinitionInternal(Record.DefinitionId);
		if (!Definition || !ValidateBodyContainerDefinition(*Definition, OutError)
			|| Record.Receipt.DefinitionContentRevision != Definition->ContentRevision
			|| Record.Receipt.DefinitionDigest != BodyContainerDefinitionDigest(*Definition))
		{
			if (OutError.IsEmpty()) OutError = TEXT("Code B P11 body-container definition receipt is invalid.");
			return false;
		}
		if (!ValidateLootProfileReceiptProvenance(Record.OwnerId, Record.RunInstanceId, Record.BodyTargetId,
			Record.DefinitionId, Definition->Capacity, Record.Receipt.DeathReceipt.DeathReceiptId,
			Record.Receipt.LootProfileId, Record.Receipt.LootProfileVersion, Record.Receipt.LootProfileDigest,
			Record.Receipt.LootAlgorithmVersion, Record.Receipt.LootResultDigest, nullptr, OutError))
		{
			return false;
		}
		const FCodeBLootProfile* Profile = FindLootProfileByProvenance(
			Record.DefinitionId, Record.Receipt.LootProfileId, Record.Receipt.LootProfileVersion);
		const bool bP21R3 = Profile && IsP21BasicCorpseR3(*Profile);
		if ((bP21R3 && (!ValidateP21BodyEquipmentCatalog(OutError)
			|| Record.Receipt.EquipmentCandidateSetDigest != P21EquipmentCandidateSetDigest()))
			|| (!bP21R3 && !Record.Receipt.EquipmentCandidateSetDigest.IsEmpty()))
		{
			if (OutError.IsEmpty()) OutError = TEXT("Code B P21 body equipment candidate-set provenance is invalid.");
			return false;
		}
		FCodeBRepository ValidationRepository;
		if (!ValidationRepository.LoadPersistedSnapshot(Record.ContainerSnapshot, &OutError)) return false;
		const FCodeBContainer* Root = Record.ContainerSnapshot.Containers.Find(Record.ContainerId);
		if (!Root || Root->ContainerType != Definition->ContainerType
			|| Root->Kind != ECodeBContainerKind::Storage || Root->EquipmentSlot != ECodeBEquipSlot::None
			|| Root->Slots.Num() != Definition->Capacity)
		{
			OutError = TEXT("Code B P11 body-container root does not match its definition.");
			return false;
		}
		TSet<FGuid> VisitedContainers, VisitedItems;
		TArray<FGuid> PendingContainers { Record.ContainerId };
		if (bP21R3)
		{
			for (const FP21BodyEquipmentSlot& EquipmentSlot : P21BodyEquipmentSlots())
			{
				const FGuid EquipmentContainerId = P21BodyEquipmentContainerGuid(
					Record.BodyTargetId, Record.DefinitionId, EquipmentSlot.Semantic);
				const FCodeBContainer* EquipmentContainer = Record.ContainerSnapshot.Containers.Find(EquipmentContainerId);
				if (!EquipmentContainer || EquipmentContainer->ContainerType != EquipmentSlot.ContainerType
					|| !EquipmentContainer->IsEquipment() || EquipmentContainer->EquipmentSlot != EquipmentSlot.EquipmentSlot
					|| EquipmentContainer->Slots.Num() != 1)
				{
					OutError = TEXT("Code B P21 body equipment slot graph is missing or has invalid provenance.");
					return false;
				}
				PendingContainers.Add(EquipmentContainerId);
			}
		}
		while (!PendingContainers.IsEmpty())
		{
			const FGuid CurrentContainerId = PendingContainers.Pop();
			if (VisitedContainers.Contains(CurrentContainerId))
			{
				OutError = TEXT("Code B P11 body-container graph contains a cycle.");
				return false;
			}
			const FCodeBContainer* Container = Record.ContainerSnapshot.Containers.Find(CurrentContainerId);
			if (!Container)
			{
				OutError = TEXT("Code B P11 body-container graph is missing a child container.");
				return false;
			}
			VisitedContainers.Add(CurrentContainerId);
			for (const FGuid& ItemId : Container->Slots)
			{
				if (!ItemId.IsValid()) continue;
				const FCodeBItemInstance* Item = Record.ContainerSnapshot.Items.Find(ItemId);
				if (!Item || Item->ParentContainerId != CurrentContainerId || VisitedItems.Contains(ItemId))
				{
					OutError = TEXT("Code B P11 body-container graph has duplicate or invalid item placement.");
					return false;
				}
				VisitedItems.Add(ItemId);
				if (Item->ChildContainerId.IsValid()) PendingContainers.Add(Item->ChildContainerId);
			}
		}
		if (VisitedContainers.Num() != Record.ContainerSnapshot.Containers.Num()
			|| VisitedItems.Num() != Record.ContainerSnapshot.Items.Num())
		{
			OutError = TEXT("Code B P11 body-container graph contains an orphan item or container.");
			return false;
		}
		TSet<FGuid> VisibilityItemIds;
		for (const FCodeBBodyContainerItemVisibility& Visibility : Record.ItemVisibilities)
		{
			if (!Record.ContainerSnapshot.Items.Contains(Visibility.ItemId)
				|| VisibilityItemIds.Contains(Visibility.ItemId)
				|| (Visibility.Visibility != ECodeBBodyContainerVisibility::Hidden
					&& Visibility.Visibility != ECodeBBodyContainerVisibility::Searching
					&& Visibility.Visibility != ECodeBBodyContainerVisibility::Revealed))
			{
				OutError = TEXT("Code B P12 body-container visibility must cover each current item exactly once.");
				return false;
			}
			VisibilityItemIds.Add(Visibility.ItemId);
		}
		if (VisibilityItemIds.Num() != Record.ContainerSnapshot.Items.Num())
		{
			OutError = TEXT("Code B P12 body-container visibility coverage is invalid.");
			return false;
		}
		if (bP21R3)
		{
			int32 EquipmentItemCount = 0;
			for (const FP21BodyEquipmentSlot& EquipmentSlot : P21BodyEquipmentSlots())
			{
				const FCodeBContainer& EquipmentContainer = Record.ContainerSnapshot.Containers.FindChecked(
					P21BodyEquipmentContainerGuid(Record.BodyTargetId, Record.DefinitionId, EquipmentSlot.Semantic));
				if (!EquipmentContainer.Slots[0].IsValid()) continue;
				const FCodeBItemInstance* Item = Record.ContainerSnapshot.Items.Find(EquipmentContainer.Slots[0]);
				FCodeBItemDefinition ItemDefinition;
				if (++EquipmentItemCount > 1 || !Item || Item->ParentContainerId != EquipmentContainer.ContainerId
					|| Item->SlotIndex != 0 || Item->Quantity != 1 || Item->ChildContainerId.IsValid()
					|| Item->DefinitionId != EquipmentSlot.ItemDefinitionId
					|| !BuildCanonicalCodeBItemDefinition(Item->DefinitionId, ItemDefinition, OutError)
					|| ItemDefinition.EquipSlot != EquipmentSlot.EquipmentSlot
					|| ItemDefinition.SpatialContainerSemantic != ECodeBSpatialContainerSemantic::None)
				{
					if (OutError.IsEmpty()) OutError = TEXT("Code B P21 body equipment item is not one legal simple formal candidate.");
					return false;
				}
			}
		}
		if (Record.State == ECodeBBodyContainerState::BodyMaterialized
			&& (Record.ActiveActionId.IsValid() || Record.ActiveSearchItemId.IsValid()))
		{
			OutError = TEXT("Code B P12 BodyMaterialized state cannot retain an action identity.");
			return false;
		}
		if (Record.Revision == 1
			&& Record.Receipt.MaterializationDigest != BodyContainerMaterializationDigest(Record))
		{
			OutError = TEXT("Code B P11 initial body materialization digest is invalid.");
			return false;
		}
		if (Record.Revision == 1 && !ValidateInitialLootProfileGraph(Record.OwnerId, Record.RunInstanceId,
			Record.BodyTargetId, Record.DefinitionId, Definition->Capacity, Record.Receipt.DeathReceipt.DeathReceiptId,
			Record.Receipt.LootProfileId, Record.Receipt.LootProfileVersion, Record.Receipt.LootProfileDigest,
			Record.Receipt.LootAlgorithmVersion, Record.Receipt.LootResultDigest, Record.ContainerId,
			Record.ContainerSnapshot, OutError))
		{
			return false;
		}
		if (Record.State == ECodeBBodyContainerState::Opening
			&& (!Record.ActiveActionId.IsValid() || Record.ActiveSearchItemId.IsValid()))
		{
			OutError = TEXT("Code B P12 Opening state requires exactly one opening action identity.");
			return false;
		}
		if (Record.State == ECodeBBodyContainerState::Open)
		{
			if (Record.ActiveSearchItemId.IsValid())
			{
				const FCodeBBodyContainerItemVisibility* Searching = Record.ItemVisibilities.FindByPredicate(
					[&Record](const FCodeBBodyContainerItemVisibility& Value)
					{ return Value.ItemId == Record.ActiveSearchItemId; });
				if (!Record.ActiveActionId.IsValid() || !Searching
					|| Searching->Visibility != ECodeBBodyContainerVisibility::Searching)
				{
					OutError = TEXT("Code B P12 Open search action is not matched to one Searching item.");
					return false;
				}
			}
			else if (Record.ActiveActionId.IsValid())
			{
				OutError = TEXT("Code B P12 Open state cannot retain an orphan action identity.");
				return false;
			}
		}
		if (Record.State == ECodeBBodyContainerState::Interrupted
			&& (Record.ActiveActionId.IsValid() || Record.ActiveSearchItemId.IsValid()))
		{
			OutError = TEXT("Code B P12 Interrupted state cannot retain an action identity.");
			return false;
		}
		return true;
	}

	bool BuildBodyContainerProjection(
		const FCodeBRunLocalBodyContainerRecord& Record,
		FCodeBBodyContainerProjection& OutProjection,
		FString& OutError)
	{
		if (!ValidateRunLocalBodyContainerRecord(Record, OutError)) return false;
		OutProjection = FCodeBBodyContainerProjection();
		OutProjection.OwnerId = Record.OwnerId;
		OutProjection.RunInstanceId = Record.RunInstanceId;
		OutProjection.BodyTargetId = Record.BodyTargetId;
		OutProjection.DefinitionId = Record.DefinitionId;
		OutProjection.ContainerId = Record.ContainerId;
		OutProjection.State = Record.State;
		OutProjection.Revision = Record.Revision;
		OutProjection.ActiveActionId = Record.ActiveActionId;
		OutProjection.ActiveSearchItemId = Record.ActiveSearchItemId;
		OutProjection.Receipt = Record.Receipt;
		TMap<FGuid, ECodeBBodyContainerVisibility> VisibilityByItemId;
		for (const FCodeBBodyContainerItemVisibility& Visibility : Record.ItemVisibilities)
		{
			VisibilityByItemId.Add(Visibility.ItemId, Visibility.Visibility);
		}
		for (const TPair<FGuid, FCodeBItemInstance>& Pair : Record.ContainerSnapshot.Items)
		{
			const FCodeBItemInstance& Item = Pair.Value;
			FCodeBBodyContainerItemProjection& ProjectionItem = OutProjection.Items.AddDefaulted_GetRef();
			ProjectionItem.ItemId = Item.ItemId;
			ProjectionItem.DefinitionId = Item.DefinitionId;
			ProjectionItem.Quantity = Item.Quantity;
			ProjectionItem.ParentContainerId = Item.ParentContainerId;
			ProjectionItem.SlotIndex = Item.SlotIndex;
			ProjectionItem.ChildContainerId = Item.ChildContainerId;
			ProjectionItem.Visibility = VisibilityByItemId.FindChecked(Item.ItemId);
		}
		OutProjection.Items.Sort([](const FCodeBBodyContainerItemProjection& Left, const FCodeBBodyContainerItemProjection& Right)
		{
			return Left.ItemId.ToString(EGuidFormats::DigitsWithHyphensLower) < Right.ItemId.ToString(EGuidFormats::DigitsWithHyphensLower);
		});
		const FCodeBLootProfile* Profile = FindLootProfileByProvenance(
			Record.DefinitionId, Record.Receipt.LootProfileId, Record.Receipt.LootProfileVersion);
		if (Profile && IsP21BasicCorpseR3(*Profile))
		{
			for (const FP21BodyEquipmentSlot& EquipmentSlot : P21BodyEquipmentSlots())
			{
				FCodeBBodyContainerEquipmentSlotProjection& Slot = OutProjection.EquipmentSlots.AddDefaulted_GetRef();
				Slot.SlotSemantic = EquipmentSlot.Semantic;
				Slot.ContainerId = P21BodyEquipmentContainerGuid(
					Record.BodyTargetId, Record.DefinitionId, EquipmentSlot.Semantic);
				Slot.EquipmentSlot = EquipmentSlot.EquipmentSlot;
			}
		}
		return true;
	}

	bool BuildMaterializedBodyContainerRecord(
		const FGuid& OwnerId,
		const FGuid& RunInstanceId,
		const FGuid& BodyTargetId,
		const FCodeBBodyContainerDefinition& Definition,
		const FCodeBBodyContainerDeathReceipt& DeathReceipt,
		FCodeBRunLocalBodyContainerRecord& OutRecord,
		FString& OutError)
	{
		OutRecord = FCodeBRunLocalBodyContainerRecord();
		if (!ValidateBodyContainerDefinition(Definition, OutError)
			|| !ValidateBodyContainerDeathReceipt(DeathReceipt, OwnerId, RunInstanceId, BodyTargetId, Definition.DefinitionId, OutError))
		{
			return false;
		}
		FCodeBLootProfileRollResult Roll;
		if (!BuildDeterministicLootProfileRoll(OwnerId, RunInstanceId, BodyTargetId,
			Definition.DefinitionId, Definition.Capacity, DeathReceipt.DeathReceiptId, Roll, OutError))
		{
			return false;
		}
		const FCodeBLootProfile* Profile = FindLootProfileInternal(Definition.DefinitionId);
		if (!Profile)
		{
			OutError = TEXT("Code B P16 body-container Profile disappeared after validation.");
			return false;
		}
		const bool bP21R3 = IsP21BasicCorpseR3(*Profile);
		if (bP21R3 && !ValidateP21BodyEquipmentCatalog(OutError))
		{
			return false;
		}
		const FString LootIdentity = LootRollIdentityText(OwnerId, RunInstanceId, BodyTargetId,
			Definition.DefinitionId, DeathReceipt.DeathReceiptId, *Profile);
		OutRecord.OwnerId = OwnerId;
		OutRecord.RunInstanceId = RunInstanceId;
		OutRecord.BodyTargetId = BodyTargetId;
		OutRecord.DefinitionId = Definition.DefinitionId;
		OutRecord.ContainerId = StableBodyContainerGuid(OwnerId, RunInstanceId, BodyTargetId, Definition.DefinitionId, Definition.ContentRevision, 2);
		OutRecord.bMaterialized = true;
		OutRecord.State = ECodeBBodyContainerState::BodyMaterialized;
		OutRecord.Revision = 1;

		FCodeBRepository LocalRepository;
		TSet<FName> RegisteredDefinitions;
		for (const FCodeBLootProfileRollEntry& Entry : Roll.Entries)
		{
			if (RegisteredDefinitions.Contains(Entry.ItemDefinitionId)) continue;
			FCodeBItemDefinition ItemDefinition;
			if (!BuildCanonicalCodeBItemDefinition(Entry.ItemDefinitionId, ItemDefinition, OutError)
				|| !LocalRepository.RegisterDefinition(ItemDefinition, &OutError)) return false;
			RegisteredDefinitions.Add(Entry.ItemDefinitionId);
		}
		if (!LocalRepository.CreateContainer(Definition.ContainerType, Definition.Capacity,
			ECodeBContainerKind::Storage, ECodeBEquipSlot::None, &OutError, OutRecord.ContainerId).IsValid())
		{
			return false;
		}
		TSet<FGuid> ReservedGraphIds { OutRecord.ContainerId };
		TMap<FName, FGuid> P21EquipmentContainerIds;
		if (bP21R3)
		{
			for (const FP21BodyEquipmentSlot& EquipmentSlot : P21BodyEquipmentSlots())
			{
				const FGuid ContainerId = P21BodyEquipmentContainerGuid(
					BodyTargetId, Definition.DefinitionId, EquipmentSlot.Semantic);
				if (!ContainerId.IsValid() || ReservedGraphIds.Contains(ContainerId)
					|| !LocalRepository.CreateContainer(EquipmentSlot.ContainerType, 1, ECodeBContainerKind::Equipment,
						EquipmentSlot.EquipmentSlot, &OutError, ContainerId).IsValid())
				{
					if (OutError.IsEmpty()) OutError = TEXT("Code B P21 cannot atomically create the BasicCorpse fixed equipment graph.");
					return false;
				}
				ReservedGraphIds.Add(ContainerId);
				P21EquipmentContainerIds.Add(EquipmentSlot.Semantic, ContainerId);
			}
		}
		int32 P21EquipmentItemCount = 0;
		for (int32 EntryIndex = 0; EntryIndex < Roll.Entries.Num(); ++EntryIndex)
		{
			const FCodeBLootProfileRollEntry& Entry = Roll.Entries[EntryIndex];
			const FGuid ItemId = StableLootGraphGuid(LootIdentity, TEXT("item"), EntryIndex);
			const FP21BodyEquipmentSlot* P21EquipmentSlot = bP21R3
				? FindP21BodyEquipmentSlot(Entry.ItemDefinitionId) : nullptr;
			FCodeBItemDefinition CanonicalDefinition;
			if (!ItemId.IsValid() || ReservedGraphIds.Contains(ItemId)
				|| !BuildCanonicalCodeBItemDefinition(Entry.ItemDefinitionId, CanonicalDefinition, OutError)
				|| (P21EquipmentSlot && (++P21EquipmentItemCount != 1 || Entry.SlotIndex != INDEX_NONE))
				|| !LocalRepository.CreateItem(Entry.ItemDefinitionId, Entry.Quantity,
					P21EquipmentSlot ? P21EquipmentContainerIds.FindChecked(P21EquipmentSlot->Semantic) : OutRecord.ContainerId,
					P21EquipmentSlot ? 0 : Entry.SlotIndex, ItemId, &OutError).IsValid())
			{
				if (OutError.IsEmpty()) OutError = TEXT("Code B P11 body item identity is duplicated or invalid.");
				return false;
			}
			ReservedGraphIds.Add(ItemId);
			if (CanonicalDefinition.SpatialContainerSemantic != ECodeBSpatialContainerSemantic::None)
			{
				const FGuid ChildContainerId = SpatialChildGuid(ItemId);
				const FName ChildContainerType = CanonicalDefinition.SpatialContainerSemantic == ECodeBSpatialContainerSemantic::QuickRing
					? FName(TEXT("CodeB.SpatialChild.QuickRing"))
					: FName(TEXT("CodeB.SpatialChild.StoragePouch"));
				if (!ChildContainerId.IsValid() || ReservedGraphIds.Contains(ChildContainerId)
					|| !LocalRepository.CreateContainer(ChildContainerType, CanonicalDefinition.ChildContainerCapacity,
						ECodeBContainerKind::Storage, ECodeBEquipSlot::None, &OutError, ChildContainerId).IsValid()
					|| !LocalRepository.AssociateChildContainer(ItemId, ChildContainerId, &OutError))
				{
					if (OutError.IsEmpty()) OutError = TEXT("Code B P20 cannot atomically create the BasicCorpse canonical empty spatial child graph.");
					return false;
				}
				ReservedGraphIds.Add(ChildContainerId);
			}
			FCodeBBodyContainerItemVisibility& Visibility = OutRecord.ItemVisibilities.AddDefaulted_GetRef();
			Visibility.ItemId = ItemId;
			Visibility.Visibility = ECodeBBodyContainerVisibility::Hidden;
		}
		OutRecord.ItemVisibilities.Sort([](const FCodeBBodyContainerItemVisibility& Left, const FCodeBBodyContainerItemVisibility& Right)
		{
			return Left.ItemId.ToString(EGuidFormats::DigitsWithHyphensLower) < Right.ItemId.ToString(EGuidFormats::DigitsWithHyphensLower);
		});
		OutRecord.ContainerSnapshot = LocalRepository.CaptureSnapshot();
		OutRecord.Receipt.ReceiptId = StableBodyContainerGuid(OwnerId, RunInstanceId, BodyTargetId, Definition.DefinitionId, Definition.ContentRevision, 1);
		OutRecord.Receipt.OwnerId = OwnerId;
		OutRecord.Receipt.RunInstanceId = RunInstanceId;
		OutRecord.Receipt.BodyTargetId = BodyTargetId;
		OutRecord.Receipt.DefinitionId = Definition.DefinitionId;
		OutRecord.Receipt.ContainerId = OutRecord.ContainerId;
		OutRecord.Receipt.DefinitionContentRevision = Definition.ContentRevision;
		OutRecord.Receipt.DefinitionDigest = BodyContainerDefinitionDigest(Definition);
		OutRecord.Receipt.LootProfileId = Roll.LootProfileId;
		OutRecord.Receipt.LootProfileVersion = Roll.ProfileVersion;
		OutRecord.Receipt.LootProfileDigest = Roll.ProfileDigest;
		OutRecord.Receipt.LootAlgorithmVersion = Roll.AlgorithmVersion;
		OutRecord.Receipt.LootResultDigest = Roll.ResultDigest;
		OutRecord.Receipt.EquipmentCandidateSetDigest = bP21R3 ? P21EquipmentCandidateSetDigest() : FString();
		OutRecord.Receipt.DeathReceipt = DeathReceipt;
		OutRecord.Receipt.MaterializedUtc = UtcNow();
		OutRecord.Receipt.MaterializationDigest = BodyContainerMaterializationDigest(OutRecord);
		return ValidateRunLocalBodyContainerRecord(OutRecord, OutError);
	}

	bool EquivalentRunLocalBodyContainer(
		const FCodeBRunLocalBodyContainerRecord& Left,
		const FCodeBRunLocalBodyContainerRecord& Right)
	{
		if (Left.SchemaVersion != Right.SchemaVersion || Left.OwnerId != Right.OwnerId
			|| Left.RunInstanceId != Right.RunInstanceId || Left.BodyTargetId != Right.BodyTargetId
			|| Left.DefinitionId != Right.DefinitionId || Left.ContainerId != Right.ContainerId
			|| Left.bMaterialized != Right.bMaterialized || Left.State != Right.State
			|| Left.Revision != Right.Revision || Left.ActiveActionId != Right.ActiveActionId
			|| Left.ActiveSearchItemId != Right.ActiveSearchItemId
			|| !(Left.ContainerSnapshot == Right.ContainerSnapshot)
			|| Left.ItemVisibilities.Num() != Right.ItemVisibilities.Num()) return false;
		const FCodeBBodyContainerMaterializationReceipt& L = Left.Receipt;
		const FCodeBBodyContainerMaterializationReceipt& R = Right.Receipt;
		if (L.ReceiptId != R.ReceiptId || L.OwnerId != R.OwnerId || L.RunInstanceId != R.RunInstanceId
			|| L.BodyTargetId != R.BodyTargetId || L.DefinitionId != R.DefinitionId || L.ContainerId != R.ContainerId
			|| L.DefinitionContentRevision != R.DefinitionContentRevision || L.DefinitionDigest != R.DefinitionDigest
			|| L.LootProfileId != R.LootProfileId || L.LootProfileVersion != R.LootProfileVersion
			|| L.LootProfileDigest != R.LootProfileDigest || L.LootAlgorithmVersion != R.LootAlgorithmVersion
			|| L.LootResultDigest != R.LootResultDigest
			|| L.EquipmentCandidateSetDigest != R.EquipmentCandidateSetDigest
			|| L.MaterializationDigest != R.MaterializationDigest || L.MaterializedUtc != R.MaterializedUtc
			|| L.DeathReceipt.DeathReceiptId != R.DeathReceipt.DeathReceiptId
			|| L.DeathReceipt.OwnerId != R.DeathReceipt.OwnerId || L.DeathReceipt.RunInstanceId != R.DeathReceipt.RunInstanceId
			|| L.DeathReceipt.BodyTargetId != R.DeathReceipt.BodyTargetId || L.DeathReceipt.DefinitionId != R.DeathReceipt.DefinitionId
			|| L.DeathReceipt.StaticSpawnIdentity != R.DeathReceipt.StaticSpawnIdentity
			|| L.DeathReceipt.SpawnOrdinal != R.DeathReceipt.SpawnOrdinal
			|| L.DeathReceipt.CommittedUtc != R.DeathReceipt.CommittedUtc) return false;
		for (int32 Index = 0; Index < Left.ItemVisibilities.Num(); ++Index)
		{
			if (Left.ItemVisibilities[Index].ItemId != Right.ItemVisibilities[Index].ItemId
				|| Left.ItemVisibilities[Index].Visibility != Right.ItemVisibilities[Index].Visibility) return false;
		}
		return true;
	}

	bool BuildMaterializedNormalContainerRecord(
		const FGuid& OwnerId,
		const FGuid& RunInstanceId,
		const FGuid& SearchTargetId,
		const FCodeBNormalContainerDefinition& Definition,
		FCodeBRunLocalNormalContainerRecord& OutRecord,
		FString& OutError)
	{
		OutRecord = FCodeBRunLocalNormalContainerRecord();
		if (!ValidateNormalContainerDefinition(Definition, OutError))
		{
			return false;
		}
		FCodeBLootProfileRollResult Roll;
		if (!BuildDeterministicLootProfileRoll(OwnerId, RunInstanceId, SearchTargetId,
			Definition.DefinitionId, Definition.Capacity, FGuid(), Roll, OutError))
		{
			return false;
		}
		const FCodeBLootProfile* Profile = FindLootProfileInternal(Definition.DefinitionId);
		if (!Profile)
		{
			OutError = TEXT("Code B P16 normal-container Profile disappeared after validation.");
			return false;
		}
		const FString LootIdentity = LootRollIdentityText(OwnerId, RunInstanceId, SearchTargetId,
			Definition.DefinitionId, FGuid(), *Profile);
		OutRecord.OwnerId = OwnerId;
		OutRecord.RunInstanceId = RunInstanceId;
		OutRecord.SearchTargetId = SearchTargetId;
		OutRecord.DefinitionId = Definition.DefinitionId;
		OutRecord.ContainerId = StableRunLocalGuid(
			OwnerId, RunInstanceId, SearchTargetId, Definition.DefinitionId, Definition.ContentRevision, 2);
		OutRecord.bMaterialized = true;
		OutRecord.State = ECodeBNormalContainerState::Closed;
		OutRecord.Revision = 1;

		FCodeBRepository LocalRepository;
		TSet<FName> RegisteredDefinitions;
		for (const FCodeBLootProfileRollEntry& Entry : Roll.Entries)
		{
			if (RegisteredDefinitions.Contains(Entry.ItemDefinitionId)) continue;
			FCodeBItemDefinition ItemDefinition;
			if (!BuildCanonicalCodeBItemDefinition(Entry.ItemDefinitionId, ItemDefinition, OutError)
				|| !LocalRepository.RegisterDefinition(ItemDefinition, &OutError))
			{
				return false;
			}
			RegisteredDefinitions.Add(Entry.ItemDefinitionId);
		}
		if (!LocalRepository.CreateContainer(
			Definition.ContainerType, Definition.Capacity, ECodeBContainerKind::Storage,
			ECodeBEquipSlot::None, &OutError, OutRecord.ContainerId).IsValid())
		{
			return false;
		}
		TSet<FGuid> ReservedGraphIds;
		ReservedGraphIds.Add(OutRecord.ContainerId);
		for (int32 EntryIndex = 0; EntryIndex < Roll.Entries.Num(); ++EntryIndex)
		{
			const FCodeBLootProfileRollEntry& Entry = Roll.Entries[EntryIndex];
			const FGuid ItemId = StableLootGraphGuid(LootIdentity, TEXT("item"), EntryIndex);
			FCodeBItemDefinition CanonicalDefinition;
			if (!ItemId.IsValid() || ReservedGraphIds.Contains(ItemId)
				|| !BuildCanonicalCodeBItemDefinition(Entry.ItemDefinitionId, CanonicalDefinition, OutError)
				|| !LocalRepository.CreateItem(
					Entry.ItemDefinitionId, Entry.Quantity, OutRecord.ContainerId, Entry.SlotIndex, ItemId, &OutError).IsValid())
			{
				if (OutError.IsEmpty()) OutError = TEXT("Code B normal-container item identity is duplicated or invalid.");
				return false;
			}
			ReservedGraphIds.Add(ItemId);
			if (CanonicalDefinition.SpatialContainerSemantic != ECodeBSpatialContainerSemantic::None)
			{
				const FGuid ChildContainerId = SpatialChildGuid(ItemId);
				const FName ChildContainerType = CanonicalDefinition.SpatialContainerSemantic == ECodeBSpatialContainerSemantic::QuickRing
					? FName(TEXT("CodeB.SpatialChild.QuickRing"))
					: FName(TEXT("CodeB.SpatialChild.StoragePouch"));
				if (!ChildContainerId.IsValid() || ReservedGraphIds.Contains(ChildContainerId)
					|| !LocalRepository.CreateContainer(
						ChildContainerType, CanonicalDefinition.ChildContainerCapacity,
						ECodeBContainerKind::Storage, ECodeBEquipSlot::None, &OutError, ChildContainerId).IsValid()
					|| !LocalRepository.AssociateChildContainer(ItemId, ChildContainerId, &OutError))
				{
					if (OutError.IsEmpty()) OutError = TEXT("Code B P18 cannot atomically create the canonical spatial child graph.");
					return false;
				}
				ReservedGraphIds.Add(ChildContainerId);
			}
			FCodeBNormalContainerItemReveal& Reveal = OutRecord.ItemRevealStates.AddDefaulted_GetRef();
			Reveal.ItemId = ItemId;
			Reveal.RevealState = ECodeBNormalContainerRevealState::Hidden;
		}
		OutRecord.ItemRevealStates.Sort([](const FCodeBNormalContainerItemReveal& Left, const FCodeBNormalContainerItemReveal& Right)
		{
			return Left.ItemId.ToString(EGuidFormats::DigitsWithHyphensLower) < Right.ItemId.ToString(EGuidFormats::DigitsWithHyphensLower);
		});
		OutRecord.ContainerSnapshot = LocalRepository.CaptureSnapshot();
		OutRecord.Receipt.ReceiptId = StableRunLocalGuid(
			OwnerId, RunInstanceId, SearchTargetId, Definition.DefinitionId, Definition.ContentRevision, 1);
		OutRecord.Receipt.OwnerId = OwnerId;
		OutRecord.Receipt.RunInstanceId = RunInstanceId;
		OutRecord.Receipt.SearchTargetId = SearchTargetId;
		OutRecord.Receipt.DefinitionId = Definition.DefinitionId;
		OutRecord.Receipt.ContainerId = OutRecord.ContainerId;
		OutRecord.Receipt.DefinitionContentRevision = Definition.ContentRevision;
		OutRecord.Receipt.DefinitionDigest = NormalContainerDefinitionDigest(Definition);
		OutRecord.Receipt.LootProfileId = Roll.LootProfileId;
		OutRecord.Receipt.LootProfileVersion = Roll.ProfileVersion;
		OutRecord.Receipt.LootProfileDigest = Roll.ProfileDigest;
		OutRecord.Receipt.LootAlgorithmVersion = Roll.AlgorithmVersion;
		OutRecord.Receipt.LootResultDigest = Roll.ResultDigest;
		OutRecord.Receipt.MaterializedUtc = UtcNow();
		OutRecord.Receipt.MaterializationDigest = NormalContainerMaterializationDigest(OutRecord);
		return ValidateRunLocalNormalContainerRecord(OutRecord, OutError);
	}

	bool IsExactCommittedActiveRun(
		const FCodeBOutOfRaidInventoryRecord& Record,
		const FGuid& OwnerId,
		const FGuid& RunInstanceId)
	{
		return Record.OwnerId == OwnerId
			&& Record.Receipt.State == ECodeBOutOfRaidHandoffState::Committed
			&& Record.bHasActiveRunInventorySession
			&& Record.ActiveRunInventorySession.OwnerId == OwnerId
			&& Record.ActiveRunInventorySession.RunInstanceId == RunInstanceId
			&& Record.ActiveRunInventorySession.BridgeState == ECodeBRunInventoryBridgeState::Committed
			&& Record.ActiveRunInventorySession.Receipt.State == ECodeBRunInventoryBridgeState::Committed;
	}

	bool EquivalentRunLocalNormalContainer(
		const FCodeBRunLocalNormalContainerRecord& Left,
		const FCodeBRunLocalNormalContainerRecord& Right)
	{
		if (Left.SchemaVersion != Right.SchemaVersion || Left.OwnerId != Right.OwnerId
			|| Left.RunInstanceId != Right.RunInstanceId || Left.SearchTargetId != Right.SearchTargetId
			|| Left.DefinitionId != Right.DefinitionId || Left.ContainerId != Right.ContainerId
			|| Left.bMaterialized != Right.bMaterialized || Left.State != Right.State
			|| Left.ActiveActionId != Right.ActiveActionId
			|| Left.ActiveSearchItemId != Right.ActiveSearchItemId
			|| Left.Revision != Right.Revision || !(Left.ContainerSnapshot == Right.ContainerSnapshot)
			|| Left.ItemRevealStates.Num() != Right.ItemRevealStates.Num())
		{
			return false;
		}
		const FCodeBNormalContainerMaterializationReceipt& LeftReceipt = Left.Receipt;
		const FCodeBNormalContainerMaterializationReceipt& RightReceipt = Right.Receipt;
		if (LeftReceipt.ReceiptId != RightReceipt.ReceiptId || LeftReceipt.OwnerId != RightReceipt.OwnerId
			|| LeftReceipt.RunInstanceId != RightReceipt.RunInstanceId || LeftReceipt.SearchTargetId != RightReceipt.SearchTargetId
			|| LeftReceipt.DefinitionId != RightReceipt.DefinitionId || LeftReceipt.ContainerId != RightReceipt.ContainerId
			|| LeftReceipt.DefinitionContentRevision != RightReceipt.DefinitionContentRevision
			|| LeftReceipt.DefinitionDigest != RightReceipt.DefinitionDigest
			|| LeftReceipt.LootProfileId != RightReceipt.LootProfileId
			|| LeftReceipt.LootProfileVersion != RightReceipt.LootProfileVersion
			|| LeftReceipt.LootProfileDigest != RightReceipt.LootProfileDigest
			|| LeftReceipt.LootAlgorithmVersion != RightReceipt.LootAlgorithmVersion
			|| LeftReceipt.LootResultDigest != RightReceipt.LootResultDigest
			|| LeftReceipt.MaterializationDigest != RightReceipt.MaterializationDigest
			|| LeftReceipt.MaterializedUtc != RightReceipt.MaterializedUtc)
		{
			return false;
		}
		for (int32 Index = 0; Index < Left.ItemRevealStates.Num(); ++Index)
		{
			if (Left.ItemRevealStates[Index].ItemId != Right.ItemRevealStates[Index].ItemId
				|| Left.ItemRevealStates[Index].RevealState != Right.ItemRevealStates[Index].RevealState)
			{
				return false;
			}
		}
		return true;
	}

	TSharedRef<FJsonObject> RunInventorySessionJson(const FCodeBRunInventorySession& Session)
	{
		TSharedRef<FJsonObject> Object = MakeShared<FJsonObject>();
		Object->SetNumberField(TEXT("SchemaVersion"), Session.SchemaVersion);
		Object->SetStringField(TEXT("OwnerId"), GuidText(Session.OwnerId));
		Object->SetStringField(TEXT("RunInstanceId"), GuidText(Session.RunInstanceId));
		Object->SetNumberField(TEXT("SessionRevision"), Session.SessionRevision);
		Object->SetNumberField(TEXT("SourceOutOfRaidRevision"), Session.SourceOutOfRaidRevision);
		Object->SetStringField(TEXT("CreatedUtc"), Session.CreatedUtc);
		Object->SetStringField(TEXT("LastCommittedUtc"), Session.LastCommittedUtc);
		Object->SetStringField(TEXT("BridgeState"), Session.BridgeState == ECodeBRunInventoryBridgeState::Committed ? TEXT("Committed") : TEXT("Prepared"));
		TSharedRef<FJsonObject> Receipt = MakeShared<FJsonObject>();
		Receipt->SetStringField(TEXT("State"), Session.Receipt.State == ECodeBRunInventoryBridgeState::Committed ? TEXT("Committed") : TEXT("Prepared"));
		Receipt->SetStringField(TEXT("ReceiptId"), GuidText(Session.Receipt.ReceiptId));
		Receipt->SetStringField(TEXT("OriginRunId"), GuidText(Session.Receipt.OriginRunId));
		Receipt->SetStringField(TEXT("OwnerId"), GuidText(Session.Receipt.OwnerId));
		Receipt->SetStringField(TEXT("RunInstanceId"), GuidText(Session.Receipt.RunInstanceId));
		Receipt->SetNumberField(TEXT("SourceOutOfRaidRevision"), Session.Receipt.SourceOutOfRaidRevision);
		Receipt->SetStringField(TEXT("PayloadDigest"), Session.Receipt.PayloadDigest);
		Receipt->SetStringField(TEXT("PreparedUtc"), Session.Receipt.PreparedUtc);
		Receipt->SetStringField(TEXT("CommittedUtc"), Session.Receipt.CommittedUtc);
		Receipt->SetStringField(TEXT("RecoveryDiagnostic"), Session.Receipt.RecoveryDiagnostic);
		TArray<TSharedPtr<FJsonValue>> Moved;
		for (const FGuid& ItemId : Session.Receipt.MovedItemIds)
		{
			Moved.Add(MakeShared<FJsonValueString>(GuidText(ItemId)));
		}
		Receipt->SetArrayField(TEXT("MovedItemIds"), Moved);
		TArray<TSharedPtr<FJsonValue>> ImmutablePayload;
		for (const FCodeBRunInventoryReceiptItem& Item : Session.Receipt.ImmutablePayloadItems)
		{
			TSharedRef<FJsonObject> ItemObject = MakeShared<FJsonObject>();
			ItemObject->SetStringField(TEXT("ItemId"), GuidText(Item.ItemId));
			ItemObject->SetStringField(TEXT("DefinitionId"), Item.DefinitionId.ToString());
			ItemObject->SetNumberField(TEXT("Quantity"), Item.Quantity);
			ItemObject->SetStringField(TEXT("ParentContainerId"), GuidText(Item.ParentContainerId));
			ItemObject->SetNumberField(TEXT("SlotIndex"), Item.SlotIndex);
			ItemObject->SetStringField(TEXT("ChildContainerId"), GuidText(Item.ChildContainerId));
			ImmutablePayload.Add(MakeShared<FJsonValueObject>(ItemObject));
		}
		Receipt->SetArrayField(TEXT("ImmutablePayloadItems"), ImmutablePayload);
		TArray<TSharedPtr<FJsonValue>> Rebinds;
		for (const FCodeBRunInventoryRecoveryRebind& Rebind : Session.Receipt.RecoveryRebindHistory)
		{
			TSharedRef<FJsonObject> RebindObject = MakeShared<FJsonObject>();
			RebindObject->SetNumberField(TEXT("Sequence"), Rebind.Sequence);
			RebindObject->SetStringField(TEXT("OldRunId"), GuidText(Rebind.OldRunId));
			RebindObject->SetStringField(TEXT("NewRunId"), GuidText(Rebind.NewRunId));
			RebindObject->SetStringField(TEXT("CodeATerminalCause"), Rebind.CodeATerminalCause);
			RebindObject->SetStringField(TEXT("ReboundUtc"), Rebind.ReboundUtc);
			Rebinds.Add(MakeShared<FJsonValueObject>(RebindObject));
		}
		Receipt->SetArrayField(TEXT("RecoveryRebindHistory"), Rebinds);
		Object->SetObjectField(TEXT("Receipt"), Receipt);
		Object->SetObjectField(TEXT("Layout"), LayoutJson(Session.Layout));
		Object->SetObjectField(TEXT("RepositorySnapshot"), SnapshotJson(Session.RepositorySnapshot));
		Object->SetArrayField(TEXT("HotbarBindings"), HotbarBindingsJson(Session.HotbarBindings));
		TArray<TSharedPtr<FJsonValue>> WorldDrops;
		TArray<FCodeBWorldDropRecord> CanonicalWorldDrops = Session.WorldDrops;
		SortWorldDropRegistry(CanonicalWorldDrops);
		for (const FCodeBWorldDropRecord& Record : CanonicalWorldDrops)
		{
			WorldDrops.Add(MakeShared<FJsonValueObject>(WorldDropJson(Record)));
		}
		Object->SetArrayField(TEXT("WorldDrops"), WorldDrops);
		Object->SetNumberField(TEXT("NextWorldDropOrdinal"), Session.NextWorldDropOrdinal);
		TArray<TSharedPtr<FJsonValue>> QuickUseReceipts;
		for (const FCodeBQuickUseReceipt& QuickReceipt : Session.QuickUseReceipts)
		{
			TSharedRef<FJsonObject> ReceiptObject = MakeShared<FJsonObject>();
			ReceiptObject->SetStringField(TEXT("ReceiptId"), GuidText(QuickReceipt.ReceiptId));
			ReceiptObject->SetNumberField(TEXT("Ordinal"), QuickReceipt.ReceiptOrdinal);
			ReceiptObject->SetNumberField(TEXT("SlotIndex"), QuickReceipt.SlotIndex);
			ReceiptObject->SetStringField(TEXT("SourceItemId"), GuidText(QuickReceipt.SourceItemId));
			ReceiptObject->SetNumberField(TEXT("EffectKind"), static_cast<int32>(QuickReceipt.EffectKind));
			ReceiptObject->SetNumberField(TEXT("RestoreAmount"), QuickReceipt.RestoreAmount);
			ReceiptObject->SetStringField(TEXT("State"), QuickUseReceiptStateText(QuickReceipt.State));
			QuickUseReceipts.Add(MakeShared<FJsonValueObject>(ReceiptObject));
		}
		Object->SetArrayField(TEXT("QuickUseReceipts"), QuickUseReceipts);
		Object->SetNumberField(TEXT("NextQuickUseReceiptOrdinal"), Session.NextQuickUseReceiptOrdinal);
		return Object;
	}

	TSharedRef<FJsonObject> RunInventoryTerminalReceiptJson(
		const FCodeBRunInventoryTerminalReceipt& Receipt)
	{
		TSharedRef<FJsonObject> Object = MakeShared<FJsonObject>();
		Object->SetStringField(TEXT("ReceiptId"), GuidText(Receipt.ReceiptId));
		Object->SetStringField(TEXT("OwnerId"), GuidText(Receipt.OwnerId));
		Object->SetStringField(TEXT("RunInstanceId"), GuidText(Receipt.RunInstanceId));
		Object->SetStringField(TEXT("TerminalState"), TerminalStateText(Receipt.TerminalState));
		Object->SetNumberField(TEXT("SourceSessionRevision"), Receipt.SourceSessionRevision);
		Object->SetStringField(TEXT("CommittedUtc"), Receipt.CommittedUtc);
		Object->SetStringField(TEXT("FrozenSnapshotDigest"), Receipt.FrozenSnapshotDigest);
		Object->SetObjectField(TEXT("FrozenRunLayout"), LayoutJson(Receipt.FrozenRunLayout));
		Object->SetObjectField(TEXT("FrozenRunSnapshot"), SnapshotJson(Receipt.FrozenRunSnapshot));
		return Object;
	}

	bool JsonToRunInventoryTerminalReceipt(
		const TSharedPtr<FJsonObject>& Object,
		FCodeBRunInventoryTerminalReceipt& OutReceipt,
		FString& OutError)
	{
		OutReceipt = FCodeBRunInventoryTerminalReceipt();
		FString ReceiptId, OwnerId, RunInstanceId, State, CommittedUtc, Digest;
		const TSharedPtr<FJsonObject>* LayoutObject = nullptr;
		const TSharedPtr<FJsonObject>* SnapshotObject = nullptr;
		if (!Object.IsValid()
			|| !Object->TryGetStringField(TEXT("ReceiptId"), ReceiptId)
			|| !Object->TryGetStringField(TEXT("OwnerId"), OwnerId)
			|| !Object->TryGetStringField(TEXT("RunInstanceId"), RunInstanceId)
			|| !Object->TryGetStringField(TEXT("TerminalState"), State)
			|| !Object->TryGetStringField(TEXT("CommittedUtc"), CommittedUtc)
			|| !Object->TryGetStringField(TEXT("FrozenSnapshotDigest"), Digest)
			|| !Object->TryGetObjectField(TEXT("FrozenRunLayout"), LayoutObject)
			|| !Object->TryGetObjectField(TEXT("FrozenRunSnapshot"), SnapshotObject)
			|| !ReadInt(Object, TEXT("SourceSessionRevision"), OutReceipt.SourceSessionRevision, OutError)
			|| !TryGuidText(ReceiptId, OutReceipt.ReceiptId) || !OutReceipt.ReceiptId.IsValid()
			|| !TryGuidText(OwnerId, OutReceipt.OwnerId) || !OutReceipt.OwnerId.IsValid()
			|| !TryGuidText(RunInstanceId, OutReceipt.RunInstanceId) || !OutReceipt.RunInstanceId.IsValid()
			|| !TryTerminalState(State, OutReceipt.TerminalState)
			|| CommittedUtc.IsEmpty() || Digest.IsEmpty()
			|| !JsonToLayout(*LayoutObject, OutReceipt.FrozenRunLayout, OutError, false)
			|| !JsonToSnapshot(*SnapshotObject, OutReceipt.FrozenRunSnapshot, OutError))
		{
			if (OutError.IsEmpty()) OutError = TEXT("Code B Run terminal receipt JSON is invalid.");
			return false;
		}
		OutReceipt.CommittedUtc = CommittedUtc;
		OutReceipt.FrozenSnapshotDigest = Digest;
		return true;
	}

	bool JsonToRunInventorySession(
		const TSharedPtr<FJsonObject>& Object,
		FCodeBRunInventorySession& OutSession,
		FString& OutError)
	{
		OutSession = FCodeBRunInventorySession();
		double Schema = 0.0, SessionRevision = 0.0, SourceRevision = 0.0;
		FString Owner, Run, Created, Committed, BridgeState;
		const TSharedPtr<FJsonObject> *ReceiptObject = nullptr, *LayoutObject = nullptr, *SnapshotObject = nullptr;
		const TArray<TSharedPtr<FJsonValue>>* HotbarValues = nullptr;
		const TArray<TSharedPtr<FJsonValue>>* WorldDropValues = nullptr;
		double NextWorldDropOrdinal = 1.0, NextQuickUseReceiptOrdinal = 1.0;
		const TArray<TSharedPtr<FJsonValue>>* QuickUseReceiptValues = nullptr;
		if (!Object.IsValid()
			|| !Object->TryGetNumberField(TEXT("SchemaVersion"), Schema)
			|| !Object->TryGetStringField(TEXT("OwnerId"), Owner)
			|| !Object->TryGetStringField(TEXT("RunInstanceId"), Run)
			|| !Object->TryGetNumberField(TEXT("SessionRevision"), SessionRevision)
			|| !Object->TryGetNumberField(TEXT("SourceOutOfRaidRevision"), SourceRevision)
			|| !Object->TryGetStringField(TEXT("CreatedUtc"), Created)
			|| !Object->TryGetStringField(TEXT("LastCommittedUtc"), Committed)
			|| !Object->TryGetStringField(TEXT("BridgeState"), BridgeState)
			|| !Object->TryGetObjectField(TEXT("Receipt"), ReceiptObject)
			|| !Object->TryGetObjectField(TEXT("Layout"), LayoutObject)
			|| !Object->TryGetObjectField(TEXT("RepositorySnapshot"), SnapshotObject)
			|| !FMath::IsNearlyEqual(Schema, FMath::RoundToDouble(Schema))
			|| !FMath::IsNearlyEqual(SessionRevision, FMath::RoundToDouble(SessionRevision))
			|| !FMath::IsNearlyEqual(SourceRevision, FMath::RoundToDouble(SourceRevision))
			|| !TryGuidText(Owner, OutSession.OwnerId)
			|| !TryGuidText(Run, OutSession.RunInstanceId)
			|| !OutSession.OwnerId.IsValid() || !OutSession.RunInstanceId.IsValid()
			|| (static_cast<int32>(Schema) < 1
				|| static_cast<int32>(Schema) > FCodeBRunInventorySession::CurrentSchemaVersion)
			|| (BridgeState != TEXT("Prepared") && BridgeState != TEXT("Committed")))
		{
			OutError = TEXT("Code B Run inventory session JSON is invalid.");
			return false;
		}
		const int32 StoredSchemaVersion = static_cast<int32>(Schema);
		OutSession.SchemaVersion = FCodeBRunInventorySession::CurrentSchemaVersion;
		OutSession.SessionRevision = static_cast<int32>(SessionRevision);
		OutSession.SourceOutOfRaidRevision = static_cast<int32>(SourceRevision);
		OutSession.CreatedUtc = Created;
		OutSession.LastCommittedUtc = Committed;
		OutSession.BridgeState = BridgeState == TEXT("Committed") ? ECodeBRunInventoryBridgeState::Committed : ECodeBRunInventoryBridgeState::Prepared;
		if (!JsonToLayout(*LayoutObject, OutSession.Layout, OutError, false)
			|| !JsonToSnapshot(*SnapshotObject, OutSession.RepositorySnapshot, OutError))
		{
			return false;
		}
		const TSharedPtr<FJsonObject> Receipt = *ReceiptObject;
		double ReceiptSourceRevision = 0.0;
		FString ReceiptState, ReceiptOwner, ReceiptRun, Prepared, ReceiptCommitted, Recovery;
		FString ReceiptId, OriginRunId, PayloadDigest;
		const TArray<TSharedPtr<FJsonValue>>* Moved = nullptr;
		if (!Receipt.IsValid()
			|| !Receipt->TryGetStringField(TEXT("State"), ReceiptState)
			|| !Receipt->TryGetStringField(TEXT("OwnerId"), ReceiptOwner)
			|| !Receipt->TryGetStringField(TEXT("RunInstanceId"), ReceiptRun)
			|| !Receipt->TryGetNumberField(TEXT("SourceOutOfRaidRevision"), ReceiptSourceRevision)
			|| !Receipt->TryGetStringField(TEXT("PreparedUtc"), Prepared)
			|| !Receipt->TryGetStringField(TEXT("CommittedUtc"), ReceiptCommitted)
			|| !Receipt->TryGetStringField(TEXT("RecoveryDiagnostic"), Recovery)
			|| !Receipt->TryGetArrayField(TEXT("MovedItemIds"), Moved)
			|| !FMath::IsNearlyEqual(ReceiptSourceRevision, FMath::RoundToDouble(ReceiptSourceRevision))
			|| !TryGuidText(ReceiptOwner, OutSession.Receipt.OwnerId)
			|| !TryGuidText(ReceiptRun, OutSession.Receipt.RunInstanceId)
			|| (ReceiptState != TEXT("Prepared") && ReceiptState != TEXT("Committed")))
		{
			OutError = TEXT("Code B Run inventory receipt JSON is invalid.");
			return false;
		}
		OutSession.Receipt.State = ReceiptState == TEXT("Committed") ? ECodeBRunInventoryBridgeState::Committed : ECodeBRunInventoryBridgeState::Prepared;
		OutSession.Receipt.SourceOutOfRaidRevision = static_cast<int32>(ReceiptSourceRevision);
		OutSession.Receipt.PreparedUtc = Prepared;
		OutSession.Receipt.CommittedUtc = ReceiptCommitted;
		OutSession.Receipt.RecoveryDiagnostic = Recovery;
		if (StoredSchemaVersion >= 2)
		{
			if (!Receipt->TryGetStringField(TEXT("ReceiptId"), ReceiptId)
				|| !Receipt->TryGetStringField(TEXT("OriginRunId"), OriginRunId)
				|| !Receipt->TryGetStringField(TEXT("PayloadDigest"), PayloadDigest)
				|| !TryGuidText(ReceiptId, OutSession.Receipt.ReceiptId) || !OutSession.Receipt.ReceiptId.IsValid()
				|| !TryGuidText(OriginRunId, OutSession.Receipt.OriginRunId) || !OutSession.Receipt.OriginRunId.IsValid()
				|| PayloadDigest.IsEmpty())
			{
				OutError = TEXT("Code B Run inventory v2 receipt identity is invalid.");
				return false;
			}
			OutSession.Receipt.PayloadDigest = PayloadDigest;
			const TArray<TSharedPtr<FJsonValue>>* Rebinds = nullptr;
			if (!Receipt->TryGetArrayField(TEXT("RecoveryRebindHistory"), Rebinds) || !Rebinds)
			{
				OutError = TEXT("Code B Run inventory v2 recovery history is missing.");
				return false;
			}
			for (const TSharedPtr<FJsonValue>& Value : *Rebinds)
			{
				const TSharedPtr<FJsonObject> Rebind = Value.IsValid() ? Value->AsObject() : nullptr;
				FCodeBRunInventoryRecoveryRebind Entry;
				if (!ReadInt(Rebind, TEXT("Sequence"), Entry.Sequence, OutError)
					|| !ReadGuid(Rebind, TEXT("OldRunId"), Entry.OldRunId, OutError, true)
					|| !ReadGuid(Rebind, TEXT("NewRunId"), Entry.NewRunId, OutError, true)
					|| !Rebind->TryGetStringField(TEXT("CodeATerminalCause"), Entry.CodeATerminalCause)
					|| !Rebind->TryGetStringField(TEXT("ReboundUtc"), Entry.ReboundUtc))
				{
					if (OutError.IsEmpty()) OutError = TEXT("Code B Run inventory recovery history is invalid.");
					return false;
				}
				OutSession.Receipt.RecoveryRebindHistory.Add(MoveTemp(Entry));
			}
		}
		else
		{
			// P6r2 receipts were valid but had no reusable identity.  Their original
			// RunId is deterministically promoted to both immutable values on read.
			OutSession.Receipt.ReceiptId = OutSession.RunInstanceId;
			OutSession.Receipt.OriginRunId = OutSession.RunInstanceId;
		}
		if (StoredSchemaVersion >= 3)
		{
			if (!Object->TryGetArrayField(TEXT("HotbarBindings"), HotbarValues) || !HotbarValues
				|| !JsonToHotbarBindings(*HotbarValues, OutSession.HotbarBindings, OutError))
			{
				if (OutError.IsEmpty()) OutError = TEXT("Code B P13 active Run hotbar JSON is missing or invalid.");
				return false;
			}
		}
		else if (!ReconcileHotbarBindings(OutSession.HotbarBindings, OutSession.RepositorySnapshot, OutSession.Layout, OutError))
		{
			return false;
		}
		if (StoredSchemaVersion >= 4)
		{
			if (!Object->TryGetArrayField(TEXT("WorldDrops"), WorldDropValues) || !WorldDropValues
				|| !Object->TryGetNumberField(TEXT("NextWorldDropOrdinal"), NextWorldDropOrdinal)
				|| !FMath::IsNearlyEqual(NextWorldDropOrdinal, FMath::RoundToDouble(NextWorldDropOrdinal))
				|| NextWorldDropOrdinal < 1.0 || NextWorldDropOrdinal > MAX_int32)
			{
				OutError = TEXT("Code B P14 world-drop migration fields are invalid.");
				return false;
			}
			OutSession.NextWorldDropOrdinal = static_cast<int32>(NextWorldDropOrdinal);
			for (const TSharedPtr<FJsonValue>& Value : *WorldDropValues)
			{
				FCodeBWorldDropRecord Record;
				if (!JsonToWorldDrop(Value.IsValid() ? Value->AsObject() : nullptr,
					StoredSchemaVersion >= 6, Record, OutError)) return false;
				OutSession.WorldDrops.Add(MoveTemp(Record));
			}
		}
		else
		{
			// P14's migration is deliberately one-way: accepted P5/P6 records begin
			// with no world authority, and the next durable write serializes schema 4.
			OutSession.WorldDrops.Reset();
			OutSession.NextWorldDropOrdinal = 1;
		}
		if (!UpgradeWorldDropRegistry(OutSession, StoredSchemaVersion, OutError))
		{
			return false;
		}
		if (StoredSchemaVersion >= 5)
		{
			if (!Object->TryGetArrayField(TEXT("QuickUseReceipts"), QuickUseReceiptValues) || !QuickUseReceiptValues
				|| !Object->TryGetNumberField(TEXT("NextQuickUseReceiptOrdinal"), NextQuickUseReceiptOrdinal)
				|| !FMath::IsNearlyEqual(NextQuickUseReceiptOrdinal, FMath::RoundToDouble(NextQuickUseReceiptOrdinal))
				|| NextQuickUseReceiptOrdinal < 1.0 || NextQuickUseReceiptOrdinal > MAX_int32)
			{
				OutError = TEXT("Code B P15 quick-use receipt migration fields are invalid.");
				return false;
			}
			OutSession.NextQuickUseReceiptOrdinal = static_cast<int32>(NextQuickUseReceiptOrdinal);
			for (const TSharedPtr<FJsonValue>& Value : *QuickUseReceiptValues)
			{
				const TSharedPtr<FJsonObject> ReceiptValue = Value.IsValid() ? Value->AsObject() : nullptr;
				FCodeBQuickUseReceipt ReceiptEntry;
				FString StateText;
				int32 EffectValue = 0;
				if (!ReadGuid(ReceiptValue, TEXT("ReceiptId"), ReceiptEntry.ReceiptId, OutError, true)
					|| !ReadInt(ReceiptValue, TEXT("Ordinal"), ReceiptEntry.ReceiptOrdinal, OutError)
					|| !ReadInt(ReceiptValue, TEXT("SlotIndex"), ReceiptEntry.SlotIndex, OutError)
					|| !ReadGuid(ReceiptValue, TEXT("SourceItemId"), ReceiptEntry.SourceItemId, OutError, true)
					|| !ReadInt(ReceiptValue, TEXT("EffectKind"), EffectValue, OutError)
					|| !ReadInt(ReceiptValue, TEXT("RestoreAmount"), ReceiptEntry.RestoreAmount, OutError)
					|| !ReceiptValue->TryGetStringField(TEXT("State"), StateText)
					|| !TryQuickUseReceiptState(StateText, ReceiptEntry.State))
				{
					if (OutError.IsEmpty()) OutError = TEXT("Code B P15 quick-use receipt JSON is invalid.");
					return false;
				}
				ReceiptEntry.EffectKind = static_cast<ECodeBQuickUseEffectKind>(EffectValue);
				OutSession.QuickUseReceipts.Add(MoveTemp(ReceiptEntry));
			}
		}
		else
		{
			// P15 migration starts every prior committed P6 session with no delivery debt.
			OutSession.QuickUseReceipts.Reset();
			OutSession.NextQuickUseReceiptOrdinal = 1;
		}
		for (const TSharedPtr<FJsonValue>& Value : *Moved)
		{
			FGuid ItemId;
			if (!Value.IsValid() || Value->Type != EJson::String || !TryGuidText(Value->AsString(), ItemId) || !ItemId.IsValid())
			{
				OutError = TEXT("Code B Run inventory moved-item JSON is invalid.");
				return false;
			}
			OutSession.Receipt.MovedItemIds.Add(ItemId);
		}
		const TArray<TSharedPtr<FJsonValue>>* ImmutablePayload = nullptr;
		if (Receipt->TryGetArrayField(TEXT("ImmutablePayloadItems"), ImmutablePayload) && ImmutablePayload)
		{
			for (const TSharedPtr<FJsonValue>& Value : *ImmutablePayload)
			{
				const TSharedPtr<FJsonObject> Item = Value.IsValid() ? Value->AsObject() : nullptr;
				FCodeBRunInventoryReceiptItem FrozenItem;
				FString FrozenDefinitionId;
				if (!ReadGuid(Item, TEXT("ItemId"), FrozenItem.ItemId, OutError, true)
					|| !Item->TryGetStringField(TEXT("DefinitionId"), FrozenDefinitionId)
					|| !ReadInt(Item, TEXT("Quantity"), FrozenItem.Quantity, OutError)
					|| !ReadGuid(Item, TEXT("ParentContainerId"), FrozenItem.ParentContainerId, OutError, true)
					|| !ReadInt(Item, TEXT("SlotIndex"), FrozenItem.SlotIndex, OutError)
					|| !ReadGuid(Item, TEXT("ChildContainerId"), FrozenItem.ChildContainerId, OutError, false))
				{
					if (OutError.IsEmpty()) OutError = TEXT("Code B Run inventory immutable receipt item is invalid.");
					return false;
				}
				FrozenItem.DefinitionId = FName(*FrozenDefinitionId);
				OutSession.Receipt.ImmutablePayloadItems.Add(MoveTemp(FrozenItem));
			}
		}
		if (StoredSchemaVersion == 1)
		{
			OutSession.Receipt.PayloadDigest = RunInventoryPayloadDigest(OutSession);
		}
		return true;
	}

	bool ValidateRunInventorySession(const FCodeBRunInventorySession& Session, FString& OutError)
	{
		if (Session.SchemaVersion != FCodeBRunInventorySession::CurrentSchemaVersion
			|| !Session.OwnerId.IsValid() || !Session.RunInstanceId.IsValid()
			|| Session.SessionRevision < 1 || Session.SourceOutOfRaidRevision < 1
			|| !Session.Receipt.ReceiptId.IsValid() || !Session.Receipt.OriginRunId.IsValid()
			|| Session.Receipt.PayloadDigest.IsEmpty()
			|| Session.Receipt.OwnerId != Session.OwnerId || Session.Receipt.RunInstanceId != Session.RunInstanceId
			|| Session.Receipt.SourceOutOfRaidRevision != Session.SourceOutOfRaidRevision
			|| Session.Receipt.State != Session.BridgeState)
		{
			OutError = TEXT("Code B Run inventory session identity or receipt is invalid.");
			return false;
		}
		if (Session.NextQuickUseReceiptOrdinal < 1)
		{
			OutError = TEXT("Code B P15 quick-use receipt ordinal is invalid.");
			return false;
		}
		TSet<FGuid> QuickReceiptIds;
		TSet<int32> QuickReceiptOrdinals;
		TSet<FGuid> ConsumedSourceItemIds;
		for (const FCodeBQuickUseReceipt& QuickReceipt : Session.QuickUseReceipts)
		{
			if (!QuickReceipt.ReceiptId.IsValid() || !QuickReceipt.SourceItemId.IsValid()
				|| QuickReceipt.ReceiptOrdinal < 1 || QuickReceipt.ReceiptOrdinal >= Session.NextQuickUseReceiptOrdinal
				|| QuickReceipt.SlotIndex < 1 || QuickReceipt.SlotIndex > FCodeBHotbarBindings::SlotCount
				|| QuickReceipt.EffectKind != ECodeBQuickUseEffectKind::RestoreHealth
				|| QuickReceipt.RestoreAmount <= 0
				|| (QuickReceipt.State != ECodeBQuickUseReceiptState::Pending
					&& QuickReceipt.State != ECodeBQuickUseReceiptState::Acknowledged)
				|| QuickReceipt.ReceiptId != QuickUseReceiptGuid(Session.OwnerId, Session.RunInstanceId, QuickReceipt.ReceiptOrdinal)
				|| QuickReceiptIds.Contains(QuickReceipt.ReceiptId)
				|| QuickReceiptOrdinals.Contains(QuickReceipt.ReceiptOrdinal))
			{
				OutError = TEXT("Code B P15 quick-use receipt ledger is invalid.");
				return false;
			}
			QuickReceiptIds.Add(QuickReceipt.ReceiptId);
			QuickReceiptOrdinals.Add(QuickReceipt.ReceiptOrdinal);
			ConsumedSourceItemIds.Add(QuickReceipt.SourceItemId);
		}
		TSet<FGuid> Seen;
		for (const FGuid& ItemId : Session.Receipt.MovedItemIds)
		{
			if (!ItemId.IsValid() || Seen.Contains(ItemId)
				|| (!Session.RepositorySnapshot.Items.Contains(ItemId) && !ConsumedSourceItemIds.Contains(ItemId)))
			{
				OutError = TEXT("Code B Run inventory receipt has a duplicate or missing moved item.");
				return false;
			}
			Seen.Add(ItemId);
		}
		if (!Session.Receipt.ImmutablePayloadItems.IsEmpty())
		{
			if (Session.Receipt.ImmutablePayloadItems.Num() != Session.Receipt.MovedItemIds.Num())
			{
				OutError = TEXT("Code B Run inventory immutable receipt item count does not match its moved-item list.");
				return false;
			}
			TSet<FGuid> ImmutableSeen;
			for (const FCodeBRunInventoryReceiptItem& Item : Session.Receipt.ImmutablePayloadItems)
			{
				if (!Item.ItemId.IsValid() || !Seen.Contains(Item.ItemId) || ImmutableSeen.Contains(Item.ItemId)
					|| Item.DefinitionId.IsNone() || Item.Quantity <= 0 || !Item.ParentContainerId.IsValid() || Item.SlotIndex < 0)
				{
					OutError = TEXT("Code B Run inventory immutable receipt item is invalid.");
					return false;
				}
				ImmutableSeen.Add(Item.ItemId);
			}
		}
		if (Session.Receipt.PayloadDigest != RunInventoryPayloadDigest(Session))
		{
			OutError = TEXT("Code B Run inventory receipt payload digest does not match its immutable item graph.");
			return false;
		}
		FGuid ExpectedRunId = Session.Receipt.OriginRunId;
		for (int32 Index = 0; Index < Session.Receipt.RecoveryRebindHistory.Num(); ++Index)
		{
			const FCodeBRunInventoryRecoveryRebind& Rebind = Session.Receipt.RecoveryRebindHistory[Index];
			if (Rebind.Sequence != Index + 1 || Rebind.OldRunId != ExpectedRunId
				|| !Rebind.NewRunId.IsValid() || Rebind.NewRunId == Rebind.OldRunId
				|| Rebind.CodeATerminalCause != TEXT("RecoveredAbandon") || Rebind.ReboundUtc.IsEmpty())
			{
				OutError = TEXT("Code B Run inventory receipt recovery history is not a continuous recovered-abandon chain.");
				return false;
			}
			ExpectedRunId = Rebind.NewRunId;
		}
		if (ExpectedRunId != Session.RunInstanceId)
		{
			OutError = TEXT("Code B Run inventory receipt current RunId does not match its recovery history.");
			return false;
		}
		FCodeBRepository Repository;
		if (!Repository.LoadPersistedSnapshot(Session.RepositorySnapshot, &OutError)) return false;
		if (!ValidateHotbarBindingsAgainstSnapshot(
			Session.HotbarBindings, Session.RepositorySnapshot, Session.Layout, OutError))
		{
			return false;
		}
		if (!ValidateWorldDrops(Session, OutError)) return false;
		FCodeBP2Projection Projection;
		return FCodeBP2ProjectionBuilder::Build(Repository, Session.Layout, Projection, nullptr, &OutError);
	}

	bool ValidateRunInventoryTerminalReceipt(
		const FCodeBRunInventoryTerminalReceipt& Receipt,
		FString& OutError)
	{
		if (!Receipt.ReceiptId.IsValid() || !Receipt.OwnerId.IsValid()
			|| !Receipt.RunInstanceId.IsValid()
			|| Receipt.TerminalState == ECodeBRunInventoryTerminalState::Unknown
			|| Receipt.SourceSessionRevision < 1 || Receipt.CommittedUtc.IsEmpty()
			|| Receipt.FrozenSnapshotDigest.IsEmpty()
			|| Receipt.FrozenSnapshotDigest != TerminalSnapshotDigest(
				Receipt.FrozenRunSnapshot, Receipt.FrozenRunLayout))
		{
			OutError = TEXT("Code B Run terminal receipt identity or frozen graph is invalid.");
			return false;
		}
		FCodeBRepository Repository;
		if (!Repository.LoadPersistedSnapshot(Receipt.FrozenRunSnapshot, &OutError))
		{
			return false;
		}
		FCodeBP2Projection Projection;
		return FCodeBP2ProjectionBuilder::Build(
			Repository, Receipt.FrozenRunLayout, Projection, nullptr, &OutError);
	}

	TSet<FGuid> CarryRootContainerIds(const FCodeBP2PlayerLayout& Layout)
	{
		TSet<FGuid> Result;
		const auto Add = [&Result](const FGuid& ContainerId)
		{
			if (ContainerId.IsValid()) Result.Add(ContainerId);
		};
		Add(Layout.BasicContainerId);
		Add(Layout.WeaponContainerId);
		Add(Layout.ArmorContainerId);
		Add(Layout.SpatialContainerId);
		Add(Layout.BackpackContainerId);
		for (const FGuid& ContainerId : Layout.AccessoryContainerIds) Add(ContainerId);
		return Result;
	}

	bool BuildP14PlayerOnlySession(
		const FCodeBRunInventorySession& Source,
		FCodeBRunInventorySession& OutSession,
		FString& OutError)
	{
		OutSession = Source;
		for (const FCodeBWorldDropRecord& Record : Source.WorldDrops)
		{
			if (!DiscardP19WorldDropClosure(OutSession.RepositorySnapshot, Record, OutError)) return false;
		}
		OutSession.WorldDrops.Reset();
		FCodeBRepository Validation;
		return Validation.LoadPersistedSnapshot(OutSession.RepositorySnapshot, &OutError);
	}

	bool MergeCurrentRunInventoryIntoOutOfRaid(
		FCodeBSnapshot& InOutOutOfRaidSnapshot,
		const FCodeBP2PlayerLayout& OutOfRaidLayout,
		const FCodeBRunInventorySession& Session,
		FString& OutError)
	{
		const TSet<FGuid> CarryRoots = CarryRootContainerIds(Session.Layout);
		for (const FGuid& ContainerId : CarryRoots)
		{
			const FCodeBContainer* const RunContainer =
				Session.RepositorySnapshot.Containers.Find(ContainerId);
			FCodeBContainer* const ProfileContainer =
				InOutOutOfRaidSnapshot.Containers.Find(ContainerId);
			if (!RunContainer || !ProfileContainer)
			{
				OutError = TEXT("Code B P8 return requires matching P5/P6 carry root containers.");
				return false;
			}
			for (const FGuid& ExistingItemId : ProfileContainer->Slots)
			{
				if (ExistingItemId.IsValid())
				{
					OutError = TEXT("Code B P8 return refused a non-empty P5 carry root.");
					return false;
				}
			}
		}
		for (const TPair<FName, FCodeBItemDefinition>& Definition : Session.RepositorySnapshot.Definitions)
		{
			if (!InOutOutOfRaidSnapshot.Definitions.Contains(Definition.Key))
			{
				OutError = TEXT("Code B P8 return found a P6 definition missing from P5.");
				return false;
			}
		}
		for (const TPair<FGuid, FCodeBItemInstance>& Item : Session.RepositorySnapshot.Items)
		{
			if (InOutOutOfRaidSnapshot.Items.Contains(Item.Key))
			{
				OutError = TEXT("Code B P8 return refused to duplicate an existing P5 ItemId.");
				return false;
			}
		}
		for (const TPair<FGuid, FCodeBContainer>& Container : Session.RepositorySnapshot.Containers)
		{
			if (!CarryRoots.Contains(Container.Key)
				&& InOutOutOfRaidSnapshot.Containers.Contains(Container.Key))
			{
				OutError = TEXT("Code B P8 return found a conflicting P5 ChildContainerId.");
				return false;
			}
		}
		if (InOutOutOfRaidSnapshot.Revision == MAX_int32)
		{
			OutError = TEXT("Code B P8 return cannot advance the P5 repository revision.");
			return false;
		}
		for (const TPair<FGuid, FCodeBContainer>& Container : Session.RepositorySnapshot.Containers)
		{
			InOutOutOfRaidSnapshot.Containers.Add(Container.Key, Container.Value);
		}
		for (const TPair<FGuid, FCodeBItemInstance>& Item : Session.RepositorySnapshot.Items)
		{
			InOutOutOfRaidSnapshot.Items.Add(Item.Key, Item.Value);
		}
		++InOutOutOfRaidSnapshot.Revision;
		FCodeBRepository Repository;
		if (!Repository.LoadPersistedSnapshot(InOutOutOfRaidSnapshot, &OutError))
		{
			return false;
		}
		FCodeBP2Projection Projection;
		return FCodeBP2ProjectionBuilder::Build(
			Repository, OutOfRaidLayout, Projection, nullptr, &OutError);
	}

	bool BuildRunInventorySession(
		const FCodeBOutOfRaidInventoryRecord& Record,
		const FGuid& RunInstanceId,
		FCodeBRunInventorySession& OutSession,
		FString& OutError)
	{
		OutSession = FCodeBRunInventorySession();
		if (!RunInstanceId.IsValid() || Record.PersistentRevision < 1)
		{
			OutError = TEXT("Code B Run bridge received an invalid committed identity.");
			return false;
		}
		OutSession.OwnerId = Record.OwnerId;
		OutSession.RunInstanceId = RunInstanceId;
		OutSession.SessionRevision = 1;
		OutSession.SourceOutOfRaidRevision = Record.PersistentRevision;
		OutSession.CreatedUtc = UtcNow();
		OutSession.LastCommittedUtc = FString();
		OutSession.BridgeState = ECodeBRunInventoryBridgeState::Prepared;
		OutSession.Receipt.State = ECodeBRunInventoryBridgeState::Prepared;
		OutSession.Receipt.ReceiptId = FGuid::NewGuid();
		OutSession.Receipt.OriginRunId = RunInstanceId;
		OutSession.Receipt.OwnerId = Record.OwnerId;
		OutSession.Receipt.RunInstanceId = RunInstanceId;
		OutSession.Receipt.SourceOutOfRaidRevision = Record.PersistentRevision;
		OutSession.Receipt.PreparedUtc = OutSession.CreatedUtc;
		OutSession.Layout = Record.Layout;
		OutSession.Layout.LayoutId = FName(TEXT("CodeB.RunInventory.ReadOnly"));
		OutSession.Layout.WarehouseContainerId.Invalidate();
		OutSession.Layout.bUseConditionalSpatialContainers = true;
		OutSession.RepositorySnapshot.Revision = 1;
		OutSession.RepositorySnapshot.Definitions = Record.RepositorySnapshot.Definitions;

		TSet<FGuid> VisitedContainers;
		TSet<FGuid> MovedItems;
		const auto CopyContainer = [&Record, &OutSession, &VisitedContainers, &MovedItems, &OutError](auto&& Self, const FGuid& ContainerId) -> bool
		{
			if (!ContainerId.IsValid() || VisitedContainers.Contains(ContainerId)) return true;
			const FCodeBContainer* Container = Record.RepositorySnapshot.Containers.Find(ContainerId);
			if (!Container)
			{
				OutError = TEXT("Code B Run bridge found a carry container missing from the committed Profile snapshot.");
				return false;
			}
			VisitedContainers.Add(ContainerId);
			OutSession.RepositorySnapshot.Containers.Add(ContainerId, *Container);
			for (const FGuid& ItemId : Container->Slots)
			{
				if (!ItemId.IsValid()) continue;
				const FCodeBItemInstance* Item = Record.RepositorySnapshot.Items.Find(ItemId);
				if (!Item || Item->ParentContainerId != ContainerId)
				{
					OutError = TEXT("Code B Run bridge found an invalid carry item placement.");
					return false;
				}
				MovedItems.Add(ItemId);
				OutSession.RepositorySnapshot.Items.Add(ItemId, *Item);
				if (Item->ChildContainerId.IsValid() && !Self(Self, Item->ChildContainerId)) return false;
			}
			return true;
		};
		for (const FGuid& ContainerId : CarryRootContainerIds(Record.Layout))
		{
			if (!CopyContainer(CopyContainer, ContainerId)) return false;
		}
		for (const FGuid& ItemId : MovedItems)
		{
			OutSession.Receipt.MovedItemIds.Add(ItemId);
		}
		OutSession.Receipt.MovedItemIds.Sort([](const FGuid& Left, const FGuid& Right)
		{
			return Left.ToString(EGuidFormats::DigitsWithHyphensLower) < Right.ToString(EGuidFormats::DigitsWithHyphensLower);
		});
		// Bindings travel with the same P5->P6 carry graph.  The P5 side remains
		// locked throughout the prepared receipt and is reconciled after extraction,
		// so there is never a pair of concurrently editable binding truths.
		OutSession.HotbarBindings = Record.HotbarBindings;
		if (!ReconcileHotbarBindings(
			OutSession.HotbarBindings, OutSession.RepositorySnapshot, OutSession.Layout, OutError))
		{
			return false;
		}
		OutSession.Receipt.PayloadDigest = RunInventoryPayloadDigest(OutSession);
		return ValidateRunInventorySession(OutSession, OutError);
	}

	bool ApplyRunInventoryExtraction(
		FCodeBSnapshot& InOutOutOfRaidSnapshot,
		const FCodeBP2PlayerLayout& OutOfRaidLayout,
		const TArray<FGuid>& MovedItemIds,
		FString& OutError)
	{
		const TSet<FGuid> CarryRoots = CarryRootContainerIds(OutOfRaidLayout);
		TSet<FGuid> ChildContainersToRemove;
		for (const FGuid& ItemId : MovedItemIds)
		{
			const FCodeBItemInstance* Item = InOutOutOfRaidSnapshot.Items.Find(ItemId);
			if (!Item)
			{
				OutError = TEXT("Code B Run bridge could not recover a prepared extraction item.");
				return false;
			}
			FCodeBContainer* Parent = InOutOutOfRaidSnapshot.Containers.Find(Item->ParentContainerId);
			if (!Parent || !Parent->Slots.IsValidIndex(Item->SlotIndex) || Parent->Slots[Item->SlotIndex] != ItemId)
			{
				OutError = TEXT("Code B Run bridge found an invalid parent while extracting a carry item.");
				return false;
			}
			if (!CarryRoots.Contains(Item->ParentContainerId))
			{
				ChildContainersToRemove.Add(Item->ParentContainerId);
			}
			Parent->Slots[Item->SlotIndex].Invalidate();
			InOutOutOfRaidSnapshot.Items.Remove(ItemId);
		}
		for (const FGuid& ContainerId : ChildContainersToRemove)
		{
			const FCodeBContainer* Container = InOutOutOfRaidSnapshot.Containers.Find(ContainerId);
			if (!Container)
			{
				OutError = TEXT("Code B Run bridge lost a child container during extraction.");
				return false;
			}
			for (const FGuid& ItemId : Container->Slots)
			{
				if (ItemId.IsValid())
				{
					OutError = TEXT("Code B Run bridge refuses a partial child-container extraction.");
					return false;
				}
			}
			InOutOutOfRaidSnapshot.Containers.Remove(ContainerId);
		}
		if (!MovedItemIds.IsEmpty())
		{
			if (InOutOutOfRaidSnapshot.Revision == MAX_int32)
			{
				OutError = TEXT("Code B out-of-raid repository revision cannot advance for the Run bridge.");
				return false;
			}
			++InOutOutOfRaidSnapshot.Revision;
		}
		return true;
	}

	bool EquivalentHotbarBindings(
		const FCodeBHotbarBindings& Left,
		const FCodeBHotbarBindings& Right)
	{
		if (Left.Slots.Num() != Right.Slots.Num()) return false;
		for (int32 Index = 0; Index < Left.Slots.Num(); ++Index)
		{
			const FCodeBHotbarBinding& LeftSlot = Left.Slots[Index];
			const FCodeBHotbarBinding& RightSlot = Right.Slots[Index];
			if (LeftSlot.SlotIndex != RightSlot.SlotIndex
				|| LeftSlot.bHasReference != RightSlot.bHasReference
				|| LeftSlot.ItemId != RightSlot.ItemId)
			{
				return false;
			}
		}
		return true;
	}

	bool EquivalentRecord(
		const FCodeBOutOfRaidInventoryRecord& Left,
		const FCodeBOutOfRaidInventoryRecord& Right)
	{
		const FCodeBP2PlayerLayout& LeftLayout = Left.Layout;
		const FCodeBP2PlayerLayout& RightLayout = Right.Layout;
		if (Left.SchemaVersion != Right.SchemaVersion
			|| Left.OwnerId != Right.OwnerId
			|| Left.PersistentRevision != Right.PersistentRevision
			|| Left.CreatedUtc != Right.CreatedUtc
			|| Left.LastCommittedUtc != Right.LastCommittedUtc
			|| !(Left.RepositorySnapshot == Right.RepositorySnapshot)
			|| LeftLayout.LayoutId != RightLayout.LayoutId
			|| LeftLayout.WarehouseContainerId != RightLayout.WarehouseContainerId
			|| LeftLayout.BasicContainerId != RightLayout.BasicContainerId
			|| LeftLayout.WeaponContainerId != RightLayout.WeaponContainerId
			|| LeftLayout.ArmorContainerId != RightLayout.ArmorContainerId
			|| LeftLayout.SpatialContainerId != RightLayout.SpatialContainerId
			|| LeftLayout.BackpackContainerId != RightLayout.BackpackContainerId
			|| LeftLayout.SpatialInternalContainerId != RightLayout.SpatialInternalContainerId
			|| LeftLayout.PouchInternalContainerId != RightLayout.PouchInternalContainerId
			|| LeftLayout.AccessoryContainerIds != RightLayout.AccessoryContainerIds
			|| LeftLayout.bUseConditionalSpatialContainers != RightLayout.bUseConditionalSpatialContainers)
		{
			return false;
		}

		const FCodeBOutOfRaidHandoffReceipt& LeftReceipt = Left.Receipt;
		const FCodeBOutOfRaidHandoffReceipt& RightReceipt = Right.Receipt;
		if (LeftReceipt.State != RightReceipt.State
			|| LeftReceipt.SourceProfileId != RightReceipt.SourceProfileId
			|| LeftReceipt.SourceFingerprint != RightReceipt.SourceFingerprint
			|| LeftReceipt.StartedUtc != RightReceipt.StartedUtc
			|| LeftReceipt.CommittedUtc != RightReceipt.CommittedUtc
			|| LeftReceipt.InvalidLegacyEntries != RightReceipt.InvalidLegacyEntries
			|| LeftReceipt.ItemMappings.Num() != RightReceipt.ItemMappings.Num())
		{
			return false;
		}
		for (int32 Index = 0; Index < LeftReceipt.ItemMappings.Num(); ++Index)
		{
			if (LeftReceipt.ItemMappings[Index].LegacyItemId != RightReceipt.ItemMappings[Index].LegacyItemId
				|| LeftReceipt.ItemMappings[Index].CodeBItemId != RightReceipt.ItemMappings[Index].CodeBItemId)
			{
				return false;
			}
		}
		if (Left.TerminalReceipts.Num() != Right.TerminalReceipts.Num())
		{
			return false;
		}
		for (int32 Index = 0; Index < Left.TerminalReceipts.Num(); ++Index)
		{
			const FCodeBRunInventoryTerminalReceipt& LeftTerminal = Left.TerminalReceipts[Index];
			const FCodeBRunInventoryTerminalReceipt& RightTerminal = Right.TerminalReceipts[Index];
			if (LeftTerminal.ReceiptId != RightTerminal.ReceiptId
				|| LeftTerminal.OwnerId != RightTerminal.OwnerId
				|| LeftTerminal.RunInstanceId != RightTerminal.RunInstanceId
				|| LeftTerminal.TerminalState != RightTerminal.TerminalState
				|| LeftTerminal.SourceSessionRevision != RightTerminal.SourceSessionRevision
				|| LeftTerminal.CommittedUtc != RightTerminal.CommittedUtc
				|| LeftTerminal.FrozenSnapshotDigest != RightTerminal.FrozenSnapshotDigest
				|| !(LeftTerminal.FrozenRunSnapshot == RightTerminal.FrozenRunSnapshot)
				|| !EquivalentLayout(LeftTerminal.FrozenRunLayout, RightTerminal.FrozenRunLayout))
			{
				return false;
			}
		}
		if (Left.RunLocalNormalContainers.Num() != Right.RunLocalNormalContainers.Num())
		{
			return false;
		}
		for (int32 Index = 0; Index < Left.RunLocalNormalContainers.Num(); ++Index)
		{
			if (!EquivalentRunLocalNormalContainer(
				Left.RunLocalNormalContainers[Index], Right.RunLocalNormalContainers[Index]))
			{
				return false;
			}
		}
		if (Left.RunLocalBodyContainers.Num() != Right.RunLocalBodyContainers.Num())
		{
			return false;
		}
		for (int32 Index = 0; Index < Left.RunLocalBodyContainers.Num(); ++Index)
		{
			if (!EquivalentRunLocalBodyContainer(
				Left.RunLocalBodyContainers[Index], Right.RunLocalBodyContainers[Index]))
			{
				return false;
			}
		}
		if (Left.bHasActiveRunInventorySession != Right.bHasActiveRunInventorySession)
		{
			return false;
		}
		if (!Left.bHasActiveRunInventorySession)
		{
			return EquivalentHotbarBindings(Left.HotbarBindings, Right.HotbarBindings);
		}
		const FCodeBRunInventorySession& LeftSession = Left.ActiveRunInventorySession;
		const FCodeBRunInventorySession& RightSession = Right.ActiveRunInventorySession;
		const FCodeBRunInventoryBridgeReceipt& LeftRunReceipt = LeftSession.Receipt;
		const FCodeBRunInventoryBridgeReceipt& RightRunReceipt = RightSession.Receipt;
		if (LeftRunReceipt.RecoveryRebindHistory.Num() != RightRunReceipt.RecoveryRebindHistory.Num())
		{
			return false;
		}
		for (int32 Index = 0; Index < LeftRunReceipt.RecoveryRebindHistory.Num(); ++Index)
		{
			const FCodeBRunInventoryRecoveryRebind& LeftRebind = LeftRunReceipt.RecoveryRebindHistory[Index];
			const FCodeBRunInventoryRecoveryRebind& RightRebind = RightRunReceipt.RecoveryRebindHistory[Index];
			if (LeftRebind.Sequence != RightRebind.Sequence || LeftRebind.OldRunId != RightRebind.OldRunId
				|| LeftRebind.NewRunId != RightRebind.NewRunId || LeftRebind.CodeATerminalCause != RightRebind.CodeATerminalCause
				|| LeftRebind.ReboundUtc != RightRebind.ReboundUtc)
			{
				return false;
			}
		}
		if (LeftRunReceipt.ImmutablePayloadItems.Num() != RightRunReceipt.ImmutablePayloadItems.Num())
		{
			return false;
		}
		for (int32 Index = 0; Index < LeftRunReceipt.ImmutablePayloadItems.Num(); ++Index)
		{
			const FCodeBRunInventoryReceiptItem& LeftItem = LeftRunReceipt.ImmutablePayloadItems[Index];
			const FCodeBRunInventoryReceiptItem& RightItem = RightRunReceipt.ImmutablePayloadItems[Index];
			if (LeftItem.ItemId != RightItem.ItemId || LeftItem.DefinitionId != RightItem.DefinitionId
				|| LeftItem.Quantity != RightItem.Quantity || LeftItem.ParentContainerId != RightItem.ParentContainerId
				|| LeftItem.SlotIndex != RightItem.SlotIndex || LeftItem.ChildContainerId != RightItem.ChildContainerId)
			{
				return false;
			}
		}
		if (LeftSession.QuickUseReceipts.Num() != RightSession.QuickUseReceipts.Num()
			|| LeftSession.NextQuickUseReceiptOrdinal != RightSession.NextQuickUseReceiptOrdinal)
		{
			return false;
		}
		for (int32 Index = 0; Index < LeftSession.QuickUseReceipts.Num(); ++Index)
		{
			const FCodeBQuickUseReceipt& LeftQuick = LeftSession.QuickUseReceipts[Index];
			const FCodeBQuickUseReceipt& RightQuick = RightSession.QuickUseReceipts[Index];
			if (LeftQuick.ReceiptId != RightQuick.ReceiptId || LeftQuick.ReceiptOrdinal != RightQuick.ReceiptOrdinal
				|| LeftQuick.SlotIndex != RightQuick.SlotIndex || LeftQuick.SourceItemId != RightQuick.SourceItemId
				|| LeftQuick.EffectKind != RightQuick.EffectKind || LeftQuick.RestoreAmount != RightQuick.RestoreAmount
				|| LeftQuick.State != RightQuick.State)
			{
				return false;
			}
		}
		return LeftSession.SchemaVersion == RightSession.SchemaVersion
			&& LeftSession.OwnerId == RightSession.OwnerId
			&& LeftSession.RunInstanceId == RightSession.RunInstanceId
			&& LeftSession.SessionRevision == RightSession.SessionRevision
			&& LeftSession.SourceOutOfRaidRevision == RightSession.SourceOutOfRaidRevision
			&& LeftSession.CreatedUtc == RightSession.CreatedUtc
			&& LeftSession.LastCommittedUtc == RightSession.LastCommittedUtc
			&& LeftSession.BridgeState == RightSession.BridgeState
			&& LeftSession.RepositorySnapshot == RightSession.RepositorySnapshot
			&& EquivalentHotbarBindings(Left.HotbarBindings, Right.HotbarBindings)
			&& EquivalentHotbarBindings(LeftSession.HotbarBindings, RightSession.HotbarBindings)
			&& LeftSession.Layout.LayoutId == RightSession.Layout.LayoutId
			&& LeftSession.Layout.WarehouseContainerId == RightSession.Layout.WarehouseContainerId
			&& LeftSession.Layout.BasicContainerId == RightSession.Layout.BasicContainerId
			&& LeftSession.Layout.WeaponContainerId == RightSession.Layout.WeaponContainerId
			&& LeftSession.Layout.ArmorContainerId == RightSession.Layout.ArmorContainerId
			&& LeftSession.Layout.SpatialContainerId == RightSession.Layout.SpatialContainerId
			&& LeftSession.Layout.BackpackContainerId == RightSession.Layout.BackpackContainerId
			&& LeftSession.Layout.AccessoryContainerIds == RightSession.Layout.AccessoryContainerIds
			&& LeftSession.Layout.SpatialInternalContainerId == RightSession.Layout.SpatialInternalContainerId
			&& LeftSession.Layout.PouchInternalContainerId == RightSession.Layout.PouchInternalContainerId
			&& LeftSession.Layout.bUseConditionalSpatialContainers == RightSession.Layout.bUseConditionalSpatialContainers
			&& LeftRunReceipt.State == RightRunReceipt.State
			&& LeftRunReceipt.ReceiptId == RightRunReceipt.ReceiptId
			&& LeftRunReceipt.OriginRunId == RightRunReceipt.OriginRunId
			&& LeftRunReceipt.OwnerId == RightRunReceipt.OwnerId
			&& LeftRunReceipt.RunInstanceId == RightRunReceipt.RunInstanceId
			&& LeftRunReceipt.SourceOutOfRaidRevision == RightRunReceipt.SourceOutOfRaidRevision
			&& LeftRunReceipt.PayloadDigest == RightRunReceipt.PayloadDigest
			&& LeftRunReceipt.PreparedUtc == RightRunReceipt.PreparedUtc
			&& LeftRunReceipt.CommittedUtc == RightRunReceipt.CommittedUtc
			&& LeftRunReceipt.MovedItemIds == RightRunReceipt.MovedItemIds
			&& LeftRunReceipt.RecoveryDiagnostic == RightRunReceipt.RecoveryDiagnostic;
	}
}

FCodeBOutOfRaidProfileStore::FCodeBOutOfRaidProfileStore(FString InStorageRoot, FGuid InOwnerId)
	: StorageRoot(MoveTemp(InStorageRoot))
	, OwnerId(InOwnerId)
{
}

FString FCodeBOutOfRaidProfileStore::GetPrimaryPath() const
{
	return FPaths::Combine(StorageRoot, TEXT("CodeBOutOfRaid"), GuidText(OwnerId) + TEXT(".json"));
}

FString FCodeBOutOfRaidProfileStore::GetBackupPath() const
{
	return GetPrimaryPath() + TEXT(".bak");
}

FString FCodeBOutOfRaidProfileStore::GetTempPath() const
{
	return GetPrimaryPath() + TEXT(".tmp");
}

bool FCodeBOutOfRaidProfileStore::ValidateRecord(const FCodeBOutOfRaidInventoryRecord& InRecord, FString& OutError) const
{
	if (InRecord.SchemaVersion != FCodeBOutOfRaidInventoryRecord::CurrentSchemaVersion
		|| !OwnerId.IsValid() || InRecord.OwnerId != OwnerId || InRecord.PersistentRevision < 1
		|| !InRecord.Layout.LayoutId.IsValid() || !InRecord.Receipt.SourceProfileId.IsValid())
	{
		OutError = TEXT("Code B out-of-raid record identity or version is invalid.");
		return false;
	}
	FCodeBRepository ValidationRepository;
	if (!ValidationRepository.LoadPersistedSnapshot(InRecord.RepositorySnapshot, &OutError))
	{
		return false;
	}
	FCodeBP2Projection Projection;
	if (!FCodeBP2ProjectionBuilder::Build(ValidationRepository, InRecord.Layout, Projection, nullptr, &OutError))
	{
		return false;
	}
	if (!ValidateHotbarBindingsAgainstSnapshot(
		InRecord.HotbarBindings, InRecord.RepositorySnapshot, InRecord.Layout, OutError))
	{
		return false;
	}
	TSet<FGuid> TerminalReceiptIds;
	TSet<FGuid> TerminalRunIds;
	for (const FCodeBRunInventoryTerminalReceipt& Terminal : InRecord.TerminalReceipts)
	{
		if (Terminal.OwnerId != InRecord.OwnerId
			|| TerminalReceiptIds.Contains(Terminal.ReceiptId)
			|| TerminalRunIds.Contains(Terminal.RunInstanceId)
			|| !ValidateRunInventoryTerminalReceipt(Terminal, OutError))
		{
			if (OutError.IsEmpty()) OutError = TEXT("Code B Run terminal receipt history is invalid.");
			return false;
		}
		TerminalReceiptIds.Add(Terminal.ReceiptId);
		TerminalRunIds.Add(Terminal.RunInstanceId);
	}
	if (!InRecord.bHasActiveRunInventorySession)
	{
		if (!InRecord.RunLocalNormalContainers.IsEmpty() || !InRecord.RunLocalBodyContainers.IsEmpty())
		{
			OutError = TEXT("Code B terminal Profile record cannot retain Run-local container records.");
			return false;
		}
		return true;
	}
	const FCodeBRunInventorySession& Session = InRecord.ActiveRunInventorySession;
	if (!ValidateRunInventorySession(Session, OutError) || Session.OwnerId != InRecord.OwnerId)
	{
		return false;
	}
	if (TerminalRunIds.Contains(Session.RunInstanceId))
	{
		OutError = TEXT("Code B Run inventory record cannot retain an active session after its terminal receipt.");
		return false;
	}
	int32 SourceMovedCount = 0;
	for (const FGuid& ItemId : Session.Receipt.MovedItemIds)
	{
		SourceMovedCount += InRecord.RepositorySnapshot.Items.Contains(ItemId) ? 1 : 0;
	}
	if (Session.BridgeState == ECodeBRunInventoryBridgeState::Committed)
	{
		if (SourceMovedCount != 0)
		{
			OutError = TEXT("Committed Code B Run inventory still duplicates an out-of-raid item.");
			return false;
		}
	}
	else if (SourceMovedCount != 0 && SourceMovedCount != Session.Receipt.MovedItemIds.Num())
	{
		OutError = TEXT("Prepared Code B Run inventory has an indeterminate out-of-raid extraction.");
		return false;
	}
	TSet<FGuid> SearchTargetIds;
	for (const FCodeBRunLocalNormalContainerRecord& NormalContainer : InRecord.RunLocalNormalContainers)
	{
		if (Session.BridgeState != ECodeBRunInventoryBridgeState::Committed
			|| NormalContainer.OwnerId != InRecord.OwnerId
			|| NormalContainer.RunInstanceId != Session.RunInstanceId
			|| SearchTargetIds.Contains(NormalContainer.SearchTargetId)
			|| !ValidateRunLocalNormalContainerRecord(NormalContainer, OutError))
		{
			if (OutError.IsEmpty()) OutError = TEXT("Code B Run-local normal-container registry is invalid.");
			return false;
		}
		SearchTargetIds.Add(NormalContainer.SearchTargetId);
	}
	TSet<FGuid> BodyTargetIds;
	TSet<FGuid> DeathReceiptIds;
	for (const FCodeBRunLocalBodyContainerRecord& BodyContainer : InRecord.RunLocalBodyContainers)
	{
		if (Session.BridgeState != ECodeBRunInventoryBridgeState::Committed
			|| BodyContainer.OwnerId != InRecord.OwnerId
			|| BodyContainer.RunInstanceId != Session.RunInstanceId
			|| BodyTargetIds.Contains(BodyContainer.BodyTargetId)
			|| DeathReceiptIds.Contains(BodyContainer.Receipt.DeathReceipt.DeathReceiptId)
			|| !ValidateRunLocalBodyContainerRecord(BodyContainer, OutError))
		{
			if (OutError.IsEmpty()) OutError = TEXT("Code B P11 Run-local body-container registry is invalid.");
			return false;
		}
		BodyTargetIds.Add(BodyContainer.BodyTargetId);
		DeathReceiptIds.Add(BodyContainer.Receipt.DeathReceipt.DeathReceiptId);
	}
	return true;
}

bool FCodeBOutOfRaidProfileStore::SaveRecord(const FCodeBOutOfRaidInventoryRecord& InRecord, FString& OutError) const
{
	if (!ValidateRecord(InRecord, OutError))
	{
		return false;
	}
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(GetPrimaryPath()), true);
	TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetNumberField(TEXT("SchemaVersion"), InRecord.SchemaVersion);
	Root->SetStringField(TEXT("OwnerId"), GuidText(InRecord.OwnerId));
	Root->SetNumberField(TEXT("PersistentRevision"), InRecord.PersistentRevision);
	Root->SetStringField(TEXT("CreatedUtc"), InRecord.CreatedUtc);
	Root->SetStringField(TEXT("LastCommittedUtc"), InRecord.LastCommittedUtc);
	Root->SetObjectField(TEXT("Layout"), LayoutJson(InRecord.Layout));
	TSharedRef<FJsonObject> Receipt = MakeShared<FJsonObject>();
	Receipt->SetStringField(TEXT("State"), InRecord.Receipt.State == ECodeBOutOfRaidHandoffState::Committed ? TEXT("Committed") : TEXT("Prepared"));
	Receipt->SetStringField(TEXT("SourceProfileId"), GuidText(InRecord.Receipt.SourceProfileId));
	Receipt->SetStringField(TEXT("SourceFingerprint"), InRecord.Receipt.SourceFingerprint);
	Receipt->SetStringField(TEXT("StartedUtc"), InRecord.Receipt.StartedUtc);
	Receipt->SetStringField(TEXT("CommittedUtc"), InRecord.Receipt.CommittedUtc);
	TArray<TSharedPtr<FJsonValue>> Mappings;
	for (const FCodeBOutOfRaidHandoffMapping& Mapping : InRecord.Receipt.ItemMappings)
	{
		TSharedRef<FJsonObject> MappingObject = MakeShared<FJsonObject>();
		MappingObject->SetStringField(TEXT("Legacy"), GuidText(Mapping.LegacyItemId));
		MappingObject->SetStringField(TEXT("CodeB"), GuidText(Mapping.CodeBItemId));
		Mappings.Add(MakeShared<FJsonValueObject>(MappingObject));
	}
	Receipt->SetArrayField(TEXT("Mappings"), Mappings);
	TArray<TSharedPtr<FJsonValue>> Invalid;
	for (const FString& Entry : InRecord.Receipt.InvalidLegacyEntries)
	{
		Invalid.Add(MakeShared<FJsonValueString>(Entry));
	}
	Receipt->SetArrayField(TEXT("InvalidEntries"), Invalid);
	Root->SetObjectField(TEXT("Receipt"), Receipt);
	Root->SetObjectField(TEXT("RepositorySnapshot"), SnapshotJson(InRecord.RepositorySnapshot));
	Root->SetArrayField(TEXT("HotbarBindings"), HotbarBindingsJson(InRecord.HotbarBindings));
	TArray<TSharedPtr<FJsonValue>> TerminalReceipts;
	for (const FCodeBRunInventoryTerminalReceipt& Terminal : InRecord.TerminalReceipts)
	{
		TerminalReceipts.Add(MakeShared<FJsonValueObject>(RunInventoryTerminalReceiptJson(Terminal)));
	}
	Root->SetArrayField(TEXT("TerminalReceipts"), TerminalReceipts);
	TArray<TSharedPtr<FJsonValue>> RunLocalNormalContainers;
	for (const FCodeBRunLocalNormalContainerRecord& NormalContainer : InRecord.RunLocalNormalContainers)
	{
		RunLocalNormalContainers.Add(MakeShared<FJsonValueObject>(RunLocalNormalContainerJson(NormalContainer)));
	}
	Root->SetArrayField(TEXT("RunLocalNormalContainers"), RunLocalNormalContainers);
	TArray<TSharedPtr<FJsonValue>> RunLocalBodyContainers;
	for (const FCodeBRunLocalBodyContainerRecord& BodyContainer : InRecord.RunLocalBodyContainers)
	{
		RunLocalBodyContainers.Add(MakeShared<FJsonValueObject>(RunLocalBodyContainerJson(BodyContainer)));
	}
	Root->SetArrayField(TEXT("RunLocalBodyContainers"), RunLocalBodyContainers);
	Root->SetBoolField(TEXT("HasActiveRunInventorySession"), InRecord.bHasActiveRunInventorySession);
	if (InRecord.bHasActiveRunInventorySession)
	{
		Root->SetObjectField(TEXT("ActiveRunInventorySession"), RunInventorySessionJson(InRecord.ActiveRunInventorySession));
	}

	FString Payload;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Payload);
	if (!FJsonSerializer::Serialize(Root, Writer) || !FFileHelper::SaveStringToFile(Payload, *GetTempPath()))
	{
		OutError = TEXT("Code B out-of-raid temporary write failed.");
		return false;
	}
	FString VerificationError;
	FCodeBOutOfRaidInventoryRecord VerifiedRecord;
	if (!LoadRecordAtPath(GetTempPath(), VerifiedRecord, VerificationError)
		|| !EquivalentRecord(VerifiedRecord, InRecord))
	{
		OutError = FString::Printf(
			TEXT("Code B out-of-raid temporary verification failed: %s"),
			*VerificationError);
		return false;
	}
	if (IFileManager::Get().FileExists(*GetPrimaryPath())
		&& IFileManager::Get().Copy(*GetBackupPath(), *GetPrimaryPath(), true, true) != COPY_OK)
	{
		OutError = TEXT("Code B out-of-raid backup preparation failed.");
		return false;
	}
	if (!IFileManager::Get().Move(*GetPrimaryPath(), *GetTempPath(), true, false, true, true))
	{
		OutError = TEXT("Code B out-of-raid atomic replacement failed.");
		return false;
	}
	return true;
}

bool FCodeBOutOfRaidProfileStore::LoadRecordAtPath(
	const FString& Path,
	FCodeBOutOfRaidInventoryRecord& OutRecord,
	FString& OutError) const
{
	OutRecord = FCodeBOutOfRaidInventoryRecord();
	FString Payload;
	TSharedPtr<FJsonObject> Root;
	if (!FFileHelper::LoadFileToString(Payload, *Path)
		|| !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Payload), Root)
		|| !Root.IsValid())
	{
		OutError = TEXT("Code B out-of-raid record JSON cannot be read.");
		return false;
	}
		double SchemaNumber = 0.0, PersistentRevision = 0.0;
		FString OwnerText, Created, LastCommitted;
		const TSharedPtr<FJsonObject>* LayoutValue = nullptr;
		const TSharedPtr<FJsonObject>* ReceiptValue = nullptr;
		const TSharedPtr<FJsonObject>* SnapshotValue = nullptr;
		if (!Root->TryGetNumberField(TEXT("SchemaVersion"), SchemaNumber)
			|| !Root->TryGetStringField(TEXT("OwnerId"), OwnerText)
			|| !Root->TryGetNumberField(TEXT("PersistentRevision"), PersistentRevision)
			|| !Root->TryGetStringField(TEXT("CreatedUtc"), Created)
			|| !Root->TryGetStringField(TEXT("LastCommittedUtc"), LastCommitted)
			|| !Root->TryGetObjectField(TEXT("Layout"), LayoutValue)
			|| !Root->TryGetObjectField(TEXT("Receipt"), ReceiptValue)
			|| !Root->TryGetObjectField(TEXT("RepositorySnapshot"), SnapshotValue)
			|| !FMath::IsNearlyEqual(SchemaNumber, FMath::RoundToDouble(SchemaNumber))
			|| !FMath::IsNearlyEqual(PersistentRevision, FMath::RoundToDouble(PersistentRevision))
			|| !TryGuidText(OwnerText, OutRecord.OwnerId))
		{
			return false;
		}
		const TSharedPtr<FJsonObject> LayoutObject = *LayoutValue;
		const TSharedPtr<FJsonObject> ReceiptObject = *ReceiptValue;
		const TSharedPtr<FJsonObject> SnapshotObject = *SnapshotValue;
		const int32 StoredSchemaVersion = static_cast<int32>(SchemaNumber);
		if (StoredSchemaVersion != 1 && StoredSchemaVersion != 2
			&& StoredSchemaVersion != 3
			&& StoredSchemaVersion != 4
			&& StoredSchemaVersion != 5
			&& StoredSchemaVersion != 6
			&& StoredSchemaVersion != 7
			&& StoredSchemaVersion != FCodeBOutOfRaidInventoryRecord::CurrentSchemaVersion)
		{
			OutError = TEXT("Code B out-of-raid record schema is unsupported.");
			return false;
		}
		// P8 added terminal receipts, P9 added Run-local normal-container records,
		// P10 adds only per-record action identifiers, P11 adds independent
		// BodyContainer records, P12 promotes body action/reveal state, and P13
		// adds reference-only hotbar bindings. Valid v1--v6 documents
		// are promoted in memory and rewritten only by an accepted durable commit.
		OutRecord.SchemaVersion = FCodeBOutOfRaidInventoryRecord::CurrentSchemaVersion;
		OutRecord.PersistentRevision = static_cast<int32>(PersistentRevision);
		OutRecord.CreatedUtc = Created;
		OutRecord.LastCommittedUtc = LastCommitted;
		FString ParseError;
		if (!JsonToLayout(LayoutObject, OutRecord.Layout, ParseError)) return false;
		FString ReceiptState, SourceProfile, Fingerprint, Started, Committed;
		const TArray<TSharedPtr<FJsonValue>>* Mappings = nullptr;
		const TArray<TSharedPtr<FJsonValue>>* Invalid = nullptr;
		if (!ReceiptObject->TryGetStringField(TEXT("State"), ReceiptState)
			|| !ReceiptObject->TryGetStringField(TEXT("SourceProfileId"), SourceProfile)
			|| !ReceiptObject->TryGetStringField(TEXT("SourceFingerprint"), Fingerprint)
			|| !ReceiptObject->TryGetStringField(TEXT("StartedUtc"), Started)
			|| !ReceiptObject->TryGetStringField(TEXT("CommittedUtc"), Committed)
			|| !ReceiptObject->TryGetArrayField(TEXT("Mappings"), Mappings)
			|| !ReceiptObject->TryGetArrayField(TEXT("InvalidEntries"), Invalid)
			|| !TryGuidText(SourceProfile, OutRecord.Receipt.SourceProfileId)
			|| !OutRecord.Receipt.SourceProfileId.IsValid()
			|| (ReceiptState != TEXT("Prepared") && ReceiptState != TEXT("Committed"))) return false;
		OutRecord.Receipt.State = ReceiptState == TEXT("Committed") ? ECodeBOutOfRaidHandoffState::Committed : ECodeBOutOfRaidHandoffState::Prepared;
		OutRecord.Receipt.SourceFingerprint = Fingerprint;
		OutRecord.Receipt.StartedUtc = Started;
		OutRecord.Receipt.CommittedUtc = Committed;
		for (const TSharedPtr<FJsonValue>& Value : *Mappings)
		{
			TSharedPtr<FJsonObject> Mapping = Value.IsValid() ? Value->AsObject() : nullptr;
			FCodeBOutOfRaidHandoffMapping Entry;
			if (!Mapping || !ReadGuid(Mapping, TEXT("Legacy"), Entry.LegacyItemId, ParseError, true)
				|| !ReadGuid(Mapping, TEXT("CodeB"), Entry.CodeBItemId, ParseError, true)) return false;
			OutRecord.Receipt.ItemMappings.Add(Entry);
		}
		for (const TSharedPtr<FJsonValue>& Value : *Invalid)
		{
			if (!Value.IsValid() || Value->Type != EJson::String) return false;
			OutRecord.Receipt.InvalidLegacyEntries.Add(Value->AsString());
		}
		if (!JsonToSnapshot(SnapshotObject, OutRecord.RepositorySnapshot, ParseError)) return false;
		if (StoredSchemaVersion >= 7)
		{
			const TArray<TSharedPtr<FJsonValue>>* HotbarValues = nullptr;
			if (!Root->TryGetArrayField(TEXT("HotbarBindings"), HotbarValues) || !HotbarValues
				|| !JsonToHotbarBindings(*HotbarValues, OutRecord.HotbarBindings, ParseError))
			{
				OutError = ParseError.IsEmpty()
					? TEXT("Code B P13 Profile hotbar JSON is missing or invalid.") : ParseError;
				return false;
			}
		}
		else if (!ReconcileHotbarBindings(
			OutRecord.HotbarBindings, OutRecord.RepositorySnapshot, OutRecord.Layout, ParseError))
		{
			OutError = ParseError;
			return false;
		}
		if (StoredSchemaVersion >= 2)
		{
			const TArray<TSharedPtr<FJsonValue>>* TerminalReceipts = nullptr;
			if (!Root->TryGetArrayField(TEXT("TerminalReceipts"), TerminalReceipts)
				|| !TerminalReceipts)
			{
				OutError = TEXT("Code B P8 terminal receipt history is missing.");
				return false;
			}
			for (const TSharedPtr<FJsonValue>& Value : *TerminalReceipts)
			{
				FCodeBRunInventoryTerminalReceipt Terminal;
				if (!JsonToRunInventoryTerminalReceipt(
					Value.IsValid() ? Value->AsObject() : nullptr, Terminal, ParseError))
				{
					OutError = ParseError;
					return false;
				}
				OutRecord.TerminalReceipts.Add(MoveTemp(Terminal));
			}
		}
		if (StoredSchemaVersion >= 3)
		{
			const TArray<TSharedPtr<FJsonValue>>* RunLocalNormalContainers = nullptr;
			if (!Root->TryGetArrayField(TEXT("RunLocalNormalContainers"), RunLocalNormalContainers)
				|| !RunLocalNormalContainers)
			{
				OutError = TEXT("Code B P9 Run-local normal-container registry is missing.");
				return false;
			}
			for (const TSharedPtr<FJsonValue>& Value : *RunLocalNormalContainers)
			{
				FCodeBRunLocalNormalContainerRecord NormalContainer;
				if (!JsonToRunLocalNormalContainer(
					Value.IsValid() ? Value->AsObject() : nullptr, NormalContainer, ParseError))
				{
					OutError = ParseError;
					return false;
				}
				OutRecord.RunLocalNormalContainers.Add(MoveTemp(NormalContainer));
			}
		}
		if (StoredSchemaVersion >= 5)
		{
			const TArray<TSharedPtr<FJsonValue>>* RunLocalBodyContainers = nullptr;
			if (!Root->TryGetArrayField(TEXT("RunLocalBodyContainers"), RunLocalBodyContainers)
				|| !RunLocalBodyContainers)
			{
				OutError = TEXT("Code B P11 Run-local body-container registry is missing.");
				return false;
			}
			for (const TSharedPtr<FJsonValue>& Value : *RunLocalBodyContainers)
			{
				FCodeBRunLocalBodyContainerRecord BodyContainer;
				if (!JsonToRunLocalBodyContainer(
					Value.IsValid() ? Value->AsObject() : nullptr, BodyContainer, ParseError))
				{
					OutError = ParseError;
					return false;
				}
				OutRecord.RunLocalBodyContainers.Add(MoveTemp(BodyContainer));
			}
		}
		bool bHasRunSession = false;
		if (Root->HasField(TEXT("HasActiveRunInventorySession")))
		{
			const TSharedPtr<FJsonObject>* SessionObject = nullptr;
			if (!Root->TryGetBoolField(TEXT("HasActiveRunInventorySession"), bHasRunSession)
				|| (bHasRunSession && (!Root->TryGetObjectField(TEXT("ActiveRunInventorySession"), SessionObject)
					|| !JsonToRunInventorySession(*SessionObject, OutRecord.ActiveRunInventorySession, ParseError))))
			{
				return false;
			}
		}
		OutRecord.bHasActiveRunInventorySession = bHasRunSession;
		if (!ValidateRecord(OutRecord, ParseError))
		{
			OutError = ParseError;
			return false;
		}
		return true;
	}

bool FCodeBOutOfRaidProfileStore::LoadRecord(FCodeBOutOfRaidInventoryRecord& OutRecord, FString& OutError)
{
	bLastLoadUsedBackup = false;

	OutRecord = FCodeBOutOfRaidInventoryRecord();
	if (IFileManager::Get().FileExists(*GetPrimaryPath())
		&& LoadRecordAtPath(GetPrimaryPath(), OutRecord, OutError))
	{
		return true;
	}
	OutRecord = FCodeBOutOfRaidInventoryRecord();
	if (IFileManager::Get().FileExists(*GetBackupPath())
		&& LoadRecordAtPath(GetBackupPath(), OutRecord, OutError))
	{
		bLastLoadUsedBackup = true;
		return true;
	}
	OutError = TEXT("Code B out-of-raid record is absent or cannot be safely recovered.");
	return false;
}

bool FCodeBOutOfRaidProfileStore::BuildInitialRecord(const Fdemo_mapProfileSessionSnapshot& ProfileSnapshot, FCodeBOutOfRaidInventoryRecord& OutRecord, FString& OutError) const
{
	OutRecord = FCodeBOutOfRaidInventoryRecord();
	OutRecord.OwnerId = OwnerId;
	OutRecord.PersistentRevision = 1;
	OutRecord.CreatedUtc = UtcNow();
	OutRecord.LastCommittedUtc = OutRecord.CreatedUtc;
	OutRecord.Receipt.SourceProfileId = OwnerId;
	OutRecord.Receipt.SourceFingerprint = SourceFingerprint(ProfileSnapshot);
	OutRecord.Receipt.StartedUtc = OutRecord.CreatedUtc;
	OutRecord.Receipt.State = ECodeBOutOfRaidHandoffState::Prepared;

	FCodeBRepository Repository;
	FCodeBP2PlayerLayout& Layout = OutRecord.Layout;
	Layout.LayoutId = FName(TEXT("CodeB.Profile.OutOfRaid"));
	Layout.bUseConditionalSpatialContainers = true;
	Layout.WarehouseContainerId = StableGuid(OwnerId, 1);
	Layout.BasicContainerId = StableGuid(OwnerId, 2);
	Layout.WeaponContainerId = StableGuid(OwnerId, 3);
	Layout.ArmorContainerId = StableGuid(OwnerId, 4);
	Layout.SpatialContainerId = StableGuid(OwnerId, 5);
	Layout.BackpackContainerId = StableGuid(OwnerId, 6);
	Layout.AccessoryContainerIds = { StableGuid(OwnerId, 7) };
	Layout.SpatialInternalContainerId = StableGuid(OwnerId, 8);
	Layout.PouchInternalContainerId = StableGuid(OwnerId, 9);

	Fdemo_mapPersistentProfile LegacyProjection;
	LegacyProjection.ProfileId = OwnerId;
	LegacyProjection.PermanentStash = ProfileSnapshot.OrderedPermanentStash;
	LegacyProjection.WarehouseLayout = ProfileSnapshot.WarehouseLayout;
	const TArray<FGuid> LegacyWarehouseSlots = Fdemo_mapWarehouseSlotProjection::Build(LegacyProjection);
	int32 WarehouseCapacity = FMath::Max(30, LegacyWarehouseSlots.Num());
	FString Error;
	if (!Repository.CreateContainer(FName(TEXT("Warehouse")), WarehouseCapacity, ECodeBContainerKind::Storage, ECodeBEquipSlot::None, &Error, Layout.WarehouseContainerId).IsValid()
		|| !Repository.CreateContainer(FName(TEXT("Basic6")), 6, ECodeBContainerKind::Storage, ECodeBEquipSlot::None, &Error, Layout.BasicContainerId).IsValid()
		|| !Repository.CreateContainer(FName(TEXT("Weapon")), 1, ECodeBContainerKind::Equipment, ECodeBEquipSlot::Weapon, &Error, Layout.WeaponContainerId).IsValid()
		|| !Repository.CreateContainer(FName(TEXT("Armor")), 1, ECodeBContainerKind::Equipment, ECodeBEquipSlot::Armor, &Error, Layout.ArmorContainerId).IsValid()
		|| !Repository.CreateContainer(FName(TEXT("SpatialRing")), 1, ECodeBContainerKind::Equipment, ECodeBEquipSlot::SpatialItem, &Error, Layout.SpatialContainerId).IsValid()
		|| !Repository.CreateContainer(FName(TEXT("Backpack")), 1, ECodeBContainerKind::Equipment, ECodeBEquipSlot::Backpack, &Error, Layout.BackpackContainerId).IsValid()
		|| !Repository.CreateContainer(FName(TEXT("Accessory0")), 1, ECodeBContainerKind::Equipment, ECodeBEquipSlot::Accessory, &Error, Layout.AccessoryContainerIds[0]).IsValid())
	{
		OutError = Error;
		return false;
	}

	TMap<FGuid, const Fdemo_mapPersistentItemRecord*> ValidItems;
	TMap<FName, int32> MaxQuantities;
	for (const Fdemo_mapPersistentItemRecord& Item : ProfileSnapshot.OrderedPermanentStash)
	{
		if (!Item.ItemInstanceId.IsValid() || Item.StackCount <= 0 || Item.PersistentDomain != Edemo_mapPersistentDomain::PermanentStash)
		{
			OutRecord.Receipt.InvalidLegacyEntries.Add(FString::Printf(TEXT("Invalid legacy permanent-stash identity/count/domain: %s"), *GuidText(Item.ItemInstanceId)));
			continue;
		}
		if (ValidItems.Contains(Item.ItemInstanceId))
		{
			OutRecord.Receipt.InvalidLegacyEntries.Add(FString::Printf(TEXT("Duplicate legacy item id: %s"), *GuidText(Item.ItemInstanceId)));
			continue;
		}
		if (!Fdemo_mapItemDefinitions::Find(Item.ItemDefinitionId))
		{
			OutRecord.Receipt.InvalidLegacyEntries.Add(FString::Printf(TEXT("Unknown legacy definition: %s for %s"), *Item.ItemDefinitionId.ToString(), *GuidText(Item.ItemInstanceId)));
			continue;
		}
		if (const Fdemo_mapItemDefinition* LegacyDefinition = Fdemo_mapItemDefinitions::Find(Item.ItemDefinitionId);
			!LegacyDefinition || Item.StackCount > LegacyDefinition->MaxStackSize)
		{
			OutRecord.Receipt.InvalidLegacyEntries.Add(FString::Printf(TEXT("Invalid legacy stack count for %s: %d"), *GuidText(Item.ItemInstanceId), Item.StackCount));
			continue;
		}
		ValidItems.Add(Item.ItemInstanceId, &Item);
		MaxQuantities.FindOrAdd(Item.ItemDefinitionId) = FMath::Max(MaxQuantities.FindRef(Item.ItemDefinitionId), Item.StackCount);
	}

	for (const TPair<FName, int32>& Pair : MaxQuantities)
	{
		FCodeBItemDefinition Definition;
		if (!BuildCanonicalCodeBItemDefinition(Pair.Key, Definition, Error)
			|| !Repository.RegisterDefinition(Definition, &Error))
		{
			OutError = Error;
			return false;
		}
	}

	TMap<FGuid, FGuid> TargetContainers;
	TMap<FGuid, int32> TargetSlots;
	TSet<int32> UsedWarehouseSlots;
	for (int32 SlotIndex = 0; SlotIndex < LegacyWarehouseSlots.Num(); ++SlotIndex)
	{
		if (ValidItems.Contains(LegacyWarehouseSlots[SlotIndex]))
		{
			TargetContainers.Add(LegacyWarehouseSlots[SlotIndex], Layout.WarehouseContainerId);
			TargetSlots.Add(LegacyWarehouseSlots[SlotIndex], SlotIndex);
			UsedWarehouseSlots.Add(SlotIndex);
		}
	}
	int32 NextWarehouseSlot = 0;
	for (const TPair<FGuid, const Fdemo_mapPersistentItemRecord*>& Pair : ValidItems)
	{
		if (TargetContainers.Contains(Pair.Key)) continue;
		while (UsedWarehouseSlots.Contains(NextWarehouseSlot)) ++NextWarehouseSlot;
		TargetContainers.Add(Pair.Key, Layout.WarehouseContainerId);
		TargetSlots.Add(Pair.Key, NextWarehouseSlot);
		UsedWarehouseSlots.Add(NextWarehouseSlot++);
	}

	const auto AssignEquipment = [&ValidItems, &TargetContainers, &TargetSlots, &OutRecord](const FGuid& ItemId, const FGuid& ContainerId, ECodeBEquipSlot RequiredSlot, const TCHAR* Label)
	{
		const Fdemo_mapPersistentItemRecord* const* Found = ValidItems.Find(ItemId);
		if (!ItemId.IsValid()) return;
		if (!Found || !*Found || !Fdemo_mapItemDefinitions::Find((*Found)->ItemDefinitionId)
			|| CodeBEquipSlot(*Fdemo_mapItemDefinitions::Find((*Found)->ItemDefinitionId)) != RequiredSlot)
		{
			OutRecord.Receipt.InvalidLegacyEntries.Add(FString::Printf(TEXT("Invalid legacy %s equipment reference: %s"), Label, *GuidText(ItemId)));
			return;
		}
		TargetContainers[ItemId] = ContainerId;
		TargetSlots[ItemId] = 0;
	};
	AssignEquipment(ProfileSnapshot.PreparationLayout.WeaponItemInstanceId, Layout.WeaponContainerId, ECodeBEquipSlot::Weapon, TEXT("weapon"));
	AssignEquipment(ProfileSnapshot.PreparationLayout.ArmorItemInstanceId, Layout.ArmorContainerId, ECodeBEquipSlot::Armor, TEXT("armor"));
	AssignEquipment(ProfileSnapshot.PreparationLayout.AccessoryItemInstanceId, Layout.AccessoryContainerIds[0], ECodeBEquipSlot::Accessory, TEXT("accessory"));
	AssignEquipment(ProfileSnapshot.PreparationLayout.SpatialRingItemInstanceId, Layout.SpatialContainerId, ECodeBEquipSlot::SpatialItem, TEXT("spatial ring"));
	AssignEquipment(ProfileSnapshot.PreparationLayout.BackpackItemInstanceId, Layout.BackpackContainerId, ECodeBEquipSlot::Backpack, TEXT("backpack"));

	int32 NextBasicSlot = 0;
	for (const FGuid& ItemId : ProfileSnapshot.PreparationLayout.OrderedRunInventoryItemInstanceIds)
	{
		if (NextBasicSlot >= 6) break;
		if (ValidItems.Contains(ItemId) && TargetContainers.FindRef(ItemId) == Layout.WarehouseContainerId)
		{
			TargetContainers[ItemId] = Layout.BasicContainerId;
			TargetSlots[ItemId] = NextBasicSlot++;
		}
	}

	// P5 accepts the optional read-only legacy spatial relationship.  It is
	// deliberately resolved before item creation so a content item has one
	// authoritative Code B parent from its first committed snapshot.  Bad links
	// stay auditable and leave their otherwise valid item in the warehouse;
	// migration never manufactures a substitute or discards the item.
	TMap<FGuid, FGuid> SpatialChildContainers;
	for (const TPair<FGuid, const Fdemo_mapPersistentItemRecord*>& Pair : ValidItems)
	{
		const FCodeBItemDefinition* Definition = Repository.FindDefinition(Pair.Value->ItemDefinitionId);
		if (!Definition || Definition->SpatialContainerSemantic == ECodeBSpatialContainerSemantic::None) continue;

		const FGuid ChildId = SpatialChildGuid(Pair.Key);
		const FName ContainerType = Definition->SpatialContainerSemantic == ECodeBSpatialContainerSemantic::QuickRing
			? FName(TEXT("CodeB.SpatialChild.QuickRing"))
			: FName(TEXT("CodeB.SpatialChild.StoragePouch"));
		if (!Repository.CreateContainer(ContainerType, Definition->ChildContainerCapacity, ECodeBContainerKind::Storage, ECodeBEquipSlot::None, &Error, ChildId).IsValid())
		{
			OutError = Error;
			return false;
		}
		SpatialChildContainers.Add(Pair.Key, ChildId);
	}
	TMap<FGuid, int32> NextSpatialChildSlots;
	for (const TPair<FGuid, const Fdemo_mapPersistentItemRecord*>& Pair : ValidItems)
	{
		const Fdemo_mapPersistentItemRecord& LegacyItem = *Pair.Value;
		if (!LegacyItem.LegacySpatialParentItemInstanceId.IsValid()) continue;

		const FGuid ParentId = LegacyItem.LegacySpatialParentItemInstanceId;
		const FGuid* ChildContainerId = SpatialChildContainers.Find(ParentId);
		const FCodeBItemDefinition* ChildDefinition = Repository.FindDefinition(LegacyItem.ItemDefinitionId);
		if (ParentId == LegacyItem.ItemInstanceId || !ChildContainerId
			|| !ChildDefinition
			|| ChildDefinition->ItemType == ECodeBItemType::SpatialItem
			|| ChildDefinition->ItemType == ECodeBItemType::Backpack
			|| ChildDefinition->SpatialContainerSemantic != ECodeBSpatialContainerSemantic::None)
		{
			OutRecord.Receipt.InvalidLegacyEntries.Add(FString::Printf(TEXT("Invalid or nested legacy spatial parent reference for %s: %s"), *GuidText(LegacyItem.ItemInstanceId), *GuidText(ParentId)));
			continue;
		}
		const FCodeBContainer* ChildContainer = Repository.FindContainer(*ChildContainerId);
		int32& NextChildSlot = NextSpatialChildSlots.FindOrAdd(ParentId);
		if (!ChildContainer || NextChildSlot >= ChildContainer->Slots.Num())
		{
			OutRecord.Receipt.InvalidLegacyEntries.Add(FString::Printf(TEXT("Legacy spatial child capacity exceeded for %s under %s"), *GuidText(LegacyItem.ItemInstanceId), *GuidText(ParentId)));
			continue;
		}
		TargetContainers[LegacyItem.ItemInstanceId] = *ChildContainerId;
		TargetSlots[LegacyItem.ItemInstanceId] = NextChildSlot++;
	}

	FCodeBSnapshot SeedSnapshot;
	for (const TPair<FGuid, const Fdemo_mapPersistentItemRecord*>& Pair : ValidItems)
	{
		const Fdemo_mapPersistentItemRecord& Item = *Pair.Value;
		const FGuid ContainerId = TargetContainers.FindRef(Item.ItemInstanceId);
		const int32 SlotIndex = TargetSlots.FindRef(Item.ItemInstanceId);
		if (!Repository.CreateItem(Item.ItemDefinitionId, Item.StackCount, ContainerId, SlotIndex, Item.ItemInstanceId, &Error).IsValid())
		{
			OutError = FString::Printf(TEXT("Unable to migrate %s: %s"), *GuidText(Item.ItemInstanceId), *Error);
			return false;
		}
		OutRecord.Receipt.ItemMappings.Add({ Item.ItemInstanceId, Item.ItemInstanceId });
	}
	SeedSnapshot = Repository.CaptureSnapshot();
	for (const TPair<FGuid, const Fdemo_mapPersistentItemRecord*>& Pair : ValidItems)
	{
		FCodeBItemInstance* CodeBItem = SeedSnapshot.Items.Find(Pair.Key);
		const Fdemo_mapItemDefinition* LegacyDefinition = Fdemo_mapItemDefinitions::Find(Pair.Value->ItemDefinitionId);
		if (!CodeBItem || !LegacyDefinition) continue;
		CodeBItem->Level = LegacyDefinition->Level;
		CodeBItem->Quality = Pair.Value->AffixSet.IsEmpty() ? 0 : 1;
		CodeBItem->RandomSeed = static_cast<int32>(GetTypeHash(Pair.Key));
		CodeBItem->LegacyAffixDigest = LegacyAffixDigest(*Pair.Value);
	}

	// Each actual quick ring or storage pouch has a stable child container. The
	// layout references track the formally equipped owner; when unequipped P2's
	// conditional layout hides the child rather than exposing a spare warehouse.
	for (const TPair<FGuid, const Fdemo_mapPersistentItemRecord*>& Pair : ValidItems)
	{
		FCodeBItemInstance* CodeBItem = SeedSnapshot.Items.Find(Pair.Key);
		const FCodeBItemDefinition* Definition = SeedSnapshot.Definitions.Find(Pair.Value->ItemDefinitionId);
		if (!CodeBItem || !Definition || Definition->SpatialContainerSemantic == ECodeBSpatialContainerSemantic::None) continue;
		const FGuid* ChildId = SpatialChildContainers.Find(CodeBItem->ItemId);
		if (!ChildId)
		{
			OutError = FString::Printf(TEXT("Missing Code B child container for spatial legacy item %s."), *GuidText(CodeBItem->ItemId));
			return false;
		}
		CodeBItem->ChildContainerId = *ChildId;
		if (Definition->SpatialContainerSemantic == ECodeBSpatialContainerSemantic::QuickRing
			&& CodeBItem->ItemId == ProfileSnapshot.PreparationLayout.SpatialRingItemInstanceId)
		{
			Layout.SpatialInternalContainerId = *ChildId;
		}
		if (Definition->SpatialContainerSemantic == ECodeBSpatialContainerSemantic::StoragePouch
			&& CodeBItem->ItemId == ProfileSnapshot.PreparationLayout.BackpackItemInstanceId)
		{
			Layout.PouchInternalContainerId = *ChildId;
		}
	}
	if (!Repository.LoadPersistedSnapshot(SeedSnapshot, &OutError))
	{
		return false;
	}
	OutRecord.RepositorySnapshot = Repository.CaptureSnapshot();
	return ValidateRecord(OutRecord, OutError);
}

bool FCodeBOutOfRaidProfileStore::FinalizePreparedReceipt(FString& OutError)
{
	if (Record.Receipt.State == ECodeBOutOfRaidHandoffState::Committed)
	{
		return true;
	}
	if (Record.PersistentRevision == MAX_int32)
	{
		OutError = TEXT("Code B persistent revision cannot be incremented.");
		return false;
	}
	Record.Receipt.State = ECodeBOutOfRaidHandoffState::Committed;
	Record.Receipt.CommittedUtc = UtcNow();
	Record.LastCommittedUtc = Record.Receipt.CommittedUtc;
	++Record.PersistentRevision;
	return SaveRecord(Record, OutError);
}

FCodeBOutOfRaidOpenResult FCodeBOutOfRaidProfileStore::OpenOrMigrate(const Fdemo_mapProfileSessionSnapshot& ProfileSnapshot, FCodeBRepository& OutRepository, FCodeBP2PlayerLayout& OutLayout)
{
	FCodeBOutOfRaidOpenResult Result;
	if (!OwnerId.IsValid() || ProfileSnapshot.ProfileId != OwnerId)
	{
		Result.Diagnostic = TEXT("Code B 局外仓库的 Profile 身份不匹配。");
		return Result;
	}
	// ActiveRunId is retained in the Profile snapshot after a terminal
	// settlement as the first-event-wins tombstone identity.  It is history,
	// not proof of a live run.  ReadyForPreparation is the authoritative
	// session gate for out-of-raid organization; rejecting the tombstone here
	// would lock a returned player out of P5 after any completed/rolled-back run.
	if (ProfileSnapshot.SessionState != Edemo_mapProfileSessionState::ReadyForPreparation)
	{
		Result.Diagnostic = TEXT("结束当前 Run 后再整理");
		return Result;
	}

	FString Error;
	FCodeBOutOfRaidInventoryRecord Loaded;
	const bool bPrimaryExists = IFileManager::Get().FileExists(*GetPrimaryPath()) || IFileManager::Get().FileExists(*GetBackupPath());
	if (bPrimaryExists)
	{
		if (!LoadRecord(Loaded, Error))
		{
			Result.Diagnostic = Error;
			return Result;
		}
		Record = MoveTemp(Loaded);
		bHasRecord = true;
		Result.bRecoveredPendingReceipt = Record.Receipt.State == ECodeBOutOfRaidHandoffState::Prepared;
		if (bLastLoadUsedBackup)
		{
			if (Record.PersistentRevision == MAX_int32)
			{
				Result.Diagnostic = TEXT("Code B recovered record cannot advance its persistent revision.");
				return Result;
			}
			++Record.PersistentRevision;
			Record.LastCommittedUtc = UtcNow();
			if (!SaveRecord(Record, Error))
			{
				Result.Diagnostic = Error;
				return Result;
			}
		}
		bool bP17SpatialMigration = false;
		if (!EnsureP17SpatialChildrenForRecord(Record, bP17SpatialMigration, Error))
		{
			Result.Diagnostic = Error;
			return Result;
		}
		if (bP17SpatialMigration)
		{
			if (Record.PersistentRevision == MAX_int32)
			{
				Result.Diagnostic = TEXT("Code B P17 spatial migration cannot advance its Owner document revision.");
				return Result;
			}
			// P5 and a possible P6 snapshot are replaced together in this one
			// Owner document write; existing child graphs are never rebuilt here.
			++Record.PersistentRevision;
			Record.LastCommittedUtc = UtcNow();
			if (!SaveRecord(Record, Error))
			{
				Result.Diagnostic = Error;
				return Result;
			}
		}
		if (Result.bRecoveredPendingReceipt && !FinalizePreparedReceipt(Error))
		{
			Result.Diagnostic = Error;
			return Result;
		}
	}
	else
	{
		if (!BuildInitialRecord(ProfileSnapshot, Record, Error) || !SaveRecord(Record, Error))
		{
			Result.Diagnostic = Error;
			return Result;
		}
		bHasRecord = true;
		Result.bCreated = true;
#if WITH_DEV_AUTOMATION_TESTS
		if (bInterruptAfterPreparedReceiptForAutomation)
		{
			Result.Diagnostic = TEXT("Automation interrupted after verified Prepared handoff receipt.");
			return Result;
		}
#endif
		if (!FinalizePreparedReceipt(Error))
		{
			Result.Diagnostic = Error;
			return Result;
		}
	}
	if (!OutRepository.LoadPersistedSnapshot(Record.RepositorySnapshot, &Error))
	{
		Result.Diagnostic = Error;
		return Result;
	}
	OutLayout = Record.Layout;
	Result.bSuccess = true;
	Result.Diagnostic = Result.bCreated ? TEXT("已创建真实 Profile 的 Code B 局外仓库。") : TEXT("已打开真实 Profile 的 Code B 局外仓库。");
	Result.Record = Record;
	return Result;
}

bool FCodeBOutOfRaidProfileStore::CommitAcceptedSnapshot(const FCodeBSnapshot& Snapshot, FString* OutError)
{
	if (!bHasRecord || Record.PersistentRevision == MAX_int32)
	{
		if (OutError) *OutError = TEXT("Code B Profile store is not open or its revision is exhausted.");
		return false;
	}
	FCodeBOutOfRaidInventoryRecord Candidate = Record;
	Candidate.RepositorySnapshot = Snapshot;
	FString Error;
	if (!ReconcileHotbarBindings(
		Candidate.HotbarBindings, Candidate.RepositorySnapshot, Candidate.Layout, Error))
	{
		if (OutError) *OutError = Error;
		return false;
	}
	++Candidate.PersistentRevision;
	Candidate.LastCommittedUtc = UtcNow();
	if (!SaveRecord(Candidate, Error))
	{
		if (OutError) *OutError = Error;
		return false;
	}
	Record = MoveTemp(Candidate);
	return true;
}

bool FCodeBOutOfRaidProfileStore::TryGetOutOfRaidHotbarProjection(
	FCodeBHotbarProjection& OutProjection,
	FString* OutError) const
{
	OutProjection = FCodeBHotbarProjection();
	if (OutError) OutError->Reset();
	if (!bHasRecord || Record.OwnerId != OwnerId)
	{
		if (OutError) *OutError = TEXT("Code B P13 Profile hotbar has no loaded Owner-matched record.");
		return false;
	}
	FString Error;
	if (!BuildHotbarProjection(
		Record.HotbarBindings, Record.RepositorySnapshot,
		!Record.bHasActiveRunInventorySession, false, Record.PersistentRevision,
		OutProjection, Error))
	{
		if (OutError) *OutError = Error;
		return false;
	}
	return true;
}

bool FCodeBOutOfRaidProfileStore::BindOutOfRaidHotbarSlot(
	const FGuid& ItemId,
	const int32 SlotIndex,
	FCodeBHotbarProjection& OutProjection,
	FString* OutError)
{
	OutProjection = FCodeBHotbarProjection();
	if (OutError) OutError->Reset();
	if (!bHasRecord || Record.OwnerId != OwnerId || Record.bHasActiveRunInventorySession
		|| Record.PersistentRevision == MAX_int32)
	{
		if (OutError) *OutError = TEXT("Code B P13 Profile hotbar is locked or has no writable Owner-matched P5 record.");
		return false;
	}
	FCodeBOutOfRaidInventoryRecord Candidate = Record;
	FString Error;
	if (!ApplyHotbarBind(
		Candidate.HotbarBindings, Candidate.RepositorySnapshot, Candidate.Layout, ItemId, SlotIndex, Error))
	{
		if (OutError) *OutError = Error;
		return false;
	}
	++Candidate.PersistentRevision;
	Candidate.LastCommittedUtc = UtcNow();
	if (!BuildHotbarProjection(Candidate.HotbarBindings, Candidate.RepositorySnapshot,
			true, false, Candidate.PersistentRevision, OutProjection, Error)
		|| !SaveRecord(Candidate, Error))
	{
		if (OutError) *OutError = Error;
		return false;
	}
	Record = MoveTemp(Candidate);
	return true;
}

bool FCodeBOutOfRaidProfileStore::UnbindOutOfRaidHotbarSlot(
	const int32 SlotIndex,
	FCodeBHotbarProjection& OutProjection,
	FString* OutError)
{
	OutProjection = FCodeBHotbarProjection();
	if (OutError) OutError->Reset();
	if (!bHasRecord || Record.OwnerId != OwnerId || Record.bHasActiveRunInventorySession
		|| Record.PersistentRevision == MAX_int32)
	{
		if (OutError) *OutError = TEXT("Code B P13 Profile hotbar is locked or has no writable Owner-matched P5 record.");
		return false;
	}
	FCodeBOutOfRaidInventoryRecord Candidate = Record;
	FString Error;
	if (!ApplyHotbarUnbind(Candidate.HotbarBindings, SlotIndex, Error))
	{
		if (OutError) *OutError = Error;
		return false;
	}
	++Candidate.PersistentRevision;
	Candidate.LastCommittedUtc = UtcNow();
	if (!BuildHotbarProjection(Candidate.HotbarBindings, Candidate.RepositorySnapshot,
			true, false, Candidate.PersistentRevision, OutProjection, Error)
		|| !SaveRecord(Candidate, Error))
	{
		if (OutError) *OutError = Error;
		return false;
	}
	Record = MoveTemp(Candidate);
	return true;
}

FCodeBRunInventoryBridgeResult FCodeBOutOfRaidProfileStore::NotifySuccessfulRun(
	const FString& InStorageRoot,
	const FGuid& InOwnerId,
	const FGuid& RunInstanceId,
	const FCodeBRunInventoryRecoveryContext& RecoveryContext)
{
	FCodeBOutOfRaidProfileStore Store(InStorageRoot, InOwnerId);
#if WITH_DEV_AUTOMATION_TESTS
	Store.SetInterruptAfterRunPreparedReceiptForAutomation(
		GInterruptAfterRunPreparedReceiptForLifecycleAutomation);
#endif
	return Store.BridgeSuccessfulRun(RunInstanceId, RecoveryContext);
}

bool FCodeBOutOfRaidProfileStore::BuildLoadoutSelection(
	const FCodeBOutOfRaidInventoryRecord& Record,
	FCodeBLoadoutSelection& OutSelection,
	FString* OutError)
{
	OutSelection = FCodeBLoadoutSelection();
	if (OutError) OutError->Reset();
	if (!Record.OwnerId.IsValid() || Record.PersistentRevision < 1
		|| Record.bHasActiveRunInventorySession)
	{
		if (OutError) *OutError = TEXT("Code B loadout selection requires an unlocked, Owner-matched P5 record.");
		return false;
	}
	FCodeBRepository ValidationRepository;
	FString Error;
	if (!ValidationRepository.LoadPersistedSnapshot(Record.RepositorySnapshot, &Error))
	{
		if (OutError) *OutError = Error;
		return false;
	}
	OutSelection.bEnrolled = true;
	OutSelection.OwnerId = Record.OwnerId;
	OutSelection.PersistentRevision = Record.PersistentRevision;
	OutSelection.GraphRevision = Record.RepositorySnapshot.Revision;
	TArray<TPair<FName, FGuid>> CarryRoots;
	CarryRoots.Add(TPair<FName, FGuid>(FName(TEXT("Carry")), Record.Layout.BasicContainerId));
	CarryRoots.Add(TPair<FName, FGuid>(FName(TEXT("Weapon")), Record.Layout.WeaponContainerId));
	CarryRoots.Add(TPair<FName, FGuid>(FName(TEXT("Armor")), Record.Layout.ArmorContainerId));
	CarryRoots.Add(TPair<FName, FGuid>(FName(TEXT("Spatial")), Record.Layout.SpatialContainerId));
	if (Record.Layout.BackpackContainerId.IsValid())
	{
		CarryRoots.Add(TPair<FName, FGuid>(FName(TEXT("Backpack")), Record.Layout.BackpackContainerId));
	}
	for (int32 Index = 0; Index < Record.Layout.AccessoryContainerIds.Num(); ++Index)
	{
		CarryRoots.Add(TPair<FName, FGuid>(
			FName(*FString::Printf(TEXT("Accessory%d"), Index)), Record.Layout.AccessoryContainerIds[Index]));
	}

	const auto CollectClosure = [&ValidationRepository, &Error](auto&& Self, const FGuid& ItemId,
		TSet<FGuid>& VisitedItems, TSet<FGuid>& VisitedContainers, TArray<FGuid>& OutIds) -> bool
	{
		if (!ItemId.IsValid() || VisitedItems.Contains(ItemId))
		{
			Error = TEXT("Code B loadout selection found a duplicate or invalid graph ItemId.");
			return false;
		}
		const FCodeBItemInstance* Item = ValidationRepository.FindItem(ItemId);
		if (!Item)
		{
			Error = TEXT("Code B loadout selection found an item missing from P5.");
			return false;
		}
		VisitedItems.Add(ItemId);
		OutIds.Add(ItemId);
		if (!Item->ChildContainerId.IsValid()) return true;
		if (VisitedContainers.Contains(Item->ChildContainerId))
		{
			Error = TEXT("Code B loadout selection found a cyclic spatial child container.");
			return false;
		}
		const FCodeBContainer* Child = ValidationRepository.FindContainer(Item->ChildContainerId);
		if (!Child)
		{
			Error = TEXT("Code B loadout selection found a missing spatial child container.");
			return false;
		}
		VisitedContainers.Add(Item->ChildContainerId);
		for (const FGuid& ChildItemId : Child->Slots)
		{
			if (!ChildItemId.IsValid()) continue;
			const FCodeBItemInstance* ChildItem = ValidationRepository.FindItem(ChildItemId);
			if (!ChildItem || ChildItem->ParentContainerId != Child->ContainerId
				|| !Self(Self, ChildItemId, VisitedItems, VisitedContainers, OutIds))
			{
				if (Error.IsEmpty()) Error = TEXT("Code B loadout selection found a detached spatial child item.");
				return false;
			}
		}
		return true;
	};

	for (const TPair<FName, FGuid>& CarryRoot : CarryRoots)
	{
		if (!CarryRoot.Value.IsValid()) continue;
		const FCodeBContainer* Container = ValidationRepository.FindContainer(CarryRoot.Value);
		if (!Container)
		{
			if (OutError) *OutError = TEXT("Code B loadout selection found a missing formal carry container.");
			return false;
		}
		for (int32 SlotIndex = 0; SlotIndex < Container->Slots.Num(); ++SlotIndex)
		{
			const FGuid& ItemId = Container->Slots[SlotIndex];
			if (!ItemId.IsValid()) continue;
			const FCodeBItemInstance* Item = ValidationRepository.FindItem(ItemId);
			if (!Item || Item->ParentContainerId != Container->ContainerId || Item->SlotIndex != SlotIndex)
			{
				if (OutError) *OutError = TEXT("Code B loadout selection found a carry root placement mismatch.");
				return false;
			}
			FCodeBLoadoutSelectionRoot Root;
			Root.RootItemId = ItemId;
			Root.ContainerId = Container->ContainerId;
			Root.SlotIndex = SlotIndex;
			Root.SlotSemantic = CarryRoot.Key;
			TSet<FGuid> VisitedItems;
			TSet<FGuid> VisitedContainers;
			if (!CollectClosure(CollectClosure, ItemId, VisitedItems, VisitedContainers, Root.GraphClosureItemIds))
			{
				if (OutError) *OutError = Error;
				return false;
			}
			Root.GraphClosureItemIds.Sort([](const FGuid& Left, const FGuid& Right)
			{
				return Left.ToString(EGuidFormats::DigitsWithHyphensLower)
					< Right.ToString(EGuidFormats::DigitsWithHyphensLower);
			});
			OutSelection.Roots.Add(MoveTemp(Root));
		}
	}
	OutSelection.Roots.Sort([](const FCodeBLoadoutSelectionRoot& Left, const FCodeBLoadoutSelectionRoot& Right)
	{
		const FString LeftContainer = Left.ContainerId.ToString(EGuidFormats::DigitsWithHyphensLower);
		const FString RightContainer = Right.ContainerId.ToString(EGuidFormats::DigitsWithHyphensLower);
		if (LeftContainer != RightContainer) return LeftContainer < RightContainer;
		if (Left.SlotIndex != Right.SlotIndex) return Left.SlotIndex < Right.SlotIndex;
		return Left.RootItemId.ToString(EGuidFormats::DigitsWithHyphensLower)
			< Right.RootItemId.ToString(EGuidFormats::DigitsWithHyphensLower);
	});
	TArray<FString> DigestPieces;
	DigestPieces.Add(OutSelection.OwnerId.ToString(EGuidFormats::DigitsWithHyphensLower));
	DigestPieces.Add(FString::FromInt(OutSelection.PersistentRevision));
	DigestPieces.Add(FString::FromInt(OutSelection.GraphRevision));
	for (const FCodeBLoadoutSelectionRoot& Root : OutSelection.Roots)
	{
		TArray<FString> Closure;
		for (const FGuid& ItemId : Root.GraphClosureItemIds)
		{
			Closure.Add(ItemId.ToString(EGuidFormats::DigitsWithHyphensLower));
		}
		DigestPieces.Add(FString::Printf(TEXT("%s:%s:%d:%s:%s"),
			*Root.ContainerId.ToString(EGuidFormats::DigitsWithHyphensLower), *Root.SlotSemantic.ToString(),
			Root.SlotIndex, *Root.RootItemId.ToString(EGuidFormats::DigitsWithHyphensLower),
			*FString::Join(Closure, TEXT(","))));
	}
	OutSelection.Digest = FString::Printf(TEXT("%08X"), FCrc::StrCrc32(*FString::Join(DigestPieces, TEXT("|"))));
	return true;
}

FCodeBRunInventoryBridgeResult FCodeBOutOfRaidProfileStore::NotifySuccessfulRunWithLoadoutSelection(
	const FString& InStorageRoot,
	const FGuid& InOwnerId,
	const FGuid& RunInstanceId,
	const FCodeBLoadoutSelection& Selection,
	const FCodeBRunInventoryRecoveryContext& RecoveryContext)
{
	FCodeBRunInventoryBridgeResult Result;
	if (!InOwnerId.IsValid() || !RunInstanceId.IsValid() || !Selection.IsUsable()
		|| Selection.OwnerId != InOwnerId)
	{
		Result.Status = ECodeBRunInventoryBridgeStatus::InvalidIdentity;
		Result.Diagnostic = TEXT("Code B selected-loadout bridge received an invalid Owner, Run, or P5 selection identity.");
		return Result;
	}
	FCodeBOutOfRaidProfileStore Store(InStorageRoot, InOwnerId);
	if (!IFileManager::Get().FileExists(*Store.GetPrimaryPath())
		&& !IFileManager::Get().FileExists(*Store.GetBackupPath()))
	{
		Result.Status = ECodeBRunInventoryBridgeStatus::NotEnrolled;
		Result.Diagnostic = TEXT("Code B selected-loadout bridge found no enrolled P5 sidecar.");
		return Result;
	}
	FString Error;
	FCodeBOutOfRaidInventoryRecord Loaded;
	if (!Store.LoadRecord(Loaded, Error))
	{
		Result.Status = ECodeBRunInventoryBridgeStatus::StorageFailure;
		Result.Diagnostic = Error;
		return Result;
	}
	FCodeBLoadoutSelection Current;
	if (!BuildLoadoutSelection(Loaded, Current, &Error))
	{
		Result.Status = ECodeBRunInventoryBridgeStatus::SelectionMismatch;
		Result.Diagnostic = Error;
		return Result;
	}
	if (Current.PersistentRevision != Selection.PersistentRevision
		|| Current.GraphRevision != Selection.GraphRevision || Current.Digest != Selection.Digest)
	{
		Result.Status = ECodeBRunInventoryBridgeStatus::SelectionMismatch;
		Result.Diagnostic = TEXT("Code B selected-loadout bridge refused a stale P5 revision, graph revision, or digest.");
		return Result;
	}
	return Store.BridgeSuccessfulRun(RunInstanceId, RecoveryContext);
}

FCodeBRunInventoryTerminalResult FCodeBOutOfRaidProfileStore::NotifyCommittedRunTerminal(
	const FString& InStorageRoot,
	const FGuid& InOwnerId,
	const FGuid& RunInstanceId,
	const ECodeBRunInventoryTerminalState TerminalState)
{
	FCodeBRunInventoryTerminalResult Result;
	if (!InOwnerId.IsValid() || !RunInstanceId.IsValid())
	{
		Result.Status = ECodeBRunInventoryTerminalStatus::InvalidIdentity;
		Result.Diagnostic = TEXT("Code B P8 terminal observer received an invalid OwnerId or RunInstanceId.");
		return Result;
	}
	if (TerminalState == ECodeBRunInventoryTerminalState::Unknown)
	{
		Result.Status = ECodeBRunInventoryTerminalStatus::UnknownTerminal;
		Result.Diagnostic = TEXT("Code B P8 terminal observer refused an unknown Code A terminal classification.");
		return Result;
	}
	FCodeBOutOfRaidProfileStore Store(InStorageRoot, InOwnerId);
	if (!IFileManager::Get().FileExists(*Store.GetPrimaryPath())
		&& !IFileManager::Get().FileExists(*Store.GetBackupPath()))
	{
		Result.Status = ECodeBRunInventoryTerminalStatus::NotEnrolled;
		Result.Diagnostic = TEXT("Code B P5 sidecar is absent; the P8 terminal observer is intentionally not enrolled.");
		return Result;
	}
	FString Error;
	FCodeBOutOfRaidInventoryRecord Loaded;
	if (!Store.LoadRecord(Loaded, Error))
	{
		Result.Status = ECodeBRunInventoryTerminalStatus::StorageFailure;
		Result.Diagnostic = Error;
		return Result;
	}
	if (Loaded.OwnerId != InOwnerId)
	{
		Result.Status = ECodeBRunInventoryTerminalStatus::StorageFailure;
		Result.Diagnostic = TEXT("Code B P8 terminal observer loaded an Owner-mismatched record.");
		return Result;
	}
	Store.Record = MoveTemp(Loaded);
	Store.bHasRecord = true;
	return Store.FinalizeCommittedRunTerminal(RunInstanceId, TerminalState);
}

const FCodeBNormalContainerDefinition* FCodeBOutOfRaidProfileStore::FindNormalContainerDefinition(
	const FName DefinitionId)
{
	return FindNormalContainerDefinitionInternal(DefinitionId);
}

const FCodeBBodyContainerDefinition* FCodeBOutOfRaidProfileStore::FindBodyContainerDefinition(
	const FName DefinitionId)
{
	return FindBodyContainerDefinitionInternal(DefinitionId);
}

FCodeBBodyContainerMaterializationResult
FCodeBOutOfRaidProfileStore::MaterializeMatchedRunBodyContainerOnDeath(
	const FString& InStorageRoot,
	const FGuid& InOwnerId,
	const FGuid& InRunInstanceId,
	const FGuid& BodyTargetId,
	const FName DefinitionId,
	const FCodeBBodyContainerDeathReceipt& DeathReceipt)
{
	FCodeBBodyContainerMaterializationResult Result;
	if (!InOwnerId.IsValid() || !InRunInstanceId.IsValid() || !BodyTargetId.IsValid()
		|| DefinitionId.IsNone())
	{
		Result.Status = ECodeBBodyContainerMaterializationStatus::InvalidIdentity;
		Result.Diagnostic = TEXT("Code B P11 body materialization requires exact OwnerId, RunInstanceId, BodyTargetId, and DefinitionId.");
		return Result;
	}
	FString Error;
	if (!ValidateBodyContainerDeathReceipt(
		DeathReceipt, InOwnerId, InRunInstanceId, BodyTargetId, DefinitionId, Error))
	{
		Result.Status = ECodeBBodyContainerMaterializationStatus::InvalidDeathReceipt;
		Result.Diagnostic = Error;
		return Result;
	}
	const FCodeBBodyContainerDefinition* Definition = FindBodyContainerDefinitionInternal(DefinitionId);
	if (!Definition)
	{
		Result.Status = ECodeBBodyContainerMaterializationStatus::UnknownDefinition;
		Result.Diagnostic = TEXT("Code B P11 body materialization refused an unknown definition.");
		return Result;
	}
	if (!ValidateBodyContainerDefinition(*Definition, Error))
	{
		Result.Status = ECodeBBodyContainerMaterializationStatus::InvalidDefinition;
		Result.Diagnostic = Error;
		return Result;
	}
	FCodeBOutOfRaidProfileStore Store(InStorageRoot, InOwnerId);
	if (!IFileManager::Get().FileExists(*Store.GetPrimaryPath())
		&& !IFileManager::Get().FileExists(*Store.GetBackupPath()))
	{
		Result.Status = ECodeBBodyContainerMaterializationStatus::NotEnrolled;
		Result.Diagnostic = TEXT("Code B Profile sidecar is absent; P11 body materialization is intentionally not enrolled.");
		return Result;
	}
	FCodeBOutOfRaidInventoryRecord Loaded;
	if (!Store.LoadRecord(Loaded, Error))
	{
		Result.Status = ECodeBBodyContainerMaterializationStatus::StorageFailure;
		Result.Diagnostic = Error;
		return Result;
	}
	if (!IsExactCommittedActiveRun(Loaded, InOwnerId, InRunInstanceId))
	{
		Result.Status = ECodeBBodyContainerMaterializationStatus::ActiveSessionNotCommitted;
		Result.Diagnostic = TEXT("Code B P11 body materialization refused a missing, terminal, mismatched, or non-committed P6 session.");
		return Result;
	}
	for (const FCodeBRunLocalBodyContainerRecord& Existing : Loaded.RunLocalBodyContainers)
	{
		if (Existing.BodyTargetId == BodyTargetId)
		{
			if (Existing.DefinitionId != DefinitionId)
			{
				Result.Status = ECodeBBodyContainerMaterializationStatus::TargetDefinitionConflict;
				Result.Diagnostic = TEXT("Code B P11 body materialization refused a second definition for one BodyTargetId.");
				return Result;
			}
			if (Existing.Receipt.DeathReceipt.DeathReceiptId != DeathReceipt.DeathReceiptId)
			{
				Result.Status = ECodeBBodyContainerMaterializationStatus::InvalidDeathReceipt;
				Result.Diagnostic = TEXT("Code B P11 body materialization refused a conflicting death receipt for an already materialized BodyTargetId.");
				return Result;
			}
			if (!BuildBodyContainerProjection(Existing, Result.Projection, Error))
			{
				Result.Status = ECodeBBodyContainerMaterializationStatus::StorageFailure;
				Result.Diagnostic = Error;
				return Result;
			}
			Result.Status = ECodeBBodyContainerMaterializationStatus::AlreadyMaterialized;
			Result.Diagnostic = TEXT("Code B P11 replay returned the existing BodyMaterialized projection without writing.");
			return Result;
		}
		if (Existing.Receipt.DeathReceipt.DeathReceiptId == DeathReceipt.DeathReceiptId)
		{
			Result.Status = ECodeBBodyContainerMaterializationStatus::InvalidDeathReceipt;
			Result.Diagnostic = TEXT("Code B P11 body materialization refused one death receipt bound to multiple BodyTargetIds.");
			return Result;
		}
	}
	if (Loaded.PersistentRevision == MAX_int32)
	{
		Result.Status = ECodeBBodyContainerMaterializationStatus::StorageFailure;
		Result.Diagnostic = TEXT("Code B P11 body materialization cannot advance the durable Profile revision.");
		return Result;
	}
	FCodeBRunLocalBodyContainerRecord Materialized;
	if (!BuildMaterializedBodyContainerRecord(InOwnerId, InRunInstanceId, BodyTargetId,
		*Definition, DeathReceipt, Materialized, Error))
	{
		Result.Status = ECodeBBodyContainerMaterializationStatus::InvalidDefinition;
		Result.Diagnostic = Error;
		return Result;
	}
	FCodeBOutOfRaidInventoryRecord Candidate = Loaded;
	Candidate.RunLocalBodyContainers.Add(MoveTemp(Materialized));
	Candidate.LastCommittedUtc = UtcNow();
	++Candidate.PersistentRevision;
	if (!Store.SaveRecord(Candidate, Error))
	{
		Result.Status = ECodeBBodyContainerMaterializationStatus::StorageFailure;
		Result.Diagnostic = Error;
		return Result;
	}
	Store.Record = MoveTemp(Candidate);
	Store.bHasRecord = true;
	if (!BuildBodyContainerProjection(Store.Record.RunLocalBodyContainers.Last(), Result.Projection, Error))
	{
		Result.Status = ECodeBBodyContainerMaterializationStatus::StorageFailure;
		Result.Diagnostic = Error;
		return Result;
	}
	Result.Status = ECodeBBodyContainerMaterializationStatus::Materialized;
	Result.Diagnostic = TEXT("Code B P11 atomically materialized one Hidden Run-local body-container graph from a validated death receipt.");
	return Result;
}

bool FCodeBOutOfRaidProfileStore::TryGetMatchedRunBodyContainerProjection(
	const FString& InStorageRoot,
	const FGuid& InOwnerId,
	const FGuid& InRunInstanceId,
	const FGuid& BodyTargetId,
	FCodeBBodyContainerProjection& OutProjection,
	FString* OutError)
{
	OutProjection = FCodeBBodyContainerProjection();
	if (!InOwnerId.IsValid() || !InRunInstanceId.IsValid() || !BodyTargetId.IsValid())
	{
		if (OutError) *OutError = TEXT("Code B P11 body projection requires exact OwnerId, RunInstanceId, and BodyTargetId.");
		return false;
	}
	FCodeBOutOfRaidProfileStore Store(InStorageRoot, InOwnerId);
	if (!IFileManager::Get().FileExists(*Store.GetPrimaryPath())
		&& !IFileManager::Get().FileExists(*Store.GetBackupPath()))
	{
		if (OutError) *OutError = TEXT("Code B P11 body projection cannot create an absent sidecar.");
		return false;
	}
	FString Error;
	FCodeBOutOfRaidInventoryRecord Loaded;
	if (!Store.LoadRecord(Loaded, Error) || !IsExactCommittedActiveRun(Loaded, InOwnerId, InRunInstanceId))
	{
		if (OutError) *OutError = Error.IsEmpty()
			? TEXT("Code B P11 body projection refused a missing, terminal, mismatched, or non-committed P6 session.")
			: Error;
		return false;
	}
	const FCodeBRunLocalBodyContainerRecord* Record = Loaded.RunLocalBodyContainers.FindByPredicate(
		[BodyTargetId](const FCodeBRunLocalBodyContainerRecord& Value)
		{
			return Value.BodyTargetId == BodyTargetId;
		});
	if (!Record)
	{
		if (OutError) *OutError = TEXT("Code B P11 body projection found no materialized record for the exact BodyTargetId.");
		return false;
	}
	if (!BuildBodyContainerProjection(*Record, OutProjection, Error))
	{
		if (OutError) *OutError = Error;
		return false;
	}
	return true;
}

FCodeBBodyContainerActionResult FCodeBOutOfRaidProfileStore::BeginMatchedRunBodyContainerOpen(
	const FString& InStorageRoot,
	const FGuid& InOwnerId,
	const FGuid& InRunInstanceId,
	const FGuid& BodyTargetId,
	const FName DefinitionId)
{
	FCodeBBodyContainerActionResult Result;
	if (!InOwnerId.IsValid() || !InRunInstanceId.IsValid() || !BodyTargetId.IsValid() || DefinitionId.IsNone())
	{
		Result.Status = ECodeBBodyContainerActionStatus::InvalidIdentity;
		Result.Diagnostic = TEXT("Code B P12 body open requires exact Owner, Run, BodyTarget, and Definition identities.");
		return Result;
	}
	FCodeBOutOfRaidProfileStore Store(InStorageRoot, InOwnerId);
	FCodeBRunInventorySession Session;
	FString Error;
	if (!Store.OpenMatchedActiveRunInventorySession(InRunInstanceId, Session, &Error))
	{
		Result.Status = ECodeBBodyContainerActionStatus::ActiveSessionNotCommitted;
		Result.Diagnostic = Error;
		return Result;
	}
	FCodeBOutOfRaidInventoryRecord Candidate = Store.Record;
	FCodeBRunLocalBodyContainerRecord* Record = Candidate.RunLocalBodyContainers.FindByPredicate(
		[BodyTargetId](const FCodeBRunLocalBodyContainerRecord& Value) { return Value.BodyTargetId == BodyTargetId; });
	if (!Record)
	{
		Result.Status = ECodeBBodyContainerActionStatus::NotMaterialized;
		Result.Diagnostic = TEXT("Code B P12 body open found no exact P11 materialized body record.");
		return Result;
	}
	if (Record->DefinitionId != DefinitionId)
	{
		Result.Status = ECodeBBodyContainerActionStatus::TargetDefinitionConflict;
		Result.Diagnostic = TEXT("Code B P12 body open refused a DefinitionId mismatch.");
		return Result;
	}
	if (Record->State == ECodeBBodyContainerState::Open && !Record->ActiveActionId.IsValid())
	{
		if (!BuildBodyContainerProjection(*Record, Result.Projection, Error))
		{
			Result.Status = ECodeBBodyContainerActionStatus::StorageFailure;
			Result.Diagnostic = Error;
			return Result;
		}
		Result.Status = ECodeBBodyContainerActionStatus::AlreadyInRequestedState;
		Result.Diagnostic = TEXT("Code B P12 body reopen returned its existing Open projection without writing.");
		return Result;
	}
	if (Record->State == ECodeBBodyContainerState::Opening)
	{
		if (!BuildBodyContainerProjection(*Record, Result.Projection, Error))
		{
			Result.Status = ECodeBBodyContainerActionStatus::StorageFailure;
			Result.Diagnostic = Error;
			return Result;
		}
		Result.Status = ECodeBBodyContainerActionStatus::AlreadyInRequestedState;
		Result.Diagnostic = TEXT("Code B P12 body open retained the existing exact opening action without duplicating it.");
		return Result;
	}
	if (Record->State != ECodeBBodyContainerState::BodyMaterialized
		&& Record->State != ECodeBBodyContainerState::Interrupted)
	{
		Result.Status = ECodeBBodyContainerActionStatus::InvalidState;
		Result.Diagnostic = TEXT("Code B P12 body open found an invalid body action state.");
		return Result;
	}
	if (Candidate.PersistentRevision == MAX_int32 || Record->Revision == MAX_int32)
	{
		Result.Status = ECodeBBodyContainerActionStatus::StorageFailure;
		Result.Diagnostic = TEXT("Code B P12 body open cannot advance durable revisions.");
		return Result;
	}
	Record->SchemaVersion = FCodeBRunLocalBodyContainerRecord::CurrentSchemaVersion;
	Record->State = ECodeBBodyContainerState::Opening;
	Record->ActiveActionId = FGuid::NewGuid();
	Record->ActiveSearchItemId.Invalidate();
	++Record->Revision;
	Candidate.LastCommittedUtc = UtcNow();
	++Candidate.PersistentRevision;
	if (!Store.SaveRecord(Candidate, Error) || !BuildBodyContainerProjection(*Record, Result.Projection, Error))
	{
		Result.Status = ECodeBBodyContainerActionStatus::StorageFailure;
		Result.Diagnostic = Error;
		return Result;
	}
	Result.Status = ECodeBBodyContainerActionStatus::Committed;
	Result.Diagnostic = TEXT("Code B P12 atomically began one exact body Opening action.");
	return Result;
}

FCodeBBodyContainerActionResult FCodeBOutOfRaidProfileStore::CompleteMatchedRunBodyContainerOpen(
	const FString& InStorageRoot,
	const FGuid& InOwnerId,
	const FGuid& InRunInstanceId,
	const FGuid& BodyTargetId,
	const FName DefinitionId,
	const FGuid& ActionId)
{
	FCodeBBodyContainerActionResult Result;
	if (!InOwnerId.IsValid() || !InRunInstanceId.IsValid() || !BodyTargetId.IsValid()
		|| DefinitionId.IsNone() || !ActionId.IsValid())
	{
		Result.Status = ECodeBBodyContainerActionStatus::InvalidIdentity;
		Result.Diagnostic = TEXT("Code B P12 body open completion requires the exact issued action identity.");
		return Result;
	}
	FCodeBOutOfRaidProfileStore Store(InStorageRoot, InOwnerId);
	FCodeBRunInventorySession Session;
	FString Error;
	if (!Store.OpenMatchedActiveRunInventorySession(InRunInstanceId, Session, &Error))
	{
		Result.Status = ECodeBBodyContainerActionStatus::ActiveSessionNotCommitted;
		Result.Diagnostic = Error;
		return Result;
	}
	FCodeBOutOfRaidInventoryRecord Candidate = Store.Record;
	FCodeBRunLocalBodyContainerRecord* Record = Candidate.RunLocalBodyContainers.FindByPredicate(
		[BodyTargetId](const FCodeBRunLocalBodyContainerRecord& Value) { return Value.BodyTargetId == BodyTargetId; });
	if (!Record)
	{
		Result.Status = ECodeBBodyContainerActionStatus::NotMaterialized;
		Result.Diagnostic = TEXT("Code B P12 body open completion found no exact body record.");
		return Result;
	}
	if (Record->DefinitionId != DefinitionId)
	{
		Result.Status = ECodeBBodyContainerActionStatus::TargetDefinitionConflict;
		Result.Diagnostic = TEXT("Code B P12 body open completion refused a DefinitionId mismatch.");
		return Result;
	}
	if (Record->State == ECodeBBodyContainerState::Open && !Record->ActiveActionId.IsValid())
	{
		if (!BuildBodyContainerProjection(*Record, Result.Projection, Error))
		{
			Result.Status = ECodeBBodyContainerActionStatus::StorageFailure;
			Result.Diagnostic = Error;
			return Result;
		}
		Result.Status = ECodeBBodyContainerActionStatus::AlreadyInRequestedState;
		Result.Diagnostic = TEXT("Code B P12 body open completion found the exact target already Open.");
		return Result;
	}
	if (Record->State != ECodeBBodyContainerState::Opening || Record->ActiveActionId != ActionId)
	{
		Result.Status = ECodeBBodyContainerActionStatus::ActionMismatch;
		Result.Diagnostic = TEXT("Code B P12 body open completion rejected a stale, cancelled, or mismatched action.");
		return Result;
	}
	if (Candidate.PersistentRevision == MAX_int32 || Record->Revision == MAX_int32)
	{
		Result.Status = ECodeBBodyContainerActionStatus::StorageFailure;
		Result.Diagnostic = TEXT("Code B P12 body open completion cannot advance durable revisions.");
		return Result;
	}
	Record->State = ECodeBBodyContainerState::Open;
	Record->ActiveActionId.Invalidate();
	++Record->Revision;
	Candidate.LastCommittedUtc = UtcNow();
	++Candidate.PersistentRevision;
	if (!Store.SaveRecord(Candidate, Error) || !BuildBodyContainerProjection(*Record, Result.Projection, Error))
	{
		Result.Status = ECodeBBodyContainerActionStatus::StorageFailure;
		Result.Diagnostic = Error;
		return Result;
	}
	Result.Status = ECodeBBodyContainerActionStatus::Committed;
	Result.Diagnostic = TEXT("Code B P12 atomically completed the exact body opening action.");
	return Result;
}

FCodeBBodyContainerActionResult FCodeBOutOfRaidProfileStore::BeginMatchedRunBodyContainerItemSearch(
	const FString& InStorageRoot,
	const FGuid& InOwnerId,
	const FGuid& InRunInstanceId,
	const FGuid& BodyTargetId,
	const FName DefinitionId,
	const FGuid& ItemId)
{
	FCodeBBodyContainerActionResult Result;
	if (!InOwnerId.IsValid() || !InRunInstanceId.IsValid() || !BodyTargetId.IsValid()
		|| DefinitionId.IsNone() || !ItemId.IsValid())
	{
		Result.Status = ECodeBBodyContainerActionStatus::InvalidIdentity;
		Result.Diagnostic = TEXT("Code B P12 body item search requires exact target and item identities.");
		return Result;
	}
	FCodeBOutOfRaidProfileStore Store(InStorageRoot, InOwnerId);
	FCodeBRunInventorySession Session;
	FString Error;
	if (!Store.OpenMatchedActiveRunInventorySession(InRunInstanceId, Session, &Error))
	{
		Result.Status = ECodeBBodyContainerActionStatus::ActiveSessionNotCommitted;
		Result.Diagnostic = Error;
		return Result;
	}
	FCodeBOutOfRaidInventoryRecord Candidate = Store.Record;
	FCodeBRunLocalBodyContainerRecord* Record = Candidate.RunLocalBodyContainers.FindByPredicate(
		[BodyTargetId](const FCodeBRunLocalBodyContainerRecord& Value) { return Value.BodyTargetId == BodyTargetId; });
	if (!Record)
	{
		Result.Status = ECodeBBodyContainerActionStatus::NotMaterialized;
		Result.Diagnostic = TEXT("Code B P12 body item search found no exact body record.");
		return Result;
	}
	if (Record->DefinitionId != DefinitionId)
	{
		Result.Status = ECodeBBodyContainerActionStatus::TargetDefinitionConflict;
		Result.Diagnostic = TEXT("Code B P12 body item search refused a DefinitionId mismatch.");
		return Result;
	}
	FCodeBBodyContainerItemVisibility* Visibility = Record->ItemVisibilities.FindByPredicate(
		[ItemId](const FCodeBBodyContainerItemVisibility& Value) { return Value.ItemId == ItemId; });
	if (!Visibility || !Record->ContainerSnapshot.Items.Contains(ItemId))
	{
		Result.Status = ECodeBBodyContainerActionStatus::ItemNotFound;
		Result.Diagnostic = TEXT("Code B P12 body item search refused an item outside the exact body snapshot.");
		return Result;
	}
	if (Record->State != ECodeBBodyContainerState::Open || Record->ActiveActionId.IsValid()
		|| Record->ActiveSearchItemId.IsValid())
	{
		Result.Status = ECodeBBodyContainerActionStatus::InvalidState;
		Result.Diagnostic = TEXT("Code B P12 body item search requires an Open body with no in-flight action.");
		return Result;
	}
	if (Visibility->Visibility != ECodeBBodyContainerVisibility::Hidden)
	{
		Result.Status = ECodeBBodyContainerActionStatus::ItemNotHidden;
		Result.Diagnostic = TEXT("Code B P12 body item search refused a non-Hidden item without rerolling or re-hiding it.");
		return Result;
	}
	if (Candidate.PersistentRevision == MAX_int32 || Record->Revision == MAX_int32)
	{
		Result.Status = ECodeBBodyContainerActionStatus::StorageFailure;
		Result.Diagnostic = TEXT("Code B P12 body item search cannot advance durable revisions.");
		return Result;
	}
	Record->SchemaVersion = FCodeBRunLocalBodyContainerRecord::CurrentSchemaVersion;
	Visibility->Visibility = ECodeBBodyContainerVisibility::Searching;
	Record->ActiveSearchItemId = ItemId;
	Record->ActiveActionId = FGuid::NewGuid();
	++Record->Revision;
	Candidate.LastCommittedUtc = UtcNow();
	++Candidate.PersistentRevision;
	if (!Store.SaveRecord(Candidate, Error) || !BuildBodyContainerProjection(*Record, Result.Projection, Error))
	{
		Result.Status = ECodeBBodyContainerActionStatus::StorageFailure;
		Result.Diagnostic = Error;
		return Result;
	}
	Result.Status = ECodeBBodyContainerActionStatus::Committed;
	Result.Diagnostic = TEXT("Code B P12 atomically began one exact Hidden-to-Searching body reveal.");
	return Result;
}

FCodeBBodyContainerActionResult FCodeBOutOfRaidProfileStore::CompleteMatchedRunBodyContainerItemSearch(
	const FString& InStorageRoot,
	const FGuid& InOwnerId,
	const FGuid& InRunInstanceId,
	const FGuid& BodyTargetId,
	const FName DefinitionId,
	const FGuid& ActionId)
{
	FCodeBBodyContainerActionResult Result;
	if (!InOwnerId.IsValid() || !InRunInstanceId.IsValid() || !BodyTargetId.IsValid()
		|| DefinitionId.IsNone() || !ActionId.IsValid())
	{
		Result.Status = ECodeBBodyContainerActionStatus::InvalidIdentity;
		Result.Diagnostic = TEXT("Code B P12 body search completion requires the exact issued action identity.");
		return Result;
	}
	FCodeBOutOfRaidProfileStore Store(InStorageRoot, InOwnerId);
	FCodeBRunInventorySession Session;
	FString Error;
	if (!Store.OpenMatchedActiveRunInventorySession(InRunInstanceId, Session, &Error))
	{
		Result.Status = ECodeBBodyContainerActionStatus::ActiveSessionNotCommitted;
		Result.Diagnostic = Error;
		return Result;
	}
	FCodeBOutOfRaidInventoryRecord Candidate = Store.Record;
	FCodeBRunLocalBodyContainerRecord* Record = Candidate.RunLocalBodyContainers.FindByPredicate(
		[BodyTargetId](const FCodeBRunLocalBodyContainerRecord& Value) { return Value.BodyTargetId == BodyTargetId; });
	if (!Record)
	{
		Result.Status = ECodeBBodyContainerActionStatus::NotMaterialized;
		Result.Diagnostic = TEXT("Code B P12 body search completion found no exact body record.");
		return Result;
	}
	if (Record->DefinitionId != DefinitionId)
	{
		Result.Status = ECodeBBodyContainerActionStatus::TargetDefinitionConflict;
		Result.Diagnostic = TEXT("Code B P12 body search completion refused a DefinitionId mismatch.");
		return Result;
	}
	if (Record->State != ECodeBBodyContainerState::Open || Record->ActiveActionId != ActionId
		|| !Record->ActiveSearchItemId.IsValid())
	{
		Result.Status = ECodeBBodyContainerActionStatus::ActionMismatch;
		Result.Diagnostic = TEXT("Code B P12 body search completion rejected a stale, cancelled, or mismatched action.");
		return Result;
	}
	FCodeBBodyContainerItemVisibility* Visibility = Record->ItemVisibilities.FindByPredicate(
		[Record](const FCodeBBodyContainerItemVisibility& Value) { return Value.ItemId == Record->ActiveSearchItemId; });
	if (!Visibility || Visibility->Visibility != ECodeBBodyContainerVisibility::Searching
		|| !Record->ContainerSnapshot.Items.Contains(Record->ActiveSearchItemId))
	{
		Result.Status = ECodeBBodyContainerActionStatus::StorageFailure;
		Result.Diagnostic = TEXT("Code B P12 body search completion found an invalid durable Searching item.");
		return Result;
	}
	if (Candidate.PersistentRevision == MAX_int32 || Record->Revision == MAX_int32)
	{
		Result.Status = ECodeBBodyContainerActionStatus::StorageFailure;
		Result.Diagnostic = TEXT("Code B P12 body search completion cannot advance durable revisions.");
		return Result;
	}
	Visibility->Visibility = ECodeBBodyContainerVisibility::Revealed;
	Record->ActiveActionId.Invalidate();
	Record->ActiveSearchItemId.Invalidate();
	++Record->Revision;
	Candidate.LastCommittedUtc = UtcNow();
	++Candidate.PersistentRevision;
	if (!Store.SaveRecord(Candidate, Error) || !BuildBodyContainerProjection(*Record, Result.Projection, Error))
	{
		Result.Status = ECodeBBodyContainerActionStatus::StorageFailure;
		Result.Diagnostic = Error;
		return Result;
	}
	Result.Status = ECodeBBodyContainerActionStatus::Committed;
	Result.Diagnostic = TEXT("Code B P12 atomically completed Searching-to-Revealed without rerolling the item.");
	return Result;
}

FCodeBBodyContainerActionResult FCodeBOutOfRaidProfileStore::InterruptMatchedRunBodyContainerAction(
	const FString& InStorageRoot,
	const FGuid& InOwnerId,
	const FGuid& InRunInstanceId,
	const FGuid& BodyTargetId,
	const FName DefinitionId,
	const FString& Reason)
{
	FCodeBBodyContainerActionResult Result;
	if (!InOwnerId.IsValid() || !InRunInstanceId.IsValid() || !BodyTargetId.IsValid() || DefinitionId.IsNone())
	{
		Result.Status = ECodeBBodyContainerActionStatus::InvalidIdentity;
		Result.Diagnostic = TEXT("Code B P12 body action interruption requires an exact target identity.");
		return Result;
	}
	FCodeBOutOfRaidProfileStore Store(InStorageRoot, InOwnerId);
	FCodeBRunInventorySession Session;
	FString Error;
	if (!Store.OpenMatchedActiveRunInventorySession(InRunInstanceId, Session, &Error))
	{
		Result.Status = ECodeBBodyContainerActionStatus::ActiveSessionNotCommitted;
		Result.Diagnostic = Error;
		return Result;
	}
	FCodeBOutOfRaidInventoryRecord Candidate = Store.Record;
	FCodeBRunLocalBodyContainerRecord* Record = Candidate.RunLocalBodyContainers.FindByPredicate(
		[BodyTargetId](const FCodeBRunLocalBodyContainerRecord& Value) { return Value.BodyTargetId == BodyTargetId; });
	if (!Record)
	{
		Result.Status = ECodeBBodyContainerActionStatus::NotMaterialized;
		Result.Diagnostic = TEXT("Code B P12 body action interruption found no exact body record.");
		return Result;
	}
	if (Record->DefinitionId != DefinitionId)
	{
		Result.Status = ECodeBBodyContainerActionStatus::TargetDefinitionConflict;
		Result.Diagnostic = TEXT("Code B P12 body action interruption refused a DefinitionId mismatch.");
		return Result;
	}
	if (!Record->ActiveActionId.IsValid())
	{
		if (!BuildBodyContainerProjection(*Record, Result.Projection, Error))
		{
			Result.Status = ECodeBBodyContainerActionStatus::StorageFailure;
			Result.Diagnostic = Error;
			return Result;
		}
		Result.Status = ECodeBBodyContainerActionStatus::AlreadyInRequestedState;
		Result.Diagnostic = TEXT("Code B P12 body action interruption found no pending action.");
		return Result;
	}
	if (Candidate.PersistentRevision == MAX_int32 || Record->Revision == MAX_int32)
	{
		Result.Status = ECodeBBodyContainerActionStatus::StorageFailure;
		Result.Diagnostic = TEXT("Code B P12 body action interruption cannot advance durable revisions.");
		return Result;
	}
	if (Record->State == ECodeBBodyContainerState::Opening)
	{
		Record->State = ECodeBBodyContainerState::Interrupted;
	}
	else if (Record->State == ECodeBBodyContainerState::Open && Record->ActiveSearchItemId.IsValid())
	{
		FCodeBBodyContainerItemVisibility* Visibility = Record->ItemVisibilities.FindByPredicate(
			[Record](const FCodeBBodyContainerItemVisibility& Value) { return Value.ItemId == Record->ActiveSearchItemId; });
		if (!Visibility || Visibility->Visibility != ECodeBBodyContainerVisibility::Searching)
		{
			Result.Status = ECodeBBodyContainerActionStatus::StorageFailure;
			Result.Diagnostic = TEXT("Code B P12 body action interruption found no matching Searching item.");
			return Result;
		}
		Visibility->Visibility = ECodeBBodyContainerVisibility::Hidden;
	}
	else
	{
		Result.Status = ECodeBBodyContainerActionStatus::InvalidState;
		Result.Diagnostic = TEXT("Code B P12 body action interruption found an invalid pending action state.");
		return Result;
	}
	Record->ActiveActionId.Invalidate();
	Record->ActiveSearchItemId.Invalidate();
	++Record->Revision;
	Candidate.LastCommittedUtc = UtcNow();
	++Candidate.PersistentRevision;
	if (!Store.SaveRecord(Candidate, Error) || !BuildBodyContainerProjection(*Record, Result.Projection, Error))
	{
		Result.Status = ECodeBBodyContainerActionStatus::StorageFailure;
		Result.Diagnostic = Error;
		return Result;
	}
	Result.Status = ECodeBBodyContainerActionStatus::Committed;
	Result.Diagnostic = FString::Printf(TEXT("Code B P12 atomically interrupted the exact pending body action: %s."), *Reason);
	return Result;
}

bool FCodeBOutOfRaidProfileStore::CommitAcceptedMatchedRunBodyContainerTransfer(
	const FString& InStorageRoot,
	const FGuid& InOwnerId,
	const FGuid& InRunInstanceId,
	const FGuid& BodyTargetId,
	const FName DefinitionId,
	const int32 ExpectedP6SnapshotRevision,
	const int32 ExpectedBodyContainerRevision,
	const FCodeBSnapshot& CompositeSnapshot,
	FString* OutError)
{
	if (OutError) OutError->Reset();
	if (!InOwnerId.IsValid() || !InRunInstanceId.IsValid() || !BodyTargetId.IsValid()
		|| DefinitionId.IsNone() || ExpectedP6SnapshotRevision < 0
		|| ExpectedBodyContainerRevision < 1)
	{
		if (OutError) *OutError = TEXT("Code B P12 body transfer requires exact Owner/Run/body/definition and source revisions.");
		return false;
	}
	FCodeBRepository CompositeValidation;
	FString Error;
	if (!CompositeValidation.LoadPersistedSnapshot(CompositeSnapshot, &Error))
	{
		if (OutError) *OutError = TEXT("Code B P12 body transfer refused an invalid P1 composite snapshot: ") + Error;
		return false;
	}
	FCodeBOutOfRaidProfileStore Store(InStorageRoot, InOwnerId);
	FCodeBRunInventorySession Session;
	if (!Store.OpenMatchedActiveRunInventorySession(InRunInstanceId, Session, &Error))
	{
		if (OutError) *OutError = TEXT("Code B P12 body transfer has no exact committed P6 session: ") + Error;
		return false;
	}
	FCodeBOutOfRaidInventoryRecord Candidate = Store.Record;
	FCodeBRunLocalBodyContainerRecord* Record = Candidate.RunLocalBodyContainers.FindByPredicate(
		[BodyTargetId](const FCodeBRunLocalBodyContainerRecord& Value) { return Value.BodyTargetId == BodyTargetId; });
	if (!Record || Record->DefinitionId != DefinitionId
		|| Record->State != ECodeBBodyContainerState::Open
		|| Record->ActiveActionId.IsValid() || Record->ActiveSearchItemId.IsValid()
		|| Candidate.ActiveRunInventorySession.RepositorySnapshot.Revision != ExpectedP6SnapshotRevision
		|| Record->Revision != ExpectedBodyContainerRevision)
	{
		if (OutError) *OutError = TEXT("Code B P12 body transfer refused a stale, non-Open, action-pending, or definition-mismatched target.");
		return false;
	}
	if (Candidate.PersistentRevision == MAX_int32
		|| Candidate.ActiveRunInventorySession.SessionRevision == MAX_int32
		|| Record->Revision == MAX_int32)
	{
		if (OutError) *OutError = TEXT("Code B P12 body transfer cannot advance durable revisions.");
		return false;
	}

	TSet<FGuid> PriorItemIds;
	TSet<FGuid> PriorContainerIds;
	for (const TPair<FGuid, FCodeBItemInstance>& Pair : Candidate.ActiveRunInventorySession.RepositorySnapshot.Items)
	{
		PriorItemIds.Add(Pair.Key);
	}
	for (const TPair<FGuid, FCodeBItemInstance>& Pair : Record->ContainerSnapshot.Items)
	{
		if (PriorItemIds.Contains(Pair.Key))
		{
			if (OutError) *OutError = TEXT("Code B P12 body transfer found a duplicate P6/body item identity.");
			return false;
		}
		PriorItemIds.Add(Pair.Key);
	}
	for (const TPair<FGuid, FCodeBContainer>& Pair : Candidate.ActiveRunInventorySession.RepositorySnapshot.Containers)
	{
		PriorContainerIds.Add(Pair.Key);
	}
	for (const TPair<FGuid, FCodeBContainer>& Pair : Record->ContainerSnapshot.Containers)
	{
		if (PriorContainerIds.Contains(Pair.Key))
		{
			if (OutError) *OutError = TEXT("Code B P12 body transfer found a duplicate P6/body container identity.");
			return false;
		}
		PriorContainerIds.Add(Pair.Key);
	}
	FCodeBSnapshot P24PriorComposite;
	if (!BuildP24PriorComposite(
		Candidate.ActiveRunInventorySession.RepositorySnapshot,
		Record->ContainerSnapshot, P24PriorComposite, Error))
	{
		if (OutError) *OutError = Error;
		return false;
	}
	bool bHasP24CreatedIdentity = false;
	for (const TPair<FGuid, FCodeBItemInstance>& Pair : CompositeSnapshot.Items)
	{
		if (!PriorItemIds.Contains(Pair.Key))
		{
			bHasP24CreatedIdentity = true;
			break;
		}
	}
	const bool bAcceptedP24SplitIdentity = bHasP24CreatedIdentity
		&& IsExactP24SplitDelta(P24PriorComposite, CompositeSnapshot, Error);
	if (bHasP24CreatedIdentity && !bAcceptedP24SplitIdentity)
	{
		if (OutError) *OutError = TEXT("Code B P12 body transfer refused a non-P1 Split identity: ") + Error;
		return false;
	}
	const bool bHasP25MergeDelta = HasP25MergeQuantityOrRemovalDelta(
		P24PriorComposite, CompositeSnapshot);
	if (!bAcceptedP24SplitIdentity && bHasP25MergeDelta
		&& !IsExactP25MergeDelta(P24PriorComposite, CompositeSnapshot, Error))
	{
		if (OutError) *OutError = TEXT("Code B P12 body transfer refused a non-P1 Merge quantity delta: ") + Error;
		return false;
	}
	for (const TPair<FGuid, FCodeBItemInstance>& Pair : CompositeSnapshot.Items)
	{
		if (!PriorItemIds.Contains(Pair.Key) && !bAcceptedP24SplitIdentity)
		{
			if (OutError) *OutError = TEXT("Code B P12 body transfer refused an invented item identity.");
			return false;
		}
	}
	for (const TPair<FGuid, FCodeBContainer>& Pair : CompositeSnapshot.Containers)
	{
		if (!PriorContainerIds.Contains(Pair.Key))
		{
			if (OutError) *OutError = TEXT("Code B P12 body transfer refused an invented container identity.");
			return false;
		}
	}
	TMap<FGuid, ECodeBBodyContainerVisibility> OriginalVisibilities;
	for (const FCodeBBodyContainerItemVisibility& Visibility : Record->ItemVisibilities)
	{
		OriginalVisibilities.Add(Visibility.ItemId, Visibility.Visibility);
	}

	// P21's fixed corpse-equipment cells are a one-way extraction boundary.
	// Before the generic P12 material path can partition a candidate snapshot,
	// prove that every body equipment cell is unchanged or that its one real,
	// Revealed item made the exact P1 Move into an initially empty P6 BaseQuick
	// cell.  This rejects direct player equip, return-to-corpse, cloning, and
	// any widget-side rewrite while keeping all earlier r1/r2 records untouched.
	const FCodeBLootProfile* BodyProfile = FindLootProfileByProvenance(
		Record->DefinitionId, Record->Receipt.LootProfileId, Record->Receipt.LootProfileVersion);
	const bool bP21R3 = BodyProfile && IsP21BasicCorpseR3(*BodyProfile);
	TSet<FGuid> P21EquipmentContainerIds;
	bool bMovedP21Equipment = false;
	FGuid MovedP21EquipmentItemId;
	FGuid MovedP21EquipmentContainerId;
	int32 MovedP21EquipmentTargetSlot = INDEX_NONE;
	if (bP21R3)
	{
		if (!ValidateP21BodyEquipmentCatalog(Error)
			|| Record->Receipt.EquipmentCandidateSetDigest != P21EquipmentCandidateSetDigest())
		{
			if (OutError) *OutError = Error.IsEmpty()
				? TEXT("Code B P21 body transfer has invalid r3 equipment provenance.") : Error;
			return false;
		}
	}
	else if (!Record->Receipt.EquipmentCandidateSetDigest.IsEmpty())
	{
		if (OutError) *OutError = TEXT("Code B P21 body transfer refused equipment provenance on a historical body record.");
		return false;
	}
	if (bP21R3)
	{
		const FCodeBContainer* OriginalBaseQuick = Candidate.ActiveRunInventorySession.RepositorySnapshot.Containers.Find(
			Candidate.ActiveRunInventorySession.Layout.BasicContainerId);
		const FCodeBContainer* CandidateBaseQuick = CompositeSnapshot.Containers.Find(
			Candidate.ActiveRunInventorySession.Layout.BasicContainerId);
		if (!OriginalBaseQuick || !CandidateBaseQuick
			|| OriginalBaseQuick->IsEquipment() || CandidateBaseQuick->IsEquipment())
		{
			if (OutError) *OutError = TEXT("Code B P21 body transfer requires the exact P6 BaseQuick storage container.");
			return false;
		}
		for (const FP21BodyEquipmentSlot& EquipmentSlot : P21BodyEquipmentSlots())
		{
			const FGuid EquipmentContainerId = P21BodyEquipmentContainerGuid(
				Record->BodyTargetId, Record->DefinitionId, EquipmentSlot.Semantic);
			P21EquipmentContainerIds.Add(EquipmentContainerId);
			const FCodeBContainer* OriginalEquipment = Record->ContainerSnapshot.Containers.Find(EquipmentContainerId);
			const FCodeBContainer* CandidateEquipment = CompositeSnapshot.Containers.Find(EquipmentContainerId);
			if (!OriginalEquipment || !CandidateEquipment
				|| OriginalEquipment->ContainerType != EquipmentSlot.ContainerType
				|| CandidateEquipment->ContainerType != EquipmentSlot.ContainerType
				|| !OriginalEquipment->IsEquipment() || !CandidateEquipment->IsEquipment()
				|| OriginalEquipment->EquipmentSlot != EquipmentSlot.EquipmentSlot
				|| CandidateEquipment->EquipmentSlot != EquipmentSlot.EquipmentSlot
				|| OriginalEquipment->Slots.Num() != 1 || CandidateEquipment->Slots.Num() != 1)
			{
				if (OutError) *OutError = TEXT("Code B P21 body transfer found an invalid fixed corpse equipment cell.");
				return false;
			}
			const FGuid OriginalItemId = OriginalEquipment->Slots[0];
			const FGuid CandidateItemId = CandidateEquipment->Slots[0];
			if (!OriginalItemId.IsValid())
			{
				if (CandidateItemId.IsValid())
				{
					if (OutError) *OutError = TEXT("Code B P21 refuses every player-to-corpse equipment return.");
					return false;
				}
				continue;
			}
			const FCodeBItemInstance* OriginalItem = Record->ContainerSnapshot.Items.Find(OriginalItemId);
			const FCodeBItemInstance* CandidateItem = CompositeSnapshot.Items.Find(OriginalItemId);
			const ECodeBBodyContainerVisibility* Visibility = OriginalVisibilities.Find(OriginalItemId);
			if (!OriginalItem || !CandidateItem || CandidateItemId.IsValid()
				|| OriginalItem->DefinitionId != EquipmentSlot.ItemDefinitionId
				|| OriginalItem->Quantity != 1 || OriginalItem->ChildContainerId.IsValid()
				|| CandidateItem->DefinitionId != OriginalItem->DefinitionId
				|| CandidateItem->Quantity != 1 || CandidateItem->ChildContainerId.IsValid())
			{
				if (OutError) *OutError = TEXT("Code B P21 body transfer refused a cloned, mutated, or non-simple corpse equipment item.");
				return false;
			}
			if (CandidateItem->ParentContainerId == EquipmentContainerId)
			{
				if (CandidateItem->SlotIndex != 0)
				{
					if (OutError) *OutError = TEXT("Code B P21 body transfer refused a corpse equipment rearrangement.");
					return false;
				}
				continue;
			}
			if (bMovedP21Equipment || !Visibility || *Visibility != ECodeBBodyContainerVisibility::Revealed
				|| CandidateItem->ParentContainerId != Candidate.ActiveRunInventorySession.Layout.BasicContainerId
				|| !OriginalBaseQuick->Slots.IsValidIndex(CandidateItem->SlotIndex)
				|| OriginalBaseQuick->Slots[CandidateItem->SlotIndex].IsValid()
				|| !CandidateBaseQuick->Slots.IsValidIndex(CandidateItem->SlotIndex)
				|| CandidateBaseQuick->Slots[CandidateItem->SlotIndex] != OriginalItemId)
			{
				if (OutError) *OutError = TEXT("Code B P21 only moves a Revealed corpse equipment item to one empty exact P6 BaseQuick cell.");
				return false;
			}
			bMovedP21Equipment = true;
			MovedP21EquipmentItemId = OriginalItemId;
			MovedP21EquipmentContainerId = EquipmentContainerId;
			MovedP21EquipmentTargetSlot = CandidateItem->SlotIndex;
		}
		for (const TPair<FGuid, FCodeBItemInstance>& Pair : Candidate.ActiveRunInventorySession.RepositorySnapshot.Items)
		{
			const FCodeBItemInstance* CandidateItem = CompositeSnapshot.Items.Find(Pair.Key);
			if (!CandidateItem)
			{
				if (OutError) *OutError = TEXT("Code B P21 transfer lost a pre-existing P6 item.");
				return false;
			}
			if (P21EquipmentContainerIds.Contains(CandidateItem->ParentContainerId))
			{
				if (OutError) *OutError = TEXT("Code B P21 refuses every P6 item entering a corpse equipment cell.");
				return false;
			}
		}
		if (bMovedP21Equipment)
		{
			FCodeBSnapshot ExpectedComposite = Candidate.ActiveRunInventorySession.RepositorySnapshot;
			ExpectedComposite.Revision = FMath::Max(ExpectedComposite.Revision, Record->ContainerSnapshot.Revision);
			for (const TPair<FName, FCodeBItemDefinition>& Pair : Record->ContainerSnapshot.Definitions)
			{
				if (const FCodeBItemDefinition* Existing = ExpectedComposite.Definitions.Find(Pair.Key);
					Existing && !(*Existing == Pair.Value))
				{
					if (OutError) *OutError = TEXT("Code B P21 transfer found conflicting P6/corpse item definitions.");
					return false;
				}
				ExpectedComposite.Definitions.Add(Pair.Key, Pair.Value);
			}
			for (const TPair<FGuid, FCodeBItemInstance>& Pair : Record->ContainerSnapshot.Items)
			{
				if (ExpectedComposite.Items.Contains(Pair.Key))
				{
					if (OutError) *OutError = TEXT("Code B P21 transfer found duplicate P6/corpse item identities.");
					return false;
				}
				ExpectedComposite.Items.Add(Pair.Key, Pair.Value);
			}
			for (const TPair<FGuid, FCodeBContainer>& Pair : Record->ContainerSnapshot.Containers)
			{
				if (ExpectedComposite.Containers.Contains(Pair.Key))
				{
					if (OutError) *OutError = TEXT("Code B P21 transfer found duplicate P6/corpse container identities.");
					return false;
				}
				ExpectedComposite.Containers.Add(Pair.Key, Pair.Value);
			}
			FCodeBRepository ExpectedRepository;
			FCodeBTransactionRequest Move;
			Move.TransactionId = FGuid::NewGuid();
			Move.Operation = ECodeBOperation::Move;
			Move.ItemId = MovedP21EquipmentItemId;
			Move.SourceContainerId = MovedP21EquipmentContainerId;
			Move.SourceSlot = 0;
			Move.TargetContainerId = Candidate.ActiveRunInventorySession.Layout.BasicContainerId;
			Move.TargetSlot = MovedP21EquipmentTargetSlot;
			Move.ExpectedRevision = ExpectedComposite.Revision;
			if (!ExpectedRepository.LoadPersistedSnapshot(ExpectedComposite, &Error)
				|| !ExpectedRepository.ExecuteTransaction(Move).IsSuccess()
				|| ExpectedRepository.CaptureSnapshot() != CompositeSnapshot)
			{
				if (OutError) *OutError = TEXT("Code B P21 transfer candidate differs from the one allowed P11-to-empty-P6-BaseQuick P1 Move.");
				return false;
			}
		}
	}

	// P20 adds one deliberately narrow P12 path. A formal parent may leave this
	// exact, Revealed BasicCorpse root only as its unchanged empty P17 closure,
	// only through one empty P6 BaseQuick cell. The comparison below makes the
	// accepted composite identical to that one P1 Move, rather than a generic
	// widget-authored rearrangement. Existing simple-item P12 moves remain below.
	const FCodeBContainer* OriginalBaseQuick = Candidate.ActiveRunInventorySession.RepositorySnapshot.Containers.Find(
		Candidate.ActiveRunInventorySession.Layout.BasicContainerId);
	const FCodeBContainer* CandidateBaseQuick = CompositeSnapshot.Containers.Find(
		Candidate.ActiveRunInventorySession.Layout.BasicContainerId);
	int32 SpatialParentCount = 0;
	bool bMovedSpatialParent = false;
	FGuid MovedSpatialParentId;
	int32 MovedSpatialSourceSlot = INDEX_NONE;
	int32 MovedSpatialTargetSlot = INDEX_NONE;
	for (const TPair<FGuid, FCodeBItemInstance>& Pair : Record->ContainerSnapshot.Items)
	{
		const FCodeBItemInstance& OriginalItem = Pair.Value;
		if (OriginalItem.ParentContainerId != Record->ContainerId) continue;
		FCodeBItemDefinition CanonicalDefinition;
		if (!BuildCanonicalCodeBItemDefinition(OriginalItem.DefinitionId, CanonicalDefinition, Error))
		{
			if (OutError) *OutError = TEXT("Code B P20 transfer cannot resolve a BasicCorpse root definition: ") + Error;
			return false;
		}
		if (CanonicalDefinition.SpatialContainerSemantic == ECodeBSpatialContainerSemantic::None) continue;
		if (++SpatialParentCount != 1
			|| (OriginalItem.DefinitionId != Fdemo_mapItemIds::WindTalisman
				&& OriginalItem.DefinitionId != Fdemo_mapItemIds::BackpackLevel1)
			|| !ValidateP20BasicCorpseSpatialClosure(Record->ContainerSnapshot, OriginalItem, Error))
		{
			if (OutError) *OutError = Error.IsEmpty()
				? TEXT("Code B P20 BasicCorpse contains more than one or an unsupported spatial parent.") : Error;
			return false;
		}
		const ECodeBBodyContainerVisibility* Visibility = OriginalVisibilities.Find(OriginalItem.ItemId);
		const FCodeBItemInstance* FinalItem = CompositeSnapshot.Items.Find(OriginalItem.ItemId);
		if (!Visibility || !FinalItem || FinalItem->DefinitionId != OriginalItem.DefinitionId
			|| FinalItem->Quantity != OriginalItem.Quantity
			|| !ValidateP20BasicCorpseSpatialClosure(CompositeSnapshot, *FinalItem, Error))
		{
			if (OutError) *OutError = Error.IsEmpty()
				? TEXT("Code B P20 transfer refused a missing, cloned, flattened, or mutated spatial closure.") : Error;
			return false;
		}
		if (FinalItem->ParentContainerId == Record->ContainerId)
		{
			if (FinalItem->SlotIndex != OriginalItem.SlotIndex)
			{
				if (OutError) *OutError = TEXT("Code B P20 transfer refused to rearrange an untransferred BasicCorpse spatial parent.");
				return false;
			}
			continue;
		}
		if (*Visibility != ECodeBBodyContainerVisibility::Revealed
			|| !OriginalBaseQuick || !CandidateBaseQuick
			|| FinalItem->ParentContainerId != Candidate.ActiveRunInventorySession.Layout.BasicContainerId
			|| !OriginalBaseQuick->Slots.IsValidIndex(FinalItem->SlotIndex)
			|| OriginalBaseQuick->Slots[FinalItem->SlotIndex].IsValid()
			|| !CandidateBaseQuick->Slots.IsValidIndex(FinalItem->SlotIndex)
			|| CandidateBaseQuick->Slots[FinalItem->SlotIndex] != OriginalItem.ItemId)
		{
			if (OutError) *OutError = TEXT("Code B P20 only moves a Revealed BasicCorpse spatial root to one empty exact P6 BaseQuick cell.");
			return false;
		}
		bMovedSpatialParent = true;
		MovedSpatialParentId = OriginalItem.ItemId;
		MovedSpatialSourceSlot = OriginalItem.SlotIndex;
		MovedSpatialTargetSlot = FinalItem->SlotIndex;
	}
	for (const TPair<FGuid, FCodeBItemInstance>& Pair : Candidate.ActiveRunInventorySession.RepositorySnapshot.Items)
	{
		FCodeBItemDefinition CanonicalDefinition;
		const FCodeBItemInstance* FinalItem = CompositeSnapshot.Items.Find(Pair.Key);
		if (!FinalItem || !BuildCanonicalCodeBItemDefinition(Pair.Value.DefinitionId, CanonicalDefinition, Error))
		{
			if (OutError) *OutError = Error.IsEmpty()
				? TEXT("Code B P20 transfer lost a pre-existing P6 item.") : Error;
			return false;
		}
		if (CanonicalDefinition.SpatialContainerSemantic != ECodeBSpatialContainerSemantic::None
			&& FinalItem->ParentContainerId == Record->ContainerId)
		{
			if (OutError) *OutError = TEXT("Code B P20 refuses every P6-to-BasicCorpse spatial parent or child graph return.");
			return false;
		}
	}
	if (bMovedSpatialParent)
	{
		FCodeBSnapshot ExpectedComposite = Candidate.ActiveRunInventorySession.RepositorySnapshot;
		ExpectedComposite.Revision = FMath::Max(ExpectedComposite.Revision, Record->ContainerSnapshot.Revision);
		for (const TPair<FName, FCodeBItemDefinition>& Pair : Record->ContainerSnapshot.Definitions)
		{
			if (const FCodeBItemDefinition* Existing = ExpectedComposite.Definitions.Find(Pair.Key);
				Existing && !(*Existing == Pair.Value))
			{
				if (OutError) *OutError = TEXT("Code B P20 transfer found conflicting P6/BasicCorpse item definitions.");
				return false;
			}
			ExpectedComposite.Definitions.Add(Pair.Key, Pair.Value);
		}
		for (const TPair<FGuid, FCodeBItemInstance>& Pair : Record->ContainerSnapshot.Items)
		{
			if (ExpectedComposite.Items.Contains(Pair.Key))
			{
				if (OutError) *OutError = TEXT("Code B P20 transfer found duplicate P6/BasicCorpse item identities.");
				return false;
			}
			ExpectedComposite.Items.Add(Pair.Key, Pair.Value);
		}
		for (const TPair<FGuid, FCodeBContainer>& Pair : Record->ContainerSnapshot.Containers)
		{
			if (ExpectedComposite.Containers.Contains(Pair.Key))
			{
				if (OutError) *OutError = TEXT("Code B P20 transfer found duplicate P6/BasicCorpse container identities.");
				return false;
			}
			ExpectedComposite.Containers.Add(Pair.Key, Pair.Value);
		}
		FCodeBRepository ExpectedRepository;
		FCodeBTransactionRequest Move;
		Move.TransactionId = FGuid::NewGuid();
		Move.Operation = ECodeBOperation::Move;
		Move.ItemId = MovedSpatialParentId;
		Move.SourceContainerId = Record->ContainerId;
		Move.SourceSlot = MovedSpatialSourceSlot;
		Move.TargetContainerId = Candidate.ActiveRunInventorySession.Layout.BasicContainerId;
		Move.TargetSlot = MovedSpatialTargetSlot;
		Move.ExpectedRevision = ExpectedComposite.Revision;
		if (!ExpectedRepository.LoadPersistedSnapshot(ExpectedComposite, &Error)
			|| !ExpectedRepository.ExecuteTransaction(Move).IsSuccess()
			|| ExpectedRepository.CaptureSnapshot() != CompositeSnapshot)
		{
			if (OutError) *OutError = TEXT("Code B P20 transfer candidate differs from the one allowed P11-to-empty-P6-BaseQuick whole-graph Move.");
			return false;
		}
	}

	TSet<FGuid> TargetContainerIds;
	TSet<FGuid> TargetItemIds;
	TArray<FGuid> PendingContainers;
	PendingContainers.Add(Record->ContainerId);
	for (const FGuid& EquipmentContainerId : P21EquipmentContainerIds)
	{
		PendingContainers.Add(EquipmentContainerId);
	}
	while (!PendingContainers.IsEmpty())
	{
		const FGuid CurrentContainerId = PendingContainers.Pop();
		if (TargetContainerIds.Contains(CurrentContainerId)) continue;
		const FCodeBContainer* CurrentContainer = CompositeSnapshot.Containers.Find(CurrentContainerId);
		if (!CurrentContainer)
		{
			if (OutError) *OutError = TEXT("Code B P12 body transfer lost the exact body root or child container.");
			return false;
		}
		TargetContainerIds.Add(CurrentContainerId);
		for (const FGuid& ItemId : CurrentContainer->Slots)
		{
			if (!ItemId.IsValid()) continue;
			const FCodeBItemInstance* Item = CompositeSnapshot.Items.Find(ItemId);
			if (!Item || Item->ParentContainerId != CurrentContainerId || TargetItemIds.Contains(ItemId))
			{
				if (OutError) *OutError = TEXT("Code B P12 body transfer found an invalid body item placement.");
				return false;
			}
			TargetItemIds.Add(ItemId);
			if (Item->ChildContainerId.IsValid()) PendingContainers.Add(Item->ChildContainerId);
		}
	}

	FCodeBSnapshot PlayerSnapshot;
	FCodeBSnapshot TargetSnapshot;
	PlayerSnapshot.Revision = CompositeSnapshot.Revision;
	TargetSnapshot.Revision = CompositeSnapshot.Revision;
	PlayerSnapshot.Definitions = CompositeSnapshot.Definitions;
	TargetSnapshot.Definitions = CompositeSnapshot.Definitions;
	for (const TPair<FGuid, FCodeBContainer>& Pair : CompositeSnapshot.Containers)
	{
		(TargetContainerIds.Contains(Pair.Key) ? TargetSnapshot.Containers : PlayerSnapshot.Containers).Add(Pair.Key, Pair.Value);
	}
	for (const TPair<FGuid, FCodeBItemInstance>& Pair : CompositeSnapshot.Items)
	{
		(TargetItemIds.Contains(Pair.Key) ? TargetSnapshot.Items : PlayerSnapshot.Items).Add(Pair.Key, Pair.Value);
	}
	FCodeBRepository PlayerValidation;
	FCodeBRepository TargetValidation;
	if (!PlayerValidation.LoadPersistedSnapshot(PlayerSnapshot, &Error)
		|| !TargetValidation.LoadPersistedSnapshot(TargetSnapshot, &Error))
	{
		if (OutError) *OutError = TEXT("Code B P12 body transfer partition is not independently P1-valid: ") + Error;
		return false;
	}
	for (const FCodeBBodyContainerItemVisibility& Visibility : Record->ItemVisibilities)
	{
		if ((Visibility.Visibility == ECodeBBodyContainerVisibility::Hidden
				|| Visibility.Visibility == ECodeBBodyContainerVisibility::Searching)
			&& !TargetSnapshot.Items.Contains(Visibility.ItemId))
		{
			if (OutError) *OutError = TEXT("Code B P12 body transfer refused a Hidden or Searching item leaving the body.");
			return false;
		}
	}

	TMap<FGuid, ECodeBBodyContainerVisibility> ExistingVisibility;
	for (const FCodeBBodyContainerItemVisibility& Visibility : Record->ItemVisibilities)
	{
		ExistingVisibility.Add(Visibility.ItemId, Visibility.Visibility);
	}
	TArray<FCodeBBodyContainerItemVisibility> NextVisibilities;
	NextVisibilities.Reserve(TargetSnapshot.Items.Num());
	for (const TPair<FGuid, FCodeBItemInstance>& Pair : TargetSnapshot.Items)
	{
		FCodeBBodyContainerItemVisibility& Visibility = NextVisibilities.AddDefaulted_GetRef();
		Visibility.ItemId = Pair.Key;
		if (const ECodeBBodyContainerVisibility* Existing = ExistingVisibility.Find(Pair.Key))
		{
			Visibility.Visibility = *Existing;
		}
		else
		{
			// A player-origin item is never hidden by a corpse search; it is an explicit
			// already-known Drop target and remains readable if later moved back out.
			Visibility.Visibility = ECodeBBodyContainerVisibility::Revealed;
		}
	}
	NextVisibilities.Sort([](const FCodeBBodyContainerItemVisibility& Left, const FCodeBBodyContainerItemVisibility& Right)
	{
		return Left.ItemId.ToString(EGuidFormats::DigitsWithHyphensLower)
			< Right.ItemId.ToString(EGuidFormats::DigitsWithHyphensLower);
	});

	FCodeBRunInventorySession& CandidateSession = Candidate.ActiveRunInventorySession;
	if (!ReconcileHotbarBindings(
		CandidateSession.HotbarBindings, PlayerSnapshot, CandidateSession.Layout, Error))
	{
		if (OutError) *OutError = Error;
		return false;
	}
	if (!FreezeRunInventoryPayloadReceipt(CandidateSession, Error))
	{
		if (OutError) *OutError = Error;
		return false;
	}
	CandidateSession.RepositorySnapshot = MoveTemp(PlayerSnapshot);
	CandidateSession.LastCommittedUtc = UtcNow();
	++CandidateSession.SessionRevision;
	Record->SchemaVersion = FCodeBRunLocalBodyContainerRecord::CurrentSchemaVersion;
	Record->ContainerSnapshot = MoveTemp(TargetSnapshot);
	Record->ItemVisibilities = MoveTemp(NextVisibilities);
	++Record->Revision;
	Candidate.LastCommittedUtc = CandidateSession.LastCommittedUtc;
	++Candidate.PersistentRevision;
	if (!Store.SaveRecord(Candidate, Error))
	{
		if (OutError) *OutError = Error;
		return false;
	}
	return true;
}

FCodeBNormalContainerMaterializationResult FCodeBOutOfRaidProfileStore::MaterializeMatchedRunNormalContainer(
	const FString& InStorageRoot,
	const FGuid& InOwnerId,
	const FGuid& RunInstanceId,
	const FGuid& SearchTargetId,
	const FName DefinitionId)
{
	FCodeBNormalContainerMaterializationResult Result;
	if (!InOwnerId.IsValid() || !RunInstanceId.IsValid() || !SearchTargetId.IsValid() || DefinitionId.IsNone())
	{
		Result.Status = ECodeBNormalContainerMaterializationStatus::InvalidIdentity;
		Result.Diagnostic = TEXT("Code B normal-container materialization requires an exact OwnerId, RunInstanceId, SearchTargetId, and DefinitionId.");
		return Result;
	}
	const FCodeBNormalContainerDefinition* Definition = FindNormalContainerDefinitionInternal(DefinitionId);
	if (!Definition)
	{
		Result.Status = ECodeBNormalContainerMaterializationStatus::UnknownDefinition;
		Result.Diagnostic = TEXT("Code B normal-container materialization refused an unknown definition.");
		return Result;
	}

	FCodeBOutOfRaidProfileStore Store(InStorageRoot, InOwnerId);
	if (!IFileManager::Get().FileExists(*Store.GetPrimaryPath())
		&& !IFileManager::Get().FileExists(*Store.GetBackupPath()))
	{
		Result.Status = ECodeBNormalContainerMaterializationStatus::NotEnrolled;
		Result.Diagnostic = TEXT("Code B Profile sidecar is absent; normal-container materialization is intentionally not enrolled.");
		return Result;
	}
	FString Error;
	FCodeBOutOfRaidInventoryRecord Loaded;
	if (!Store.LoadRecord(Loaded, Error))
	{
		Result.Status = ECodeBNormalContainerMaterializationStatus::StorageFailure;
		Result.Diagnostic = Error;
		return Result;
	}
	if (!IsExactCommittedActiveRun(Loaded, InOwnerId, RunInstanceId))
	{
		Result.Status = ECodeBNormalContainerMaterializationStatus::ActiveSessionNotCommitted;
		Result.Diagnostic = TEXT("Code B normal-container materialization refused a missing, terminal, mismatched, or non-committed P6 session.");
		return Result;
	}
	for (const FCodeBRunLocalNormalContainerRecord& Existing : Loaded.RunLocalNormalContainers)
	{
		if (Existing.SearchTargetId != SearchTargetId) continue;
		if (Existing.DefinitionId != DefinitionId)
		{
			Result.Status = ECodeBNormalContainerMaterializationStatus::TargetDefinitionConflict;
			Result.Diagnostic = TEXT("Code B normal-container materialization refused a second definition for an existing SearchTargetId.");
			return Result;
		}
		if (!BuildNormalContainerProjection(Existing, Result.Projection, Error))
		{
			Result.Status = ECodeBNormalContainerMaterializationStatus::StorageFailure;
			Result.Diagnostic = Error;
			return Result;
		}
		Result.Status = ECodeBNormalContainerMaterializationStatus::AlreadyMaterialized;
		Result.Diagnostic = TEXT("Code B normal-container materialization returned its existing Run-local target record without writing.");
		return Result;
	}
	if (Loaded.PersistentRevision == MAX_int32)
	{
		Result.Status = ECodeBNormalContainerMaterializationStatus::StorageFailure;
		Result.Diagnostic = TEXT("Code B normal-container materialization cannot advance the durable Profile revision.");
		return Result;
	}
	FCodeBRunLocalNormalContainerRecord Materialized;
	if (!BuildMaterializedNormalContainerRecord(
		InOwnerId, RunInstanceId, SearchTargetId, *Definition, Materialized, Error))
	{
		Result.Status = ECodeBNormalContainerMaterializationStatus::InvalidDefinition;
		Result.Diagnostic = Error;
		return Result;
	}
	FCodeBOutOfRaidInventoryRecord Candidate = Loaded;
	Candidate.RunLocalNormalContainers.Add(MoveTemp(Materialized));
	Candidate.LastCommittedUtc = UtcNow();
	++Candidate.PersistentRevision;
	if (!Store.SaveRecord(Candidate, Error))
	{
		Result.Status = ECodeBNormalContainerMaterializationStatus::StorageFailure;
		Result.Diagnostic = Error;
		return Result;
	}
	Store.Record = MoveTemp(Candidate);
	Store.bHasRecord = true;
	if (!BuildNormalContainerProjection(Store.Record.RunLocalNormalContainers.Last(), Result.Projection, Error))
	{
		Result.Status = ECodeBNormalContainerMaterializationStatus::StorageFailure;
		Result.Diagnostic = Error;
		return Result;
	}
	Result.Status = ECodeBNormalContainerMaterializationStatus::Materialized;
	Result.Diagnostic = TEXT("Code B atomically materialized one Hidden Run-local normal-container graph.");
	return Result;
}

bool FCodeBOutOfRaidProfileStore::TryGetMatchedRunNormalContainerProjection(
	const FString& InStorageRoot,
	const FGuid& InOwnerId,
	const FGuid& RunInstanceId,
	const FGuid& SearchTargetId,
	FCodeBNormalContainerProjection& OutProjection,
	FString* OutError)
{
	OutProjection = FCodeBNormalContainerProjection();
	if (!InOwnerId.IsValid() || !RunInstanceId.IsValid() || !SearchTargetId.IsValid())
	{
		if (OutError) *OutError = TEXT("Code B normal-container projection requires exact OwnerId, RunInstanceId, and SearchTargetId.");
		return false;
	}
	FCodeBOutOfRaidProfileStore Store(InStorageRoot, InOwnerId);
	if (!IFileManager::Get().FileExists(*Store.GetPrimaryPath())
		&& !IFileManager::Get().FileExists(*Store.GetBackupPath()))
	{
		if (OutError) *OutError = TEXT("Code B Profile sidecar is absent; normal-container projection cannot create one.");
		return false;
	}
	FString Error;
	FCodeBOutOfRaidInventoryRecord Loaded;
	if (!Store.LoadRecord(Loaded, Error))
	{
		if (OutError) *OutError = Error;
		return false;
	}
	if (!IsExactCommittedActiveRun(Loaded, InOwnerId, RunInstanceId))
	{
		if (OutError) *OutError = TEXT("Code B normal-container projection refused a missing, terminal, mismatched, or non-committed P6 session.");
		return false;
	}
	for (const FCodeBRunLocalNormalContainerRecord& Existing : Loaded.RunLocalNormalContainers)
	{
		if (Existing.SearchTargetId == SearchTargetId)
		{
			if (!BuildNormalContainerProjection(Existing, OutProjection, Error))
			{
				if (OutError) *OutError = Error;
				return false;
			}
			return true;
		}
	}
	if (OutError) *OutError = TEXT("Code B normal-container projection found no materialized record for the exact SearchTargetId.");
	return false;
}

FCodeBNormalContainerActionResult
FCodeBOutOfRaidProfileStore::BeginMatchedRunNormalContainerOpen(
	const FString& InStorageRoot,
	const FGuid& InOwnerId,
	const FGuid& RunInstanceId,
	const FGuid& SearchTargetId,
	const FName DefinitionId)
{
	FCodeBNormalContainerActionResult Result;
	if (!InOwnerId.IsValid() || !RunInstanceId.IsValid()
		|| !SearchTargetId.IsValid() || DefinitionId.IsNone())
	{
		Result.Status = ECodeBNormalContainerActionStatus::InvalidIdentity;
		Result.Diagnostic = TEXT("Code B P10 open requires an exact OwnerId, RunInstanceId, SearchTargetId, and DefinitionId.");
		return Result;
	}
	const FCodeBNormalContainerMaterializationResult Materialized =
		MaterializeMatchedRunNormalContainer(
			InStorageRoot, InOwnerId, RunInstanceId, SearchTargetId, DefinitionId);
	if (!Materialized.IsMaterialized())
	{
		Result.Diagnostic = Materialized.Diagnostic;
		Result.Status = Materialized.Status == ECodeBNormalContainerMaterializationStatus::NotEnrolled
			? ECodeBNormalContainerActionStatus::NotEnrolled
			: Materialized.Status == ECodeBNormalContainerMaterializationStatus::ActiveSessionNotCommitted
				? ECodeBNormalContainerActionStatus::ActiveSessionNotCommitted
				: Materialized.Status == ECodeBNormalContainerMaterializationStatus::TargetDefinitionConflict
					? ECodeBNormalContainerActionStatus::TargetDefinitionConflict
					: Materialized.Status == ECodeBNormalContainerMaterializationStatus::InvalidIdentity
						? ECodeBNormalContainerActionStatus::InvalidIdentity
						: ECodeBNormalContainerActionStatus::StorageFailure;
		return Result;
	}

	FCodeBOutOfRaidProfileStore Store(InStorageRoot, InOwnerId);
	FCodeBRunInventorySession Session;
	FString Error;
	if (!Store.OpenMatchedActiveRunInventorySession(RunInstanceId, Session, &Error))
	{
		Result.Status = ECodeBNormalContainerActionStatus::ActiveSessionNotCommitted;
		Result.Diagnostic = Error;
		return Result;
	}
	FCodeBOutOfRaidInventoryRecord Candidate = Store.Record;
	FCodeBRunLocalNormalContainerRecord* Record = Candidate.RunLocalNormalContainers.FindByPredicate(
		[SearchTargetId](const FCodeBRunLocalNormalContainerRecord& Value)
		{
			return Value.SearchTargetId == SearchTargetId;
		});
	if (!Record)
	{
		Result.Status = ECodeBNormalContainerActionStatus::NotMaterialized;
		Result.Diagnostic = TEXT("Code B P10 open found no exact materialized normal-container record.");
		return Result;
	}
	if (Record->DefinitionId != DefinitionId)
	{
		Result.Status = ECodeBNormalContainerActionStatus::TargetDefinitionConflict;
		Result.Diagnostic = TEXT("Code B P10 open refused a DefinitionId different from the materialized target.");
		return Result;
	}
	if (Record->State == ECodeBNormalContainerState::Open)
	{
		if (!BuildNormalContainerProjection(*Record, Result.Projection, Error))
		{
			Result.Status = ECodeBNormalContainerActionStatus::StorageFailure;
			Result.Diagnostic = Error;
			return Result;
		}
		Result.Status = ECodeBNormalContainerActionStatus::AlreadyInRequestedState;
		Result.Diagnostic = TEXT("Code B P10 reopen returned the existing Open projection without writing.");
		return Result;
	}
	if (Record->State == ECodeBNormalContainerState::Opening)
	{
		// A live timer owns its non-zero action id. A later fresh interaction must
		// first recover it through the explicit cancellation/recovery entry.
		if (!BuildNormalContainerProjection(*Record, Result.Projection, Error))
		{
			Result.Status = ECodeBNormalContainerActionStatus::StorageFailure;
			Result.Diagnostic = Error;
			return Result;
		}
		Result.Status = ECodeBNormalContainerActionStatus::AlreadyInRequestedState;
		Result.Diagnostic = TEXT("Code B P10 open retained the exact in-flight opening action without duplicating it.");
		return Result;
	}
	if (Record->State != ECodeBNormalContainerState::Closed
		&& Record->State != ECodeBNormalContainerState::Interrupted)
	{
		Result.Status = ECodeBNormalContainerActionStatus::InvalidState;
		Result.Diagnostic = TEXT("Code B P10 open found an invalid normal-container state.");
		return Result;
	}
	if (Candidate.PersistentRevision == MAX_int32 || Record->Revision == MAX_int32)
	{
		Result.Status = ECodeBNormalContainerActionStatus::StorageFailure;
		Result.Diagnostic = TEXT("Code B P10 open cannot advance its durable revision.");
		return Result;
	}
	Record->SchemaVersion = FCodeBRunLocalNormalContainerRecord::CurrentSchemaVersion;
	Record->State = ECodeBNormalContainerState::Opening;
	Record->ActiveActionId = FGuid::NewGuid();
	Record->ActiveSearchItemId.Invalidate();
	++Record->Revision;
	Candidate.LastCommittedUtc = UtcNow();
	++Candidate.PersistentRevision;
	if (!Store.SaveRecord(Candidate, Error))
	{
		Result.Status = ECodeBNormalContainerActionStatus::StorageFailure;
		Result.Diagnostic = Error;
		return Result;
	}
	if (!BuildNormalContainerProjection(*Record, Result.Projection, Error))
	{
		Result.Status = ECodeBNormalContainerActionStatus::StorageFailure;
		Result.Diagnostic = Error;
		return Result;
	}
	Result.Status = ECodeBNormalContainerActionStatus::Committed;
	Result.Diagnostic = TEXT("Code B P10 atomically committed Closed/Interrupted to Opening for one exact normal-container target.");
	return Result;
}

FCodeBNormalContainerActionResult
FCodeBOutOfRaidProfileStore::CompleteMatchedRunNormalContainerOpen(
	const FString& InStorageRoot,
	const FGuid& InOwnerId,
	const FGuid& RunInstanceId,
	const FGuid& SearchTargetId,
	const FName DefinitionId,
	const FGuid& ActionId)
{
	FCodeBNormalContainerActionResult Result;
	if (!InOwnerId.IsValid() || !RunInstanceId.IsValid() || !SearchTargetId.IsValid()
		|| DefinitionId.IsNone() || !ActionId.IsValid())
	{
		Result.Status = ECodeBNormalContainerActionStatus::InvalidIdentity;
		Result.Diagnostic = TEXT("Code B P10 open completion requires the exact issued action identity.");
		return Result;
	}
	FCodeBOutOfRaidProfileStore Store(InStorageRoot, InOwnerId);
	FCodeBRunInventorySession Session;
	FString Error;
	if (!Store.OpenMatchedActiveRunInventorySession(RunInstanceId, Session, &Error))
	{
		Result.Status = ECodeBNormalContainerActionStatus::ActiveSessionNotCommitted;
		Result.Diagnostic = Error;
		return Result;
	}
	FCodeBOutOfRaidInventoryRecord Candidate = Store.Record;
	FCodeBRunLocalNormalContainerRecord* Record = Candidate.RunLocalNormalContainers.FindByPredicate(
		[SearchTargetId](const FCodeBRunLocalNormalContainerRecord& Value) { return Value.SearchTargetId == SearchTargetId; });
	if (!Record)
	{
		Result.Status = ECodeBNormalContainerActionStatus::NotMaterialized;
		Result.Diagnostic = TEXT("Code B P10 open completion found no exact materialized target.");
		return Result;
	}
	if (Record->DefinitionId != DefinitionId)
	{
		Result.Status = ECodeBNormalContainerActionStatus::TargetDefinitionConflict;
		Result.Diagnostic = TEXT("Code B P10 open completion refused a DefinitionId mismatch.");
		return Result;
	}
	if (Record->State == ECodeBNormalContainerState::Open && !Record->ActiveActionId.IsValid())
	{
		if (!BuildNormalContainerProjection(*Record, Result.Projection, Error))
		{
			Result.Status = ECodeBNormalContainerActionStatus::StorageFailure;
			Result.Diagnostic = Error;
			return Result;
		}
		Result.Status = ECodeBNormalContainerActionStatus::AlreadyInRequestedState;
		Result.Diagnostic = TEXT("Code B P10 open completion found the exact target already Open.");
		return Result;
	}
	if (Record->State != ECodeBNormalContainerState::Opening || Record->ActiveActionId != ActionId)
	{
		Result.Status = ECodeBNormalContainerActionStatus::ActionMismatch;
		Result.Diagnostic = TEXT("Code B P10 open completion rejected a stale, cancelled, or mismatched action.");
		return Result;
	}
	if (Candidate.PersistentRevision == MAX_int32 || Record->Revision == MAX_int32)
	{
		Result.Status = ECodeBNormalContainerActionStatus::StorageFailure;
		Result.Diagnostic = TEXT("Code B P10 open completion cannot advance its durable revision.");
		return Result;
	}
	Record->State = ECodeBNormalContainerState::Open;
	Record->ActiveActionId.Invalidate();
	++Record->Revision;
	Candidate.LastCommittedUtc = UtcNow();
	++Candidate.PersistentRevision;
	if (!Store.SaveRecord(Candidate, Error) || !BuildNormalContainerProjection(*Record, Result.Projection, Error))
	{
		Result.Status = ECodeBNormalContainerActionStatus::StorageFailure;
		Result.Diagnostic = Error;
		return Result;
	}
	Result.Status = ECodeBNormalContainerActionStatus::Committed;
	Result.Diagnostic = TEXT("Code B P10 atomically completed the exact normal-container opening action.");
	return Result;
}

FCodeBNormalContainerActionResult
FCodeBOutOfRaidProfileStore::BeginMatchedRunNormalContainerItemSearch(
	const FString& InStorageRoot,
	const FGuid& InOwnerId,
	const FGuid& RunInstanceId,
	const FGuid& SearchTargetId,
	const FName DefinitionId,
	const FGuid& ItemId)
{
	FCodeBNormalContainerActionResult Result;
	if (!InOwnerId.IsValid() || !RunInstanceId.IsValid() || !SearchTargetId.IsValid()
		|| DefinitionId.IsNone() || !ItemId.IsValid())
	{
		Result.Status = ECodeBNormalContainerActionStatus::InvalidIdentity;
		Result.Diagnostic = TEXT("Code B P10 item search requires exact target and item identities.");
		return Result;
	}
	FCodeBOutOfRaidProfileStore Store(InStorageRoot, InOwnerId);
	FCodeBRunInventorySession Session;
	FString Error;
	if (!Store.OpenMatchedActiveRunInventorySession(RunInstanceId, Session, &Error))
	{
		Result.Status = ECodeBNormalContainerActionStatus::ActiveSessionNotCommitted;
		Result.Diagnostic = Error;
		return Result;
	}
	FCodeBOutOfRaidInventoryRecord Candidate = Store.Record;
	FCodeBRunLocalNormalContainerRecord* Record = Candidate.RunLocalNormalContainers.FindByPredicate(
		[SearchTargetId](const FCodeBRunLocalNormalContainerRecord& Value) { return Value.SearchTargetId == SearchTargetId; });
	if (!Record)
	{
		Result.Status = ECodeBNormalContainerActionStatus::NotMaterialized;
		Result.Diagnostic = TEXT("Code B P10 item search found no exact materialized target.");
		return Result;
	}
	if (Record->DefinitionId != DefinitionId)
	{
		Result.Status = ECodeBNormalContainerActionStatus::TargetDefinitionConflict;
		Result.Diagnostic = TEXT("Code B P10 item search refused a DefinitionId mismatch.");
		return Result;
	}
	FCodeBNormalContainerItemReveal* Reveal = Record->ItemRevealStates.FindByPredicate(
		[ItemId](const FCodeBNormalContainerItemReveal& Value) { return Value.ItemId == ItemId; });
	if (!Reveal || !Record->ContainerSnapshot.Items.Contains(ItemId))
	{
		Result.Status = ECodeBNormalContainerActionStatus::ItemNotFound;
		Result.Diagnostic = TEXT("Code B P10 item search refused an item outside the exact current target snapshot.");
		return Result;
	}
	if (Record->State != ECodeBNormalContainerState::Open || Record->ActiveActionId.IsValid()
		|| Record->ActiveSearchItemId.IsValid())
	{
		Result.Status = ECodeBNormalContainerActionStatus::InvalidState;
		Result.Diagnostic = TEXT("Code B P10 item search requires an Open target with no in-flight action.");
		return Result;
	}
	if (Reveal->RevealState != ECodeBNormalContainerRevealState::Hidden)
	{
		Result.Status = ECodeBNormalContainerActionStatus::ItemNotHidden;
		Result.Diagnostic = TEXT("Code B P10 item search refused a non-Hidden item without rerolling or re-hiding it.");
		return Result;
	}
	if (Candidate.PersistentRevision == MAX_int32 || Record->Revision == MAX_int32)
	{
		Result.Status = ECodeBNormalContainerActionStatus::StorageFailure;
		Result.Diagnostic = TEXT("Code B P10 item search cannot advance its durable revision.");
		return Result;
	}
	Record->SchemaVersion = FCodeBRunLocalNormalContainerRecord::CurrentSchemaVersion;
	Reveal->RevealState = ECodeBNormalContainerRevealState::Searching;
	Record->ActiveSearchItemId = ItemId;
	Record->ActiveActionId = FGuid::NewGuid();
	++Record->Revision;
	Candidate.LastCommittedUtc = UtcNow();
	++Candidate.PersistentRevision;
	if (!Store.SaveRecord(Candidate, Error) || !BuildNormalContainerProjection(*Record, Result.Projection, Error))
	{
		Result.Status = ECodeBNormalContainerActionStatus::StorageFailure;
		Result.Diagnostic = Error;
		return Result;
	}
	Result.Status = ECodeBNormalContainerActionStatus::Committed;
	Result.Diagnostic = TEXT("Code B P10 atomically began one exact Hidden-to-Searching normal-container reveal.");
	return Result;
}

FCodeBNormalContainerActionResult
FCodeBOutOfRaidProfileStore::CompleteMatchedRunNormalContainerItemSearch(
	const FString& InStorageRoot,
	const FGuid& InOwnerId,
	const FGuid& RunInstanceId,
	const FGuid& SearchTargetId,
	const FName DefinitionId,
	const FGuid& ActionId)
{
	FCodeBNormalContainerActionResult Result;
	if (!InOwnerId.IsValid() || !RunInstanceId.IsValid() || !SearchTargetId.IsValid()
		|| DefinitionId.IsNone() || !ActionId.IsValid())
	{
		Result.Status = ECodeBNormalContainerActionStatus::InvalidIdentity;
		Result.Diagnostic = TEXT("Code B P10 search completion requires the exact issued action identity.");
		return Result;
	}
	FCodeBOutOfRaidProfileStore Store(InStorageRoot, InOwnerId);
	FCodeBRunInventorySession Session;
	FString Error;
	if (!Store.OpenMatchedActiveRunInventorySession(RunInstanceId, Session, &Error))
	{
		Result.Status = ECodeBNormalContainerActionStatus::ActiveSessionNotCommitted;
		Result.Diagnostic = Error;
		return Result;
	}
	FCodeBOutOfRaidInventoryRecord Candidate = Store.Record;
	FCodeBRunLocalNormalContainerRecord* Record = Candidate.RunLocalNormalContainers.FindByPredicate(
		[SearchTargetId](const FCodeBRunLocalNormalContainerRecord& Value) { return Value.SearchTargetId == SearchTargetId; });
	if (!Record)
	{
		Result.Status = ECodeBNormalContainerActionStatus::NotMaterialized;
		Result.Diagnostic = TEXT("Code B P10 search completion found no exact materialized target.");
		return Result;
	}
	if (Record->DefinitionId != DefinitionId)
	{
		Result.Status = ECodeBNormalContainerActionStatus::TargetDefinitionConflict;
		Result.Diagnostic = TEXT("Code B P10 search completion refused a DefinitionId mismatch.");
		return Result;
	}
	if (Record->State != ECodeBNormalContainerState::Open
		|| Record->ActiveActionId != ActionId
		|| !Record->ActiveSearchItemId.IsValid())
	{
		Result.Status = ECodeBNormalContainerActionStatus::ActionMismatch;
		Result.Diagnostic = TEXT("Code B P10 search completion rejected a stale, cancelled, or mismatched action.");
		return Result;
	}
	FCodeBNormalContainerItemReveal* Reveal = Record->ItemRevealStates.FindByPredicate(
		[Record](const FCodeBNormalContainerItemReveal& Value) { return Value.ItemId == Record->ActiveSearchItemId; });
	if (!Reveal || Reveal->RevealState != ECodeBNormalContainerRevealState::Searching
		|| !Record->ContainerSnapshot.Items.Contains(Record->ActiveSearchItemId))
	{
		Result.Status = ECodeBNormalContainerActionStatus::StorageFailure;
		Result.Diagnostic = TEXT("Code B P10 search completion found an invalid durable searching item.");
		return Result;
	}
	if (Candidate.PersistentRevision == MAX_int32 || Record->Revision == MAX_int32)
	{
		Result.Status = ECodeBNormalContainerActionStatus::StorageFailure;
		Result.Diagnostic = TEXT("Code B P10 search completion cannot advance its durable revision.");
		return Result;
	}
	Reveal->RevealState = ECodeBNormalContainerRevealState::Revealed;
	Record->ActiveActionId.Invalidate();
	Record->ActiveSearchItemId.Invalidate();
	++Record->Revision;
	Candidate.LastCommittedUtc = UtcNow();
	++Candidate.PersistentRevision;
	if (!Store.SaveRecord(Candidate, Error) || !BuildNormalContainerProjection(*Record, Result.Projection, Error))
	{
		Result.Status = ECodeBNormalContainerActionStatus::StorageFailure;
		Result.Diagnostic = Error;
		return Result;
	}
	Result.Status = ECodeBNormalContainerActionStatus::Committed;
	Result.Diagnostic = TEXT("Code B P10 atomically completed Searching-to-Revealed without rerolling the item.");
	return Result;
}

FCodeBNormalContainerActionResult
FCodeBOutOfRaidProfileStore::InterruptMatchedRunNormalContainerAction(
	const FString& InStorageRoot,
	const FGuid& InOwnerId,
	const FGuid& RunInstanceId,
	const FGuid& SearchTargetId,
	const FName DefinitionId,
	const FString& Reason)
{
	FCodeBNormalContainerActionResult Result;
	if (!InOwnerId.IsValid() || !RunInstanceId.IsValid() || !SearchTargetId.IsValid() || DefinitionId.IsNone())
	{
		Result.Status = ECodeBNormalContainerActionStatus::InvalidIdentity;
		Result.Diagnostic = TEXT("Code B P10 action interruption requires an exact target identity.");
		return Result;
	}
	FCodeBOutOfRaidProfileStore Store(InStorageRoot, InOwnerId);
	FCodeBRunInventorySession Session;
	FString Error;
	if (!Store.OpenMatchedActiveRunInventorySession(RunInstanceId, Session, &Error))
	{
		Result.Status = ECodeBNormalContainerActionStatus::ActiveSessionNotCommitted;
		Result.Diagnostic = Error;
		return Result;
	}
	FCodeBOutOfRaidInventoryRecord Candidate = Store.Record;
	FCodeBRunLocalNormalContainerRecord* Record = Candidate.RunLocalNormalContainers.FindByPredicate(
		[SearchTargetId](const FCodeBRunLocalNormalContainerRecord& Value) { return Value.SearchTargetId == SearchTargetId; });
	if (!Record)
	{
		Result.Status = ECodeBNormalContainerActionStatus::NotMaterialized;
		Result.Diagnostic = TEXT("Code B P10 action interruption found no exact materialized target.");
		return Result;
	}
	if (Record->DefinitionId != DefinitionId)
	{
		Result.Status = ECodeBNormalContainerActionStatus::TargetDefinitionConflict;
		Result.Diagnostic = TEXT("Code B P10 action interruption refused a DefinitionId mismatch.");
		return Result;
	}
	if (!Record->ActiveActionId.IsValid())
	{
		if (!BuildNormalContainerProjection(*Record, Result.Projection, Error))
		{
			Result.Status = ECodeBNormalContainerActionStatus::StorageFailure;
			Result.Diagnostic = Error;
			return Result;
		}
		Result.Status = ECodeBNormalContainerActionStatus::AlreadyInRequestedState;
		Result.Diagnostic = TEXT("Code B P10 action interruption found no pending action to cancel.");
		return Result;
	}
	if (Candidate.PersistentRevision == MAX_int32 || Record->Revision == MAX_int32)
	{
		Result.Status = ECodeBNormalContainerActionStatus::StorageFailure;
		Result.Diagnostic = TEXT("Code B P10 action interruption cannot advance its durable revision.");
		return Result;
	}
	if (Record->State == ECodeBNormalContainerState::Opening)
	{
		Record->State = ECodeBNormalContainerState::Interrupted;
	}
	else if (Record->State == ECodeBNormalContainerState::Open && Record->ActiveSearchItemId.IsValid())
	{
		FCodeBNormalContainerItemReveal* Reveal = Record->ItemRevealStates.FindByPredicate(
			[Record](const FCodeBNormalContainerItemReveal& Value) { return Value.ItemId == Record->ActiveSearchItemId; });
		if (!Reveal || Reveal->RevealState != ECodeBNormalContainerRevealState::Searching)
		{
			Result.Status = ECodeBNormalContainerActionStatus::StorageFailure;
			Result.Diagnostic = TEXT("Code B P10 action interruption found no matching Searching item.");
			return Result;
		}
		Reveal->RevealState = ECodeBNormalContainerRevealState::Hidden;
	}
	else
	{
		Result.Status = ECodeBNormalContainerActionStatus::InvalidState;
		Result.Diagnostic = TEXT("Code B P10 action interruption found an invalid pending action state.");
		return Result;
	}
	Record->ActiveActionId.Invalidate();
	Record->ActiveSearchItemId.Invalidate();
	++Record->Revision;
	Candidate.LastCommittedUtc = UtcNow();
	++Candidate.PersistentRevision;
	if (!Store.SaveRecord(Candidate, Error) || !BuildNormalContainerProjection(*Record, Result.Projection, Error))
	{
		Result.Status = ECodeBNormalContainerActionStatus::StorageFailure;
		Result.Diagnostic = Error;
		return Result;
	}
	Result.Status = ECodeBNormalContainerActionStatus::Committed;
	Result.Diagnostic = FString::Printf(TEXT("Code B P10 atomically interrupted the exact pending normal-container action: %s."), *Reason);
	return Result;
}

bool FCodeBOutOfRaidProfileStore::CommitAcceptedMatchedRunNormalContainerTransfer(
	const FString& InStorageRoot,
	const FGuid& InOwnerId,
	const FGuid& RunInstanceId,
	const FGuid& SearchTargetId,
	const FName DefinitionId,
	const int32 ExpectedP6SnapshotRevision,
	const int32 ExpectedNormalContainerRevision,
	const FCodeBSnapshot& CompositeSnapshot,
	FString* OutError)
{
	if (OutError) OutError->Reset();
	if (!InOwnerId.IsValid() || !RunInstanceId.IsValid() || !SearchTargetId.IsValid()
		|| DefinitionId.IsNone() || ExpectedP6SnapshotRevision < 0
		|| ExpectedNormalContainerRevision < 1)
	{
		if (OutError) *OutError = TEXT("Code B P10 dual transfer requires exact Owner/Run/target/definition and source revisions.");
		return false;
	}
	FCodeBRepository CompositeValidation;
	FString Error;
	if (!CompositeValidation.LoadPersistedSnapshot(CompositeSnapshot, &Error))
	{
		if (OutError) *OutError = TEXT("Code B P10 dual transfer refused an invalid P1 composite snapshot: ") + Error;
		return false;
	}
	FCodeBOutOfRaidProfileStore Store(InStorageRoot, InOwnerId);
	FCodeBRunInventorySession Session;
	if (!Store.OpenMatchedActiveRunInventorySession(RunInstanceId, Session, &Error))
	{
		if (OutError) *OutError = TEXT("Code B P10 dual transfer has no exact committed P6 session: ") + Error;
		return false;
	}
	FCodeBOutOfRaidInventoryRecord Candidate = Store.Record;
	FCodeBRunLocalNormalContainerRecord* Record = Candidate.RunLocalNormalContainers.FindByPredicate(
		[SearchTargetId](const FCodeBRunLocalNormalContainerRecord& Value) { return Value.SearchTargetId == SearchTargetId; });
	if (!Record || Record->DefinitionId != DefinitionId
		|| Record->State != ECodeBNormalContainerState::Open
		|| Record->ActiveActionId.IsValid() || Record->ActiveSearchItemId.IsValid()
		|| Candidate.ActiveRunInventorySession.RepositorySnapshot.Revision != ExpectedP6SnapshotRevision
		|| Record->Revision != ExpectedNormalContainerRevision)
	{
		if (OutError) *OutError = TEXT("Code B P10 dual transfer refused a stale, non-Open, action-pending, or definition-mismatched target.");
		return false;
	}
	if (Candidate.PersistentRevision == MAX_int32
		|| Candidate.ActiveRunInventorySession.SessionRevision == MAX_int32
		|| Record->Revision == MAX_int32)
	{
		if (OutError) *OutError = TEXT("Code B P10 dual transfer cannot advance its durable revision.");
		return false;
	}

	// The P1 composite must contain exactly the prior P6 and P9 identities,
	// except that a legal merge may consume a Revealed source stack. It cannot
	// invent an item, container, or definition by writing a UI projection.
	TSet<FGuid> PriorItemIds;
	TSet<FGuid> PriorContainerIds;
	for (const TPair<FGuid, FCodeBItemInstance>& Pair : Candidate.ActiveRunInventorySession.RepositorySnapshot.Items)
	{
		PriorItemIds.Add(Pair.Key);
	}
	for (const TPair<FGuid, FCodeBItemInstance>& Pair : Record->ContainerSnapshot.Items)
	{
		if (PriorItemIds.Contains(Pair.Key))
		{
			if (OutError) *OutError = TEXT("Code B P10 dual transfer found a duplicate P6/P9 item identity.");
			return false;
		}
		PriorItemIds.Add(Pair.Key);
	}
	for (const TPair<FGuid, FCodeBContainer>& Pair : Candidate.ActiveRunInventorySession.RepositorySnapshot.Containers)
	{
		PriorContainerIds.Add(Pair.Key);
	}
	for (const TPair<FGuid, FCodeBContainer>& Pair : Record->ContainerSnapshot.Containers)
	{
		if (PriorContainerIds.Contains(Pair.Key))
		{
			if (OutError) *OutError = TEXT("Code B P10 dual transfer found a duplicate P6/P9 container identity.");
			return false;
		}
		PriorContainerIds.Add(Pair.Key);
	}
	FCodeBSnapshot P24PriorComposite;
	if (!BuildP24PriorComposite(
		Candidate.ActiveRunInventorySession.RepositorySnapshot,
		Record->ContainerSnapshot, P24PriorComposite, Error))
	{
		if (OutError) *OutError = Error;
		return false;
	}
	bool bHasP24CreatedIdentity = false;
	for (const TPair<FGuid, FCodeBItemInstance>& Pair : CompositeSnapshot.Items)
	{
		if (!PriorItemIds.Contains(Pair.Key))
		{
			bHasP24CreatedIdentity = true;
			break;
		}
	}
	const bool bAcceptedP24SplitIdentity = bHasP24CreatedIdentity
		&& IsExactP24SplitDelta(P24PriorComposite, CompositeSnapshot, Error);
	if (bHasP24CreatedIdentity && !bAcceptedP24SplitIdentity)
	{
		if (OutError) *OutError = TEXT("Code B P10 dual transfer refused a non-P1 Split identity: ") + Error;
		return false;
	}
	const bool bHasP25MergeDelta = HasP25MergeQuantityOrRemovalDelta(
		P24PriorComposite, CompositeSnapshot);
	if (!bAcceptedP24SplitIdentity && bHasP25MergeDelta
		&& !IsExactP25MergeDelta(P24PriorComposite, CompositeSnapshot, Error))
	{
		if (OutError) *OutError = TEXT("Code B P10 dual transfer refused a non-P1 Merge quantity delta: ") + Error;
		return false;
	}
	for (const TPair<FGuid, FCodeBItemInstance>& Pair : CompositeSnapshot.Items)
	{
		if (!PriorItemIds.Contains(Pair.Key) && !bAcceptedP24SplitIdentity)
		{
			if (OutError) *OutError = TEXT("Code B P10 dual transfer refused an invented item identity.");
			return false;
		}
	}
	for (const TPair<FGuid, FCodeBContainer>& Pair : CompositeSnapshot.Containers)
	{
		if (!PriorContainerIds.Contains(Pair.Key))
		{
			if (OutError) *OutError = TEXT("Code B P10 dual transfer refused an invented container identity.");
			return false;
		}
	}

	// P18 admits only a revealed spatial parent originating at this exact
	// BasicCache root.  Its P17 child is never an independently draggable
	// item: the pre/post composite must retain that stable, empty child graph
	// whole, and a moved parent must land in an existing P6 container.
	TMap<FGuid, ECodeBNormalContainerRevealState> RevealStates;
	for (const FCodeBNormalContainerItemReveal& Reveal : Record->ItemRevealStates)
	{
		RevealStates.Add(Reveal.ItemId, Reveal.RevealState);
	}
	for (const TPair<FGuid, FCodeBItemInstance>& Pair : Record->ContainerSnapshot.Items)
	{
		const FCodeBItemInstance& OriginalItem = Pair.Value;
		if (OriginalItem.ParentContainerId != Record->ContainerId) continue;
		FCodeBItemDefinition CanonicalDefinition;
		if (!BuildCanonicalCodeBItemDefinition(OriginalItem.DefinitionId, CanonicalDefinition, Error))
		{
			if (OutError) *OutError = TEXT("Code B P18 transfer cannot resolve a BasicCache root definition: ") + Error;
			return false;
		}
		if (CanonicalDefinition.SpatialContainerSemantic == ECodeBSpatialContainerSemantic::None) continue;
		const ECodeBNormalContainerRevealState* RevealState = RevealStates.Find(OriginalItem.ItemId);
		const FGuid ExpectedChildId = SpatialChildGuid(OriginalItem.ItemId);
		const FCodeBContainer* OriginalChild = Record->ContainerSnapshot.Containers.Find(ExpectedChildId);
		const FCodeBItemInstance* FinalItem = CompositeSnapshot.Items.Find(OriginalItem.ItemId);
		const FCodeBContainer* FinalChild = CompositeSnapshot.Containers.Find(ExpectedChildId);
		if (!RevealState || *RevealState != ECodeBNormalContainerRevealState::Revealed
			|| OriginalItem.ChildContainerId != ExpectedChildId || !OriginalChild || !FinalItem || !FinalChild
			|| OriginalChild->Slots.Num() != CanonicalDefinition.ChildContainerCapacity
			|| OriginalChild->Slots.ContainsByPredicate([](const FGuid& Value) { return Value.IsValid(); })
			|| FinalItem->DefinitionId != OriginalItem.DefinitionId || FinalItem->Quantity != OriginalItem.Quantity
			|| FinalItem->ChildContainerId != ExpectedChildId
			|| FinalChild->ContainerType != OriginalChild->ContainerType
			|| FinalChild->Slots != OriginalChild->Slots)
		{
			if (OutError) *OutError = TEXT("Code B P18 transfer refused a non-revealed, partial, cloned, or mutated spatial graph.");
			return false;
		}
		if (FinalItem->ParentContainerId == Record->ContainerId)
		{
			if (FinalItem->SlotIndex != OriginalItem.SlotIndex)
			{
				if (OutError) *OutError = TEXT("Code B P18 transfer refused to rearrange an untransferred BasicCache spatial parent.");
				return false;
			}
			continue;
		}
		const FCodeBContainer* Destination = CompositeSnapshot.Containers.Find(FinalItem->ParentContainerId);
		if (!Destination || !Candidate.ActiveRunInventorySession.RepositorySnapshot.Containers.Contains(FinalItem->ParentContainerId)
			|| !Destination->Slots.IsValidIndex(FinalItem->SlotIndex)
			|| Destination->Slots[FinalItem->SlotIndex] != FinalItem->ItemId)
		{
			if (OutError) *OutError = TEXT("Code B P18 transfer refused a spatial parent destination outside the exact P6 graph.");
			return false;
		}
	}

	// Partition the validated P1 graph by the exact P9 root. Any child storage
	// follows its owner, keeping item/container ownership single and preventing
	// a half-moved spatial graph.
	TSet<FGuid> TargetContainerIds;
	TSet<FGuid> TargetItemIds;
	TArray<FGuid> PendingContainers;
	PendingContainers.Add(Record->ContainerId);
	while (!PendingContainers.IsEmpty())
	{
		const FGuid CurrentContainerId = PendingContainers.Pop();
		if (TargetContainerIds.Contains(CurrentContainerId)) continue;
		const FCodeBContainer* CurrentContainer = CompositeSnapshot.Containers.Find(CurrentContainerId);
		if (!CurrentContainer)
		{
			if (OutError) *OutError = TEXT("Code B P10 dual transfer lost the exact target root or child container.");
			return false;
		}
		TargetContainerIds.Add(CurrentContainerId);
		for (const FGuid& ItemId : CurrentContainer->Slots)
		{
			if (!ItemId.IsValid()) continue;
			const FCodeBItemInstance* Item = CompositeSnapshot.Items.Find(ItemId);
			if (!Item || Item->ParentContainerId != CurrentContainerId
				|| TargetItemIds.Contains(ItemId))
			{
				if (OutError) *OutError = TEXT("Code B P10 dual transfer found an invalid target item placement.");
				return false;
			}
			TargetItemIds.Add(ItemId);
			if (Item->ChildContainerId.IsValid()) PendingContainers.Add(Item->ChildContainerId);
		}
	}

	FCodeBSnapshot PlayerSnapshot;
	FCodeBSnapshot TargetSnapshot;
	PlayerSnapshot.Revision = CompositeSnapshot.Revision;
	TargetSnapshot.Revision = CompositeSnapshot.Revision;
	PlayerSnapshot.Definitions = CompositeSnapshot.Definitions;
	TargetSnapshot.Definitions = CompositeSnapshot.Definitions;
	for (const TPair<FGuid, FCodeBContainer>& Pair : CompositeSnapshot.Containers)
	{
		(TargetContainerIds.Contains(Pair.Key) ? TargetSnapshot.Containers : PlayerSnapshot.Containers).Add(Pair.Key, Pair.Value);
	}
	for (const TPair<FGuid, FCodeBItemInstance>& Pair : CompositeSnapshot.Items)
	{
		(TargetItemIds.Contains(Pair.Key) ? TargetSnapshot.Items : PlayerSnapshot.Items).Add(Pair.Key, Pair.Value);
	}
	FCodeBRepository PlayerValidation;
	FCodeBRepository TargetValidation;
	if (!PlayerValidation.LoadPersistedSnapshot(PlayerSnapshot, &Error)
		|| !TargetValidation.LoadPersistedSnapshot(TargetSnapshot, &Error))
	{
		if (OutError) *OutError = TEXT("Code B P10 dual transfer partition is not independently P1-valid: ") + Error;
		return false;
	}

	for (const FCodeBNormalContainerItemReveal& Reveal : Record->ItemRevealStates)
	{
		if ((Reveal.RevealState == ECodeBNormalContainerRevealState::Hidden
				|| Reveal.RevealState == ECodeBNormalContainerRevealState::Searching)
			&& !TargetSnapshot.Items.Contains(Reveal.ItemId))
		{
			if (OutError) *OutError = TEXT("Code B P10 dual transfer refused a Hidden or Searching item leaving its target.");
			return false;
		}
	}
	TMap<FGuid, ECodeBNormalContainerRevealState> ExistingRevealStates;
	for (const FCodeBNormalContainerItemReveal& Reveal : Record->ItemRevealStates)
	{
		ExistingRevealStates.Add(Reveal.ItemId, Reveal.RevealState);
	}
	TArray<FCodeBNormalContainerItemReveal> NextRevealStates;
	NextRevealStates.Reserve(TargetSnapshot.Items.Num());
	for (const TPair<FGuid, FCodeBItemInstance>& Pair : TargetSnapshot.Items)
	{
		FCodeBNormalContainerItemReveal& Reveal = NextRevealStates.AddDefaulted_GetRef();
		Reveal.ItemId = Pair.Key;
		if (const ECodeBNormalContainerRevealState* Existing = ExistingRevealStates.Find(Pair.Key))
		{
			Reveal.RevealState = *Existing;
		}
		else
		{
			// A player-origin split is an explicit known Drop, never newly hidden.
			Reveal.RevealState = ECodeBNormalContainerRevealState::Revealed;
		}
	}
	NextRevealStates.Sort([](const FCodeBNormalContainerItemReveal& Left, const FCodeBNormalContainerItemReveal& Right)
	{
		return Left.ItemId.ToString(EGuidFormats::DigitsWithHyphensLower)
			< Right.ItemId.ToString(EGuidFormats::DigitsWithHyphensLower);
	});

	FCodeBRunInventorySession& CandidateSession = Candidate.ActiveRunInventorySession;
	if (!ReconcileHotbarBindings(
		CandidateSession.HotbarBindings, PlayerSnapshot, CandidateSession.Layout, Error))
	{
		if (OutError) *OutError = Error;
		return false;
	}
	if (!FreezeRunInventoryPayloadReceipt(CandidateSession, Error))
	{
		if (OutError) *OutError = Error;
		return false;
	}
	CandidateSession.RepositorySnapshot = MoveTemp(PlayerSnapshot);
	CandidateSession.LastCommittedUtc = UtcNow();
	++CandidateSession.SessionRevision;
	Record->ContainerSnapshot = MoveTemp(TargetSnapshot);
	Record->ItemRevealStates = MoveTemp(NextRevealStates);
	++Record->Revision;
	Candidate.LastCommittedUtc = CandidateSession.LastCommittedUtc;
	++Candidate.PersistentRevision;
	if (!Store.SaveRecord(Candidate, Error))
	{
		if (OutError) *OutError = Error;
		return false;
	}
	return true;
}

#if WITH_DEV_AUTOMATION_TESTS
void FCodeBOutOfRaidProfileStore::SetInterruptAfterRunPreparedReceiptForLifecycleAutomation(
	bool bEnabled)
{
	GInterruptAfterRunPreparedReceiptForLifecycleAutomation = bEnabled;
}

bool FCodeBOutOfRaidProfileStore::TryReadRecordForLifecycleAutomation(
	const FString& InStorageRoot,
	const FGuid& InOwnerId,
	FCodeBOutOfRaidInventoryRecord& OutRecord,
	FString* OutError)
{
	OutRecord = FCodeBOutOfRaidInventoryRecord();
	if (OutError) OutError->Reset();
	FCodeBOutOfRaidProfileStore Store(InStorageRoot, InOwnerId);
	if (!InOwnerId.IsValid())
	{
		if (OutError) *OutError = TEXT("Code B lifecycle assertion received an invalid OwnerId.");
		return false;
	}
	FString Error;
	if (!Store.LoadRecord(OutRecord, Error))
	{
		if (OutError) *OutError = Error;
		return false;
	}
	return OutRecord.OwnerId == InOwnerId;
}
#endif

bool FCodeBOutOfRaidProfileStore::HasActiveRunInventorySession(
	const FString& InStorageRoot,
	const FGuid& InOwnerId,
	FString* OutError)
{
	if (OutError) OutError->Reset();
	FCodeBOutOfRaidProfileStore Store(InStorageRoot, InOwnerId);
	if (!InOwnerId.IsValid()
		|| (!IFileManager::Get().FileExists(*Store.GetPrimaryPath())
			&& !IFileManager::Get().FileExists(*Store.GetBackupPath())))
	{
		return false;
	}
	FCodeBOutOfRaidInventoryRecord Loaded;
	FString Error;
	if (!Store.LoadRecord(Loaded, Error))
	{
		if (OutError) *OutError = Error;
		return false;
	}
	return Loaded.bHasActiveRunInventorySession;
}

bool FCodeBOutOfRaidProfileStore::TryGetActiveRunInventorySession(
	FCodeBRunInventorySession& OutSession,
	FString* OutError) const
{
	OutSession = FCodeBRunInventorySession();
	if (!bHasRecord || !Record.bHasActiveRunInventorySession)
	{
		if (OutError) *OutError = TEXT("Code B has no active Run inventory session.");
		return false;
	}
	OutSession = Record.ActiveRunInventorySession;
	return true;
}

bool FCodeBOutOfRaidProfileStore::OpenMatchedActiveRunInventorySession(
	const FGuid& ExpectedRunInstanceId,
	FCodeBRunInventorySession& OutSession,
	FString* OutError)
{
	OutSession = FCodeBRunInventorySession();
	if (OutError) OutError->Reset();
	if (!OwnerId.IsValid() || !ExpectedRunInstanceId.IsValid())
	{
		if (OutError) *OutError = TEXT("Code B Run inventory query received an invalid OwnerId or RunInstanceId.");
		return false;
	}
	FCodeBOutOfRaidInventoryRecord Loaded;
	FString Error;
	if (!LoadRecord(Loaded, Error))
	{
		if (OutError) *OutError = Error;
		return false;
	}
	if (Loaded.OwnerId != OwnerId || !Loaded.bHasActiveRunInventorySession)
	{
		if (OutError) *OutError = TEXT("Code B has no Owner-matched active Run inventory session.");
		return false;
	}
	const FCodeBRunInventorySession& Session = Loaded.ActiveRunInventorySession;
	if (Session.OwnerId != OwnerId
		|| Session.BridgeState != ECodeBRunInventoryBridgeState::Committed
		|| Session.RunInstanceId != ExpectedRunInstanceId)
	{
		if (OutError) *OutError = TEXT("Code B active Run inventory session does not match the current committed RunId.");
		return false;
	}
	Record = MoveTemp(Loaded);
	bHasRecord = true;
	OutSession = Record.ActiveRunInventorySession;
	return true;
}

bool FCodeBOutOfRaidProfileStore::CommitAcceptedActiveRunInventorySnapshot(
	const FGuid& ExpectedRunInstanceId,
	const FCodeBSnapshot& Snapshot,
	FString* OutError)
{
	if (OutError) OutError->Reset();
	if (!bHasRecord || !OwnerId.IsValid() || !ExpectedRunInstanceId.IsValid()
		|| !Record.bHasActiveRunInventorySession || Record.OwnerId != OwnerId)
	{
		if (OutError) *OutError = TEXT("Code B Run inventory commit has no loaded Owner-matched active session.");
		return false;
	}
	if (Record.PersistentRevision == MAX_int32
		|| Record.ActiveRunInventorySession.SessionRevision == MAX_int32
		|| Record.ActiveRunInventorySession.BridgeState != ECodeBRunInventoryBridgeState::Committed
		|| Record.ActiveRunInventorySession.RunInstanceId != ExpectedRunInstanceId)
	{
		if (OutError) *OutError = TEXT("Code B Run inventory commit refused a stale or non-committed Run session.");
		return false;
	}

	FCodeBOutOfRaidInventoryRecord Candidate = Record;
	FCodeBRunInventorySession& Session = Candidate.ActiveRunInventorySession;
	FString Error;
	if (!FreezeRunInventoryPayloadReceipt(Session, Error))
	{
		if (OutError) *OutError = Error;
		return false;
	}
	Session.RepositorySnapshot = Snapshot;
	if (!ReconcileHotbarBindings(Session.HotbarBindings, Session.RepositorySnapshot, Session.Layout, Error))
	{
		if (OutError) *OutError = Error;
		return false;
	}
	if (!ValidateWorldDrops(Session, Error))
	{
		if (OutError) *OutError = Error;
		return false;
	}
	const FString CommitUtc = UtcNow();
	Session.LastCommittedUtc = CommitUtc;
	++Session.SessionRevision;
	Candidate.LastCommittedUtc = CommitUtc;
	++Candidate.PersistentRevision;
	if (!SaveRecord(Candidate, Error))
	{
		if (OutError) *OutError = Error;
		return false;
	}
	Record = MoveTemp(Candidate);
	return true;
}

bool FCodeBOutOfRaidProfileStore::UseMatchedActiveRunBoundQuickSlot(
	const FGuid& ExpectedRunInstanceId,
	const int32 SlotIndex,
	FCodeBQuickUseReceipt& OutReceipt,
	FString* OutError)
{
	OutReceipt = FCodeBQuickUseReceipt();
	if (OutError) OutError->Reset();
	FCodeBRunInventorySession Current;
	FString Error;
	if (!IsHotbarSlotIndex(SlotIndex)
		|| !OpenMatchedActiveRunInventorySession(ExpectedRunInstanceId, Current, &Error))
	{
		if (OutError) *OutError = Error.IsEmpty() ? TEXT("Code B P15 received an invalid quick-use request.") : Error;
		return false;
	}
	if (Record.PersistentRevision == MAX_int32 || Current.SessionRevision == MAX_int32
		|| Current.RepositorySnapshot.Revision == MAX_int32
		|| Current.NextQuickUseReceiptOrdinal < 1 || Current.NextQuickUseReceiptOrdinal == MAX_int32)
	{
		if (OutError) *OutError = TEXT("Code B P15 refused an exhausted durable revision or receipt ordinal.");
		return false;
	}
	const FCodeBHotbarBinding& Binding = Current.HotbarBindings.Slots[SlotIndex - 1];
	const FCodeBItemInstance* Source = Binding.bHasReference
		? Current.RepositorySnapshot.Items.Find(Binding.ItemId) : nullptr;
	const FCodeBItemDefinition* Definition = Source
		? Current.RepositorySnapshot.Definitions.Find(Source->DefinitionId) : nullptr;
	const FCodeBContainer* BaseQuick = Current.RepositorySnapshot.Containers.Find(Current.Layout.BasicContainerId);
	if (!Source || !Definition || !BaseQuick || Source->Quantity < 1 || Source->ChildContainerId.IsValid()
		|| Source->ParentContainerId != Current.Layout.BasicContainerId
		|| !BaseQuick->Slots.IsValidIndex(Source->SlotIndex)
		|| BaseQuick->Slots[Source->SlotIndex] != Source->ItemId
		|| !Definition->bQuickUsable
		|| Definition->QuickUseEffect != ECodeBQuickUseEffectKind::RestoreHealth
		|| Definition->QuickUseRestoreAmount <= 0)
	{
		if (OutError) *OutError = TEXT("Code B P15 slot is not a current simple BaseQuick RestoreHealth consumable.");
		return false;
	}

	FCodeBOutOfRaidInventoryRecord Candidate = Record;
	FCodeBRunInventorySession& Session = Candidate.ActiveRunInventorySession;
	FCodeBItemInstance* CandidateItem = Session.RepositorySnapshot.Items.Find(Source->ItemId);
	FCodeBContainer* CandidateContainer = Session.RepositorySnapshot.Containers.Find(Source->ParentContainerId);
	if (!CandidateItem || !CandidateContainer || !CandidateContainer->Slots.IsValidIndex(CandidateItem->SlotIndex))
	{
		if (OutError) *OutError = TEXT("Code B P15 candidate source graph became invalid.");
		return false;
	}
	if (--CandidateItem->Quantity == 0)
	{
		CandidateContainer->Slots[CandidateItem->SlotIndex].Invalidate();
		Session.RepositorySnapshot.Items.Remove(CandidateItem->ItemId);
	}
	++Session.RepositorySnapshot.Revision;
	if (!ReconcileHotbarBindings(Session.HotbarBindings, Session.RepositorySnapshot, Session.Layout, Error))
	{
		if (OutError) *OutError = Error;
		return false;
	}
	FCodeBQuickUseReceipt& Receipt = Session.QuickUseReceipts.AddDefaulted_GetRef();
	Receipt.ReceiptOrdinal = Session.NextQuickUseReceiptOrdinal;
	Receipt.ReceiptId = QuickUseReceiptGuid(Session.OwnerId, Session.RunInstanceId, Receipt.ReceiptOrdinal);
	Receipt.SlotIndex = SlotIndex;
	Receipt.SourceItemId = Source->ItemId;
	Receipt.EffectKind = Definition->QuickUseEffect;
	Receipt.RestoreAmount = Definition->QuickUseRestoreAmount;
	Receipt.State = ECodeBQuickUseReceiptState::Pending;
	++Session.NextQuickUseReceiptOrdinal;
	FCodeBRepository Validation;
	if (!Validation.LoadPersistedSnapshot(Session.RepositorySnapshot, &Error)
		|| !ValidateRunInventorySession(Session, Error))
	{
		if (OutError) *OutError = Error;
		return false;
	}
	const FString CommitUtc = UtcNow();
	Session.LastCommittedUtc = CommitUtc;
	++Session.SessionRevision;
	Candidate.LastCommittedUtc = CommitUtc;
	++Candidate.PersistentRevision;
	if (!SaveRecord(Candidate, Error))
	{
		if (OutError) *OutError = Error;
		return false;
	}
	OutReceipt = Receipt;
	Record = MoveTemp(Candidate);
	return true;
}

bool FCodeBOutOfRaidProfileStore::TryGetMatchedActiveRunPendingQuickUseReceipts(
	const FGuid& ExpectedRunInstanceId,
	TArray<FCodeBQuickUseReceipt>& OutReceipts,
	FString* OutError)
{
	OutReceipts.Reset();
	FCodeBRunInventorySession Session;
	if (!OpenMatchedActiveRunInventorySession(ExpectedRunInstanceId, Session, OutError)) return false;
	for (const FCodeBQuickUseReceipt& Receipt : Session.QuickUseReceipts)
	{
		if (Receipt.State == ECodeBQuickUseReceiptState::Pending) OutReceipts.Add(Receipt);
	}
	return true;
}

bool FCodeBOutOfRaidProfileStore::AcknowledgeMatchedActiveRunQuickUseReceipt(
	const FGuid& ExpectedRunInstanceId,
	const FGuid& ReceiptId,
	FString* OutError)
{
	if (OutError) OutError->Reset();
	FCodeBRunInventorySession Current;
	FString Error;
	if (!ReceiptId.IsValid() || !OpenMatchedActiveRunInventorySession(ExpectedRunInstanceId, Current, &Error))
	{
		if (OutError) *OutError = Error.IsEmpty() ? TEXT("Code B P15 acknowledgement identity is invalid.") : Error;
		return false;
	}
	if (Record.PersistentRevision == MAX_int32 || Current.SessionRevision == MAX_int32)
	{
		if (OutError) *OutError = TEXT("Code B P15 acknowledgement revision is exhausted.");
		return false;
	}
	FCodeBOutOfRaidInventoryRecord Candidate = Record;
	FCodeBRunInventorySession& Session = Candidate.ActiveRunInventorySession;
	FCodeBQuickUseReceipt* Receipt = Session.QuickUseReceipts.FindByPredicate([ReceiptId](const FCodeBQuickUseReceipt& Entry)
	{
		return Entry.ReceiptId == ReceiptId;
	});
	if (!Receipt)
	{
		if (OutError) *OutError = TEXT("Code B P15 acknowledgement receipt does not exist in this session.");
		return false;
	}
	if (Receipt->State == ECodeBQuickUseReceiptState::Acknowledged) return true;
	Receipt->State = ECodeBQuickUseReceiptState::Acknowledged;
	if (!ValidateRunInventorySession(Session, Error))
	{
		if (OutError) *OutError = Error;
		return false;
	}
	const FString CommitUtc = UtcNow();
	Session.LastCommittedUtc = CommitUtc;
	++Session.SessionRevision;
	Candidate.LastCommittedUtc = CommitUtc;
	++Candidate.PersistentRevision;
	if (!SaveRecord(Candidate, Error))
	{
		if (OutError) *OutError = Error;
		return false;
	}
	Record = MoveTemp(Candidate);
	return true;
}

bool FCodeBOutOfRaidProfileStore::TryGetMatchedActiveRunHotbarProjection(
	const FGuid& ExpectedRunInstanceId,
	FCodeBHotbarProjection& OutProjection,
	FString* OutError) const
{
	OutProjection = FCodeBHotbarProjection();
	if (OutError) OutError->Reset();
	if (!bHasRecord || Record.OwnerId != OwnerId || !ExpectedRunInstanceId.IsValid()
		|| !Record.bHasActiveRunInventorySession)
	{
		if (OutError) *OutError = TEXT("Code B P13 hotbar has no loaded exact active Run session.");
		return false;
	}
	const FCodeBRunInventorySession& Session = Record.ActiveRunInventorySession;
	if (Session.OwnerId != OwnerId || Session.RunInstanceId != ExpectedRunInstanceId
		|| Session.BridgeState != ECodeBRunInventoryBridgeState::Committed)
	{
		if (OutError) *OutError = TEXT("Code B P13 hotbar refused a non-committed or Run-mismatched P6 session.");
		return false;
	}
	FString Error;
	if (!BuildHotbarProjection(Session.HotbarBindings, Session.RepositorySnapshot,
		true, true, Session.SessionRevision, OutProjection, Error))
	{
		if (OutError) *OutError = Error;
		return false;
	}
	return true;
}

bool FCodeBOutOfRaidProfileStore::BindMatchedActiveRunHotbarSlot(
	const FGuid& ExpectedRunInstanceId,
	const FGuid& ItemId,
	const int32 SlotIndex,
	FCodeBHotbarProjection& OutProjection,
	FString* OutError)
{
	OutProjection = FCodeBHotbarProjection();
	if (OutError) OutError->Reset();
	if (!bHasRecord || Record.OwnerId != OwnerId || !ExpectedRunInstanceId.IsValid()
		|| !Record.bHasActiveRunInventorySession || Record.PersistentRevision == MAX_int32)
	{
		if (OutError) *OutError = TEXT("Code B P13 has no writable Owner-matched P6 session.");
		return false;
	}
	FCodeBOutOfRaidInventoryRecord Candidate = Record;
	FCodeBRunInventorySession& Session = Candidate.ActiveRunInventorySession;
	if (Session.OwnerId != OwnerId || Session.RunInstanceId != ExpectedRunInstanceId
		|| Session.BridgeState != ECodeBRunInventoryBridgeState::Committed
		|| Session.SessionRevision == MAX_int32)
	{
		if (OutError) *OutError = TEXT("Code B P13 refused a stale, Prepared, terminal, or Run-mismatched P6 hotbar write.");
		return false;
	}
	FString Error;
	if (!ApplyHotbarBind(Session.HotbarBindings, Session.RepositorySnapshot, Session.Layout, ItemId, SlotIndex, Error))
	{
		if (OutError) *OutError = Error;
		return false;
	}
	const FString CommitUtc = UtcNow();
	++Session.SessionRevision;
	Session.LastCommittedUtc = CommitUtc;
	++Candidate.PersistentRevision;
	Candidate.LastCommittedUtc = CommitUtc;
	if (!BuildHotbarProjection(Session.HotbarBindings, Session.RepositorySnapshot,
			true, true, Session.SessionRevision, OutProjection, Error)
		|| !SaveRecord(Candidate, Error))
	{
		if (OutError) *OutError = Error;
		return false;
	}
	Record = MoveTemp(Candidate);
	return true;
}

bool FCodeBOutOfRaidProfileStore::UnbindMatchedActiveRunHotbarSlot(
	const FGuid& ExpectedRunInstanceId,
	const int32 SlotIndex,
	FCodeBHotbarProjection& OutProjection,
	FString* OutError)
{
	OutProjection = FCodeBHotbarProjection();
	if (OutError) OutError->Reset();
	if (!bHasRecord || Record.OwnerId != OwnerId || !ExpectedRunInstanceId.IsValid()
		|| !Record.bHasActiveRunInventorySession || Record.PersistentRevision == MAX_int32)
	{
		if (OutError) *OutError = TEXT("Code B P13 has no writable Owner-matched P6 session.");
		return false;
	}
	FCodeBOutOfRaidInventoryRecord Candidate = Record;
	FCodeBRunInventorySession& Session = Candidate.ActiveRunInventorySession;
	if (Session.OwnerId != OwnerId || Session.RunInstanceId != ExpectedRunInstanceId
		|| Session.BridgeState != ECodeBRunInventoryBridgeState::Committed
		|| Session.SessionRevision == MAX_int32)
	{
		if (OutError) *OutError = TEXT("Code B P13 refused a stale, Prepared, terminal, or Run-mismatched P6 hotbar write.");
		return false;
	}
	FString Error;
	if (!ApplyHotbarUnbind(Session.HotbarBindings, SlotIndex, Error))
	{
		if (OutError) *OutError = Error;
		return false;
	}
	const FString CommitUtc = UtcNow();
	++Session.SessionRevision;
	Session.LastCommittedUtc = CommitUtc;
	++Candidate.PersistentRevision;
	Candidate.LastCommittedUtc = CommitUtc;
	if (!BuildHotbarProjection(Session.HotbarBindings, Session.RepositorySnapshot,
			true, true, Session.SessionRevision, OutProjection, Error)
		|| !SaveRecord(Candidate, Error))
	{
		if (OutError) *OutError = Error;
		return false;
	}
	Record = MoveTemp(Candidate);
	return true;
}

bool FCodeBOutOfRaidProfileStore::TryGetMatchedActiveRunWorldDropProjections(
	const FGuid& ExpectedRunInstanceId,
	TArray<FCodeBWorldDropProjection>& OutProjections,
	FString* OutError) const
{
	OutProjections.Reset();
	if (OutError) OutError->Reset();
	if (!bHasRecord || Record.OwnerId != OwnerId || !ExpectedRunInstanceId.IsValid()
		|| !Record.bHasActiveRunInventorySession)
	{
		if (OutError) *OutError = TEXT("Code B P14 has no loaded Owner-matched P6 session.");
		return false;
	}
	const FCodeBRunInventorySession& Session = Record.ActiveRunInventorySession;
	FString Error;
	if (Session.OwnerId != OwnerId || Session.RunInstanceId != ExpectedRunInstanceId
		|| Session.BridgeState != ECodeBRunInventoryBridgeState::Committed
		|| !ValidateWorldDrops(Session, Error))
	{
		if (OutError) *OutError = Error.IsEmpty()
			? TEXT("Code B P14 refused a stale or non-committed P6 session.") : Error;
		return false;
	}
	for (const FCodeBWorldDropRecord& RecordValue : Session.WorldDrops)
	{
		OutProjections.Add(WorldDropProjection(Session, RecordValue));
	}
	return true;
}

bool FCodeBOutOfRaidProfileStore::DropMatchedActiveRunWorldDropItem(
	const FGuid& ExpectedRunInstanceId,
	const FGuid& ItemId,
	const FGuid& ExpectedSourceContainerId,
	const int32 ExpectedSourceSlot,
	const int32 ExpectedP6SnapshotRevision,
	const int32 RequestedSplitQuantity,
	const FName MapRoute,
	const FTransform& FloorTransform,
	FCodeBWorldDropProjection& OutProjection,
	FString* OutError)
{
	OutProjection = FCodeBWorldDropProjection();
	if (OutError) OutError->Reset();
	FCodeBRunInventorySession Current;
	FString Error;
	if (!ExpectedRunInstanceId.IsValid() || !ItemId.IsValid() || !ExpectedSourceContainerId.IsValid()
		|| ExpectedSourceSlot < 0 || ExpectedP6SnapshotRevision < 1 || RequestedSplitQuantity < 0
		|| MapRoute.IsNone()
		|| !IsFiniteWorldDropTransform(FloorTransform)
		|| !OpenMatchedActiveRunInventorySession(ExpectedRunInstanceId, Current, &Error))
	{
		if (OutError) *OutError = Error.IsEmpty()
			? TEXT("Code B P14 drop requires a legal map transform and exact committed P6 session.") : Error;
		return false;
	}
	if (Current.RepositorySnapshot.Revision != ExpectedP6SnapshotRevision
		|| Record.PersistentRevision == MAX_int32 || Current.SessionRevision == MAX_int32
		|| Current.NextWorldDropOrdinal == MAX_int32)
	{
		if (OutError) *OutError = TEXT("Code B P26 drop refused a stale graph or exhausted durable session/ordinal.");
		return false;
	}
	const FCodeBItemInstance* Item = Current.RepositorySnapshot.Items.Find(ItemId);
	const FCodeBContainer* Source = Current.RepositorySnapshot.Containers.Find(ExpectedSourceContainerId);
	bool bIsSpatialClosure = false;
	if (!Item || !Source || Item->Quantity <= 0
		|| Item->ParentContainerId != ExpectedSourceContainerId
		|| Item->SlotIndex != ExpectedSourceSlot
		|| !Source->Slots.IsValidIndex(Item->SlotIndex) || Source->Slots[Item->SlotIndex] != ItemId
		|| !ValidateP19WorldDropClosure(Current.RepositorySnapshot, *Item, bIsSpatialClosure, Error))
	{
		if (OutError) *OutError = Error.IsEmpty()
			? TEXT("Code B P14 requires the exact P7 source cell to contain a valid whole item graph.") : Error;
		return false;
	}
	const bool bFromBasic = ExpectedSourceContainerId == Current.Layout.BasicContainerId;
	const bool bFromMatchingSpatialEquipment = bIsSpatialClosure
		&& Item->DefinitionId == Fdemo_mapItemIds::WindTalisman
		&& ExpectedSourceContainerId == Current.Layout.SpatialContainerId
		&& Source->IsEquipment();
	const bool bFromMatchingBackpackEquipment = bIsSpatialClosure
		&& Item->DefinitionId == Fdemo_mapItemIds::BackpackLevel1
		&& ExpectedSourceContainerId == Current.Layout.BackpackContainerId
		&& Source->IsEquipment();
	const bool bFormalSpatialDefinition = Item->DefinitionId == Fdemo_mapItemIds::WindTalisman
		|| Item->DefinitionId == Fdemo_mapItemIds::BackpackLevel1;
	const bool bP26SimpleStackSource = !bIsSpatialClosure
		&& IsP26SimpleStack(Current, *Item)
		&& IsP26PlayerStorageContainer(Current, ExpectedSourceContainerId);
	const bool bP32EquippedStandardSource = !bIsSpatialClosure
		&& IsP32EquippedStandardRoot(Current, *Item, ExpectedSourceContainerId);
	const bool bP33BaseQuickStandardSource = !bIsSpatialClosure && bFromBasic
		&& !Source->IsEquipment() && IsP32StandardEquipmentRoot(Current, *Item);
	const bool bPartialDrop = RequestedSplitQuantity > 0;
	if ((bPartialDrop && (!bP26SimpleStackSource || RequestedSplitQuantity >= Item->Quantity))
			|| (!bPartialDrop && ((bFormalSpatialDefinition && !bIsSpatialClosure)
			|| (!bIsSpatialClosure && !bFromBasic && !bP26SimpleStackSource && !bP32EquippedStandardSource)
			|| (bIsSpatialClosure && !bFromBasic && !bFromMatchingSpatialEquipment && !bFromMatchingBackpackEquipment))))
	{
		if (OutError)
		{
			*OutError = bPartialDrop
				? TEXT("Code B P26 partial drop requires one exact simple stack in BaseQuick or an equipped current child, leaving a non-empty source.")
				: ((bIsSpatialClosure || bFormalSpatialDefinition)
					? TEXT("Code B P19 only drops a formal spatial parent from BaseQuick or its matching equipment slot.")
					: TEXT("Code B P14/P26 only drops a legal whole simple item from BaseQuick or a P26 simple stack from an equipped child."));
		}
		return false;
	}
	const FGuid WorldDropId = WorldDropGuid(OwnerId, ExpectedRunInstanceId, Current.NextWorldDropOrdinal);
	const FGuid WorldContainerId = WorldDropContainerGuid(WorldDropId);
	if (!WorldDropId.IsValid() || !WorldContainerId.IsValid()
		|| Current.RepositorySnapshot.Containers.Contains(WorldContainerId)
		|| Current.WorldDrops.ContainsByPredicate([WorldDropId](const FCodeBWorldDropRecord& Value)
			{ return Value.WorldDropId == WorldDropId; }))
	{
		if (OutError) *OutError = TEXT("Code B P14 refused a duplicate deterministic world-drop identity.");
		return false;
	}
	FCodeBRepository Repository;
	if (!Repository.LoadPersistedSnapshot(Current.RepositorySnapshot, &Error)
		|| !Repository.CreateContainer(FName(TEXT("WorldDrop")), 1, ECodeBContainerKind::Storage,
			ECodeBEquipSlot::None, &Error, WorldContainerId).IsValid())
	{
		if (OutError) *OutError = Error;
		return false;
	}
	FCodeBTransactionRequest Move;
	Move.TransactionId = FGuid::NewGuid();
	Move.Operation = bPartialDrop
		? ECodeBOperation::Split
		: (Source->IsEquipment() ? ECodeBOperation::Unequip : ECodeBOperation::Move);
	Move.ItemId = ItemId;
	Move.SourceContainerId = ExpectedSourceContainerId;
	Move.SourceSlot = Item->SlotIndex;
	Move.TargetContainerId = WorldContainerId;
	Move.TargetSlot = 0;
	Move.Quantity = bPartialDrop ? RequestedSplitQuantity : 0;
	Move.ExpectedRevision = Repository.GetRevision();
	const FCodeBTransactionResult Transaction = Repository.ExecuteTransaction(Move);
	if (!Transaction.IsSuccess())
	{
		if (OutError) *OutError = bPartialDrop
			? TEXT("Code B P26 P1 exact split-to-ground transaction was rejected.")
			: bIsSpatialClosure
			? TEXT("Code B P19 P1 whole-graph-to-ground transaction was rejected.")
			: TEXT("Code B P14 P1 BaseQuick-to-ground move was rejected.");
		return false;
	}
	const FCodeBSnapshot AcceptedSnapshot = Repository.CaptureSnapshot();
	const FCodeBContainer* AcceptedWorldContainer = AcceptedSnapshot.Containers.Find(WorldContainerId);
	const FGuid AcceptedWorldItemId = AcceptedWorldContainer && AcceptedWorldContainer->Slots.Num() == 1
		? AcceptedWorldContainer->Slots[0]
		: FGuid();
	if (!AcceptedWorldItemId.IsValid()
		|| (bPartialDrop && (AcceptedWorldItemId != Transaction.CreatedItemId || AcceptedWorldItemId == ItemId))
		|| (!bPartialDrop && AcceptedWorldItemId != ItemId))
	{
		if (OutError) *OutError = TEXT("Code B P26 accepted P1 result did not expose the exact single world root identity.");
		return false;
	}
	for (const FCodeBWorldDropRecord& ExistingDrop : Current.WorldDrops)
	{
		if (!IsWorldDropClosureUnchanged(Current.RepositorySnapshot, AcceptedSnapshot, ExistingDrop))
		{
			if (OutError) *OutError = TEXT("Code B P31 refused a create candidate that changed an existing WorldDrop record graph.");
			return false;
		}
	}
	const FCodeBItemInstance* AcceptedWorldRoot = AcceptedSnapshot.Items.Find(AcceptedWorldItemId);
	FCodeBItemInstance ExpectedStandardWorldRoot;
	const bool bP32OrP33StandardSource = bP32EquippedStandardSource || bP33BaseQuickStandardSource;
	if (bP32OrP33StandardSource)
	{
		ExpectedStandardWorldRoot = *Item;
		ExpectedStandardWorldRoot.ParentContainerId = WorldContainerId;
		ExpectedStandardWorldRoot.SlotIndex = 0;
	}
	if (!AcceptedWorldRoot
		|| (bP32OrP33StandardSource && !(*AcceptedWorldRoot == ExpectedStandardWorldRoot)))
	{
		if (OutError) *OutError = bP32OrP33StandardSource
			? TEXT("Code B P32/P33 accepted standard-equipment relocation changed more than the root placement.")
			: TEXT("Code B P31 accepted create candidate lost its exact world root.");
		return false;
	}
	FCodeBOutOfRaidInventoryRecord Candidate = Record;
	FCodeBRunInventorySession& Session = Candidate.ActiveRunInventorySession;
	Session.RepositorySnapshot = AcceptedSnapshot;
	FCodeBWorldDropRecord& RecordValue = Session.WorldDrops.AddDefaulted_GetRef();
	RecordValue.OwnerId = Session.OwnerId;
	RecordValue.RunInstanceId = Session.RunInstanceId;
	RecordValue.WorldDropId = WorldDropId;
	RecordValue.Ordinal = Session.NextWorldDropOrdinal;
	RecordValue.WorldContainerId = WorldContainerId;
	RecordValue.ItemId = AcceptedWorldItemId;
	RecordValue.SpatialChildContainerId = AcceptedWorldRoot->ChildContainerId;
	RecordValue.MapRoute = MapRoute;
	RecordValue.FloorTransform = FloorTransform;
	RecordValue.ActionState = ECodeBWorldDropActionState::Available;
	RecordValue.RecordRevision = 1;
	RecordValue.Provenance = bPartialDrop
		? TEXT("P31.AcceptedGroundDrop.Split")
		: (bIsSpatialClosure
			? TEXT("P31.AcceptedGroundDrop.CompleteGraph")
			: (bP32EquippedStandardSource
				? TEXT("P32.AcceptedGroundDrop.StandardEquipment")
				: (bP33BaseQuickStandardSource
					? TEXT("P33.AcceptedGroundDrop.BaseQuickStandardEquipment")
					: TEXT("P31.AcceptedGroundDrop.WholeRoot"))));
	++Session.NextWorldDropOrdinal;
	if (!ReconcileHotbarBindings(Session.HotbarBindings, Session.RepositorySnapshot, Session.Layout, Error)
		|| !FreezeRunInventoryPayloadReceipt(Session, Error)
		|| !ValidateRunInventorySession(Session, Error))
	{
		if (OutError) *OutError = Error;
		return false;
	}
	const FString CommitUtc = UtcNow();
	Session.LastCommittedUtc = CommitUtc;
	++Session.SessionRevision;
	Candidate.LastCommittedUtc = CommitUtc;
	++Candidate.PersistentRevision;
	const FCodeBWorldDropProjection CommittedProjection = WorldDropProjection(Session, RecordValue);
	if (!SaveRecord(Candidate, Error))
	{
		if (OutError) *OutError = Error;
		return false;
	}
	Record = MoveTemp(Candidate);
	OutProjection = CommittedProjection;
	return true;
}

bool FCodeBOutOfRaidProfileStore::CommitAcceptedMatchedRunWorldDropPickup(
	const FString& InStorageRoot,
	const FGuid& InOwnerId,
	const FGuid& InRunInstanceId,
	const FGuid& WorldDropId,
	const int32 ExpectedWorldDropOrdinal,
	const int32 ExpectedWorldDropRecordRevision,
	const int32 ExpectedP6SnapshotRevision,
	const FCodeBP2Command& AcceptedCommand,
	const FCodeBSnapshot& CandidateSnapshot,
	FString* OutError)
{
	if (OutError) OutError->Reset();
	if (!InOwnerId.IsValid() || !InRunInstanceId.IsValid() || !WorldDropId.IsValid()
		|| ExpectedWorldDropOrdinal < 1 || ExpectedWorldDropRecordRevision < 1
		|| ExpectedP6SnapshotRevision < 1)
	{
		if (OutError) *OutError = TEXT("Code B P14 pickup requires exact Owner/Run/drop and P6 revision.");
		return false;
	}
	FCodeBOutOfRaidProfileStore Store(InStorageRoot, InOwnerId);
	FCodeBRunInventorySession Existing;
	FString Error;
	if (!Store.OpenMatchedActiveRunInventorySession(InRunInstanceId, Existing, &Error))
	{
		if (OutError) *OutError = Error;
		return false;
	}
	const FCodeBWorldDropRecord* Drop = Existing.WorldDrops.FindByPredicate(
		[WorldDropId](const FCodeBWorldDropRecord& Value) { return Value.WorldDropId == WorldDropId; });
	if (!Drop || Drop->OwnerId != InOwnerId || Drop->RunInstanceId != InRunInstanceId
		|| Drop->Ordinal != ExpectedWorldDropOrdinal
		|| Drop->RecordRevision != ExpectedWorldDropRecordRevision
		|| Existing.RepositorySnapshot.Revision != ExpectedP6SnapshotRevision
		|| Store.Record.PersistentRevision == MAX_int32 || Existing.SessionRevision == MAX_int32)
	{
		if (OutError) *OutError = TEXT("Code B P14 pickup refused a stale, missing, or terminal ground drop.");
		return false;
	}
	const FCodeBItemInstance* SourceItem = Existing.RepositorySnapshot.Items.Find(Drop->ItemId);
	bool bIsSpatialClosure = false;
	if (!SourceItem || SourceItem->ParentContainerId != Drop->WorldContainerId || SourceItem->SlotIndex != 0
		|| Drop->SpatialChildContainerId != SourceItem->ChildContainerId
		|| !ValidateP19WorldDropClosure(Existing.RepositorySnapshot, *SourceItem, bIsSpatialClosure, Error))
	{
		if (OutError) *OutError = Error.IsEmpty()
			? TEXT("Code B P14 pickup found an invalid stored ground item.") : Error;
		return false;
	}
	const FCodeBItemInstance* CandidateItem = CandidateSnapshot.Items.Find(Drop->ItemId);
	const bool bP26SimpleStack = !bIsSpatialClosure && IsP26SimpleStack(Existing, *SourceItem);
	const bool bP32StandardEquipmentRoot = !bIsSpatialClosure
		&& IsP32StandardEquipmentRoot(Existing, *SourceItem);
	const bool bQuickTransfer = AcceptedCommand.Intent == ECodeBP2CommandIntent::QuickTransfer;
	bool bP29RetainWorldRoot = false;
	bool bP30CompleteGraphQuickTransfer = false;
	bool bP34StandardEquipmentQuickTransfer = false;
	const bool bP28MergeIntent = AcceptedCommand.Operation == ECodeBOperation::Merge
		&& AcceptedCommand.Quantity > 0 && !bQuickTransfer;
	const FCodeBItemInstance* P27CreatedItem = nullptr;
	bool bMultipleP27CreatedItems = false;
	for (const TPair<FGuid, FCodeBItemInstance>& Pair : CandidateSnapshot.Items)
	{
		if (Existing.RepositorySnapshot.Items.Contains(Pair.Key)) continue;
		if (P27CreatedItem)
		{
			bMultipleP27CreatedItems = true;
			break;
		}
		P27CreatedItem = &Pair.Value;
	}
	const bool bP27Split = bP26SimpleStack && !bMultipleP27CreatedItems && P27CreatedItem
		&& CandidateItem && CandidateItem->ParentContainerId == Drop->WorldContainerId
		&& CandidateItem->SlotIndex == 0 && CandidateItem->ItemId == SourceItem->ItemId
		&& CandidateItem->Quantity > 0 && CandidateItem->Quantity < SourceItem->Quantity;
	FCodeBTransactionRequest Move;
	Move.TransactionId = FGuid::NewGuid();
	Move.ItemId = Drop->ItemId;
	Move.SourceContainerId = Drop->WorldContainerId;
	Move.SourceSlot = 0;
	Move.ExpectedRevision = Existing.RepositorySnapshot.Revision;
	bool bP26Merge = false;
	bool bP28Merge = false;
	if (bQuickTransfer)
	{
		if (bIsSpatialClosure)
		{
			if (!IsExactP30WorldDropCompleteGraphQuickTransferDelta(
				Existing, *Drop, AcceptedCommand, CandidateSnapshot, Error))
			{
				if (OutError) *OutError = Error.IsEmpty()
					? TEXT("Code B P30 Ctrl quick transfer requires one exact complete-graph Move to first-empty BaseQuick.")
					: Error;
				return false;
			}
			bP30CompleteGraphQuickTransfer = true;
		}
		else if (bP32StandardEquipmentRoot)
		{
			if (!IsExactP34WorldDropStandardEquipmentQuickTransferDelta(
				Existing, *Drop, AcceptedCommand, CandidateSnapshot, Error))
			{
				if (OutError) *OutError = Error.IsEmpty()
					? TEXT("Code B P34 Ctrl quick transfer requires one exact standard-equipment Move(1) to first-empty BaseQuick.")
					: Error;
				return false;
			}
			bP34StandardEquipmentQuickTransfer = true;
		}
		else if (!IsExactP29WorldDropQuickTransferDelta(
			Existing, *Drop, AcceptedCommand, CandidateSnapshot, bP29RetainWorldRoot, Error))
		{
			if (OutError) *OutError = Error.IsEmpty()
				? TEXT("Code B P29 Ctrl quick transfer requires one exact simple-stack Move/Merge(0) delta.")
				: Error;
			return false;
		}
		Move.Operation = AcceptedCommand.Operation;
		Move.ItemId = AcceptedCommand.ItemId;
		Move.SourceContainerId = AcceptedCommand.SourceContainerId;
		Move.SourceSlot = AcceptedCommand.SourceSlot;
		Move.TargetContainerId = AcceptedCommand.TargetContainerId;
		Move.TargetSlot = AcceptedCommand.TargetSlot;
		Move.Quantity = AcceptedCommand.Quantity;
	}
	else if (bP28MergeIntent)
	{
		const FCodeBContainer* P28TargetContainer = Existing.RepositorySnapshot.Containers.Find(
			AcceptedCommand.TargetContainerId);
		if (!bP26SimpleStack
			|| !IsP26PlayerStorageContainer(Existing, AcceptedCommand.TargetContainerId)
			|| !P28TargetContainer || P28TargetContainer->IsEquipment()
			|| !IsExactP28WorldPickupMergeDelta(
				Existing.RepositorySnapshot, CandidateSnapshot, AcceptedCommand, Drop->ItemId, Error))
		{
			if (OutError) *OutError = Error.IsEmpty()
				? TEXT("Code B P28 pickup requires one exact P1 Merge(N) into an explicit compatible occupied player stack.")
				: Error;
			return false;
		}
		Move.Operation = ECodeBOperation::Merge;
		Move.TargetContainerId = AcceptedCommand.TargetContainerId;
		Move.TargetSlot = AcceptedCommand.TargetSlot;
		Move.Quantity = AcceptedCommand.Quantity;
		bP28Merge = true;
	}
	else if (bIsSpatialClosure)
	{
		if (!CandidateItem)
		{
			if (OutError) *OutError = TEXT("Code B P19 pickup candidate is missing the spatial WorldDrop root.");
			return false;
		}
		const FCodeBContainer* CandidateBasic = CandidateSnapshot.Containers.Find(Existing.Layout.BasicContainerId);
		if (CandidateBasic && CandidateItem->ParentContainerId == Existing.Layout.BasicContainerId
			&& CandidateBasic->Slots.IsValidIndex(CandidateItem->SlotIndex)
			&& CandidateBasic->Slots[CandidateItem->SlotIndex] == Drop->ItemId)
		{
			Move.Operation = ECodeBOperation::Move;
			Move.TargetContainerId = Existing.Layout.BasicContainerId;
			Move.TargetSlot = CandidateItem->SlotIndex;
		}
		else
		{
			const bool bWindTalisman = SourceItem->DefinitionId == Fdemo_mapItemIds::WindTalisman;
			const bool bBackpackLevel1 = SourceItem->DefinitionId == Fdemo_mapItemIds::BackpackLevel1;
			const FGuid ExpectedEquipmentContainerId = bWindTalisman
				? Existing.Layout.SpatialContainerId
				: (bBackpackLevel1 ? Existing.Layout.BackpackContainerId : FGuid());
			const FCodeBContainer* ExistingEquipment = Existing.RepositorySnapshot.Containers.Find(ExpectedEquipmentContainerId);
			const FCodeBContainer* CandidateEquipment = CandidateSnapshot.Containers.Find(ExpectedEquipmentContainerId);
			if (!ExpectedEquipmentContainerId.IsValid() || !ExistingEquipment || !CandidateEquipment
				|| !ExistingEquipment->IsEquipment() || ExistingEquipment->Slots.Num() != 1
				|| ExistingEquipment->Slots[0].IsValid()
				|| CandidateItem->ParentContainerId != ExpectedEquipmentContainerId || CandidateItem->SlotIndex != 0
				|| CandidateEquipment->Slots.Num() != 1 || CandidateEquipment->Slots[0] != Drop->ItemId)
			{
				if (OutError) *OutError = TEXT("Code B P19 pickup only accepts the formal spatial root dragged to an empty matching equipment slot.");
				return false;
			}
			Move.Operation = ECodeBOperation::Equip;
			Move.TargetContainerId = ExpectedEquipmentContainerId;
			Move.TargetSlot = 0;
		}
	}
	else if (bP32StandardEquipmentRoot)
	{
		const FCodeBItemDefinition* Definition = Existing.RepositorySnapshot.Definitions.Find(SourceItem->DefinitionId);
		const FCodeBContainer* ExistingTarget = Existing.RepositorySnapshot.Containers.Find(
			AcceptedCommand.TargetContainerId);
		const FCodeBContainer* CandidateTarget = CandidateSnapshot.Containers.Find(
			AcceptedCommand.TargetContainerId);
		const bool bBaseQuickTarget = AcceptedCommand.TargetContainerId == Existing.Layout.BasicContainerId
			&& AcceptedCommand.Operation == ECodeBOperation::Move;
		const bool bEquipmentTarget = Definition
			&& AcceptedCommand.Operation == ECodeBOperation::Equip
			&& AcceptedCommand.TargetSlot == 0
			&& IsP32ActiveStandardEquipmentContainer(
				Existing, AcceptedCommand.TargetContainerId, Definition->EquipSlot);
		if (!CandidateItem || !Definition || !ExistingTarget || !CandidateTarget
			|| AcceptedCommand.Intent != ECodeBP2CommandIntent::Standard
			|| !AcceptedCommand.TransactionId.IsValid()
			|| AcceptedCommand.ItemId != Drop->ItemId
			|| AcceptedCommand.SourceContainerId != Drop->WorldContainerId
			|| AcceptedCommand.SourceSlot != 0
			|| AcceptedCommand.Quantity != 0
			|| AcceptedCommand.ExpectedRevision != Existing.RepositorySnapshot.Revision
			|| (!bBaseQuickTarget && !bEquipmentTarget)
			|| !ExistingTarget->Slots.IsValidIndex(AcceptedCommand.TargetSlot)
			|| ExistingTarget->Slots[AcceptedCommand.TargetSlot].IsValid()
			|| !CandidateTarget->Slots.IsValidIndex(AcceptedCommand.TargetSlot)
			|| CandidateTarget->Slots[AcceptedCommand.TargetSlot] != Drop->ItemId
			|| CandidateItem->ParentContainerId != AcceptedCommand.TargetContainerId
			|| CandidateItem->SlotIndex != AcceptedCommand.TargetSlot)
		{
			if (OutError) *OutError = TEXT("Code B P32 pickup requires one exact normal Drag P1 Move/Equip into the explicit empty BaseQuick or compatible active equipment cell; replacement is forbidden.");
			return false;
		}
		Move.TransactionId = AcceptedCommand.TransactionId;
		Move.Operation = AcceptedCommand.Operation;
		Move.TargetContainerId = AcceptedCommand.TargetContainerId;
		Move.TargetSlot = AcceptedCommand.TargetSlot;
	}
	else if (bP27Split)
	{
		const FCodeBContainer* OriginalTarget = Existing.RepositorySnapshot.Containers.Find(
			P27CreatedItem->ParentContainerId);
		if (!IsExactP24SplitDelta(Existing.RepositorySnapshot, CandidateSnapshot, Error)
			|| !IsP26PlayerStorageContainer(Existing, P27CreatedItem->ParentContainerId)
			|| !OriginalTarget || OriginalTarget->IsEquipment()
			|| !OriginalTarget->Slots.IsValidIndex(P27CreatedItem->SlotIndex)
			|| OriginalTarget->Slots[P27CreatedItem->SlotIndex].IsValid()
			|| P27CreatedItem->DefinitionId != SourceItem->DefinitionId
			|| P27CreatedItem->Quantity != SourceItem->Quantity - CandidateItem->Quantity)
		{
			if (OutError) *OutError = Error.IsEmpty()
				? TEXT("Code B P27 pickup requires one exact P1 Split into an explicit empty player storage cell.")
				: Error;
			return false;
		}
		Move.Operation = ECodeBOperation::Split;
		Move.TargetContainerId = P27CreatedItem->ParentContainerId;
		Move.TargetSlot = P27CreatedItem->SlotIndex;
		Move.Quantity = P27CreatedItem->Quantity;
	}
	else if (CandidateItem && CandidateItem->ParentContainerId != Drop->WorldContainerId)
	{
		const FCodeBContainer* CandidateTarget = CandidateSnapshot.Containers.Find(CandidateItem->ParentContainerId);
		const bool bLegalTarget = bP26SimpleStack
			? IsP26PlayerStorageContainer(Existing, CandidateItem->ParentContainerId)
			: CandidateItem->ParentContainerId == Existing.Layout.BasicContainerId;
		if (!bLegalTarget || !CandidateTarget || CandidateTarget->IsEquipment()
			|| !CandidateTarget->Slots.IsValidIndex(CandidateItem->SlotIndex)
			|| CandidateTarget->Slots[CandidateItem->SlotIndex] != Drop->ItemId)
		{
			if (OutError) *OutError = TEXT("Code B P14/P26 pickup requires one explicit legal empty player storage cell.");
			return false;
		}
		Move.Operation = ECodeBOperation::Move;
		Move.TargetContainerId = CandidateItem->ParentContainerId;
		Move.TargetSlot = CandidateItem->SlotIndex;
	}
	else
	{
		if (!bP26SimpleStack)
		{
			if (OutError) *OutError = TEXT("Code B P26 merge pickup requires a simple stackable world root.");
			return false;
		}
		const FCodeBItemInstance* OriginalTarget = nullptr;
		for (const TPair<FGuid, FCodeBItemInstance>& Pair : Existing.RepositorySnapshot.Items)
		{
			if (Pair.Key == Drop->ItemId) continue;
			const FCodeBItemInstance* Current = CandidateSnapshot.Items.Find(Pair.Key);
			if (Current && Current->Quantity > Pair.Value.Quantity)
			{
				if (OriginalTarget)
				{
					if (OutError) *OutError = TEXT("Code B P26 pickup candidate increased more than one target stack.");
					return false;
				}
				OriginalTarget = &Pair.Value;
			}
		}
		if (!OriginalTarget || !IsP26PlayerStorageContainer(Existing, OriginalTarget->ParentContainerId))
		{
			if (OutError) *OutError = TEXT("Code B P26 merge pickup has no one explicit compatible player target stack.");
			return false;
		}
		Move.Operation = ECodeBOperation::Merge;
		Move.TargetContainerId = OriginalTarget->ParentContainerId;
		Move.TargetSlot = OriginalTarget->SlotIndex;
		Move.Quantity = 0;
		bP26Merge = true;
		if (!IsExactP25MergeDelta(Existing.RepositorySnapshot, CandidateSnapshot, Error))
		{
			if (OutError) *OutError = Error;
			return false;
		}
	}
	FCodeBSnapshot ExpectedSnapshot;
	if (bP27Split || bP28Merge || bQuickTransfer)
	{
		// The candidate entered this callback only after the shared P2 service
		// accepted one P1 command. P27 cannot replay its created GUID, while P28/P29
		// deliberately avoid a second Merge or Merge(0) fallback. P30/P34 likewise prove
		// the whole-graph/whole-root Move structurally; each proof binds the accepted command to
		// the exact snapshot delta without a second durable mutation.
		ExpectedSnapshot = CandidateSnapshot;
	}
	else
	{
		FCodeBRepository ExpectedRepository;
		if (!ExpectedRepository.LoadPersistedSnapshot(Existing.RepositorySnapshot, &Error))
		{
			if (OutError) *OutError = Error;
			return false;
		}
		if (!ExpectedRepository.ExecuteTransaction(Move).IsSuccess())
		{
			if (OutError) *OutError = bIsSpatialClosure
				? TEXT("Code B P19 pickup whole-graph P1 transaction was rejected.")
				: TEXT("Code B P14 pickup P1 transaction was rejected.");
			return false;
		}
		ExpectedSnapshot = ExpectedRepository.CaptureSnapshot();
		if (CandidateSnapshot != ExpectedSnapshot)
		{
			if (OutError) *OutError = TEXT("Code B P14/P26 pickup refused a candidate differing from the exact P1 Move/Merge.");
			return false;
		}
	}
	const FCodeBItemInstance* RetainedWorldItem = ExpectedSnapshot.Items.Find(Drop->ItemId);
	const bool bRetainWorldRoot = (bQuickTransfer
		? ((bP30CompleteGraphQuickTransfer || bP34StandardEquipmentQuickTransfer)
			? false : bP29RetainWorldRoot)
		: ((bP26Merge || bP27Split || bP28Merge) && RetainedWorldItem
		&& RetainedWorldItem->ParentContainerId == Drop->WorldContainerId
		&& RetainedWorldItem->SlotIndex == 0 && RetainedWorldItem->Quantity > 0));
	if (!bRetainWorldRoot)
	{
		ExpectedSnapshot.Containers.Remove(Drop->WorldContainerId);
	}
	for (const FCodeBWorldDropRecord& OtherDrop : Existing.WorldDrops)
	{
		if (OtherDrop.WorldDropId == WorldDropId) continue;
		if (!IsWorldDropClosureUnchanged(Existing.RepositorySnapshot, ExpectedSnapshot, OtherDrop))
		{
			if (OutError) *OutError = TEXT("Code B P31 refused a pickup candidate that changed another WorldDrop record graph.");
			return false;
		}
	}
	FCodeBRepository FinalValidation;
	if (!FinalValidation.LoadPersistedSnapshot(ExpectedSnapshot, &Error))
	{
		if (OutError) *OutError = Error;
		return false;
	}
	FCodeBOutOfRaidInventoryRecord Candidate = Store.Record;
	FCodeBRunInventorySession& Session = Candidate.ActiveRunInventorySession;
	Session.RepositorySnapshot = MoveTemp(ExpectedSnapshot);
	if (bRetainWorldRoot)
	{
		const FCodeBWorldDropRecord* Retained = Session.WorldDrops.FindByPredicate(
			[WorldDropId](const FCodeBWorldDropRecord& Value) { return Value.WorldDropId == WorldDropId; });
		if (!Retained || Retained->RecordRevision != ExpectedWorldDropRecordRevision)
		{
			if (OutError) *OutError = TEXT("Code B P31 retained WorldDrop record identity revision changed unexpectedly.");
			return false;
		}
	}
	else
	{
		const int32 Removed = Session.WorldDrops.RemoveAll([WorldDropId](const FCodeBWorldDropRecord& Value)
			{ return Value.WorldDropId == WorldDropId; });
		if (Removed != 1)
		{
			if (OutError) *OutError = TEXT("Code B P31 pickup did not remove exactly one WorldDrop Registry record.");
			return false;
		}
	}
	SortWorldDropRegistry(Session.WorldDrops);
	if (!ReconcileHotbarBindings(Session.HotbarBindings, Session.RepositorySnapshot, Session.Layout, Error)
		|| !FreezeRunInventoryPayloadReceipt(Session, Error)
		|| !ValidateRunInventorySession(Session, Error))
	{
		if (OutError) *OutError = Error;
		return false;
	}
	const FString CommitUtc = UtcNow();
	Session.LastCommittedUtc = CommitUtc;
	++Session.SessionRevision;
	Candidate.LastCommittedUtc = CommitUtc;
	++Candidate.PersistentRevision;
	if (!Store.SaveRecord(Candidate, Error))
	{
		if (OutError) *OutError = Error;
		return false;
	}
	Store.Record = MoveTemp(Candidate);
	return true;
}

FCodeBRunInventoryTerminalResult FCodeBOutOfRaidProfileStore::FinalizeCommittedRunTerminal(
	const FGuid& RunInstanceId,
	const ECodeBRunInventoryTerminalState TerminalState)
{
	FCodeBRunInventoryTerminalResult Result;
	if (!bHasRecord || !OwnerId.IsValid() || !RunInstanceId.IsValid()
		|| Record.OwnerId != OwnerId)
	{
		Result.Status = ECodeBRunInventoryTerminalStatus::InvalidIdentity;
		Result.Diagnostic = TEXT("Code B P8 terminal finalizer has no Owner-matched durable record.");
		return Result;
	}
	if (TerminalState == ECodeBRunInventoryTerminalState::Unknown)
	{
		Result.Status = ECodeBRunInventoryTerminalStatus::UnknownTerminal;
		Result.Diagnostic = TEXT("Code B P8 terminal finalizer refused an unknown terminal state.");
		return Result;
	}
	for (const FCodeBRunInventoryTerminalReceipt& Existing : Record.TerminalReceipts)
	{
		if (Existing.OwnerId == OwnerId && Existing.RunInstanceId == RunInstanceId)
		{
			Result.Receipt = Existing;
			Result.Status = Existing.TerminalState == TerminalState
				? ECodeBRunInventoryTerminalStatus::AlreadyCommitted
				: ECodeBRunInventoryTerminalStatus::ConflictingTerminal;
			Result.Diagnostic = Existing.TerminalState == TerminalState
				? TEXT("Code B P8 terminal observer confirmed its existing terminal receipt.")
				: TEXT("Code B P8 terminal observer refused a conflicting terminal classification.");
			return Result;
		}
	}
	if (!Record.bHasActiveRunInventorySession)
	{
		Result.Status = ECodeBRunInventoryTerminalStatus::NoMatchedSession;
		Result.Diagnostic = TEXT("Code B P8 terminal observer found no active P6 session for this Run.");
		return Result;
	}
	const FCodeBRunInventorySession& CurrentSession = Record.ActiveRunInventorySession;
	if (CurrentSession.OwnerId != OwnerId || CurrentSession.RunInstanceId != RunInstanceId)
	{
		Result.Status = ECodeBRunInventoryTerminalStatus::NoMatchedSession;
		Result.Diagnostic = TEXT("Code B P8 terminal observer refused an Owner- or Run-mismatched P6 session.");
		return Result;
	}
	if (CurrentSession.BridgeState != ECodeBRunInventoryBridgeState::Committed)
	{
		Result.Status = ECodeBRunInventoryTerminalStatus::SessionNotCommitted;
		Result.Diagnostic = TEXT("Code B P8 terminal observer refuses a non-committed P6 session.");
		return Result;
	}
	FString Error;
	if (!ValidateRunInventorySession(CurrentSession, Error))
	{
		Result.Status = ECodeBRunInventoryTerminalStatus::StorageFailure;
		Result.Diagnostic = Error;
		return Result;
	}
	if (Record.PersistentRevision == MAX_int32)
	{
		Result.Status = ECodeBRunInventoryTerminalStatus::StorageFailure;
		Result.Diagnostic = TEXT("Code B P8 terminal finalizer cannot advance the persistent record revision.");
		return Result;
	}

	FCodeBOutOfRaidInventoryRecord Candidate = Record;
	const FCodeBRunInventorySession& FrozenSession = Candidate.ActiveRunInventorySession;
	FCodeBRunInventorySession PlayerOnlySession;
	if (!BuildP14PlayerOnlySession(FrozenSession, PlayerOnlySession, Error))
	{
		Result.Status = ECodeBRunInventoryTerminalStatus::StorageFailure;
		Result.Diagnostic = Error;
		return Result;
	}
	if (TerminalState == ECodeBRunInventoryTerminalState::Extracted
		&& !MergeCurrentRunInventoryIntoOutOfRaid(
			Candidate.RepositorySnapshot, Candidate.Layout, PlayerOnlySession, Error))
	{
		Result.Status = ECodeBRunInventoryTerminalStatus::StorageFailure;
		Result.Diagnostic = Error;
		return Result;
	}
	if (TerminalState == ECodeBRunInventoryTerminalState::Extracted)
	{
		// Only the exact P6 references whose items returned to P5 BaseQuick
		// survive the same P8 owner-record replacement.  Reconcile drops all
		// external, merged-away, zero-count, or non-QuickUsable references.
		Candidate.HotbarBindings = PlayerOnlySession.HotbarBindings;
	}
	if (!ReconcileHotbarBindings(
		Candidate.HotbarBindings, Candidate.RepositorySnapshot, Candidate.Layout, Error))
	{
		Result.Status = ECodeBRunInventoryTerminalStatus::StorageFailure;
		Result.Diagnostic = Error;
		return Result;
	}

	FCodeBRunInventoryTerminalReceipt Receipt;
	Receipt.ReceiptId = FGuid::NewGuid();
	Receipt.OwnerId = OwnerId;
	Receipt.RunInstanceId = RunInstanceId;
	Receipt.TerminalState = TerminalState;
	Receipt.SourceSessionRevision = FrozenSession.SessionRevision;
	Receipt.CommittedUtc = UtcNow();
	// Ground graphs are terminal-only P6 state.  P8 receipts and P5 extraction
	// intentionally freeze only the player carry graph, while the session close
	// atomically discards every WorldDrop record and its complete P1 root closure.
	Receipt.FrozenRunSnapshot = PlayerOnlySession.RepositorySnapshot;
	Receipt.FrozenRunLayout = FrozenSession.Layout;
	Receipt.FrozenSnapshotDigest = TerminalSnapshotDigest(
		Receipt.FrozenRunSnapshot, Receipt.FrozenRunLayout);
	if (!ValidateRunInventoryTerminalReceipt(Receipt, Error))
	{
		Result.Status = ECodeBRunInventoryTerminalStatus::StorageFailure;
		Result.Diagnostic = Error;
		return Result;
	}
	Candidate.TerminalReceipts.Add(Receipt);
	// P9/P11 target records are strictly Run-local. They never enter the P8
	// frozen player-carry receipt or P5 return merge, and close with the exact
	// session without moving, compensating, or recreating their items.
	Candidate.RunLocalNormalContainers.Reset();
	Candidate.RunLocalBodyContainers.Reset();
	Candidate.bHasActiveRunInventorySession = false;
	Candidate.ActiveRunInventorySession = FCodeBRunInventorySession();
	Candidate.LastCommittedUtc = Receipt.CommittedUtc;
	++Candidate.PersistentRevision;
	if (!SaveRecord(Candidate, Error))
	{
		Result.Status = ECodeBRunInventoryTerminalStatus::StorageFailure;
		Result.Diagnostic = Error;
		return Result;
	}
	Record = MoveTemp(Candidate);
	Result.Status = ECodeBRunInventoryTerminalStatus::Committed;
	Result.Diagnostic = TerminalState == ECodeBRunInventoryTerminalState::Extracted
		? TEXT("Code B P8 atomically returned the frozen P7 current graph to P5 and closed P6.")
		: TEXT("Code B P8 atomically confiscated the frozen P7 current graph and closed P6.");
	Result.Receipt = Record.TerminalReceipts.Last();
	return Result;
}

FCodeBRunInventoryBridgeResult FCodeBOutOfRaidProfileStore::BridgeSuccessfulRun(
	const FGuid& RunInstanceId,
	const FCodeBRunInventoryRecoveryContext& RecoveryContext)
{
	FCodeBRunInventoryBridgeResult Result;
	if (!OwnerId.IsValid() || !RunInstanceId.IsValid())
	{
		Result.Status = ECodeBRunInventoryBridgeStatus::InvalidIdentity;
		Result.Diagnostic = TEXT("Code B Run bridge received an invalid OwnerId or RunInstanceId.");
		return Result;
	}
	if (!IFileManager::Get().FileExists(*GetPrimaryPath()) && !IFileManager::Get().FileExists(*GetBackupPath()))
	{
		Result.Status = ECodeBRunInventoryBridgeStatus::NotEnrolled;
		Result.Diagnostic = TEXT("Code B P5 sidecar is absent; post-success bridge is intentionally not enrolled.");
		return Result;
	}
	FString Error;
	FCodeBOutOfRaidInventoryRecord Loaded;
	if (!LoadRecord(Loaded, Error))
	{
		Result.Status = ECodeBRunInventoryBridgeStatus::StorageFailure;
		Result.Diagnostic = Error;
		return Result;
	}
	Record = MoveTemp(Loaded);
	bHasRecord = true;
	if (Record.OwnerId != OwnerId || Record.Receipt.State != ECodeBOutOfRaidHandoffState::Committed)
	{
		Result.Status = ECodeBRunInventoryBridgeStatus::StorageFailure;
		Result.Diagnostic = TEXT("Code B P5 sidecar is not a committed Owner-matched Profile.");
		return Result;
	}
	if (Record.bHasActiveRunInventorySession)
	{
		if (Record.ActiveRunInventorySession.RunInstanceId != RunInstanceId)
		{
			if (Record.ActiveRunInventorySession.BridgeState == ECodeBRunInventoryBridgeState::Prepared)
			{
				RebindPreparedRunInventoryReceipt(RunInstanceId, RecoveryContext, Result);
				return Result;
			}
			Result.Status = ECodeBRunInventoryBridgeStatus::ActiveSessionConflict;
			Result.Diagnostic = TEXT("Code B Run bridge refused a different RunInstanceId while the previous Run session remains active.");
			Result.Session = Record.ActiveRunInventorySession;
			return Result;
		}
		if (Record.ActiveRunInventorySession.BridgeState == ECodeBRunInventoryBridgeState::Committed)
		{
			Result.Status = ECodeBRunInventoryBridgeStatus::AlreadyCommitted;
			Result.Diagnostic = TEXT("Code B Run bridge observed its existing committed session.");
			Result.Session = Record.ActiveRunInventorySession;
			return Result;
		}
		Record.ActiveRunInventorySession.Receipt.RecoveryDiagnostic = TEXT("Recovered verified Prepared Run bridge receipt after interruption.");
		FinalizePreparedRunInventoryReceipt(Result);
		return Result;
	}
	if (Record.PersistentRevision == MAX_int32)
	{
		Result.Status = ECodeBRunInventoryBridgeStatus::StorageFailure;
		Result.Diagnostic = TEXT("Code B out-of-raid persistent revision cannot allocate a Run session.");
		return Result;
	}
	FCodeBRunInventorySession NewSession;
	if (!BuildRunInventorySession(Record, RunInstanceId, NewSession, Error))
	{
		Result.Status = ECodeBRunInventoryBridgeStatus::StorageFailure;
		Result.Diagnostic = Error;
		return Result;
	}
	FCodeBOutOfRaidInventoryRecord Prepared = Record;
	Prepared.bHasActiveRunInventorySession = true;
	Prepared.ActiveRunInventorySession = MoveTemp(NewSession);
	++Prepared.PersistentRevision;
	Prepared.LastCommittedUtc = UtcNow();
	if (!SaveRecord(Prepared, Error))
	{
		Result.Status = ECodeBRunInventoryBridgeStatus::StorageFailure;
		Result.Diagnostic = Error;
		return Result;
	}
	Record = MoveTemp(Prepared);
#if WITH_DEV_AUTOMATION_TESTS
	if (bInterruptAfterRunPreparedReceiptForAutomation)
	{
		Result.Status = ECodeBRunInventoryBridgeStatus::StorageFailure;
		Result.Diagnostic = TEXT("Automation interrupted after verified Prepared Run bridge receipt.");
		return Result;
	}
#endif
	FinalizePreparedRunInventoryReceipt(Result);
	return Result;
}

bool FCodeBOutOfRaidProfileStore::RebindPreparedRunInventoryReceipt(
	const FGuid& NewRunInstanceId,
	const FCodeBRunInventoryRecoveryContext& RecoveryContext,
	FCodeBRunInventoryBridgeResult& OutResult)
{
	if (!bHasRecord || !Record.bHasActiveRunInventorySession
		|| Record.ActiveRunInventorySession.BridgeState != ECodeBRunInventoryBridgeState::Prepared)
	{
		OutResult.Status = ECodeBRunInventoryBridgeStatus::StorageFailure;
		OutResult.Diagnostic = TEXT("Code B Run bridge has no verified Prepared receipt eligible for rebind.");
		return false;
	}
	const FCodeBRunInventorySession& Existing = Record.ActiveRunInventorySession;
	if (!RecoveryContext.AuthorizesRebindFrom(Existing.RunInstanceId))
	{
		OutResult.Status = ECodeBRunInventoryBridgeStatus::ActiveSessionConflict;
		OutResult.Diagnostic = TEXT("Code B Run bridge refused a new RunId without matching Code A RecoveredAbandon context.");
		OutResult.Session = Existing;
		return false;
	}
	FString Error;
	if (!ValidateRunInventorySession(Existing, Error))
	{
		OutResult.Status = ECodeBRunInventoryBridgeStatus::StorageFailure;
		OutResult.Diagnostic = Error;
		return false;
	}
	if (Record.PersistentRevision == MAX_int32 || Existing.SessionRevision == MAX_int32)
	{
		OutResult.Status = ECodeBRunInventoryBridgeStatus::StorageFailure;
		OutResult.Diagnostic = TEXT("Code B Run bridge cannot advance the verified receipt rebind revision.");
		return false;
	}

	FCodeBOutOfRaidInventoryRecord Candidate = Record;
	FCodeBRunInventorySession& Session = Candidate.ActiveRunInventorySession;
	FCodeBRunInventoryRecoveryRebind Rebind;
	Rebind.Sequence = Session.Receipt.RecoveryRebindHistory.Num() + 1;
	Rebind.OldRunId = Session.RunInstanceId;
	Rebind.NewRunId = NewRunInstanceId;
	Rebind.CodeATerminalCause = RecoveryContext.CodeATerminalCause;
	Rebind.ReboundUtc = UtcNow();
	Session.RunInstanceId = NewRunInstanceId;
	Session.Receipt.RunInstanceId = NewRunInstanceId;
	Session.Receipt.RecoveryRebindHistory.Add(MoveTemp(Rebind));
	++Session.SessionRevision;
	Session.Receipt.RecoveryDiagnostic = TEXT("Rebound verified Prepared receipt to a new RunId after Code A RecoveredAbandon.");
	++Candidate.PersistentRevision;
	Candidate.LastCommittedUtc = UtcNow();
	if (!ValidateRunInventorySession(Session, Error) || !SaveRecord(Candidate, Error))
	{
		OutResult.Status = ECodeBRunInventoryBridgeStatus::StorageFailure;
		OutResult.Diagnostic = Error;
		return false;
	}
	Record = MoveTemp(Candidate);
	return FinalizePreparedRunInventoryReceipt(OutResult);
}

bool FCodeBOutOfRaidProfileStore::FinalizePreparedRunInventoryReceipt(FCodeBRunInventoryBridgeResult& OutResult)
{
	if (!bHasRecord || !Record.bHasActiveRunInventorySession
		|| Record.ActiveRunInventorySession.BridgeState != ECodeBRunInventoryBridgeState::Prepared)
	{
		OutResult.Status = ECodeBRunInventoryBridgeStatus::StorageFailure;
		OutResult.Diagnostic = TEXT("Code B Run bridge has no recoverable Prepared receipt.");
		return false;
	}
	FCodeBOutOfRaidInventoryRecord Candidate = Record;
	FCodeBRunInventorySession& Session = Candidate.ActiveRunInventorySession;
	int32 SourceMovedCount = 0;
	for (const FGuid& ItemId : Session.Receipt.MovedItemIds)
	{
		SourceMovedCount += Candidate.RepositorySnapshot.Items.Contains(ItemId) ? 1 : 0;
	}
	FString Error;
	if (SourceMovedCount == Session.Receipt.MovedItemIds.Num())
	{
		if (!ApplyRunInventoryExtraction(
			Candidate.RepositorySnapshot,
			Candidate.Layout,
			Session.Receipt.MovedItemIds,
			Error))
		{
			OutResult.Status = ECodeBRunInventoryBridgeStatus::StorageFailure;
			OutResult.Diagnostic = Error;
			return false;
		}
	}
	else if (SourceMovedCount != 0)
	{
		OutResult.Status = ECodeBRunInventoryBridgeStatus::StorageFailure;
		OutResult.Diagnostic = TEXT("Code B Run bridge refused an indeterminate Prepared extraction.");
		return false;
	}
	if (!ReconcileHotbarBindings(
		Candidate.HotbarBindings, Candidate.RepositorySnapshot, Candidate.Layout, Error)
		|| !ReconcileHotbarBindings(
			Session.HotbarBindings, Session.RepositorySnapshot, Session.Layout, Error))
	{
		OutResult.Status = ECodeBRunInventoryBridgeStatus::StorageFailure;
		OutResult.Diagnostic = Error;
		return false;
	}
	if (Candidate.PersistentRevision == MAX_int32 || Session.SessionRevision == MAX_int32)
	{
		OutResult.Status = ECodeBRunInventoryBridgeStatus::StorageFailure;
		OutResult.Diagnostic = TEXT("Code B Run bridge revision cannot advance during receipt commit.");
		return false;
	}
	const bool bRecovered = !Session.Receipt.RecoveryDiagnostic.IsEmpty();
	const FString CommitUtc = UtcNow();
	Session.BridgeState = ECodeBRunInventoryBridgeState::Committed;
	Session.Receipt.State = ECodeBRunInventoryBridgeState::Committed;
	Session.Receipt.CommittedUtc = CommitUtc;
	Session.LastCommittedUtc = CommitUtc;
	++Session.SessionRevision;
	Candidate.LastCommittedUtc = CommitUtc;
	++Candidate.PersistentRevision;
	if (!SaveRecord(Candidate, Error))
	{
		OutResult.Status = ECodeBRunInventoryBridgeStatus::StorageFailure;
		OutResult.Diagnostic = Error;
		return false;
	}
	Record = MoveTemp(Candidate);
	OutResult.Status = bRecovered
		? ECodeBRunInventoryBridgeStatus::RecoveredPreparedReceipt
		: ECodeBRunInventoryBridgeStatus::Committed;
	OutResult.Diagnostic = bRecovered
		? TEXT("Code B Run bridge recovered and committed its Prepared receipt.")
		: TEXT("Code B Run bridge committed a read-only Run inventory session.");
	OutResult.Session = Record.ActiveRunInventorySession;
	return true;
}
