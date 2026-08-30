#pragma once

#include "CoreMinimal.h"
#include "demo_mapAttributeTypes.h"
#include "demo_mapShanmenFormationInfluenceLeaseExecutor.h"

/**
 * Content-versioned mapping from one frozen influence channel to one existing
 * product attribute contract. The first supported consumer intentionally maps
 * offense power to the legacy AttackPower additive seam without converting the
 * fixed-point magnitude to a floating-point value or mutating an attribute
 * component.
 */
struct Fdemo_mapShanmenFormationInfluenceConsumerDefinition
{
public:
	static bool TryCreateOffensePowerAdditive(
		FName DefinitionId,
		int64 MagnitudeUnitsPerAttributePoint,
		int32 Priority,
		const FShanmenContentStamp& Content,
		Fdemo_mapShanmenFormationInfluenceConsumerDefinition& OutDefinition);

	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenFormationInfluenceConsumerDefinition& Other)
		const;

	FName GetDefinitionId() const { return DefinitionId; }
	const FGameplayTag& GetSourceChannel() const { return SourceChannel; }
	FName GetTargetAttributeId() const { return TargetAttributeId; }
	Edemo_mapModifierOperation GetOperation() const { return Operation; }
	int64 GetMagnitudeUnitsPerAttributePoint() const
	{
		return MagnitudeUnitsPerAttributePoint;
	}
	int32 GetPriority() const { return Priority; }
	const FShanmenContentStamp& GetContent() const { return Content; }

private:
	FName DefinitionId = NAME_None;
	FGameplayTag SourceChannel;
	FName TargetAttributeId = NAME_None;
	Edemo_mapModifierOperation Operation = Edemo_mapModifierOperation::Add;
	int64 MagnitudeUnitsPerAttributePoint = 0;
	int32 Priority = 0;
	FShanmenContentStamp Content;
};

/**
 * Self-validating pure-value projection of one active lease.
 *
 * Magnitude remains an exact signed rational:
 * FinalMagnitudeUnits / MagnitudeUnitsPerAttributePoint. A later product
 * adapter may consume this contract, but this type owns no Actor, component,
 * GAS, scheduled work, persistence, or floating-point conversion behavior.
 */
struct Fdemo_mapShanmenFormationInfluenceConsumerProjection
{
public:
	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenFormationInfluenceConsumerProjection& Other)
		const;

	const Fdemo_mapModifierHandle& GetHandle() const { return Handle; }
	FName GetSourceId() const { return SourceId; }
	const Fdemo_mapShanmenFormationInfluenceLeaseSnapshot& GetLease() const
	{
		return Lease;
	}
	const Fdemo_mapShanmenFormationInfluenceConsumerDefinition& GetDefinition()
		const
	{
		return Definition;
	}
	int64 GetMagnitudeUnits() const
	{
		return Lease.EvaluationReceipt.FinalMagnitudeUnits;
	}
	int64 GetMagnitudeUnitsPerAttributePoint() const
	{
		return Definition.GetMagnitudeUnitsPerAttributePoint();
	}

private:
	Fdemo_mapModifierHandle Handle;
	FName SourceId = NAME_None;
	Fdemo_mapShanmenFormationInfluenceLeaseSnapshot Lease;
	Fdemo_mapShanmenFormationInfluenceConsumerDefinition Definition;

	friend class Fdemo_mapShanmenFormationInfluenceConsumerProjector;
};

enum class Edemo_mapShanmenFormationInfluenceConsumerProjectionStatus : uint8
{
	Projected,
	NoContribution,
	LeaseInvalid,
	DefinitionInvalid,
	ContentMismatch,
	ChannelUnsupported,
	ProjectionRejected
};

struct Fdemo_mapShanmenFormationInfluenceConsumerProjectionResult
{
	Edemo_mapShanmenFormationInfluenceConsumerProjectionStatus Status =
		Edemo_mapShanmenFormationInfluenceConsumerProjectionStatus::LeaseInvalid;
	FString Diagnostic;
	Fdemo_mapShanmenFormationInfluenceConsumerProjection Projection;

	bool IsSuccess() const;
	bool HasProjection() const;
};

enum class Edemo_mapShanmenFormationInfluenceConsumerCommandOperation : uint8
{
	Apply,
	Remove
};

/** Reversible pure-value command; Apply and Remove retain the same handle. */
struct Fdemo_mapShanmenFormationInfluenceConsumerCommand
{
public:
	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenFormationInfluenceConsumerCommand& Other) const;

	const FGuid& GetCommandId() const { return CommandId; }
	Edemo_mapShanmenFormationInfluenceConsumerCommandOperation GetOperation()
		const
	{
		return Operation;
	}
	const Fdemo_mapShanmenFormationInfluenceConsumerProjection& GetProjection()
		const
	{
		return Projection;
	}
	const Fdemo_mapModifierHandle& GetHandle() const
	{
		return Projection.GetHandle();
	}

private:
	FGuid CommandId;
	Edemo_mapShanmenFormationInfluenceConsumerCommandOperation Operation =
		Edemo_mapShanmenFormationInfluenceConsumerCommandOperation::Apply;
	Fdemo_mapShanmenFormationInfluenceConsumerProjection Projection;

	friend class Fdemo_mapShanmenFormationInfluenceConsumerProjector;
};

/** Stateless lease-to-consumer projection and reversible-command factory. */
class Fdemo_mapShanmenFormationInfluenceConsumerProjector
{
public:
	static Fdemo_mapShanmenFormationInfluenceConsumerProjectionResult
	ProjectActiveLease(
		const Fdemo_mapShanmenFormationInfluenceLeaseSnapshot& Lease,
		const Fdemo_mapShanmenFormationInfluenceConsumerDefinition& Definition);

	static bool TryBuildCommand(
		const Fdemo_mapShanmenFormationInfluenceConsumerProjection& Projection,
		Edemo_mapShanmenFormationInfluenceConsumerCommandOperation Operation,
		Fdemo_mapShanmenFormationInfluenceConsumerCommand& OutCommand);
};
