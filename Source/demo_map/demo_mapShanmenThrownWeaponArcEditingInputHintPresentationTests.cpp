#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenThrownWeaponArcEditingInputHintPresentation.h"

#include "Misc/AutomationTest.h"
#include "demo_mapShanmenThrownWeaponInputChoiceReducer.h"

namespace
{
	using ETrajectory =
		Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind;
	using FArcPresentation =
		Fdemo_mapShanmenThrownWeaponArcEditingPresentation;
	using FInputHintPresentation =
		Fdemo_mapShanmenThrownWeaponArcEditingInputHintPresentation;

	constexpr EAutomationTestFlags InputHintFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::EngineFilter;

	Fdemo_mapShanmenThrownWeaponInputChoiceState Apply(
		const Fdemo_mapShanmenThrownWeaponInputChoiceState& State,
		const Fdemo_mapShanmenThrownWeaponInputChoiceCommand& Command)
	{
		const auto Result =
			Fdemo_mapShanmenThrownWeaponInputChoiceReducer::Reduce(
				State, Command);
		check(Result.IsSuccess());
		return Result.State;
	}

	Fdemo_mapShanmenThrownWeaponInputChoiceState CreateArc()
	{
		const auto Initial =
			Fdemo_mapShanmenThrownWeaponInputChoiceState::CreateInitial();
		Fdemo_mapShanmenThrownWeaponInputChoiceCommand Command;
		check(Fdemo_mapShanmenThrownWeaponInputChoiceCommand::
			TryCaptureTrajectorySelection(
				Initial.GetRevision(), ETrajectory::BallisticArc, Command));
		return Apply(Initial, Command);
	}

	Fdemo_mapShanmenThrownWeaponInputChoiceState SetTarget(
		const Fdemo_mapShanmenThrownWeaponInputChoiceState& State)
	{
		Fdemo_mapShanmenThrownWeaponInputChoiceCommand Command;
		check(Fdemo_mapShanmenThrownWeaponInputChoiceCommand::
			TryCaptureArcTargetIntent(
				State.GetRevision(), FVector2D(3.0, 4.0), Command));
		return Apply(State, Command);
	}

	Fdemo_mapShanmenThrownWeaponInputChoiceState AdjustApex(
		const Fdemo_mapShanmenThrownWeaponInputChoiceState& State,
		const double Delta)
	{
		Fdemo_mapShanmenThrownWeaponInputChoiceCommand Command;
		check(Fdemo_mapShanmenThrownWeaponInputChoiceCommand::
			TryCaptureArcApexAdjustment(
				State.GetRevision(), Delta, Command));
		return Apply(State, Command);
	}

	bool ProjectArc(
		const Fdemo_mapShanmenThrownWeaponInputChoiceState& State,
		FArcPresentation& OutPresentation)
	{
		const auto Read =
			Fdemo_mapShanmenThrownWeaponInputChoiceInteractionPort::Read(
				[]() { return true; },
				[&State]() { return State; });
		return FArcPresentation::TryProject(Read, OutPresentation);
	}

	bool ProjectHint(
		const FArcPresentation& ArcPresentation,
		FInputHintPresentation& OutPresentation,
		const FString& Target = TEXT("Middle Mouse"),
		const FString& Increase = TEXT("Right Bracket"),
		const FString& Decrease = TEXT("Left Bracket"),
		const FString& Clear = TEXT("Delete"))
	{
		return FInputHintPresentation::TryProject(
			ArcPresentation,
			Target,
			Increase,
			Decrease,
			Clear,
			OutPresentation);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcInputHintNeutralTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcEditingInputHintPresentation.NeutralDefaults",
	InputHintFlags)

bool Fdemo_mapThrownWeaponArcInputHintNeutralTest::RunTest(const FString&)
{
	FArcPresentation Arc;
	FInputHintPresentation Hint;
	TestTrue(TEXT("neutral Arc projection is available"),
		ProjectArc(CreateArc(), Arc) && ProjectHint(Arc, Hint));
	TestTrue(TEXT("neutral hint exposes configured controls and capability state"),
		Hint.IsValid()
			&& Hint.GetTargetKeyLabel() == TEXT("Middle Mouse")
			&& Hint.GetApexIncreaseKeyLabel() == TEXT("Right Bracket")
			&& Hint.GetApexDecreaseKeyLabel() == TEXT("Left Bracket")
			&& Hint.GetClearKeyLabel() == TEXT("Delete")
			&& Hint.GetDisplayText() == TEXT(
				"ARC INPUT: [Middle Mouse] SET TARGET | [Right Bracket] APEX + | [Left Bracket] APEX - | [Delete] CLEAR (NO TARGET)"));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcInputHintTargetTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcEditingInputHintPresentation.TargetEnablesClear",
	InputHintFlags)

bool Fdemo_mapThrownWeaponArcInputHintTargetTest::RunTest(const FString&)
{
	FArcPresentation Arc;
	FInputHintPresentation Hint;
	TestTrue(TEXT("targeted Arc hint projects"),
		ProjectArc(SetTarget(CreateArc()), Arc) && ProjectHint(Arc, Hint));
	TestTrue(TEXT("clear is shown without a false unavailable annotation"),
		Hint.IsValid()
			&& Hint.GetDisplayText().EndsWith(TEXT("[Delete] CLEAR"))
			&& !Hint.GetDisplayText().Contains(TEXT("NO TARGET")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcInputHintBoundaryTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcEditingInputHintPresentation.ApexCapabilityBoundaries",
	InputHintFlags)

bool Fdemo_mapThrownWeaponArcInputHintBoundaryTest::RunTest(const FString&)
{
	FArcPresentation MaximumArc;
	FArcPresentation MinimumArc;
	FInputHintPresentation MaximumHint;
	FInputHintPresentation MinimumHint;
	TestTrue(TEXT("both boundary hints project"),
		ProjectArc(AdjustApex(CreateArc(), 1.0), MaximumArc)
			&& ProjectHint(MaximumArc, MaximumHint)
			&& ProjectArc(AdjustApex(CreateArc(), -1.0), MinimumArc)
			&& ProjectHint(MinimumArc, MinimumHint));
	TestTrue(TEXT("only the blocked apex direction is marked at each boundary"),
		MaximumHint.IsValid() && MinimumHint.IsValid()
			&& MaximumHint.GetDisplayText().Contains(
				TEXT("[Right Bracket] APEX + (LIMIT)"))
			&& !MaximumHint.GetDisplayText().Contains(
				TEXT("[Left Bracket] APEX - (LIMIT)"))
			&& MinimumHint.GetDisplayText().Contains(
				TEXT("[Left Bracket] APEX - (LIMIT)"))
			&& !MinimumHint.GetDisplayText().Contains(
				TEXT("[Right Bracket] APEX + (LIMIT)")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcInputHintRemapTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcEditingInputHintPresentation.RemappedLabels",
	InputHintFlags)

bool Fdemo_mapThrownWeaponArcInputHintRemapTest::RunTest(const FString&)
{
	FArcPresentation Arc;
	FInputHintPresentation Hint;
	TestTrue(TEXT("custom labels project after canonical trimming"),
		ProjectArc(SetTarget(CreateArc()), Arc)
			&& ProjectHint(
				Arc,
				Hint,
				TEXT("  H "),
				TEXT(" J"),
				TEXT("K "),
				TEXT(" L ")));
	TestTrue(TEXT("only canonical remapped labels appear"),
		Hint.IsValid()
			&& Hint.GetTargetKeyLabel() == TEXT("H")
			&& Hint.GetApexIncreaseKeyLabel() == TEXT("J")
			&& Hint.GetApexDecreaseKeyLabel() == TEXT("K")
			&& Hint.GetClearKeyLabel() == TEXT("L")
			&& Hint.GetDisplayText() == TEXT(
				"ARC INPUT: [H] SET TARGET | [J] APEX + | [K] APEX - | [L] CLEAR"));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcInputHintInvalidLabelsTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcEditingInputHintPresentation.InvalidLabelsFailClosed",
	InputHintFlags)

bool Fdemo_mapThrownWeaponArcInputHintInvalidLabelsTest::RunTest(
	const FString&)
{
	FArcPresentation Arc;
	FInputHintPresentation Reused;
	TestTrue(TEXT("fixture begins with a valid hint"),
		ProjectArc(CreateArc(), Arc) && ProjectHint(Arc, Reused));
	TestTrue(TEXT("blank label fails and clears reused output"),
		!ProjectHint(
			Arc,
			Reused,
			TEXT("Middle Mouse"),
			TEXT(" "),
			TEXT("Left Bracket"),
			TEXT("Delete"))
			&& !Reused.IsValid()
			&& Reused.GetDisplayText().IsEmpty());
	TestTrue(TEXT("ambiguous duplicate labels also fail closed"),
		!ProjectHint(
			Arc,
			Reused,
			TEXT("H"),
			TEXT("J"),
			TEXT("j"),
			TEXT("L"))
			&& !Reused.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcInputHintInvalidArcTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcEditingInputHintPresentation.InvalidArcFailsClosed",
	InputHintFlags)

bool Fdemo_mapThrownWeaponArcInputHintInvalidArcTest::RunTest(const FString&)
{
	FArcPresentation ValidArc;
	FArcPresentation InvalidArc;
	FInputHintPresentation Reused;
	TestTrue(TEXT("fixture begins with a valid hint"),
		ProjectArc(CreateArc(), ValidArc) && ProjectHint(ValidArc, Reused));
	TestTrue(TEXT("invalid Arc presentation fails and clears reused output"),
		!ProjectHint(InvalidArc, Reused)
			&& !Reused.IsValid()
			&& Reused.GetTargetKeyLabel().IsEmpty()
			&& Reused.GetDisplayText().IsEmpty());
	return true;
}

#endif
