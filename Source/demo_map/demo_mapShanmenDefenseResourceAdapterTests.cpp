#if WITH_DEV_AUTOMATION_TESTS

#include "demo_mapShanmenDefenseResourceAdapter.h"

#include "Misc/AutomationTest.h"
#include "demo_mapProfilePreparationTypes.h"

namespace
{
	const FGuid TestOwnerId(0xD3520001, 0, 0, 1);
	const FGuid TestScopeId(0xD3520002, 0, 0, 1);
	const FGuid TestActiveRunId(0xD3520003, 0, 0, 1);
	const FGuid TestSwordId(0xD3520004, 0, 0, 1);
	const FGuid TestImpactId(0xD3520005, 0, 0, 1);
	const FGuid TestReservationId(0xD3520006, 0, 0, 1);

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
		Correlation.OrderedPreparedItemInstanceIds = { TestSwordId };
		Correlation.HotbarItemInstanceIds.SetNum(
			Fdemo_mapPersistentPreparationLayout::HotbarSlotCount);
		return Correlation;
	}

	FShanmenContentStamp MakeContent()
	{
		FShanmenContentStamp Content;
		Content.Version = TEXT("Shanmen.0.0.10.P5.2");
		Content.Digest = TEXT("P5.2.DefenseResourceCommit.v1");
		return Content;
	}

	FShanmenImpactResult MakeImpact(bool bRequiresCommit = true)
	{
		FShanmenImpactResult Impact;
		Impact.bAccepted = true;
		Impact.ImpactId = TestImpactId;
		Impact.Outcome = EShanmenDefenseOutcome::FullyPrevented;
		Impact.RawDamage = 10.0f;
		Impact.PreventedDamage = 10.0f;
		Impact.FinalDamage = 0.0f;
		FShanmenDefenseLayerResult& Layer =
			Impact.TriggeredLayers.AddDefaulted_GetRef();
		Layer.LayerId = TestReservationId;
		Layer.RuleId = TEXT("Defense.Artifact.HeartMirror");
		Layer.SourceInstanceId = TestSwordId;
		Layer.Operation = EShanmenDefenseOperation::PreventLethal;
		Layer.PreventedDamage = 10.0f;
		Layer.bRequiresCommit = bRequiresCommit;
		return Impact;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapShanmenDefenseResourceCanonicalRequestTest,
	"Shanmen.0_0_10.Items.DefenseResourceAdapter.CanonicalRequest",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapShanmenDefenseResourceCanonicalRequestTest::RunTest(
	const FString&)
{
	const Fdemo_mapShanmenRunCorrelation Correlation = MakeCorrelation();
	const FShanmenContentStamp Content = MakeContent();
	const FShanmenImpactResult Impact = MakeImpact();
	FShanmenItemRunResourceCommitRequest First;
	FShanmenItemRunResourceCommitRequest Replay;
	FString Diagnostic;
	FString ReplayDiagnostic;
	TestTrue(TEXT("One triggered layer forms a canonical ActiveRun command"),
		Correlation.IsValid() && Impact.IsConserved()
			&& Fdemo_mapShanmenDefenseResourceAdapter::BuildCommitRequest(
				Impact, Correlation, Content, First, Diagnostic)
			&& First.IsValid()
			&& First.ActiveRunId == TestActiveRunId
			&& First.OrderedLines.Num() == 1
			&& First.OrderedLines[0].ReservationId == TestReservationId
			&& First.OrderedLines[0].ItemInstanceId == TestSwordId);
	TestTrue(TEXT("Exact impact replay derives the exact request identity"),
		Fdemo_mapShanmenDefenseResourceAdapter::BuildCommitRequest(
			Impact, Correlation, Content, Replay, ReplayDiagnostic)
			&& Replay.Context.RequestId == First.Context.RequestId
			&& Replay.OrderedLines == First.OrderedLines);

	FShanmenImpactResult NoCommit = MakeImpact(false);
	FShanmenItemRunResourceCommitRequest NoCommitRequest;
	TestFalse(TEXT("Non-resource defense does not fabricate an item command"),
		Fdemo_mapShanmenDefenseResourceAdapter::BuildCommitRequest(
			NoCommit, Correlation, Content, NoCommitRequest, Diagnostic));
	TestTrue(TEXT("Non-resource defense leaves an empty request"),
		NoCommitRequest.OrderedLines.IsEmpty());

	FShanmenImpactResult Unprepared = MakeImpact();
	Unprepared.TriggeredLayers[0].SourceInstanceId =
		FGuid(0xD3520020, 0, 0, 1);
	FShanmenItemRunResourceCommitRequest InvalidRequest;
	TestFalse(TEXT("An unprepared source identity fails closed"),
		Fdemo_mapShanmenDefenseResourceAdapter::BuildCommitRequest(
			Unprepared, Correlation, Content, InvalidRequest, Diagnostic));

	FShanmenImpactResult Duplicate = MakeImpact();
	const FShanmenDefenseLayerResult DuplicateLayer =
		Duplicate.TriggeredLayers[0];
	Duplicate.TriggeredLayers.Add(DuplicateLayer);
	TestFalse(TEXT("Duplicate reservation identity fails before authority"),
		Fdemo_mapShanmenDefenseResourceAdapter::BuildCommitRequest(
			Duplicate, Correlation, Content, InvalidRequest, Diagnostic));
	return true;
}

#endif
