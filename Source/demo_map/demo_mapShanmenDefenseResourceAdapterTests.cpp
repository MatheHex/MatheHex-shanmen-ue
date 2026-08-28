#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenDefenseResourceAdapter.h"

#include "Misc/AutomationTest.h"
#include "ShanmenCombatResolver.h"
#include "ShanmenCombatTags.h"
#include "demo_map0909BSectWarehouseService.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapItemSubsystem.h"
#include "demo_mapPlayerHealthComponent.h"
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

	struct FP54DefenseFixture
	{
		FString Root;
		Fdemo_mapProfileStorageContext Storage;
		Fdemo_mapPersistentProfile SeedProfile;
		FGuid SpiritGuardId;
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

		bool SeedAndStart(FAutomationTestBase& Test, const TCHAR* Label)
		{
			Root = NewP54Root(Label);
			Storage = Fdemo_mapProfileStorageContext::ForRoot(Root);
			Fdemo_mapProfileRepository Repository;
			SeedProfile = Repository.CreateFreshProfile();
			Fdemo_mapPersistentItemRecord SpiritGuard;
			SpiritGuardId = SpiritGuard.ItemInstanceId = FGuid::NewGuid();
			SpiritGuard.ItemDefinitionId = Fdemo_mapItemIds::SpiritGuardRobe;
			SpiritGuard.StackCount = 1;
			SpiritGuard.PersistentDomain =
				Edemo_mapPersistentDomain::PermanentStash;
			SeedProfile.PermanentStash.Add(SpiritGuard);
			SeedProfile.PreparationLayout.ArmorItemInstanceId = SpiritGuardId;
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
				|| Started.RunCorrelation.ArmorItemInstanceId != SpiritGuardId)
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

	bool BuildP54Impact(
		const FP54DefenseFixture& Fixture,
		Udemo_mapPlayerHealthComponent& Health,
		uint64 Sequence,
		FShanmenDefenseSnapshot Defense,
		FShanmenImpactRequest& OutRequest,
		FShanmenImpactResult& OutResult,
		FString& OutDiagnostic)
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
		OutRequest.Damage.FormulaId = TEXT("Formula.Test.P5.4.FixedFour");
		OutRequest.Damage.RawDamage = 4.0f;
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

#endif
