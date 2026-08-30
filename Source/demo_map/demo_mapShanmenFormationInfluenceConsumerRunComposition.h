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

enum class
	Edemo_mapShanmenFormationInfluenceConsumerRunDeactivationStatus : uint8
{
	Deactivated,
	DeactivationReplayed,
	ActivationEvidenceRejected,
	DeliveryApplicationRejected,
	StateInvalid
};

/**
 * Pointer-free evidence for removing the exact delivery proven by one prior
 * successful Run activation.
 *
 * Deactivation deliberately consumes the frozen activation result rather than
 * rediscovering a component or requiring the World registry to remain alive.
 * This preserves an explicit cleanup path during forward teardown recovery.
 */
struct Fdemo_mapShanmenFormationInfluenceConsumerRunDeactivationResult
{
	Edemo_mapShanmenFormationInfluenceConsumerRunDeactivationStatus Status =
		Edemo_mapShanmenFormationInfluenceConsumerRunDeactivationStatus::
			ActivationEvidenceRejected;
	FString Diagnostic;
	FGuid RunId;
	bool bActivationEvidenceChecked = false;
	bool bDeliveryAttempted = false;
	Fdemo_mapShanmenFormationInfluenceConsumerRunCompositionResult
		Activation;
	Fdemo_mapShanmenFormationInfluenceConsumerDeliveryApplicationResult
		Application;

	bool IsSuccess() const;
};

/**
 * Explicit product composition boundary for formation consumers.
 *
 * The caller supplies every live capability. Activation composes
 * CombatRunCoordinator aliasing, registry-backed subject resolution, and
 * LifecycleCommandHost delivery application exactly once. Deactivation
 * consumes the frozen activation evidence and routes its exact delivery to the
 * same product-owned lifecycle boundary. Neither path performs World scan,
 * component discovery, polling, scheduling, retry, or pointer storage.
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

	static
	Fdemo_mapShanmenFormationInfluenceConsumerRunDeactivationResult
	TryDeactivate(
		const Fdemo_mapShanmenFormationProductHost& ProductHost,
		Fdemo_mapShanmenFormationInfluenceLifecycleCommandHost& LifecycleHost,
		const Fdemo_mapShanmenFormationInfluenceConsumerRunCompositionResult&
			ActivationEvidence);
};
