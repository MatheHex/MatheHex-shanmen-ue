#pragma once

#include "CoreMinimal.h"
#include "ShanmenCombatTypes.h"
#include "ShanmenVitalityAuthority.h"
#include "ShanmenItemAuthorityService.h"
#include "demo_mapShanmenRunCorrelation.h"

class Udemo_mapShanmenItemAuthoritySubsystem;
class Udemo_mapPlayerHealthComponent;

enum class Edemo_mapShanmenDefenseResourcePreparationStatus : uint8
{
	Prepared,
	Replayed,
	NotApplicable,
	ResourceUnavailable,
	AuthorityNotReady,
	PriorRecoveryRejected,
	RunCorrelationInvalid,
	SnapshotInvalid,
	DefenseMismatch,
	ReservationRejected,
	ReservationInvalid
};

/** Durable reservation and rewritten defense snapshot for one incoming impact. */
struct Fdemo_mapShanmenDefenseResourcePreparationResult
{
	Edemo_mapShanmenDefenseResourcePreparationStatus Status =
		Edemo_mapShanmenDefenseResourcePreparationStatus::SnapshotInvalid;
	FString Diagnostic;
	FShanmenItemReserveRequest ReserveRequest;
	FShanmenItemDurableCommandResult ReserveCommand;
	FGuid ReservationId;

	bool IsSuccess() const
	{
		return Status
			== Edemo_mapShanmenDefenseResourcePreparationStatus::Prepared
			|| Status
				== Edemo_mapShanmenDefenseResourcePreparationStatus::Replayed
			|| Status
				== Edemo_mapShanmenDefenseResourcePreparationStatus::NotApplicable
			|| Status
				== Edemo_mapShanmenDefenseResourcePreparationStatus::ResourceUnavailable;
	}

	bool HasResourceLayer() const
	{
		return (Status
				== Edemo_mapShanmenDefenseResourcePreparationStatus::Prepared
			|| Status
				== Edemo_mapShanmenDefenseResourcePreparationStatus::Replayed)
			&& ReservationId.IsValid();
	}
};

/** Cleanup receipt for pre-intent P5.4 reservations left by an interruption. */
struct Fdemo_mapShanmenDefenseOrphanRecoveryResult
{
	bool bSuccess = false;
	int32 CancelledReservationCount = 0;
	FString Diagnostic;
	TArray<FShanmenItemDurableCommandResult> CancellationCommands;
};

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
	/** Cancels only recognizable P5.4 reservations that never entered an intent. */
	static Fdemo_mapShanmenDefenseOrphanRecoveryResult
	RecoverOrphanedDefenseReservations(
		Udemo_mapShanmenItemAuthoritySubsystem& Authority);

	/**
	 * Splits the equipped Spirit Guard Robe out of legacy aggregate armor,
	 * reserves one Durability, and appends the resource-backed canonical layer.
	 */
	static Fdemo_mapShanmenDefenseResourcePreparationResult
	PrepareImpactDefense(
		Udemo_mapShanmenItemAuthoritySubsystem& Authority,
		Udemo_mapPlayerHealthComponent& VitalityHost,
		const FGuid& ImpactId,
		FShanmenDefenseSnapshot& InOutDefense);

	/** Best-effort durable cancellation before an impact enters coordination. */
	static bool CancelPreparedDefenseReservation(
		Udemo_mapShanmenItemAuthoritySubsystem& Authority,
		const Fdemo_mapShanmenDefenseResourcePreparationResult& Preparation,
		FString& OutDiagnostic);

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
