#pragma once

#include "CoreMinimal.h"

class Fdemo_mapItemAuthority;

/** Immutable proof of one exact guard-capable item occupying WeaponSlot. */
class Fdemo_mapShanmenWeaponGuardItemAuthorization
{
public:
	bool IsValid() const;
	const FGuid& GetAuthorizationId() const { return AuthorizationId; }
	int32 GetAuthorityRevision() const { return AuthorityRevision; }
	const FGuid& GetSourceItemInstanceId() const
	{
		return SourceItemInstanceId;
	}
	FName GetDefinitionId() const { return DefinitionId; }
	FName GetContentVersionId() const { return ContentVersionId; }
	const FString& GetContentDigest() const { return ContentDigest; }

	bool operator==(
		const Fdemo_mapShanmenWeaponGuardItemAuthorization& Other) const;

private:
	friend struct Fdemo_mapShanmenWeaponGuardItemAdapter;

	FGuid AuthorizationId;
	int32 AuthorityRevision = INDEX_NONE;
	FGuid SourceItemInstanceId;
	FName DefinitionId = NAME_None;
	FName ContentVersionId = NAME_None;
	FString ContentDigest;
};

enum class Edemo_mapShanmenWeaponGuardItemStatus : uint8
{
	Authorized,
	AuthorityInvalid,
	WeaponNotEquipped,
	ItemUnavailable,
	EquipmentStateMismatch,
	DefinitionUnavailable,
	DefinitionNotGuardCapable
};

struct Fdemo_mapShanmenWeaponGuardItemResult
{
	Edemo_mapShanmenWeaponGuardItemStatus Status =
		Edemo_mapShanmenWeaponGuardItemStatus::AuthorityInvalid;
	Fdemo_mapShanmenWeaponGuardItemAuthorization Authorization;
	FString Diagnostic;

	bool IsAuthorized() const;
};

/**
 * Read-only adapter over the existing Runtime item/equipment authority.
 *
 * It never accepts a caller-selected item ID. The exact instance must be the
 * current WeaponSlot occupant and its canonical definition must explicitly
 * own WeaponGuard semantics. Evidence is revision-bound and therefore cannot
 * survive an equip, unequip, replacement or any other authority mutation.
 */
struct Fdemo_mapShanmenWeaponGuardItemAdapter
{
	static Fdemo_mapShanmenWeaponGuardItemResult AuthorizeEquippedWeapon(
		const Fdemo_mapItemAuthority& Authority);

	static bool IsCurrentAuthorization(
		const Fdemo_mapItemAuthority& Authority,
		const Fdemo_mapShanmenWeaponGuardItemAuthorization& Authorization);
};
