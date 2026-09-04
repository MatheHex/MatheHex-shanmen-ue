#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenThrownWeaponArcConfirmationOwner.h"

#include "demo_mapPlayerController.h"

#include "Misc/AutomationTest.h"

namespace
{
	using EConfirmationStatus =
		Edemo_mapShanmenThrownWeaponArcConfirmationStatus;
	using ETrajectory =
		Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind;

	FGuid MakeEventId(const uint32 Value)
	{
		return FGuid(0xAC000000u | Value, Value + 1, Value + 2, Value + 3);
	}

	Fdemo_mapShanmenThrownWeaponArcChoicePolicy MakePolicy(
		const double MaximumForwardDistance = 1200.0)
	{
		Fdemo_mapShanmenThrownWeaponArcChoicePolicy Policy;
		check(Fdemo_mapShanmenThrownWeaponArcChoicePolicy::TryCapture(
			400.0,
			MaximumForwardDistance,
			300.0,
			100.0,
			500.0,
			Policy));
		return Policy;
	}

	Fdemo_mapShanmenThrownWeaponInputChoiceState MakeArcChoice()
	{
		Fdemo_mapShanmenThrownWeaponInputChoiceState State =
			Fdemo_mapShanmenThrownWeaponInputChoiceState::CreateInitial();
		Fdemo_mapShanmenThrownWeaponInputChoiceCommand Command;
		check(Fdemo_mapShanmenThrownWeaponInputChoiceCommand::
			TryCaptureTrajectorySelection(
				0, ETrajectory::BallisticArc, Command));
		auto Reduced =
			Fdemo_mapShanmenThrownWeaponInputChoiceReducer::Reduce(
				State, Command);
		check(Reduced.DidChange());
		State = Reduced.State;
		check(Fdemo_mapShanmenThrownWeaponInputChoiceCommand::
			TryCaptureArcTargetIntent(
				1, FVector2D(0.5, 0.25), Command));
		Reduced = Fdemo_mapShanmenThrownWeaponInputChoiceReducer::Reduce(
			State, Command);
		check(Reduced.DidChange());
		return Reduced.State;
	}

	Fdemo_mapShanmenThrownWeaponArcConfirmationIntent MakeIntent(
		const uint32 EventValue = 1,
		const int32 HotbarSlotNumber = 2,
		const Fdemo_mapShanmenThrownWeaponArcChoicePolicy& Policy =
			MakePolicy())
	{
		Fdemo_mapShanmenThrownWeaponArcConfirmationIntent Intent;
		check(Fdemo_mapShanmenThrownWeaponArcConfirmationIntent::TryCapture(
			MakeEventId(EventValue),
			HotbarSlotNumber,
			Policy,
			Intent));
		return Intent;
	}

	Fdemo_mapShanmenThrownWeaponArcLaunchInputResult MakeLaunchInput(
		const Fdemo_mapShanmenThrownWeaponArcLaunchCommand& Command)
	{
		return Fdemo_mapShanmenThrownWeaponArcLaunchInputAdapter::Route(
			Command,
			true,
			true,
			true,
			[]() { return true; },
			[&Command]() { return Command.GetChoiceState(); },
			[](int32,
				const Fdemo_mapShanmenThrownWeaponArcChoicePolicy&)
			{
				return Fdemo_mapShanmenThrownWeaponArcSourceBasisRouteResult();
			});
	}

	struct FConfirmationProbe
	{
		Fdemo_mapShanmenThrownWeaponInputChoiceState Choice = MakeArcChoice();
		int32 ChoiceReadCount = 0;
		int32 LaunchRouteCount = 0;
		Fdemo_mapShanmenThrownWeaponArcLaunchCommand CapturedCommand;
		bool bReturnInvalidInput = false;
		bool bReturnMismatchedInput = false;

		Fdemo_mapShanmenThrownWeaponArcConfirmationResult Confirm(
			Fdemo_mapShanmenThrownWeaponArcConfirmationOwner& Owner,
			const Fdemo_mapShanmenThrownWeaponArcConfirmationIntent& Intent,
			const bool bAllowed = true)
		{
			return Owner.Confirm(
				Intent,
				bAllowed,
				[this]()
				{
					++ChoiceReadCount;
					return Choice;
				},
				[this](
					const Fdemo_mapShanmenThrownWeaponArcLaunchCommand& Command)
				{
					++LaunchRouteCount;
					CapturedCommand = Command;
					if (bReturnInvalidInput)
					{
						return Fdemo_mapShanmenThrownWeaponArcLaunchInputResult();
					}
					if (bReturnMismatchedInput)
					{
						Fdemo_mapShanmenThrownWeaponArcLaunchCommand Other;
						check(Fdemo_mapShanmenThrownWeaponArcLaunchCommand::
							TryCapture(
								Command.GetHotbarSlotNumber() == 1 ? 2 : 1,
								Command.GetChoiceState(),
								Command.GetPolicy(),
								Other));
						return MakeLaunchInput(Other);
					}
					return MakeLaunchInput(Command);
				});
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcConfirmationIntentContractTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcConfirmationOwner.IntentContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcConfirmationIntentContractTest::RunTest(
	const FString&)
{
	const auto Policy = MakePolicy();
	const auto First = MakeIntent(1, 2, Policy);
	const auto Replay = MakeIntent(1, 2, Policy);
	const auto OtherEvent = MakeIntent(2, 2, Policy);
	const auto OtherSlot = MakeIntent(1, 3, Policy);
	const auto OtherPolicy = MakeIntent(1, 2, MakePolicy(1300.0));
	TestTrue(TEXT("Equal canonical confirmation fields reproduce identity"),
		First.IsValid()
			&& First.Matches(Replay)
			&& First.GetIntentId() == Replay.GetIntentId()
			&& First.GetConfirmationEventId() == MakeEventId(1)
			&& First.GetHotbarSlotNumber() == 2
			&& First.GetPolicy().Matches(Policy));
	TestTrue(TEXT("Event, slot, and policy each participate in identity"),
		First.GetIntentId() != OtherEvent.GetIntentId()
			&& First.GetIntentId() != OtherSlot.GetIntentId()
			&& First.GetIntentId() != OtherPolicy.GetIntentId());

	Fdemo_mapShanmenThrownWeaponArcConfirmationIntent Rejected;
	TestFalse(TEXT("Invalid event identity fails closed"),
		Fdemo_mapShanmenThrownWeaponArcConfirmationIntent::TryCapture(
			FGuid(), 2, Policy, Rejected));
	TestFalse(TEXT("Slot zero fails closed"),
		Fdemo_mapShanmenThrownWeaponArcConfirmationIntent::TryCapture(
			MakeEventId(3), 0, Policy, Rejected));
	TestFalse(TEXT("Slot ten fails closed"),
		Fdemo_mapShanmenThrownWeaponArcConfirmationIntent::TryCapture(
			MakeEventId(3), 10, Policy, Rejected));
	TestFalse(TEXT("Invalid policy fails closed"),
		Fdemo_mapShanmenThrownWeaponArcConfirmationIntent::TryCapture(
			MakeEventId(3),
			2,
			Fdemo_mapShanmenThrownWeaponArcChoicePolicy(),
			Rejected));
	TestFalse(TEXT("Failed capture leaves no reusable intent"),
		Rejected.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcConfirmationCaptureGateTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcConfirmationOwner.CaptureGates",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcConfirmationCaptureGateTest::RunTest(
	const FString&)
{
	Fdemo_mapShanmenThrownWeaponArcConfirmationOwner Owner;
	FConfirmationProbe Probe;
	const auto Invalid = Probe.Confirm(
		Owner,
		Fdemo_mapShanmenThrownWeaponArcConfirmationIntent());
	const auto Blocked = Probe.Confirm(Owner, MakeIntent(2), false);
	Probe.Choice = Fdemo_mapShanmenThrownWeaponInputChoiceState();
	const auto ChoiceInvalid = Probe.Confirm(Owner, MakeIntent(3));
	Probe.Choice =
		Fdemo_mapShanmenThrownWeaponInputChoiceState::CreateInitial();
	const auto CaptureRejected = Probe.Confirm(Owner, MakeIntent(4));

	TestTrue(TEXT("Invalid intent and local context stop before choice read"),
		Invalid.IsValid()
			&& Invalid.GetStatus() == EConfirmationStatus::IntentInvalid
			&& Blocked.IsValid()
			&& Blocked.GetStatus()
				== EConfirmationStatus::ConfirmationBlocked
			&& Blocked.GetChoiceStateReadCount() == 0
			&& Blocked.GetLaunchRouteInvocationCount() == 0);
	TestTrue(TEXT("Invalid and non-Arc choices stop after exactly one read"),
		ChoiceInvalid.IsValid()
			&& ChoiceInvalid.GetStatus()
				== EConfirmationStatus::ChoiceStateInvalid
			&& CaptureRejected.IsValid()
			&& CaptureRejected.GetStatus()
				== EConfirmationStatus::CommandCaptureRejected
			&& Probe.ChoiceReadCount == 2
			&& Probe.LaunchRouteCount == 0);
	TestTrue(TEXT("Only valid explicit events enter bounded ownership"),
		Owner.IsValid() && Owner.NumRetainedEvents() == 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcConfirmationReplayTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcConfirmationOwner.RouteReplayConflict",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcConfirmationReplayTest::RunTest(const FString&)
{
	Fdemo_mapShanmenThrownWeaponArcConfirmationOwner Owner;
	FConfirmationProbe Probe;
	const auto Intent = MakeIntent(10);
	const auto First = Probe.Confirm(Owner, Intent);
	const auto Replay = Probe.Confirm(Owner, Intent);
	const auto Conflict = Probe.Confirm(Owner, MakeIntent(10, 3));

	TestTrue(TEXT("Fresh confirmation captures and routes exactly once"),
		First.IsValid()
			&& First.GetStatus() == EConfirmationStatus::Routed
			&& !First.WasReplay()
			&& First.GetChoiceStateReadCount() == 1
			&& First.GetLaunchRouteInvocationCount() == 1
			&& First.GetLaunchCommand().GetHotbarSlotNumber() == 2
			&& First.GetLaunchCommand().GetPolicy().Matches(
				Intent.GetPolicy()));
	TestTrue(TEXT("Exact event replay returns frozen evidence with zero work"),
		Replay.IsValid()
			&& Replay.GetStatus() == EConfirmationStatus::Routed
			&& Replay.WasReplay()
			&& Replay.GetChoiceStateReadCount() == 0
			&& Replay.GetLaunchRouteInvocationCount() == 0
			&& Replay.GetLaunchCommand().Matches(
				First.GetLaunchCommand())
			&& Probe.ChoiceReadCount == 1
			&& Probe.LaunchRouteCount == 1);
	TestTrue(TEXT("Same event identity with changed payload fails closed"),
		Conflict.IsValid()
			&& Conflict.GetStatus() == EConfirmationStatus::EventConflict
			&& !Conflict.WasReplay()
			&& Owner.NumRetainedEvents() == 1
			&& Probe.ChoiceReadCount == 1
			&& Probe.LaunchRouteCount == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcConfirmationProtocolTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcConfirmationOwner.InputProtocol",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcConfirmationProtocolTest::RunTest(
	const FString&)
{
	Fdemo_mapShanmenThrownWeaponArcConfirmationOwner Owner;
	FConfirmationProbe InvalidProbe;
	InvalidProbe.bReturnInvalidInput = true;
	const auto Invalid = InvalidProbe.Confirm(Owner, MakeIntent(20));
	FConfirmationProbe MismatchProbe;
	MismatchProbe.bReturnMismatchedInput = true;
	const auto Mismatch = MismatchProbe.Confirm(Owner, MakeIntent(21));
	TestTrue(TEXT("Invalid P20.15 evidence becomes typed protocol rejection"),
		Invalid.IsValid()
			&& Invalid.GetStatus()
				== EConfirmationStatus::InputProtocolRejected
			&& Invalid.GetLaunchRouteInvocationCount() == 1
			&& !Invalid.GetLaunchInput().IsValid());
	TestTrue(TEXT("Mismatched P20.15 command evidence also fails closed"),
		Mismatch.IsValid()
			&& Mismatch.GetStatus()
				== EConfirmationStatus::InputProtocolRejected
			&& Mismatch.GetLaunchInput().IsValid()
			&& !Mismatch.GetLaunchInput().GetCommand().Matches(
				Mismatch.GetLaunchCommand()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcConfirmationRetentionTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcConfirmationOwner.BoundedRetention",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcConfirmationRetentionTest::RunTest(
	const FString&)
{
	Fdemo_mapShanmenThrownWeaponArcConfirmationOwner Owner;
	FConfirmationProbe Probe;
	for (int32 Index = 1;
		Index <= Fdemo_mapShanmenThrownWeaponArcConfirmationOwner::
			MaximumRetainedEvents + 1;
		++Index)
	{
		const auto Result = Probe.Confirm(
			Owner, MakeIntent(100 + Index), false);
		TestTrue(
			*FString::Printf(TEXT("Blocked event %d is valid"), Index),
			Result.IsValid());
	}
	const auto LastReplay = Probe.Confirm(
		Owner,
		MakeIntent(
			100 + Fdemo_mapShanmenThrownWeaponArcConfirmationOwner::
				MaximumRetainedEvents + 1),
		false);
	const auto EvictedFirst = Probe.Confirm(Owner, MakeIntent(101), false);
	TestTrue(TEXT("Owner retains a fixed recent-event window"),
		Owner.IsValid()
			&& Owner.NumRetainedEvents()
				== Fdemo_mapShanmenThrownWeaponArcConfirmationOwner::
					MaximumRetainedEvents
			&& LastReplay.IsValid()
			&& LastReplay.WasReplay()
			&& EvictedFirst.IsValid()
			&& !EvictedFirst.WasReplay());
	Owner.Reset();
	TestTrue(TEXT("Explicit reset returns canonical empty valid state"),
		Owner.IsValid() && Owner.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcConfirmationPlayerControllerBoundaryTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcConfirmationOwner.PlayerControllerBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcConfirmationPlayerControllerBoundaryTest::RunTest(
	const FString&)
{
	Ademo_mapPlayerController* Controller =
		NewObject<Ademo_mapPlayerController>(
			GetTransientPackage(), NAME_None, RF_Transient);
	if (!TestNotNull(TEXT("Transient PlayerController exists"), Controller))
	{
		return false;
	}
	const auto Intent = MakeIntent(500);
	const auto First = Controller->RouteThrownWeaponArcConfirmation(Intent);
	const auto Replay = Controller->RouteThrownWeaponArcConfirmation(Intent);
	TestTrue(TEXT("Uninitialized input context blocks before World access"),
		First.IsValid()
			&& First.GetStatus()
				== EConfirmationStatus::ConfirmationBlocked
			&& !First.WasReplay()
			&& First.GetChoiceStateReadCount() == 0
			&& First.GetLaunchRouteInvocationCount() == 0
			&& Controller->GetInputSurfaceState() == TEXT("Gameplay")
			&& Controller->GetInputModeState() == TEXT("Uninitialized"));
	TestTrue(TEXT("Controller-owned event replay performs no downstream work"),
		Replay.IsValid()
			&& Replay.WasReplay()
			&& Replay.GetIntent().Matches(Intent)
			&& Replay.GetChoiceStateReadCount() == 0
			&& Replay.GetLaunchRouteInvocationCount() == 0);
	return true;
}

#endif
