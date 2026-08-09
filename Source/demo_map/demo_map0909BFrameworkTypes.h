#pragma once

#include "CoreMinimal.h"
#include "CodeB/demo_mapCodeBLoadoutSelection.h"

/**
 * The only top-level state vocabulary exposed by the 0.0.9B product shell.
 * Technical start failures are deliberately distinct from player terminal
 * states; they never become Abandon, Dead, Extracted, or a Code B P8 event.
 */
enum class Edemo_map0909BTopState : uint8
{
	AtSect,
	PreparingStart,
	ActivatingWorld,
	InRun,
	ResolvingTerminal,
	TechnicalStartFailure
};

inline const TCHAR* demo_map0909BTopStateName(const Edemo_map0909BTopState State)
{
	switch (State)
	{
	case Edemo_map0909BTopState::AtSect: return TEXT("AtSect");
	case Edemo_map0909BTopState::PreparingStart: return TEXT("PreparingStart");
	case Edemo_map0909BTopState::ActivatingWorld: return TEXT("ActivatingWorld");
	case Edemo_map0909BTopState::InRun: return TEXT("InRun");
	case Edemo_map0909BTopState::ResolvingTerminal: return TEXT("ResolvingTerminal");
	case Edemo_map0909BTopState::TechnicalStartFailure: return TEXT("TechnicalStartFailure");
	default: return TEXT("Unknown");
	}
}

/**
 * The M01 adapter reports runtime facts with the attempt that caused them.
 * These are deliberately deployment facts, not player terminal states and not
 * Profile inventory state.
 */
enum class Edemo_map0909BM01RuntimeReceiptClass : uint8
{
	None,
	ActivationRequestAccepted,
	RuntimeReady,
	TechnicalFailure,
	StaleEventIgnored,
	DuplicateReadyIgnored
};

inline const TCHAR* demo_map0909BM01RuntimeReceiptClassName(
	const Edemo_map0909BM01RuntimeReceiptClass Value)
{
	switch (Value)
	{
	case Edemo_map0909BM01RuntimeReceiptClass::ActivationRequestAccepted:
		return TEXT("ActivationRequestAccepted");
	case Edemo_map0909BM01RuntimeReceiptClass::RuntimeReady:
		return TEXT("RuntimeReady");
	case Edemo_map0909BM01RuntimeReceiptClass::TechnicalFailure:
		return TEXT("TechnicalFailure");
	case Edemo_map0909BM01RuntimeReceiptClass::StaleEventIgnored:
		return TEXT("StaleEventIgnored");
	case Edemo_map0909BM01RuntimeReceiptClass::DuplicateReadyIgnored:
		return TEXT("DuplicateReadyIgnored");
	default:
		return TEXT("None");
	}
}

/** Immutable correlation evidence emitted by the formal M01 runtime adapter. */
struct Fdemo_map0909BM01RuntimeReceipt
{
	FGuid StartAttemptId;
	FGuid OwnerId;
	FGuid RunInstanceId;
	int64 Sequence = 0;
	FString MapDescriptor;
	FString MapIdentity;
	FString FailureClass = TEXT("None");
	FString Detail;
	Edemo_map0909BM01RuntimeReceiptClass ReceiptClass =
		Edemo_map0909BM01RuntimeReceiptClass::None;
	bool bDescriptorResolved = false;
	bool bActivationRequestAccepted = false;
	bool bWorldMatched = false;
	bool bGameModeReady = false;
	bool bWorldSettingsReady = false;
	bool bControllerReady = false;
	bool bPawnReady = false;
	bool bInputRestored = false;
	bool bCodeARunCreated = false;
	bool bRuntimeReady = false;

	bool IsRuntimeReadyFor(const FGuid& AttemptId) const
	{
		return bRuntimeReady && ReceiptClass == Edemo_map0909BM01RuntimeReceiptClass::RuntimeReady
			&& StartAttemptId == AttemptId && OwnerId.IsValid() && RunInstanceId.IsValid();
	}
};

/** Structured, read-only evidence for every 0.0.9B deployment attempt. */
struct Fdemo_map0909BStartDiagnostic
{
	FGuid StartAttemptId;
	FGuid OwnerId;
	FGuid RunId;
	FGuid ReleasedRunId;
	int64 AttemptSequence = 0;
	int64 RuntimeReceiptSequence = 0;
	Edemo_map0909BTopState BeforeState = Edemo_map0909BTopState::AtSect;
	Edemo_map0909BTopState AfterState = Edemo_map0909BTopState::AtSect;
	FString FailureClass = TEXT("None");
	FString Sequence;
	FString Detail;
	int32 LoadoutPersistentRevision = INDEX_NONE;
	int32 LoadoutGraphRevision = INDEX_NONE;
	FString LoadoutSelectionDigest;
	FString BridgeDiagnostic;
	FString RequestedAtUtc;
	FString M01MapDescriptor;
	FString M01MapIdentity;
	FString RuntimeReceiptClass;

	FString ToLogString() const
	{
		return FString::Printf(
			TEXT("AttemptId=%s AttemptSequence=%lld RuntimeReceiptSequence=%lld OwnerId=%s RunId=%s ReleasedRunId=%s Before=%s After=%s FailureClass=%s RuntimeReceipt=%s MapDescriptor=%s MapIdentity=%s RequestedAt=%s Sequence=%s LoadoutPersistentRevision=%d LoadoutGraphRevision=%d LoadoutDigest=%s Bridge=%s Detail=%s"),
			*StartAttemptId.ToString(EGuidFormats::DigitsWithHyphens),
			AttemptSequence,
			RuntimeReceiptSequence,
			*OwnerId.ToString(EGuidFormats::DigitsWithHyphens),
			*RunId.ToString(EGuidFormats::DigitsWithHyphens),
			*ReleasedRunId.ToString(EGuidFormats::DigitsWithHyphens),
			demo_map0909BTopStateName(BeforeState),
			demo_map0909BTopStateName(AfterState),
			*FailureClass,
			*RuntimeReceiptClass,
			*M01MapDescriptor,
			*M01MapIdentity,
			*RequestedAtUtc,
			*Sequence,
			LoadoutPersistentRevision,
			LoadoutGraphRevision,
			*LoadoutSelectionDigest,
			*BridgeDiagnostic,
			*Detail);
	}
};

/**
 * Narrow, transient result owned by the top-level coordinator.  It carries
 * identity only; Code B's P5/P6 graphs never cross the game-framework edge.
 */
struct Fdemo_map0909BRunStartResult
{
	FGuid OwnerId;
	FGuid RunInstanceId;
	FString Diagnostic;
	bool bRunActive = false;
};
