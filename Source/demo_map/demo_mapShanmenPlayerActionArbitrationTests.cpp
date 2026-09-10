#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenPlayerActionArbitration.h"

#include "demo_mapCombatRunCoordinator.h"
#include "demo_mapPlayerHealthComponent.h"
#include "GameFramework/Pawn.h"
#include "Misc/AutomationTest.h"

namespace
{
	constexpr EAutomationTestFlags ArbitrationFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;
	const FGuid ArbitrationRun(0xA1150001, 0, 0, 1);
	const FGuid ArbitrationPlayer(0xA1150002, 0, 0, 1);
	const FGuid ArbitrationGuardHost(0xA1150003, 0, 0, 1);
	const FGuid ArbitrationThrownOwner(0xA1160001, 0, 0, 1);
	const FGuid ArbitrationSpiritHost(0xA1160002, 0, 0, 1);
	const FGuid ArbitrationSwordQiOwner(0xA1160003, 0, 0, 1);

	Fdemo_mapShanmenPlayerActionOccupancySnapshot OccupancyWith(
		const Edemo_mapShanmenPlayerActionKind Action,
		const FGuid& OwnerId,
		const Edemo_mapShanmenPlayerActionClaimPreemption Preemption)
	{
		Fdemo_mapShanmenPlayerActionOccupancySnapshot Result;
		Result.TryRegisterClaim(Action, OwnerId, Preemption);
		return Result;
	}

	Fdemo_mapShanmenPlayerActionOccupancySnapshot GuardOccupancy()
	{
		return OccupancyWith(
			Edemo_mapShanmenPlayerActionKind::WeaponGuard,
			ArbitrationGuardHost,
			Edemo_mapShanmenPlayerActionClaimPreemption::ExactOwner);
	}

	Fdemo_mapShanmenPlayerActionArbitrationReceipt Evaluate(
		const uint64 Sequence,
		const Edemo_mapShanmenPlayerActionKind Action,
		const Fdemo_mapShanmenPlayerActionOccupancySnapshot& Occupancy)
	{
		return Fdemo_mapShanmenPlayerActionArbitrationPolicy::Evaluate(
			ArbitrationRun,
			ArbitrationPlayer,
			Sequence,
			Action,
			Occupancy);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapPlayerActionArbitrationPolicyTest,
	"Shanmen.0_0_10.Product.PlayerActionArbitration.PolicyMatrix",
	ArbitrationFlags)

bool Fdemo_mapPlayerActionArbitrationPolicyTest::RunTest(const FString&)
{
	TestFalse(TEXT("default arbitration receipt is explicitly absent"),
		Fdemo_mapShanmenPlayerActionArbitrationReceipt().IsValid());
	TestFalse(TEXT("default action gate is explicitly absent"),
		Fdemo_mapShanmenPlayerActionGateResult().IsValid());

	const TArray<Edemo_mapShanmenPlayerActionKind> Actions = {
		Edemo_mapShanmenPlayerActionKind::BasicSword,
		Edemo_mapShanmenPlayerActionKind::ThrownWeapon,
		Edemo_mapShanmenPlayerActionKind::SwordQi,
		Edemo_mapShanmenPlayerActionKind::SpiritEvasion,
		Edemo_mapShanmenPlayerActionKind::SpiritShield,
		Edemo_mapShanmenPlayerActionKind::WeaponGuard
	};
	uint64 Sequence = 1;
	for (const Edemo_mapShanmenPlayerActionKind Action : Actions)
	{
		const Fdemo_mapShanmenPlayerActionArbitrationReceipt Granted =
			Evaluate(
				Sequence++,
				Action,
				Fdemo_mapShanmenPlayerActionOccupancySnapshot());
		TestTrue(TEXT("empty lane grants every typed player action"),
			Granted.IsAuthorized()
				&& Granted.Status
					== Edemo_mapShanmenPlayerActionArbitrationStatus::Granted);
	}

	const Fdemo_mapShanmenPlayerActionArbitrationReceipt ReplayedA =
		Evaluate(80, Edemo_mapShanmenPlayerActionKind::BasicSword,
			Fdemo_mapShanmenPlayerActionOccupancySnapshot());
	const Fdemo_mapShanmenPlayerActionArbitrationReceipt ReplayedB =
		Evaluate(80, Edemo_mapShanmenPlayerActionKind::BasicSword,
			Fdemo_mapShanmenPlayerActionOccupancySnapshot());
	TestTrue(TEXT("equal canonical command inputs replay one identity"),
		ReplayedA.IsValid()
			&& ReplayedA.CommandId == ReplayedB.CommandId);

	for (const Edemo_mapShanmenPlayerActionKind Action : Actions)
	{
		const Fdemo_mapShanmenPlayerActionArbitrationReceipt Guarded =
			Evaluate(Sequence++, Action, GuardOccupancy());
		if (Action == Edemo_mapShanmenPlayerActionKind::WeaponGuard)
		{
			TestTrue(TEXT("same guard remains idempotently active"),
				Guarded.IsAuthorized()
					&& Guarded.Status
						== Edemo_mapShanmenPlayerActionArbitrationStatus::
							AlreadyActive
					&& !Guarded.RequiresWeaponGuardPreemption());
		}
		else
		{
			TestTrue(TEXT("another action requires exact guard preemption"),
				Guarded.RequiresWeaponGuardPreemption()
					&& Guarded.OccupyingAction
						== Edemo_mapShanmenPlayerActionKind::WeaponGuard
					&& Guarded.OccupyingOwnerId == ArbitrationGuardHost);
		}
	}

	const Fdemo_mapShanmenPlayerActionOccupancySnapshot Thrown =
		OccupancyWith(
			Edemo_mapShanmenPlayerActionKind::ThrownWeapon,
			ArbitrationThrownOwner,
			Edemo_mapShanmenPlayerActionClaimPreemption::None);
	const Fdemo_mapShanmenPlayerActionOccupancySnapshot Spirit =
		OccupancyWith(
			Edemo_mapShanmenPlayerActionKind::SpiritEvasion,
			ArbitrationSpiritHost,
			Edemo_mapShanmenPlayerActionClaimPreemption::None);
	const Fdemo_mapShanmenPlayerActionOccupancySnapshot SwordQi =
		OccupancyWith(
			Edemo_mapShanmenPlayerActionKind::SwordQi,
			ArbitrationSwordQiOwner,
			Edemo_mapShanmenPlayerActionClaimPreemption::None);
	for (const Edemo_mapShanmenPlayerActionKind Action : Actions)
	{
		const Fdemo_mapShanmenPlayerActionArbitrationReceipt DuringThrow =
			Evaluate(Sequence++, Action, Thrown);
		const Fdemo_mapShanmenPlayerActionArbitrationReceipt DuringSpirit =
			Evaluate(Sequence++, Action, Spirit);
		const Fdemo_mapShanmenPlayerActionArbitrationReceipt DuringSwordQi =
			Evaluate(Sequence++, Action, SwordQi);
		TestTrue(TEXT("in-flight thrown weapon rejects every new lane action"),
			DuringThrow.IsValid() && !DuringThrow.IsAuthorized()
				&& DuringThrow.Error
					== Edemo_mapShanmenPlayerActionArbitrationError::
						ConflictingProductActive
				&& DuringThrow.OccupyingAction
					== Edemo_mapShanmenPlayerActionKind::ThrownWeapon
				&& DuringThrow.OccupyingOwnerId
					== ArbitrationThrownOwner);
		TestTrue(TEXT("nonterminal evasion rejects every new lane action"),
			DuringSpirit.IsValid() && !DuringSpirit.IsAuthorized()
				&& DuringSpirit.Error
					== Edemo_mapShanmenPlayerActionArbitrationError::
						ConflictingProductActive
				&& DuringSpirit.OccupyingAction
					== Edemo_mapShanmenPlayerActionKind::SpiritEvasion
				&& DuringSpirit.OccupyingOwnerId
					== ArbitrationSpiritHost);
		TestTrue(TEXT("in-flight Sword Qi rejects every new lane action"),
			DuringSwordQi.IsValid() && !DuringSwordQi.IsAuthorized()
				&& DuringSwordQi.Error
					== Edemo_mapShanmenPlayerActionArbitrationError::
						ConflictingProductActive
				&& DuringSwordQi.OccupyingAction
					== Edemo_mapShanmenPlayerActionKind::SwordQi
				&& DuringSwordQi.OccupyingOwnerId
					== ArbitrationSwordQiOwner);
	}

	Fdemo_mapShanmenPlayerActionOccupancySnapshot Multiple = GuardOccupancy();
	TestTrue(TEXT("a distinct product claim registers into the projection"),
		Multiple.TryRegisterClaim(
			Edemo_mapShanmenPlayerActionKind::ThrownWeapon,
			ArbitrationThrownOwner,
			Edemo_mapShanmenPlayerActionClaimPreemption::None));
	const Fdemo_mapShanmenPlayerActionArbitrationReceipt Desynchronized =
		Evaluate(Sequence, Edemo_mapShanmenPlayerActionKind::BasicSword, Multiple);
	TestTrue(TEXT("multiple existing owners fail closed with replayable proof"),
		Desynchronized.IsValid() && !Desynchronized.IsAuthorized()
			&& Desynchronized.Error
				== Edemo_mapShanmenPlayerActionArbitrationError::
					MultipleActiveProducts
			&& Desynchronized.OccupyingAction
				== Edemo_mapShanmenPlayerActionKind::None
			&& !Desynchronized.OccupyingOwnerId.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapPlayerActionClaimProjectionTest,
	"Shanmen.0_0_10.Product.PlayerActionArbitration.TypedClaimProjection",
	ArbitrationFlags)

bool Fdemo_mapPlayerActionClaimProjectionTest::RunTest(const FString&)
{
	Fdemo_mapShanmenPlayerActionClaim Guard;
	TestTrue(TEXT("guard claim requires exact-owner preemption"),
		Fdemo_mapShanmenPlayerActionClaim::TryCreate(
			Edemo_mapShanmenPlayerActionKind::WeaponGuard,
			ArbitrationGuardHost,
			Edemo_mapShanmenPlayerActionClaimPreemption::ExactOwner,
			Guard)
			&& Guard.RequiresExactOwnerPreemption());
	Fdemo_mapShanmenPlayerActionClaim InvalidGuard;
	TestFalse(TEXT("guard cannot silently become non-preemptible"),
		Fdemo_mapShanmenPlayerActionClaim::TryCreate(
			Edemo_mapShanmenPlayerActionKind::WeaponGuard,
			ArbitrationGuardHost,
			Edemo_mapShanmenPlayerActionClaimPreemption::None,
			InvalidGuard));
	Fdemo_mapShanmenPlayerActionClaim InvalidThrown;
	TestFalse(TEXT("thrown weapon cannot silently become preemptible"),
		Fdemo_mapShanmenPlayerActionClaim::TryCreate(
			Edemo_mapShanmenPlayerActionKind::ThrownWeapon,
			ArbitrationThrownOwner,
			Edemo_mapShanmenPlayerActionClaimPreemption::ExactOwner,
			InvalidThrown));
	Fdemo_mapShanmenPlayerActionClaim InvalidSwordQi;
	TestFalse(TEXT("Sword Qi cannot silently become preemptible"),
		Fdemo_mapShanmenPlayerActionClaim::TryCreate(
			Edemo_mapShanmenPlayerActionKind::SwordQi,
			ArbitrationSwordQiOwner,
			Edemo_mapShanmenPlayerActionClaimPreemption::ExactOwner,
			InvalidSwordQi));

	Fdemo_mapShanmenPlayerActionOccupancySnapshot Duplicate;
	TestTrue(TEXT("first typed claim registers"),
		Duplicate.TryRegisterClaim(
			Edemo_mapShanmenPlayerActionKind::SpiritEvasion,
			ArbitrationSpiritHost,
			Edemo_mapShanmenPlayerActionClaimPreemption::None));
	TestFalse(TEXT("duplicate product claim fails registration"),
		Duplicate.TryRegisterClaim(
			Edemo_mapShanmenPlayerActionKind::SpiritEvasion,
			FGuid(0xA1160003, 0, 0, 1),
			Edemo_mapShanmenPlayerActionClaimPreemption::None));
	TestFalse(TEXT("duplicate projection remains structurally invalid"),
		Duplicate.IsValid());

	Fdemo_mapShanmenPlayerActionOccupancySnapshot InvalidIdentity;
	TestFalse(TEXT("claim without owner identity fails closed"),
		InvalidIdentity.TryRegisterClaim(
			Edemo_mapShanmenPlayerActionKind::ThrownWeapon,
			FGuid(),
			Edemo_mapShanmenPlayerActionClaimPreemption::None));
	TestFalse(TEXT("failed identity poisons the complete projection"),
		InvalidIdentity.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapPlayerActionGateProofTest,
	"Shanmen.0_0_10.Product.PlayerActionArbitration.GuardPreemptionProof",
	ArbitrationFlags)

bool Fdemo_mapPlayerActionGateProofTest::RunTest(const FString&)
{
	const Fdemo_mapShanmenPlayerActionArbitrationReceipt Arbitration =
		Evaluate(
			1,
			Edemo_mapShanmenPlayerActionKind::BasicSword,
			GuardOccupancy());
	const Fdemo_mapShanmenPlayerActionGateResult Exact =
		Fdemo_mapShanmenPlayerActionGateResult::FromGuardPreemption(
			Arbitration,
			ArbitrationGuardHost,
			Fdemo_mapShanmenPlayerActionOccupancySnapshot());
	const Fdemo_mapShanmenPlayerActionGateResult Wrong =
		Fdemo_mapShanmenPlayerActionGateResult::FromGuardPreemption(
			Arbitration,
			FGuid(0xA1150004, 0, 0, 1),
			Fdemo_mapShanmenPlayerActionOccupancySnapshot());
	const Fdemo_mapShanmenPlayerActionGateResult Rejected =
		Fdemo_mapShanmenPlayerActionGateResult::RejectGuardPreemption(
			Arbitration,
			TEXT("Synthetic Host rejection."));
	const Fdemo_mapShanmenPlayerActionGateResult OccupiedAfterRetirement =
		Fdemo_mapShanmenPlayerActionGateResult::FromGuardPreemption(
			Arbitration,
			ArbitrationGuardHost,
			OccupancyWith(
				Edemo_mapShanmenPlayerActionKind::ThrownWeapon,
				ArbitrationThrownOwner,
				Edemo_mapShanmenPlayerActionClaimPreemption::None));
	Fdemo_mapShanmenPlayerActionOccupancySnapshot InvalidAfterRetirement;
	InvalidAfterRetirement.Invalidate();
	const Fdemo_mapShanmenPlayerActionGateResult InvalidAfterRetirementResult =
		Fdemo_mapShanmenPlayerActionGateResult::FromGuardPreemption(
			Arbitration,
			ArbitrationGuardHost,
			InvalidAfterRetirement);
	TestTrue(TEXT("only the exact retired guard Host authorizes the action"),
		Exact.IsValid() && Exact.IsAuthorized()
			&& Exact.RetiredWeaponGuardHostId == ArbitrationGuardHost
			&& Exact.PostPreemptionObservation
				== Edemo_mapShanmenPlayerActionPostPreemptionObservation::Empty);
	TestTrue(TEXT("wrong retired Host remains a valid fail-closed receipt"),
		Wrong.IsValid() && !Wrong.IsAuthorized()
			&& !Wrong.RetiredWeaponGuardHostId.IsValid());
	TestTrue(TEXT("typed preemption failure remains auditable and rejected"),
		Rejected.IsValid() && !Rejected.IsAuthorized()
			&& Rejected.Error
				== Edemo_mapShanmenPlayerActionGateError::
					WeaponGuardPreemptionRejected);
	TestTrue(TEXT("a new owner observed after exact retirement rejects the action"),
		OccupiedAfterRetirement.IsValid()
			&& !OccupiedAfterRetirement.IsAuthorized()
			&& OccupiedAfterRetirement.RetiredWeaponGuardHostId
				== ArbitrationGuardHost
			&& OccupiedAfterRetirement.Error
				== Edemo_mapShanmenPlayerActionGateError::
					PostPreemptionLaneOccupied
			&& OccupiedAfterRetirement.PostPreemptionObservation
				== Edemo_mapShanmenPlayerActionPostPreemptionObservation::
					Occupied);
	TestTrue(TEXT("an invalid post-retirement projection rejects the action"),
		InvalidAfterRetirementResult.IsValid()
			&& !InvalidAfterRetirementResult.IsAuthorized()
			&& InvalidAfterRetirementResult.RetiredWeaponGuardHostId
				== ArbitrationGuardHost
			&& InvalidAfterRetirementResult.Error
				== Edemo_mapShanmenPlayerActionGateError::
					PostPreemptionProjectionInvalid
			&& InvalidAfterRetirementResult.PostPreemptionObservation
				== Edemo_mapShanmenPlayerActionPostPreemptionObservation::
					Invalid);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapPlayerActionCoordinatorIdentityTest,
	"Shanmen.0_0_10.Product.PlayerActionArbitration.CoordinatorIdentity",
	ArbitrationFlags)

bool Fdemo_mapPlayerActionCoordinatorIdentityTest::RunTest(const FString&)
{
	APawn* Pawn = NewObject<APawn>(GetTransientPackage());
	Udemo_mapPlayerHealthComponent* Health = Pawn
		? NewObject<Udemo_mapPlayerHealthComponent>(Pawn, TEXT("P1115Health"))
		: nullptr;
	Fdemo_mapCombatRunCoordinator Coordinator;
	FString Diagnostic;
	if (!Pawn || !Health
		|| !Coordinator.TryBeginRun(
			ArbitrationRun,
			Pawn,
			Health,
			Diagnostic))
	{
		AddError(FString::Printf(
			TEXT("P11.15 coordinator fixture failed: %s"),
			*Diagnostic));
		return false;
	}

	const Fdemo_mapShanmenPlayerActionArbitrationReceipt Granted =
		Coordinator.TryAuthorizePlayerAction(
			Edemo_mapShanmenPlayerActionKind::BasicSword,
			Fdemo_mapShanmenPlayerActionOccupancySnapshot());
	Fdemo_mapShanmenPlayerActionOccupancySnapshot Invalid;
	Invalid.Invalidate();
	const Fdemo_mapShanmenPlayerActionArbitrationReceipt InvalidReceipt =
		Coordinator.TryAuthorizePlayerAction(
			Edemo_mapShanmenPlayerActionKind::ThrownWeapon,
			Invalid);
	const uint64 SequenceAfterInvalid =
		Coordinator.GetNextPlayerActionArbitrationSequence();
	const Fdemo_mapShanmenPlayerActionOccupancySnapshot Busy =
		OccupancyWith(
			Edemo_mapShanmenPlayerActionKind::SpiritEvasion,
			ArbitrationSpiritHost,
			Edemo_mapShanmenPlayerActionClaimPreemption::None);
	const Fdemo_mapShanmenPlayerActionArbitrationReceipt Conflict =
		Coordinator.TryAuthorizePlayerAction(
			Edemo_mapShanmenPlayerActionKind::WeaponGuard,
			Busy);
	TestTrue(TEXT("grant consumes deterministic sequence one"),
		Granted.IsAuthorized() && Granted.CommandSequence == 1);
	TestTrue(TEXT("malformed occupancy consumes no identity"),
		InvalidReceipt.IsValid() && !InvalidReceipt.CommandId.IsValid()
			&& SequenceAfterInvalid == 2);
	TestTrue(TEXT("policy conflict consumes replayable sequence two"),
		Conflict.IsValid() && !Conflict.IsAuthorized()
			&& Conflict.CommandSequence == 2);
	TestTrue(TEXT("Run release resets the action command sequence"),
		Coordinator.TryEndRun(ArbitrationRun, Diagnostic)
			&& Coordinator.GetNextPlayerActionArbitrationSequence() == 1);
	return true;
}

#endif
