#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "ShanmenVitalityAuthority.h"
#include "demo_mapAttributeComponent.h"
#include "demo_mapAttributeDefinitions.h"
#include "demo_mapGameMode.h"
#include "demo_mapShanmenCombatConditionComponent.h"
#include "demo_mapShanmenCombatRunFixedTimeline.h"

namespace
{
	constexpr EAutomationTestFlags ConditionFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;
	const FGuid ConditionRunId(
		0xC1400001, 0xC1400002, 0xC1400003, 0xC1400004);
	const FGuid ForeignRunId(
		0xC1410001, 0xC1410002, 0xC1410003, 0xC1410004);
	const FGuid ConditionTargetId(
		0xC1420001, 0xC1420002, 0xC1420003, 0xC1420004);
	const FGuid ForeignTargetId(
		0xC1430001, 0xC1430002, 0xC1430003, 0xC1430004);
	const FGuid ImpactA(
		0xC1440001, 0xC1440002, 0xC1440003, 0xC1440004);
	const FGuid ImpactB(
		0xC1450001, 0xC1450002, 0xC1450003, 0xC1450004);
	const FGuid ResolutionA(
		0xC1460001, 0xC1460002, 0xC1460003, 0xC1460004);
	const FGuid ResolutionB(
		0xC1470001, 0xC1470002, 0xC1470003, 0xC1470004);

	FShanmenVitalityCommitReceipt MakeCommittedReceipt(
		const FGuid& TargetEntityId,
		const FGuid& ImpactId,
		const FGuid& ResolutionId,
		const float CurrentVitality = 100.0f,
		const float RequestedDamage = 10.0f)
	{
		FShanmenVitalityAuthority Authority;
		check(FShanmenVitalityAuthority::TryCreate(
			TargetEntityId,
			CurrentVitality,
			100.0f,
			0,
			Authority));
		FShanmenVitalityCommitCommand Command;
		check(FShanmenVitalityCommitCommand::TryRestoreFromDurableIntent(
			ImpactId,
			ResolutionId,
			TargetEntityId,
			0,
			CurrentVitality,
			100.0f,
			RequestedDamage,
			0.0f,
			RequestedDamage,
			EShanmenDefenseOutcome::Applied,
			Command));
		const FShanmenVitalityCommitResult Result = Authority.Commit(Command);
		check(Result.Status == EShanmenVitalityCommitStatus::Committed);
		check(Result.Receipt.IsValid());
		return Result.Receipt;
	}

	struct FConditionFixture
	{
		Udemo_mapAttributeComponent* Attributes = nullptr;
		Udemo_mapShanmenCombatConditionComponent* Conditions = nullptr;
		Fdemo_mapShanmenCombatRunFixedTimeline Timeline;
		Fdemo_mapShanmenCombatRunTimelineSample Sample;
		bool bReady = false;

		FConditionFixture()
		{
			Attributes = NewObject<Udemo_mapAttributeComponent>(
				GetTransientPackage());
			Conditions = NewObject<
				Udemo_mapShanmenCombatConditionComponent>(
					GetTransientPackage());
			FString Diagnostic;
			bReady = Attributes != nullptr
				&& Conditions != nullptr
				&& Timeline.TryBegin(ConditionRunId, Diagnostic)
				&& Conditions->TryBegin(
					ConditionRunId,
					ConditionTargetId,
					Timeline.GetTimelineId(),
					Attributes,
					Diagnostic)
				&& Timeline.TryCapture(Sample);
		}

		bool AdvanceTicks(const int64 TickCount)
		{
			int64 AdvancedTicks = INDEX_NONE;
			FString Diagnostic;
			return TickCount >= 0
				&& Timeline.TryAdvance(
					static_cast<double>(TickCount)
						/ static_cast<double>(
							Fdemo_mapShanmenCombatRunFixedTimeline::
								CanonicalTicksPerSecond()),
					AdvancedTicks,
					Diagnostic)
				&& AdvancedTicks == TickCount
				&& Timeline.TryCapture(Sample);
		}

		float MoveSpeed() const
		{
			float Value = -1.0f;
			check(Attributes->GetFinalValue(
				Fdemo_mapAttributeIds::MoveSpeed,
				Value));
			return Value;
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapMeridianShockCommittedImpactTest,
	"Shanmen.0_0_10.Product.CombatCondition.MeridianShock.CommittedImpact",
	ConditionFlags)

bool Fdemo_mapMeridianShockCommittedImpactTest::RunTest(const FString&)
{
	FConditionFixture Fixture;
	TestTrue(TEXT("condition fixture begins"), Fixture.bReady);
	if (!Fixture.bReady)
	{
		return false;
	}
	TestTrue(TEXT("base movement begins at the product default"),
		FMath::IsNearlyEqual(Fixture.MoveSpeed(), 600.0f));
	const FShanmenVitalityCommitReceipt Vitality = MakeCommittedReceipt(
		ConditionTargetId,
		ImpactA,
		ResolutionA);
	const Fdemo_mapShanmenCombatConditionApplicationResult Applied =
		Fixture.Conditions->TryApplyMeridianShock(Vitality, Fixture.Sample);
	TestTrue(TEXT("committed damage applies one immutable condition receipt"),
		Applied.IsSuccess()
			&& Applied.Status
				== Edemo_mapShanmenCombatConditionApplicationStatus::Applied
			&& Applied.Receipt.GetAppliedAtTick() == 0
			&& Applied.Receipt.GetExpiryTick() == 90
			&& Applied.Receipt.GetConditionRevision() == 1);
	TestTrue(TEXT("Meridian Shock projects exactly one x0.75 movement modifier"),
		Fixture.Conditions->IsMeridianShockActive()
			&& Fixture.Conditions->NumProcessedApplications() == 1
			&& Fixture.Attributes->GetModifierCountBySource(
				Udemo_mapShanmenCombatConditionComponent::
					MeridianShockModifierSourceId()) == 1
			&& FMath::IsNearlyEqual(Fixture.MoveSpeed(), 450.0f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapMeridianShockReplayRefreshTest,
	"Shanmen.0_0_10.Product.CombatCondition.MeridianShock.ReplayAndRefresh",
	ConditionFlags)

bool Fdemo_mapMeridianShockReplayRefreshTest::RunTest(const FString&)
{
	FConditionFixture Fixture;
	TestTrue(TEXT("condition fixture begins"), Fixture.bReady);
	if (!Fixture.bReady)
	{
		return false;
	}
	const FShanmenVitalityCommitReceipt FirstReceipt = MakeCommittedReceipt(
		ConditionTargetId,
		ImpactA,
		ResolutionA);
	const Fdemo_mapShanmenCombatConditionApplicationResult First =
		Fixture.Conditions->TryApplyMeridianShock(
			FirstReceipt,
			Fixture.Sample);
	const Fdemo_mapShanmenCombatConditionApplicationResult Replay =
		Fixture.Conditions->TryApplyMeridianShock(
			FirstReceipt,
			Fixture.Sample);
	TestTrue(TEXT("exact Impact replay returns original evidence without mutation"),
		First.IsSuccess()
			&& Replay.IsSuccess()
			&& Replay.Status
				== Edemo_mapShanmenCombatConditionApplicationStatus::AlreadyApplied
			&& Replay.Receipt.GetApplicationId()
				== First.Receipt.GetApplicationId()
			&& Fixture.Conditions->GetConditionRevision() == 1
			&& Fixture.Conditions->NumProcessedApplications() == 1);

	TestTrue(TEXT("timeline reaches tick 30"), Fixture.AdvanceTicks(30));
	const FShanmenVitalityCommitReceipt SecondReceipt = MakeCommittedReceipt(
		ConditionTargetId,
		ImpactB,
		ResolutionB);
	const Fdemo_mapShanmenCombatConditionApplicationResult Refreshed =
		Fixture.Conditions->TryApplyMeridianShock(
			SecondReceipt,
			Fixture.Sample);
	TestTrue(TEXT("new committed Impact refreshes duration without stacking"),
		Refreshed.IsSuccess()
			&& Refreshed.Status
				== Edemo_mapShanmenCombatConditionApplicationStatus::Refreshed
			&& Refreshed.Receipt.GetExpiryTick() == 120
			&& Fixture.Conditions->GetMeridianShockExpiryTick() == 120
			&& Fixture.Conditions->GetConditionRevision() == 2
			&& Fixture.Conditions->NumProcessedApplications() == 2
			&& Fixture.Attributes->GetModifierCountBySource(
				Udemo_mapShanmenCombatConditionComponent::
					MeridianShockModifierSourceId()) == 1
			&& FMath::IsNearlyEqual(Fixture.MoveSpeed(), 450.0f));
	const Fdemo_mapShanmenCombatConditionApplicationResult OldReplay =
		Fixture.Conditions->TryApplyMeridianShock(
			FirstReceipt,
			Fixture.Sample);
	TestTrue(TEXT("older exact replay cannot shorten the refreshed deadline"),
		OldReplay.IsSuccess()
			&& OldReplay.Receipt.GetExpiryTick() == 90
			&& Fixture.Conditions->GetMeridianShockExpiryTick() == 120
			&& Fixture.Conditions->GetConditionRevision() == 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapMeridianShockIdentityFencesTest,
	"Shanmen.0_0_10.Product.CombatCondition.MeridianShock.IdentityFences",
	ConditionFlags)

bool Fdemo_mapMeridianShockIdentityFencesTest::RunTest(const FString&)
{
	FConditionFixture Fixture;
	TestTrue(TEXT("condition fixture begins"), Fixture.bReady);
	if (!Fixture.bReady)
	{
		return false;
	}
	const FShanmenVitalityCommitReceipt Valid = MakeCommittedReceipt(
		ConditionTargetId,
		ImpactA,
		ResolutionA);
	const FShanmenVitalityCommitReceipt ForeignTarget = MakeCommittedReceipt(
		ForeignTargetId,
		ImpactA,
		ResolutionA);
	const FShanmenVitalityCommitReceipt NoDamage = MakeCommittedReceipt(
		ConditionTargetId,
		ImpactB,
		ResolutionB,
		0.0f,
		10.0f);
	Fdemo_mapShanmenCombatRunTimelineSample ForeignTimeline;
	check(Fdemo_mapShanmenCombatRunTimelineSample::TryCapture(
		Fdemo_mapShanmenCombatRunFixedTimeline::MakeTimelineId(ForeignRunId),
		0,
		ForeignTimeline));
	TestTrue(TEXT("foreign target fails closed"),
		Fixture.Conditions->TryApplyMeridianShock(
			ForeignTarget,
			Fixture.Sample).Error
			== Edemo_mapShanmenCombatConditionError::TargetMismatch);
	TestTrue(TEXT("foreign timeline fails closed"),
		Fixture.Conditions->TryApplyMeridianShock(
			Valid,
			ForeignTimeline).Error
			== Edemo_mapShanmenCombatConditionError::TimelineMismatch);
	TestTrue(TEXT("zero applied vitality damage cannot author injury"),
		Fixture.Conditions->TryApplyMeridianShock(
			NoDamage,
			Fixture.Sample).Error
			== Edemo_mapShanmenCombatConditionError::NoCommittedDamage);

	TestTrue(TEXT("valid application succeeds"),
		Fixture.Conditions->TryApplyMeridianShock(
			Valid,
			Fixture.Sample).IsSuccess());
	const FShanmenVitalityCommitReceipt Conflict = MakeCommittedReceipt(
		ConditionTargetId,
		ImpactA,
		ResolutionB);
	TestTrue(TEXT("same Impact with foreign resolution is an atomic conflict"),
		Fixture.Conditions->TryApplyMeridianShock(
			Conflict,
			Fixture.Sample).Error
			== Edemo_mapShanmenCombatConditionError::ImpactConflict);
	TestTrue(TEXT("identity rejections leave one exact projection"),
		Fixture.Conditions->GetConditionRevision() == 1
			&& Fixture.Conditions->NumProcessedApplications() == 1
			&& Fixture.Attributes->GetActiveModifierCount() == 1
			&& FMath::IsNearlyEqual(Fixture.MoveSpeed(), 450.0f));

	TestTrue(TEXT("timeline reaches tick 30"), Fixture.AdvanceTicks(30));
	TestTrue(TEXT("tick 30 observation is accepted"),
		Fixture.Conditions->TryAdvance(Fixture.Sample).IsSuccess());
	Fdemo_mapShanmenCombatRunTimelineSample Stale;
	check(Fdemo_mapShanmenCombatRunTimelineSample::TryCapture(
		Fixture.Timeline.GetTimelineId(),
		29,
		Stale));
	TestTrue(TEXT("backward timeline sample fails closed"),
		Fixture.Conditions->TryAdvance(Stale).Error
			== Edemo_mapShanmenCombatConditionError::StaleTimeline);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapMeridianShockExpiryTest,
	"Shanmen.0_0_10.Product.CombatCondition.MeridianShock.Expiry",
	ConditionFlags)

bool Fdemo_mapMeridianShockExpiryTest::RunTest(const FString&)
{
	FConditionFixture Fixture;
	TestTrue(TEXT("condition fixture begins"), Fixture.bReady);
	if (!Fixture.bReady)
	{
		return false;
	}
	const FShanmenVitalityCommitReceipt Vitality = MakeCommittedReceipt(
		ConditionTargetId,
		ImpactA,
		ResolutionA);
	TestTrue(TEXT("condition applies"),
		Fixture.Conditions->TryApplyMeridianShock(
			Vitality,
			Fixture.Sample).IsSuccess());
	TestTrue(TEXT("timeline reaches tick 89"), Fixture.AdvanceTicks(89));
	const Fdemo_mapShanmenCombatConditionAdvanceResult BeforeExpiry =
		Fixture.Conditions->TryAdvance(Fixture.Sample);
	TestTrue(TEXT("condition remains active before exact deadline"),
		BeforeExpiry.IsSuccess()
			&& BeforeExpiry.Status
				== Edemo_mapShanmenCombatConditionAdvanceStatus::Observed
			&& Fixture.Conditions->IsMeridianShockActive()
			&& FMath::IsNearlyEqual(Fixture.MoveSpeed(), 450.0f));
	TestTrue(TEXT("timeline reaches exact tick 90"), Fixture.AdvanceTicks(1));
	const Fdemo_mapShanmenCombatConditionAdvanceResult Expired =
		Fixture.Conditions->TryAdvance(Fixture.Sample);
	TestTrue(TEXT("exact deadline removes the modifier and advances revision"),
		Expired.IsSuccess()
			&& Expired.Status
				== Edemo_mapShanmenCombatConditionAdvanceStatus::Expired
			&& !Fixture.Conditions->IsMeridianShockActive()
			&& Fixture.Conditions->GetConditionRevision() == 2
			&& Fixture.Attributes->GetActiveModifierCount() == 0
			&& FMath::IsNearlyEqual(Fixture.MoveSpeed(), 600.0f));
	const Fdemo_mapShanmenCombatConditionApplicationResult Replay =
		Fixture.Conditions->TryApplyMeridianShock(Vitality, Fixture.Sample);
	TestTrue(TEXT("expired exact Impact replay cannot resurrect the condition"),
		Replay.IsSuccess()
			&& Replay.Status
				== Edemo_mapShanmenCombatConditionApplicationStatus::AlreadyApplied
			&& !Fixture.Conditions->IsMeridianShockActive()
			&& Fixture.Attributes->GetActiveModifierCount() == 0
			&& Fixture.Conditions->GetConditionRevision() == 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapMeridianShockRunTeardownTest,
	"Shanmen.0_0_10.Product.CombatCondition.MeridianShock.RunTeardown",
	ConditionFlags)

bool Fdemo_mapMeridianShockRunTeardownTest::RunTest(const FString&)
{
	FConditionFixture Fixture;
	TestTrue(TEXT("condition fixture begins"), Fixture.bReady);
	if (!Fixture.bReady)
	{
		return false;
	}
	TestTrue(TEXT("condition applies"),
		Fixture.Conditions->TryApplyMeridianShock(
			MakeCommittedReceipt(
				ConditionTargetId,
				ImpactA,
				ResolutionA),
			Fixture.Sample).IsSuccess());
	FString Diagnostic;
	TestFalse(TEXT("foreign Run teardown is rejected atomically"),
		Fixture.Conditions->TryEnd(ForeignRunId, Diagnostic));
	TestTrue(TEXT("foreign teardown preserves condition and projection"),
		Fixture.Conditions->IsMeridianShockActive()
			&& FMath::IsNearlyEqual(Fixture.MoveSpeed(), 450.0f));
	TestTrue(TEXT("matching Run teardown removes projection and state"),
		Fixture.Conditions->TryEnd(ConditionRunId, Diagnostic)
			&& Fixture.Conditions->IsValid()
			&& Fixture.Conditions->IsEmpty()
			&& Fixture.Attributes->GetActiveModifierCount() == 0
			&& FMath::IsNearlyEqual(Fixture.MoveSpeed(), 600.0f));
	TestTrue(TEXT("same component can begin a later clean Run"),
		Fixture.Conditions->TryBegin(
			ForeignRunId,
			ConditionTargetId,
			Fdemo_mapShanmenCombatRunFixedTimeline::MakeTimelineId(
				ForeignRunId),
			Fixture.Attributes,
			Diagnostic)
			&& Fixture.Conditions->GetConditionRevision() == 0
			&& Fixture.Conditions->NumProcessedApplications() == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapMeridianShockStatusInitialSnapshotTest,
	"Shanmen.0_0_10.Product.CombatCondition.MeridianShock.Status.InitialSnapshot",
	ConditionFlags)

bool Fdemo_mapMeridianShockStatusInitialSnapshotTest::RunTest(
	const FString&)
{
	FConditionFixture Fixture;
	TestTrue(TEXT("condition fixture begins"), Fixture.bReady);
	if (!Fixture.bReady)
	{
		return false;
	}

	Fdemo_mapShanmenCombatConditionStatusSnapshot First;
	Fdemo_mapShanmenCombatConditionStatusSnapshot Repeated;
	TestTrue(TEXT("bound empty condition captures an immutable inactive status"),
		Fixture.Conditions->TryCaptureMeridianShockStatus(First)
			&& Fixture.Conditions->TryCaptureMeridianShockStatus(Repeated)
			&& First.IsValid()
			&& First.Matches(Repeated));
	TestTrue(TEXT("inactive status exposes canonical identity and authored prototype"),
		!First.IsActive()
			&& First.GetRunId() == ConditionRunId
			&& First.GetTargetEntityId() == ConditionTargetId
			&& First.GetTimelineId() == Fixture.Timeline.GetTimelineId()
			&& First.GetDefinitionId()
				== Udemo_mapShanmenCombatConditionComponent::
					MeridianShockDefinitionId()
			&& First.GetObservedTick() == 0
			&& First.GetExpiryTick() == INDEX_NONE
			&& First.GetRemainingTicks() == 0
			&& First.GetConditionRevision() == 0
			&& First.GetDurationTicks() == 90
			&& First.GetTimelineTicksPerSecond() == 30
			&& FMath::IsNearlyEqual(
				First.GetMoveSpeedMultiplier(),
				0.75f));

	Ademo_mapGameMode* EmptyGameMode =
		NewObject<Ademo_mapGameMode>(GetTransientPackage());
	TestNotNull(TEXT("empty GameMode query fixture exists"), EmptyGameMode);
	Fdemo_mapShanmenCombatConditionStatusSnapshot EmptyStatus = First;
	const bool bAvailable = EmptyGameMode
		? EmptyGameMode->TryGetMeridianShockStatus(EmptyStatus)
		: false;
	TestFalse(TEXT("GameMode query fails closed outside a bound Combat Run"),
		bAvailable);
	TestFalse(TEXT("failed GameMode query clears caller-owned output"),
		EmptyStatus.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapMeridianShockStatusActiveCountdownTest,
	"Shanmen.0_0_10.Product.CombatCondition.MeridianShock.Status.ActiveCountdown",
	ConditionFlags)

bool Fdemo_mapMeridianShockStatusActiveCountdownTest::RunTest(
	const FString&)
{
	FConditionFixture Fixture;
	TestTrue(TEXT("condition fixture begins"), Fixture.bReady);
	if (!Fixture.bReady)
	{
		return false;
	}
	TestTrue(TEXT("committed hit applies condition"),
		Fixture.Conditions->TryApplyMeridianShock(
			MakeCommittedReceipt(
				ConditionTargetId,
				ImpactA,
				ResolutionA),
			Fixture.Sample).IsSuccess());

	Fdemo_mapShanmenCombatConditionStatusSnapshot AtZero;
	Fdemo_mapShanmenCombatConditionStatusSnapshot AtZeroReplay;
	TestTrue(TEXT("active state captures deterministically at tick zero"),
		Fixture.Conditions->TryCaptureMeridianShockStatus(AtZero)
			&& Fixture.Conditions->TryCaptureMeridianShockStatus(AtZeroReplay)
			&& AtZero.Matches(AtZeroReplay)
			&& AtZero.IsActive()
			&& AtZero.GetObservedTick() == 0
			&& AtZero.GetExpiryTick() == 90
			&& AtZero.GetRemainingTicks() == 90
			&& AtZero.GetConditionRevision() == 1);

	TestTrue(TEXT("canonical timeline advances to tick 30"),
		Fixture.AdvanceTicks(30));
	TestTrue(TEXT("condition observes the same canonical sample"),
		Fixture.Conditions->TryAdvance(Fixture.Sample).IsSuccess());
	Fdemo_mapShanmenCombatConditionStatusSnapshot AtThirty;
	TestTrue(TEXT("countdown snapshot reflects timeline without condition mutation"),
		Fixture.Conditions->TryCaptureMeridianShockStatus(AtThirty)
			&& AtThirty.IsActive()
			&& AtThirty.GetObservedTick() == 30
			&& AtThirty.GetExpiryTick() == 90
			&& AtThirty.GetRemainingTicks() == 60
			&& AtThirty.GetConditionRevision() == 1
			&& !AtZero.Matches(AtThirty));
	TestTrue(TEXT("earlier copied snapshot remains immutable and self-validating"),
		AtZero.IsValid()
			&& AtZero.GetObservedTick() == 0
			&& AtZero.GetRemainingTicks() == 90);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapMeridianShockStatusReplayRefreshTest,
	"Shanmen.0_0_10.Product.CombatCondition.MeridianShock.Status.ReplayAndRefresh",
	ConditionFlags)

bool Fdemo_mapMeridianShockStatusReplayRefreshTest::RunTest(
	const FString&)
{
	FConditionFixture Fixture;
	TestTrue(TEXT("condition fixture begins"), Fixture.bReady);
	if (!Fixture.bReady)
	{
		return false;
	}
	const FShanmenVitalityCommitReceipt FirstReceipt = MakeCommittedReceipt(
		ConditionTargetId,
		ImpactA,
		ResolutionA);
	TestTrue(TEXT("first committed hit applies"),
		Fixture.Conditions->TryApplyMeridianShock(
			FirstReceipt,
			Fixture.Sample).IsSuccess());
	Fdemo_mapShanmenCombatConditionStatusSnapshot BeforeReplay;
	Fdemo_mapShanmenCombatConditionStatusSnapshot AfterReplay;
	TestTrue(TEXT("first active status captures"),
		Fixture.Conditions->TryCaptureMeridianShockStatus(BeforeReplay));
	TestTrue(TEXT("exact impact replay is accepted"),
		Fixture.Conditions->TryApplyMeridianShock(
			FirstReceipt,
			Fixture.Sample).Status
			== Edemo_mapShanmenCombatConditionApplicationStatus::AlreadyApplied);
	TestTrue(TEXT("exact replay preserves the same status identity"),
		Fixture.Conditions->TryCaptureMeridianShockStatus(AfterReplay)
			&& BeforeReplay.Matches(AfterReplay)
			&& AfterReplay.GetConditionRevision() == 1);

	TestTrue(TEXT("canonical timeline advances to tick 30"),
		Fixture.AdvanceTicks(30));
	TestTrue(TEXT("second committed hit refreshes through the authority"),
		Fixture.Conditions->TryApplyMeridianShock(
			MakeCommittedReceipt(
				ConditionTargetId,
				ImpactB,
				ResolutionB),
			Fixture.Sample).Status
			== Edemo_mapShanmenCombatConditionApplicationStatus::Refreshed);
	Fdemo_mapShanmenCombatConditionStatusSnapshot Refreshed;
	TestTrue(TEXT("refreshed status exposes new revision and exact deadline"),
		Fixture.Conditions->TryCaptureMeridianShockStatus(Refreshed)
			&& Refreshed.IsActive()
			&& Refreshed.GetObservedTick() == 30
			&& Refreshed.GetExpiryTick() == 120
			&& Refreshed.GetRemainingTicks() == 90
			&& Refreshed.GetConditionRevision() == 2
			&& !BeforeReplay.Matches(Refreshed)
			&& Fixture.Attributes->GetModifierCountBySource(
				Udemo_mapShanmenCombatConditionComponent::
					MeridianShockModifierSourceId()) == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapMeridianShockStatusExpiryTransitionTest,
	"Shanmen.0_0_10.Product.CombatCondition.MeridianShock.Status.ExpiryTransition",
	ConditionFlags)

bool Fdemo_mapMeridianShockStatusExpiryTransitionTest::RunTest(
	const FString&)
{
	FConditionFixture Fixture;
	TestTrue(TEXT("condition fixture begins"), Fixture.bReady);
	if (!Fixture.bReady)
	{
		return false;
	}
	TestTrue(TEXT("committed hit applies condition"),
		Fixture.Conditions->TryApplyMeridianShock(
			MakeCommittedReceipt(
				ConditionTargetId,
				ImpactA,
				ResolutionA),
			Fixture.Sample).IsSuccess());
	Fdemo_mapShanmenCombatConditionStatusSnapshot Active;
	TestTrue(TEXT("active status captures"),
		Fixture.Conditions->TryCaptureMeridianShockStatus(Active));

	TestTrue(TEXT("canonical timeline reaches exact expiry"),
		Fixture.AdvanceTicks(90));
	const Fdemo_mapShanmenCombatConditionAdvanceResult Expired =
		Fixture.Conditions->TryAdvance(Fixture.Sample);
	Fdemo_mapShanmenCombatConditionStatusSnapshot Inactive;
	TestTrue(TEXT("expiry transition captures as inactive revision two"),
		Expired.Status
				== Edemo_mapShanmenCombatConditionAdvanceStatus::Expired
			&& Fixture.Conditions->TryCaptureMeridianShockStatus(Inactive)
			&& !Inactive.IsActive()
			&& Inactive.GetObservedTick() == 90
			&& Inactive.GetExpiryTick() == INDEX_NONE
			&& Inactive.GetRemainingTicks() == 0
			&& Inactive.GetConditionRevision() == 2
			&& !Active.Matches(Inactive));
	TestTrue(TEXT("pre-expiry copy remains valid after authority transition"),
		Active.IsValid()
			&& Active.IsActive()
			&& Active.GetRemainingTicks() == 90);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapMeridianShockStatusRunTeardownTest,
	"Shanmen.0_0_10.Product.CombatCondition.MeridianShock.Status.RunTeardown",
	ConditionFlags)

bool Fdemo_mapMeridianShockStatusRunTeardownTest::RunTest(
	const FString&)
{
	FConditionFixture Fixture;
	TestTrue(TEXT("condition fixture begins"), Fixture.bReady);
	if (!Fixture.bReady)
	{
		return false;
	}
	TestTrue(TEXT("committed hit applies condition"),
		Fixture.Conditions->TryApplyMeridianShock(
			MakeCommittedReceipt(
				ConditionTargetId,
				ImpactA,
				ResolutionA),
			Fixture.Sample).IsSuccess());
	Fdemo_mapShanmenCombatConditionStatusSnapshot BeforeTeardown;
	TestTrue(TEXT("pre-teardown status captures"),
		Fixture.Conditions->TryCaptureMeridianShockStatus(BeforeTeardown));

	FString Diagnostic;
	TestTrue(TEXT("matching Run teardown succeeds"),
		Fixture.Conditions->TryEnd(ConditionRunId, Diagnostic));
	Fdemo_mapShanmenCombatConditionStatusSnapshot AfterTeardown =
		BeforeTeardown;
	TestFalse(TEXT("empty authority no longer exposes a status"),
		Fixture.Conditions->TryCaptureMeridianShockStatus(AfterTeardown));
	TestFalse(TEXT("failed capture clears caller-owned output"),
		AfterTeardown.IsValid());
	TestTrue(TEXT("copied pre-teardown status remains immutable evidence"),
		BeforeTeardown.IsValid()
			&& BeforeTeardown.GetRunId() == ConditionRunId);

	TestTrue(TEXT("component binds a later clean Run"),
		Fixture.Conditions->TryBegin(
			ForeignRunId,
			ConditionTargetId,
			Fdemo_mapShanmenCombatRunFixedTimeline::MakeTimelineId(
				ForeignRunId),
			Fixture.Attributes,
			Diagnostic));
	Fdemo_mapShanmenCombatConditionStatusSnapshot NextRun;
	TestTrue(TEXT("later Run exposes a distinct canonical inactive status"),
		Fixture.Conditions->TryCaptureMeridianShockStatus(NextRun)
			&& NextRun.IsValid()
			&& !NextRun.IsActive()
			&& NextRun.GetRunId() == ForeignRunId
			&& NextRun.GetConditionRevision() == 0
			&& !BeforeTeardown.Matches(NextRun));
	return true;
}

#endif
