#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenDivineSenseProductRoute.h"

class AActor;
class Fdemo_mapCombatRunCoordinator;
class UWorld;

enum class Edemo_mapShanmenDivineSenseLogicalInputAvailabilityState : uint8
{
	Inactive,
	UseReady,
	ProductUnavailable,
	RetryRequired
};

/** Pointer-free read model for the next legal logical input operation. */
class Fdemo_mapShanmenDivineSenseLogicalInputAvailability
{
public:
	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenDivineSenseLogicalInputAvailability& Other)
		const;
	bool CanUse() const
	{
		return IsValid()
			&& State
				== Edemo_mapShanmenDivineSenseLogicalInputAvailabilityState::
					UseReady;
	}
	bool CanRetry() const
	{
		return IsValid()
			&& State
				== Edemo_mapShanmenDivineSenseLogicalInputAvailabilityState::
					RetryRequired;
	}
	bool CanCancel() const { return CanRetry(); }

	const FGuid& GetProjectionId() const { return ProjectionId; }
	Edemo_mapShanmenDivineSenseLogicalInputAvailabilityState GetState() const
	{
		return State;
	}
	const FGuid& GetControllerId() const { return ControllerId; }
	const FGuid& GetRunId() const { return RunId; }
	const Fdemo_mapShanmenDivineSenseProductAvailability&
	GetProductAvailability() const
	{
		return ProductAvailability;
	}
	const Fdemo_mapShanmenDivineSenseProductUseAttempt* GetPendingAttempt()
		const
	{
		return CanRetry() ? &PendingAttempt : nullptr;
	}

private:
	friend class Fdemo_mapShanmenDivineSenseLogicalInputAdapter;

	FGuid ProjectionId;
	Edemo_mapShanmenDivineSenseLogicalInputAvailabilityState State =
		Edemo_mapShanmenDivineSenseLogicalInputAvailabilityState::Inactive;
	FGuid ControllerId;
	FGuid RunId;
	Fdemo_mapShanmenDivineSenseProductAvailability ProductAvailability;
	Fdemo_mapShanmenDivineSenseProductUseAttempt PendingAttempt;
};

enum class Edemo_mapShanmenDivineSenseLogicalInputStatus : uint8
{
	Invalid,
	Applied,
	Replayed,
	AdapterInactive,
	AdapterInvalid,
	BindingMismatch,
	Busy,
	ProductUnavailable,
	RetryUnavailable,
	RetryRequired,
	ProductRejected,
	StateDesynchronized
};

/** Audit evidence for exactly one logical use or explicit retry decision. */
struct Fdemo_mapShanmenDivineSenseLogicalInputResult
{
	Edemo_mapShanmenDivineSenseLogicalInputStatus Status =
		Edemo_mapShanmenDivineSenseLogicalInputStatus::Invalid;
	FString Diagnostic;
	bool bRetryAttempt = false;
	bool bProductRouteInvoked = false;
	bool bPendingRetryStored = false;
	Fdemo_mapShanmenDivineSenseLogicalInputAvailability AvailabilityBefore;
	Fdemo_mapShanmenDivineSenseLogicalInputAvailability AvailabilityAfter;
	Fdemo_mapShanmenDivineSenseProductRouteResult ProductRoute;

	bool IsValid() const;
	bool IsAccepted() const;
	bool RequiresRetry() const
	{
		return IsValid()
			&& Status
				== Edemo_mapShanmenDivineSenseLogicalInputStatus::RetryRequired;
	}
};

/** Immutable proof that one retained route-issued attempt was abandoned. */
class Fdemo_mapShanmenDivineSenseLogicalInputCancellation
{
public:
	bool IsValid() const;
	const FGuid& GetControllerId() const { return ControllerId; }
	const FGuid& GetRunId() const { return RunId; }
	const Fdemo_mapShanmenDivineSenseProductUseAttempt& GetAttempt() const
	{
		return Attempt;
	}

private:
	friend class Fdemo_mapShanmenDivineSenseLogicalInputAdapter;

	FGuid ControllerId;
	FGuid RunId;
	Fdemo_mapShanmenDivineSenseProductUseAttempt Attempt;
};

/** Run-teardown evidence emitted before the adapter releases its binding. */
class Fdemo_mapShanmenDivineSenseLogicalInputEndSummary
{
public:
	bool IsValid() const;
	bool HadPendingRetry() const { return PendingAttempt.IsValid(); }
	const FGuid& GetControllerId() const { return ControllerId; }
	const FGuid& GetRunId() const { return RunId; }
	const Fdemo_mapShanmenDivineSenseProductUseAttempt& GetPendingAttempt()
		const
	{
		return PendingAttempt;
	}

private:
	friend class Fdemo_mapShanmenDivineSenseLogicalInputAdapter;

	FGuid ControllerId;
	FGuid RunId;
	Fdemo_mapShanmenDivineSenseProductUseAttempt PendingAttempt;
};

/**
 * Run-scoped logical-input seam in front of the P19.8 product route.
 *
 * One use call delegates at most once. A route-issued attempt is retained only
 * after a typed Controller rejection and occupies the sole retry slot. While
 * occupied, new use calls fail Busy before World/provider access or identity
 * allocation. Retry is explicit, reuses that exact attempt and never loops.
 * Cancel and teardown preserve value-only evidence. This adapter owns no key,
 * UI, Actor discovery, product policy, resource, World or replay authority.
 */
class Fdemo_mapShanmenDivineSenseLogicalInputAdapter
{
public:
	bool TryBegin(
		const Fdemo_mapShanmenDivineSenseProductController& Controller,
		const Fdemo_mapCombatRunCoordinator& Coordinator,
		FString& OutDiagnostic);

	Fdemo_mapShanmenDivineSenseLogicalInputResult TryUse(
		Fdemo_mapShanmenDivineSenseProductController& Controller,
		Fdemo_mapCombatRunCoordinator& Coordinator,
		UWorld* World,
		AActor* SourceActor,
		const TArray<AActor*>& SubjectActors,
		const Idemo_mapShanmenDivineSenseWorldEvidenceProvider&
			EvidenceProvider);

	Fdemo_mapShanmenDivineSenseLogicalInputResult TryRetry(
		Fdemo_mapShanmenDivineSenseProductController& Controller,
		const Fdemo_mapCombatRunCoordinator& Coordinator,
		UWorld* World,
		AActor* SourceActor,
		const TArray<AActor*>& SubjectActors,
		const Idemo_mapShanmenDivineSenseWorldEvidenceProvider&
			EvidenceProvider);

	bool TryCancelPending(
		Fdemo_mapShanmenDivineSenseLogicalInputCancellation& OutCancellation,
		FString& OutDiagnostic);
	bool TryProjectAvailability(
		const Fdemo_mapShanmenDivineSenseProductController& Controller,
		const Fdemo_mapCombatRunCoordinator& Coordinator,
		Fdemo_mapShanmenDivineSenseLogicalInputAvailability& OutAvailability,
		FString& OutDiagnostic) const;
	bool TryEnd(
		const Fdemo_mapShanmenDivineSenseProductController& Controller,
		const Fdemo_mapCombatRunCoordinator& Coordinator,
		Fdemo_mapShanmenDivineSenseLogicalInputEndSummary& OutSummary,
		FString& OutDiagnostic);

	/** Clears canonical inactive state; active state must TryEnd. */
	bool Reset();
	bool IsValid() const;
	bool IsActive() const { return bActive; }
	bool IsEmpty() const
	{
		return !bActive && !ControllerId.IsValid() && !RunId.IsValid()
			&& !PendingAttempt.IsValid();
	}
	bool HasPendingRetry() const { return PendingAttempt.IsValid(); }
	const FGuid& GetControllerId() const { return ControllerId; }
	const FGuid& GetRunId() const { return RunId; }
	const Fdemo_mapShanmenDivineSenseProductUseAttempt* GetPendingAttempt()
		const
	{
		return HasPendingRetry() ? &PendingAttempt : nullptr;
	}

private:
	friend class Fdemo_mapShanmenDivineSenseLogicalInputAvailability;

	static FGuid MakeAvailabilityId(
		const Fdemo_mapShanmenDivineSenseLogicalInputAvailability&
			Availability);
	bool ValidateBinding(
		const Fdemo_mapShanmenDivineSenseProductController& Controller,
		const Fdemo_mapCombatRunCoordinator& Coordinator,
		FString& OutDiagnostic) const;
	bool TryBuildAvailability(
		const Fdemo_mapShanmenDivineSenseProductController& Controller,
		const Fdemo_mapCombatRunCoordinator& Coordinator,
		Fdemo_mapShanmenDivineSenseLogicalInputAvailability& OutAvailability,
		FString& OutDiagnostic) const;
	static bool IsRetryableRejection(
		const Fdemo_mapShanmenDivineSenseProductRouteResult& Result);

	FGuid ControllerId;
	FGuid RunId;
	Fdemo_mapShanmenDivineSenseProductUseAttempt PendingAttempt;
	bool bActive = false;
};
