#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenSwordQiItemAdapter.h"
#include "demo_mapShanmenSwordQiProductSession.h"

class Fdemo_mapCombatRunCoordinator;
class Fdemo_mapItemAuthority;
class Udemo_mapAttributeComponent;
class UWorld;

/** Stable device-independent request captured before any product authority. */
class Fdemo_mapShanmenSwordQiIntent
{
public:
	static bool TryCapture(
		const FGuid& RequestedIntentId,
		const FGuid& RequestedRunId,
		const FVector& RequestedOrigin,
		const FVector& RequestedAimDirection,
		Fdemo_mapShanmenSwordQiIntent& OutIntent);

	bool IsValid() const;
	bool Matches(const Fdemo_mapShanmenSwordQiIntent& Other) const;
	const FGuid& GetIntentId() const { return IntentId; }
	const FGuid& GetRunId() const { return RunId; }
	const FVector& GetOrigin() const { return Origin; }
	const FVector& GetAimDirection() const { return AimDirection; }

private:
	FGuid IntentId;
	FGuid RunId;
	FVector Origin = FVector::ZeroVector;
	FVector AimDirection = FVector::ZeroVector;
};

enum class Edemo_mapShanmenSwordQiControllerStatus : uint8
{
	Applied,
	ControllerInactive,
	ControllerInvalid,
	IntentInvalid,
	RunMismatch,
	IntentIdConflict,
	ItemAuthorizationRejected,
	AttackPowerUnavailable,
	ProductDependenciesUnavailable,
	ProductCaptureRejected,
	RouteRejected
};

/** Complete proof from equipped-item read through the P18.3 product route. */
struct Fdemo_mapShanmenSwordQiControllerResult
{
	Edemo_mapShanmenSwordQiControllerStatus Status =
		Edemo_mapShanmenSwordQiControllerStatus::ControllerInactive;
	bool bReusedIntent = false;
	FGuid IntentId;
	FGuid RunId;
	Fdemo_mapShanmenSwordQiItemResult Item;
	float AttackPower = 0.0f;
	Fdemo_mapShanmenSwordQiProductStartResult Start;
	Fdemo_mapShanmenSwordQiProductRouteResult Route;
	FString Diagnostic;

	bool IsAccepted() const;
};

struct Fdemo_mapShanmenSwordQiControllerEndSummary
{
	FGuid RunId;
	int32 CapturedIntentCount = 0;
	int32 ProcessedCommandCount = 0;
	bool bInterruptedFlight = false;
	Fdemo_mapShanmenSwordQiTerminalReceipt TerminalReceipt;

	bool IsValid() const;
};

/**
 * The sole Run-scoped composition owner for player Sword Qi.
 *
 * A new IntentId samples the current equipped-sword authority and final
 * AttackPower exactly once, then reserves exactly one Run sequence. Exact
 * replay uses the frozen command even if equipment or attributes later move.
 * No input device is referenced and no inventory state is mutated here.
 */
class Fdemo_mapShanmenSwordQiProductController
{
public:
	bool TryBegin(const FGuid& RequestedRunId, FString& OutDiagnostic);

	Fdemo_mapShanmenSwordQiControllerResult TrySubmit(
		UWorld* World,
		TSubclassOf<Ademo_mapShanmenSwordQiProjectile> ProjectileClass,
		const Fdemo_mapItemAuthority& ItemAuthority,
		const Udemo_mapAttributeComponent& Attributes,
		Fdemo_mapCombatRunCoordinator& Coordinator,
		AActor* SourceActor,
		const Fdemo_mapShanmenSwordQiIntent& Intent,
		TFunctionRef<Fdemo_mapShanmenPlayerActionGateResult()>
			AuthorizeAction);

	bool TryInterrupt();
	bool TryExpireRange();
	bool TryRetireTerminal(
		Fdemo_mapShanmenSwordQiTerminalReceipt& OutReceipt);
	bool TryAppendOccupancy(
		Fdemo_mapShanmenPlayerActionOccupancySnapshot& InOutSnapshot) const;
	bool TryEnd(
		const FGuid& ExpectedRunId,
		Fdemo_mapShanmenSwordQiControllerEndSummary& OutSummary,
		FString& OutDiagnostic);
	void Reset();

	bool IsValid() const;
	bool IsActive() const { return RunId.IsValid(); }
	bool IsEmpty() const;
	const FGuid& GetRunId() const { return RunId; }
	int32 NumCapturedIntents() const { return CapturedIntents.Num(); }
	const Fdemo_mapShanmenSwordQiProductSession& GetSession() const
	{
		return Session;
	}
	const Fdemo_mapShanmenSwordQiLaunchCommand* FindCapturedCommand(
		const FGuid& IntentId) const;

private:
	struct FCapturedIntent
	{
		Fdemo_mapShanmenSwordQiIntent Intent;
		Fdemo_mapShanmenSwordQiItemResult Item;
		float AttackPower = 0.0f;
		Fdemo_mapShanmenSwordQiProductStartResult Start;
		Fdemo_mapShanmenSwordQiProductRouteResult LastRoute;
	};

	Fdemo_mapShanmenSwordQiControllerResult RouteCaptured(
		UWorld* World,
		TSubclassOf<Ademo_mapShanmenSwordQiProjectile> ProjectileClass,
		Fdemo_mapCombatRunCoordinator& Coordinator,
		AActor* SourceActor,
		FCapturedIntent& Captured,
		bool bReusedIntent,
		TFunctionRef<Fdemo_mapShanmenPlayerActionGateResult()>
			AuthorizeAction);

	FGuid RunId;
	TMap<FGuid, FCapturedIntent> CapturedIntents;
	Fdemo_mapShanmenSwordQiProductSession Session;
};
