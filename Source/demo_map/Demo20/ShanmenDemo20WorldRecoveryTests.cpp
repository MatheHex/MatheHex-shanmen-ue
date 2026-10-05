#if WITH_DEV_AUTOMATION_TESTS
#include "ShanmenDemo20WorldCheckpoint.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

namespace
{
	constexpr auto Flags=EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter;
	using FStore=FShanmenDemo20WorldCheckpointStore;
	struct FFixture
	{
		FString Root=FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("Automation/Demo20.M4.WorldRepair"),FGuid::NewGuid().ToString(EGuidFormats::Digits));
		FGuid Run=FGuid::NewGuid();
		FShanmenDemo20WorldCheckpoint C;
		FString Why;
		FString Path() const { return FStore::Path(Root,Run); }
		TArray<uint8> Bytes(const FString& Suffix=TEXT("")) const
		{ TArray<uint8> B; FFileHelper::LoadFileToArray(B,*(Path()+Suffix)); return B; }
		bool Write(const TArray<uint8>& B,const FString& Suffix=TEXT("")) const
		{ return FFileHelper::SaveArrayToFile(B,*(Path()+Suffix)); }
		bool Start(int32 Revision=2)
		{
			FShanmenDemo20Session S; if (!S.BeginExpedition(Run,26.f,.12f,Revision)) return false;
			FShanmenDemo20WorldCheckpoint I; I.ContentId=FShanmenDemo20Encounters::ContentId(Revision); I.RunSeed=I.SeedForRun(Run);
			S.CaptureExpedition(I.Combat); return FStore::Save(Root,C,I,Why);
		}
		bool Progress()
		{
			FShanmenDemo20Session S; if (!S.RestoreExpedition(C.Combat) || !S.ReceiveSentinelStrike(0)) return false;
			for (int32 N=0;N<8 && S.GetHealth(1)>0.f;++N) { S.Advance(.5f); if (!S.StrikeSentinel(0)) return false; }
			auto I=C; S.CaptureExpedition(I.Combat); I.PlayerPosition=FVector(1160,320,90); I.PlayerYaw=42;
			I.EnemyPositions[1]=FVector(2950,-190,65); I.WarningTargets[1]=FVector(1150,300,8); I.EnemyClocks[1]=.45f;
			return S.GetHealth(1)==0.f && S.GetHealth()<100.f && S.GetHealth()>0.f && FStore::Save(Root,C,I,Why);
		}
		TArray<uint8> Damage()
		{ auto B=Bytes(); if (!B.IsEmpty()) B.Last()^=1; Write(B); return B; }
		bool HasArchived(const TArray<uint8>& Original) const
		{
			TArray<FString> Names; IFileManager::Get().FindFiles(Names,*(Path()+TEXT(".damaged.*")),true,false);
			for (const auto& N:Names) { TArray<uint8> B; if (FFileHelper::LoadFileToArray(B,*FPaths::Combine(FPaths::GetPath(Path()),N)) && B==Original) return true; }
			return false;
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemo20WorldRepairExactTest,"Shanmen.Demo20.Expedition.WorldRepairLatestExactNoResurrection",Flags)
bool FDemo20WorldRepairExactTest::RunTest(const FString&)
{
	for (int32 Revision:{1,2})
	{
		FFixture F; if (!TestTrue(TEXT("Actual attacks produce nonzero damaged player and defeated enemy"),F.Start(Revision) && F.Progress())) return false;
		const auto Expected=F.C; const auto Latest=F.Bytes(); const auto Damaged=F.Damage(); auto Loaded=Expected; Loaded.PlayerYaw=-17;
		TestFalse(TEXT("Readonly load never chooses old .bak"),FStore::Load(F.Root,F.Run,Loaded,F.Why));
		TestEqual(TEXT("Rejected load does not publish partial output"),Loaded.PlayerYaw,-17.f);
		if (!TestTrue(TEXT("Explicit latest replica recovery"),FStore::Recover(F.Root,F.Run,Loaded,F.Why))) return false;
		TestTrue(TEXT("Exact encoded latest bytes, not reconstructed state"),F.Bytes()==Latest);
		TestTrue(TEXT("Damaged bytes kept for diagnosis"),F.HasArchived(Damaged));
		TestEqual(TEXT("Health never restored to full"),Loaded.Combat.Health[0],Expected.Combat.Health[0]);
		TestEqual(TEXT("Defeated enemy not resurrected"),Loaded.Combat.Health[1],0.f);
		TestEqual(TEXT("Seed remains original"),Loaded.RunSeed,Expected.RunSeed); TestEqual(TEXT("Sequence remains original"),Loaded.Combat.Sequence,Expected.Combat.Sequence);
		TestEqual(TEXT("Spatial and warning state exact"),Loaded.WarningTargets[1],Expected.WarningTargets[1]);
		TestEqual(TEXT("Cooldown exact"),Loaded.Combat.AttackCooldown,Expected.Combat.AttackCooldown);
		TestTrue(TEXT("Repeated repair is idempotent"),FStore::Recover(F.Root,F.Run,Loaded,F.Why) && Loaded.Generation==Expected.Generation && F.Bytes()==Latest);
		auto Next=Loaded; Next.PlayerPosition.X+=20;
		TestTrue(TEXT("Recovered generation supports normal next CAS"),FStore::Save(F.Root,Loaded,Next,F.Why) && Loaded.Generation==Expected.Generation+1);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemo20WorldRepairStaleTest,"Shanmen.Demo20.Expedition.WorldRepairRejectsStaleForeignAndInvalidWitness",Flags)
bool FDemo20WorldRepairStaleTest::RunTest(const FString&)
{
	for (int32 Fault=0;Fault<5;++Fault)
	{
		FFixture F; if (!TestTrue(TEXT("Generation three with distinct previous state"),F.Start())) return false;
		const auto Stale=F.Bytes(); if (!TestTrue(TEXT("Generation two"),F.Progress())) return false;
		auto Next=F.C; Next.PlayerYaw=63; if (!TestTrue(TEXT("Generation three"),FStore::Save(F.Root,F.C,Next,F.Why))) return false;
		const auto Damaged=F.Damage(); const auto Head=F.Bytes(TEXT(".head"));
		if (Fault==0) F.Write(Stale,TEXT(".replica"));
		if (Fault==1) { FFixture Foreign; TestTrue(TEXT("Other Run has legitimate encoding"),Foreign.Start()); F.Write(Foreign.Bytes(),TEXT(".replica")); }
		if (Fault==2) { auto Bad=Head; Bad.Last()^=1; F.Write(Bad,TEXT(".head")); }
		if (Fault==3) IFileManager::Get().Delete(*(F.Path()+TEXT(".head")),false,true);
		if (Fault==4) { FFixture Foreign; TestTrue(TEXT("Foreign witness fixture"),Foreign.Start()); F.Write(Foreign.Bytes(TEXT(".head")),TEXT(".head")); }
		auto Output=F.C; Output.PlayerYaw=-23;
		TestFalse(TEXT("No rollback, foreign adoption, or invented witness"),FStore::Recover(F.Root,F.Run,Output,F.Why));
		TestEqual(TEXT("Output unchanged on refusal"),Output.PlayerYaw,-23.f);
		TestTrue(TEXT("Corrupt primary never overwritten on refusal"),F.Bytes()==Damaged);
		TestFalse(TEXT("Previous-generation backup remains inadmissible"),FStore::Load(F.Root,F.Run,Output,F.Why));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemo20WorldRepairAdmittedTest,"Shanmen.Demo20.Expedition.WorldRepairFinishesAdmittedCandidateOnly",Flags)
bool FDemo20WorldRepairAdmittedTest::RunTest(const FString&)
{
	FFixture F; if (!TestTrue(TEXT("Initial world"),F.Start() && F.Progress())) return false;
	const auto Before=F.C; const auto Old=F.Bytes(); auto Intent=Before; Intent.PlayerPosition.X+=90;
	{ TGuardValue<bool> Fault(FStore::bFailAfterWitnessBeforeReplace,true);
		TestFalse(TEXT("Admitted but not replaced is not success"),FStore::Save(F.Root,F.C,Intent,F.Why)); }
	TestEqual(TEXT("No candidate generation published"),F.C.Generation,Before.Generation); TestTrue(TEXT("Old primary still present"),F.Bytes()==Old);
	const auto Head=F.Bytes(TEXT(".head")); const auto Prepared=F.Bytes(TEXT(".tmp"));
	auto Different=Intent; Different.PlayerYaw=73;
	TestFalse(TEXT("Same generation cannot admit another intent"),FStore::Save(F.Root,F.C,Different,F.Why));
	TestTrue(TEXT("Rejected replacement preserves exact admission and prepared bytes"),Head==F.Bytes(TEXT(".head")) && Prepared==F.Bytes(TEXT(".tmp")));
	auto Recovered=Before; TestFalse(TEXT("Load cannot roll back past durable high water"),FStore::Load(F.Root,F.Run,Recovered,F.Why));
	if (!TestTrue(TEXT("Explicit repair finishes exact prepared candidate"),FStore::Recover(F.Root,F.Run,Recovered,F.Why))) return false;
	TestTrue(TEXT("Exact prepared bytes become primary"),F.Bytes()==Prepared && F.HasArchived(Old));
	TestEqual(TEXT("Exactly one pending generation"),Recovered.Generation,Before.Generation+1);
	TestEqual(TEXT("Admitted position, not fresh spawn"),Recovered.PlayerPosition,Intent.PlayerPosition);
	TestTrue(TEXT("Original request adopts exact post-replace result"),FStore::Save(F.Root,F.C,Intent,F.Why) && F.C.Generation==Recovered.Generation);
	TestTrue(TEXT("Replay does not advance another generation"),F.Bytes()==Prepared);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemo20WorldRepairFailureTest,"Shanmen.Demo20.Expedition.WorldRepairFaultsAndExactRetry",Flags)
bool FDemo20WorldRepairFailureTest::RunTest(const FString&)
{
	FFixture F; if (!TestTrue(TEXT("Nonzero baseline"),F.Start() && F.Progress())) return false;
	const auto Before=F.C; auto Intent=Before; Intent.PlayerYaw=61;
	{ TGuardValue<bool> Fault(FStore::bFailReplicaWrite,true);
		TestFalse(TEXT("Mirror write failure cannot claim save success"),FStore::Save(F.Root,F.C,Intent,F.Why)); }
	TestEqual(TEXT("Unconfirmed in-memory generation not published"),F.C.Generation,Before.Generation);
	FShanmenDemo20WorldCheckpoint Loaded;
	TestTrue(TEXT("Restart proves actual primary matches exact admitted candidate"),FStore::Load(F.Root,F.Run,Loaded,F.Why));
	TestEqual(TEXT("Not previous generation"),Loaded.Generation,Before.Generation+1);
	TestTrue(TEXT("Exact same-process retry repairs mirror without another generation"),FStore::Save(F.Root,F.C,Intent,F.Why) && F.C.Generation==Loaded.Generation);
	const auto Latest=F.Bytes(); const auto Damaged=F.Damage(); Loaded.PlayerYaw=-11;
	{ TGuardValue<bool> Fault(FStore::bFailBeforeRepairReplace,true);
		TestFalse(TEXT("Repair failure not announced as recovery"),FStore::Recover(F.Root,F.Run,Loaded,F.Why)); }
	TestTrue(TEXT("Failed repair preserves damaged primary and archive"),F.Bytes()==Damaged && F.HasArchived(Damaged));
	TestEqual(TEXT("Failed repair cannot publish state"),Loaded.PlayerYaw,-11.f);
	TestTrue(TEXT("Retry repairs exact same generation"),FStore::Recover(F.Root,F.Run,Loaded,F.Why) && F.Bytes()==Latest && Loaded.Generation==F.C.Generation);
	auto ForgedPredecessor=Before; ForgedPredecessor.PlayerYaw=-42;
	TestFalse(TEXT("Even exact post-replace body cannot bypass original predecessor CAS"),FStore::Save(F.Root,ForgedPredecessor,Intent,F.Why));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemo20WorldRepairLegacyTest,"Shanmen.Demo20.Expedition.WorldRepairLegacyReadOnlyAndBoundedDamage",Flags)
bool FDemo20WorldRepairLegacyTest::RunTest(const FString&)
{
	FFixture F; if (!TestTrue(TEXT("Canonical legacy payload"),F.Start(1) && F.Progress())) return false;
	const auto Old=F.Bytes(); TestEqual(TEXT("World payload is still 333 bytes"),Old.Num(),333);
	IFileManager::Get().Delete(*(F.Path()+TEXT(".head")),false,true);
	IFileManager::Get().Delete(*(F.Path()+TEXT(".replica")),false,true);
	FShanmenDemo20WorldCheckpoint Loaded;
	TestTrue(TEXT("Healthy legacy read only"),FStore::Load(F.Root,F.Run,Loaded,F.Why) && F.Bytes()==Old);
	TestFalse(TEXT("Load never fabricates metadata"),IFileManager::Get().FileExists(*(F.Path()+TEXT(".head"))));
	const auto Damaged=F.Damage();
	TestFalse(TEXT("Legacy corrupt data cannot invent recovery from .bak"),FStore::Recover(F.Root,F.Run,Loaded,F.Why));
	TestTrue(TEXT("Legacy corruption kept"),F.Bytes()==Damaged);
	TestTrue(TEXT("Restore isolated legacy fixture for normal save enrollment"),F.Write(Old));
	auto Intent=Loaded; Intent.PlayerYaw=51;
	TestTrue(TEXT("Normal legacy CAS save enrolls witness and same-generation replica"),FStore::Save(F.Root,Loaded,Intent,F.Why)
		&& F.Bytes(TEXT(".head")).Num()==76 && F.Bytes(TEXT(".replica"))==F.Bytes());
	const auto Latest=F.Bytes(); const auto LatestHead=F.Bytes(TEXT(".head")); const auto LatestGeneration=Loaded.Generation;
	TArray<uint8> Oversized; Oversized.Init(7,1025); TestTrue(TEXT("Isolated oversized damage"),F.Write(Oversized));
	TestFalse(TEXT("No overwrite of unbounded/unreadable original"),FStore::Recover(F.Root,F.Run,Loaded,F.Why));
	TestTrue(TEXT("Oversized original unchanged"),F.Bytes()==Oversized);
	TArray<uint8> Empty; TestTrue(TEXT("Isolated zero-byte damage"),F.Write(Empty));
	TestFalse(TEXT("An empty primary is not a valid checkpoint"),FStore::Load(F.Root,F.Run,Loaded,F.Why));
	if (!TestTrue(TEXT("Bounded empty original may be repaired"),FStore::Recover(F.Root,F.Run,Loaded,F.Why)))
	{ AddError(F.Why); return false; }
	TestTrue(TEXT("Empty damaged original preserved as an actual file"),F.HasArchived(Empty));
	TestTrue(TEXT("Repair restores exact latest body and retains witness"),F.Bytes()==Latest && F.Bytes(TEXT(".head"))==LatestHead);
	TestEqual(TEXT("Repair never advances generation"),Loaded.Generation,LatestGeneration);
	TestTrue(TEXT("Repeat repair remains exact and idempotent"),FStore::Recover(F.Root,F.Run,Loaded,F.Why)
		&& Loaded.Generation==LatestGeneration && F.Bytes()==Latest && F.HasArchived(Empty));
	return true;
}
#endif
