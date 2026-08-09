#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "demo_mapEntityLoadoutPresenter.h"
#include "demo_mapItemAuthority.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapProfileRepository.h"

namespace
{
	FGuid AddOneR6(
		FAutomationTestBase& Test,
		Fdemo_mapItemAuthority& Authority,
		FName DefinitionId)
	{
		TArray<FGuid> Affected;
		const Fdemo_mapItemOperationResult Result =
			Authority.AddDefinition(DefinitionId, 1, &Affected);
		Test.TestTrue(
			FString::Printf(TEXT("Add %s"), *DefinitionId.ToString()),
			Result.bSuccess && Affected.Num() == 1 && Affected[0].IsValid());
		return Affected.Num() == 1 ? Affected[0] : FGuid();
	}

	FString NewR6ProfileRoot()
	{
		return FPaths::Combine(
			FPaths::ProjectSavedDir(),
			TEXT("Automation"),
			TEXT("Dev.D.UE.0.0.9-XFix1.P1.0.r6"),
			FGuid::NewGuid().ToString(EGuidFormats::Digits),
			TEXT("Profile"));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FP1R6SpatialRingAuthorityTest,
	"demo_map.P1R6.SpatialRing.AuthorityAndCapacity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FP1R6SpatialRingAuthorityTest::RunTest(const FString&)
{
	FString Error;
	TestTrue(TEXT("Five-slot registry validates"),
		Fdemo_mapItemDefinitions::Validate(&Error));
	TestTrue(TEXT("Accessory and spatial ring have independent definitions"),
		Fdemo_mapItemDefinitions::Find(Fdemo_mapItemIds::EvasionCharm)
			->CompatibleSlotIds == TArray<FName>{ Fdemo_mapItemIds::AccessorySlot }
		&& Fdemo_mapItemDefinitions::Find(Fdemo_mapItemIds::WindTalisman)
			->CompatibleSlotIds == TArray<FName>{ Fdemo_mapItemIds::SpatialRingSlot }
		&& Fdemo_mapItemDefinitions::GetEquipmentSlotIds()
			== TArray<FName>{
				Fdemo_mapItemIds::WeaponSlot,
				Fdemo_mapItemIds::ArmorSlot,
				Fdemo_mapItemIds::AccessorySlot,
				Fdemo_mapItemIds::SpatialRingSlot,
				Fdemo_mapItemIds::BackpackSlot });

	Fdemo_mapItemAuthority Authority;
	const FGuid Accessory =
		AddOneR6(*this, Authority, Fdemo_mapItemIds::EvasionCharm);
	const FGuid Ring =
		AddOneR6(*this, Authority, Fdemo_mapItemIds::WindTalisman);
	TestTrue(TEXT("Accessory and ring equip simultaneously"),
		Authority.Equip(Accessory, Fdemo_mapItemIds::AccessorySlot).bSuccess
		&& Authority.Equip(Ring, Fdemo_mapItemIds::SpatialRingSlot).bSuccess
		&& Authority.GetEquippedInstance(Fdemo_mapItemIds::AccessorySlot)
			== Accessory
		&& Authority.GetEquippedInstance(Fdemo_mapItemIds::SpatialRingSlot)
			== Ring);
	const Fdemo_mapInventoryCapacityResult Capacity =
		Authority.GetInventoryCapacityResult();
	TestTrue(TEXT("Ring alone supplies four independent quick cells"),
		Capacity.bSuccess
		&& Capacity.RingQuickCapacity == 4
		&& Capacity.SpatialBagCapacity == 0
		&& Capacity.Capacity == 10);
	TestTrue(TEXT("Authority remains internally valid"),
		Authority.ValidateInvariants(&Error));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FP1R6SpatialRingProfileMigrationTest,
	"demo_map.P1R6.SpatialRing.Schema5Migration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FP1R6SpatialRingProfileMigrationTest::RunTest(const FString&)
{
	Fdemo_mapProfileRepository Repository;
	const Fdemo_mapProfileStorageContext Storage =
		Fdemo_mapProfileStorageContext::ForRoot(NewR6ProfileRoot());
	Fdemo_mapPersistentProfile Profile = Repository.CreateFreshProfile();
	const FGuid LegacyRing = Profile.PermanentStash[2].ItemInstanceId;
	FString Error;
	TestTrue(TEXT("Save current source profile"),
		Repository.SaveProfile(Profile, Storage).IsSuccess());

	FString Json;
	TestTrue(TEXT("Read current source JSON"),
		FFileHelper::LoadFileToString(Json, *Storage.PrimaryPath()));
	const FString RingGuid = LegacyRing.ToString(EGuidFormats::DigitsWithHyphens);
	Json.ReplaceInline(TEXT("\"SchemaVersion\":6"), TEXT("\"SchemaVersion\":5"));
	Json.ReplaceInline(
		TEXT("\"AccessoryItemInstanceId\":\"\""),
		*FString::Printf(
			TEXT("\"AccessoryItemInstanceId\":\"%s\""), *RingGuid));
	Json.ReplaceInline(
		TEXT(",\"SpatialRingItemInstanceId\":\"\""),
		TEXT(""));
	TestTrue(TEXT("Write legacy schema five source"),
		FFileHelper::SaveStringToFile(Json, *Storage.PrimaryPath()));

	const Fdemo_mapProfileLoadResult Loaded =
		Repository.LoadExistingProfile(Storage);
	if (!Loaded.IsSuccess())
	{
		AddError(Loaded.Diagnostic);
	}
	TestTrue(TEXT("Schema five source migrates atomically"),
		Loaded.IsSuccess()
		&& Loaded.Profile.SchemaVersion
			== Fdemo_mapPersistentProfile::CurrentSchemaVersion
		&& Loaded.Profile.PreparationLayout.SpatialRingItemInstanceId
			== LegacyRing
		&& !Loaded.Profile.PreparationLayout.AccessoryItemInstanceId.IsValid()
		&& Repository.ValidateProfile(Loaded.Profile, &Error));
	return true;
}

#endif
