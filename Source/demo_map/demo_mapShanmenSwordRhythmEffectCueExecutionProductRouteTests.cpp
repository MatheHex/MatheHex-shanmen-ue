#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Algo/Reverse.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "demo_mapCombatRunCoordinator.h"
#include "demo_mapPlayerHealthComponent.h"
#include "demo_mapShanmenSwordRhythmEffectCueExecutionProductRoute.h"

namespace
{
	constexpr EAutomationTestFlags SwordRhythmCueProductRouteFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;
	const FGuid TestRun(
		0xAE000001, 0xAE000002, 0xAE000003, 0xAE000004);
	const FGuid ForeignRun(
		0xAE000011, 0xAE000012, 0xAE000013, 0xAE000014);
	const FGuid TestHost(
		0xAE010001, 0xAE010002, 0xAE010003, 0xAE010004);
	const FGuid ForeignHost(
		0xAE010011, 0xAE010012, 0xAE010013, 0xAE010014);
	const FGuid TestWeapon(
		0xAE020001, 0xAE020002, 0xAE020003, 0xAE020004);
	const FGuid VisualConsumer(
		0xAE030001, 0xAE030002, 0xAE030003, 0xAE030004);
	const FGuid AudioConsumer(
		0xAE040001, 0xAE040002, 0xAE040003, 0xAE040004);

	using FProjection =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductProjection;

	struct FSwordRhythmCueProductRouteFixture
	{
		APawn* Pawn = nullptr;
		UBoxComponent* CollisionRoot = nullptr;
		Udemo_mapPlayerHealthComponent* Health = nullptr;
		Fdemo_mapCombatRunCoordinator Coordinator;
		Fdemo_mapShanmenCombatRunFixedTimeline Timeline;
		Fdemo_mapShanmenSwordRhythmProductSession Session;
		FString Diagnostic;
		bool bReady = false;

		explicit FSwordRhythmCueProductRouteFixture(
			const FGuid& RunId = TestRun)
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
					RunId, Pawn, Health, Diagnostic)
				&& Timeline.TryBegin(RunId, Diagnostic)
				&& Session.TryBegin(RunId, Diagnostic);
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

		bool TryExecuteProjection(FProjection& OutProjection)
		{
			const auto Action = Coordinator.ExecutePlayerBasicSwordSweep(
				TestWeapon, 1.0f, {});
			Fdemo_mapShanmenCombatRunTimelineSample Sample;
			FShanmenSwordRhythmReceipt Receipt;
			if (!Action.IsExecuted()
				|| !Timeline.TryCapture(Sample)
				|| !Session.TryObserveExecutedBasicSword(
					Action, Sample, Receipt, Diagnostic))
			{
				return false;
			}
			const auto Captured =
				Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductProjector::
					CaptureCurrent(Session);
			if (!Captured.IsCaptured())
			{
				Diagnostic = Captured.Diagnostic;
				return false;
			}
			OutProjection = Captured.Projection;
			return true;
		}

		bool TryMakeOrderedBatch(TArray<FProjection>& OutProjections)
		{
			OutProjections.Reset();
			FProjection First;
			FProjection Second;
			FProjection Third;
			if (!TryExecuteProjection(First)
				|| !TryAdvanceTicks(
					Fdemo_mapShanmenSwordRhythmProductConfig::
						CanonicalLinkOpenOffsetTicks())
				|| !TryExecuteProjection(Second)
				|| !TryExecuteProjection(Third))
			{
				return false;
			}
			OutProjections = {First, Second, Third};
			return First.GetObservationRevision()
					< Second.GetObservationRevision()
				&& Second.GetObservationRevision()
					< Third.GetObservationRevision();
		}

		bool TryMakeEffectBatch(TArray<FProjection>& OutProjections)
		{
			OutProjections.Reset();
			FProjection Discarded;
			FProjection First;
			FProjection Second;
			if (!TryExecuteProjection(Discarded)
				|| !TryAdvanceTicks(
					Fdemo_mapShanmenSwordRhythmProductConfig::
						CanonicalLinkOpenOffsetTicks())
				|| !TryExecuteProjection(Discarded)
				|| !TryExecuteProjection(First)
				|| !TryAdvanceTicks(1)
				|| !TryExecuteProjection(Second))
			{
				return false;
			}
			OutProjections = {First, Second};
			return First.GetEvent().NumCommands() == 2
				&& First.GetObservationRevision()
					< Second.GetObservationRevision();
		}
	};

	class FProductRouteFakeExecutor final
		: public Idemo_mapShanmenSwordRhythmEffectCueExecutor
	{
	public:
		explicit FProductRouteFakeExecutor(const uint32 InReceiptSeed)
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
					"Product-route fake receipt capture failed.");
				return Result;
			}
			Result.Status =
				Edemo_mapShanmenSwordRhythmEffectCueExecutorStatus::Completed;
			Result.Diagnostic = TEXT(
				"Product-route fake returned opaque evidence.");
			return Result;
		}

	private:
		uint32 ReceiptSeed = 0;
	};

	bool TryCaptureProcess(
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost& Host,
		const int64 Sequence,
		const FGuid& CommandId,
		const FGuid& VisualAttemptId,
		const FGuid& AudioAttemptId,
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope&
			OutEnvelope)
	{
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand Command;
		return Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand::
				TryCaptureProcessNext(
					CommandId,
					Host.GetRunId(),
					Host.GetBatchId(),
					VisualAttemptId,
					AudioAttemptId,
					Command)
			&& Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope::
				TryCapture(
					Host.GetHostId(), Sequence, Command, OutEnvelope);
	}

	bool TryCaptureEnd(
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost& Host,
		const int64 Sequence,
		const FGuid& CommandId,
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope&
			OutEnvelope)
	{
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand Command;
		return Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand::
				TryCaptureEnd(
					CommandId, Host.GetRunId(), Host.GetBatchId(), Command)
			&& Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope::
				TryCapture(
					Host.GetHostId(), Sequence, Command, OutEnvelope);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueProductRouteProjectionTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductRoute.Projection",
	SwordRhythmCueProductRouteFlags)

bool Fdemo_mapSwordRhythmCueProductRouteProjectionTest::RunTest(
	const FString&)
{
	FSwordRhythmCueProductRouteFixture Fixture;
	TestTrue(TEXT("fixture begins without a projectable presentation"),
		Fixture.bReady
			&& !Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductProjector::
				CaptureCurrent(Fixture.Session).IsCaptured());
	FProjection First;
	TestTrue(TEXT("current ProductSession state captures once observed"),
		Fixture.TryExecuteProjection(First)
			&& First.IsValid()
			&& First.MatchesCurrentSession(Fixture.Session)
			&& First.GetRunId() == TestRun
			&& First.GetConfigId()
				== Fixture.Session.GetConfig().GetConfigId()
			&& First.GetCuePolicyId()
				== Fixture.Session.GetConfig().GetEffectCuePolicy().GetPolicyId());
	const auto Repeated =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductProjector::
			CaptureCurrent(Fixture.Session);
	TestTrue(TEXT("repeated projection is deterministic"),
		Repeated.IsCaptured() && First.Matches(Repeated.Projection));

	FProjection Second;
	TestTrue(TEXT("old projection stays valid while Session advances"),
		Fixture.TryAdvanceTicks(1)
			&& Fixture.TryExecuteProjection(Second)
			&& First.IsValid()
			&& Second.IsValid()
			&& First.GetObservationRevision()
				< Second.GetObservationRevision()
			&& !First.MatchesCurrentSession(Fixture.Session)
			&& Second.MatchesCurrentSession(Fixture.Session));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueProductRouteCreateReplayTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductRoute.CreateReplay",
	SwordRhythmCueProductRouteFlags)

bool Fdemo_mapSwordRhythmCueProductRouteCreateReplayTest::RunTest(
	const FString&)
{
	FSwordRhythmCueProductRouteFixture Fixture;
	TArray<FProjection> Projections;
	TestTrue(TEXT("ordered Product projection batch is ready"),
		Fixture.bReady && Fixture.TryMakeOrderedBatch(Projections));
	const FGuid CommandId(0xAE100001, 0, 0, 1);
	const auto Captured =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductCreateFactory::
			Capture(
				TestHost,
				0,
				CommandId,
				Projections,
				VisualConsumer,
				AudioConsumer);
	const auto Repeated =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductCreateFactory::
			Capture(
				TestHost,
				0,
				CommandId,
				Projections,
				VisualConsumer,
				AudioConsumer);
	TestTrue(TEXT("frozen Create request is deterministic"),
		Captured.IsCaptured()
			&& Repeated.IsCaptured()
			&& Captured.Request.Matches(Repeated.Request));

	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost Host;
	const auto Created =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRoute::TryCreate(
			Captured.Request, Host);
	FProjection Advanced;
	TestTrue(TEXT("source ProductSession may advance after request capture"),
		Fixture.TryAdvanceTicks(1)
			&& Fixture.TryExecuteProjection(Advanced)
			&& Advanced.GetObservationRevision()
				> Projections.Last().GetObservationRevision());
	const auto Replayed =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRoute::TryCreate(
			Captured.Request, Host);
	TestTrue(TEXT("Product Create delegates durable replay to one Host"),
		Created.IsSuccess()
			&& !Created.bReplay
			&& Replayed.IsSuccess()
			&& Replayed.bReplay
			&& Host.IsValid()
			&& Host.GetRecordCount() == 1
			&& Host.GetNextSequence() == 1
			&& Host.GetRouter().NumEvents() == Projections.Num());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueProductRouteCaptureFenceTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductRoute.CaptureFence",
	SwordRhythmCueProductRouteFlags)

bool Fdemo_mapSwordRhythmCueProductRouteCaptureFenceTest::RunTest(
	const FString&)
{
	FSwordRhythmCueProductRouteFixture Fixture;
	FSwordRhythmCueProductRouteFixture ForeignFixture(ForeignRun);
	TArray<FProjection> Ordered;
	FProjection Foreign;
	TestTrue(TEXT("local and foreign projections are ready"),
		Fixture.bReady
			&& ForeignFixture.bReady
			&& Fixture.TryMakeOrderedBatch(Ordered)
			&& ForeignFixture.TryExecuteProjection(Foreign));

	const FGuid CommandId(0xAE100011, 0, 0, 1);
	const TArray<FProjection> Empty;
	const auto EmptyResult =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductCreateFactory::
			Capture(
				TestHost,
				0,
				CommandId,
				Empty,
				VisualConsumer,
				AudioConsumer);
	TArray<FProjection> Reversed = Ordered;
	Algo::Reverse(Reversed);
	const auto ReversedResult =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductCreateFactory::
			Capture(
				TestHost,
				0,
				CommandId,
				Reversed,
				VisualConsumer,
				AudioConsumer);
	const TArray<FProjection> Mixed = {Ordered[0], Foreign};
	const auto MixedResult =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductCreateFactory::
			Capture(
				TestHost,
				0,
				CommandId,
				Mixed,
				VisualConsumer,
				AudioConsumer);
	const TArray<FProjection> Duplicate = {Ordered[0], Ordered[0]};
	const auto DuplicateResult =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductCreateFactory::
			Capture(
				TestHost,
				0,
				CommandId,
				Duplicate,
				VisualConsumer,
				AudioConsumer);
	const auto SequenceResult =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductCreateFactory::
			Capture(
				TestHost,
				1,
				CommandId,
				Ordered,
				VisualConsumer,
				AudioConsumer);
	const auto ConsumerAliasResult =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductCreateFactory::
			Capture(
				TestHost,
				0,
				CommandId,
				Ordered,
				VisualConsumer,
				VisualConsumer);
	TestTrue(TEXT("capture fences invalid, mixed and unordered batches"),
		EmptyResult.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductCreateCaptureStatus::
					ProjectionInvalid
			&& ReversedResult.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductCreateCaptureStatus::
					ProjectionOrderInvalid
			&& MixedResult.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductCreateCaptureStatus::
					ProjectionIdentityMismatch
			&& DuplicateResult.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductCreateCaptureStatus::
					ProjectionOrderInvalid
			&& SequenceResult.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductCreateCaptureStatus::
					TransportIdentityInvalid
			&& ConsumerAliasResult.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductCreateCaptureStatus::
					TransportIdentityInvalid);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueProductRouteHostFenceTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductRoute.HostFence",
	SwordRhythmCueProductRouteFlags)

bool Fdemo_mapSwordRhythmCueProductRouteHostFenceTest::RunTest(
	const FString&)
{
	FSwordRhythmCueProductRouteFixture Fixture;
	TArray<FProjection> Projections;
	TestTrue(TEXT("one Product projection is ready"),
		Fixture.bReady && Fixture.TryMakeOrderedBatch(Projections));
	Projections.SetNum(1);
	const auto Primary =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductCreateFactory::
			Capture(
				TestHost,
				0,
				FGuid(0xAE100021, 0, 0, 1),
				Projections,
				VisualConsumer,
				AudioConsumer);
	const auto Foreign =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductCreateFactory::
			Capture(
				ForeignHost,
				0,
				FGuid(0xAE100022, 0, 0, 1),
				Projections,
				VisualConsumer,
				AudioConsumer);
	const auto Conflict =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductCreateFactory::
			Capture(
				TestHost,
				0,
				FGuid(0xAE100023, 0, 0, 1),
				Projections,
				VisualConsumer,
				AudioConsumer);
	TestTrue(TEXT("all three structurally valid Create requests capture"),
		Primary.IsCaptured()
			&& Foreign.IsCaptured()
			&& Conflict.IsCaptured());

	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost Host;
	const auto Created =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRoute::TryCreate(
			Primary.Request, Host);
	const auto ForeignRejected =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRoute::TryCreate(
			Foreign.Request, Host);
	const auto ConflictRejected =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRoute::TryCreate(
			Conflict.Request, Host);
	const auto Replayed =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRoute::TryCreate(
			Primary.Request, Host);
	TestTrue(TEXT("historical sequence fence precedes envelope identity"),
		Created.IsSuccess()
			&& ForeignRejected.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductRouteStatus::
					HostRejected
			&& ForeignRejected.Host.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandHostStatus::
					SequenceConflict
			&& ConflictRejected.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductRouteStatus::
					HostRejected
			&& ConflictRejected.Host.Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionCommandHostStatus::
					SequenceConflict
			&& Replayed.IsSuccess()
			&& Replayed.bReplay
			&& Host.GetRecordCount() == 1
			&& Host.GetNextSequence() == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSwordRhythmCueProductRouteCompatibilityTest,
	"Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductRoute.FullCompatibility",
	SwordRhythmCueProductRouteFlags)

bool Fdemo_mapSwordRhythmCueProductRouteCompatibilityTest::RunTest(
	const FString&)
{
	FSwordRhythmCueProductRouteFixture Fixture;
	TArray<FProjection> Projections;
	const auto Captured = [&Fixture, &Projections]()
	{
		if (!Fixture.bReady || !Fixture.TryMakeEffectBatch(Projections))
		{
			return Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductCreateCaptureResult();
		}
		return Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductCreateFactory::
			Capture(
				TestHost,
				0,
				FGuid(0xAE100031, 0, 0, 1),
				Projections,
				VisualConsumer,
				AudioConsumer);
	}();
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost Host;
	TestTrue(TEXT("Product projection batch establishes P12.19 Host"),
		Captured.IsCaptured()
			&& Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRoute::
				TryCreate(Captured.Request, Host).IsSuccess());

	FProductRouteFakeExecutor VisualExecutor(0xAE200000);
	FProductRouteFakeExecutor AudioExecutor(0xAE300000);
	for (int32 Index = 0; Index < Projections.Num(); ++Index)
	{
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope Process;
		TestTrue(TEXT("caller captures one explicit ProcessNext envelope"),
			TryCaptureProcess(
				Host,
				Index + 1,
				FGuid(0xAE110000 + Index, 0, 0, 1),
				FGuid(0xAE210000 + Index, 0, 0, 1),
				FGuid(0xAE310000 + Index, 0, 0, 1),
				Process));
		TestTrue(TEXT("existing Host processes one Product event"),
			Host.TryRouteProcessNext(
				Process, VisualExecutor, AudioExecutor).IsSuccess()
				&& Host.GetRouter().NumCompletedEvents() == Index + 1);
	}

	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope End;
	const bool bEnded = TryCaptureEnd(
		Host,
		Projections.Num() + 1,
		FGuid(0xAE120001, 0, 0, 1),
		End)
		&& Host.TryRoute(End).IsSuccess();
	const int32 RecordsAfterEnd = Host.GetRecordCount();
	const auto HistoricalCreate =
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRoute::TryCreate(
			Captured.Request, Host);
	TestTrue(TEXT("caller-owned lifecycle ends and keeps Create replayable"),
		bEnded
			&& Host.IsTerminal()
			&& HistoricalCreate.IsSuccess()
			&& HistoricalCreate.bReplay
			&& Host.GetRecordCount() == RecordsAfterEnd
			&& RecordsAfterEnd == Projections.Num() + 2
			&& VisualExecutor.InvocationCount > 0
			&& AudioExecutor.InvocationCount > 0);
	return true;
}

#endif
