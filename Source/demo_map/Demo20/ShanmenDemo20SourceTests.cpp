#if WITH_DEV_AUTOMATION_TESTS
#include "ShanmenDemo20Sources.h"
#include "ShanmenDemo20Catalog.h"
#include "ShanmenDemo20Loadout.h"
#include "ShanmenDemo20WorldCheckpoint.h"
#include "demo_mapDeterministicRewardRandom.h"
#include "demo_mapShanmenItemAuthoritySubsystem.h"
#include "Engine/GameInstance.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"

namespace
{
	constexpr auto Flags=EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter;
	struct FFixture
	{
		FShanmenItemStorageContext Disk=FShanmenItemStorageContext::ForRoot(FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("Automation/Demo20.M3.Sources/Native"),
			FGuid::NewGuid().ToString(EGuidFormats::Digits)),FShanmenDemo20Catalog::OwnerId());
		TUniquePtr<FShanmenItemAuthorityService> Service=MakeUnique<FShanmenItemAuthorityService>();
		FGuid Run; uint64 Seed=0; FString Why;
		bool Start()
		{
			if (!Service->StartNativeProfile(Disk,FShanmenDemo20Catalog::ProductId(),FShanmenDemo20Catalog::Initial()).IsReady()) return false;
			FShanmenItemAuthoritySnapshot S; Service->TryCaptureSnapshot(S); FShanmenItemLoadoutStartRequest R;
			if (!FShanmenDemo20Loadout::Build(S,R,Why)) return false;
			const auto Result=Service->StartLoadoutDurable(R); Run=Result.Receipt.ReservationId;
			Seed=FShanmenDemo20WorldCheckpoint::SeedForRun(Run); return Result.IsCommandSuccess();
		}
		FShanmenItemAuthoritySnapshot Snapshot() { FShanmenItemAuthoritySnapshot S; Service->TryCaptureSnapshot(S); return S; }
		FShanmenDemo20SourcePorts Ports()
		{
			FShanmenDemo20SourcePorts P; P.Read=[this](const auto& O,const auto& R,FName Role){return Service->ReadGeneratedSource(O,R,Role);};
			P.Accept=[this](const auto& R){return Service->AcceptGeneratedSourceDurable(R);}; return P;
		}
		FShanmenItemGeneratedSourceReadResult Read(FName Role) { return Service->ReadGeneratedSource(FShanmenDemo20Catalog::OwnerId(),Run,Role); }
		bool Restart()
		{
			Service=MakeUnique<FShanmenItemAuthorityService>();
			return Service->StartNativeProfile(Disk,FShanmenDemo20Catalog::ProductId(),FShanmenDemo20Catalog::Initial()).IsReady();
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemo20SourceStreamTest,"Shanmen.Demo20.Sources.ExistingRandomStreamVectors",Flags)
bool FDemo20SourceStreamTest::RunTest(const FString&)
{
	Fdemo_mapDeterministicRewardRandom R(1),Z(0),Default(0x9E3779B97F4A7C15ull);
	const uint64 V[]={5180492295206395165ull,12380297144915551517ull,13389498078930870103ull};
	for (const auto X:V) TestEqual(TEXT("Historical xorshift64* byte-independent vector"),R.Next(),X);
	for (int32 I=0;I<100;++I) TestEqual(TEXT("Zero seed retains exact old fallback stream"),Z.Next(),Default.Next());
	Fdemo_mapDeterministicRewardRandom A(42),B(42);
	TestEqual(TEXT("Collapsed range does not advance"),A.RangeInclusive(7,7),7ull);
	TestEqual(TEXT("Reversed range does not advance"),A.RangeInclusive(9,2),9ull);
	TestEqual(TEXT("Still same stream"),A.Next(),B.Next()); return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemo20SourcePlanTest,"Shanmen.Demo20.Sources.RegisteredPlanAndOrderIndependence",Flags)
bool FDemo20SourcePlanTest::RunTest(const FString&)
{
	FFixture F; if (!TestTrue(TEXT("Real Run"),F.Start())) return false;
	for (int32 I=0;I<6;++I)
	{
		const FName Role=I<3?FShanmenDemo20Sources::ChestRole(I):FShanmenDemo20Sources::EnemyRole(I-3);
		auto Read=F.Read(Role); FShanmenItemGeneratedSourceRequest A,B;
		if (!TestTrue(TEXT("Product plan"),FShanmenDemo20Sources::Build(Read,F.Seed,A,F.Why))) return false;
		Read.AcceptedSequence=23; Read.PityState=11;
		TestTrue(TEXT("Different prior cursor is legal"),FShanmenDemo20Sources::Build(Read,F.Seed,B,F.Why));
		TestTrue(TEXT("Interaction order does not change entries"),A.Plan.Entries==B.Plan.Entries);
		TestEqual(TEXT("Independent source seed"),A.Plan.EffectiveSeed,B.Plan.EffectiveSeed);
		TestEqual(TEXT("Carry unrelated pity unchanged"),B.Plan.PityStateAfter,11);
		TestFalse(TEXT("No legacy bypass"),A.Plan.bLegacyCompatibilityView);
		TestTrue(TEXT("Full source contract"),A.Plan.IsValid()); int64 Total=0;
		for (const auto& E:A.Plan.Entries) { FShanmenItemDefinition D;
			TestTrue(TEXT("Only canonical native definitions"),FShanmenDemo20Catalog::Definition(E.Definition.DefinitionId,D) && D==E.Definition);
			TestTrue(TEXT("Finite stack"),E.Quantity>=1 && E.Quantity<=D.MaxStack); Total+=E.TotalValue; }
		TestEqual(TEXT("Generation accounting conserves nonzero score"),Total,A.Plan.RandomizedBudget);
	}
	FShanmenItemGeneratedSourceRequest A; auto Read=F.Read(TEXT("Chest.NotRegistered"));
	TestFalse(TEXT("Arbitrary role not authorized"),FShanmenDemo20Sources::Build(Read,F.Seed,A,F.Why));
	Read=F.Read(FShanmenDemo20Sources::ChestRole(0));
	TestFalse(TEXT("Wrong seed cannot masquerade as Run"),FShanmenDemo20Sources::Build(Read,F.Seed+1,A,F.Why));
	Read.SourceContent.Digest=TEXT("Changed"); TestFalse(TEXT("Partial or foreign content stamp"),FShanmenDemo20Sources::Build(Read,F.Seed,A,F.Why));
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemo20SourceLocationTest,"Shanmen.Demo20.Sources.CandidatePositionsAndNewRunVariance",Flags)
bool FDemo20SourceLocationTest::RunTest(const FString&)
{
	TSet<FString> Outcomes;
	for (uint32 N=1;N<=48;++N)
	{
		const FGuid Run(N,2,3,4); const auto Seed=FShanmenDemo20WorldCheckpoint::SeedForRun(Run); FString Signature;
		for (int32 I=0;I<3;++I) { FVector A,B;
			TestTrue(TEXT("Registered position"),FShanmenDemo20Sources::ChestPosition(Run,Seed,I,A));
			TestTrue(TEXT("Repeat independent of frame/UI"),FShanmenDemo20Sources::ChestPosition(Run,Seed,I,B) && A==B);
			TestTrue(TEXT("Within correct clear zone; away from portals/spawn/pillars"),A.X==800+2100*I && FMath::Abs(A.Y)==620 && A.Z==40);
			Signature+=A.ToString(); }
		Outcomes.Add(Signature);
	}
	TestTrue(TEXT("New independent Runs vary chest layout"),Outcomes.Num()>1);
	FVector Unchanged(9,8,7); TestFalse(TEXT("No unregistered candidate"),FShanmenDemo20Sources::ChestPosition(FGuid(1,2,3,4),1,99,Unchanged));
	TestEqual(TEXT("Rejected result not published"),Unchanged,FVector(9,8,7)); return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemo20SourceReplayTest,"Shanmen.Demo20.Sources.DurableSearchReplayAndNoAcquisition",Flags)
bool FDemo20SourceReplayTest::RunTest(const FString&)
{
	FFixture F; if (!TestTrue(TEXT("Real Run"),F.Start())) return false;
	const auto Before=F.Snapshot(); FShanmenItemGeneratedSourceReceipt First,Second;
	const FName Role=FShanmenDemo20Sources::ChestRole(0);
	if (!TestTrue(TEXT("Actual durable admission"),FShanmenDemo20Sources::Resolve(F.Run,F.Seed,Role,F.Ports(),First,F.Why))) return false;
	const auto Accepted=F.Snapshot();
	TestEqual(TEXT("One saved plan"),Accepted.GeneratedSources.Num(),1);
	TestTrue(TEXT("Plan is not acquired inventory"),Accepted.Items==Before.Items && Accepted.Containers==Before.Containers && Accepted.Grid==Before.Grid);
	TestTrue(TEXT("Reservation quantities untouched"),Accepted.Reservations==Before.Reservations);
	for (const auto& Id:First.GetItemIds()) TestFalse(TEXT("Unacquired IDs not inserted into item graph"),Accepted.Items.ContainsByPredicate([&](const auto& I){return I.ItemInstanceId==Id;}));
	TestTrue(TEXT("New source identities recorded"),First.IsValid() && First.GetItemIds().Num()>0);
	if (!TestTrue(TEXT("Restart actual authority, not a cached generator"),F.Restart())) return false;
	int32 Writes=0; auto P=F.Ports(); P.Accept=[&](const auto&){++Writes;return FShanmenItemDurableCommandResult();};
	TestTrue(TEXT("Existing read requires no generator or write"),FShanmenDemo20Sources::Resolve(F.Run,F.Seed,Role,P,Second,F.Why));
	TestTrue(TEXT("All resolved metadata and IDs retained"),Second==First);
	TestEqual(TEXT("Zero write attempts when already searched"),Writes,0);
	TestTrue(TEXT("Restart and reopen leave full ledger exact"),F.Snapshot()==Accepted);
	TestTrue(TEXT("Honest player preview"),FShanmenDemo20Sources::Preview(Second).Contains(TEXT("尚不能领取"))); return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemo20SourceFailureTest,"Shanmen.Demo20.Sources.SaveFailureAndUncertainRecovery",Flags)
bool FDemo20SourceFailureTest::RunTest(const FString&)
{
	for (const auto Stage:{EShanmenItemStoreFailureStage::AtomicReplace,EShanmenItemStoreFailureStage::ReadBackCommittedPrimary})
	{
		FFixture F; if (!F.Start()) return false; const auto Before=F.Snapshot();
		const FName Role=FShanmenDemo20Sources::ChestRole(1); FShanmenItemGeneratedSourceRequest Expected;
		FShanmenDemo20Sources::Build(F.Read(Role),F.Seed,Expected,F.Why);
		F.Service->SetInjectedFailureForTests(Stage); FShanmenItemGeneratedSourceReceipt Result;
		EShanmenItemDurableCommandStatus Status=EShanmenItemDurableCommandStatus::NotReady; auto P=F.Ports();
		P.Accept=[&](const auto& R){auto Actual=F.Service->AcceptGeneratedSourceDurable(R);Status=Actual.Status;return Actual;};
		const bool Resolved=FShanmenDemo20Sources::Resolve(F.Run,F.Seed,Role,P,Result,F.Why);
		if (Stage==EShanmenItemStoreFailureStage::AtomicReplace)
		{
			TestFalse(TEXT("Before replace is not falsely confirmed"),Resolved); TestFalse(TEXT("No preview on rollback"),Result.IsValid());
			TestTrue(TEXT("Before replace exact rollback"),F.Snapshot()==Before);
		}
		else
		{
			TestTrue(TEXT("Only exact reopened primary may confirm"),Resolved);
			TestEqual(TEXT("Existing service reconciliation, not a fake success"),Status,EShanmenItemDurableCommandStatus::ResolvedAfterReopen);
			TestEqual(TEXT("Coherent durable read"),F.Read(Role).Status,EShanmenItemGeneratedSourceReadStatus::Accepted);
		}
		if (!TestTrue(TEXT("Restart recovers actual primary"),F.Restart())) return false;
		TestTrue(TEXT("Retry same source, no substitute identities"),FShanmenDemo20Sources::Resolve(F.Run,F.Seed,Role,F.Ports(),Result,F.Why));
		TestTrue(TEXT("No rerolled plan"),Result.GetPlan()==Expected.Plan);
		const auto After=F.Snapshot(); TestEqual(TEXT("Single source once"),After.GeneratedSources.Num(),1);
		TestTrue(TEXT("No inventory changes from search"),After.Items==Before.Items && After.Containers==Before.Containers);
	} return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemo20SourceDivergenceTest,"Shanmen.Demo20.Sources.UnresolvedDivergenceFailsClosed",Flags)
bool FDemo20SourceDivergenceTest::RunTest(const FString&)
{
	FFixture F; if (!F.Start()) return false;
	FShanmenItemAuthorityService Stale; if (!Stale.StartExisting(F.Disk).IsReady()) return false;
	FShanmenItemGeneratedSourceReceipt Other,Unconfirmed;
	TestTrue(TEXT("External newer valid generation"),FShanmenDemo20Sources::Resolve(F.Run,F.Seed,FShanmenDemo20Sources::ChestRole(1),F.Ports(),Other,F.Why));
	const auto Durable=F.Snapshot(); FShanmenDemo20SourcePorts P;
	P.Read=[&](const auto& O,const auto& R,FName Role){return Stale.ReadGeneratedSource(O,R,Role);};
	P.Accept=[&](const auto& R){return Stale.AcceptGeneratedSourceDurable(R);};
	TestFalse(TEXT("Older generation cannot overwrite or confirm"),FShanmenDemo20Sources::Resolve(F.Run,F.Seed,FShanmenDemo20Sources::ChestRole(0),P,Unconfirmed,F.Why));
	TestFalse(TEXT("No source preview on unresolved persistence"),Unconfirmed.IsValid());
	TestEqual(TEXT("Unresolved lifecycle blocks later reads"),Stale.ReadGeneratedSource(FShanmenDemo20Catalog::OwnerId(),F.Run,FShanmenDemo20Sources::ChestRole(0)).Status,
		EShanmenItemGeneratedSourceReadStatus::Unavailable);
	if (!TestTrue(TEXT("Reopen durable sole truth"),F.Restart())) return false;
	TestTrue(TEXT("Newer source and all inventory preserved exactly"),F.Snapshot()==Durable); return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemo20SourceCursorTest,"Shanmen.Demo20.Sources.CursorConflictAndTerminalGate",Flags)
bool FDemo20SourceCursorTest::RunTest(const FString&)
{
	FFixture F; if (!F.Start()) return false; FShanmenItemGeneratedSourceRequest Old;
	FShanmenDemo20Sources::Build(F.Read(FShanmenDemo20Sources::ChestRole(0)),F.Seed,Old,F.Why);
	FShanmenItemGeneratedSourceReceipt Other,Fresh;
	TestTrue(TEXT("Another source accepted first"),FShanmenDemo20Sources::Resolve(F.Run,F.Seed,FShanmenDemo20Sources::EnemyRole(2),F.Ports(),Other,F.Why));
	TestFalse(TEXT("Old cursor does not overwrite"),F.Service->AcceptGeneratedSourceDurable(Old).IsCommandSuccess());
	TestTrue(TEXT("Fresh coherent cursor"),FShanmenDemo20Sources::Resolve(F.Run,F.Seed,FShanmenDemo20Sources::ChestRole(0),F.Ports(),Fresh,F.Why));
	TestTrue(TEXT("Cursor refresh does not reroll"),Fresh.GetPlan().Entries==Old.Plan.Entries);
	TestTrue(TEXT("Source domains independent"),Other.GetPlan().EffectiveSeed!=Fresh.GetPlan().EffectiveSeed);
	auto S=F.Snapshot(); FShanmenDemo20ActiveLoadout A; if (!FShanmenDemo20Loadout::InspectActive(S,A,F.Why)) return false;
	FShanmenItemRunFinalizeRequest End; End.Context.OwnerId=FShanmenDemo20Catalog::OwnerId(); End.Context.RunId=FShanmenDemo20Catalog::ScopeId();
	End.Context.Content=S.Content; End.Context.RequestId=FGuid::NewGuid(); End.ActiveRunId=F.Run;
	End.TerminalReason=EShanmenItemRunTerminalReason::Extraction; End.SecuredOriginals=A.RemainingOriginals;
	if (!TestTrue(TEXT("Actual terminal with searched but unclaimed sources"),F.Service->FinalizePreparedRunDurable(End).IsCommandSuccess())) return false;
	const auto Terminal=F.Snapshot(); FShanmenItemGeneratedSourceReceipt Closed;
	TestFalse(TEXT("No new source on closed Run"),FShanmenDemo20Sources::Resolve(F.Run,F.Seed,FShanmenDemo20Sources::ChestRole(2),F.Ports(),Closed,F.Why));
	TestTrue(TEXT("Terminal remains exact"),F.Snapshot()==Terminal); return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemo20SourceTimerTest,"Shanmen.Demo20.Sources.SearchInterruptionAndFramePartition",Flags)
bool FDemo20SourceTimerTest::RunTest(const FString&)
{
	const FName Role=FShanmenDemo20Sources::ChestRole(0); const FVector At(800,620,90); FShanmenDemo20Search A,B;
	TestTrue(TEXT("Valid search"),A.Begin(Role,At,73)); B.Begin(Role,At,73);
	TestEqual(TEXT("Half-second never completes"),A.Advance(.5f,At,73,true),EShanmenDemo20SearchStep::Pending);
	TestEqual(TEXT("One second completes"),A.Advance(.5f,At,73,true),EShanmenDemo20SearchStep::Complete);
	for (int32 I=0;I<3;++I) TestEqual(TEXT("Four frame partition"),B.Advance(.25f,At,73,true),EShanmenDemo20SearchStep::Pending);
	TestEqual(TEXT("Same deadline"),B.Advance(.25f,At,73,true),EShanmenDemo20SearchStep::Complete);
	A.Cancel(); A.Begin(Role,At,73); TestEqual(TEXT("Damage interrupts before completion"),A.Advance(1,At,72,true),EShanmenDemo20SearchStep::Interrupted);
	TestFalse(TEXT("No stale role remains"),A.IsActive()); A.Begin(Role,At,73);
	TestEqual(TEXT("Moved away"),A.Advance(1,At+FVector(21,0,0),73,true),EShanmenDemo20SearchStep::Interrupted);
	A.Begin(Role,At,73); TestEqual(TEXT("Owner unavailable/focus cancel"),A.Advance(1,At,73,false),EShanmenDemo20SearchStep::Interrupted);
	A.Begin(Role,At,73); TestEqual(TEXT("Bad delta"),A.Advance(-1,At,73,true),EShanmenDemo20SearchStep::Interrupted);
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemo20SourceSubsystemTest,"Shanmen.Demo20.Sources.GameInstancePortsAndBoundOwner",Flags)
bool FDemo20SourceSubsystemTest::RunTest(const FString&)
{
	auto* GI=NewObject<UGameInstance>(); GI->Init(); auto* A=GI->GetSubsystem<Udemo_mapShanmenItemAuthoritySubsystem>();
	if (!TestNotNull(TEXT("Real product subsystem"),A)) { GI->Shutdown(); return false; }
	FFixture F;
	TestEqual(TEXT("Unbound is not proven absence"),A->ReadGeneratedSource(FShanmenDemo20Catalog::OwnerId(),FGuid::NewGuid(),FShanmenDemo20Sources::ChestRole(0)).Status,
		EShanmenItemGeneratedSourceReadStatus::Unavailable);
	const auto Bound=A->BindNativeProfile(Fdemo_mapProfileStorageContext::ForRoot(F.Disk.RootDirectory),FShanmenDemo20Catalog::OwnerId(),FShanmenDemo20Catalog::ProductId(),FShanmenDemo20Catalog::Initial());
	if (!TestTrue(TEXT("Bound native owner"),Bound.IsReady())) { GI->Shutdown(); return false; }
	FShanmenItemAuthoritySnapshot S; A->TryCaptureSnapshot(S); FShanmenItemLoadoutStartRequest R;
	FShanmenDemo20Loadout::Build(S,R,F.Why); const auto Started=A->StartLoadoutDurable(R); F.Run=Started.Receipt.ReservationId; F.Seed=FShanmenDemo20WorldCheckpoint::SeedForRun(F.Run);
	FShanmenDemo20SourcePorts Ports; Ports.Read=[A](const auto& O,const auto& Run,FName Role){return A->ReadGeneratedSource(O,Run,Role);};
	Ports.Accept=[A](const auto& Request){return A->AcceptGeneratedSourceDurable(Request);}; FShanmenItemGeneratedSourceReceipt Receipt;
	TestTrue(TEXT("Search enters existing sole product owner"),FShanmenDemo20Sources::Resolve(F.Run,F.Seed,FShanmenDemo20Sources::ChestRole(0),Ports,Receipt,F.Why));
	TestEqual(TEXT("Foreign owner not absence"),A->ReadGeneratedSource(FGuid::NewGuid(),F.Run,FShanmenDemo20Sources::ChestRole(0)).Status,EShanmenItemGeneratedSourceReadStatus::Unavailable);
	A->TryCaptureSnapshot(S); TestEqual(TEXT("One authoritative plan"),S.GeneratedSources.Num(),1);
	GI->Shutdown(); return true;
}
#endif
