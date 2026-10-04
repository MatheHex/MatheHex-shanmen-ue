#if WITH_DEV_AUTOMATION_TESTS
#include "ShanmenDemo20Catalog.h"
#include "ShanmenItemAuthorityService.h"
#include "demo_mapShanmenItemAuthoritySubsystem.h"
#include "Engine/GameInstance.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Algo/Reverse.h"

namespace
{
	constexpr auto Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;
	using EError = EShanmenItemTransactionError;
	FShanmenItemStorageContext Disk(const TCHAR* Label)
	{
		return FShanmenItemStorageContext::ForRoot(FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Automation/Demo20.M1.Resupply"),
			Label, FGuid::NewGuid().ToString(EGuidFormats::Digits)), FShanmenDemo20Catalog::OwnerId());
	}
	FShanmenOperationContext Context(const FShanmenItemAuthoritySnapshot& S)
	{
		FShanmenOperationContext C; C.OwnerId = FShanmenDemo20Catalog::OwnerId(); C.RunId = FShanmenDemo20Catalog::ScopeId();
		C.Content = S.Content; C.RequestId = FGuid::NewGuid(); return C;
	}
	/** Exercise the actual durable Reserve -> Start -> Death ports, not fabricated receipts or player-save edits. */
	template<typename TAuthority>
	FShanmenItemDurableCommandResult Lose(TAuthority& Service, bool AllOwnedEquipment = true,
		EShanmenItemRunTerminalReason Reason = EShanmenItemRunTerminalReason::Death, bool LosePills = true)
	{
		FShanmenItemAuthoritySnapshot S; Service.TryCaptureSnapshot(S);
		FShanmenItemRunStartRequest Start; Start.Context = Context(S);
		TArray<FShanmenItemRunSecuredOriginal> Originals;
		for (const auto& I : S.Items)
		{
			const auto* F = S.Grid.Footprints.FindByPredicate([&](const auto& V) { return V.DefinitionId == I.DefinitionId; });
			const bool Equipment = F && (F->EquipmentRole == TEXT("Weapon") || F->EquipmentRole == TEXT("Armor") || F->EquipmentRole == TEXT("Backpack"));
			if (I.State != EShanmenItemInstanceState::Stored || (!Equipment && (I.DefinitionId != TEXT("Heal.Pill") || !LosePills))) continue;
			if (Equipment && !AllOwnedEquipment && I.ParentContainerId == FShanmenDemo20Catalog::ContainerId(TEXT("Stash"))) continue;
			FShanmenItemReserveRequest R; R.Context = Context(S); R.ItemInstanceId = I.ItemInstanceId; R.ExpectedItemRevision = I.Revision;
			R.ResourceKind = Equipment ? EShanmenItemResourceKind::DeploymentLock : EShanmenItemResourceKind::Quantity;
			R.Amount = Equipment ? 1 : I.Quantity;
			R.PurposeId = Equipment ? FName(TEXT("Demo20.Equipment")) : FShanmenItemReservationPlacement::Encode(TEXT("Demo20.Pill"), I.ParentContainerId, I.SlotIndex);
			const auto Reserved = Service.ReserveDurable(R);
			if (!Reserved.IsCommandSuccess()) return Reserved;
			Start.ReservationIds.Add(Reserved.Receipt.ReservationId);
			FShanmenItemRunSecuredOriginal Original; Original.ItemInstanceId = I.ItemInstanceId; Original.RemainingQuantity = I.Quantity; Originals.Add(Original);
		}
		const auto Active = Service.StartPreparedRunDurable(Start); if (!Active.IsCommandSuccess()) return Active;
		FShanmenItemRunFinalizeRequest Finish; Finish.Context = Context(S); Finish.ActiveRunId = Active.Receipt.ReservationId; Finish.TerminalReason = Reason;
		if (Reason == EShanmenItemRunTerminalReason::Extraction) Finish.SecuredOriginals = Originals;
		return Service.FinalizePreparedRunDurable(Finish);
	}
	int32 Count(const FShanmenItemAuthoritySnapshot& S, FName Definition)
	{
		int32 Total = 0; for (const auto& I : S.Items) if (I.DefinitionId == Definition && I.State == EShanmenItemInstanceState::Stored) Total += I.Quantity;
		return Total;
	}
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemo20FiniteSupplyTest,"Shanmen.Demo20.Preparation.FiniteDeathSupply",Flags)
bool FDemo20FiniteSupplyTest::RunTest(const FString&)
{
	FShanmenItemAuthorityService Service; const auto Storage = Disk(TEXT("Finite"));
	if (!TestTrue(TEXT("Native profile"), Service.StartNativeProfile(Storage,FShanmenDemo20Catalog::ProductId(),FShanmenDemo20Catalog::Initial()).IsReady())) return false;
	const auto Death = Lose(Service); if (!TestTrue(TEXT("Real prepared Run death with grid/storage succeeds"),Death.IsCommandSuccess())) return false;
	FShanmenItemAuthoritySnapshot S; Service.TryCaptureSnapshot(S);
	const auto* Carry = S.Grid.Layouts.FindByPredicate([](const auto& L) { return L.Kind == EShanmenItemGridKind::Carry; });
	TestTrue(TEXT("Lost bag disables empty carry in same terminal generation"),Carry && Carry->Width == 1 && Carry->Height == 1 && FShanmenItemGridPolicy::Validate(S));
	TestEqual(TEXT("Death did not resurrect ordinary carried pills"),Count(S,TEXT("Heal.Pill")),0);
	TestEqual(TEXT("Secure gear retained when not lost"),Count(S,TEXT("Secure.Basic")),1);
	TestEqual(TEXT("Secure original retained"),Count(S,TEXT("Trophy.Jade")),1);
	const auto Supply = FShanmenDemo20Catalog::BasicSupply(S); const auto Granted = Service.ReplenishBasicsDurable(Supply);
	if (!TestTrue(TEXT("Atomic four-line finite supply"),Granted.IsCommandSuccess() && Granted.Receipt.Amount == 4)) return false;
	FShanmenItemAuthorityDocument After; Service.TryGetDocument(After);
	TestEqual(TEXT("Basic sword only"),Count(After.Authority,TEXT("Sword.Plain")),1);
	TestEqual(TEXT("Two finite pills, not original eight"),Count(After.Authority,TEXT("Heal.Pill")),2);
	TestEqual(TEXT("No expensive sword restored"),Count(After.Authority,TEXT("Sword.Heavy")),0);
	TestEqual(TEXT("Money unchanged"),Count(After.Authority,TEXT("Currency.Test")),1000000);
	const auto* RestoredCarry = After.Authority.Grid.Layouts.FindByPredicate([](const auto& L) { return L.Kind == EShanmenItemGridKind::Carry; });
	TestTrue(TEXT("Basic bag restores 6x4"),RestoredCarry && RestoredCarry->Width == 6 && RestoredCarry->Height == 4);
	auto Repeated = FShanmenDemo20Catalog::BasicSupply(After.Authority); Algo::Reverse(Repeated.Lines);
	TestTrue(TEXT("New click/current CAS and reordered policy replay exact receipt"),Service.ReplenishBasicsDurable(Repeated).Receipt == Granted.Receipt);
	FShanmenItemAuthorityDocument Replay; Service.TryGetDocument(Replay); TestTrue(TEXT("No extra generation on repeat"),Replay == After);
	auto Forged = Repeated; Forged.Context.RequestId = FGuid::NewGuid();
	TestEqual(TEXT("Arbitrary request cannot bypass epoch"),Service.ReplenishBasicsDurable(Forged).Receipt.Error,EError::InvalidRequest);
	Forged = Repeated; Forged.PolicyId = TEXT("Other.Policy"); Forged.Context.RequestId = FShanmenItemBasicSupplyRequest::MakeRequestId(Forged.Context.OwnerId,Forged.Context.RunId,Forged.DeathRequestId,Forged.PolicyId);
	TestEqual(TEXT("Changing policy cannot obtain another grant"),Service.ReplenishBasicsDurable(Forged).Receipt.Error,EError::BasicSupplyAlreadyUsed);
	FShanmenItemAuthorityService Restart; TestTrue(TEXT("Native reopen retains origin/catalog"),Restart.StartNativeProfile(Storage,FShanmenDemo20Catalog::ProductId(),FShanmenDemo20Catalog::Initial()).IsReady());
	TestTrue(TEXT("Replay after restart"),Restart.ReplenishBasicsDurable(Repeated).Receipt == Granted.Receipt);
	Restart.TryGetDocument(Replay); TestTrue(TEXT("Exact restarted document"),Replay == After);
	// A later actual loss opens a fresh finite opportunity; the old event cannot grant again.
	TestTrue(TEXT("Next real prepared Run and death"),Lose(Restart).IsCommandSuccess());
	Restart.TryCaptureSnapshot(S); const auto Next = Restart.ReplenishBasicsDurable(FShanmenDemo20Catalog::BasicSupply(S));
	TestTrue(TEXT("Next death gets distinct stable identity"),Next.IsCommandSuccess() && Next.Receipt.RequestId != Granted.Receipt.RequestId);
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemo20AlternativeSupplyTest,"Shanmen.Demo20.Preparation.AlternativeStockPreventsFarming",Flags)
bool FDemo20AlternativeSupplyTest::RunTest(const FString&)
{
	FShanmenItemAuthorityService Service; Service.StartNativeProfile(Disk(TEXT("Alternatives")),FShanmenDemo20Catalog::ProductId(),FShanmenDemo20Catalog::Initial());
	if (!TestTrue(TEXT("Lose equipped basics only"),Lose(Service,false).IsCommandSuccess())) return false;
	FShanmenItemAuthoritySnapshot S; Service.TryCaptureSnapshot(S);
	const auto Result = Service.ReplenishBasicsDurable(FShanmenDemo20Catalog::BasicSupply(S));
	TestTrue(TEXT("Alternative sword/armor/bag in stash count as owned; only missing medicine supplied"),Result.IsCommandSuccess() && Result.Receipt.Amount == 1);
	Service.TryCaptureSnapshot(S);
	TestEqual(TEXT("No extra baseline sword"),Count(S,TEXT("Sword.Plain")),0);
	TestEqual(TEXT("No extra baseline bag"),Count(S,TEXT("Backpack.Small")),0);
	TestEqual(TEXT("Stash alternative retained"),Count(S,TEXT("Sword.Heavy")),1);
	TestEqual(TEXT("Finite medicine"),Count(S,TEXT("Heal.Pill")),2);
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemo20SupplyFailureTest,"Shanmen.Demo20.Preparation.SupplySaveRollback",Flags)
bool FDemo20SupplyFailureTest::RunTest(const FString&)
{
	FShanmenItemAuthorityService Service; const auto Storage = Disk(TEXT("Rollback"));
	Service.StartNativeProfile(Storage,FShanmenDemo20Catalog::ProductId(),FShanmenDemo20Catalog::Initial());
	if (!TestTrue(TEXT("Death"),Lose(Service).IsCommandSuccess())) return false;
	FShanmenItemAuthorityDocument Before; Service.TryGetDocument(Before); const auto Request = FShanmenDemo20Catalog::BasicSupply(Before.Authority);
	Service.SetInjectedFailureForTests(EShanmenItemStoreFailureStage::AtomicReplace);
	const auto Failed = Service.ReplenishBasicsDurable(Request);
	TestTrue(TEXT("No false success on publication failure"),!Failed.IsCommandSuccess() && Failed.Status == EShanmenItemDurableCommandStatus::PersistenceFailedRolledBack);
	FShanmenItemAuthorityDocument After; Service.TryGetDocument(After); TestTrue(TEXT("All quantities/grid/epoch/ledger rolled back"),After == Before);
	Service.SetInjectedFailureForTests(EShanmenItemStoreFailureStage::None);
	TestTrue(TEXT("Same request retry remains eligible"),Service.ReplenishBasicsDurable(Request).IsCommandSuccess());
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemo20SupplyEligibilityTest,"Shanmen.Demo20.Preparation.SupplyEligibilityAndCAS",Flags)
bool FDemo20SupplyEligibilityTest::RunTest(const FString&)
{
	FShanmenItemAuthorityService Service; Service.StartNativeProfile(Disk(TEXT("Eligibility")),FShanmenDemo20Catalog::ProductId(),FShanmenDemo20Catalog::Initial());
	FShanmenItemAuthoritySnapshot S; Service.TryCaptureSnapshot(S); const auto Genesis = S;
	auto Request = FShanmenDemo20Catalog::BasicSupply(S);
	TestFalse(TEXT("New profile cannot claim assistance"),Service.ReplenishBasicsDurable(Request).IsCommandSuccess());
	Service.TryCaptureSnapshot(S); TestTrue(TEXT("Invalid claim does not change genesis"),S == Genesis);
	if (!TestTrue(TEXT("Death"),Lose(Service).IsCommandSuccess())) return false;
	Service.TryCaptureSnapshot(S); Request = FShanmenDemo20Catalog::BasicSupply(S);
	auto Stale = Request; --Stale.ExpectedAuthorityRevision;
	TestEqual(TEXT("Stale intent refused"),Service.ReplenishBasicsDurable(Stale).Receipt.Error,EError::StaleAuthorityRevision);
	auto Foreign = Request; Foreign.Context.OwnerId = FGuid::NewGuid(); Foreign.Context.RequestId = FShanmenItemBasicSupplyRequest::MakeRequestId(Foreign.Context.OwnerId,Foreign.Context.RunId,Foreign.DeathRequestId,Foreign.PolicyId);
	TestEqual(TEXT("Foreign owner refused"),Service.ReplenishBasicsDurable(Foreign).Receipt.Error,EError::ScopeMismatch);
	Foreign = Request; Foreign.Context.RunId = FGuid::NewGuid(); Foreign.Context.RequestId = FShanmenItemBasicSupplyRequest::MakeRequestId(Foreign.Context.OwnerId,Foreign.Context.RunId,Foreign.DeathRequestId,Foreign.PolicyId);
	TestEqual(TEXT("Foreign scope refused"),Service.ReplenishBasicsDurable(Foreign).Receipt.Error,EError::ScopeMismatch);
	TestTrue(TEXT("Stale/foreign rejections preserve valid retry"),Service.ReplenishBasicsDurable(Request).IsCommandSuccess());
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemo20SupplyNoNeedTest,"Shanmen.Demo20.Preparation.SupplyNoNeedAndPendingRun",Flags)
bool FDemo20SupplyNoNeedTest::RunTest(const FString&)
{
	FShanmenItemAuthorityService Service; Service.StartNativeProfile(Disk(TEXT("NoNeed")),FShanmenDemo20Catalog::ProductId(),FShanmenDemo20Catalog::Initial());
	// Empty carry legitimately before departing with equipment only.
	FShanmenItemAuthoritySnapshot S; Service.TryCaptureSnapshot(S);
	const auto* Pill = S.Items.FindByPredicate([](const auto& I) { return I.DefinitionId == TEXT("Heal.Pill"); });
	FShanmenItemGridRequest Move; Move.Context = Context(S); Move.ItemInstanceId = Pill->ItemInstanceId; Move.ExpectedItemRevision = Pill->Revision;
	Move.ExpectedAuthorityRevision = S.AuthorityRevision; Move.DestinationContainerId = FShanmenDemo20Catalog::ContainerId(TEXT("Stash")); Move.X = 8; Move.Y = 0;
	if (!TestTrue(TEXT("Store pills before actual equipment-only departure"),Service.EditGridDurable(Move).IsCommandSuccess())) return false;
	if (!TestTrue(TEXT("Death with alternatives and stored medicine spared"),Lose(Service,false,EShanmenItemRunTerminalReason::Death,false).IsCommandSuccess())) return false;
	FShanmenItemAuthorityDocument Before; Service.TryGetDocument(Before); auto Request = FShanmenDemo20Catalog::BasicSupply(Before.Authority);
	TestEqual(TEXT("No missing basics is a no-op refusal"),Service.ReplenishBasicsDurable(Request).Receipt.Error,EError::BasicSupplyNotNeeded);
	FShanmenItemAuthorityDocument After; Service.TryGetDocument(After); TestTrue(TEXT("No epoch use, no generation, no money change"),Before == After);
	const auto* Heavy = After.Authority.Items.FindByPredicate([](const auto& I) { return I.DefinitionId == TEXT("Sword.Heavy") && I.State == EShanmenItemInstanceState::Stored; });
	FShanmenItemReserveRequest Reserve; Reserve.Context = Context(After.Authority); Reserve.ItemInstanceId = Heavy->ItemInstanceId;
	Reserve.ExpectedItemRevision = Heavy->Revision; Reserve.ResourceKind = EShanmenItemResourceKind::DeploymentLock; Reserve.PurposeId = TEXT("Demo20.NextRun");
	const auto Reserved = Service.ReserveDurable(Reserve); if (!TestTrue(TEXT("Prepare next departure"),Reserved.IsCommandSuccess())) return false;
	Service.TryCaptureSnapshot(S); Request.ExpectedAuthorityRevision = S.AuthorityRevision;
	TestEqual(TEXT("Pending preparation blocks old death assistance"),Service.ReplenishBasicsDurable(Request).Receipt.Error,EError::BasicSupplyNotEligible);
	FShanmenItemRunStartRequest Start; Start.Context = Context(S); Start.ReservationIds = {Reserved.Receipt.ReservationId};
	if (!TestTrue(TEXT("Next active Run"),Service.StartPreparedRunDurable(Start).IsCommandSuccess())) return false;
	Service.TryCaptureSnapshot(S); Request.ExpectedAuthorityRevision = S.AuthorityRevision;
	TestEqual(TEXT("Later active Run also blocks old epoch"),Service.ReplenishBasicsDurable(Request).Receipt.Error,EError::BasicSupplyNotEligible);
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemo20SupplyTerminalTest,"Shanmen.Demo20.Preparation.SupplyRejectsNonDeathAndTamper",Flags)
bool FDemo20SupplyTerminalTest::RunTest(const FString&)
{
	FShanmenItemAuthorityService Service; Service.StartNativeProfile(Disk(TEXT("NonDeath")),FShanmenDemo20Catalog::ProductId(),FShanmenDemo20Catalog::Initial());
	const auto Extracted = Lose(Service,true,EShanmenItemRunTerminalReason::Extraction);
	if (!TestTrue(TEXT("Formal extraction"),Extracted.IsCommandSuccess())) return false;
	FShanmenItemAuthoritySnapshot S; Service.TryCaptureSnapshot(S); auto Request = FShanmenDemo20Catalog::BasicSupply(S);
	Request.DeathRequestId = Extracted.Receipt.RequestId; Request.Context.RequestId = FShanmenItemBasicSupplyRequest::MakeRequestId(Request.Context.OwnerId,Request.Context.RunId,Request.DeathRequestId,Request.PolicyId);
	TestEqual(TEXT("Extraction cannot sponsor supply"),Service.ReplenishBasicsDurable(Request).Receipt.Error,EError::BasicSupplyNotEligible);
	if (!TestTrue(TEXT("Real next death"),Lose(Service).IsCommandSuccess())) return false;
	Service.TryCaptureSnapshot(S); Request = FShanmenDemo20Catalog::BasicSupply(S);
	auto Bad = Request; Bad.Lines[0].Quantity = 2;
	TestEqual(TEXT("Equipment amount above its immutable max stack is an invalid request"),Service.ReplenishBasicsDurable(Bad).Receipt.Error,EError::InvalidRequest);
	FShanmenItemAuthoritySnapshot Unchanged; Service.TryCaptureSnapshot(Unchanged); TestTrue(TEXT("Late-line rejection rolls back earlier candidate grants"),Unchanged == S);
	TestTrue(TEXT("Correct request after bad policy not poisoned"),Service.ReplenishBasicsDurable(Request).IsCommandSuccess());
	Service.TryCaptureSnapshot(S); auto Tampered = S;
	auto* P = Tampered.ProcessedRequests.FindByPredicate([](const auto& V) { return V.Receipt.Operation == EShanmenItemTransactionOperation::ReplenishBasics; });
	P->Receipt.ReservationId = Extracted.Receipt.RequestId;
	FShanmenItemRepository Repo; TestFalse(TEXT("Snapshot validator refuses supply linked to extraction"),Repo.TryLoadSnapshot(Tampered));
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemo20SupplySubsystemTest,"Shanmen.Demo20.Preparation.SupplyThroughExistingSubsystem",Flags)
bool FDemo20SupplySubsystemTest::RunTest(const FString&)
{
	auto* GI = NewObject<UGameInstance>(); GI->Init();
	auto* Authority = GI->GetSubsystem<Udemo_mapShanmenItemAuthoritySubsystem>();
	if (!TestNotNull(TEXT("Single existing product authority"),Authority)) { GI->Shutdown(); return false; }
	const auto Storage = Disk(TEXT("Subsystem"));
	TestTrue(TEXT("Native bind"),Authority->BindNativeProfile(Fdemo_mapProfileStorageContext::ForRoot(Storage.RootDirectory),Storage.OwnerId,
		FShanmenDemo20Catalog::ProductId(),FShanmenDemo20Catalog::Initial()).IsReady());
	if (!TestTrue(TEXT("Reserve/start/death through actual product ports"),Lose(*Authority).IsCommandSuccess())) { GI->Shutdown(); return false; }
	FShanmenItemAuthoritySnapshot S; Authority->TryCaptureSnapshot(S);
	const auto Request = FShanmenDemo20Catalog::BasicSupply(S);
	const auto Granted = Authority->ReplenishBasicsDurable(Request);
	TestTrue(TEXT("Product supply durable"),Granted.IsCommandSuccess() && Granted.Receipt.Amount == 4);
	FShanmenItemAuthorityDocument Before,After; Authority->TryGetDocument(Before);
	TestTrue(TEXT("Product exact replay"),Authority->ReplenishBasicsDurable(Request).Receipt == Granted.Receipt);
	Authority->TryGetDocument(After); TestTrue(TEXT("Product replay generation stable"),Before == After);
	GI->Shutdown(); return true;
}
#endif
