#include "demo_mapSearchContainerTypes.h"
#include "demo_mapItemDefinitions.h"

Fdemo_mapRuntimeContainerResult Fdemo_mapRuntimeContainerResult::Success(
	FGuid InContainerId,
	int32 InRevisionBefore,
	int32 InRevisionAfter,
	FGuid InEntryId,
	FGuid InItemInstanceId)
{
	Fdemo_mapRuntimeContainerResult Result;
	Result.ContainerId = InContainerId;
	Result.EntryId = InEntryId;
	Result.ItemInstanceId = InItemInstanceId;
	Result.RevisionBefore = InRevisionBefore;
	Result.RevisionAfter = InRevisionAfter;
	return Result;
}

Fdemo_mapRuntimeContainerResult Fdemo_mapRuntimeContainerResult::Failure(
	Edemo_mapRuntimeContainerResultCode InCode,
	const FString& InDiagnostic,
	FGuid InContainerId,
	int32 InRevision,
	FGuid InEntryId)
{
	Fdemo_mapRuntimeContainerResult Result;
	Result.Code = InCode;
	Result.bSuccess = false;
	Result.ContainerId = InContainerId;
	Result.EntryId = InEntryId;
	Result.RevisionBefore = InRevision;
	Result.RevisionAfter = InRevision;
	Result.Diagnostic = InDiagnostic;
	return Result;
}

float Fdemo_mapSearchContainerPrototypeConfig::GetOpenSeconds(
	Edemo_mapRuntimeContainerKind Kind)
{
	return Kind == Edemo_mapRuntimeContainerKind::Chest
		? ChestOpenSeconds
		: CorpseOpenSeconds;
}

float Fdemo_mapSearchContainerPrototypeConfig::GetSearchSeconds(
	Edemo_mapRuntimeContainerKind Kind,
	Edemo_mapRuntimeContainerSection Section)
{
	if (Kind == Edemo_mapRuntimeContainerKind::Chest)
	{
		return Section == Edemo_mapRuntimeContainerSection::Chest
			? ChestEntrySearchSeconds
			: 0.0f;
	}
	switch (Section)
	{
	case Edemo_mapRuntimeContainerSection::Equipment:
		return CorpseEquipmentSearchSeconds;
	case Edemo_mapRuntimeContainerSection::Backpack:
		return CorpseBackpackEntrySearchSeconds;
	case Edemo_mapRuntimeContainerSection::Body:
		return CorpseBodyEntrySearchSeconds;
	default:
		return 0.0f;
	}
}

int32 Fdemo_mapSearchContainerPrototypeConfig::GetSectionCapacity(
	Edemo_mapRuntimeContainerKind Kind,
	Edemo_mapRuntimeContainerSection Section)
{
	if (Kind == Edemo_mapRuntimeContainerKind::Chest)
	{
		return Section == Edemo_mapRuntimeContainerSection::Chest
			? ChestPrototypeCapacity
			: 0;
	}
	switch (Section)
	{
	case Edemo_mapRuntimeContainerSection::Equipment:
		return CorpseEquipmentCapacity;
	case Edemo_mapRuntimeContainerSection::Backpack:
		return CorpseRuntimeBackpackCapacity;
	case Edemo_mapRuntimeContainerSection::Body:
		return CorpseBodyCapacity;
	default:
		return 0;
	}
}

TArray<Edemo_mapRuntimeContainerSection> Fdemo_mapSearchContainerPrototypeConfig::GetSectionOrder(
	Edemo_mapRuntimeContainerKind Kind)
{
	return Kind == Edemo_mapRuntimeContainerKind::Chest
		? TArray<Edemo_mapRuntimeContainerSection>{ Edemo_mapRuntimeContainerSection::Chest }
		: TArray<Edemo_mapRuntimeContainerSection>{
			Edemo_mapRuntimeContainerSection::Equipment,
			Edemo_mapRuntimeContainerSection::Backpack,
			Edemo_mapRuntimeContainerSection::Body };
}

TArray<Fdemo_mapRuntimeContainerSeedEntry> Fdemo_mapSearchContainerPrototypeConfig::BuildChestSeed(
	int32 ChestIndex)
{
	if (ChestIndex != 0)
	{
		return {};
	}
	return {
		{
			Edemo_mapRuntimeContainerSection::Chest,
			0,
			Fdemo_mapItemIds::SpiritWoodLevel1,
			2
		},
		{
			Edemo_mapRuntimeContainerSection::Chest,
			1,
			Fdemo_mapItemIds::HealingPillLevel1,
			1
		}
	};
}

TArray<Fdemo_mapRuntimeContainerSeedEntry> Fdemo_mapSearchContainerPrototypeConfig::BuildPrototypeCorpseSeed()
{
	return {
		{
			Edemo_mapRuntimeContainerSection::Equipment,
			0,
			Fdemo_mapItemIds::WeaponLevel1,
			1
		},
		{
			Edemo_mapRuntimeContainerSection::Backpack,
			0,
			Fdemo_mapItemIds::SpiritOreLevel1,
			2
		},
		{
			Edemo_mapRuntimeContainerSection::Body,
			0,
			Fdemo_mapItemIds::SoulBone,
			1
		}
	};
}
