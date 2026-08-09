#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "demo_mapItemDefinitions.h"
#include "demo_mapM01EnemyTypes.h"
#include "demo_mapProfileRepository.h"
#include "demo_mapProfileSettlementTransaction.h"
#include "demo_mapSpiritStoneTransaction.h"
#include "demo_mapTownProgressionRules.h"
#include "demo_mapTownUpgradeTransaction.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"

namespace
{
	struct FP5EconomyTestRoot
	{
		FString Path = FPaths::Combine(
			FPaths::ProjectSavedDir(),
			TEXT("Automation"),
			TEXT("Dev.D.UE.0.0.9.P5.0.r0"),
			FGuid::NewGuid().ToString(EGuidFormats::Digits));

		~FP5EconomyTestRoot()
		{
			const FString Full = FPaths::ConvertRelativePathToFull(Path);
			const FString Allowed = FPaths::ConvertRelativePathToFull(FPaths::Combine(
				FPaths::ProjectSavedDir(), TEXT("Automation"), TEXT("Dev.D.UE.0.0.9.P5.0.r0")));
			if (Full.StartsWith(Allowed))
			{
				IFileManager::Get().DeleteDirectory(*Full, false, true);
			}
		}

		Fdemo_mapProfileStorageContext Storage() const
		{
			return Fdemo_mapProfileStorageContext::ForRoot(Path);
		}
	};

	Fdemo_mapPersistentItemRecord AddPermanentStack(
		Fdemo_mapPersistentProfile& Profile,
		FName DefinitionId,
		int32 Quantity)
	{
		Fdemo_mapPersistentItemRecord Item;
		Item.ItemInstanceId = FGuid::NewGuid();
		Item.ItemDefinitionId = DefinitionId;
		Item.StackCount = Quantity;
		Item.PersistentDomain = Edemo_mapPersistentDomain::PermanentStash;
		Profile.PermanentStash.Add(Item);
		return Item;
	}

	int32 CountStacks(const Fdemo_mapPersistentProfile& Profile, FName DefinitionId)
	{
		int32 Total = 0;
		for (const Fdemo_mapPersistentItemRecord& Item : Profile.PermanentStash)
		{
			if (Item.PersistentDomain == Edemo_mapPersistentDomain::PermanentStash
				&& Item.ItemDefinitionId == DefinitionId)
			{
				Total += Item.StackCount;
			}
		}
		return Total;
	}

	const Fdemo_mapPersistentItemRecord* FindStack(
		const Fdemo_mapPersistentProfile& Profile,
		const FGuid& Id)
	{
		return Profile.PermanentStash.FindByPredicate(
			[&Id](const Fdemo_mapPersistentItemRecord& Candidate)
			{
				return Candidate.ItemInstanceId == Id;
			});
	}

	Fdemo_mapTownUpgradeIntent MakeTownIntent(const Fdemo_mapPersistentProfile& Profile)
	{
		Fdemo_mapTownUpgradeCost Cost;
		Fdemo_mapTownProgressionRules::TryGetNextLevelCost(Profile.TownLevel, Cost);
		Fdemo_mapTownUpgradeIntent Intent;
		Intent.ExpectedProfileId = Profile.ProfileId;
		Intent.ExpectedSaveGeneration = Profile.SaveGeneration;
		Intent.ExpectedTownLevel = Profile.TownLevel;
		Intent.RequestedNextLevel = Profile.TownLevel + 1;
		Intent.ExpectedSpiritWoodCost = Cost.SpiritWood;
		Intent.ExpectedSpiritOreCost = Cost.SpiritOre;
		Intent.ExpectedSpiritStoneCost = Cost.SpiritStones;
		return Intent;
	}

	Fdemo_mapSpiritStonePickupIntent MakePickupIntent(
		const Fdemo_mapPersistentProfile& Profile,
		FName PickupId,
		FName SourceId,
		int64 Value)
	{
		Fdemo_mapSpiritStonePickupIntent Intent;
		Intent.ExpectedProfileId = Profile.ProfileId;
		Intent.ExpectedSaveGeneration = Profile.SaveGeneration;
		Intent.ExpectedActiveRunId = Profile.ActiveRun.ActiveRunId;
		Intent.PickupId = PickupId;
		Intent.SourceId = SourceId;
		Intent.Value = Value;
		return Intent;
	}

	Fdemo_mapProfileSettlementResult SettleRiskCurrency(
		Fdemo_mapPersistentProfile& Profile,
		Edemo_mapRunEndReason Reason,
		const Fdemo_mapProfileRepository& Repository,
		const Fdemo_mapProfileStorageContext& Storage)
	{
		Fdemo_mapRuntimeSettlementSnapshot RuntimeSnapshot;
		RuntimeSnapshot.ActiveRunId = Profile.ActiveRun.ActiveRunId;
		RuntimeSnapshot.CommittedEndReason = Reason;
		RuntimeSnapshot.bValid = true;
		Fdemo_mapProfileSettlementRequest Request;
		Request.ExpectedProfileId = Profile.ProfileId;
		Request.ExpectedSaveGeneration = Profile.SaveGeneration;
		Request.ExpectedActiveRunId = Profile.ActiveRun.ActiveRunId;
		Request.RequestedEndReason = Reason;
		Request.RuntimeSnapshot = RuntimeSnapshot;
		return Fdemo_mapProfileSettlementTransaction().Execute(
			Profile, Request, Repository, Storage);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapP5TownUpgradeAtomicTest,
	"demo_map.P5.TownUpgrade.AtomicMultiStackAndStaleReject",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapP5TownUpgradeAtomicTest::RunTest(const FString&)
{
	static constexpr int32 ExpectedWood[] = { 30, 40, 50, 60, 70 };
	static constexpr int32 ExpectedOre[] = { 30, 40, 50, 60, 70 };
	static constexpr int64 ExpectedStones[] = { 100, 500, 2500, 12500, 62500 };
	for (int32 Level = 0; Level < Fdemo_mapTownProgressionRules::MaxTownLevel; ++Level)
	{
		Fdemo_mapTownUpgradeCost FrozenCost;
		TestTrue(TEXT("P5 frozen town cost row exists"),
			Fdemo_mapTownProgressionRules::TryGetNextLevelCost(Level, FrozenCost));
		TestTrue(TEXT("P5 frozen town cost row is exact"),
			FrozenCost.SpiritWood == ExpectedWood[Level]
				&& FrozenCost.SpiritOre == ExpectedOre[Level]
				&& FrozenCost.SpiritStones == ExpectedStones[Level]);
	}
	Fdemo_mapTownUpgradeCost NoCost;
	TestFalse(TEXT("P5 town level 5 has no further cost row"),
		Fdemo_mapTownProgressionRules::TryGetNextLevelCost(5, NoCost));

	FP5EconomyTestRoot Root;
	Fdemo_mapProfileRepository Repository;
	const Fdemo_mapProfileStorageContext Storage = Root.Storage();
	Fdemo_mapPersistentProfile Profile = Repository.CreateFreshProfile();
	const Fdemo_mapPersistentItemRecord SpentWood =
		AddPermanentStack(Profile, Fdemo_mapItemIds::SpiritWoodLevel1, 20);
	const Fdemo_mapPersistentItemRecord RetainedWood =
		AddPermanentStack(Profile, Fdemo_mapItemIds::SpiritWoodLevel2, 20);
	const Fdemo_mapPersistentItemRecord ProtectedWood =
		AddPermanentStack(Profile, Fdemo_mapItemIds::SpiritWoodLevel3, 20);
	const Fdemo_mapPersistentItemRecord RetainedOre =
		AddPermanentStack(Profile, Fdemo_mapItemIds::SpiritOreLevel1, 35);
	Profile.PreparationLayout.OrderedRunInventoryItemInstanceIds.Add(
		ProtectedWood.ItemInstanceId);
	Profile.PersistentSpiritStones = 100;
	TestTrue(TEXT("P5 town fixture saves"), Repository.SaveProfile(Profile, Storage).IsSuccess());

	const Fdemo_mapTownUpgradeIntent Intent = MakeTownIntent(Profile);
	const int32 GenerationBefore = Profile.SaveGeneration;
	const Fdemo_mapTownUpgradeResult Result =
		Fdemo_mapTownUpgradeTransaction().Execute(Profile, Intent, Repository, Storage);
	TestTrue(TEXT("P5 first upgrade commits"), Result.IsCommitted());
	TestEqual(TEXT("P5 town increments exactly once"), Profile.TownLevel, 1);
	TestEqual(TEXT("P5 one save generation increments exactly once"),
		Profile.SaveGeneration, GenerationBefore + 1);
	TestEqual(TEXT("P5 wood uses the frozen 30 cost"),
		CountStacks(Profile, Fdemo_mapItemIds::SpiritWoodLevel1)
			+ CountStacks(Profile, Fdemo_mapItemIds::SpiritWoodLevel2)
			+ CountStacks(Profile, Fdemo_mapItemIds::SpiritWoodLevel3), 30);
	TestEqual(TEXT("P5 ore uses the frozen 30 cost"),
		CountStacks(Profile, Fdemo_mapItemIds::SpiritOreLevel1), 5);
	TestEqual(TEXT("P5 persistent stone uses the frozen 100 cost"),
		Profile.PersistentSpiritStones, static_cast<int64>(0));
	TestTrue(TEXT("P5 exhausted stack is removed"), FindStack(Profile, SpentWood.ItemInstanceId) == nullptr);
	const Fdemo_mapPersistentItemRecord* WoodAfter = FindStack(Profile, RetainedWood.ItemInstanceId);
	const Fdemo_mapPersistentItemRecord* OreAfter = FindStack(Profile, RetainedOre.ItemInstanceId);
	TestTrue(TEXT("P5 partially used wood retains its GUID and remainder"),
		WoodAfter && WoodAfter->StackCount == 10);
	const Fdemo_mapPersistentItemRecord* ProtectedWoodAfter =
		FindStack(Profile, ProtectedWood.ItemInstanceId);
	TestTrue(TEXT("P5 keeps referenced material intact when ordinary stacks can pay"),
		ProtectedWoodAfter && ProtectedWoodAfter->StackCount == 20
			&& Profile.PreparationLayout.OrderedRunInventoryItemInstanceIds.Contains(
				ProtectedWood.ItemInstanceId));
	TestTrue(TEXT("P5 partially used ore retains its GUID and remainder"),
		OreAfter && OreAfter->StackCount == 5);

	const Fdemo_mapPersistentProfile AfterCommit = Profile;
	const Fdemo_mapTownUpgradeResult Stale =
		Fdemo_mapTownUpgradeTransaction().Execute(Profile, Intent, Repository, Storage);
	TestEqual(TEXT("P5 repeated old intent is rejected as stale"),
		static_cast<uint8>(Stale.Status),
		static_cast<uint8>(Edemo_mapTownUpgradeStatus::ProfileGenerationMismatch));
	TestTrue(TEXT("P5 stale retry leaves the complete profile unchanged"), Profile == AfterCommit);

	const Fdemo_mapTownUpgradeIntent InsufficientIntent = MakeTownIntent(Profile);
	const Fdemo_mapTownUpgradeResult Insufficient =
		Fdemo_mapTownUpgradeTransaction().Execute(Profile, InsufficientIntent, Repository, Storage);
	TestEqual(TEXT("P5 missing resources reject before mutation"),
		static_cast<uint8>(Insufficient.Status),
		static_cast<uint8>(Edemo_mapTownUpgradeStatus::InsufficientSpiritWood));
	TestTrue(TEXT("P5 insufficient resources leave the complete profile unchanged"), Profile == AfterCommit);

	const Fdemo_mapProfileLoadResult Reloaded = Repository.LoadExistingProfile(Storage);
	TestTrue(TEXT("P5 committed town upgrade persists through reload"),
		Reloaded.IsSuccess() && Reloaded.Profile == AfterCommit);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapP5RiskStoneTransactionTest,
	"demo_map.P5.RiskSpiritStones.ExactlyOnceAndRange",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapP5RiskStoneTransactionTest::RunTest(const FString&)
{
	FP5EconomyTestRoot Root;
	Fdemo_mapProfileRepository Repository;
	const Fdemo_mapProfileStorageContext Storage = Root.Storage();
	Fdemo_mapPersistentProfile Profile = Repository.CreateFreshProfile();
	Profile.ActiveRun.bHasActiveRun = true;
	Profile.ActiveRun.ActiveRunId = FGuid::NewGuid();
	Profile.ActiveRun.ActiveRunState = Edemo_mapPersistentActiveRunState::Prepared;
	TestTrue(TEXT("P5 active run fixture saves"), Repository.SaveProfile(Profile, Storage).IsSuccess());

	const Fdemo_mapSpiritStonePickupIntent Low = MakePickupIntent(
		Profile, TEXT("P5.SpiritStone.Pickup.Low"), TEXT("P5.SpiritStone.Low"), 15);
	const Fdemo_mapSpiritStonePickupResult LowResult =
		Fdemo_mapSpiritStoneTransaction().Execute(Profile, Low, Repository, Storage);
	TestTrue(TEXT("P5 normal lower-bound risk stone commits"), LowResult.IsCommitted());
	TestEqual(TEXT("P5 risk balance updates immediately"), Profile.ActiveRun.RiskSpiritStones, static_cast<int64>(15));

	const Fdemo_mapSpiritStonePickupIntent DuplicateIntent = MakePickupIntent(
		Profile, Low.PickupId, Low.SourceId, Low.Value);
	const Fdemo_mapSpiritStonePickupResult Duplicate =
		Fdemo_mapSpiritStoneTransaction().Execute(Profile, DuplicateIntent, Repository, Storage);
	TestEqual(TEXT("P5 same source cannot be collected twice"),
		static_cast<uint8>(Duplicate.Status),
		static_cast<uint8>(Edemo_mapSpiritStonePickupStatus::DuplicateSource));

	const Fdemo_mapSpiritStonePickupIntent High = MakePickupIntent(
		Profile, TEXT("P5.SpiritStone.Pickup.High"), TEXT("P5.SpiritStone.High"), 120);
	const Fdemo_mapSpiritStonePickupResult HighResult =
		Fdemo_mapSpiritStoneTransaction().Execute(Profile, High, Repository, Storage);
	TestTrue(TEXT("P5 boss upper-bound risk stone commits"), HighResult.IsCommitted());
	TestEqual(TEXT("P5 separate source adds without item stacks"),
		Profile.ActiveRun.RiskSpiritStones, static_cast<int64>(135));
	const Fdemo_mapPersistentProfile AfterValidPickups = Profile;

	const Fdemo_mapSpiritStonePickupIntent Invalid = MakePickupIntent(
		Profile, TEXT("P5.SpiritStone.Pickup.Invalid"), TEXT("P5.SpiritStone.Invalid"), 14);
	const Fdemo_mapSpiritStonePickupResult InvalidResult =
		Fdemo_mapSpiritStoneTransaction().Execute(Profile, Invalid, Repository, Storage);
	TestEqual(TEXT("P5 out-of-contract risk stone is rejected"),
		static_cast<uint8>(InvalidResult.Status),
		static_cast<uint8>(Edemo_mapSpiritStonePickupStatus::ValueRejected));
	TestTrue(TEXT("P5 invalid risk stone leaves the complete profile unchanged"),
		Profile == AfterValidPickups);

	const Fdemo_mapProfileLoadResult Reloaded = Repository.LoadExistingProfile(Storage);
	TestTrue(TEXT("P5 risk balance and consumed source ids persist through reload"),
		Reloaded.IsSuccess()
			&& Reloaded.Profile.ActiveRun.RiskSpiritStones == 135
			&& Reloaded.Profile.ActiveRun.ConsumedSpiritStoneSourceIds.Num() == 2);
	const Fdemo_mapProfileSettlementResult Extracted = SettleRiskCurrency(
		Profile, Edemo_mapRunEndReason::Extraction, Repository, Storage);
	TestTrue(TEXT("P5 Extraction transfers all collected dynamic risk currency once"),
		Extracted.IsCommitted()
			&& Profile.PersistentSpiritStones == 135
			&& Profile.ActiveRun.RiskSpiritStones == 0
			&& Profile.ActiveRun.ConsumedSpiritStoneSourceIds.IsEmpty());

	FP5EconomyTestRoot LossRoot;
	const Fdemo_mapProfileStorageContext LossStorage = LossRoot.Storage();
	Fdemo_mapPersistentProfile LossProfile = Repository.CreateFreshProfile();
	LossProfile.ActiveRun.bHasActiveRun = true;
	LossProfile.ActiveRun.ActiveRunId = FGuid::NewGuid();
	LossProfile.ActiveRun.ActiveRunState = Edemo_mapPersistentActiveRunState::Prepared;
	LossProfile.ActiveRun.RiskSpiritStones = 55;
	TestTrue(TEXT("P5 lost-risk fixture saves"), Repository.SaveProfile(LossProfile, LossStorage).IsSuccess());
	const Fdemo_mapProfileSettlementResult Lost = SettleRiskCurrency(
		LossProfile, Edemo_mapRunEndReason::Death, Repository, LossStorage);
	TestTrue(TEXT("P5 Death clears risk currency without persistent transfer"),
		Lost.IsCommitted()
			&& LossProfile.PersistentSpiritStones == 0
			&& LossProfile.ActiveRun.RiskSpiritStones == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapP5EnemyStoneRollTest,
	"demo_map.P5.EnemySpiritStones.StableRangesAndM01Composition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapP5EnemyStoneRollTest::RunTest(const FString&)
{
	FString DefinitionError;
	const TArray<Fdemo_mapM01EnemyDefinition>& Definitions =
		Fdemo_mapM01EnemyConfig::GetDefinitions();
	TestTrue(TEXT("P5 M01 keeps the frozen 10 normal / 3 elite / 1 boss composition"),
		Fdemo_mapM01EnemyConfig::Validate(&DefinitionError));
	TestEqual(TEXT("P5 M01 keeps exactly fourteen enemy reward occurrences"), Definitions.Num(), 14);

	const FGuid RunId(0xA01B02C3, 0xD04E05F6, 0x0718091A, 0x2B3C4D5E);
	int32 NormalCount = 0;
	int32 EliteCount = 0;
	int32 BossCount = 0;
	int64 MinimumTotal = 0;
	int64 MaximumTotal = 0;
	for (const Fdemo_mapM01EnemyDefinition& Definition : Definitions)
	{
		int64 FirstValue = 0;
		int64 RepeatedValue = 0;
		const bool bResolved = Fdemo_mapTownProgressionRules::TryResolveEnemySpiritStoneValue(
			RunId,
			Definition.EncounterId,
			Definition.RiskTierId,
			Definition.IsElite(),
			Definition.IsBoss(),
			FirstValue);
		const bool bRepeated = Fdemo_mapTownProgressionRules::TryResolveEnemySpiritStoneValue(
			RunId,
			Definition.EncounterId,
			Definition.RiskTierId,
			Definition.IsElite(),
			Definition.IsBoss(),
			RepeatedValue);
		TestTrue(TEXT("P5 every stable M01 source has a deterministic currency roll"),
			bResolved && bRepeated && FirstValue == RepeatedValue);
		if (Definition.IsBoss())
		{
			++BossCount;
			MinimumTotal += Fdemo_mapTownProgressionRules::BossSpiritStoneMin;
			MaximumTotal += Fdemo_mapTownProgressionRules::BossSpiritStoneMax;
			TestTrue(TEXT("P5 Boss roll is inside 80-120"), FirstValue >= 80 && FirstValue <= 120);
		}
		else if (Definition.IsElite())
		{
			++EliteCount;
			MinimumTotal += Fdemo_mapTownProgressionRules::EliteSpiritStoneMin;
			MaximumTotal += Fdemo_mapTownProgressionRules::EliteSpiritStoneMax;
			TestTrue(TEXT("P5 elite roll is inside 35-55"), FirstValue >= 35 && FirstValue <= 55);
		}
		else
		{
			++NormalCount;
			MinimumTotal += Fdemo_mapTownProgressionRules::NormalSpiritStoneMin;
			MaximumTotal += Fdemo_mapTownProgressionRules::NormalSpiritStoneMax;
			TestTrue(TEXT("P5 normal roll is inside 15-25"), FirstValue >= 15 && FirstValue <= 25);
		}
	}
	TestTrue(TEXT("P5 M01 reward composition remains 10/3/1"),
		NormalCount == 10 && EliteCount == 3 && BossCount == 1);
	TestTrue(TEXT("P5 M01 theoretical currency interval is 335-535"),
		MinimumTotal == 335 && MaximumTotal == 535);
	return true;
}

#endif
