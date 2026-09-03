#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenDivineSenseProductSession.h"

class AActor;
class Fdemo_mapCombatRunCoordinator;
class UWorld;

/** Product-owned Divine Sense values frozen once for one Combat Run. */
class Fdemo_mapShanmenDivineSenseProductConfig
{
public:
	static bool TryCapture(
		const FShanmenDivineSenseDefinition& Definition,
		const FShanmenActionResourceCost& Cost,
		int32 PulseCapacity,
		Fdemo_mapShanmenDivineSenseProductConfig& OutConfig);

	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenDivineSenseProductConfig& Other) const;
	const FGuid& GetConfigId() const { return ConfigId; }
	const FShanmenDivineSenseDefinition& GetDefinition() const
	{
		return Definition;
	}
	const FShanmenActionResourceCost& GetCost() const { return Cost; }
	int32 GetPulseCapacity() const { return PulseCapacity; }

private:
	FGuid ConfigId;
	FShanmenDivineSenseDefinition Definition;
	FShanmenActionResourceCost Cost;
	int32 PulseCapacity = 0;
};

/**
 * Stable device-independent request for one caller-selected Divine Sense batch.
 *
 * Subject Actors stay outside the value object. The first successful command
 * capture permanently binds this IntentId to the Router's canonical entity set.
 */
class Fdemo_mapShanmenDivineSenseProductIntent
{
public:
	static bool TryCapture(
		const FGuid& RequestedIntentId,
		const FShanmenCombatActionSnapshot& Action,
		int32 ScanOrdinal,
		int32 SubjectActorBudget,
		Fdemo_mapShanmenDivineSenseProductIntent& OutIntent);

	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenDivineSenseProductIntent& Other) const;
	const FGuid& GetIntentId() const { return IntentId; }
	const FGuid& GetRunId() const { return Action.GetRunId(); }
	const FShanmenCombatActionSnapshot& GetAction() const { return Action; }
	int32 GetScanOrdinal() const { return ScanOrdinal; }
	int32 GetSubjectActorBudget() const { return SubjectActorBudget; }

private:
	FGuid IntentId;
	FShanmenCombatActionSnapshot Action;
	int32 ScanOrdinal = INDEX_NONE;
	int32 SubjectActorBudget = INDEX_NONE;
};

/** Pointer-free product view over Session resources and Controller capacity. */
class Fdemo_mapShanmenDivineSenseProductAvailability
{
public:
	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenDivineSenseProductAvailability& Other) const;
	bool CanCaptureNewIntent() const;

	const FGuid& GetAvailabilityId() const { return AvailabilityId; }
	const FGuid& GetControllerId() const { return ControllerId; }
	const Fdemo_mapShanmenDivineSenseProductConfig& GetConfig() const
	{
		return Config;
	}
	const Fdemo_mapShanmenDivineSenseAvailabilityProjection&
	GetSessionAvailability() const
	{
		return SessionAvailability;
	}
	int32 GetCapturedIntentCount() const { return CapturedIntentCount; }
	int32 GetRemainingIntentCapacity() const
	{
		return IsValid()
			? Config.GetPulseCapacity() - CapturedIntentCount
			: 0;
	}

private:
	friend class Fdemo_mapShanmenDivineSenseProductController;

	FGuid AvailabilityId;
	FGuid ControllerId;
	Fdemo_mapShanmenDivineSenseProductConfig Config;
	Fdemo_mapShanmenDivineSenseAvailabilityProjection SessionAvailability;
	int32 CapturedIntentCount = INDEX_NONE;
};

enum class Edemo_mapShanmenDivineSenseProductControllerState : uint8
{
	Empty,
	Active,
	Ended
};

enum class Edemo_mapShanmenDivineSenseProductControllerStatus : uint8
{
	Invalid,
	Applied,
	AlreadyApplied,
	ControllerNotActive,
	ControllerInvalid,
	CoordinatorNotReady,
	RunMismatch,
	SourceMismatch,
	IntentInvalid,
	IntentIdConflict,
	IntentCapacityExceeded,
	CommandCaptureRejected,
	RouteRejected,
	StateDesynchronized
};

/** Product proof joining one stable Intent to the sole P19.5 route. */
struct Fdemo_mapShanmenDivineSenseProductControllerResult
{
	Edemo_mapShanmenDivineSenseProductControllerStatus Status =
		Edemo_mapShanmenDivineSenseProductControllerStatus::Invalid;
	FString Diagnostic;
	bool bReusedIntent = false;
	FGuid ControllerId;
	FGuid RunId;
	Fdemo_mapShanmenDivineSenseProductIntent Intent;
	Fdemo_mapShanmenDivineSenseRouteCommand Command;
	Fdemo_mapShanmenDivineSenseProductAvailability AvailabilityBefore;
	Fdemo_mapShanmenDivineSenseProductAvailability AvailabilityAfter;
	Fdemo_mapShanmenDivineSenseSessionRouteResult Route;

	bool IsValid() const;
	bool IsAccepted() const;
	bool IsReplay() const
	{
		return Status
			== Edemo_mapShanmenDivineSenseProductControllerStatus::
				AlreadyApplied;
	}
};

/** Immutable product-level proof retained after Session teardown. */
class Fdemo_mapShanmenDivineSenseProductControllerEndReceipt
{
public:
	bool IsValid() const;
	const FGuid& GetReceiptId() const { return ReceiptId; }
	const FGuid& GetControllerId() const { return ControllerId; }
	const FGuid& GetConfigId() const { return ConfigId; }
	const FGuid& GetRunId() const { return RunId; }
	const FGuid& GetSourceEntityId() const { return SourceEntityId; }
	const FGuid& GetSessionEndReceiptId() const
	{
		return SessionEndReceiptId;
	}
	int32 GetCapturedIntentCount() const { return CapturedIntentCount; }

private:
	friend class Fdemo_mapShanmenDivineSenseProductController;

	FGuid ReceiptId;
	FGuid ControllerId;
	FGuid ConfigId;
	FGuid RunId;
	FGuid SourceEntityId;
	FGuid SessionEndReceiptId;
	int32 CapturedIntentCount = INDEX_NONE;
};

enum class Edemo_mapShanmenDivineSenseProductControllerEndStatus : uint8
{
	Invalid,
	Ended,
	AlreadyEnded,
	ControllerNotActive,
	ControllerInvalid,
	ControllerMismatch,
	SessionRejected,
	StateDesynchronized
};

struct Fdemo_mapShanmenDivineSenseProductControllerEndResult
{
	Edemo_mapShanmenDivineSenseProductControllerEndStatus Status =
		Edemo_mapShanmenDivineSenseProductControllerEndStatus::Invalid;
	FString Diagnostic;
	Fdemo_mapShanmenDivineSenseSessionEndResult SessionEnd;
	Fdemo_mapShanmenDivineSenseProductControllerEndReceipt Receipt;

	bool IsValid() const;
	bool IsSuccess() const;
	bool IsReplay() const
	{
		return Status
			== Edemo_mapShanmenDivineSenseProductControllerEndStatus::
				AlreadyEnded;
	}
};

/**
 * Sole Run-scoped product composition owner for Divine Sense.
 *
 * Begin freezes definition, cost and bounded capacity. A first IntentId capture
 * delegates command creation to P19.5 and retains that command before route, so
 * a transient World/provider rejection can retry without recapturing policy or
 * identity. Exact accepted replay remains free of live World, Actor and provider
 * reads. Input binding, Actor discovery, UI and final balance stay outside.
 */
class Fdemo_mapShanmenDivineSenseProductController
{
public:
	bool TryBegin(
		const Fdemo_mapCombatRunCoordinator& Coordinator,
		const FShanmenActionResourceSnapshot& OpeningSpiritEnergy,
		const Fdemo_mapShanmenDivineSenseProductConfig& Config,
		FString& OutDiagnostic);

	bool TryCaptureAvailability(
		const Fdemo_mapCombatRunCoordinator& Coordinator,
		Fdemo_mapShanmenDivineSenseProductAvailability& OutAvailability,
		FString& OutDiagnostic) const;
	bool IsAvailabilityCurrent(
		const Fdemo_mapCombatRunCoordinator& Coordinator,
		const Fdemo_mapShanmenDivineSenseProductAvailability& Availability)
		const;

	Fdemo_mapShanmenDivineSenseProductControllerResult TrySubmit(
		const Fdemo_mapCombatRunCoordinator& Coordinator,
		UWorld* World,
		AActor* SourceActor,
		const Fdemo_mapShanmenDivineSenseProductIntent& Intent,
		const TArray<AActor*>& SubjectActors,
		const Idemo_mapShanmenDivineSenseWorldEvidenceProvider&
			EvidenceProvider);

	Fdemo_mapShanmenDivineSenseProductControllerEndResult TryEnd(
		const Fdemo_mapCombatRunCoordinator& Coordinator,
		const FGuid& ExpectedControllerId);

	/** Clears only empty or already-ended state; active state must TryEnd. */
	bool Reset();
	bool IsValid() const;
	bool IsEmpty() const
	{
		return State
			== Edemo_mapShanmenDivineSenseProductControllerState::Empty;
	}
	bool IsActive() const
	{
		return State
			== Edemo_mapShanmenDivineSenseProductControllerState::Active;
	}
	bool IsEnded() const
	{
		return State
			== Edemo_mapShanmenDivineSenseProductControllerState::Ended;
	}

	Edemo_mapShanmenDivineSenseProductControllerState GetState() const
	{
		return State;
	}
	const FGuid& GetControllerId() const { return ControllerId; }
	const FGuid& GetRunId() const { return RunId; }
	const FGuid& GetSourceEntityId() const { return SourceEntityId; }
	const Fdemo_mapShanmenDivineSenseProductConfig& GetConfig() const
	{
		return Config;
	}
	const Fdemo_mapShanmenDivineSenseProductSession& GetSession() const
	{
		return Session;
	}
	const Fdemo_mapShanmenDivineSenseProductControllerEndReceipt&
	GetEndReceipt() const
	{
		return EndReceipt;
	}
	int32 NumCapturedIntents() const { return CapturedIntents.Num(); }
	const Fdemo_mapShanmenDivineSenseRouteCommand* FindCapturedCommand(
		const FGuid& IntentId) const;

private:
	struct FCapturedIntent
	{
		Fdemo_mapShanmenDivineSenseProductIntent Intent;
		Fdemo_mapShanmenDivineSenseRouteCommand Command;
		Fdemo_mapShanmenDivineSenseSessionRouteResult LastRoute;
	};

	bool TryBuildAvailability(
		const Fdemo_mapCombatRunCoordinator& Coordinator,
		Fdemo_mapShanmenDivineSenseProductAvailability& OutAvailability,
		FString& OutDiagnostic) const;
	Fdemo_mapShanmenDivineSenseProductControllerResult RouteCaptured(
		const Fdemo_mapCombatRunCoordinator& Coordinator,
		UWorld* World,
		AActor* SourceActor,
		const TArray<AActor*>& SubjectActors,
		const Idemo_mapShanmenDivineSenseWorldEvidenceProvider&
			EvidenceProvider,
		FCapturedIntent& Captured,
		bool bReusedIntent,
		const Fdemo_mapShanmenDivineSenseProductAvailability& Before);

	FGuid ControllerId;
	FGuid RunId;
	FGuid SourceEntityId;
	Fdemo_mapShanmenDivineSenseProductConfig Config;
	Fdemo_mapShanmenDivineSenseProductSession Session;
	TMap<FGuid, FCapturedIntent> CapturedIntents;
	Fdemo_mapShanmenDivineSenseProductControllerEndReceipt EndReceipt;
	Edemo_mapShanmenDivineSenseProductControllerState State =
		Edemo_mapShanmenDivineSenseProductControllerState::Empty;
};
