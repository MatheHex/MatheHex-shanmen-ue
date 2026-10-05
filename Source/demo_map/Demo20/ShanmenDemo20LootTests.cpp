#if WITH_DEV_AUTOMATION_TESTS
#include "ShanmenDemo20Sources.h"
#include "ShanmenDemo20Catalog.h"
#include "ShanmenDemo20Loadout.h"
#include "ShanmenDemo20Medicine.h"
#include "ShanmenItemStackTransfer.h"
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
		FShanmenItemRunInventoryRequest InventoryRequest() const
		{
			FShanmenItemRunInventoryRequest R; R.Context=Context(Snapshot()); R.ActiveRunId=Run;
			R.Context.RequestId=R.MakeRequestId(R.Context.OwnerId,R.Context.RunId,Run); return R;
		}
		bool Unify() { return Items->MaterializeRunInventoryDurable(InventoryRequest()).IsCommandSuccess(); }
		FGuid OriginalPills() const
		{
			const auto S=Snapshot();
			for (const auto& V:S.Reservations) if (V.State==EShanmenItemReservationState::Committed
				&& V.ResourceKind==EShanmenItemResourceKind::Quantity)
				if (const auto* I=Find(S,V.ItemInstanceId); I && I->DefinitionId==TEXT("Heal.Pill")) return I->ItemInstanceId;
			return {};
		}
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
		FShanmenItemGroundDropRequest Drop(FGuid Id,FIntVector Position=FIntVector(120,-160,12)) const
		{
			const auto S=Snapshot(); FShanmenItemGroundDropRequest R; R.Context=Context(S); R.ActiveRunId=Run;
			R.ItemInstanceId=Id; R.Position=Position; R.ExpectedAuthorityRevision=S.AuthorityRevision;
			if (const auto* I=Find(S,Id)) R.ExpectedItemRevision=I->Revision;
			R.Context.RequestId=R.MakeRequestId(R.Context.OwnerId,R.Context.RunId,Run,Id,R.ExpectedItemRevision); return R;
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
		FShanmenDemo20WorldCheckpoint C,Fresh; Fresh.ContentId=C.LegacyContentId(); Fresh.RunSeed=C.SeedForRun(F.Run); Session.CaptureExpedition(Fresh.Combat);
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
	TestFalse(TEXT("Unbound Run inventory transfer is not success"),A->MaterializeRunInventoryDurable({}).IsCommandSuccess());
	TestFalse(TEXT("Unbound ground transfer is not success"),A->DropActiveRunItemDurable({}).IsCommandSuccess());
	if (!TestTrue(TEXT("Isolated existing native profile"),A->BindNativeProfile(Fdemo_mapProfileStorageContext::ForRoot(F.Root),
		FShanmenDemo20Catalog::OwnerId(),FShanmenDemo20Catalog::ProductId(),FShanmenDemo20Catalog::Initial()).IsReady())) { GI->Shutdown(); return false; }
	FShanmenItemAuthoritySnapshot S; A->TryCaptureSnapshot(S); FShanmenItemLoadoutStartRequest L; FShanmenDemo20Loadout::Build(S,L,F.Why);
	const auto Started=A->StartLoadoutDurable(L); F.Run=Started.Receipt.ReservationId;
	A->TryCaptureSnapshot(S); FShanmenItemRunInventoryRequest Inventory; Inventory.Context=F.Context(S); Inventory.ActiveRunId=F.Run;
	Inventory.Context.RequestId=Inventory.MakeRequestId(Inventory.Context.OwnerId,Inventory.Context.RunId,F.Run);
	TestTrue(TEXT("Existing GameInstance facade transfers actual original quantity"),A->MaterializeRunInventoryDurable(Inventory).IsCommandSuccess());
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
	auto Drop=F.Drop(ItemId); Drop.Context=F.Context(S); Drop.ExpectedAuthorityRevision=S.AuthorityRevision; Drop.ExpectedItemRevision=F.Find(S,ItemId)->Revision;
	Drop.Context.RequestId=Drop.MakeRequestId(Drop.Context.OwnerId,Drop.Context.RunId,F.Run,ItemId,Drop.ExpectedItemRevision);
	TestTrue(TEXT("Bound facade durable ground transfer"),A->DropActiveRunItemDurable(Drop).IsCommandSuccess());
	A->TryCaptureSnapshot(S); TestEqual(TEXT("Same bound graph, not actor quantity"),F.Find(S,ItemId)->ParentContainerId,Drop.ContainerId());
	GI->Shutdown(); return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemo20UnifiedTransferTest,"Shanmen.Demo20.RunInventory.PreparedBalanceOneWayTransferAndNativeReplay",Flags)
bool FDemo20UnifiedTransferTest::RunTest(const FString&)
{
	FFixture F; if (!TestTrue(TEXT("Existing loadout"),F.Start())) return false;
	const auto Id=F.OriginalPills(); const auto Legacy=F.Snapshot();
	FShanmenItemRunConsumeRequest Consume; Consume.Context=F.Context(Legacy); Consume.ActiveRunId=F.Run;
	Consume.ItemInstanceId=Id; Consume.ExpectedQuantityBefore=8; Consume.Amount=3; Consume.PurposeId=TEXT("Test.Inventory.PriorConsume");
	if (!TestTrue(TEXT("Three durably used before upgrade"),F.Items->ConsumePreparedRunItemDurable(Consume).IsCommandSuccess())) return false;
	const auto Before=F.Snapshot(); const auto R=F.InventoryRequest();
	if (!TestTrue(TEXT("Exact five transferred in one durable command"),F.Items->MaterializeRunInventoryDurable(R).IsCommandSuccess())) return false;
	const auto S=F.Snapshot(); TestEqual(TEXT("No new items"),S.Items.Num(),Before.Items.Num());
	TestEqual(TEXT("Same original identity restored"),F.Find(S,Id)->ParentContainerId,Carry());
	TestEqual(TEXT("Only unconsumed five available"),F.Find(S,Id)->Quantity,5);
	FShanmenItemAuthoritySnapshot P; TestTrue(TEXT("Projection is exact graph, no second balance"),
		FShanmenItemRunGridPolicy::Project(S,R.Context.OwnerId,R.Context.RunId,F.Run,P) && P==S);
	TestTrue(TEXT("Exact command duplicate"),F.Items->MaterializeRunInventoryDurable(R).IsCommandSuccess() && S==F.Snapshot());
	TestTrue(TEXT("Native reopen before retry"),F.Restart());
	TestTrue(TEXT("No migration refill after native reopen"),S==F.Snapshot() && F.Items->MaterializeRunInventoryDurable(R).IsCommandSuccess() && S==F.Snapshot());
	Consume.Context=F.Context(S); Consume.ExpectedQuantityBefore=5; Consume.Amount=1;
	TestFalse(TEXT("Old quantity ledger cannot consume after transfer"),F.Items->ConsumePreparedRunItemDurable(Consume).IsCommandSuccess());
	TestEqual(TEXT("Actual five unchanged by stale writer"),F.Find(F.Snapshot(),Id)->Quantity,5);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemo20UnifiedGridTest,"Shanmen.Demo20.RunInventory.OriginalMoveSplitMergeSecureAndExtraction",Flags)
bool FDemo20UnifiedGridTest::RunTest(const FString&)
{
	FFixture F; if (!TestTrue(TEXT("Unified original eight"),F.Start() && F.Unify())) return false;
	const auto Id=F.OriginalPills(); const auto Move=F.Move(Id,Carry(),4,3);
	if (!TestTrue(TEXT("Original stack freely moved"),F.Items->EditActiveRunGridDurable(Move).IsCommandSuccess())) return false;
	auto Split=F.Move(Id,Secure(),1,0); Split.Grid.Action=EAction::Split; Split.Grid.Amount=3;
	const auto Divided=F.Items->EditActiveRunGridDurable(Split);
	if (!TestTrue(TEXT("Original three placed secure"),Divided.IsCommandSuccess())) return false;
	auto S=F.Snapshot(); TestEqual(TEXT("Five ordinary"),F.Find(S,Id)->Quantity,5);
	TestEqual(TEXT("Three secure"),F.Find(S,Divided.Receipt.ItemInstanceId)->Quantity,3);
	TestTrue(TEXT("Original split can merge with same provenance"),F.Items->EditActiveRunGridDurable(F.Merge(Divided.Receipt.ItemInstanceId,Id,1)).IsCommandSuccess());
	S=F.Snapshot(); TestEqual(TEXT("Six ordinary"),F.Find(S,Id)->Quantity,6);
	TestEqual(TEXT("Two secure"),F.Find(S,Divided.Receipt.ItemInstanceId)->Quantity,2);
	const auto Terminal=F.Terminal(EShanmenItemRunTerminalReason::Extraction);
	TestFalse(TEXT("No caller quantity return for transferred originals"),Terminal.SecuredOriginals.ContainsByPredicate([&](const auto& I){return I.ItemInstanceId==Id;}));
	auto Forged=Terminal; Forged.Context=F.Context(S); FShanmenItemRunSecuredOriginal Extra; Extra.ItemInstanceId=Id; Extra.RemainingQuantity=6;
	Forged.SecuredOriginals.Add(Extra);
	TestFalse(TEXT("Caller cannot return transferred balance again"),F.Items->FinalizePreparedRunDurable(Forged).IsCommandSuccess());
	TestEqual(TEXT("Rejected duplicate return leaves actual six"),F.Find(F.Snapshot(),Id)->Quantity,6);
	if (!TestTrue(TEXT("Extraction preserves graph without original-position restore"),F.Items->FinalizePreparedRunDurable(Terminal).IsCommandSuccess())) return false;
	S=F.Snapshot(); TestEqual(TEXT("Six kept in moved ordinary cell"),F.Find(S,Id)->SlotIndex,22);
	TestEqual(TEXT("Secure split kept"),F.Find(S,Divided.Receipt.ItemInstanceId)->Quantity,2);
	TestTrue(TEXT("Native terminal and exact replay"),F.Restart() && S==F.Snapshot()
		&& F.Items->FinalizePreparedRunDurable(Terminal).IsCommandSuccess() && S==F.Snapshot());
	FShanmenItemLoadoutStartRequest Next;
	TestTrue(TEXT("Rearranged originals can start next real Run"),FShanmenDemo20Loadout::Build(S,Next,F.Why) && F.Items->StartLoadoutDurable(Next).IsCommandSuccess());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemo20UnifiedDeathTest,"Shanmen.Demo20.RunInventory.DeathKeepsActualOriginalSecurePlacement",Flags)
bool FDemo20UnifiedDeathTest::RunTest(const FString&)
{
	FFixture F; if (!TestTrue(TEXT("Unified original"),F.Start() && F.Unify())) return false;
	const auto Id=F.OriginalPills(); auto Split=F.Move(Id,Secure(),1,0); Split.Grid.Action=EAction::Split; Split.Grid.Amount=3;
	const auto Divided=F.Items->EditActiveRunGridDurable(Split); if (!TestTrue(TEXT("Three safe"),Divided.IsCommandSuccess())) return false;
	Split=F.Move(Divided.Receipt.ItemInstanceId,Carry(),2,0); Split.Grid.Action=EAction::Split; Split.Grid.Amount=1;
	const auto Out=F.Items->EditActiveRunGridDurable(Split); if (!TestTrue(TEXT("One moved out of secure"),Out.IsCommandSuccess())) return false;
	const auto Before=F.Snapshot(); const auto Terminal=F.Terminal(EShanmenItemRunTerminalReason::Death);
	if (!TestTrue(TEXT("Death closes quantities and ordinary equipment"),F.Items->FinalizePreparedRunDurable(Terminal).IsCommandSuccess())) return false;
	const auto S=F.Snapshot(); TestEqual(TEXT("Original ordinary five lost"),F.Find(S,Id)->State,EState::Destroyed);
	TestEqual(TEXT("Moved-out original one lost"),F.Find(S,Out.Receipt.ItemInstanceId)->State,EState::Destroyed);
	TestEqual(TEXT("Actually secure original two retained"),F.Find(S,Divided.Receipt.ItemInstanceId)->Quantity,2);
	TestEqual(TEXT("Secure parent retained"),F.Find(S,Divided.Receipt.ItemInstanceId)->ParentContainerId,Secure());
	for (const auto& I:Before.Items) if (I.ParentContainerId==FShanmenDemo20Catalog::ContainerId(TEXT("Stash"))
		|| I.DefinitionId==TEXT("Currency.Test") || I.ParentContainerId==FShanmenDemo20Catalog::ContainerId(TEXT("SecureBox")))
		TestTrue(TEXT("Exact nonzero warehouse wallet and safe equipment unchanged"),F.Find(S,I.ItemInstanceId) && *F.Find(S,I.ItemInstanceId)==I);
	TestTrue(TEXT("Restart and duplicate death preserve exact graph"),F.Restart() && S==F.Snapshot()
		&& F.Items->FinalizePreparedRunDurable(Terminal).IsCommandSuccess() && S==F.Snapshot()); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemo20UnifiedPendingTest,"Shanmen.Demo20.RunInventory.PendingLegacyIntentMustResolveBeforeTransfer",Flags)
bool FDemo20UnifiedPendingTest::RunTest(const FString&)
{
	FFixture F; if (!TestTrue(TEXT("Legacy native active Run"),F.Start())) return false;
	FShanmenItemRunQuantityIntentRequest R; R.Context=F.Context(F.Snapshot()); R.ActiveRunId=F.Run; R.IntentId=FGuid::NewGuid();
	R.ItemInstanceId=F.OriginalPills(); R.ExpectedQuantityBefore=8; R.Amount=1; R.PurposeId=TEXT("Test.Inventory.Pending");
	if (!TestTrue(TEXT("Pending original medicine"),F.Items->PreparePreparedRunQuantityIntentDurable(R).IsCommandSuccess())) return false;
	const auto Pending=F.Snapshot(); TestFalse(TEXT("Transfer fails closed"),F.Unify()); TestTrue(TEXT("Pending ledger unchanged"),Pending==F.Snapshot());
	TestTrue(TEXT("Restart keeps pending intent, still no transfer"),F.Restart() && !F.Unify() && Pending==F.Snapshot());
	FShanmenItemRunQuantityIntentFinalizeRequest Done; Done.Context=F.Context(Pending); Done.ActiveRunId=F.Run;
	Done.PrepareRequestId=R.Context.RequestId; Done.IntentId=R.IntentId; Done.ItemInstanceId=R.ItemInstanceId; Done.bCommit=true;
	if (!TestTrue(TEXT("Existing formal resolution before transfer"),F.Items->FinalizePreparedRunQuantityIntentDurable(Done).IsCommandSuccess() && F.Unify())) return false;
	TestEqual(TEXT("No refund of confirmed one"),F.Find(F.Snapshot(),R.ItemInstanceId)->Quantity,7);
	R.Context=F.Context(F.Snapshot()); R.ExpectedQuantityBefore=7;
	TestFalse(TEXT("No new legacy intent after transfer"),F.Items->PreparePreparedRunQuantityIntentDurable(R).IsCommandSuccess()); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemo20UnifiedFailureTest,"Shanmen.Demo20.RunInventory.NativeTransferFailureAndExactRecovery",Flags)
bool FDemo20UnifiedFailureTest::RunTest(const FString&)
{
	for (auto Fault:{EShanmenItemStoreFailureStage::AtomicReplace,EShanmenItemStoreFailureStage::ReadBackCommittedPrimary})
	{
		FFixture F; if (!TestTrue(TEXT("Native active"),F.Start())) return false;
		const auto Before=F.Snapshot(); const auto R=F.InventoryRequest(); F.Items->SetInjectedFailureForTests(Fault);
		const auto Failed=F.Items->MaterializeRunInventoryDurable(R);
		if (Fault==EShanmenItemStoreFailureStage::AtomicReplace)
			TestTrue(TEXT("No accepted write no claimed success"),!Failed.IsCommandSuccess() && Before==F.Snapshot());
		else TestTrue(TEXT("Exact poststate can be proven by service reopen"),Failed.IsCommandSuccess() && Failed.IsDurable());
		if (!TestTrue(TEXT("Native reopen then exact retry"),F.Restart() && F.Items->MaterializeRunInventoryDurable(R).IsCommandSuccess())) return false;
		const auto S=F.Snapshot(); TestEqual(TEXT("One accepted transfer only"),S.AuthorityRevision,Before.AuthorityRevision+1);
		TestEqual(TEXT("Exactly eight, not sixteen"),F.Find(S,F.OriginalPills())->Quantity,8);
		TestTrue(TEXT("Replay same identity has no refill"),F.Items->MaterializeRunInventoryDurable(R).IsCommandSuccess() && S==F.Snapshot());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemo20UnifiedMedicineTest,"Shanmen.Demo20.RunInventory.RearrangedOriginalUsesActualQuantityAndRecovery",Flags)
bool FDemo20UnifiedMedicineTest::RunTest(const FString&)
{
	FFixture F; if (!TestTrue(TEXT("Original eight and same graph"),F.Start() && F.Unify())) return false;
	const auto Id=F.OriginalPills(); if (!TestTrue(TEXT("Original moved"),F.Items->EditActiveRunGridDurable(F.Move(Id,Carry(),5,3)).IsCommandSuccess())) return false;
	FShanmenDemo20Session Session; Session.BeginExpedition(F.Run,26,.12f); for (int32 N=0;N<4;++N) Session.ReceiveSentinelStrike(0);
	FShanmenDemo20WorldCheckpoint C,Fresh; Fresh.ContentId=C.LegacyContentId(); Fresh.RunSeed=C.SeedForRun(F.Run); Session.CaptureExpedition(Fresh.Combat);
	if (!TestTrue(TEXT("Nonzero HP saved"),FShanmenDemo20WorldCheckpointStore::Save(F.Root,C,Fresh,F.Why))) return false;
	const auto HP=C.Combat.Health[0]; FShanmenDemo20WorldCheckpoint Intent;
	if (!TestTrue(TEXT("Formal intent on moved original"),FShanmenDemo20Medicine::BuildIntent(C,F.Snapshot(),Intent,F.Why))) return false;
	TestEqual(TEXT("Original now actual carry, not virtual ledger"),Intent.Medicine.Origin,EOrigin::StoredCarry);
	TestEqual(TEXT("Exact original identity"),Intent.Medicine.ItemId,Id);
	if (!TestTrue(TEXT("World intent saved"),FShanmenDemo20WorldCheckpointStore::Save(F.Root,C,Intent,F.Why))) return false;
	{ TGuardValue<bool> Fault(FShanmenDemo20WorldCheckpointStore::bFailBeforeReplace,true);
		TestFalse(TEXT("Confirmed item use but HP save fails, no fake heal"),FShanmenDemo20Medicine::Recover(F.Root,C,F.MedicinePorts(),F.Why)); }
	TestEqual(TEXT("HP remains unconfirmed"),C.Combat.Health[0],HP);
	if (!TestTrue(TEXT("Native restart same intent"),F.Restart() && FShanmenDemo20WorldCheckpointStore::Load(F.Root,F.Run,C,F.Why)
		&& F.Unify() && FShanmenDemo20Medicine::Recover(F.Root,C,F.MedicinePorts(),F.Why))) return false;
	const auto S=F.Snapshot(); TestEqual(TEXT("One original deducted not refunded by materialize replay"),F.Find(S,Id)->Quantity,7);
	TestTrue(TEXT("Health +35 once"),FMath::IsNearlyEqual(C.Combat.Health[0],HP+35));
	TestTrue(TEXT("Repeat restore no second consumption"),FShanmenDemo20Medicine::Recover(F.Root,C,F.MedicinePorts(),F.Why) && S==F.Snapshot()); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemo20UnifiedIntegrityTest,"Shanmen.Demo20.RunInventory.ScopeAndReceiptIntegrity",Flags)
bool FDemo20UnifiedIntegrityTest::RunTest(const FString&)
{
	FFixture F; if (!TestTrue(TEXT("Active Run"),F.Start())) return false;
	const auto Before=F.Snapshot(); auto R=F.InventoryRequest(); R.Context.OwnerId=FGuid::NewGuid();
	R.Context.RequestId=R.MakeRequestId(R.Context.OwnerId,R.Context.RunId,R.ActiveRunId);
	TestFalse(TEXT("Foreign owner cannot transfer"),F.Items->MaterializeRunInventoryDurable(R).IsCommandSuccess());
	R=F.InventoryRequest(); R.Context.RunId=FGuid::NewGuid();
	R.Context.RequestId=R.MakeRequestId(R.Context.OwnerId,R.Context.RunId,R.ActiveRunId);
	TestFalse(TEXT("Foreign preparation scope cannot transfer"),F.Items->MaterializeRunInventoryDurable(R).IsCommandSuccess());
	R=F.InventoryRequest(); R.ActiveRunId=FGuid::NewGuid(); R.Context.RequestId=R.MakeRequestId(R.Context.OwnerId,R.Context.RunId,R.ActiveRunId);
	TestFalse(TEXT("Foreign Run"),F.Items->MaterializeRunInventoryDurable(R).IsCommandSuccess());
	TestTrue(TEXT("Scope rejects preserve exact native state"),Before==F.Snapshot());
	if (!TestTrue(TEXT("Actual transfer"),F.Unify())) return false;
	const auto S=F.Snapshot(); FShanmenItemRepository Check; TestTrue(TEXT("Actual native graph validates"),Check.TryLoadSnapshot(S));
	auto Bad=S; Bad.ProcessedRequests.RemoveAll([](const auto& P){return P.Receipt.Operation==EShanmenItemTransactionOperation::MaterializeRunInventory;});
	TestFalse(TEXT("Actual original quantity needs cutover receipt"),Check.TryLoadSnapshot(Bad));
	Bad=S; for (auto& P:Bad.ProcessedRequests) if (P.Receipt.Operation==EShanmenItemTransactionOperation::MaterializeRunInventory)
	{ ++P.Receipt.ResourceBefore; ++P.Receipt.ResourceAfter; ++P.Receipt.AvailableAfter; }
	TestFalse(TEXT("False historical transfer balance"),Check.TryLoadSnapshot(Bad));
	Bad=S; for (auto& P:Bad.ProcessedRequests) if (P.Receipt.Operation==EShanmenItemTransactionOperation::MaterializeRunInventory)
		P.Fingerprint=FGuid::NewGuid();
	TestFalse(TEXT("False command fingerprint"),Check.TryLoadSnapshot(Bad));
	R=F.InventoryRequest(); R.Context.RequestId=FGuid::NewGuid();
	TestFalse(TEXT("Caller cannot create a second cutover identity"),F.Items->MaterializeRunInventoryDurable(R).IsCommandSuccess());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemo20UnifiedUnequippedTest,"Shanmen.Demo20.RunInventory.CarriedUnequippedWeaponAndZeroBalance",Flags)
bool FDemo20UnifiedUnequippedTest::RunTest(const FString&)
{
	FFixture F;
	if (!TestTrue(TEXT("Native prep"),F.Items->StartNativeProfile(F.Disk,FShanmenDemo20Catalog::ProductId(),FShanmenDemo20Catalog::Initial()).IsReady())) return false;
	auto S=F.Snapshot(); const auto* Spare=S.Items.FindByPredicate([](const auto& I){return I.DefinitionId==TEXT("Sword.Heavy") && I.State==EState::Stored;});
	if (!TestNotNull(TEXT("Existing spare heavy sword"),Spare)) return false; const auto Id=Spare->ItemInstanceId;
	auto Move=F.Move(Id,Carry(),2,0);
	if (!TestTrue(TEXT("Ordinary prep carries spare weapon"),F.Items->EditGridDurable(Move.Grid).IsCommandSuccess())) return false;
	FShanmenItemLoadoutStartRequest Loadout;
	if (!TestTrue(TEXT("Start ordinary loadout"),FShanmenDemo20Loadout::Build(F.Snapshot(),Loadout,F.Why))) return false;
	const auto Started=F.Items->StartLoadoutDurable(Loadout); F.Run=Started.Receipt.ReservationId;
	if (!TestTrue(TEXT("Original quantity legitimately spent, then transferred"),Started.IsCommandSuccess() && F.EmptyPreparedPills() && F.Unify())) return false;
	S=F.Snapshot(); const auto Pills=F.OriginalPills(); TestEqual(TEXT("Zero original balance not recreated"),F.Find(S,Pills)->Quantity,0);
	TestEqual(TEXT("Carried but unequipped weapon uses actual Stored state"),F.Find(S,Id)->State,EState::Stored);
	TestFalse(TEXT("No stale deployment identity"),F.Find(S,Id)->DeploymentReservationId.IsValid());
	if (!TestTrue(TEXT("Original spare weapon may rotate and move"),F.Items->EditActiveRunGridDurable(F.Move(Id,Carry(),0,2,true)).IsCommandSuccess())) return false;
	const auto Terminal=F.Terminal(EShanmenItemRunTerminalReason::Extraction);
	if (!TestTrue(TEXT("Extraction retains moved spare without deploying or returning twice"),F.Items->FinalizePreparedRunDurable(Terminal).IsCommandSuccess())) return false;
	S=F.Snapshot(); TestEqual(TEXT("Spare remains same ID at rotated cell"),F.Find(S,Id)->SlotIndex,12);
	TestTrue(TEXT("Orientation preserved"),S.Grid.RotatedItems.Contains(Id));
	TestEqual(TEXT("Spent original never refunded"),F.Find(S,Pills)->Quantity,0);
	TestTrue(TEXT("Native reopen"),F.Restart() && S==F.Snapshot()); return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemo20GroundRoundTripTest,"Shanmen.Demo20.GroundDrop.SameIdentityRoundTripAndNativeReplay",Flags)
bool FDemo20GroundRoundTripTest::RunTest(const FString&)
{
	FFixture F; if (!TestTrue(TEXT("Native unified Run"),F.Start() && F.Unify())) return false;
	const auto Id=F.OriginalPills(); const auto Before=F.Snapshot(); const auto Drop=F.Drop(Id);
	const auto Result=F.Items->DropActiveRunItemDurable(Drop);
	if (!TestTrue(TEXT("Whole original stack durably dropped"),Result.IsCommandSuccess())) return false;
	auto S=F.Snapshot(); TestEqual(TEXT("No new item identity"),S.Items.Num(),Before.Items.Num());
	TestEqual(TEXT("Real quantity unchanged"),F.Find(S,Id)->Quantity,8); TestEqual(TEXT("Same original ID at ground"),F.Find(S,Id)->ParentContainerId,Drop.ContainerId());
	TArray<FShanmenItemGroundDropView> Views;
	TestTrue(TEXT("Actual read-only world projection"),FShanmenItemGroundDropPolicy::Read(S,FShanmenDemo20Catalog::OwnerId(),FShanmenDemo20Catalog::ScopeId(),F.Run,Views));
	if (!TestEqual(TEXT("Exactly one ground position"),Views.Num(),1)) return false;
	TestTrue(TEXT("Frozen centimetre position and nonempty"),Views[0].Position==Drop.Position && !Views[0].bEmpty);
	TestTrue(TEXT("Exact repeated drop"),F.Items->DropActiveRunItemDurable(Drop).IsCommandSuccess() && S==F.Snapshot());
	TestTrue(TEXT("Native reopen exact ground graph"),F.Restart() && S==F.Snapshot());
	TestTrue(TEXT("Prior request after reopen"),F.Items->DropActiveRunItemDurable(Drop).IsCommandSuccess() && S==F.Snapshot());
	const auto Pickup=F.Move(Id,Carry(),0,0); TestTrue(TEXT("Existing Run grid picks same instance"),F.Items->EditActiveRunGridDurable(Pickup).IsCommandSuccess());
	S=F.Snapshot(); TestTrue(TEXT("Repeated pickup exact replay"),F.Items->EditActiveRunGridDurable(Pickup).IsCommandSuccess() && S==F.Snapshot());
	TestTrue(TEXT("Old drop replay never moves picked item again"),F.Items->DropActiveRunItemDurable(Drop).IsCommandSuccess() && S==F.Snapshot());
	TestTrue(TEXT("Empty is derived from real slots"),FShanmenItemGroundDropPolicy::Read(S,FShanmenDemo20Catalog::OwnerId(),FShanmenDemo20Catalog::ScopeId(),F.Run,Views) && Views[0].bEmpty);
	const auto Again=F.Drop(Id,FIntVector(250,300,12)); TestTrue(TEXT("A later intentional drop has a new identity"),Again.ContainerId()!=Drop.ContainerId() && F.Items->DropActiveRunItemDurable(Again).IsCommandSuccess());
	S=F.Snapshot(); TestEqual(TEXT("Still no copied items after a second trip"),S.Items.Num(),Before.Items.Num());
	TestTrue(TEXT("Restore both positions, not redraw generation"),F.Restart() && S==F.Snapshot()
		&& FShanmenItemGroundDropPolicy::Read(S,FShanmenDemo20Catalog::OwnerId(),FShanmenDemo20Catalog::ScopeId(),F.Run,Views) && Views.Num()==2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemo20GroundTerminalTest,"Shanmen.Demo20.GroundDrop.SecureDiscardLosesProtectionAndTerminalReplay",Flags)
bool FDemo20GroundTerminalTest::RunTest(const FString&)
{
	for (auto Reason:{EShanmenItemRunTerminalReason::Extraction,EShanmenItemRunTerminalReason::Death})
	{
		FFixture F; if (!TestTrue(TEXT("Native Run"),F.Start() && F.Unify())) return false;
		const auto Original=F.OriginalPills(); auto Split=F.Move(Original,Secure(),1,0); Split.Grid.Action=EAction::Split; Split.Grid.Amount=3;
		const auto Divided=F.Items->EditActiveRunGridDurable(Split); if (!TestTrue(TEXT("Three confirmed safe pills"),Divided.IsCommandSuccess())) return false;
		const auto Drop=F.Drop(Divided.Receipt.ItemInstanceId); if (!TestTrue(TEXT("Explicit safe discard"),F.Items->DropActiveRunItemDurable(Drop).IsCommandSuccess())) return false;
		const auto Before=F.Snapshot(); const auto Terminal=F.Terminal(Reason);
		if (!TestTrue(TEXT("Existing atomic terminal includes ground"),F.Items->FinalizePreparedRunDurable(Terminal).IsCommandSuccess())) return false;
		const auto After=F.Snapshot(); const auto* Lost=F.Find(After,Divided.Receipt.ItemInstanceId);
		TestTrue(TEXT("Unpicked safe discard is lost even on extraction"),Lost && Lost->State==EState::Destroyed && Lost->Quantity==0 && !Lost->ParentContainerId.IsValid());
		TestEqual(TEXT("Ordinary five follow terminal reason, never refunded eight"),F.Find(After,Original)->Quantity,Reason==EShanmenItemRunTerminalReason::Extraction?5:0);
		for (const auto& I:Before.Items) if (I.ParentContainerId==Secure() || I.ParentContainerId==FShanmenDemo20Catalog::ContainerId(TEXT("Stash"))
			|| I.ParentContainerId==FShanmenDemo20Catalog::ContainerId(TEXT("Wallet")) || I.ParentContainerId==FShanmenDemo20Catalog::ContainerId(TEXT("SecureBox")))
			TestTrue(TEXT("Nonzero secure jade, equipment, stash and wallet unchanged"),F.Find(After,I.ItemInstanceId) && *F.Find(After,I.ItemInstanceId)==I);
		TestTrue(TEXT("Terminal and drop replay cannot revive ground"),F.Items->FinalizePreparedRunDurable(Terminal).IsCommandSuccess()
			&& F.Items->DropActiveRunItemDurable(Drop).IsCommandSuccess() && After==F.Snapshot());
		TestTrue(TEXT("Native terminal remains exact"),F.Restart() && After==F.Snapshot());
		TestFalse(TEXT("New ground write after terminal"),F.Items->DropActiveRunItemDurable(F.Drop(Original)).IsCommandSuccess());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemo20GroundPartialTest,"Shanmen.Demo20.GroundDrop.FailedPickupAndPartialStackStayOnGround",Flags)
bool FDemo20GroundPartialTest::RunTest(const FString&)
{
	FFixture F; if (!TestTrue(TEXT("Native real source stacks"),F.Start() && F.Unify() && F.Accept(FShanmenDemo20Sources::EnemyRole(2),TEXT("Heal.Pill")) && F.Materialize())) return false;
	const auto Ids=F.Source.GetItemIds();
	if (!TestTrue(TEXT("Pickup drop stack"),F.Items->EditActiveRunGridDurable(F.Move(Ids[0],Carry(),1,0)).IsCommandSuccess())) return false;
	const auto Drop=F.Drop(Ids[0]); if (!TestTrue(TEXT("Source provenance retained on ground"),F.Items->DropActiveRunItemDurable(Drop).IsCommandSuccess())) return false;
	if (!TestTrue(TEXT("Pickup target stack"),F.Items->EditActiveRunGridDurable(F.Move(Ids[1],Carry(),2,0)).IsCommandSuccess())) return false;
	for (int32 N=2;N<4;++N) if (!TestTrue(TEXT("Make actual target nine"),F.Items->EditActiveRunGridDurable(F.Merge(Ids[N],Ids[1])).IsCommandSuccess())) return false;
	const auto Before=F.Snapshot(); TestEqual(TEXT("Nonzero target nine"),F.Find(Before,Ids[1])->Quantity,9);
	TestFalse(TEXT("Occupied backpack cell does not erase ground"),F.Items->EditActiveRunGridDurable(F.Move(Ids[0],Carry(),0,0)).IsCommandSuccess());
	TestFalse(TEXT("Out of capacity does not erase ground"),F.Items->EditActiveRunGridDurable(F.Move(Ids[0],Carry(),6,0)).IsCommandSuccess());
	TestTrue(TEXT("Both failures preserve exact native graph"),Before==F.Snapshot());
	const auto Merge=F.Merge(Ids[0],Ids[1]); TestTrue(TEXT("One fits, rest remains"),F.Items->EditActiveRunGridDurable(Merge).IsCommandSuccess());
	const auto S=F.Snapshot(); TestEqual(TEXT("Target bounded at ten"),F.Find(S,Ids[1])->Quantity,10);
	TestTrue(TEXT("Ground two retain same ID, origin and container"),F.Find(S,Ids[0])->Quantity==2 && F.Find(S,Ids[0])->ParentContainerId==Drop.ContainerId()
		&& F.Find(S,Ids[0])->RewardMetadata==F.Find(Before,Ids[0])->RewardMetadata);
	TestTrue(TEXT("No duplicate on repeat or native reopen"),F.Items->EditActiveRunGridDurable(Merge).IsCommandSuccess() && S==F.Snapshot() && F.Restart() && S==F.Snapshot());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemo20GroundFailureTest,"Shanmen.Demo20.GroundDrop.NativeSaveFailureDropAndPickupRecovery",Flags)
bool FDemo20GroundFailureTest::RunTest(const FString&)
{
	for (bool Pickup:{false,true}) for (auto Fault:{EShanmenItemStoreFailureStage::AtomicReplace,EShanmenItemStoreFailureStage::ReadBackCommittedPrimary})
	{
		FFixture F; if (!TestTrue(TEXT("Native isolated unified Run"),F.Start() && F.Unify())) return false;
		const auto Id=F.OriginalPills(); const auto Drop=F.Drop(Id);
		if (Pickup && !TestTrue(TEXT("Drop before pickup failure"),F.Items->DropActiveRunItemDurable(Drop).IsCommandSuccess())) return false;
		const auto Move=F.Move(Id,Carry(),0,0); const auto Before=F.Snapshot(); F.Items->SetInjectedFailureForTests(Fault);
		const auto Result=Pickup?F.Items->EditActiveRunGridDurable(Move):F.Items->DropActiveRunItemDurable(Drop);
		if (Fault==EShanmenItemStoreFailureStage::AtomicReplace) TestTrue(TEXT("Not confirmed, exact before-state"),!Result.IsCommandSuccess() && Before==F.Snapshot());
		else TestTrue(TEXT("Existing service proves exact durable after-state"),Result.IsCommandSuccess() && Result.IsDurable());
		if (!TestTrue(TEXT("Reopen native primary"),F.Restart())) return false;
		TestTrue(TEXT("Retry exact identity, never a second grant"),(Pickup?F.Items->EditActiveRunGridDurable(Move):F.Items->DropActiveRunItemDurable(Drop)).IsCommandSuccess());
		const auto S=F.Snapshot(); TestEqual(TEXT("Only one mutation committed"),S.AuthorityRevision,Before.AuthorityRevision+1);
		TestEqual(TEXT("Eight same-ID pills, not sixteen"),F.Find(S,Id)->Quantity,8);
		TestEqual(TEXT("Actual container after recovery"),F.Find(S,Id)->ParentContainerId,Pickup?Carry():Drop.ContainerId());
		TestTrue(TEXT("No second mutation on repeated recover"),(Pickup?F.Items->EditActiveRunGridDurable(Move):F.Items->DropActiveRunItemDurable(Drop)).IsCommandSuccess() && S==F.Snapshot());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemo20GroundScopeTest,"Shanmen.Demo20.GroundDrop.ScopePlacementAndReceiptIntegrity",Flags)
bool FDemo20GroundScopeTest::RunTest(const FString&)
{
	FFixture F; if (!TestTrue(TEXT("Native unified Run"),F.Start() && F.Unify())) return false;
	const auto Id=F.OriginalPills(); const auto Before=F.Snapshot(); const auto Good=F.Drop(Id);
	for (int32 N=0;N<6;++N)
	{
		auto R=Good;
		if (N==0) R.Context.OwnerId=FGuid::NewGuid(); if (N==1) R.Context.RunId=FGuid::NewGuid(); if (N==2) R.ActiveRunId=FGuid::NewGuid();
		if (N==3) R.Position.X=1000001; if (N==4) R.ExpectedItemRevision+=1; if (N==5) R.ExpectedAuthorityRevision+=1;
		R.Context.RequestId=R.MakeRequestId(R.Context.OwnerId,R.Context.RunId,R.ActiveRunId,Id,R.ExpectedItemRevision);
		TestFalse(TEXT("Foreign identity, invalid position and stale revisions refuse"),F.Items->DropActiveRunItemDurable(R).IsCommandSuccess());
	}
	auto R=Good; R.Context.RequestId=FGuid::NewGuid(); TestFalse(TEXT("Arbitrary request cannot create ground identities"),F.Items->DropActiveRunItemDurable(R).IsCommandSuccess());
	for (const auto& I:Before.Items) if (I.ParentContainerId==FShanmenDemo20Catalog::ContainerId(TEXT("Stash"))
		|| I.ParentContainerId==FShanmenDemo20Catalog::ContainerId(TEXT("Weapon")) || I.ParentContainerId==FShanmenDemo20Catalog::ContainerId(TEXT("Backpack")))
		TestFalse(TEXT("No stash, worn or storage discard"),F.Items->DropActiveRunItemDurable(F.Drop(I.ItemInstanceId)).IsCommandSuccess());
	TestTrue(TEXT("All rejects preserve real quantity and graph"),Before==F.Snapshot());
	if (!TestTrue(TEXT("Canonical ground command"),F.Items->DropActiveRunItemDurable(Good).IsCommandSuccess())) return false;
	const auto S=F.Snapshot(); FShanmenItemRepository Check;
	for (int32 N=0;N<5;++N)
	{
		auto Bad=S;
		if (N==0) Bad.ProcessedRequests.RemoveAll([](const auto& P){return P.Receipt.Operation==EShanmenItemTransactionOperation::DropActiveRunItem;});
		if (N==1) for (auto& P:Bad.ProcessedRequests) if (P.Receipt.Operation==EShanmenItemTransactionOperation::DropActiveRunItem) P.Receipt.PurposeId=TEXT("Run.GroundDrop.r1.X999.Y0.Z12");
		if (N==2) for (auto& C:Bad.Containers) if (C.ContainerId==Good.ContainerId()) C.OwnerId=FGuid::NewGuid();
		if (N==3) for (auto& L:Bad.Grid.Layouts) if (L.ContainerId==Good.ContainerId()) L.Kind=EShanmenItemGridKind::Stash;
		if (N==4) for (auto& P:Bad.ProcessedRequests) if (P.Receipt.Operation==EShanmenItemTransactionOperation::DropActiveRunItem) P.Fingerprint=FGuid::NewGuid();
		TestFalse(TEXT("Orphan, position, owner, geometry and fingerprint tamper fail load"),Check.TryLoadSnapshot(Bad));
	}
	R=Good; R.Position.Y+=1; TestFalse(TEXT("Same identity changed payload is conflict"),F.Items->DropActiveRunItemDurable(R).IsCommandSuccess());
	TestTrue(TEXT("Conflicting replay does not change ground"),S==F.Snapshot()); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemo20GroundReservedTest,"Shanmen.Demo20.GroundDrop.PendingConsumeMustCloseBeforeDiscard",Flags)
bool FDemo20GroundReservedTest::RunTest(const FString&)
{
	FFixture F; if (!TestTrue(TEXT("Native Run"),F.Start() && F.Unify())) return false;
	const auto Id=F.OriginalPills(); const auto S=F.Snapshot(); FShanmenItemReserveRequest Reserve;
	Reserve.Context=F.Context(S); Reserve.ItemInstanceId=Id; Reserve.ResourceKind=EShanmenItemResourceKind::Quantity; Reserve.Amount=1;
	Reserve.ExpectedItemRevision=F.Find(S,Id)->Revision; Reserve.PurposeId=TEXT("Test.Pending.Medicine");
	const auto Pending=F.Items->ReserveDurable(Reserve); if (!TestTrue(TEXT("Existing quantity intent pending"),Pending.IsCommandSuccess())) return false;
	const auto Before=F.Snapshot(); TestFalse(TEXT("Pending quantity cannot disappear into ground"),F.Items->DropActiveRunItemDurable(F.Drop(Id)).IsCommandSuccess());
	TestTrue(TEXT("Pending state unchanged across native reopen"),Before==F.Snapshot() && F.Restart() && Before==F.Snapshot());
	FShanmenItemReservationActionRequest Cancel; Cancel.Context=F.Context(Before); Cancel.ReservationId=Pending.Receipt.ReservationId;
	if (!TestTrue(TEXT("Existing cancel resolves intent"),F.Items->CancelDurable(Cancel).IsCommandSuccess())) return false;
	TestTrue(TEXT("Actual stored quantity may now transfer"),F.Items->DropActiveRunItemDurable(F.Drop(Id)).IsCommandSuccess()); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemo20GroundWeaponTest,"Shanmen.Demo20.GroundDrop.RotationAndNewRunCannotReachOldGround",Flags)
bool FDemo20GroundWeaponTest::RunTest(const FString&)
{
	FFixture F; if (!TestTrue(TEXT("Actual weapon source"),F.Start() && F.Unify() && F.Accept(FShanmenDemo20Sources::EnemyRole(2),TEXT("Sword.Plain"),1) && F.Materialize())) return false;
	const auto Id=F.Source.GetItemIds()[0]; if (!TestTrue(TEXT("Rotate picked weapon"),F.Items->EditActiveRunGridDurable(F.Move(Id,Carry(),2,1,true)).IsCommandSuccess())) return false;
	const auto Drop=F.Drop(Id); if (!TestTrue(TEXT("Backpack weapon may discard without unequipping worn sword"),F.Items->DropActiveRunItemDurable(Drop).IsCommandSuccess())) return false;
	auto S=F.Snapshot(); TestTrue(TEXT("Same ID, provenance and rotation"),S.Grid.RotatedItems.Contains(Id) && F.Find(S,Id)->Quantity==1);
	TestTrue(TEXT("World edge failure keeps geometry"),!F.Items->EditActiveRunGridDurable(F.Move(Id,Drop.ContainerId(),7,0,true)).IsCommandSuccess() && S==F.Snapshot());
	TestTrue(TEXT("Pick up rotated same item"),F.Items->EditActiveRunGridDurable(F.Move(Id,Carry(),2,1,true)).IsCommandSuccess());
	const auto Terminal=F.Terminal(EShanmenItemRunTerminalReason::Extraction); if (!TestTrue(TEXT("Extraction with picked weapon"),F.Items->FinalizePreparedRunDurable(Terminal).IsCommandSuccess())) return false;
	S=F.Snapshot(); TestTrue(TEXT("Picked weapon retained, exact native reopen"),F.Find(S,Id)->State==EState::Stored && S.Grid.RotatedItems.Contains(Id) && F.Restart() && S==F.Snapshot());
	const auto OldRun=F.Run; FShanmenItemLoadoutStartRequest Loadout; if (!TestTrue(TEXT("Next normal departure"),FShanmenDemo20Loadout::Build(S,Loadout,F.Why))) return false;
	const auto Start=F.Items->StartLoadoutDurable(Loadout); F.Run=Start.Receipt.ReservationId; if (!TestTrue(TEXT("New native Run and transfer"),Start.IsCommandSuccess() && F.Run!=OldRun && F.Unify())) return false;
	S=F.Snapshot(); TArray<FShanmenItemGroundDropView> Views;
	TestTrue(TEXT("Old positions never appear in new Run"),FShanmenItemGroundDropPolicy::Read(S,FShanmenDemo20Catalog::OwnerId(),FShanmenDemo20Catalog::ScopeId(),F.Run,Views) && Views.IsEmpty());
	TestFalse(TEXT("New Run cannot write previous ground"),F.Items->EditActiveRunGridDurable(F.Move(Id,Drop.ContainerId(),0,0)).IsCommandSuccess());
	TestTrue(TEXT("Previous terminal exact drop replay has no new effect"),F.Items->DropActiveRunItemDurable(Drop).IsCommandSuccess() && S==F.Snapshot()); return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemo20CrossOriginPartialTest,"Shanmen.Demo20.StackTransfer.CrossSourcePreparedPartialAndNativeReplay",Flags)
bool FDemo20CrossOriginPartialTest::RunTest(const FString&)
{
	FFixture F; if (!TestTrue(TEXT("Ordinary original and two independent sources"),F.Start() && F.Unify()
		&& F.Accept(FShanmenDemo20Sources::EnemyRole(0),TEXT("Heal.Pill")) && F.Materialize())) return false;
	const auto Enemy=F.Source; if (!TestTrue(TEXT("Chest independently accepted"),F.Accept(FShanmenDemo20Sources::ChestRole(0),TEXT("Heal.Pill")) && F.Materialize())) return false;
	const auto Chest=F.Source; const auto Original=F.OriginalPills(); const auto Before=F.Snapshot();
	const auto Q=F.Merge(Enemy.GetItemIds()[0],Original); const auto Result=F.Items->EditActiveRunGridDurable(Q);
	if (!TestTrue(TEXT("Enemy ordinary pills mix with prepared eight"),Result.IsCommandSuccess())) return false;
	auto S=F.Snapshot(); TestEqual(TEXT("Actual capacity ten"),F.Find(S,Original)->Quantity,10);
	TestEqual(TEXT("Only two transferred"),Result.Receipt.Amount,2); TestEqual(TEXT("One remains in actual source"),F.Find(S,Enemy.GetItemIds()[0])->Quantity,1);
	TestTrue(TEXT("Both immutable birth metadata unchanged"),F.Find(S,Original)->RewardMetadata==F.Find(Before,Original)->RewardMetadata
		&& F.Find(S,Enemy.GetItemIds()[0])->RewardMetadata==F.Find(Before,Enemy.GetItemIds()[0])->RewardMetadata && S.GeneratedSources==Before.GeneratedSources);
	TestTrue(TEXT("Durable edge records both identities, not another quantity map"),Result.Receipt.ReservationIds.Num()==3
		&& Result.Receipt.ReservationIds[0]==Enemy.GetItemIds()[0] && Result.Receipt.ReservationIds[1]==Original);
	const auto Failed=F.Merge(Chest.GetItemIds()[0],Original); TestFalse(TEXT("Full target refuses without mutation"),F.Items->EditActiveRunGridDurable(Failed).IsCommandSuccess());
	TestTrue(TEXT("Exact nonzero source graph remains"),S==F.Snapshot());
	TestTrue(TEXT("Chest and enemy may merge at world sources"),F.Items->EditActiveRunGridDurable(F.Merge(Chest.GetItemIds()[0],Enemy.GetItemIds()[0])).IsCommandSuccess());
	S=F.Snapshot(); TestEqual(TEXT("Recipient is actual four"),F.Find(S,Enemy.GetItemIds()[0])->Quantity,4);
	TestEqual(TEXT("Consumed source tombstone preserved for lineage"),F.Find(S,Chest.GetItemIds()[0])->State,EState::Depleted);
	int32 Total=0; for (const auto& I:S.Items) if (I.DefinitionId==TEXT("Heal.Pill")) Total+=I.Quantity;
	TestEqual(TEXT("All ordinary pills conserved across distinct origins"),Total,32);
	TestTrue(TEXT("Reopen exact plans, quantities, lineage and replay"),F.Restart() && S==F.Snapshot()
		&& F.Items->EditActiveRunGridDurable(Q).IsCommandSuccess() && S==F.Snapshot()); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemo20CrossOriginSplitTest,"Shanmen.Demo20.StackTransfer.MixedSplitDropAndTerminalPlacement",Flags)
bool FDemo20CrossOriginSplitTest::RunTest(const FString&)
{
	for (auto Reason:{EShanmenItemRunTerminalReason::Extraction,EShanmenItemRunTerminalReason::Death})
	{
		FFixture F; if (!TestTrue(TEXT("Native unified source"),F.Start() && F.Unify() && F.Accept(FShanmenDemo20Sources::EnemyRole(1),TEXT("Heal.Pill")) && F.Materialize())) return false;
		const auto Origin=F.OriginalPills(), Target=F.Source.GetItemIds()[0];
		if (!TestTrue(TEXT("Actual pickup then reversed-origin merge"),F.Items->EditActiveRunGridDurable(F.Move(Target,Carry(),1,0)).IsCommandSuccess()
			&& F.Items->EditActiveRunGridDurable(F.Merge(Origin,Target)).IsCommandSuccess())) return false;
		TestEqual(TEXT("Target ten, original remainder one"),F.Find(F.Snapshot(),Origin)->Quantity,1);
		auto Split=F.Move(Target,Secure(),1,0); Split.Grid.Action=EAction::Split; Split.Grid.Amount=3;
		const auto Divided=F.Items->EditActiveRunGridDurable(Split); if (!TestTrue(TEXT("Mixed three explicitly confirmed safe"),Divided.IsCommandSuccess())) return false;
		const auto Child=Divided.Receipt.ItemInstanceId; const auto Before=F.Snapshot(); const auto Drop=F.Drop(Target);
		if (!TestTrue(TEXT("Mixed remainder same-ID ground trip"),F.Items->DropActiveRunItemDurable(Drop).IsCommandSuccess()
			&& F.Items->EditActiveRunGridDurable(F.Move(Target,Carry(),1,0)).IsCommandSuccess())) return false;
		const auto Terminal=F.Terminal(Reason); if (!TestTrue(TEXT("Terminal follows confirmed placement, not birth source"),F.Items->FinalizePreparedRunDurable(Terminal).IsCommandSuccess())) return false;
		const auto S=F.Snapshot(); TestEqual(TEXT("Safe mixed three preserved"),F.Find(S,Child)->Quantity,3);
		TestEqual(TEXT("Ordinary mixed seven follow terminal reason"),F.Find(S,Target)->Quantity,Reason==EShanmenItemRunTerminalReason::Extraction?7:0);
		TestEqual(TEXT("Original remainder one follows actual ordinary placement"),F.Find(S,Origin)->Quantity,Reason==EShanmenItemRunTerminalReason::Extraction?1:0);
		TestTrue(TEXT("Split edge links mixed parent and same-metadata child"),Divided.Receipt.ReservationIds[0]==Target && Divided.Receipt.ReservationIds[1]==Child
			&& F.Find(S,Child)->RewardMetadata==F.Find(Before,Target)->RewardMetadata);
		TestTrue(TEXT("Restart and terminal replay never refund merged originals"),F.Restart() && S==F.Snapshot()
			&& F.Items->FinalizePreparedRunDurable(Terminal).IsCommandSuccess() && S==F.Snapshot());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemo20CrossOriginFailureTest,"Shanmen.Demo20.StackTransfer.NativeFailureAndExactRetry",Flags)
bool FDemo20CrossOriginFailureTest::RunTest(const FString&)
{
	for (bool Split:{false,true}) for (auto Fault:{EShanmenItemStoreFailureStage::AtomicReplace,EShanmenItemStoreFailureStage::ReadBackCommittedPrimary})
	{
		FFixture F; if (!TestTrue(TEXT("Isolated native source"),F.Start() && F.Unify() && F.Accept(FShanmenDemo20Sources::EnemyRole(0),TEXT("Heal.Pill")) && F.Materialize())) return false;
		const auto Id=F.Source.GetItemIds()[0]; auto Q=F.Merge(Id,F.OriginalPills());
		if (Split) { Q=F.Move(F.OriginalPills(),Secure(),1,0); Q.Grid.Action=EAction::Split; Q.Grid.Amount=2; }
		const auto Before=F.Snapshot(); F.Items->SetInjectedFailureForTests(Fault); const auto Result=F.Items->EditActiveRunGridDurable(Q);
		if (Fault==EShanmenItemStoreFailureStage::AtomicReplace) TestTrue(TEXT("Failed write preserves quantity and no witness is installed"),!Result.IsCommandSuccess() && Before==F.Snapshot());
		else TestTrue(TEXT("Confirmed durable post-state, not guessed success"),Result.IsCommandSuccess() && Result.IsDurable());
		if (!TestTrue(TEXT("Native reopen and exact retry"),F.Restart() && F.Items->EditActiveRunGridDurable(Q).IsCommandSuccess())) return false;
		const auto S=F.Snapshot(); TestEqual(TEXT("Exactly one authority mutation"),S.AuthorityRevision,Before.AuthorityRevision+1);
		TestTrue(TEXT("Exact retry does not create another edge or quantity"),F.Items->EditActiveRunGridDurable(Q).IsCommandSuccess() && S==F.Snapshot());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemo20CrossOriginIntegrityTest,"Shanmen.Demo20.StackTransfer.WitnessParticipantsAndBirthIntegrity",Flags)
bool FDemo20CrossOriginIntegrityTest::RunTest(const FString&)
{
	FFixture F; if (!TestTrue(TEXT("Native source"),F.Start() && F.Unify() && F.Accept(FShanmenDemo20Sources::EnemyRole(0),TEXT("Heal.Pill")) && F.Materialize())) return false;
	const auto Q=F.Merge(F.Source.GetItemIds()[0],F.OriginalPills()); if (!TestTrue(TEXT("Witnessed cross-origin merge"),F.Items->EditActiveRunGridDurable(Q).IsCommandSuccess())) return false;
	const auto S=F.Snapshot(); FShanmenItemRepository Check;
	for (int32 N=0;N<6;++N)
	{
		auto Bad=S; for (auto& P:Bad.ProcessedRequests) if (P.Receipt.RequestId==Q.Grid.Context.RequestId)
		{
			if (N==0) P.Receipt.ReservationIds.Reset();
			if (N==1) P.Receipt.ReservationIds[1]=F.Source.GetItemIds()[1];
			if (N==2) P.Receipt.ReservationIds[2]=FGuid::NewGuid();
			if (N==3) { ++P.Receipt.Amount; --P.Receipt.ResourceAfter; --P.Receipt.AvailableAfter; }
			if (N==4) P.Fingerprint=FGuid::NewGuid();
		}
		if (N==5) for (auto& I:Bad.Items) if (I.ItemInstanceId==Q.Grid.ItemInstanceId) I.RewardMetadata.RewardSourceRoleId=TEXT("Chest.False");
		TestFalse(TEXT("Missing witness, false participant, witness, balanced amount, fingerprint and overwritten birth all fail load"),Check.TryLoadSnapshot(Bad));
	}
	auto Split=F.Move(F.OriginalPills(),Secure(),1,0); Split.Grid.Action=EAction::Split; Split.Grid.Amount=2;
	const auto Result=F.Items->EditActiveRunGridDurable(Split); if (!TestTrue(TEXT("Split descendant"),Result.IsCommandSuccess())) return false;
	auto Bad=F.Snapshot(); for (auto& I:Bad.Items) if (I.ItemInstanceId==Result.Receipt.ItemInstanceId) I.RewardMetadata.RewardSourceRoleId=TEXT("Chest.False");
	TestFalse(TEXT("Split cannot invent a new birth origin"),Check.TryLoadSnapshot(Bad));
	TestTrue(TEXT("Valid native graph still exact after all rejected loads"),F.Restart() && F.Snapshot()!=Bad); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemo20CrossOriginMedicineTest,"Shanmen.Demo20.StackTransfer.MixedMedicineConsumesOnceAcrossRecovery",Flags)
bool FDemo20CrossOriginMedicineTest::RunTest(const FString&)
{
	FFixture F; if (!TestTrue(TEXT("Native mixed ordinary pills"),F.Start() && F.Unify() && F.Accept(FShanmenDemo20Sources::EnemyRole(0),TEXT("Heal.Pill")) && F.Materialize()
		&& F.Items->EditActiveRunGridDurable(F.Merge(F.Source.GetItemIds()[0],F.OriginalPills())).IsCommandSuccess())) return false;
	const auto Id=F.OriginalPills(); FShanmenDemo20Session Session; Session.BeginExpedition(F.Run,26,.12f);
	for (int32 N=0;N<4;++N) Session.ReceiveSentinelStrike(0);
	FShanmenDemo20WorldCheckpoint C,Fresh; Fresh.ContentId=C.LegacyContentId(); Fresh.RunSeed=C.SeedForRun(F.Run); Session.CaptureExpedition(Fresh.Combat);
	if (!TestTrue(TEXT("Save nonzero damaged health"),FShanmenDemo20WorldCheckpointStore::Save(F.Root,C,Fresh,F.Why))) return false;
	const auto HP=C.Combat.Health[0]; FShanmenDemo20WorldCheckpoint Intent;
	if (!TestTrue(TEXT("Formal medicine intent sees actual mixed ten"),FShanmenDemo20Medicine::BuildIntent(C,F.Snapshot(),Intent,F.Why))) return false;
	TestEqual(TEXT("Exact target identity"),Intent.Medicine.ItemId,Id); TestEqual(TEXT("Quantity from actual graph"),Intent.Medicine.ExpectedQuantity,10);
	if (!TestTrue(TEXT("Durable intent"),FShanmenDemo20WorldCheckpointStore::Save(F.Root,C,Intent,F.Why))) return false;
	{ TGuardValue<bool> Fault(FShanmenDemo20WorldCheckpointStore::bFailBeforeReplace,true);
		TestFalse(TEXT("Item deducted, failed HP save does not pretend healing"),FShanmenDemo20Medicine::Recover(F.Root,C,F.MedicinePorts(),F.Why)); }
	TestEqual(TEXT("HP still pre-confirmation"),C.Combat.Health[0],HP);
	if (!TestTrue(TEXT("Reopen exact intent and recover existing consumption"),F.Restart() && FShanmenDemo20WorldCheckpointStore::Load(F.Root,F.Run,C,F.Why)
		&& FShanmenDemo20Medicine::Recover(F.Root,C,F.MedicinePorts(),F.Why))) return false;
	const auto S=F.Snapshot(); TestEqual(TEXT("Mixed ten becomes nine once"),F.Find(S,Id)->Quantity,9);
	TestTrue(TEXT("HP increases thirty-five once"),FMath::IsNearlyEqual(C.Combat.Health[0],HP+35));
	TestTrue(TEXT("No second consume across duplicate recovery"),FShanmenDemo20Medicine::Recover(F.Root,C,F.MedicinePorts(),F.Why) && S==F.Snapshot()); return true;
}
#endif
