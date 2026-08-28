#include "demo_map0909BM01RuntimeAdapter.h"

#include "demo_map.h"
#include "demo_mapGameMode.h"
#include "demo_mapPlayerController.h"
#include "demo_mapProfileSessionTypes.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/WorldSettings.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/PackageName.h"

const TCHAR* Fdemo_map0909BM01RuntimeAdapter::ExpectedM01MapDescriptor()
{
	return TEXT("/Game/M01/Maps/L_M01_Expedition");
}

bool Fdemo_map0909BM01RuntimeAdapter::ValidateM01DescriptorConfiguration(
	FString& OutDiagnostic)
{
	FString GameDefault;
	FString EditorDefault;
	GConfig->GetString(
		TEXT("/Script/EngineSettings.GameMapsSettings"),
		TEXT("GameDefaultMap"), GameDefault, GEngineIni);
	GConfig->GetString(
		TEXT("/Script/EngineSettings.GameMapsSettings"),
		TEXT("EditorStartupMap"), EditorDefault, GEngineIni);
	const FString Expected = ExpectedM01MapDescriptor();
	const FString PackageName = FPackageName::ObjectPathToPackageName(GameDefault);
	const bool bGameDefaultMatches = GameDefault.StartsWith(Expected);
	const bool bEditorDefaultMatches = EditorDefault.StartsWith(Expected);
	const bool bPackageExists = !PackageName.IsEmpty()
		&& FPackageName::DoesPackageExist(PackageName);
	OutDiagnostic = FString::Printf(
		TEXT("Descriptor=%s GameDefault=%s EditorDefault=%s PackageExists=%d"),
		*Expected, *GameDefault, *EditorDefault, bPackageExists ? 1 : 0);
	return bGameDefaultMatches && bEditorDefaultMatches && bPackageExists;
}

void Fdemo_map0909BM01RuntimeAdapter::Initialize(
	Ademo_mapGameMode* InGameMode,
	Ademo_mapPlayerController* InController)
{
	GameMode = InGameMode;
	Controller = InController;
	ActiveAttempt.Reset();
	NextReceiptSequence = 0;
}

void Fdemo_map0909BM01RuntimeAdapter::StartReceipt(
	const FGuid& StartAttemptId,
	Fdemo_map0909BM01RuntimeReceipt& OutReceipt)
{
	OutReceipt = Fdemo_map0909BM01RuntimeReceipt();
	OutReceipt.StartAttemptId = StartAttemptId;
	OutReceipt.Sequence = ++NextReceiptSequence;
	OutReceipt.MapDescriptor = ExpectedM01MapDescriptor();
}

void Fdemo_map0909BM01RuntimeAdapter::MarkTechnicalFailure(
	Fdemo_map0909BM01RuntimeReceipt& InOutReceipt,
	const FString& FailureClass,
	const FString& Detail) const

{
	InOutReceipt.ReceiptClass = Edemo_map0909BM01RuntimeReceiptClass::TechnicalFailure;
	InOutReceipt.FailureClass = FailureClass;
	InOutReceipt.Detail = Detail;
}

bool Fdemo_map0909BM01RuntimeAdapter::CollectWorldFacts(
	const FGuid& StartAttemptId,
	const FGuid& ExpectedOwnerId,
	const FGuid& ExpectedRunId,
	const bool bRequireRestoredInput,
	Fdemo_map0909BM01RuntimeReceipt& OutReceipt) const

{
	if (!StartAttemptId.IsValid() || !ExpectedOwnerId.IsValid())
	{
		MarkTechnicalFailure(OutReceipt, TEXT("AttemptIdentityInvalid"),
			TEXT("M01 adapter received an invalid StartAttemptId or OwnerId."));
		return false;
	}
	if (!GameMode.IsValid() || !GameMode->Is0909BRuntimeReady() || !Controller.IsValid())
	{
		MarkTechnicalFailure(OutReceipt, TEXT("RuntimeDependencyMissing"),
			TEXT("M01 adapter is missing the formal GameMode runtime or PlayerController."));
		return false;
	}
	UWorld* World = GameMode->GetWorld();
	if (!World)
	{
		MarkTechnicalFailure(OutReceipt, TEXT("TargetWorldMissing"),
			TEXT("The formal M01 target World is unavailable."));
		return false;
	}
	OutReceipt.MapIdentity = World->GetMapName();
	OutReceipt.bDescriptorResolved = true;
	OutReceipt.bWorldMatched = GameMode->IsM01ExpeditionMap()
		&& OutReceipt.MapIdentity.EndsWith(TEXT("L_M01_Expedition"));
	if (!OutReceipt.bWorldMatched)
	{
		MarkTechnicalFailure(OutReceipt, TEXT("TargetWorldMismatch"),
			FString::Printf(TEXT("Expected=%s Actual=%s"),
				*OutReceipt.MapDescriptor, *OutReceipt.MapIdentity));
		return false;
	}
	OutReceipt.bGameModeReady = World->GetAuthGameMode() == GameMode.Get();
	OutReceipt.bWorldSettingsReady = World->GetWorldSettings() != nullptr;
	if (!OutReceipt.bGameModeReady || !OutReceipt.bWorldSettingsReady)
	{
		MarkTechnicalFailure(OutReceipt, TEXT("WorldAuthorityNotReady"),
			TEXT("M01 World did not expose the expected GameMode and WorldSettings."));
		return false;
	}
	OutReceipt.bControllerReady = Controller->GetWorld() == World;
	OutReceipt.bPawnReady = Controller->GetPawn() != nullptr;
	if (!OutReceipt.bControllerReady || !OutReceipt.bPawnReady)
	{
		MarkTechnicalFailure(OutReceipt, TEXT("PlayerRuntimeNotReady"),
			TEXT("M01 World has no matching PlayerController or possessed Pawn."));
		return false;
	}
	int32 PlayerStartCount = 0;
	for (TActorIterator<APlayerStart> It(World); It; ++It)
	{
		++PlayerStartCount;
	}
	if (PlayerStartCount != 1)
	{
		MarkTechnicalFailure(OutReceipt, TEXT("PlayerStartConfigurationInvalid"),
			FString::Printf(TEXT("M01 requires exactly one authored PlayerStart; found=%d."),
				PlayerStartCount));
		return false;
	}
	Fdemo_mapProfileSessionSnapshot Snapshot;
	FString SnapshotDiagnostic;
	if (!GameMode->Get0909BProfileSnapshot(Snapshot, SnapshotDiagnostic)
		|| Snapshot.ProfileId != ExpectedOwnerId)
	{
		MarkTechnicalFailure(OutReceipt, TEXT("ProfileOwnerMismatch"),
			SnapshotDiagnostic.IsEmpty()
				? TEXT("The current Profile OwnerId differs from the StartAttempt OwnerId.")
				: SnapshotDiagnostic);
		return false;
	}
	if (ExpectedRunId.IsValid() && Snapshot.ActiveRunId != ExpectedRunId)
	{
		MarkTechnicalFailure(OutReceipt, TEXT("RunIdentityMismatch"),
			TEXT("The active Code A Run no longer matches the StartAttempt runtime receipt."));
		return false;
	}
	if (bRequireRestoredInput)
	{
		OutReceipt.bInputRestored = Controller->IsGameplayInputAllowed()
			&& Controller->GetInputSurfaceState() == TEXT("Gameplay")
			&& Controller->GetInputModeState() == TEXT("GameOnly")
			&& !Controller->IsProfilePreparationInputLocked()
			&& !Controller->IsSearchContainerInputLocked()
			&& !Controller->IsInventoryInputLocked();
		if (!OutReceipt.bInputRestored)
		{
			MarkTechnicalFailure(OutReceipt, TEXT("GameplayInputNotReady"),
				TEXT("M01 PlayerController did not restore the formal Gameplay/GameOnly input surface."));
			return false;
		}
	}
	return true;
}

bool Fdemo_map0909BM01RuntimeAdapter::BeginActivation(
	const FGuid& StartAttemptId,
	Fdemo_map0909BM01RuntimeReceipt& OutReceipt)
{
	StartReceipt(StartAttemptId, OutReceipt);
	if (ActiveAttempt.IsSet())
	{
		OutReceipt = ActiveAttempt.GetValue();
		OutReceipt.Sequence = ++NextReceiptSequence;
		OutReceipt.ReceiptClass = ActiveAttempt->StartAttemptId == StartAttemptId
			? Edemo_map0909BM01RuntimeReceiptClass::DuplicateReadyIgnored
			: Edemo_map0909BM01RuntimeReceiptClass::StaleEventIgnored;
		OutReceipt.FailureClass = TEXT("AttemptAlreadyPending");
		OutReceipt.Detail = TEXT("M01 adapter retained one pending StartAttempt and ignored a duplicate request.");
		return false;
	}
	Fdemo_mapProfileSessionSnapshot PreflightSnapshot;
	FString PreflightDiagnostic;
	if (!GameMode.IsValid()
		|| !GameMode->Get0909BProfileSnapshot(
			PreflightSnapshot, PreflightDiagnostic))
	{
		MarkTechnicalFailure(
			OutReceipt, TEXT("ProfileOwnerUnavailable"),
			PreflightDiagnostic.IsEmpty()
				? TEXT("M01 activation could not resolve the authority owner.")
				: PreflightDiagnostic);
		return false;
	}
	if (!CollectWorldFacts(
			StartAttemptId, PreflightSnapshot.ProfileId,
			FGuid(), false, OutReceipt))
	{
		return false;
	}
	Fdemo_map0909BRunStartResult Begin;
	if (!GameMode->Prepare0909BRun(Begin) || !Begin.bRunActive
		|| !Begin.OwnerId.IsValid() || !Begin.RunInstanceId.IsValid()
		|| !Begin.RunCorrelation.IsValid())
	{
		MarkTechnicalFailure(OutReceipt, TEXT("CodeARunPreparationRejected"), Begin.Diagnostic);
		return false;
	}
	OutReceipt.OwnerId = Begin.OwnerId;
	OutReceipt.RunInstanceId = Begin.RunInstanceId;
	OutReceipt.RunCorrelation = Begin.RunCorrelation;
	OutReceipt.bCodeARunCreated = true;
	if (Begin.OwnerId != PreflightSnapshot.ProfileId
		|| Begin.RunCorrelation.OwnerId != Begin.OwnerId
		|| Begin.RunCorrelation.ActiveRunId != Begin.RunInstanceId)
	{
		ActiveAttempt = OutReceipt;
		MarkTechnicalFailure(OutReceipt, TEXT("PreparedRunOwnerMismatch"),
			TEXT("The prepared Run differs from its preflight owner or immutable authority correlation."));
		ActiveAttempt = OutReceipt;
		return false;
	}
	ActiveAttempt = OutReceipt;
	FString ActivationDiagnostic;
	if (!GameMode->Activate0909BM01World(ActivationDiagnostic))
	{
		MarkTechnicalFailure(OutReceipt, TEXT("WorldActivationRequestRejected"), ActivationDiagnostic);
		ActiveAttempt = OutReceipt;
		return false;
	}
	OutReceipt.Sequence = ++NextReceiptSequence;
	OutReceipt.bActivationRequestAccepted = true;
	OutReceipt.ReceiptClass = Edemo_map0909BM01RuntimeReceiptClass::ActivationRequestAccepted;
	OutReceipt.Detail = TEXT("The formal M01 activation request was accepted; awaiting the correlated RuntimeReady callback.");
	ActiveAttempt = OutReceipt;
	UE_LOG(Logdemo_map, Log,
		TEXT("0_0_9BFIX_M01_ADAPTER Event=ActivationRequestAccepted AttemptId=%s OwnerId=%s RunId=%s Descriptor=%s Map=%s"),
		*StartAttemptId.ToString(EGuidFormats::DigitsWithHyphens),
		*OutReceipt.OwnerId.ToString(EGuidFormats::DigitsWithHyphens),
		*OutReceipt.RunInstanceId.ToString(EGuidFormats::DigitsWithHyphens),
		*OutReceipt.MapDescriptor, *OutReceipt.MapIdentity);
	return true;
}

bool Fdemo_map0909BM01RuntimeAdapter::ConfirmRuntimeReady(
	const FGuid& StartAttemptId,
	Fdemo_map0909BM01RuntimeReceipt& OutReceipt)
{
	StartReceipt(StartAttemptId, OutReceipt);
	if (!ActiveAttempt.IsSet() || ActiveAttempt->StartAttemptId != StartAttemptId)
	{
		OutReceipt.ReceiptClass = Edemo_map0909BM01RuntimeReceiptClass::StaleEventIgnored;
		OutReceipt.FailureClass = TEXT("StaleRuntimeCallback");
		OutReceipt.Detail = TEXT("Ignored RuntimeReady callback for a non-pending StartAttempt.");
		return false;
	}
	if (ActiveAttempt->bRuntimeReady)
	{
		OutReceipt = ActiveAttempt.GetValue();
		OutReceipt.Sequence = ++NextReceiptSequence;
		OutReceipt.ReceiptClass = Edemo_map0909BM01RuntimeReceiptClass::DuplicateReadyIgnored;
		OutReceipt.FailureClass = TEXT("DuplicateRuntimeReady");
		OutReceipt.Detail = TEXT("Ignored duplicate RuntimeReady callback for the already confirmed attempt.");
		return false;
	}
	OutReceipt = ActiveAttempt.GetValue();
	OutReceipt.Sequence = ++NextReceiptSequence;
	if (!CollectWorldFacts(StartAttemptId, OutReceipt.OwnerId, OutReceipt.RunInstanceId, true, OutReceipt))
	{
		ActiveAttempt = OutReceipt;
		return false;
	}
	OutReceipt.bRuntimeReady = true;
	OutReceipt.ReceiptClass = Edemo_map0909BM01RuntimeReceiptClass::RuntimeReady;
	OutReceipt.FailureClass = TEXT("None");
	OutReceipt.Detail = TEXT("M01 World, GameMode, WorldSettings, PlayerController, Pawn and restored input are ready for this attempt.");
	ActiveAttempt = OutReceipt;
	UE_LOG(Logdemo_map, Log,
		TEXT("0_0_9BFIX_M01_ADAPTER Event=RuntimeReady AttemptId=%s OwnerId=%s RunId=%s Descriptor=%s Map=%s"),
		*StartAttemptId.ToString(EGuidFormats::DigitsWithHyphens),
		*OutReceipt.OwnerId.ToString(EGuidFormats::DigitsWithHyphens),
		*OutReceipt.RunInstanceId.ToString(EGuidFormats::DigitsWithHyphens),
		*OutReceipt.MapDescriptor, *OutReceipt.MapIdentity);
	return true;
}

bool Fdemo_map0909BM01RuntimeAdapter::CancelAttempt(
	const FGuid& StartAttemptId,
	FString& OutDiagnostic)
{
	OutDiagnostic.Reset();
	if (!ActiveAttempt.IsSet())
	{
		OutDiagnostic = TEXT("No M01 adapter attempt was pending; transient cleanup is already complete.");
		return true;
	}
	if (ActiveAttempt->StartAttemptId != StartAttemptId)
	{
		OutDiagnostic = TEXT("Ignored stale technical-failure cleanup for a different StartAttempt.");
		return true;
	}
	const bool bCodeARunCreated = ActiveAttempt->bCodeARunCreated;
	if (!bCodeARunCreated)
	{
		ActiveAttempt.Reset();
		OutDiagnostic = TEXT("Discarded the uncommitted M01 activation request; no Code A Run existed.");
		return true;
	}
	if (!GameMode.IsValid())
	{
		OutDiagnostic = TEXT("Cannot clean the matching M01 attempt because its GameMode is unavailable.");
		return false;
	}
	FString RollbackDiagnostic;
	if (!GameMode->Rollback0909BPreparedRun(RollbackDiagnostic))
	{
		OutDiagnostic = TEXT("Matching M01 attempt rollback failed: ") + RollbackDiagnostic;
		return false;
	}
	ActiveAttempt.Reset();
	OutDiagnostic = TEXT("Matching M01 attempt delegates, transient identity and activation state were released: ")
		+ RollbackDiagnostic;
	return true;
}

bool Fdemo_map0909BM01RuntimeAdapter::IsAttemptPending(const FGuid& StartAttemptId) const
{
	return ActiveAttempt.IsSet() && ActiveAttempt->StartAttemptId == StartAttemptId
		&& !ActiveAttempt->bRuntimeReady;
}
