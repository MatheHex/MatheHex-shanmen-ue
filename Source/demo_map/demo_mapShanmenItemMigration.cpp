#include "demo_mapShanmenItemMigration.h"

#include "CodeB/demo_mapCodeBP2.h"
#include "ShanmenDeterministicId.h"
#include "ShanmenItemRepository.h"
#include "ShanmenItemTags.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapProfileRepository.h"
#include "demo_mapShanmenItemMetadataAdapter.h"

namespace
{
	using namespace demo_map_code_b;

	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	bool GuidLess(const FGuid& Left, const FGuid& Right)
	{
		return GuidDigits(Left) < GuidDigits(Right);
	}

	Fdemo_mapProfileSessionSnapshot MakeProfileSnapshot(
		const Fdemo_mapPersistentProfile& Profile)
	{
		Fdemo_mapProfileSessionSnapshot Snapshot;
		Snapshot.SessionState = Edemo_mapProfileSessionState::ReadyForPreparation;
		Snapshot.ProfileId = Profile.ProfileId;
		Snapshot.SaveGeneration = Profile.SaveGeneration;
		Snapshot.PersistentSpiritStones = Profile.PersistentSpiritStones;
		Snapshot.TownLevel = Profile.TownLevel;
		Snapshot.RiskSpiritStones = Profile.ActiveRun.RiskSpiritStones;
		Snapshot.OrderedPermanentStash = Profile.PermanentStash;
		Snapshot.ShopStock = Profile.ShopStock;
		Snapshot.PreparationLayout = Profile.PreparationLayout;
		Snapshot.WarehouseLayout = Profile.WarehouseLayout;
		Snapshot.ConsumedSpiritStoneSourceIds =
			Profile.ActiveRun.ConsumedSpiritStoneSourceIds;
		Snapshot.ActiveRunId = Profile.ActiveRun.ActiveRunId;
		Snapshot.LastSettlementId = Profile.LastSettlementId;
		return Snapshot;
	}

	Fdemo_mapShanmenItemMigrationResult Fail(
		Edemo_mapShanmenItemMigrationError Error,
		const FString& Diagnostic)
	{
		Fdemo_mapShanmenItemMigrationResult Result;
		Result.Error = Error;
		Result.Diagnostic = Diagnostic;
		Result.Receipt.Error = Error;
		return Result;
	}

	bool CheckEquipmentPlacement(
		const FGuid& ItemId,
		const FGuid& ExpectedContainerId,
		const FCodeBSnapshot& Snapshot)
	{
		if (!ItemId.IsValid())
		{
			return true;
		}
		const FCodeBItemInstance* Item = Snapshot.Items.Find(ItemId);
		return Item
			&& ExpectedContainerId.IsValid()
			&& Item->ParentContainerId == ExpectedContainerId
			&& Item->SlotIndex == 0;
	}

	FString BuildCandidateDigest(
		const FShanmenItemAuthoritySnapshot& Candidate,
		bool bIncludeRewardMetadata)
	{
		TArray<FString> Parts;
		Parts.Add(Candidate.Content.Version.ToString());
		Parts.Add(Candidate.Content.Digest);
		Parts.Add(FString::FromInt(Candidate.AuthorityRevision));
		for (const FShanmenItemDefinition& Definition : Candidate.Definitions)
		{
			Parts.Add(FString::Printf(
				TEXT("D:%s:%d:%d:%d:%d"),
				*Definition.DefinitionId.ToString(),
				Definition.MaxStack,
				Definition.MaxDurability,
				Definition.MaxCharges,
				Definition.Supports(EShanmenItemResourceKind::Quantity) ? 1 : 0));
		}
		for (const FShanmenItemContainer& Container : Candidate.Containers)
		{
			TArray<FString> SlotIds;
			SlotIds.Reserve(Container.Slots.Num());
			for (const FGuid& ItemId : Container.Slots)
			{
				SlotIds.Add(GuidDigits(ItemId));
			}
			Parts.Add(FString::Printf(
				TEXT("C:%s:%s:%s:%s:%s"),
				*GuidDigits(Container.ContainerId),
				*GuidDigits(Container.RunId),
				*GuidDigits(Container.OwnerId),
				*Container.ContainerType.ToString(),
				*FString::Join(SlotIds, TEXT(","))));
		}
		for (const FShanmenItemInstance& Item : Candidate.Items)
		{
			Parts.Add(FString::Printf(
				TEXT("I:%s:%s:%s:%s:%d:%d"),
				*GuidDigits(Item.ItemInstanceId),
				*Item.DefinitionId.ToString(),
				*GuidDigits(Item.ParentContainerId),
				*GuidDigits(Item.ChildContainerId),
				Item.SlotIndex,
				Item.Quantity));
			if (bIncludeRewardMetadata && !Item.RewardMetadata.IsEmpty())
			{
				const FShanmenItemRewardMetadata& Metadata = Item.RewardMetadata;
				Parts.Add(FString::Printf(
					TEXT("M:%d:%s:%d:%s:%s:%s:%s:%lld:%s:%s:%d:%d"),
					static_cast<int32>(Metadata.RewardEventKind),
					*GuidDigits(Metadata.RewardEventId),
					Metadata.RewardValueMultiplierBps,
					*Metadata.RewardSourceRoleId.ToString(),
					*GuidDigits(Metadata.RareRewardEventId),
					*Metadata.RareRewardPolicyId.ToString(),
					*Metadata.RareRewardTierId.ToString(),
					Metadata.RareRewardBonusValue,
					*GuidDigits(Metadata.AffixSetEventId),
					*Metadata.AffixPolicyId.ToString(),
					static_cast<int32>(Metadata.AffixAcquisition),
					Metadata.Affixes.Num()));
				for (const FShanmenItemResolvedRewardAffix& Affix :
					Metadata.Affixes)
				{
					Parts.Add(FString::Printf(
						TEXT("A:%s:%d:%d:%lld"),
						*Affix.AffixId.ToString(),
						static_cast<int32>(Affix.Tier),
						Affix.ResolvedMagnitudeScaled,
						Affix.ResolvedValue));
				}
			}
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			FName(TEXT("Shanmen.Items.MigrationCandidateDigest")), Parts)
			.ToString(EGuidFormats::Digits);
	}
}

FShanmenItemMigrationEvidence
Fdemo_mapShanmenItemMigrationReceipt::ToPersistenceEvidence() const
{
	FShanmenItemMigrationEvidence Evidence;
	Evidence.MigrationId = MigrationId;
	Evidence.OwnerId = OwnerId;
	Evidence.SourceProfileSchema = SourceProfileSchema;
	Evidence.SourceSaveGeneration = SourceSaveGeneration;
	Evidence.SourceCodeBPersistentRevision = SourceCodeBPersistentRevision;
	Evidence.SourceCodeBRepositoryRevision = SourceCodeBRepositoryRevision;
	Evidence.DefinitionCount = DefinitionCount;
	Evidence.ContainerCount = ContainerCount;
	Evidence.ItemCount = ItemCount;
	Evidence.SourceFingerprint = SourceFingerprint;
	Evidence.CandidateDigest = CandidateDigest;
	return Evidence;
}

Fdemo_mapShanmenItemMigrationResult Fdemo_mapShanmenItemMigration::BuildCandidate(
	const Fdemo_mapPersistentProfile& CodeAProfile,
	const FCodeBOutOfRaidInventoryRecord& CodeBRecord,
	const FShanmenContentStamp& TargetContent)
{
	using namespace demo_map_code_b;

	if (!TargetContent.IsValid())
	{
		return Fail(
			Edemo_mapShanmenItemMigrationError::InvalidTargetContent,
			TEXT("P1.1 migration requires a valid target ContentStamp."));
	}

	FString ProfileError;
	if (CodeAProfile.SchemaVersion
			!= Fdemo_mapPersistentProfile::CurrentSchemaVersion
		|| !Fdemo_mapProfileRepository().ValidateProfile(
			CodeAProfile, &ProfileError))
	{
		return Fail(
			Edemo_mapShanmenItemMigrationError::InvalidCodeAProfile,
			ProfileError.IsEmpty()
				? TEXT("Code A Profile is not at the current validated schema.")
				: ProfileError);
	}
	if (CodeAProfile.ActiveRun.bHasActiveRun
		|| CodeBRecord.bHasActiveRunInventorySession)
	{
		return Fail(
			Edemo_mapShanmenItemMigrationError::ActiveRunUnsupported,
			TEXT("P1.1 refuses mid-Run migration; settle or abandon the legacy Run first."));
	}

	if (CodeBRecord.SchemaVersion
			!= FCodeBOutOfRaidInventoryRecord::CurrentSchemaVersion
		|| CodeBRecord.PersistentRevision < 1
		|| CodeBRecord.RepositorySnapshot.Revision < 0
		|| CodeBRecord.CreatedUtc.IsEmpty()
		|| CodeBRecord.LastCommittedUtc.IsEmpty()
		|| CodeBRecord.Receipt.State
			!= ECodeBOutOfRaidHandoffState::Committed
		|| !CodeBRecord.Layout.LayoutId.IsValid()
		|| !CodeBRecord.RunLocalNormalContainers.IsEmpty()
		|| !CodeBRecord.RunLocalBodyContainers.IsEmpty())
	{
		return Fail(
			Edemo_mapShanmenItemMigrationError::InvalidCodeBRecord,
			TEXT("Code B sidecar is not a current, terminal, committed out-of-raid record."));
	}
	if (!CodeAProfile.ProfileId.IsValid()
		|| CodeBRecord.OwnerId != CodeAProfile.ProfileId
		|| CodeBRecord.Receipt.SourceProfileId != CodeAProfile.ProfileId)
	{
		return Fail(
			Edemo_mapShanmenItemMigrationError::SourceIdentityMismatch,
			TEXT("Code A ProfileId and Code B Owner/source identities do not match."));
	}

	const Fdemo_mapProfileSessionSnapshot ProfileSnapshot =
		MakeProfileSnapshot(CodeAProfile);
	const FString ExpectedSourceFingerprint =
		FCodeBOutOfRaidProfileStore::
			ComputeProfileSourceFingerprintForMigration(ProfileSnapshot);
	if (CodeBRecord.Receipt.SourceFingerprint != ExpectedSourceFingerprint
		|| !CodeBRecord.Receipt.InvalidLegacyEntries.IsEmpty())
	{
		return Fail(
			Edemo_mapShanmenItemMigrationError::SourceProvenanceMismatch,
			TEXT("Code B handoff provenance does not exactly describe the current Code A Profile."));
	}

	FCodeBRepository CodeBValidation;
	FString CodeBError;
	if (!CodeBValidation.LoadPersistedSnapshot(
			CodeBRecord.RepositorySnapshot, &CodeBError))
	{
		return Fail(
			Edemo_mapShanmenItemMigrationError::InvalidCodeBRecord,
			CodeBError.IsEmpty()
				? TEXT("Code B repository snapshot is invalid.")
				: CodeBError);
	}
	FCodeBP2Projection Projection;
	if (!FCodeBP2ProjectionBuilder::Build(
			CodeBValidation, CodeBRecord.Layout, Projection, nullptr, &CodeBError))
	{
		return Fail(
			Edemo_mapShanmenItemMigrationError::InvalidCodeBRecord,
			CodeBError.IsEmpty()
				? TEXT("Code B layout does not close over its repository snapshot.")
				: CodeBError);
	}

	if (CodeBRecord.Receipt.ItemMappings.Num()
			!= CodeAProfile.PermanentStash.Num()
		|| CodeBRecord.RepositorySnapshot.Items.Num()
			!= CodeAProfile.PermanentStash.Num())
	{
		return Fail(
			Edemo_mapShanmenItemMigrationError::SourceItemMismatch,
			TEXT("Code A and Code B item counts do not match exactly."));
	}
	TSet<FGuid> MappedItemIds;
	for (const FCodeBOutOfRaidHandoffMapping& Mapping :
		CodeBRecord.Receipt.ItemMappings)
	{
		if (!Mapping.LegacyItemId.IsValid()
			|| Mapping.LegacyItemId != Mapping.CodeBItemId
			|| MappedItemIds.Contains(Mapping.LegacyItemId))
		{
			return Fail(
				Edemo_mapShanmenItemMigrationError::SourceProvenanceMismatch,
				TEXT("Code B handoff mappings are not a one-to-one identity map."));
		}
		MappedItemIds.Add(Mapping.LegacyItemId);
	}

	TSet<FName> UsedDefinitionIds;
	TMap<FGuid, const Fdemo_mapPersistentItemRecord*> CodeAItemsById;
	TSet<FGuid> CodeBChildContainerIds;
	for (const TPair<FGuid, FCodeBItemInstance>& Pair :
		CodeBRecord.RepositorySnapshot.Items)
	{
		if (Pair.Value.ChildContainerId.IsValid())
		{
			if (CodeBChildContainerIds.Contains(Pair.Value.ChildContainerId))
			{
				return Fail(
					Edemo_mapShanmenItemMigrationError::SourceContainerMismatch,
					TEXT("Multiple Code B items claim the same child container."));
			}
			CodeBChildContainerIds.Add(Pair.Value.ChildContainerId);
		}
	}
	for (const Fdemo_mapPersistentItemRecord& CodeAItem :
		CodeAProfile.PermanentStash)
	{
		if (CodeAItemsById.Contains(CodeAItem.ItemInstanceId))
		{
			return Fail(
				Edemo_mapShanmenItemMigrationError::SourceItemMismatch,
				TEXT("Code A contains duplicate persistent item identities."));
		}
		CodeAItemsById.Add(CodeAItem.ItemInstanceId, &CodeAItem);
		const FCodeBItemInstance* CodeBItem =
			CodeBRecord.RepositorySnapshot.Items.Find(CodeAItem.ItemInstanceId);
		const FCodeBItemDefinition* CodeBDefinition = CodeBItem
			? CodeBRecord.RepositorySnapshot.Definitions.Find(
				CodeBItem->DefinitionId)
			: nullptr;
		const Fdemo_mapItemDefinition* LegacyDefinition =
			Fdemo_mapItemDefinitions::Find(CodeAItem.ItemDefinitionId);
		if (!MappedItemIds.Contains(CodeAItem.ItemInstanceId)
			|| !CodeBItem
			|| !CodeBDefinition
			|| !LegacyDefinition
			|| CodeBItem->DefinitionId != CodeAItem.ItemDefinitionId
			|| CodeBItem->Quantity != CodeAItem.StackCount
			|| CodeBItem->LegacyAffixDigest
				!= FCodeBOutOfRaidProfileStore::
					ComputeLegacyAffixDigestForMigration(CodeAItem)
			|| CodeBDefinition->MaxStack != LegacyDefinition->MaxStackSize
			|| CodeBDefinition->bStackable
				!= (LegacyDefinition->MaxStackSize > 1))
		{
			return Fail(
				Edemo_mapShanmenItemMigrationError::SourceItemMismatch,
				FString::Printf(
					TEXT("Code A and Code B disagree for item %s."),
					*GuidDigits(CodeAItem.ItemInstanceId)));
		}
		if (CodeAItem.LegacySpatialParentItemInstanceId.IsValid())
		{
			const FCodeBItemInstance* Parent =
				CodeBRecord.RepositorySnapshot.Items.Find(
					CodeAItem.LegacySpatialParentItemInstanceId);
			if (!Parent
				|| !Parent->ChildContainerId.IsValid()
				|| Parent->ChildContainerId != CodeBItem->ParentContainerId)
			{
				return Fail(
					Edemo_mapShanmenItemMigrationError::SourceContainerMismatch,
					TEXT("Legacy spatial parent relation differs from Code B closure."));
			}
		}
		else if (CodeBChildContainerIds.Contains(CodeBItem->ParentContainerId))
		{
			return Fail(
				Edemo_mapShanmenItemMigrationError::SourceContainerMismatch,
				TEXT("Code B places an item in child storage absent from Code A provenance."));
		}
		UsedDefinitionIds.Add(CodeAItem.ItemDefinitionId);
	}

	if (!CheckEquipmentPlacement(
			CodeAProfile.PreparationLayout.WeaponItemInstanceId,
			CodeBRecord.Layout.WeaponContainerId,
			CodeBRecord.RepositorySnapshot)
		|| !CheckEquipmentPlacement(
			CodeAProfile.PreparationLayout.ArmorItemInstanceId,
			CodeBRecord.Layout.ArmorContainerId,
			CodeBRecord.RepositorySnapshot)
		|| !CheckEquipmentPlacement(
			CodeAProfile.PreparationLayout.AccessoryItemInstanceId,
			CodeBRecord.Layout.AccessoryContainerIds.IsValidIndex(0)
				? CodeBRecord.Layout.AccessoryContainerIds[0] : FGuid(),
			CodeBRecord.RepositorySnapshot)
		|| !CheckEquipmentPlacement(
			CodeAProfile.PreparationLayout.SpatialRingItemInstanceId,
			CodeBRecord.Layout.SpatialContainerId,
			CodeBRecord.RepositorySnapshot)
		|| !CheckEquipmentPlacement(
			CodeAProfile.PreparationLayout.BackpackItemInstanceId,
			CodeBRecord.Layout.BackpackContainerId,
			CodeBRecord.RepositorySnapshot))
	{
		return Fail(
			Edemo_mapShanmenItemMigrationError::SourceContainerMismatch,
			TEXT("Code A equipment references disagree with Code B placement."));
	}

	TSet<FGuid> AllowedContainerIds;
	for (const TPair<FName, FGuid>& Root :
		CodeBRecord.Layout.GetOrderedContainers())
	{
		if (Root.Value.IsValid())
		{
			AllowedContainerIds.Add(Root.Value);
		}
	}
	AllowedContainerIds.Append(CodeBChildContainerIds);
	for (const TPair<FGuid, FCodeBContainer>& Pair :
		CodeBRecord.RepositorySnapshot.Containers)
	{
		if (!AllowedContainerIds.Contains(Pair.Key))
		{
			return Fail(
				Edemo_mapShanmenItemMigrationError::SourceContainerMismatch,
				TEXT("Code B snapshot contains a container outside the committed layout closure."));
		}
	}

	const FGuid ScopeId = FShanmenDeterministicId::FromCanonicalParts(
		FName(TEXT("Shanmen.Items.OutOfRaidMigrationScope")),
		{
			GuidDigits(CodeAProfile.ProfileId),
			FString::FromInt(CodeAProfile.SaveGeneration),
			FString::FromInt(CodeBRecord.PersistentRevision),
			FString::FromInt(CodeBRecord.RepositorySnapshot.Revision)
		});
	if (!ScopeId.IsValid())
	{
		return Fail(
			Edemo_mapShanmenItemMigrationError::CandidateRejected,
			TEXT("Unable to derive the deterministic out-of-raid migration scope."));
	}

	FShanmenItemAuthoritySnapshot Candidate;
	Candidate.AuthorityRevision = 0;
	Candidate.Content = TargetContent;
	TArray<FName> OrderedDefinitionIds = UsedDefinitionIds.Array();
	OrderedDefinitionIds.Sort([](FName Left, FName Right)
	{
		return Left.ToString() < Right.ToString();
	});
	for (FName DefinitionId : OrderedDefinitionIds)
	{
		const FCodeBItemDefinition* SourceDefinition =
			CodeBRecord.RepositorySnapshot.Definitions.Find(DefinitionId);
		if (!SourceDefinition)
		{
			return Fail(
				Edemo_mapShanmenItemMigrationError::SourceItemMismatch,
				TEXT("A used Code B definition disappeared during normalization."));
		}
		FShanmenItemDefinition Definition;
		Definition.DefinitionId = DefinitionId;
		Definition.MaxStack = SourceDefinition->MaxStack;
		if (SourceDefinition->bStackable)
		{
			Definition.ItemTags.AddTag(
				FShanmenItemNativeTags::CapabilityConsumeQuantity());
		}
		else if (const Fdemo_mapItemDefinition* ProductDefinition =
			Fdemo_mapItemDefinitions::Find(DefinitionId);
			ProductDefinition
			&& ProductDefinition->MaxStackSize == 1
			&& !ProductDefinition->CompatibleSlotIds.IsEmpty())
		{
			// P1.6 preparation selection is a durable deployment intent. The
			// capability is derived from the immutable product definition during
			// the one-time migration; mutable UI/profile state never grants it.
			Definition.ItemTags.AddTag(
				FShanmenItemNativeTags::CapabilityDeploy());
		}
		Candidate.Definitions.Add(MoveTemp(Definition));
	}

	TArray<FGuid> OrderedContainerIds;
	CodeBRecord.RepositorySnapshot.Containers.GetKeys(OrderedContainerIds);
	OrderedContainerIds.Sort(GuidLess);
	for (const FGuid& ContainerId : OrderedContainerIds)
	{
		const FCodeBContainer& SourceContainer =
			CodeBRecord.RepositorySnapshot.Containers.FindChecked(ContainerId);
		FShanmenItemContainer Container;
		Container.ContainerId = SourceContainer.ContainerId;
		Container.RunId = ScopeId;
		Container.OwnerId = CodeAProfile.ProfileId;
		Container.ContainerType = SourceContainer.ContainerType;
		Container.Slots = SourceContainer.Slots;
		Candidate.Containers.Add(MoveTemp(Container));
	}

	TArray<FGuid> OrderedItemIds;
	CodeBRecord.RepositorySnapshot.Items.GetKeys(OrderedItemIds);
	OrderedItemIds.Sort(GuidLess);
	for (const FGuid& ItemId : OrderedItemIds)
	{
		const FCodeBItemInstance& SourceItem =
			CodeBRecord.RepositorySnapshot.Items.FindChecked(ItemId);
		const Fdemo_mapPersistentItemRecord* SourceMetadata =
			CodeAItemsById.FindRef(ItemId);
		if (!SourceMetadata)
		{
			return Fail(
				Edemo_mapShanmenItemMigrationError::SourceItemMismatch,
				TEXT("Code A metadata source disappeared during normalization."));
		}
		FShanmenItemInstance Item;
		Item.ItemInstanceId = SourceItem.ItemId;
		Item.DefinitionId = SourceItem.DefinitionId;
		FString MetadataError;
		if (!Fdemo_mapShanmenItemMetadataAdapter::FromPersistentItem(
				*SourceMetadata, Item.RewardMetadata, MetadataError))
		{
			return Fail(
				Edemo_mapShanmenItemMigrationError::SourceItemMismatch,
				FString::Printf(
					TEXT("Code A reward metadata is invalid for item %s: %s"),
					*GuidDigits(ItemId), *MetadataError));
		}
		Item.RunId = ScopeId;
		Item.OwnerId = CodeAProfile.ProfileId;
		Item.ParentContainerId = SourceItem.ParentContainerId;
		Item.ChildContainerId = SourceItem.ChildContainerId;
		Item.SlotIndex = SourceItem.SlotIndex;
		Item.Quantity = SourceItem.Quantity;
		Item.Revision = 0;
		Item.State = EShanmenItemInstanceState::Stored;
		Candidate.Items.Add(MoveTemp(Item));
	}

	FShanmenItemRepository ValidationRepository;
	EShanmenItemTransactionError CandidateError =
		EShanmenItemTransactionError::None;
	if (!ValidationRepository.TryLoadSnapshot(Candidate, &CandidateError))
	{
		return Fail(
			Edemo_mapShanmenItemMigrationError::CandidateRejected,
			FString::Printf(
				TEXT("Normalized ShanmenItems candidate failed invariant validation (%d)."),
				static_cast<int32>(CandidateError)));
	}
	Candidate = ValidationRepository.CaptureSnapshot();
	const FString CandidateDigest = BuildCandidateDigest(Candidate, true);
	// MigrationId remains compatible with schema-1 authorities that omitted
	// reward metadata. CandidateDigest still seals the complete schema-2 value.
	const FString IdentityCandidateDigest = BuildCandidateDigest(Candidate, false);
	const FGuid MigrationId = FShanmenDeterministicId::FromCanonicalParts(
		FName(TEXT("Shanmen.Items.LegacyMigration")),
		{
			GuidDigits(CodeAProfile.ProfileId),
			ExpectedSourceFingerprint,
			FString::FromInt(CodeBRecord.PersistentRevision),
			TargetContent.Version.ToString(),
			TargetContent.Digest,
			IdentityCandidateDigest
		});

	Fdemo_mapShanmenItemMigrationResult Result;
	Result.Error = Edemo_mapShanmenItemMigrationError::None;
	Result.Diagnostic =
		TEXT("Code A and Code B matched; one validated ShanmenItems candidate was produced without mutating either legacy source.");
	Result.Candidate = MoveTemp(Candidate);
	Result.Receipt.bSuccess = true;
	Result.Receipt.Error = Edemo_mapShanmenItemMigrationError::None;
	Result.Receipt.MigrationId = MigrationId;
	Result.Receipt.OwnerId = CodeAProfile.ProfileId;
	Result.Receipt.ScopeId = ScopeId;
	Result.Receipt.SourceProfileSchema = CodeAProfile.SchemaVersion;
	Result.Receipt.SourceSaveGeneration = CodeAProfile.SaveGeneration;
	Result.Receipt.SourceCodeBPersistentRevision = CodeBRecord.PersistentRevision;
	Result.Receipt.SourceCodeBRepositoryRevision =
		CodeBRecord.RepositorySnapshot.Revision;
	Result.Receipt.DefinitionCount = Result.Candidate.Definitions.Num();
	Result.Receipt.ContainerCount = Result.Candidate.Containers.Num();
	Result.Receipt.ItemCount = Result.Candidate.Items.Num();
	Result.Receipt.SourceFingerprint = ExpectedSourceFingerprint;
	Result.Receipt.CandidateDigest = CandidateDigest;
	return Result;
}
