#if WITH_DEV_AUTOMATION_TESTS

#include "demo_mapShanmenDefenseResourceAdapter.h"

#include "Misc/AutomationTest.h"
#include "ShanmenCombatResolver.h"
#include "ShanmenCombatTags.h"
#include "demo_mapProfilePreparationTypes.h"

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

#endif
