#if WITH_DEV_AUTOMATION_TESTS
#include "ShanmenDemo20Sources.h"
#include "ShanmenDemo20Catalog.h"
#include "ShanmenDemo20Loadout.h"
#include "ShanmenDemo20Medicine.h"
#include "demo_mapShanmenItemAuthoritySubsystem.h"
#include "Engine/GameInstance.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"

namespace
{
	constexpr auto Flags=EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter;
	using EAction=EShanmenItemGridAction;
	using EState=EShanmenItemInstanceState;
	using EOrigin=EShanmenDemo20MedicineOrigin;
	struct FFixture
	{
		FString Root=FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("Automation/Demo20.M3.Loot/Native"),FGuid::NewGuid().ToString(EGuidFormats::Digits));
		FShanmenItemStorageContext Disk=FShanmenItemStorageContext::ForRoot(Root,FShanmenDemo20Catalog::OwnerId());
		TUniquePtr<FShanmenItemAuthorityService> Items=MakeUnique<FShanmenItemAuthorityService>();
		FGuid Run; FString Why; FShanmenItemGeneratedSourceReceipt Source;
		FShanmenOperationContext Context(const FShanmenItemAuthoritySnapshot& S) const
		{ FShanmenOperationContext C; C.OwnerId=FShanmenDemo20Catalog::OwnerId(); C.RunId=FShanmenDemo20Catalog::ScopeId(); C.Content=S.Content; C.RequestId=FGuid::NewGuid(); return C; }
		FShanmenItemAuthoritySnapshot Snapshot() const { FShanmenItemAuthoritySnapshot S; Items->TryCaptureSnapshot(S); return S; }
		static const FShanmenItemInstance* Find(const FShanmenItemAuthoritySnapshot& S,FGuid Id)
		{ return S.Items.FindByPredicate([&](const auto& I){return I.ItemInstanceId==Id;}); }
		bool Start()
		{
			if (!Items->StartNativeProfile(Disk,FShanmenDemo20Catalog::ProductId(),FShanmenDemo20Catalog::Initial()).IsReady()) return false;
			FShanmenItemLoadoutStartRequest R; if (!FShanmenDemo20Loadout::Build(Snapshot(),R,Why)) return false;
			const auto Started=Items->StartLoadoutDurable(R); Run=Started.Receipt.ReservationId; return Started.IsCommandSuccess();
		}
		bool Restart()
		{ Items=MakeUnique<FShanmenItemAuthorityService>(); return Items->StartNativeProfile(Disk,FShanmenDemo20Catalog::ProductId(),FShanmenDemo20Catalog::Initial()).IsReady(); }
		bool Accept(FName Role=FShanmenDemo20Sources::EnemyRole(2),FName CuratedDefinition=NAME_None,int32 Quantity=3)
		{
			FShanmenItemGeneratedSourceRequest R;
			if (!FShanmenDemo20Sources::Build(Items->ReadGeneratedSource(FShanmenDemo20Catalog::OwnerId(),Run,Role),
				FShanmenDemo20WorldCheckpoint::SeedForRun(Run),R,Why)) return false;
			if (!CuratedDefinition.IsNone())
			{
				// A legal native plan fixture, not a claim that this is the RNG's exact
				// output. Actual randomized plans are covered separately for all roles.
				R.Plan.Entries.Reset(); R.Plan.GeneratedTotalValue=0;
				for (int32 N=0;N<4;++N)
				{
					FShanmenItemGeneratedSourceEntry E; if (!FShanmenDemo20Catalog::Definition(CuratedDefinition,E.Definition)) return false;
					E.Quantity=Quantity; E.SectionId=TEXT("Loot"); E.SlotIndex=N; E.UnitValue=12; E.TotalValue=12*Quantity;
					E.RewardMetadata.RewardSourceRoleId=Role; R.Plan.Entries.Add(E); R.Plan.GeneratedTotalValue+=E.TotalValue;
				}
				R.Plan.RandomizedBudget=R.Plan.GeneratedTotalValue;
			}
			if (!Items->AcceptGeneratedSourceDurable(R).IsCommandSuccess()) return false;
			Source=Items->ReadGeneratedSource(FShanmenDemo20Catalog::OwnerId(),Run,Role).Receipt; return Source.IsValid();
		}
		FShanmenItemSourceMaterializeRequest MaterializeRequest() const
		{
			FShanmenItemSourceMaterializeRequest R; R.Context=Context(Snapshot()); R.ActiveRunId=Run; R.SourceRoleId=Source.GetPlan().SourceRoleId;
			R.Context.RequestId=R.MakeRequestId(R.Context.OwnerId,Run,R.SourceRoleId); return R;
		}
		bool Materialize() { return Items->MaterializeGeneratedSourceDurable(MaterializeRequest()).IsCommandSuccess(); }
		FShanmenItemRunGridRequest Move(FGuid Id,FGuid Container,int32 X,int32 Y,bool Rotated=false) const
		{
			const auto S=Snapshot(); FShanmenItemRunGridRequest R; R.ActiveRunId=Run; auto& G=R.Grid;
			G.Context=Context(S); G.ItemInstanceId=Id; G.ExpectedAuthorityRevision=S.AuthorityRevision;
			if (const auto* I=Find(S,Id)) G.ExpectedItemRevision=I->Revision;
			G.DestinationContainerId=Container; G.X=X; G.Y=Y; G.bRotated=Rotated; return R;
		}
		FShanmenItemRunGridRequest Merge(FGuid From,FGuid To,int32 Amount=MAX_int32) const
		{
			auto R=Move(From,FGuid(),0,0); R.Grid.Action=EAction::Merge; R.Grid.MergeTargetId=To; R.Grid.Amount=Amount;
			const auto S=Snapshot(); if (const auto* I=Find(S,To)) R.Grid.ExpectedTargetRevision=I->Revision; return R;
		}
		FShanmenItemRunFinalizeRequest Terminal(EShanmenItemRunTerminalReason Reason) const
		{
			const auto S=Snapshot(); FShanmenItemRunFinalizeRequest R; R.Context=Context(S); R.ActiveRunId=Run; R.TerminalReason=Reason;
			if (Reason==EShanmenItemRunTerminalReason::Extraction)
			{ FShanmenDemo20ActiveLoadout A; FString W; if (FShanmenDemo20Loadout::InspectActive(S,A,W)) R.SecuredOriginals=A.RemainingOriginals; }
			return R;
		}
		bool EmptyPreparedPills()
		{
			const auto S=Snapshot(); FShanmenDemo20ActiveLoadout A; if (!FShanmenDemo20Loadout::InspectActive(S,A,Why)) return false;
			for (const auto& L:A.RemainingOriginals) if (const auto* I=Find(S,L.ItemInstanceId); I && I->DefinitionId==TEXT("Heal.Pill"))
			{ FShanmenItemRunConsumeRequest U; U.Context=Context(S); U.ActiveRunId=Run; U.ItemInstanceId=L.ItemInstanceId;
				U.Amount=U.ExpectedQuantityBefore=L.RemainingQuantity; U.PurposeId=TEXT("Test.Loot.EmptyCarry");
				if (!Items->ConsumePreparedRunItemDurable(U).IsCommandSuccess()) return false; }
			return true;
		}
		FShanmenDemo20MedicinePorts MedicinePorts()
		{
			FShanmenDemo20MedicinePorts P; P.Capture=[this](auto& S){return Items->TryCaptureSnapshot(S);};
			P.PrepareRun=[this](const auto& R){return Items->PreparePreparedRunQuantityIntentDurable(R);};
			P.FinalizeRun=[this](const auto& R){return Items->FinalizePreparedRunQuantityIntentDurable(R);};
			P.Reserve=[this](const auto& R){return Items->ReserveDurable(R);}; P.Commit=[this](const auto& R){return Items->CommitDurable(R);}; return P;
		}
	};
	FGuid Carry() { return FShanmenDemo20Catalog::ContainerId(TEXT("Carry")); }
	FGuid Secure() { return FShanmenDemo20Catalog::ContainerId(TEXT("Secure")); }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemo20LootActualSourcesTest,"Shanmen.Demo20.Loot.MaterializeActualSixSourcesNativeReplay",Flags)
bool FDemo20LootActualSourcesTest::RunTest(const FString&)
{
	FFixture F; if (!TestTrue(TEXT("Actual native Run"),F.Start())) return false;
	const int32 InitialItems=F.Snapshot().Items.Num(); int32 Added=0;
	for (int32 N=0;N<6;++N)
	{
		const FName Role=N<3?FShanmenDemo20Sources::ChestRole(N):FShanmenDemo20Sources::EnemyRole(N-3);
		if (!TestTrue(TEXT("Actual deterministic source plan"),F.Accept(Role) && F.Materialize())) return false;
		const auto S=F.Snapshot(); Added+=F.Source.GetItemIds().Num(); TestEqual(TEXT("Only canonical actual items added"),S.Items.Num(),InitialItems+Added);
		for (int32 I=0;I<F.Source.GetItemIds().Num();++I)
		{
			const auto* Item=F.Find(S,F.Source.GetItemIds()[I]); const auto& E=F.Source.GetPlan().Entries[I];
			TestTrue(TEXT("Stable ID, exact quantity and provenance"),Item && Item->DefinitionId==E.Definition.DefinitionId
				&& Item->Quantity==E.Quantity && Item->RewardMetadata==E.RewardMetadata && Item->ParentContainerId==F.Source.GetContainerId());
		}
		TestTrue(TEXT("Repeated materialization is success"),F.Materialize()); TestTrue(TEXT("No duplicate item or document revision"),S==F.Snapshot());
		TestTrue(TEXT("New enum receipts reopen via existing JSON codec"),F.Restart()); TestTrue(TEXT("Exact graph survives native reopen"),S==F.Snapshot());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemo20LootProjectionTest,"Shanmen.Demo20.Loot.PreparedOverlayAndRunScopeGates",Flags)
bool FDemo20LootProjectionTest::RunTest(const FString&)
{
	FFixture F; if (!TestTrue(TEXT("Finite pill source"),F.Start() && F.Accept(FShanmenDemo20Sources::EnemyRole(2),TEXT("Heal.Pill")) && F.Materialize())) return false;
	const auto S=F.Snapshot(); FShanmenItemAuthoritySnapshot P;
	TestTrue(TEXT("Read-only projection"),FShanmenItemRunGridPolicy::Project(S,FShanmenDemo20Catalog::OwnerId(),FShanmenDemo20Catalog::ScopeId(),F.Run,P));
	const auto* Virtual=P.Items.FindByPredicate([](const auto& I){return I.DefinitionId==TEXT("Heal.Pill") && I.ParentContainerId==Carry();});
	if (!TestNotNull(TEXT("Prepared eight still occupy original cell"),Virtual)) return false;
	TestEqual(TEXT("Nonzero ledger balance"),Virtual->Quantity,8); TestEqual(TEXT("Actual graph remains depleted"),F.Find(S,Virtual->ItemInstanceId)->Quantity,0);
	FShanmenItemRepository Check; TestTrue(TEXT("Real authority remains loadable"),Check.TryLoadSnapshot(S));
	TestTrue(TEXT("Display projection has not changed or persisted the authority"),S==F.Snapshot());
	auto R=F.Move(F.Source.GetItemIds()[0],Carry(),0,0);
	TestFalse(TEXT("Prepared return cell cannot be stolen"),F.Items->EditActiveRunGridDurable(R).IsCommandSuccess()); TestTrue(TEXT("Failed pickup keeps exact state"),S==F.Snapshot());
	R=F.Move(F.Source.GetItemIds()[0],Carry(),1,0);
	TestFalse(TEXT("Preparation port stays closed"),F.Items->EditGridDurable(R.Grid).IsCommandSuccess());
	R.ActiveRunId=FGuid::NewGuid(); TestFalse(TEXT("Foreign active Run"),F.Items->EditActiveRunGridDurable(R).IsCommandSuccess());
	R=F.Move(F.Source.GetItemIds()[0],FShanmenDemo20Catalog::ContainerId(TEXT("Stash")),9,0);
	TestFalse(TEXT("No remote stash write during Run"),F.Items->EditActiveRunGridDurable(R).IsCommandSuccess());
	R=F.Move(Virtual->ItemInstanceId,Secure(),1,0); TestFalse(TEXT("Prepared quantity is explicitly read-only"),F.Items->EditActiveRunGridDurable(R).IsCommandSuccess());
	R=F.Move(F.Source.GetItemIds()[0],Carry(),1,0); R.Grid.Context.OwnerId=FGuid::NewGuid();
	TestFalse(TEXT("Foreign owner"),F.Items->EditActiveRunGridDurable(R).IsCommandSuccess());
	TestTrue(TEXT("All rejects leave native ledger unchanged"),S==F.Snapshot()); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemo20LootStackTest,"Shanmen.Demo20.Loot.PartialMergeSplitCapacityAndReopen",Flags)
bool FDemo20LootStackTest::RunTest(const FString&)
{
	FFixture F; if (!TestTrue(TEXT("Four actual native stacks, total twelve"),F.Start() && F.Accept(FShanmenDemo20Sources::EnemyRole(2),TEXT("Heal.Pill")) && F.Materialize())) return false;
	const auto Ids=F.Source.GetItemIds(); const auto Pickup=F.Move(Ids[0],Carry(),1,0);
	if (!TestTrue(TEXT("Confirmed pickup"),F.Items->EditActiveRunGridDurable(Pickup).IsCommandSuccess())) return false;
	const auto First=F.Snapshot(); TestTrue(TEXT("Duplicate click"),F.Items->EditActiveRunGridDurable(Pickup).IsCommandSuccess()); TestTrue(TEXT("No duplicate mutation"),First==F.Snapshot());
	for (int32 N=1;N<4;++N) if (!TestTrue(TEXT("Merge from source"),F.Items->EditActiveRunGridDurable(F.Merge(Ids[N],Ids[0])).IsCommandSuccess())) return false;
	const auto Merged=F.Snapshot(); TestEqual(TEXT("Target bounded at ten"),F.Find(Merged,Ids[0])->Quantity,10);
	TestEqual(TEXT("Partial remainder stays two at source"),F.Find(Merged,Ids[3])->Quantity,2);
	TestEqual(TEXT("Remainder never vanished"),F.Find(Merged,Ids[3])->ParentContainerId,F.Source.GetContainerId());
	TestEqual(TEXT("Exhausted source is real tombstone"),F.Find(Merged,Ids[1])->State,EState::Depleted);
	auto Split=F.Move(Ids[0],Secure(),1,0); Split.Grid.Action=EAction::Split; Split.Grid.Amount=4;
	const auto Result=F.Items->EditActiveRunGridDurable(Split); if (!TestTrue(TEXT("Split into actual secure grid"),Result.IsCommandSuccess())) return false;
	const auto S=F.Snapshot(); TestEqual(TEXT("Six ordinary left"),F.Find(S,Ids[0])->Quantity,6);
	TestEqual(TEXT("Four secure"),F.Find(S,Result.Receipt.ItemInstanceId)->Quantity,4);
	TestFalse(TEXT("Occupied target rejected"),F.Items->EditActiveRunGridDurable(F.Move(Ids[3],Carry(),1,0)).IsCommandSuccess());
	TestFalse(TEXT("Outside actual six-column backpack"),F.Items->EditActiveRunGridDurable(F.Move(Ids[3],Carry(),6,0)).IsCommandSuccess());
	TestTrue(TEXT("Failures leave exact split, remainder and identities"),S==F.Snapshot());
	TestTrue(TEXT("Native restart after split and partial stack"),F.Restart()); TestTrue(TEXT("No restore duplication"),S==F.Snapshot()); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemo20LootGeometryTest,"Shanmen.Demo20.Loot.RotationAndSecurePolicy",Flags)
bool FDemo20LootGeometryTest::RunTest(const FString&)
{
	FFixture F; if (!TestTrue(TEXT("Three-cell weapon fixture"),F.Start() && F.Accept(FShanmenDemo20Sources::EnemyRole(2),TEXT("Sword.Plain"),1) && F.Materialize())) return false;
	const auto Id=F.Source.GetItemIds()[0]; auto S=F.Snapshot();
	TestFalse(TEXT("Weapons never enter secure"),F.Items->EditActiveRunGridDurable(F.Move(Id,Secure(),1,0)).IsCommandSuccess());
	TestFalse(TEXT("Rotated width three crosses edge"),F.Items->EditActiveRunGridDurable(F.Move(Id,Carry(),4,1,true)).IsCommandSuccess());
	TestTrue(TEXT("Exact source survives illegal geometry"),S==F.Snapshot());
	TestTrue(TEXT("Legal rotated pickup"),F.Items->EditActiveRunGridDurable(F.Move(Id,Carry(),2,1,true)).IsCommandSuccess());
	S=F.Snapshot(); TestTrue(TEXT("Rotation confirmed in same graph"),S.Grid.RotatedItems.Contains(Id));
	TestTrue(TEXT("Rotate back in ordinary grid"),F.Items->EditActiveRunGridDurable(F.Move(Id,Carry(),2,1,false)).IsCommandSuccess());
	S=F.Snapshot(); TestFalse(TEXT("Rotation cleared"),S.Grid.RotatedItems.Contains(Id));
	auto Equip=F.Move(Id,FShanmenDemo20Catalog::ContainerId(TEXT("Weapon")),0,0); Equip.Grid.Action=EAction::Equip;
	TestFalse(TEXT("No active equipment replacement bypass"),F.Items->EditActiveRunGridDurable(Equip).IsCommandSuccess()); TestTrue(TEXT("Equipment rejection atomic"),S==F.Snapshot()); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemo20LootExtractionTest,"Shanmen.Demo20.Loot.ExtractionClosesWorldAndKeepsClaimed",Flags)
bool FDemo20LootExtractionTest::RunTest(const FString&)
{
	FFixture F; if (!TestTrue(TEXT("Actual graph"),F.Start() && F.Accept(FShanmenDemo20Sources::EnemyRole(2),TEXT("Heal.Pill")) && F.Materialize())) return false;
	const auto Ids=F.Source.GetItemIds();
	if (!TestTrue(TEXT("Ordinary and secure claimed"),F.Items->EditActiveRunGridDurable(F.Move(Ids[0],Carry(),1,0)).IsCommandSuccess()
		&& F.Items->EditActiveRunGridDurable(F.Move(Ids[1],Secure(),1,0)).IsCommandSuccess())) return false;
	const auto R=F.Terminal(EShanmenItemRunTerminalReason::Extraction);
	if (!TestTrue(TEXT("Existing terminal port"),F.Items->FinalizePreparedRunDurable(R).IsCommandSuccess())) return false;
	const auto S=F.Snapshot(); TestEqual(TEXT("Claimed ordinary three"),F.Find(S,Ids[0])->Quantity,3); TestEqual(TEXT("Claimed secure three"),F.Find(S,Ids[1])->Quantity,3);
	TestEqual(TEXT("Unclaimed world is destroyed, not rewarded"),F.Find(S,Ids[2])->State,EState::Destroyed);
	TestTrue(TEXT("Prepared nonzero eight returned without overlap"),S.Items.ContainsByPredicate([](const auto& I){return I.DefinitionId==TEXT("Heal.Pill") && I.ParentContainerId==Carry() && I.SlotIndex==0 && I.Quantity==8;}));
	TestTrue(TEXT("Repeated terminal"),F.Items->FinalizePreparedRunDurable(R).IsCommandSuccess()); TestTrue(TEXT("No double materialization or return"),S==F.Snapshot());
	TestTrue(TEXT("Materialization replay after closed Run does not refill"),F.Materialize()); TestTrue(TEXT("Exact finalized graph"),S==F.Snapshot());
	TestFalse(TEXT("New grid edit on closed Run"),F.Items->EditActiveRunGridDurable(F.Move(Ids[0],Secure(),0,1)).IsCommandSuccess());
	TestTrue(TEXT("Reopen terminal"),F.Restart()); TestTrue(TEXT("Exact graph persists"),S==F.Snapshot());
	FShanmenItemLoadoutStartRequest Next; TestTrue(TEXT("Normal next departure is possible"),FShanmenDemo20Loadout::Build(S,Next,F.Why) && F.Items->StartLoadoutDurable(Next).IsCommandSuccess()); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemo20LootDeathTest,"Shanmen.Demo20.Loot.DeathUsesConfirmedPlacementAndAllowsNextRun",Flags)
bool FDemo20LootDeathTest::RunTest(const FString&)
{
	FFixture F; if (!TestTrue(TEXT("Actual graph"),F.Start() && F.Accept(FShanmenDemo20Sources::EnemyRole(2),TEXT("Heal.Pill")) && F.Materialize())) return false;
	const auto Ids=F.Source.GetItemIds();
	if (!TestTrue(TEXT("Two secure placements"),F.Items->EditActiveRunGridDurable(F.Move(Ids[0],Secure(),1,0)).IsCommandSuccess()
		&& F.Items->EditActiveRunGridDurable(F.Move(Ids[1],Secure(),0,1)).IsCommandSuccess())) return false;
	TestTrue(TEXT("Moved out of secure before death"),F.Items->EditActiveRunGridDurable(F.Move(Ids[1],Carry(),1,0)).IsCommandSuccess());
	const auto Before=F.Snapshot(); const auto D=F.Terminal(EShanmenItemRunTerminalReason::Death);
	if (!TestTrue(TEXT("Death confirmed"),F.Items->FinalizePreparedRunDurable(D).IsCommandSuccess())) return false;
	const auto S=F.Snapshot(); TestEqual(TEXT("Confirmed secure remains three"),F.Find(S,Ids[0])->Quantity,3);
	TestEqual(TEXT("Secure moved out is ordinary loss"),F.Find(S,Ids[1])->State,EState::Destroyed); TestEqual(TEXT("Unclaimed source lost"),F.Find(S,Ids[2])->State,EState::Destroyed);
	for (const auto& I:Before.Items) if (I.ParentContainerId==FShanmenDemo20Catalog::ContainerId(TEXT("Stash"))
		|| I.ParentContainerId==FShanmenDemo20Catalog::ContainerId(TEXT("Wallet")) || I.ParentContainerId==FShanmenDemo20Catalog::ContainerId(TEXT("SecureBox")))
		TestTrue(TEXT("Exact nonzero stash, test money and secure equipment preserved"),S.Items.ContainsByPredicate([&](const auto& V){return V==I;}));
	TestTrue(TEXT("Death replay"),F.Items->FinalizePreparedRunDurable(D).IsCommandSuccess()); TestTrue(TEXT("No repeated retention"),S==F.Snapshot());
	TestTrue(TEXT("Reopen death"),F.Restart()); TestTrue(TEXT("Same death graph"),S==F.Snapshot());
	const auto Supply=F.Items->ReplenishBasicsDurable(FShanmenDemo20Catalog::BasicSupply(S));
	TestEqual(TEXT("Existing spare equipment and secure pills prevent duplicate supplies"),Supply.Receipt.Error,EShanmenItemTransactionError::BasicSupplyNotNeeded);
	TestTrue(TEXT("No-op supply rejection preserves exact nonzero stock"),S==F.Snapshot());
	for (const auto& Choice:TArray<TPair<FName,FName>>{{TEXT("Sword.Heavy"),TEXT("Weapon")},{TEXT("Armor.Leather"),TEXT("Armor")},{TEXT("Backpack.Large"),TEXT("Backpack")}})
	{
		const auto Stock=F.Snapshot(); const auto* Spare=Stock.Items.FindByPredicate([&](const auto& I){return I.DefinitionId==Choice.Key
			&& I.State==EState::Stored && I.ParentContainerId==FShanmenDemo20Catalog::ContainerId(TEXT("Stash"));});
		if (!TestNotNull(TEXT("Retained actual spare exists"),Spare)) return false;
		auto Equip=F.Move(Spare->ItemInstanceId,FShanmenDemo20Catalog::ContainerId(Choice.Value),0,0); Equip.Grid.Action=EAction::Equip;
		if (!TestTrue(TEXT("Normal preparation equips existing spare, no debug grant"),F.Items->EditGridDurable(Equip.Grid).IsCommandSuccess())) return false;
	}
	FShanmenItemLoadoutStartRequest Next; if (!TestTrue(TEXT("Next loadout"),FShanmenDemo20Loadout::Build(F.Snapshot(),Next,F.Why))) return false;
	const auto Started=F.Items->StartLoadoutDurable(Next); TestTrue(TEXT("Another real Run"),Started.IsCommandSuccess()); F.Run=Started.Receipt.ReservationId;
	TestFalse(TEXT("Prior closed world's items cannot be picked in new Run"),F.Items->EditActiveRunGridDurable(F.Move(Ids[2],Carry(),0,0)).IsCommandSuccess()); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemo20LootFailureTest,"Shanmen.Demo20.Loot.MaterializeAndPickupNativeFailureRecovery",Flags)
bool FDemo20LootFailureTest::RunTest(const FString&)
{
	for (bool Grid:{false,true}) for (auto Fault:{EShanmenItemStoreFailureStage::AtomicReplace,EShanmenItemStoreFailureStage::ReadBackCommittedPrimary})
	{
		FFixture F; if (!TestTrue(TEXT("Native isolated plan"),F.Start() && F.Accept(FShanmenDemo20Sources::EnemyRole(2),TEXT("Heal.Pill")))) return false;
		if (Grid && !TestTrue(TEXT("Materialized first"),F.Materialize())) return false;
		const auto Before=F.Snapshot(); const auto M=F.MaterializeRequest(); const auto G=F.Move(F.Source.GetItemIds()[0],Carry(),1,0);
		F.Items->SetInjectedFailureForTests(Fault);
		const auto Failed=Grid?F.Items->EditActiveRunGridDurable(G):F.Items->MaterializeGeneratedSourceDurable(M);
		if (Fault==EShanmenItemStoreFailureStage::AtomicReplace)
		{ TestFalse(TEXT("Unaccepted mutation cannot announce success"),Failed.IsCommandSuccess()); TestTrue(TEXT("Before-replace failure leaves ledger"),Before==F.Snapshot()); }
		else TestTrue(TEXT("Existing service may confirm the exact accepted after-state by reopen"),Failed.IsCommandSuccess() && Failed.IsDurable());
		if (!TestTrue(TEXT("Recover actual primary"),F.Restart())) return false;
		const auto Recovered=F.Snapshot();
		if (Fault==EShanmenItemStoreFailureStage::ReadBackCommittedPrimary) TestEqual(TEXT("One accepted mutation exists"),Recovered.AuthorityRevision,Before.AuthorityRevision+1);
		const auto Retry=Grid?F.Items->EditActiveRunGridDurable(G):F.Items->MaterializeGeneratedSourceDurable(M);
		TestTrue(TEXT("Exact original request rolls forward"),Retry.IsCommandSuccess());
		const auto Final=F.Snapshot(); TestEqual(TEXT("Exactly one mutation after retry"),Final.AuthorityRevision,Before.AuthorityRevision+1);
		TestTrue(TEXT("Replay doesn't mutate"),(Grid?F.Items->EditActiveRunGridDurable(G):F.Items->MaterializeGeneratedSourceDurable(M)).IsCommandSuccess() && Final==F.Snapshot());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemo20LootPendingTest,"Shanmen.Demo20.Loot.PendingPreparedConsumptionFailsClosed",Flags)
bool FDemo20LootPendingTest::RunTest(const FString&)
{
	FFixture F; if (!TestTrue(TEXT("Native fixture"),F.Start() && F.Accept(FShanmenDemo20Sources::EnemyRole(2),TEXT("Heal.Pill")) && F.Materialize())) return false;
	const auto S=F.Snapshot(); const auto* Pill=S.Items.FindByPredicate([](const auto& I){return I.DefinitionId==TEXT("Heal.Pill") && I.State==EState::Depleted;});
	if (!TestNotNull(TEXT("Prepared original"),Pill)) return false;
	FShanmenItemRunQuantityIntentRequest R; R.Context=F.Context(S); R.ActiveRunId=F.Run; R.IntentId=FGuid::NewGuid();
	R.ItemInstanceId=Pill->ItemInstanceId; R.Amount=1; R.ExpectedQuantityBefore=8; R.PurposeId=TEXT("Test.Loot.Pending");
	if (!TestTrue(TEXT("Actual pending consume"),F.Items->PreparePreparedRunQuantityIntentDurable(R).IsCommandSuccess())) return false;
	const auto Pending=F.Snapshot(); FShanmenItemAuthoritySnapshot P;
	TestFalse(TEXT("No stale balance overlay"),FShanmenItemRunGridPolicy::Project(Pending,R.Context.OwnerId,R.Context.RunId,F.Run,P));
	TestFalse(TEXT("No pickup during pending consume"),F.Items->EditActiveRunGridDurable(F.Move(F.Source.GetItemIds()[0],Carry(),1,0)).IsCommandSuccess());
	TestTrue(TEXT("Pending rejection does not rewrite"),Pending==F.Snapshot());
	FShanmenItemRunQuantityIntentFinalizeRequest Done; Done.Context=F.Context(Pending); Done.ActiveRunId=F.Run;
	Done.PrepareRequestId=R.Context.RequestId; Done.IntentId=R.IntentId; Done.ItemInstanceId=R.ItemInstanceId; Done.bCommit=true;
	TestTrue(TEXT("Formal consume finalization"),F.Items->FinalizePreparedRunQuantityIntentDurable(Done).IsCommandSuccess());
	TestTrue(TEXT("Projection now authoritative seven"),FShanmenItemRunGridPolicy::Project(F.Snapshot(),R.Context.OwnerId,R.Context.RunId,F.Run,P));
	TestEqual(TEXT("Nonzero seven"),F.Find(P,Pill->ItemInstanceId)->Quantity,7); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemo20LootMedicineTest,"Shanmen.Demo20.Loot.NewlyPickedMedicineExactlyOnceAndLastPillRecovery",Flags)
bool FDemo20LootMedicineTest::RunTest(const FString&)
{
	for (bool Last:{false,true})
	{
		FFixture F; if (!TestTrue(TEXT("Only newly picked ordinary pills available"),F.Start() && F.EmptyPreparedPills()
			&& F.Accept(FShanmenDemo20Sources::EnemyRole(2),TEXT("Heal.Pill"),Last?1:3) && F.Materialize())) return false;
		const auto Id=F.Source.GetItemIds()[0]; if (!TestTrue(TEXT("Pickup into released original cell"),F.Items->EditActiveRunGridDurable(F.Move(Id,Carry(),0,0)).IsCommandSuccess())) return false;
		FShanmenDemo20Session Session; Session.BeginExpedition(F.Run,26,.12f); for (int32 N=0;N<4;++N) Session.ReceiveSentinelStrike(0);
		FShanmenDemo20WorldCheckpoint C,Fresh; Fresh.ContentId=C.CurrentContentId(); Fresh.RunSeed=C.SeedForRun(F.Run); Session.CaptureExpedition(Fresh.Combat);
		if (!TestTrue(TEXT("Actual nonzero attacked health saved"),FShanmenDemo20WorldCheckpointStore::Save(F.Root,C,Fresh,F.Why))) return false;
		const float HP=C.Combat.Health[0]; FShanmenDemo20WorldCheckpoint Intent;
		TestTrue(TEXT("Newly picked pill accepted by existing medicine saga"),FShanmenDemo20Medicine::BuildIntent(C,F.Snapshot(),Intent,F.Why));
		TestEqual(TEXT("Stored ordinary origin not secure"),Intent.Medicine.Origin,EOrigin::StoredCarry);
		if (!TestTrue(TEXT("Intent saved before consume"),FShanmenDemo20WorldCheckpointStore::Save(F.Root,C,Intent,F.Why))) return false;
		if (Last)
		{
			TGuardValue<bool> Fault(FShanmenDemo20WorldCheckpointStore::bFailBeforeReplace,true);
			TestFalse(TEXT("Last pill committed, world save fails without fake heal"),FShanmenDemo20Medicine::Recover(F.Root,C,F.MedicinePorts(),F.Why));
			TestEqual(TEXT("Unconfirmed HP unchanged"),C.Combat.Health[0],HP);
		}
		if (Last && !TestTrue(TEXT("Native pending intent reopens"),F.Restart() && FShanmenDemo20WorldCheckpointStore::Load(F.Root,F.Run,C,F.Why))) return false;
		if (!TestTrue(TEXT("Confirm exact treatment"),FShanmenDemo20Medicine::Recover(F.Root,C,F.MedicinePorts(),F.Why))) return false;
		TestTrue(TEXT("35 real health restored once"),FMath::IsNearlyEqual(C.Combat.Health[0],HP+35));
		const auto S=F.Snapshot(); TestEqual(TEXT("One real quantity deducted"),F.Find(S,Id)->Quantity,Last?0:2);
		if (Last) TestEqual(TEXT("Last pill tombstone"),F.Find(S,Id)->State,EState::Depleted);
		TestTrue(TEXT("Repeat treatment recovery no double consume"),FShanmenDemo20Medicine::Recover(F.Root,C,F.MedicinePorts(),F.Why) && S==F.Snapshot());
		TestTrue(TEXT("Native graph and healed checkpoint restart"),F.Restart() && FShanmenDemo20WorldCheckpointStore::Load(F.Root,F.Run,C,F.Why));
		TestTrue(TEXT("No quantity or HP refund"),S==F.Snapshot() && FMath::IsNearlyEqual(C.Combat.Health[0],HP+35));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemo20LootTamperTest,"Shanmen.Demo20.Loot.RestoredGraphRequiresPlanAndRunReceipts",Flags)
bool FDemo20LootTamperTest::RunTest(const FString&)
{
	FFixture F; if (!TestTrue(TEXT("Native source graph"),F.Start() && F.Accept(FShanmenDemo20Sources::EnemyRole(2),TEXT("Heal.Pill")) && F.Materialize())) return false;
	const auto Move=F.Move(F.Source.GetItemIds()[0],Carry(),1,0); if (!TestTrue(TEXT("Confirmed move"),F.Items->EditActiveRunGridDurable(Move).IsCommandSuccess())) return false;
	const auto S=F.Snapshot(); FShanmenItemRepository Check; TestTrue(TEXT("Actual graph validates"),Check.TryLoadSnapshot(S));
	auto Bad=S; Bad.ProcessedRequests.RemoveAll([](const auto& P){return P.Receipt.Operation==EShanmenItemTransactionOperation::MaterializeGeneratedSource;});
	TestFalse(TEXT("Source items without materialization receipt"),Check.TryLoadSnapshot(Bad));
	Bad=S; for (auto& P:Bad.ProcessedRequests) if (P.Receipt.Operation==EShanmenItemTransactionOperation::EditActiveRunGrid) P.Receipt.ReservationId=FGuid::NewGuid();
	TestFalse(TEXT("Edited item bound to nonexistent Run"),Check.TryLoadSnapshot(Bad));
	Bad=S; for (auto& I:Bad.Items) if (I.ItemInstanceId==F.Source.GetItemIds()[0]) I.RewardMetadata.RewardSourceRoleId=TEXT("Foreign.Role");
	TestFalse(TEXT("Source provenance cannot change"),Check.TryLoadSnapshot(Bad));
	Bad=S; for (auto& L:Bad.Grid.Layouts) if (L.ContainerId==F.Source.GetContainerId()) L.Kind=EShanmenItemGridKind::Stash;
	TestFalse(TEXT("Source container cannot masquerade as stash"),Check.TryLoadSnapshot(Bad));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemo20LootFacadeTest,"Shanmen.Demo20.Loot.GameInstanceUsesExistingBoundAuthority",Flags)
bool FDemo20LootFacadeTest::RunTest(const FString&)
{
	auto* GI=NewObject<UGameInstance>(); GI->Init(); auto* A=GI->GetSubsystem<Udemo_mapShanmenItemAuthoritySubsystem>();
	if (!TestNotNull(TEXT("Existing product subsystem"),A)) { GI->Shutdown(); return false; }
	FFixture F; FShanmenItemSourceMaterializeRequest Unbound;
	TestFalse(TEXT("Unbound materialization is not success"),A->MaterializeGeneratedSourceDurable(Unbound).IsCommandSuccess());
	if (!TestTrue(TEXT("Isolated existing native profile"),A->BindNativeProfile(Fdemo_mapProfileStorageContext::ForRoot(F.Root),
		FShanmenDemo20Catalog::OwnerId(),FShanmenDemo20Catalog::ProductId(),FShanmenDemo20Catalog::Initial()).IsReady())) { GI->Shutdown(); return false; }
	FShanmenItemAuthoritySnapshot S; A->TryCaptureSnapshot(S); FShanmenItemLoadoutStartRequest L; FShanmenDemo20Loadout::Build(S,L,F.Why);
	const auto Started=A->StartLoadoutDurable(L); F.Run=Started.Receipt.ReservationId;
	FShanmenDemo20SourcePorts P; P.Read=[A](const auto& O,const auto& R,FName Role){return A->ReadGeneratedSource(O,R,Role);};
	P.Accept=[A](const auto& R){return A->AcceptGeneratedSourceDurable(R);};
	if (!TestTrue(TEXT("Actual product source"),FShanmenDemo20Sources::Resolve(F.Run,FShanmenDemo20WorldCheckpoint::SeedForRun(F.Run),FShanmenDemo20Sources::ChestRole(0),P,F.Source,F.Why))) { GI->Shutdown(); return false; }
	A->TryCaptureSnapshot(S); FShanmenItemSourceMaterializeRequest M; M.Context=F.Context(S); M.ActiveRunId=F.Run;
	M.SourceRoleId=F.Source.GetPlan().SourceRoleId; M.Context.RequestId=M.MakeRequestId(M.Context.OwnerId,M.ActiveRunId,M.SourceRoleId);
	if (!TestTrue(TEXT("Facade durable materialization"),A->MaterializeGeneratedSourceDurable(M).IsCommandSuccess())) { GI->Shutdown(); return false; }
	A->TryCaptureSnapshot(S); const auto* I=F.Find(S,F.Source.GetItemIds()[0]);
	if (!TestNotNull(TEXT("Materialized item"),I)) { GI->Shutdown(); return false; } const auto ItemId=I->ItemInstanceId;
	FShanmenItemRunGridRequest G; G.ActiveRunId=F.Run; G.Grid.Context=F.Context(S); G.Grid.ItemInstanceId=I->ItemInstanceId;
	G.Grid.ExpectedItemRevision=I->Revision; G.Grid.ExpectedAuthorityRevision=S.AuthorityRevision; G.Grid.DestinationContainerId=Carry(); G.Grid.X=2;
	TestTrue(TEXT("Facade durable real pickup"),A->EditActiveRunGridDurable(G).IsCommandSuccess());
	A->TryCaptureSnapshot(S); TestEqual(TEXT("Actual same subsystem graph"),F.Find(S,ItemId)->ParentContainerId,Carry());
	GI->Shutdown(); return true;
}
#endif
