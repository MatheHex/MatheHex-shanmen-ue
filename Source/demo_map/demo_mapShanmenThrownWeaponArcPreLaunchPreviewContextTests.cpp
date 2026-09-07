#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

#include "demo_mapShanmenThrownWeaponArcPreLaunchPreviewContext.h"

#include "Misc/AutomationTest.h"

namespace
{
	using EAction =
		Edemo_mapShanmenThrownWeaponArcPreLaunchPressAction;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreLaunchPreviewGestureTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreLaunchPreviewContext.ArmRearmConfirmAndComplete",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreLaunchPreviewGestureTest::RunTest(
	const FString&)
{
	Fdemo_mapShanmenThrownWeaponArcPreLaunchPreviewContext Context;
	const FGuid RunId(
		0xF4630001, 0xF4630002, 0xF4630003, 0xF4630004);
	FString Diagnostic;
	EAction Action = EAction::Invalid;
	TestTrue(TEXT("default context is a valid empty value"),
		Context.IsValid() && !Context.IsActive()
			&& !Context.HasArmedHotbarSlot());
	check(Context.TryBegin(RunId, Diagnostic));
	TestFalse(TEXT("invalid slot cannot mutate the active context"),
		Context.RouteEligibleHotbarPress(RunId, 0, Action, Diagnostic));
	TestTrue(TEXT("first eligible press arms one slot and one revision"),
		Context.RouteEligibleHotbarPress(RunId, 2, Action, Diagnostic)
			&& Action == EAction::Armed
			&& Context.GetArmedHotbarSlotNumber() == 2
			&& Context.GetRevision() == 1);
	TestTrue(TEXT("same-slot press requests confirmation without mutation"),
		Context.RouteEligibleHotbarPress(RunId, 2, Action, Diagnostic)
			&& Action == EAction::ConfirmationRequested
			&& Context.GetArmedHotbarSlotNumber() == 2
			&& Context.GetRevision() == 1);
	TestTrue(TEXT("different eligible slot atomically replaces the selection"),
		Context.RouteEligibleHotbarPress(RunId, 4, Action, Diagnostic)
			&& Action == EAction::Rearmed
			&& Context.GetArmedHotbarSlotNumber() == 4
			&& Context.GetRevision() == 2);
	TestTrue(TEXT("rejected launch preserves a correctable selection"),
		Context.TryCompleteConfirmation(RunId, 4, false, Diagnostic)
			&& Context.GetArmedHotbarSlotNumber() == 4
			&& Context.GetRevision() == 2);
	TestTrue(TEXT("accepted launch consumes only the preview context"),
		Context.TryCompleteConfirmation(RunId, 4, true, Diagnostic)
			&& !Context.HasArmedHotbarSlot()
			&& Context.GetRevision() == 3
			&& Context.IsValid());
	check(Context.TryEnd(RunId, Diagnostic));
	TestTrue(TEXT("Run end restores the canonical empty value"),
		Context.IsValid() && !Context.IsActive()
			&& Context.GetRevision() == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	Fdemo_mapThrownWeaponArcPreLaunchPreviewFenceTest,
	"Shanmen.0_0_10.Product.ThrownWeaponArcPreLaunchPreviewContext.CancelAndRunFences",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool Fdemo_mapThrownWeaponArcPreLaunchPreviewFenceTest::RunTest(
	const FString&)
{
	Fdemo_mapShanmenThrownWeaponArcPreLaunchPreviewContext Context;
	const FGuid RunId(
		0xF4630101, 0xF4630102, 0xF4630103, 0xF4630104);
	const FGuid ForeignRunId(
		0xF4630201, 0xF4630202, 0xF4630203, 0xF4630204);
	FString Diagnostic;
	EAction Action = EAction::Invalid;
	check(Context.TryBegin(RunId, Diagnostic));
	check(Context.RouteEligibleHotbarPress(
		RunId, 7, Action, Diagnostic));
	TestFalse(TEXT("foreign cancellation cannot clear the selected slot"),
		Context.TryCancel(ForeignRunId, Diagnostic));
	TestFalse(TEXT("foreign completion cannot consume the selected slot"),
		Context.TryCompleteConfirmation(
			ForeignRunId, 7, true, Diagnostic));
	TestFalse(TEXT("foreign teardown cannot end the context"),
		Context.TryEnd(ForeignRunId, Diagnostic));
	TestTrue(TEXT("all foreign attempts preserve exact local state"),
		Context.GetRunId() == RunId
			&& Context.GetArmedHotbarSlotNumber() == 7
			&& Context.GetRevision() == 1);
	TestTrue(TEXT("explicit target-clear cancellation removes the selection"),
		Context.TryCancel(RunId, Diagnostic)
			&& !Context.HasArmedHotbarSlot()
			&& Context.GetRevision() == 2);
	TestTrue(TEXT("duplicate cancellation is an idempotent no-op"),
		Context.TryCancel(RunId, Diagnostic)
			&& Context.GetRevision() == 2);
	TestFalse(TEXT("completion requires the exact currently armed slot"),
		Context.TryCompleteConfirmation(RunId, 7, true, Diagnostic));
	return true;
}

#endif
