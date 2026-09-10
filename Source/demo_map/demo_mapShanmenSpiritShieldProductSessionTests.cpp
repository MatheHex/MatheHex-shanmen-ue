#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenSpiritShieldProductSession.h"

#include "GameFramework/Pawn.h"
#include "Misc/AutomationTest.h"
#include "ShanmenCombatResolver.h"
#include "ShanmenCombatRuntimeTags.h"
#include "ShanmenCombatTags.h"
#include "demo_mapCombatRunCoordinator.h"
#include "demo_mapPlayerHealthComponent.h"
#include "demo_mapShanmenDivineSenseProductAuthority.h"
#include "demo_mapShanmenSpiritShieldHUDPresentation.h"

namespace
{
	constexpr EAutomationTestFlags SpiritShieldProductFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;
	const FGuid SpiritShieldProductRun(
		0x25100001, 0x25100002, 0x25100003, 0x25100004);
	const FGuid SpiritShieldAttackSource(
		0x25200001, 0x25200002, 0x25200003, 0x25200004);

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

	struct FSpiritShieldProductImpactIdentity
	{
		FShanmenCombatActionSnapshot Action;
		FShanmenHitCandidate Candidate;
		FGuid ImpactId;
	};

	FSpiritShieldProductImpactIdentity MakeImpactIdentity(
		const FGuid& TargetEntityId,
		int32 HitOrdinal)
	{
		FShanmenCombatActionCapture Capture;
		Capture.RunId = SpiritShieldProductRun;
		Capture.OwnerId = SpiritShieldAttackSource;
		Capture.SourceEntityId = SpiritShieldAttackSource;
		Capture.ActionDefinitionId = TEXT("Combat.Action.Test.P25_2Attack");
		Capture.Content.Version = TEXT("0.0.10.P25.2");
		Capture.Content.Digest = TEXT("TEST-DIGEST-P25.2-ATTACK");
		Capture.SourceTags.AddTag(FShanmenCombatNativeTags::SourcePlayer());
		Capture.ActivationId = FShanmenCombatIdFactory::MakeActivationId(
			Capture.RunId,
			Capture.SourceEntityId,
			Capture.ActionDefinitionId,
			252);

		FSpiritShieldProductImpactIdentity Identity;
		check(FShanmenCombatActionSnapshot::TryCapture(
			Capture, Identity.Action));
		Identity.Candidate.ActivationId = Identity.Action.GetActivationId();
		Identity.Candidate.SourceEntityId = Identity.Action.GetSourceEntityId();
		Identity.Candidate.TargetEntityId = TargetEntityId;
		Identity.Candidate.DetectorId = TEXT("Detector.Test.P25_2Attack");
		Identity.Candidate.DetectorKind = EShanmenHitDetectorKind::Shape;
		Identity.Candidate.HitNormal = FVector::BackwardVector;
		Identity.Candidate.HitOrdinal = HitOrdinal;
		Identity.ImpactId = FShanmenCombatIdFactory::MakeImpactId(
			Identity.Action.GetRunId(),
			Identity.Candidate.ActivationId,
			Identity.Candidate.DetectorId,
			Identity.Candidate.TargetEntityId,
			Identity.Candidate.HitOrdinal);
		return Identity;
	}

	FShanmenDefenseSnapshot MakeBaseDefense(bool bPreventBeforeShield = false)
	{
		FShanmenDefenseSnapshot Defense;
		Defense.TargetTags.AddTag(FShanmenCombatNativeTags::TargetLiving());
		if (bPreventBeforeShield)
		{
			FShanmenDefenseLayer& Evasion = Defense.Layers.AddDefaulted_GetRef();
			Evasion.LayerId = FGuid(
				0x25210001, 0x25210002, 0x25210003, 0x25210004);
			Evasion.RuleId = TEXT("Defense.Test.P25_2.Evasion");
			Evasion.Operation = EShanmenDefenseOperation::PreventAll;
			Evasion.Order = FShanmenDefenseOrder::Avoidance;
			Evasion.LayerTags.AddTag(FShanmenCombatNativeTags::DefenseEvade());
		}
		check(Defense.IsValid());
		return Defense;
	}

	FShanmenImpactRequest MakeImpactRequest(
		const FSpiritShieldProductImpactIdentity& Identity,
		const FShanmenDefenseSnapshot& Defense,
		float RawDamage)
	{
		FShanmenImpactRequest Request;
		Request.ImpactId = Identity.ImpactId;
		Request.Action = Identity.Action;
		Request.Candidate = Identity.Candidate;
		Request.Damage.FormulaId = TEXT("Combat.Formula.Test.P25_2Attack");
		Request.Damage.RawDamage = RawDamage;
		Request.Damage.DamageTags.AddTag(
			FShanmenCombatNativeTags::DamagePhysicalSlash());
		Request.TargetVitality.CurrentVitality = 100.0f;
		Request.TargetVitality.MaximumVitality = 100.0f;
		Request.TargetVitality.AuthorityRevision = 0;
		Request.Defense = Defense;
		check(Request.IsValid());
		return Request;
	}
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
	Fdemo_mapShanmenSpiritShieldInputFeedbackPresentation Feedback;
	TestTrue(TEXT("accepted activation projects immediate player feedback"),
		Fdemo_mapShanmenSpiritShieldInputFeedbackPresentation::TryProject(
			Activated, TEXT("H"), Feedback)
			&& Feedback.GetReason()
				== Edemo_mapShanmenSpiritShieldInputFeedbackReason::Activated
			&& Feedback.GetTone()
				== Edemo_mapShanmenSpiritShieldInputFeedbackTone::Success
			&& Feedback.GetDisplayText()
				== TEXT("SPIRIT SHIELD · ACTIVE"));

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSpiritShieldProductImpactCommitTest,
	"Shanmen.0_0_10.Product.SpiritShieldProductSession.ImpactCommitAndReplay",
	SpiritShieldProductFlags)

bool Fdemo_mapSpiritShieldProductImpactCommitTest::RunTest(const FString&)
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

	const FSpiritShieldProductImpactIdentity Identity = MakeImpactIdentity(
		Fixture.Coordinator.GetPlayerEntityId(), 0);
	const auto Defense = Session.TryComposeImpactDefense(
		Identity.ImpactId, Fixture.CaptureTimeline(), MakeBaseDefense());
	const FShanmenImpactRequest Request = MakeImpactRequest(
		Identity, Defense.Defense, 12.0f);
	const FShanmenImpactResult Resolution =
		FShanmenDefenseResolver::Resolve(Request);
	int32 DeliveryCount = 0;
	const auto Committed = Session.CommitImpact(
		Defense,
		Request,
		Resolution,
		[&DeliveryCount]()
		{
			++DeliveryCount;
			return true;
		});
	TestTrue(TEXT("triggered shield commits exact prevented damage"),
		Defense.IsSuccess()
			&& Committed.IsSuccess() && Committed.DidConsumeCapacity()
			&& Resolution.TriggeredLayers.Num() == 1
			&& FMath::IsNearlyEqual(Resolution.PreventedDamage, 12.0f)
			&& FMath::IsNearlyEqual(Resolution.FinalDamage, 0.0f)
			&& FMath::IsNearlyEqual(Session.GetAvailableCapacity(), 18.0f)
			&& Session.GetSession().GetCapacityAuthority().
				NumCommittedImpacts() == 1
			&& DeliveryCount == 1);

	const auto Replay = Session.CommitImpact(
		Defense,
		Request,
		Resolution,
		[&DeliveryCount]()
		{
			++DeliveryCount;
			return true;
		});
	TestTrue(TEXT("exact replay cannot consume capacity twice"),
		Replay.IsSuccess()
			&& Replay.Status
				== Edemo_mapShanmenSpiritShieldImpactCommitStatus::
					AlreadyCommitted
			&& FMath::IsNearlyEqual(Session.GetAvailableCapacity(), 18.0f)
			&& Session.GetSession().GetCapacityAuthority().
				NumCommittedImpacts() == 1
			&& DeliveryCount == 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSpiritShieldProductImpactAtomicFailureTest,
	"Shanmen.0_0_10.Product.SpiritShieldProductSession.ImpactDeliveryAtomicity",
	SpiritShieldProductFlags)

bool Fdemo_mapSpiritShieldProductImpactAtomicFailureTest::RunTest(
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
	const FSpiritShieldProductImpactIdentity Identity = MakeImpactIdentity(
		Fixture.Coordinator.GetPlayerEntityId(), 1);
	const auto Defense = Session.TryComposeImpactDefense(
		Identity.ImpactId, Fixture.CaptureTimeline(), MakeBaseDefense());
	const FShanmenImpactRequest Request = MakeImpactRequest(
		Identity, Defense.Defense, 10.0f);
	const FShanmenImpactResult Resolution =
		FShanmenDefenseResolver::Resolve(Request);

	const auto Rejected = Session.CommitImpact(
		Defense, Request, Resolution, []() { return false; });
	TestTrue(TEXT("rejected delivery publishes no staged capacity mutation"),
		Rejected.IsValid() && !Rejected.IsSuccess()
			&& Rejected.Error
				== Edemo_mapShanmenSpiritShieldImpactCommitError::DeliveryRejected
			&& FMath::IsNearlyEqual(Session.GetAvailableCapacity(), 30.0f)
			&& Session.GetSession().GetCapacityAuthority().
				NumCommittedImpacts() == 0);
	const auto Retry = Session.CommitImpact(
		Defense, Request, Resolution, []() { return true; });
	TestTrue(TEXT("same proof can commit after downstream recovery"),
		Retry.IsSuccess() && Retry.DidConsumeCapacity()
			&& FMath::IsNearlyEqual(Session.GetAvailableCapacity(), 20.0f)
			&& Session.GetSession().GetCapacityAuthority().
				NumCommittedImpacts() == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapSpiritShieldProductImpactOrderingTest,
	"Shanmen.0_0_10.Product.SpiritShieldProductSession.ImpactOrderingAndWindow",
	SpiritShieldProductFlags)

bool Fdemo_mapSpiritShieldProductImpactOrderingTest::RunTest(const FString&)
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
	const FSpiritShieldProductImpactIdentity Identity = MakeImpactIdentity(
		Fixture.Coordinator.GetPlayerEntityId(), 2);
	const auto Defense = Session.TryComposeImpactDefense(
		Identity.ImpactId,
		Fixture.CaptureTimeline(),
		MakeBaseDefense(true));
	const FShanmenImpactRequest Request = MakeImpactRequest(
		Identity, Defense.Defense, 8.0f);
	const FShanmenImpactResult Resolution =
		FShanmenDefenseResolver::Resolve(Request);
	const auto NotTriggered = Session.CommitImpact(
		Defense, Request, Resolution, []() { return true; });
	TestTrue(TEXT("earlier prevention leaves shield capacity untouched"),
		NotTriggered.IsSuccess()
			&& NotTriggered.Status
				== Edemo_mapShanmenSpiritShieldImpactCommitStatus::NotTriggered
			&& Resolution.TriggeredLayers.Num() == 1
			&& Resolution.TriggeredLayers[0].Operation
				== EShanmenDefenseOperation::PreventAll
			&& FMath::IsNearlyEqual(Session.GetAvailableCapacity(), 30.0f));

	Fdemo_mapShanmenCombatRunTimelineSample Foreign;
	check(Fdemo_mapShanmenCombatRunTimelineSample::TryCapture(
		FGuid(0x25220001, 0x25220002, 0x25220003, 0x25220004),
		0,
		Foreign));
	const auto ForeignRejected = Session.TryComposeImpactDefense(
		MakeImpactIdentity(Fixture.Coordinator.GetPlayerEntityId(), 3).ImpactId,
		Foreign,
		MakeBaseDefense());
	TestTrue(TEXT("foreign timeline fails closed"),
		ForeignRejected.IsValid() && !ForeignRejected.IsSuccess()
			&& ForeignRejected.Error
				== Edemo_mapShanmenSpiritShieldImpactDefenseError::
					TimelineMismatch);

	int64 AdvancedTicks = 0;
	check(Fixture.Timeline.TryAdvance(
		3.0, AdvancedTicks, Fixture.Diagnostic));
	const auto DueRejected = Session.TryComposeImpactDefense(
		MakeImpactIdentity(Fixture.Coordinator.GetPlayerEntityId(), 4).ImpactId,
		Fixture.CaptureTimeline(),
		MakeBaseDefense());
	TestTrue(TEXT("deadline sample cannot receive active shield defense"),
		DueRejected.IsValid() && !DueRejected.IsSuccess()
			&& DueRejected.Error
				== Edemo_mapShanmenSpiritShieldImpactDefenseError::
					OutsideActiveWindow);
	return true;
}

#endif
