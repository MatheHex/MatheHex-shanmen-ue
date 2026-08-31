#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "demo_mapCombatRunCoordinator.h"
#include "demo_mapPlayerHealthComponent.h"
#include "demo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchPlan.h"

namespace
{
	constexpr EAutomationTestFlags SwordRhythmCueProductDispatchPlanFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;
	const FGuid TestRun(
		0xB1000001, 0xB1000002, 0xB1000003, 0xB1000004);
	const FGuid TestWeapon(
		0xB1010001, 0xB1010002, 0xB1010003, 0xB1010004);

	using FSeed =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchPlanSeed;
	using FIdentity =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionCapture;

	FSeed MakeSeed(const uint32 Value)
	{
		FSeed Seed;
		Seed.DispatchSeed = FGuid(Value + 1, 0, 0, 1);
		Seed.VisualConsumerScopeId = FGuid(Value + 2, 0, 0, 1);
		Seed.AudioConsumerScopeId = FGuid(Value + 3, 0, 0, 1);
		return Seed;
	}

	TArray<FGuid> IdentityValues(const FIdentity& Identity)
	{
		return {
			Identity.HostId,
			Identity.CreateCommandId,
			Identity.ProcessCommandId,
			Identity.EndCommandId,
			Identity.VisualConsumerId,
			Identity.AudioConsumerId,
			Identity.VisualAttemptId,
			Identity.AudioAttemptId
		};
	}

	bool AllDistinct(const FIdentity& Identity)
	{
		const TArray<FGuid> Values = IdentityValues(Identity);
		for (int32 Left = 0; Left < Values.Num(); ++Left)
		{
			if (!Values[Left].IsValid())
			{
				return false;
			}
			for (int32 Right = Left + 1; Right < Values.Num(); ++Right)
			{
				if (Values[Left] == Values[Right])
				{
					return false;
				}
			}
		}
		return true;
	}

	bool AllDifferent(const FIdentity& Left, const FIdentity& Right)
	{
		const TArray<FGuid> LeftValues = IdentityValues(Left);
		const TArray<FGuid> RightValues = IdentityValues(Right);
		if (LeftValues.Num() != RightValues.Num())
		{
			return false;
		}
		for (int32 Index = 0; Index < LeftValues.Num(); ++Index)
		{
			if (LeftValues[Index] == RightValues[Index])
			{
				return false;
			}
		}
		return true;
	}

	struct FSwordRhythmCueProductDispatchPlanFixture
	{
		APawn* Pawn = nullptr;
		UBoxComponent* CollisionRoot = nullptr;
		Udemo_mapPlayerHealthComponent* Health = nullptr;
		Fdemo_mapCombatRunCoordinator Coordinator;
		Fdemo_mapShanmenCombatRunFixedTimeline Timeline;
		Fdemo_mapShanmenSwordRhythmProductSession Session;
		FString Diagnostic;
		bool bReady = false;

		FSwordRhythmCueProductDispatchPlanFixture()
		{
			Pawn = NewObject<APawn>(GetTransientPackage());
			CollisionRoot = Pawn ? NewObject<UBoxComponent>(Pawn) : nullptr;
			Health = Pawn
				? NewObject<Udemo_mapPlayerHealthComponent>(Pawn)
				: nullptr;
			if (Pawn && CollisionRoot)
			{
				Pawn->SetRootComponent(CollisionRoot);
			}
			bReady = Pawn && CollisionRoot && Health
				&& Coordinator.TryBeginRun(
					TestRun, Pawn, Health, Diagnostic)
				&& Timeline.TryBegin(TestRun, Diagnostic)
				&& Session.TryBegin(TestRun, Diagnostic);
		}

		bool TryAdvanceTicks(const int64 RequestedTicks)
		{
			int64 AdvancedTicks = 0;
			const double Seconds = static_cast<double>(RequestedTicks)
				/ static_cast<double>(
					Fdemo_mapShanmenCombatRunFixedTimeline::
						CanonicalTicksPerSecond());
			return Timeline.TryAdvance(Seconds, AdvancedTicks, Diagnostic)
				&& AdvancedTicks == RequestedTicks;
		}

		bool TryExecute()
		{
			const auto Action = Coordinator.ExecutePlayerBasicSwordSweep(
				TestWeapon, 1.0f, {});
			Fdemo_mapShanmenCombatRunTimelineSample Sample;
			FShanmenSwordRhythmReceipt Receipt;
			return Action.IsExecuted()
				&& Timeline.TryCapture(Sample)
				&& Session.TryObserveExecutedBasicSword(
					Action, Sample, Receipt, Diagnostic);
		}

		bool TryPrepareCurrentEffect()
		{
			return bReady
				&& TryExecute()
				&& TryAdvanceTicks(
					Fdemo_mapShanmenSwordRhythmProductConfig::
						CanonicalLinkOpenOffsetTicks())
				&& TryExecute()
				&& TryExecute();
		}

		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductProjectionResult
		CaptureCurrent() const
		{
			return
				Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductProjector::
					CaptureCurrent(Session);
		}
	};

	class FProductDispatchPlanFakeExecutor final
		: public Idemo_mapShanmenSwordRhythmEffectCueExecutor
	{
	public:
		explicit FProductDispatchPlanFakeExecutor(const uint32 InReceiptSeed)
			: ReceiptSeed(InReceiptSeed)
		{
		}

		int32 InvocationCount = 0;

		virtual Fdemo_mapShanmenSwordRhythmEffectCueExecutorResult Execute(
			const Fdemo_mapShanmenSwordRhythmEffectCueExecutorInvocation&
				Invocation) override
		{
			++InvocationCount;
			Fdemo_mapShanmenSwordRhythmEffectCueExecutorResult Result;
			if (!Fdemo_mapShanmenSwordRhythmEffectCueExecutorReceipt::TryCreate(
					Invocation,
					FGuid(ReceiptSeed + InvocationCount, 0, 0, 1),
					Edemo_mapShanmenSwordRhythmEffectCueAttemptOutcome::Succeeded,
					Result.Receipt))
			{
				Result.Status =
					Edemo_mapShanmenSwordRhythmEffectCueExecutorStatus::Rejected;
				Result.Diagnostic = TEXT(
					"Dispatch-plan fake receipt capture failed.");
				return Result;
			}
			Result.Status =
				Edemo_mapShanmenSwordRhythmEffectCueExecutorStatus::Completed;
			Result.Diagnostic = TEXT(
				"Dispatch-plan fake returned opaque evidence.");
			return Result;
		}

	private:
		uint32 ReceiptSeed = 0;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueProductDispatchPlanDeterminismTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductDispatchPlan.DeterministicCapture",
	SwordRhythmCueProductDispatchPlanFlags)

bool Fdemo_mapSwordRhythmCueProductDispatchPlanDeterminismTest::RunTest(
	const FString&)
{
	FSwordRhythmCueProductDispatchPlanFixture Fixture;
	TestTrue(TEXT("effect projection is prepared"),
		Fixture.TryPrepareCurrentEffect());
	const auto Projection = Fixture.CaptureCurrent();
	const FSeed Seed = MakeSeed(0xB1100000);
	const auto First =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchPlanFactory::
			Capture(Projection.Projection, Seed);
	const auto Second =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchPlanFactory::
			Capture(Projection.Projection, Seed);
	const auto FirstRequest =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionFactory::
			Capture(
				Projection.Projection,
				First.Plan.GetTransactionIdentity());
	const auto SecondRequest =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionFactory::
			Capture(
				Projection.Projection,
				Second.Plan.GetTransactionIdentity());
	TestTrue(TEXT("same inputs reproduce one role-isolated valid plan"),
		Projection.IsCaptured()
			&& First.IsCaptured()
			&& Second.IsCaptured()
			&& First.Plan.Matches(Second.Plan)
			&& First.Plan.MatchesProjection(Projection.Projection)
			&& First.Plan.MatchesCurrentSession(Fixture.Session)
			&& AllDistinct(First.Plan.GetTransactionIdentity())
			&& FirstRequest.IsCaptured()
			&& SecondRequest.IsCaptured()
			&& FirstRequest.Request.Matches(SecondRequest.Request));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueProductDispatchPlanDomainTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductDispatchPlan.SeedAndScopeDomainSeparation",
	SwordRhythmCueProductDispatchPlanFlags)

bool Fdemo_mapSwordRhythmCueProductDispatchPlanDomainTest::RunTest(
	const FString&)
{
	FSwordRhythmCueProductDispatchPlanFixture Fixture;
	TestTrue(TEXT("effect projection is prepared"),
		Fixture.TryPrepareCurrentEffect());
	const auto Projection = Fixture.CaptureCurrent();
	const FSeed BaseSeed = MakeSeed(0xB1200000);
	FSeed OtherDispatch = BaseSeed;
	OtherDispatch.DispatchSeed = FGuid(0xB1200010, 0, 0, 1);
	FSeed OtherScope = BaseSeed;
	OtherScope.VisualConsumerScopeId = FGuid(0xB1200020, 0, 0, 1);
	const auto Base =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchPlanFactory::
			Capture(Projection.Projection, BaseSeed);
	const auto DispatchChanged =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchPlanFactory::
			Capture(Projection.Projection, OtherDispatch);
	const auto ScopeChanged =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchPlanFactory::
			Capture(Projection.Projection, OtherScope);
	TestTrue(TEXT("seed and consumer scope changes separate every role"),
		Projection.IsCaptured()
			&& Base.IsCaptured()
			&& DispatchChanged.IsCaptured()
			&& ScopeChanged.IsCaptured()
			&& AllDifferent(
				Base.Plan.GetTransactionIdentity(),
				DispatchChanged.Plan.GetTransactionIdentity())
			&& AllDifferent(
				Base.Plan.GetTransactionIdentity(),
				ScopeChanged.Plan.GetTransactionIdentity()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueProductDispatchPlanProjectionTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductDispatchPlan.ProjectionRevisionSeparation",
	SwordRhythmCueProductDispatchPlanFlags)

bool Fdemo_mapSwordRhythmCueProductDispatchPlanProjectionTest::RunTest(
	const FString&)
{
	FSwordRhythmCueProductDispatchPlanFixture Fixture;
	TestTrue(TEXT("first effect projection is prepared"),
		Fixture.TryPrepareCurrentEffect());
	const FSeed Seed = MakeSeed(0xB1300000);
	const auto FirstProjection = Fixture.CaptureCurrent();
	const auto First =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchPlanFactory::
			Capture(FirstProjection.Projection, Seed);
	TestTrue(TEXT("source advances to a new presentation revision"),
		Fixture.TryAdvanceTicks(1) && Fixture.TryExecute());
	const auto SecondProjection = Fixture.CaptureCurrent();
	const auto Second =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchPlanFactory::
			Capture(SecondProjection.Projection, Seed);
	TestTrue(TEXT("projection identity changes separate every derived role"),
		FirstProjection.IsCaptured()
			&& SecondProjection.IsCaptured()
			&& First.IsCaptured()
			&& Second.IsCaptured()
			&& !First.Plan.MatchesCurrentSession(Fixture.Session)
			&& Second.Plan.MatchesCurrentSession(Fixture.Session)
			&& !First.Plan.Matches(Second.Plan)
			&& AllDifferent(
				First.Plan.GetTransactionIdentity(),
				Second.Plan.GetTransactionIdentity()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueProductDispatchPlanReplayTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductDispatchPlan.FeedsCurrentDispatchReplay",
	SwordRhythmCueProductDispatchPlanFlags)

bool Fdemo_mapSwordRhythmCueProductDispatchPlanReplayTest::RunTest(
	const FString&)
{
	FSwordRhythmCueProductDispatchPlanFixture Fixture;
	TestTrue(TEXT("effect projection is prepared"),
		Fixture.TryPrepareCurrentEffect());
	const auto Projection = Fixture.CaptureCurrent();
	const auto Planned =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchPlanFactory::
			Capture(Projection.Projection, MakeSeed(0xB1400000));
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost Host;
	FProductDispatchPlanFakeExecutor Visual(0xB1410000);
	FProductDispatchPlanFakeExecutor Audio(0xB1420000);
	const auto First =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatch::
			TryDispatchCurrent(
				Fixture.Session,
				Planned.Plan.GetTransactionIdentity(),
				Host,
				Visual,
				Audio);
	const int32 VisualBeforeReplay = Visual.InvocationCount;
	const int32 AudioBeforeReplay = Audio.InvocationCount;
	const auto Replay =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatch::
			TryDispatchCurrent(
				Fixture.Session,
				Planned.Plan.GetTransactionIdentity(),
				Host,
				Visual,
				Audio);
	TestTrue(TEXT("deterministic plan feeds one dispatch and exact replay"),
		Projection.IsCaptured()
			&& Planned.IsCaptured()
			&& First.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchStatus::
					Dispatched
			&& First.IsSuccess()
			&& Replay.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchStatus::
					Replayed
			&& Replay.IsSuccess()
			&& Host.GetRecordCount() == 3
			&& Visual.InvocationCount == VisualBeforeReplay
			&& Audio.InvocationCount == AudioBeforeReplay);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueProductDispatchPlanFenceTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductDispatchPlan.InvalidAndForeignPlanFence",
	SwordRhythmCueProductDispatchPlanFlags)

bool Fdemo_mapSwordRhythmCueProductDispatchPlanFenceTest::RunTest(
	const FString&)
{
	FSwordRhythmCueProductDispatchPlanFixture Fixture;
	FSeed InvalidSeed = MakeSeed(0xB1500000);
	InvalidSeed.AudioConsumerScopeId = InvalidSeed.VisualConsumerScopeId;
	const auto InvalidProjection =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchPlanFactory::
			Capture(
				Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductProjection(),
				MakeSeed(0xB1500010));
	TestTrue(TEXT("invalid projection and aliased scopes fail closed"),
		!Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchPlanFactory::
			Capture(
				Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductProjection(),
				InvalidSeed)
			.IsCaptured()
			&& InvalidProjection.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchPlanCaptureStatus::
					ProjectionInvalid);

	TestTrue(TEXT("effect projection is prepared"),
		Fixture.TryPrepareCurrentEffect());
	const auto Projection = Fixture.CaptureCurrent();
	const auto FirstPlan =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchPlanFactory::
			Capture(Projection.Projection, MakeSeed(0xB1500100));
	const auto ForeignPlan =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchPlanFactory::
			Capture(Projection.Projection, MakeSeed(0xB1500200));
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost Host;
	FProductDispatchPlanFakeExecutor Visual(0xB1510000);
	FProductDispatchPlanFakeExecutor Audio(0xB1520000);
	const auto First =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatch::
			TryDispatchCurrent(
				Fixture.Session,
				FirstPlan.Plan.GetTransactionIdentity(),
				Host,
				Visual,
				Audio);
	const int32 VisualBeforeForeign = Visual.InvocationCount;
	const int32 AudioBeforeForeign = Audio.InvocationCount;
	const auto Foreign =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatch::
			TryDispatchCurrent(
				Fixture.Session,
				ForeignPlan.Plan.GetTransactionIdentity(),
				Host,
				Visual,
				Audio);
	TestTrue(TEXT("foreign deterministic plan cannot alias a bound Host"),
		Projection.IsCaptured()
			&& FirstPlan.IsCaptured()
			&& ForeignPlan.IsCaptured()
			&& First.IsSuccess()
			&& Foreign.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchStatus::
					TransactionIncomplete
			&& Foreign.Transaction.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionStatus::
					CreateRejected
			&& Host.GetRecordCount() == 3
			&& Visual.InvocationCount == VisualBeforeForeign
			&& Audio.InvocationCount == AudioBeforeForeign);
	return true;
}

#endif
