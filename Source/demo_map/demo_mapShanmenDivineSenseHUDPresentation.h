#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenDivineSenseLogicalInputAdapter.h"

enum class Edemo_mapShanmenDivineSenseMarkerPlacement : uint8
{
	Invalid,
	WorldProjected,
	ScreenEdge
};

/**
 * Pure screen-space plan for one Divine Sense reveal marker.
 *
 * A successful world projection is preserved while it remains inside the
 * readable HUD area. Off-screen and behind-camera subjects are clamped to an
 * edge using either the projected displacement or a camera-relative bearing.
 * The plan owns no Canvas, World, Actor, timer or mutable presentation state.
 */
class Fdemo_mapShanmenDivineSenseHUDMarkerPlan
{
public:
	static bool TryPlan(
		const FVector2D& CanvasSize,
		bool bWorldProjectionSucceeded,
		const FVector2D& ProjectedScreenPosition,
		const FVector2D& CameraRelativeFallbackBearing,
		Fdemo_mapShanmenDivineSenseHUDMarkerPlan& OutPlan);

	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenDivineSenseHUDMarkerPlan& Other) const;
	bool IsAtScreenEdge() const
	{
		return IsValid()
			&& Placement
				== Edemo_mapShanmenDivineSenseMarkerPlacement::ScreenEdge;
	}
	Edemo_mapShanmenDivineSenseMarkerPlacement GetPlacement() const
	{
		return Placement;
	}
	const FVector2D& GetCanvasSize() const { return CanvasSize; }
	const FVector2D& GetScreenPosition() const { return ScreenPosition; }
	const FVector2D& GetEdgeDirection() const { return EdgeDirection; }

	static double GetHorizontalSafeMargin() { return 36.0; }
	static double GetTopSafeMargin() { return 112.0; }
	static double GetBottomSafeMargin() { return 56.0; }

private:
	Edemo_mapShanmenDivineSenseMarkerPlacement Placement =
		Edemo_mapShanmenDivineSenseMarkerPlacement::Invalid;
	FVector2D CanvasSize = FVector2D::ZeroVector;
	FVector2D ScreenPosition = FVector2D::ZeroVector;
	FVector2D EdgeDirection = FVector2D::ZeroVector;
};

enum class Edemo_mapShanmenDivineSenseHUDFeedbackReason : uint8
{
	Invalid,
	InsufficientSpirit,
	PulseLimitReached,
	RetryRequired,
	Busy,
	Unavailable,
	Failed
};

enum class Edemo_mapShanmenDivineSenseHUDFeedbackTone : uint8
{
	Invalid,
	Warning,
	Error
};

/**
 * Pure player-facing summary of one rejected Divine Sense input result.
 *
 * The product result remains authoritative. This projection only converts its
 * typed status and frozen availability into concise HUD copy. Successful uses
 * deliberately produce no feedback because the reveal panel already confirms
 * them. The projection owns no World, timer, input or resource state.
 */
class Fdemo_mapShanmenDivineSenseHUDFeedbackPresentation
{
public:
	static bool TryProject(
		const Fdemo_mapShanmenDivineSenseLogicalInputResult& Result,
		const FString& UseKeyLabel,
		Fdemo_mapShanmenDivineSenseHUDFeedbackPresentation&
			OutPresentation);

	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenDivineSenseHUDFeedbackPresentation& Other)
		const;
	Edemo_mapShanmenDivineSenseHUDFeedbackReason GetReason() const
	{
		return Reason;
	}
	Edemo_mapShanmenDivineSenseHUDFeedbackTone GetTone() const
	{
		return Tone;
	}
	const FString& GetDisplayText() const { return DisplayText; }

private:
	Edemo_mapShanmenDivineSenseHUDFeedbackReason Reason =
		Edemo_mapShanmenDivineSenseHUDFeedbackReason::Invalid;
	Edemo_mapShanmenDivineSenseHUDFeedbackTone Tone =
		Edemo_mapShanmenDivineSenseHUDFeedbackTone::Invalid;
	FString DisplayText;
};
