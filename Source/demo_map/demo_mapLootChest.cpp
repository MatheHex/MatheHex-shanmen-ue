#include "demo_mapLootChest.h"
#include "demo_map.h"
#include "demo_mapSearchContainerTypes.h"
#include "demo_mapFixedLootTableRegistry.h"
#include "demo_mapItemSubsystem.h"
#include "demo_mapRewardGenerator.h"
#include "demo_mapV3ProgressionManager.h"

Ademo_mapLootChest::Ademo_mapLootChest()
{
	SetContainerMeshColor(FLinearColor(0.55f, 0.34f, 0.08f));
}

void Ademo_mapLootChest::ConfigureChest(int32 InChestIndex, FName InChestId)
{
	ChestIndex = InChestIndex;
	ChestId = InChestId;
	LootTableId = NAME_None;
	RewardSourceId = NAME_None;
	RewardProjectionId = NAME_None;
	RewardBudgetProfileId = NAME_None;
	RewardSourceTags.Reset();
	ConfiguredRewardProjection = Fdemo_mapRewardSourceProjection();
	FixedFallbackTableId = NAME_None;
	RewardSourceMode = Edemo_mapRewardSourceMode::FixedTable;
	bAllowFixedFallbackOnFailure = false;
	bUsedFixedFallback = false;
	LastRewardGenerationResult = Fdemo_mapRewardGenerationResult();
	LastProjectionResult = Fdemo_mapRewardSourceProjectionResult();
	SetPlayerDepositAllowed(true);
	SetContainerDisplayLabel(
		FString::Printf(TEXT("CHEST %02d"), FMath::Max(0, ChestIndex) + 1));
}

void Ademo_mapLootChest::ConfigureRewardProjection(
	int32 InChestIndex,
	const Fdemo_mapRewardSourceProjection& Projection)
{
	ConfigureChest(InChestIndex, Projection.MarkerId);
	RewardSourceMode = Edemo_mapRewardSourceMode::GeneratedReward;
	RewardProjectionId = Projection.ProjectionId;
	RewardSourceId = Projection.StableSourceRoleId;
	RewardBudgetProfileId = Projection.BudgetProfileId;
	RewardSourceTags = Projection.SourceTags;
	SetPlayerDepositAllowed(!RewardSourceTags.Contains(
		Fdemo_mapRewardProjectionTagIds::SourceContainerWood)
		&& !RewardSourceTags.Contains(
			Fdemo_mapRewardProjectionTagIds::SourceContainerOre));
	ConfiguredRewardProjection = Projection;
	LootTableId = Projection.FixedFallbackTableId;
	FixedFallbackTableId = Projection.FixedFallbackTableId;
	bAllowFixedFallbackOnFailure =
		Projection.bAllowFixedFallbackOnFailure;
	if (!Projection.SourceDisplayLabel.IsEmpty())
	{
		SetContainerDisplayLabel(Projection.SourceDisplayLabel);
	}
	if (Projection.SourceTags.Contains(
		Fdemo_mapRewardProjectionTagIds::ValueHigh))
	{
		SetContainerMeshColor(FLinearColor(0.62f, 0.18f, 0.82f));
	}
	else if (Projection.SourceTags.Contains(
		Fdemo_mapRewardProjectionTagIds::SourceContainerWood))
	{
		SetContainerMeshColor(FLinearColor(0.18f, 0.52f, 0.16f));
	}
	else if (Projection.SourceTags.Contains(
		Fdemo_mapRewardProjectionTagIds::SourceContainerOre))
	{
		SetContainerMeshColor(FLinearColor(0.16f, 0.42f, 0.68f));
	}
}

void Ademo_mapLootChest::ConfigureFixedChest(
	int32 InChestIndex,
	FName InChestMarkerId,
	FName InLootTableId)
{
	ConfigureChest(InChestIndex, InChestMarkerId);
	LootTableId = InLootTableId;
}

void Ademo_mapLootChest::ConfigureRewardChest(
	int32 InChestIndex,
	const Fdemo_mapRewardSourceDefinition& Source)
{
	ConfigureChest(InChestIndex, Source.MarkerId);
	RewardSourceMode = Source.Mode;
	RewardSourceId = Source.RewardSourceId;
	RewardBudgetProfileId = Source.BudgetProfileId;
	RewardSourceTags = Source.SourceTags;
	SetPlayerDepositAllowed(!RewardSourceTags.Contains(
		Fdemo_mapRewardProjectionTagIds::SourceContainerWood)
		&& !RewardSourceTags.Contains(
			Fdemo_mapRewardProjectionTagIds::SourceContainerOre));
	LootTableId = Source.FixedTableId;
	FixedFallbackTableId = Source.FixedFallbackTableId;
	bAllowFixedFallbackOnFailure = Source.bAllowFixedFallbackOnFailure;
}

bool Ademo_mapLootChest::InitializeChest(
	Ademo_mapV3ProgressionManager* InManager,
	Udemo_mapItemSubsystem* InItems,
	FGuid InRunId)
{
	bUsedFixedFallback = false;
	if (RewardSourceMode == Edemo_mapRewardSourceMode::GeneratedReward)
	{
		if (!InManager
			|| RewardSourceId.IsNone()
			|| RewardBudgetProfileId.IsNone()
			|| RewardSourceTags.IsEmpty()
			|| !InManager->CanGenerateRewardSource(
				InRunId,
				RewardSourceId))
		{
			LastRewardGenerationResult.Status =
				Edemo_mapRewardGenerationStatus::DuplicateSource;
			LastRewardGenerationResult.Trace.RunId = InRunId;
			LastRewardGenerationResult.Trace.LootSourceId =
				RewardSourceId;
			LastRewardGenerationResult.Trace.BudgetProfileId =
				RewardBudgetProfileId;
			LastRewardGenerationResult.Trace.Diagnostic =
				TEXT("Reward source was already committed for this run.");
			return false;
		}

		if (!RewardProjectionId.IsNone())
		{
			const Fdemo_mapRewardSourceProjection* Projection =
				ConfiguredRewardProjection.IsValid()
					&& ConfiguredRewardProjection.ProjectionId
						== RewardProjectionId
				? &ConfiguredRewardProjection
				: Fdemo_mapRewardSourceProjectionRegistry::Find(
					RewardProjectionId);
			if (!Projection)
			{
				LastRewardGenerationResult.Status =
					Edemo_mapRewardGenerationStatus::InvalidRequest;
				LastRewardGenerationResult.Trace.Diagnostic =
					TEXT("Reward Source Projection is missing.");
				return false;
			}
			LastProjectionResult =
				Fdemo_mapRewardSourceProjectionPlanner::Plan(
					*Projection,
					InRunId,
					InManager->GetRewardAffixPityState(InRunId));
			LastRewardGenerationResult.Status =
				LastProjectionResult.Status;
			LastRewardGenerationResult.PlannedStacks =
				LastProjectionResult.PlannedStacks;
			LastRewardGenerationResult.Trace.RunId = InRunId;
			LastRewardGenerationResult.Trace.LootSourceId =
				RewardSourceId;
			LastRewardGenerationResult.Trace.BudgetProfileId =
				RewardBudgetProfileId;
			LastRewardGenerationResult.Trace.EffectiveSeed =
				LastProjectionResult.Trace.EffectiveSeed;
			LastRewardGenerationResult.Trace.MultiplierBps =
				LastProjectionResult.Trace.MultiplierBps;
			LastRewardGenerationResult.Trace.RandomizedBudget =
				LastProjectionResult.Trace.RandomizedBudget;
			LastRewardGenerationResult.Trace.GeneratedTotalValue =
				LastProjectionResult.Trace.GeneratedTotalValue;
			LastRewardGenerationResult.Trace.ResidualValue =
				LastProjectionResult.Trace.ResidualValue;
			LastRewardGenerationResult.Trace.Diagnostic =
				LastProjectionResult.Trace.Diagnostic;
			if (LastProjectionResult.IsSuccess())
			{
				const bool bInitialized = InitializeSearchContainer(
					InManager,
					InItems,
					InRunId,
					Edemo_mapRuntimeContainerKind::Chest,
					RewardSourceId,
					Fdemo_mapRewardSourceProjectionPlanner::
						BuildContainerSeed(LastProjectionResult));
				if (bInitialized
					&& InManager->CommitGeneratedRewardSource(
						InRunId,
						RewardSourceId)
					&& InManager->CommitRewardAffixPity(
						InRunId,
						RewardSourceId,
						LastProjectionResult))
				{
					UE_LOG(
						Logdemo_map,
						Log,
						TEXT("P2_REWARD_SOURCE_PROJECTION: projection=%s role=%s profile=%s budget=%lld generated=%lld residual=%lld fallback=0."),
						*RewardProjectionId.ToString(),
						*RewardSourceId.ToString(),
						*RewardBudgetProfileId.ToString(),
						LastProjectionResult.Trace.RandomizedBudget,
						LastProjectionResult.Trace.GeneratedTotalValue,
						LastProjectionResult.Trace.ResidualValue);
					return true;
				}
				LastRewardGenerationResult.Status =
					Edemo_mapRewardGenerationStatus::MaterializationFailed;
				LastRewardGenerationResult.Trace.Diagnostic =
					TEXT("Projection materialization or ledger commit failed.");
			}
		}
		else
		{
		Fdemo_mapRewardGenerationRequest Request;
		Request.RequestId = FName(*FString::Printf(
			TEXT("P1.Request.%s.%s"),
			*InRunId.ToString(EGuidFormats::Digits),
			*RewardSourceId.ToString()));
		Request.RunId = InRunId;
		Request.LootSourceId = RewardSourceId;
		Request.BudgetProfileId = RewardBudgetProfileId;
		Request.SourceTags = RewardSourceTags;
		Request.StableSeed =
			Fdemo_mapRewardGenerator::ComputeStableSeed(
				InRunId,
				RewardSourceId,
				RewardBudgetProfileId);
		Request.TargetSection =
			Edemo_mapRuntimeContainerSection::Chest;
		Request.TargetCapacity =
			Fdemo_mapSearchContainerPrototypeConfig::ChestPrototypeCapacity;
		LastRewardGenerationResult =
			Fdemo_mapRewardGenerator::Generate(Request);
		if (LastRewardGenerationResult.IsSuccess())
		{
			const bool bInitialized = InitializeSearchContainer(
				InManager,
				InItems,
				InRunId,
				Edemo_mapRuntimeContainerKind::Chest,
				RewardSourceId,
				Fdemo_mapRewardGenerator::BuildContainerSeed(
					LastRewardGenerationResult));
			if (bInitialized
				&& InManager->CommitGeneratedRewardSource(
					InRunId,
					RewardSourceId))
			{
				const Fdemo_mapRewardGenerationTrace& Trace =
					LastRewardGenerationResult.Trace;
				UE_LOG(
					Logdemo_map,
					Log,
					TEXT("P1_REWARD_GENERATION: run=%s source=%s profile=%s seed=%llu base=%lld multiplier_bps=%d budget=%lld generated=%lld residual=%lld stacks=%d fallback=0 status=%d."),
					*InRunId.ToString(EGuidFormats::Digits),
					*RewardSourceId.ToString(),
					*RewardBudgetProfileId.ToString(),
					static_cast<unsigned long long>(Trace.EffectiveSeed),
					Trace.BaseValue,
					Trace.MultiplierBps,
					Trace.RandomizedBudget,
					Trace.GeneratedTotalValue,
					Trace.ResidualValue,
					LastRewardGenerationResult.PlannedStacks.Num(),
					static_cast<int32>(LastRewardGenerationResult.Status));
				return true;
			}
			LastRewardGenerationResult.Status =
				Edemo_mapRewardGenerationStatus::MaterializationFailed;
			LastRewardGenerationResult.Trace.Diagnostic =
				TEXT("Generated Reward Plan could not be atomically materialized and committed.");
		}
		}

		if (!bAllowFixedFallbackOnFailure)
		{
			return false;
		}
		const Fdemo_mapFixedLootTableDefinition* Fallback =
			Fdemo_mapFixedLootTableRegistry::Find(
				FixedFallbackTableId);
		if (!Fallback
			|| Fallback->Kind !=
				Edemo_mapRuntimeContainerKind::Chest)
		{
			return false;
		}
		bUsedFixedFallback = true;
		UE_LOG(
			Logdemo_map,
			Warning,
			TEXT("P1_REWARD_GENERATION: generated plan failed; explicit fixed fallback table=%s diagnostic=%s."),
			*FixedFallbackTableId.ToString(),
			*LastRewardGenerationResult.Trace.Diagnostic);
		return InitializeSearchContainer(
			InManager,
			InItems,
			InRunId,
			Edemo_mapRuntimeContainerKind::Chest,
			RewardSourceId,
			Fallback->Entries);
	}

	const Fdemo_mapFixedLootTableDefinition* FixedTable =
		LootTableId.IsNone()
			? nullptr
			: Fdemo_mapFixedLootTableRegistry::Find(LootTableId);
	if (!LootTableId.IsNone()
		&& (!FixedTable
			|| FixedTable->Kind != Edemo_mapRuntimeContainerKind::Chest))
	{
		return false;
	}
	return InitializeSearchContainer(
		InManager,
		InItems,
		InRunId,
		Edemo_mapRuntimeContainerKind::Chest,
		ChestId,
		FixedTable
			? FixedTable->Entries
			: Fdemo_mapSearchContainerPrototypeConfig::BuildChestSeed(
				ChestIndex));
}

bool Ademo_mapLootChest::HasOpenedPresentation() const
{
	return IsContainerOpened()
		&& GetWorldLabelText().Contains(IsContainerEmpty() ? TEXT("EMPTY") : TEXT("OPENED"));
}
