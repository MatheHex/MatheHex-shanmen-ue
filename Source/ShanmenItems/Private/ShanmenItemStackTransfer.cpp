#include "ShanmenItemStackTransfer.h"
#include "ShanmenItemTypes.h"
#include "ShanmenDeterministicId.h"

namespace
{
	FGuid Witness(const FShanmenItemTransactionReceipt& R, const FGuid& Fingerprint)
	{
		if (R.ReservationIds.Num()<2) return {};
		return FShanmenDeterministicId::FromCanonicalParts(TEXT("Shanmen.Items.Grid.Transfer.Witness.r2"),
			{R.RequestId.ToString(), Fingerprint.ToString(), R.ReceiptId.ToString(), R.ReservationId.ToString(),
			 R.ItemInstanceId.ToString(), R.ReservationIds[0].ToString(), R.ReservationIds[1].ToString(), R.PurposeId.ToString(),
			 LexToString(static_cast<uint8>(R.Operation)), LexToString(static_cast<uint8>(R.ResourceKind)),
			 LexToString(R.Amount), LexToString(R.ResourceBefore), LexToString(R.ResourceAfter), LexToString(R.AvailableAfter),
			 LexToString(R.ItemRevision), LexToString(R.AuthorityRevision)});
	}
}

bool FShanmenItemStackTransferPolicy::MetadataCompatible(const FShanmenItemRewardMetadata& A, const FShanmenItemRewardMetadata& B)
{
	if (!A.IsValid() || !B.IsValid()) return false;
	if (A==B) return true;
	// Do not erase metadata on either item or the accepted immutable source plan.
	// Role is provenance, not a fixed-stat difference between ordinary units.
	auto Left=A, Right=B; Left.RewardSourceRoleId=Right.RewardSourceRoleId=NAME_None;
	return Left.IsEmpty() && Right.IsEmpty();
}

bool FShanmenItemStackTransferPolicy::HasWitness(const FShanmenItemTransactionReceipt& R)
{
	return R.PurposeId==TEXT("Grid.Merge.r2") || R.PurposeId==TEXT("Grid.Split.r2");
}

bool FShanmenItemStackTransferPolicy::IsWitnessShapeValid(const FShanmenItemTransactionReceipt& R)
{
	const bool Split=R.PurposeId==TEXT("Grid.Split.r2");
	return HasWitness(R) && R.ReservationIds.Num()==3 && R.ReservationIds[0].IsValid() && R.ReservationIds[1].IsValid()
		&& R.ReservationIds[2].IsValid() && R.ReservationIds[0]!=R.ReservationIds[1]
		&& R.ReservationIds[2]!=R.ReservationIds[0] && R.ReservationIds[2]!=R.ReservationIds[1]
		&& R.ItemInstanceId==R.ReservationIds[Split?1:0] && R.Amount>0 && R.ResourceBefore>0
		&& R.Amount<=R.ResourceBefore && R.ResourceAfter==R.ResourceBefore-R.Amount && R.AvailableAfter==R.ResourceAfter
		&& R.AuthorityRevision>0 && (Split ? R.ItemRevision==0 && R.ResourceAfter>0 : R.ItemRevision>0);
}

void FShanmenItemStackTransferPolicy::Stamp(FShanmenItemTransactionReceipt& R, const FGuid& Source, const FGuid& Recipient, const FGuid& Fingerprint)
{
	R.ReservationIds={Source,Recipient}; R.ReservationIds.Add(Witness(R,Fingerprint));
}

bool FShanmenItemStackTransferPolicy::Validate(const FShanmenItemTransactionReceipt& R, const FGuid& Fingerprint,
	const FShanmenItemInstance& Source, const FShanmenItemInstance& Recipient, const FShanmenItemDefinition& Definition)
{
	if (!IsWitnessShapeValid(R) || !Fingerprint.IsValid() || R.ReservationIds[2]!=Witness(R,Fingerprint)
		|| Source.ItemInstanceId!=R.ReservationIds[0] || Recipient.ItemInstanceId!=R.ReservationIds[1]
		|| Source.OwnerId!=Recipient.OwnerId || Source.RunId!=Recipient.RunId || Source.DefinitionId!=Recipient.DefinitionId
		|| Source.DefinitionId!=Definition.DefinitionId || !Definition.Supports(EShanmenItemResourceKind::Quantity)
		|| R.ResourceBefore>Definition.MaxStack || R.Amount>Definition.MaxStack || Source.ChildContainerId.IsValid()
		|| Recipient.ChildContainerId.IsValid() || Source.Revision<1) return false;
	if (R.PurposeId==TEXT("Grid.Split.r2"))
	{
		const auto Child=FShanmenDeterministicId::FromCanonicalParts(TEXT("Shanmen.Items.Grid.Split.r1"),
			{Source.OwnerId.ToString(),Source.RunId.ToString(),R.RequestId.ToString(),Source.ItemInstanceId.ToString()});
		return Child==Recipient.ItemInstanceId && Source.RewardMetadata==Recipient.RewardMetadata;
	}
	return R.ItemRevision<=Source.Revision && Recipient.Revision>0 && MetadataCompatible(Source.RewardMetadata,Recipient.RewardMetadata);
}
