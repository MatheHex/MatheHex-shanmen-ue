#include "demo_mapShanmenFormationScatterWorldPlacementHandoff.h"

#include "ShanmenDeterministicId.h"

namespace
{
	using EStatus =
		Edemo_mapShanmenFormationScatterWorldPlacementHandoffStatus;

	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	bool IntentsMatch(
		const Fdemo_mapShanmenFormationAnchorPlacementIntent& Left,
		const Fdemo_mapShanmenFormationAnchorPlacementIntent& Right)
	{
		return Left.IsValid() && Right.IsValid()
			&& Left.PlacementId == Right.PlacementId
			&& Left.RunId == Right.RunId
			&& Left.OwnerId == Right.OwnerId
			&& Left.DeploymentId == Right.DeploymentId
			&& Left.AnchorDefinitionId == Right.AnchorDefinitionId
			&& Left.AnchorInstanceId == Right.AnchorInstanceId
			&& Left.WorldLocation.Equals(
				Right.WorldLocation, KINDA_SMALL_NUMBER)
			&& Left.AttemptId == Right.AttemptId
			&& Left.FulfillmentId == Right.FulfillmentId
			&& Left.DeploymentReceiptId == Right.DeploymentReceiptId
			&& Left.AuthorityRevision == Right.AuthorityRevision
			&& Left.Content.Version == Right.Content.Version
			&& Left.Content.Digest == Right.Content.Digest;
	}

	Fdemo_mapShanmenFormationScatterWorldPlacementHandoffResult Reject(
		const EStatus Status,
		const TCHAR* Diagnostic)
	{
		Fdemo_mapShanmenFormationScatterWorldPlacementHandoffResult Result;
		Result.Status = Status;
		Result.Diagnostic = Diagnostic;
		return Result;
	}
}

FGuid Fdemo_mapShanmenFormationScatterAnchorWorldPlacementHandoff::
BuildHandoffId(
	const Fdemo_mapShanmenFormationScatterAnchorWorldPlacementHandoff&
		Handoff)
{
	if (!Handoff.DeploymentCommitEvidenceId.IsValid()
		|| !Handoff.DeploymentHandoffId.IsValid()
		|| Handoff.AnchorOrder < 0
		|| !Handoff.PlacementIntent.IsValid())
	{
		return FGuid();
	}
	return FShanmenDeterministicId::FromCanonicalParts(
		TEXT("demo_map.Formation.ScatterAnchorWorldPlacementHandoff.r1"),
		{
			GuidDigits(Handoff.DeploymentCommitEvidenceId),
			GuidDigits(Handoff.DeploymentHandoffId),
			FString::FromInt(Handoff.AnchorOrder),
			GuidDigits(Handoff.PlacementIntent.PlacementId)
		});
}

bool Fdemo_mapShanmenFormationScatterAnchorWorldPlacementHandoff::IsValid()
	const
{
	return HandoffId.IsValid()
		&& DeploymentCommitEvidenceId.IsValid()
		&& DeploymentHandoffId.IsValid()
		&& AnchorOrder >= 0
		&& PlacementIntent.IsValid()
		&& PlacementIntent.AttemptId == DeploymentHandoffId
		&& HandoffId == BuildHandoffId(*this);
}

bool Fdemo_mapShanmenFormationScatterAnchorWorldPlacementHandoff::operator==(
	const Fdemo_mapShanmenFormationScatterAnchorWorldPlacementHandoff& Other)
	const
{
	return HandoffId == Other.HandoffId
		&& DeploymentCommitEvidenceId
			== Other.DeploymentCommitEvidenceId
		&& DeploymentHandoffId == Other.DeploymentHandoffId
		&& AnchorOrder == Other.AnchorOrder
		&& IntentsMatch(PlacementIntent, Other.PlacementIntent);
}

FGuid Fdemo_mapShanmenFormationScatterWorldPlacementHandoffEvidence::
BuildEvidenceId(
	const Fdemo_mapShanmenFormationScatterWorldPlacementHandoffEvidence&
		Evidence)
{
	if (!Evidence.DeploymentEvidence.IsValid()
		|| Evidence.Handoffs.IsEmpty()
		|| Evidence.TotalCommittedQuantity <= 0)
	{
		return FGuid();
	}
	TArray<FString> Parts =
	{
		GuidDigits(Evidence.DeploymentEvidence.GetEvidenceId()),
		GuidDigits(
			Evidence.DeploymentEvidence.GetDeployment().GetDeploymentId()),
		FString::FromInt(Evidence.Handoffs.Num()),
		FString::Printf(TEXT("%lld"), Evidence.TotalCommittedQuantity)
	};
	for (const auto& Handoff : Evidence.Handoffs)
	{
		if (!Handoff.IsValid())
		{
			return FGuid();
		}
		Parts.Add(GuidDigits(Handoff.GetHandoffId()));
	}
	return FShanmenDeterministicId::FromCanonicalParts(
		TEXT("demo_map.Formation.ScatterWorldPlacementHandoffEvidence.r1"),
		Parts);
}

bool Fdemo_mapShanmenFormationScatterWorldPlacementHandoffEvidence::IsValid()
	const
{
	if (!EvidenceId.IsValid()
		|| !DeploymentEvidence.IsValid()
		|| Handoffs.Num()
			!= DeploymentEvidence.GetHandoffs().Num()
		|| Handoffs.Num()
			!= DeploymentEvidence.GetDeployment().GetAnchors().Num()
		|| TotalCommittedQuantity
			!= DeploymentEvidence.GetTotalCommittedQuantity())
	{
		return false;
	}

	TSet<FGuid> HandoffIds;
	TSet<FGuid> PlacementIds;
	for (int32 Index = 0; Index < Handoffs.Num(); ++Index)
	{
		Fdemo_mapShanmenFormationScatterAnchorWorldPlacementHandoff Expected;
		const auto& Handoff = Handoffs[Index];
		if (!Fdemo_mapShanmenFormationScatterWorldPlacementHandoffBuilder::
				BuildAnchorHandoff(DeploymentEvidence, Index, Expected)
			|| !(Handoff == Expected)
			|| HandoffIds.Contains(Handoff.GetHandoffId())
			|| PlacementIds.Contains(
				Handoff.GetPlacementIntent().PlacementId))
		{
			return false;
		}
		HandoffIds.Add(Handoff.GetHandoffId());
		PlacementIds.Add(Handoff.GetPlacementIntent().PlacementId);
	}
	return EvidenceId == BuildEvidenceId(*this);
}

bool Fdemo_mapShanmenFormationScatterWorldPlacementHandoffEvidence::operator==(
	const Fdemo_mapShanmenFormationScatterWorldPlacementHandoffEvidence& Other)
	const
{
	return EvidenceId == Other.EvidenceId
		&& DeploymentEvidence == Other.DeploymentEvidence
		&& Handoffs == Other.Handoffs
		&& TotalCommittedQuantity == Other.TotalCommittedQuantity;
}

bool Fdemo_mapShanmenFormationScatterWorldPlacementHandoffResult::IsValid()
	const
{
	if (Status == EStatus::Invalid || Diagnostic.IsEmpty())
	{
		return false;
	}
	return Status == EStatus::Ready
		? Evidence.IsValid()
		: !Evidence.IsValid();
}

bool Fdemo_mapShanmenFormationScatterWorldPlacementHandoffResult::IsReady()
	const
{
	return Status == EStatus::Ready && IsValid();
}

bool Fdemo_mapShanmenFormationScatterWorldPlacementHandoffBuilder::
BuildAnchorHandoff(
	const Fdemo_mapShanmenFormationScatterDeploymentCommitEvidence&
		DeploymentEvidence,
	const int32 AnchorIndex,
	Fdemo_mapShanmenFormationScatterAnchorWorldPlacementHandoff& OutHandoff)
{
	OutHandoff =
		Fdemo_mapShanmenFormationScatterAnchorWorldPlacementHandoff();
	if (!DeploymentEvidence.IsValid()
		|| !DeploymentEvidence.GetHandoffs().IsValidIndex(AnchorIndex)
		|| !DeploymentEvidence.GetDeployment().GetAnchors().IsValidIndex(
			AnchorIndex))
	{
		return false;
	}

	const auto& Source = DeploymentEvidence.GetHandoffs()[AnchorIndex];
	const auto& Progress =
		DeploymentEvidence.GetDeployment().GetAnchors()[AnchorIndex];
	if (!Source.IsValid()
		|| Source.GetResourceFulfillment().GetAnchorOrder()
			!= AnchorIndex
		|| !Progress.IsCommitted()
		|| Progress.GetAnchorDefinitionId()
			!= Source.GetDeploymentEvidence().AnchorDefinitionId
		|| Progress.GetAnchorInstanceId()
			!= Source.GetResourceFulfillment().GetAnchorInstanceId())
	{
		return false;
	}

	OutHandoff.DeploymentCommitEvidenceId =
		DeploymentEvidence.GetEvidenceId();
	OutHandoff.DeploymentHandoffId = Source.GetHandoffId();
	OutHandoff.AnchorOrder = AnchorIndex;
	if (!Fdemo_mapShanmenFormationWorldAdapter::BuildPlacementIntent(
			DeploymentEvidence.GetDeployment(), Source.GetHandoffId(),
			Source.GetDeploymentEvidence(), Source.GetDeploymentReceipt(),
			OutHandoff.PlacementIntent))
	{
		OutHandoff =
			Fdemo_mapShanmenFormationScatterAnchorWorldPlacementHandoff();
		return false;
	}
	OutHandoff.HandoffId =
		Fdemo_mapShanmenFormationScatterAnchorWorldPlacementHandoff::
			BuildHandoffId(OutHandoff);
	if (!OutHandoff.IsValid())
	{
		OutHandoff =
			Fdemo_mapShanmenFormationScatterAnchorWorldPlacementHandoff();
		return false;
	}
	return true;
}

Fdemo_mapShanmenFormationScatterWorldPlacementHandoffResult
Fdemo_mapShanmenFormationScatterWorldPlacementHandoffBuilder::Build(
	const Fdemo_mapShanmenFormationScatterDeploymentCommitEvidence&
		DeploymentEvidence)
{
	if (!DeploymentEvidence.IsValid())
	{
		return Reject(
			EStatus::DeploymentEvidenceInvalid,
			TEXT("World-placement handoff requires complete P27.21 deployment evidence."));
	}

	Fdemo_mapShanmenFormationScatterWorldPlacementHandoffEvidence Evidence;
	Evidence.DeploymentEvidence = DeploymentEvidence;
	Evidence.TotalCommittedQuantity =
		DeploymentEvidence.GetTotalCommittedQuantity();
	for (int32 Index = 0;
		Index < DeploymentEvidence.GetHandoffs().Num(); ++Index)
	{
		Fdemo_mapShanmenFormationScatterAnchorWorldPlacementHandoff Handoff;
		if (!BuildAnchorHandoff(DeploymentEvidence, Index, Handoff))
		{
			return Reject(
				EStatus::PlacementIntentRejected,
				TEXT("One committed scatter anchor could not form the canonical placement intent."));
		}
		Evidence.Handoffs.Add(Handoff);
	}
	Evidence.EvidenceId =
		Fdemo_mapShanmenFormationScatterWorldPlacementHandoffEvidence::
			BuildEvidenceId(Evidence);
	if (!Evidence.IsValid())
	{
		return Reject(
			EStatus::CompletionEvidenceInvalid,
			TEXT("The complete placement handoff failed its final invariant check."));
	}

	Fdemo_mapShanmenFormationScatterWorldPlacementHandoffResult Result;
	Result.Status = EStatus::Ready;
	Result.Diagnostic =
		TEXT("Every P27.21 anchor is ready as one canonical placement intent.");
	Result.Evidence = Evidence;
	if (!Result.IsValid())
	{
		return Reject(
			EStatus::CompletionEvidenceInvalid,
			TEXT("The placement handoff result failed its final invariant check."));
	}
	return Result;
}
