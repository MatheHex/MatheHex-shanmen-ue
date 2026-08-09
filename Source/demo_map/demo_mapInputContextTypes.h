#pragma once

#include "CoreMinimal.h"

enum class Edemo_mapInputContext : uint8
{
	Gameplay,
	SearchContainer,
	Inventory,
	Preparation,
	Settlement,
	Defeated,
	Reset
};

struct Fdemo_mapInputContextReasons
{
	bool bSearchContainer = false;
	bool bInventory = false;
	bool bPreparation = false;
	bool bSettlement = false;
	bool bDefeated = false;
	bool bReset = false;
};

struct Fdemo_mapInputContextResolution
{
	Edemo_mapInputContext Context = Edemo_mapInputContext::Gameplay;
	bool bGameplayAllowed = true;
	bool bOwnedMoveLookIgnoreRequired = false;
	bool bUseGameOnly = true;
	bool bUseGameAndUI = false;
	bool bUseUIOnly = false;
	bool bShowCursor = true;

	bool operator==(const Fdemo_mapInputContextResolution& Other) const
	{
		return Context == Other.Context
			&& bGameplayAllowed == Other.bGameplayAllowed
			&& bOwnedMoveLookIgnoreRequired
				== Other.bOwnedMoveLookIgnoreRequired
			&& bUseGameOnly == Other.bUseGameOnly
			&& bUseGameAndUI == Other.bUseGameAndUI
			&& bUseUIOnly == Other.bUseUIOnly
			&& bShowCursor == Other.bShowCursor;
	}
};

/** Pure modal-state derivation. It never calls PlayerInput or mutates a Pawn. */
struct Fdemo_mapInputContextResolver
{
	static Fdemo_mapInputContextResolution Resolve(
		const Fdemo_mapInputContextReasons& Reasons)
	{
		Fdemo_mapInputContextResolution Result;
		if (Reasons.bReset)
		{
			Result.Context = Edemo_mapInputContext::Reset;
		}
		else if (Reasons.bDefeated)
		{
			Result.Context = Edemo_mapInputContext::Defeated;
		}
		else if (Reasons.bSettlement)
		{
			Result.Context = Edemo_mapInputContext::Settlement;
		}
		else if (Reasons.bPreparation)
		{
			Result.Context = Edemo_mapInputContext::Preparation;
		}
		else if (Reasons.bSearchContainer)
		{
			Result.Context = Edemo_mapInputContext::SearchContainer;
		}
		else if (Reasons.bInventory)
		{
			Result.Context = Edemo_mapInputContext::Inventory;
		}
		Result.bGameplayAllowed =
			Result.Context == Edemo_mapInputContext::Gameplay;
		Result.bOwnedMoveLookIgnoreRequired = !Result.bGameplayAllowed;
		Result.bUseGameOnly = Result.bGameplayAllowed;
		Result.bUseGameAndUI =
			Result.Context == Edemo_mapInputContext::SearchContainer
			|| Result.Context == Edemo_mapInputContext::Inventory;
		Result.bUseUIOnly = !Result.bUseGameOnly
			&& !Result.bUseGameAndUI;
		return Result;
	}
};
