#include "demo_mapProfilePreparationFlow.h"

#include "demo_mapItemSubsystem.h"
#include "demo_mapProfilePreparationWidget.h"
#include "demo_mapProfileSessionSubsystem.h"
#include "demo_mapShanmenItemAuthoritySubsystem.h"
#include "demo_mapShanmenRunLifecycleAdapter.h"
#include "Engine/GameInstance.h"
#if !UE_BUILD_SHIPPING
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#endif

#if !UE_BUILD_SHIPPING
namespace
{
	FString CanonicalDirectory(const FString& Path)
	{
		FString Result = FPaths::ConvertRelativePathToFull(Path);
		FPaths::CollapseRelativeDirectories(Result);
		FPaths::NormalizeDirectoryName(Result);
		return Result;
	}

	bool IsSameOrChildPath(const FString& Candidate, const FString& Parent)
	{
		return Candidate.Equals(Parent, ESearchCase::IgnoreCase)
			|| Candidate.StartsWith(Parent + TEXT("/"), ESearchCase::IgnoreCase);
	}

	TSet<FString> ExactStructuredAutomationLeaves()
	{
		return {
			TEXT("Dev.D.UE.0.0.5.P8.0.r0"),
			TEXT("Dev.D.UE.0.0.5.P8.2.r0"),
			TEXT("Dev.D.UE.0.0.5.P8.3.r0"),
			TEXT("Dev.D.UE.0.0.5.P8.4.r0"),
			TEXT("Dev.D.UE.0.0.5.P8.6.r0"),
			TEXT("Dev.D.UE.0.0.5.P8.7.r0"),
			TEXT("Dev.D.UE.0.0.5.P8.8.r0"),
			TEXT("Dev.D.UE.0.0.5.P8.9.r0"),
			TEXT("Dev.D.UE.0.0.5.P8.10.r0"),
			TEXT("Dev.D.UE.0.0.5.P8.11.r0"),
			TEXT("Dev.D.UE.0.0.5.P8.12.r0"),
			TEXT("Dev.D.UE.0.0.5.F0.0.r0"),
			TEXT("Dev.D.UE.0.0.5.F0.0.r1"),
			TEXT("Dev.D.UE.0.0.5.F0.0.r2"),
			TEXT("Dev.D.UE.0.0.5.F0.0.r3"),
			TEXT("Dev.D.UE.0.0.5.F0.0.r4"),
			TEXT("Dev.D.UE.0.0.5.F0.0.r5"),
			TEXT("Dev.D.UE.0.0.5.F0.0.r6"),
			TEXT("Dev.D.UE.0.0.5.F0.0.r7"),
			TEXT("Dev.D.UE.0.0.5.P8.15.r0"),
			TEXT("Dev.D.UE.0.0.5.P8.16.r0"),
			TEXT("Dev.D.UE.0.0.5.P8.17.r0"),
			TEXT("Dev.D.UE.0.0.5.P8.18.r0"),
			TEXT("Dev.D.UE.0.0.5.P8.20.r0"),
			TEXT("Dev.D.UE.0.0.6.F0.0.r1"),
			TEXT("Dev.D.UE.0.0.6.F0.0.r2"),
			TEXT("Dev.D.UE.0.0.6.F0.0.r3"),
			TEXT("Dev.D.UE.0.0.6.F0.0.r4"),
			TEXT("Dev.D.UE.0.0.7.F0.0.r0"),
			// P1.0.r1 keeps its two-run smoke profile, generated config, and
			// evidence under one task-scoped root rather than using production
			// save data.
			TEXT("Dev.D.UE.0.0.9-XFix1.P1.0.r1")
		};
	}

	void AddReparseChain(
		const FString& TargetPath,
		const FString& ProjectRoot,
		TArray<FString>& OutReparsePaths)
	{
		FString Current = CanonicalDirectory(TargetPath);
		const FString CanonicalProjectRoot = CanonicalDirectory(ProjectRoot);
		while (!Current.IsEmpty())
		{
			const bool bExists = IFileManager::Get().DirectoryExists(*Current)
				|| IFileManager::Get().FileExists(*Current);
			if (bExists && IFileManager::Get().IsSymlink(*Current))
			{
				OutReparsePaths.AddUnique(Current);
			}
			if (Current.Equals(CanonicalProjectRoot, ESearchCase::IgnoreCase))
			{
				break;
			}
			const FString Parent = CanonicalDirectory(FPaths::GetPath(Current));
			if (Parent.IsEmpty() || Parent.Equals(Current, ESearchCase::IgnoreCase))
			{
				break;
			}
			Current = Parent;
		}
	}
}
#endif

Fdemo_mapProfileSessionInitializeResult Fdemo_mapProfilePreparationFlow::InitializeProduction(
	UGameInstance* GameInstance)
{
	return InitializeWithStorage(GameInstance, Fdemo_mapProfileStorageContext::Production());
}

#if !UE_BUILD_SHIPPING
FString Fdemo_mapProfilePreparationFlow::AllowedAutomationRoot()
{
	return CanonicalDirectory(FPaths::Combine(
		FPaths::ProjectSavedDir(),
		TEXT("Automation"),
		TEXT("Dev.D.UE.0.0.4.14.r0")));
}

bool Fdemo_mapProfilePreparationFlow::IsRootKindAcceptedForAutomation(
	bool bDirectoryExists,
	bool bIsReparseOrSymlink)
{
	return !bDirectoryExists || !bIsReparseOrSymlink;
}

TArray<FString> Fdemo_mapProfilePreparationFlow::ProtectedProductionRootsForAutomation(
	const FString& CanonicalProjectRoot)
{
	TArray<FString> ProtectedRoots;
	if (!CanonicalProjectRoot.IsEmpty())
	{
		ProtectedRoots.Add(FPaths::Combine(
			CanonicalProjectRoot,
			TEXT("Saved"),
			TEXT("SaveGames"),
			TEXT("Shanmen")));
	}

	const FString LocalAppData = FPlatformMisc::GetEnvironmentVariable(TEXT("LOCALAPPDATA"));
	if (!LocalAppData.IsEmpty())
	{
		ProtectedRoots.Add(FPaths::Combine(LocalAppData, TEXT("demo_map")));
	}
	return ProtectedRoots;
}

Fdemo_mapAutomationRootBoundaryResult
Fdemo_mapProfilePreparationFlow::ValidateExplicitAutomationBoundary(
	const FString& AutomationRoot,
	const FString& StorageRoot,
	const FString& UserConfigRoot,
	const FString& UserDirRoot,
	const FString& GeneratedConfigRoot,
	const TArray<FString>& EvidencePaths)
{
	Fdemo_mapAutomationRootBoundaryPaths Paths;
	Paths.AutomationRoot = AutomationRoot;
	Paths.StorageRoot = StorageRoot;
	Paths.UserConfigRoot = UserConfigRoot;
	Paths.UserDirRoot = UserDirRoot;
	Paths.GeneratedConfigRoot = GeneratedConfigRoot;
	Paths.EvidencePaths = EvidencePaths;

	FString CanonicalRoot;
	FString ProjectRoot;
	Fdemo_mapAutomationRootBoundary::DeriveCanonicalProjectRoot(
		AutomationRoot,
		CanonicalRoot,
		ProjectRoot);
	const FString AutomationParent = CanonicalDirectory(FPaths::GetPath(CanonicalRoot));
	const FString SavedParent = CanonicalDirectory(FPaths::GetPath(AutomationParent));
	const FString UprojectPath = CanonicalDirectory(
		FPaths::Combine(ProjectRoot, TEXT("demo_map.uproject")));
	if (IFileManager::Get().FileExists(*UprojectPath)
		&& !IFileManager::Get().DirectoryExists(*UprojectPath)
		&& !IFileManager::Get().IsSymlink(*UprojectPath))
	{
		Paths.ExistingOrdinaryFiles.Add(UprojectPath);
	}

	TArray<FString> ReparseTargets = {
		ProjectRoot,
		SavedParent,
		AutomationParent,
		CanonicalRoot,
		StorageRoot,
		UserConfigRoot,
		UserDirRoot,
		GeneratedConfigRoot,
		UprojectPath
	};
	ReparseTargets.Append(EvidencePaths);
	for (const FString& Target : ReparseTargets)
	{
		AddReparseChain(Target, ProjectRoot, Paths.ReparsePaths);
	}

	Paths.ProductionRoots = ProtectedProductionRootsForAutomation(ProjectRoot);

	const FString WorkspaceRoot = CanonicalDirectory(FPaths::GetPath(ProjectRoot));
	const FString DemoBuildRoot = FPaths::Combine(WorkspaceRoot, TEXT("Builds"), TEXT("demo_map"));
	Paths.ExternalDeliveryRoots = {
		FPaths::Combine(DemoBuildRoot, TEXT("Candidate")),
		FPaths::Combine(DemoBuildRoot, TEXT("ManualAcceptance")),
		FPaths::Combine(DemoBuildRoot, TEXT("Latest")),
		FPaths::Combine(DemoBuildRoot, TEXT("Launcher")),
		FPaths::Combine(DemoBuildRoot, TEXT("Freeze")),
		FPaths::Combine(WorkspaceRoot, TEXT("Versions"))
	};

	return Fdemo_mapAutomationRootBoundary::Evaluate(
		Paths,
		ExactStructuredAutomationLeaves());
}

bool Fdemo_mapProfilePreparationFlow::ValidateInjectedStorageRoot(
	const FString& RequestedRoot,
	FString& OutCanonicalRoot,
	FString& OutDiagnostic)
{
	OutCanonicalRoot.Reset();
	if (RequestedRoot.TrimStartAndEnd().IsEmpty())
	{
		OutDiagnostic = TEXT("Profile Flow requires an explicitly injected storage root.");
		return false;
	}

	FString ExplicitAutomationRoot;
	FString ExplicitUserConfigRoot;
	FParse::Value(
		FCommandLine::Get(),
		TEXT("FullSystemLoopAutomationRoot="),
		ExplicitAutomationRoot);
	FParse::Value(
		FCommandLine::Get(),
		TEXT("FullSystemLoopUserConfigRoot="),
		ExplicitUserConfigRoot);
	if (ExplicitAutomationRoot.IsEmpty())
	{
		FParse::Value(
			FCommandLine::Get(),
			TEXT("InputRestoreAutomationRoot="),
			ExplicitAutomationRoot);
		FParse::Value(
			FCommandLine::Get(),
			TEXT("InputRestoreUserConfigRoot="),
			ExplicitUserConfigRoot);
	}
	if (!ExplicitAutomationRoot.IsEmpty())
	{
		FString CommandUserDir;
		FParse::Value(FCommandLine::Get(), TEXT("UserDir="), CommandUserDir);
		const Fdemo_mapAutomationRootBoundaryResult Boundary =
			ValidateExplicitAutomationBoundary(
				ExplicitAutomationRoot,
				RequestedRoot,
				ExplicitUserConfigRoot,
				CommandUserDir,
				FPaths::GeneratedConfigDir(),
				{ RequestedRoot, ExplicitUserConfigRoot, FPaths::GeneratedConfigDir() });
		if (!Boundary.bAccepted)
		{
			OutDiagnostic = Boundary.Diagnostic();
			return false;
		}
		OutCanonicalRoot = Boundary.CanonicalStorageRoot;
		OutDiagnostic = Boundary.Diagnostic();
		return true;
	}

	const FString Canonical = CanonicalDirectory(RequestedRoot);
	const FString Allowed = AllowedAutomationRoot();
	const FString ProfileTradeAllowed = CanonicalDirectory(FPaths::Combine(
		FPaths::ProjectSavedDir(),
		TEXT("Automation"),
		TEXT("Dev.D.UE.0.0.5.P3.0.r0")));
	const FString FullSystemLoopAllowed = CanonicalDirectory(FPaths::Combine(
		FPaths::ProjectDir(),
		TEXT("Saved"),
		TEXT("Automation"),
		TEXT("Dev.D.UE.0.0.5.P8.0.r0")));
	const FString P6r3Allowed = CanonicalDirectory(FPaths::Combine(
		FPaths::ProjectSavedDir(),
		TEXT("Automation"),
		TEXT("Dev.D.UE.0.0.9B.P6.0.r3")));
	// I1's two real CTA checks use an isolated profile under Saved/Automation.
	// This is intentionally a narrow task leaf, never a production SaveGames
	// path and never an unrestricted user-provided storage root.
	const FString I1Allowed = CanonicalDirectory(FPaths::Combine(
		FPaths::ProjectSavedDir(),
		TEXT("Automation"),
		TEXT("Dev.D.UE.0.0.9B.I1.0.r0")));
	const FString Production = CanonicalDirectory(Fdemo_mapProfileStorageContext::Production().RootDirectory);
	if (!IsSameOrChildPath(Canonical, Allowed)
		&& !IsSameOrChildPath(Canonical, ProfileTradeAllowed)
		&& !IsSameOrChildPath(Canonical, FullSystemLoopAllowed)
		&& !IsSameOrChildPath(Canonical, P6r3Allowed)
		&& !IsSameOrChildPath(Canonical, I1Allowed))
	{
		OutDiagnostic = FString::Printf(
			TEXT("Profile Flow storage root escaped the task automation boundary: %s"),
			*Canonical);
		return false;
	}
	if (IsSameOrChildPath(Canonical, Production) || IsSameOrChildPath(Production, Canonical))
	{
		OutDiagnostic = TEXT("Profile Flow storage root overlaps the production SaveGames/Shanmen boundary.");
		return false;
	}
	const bool bDirectoryExists = IFileManager::Get().DirectoryExists(*Canonical);
	if (!IsRootKindAcceptedForAutomation(bDirectoryExists, bDirectoryExists && IFileManager::Get().IsSymlink(*Canonical)))
	{
		OutDiagnostic = TEXT("Profile Flow storage root is a reparse/symlink directory and was rejected.");
		return false;
	}

	OutCanonicalRoot = Canonical;
	OutDiagnostic = TEXT("Profile Flow storage root accepted inside the task-isolated automation boundary.");
	return true;
}

Fdemo_mapProfileSessionInitializeResult Fdemo_mapProfilePreparationFlow::InitializeExplicit(
	UGameInstance* GameInstance,
	const FString& RequestedRoot)
{
	Fdemo_mapProfileSessionInitializeResult Result;
	FString Canonical;
	FString Diagnostic;
	if (!GameInstance || !ValidateInjectedStorageRoot(RequestedRoot, Canonical, Diagnostic))
	{
		Result.Status = Edemo_mapProfileSessionInitializeStatus::FatalProfileError;
		Result.Diagnostic = GameInstance ? Diagnostic : TEXT("Profile Flow requires an owning GameInstance.");
		return Result;
	}
	return InitializeWithStorage(GameInstance, Fdemo_mapProfileStorageContext::ForRoot(Canonical));
}
#endif

Fdemo_mapProfileSessionInitializeResult Fdemo_mapProfilePreparationFlow::InitializeWithStorage(
	UGameInstance* GameInstance,
	const Fdemo_mapProfileStorageContext& Storage)
{
	Fdemo_mapProfileSessionInitializeResult Result;
	if (!GameInstance || Storage.RootDirectory.TrimStartAndEnd().IsEmpty())
	{
		Result.Status = Edemo_mapProfileSessionInitializeStatus::FatalProfileError;
		Result.Diagnostic = GameInstance
			? TEXT("Profile Flow requires an authoritative storage context.")
			: TEXT("Profile Flow requires an owning GameInstance.");
		return Result;
	}
	if (Phase != Edemo_mapProfilePreparationFlowPhase::Disabled)
	{
		Result.Status = Edemo_mapProfileSessionInitializeStatus::RecoveryRequired;
		Result.Diagnostic = TEXT("Profile Flow adapter rebinding was rejected; unbind before a new lifecycle.");
		Result.Snapshot = Session.IsValid() ? Session->GetSnapshot() : Fdemo_mapProfileSessionSnapshot();
		return Result;
	}

	Session = GameInstance->GetSubsystem<Udemo_mapProfileSessionSubsystem>();
	Runtime = GameInstance->GetSubsystem<Udemo_mapItemSubsystem>();
	ShanmenAuthority =
		GameInstance->GetSubsystem<Udemo_mapShanmenItemAuthoritySubsystem>();
	if (!Session.IsValid() || !Runtime.IsValid())
	{
		Session.Reset();
		Runtime.Reset();
		ShanmenAuthority.Reset();
		Result.Status = Edemo_mapProfileSessionInitializeStatus::FatalProfileError;
		Result.Diagnostic = TEXT("Profile Flow could not bind the existing Session and Runtime authorities.");
		return Result;
	}

	StorageRoot = Storage.RootDirectory;
	Result = Session->InitializeSession(Storage);
	PreviousRunId = Result.Snapshot.ActiveRunId;
	RecoveredAbandonRunId = Result.IsReady()
		&& Result.Snapshot.LastTerminalReason == Edemo_mapRunEndReason::RecoveredAbandon
		&& (Result.Status == Edemo_mapProfileSessionInitializeStatus::RecoveredAbandonCommitted
			|| Result.Status == Edemo_mapProfileSessionInitializeStatus::RecoveredAbandonAlreadyCommitted)
		? Result.Snapshot.ActiveRunId : FGuid();
	PreviousSettlementId = Result.Snapshot.LastSettlementId;
	ProfileId = Result.Snapshot.ProfileId;
	Phase = Result.IsReady()
		? Edemo_mapProfilePreparationFlowPhase::Preparation
		: Edemo_mapProfilePreparationFlowPhase::RecoveryRequired;
	return Result;
}

Fdemo_mapProfileSessionBeginResult Fdemo_mapProfilePreparationFlow::StartPreparedRunThroughWidget(
	Udemo_mapProfilePreparationWidget* Widget)
{
	Fdemo_mapProfileSessionBeginResult Result;
	if (Phase != Edemo_mapProfilePreparationFlowPhase::Preparation
		|| !Session.IsValid()
		|| !Runtime.IsValid()
		|| !Widget)
	{
		Result.Status = Edemo_mapProfileSessionBeginStatus::SessionNotReady;
		Result.Diagnostic = TEXT("Profile Flow Start requires the bound Preparation phase and its existing UI.");
		Result.Snapshot = Session.IsValid() ? Session->GetSnapshot() : Fdemo_mapProfileSessionSnapshot();
		return Result;
	}

	Widget->InitializeForSession(Session.Get());
	if (UsesShanmenItemLifecycle())
	{
		// The retained widget is presentation only after cutover.  Its Start
		// button must enter the same atomic authority path as the sect CTA.
		return StartPreparedRunDirect();
	}
	Result = Widget->RequestStartRun();
	if (!Result.IsRunActive())
	{
		if (Result.Snapshot.SessionState == Edemo_mapProfileSessionState::RecoveryRequired
			|| Result.Snapshot.SessionState == Edemo_mapProfileSessionState::FatalProfileError)
		{
			Phase = Edemo_mapProfilePreparationFlowPhase::RecoveryRequired;
		}
		return Result;
	}

	const FGuid SessionRunId = Result.Snapshot.ActiveRunId;
	const FGuid RuntimeRunId = Runtime->GetActiveRunId();
	if (!SessionRunId.IsValid()
		|| SessionRunId != RuntimeRunId
		|| Runtime->GetRunState() != Edemo_mapRunState::Active)
	{
		Result.Status = Edemo_mapProfileSessionBeginStatus::RuntimeMaterializationFailed;
		Result.Diagnostic = TEXT("Prepared Run materialized without one matching Session/Runtime ActiveRunId.");
		Phase = Edemo_mapProfilePreparationFlowPhase::RecoveryRequired;
		return Result;
	}

	StartedRunId = RuntimeRunId;
	Phase = Edemo_mapProfilePreparationFlowPhase::RunActive;
	return Result;
}

Fdemo_mapProfileSessionBeginResult Fdemo_mapProfilePreparationFlow::StartPreparedRunDirect()
{
	Fdemo_mapProfileSessionBeginResult Result;
	if (Phase != Edemo_mapProfilePreparationFlowPhase::Preparation
		|| !Session.IsValid()
		|| !Runtime.IsValid())
	{
		Result.Status = Edemo_mapProfileSessionBeginStatus::SessionNotReady;
		Result.Diagnostic = TEXT("Profile Flow Start requires a ready preparation session.");
		Result.Snapshot = Session.IsValid()
			? Session->GetSnapshot() : Fdemo_mapProfileSessionSnapshot();
		return Result;
	}

	if (Udemo_mapShanmenItemAuthoritySubsystem* Authority =
		FindBoundShanmenAuthority())
	{
		const Fdemo_mapShanmenRunStartResult Start =
			Fdemo_mapShanmenRunLifecycleAdapter::StartPreparedRun(
				*Authority, *Runtime);
		Result.Diagnostic = Start.Diagnostic;
		Result.RuntimeResult = Start.RuntimeResult;
		if (Start.IsStarted()
			&& Start.ActiveRunId.IsValid()
			&& Runtime->GetRunState() == Edemo_mapRunState::Active
			&& Runtime->GetActiveRunId() == Start.ActiveRunId)
		{
			StartedRunId = Start.ActiveRunId;
			bShanmenRunMaterialized = true;
			PendingShanmenSettlement.Reset();
			Phase = Edemo_mapProfilePreparationFlowPhase::RunActive;
			Result.Status =
				Edemo_mapProfileSessionBeginStatus::CommittedAndMaterialized;
			Result.Snapshot = GetPresentationSnapshot();
			return Result;
		}

		const FGuid RecoverableRunId = GetRecoverableShanmenRunId();
		if (RecoverableRunId.IsValid())
		{
			StartedRunId = RecoverableRunId;
			bShanmenRunMaterialized = false;
			Phase = Edemo_mapProfilePreparationFlowPhase::Preparation;
			Result.Status =
				Edemo_mapProfileSessionBeginStatus::RuntimeMaterializationFailed;
			Result.Diagnostic +=
				TEXT(" Durable ActiveRun remains recoverable; Start again to rematerialize the same identity.");
		}
		else
		{
			Result.Status = Start.Status
					== Edemo_mapShanmenRunLifecycleStatus::PreparedLoadoutRejected
				? Edemo_mapProfileSessionBeginStatus::StaleIntent
				: Start.Status
						== Edemo_mapShanmenRunLifecycleStatus::AuthorityNotReady
					? Edemo_mapProfileSessionBeginStatus::SessionNotReady
					: Edemo_mapProfileSessionBeginStatus::RuntimeMaterializationFailed;
			Phase = Start.Status
					== Edemo_mapShanmenRunLifecycleStatus::AuthorityNotReady
				? Edemo_mapProfilePreparationFlowPhase::RecoveryRequired
				: Edemo_mapProfilePreparationFlowPhase::Preparation;
		}
		Result.Snapshot = GetPresentationSnapshot();
		return Result;
	}

	// The product teleport path never needs a Preparation Widget or a committed
	// PreparationLayout.  The Flow remains the existing Profile/Runtime lifecycle
	// owner, while Start Run itself bypasses all legacy loadout gate semantics.
	Result = Session->StartRunWithoutPreparation();
	if (!Result.IsRunActive())
	{
		if (Result.Snapshot.SessionState == Edemo_mapProfileSessionState::RecoveryRequired
			|| Result.Snapshot.SessionState == Edemo_mapProfileSessionState::FatalProfileError)
		{
			Phase = Edemo_mapProfilePreparationFlowPhase::RecoveryRequired;
		}
		return Result;
	}

	const FGuid SessionRunId = Result.Snapshot.ActiveRunId;
	const FGuid RuntimeRunId = Runtime->GetActiveRunId();
	if (!SessionRunId.IsValid()
		|| SessionRunId != RuntimeRunId
		|| Runtime->GetRunState() != Edemo_mapRunState::Active)
	{
		Result.Status = Edemo_mapProfileSessionBeginStatus::RuntimeMaterializationFailed;
		Result.Diagnostic = TEXT("Prepared Run materialized without one matching Session/Runtime ActiveRunId.");
		Phase = Edemo_mapProfilePreparationFlowPhase::RecoveryRequired;
		return Result;
	}

	StartedRunId = RuntimeRunId;
	Phase = Edemo_mapProfilePreparationFlowPhase::RunActive;
	return Result;
}

Fdemo_mapProfileSessionSettlementResult Fdemo_mapProfilePreparationFlow::CommitRuntimeSettlement(
	const Fdemo_mapSettlementSummary& Summary)
{
	if (Phase != Edemo_mapProfilePreparationFlowPhase::RunActive || !Session.IsValid() || !Runtime.IsValid())
	{
		return RejectSettlement(TEXT("Profile Flow rejected a duplicate or out-of-phase settlement submission."));
	}
	if (!Summary.bValid
		|| !Summary.RuntimeSnapshot.bValid
		|| Summary.RunId != StartedRunId
		|| Summary.RuntimeSnapshot.ActiveRunId != StartedRunId
		|| Summary.Reason == Edemo_mapRunEndReason::None
		|| Summary.RuntimeSnapshot.CommittedEndReason != Summary.Reason)
	{
		return RejectSettlement(TEXT("Profile Flow settlement RunId/reason/evidence did not match the active prepared run."));
	}

	if (bShanmenRunMaterialized && UsesShanmenItemLifecycle())
	{
		return FinalizeShanmenSettlement(Summary, false);
	}

	++SettlementSubmitCount;
	const Fdemo_mapProfileSessionSettlementResult Result = Session->CommitRuntimeSettlement(Summary);
	ApplySettlementResult(Result);
	return Result;
}

Fdemo_mapProfileSessionSettlementResult Fdemo_mapProfilePreparationFlow::RetryPendingSettlement()
{
	if (Phase != Edemo_mapProfilePreparationFlowPhase::SettlementPending || !Session.IsValid())
	{
		return RejectSettlement(TEXT("Profile Flow has no explicit pending settlement retry to submit."));
	}
	if (bShanmenRunMaterialized && PendingShanmenSettlement.IsSet()
		&& UsesShanmenItemLifecycle())
	{
		return FinalizeShanmenSettlement(
			PendingShanmenSettlement.GetValue(), true);
	}
	++SettlementRetryCount;
	const Fdemo_mapProfileSessionSettlementResult Result = Session->RetryPendingSettlement();
	ApplySettlementResult(Result);
	return Result;
}

Fdemo_mapProfileSessionSettlementResult Fdemo_mapProfilePreparationFlow::CancelActiveRunForActivationFailure()
{
	if (Phase != Edemo_mapProfilePreparationFlowPhase::RunActive || !Runtime.IsValid())
	{
		return RejectSettlement(TEXT("Profile Flow has no active prepared run to roll back after world activation failure."));
	}
	if (bShanmenRunMaterialized && UsesShanmenItemLifecycle())
	{
		Fdemo_mapSettlementSummary Summary;
		const Fdemo_mapItemOperationResult RuntimeResult =
			Runtime->RequestSettlement(
				Edemo_mapRunEndReason::ActivationFailure, Summary);
		if (!RuntimeResult.bSuccess)
		{
			Phase = Edemo_mapProfilePreparationFlowPhase::RecoveryRequired;
			return RejectSettlement(RuntimeResult.Diagnostic);
		}
		++SettlementSubmitCount;
		const Fdemo_mapItemOperationResult Prepared =
			Runtime->PrepareForPersistentRun();
		Fdemo_mapProfileSessionSettlementResult Result;
		Result.Snapshot = Session->GetSnapshot();
		Result.Snapshot.LastTerminalReason =
			Edemo_mapRunEndReason::ActivationFailure;
		Result.Status = Prepared.bSuccess
			? Edemo_mapProfileSessionSettlementStatus::RuntimeRollbackReady
			: Edemo_mapProfileSessionSettlementStatus::FatalProfileError;
		Result.Diagnostic = Prepared.bSuccess
			? TEXT("Technical activation rollback cleared only Runtime; the durable Shanmen ActiveRun remains available for exact-identity recovery.")
			: Prepared.Diagnostic;
		bShanmenRunMaterialized = false;
		PendingShanmenSettlement.Reset();
		Phase = Prepared.bSuccess
			? Edemo_mapProfilePreparationFlowPhase::Preparation
			: Edemo_mapProfilePreparationFlowPhase::RecoveryRequired;
		return Result;
	}
	Fdemo_mapSettlementSummary Summary;
	const Fdemo_mapItemOperationResult RuntimeResult = Runtime->RequestSettlement(
		Edemo_mapRunEndReason::ActivationFailure,
		Summary);
	if (!RuntimeResult.bSuccess)
	{
		Phase = Edemo_mapProfilePreparationFlowPhase::RecoveryRequired;
		return RejectSettlement(RuntimeResult.Diagnostic);
	}
	return CommitRuntimeSettlement(Summary);
}

void Fdemo_mapProfilePreparationFlow::Unbind()
{
	Session.Reset();
	Runtime.Reset();
	ShanmenAuthority.Reset();
	StorageRoot.Reset();
	ProfileId.Invalidate();
	StartedRunId.Invalidate();
	PreviousRunId.Invalidate();
	RecoveredAbandonRunId.Invalidate();
	PreviousSettlementId.Invalidate();
	Phase = Edemo_mapProfilePreparationFlowPhase::Disabled;
	SettlementSubmitCount = 0;
	SettlementRetryCount = 0;
	bShanmenRunMaterialized = false;
	PendingShanmenSettlement.Reset();
}

Udemo_mapShanmenItemAuthoritySubsystem*
Fdemo_mapProfilePreparationFlow::FindBoundShanmenAuthority() const
{
	Udemo_mapShanmenItemAuthoritySubsystem* Authority =
		ShanmenAuthority.Get();
	return Authority
		&& Authority->GetLifecycleState()
			== Edemo_mapShanmenItemAuthorityLifecycleState::Ready
		&& Authority->GetBoundOwnerId() == ProfileId
		&& Authority->GetBoundStorageRoot().Equals(
			StorageRoot, ESearchCase::IgnoreCase)
		? Authority : nullptr;
}

bool Fdemo_mapProfilePreparationFlow::UsesShanmenItemLifecycle() const
{
	return FindBoundShanmenAuthority() != nullptr;
}

FGuid Fdemo_mapProfilePreparationFlow::GetRecoverableShanmenRunId() const
{
	FGuid ActiveRunId;
	if (const Udemo_mapShanmenItemAuthoritySubsystem* Authority =
		FindBoundShanmenAuthority())
	{
		Fdemo_mapShanmenRunLifecycleAdapter::TryFindRecoverableActiveRun(
			*Authority, ActiveRunId);
	}
	return ActiveRunId;
}

bool Fdemo_mapProfilePreparationFlow::TryGetActiveShanmenRunCorrelation(
	Fdemo_mapShanmenRunCorrelation& OutCorrelation,
	FString* OutDiagnostic) const
{
	if (const Udemo_mapShanmenItemAuthoritySubsystem* Authority =
		FindBoundShanmenAuthority())
	{
		return Fdemo_mapShanmenRunLifecycleAdapter::TryGetActiveRunCorrelation(
			*Authority, OutCorrelation, OutDiagnostic);
	}
	OutCorrelation = Fdemo_mapShanmenRunCorrelation();
	if (OutDiagnostic)
	{
		*OutDiagnostic =
			TEXT("No bound ShanmenItems authority exists for Run correlation.");
	}
	return false;
}

Fdemo_mapProfileSessionSnapshot
Fdemo_mapProfilePreparationFlow::GetPresentationSnapshot() const
{
	Fdemo_mapProfileSessionSnapshot Snapshot = Session.IsValid()
		? Session->GetSnapshot() : Fdemo_mapProfileSessionSnapshot();
	if (bShanmenRunMaterialized
		&& Phase == Edemo_mapProfilePreparationFlowPhase::RunActive
		&& StartedRunId.IsValid())
	{
		Snapshot.SessionState = Edemo_mapProfileSessionState::RunActive;
		Snapshot.ActiveRunId = StartedRunId;
		Snapshot.bCanBeginRun = false;
		Snapshot.bCanRetrySettlement = false;
		Snapshot.VisibleDiagnostic =
			TEXT("Runtime is a transient projection of the durable Shanmen ActiveRun.");
	}
	return Snapshot;
}

Fdemo_mapProfileSessionSettlementResult
Fdemo_mapProfilePreparationFlow::FinalizeShanmenSettlement(
	const Fdemo_mapSettlementSummary& Summary,
	const bool bRetry)
{
	Fdemo_mapProfileSessionSettlementResult Result;
	Udemo_mapShanmenItemAuthoritySubsystem* Authority =
		FindBoundShanmenAuthority();
	if (!Authority || !Runtime.IsValid())
	{
		Result = RejectSettlement(
			TEXT("Shanmen settlement lost its bound authority or Runtime."));
		Phase = Edemo_mapProfilePreparationFlowPhase::RecoveryRequired;
		return Result;
	}
	if (bRetry)
	{
		++SettlementRetryCount;
	}
	else
	{
		++SettlementSubmitCount;
	}
	const Fdemo_mapShanmenRunFinalizeResult Finalized =
		Fdemo_mapShanmenRunLifecycleAdapter::FinalizeSettlement(
			*Authority, Summary);
	Result.Snapshot = Session->GetSnapshot();
	Result.Snapshot.LastTerminalReason = Summary.Reason;
	Result.Diagnostic = Finalized.Diagnostic;
	if (Finalized.IsFinalized())
	{
		Result.Status = Finalized.Status
				== Edemo_mapShanmenRunLifecycleStatus::NoChange
			? Edemo_mapProfileSessionSettlementStatus::AlreadyCommitted
			: Edemo_mapProfileSessionSettlementStatus::Committed;
		const Fdemo_mapItemOperationResult RuntimePreparation =
			Runtime->PrepareForPersistentRun();
		bShanmenRunMaterialized = false;
		PendingShanmenSettlement.Reset();
		StartedRunId.Invalidate();
		Phase = RuntimePreparation.bSuccess
			? Edemo_mapProfilePreparationFlowPhase::Preparation
			: Edemo_mapProfilePreparationFlowPhase::RecoveryRequired;
		if (!RuntimePreparation.bSuccess)
		{
			Result.Status =
				Edemo_mapProfileSessionSettlementStatus::FatalProfileError;
			Result.Diagnostic += TEXT(" Runtime cleanup failed: ")
				+ RuntimePreparation.Diagnostic;
		}
		return Result;
	}

	const bool bRetryable = Finalized.Status
		== Edemo_mapShanmenRunLifecycleStatus::FinalizeRejected;
	Result.Status = bRetryable
		? Edemo_mapProfileSessionSettlementStatus::PendingRetry
		: Finalized.Status
			== Edemo_mapShanmenRunLifecycleStatus::SettlementInvalid
			|| Finalized.Status
				== Edemo_mapShanmenRunLifecycleStatus::UnsupportedTerminalReason
		? Edemo_mapProfileSessionSettlementStatus::EvidenceRejected
		: Edemo_mapProfileSessionSettlementStatus::FatalProfileError;
	PendingShanmenSettlement = bRetryable
		? TOptional<Fdemo_mapSettlementSummary>(Summary)
		: TOptional<Fdemo_mapSettlementSummary>();
	Phase = bRetryable
		? Edemo_mapProfilePreparationFlowPhase::SettlementPending
		: Edemo_mapProfilePreparationFlowPhase::RecoveryRequired;
	return Result;
}

Fdemo_mapProfileSessionSettlementResult Fdemo_mapProfilePreparationFlow::RejectSettlement(
	const FString& Diagnostic) const
{
	Fdemo_mapProfileSessionSettlementResult Result;
	Result.Status = Edemo_mapProfileSessionSettlementStatus::SessionStateRejected;
	Result.Diagnostic = Diagnostic;
	Result.Snapshot = Session.IsValid() ? Session->GetSnapshot() : Fdemo_mapProfileSessionSnapshot();
	return Result;
}

void Fdemo_mapProfilePreparationFlow::ApplySettlementResult(
	const Fdemo_mapProfileSessionSettlementResult& Result)
{
	if (Result.IsDurablySettled())
	{
		const Fdemo_mapItemOperationResult RuntimePreparation =
			Runtime.IsValid()
				? Runtime->PrepareForPersistentRun()
				: Fdemo_mapItemOperationResult::Failure(
					Edemo_mapItemResultCode::InvariantViolation,
					TEXT("Durable Profile settlement lost its Runtime authority before cleanup."));
		Phase = RuntimePreparation.bSuccess
			? Edemo_mapProfilePreparationFlowPhase::Preparation
			: Edemo_mapProfilePreparationFlowPhase::RecoveryRequired;
	}
	else if (Result.Status == Edemo_mapProfileSessionSettlementStatus::PendingRetry)
	{
		Phase = Edemo_mapProfilePreparationFlowPhase::SettlementPending;
	}
	else
	{
		Phase = Edemo_mapProfilePreparationFlowPhase::RecoveryRequired;
	}
}
