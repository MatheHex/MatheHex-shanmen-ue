#include "demo_mapShanmenWeaponGuardItemAdapter.h"

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
			TEXT("demo_map.Sword.WeaponGuard.ItemAuthorization.r1"),
			{
				FString::FromInt(AuthorityRevision),
				SourceItemInstanceId.ToString(EGuidFormats::Digits),
				DefinitionId.ToString(),
				Fdemo_mapItemIds::WeaponSlot.ToString(),
				ContentVersionId.ToString(),
				ContentDigest
			});
	}

	Fdemo_mapShanmenWeaponGuardItemResult Reject(
		Edemo_mapShanmenWeaponGuardItemStatus Status,
		const TCHAR* Diagnostic)
	{
		Fdemo_mapShanmenWeaponGuardItemResult Result;
		Result.Status = Status;
		Result.Diagnostic = Diagnostic;
		return Result;
	}
}

bool Fdemo_mapShanmenWeaponGuardItemAuthorization::IsValid() const
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
			Edemo_mapItemGameplaySemantic::WeaponGuard)
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

bool Fdemo_mapShanmenWeaponGuardItemAuthorization::operator==(
	const Fdemo_mapShanmenWeaponGuardItemAuthorization& Other) const
{
	return AuthorizationId == Other.AuthorizationId
		&& AuthorityRevision == Other.AuthorityRevision
		&& SourceItemInstanceId == Other.SourceItemInstanceId
		&& DefinitionId == Other.DefinitionId
		&& ContentVersionId == Other.ContentVersionId
		&& ContentDigest == Other.ContentDigest;
}

bool Fdemo_mapShanmenWeaponGuardItemResult::IsAuthorized() const
{
	return Status == Edemo_mapShanmenWeaponGuardItemStatus::Authorized
		&& Authorization.IsValid();
}

Fdemo_mapShanmenWeaponGuardItemResult
Fdemo_mapShanmenWeaponGuardItemAdapter::AuthorizeEquippedWeapon(
	const Fdemo_mapItemAuthority& Authority)
{
	FString InvariantError;
	if (!Authority.ValidateInvariants(&InvariantError))
	{
		Fdemo_mapShanmenWeaponGuardItemResult Result = Reject(
			Edemo_mapShanmenWeaponGuardItemStatus::AuthorityInvalid,
			TEXT("Weapon guard requires a valid Runtime item authority."));
		Result.Diagnostic += TEXT(" ");
		Result.Diagnostic += InvariantError;
		return Result;
	}

	const FGuid ItemId =
		Authority.GetEquippedInstance(Fdemo_mapItemIds::WeaponSlot);
	if (!ItemId.IsValid())
	{
		return Reject(
			Edemo_mapShanmenWeaponGuardItemStatus::WeaponNotEquipped,
			TEXT("Weapon guard requires one exact equipped WeaponSlot item."));
	}
	const Fdemo_mapItemInstance* Item = Authority.FindInstance(ItemId);
	if (!Item)
	{
		return Reject(
			Edemo_mapShanmenWeaponGuardItemStatus::ItemUnavailable,
			TEXT("WeaponSlot points to an unavailable item instance."));
	}
	if (Item->OwnershipState != Edemo_mapItemOwnershipState::Equipped
		|| Item->OwnerId != Fdemo_mapItemIds::LocalPlayerOwner
		|| Item->ContainerId != Fdemo_mapItemIds::EquipmentContainer
		|| Item->EquippedSlotId != Fdemo_mapItemIds::WeaponSlot
		|| Item->Quantity != 1)
	{
		return Reject(
			Edemo_mapShanmenWeaponGuardItemStatus::EquipmentStateMismatch,
			TEXT("WeaponSlot item state does not match the equipment authority."));
	}

	const Fdemo_mapItemDefinition* Definition =
		Fdemo_mapItemDefinitions::Find(Item->DefinitionId);
	if (!Definition)
	{
		return Reject(
			Edemo_mapShanmenWeaponGuardItemStatus::DefinitionUnavailable,
			TEXT("Equipped weapon definition is absent from the canonical catalog."));
	}
	if (!Definition->HasGameplaySemantic(
			Edemo_mapItemGameplaySemantic::WeaponGuard)
		|| Definition->EquipmentSlotId != Fdemo_mapItemIds::WeaponSlot
		|| Definition->CompatibleSlotIds
			!= TArray<FName>({ Fdemo_mapItemIds::WeaponSlot }))
	{
		return Reject(
			Edemo_mapShanmenWeaponGuardItemStatus::
				DefinitionNotGuardCapable,
			TEXT("An equipped item cannot guard unless its definition explicitly says so."));
	}

	Fdemo_mapShanmenWeaponGuardItemResult Result;
	Result.Status = Edemo_mapShanmenWeaponGuardItemStatus::Authorized;
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
			Edemo_mapShanmenWeaponGuardItemStatus::AuthorityInvalid,
			TEXT("Validated equipment produced invalid weapon-guard evidence."));
	}
	Result.Diagnostic =
		TEXT("Exact equipped weapon authorized for one revision-bound guard start.");
	return Result;
}

bool Fdemo_mapShanmenWeaponGuardItemAdapter::IsCurrentAuthorization(
	const Fdemo_mapItemAuthority& Authority,
	const Fdemo_mapShanmenWeaponGuardItemAuthorization& Authorization)
{
	const Fdemo_mapShanmenWeaponGuardItemResult Current =
		AuthorizeEquippedWeapon(Authority);
	return Authorization.IsValid()
		&& Current.IsAuthorized()
		&& Current.Authorization == Authorization;
}
