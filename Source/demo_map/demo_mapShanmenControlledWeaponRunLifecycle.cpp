#include "demo_mapShanmenControlledWeaponRunLifecycle.h"

Fdemo_mapShanmenControlledWeaponRunEndResult
Fdemo_mapShanmenControlledWeaponRunLifecycle::TryEndRun(
	Fdemo_mapShanmenControlledWeaponRunHost& Host,
	Fdemo_mapCombatRunCoordinator& Coordinator)
{
	Fdemo_mapShanmenControlledWeaponRunEndResult Result;
	if (!Coordinator.IsActive())
	{
		Result.Diagnostic =
			TEXT("Controlled-weapon Run end requires one active Coordinator Run.");
		return Result;
	}

	Result.RunId = Coordinator.GetRunId();
	Result.BoundItemCount = Host.NumBound();
	Result.ActiveItemCount = Host.NumActive();
	Fdemo_mapShanmenControlledWeaponRunHost Candidate = Host;
	TArray<Fdemo_mapShanmenControlledWeaponHostInterruptReceipt>
		InterruptReceipts;
	if (!Candidate.IsEmpty())
	{
		if (!Candidate.IsValid())
		{
			Result.Status =
				Edemo_mapShanmenControlledWeaponRunEndStatus::HostInvalid;
			Result.Diagnostic =
				TEXT("Controlled-weapon Host is non-empty but structurally invalid.");
			return Result;
		}
		if (Candidate.GetRunId() != Result.RunId)
		{
			Result.Status =
				Edemo_mapShanmenControlledWeaponRunEndStatus::HostRunMismatch;
			Result.Diagnostic =
				TEXT("Controlled-weapon Host belongs to a different Run.");
			return Result;
		}
		if (Result.ActiveItemCount > 0
			&& (!Candidate.TryInterruptAll(InterruptReceipts)
				|| InterruptReceipts.Num() != Result.ActiveItemCount))
		{
			Result.Status =
				Edemo_mapShanmenControlledWeaponRunEndStatus::HostInterruptRejected;
			Result.Diagnostic =
				TEXT("Controlled-weapon Host could not stage every active interruption.");
			return Result;
		}

		const TArray<FGuid> OrderedItemIds =
			Candidate.GetOrderedItemInstanceIds();
		for (const FGuid& ItemInstanceId : OrderedItemIds)
		{
			if (!Candidate.TryRemoveTerminal(ItemInstanceId))
			{
				Result.Status =
					Edemo_mapShanmenControlledWeaponRunEndStatus::HostRetirementRejected;
				Result.Diagnostic =
					TEXT("Controlled-weapon Host retained a non-terminal item during Run end.");
				return Result;
			}
		}
		if (!Candidate.IsEmpty())
		{
			Result.Status =
				Edemo_mapShanmenControlledWeaponRunEndStatus::HostRetirementRejected;
			Result.Diagnostic =
				TEXT("Controlled-weapon Host did not clear after terminal retirement.");
			return Result;
		}
	}

	FString CoordinatorDiagnostic;
	if (!Coordinator.TryEndRun(Result.RunId, CoordinatorDiagnostic))
	{
		Result.Status =
			Edemo_mapShanmenControlledWeaponRunEndStatus::CoordinatorEndRejected;
		Result.Diagnostic = CoordinatorDiagnostic.IsEmpty()
			? TEXT("Combat Run Coordinator rejected the exact Run end.")
			: CoordinatorDiagnostic;
		return Result;
	}

	Host = MoveTemp(Candidate);
	Result.Status = Edemo_mapShanmenControlledWeaponRunEndStatus::Ended;
	Result.InterruptReceipts = MoveTemp(InterruptReceipts);
	Result.InterruptedItemCount = Result.InterruptReceipts.Num();
	Result.RetiredItemCount = Result.BoundItemCount;
	Result.Diagnostic =
		TEXT("Controlled weapons reached terminal state before combat Run identities were released.");
	return Result;
}
