#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "demo_mapItemAuthority.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapShanmenWeaponGuardItemAdapter.h"

namespace
{
	const EAutomationTestFlags ItemAdapterFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;

	FGuid AddOne(
		FAutomationTestBase& Test,
		Fdemo_mapItemAuthority& Authority,
		FName DefinitionId)
	{
		TArray<FGuid> Affected;
		const Fdemo_mapItemOperationResult Added =
			Authority.AddDefinition(DefinitionId, 1, &Affected);
		Test.TestTrue(TEXT("fixture item added"),
			Added.bSuccess && Affected.Num() == 1);
		return Affected.Num() == 1 ? Affected[0] : FGuid();
	}

	Fdemo_mapShanmenWeaponGuardItemResult EquipAndAuthorize(
		FAutomationTestBase& Test,
		Fdemo_mapItemAuthority& Authority,
		FName DefinitionId,
		FGuid& OutItemId)
	{
		OutItemId = AddOne(Test, Authority, DefinitionId);
		Test.TestTrue(TEXT("fixture weapon equipped"),
			OutItemId.IsValid()
				&& Authority.Equip(
					OutItemId,
					Fdemo_mapItemIds::WeaponSlot).bSuccess);
		return Fdemo_mapShanmenWeaponGuardItemAdapter::
			AuthorizeEquippedWeapon(Authority);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapWeaponGuardItemCatalogTest,
	"Shanmen.0_0_10.Product.WeaponGuardItemAdapter.CanonicalCatalog",
	ItemAdapterFlags)

bool Fdemo_mapWeaponGuardItemCatalogTest::RunTest(const FString&)
{
	const TArray<FName> GuardDefinitions = {
		Fdemo_mapItemIds::TrainingBlade,
		Fdemo_mapItemIds::HeavyPracticeBlade,
		Fdemo_mapItemIds::WeaponLevel1,
		Fdemo_mapItemIds::WeaponLevel2,
		Fdemo_mapItemIds::WeaponLevel3,
		Fdemo_mapItemIds::WeaponLevel4
	};
	for (FName DefinitionId : GuardDefinitions)
	{
		const Fdemo_mapItemDefinition* Definition =
			Fdemo_mapItemDefinitions::Find(DefinitionId);
		TestTrue(
			FString::Printf(
				TEXT("%s explicitly authorizes weapon guard"),
				*DefinitionId.ToString()),
			Definition
				&& Definition->HasGameplaySemantic(
					Edemo_mapItemGameplaySemantic::WeaponGuard)
				&& Definition->EquipmentSlotId
					== Fdemo_mapItemIds::WeaponSlot);
	}
	const Fdemo_mapItemDefinition* Armor =
		Fdemo_mapItemDefinitions::Find(Fdemo_mapItemIds::TrainingVest);
	const Fdemo_mapItemDefinition* Thrown =
		Fdemo_mapItemDefinitions::Find(
			Fdemo_mapItemIds::TrainingThrowingKnife);
	TestTrue(TEXT("non-weapons cannot borrow guard semantics"),
		Armor && Thrown
			&& !Armor->HasGameplaySemantic(
				Edemo_mapItemGameplaySemantic::WeaponGuard)
			&& !Thrown->HasGameplaySemantic(
				Edemo_mapItemGameplaySemantic::WeaponGuard));
	TestTrue(TEXT("P11.7 content identity is current"),
		Fdemo_mapItemDefinitions::IsCurrentContentIdentity(
			TEXT("CodeB.Content.0.0.10.P11.7"),
			TEXT("6D01652004E386DC469CB09FF0F3A77110C53841F6AF3F66F5600C3C9B9B4179")));
	TestTrue(TEXT("P7.7 content identity remains historical"),
		Fdemo_mapItemDefinitions::IsKnownContentIdentity(
			TEXT("CodeB.Content.0.0.10.P7.7"),
			TEXT("6C30E84A05386A7986A2344DB8247961E75F0FE2179F41927A0C7DF950F45A00")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapWeaponGuardItemExactAuthorizationTest,
	"Shanmen.0_0_10.Product.WeaponGuardItemAdapter.ExactAuthorization",
	ItemAdapterFlags)

bool Fdemo_mapWeaponGuardItemExactAuthorizationTest::RunTest(const FString&)
{
	Fdemo_mapItemAuthority Authority;
	FGuid WeaponId;
	const auto First = EquipAndAuthorize(
		*this,
		Authority,
		Fdemo_mapItemIds::TrainingBlade,
		WeaponId);
	const auto Replay =
		Fdemo_mapShanmenWeaponGuardItemAdapter::AuthorizeEquippedWeapon(
			Authority);
	TestTrue(TEXT("exact equipped weapon is authorized"),
		First.IsAuthorized()
			&& First.Authorization.GetSourceItemInstanceId() == WeaponId
			&& First.Authorization.GetDefinitionId()
				== Fdemo_mapItemIds::TrainingBlade
			&& First.Authorization.GetAuthorityRevision()
				== Authority.GetAuthorityRevision());
	TestTrue(TEXT("unchanged authority replays exact authorization"),
		Replay.IsAuthorized()
			&& Replay.Authorization == First.Authorization
			&& Fdemo_mapShanmenWeaponGuardItemAdapter::
				IsCurrentAuthorization(Authority, First.Authorization));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapWeaponGuardItemEquipmentFenceTest,
	"Shanmen.0_0_10.Product.WeaponGuardItemAdapter.EquipmentFences",
	ItemAdapterFlags)

bool Fdemo_mapWeaponGuardItemEquipmentFenceTest::RunTest(const FString&)
{
	Fdemo_mapItemAuthority Authority;
	const auto Empty =
		Fdemo_mapShanmenWeaponGuardItemAdapter::AuthorizeEquippedWeapon(
			Authority);
	TestEqual(TEXT("empty slot is rejected"),
		Empty.Status,
		Edemo_mapShanmenWeaponGuardItemStatus::WeaponNotEquipped);
	const FGuid InventoryWeapon = AddOne(
		*this,
		Authority,
		Fdemo_mapItemIds::TrainingBlade);
	const auto InventoryOnly =
		Fdemo_mapShanmenWeaponGuardItemAdapter::AuthorizeEquippedWeapon(
			Authority);
	TestTrue(TEXT("caller-known inventory ID cannot bypass equipment"),
		InventoryWeapon.IsValid()
			&& InventoryOnly.Status
				== Edemo_mapShanmenWeaponGuardItemStatus::WeaponNotEquipped
			&& !InventoryOnly.Authorization.IsValid());
	TestTrue(TEXT("weapon can be equipped then removed"),
		Authority.Equip(
			InventoryWeapon,
			Fdemo_mapItemIds::WeaponSlot).bSuccess
			&& Authority.Unequip(Fdemo_mapItemIds::WeaponSlot).bSuccess);
	const auto Unequipped =
		Fdemo_mapShanmenWeaponGuardItemAdapter::AuthorizeEquippedWeapon(
			Authority);
	TestEqual(TEXT("unequipped item is rejected"),
		Unequipped.Status,
		Edemo_mapShanmenWeaponGuardItemStatus::WeaponNotEquipped);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapWeaponGuardItemRevisionFenceTest,
	"Shanmen.0_0_10.Product.WeaponGuardItemAdapter.RevisionFence",
	ItemAdapterFlags)

bool Fdemo_mapWeaponGuardItemRevisionFenceTest::RunTest(const FString&)
{
	Fdemo_mapItemAuthority Authority;
	FGuid FirstId;
	const auto First = EquipAndAuthorize(
		*this,
		Authority,
		Fdemo_mapItemIds::TrainingBlade,
		FirstId);
	const int32 FirstRevision = Authority.GetAuthorityRevision();
	FGuid SecondId;
	const auto Second = EquipAndAuthorize(
		*this,
		Authority,
		Fdemo_mapItemIds::WeaponLevel1,
		SecondId);
	TestTrue(TEXT("replacement changes exact authorization"),
		First.IsAuthorized() && Second.IsAuthorized()
			&& FirstId != SecondId
			&& Second.Authorization.GetSourceItemInstanceId() == SecondId
			&& Second.Authorization.GetAuthorityRevision() > FirstRevision
			&& First.Authorization.GetAuthorizationId()
				!= Second.Authorization.GetAuthorizationId());
	TestFalse(TEXT("old authorization becomes stale"),
		Fdemo_mapShanmenWeaponGuardItemAdapter::IsCurrentAuthorization(
			Authority,
			First.Authorization));
	TestTrue(TEXT("replacement authorization is current"),
		Fdemo_mapShanmenWeaponGuardItemAdapter::IsCurrentAuthorization(
			Authority,
			Second.Authorization));
	return true;
}

#endif
