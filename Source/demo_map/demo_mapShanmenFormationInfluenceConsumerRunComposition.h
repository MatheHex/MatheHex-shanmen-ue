#pragma once

#include "CoreMinimal.h"
#include "demo_mapCombatRunCoordinator.h"
#include "demo_mapShanmenFormationInfluenceConsumerWorldResolution.h"

enum class
	Edemo_mapShanmenFormationInfluenceConsumerRunCompositionStatus : uint8
{
	Activated,
	ActivationReplayed,
	AliasRejected,
	WorldResolutionRejected,
	DeliveryApplicationRejected,
	StateInvalid
};

/**
 * Pointer-free evidence for one explicit combat-Run consumer activation.
 *
 * Every nested receipt remains visible. Engine-object pointers are borrowed
 * only by TryActivate and are never retained in this result.
 */
struct Fdemo_mapShanmenFormationInfluenceConsumerRunCompositionResult
{
	Edemo_mapShanmenFormationInfluenceConsumerRunCompositionStatus Status =
		Edemo_mapShanmenFormationInfluenceConsumerRunCompositionStatus::
			AliasRejected;
	FString Diagnostic;
	FGuid RunId;
	bool bAliasChecked = false;
	bool bWorldResolutionChecked = false;
	bool bDeliveryAttempted = false;
	Fdemo_mapCombatRunEntityAliasResult Alias;
	Fdemo_mapShanmenFormationInfluenceConsumerWorldResolutionResult
		WorldResolution;
	Fdemo_mapShanmenFormationInfluenceConsumerDeliveryApplicationResult
		Application;

	bool IsSuccess() const;
};

/**
 * First explicit product composition command for formation consumers.
 *
 * The caller supplies every live capability. This command performs no World
 * scan, component discovery, polling, scheduling, retry, or pointer storage:
 * it composes CombatRunCoordinator aliasing, registry-backed subject
 * resolution, and LifecycleCommandHost delivery application exactly once.
 */
class Fdemo_mapShanmenFormationInfluenceConsumerRunComposition
{
public:
	static
	Fdemo_mapShanmenFormationInfluenceConsumerRunCompositionResult TryActivate(
		Fdemo_mapCombatRunCoordinator& CombatRun,
		const UObject* RegisteredSubjectObject,
		const Fdemo_mapShanmenFormationProductHost& ProductHost,
		Fdemo_mapShanmenFormationInfluenceLifecycleCommandHost& LifecycleHost,
		const Fdemo_mapShanmenFormationInfluenceConsumerCommandDelivery&
			Delivery,
		Udemo_mapAttributeComponent* AttributeComponent,
		int32 RegisteredBodyIndex = INDEX_NONE,
		int32 AttributeBodyIndex = INDEX_NONE);
};
