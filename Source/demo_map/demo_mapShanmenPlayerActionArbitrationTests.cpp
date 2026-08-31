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

	Fdemo_mapShanmenPlayerActionOccupancySnapshot GuardOccupancy()
	{
		Fdemo_mapShanmenPlayerActionOccupancySnapshot Result;
		Result.bWeaponGuardActive = true;
		Result.WeaponGuardHostId = ArbitrationGuardHost;
		return Result;
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
		Edemo_mapShanmenPlayerActionKind::SpiritEvasion,
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
					&& Guarded.WeaponGuardHostId == ArbitrationGuardHost);
		}
	}

	Fdemo_mapShanmenPlayerActionOccupancySnapshot Thrown;
	Thrown.bThrownWeaponInFlight = true;
	Fdemo_mapShanmenPlayerActionOccupancySnapshot Spirit;
	Spirit.bSpiritEvasionBusy = true;
	for (const Edemo_mapShanmenPlayerActionKind Action : Actions)
	{
		const Fdemo_mapShanmenPlayerActionArbitrationReceipt DuringThrow =
			Evaluate(Sequence++, Action, Thrown);
		const Fdemo_mapShanmenPlayerActionArbitrationReceipt DuringSpirit =
			Evaluate(Sequence++, Action, Spirit);
		TestTrue(TEXT("in-flight thrown weapon rejects every new lane action"),
			DuringThrow.IsValid() && !DuringThrow.IsAuthorized()
				&& DuringThrow.Error
					== Edemo_mapShanmenPlayerActionArbitrationError::
						ConflictingProductActive);
		TestTrue(TEXT("nonterminal evasion rejects every new lane action"),
			DuringSpirit.IsValid() && !DuringSpirit.IsAuthorized()
				&& DuringSpirit.Error
					== Edemo_mapShanmenPlayerActionArbitrationError::
						ConflictingProductActive);
	}

	Fdemo_mapShanmenPlayerActionOccupancySnapshot Multiple = GuardOccupancy();
	Multiple.bThrownWeaponInFlight = true;
	const Fdemo_mapShanmenPlayerActionArbitrationReceipt Desynchronized =
		Evaluate(Sequence, Edemo_mapShanmenPlayerActionKind::BasicSword, Multiple);
	TestTrue(TEXT("multiple existing owners fail closed with replayable proof"),
		Desynchronized.IsValid() && !Desynchronized.IsAuthorized()
			&& Desynchronized.Error
				== Edemo_mapShanmenPlayerActionArbitrationError::
					MultipleActiveProducts);
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
			ArbitrationGuardHost);
	const Fdemo_mapShanmenPlayerActionGateResult Wrong =
		Fdemo_mapShanmenPlayerActionGateResult::FromGuardPreemption(
			Arbitration,
			FGuid(0xA1150004, 0, 0, 1));
	const Fdemo_mapShanmenPlayerActionGateResult Rejected =
		Fdemo_mapShanmenPlayerActionGateResult::RejectGuardPreemption(
			Arbitration,
			TEXT("Synthetic Host rejection."));
	TestTrue(TEXT("only the exact retired guard Host authorizes the action"),
		Exact.IsValid() && Exact.IsAuthorized()
			&& Exact.RetiredWeaponGuardHostId == ArbitrationGuardHost);
	TestTrue(TEXT("wrong retired Host remains a valid fail-closed receipt"),
		Wrong.IsValid() && !Wrong.IsAuthorized()
			&& !Wrong.RetiredWeaponGuardHostId.IsValid());
	TestTrue(TEXT("typed preemption failure remains auditable and rejected"),
		Rejected.IsValid() && !Rejected.IsAuthorized()
			&& Rejected.Error
				== Edemo_mapShanmenPlayerActionGateError::
					WeaponGuardPreemptionRejected);
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
	Invalid.bWeaponGuardActive = true;
	const Fdemo_mapShanmenPlayerActionArbitrationReceipt InvalidReceipt =
		Coordinator.TryAuthorizePlayerAction(
			Edemo_mapShanmenPlayerActionKind::ThrownWeapon,
			Invalid);
	const uint64 SequenceAfterInvalid =
		Coordinator.GetNextPlayerActionArbitrationSequence();
	Fdemo_mapShanmenPlayerActionOccupancySnapshot Busy;
	Busy.bSpiritEvasionBusy = true;
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
