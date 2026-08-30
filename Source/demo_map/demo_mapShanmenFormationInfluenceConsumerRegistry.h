#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenFormationInfluenceConsumerProjection.h"

/** Immutable evidence for one consumer projection currently applied in-state. */
struct Fdemo_mapShanmenFormationInfluenceConsumerApplicationSnapshot
{
public:
	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenFormationInfluenceConsumerApplicationSnapshot&
			Other) const;
	bool MatchesSlot(
		const Fdemo_mapShanmenFormationInfluenceConsumerProjection& Projection)
		const;

	const FGuid& GetApplicationId() const { return ApplicationId; }
	const FGuid& GetRegistryId() const { return RegistryId; }
	const FGuid& GetApplyCommandId() const { return ApplyCommandId; }
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
	FGuid ApplicationId;
	FGuid RegistryId;
	FGuid ApplyCommandId;
	Fdemo_mapShanmenFormationInfluenceConsumerProjection Projection;

	friend class Fdemo_mapShanmenFormationInfluenceConsumerApplicationRegistry;
};

/** Self-validating evidence emitted after one successful state transition. */
struct Fdemo_mapShanmenFormationInfluenceConsumerApplicationReceipt
{
public:
	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenFormationInfluenceConsumerApplicationReceipt& Other)
		const;
	bool MatchesCommand(
		const Fdemo_mapShanmenFormationInfluenceConsumerCommand& Other) const;

	const FGuid& GetReceiptId() const { return ReceiptId; }
	const FGuid& GetRegistryId() const { return RegistryId; }
	const FGuid& GetApplicationId() const { return ApplicationId; }
	const Fdemo_mapShanmenFormationInfluenceConsumerCommand& GetCommand() const
	{
		return Command;
	}
	int32 GetActiveCountAfter() const { return ActiveCountAfter; }

private:
	FGuid ReceiptId;
	FGuid RegistryId;
	FGuid ApplicationId;
	Fdemo_mapShanmenFormationInfluenceConsumerCommand Command;
	int32 ActiveCountAfter = INDEX_NONE;

	friend class Fdemo_mapShanmenFormationInfluenceConsumerApplicationRegistry;
};

enum class Edemo_mapShanmenFormationInfluenceConsumerApplicationStatus : uint8
{
	Applied,
	Removed,
	RegistryInvalid,
	CommandInvalid,
	ScopeMismatch,
	SlotOccupied,
	ApplicationMissing,
	StaleHandle,
	StateInvalid
};

struct Fdemo_mapShanmenFormationInfluenceConsumerApplicationResult
{
	Edemo_mapShanmenFormationInfluenceConsumerApplicationStatus Status =
		Edemo_mapShanmenFormationInfluenceConsumerApplicationStatus::
			RegistryInvalid;
	FString Diagnostic;
	bool bCommandReplayed = false;
	Fdemo_mapShanmenFormationInfluenceConsumerApplicationReceipt Receipt;

	bool IsSuccess() const;
};

/**
 * Run-scoped pure-value registry for projected consumer applications.
 *
 * Only successful commands enter durable replay history. Transient missing or
 * occupied-state rejections remain retryable. The registry owns no Actor,
 * component, attribute math, queue, scheduled work, persistence, or Host
 * authority.
 */
class Fdemo_mapShanmenFormationInfluenceConsumerApplicationRegistry
{
public:
	static bool TryCreate(
		const FGuid& RunId,
		const FShanmenContentStamp& Content,
		Fdemo_mapShanmenFormationInfluenceConsumerApplicationRegistry&
			OutRegistry);

	Fdemo_mapShanmenFormationInfluenceConsumerApplicationResult Execute(
		const Fdemo_mapShanmenFormationInfluenceConsumerCommand& Command);

	bool IsValid() const { return IsConsistent(); }
	bool IsConsistent() const;
	bool IsDrained() const
	{
		return IsConsistent() && ActiveApplications.IsEmpty();
	}
	const FGuid& GetRegistryId() const { return RegistryId; }
	const FGuid& GetRunId() const { return RunId; }
	const FShanmenContentStamp& GetContent() const { return Content; }
	int32 GetActiveApplicationCount() const
	{
		return ActiveApplications.Num();
	}
	int32 GetActiveApplicationCountForLease(const FGuid& LeaseId) const;
	int32 GetCompletedCommandCount() const { return CompletedCommands.Num(); }

	bool TryGetActiveApplication(
		const Fdemo_mapModifierHandle& Handle,
		Fdemo_mapShanmenFormationInfluenceConsumerApplicationSnapshot&
			OutApplication) const;
	bool TryGetActiveApplicationForSlot(
		const FGuid& LeaseId,
		FName TargetAttributeId,
		Fdemo_mapShanmenFormationInfluenceConsumerApplicationSnapshot&
			OutApplication) const;
	bool TryGetCompletedResult(
		const FGuid& CommandId,
		Fdemo_mapShanmenFormationInfluenceConsumerApplicationResult& OutResult)
		const;

private:
	struct FCompletedCommandRecord
	{
		Fdemo_mapShanmenFormationInfluenceConsumerCommand Command;
		Fdemo_mapShanmenFormationInfluenceConsumerApplicationResult Result;
	};

	bool MatchesScope(
		const Fdemo_mapShanmenFormationInfluenceConsumerCommand& Command) const;
	bool MatchesScope(
		const Fdemo_mapShanmenFormationInfluenceConsumerProjection& Projection)
		const;
	Fdemo_mapShanmenFormationInfluenceConsumerApplicationSnapshot*
	FindActiveByHandle(const Fdemo_mapModifierHandle& Handle);
	const Fdemo_mapShanmenFormationInfluenceConsumerApplicationSnapshot*
	FindActiveByHandle(const Fdemo_mapModifierHandle& Handle) const;
	Fdemo_mapShanmenFormationInfluenceConsumerApplicationSnapshot*
	FindActiveSlot(
		const Fdemo_mapShanmenFormationInfluenceConsumerProjection& Projection);
	const Fdemo_mapShanmenFormationInfluenceConsumerApplicationSnapshot*
	FindActiveSlot(
		const Fdemo_mapShanmenFormationInfluenceConsumerProjection& Projection)
		const;
	const FCompletedCommandRecord* FindCompletedCommand(
		const FGuid& CommandId) const;

	FGuid RegistryId;
	FGuid RunId;
	FShanmenContentStamp Content;
	TArray<Fdemo_mapShanmenFormationInfluenceConsumerApplicationSnapshot>
		ActiveApplications;
	TArray<FCompletedCommandRecord> CompletedCommands;
};
