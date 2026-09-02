#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "demo_mapAttributeDefinitions.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapProfileRepository.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
	FString NewItemEconomyRoot()
	{
		return FPaths::Combine(
			FPaths::ProjectSavedDir(),
			TEXT("Automation"),
			TEXT("Dev.D.UE.0.0.5.P1.0.r0"),
			TEXT("ItemEconomySchema"),
			FGuid::NewGuid().ToString(EGuidFormats::Digits));
	}

	bool ReadBytesP10(const FString& Path, TArray<uint8>& OutBytes)
	{
		OutBytes.Reset();
		return FFileHelper::LoadFileToArray(OutBytes, *Path);
	}

	bool WriteBytesP10(const FString& Path, const TArray<uint8>& Bytes)
	{
		return FFileHelper::SaveArrayToFile(Bytes, *Path);
	}

	FString BytesToStringP10(const TArray<uint8>& Bytes)
	{
		FUTF8ToTCHAR Converted(reinterpret_cast<const ANSICHAR*>(Bytes.GetData()), Bytes.Num());
		return FString(Converted.Length(), Converted.Get());
	}

	TArray<uint8> StringToBytesP10(const FString& Text)
	{
		FTCHARToUTF8 Converted(*Text);
		TArray<uint8> Bytes;
		Bytes.Append(reinterpret_cast<const uint8*>(Converted.Get()), Converted.Length());
		return Bytes;
	}

	FString SchemaVersionTokenP10(int32 SchemaVersion)
	{
		return FString::Printf(TEXT("\"SchemaVersion\":%d"), SchemaVersion);
	}

	bool DowngradeSchemaBytes(const TArray<uint8>& CurrentBytes, const int32 TargetSchema, TArray<uint8>& OutLegacyBytes)
	{
		FString Json = BytesToStringP10(CurrentBytes);
		const int32 SchemaChanges = Json.ReplaceInline(
			*SchemaVersionTokenP10(Fdemo_mapPersistentProfile::CurrentSchemaVersion),
			*SchemaVersionTokenP10(TargetSchema),
			ESearchCase::CaseSensitive);
		if (SchemaChanges != 1)
		{
			return false;
		}
		OutLegacyBytes = StringToBytesP10(Json);
		return true;
	}

	bool DowngradeSchemaTwoBytes(const TArray<uint8>& SchemaTwoBytes, TArray<uint8>& OutSchemaOneBytes)
	{
		FString Json = BytesToStringP10(SchemaTwoBytes);
		const int32 SchemaChanges = Json.ReplaceInline(
			*SchemaVersionTokenP10(Fdemo_mapPersistentProfile::CurrentSchemaVersion),
			*SchemaVersionTokenP10(1),
			ESearchCase::CaseSensitive);
		const int32 PersistentChanges = Json.ReplaceInline(TEXT(",\"PersistentSpiritStones\":\"0\""), TEXT(""), ESearchCase::CaseSensitive);
		const int32 RiskChanges = Json.ReplaceInline(TEXT(",\"RiskSpiritStones\":\"0\""), TEXT(""), ESearchCase::CaseSensitive);
		if (SchemaChanges != 1 || PersistentChanges != 1 || RiskChanges != 1)
		{
			return false;
		}
		OutSchemaOneBytes = StringToBytesP10(Json);
		return true;
	}

	Fdemo_mapPersistentProfile CreateActiveProfile()
	{
		Fdemo_mapProfileRepository Repository;
		Fdemo_mapPersistentProfile Profile = Repository.CreateFreshProfile();
		Profile.LastSettlementId = FGuid::NewGuid();
		Profile.ActiveRun.bHasActiveRun = true;
		Profile.ActiveRun.ActiveRunId = FGuid::NewGuid();
		Profile.ActiveRun.ActiveRunState = Edemo_mapPersistentActiveRunState::InProgress;
		Profile.ActiveRun.RiskSpiritStones = 0;
		Fdemo_mapPersistentItemRecord Deployed = Profile.PermanentStash[0];
		Profile.PermanentStash.RemoveAt(0);
		Deployed.PersistentDomain = Edemo_mapPersistentDomain::ActiveRun;
		Deployed.EquipmentSlotId = Fdemo_mapItemIds::WeaponSlot;
		Profile.ActiveRun.DeployedItemIds.Add(Deployed.ItemInstanceId);
		Profile.ActiveRun.ActiveRunItems.Add(Deployed);
		return Profile;
	}

	bool ContainsDefinition(const TArray<FName>& Ids, FName Id)
	{
		return Ids.Contains(Id) && Fdemo_mapItemDefinitions::Find(Id) != nullptr;
	}

	const Fdemo_mapItemEffectParameter* FindEffect(const Fdemo_mapItemDefinition& Definition, FName EffectId)
	{
		return Definition.EffectParameters.FindByPredicate([EffectId](const Fdemo_mapItemEffectParameter& Effect)
		{
			return Effect.ParameterId == EffectId;
		});
	}

	bool ExactEffect(FName DefinitionId, FName EffectId, double ExpectedValue)
	{
		const Fdemo_mapItemDefinition* Definition = Fdemo_mapItemDefinitions::Find(DefinitionId);
		const Fdemo_mapItemEffectParameter* Effect = Definition ? FindEffect(*Definition, EffectId) : nullptr;
		return Effect && FMath::IsNearlyEqual(Effect->Value, ExpectedValue);
	}

	struct FEconomyExpectation
	{
		FName Id;
		bool bPurchasable;
		bool bSellable;
		int64 Buy;
		int64 Sell;
	};

	bool FileStateEqualsP10(const FString& Path, bool bExisted, const TArray<uint8>& Before)
	{
		if (IFileManager::Get().FileExists(*Path) != bExisted)
		{
			return false;
		}
		if (!bExisted)
		{
			return true;
		}
		TArray<uint8> After;
		return ReadBytesP10(Path, After) && After == Before;
	}

	void SnapshotTreeMetadata(const FString& Root, TArray<FString>& OutSnapshot)
	{
		OutSnapshot.Reset();
		if (!IFileManager::Get().DirectoryExists(*Root))
		{
			OutSnapshot.Add(TEXT("<absent>"));
			return;
		}
		IFileManager::Get().IterateDirectoryRecursively(*Root, [&Root, &OutSnapshot](const TCHAR* FilenameOrDirectory, bool bIsDirectory)
		{
			if (!bIsDirectory)
			{
				const FString Path(FilenameOrDirectory);
				OutSnapshot.Add(FString::Printf(
					TEXT("%s|%lld|%lld"),
					*Path.RightChop(Root.Len()),
					static_cast<long long>(IFileManager::Get().FileSize(*Path)),
					static_cast<long long>(IFileManager::Get().GetTimeStamp(*Path).GetTicks())));
			}
			return true;
		});
		OutSnapshot.Sort();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FItemEconomySchema01LegacyDefinitions, "demo_map.ItemEconomySchema.01.LegacyDefinitions", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FItemEconomySchema01LegacyDefinitions::RunTest(const FString&)
{
	const TArray<FName> Legacy = {
		Fdemo_mapItemIds::TrainingBlade, Fdemo_mapItemIds::TrainingVest, Fdemo_mapItemIds::WindTalisman,
		Fdemo_mapItemIds::SpiritDust, Fdemo_mapItemIds::IronShard, Fdemo_mapItemIds::AncientToken };
	for (FName Id : Legacy) TestTrue(TEXT("Frozen legacy ID resolves"), Fdemo_mapItemDefinitions::Find(Id) != nullptr);
	const Fdemo_mapItemDefinition* Blade = Fdemo_mapItemDefinitions::Find(Fdemo_mapItemIds::TrainingBlade);
	const Fdemo_mapItemDefinition* Vest = Fdemo_mapItemDefinitions::Find(Fdemo_mapItemIds::TrainingVest);
	const Fdemo_mapItemDefinition* Talisman = Fdemo_mapItemDefinitions::Find(Fdemo_mapItemIds::WindTalisman);
	TestTrue(TEXT("Starter equipment slots match the current five-slot contract"), Blade && Vest && Talisman
		&& Blade->CompatibleSlotIds == TArray<FName>({ Fdemo_mapItemIds::WeaponSlot })
		&& Vest->CompatibleSlotIds == TArray<FName>({ Fdemo_mapItemIds::ArmorSlot })
		&& Talisman->CompatibleSlotIds == TArray<FName>({ Fdemo_mapItemIds::SpatialRingSlot }));
	TestTrue(TEXT("TrainingBlade modifier unchanged"), Blade && Blade->Modifiers.Num() == 1 && Blade->Modifiers[0].AttributeId == Fdemo_mapAttributeIds::AttackPower && Blade->Modifiers[0].Operation == Edemo_mapModifierOperation::Add && FMath::IsNearlyEqual(Blade->Modifiers[0].Value, 1.0f));
	TestTrue(TEXT("TrainingVest modifier unchanged"), Vest && Vest->Modifiers.Num() == 1 && Vest->Modifiers[0].AttributeId == Fdemo_mapAttributeIds::MaxHealth && Vest->Modifiers[0].Operation == Edemo_mapModifierOperation::Add && FMath::IsNearlyEqual(Vest->Modifiers[0].Value, 2.0f));
	TestTrue(TEXT("WindTalisman modifier unchanged"), Talisman && Talisman->Modifiers.Num() == 1 && Talisman->Modifiers[0].AttributeId == Fdemo_mapAttributeIds::MoveSpeed && Talisman->Modifiers[0].Operation == Edemo_mapModifierOperation::Multiply && FMath::IsNearlyEqual(Talisman->Modifiers[0].Value, 1.10f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FItemEconomySchema02NewDefinitionSet, "demo_map.ItemEconomySchema.02.NewDefinitionSet", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FItemEconomySchema02NewDefinitionSet::RunTest(const FString&)
{
	const TArray<FName> Expected = {
		Fdemo_mapItemIds::WeaponLevel1, Fdemo_mapItemIds::WeaponLevel2, Fdemo_mapItemIds::WeaponLevel3, Fdemo_mapItemIds::WeaponLevel4,
		Fdemo_mapItemIds::ArmorRobeLevel1, Fdemo_mapItemIds::ArmorRobeLevel2, Fdemo_mapItemIds::ArmorRobeLevel3, Fdemo_mapItemIds::ArmorRobeLevel4,
		Fdemo_mapItemIds::SpiritGuardRobe,
		Fdemo_mapItemIds::AccessoryLevel1, Fdemo_mapItemIds::AccessoryLevel2, Fdemo_mapItemIds::AccessoryLevel3, Fdemo_mapItemIds::AccessoryLevel4,
		Fdemo_mapItemIds::HeartProtectingMirror,
		Fdemo_mapItemIds::BackpackLevel1, Fdemo_mapItemIds::BackpackLevel2,
		Fdemo_mapItemIds::SpiritWoodLevel1, Fdemo_mapItemIds::SpiritWoodLevel2, Fdemo_mapItemIds::SpiritWoodLevel3,
		Fdemo_mapItemIds::SpiritOreLevel1, Fdemo_mapItemIds::SpiritOreLevel2, Fdemo_mapItemIds::SpiritOreLevel3,
		Fdemo_mapItemIds::HealingPillLevel1, Fdemo_mapItemIds::HealingPillLevel2, Fdemo_mapItemIds::HealingPillLevel3,
		Fdemo_mapItemIds::MeridianStabilizingPillLevel1,
		Fdemo_mapItemIds::TrainingThrowingKnife,
		Fdemo_mapItemIds::SoulBone, Fdemo_mapItemIds::SpiritBone, Fdemo_mapItemIds::DaoBone,
		Fdemo_mapItemIds::InnerCoreLevel5, Fdemo_mapItemIds::InnerCoreLevel10, Fdemo_mapItemIds::InnerCoreLevel15 };
	TestEqual(TEXT("Exactly 33 new IDs"), Expected.Num(), 33);
	TSet<FName> Unique(Expected);
	TestEqual(TEXT("New IDs are unique"), Unique.Num(), 33);
	for (FName Id : Expected) TestTrue(TEXT("New stable ID resolves"), ContainsDefinition(Expected, Id));
	TestEqual(TEXT("Legacy nine plus new thirty-three"), Fdemo_mapItemDefinitions::GetAll().Num(), 42);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FItemEconomySchema03RegistryIntegrity, "demo_map.ItemEconomySchema.03.RegistryIntegrity", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FItemEconomySchema03RegistryIntegrity::RunTest(const FString&)
{
	FString Error;
	TestTrue(TEXT("Registry validation passes"), Fdemo_mapItemDefinitions::Validate(&Error));
	TSet<FName> Ids;
	for (const Fdemo_mapItemDefinition& Definition : Fdemo_mapItemDefinitions::GetAll())
	{
		TestTrue(TEXT("ID, display name, and category present"), !Definition.DefinitionId.IsNone() && !Definition.DisplayName.ToString().IsEmpty() && !Definition.CategoryId.IsNone());
		Ids.Add(Definition.DefinitionId);
	}
	TestEqual(TEXT("All registry IDs unique"), Ids.Num(), Fdemo_mapItemDefinitions::GetAll().Num());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FItemEconomySchema04ShapeMetadata, "demo_map.ItemEconomySchema.04.ShapeMetadata", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FItemEconomySchema04ShapeMetadata::RunTest(const FString&)
{
	for (const Fdemo_mapItemDefinition& Definition : Fdemo_mapItemDefinitions::GetAll())
	{
		TestTrue(TEXT("Level, stack, and 1x1 grid are legal"), Definition.Level >= 0 && Definition.MaxStackSize > 0 && Definition.GridWidth == 1 && Definition.GridHeight == 1);
	}
	TestTrue(TEXT("Runtime slots expose the complete five-slot contract"), Fdemo_mapItemDefinitions::GetEquipmentSlotIds() == TArray<FName>({ Fdemo_mapItemIds::WeaponSlot, Fdemo_mapItemIds::ArmorSlot, Fdemo_mapItemIds::AccessorySlot, Fdemo_mapItemIds::SpatialRingSlot, Fdemo_mapItemIds::BackpackSlot }));
	TestTrue(TEXT("Backpack metadata activates its stable runtime slot"), Fdemo_mapItemDefinitions::Find(Fdemo_mapItemIds::BackpackLevel1)->EquipmentSlotId == Fdemo_mapItemIds::BackpackSlot
		&& Fdemo_mapItemDefinitions::Find(Fdemo_mapItemIds::BackpackLevel1)->CompatibleSlotIds == TArray<FName>({ Fdemo_mapItemIds::BackpackSlot })
		&& Fdemo_mapSpiritStoneRules::BaseInventoryCapacityWithoutBackpack == 6);
	for (FName Id : { Fdemo_mapItemIds::SpiritWoodLevel1, Fdemo_mapItemIds::SpiritOreLevel1, Fdemo_mapItemIds::SoulBone, Fdemo_mapItemIds::InnerCoreLevel15 })
		TestEqual(TEXT("Material/core stack is 99"), Fdemo_mapItemDefinitions::Find(Id)->MaxStackSize, 99);
	for (FName Id : { Fdemo_mapItemIds::HealingPillLevel1, Fdemo_mapItemIds::HealingPillLevel2, Fdemo_mapItemIds::HealingPillLevel3, Fdemo_mapItemIds::MeridianStabilizingPillLevel1, Fdemo_mapItemIds::TrainingThrowingKnife })
		TestEqual(TEXT("Consumable stack is 20"), Fdemo_mapItemDefinitions::Find(Id)->MaxStackSize, 20);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FItemEconomySchema05EconomyValues, "demo_map.ItemEconomySchema.05.EconomyValues", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FItemEconomySchema05EconomyValues::RunTest(const FString&)
{
	const TArray<FEconomyExpectation> Expected = {
		{Fdemo_mapItemIds::TrainingBlade,false,true,0,25},{Fdemo_mapItemIds::TrainingVest,false,true,0,25},{Fdemo_mapItemIds::WindTalisman,false,true,0,25},{Fdemo_mapItemIds::SpiritDust,false,true,0,5},{Fdemo_mapItemIds::IronShard,false,true,0,10},{Fdemo_mapItemIds::AncientToken,false,true,0,20},
		{Fdemo_mapItemIds::WeaponLevel1,true,true,100,50},{Fdemo_mapItemIds::WeaponLevel2,true,true,200,100},{Fdemo_mapItemIds::WeaponLevel3,true,true,400,200},{Fdemo_mapItemIds::WeaponLevel4,true,true,800,400},
		{Fdemo_mapItemIds::ArmorRobeLevel1,true,true,100,50},{Fdemo_mapItemIds::ArmorRobeLevel2,true,true,200,100},{Fdemo_mapItemIds::ArmorRobeLevel3,true,true,400,200},{Fdemo_mapItemIds::ArmorRobeLevel4,true,true,800,400},
		{Fdemo_mapItemIds::AccessoryLevel1,true,true,80,40},{Fdemo_mapItemIds::AccessoryLevel2,false,true,0,80},{Fdemo_mapItemIds::AccessoryLevel3,false,true,0,160},{Fdemo_mapItemIds::AccessoryLevel4,false,true,0,320},
		{Fdemo_mapItemIds::BackpackLevel1,true,true,120,60},{Fdemo_mapItemIds::BackpackLevel2,false,true,0,150},
		{Fdemo_mapItemIds::SpiritWoodLevel1,false,true,0,10},{Fdemo_mapItemIds::SpiritWoodLevel2,false,true,0,25},{Fdemo_mapItemIds::SpiritWoodLevel3,false,true,0,60},
		{Fdemo_mapItemIds::SpiritOreLevel1,false,true,0,15},{Fdemo_mapItemIds::SpiritOreLevel2,false,true,0,35},{Fdemo_mapItemIds::SpiritOreLevel3,false,true,0,80},
		{Fdemo_mapItemIds::HealingPillLevel1,true,true,30,15},{Fdemo_mapItemIds::HealingPillLevel2,true,true,60,30},{Fdemo_mapItemIds::HealingPillLevel3,true,true,120,60},
		{Fdemo_mapItemIds::MeridianStabilizingPillLevel1,true,true,45,22},
		{Fdemo_mapItemIds::TrainingThrowingKnife,true,true,30,15},
		{Fdemo_mapItemIds::SoulBone,false,true,0,100},{Fdemo_mapItemIds::SpiritBone,false,true,0,250},{Fdemo_mapItemIds::DaoBone,false,true,0,600},
		{Fdemo_mapItemIds::InnerCoreLevel5,false,true,0,150},{Fdemo_mapItemIds::InnerCoreLevel10,false,true,0,400},{Fdemo_mapItemIds::InnerCoreLevel15,false,true,0,1000} };
	for (const FEconomyExpectation& Row : Expected)
	{
		const Fdemo_mapItemDefinition* Definition = Fdemo_mapItemDefinitions::Find(Row.Id);
		TestTrue(TEXT("Economy row matches exactly"), Definition && Definition->bPurchasable == Row.bPurchasable && Definition->bSellable == Row.bSellable && Definition->BuyPrice == Row.Buy && Definition->SellPrice == Row.Sell);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FItemEconomySchema06PurchasableOrder, "demo_map.ItemEconomySchema.06.PurchasableOrder", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FItemEconomySchema06PurchasableOrder::RunTest(const FString&)
{
	const TArray<FName> Expected = {
		Fdemo_mapItemIds::WeaponLevel1, Fdemo_mapItemIds::WeaponLevel2, Fdemo_mapItemIds::WeaponLevel3, Fdemo_mapItemIds::WeaponLevel4,
		Fdemo_mapItemIds::ArmorRobeLevel1, Fdemo_mapItemIds::ArmorRobeLevel2, Fdemo_mapItemIds::ArmorRobeLevel3, Fdemo_mapItemIds::ArmorRobeLevel4,
		Fdemo_mapItemIds::AccessoryLevel1,
		Fdemo_mapItemIds::BackpackLevel1, Fdemo_mapItemIds::HealingPillLevel1, Fdemo_mapItemIds::HealingPillLevel2, Fdemo_mapItemIds::HealingPillLevel3,
		Fdemo_mapItemIds::MeridianStabilizingPillLevel1,
		Fdemo_mapItemIds::TrainingThrowingKnife };
	TestTrue(TEXT("Purchasable set and order derive exactly from definitions"), Fdemo_mapItemDefinitions::GetPurchasableDefinitionIds() == Expected);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FItemEconomySchema07EffectValues, "demo_map.ItemEconomySchema.07.EffectValuesDataOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FItemEconomySchema07EffectValues::RunTest(const FString&)
{
	for (int32 Level = 1; Level <= 4; ++Level)
	{
		const FName Weapons[] = { Fdemo_mapItemIds::WeaponLevel1,Fdemo_mapItemIds::WeaponLevel2,Fdemo_mapItemIds::WeaponLevel3,Fdemo_mapItemIds::WeaponLevel4 };
		const FName Robes[] = { Fdemo_mapItemIds::ArmorRobeLevel1,Fdemo_mapItemIds::ArmorRobeLevel2,Fdemo_mapItemIds::ArmorRobeLevel3,Fdemo_mapItemIds::ArmorRobeLevel4 };
		const FName Accessories[] = { Fdemo_mapItemIds::AccessoryLevel1,Fdemo_mapItemIds::AccessoryLevel2,Fdemo_mapItemIds::AccessoryLevel3,Fdemo_mapItemIds::AccessoryLevel4 };
		TestTrue(TEXT("Weapon effect exact"), ExactEffect(Weapons[Level - 1], Fdemo_mapItemEffectIds::AttackBonus, Level));
		TestTrue(TEXT("Robe effects exact"), ExactEffect(Robes[Level - 1], Fdemo_mapItemEffectIds::MaxHealthBonus, Level * 2.0) && ExactEffect(Robes[Level - 1], Fdemo_mapItemEffectIds::FlatDamageReduction, Level));
		TestTrue(TEXT("Accessory effect exact"), ExactEffect(Accessories[Level - 1], Fdemo_mapItemEffectIds::CooldownMultiplier, 1.0 - Level * 0.05));
	}
	TestTrue(TEXT("Backpack effects exact"), ExactEffect(Fdemo_mapItemIds::BackpackLevel1, Fdemo_mapItemEffectIds::TotalCapacity, 36.0) && ExactEffect(Fdemo_mapItemIds::BackpackLevel2, Fdemo_mapItemEffectIds::TotalCapacity, 36.0));
	TestTrue(TEXT("Pill effects exact"), ExactEffect(Fdemo_mapItemIds::HealingPillLevel1, Fdemo_mapItemEffectIds::HealAmount, 1.0) && ExactEffect(Fdemo_mapItemIds::HealingPillLevel2, Fdemo_mapItemEffectIds::HealAmount, 2.0) && ExactEffect(Fdemo_mapItemIds::HealingPillLevel3, Fdemo_mapItemEffectIds::HealAmount, 3.0));
	for (const Fdemo_mapItemDefinition& Definition : Fdemo_mapItemDefinitions::GetAll())
		if (Definition.Level > 0) TestTrue(TEXT("New catalog effects are not wired through gameplay modifiers"), Definition.Modifiers.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FItemEconomySchema08SpiritStoneNotItem, "demo_map.ItemEconomySchema.08.SpiritStoneNotItem", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FItemEconomySchema08SpiritStoneNotItem::RunTest(const FString&)
{
	for (const Fdemo_mapItemDefinition& Definition : Fdemo_mapItemDefinitions::GetAll())
		TestFalse(TEXT("No spirit-stone DefinitionId exists"), Definition.DefinitionId.ToString().Contains(TEXT("SpiritStone"), ESearchCase::IgnoreCase));
	const Fdemo_mapPersistentProfile Fresh = Fdemo_mapProfileRepository().CreateFreshProfile();
	for (const Fdemo_mapPersistentItemRecord& Item : Fresh.PermanentStash)
		TestFalse(TEXT("No spirit-stone ItemInstance exists"), Item.ItemDefinitionId.ToString().Contains(TEXT("SpiritStone"), ESearchCase::IgnoreCase));
	TestEqual(TEXT("Future world pickup value is pure int64 metadata"), Fdemo_mapSpiritStoneRules::FixedWorldPickupValue, static_cast<int64>(20));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FItemEconomySchema09FreshSchemaTwo, "demo_map.ItemEconomySchema.09.FreshSchemaTwo", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FItemEconomySchema09FreshSchemaTwo::RunTest(const FString&)
{
	const Fdemo_mapPersistentProfile Fresh = Fdemo_mapProfileRepository().CreateFreshProfile();
	TestTrue(TEXT("Fresh current Schema and zero balances"), Fresh.SchemaVersion == Fdemo_mapPersistentProfile::CurrentSchemaVersion && Fresh.PersistentSpiritStones == 0 && Fresh.ActiveRun.RiskSpiritStones == 0 && Fresh.TownLevel == 0);
	TestTrue(TEXT("Starter order frozen"), Fresh.PermanentStash.Num() == 3
		&& Fresh.PermanentStash[0].ItemDefinitionId == Fdemo_mapItemIds::TrainingBlade
		&& Fresh.PermanentStash[1].ItemDefinitionId == Fdemo_mapItemIds::TrainingVest
		&& Fresh.PermanentStash[2].ItemDefinitionId == Fdemo_mapItemIds::WindTalisman);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FItemEconomySchema10NoNewFreshInstances, "demo_map.ItemEconomySchema.10.NoNewFreshInstances", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FItemEconomySchema10NoNewFreshInstances::RunTest(const FString&)
{
	const Fdemo_mapPersistentProfile Fresh = Fdemo_mapProfileRepository().CreateFreshProfile();
	for (const Fdemo_mapPersistentItemRecord& Item : Fresh.PermanentStash)
		TestTrue(TEXT("Fresh only grants the legacy starter trio"), Item.ItemDefinitionId == Fdemo_mapItemIds::TrainingBlade || Item.ItemDefinitionId == Fdemo_mapItemIds::TrainingVest || Item.ItemDefinitionId == Fdemo_mapItemIds::WindTalisman);
	TestEqual(TEXT("Exactly three fresh instances"), Fresh.PermanentStash.Num(), 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FItemEconomySchema11SchemaTwoRoundTrip, "demo_map.ItemEconomySchema.11.SchemaTwoRoundTrip", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FItemEconomySchema11SchemaTwoRoundTrip::RunTest(const FString&)
{
	Fdemo_mapProfileRepository Repository;
	const Fdemo_mapProfileStorageContext Storage = Fdemo_mapProfileStorageContext::ForRoot(NewItemEconomyRoot());
	Fdemo_mapPersistentProfile Profile = Repository.CreateFreshProfile();
	Profile.PersistentSpiritStones = MAX_int64;
	const Fdemo_mapProfileSaveResult Save = Repository.SaveProfile(Profile, Storage);
	const Fdemo_mapProfileLoadResult Load = Repository.LoadExistingProfile(Storage);
	TestTrue(TEXT("Schema 3 int64 round trip exact"), Save.IsSuccess() && Load.IsSuccess() && Load.Profile == Profile && Load.Profile.PersistentSpiritStones == MAX_int64);
	TArray<uint8> Bytes; ReadBytesP10(Storage.PrimaryPath(), Bytes);
	TestTrue(TEXT("int64 persists as canonical lossless decimal"), BytesToStringP10(Bytes).Contains(TEXT("\"PersistentSpiritStones\":\"9223372036854775807\"")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FItemEconomySchema12IdleMigration, "demo_map.ItemEconomySchema.12.IdleSchemaOneMigration", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FItemEconomySchema12IdleMigration::RunTest(const FString&)
{
	Fdemo_mapProfileRepository Repository;
	const Fdemo_mapProfileStorageContext Storage = Fdemo_mapProfileStorageContext::ForRoot(NewItemEconomyRoot());
	const Fdemo_mapProfileLoadResult Created = Repository.LoadOrCreateDefaultProfile(Storage);
	const FGuid ProfileId = Created.Profile.ProfileId;
	TArray<FGuid> ItemIds; for (const Fdemo_mapPersistentItemRecord& Item : Created.Profile.PermanentStash) ItemIds.Add(Item.ItemInstanceId);
	TArray<uint8> Current, Legacy; ReadBytesP10(Storage.PrimaryPath(), Current);
	TestTrue(TEXT("Schema 1 fixture created"), DowngradeSchemaTwoBytes(Current, Legacy) && WriteBytesP10(Storage.PrimaryPath(), Legacy));
	const Fdemo_mapProfileLoadResult Migrated = Repository.LoadExistingProfile(Storage);
	TArray<FGuid> AfterIds; for (const Fdemo_mapPersistentItemRecord& Item : Migrated.Profile.PermanentStash) AfterIds.Add(Item.ItemInstanceId);
	TestTrue(TEXT("Idle migration preserves profile and ordered item identities"), Migrated.IsSuccess() && Migrated.Profile.SchemaVersion == Fdemo_mapPersistentProfile::CurrentSchemaVersion && Migrated.Profile.PersistentSpiritStones == 0 && Migrated.Profile.TownLevel == 0 && Migrated.Profile.ProfileId == ProfileId && AfterIds == ItemIds);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FItemEconomySchema13ActiveMigration, "demo_map.ItemEconomySchema.13.ActiveSchemaOneMigration", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FItemEconomySchema13ActiveMigration::RunTest(const FString&)
{
	Fdemo_mapProfileRepository Repository;
	const Fdemo_mapProfileStorageContext Storage = Fdemo_mapProfileStorageContext::ForRoot(NewItemEconomyRoot());
	Fdemo_mapPersistentProfile Source = CreateActiveProfile();
	TestTrue(TEXT("Active fixture valid and saved"), Repository.SaveProfile(Source, Storage).IsSuccess());
	const FGuid RunId = Source.ActiveRun.ActiveRunId; const FGuid SettlementBoundary = Source.LastSettlementId;
	const TArray<FGuid> Deployed = Source.ActiveRun.DeployedItemIds; const TArray<Fdemo_mapPersistentItemRecord> Items = Source.ActiveRun.ActiveRunItems;
	TArray<uint8> Current, Legacy; ReadBytesP10(Storage.PrimaryPath(), Current);
	TestTrue(TEXT("Active Schema 1 fixture created"), DowngradeSchemaTwoBytes(Current, Legacy) && WriteBytesP10(Storage.PrimaryPath(), Legacy));
	const Fdemo_mapProfileLoadResult Migrated = Repository.LoadExistingProfile(Storage);
	TestTrue(TEXT("Active migration preserves run and settlement boundaries"), Migrated.IsSuccess()
		&& Migrated.Profile.ActiveRun.ActiveRunId == RunId
		&& Migrated.Profile.ActiveRun.ActiveRunState == Edemo_mapPersistentActiveRunState::InProgress
		&& Migrated.Profile.ActiveRun.DeployedItemIds == Deployed
		&& Migrated.Profile.ActiveRun.ActiveRunItems == Items
		&& Migrated.Profile.LastSettlementId == SettlementBoundary
		&& Migrated.Profile.ActiveRun.RiskSpiritStones == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FItemEconomySchema14MigrationGeneration, "demo_map.ItemEconomySchema.14.MigrationGenerationOnce", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FItemEconomySchema14MigrationGeneration::RunTest(const FString&)
{
	Fdemo_mapProfileRepository Repository; const Fdemo_mapProfileStorageContext Storage = Fdemo_mapProfileStorageContext::ForRoot(NewItemEconomyRoot());
	const Fdemo_mapProfileLoadResult Created = Repository.LoadOrCreateDefaultProfile(Storage); const int32 Before = Created.Profile.SaveGeneration;
	TArray<uint8> Current, Legacy; ReadBytesP10(Storage.PrimaryPath(), Current); DowngradeSchemaTwoBytes(Current, Legacy); WriteBytesP10(Storage.PrimaryPath(), Legacy);
	const Fdemo_mapProfileLoadResult Migrated = Repository.LoadExistingProfile(Storage);
	TestTrue(TEXT("Successful migration increments exactly once"), Migrated.IsSuccess() && Migrated.Profile.SaveGeneration == Before + 1 && Migrated.CommittedGeneration == Before + 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FItemEconomySchema15MigrationIdempotent, "demo_map.ItemEconomySchema.15.MigrationIdempotent", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FItemEconomySchema15MigrationIdempotent::RunTest(const FString&)
{
	Fdemo_mapProfileRepository Repository; const Fdemo_mapProfileStorageContext Storage = Fdemo_mapProfileStorageContext::ForRoot(NewItemEconomyRoot());
	Repository.LoadOrCreateDefaultProfile(Storage); TArray<uint8> Current, Legacy; ReadBytesP10(Storage.PrimaryPath(), Current); DowngradeSchemaTwoBytes(Current, Legacy); WriteBytesP10(Storage.PrimaryPath(), Legacy);
	const Fdemo_mapProfileLoadResult Migrated = Repository.LoadExistingProfile(Storage); TArray<uint8> Before; ReadBytesP10(Storage.PrimaryPath(), Before);
	const Fdemo_mapProfileLoadResult A = Repository.LoadExistingProfile(Storage); const Fdemo_mapProfileLoadResult B = Repository.LoadExistingProfile(Storage); TArray<uint8> After; ReadBytesP10(Storage.PrimaryPath(), After);
	TestTrue(TEXT("Repeated migrated legacy loads are byte and generation idempotent"), Migrated.IsSuccess() && A.IsSuccess() && B.IsSuccess() && A.Profile.SaveGeneration == Migrated.Profile.SaveGeneration && B.Profile == A.Profile && Before == After);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FItemEconomySchema16MigrationFailureAtomic, "demo_map.ItemEconomySchema.16.MigrationFailureAtomic", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FItemEconomySchema16MigrationFailureAtomic::RunTest(const FString&)
{
	Fdemo_mapProfileRepository Repository; Fdemo_mapProfileStorageContext Storage = Fdemo_mapProfileStorageContext::ForRoot(NewItemEconomyRoot());
	Repository.LoadOrCreateDefaultProfile(Storage); TArray<uint8> Current, Legacy; ReadBytesP10(Storage.PrimaryPath(), Current); DowngradeSchemaTwoBytes(Current, Legacy); WriteBytesP10(Storage.PrimaryPath(), Legacy); WriteBytesP10(Storage.BackupPath(), Legacy);
	const TArray<uint8> PrimaryBefore = Legacy, BackupBefore = Legacy;
	Storage.InjectedFailure = Edemo_mapProfileFailureStage::WriteTemp; const Fdemo_mapProfileLoadResult Failed = Repository.LoadExistingProfile(Storage);
	TArray<uint8> PrimaryAfter, BackupAfter; ReadBytesP10(Storage.PrimaryPath(), PrimaryAfter); ReadBytesP10(Storage.BackupPath(), BackupAfter);
	TestTrue(TEXT("Pre-write migration failure preserves both Schema 1 files and never creates fresh"), Failed.Status == Edemo_mapProfileLoadStatus::WriteRecoveryFailed && PrimaryAfter == PrimaryBefore && BackupAfter == BackupBefore);
	Storage.InjectedFailure = Edemo_mapProfileFailureStage::None; const Fdemo_mapProfileLoadResult Retry = Repository.LoadExistingProfile(Storage);
	TestTrue(TEXT("Preserved legacy primary remains migratable"), Retry.IsSuccess() && Retry.Profile.SchemaVersion == Fdemo_mapPersistentProfile::CurrentSchemaVersion && Retry.Profile.TownLevel == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FItemEconomySchema17CorruptNoFresh, "demo_map.ItemEconomySchema.17.CorruptNoFreshFallback", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FItemEconomySchema17CorruptNoFresh::RunTest(const FString&)
{
	Fdemo_mapProfileRepository Repository; const Fdemo_mapProfileStorageContext Storage = Fdemo_mapProfileStorageContext::ForRoot(NewItemEconomyRoot());
	Repository.LoadOrCreateDefaultProfile(Storage); IFileManager::Get().Delete(*Storage.BackupPath(), false, true, true);
	const TArray<uint8> Corrupt = {0x7b,0x22,0x78,0x22,0x3a,0xff}; WriteBytesP10(Storage.PrimaryPath(), Corrupt);
	const Fdemo_mapProfileLoadResult Load = Repository.LoadOrCreateDefaultProfile(Storage); TArray<uint8> After; ReadBytesP10(Storage.PrimaryPath(), After);
	TestTrue(TEXT("Corrupt primary without backup is rejected without Fresh"), Load.Status == Edemo_mapProfileLoadStatus::CorruptPrimaryNoValidBackup && !Load.IsSuccess() && After == Corrupt);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FItemEconomySchema18FutureReadOnly, "demo_map.ItemEconomySchema.18.FutureSchemaReadOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FItemEconomySchema18FutureReadOnly::RunTest(const FString&)
{
	Fdemo_mapProfileRepository Repository; const Fdemo_mapProfileStorageContext Storage = Fdemo_mapProfileStorageContext::ForRoot(NewItemEconomyRoot());
	Repository.LoadOrCreateDefaultProfile(Storage); TArray<uint8> Bytes; ReadBytesP10(Storage.PrimaryPath(), Bytes); FString Json = BytesToStringP10(Bytes);
	TestEqual(TEXT("Future fixture token replaced"), Json.ReplaceInline(
		*SchemaVersionTokenP10(Fdemo_mapPersistentProfile::CurrentSchemaVersion),
		*SchemaVersionTokenP10(Fdemo_mapPersistentProfile::CurrentSchemaVersion + 1),
		ESearchCase::CaseSensitive), 1);
	const TArray<uint8> Future = StringToBytesP10(Json); WriteBytesP10(Storage.PrimaryPath(), Future);
	const Fdemo_mapProfileLoadResult Load = Repository.LoadExistingProfile(Storage); TArray<uint8> After; ReadBytesP10(Storage.PrimaryPath(), After);
	TestTrue(TEXT("Future schema is rejected read-only"), Load.Status == Edemo_mapProfileLoadStatus::FutureSchemaRejected && After == Future);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FItemEconomySchema20SchemaThreeTownMigration, "demo_map.ItemEconomySchema.20.SchemaThreeTownMigration", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FItemEconomySchema20SchemaThreeTownMigration::RunTest(const FString&)
{
	Fdemo_mapProfileRepository Repository;
	const Fdemo_mapProfileStorageContext Storage = Fdemo_mapProfileStorageContext::ForRoot(NewItemEconomyRoot());
	Fdemo_mapPersistentProfile Source = Repository.CreateFreshProfile();
	Source.PersistentSpiritStones = 777;
	TestTrue(TEXT("Schema 4 profile saved"), Repository.SaveProfile(Source, Storage).IsSuccess());
	TArray<uint8> Bytes;
	ReadBytesP10(Storage.PrimaryPath(), Bytes);
	FString LegacyJson = BytesToStringP10(Bytes);
	TestEqual(TEXT("Schema 3 fixture version replaced"), LegacyJson.ReplaceInline(
		*SchemaVersionTokenP10(Fdemo_mapPersistentProfile::CurrentSchemaVersion),
		*SchemaVersionTokenP10(3),
		ESearchCase::CaseSensitive), 1);
	TestEqual(TEXT("Schema 3 fixture has no TownLevel field"), LegacyJson.ReplaceInline(TEXT(",\"TownLevel\":0"), TEXT(""), ESearchCase::CaseSensitive), 1);
	TestTrue(TEXT("Schema 3 fixture written"), WriteBytesP10(Storage.PrimaryPath(), StringToBytesP10(LegacyJson)));
	const Fdemo_mapProfileLoadResult Migrated = Repository.LoadExistingProfile(Storage);
	TestTrue(TEXT("Schema 3 migration preserves resources and supplies TownLevel 0"),
		Migrated.IsSuccess()
			&& Migrated.Profile.SchemaVersion == Fdemo_mapPersistentProfile::CurrentSchemaVersion
			&& Migrated.Profile.PersistentSpiritStones == Source.PersistentSpiritStones
			&& Migrated.Profile.TownLevel == 0
			&& Migrated.Profile.PermanentStash == Source.PermanentStash);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FItemEconomySchema21SchemaTwoMigration, "demo_map.ItemEconomySchema.21.SchemaTwoMigration", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FItemEconomySchema21SchemaTwoMigration::RunTest(const FString&)
{
	Fdemo_mapProfileRepository Repository;
	const Fdemo_mapProfileStorageContext Storage = Fdemo_mapProfileStorageContext::ForRoot(NewItemEconomyRoot());
	const Fdemo_mapProfileLoadResult Created = Repository.LoadOrCreateDefaultProfile(Storage);
	TArray<uint8> Current, Legacy;
	TestTrue(TEXT("Schema 2 fixture created"), ReadBytesP10(Storage.PrimaryPath(), Current)
		&& DowngradeSchemaBytes(Current, 2, Legacy)
		&& WriteBytesP10(Storage.PrimaryPath(), Legacy));
	const Fdemo_mapProfileLoadResult Migrated = Repository.LoadExistingProfile(Storage);
	TestTrue(TEXT("Schema 2 migration preserves persistent items and clears unavailable layout fields"),
		Migrated.IsSuccess()
			&& Migrated.Profile.SchemaVersion == Fdemo_mapPersistentProfile::CurrentSchemaVersion
			&& Migrated.Profile.PersistentSpiritStones == Created.Profile.PersistentSpiritStones
			&& Migrated.Profile.PermanentStash == Created.Profile.PermanentStash
			&& Migrated.Profile.PreparationLayout == Fdemo_mapPersistentPreparationLayout());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FItemEconomySchema22SchemaFourMigration, "demo_map.ItemEconomySchema.22.SchemaFourMigration", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FItemEconomySchema22SchemaFourMigration::RunTest(const FString&)
{
	Fdemo_mapProfileRepository Repository;
	const Fdemo_mapProfileStorageContext Storage = Fdemo_mapProfileStorageContext::ForRoot(NewItemEconomyRoot());
	Fdemo_mapPersistentProfile Created = Repository.CreateFreshProfile();
	Created.PersistentSpiritStones = 404;
	Created.TownLevel = 4;
	TestTrue(TEXT("Nonzero Schema 4 source saved"),
		Repository.SaveProfile(Created, Storage).IsSuccess());
	TArray<uint8> Current, Legacy;
	TestTrue(TEXT("Schema 4 fixture created"), ReadBytesP10(Storage.PrimaryPath(), Current)
		&& DowngradeSchemaBytes(Current, 4, Legacy)
		&& WriteBytesP10(Storage.PrimaryPath(), Legacy));
	const Fdemo_mapProfileLoadResult Migrated = Repository.LoadExistingProfile(Storage);
	TestTrue(TEXT("Schema 4 migration preserves nonzero current-era state"),
		Migrated.IsSuccess()
			&& Migrated.Profile.SchemaVersion == Fdemo_mapPersistentProfile::CurrentSchemaVersion
			&& Migrated.Profile.PersistentSpiritStones == Created.PersistentSpiritStones
			&& Migrated.Profile.TownLevel == Created.TownLevel
			&& Migrated.Profile.PermanentStash == Created.PermanentStash
			&& Migrated.Profile.ShopStock == Created.ShopStock);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FItemEconomySchema23SchemaFiveAndSixTownMigration, "demo_map.ItemEconomySchema.23.SchemaFiveAndSixTownMigration", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FItemEconomySchema23SchemaFiveAndSixTownMigration::RunTest(const FString&)
{
	for (const int32 TargetSchema : { 5, 6 })
	{
		Fdemo_mapProfileRepository Repository;
		const FString Root = NewItemEconomyRoot();
		const Fdemo_mapProfileStorageContext Storage =
			Fdemo_mapProfileStorageContext::ForRoot(Root);
		Fdemo_mapPersistentProfile Source = Repository.CreateFreshProfile();
		Source.PersistentSpiritStones = 500 + TargetSchema;
		Source.TownLevel = TargetSchema - 1;
		TestTrue(
			FString::Printf(TEXT("Schema %d nonzero source saved"), TargetSchema),
			Repository.SaveProfile(Source, Storage).IsSuccess());

		TArray<uint8> Current;
		TArray<uint8> Legacy;
		TestTrue(
			FString::Printf(TEXT("Schema %d fixture created"), TargetSchema),
			ReadBytesP10(Storage.PrimaryPath(), Current)
				&& DowngradeSchemaBytes(Current, TargetSchema, Legacy)
				&& WriteBytesP10(Storage.PrimaryPath(), Legacy));
		const Fdemo_mapProfileLoadResult Migrated =
			Repository.LoadExistingProfile(Storage);
		TestTrue(
			FString::Printf(
				TEXT("Schema %d preserves nonzero TownLevel and persistent state"),
				TargetSchema),
			Migrated.IsSuccess()
				&& Migrated.Profile.SchemaVersion
					== Fdemo_mapPersistentProfile::CurrentSchemaVersion
				&& Migrated.Profile.TownLevel == Source.TownLevel
				&& Migrated.Profile.PersistentSpiritStones
					== Source.PersistentSpiritStones
				&& Migrated.Profile.PermanentStash == Source.PermanentStash);
		IFileManager::Get().DeleteDirectory(*Root, false, true);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FItemEconomySchema19IllegalSpiritStones, "demo_map.ItemEconomySchema.19.IllegalSpiritStoneJson", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FItemEconomySchema19IllegalSpiritStones::RunTest(const FString&)
{
	for (const FString& Invalid : { FString(TEXT("-1")), FString(TEXT("9223372036854775808")), FString(TEXT("1.5")), FString(TEXT("01")) })
	{
		Fdemo_mapProfileRepository Repository; const Fdemo_mapProfileStorageContext Storage = Fdemo_mapProfileStorageContext::ForRoot(NewItemEconomyRoot());
		Repository.LoadOrCreateDefaultProfile(Storage); TArray<uint8> Bytes; ReadBytesP10(Storage.PrimaryPath(), Bytes); FString Json = BytesToStringP10(Bytes);
		const FString Replacement = FString::Printf(TEXT("\"PersistentSpiritStones\":\"%s\""), *Invalid);
		TestEqual(TEXT("Illegal fixture field replaced"), Json.ReplaceInline(TEXT("\"PersistentSpiritStones\":\"0\""), *Replacement, ESearchCase::CaseSensitive), 1);
		const TArray<uint8> Illegal = StringToBytesP10(Json); WriteBytesP10(Storage.PrimaryPath(), Illegal);
		const Fdemo_mapProfileLoadResult Load = Repository.LoadExistingProfile(Storage); TArray<uint8> After; ReadBytesP10(Storage.PrimaryPath(), After);
		TestTrue(TEXT("Illegal spirit-stone JSON rejected and untouched"), Load.Status == Edemo_mapProfileLoadStatus::CorruptPrimaryNoValidBackup && After == Illegal);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FItemEconomySchema20ProtectedIsolation, "demo_map.ItemEconomySchema.20.ProtectedIsolation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FItemEconomySchema20ProtectedIsolation::RunTest(const FString&)
{
	const FString ProjectRoot = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
	const TArray<FString> ProtectedTrees = {
		FPaths::Combine(ProjectRoot, TEXT("Content")),
		FPaths::Combine(ProjectRoot, TEXT("Config")),
		FPaths::Combine(ProjectRoot, TEXT("Plugins")) };
	TArray<TArray<FString>> BeforeTrees;
	for (const FString& Tree : ProtectedTrees) { TArray<FString> Snapshot; SnapshotTreeMetadata(Tree, Snapshot); BeforeTrees.Add(Snapshot); }
	const Fdemo_mapProfileStorageContext Production = Fdemo_mapProfileStorageContext::Production();
	const TArray<FString> ProductionPaths = { Production.PrimaryPath(), Production.BackupPath(), Production.TempPath() };
	TArray<bool> Existed; TArray<TArray<uint8>> ProductionBytes;
	for (const FString& Path : ProductionPaths) { const bool bExists = IFileManager::Get().FileExists(*Path); Existed.Add(bExists); TArray<uint8> Bytes; if (bExists) ReadBytesP10(Path, Bytes); ProductionBytes.Add(Bytes); }
	const FString UprojectPath = FPaths::Combine(ProjectRoot, TEXT("demo_map.uproject")); TArray<uint8> UprojectBefore; ReadBytesP10(UprojectPath, UprojectBefore);

	Fdemo_mapProfileRepository Repository; const Fdemo_mapPersistentProfile Fresh = Repository.CreateFreshProfile(); FString Error;
	TestTrue(TEXT("Default V3-facing Profile remains unchanged while Runtime exposes five slots"), Repository.ValidateProfile(Fresh, &Error) && Fdemo_mapItemDefinitions::GetEquipmentSlotIds().Num() == 5 && Fresh.PermanentStash.Num() == 3);

	for (int32 Index = 0; Index < ProtectedTrees.Num(); ++Index) { TArray<FString> After; SnapshotTreeMetadata(ProtectedTrees[Index], After); TestTrue(TEXT("Protected tree metadata unchanged"), After == BeforeTrees[Index]); }
	for (int32 Index = 0; Index < ProductionPaths.Num(); ++Index) TestTrue(TEXT("Production save bytes/existence unchanged"), FileStateEqualsP10(ProductionPaths[Index], Existed[Index], ProductionBytes[Index]));
	TArray<uint8> UprojectAfter; ReadBytesP10(UprojectPath, UprojectAfter); TestTrue(TEXT("Project descriptor and map-bearing Content are untouched"), UprojectAfter == UprojectBefore);
	return true;
}

#endif
