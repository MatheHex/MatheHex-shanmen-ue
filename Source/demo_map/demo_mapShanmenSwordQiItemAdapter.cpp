#include "demo_mapShanmenSwordQiItemAdapter.h"

#include "ShanmenDeterministicId.h"
#include "demo_mapItemAuthority.h"
#include "demo_mapItemDefinitions.h"

namespace
{
	FGuid MakeAuthorizationId(
		int32 AuthorityRevision,
		const FGuid& SourceItemInstanceId,
		FName DefinitionId,
		FName ContentVersionId,
		const FString& ContentDigest)
	{
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.SwordQi.ItemAuthorization.r1"),
			{
				FString::FromInt(AuthorityRevision),
				SourceItemInstanceId.ToString(EGuidFormats::Digits),
				DefinitionId.ToString(),
				Fdemo_mapItemIds::WeaponSlot.ToString(),
				ContentVersionId.ToString(),
				ContentDigest
			});
	}

	Fdemo_mapShanmenSwordQiItemResult Reject(
		Edemo_mapShanmenSwordQiItemStatus Status,
		const TCHAR* Diagnostic)
	{
		Fdemo_mapShanmenSwordQiItemResult Result;
		Result.Status = Status;
		Result.Diagnostic = Diagnostic;
		return Result;
	}
}

bool Fdemo_mapShanmenSwordQiItemAuthorization::IsValid() const
{
	const Fdemo_mapItemDefinition* Definition =
		Fdemo_mapItemDefinitions::Find(DefinitionId);
	return AuthorizationId.IsValid()
		&& AuthorityRevision > 0
		&& SourceItemInstanceId.IsValid()
		&& !DefinitionId.IsNone()
		&& Fdemo_mapItemDefinitions::IsCurrentContentIdentity(
			ContentVersionId,
			ContentDigest)
		&& Definition
		&& Definition->HasGameplaySemantic(
			Edemo_mapItemGameplaySemantic::SwordQiSource)
		&& Definition->EquipmentSlotId == Fdemo_mapItemIds::WeaponSlot
		&& Definition->CompatibleSlotIds
			== TArray<FName>({ Fdemo_mapItemIds::WeaponSlot })
		&& AuthorizationId == MakeAuthorizationId(
			AuthorityRevision,
			SourceItemInstanceId,
			DefinitionId,
			ContentVersionId,
			ContentDigest);
}

bool Fdemo_mapShanmenSwordQiItemAuthorization::operator==(
	const Fdemo_mapShanmenSwordQiItemAuthorization& Other) const
{
	return AuthorizationId == Other.AuthorizationId
		&& AuthorityRevision == Other.AuthorityRevision
		&& SourceItemInstanceId == Other.SourceItemInstanceId
		&& DefinitionId == Other.DefinitionId
		&& ContentVersionId == Other.ContentVersionId
		&& ContentDigest == Other.ContentDigest;
}

bool Fdemo_mapShanmenSwordQiItemResult::IsAuthorized() const
{
	return Status == Edemo_mapShanmenSwordQiItemStatus::Authorized
		&& Authorization.IsValid();
}

Fdemo_mapShanmenSwordQiItemResult
Fdemo_mapShanmenSwordQiItemAdapter::AuthorizeEquippedSword(
	const Fdemo_mapItemAuthority& Authority)
{
	FString InvariantError;
	if (!Authority.ValidateInvariants(&InvariantError))
	{
		Fdemo_mapShanmenSwordQiItemResult Result = Reject(
			Edemo_mapShanmenSwordQiItemStatus::AuthorityInvalid,
			TEXT("Sword Qi requires a valid Runtime item authority."));
		Result.Diagnostic += TEXT(" ");
		Result.Diagnostic += InvariantError;
		return Result;
	}

	const FGuid ItemId =
		Authority.GetEquippedInstance(Fdemo_mapItemIds::WeaponSlot);
	if (!ItemId.IsValid())
	{
		return Reject(
			Edemo_mapShanmenSwordQiItemStatus::WeaponNotEquipped,
			TEXT("Sword Qi requires one exact equipped WeaponSlot item."));
	}
	const Fdemo_mapItemInstance* Item = Authority.FindInstance(ItemId);
	if (!Item)
	{
		return Reject(
			Edemo_mapShanmenSwordQiItemStatus::ItemUnavailable,
			TEXT("WeaponSlot points to an unavailable item instance."));
	}
	if (Item->OwnershipState != Edemo_mapItemOwnershipState::Equipped
		|| Item->OwnerId != Fdemo_mapItemIds::LocalPlayerOwner
		|| Item->ContainerId != Fdemo_mapItemIds::EquipmentContainer
		|| Item->EquippedSlotId != Fdemo_mapItemIds::WeaponSlot
		|| Item->Quantity != 1)
	{
		return Reject(
			Edemo_mapShanmenSwordQiItemStatus::EquipmentStateMismatch,
			TEXT("WeaponSlot item state does not match the equipment authority."));
	}

	const Fdemo_mapItemDefinition* Definition =
		Fdemo_mapItemDefinitions::Find(Item->DefinitionId);
	if (!Definition)
	{
		return Reject(
			Edemo_mapShanmenSwordQiItemStatus::DefinitionUnavailable,
			TEXT("Equipped sword definition is absent from the canonical catalog."));
	}
	if (!Definition->HasGameplaySemantic(
			Edemo_mapItemGameplaySemantic::SwordQiSource)
		|| Definition->EquipmentSlotId != Fdemo_mapItemIds::WeaponSlot
		|| Definition->CompatibleSlotIds
			!= TArray<FName>({ Fdemo_mapItemIds::WeaponSlot }))
	{
		return Reject(
			Edemo_mapShanmenSwordQiItemStatus::
				DefinitionNotSwordQiCapable,
			TEXT("An equipped item cannot source Sword Qi unless its definition explicitly says so."));
	}

	Fdemo_mapShanmenSwordQiItemResult Result;
	Result.Status = Edemo_mapShanmenSwordQiItemStatus::Authorized;
	Result.Authorization.AuthorityRevision = Authority.GetAuthorityRevision();
	Result.Authorization.SourceItemInstanceId = ItemId;
	Result.Authorization.DefinitionId = Item->DefinitionId;
	Result.Authorization.ContentVersionId =
		Fdemo_mapItemDefinitions::GetContentVersionId();
	Result.Authorization.ContentDigest =
		Fdemo_mapItemDefinitions::GetContentDigest();
	Result.Authorization.AuthorizationId = MakeAuthorizationId(
		Result.Authorization.AuthorityRevision,
		Result.Authorization.SourceItemInstanceId,
		Result.Authorization.DefinitionId,
		Result.Authorization.ContentVersionId,
		Result.Authorization.ContentDigest);
	if (!Result.Authorization.IsValid())
	{
		return Reject(
			Edemo_mapShanmenSwordQiItemStatus::AuthorityInvalid,
			TEXT("Validated equipment produced invalid Sword Qi evidence."));
	}
	Result.Diagnostic =
		TEXT("Exact equipped sword authorized for one revision-bound Sword Qi capture.");
	return Result;
}

bool Fdemo_mapShanmenSwordQiItemAdapter::IsCurrentAuthorization(
	const Fdemo_mapItemAuthority& Authority,
	const Fdemo_mapShanmenSwordQiItemAuthorization& Authorization)
{
	const Fdemo_mapShanmenSwordQiItemResult Current =
		AuthorizeEquippedSword(Authority);
	return Authorization.IsValid()
		&& Current.IsAuthorized()
		&& Current.Authorization == Authorization;
}
