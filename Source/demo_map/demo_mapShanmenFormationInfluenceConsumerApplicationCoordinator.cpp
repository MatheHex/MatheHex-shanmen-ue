#include "demo_mapShanmenFormationInfluenceConsumerApplicationCoordinator.h"

#include "ShanmenDeterministicId.h"
#include "demo_mapAttributeComponent.h"

namespace
{
	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	FGuid MakeCoordinatorId(
		const FGuid& RegistryId,
		const FGuid& SubjectEntityId)
	{
		if (!RegistryId.IsValid() || !SubjectEntityId.IsValid())
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Formation.InfluenceConsumerCoordinator.r1"),
			{ GuidDigits(RegistryId), GuidDigits(SubjectEntityId) });
	}

	FGuid MakeTransactionReceiptId(
		const FGuid& CoordinatorId,
		const FGuid& SubjectEntityId,
		const Fdemo_mapShanmenFormationInfluenceConsumerApplicationReceipt&
			RegistryReceipt,
		const Fdemo_mapShanmenFormationInfluenceConsumerAttributeAcknowledgement&
			Acknowledgement)
	{
		if (!CoordinatorId.IsValid() || !SubjectEntityId.IsValid()
			|| !RegistryReceipt.IsValid() || !Acknowledgement.IsValid()
			|| !RegistryReceipt.Matches(
				Acknowledgement.GetApplicationReceipt()))
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Formation.InfluenceConsumerTransactionReceipt.r1"),
			{
				GuidDigits(CoordinatorId), GuidDigits(SubjectEntityId),
				GuidDigits(RegistryReceipt.GetReceiptId()),
				GuidDigits(Acknowledgement.GetAcknowledgementId())
			});
	}

	bool ResultMatchesCommandOperation(
		const Edemo_mapShanmenFormationInfluenceConsumerTransactionStatus Status,
		const Fdemo_mapShanmenFormationInfluenceConsumerCommand& Command)
	{
		return (Status
				== Edemo_mapShanmenFormationInfluenceConsumerTransactionStatus::
					Applied
			&& Command.GetOperation()
				== Edemo_mapShanmenFormationInfluenceConsumerCommandOperation::Apply)
			|| (Status
				== Edemo_mapShanmenFormationInfluenceConsumerTransactionStatus::
					Removed
				&& Command.GetOperation()
					== Edemo_mapShanmenFormationInfluenceConsumerCommandOperation::
						Remove);
	}

	Fdemo_mapShanmenFormationInfluenceConsumerTransactionResult Reject(
		const Edemo_mapShanmenFormationInfluenceConsumerTransactionStatus Status,
		const TCHAR* Diagnostic)
	{
		Fdemo_mapShanmenFormationInfluenceConsumerTransactionResult Result;
		Result.Status = Status;
		Result.Diagnostic = Diagnostic;
		return Result;
	}

	bool UndoNativeMutation(
		Udemo_mapAttributeComponent* AttributeComponent,
		const Fdemo_mapShanmenFormationInfluenceConsumerCommand& Command,
		const Fdemo_mapShanmenFormationInfluenceConsumerAttributeResult&
			NativeResult)
	{
		if (!AttributeComponent || !Command.IsValid()
			|| !NativeResult.IsSuccess())
		{
			return false;
		}
		if (!NativeResult.bComponentMutated)
		{
			return true;
		}
		const auto& Spec = NativeResult.Acknowledgement.GetModifierSpec();
		const auto Status = Command.GetOperation()
			== Edemo_mapShanmenFormationInfluenceConsumerCommandOperation::Apply
			? AttributeComponent->EnsureModifierRemoved(
				Spec, Command.GetHandle())
			: AttributeComponent->EnsureModifierApplied(
				Spec, Command.GetHandle());
		return Command.GetOperation()
			== Edemo_mapShanmenFormationInfluenceConsumerCommandOperation::Apply
			? Status == Edemo_mapExactModifierMutationStatus::Removed
				|| Status
					== Edemo_mapExactModifierMutationStatus::RemoveReplayed
			: Status == Edemo_mapExactModifierMutationStatus::Applied
				|| Status
					== Edemo_mapExactModifierMutationStatus::ApplyReplayed;
	}
}

bool Fdemo_mapShanmenFormationInfluenceConsumerTransactionReceipt::IsValid()
	const
{
	return CoordinatorId.IsValid() && SubjectEntityId.IsValid()
		&& RegistryReceipt.IsValid() && AttributeAcknowledgement.IsValid()
		&& RegistryReceipt.Matches(
			AttributeAcknowledgement.GetApplicationReceipt())
		&& CoordinatorId == MakeCoordinatorId(
			RegistryReceipt.GetRegistryId(), SubjectEntityId)
		&& ReceiptId == MakeTransactionReceiptId(
			CoordinatorId, SubjectEntityId, RegistryReceipt,
			AttributeAcknowledgement);
}

bool Fdemo_mapShanmenFormationInfluenceConsumerTransactionReceipt::Matches(
	const Fdemo_mapShanmenFormationInfluenceConsumerTransactionReceipt& Other)
	const
{
	return IsValid() && Other.IsValid() && ReceiptId == Other.ReceiptId
		&& CoordinatorId == Other.CoordinatorId
		&& SubjectEntityId == Other.SubjectEntityId
		&& RegistryReceipt.Matches(Other.RegistryReceipt)
		&& AttributeAcknowledgement.Matches(Other.AttributeAcknowledgement);
}

bool Fdemo_mapShanmenFormationInfluenceConsumerTransactionResult::IsSuccess()
	const
{
	if (!Registry.IsSuccess() || !Native.IsSuccess() || !Receipt.IsValid()
		|| !Registry.Receipt.Matches(Receipt.GetRegistryReceipt())
		|| !Native.Acknowledgement.Matches(
			Receipt.GetAttributeAcknowledgement()))
	{
		return false;
	}
	if (Status
		== Edemo_mapShanmenFormationInfluenceConsumerTransactionStatus::
			TransactionReplayed)
	{
		return !bCoordinatorStateCommitted && bTransactionReplayed;
	}
	return bCoordinatorStateCommitted && !bTransactionReplayed
		&& ResultMatchesCommandOperation(
			Status, Registry.Receipt.GetCommand());
}

bool Fdemo_mapShanmenFormationInfluenceConsumerApplicationCoordinator::
TryCreate(
	const FGuid& RunId,
	const FShanmenContentStamp& Content,
	const FGuid& SubjectEntityId,
	Udemo_mapAttributeComponent* AttributeComponent,
	Fdemo_mapShanmenFormationInfluenceConsumerApplicationCoordinator&
		OutCoordinator)
{
	OutCoordinator =
		Fdemo_mapShanmenFormationInfluenceConsumerApplicationCoordinator();
	if (!SubjectEntityId.IsValid() || !AttributeComponent)
	{
		return false;
	}
	Fdemo_mapShanmenFormationInfluenceConsumerApplicationCoordinator Candidate;
	if (!Fdemo_mapShanmenFormationInfluenceConsumerApplicationRegistry::
		TryCreate(RunId, Content, Candidate.Registry))
	{
		return false;
	}
	Candidate.SubjectEntityId = SubjectEntityId;
	Candidate.BoundAttributeComponent = AttributeComponent;
	Candidate.CoordinatorId = MakeCoordinatorId(
		Candidate.Registry.GetRegistryId(), SubjectEntityId);
	if (!Candidate.IsConsistent())
	{
		return false;
	}
	OutCoordinator = MoveTemp(Candidate);
	return true;
}

bool Fdemo_mapShanmenFormationInfluenceConsumerApplicationCoordinator::
MatchesBinding(
	const FGuid& RequestedSubjectEntityId,
	const Udemo_mapAttributeComponent* RequestedComponent) const
{
	return IsConsistent() && RequestedSubjectEntityId == SubjectEntityId
		&& RequestedComponent
		&& BoundAttributeComponent.Get() == RequestedComponent;
}

const Fdemo_mapShanmenFormationInfluenceConsumerApplicationCoordinator::
	FCompletedTransactionRecord*
Fdemo_mapShanmenFormationInfluenceConsumerApplicationCoordinator::
FindCompletedTransaction(const FGuid& CommandId) const
{
	return CompletedTransactions.FindByPredicate(
		[&CommandId](const FCompletedTransactionRecord& Record)
		{
			return CommandId.IsValid()
				&& Record.Command.GetCommandId() == CommandId;
		});
}

bool Fdemo_mapShanmenFormationInfluenceConsumerApplicationCoordinator::
IsConsistent() const
{
	if (!Registry.IsConsistent() || !CoordinatorId.IsValid()
		|| !SubjectEntityId.IsValid()
		|| BoundAttributeComponent.IsExplicitlyNull()
		|| CoordinatorId != MakeCoordinatorId(
			Registry.GetRegistryId(), SubjectEntityId)
		|| CompletedTransactions.Num() != Registry.GetCompletedCommandCount())
	{
		return false;
	}

	TSet<FGuid> CommandIds;
	TSet<FGuid> ReceiptIds;
	for (const FCompletedTransactionRecord& Record : CompletedTransactions)
	{
		const auto& Command = Record.Command;
		const auto& Result = Record.Result;
		Fdemo_mapShanmenFormationInfluenceConsumerApplicationResult StoredRegistry;
		if (!Command.IsValid()
			|| Command.GetProjection().GetLease().Key.SubjectEntityId
				!= SubjectEntityId
			|| !Result.IsSuccess() || Result.bTransactionReplayed
			|| !Result.bCoordinatorStateCommitted
			|| !ResultMatchesCommandOperation(Result.Status, Command)
			|| !Result.Registry.Receipt.MatchesCommand(Command)
			|| !Registry.TryGetCompletedResult(
				Command.GetCommandId(), StoredRegistry)
			|| StoredRegistry.Status != Result.Registry.Status
			|| !StoredRegistry.Receipt.Matches(Result.Registry.Receipt)
			|| Result.Receipt.GetCoordinatorId() != CoordinatorId
			|| Result.Receipt.GetSubjectEntityId() != SubjectEntityId
			|| CommandIds.Contains(Command.GetCommandId())
			|| ReceiptIds.Contains(Result.Receipt.GetReceiptId()))
		{
			return false;
		}
		CommandIds.Add(Command.GetCommandId());
		ReceiptIds.Add(Result.Receipt.GetReceiptId());
	}
	return true;
}

Fdemo_mapShanmenFormationInfluenceConsumerTransactionResult
Fdemo_mapShanmenFormationInfluenceConsumerApplicationCoordinator::Execute(
	const Fdemo_mapShanmenFormationInfluenceConsumerCommand& Command)
{
	if (!IsConsistent())
	{
		return Reject(
			Edemo_mapShanmenFormationInfluenceConsumerTransactionStatus::
				CoordinatorInvalid,
			TEXT("Consumer application Coordinator is internally inconsistent."));
	}
	if (!Command.IsValid())
	{
		return Reject(
			Edemo_mapShanmenFormationInfluenceConsumerTransactionStatus::
				CommandInvalid,
			TEXT("Consumer transaction requires one valid command."));
	}
	if (Command.GetProjection().GetLease().Key.SubjectEntityId
		!= SubjectEntityId)
	{
		return Reject(
			Edemo_mapShanmenFormationInfluenceConsumerTransactionStatus::
				TargetMismatch,
			TEXT("Consumer command targets a different bound subject entity."));
	}
	if (const auto* Existing =
		FindCompletedTransaction(Command.GetCommandId()))
	{
		if (!Existing->Command.Matches(Command))
		{
			return Reject(
				Edemo_mapShanmenFormationInfluenceConsumerTransactionStatus::
					CommitRejected,
				TEXT("CommandId collides with different completed transaction evidence."));
		}
		auto Replay = Existing->Result;
		Replay.Status =
			Edemo_mapShanmenFormationInfluenceConsumerTransactionStatus::
				TransactionReplayed;
		Replay.Diagnostic =
			TEXT("Completed consumer transaction replayed without authority mutation.");
		Replay.bCoordinatorStateCommitted = false;
		Replay.bTransactionReplayed = true;
		Replay.Registry.bCommandReplayed = true;
		return Replay;
	}

	Udemo_mapAttributeComponent* AttributeComponent =
		BoundAttributeComponent.Get();
	if (!AttributeComponent)
	{
		return Reject(
			Edemo_mapShanmenFormationInfluenceConsumerTransactionStatus::
				ComponentUnavailable,
			TEXT("The Coordinator's bound attribute component is unavailable."));
	}

	auto Candidate = *this;
	Fdemo_mapShanmenFormationInfluenceConsumerTransactionResult Result;
	Result.Registry = Candidate.Registry.Execute(Command);
	if (!Result.Registry.IsSuccess())
	{
		Result.Status =
			Edemo_mapShanmenFormationInfluenceConsumerTransactionStatus::
				RegistryRejected;
		Result.Diagnostic = Result.Registry.Diagnostic;
		return Result;
	}
	if (Result.Registry.bCommandReplayed)
	{
		return Reject(
			Edemo_mapShanmenFormationInfluenceConsumerTransactionStatus::
				CommitRejected,
			TEXT("Registry history has no matching Coordinator transaction."));
	}

	Result.Native =
		Fdemo_mapShanmenFormationInfluenceConsumerAttributeAdapter::Synchronize(
			AttributeComponent, Result.Registry);
	if (!Result.Native.IsSuccess())
	{
		Result.Status =
			Edemo_mapShanmenFormationInfluenceConsumerTransactionStatus::
				NativeRejected;
		Result.Diagnostic = Result.Native.Diagnostic;
		return Result;
	}

	Result.Status = Command.GetOperation()
		== Edemo_mapShanmenFormationInfluenceConsumerCommandOperation::Apply
		? Edemo_mapShanmenFormationInfluenceConsumerTransactionStatus::Applied
		: Edemo_mapShanmenFormationInfluenceConsumerTransactionStatus::Removed;
	Result.Diagnostic =
		TEXT("Registry and native attribute authority committed atomically.");
	Result.bCoordinatorStateCommitted = true;
	Result.Receipt.CoordinatorId = CoordinatorId;
	Result.Receipt.SubjectEntityId = SubjectEntityId;
	Result.Receipt.RegistryReceipt = Result.Registry.Receipt;
	Result.Receipt.AttributeAcknowledgement = Result.Native.Acknowledgement;
	Result.Receipt.ReceiptId = MakeTransactionReceiptId(
		CoordinatorId, SubjectEntityId, Result.Registry.Receipt,
		Result.Native.Acknowledgement);

	FCompletedTransactionRecord& Record =
		Candidate.CompletedTransactions.AddDefaulted_GetRef();
	Record.Command = Command;
	Record.Result = Result;
	if (!Candidate.IsConsistent())
	{
		const bool bUndone = UndoNativeMutation(
			AttributeComponent, Command, Result.Native);
		auto Rejected = Reject(
			Edemo_mapShanmenFormationInfluenceConsumerTransactionStatus::
				CommitRejected,
			bUndone
				? TEXT("Candidate validation failed; native mutation was compensated.")
				: TEXT("Candidate validation and native compensation both failed."));
		Rejected.Native = Result.Native;
		return Rejected;
	}

	*this = MoveTemp(Candidate);
	return Result;
}

bool Fdemo_mapShanmenFormationInfluenceConsumerApplicationCoordinator::
TryGetCompletedResult(
	const FGuid& CommandId,
	Fdemo_mapShanmenFormationInfluenceConsumerTransactionResult& OutResult) const
{
	OutResult =
		Fdemo_mapShanmenFormationInfluenceConsumerTransactionResult();
	if (!IsConsistent() || !CommandId.IsValid())
	{
		return false;
	}
	const auto* Record = FindCompletedTransaction(CommandId);
	if (!Record)
	{
		return false;
	}
	OutResult = Record->Result;
	return OutResult.IsSuccess() && !OutResult.bTransactionReplayed;
}
