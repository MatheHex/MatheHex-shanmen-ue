#pragma once

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

enum class Edemo_mapInputRestoreTracePhase : uint8
{
	Unknown,
	RunStartFresh,
	RunStartLegacy,
	ChestTakeClose,
	CorpseTakeClose,
	Reload
};

enum class Edemo_mapInputRestoreTraceBoundary : uint8
{
	None,
	RunStartPlayable,
	ChestTakeCloseCommitted,
	CorpseTakeCloseCommitted
};

enum class Edemo_mapInputRestoreTraceEvent : uint8
{
	AutomationInitialized,
	RunActivated,
	ContainerOpenBegin,
	ContainerOpenCommitted,
	TakeCommitted,
	CloseIntentReceived,
	ContainerCloseBegin,
	ContainerCloseCommitted,
	SearchLockApplied,
	SearchAuthorityCleared,
	WidgetRemovalRequested,
	WidgetNoLongerAuthoritative,
	SearchReleaseRequested,
	SearchReleased,
	ReconcileBegin,
	GameOnlyApplied,
	KeyboardFocusClearRequested,
	ReconcileEnd,
	ExistingProbeBoundaryEmitted,
	MoveKeyDispatch,
	MoveActionHandlerEntered,
	ControlInputBecameNonZero,
	MoveHandlerRejected,
	MovementApplied,
	ProbeSample,
	VelocityBecameNonZero,
	FirstPlanarDisplacement,
	DisplacementExceeded10UU,
	ProbePass,
	ProbeFail,
	StaleCallbackAfterClose,
	NormalClose
};

enum class Edemo_mapInputRestoreTraceCaller : uint8
{
	Manager,
	Controller,
	Widget,
	Probe
};

struct Fdemo_mapInputRestoreTraceSnapshot
{
	double RealSeconds = -1.0;
	double WorldSeconds = -1.0;
	double LatencySeconds = -1.0;
	float DistanceUU = -1.0f;
	float VelocityUUPerSecond = -1.0f;
	float LocationX = 0.0f;
	float LocationY = 0.0f;
	int32 MovementMode = INDEX_NONE;
	int32 InputSurface = INDEX_NONE;
	int32 InputMode = INDEX_NONE;
	uint64 ControllerIdentity = 0;
	uint64 PawnIdentity = 0;
	FGuid RunId;
	FGuid ContainerId;
	bool bSearchOpen = false;
	bool bWidgetValid = false;
	bool bWidgetInViewport = false;
	bool bWidgetFocus = false;
	bool bSlateKeyboardFocus = false;
	bool bSearchLock = false;
	bool bPreparationLock = false;
	bool bSettlementLock = false;
	bool bOwnedIgnore = false;
	bool bMoveIgnored = false;
	bool bLookIgnored = false;
	bool bGameplayAllowed = false;
	bool bMoveForwardPressed = false;
	bool bPlayerInputPresent = false;
	bool bInputComponentPresent = false;
	bool bShowMouseCursor = false;
	bool bPossessed = false;
};

struct Fdemo_mapInputRestoreTraceEntry
{
	uint64 Sequence = 0;
	uint64 Frame = 0;
	uint32 TransitionSerial = 0;
	Edemo_mapInputRestoreTracePhase Phase =
		Edemo_mapInputRestoreTracePhase::Unknown;
	Edemo_mapInputRestoreTraceBoundary Boundary =
		Edemo_mapInputRestoreTraceBoundary::None;
	Edemo_mapInputRestoreTraceEvent Event =
		Edemo_mapInputRestoreTraceEvent::AutomationInitialized;
	Edemo_mapInputRestoreTraceCaller Caller =
		Edemo_mapInputRestoreTraceCaller::Manager;
	Fdemo_mapInputRestoreTraceSnapshot Snapshot;
};

/**
 * P8.6 diagnostic-only observer. Recording is a bounded POD copy with no
 * event-time allocation, logging, file I/O, product-state write, or Tick loop.
 */
class Fdemo_mapInputRestoreTrace final
{
public:
	static constexpr int32 Capacity = 256;

	explicit Fdemo_mapInputRestoreTrace(bool bInEnabled = true);

	bool IsEnabled() const { return bEnabled; }
	uint32 BeginTransition(Edemo_mapInputRestoreTracePhase InPhase);
	void SetBoundary(Edemo_mapInputRestoreTraceBoundary InBoundary);
	void Record(
		Edemo_mapInputRestoreTraceEvent Event,
		const Fdemo_mapInputRestoreTraceSnapshot& Snapshot,
		Edemo_mapInputRestoreTraceCaller Caller =
			Edemo_mapInputRestoreTraceCaller::Manager);
	void FlushToLog(const TCHAR* Terminal) const;

	int32 Num() const { return Count; }
	int32 GetDroppedCount() const { return DroppedCount; }
	uint32 GetTransitionSerial() const { return TransitionSerial; }
	Edemo_mapInputRestoreTracePhase GetPhase() const { return Phase; }
	Edemo_mapInputRestoreTraceBoundary GetBoundary() const { return Boundary; }
	const Fdemo_mapInputRestoreTraceEntry* GetEntry(int32 Index) const;

	static const TCHAR* PhaseName(Edemo_mapInputRestoreTracePhase Value);
	static const TCHAR* BoundaryName(Edemo_mapInputRestoreTraceBoundary Value);
	static const TCHAR* EventName(Edemo_mapInputRestoreTraceEvent Value);
	static const TCHAR* CallerName(Edemo_mapInputRestoreTraceCaller Value);

private:
	Fdemo_mapInputRestoreTraceEntry Entries[Capacity] = {};
	int32 Count = 0;
	int32 DroppedCount = 0;
	uint32 TransitionSerial = 0;
	Edemo_mapInputRestoreTracePhase Phase =
		Edemo_mapInputRestoreTracePhase::Unknown;
	Edemo_mapInputRestoreTraceBoundary Boundary =
		Edemo_mapInputRestoreTraceBoundary::None;
	bool bEnabled = false;
	mutable bool bFlushed = false;
};

bool Isdemo_mapInputRestoreTraceRuntimeActive();
void Setdemo_mapInputRestoreTraceRuntimeActive(bool bActive);

#endif
