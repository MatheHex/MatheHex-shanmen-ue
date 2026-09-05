#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenThrownWeaponArcPreviewComposition.h"
#include "demo_mapShanmenThrownWeaponProductSession.h"

/**
 * Immutable read-only inputs for one prospective Arc preview configuration.
 *
 * ObservedNextActivationSequence is evidence only. Capturing this request or
 * applying the policy never reserves, increments, or owns the Run sequence.
 */
class Fdemo_mapShanmenThrownWeaponArcPreviewCaptureRequest
{
public:
	static bool TryCapture(
		const FGuid& PreviewRequestId,
		const FGuid& RunId,
		const FGuid& PlayerEntityId,
		const FGuid& SourceItemInstanceId,
		const FShanmenContentStamp& AuthorityContent,
		uint64 ObservedNextActivationSequence,
		const Fdemo_mapShanmenThrownWeaponSessionConfig& SessionConfig,
		const Fdemo_mapShanmenThrownWeaponArcChoicePolicy& ChoicePolicy,
		int32 SegmentCount,
		Fdemo_mapShanmenThrownWeaponArcPreviewCaptureRequest& OutRequest);

	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenThrownWeaponArcPreviewCaptureRequest& Other)
		const;
	const FGuid& GetPreviewRequestId() const { return PreviewRequestId; }
	const FGuid& GetRunId() const { return RunId; }
	const FGuid& GetPlayerEntityId() const { return PlayerEntityId; }
	const FGuid& GetSourceItemInstanceId() const
	{
		return SourceItemInstanceId;
	}
	const FShanmenContentStamp& GetAuthorityContent() const
	{
		return AuthorityContent;
	}
	uint64 GetObservedNextActivationSequence() const
	{
		return ObservedNextActivationSequence;
	}
	const Fdemo_mapShanmenThrownWeaponSessionConfig& GetSessionConfig() const
	{
		return SessionConfig;
	}
	const Fdemo_mapShanmenThrownWeaponArcChoicePolicy& GetChoicePolicy() const
	{
		return ChoicePolicy;
	}
	int32 GetSegmentCount() const { return SegmentCount; }

private:
	FGuid PreviewRequestId;
	FGuid RunId;
	FGuid PlayerEntityId;
	FGuid SourceItemInstanceId;
	FShanmenContentStamp AuthorityContent;
	uint64 ObservedNextActivationSequence = 0;
	Fdemo_mapShanmenThrownWeaponSessionConfig SessionConfig;
	Fdemo_mapShanmenThrownWeaponArcChoicePolicy ChoicePolicy;
	int32 SegmentCount = 0;
};

enum class Edemo_mapShanmenThrownWeaponArcPreviewCaptureStatus : uint8
{
	Invalid,
	RequestRejected,
	ActionRejected,
	ConfigurationRejected,
	Captured
};

/** Immutable evidence for one prospective, non-reserving preview capture. */
class Fdemo_mapShanmenThrownWeaponArcPreviewCaptureResult
{
public:
	bool IsValid() const;
	bool IsCaptured() const;
	bool Matches(
		const Fdemo_mapShanmenThrownWeaponArcPreviewCaptureResult& Other)
		const;
	Edemo_mapShanmenThrownWeaponArcPreviewCaptureStatus GetStatus() const
	{
		return Status;
	}
	const FString& GetDiagnostic() const { return Diagnostic; }
	const Fdemo_mapShanmenThrownWeaponArcPreviewCaptureRequest& GetRequest()
		const
	{
		return Request;
	}
	const FGuid& GetPreviewActivationId() const
	{
		return PreviewActivationId;
	}
	const FGuid& GetProspectiveRealActivationId() const
	{
		return ProspectiveRealActivationId;
	}
	const FShanmenCombatActionSnapshot& GetPreviewAction() const
	{
		return PreviewAction;
	}
	const Fdemo_mapShanmenThrownWeaponArcPreviewConfiguration&
	GetConfiguration() const
	{
		return Configuration;
	}

private:
	friend class Fdemo_mapShanmenThrownWeaponArcPreviewCapturePolicy;

	Edemo_mapShanmenThrownWeaponArcPreviewCaptureStatus Status =
		Edemo_mapShanmenThrownWeaponArcPreviewCaptureStatus::Invalid;
	FString Diagnostic;
	Fdemo_mapShanmenThrownWeaponArcPreviewCaptureRequest Request;
	FGuid PreviewActivationId;
	FGuid ProspectiveRealActivationId;
	FShanmenCombatActionSnapshot PreviewAction;
	Fdemo_mapShanmenThrownWeaponArcPreviewConfiguration Configuration;
};

/**
 * Pure preview-only action/configuration policy.
 *
 * The preview activation uses its own deterministic namespace. The standard
 * future activation ID is emitted only as comparison evidence; no coordinator,
 * inventory authority, product session, World, or launch object is mutated.
 */
class Fdemo_mapShanmenThrownWeaponArcPreviewCapturePolicy
{
public:
	static Fdemo_mapShanmenThrownWeaponArcPreviewCaptureResult Capture(
		const Fdemo_mapShanmenThrownWeaponArcPreviewCaptureRequest& Request);
};
