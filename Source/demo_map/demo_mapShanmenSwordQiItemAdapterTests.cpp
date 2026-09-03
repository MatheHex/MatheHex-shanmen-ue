#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "demo_mapItemAuthority.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapShanmenSwordQiItemAdapter.h"

namespace
{
	const EAutomationTestFlags SwordQiItemFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;

	FGuid AddSwordQiItem(
		FAutomationTestBase& Test,
		Fdemo_mapItemAuthority& Authority,
		FName DefinitionId)
	{
		TArray<FGuid> AddedIds;
		const Fdemo_mapItemOperationResult Added =
			Authority.AddDefinition(DefinitionId, 1, &AddedIds);
		Test.TestTrue(TEXT("fixture item added"),
			Added.bSuccess && AddedIds.Num() == 1);
		return AddedIds.Num() == 1 ? AddedIds[0] : FGuid();
	}

	Fdemo_mapShanmenSwordQiItemResult EquipAndAuthorizeSwordQi(
		FAutomationTestBase& Test,
		Fdemo_mapItemAuthority& Authority,
		FName DefinitionId,
		FGuid& OutItemId)
	{
		OutItemId = AddSwordQiItem(Test, Authority, DefinitionId);
		Test.TestTrue(TEXT("fixture sword equipped"),
			OutItemId.IsValid()
				&& Authority.Equip(
					OutItemId,
					Fdemo_mapItemIds::WeaponSlot).bSuccess);
		return Fdemo_mapShanmenSwordQiItemAdapter::
			AuthorizeEquippedSword(Authority);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordQiItemCatalogTest,
	"Shanmen.0_0_10.Product.SwordQiItemAdapter.CanonicalCatalog",
	SwordQiItemFlags)

bool Fdemo_mapSwordQiItemCatalogTest::RunTest(const FString&)
{
	const TArray<FName> SwordDefinitions = {
		Fdemo_mapItemIds::TrainingBlade,
		Fdemo_mapItemIds::HeavyPracticeBlade,
		Fdemo_mapItemIds::WeaponLevel1,
		Fdemo_mapItemIds::WeaponLevel2,
		Fdemo_mapItemIds::WeaponLevel3,
		Fdemo_mapItemIds::WeaponLevel4
	};
	for (FName DefinitionId : SwordDefinitions)
	{
		const Fdemo_mapItemDefinition* Definition =
			Fdemo_mapItemDefinitions::Find(DefinitionId);
		TestTrue(
			FString::Printf(
				TEXT("%s explicitly authorizes Sword Qi"),
				*DefinitionId.ToString()),
			Definition
				&& Definition->HasGameplaySemantic(
					Edemo_mapItemGameplaySemantic::SwordQiSource)
				&& Definition->EquipmentSlotId
					== Fdemo_mapItemIds::WeaponSlot
				&& Definition->CompatibleSlotIds
					== TArray<FName>({ Fdemo_mapItemIds::WeaponSlot }));
	}
	const Fdemo_mapItemDefinition* Armor =
		Fdemo_mapItemDefinitions::Find(Fdemo_mapItemIds::TrainingVest);
	const Fdemo_mapItemDefinition* Thrown =
		Fdemo_mapItemDefinitions::Find(
			Fdemo_mapItemIds::TrainingThrowingKnife);
	TestTrue(TEXT("non-weapons cannot borrow Sword Qi semantics"),
		Armor && Thrown
			&& !Armor->HasGameplaySemantic(
				Edemo_mapItemGameplaySemantic::SwordQiSource)
			&& !Thrown->HasGameplaySemantic(
				Edemo_mapItemGameplaySemantic::SwordQiSource));
	TestTrue(TEXT("P18.4 identity names the Sword Qi catalog change"),
		Fdemo_mapItemDefinitions::IsCurrentContentIdentity(
			TEXT("CodeB.Content.0.0.10.P18.4"),
			TEXT("D6EF276B3BB268D33A9E1242DC3620A7F33F2B4B966503E1056BA3E381C7B043")));
	TestTrue(TEXT("P17.0 identity remains historical evidence"),
		Fdemo_mapItemDefinitions::IsKnownContentIdentity(
			TEXT("CodeB.Content.0.0.10.P17.0"),
			TEXT("5C09D58AA2EB206FA39F2BE896F7B071149CE8FE3D4E75780CFE213432638399")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordQiItemExactAuthorizationTest,
	"Shanmen.0_0_10.Product.SwordQiItemAdapter.ExactAuthorization",
	SwordQiItemFlags)

bool Fdemo_mapSwordQiItemExactAuthorizationTest::RunTest(const FString&)
{
	Fdemo_mapItemAuthority Authority;
	FGuid SwordId;
	const Fdemo_mapShanmenSwordQiItemResult First =
		EquipAndAuthorizeSwordQi(
			*this,
			Authority,
			Fdemo_mapItemIds::TrainingBlade,
			SwordId);
	const Fdemo_mapShanmenSwordQiItemResult Replay =
		Fdemo_mapShanmenSwordQiItemAdapter::AuthorizeEquippedSword(
			Authority);
	TestTrue(TEXT("exact equipped sword is authorized"),
		First.IsAuthorized()
			&& First.Authorization.GetSourceItemInstanceId() == SwordId
			&& First.Authorization.GetDefinitionId()
				== Fdemo_mapItemIds::TrainingBlade
			&& First.Authorization.GetAuthorityRevision()
				== Authority.GetAuthorityRevision());
	TestTrue(TEXT("unchanged authority replays exact evidence"),
		Replay.IsAuthorized()
			&& Replay.Authorization == First.Authorization
			&& Fdemo_mapShanmenSwordQiItemAdapter::
				IsCurrentAuthorization(Authority, First.Authorization));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordQiItemEquipmentFenceTest,
	"Shanmen.0_0_10.Product.SwordQiItemAdapter.EquipmentFences",
	SwordQiItemFlags)

bool Fdemo_mapSwordQiItemEquipmentFenceTest::RunTest(const FString&)
{
	Fdemo_mapItemAuthority Authority;
	const Fdemo_mapShanmenSwordQiItemResult Empty =
		Fdemo_mapShanmenSwordQiItemAdapter::AuthorizeEquippedSword(
			Authority);
	TestEqual(TEXT("empty WeaponSlot is rejected"),
		Empty.Status,
		Edemo_mapShanmenSwordQiItemStatus::WeaponNotEquipped);
	const FGuid InventorySword = AddSwordQiItem(
		*this,
		Authority,
		Fdemo_mapItemIds::TrainingBlade);
	const Fdemo_mapShanmenSwordQiItemResult InventoryOnly =
		Fdemo_mapShanmenSwordQiItemAdapter::AuthorizeEquippedSword(
			Authority);
	TestTrue(TEXT("caller-known inventory identity cannot bypass equipment"),
		InventorySword.IsValid()
			&& InventoryOnly.Status
				== Edemo_mapShanmenSwordQiItemStatus::WeaponNotEquipped
			&& !InventoryOnly.Authorization.IsValid());
	TestTrue(TEXT("equipped item can leave the exact slot"),
		Authority.Equip(
			InventorySword,
			Fdemo_mapItemIds::WeaponSlot).bSuccess
			&& Authority.Unequip(Fdemo_mapItemIds::WeaponSlot).bSuccess);
	TestEqual(TEXT("unequipped item is rejected"),
		Fdemo_mapShanmenSwordQiItemAdapter::AuthorizeEquippedSword(
			Authority).Status,
		Edemo_mapShanmenSwordQiItemStatus::WeaponNotEquipped);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordQiItemRevisionFenceTest,
	"Shanmen.0_0_10.Product.SwordQiItemAdapter.RevisionFence",
	SwordQiItemFlags)

bool Fdemo_mapSwordQiItemRevisionFenceTest::RunTest(const FString&)
{
	Fdemo_mapItemAuthority Authority;
	FGuid FirstId;
	const Fdemo_mapShanmenSwordQiItemResult First =
		EquipAndAuthorizeSwordQi(
			*this,
			Authority,
			Fdemo_mapItemIds::TrainingBlade,
			FirstId);
	const int32 FirstRevision = Authority.GetAuthorityRevision();
	FGuid SecondId;
	const Fdemo_mapShanmenSwordQiItemResult Second =
		EquipAndAuthorizeSwordQi(
			*this,
			Authority,
			Fdemo_mapItemIds::WeaponLevel1,
			SecondId);
	TestTrue(TEXT("replacement changes exact Sword Qi evidence"),
		First.IsAuthorized() && Second.IsAuthorized()
			&& FirstId != SecondId
			&& Second.Authorization.GetSourceItemInstanceId() == SecondId
			&& Second.Authorization.GetAuthorityRevision() > FirstRevision
			&& First.Authorization.GetAuthorizationId()
				!= Second.Authorization.GetAuthorizationId());
	TestFalse(TEXT("old authorization becomes stale"),
		Fdemo_mapShanmenSwordQiItemAdapter::IsCurrentAuthorization(
			Authority,
			First.Authorization));
	TestTrue(TEXT("replacement authorization is current"),
		Fdemo_mapShanmenSwordQiItemAdapter::IsCurrentAuthorization(
			Authority,
			Second.Authorization));
	return true;
}

#endif
