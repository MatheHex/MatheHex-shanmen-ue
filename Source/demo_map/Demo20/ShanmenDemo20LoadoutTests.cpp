#if WITH_DEV_AUTOMATION_TESTS
#include "ShanmenDemo20Loadout.h"
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
		return FShanmenItemStorageContext::ForRoot(FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Automation/Demo20.M1.Loadout"),
			Label, FGuid::NewGuid().ToString(EGuidFormats::Digits)), FShanmenDemo20Catalog::OwnerId());
	}
	FShanmenOperationContext Context(const FShanmenItemAuthoritySnapshot& S)
	{
		FShanmenOperationContext C; C.OwnerId = FShanmenDemo20Catalog::OwnerId(); C.RunId = FShanmenDemo20Catalog::ScopeId();
		C.Content = S.Content; C.RequestId = FGuid::NewGuid(); return C;
	}
	FShanmenItemLoadoutStartRequest Plan(const FShanmenItemAuthoritySnapshot& S)
	{
		FShanmenItemLoadoutStartRequest R; FString Reason; FShanmenDemo20Loadout::Build(S,R,Reason); return R;
	}
	const FShanmenItemInstance* Item(const FShanmenItemAuthoritySnapshot& S, FName Def)
	{
		return S.Items.FindByPredicate([&](const auto& I) { return I.DefinitionId == Def && I.State == EShanmenItemInstanceState::Stored; });
	}
	FShanmenItemGridRequest Move(const FShanmenItemAuthoritySnapshot& S, FName Def, FName Role, int32 X, int32 Y)
	{
		FShanmenItemGridRequest R; R.Context = Context(S); R.ExpectedAuthorityRevision = S.AuthorityRevision;
		if (const auto* I = Item(S,Def)) { R.ItemInstanceId = I->ItemInstanceId; R.ExpectedItemRevision = I->Revision; }
		R.DestinationContainerId = FShanmenDemo20Catalog::ContainerId(Role); R.X = X; R.Y = Y; return R;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemo20LoadoutSelectionTest,"Shanmen.Demo20.Preparation.LoadoutSelection",Flags)
bool FDemo20LoadoutSelectionTest::RunTest(const FString&)
{
	auto S = FShanmenDemo20Catalog::Initial(); auto R = Plan(S);
	TestEqual(TEXT("Only three equipped ordinary items and carry pill"),R.Lines.Num(),4);
	for (const auto& L : R.Lines)
	{
		const auto* I = S.Items.FindByPredicate([&](const auto& V) { return V.ItemInstanceId == L.ItemInstanceId; });
		TestTrue(TEXT("No stash, wallet, secure gear or secure contents admitted"),I && I->ParentContainerId != FShanmenDemo20Catalog::ContainerId(TEXT("Stash"))
			&& I->ParentContainerId != FShanmenDemo20Catalog::ContainerId(TEXT("Wallet")) && I->ParentContainerId != FShanmenDemo20Catalog::ContainerId(TEXT("Secure"))
			&& I->ParentContainerId != FShanmenDemo20Catalog::ContainerId(TEXT("SecureBox")));
		TestEqual(TEXT("Exact prepared placement"),L.PurposeId,FShanmenItemReservationPlacement::Encode(TEXT("Demo20.Expedition.Ordinary.r1"),I->ParentContainerId,I->SlotIndex));
	}
	TestTrue(TEXT("Readable, real nonzero baseline summary"),FShanmenDemo20Loadout::Summary(S).Contains(TEXT("3 件普通装备 / 8 件物资")));
	Algo::Reverse(S.Items); Algo::Reverse(S.Containers);
	const auto Reordered = Plan(S);
	TestEqual(TEXT("Array order does not alter start identity"),Reordered.Context.RequestId,R.Context.RequestId);
	TestEqual(TEXT("Canonical first line"),Reordered.Lines[0].ItemInstanceId,R.Lines[0].ItemInstanceId);
	FShanmenItemRepository Repo; Repo.TryLoadSnapshot(S);
	TestTrue(TEXT("Unequip armor through real grid command"),Repo.EditGrid(Move(S,TEXT("Armor.Robe"),TEXT("Stash"),0,6)).IsSuccess());
	FString Reason; TestFalse(TEXT("Missing armor rejects preparation"),FShanmenDemo20Loadout::Build(Repo.CaptureSnapshot(),R,Reason));
	TestTrue(TEXT("Specific missing equipment reason"),Reason.Contains(TEXT("护具")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemo20AtomicLoadoutTest,"Shanmen.Demo20.Preparation.AtomicLoadoutStart",Flags)
bool FDemo20AtomicLoadoutTest::RunTest(const FString&)
{
	FShanmenItemAuthorityService Service; const auto Storage = Disk(TEXT("Atomic"));
	if (!TestTrue(TEXT("Native profile"),Service.StartNativeProfile(Storage,FShanmenDemo20Catalog::ProductId(),FShanmenDemo20Catalog::Initial()).IsReady())) return false;
	FShanmenItemAuthorityDocument Before; Service.TryGetDocument(Before); const auto R = Plan(Before.Authority);
	const auto Started = Service.StartLoadoutDurable(R); if (!TestTrue(TEXT("One durable bundle"),Started.IsCommandSuccess())) return false;
	FShanmenItemAuthorityDocument After; Service.TryGetDocument(After);
	TestEqual(TEXT("Exactly one new save generation, not one per reserve"),After.SaveGeneration,Before.SaveGeneration+1);
	TestEqual(TEXT("Four reservation receipts retained"),After.Authority.Reservations.Num(),4);
	TestTrue(TEXT("No partial pending state published"),!After.Authority.Reservations.ContainsByPredicate([](const auto& V) { return V.State == EShanmenItemReservationState::Reserved; }));
	TestEqual(TEXT("Existing lifecycle receipt, no new save format"),Started.Receipt.Operation,EShanmenItemTransactionOperation::StartPreparedRun);
	TestTrue(TEXT("Secure original untouched"),*Item(After.Authority,TEXT("Trophy.Jade")) == *Item(Before.Authority,TEXT("Trophy.Jade")));
	FShanmenDemo20ActiveLoadout Active; FString Reason;
	TestTrue(TEXT("Readonly reconstruction"),FShanmenDemo20Loadout::InspectActive(After.Authority,Active,Reason));
	TestEqual(TEXT("Reconstructed real Run"),Active.RunId,Started.Receipt.ReservationId);
	TestEqual(TEXT("Reconstructed exact four original balances"),Active.RemainingOriginals.Num(),4);
	FShanmenItemLoadoutStartRequest Fresh;
	TestFalse(TEXT("Fresh product start denied while active"),FShanmenDemo20Loadout::Build(After.Authority,Fresh,Reason));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemo20LoadoutRollbackTest,"Shanmen.Demo20.Preparation.LoadoutSaveRollback",Flags)
bool FDemo20LoadoutRollbackTest::RunTest(const FString&)
{
	FShanmenItemAuthorityService Service; const auto Storage = Disk(TEXT("Rollback"));
	Service.StartNativeProfile(Storage,FShanmenDemo20Catalog::ProductId(),FShanmenDemo20Catalog::Initial());
	FShanmenItemAuthorityDocument Before,After; Service.TryGetDocument(Before); const auto R = Plan(Before.Authority);
	FString Bytes; FFileHelper::LoadFileToString(Bytes,*Storage.PrimaryPath());
	Service.SetInjectedFailureForTests(EShanmenItemStoreFailureStage::AtomicReplace);
	TestFalse(TEXT("Failed save cannot report started"),Service.StartLoadoutDurable(R).IsCommandSuccess());
	Service.TryGetDocument(After); TestTrue(TEXT("All reserves and lifecycle roll back exactly"),After == Before);
	FString Reopened; FFileHelper::LoadFileToString(Reopened,*Storage.PrimaryPath()); TestEqual(TEXT("Disk unchanged"),Reopened,Bytes);
	Service.SetInjectedFailureForTests(EShanmenItemStoreFailureStage::None);
	TestTrue(TEXT("Retry original intent, not rebuilt genesis"),Service.StartLoadoutDurable(R).IsCommandSuccess());
	Service.TryGetDocument(After); TestEqual(TEXT("Retry advances one generation"),After.SaveGeneration,Before.SaveGeneration+1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemo20LoadoutRejectTest,"Shanmen.Demo20.Preparation.LoadoutCandidateReject",Flags)
bool FDemo20LoadoutRejectTest::RunTest(const FString&)
{
	FShanmenItemRepository Repo; Repo.TryLoadSnapshot(FShanmenDemo20Catalog::Initial()); const auto Before = Repo.CaptureSnapshot();
	auto R = Plan(Before); R.Lines.Last().ExpectedItemRevision += 1;
	TestEqual(TEXT("Late line failure"),Repo.StartLoadout(R).Error,EError::StaleItemRevision);
	TestTrue(TEXT("Earlier successful candidate reserves never leak"),Repo.CaptureSnapshot() == Before);
	R = Plan(Before); const auto Duplicate = R.Lines[0]; R.Lines.Add(Duplicate);
	TestEqual(TEXT("Duplicate identity"),Repo.StartLoadout(R).Error,EError::InvalidRequest);
	R = Plan(Before); R.Lines[0].Context.OwnerId = FGuid::NewGuid(); TestEqual(TEXT("Owner swap"),Repo.StartLoadout(R).Error,EError::InvalidRequest);
	R = Plan(Before); R.Context.RequestId = FGuid::NewGuid(); TestEqual(TEXT("Arbitrary epoch identity"),Repo.StartLoadout(R).Error,EError::InvalidRequest);
	R = Plan(Before); --R.ExpectedAuthorityRevision; R.Context.RequestId = FShanmenItemLoadoutStartRequest::MakeRequestId(R.Context,R.ExpectedAuthorityRevision);
	for (auto& L : R.Lines) L.Context.RequestId = FShanmenItemLoadoutStartRequest::MakeLineRequestId(R.Context.RequestId,L.ItemInstanceId);
	TestEqual(TEXT("Negative initial epoch rejected"),Repo.StartLoadout(R).Error,EError::InvalidRequest);
	TestTrue(TEXT("Every rejection leaves exact nonzero stock and ledger"),Repo.CaptureSnapshot() == Before);
	R = Plan(Before);
	// The 2x2 scroll occupies columns 7-8. Use a genuinely empty destination.
	TestTrue(TEXT("Actual intervening grid edit"),Repo.EditGrid(Move(Before,TEXT("Material.Herb"),TEXT("Stash"),10,4)).IsSuccess());
	const auto Edited = Repo.CaptureSnapshot();
	TestEqual(TEXT("Old frozen preparation epoch cannot overwrite edit"),Repo.StartLoadout(R).Error,EError::StaleAuthorityRevision);
	TestTrue(TEXT("Stale bundle leaves newer placement intact"),Repo.CaptureSnapshot() == Edited);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemo20LoadoutReplayTest,"Shanmen.Demo20.Preparation.LoadoutReplayAndTamper",Flags)
bool FDemo20LoadoutReplayTest::RunTest(const FString&)
{
	FShanmenItemAuthorityService Service; const auto Storage = Disk(TEXT("Replay"));
	Service.StartNativeProfile(Storage,FShanmenDemo20Catalog::ProductId(),FShanmenDemo20Catalog::Initial());
	FShanmenItemAuthoritySnapshot S; Service.TryCaptureSnapshot(S); auto R = Plan(S); const auto Started = Service.StartLoadoutDurable(R);
	if (!TestTrue(TEXT("Started"),Started.IsCommandSuccess())) return false;
	FShanmenItemAuthorityDocument Before,After; Service.TryGetDocument(Before);
	Algo::Reverse(R.Lines); TestTrue(TEXT("Reordered duplicate exact replay"),Service.StartLoadoutDurable(R).Receipt == Started.Receipt);
	auto Tamper = R; Tamper.Lines[0].PurposeId = TEXT("Other.Purpose");
	TestEqual(TEXT("Same reserve ID, changed semantic line"),Service.StartLoadoutDurable(Tamper).Receipt.Error,EError::RequestIdConflict);
	Tamper = R;
	auto* QuantityLine = Tamper.Lines.FindByPredicate([](const auto& L) { return L.ResourceKind == EShanmenItemResourceKind::Quantity; });
	if (!TestNotNull(TEXT("Nonzero quantity line for valid payload tamper"),QuantityLine)) return false;
	--QuantityLine->Amount;
	TestEqual(TEXT("Changed amount cannot replay batch success"),Service.StartLoadoutDurable(Tamper).Receipt.Error,EError::RequestIdConflict);
	Tamper = R;
	auto* EquipmentLine = Tamper.Lines.FindByPredicate([](const auto& L) { return L.ResourceKind == EShanmenItemResourceKind::DeploymentLock; });
	if (!TestNotNull(TEXT("Deployment line for malformed payload"),EquipmentLine)) return false;
	EquipmentLine->Amount = 2;
	TestEqual(TEXT("Malformed deployment amount fails before replay"),Service.StartLoadoutDurable(Tamper).Receipt.Error,EError::InvalidRequest);
	Tamper = R; ++Tamper.Lines[0].ExpectedItemRevision;
	TestEqual(TEXT("Same derived reserve ID but changed immutable revision cannot replay"),Service.StartLoadoutDurable(Tamper).Receipt.Error,EError::RequestIdConflict);
	Service.TryGetDocument(After); TestTrue(TEXT("Replays and conflicts write nothing"),After == Before);
	FShanmenItemAuthorityService Restart;
	TestTrue(TEXT("Native restart"),Restart.StartNativeProfile(Storage,FShanmenDemo20Catalog::ProductId(),FShanmenDemo20Catalog::Initial()).IsReady());
	TestTrue(TEXT("Same bundle restart replay"),Restart.StartLoadoutDurable(R).Receipt == Started.Receipt);
	Restart.TryGetDocument(After); TestTrue(TEXT("Exact durable doc survives"),After == Before);
	// A new valid epoch must not adopt or publish a second active Run.
	R.ExpectedAuthorityRevision = After.Authority.AuthorityRevision;
	R.Context.RequestId = FShanmenItemLoadoutStartRequest::MakeRequestId(R.Context,R.ExpectedAuthorityRevision);
	for (auto& L : R.Lines) { L.Context.RequestId = FShanmenItemLoadoutStartRequest::MakeLineRequestId(R.Context.RequestId,L.ItemInstanceId); ++L.ExpectedItemRevision; }
	TestFalse(TEXT("Second active Run rejected"),Restart.StartLoadoutDurable(R).IsCommandSuccess());
	Restart.TryGetDocument(After); TestTrue(TEXT("No secondary reserves escape rejection"),After == Before);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemo20LoadoutRecoveryTest,"Shanmen.Demo20.Preparation.LoadoutBalancesAndSeedRecovery",Flags)
bool FDemo20LoadoutRecoveryTest::RunTest(const FString&)
{
	FShanmenItemAuthorityService Service; const auto Storage = Disk(TEXT("Recovery"));
	Service.StartNativeProfile(Storage,FShanmenDemo20Catalog::ProductId(),FShanmenDemo20Catalog::Initial());
	FShanmenItemAuthoritySnapshot S; Service.TryCaptureSnapshot(S); const auto PillId = Item(S,TEXT("Heal.Pill"))->ItemInstanceId;
	const auto Started = Service.StartLoadoutDurable(Plan(S)); if (!Started.IsCommandSuccess()) return false;
	Service.TryCaptureSnapshot(S); FShanmenDemo20ActiveLoadout Active; FString Reason;
	if (!TestTrue(TEXT("Initial loadout"),FShanmenDemo20Loadout::InspectActive(S,Active,Reason))) return false;
	const auto Seed = Active.RunSeed;
	FShanmenItemRunConsumeRequest Use; Use.Context = Context(S); Use.ActiveRunId = Active.RunId; Use.ItemInstanceId = PillId;
	Use.Amount = 3; Use.ExpectedQuantityBefore = 8; Use.PurposeId = TEXT("Demo20.Heal.r1");
	if (!TestTrue(TEXT("Durable nonzero pill consume"),Service.ConsumePreparedRunItemDurable(Use).IsCommandSuccess())) return false;
	Service.TryCaptureSnapshot(S); const auto Consumed = S;
	FShanmenItemAuthorityService Restart; Restart.StartNativeProfile(Storage,FShanmenDemo20Catalog::ProductId(),FShanmenDemo20Catalog::Initial());
	Restart.TryCaptureSnapshot(S); TestTrue(TEXT("No genesis refill"),S == Consumed);
	TestTrue(TEXT("Reconstructed balance"),FShanmenDemo20Loadout::InspectActive(S,Active,Reason));
	TestEqual(TEXT("Seed survives UI-independent restart"),Active.RunSeed,Seed);
	const auto* Pill = Active.RemainingOriginals.FindByPredicate([&](const auto& L) { return L.ItemInstanceId == PillId; });
	TestTrue(TEXT("Five, not eight, remaining"),Pill && Pill->RemainingQuantity == 5);
	FShanmenItemRunFinalizeRequest End; End.Context = Context(S); End.ActiveRunId = Active.RunId;
	End.TerminalReason = EShanmenItemRunTerminalReason::Extraction; End.SecuredOriginals = Active.RemainingOriginals;
	TestTrue(TEXT("Extract original stock via sole authority"),Restart.FinalizePreparedRunDurable(End).IsCommandSuccess());
	Restart.TryCaptureSnapshot(S); TestEqual(TEXT("Exactly five returned to Carry"),Item(S,TEXT("Heal.Pill"))->Quantity,5);
	const auto Next = Restart.StartLoadoutDurable(Plan(S)); TestTrue(TEXT("Next distinct real Run"),Next.IsCommandSuccess() && Next.Receipt.ReservationId != Active.RunId);
	Restart.TryCaptureSnapshot(S); FShanmenDemo20ActiveLoadout NextActive;
	TestTrue(TEXT("Next inspect"),FShanmenDemo20Loadout::InspectActive(S,NextActive,Reason));
	TestTrue(TEXT("New deterministic independent seed"),NextActive.RunSeed != Seed);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemo20LoadoutSecureTest,"Shanmen.Demo20.Preparation.LoadoutDeathSecureAndStash",Flags)
bool FDemo20LoadoutSecureTest::RunTest(const FString&)
{
	FShanmenItemAuthorityService Service; const auto Storage = Disk(TEXT("Secure"));
	Service.StartNativeProfile(Storage,FShanmenDemo20Catalog::ProductId(),FShanmenDemo20Catalog::Initial());
	FShanmenItemAuthoritySnapshot S; Service.TryCaptureSnapshot(S);
	auto Split = Move(S,TEXT("Heal.Pill"),TEXT("Secure"),1,0); Split.Action = EShanmenItemGridAction::Split; Split.Amount = 3;
	if (!TestTrue(TEXT("Confirm three pills into Secure via durable grid"),Service.EditGridDurable(Split).IsCommandSuccess())) return false;
	Service.TryCaptureSnapshot(S); const auto Before = S;
	const auto Started = Service.StartLoadoutDurable(Plan(S)); if (!TestTrue(TEXT("Atomic ordinary loadout"),Started.IsCommandSuccess())) return false;
	Service.TryCaptureSnapshot(S); const auto During = S;
	auto Bypass = Move(S,TEXT("Trophy.Jade"),TEXT("Carry"),1,0);
	TestEqual(TEXT("Preparation cannot move secure stock during active Run"),Service.EditGridDurable(Bypass).Receipt.Error,EError::ActiveRunConflict);
	Service.TryCaptureSnapshot(S); TestTrue(TEXT("No bypass mutation"),S == During);
	FShanmenItemRunFinalizeRequest Death; Death.Context = Context(S); Death.ActiveRunId = Started.Receipt.ReservationId; Death.TerminalReason = EShanmenItemRunTerminalReason::Death;
	if (!TestTrue(TEXT("Death through existing terminal port"),Service.FinalizePreparedRunDurable(Death).IsCommandSuccess())) return false;
	Service.TryCaptureSnapshot(S);
	for (const auto& I : Before.Items)
	{
		const bool Protected = I.ParentContainerId == FShanmenDemo20Catalog::ContainerId(TEXT("Secure"))
			|| I.ParentContainerId == FShanmenDemo20Catalog::ContainerId(TEXT("SecureBox"))
			|| I.ParentContainerId == FShanmenDemo20Catalog::ContainerId(TEXT("Stash")) || I.ParentContainerId == FShanmenDemo20Catalog::ContainerId(TEXT("Wallet"));
		const auto* After = S.Items.FindByPredicate([&](const auto& V) { return V.ItemInstanceId == I.ItemInstanceId; });
		if (Protected) TestTrue(TEXT("Exact nonzero protected item identity, quantity, placement and revision"),After && *After == I);
		else TestTrue(TEXT("Ordinary item loss tombstone"),After && After->State == EShanmenItemInstanceState::Destroyed && After->Quantity == 0);
	}
	const auto Final = S; TestTrue(TEXT("Repeated death replay"),Service.FinalizePreparedRunDurable(Death).IsCommandSuccess());
	Service.TryCaptureSnapshot(S); TestTrue(TEXT("No double retention or loss"),S == Final);
	// Active-Run preparation lock disappears after confirmed terminal save.
	TestTrue(TEXT("Secure edit legal again after terminal"),Service.EditGridDurable(Move(S,TEXT("Trophy.Jade"),TEXT("Stash"),2,6)).IsCommandSuccess());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemo20LoadoutSubsystemTest,"Shanmen.Demo20.Preparation.LoadoutThroughExistingSubsystem",Flags)
bool FDemo20LoadoutSubsystemTest::RunTest(const FString&)
{
	auto* GI = NewObject<UGameInstance>(); GI->Init();
	auto* Authority = GI->GetSubsystem<Udemo_mapShanmenItemAuthoritySubsystem>();
	if (!TestNotNull(TEXT("Real initialized GameInstance subsystem"),Authority)) { GI->Shutdown(); return false; }
	const auto Storage = Disk(TEXT("Subsystem"));
	const auto Bound = Authority->BindNativeProfile(Fdemo_mapProfileStorageContext::ForRoot(Storage.RootDirectory),FShanmenDemo20Catalog::OwnerId(),
		FShanmenDemo20Catalog::ProductId(),FShanmenDemo20Catalog::Initial());
	if (!TestTrue(TEXT("Existing GameInstance native binding"),Bound.IsReady())) { GI->Shutdown(); return false; }
	FShanmenItemAuthoritySnapshot S; Authority->TryCaptureSnapshot(S); const auto R = Plan(S);
	const auto Started = Authority->StartLoadoutDurable(R);
	TestTrue(TEXT("Product start uses same subsystem facade"),Started.IsCommandSuccess());
	Authority->TryCaptureSnapshot(S); FShanmenDemo20ActiveLoadout Active; FString Reason;
	TestTrue(TEXT("Same ledger projection"),FShanmenDemo20Loadout::InspectActive(S,Active,Reason));
	TestEqual(TEXT("Same Run"),Active.RunId,Started.Receipt.ReservationId);
	GI->Shutdown(); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemo20LoadoutIntentTest,"Shanmen.Demo20.Preparation.LoadoutPendingIntentFailsClosed",Flags)
bool FDemo20LoadoutIntentTest::RunTest(const FString&)
{
	FShanmenItemRepository Repo; Repo.TryLoadSnapshot(FShanmenDemo20Catalog::Initial()); auto S = Repo.CaptureSnapshot();
	FShanmenItemReserveRequest Other; Other.Context = Context(S); Other.ItemInstanceId = Item(S,TEXT("Sword.Heavy"))->ItemInstanceId;
	Other.ExpectedItemRevision = Item(S,TEXT("Sword.Heavy"))->Revision; Other.Amount = 1; Other.ResourceKind = EShanmenItemResourceKind::DeploymentLock; Other.PurposeId = TEXT("Other.Preparation");
	TestTrue(TEXT("Existing unrelated real reservation"),Repo.Reserve(Other).IsSuccess()); S = Repo.CaptureSnapshot();
	TestEqual(TEXT("Bundle cannot strand pending preparation"),Repo.StartLoadout(Plan(S)).Error,EError::ActiveRunConflict);
	TestTrue(TEXT("No new reserves leaked"),Repo.CaptureSnapshot() == S);
	Repo.TryLoadSnapshot(FShanmenDemo20Catalog::Initial()); S = Repo.CaptureSnapshot(); const auto PillId = Item(S,TEXT("Heal.Pill"))->ItemInstanceId;
	const auto Started = Repo.StartLoadout(Plan(S)); if (!TestTrue(TEXT("Atomic start"),Started.IsSuccess())) return false;
	S = Repo.CaptureSnapshot();
	FShanmenItemRunQuantityIntentRequest Prepare; Prepare.Context = Context(S); Prepare.ActiveRunId = Started.ReservationId;
	Prepare.ItemInstanceId = PillId; Prepare.IntentId = FGuid::NewGuid(); Prepare.Amount = 2; Prepare.ExpectedQuantityBefore = 8; Prepare.PurposeId = TEXT("Demo20.Heal.r1");
	if (!TestTrue(TEXT("Real pending quantity intent"),Repo.PreparePreparedRunQuantityIntent(Prepare).IsSuccess())) return false;
	S = Repo.CaptureSnapshot(); FShanmenDemo20ActiveLoadout Active; FString Reason;
	TestFalse(TEXT("Readonly resume cannot pretend unresolved use was free"),FShanmenDemo20Loadout::InspectActive(S,Active,Reason));
	TestTrue(TEXT("Explicit recovery reason"),Reason.Contains(TEXT("尚待恢复")) && !Active.RunId.IsValid());
	FShanmenItemRunQuantityIntentFinalizeRequest End; End.Context = Context(S); End.ActiveRunId = Started.ReservationId;
	End.PrepareRequestId = Prepare.Context.RequestId; End.IntentId = Prepare.IntentId; End.ItemInstanceId = PillId; End.bCommit = true;
	TestTrue(TEXT("Confirm intent through existing port"),Repo.FinalizePreparedRunQuantityIntent(End).IsSuccess());
	TestTrue(TEXT("Resume confirmed balance"),FShanmenDemo20Loadout::InspectActive(Repo.CaptureSnapshot(),Active,Reason));
	const auto* Pill = Active.RemainingOriginals.FindByPredicate([&](const auto& L) { return L.ItemInstanceId == PillId; });
	TestTrue(TEXT("Six remaining after intent confirmation"),Pill && Pill->RemainingQuantity == 6);
	return true;
}
#endif
