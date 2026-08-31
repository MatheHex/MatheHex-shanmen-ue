#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenSwordRhythmEffectCueExecutionCommandHost.h"
#include "demo_mapShanmenSwordRhythmProductSession.h"

/**
 * Frozen proof that one EffectCue event came from a valid ProductSession
 * presentation state and that Session's canonical cue policy.
 *
 * Callers may retain these values while the ProductSession advances, then
 * assemble an ordered batch explicitly. The projection owns no Session,
 * ledger, executor, sequence or retry state.
 */
class Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductProjection
{
public:
	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductProjection&
			Other) const;
	bool MatchesCurrentSession(
		const Fdemo_mapShanmenSwordRhythmProductSession& Session) const;

	const FGuid& GetRunId() const { return RunId; }
	const FGuid& GetConfigId() const { return ConfigId; }
	const FGuid& GetCuePolicyId() const { return CuePolicyId; }
	int32 GetObservationRevision() const
	{
		return Event.GetState().GetObservationRevision();
	}
	const Fdemo_mapShanmenSwordRhythmEffectCueEvent& GetEvent() const
	{
		return Event;
	}

private:
	friend class
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductProjector;

	FGuid RunId;
	FGuid ConfigId;
	FGuid CuePolicyId;
	Fdemo_mapShanmenSwordRhythmEffectCueEvent Event;
};

enum class
	Edemo_mapShanmenSwordRhythmEffectCueExecutionProductProjectionStatus
	: uint8
{
	Captured,
	SessionInvalid,
	PresentationUnavailable,
	IdentityMismatch,
	AdapterRejected,
	ProjectionRejected
};

struct Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductProjectionResult
{
	Edemo_mapShanmenSwordRhythmEffectCueExecutionProductProjectionStatus Status =
		Edemo_mapShanmenSwordRhythmEffectCueExecutionProductProjectionStatus::
			SessionInvalid;
	FString Diagnostic;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductProjection Projection;

	bool IsCaptured() const
	{
		return Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductProjectionStatus::
					Captured
			&& Projection.IsValid();
	}
};

/** Stateless ProductSession read-model to frozen EffectCue projection seam. */
class Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductProjector
{
public:
	static
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductProjectionResult
	CaptureCurrent(
		const Fdemo_mapShanmenSwordRhythmProductSession& Session);
};

/**
 * Frozen Create request assembled from caller-retained Product projections.
 *
 * All projections must share Run/config/cue-policy identity and be strictly
 * increasing by observation revision. Sequence zero remains explicit: this
 * request can only establish a fresh P12.19 command Host. The request keeps
 * exact replay possible even after the source ProductSession has advanced.
 */
class Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductCreateRequest
{
public:
	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductCreateRequest&
			Other) const;

	const FGuid& GetRunId() const { return RunId; }
	const FGuid& GetConfigId() const { return ConfigId; }
	const FGuid& GetCuePolicyId() const { return CuePolicyId; }
	const TArray<
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductProjection>&
	GetProjections() const
	{
		return Projections;
	}
	const Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope&
	GetEnvelope() const
	{
		return Envelope;
	}

private:
	friend class
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductCreateFactory;

	FGuid RunId;
	FGuid ConfigId;
	FGuid CuePolicyId;
	TArray<Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductProjection>
		Projections;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope Envelope;
};

enum class
	Edemo_mapShanmenSwordRhythmEffectCueExecutionProductCreateCaptureStatus
	: uint8
{
	Captured,
	TransportIdentityInvalid,
	ProjectionInvalid,
	ProjectionIdentityMismatch,
	ProjectionOrderInvalid,
	CommandRejected,
	EnvelopeRejected,
	RequestRejected
};

struct Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductCreateCaptureResult
{
	Edemo_mapShanmenSwordRhythmEffectCueExecutionProductCreateCaptureStatus
		Status =
			Edemo_mapShanmenSwordRhythmEffectCueExecutionProductCreateCaptureStatus::
				TransportIdentityInvalid;
	FString Diagnostic;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductCreateRequest Request;

	bool IsCaptured() const
	{
		return Status
				== Edemo_mapShanmenSwordRhythmEffectCueExecutionProductCreateCaptureStatus::
					Captured
			&& Request.IsValid();
	}
};

/** Stateless ordered-projection to P12.19 Create-envelope capture. */
class Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductCreateFactory
{
public:
	static Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductCreateCaptureResult
	Capture(
		const FGuid& HostId,
		int64 Sequence,
		const FGuid& CommandId,
		const TArray<
			Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductProjection>&
			Projections,
		const FGuid& VisualConsumerId,
		const FGuid& AudioConsumerId);
};

enum class Edemo_mapShanmenSwordRhythmEffectCueExecutionProductRouteStatus
	: uint8
{
	Created,
	Replayed,
	RequestInvalid,
	HostRejected,
	StateInvalid
};

/** Product-route evidence around exactly one P12.19 Create dispatch. */
struct Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRouteResult
{
	Edemo_mapShanmenSwordRhythmEffectCueExecutionProductRouteStatus Status =
		Edemo_mapShanmenSwordRhythmEffectCueExecutionProductRouteStatus::
			RequestInvalid;
	FGuid RunId;
	FGuid ConfigId;
	FGuid CuePolicyId;
	FGuid HostId;
	FGuid DispatchId;
	int32 EventCount = 0;
	bool bReplay = false;
	FString Diagnostic;
	Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHostResult Host;

	bool IsSuccess() const;
};

/**
 * Stateless production seam for a frozen Product Create request.
 *
 * It delegates ordering, replay and lifecycle authority to the caller-owned
 * P12.19 Host. ProcessNext and End remain explicit Host operations so this
 * seam cannot become a second sequence, retry or executor owner.
 */
class Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRoute
{
public:
	static Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRouteResult
	TryCreate(
		const Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductCreateRequest&
			Request,
		Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandHost& Host);
};
