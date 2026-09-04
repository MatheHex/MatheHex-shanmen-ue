#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenThrownWeaponInputChoiceIntentAdapter.h"

/** Device/UI-neutral operations currently meaningful for the visible choice. */
enum class Edemo_mapShanmenThrownWeaponInputChoiceInteractionCapability : uint8
{
	None = 0,
	SelectStraightTrajectory = 1 << 0,
	SelectBallisticArcTrajectory = 1 << 1,
	SetArcTargetIntent = 1 << 2,
	IncreaseArcApex = 1 << 3,
	DecreaseArcApex = 1 << 4,
	ClearArcTargetIntent = 1 << 5
};
ENUM_CLASS_FLAGS(
	Edemo_mapShanmenThrownWeaponInputChoiceInteractionCapability)

/**
 * Immutable, revisionless projection of the sole P20.11 choice state.
 *
 * It exposes only canonical values and currently meaningful edit operations.
 * Equal visible choices reproduce one identity regardless of state revision.
 */
class Fdemo_mapShanmenThrownWeaponInputChoiceInteractionReadModel
{
public:
	static bool TryProject(
		const Fdemo_mapShanmenThrownWeaponInputChoiceState& State,
		Fdemo_mapShanmenThrownWeaponInputChoiceInteractionReadModel& OutModel);

	bool IsValid() const;
	bool Matches(
		const Fdemo_mapShanmenThrownWeaponInputChoiceInteractionReadModel&
			Other) const;
	bool HasCapability(
		Edemo_mapShanmenThrownWeaponInputChoiceInteractionCapability
			Capability) const;
	const FGuid& GetReadModelId() const { return ReadModelId; }
	Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind
	GetTrajectoryKind() const { return TrajectoryKind; }
	bool HasArcTargetIntent() const { return bHasArcTargetIntent; }
	const FVector2D& GetArcTargetIntent() const { return ArcTargetIntent; }
	double GetArcApexAdjustment() const { return ArcApexAdjustment; }
	Edemo_mapShanmenThrownWeaponInputChoiceInteractionCapability
	GetCapabilities() const { return Capabilities; }

private:
	FGuid ReadModelId;
	Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind TrajectoryKind =
		Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::Invalid;
	bool bHasArcTargetIntent = false;
	FVector2D ArcTargetIntent = FVector2D::ZeroVector;
	double ArcApexAdjustment = 0.0;
	Edemo_mapShanmenThrownWeaponInputChoiceInteractionCapability Capabilities =
		Edemo_mapShanmenThrownWeaponInputChoiceInteractionCapability::None;
};

enum class Edemo_mapShanmenThrownWeaponInputChoiceInteractionReadStatus : uint8
{
	Invalid,
	ChoiceSourceUnavailable,
	ChoiceStateInvalid,
	Projected,
	ProjectionRejected
};

/** Immutable evidence for one authoritative interaction projection read. */
class Fdemo_mapShanmenThrownWeaponInputChoiceInteractionReadResult
{
public:
	bool IsValid() const;
	bool IsProjected() const;
	Edemo_mapShanmenThrownWeaponInputChoiceInteractionReadStatus
	GetStatus() const { return Status; }
	const FString& GetDiagnostic() const { return Diagnostic; }
	int32 GetChoiceSourceResolutionCount() const
	{
		return ChoiceSourceResolutionCount;
	}
	int32 GetChoiceStateReadCount() const { return ChoiceStateReadCount; }
	int32 GetProjectionCount() const { return ProjectionCount; }
	const Fdemo_mapShanmenThrownWeaponInputChoiceInteractionReadModel&
	GetReadModel() const { return ReadModel; }

private:
	friend struct Fdemo_mapShanmenThrownWeaponInputChoiceInteractionPort;

	Edemo_mapShanmenThrownWeaponInputChoiceInteractionReadStatus Status =
		Edemo_mapShanmenThrownWeaponInputChoiceInteractionReadStatus::Invalid;
	FString Diagnostic;
	int32 ChoiceSourceResolutionCount = 0;
	int32 ChoiceStateReadCount = 0;
	int32 ProjectionCount = 0;
	Fdemo_mapShanmenThrownWeaponInputChoiceInteractionReadModel ReadModel;
};

/**
 * Stateless interaction seam above the P20.19 intent contract.
 *
 * Read resolves the authoritative source once and projects once. Emit helpers
 * consume only a valid read model and create existing revisionless intents.
 * The port owns no UI, key, World, Actor, revision, session, route, or retry.
 */
struct Fdemo_mapShanmenThrownWeaponInputChoiceInteractionPort
{
	using FResolveChoiceSource = TFunctionRef<bool()>;
	using FReadChoiceState = TFunctionRef<
		Fdemo_mapShanmenThrownWeaponInputChoiceState()>;

	static Fdemo_mapShanmenThrownWeaponInputChoiceInteractionReadResult Read(
		FResolveChoiceSource ResolveChoiceSource,
		FReadChoiceState ReadChoiceState);
	static bool TryEmitTrajectorySelection(
		const Fdemo_mapShanmenThrownWeaponInputChoiceInteractionReadModel&
			ReadModel,
		Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind TrajectoryKind,
		Fdemo_mapShanmenThrownWeaponInputChoiceIntent& OutIntent);
	static bool TryEmitArcTargetIntent(
		const Fdemo_mapShanmenThrownWeaponInputChoiceInteractionReadModel&
			ReadModel,
		const FVector2D& RawTargetIntent,
		Fdemo_mapShanmenThrownWeaponInputChoiceIntent& OutIntent);
	static bool TryEmitArcApexAdjustment(
		const Fdemo_mapShanmenThrownWeaponInputChoiceInteractionReadModel&
			ReadModel,
		double RawNormalizedDelta,
		Fdemo_mapShanmenThrownWeaponInputChoiceIntent& OutIntent);
	static bool TryEmitArcTargetClear(
		const Fdemo_mapShanmenThrownWeaponInputChoiceInteractionReadModel&
			ReadModel,
		Fdemo_mapShanmenThrownWeaponInputChoiceIntent& OutIntent);
};
