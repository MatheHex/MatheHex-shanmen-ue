#pragma once

#include "CoreMinimal.h"
#include "demo_mapShanmenFormationScatterWorldPlacementHandoff.h"
#include "demo_mapShanmenFormationWorldAdapter.h"

class AActor;
class UWorld;

struct Fdemo_mapShanmenFormationScatterWorldPublisher;

enum class Edemo_mapShanmenFormationScatterWorldPublicationStatus : uint8
{
	Invalid,
	Published,
	Recovered,
	Replayed,
	HandoffEvidenceInvalid,
	PortInvalid,
	LedgerConflict,
	PublicationRejected,
	ReceiptInvalid,
	CompletionEvidenceInvalid
};

/** One successful canonical P27.22 handoff recorded after World acceptance. */
class Fdemo_mapShanmenFormationScatterAnchorWorldPublication
{
public:
	bool IsValid() const;
	const FGuid& GetRecordId() const { return RecordId; }
	const FGuid& GetHandoffEvidenceId() const
	{
		return HandoffEvidenceId;
	}
	const FGuid& GetSourceHandoffId() const { return SourceHandoffId; }
	const FGuid& GetDeploymentHandoffId() const
	{
		return DeploymentHandoffId;
	}
	int32 GetAnchorOrder() const { return AnchorOrder; }
	const Fdemo_mapShanmenFormationAnchorPlacementReceipt& GetReceipt() const
	{
		return Receipt;
	}

	bool operator==(
		const Fdemo_mapShanmenFormationScatterAnchorWorldPublication& Other)
		const;

private:
	friend struct Fdemo_mapShanmenFormationScatterWorldPublisher;

	static FGuid BuildRecordId(
		const Fdemo_mapShanmenFormationScatterAnchorWorldPublication& Record);

	FGuid RecordId;
	FGuid HandoffEvidenceId;
	FGuid SourceHandoffId;
	FGuid DeploymentHandoffId;
	int32 AnchorOrder = INDEX_NONE;
	Fdemo_mapShanmenFormationAnchorPlacementReceipt Receipt;
};

/**
 * Forward-only publication ledger. A non-empty ledger is bound to one exact
 * P27.22 evidence set and Actor class, and can contain only its canonical
 * successful prefix.
 */
class Fdemo_mapShanmenFormationScatterWorldPublicationLedger
{
public:
	bool IsValid() const;
	bool IsCompatibleWith(
		const Fdemo_mapShanmenFormationScatterWorldPlacementHandoffEvidence&
			HandoffEvidence,
		const FString& ActorClassPath) const;
	bool IsEmpty() const { return Records.IsEmpty(); }
	const FGuid& GetLedgerId() const { return LedgerId; }
	const FGuid& GetHandoffEvidenceId() const
	{
		return HandoffEvidenceId;
	}
	const FGuid& GetDeploymentId() const { return DeploymentId; }
	const FString& GetActorClassPath() const { return ActorClassPath; }
	const TArray<Fdemo_mapShanmenFormationScatterAnchorWorldPublication>&
	GetRecords() const
	{
		return Records;
	}
	int32 GetPublishedCount() const { return Records.Num(); }

	bool operator==(
		const Fdemo_mapShanmenFormationScatterWorldPublicationLedger& Other)
		const;

private:
	friend struct Fdemo_mapShanmenFormationScatterWorldPublisher;

	static FGuid BuildLedgerId(
		const Fdemo_mapShanmenFormationScatterWorldPlacementHandoffEvidence&
			HandoffEvidence,
		const FString& ActorClassPath);

	FGuid LedgerId;
	FGuid HandoffEvidenceId;
	FGuid DeploymentId;
	FString ActorClassPath;
	TArray<Fdemo_mapShanmenFormationScatterAnchorWorldPublication> Records;
};

/** Immutable proof that the entire P27.22 batch has World receipts. */
class Fdemo_mapShanmenFormationScatterWorldPublicationEvidence
{
public:
	bool IsValid() const;
	const FGuid& GetEvidenceId() const { return EvidenceId; }
	const Fdemo_mapShanmenFormationScatterWorldPlacementHandoffEvidence&
	GetHandoffEvidence() const
	{
		return HandoffEvidence;
	}
	const Fdemo_mapShanmenFormationScatterWorldPublicationLedger& GetLedger()
		const
	{
		return Ledger;
	}

	bool operator==(
		const Fdemo_mapShanmenFormationScatterWorldPublicationEvidence& Other)
		const;

private:
	friend struct Fdemo_mapShanmenFormationScatterWorldPublisher;

	static FGuid BuildEvidenceId(
		const Fdemo_mapShanmenFormationScatterWorldPublicationEvidence&
			Evidence);

	FGuid EvidenceId;
	Fdemo_mapShanmenFormationScatterWorldPlacementHandoffEvidence
		HandoffEvidence;
	Fdemo_mapShanmenFormationScatterWorldPublicationLedger Ledger;
};

struct Fdemo_mapShanmenFormationScatterWorldPublicationResult
{
	Edemo_mapShanmenFormationScatterWorldPublicationStatus Status =
		Edemo_mapShanmenFormationScatterWorldPublicationStatus::Invalid;
	FString Diagnostic;
	int32 InitialPublishedCount = 0;
	int32 FinalPublishedCount = 0;
	int32 FailedAnchorOrder = INDEX_NONE;
	TArray<Fdemo_mapShanmenFormationScatterAnchorWorldPublication> NewRecords;
	Fdemo_mapShanmenFormationScatterWorldPublicationEvidence Evidence;

	bool IsValid() const;
	bool IsSuccess() const;
};

/** Narrow injectable World boundary used by the forward-only publisher. */
class Idemo_mapShanmenFormationScatterWorldPlacementPort
{
public:
	virtual ~Idemo_mapShanmenFormationScatterWorldPlacementPort() = default;
	virtual FString GetActorClassPath() const = 0;
	virtual Fdemo_mapShanmenFormationWorldResult Publish(
		const Fdemo_mapShanmenFormationAnchorPlacementIntent& Intent) = 0;
};

/** Concrete port that delegates each immutable intent to the existing adapter. */
class Fdemo_mapShanmenFormationScatterWorldAdapterPublicationPort final
	: public Idemo_mapShanmenFormationScatterWorldPlacementPort
{
public:
	Fdemo_mapShanmenFormationScatterWorldAdapterPublicationPort(
		UWorld* InWorld,
		TSubclassOf<AActor> InActorClass,
		Fdemo_mapShanmenFormationWorldAdapter& InAdapter);

	virtual FString GetActorClassPath() const override;
	virtual Fdemo_mapShanmenFormationWorldResult Publish(
		const Fdemo_mapShanmenFormationAnchorPlacementIntent& Intent) override;

private:
	UWorld* World = nullptr;
	TSubclassOf<AActor> ActorClass;
	Fdemo_mapShanmenFormationWorldAdapter& Adapter;
};

/**
 * Sequentially publishes one P27.22 batch. Successful prefixes are retained;
 * retries verify that prefix through the port and continue only at the first
 * missing anchor.
 */
struct Fdemo_mapShanmenFormationScatterWorldPublisher
{
	static Fdemo_mapShanmenFormationScatterWorldPublicationResult Publish(
		const Fdemo_mapShanmenFormationScatterWorldPlacementHandoffEvidence&
			HandoffEvidence,
		Idemo_mapShanmenFormationScatterWorldPlacementPort& Port,
		Fdemo_mapShanmenFormationScatterWorldPublicationLedger& Ledger);

	static bool BuildRecord(
		const Fdemo_mapShanmenFormationScatterWorldPlacementHandoffEvidence&
			HandoffEvidence,
		int32 AnchorIndex,
		const Fdemo_mapShanmenFormationAnchorPlacementReceipt& Receipt,
		Fdemo_mapShanmenFormationScatterAnchorWorldPublication& OutRecord);
};
