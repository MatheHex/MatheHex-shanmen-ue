#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenFormationInfluenceLifecycleCommandHost.h"

class FShanmenWorldEntityRegistry;

enum class Edemo_mapShanmenFormationInfluenceConsumerWorldResolutionStatus
	: uint8
{
	Resolved,
	ExpectedRunIdInvalid,
	RegistryUnavailable,
	RegistryRunMismatch,
	AttributeComponentUnavailable,
	BodyIndexInvalid,
	EntityNotFound,
	ResolutionRejected,
	StateInvalid
};

/**
 * Auditable, pointer-free result of resolving one caller-owned attribute
 * capability through the existing run-scoped World entity registry.
 */
struct Fdemo_mapShanmenFormationInfluenceConsumerWorldResolutionResult
{
	Edemo_mapShanmenFormationInfluenceConsumerWorldResolutionStatus Status =
		Edemo_mapShanmenFormationInfluenceConsumerWorldResolutionStatus::
			ExpectedRunIdInvalid;
	FString Diagnostic;
	bool bRegistryChecked = false;
	FGuid ExpectedRunId;
	FGuid RegistryRunId;
	FGuid ResolvedEntityId;
	int32 BodyIndex = INDEX_NONE;
	Fdemo_mapShanmenFormationInfluenceConsumerSubjectResolution Resolution;

	bool IsSuccess() const;
};

/**
 * Stateless caller adapter from an already-held AttributeComponent capability
 * to the pointer-free consumer resolution consumed by LifecycleCommandHost.
 *
 * The exact component must already be bound in the supplied registry. This
 * adapter never scans Actors, discovers components, retains UObject pointers,
 * mutates the registry, or invokes the consumer runtime.
 */
class Fdemo_mapShanmenFormationInfluenceConsumerWorldResolver
{
public:
	static Fdemo_mapShanmenFormationInfluenceConsumerWorldResolutionResult
	Resolve(
		const FShanmenWorldEntityRegistry& EntityRegistry,
		const FGuid& ExpectedRunId,
		const Udemo_mapAttributeComponent* AttributeComponent,
		int32 BodyIndex = INDEX_NONE);
};
