#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenThrownWeaponInputChoiceInteractionPort.h"

/**
 * Immutable interaction request captured from one visible P20.20 read model.
 *
 * The request stores only the expected revisionless read-model identity and
 * one canonical P20.19 intent. It owns no authoritative state or revision.
 */
class Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest
{
public:
	static bool TryCaptureTrajectorySelection(
		const Fdemo_mapShanmenThrownWeaponInputChoiceInteractionReadModel&
			ReadModel,
		Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind TrajectoryKind,
		Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest& OutRequest);
	static bool TryCaptureArcTargetIntent(
		const Fdemo_mapShanmenThrownWeaponInputChoiceInteractionReadModel&
			ReadModel,
		const FVector2D& RawTargetIntent,
		Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest& OutRequest);
	static bool TryCaptureArcApexAdjustment(
		const Fdemo_mapShanmenThrownWeaponInputChoiceInteractionReadModel&
			ReadModel,
		double RawNormalizedDelta,
		Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest& OutRequest);
	static bool TryCaptureArcTargetClear(
		const Fdemo_mapShanmenThrownWeaponInputChoiceInteractionReadModel&
			ReadModel,
		Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest& OutRequest);

	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest&
			Other) const;
	const FGuid& GetRequestId() const { return RequestId; }
	const FGuid& GetExpectedReadModelId() const
	{
		return ExpectedReadModelId;
	}
	const Fdemo_mapShanmenThrownWeaponInputChoiceIntent& GetIntent() const
	{
		return Intent;
	}

private:
	static bool TryCapture(
		const Fdemo_mapShanmenThrownWeaponInputChoiceInteractionReadModel&
			ReadModel,
		const Fdemo_mapShanmenThrownWeaponInputChoiceIntent& Intent,
		Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest& OutRequest);

	FGuid RequestId;
	FGuid ExpectedReadModelId;
	Fdemo_mapShanmenThrownWeaponInputChoiceIntent Intent;
};

enum class Edemo_mapShanmenThrownWeaponInputChoiceInteractionRequestStatus :
	uint8
{
	Invalid,
	RequestInvalid,
	InteractionReadRejected,
	InteractionReadProtocolRejected,
	StaleReadModel,
	Routed,
	IntentRouteProtocolRejected
};

/** Immutable evidence for one stale-safe interaction request attempt. */
class Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequestResult
{
public:
	bool IsValid() const;
	bool IsAccepted() const;
	bool WasRejectedByIntentRoute() const;
	Edemo_mapShanmenThrownWeaponInputChoiceInteractionRequestStatus
	GetStatus() const { return Status; }
	const FString& GetDiagnostic() const { return Diagnostic; }
	int32 GetInteractionReadCount() const { return InteractionReadCount; }
	int32 GetIntentRouteCount() const { return IntentRouteCount; }
	const Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest&
	GetRequest() const { return Request; }
	const Fdemo_mapShanmenThrownWeaponInputChoiceInteractionReadResult&
	GetInteractionReadResult() const { return InteractionReadResult; }
	const Fdemo_mapShanmenThrownWeaponInputChoiceIntentResult&
	GetIntentResult() const { return IntentResult; }

private:
	friend struct
		Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequestCoordinator;

	Edemo_mapShanmenThrownWeaponInputChoiceInteractionRequestStatus Status =
		Edemo_mapShanmenThrownWeaponInputChoiceInteractionRequestStatus::Invalid;
	FString Diagnostic;
	int32 InteractionReadCount = 0;
	int32 IntentRouteCount = 0;
	Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest Request;
	Fdemo_mapShanmenThrownWeaponInputChoiceInteractionReadResult
		InteractionReadResult;
	Fdemo_mapShanmenThrownWeaponInputChoiceIntentResult IntentResult;
};

/**
 * Stateless stale-view fence between a P20.20 request and P20.19 route.
 *
 * It reads the current interaction projection once, compares its revisionless
 * identity with the captured request, then routes the stored intent at most
 * once. It owns no UI, key, World, Actor, state, revision, session, or retry.
 */
struct Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequestCoordinator
{
	using FReadCurrentInteraction = TFunctionRef<
		Fdemo_mapShanmenThrownWeaponInputChoiceInteractionReadResult()>;
	using FRouteIntent = TFunctionRef<
		Fdemo_mapShanmenThrownWeaponInputChoiceIntentResult(
			const Fdemo_mapShanmenThrownWeaponInputChoiceIntent&)>;

	static Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequestResult
	Execute(
		const Fdemo_mapShanmenThrownWeaponInputChoiceInteractionRequest& Request,
		FReadCurrentInteraction ReadCurrentInteraction,
		FRouteIntent RouteIntent);
};
