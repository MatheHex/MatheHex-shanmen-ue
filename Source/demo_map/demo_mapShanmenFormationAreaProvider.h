#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenFormationWorldAdapter.h"

enum class Edemo_mapShanmenFormationAreaBuildStatus : uint8
{
	Built,
	EmptyReceiptSet,
	InsufficientAnchors,
	ReceiptInvalid,
	ScopeMismatch,
	DuplicateIdentity,
	DuplicatePoint,
	DegenerateGeometry
};

enum class Edemo_mapShanmenFormationAreaRelation : uint8
{
	Outside,
	Boundary,
	Inside
};

enum class Edemo_mapShanmenFormationMembershipStatus : uint8
{
	Evaluated,
	AreaInvalid,
	QueryInvalid
};

enum class Edemo_mapShanmenFormationCoverageStatus : uint8
{
	Evaluated,
	AreaInvalid,
	QuerySetEmpty,
	QueryInvalid,
	DuplicateSubject
};

/** Frozen anchor evidence copied from one valid P8.3 placement receipt. */
struct Fdemo_mapShanmenFormationAreaAnchor
{
	FGuid PlacementReceiptId;
	FGuid PlacementId;
	FName AnchorDefinitionId = NAME_None;
	FGuid AnchorInstanceId;
	FVector WorldLocation = FVector::ZeroVector;

	bool IsValid() const;
};

/**
 * Immutable horizontal convex area derived from one deployment's placed anchors.
 *
 * Anchors contains every source placement in canonical geometric order, while
 * BoundaryAnchorIndices selects the counter-clockwise convex hull. Z remains in
 * identity/evidence, but membership intentionally evaluates the XY ground plane.
 */
struct Fdemo_mapShanmenFormationAreaSnapshot
{
	FGuid AreaId;
	FGuid RunId;
	FGuid OwnerId;
	FGuid DeploymentId;
	FShanmenContentStamp Content;
	TArray<Fdemo_mapShanmenFormationAreaAnchor> Anchors;
	TArray<int32> BoundaryAnchorIndices;

	bool IsValid() const;
};

struct Fdemo_mapShanmenFormationAreaBuildResult
{
	Edemo_mapShanmenFormationAreaBuildStatus Status =
		Edemo_mapShanmenFormationAreaBuildStatus::EmptyReceiptSet;
	FString Diagnostic;
	Fdemo_mapShanmenFormationAreaSnapshot Area;

	bool IsSuccess() const;
};

/** One entity/location query; no Actor or registry pointer crosses this seam. */
struct Fdemo_mapShanmenFormationAreaQuery
{
	FGuid SubjectEntityId;
	FVector WorldLocation = FVector::ZeroVector;

	bool IsValid() const;
};

/** Deterministic evidence for one exact query against one exact area identity. */
struct Fdemo_mapShanmenFormationMembershipReceipt
{
	FGuid ReceiptId;
	FGuid AreaId;
	FGuid SubjectEntityId;
	FVector WorldLocation = FVector::ZeroVector;
	Edemo_mapShanmenFormationAreaRelation Relation =
		Edemo_mapShanmenFormationAreaRelation::Outside;

	bool IsValid() const;
	bool IsCovered() const
	{
		return Relation != Edemo_mapShanmenFormationAreaRelation::Outside;
	}
};

struct Fdemo_mapShanmenFormationMembershipResult
{
	Edemo_mapShanmenFormationMembershipStatus Status =
		Edemo_mapShanmenFormationMembershipStatus::AreaInvalid;
	FString Diagnostic;
	Fdemo_mapShanmenFormationMembershipReceipt Receipt;

	bool IsSuccess() const;
};

/** Canonically ordered batch evidence; Boundary remains distinct from Inside. */
struct Fdemo_mapShanmenFormationCoverageReceipt
{
	FGuid ReceiptId;
	FGuid AreaId;
	TArray<Fdemo_mapShanmenFormationMembershipReceipt> Memberships;
	int32 InsideCount = 0;
	int32 BoundaryCount = 0;
	int32 OutsideCount = 0;

	bool IsValid() const;
	int32 GetCoveredCount() const { return InsideCount + BoundaryCount; }
};

struct Fdemo_mapShanmenFormationCoverageResult
{
	Edemo_mapShanmenFormationCoverageStatus Status =
		Edemo_mapShanmenFormationCoverageStatus::AreaInvalid;
	FString Diagnostic;
	Fdemo_mapShanmenFormationCoverageReceipt Receipt;

	bool IsSuccess() const;
};

/**
 * Pure formation-area rules. It reads immutable World-placement receipts and
 * emits geometry/membership evidence only; it never applies damage, buffs,
 * cadence, content values, inventory writes, Actor state, or UI state.
 */
class Fdemo_mapShanmenFormationAreaProvider
{
public:
	static Fdemo_mapShanmenFormationAreaBuildResult BuildArea(
		const TArray<Fdemo_mapShanmenFormationAnchorPlacementReceipt>&
			PlacementReceipts);
	static Fdemo_mapShanmenFormationMembershipResult EvaluateMembership(
		const Fdemo_mapShanmenFormationAreaSnapshot& Area,
		const Fdemo_mapShanmenFormationAreaQuery& Query);
	static Fdemo_mapShanmenFormationCoverageResult EvaluateCoverage(
		const Fdemo_mapShanmenFormationAreaSnapshot& Area,
		const TArray<Fdemo_mapShanmenFormationAreaQuery>& Queries);
};
