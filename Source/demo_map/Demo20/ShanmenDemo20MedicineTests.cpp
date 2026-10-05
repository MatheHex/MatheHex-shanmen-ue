#if WITH_DEV_AUTOMATION_TESTS
#include "ShanmenDemo20Medicine.h"
#include "ShanmenDemo20Loadout.h"
#include "ShanmenDemo20Catalog.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"

namespace
{
	constexpr auto Flags=EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter;
	struct FFixture
	{
		FString Root=FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("Automation/Demo20.M2.Medicine/Native"),FGuid::NewGuid().ToString(EGuidFormats::Digits));
		FShanmenItemStorageContext Disk=FShanmenItemStorageContext::ForRoot(Root,FShanmenDemo20Catalog::OwnerId());
		TUniquePtr<FShanmenItemAuthorityService> Items=MakeUnique<FShanmenItemAuthorityService>();
		FShanmenDemo20WorldCheckpoint C;
		FString Why;
		FShanmenOperationContext Context(const FShanmenItemAuthoritySnapshot& S)
		{ FShanmenOperationContext R; R.OwnerId=FShanmenDemo20Catalog::OwnerId(); R.RunId=FShanmenDemo20Catalog::ScopeId(); R.Content=S.Content; R.RequestId=FGuid::NewGuid(); return R; }
		FShanmenItemAuthoritySnapshot Snapshot() { FShanmenItemAuthoritySnapshot S; Items->TryCaptureSnapshot(S); return S; }
		bool Start(int32 Secure=0, bool EmptyCarry=false)
		{
			if (!Items->StartNativeProfile(Disk,FShanmenDemo20Catalog::ProductId(),FShanmenDemo20Catalog::Initial()).IsReady()) return false;
			auto S=Snapshot();
			if (Secure)
			{
				const auto* Pill=S.Items.FindByPredicate([](const auto& I){return I.DefinitionId==TEXT("Heal.Pill") && I.ParentContainerId==FShanmenDemo20Catalog::ContainerId(TEXT("Carry"));});
				if (!Pill) return false;
				FShanmenItemGridRequest R; R.Context=Context(S); R.ExpectedAuthorityRevision=S.AuthorityRevision;
				R.ItemInstanceId=Pill->ItemInstanceId; R.ExpectedItemRevision=Pill->Revision; R.Action=EShanmenItemGridAction::Split; R.Amount=Secure;
				R.DestinationContainerId=FShanmenDemo20Catalog::ContainerId(TEXT("Secure")); R.X=1; R.Y=0;
				if (!Items->EditGridDurable(R).IsCommandSuccess()) return false; S=Snapshot();
			}
			FShanmenItemLoadoutStartRequest R; if (!FShanmenDemo20Loadout::Build(S,R,Why)) return false;
			const auto Started=Items->StartLoadoutDurable(R); if (!Started.IsCommandSuccess()) return false;
			FShanmenDemo20Session Session; if (!Session.BeginExpedition(Started.Receipt.ReservationId,26.f,.12f)) return false;
			// Nonzero health baseline produced by actual canonical attacks, not 0==0.
			for (int32 I=0; I<4; ++I) if (!Session.ReceiveSentinelStrike(0)) return false;
			auto Fresh=C; Fresh.ContentId=C.LegacyContentId(); Fresh.RunSeed=C.SeedForRun(Started.Receipt.ReservationId); Session.CaptureExpedition(Fresh.Combat);
			if (!FShanmenDemo20WorldCheckpointStore::Save(Root,C,Fresh,Why)) return false;
			if (EmptyCarry)
			{
				S=Snapshot(); FShanmenDemo20ActiveLoadout A; if (!FShanmenDemo20Loadout::InspectActive(S,A,Why)) return false;
				for (const auto& L:A.RemainingOriginals)
				{
					const auto* I=S.Items.FindByPredicate([&](const auto& V){return V.ItemInstanceId==L.ItemInstanceId;});
					if (I && I->DefinitionId==TEXT("Heal.Pill"))
					{ FShanmenItemRunConsumeRequest U; U.Context=Context(S); U.ActiveRunId=A.RunId; U.ItemInstanceId=I->ItemInstanceId;
						U.Amount=L.RemainingQuantity; U.ExpectedQuantityBefore=U.Amount; U.PurposeId=TEXT("Test.EmptyCarry");
						if (!Items->ConsumePreparedRunItemDurable(U).IsCommandSuccess()) return false; }
				}
			}
			return true;
		}
		FShanmenDemo20MedicinePorts Ports()
		{
			FShanmenDemo20MedicinePorts P; P.Capture=[this](auto& S){return Items->TryCaptureSnapshot(S);};
			P.PrepareRun=[this](const auto& R){return Items->PreparePreparedRunQuantityIntentDurable(R);};
			P.FinalizeRun=[this](const auto& R){return Items->FinalizePreparedRunQuantityIntentDurable(R);};
			P.Reserve=[this](const auto& R){return Items->ReserveDurable(R);}; P.Commit=[this](const auto& R){return Items->CommitDurable(R);}; return P;
		}
		bool Intent() { FShanmenDemo20WorldCheckpoint I; return FShanmenDemo20Medicine::BuildIntent(C,Snapshot(),I,Why) && FShanmenDemo20WorldCheckpointStore::Save(Root,C,I,Why); }
		bool Restart() { Items=MakeUnique<FShanmenItemAuthorityService>(); return Items->StartNativeProfile(Disk,FShanmenDemo20Catalog::ProductId(),FShanmenDemo20Catalog::Initial()).IsReady()
			&& FShanmenDemo20WorldCheckpointStore::Load(Root,C.Combat.RunId,C,Why); }
		int32 Quantity(EShanmenDemo20MedicineOrigin O) { TArray<FShanmenDemo20MedicineLine> L; if (!FShanmenDemo20Medicine::Capture(Snapshot(),C.Combat.RunId,L,Why)) return -1;
			int32 N=0; for (const auto& V:L) if (V.Origin==O) N+=V.Quantity; return N; }
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemo20MedicineRejectTest,"Shanmen.Demo20.Expedition.MedicineRejectsAndProjection",Flags)
bool FDemo20MedicineRejectTest::RunTest(const FString&)
{
	FFixture F; if (!TestTrue(TEXT("Finite formal fixture"),F.Start(3))) return false;
	TestEqual(TEXT("Five ordinary, not depleted preparation zero"),F.Quantity(EShanmenDemo20MedicineOrigin::PreparedCarry),5);
	TestEqual(TEXT("Three equipped secure pills, not stash"),F.Quantity(EShanmenDemo20MedicineOrigin::Secure),3);
	FShanmenDemo20Session S; S.RestoreExpedition(F.C.Combat); S.SetGuarding(true); FString Why;
	TestFalse(TEXT("Held guard rejects actual session"),S.TryUseMedicine(Why)); S.SetGuarding(false); S.TryEvade();
	TestFalse(TEXT("Evade rejects"),S.TryUseMedicine(Why)); S.Advance(.4f); S.StrikeSentinel(0);
	TestFalse(TEXT("Sword recovery rejects"),S.TryUseMedicine(Why));
	S.BeginExpedition(FGuid::NewGuid(),26,.12f); TestFalse(TEXT("Full vitality rejects before consuming"),S.TryUseMedicine(Why));
	for (int32 I=0; I<7; ++I) S.ReceiveSentinelStrike(0);
	TestFalse(TEXT("Death rejects"),S.TryUseMedicine(Why));
	FFixture Empty; TestTrue(TEXT("Carry used up with stash still nonzero"),Empty.Start(0,true));
	FShanmenDemo20WorldCheckpoint Intent; const auto Before=Empty.Snapshot();
	TestFalse(TEXT("Stash cannot be healed from"),FShanmenDemo20Medicine::BuildIntent(Empty.C,Before,Intent,Why));
	TestTrue(TEXT("Reject leaves exact ledger"),Before==Empty.Snapshot());
	TestTrue(TEXT("Ordinary priority intent"),F.Intent()); TestEqual(TEXT("Carry preferred to secure"),F.C.Medicine.Origin,EShanmenDemo20MedicineOrigin::PreparedCarry);
	const auto Id=FShanmenDemo20Medicine::IntentId(F.C); TestTrue(TEXT("Stable identity"),Id.IsValid() && Id==FShanmenDemo20Medicine::IntentId(F.C));
	TestFalse(TEXT("No second pending selection"),FShanmenDemo20Medicine::BuildIntent(F.C,F.Snapshot(),Intent,Why)); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemo20MedicineOrdinaryTest,"Shanmen.Demo20.Expedition.MedicineOrdinaryNativeExactlyOnce",Flags)
bool FDemo20MedicineOrdinaryTest::RunTest(const FString&)
{
	FFixture F; if (!TestTrue(TEXT("Start"),F.Start())) return false;
	const float HP=F.C.Combat.Health[0]; const auto Revision=F.C.Combat.Revisions[0]; const auto Sequence=F.C.Combat.Sequence;
	if (!TestTrue(TEXT("Persist exact intent"),F.Intent())) return false;
	TestTrue(TEXT("Confirmed heal"),FShanmenDemo20Medicine::Recover(F.Root,F.C,F.Ports(),F.Why));
	TestTrue(TEXT("35 actual health"),FMath::IsNearlyEqual(F.C.Combat.Health[0],HP+35.f));
	TestEqual(TEXT("One vitality revision"),F.C.Combat.Revisions[0],Revision+1); TestEqual(TEXT("One sequence"),F.C.Combat.Sequence,Sequence+1);
	TestEqual(TEXT("Seven ordinary left"),F.Quantity(EShanmenDemo20MedicineOrigin::PreparedCarry),7);
	const auto After=F.Snapshot(); const auto Generation=F.C.Generation;
	TestTrue(TEXT("Repeated recover without intent"),FShanmenDemo20Medicine::Recover(F.Root,F.C,F.Ports(),F.Why));
	TestTrue(TEXT("No item reexecution"),After==F.Snapshot()); TestEqual(TEXT("No world rewrite"),F.C.Generation,Generation);
	FShanmenDemo20WorldCheckpoint I; TestFalse(TEXT("Immediate repeat cooldown"),FShanmenDemo20Medicine::BuildIntent(F.C,After,I,F.Why));
	TestTrue(TEXT("Native restart"),F.Restart()); TestEqual(TEXT("No refunded pill"),F.Quantity(EShanmenDemo20MedicineOrigin::PreparedCarry),7);
	TestTrue(TEXT("Confirmed HP persists"),FMath::IsNearlyEqual(F.C.Combat.Health[0],HP+35.f));
	FShanmenDemo20Session S; S.RestoreExpedition(F.C.Combat); S.Advance(.7f);
	I=F.C; S.CaptureExpedition(I.Combat); FShanmenDemo20WorldCheckpointStore::Save(F.Root,F.C,I,F.Why);
	TestTrue(TEXT("Second legitimate medicine"),F.Intent() && FShanmenDemo20Medicine::Recover(F.Root,F.C,F.Ports(),F.Why));
	TestEqual(TEXT("Clamped health, not overflow"),F.C.Combat.Health[0],100.f); TestEqual(TEXT("Exactly six remain"),F.Quantity(EShanmenDemo20MedicineOrigin::PreparedCarry),6);
	const auto Last=F.Snapshot(); TestFalse(TEXT("Full health rejects"),F.Intent()); TestTrue(TEXT("Full-health rejection leaves ledger"),Last==F.Snapshot()); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemo20MedicineSecureTest,"Shanmen.Demo20.Expedition.MedicineSecureDepletedRestartAndDeath",Flags)
bool FDemo20MedicineSecureTest::RunTest(const FString&)
{
	FFixture F; if (!TestTrue(TEXT("One secure pill and empty ordinary"),F.Start(1,true) && F.Intent())) return false;
	TestEqual(TEXT("Secure selection"),F.C.Medicine.Origin,EShanmenDemo20MedicineOrigin::Secure);
	const auto Id=F.C.Medicine.ItemId;
	{ TGuardValue<bool> Fault(FShanmenDemo20WorldCheckpointStore::bFailBeforeReplace,true);
		TestFalse(TEXT("Last secure commit then world failure cannot publish heal"),FShanmenDemo20Medicine::Recover(F.Root,F.C,F.Ports(),F.Why)); }
	const auto Consumed=F.Snapshot(); const auto* Tombstone=Consumed.Items.FindByPredicate([&](const auto& I){return I.ItemInstanceId==Id;});
	TestTrue(TEXT("Actual depleted last pill clears placement"),Tombstone && Tombstone->Quantity==0 && !Tombstone->ParentContainerId.IsValid());
	TestTrue(TEXT("Restart exact tombstone intent"),F.Restart());
	TestTrue(TEXT("Recover cannot select replacement"),FShanmenDemo20Medicine::Recover(F.Root,F.C,F.Ports(),F.Why));
	TestTrue(TEXT("No second quantity commit"),Consumed==F.Snapshot()); TestEqual(TEXT("Secure empty, stash inaccessible"),F.Quantity(EShanmenDemo20MedicineOrigin::Secure),0);
	auto S=F.Snapshot(); FShanmenDemo20ActiveLoadout A; FShanmenDemo20Loadout::InspectActive(S,A,F.Why);
	FShanmenItemRunFinalizeRequest D; D.Context=F.Context(S); D.ActiveRunId=A.RunId; D.TerminalReason=EShanmenItemRunTerminalReason::Death;
	TestTrue(TEXT("Death after medicine"),F.Items->FinalizePreparedRunDurable(D).IsCommandSuccess()); const auto Dead=F.Snapshot();
	for (const auto& I:S.Items) if (I.ParentContainerId==FShanmenDemo20Catalog::ContainerId(TEXT("Stash")) || I.ParentContainerId==FShanmenDemo20Catalog::ContainerId(TEXT("SecureBox")))
		TestTrue(TEXT("Exact nonzero stash and secure equipment retained"),Dead.Items.ContainsByPredicate([&](const auto& V){return V==I;}));
	TestTrue(TEXT("Repeated death"),F.Items->FinalizePreparedRunDurable(D).IsCommandSuccess()); TestTrue(TEXT("No double return"),Dead==F.Snapshot()); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemo20MedicineFailureTest,"Shanmen.Demo20.Expedition.MedicineFailureRecoveryMatrix",Flags)
bool FDemo20MedicineFailureTest::RunTest(const FString&)
{
	for (bool Secure:{false,true}) for (int32 Fault=0; Fault<4; ++Fault)
	{
		FFixture F; if (!TestTrue(TEXT("Native matrix start and intent"),F.Start(Secure?3:0,Secure) && F.Intent())) return false;
		const float HP=F.C.Combat.Health[0]; const auto P0=F.Snapshot(); auto Ports=F.Ports();
		if (Fault==0) { if (Secure) Ports.Reserve=[](const auto&){return FShanmenItemDurableCommandResult();}; else Ports.PrepareRun=[](const auto&){return FShanmenItemDurableCommandResult();}; }
		if (Fault==1) F.Items->SetInjectedFailureForTests(EShanmenItemStoreFailureStage::AtomicReplace);
		if (Fault==2) { if (Secure) Ports.Commit=[](const auto&){return FShanmenItemDurableCommandResult();}; else Ports.FinalizeRun=[](const auto&){return FShanmenItemDurableCommandResult();}; }
		if (Fault==3) FShanmenDemo20WorldCheckpointStore::bFailFirstReadAfterReplace=true;
		TestFalse(TEXT("Failure never announces heal success"),FShanmenDemo20Medicine::Recover(F.Root,F.C,Ports,F.Why));
		TestEqual(TEXT("Unconfirmed health not published"),F.C.Combat.Health[0],HP); TestTrue(TEXT("Exact pending intent held"),F.C.Medicine.IsSet());
		if (Fault<2) TestTrue(TEXT("No quantity or ledger changed before acceptance"),P0==F.Snapshot());
		if (Fault==2 && !Secure) { FShanmenDemo20ActiveLoadout A; TestFalse(TEXT("Strict balance cannot hide pending prepared consume"),FShanmenDemo20Loadout::InspectActive(F.Snapshot(),A,F.Why)); }
		TestTrue(TEXT("Native restart with actual ledger and world"),F.Restart());
		TestTrue(TEXT("Roll forward exact request"),FShanmenDemo20Medicine::Recover(F.Root,F.C,F.Ports(),F.Why));
		TestTrue(TEXT("35 once, not zero or seventy"),FMath::IsNearlyEqual(F.C.Combat.Health[0],HP+35.f));
		TestEqual(TEXT("Exactly one consumed"),F.Quantity(Secure?EShanmenDemo20MedicineOrigin::Secure:EShanmenDemo20MedicineOrigin::PreparedCarry),Secure?2:7);
		const auto Final=F.Snapshot(); TestTrue(TEXT("Second native reopen"),F.Restart()); TestTrue(TEXT("Same exact items"),Final==F.Snapshot());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemo20MedicineEncodingTest,"Shanmen.Demo20.Expedition.MedicineLegacy333ByteCompatibility",Flags)
bool FDemo20MedicineEncodingTest::RunTest(const FString&)
{
	FFixture F; if (!TestTrue(TEXT("Existing r1 checkpoint"),F.Start())) return false;
	TArray<uint8> Bytes; const auto P=FShanmenDemo20WorldCheckpointStore::Path(F.Root,F.C.Combat.RunId); FFileHelper::LoadFileToArray(Bytes,*P);
	TestEqual(TEXT("No-intent byte layout unchanged"),Bytes.Num(),333);
	TestTrue(TEXT("Legacy CAS accepts pending extension"),F.Intent()); FFileHelper::LoadFileToArray(Bytes,*P);
	TestEqual(TEXT("Only bounded optional intent extension"),Bytes.Num(),366);
	TestTrue(TEXT("Extension reopens"),F.Restart() && F.C.Medicine.IsSet());
	TestTrue(TEXT("After consumption encoding back to r1"),FShanmenDemo20Medicine::Recover(F.Root,F.C,F.Ports(),F.Why));
	FFileHelper::LoadFileToArray(Bytes,*P); TestEqual(TEXT("No permanent inventory payload"),Bytes.Num(),333);
	Bytes.Add(1); FFileHelper::SaveArrayToFile(Bytes,*P); FShanmenDemo20WorldCheckpoint C;
	TestFalse(TEXT("Unknown tail rejected, not guessed migration"),FShanmenDemo20WorldCheckpointStore::Load(F.Root,F.C.Combat.RunId,C,F.Why)); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDemo20MedicineWorldRetryTest,"Shanmen.Demo20.Expedition.MedicineWorldAcceptanceAndSameProcessRetry",Flags)
bool FDemo20MedicineWorldRetryTest::RunTest(const FString&)
{
	FFixture F; if (!TestTrue(TEXT("Formal fixture"),F.Start())) return false;
	const auto Items=F.Snapshot(); const auto Before=F.C; FShanmenDemo20WorldCheckpoint Intent;
	TestTrue(TEXT("Build is read only"),FShanmenDemo20Medicine::BuildIntent(F.C,Items,Intent,F.Why));
	{ TGuardValue<bool> Fault(FShanmenDemo20WorldCheckpointStore::bFailBeforeReplace,true);
		TestFalse(TEXT("World intent not accepted before replace"),FShanmenDemo20WorldCheckpointStore::Save(F.Root,F.C,Intent,F.Why)); }
	TestTrue(TEXT("No item writes during world admission failure"),Items==F.Snapshot());
	TestEqual(TEXT("Unchanged generation"),F.C.Generation,Before.Generation);
	FShanmenDemo20WorldCheckpointStore::bFailFirstReadAfterReplace=true;
	TestFalse(TEXT("Ambiguous admission not announced"),FShanmenDemo20WorldCheckpointStore::Save(F.Root,F.C,Intent,F.Why));
	TestTrue(TEXT("Adopt exact pending admission"),FShanmenDemo20WorldCheckpointStore::Save(F.Root,F.C,Intent,F.Why));
	TestEqual(TEXT("One admission generation"),F.C.Generation,Before.Generation+1);
	const auto PendingGeneration=F.C.Generation; FShanmenDemo20WorldCheckpointStore::bFailFirstReadAfterReplace=true;
	TestFalse(TEXT("Ambiguous healed checkpoint not announced"),FShanmenDemo20Medicine::Recover(F.Root,F.C,F.Ports(),F.Why));
	const auto Consumed=F.Snapshot();
	TestTrue(TEXT("Same-process replay rolls forward, not another medicine"),FShanmenDemo20Medicine::Recover(F.Root,F.C,F.Ports(),F.Why));
	TestEqual(TEXT("Only one heal generation"),F.C.Generation,PendingGeneration+1); TestTrue(TEXT("Exact item document not rewritten"),Consumed==F.Snapshot());
	TestTrue(TEXT("Actual nonzero health plus 35"),FMath::IsNearlyEqual(F.C.Combat.Health[0],Before.Combat.Health[0]+35.f)); return true;
}
#endif
