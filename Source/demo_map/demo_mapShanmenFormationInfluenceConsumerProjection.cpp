#include "demo_mapShanmenFormationInfluenceConsumerProjection.h"

#include "ShanmenCombatTags.h"
#include "ShanmenDeterministicId.h"
#include "demo_mapAttributeDefinitions.h"

namespace
{
	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	bool SameContent(
		const FShanmenContentStamp& Left,
		const FShanmenContentStamp& Right)
	{
		return Left.IsValid() && Right.IsValid()
			&& Left.Version == Right.Version && Left.Digest == Right.Digest;
	}

	bool IsKnownCommandOperation(
		const Edemo_mapShanmenFormationInfluenceConsumerCommandOperation
			Operation)
	{
		return Operation
				== Edemo_mapShanmenFormationInfluenceConsumerCommandOperation::Apply
			|| Operation
				== Edemo_mapShanmenFormationInfluenceConsumerCommandOperation::Remove;
	}

	FGuid MakeProjectionHandleValue(
		const Fdemo_mapShanmenFormationInfluenceLeaseSnapshot& Lease,
		const Fdemo_mapShanmenFormationInfluenceConsumerDefinition& Definition)
	{
		if (!Lease.IsValid() || !Definition.IsValid()
			|| Lease.EvaluationReceipt.FinalMagnitudeUnits == 0
			|| !SameContent(Lease.Key.Content, Definition.GetContent())
			|| Lease.EvaluationReceipt.Context.Channel
				!= Definition.GetSourceChannel())
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Formation.InfluenceConsumerProjectionHandle.r1"),
			{
				GuidDigits(Lease.LeaseId), GuidDigits(Lease.ApplyIntentId),
				GuidDigits(Lease.EvaluationReceipt.ReceiptId),
				GuidDigits(Lease.Key.RunId), GuidDigits(Lease.Key.OwnerId),
				GuidDigits(Lease.Key.SourceEntityId),
				GuidDigits(Lease.Key.DeploymentId), GuidDigits(Lease.Key.AreaId),
				GuidDigits(Lease.Key.SubjectEntityId),
				Lease.Key.PolicyDefinitionId.ToString(),
				Lease.Key.InfluenceDefinitionId.ToString(),
				Definition.GetDefinitionId().ToString(),
				Definition.GetSourceChannel().ToString(),
				Definition.GetTargetAttributeId().ToString(),
				FString::FromInt(static_cast<int32>(Definition.GetOperation())),
				LexToString(Lease.EvaluationReceipt.FinalMagnitudeUnits),
				LexToString(Definition.GetMagnitudeUnitsPerAttributePoint()),
				FString::FromInt(Definition.GetPriority()),
				Lease.Key.Content.Version.ToString(), Lease.Key.Content.Digest
			});
	}

	FName MakeSourceId(const Fdemo_mapModifierHandle& Handle)
	{
		return Handle.IsValid()
			? FName(*FString::Printf(
				TEXT("Formation.Influence.%s"),
				*Handle.Value.ToString(EGuidFormats::Digits)))
			: NAME_None;
	}

	FGuid MakeCommandId(
		const Fdemo_mapShanmenFormationInfluenceConsumerProjection& Projection,
		const Edemo_mapShanmenFormationInfluenceConsumerCommandOperation
			Operation)
	{
		if (!Projection.IsValid() || !IsKnownCommandOperation(Operation))
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Formation.InfluenceConsumerCommand.r1"),
			{
				GuidDigits(Projection.GetHandle().Value),
				GuidDigits(Projection.GetLease().LeaseId),
				GuidDigits(Projection.GetLease().EvaluationReceipt.ReceiptId),
				FString::FromInt(static_cast<int32>(Operation))
			});
	}

	Fdemo_mapShanmenFormationInfluenceConsumerProjectionResult Reject(
		const Edemo_mapShanmenFormationInfluenceConsumerProjectionStatus Status,
		const TCHAR* Diagnostic)
	{
		Fdemo_mapShanmenFormationInfluenceConsumerProjectionResult Result;
		Result.Status = Status;
		Result.Diagnostic = Diagnostic;
		return Result;
	}
}

bool Fdemo_mapShanmenFormationInfluenceConsumerDefinition::
TryCreateOffensePowerAdditive(
	const FName DefinitionId,
	const int64 MagnitudeUnitsPerAttributePoint,
	const int32 Priority,
	const FShanmenContentStamp& Content,
	Fdemo_mapShanmenFormationInfluenceConsumerDefinition& OutDefinition)
{
	OutDefinition = Fdemo_mapShanmenFormationInfluenceConsumerDefinition();
	Fdemo_mapShanmenFormationInfluenceConsumerDefinition Candidate;
	Candidate.DefinitionId = DefinitionId;
	Candidate.SourceChannel =
		FShanmenCombatNativeTags::InfluenceOffensePower();
	Candidate.TargetAttributeId = Fdemo_mapAttributeIds::AttackPower;
	Candidate.Operation = Edemo_mapModifierOperation::Add;
	Candidate.MagnitudeUnitsPerAttributePoint =
		MagnitudeUnitsPerAttributePoint;
	Candidate.Priority = Priority;
	Candidate.Content = Content;
	if (!Candidate.IsValid())
	{
		return false;
	}
	OutDefinition = Candidate;
	return true;
}

bool Fdemo_mapShanmenFormationInfluenceConsumerDefinition::IsValid() const
{
	return !DefinitionId.IsNone()
		&& SourceChannel.IsValid()
		&& SourceChannel
			== FShanmenCombatNativeTags::InfluenceOffensePower()
		&& TargetAttributeId == Fdemo_mapAttributeIds::AttackPower
		&& Operation == Edemo_mapModifierOperation::Add
		&& MagnitudeUnitsPerAttributePoint > 0 && Content.IsValid();
}

bool Fdemo_mapShanmenFormationInfluenceConsumerDefinition::Matches(
	const Fdemo_mapShanmenFormationInfluenceConsumerDefinition& Other) const
{
	return IsValid() && Other.IsValid()
		&& DefinitionId == Other.DefinitionId
		&& SourceChannel == Other.SourceChannel
		&& TargetAttributeId == Other.TargetAttributeId
		&& Operation == Other.Operation
		&& MagnitudeUnitsPerAttributePoint
			== Other.MagnitudeUnitsPerAttributePoint
		&& Priority == Other.Priority && SameContent(Content, Other.Content);
}

bool Fdemo_mapShanmenFormationInfluenceConsumerProjection::IsValid() const
{
	if (!Lease.IsValid() || !Definition.IsValid()
		|| Lease.EvaluationReceipt.FinalMagnitudeUnits == 0
		|| !SameContent(Lease.Key.Content, Definition.GetContent())
		|| Lease.EvaluationReceipt.Context.Channel
			!= Definition.GetSourceChannel())
	{
		return false;
	}
	Fdemo_mapModifierHandle ExpectedHandle;
	ExpectedHandle.Value = MakeProjectionHandleValue(Lease, Definition);
	return Handle.IsValid() && Handle == ExpectedHandle
		&& SourceId == MakeSourceId(ExpectedHandle);
}

bool Fdemo_mapShanmenFormationInfluenceConsumerProjection::Matches(
	const Fdemo_mapShanmenFormationInfluenceConsumerProjection& Other) const
{
	return IsValid() && Other.IsValid() && Handle == Other.Handle;
}

bool Fdemo_mapShanmenFormationInfluenceConsumerProjectionResult::
IsSuccess() const
{
	return Status
			== Edemo_mapShanmenFormationInfluenceConsumerProjectionStatus::Projected
		|| Status
			== Edemo_mapShanmenFormationInfluenceConsumerProjectionStatus::
				NoContribution;
}

bool Fdemo_mapShanmenFormationInfluenceConsumerProjectionResult::
HasProjection() const
{
	return Status
			== Edemo_mapShanmenFormationInfluenceConsumerProjectionStatus::Projected
		&& Projection.IsValid();
}

bool Fdemo_mapShanmenFormationInfluenceConsumerCommand::IsValid() const
{
	return CommandId.IsValid() && IsKnownCommandOperation(Operation)
		&& Projection.IsValid()
		&& CommandId == MakeCommandId(Projection, Operation);
}

bool Fdemo_mapShanmenFormationInfluenceConsumerCommand::Matches(
	const Fdemo_mapShanmenFormationInfluenceConsumerCommand& Other) const
{
	return IsValid() && Other.IsValid() && CommandId == Other.CommandId
		&& Operation == Other.Operation
		&& Projection.Matches(Other.Projection);
}

Fdemo_mapShanmenFormationInfluenceConsumerProjectionResult
Fdemo_mapShanmenFormationInfluenceConsumerProjector::ProjectActiveLease(
	const Fdemo_mapShanmenFormationInfluenceLeaseSnapshot& Lease,
	const Fdemo_mapShanmenFormationInfluenceConsumerDefinition& Definition)
{
	if (!Lease.IsValid())
	{
		return Reject(
			Edemo_mapShanmenFormationInfluenceConsumerProjectionStatus::
				LeaseInvalid,
			TEXT("Consumer projection requires one valid active lease snapshot."));
	}
	if (!Definition.IsValid())
	{
		return Reject(
			Edemo_mapShanmenFormationInfluenceConsumerProjectionStatus::
				DefinitionInvalid,
			TEXT("Consumer projection requires one valid authored definition."));
	}
	if (!SameContent(Lease.Key.Content, Definition.GetContent()))
	{
		return Reject(
			Edemo_mapShanmenFormationInfluenceConsumerProjectionStatus::
				ContentMismatch,
			TEXT("Lease and consumer definition content identities differ."));
	}
	if (Lease.EvaluationReceipt.Context.Channel
		!= Definition.GetSourceChannel())
	{
		return Reject(
			Edemo_mapShanmenFormationInfluenceConsumerProjectionStatus::
				ChannelUnsupported,
			TEXT("The active lease channel has no matching consumer mapping."));
	}
	if (Lease.EvaluationReceipt.FinalMagnitudeUnits == 0)
	{
		auto Result = Reject(
			Edemo_mapShanmenFormationInfluenceConsumerProjectionStatus::
				NoContribution,
			TEXT("The active lease evaluates to a canonical zero contribution."));
		return Result;
	}

	Fdemo_mapShanmenFormationInfluenceConsumerProjection Candidate;
	Candidate.Lease = Lease;
	Candidate.Definition = Definition;
	Candidate.Handle.Value = MakeProjectionHandleValue(Lease, Definition);
	Candidate.SourceId = MakeSourceId(Candidate.Handle);
	if (!Candidate.IsValid())
	{
		return Reject(
			Edemo_mapShanmenFormationInfluenceConsumerProjectionStatus::
				ProjectionRejected,
			TEXT("Projected consumer evidence failed self-validation."));
	}

	Fdemo_mapShanmenFormationInfluenceConsumerProjectionResult Result;
	Result.Status =
		Edemo_mapShanmenFormationInfluenceConsumerProjectionStatus::Projected;
	Result.Diagnostic =
		TEXT("The active lease projected to one reversible consumer handle.");
	Result.Projection = Candidate;
	return Result;
}

bool Fdemo_mapShanmenFormationInfluenceConsumerProjector::TryBuildCommand(
	const Fdemo_mapShanmenFormationInfluenceConsumerProjection& Projection,
	const Edemo_mapShanmenFormationInfluenceConsumerCommandOperation Operation,
	Fdemo_mapShanmenFormationInfluenceConsumerCommand& OutCommand)
{
	OutCommand = Fdemo_mapShanmenFormationInfluenceConsumerCommand();
	if (!Projection.IsValid() || !IsKnownCommandOperation(Operation))
	{
		return false;
	}
	Fdemo_mapShanmenFormationInfluenceConsumerCommand Candidate;
	Candidate.Operation = Operation;
	Candidate.Projection = Projection;
	Candidate.CommandId = MakeCommandId(Projection, Operation);
	if (!Candidate.IsValid())
	{
		return false;
	}
	OutCommand = Candidate;
	return true;
}
