#pragma once

#include "CoreMinimal.h"
#include "ShanmenCombatTypes.h"
#include "ShanmenVitalityAuthority.h"
#include "ShanmenItemAuthorityService.h"
#include "demo_mapShanmenRunCorrelation.h"

class Udemo_mapShanmenItemAuthoritySubsystem;
class Udemo_mapPlayerHealthComponent;

enum class Edemo_mapShanmenDefenseResourceCoordinationStatus : uint8
{
	Coordinated,
	Recovered,
	NoResourceIntent,
	AuthorityNotReady,
	ImpactInvalid,
	RunCorrelationInvalid,
	RequestInvalid,
	PrepareRejected,
	VitalityRejected,
	FinalizeInDoubt,
	RecoveryAmbiguous
};

/** One auditable attempt across durable Items and in-memory product vitality. */
struct Fdemo_mapShanmenDefenseResourceCoordinationResult
{
	Edemo_mapShanmenDefenseResourceCoordinationStatus Status =
		Edemo_mapShanmenDefenseResourceCoordinationStatus::ImpactInvalid;
	FString Diagnostic;
	FShanmenItemRunResourceIntentRequest PrepareRequest;
	FShanmenItemRunResourceIntentFinalizeRequest FinalizeRequest;
	FShanmenItemDurableCommandResult PrepareCommand;
	FShanmenVitalityCommitResult VitalityCommand;
	FShanmenItemDurableCommandResult FinalizeCommand;

	bool IsSuccess() const
	{
		return Status
			== Edemo_mapShanmenDefenseResourceCoordinationStatus::NoResourceIntent
			|| ((Status
					== Edemo_mapShanmenDefenseResourceCoordinationStatus::Coordinated
					|| Status
						== Edemo_mapShanmenDefenseResourceCoordinationStatus::Recovered)
				&& VitalityCommand.IsSuccess()
				&& FinalizeCommand.IsCommandSuccess());
	}
};

/** Recoverable product bridge between CombatCore, vitality, and item resources. */
struct Fdemo_mapShanmenDefenseResourceAdapter
{
	/** Builds the durable intent with triggered lines first and untriggered lines second. */
	static bool BuildIntentRequest(
		const FShanmenImpactRequest& Request,
		const FShanmenImpactResult& Impact,
		const Fdemo_mapShanmenRunCorrelation& Correlation,
		const FShanmenContentStamp& Content,
		FShanmenItemRunResourceIntentRequest& OutRequest,
		FShanmenVitalityCommitCommand& OutVitalityCommand,
		FString& OutDiagnostic);

	static bool BuildFinalizeRequest(
		const FShanmenItemRunResourceIntentRequest& PrepareRequest,
		bool bExternalCommitSucceeded,
		FShanmenItemRunResourceIntentFinalizeRequest& OutRequest);

	static bool EncodeVitalityIntent(
		const FShanmenVitalityCommitCommand& Command,
		FName& OutMetadata);
	static bool DecodeVitalityIntent(
		FName Metadata,
		FShanmenVitalityCommitCommand& OutCommand);

	/** Prepare Items, commit/recover vitality, then durably decide all resources. */
	static Fdemo_mapShanmenDefenseResourceCoordinationResult CoordinateImpact(
		Udemo_mapShanmenItemAuthoritySubsystem& Authority,
		Udemo_mapPlayerHealthComponent& VitalityHost,
		const FShanmenImpactRequest& Request,
		const FShanmenImpactResult& Impact);

	/** Completes the unique pending intent reconstructed from durable Items state. */
	static Fdemo_mapShanmenDefenseResourceCoordinationResult RecoverPendingIntent(
		Udemo_mapShanmenItemAuthoritySubsystem& Authority,
		Udemo_mapPlayerHealthComponent& VitalityHost);
};
