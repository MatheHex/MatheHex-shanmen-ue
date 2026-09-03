#pragma once

#include "CoreMinimal.h"

class Fdemo_mapItemAuthority;

/** Immutable proof of one exact Sword-Qi-capable item in WeaponSlot. */
class Fdemo_mapShanmenSwordQiItemAuthorization
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
		const Fdemo_mapShanmenSwordQiItemAuthorization& Other) const;

private:
	friend struct Fdemo_mapShanmenSwordQiItemAdapter;

	FGuid AuthorizationId;
	int32 AuthorityRevision = INDEX_NONE;
	FGuid SourceItemInstanceId;
	FName DefinitionId = NAME_None;
	FName ContentVersionId = NAME_None;
	FString ContentDigest;
};

enum class Edemo_mapShanmenSwordQiItemStatus : uint8
{
	Authorized,
	AuthorityInvalid,
	WeaponNotEquipped,
	ItemUnavailable,
	EquipmentStateMismatch,
	DefinitionUnavailable,
	DefinitionNotSwordQiCapable
};

struct Fdemo_mapShanmenSwordQiItemResult
{
	Edemo_mapShanmenSwordQiItemStatus Status =
		Edemo_mapShanmenSwordQiItemStatus::AuthorityInvalid;
	Fdemo_mapShanmenSwordQiItemAuthorization Authorization;
	FString Diagnostic;

	bool IsAuthorized() const;
};

/**
 * Read-only adapter over the existing Runtime item/equipment authority.
 *
 * The caller cannot choose an item ID. The exact current WeaponSlot occupant
 * must own the explicit SwordQiSource semantic. Evidence is revision-bound;
 * equip, unequip, replacement or any authority mutation makes it stale.
 */
struct Fdemo_mapShanmenSwordQiItemAdapter
{
	static Fdemo_mapShanmenSwordQiItemResult AuthorizeEquippedSword(
		const Fdemo_mapItemAuthority& Authority);

	static bool IsCurrentAuthorization(
		const Fdemo_mapItemAuthority& Authority,
		const Fdemo_mapShanmenSwordQiItemAuthorization& Authorization);
};
