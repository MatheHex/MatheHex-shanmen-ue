#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenThrownWeaponArcLaunchInputAdapter.h"

#include "demo_mapPlayerController.h"

#include "Components/SceneComponent.h"
#include "GameFramework/Actor.h"
#include "Misc/AutomationTest.h"

namespace
{
	using EInputStatus =
		Edemo_mapShanmenThrownWeaponArcLaunchInputStatus;
	using EProductStatus =
		Edemo_mapShanmenThrownWeaponArcSourceBasisRouteStatus;
	using ECompositionStatus =
		Edemo_mapShanmenThrownWeaponArcChoiceInputCompositionStatus;
	using ETrajectory =
		Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind;

	Fdemo_mapShanmenThrownWeaponInputChoiceState MakeArcChoice(
		const FVector2D& TargetIntent = FVector2D(0.5, 0.5),
		const double ApexAdjustment = 0.25)
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
			TryCaptureArcTargetIntent(1, TargetIntent, Command));
		Reduced = Fdemo_mapShanmenThrownWeaponInputChoiceReducer::Reduce(
			State, Command);
		check(Reduced.DidChange());
		State = Reduced.State;
		check(Fdemo_mapShanmenThrownWeaponInputChoiceCommand::
			TryCaptureArcApexAdjustment(2, ApexAdjustment, Command));
		Reduced = Fdemo_mapShanmenThrownWeaponInputChoiceReducer::Reduce(
			State, Command);
		check(Reduced.DidChange());
		return Reduced.State;
	}

	Fdemo_mapShanmenThrownWeaponInputChoiceState MakeArcChoiceWithoutTarget()
	{
		const auto Initial =
			Fdemo_mapShanmenThrownWeaponInputChoiceState::CreateInitial();
		Fdemo_mapShanmenThrownWeaponInputChoiceCommand Command;
		check(Fdemo_mapShanmenThrownWeaponInputChoiceCommand::
			TryCaptureTrajectorySelection(
				0, ETrajectory::BallisticArc, Command));
		const auto Reduced =
			Fdemo_mapShanmenThrownWeaponInputChoiceReducer::Reduce(
				Initial, Command);
		check(Reduced.DidChange());
		return Reduced.State;
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

	Fdemo_mapShanmenThrownWeaponArcLaunchCommand MakeCommand(
		const int32 HotbarSlotNumber = 2,
		const Fdemo_mapShanmenThrownWeaponInputChoiceState& ChoiceState =
			MakeArcChoice(),
		const Fdemo_mapShanmenThrownWeaponArcChoicePolicy& Policy =
			MakePolicy())
	{
		Fdemo_mapShanmenThrownWeaponArcLaunchCommand Command;
		check(Fdemo_mapShanmenThrownWeaponArcLaunchCommand::TryCapture(
			HotbarSlotNumber, ChoiceState, Policy, Command));
		return Command;
	}

	AActor* MakeSourceActor()
	{
		AActor* Actor = NewObject<AActor>(
			GetTransientPackage(), NAME_None, RF_Transient);
		check(Actor);
		USceneComponent* Root = NewObject<USceneComponent>(
			Actor, NAME_None, RF_Transient);
		check(Root);
		Actor->SetRootComponent(Root);
		Root->SetWorldLocationAndRotation(
			FVector(100.0, 200.0, 0.0),
			FRotator::ZeroRotator.Quaternion());
		return Actor;
	}

	Fdemo_mapShanmenThrownWeaponInputResult MakeRejectedInputResult()
	{
		Fdemo_mapShanmenThrownWeaponInputResult Result;
		Result.Status =
			Edemo_mapShanmenThrownWeaponInputStatus::ProductRejected;
		Result.TrajectoryKind = ETrajectory::BallisticArc;
		Result.bTargetSampled = true;
		Result.bApexClearanceSampled = true;
		Result.Diagnostic = TEXT("Synthetic product route rejected after delegation.");
		return Result;
	}

	Fdemo_mapShanmenThrownWeaponArcSourceBasisRouteResult MakeProductRoute(
		const Fdemo_mapShanmenThrownWeaponInputChoiceState& ChoiceState,
		const Fdemo_mapShanmenThrownWeaponArcChoicePolicy& Policy)
	{
		const Fdemo_mapShanmenThrownWeaponArcSourceBasisAdapter SourceAdapter;
		const Fdemo_mapShanmenThrownWeaponArcChoiceInputComposition Composition;
		return SourceAdapter.Route(
			MakeSourceActor(),
			[&](TFunctionRef<
				Fdemo_mapShanmenThrownWeaponArcChoiceBasis()> SampleBasis)
			{
				return Composition.Route(
					ChoiceState,
					Policy,
					SampleBasis,
					[](TFunctionRef<FVector()> SampleTarget,
						TFunctionRef<double()> SampleApex)
					{
						SampleTarget();
						SampleApex();
						return MakeRejectedInputResult();
					});
			});
	}

	struct FRouteProbe
	{
		bool bRouteAvailable = true;
		bool bReturnInvalidProduct = false;
		int32 ResolutionCount = 0;
		int32 ChoiceReadCount = 0;
		int32 ProductRouteCount = 0;
		int32 CapturedSlot = INDEX_NONE;
		FGuid CapturedPolicyId;
		Fdemo_mapShanmenThrownWeaponInputChoiceState CurrentChoice =
			MakeArcChoice();

		Fdemo_mapShanmenThrownWeaponArcLaunchInputResult Route(
			const Fdemo_mapShanmenThrownWeaponArcLaunchCommand& Command,
			const bool bGameplayInputAllowed = true,
			const bool bGameplaySurface = true,
			const bool bGameOnlyInputMode = true)
		{
			return Fdemo_mapShanmenThrownWeaponArcLaunchInputAdapter::Route(
				Command,
				bGameplayInputAllowed,
				bGameplaySurface,
				bGameOnlyInputMode,
				[this]()
				{
					++ResolutionCount;
					return bRouteAvailable;
				},
				[this]()
				{
					++ChoiceReadCount;
					return CurrentChoice;
				},
				[this](
					const int32 HotbarSlotNumber,
					const Fdemo_mapShanmenThrownWeaponArcChoicePolicy& Policy)
				{
					++ProductRouteCount;
					CapturedSlot = HotbarSlotNumber;
					CapturedPolicyId = Policy.GetPolicyId();
					return bReturnInvalidProduct
						? Fdemo_mapShanmenThrownWeaponArcSourceBasisRouteResult()
						: MakeProductRoute(CurrentChoice, Policy);
				});
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcLaunchCommandContractTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcLaunchInputAdapter.CommandContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcLaunchCommandContractTest::RunTest(
	const FString&)
{
	const auto Choice = MakeArcChoice();
	const auto Policy = MakePolicy();
	const auto First = MakeCommand(2, Choice, Policy);
	const auto Replay = MakeCommand(2, Choice, Policy);
	const auto OtherSlot = MakeCommand(3, Choice, Policy);
	const auto OtherChoice = MakeCommand(2, MakeArcChoice(
		FVector2D(0.25, 0.75), -0.25), Policy);
	const auto OtherPolicy = MakeCommand(2, Choice, MakePolicy(1300.0));
	TestTrue(TEXT("Equal canonical launch inputs replay one command identity"),
		First.IsValid()
			&& First.Matches(Replay)
			&& First.GetCommandId() == Replay.GetCommandId()
			&& First.GetHotbarSlotNumber() == 2
			&& First.GetChoiceState().Matches(Choice)
			&& First.GetPolicy().Matches(Policy));
	TestTrue(TEXT("Slot, choice, and policy each participate in identity"),
		First.GetCommandId() != OtherSlot.GetCommandId()
			&& First.GetCommandId() != OtherChoice.GetCommandId()
			&& First.GetCommandId() != OtherPolicy.GetCommandId());

	Fdemo_mapShanmenThrownWeaponArcLaunchCommand Rejected;
	TestFalse(TEXT("Slot zero fails closed"),
		Fdemo_mapShanmenThrownWeaponArcLaunchCommand::TryCapture(
			0, Choice, Policy, Rejected));
	TestFalse(TEXT("Slot ten fails closed"),
		Fdemo_mapShanmenThrownWeaponArcLaunchCommand::TryCapture(
			10, Choice, Policy, Rejected));
	TestFalse(TEXT("Straight choice fails closed"),
		Fdemo_mapShanmenThrownWeaponArcLaunchCommand::TryCapture(
			2,
			Fdemo_mapShanmenThrownWeaponInputChoiceState::CreateInitial(),
			Policy,
			Rejected));
	TestFalse(TEXT("Arc choice without target intent fails closed"),
		Fdemo_mapShanmenThrownWeaponArcLaunchCommand::TryCapture(
			2, MakeArcChoiceWithoutTarget(), Policy, Rejected));
	TestFalse(TEXT("Invalid policy fails closed"),
		Fdemo_mapShanmenThrownWeaponArcLaunchCommand::TryCapture(
			2,
			Choice,
			Fdemo_mapShanmenThrownWeaponArcChoicePolicy(),
			Rejected));
	TestFalse(TEXT("Failed capture leaves no reusable command"),
		Rejected.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcLaunchLocalGateTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcLaunchInputAdapter.LocalGateOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcLaunchLocalGateTest::RunTest(const FString&)
{
	FRouteProbe Probe;
	const auto Command = MakeCommand(2, Probe.CurrentChoice, MakePolicy());
	const auto Gameplay = Probe.Route(Command, false, false, false);
	const auto Surface = Probe.Route(Command, true, false, false);
	const auto Mode = Probe.Route(Command, true, true, false);
	TestTrue(TEXT("Gameplay, surface, and mode gates reject in fixed order"),
		Gameplay.IsValid()
			&& Gameplay.GetStatus() == EInputStatus::GameplayBlocked
			&& Surface.IsValid()
			&& Surface.GetStatus() == EInputStatus::InputSurfaceBlocked
			&& Mode.IsValid()
			&& Mode.GetStatus() == EInputStatus::InputModeBlocked);
	TestTrue(TEXT("Local rejection resolves, reads, and routes nothing"),
		Probe.ResolutionCount == 0
			&& Probe.ChoiceReadCount == 0
			&& Probe.ProductRouteCount == 0
			&& Gameplay.GetProductRouteResolutionCount() == 0
			&& Surface.GetChoiceStateReadCount() == 0
			&& Mode.GetProductRouteInvocationCount() == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcLaunchContextFenceTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcLaunchInputAdapter.ContextFences",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcLaunchContextFenceTest::RunTest(
	const FString&)
{
	const auto FrozenChoice = MakeArcChoice();
	const auto Command = MakeCommand(2, FrozenChoice, MakePolicy());
	FRouteProbe Unavailable;
	Unavailable.CurrentChoice = FrozenChoice;
	Unavailable.bRouteAvailable = false;
	const auto Missing = Unavailable.Route(Command);
	TestTrue(TEXT("Unavailable GameMode route stops before choice read"),
		Missing.IsValid()
			&& Missing.GetStatus() == EInputStatus::ProductRouteUnavailable
			&& Unavailable.ResolutionCount == 1
			&& Unavailable.ChoiceReadCount == 0
			&& Unavailable.ProductRouteCount == 0);

	FRouteProbe InvalidChoice;
	InvalidChoice.CurrentChoice =
		Fdemo_mapShanmenThrownWeaponInputChoiceState();
	const auto Invalid = InvalidChoice.Route(Command);
	TestTrue(TEXT("Invalid current choice fails closed after one read"),
		Invalid.IsValid()
			&& Invalid.GetStatus() == EInputStatus::ChoiceStateInvalid
			&& InvalidChoice.ResolutionCount == 1
			&& InvalidChoice.ChoiceReadCount == 1
			&& InvalidChoice.ProductRouteCount == 0);

	FRouteProbe StaleChoice;
	StaleChoice.CurrentChoice = MakeArcChoice(
		FVector2D(0.25, 0.75), -0.25);
	const auto Stale = StaleChoice.Route(Command);
	TestTrue(TEXT("Changed authoritative choice rejects the frozen command"),
		Stale.IsValid()
			&& Stale.GetStatus() == EInputStatus::ChoiceStateMismatch
			&& !Stale.GetCurrentChoiceState().Matches(
				Command.GetChoiceState())
			&& StaleChoice.ResolutionCount == 1
			&& StaleChoice.ChoiceReadCount == 1
			&& StaleChoice.ProductRouteCount == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcLaunchDelegationTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcLaunchInputAdapter.DelegateAndReplay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcLaunchDelegationTest::RunTest(const FString&)
{
	FRouteProbe Probe;
	const auto Policy = MakePolicy();
	const auto Command = MakeCommand(2, Probe.CurrentChoice, Policy);
	const auto First = Probe.Route(Command);
	const auto Replay = Probe.Route(Command);
	TestTrue(TEXT("Eligible command delegates one exact P20.14 route"),
		First.IsValid()
			&& First.GetStatus() == EInputStatus::Delegated
			&& First.GetProductRoute().GetStatus() == EProductStatus::Composed
			&& First.GetProductRoute().GetSourceSampleRequestCount() == 1
			&& First.GetProductRoute().GetComposition().GetStatus()
				== ECompositionStatus::Delegated
			&& First.GetProductRouteResolutionCount() == 1
			&& First.GetChoiceStateReadCount() == 1
			&& First.GetProductRouteInvocationCount() == 1);
	TestTrue(TEXT("Exact replay preserves command and delegation evidence"),
		Replay.IsValid()
			&& Replay.GetStatus() == EInputStatus::Delegated
			&& Replay.GetCommand().Matches(Command)
			&& Replay.GetCurrentChoiceState().Matches(
				Command.GetChoiceState())
			&& Probe.ResolutionCount == 2
			&& Probe.ChoiceReadCount == 2
			&& Probe.ProductRouteCount == 2
			&& Probe.CapturedSlot == 2
			&& Probe.CapturedPolicyId == Policy.GetPolicyId());
	TestFalse(TEXT("Input seam does not promote a downstream product rejection"),
		First.IsAccepted() || Replay.IsAccepted());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcLaunchProtocolTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcLaunchInputAdapter.ProductProtocol",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcLaunchProtocolTest::RunTest(const FString&)
{
	FRouteProbe Probe;
	Probe.bReturnInvalidProduct = true;
	const auto Command = MakeCommand(2, Probe.CurrentChoice, MakePolicy());
	const auto Result = Probe.Route(Command);
	TestTrue(TEXT("Invalid downstream evidence becomes a typed protocol rejection"),
		Result.IsValid()
			&& Result.GetStatus() == EInputStatus::ProductProtocolRejected
			&& !Result.IsAccepted()
			&& !Result.GetProductRoute().IsValid());
	TestTrue(TEXT("Protocol rejection still records one synchronous delegation"),
		Probe.ResolutionCount == 1
			&& Probe.ChoiceReadCount == 1
			&& Probe.ProductRouteCount == 1
			&& Result.GetProductRouteResolutionCount() == 1
			&& Result.GetChoiceStateReadCount() == 1
			&& Result.GetProductRouteInvocationCount() == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcLaunchPlayerControllerBoundaryTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcLaunchInputAdapter.PlayerControllerBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcLaunchPlayerControllerBoundaryTest::RunTest(
	const FString&)
{
	Ademo_mapPlayerController* Controller =
		NewObject<Ademo_mapPlayerController>(
			GetTransientPackage(), NAME_None, RF_Transient);
	if (!TestNotNull(TEXT("Transient PlayerController exists"), Controller))
	{
		return false;
	}
	const auto Command = MakeCommand();
	const auto Result =
		Controller->RouteThrownWeaponArcLaunchCommand(Command);
	TestTrue(TEXT("Uninitialized controller input mode fails before GameMode"),
		Result.IsValid()
			&& Result.GetStatus() == EInputStatus::InputModeBlocked
			&& Controller->GetInputSurfaceState() == TEXT("Gameplay")
			&& Controller->GetInputModeState() == TEXT("Uninitialized"));
	TestTrue(TEXT("Controller boundary preserves command and resolves no route"),
		Result.GetCommand().Matches(Command)
			&& Result.GetProductRouteResolutionCount() == 0
			&& Result.GetChoiceStateReadCount() == 0
			&& Result.GetProductRouteInvocationCount() == 0);
	return true;
}

#endif
