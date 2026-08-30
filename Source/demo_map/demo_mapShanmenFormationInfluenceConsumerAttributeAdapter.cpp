#include "demo_mapShanmenFormationInfluenceConsumerAttributeAdapter.h"

#include "ShanmenDeterministicId.h"
#include "demo_mapAttributeComponent.h"

namespace
{
	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	bool IsApplyStatus(
		const Edemo_mapExactModifierMutationStatus Status)
	{
		return Status == Edemo_mapExactModifierMutationStatus::Applied
			|| Status
				== Edemo_mapExactModifierMutationStatus::ApplyReplayed;
	}

	bool IsRemoveStatus(
		const Edemo_mapExactModifierMutationStatus Status)
	{
		return Status == Edemo_mapExactModifierMutationStatus::Removed
			|| Status
				== Edemo_mapExactModifierMutationStatus::RemoveReplayed;
	}

	FGuid MakeAcknowledgementId(
		const Fdemo_mapShanmenFormationInfluenceConsumerApplicationReceipt&
			ApplicationReceipt)
	{
		if (!ApplicationReceipt.IsValid())
		{
			return FGuid();
		}
		const auto& Projection =
			ApplicationReceipt.GetCommand().GetProjection();
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Formation.InfluenceConsumerAttributeAck.r1"),
			{
				GuidDigits(ApplicationReceipt.GetReceiptId()),
				GuidDigits(ApplicationReceipt.GetApplicationId()),
				GuidDigits(Projection.GetHandle().Value),
				Projection.GetSourceId().ToString(),
				Projection.GetDefinition().GetTargetAttributeId().ToString(),
				FString::FromInt(static_cast<int32>(
					ApplicationReceipt.GetCommand().GetOperation())),
				FString::Printf(TEXT("%lld"), Projection.GetMagnitudeUnits()),
				FString::Printf(
					TEXT("%lld"),
					Projection.GetMagnitudeUnitsPerAttributePoint()),
				FString::FromInt(Projection.GetDefinition().GetPriority())
			});
	}

	bool IsReceiptOperationConsistent(
		const Fdemo_mapShanmenFormationInfluenceConsumerApplicationResult&
			Application)
	{
		if (!Application.IsSuccess())
		{
			return false;
		}
		const auto Operation =
			Application.Receipt.GetCommand().GetOperation();
		return (Application.Status
				== Edemo_mapShanmenFormationInfluenceConsumerApplicationStatus::
					Applied
			&& Operation
				== Edemo_mapShanmenFormationInfluenceConsumerCommandOperation::
					Apply)
			|| (Application.Status
				== Edemo_mapShanmenFormationInfluenceConsumerApplicationStatus::
					Removed
				&& Operation
					== Edemo_mapShanmenFormationInfluenceConsumerCommandOperation::
						Remove);
	}

	Fdemo_mapShanmenFormationInfluenceConsumerAttributeResult Reject(
		const Edemo_mapShanmenFormationInfluenceConsumerAttributeStatus Status,
		const TCHAR* Diagnostic)
	{
		Fdemo_mapShanmenFormationInfluenceConsumerAttributeResult Result;
		Result.Status = Status;
		Result.Diagnostic = Diagnostic;
		return Result;
	}
}

bool Fdemo_mapShanmenFormationInfluenceConsumerAttributeAcknowledgement::
IsValid() const
{
	if (!ApplicationReceipt.IsValid()
		|| AcknowledgementId != MakeAcknowledgementId(ApplicationReceipt))
	{
		return false;
	}
	Fdemo_mapModifierSpec Expected;
	if (!Fdemo_mapShanmenFormationInfluenceConsumerAttributeAdapter::
		TryBuildModifierSpec(
			ApplicationReceipt.GetCommand().GetProjection(), Expected)
		|| !ModifierSpec.Matches(Expected))
	{
		return false;
	}
	const auto Operation = ApplicationReceipt.GetCommand().GetOperation();
	return (Operation
			== Edemo_mapShanmenFormationInfluenceConsumerCommandOperation::Apply
		&& IsApplyStatus(NativeStatus))
		|| (Operation
			== Edemo_mapShanmenFormationInfluenceConsumerCommandOperation::Remove
			&& IsRemoveStatus(NativeStatus));
}

bool Fdemo_mapShanmenFormationInfluenceConsumerAttributeAcknowledgement::
Matches(
	const Fdemo_mapShanmenFormationInfluenceConsumerAttributeAcknowledgement&
		Other) const
{
	return IsValid() && Other.IsValid()
		&& AcknowledgementId == Other.AcknowledgementId
		&& ApplicationReceipt.Matches(Other.ApplicationReceipt)
		&& ModifierSpec.Matches(Other.ModifierSpec);
}

bool Fdemo_mapShanmenFormationInfluenceConsumerAttributeResult::IsSuccess()
	const
{
	if (!Acknowledgement.IsValid())
	{
		return false;
	}
	if (NativeStatus != Acknowledgement.GetNativeStatus())
	{
		return false;
	}
	switch (Status)
	{
	case Edemo_mapShanmenFormationInfluenceConsumerAttributeStatus::Applied:
		return bComponentMutated
			&& Acknowledgement.GetNativeStatus()
				== Edemo_mapExactModifierMutationStatus::Applied;
	case Edemo_mapShanmenFormationInfluenceConsumerAttributeStatus::
		ApplyReplayed:
		return !bComponentMutated
			&& Acknowledgement.GetNativeStatus()
				== Edemo_mapExactModifierMutationStatus::ApplyReplayed;
	case Edemo_mapShanmenFormationInfluenceConsumerAttributeStatus::Removed:
		return bComponentMutated
			&& Acknowledgement.GetNativeStatus()
				== Edemo_mapExactModifierMutationStatus::Removed;
	case Edemo_mapShanmenFormationInfluenceConsumerAttributeStatus::
		RemoveReplayed:
		return !bComponentMutated
			&& Acknowledgement.GetNativeStatus()
				== Edemo_mapExactModifierMutationStatus::RemoveReplayed;
	default:
		return false;
	}
}

bool Fdemo_mapShanmenFormationInfluenceConsumerAttributeAdapter::
TryBuildModifierSpec(
	const Fdemo_mapShanmenFormationInfluenceConsumerProjection& Projection,
	Fdemo_mapModifierSpec& OutSpec)
{
	OutSpec = Fdemo_mapModifierSpec();
	if (!Projection.IsValid()
		|| Projection.GetMagnitudeUnitsPerAttributePoint() <= 0)
	{
		return false;
	}
	const double Value =
		static_cast<double>(Projection.GetMagnitudeUnits())
		/ static_cast<double>(
			Projection.GetMagnitudeUnitsPerAttributePoint());
	if (!FMath::IsFinite(Value)
		|| Value > static_cast<double>(TNumericLimits<float>::Max())
		|| Value < -static_cast<double>(TNumericLimits<float>::Max()))
	{
		return false;
	}
	const float NativeValue = static_cast<float>(Value);
	if (!FMath::IsFinite(NativeValue)
		|| (Projection.GetMagnitudeUnits() != 0 && NativeValue == 0.0f))
	{
		return false;
	}

	Fdemo_mapModifierSpec Candidate;
	Candidate.SourceId = Projection.GetSourceId();
	Candidate.AttributeId =
		Projection.GetDefinition().GetTargetAttributeId();
	Candidate.Operation = Projection.GetDefinition().GetOperation();
	Candidate.Value = NativeValue;
	Candidate.Priority = Projection.GetDefinition().GetPriority();
	OutSpec = Candidate;
	return true;
}

Fdemo_mapShanmenFormationInfluenceConsumerAttributeResult
Fdemo_mapShanmenFormationInfluenceConsumerAttributeAdapter::Synchronize(
	Udemo_mapAttributeComponent* AttributeComponent,
	const Fdemo_mapShanmenFormationInfluenceConsumerApplicationResult&
		Application)
{
	if (!Application.IsSuccess())
	{
		return Reject(
			Edemo_mapShanmenFormationInfluenceConsumerAttributeStatus::
				ApplicationRejected,
			TEXT("Attribute synchronization requires one accepted registry command."));
	}
	if (!IsReceiptOperationConsistent(Application))
	{
		return Reject(
			Edemo_mapShanmenFormationInfluenceConsumerAttributeStatus::
				EvidenceMismatch,
			TEXT("Registry status did not match its immutable command operation."));
	}
	if (!AttributeComponent)
	{
		return Reject(
			Edemo_mapShanmenFormationInfluenceConsumerAttributeStatus::
				ComponentUnavailable,
			TEXT("No authoritative attribute component was supplied."));
	}

	Fdemo_mapModifierSpec Spec;
	if (!TryBuildModifierSpec(
		Application.Receipt.GetCommand().GetProjection(), Spec))
	{
		return Reject(
			Edemo_mapShanmenFormationInfluenceConsumerAttributeStatus::
				ConversionRejected,
			TEXT("Consumer fixed-point evidence could not cross the float boundary."));
	}

	const auto& Command = Application.Receipt.GetCommand();
	const auto NativeStatus = Command.GetOperation()
		== Edemo_mapShanmenFormationInfluenceConsumerCommandOperation::Apply
		? AttributeComponent->EnsureModifierApplied(Spec, Command.GetHandle())
		: AttributeComponent->EnsureModifierRemoved(Spec, Command.GetHandle());

	Fdemo_mapShanmenFormationInfluenceConsumerAttributeResult Result;
	Result.NativeStatus = NativeStatus;
	Result.Acknowledgement.ApplicationReceipt = Application.Receipt;
	Result.Acknowledgement.NativeStatus = NativeStatus;
	Result.Acknowledgement.ModifierSpec = Spec;
	Result.Acknowledgement.AcknowledgementId =
		MakeAcknowledgementId(Application.Receipt);
	switch (NativeStatus)
	{
	case Edemo_mapExactModifierMutationStatus::Applied:
		Result.Status =
			Edemo_mapShanmenFormationInfluenceConsumerAttributeStatus::Applied;
		Result.bComponentMutated = true;
		Result.Diagnostic =
			TEXT("Consumer modifier was applied with its exact handle.");
		break;
	case Edemo_mapExactModifierMutationStatus::ApplyReplayed:
		Result.Status = Edemo_mapShanmenFormationInfluenceConsumerAttributeStatus::
			ApplyReplayed;
		Result.Diagnostic =
			TEXT("Consumer Apply converged to an already matching modifier.");
		break;
	case Edemo_mapExactModifierMutationStatus::Removed:
		Result.Status =
			Edemo_mapShanmenFormationInfluenceConsumerAttributeStatus::Removed;
		Result.bComponentMutated = true;
		Result.Diagnostic =
			TEXT("Consumer modifier was removed by its exact handle.");
		break;
	case Edemo_mapExactModifierMutationStatus::RemoveReplayed:
		Result.Status = Edemo_mapShanmenFormationInfluenceConsumerAttributeStatus::
			RemoveReplayed;
		Result.Diagnostic =
			TEXT("Consumer Remove converged to an already absent modifier.");
		break;
	default:
		Result.Status =
			Edemo_mapShanmenFormationInfluenceConsumerAttributeStatus::
				NativeMutationRejected;
		Result.Diagnostic =
			TEXT("The authoritative attribute component rejected native convergence.");
		Result.Acknowledgement =
			Fdemo_mapShanmenFormationInfluenceConsumerAttributeAcknowledgement();
		return Result;
	}
	if (!Result.IsSuccess())
	{
		return Reject(
			Edemo_mapShanmenFormationInfluenceConsumerAttributeStatus::StateInvalid,
			TEXT("Native convergence produced invalid acknowledgement evidence."));
	}
	return Result;
}
