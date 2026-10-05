#if WITH_DEV_AUTOMATION_TESTS
#include "ShanmenDemo20WorldCheckpoint.h"
#include "ShanmenDemo20Loadout.h"
#include "ShanmenDemo20Catalog.h"
#include "ShanmenItemAuthorityService.h"
#include "ShanmenDeterministicId.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"

namespace
{
	constexpr auto Flags=EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter;
	FString Root(const TCHAR* Label) { return FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("Automation/Demo20.M2.Entry"),Label,FGuid::NewGuid().ToString(EGuidFormats::Digits)); }
	FShanmenDemo20WorldCheckpoint Fresh(FGuid Run)
	{
		FShanmenDemo20Session Session; Session.BeginExpedition(Run,26.f,.12f);
		FShanmenDemo20WorldCheckpoint C; C.ContentId=C.LegacyContentId(); C.RunSeed=C.SeedForRun(Run); Session.CaptureExpedition(C.Combat); return C;
	}
	FShanmenItemRunFinalizeRequest Terminal(const FShanmenItemAuthoritySnapshot& S, const FShanmenDemo20ActiveLoadout& A, bool Death)
	{
		FShanmenItemRunFinalizeRequest R; R.Context.OwnerId=FShanmenDemo20Catalog::OwnerId(); R.Context.RunId=FShanmenDemo20Catalog::ScopeId(); R.Context.Content=S.Content;
		R.ActiveRunId=A.RunId; R.TerminalReason=Death?EShanmenItemRunTerminalReason::Death:EShanmenItemRunTerminalReason::Extraction;
		R.Context.RequestId=FShanmenDeterministicId::FromCanonicalParts(TEXT("Demo20.Expedition.Terminal.r1"),{A.RunId.ToString(),FString::FromInt(static_cast<int32>(R.TerminalReason))});
		if (!Death) R.SecuredOriginals=A.RemainingOriginals; return R;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemo20ExpeditionCombatTest,"Shanmen.Demo20.Expedition.CombatCheckpointAndGear",Flags)
bool FDemo20ExpeditionCombatTest::RunTest(const FString&)
{
	FShanmenDemo20Session Session;
	TestTrue(TEXT("Formal combat uses stable Run identity"),Session.BeginExpedition(FGuid(20,200,1,1),26.f,.12f));
	Session.SetGuarding(true); TestTrue(TEXT("Canonical layered guard and armor"),Session.ReceiveSentinelStrike(0));
	TestTrue(TEXT("Nonzero health 100 - 18*.25*.88"),FMath::IsNearlyEqual(Session.GetHealth(),96.04f));
	FShanmenDemo20CombatCheckpoint C; TestTrue(TEXT("Capture complete values"),Session.CaptureExpedition(C));
	FShanmenDemo20Session Restart; TestTrue(TEXT("Restore exact revision/sequence, no stale Impact reuse"),Restart.RestoreExpedition(C));
	TestEqual(TEXT("Not full health on restart"),Restart.GetHealth(),Session.GetHealth());
	TestFalse(TEXT("Held input not restored"),Restart.IsGuarding());
	TestTrue(TEXT("Different enemy maximum"),Restart.GetHealth(2)==65.f && Restart.GetHealth(3)==156.f);
	TestTrue(TEXT("Post-restart canonical strike"),Restart.ReceiveSentinelStrike(1));
	TestTrue(TEXT("Independent ordinary ranged damage"),FMath::IsNearlyEqual(Restart.GetHealth(),83.72f));
	Restart.CaptureExpedition(C); TestEqual(TEXT("Sequence continued, not reset"),C.Sequence,static_cast<uint64>(2));
	const auto Before=C; C.Health[0]=101; TestFalse(TEXT("Invalid recovery fails closed"),Restart.RestoreExpedition(C));
	Restart.CaptureExpedition(C); TestEqual(TEXT("Invalid restore retains nonzero health"),C.Health[0],Before.Health[0]);
	TestTrue(TEXT("Early exit available; no full-clear condition"),Restart.TryExtract());
	FShanmenDemo20Session Heavy; Heavy.BeginExpedition(FGuid(20,201,1,1),34.f,.28f); Heavy.StrikeSentinel(0);
	TestEqual(TEXT("Equipped heavy sword affects canonical damage"),Heavy.GetHealth(1),44.f);
	Heavy.ReceiveSentinelStrike(0); TestTrue(TEXT("Fixed leather armor affects damage"),FMath::IsNearlyEqual(Heavy.GetHealth(),87.04f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemo20WorldRoundTripTest,"Shanmen.Demo20.Expedition.WorldCheckpointRoundTrip",Flags)
bool FDemo20WorldRoundTripTest::RunTest(const FString&)
{
	const auto Disk=Root(TEXT("RoundTrip")); const FGuid Run(20,202,1,1); auto C=Fresh(Run); FShanmenDemo20WorldCheckpoint Confirmed; FString Reason;
	TestTrue(TEXT("Initial checkpoint confirmed before item start"),FShanmenDemo20WorldCheckpointStore::Save(Disk,Confirmed,C,Reason));
	FShanmenDemo20Session Session; Session.RestoreExpedition(C.Combat); Session.ReceiveSentinelStrike(0);
	for (int32 I=0; I<3; ++I) { Session.Advance(.4f); Session.StrikeSentinel(0); }
	Session.CaptureExpedition(C.Combat); C.PlayerPosition=FVector(1120,320,80); C.PlayerYaw=42; C.EnemyPositions[1]=FVector(2940,-210,65); C.EnemyClocks[1]=.45f;
	C.WarningTargets[1]=FVector(1000,250,8);
	TestTrue(TEXT("Save damaged player and defeated enemy"),FShanmenDemo20WorldCheckpointStore::Save(Disk,Confirmed,C,Reason));
	FShanmenDemo20WorldCheckpoint Loaded; TestTrue(TEXT("Same Run reopen"),FShanmenDemo20WorldCheckpointStore::Load(Disk,Run,Loaded,Reason));
	TestEqual(TEXT("Same seed"),Loaded.RunSeed,C.RunSeed); TestEqual(TEXT("Same health, not 100"),Loaded.Combat.Health[0],84.16f);
	TestEqual(TEXT("Defeated enemy remains dead"),Loaded.Combat.Health[1],0.f); TestEqual(TEXT("Exact position"),Loaded.PlayerPosition,C.PlayerPosition);
	TestEqual(TEXT("Attack clock retained"),Loaded.EnemyClocks[1],.45f); TestEqual(TEXT("Warning target retained"),Loaded.WarningTargets[1],C.WarningTargets[1]);
	TestFalse(TEXT("Other Run cannot adopt checkpoint"),FShanmenDemo20WorldCheckpointStore::Load(Disk,FGuid(20,203,1,1),Loaded,Reason));
	TArray<uint8> Bytes; FFileHelper::LoadFileToArray(Bytes,*FShanmenDemo20WorldCheckpointStore::Path(Disk,Run));
	Bytes.Last()^=1; FFileHelper::SaveArrayToFile(Bytes,*FShanmenDemo20WorldCheckpointStore::Path(Disk,Run));
	TestFalse(TEXT("Corrupt primary does not use older backup to resurrect enemy"),FShanmenDemo20WorldCheckpointStore::Load(Disk,Run,Loaded,Reason));
	TestTrue(TEXT("Concrete recovery reason"),Reason.Contains(TEXT("不会重置生命"))); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemo20WorldFailureTest,"Shanmen.Demo20.Expedition.WorldCheckpointFailureAndCAS",Flags)
bool FDemo20WorldFailureTest::RunTest(const FString&)
{
	const auto Disk=Root(TEXT("Failure")); const FGuid Run(20,204,1,1); auto C=Fresh(Run); FShanmenDemo20WorldCheckpoint Confirmed; FString Reason;
	FShanmenDemo20WorldCheckpointStore::Save(Disk,Confirmed,C,Reason); const auto Before=Confirmed;
	TArray<uint8> Old; FFileHelper::LoadFileToArray(Old,*FShanmenDemo20WorldCheckpointStore::Path(Disk,Run)); C.PlayerPosition.X=500;
	FShanmenDemo20WorldCheckpointStore::bFailBeforeReplace=true;
	TestFalse(TEXT("Pre-replace failure is not success"),FShanmenDemo20WorldCheckpointStore::Save(Disk,Confirmed,C,Reason));
	FShanmenDemo20WorldCheckpointStore::bFailBeforeReplace=false;
	TArray<uint8> Still; FFileHelper::LoadFileToArray(Still,*FShanmenDemo20WorldCheckpointStore::Path(Disk,Run));
	TestTrue(TEXT("Exact primary bytes preserved"),Old==Still); TestEqual(TEXT("No published generation"),Confirmed.Generation,Before.Generation);
	FShanmenDemo20WorldCheckpointStore::bFailFirstReadAfterReplace=true;
	TestFalse(TEXT("Ambiguous save cannot announce success"),FShanmenDemo20WorldCheckpointStore::Save(Disk,Confirmed,C,Reason));
	TestTrue(TEXT("Exact pending candidate retry adopts confirmed bytes"),FShanmenDemo20WorldCheckpointStore::Save(Disk,Confirmed,C,Reason));
	TestEqual(TEXT("One generation, not two"),Confirmed.Generation,2);
	auto Stale=Before; auto Changed=C; Changed.PlayerPosition.X=800;
	TestFalse(TEXT("Stale generation cannot overwrite confirmed progress"),FShanmenDemo20WorldCheckpointStore::Save(Disk,Stale,Changed,Reason));
	FShanmenDemo20WorldCheckpoint Loaded; FShanmenDemo20WorldCheckpointStore::Load(Disk,Run,Loaded,Reason);
	TestEqual(TEXT("Latest position kept"),Loaded.PlayerPosition.X,500.0); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemo20EntryRecoveryTest,"Shanmen.Demo20.Expedition.NativeEntryAndTerminalRecovery",Flags)
bool FDemo20EntryRecoveryTest::RunTest(const FString&)
{
	const auto Disk=Root(TEXT("Entry")); const auto ItemDisk=FShanmenItemStorageContext::ForRoot(Disk,FShanmenDemo20Catalog::OwnerId());
	FShanmenItemAuthorityService Items; Items.StartNativeProfile(ItemDisk,FShanmenDemo20Catalog::ProductId(),FShanmenDemo20Catalog::Initial());
	FShanmenItemAuthoritySnapshot S; Items.TryCaptureSnapshot(S); FShanmenItemLoadoutStartRequest R; FString Reason;
	FShanmenDemo20Loadout::Build(S,R,Reason); FShanmenItemRepository Preview; Preview.TryLoadSnapshot(S); const auto Predicted=Preview.StartLoadout(R);
	auto C=Fresh(Predicted.ReservationId); FShanmenDemo20WorldCheckpoint Confirmed;
	if (!TestTrue(TEXT("World first"),FShanmenDemo20WorldCheckpointStore::Save(Disk,Confirmed,C,Reason))) return false;
	Items.SetInjectedFailureForTests(EShanmenItemStoreFailureStage::AtomicReplace);
	TestFalse(TEXT("Item start failure cannot deploy or publish Run"),Items.StartLoadoutDurable(R).IsCommandSuccess());
	Items.SetInjectedFailureForTests(EShanmenItemStoreFailureStage::None); Items.TryCaptureSnapshot(S); FShanmenDemo20ActiveLoadout Active;
	TestFalse(TEXT("No active Run from orphan preflight world"),FShanmenDemo20Loadout::InspectActive(S,Active,Reason));
	if (!TestTrue(TEXT("Retry same start, not another inventory"),Items.StartLoadoutDurable(R).IsCommandSuccess())) return false;
	Items.TryCaptureSnapshot(S); FShanmenDemo20Loadout::InspectActive(S,Active,Reason); TestEqual(TEXT("World and item Run agree"),Active.RunId,Confirmed.Combat.RunId);
	TestEqual(TEXT("Same seed, no second random stream"),Active.RunSeed,Confirmed.RunSeed);
	FShanmenDemo20Session Session; Session.RestoreExpedition(Confirmed.Combat); Session.TryExtract(); Session.CaptureExpedition(C.Combat);
	TestTrue(TEXT("Durable terminal decision before item settlement"),FShanmenDemo20WorldCheckpointStore::Save(Disk,Confirmed,C,Reason));
	const auto Final=Terminal(S,Active,false); Items.SetInjectedFailureForTests(EShanmenItemStoreFailureStage::AtomicReplace);
	TestFalse(TEXT("Terminal save failure cannot announce carried home"),Items.FinalizePreparedRunDurable(Final).IsCommandSuccess());
	FShanmenItemAuthorityService Restart; TestTrue(TEXT("Native reopen"),Restart.StartNativeProfile(ItemDisk,FShanmenDemo20Catalog::ProductId(),FShanmenDemo20Catalog::Initial()).IsReady());
	const FString WorldPath=FShanmenDemo20WorldCheckpointStore::Path(Disk,Active.RunId); TArray<uint8> Damaged;
	FFileHelper::LoadFileToArray(Damaged,*WorldPath); Damaged.Last()^=1; FFileHelper::SaveArrayToFile(Damaged,*WorldPath);
	FShanmenDemo20WorldCheckpoint Loaded;
	TestFalse(TEXT("Corrupt terminal primary cannot resume earlier combat"),FShanmenDemo20WorldCheckpointStore::Load(Disk,Active.RunId,Loaded,Reason));
	TestTrue(TEXT("Explicit repair retains terminal intention for same item Run"),FShanmenDemo20WorldCheckpointStore::Recover(Disk,Active.RunId,Loaded,Reason));
	TestEqual(TEXT("No resumed combat after extraction"),Loaded.Combat.Phase,EShanmenDemo20Phase::Extracted);
	TestTrue(TEXT("Existing terminal command retry"),Restart.FinalizePreparedRunDurable(Final).IsCommandSuccess());
	FShanmenItemAuthorityDocument Before,After; Restart.TryGetDocument(Before);
	TestTrue(TEXT("Repeated terminal replay"),Restart.FinalizePreparedRunDurable(Final).IsCommandSuccess()); Restart.TryGetDocument(After);
	TestTrue(TEXT("No duplicate rewards/return"),Before==After);
	Restart.TryCaptureSnapshot(S); TestFalse(TEXT("Old Run no longer active"),FShanmenDemo20Loadout::InspectActive(S,Active,Reason));
	TestTrue(TEXT("Normal next loadout available"),FShanmenDemo20Loadout::Build(S,R,Reason));
	const auto Next=Restart.StartLoadoutDurable(R); TestTrue(TEXT("Independent next Run"),Next.IsCommandSuccess() && Next.Receipt.ReservationId!=Loaded.Combat.RunId);
	return true;
}
#endif
