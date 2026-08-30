#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "ShanmenCombatResolver.h"
#include "ShanmenCombatTags.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "demo_mapCombatRunCoordinator.h"
#include "demo_mapPlayerHealthComponent.h"
#include "demo_mapShanmenSpiritEvasionProductAuthority.h"

#include <limits>

namespace
{
	const EAutomationTestFlags ProductAuthorityFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;
	const FGuid ProductAuthorityRunA(
		0xE0300001, 0xE0300002, 0xE0300003, 0xE0300004);
	const FGuid ProductAuthorityRunB(
		0xE0310001, 0xE0310002, 0xE0310003, 0xE0310004);

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
					Pawn,
					TEXT("SpiritEvasionAuthorityRoot"))
				: nullptr;
			Health = Pawn
				? NewObject<Udemo_mapPlayerHealthComponent>(
					Pawn,
					TEXT("SpiritEvasionAuthorityHealth"))
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

	Fdemo_mapShanmenSpiritEvasionProductConfig MakeCanonicalConfig()
	{
		Fdemo_mapShanmenSpiritEvasionProductConfig Config;
		check(Fdemo_mapShanmenSpiritEvasionProductConfig::TryCreateCanonical(
			Config));
		return Config;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSpiritEvasionCanonicalConfigTest,
	"Shanmen.0_0_10.Product.SpiritEvasionProductAuthority.CanonicalConfig",
	ProductAuthorityFlags)

bool Fdemo_mapSpiritEvasionCanonicalConfigTest::RunTest(
	const FString& Parameters)
{
	Fdemo_mapShanmenSpiritEvasionProductConfig Empty;
	TestFalse(TEXT("default config is invalid"), Empty.IsValid());

	const Fdemo_mapShanmenSpiritEvasionProductConfig First =
		MakeCanonicalConfig();
	const Fdemo_mapShanmenSpiritEvasionProductConfig Replay =
		MakeCanonicalConfig();
	FGameplayTagContainer ExpectedTargetTags;
	ExpectedTargetTags.AddTag(FShanmenCombatNativeTags::TargetLiving());
	TestTrue(TEXT("canonical config is valid"), First.IsValid());
	TestEqual(
		TEXT("config identity is replay stable"),
		First.GetConfigId(),
		Replay.GetConfigId());
	TestEqual(
		TEXT("config id is the canonical identity"),
		First.GetConfigId(),
		Fdemo_mapShanmenSpiritEvasionProductConfig::CanonicalConfigId());
	TestEqual(
		TEXT("content version is frozen"),
		First.GetContent().Version,
		Fdemo_mapShanmenSpiritEvasionProductConfig::
			CanonicalContentVersion());
	TestEqual(
		TEXT("content digest is frozen"),
		First.GetContent().Digest,
		Fdemo_mapShanmenSpiritEvasionProductConfig::
			CanonicalContentDigest());
	TestEqual(
		TEXT("active evasion targets living recipients"),
		First.GetDefinition().GetRequiredTargetTags(),
		ExpectedTargetTags);
	TestTrue(
		TEXT("active evasion is not incorrectly restricted to player attackers"),
		First.GetDefinition().GetRequiredSourceTags().IsEmpty());
	TestTrue(
		TEXT("active evasion accepts every canonical damage family"),
		First.GetDefinition().GetRequiredDamageTags().IsEmpty()
			&& First.GetDefinition().GetBlockedDamageTags().IsEmpty());
	TestEqual(
		TEXT("requested distance is product-owned"),
		First.GetPolicy().GetRequestedDistance(),
		400.0f);
	TestEqual(
		TEXT("minimum distance is product-owned"),
		First.GetPolicy().GetMinimumResolvedDistance(),
		100.0f);
	TestEqual(
		TEXT("trajectory duration is product-owned"),
		First.GetTrajectory().GetDurationSeconds(),
		0.2f);
	TestEqual(
		TEXT("trajectory segmentation is product-owned"),
		First.GetTrajectory().GetSegmentCount(),
		2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSpiritEvasionReservationFenceTest,
	"Shanmen.0_0_10.Product.SpiritEvasionProductAuthority.ReservationFences",
	ProductAuthorityFlags)

bool Fdemo_mapSpiritEvasionReservationFenceTest::RunTest(
	const FString& Parameters)
{
	Fdemo_mapCombatRunCoordinator Unready;
	Fdemo_mapPlayerSpiritEvasionActionReservation Reservation;
	FString Diagnostic;
	TestFalse(
		TEXT("an unready Run cannot reserve identity"),
		Unready.TryReservePlayerSpiritEvasionAction(
			MakeCanonicalConfig(),
			Reservation,
			Diagnostic));
	TestFalse(TEXT("unready rejection returns no reservation"), Reservation.IsValid());

	FProductAuthorityFixture Fixture;
	TestTrue(TEXT("fixture is ready"), Fixture.bReady);
	Fdemo_mapShanmenSpiritEvasionProductConfig InvalidConfig;
	TestFalse(
		TEXT("noncanonical content cannot consume the Run sequence"),
		Fixture.Coordinator.TryReservePlayerSpiritEvasionAction(
			InvalidConfig,
			Reservation,
			Diagnostic));
	TestEqual(
		TEXT("rejected config leaves sequence untouched"),
		Fixture.Coordinator.GetNextPlayerSpiritEvasionActivationSequence(),
		static_cast<uint64>(1));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSpiritEvasionSequentialReservationTest,
	"Shanmen.0_0_10.Product.SpiritEvasionProductAuthority.SequentialReservation",
	ProductAuthorityFlags)

bool Fdemo_mapSpiritEvasionSequentialReservationTest::RunTest(
	const FString& Parameters)
{
	FProductAuthorityFixture Fixture;
	const Fdemo_mapShanmenSpiritEvasionProductConfig Config =
		MakeCanonicalConfig();
	Fdemo_mapPlayerSpiritEvasionActionReservation First;
	Fdemo_mapPlayerSpiritEvasionActionReservation Second;
	FString Diagnostic;
	TestTrue(
		TEXT("first identity is reserved"),
		Fixture.bReady
			&& Fixture.Coordinator.TryReservePlayerSpiritEvasionAction(
				Config,
				First,
				Diagnostic));
	TestTrue(TEXT("first reservation is self-validating"), First.IsValid());
	TestEqual(
		TEXT("first sequence begins at one"),
		First.GetActivationSequence(),
		static_cast<uint64>(1));
	TestEqual(
		TEXT("reservation belongs to exact Run"),
		First.GetAction().GetRunId(),
		Fixture.Coordinator.GetRunId());
	TestEqual(
		TEXT("reservation belongs to registered player"),
		First.GetAction().GetSourceEntityId(),
		Fixture.Coordinator.GetPlayerEntityId());
	TestEqual(
		TEXT("player is also the action owner"),
		First.GetAction().GetOwnerId(),
		Fixture.Coordinator.GetPlayerEntityId());
	TestFalse(
		TEXT("Spirit Evasion does not forge an item identity"),
		First.GetAction().GetSourceItemInstanceId().IsValid());
	TestEqual(
		TEXT("activation id is derived from sequence"),
		First.GetActivationId(),
		FShanmenCombatIdFactory::MakeActivationId(
			First.GetAction().GetRunId(),
			First.GetAction().GetSourceEntityId(),
			First.GetAction().GetActionDefinitionId(),
			First.GetActivationSequence()));

	TestTrue(
		TEXT("second identity is reserved"),
		Fixture.Coordinator.TryReservePlayerSpiritEvasionAction(
			Config,
			Second,
			Diagnostic));
	TestEqual(
		TEXT("second sequence is monotonic"),
		Second.GetActivationSequence(),
		static_cast<uint64>(2));
	TestNotEqual(
		TEXT("separate attempts cannot share activation identity"),
		Second.GetActivationId(),
		First.GetActivationId());
	TestEqual(
		TEXT("next sequence advances only after accepted reservations"),
		Fixture.Coordinator.GetNextPlayerSpiritEvasionActivationSequence(),
		static_cast<uint64>(3));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSpiritEvasionRunResetReservationTest,
	"Shanmen.0_0_10.Product.SpiritEvasionProductAuthority.RunReset",
	ProductAuthorityFlags)

bool Fdemo_mapSpiritEvasionRunResetReservationTest::RunTest(
	const FString& Parameters)
{
	FProductAuthorityFixture Fixture;
	const Fdemo_mapShanmenSpiritEvasionProductConfig Config =
		MakeCanonicalConfig();
	Fdemo_mapPlayerSpiritEvasionActionReservation FirstRun;
	FString Diagnostic;
	TestTrue(
		TEXT("first Run reserves one identity"),
		Fixture.bReady
			&& Fixture.Coordinator.TryReservePlayerSpiritEvasionAction(
				Config,
				FirstRun,
				Diagnostic));
	TestTrue(
		TEXT("first Run closes cleanly"),
		Fixture.Coordinator.TryEndRun(ProductAuthorityRunA, Diagnostic));
	TestEqual(
		TEXT("closed Run resets the local sequence"),
		Fixture.Coordinator.GetNextPlayerSpiritEvasionActivationSequence(),
		static_cast<uint64>(1));
	TestTrue(
		TEXT("same player can bind the next Run"),
		Fixture.Coordinator.TryBeginRun(
			ProductAuthorityRunB,
			Fixture.Pawn,
			Fixture.Health,
			Diagnostic));
	Fdemo_mapPlayerSpiritEvasionActionReservation SecondRun;
	TestTrue(
		TEXT("next Run starts at its own sequence one"),
		Fixture.Coordinator.TryReservePlayerSpiritEvasionAction(
			Config,
			SecondRun,
			Diagnostic)
			&& SecondRun.GetActivationSequence() == 1);
	TestNotEqual(
		TEXT("Run identity keeps sequence-one activations distinct"),
		FirstRun.GetActivationId(),
		SecondRun.GetActivationId());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSpiritEvasionInvalidDirectionTest,
	"Shanmen.0_0_10.Product.SpiritEvasionProductAuthority.InvalidDirection",
	ProductAuthorityFlags)

bool Fdemo_mapSpiritEvasionInvalidDirectionTest::RunTest(
	const FString& Parameters)
{
	FProductAuthorityFixture Fixture;
	const Fdemo_mapShanmenSpiritEvasionProductStartResult Zero =
		Fdemo_mapShanmenSpiritEvasionProductAuthority::PrepareStart(
			Fixture.Coordinator,
			FVector::ZeroVector);
	TestEqual(
		TEXT("zero direction is classified before reservation"),
		Zero.Status,
		Edemo_mapShanmenSpiritEvasionProductStartStatus::InvalidDirection);
	TestFalse(TEXT("zero direction has no command"), Zero.IsReady());
	TestEqual(
		TEXT("zero direction does not consume sequence"),
		Fixture.Coordinator.GetNextPlayerSpiritEvasionActivationSequence(),
		static_cast<uint64>(1));

	const float NaN = std::numeric_limits<float>::quiet_NaN();
	const Fdemo_mapShanmenSpiritEvasionProductStartResult NonFinite =
		Fdemo_mapShanmenSpiritEvasionProductAuthority::PrepareStart(
			Fixture.Coordinator,
			FVector(NaN, 1.0f, 0.0f));
	TestEqual(
		TEXT("non-finite direction is rejected"),
		NonFinite.Status,
		Edemo_mapShanmenSpiritEvasionProductStartStatus::InvalidDirection);
	TestEqual(
		TEXT("non-finite rejection also preserves sequence"),
		Fixture.Coordinator.GetNextPlayerSpiritEvasionActivationSequence(),
		static_cast<uint64>(1));

	const Fdemo_mapShanmenSpiritEvasionProductStartResult Vertical =
		Fdemo_mapShanmenSpiritEvasionProductAuthority::PrepareStart(
			Fixture.Coordinator,
			FVector::UpVector);
	TestEqual(
		TEXT("pure vertical input has no product movement direction"),
		Vertical.Status,
		Edemo_mapShanmenSpiritEvasionProductStartStatus::InvalidDirection);
	TestEqual(
		TEXT("vertical rejection occurs before sequence reservation"),
		Fixture.Coordinator.GetNextPlayerSpiritEvasionActivationSequence(),
		static_cast<uint64>(1));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSpiritEvasionStartCompositionTest,
	"Shanmen.0_0_10.Product.SpiritEvasionProductAuthority.StartComposition",
	ProductAuthorityFlags)

bool Fdemo_mapSpiritEvasionStartCompositionTest::RunTest(
	const FString& Parameters)
{
	FProductAuthorityFixture Fixture;
	const Fdemo_mapShanmenSpiritEvasionProductStartResult Result =
		Fdemo_mapShanmenSpiritEvasionProductAuthority::PrepareStart(
			Fixture.Coordinator,
			FVector(3.0f, 4.0f, 7.0f));
	TestTrue(TEXT("complete start proof is ready"), Fixture.bReady && Result.IsReady());
	TestEqual(
		TEXT("command retains the Run reservation action"),
		Result.Command.GetActivationId(),
		Result.Reservation.GetActivationId());
	TestEqual(
		TEXT("command retains canonical defense content"),
		Result.Command.GetDefinition().GetRuleId(),
		Result.Config.GetDefinition().GetRuleId());
	TestEqual(
		TEXT("command retains canonical movement policy"),
		Result.Command.GetPolicy().GetMovementPolicyId(),
		Result.Config.GetPolicy().GetMovementPolicyId());
	TestEqual(
		TEXT("direction is normalized once at typed capture"),
		Result.Command.GetCandidateDirection(),
		FVector(0.6f, 0.8f, 0.0f));
	TestEqual(
		TEXT("one accepted composition consumes one sequence"),
		Fixture.Coordinator.GetNextPlayerSpiritEvasionActivationSequence(),
		static_cast<uint64>(2));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSpiritEvasionDistinctStartCompositionTest,
	"Shanmen.0_0_10.Product.SpiritEvasionProductAuthority.DistinctAttempts",
	ProductAuthorityFlags)

bool Fdemo_mapSpiritEvasionDistinctStartCompositionTest::RunTest(
	const FString& Parameters)
{
	FProductAuthorityFixture Fixture;
	const Fdemo_mapShanmenSpiritEvasionProductStartResult First =
		Fdemo_mapShanmenSpiritEvasionProductAuthority::PrepareStart(
			Fixture.Coordinator,
			FVector::ForwardVector);
	const Fdemo_mapShanmenSpiritEvasionProductStartResult Second =
		Fdemo_mapShanmenSpiritEvasionProductAuthority::PrepareStart(
			Fixture.Coordinator,
			FVector::RightVector);
	TestTrue(
		TEXT("both attempts produce auditable commands"),
		First.IsReady() && Second.IsReady());
	TestEqual(
		TEXT("both attempts use one canonical config identity"),
		First.Config.GetConfigId(),
		Second.Config.GetConfigId());
	TestNotEqual(
		TEXT("each attempt receives a unique activation identity"),
		First.Command.GetActivationId(),
		Second.Command.GetActivationId());
	TestEqual(
		TEXT("directions remain request-specific"),
		First.Command.GetCandidateDirection(),
		FVector::ForwardVector);
	TestEqual(
		TEXT("second direction remains request-specific"),
		Second.Command.GetCandidateDirection(),
		FVector::RightVector);
	return true;
}

#endif
