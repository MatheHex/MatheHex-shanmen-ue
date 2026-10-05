#if WITH_DEV_AUTOMATION_TESTS
#include "ShanmenDemo20Encounters.h"
#include "ShanmenDemo20WorldCheckpoint.h"
#include "ShanmenDemo20Sources.h"
#include "ShanmenDemo20Catalog.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"

namespace
{
	constexpr auto Flags=EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter;
	using Kind=EShanmenDemo20EnemyKind;
	FString Disk(const TCHAR* Label) { return FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("Automation/Demo20.M3.Encounters"),Label,FGuid::NewGuid().ToString(EGuidFormats::Digits)); }
	bool Fresh(FGuid Run,int32 Revision,FShanmenDemo20WorldCheckpoint& Out)
	{
		FShanmenDemo20Session Session; FShanmenDemo20EnemySpec Specs[3];
		if (!Session.BeginExpedition(Run,26,.12f,Revision) || !FShanmenDemo20Encounters::Build(Run,Revision,Specs)) return false;
		FShanmenDemo20WorldCheckpoint C; C.ContentId=FShanmenDemo20Encounters::ContentId(Revision); C.RunSeed=C.SeedForRun(Run);
		if (!Session.CaptureExpedition(C.Combat)) return false;
		for (int32 I=0;I<3;++I) C.EnemyPositions[I]=Specs[I].Spawn;
		if (!C.IsValid()) return false; Out=C; return true;
	}
	bool SameSpec(const FShanmenDemo20EnemySpec& A,const FShanmenDemo20EnemySpec& B)
	{
		return A.Kind==B.Kind && A.Spawn==B.Spawn && A.Health==B.Health && A.Damage==B.Damage
			&& A.Range==B.Range && A.Speed==B.Speed && A.Windup==B.Windup && A.WarningRadius==B.WarningRadius && A.Scale==B.Scale && A.Color==B.Color;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemo20EncounterGenerationTest,"Shanmen.Demo20.Encounters.SeededTypesPositionsAndIsolatedChannels",Flags)
bool FDemo20EncounterGenerationTest::RunTest(const FString&)
{
	TSet<int32> Combinations,Locations[3];
	for (int32 N=1;N<=128;++N)
	{
		const FGuid Run(20,301,N,7); FShanmenDemo20EnemySpec A[3],B[3];
		if (!TestTrue(TEXT("Stable roster"),FShanmenDemo20Encounters::Build(Run,2,A))) return false;
		FShanmenItemGeneratedSourceReadResult Read; Read.OwnerId=FShanmenDemo20Catalog::OwnerId(); Read.RunId=Run;
		Read.ItemContent=FShanmenDemo20Catalog::ContentStamp(); Read.SourceRoleId=FShanmenDemo20Sources::ChestRole(0);
		Read.Status=EShanmenItemGeneratedSourceReadStatus::Absent; Read.RunState=EShanmenItemGeneratedSourceRunState::Active;
		Read.AuthorityRevision=17; Read.AcceptedSequence=23; Read.PityState=11;
		const uint64 Seed=FShanmenDemo20WorldCheckpoint::SeedForRun(Run);
		FShanmenItemGeneratedSourceRequest LootBefore,LootAfter; FString Why;
		if (!TestTrue(TEXT("Real loot generator before unrelated channel"),FShanmenDemo20Sources::Build(Read,Seed,LootBefore,Why))) return false;
		for (int32 I=2;I>=0;--I) { FVector Position; TestTrue(TEXT("Chest position independent"),FShanmenDemo20Sources::ChestPosition(Run,Seed,I,Position)); }
		TestTrue(TEXT("Roster regenerated without mutation"),FShanmenDemo20Encounters::Build(Run,2,B));
		TestTrue(TEXT("Real loot generator after channel"),FShanmenDemo20Sources::Build(Read,Seed,LootAfter,Why));
		TestTrue(TEXT("No loot reseed by roster/search order"),LootBefore.Plan==LootAfter.Plan);
		Combinations.Add((A[0].Kind==Kind::Ranged?1:0)|(A[1].Kind==Kind::Ranged?2:0));
		for (int32 I=0;I<3;++I)
		{
			TestTrue(TEXT("Same Run exact types/stats/positions"),SameSpec(A[I],B[I]));
			TestTrue(TEXT("Ordinary zones only approved archetypes"),I==2?A[I].Kind==Kind::Elite:A[I].Kind==Kind::Melee || A[I].Kind==Kind::Ranged);
			const float Base=1000.f+2100.f*I;
			TestTrue(TEXT("Four safe candidates per fixed zone"),FMath::Abs(A[I].Spawn.X-Base)==220 && FMath::Abs(A[I].Spawn.Y)==320);
			TestTrue(TEXT("Away from chest candidates and pillar lanes"),FMath::Abs(A[I].Spawn.Y)!=620 && FMath::Abs(A[I].Spawn.Y)<700);
			Locations[I].Add((A[I].Spawn.X>Base?1:0)|(A[I].Spawn.Y>0?2:0));
		}
	}
	TestEqual(TEXT("All four real ordinary-type combinations across fixed sample"),Combinations.Num(),4);
	for (const auto& L:Locations) TestEqual(TEXT("Every zone reaches all four spawn candidates"),L.Num(),4);
	FShanmenDemo20EnemySpec Before[3],Invalid[3]; FShanmenDemo20Encounters::Build(FGuid(20,302,1,7),2,Before);
	for (int32 I=0;I<3;++I) Invalid[I]=Before[I];
	TestFalse(TEXT("No invalid Run fallback"),FShanmenDemo20Encounters::Build(FGuid(),2,Invalid));
	TestFalse(TEXT("Unknown content fails closed"),FShanmenDemo20Encounters::Build(FGuid(20,302,1,7),3,Invalid));
	for (int32 I=0;I<3;++I) TestTrue(TEXT("Rejected generation preserves output"),SameSpec(Before[I],Invalid[I]));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemo20EncounterCombatTest,"Shanmen.Demo20.Encounters.ArchetypeCombatAndRestore",Flags)
bool FDemo20EncounterCombatTest::RunTest(const FString&)
{
	TSet<int32> Seen;
	for (int32 N=1;N<=32;++N)
	{
		const FGuid Run(20,303,N,7); FShanmenDemo20EnemySpec Specs[3]; FShanmenDemo20Encounters::Build(Run,2,Specs);
		for (int32 I=0;I<3;++I)
		{
			FShanmenDemo20Session S; if (!TestTrue(TEXT("Seeded combat starts with actual archetype maxima"),S.BeginExpedition(Run,26,.12f,2))) return false;
			Seen.Add(static_cast<int32>(Specs[I].Kind));
			TestEqual(TEXT("Health from archetype, not slot"),S.GetHealth(I+1),Specs[I].Health);
			const float Damage=Specs[I].Kind==Kind::Elite?28.f:Specs[I].Kind==Kind::Ranged?14.f:18.f;
			const float Maximum=Specs[I].Kind==Kind::Elite?156.f:Specs[I].Kind==Kind::Ranged?65.f:78.f;
			TestTrue(TEXT("Canonical incoming contact"),S.ReceiveSentinelStrike(I));
			TestTrue(TEXT("Independent expected nonzero incoming damage"),FMath::IsNearlyEqual(S.GetHealth(),100.f-Damage*.88f));
			TestTrue(TEXT("Sword contacts correct slot"),S.StrikeSentinel(I)); TestEqual(TEXT("Independent sword arithmetic"),S.GetHealth(I+1),Maximum-26);
			FShanmenDemo20CombatCheckpoint C; S.CaptureExpedition(C); FShanmenDemo20Session Reload;
			TestTrue(TEXT("Exact seeded revision restore"),Reload.RestoreExpedition(C)); Reload.Advance(.5f);
			Reload.SetGuarding(true); TestTrue(TEXT("Restored enemy damage does not revert to slot"),Reload.ReceiveSentinelStrike(I));
			TestTrue(TEXT("Guard plus armor remains canonical"),FMath::IsNearlyEqual(Reload.GetHealth(),100.f-Damage*.88f-Damage*.25f*.88f));
			const float Kept=Reload.GetHealth(); C.Health[I+1]=Maximum+1;
			TestFalse(TEXT("Archetype maximum rejects recovery overflow"),Reload.RestoreExpedition(C)); TestEqual(TEXT("Invalid restore preserves health"),Reload.GetHealth(),Kept);
		}
	}
	TestEqual(TEXT("All three actual combat archetypes exercised"),Seen.Num(),3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemo20EncounterWorldTest,"Shanmen.Demo20.Encounters.NativeWorldRecoveryAndContentLock",Flags)
bool FDemo20EncounterWorldTest::RunTest(const FString&)
{
	for (int32 Revision:{1,2})
	{
		const FGuid Run(20,304,Revision,7); const FString Root=Disk(TEXT("RoundTrip")); FShanmenDemo20WorldCheckpoint C,Confirmed; FString Why;
		if (!TestTrue(TEXT("Versioned fresh world"),Fresh(Run,Revision,C))) return false;
		if (!TestTrue(TEXT("Native initial save"),FShanmenDemo20WorldCheckpointStore::Save(Root,Confirmed,C,Why))) return false;
		TArray<uint8> Bytes; FFileHelper::LoadFileToArray(Bytes,*FShanmenDemo20WorldCheckpointStore::Path(Root,Run));
		TestEqual(TEXT("No new serialized field, same empty-intent layout"),Bytes.Num(),333);
		FShanmenDemo20Session S; S.RestoreExpedition(C.Combat); S.ReceiveSentinelStrike(0);
		while (S.GetHealth(1)>0) { S.Advance(.4f); if (!S.StrikeSentinel(0)) return false; }
		S.CaptureExpedition(C.Combat); C.PlayerPosition=FVector(1340,-140,90); C.EnemyPositions[1]+=FVector(70,-50,0);
		C.WarningTargets[1]=FVector(1260,-130,8); C.EnemyClocks[1]=.43f;
		TestTrue(TEXT("Save mutated health, death and warning"),FShanmenDemo20WorldCheckpointStore::Save(Root,Confirmed,C,Why));
		FShanmenDemo20WorldCheckpoint Loaded; if (!TestTrue(TEXT("Native reopen same world"),FShanmenDemo20WorldCheckpointStore::Load(Root,Run,Loaded,Why))) return false;
		TestEqual(TEXT("Revision derived from existing ContentId"),Loaded.Combat.EncounterRevision,Revision);
		TestEqual(TEXT("Dead enemy not respawned"),Loaded.Combat.Health[1],0.f);
		TestTrue(TEXT("Player not reset to full"),Loaded.Combat.Health[0]<100 && Loaded.Combat.Health[0]==C.Combat.Health[0]);
		TestEqual(TEXT("Moved position not regenerated"),Loaded.EnemyPositions[1],C.EnemyPositions[1]);
		TestEqual(TEXT("Warning location retained"),Loaded.WarningTargets[1],C.WarningTargets[1]); TestEqual(TEXT("Warning timer retained"),Loaded.EnemyClocks[1],.43f);
		auto Flip=Loaded; Flip.Combat.EncounterRevision=Revision==1?2:1; Flip.ContentId=FShanmenDemo20Encounters::ContentId(Flip.Combat.EncounterRevision);
		for (int32 I=1;I<4;++I) Flip.Combat.Health[I]=0;
		TestTrue(TEXT("Alternate content otherwise structurally valid"),Flip.IsValid());
		TestFalse(TEXT("CAS cannot switch accepted Run content"),FShanmenDemo20WorldCheckpointStore::Save(Root,Confirmed,Flip,Why));
		auto Bad=Loaded; Bad.ContentId=FGuid(9,8,7,6); TestFalse(TEXT("Unknown content fails closed"),Bad.IsValid());
		Bad=Loaded; Bad.Combat.EncounterRevision=Revision==1?2:1; TestFalse(TEXT("Memory revision cannot disagree with content"),Bad.IsValid());
		TestTrue(TEXT("Original exact version remains writable"),FShanmenDemo20WorldCheckpointStore::Save(Root,Confirmed,Loaded,Why));
	}
	FShanmenDemo20EnemySpec Old[3]; FShanmenDemo20Encounters::Build(FGuid(20,305,1,7),1,Old);
	TestTrue(TEXT("Legacy exact three kinds"),Old[0].Kind==Kind::Melee && Old[1].Kind==Kind::Ranged && Old[2].Kind==Kind::Elite);
	TestTrue(TEXT("Legacy exact positions"),Old[0].Spawn==FVector(1000,-350,65) && Old[1].Spawn==FVector(3100,200,65) && Old[2].Spawn==FVector(5200,-300,80));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemo20EncounterFailureTest,"Shanmen.Demo20.Encounters.NativeSaveFailureAndTerminal",Flags)
bool FDemo20EncounterFailureTest::RunTest(const FString&)
{
	const FGuid Run(20,306,1,7); const FString Root=Disk(TEXT("Failure")); FShanmenDemo20WorldCheckpoint C,Confirmed; FString Why;
	if (!Fresh(Run,2,C) || !FShanmenDemo20WorldCheckpointStore::Save(Root,Confirmed,C,Why)) return false;
	const auto Before=Confirmed; TArray<uint8> Bytes; FFileHelper::LoadFileToArray(Bytes,*FShanmenDemo20WorldCheckpointStore::Path(Root,Run));
	FShanmenDemo20Session S; S.RestoreExpedition(C.Combat); S.ReceiveSentinelStrike(1); S.CaptureExpedition(C.Combat);
	FShanmenDemo20WorldCheckpointStore::bFailBeforeReplace=true;
	TestFalse(TEXT("World failure does not confirm seeded hit"),FShanmenDemo20WorldCheckpointStore::Save(Root,Confirmed,C,Why));
	FShanmenDemo20WorldCheckpointStore::bFailBeforeReplace=false;
	TArray<uint8> Kept; FFileHelper::LoadFileToArray(Kept,*FShanmenDemo20WorldCheckpointStore::Path(Root,Run)); TestTrue(TEXT("Exact old primary retained"),Bytes==Kept);
	TestEqual(TEXT("Generation not published"),Confirmed.Generation,Before.Generation);
	FShanmenDemo20WorldCheckpointStore::bFailFirstReadAfterReplace=true;
	TestFalse(TEXT("Ambiguous replace remains pending"),FShanmenDemo20WorldCheckpointStore::Save(Root,Confirmed,C,Why));
	TestTrue(TEXT("Same intent retry"),FShanmenDemo20WorldCheckpointStore::Save(Root,Confirmed,C,Why)); TestEqual(TEXT("Only one generation advanced"),Confirmed.Generation,2);
	FShanmenDemo20WorldCheckpoint Loaded; FShanmenDemo20WorldCheckpointStore::Load(Root,Run,Loaded,Why);
	TestEqual(TEXT("Exact damaged health on reopen"),Loaded.Combat.Health[0],C.Combat.Health[0]);
	TestEqual(TEXT("Content retained after ambiguous save"),Loaded.ContentId,C.ContentId);
	S.RestoreExpedition(Loaded.Combat); TestTrue(TEXT("Early exit does not require all three defeated"),S.TryExtract()); S.CaptureExpedition(C.Combat);
	TestTrue(TEXT("Terminal uses same accepted content"),FShanmenDemo20WorldCheckpointStore::Save(Root,Confirmed,C,Why));
	FShanmenDemo20WorldCheckpointStore::Load(Root,Run,Loaded,Why); TestEqual(TEXT("Terminal stays terminal on reopen"),Loaded.Combat.Phase,EShanmenDemo20Phase::Extracted);
	return true;
}
#endif
