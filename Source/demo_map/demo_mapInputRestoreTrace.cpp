#include "demo_mapInputRestoreTrace.h"

#if !UE_BUILD_SHIPPING

#include "demo_map.h"

namespace
{
	bool Gdemo_mapInputRestoreTraceRuntimeActive = false;
}

Fdemo_mapInputRestoreTrace::Fdemo_mapInputRestoreTrace(bool bInEnabled)
	: bEnabled(bInEnabled)
{
}

uint32 Fdemo_mapInputRestoreTrace::BeginTransition(
	Edemo_mapInputRestoreTracePhase InPhase)
{
	if (!bEnabled)
	{
		return TransitionSerial;
	}
	++TransitionSerial;
	Phase = InPhase;
	Boundary = Edemo_mapInputRestoreTraceBoundary::None;
	return TransitionSerial;
}

void Fdemo_mapInputRestoreTrace::SetBoundary(
	Edemo_mapInputRestoreTraceBoundary InBoundary)
{
	if (bEnabled)
	{
		Boundary = InBoundary;
	}
}

void Fdemo_mapInputRestoreTrace::Record(
	Edemo_mapInputRestoreTraceEvent Event,
	const Fdemo_mapInputRestoreTraceSnapshot& Snapshot,
	Edemo_mapInputRestoreTraceCaller Caller)
{
	if (!bEnabled)
	{
		return;
	}
	if (Count >= Capacity)
	{
		++DroppedCount;
		return;
	}
	Fdemo_mapInputRestoreTraceEntry& Entry = Entries[Count];
	Entry.Sequence = static_cast<uint64>(Count + 1);
	Entry.Frame = static_cast<uint64>(GFrameCounter);
	Entry.TransitionSerial = TransitionSerial;
	Entry.Phase = Phase;
	Entry.Boundary = Boundary;
	Entry.Event = Event;
	Entry.Caller = Caller;
	Entry.Snapshot = Snapshot;
	++Count;
}

void Fdemo_mapInputRestoreTrace::FlushToLog(const TCHAR* Terminal) const
{
	if (!bEnabled || bFlushed)
	{
		return;
	}
	bFlushed = true;
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const Fdemo_mapInputRestoreTraceEntry& Entry = Entries[Index];
		const Fdemo_mapInputRestoreTraceSnapshot& State = Entry.Snapshot;
		UE_LOG(
			Logdemo_map,
			Log,
			TEXT("INPUT_RESTORE_TRACE terminal=%s seq=%llu frame=%llu transition_serial=%u phase=%s boundary=%s event=%s caller=%s reason=%s real_seconds=%.6f world_seconds=%.6f controller=0x%llx pawn=0x%llx run_id=%s container_id=%s widget_valid=%d widget_in_viewport=%d widget_focus=%d slate_keyboard_focus=%d preparation_owned=%d search_owned=%d settlement_owned=%d move_ignored=%d look_ignored=%d gameplay_allowed=%d context=%d input_mode=%d show_mouse_cursor=%d possessed=%d movement_mode=%d velocity_xy=%.3f location_xy=(%.3f,%.3f) move_key_down_if_observable=%d move_action_value_if_observable=%.1f latency=%.6f distance=%.3f owned_ignore=%d player_input=%d input_component=%d"),
			Terminal ? Terminal : TEXT("Unknown"),
			Entry.Sequence,
			Entry.Frame,
			Entry.TransitionSerial,
			PhaseName(Entry.Phase),
			BoundaryName(Entry.Boundary),
			EventName(Entry.Event),
			CallerName(Entry.Caller),
			EventName(Entry.Event),
			State.RealSeconds,
			State.WorldSeconds,
			State.ControllerIdentity,
			State.PawnIdentity,
			*State.RunId.ToString(EGuidFormats::DigitsWithHyphens),
			*State.ContainerId.ToString(EGuidFormats::DigitsWithHyphens),
			State.bWidgetValid,
			State.bWidgetInViewport,
			State.bWidgetFocus,
			State.bSlateKeyboardFocus,
			State.bPreparationLock,
			State.bSearchLock,
			State.bSettlementLock,
			State.bMoveIgnored,
			State.bLookIgnored,
			State.bGameplayAllowed,
			State.InputSurface,
			State.InputMode,
			State.bShowMouseCursor,
			State.bPossessed,
			State.MovementMode,
			State.VelocityUUPerSecond,
			State.LocationX,
			State.LocationY,
			State.bMoveForwardPressed,
			State.bMoveForwardPressed ? 1.0 : 0.0,
			State.LatencySeconds,
			State.DistanceUU,
			State.bOwnedIgnore,
			State.bPlayerInputPresent,
			State.bInputComponentPresent);
	}
	UE_LOG(
		Logdemo_map,
		Log,
		TEXT("INPUT_RESTORE_TRACE terminal=%s summary=1 count=%d capacity=%d dropped=%d serial=%u"),
		Terminal ? Terminal : TEXT("Unknown"),
		Count,
		Capacity,
		DroppedCount,
		TransitionSerial);
}

const Fdemo_mapInputRestoreTraceEntry*
Fdemo_mapInputRestoreTrace::GetEntry(int32 Index) const
{
	return Index >= 0 && Index < Count ? &Entries[Index] : nullptr;
}

const TCHAR* Fdemo_mapInputRestoreTrace::PhaseName(
	Edemo_mapInputRestoreTracePhase Value)
{
	switch (Value)
	{
	case Edemo_mapInputRestoreTracePhase::RunStartFresh: return TEXT("RunStartFresh");
	case Edemo_mapInputRestoreTracePhase::RunStartLegacy: return TEXT("RunStartLegacy");
	case Edemo_mapInputRestoreTracePhase::ChestTakeClose: return TEXT("ChestTakeClose");
	case Edemo_mapInputRestoreTracePhase::CorpseTakeClose: return TEXT("CorpseTakeClose");
	case Edemo_mapInputRestoreTracePhase::Reload: return TEXT("Reload");
	default: return TEXT("Unknown");
	}
}

const TCHAR* Fdemo_mapInputRestoreTrace::BoundaryName(
	Edemo_mapInputRestoreTraceBoundary Value)
{
	switch (Value)
	{
	case Edemo_mapInputRestoreTraceBoundary::RunStartPlayable: return TEXT("RunStartPlayable");
	case Edemo_mapInputRestoreTraceBoundary::ChestTakeCloseCommitted: return TEXT("ChestTakeCloseCommitted");
	case Edemo_mapInputRestoreTraceBoundary::CorpseTakeCloseCommitted: return TEXT("CorpseTakeCloseCommitted");
	default: return TEXT("None");
	}
}

const TCHAR* Fdemo_mapInputRestoreTrace::EventName(
	Edemo_mapInputRestoreTraceEvent Value)
{
	switch (Value)
	{
	case Edemo_mapInputRestoreTraceEvent::AutomationInitialized: return TEXT("AutomationInitialized");
	case Edemo_mapInputRestoreTraceEvent::RunActivated: return TEXT("RunActivated");
	case Edemo_mapInputRestoreTraceEvent::ContainerOpenBegin: return TEXT("ContainerOpenBegin");
	case Edemo_mapInputRestoreTraceEvent::ContainerOpenCommitted: return TEXT("ContainerOpenCommitted");
	case Edemo_mapInputRestoreTraceEvent::TakeCommitted: return TEXT("TakeCommitted");
	case Edemo_mapInputRestoreTraceEvent::CloseIntentReceived: return TEXT("CloseIntentReceived");
	case Edemo_mapInputRestoreTraceEvent::ContainerCloseBegin: return TEXT("ContainerCloseBegin");
	case Edemo_mapInputRestoreTraceEvent::ContainerCloseCommitted: return TEXT("ContainerCloseCommitted");
	case Edemo_mapInputRestoreTraceEvent::SearchLockApplied: return TEXT("SearchLockApplied");
	case Edemo_mapInputRestoreTraceEvent::SearchAuthorityCleared: return TEXT("SearchAuthorityCleared");
	case Edemo_mapInputRestoreTraceEvent::WidgetRemovalRequested: return TEXT("WidgetRemovalRequested");
	case Edemo_mapInputRestoreTraceEvent::WidgetNoLongerAuthoritative: return TEXT("WidgetNoLongerAuthoritative");
	case Edemo_mapInputRestoreTraceEvent::SearchReleaseRequested: return TEXT("SearchReleaseRequested");
	case Edemo_mapInputRestoreTraceEvent::SearchReleased: return TEXT("SearchReleased");
	case Edemo_mapInputRestoreTraceEvent::ReconcileBegin: return TEXT("ReconcileBegin");
	case Edemo_mapInputRestoreTraceEvent::GameOnlyApplied: return TEXT("GameOnlyApplied");
	case Edemo_mapInputRestoreTraceEvent::KeyboardFocusClearRequested: return TEXT("KeyboardFocusClearRequested");
	case Edemo_mapInputRestoreTraceEvent::ReconcileEnd: return TEXT("ReconcileEnd");
	case Edemo_mapInputRestoreTraceEvent::ExistingProbeBoundaryEmitted: return TEXT("ExistingProbeBoundaryEmitted");
	case Edemo_mapInputRestoreTraceEvent::MoveKeyDispatch: return TEXT("MoveKeyDispatch");
	case Edemo_mapInputRestoreTraceEvent::MoveActionHandlerEntered: return TEXT("MoveActionHandlerEntered");
	case Edemo_mapInputRestoreTraceEvent::ControlInputBecameNonZero: return TEXT("ControlInputBecameNonZero");
	case Edemo_mapInputRestoreTraceEvent::MoveHandlerRejected: return TEXT("MoveHandlerRejected");
	case Edemo_mapInputRestoreTraceEvent::MovementApplied: return TEXT("MovementApplied");
	case Edemo_mapInputRestoreTraceEvent::ProbeSample: return TEXT("ProbeSample");
	case Edemo_mapInputRestoreTraceEvent::VelocityBecameNonZero: return TEXT("VelocityBecameNonZero");
	case Edemo_mapInputRestoreTraceEvent::FirstPlanarDisplacement: return TEXT("FirstPlanarDisplacement");
	case Edemo_mapInputRestoreTraceEvent::DisplacementExceeded10UU: return TEXT("DisplacementExceeded10UU");
	case Edemo_mapInputRestoreTraceEvent::ProbePass: return TEXT("ProbePass");
	case Edemo_mapInputRestoreTraceEvent::ProbeFail: return TEXT("ProbeFail");
	case Edemo_mapInputRestoreTraceEvent::StaleCallbackAfterClose: return TEXT("StaleCallbackAfterClose");
	case Edemo_mapInputRestoreTraceEvent::NormalClose: return TEXT("NormalClose");
	default: return TEXT("Unknown");
	}
}

const TCHAR* Fdemo_mapInputRestoreTrace::CallerName(
	Edemo_mapInputRestoreTraceCaller Value)
{
	switch (Value)
	{
	case Edemo_mapInputRestoreTraceCaller::Controller: return TEXT("PlayerController");
	case Edemo_mapInputRestoreTraceCaller::Widget: return TEXT("SearchContainerWidget");
	case Edemo_mapInputRestoreTraceCaller::Probe: return TEXT("InputRestoreProbe");
	default: return TEXT("V3ProgressionManager");
	}
}

bool Isdemo_mapInputRestoreTraceRuntimeActive()
{
	return Gdemo_mapInputRestoreTraceRuntimeActive;
}

void Setdemo_mapInputRestoreTraceRuntimeActive(bool bActive)
{
	Gdemo_mapInputRestoreTraceRuntimeActive = bActive;
}

#endif
