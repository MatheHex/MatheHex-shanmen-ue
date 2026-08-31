#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "ShanmenCombatResolver.h"
#include "ShanmenCombatTags.h"
#include "demo_mapCombatRunCoordinator.h"
#include "demo_mapPlayerHealthComponent.h"
#include "demo_mapShanmenWeaponGuardProductAuthority.h"

namespace
{
	const EAutomationTestFlags ProductAuthorityFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;
	const FGuid ProductAuthorityRunA(
		0xDF200001, 0xDF200002, 0xDF200003, 0xDF200004);
	const FGuid ProductAuthorityRunB(
		0xDF210001, 0xDF210002, 0xDF210003, 0xDF210004);
	const FGuid ProductAuthorityWeaponA(
		0xDF220001, 0xDF220002, 0xDF220003, 0xDF220004);
	const FGuid ProductAuthorityWeaponB(
		0xDF230001, 0xDF230002, 0xDF230003, 0xDF230004);
	const FGuid ProductAuthorityTimeline(
		0xDF240001, 0xDF240002, 0xDF240003, 0xDF240004);

	struct FProductAuthorityFixture
	{
		APawn* Pawn = nullptr;
		UBoxComponent* Root = nullptr;
		Udemo_mapPlayerHealthComponent* Health = nullptr;
		Fdemo_mapCombatRunCoordinator Coordinator;
		FString Diagnostic;
		bool bReady = false;

		FProductAuthorityFixture()
		{
			Pawn = NewObject<APawn>(GetTransientPackage());
			Root = Pawn
				? NewObject<UBoxComponent>(
					Pawn, TEXT("WeaponGuardAuthorityRoot"))
				: nullptr;
			Health = Pawn
				? NewObject<Udemo_mapPlayerHealthComponent>(
					Pawn, TEXT("WeaponGuardAuthorityHealth"))
				: nullptr;
			if (Pawn && Root)
			{
				Pawn->SetRootComponent(Root);
			}
			bReady = Pawn && Root && Health
				&& Coordinator.TryBeginRun(
					ProductAuthorityRunA,
					Pawn,
					Health,
					Diagnostic);
		}

		~FProductAuthorityFixture()
		{
			Coordinator.Reset();
		}
	};

	Fdemo_mapShanmenWeaponGuardProductConfig MakeCanonicalConfig()
	{
		Fdemo_mapShanmenWeaponGuardProductConfig Config;
		check(Fdemo_mapShanmenWeaponGuardProductConfig::TryCreateCanonical(
			Config));
		return Config;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapWeaponGuardCanonicalConfigTest,
	"Shanmen.0_0_10.Product.WeaponGuardProductAuthority.CanonicalConfig",
	ProductAuthorityFlags)

bool Fdemo_mapWeaponGuardCanonicalConfigTest::RunTest(const FString&)
{
	Fdemo_mapShanmenWeaponGuardProductConfig Empty;
	TestFalse(TEXT("default config is invalid"), Empty.IsValid());
	const Fdemo_mapShanmenWeaponGuardProductConfig First =
		MakeCanonicalConfig();
	const Fdemo_mapShanmenWeaponGuardProductConfig Replay =
		MakeCanonicalConfig();
	TestTrue(TEXT("canonical config is valid"), First.IsValid());
	TestEqual(TEXT("config identity replays"),
		First.GetConfigId(), Replay.GetConfigId());
	TestEqual(TEXT("ordinary guard is product-owned"),
		First.GetDefinition().GetGuardFraction(), 0.25f);
	TestTrue(TEXT("ordinary guard requires physical damage"),
		First.GetDefinition().GetRequiredDamageTags().HasTagExact(
			FShanmenCombatNativeTags::DamagePhysical()));
	TestTrue(TEXT("mental damage is explicitly blocked"),
		First.GetDefinition().GetBlockedDamageTags().HasTagExact(
			FShanmenCombatNativeTags::DamageMental()));
	TestTrue(TEXT("guard targets living recipients"),
		First.GetDefinition().GetRequiredTargetTags().HasTagExact(
			FShanmenCombatNativeTags::TargetLiving()));
	TestEqual(TEXT("perfect interval is product-owned"),
		First.GetPerfectWindowTickCount(), static_cast<int64>(5));
	TestEqual(TEXT("facing threshold is product-owned"),
		First.GetMinimumFacingDot(), 0.5);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapWeaponGuardReservationFenceTest,
	"Shanmen.0_0_10.Product.WeaponGuardProductAuthority.ReservationFences",
	ProductAuthorityFlags)

bool Fdemo_mapWeaponGuardReservationFenceTest::RunTest(const FString&)
{
	Fdemo_mapCombatRunCoordinator Unready;
	Fdemo_mapPlayerWeaponGuardActionReservation Reservation;
	FString Diagnostic;
	TestFalse(TEXT("unready Run cannot reserve identity"),
		Unready.TryReservePlayerWeaponGuardAction(
			MakeCanonicalConfig(),
			ProductAuthorityWeaponA,
			Reservation,
			Diagnostic));
	TestFalse(TEXT("unready rejection has no reservation"),
		Reservation.IsValid());

	FProductAuthorityFixture Fixture;
	Fdemo_mapShanmenWeaponGuardProductConfig InvalidConfig;
	TestTrue(TEXT("fixture is ready"), Fixture.bReady);
	TestFalse(TEXT("noncanonical config is rejected"),
		Fixture.Coordinator.TryReservePlayerWeaponGuardAction(
			InvalidConfig,
			ProductAuthorityWeaponA,
			Reservation,
			Diagnostic));
	TestFalse(TEXT("missing item identity is rejected"),
		Fixture.Coordinator.TryReservePlayerWeaponGuardAction(
			MakeCanonicalConfig(),
			FGuid(),
			Reservation,
			Diagnostic));
	TestEqual(TEXT("all fences preserve sequence one"),
		Fixture.Coordinator.GetNextPlayerWeaponGuardActivationSequence(),
		static_cast<uint64>(1));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapWeaponGuardSequentialReservationTest,
	"Shanmen.0_0_10.Product.WeaponGuardProductAuthority.SequentialReservation",
	ProductAuthorityFlags)

bool Fdemo_mapWeaponGuardSequentialReservationTest::RunTest(const FString&)
{
	FProductAuthorityFixture Fixture;
	const Fdemo_mapShanmenWeaponGuardProductConfig Config =
		MakeCanonicalConfig();
	Fdemo_mapPlayerWeaponGuardActionReservation First;
	Fdemo_mapPlayerWeaponGuardActionReservation Second;
	FString Diagnostic;
	TestTrue(TEXT("first identity is reserved"),
		Fixture.bReady
			&& Fixture.Coordinator.TryReservePlayerWeaponGuardAction(
				Config,
				ProductAuthorityWeaponA,
				First,
				Diagnostic));
	TestTrue(TEXT("first reservation self-validates"), First.IsValid());
	TestEqual(TEXT("sequence starts at one"),
		First.GetActivationSequence(), static_cast<uint64>(1));
	TestEqual(TEXT("reservation belongs to exact Run"),
		First.GetAction().GetRunId(), Fixture.Coordinator.GetRunId());
	TestEqual(TEXT("player owns and sources the action"),
		First.GetAction().GetOwnerId(),
		Fixture.Coordinator.GetPlayerEntityId());
	TestEqual(TEXT("exact weapon identity is retained"),
		First.GetSourceItemInstanceId(), ProductAuthorityWeaponA);
	TestTrue(TEXT("second identity is reserved"),
		Fixture.Coordinator.TryReservePlayerWeaponGuardAction(
			Config,
			ProductAuthorityWeaponB,
			Second,
			Diagnostic));
	TestEqual(TEXT("second sequence is monotonic"),
		Second.GetActivationSequence(), static_cast<uint64>(2));
	TestNotEqual(TEXT("attempts have distinct activation identity"),
		First.GetActivationId(), Second.GetActivationId());
	TestEqual(TEXT("next sequence advances after two accepts"),
		Fixture.Coordinator.GetNextPlayerWeaponGuardActivationSequence(),
		static_cast<uint64>(3));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapWeaponGuardRunResetReservationTest,
	"Shanmen.0_0_10.Product.WeaponGuardProductAuthority.RunReset",
	ProductAuthorityFlags)

bool Fdemo_mapWeaponGuardRunResetReservationTest::RunTest(const FString&)
{
	FProductAuthorityFixture Fixture;
	const Fdemo_mapShanmenWeaponGuardProductConfig Config =
		MakeCanonicalConfig();
	Fdemo_mapPlayerWeaponGuardActionReservation FirstRun;
	FString Diagnostic;
	TestTrue(TEXT("first Run reserves identity"),
		Fixture.bReady
			&& Fixture.Coordinator.TryReservePlayerWeaponGuardAction(
				Config,
				ProductAuthorityWeaponA,
				FirstRun,
				Diagnostic));
	TestTrue(TEXT("first Run closes"),
		Fixture.Coordinator.TryEndRun(ProductAuthorityRunA, Diagnostic));
	TestEqual(TEXT("closed Run resets sequence"),
		Fixture.Coordinator.GetNextPlayerWeaponGuardActivationSequence(),
		static_cast<uint64>(1));
	TestTrue(TEXT("same player binds next Run"),
		Fixture.Coordinator.TryBeginRun(
			ProductAuthorityRunB,
			Fixture.Pawn,
			Fixture.Health,
			Diagnostic));
	Fdemo_mapPlayerWeaponGuardActionReservation SecondRun;
	TestTrue(TEXT("next Run starts at sequence one"),
		Fixture.Coordinator.TryReservePlayerWeaponGuardAction(
			Config,
			ProductAuthorityWeaponA,
			SecondRun,
			Diagnostic)
			&& SecondRun.GetActivationSequence() == 1);
	TestNotEqual(TEXT("Run identity separates sequence-one attempts"),
		FirstRun.GetActivationId(), SecondRun.GetActivationId());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapWeaponGuardTimingFenceTest,
	"Shanmen.0_0_10.Product.WeaponGuardProductAuthority.TimingFences",
	ProductAuthorityFlags)

bool Fdemo_mapWeaponGuardTimingFenceTest::RunTest(const FString&)
{
	FProductAuthorityFixture Fixture;
	const auto MissingTimeline =
		Fdemo_mapShanmenWeaponGuardProductAuthority::PrepareStart(
			Fixture.Coordinator,
			ProductAuthorityWeaponA,
			FGuid(),
			10);
	TestEqual(TEXT("missing timeline is classified"),
		MissingTimeline.Status,
		Edemo_mapShanmenWeaponGuardProductStartStatus::InvalidTimeline);
	const auto NegativeTick =
		Fdemo_mapShanmenWeaponGuardProductAuthority::PrepareStart(
			Fixture.Coordinator,
			ProductAuthorityWeaponA,
			ProductAuthorityTimeline,
			-1);
	TestEqual(TEXT("negative tick is classified"),
		NegativeTick.Status,
		Edemo_mapShanmenWeaponGuardProductStartStatus::InvalidTimeline);
	const auto Overflow =
		Fdemo_mapShanmenWeaponGuardProductAuthority::PrepareStart(
			Fixture.Coordinator,
			ProductAuthorityWeaponA,
			ProductAuthorityTimeline,
			MAX_int64 - 4);
	TestEqual(TEXT("window overflow is classified"),
		Overflow.Status,
		Edemo_mapShanmenWeaponGuardProductStartStatus::InvalidTimeline);
	const auto MissingItem =
		Fdemo_mapShanmenWeaponGuardProductAuthority::PrepareStart(
			Fixture.Coordinator,
			FGuid(),
			ProductAuthorityTimeline,
			10);
	TestEqual(TEXT("missing item is classified"),
		MissingItem.Status,
		Edemo_mapShanmenWeaponGuardProductStartStatus::InvalidSourceItem);
	TestEqual(TEXT("preflight fences do not consume sequence"),
		Fixture.Coordinator.GetNextPlayerWeaponGuardActivationSequence(),
		static_cast<uint64>(1));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapWeaponGuardStartCompositionTest,
	"Shanmen.0_0_10.Product.WeaponGuardProductAuthority.StartComposition",
	ProductAuthorityFlags)

bool Fdemo_mapWeaponGuardStartCompositionTest::RunTest(const FString&)
{
	FProductAuthorityFixture Fixture;
	const auto Result =
		Fdemo_mapShanmenWeaponGuardProductAuthority::PrepareStart(
			Fixture.Coordinator,
			ProductAuthorityWeaponA,
			ProductAuthorityTimeline,
			10);
	TestTrue(TEXT("complete start proof is ready"),
		Fixture.bReady && Result.IsReady());
	TestEqual(TEXT("host retains reserved activation"),
		Result.Host.GetActionRuntime().GetAction().GetActivationId(),
		Result.Reservation.GetActivationId());
	TestEqual(TEXT("host retains exact weapon"),
		Result.Host.GetActionRuntime().GetAction().GetSourceItemInstanceId(),
		ProductAuthorityWeaponA);
	TestEqual(TEXT("active start is caller-sampled"),
		Result.Host.GetTimingPolicy().GetActiveStartTick(),
		static_cast<int64>(10));
	TestEqual(TEXT("perfect end derives from config"),
		Result.Host.GetTimingPolicy().GetPerfectEndTick(),
		static_cast<int64>(15));
	TestEqual(TEXT("host uses canonical arc threshold"),
		Result.Host.GetArcPolicy().GetMinimumFacingDot(), 0.5);
	TestEqual(TEXT("one start consumes one sequence"),
		Fixture.Coordinator.GetNextPlayerWeaponGuardActivationSequence(),
		static_cast<uint64>(2));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapWeaponGuardDistinctStartCompositionTest,
	"Shanmen.0_0_10.Product.WeaponGuardProductAuthority.DistinctAttempts",
	ProductAuthorityFlags)

bool Fdemo_mapWeaponGuardDistinctStartCompositionTest::RunTest(
	const FString&)
{
	FProductAuthorityFixture Fixture;
	const auto First =
		Fdemo_mapShanmenWeaponGuardProductAuthority::PrepareStart(
			Fixture.Coordinator,
			ProductAuthorityWeaponA,
			ProductAuthorityTimeline,
			10);
	const auto Second =
		Fdemo_mapShanmenWeaponGuardProductAuthority::PrepareStart(
			Fixture.Coordinator,
			ProductAuthorityWeaponA,
			ProductAuthorityTimeline,
			10);
	TestTrue(TEXT("both attempts are independently auditable"),
		First.IsReady() && Second.IsReady());
	TestEqual(TEXT("attempts share one product config"),
		First.Config.GetConfigId(), Second.Config.GetConfigId());
	TestNotEqual(TEXT("attempts have distinct activation identity"),
		First.Reservation.GetActivationId(),
		Second.Reservation.GetActivationId());
	TestNotEqual(TEXT("attempts have distinct host identity"),
		First.Host.GetHostId(), Second.Host.GetHostId());
	return true;
}

#endif
