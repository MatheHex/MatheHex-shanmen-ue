#pragma once

#include "CoreMinimal.h"
#include "ShanmenCoreTypes.h"
#include "ShanmenFormationMastery.h"

/** Mutable transport returned by the existing progression/save authority. */
struct Fdemo_mapShanmenFormationMasteryAuthorityCapture
{
	FGuid OwnerId;
	int64 AuthorityRevision = INDEX_NONE;
	FShanmenContentStamp Content;
	EShanmenFormationMasteryTier MasteryTier =
		EShanmenFormationMasteryTier::Invalid;
};

/** Immutable identity-bound evidence from one mastery authority read. */
class Fdemo_mapShanmenFormationMasteryAuthorityRead
{
public:
	static bool TryCapture(
		const Fdemo_mapShanmenFormationMasteryAuthorityCapture& Capture,
		Fdemo_mapShanmenFormationMasteryAuthorityRead& OutRead,
		FString& OutDiagnostic);

	bool IsValid() const;
	const FGuid& GetReadId() const { return ReadId; }
	const FGuid& GetOwnerId() const { return OwnerId; }
	int64 GetAuthorityRevision() const { return AuthorityRevision; }
	const FShanmenContentStamp& GetContent() const { return Content; }
	const FShanmenFormationMasteryPolicy& GetMasteryPolicy() const
	{
		return MasteryPolicy;
	}

private:
	static FGuid BuildReadId(
		const FGuid& OwnerId,
		int64 AuthorityRevision,
		const FShanmenContentStamp& Content,
		EShanmenFormationMasteryTier MasteryTier);

	FGuid ReadId;
	FGuid OwnerId;
	int64 AuthorityRevision = INDEX_NONE;
	FShanmenContentStamp Content;
	FShanmenFormationMasteryPolicy MasteryPolicy;
};

enum class Edemo_mapShanmenFormationMasteryProjectionStatus : uint8
{
	Invalid,
	Projected,
	ContentInvalid,
	OwnerInvalid,
	AuthorityUnavailable,
	AuthorityReadInvalid,
	OwnerMismatch,
	ContentMismatch
};

/** Auditable result of one bounded mastery authority read and projection. */
struct Fdemo_mapShanmenFormationMasteryProjectionResult
{
	Edemo_mapShanmenFormationMasteryProjectionStatus Status =
		Edemo_mapShanmenFormationMasteryProjectionStatus::Invalid;
	FString Diagnostic;
	FShanmenContentStamp ExpectedContent;
	FGuid RequestedOwnerId;
	int32 AuthorityReadCount = 0;
	Fdemo_mapShanmenFormationMasteryAuthorityRead AuthorityRead;

	bool IsValid() const;
	bool IsProjected() const;
};

/**
 * Single-direction, read-only projection from the existing progression/save
 * authority into P27.14's pure Formation Mastery capability policy.
 *
 * It owns no profile schema, mastery mutation, progression rule, retry,
 * inventory, UI, input, World, Actor, diagram or deployment lifecycle.
 */
struct Fdemo_mapShanmenFormationMasteryAuthorityAdapter
{
	using FReadAuthority = TFunctionRef<bool(
		const FGuid& RequestedOwnerId,
		Fdemo_mapShanmenFormationMasteryAuthorityCapture& OutCapture,
		FString& OutDiagnostic)>;

	static Fdemo_mapShanmenFormationMasteryProjectionResult Project(
		const FShanmenContentStamp& ExpectedContent,
		const FGuid& RequestedOwnerId,
		FReadAuthority ReadAuthority);
};
