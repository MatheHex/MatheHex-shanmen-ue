#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ShanmenCombatResolver.h"

#include <limits>

namespace
{
	FShanmenImpactRequest MakeValidRequest(float IncomingDamage = 100.0f)
	{
		FShanmenImpactRequest Request;
		Request.Action.RunId = FGuid(1, 2, 3, 4);
		Request.Action.OwnerId = FGuid(5, 6, 7, 8);
		Request.Action.SourceEntityId = FGuid(9, 10, 11, 12);
		Request.Action.ActionDefinitionId = TEXT("Combat.Action.Sword.Basic01");
		Request.Action.Content.Version = TEXT("0.0.10.P0");
		Request.Action.Content.Digest = TEXT("TEST-DIGEST");
		Request.Action.BasePower = IncomingDamage;
		Request.Action.ActivationId = FShanmenCombatIdFactory::MakeActivationId(
			Request.Action.RunId,
			Request.Action.SourceEntityId,
			Request.Action.ActionDefinitionId,
			1);

		Request.Candidate.ActivationId = Request.Action.ActivationId;
		Request.Candidate.SourceEntityId = Request.Action.SourceEntityId;
		Request.Candidate.TargetEntityId = FGuid(13, 14, 15, 16);
		Request.Candidate.DetectorId = TEXT("Detector.Weapon.Main");
		Request.Candidate.DetectorKind = EShanmenHitDetectorKind::WeaponTrajectory;
		Request.Candidate.HitOrdinal = 0;
		Request.ImpactId = FShanmenCombatIdFactory::MakeImpactId(
			Request.Action.RunId,
			Request.Action.ActivationId,
			Request.Candidate.DetectorId,
			Request.Candidate.TargetEntityId,
			Request.Candidate.HitOrdinal);
		Request.IncomingDamage = IncomingDamage;
		return Request;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenCombatDeterministicIdentityTest,
	"Shanmen.0_0_10.CombatCore.DeterministicIdentity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenCombatDeterministicIdentityTest::RunTest(const FString&)
{
	const FShanmenImpactRequest Request = MakeValidRequest();
	const FGuid SameImpact = FShanmenCombatIdFactory::MakeImpactId(
		Request.Action.RunId,
		Request.Action.ActivationId,
		Request.Candidate.DetectorId,
		Request.Candidate.TargetEntityId,
		Request.Candidate.HitOrdinal);
	const FGuid DifferentOrdinal = FShanmenCombatIdFactory::MakeImpactId(
		Request.Action.RunId,
		Request.Action.ActivationId,
		Request.Candidate.DetectorId,
		Request.Candidate.TargetEntityId,
		Request.Candidate.HitOrdinal + 1);

	TestTrue(TEXT("Canonical impact id is valid"), Request.ImpactId.IsValid());
	TestTrue(TEXT("Same canonical values replay the same id"), Request.ImpactId == SameImpact);
	TestTrue(TEXT("Hit ordinal participates in identity"), Request.ImpactId != DifferentOrdinal);

	FShanmenHitCandidate SelfCandidate = Request.Candidate;
	SelfCandidate.TargetEntityId = SelfCandidate.SourceEntityId;
	TestTrue(TEXT("Candidate remains geometric; target policy decides whether self-targeting is legal"), SelfCandidate.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenCombatImpactLedgerTest,
	"Shanmen.0_0_10.CombatCore.ImpactLedger",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenCombatImpactLedgerTest::RunTest(const FString&)
{
	const FGuid ImpactId = MakeValidRequest().ImpactId;
	FShanmenImpactLedger Ledger;

	TestTrue(TEXT("First impact is accepted"), Ledger.TryAccept(ImpactId));
	TestFalse(TEXT("Replay is rejected"), Ledger.TryAccept(ImpactId));
	TestFalse(TEXT("Invalid sentinel is rejected"), Ledger.TryAccept(FGuid()));
	TestEqual(TEXT("Only one accepted identity is retained"), Ledger.Num(), 1);
	TestTrue(TEXT("Accepted identity can be queried"), Ledger.Contains(ImpactId));
	Ledger.Reset();
	TestEqual(TEXT("Run boundary clears the ledger"), Ledger.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenCombatDefenseOrderTest,
	"Shanmen.0_0_10.CombatCore.DefenseOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenCombatDefenseOrderTest::RunTest(const FString&)
{
	FShanmenImpactRequest Request = MakeValidRequest(100.0f);
	Request.Defense.bGuardActive = true;
	Request.Defense.GuardReductionFraction = 0.25f;
	Request.Defense.ShieldPoints = 10.0f;
	Request.Defense.ArmorResistanceFraction = 0.20f;

	const FShanmenImpactResult Result = FShanmenDefenseResolver::Resolve(Request);
	TestTrue(TEXT("Valid impact is accepted"), Result.bAccepted);
	TestTrue(TEXT("Layered defense reports mitigation"), Result.Outcome == EShanmenDefenseOutcome::Mitigated);
	TestTrue(TEXT("Guard resolves first"), FMath::IsNearlyEqual(Result.GuardPrevented, 25.0f));
	TestTrue(TEXT("Shield resolves second"), FMath::IsNearlyEqual(Result.ShieldAbsorbed, 10.0f));
	TestTrue(TEXT("Armor resolves after shield"), FMath::IsNearlyEqual(Result.ArmorPrevented, 13.0f));
	TestTrue(TEXT("Final vitality damage remains auditable"), FMath::IsNearlyEqual(Result.FinalDamage, 52.0f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenCombatActiveDefensePrecedenceTest,
	"Shanmen.0_0_10.CombatCore.ActiveDefensePrecedence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenCombatActiveDefensePrecedenceTest::RunTest(const FString&)
{
	FShanmenImpactRequest Dodge = MakeValidRequest();
	Dodge.Defense.bDodgeWindowActive = true;
	Dodge.Defense.bPerfectGuardWindowActive = true;
	Dodge.Defense.ShieldPoints = 1000.0f;
	const FShanmenImpactResult DodgeResult = FShanmenDefenseResolver::Resolve(Dodge);
	TestTrue(TEXT("Dodge has highest precedence"), DodgeResult.Outcome == EShanmenDefenseOutcome::Evaded);
	TestTrue(TEXT("Dodge reaches zero vitality damage"), FMath::IsNearlyZero(DodgeResult.FinalDamage));

	FShanmenImpactRequest PerfectGuard = MakeValidRequest();
	PerfectGuard.Defense.bPerfectGuardWindowActive = true;
	PerfectGuard.Defense.bGuardActive = true;
	PerfectGuard.Defense.GuardReductionFraction = 0.5f;
	const FShanmenImpactResult GuardResult = FShanmenDefenseResolver::Resolve(PerfectGuard);
	TestTrue(TEXT("Perfect guard precedes ordinary guard"), GuardResult.Outcome == EShanmenDefenseOutcome::PerfectGuarded);
	TestTrue(TEXT("Perfect guard records complete prevention"), FMath::IsNearlyEqual(GuardResult.GuardPrevented, 100.0f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShanmenCombatInvalidRequestTest,
	"Shanmen.0_0_10.CombatCore.InvalidRequestFailsClosed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShanmenCombatInvalidRequestTest::RunTest(const FString&)
{
	FShanmenImpactRequest Request = MakeValidRequest();
	Request.IncomingDamage = std::numeric_limits<float>::quiet_NaN();
	const FShanmenImpactResult Result = FShanmenDefenseResolver::Resolve(Request);
	TestFalse(TEXT("Invalid numeric input is not accepted"), Result.bAccepted);
	TestTrue(TEXT("Invalid request remains explicit"), Result.Outcome == EShanmenDefenseOutcome::Invalid);
	TestTrue(TEXT("Invalid request cannot apply vitality damage"), FMath::IsNearlyZero(Result.FinalDamage));
	return true;
}

#endif
