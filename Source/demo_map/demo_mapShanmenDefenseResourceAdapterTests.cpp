#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenDefenseResourceAdapter.h"

#include "Misc/AutomationTest.h"
#include "ShanmenCombatResolver.h"
#include "ShanmenCombatTags.h"
#include "demo_map0909BSectWarehouseService.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapItemSubsystem.h"
#include "demo_mapPlayerHealthComponent.h"
#include "demo_mapProfilePreparationPresenter.h"
#include "demo_mapProfileRepository.h"
#include "demo_mapProfilePreparationTypes.h"
#include "demo_mapProfileSessionSubsystem.h"
#include "demo_mapShanmenItemCutover.h"
#include "demo_mapShanmenRunLifecycleAdapter.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	const FGuid TestOwnerId(0xD3520001, 0, 0, 1);
	const FGuid TestScopeId(0xD3520002, 0, 0, 1);
	const FGuid TestActiveRunId(0xD3520003, 0, 0, 1);
	const FGuid TestSwordId(0xD3520004, 0, 0, 1);
	const FGuid TestMirrorId(0xD3520004, 0, 0, 2);
	const FGuid TestReservationId(0xD3520006, 0, 0, 1);
	const FGuid TestUntriggeredReservationId(0xD3520006, 0, 0, 2);

	Fdemo_mapShanmenRunCorrelation MakeCorrelation()
	{
		Fdemo_mapShanmenRunCorrelation Correlation;
		Correlation.CorrelationId = FGuid(0xD3520010, 0, 0, 1);
		Correlation.OwnerId = TestOwnerId;
		Correlation.ScopeId = TestScopeId;
		Correlation.ActiveRunId = TestActiveRunId;
		Correlation.PreparedRequestId = FGuid(0xD3520011, 0, 0, 1);
		Correlation.PreparedReceiptId = FGuid(0xD3520012, 0, 0, 1);
		Correlation.LifecycleRequestId = FGuid(0xD3520013, 0, 0, 1);
		Correlation.LifecycleReceiptId = FGuid(0xD3520014, 0, 0, 1);
		Correlation.PreparedAuthorityRevision = 4;
		Correlation.LifecycleAuthorityRevision = 5;
		Correlation.WeaponItemInstanceId = TestSwordId;
		Correlation.AccessoryItemInstanceId = TestMirrorId;
		Correlation.OrderedPreparedItemInstanceIds = {
			TestSwordId, TestMirrorId };
		Correlation.HotbarItemInstanceIds.SetNum(
			Fdemo_mapPersistentPreparationLayout::HotbarSlotCount);
		return Correlation;
	}

	FShanmenContentStamp MakeContent()
	{
		FShanmenContentStamp Content;
		Content.Version = TEXT("Shanmen.0.0.10.P5.3");
		Content.Digest = TEXT("P5.3.DefenseResourceCoordination.v1");
		return Content;
	}

	FShanmenImpactRequest MakeResourceImpactRequest()
	{
		FShanmenCombatActionCapture Capture;
		Capture.RunId = FGuid(0xD3520030, 0, 0, 1);
		Capture.OwnerId = TestOwnerId;
		Capture.SourceEntityId = FGuid(0xD3520031, 0, 0, 1);
		Capture.ActionDefinitionId = TEXT("Action.Test.DefenseIntent");
		Capture.Content = MakeContent();
		Capture.ActivationId = FShanmenCombatIdFactory::MakeActivationId(
			Capture.RunId,
			Capture.SourceEntityId,
			Capture.ActionDefinitionId,
			1);
		FShanmenCombatActionSnapshot Action;
		check(FShanmenCombatActionSnapshot::TryCapture(Capture, Action));

		FShanmenImpactRequest Request;
		Request.Action = Action;
		Request.Candidate.ActivationId = Action.GetActivationId();
		Request.Candidate.SourceEntityId = Action.GetSourceEntityId();
		Request.Candidate.TargetEntityId = FGuid(0xD3520032, 0, 0, 1);
		Request.Candidate.DetectorId = TEXT("Detector.Test.DefenseIntent");
		Request.Candidate.DetectorKind = EShanmenHitDetectorKind::TargetedRule;
		Request.Candidate.HitOrdinal = 0;
		Request.Damage.FormulaId = TEXT("Formula.Test.DefenseIntent");
		Request.Damage.RawDamage = 10.0f;
		Request.Damage.DamageTags.AddTag(
			FShanmenCombatNativeTags::DamagePhysical());
		Request.TargetVitality.CurrentVitality = 10.0f;
		Request.TargetVitality.MaximumVitality = 10.0f;
		Request.TargetVitality.AuthorityRevision = 4;
		Request.Defense.TargetTags.AddTag(
			FShanmenCombatNativeTags::TargetLiving());
		FShanmenDefenseLayer& Triggered =
			Request.Defense.Layers.AddDefaulted_GetRef();
		Triggered.LayerId = TestReservationId;
		Triggered.RuleId = TEXT("Defense.Test.Resource.PreventAll");
		Triggered.SourceInstanceId = TestSwordId;
		Triggered.Operation = EShanmenDefenseOperation::PreventAll;
		Triggered.Order = FShanmenDefenseOrder::Shield;
		Triggered.LayerTags.AddTag(FShanmenCombatNativeTags::DefenseShield());
		Triggered.bRequiresCommitOnTrigger = true;
		FShanmenDefenseLayer& Untriggered =
			Request.Defense.Layers.AddDefaulted_GetRef();
		Untriggered.LayerId = TestUntriggeredReservationId;
		Untriggered.RuleId = TEXT("Defense.Test.Resource.LethalGuard");
		Untriggered.SourceInstanceId = TestMirrorId;
		Untriggered.Operation = EShanmenDefenseOperation::PreventLethal;
		Untriggered.Order = FShanmenDefenseOrder::LethalInterception;
		Untriggered.Magnitude = 1.0f;
		Untriggered.LayerTags.AddTag(
			FShanmenCombatNativeTags::DefenseLethalIntercept());
		Untriggered.bRequiresCommitOnTrigger = true;
		Request.ImpactId = FShanmenCombatIdFactory::MakeImpactId(
			Action.GetRunId(),
			Request.Candidate.ActivationId,
			Request.Candidate.DetectorId,
			Request.Candidate.TargetEntityId,
			Request.Candidate.HitOrdinal);
		check(Request.IsValid());
		return Request;
	}

	FString NewP54Root(const TCHAR* Label)
	{
		return FPaths::Combine(
			FPaths::ProjectSavedDir(), TEXT("Automation"),
			TEXT("Dev.D.UE.0.0.10.P5.4.r0"), Label,
			FGuid::NewGuid().ToString(EGuidFormats::Digits));
	}

	const FShanmenItemInstance* FindAuthorityItem(
		const FShanmenItemAuthoritySnapshot& Snapshot,
		const FGuid& ItemInstanceId)
	{
		return Snapshot.Items.FindByPredicate(
			[&ItemInstanceId](const FShanmenItemInstance& Item)
			{
				return Item.ItemInstanceId == ItemInstanceId;
			});
	}

	const Fdemo_mapProfilePreparationStashRow* FindPreparationRow(
		const Fdemo_mapProfilePreparationSnapshot& Snapshot,
		const FGuid& ItemInstanceId)
	{
		return Snapshot.OrderedPermanentStashRows.FindByPredicate(
			[&ItemInstanceId](
				const Fdemo_mapProfilePreparationStashRow& Row)
			{
				return Row.ItemInstanceId == ItemInstanceId;
			});
	}

	struct FP54DefenseFixture
	{
		FString Root;
		Fdemo_mapProfileStorageContext Storage;
		Fdemo_mapPersistentProfile SeedProfile;
		FGuid SpiritGuardId;
		FGuid HeartMirrorId;
		UGameInstance* GameInstance = nullptr;
		Udemo_mapShanmenItemAuthoritySubsystem* Authority = nullptr;
		Udemo_mapProfileSessionSubsystem* Session = nullptr;
		Udemo_mapItemSubsystem* Runtime = nullptr;
		Fdemo_map0909BSectWarehouseService Warehouse;
		Fdemo_mapShanmenRunStartResult Started;

		bool StartGameInstance(FAutomationTestBase& Test)
		{
			if (!GEngine)
			{
				Test.AddError(TEXT("GEngine is unavailable for the P5.4 fixture."));
				return false;
			}
			GameInstance = NewObject<UGameInstance>(
				GEngine, NAME_None, RF_Transient);
			if (!GameInstance)
			{
				Test.AddError(TEXT("Could not allocate the P5.4 GameInstance."));
				return false;
			}
			GameInstance->AddToRoot();
			GameInstance->Init();
			Authority = GameInstance->GetSubsystem<
				Udemo_mapShanmenItemAuthoritySubsystem>();
			Session = GameInstance->GetSubsystem<
				Udemo_mapProfileSessionSubsystem>();
			Runtime = GameInstance->GetSubsystem<Udemo_mapItemSubsystem>();
			return Authority && Session && Runtime;
		}

		bool SeedAndStart(
			FAutomationTestBase& Test,
			const TCHAR* Label,
			bool bIncludeSpiritGuard = true,
			bool bIncludeHeartMirror = false)
		{
			Root = NewP54Root(Label);
			Storage = Fdemo_mapProfileStorageContext::ForRoot(Root);
			Fdemo_mapProfileRepository Repository;
			SeedProfile = Repository.CreateFreshProfile();
			if (bIncludeSpiritGuard)
			{
				Fdemo_mapPersistentItemRecord SpiritGuard;
				SpiritGuardId = SpiritGuard.ItemInstanceId = FGuid::NewGuid();
				SpiritGuard.ItemDefinitionId = Fdemo_mapItemIds::SpiritGuardRobe;
				SpiritGuard.StackCount = 1;
				SpiritGuard.PersistentDomain =
					Edemo_mapPersistentDomain::PermanentStash;
				SeedProfile.PermanentStash.Add(SpiritGuard);
				SeedProfile.PreparationLayout.ArmorItemInstanceId = SpiritGuardId;
			}
			if (bIncludeHeartMirror)
			{
				Fdemo_mapPersistentItemRecord HeartMirror;
				HeartMirrorId = HeartMirror.ItemInstanceId = FGuid::NewGuid();
				HeartMirror.ItemDefinitionId =
					Fdemo_mapItemIds::HeartProtectingMirror;
				HeartMirror.StackCount = 1;
				HeartMirror.PersistentDomain =
					Edemo_mapPersistentDomain::PermanentStash;
				SeedProfile.PermanentStash.Add(HeartMirror);
				SeedProfile.PreparationLayout.AccessoryItemInstanceId = HeartMirrorId;
			}
			const Fdemo_mapProfileSaveResult Saved =
				Repository.SaveProfile(SeedProfile, Storage);
			if (!Saved.IsSuccess() || !StartGameInstance(Test))
			{
				Test.AddError(FString::Printf(
					TEXT("P5.4 seed failed: %s"), *Saved.Diagnostic));
				return false;
			}

			const Fdemo_mapProfileSessionInitializeResult Initialized =
				Session->InitializeSession(Storage);
			Fdemo_map0909BWarehousePresentation Presentation;
			FString Diagnostic;
			if (!Initialized.IsReady()
				|| !Warehouse.OpenForSect(
					Root, Initialized.Snapshot,
					Edemo_map0909BTopState::AtSect,
					Presentation, Diagnostic))
			{
				Test.AddError(FString::Printf(
					TEXT("P5.4 stable source open failed: %s"),
					*Diagnostic));
				return false;
			}
			const Fdemo_mapShanmenItemCutoverResult Cutover =
				Fdemo_mapShanmenItemCutoverCoordinator::Execute(
					Storage, SeedProfile.ProfileId, *Authority,
					*Session, Warehouse);
			if (!Cutover.IsReady())
			{
				Test.AddError(FString::Printf(
					TEXT("P5.4 cutover failed: %s"),
					*Cutover.Diagnostic));
				return false;
			}
			Runtime->ResetForAutomation();
			Started = Fdemo_mapShanmenRunLifecycleAdapter::StartPreparedRun(
				*Authority, *Runtime);
			if (!Started.IsStarted()
				|| (bIncludeSpiritGuard
					&& Started.RunCorrelation.ArmorItemInstanceId
						!= SpiritGuardId)
				|| (!bIncludeSpiritGuard
					&& Started.RunCorrelation.ArmorItemInstanceId.IsValid())
				|| (bIncludeHeartMirror
					&& Started.RunCorrelation.AccessoryItemInstanceId
						!= HeartMirrorId)
				|| (!bIncludeHeartMirror
					&& Started.RunCorrelation.AccessoryItemInstanceId.IsValid()))
			{
				Test.AddError(FString::Printf(
					TEXT("P5.4 active Run failed: %s"),
					*Started.Diagnostic));
				return false;
			}
			return true;
		}

		bool RestartAndBind(FAutomationTestBase& Test)
		{
			Stop();
			if (!StartGameInstance(Test)
				|| !Session->InitializeSession(Storage).IsReady())
			{
				return false;
			}
			const Fdemo_mapShanmenItemAuthorityBindResult Bound =
				Authority->BindExisting(Storage, SeedProfile.ProfileId);
			if (!Bound.IsReady())
			{
				Test.AddError(FString::Printf(
					TEXT("P5.4 restart bind failed: %s"),
					*Bound.Diagnostic));
				return false;
			}
			return true;
		}

		void Stop()
		{
			if (!GameInstance)
			{
				return;
			}
			GameInstance->Shutdown();
			Authority = nullptr;
			Session = nullptr;
			Runtime = nullptr;
			GameInstance->RemoveFromRoot();
			GameInstance->MarkAsGarbage();
			GameInstance = nullptr;
			CollectGarbage(RF_NoFlags);
		}

		~FP54DefenseFixture()
		{
			Stop();
			if (!Root.IsEmpty())
			{
				IFileManager::Get().DeleteDirectory(*Root, false, true);
			}
		}
	};

	FShanmenDefenseSnapshot MakeP54BaseDefense(bool bPreventAllFirst)
	{
		FShanmenDefenseSnapshot Defense;
		Defense.TargetTags.AddTag(FShanmenCombatNativeTags::TargetLiving());
		if (bPreventAllFirst)
		{
			FShanmenDefenseLayer& Avoidance =
				Defense.Layers.AddDefaulted_GetRef();
			Avoidance.LayerId = FGuid(0xD3540001, 0, 0, 1);
			Avoidance.RuleId = TEXT("Combat.Defense.Test.P5.4.Avoidance");
			Avoidance.Operation = EShanmenDefenseOperation::PreventAll;
			Avoidance.Order = FShanmenDefenseOrder::Avoidance;
			Avoidance.LayerTags.AddTag(
				FShanmenCombatNativeTags::DefenseEvade());
			Avoidance.RequiredTargetTags.AddTag(
				FShanmenCombatNativeTags::TargetLiving());
		}
		FShanmenDefenseLayer& Aggregate =
			Defense.Layers.AddDefaulted_GetRef();
		Aggregate.LayerId = FGuid(0xD3540002, 0, 0, 1);
		Aggregate.RuleId =
			TEXT("Combat.Defense.Player.FlatDamageReduction.r1");
		Aggregate.Operation = EShanmenDefenseOperation::AbsorbPoints;
		Aggregate.Order = FShanmenDefenseOrder::Resistance;
		Aggregate.Magnitude = 2.0f;
		Aggregate.LayerTags.AddTag(
			FShanmenCombatNativeTags::DefenseArmor());
		Aggregate.RequiredTargetTags.AddTag(
			FShanmenCombatNativeTags::TargetLiving());
		check(Defense.IsValid());
		return Defense;
	}

	FShanmenDefenseSnapshot MakeHeartMirrorBaseDefense()
	{
		FShanmenDefenseSnapshot Defense;
		Defense.TargetTags.AddTag(FShanmenCombatNativeTags::TargetLiving());
		check(Defense.IsValid());
		return Defense;
	}

	bool BuildP54Impact(
		const FP54DefenseFixture& Fixture,
		Udemo_mapPlayerHealthComponent& Health,
		uint64 Sequence,
		FShanmenDefenseSnapshot Defense,
		FShanmenImpactRequest& OutRequest,
		FShanmenImpactResult& OutResult,
		FString& OutDiagnostic,
		float RawDamage = 4.0f)
	{
		OutRequest = FShanmenImpactRequest();
		OutResult = FShanmenImpactResult();
		FShanmenCombatActionCapture Capture;
		Capture.RunId = Fixture.Started.ActiveRunId;
		Capture.OwnerId = Fixture.SeedProfile.ProfileId;
		Capture.SourceEntityId = FGuid(0xD3540010, 0, 0, 1);
		Capture.ActionDefinitionId = TEXT("Action.Test.P5.4.EnemyStrike");
		Capture.Content = Udemo_mapShanmenItemAuthoritySubsystem::
			ProductContentStamp();
		Capture.ActivationId = FShanmenCombatIdFactory::MakeActivationId(
			Capture.RunId, Capture.SourceEntityId,
			Capture.ActionDefinitionId, Sequence);
		if (!FShanmenCombatActionSnapshot::TryCapture(
				Capture, OutRequest.Action))
		{
			OutDiagnostic = TEXT("P5.4 action capture failed.");
			return false;
		}
		OutRequest.Candidate.ActivationId =
			OutRequest.Action.GetActivationId();
		OutRequest.Candidate.SourceEntityId =
			OutRequest.Action.GetSourceEntityId();
		OutRequest.Candidate.TargetEntityId = Health.GetCombatEntityId();
		OutRequest.Candidate.DetectorId = TEXT("Detector.Test.P5.4.Contact");
		OutRequest.Candidate.DetectorKind =
			EShanmenHitDetectorKind::TargetedRule;
		OutRequest.Candidate.HitOrdinal = 0;
		OutRequest.ImpactId = FShanmenCombatIdFactory::MakeImpactId(
			OutRequest.Action.GetRunId(),
			OutRequest.Candidate.ActivationId,
			OutRequest.Candidate.DetectorId,
			OutRequest.Candidate.TargetEntityId,
			OutRequest.Candidate.HitOrdinal);
		const Fdemo_mapShanmenDefenseResourcePreparationResult Prepared =
			Fdemo_mapShanmenDefenseResourceAdapter::PrepareImpactDefense(
				*Fixture.Authority, Health, OutRequest.ImpactId, Defense);
		if (!Prepared.IsSuccess() || !Prepared.HasResourceLayer()
			|| !Health.TryCaptureCombatVitalitySnapshot(
				OutRequest.TargetVitality))
		{
			OutDiagnostic = Prepared.Diagnostic.IsEmpty()
				? TEXT("P5.4 defense or vitality capture failed.")
				: Prepared.Diagnostic;
			return false;
		}
		OutRequest.Candidate.HitLocation = FVector::ZeroVector;
		OutRequest.Candidate.HitNormal = FVector::UpVector;
		OutRequest.Damage.FormulaId = TEXT("Formula.Test.P17.FixedDamage");
		OutRequest.Damage.RawDamage = RawDamage;
		OutRequest.Damage.DamageTags.AddTag(
			FShanmenCombatNativeTags::DamagePhysical());
		OutRequest.Defense = MoveTemp(Defense);
		OutResult = FShanmenDefenseResolver::Resolve(OutRequest);
		if (!OutRequest.IsValid() || !OutResult.bAccepted
			|| !OutResult.IsConserved())
		{
			OutDiagnostic = TEXT("P5.4 canonical resolve failed.");
			return false;
		}
		OutDiagnostic.Reset();
		return true;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapShanmenDefenseResourceIntentRequestTest,
	"Shanmen.0_0_10.Items.DefenseResourceAdapter.RecoverableIntent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapShanmenDefenseResourceIntentRequestTest::RunTest(
	const FString&)
{
	const Fdemo_mapShanmenRunCorrelation Correlation = MakeCorrelation();
	const FShanmenContentStamp Content = MakeContent();
	const FShanmenImpactRequest Request = MakeResourceImpactRequest();
	const FShanmenImpactResult Impact =
		FShanmenDefenseResolver::Resolve(Request);
	TestTrue(TEXT("Fixture triggers only the first of two resource layers"),
		Impact.bAccepted && Impact.IsConserved()
			&& Impact.TriggeredLayers.Num() == 1
			&& Impact.TriggeredLayers[0].LayerId == TestReservationId
			&& Impact.TriggeredLayers[0].bRequiresCommit);

	FShanmenItemRunResourceIntentRequest First;
	FShanmenItemRunResourceIntentRequest Replay;
	FShanmenVitalityCommitCommand FirstVitality;
	FShanmenVitalityCommitCommand ReplayVitality;
	FString Diagnostic;
	FString ReplayDiagnostic;
	TestTrue(TEXT("Intent orders triggered prefix before untriggered cancellation"),
		Fdemo_mapShanmenDefenseResourceAdapter::BuildIntentRequest(
			Request,
			Impact,
			Correlation,
			Content,
			First,
			FirstVitality,
			Diagnostic)
			&& First.IsValid()
			&& First.IntentId == Request.ImpactId
			&& First.TriggeredLineCount == 1
			&& First.OrderedLines.Num() == 2
			&& First.OrderedLines[0].ReservationId == TestReservationId
			&& First.OrderedLines[0].ItemInstanceId == TestSwordId
			&& First.OrderedLines[1].ReservationId
				== TestUntriggeredReservationId
			&& First.OrderedLines[1].ItemInstanceId == TestMirrorId);
	TestTrue(TEXT("Exact resolve derives the same prepare identity and metadata"),
		Fdemo_mapShanmenDefenseResourceAdapter::BuildIntentRequest(
			Request,
			Impact,
			Correlation,
			Content,
			Replay,
			ReplayVitality,
			ReplayDiagnostic)
			&& Replay.Context.RequestId == First.Context.RequestId
			&& Replay.IntentMetadata == First.IntentMetadata
			&& Replay.OrderedLines == First.OrderedLines
			&& ReplayVitality.GetResolutionId()
				== FirstVitality.GetResolutionId());

	FShanmenVitalityCommitCommand Decoded;
	TestTrue(TEXT("Opaque durable metadata restores the exact vitality CAS command"),
		Fdemo_mapShanmenDefenseResourceAdapter::DecodeVitalityIntent(
			First.IntentMetadata, Decoded)
			&& Decoded.GetImpactId() == FirstVitality.GetImpactId()
			&& Decoded.GetResolutionId() == FirstVitality.GetResolutionId()
			&& Decoded.GetTargetEntityId()
				== FirstVitality.GetTargetEntityId()
			&& Decoded.GetExpectedAuthorityRevision()
				== FirstVitality.GetExpectedAuthorityRevision()
			&& FMath::IsNearlyEqual(
				Decoded.GetExpectedCurrentVitality(),
				FirstVitality.GetExpectedCurrentVitality())
			&& FMath::IsNearlyEqual(
				Decoded.GetExpectedVitalityAfter(),
				FirstVitality.GetExpectedVitalityAfter()));

	FShanmenItemRunResourceIntentFinalizeRequest Success;
	FShanmenItemRunResourceIntentFinalizeRequest Failure;
	TestTrue(TEXT("Terminal decisions have deterministic, distinct request identities"),
		Fdemo_mapShanmenDefenseResourceAdapter::BuildFinalizeRequest(
			First, true, Success)
			&& Fdemo_mapShanmenDefenseResourceAdapter::BuildFinalizeRequest(
				First, false, Failure)
			&& Success.IsValid() && Failure.IsValid()
			&& Success.Context.RequestId != Failure.Context.RequestId
			&& Success.PrepareRequestId == First.Context.RequestId
			&& Success.IntentId == First.IntentId
			&& Success.bExternalCommitSucceeded
			&& !Failure.bExternalCommitSucceeded);

	FShanmenImpactResult Mismatched = Impact;
	Mismatched.TriggeredLayers[0].SourceInstanceId = TestMirrorId;
	FShanmenItemRunResourceIntentRequest Invalid;
	FShanmenVitalityCommitCommand InvalidVitality;
	TestFalse(TEXT("Result/input source mismatch fails before either authority"),
		Fdemo_mapShanmenDefenseResourceAdapter::BuildIntentRequest(
			Request,
			Mismatched,
			Correlation,
			Content,
			Invalid,
			InvalidVitality,
			Diagnostic));
	TestFalse(TEXT("Malformed metadata cannot fabricate a recovery command"),
		Fdemo_mapShanmenDefenseResourceAdapter::DecodeVitalityIntent(
			TEXT("SMV1_CORRUPT"), InvalidVitality));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSpiritGuardTriggeredDurabilityTest,
	"Shanmen.0_0_10.Items.DefenseResourceAdapter.SpiritGuardTriggeredCommit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapSpiritGuardTriggeredDurabilityTest::RunTest(const FString&)
{
	FP54DefenseFixture Fixture;
	if (!Fixture.SeedAndStart(*this, TEXT("TriggeredCommit")))
	{
		return false;
	}
	Udemo_mapPlayerHealthComponent* Health =
		NewObject<Udemo_mapPlayerHealthComponent>(GetTransientPackage());
	const FGuid TargetId(0xD3540100, 0, 0, 1);
	if (!Health || !Health->TryBindCombatEntity(TargetId))
	{
		AddError(TEXT("P5.4 trigger fixture could not bind vitality."));
		return false;
	}
	FShanmenTargetVitalitySnapshot BeforeVitality;
	Health->TryCaptureCombatVitalitySnapshot(BeforeVitality);
	FShanmenImpactRequest Request;
	FShanmenImpactResult Impact;
	FString Diagnostic;
	if (!BuildP54Impact(
		Fixture, *Health, 1, MakeP54BaseDefense(false),
		Request, Impact, Diagnostic))
	{
		AddError(Diagnostic);
		return false;
	}
	const FShanmenDefenseLayer* ResourceLayer =
		Request.Defense.Layers.FindByPredicate(
			[](const FShanmenDefenseLayer& Layer)
			{
				return Layer.bRequiresCommitOnTrigger;
			});
	TestTrue(TEXT("Spirit Guard replaces its aggregate contribution"),
		Request.Defense.Layers.Num() == 1
		&& ResourceLayer
		&& ResourceLayer->SourceInstanceId == Fixture.SpiritGuardId
		&& FMath::IsNearlyEqual(ResourceLayer->Magnitude, 2.0f));
	TestTrue(TEXT("Triggered robe prevents two of four damage"),
		Impact.TriggeredLayers.Num() == 1
		&& ResourceLayer
		&& Impact.TriggeredLayers[0].LayerId == ResourceLayer->LayerId
		&& Impact.TriggeredLayers[0].bRequiresCommit
		&& FMath::IsNearlyEqual(Impact.PreventedDamage, 2.0f)
		&& FMath::IsNearlyEqual(Impact.FinalDamage, 2.0f));

	const Fdemo_mapShanmenDefenseResourceCoordinationResult Coordinated =
		Fdemo_mapShanmenDefenseResourceAdapter::CoordinateImpact(
			*Fixture.Authority, *Health, Request, Impact);
	FShanmenItemAuthoritySnapshot After;
	const bool bCaptured = Fixture.Authority->TryCaptureSnapshot(After);
	const FShanmenItemInstance* Armor = bCaptured
		? FindAuthorityItem(After, Fixture.SpiritGuardId) : nullptr;
	TestTrue(TEXT("Triggered mitigation atomically commits vitality and durability"),
		Coordinated.IsSuccess()
		&& Coordinated.Status
			== Edemo_mapShanmenDefenseResourceCoordinationStatus::Coordinated
		&& FMath::IsNearlyEqual(
			Health->GetCurrentVitality(),
			BeforeVitality.CurrentVitality - 2.0f)
		&& Armor && Armor->Durability == 19);

	const Fdemo_mapShanmenDefenseResourceCoordinationResult Replayed =
		Fdemo_mapShanmenDefenseResourceAdapter::CoordinateImpact(
			*Fixture.Authority, *Health, Request, Impact);
	FShanmenItemAuthoritySnapshot ReplaySnapshot;
	Fixture.Authority->TryCaptureSnapshot(ReplaySnapshot);
	const FShanmenItemInstance* ReplayedArmor =
		FindAuthorityItem(ReplaySnapshot, Fixture.SpiritGuardId);
	TestTrue(TEXT("Exact replay cannot spend a second durability point"),
		Replayed.IsSuccess()
		&& Replayed.PrepareCommand.Status
			== EShanmenItemDurableCommandStatus::Replayed
		&& Replayed.FinalizeCommand.Status
			== EShanmenItemDurableCommandStatus::Replayed
		&& FMath::IsNearlyEqual(
			Health->GetCurrentVitality(),
			BeforeVitality.CurrentVitality - 2.0f)
		&& ReplayedArmor && ReplayedArmor->Durability == 19);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSpiritGuardDurableLifecycleTest,
	"Shanmen.0_0_10.Items.DefenseResourceAdapter.SpiritGuardDurableLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapSpiritGuardDurableLifecycleTest::RunTest(const FString&)
{
	FP54DefenseFixture Fixture;
	if (!Fixture.SeedAndStart(*this, TEXT("DurableLifecycle")))
	{
		return false;
	}
	const FGuid FirstRunId = Fixture.Started.ActiveRunId;
	Udemo_mapPlayerHealthComponent* FirstHealth =
		NewObject<Udemo_mapPlayerHealthComponent>(GetTransientPackage());
	if (!FirstHealth
		|| !FirstHealth->TryBindCombatEntity(
			FGuid(0xD3550100, 0, 0, 1)))
	{
		AddError(TEXT("P5.5 first Run could not bind vitality."));
		return false;
	}
	FShanmenImpactRequest FirstRequest;
	FShanmenImpactResult FirstImpact;
	FString Diagnostic;
	if (!BuildP54Impact(
		Fixture, *FirstHealth, 1, MakeP54BaseDefense(false),
		FirstRequest, FirstImpact, Diagnostic))
	{
		AddError(Diagnostic);
		return false;
	}
	const Fdemo_mapShanmenDefenseResourceCoordinationResult FirstWear =
		Fdemo_mapShanmenDefenseResourceAdapter::CoordinateImpact(
			*Fixture.Authority, *FirstHealth,
			FirstRequest, FirstImpact);
	FShanmenItemAuthoritySnapshot WornInFirstRun;
	const FShanmenItemInstance* FirstWornArmor =
		Fixture.Authority->TryCaptureSnapshot(WornInFirstRun)
		? FindAuthorityItem(WornInFirstRun, Fixture.SpiritGuardId)
		: nullptr;
	TestTrue(TEXT("First Run durably wears the robe from twenty to nineteen"),
		FirstWear.IsSuccess()
		&& FirstWornArmor && FirstWornArmor->Durability == 19);

	Fdemo_mapSettlementSummary ExtractionSummary;
	const Fdemo_mapItemOperationResult RuntimeExtraction =
		Fixture.Runtime->RequestSettlement(
			Edemo_mapRunEndReason::Extraction, ExtractionSummary);
	const Fdemo_mapShanmenRunFinalizeResult Extraction =
		RuntimeExtraction.bSuccess
		? Fdemo_mapShanmenRunLifecycleAdapter::FinalizeSettlement(
			*Fixture.Authority, ExtractionSummary)
		: Fdemo_mapShanmenRunFinalizeResult();
	FShanmenItemAuthoritySnapshot Extracted;
	const FShanmenItemInstance* ExtractedArmor =
		Fixture.Authority->TryCaptureSnapshot(Extracted)
		? FindAuthorityItem(Extracted, Fixture.SpiritGuardId)
		: nullptr;
	const Fdemo_mapProfilePreparationSnapshot ExtractedPreparation =
		Fixture.Session->GetPreparationSnapshot();
	const Fdemo_mapProfilePreparationStashRow* ExtractedRow =
		FindPreparationRow(
			ExtractedPreparation, Fixture.SpiritGuardId);
	const Fdemo_mapProfilePreparationViewState ExtractedView =
		Fdemo_mapProfilePreparationPresenter::BuildViewState(
			ExtractedPreparation);
	const Fdemo_mapProfilePreparationRowView* ExtractedViewRow =
		ExtractedView.OrderedPermanentStashRows.FindByPredicate(
			[&Fixture](const Fdemo_mapProfilePreparationRowView& Row)
			{
				return Row.ItemInstanceId == Fixture.SpiritGuardId;
			});
	TestTrue(TEXT("Extraction preserves wear and exposes it in preparation"),
		Extraction.IsFinalized()
		&& ExtractedArmor
		&& ExtractedArmor->State == EShanmenItemInstanceState::Stored
		&& ExtractedArmor->Durability == 19
		&& ExtractedRow
		&& ExtractedRow->Durability == 19
		&& ExtractedRow->MaxDurability == 20
		&& ExtractedRow->HasValidResourceState()
		&& ExtractedViewRow
		&& ExtractedViewRow->ResourceLabel == TEXT("DUR 19/20"));

	if (!Fixture.RestartAndBind(*this))
	{
		return false;
	}
	FShanmenItemAuthoritySnapshot RestartedAfterExtraction;
	const FShanmenItemInstance* RestartedArmor =
		Fixture.Authority->TryCaptureSnapshot(RestartedAfterExtraction)
		? FindAuthorityItem(
			RestartedAfterExtraction, Fixture.SpiritGuardId)
		: nullptr;
	const Fdemo_mapProfilePreparationSnapshot RestartedPreparation =
		Fixture.Session->GetPreparationSnapshot();
	const Fdemo_mapProfilePreparationStashRow* RestartedRow =
		FindPreparationRow(
			RestartedPreparation, Fixture.SpiritGuardId);
	TestTrue(TEXT("Process restart preserves nineteen durability"),
		RestartedArmor && RestartedArmor->Durability == 19
		&& RestartedRow
		&& RestartedRow->Durability == 19
		&& RestartedRow->MaxDurability == 20);

	const Fdemo_mapProfilePreparationSelectionResult Reselected =
		Fixture.Session->SetPreparationEquipment(
			Fdemo_mapItemIds::ArmorSlot, Fixture.SpiritGuardId);
	Fixture.Runtime->ResetForAutomation();
	Fixture.Started =
		Fdemo_mapShanmenRunLifecycleAdapter::StartPreparedRun(
			*Fixture.Authority, *Fixture.Runtime);
	TestTrue(TEXT("A later Run reuses the same worn identity"),
		Reselected.IsAccepted()
		&& Fixture.Started.IsStarted()
		&& Fixture.Started.ActiveRunId != FirstRunId
		&& Fixture.Started.RunCorrelation.ArmorItemInstanceId
			== Fixture.SpiritGuardId);

	Udemo_mapPlayerHealthComponent* SecondHealth =
		NewObject<Udemo_mapPlayerHealthComponent>(GetTransientPackage());
	if (!SecondHealth
		|| !SecondHealth->TryBindCombatEntity(
			FGuid(0xD3550200, 0, 0, 1)))
	{
		AddError(TEXT("P5.5 second Run could not bind vitality."));
		return false;
	}
	FShanmenImpactRequest SecondRequest;
	FShanmenImpactResult SecondImpact;
	if (!BuildP54Impact(
		Fixture, *SecondHealth, 2, MakeP54BaseDefense(false),
		SecondRequest, SecondImpact, Diagnostic))
	{
		AddError(Diagnostic);
		return false;
	}
	const Fdemo_mapShanmenDefenseResourceCoordinationResult SecondWear =
		Fdemo_mapShanmenDefenseResourceAdapter::CoordinateImpact(
			*Fixture.Authority, *SecondHealth,
			SecondRequest, SecondImpact);
	FShanmenItemAuthoritySnapshot WornInSecondRun;
	const FShanmenItemInstance* SecondWornArmor =
		Fixture.Authority->TryCaptureSnapshot(WornInSecondRun)
		? FindAuthorityItem(WornInSecondRun, Fixture.SpiritGuardId)
		: nullptr;
	TestTrue(TEXT("Later Run continues wear from nineteen to eighteen"),
		SecondWear.IsSuccess()
		&& SecondWornArmor && SecondWornArmor->Durability == 18);

	Fdemo_mapSettlementSummary DeathSummary;
	const Fdemo_mapItemOperationResult RuntimeDeath =
		Fixture.Runtime->RequestSettlement(
			Edemo_mapRunEndReason::Death, DeathSummary);
	const Fdemo_mapShanmenRunFinalizeResult Death = RuntimeDeath.bSuccess
		? Fdemo_mapShanmenRunLifecycleAdapter::FinalizeSettlement(
			*Fixture.Authority, DeathSummary)
		: Fdemo_mapShanmenRunFinalizeResult();
	FShanmenItemAuthoritySnapshot Destroyed;
	const FShanmenItemInstance* DestroyedArmor =
		Fixture.Authority->TryCaptureSnapshot(Destroyed)
		? FindAuthorityItem(Destroyed, Fixture.SpiritGuardId)
		: nullptr;
	const Fdemo_mapProfilePreparationSnapshot DestroyedPreparation =
		Fixture.Session->GetPreparationSnapshot();
	TestTrue(TEXT("Death converts the worn identity to a zeroed tombstone"),
		Death.IsFinalized()
		&& DestroyedArmor
		&& DestroyedArmor->State == EShanmenItemInstanceState::Destroyed
		&& DestroyedArmor->Quantity == 0
		&& DestroyedArmor->Durability == 0
		&& DestroyedArmor->Charges == 0
		&& !FindPreparationRow(
			DestroyedPreparation, Fixture.SpiritGuardId));

	if (!Fixture.RestartAndBind(*this))
	{
		return false;
	}
	FShanmenItemAuthoritySnapshot RestartedAfterDeath;
	const FShanmenItemInstance* RestartedTombstone =
		Fixture.Authority->TryCaptureSnapshot(RestartedAfterDeath)
		? FindAuthorityItem(
			RestartedAfterDeath, Fixture.SpiritGuardId)
		: nullptr;
	TestTrue(TEXT("Destroyed resource state remains terminal after restart"),
		RestartedTombstone
		&& RestartedTombstone->State
			== EShanmenItemInstanceState::Destroyed
		&& RestartedTombstone->Quantity == 0
		&& RestartedTombstone->Durability == 0
		&& !FindPreparationRow(
			Fixture.Session->GetPreparationSnapshot(),
			Fixture.SpiritGuardId));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSpiritGuardUntriggeredCancellationTest,
	"Shanmen.0_0_10.Items.DefenseResourceAdapter.SpiritGuardUntriggeredCancel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapSpiritGuardUntriggeredCancellationTest::RunTest(
	const FString&)
{
	FP54DefenseFixture Fixture;
	if (!Fixture.SeedAndStart(*this, TEXT("UntriggeredCancel")))
	{
		return false;
	}
	Udemo_mapPlayerHealthComponent* Health =
		NewObject<Udemo_mapPlayerHealthComponent>(GetTransientPackage());
	if (!Health
		|| !Health->TryBindCombatEntity(FGuid(0xD3540200, 0, 0, 1)))
	{
		AddError(TEXT("P5.4 untriggered fixture could not bind vitality."));
		return false;
	}
	const float VitalityBefore = Health->GetCurrentVitality();
	FShanmenImpactRequest Request;
	FShanmenImpactResult Impact;
	FString Diagnostic;
	if (!BuildP54Impact(
		Fixture, *Health, 2, MakeP54BaseDefense(true),
		Request, Impact, Diagnostic))
	{
		AddError(Diagnostic);
		return false;
	}
	const FShanmenDefenseLayer* ResourceLayer =
		Request.Defense.Layers.FindByPredicate(
			[](const FShanmenDefenseLayer& Layer)
			{
				return Layer.bRequiresCommitOnTrigger;
			});
	TestTrue(TEXT("Earlier avoidance leaves robe untriggered"),
		ResourceLayer && Impact.TriggeredLayers.Num() == 1
		&& Impact.TriggeredLayers[0].LayerId
			!= ResourceLayer->LayerId
		&& FMath::IsNearlyZero(Impact.FinalDamage));
	const Fdemo_mapShanmenDefenseResourceCoordinationResult Coordinated =
		Fdemo_mapShanmenDefenseResourceAdapter::CoordinateImpact(
			*Fixture.Authority, *Health, Request, Impact);
	FShanmenItemAuthoritySnapshot After;
	Fixture.Authority->TryCaptureSnapshot(After);
	const FShanmenItemInstance* Armor =
		FindAuthorityItem(After, Fixture.SpiritGuardId);
	const FShanmenItemReservationSnapshot* Reservation = ResourceLayer
		? After.Reservations.FindByPredicate(
			[ResourceLayer](const FShanmenItemReservationSnapshot& Candidate)
			{
				return Candidate.ReservationId == ResourceLayer->LayerId;
			})
		: nullptr;
	TestTrue(TEXT("Untriggered robe is cancelled without durability wear"),
		Coordinated.IsSuccess()
		&& FMath::IsNearlyEqual(Health->GetCurrentVitality(), VitalityBefore)
		&& Armor && Armor->Durability == 20
		&& Reservation
		&& Reservation->State == EShanmenItemReservationState::Cancelled);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSpiritGuardPreIntentRestartRecoveryTest,
	"Shanmen.0_0_10.Items.DefenseResourceAdapter.SpiritGuardPreIntentRestart",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapSpiritGuardPreIntentRestartRecoveryTest::RunTest(
	const FString&)
{
	FP54DefenseFixture Fixture;
	if (!Fixture.SeedAndStart(*this, TEXT("PreIntentRestart")))
	{
		return false;
	}
	Udemo_mapPlayerHealthComponent* Health =
		NewObject<Udemo_mapPlayerHealthComponent>(GetTransientPackage());
	if (!Health
		|| !Health->TryBindCombatEntity(FGuid(0xD3540300, 0, 0, 1)))
	{
		AddError(TEXT("P5.4 restart fixture could not bind vitality."));
		return false;
	}
	FShanmenImpactRequest Request;
	FShanmenImpactResult Impact;
	FString Diagnostic;
	if (!BuildP54Impact(
		Fixture, *Health, 3, MakeP54BaseDefense(false),
		Request, Impact, Diagnostic))
	{
		AddError(Diagnostic);
		return false;
	}
	const FShanmenDefenseLayer* ResourceLayer =
		Request.Defense.Layers.FindByPredicate(
			[](const FShanmenDefenseLayer& Layer)
			{
				return Layer.bRequiresCommitOnTrigger;
			});
	FShanmenItemAuthoritySnapshot BeforeRestart;
	Fixture.Authority->TryCaptureSnapshot(BeforeRestart);
	const FShanmenItemReservationSnapshot* Pending = ResourceLayer
		? BeforeRestart.Reservations.FindByPredicate(
			[ResourceLayer](const FShanmenItemReservationSnapshot& Candidate)
			{
				return Candidate.ReservationId == ResourceLayer->LayerId;
			})
		: nullptr;
	TestTrue(TEXT("Interruption point owns one temporary durability reservation"),
		Pending && Pending->State == EShanmenItemReservationState::Reserved
		&& Pending->PurposeId.ToString().StartsWith(TEXT("SMDR1_")));
	if (!Fixture.RestartAndBind(*this))
	{
		return false;
	}
	const Fdemo_mapShanmenDefenseOrphanRecoveryResult Recovered =
		Fdemo_mapShanmenDefenseResourceAdapter::
			RecoverOrphanedDefenseReservations(*Fixture.Authority);
	const Fdemo_mapShanmenDefenseOrphanRecoveryResult Replay =
		Fdemo_mapShanmenDefenseResourceAdapter::
			RecoverOrphanedDefenseReservations(*Fixture.Authority);
	FShanmenItemAuthoritySnapshot AfterRestart;
	Fixture.Authority->TryCaptureSnapshot(AfterRestart);
	const FShanmenItemInstance* Armor =
		FindAuthorityItem(AfterRestart, Fixture.SpiritGuardId);
	const FShanmenItemReservationSnapshot* Cancelled = ResourceLayer
		? AfterRestart.Reservations.FindByPredicate(
			[ResourceLayer](const FShanmenItemReservationSnapshot& Candidate)
			{
				return Candidate.ReservationId == ResourceLayer->LayerId;
			})
		: nullptr;
	TestTrue(TEXT("Restart cancels the orphan exactly once without wear"),
		Recovered.bSuccess && Recovered.CancelledReservationCount == 1
		&& Replay.bSuccess && Replay.CancelledReservationCount == 0
		&& Armor && Armor->Durability == 20
		&& Cancelled
		&& Cancelled->State == EShanmenItemReservationState::Cancelled);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapHeartMirrorTriggeredChargeTest,
	"Shanmen.0_0_10.Items.DefenseResourceAdapter.HeartMirrorTriggeredCommit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapHeartMirrorTriggeredChargeTest::RunTest(const FString&)
{
	FP54DefenseFixture Fixture;
	if (!Fixture.SeedAndStart(
			*this, TEXT("HeartMirrorTriggered"), false, true))
	{
		return false;
	}
	Udemo_mapPlayerHealthComponent* Health =
		NewObject<Udemo_mapPlayerHealthComponent>(GetTransientPackage());
	if (!Health
		|| !Health->TryBindCombatEntity(FGuid(0xD3570100, 0, 0, 1)))
	{
		AddError(TEXT("P17.0 mirror fixture could not bind vitality."));
		return false;
	}
	Health->SetCurrentHealthForAutomation(3);
	FShanmenImpactRequest Request;
	FShanmenImpactResult Impact;
	FString Diagnostic;
	if (!BuildP54Impact(
			Fixture, *Health, 10, MakeHeartMirrorBaseDefense(),
			Request, Impact, Diagnostic, 5.0f))
	{
		AddError(Diagnostic);
		return false;
	}
	const FShanmenDefenseLayer* MirrorLayer =
		Request.Defense.Layers.FindByPredicate(
			[&Fixture](const FShanmenDefenseLayer& Layer)
			{
				return Layer.SourceInstanceId == Fixture.HeartMirrorId;
			});
	TestTrue(TEXT("Lethal impact resolves through one exact mirror layer"),
		MirrorLayer
		&& Request.Defense.Layers.Num() == 1
		&& MirrorLayer->Operation
			== EShanmenDefenseOperation::PreventLethal
		&& MirrorLayer->Order == FShanmenDefenseOrder::LethalInterception
		&& FMath::IsNearlyEqual(MirrorLayer->Magnitude, 1.0f)
		&& MirrorLayer->LayerTags.HasTagExact(
			FShanmenCombatNativeTags::DefenseLethalIntercept())
		&& Impact.TriggeredLayers.Num() == 1
		&& Impact.TriggeredLayers[0].LayerId == MirrorLayer->LayerId
		&& FMath::IsNearlyEqual(Impact.PreventedDamage, 3.0f)
		&& FMath::IsNearlyEqual(Impact.FinalDamage, 2.0f));

	const Fdemo_mapShanmenDefenseResourceCoordinationResult Coordinated =
		Fdemo_mapShanmenDefenseResourceAdapter::CoordinateImpact(
			*Fixture.Authority, *Health, Request, Impact);
	FShanmenItemAuthoritySnapshot After;
	const FShanmenItemInstance* Mirror =
		Fixture.Authority->TryCaptureSnapshot(After)
		? FindAuthorityItem(After, Fixture.HeartMirrorId)
		: nullptr;
	TestTrue(TEXT("Triggered mirror atomically leaves one vitality and spends one charge"),
		Coordinated.IsSuccess()
		&& Coordinated.PrepareRequest.OrderedLines.Num() == 1
		&& Coordinated.PrepareRequest.TriggeredLineCount == 1
		&& FMath::IsNearlyEqual(Health->GetCurrentVitality(), 1.0f)
		&& Mirror && Mirror->Charges == 0);

	const Fdemo_mapShanmenDefenseResourceCoordinationResult Replayed =
		Fdemo_mapShanmenDefenseResourceAdapter::CoordinateImpact(
			*Fixture.Authority, *Health, Request, Impact);
	FShanmenItemAuthoritySnapshot ReplaySnapshot;
	Fixture.Authority->TryCaptureSnapshot(ReplaySnapshot);
	const FShanmenItemInstance* ReplayedMirror =
		FindAuthorityItem(ReplaySnapshot, Fixture.HeartMirrorId);
	TestTrue(TEXT("Exact replay cannot spend a second mirror charge"),
		Replayed.IsSuccess()
		&& Replayed.PrepareCommand.Status
			== EShanmenItemDurableCommandStatus::Replayed
		&& Replayed.FinalizeCommand.Status
			== EShanmenItemDurableCommandStatus::Replayed
		&& FMath::IsNearlyEqual(Health->GetCurrentVitality(), 1.0f)
		&& ReplayedMirror && ReplayedMirror->Charges == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapHeartMirrorSettlementLifecycleTest,
	"Shanmen.0_0_10.Items.DefenseResourceAdapter.HeartMirrorSettlementLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapHeartMirrorSettlementLifecycleTest::RunTest(const FString&)
{
	FP54DefenseFixture Fixture;
	if (!Fixture.SeedAndStart(
			*this, TEXT("HeartMirrorSettlementLifecycle"), false, true))
	{
		return false;
	}
	const FGuid FirstRunId = Fixture.Started.ActiveRunId;
	Udemo_mapPlayerHealthComponent* FirstHealth =
		NewObject<Udemo_mapPlayerHealthComponent>(GetTransientPackage());
	if (!FirstHealth
		|| !FirstHealth->TryBindCombatEntity(
			FGuid(0xD3570500, 0, 0, 1)))
	{
		AddError(TEXT("P17.2 first Run could not bind vitality."));
		return false;
	}
	FirstHealth->SetCurrentHealthForAutomation(3);
	FShanmenImpactRequest FirstRequest;
	FShanmenImpactResult FirstImpact;
	FString Diagnostic;
	if (!BuildP54Impact(
			Fixture,
			*FirstHealth,
			20,
			MakeHeartMirrorBaseDefense(),
			FirstRequest,
			FirstImpact,
			Diagnostic,
			5.0f))
	{
		AddError(Diagnostic);
		return false;
	}
	const Fdemo_mapShanmenDefenseResourceCoordinationResult FirstSpend =
		Fdemo_mapShanmenDefenseResourceAdapter::CoordinateImpact(
			*Fixture.Authority,
			*FirstHealth,
			FirstRequest,
			FirstImpact);
	FShanmenItemAuthoritySnapshot SpentInFirstRun;
	const FShanmenItemInstance* FirstSpentMirror =
		Fixture.Authority->TryCaptureSnapshot(SpentInFirstRun)
			? FindAuthorityItem(
				SpentInFirstRun, Fixture.HeartMirrorId)
			: nullptr;
	TestTrue(TEXT("First Run spends the only mirror charge before settlement"),
		FirstSpend.IsSuccess()
			&& FMath::IsNearlyEqual(
				FirstHealth->GetCurrentVitality(), 1.0f)
			&& FirstSpentMirror
			&& FirstSpentMirror->State
				== EShanmenItemInstanceState::Deployed
			&& FirstSpentMirror->Charges == 0);

	Fdemo_mapSettlementSummary ExtractionSummary;
	const Fdemo_mapItemOperationResult RuntimeExtraction =
		Fixture.Runtime->RequestSettlement(
			Edemo_mapRunEndReason::Extraction, ExtractionSummary);
	const Fdemo_mapShanmenRunFinalizeResult Extraction =
		RuntimeExtraction.bSuccess
			? Fdemo_mapShanmenRunLifecycleAdapter::FinalizeSettlement(
				*Fixture.Authority, ExtractionSummary)
			: Fdemo_mapShanmenRunFinalizeResult();
	FShanmenItemAuthoritySnapshot Extracted;
	const FShanmenItemInstance* ExtractedMirror =
		Fixture.Authority->TryCaptureSnapshot(Extracted)
			? FindAuthorityItem(Extracted, Fixture.HeartMirrorId)
			: nullptr;
	const Fdemo_mapProfilePreparationSnapshot ExtractedPreparation =
		Fixture.Session->GetPreparationSnapshot();
	const Fdemo_mapProfilePreparationStashRow* ExtractedRow =
		FindPreparationRow(
			ExtractedPreparation, Fixture.HeartMirrorId);
	const Fdemo_mapProfilePreparationViewState ExtractedView =
		Fdemo_mapProfilePreparationPresenter::BuildViewState(
			ExtractedPreparation);
	const Fdemo_mapProfilePreparationRowView* ExtractedViewRow =
		ExtractedView.OrderedPermanentStashRows.FindByPredicate(
			[&Fixture](const Fdemo_mapProfilePreparationRowView& Row)
			{
				return Row.ItemInstanceId == Fixture.HeartMirrorId;
			});
	TestTrue(TEXT("Extraction secures the same spent mirror without refilling it"),
		Extraction.IsFinalized()
			&& ExtractionSummary.RunId == FirstRunId
			&& ExtractionSummary.Reason
				== Edemo_mapRunEndReason::Extraction
			&& ExtractedMirror
			&& ExtractedMirror->State == EShanmenItemInstanceState::Stored
			&& ExtractedMirror->Charges == 0
			&& ExtractedRow
			&& ExtractedRow->Charges == 0
			&& ExtractedRow->MaxCharges == 1
			&& ExtractedRow->HasValidResourceState()
			&& ExtractedViewRow
			&& ExtractedViewRow->ResourceLabel == TEXT("CHG 0/1"));

	if (!Fixture.RestartAndBind(*this))
	{
		return false;
	}
	FShanmenItemAuthoritySnapshot RestartedAfterExtraction;
	const FShanmenItemInstance* RestartedSpentMirror =
		Fixture.Authority->TryCaptureSnapshot(RestartedAfterExtraction)
			? FindAuthorityItem(
				RestartedAfterExtraction, Fixture.HeartMirrorId)
			: nullptr;
	const Fdemo_mapProfilePreparationStashRow* RestartedSpentRow =
		FindPreparationRow(
			Fixture.Session->GetPreparationSnapshot(),
			Fixture.HeartMirrorId);
	TestTrue(TEXT("Process restart preserves the extracted zero-charge state"),
		RestartedSpentMirror
			&& RestartedSpentMirror->State
				== EShanmenItemInstanceState::Stored
			&& RestartedSpentMirror->Charges == 0
			&& RestartedSpentRow
			&& RestartedSpentRow->Charges == 0
			&& RestartedSpentRow->MaxCharges == 1);

	const Fdemo_mapProfilePreparationSelectionResult Reselected =
		Fixture.Session->SetPreparationEquipment(
			Fdemo_mapItemIds::AccessorySlot, Fixture.HeartMirrorId);
	Fixture.Runtime->ResetForAutomation();
	Fixture.Started =
		Fdemo_mapShanmenRunLifecycleAdapter::StartPreparedRun(
			*Fixture.Authority, *Fixture.Runtime);
	FShanmenItemAuthoritySnapshot Redeployed;
	const FShanmenItemInstance* RedeployedMirror =
		Fixture.Authority->TryCaptureSnapshot(Redeployed)
			? FindAuthorityItem(Redeployed, Fixture.HeartMirrorId)
			: nullptr;
	TestTrue(TEXT("A later Run redeploys the same identity without restoring charge"),
		Reselected.IsAccepted()
			&& Fixture.Started.IsStarted()
			&& Fixture.Started.ActiveRunId != FirstRunId
			&& Fixture.Started.RunCorrelation.AccessoryItemInstanceId
				== Fixture.HeartMirrorId
			&& RedeployedMirror
			&& RedeployedMirror->State
				== EShanmenItemInstanceState::Deployed
			&& RedeployedMirror->Charges == 0);

	Udemo_mapPlayerHealthComponent* SecondHealth =
		NewObject<Udemo_mapPlayerHealthComponent>(GetTransientPackage());
	if (!SecondHealth
		|| !SecondHealth->TryBindCombatEntity(
			FGuid(0xD3570500, 0, 0, 2)))
	{
		AddError(TEXT("P17.2 later Run could not bind vitality."));
		return false;
	}
	FShanmenDefenseSnapshot ExhaustedDefense = MakeHeartMirrorBaseDefense();
	const Fdemo_mapShanmenDefenseResourcePreparationResult Exhausted =
		Fdemo_mapShanmenDefenseResourceAdapter::PrepareImpactDefense(
			*Fixture.Authority,
			*SecondHealth,
			FGuid(0xD3570500, 0, 0, 3),
			ExhaustedDefense);
	TestTrue(TEXT("The later Run cannot synthesize a defense layer from zero charge"),
		Exhausted.IsSuccess()
			&& Exhausted.Status
				== Edemo_mapShanmenDefenseResourcePreparationStatus::
					ResourceUnavailable
			&& !Exhausted.HasResourceLayer()
			&& Exhausted.ReservationIds.IsEmpty()
			&& ExhaustedDefense.Layers.IsEmpty());

	Fdemo_mapSettlementSummary DeathSummary;
	const Fdemo_mapItemOperationResult RuntimeDeath =
		Fixture.Runtime->RequestSettlement(
			Edemo_mapRunEndReason::Death, DeathSummary);
	const Fdemo_mapShanmenRunFinalizeResult Death = RuntimeDeath.bSuccess
		? Fdemo_mapShanmenRunLifecycleAdapter::FinalizeSettlement(
			*Fixture.Authority, DeathSummary)
		: Fdemo_mapShanmenRunFinalizeResult();
	FShanmenItemAuthoritySnapshot Destroyed;
	const FShanmenItemInstance* DestroyedMirror =
		Fixture.Authority->TryCaptureSnapshot(Destroyed)
			? FindAuthorityItem(Destroyed, Fixture.HeartMirrorId)
			: nullptr;
	TestTrue(TEXT("Death converts the spent mirror to a zeroed tombstone"),
		Death.IsFinalized()
			&& DeathSummary.RunId == Fixture.Started.ActiveRunId
			&& DeathSummary.Reason == Edemo_mapRunEndReason::Death
			&& DeathSummary.RuntimeSnapshot.OrderedSecuredItems.IsEmpty()
			&& DestroyedMirror
			&& DestroyedMirror->State
				== EShanmenItemInstanceState::Destroyed
			&& DestroyedMirror->Quantity == 0
			&& DestroyedMirror->Durability == 0
			&& DestroyedMirror->Charges == 0
			&& !FindPreparationRow(
				Fixture.Session->GetPreparationSnapshot(),
				Fixture.HeartMirrorId));

	if (!Fixture.RestartAndBind(*this))
	{
		return false;
	}
	FShanmenItemAuthoritySnapshot RestartedAfterDeath;
	const FShanmenItemInstance* RestartedTombstone =
		Fixture.Authority->TryCaptureSnapshot(RestartedAfterDeath)
			? FindAuthorityItem(
				RestartedAfterDeath, Fixture.HeartMirrorId)
			: nullptr;
	TestTrue(TEXT("Destroyed mirror remains terminal after another restart"),
		RestartedTombstone
			&& RestartedTombstone->State
				== EShanmenItemInstanceState::Destroyed
			&& RestartedTombstone->Quantity == 0
			&& RestartedTombstone->Durability == 0
			&& RestartedTombstone->Charges == 0
			&& !FindPreparationRow(
				Fixture.Session->GetPreparationSnapshot(),
				Fixture.HeartMirrorId));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapHeartMirrorUntriggeredCancellationTest,
	"Shanmen.0_0_10.Items.DefenseResourceAdapter.HeartMirrorUntriggeredCancel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapHeartMirrorUntriggeredCancellationTest::RunTest(
	const FString&)
{
	FP54DefenseFixture Fixture;
	if (!Fixture.SeedAndStart(
			*this, TEXT("HeartMirrorUntriggered"), false, true))
	{
		return false;
	}
	Udemo_mapPlayerHealthComponent* Health =
		NewObject<Udemo_mapPlayerHealthComponent>(GetTransientPackage());
	if (!Health
		|| !Health->TryBindCombatEntity(FGuid(0xD3570200, 0, 0, 1)))
	{
		AddError(TEXT("P17.0 nonlethal fixture could not bind vitality."));
		return false;
	}
	FShanmenImpactRequest Request;
	FShanmenImpactResult Impact;
	FString Diagnostic;
	if (!BuildP54Impact(
			Fixture, *Health, 11, MakeHeartMirrorBaseDefense(),
			Request, Impact, Diagnostic, 3.0f))
	{
		AddError(Diagnostic);
		return false;
	}
	const FShanmenDefenseLayer* MirrorLayer =
		Request.Defense.Layers.FindByPredicate(
			[&Fixture](const FShanmenDefenseLayer& Layer)
			{
				return Layer.SourceInstanceId == Fixture.HeartMirrorId;
			});
	TestTrue(TEXT("Nonlethal impact does not trigger the mirror"),
		MirrorLayer && Impact.TriggeredLayers.IsEmpty()
		&& FMath::IsNearlyZero(Impact.PreventedDamage)
		&& FMath::IsNearlyEqual(Impact.FinalDamage, 3.0f));
	const Fdemo_mapShanmenDefenseResourceCoordinationResult Coordinated =
		Fdemo_mapShanmenDefenseResourceAdapter::CoordinateImpact(
			*Fixture.Authority, *Health, Request, Impact);
	FShanmenItemAuthoritySnapshot After;
	Fixture.Authority->TryCaptureSnapshot(After);
	const FShanmenItemInstance* Mirror =
		FindAuthorityItem(After, Fixture.HeartMirrorId);
	const FShanmenItemReservationSnapshot* Reservation = MirrorLayer
		? After.Reservations.FindByPredicate(
			[MirrorLayer](const FShanmenItemReservationSnapshot& Candidate)
			{
				return Candidate.ReservationId == MirrorLayer->LayerId;
			})
		: nullptr;
	TestTrue(TEXT("Untriggered mirror reservation cancels without charge loss"),
		Coordinated.IsSuccess()
		&& Coordinated.PrepareRequest.TriggeredLineCount == 0
		&& FMath::IsNearlyEqual(Health->GetCurrentVitality(), 2.0f)
		&& Mirror && Mirror->Charges == 1
		&& Reservation
		&& Reservation->State == EShanmenItemReservationState::Cancelled);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapHeartMirrorSpiritGuardCoexistenceTest,
	"Shanmen.0_0_10.Items.DefenseResourceAdapter.HeartMirrorSpiritGuardCoexistence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapHeartMirrorSpiritGuardCoexistenceTest::RunTest(
	const FString&)
{
	FP54DefenseFixture Fixture;
	if (!Fixture.SeedAndStart(
			*this, TEXT("HeartMirrorCoexistence"), true, true))
	{
		return false;
	}
	Udemo_mapPlayerHealthComponent* Health =
		NewObject<Udemo_mapPlayerHealthComponent>(GetTransientPackage());
	if (!Health
		|| !Health->TryBindCombatEntity(FGuid(0xD3570300, 0, 0, 1)))
	{
		AddError(TEXT("P17.0 coexistence fixture could not bind vitality."));
		return false;
	}
	FShanmenImpactRequest Request;
	FShanmenImpactResult Impact;
	FString Diagnostic;
	if (!BuildP54Impact(
			Fixture, *Health, 12, MakeP54BaseDefense(false),
			Request, Impact, Diagnostic, 10.0f))
	{
		AddError(Diagnostic);
		return false;
	}
	TestTrue(TEXT("Robe then mirror resolve in canonical order"),
		Request.Defense.Layers.Num() == 2
		&& Impact.TriggeredLayers.Num() == 2
		&& Impact.TriggeredLayers[0].SourceInstanceId
			== Fixture.SpiritGuardId
		&& Impact.TriggeredLayers[1].SourceInstanceId
			== Fixture.HeartMirrorId
		&& FMath::IsNearlyEqual(
			Impact.TriggeredLayers[0].PreventedDamage, 2.0f)
		&& FMath::IsNearlyEqual(
			Impact.TriggeredLayers[1].PreventedDamage, 4.0f)
		&& FMath::IsNearlyEqual(Impact.FinalDamage, 4.0f));
	const Fdemo_mapShanmenDefenseResourceCoordinationResult Coordinated =
		Fdemo_mapShanmenDefenseResourceAdapter::CoordinateImpact(
			*Fixture.Authority, *Health, Request, Impact);
	FShanmenItemAuthoritySnapshot After;
	Fixture.Authority->TryCaptureSnapshot(After);
	const FShanmenItemInstance* Armor =
		FindAuthorityItem(After, Fixture.SpiritGuardId);
	const FShanmenItemInstance* Mirror =
		FindAuthorityItem(After, Fixture.HeartMirrorId);
	TestTrue(TEXT("One intent commits both resource sources with vitality"),
		Coordinated.IsSuccess()
		&& Coordinated.PrepareRequest.OrderedLines.Num() == 2
		&& Coordinated.PrepareRequest.TriggeredLineCount == 2
		&& FMath::IsNearlyEqual(Health->GetCurrentVitality(), 1.0f)
		&& Armor && Armor->Durability == 19
		&& Mirror && Mirror->Charges == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapHeartMirrorPreIntentRestartRecoveryTest,
	"Shanmen.0_0_10.Items.DefenseResourceAdapter.HeartMirrorPreIntentRestart",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapHeartMirrorPreIntentRestartRecoveryTest::RunTest(
	const FString&)
{
	FP54DefenseFixture Fixture;
	if (!Fixture.SeedAndStart(
			*this, TEXT("HeartMirrorRestart"), false, true))
	{
		return false;
	}
	Udemo_mapPlayerHealthComponent* Health =
		NewObject<Udemo_mapPlayerHealthComponent>(GetTransientPackage());
	if (!Health
		|| !Health->TryBindCombatEntity(FGuid(0xD3570400, 0, 0, 1)))
	{
		AddError(TEXT("P17.0 restart fixture could not bind vitality."));
		return false;
	}
	FShanmenImpactRequest Request;
	FShanmenImpactResult Impact;
	FString Diagnostic;
	if (!BuildP54Impact(
			Fixture, *Health, 13, MakeHeartMirrorBaseDefense(),
			Request, Impact, Diagnostic, 5.0f))
	{
		AddError(Diagnostic);
		return false;
	}
	const FGuid ReservationId = Request.Defense.Layers.Num() == 1
		? Request.Defense.Layers[0].LayerId : FGuid();
	if (!Fixture.RestartAndBind(*this))
	{
		return false;
	}
	const Fdemo_mapShanmenDefenseOrphanRecoveryResult Recovered =
		Fdemo_mapShanmenDefenseResourceAdapter::
			RecoverOrphanedDefenseReservations(*Fixture.Authority);
	const Fdemo_mapShanmenDefenseOrphanRecoveryResult Replayed =
		Fdemo_mapShanmenDefenseResourceAdapter::
			RecoverOrphanedDefenseReservations(*Fixture.Authority);
	FShanmenItemAuthoritySnapshot After;
	Fixture.Authority->TryCaptureSnapshot(After);
	const FShanmenItemInstance* Mirror =
		FindAuthorityItem(After, Fixture.HeartMirrorId);
	const FShanmenItemReservationSnapshot* Reservation =
		After.Reservations.FindByPredicate(
			[ReservationId](const FShanmenItemReservationSnapshot& Candidate)
			{
				return Candidate.ReservationId == ReservationId;
			});
	TestTrue(TEXT("Restart recovers one orphan mirror reservation exactly once"),
		ReservationId.IsValid()
		&& Recovered.bSuccess && Recovered.CancelledReservationCount == 1
		&& Replayed.bSuccess && Replayed.CancelledReservationCount == 0
		&& Mirror && Mirror->Charges == 1
		&& Reservation
		&& Reservation->State == EShanmenItemReservationState::Cancelled);
	return true;
}

#endif
