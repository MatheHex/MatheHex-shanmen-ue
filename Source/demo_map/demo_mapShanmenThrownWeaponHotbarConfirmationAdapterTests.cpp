#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenThrownWeaponHotbarConfirmationAdapter.h"

#include "demo_mapPlayerController.h"

#include "Misc/AutomationTest.h"

namespace
{
	using EHotbarStatus =
		Edemo_mapShanmenThrownWeaponHotbarConfirmationStatus;
	using EConfirmationStatus =
		Edemo_mapShanmenThrownWeaponArcConfirmationStatus;
	using ELaunchStatus =
		Edemo_mapShanmenThrownWeaponArcLaunchInputStatus;
	using ESourceStatus =
		Edemo_mapShanmenThrownWeaponArcSourceBasisRouteStatus;
	using ECompositionStatus =
		Edemo_mapShanmenThrownWeaponArcChoiceInputCompositionStatus;
	using EInputStatus = Edemo_mapShanmenThrownWeaponInputStatus;
	using ETrajectory =
		Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind;

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

	Fdemo_mapShanmenThrownWeaponInputResult MakeInput(
		const EInputStatus Status,
		const int32 HotbarSlotNumber,
		const ETrajectory TrajectoryKind)
	{
		Fdemo_mapShanmenThrownWeaponInputResult Result;
		Result.Status = Status;
		Result.HotbarSlotNumber = HotbarSlotNumber;
		Result.TrajectoryKind = TrajectoryKind;
		Result.Diagnostic = TEXT("Synthetic hotbar product route completed.");
		return Result;
	}

	Fdemo_mapShanmenThrownWeaponArcSourceBasisRouteResult MakeArcProductRoute(
		const Fdemo_mapShanmenThrownWeaponArcLaunchCommand& Command,
		const EInputStatus InputStatus)
	{
		const Fdemo_mapShanmenThrownWeaponArcSourceBasisAdapter SourceAdapter;
		const Fdemo_mapShanmenThrownWeaponArcChoiceInputComposition Composition;
		return SourceAdapter.Route(
			nullptr,
			[&](TFunctionRef<
				Fdemo_mapShanmenThrownWeaponArcChoiceBasis()> SampleBasis)
			{
				return Composition.Route(
					Command.GetChoiceState(),
					Command.GetPolicy(),
					SampleBasis,
					[&](TFunctionRef<FVector()>, TFunctionRef<double()>)
					{
						return MakeInput(
							InputStatus,
							Command.GetHotbarSlotNumber(),
							ETrajectory::BallisticArc);
					});
			});
	}

	Fdemo_mapShanmenThrownWeaponArcConfirmationResult MakeArcConfirmation(
		const Fdemo_mapShanmenThrownWeaponArcConfirmationIntent& Intent,
		const Fdemo_mapShanmenThrownWeaponInputChoiceState& Choice,
		const EInputStatus InputStatus)
	{
		Fdemo_mapShanmenThrownWeaponArcConfirmationOwner Owner;
		return Owner.Confirm(
			Intent,
			true,
			[&Choice]() { return Choice; },
			[&Choice, InputStatus](
				const Fdemo_mapShanmenThrownWeaponArcLaunchCommand& Command)
			{
				return Fdemo_mapShanmenThrownWeaponArcLaunchInputAdapter::Route(
					Command,
					true,
					true,
					true,
					[]() { return true; },
					[&Choice]() { return Choice; },
					[&Command, InputStatus](
						int32,
						const Fdemo_mapShanmenThrownWeaponArcChoicePolicy&)
					{
						return MakeArcProductRoute(Command, InputStatus);
					});
			});
	}

	struct FHotbarProbe
	{
		Fdemo_mapShanmenThrownWeaponInputChoiceState Choice =
			Fdemo_mapShanmenThrownWeaponInputChoiceState::CreateInitial();
		Fdemo_mapShanmenThrownWeaponArcChoicePolicy Policy =
			Fdemo_mapShanmenThrownWeaponArcChoiceProductPolicySource::
				GetCanonical();
		EInputStatus StraightStatus = EInputStatus::PassThrough;
		EInputStatus ArcInputStatus = EInputStatus::PassThrough;
		bool bMismatchedStraightSlot = false;
		bool bReturnInvalidArc = false;
		bool bReturnMismatchedArc = false;
		int32 ChoiceReadCount = 0;
		int32 PolicyReadCount = 0;
		int32 StraightRouteCount = 0;
		int32 ArcRouteCount = 0;
		Fdemo_mapShanmenThrownWeaponArcConfirmationIntent CapturedArcIntent;

		Fdemo_mapShanmenThrownWeaponHotbarConfirmationResult Route(
			Fdemo_mapShanmenThrownWeaponHotbarConfirmationAdapter& Adapter,
			const int32 HotbarSlotNumber,
			const bool bInputAllowed = true,
			const bool bConsumerAvailable = true)
		{
			return Adapter.Route(
				HotbarSlotNumber,
				bInputAllowed,
				bConsumerAvailable,
				[this]()
				{
					++ChoiceReadCount;
					return Choice;
				},
				[this]()
				{
					++PolicyReadCount;
					return Policy;
				},
				[this](const int32 SlotNumber)
				{
					++StraightRouteCount;
					return MakeInput(
						StraightStatus,
						bMismatchedStraightSlot ? SlotNumber + 1 : SlotNumber,
						ETrajectory::Straight);
				},
				[this](
					const Fdemo_mapShanmenThrownWeaponArcConfirmationIntent& Intent)
				{
					++ArcRouteCount;
					CapturedArcIntent = Intent;
					if (bReturnInvalidArc)
					{
						return Fdemo_mapShanmenThrownWeaponArcConfirmationResult();
					}
					if (bReturnMismatchedArc)
					{
						Fdemo_mapShanmenThrownWeaponArcConfirmationIntent Other;
						check(Fdemo_mapShanmenThrownWeaponArcConfirmationIntent::
							TryCapture(
								Fdemo_mapShanmenThrownWeaponHotbarConfirmationAdapter::
									MakeConfirmationEventId(9000),
								Intent.GetHotbarSlotNumber(),
								Intent.GetPolicy(),
								Other));
						return MakeArcConfirmation(
							Other, Choice, ArcInputStatus);
					}
					return MakeArcConfirmation(Intent, Choice, ArcInputStatus);
				});
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponHotbarConfirmationIdentityTest,
	"Shanmen.0_0_10.Product.ThrownWeaponHotbarConfirmationAdapter.PolicyAndIdentity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponHotbarConfirmationIdentityTest::RunTest(
	const FString&)
{
	const auto& Policy =
		Fdemo_mapShanmenThrownWeaponArcChoiceProductPolicySource::GetCanonical();
	const FGuid First =
		Fdemo_mapShanmenThrownWeaponHotbarConfirmationAdapter::
			MakeConfirmationEventId(1);
	const FGuid Replay =
		Fdemo_mapShanmenThrownWeaponHotbarConfirmationAdapter::
			MakeConfirmationEventId(1);
	const FGuid Next =
		Fdemo_mapShanmenThrownWeaponHotbarConfirmationAdapter::
			MakeConfirmationEventId(2);
	TestTrue(TEXT("Canonical Arc policy is valid and centrally frozen"),
		Policy.IsValid()
			&& Policy.GetMinimumForwardDistance() == 400.0
			&& Policy.GetMaximumForwardDistance() == 1200.0
			&& Policy.GetMaximumLateralOffset() == 300.0
			&& Policy.GetMinimumApexClearance() == 100.0
			&& Policy.GetMaximumApexClearance() == 500.0);
	TestTrue(TEXT("Equal ordinals reproduce identity and new ordinals differ"),
		First.IsValid() && First == Replay && First != Next);
	TestTrue(TEXT("Reserved sequence values fail closed"),
		!Fdemo_mapShanmenThrownWeaponHotbarConfirmationAdapter::
			MakeConfirmationEventId(0).IsValid()
			&& !Fdemo_mapShanmenThrownWeaponHotbarConfirmationAdapter::
				MakeConfirmationEventId(MAX_uint64).IsValid());
	Fdemo_mapShanmenThrownWeaponHotbarConfirmationAdapter Adapter;
	TestTrue(TEXT("Adapter begins at the first valid owner-local ordinal"),
		Adapter.IsValid() && Adapter.GetNextConfirmationOrdinal() == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponHotbarConfirmationGateTest,
	"Shanmen.0_0_10.Product.ThrownWeaponHotbarConfirmationAdapter.GatesAndChoice",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponHotbarConfirmationGateTest::RunTest(const FString&)
{
	Fdemo_mapShanmenThrownWeaponHotbarConfirmationAdapter Adapter;
	FHotbarProbe Probe;
	const auto InvalidSlot = Probe.Route(Adapter, 0);
	const auto Blocked = Probe.Route(Adapter, 1, false, true);
	const auto Missing = Probe.Route(Adapter, 1, true, false);
	Probe.Choice = Fdemo_mapShanmenThrownWeaponInputChoiceState();
	const auto InvalidChoice = Probe.Route(Adapter, 1);
	TestTrue(TEXT("Local gates fail in fixed order with valid evidence"),
		InvalidSlot.IsValid()
			&& InvalidSlot.GetStatus() == EHotbarStatus::InvalidSlot
			&& Blocked.IsValid()
			&& Blocked.GetStatus() == EHotbarStatus::InputBlocked
			&& Missing.IsValid()
			&& Missing.GetStatus() == EHotbarStatus::ConsumerUnavailable
			&& InvalidChoice.IsValid()
			&& InvalidChoice.GetStatus()
				== EHotbarStatus::ChoiceStateInvalid);
	TestTrue(TEXT("Only an eligible consumer reads choice once"),
		Probe.ChoiceReadCount == 1
			&& Probe.PolicyReadCount == 0
			&& Probe.StraightRouteCount == 0
			&& Probe.ArcRouteCount == 0
			&& Adapter.GetNextConfirmationOrdinal() == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponHotbarConfirmationStraightTest,
	"Shanmen.0_0_10.Product.ThrownWeaponHotbarConfirmationAdapter.StraightPassThroughAndHandled",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponHotbarConfirmationStraightTest::RunTest(
	const FString&)
{
	Fdemo_mapShanmenThrownWeaponHotbarConfirmationAdapter Adapter;
	FHotbarProbe Probe;
	const auto PassThrough = Probe.Route(Adapter, 2);
	Probe.StraightStatus = EInputStatus::ProductRejected;
	const auto Handled = Probe.Route(Adapter, 3);
	TestTrue(TEXT("Straight choice delegates to the unchanged route semantics"),
		PassThrough.IsValid()
			&& PassThrough.GetStatus() == EHotbarStatus::StraightDelegated
			&& PassThrough.ShouldPassThrough()
			&& Handled.IsValid()
			&& Handled.GetStatus() == EHotbarStatus::StraightDelegated
			&& !Handled.ShouldPassThrough());
	TestTrue(TEXT("Straight never reads Arc policy or consumes Arc identity"),
		Probe.ChoiceReadCount == 2
			&& Probe.StraightRouteCount == 2
			&& Probe.PolicyReadCount == 0
			&& Probe.ArcRouteCount == 0
			&& PassThrough.GetConfirmationOrdinal() == 0
			&& Handled.GetConfirmationOrdinal() == 0
			&& Adapter.GetNextConfirmationOrdinal() == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponHotbarConfirmationArcTest,
	"Shanmen.0_0_10.Product.ThrownWeaponHotbarConfirmationAdapter.ArcPassThroughAndHandled",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponHotbarConfirmationArcTest::RunTest(const FString&)
{
	Fdemo_mapShanmenThrownWeaponHotbarConfirmationAdapter Adapter;
	FHotbarProbe Probe;
	Probe.Choice = MakeArcChoice();
	const auto PassThrough = Probe.Route(Adapter, 2);
	Probe.ArcInputStatus = EInputStatus::ProductTrajectoryMismatch;
	const auto Handled = Probe.Route(Adapter, 3);
	TestTrue(TEXT("Arc non-thrown classification preserves generic pass-through"),
		PassThrough.IsValid()
			&& PassThrough.GetStatus() == EHotbarStatus::ArcDelegated
			&& PassThrough.ShouldPassThrough()
			&& PassThrough.GetArcConfirmation().GetStatus()
				== EConfirmationStatus::Routed
			&& PassThrough.GetArcConfirmation().GetLaunchInput().GetStatus()
				== ELaunchStatus::Delegated
			&& PassThrough.GetArcConfirmation().GetLaunchInput()
				.GetProductRoute().GetStatus()
					== ESourceStatus::InputCompletedBeforeSourceSample
			&& PassThrough.GetArcConfirmation().GetLaunchInput()
				.GetProductRoute().GetComposition().GetStatus()
					== ECompositionStatus::InputCompletedBeforeProjection);
	TestTrue(TEXT("Arc thrown handling remains owned by the existing product route"),
		Handled.IsValid()
			&& Handled.GetStatus() == EHotbarStatus::ArcDelegated
			&& !Handled.ShouldPassThrough());
	TestTrue(TEXT("Every Arc press receives one fresh deterministic event"),
		Probe.ChoiceReadCount == 2
			&& Probe.PolicyReadCount == 2
			&& Probe.StraightRouteCount == 0
			&& Probe.ArcRouteCount == 2
			&& PassThrough.GetConfirmationOrdinal() == 1
			&& Handled.GetConfirmationOrdinal() == 2
			&& PassThrough.GetConfirmationEventId()
				!= Handled.GetConfirmationEventId()
			&& Adapter.GetNextConfirmationOrdinal() == 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponHotbarConfirmationProtocolTest,
	"Shanmen.0_0_10.Product.ThrownWeaponHotbarConfirmationAdapter.ProtocolFailures",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponHotbarConfirmationProtocolTest::RunTest(
	const FString&)
{
	Fdemo_mapShanmenThrownWeaponHotbarConfirmationAdapter StraightAdapter;
	FHotbarProbe StraightProbe;
	StraightProbe.bMismatchedStraightSlot = true;
	const auto BadStraight = StraightProbe.Route(StraightAdapter, 2);
	TestTrue(TEXT("Mismatched Straight evidence fails closed"),
		BadStraight.IsValid()
			&& BadStraight.GetStatus()
				== EHotbarStatus::StraightRouteProtocolRejected
			&& !BadStraight.ShouldPassThrough());

	Fdemo_mapShanmenThrownWeaponHotbarConfirmationAdapter ArcAdapter;
	FHotbarProbe ArcProbe;
	ArcProbe.Choice = MakeArcChoice();
	ArcProbe.Policy = Fdemo_mapShanmenThrownWeaponArcChoicePolicy();
	const auto BadPolicy = ArcProbe.Route(ArcAdapter, 2);
	ArcProbe.Policy =
		Fdemo_mapShanmenThrownWeaponArcChoiceProductPolicySource::GetCanonical();
	ArcProbe.bReturnInvalidArc = true;
	const auto InvalidArc = ArcProbe.Route(ArcAdapter, 2);
	ArcProbe.bReturnInvalidArc = false;
	ArcProbe.bReturnMismatchedArc = true;
	const auto MismatchedArc = ArcProbe.Route(ArcAdapter, 2);
	TestTrue(TEXT("Invalid policy stops before P20.16 route"),
		BadPolicy.IsValid()
			&& BadPolicy.GetStatus() == EHotbarStatus::ArcPolicyInvalid
			&& BadPolicy.GetPolicyReadCount() == 1
			&& BadPolicy.GetArcRouteInvocationCount() == 0);
	TestTrue(TEXT("Invalid and mismatched P20.16 evidence are typed failures"),
		InvalidArc.IsValid()
			&& InvalidArc.GetStatus()
				== EHotbarStatus::ArcRouteProtocolRejected
			&& MismatchedArc.IsValid()
			&& MismatchedArc.GetStatus()
				== EHotbarStatus::ArcRouteProtocolRejected
			&& !InvalidArc.ShouldPassThrough()
			&& !MismatchedArc.ShouldPassThrough());
	TestTrue(TEXT("Failed Arc attempts still consume distinct event ordinals"),
		BadPolicy.GetConfirmationOrdinal() == 1
			&& InvalidArc.GetConfirmationOrdinal() == 2
			&& MismatchedArc.GetConfirmationOrdinal() == 3
			&& ArcAdapter.GetNextConfirmationOrdinal() == 4
			&& ArcProbe.PolicyReadCount == 3
			&& ArcProbe.ArcRouteCount == 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponHotbarConfirmationSequenceTest,
	"Shanmen.0_0_10.Product.ThrownWeaponHotbarConfirmationAdapter.SequenceExhaustion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponHotbarConfirmationSequenceTest::RunTest(
	const FString&)
{
	Fdemo_mapShanmenThrownWeaponHotbarConfirmationAdapter Adapter;
	Adapter.SetNextConfirmationOrdinalForAutomation(MAX_uint64 - 1);
	FHotbarProbe Probe;
	Probe.Choice = MakeArcChoice();
	const auto Last = Probe.Route(Adapter, 2);
	const auto Exhausted = Probe.Route(Adapter, 2);
	TestTrue(TEXT("Last usable sequence value routes and advances to sentinel"),
		Last.IsValid()
			&& Last.GetStatus() == EHotbarStatus::ArcDelegated
			&& Last.GetConfirmationOrdinal() == MAX_uint64 - 1
			&& Last.GetConfirmationEventId().IsValid()
			&& Adapter.GetNextConfirmationOrdinal() == MAX_uint64);
	TestTrue(TEXT("Exhausted sequence never wraps or performs Arc work"),
		Exhausted.IsValid()
			&& Exhausted.GetStatus()
				== EHotbarStatus::ConfirmationSequenceExhausted
			&& Exhausted.GetConfirmationOrdinal() == MAX_uint64
			&& !Exhausted.GetConfirmationEventId().IsValid()
			&& Probe.ChoiceReadCount == 2
			&& Probe.PolicyReadCount == 1
			&& Probe.ArcRouteCount == 1
			&& Adapter.GetNextConfirmationOrdinal() == MAX_uint64);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponHotbarConfirmationControllerBoundaryTest,
	"Shanmen.0_0_10.Product.ThrownWeaponHotbarConfirmationAdapter.PlayerControllerBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponHotbarConfirmationControllerBoundaryTest::RunTest(
	const FString&)
{
	Ademo_mapPlayerController* Controller =
		NewObject<Ademo_mapPlayerController>(
			GetTransientPackage(), NAME_None, RF_Transient);
	if (!TestNotNull(TEXT("Transient PlayerController exists"), Controller))
	{
		return false;
	}
	const auto Result =
		Controller->RouteThrownWeaponHotbarConfirmationInput(2);
	TestTrue(TEXT("Uninitialized controller blocks before World consumer access"),
		Result.IsValid()
			&& Result.GetStatus() == EHotbarStatus::InputBlocked
			&& Result.GetChoiceStateReadCount() == 0
			&& Result.GetPolicyReadCount() == 0
			&& Result.GetStraightRouteInvocationCount() == 0
			&& Result.GetArcRouteInvocationCount() == 0
			&& Controller->GetInputSurfaceState() == TEXT("Gameplay")
			&& Controller->GetInputModeState() == TEXT("Uninitialized"));
	return true;
}

#endif
