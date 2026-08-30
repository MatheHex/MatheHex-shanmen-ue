#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenFormationInfluenceConsumerRegistry.h"

class Udemo_mapAttributeComponent;

/** Stable evidence that one accepted consumer command reached native state. */
struct Fdemo_mapShanmenFormationInfluenceConsumerAttributeAcknowledgement
{
public:
	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenFormationInfluenceConsumerAttributeAcknowledgement&
			Other) const;

	const FGuid& GetAcknowledgementId() const { return AcknowledgementId; }
	const Fdemo_mapShanmenFormationInfluenceConsumerApplicationReceipt&
	GetApplicationReceipt() const
	{
		return ApplicationReceipt;
	}
	Edemo_mapExactModifierMutationStatus GetNativeStatus() const
	{
		return NativeStatus;
	}
	const Fdemo_mapModifierSpec& GetModifierSpec() const { return ModifierSpec; }

private:
	FGuid AcknowledgementId;
	Fdemo_mapShanmenFormationInfluenceConsumerApplicationReceipt
		ApplicationReceipt;
	Edemo_mapExactModifierMutationStatus NativeStatus =
		Edemo_mapExactModifierMutationStatus::InvalidHandle;
	Fdemo_mapModifierSpec ModifierSpec;

	friend class
		Fdemo_mapShanmenFormationInfluenceConsumerAttributeAdapter;
};

enum class Edemo_mapShanmenFormationInfluenceConsumerAttributeStatus : uint8
{
	Applied,
	ApplyReplayed,
	Removed,
	RemoveReplayed,
	ApplicationRejected,
	EvidenceMismatch,
	ComponentUnavailable,
	ConversionRejected,
	NativeMutationRejected,
	StateInvalid
};

struct Fdemo_mapShanmenFormationInfluenceConsumerAttributeResult
{
	Edemo_mapShanmenFormationInfluenceConsumerAttributeStatus Status =
		Edemo_mapShanmenFormationInfluenceConsumerAttributeStatus::
			ApplicationRejected;
	FString Diagnostic;
	bool bComponentMutated = false;
	Edemo_mapExactModifierMutationStatus NativeStatus =
		Edemo_mapExactModifierMutationStatus::InvalidHandle;
	Fdemo_mapShanmenFormationInfluenceConsumerAttributeAcknowledgement
		Acknowledgement;

	bool IsSuccess() const;
};

/**
 * Stateless desired-state adapter from accepted consumer commands to the
 * existing authoritative attribute component.
 *
 * This is the only fixed-point-to-float conversion boundary. The adapter
 * stores no modifier table and uses the projection's exact deterministic
 * handle for idempotent native Apply/Remove convergence.
 */
class Fdemo_mapShanmenFormationInfluenceConsumerAttributeAdapter
{
public:
	static bool TryBuildModifierSpec(
		const Fdemo_mapShanmenFormationInfluenceConsumerProjection& Projection,
		Fdemo_mapModifierSpec& OutSpec);

	static Fdemo_mapShanmenFormationInfluenceConsumerAttributeResult
	Synchronize(
		Udemo_mapAttributeComponent* AttributeComponent,
		const Fdemo_mapShanmenFormationInfluenceConsumerApplicationResult&
			Application);
};
