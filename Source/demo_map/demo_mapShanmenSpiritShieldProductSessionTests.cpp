#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenSpiritShieldProductSession.h"

#include "GameFramework/Pawn.h"
#include "Misc/AutomationTest.h"
#include "ShanmenCombatRuntimeTags.h"
#include "ShanmenCombatTags.h"
#include "demo_mapCombatRunCoordinator.h"
#include "demo_mapPlayerHealthComponent.h"
#include "demo_mapShanmenDivineSenseProductAuthority.h"

namespace
{
	constexpr EAutomationTestFlags SpiritShieldProductFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;
	const FGuid SpiritShieldProductRun(
		0x25100001, 0x25100002, 0x25100003, 0x25100004);

	struct FSpiritShieldProductFixture
	{
		APawn* Player = nullptr;
		Udemo_mapPlayerHealthComponent* Health = nullptr;
		Fdemo_mapCombatRunCoordinator Coordinator;
		Fdemo_mapShanmenCombatRunFixedTimeline Timeline;
		Fdemo_mapShanmenDivineSenseProductController SpiritEnergy;
		FString Diagnostic;

		bool Start(float OpeningSpiritEnergy = 100.0f)
		{
			Player = NewObject<APawn>(GetTransientPackage());
			Health = Player
				? NewObject<Udemo_mapPlayerHealthComponent>(
					Player, TEXT("P251SpiritShieldHealth"))
				: nullptr;
			if (!Player || !Health
				|| !Coordinator.TryBeginRun(
					SpiritShieldProductRun, Player, Health, Diagnostic)
				|| !Timeline.TryBegin(SpiritShieldProductRun, Diagnostic))
			{
				return false;
			}
			FShanmenActionResourceAuthority OpeningAuthority;
			FShanmenActionResourceSnapshot OpeningSnapshot;
			Fdemo_mapShanmenDivineSenseProductConfig Config;
			return FShanmenActionResourceAuthority::TryCreate(
					Coordinator.GetPlayerEntityId(),
					FShanmenCombatRuntimeNativeTags::ResourceSpiritEnergy(),
					OpeningSpiritEnergy,
					100.0f,
					0,
					OpeningAuthority)
				&& OpeningAuthority.TryCaptureSnapshot(OpeningSnapshot)
				&& Fdemo_mapShanmenDivineSenseProductAuthority::
					TryCreateCanonicalConfig(Config)
				&& SpiritEnergy.TryBegin(
					Coordinator, OpeningSnapshot, Config, Diagnostic);
		}

		Fdemo_mapShanmenCombatRunTimelineSample CaptureTimeline()
		{
			Fdemo_mapShanmenCombatRunTimelineSample Sample;
			Timeline.TryCapture(Sample);
			return Sample;
		}

		Fdemo_mapShanmenPlayerActionGateResult AuthorizeShield()
		{
			return Fdemo_mapShanmenPlayerActionGateResult::FromArbitration(
				Coordinator.TryAuthorizePlayerAction(
					Edemo_mapShanmenPlayerActionKind::SpiritShield,
					Fdemo_mapShanmenPlayerActionOccupancySnapshot()));
		}

		float CurrentSpiritEnergy() const
		{
			Fdemo_mapShanmenDivineSenseProductAvailability Availability;
			FString AvailabilityDiagnostic;
			return SpiritEnergy.TryCaptureAvailability(
					Coordinator, Availability, AvailabilityDiagnostic)
				? Availability.GetSessionAvailability().GetResourceSnapshot().
					GetCurrentAmount()
				: -1.0f;
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSpiritShieldProductCanonicalPolicyTest,
	"Shanmen.0_0_10.Product.SpiritShieldProductSession.CanonicalPolicy",
	SpiritShieldProductFlags)

bool Fdemo_mapSpiritShieldProductCanonicalPolicyTest::RunTest(
	const FString&)
{
	FShanmenSpiritShieldDefinition Definition;
	FShanmenActionResourceCost Cost;
	TestTrue(TEXT("canonical shield definition captures"),
		Fdemo_mapShanmenSpiritShieldProductAuthority::
			TryCreateCanonicalDefinition(Definition));
	TestTrue(TEXT("canonical shared-energy cost captures"),
		Fdemo_mapShanmenSpiritShieldProductAuthority::
			TryCreateCanonicalCost(Cost));
	TestTrue(TEXT("P25.1 prototype policy is explicit and typed"),
		Definition.IsValid()
			&& Definition.GetActionDefinitionId()
				== FShanmenSpiritShieldDefinition::
					CanonicalActionDefinitionId()
			&& Definition.GetRuleId()
				== Fdemo_mapShanmenSpiritShieldProductAuthority::
					CanonicalRuleId()
			&& Definition.GetRequiredDamageTags().HasTagExact(
				FShanmenCombatNativeTags::DamagePhysical())
			&& Definition.GetRequiredTargetTags().HasTagExact(
				FShanmenCombatNativeTags::TargetLiving())
			&& FMath::IsNearlyEqual(
				Definition.GetMaximumCapacity(), 30.0f)
			&& Cost.GetResourceChannel()
				== FShanmenCombatRuntimeNativeTags::ResourceSpiritEnergy()
			&& FMath::IsNearlyEqual(Cost.GetAmount(), 20.0f)
			&& Fdemo_mapShanmenSpiritShieldProductAuthority::
				CanonicalDurationTicks() == 90);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSpiritShieldProductActivationTest,
	"Shanmen.0_0_10.Product.SpiritShieldProductSession.SharedLedgerActivation",
	SpiritShieldProductFlags)

bool Fdemo_mapSpiritShieldProductActivationTest::RunTest(const FString&)
{
	FSpiritShieldProductFixture Fixture;
	if (!Fixture.Start())
	{
		AddError(Fixture.Diagnostic);
		return false;
	}
	Fdemo_mapShanmenSpiritShieldProductSession Session;
	const auto Activated = Session.TryActivate(
		Fixture.Coordinator,
		Fixture.SpiritEnergy,
		Fixture.CaptureTimeline(),
		[&Fixture]() { return Fixture.AuthorizeShield(); });
	TestTrue(TEXT("one input atomically activates the full shield session"),
		Activated.IsAccepted()
			&& Session.IsValid() && Session.IsActive()
			&& Session.GetReservation().GetActivationSequence() == 1
			&& Session.GetDeadlineTick() == 90
			&& FMath::IsNearlyEqual(Session.GetAvailableCapacity(), 30.0f));
	TestTrue(TEXT("activation spends exactly once from the shared ledger"),
		FMath::IsNearlyEqual(Fixture.CurrentSpiritEnergy(), 80.0f)
			&& Fixture.SpiritEnergy.GetSession().GetHost().
				NumExternalSpiritEnergyTransactions() == 1
			&& Activated.SharedResource.Receipt.GetResourceBefore().
				GetCurrentAmount() == 100.0f
			&& Activated.SharedResource.Receipt.GetResourceAfter().
				GetCurrentAmount() == 80.0f);

	const auto Duplicate = Session.TryActivate(
		Fixture.Coordinator,
		Fixture.SpiritEnergy,
		Fixture.CaptureTimeline(),
		[&Fixture]() { return Fixture.AuthorizeShield(); });
	TestTrue(TEXT("repeat input while active cannot spend or reserve again"),
		Duplicate.IsValid() && !Duplicate.IsAccepted()
			&& Duplicate.Error
				== Edemo_mapShanmenSpiritShieldProductActivationError::
					AlreadyActive
			&& FMath::IsNearlyEqual(Fixture.CurrentSpiritEnergy(), 80.0f)
			&& Fixture.Coordinator.
				GetNextPlayerSpiritShieldActivationSequence() == 2
			&& Fixture.Coordinator.
				GetNextPlayerActionArbitrationSequence() == 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSpiritShieldProductInsufficientEnergyTest,
	"Shanmen.0_0_10.Product.SpiritShieldProductSession.InsufficientEnergyAtomicity",
	SpiritShieldProductFlags)

bool Fdemo_mapSpiritShieldProductInsufficientEnergyTest::RunTest(
	const FString&)
{
	FSpiritShieldProductFixture Fixture;
	if (!Fixture.Start(15.0f))
	{
		AddError(Fixture.Diagnostic);
		return false;
	}
	Fdemo_mapShanmenSpiritShieldProductSession Session;
	const auto Rejected = Session.TryActivate(
		Fixture.Coordinator,
		Fixture.SpiritEnergy,
		Fixture.CaptureTimeline(),
		[&Fixture]() { return Fixture.AuthorizeShield(); });
	TestTrue(TEXT("insufficient shared energy rejects a complete activation"),
		Rejected.IsValid() && !Rejected.IsAccepted()
			&& Rejected.Error
				== Edemo_mapShanmenSpiritShieldProductActivationError::
					SessionRejected
			&& Session.IsValid() && Session.IsEmpty());
	TestTrue(TEXT("failed mutation publishes neither spend nor ledger proof"),
		FMath::IsNearlyEqual(Fixture.CurrentSpiritEnergy(), 15.0f)
			&& Fixture.SpiritEnergy.GetSession().GetHost().
				NumExternalSpiritEnergyTransactions() == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSpiritShieldProductDeadlineTest,
	"Shanmen.0_0_10.Product.SpiritShieldProductSession.FixedTimelineDeadline",
	SpiritShieldProductFlags)

bool Fdemo_mapSpiritShieldProductDeadlineTest::RunTest(const FString&)
{
	FSpiritShieldProductFixture Fixture;
	if (!Fixture.Start())
	{
		AddError(Fixture.Diagnostic);
		return false;
	}
	Fdemo_mapShanmenSpiritShieldProductSession Session;
	check(Session.TryActivate(
		Fixture.Coordinator,
		Fixture.SpiritEnergy,
		Fixture.CaptureTimeline(),
		[&Fixture]() { return Fixture.AuthorizeShield(); }).IsAccepted());
	const auto Early = Session.ObserveTimeline(Fixture.CaptureTimeline());
	TestTrue(TEXT("an early sample leaves the shield active"),
		Early.IsSuccess()
			&& Early.Status
				== Edemo_mapShanmenSpiritShieldProductTimelineStatus::Waiting
			&& Session.IsActive());

	int64 AdvancedTicks = 0;
	TestTrue(TEXT("fixture advances by the canonical three seconds"),
		Fixture.Timeline.TryAdvance(
			3.0, AdvancedTicks, Fixture.Diagnostic)
			&& AdvancedTicks == 90);
	const auto Due = Session.ObserveTimeline(Fixture.CaptureTimeline());
	TestTrue(TEXT("deadline closes shield and action together"),
		Due.IsSuccess()
			&& Due.Status
				== Edemo_mapShanmenSpiritShieldProductTimelineStatus::Closed
			&& Due.Closure.IsSuccess()
			&& Session.IsValid() && Session.IsClosed()
			&& !Session.IsActive());
	TestTrue(TEXT("duration closure never refunds an already committed cost"),
		FMath::IsNearlyEqual(Fixture.CurrentSpiritEnergy(), 80.0f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSpiritShieldProductReactivationAndReleaseTest,
	"Shanmen.0_0_10.Product.SpiritShieldProductSession.ReactivationAndOwnerRelease",
	SpiritShieldProductFlags)

bool Fdemo_mapSpiritShieldProductReactivationAndReleaseTest::RunTest(
	const FString&)
{
	FSpiritShieldProductFixture Fixture;
	if (!Fixture.Start())
	{
		AddError(Fixture.Diagnostic);
		return false;
	}
	Fdemo_mapShanmenSpiritShieldProductSession Session;
	check(Session.TryActivate(
		Fixture.Coordinator,
		Fixture.SpiritEnergy,
		Fixture.CaptureTimeline(),
		[&Fixture]() { return Fixture.AuthorizeShield(); }).IsAccepted());
	FString ReleaseDiagnostic;
	TestTrue(TEXT("Run teardown closes and clears an active shield"),
		Session.TryReleaseOwner(ReleaseDiagnostic)
			&& Session.IsValid() && Session.IsEmpty());
	const auto Reactivated = Session.TryActivate(
		Fixture.Coordinator,
		Fixture.SpiritEnergy,
		Fixture.CaptureTimeline(),
		[&Fixture]() { return Fixture.AuthorizeShield(); });
	TestTrue(TEXT("the same Run can activate a new deterministic session"),
		Reactivated.IsAccepted()
			&& Reactivated.Reservation.GetActivationSequence() == 2
			&& FMath::IsNearlyEqual(Fixture.CurrentSpiritEnergy(), 60.0f)
			&& Fixture.SpiritEnergy.GetSession().GetHost().
				NumExternalSpiritEnergyTransactions() == 2);
	return true;
}

#endif
