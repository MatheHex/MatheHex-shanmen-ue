#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "demo_mapAttributeComponent.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapItemSubsystem.h"
#include "Engine/GameInstance.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"

namespace
{
	Udemo_mapItemSubsystem* NewPreparedItems()
	{
		UGameInstance* Owner = NewObject<UGameInstance>(GetTransientPackage());
		return NewObject<Udemo_mapItemSubsystem>(Owner);
	}

	Fdemo_mapCommittedRunLoadoutPlan NewPlan()
	{
		Fdemo_mapCommittedRunLoadoutPlan Plan;
		Plan.ProfileId = FGuid::NewGuid();
		Plan.CommittedGeneration = 1;
		Plan.ActiveRunId = FGuid::NewGuid();
		return Plan;
	}

	Fdemo_mapPersistentItemRecord Item(FName DefinitionId, int32 Count, FName SlotId = NAME_None)
	{
		Fdemo_mapPersistentItemRecord Record;
		Record.ItemInstanceId = FGuid::NewGuid();
		Record.ItemDefinitionId = DefinitionId;
		Record.StackCount = Count;
		Record.PersistentDomain = Edemo_mapPersistentDomain::ActiveRun;
		Record.EquipmentSlotId = SlotId;
		return Record;
	}

	void Add(Fdemo_mapCommittedRunLoadoutPlan& Plan, const Fdemo_mapPersistentItemRecord& Record)
	{
		Plan.OrderedItems.Add(Record);
		Plan.DeployedItemIds.Add(Record.ItemInstanceId);
	}

	Fdemo_mapPreparedRunRuntimeResult Materialize(Udemo_mapItemSubsystem* Items, const Fdemo_mapCommittedRunLoadoutPlan& Plan)
	{
		Fdemo_mapPreparedRunRuntimeRequest Request;
		Request.CommittedPlan = Plan;
		return Items->MaterializePreparedRun(Request);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPreparedRuntime01, "demo_map.PreparedRunRuntime.01.EmptyPlan", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPreparedRuntime01::RunTest(const FString&)
{
	auto* Items = NewPreparedItems(); const auto Plan = NewPlan(); const auto Result = Materialize(Items, Plan);
	TestTrue(TEXT("empty committed plan materialized"), Result.IsMaterialized()); TestEqual(TEXT("existing run ID reused"), Items->GetActiveRunId(), Plan.ActiveRunId); TestEqual(TEXT("no deployed IDs"), Items->GetDeployedItemIds().Num(), 0); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPreparedRuntime02, "demo_map.PreparedRunRuntime.02.ThreeEquipmentPreserveIdentity", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPreparedRuntime02::RunTest(const FString&)
{
	auto Plan = NewPlan(); const auto Weapon = Item(Fdemo_mapItemIds::TrainingBlade, 1, Fdemo_mapItemIds::WeaponSlot); const auto Armor = Item(Fdemo_mapItemIds::TrainingVest, 1, Fdemo_mapItemIds::ArmorSlot); const auto Accessory = Item(Fdemo_mapItemIds::EvasionCharm, 1, Fdemo_mapItemIds::AccessorySlot);
	Add(Plan, Accessory); Add(Plan, Weapon); Add(Plan, Armor); auto* Items = NewPreparedItems(); TestTrue(TEXT("three equipment materialized"), Materialize(Items, Plan).IsMaterialized());
	for (const auto& Record : Plan.OrderedItems) { const auto* Runtime = Items->GetAuthority().FindInstance(Record.ItemInstanceId); TestTrue(TEXT("exact deployed instance exists"), Runtime && Runtime->DefinitionId == Record.ItemDefinitionId && Runtime->Quantity == Record.StackCount && !Runtime->OriginRunId.IsValid()); }
	TestEqual(TEXT("weapon exact ID"), Items->GetAuthority().GetEquippedInstance(Fdemo_mapItemIds::WeaponSlot), Weapon.ItemInstanceId); TestEqual(TEXT("armor exact ID"), Items->GetAuthority().GetEquippedInstance(Fdemo_mapItemIds::ArmorSlot), Armor.ItemInstanceId); TestEqual(TEXT("accessory exact ID"), Items->GetAuthority().GetEquippedInstance(Fdemo_mapItemIds::AccessorySlot), Accessory.ItemInstanceId); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPreparedRuntime03, "demo_map.PreparedRunRuntime.03.ThreeCompleteMaterialStacks", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPreparedRuntime03::RunTest(const FString&)
{
	auto Plan = NewPlan(); Add(Plan, Item(Fdemo_mapItemIds::SpiritDust, 5)); Add(Plan, Item(Fdemo_mapItemIds::IronShard, 5)); Add(Plan, Item(Fdemo_mapItemIds::SpiritDust, 5)); auto* Items = NewPreparedItems(); TestTrue(TEXT("three full stacks"), Materialize(Items, Plan).IsMaterialized()); TestEqual(TEXT("three inventory slots"), Items->GetAuthority().GetUsedInventorySlots(), 3); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPreparedRuntime04, "demo_map.PreparedRunRuntime.04.MaximumSixRecords", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPreparedRuntime04::RunTest(const FString&)
{
	auto Plan = NewPlan(); Add(Plan, Item(Fdemo_mapItemIds::TrainingBlade, 1, Fdemo_mapItemIds::WeaponSlot)); Add(Plan, Item(Fdemo_mapItemIds::TrainingVest, 1, Fdemo_mapItemIds::ArmorSlot)); Add(Plan, Item(Fdemo_mapItemIds::EvasionCharm, 1, Fdemo_mapItemIds::AccessorySlot)); Add(Plan, Item(Fdemo_mapItemIds::SpiritDust, 5)); Add(Plan, Item(Fdemo_mapItemIds::IronShard, 5)); Add(Plan, Item(Fdemo_mapItemIds::SpiritDust, 5)); auto* Items = NewPreparedItems(); TestTrue(TEXT("six records"), Materialize(Items, Plan).IsMaterialized()); TestEqual(TEXT("six exact IDs"), Items->GetDeployedItemIds().Num(), 6); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPreparedRuntime05, "demo_map.PreparedRunRuntime.05.DeployedAndAcquiredRiskPredicate", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPreparedRuntime05::RunTest(const FString&)
{
	auto Plan = NewPlan(); const auto Deployed = Item(Fdemo_mapItemIds::SpiritDust, 5); Add(Plan, Deployed); auto* Items = NewPreparedItems(); Materialize(Items, Plan); TArray<FGuid> Added; Items->AddDefinition(Fdemo_mapItemIds::AncientToken, 1, &Added);
	TestTrue(TEXT("deployed original at risk"), Items->IsItemAtRiskInActiveRun(Deployed.ItemInstanceId)); TestTrue(TEXT("acquired item at risk"), Added.Num() == 1 && Items->IsItemAtRiskInActiveRun(Added[0])); TestFalse(TEXT("unknown ID not at risk"), Items->IsItemAtRiskInActiveRun(FGuid::NewGuid())); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPreparedRuntime06, "demo_map.PreparedRunRuntime.06.DuplicateDeployedIdRejected", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPreparedRuntime06::RunTest(const FString&)
{
	auto Plan = NewPlan(); const auto Record = Item(Fdemo_mapItemIds::SpiritDust, 5); Add(Plan, Record); Plan.DeployedItemIds.Add(Record.ItemInstanceId); auto* Items = NewPreparedItems(); const auto Result = Materialize(Items, Plan); TestTrue(TEXT("duplicate rejected"), !Result.IsMaterialized() && Result.Status == Edemo_mapPreparedRunRuntimeStatus::DuplicateOrMissingDeployedId); TestEqual(TEXT("runtime unchanged"), Items->GetAuthority().GetInstanceSnapshot().Num(), 0); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPreparedRuntime07, "demo_map.PreparedRunRuntime.07.MissingDeployedIdRejected", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPreparedRuntime07::RunTest(const FString&)
{
	auto Plan = NewPlan(); Plan.OrderedItems.Add(Item(Fdemo_mapItemIds::IronShard, 5)); auto* Items = NewPreparedItems(); const auto Result = Materialize(Items, Plan); TestTrue(TEXT("missing rejected"), Result.Status == Edemo_mapPreparedRunRuntimeStatus::DuplicateOrMissingDeployedId); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPreparedRuntime08, "demo_map.PreparedRunRuntime.08.UnknownDefinitionAndQuantityRejected", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPreparedRuntime08::RunTest(const FString&)
{
	auto Unknown = NewPlan(); Add(Unknown, Item(TEXT("Prototype.Item.Unknown"), 1)); auto* A = NewPreparedItems(); TestTrue(TEXT("unknown definition"), Materialize(A, Unknown).Status == Edemo_mapPreparedRunRuntimeStatus::UnknownDefinition);
	auto Quantity = NewPlan(); Add(Quantity, Item(Fdemo_mapItemIds::SpiritDust, 6)); auto* B = NewPreparedItems(); TestTrue(TEXT("quantity"), Materialize(B, Quantity).Status == Edemo_mapPreparedRunRuntimeStatus::QuantityRejected); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPreparedRuntime09, "demo_map.PreparedRunRuntime.09.WrongSlotRejected", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPreparedRuntime09::RunTest(const FString&)
{
	auto Plan = NewPlan(); Add(Plan, Item(Fdemo_mapItemIds::TrainingBlade, 1, Fdemo_mapItemIds::ArmorSlot)); auto* Items = NewPreparedItems(); TestTrue(TEXT("wrong slot"), Materialize(Items, Plan).Status == Edemo_mapPreparedRunRuntimeStatus::SlotCompatibilityRejected); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPreparedRuntime10, "demo_map.PreparedRunRuntime.10.CapacityAndNonIdleRejected", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPreparedRuntime10::RunTest(const FString&)
{
	auto TooMany = NewPlan(); for (int32 N = 0; N < 7; ++N) Add(TooMany, Item(Fdemo_mapItemIds::SpiritDust, 5)); auto* A = NewPreparedItems(); TestTrue(TEXT("base six-cell material capacity"), Materialize(A, TooMany).Status == Edemo_mapPreparedRunRuntimeStatus::CapacityRejected);
	auto* B = NewPreparedItems(); B->AddDefinition(Fdemo_mapItemIds::AncientToken, 1); TestTrue(TEXT("non-idle"), Materialize(B, NewPlan()).Status == Edemo_mapPreparedRunRuntimeStatus::RuntimeNotIdle); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPreparedRuntime11, "demo_map.PreparedRunRuntime.11.InjectedMidMutationRollback", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPreparedRuntime11::RunTest(const FString&)
{
	auto Plan = NewPlan(); Add(Plan, Item(Fdemo_mapItemIds::TrainingBlade, 1, Fdemo_mapItemIds::WeaponSlot)); Add(Plan, Item(Fdemo_mapItemIds::SpiritDust, 5)); auto* Items = NewPreparedItems(); Items->SetPreparedRunFailureAfterMutationForAutomation(1); const auto Result = Materialize(Items, Plan);
	TestTrue(TEXT("injected failure returned"), Result.Status == Edemo_mapPreparedRunRuntimeStatus::AuthorityMutationRejected); TestEqual(TEXT("all instances rolled back"), Items->GetAuthority().GetInstanceSnapshot().Num(), 0); TestTrue(TEXT("run identity rolled back"), !Items->GetActiveRunId().IsValid()); TestEqual(TEXT("deployed set rolled back"), Items->GetDeployedItemIds().Num(), 0); TestTrue(TEXT("runtime idle"), Items->GetRunState() == Edemo_mapRunState::Inactive); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPreparedRuntime12, "demo_map.PreparedRunRuntime.12.SecondMaterializationRejected", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPreparedRuntime12::RunTest(const FString&)
{
	auto* Items = NewPreparedItems(); const auto First = NewPlan(); TestTrue(TEXT("first"), Materialize(Items, First).IsMaterialized()); TestTrue(TEXT("second"), Materialize(Items, NewPlan()).Status == Edemo_mapPreparedRunRuntimeStatus::RuntimeNotIdle); TestEqual(TEXT("first run remains"), Items->GetActiveRunId(), First.ActiveRunId); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPreparedRuntime13, "demo_map.PreparedRunRuntime.13.ExtractionPreservesDeployedIds", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPreparedRuntime13::RunTest(const FString&)
{
	auto Plan = NewPlan(); const auto Weapon = Item(Fdemo_mapItemIds::TrainingBlade, 1, Fdemo_mapItemIds::WeaponSlot); const auto Material = Item(Fdemo_mapItemIds::SpiritDust, 5); Add(Plan, Weapon); Add(Plan, Material); auto* Items = NewPreparedItems(); Materialize(Items, Plan); Fdemo_mapSettlementSummary Summary; TestTrue(TEXT("extract"), Items->RequestSettlement(Edemo_mapRunEndReason::Extraction, Summary).bSuccess); TestTrue(TEXT("weapon same ID secured"), Items->GetAuthority().GetSessionStashSnapshot().Contains(Weapon.ItemInstanceId)); TestTrue(TEXT("material same ID secured"), Items->GetAuthority().GetSessionStashSnapshot().Contains(Material.ItemInstanceId)); TestEqual(TEXT("six units secured"), Summary.SecuredItemCount, 6); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPreparedRuntime14, "demo_map.PreparedRunRuntime.14.DeathAndAbandonLoseDeployed", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPreparedRuntime14::RunTest(const FString&)
{
	for (Edemo_mapRunEndReason Reason : { Edemo_mapRunEndReason::Death, Edemo_mapRunEndReason::Abandon }) { auto Plan = NewPlan(); Add(Plan, Item(Fdemo_mapItemIds::IronShard, 5)); auto* Items = NewPreparedItems(); Materialize(Items, Plan); Fdemo_mapSettlementSummary Summary; TestTrue(TEXT("terminal succeeds"), Items->RequestSettlement(Reason, Summary).bSuccess); TestEqual(TEXT("deployed lost"), Summary.LostItemCount, 5); TestEqual(TEXT("destroyed compacted"), Items->GetAuthority().GetInstanceSnapshot().Num(), 0); } return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPreparedRuntime15, "demo_map.PreparedRunRuntime.15.WorldLeftDeployedIsLost", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPreparedRuntime15::RunTest(const FString&)
{
	Fdemo_mapItemAuthority Authority; const FGuid RunId = FGuid::NewGuid(); const FGuid ItemId = FGuid::NewGuid(); TSet<FGuid> Deployed{ItemId}; TestTrue(TEXT("register"), Authority.MaterializeDeployedInstance(ItemId, Fdemo_mapItemIds::SpiritDust, 5, NAME_None).bSuccess); TestTrue(TEXT("move to world"), Authority.MoveInventoryToWorld(ItemId).bSuccess); TArray<Fdemo_mapSettlementItemRow> Rows; TestTrue(TEXT("extract settles"), Authority.SettleRunItems(RunId, Deployed, true, Rows).bSuccess); TestTrue(TEXT("world-left lost"), Rows.Num() == 1 && Rows[0].FinalOwnership == Edemo_mapItemOwnershipState::Destroyed); TestEqual(TEXT("compacted"), Authority.CompactDestroyedRun(RunId, Deployed), 1); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPreparedRuntime16, "demo_map.PreparedRunRuntime.16.FirstEventWins", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPreparedRuntime16::RunTest(const FString&)
{
	auto Plan = NewPlan(); Add(Plan, Item(Fdemo_mapItemIds::SpiritDust, 5)); auto* Items = NewPreparedItems(); Materialize(Items, Plan); Fdemo_mapSettlementSummary First, Second; TestTrue(TEXT("first"), Items->RequestSettlement(Edemo_mapRunEndReason::Extraction, First).bSuccess); const auto Again = Items->RequestSettlement(Edemo_mapRunEndReason::Death, Second); TestTrue(TEXT("second rejected"), !Again.bSuccess && Again.Code == Edemo_mapItemResultCode::SettlementAlreadyCompleted); TestTrue(TEXT("reason immutable"), Items->GetLastSettlementSummary().Reason == Edemo_mapRunEndReason::Extraction); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPreparedRuntime17, "demo_map.PreparedRunRuntime.17.PawnRebindModifierIdempotency", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPreparedRuntime17::RunTest(const FString&)
{
	auto Plan = NewPlan(); const auto Weapon = Item(Fdemo_mapItemIds::TrainingBlade, 1, Fdemo_mapItemIds::WeaponSlot); Add(Plan, Weapon); auto* Items = NewPreparedItems(); auto* A = NewObject<Udemo_mapAttributeComponent>(GetTransientPackage()); auto* B = NewObject<Udemo_mapAttributeComponent>(GetTransientPackage()); Items->BindAttributeComponent(A); TestTrue(TEXT("materialize with modifier"), Materialize(Items, Plan).IsMaterialized()); const FName Source = Udemo_mapItemSubsystem::MakeModifierSourceId(Weapon.ItemInstanceId); TestEqual(TEXT("one source on A"), A->GetModifierCountBySource(Source), 1); TestTrue(TEXT("same bind idempotent"), Items->BindAttributeComponent(A)); TestEqual(TEXT("still one on A"), A->GetModifierCountBySource(Source), 1); TestTrue(TEXT("rebind B"), Items->BindAttributeComponent(B)); TestEqual(TEXT("removed from A"), A->GetModifierCountBySource(Source), 0); TestEqual(TEXT("one on B"), B->GetModifierCountBySource(Source), 1); Fdemo_mapSettlementSummary Summary; Items->RequestSettlement(Edemo_mapRunEndReason::Death, Summary); TestEqual(TEXT("terminal cleanup"), B->GetModifierCountBySource(Source), 0); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPreparedRuntime18, "demo_map.PreparedRunRuntime.18.LegacyBeginRunAndProductionSaveIsolation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPreparedRuntime18::RunTest(const FString&)
{
	const FString ProductionRoot = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("SaveGames"), TEXT("Shanmen")); const bool ExistedBefore = IFileManager::Get().DirectoryExists(*ProductionRoot); auto* Legacy = NewPreparedItems(); TestTrue(TEXT("legacy begin unchanged"), Legacy->BeginRun().bSuccess); TestTrue(TEXT("legacy allocates run"), Legacy->GetActiveRunId().IsValid()); TestEqual(TEXT("legacy has no deployed originals"), Legacy->GetDeployedItemIds().Num(), 0); TArray<FGuid> Added; Legacy->AddDefinition(Fdemo_mapItemIds::AncientToken, 1, &Added); TestTrue(TEXT("legacy acquisition origin"), Added.Num() == 1 && Legacy->GetAuthority().FindInstance(Added[0])->OriginRunId == Legacy->GetActiveRunId()); TestEqual(TEXT("bridge path never creates production save root"), IFileManager::Get().DirectoryExists(*ProductionRoot), ExistedBefore); return true;
}

#endif
