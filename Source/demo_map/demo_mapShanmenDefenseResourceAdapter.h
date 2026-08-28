#pragma once

#include "CoreMinimal.h"
#include "ShanmenCombatTypes.h"
#include "ShanmenItemAuthorityService.h"
#include "demo_mapShanmenRunCorrelation.h"

class Udemo_mapShanmenItemAuthoritySubsystem;

enum class Edemo_mapShanmenDefenseResourceCommitStatus : uint8
{
	Committed,
	NoCommitRequired,
	AuthorityNotReady,
	ImpactInvalid,
	RunCorrelationInvalid,
	RequestInvalid,
	AuthorityRejected
};

struct Fdemo_mapShanmenDefenseResourceCommitResult
{
	Edemo_mapShanmenDefenseResourceCommitStatus Status =
		Edemo_mapShanmenDefenseResourceCommitStatus::ImpactInvalid;
	FString Diagnostic;
	FShanmenItemRunResourceCommitRequest Request;
	FShanmenItemDurableCommandResult AuthorityCommand;

	bool IsSuccess() const
	{
		return Status
			== Edemo_mapShanmenDefenseResourceCommitStatus::NoCommitRequired
			|| (Status
					== Edemo_mapShanmenDefenseResourceCommitStatus::Committed
				&& AuthorityCommand.IsCommandSuccess());
	}
};

/**
 * Commit-point bridge for CombatCore defense receipts.
 *
 * A resource-backed defense layer uses its LayerId as the pending ShanmenItems
 * ReservationId and SourceInstanceId as the exact item identity. The pure
 * resolver remains item-agnostic; this adapter turns all triggered lines from
 * one accepted impact into one atomic, durable ActiveRun command.
 */
struct Fdemo_mapShanmenDefenseResourceAdapter
{
	static bool BuildCommitRequest(
		const FShanmenImpactResult& Impact,
		const Fdemo_mapShanmenRunCorrelation& Correlation,
		const FShanmenContentStamp& Content,
		FShanmenItemRunResourceCommitRequest& OutRequest,
		FString& OutDiagnostic);

	static Fdemo_mapShanmenDefenseResourceCommitResult CommitTriggered(
		Udemo_mapShanmenItemAuthoritySubsystem& Authority,
		const FShanmenImpactResult& Impact);
};
