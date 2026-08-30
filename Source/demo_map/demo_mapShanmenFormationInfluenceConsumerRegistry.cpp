#include "demo_mapShanmenFormationInfluenceConsumerRegistry.h"

#include "ShanmenDeterministicId.h"

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

	FGuid MakeRegistryId(
		const FGuid& RunId,
		const FShanmenContentStamp& Content)
	{
		if (!RunId.IsValid() || !Content.IsValid())
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Formation.InfluenceConsumerApplicationRegistry.r1"),
			{
				GuidDigits(RunId), Content.Version.ToString(), Content.Digest
			});
	}

	bool TryMakeApplyCommand(
		const Fdemo_mapShanmenFormationInfluenceConsumerProjection& Projection,
		Fdemo_mapShanmenFormationInfluenceConsumerCommand& OutCommand)
	{
		return Fdemo_mapShanmenFormationInfluenceConsumerProjector::
			TryBuildCommand(
				Projection,
				Edemo_mapShanmenFormationInfluenceConsumerCommandOperation::Apply,
				OutCommand);
	}

	FGuid MakeApplicationId(
		const FGuid& RegistryId,
		const Fdemo_mapShanmenFormationInfluenceConsumerProjection& Projection)
	{
		Fdemo_mapShanmenFormationInfluenceConsumerCommand Apply;
		if (!RegistryId.IsValid() || !Projection.IsValid()
			|| !TryMakeApplyCommand(Projection, Apply))
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Formation.InfluenceConsumerApplication.r1"),
			{
				GuidDigits(RegistryId), GuidDigits(Apply.GetCommandId()),
				GuidDigits(Projection.GetHandle().Value),
				GuidDigits(Projection.GetLease().LeaseId),
				Projection.GetDefinition().GetTargetAttributeId().ToString()
			});
	}

	FGuid MakeReceiptId(
		const FGuid& RegistryId,
		const Fdemo_mapShanmenFormationInfluenceConsumerCommand& Command,
		const FGuid& ApplicationId,
		const int32 ActiveCountAfter)
	{
		if (!RegistryId.IsValid() || !Command.IsValid()
			|| !ApplicationId.IsValid() || ActiveCountAfter < 0)
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Formation.InfluenceConsumerApplicationReceipt.r1"),
			{
				GuidDigits(RegistryId), GuidDigits(Command.GetCommandId()),
				GuidDigits(ApplicationId),
				FString::FromInt(static_cast<int32>(Command.GetOperation())),
				FString::FromInt(ActiveCountAfter)
			});
	}

	Fdemo_mapShanmenFormationInfluenceConsumerApplicationResult Reject(
		const Edemo_mapShanmenFormationInfluenceConsumerApplicationStatus Status,
		const TCHAR* Diagnostic)
	{
		Fdemo_mapShanmenFormationInfluenceConsumerApplicationResult Result;
		Result.Status = Status;
		Result.Diagnostic = Diagnostic;
		return Result;
	}
}

bool Fdemo_mapShanmenFormationInfluenceConsumerApplicationSnapshot::
IsValid() const
{
	Fdemo_mapShanmenFormationInfluenceConsumerCommand ExpectedApply;
	return RegistryId.IsValid() && Projection.IsValid()
		&& TryMakeApplyCommand(Projection, ExpectedApply)
		&& ApplyCommandId == ExpectedApply.GetCommandId()
		&& ApplicationId == MakeApplicationId(RegistryId, Projection);
}

bool Fdemo_mapShanmenFormationInfluenceConsumerApplicationSnapshot::Matches(
	const Fdemo_mapShanmenFormationInfluenceConsumerApplicationSnapshot& Other)
	const
{
	return IsValid() && Other.IsValid()
		&& ApplicationId == Other.ApplicationId
		&& RegistryId == Other.RegistryId
		&& ApplyCommandId == Other.ApplyCommandId
		&& Projection.Matches(Other.Projection);
}

bool Fdemo_mapShanmenFormationInfluenceConsumerApplicationSnapshot::
MatchesSlot(
	const Fdemo_mapShanmenFormationInfluenceConsumerProjection& OtherProjection)
	const
{
	return IsValid() && OtherProjection.IsValid()
		&& Projection.GetLease().LeaseId
			== OtherProjection.GetLease().LeaseId
		&& Projection.GetDefinition().GetTargetAttributeId()
			== OtherProjection.GetDefinition().GetTargetAttributeId();
}

bool Fdemo_mapShanmenFormationInfluenceConsumerApplicationReceipt::
IsValid() const
{
	return RegistryId.IsValid() && Command.IsValid()
		&& ApplicationId == MakeApplicationId(
			RegistryId, Command.GetProjection())
		&& ActiveCountAfter >= 0
		&& ReceiptId == MakeReceiptId(
			RegistryId, Command, ApplicationId, ActiveCountAfter);
}

bool Fdemo_mapShanmenFormationInfluenceConsumerApplicationReceipt::Matches(
	const Fdemo_mapShanmenFormationInfluenceConsumerApplicationReceipt& Other)
	const
{
	return IsValid() && Other.IsValid() && ReceiptId == Other.ReceiptId
		&& RegistryId == Other.RegistryId
		&& ApplicationId == Other.ApplicationId
		&& Command.Matches(Other.Command)
		&& ActiveCountAfter == Other.ActiveCountAfter;
}

bool Fdemo_mapShanmenFormationInfluenceConsumerApplicationReceipt::
MatchesCommand(
	const Fdemo_mapShanmenFormationInfluenceConsumerCommand& Other) const
{
	return IsValid() && Other.IsValid() && Command.Matches(Other);
}

bool Fdemo_mapShanmenFormationInfluenceConsumerApplicationResult::
IsSuccess() const
{
	return (Status
				== Edemo_mapShanmenFormationInfluenceConsumerApplicationStatus::
					Applied
			|| Status
				== Edemo_mapShanmenFormationInfluenceConsumerApplicationStatus::
					Removed)
		&& Receipt.IsValid();
}

bool Fdemo_mapShanmenFormationInfluenceConsumerApplicationRegistry::TryCreate(
	const FGuid& RunId,
	const FShanmenContentStamp& Content,
	Fdemo_mapShanmenFormationInfluenceConsumerApplicationRegistry& OutRegistry)
{
	OutRegistry =
		Fdemo_mapShanmenFormationInfluenceConsumerApplicationRegistry();
	Fdemo_mapShanmenFormationInfluenceConsumerApplicationRegistry Candidate;
	Candidate.RunId = RunId;
	Candidate.Content = Content;
	Candidate.RegistryId = MakeRegistryId(RunId, Content);
	if (!Candidate.IsConsistent())
	{
		return false;
	}
	OutRegistry = Candidate;
	return true;
}

bool Fdemo_mapShanmenFormationInfluenceConsumerApplicationRegistry::
MatchesScope(
	const Fdemo_mapShanmenFormationInfluenceConsumerCommand& Command) const
{
	return Command.IsValid() && MatchesScope(Command.GetProjection());
}

bool Fdemo_mapShanmenFormationInfluenceConsumerApplicationRegistry::
MatchesScope(
	const Fdemo_mapShanmenFormationInfluenceConsumerProjection& Projection)
	const
{
	if (!Projection.IsValid())
	{
		return false;
	}
	const auto& LeaseKey = Projection.GetLease().Key;
	return LeaseKey.RunId == RunId && SameContent(LeaseKey.Content, Content);
}

Fdemo_mapShanmenFormationInfluenceConsumerApplicationSnapshot*
Fdemo_mapShanmenFormationInfluenceConsumerApplicationRegistry::
FindActiveByHandle(const Fdemo_mapModifierHandle& Handle)
{
	return ActiveApplications.FindByPredicate(
		[&Handle](const auto& Application)
		{
			return Handle.IsValid() && Application.GetHandle() == Handle;
		});
}

const Fdemo_mapShanmenFormationInfluenceConsumerApplicationSnapshot*
Fdemo_mapShanmenFormationInfluenceConsumerApplicationRegistry::
FindActiveByHandle(const Fdemo_mapModifierHandle& Handle) const
{
	return ActiveApplications.FindByPredicate(
		[&Handle](const auto& Application)
		{
			return Handle.IsValid() && Application.GetHandle() == Handle;
		});
}

Fdemo_mapShanmenFormationInfluenceConsumerApplicationSnapshot*
Fdemo_mapShanmenFormationInfluenceConsumerApplicationRegistry::FindActiveSlot(
	const Fdemo_mapShanmenFormationInfluenceConsumerProjection& Projection)
{
	return ActiveApplications.FindByPredicate(
		[&Projection](const auto& Application)
		{
			return Application.MatchesSlot(Projection);
		});
}

const Fdemo_mapShanmenFormationInfluenceConsumerApplicationSnapshot*
Fdemo_mapShanmenFormationInfluenceConsumerApplicationRegistry::FindActiveSlot(
	const Fdemo_mapShanmenFormationInfluenceConsumerProjection& Projection) const
{
	return ActiveApplications.FindByPredicate(
		[&Projection](const auto& Application)
		{
			return Application.MatchesSlot(Projection);
		});
}

const Fdemo_mapShanmenFormationInfluenceConsumerApplicationRegistry::
	FCompletedCommandRecord*
Fdemo_mapShanmenFormationInfluenceConsumerApplicationRegistry::
FindCompletedCommand(const FGuid& CommandId) const
{
	return CompletedCommands.FindByPredicate(
		[&CommandId](const auto& Record)
		{
			return Record.Command.GetCommandId() == CommandId;
		});
}

bool Fdemo_mapShanmenFormationInfluenceConsumerApplicationRegistry::
IsConsistent() const
{
	if (!RunId.IsValid() || !Content.IsValid()
		|| RegistryId != MakeRegistryId(RunId, Content))
	{
		return false;
	}

	struct FReplayedApplication
	{
		FGuid ApplicationId;
		FGuid LeaseId;
		FName TargetAttributeId = NAME_None;
		Fdemo_mapModifierHandle Handle;
		FGuid ApplyCommandId;
	};
	TArray<FReplayedApplication> ReplayedActive;
	TArray<FGuid> CommandIds;
	for (const auto& Record : CompletedCommands)
	{
		if (!Record.Command.IsValid() || !MatchesScope(Record.Command)
			|| CommandIds.Contains(Record.Command.GetCommandId())
			|| Record.Result.bCommandReplayed || !Record.Result.IsSuccess()
			|| !Record.Result.Receipt.MatchesCommand(Record.Command)
			|| Record.Result.Receipt.GetRegistryId() != RegistryId)
		{
			return false;
		}
		const auto& Projection = Record.Command.GetProjection();
		const FGuid ApplicationId =
			MakeApplicationId(RegistryId, Projection);
		if (Record.Result.Receipt.GetApplicationId() != ApplicationId)
		{
			return false;
		}
		switch (Record.Command.GetOperation())
		{
		case Edemo_mapShanmenFormationInfluenceConsumerCommandOperation::Apply:
			if (Record.Result.Status
					!= Edemo_mapShanmenFormationInfluenceConsumerApplicationStatus::
						Applied
				|| ReplayedActive.ContainsByPredicate(
					[&ApplicationId, &Projection](const auto& Existing)
					{
						return Existing.ApplicationId == ApplicationId
							|| (Existing.LeaseId
									== Projection.GetLease().LeaseId
								&& Existing.TargetAttributeId
									== Projection.GetDefinition().
										GetTargetAttributeId());
					}))
			{
				return false;
			}
			ReplayedActive.Add({
				ApplicationId,
				Projection.GetLease().LeaseId,
				Projection.GetDefinition().GetTargetAttributeId(),
				Projection.GetHandle(),
				Record.Command.GetCommandId()
			});
			break;
		case Edemo_mapShanmenFormationInfluenceConsumerCommandOperation::Remove:
			{
				if (Record.Result.Status
					!= Edemo_mapShanmenFormationInfluenceConsumerApplicationStatus::
						Removed)
				{
					return false;
				}
				const int32 Index = ReplayedActive.IndexOfByPredicate(
					[&ApplicationId](const auto& Existing)
					{
						return Existing.ApplicationId == ApplicationId;
					});
				if (Index == INDEX_NONE)
				{
					return false;
				}
				ReplayedActive.RemoveAt(Index);
				break;
			}
		default:
			return false;
		}
		if (Record.Result.Receipt.GetActiveCountAfter()
			!= ReplayedActive.Num())
		{
			return false;
		}
		CommandIds.Add(Record.Command.GetCommandId());
	}

	if (ReplayedActive.Num() != ActiveApplications.Num())
	{
		return false;
	}
	TArray<FGuid> ApplicationIds;
	for (const auto& Application : ActiveApplications)
	{
		if (!Application.IsValid()
			|| Application.GetRegistryId() != RegistryId
			|| !MatchesScope(Application.GetProjection())
			|| ApplicationIds.Contains(Application.GetApplicationId()))
		{
			return false;
		}
		const FReplayedApplication* Replayed = ReplayedActive.FindByPredicate(
			[&Application](const auto& Existing)
			{
				return Existing.ApplicationId == Application.GetApplicationId();
			});
		if (!Replayed || Replayed->Handle != Application.GetHandle()
			|| Replayed->ApplyCommandId != Application.GetApplyCommandId())
		{
			return false;
		}
		ApplicationIds.Add(Application.GetApplicationId());
	}
	return true;
}

Fdemo_mapShanmenFormationInfluenceConsumerApplicationResult
Fdemo_mapShanmenFormationInfluenceConsumerApplicationRegistry::Execute(
	const Fdemo_mapShanmenFormationInfluenceConsumerCommand& Command)
{
	if (!IsConsistent())
	{
		return Reject(
			Edemo_mapShanmenFormationInfluenceConsumerApplicationStatus::
				RegistryInvalid,
			TEXT("Consumer application registry is invalid or inconsistent."));
	}
	if (!Command.IsValid())
	{
		return Reject(
			Edemo_mapShanmenFormationInfluenceConsumerApplicationStatus::
				CommandInvalid,
			TEXT("Consumer application requires one valid command."));
	}
	if (!MatchesScope(Command))
	{
		return Reject(
			Edemo_mapShanmenFormationInfluenceConsumerApplicationStatus::
				ScopeMismatch,
			TEXT("Consumer command does not belong to this run/content registry."));
	}
	if (const auto* Existing = FindCompletedCommand(Command.GetCommandId()))
	{
		if (!Existing->Command.Matches(Command))
		{
			return Reject(
				Edemo_mapShanmenFormationInfluenceConsumerApplicationStatus::
					StateInvalid,
				TEXT("CommandId collides with different completed evidence."));
		}
		auto Replay = Existing->Result;
		Replay.bCommandReplayed = true;
		Replay.Diagnostic =
			TEXT("The completed consumer command replayed without state mutation.");
		return Replay;
	}

	const auto Before = *this;
	auto FinishSuccess = [this, &Before, &Command](
		const Edemo_mapShanmenFormationInfluenceConsumerApplicationStatus Status,
		const FGuid& ApplicationId,
		const TCHAR* Diagnostic)
	{
		Fdemo_mapShanmenFormationInfluenceConsumerApplicationResult Result;
		Result.Status = Status;
		Result.Diagnostic = Diagnostic;
		Result.Receipt.RegistryId = RegistryId;
		Result.Receipt.ApplicationId = ApplicationId;
		Result.Receipt.Command = Command;
		Result.Receipt.ActiveCountAfter = ActiveApplications.Num();
		Result.Receipt.ReceiptId = MakeReceiptId(
			RegistryId, Command, ApplicationId, ActiveApplications.Num());
		FCompletedCommandRecord& Record =
			CompletedCommands.AddDefaulted_GetRef();
		Record.Command = Command;
		Record.Result = Result;
		if (!IsConsistent())
		{
			*this = Before;
			return Reject(
				Edemo_mapShanmenFormationInfluenceConsumerApplicationStatus::
					StateInvalid,
				TEXT("Consumer state transition failed registry self-validation."));
		}
		return Result;
	};

	const auto& Projection = Command.GetProjection();
	switch (Command.GetOperation())
	{
	case Edemo_mapShanmenFormationInfluenceConsumerCommandOperation::Apply:
		if (FindActiveByHandle(Command.GetHandle()))
		{
			return Reject(
				Edemo_mapShanmenFormationInfluenceConsumerApplicationStatus::
					StateInvalid,
				TEXT("An unrecorded active handle violates registry history."));
		}
		if (FindActiveSlot(Projection))
		{
			return Reject(
				Edemo_mapShanmenFormationInfluenceConsumerApplicationStatus::
					SlotOccupied,
				TEXT("The lease/attribute consumer slot already has another handle."));
		}
		{
			Fdemo_mapShanmenFormationInfluenceConsumerApplicationSnapshot&
				Application = ActiveApplications.AddDefaulted_GetRef();
			Application.RegistryId = RegistryId;
			Application.Projection = Projection;
			Application.ApplyCommandId = Command.GetCommandId();
			Application.ApplicationId =
				MakeApplicationId(RegistryId, Projection);
			return FinishSuccess(
				Edemo_mapShanmenFormationInfluenceConsumerApplicationStatus::
					Applied,
				Application.ApplicationId,
				TEXT("Consumer projection became active in the pure registry."));
		}
	case Edemo_mapShanmenFormationInfluenceConsumerCommandOperation::Remove:
		{
			const auto* Slot = FindActiveSlot(Projection);
			if (!Slot)
			{
				return Reject(
					Edemo_mapShanmenFormationInfluenceConsumerApplicationStatus::
						ApplicationMissing,
					TEXT("Remove requires one matching active consumer slot."));
			}
			if (Slot->GetHandle() != Command.GetHandle())
			{
				return Reject(
					Edemo_mapShanmenFormationInfluenceConsumerApplicationStatus::
						StaleHandle,
					TEXT("Remove cannot target a newer handle in the same slot."));
			}
			const FGuid ApplicationId = Slot->GetApplicationId();
			const int32 Index = ActiveApplications.IndexOfByPredicate(
				[&ApplicationId](const auto& Application)
				{
					return Application.GetApplicationId() == ApplicationId;
				});
			if (Index == INDEX_NONE)
			{
				return Reject(
					Edemo_mapShanmenFormationInfluenceConsumerApplicationStatus::
						StateInvalid,
					TEXT("Consumer slot lookup became internally inconsistent."));
			}
			ActiveApplications.RemoveAt(Index);
			return FinishSuccess(
				Edemo_mapShanmenFormationInfluenceConsumerApplicationStatus::
					Removed,
				ApplicationId,
				TEXT("Consumer projection was removed by its exact handle."));
		}
	default:
		return Reject(
			Edemo_mapShanmenFormationInfluenceConsumerApplicationStatus::
				CommandInvalid,
			TEXT("Unknown consumer command operation."));
	}
}

bool Fdemo_mapShanmenFormationInfluenceConsumerApplicationRegistry::
TryGetActiveApplication(
	const Fdemo_mapModifierHandle& Handle,
	Fdemo_mapShanmenFormationInfluenceConsumerApplicationSnapshot&
		OutApplication) const
{
	OutApplication =
		Fdemo_mapShanmenFormationInfluenceConsumerApplicationSnapshot();
	if (!IsConsistent() || !Handle.IsValid())
	{
		return false;
	}
	const auto* Application = FindActiveByHandle(Handle);
	if (!Application)
	{
		return false;
	}
	OutApplication = *Application;
	return true;
}

int32 Fdemo_mapShanmenFormationInfluenceConsumerApplicationRegistry::
GetActiveApplicationCountForLease(const FGuid& LeaseId) const
{
	if (!IsConsistent() || !LeaseId.IsValid())
	{
		return INDEX_NONE;
	}
	int32 Count = 0;
	for (const auto& Application : ActiveApplications)
	{
		Count += Application.GetProjection().GetLease().LeaseId == LeaseId
			? 1
			: 0;
	}
	return Count;
}

bool Fdemo_mapShanmenFormationInfluenceConsumerApplicationRegistry::
TryGetActiveApplicationForSlot(
	const FGuid& LeaseId,
	const FName TargetAttributeId,
	Fdemo_mapShanmenFormationInfluenceConsumerApplicationSnapshot&
		OutApplication) const
{
	OutApplication =
		Fdemo_mapShanmenFormationInfluenceConsumerApplicationSnapshot();
	if (!IsConsistent() || !LeaseId.IsValid() || TargetAttributeId.IsNone())
	{
		return false;
	}
	const auto* Application = ActiveApplications.FindByPredicate(
		[&LeaseId, TargetAttributeId](const auto& Candidate)
		{
			return Candidate.GetProjection().GetLease().LeaseId == LeaseId
				&& Candidate.GetProjection().GetDefinition().GetTargetAttributeId()
					== TargetAttributeId;
		});
	if (!Application)
	{
		return false;
	}
	OutApplication = *Application;
	return true;
}

bool Fdemo_mapShanmenFormationInfluenceConsumerApplicationRegistry::
TryGetCompletedResult(
	const FGuid& CommandId,
	Fdemo_mapShanmenFormationInfluenceConsumerApplicationResult& OutResult) const
{
	OutResult = Fdemo_mapShanmenFormationInfluenceConsumerApplicationResult();
	if (!IsConsistent() || !CommandId.IsValid())
	{
		return false;
	}
	const auto* Record = FindCompletedCommand(CommandId);
	if (!Record)
	{
		return false;
	}
	OutResult = Record->Result;
	return OutResult.IsSuccess() && !OutResult.bCommandReplayed;
}
