#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "demo_mapItemSubsystem.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapAttributeComponent.h"
#include "Engine/GameInstance.h"

namespace
{
	Udemo_mapItemSubsystem* NewItems()
	{
		UGameInstance* Owner = NewObject<UGameInstance>(GetTransientPackage());
		return NewObject<Udemo_mapItemSubsystem>(Owner);
	}
	bool AddOne(Udemo_mapItemSubsystem* Items, FName DefinitionId, FGuid& OutId)
	{
		TArray<FGuid> Added; const Fdemo_mapItemOperationResult Result = Items->AddDefinition(DefinitionId, 1, &Added);
		if (!Result.bSuccess || Added.IsEmpty()) return false; OutId = Added[0]; return true;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRunA, "demo_map.V3.Lifecycle.A.BeginRunUniqueIdentity", EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRunA::RunTest(const FString&){auto* I=NewItems();TestTrue(TEXT("begin"),I->BeginRun().bSuccess);const FGuid A=I->GetActiveRunId();Fdemo_mapSettlementSummary S;TestTrue(TEXT("settle"),I->RequestSettlement(Edemo_mapRunEndReason::Abandon,S).bSuccess);TestTrue(TEXT("begin2"),I->BeginRun().bSuccess);TestTrue(TEXT("unique"),A.IsValid()&&I->GetActiveRunId().IsValid()&&A!=I->GetActiveRunId());return true;}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRunB, "demo_map.V3.Lifecycle.B.OriginRunIdentity", EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRunB::RunTest(const FString&){auto* I=NewItems();I->BeginRun();FGuid Id;TestTrue(TEXT("add"),AddOne(I,Fdemo_mapItemIds::TrainingBlade,Id));TestTrue(TEXT("origin"),I->GetAuthority().FindInstance(Id)&&I->GetAuthority().FindInstance(Id)->OriginRunId==I->GetActiveRunId());return true;}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRunC, "demo_map.V3.Lifecycle.C.FixedLootTables", EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRunC::RunTest(const FString&){FString E;TestTrue(TEXT("valid"),Fdemo_mapLootTables::Validate(&E));TestEqual(TEXT("melee"),Fdemo_mapLootTables::Find(Fdemo_mapLootTableIds::EnemyMelee)->operator[](0).Quantity,2);TestEqual(TEXT("ranged"),Fdemo_mapLootTables::Find(Fdemo_mapLootTableIds::EnemyRanged)->operator[](0).DefinitionId,Fdemo_mapItemIds::IronShard);TestEqual(TEXT("heavy"),Fdemo_mapLootTables::Find(Fdemo_mapLootTableIds::EnemyHeavy)->operator[](0).DefinitionId,Fdemo_mapItemIds::AncientToken);return true;}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRunD, "demo_map.V3.Lifecycle.D.TerminalIdempotency", EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRunD::RunTest(const FString&){auto* I=NewItems();I->BeginRun();Fdemo_mapSettlementSummary A,B;TestTrue(TEXT("first"),I->RequestSettlement(Edemo_mapRunEndReason::Death,A).bSuccess);const auto R=I->RequestSettlement(Edemo_mapRunEndReason::Extraction,B);TestTrue(TEXT("second rejected"),!R.bSuccess&&R.Code==Edemo_mapItemResultCode::SettlementAlreadyCompleted);TestEqual(TEXT("reason immutable"),I->GetLastSettlementSummary().Reason,Edemo_mapRunEndReason::Death);return true;}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRunE, "demo_map.V3.Lifecycle.E.AtomicInventoryRollback", EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRunE::RunTest(const FString&){Fdemo_mapItemAuthority A;TestTrue(TEXT("fill"),A.AddDefinition(Fdemo_mapItemIds::TrainingBlade,12).bSuccess);const auto Before=A.CaptureState();const auto R=A.AddDefinition(Fdemo_mapItemIds::TrainingVest,1);TestTrue(TEXT("rejected"),!R.bSuccess&&R.Code==Edemo_mapItemResultCode::InventoryFull);TestTrue(TEXT("unchanged"),A.GetInventorySlotSnapshot()==Before.InventorySlots&&A.GetInstanceSnapshot().Num()==Before.Instances.Num());return true;}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRunF, "demo_map.V3.Lifecycle.F.ForbiddenSourceResult", EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRunF::RunTest(const FString&){auto* I=NewItems();I->BeginRun();TArray<Ademo_mapWorldItem*> A;const auto R=I->CreateEnemyLoot(nullptr,Edemo_mapEnemyLootArchetype::Melee,FGuid::NewGuid(),FVector::ZeroVector,nullptr,A,false);TestTrue(TEXT("forbidden"),!R.bSuccess&&R.Code==Edemo_mapItemResultCode::ForbiddenLootSource);return true;}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRunG, "demo_map.V3.Lifecycle.G.DeathDestroysAll", EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRunG::RunTest(const FString&){auto* I=NewItems();I->BeginRun();FGuid Id;AddOne(I,Fdemo_mapItemIds::TrainingBlade,Id);Fdemo_mapSettlementSummary S;TestTrue(TEXT("death"),I->RequestSettlement(Edemo_mapRunEndReason::Death,S).bSuccess);TestEqual(TEXT("lost"),S.LostItemCount,1);TestEqual(TEXT("stash empty"),I->GetSessionStashItemCount(),0);TestEqual(TEXT("compacted"),I->GetAuthority().GetInstanceSnapshot().Num(),0);return true;}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRunH, "demo_map.V3.Lifecycle.H.ExtractionSecuresGuid", EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRunH::RunTest(const FString&){auto* I=NewItems();I->BeginRun();FGuid Id;AddOne(I,Fdemo_mapItemIds::TrainingBlade,Id);Fdemo_mapSettlementSummary S;TestTrue(TEXT("extract"),I->RequestSettlement(Edemo_mapRunEndReason::Extraction,S).bSuccess);TestTrue(TEXT("same guid"),I->GetAuthority().GetSessionStashSnapshot().Contains(Id));TestEqual(TEXT("secured"),S.SecuredValue,100);return true;}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRunI, "demo_map.V3.Lifecycle.I.AbandonDestroysAll", EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRunI::RunTest(const FString&){auto* I=NewItems();I->BeginRun();FGuid Id;AddOne(I,Fdemo_mapItemIds::WindTalisman,Id);Fdemo_mapSettlementSummary S;I->RequestSettlement(Edemo_mapRunEndReason::Abandon,S);TestTrue(TEXT("abandon"),S.bValid&&S.Reason==Edemo_mapRunEndReason::Abandon&&S.LostItemCount==1&&I->GetSessionStashItemCount()==0);return true;}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRunJ, "demo_map.V3.Lifecycle.J.FirstEventWinsTenTimes", EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRunJ::RunTest(const FString&){for(int32 N=0;N<10;++N){auto* I=NewItems();I->BeginRun();Fdemo_mapSettlementSummary A,B;const auto First=I->RequestSettlement(Edemo_mapRunEndReason::Extraction,A);const auto Second=I->RequestSettlement(Edemo_mapRunEndReason::Death,B);if(!TestTrue(TEXT("first/second"),First.bSuccess&&!Second.bSuccess&&A.Reason==Edemo_mapRunEndReason::Extraction))return false;}return true;}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRunK, "demo_map.V3.Lifecycle.K.SessionStashReadOnlyBoundary", EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRunK::RunTest(const FString&){auto* I=NewItems();I->BeginRun();FGuid Id;AddOne(I,Fdemo_mapItemIds::SpiritDust,Id);Fdemo_mapSettlementSummary S;I->RequestSettlement(Edemo_mapRunEndReason::Extraction,S);const auto Equip=I->Equip(Id,Fdemo_mapItemIds::WeaponSlot);TestTrue(TEXT("stash cannot equip"),!Equip.bSuccess&&Equip.Code==Edemo_mapItemResultCode::InvalidOwnership);TestEqual(TEXT("stash preserved"),I->GetSessionStashItemCount(),1);return true;}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRunL, "demo_map.V3.Lifecycle.L.TwentyRoundRegistryCompaction", EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRunL::RunTest(const FString&){auto* I=NewItems();for(int32 N=0;N<20;++N){if(!I->BeginRun().bSuccess)return false;FGuid Id;AddOne(I,Fdemo_mapItemIds::TrainingBlade,Id);Fdemo_mapSettlementSummary S;I->RequestSettlement(Edemo_mapRunEndReason::Death,S);if(!TestEqual(TEXT("registry zero"),I->GetAuthority().GetInstanceSnapshot().Num(),0))return false;}return true;}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRunM, "demo_map.V3.Lifecycle.M.TwentyRoundModifierCleanup", EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRunM::RunTest(const FString&){auto* I=NewItems();auto* A=NewObject<Udemo_mapAttributeComponent>(GetTransientPackage());I->BindAttributeComponent(A);for(int32 N=0;N<20;++N){I->BeginRun();FGuid Id;AddOne(I,Fdemo_mapItemIds::TrainingBlade,Id);if(!I->Equip(Id,Fdemo_mapItemIds::WeaponSlot).bSuccess)return false;Fdemo_mapSettlementSummary S;I->RequestSettlement(Edemo_mapRunEndReason::Death,S);if(!TestEqual(TEXT("modifier zero"),I->GetActiveModifierSources().Num(),0))return false;}return true;}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRunN, "demo_map.V3.Lifecycle.N.ValueAggregation", EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRunN::RunTest(const FString&){auto* I=NewItems();I->BeginRun();I->AddDefinition(Fdemo_mapItemIds::IronShard,2);I->AddDefinition(Fdemo_mapItemIds::AncientToken,1);Fdemo_mapSettlementSummary S;I->RequestSettlement(Edemo_mapRunEndReason::Extraction,S);TestEqual(TEXT("items"),S.SecuredItemCount,3);TestEqual(TEXT("value"),S.SecuredValue,506);TestEqual(TEXT("stash value"),S.StashValueAfter,506);return true;}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRunO, "demo_map.V3.Lifecycle.O.StashSurvivesDeathAndAbandon", EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRunO::RunTest(const FString&){auto* I=NewItems();I->BeginRun();I->AddDefinition(Fdemo_mapItemIds::AncientToken,1);Fdemo_mapSettlementSummary S;I->RequestSettlement(Edemo_mapRunEndReason::Extraction,S);for(Edemo_mapRunEndReason R:{Edemo_mapRunEndReason::Death,Edemo_mapRunEndReason::Abandon}){I->BeginRun();I->AddDefinition(Fdemo_mapItemIds::SpiritDust,1);I->RequestSettlement(R,S);TestEqual(TEXT("stash stable"),I->GetSessionStashValue(),500);}return true;}

#endif
