#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenFormationDiagramSelectionPort.h"
#include "demo_mapShanmenFormationRunLifecycle.h"

/** One immutable known-diagram/origin/facing sample from an input surface. */
class Fdemo_mapShanmenFormationStartInputSample
{
public:
	static bool TryCapture(
		const Fdemo_mapShanmenFormationDiagramSelection& RequestedSelection,
		const FVector& RequestedOrigin,
		const FVector& RequestedForward,
		Fdemo_mapShanmenFormationStartInputSample& OutSample);

	bool IsValid() const;
	const Fdemo_mapShanmenFormationDiagramSelection& GetSelection() const
	{
		return Selection;
	}
	const FShanmenFormationDiagramDefinition& GetDiagram() const
	{
		return Selection.GetDiagram();
	}
	const FVector& GetOrigin() const { return Origin; }
	const FVector& GetForward() const { return Forward; }

private:
	Fdemo_mapShanmenFormationDiagramSelection Selection;
	FVector Origin = FVector::ZeroVector;
	FVector Forward = FVector::ZeroVector;
};

/** One immutable anchor selection from a caller-owned input surface. */
class Fdemo_mapShanmenFormationAnchorInputSample
{
public:
	static bool TryCapture(
		FName RequestedAnchorDefinitionId,
		Fdemo_mapShanmenFormationAnchorInputSample& OutSample);

	bool IsValid() const { return !AnchorDefinitionId.IsNone(); }
	FName GetAnchorDefinitionId() const { return AnchorDefinitionId; }

private:
	FName AnchorDefinitionId = NAME_None;
};

enum class Edemo_mapShanmenFormationStartInputStatus : uint8
{
	Invalid,
	Applied,
	GameplayBlocked,
	LifecycleUnavailable,
	RunUnavailable,
	OwnerUnavailable,
	EventIdentityInvalid,
	SampleRejected,
	SelectionOwnerMismatch,
	IntentCaptureRejected,
	LifecycleRejected
};

/** Audit proof that one start event samples and delegates at most once. */
struct Fdemo_mapShanmenFormationStartInputResult
{
	Edemo_mapShanmenFormationStartInputStatus Status =
		Edemo_mapShanmenFormationStartInputStatus::Invalid;
	int32 SampleCount = 0;
	int32 LifecycleInvocationCount = 0;
	FGuid InputEventId;
	FGuid RunId;
	FGuid OwnerId;
	FGuid IntentId;
	Fdemo_mapShanmenFormationStartInputSample Sample;
	Fdemo_mapShanmenFormationIntent Intent;
	Fdemo_mapShanmenFormationControllerResult Lifecycle;
	FString Diagnostic;

	bool IsValid() const;
	bool IsAccepted() const;
};

enum class Edemo_mapShanmenFormationAnchorInputStatus : uint8
{
	Invalid,
	Applied,
	GameplayBlocked,
	LifecycleUnavailable,
	RunUnavailable,
	EventIdentityInvalid,
	SampleRejected,
	OperationCaptureRejected,
	LifecycleRejected
};

/** Audit proof that one anchor event samples and delegates at most once. */
struct Fdemo_mapShanmenFormationAnchorInputResult
{
	Edemo_mapShanmenFormationAnchorInputStatus Status =
		Edemo_mapShanmenFormationAnchorInputStatus::Invalid;
	int32 SampleCount = 0;
	int32 LifecycleInvocationCount = 0;
	FGuid InputEventId;
	FGuid RunId;
	FGuid AttemptId;
	Fdemo_mapShanmenFormationAnchorInputSample Sample;
	Fdemo_mapShanmenFormationAnchorOperation Operation;
	Fdemo_mapShanmenFormationAnchorOperationResult Lifecycle;
	FString Diagnostic;

	bool IsValid() const;
	bool IsAccepted() const;
};

/**
 * Stateless device-to-lifecycle seam for future physical formation controls.
 *
 * The caller owns gameplay gating, route availability, stable event identity,
 * one sample and the concrete lifecycle invocation. This adapter derives the
 * canonical intent/attempt identities and delegates exactly once. It owns no
 * key, UI, World, Actor, GameMode, inventory, sequence, retry or product state.
 */
struct Fdemo_mapShanmenFormationInputAdapter
{
	using FSampleStart = TFunctionRef<
		Fdemo_mapShanmenFormationStartInputSample()>;
	using FRouteStart = TFunctionRef<
		Fdemo_mapShanmenFormationControllerResult(
			const Fdemo_mapShanmenFormationIntent&)>;
	using FSampleAnchor = TFunctionRef<
		Fdemo_mapShanmenFormationAnchorInputSample()>;
	using FRouteAnchor = TFunctionRef<
		Fdemo_mapShanmenFormationAnchorOperationResult(
			const Fdemo_mapShanmenFormationAnchorOperation&)>;

	static FGuid MakeIntentId(
		const FGuid& RunId,
		const FGuid& InputEventId);
	static FGuid MakeAnchorAttemptId(
		const FGuid& RunId,
		const FGuid& InputEventId);

	static Fdemo_mapShanmenFormationStartInputResult RouteStartInput(
		bool bGameplayInputAllowed,
		bool bLifecycleAvailable,
		const FGuid& RunId,
		const FGuid& OwnerId,
		const FGuid& InputEventId,
		FSampleStart SampleStart,
		FRouteStart RouteLifecycle);

	static Fdemo_mapShanmenFormationAnchorInputResult RouteAnchorInput(
		bool bGameplayInputAllowed,
		bool bLifecycleAvailable,
		const FGuid& RunId,
		const FGuid& InputEventId,
		FSampleAnchor SampleAnchor,
		FRouteAnchor RouteLifecycle);
};
