#pragma once

#include "CoreMinimal.h"
#if !UE_BUILD_SHIPPING
#include "demo_mapAutomationRootBoundary.h"
#endif
#include "demo_mapProfileSessionTypes.h"

class UGameInstance;
class Udemo_mapItemSubsystem;
class Udemo_mapProfilePreparationWidget;
class Udemo_mapProfileSessionSubsystem;

enum class Edemo_mapProfilePreparationFlowPhase : uint8
{
	Disabled,
	Preparation,
	RunActive,
	SettlementPending,
	RecoveryRequired
};

/**
 * Non-authoritative, opt-in flow adapter. It retains only weak authority references and
 * transient lifecycle identity; Profile, Runtime items, settlement evidence and disk state
 * remain exclusively owned by the existing Session/Repository/ItemSubsystem stack.
 */
class Fdemo_mapProfilePreparationFlow
{
public:
	Fdemo_mapProfileSessionInitializeResult InitializeProduction(UGameInstance* GameInstance);
#if !UE_BUILD_SHIPPING
	static FString AllowedAutomationRoot();
	static bool ValidateInjectedStorageRoot(
		const FString& RequestedRoot,
		FString& OutCanonicalRoot,
		FString& OutDiagnostic);
	static bool IsRootKindAcceptedForAutomation(bool bDirectoryExists, bool bIsReparseOrSymlink);
	static TArray<FString> ProtectedProductionRootsForAutomation(
		const FString& CanonicalProjectRoot);
	static Fdemo_mapAutomationRootBoundaryResult ValidateExplicitAutomationBoundary(
		const FString& AutomationRoot,
		const FString& StorageRoot,
		const FString& UserConfigRoot,
		const FString& UserDirRoot,
		const FString& GeneratedConfigRoot,
		const TArray<FString>& EvidencePaths);

	Fdemo_mapProfileSessionInitializeResult InitializeExplicit(
		UGameInstance* GameInstance,
		const FString& RequestedRoot);
#endif
	Fdemo_mapProfileSessionBeginResult StartPreparedRunThroughWidget(
		Udemo_mapProfilePreparationWidget* Widget);
	/** Product-path Start Run. The Profile transaction is UI-independent. */
	Fdemo_mapProfileSessionBeginResult StartPreparedRunDirect();
	Fdemo_mapProfileSessionSettlementResult CommitRuntimeSettlement(
		const Fdemo_mapSettlementSummary& Summary);
	Fdemo_mapProfileSessionSettlementResult RetryPendingSettlement();
	/** Technical activation rollback. This must not produce a player Abandon record. */
	Fdemo_mapProfileSessionSettlementResult CancelActiveRunForActivationFailure();
	void Unbind();

	Edemo_mapProfilePreparationFlowPhase GetPhase() const { return Phase; }
	Udemo_mapProfileSessionSubsystem* GetSession() const { return Session.Get(); }
	Udemo_mapItemSubsystem* GetRuntime() const { return Runtime.Get(); }
	const FString& GetStorageRoot() const { return StorageRoot; }
	FGuid GetProfileId() const { return ProfileId; }
	FGuid GetStartedRunId() const { return StartedRunId; }
	FGuid GetPreviousRunId() const { return PreviousRunId; }
	/** Read-only startup context; Code A already owns the recovered-abandon decision. */
	FGuid GetRecoveredAbandonRunId() const { return RecoveredAbandonRunId; }
	FGuid GetPreviousSettlementId() const { return PreviousSettlementId; }
	int32 GetSettlementSubmitCount() const { return SettlementSubmitCount; }
	int32 GetSettlementRetryCount() const { return SettlementRetryCount; }

private:
	Fdemo_mapProfileSessionInitializeResult InitializeWithStorage(
		UGameInstance* GameInstance,
		const Fdemo_mapProfileStorageContext& Storage);
	Fdemo_mapProfileSessionSettlementResult RejectSettlement(const FString& Diagnostic) const;
	void ApplySettlementResult(const Fdemo_mapProfileSessionSettlementResult& Result);

	TWeakObjectPtr<Udemo_mapProfileSessionSubsystem> Session;
	TWeakObjectPtr<Udemo_mapItemSubsystem> Runtime;
	FString StorageRoot;
	FGuid ProfileId;
	FGuid StartedRunId;
	FGuid PreviousRunId;
	FGuid RecoveredAbandonRunId;
	FGuid PreviousSettlementId;
	Edemo_mapProfilePreparationFlowPhase Phase = Edemo_mapProfilePreparationFlowPhase::Disabled;
	int32 SettlementSubmitCount = 0;
	int32 SettlementRetryCount = 0;
};
