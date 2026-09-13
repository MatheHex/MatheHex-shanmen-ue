#include "demo_mapShanmenFormationScatterResourcePreparation.h"

#include "ShanmenDeterministicId.h"
#include "demo_mapShanmenItemAuthoritySubsystem.h"

namespace
{
	using EPreparationStatus =
		Edemo_mapShanmenFormationScatterResourcePreparationStatus;

	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	bool SameContent(
		const FShanmenContentStamp& Left,
		const FShanmenContentStamp& Right)
	{
		return Left.IsValid() && Right.IsValid()
			&& Left.Version == Right.Version
			&& Left.Digest == Right.Digest;
	}

	bool IsPrepareReceiptForReservation(
		const FShanmenItemTransactionReceipt& Receipt,
		const Fdemo_mapShanmenFormationScatterResourceReservationIntent&
			Reservation,
		const Fdemo_mapShanmenFormationScatterResourcePlan& Plan)
	{
		const FShanmenItemRunQuantityIntentRequest& Request =
			Reservation.GetPrepareRequest();
		return Receipt.IsSuccess()
			&& Receipt.Operation
				== EShanmenItemTransactionOperation::
					PreparePreparedRunQuantityIntent
			&& Receipt.Phase == EShanmenItemTransactionPhase::Reserved
			&& Receipt.RequestId == Request.Context.RequestId
			&& Receipt.ReservationId == Reservation.GetIntentId()
			&& Receipt.ItemInstanceId == Reservation.GetItemInstanceId()
			&& Receipt.ResourceKind == EShanmenItemResourceKind::Quantity
			&& Receipt.Amount == Reservation.GetQuantity()
			&& Receipt.ResourceBefore
				== Reservation.GetExpectedQuantityBefore()
			&& Receipt.ResourceAfter
				== Reservation.GetExpectedQuantityBefore()
			&& Receipt.AvailableAfter == Reservation.GetQuantityAfter()
			&& Receipt.PurposeId == Request.PurposeId
			&& Receipt.ReservationIds
				== TArray<FGuid>({ Plan.GetActiveRunId() });
	}

	bool IsFinalizeReceiptForReservation(
		const FShanmenItemTransactionReceipt& Receipt,
		const Fdemo_mapShanmenFormationScatterResourceReservationIntent&
			Reservation,
		const Fdemo_mapShanmenFormationScatterResourcePlan& Plan,
		const EShanmenItemTransactionPhase Phase,
		const FGuid* ExpectedRequestId = nullptr)
	{
		const int32 ExpectedAfter =
			Phase == EShanmenItemTransactionPhase::Committed
				? Reservation.GetQuantityAfter()
				: Reservation.GetExpectedQuantityBefore();
		return Receipt.IsSuccess()
			&& Receipt.Operation
				== EShanmenItemTransactionOperation::
					FinalizePreparedRunQuantityIntent
			&& Receipt.Phase == Phase
			&& (!ExpectedRequestId
				|| Receipt.RequestId == *ExpectedRequestId)
			&& Receipt.ReservationId == Reservation.GetIntentId()
			&& Receipt.ItemInstanceId == Reservation.GetItemInstanceId()
			&& Receipt.ResourceKind == EShanmenItemResourceKind::Quantity
			&& Receipt.Amount == Reservation.GetQuantity()
			&& Receipt.ResourceBefore
				== Reservation.GetExpectedQuantityBefore()
			&& Receipt.ResourceAfter == ExpectedAfter
			&& Receipt.AvailableAfter == ExpectedAfter
			&& Receipt.PurposeId
				== Reservation.GetPrepareRequest().PurposeId
			&& Receipt.ReservationIds
				== TArray<FGuid>({
					Plan.GetActiveRunId(),
					Reservation.GetPrepareRequest().Context.RequestId });
	}

	struct FPreparationEvidence
	{
		bool bValid = false;
		bool bHasPrepare = false;
		bool bHasRejectedPrepare = false;
		bool bHasPending = false;
		bool bHasCancelled = false;
		bool bHasCommitted = false;
		FString Diagnostic;
		TArray<FShanmenItemTransactionReceipt> PrepareReceipts;
		TArray<FShanmenItemTransactionReceipt> FinalizeReceipts;
	};

	FPreparationEvidence CollectEvidence(
		const FShanmenItemAuthoritySnapshot& Snapshot,
		const Fdemo_mapShanmenFormationScatterResourcePlan& Plan)
	{
		FPreparationEvidence Evidence;
		if (!Plan.IsValid())
		{
			Evidence.Diagnostic = TEXT("Resource plan is invalid.");
			return Evidence;
		}
		const auto& Reservations = Plan.GetReservations();
		Evidence.PrepareReceipts.SetNum(Reservations.Num());
		Evidence.FinalizeReceipts.SetNum(Reservations.Num());
		for (int32 Index = 0; Index < Reservations.Num(); ++Index)
		{
			const auto& Reservation = Reservations[Index];
			const FGuid PrepareRequestId =
				Reservation.GetPrepareRequest().Context.RequestId;
			const FShanmenItemProcessedRequestSnapshot* ExactPrepare = nullptr;
			int32 ExactPrepareCount = 0;
			int32 IntentPrepareCount = 0;
			const FShanmenItemTransactionReceipt* Terminal = nullptr;
			int32 TerminalCount = 0;
			for (const FShanmenItemProcessedRequestSnapshot& Processed :
				Snapshot.ProcessedRequests)
			{
				const FShanmenItemTransactionReceipt& Receipt =
					Processed.Receipt;
				if (Processed.RequestId == PrepareRequestId)
				{
					ExactPrepare = &Processed;
					++ExactPrepareCount;
				}
				if (Receipt.IsSuccess()
					&& Receipt.Operation
						== EShanmenItemTransactionOperation::
							PreparePreparedRunQuantityIntent
					&& Receipt.ReservationId == Reservation.GetIntentId())
				{
					++IntentPrepareCount;
					if (Receipt.RequestId != PrepareRequestId)
					{
						Evidence.Diagnostic = FString::Printf(
							TEXT("Reservation %d reuses the plan intent under another prepare request."),
							Index);
						return Evidence;
					}
				}
				if (Receipt.IsSuccess()
					&& Receipt.Operation
						== EShanmenItemTransactionOperation::
							FinalizePreparedRunQuantityIntent
					&& Receipt.ReservationIds.Num() == 2
					&& Receipt.ReservationIds[1] == PrepareRequestId)
				{
					Terminal = &Receipt;
					++TerminalCount;
				}
			}
			if (ExactPrepareCount > 1 || IntentPrepareCount > 1
				|| TerminalCount > 1)
			{
				Evidence.Diagnostic = FString::Printf(
					TEXT("Reservation %d has duplicate authority evidence."),
					Index);
				return Evidence;
			}

			bool bExactPrepareSuccess = false;
			if (ExactPrepare)
			{
				const FShanmenItemTransactionReceipt& Receipt =
					ExactPrepare->Receipt;
				if (Receipt.IsSuccess())
				{
					if (!IsPrepareReceiptForReservation(
							Receipt, Reservation, Plan))
					{
						Evidence.Diagnostic = FString::Printf(
							TEXT("Reservation %d prepare evidence does not match the frozen plan."),
							Index);
						return Evidence;
					}
					Evidence.PrepareReceipts[Index] = Receipt;
					Evidence.bHasPrepare = true;
					bExactPrepareSuccess = true;
				}
				else if (Receipt.IsValid()
					&& Receipt.Operation
						== EShanmenItemTransactionOperation::
							PreparePreparedRunQuantityIntent
					&& Receipt.Phase
						== EShanmenItemTransactionPhase::Rejected
					&& Receipt.RequestId == PrepareRequestId)
				{
					Evidence.bHasRejectedPrepare = true;
				}
				else
				{
					Evidence.Diagnostic = FString::Printf(
						TEXT("Reservation %d prepare request identity is occupied by invalid evidence."),
						Index);
					return Evidence;
				}
			}

			if (Terminal)
			{
				if (!bExactPrepareSuccess
					|| (Terminal->Phase
							!= EShanmenItemTransactionPhase::Cancelled
						&& Terminal->Phase
							!= EShanmenItemTransactionPhase::Committed)
					|| !IsFinalizeReceiptForReservation(
							*Terminal, Reservation, Plan,
							Terminal->Phase))
				{
					Evidence.Diagnostic = FString::Printf(
						TEXT("Reservation %d terminal evidence does not match the frozen prepare."),
						Index);
					return Evidence;
				}
				Evidence.FinalizeReceipts[Index] = *Terminal;
				Evidence.bHasCancelled |= Terminal->Phase
					== EShanmenItemTransactionPhase::Cancelled;
				Evidence.bHasCommitted |= Terminal->Phase
					== EShanmenItemTransactionPhase::Committed;
			}
			else if (bExactPrepareSuccess)
			{
				Evidence.bHasPending = true;
			}
		}
		if (Evidence.bHasCancelled && Evidence.bHasCommitted)
		{
			Evidence.Diagnostic =
				TEXT("One scatter resource plan mixes committed and cancelled terminals.");
			return Evidence;
		}
		Evidence.bValid = true;
		Evidence.Diagnostic = TEXT("Exact plan authority evidence reconstructed.");
		return Evidence;
	}

	Fdemo_mapShanmenFormationScatterResourcePreparationResult MakeResult(
		const EPreparationStatus Status,
		const Fdemo_mapShanmenFormationScatterResourcePlan& Plan,
		const FString& Diagnostic)
	{
		Fdemo_mapShanmenFormationScatterResourcePreparationResult Result;
		Result.Status = Status;
		Result.Diagnostic = Diagnostic;
		Result.Plan = Plan;
		if (Plan.IsValid())
		{
			Result.PrepareReceipts.SetNum(Plan.GetReservations().Num());
			Result.FinalizeReceipts.SetNum(Plan.GetReservations().Num());
		}
		return Result;
	}

	void ApplyEvidence(
		const FPreparationEvidence& Evidence,
		Fdemo_mapShanmenFormationScatterResourcePreparationResult& Result)
	{
		Result.PrepareReceipts = Evidence.PrepareReceipts;
		Result.FinalizeReceipts = Evidence.FinalizeReceipts;
	}

	bool IsPlanScopeCurrent(
		const Fdemo_mapShanmenFormationMasteryProjectionResult& Projection,
		const FShanmenFormationDeployment& Deployment,
		const Fdemo_mapShanmenRunCorrelation& Correlation,
		const Fdemo_mapShanmenFormationScatterResourcePlan& Plan)
	{
		return Correlation.IsValid()
			&& Deployment.IsValid()
			&& Plan.GetOwnerId() == Correlation.OwnerId
			&& Plan.GetScopeId() == Correlation.ScopeId
			&& Plan.GetActiveRunId() == Correlation.ActiveRunId
			&& Deployment.GetAction().GetOwnerId() == Plan.GetOwnerId()
			&& Deployment.GetAction().GetRunId() == Plan.GetActiveRunId()
			&& SameContent(
				Deployment.GetAction().GetContent(), Plan.GetContent())
			&& Fdemo_mapShanmenFormationScatterBatchPlanner::IsCurrentBatch(
				Projection, Deployment, Plan.GetBatch());
	}

	bool RollbackPending(
		Idemo_mapShanmenFormationScatterResourceAuthority& Authority,
		Fdemo_mapShanmenFormationScatterResourcePreparationResult& Result,
		FString& OutDiagnostic)
	{
		FShanmenItemAuthoritySnapshot Snapshot;
		if (!Authority.IsReady() || !Authority.TryCaptureSnapshot(Snapshot))
		{
			OutDiagnostic =
				TEXT("Could not reconstruct the durable authority before rollback.");
			return false;
		}
		FPreparationEvidence Evidence = CollectEvidence(Snapshot, Result.Plan);
		if (!Evidence.bValid || Evidence.bHasCommitted)
		{
			OutDiagnostic = Evidence.bHasCommitted
				? TEXT("A quantity commit appeared before rollback; the plan is forward-only.")
				: Evidence.Diagnostic;
			ApplyEvidence(Evidence, Result);
			return false;
		}
		ApplyEvidence(Evidence, Result);

		const auto& Reservations = Result.Plan.GetReservations();
		bool bAllCommandsSucceeded = true;
		for (int32 Index = Reservations.Num() - 1; Index >= 0; --Index)
		{
			if (!Result.PrepareReceipts[Index].IsSuccess()
				|| Result.FinalizeReceipts[Index].IsSuccess())
			{
				continue;
			}
			FShanmenItemRunQuantityIntentFinalizeRequest Request;
			if (!Fdemo_mapShanmenFormationScatterResourcePreparation::
					BuildFinalizeRequest(
						Result.Plan, Index, false, Request))
			{
				bAllCommandsSucceeded = false;
				continue;
			}
			const FShanmenItemDurableCommandResult Command =
				Authority.FinalizeQuantity(Request);
			Result.RollbackCommands.Add(Command);
			if (!Command.IsCommandSuccess()
				|| !IsFinalizeReceiptForReservation(
					Command.Receipt, Reservations[Index], Result.Plan,
					EShanmenItemTransactionPhase::Cancelled,
					&Request.Context.RequestId))
			{
				bAllCommandsSucceeded = false;
				continue;
			}
			Result.FinalizeReceipts[Index] = Command.Receipt;
		}

		FShanmenItemAuthoritySnapshot FinalSnapshot;
		if (!Authority.IsReady()
			|| !Authority.TryCaptureSnapshot(FinalSnapshot))
		{
			OutDiagnostic =
				TEXT("Rollback commands completed but final durable state could not be captured.");
			return false;
		}
		const FPreparationEvidence Final =
			CollectEvidence(FinalSnapshot, Result.Plan);
		ApplyEvidence(Final, Result);
		if (!Final.bValid || Final.bHasPending || Final.bHasCommitted)
		{
			OutDiagnostic = Final.bHasCommitted
				? TEXT("A quantity commit appeared during rollback; forward recovery is required.")
				: Final.bValid
					? TEXT("At least one exact prepare remains pending after the bounded rollback pass.")
					: Final.Diagnostic;
			return false;
		}
		OutDiagnostic = bAllCommandsSucceeded
			? TEXT("Every durable plan prepare is now terminally cancelled.")
			: TEXT("A rollback command failed even though no pending plan prepare remains.");
		return bAllCommandsSucceeded;
	}

	class FProductAuthority final
		: public Idemo_mapShanmenFormationScatterResourceAuthority
	{
	public:
		explicit FProductAuthority(
			Udemo_mapShanmenItemAuthoritySubsystem& InAuthority)
			: Authority(InAuthority)
		{
		}

		virtual bool IsReady() const override
		{
			return Authority.GetLifecycleState()
				== Edemo_mapShanmenItemAuthorityLifecycleState::Ready;
		}

		virtual bool TryCaptureSnapshot(
			FShanmenItemAuthoritySnapshot& OutSnapshot) const override
		{
			return Authority.TryCaptureSnapshot(OutSnapshot);
		}

		virtual FShanmenItemDurableCommandResult PrepareQuantity(
			const FShanmenItemRunQuantityIntentRequest& Request) override
		{
			return Authority.PreparePreparedRunQuantityIntentDurable(Request);
		}

		virtual FShanmenItemDurableCommandResult FinalizeQuantity(
			const FShanmenItemRunQuantityIntentFinalizeRequest& Request) override
		{
			return Authority.FinalizePreparedRunQuantityIntentDurable(Request);
		}

	private:
		Udemo_mapShanmenItemAuthoritySubsystem& Authority;
	};
}

bool Fdemo_mapShanmenFormationScatterResourcePreparationResult::IsValid()
	const
{
	if (Status == EPreparationStatus::Invalid || Diagnostic.IsEmpty())
	{
		return false;
	}
	if (!Plan.IsValid())
	{
		return Status == EPreparationStatus::PlanInvalid
			|| Status == EPreparationStatus::AuthorityNotReady
			|| Status == EPreparationStatus::SnapshotUnavailable;
	}
	const int32 ReservationCount = Plan.GetReservations().Num();
	if ((PrepareReceipts.Num() != 0
			&& PrepareReceipts.Num() != ReservationCount)
		|| (FinalizeReceipts.Num() != 0
			&& FinalizeReceipts.Num() != ReservationCount))
	{
		return false;
	}
	if (Status == EPreparationStatus::Prepared
		|| Status == EPreparationStatus::Replayed)
	{
		return IsPrepared();
	}
	if (Status == EPreparationStatus::PlanStaleRolledBack
		|| Status == EPreparationStatus::AttemptCancelled
		|| Status == EPreparationStatus::PrepareRejectedRolledBack)
	{
		return IsCancelled();
	}
	return true;
}

bool Fdemo_mapShanmenFormationScatterResourcePreparationResult::IsPrepared()
	const
{
	if ((Status != EPreparationStatus::Prepared
			&& Status != EPreparationStatus::Replayed)
		|| !Plan.IsValid()
		|| PrepareReceipts.Num() != Plan.GetReservations().Num()
		|| FinalizeReceipts.Num() != Plan.GetReservations().Num())
	{
		return false;
	}
	for (int32 Index = 0; Index < Plan.GetReservations().Num(); ++Index)
	{
		if (!IsPrepareReceiptForReservation(
				PrepareReceipts[Index], Plan.GetReservations()[Index], Plan)
			|| FinalizeReceipts[Index].IsSuccess())
		{
			return false;
		}
	}
	return true;
}

bool Fdemo_mapShanmenFormationScatterResourcePreparationResult::IsCancelled()
	const
{
	if ((Status != EPreparationStatus::PlanStaleRolledBack
			&& Status != EPreparationStatus::AttemptCancelled
			&& Status != EPreparationStatus::PrepareRejectedRolledBack)
		|| !Plan.IsValid()
		|| PrepareReceipts.Num() != Plan.GetReservations().Num()
		|| FinalizeReceipts.Num() != Plan.GetReservations().Num())
	{
		return false;
	}
	for (int32 Index = 0; Index < Plan.GetReservations().Num(); ++Index)
	{
		if (!PrepareReceipts[Index].IsSuccess())
		{
			continue;
		}
		if (!IsPrepareReceiptForReservation(
				PrepareReceipts[Index], Plan.GetReservations()[Index], Plan)
			|| !IsFinalizeReceiptForReservation(
				FinalizeReceipts[Index], Plan.GetReservations()[Index], Plan,
				EShanmenItemTransactionPhase::Cancelled))
		{
			return false;
		}
	}
	return true;
}

bool Fdemo_mapShanmenFormationScatterResourcePreparationResult::
RequiresRecovery() const
{
	return Status == EPreparationStatus::RollbackRecoveryRequired
		|| Status == EPreparationStatus::PreparationRecoveryRequired
		|| Status == EPreparationStatus::ForwardRecoveryRequired;
}

bool Fdemo_mapShanmenFormationScatterResourcePreparation::
BuildFinalizeRequest(
	const Fdemo_mapShanmenFormationScatterResourcePlan& Plan,
	const int32 ReservationIndex,
	const bool bCommit,
	FShanmenItemRunQuantityIntentFinalizeRequest& OutRequest)
{
	OutRequest = FShanmenItemRunQuantityIntentFinalizeRequest();
	if (!Plan.IsValid()
		|| !Plan.GetReservations().IsValidIndex(ReservationIndex))
	{
		return false;
	}
	const auto& Reservation = Plan.GetReservations()[ReservationIndex];
	OutRequest.Context.RunId = Plan.GetScopeId();
	OutRequest.Context.OwnerId = Plan.GetOwnerId();
	OutRequest.Context.RequestId =
		FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.Formation.ScatterResourceFinalize.r1"),
			{
				GuidDigits(Plan.GetPlanId()),
				GuidDigits(Reservation.GetIntentId()),
				GuidDigits(
					Reservation.GetPrepareRequest().Context.RequestId),
				GuidDigits(Reservation.GetItemInstanceId())
			});
	OutRequest.Context.Content = Plan.GetContent();
	OutRequest.ActiveRunId = Plan.GetActiveRunId();
	OutRequest.PrepareRequestId =
		Reservation.GetPrepareRequest().Context.RequestId;
	OutRequest.IntentId = Reservation.GetIntentId();
	OutRequest.ItemInstanceId = Reservation.GetItemInstanceId();
	OutRequest.bCommit = bCommit;
	return OutRequest.IsValid();
}

Fdemo_mapShanmenFormationScatterResourcePreparationResult
Fdemo_mapShanmenFormationScatterResourcePreparation::Prepare(
	Udemo_mapShanmenItemAuthoritySubsystem& Authority,
	const Fdemo_mapShanmenFormationMasteryProjectionResult& Projection,
	const FShanmenFormationDeployment& Deployment,
	const Fdemo_mapShanmenRunCorrelation& Correlation,
	const Fdemo_mapShanmenFormationScatterResourcePlan& Plan)
{
	FProductAuthority ProductAuthority(Authority);
	return PrepareWithAuthority(
		ProductAuthority, Projection, Deployment, Correlation, Plan);
}

Fdemo_mapShanmenFormationScatterResourcePreparationResult
Fdemo_mapShanmenFormationScatterResourcePreparation::PrepareWithAuthority(
	Idemo_mapShanmenFormationScatterResourceAuthority& Authority,
	const Fdemo_mapShanmenFormationMasteryProjectionResult& Projection,
	const FShanmenFormationDeployment& Deployment,
	const Fdemo_mapShanmenRunCorrelation& Correlation,
	const Fdemo_mapShanmenFormationScatterResourcePlan& Plan)
{
	if (!Plan.IsValid())
	{
		return MakeResult(
			EPreparationStatus::PlanInvalid, Plan,
			TEXT("Whole-batch scatter resource preparation requires a valid immutable plan."));
	}
	if (!IsInGameThread() || !Authority.IsReady())
	{
		return MakeResult(
			EPreparationStatus::AuthorityNotReady, Plan,
			TEXT("Scatter resource preparation requires the ready serialized item authority on the Game Thread."));
	}

	FShanmenItemAuthoritySnapshot Snapshot;
	if (!Authority.TryCaptureSnapshot(Snapshot))
	{
		return MakeResult(
			EPreparationStatus::SnapshotUnavailable, Plan,
			TEXT("The ready item authority could not provide a preparation snapshot."));
	}
	FPreparationEvidence Evidence = CollectEvidence(Snapshot, Plan);
	if (!Evidence.bValid)
	{
		return MakeResult(
			EPreparationStatus::EvidenceInvalid, Plan,
			Evidence.Diagnostic);
	}
	Fdemo_mapShanmenFormationScatterResourcePreparationResult Result =
		MakeResult(EPreparationStatus::Invalid, Plan, Evidence.Diagnostic);
	ApplyEvidence(Evidence, Result);

	if (Evidence.bHasCommitted)
	{
		Result.Status = EPreparationStatus::ForwardRecoveryRequired;
		Result.Diagnostic =
			TEXT("At least one plan quantity is already committed; preparation may not roll the attempt backward.");
		return Result;
	}

	if (Evidence.bHasCancelled || Evidence.bHasRejectedPrepare)
	{
		FString RollbackDiagnostic;
		if (!RollbackPending(Authority, Result, RollbackDiagnostic))
		{
			Result.Status = EPreparationStatus::RollbackRecoveryRequired;
			Result.Diagnostic = RollbackDiagnostic;
			return Result;
		}
		Result.Status = Evidence.bHasCancelled
			? EPreparationStatus::AttemptCancelled
			: EPreparationStatus::PrepareRejectedRolledBack;
		Result.Diagnostic = Evidence.bHasCancelled
			? TEXT("A prior terminal cancellation was found; every remaining exact prepare is now cancelled.")
			: TEXT("A prior rejected prepare was found; every durable earlier prepare is now cancelled.");
		return Result;
	}

	if (!IsPlanScopeCurrent(
			Projection, Deployment, Correlation, Plan))
	{
		if (!Evidence.bHasPending)
		{
			Result.Status = EPreparationStatus::PlanStale;
			Result.Diagnostic =
				TEXT("The plan no longer matches the current mastery, deployment, batch, or Run correlation.");
			return Result;
		}
		FString RollbackDiagnostic;
		if (!RollbackPending(Authority, Result, RollbackDiagnostic))
		{
			Result.Status = EPreparationStatus::RollbackRecoveryRequired;
			Result.Diagnostic = RollbackDiagnostic;
			return Result;
		}
		Result.Status = EPreparationStatus::PlanStaleRolledBack;
		Result.Diagnostic =
			TEXT("The product plan became stale after partial preparation; every durable prepare was cancelled.");
		return Result;
	}

	if (!Evidence.bHasPrepare
		&& !Fdemo_mapShanmenFormationScatterResourcePlanner::IsCurrentPlan(
			Projection, Deployment, Snapshot, Correlation, Plan))
	{
		Result.Status = EPreparationStatus::PlanStale;
		Result.Diagnostic =
			TEXT("The untouched resource plan is stale against the current item authority snapshot.");
		return Result;
	}

	bool bEveryCommandReplayed = true;
	const auto& Reservations = Plan.GetReservations();
	for (int32 Index = 0; Index < Reservations.Num(); ++Index)
	{
		const FShanmenItemDurableCommandResult Command =
			Authority.PrepareQuantity(
				Reservations[Index].GetPrepareRequest());
		Result.PrepareCommands.Add(Command);
		bEveryCommandReplayed &= Command.Status
			== EShanmenItemDurableCommandStatus::Replayed;
		if (!Command.IsCommandSuccess()
			|| !IsPrepareReceiptForReservation(
				Command.Receipt, Reservations[Index], Plan))
		{
			FString RollbackDiagnostic;
			if (!RollbackPending(Authority, Result, RollbackDiagnostic))
			{
				Result.Status =
					EPreparationStatus::RollbackRecoveryRequired;
				Result.Diagnostic = FString::Printf(
					TEXT("Prepare line %d failed and bounded rollback is unresolved: %s"),
					Index, *RollbackDiagnostic);
				return Result;
			}
			Result.Status =
				EPreparationStatus::PrepareRejectedRolledBack;
			Result.Diagnostic = FString::Printf(
				TEXT("Prepare line %d was not durably accepted; every earlier durable prepare was cancelled."),
				Index);
			return Result;
		}
		Result.PrepareReceipts[Index] = Command.Receipt;
	}

	FShanmenItemAuthoritySnapshot FinalSnapshot;
	if (!Authority.IsReady()
		|| !Authority.TryCaptureSnapshot(FinalSnapshot))
	{
		Result.Status = EPreparationStatus::PreparationRecoveryRequired;
		Result.Diagnostic =
			TEXT("All prepare commands reported success, but final durable evidence could not be captured.");
		return Result;
	}
	const FPreparationEvidence Final = CollectEvidence(FinalSnapshot, Plan);
	ApplyEvidence(Final, Result);
	if (!Final.bValid || Final.bHasCommitted || Final.bHasCancelled
		|| Final.bHasRejectedPrepare || !Final.bHasPending)
	{
		Result.Status = EPreparationStatus::PreparationRecoveryRequired;
		Result.Diagnostic = Final.bValid
			? TEXT("Final authority evidence is not the complete pending preparation set.")
			: Final.Diagnostic;
		return Result;
	}
	for (const FShanmenItemTransactionReceipt& Receipt :
		Final.PrepareReceipts)
	{
		if (!Receipt.IsSuccess())
		{
			Result.Status = EPreparationStatus::PreparationRecoveryRequired;
			Result.Diagnostic =
				TEXT("Final authority evidence is missing one exact prepare receipt.");
			return Result;
		}
	}
	Result.Status = bEveryCommandReplayed
		? EPreparationStatus::Replayed
		: EPreparationStatus::Prepared;
	Result.Diagnostic = bEveryCommandReplayed
		? TEXT("Every whole-batch prepare replayed from exact durable authority evidence.")
		: TEXT("Every whole-batch resource line is durably prepared without consuming quantity.");
	return Result;
}
