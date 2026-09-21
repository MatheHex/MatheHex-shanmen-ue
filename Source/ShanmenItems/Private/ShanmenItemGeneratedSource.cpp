#include "ShanmenItemGeneratedSource.h"

#include "ShanmenDeterministicId.h"

namespace
{
	bool SameContent(const FShanmenContentStamp& A, const FShanmenContentStamp& B)
	{
		return A.Version == B.Version && A.Digest == B.Digest;
	}

	FGuid ContainerIdentity(const FGuid& SourceId)
	{
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Items.GeneratedContainer.v1"), { SourceId.ToString(EGuidFormats::Digits) });
	}

	FGuid ItemIdentity(const FGuid& SourceId, int32 Index)
	{
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Items.GeneratedItem.v1"),
			{ SourceId.ToString(EGuidFormats::Digits), FString::FromInt(Index) });
	}
}

bool FShanmenItemGeneratedSourceEntry::IsValid() const
{
	return Definition.IsValid()
		&& Definition.ItemTags.Num() <= FShanmenItemGeneratedSourcePlan::MaxTagsPerDefinition
		&& Quantity > 0 && Quantity <= Definition.MaxStack
		&& !SectionId.IsNone()
		&& SlotIndex >= 0 && SlotIndex < FShanmenItemGeneratedSourcePlan::MaxSlots
		&& UnitValue >= 0 && TotalValue >= 0
		&& RewardMetadata.IsValid()
		&& ((ChildContainerType.IsNone() && ChildContainerCapacity == 0)
			|| (!ChildContainerType.IsNone() && Quantity == 1
				&& ChildContainerCapacity > 0
				&& ChildContainerCapacity <= FShanmenItemGeneratedSourcePlan::MaxSlots));
}

bool FShanmenItemGeneratedSourceEntry::operator==(const FShanmenItemGeneratedSourceEntry& Other) const
{
	return Definition == Other.Definition && Quantity == Other.Quantity
		&& SectionId == Other.SectionId && SlotIndex == Other.SlotIndex
		&& UnitValue == Other.UnitValue && TotalValue == Other.TotalValue
		&& RewardMetadata == Other.RewardMetadata
		&& ChildContainerType == Other.ChildContainerType
		&& ChildContainerCapacity == Other.ChildContainerCapacity;
}

bool FShanmenItemGeneratedSourcePlan::IsValid() const
{
	if (!OwnerId.IsValid() || !RunId.IsValid() || SourceRoleId.IsNone()
		|| !Content.IsValid() || Content.Digest.Len() > MaxDigestLength
		|| BudgetProfileId.IsNone() || EffectiveSeed == 0
		|| RandomizedBudget < 0 || GeneratedTotalValue < 0 || ResidualValue < 0
		|| ExpectedSequence < 0 || ExpectedSequence == MAX_int64
		|| PityStateBefore < 0 || PityStateAfter < 0
		|| (!bPityCommitRequired && PityStateBefore != PityStateAfter)
		|| (!bLegacyCompatibilityView && (ProjectionId.IsNone() || DistributionProfileId.IsNone()))
		|| Entries.IsEmpty() || Entries.Num() > MaxEntries)
	{
		return false;
	}

	TMap<FName, TSet<int32>> OccupiedSlots;
	TMap<FName, const FShanmenItemDefinition*> Definitions;
	for (const FShanmenItemGeneratedSourceEntry& Entry : Entries)
	{
		if (!Entry.IsValid()
			|| (!Entry.RewardMetadata.RewardSourceRoleId.IsNone()
				&& Entry.RewardMetadata.RewardSourceRoleId != SourceRoleId))
		{
			return false;
		}
		TSet<int32>& Slots = OccupiedSlots.FindOrAdd(Entry.SectionId);
		if (Slots.Contains(Entry.SlotIndex))
		{
			return false;
		}
		Slots.Add(Entry.SlotIndex);
		const FShanmenItemDefinition* const* Previous = Definitions.Find(Entry.Definition.DefinitionId);
		if (Previous && !(**Previous == Entry.Definition))
		{
			return false;
		}
		Definitions.FindOrAdd(Entry.Definition.DefinitionId) = &Entry.Definition;
	}
	return true;
}

bool FShanmenItemGeneratedSourcePlan::operator==(const FShanmenItemGeneratedSourcePlan& Other) const
{
	return OwnerId == Other.OwnerId && RunId == Other.RunId && SourceRoleId == Other.SourceRoleId
		&& SameContent(Content, Other.Content) && SlotId == Other.SlotId
		&& ProjectionId == Other.ProjectionId && DistributionProfileId == Other.DistributionProfileId
		&& BudgetProfileId == Other.BudgetProfileId && MarkerId == Other.MarkerId
		&& EncounterId == Other.EncounterId && JackpotPolicyId == Other.JackpotPolicyId
		&& RareExtremePolicyId == Other.RareExtremePolicyId && AffixPolicyId == Other.AffixPolicyId
		&& EffectiveSeed == Other.EffectiveSeed && RandomizedBudget == Other.RandomizedBudget
		&& GeneratedTotalValue == Other.GeneratedTotalValue && ResidualValue == Other.ResidualValue
		&& ExpectedSequence == Other.ExpectedSequence && PityStateBefore == Other.PityStateBefore
		&& PityStateAfter == Other.PityStateAfter && bPityCommitRequired == Other.bPityCommitRequired
		&& bFallbackUsed == Other.bFallbackUsed && bLegacyCompatibilityView == Other.bLegacyCompatibilityView
		&& Entries == Other.Entries;
}

bool FShanmenItemGeneratedSourceView::IsValid() const
{
	return OwnerId.IsValid() && RunId.IsValid() && SourceContent.IsValid()
		&& SourceContent.Digest.Len() <= FShanmenItemGeneratedSourcePlan::MaxDigestLength
		&& (State == EShanmenItemGeneratedSourceRunState::Active
			|| State == EShanmenItemGeneratedSourceRunState::Finalized)
		&& AcceptedSequence >= 0 && PityState >= 0;
}

bool FShanmenItemGeneratedSourceReceipt::IsValid() const
{
	if (!Plan.IsValid() || AcceptedSequence != Plan.ExpectedSequence + 1
		|| SourceId != FShanmenItemGeneratedSourceContract::MakeSourceId(
			Plan.OwnerId, Plan.RunId, Plan.SourceRoleId)
		|| ContainerId != ContainerIdentity(SourceId) || ItemIds.Num() != Plan.Entries.Num())
	{
		return false;
	}
	TSet<FGuid> Unique { SourceId, ContainerId, Plan.OwnerId, Plan.RunId };
	// Also reject the improbable domain collision instead of silently re-keying.
	if (SourceId == ContainerId || SourceId == Plan.OwnerId || SourceId == Plan.RunId
		|| ContainerId == Plan.OwnerId || ContainerId == Plan.RunId)
	{
		return false;
	}
	for (int32 Index = 0; Index < ItemIds.Num(); ++Index)
	{
		if (ItemIds[Index] != ItemIdentity(SourceId, Index) || Unique.Contains(ItemIds[Index]))
		{
			return false;
		}
		Unique.Add(ItemIds[Index]);
	}
	return true;
}

bool FShanmenItemGeneratedSourceReceipt::operator==(const FShanmenItemGeneratedSourceReceipt& Other) const
{
	return Plan == Other.Plan && SourceId == Other.SourceId && ContainerId == Other.ContainerId
		&& ItemIds == Other.ItemIds && AcceptedSequence == Other.AcceptedSequence;
}

FGuid FShanmenItemGeneratedSourceContract::MakeSourceId(
	const FGuid& OwnerId, const FGuid& RunId, FName SourceRoleId)
{
	if (!OwnerId.IsValid() || !RunId.IsValid() || SourceRoleId.IsNone())
	{
		return FGuid();
	}
	// FName equality is case-insensitive. Persisted casing must not split identity.
	return FShanmenDeterministicId::FromCanonicalParts(TEXT("Shanmen.Items.GeneratedSource.v1"),
		{ OwnerId.ToString(EGuidFormats::Digits), RunId.ToString(EGuidFormats::Digits),
			SourceRoleId.ToString().ToLower() });
}

FShanmenItemGeneratedSourceEvaluation FShanmenItemGeneratedSourceContract::Evaluate(
	const FShanmenItemGeneratedSourceView& View,
	const FShanmenItemGeneratedSourcePlan& Plan,
	const FShanmenItemGeneratedSourceReceipt* Existing)
{
	auto Reject = [](EShanmenItemGeneratedSourceError Error)
	{
		FShanmenItemGeneratedSourceEvaluation Result;
		Result.Error = Error;
		return Result;
	};
	if (!Plan.IsValid()) { return Reject(EShanmenItemGeneratedSourceError::InvalidPlan); }
	if (!View.IsValid()) { return Reject(EShanmenItemGeneratedSourceError::InvalidView); }
	if (Plan.OwnerId != View.OwnerId || Plan.RunId != View.RunId)
	{
		return Reject(EShanmenItemGeneratedSourceError::ScopeMismatch);
	}
	if (!SameContent(Plan.Content, View.SourceContent))
	{
		return Reject(EShanmenItemGeneratedSourceError::ContentMismatch);
	}
	if (Existing)
	{
		if (!Existing->IsValid() || Existing->GetSourceId() != MakeSourceId(Plan.OwnerId, Plan.RunId, Plan.SourceRoleId)
			|| Existing->GetAcceptedSequence() > View.AcceptedSequence
			|| (Existing->GetAcceptedSequence() == View.AcceptedSequence
				&& Existing->GetPlan().PityStateAfter != View.PityState))
		{
			return Reject(EShanmenItemGeneratedSourceError::InvalidExistingReceipt);
		}
		if (!(Existing->GetPlan() == Plan))
		{
			return Reject(EShanmenItemGeneratedSourceError::SourceConflict);
		}
		FShanmenItemGeneratedSourceEvaluation Result;
		Result.Decision = EShanmenItemGeneratedSourceDecision::ExactReplay;
		Result.Receipt = *Existing;
		return Result;
	}
	if (View.State != EShanmenItemGeneratedSourceRunState::Active)
	{
		return Reject(EShanmenItemGeneratedSourceError::RunClosed);
	}
	if (Plan.ExpectedSequence != View.AcceptedSequence || Plan.PityStateBefore != View.PityState)
	{
		return Reject(EShanmenItemGeneratedSourceError::CursorConflict);
	}
	FShanmenItemGeneratedSourceEvaluation Result;
	Result.Receipt.Plan = Plan;
	Result.Receipt.SourceId = MakeSourceId(Plan.OwnerId, Plan.RunId, Plan.SourceRoleId);
	Result.Receipt.ContainerId = ContainerIdentity(Result.Receipt.SourceId);
	Result.Receipt.AcceptedSequence = Plan.ExpectedSequence + 1;
	for (int32 Index = 0; Index < Plan.Entries.Num(); ++Index)
	{
		Result.Receipt.ItemIds.Add(ItemIdentity(Result.Receipt.SourceId, Index));
	}
	if (!Result.Receipt.IsValid()) { return Reject(EShanmenItemGeneratedSourceError::InvalidPlan); }
	Result.Decision = EShanmenItemGeneratedSourceDecision::CandidatePrepared;
	return Result;
}
