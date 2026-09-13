#include "demo_mapShanmenFormationScatterWorldPublication.h"

#include "GameFramework/Actor.h"
#include "ShanmenDeterministicId.h"

namespace
{
	using EStatus =
		Edemo_mapShanmenFormationScatterWorldPublicationStatus;

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

	bool ReceiptsMatch(
		const Fdemo_mapShanmenFormationAnchorPlacementReceipt& Left,
		const Fdemo_mapShanmenFormationAnchorPlacementReceipt& Right)
	{
		return Left.IsValid() && Right.IsValid()
			&& Left.ReceiptId == Right.ReceiptId
			&& IntentsMatch(Left.Intent, Right.Intent)
			&& Left.ActorClassPath == Right.ActorClassPath
			&& Left.PlacementTag == Right.PlacementTag
			&& Left.DeploymentTag == Right.DeploymentTag;
	}

	bool IsSuccessStatus(const EStatus Status)
	{
		return Status == EStatus::Published
			|| Status == EStatus::Recovered
			|| Status == EStatus::Replayed;
	}

	Fdemo_mapShanmenFormationScatterWorldPublicationResult Reject(
		const EStatus Status,
		const TCHAR* Diagnostic,
		const int32 InitialPublishedCount,
		const int32 FinalPublishedCount,
		const int32 FailedAnchorOrder,
		const TArray<
			Fdemo_mapShanmenFormationScatterAnchorWorldPublication>&
			NewRecords)
	{
		Fdemo_mapShanmenFormationScatterWorldPublicationResult Result;
		Result.Status = Status;
		Result.Diagnostic = Diagnostic;
		Result.InitialPublishedCount = InitialPublishedCount;
		Result.FinalPublishedCount = FinalPublishedCount;
		Result.FailedAnchorOrder = FailedAnchorOrder;
		Result.NewRecords = NewRecords;
		return Result;
	}
}

FGuid Fdemo_mapShanmenFormationScatterAnchorWorldPublication::BuildRecordId(
	const Fdemo_mapShanmenFormationScatterAnchorWorldPublication& Record)
{
	if (!Record.HandoffEvidenceId.IsValid()
		|| !Record.SourceHandoffId.IsValid()
		|| !Record.DeploymentHandoffId.IsValid()
		|| Record.AnchorOrder < 0
		|| !Record.Receipt.IsValid())
	{
		return FGuid();
	}
	return FShanmenDeterministicId::FromCanonicalParts(
		TEXT("demo_map.Formation.ScatterAnchorWorldPublication.r1"),
		{
			GuidDigits(Record.HandoffEvidenceId),
			GuidDigits(Record.SourceHandoffId),
			GuidDigits(Record.DeploymentHandoffId),
			FString::FromInt(Record.AnchorOrder),
			GuidDigits(Record.Receipt.ReceiptId)
		});
}

bool Fdemo_mapShanmenFormationScatterAnchorWorldPublication::IsValid() const
{
	return RecordId.IsValid()
		&& HandoffEvidenceId.IsValid()
		&& SourceHandoffId.IsValid()
		&& DeploymentHandoffId.IsValid()
		&& AnchorOrder >= 0
		&& Receipt.IsValid()
		&& Receipt.Intent.AttemptId == DeploymentHandoffId
		&& RecordId == BuildRecordId(*this);
}

bool Fdemo_mapShanmenFormationScatterAnchorWorldPublication::operator==(
	const Fdemo_mapShanmenFormationScatterAnchorWorldPublication& Other)
	const
{
	return RecordId == Other.RecordId
		&& HandoffEvidenceId == Other.HandoffEvidenceId
		&& SourceHandoffId == Other.SourceHandoffId
		&& DeploymentHandoffId == Other.DeploymentHandoffId
		&& AnchorOrder == Other.AnchorOrder
		&& ReceiptsMatch(Receipt, Other.Receipt);
}

FGuid Fdemo_mapShanmenFormationScatterWorldPublicationLedger::BuildLedgerId(
	const Fdemo_mapShanmenFormationScatterWorldPlacementHandoffEvidence&
		HandoffEvidence,
	const FString& ActorClassPath)
{
	if (!HandoffEvidence.IsValid() || ActorClassPath.IsEmpty())
	{
		return FGuid();
	}
	return FShanmenDeterministicId::FromCanonicalParts(
		TEXT("demo_map.Formation.ScatterWorldPublicationLedger.r1"),
		{
			GuidDigits(HandoffEvidence.GetEvidenceId()),
			GuidDigits(
				HandoffEvidence.GetDeploymentEvidence()
					.GetDeployment().GetDeploymentId()),
			ActorClassPath
		});
}

bool Fdemo_mapShanmenFormationScatterWorldPublicationLedger::IsValid() const
{
	if (!LedgerId.IsValid())
	{
		return !HandoffEvidenceId.IsValid()
			&& !DeploymentId.IsValid()
			&& ActorClassPath.IsEmpty()
			&& Records.IsEmpty();
	}
	if (!HandoffEvidenceId.IsValid()
		|| !DeploymentId.IsValid()
		|| ActorClassPath.IsEmpty()
		|| Records.IsEmpty())
	{
		return false;
	}

	TSet<FGuid> RecordIds;
	TSet<FGuid> PlacementIds;
	for (int32 Index = 0; Index < Records.Num(); ++Index)
	{
		const auto& Record = Records[Index];
		if (!Record.IsValid()
			|| Record.GetHandoffEvidenceId() != HandoffEvidenceId
			|| Record.GetAnchorOrder() != Index
			|| Record.GetReceipt().ActorClassPath != ActorClassPath
			|| Record.GetReceipt().Intent.DeploymentId != DeploymentId
			|| RecordIds.Contains(Record.GetRecordId())
			|| PlacementIds.Contains(Record.GetReceipt().Intent.PlacementId))
		{
			return false;
		}
		RecordIds.Add(Record.GetRecordId());
		PlacementIds.Add(Record.GetReceipt().Intent.PlacementId);
	}
	return true;
}

bool Fdemo_mapShanmenFormationScatterWorldPublicationLedger::
IsCompatibleWith(
	const Fdemo_mapShanmenFormationScatterWorldPlacementHandoffEvidence&
		HandoffEvidence,
	const FString& ExpectedActorClassPath) const
{
	if (!IsValid() || !HandoffEvidence.IsValid()
		|| ExpectedActorClassPath.IsEmpty())
	{
		return false;
	}
	if (IsEmpty())
	{
		return true;
	}
	if (LedgerId != BuildLedgerId(
			HandoffEvidence, ExpectedActorClassPath)
		|| HandoffEvidenceId != HandoffEvidence.GetEvidenceId()
		|| DeploymentId
			!= HandoffEvidence.GetDeploymentEvidence()
				.GetDeployment().GetDeploymentId()
		|| ActorClassPath != ExpectedActorClassPath
		|| Records.Num() > HandoffEvidence.GetHandoffs().Num())
	{
		return false;
	}
	for (int32 Index = 0; Index < Records.Num(); ++Index)
	{
		const auto& Source = HandoffEvidence.GetHandoffs()[Index];
		const auto& Record = Records[Index];
		if (Record.GetSourceHandoffId() != Source.GetHandoffId()
			|| Record.GetDeploymentHandoffId()
				!= Source.GetDeploymentHandoffId()
			|| !IntentsMatch(
				Record.GetReceipt().Intent, Source.GetPlacementIntent()))
		{
			return false;
		}
	}
	return true;
}

bool Fdemo_mapShanmenFormationScatterWorldPublicationLedger::operator==(
	const Fdemo_mapShanmenFormationScatterWorldPublicationLedger& Other)
	const
{
	return LedgerId == Other.LedgerId
		&& HandoffEvidenceId == Other.HandoffEvidenceId
		&& DeploymentId == Other.DeploymentId
		&& ActorClassPath == Other.ActorClassPath
		&& Records == Other.Records;
}

FGuid Fdemo_mapShanmenFormationScatterWorldPublicationEvidence::
BuildEvidenceId(
	const Fdemo_mapShanmenFormationScatterWorldPublicationEvidence& Evidence)
{
	if (!Evidence.HandoffEvidence.IsValid()
		|| !Evidence.Ledger.IsValid()
		|| !Evidence.Ledger.IsCompatibleWith(
			Evidence.HandoffEvidence,
			Evidence.Ledger.GetActorClassPath())
		|| Evidence.Ledger.GetPublishedCount()
			!= Evidence.HandoffEvidence.GetHandoffs().Num())
	{
		return FGuid();
	}
	TArray<FString> Parts =
	{
		GuidDigits(Evidence.HandoffEvidence.GetEvidenceId()),
		GuidDigits(Evidence.Ledger.GetLedgerId()),
		FString::FromInt(Evidence.Ledger.GetPublishedCount())
	};
	for (const auto& Record : Evidence.Ledger.GetRecords())
	{
		Parts.Add(GuidDigits(Record.GetRecordId()));
	}
	return FShanmenDeterministicId::FromCanonicalParts(
		TEXT("demo_map.Formation.ScatterWorldPublicationEvidence.r1"),
		Parts);
}

bool Fdemo_mapShanmenFormationScatterWorldPublicationEvidence::IsValid() const
{
	return EvidenceId.IsValid()
		&& HandoffEvidence.IsValid()
		&& Ledger.IsValid()
		&& Ledger.IsCompatibleWith(
			HandoffEvidence, Ledger.GetActorClassPath())
		&& Ledger.GetPublishedCount() == HandoffEvidence.GetHandoffs().Num()
		&& EvidenceId == BuildEvidenceId(*this);
}

bool Fdemo_mapShanmenFormationScatterWorldPublicationEvidence::operator==(
	const Fdemo_mapShanmenFormationScatterWorldPublicationEvidence& Other)
	const
{
	return EvidenceId == Other.EvidenceId
		&& HandoffEvidence == Other.HandoffEvidence
		&& Ledger == Other.Ledger;
}

bool Fdemo_mapShanmenFormationScatterWorldPublicationResult::IsSuccess() const
{
	return IsSuccessStatus(Status) && IsValid();
}

bool Fdemo_mapShanmenFormationScatterWorldPublicationResult::IsValid() const
{
	if (Status == EStatus::Invalid || Diagnostic.IsEmpty()
		|| InitialPublishedCount < 0
		|| FinalPublishedCount < InitialPublishedCount
		|| NewRecords.Num()
			!= FinalPublishedCount - InitialPublishedCount)
	{
		return false;
	}
	for (const auto& Record : NewRecords)
	{
		if (!Record.IsValid())
		{
			return false;
		}
	}
	if (IsSuccessStatus(Status))
	{
		if (!Evidence.IsValid() || FailedAnchorOrder != INDEX_NONE
			|| FinalPublishedCount
				!= Evidence.GetLedger().GetPublishedCount())
		{
			return false;
		}
		if (Status == EStatus::Published)
		{
			return InitialPublishedCount == 0
				&& FinalPublishedCount > 0;
		}
		if (Status == EStatus::Recovered)
		{
			return InitialPublishedCount > 0
				&& FinalPublishedCount > InitialPublishedCount;
		}
		return FinalPublishedCount == InitialPublishedCount
			&& NewRecords.IsEmpty();
	}
	if (Evidence.IsValid())
	{
		return false;
	}
	if (Status == EStatus::PublicationRejected
		|| Status == EStatus::ReceiptInvalid)
	{
		return FailedAnchorOrder >= 0;
	}
	return FailedAnchorOrder == INDEX_NONE;
}

Fdemo_mapShanmenFormationScatterWorldAdapterPublicationPort::
Fdemo_mapShanmenFormationScatterWorldAdapterPublicationPort(
	UWorld* InWorld,
	TSubclassOf<AActor> InActorClass,
	Fdemo_mapShanmenFormationWorldAdapter& InAdapter)
	: World(InWorld)
	, ActorClass(InActorClass)
	, Adapter(InAdapter)
{
}

FString Fdemo_mapShanmenFormationScatterWorldAdapterPublicationPort::
GetActorClassPath() const
{
	UClass* RawClass = ActorClass.Get();
	return RawClass ? RawClass->GetPathName() : FString();
}

Fdemo_mapShanmenFormationWorldResult
Fdemo_mapShanmenFormationScatterWorldAdapterPublicationPort::Publish(
	const Fdemo_mapShanmenFormationAnchorPlacementIntent& Intent)
{
	return Adapter.TryPlaceIntent(World, ActorClass, Intent);
}

bool Fdemo_mapShanmenFormationScatterWorldPublisher::BuildRecord(
	const Fdemo_mapShanmenFormationScatterWorldPlacementHandoffEvidence&
		HandoffEvidence,
	const int32 AnchorIndex,
	const Fdemo_mapShanmenFormationAnchorPlacementReceipt& Receipt,
	Fdemo_mapShanmenFormationScatterAnchorWorldPublication& OutRecord)
{
	OutRecord =
		Fdemo_mapShanmenFormationScatterAnchorWorldPublication();
	if (!HandoffEvidence.IsValid()
		|| !HandoffEvidence.GetHandoffs().IsValidIndex(AnchorIndex)
		|| !Receipt.IsValid())
	{
		return false;
	}
	const auto& Source = HandoffEvidence.GetHandoffs()[AnchorIndex];
	if (Source.GetAnchorOrder() != AnchorIndex
		|| !IntentsMatch(Receipt.Intent, Source.GetPlacementIntent()))
	{
		return false;
	}

	OutRecord.HandoffEvidenceId = HandoffEvidence.GetEvidenceId();
	OutRecord.SourceHandoffId = Source.GetHandoffId();
	OutRecord.DeploymentHandoffId = Source.GetDeploymentHandoffId();
	OutRecord.AnchorOrder = AnchorIndex;
	OutRecord.Receipt = Receipt;
	OutRecord.RecordId =
		Fdemo_mapShanmenFormationScatterAnchorWorldPublication::
			BuildRecordId(OutRecord);
	if (!OutRecord.IsValid())
	{
		OutRecord =
			Fdemo_mapShanmenFormationScatterAnchorWorldPublication();
		return false;
	}
	return true;
}

Fdemo_mapShanmenFormationScatterWorldPublicationResult
Fdemo_mapShanmenFormationScatterWorldPublisher::Publish(
	const Fdemo_mapShanmenFormationScatterWorldPlacementHandoffEvidence&
		HandoffEvidence,
	Idemo_mapShanmenFormationScatterWorldPlacementPort& Port,
	Fdemo_mapShanmenFormationScatterWorldPublicationLedger& Ledger)
{
	const int32 InitialPublishedCount = Ledger.GetPublishedCount();
	TArray<Fdemo_mapShanmenFormationScatterAnchorWorldPublication> NewRecords;
	if (!HandoffEvidence.IsValid())
	{
		return Reject(
			EStatus::HandoffEvidenceInvalid,
			TEXT("World publication requires complete P27.22 handoff evidence."),
			InitialPublishedCount, InitialPublishedCount, INDEX_NONE,
			NewRecords);
	}
	const FString ActorClassPath = Port.GetActorClassPath();
	if (ActorClassPath.IsEmpty())
	{
		return Reject(
			EStatus::PortInvalid,
			TEXT("World publication requires one explicit Actor class."),
			InitialPublishedCount, InitialPublishedCount, INDEX_NONE,
			NewRecords);
	}
	if (!Ledger.IsValid()
		|| !Ledger.IsCompatibleWith(HandoffEvidence, ActorClassPath))
	{
		return Reject(
			EStatus::LedgerConflict,
			TEXT("The publication ledger is not the canonical prefix for this handoff and Actor class."),
			InitialPublishedCount, InitialPublishedCount, INDEX_NONE,
			NewRecords);
	}

	for (int32 Index = 0;
		Index < HandoffEvidence.GetHandoffs().Num(); ++Index)
	{
		const auto& Source = HandoffEvidence.GetHandoffs()[Index];
		const Fdemo_mapShanmenFormationWorldResult Published =
			Port.Publish(Source.GetPlacementIntent());
		if (!Published.IsPlacementSuccess())
		{
			return Reject(
				EStatus::PublicationRejected,
				TEXT("The World port rejected one canonical placement intent."),
				InitialPublishedCount, Ledger.GetPublishedCount(), Index,
				NewRecords);
		}

		Fdemo_mapShanmenFormationScatterAnchorWorldPublication Record;
		if (!BuildRecord(
				HandoffEvidence, Index, Published.PlacementReceipt, Record)
			|| Record.GetReceipt().ActorClassPath != ActorClassPath)
		{
			return Reject(
				EStatus::ReceiptInvalid,
				TEXT("The World port returned a receipt outside the canonical placement contract."),
				InitialPublishedCount, Ledger.GetPublishedCount(), Index,
				NewRecords);
		}

		if (Index < InitialPublishedCount)
		{
			if (!(Ledger.GetRecords()[Index] == Record))
			{
				return Reject(
					EStatus::LedgerConflict,
					TEXT("A previously published prefix replayed with different World evidence."),
					InitialPublishedCount, Ledger.GetPublishedCount(),
					INDEX_NONE, NewRecords);
			}
			continue;
		}

		Fdemo_mapShanmenFormationScatterWorldPublicationLedger Candidate =
			Ledger;
		if (Candidate.IsEmpty())
		{
			Candidate.HandoffEvidenceId = HandoffEvidence.GetEvidenceId();
			Candidate.DeploymentId =
				HandoffEvidence.GetDeploymentEvidence()
					.GetDeployment().GetDeploymentId();
			Candidate.ActorClassPath = ActorClassPath;
			Candidate.LedgerId =
				Fdemo_mapShanmenFormationScatterWorldPublicationLedger::
					BuildLedgerId(HandoffEvidence, ActorClassPath);
		}
		Candidate.Records.Add(Record);
		if (!Candidate.IsValid()
			|| !Candidate.IsCompatibleWith(
				HandoffEvidence, ActorClassPath))
		{
			return Reject(
				EStatus::ReceiptInvalid,
				TEXT("A successful World receipt could not extend the canonical ledger."),
				InitialPublishedCount, Ledger.GetPublishedCount(), Index,
				NewRecords);
		}
		Ledger = Candidate;
		NewRecords.Add(Record);
	}

	Fdemo_mapShanmenFormationScatterWorldPublicationEvidence Evidence;
	Evidence.HandoffEvidence = HandoffEvidence;
	Evidence.Ledger = Ledger;
	Evidence.EvidenceId =
		Fdemo_mapShanmenFormationScatterWorldPublicationEvidence::
			BuildEvidenceId(Evidence);
	if (!Evidence.IsValid())
	{
		return Reject(
			EStatus::CompletionEvidenceInvalid,
			TEXT("The complete World publication failed its final invariant check."),
			InitialPublishedCount, Ledger.GetPublishedCount(), INDEX_NONE,
			NewRecords);
	}

	Fdemo_mapShanmenFormationScatterWorldPublicationResult Result;
	Result.Status = InitialPublishedCount == Ledger.GetPublishedCount()
		? EStatus::Replayed
		: InitialPublishedCount == 0
			? EStatus::Published
			: EStatus::Recovered;
	Result.Diagnostic = Result.Status == EStatus::Replayed
		? TEXT("The complete World publication replayed without ledger growth.")
		: Result.Status == EStatus::Recovered
			? TEXT("The verified World prefix resumed through the missing suffix.")
			: TEXT("Every canonical placement intent was published in order.");
	Result.InitialPublishedCount = InitialPublishedCount;
	Result.FinalPublishedCount = Ledger.GetPublishedCount();
	Result.NewRecords = NewRecords;
	Result.Evidence = Evidence;
	if (!Result.IsValid())
	{
		return Reject(
			EStatus::CompletionEvidenceInvalid,
			TEXT("The World publication result failed its final invariant check."),
			InitialPublishedCount, Ledger.GetPublishedCount(), INDEX_NONE,
			NewRecords);
	}
	return Result;
}
