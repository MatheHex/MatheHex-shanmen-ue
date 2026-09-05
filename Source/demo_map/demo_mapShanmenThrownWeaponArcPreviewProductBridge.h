#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenThrownWeaponArcPreviewCapturePolicy.h"
#include "demo_mapShanmenThrownWeaponProductLifecycle.h"

class Fdemo_mapCombatRunCoordinator;

enum class Edemo_mapShanmenThrownWeaponArcPreviewProductBridgeStatus : uint8
{
	Invalid,
	InputRejected,
	ChoiceRejected,
	ProductUnavailable,
	RunUnavailable,
	SequenceUnavailable,
	RequestRejected,
	CaptureRejected,
	SequenceChanged,
	Captured
};

/**
 * Pointer-free evidence captured from one live product lifecycle.
 *
 * The bridge records the coordinator sequence both before and after the pure
 * P20.31 capture. A successful result therefore proves that the prospective
 * preview identity was assembled without consuming a real action sequence.
 */
class Fdemo_mapShanmenThrownWeaponArcPreviewProductBridgeResult
{
public:
	bool IsCaptured() const;
	bool Matches(
		const Fdemo_mapShanmenThrownWeaponArcPreviewProductBridgeResult& Other)
		const;
	Edemo_mapShanmenThrownWeaponArcPreviewProductBridgeStatus GetStatus() const
	{
		return Status;
	}
	const FString& GetDiagnostic() const { return Diagnostic; }
	int32 GetHotbarSlotNumber() const { return HotbarSlotNumber; }
	const Fdemo_mapShanmenThrownWeaponInputChoiceState& GetChoiceState() const
	{
		return ChoiceState;
	}
	const Fdemo_mapShanmenThrownWeaponArcChoicePolicy& GetChoicePolicy() const
	{
		return ChoicePolicy;
	}
	int32 GetSegmentCount() const { return SegmentCount; }
	const FShanmenContentStamp& GetAuthorityContent() const
	{
		return AuthorityContent;
	}
	const Fdemo_mapShanmenThrownWeaponSessionConfig& GetSessionConfig() const
	{
		return SessionConfig;
	}
	const FGuid& GetLifecycleRunId() const { return LifecycleRunId; }
	const FGuid& GetCoordinatorRunId() const { return CoordinatorRunId; }
	const FGuid& GetPlayerEntityId() const { return PlayerEntityId; }
	const FGuid& GetSourceItemInstanceId() const
	{
		return SourceItemInstanceId;
	}
	uint64 GetSequenceBefore() const { return SequenceBefore; }
	uint64 GetSequenceAfter() const { return SequenceAfter; }
	int32 GetSequenceReadCount() const { return SequenceReadCount; }
	const FGuid& GetPreviewRequestId() const { return PreviewRequestId; }
	const Fdemo_mapShanmenThrownWeaponArcPreviewCaptureResult& GetCapture()
		const
	{
		return Capture;
	}

private:
	friend class Fdemo_mapShanmenThrownWeaponArcPreviewProductBridge;

	Edemo_mapShanmenThrownWeaponArcPreviewProductBridgeStatus Status =
		Edemo_mapShanmenThrownWeaponArcPreviewProductBridgeStatus::Invalid;
	FString Diagnostic;
	int32 HotbarSlotNumber = INDEX_NONE;
	Fdemo_mapShanmenThrownWeaponInputChoiceState ChoiceState;
	Fdemo_mapShanmenThrownWeaponArcChoicePolicy ChoicePolicy;
	int32 SegmentCount = 0;
	FShanmenContentStamp AuthorityContent;
	Fdemo_mapShanmenThrownWeaponSessionConfig SessionConfig;
	FGuid LifecycleRunId;
	FGuid CoordinatorRunId;
	FGuid PlayerEntityId;
	FGuid SourceItemInstanceId;
	uint64 SequenceBefore = 0;
	uint64 SequenceAfter = 0;
	int32 SequenceReadCount = 0;
	FGuid PreviewRequestId;
	Fdemo_mapShanmenThrownWeaponArcPreviewCaptureResult Capture;
};

/**
 * Read-only live-product adapter for P20.31.
 *
 * It can only copy frozen lifecycle/correlation data, read current choice and
 * coordinator identity, and invoke the pure capture policy. It receives no
 * mutable item authority and owns no launch, reservation, World, or UI path.
 */
class Fdemo_mapShanmenThrownWeaponArcPreviewProductBridge
{
public:
	static Fdemo_mapShanmenThrownWeaponArcPreviewProductBridgeResult Capture(
		int32 HotbarSlotNumber,
		const Fdemo_mapShanmenThrownWeaponInputChoiceState& ChoiceState,
		const Fdemo_mapShanmenThrownWeaponArcChoicePolicy& ChoicePolicy,
		int32 SegmentCount,
		const Fdemo_mapShanmenThrownWeaponProductLifecycle& Lifecycle,
		const Fdemo_mapCombatRunCoordinator& Coordinator);
};
