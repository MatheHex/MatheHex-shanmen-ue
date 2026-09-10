#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenSpiritShieldProductSession.h"

enum class Edemo_mapShanmenSpiritShieldHUDTone : uint8
{
	Invalid,
	Stable,
	Low,
	Depleted
};

/**
 * Pure MainHUD projection of the authoritative short Spirit Shield session.
 *
 * The caller supplies one frozen read of the product capacity and the shared
 * Combat Run timeline. This value owns no shield state, clock, World, Actor or
 * timer; it only validates and formats what the existing authorities expose.
 */
class Fdemo_mapShanmenSpiritShieldHUDPresentation
{
public:
	static bool TryProject(
		bool bSessionActive,
		float AvailableCapacity,
		float MaximumCapacity,
		int64 CurrentTick,
		int64 DeadlineTick,
		int64 TicksPerSecond,
		const FString& ActivationKeyLabel,
		Fdemo_mapShanmenSpiritShieldHUDPresentation& OutPresentation);

	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenSpiritShieldHUDPresentation& Other) const;
	Edemo_mapShanmenSpiritShieldHUDTone GetTone() const { return Tone; }
	float GetAvailableCapacity() const { return AvailableCapacity; }
	float GetMaximumCapacity() const { return MaximumCapacity; }
	int64 GetRemainingTicks() const { return RemainingTicks; }
	double GetRemainingSeconds() const { return RemainingSeconds; }
	const FString& GetDisplayText() const { return DisplayText; }

private:
	Edemo_mapShanmenSpiritShieldHUDTone Tone =
		Edemo_mapShanmenSpiritShieldHUDTone::Invalid;
	float AvailableCapacity = -1.0f;
	float MaximumCapacity = -1.0f;
	int64 RemainingTicks = INDEX_NONE;
	double RemainingSeconds = -1.0;
	FString DisplayText;
};

enum class Edemo_mapShanmenSpiritShieldInputFeedbackReason : uint8
{
	Invalid,
	Activated,
	AlreadyActive,
	InsufficientSpirit,
	ActionBusy,
	Unavailable,
	Failed
};

enum class Edemo_mapShanmenSpiritShieldInputFeedbackTone : uint8
{
	Invalid,
	Success,
	Warning,
	Error
};

/**
 * Pure player-facing projection of one authoritative Spirit Shield input result.
 *
 * It consumes typed outcome fields only. Product diagnostics remain in logs and
 * can never become arbitrary HUD copy.
 */
class Fdemo_mapShanmenSpiritShieldInputFeedbackPresentation
{
public:
	static bool TryProject(
		const Fdemo_mapShanmenSpiritShieldProductActivationResult& Result,
		const FString& ActivationKeyLabel,
		Fdemo_mapShanmenSpiritShieldInputFeedbackPresentation&
			OutPresentation);

	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenSpiritShieldInputFeedbackPresentation& Other)
		const;
	Edemo_mapShanmenSpiritShieldInputFeedbackReason GetReason() const
	{
		return Reason;
	}
	Edemo_mapShanmenSpiritShieldInputFeedbackTone GetTone() const
	{
		return Tone;
	}
	const FString& GetDisplayText() const { return DisplayText; }

private:
	Edemo_mapShanmenSpiritShieldInputFeedbackReason Reason =
		Edemo_mapShanmenSpiritShieldInputFeedbackReason::Invalid;
	Edemo_mapShanmenSpiritShieldInputFeedbackTone Tone =
		Edemo_mapShanmenSpiritShieldInputFeedbackTone::Invalid;
	FString DisplayText;
};

enum class Edemo_mapShanmenSpiritShieldImpactFeedbackKind : uint8
{
	Invalid,
	Absorbed,
	Depleted
};

/**
 * Pure player-facing projection of one newly committed shield-capacity receipt.
 *
 * Exact replays, earlier-defense outcomes and rejected commits deliberately
 * produce no feedback, so presentation can never imply a second mutation.
 */
class Fdemo_mapShanmenSpiritShieldImpactFeedbackPresentation
{
public:
	static bool TryProject(
		const Fdemo_mapShanmenSpiritShieldImpactCommitResult& Result,
		Fdemo_mapShanmenSpiritShieldImpactFeedbackPresentation&
			OutPresentation);

	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenSpiritShieldImpactFeedbackPresentation& Other)
		const;
	Edemo_mapShanmenSpiritShieldImpactFeedbackKind GetKind() const
	{
		return Kind;
	}
	float GetAbsorbedCapacity() const { return AbsorbedCapacity; }
	float GetRemainingCapacity() const { return RemainingCapacity; }
	const FString& GetDisplayText() const { return DisplayText; }

private:
	Edemo_mapShanmenSpiritShieldImpactFeedbackKind Kind =
		Edemo_mapShanmenSpiritShieldImpactFeedbackKind::Invalid;
	float AbsorbedCapacity = -1.0f;
	float RemainingCapacity = -1.0f;
	FString DisplayText;
};
