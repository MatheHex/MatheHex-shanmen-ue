#include "demo_mapShanmenItemGeneratedSourceAdapter.h"
#include "demo_mapShanmenItemDefinitionAdapter.h"
#include "demo_mapShanmenItemMetadataAdapter.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapM01RewardDistribution.h"
#include "demo_mapRewardFullMapDistribution.h"

namespace
{
	FName SectionName(Edemo_mapRuntimeContainerSection Section)
	{
		switch (Section)
		{
		case Edemo_mapRuntimeContainerSection::Chest: return TEXT("Chest");
		case Edemo_mapRuntimeContainerSection::Equipment: return TEXT("Equipment");
		case Edemo_mapRuntimeContainerSection::Backpack: return TEXT("Backpack");
		case Edemo_mapRuntimeContainerSection::Body: return TEXT("Body");
		default: return NAME_None;
		}
	}
}

bool Fdemo_mapShanmenItemGeneratedSourceAdapter::BuildRequest(
	const FShanmenContentStamp& ItemContent, const FGuid& OwnerId,
	const FGuid& RunId, FName SlotId, int64 ExpectedSequence, int32 PityState,
	FShanmenItemGeneratedSourceRequest& OutRequest, FString& OutDiagnostic)
{
	OutRequest = FShanmenItemGeneratedSourceRequest();
	auto Fail = [&](const TCHAR* Message) { OutDiagnostic = Message; return false; };
	if (!ItemContent.IsValid() || !OwnerId.IsValid() || !RunId.IsValid() || SlotId.IsNone()
		|| ExpectedSequence < 0 || ExpectedSequence == MAX_int64 || PityState < 0 || PityState > 3)
	{
		return Fail(TEXT("Invalid fresh source context or current product pity state."));
	}
	const auto* M01 = Fdemo_mapM01RewardDistribution::Find(SlotId);
	const auto* P8 = Fdemo_mapRewardFullMapDistribution::Find(SlotId);
	if ((!M01 && !P8) || (M01 && P8)) { return Fail(TEXT("Source slot is absent or ambiguous in the canonical distribution.")); }
	const auto Projection = M01 ? Fdemo_mapM01RewardDistribution::BuildProjection(*M01)
		: Fdemo_mapRewardFullMapDistribution::BuildProjection(*P8);
	if (!Projection.IsValid()) { return Fail(TEXT("Canonical source projection is invalid.")); }
	const auto Result = Fdemo_mapRewardSourceProjectionPlanner::Plan(Projection, RunId, PityState);
	if (!Result.IsSuccess()) { OutDiagnostic = Result.Trace.Diagnostic; return false; }
	const auto Receipt = Fdemo_mapRewardSourceAcceptanceReceipt::FromProjection(Projection, Result);
	if (!Receipt.IsValid()) { return Fail(TEXT("Canonical planner produced an invalid source receipt.")); }
	const auto Seed = Fdemo_mapRewardSourceProjectionPlanner::BuildContainerSeed(Result);
	// The existing product normalizer resolves equipment/backpack slots. Never
	// accept a truncated plan then silently regenerate different materialization.
	if (Seed.Num() != Result.PlannedStacks.Num()) { return Fail(TEXT("Source seed normalization dropped a planned stack.")); }
	FShanmenItemGeneratedSourceRequest Candidate;
	Candidate.ItemContent = ItemContent;
	auto& Plan = Candidate.Plan;
	Plan.OwnerId = OwnerId; Plan.RunId = RunId; Plan.ExpectedSequence = ExpectedSequence;
	Plan.SourceRoleId = Receipt.StableSourceRoleId;
	Plan.Content.Version = Receipt.ContentVersionId; Plan.Content.Digest = Receipt.ContentDigest;
	Plan.SlotId = Receipt.SlotId; Plan.ProjectionId = Receipt.ProjectionId;
	Plan.DistributionProfileId = Receipt.DistributionProfileId; Plan.BudgetProfileId = Receipt.BudgetProfileId;
	Plan.MarkerId = Receipt.MarkerId; Plan.EncounterId = Receipt.EncounterId;
	Plan.JackpotPolicyId = Receipt.JackpotPolicyId; Plan.RareExtremePolicyId = Receipt.RareExtremePolicyId;
	Plan.AffixPolicyId = Receipt.AffixPolicyId; Plan.EffectiveSeed = Receipt.EffectiveSeed;
	Plan.RandomizedBudget = Receipt.RandomizedBudget; Plan.GeneratedTotalValue = Receipt.GeneratedTotalValue;
	Plan.ResidualValue = Receipt.ResidualValue;
	Plan.bPityCommitRequired = Receipt.bPityCommitRequired;
	Plan.PityStateBefore = PityState;
	Plan.PityStateAfter = Receipt.bPityCommitRequired ? Receipt.PityStateOut : PityState;
	Plan.bFallbackUsed = Receipt.bFallbackUsed;
	Plan.bLegacyCompatibilityView = Receipt.bLegacyCompatibilityView;
	for (int32 Index = 0; Index < Seed.Num(); ++Index)
	{
		const auto& Stack = Result.PlannedStacks[Index];
		const auto& Placement = Seed[Index];
		auto& Entry = Plan.Entries.AddDefaulted_GetRef();
		if (Placement.DefinitionId != Stack.DefinitionId || Placement.StackCount != Stack.StackCount
			|| !Fdemo_mapShanmenItemDefinitionAdapter::Build(Stack.DefinitionId, Entry.Definition)
			|| !Fdemo_mapShanmenItemMetadataAdapter::FromPlannedStack(Stack, Entry.RewardMetadata, OutDiagnostic))
		{
			return Fail(TEXT("Source stack is not a canonical authority definition/metadata value."));
		}
		Entry.Quantity = Stack.StackCount; Entry.UnitValue = Stack.UnitValue; Entry.TotalValue = Stack.TotalValue;
		Entry.SectionId = SectionName(Placement.Section); Entry.SlotIndex = Placement.SlotIndex;
		const auto* Definition = Fdemo_mapItemDefinitions::Find(Stack.DefinitionId);
		if (Definition->CategoryId == Fdemo_mapItemIds::BackpackCategory)
		{
			const auto Capacity = Fdemo_mapItemDefinitions::ResolveSpatialStorageCapacity(Stack.DefinitionId);
			if (!Capacity.bSuccess) { return Fail(TEXT("Invalid source backpack capacity.")); }
			Entry.ChildContainerType = TEXT("Backpack"); Entry.ChildContainerCapacity = Capacity.Capacity;
		}
		else if (Definition->CategoryId == Fdemo_mapItemIds::SpatialRingCategory)
		{
			const auto Capacity = Fdemo_mapItemDefinitions::ResolveSpatialRingCapacity(Stack.DefinitionId);
			if (!Capacity.bSuccess) { return Fail(TEXT("Invalid source ring capacity.")); }
			Entry.ChildContainerType = TEXT("SpatialRing"); Entry.ChildContainerCapacity = Capacity.Capacity;
		}
	}
	if (!Plan.IsValid()) { return Fail(TEXT("Canonical source cannot form a complete authority plan.")); }
	OutRequest = MoveTemp(Candidate);
	OutDiagnostic.Reset();
	return true;
}
