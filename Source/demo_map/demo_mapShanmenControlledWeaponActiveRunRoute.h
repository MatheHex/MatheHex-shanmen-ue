#pragma once

#include "CoreMinimal.h"

#include "demo_mapShanmenControlledWeaponAdapter.h"
#include "demo_mapShanmenControlledWeaponRunHost.h"

class AActor;
class UPrimitiveComponent;
class Udemo_mapItemSubsystem;
class Udemo_mapShanmenItemAuthoritySubsystem;
class Fdemo_mapCombatRunCoordinator;

/** Caller-authored values for binding one deployed item to the active Run. */
struct Fdemo_mapShanmenControlledWeaponActiveRunIntent
{
	FGuid SourceItemInstanceId;
	uint64 ActivationSequence = 0;
	FShanmenControlledWeaponDefinitionCapture Definition;
	float ControlPower = 0.0f;
	FGameplayTagContainer SourceTags;
	Fdemo_mapShanmenControlledWeaponMotionCapture Motion;

	bool IsValid() const;
};

enum class Edemo_mapShanmenControlledWeaponActiveRunError : uint8
{
	None,
	DependenciesUnavailable,
	IntentInvalid,
	CoordinatorNotReady,
	SourceMismatch,
	PreparationRejected,
	AttachmentRejected,
	ResultInvalid
};

/** Complete evidence for one authority-backed controlled-weapon start. */
struct Fdemo_mapShanmenControlledWeaponActiveRunResult
{
	Edemo_mapShanmenControlledWeaponActiveRunError Error =
		Edemo_mapShanmenControlledWeaponActiveRunError::
			DependenciesUnavailable;
	FString Diagnostic;
	Fdemo_mapShanmenControlledWeaponPrepareResult Preparation;
	Fdemo_mapShanmenControlledWeaponHostAttachResult Attachment;

	bool IsStarted() const;
};

/**
 * Single product route from durable active-Run item evidence into the P6 Host.
 *
 * Player entity identity is derived from the canonical Coordinator. The route
 * never creates inventory state, deploys an unprepared item, or spawns Actors.
 */
struct Fdemo_mapShanmenControlledWeaponActiveRunRoute
{
	static Fdemo_mapShanmenControlledWeaponActiveRunResult TryStart(
		const Udemo_mapShanmenItemAuthoritySubsystem* Authority,
		const Udemo_mapItemSubsystem* Runtime,
		Fdemo_mapCombatRunCoordinator& Coordinator,
		Fdemo_mapShanmenControlledWeaponRunHost& Host,
		AActor* SourceActor,
		AActor* WeaponActor,
		UPrimitiveComponent* WeaponCollisionRoot,
		const Fdemo_mapShanmenControlledWeaponActiveRunIntent& Intent);
};
