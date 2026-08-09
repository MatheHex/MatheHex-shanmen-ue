#pragma once

#include "CoreMinimal.h"

/** P5.0 immutable prototype policy. Registry effect values remain authoritative. */
struct Fdemo_mapItemUsePrototypeConfig
{
	static constexpr float HealingPillUseSeconds = 0.0f;
	static constexpr float HealingPillGlobalCooldownSeconds = 1.0f;
	static constexpr bool AllowHealingPillAtFullHealth = false;
	static constexpr bool HealingPillCooldownSharedAcrossLevels = true;
	static constexpr bool AccessoryAffectsItemUseCooldown = false;
	static constexpr bool RescaleCooldownAlreadyInProgress = false;
	static constexpr bool MaxHealthIncreaseHealsCurrentHealth = false;
	static constexpr bool MaxHealthDecreaseClampsToNewMaximum = true;
	static constexpr bool HotbarUseRequiresActiveRunPlayable = true;
};

enum class Edemo_mapItemUseStatus : uint8
{
	Success,
	NotInActiveRun,
	PlayerUnavailable,
	PlayerDefeatedOrTerminal,
	InputLocked,
	InvalidHotbarSlot,
	EmptyBinding,
	StaleBinding,
	ExpectedInstanceMismatch,
	WrongOwnership,
	UnknownDefinition,
	NotConsumable,
	InvalidStack,
	InvalidHealAmount,
	FullHealth,
	CooldownActive,
	CommitFailed
};

#if WITH_DEV_AUTOMATION_TESTS
enum class Edemo_mapItemUseFailurePoint : uint8
{
	None,
	AfterItemMutation,
	AfterHealthMutation,
	BeforeCooldownCommit
};
#endif

struct Fdemo_mapItemUseIntent
{
	FGuid ExpectedRunId;
	int32 HotbarSlotNumber = INDEX_NONE;
	FGuid ExpectedItemInstanceId;
#if WITH_DEV_AUTOMATION_TESTS
	Edemo_mapItemUseFailurePoint FailurePoint = Edemo_mapItemUseFailurePoint::None;
#endif
};

struct Fdemo_mapItemUseCooldownSnapshot
{
	float RemainingSeconds = 0.0f;
	bool bActive = false;
};

struct Fdemo_mapItemUseResult
{
	Edemo_mapItemUseStatus Status = Edemo_mapItemUseStatus::CommitFailed;
	int32 HotbarSlotNumber = INDEX_NONE;
	FGuid ItemInstanceId;
	FName DefinitionId = NAME_None;
	int32 HealRequested = 0;
	int32 HealApplied = 0;
	int32 BeforeHealth = 0;
	int32 AfterHealth = 0;
	int32 MaxHealth = 0;
	int32 BeforeStack = 0;
	int32 AfterStack = 0;
	float CooldownBefore = 0.0f;
	float CooldownAfter = 0.0f;
	bool bBindingCleared = false;
	FString Diagnostic;

	bool IsSuccess() const { return Status == Edemo_mapItemUseStatus::Success; }
};
