#include "demo_mapShanmenDefenseResourceAdapter.h"

#include "ShanmenDeterministicId.h"
#include "demo_mapShanmenItemAuthoritySubsystem.h"
#include "demo_mapShanmenRunLifecycleAdapter.h"

namespace
{
	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	bool IsPreparedEquipment(
		const Fdemo_mapShanmenRunCorrelation& Correlation,
		const FGuid& ItemInstanceId)
	{
		return ItemInstanceId == Correlation.WeaponItemInstanceId
			|| ItemInstanceId == Correlation.ArmorItemInstanceId
			|| ItemInstanceId == Correlation.AccessoryItemInstanceId
			|| ItemInstanceId == Correlation.SpatialRingItemInstanceId
			|| ItemInstanceId == Correlation.BackpackItemInstanceId;
	}
}

bool Fdemo_mapShanmenDefenseResourceAdapter::BuildCommitRequest(
	const FShanmenImpactResult& Impact,
	const Fdemo_mapShanmenRunCorrelation& Correlation,
	const FShanmenContentStamp& Content,
	FShanmenItemRunResourceCommitRequest& OutRequest,
	FString& OutDiagnostic)
{
	OutRequest = FShanmenItemRunResourceCommitRequest();
	if (!Impact.bAccepted || !Impact.ImpactId.IsValid()
		|| !Impact.IsConserved())
	{
		OutDiagnostic =
			TEXT("Defense resource commit requires one accepted, conserved impact receipt.");
		return false;
	}
	if (!Correlation.IsValid() || !Content.IsValid())
	{
		OutDiagnostic =
			TEXT("Defense resource commit requires the exact active Run correlation and content stamp.");
		return false;
	}

	TSet<FGuid> UniqueReservationIds;
	TArray<FString> RequestParts =
	{
		GuidDigits(Correlation.OwnerId),
		GuidDigits(Correlation.ScopeId),
		GuidDigits(Correlation.ActiveRunId),
		GuidDigits(Impact.ImpactId)
	};
	for (const FShanmenDefenseLayerResult& Layer : Impact.TriggeredLayers)
	{
		if (!Layer.bRequiresCommit)
		{
			continue;
		}
		if (!Layer.LayerId.IsValid() || !Layer.SourceInstanceId.IsValid()
			|| UniqueReservationIds.Contains(Layer.LayerId)
			|| !IsPreparedEquipment(Correlation, Layer.SourceInstanceId))
		{
			OutDiagnostic =
				TEXT("A triggered resource-backed defense layer has no unique reservation or exact prepared-equipment source.");
			return false;
		}
		UniqueReservationIds.Add(Layer.LayerId);
		FShanmenItemRunResourceCommitLine& Line =
			OutRequest.OrderedLines.AddDefaulted_GetRef();
		Line.ReservationId = Layer.LayerId;
		Line.ItemInstanceId = Layer.SourceInstanceId;
		RequestParts.Add(GuidDigits(Line.ReservationId));
		RequestParts.Add(GuidDigits(Line.ItemInstanceId));
	}
	if (OutRequest.OrderedLines.IsEmpty())
	{
		OutDiagnostic = TEXT("The accepted impact triggered no resource-backed defense layer.");
		return false;
	}

	RequestParts.Insert(
		FString::FromInt(OutRequest.OrderedLines.Num()), 4);
	OutRequest.Context.RunId = Correlation.ScopeId;
	OutRequest.Context.OwnerId = Correlation.OwnerId;
	OutRequest.Context.RequestId =
		FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Product.DefenseResourceCommit.Request.r1"),
			RequestParts);
	OutRequest.Context.Content = Content;
	OutRequest.ActiveRunId = Correlation.ActiveRunId;
	OutRequest.PurposeId = TEXT("Combat.Impact.DefenseResources");
	if (!OutRequest.IsValid())
	{
		OutRequest = FShanmenItemRunResourceCommitRequest();
		OutDiagnostic =
			TEXT("Triggered defense layers could not form a canonical item-authority request.");
		return false;
	}
	OutDiagnostic.Reset();
	return true;
}

Fdemo_mapShanmenDefenseResourceCommitResult
Fdemo_mapShanmenDefenseResourceAdapter::CommitTriggered(
	Udemo_mapShanmenItemAuthoritySubsystem& Authority,
	const FShanmenImpactResult& Impact)
{
	Fdemo_mapShanmenDefenseResourceCommitResult Result;
	auto Reject = [&Result](
		Edemo_mapShanmenDefenseResourceCommitStatus Status,
		const FString& Diagnostic)
	{
		Result.Status = Status;
		Result.Diagnostic = Diagnostic;
		return Result;
	};
	if (!Impact.bAccepted || !Impact.ImpactId.IsValid()
		|| !Impact.IsConserved())
	{
		return Reject(
			Edemo_mapShanmenDefenseResourceCommitStatus::ImpactInvalid,
			TEXT("Defense resource commit rejected an invalid impact receipt."));
	}
	if (!Impact.TriggeredLayers.ContainsByPredicate(
			[](const FShanmenDefenseLayerResult& Layer)
			{
				return Layer.bRequiresCommit;
			}))
	{
		Result.Status =
			Edemo_mapShanmenDefenseResourceCommitStatus::NoCommitRequired;
		Result.Diagnostic =
			TEXT("The accepted impact requires no item resource commit.");
		return Result;
	}
	if (!IsInGameThread()
		|| Authority.GetLifecycleState()
			!= Edemo_mapShanmenItemAuthorityLifecycleState::Ready)
	{
		return Reject(
			Edemo_mapShanmenDefenseResourceCommitStatus::AuthorityNotReady,
			TEXT("Defense resource commit requires the ready GameInstance authority on the Game Thread."));
	}

	Fdemo_mapShanmenRunCorrelation Correlation;
	if (!Fdemo_mapShanmenRunLifecycleAdapter::TryGetActiveRunCorrelation(
			Authority, Correlation, &Result.Diagnostic))
	{
		Result.Status =
			Edemo_mapShanmenDefenseResourceCommitStatus::RunCorrelationInvalid;
		return Result;
	}
	FShanmenItemAuthoritySnapshot Snapshot;
	if (!Authority.TryCaptureSnapshot(Snapshot))
	{
		return Reject(
			Edemo_mapShanmenDefenseResourceCommitStatus::AuthorityNotReady,
			TEXT("Item authority snapshot disappeared before defense commit."));
	}
	if (!BuildCommitRequest(
			Impact, Correlation, Snapshot.Content,
			Result.Request, Result.Diagnostic))
	{
		Result.Status =
			Edemo_mapShanmenDefenseResourceCommitStatus::RequestInvalid;
		return Result;
	}
	Result.AuthorityCommand =
		Authority.CommitPreparedRunResourcesDurable(Result.Request);
	if (!Result.AuthorityCommand.IsCommandSuccess())
	{
		return Reject(
			Edemo_mapShanmenDefenseResourceCommitStatus::AuthorityRejected,
			Result.AuthorityCommand.Diagnostic.IsEmpty()
				? TEXT("Durable defense resource commit was rejected.")
				: Result.AuthorityCommand.Diagnostic);
	}
	Result.Status =
		Edemo_mapShanmenDefenseResourceCommitStatus::Committed;
	Result.Diagnostic =
		TEXT("Every triggered defense resource persisted in one ActiveRun transaction.");
	return Result;
}
