#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenFormationAreaProvider.h"

enum class Edemo_mapShanmenFormationCoverageTransitionKind : uint8
{
	Entered,
	StayedCovered,
	Left,
	RemainedOutside
};

enum class Edemo_mapShanmenFormationCoverageTransitionStatus : uint8
{
	Reduced,
	PreviousInvalid,
	CurrentInvalid,
	AreaMismatch,
	SubjectSetMismatch,
	ReceiptRejected
};

/**
 * Deterministic fact joining one subject's previous and current membership.
 *
 * The parent batch carries the complete immutable coverage receipts and checks
 * that these opaque membership identities refer to the matching subject.
 */
struct Fdemo_mapShanmenFormationMembershipTransitionFact
{
	FGuid FactId;
	FGuid AreaId;
	FGuid SubjectEntityId;
	FGuid PreviousMembershipReceiptId;
	FGuid CurrentMembershipReceiptId;
	Edemo_mapShanmenFormationCoverageTransitionKind Kind =
		Edemo_mapShanmenFormationCoverageTransitionKind::RemainedOutside;

	bool IsValid() const;
};

/**
 * Self-contained, canonically ordered transition evidence for two coverage
 * snapshots over the exact same Area and subject set.
 */
struct Fdemo_mapShanmenFormationCoverageTransitionReceipt
{
	FGuid ReceiptId;
	Fdemo_mapShanmenFormationCoverageReceipt PreviousCoverage;
	Fdemo_mapShanmenFormationCoverageReceipt CurrentCoverage;
	TArray<Fdemo_mapShanmenFormationMembershipTransitionFact> Facts;
	int32 EnteredCount = 0;
	int32 StayedCoveredCount = 0;
	int32 LeftCount = 0;
	int32 RemainedOutsideCount = 0;

	bool IsValid() const;
	int32 GetChangedCount() const { return EnteredCount + LeftCount; }
};

struct Fdemo_mapShanmenFormationCoverageTransitionResult
{
	Edemo_mapShanmenFormationCoverageTransitionStatus Status =
		Edemo_mapShanmenFormationCoverageTransitionStatus::PreviousInvalid;
	FString Diagnostic;
	Fdemo_mapShanmenFormationCoverageTransitionReceipt Receipt;

	bool IsSuccess() const;
};

/**
 * Pure reducer from two immutable coverage receipts to transition facts.
 *
 * It owns no baseline, cadence, Actor, World, registry, effect, or lifecycle
 * policy. Missing subjects and Area replacement remain explicit caller events.
 */
class Fdemo_mapShanmenFormationCoverageTransitionReducer
{
public:
	static Fdemo_mapShanmenFormationCoverageTransitionResult Reduce(
		const Fdemo_mapShanmenFormationCoverageReceipt& Previous,
		const Fdemo_mapShanmenFormationCoverageReceipt& Current);
};
