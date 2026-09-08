#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenControlledWeaponFlightReadModel.h"

enum class Edemo_mapShanmenControlledWeaponThreatReadoutPlacement : uint8
{
	Invalid,
	WorldTracked,
	ViewportFallback
};

/**
 * Configurable visual constants for the P21.6 flying-sword threat readout.
 *
 * This is presentation-only data. It owns no Canvas, World, Actor, threat
 * sampling, combat, AI, inventory or mutable product authority.
 */
struct Fdemo_mapShanmenControlledWeaponThreatReadoutStyle
{
	FVector2D PanelSize = FVector2D(420.0, 30.0);
	FVector2D TextInset = FVector2D(10.0, 6.0);
	double WorldVerticalLift = 46.0;
	double HorizontalMargin = 8.0;
	double TopMargin = 54.0;
	double BottomMargin = 92.0;
	double TextScale = 0.72;
	FLinearColor PanelColor = FLinearColor(0.02f, 0.18f, 0.14f, 0.88f);
	FLinearColor TextColor = FLinearColor(0.2f, 1.0f, 0.72f);

	bool IsValid() const;
};

/** Pure, immutable result consumed by Ademo_mapHUD. */
class Fdemo_mapShanmenControlledWeaponThreatReadoutPlan
{
public:
	static bool TryPlan(
		const FVector2D& CanvasSize,
		Edemo_mapShanmenControlledWeaponFlightPhase Phase,
		int32 ContactCount,
		const FString& LaunchRecallKeyLabel,
		const FString& RedirectKeyLabel,
		bool bProjectionSucceeded,
		const FVector2D& ProjectedScreenPosition,
		const Fdemo_mapShanmenControlledWeaponThreatReadoutStyle& Style,
		Fdemo_mapShanmenControlledWeaponThreatReadoutPlan& OutPlan);

	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenControlledWeaponThreatReadoutPlan& Other)
		const;
	bool IsViewportFallback() const
	{
		return IsValid()
			&& Placement
				== Edemo_mapShanmenControlledWeaponThreatReadoutPlacement::
					ViewportFallback;
	}

	Edemo_mapShanmenControlledWeaponThreatReadoutPlacement GetPlacement()
		const
	{
		return Placement;
	}
	int32 GetContactCount() const { return ContactCount; }
	Edemo_mapShanmenControlledWeaponFlightPhase GetPhase() const
	{
		return Phase;
	}
	const FString& GetText() const { return Text; }
	const FString& GetLaunchRecallKeyLabel() const
	{
		return LaunchRecallKeyLabel;
	}
	const FString& GetRedirectKeyLabel() const
	{
		return RedirectKeyLabel;
	}
	const FVector2D& GetPanelPosition() const { return PanelPosition; }
	const FVector2D& GetPanelSize() const { return Style.PanelSize; }
	FVector2D GetTextPosition() const
	{
		return PanelPosition + Style.TextInset;
	}
	double GetTextScale() const { return Style.TextScale; }
	const FLinearColor& GetPanelColor() const { return Style.PanelColor; }
	const FLinearColor& GetTextColor() const { return Style.TextColor; }

private:
	Edemo_mapShanmenControlledWeaponThreatReadoutPlacement Placement =
		Edemo_mapShanmenControlledWeaponThreatReadoutPlacement::Invalid;
	FVector2D CanvasSize = FVector2D::ZeroVector;
	FVector2D PanelPosition = FVector2D::ZeroVector;
	Edemo_mapShanmenControlledWeaponFlightPhase Phase =
		Edemo_mapShanmenControlledWeaponFlightPhase::Invalid;
	int32 ContactCount = 0;
	FString LaunchRecallKeyLabel;
	FString RedirectKeyLabel;
	FString Text;
	Fdemo_mapShanmenControlledWeaponThreatReadoutStyle Style;
};
