#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "demo_mapCombatRunCoordinator.h"
#include "demo_mapItemAuthority.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapPlayerHealthComponent.h"
#include "demo_mapShanmenWeaponGuardInputAdapter.h"

namespace
{
	const EAutomationTestFlags InputAdapterFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;
	const FGuid InputAdapterRun(
		0xDFB00001, 0xDFB00002, 0xDFB00003, 0xDFB00004);
	const FGuid InputAdapterTimeline(
		0xDFB10001, 0xDFB10002, 0xDFB10003, 0xDFB10004);

	Fdemo_mapShanmenWeaponGuardInputTimelineSample MakeTimeline(
		int64 ActiveStartTick = 10)
	{
		Fdemo_mapShanmenWeaponGuardInputTimelineSample Sample;
		const bool bCaptured =
			Fdemo_mapShanmenWeaponGuardInputTimelineSample::TryCapture(
				InputAdapterTimeline,
				ActiveStartTick,
				Sample);
		check(bCaptured);
		return Sample;
	}

	struct FInputAdapterFixture
	{
		APawn* Pawn = nullptr;
		UBoxComponent* Root = nullptr;
		Udemo_mapPlayerHealthComponent* Health = nullptr;
		Fdemo_mapCombatRunCoordinator Coordinator;
		Fdemo_mapItemAuthority Items;
		FString Diagnostic;
		bool bReady = false;

		FInputAdapterFixture()
		{
			Pawn = NewObject<APawn>(GetTransientPackage());
			Root = Pawn
				? NewObject<UBoxComponent>(
					Pawn,
					TEXT("WeaponGuardInputAdapterRoot"))
				: nullptr;
			Health = Pawn
				? NewObject<Udemo_mapPlayerHealthComponent>(
					Pawn,
					TEXT("WeaponGuardInputAdapterHealth"))
				: nullptr;
			if (Pawn && Root)
			{
				Pawn->SetRootComponent(Root);
			}
			bReady = Pawn && Root && Health
				&& Coordinator.TryBeginRun(
					InputAdapterRun,
					Pawn,
					Health,
					Diagnostic);
		}

		~FInputAdapterFixture()
		{
			Coordinator.Reset();
		}

		FGuid EquipWeapon()
		{
			TArray<FGuid> Affected;
			const Fdemo_mapItemOperationResult Added = Items.AddDefinition(
				Fdemo_mapItemIds::TrainingBlade,
				1,
				&Affected);
			if (!Added.bSuccess || Affected.Num() != 1)
			{
				return FGuid();
			}
			return Items.Equip(
					Affected[0],
					Fdemo_mapItemIds::WeaponSlot).bSuccess
				? Affected[0]
				: FGuid();
		}

		Fdemo_mapShanmenWeaponGuardInputResult Route(
			bool bGameplayAllowed,
			bool bRouteAvailable,
			const Fdemo_mapShanmenWeaponGuardInputTimelineSample& Sample,
			int32& TimelineSamples,
			int32& RouteCalls)
		{
			return Fdemo_mapShanmenWeaponGuardInputAdapter::RouteStartInput(
				bGameplayAllowed,
				bRouteAvailable,
				[&Sample, &TimelineSamples]()
				{
					++TimelineSamples;
					return Sample;
				},
				[this, &RouteCalls](
					const FGuid& TimelineId,
					int64 ActiveStartTick)
				{
					++RouteCalls;
					return Fdemo_mapShanmenWeaponGuardProductRoute::TryStart(
						Items,
						Coordinator,
						TimelineId,
						ActiveStartTick);
				});
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapWeaponGuardInputAdapterGateTest,
	"Shanmen.0_0_10.Product.WeaponGuardInputAdapter.Gates",
	InputAdapterFlags)

bool Fdemo_mapWeaponGuardInputAdapterGateTest::RunTest(const FString&)
{
	FInputAdapterFixture Fixture;
	const auto Sample = MakeTimeline();
	int32 TimelineSamples = 0;
	int32 RouteCalls = 0;
	const auto Blocked = Fixture.Route(
		false, true, Sample, TimelineSamples, RouteCalls);
	const auto MissingRoute = Fixture.Route(
		true, false, Sample, TimelineSamples, RouteCalls);
	TestTrue(TEXT("fixture Run is ready"), Fixture.bReady);
	TestEqual(TEXT("gameplay gate classifies blocked input"),
		Blocked.Status,
		Edemo_mapShanmenWeaponGuardInputStatus::GameplayBlocked);
	TestEqual(TEXT("route gate classifies unavailable product seam"),
		MissingRoute.Status,
		Edemo_mapShanmenWeaponGuardInputStatus::ProductRouteUnavailable);
	TestEqual(TEXT("preflight gates do not sample timeline"),
		TimelineSamples, 0);
	TestEqual(TEXT("preflight gates do not invoke product route"),
		RouteCalls, 0);
	TestEqual(TEXT("preflight gates preserve Run sequence"),
		Fixture.Coordinator.GetNextPlayerWeaponGuardActivationSequence(),
		static_cast<uint64>(1));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapWeaponGuardInputAdapterTimelineTest,
	"Shanmen.0_0_10.Product.WeaponGuardInputAdapter.Timeline",
	InputAdapterFlags)

bool Fdemo_mapWeaponGuardInputAdapterTimelineTest::RunTest(const FString&)
{
	FInputAdapterFixture Fixture;
	Fdemo_mapShanmenWeaponGuardInputTimelineSample Missing;
	Fdemo_mapShanmenWeaponGuardInputTimelineSample Rejected;
	const bool bMissingCaptured =
		Fdemo_mapShanmenWeaponGuardInputTimelineSample::TryCapture(
			FGuid(), 10, Rejected);
	const bool bNegativeCaptured =
		Fdemo_mapShanmenWeaponGuardInputTimelineSample::TryCapture(
			InputAdapterTimeline, -1, Rejected);
	const bool bOverflowCaptured =
		Fdemo_mapShanmenWeaponGuardInputTimelineSample::TryCapture(
			InputAdapterTimeline, MAX_int64 - 4, Rejected);
	int32 TimelineSamples = 0;
	int32 RouteCalls = 0;
	const auto Result = Fixture.Route(
		true, true, Missing, TimelineSamples, RouteCalls);
	TestFalse(TEXT("missing timeline capture fails"), bMissingCaptured);
	TestFalse(TEXT("negative timeline capture fails"), bNegativeCaptured);
	TestFalse(TEXT("overflow timeline capture fails"), bOverflowCaptured);
	TestEqual(TEXT("eligible input samples exactly once"),
		TimelineSamples, 1);
	TestEqual(TEXT("invalid sample fails before product route"),
		RouteCalls, 0);
	TestEqual(TEXT("invalid sample has typed classification"),
		Result.Status,
		Edemo_mapShanmenWeaponGuardInputStatus::TimelineRejected);
	TestEqual(TEXT("invalid sample preserves Run sequence"),
		Fixture.Coordinator.GetNextPlayerWeaponGuardActivationSequence(),
		static_cast<uint64>(1));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapWeaponGuardInputAdapterSingleSampleTest,
	"Shanmen.0_0_10.Product.WeaponGuardInputAdapter.SingleSample",
	InputAdapterFlags)

bool Fdemo_mapWeaponGuardInputAdapterSingleSampleTest::RunTest(
	const FString&)
{
	const auto Sample = MakeTimeline(33);
	int32 TimelineSamples = 0;
	int32 RouteCalls = 0;
	FGuid RoutedTimeline;
	int64 RoutedTick = INDEX_NONE;
	const auto Result =
		Fdemo_mapShanmenWeaponGuardInputAdapter::RouteStartInput(
			true,
			true,
			[&Sample, &TimelineSamples]()
			{
				++TimelineSamples;
				return Sample;
			},
			[&](const FGuid& TimelineId, int64 ActiveStartTick)
			{
				++RouteCalls;
				RoutedTimeline = TimelineId;
				RoutedTick = ActiveStartTick;
				return Fdemo_mapShanmenWeaponGuardProductRouteResult();
			});
	TestEqual(TEXT("timeline is sampled once"), TimelineSamples, 1);
	TestEqual(TEXT("product route is invoked once"), RouteCalls, 1);
	TestEqual(TEXT("timeline identity is forwarded without replacement"),
		RoutedTimeline, InputAdapterTimeline);
	TestEqual(TEXT("opaque tick is forwarded without conversion"),
		RoutedTick, static_cast<int64>(33));
	TestEqual(TEXT("downstream rejection is not retried"),
		Result.Status,
		Edemo_mapShanmenWeaponGuardInputStatus::ProductRejected);
	TestFalse(TEXT("rejected proof is not accepted"), Result.IsAccepted());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapWeaponGuardInputAdapterItemFenceTest,
	"Shanmen.0_0_10.Product.WeaponGuardInputAdapter.ItemFence",
	InputAdapterFlags)

bool Fdemo_mapWeaponGuardInputAdapterItemFenceTest::RunTest(
	const FString&)
{
	FInputAdapterFixture Fixture;
	const auto Sample = MakeTimeline();
	int32 TimelineSamples = 0;
	int32 RouteCalls = 0;
	const auto Result = Fixture.Route(
		true, true, Sample, TimelineSamples, RouteCalls);
	TestTrue(TEXT("fixture Run is ready"), Fixture.bReady);
	TestEqual(TEXT("eligible input samples once"), TimelineSamples, 1);
	TestEqual(TEXT("eligible input delegates once"), RouteCalls, 1);
	TestEqual(TEXT("item fence remains owned by P11.8"),
		Result.ProductRoute.Status,
		Edemo_mapShanmenWeaponGuardProductRouteStatus::
			ItemAuthorizationRejected);
	TestEqual(TEXT("adapter reports product rejection"),
		Result.Status,
		Edemo_mapShanmenWeaponGuardInputStatus::ProductRejected);
	TestEqual(TEXT("item rejection preserves Run sequence"),
		Fixture.Coordinator.GetNextPlayerWeaponGuardActivationSequence(),
		static_cast<uint64>(1));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapWeaponGuardInputAdapterAppliedTest,
	"Shanmen.0_0_10.Product.WeaponGuardInputAdapter.AppliedProof",
	InputAdapterFlags)

bool Fdemo_mapWeaponGuardInputAdapterAppliedTest::RunTest(const FString&)
{
	FInputAdapterFixture Fixture;
	const FGuid WeaponId = Fixture.EquipWeapon();
	const int32 Revision = Fixture.Items.GetAuthorityRevision();
	const auto Sample = MakeTimeline(40);
	int32 TimelineSamples = 0;
	int32 RouteCalls = 0;
	const auto Result = Fixture.Route(
		true, true, Sample, TimelineSamples, RouteCalls);
	TestTrue(TEXT("equipped weapon input reaches canonical product"),
		Fixture.bReady
			&& WeaponId.IsValid()
			&& Result.IsAccepted());
	TestEqual(TEXT("accepted input samples once"), TimelineSamples, 1);
	TestEqual(TEXT("accepted input delegates once"), RouteCalls, 1);
	TestEqual(TEXT("exact item remains P11.7/P11.8-owned"),
		Result.ProductRoute.ItemAuthorization.Authorization
			.GetSourceItemInstanceId(),
		WeaponId);
	TestEqual(TEXT("sample timeline reaches host unchanged"),
		Result.ProductRoute.ProductStart.Host.GetTimingPolicy()
			.GetTimelineId(),
		InputAdapterTimeline);
	TestEqual(TEXT("sample tick reaches host unchanged"),
		Result.ProductRoute.ProductStart.Host.GetTimingPolicy()
			.GetActiveStartTick(),
		static_cast<int64>(40));
	TestEqual(TEXT("input adapter never mutates item authority"),
		Fixture.Items.GetAuthorityRevision(),
		Revision);
	TestEqual(TEXT("accepted input consumes one Run sequence"),
		Fixture.Coordinator.GetNextPlayerWeaponGuardActivationSequence(),
		static_cast<uint64>(2));
	return true;
}

#endif
