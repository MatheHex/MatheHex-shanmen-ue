#include "demo_mapShanmenFormationInfluenceConsumerCommandHost.h"

#include "ShanmenDeterministicId.h"
#include "demo_mapAttributeComponent.h"

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

	FGuid MakeHostId(
		const FGuid& RunId,
		const FShanmenContentStamp& Content)
	{
		if (!RunId.IsValid() || !Content.IsValid())
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Formation.InfluenceConsumerCommandHost.r1"),
			{
				GuidDigits(RunId), Content.Version.ToString(), Content.Digest
			});
	}

	FGuid MakeBindingId(
		const FGuid& HostId,
		const FGuid& SubjectEntityId,
		const FGuid& CoordinatorId)
	{
		if (!HostId.IsValid() || !SubjectEntityId.IsValid()
			|| !CoordinatorId.IsValid())
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Formation.InfluenceConsumerBinding.r1"),
			{
				GuidDigits(HostId), GuidDigits(SubjectEntityId),
				GuidDigits(CoordinatorId)
			});
	}

	Fdemo_mapShanmenFormationInfluenceConsumerBindingResult RejectBinding(
		const Edemo_mapShanmenFormationInfluenceConsumerBindingStatus Status,
		const TCHAR* Diagnostic)
	{
		Fdemo_mapShanmenFormationInfluenceConsumerBindingResult Result;
		Result.Status = Status;
		Result.Diagnostic = Diagnostic;
		return Result;
	}

	Fdemo_mapShanmenFormationInfluenceConsumerRouteResult RejectRoute(
		const Edemo_mapShanmenFormationInfluenceConsumerRouteStatus Status,
		const TCHAR* Diagnostic)
	{
		Fdemo_mapShanmenFormationInfluenceConsumerRouteResult Result;
		Result.Status = Status;
		Result.Diagnostic = Diagnostic;
		return Result;
	}
}

bool Fdemo_mapShanmenFormationInfluenceConsumerBindingReceipt::IsValid() const
{
	return HostId.IsValid() && SubjectEntityId.IsValid()
		&& CoordinatorId.IsValid()
		&& BindingId == MakeBindingId(
			HostId, SubjectEntityId, CoordinatorId);
}

bool Fdemo_mapShanmenFormationInfluenceConsumerBindingReceipt::Matches(
	const Fdemo_mapShanmenFormationInfluenceConsumerBindingReceipt& Other) const
{
	return IsValid() && Other.IsValid() && BindingId == Other.BindingId
		&& HostId == Other.HostId && SubjectEntityId == Other.SubjectEntityId
		&& CoordinatorId == Other.CoordinatorId;
}

bool Fdemo_mapShanmenFormationInfluenceConsumerBindingResult::IsSuccess() const
{
	if (!Receipt.IsValid())
	{
		return false;
	}
	return (Status
			== Edemo_mapShanmenFormationInfluenceConsumerBindingStatus::Bound
			&& bHostStateCommitted)
		|| (Status
			== Edemo_mapShanmenFormationInfluenceConsumerBindingStatus::
				BindingReplayed
			&& !bHostStateCommitted);
}

bool Fdemo_mapShanmenFormationInfluenceConsumerRouteResult::IsSuccess() const
{
	if (!Binding.IsValid() || !Transaction.IsSuccess()
		|| Transaction.Receipt.GetSubjectEntityId()
			!= Binding.GetSubjectEntityId()
		|| Transaction.Receipt.GetCoordinatorId()
			!= Binding.GetCoordinatorId())
	{
		return false;
	}
	return (Status
			== Edemo_mapShanmenFormationInfluenceConsumerRouteStatus::Routed
			&& bHostStateChanged
			&& Transaction.bCoordinatorStateCommitted
			&& !Transaction.bTransactionReplayed)
		|| (Status
			== Edemo_mapShanmenFormationInfluenceConsumerRouteStatus::
				TransactionReplayed
			&& !bHostStateChanged
			&& !Transaction.bCoordinatorStateCommitted
			&& Transaction.bTransactionReplayed);
}

bool Fdemo_mapShanmenFormationInfluenceConsumerCommandHost::TryOpen(
	const FGuid& RunId,
	const FShanmenContentStamp& Content,
	Fdemo_mapShanmenFormationInfluenceConsumerCommandHost& OutHost)
{
	OutHost = Fdemo_mapShanmenFormationInfluenceConsumerCommandHost();
	if (!RunId.IsValid() || !Content.IsValid())
	{
		return false;
	}
	Fdemo_mapShanmenFormationInfluenceConsumerCommandHost Candidate;
	Candidate.RunId = RunId;
	Candidate.Content = Content;
	Candidate.HostId = MakeHostId(RunId, Content);
	if (!Candidate.IsConsistent())
	{
		return false;
	}
	OutHost = MoveTemp(Candidate);
	return true;
}

Fdemo_mapShanmenFormationInfluenceConsumerBindingResult
Fdemo_mapShanmenFormationInfluenceConsumerCommandHost::TryBindSubject(
	const FGuid& SubjectEntityId,
	Udemo_mapAttributeComponent* AttributeComponent)
{
	if (!IsConsistent())
	{
		return RejectBinding(
			Edemo_mapShanmenFormationInfluenceConsumerBindingStatus::HostInvalid,
			TEXT("Consumer CommandHost is not open or is inconsistent."));
	}
	if (!SubjectEntityId.IsValid())
	{
		return RejectBinding(
			Edemo_mapShanmenFormationInfluenceConsumerBindingStatus::SubjectInvalid,
			TEXT("Consumer binding requires one valid subject entity."));
	}
	if (!::IsValid(AttributeComponent))
	{
		return RejectBinding(
			Edemo_mapShanmenFormationInfluenceConsumerBindingStatus::
				ComponentUnavailable,
			TEXT("Consumer binding requires one live attribute component."));
	}

	if (const FBindingRecord* Existing = FindBinding(SubjectEntityId))
	{
		if (Existing->Coordinator.MatchesBinding(
			SubjectEntityId, AttributeComponent))
		{
			Fdemo_mapShanmenFormationInfluenceConsumerBindingResult Replay;
			Replay.Status =
				Edemo_mapShanmenFormationInfluenceConsumerBindingStatus::
					BindingReplayed;
			Replay.Receipt = Existing->Receipt;
			return Replay;
		}
		return RejectBinding(
			Edemo_mapShanmenFormationInfluenceConsumerBindingStatus::
				SubjectBindingConflict,
			TEXT("Subject already retains a different immutable component binding."));
	}

	for (const FBindingRecord& Existing : Bindings)
	{
		if (Existing.Coordinator.MatchesBinding(
			Existing.Receipt.GetSubjectEntityId(), AttributeComponent))
		{
			return RejectBinding(
				Edemo_mapShanmenFormationInfluenceConsumerBindingStatus::
					ComponentBindingConflict,
				TEXT("Attribute component is already bound to a different subject."));
		}
	}

	FBindingRecord Record;
	if (!Fdemo_mapShanmenFormationInfluenceConsumerApplicationCoordinator::
		TryCreate(
			RunId, Content, SubjectEntityId, AttributeComponent,
			Record.Coordinator))
	{
		return RejectBinding(
			Edemo_mapShanmenFormationInfluenceConsumerBindingStatus::StateInvalid,
			TEXT("Consumer coordinator rejected the requested binding."));
	}
	Record.Receipt.HostId = HostId;
	Record.Receipt.SubjectEntityId = SubjectEntityId;
	Record.Receipt.CoordinatorId = Record.Coordinator.GetCoordinatorId();
	Record.Receipt.BindingId = MakeBindingId(
		HostId, SubjectEntityId, Record.Receipt.CoordinatorId);
	if (!Record.Receipt.IsValid())
	{
		return RejectBinding(
			Edemo_mapShanmenFormationInfluenceConsumerBindingStatus::StateInvalid,
			TEXT("Consumer binding receipt could not be derived."));
	}

	Bindings.Add(MoveTemp(Record));
	if (!IsConsistent())
	{
		Bindings.RemoveAt(Bindings.Num() - 1);
		return RejectBinding(
			Edemo_mapShanmenFormationInfluenceConsumerBindingStatus::StateInvalid,
			TEXT("Consumer binding would leave the Host inconsistent."));
	}

	Fdemo_mapShanmenFormationInfluenceConsumerBindingResult Result;
	Result.Status =
		Edemo_mapShanmenFormationInfluenceConsumerBindingStatus::Bound;
	Result.bHostStateCommitted = true;
	Result.Receipt = Bindings.Last().Receipt;
	return Result;
}

Fdemo_mapShanmenFormationInfluenceConsumerRouteResult
Fdemo_mapShanmenFormationInfluenceConsumerCommandHost::TryRoute(
	const Fdemo_mapShanmenFormationInfluenceConsumerCommand& Command)
{
	if (!IsConsistent())
	{
		return RejectRoute(
			Edemo_mapShanmenFormationInfluenceConsumerRouteStatus::HostInvalid,
			TEXT("Consumer CommandHost is not open or is inconsistent."));
	}
	if (!Command.IsValid())
	{
		return RejectRoute(
			Edemo_mapShanmenFormationInfluenceConsumerRouteStatus::CommandInvalid,
			TEXT("Consumer routing requires one valid command."));
	}
	const auto& LeaseKey = Command.GetProjection().GetLease().Key;
	if (LeaseKey.RunId != RunId || !SameContent(LeaseKey.Content, Content))
	{
		return RejectRoute(
			Edemo_mapShanmenFormationInfluenceConsumerRouteStatus::ScopeMismatch,
			TEXT("Consumer command belongs to a different Run or content scope."));
	}

	FBindingRecord* Binding = FindBinding(LeaseKey.SubjectEntityId);
	if (!Binding)
	{
		return RejectRoute(
			Edemo_mapShanmenFormationInfluenceConsumerRouteStatus::SubjectUnbound,
			TEXT("Consumer command subject has no registered component binding."));
	}
	if (!Binding->Coordinator.HasLiveAttributeComponent())
	{
		auto Result = RejectRoute(
			Edemo_mapShanmenFormationInfluenceConsumerRouteStatus::
				ComponentUnavailable,
			TEXT("Bound attribute component is no longer available."));
		Result.Binding = Binding->Receipt;
		return Result;
	}

	const auto Transaction = Binding->Coordinator.Execute(Command);
	if (!Transaction.IsSuccess())
	{
		auto Result = RejectRoute(
			Edemo_mapShanmenFormationInfluenceConsumerRouteStatus::
				TransactionRejected,
			TEXT("Consumer coordinator rejected the routed transaction."));
		Result.Binding = Binding->Receipt;
		Result.Transaction = Transaction;
		if (!Transaction.Diagnostic.IsEmpty())
		{
			Result.Diagnostic = Transaction.Diagnostic;
		}
		return Result;
	}

	Fdemo_mapShanmenFormationInfluenceConsumerRouteResult Result;
	Result.Binding = Binding->Receipt;
	Result.Transaction = Transaction;
	if (Transaction.bTransactionReplayed)
	{
		Result.Status =
			Edemo_mapShanmenFormationInfluenceConsumerRouteStatus::
				TransactionReplayed;
	}
	else
	{
		Result.Status =
			Edemo_mapShanmenFormationInfluenceConsumerRouteStatus::Routed;
		Result.bHostStateChanged = true;
	}
	return Result;
}

bool Fdemo_mapShanmenFormationInfluenceConsumerCommandHost::
TryGetBindingReceipt(
	const FGuid& SubjectEntityId,
	Fdemo_mapShanmenFormationInfluenceConsumerBindingReceipt& OutReceipt) const
{
	OutReceipt = Fdemo_mapShanmenFormationInfluenceConsumerBindingReceipt();
	const FBindingRecord* Binding = IsConsistent()
		? FindBinding(SubjectEntityId)
		: nullptr;
	if (!Binding)
	{
		return false;
	}
	OutReceipt = Binding->Receipt;
	return true;
}

bool Fdemo_mapShanmenFormationInfluenceConsumerCommandHost::
TryGetCompletedTransaction(
	const Fdemo_mapShanmenFormationInfluenceConsumerCommand& Command,
	Fdemo_mapShanmenFormationInfluenceConsumerTransactionResult& OutResult)
	const
{
	OutResult = Fdemo_mapShanmenFormationInfluenceConsumerTransactionResult();
	if (!IsConsistent() || !Command.IsValid())
	{
		return false;
	}
	const FBindingRecord* Binding = FindBinding(
		Command.GetProjection().GetLease().Key.SubjectEntityId);
	if (!Binding
		|| !Binding->Coordinator.TryGetCompletedResult(
			Command.GetCommandId(), OutResult))
	{
		return false;
	}
	return OutResult.IsSuccess()
		&& !OutResult.bTransactionReplayed
		&& OutResult.Receipt.GetRegistryReceipt().MatchesCommand(Command);
}

bool Fdemo_mapShanmenFormationInfluenceConsumerCommandHost::HasBinding(
	const FGuid& SubjectEntityId) const
{
	return IsConsistent() && FindBinding(SubjectEntityId) != nullptr;
}

bool Fdemo_mapShanmenFormationInfluenceConsumerCommandHost::HasLiveBinding(
	const FGuid& SubjectEntityId) const
{
	const FBindingRecord* Binding = IsConsistent()
		? FindBinding(SubjectEntityId)
		: nullptr;
	return Binding && Binding->Coordinator.HasLiveAttributeComponent();
}

bool Fdemo_mapShanmenFormationInfluenceConsumerCommandHost::IsConsistent()
	const
{
	if (!RunId.IsValid() || !Content.IsValid()
		|| HostId != MakeHostId(RunId, Content))
	{
		return false;
	}

	TSet<FGuid> SubjectIds;
	TSet<FGuid> BindingIds;
	for (const FBindingRecord& Binding : Bindings)
	{
		const auto& Receipt = Binding.Receipt;
		const auto& Coordinator = Binding.Coordinator;
		if (!Receipt.IsValid() || Receipt.GetHostId() != HostId
			|| !Coordinator.IsConsistent()
			|| Receipt.GetCoordinatorId() != Coordinator.GetCoordinatorId()
			|| Coordinator.GetSubjectEntityId()
				!= Receipt.GetSubjectEntityId()
			|| Coordinator.GetRegistry().GetRunId() != RunId
			|| !SameContent(Coordinator.GetRegistry().GetContent(), Content)
			|| SubjectIds.Contains(Receipt.GetSubjectEntityId())
			|| BindingIds.Contains(Receipt.GetBindingId()))
		{
			return false;
		}
		SubjectIds.Add(Receipt.GetSubjectEntityId());
		BindingIds.Add(Receipt.GetBindingId());
	}
	return true;
}

bool Fdemo_mapShanmenFormationInfluenceConsumerCommandHost::IsDrained() const
{
	if (!IsConsistent())
	{
		return false;
	}
	for (const FBindingRecord& Binding : Bindings)
	{
		if (!Binding.Coordinator.IsDrained())
		{
			return false;
		}
	}
	return true;
}

int32 Fdemo_mapShanmenFormationInfluenceConsumerCommandHost::
GetActiveApplicationCount() const
{
	if (!IsConsistent())
	{
		return INDEX_NONE;
	}
	int32 Count = 0;
	for (const FBindingRecord& Binding : Bindings)
	{
		Count += Binding.Coordinator.GetActiveApplicationCount();
	}
	return Count;
}

int32 Fdemo_mapShanmenFormationInfluenceConsumerCommandHost::
GetActiveApplicationCountForLease(const FGuid& LeaseId) const
{
	if (!IsConsistent() || !LeaseId.IsValid())
	{
		return INDEX_NONE;
	}
	int32 Count = 0;
	for (const FBindingRecord& Binding : Bindings)
	{
		const int32 BindingCount = Binding.Coordinator.GetRegistry().
			GetActiveApplicationCountForLease(LeaseId);
		if (BindingCount == INDEX_NONE)
		{
			return INDEX_NONE;
		}
		Count += BindingCount;
	}
	return Count;
}

int32 Fdemo_mapShanmenFormationInfluenceConsumerCommandHost::
GetCompletedTransactionCount() const
{
	if (!IsConsistent())
	{
		return INDEX_NONE;
	}
	int32 Count = 0;
	for (const FBindingRecord& Binding : Bindings)
	{
		Count += Binding.Coordinator.GetCompletedTransactionCount();
	}
	return Count;
}

Fdemo_mapShanmenFormationInfluenceConsumerCommandHost::FBindingRecord*
Fdemo_mapShanmenFormationInfluenceConsumerCommandHost::FindBinding(
	const FGuid& SubjectEntityId)
{
	return Bindings.FindByPredicate(
		[&SubjectEntityId](const FBindingRecord& Binding)
		{
			return SubjectEntityId.IsValid()
				&& Binding.Receipt.GetSubjectEntityId() == SubjectEntityId;
		});
}

const Fdemo_mapShanmenFormationInfluenceConsumerCommandHost::FBindingRecord*
Fdemo_mapShanmenFormationInfluenceConsumerCommandHost::FindBinding(
	const FGuid& SubjectEntityId) const
{
	return Bindings.FindByPredicate(
		[&SubjectEntityId](const FBindingRecord& Binding)
		{
			return SubjectEntityId.IsValid()
				&& Binding.Receipt.GetSubjectEntityId() == SubjectEntityId;
		});
}
