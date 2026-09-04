#include "demo_mapShanmenThrownWeaponProductSession.h"

#include "demo_mapPlayerCombat.h"
#include "demo_mapProfilePreparationTypes.h"
#include "demo_mapShanmenItemAuthoritySubsystem.h"

namespace
{
	bool IsFiniteVector(const FVector& Value)
	{
		return FMath::IsFinite(Value.X)
			&& FMath::IsFinite(Value.Y)
			&& FMath::IsFinite(Value.Z);
	}

	bool DefinitionsMatch(
		const FShanmenThrownWeaponDefinition& Left,
		const FShanmenThrownWeaponDefinition& Right)
	{
		return Left.IsValid() && Right.IsValid()
			&& Left.GetActionDefinitionId() == Right.GetActionDefinitionId()
			&& Left.GetDetectorId() == Right.GetDetectorId()
			&& Left.GetFormulaId() == Right.GetFormulaId()
			&& Left.GetBaseDamage() == Right.GetBaseDamage()
			&& Left.GetTechniquePowerCoefficient()
				== Right.GetTechniquePowerCoefficient()
			&& Left.GetLaunchSpeed() == Right.GetLaunchSpeed()
			&& Left.GetDamageTags() == Right.GetDamageTags()
			&& Left.GetRequiredTargetTags()
				== Right.GetRequiredTargetTags()
			&& Left.RejectsSelf() == Right.RejectsSelf();
	}

	Fdemo_mapShanmenThrownWeaponSessionResult Reject(
		Edemo_mapShanmenThrownWeaponSessionStatus Status,
		const Fdemo_mapShanmenThrownWeaponHotbarIntent& Intent,
		const FGuid& RunId,
		const TCHAR* Diagnostic)
	{
		Fdemo_mapShanmenThrownWeaponSessionResult Result;
		Result.Status = Status;
		Result.SelectionId = Intent.GetSelectionId();
		Result.RunId = RunId;
		Result.HotbarSlotNumber = Intent.GetHotbarSlotNumber();
		Result.Diagnostic = Diagnostic;
		return Result;
	}
}

bool Fdemo_mapShanmenThrownWeaponHotbarIntent::TryCapture(
	const FGuid& RequestedSelectionId,
	int32 RequestedHotbarSlotNumber,
	const FVector& RequestedOrigin,
	const FVector& RequestedAimDirection,
	float RequestedMaximumDistance,
	Fdemo_mapShanmenThrownWeaponHotbarIntent& OutIntent)
{
	OutIntent = Fdemo_mapShanmenThrownWeaponHotbarIntent();
	if (!RequestedSelectionId.IsValid()
		|| RequestedHotbarSlotNumber < 1
		|| RequestedHotbarSlotNumber
			> Fdemo_mapPersistentPreparationLayout::HotbarSlotCount
		|| !IsFiniteVector(RequestedOrigin)
		|| !IsFiniteVector(RequestedAimDirection)
		|| RequestedAimDirection.IsNearlyZero()
		|| !FMath::IsFinite(RequestedMaximumDistance)
		|| RequestedMaximumDistance <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	OutIntent.SelectionId = RequestedSelectionId;
	OutIntent.HotbarSlotNumber = RequestedHotbarSlotNumber;
	OutIntent.Origin = RequestedOrigin;
	OutIntent.TrajectoryKind =
		Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::Straight;
	OutIntent.AimDirection = RequestedAimDirection.GetSafeNormal();
	OutIntent.MaximumDistance = RequestedMaximumDistance;
	if (!OutIntent.IsValid())
	{
		OutIntent = Fdemo_mapShanmenThrownWeaponHotbarIntent();
		return false;
	}
	return true;
}

bool Fdemo_mapShanmenThrownWeaponHotbarIntent::TryCaptureArc(
	const FGuid& RequestedSelectionId,
	int32 RequestedHotbarSlotNumber,
	const FVector& RequestedOrigin,
	const FVector& RequestedTarget,
	double RequestedApexClearance,
	Fdemo_mapShanmenThrownWeaponHotbarIntent& OutIntent)
{
	OutIntent = Fdemo_mapShanmenThrownWeaponHotbarIntent();
	if (!RequestedSelectionId.IsValid()
		|| RequestedHotbarSlotNumber < 1
		|| RequestedHotbarSlotNumber
			> Fdemo_mapPersistentPreparationLayout::HotbarSlotCount
		|| !IsFiniteVector(RequestedOrigin)
		|| !IsFiniteVector(RequestedTarget)
		|| RequestedOrigin.Equals(
			RequestedTarget, UE_DOUBLE_SMALL_NUMBER)
		|| !FMath::IsFinite(RequestedApexClearance)
		|| RequestedApexClearance <= 0.0)
	{
		return false;
	}

	OutIntent.SelectionId = RequestedSelectionId;
	OutIntent.HotbarSlotNumber = RequestedHotbarSlotNumber;
	OutIntent.Origin = RequestedOrigin;
	OutIntent.TrajectoryKind =
		Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::BallisticArc;
	OutIntent.Target = RequestedTarget;
	OutIntent.ApexClearance = RequestedApexClearance;
	if (!OutIntent.IsValid())
	{
		OutIntent = Fdemo_mapShanmenThrownWeaponHotbarIntent();
		return false;
	}
	return true;
}

bool Fdemo_mapShanmenThrownWeaponHotbarIntent::IsValid() const
{
	const bool bCommonValid = SelectionId.IsValid()
		&& HotbarSlotNumber >= 1
		&& HotbarSlotNumber
			<= Fdemo_mapPersistentPreparationLayout::HotbarSlotCount
		&& IsFiniteVector(Origin);
	if (!bCommonValid)
	{
		return false;
	}

	switch (TrajectoryKind)
	{
	case Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::Straight:
		return IsFiniteVector(AimDirection)
			&& AimDirection.IsNormalized()
			&& FMath::IsFinite(MaximumDistance)
			&& MaximumDistance > KINDA_SMALL_NUMBER
			&& Target == FVector::ZeroVector
			&& ApexClearance == 0.0;
	case Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::BallisticArc:
		return AimDirection == FVector::ZeroVector
			&& MaximumDistance == 0.0f
			&& IsFiniteVector(Target)
			&& !Origin.Equals(Target, UE_DOUBLE_SMALL_NUMBER)
			&& FMath::IsFinite(ApexClearance)
			&& ApexClearance > 0.0;
	default:
		return false;
	}
}

bool Fdemo_mapShanmenThrownWeaponHotbarIntent::Matches(
	const Fdemo_mapShanmenThrownWeaponHotbarIntent& Other) const
{
	return IsValid() && Other.IsValid()
		&& SelectionId == Other.SelectionId
		&& HotbarSlotNumber == Other.HotbarSlotNumber
		&& Origin == Other.Origin
		&& TrajectoryKind == Other.TrajectoryKind
		&& AimDirection == Other.AimDirection
		&& MaximumDistance == Other.MaximumDistance
		&& Target == Other.Target
		&& ApexClearance == Other.ApexClearance;
}

bool Fdemo_mapShanmenThrownWeaponSessionConfig::TryCapture(
	const FShanmenThrownWeaponDefinitionCapture& RequestedDefinition,
	const FGameplayTagContainer& RequestedSourceTags,
	Fdemo_mapShanmenThrownWeaponSessionConfig& OutConfig)
{
	OutConfig = Fdemo_mapShanmenThrownWeaponSessionConfig();
	Fdemo_mapShanmenThrownWeaponProductCapture Validation;
	if (!Fdemo_mapShanmenThrownWeaponProductCapture::TryCapture(
			RequestedDefinition,
			0.0f,
			RequestedSourceTags,
			Validation))
	{
		return false;
	}
	OutConfig.TrajectoryKind =
		Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::Straight;
	OutConfig.Definition = RequestedDefinition;
	OutConfig.SourceTags = RequestedSourceTags;
	return OutConfig.IsValid();
}

bool Fdemo_mapShanmenThrownWeaponSessionConfig::TryCaptureArc(
	const FShanmenThrownWeaponDefinitionCapture& RequestedDefinition,
	const FGameplayTagContainer& RequestedSourceTags,
	EShanmenThrownWeaponTechniqueTier RequestedTechniqueTier,
	double RequestedGravityMagnitude,
	double RequestedMaximumFlightTime,
	Fdemo_mapShanmenThrownWeaponSessionConfig& OutConfig)
{
	OutConfig = Fdemo_mapShanmenThrownWeaponSessionConfig();
	Fdemo_mapShanmenThrownWeaponProductCapture Validation;
	if (!Fdemo_mapShanmenThrownWeaponProductCapture::TryCaptureArc(
			RequestedDefinition,
			0.0f,
			RequestedSourceTags,
			RequestedTechniqueTier,
			RequestedGravityMagnitude,
			RequestedMaximumFlightTime,
			Validation))
	{
		return false;
	}
	OutConfig.TrajectoryKind =
		Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::BallisticArc;
	OutConfig.Definition = RequestedDefinition;
	OutConfig.SourceTags = RequestedSourceTags;
	OutConfig.ArcPolicy = Validation.GetArcPolicy();
	return OutConfig.IsValid();
}

bool Fdemo_mapShanmenThrownWeaponSessionConfig::IsValid() const
{
	Fdemo_mapShanmenThrownWeaponProductCapture Validation;
	switch (TrajectoryKind)
	{
	case Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::Straight:
		return !ArcPolicy.IsValid()
			&& Fdemo_mapShanmenThrownWeaponProductCapture::TryCapture(
				Definition, 0.0f, SourceTags, Validation);
	case Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::BallisticArc:
		return ArcPolicy.IsValid()
			&& Fdemo_mapShanmenThrownWeaponProductCapture::TryCaptureArc(
				Definition,
				0.0f,
				SourceTags,
				ArcPolicy.GetTechniqueTier(),
				ArcPolicy.GetGravityMagnitude(),
				ArcPolicy.GetMaximumFlightTime(),
				Validation);
	default:
		return false;
	}
}

bool Fdemo_mapShanmenThrownWeaponSessionConfig::Matches(
	const Fdemo_mapShanmenThrownWeaponSessionConfig& Other) const
{
	FShanmenThrownWeaponDefinition Left;
	FShanmenThrownWeaponDefinition Right;
	return FShanmenThrownWeaponDefinition::TryCapture(Definition, Left)
		&& FShanmenThrownWeaponDefinition::TryCapture(
			Other.Definition, Right)
		&& DefinitionsMatch(Left, Right)
		&& SourceTags == Other.SourceTags
		&& TrajectoryKind == Other.TrajectoryKind
		&& ((TrajectoryKind
				== Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::Straight
				&& !ArcPolicy.IsValid()
				&& !Other.ArcPolicy.IsValid())
			|| (TrajectoryKind
				== Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::BallisticArc
				&& ArcPolicy.Matches(Other.ArcPolicy)));
}

bool Fdemo_mapShanmenThrownWeaponSessionResult::IsAccepted() const
{
	return Status == Edemo_mapShanmenThrownWeaponSessionStatus::Applied
		&& SelectionId.IsValid()
		&& RunId.IsValid()
		&& HotbarSlotNumber >= 1
		&& ItemInstanceId.IsValid()
		&& FMath::IsFinite(TechniquePower)
		&& TechniquePower >= 0.0f
		&& Product.IsAccepted()
		&& Product.SelectionId == SelectionId
		&& Product.RunId == RunId
		&& Product.SourceItemInstanceId == ItemInstanceId;
}

bool Fdemo_mapShanmenThrownWeaponSessionResult::IsRecoveryApplied() const
{
	return Status
			== Edemo_mapShanmenThrownWeaponSessionStatus::RecoveryApplied
		&& SelectionId.IsValid()
		&& RunId.IsValid()
		&& HotbarSlotNumber >= 1
		&& ItemInstanceId.IsValid()
		&& Product.IsRecoveryApplied();
}

bool Fdemo_mapShanmenThrownWeaponProductSession::TryBegin(
	const Fdemo_mapShanmenRunCorrelation& RequestedCorrelation,
	AActor& RequestedSourceActor,
	const Fdemo_mapShanmenThrownWeaponSessionConfig& RequestedConfig,
	FString& OutDiagnostic)
{
	OutDiagnostic.Reset();
	if (bActive)
	{
		if (IsValid()
			&& Correlation == RequestedCorrelation
			&& SourceActor.Get() == &RequestedSourceActor
			&& Config.Matches(RequestedConfig))
		{
			OutDiagnostic = TEXT("Thrown-weapon product session is already bound to this exact Run.");
			return true;
		}
		OutDiagnostic =
			TEXT("An active thrown-weapon product session cannot change Run, source, or content.");
		return false;
	}
	if (!IsValid()
		|| !RequestedCorrelation.IsValid()
		|| !::IsValid(&RequestedSourceActor)
		|| !RequestedConfig.IsValid())
	{
		OutDiagnostic =
			TEXT("Thrown-weapon product session requires empty state and valid immutable inputs.");
		return false;
	}

	Correlation = RequestedCorrelation;
	SourceActor = &RequestedSourceActor;
	Config = RequestedConfig;
	bActive = true;
	if (!IsValid())
	{
		ClearBinding();
		OutDiagnostic =
			TEXT("Thrown-weapon product session failed closed during Run binding.");
		return false;
	}
	OutDiagnostic = TEXT("Thrown-weapon product session bound to active Run hotbar.");
	return true;
}

Fdemo_mapShanmenThrownWeaponSessionResult
Fdemo_mapShanmenThrownWeaponProductSession::TrySubmitHotbar(
	UWorld* World,
	TSubclassOf<Ademo_mapShanmenThrownWeaponProjectile> ProjectileClass,
	Udemo_mapShanmenItemAuthoritySubsystem& Authority,
	Fdemo_mapCombatRunCoordinator& Coordinator,
	const Fdemo_mapShanmenThrownWeaponHotbarIntent& Intent)
{
	if (!bActive)
	{
		return Reject(
			Edemo_mapShanmenThrownWeaponSessionStatus::SessionInactive,
			Intent,
			FGuid(),
			TEXT("Thrown-weapon hotbar submission requires one active product session."));
	}
	if (!IsValid())
	{
		return Reject(
			Edemo_mapShanmenThrownWeaponSessionStatus::SessionInvalid,
			Intent,
			Correlation.ActiveRunId,
			TEXT("Thrown-weapon product session invariants are invalid."));
	}
	if (!Intent.IsValid())
	{
		return Reject(
			Edemo_mapShanmenThrownWeaponSessionStatus::RequestInvalid,
			Intent,
			Correlation.ActiveRunId,
			TEXT("Thrown-weapon hotbar request is invalid."));
	}
	if (Intent.GetTrajectoryKind() != Config.GetTrajectoryKind())
	{
		return Reject(
			Edemo_mapShanmenThrownWeaponSessionStatus::TrajectoryMismatch,
			Intent,
			Correlation.ActiveRunId,
			TEXT("Hotbar trajectory and immutable session product disagree."));
	}
	if (!Coordinator.IsReady()
		|| Coordinator.GetRunId() != Correlation.ActiveRunId)
	{
		return Reject(
			Edemo_mapShanmenThrownWeaponSessionStatus::RunMismatch,
			Intent,
			Correlation.ActiveRunId,
			TEXT("Hotbar session and combat coordinator must name one active Run."));
	}
	if (!SourceActor.IsValid())
	{
		return Reject(
			Edemo_mapShanmenThrownWeaponSessionStatus::SourceUnavailable,
			Intent,
			Correlation.ActiveRunId,
			TEXT("The player source captured by this Run is unavailable."));
	}

	if (FCapturedSelection* Existing =
		CapturedSelections.Find(Intent.GetSelectionId()))
	{
		if (!Existing->HotbarIntent.Matches(Intent))
		{
			return Reject(
				Edemo_mapShanmenThrownWeaponSessionStatus::SelectionIdConflict,
				Intent,
				Correlation.ActiveRunId,
				TEXT("SelectionId was reused with another hotbar or trajectory payload."));
		}
		return RouteCaptured(
			World,
			ProjectileClass,
			Authority,
			Coordinator,
			*Existing,
			true);
	}

	const int32 SlotIndex = Intent.GetHotbarSlotNumber() - 1;
	const FGuid ItemId = Correlation.HotbarItemInstanceIds.IsValidIndex(SlotIndex)
		? Correlation.HotbarItemInstanceIds[SlotIndex]
		: FGuid();
	if (!ItemId.IsValid())
	{
		return Reject(
			Edemo_mapShanmenThrownWeaponSessionStatus::HotbarSlotEmpty,
			Intent,
			Correlation.ActiveRunId,
			TEXT("The frozen active-Run hotbar slot is empty."));
	}
	if (Host.IsTerminal() && !Host.Reset())
	{
		return Reject(
			Edemo_mapShanmenThrownWeaponSessionStatus::HostResetRejected,
			Intent,
			Correlation.ActiveRunId,
			TEXT("Terminal thrown-weapon Host could not reset for a new selection."));
	}

	FCapturedSelection Captured;
	Captured.HotbarIntent = Intent;
	bool bSelectionCaptured = false;
	if (Intent.GetTrajectoryKind()
		== Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::Straight)
	{
		bSelectionCaptured =
			Fdemo_mapShanmenThrownWeaponSelectionIntent::TryCapture(
				Intent.GetSelectionId(),
				Correlation.ActiveRunId,
				ItemId,
				Intent.GetOrigin(),
				Intent.GetAimDirection(),
				Intent.GetMaximumDistance(),
				Captured.ProductSelection);
	}
	else
	{
		bSelectionCaptured =
			Fdemo_mapShanmenThrownWeaponSelectionIntent::TryCaptureArc(
				Intent.GetSelectionId(),
				Correlation.ActiveRunId,
				ItemId,
				Intent.GetOrigin(),
				Intent.GetTarget(),
				Intent.GetApexClearance(),
				Captured.ProductSelection);
	}
	if (!bSelectionCaptured)
	{
		return Reject(
			Edemo_mapShanmenThrownWeaponSessionStatus::RequestInvalid,
			Intent,
			Correlation.ActiveRunId,
			TEXT("Hotbar request could not enter the exact-item product contract."));
	}
	const float TechniquePower =
		Fdemo_mapPlayerCombat::CaptureAttackPower(SourceActor.Get());
	bool bProductCaptured = false;
	if (Intent.GetTrajectoryKind()
		== Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::Straight)
	{
		bProductCaptured =
			Fdemo_mapShanmenThrownWeaponProductCapture::TryCapture(
				Config.GetDefinition(),
				TechniquePower,
				Config.GetSourceTags(),
				Captured.Product);
	}
	else
	{
		const Fdemo_mapShanmenThrownWeaponArcProductPolicy& ArcPolicy =
			Config.GetArcPolicy();
		bProductCaptured =
			Fdemo_mapShanmenThrownWeaponProductCapture::TryCaptureArc(
				Config.GetDefinition(),
				TechniquePower,
				Config.GetSourceTags(),
				ArcPolicy.GetTechniqueTier(),
				ArcPolicy.GetGravityMagnitude(),
				ArcPolicy.GetMaximumFlightTime(),
				Captured.Product);
	}
	if (!bProductCaptured)
	{
		Fdemo_mapShanmenThrownWeaponSessionResult Result = Reject(
			Edemo_mapShanmenThrownWeaponSessionStatus::ProductCaptureRejected,
			Intent,
			Correlation.ActiveRunId,
			TEXT("Current player combat values could not enter the thrown-weapon product capture."));
		Result.ItemInstanceId = ItemId;
		Result.TechniquePower = TechniquePower;
		return Result;
	}

	CapturedSelections.Add(Intent.GetSelectionId(), MoveTemp(Captured));
	FCapturedSelection& Stored =
		CapturedSelections.FindChecked(Intent.GetSelectionId());
	return RouteCaptured(
		World,
		ProjectileClass,
		Authority,
		Coordinator,
		Stored,
		false);
}

Fdemo_mapShanmenThrownWeaponSessionResult
Fdemo_mapShanmenThrownWeaponProductSession::RouteCaptured(
	UWorld* World,
	TSubclassOf<Ademo_mapShanmenThrownWeaponProjectile> ProjectileClass,
	Udemo_mapShanmenItemAuthoritySubsystem& Authority,
	Fdemo_mapCombatRunCoordinator& Coordinator,
	FCapturedSelection& Captured,
	bool bReusedSelection)
{
	if (!Captured.LastProductResult.IsAccepted()
		&& !Captured.LastProductResult.Command.IsDurableTerminal()
		&& Host.IsTerminal()
		&& !Host.Reset())
	{
		Fdemo_mapShanmenThrownWeaponSessionResult Result = Reject(
			Edemo_mapShanmenThrownWeaponSessionStatus::HostResetRejected,
			Captured.HotbarIntent,
			Correlation.ActiveRunId,
			TEXT("Terminal Host could not reset before retrying a transient selection."));
		Result.bReusedSelection = bReusedSelection;
		Result.ItemInstanceId =
			Captured.ProductSelection.GetSourceItemInstanceId();
		Result.TechniquePower = Captured.Product.GetOffense().GetTechniquePower();
		return Result;
	}

	Captured.LastProductResult = Controller.TrySubmit(
		Host,
		Router,
		World,
		ProjectileClass,
		Authority,
		Coordinator,
		SourceActor.Get(),
		Correlation,
		Captured.ProductSelection,
		Captured.Product);

	Fdemo_mapShanmenThrownWeaponSessionResult Result;
	Result.Status = Captured.LastProductResult.IsAccepted()
		? Edemo_mapShanmenThrownWeaponSessionStatus::Applied
		: Edemo_mapShanmenThrownWeaponSessionStatus::ProductRejected;
	Result.bReusedSelection = bReusedSelection;
	Result.SelectionId = Captured.HotbarIntent.GetSelectionId();
	Result.RunId = Correlation.ActiveRunId;
	Result.HotbarSlotNumber = Captured.HotbarIntent.GetHotbarSlotNumber();
	Result.ItemInstanceId =
		Captured.ProductSelection.GetSourceItemInstanceId();
	Result.TechniquePower = Captured.Product.GetOffense().GetTechniquePower();
	Result.Product = Captured.LastProductResult;
	Result.Diagnostic = Captured.LastProductResult.Diagnostic;
	if (!IsValid())
	{
		Result.Status =
			Edemo_mapShanmenThrownWeaponSessionStatus::SessionInvalid;
		Result.Diagnostic =
			TEXT("Thrown-weapon product session failed post-route invariants.");
	}
	return Result;
}

Fdemo_mapShanmenThrownWeaponSessionResult
Fdemo_mapShanmenThrownWeaponProductSession::TryRecoverCancellation(
	Udemo_mapShanmenItemAuthoritySubsystem& Authority,
	const Fdemo_mapShanmenThrownWeaponHotbarIntent& Intent)
{
	if (!bActive || !IsValid())
	{
		return Reject(
			bActive
				? Edemo_mapShanmenThrownWeaponSessionStatus::SessionInvalid
				: Edemo_mapShanmenThrownWeaponSessionStatus::SessionInactive,
			Intent,
			Correlation.ActiveRunId,
			TEXT("Cancellation recovery requires one valid active product session."));
	}
	if (!Intent.IsValid())
	{
		return Reject(
			Edemo_mapShanmenThrownWeaponSessionStatus::RequestInvalid,
			Intent,
			Correlation.ActiveRunId,
			TEXT("Cancellation recovery requires one valid hotbar request."));
	}
	FCapturedSelection* Existing =
		CapturedSelections.Find(Intent.GetSelectionId());
	if (!Existing)
	{
		return Reject(
			Edemo_mapShanmenThrownWeaponSessionStatus::SelectionNotFound,
			Intent,
			Correlation.ActiveRunId,
			TEXT("No captured hotbar selection exists for recovery."));
	}
	if (!Existing->HotbarIntent.Matches(Intent))
	{
		return Reject(
			Edemo_mapShanmenThrownWeaponSessionStatus::SelectionIdConflict,
			Intent,
			Correlation.ActiveRunId,
			TEXT("Cancellation recovery rejected a conflicting hotbar payload."));
	}

	Existing->LastProductResult = Controller.TryRecoverCancellation(
		Authority, Router, Existing->ProductSelection);
	Fdemo_mapShanmenThrownWeaponSessionResult Result;
	Result.Status = Existing->LastProductResult.IsRecoveryApplied()
		? Edemo_mapShanmenThrownWeaponSessionStatus::RecoveryApplied
		: Edemo_mapShanmenThrownWeaponSessionStatus::ProductRejected;
	Result.bReusedSelection = true;
	Result.SelectionId = Intent.GetSelectionId();
	Result.RunId = Correlation.ActiveRunId;
	Result.HotbarSlotNumber = Intent.GetHotbarSlotNumber();
	Result.ItemInstanceId =
		Existing->ProductSelection.GetSourceItemInstanceId();
	Result.TechniquePower = Existing->Product.GetOffense().GetTechniquePower();
	Result.Product = Existing->LastProductResult;
	Result.Diagnostic = Existing->LastProductResult.Diagnostic;
	return Result;
}

bool Fdemo_mapShanmenThrownWeaponProductSession::TryInterruptFlight()
{
	return bActive && IsValid() && Host.TryInterrupt();
}

bool Fdemo_mapShanmenThrownWeaponProductSession::TryExpireRange()
{
	return bActive && IsValid() && Host.TryExpireRange();
}

bool Fdemo_mapShanmenThrownWeaponProductSession::TryEnd(
	FString& OutDiagnostic)
{
	OutDiagnostic.Reset();
	if (!bActive || !IsValid())
	{
		OutDiagnostic = TEXT("Only a valid active thrown-weapon product session may end.");
		return false;
	}
	if (Host.IsInFlight())
	{
		OutDiagnostic = TEXT("An in-flight thrown weapon must become terminal before session end.");
		return false;
	}
	for (const TPair<FGuid, FCapturedSelection>& Pair : CapturedSelections)
	{
		if (Pair.Value.LastProductResult.Command.RequiresRecovery())
		{
			OutDiagnostic = TEXT("Unresolved durable cancellation or post-commit recovery blocks session end.");
			return false;
		}
	}
	if (!Host.Reset())
	{
		OutDiagnostic = TEXT("Thrown-weapon Host could not reset during session end.");
		return false;
	}
	Controller.Reset();
	Router.Reset();
	CapturedSelections.Reset();
	ClearBinding();
	if (!IsValid())
	{
		OutDiagnostic = TEXT("Thrown-weapon product session failed empty-state validation.");
		return false;
	}
	OutDiagnostic = TEXT("Thrown-weapon product session ended without hidden flight or recovery work.");
	return true;
}

bool Fdemo_mapShanmenThrownWeaponProductSession::IsValid() const
{
	const bool bHostValid = Host.GetState()
			== Edemo_mapShanmenThrownWeaponHostState::Empty
		|| Host.IsValid();
	if (!bActive)
	{
		return !Correlation.IsValid()
			&& !SourceActor.IsValid()
			&& !Config.IsValid()
			&& CapturedSelections.IsEmpty()
			&& Controller.IsEmpty()
			&& Router.IsEmpty()
			&& Host.GetState()
				== Edemo_mapShanmenThrownWeaponHostState::Empty;
	}
	if (!Correlation.IsValid()
		|| !SourceActor.IsValid()
		|| !Config.IsValid()
		|| !bHostValid
		|| !Controller.IsValid()
		|| !Router.IsValid()
		|| (!Controller.IsEmpty()
			&& Controller.GetRunId() != Correlation.ActiveRunId)
		|| (!Router.IsEmpty()
			&& Router.GetRunId() != Correlation.ActiveRunId))
	{
		return false;
	}

	for (const TPair<FGuid, FCapturedSelection>& Pair : CapturedSelections)
	{
		const FCapturedSelection& Captured = Pair.Value;
		const int32 SlotIndex =
			Captured.HotbarIntent.GetHotbarSlotNumber() - 1;
		Fdemo_mapShanmenThrownWeaponProductCapture ExpectedProduct;
		bool bTrajectoryValid = false;
		if (Captured.HotbarIntent.GetTrajectoryKind()
			== Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::Straight)
		{
			bTrajectoryValid = Config.GetTrajectoryKind()
					== Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::Straight
				&& Captured.ProductSelection.GetTrajectoryKind()
					== Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::Straight
				&& Captured.ProductSelection.GetOrigin()
					== Captured.HotbarIntent.GetOrigin()
				&& Captured.ProductSelection.GetAimDirection()
					== Captured.HotbarIntent.GetAimDirection()
				&& Captured.ProductSelection.GetMaximumDistance()
					== Captured.HotbarIntent.GetMaximumDistance()
				&& Fdemo_mapShanmenThrownWeaponProductCapture::TryCapture(
					Config.GetDefinition(),
					Captured.Product.GetOffense().GetTechniquePower(),
					Config.GetSourceTags(),
					ExpectedProduct);
		}
		else if (Captured.HotbarIntent.GetTrajectoryKind()
			== Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::BallisticArc)
		{
			const Fdemo_mapShanmenThrownWeaponArcProductPolicy& ArcPolicy =
				Config.GetArcPolicy();
			bTrajectoryValid = Config.GetTrajectoryKind()
					== Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::BallisticArc
				&& Captured.ProductSelection.GetTrajectoryKind()
					== Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::BallisticArc
				&& Captured.ProductSelection.GetOrigin()
					== Captured.HotbarIntent.GetOrigin()
				&& Captured.ProductSelection.GetTarget()
					== Captured.HotbarIntent.GetTarget()
				&& Captured.ProductSelection.GetApexClearance()
					== Captured.HotbarIntent.GetApexClearance()
				&& Fdemo_mapShanmenThrownWeaponProductCapture::TryCaptureArc(
					Config.GetDefinition(),
					Captured.Product.GetOffense().GetTechniquePower(),
					Config.GetSourceTags(),
					ArcPolicy.GetTechniqueTier(),
					ArcPolicy.GetGravityMagnitude(),
					ArcPolicy.GetMaximumFlightTime(),
					ExpectedProduct);
		}
		if (Pair.Key != Captured.HotbarIntent.GetSelectionId()
			|| !Captured.HotbarIntent.IsValid()
			|| !Captured.ProductSelection.IsValid()
			|| !Captured.Product.IsValid()
			|| !bTrajectoryValid
			|| !Correlation.HotbarItemInstanceIds.IsValidIndex(SlotIndex)
			|| Correlation.HotbarItemInstanceIds[SlotIndex]
				!= Captured.ProductSelection.GetSourceItemInstanceId()
			|| Captured.ProductSelection.GetSelectionId() != Pair.Key
			|| Captured.ProductSelection.GetRunId()
				!= Correlation.ActiveRunId
			|| !Captured.Product.Matches(ExpectedProduct)
			|| Captured.LastProductResult.SelectionId != Pair.Key
			|| Captured.LastProductResult.RunId
				!= Correlation.ActiveRunId
			|| Captured.LastProductResult.SourceItemInstanceId
				!= Captured.ProductSelection.GetSourceItemInstanceId())
		{
			return false;
		}
		if (Captured.LastProductResult.HasCapturedAction())
		{
			const Fdemo_mapShanmenThrownWeaponRunCommandIntent* Command =
				Controller.FindCapturedCommand(Pair.Key);
			const bool bArcPlanRejected =
				Captured.LastProductResult.Status
					== Edemo_mapShanmenThrownWeaponProductStatus::ArcPlanRejected;
			if ((bArcPlanRejected && Command)
				|| (!bArcPlanRejected
					&& (!Command
						|| Command->GetAction().GetSourceItemInstanceId()
							!= Captured.ProductSelection.GetSourceItemInstanceId()
						|| Command->GetOffense().GetTechniquePower()
							!= Captured.Product.GetOffense().GetTechniquePower())))
			{
				return false;
			}
		}
	}
	return true;
}

const Fdemo_mapShanmenThrownWeaponRunCommandIntent*
Fdemo_mapShanmenThrownWeaponProductSession::FindCapturedCommand(
	const FGuid& SelectionId) const
{
	return bActive && IsValid()
		? Controller.FindCapturedCommand(SelectionId)
		: nullptr;
}

void Fdemo_mapShanmenThrownWeaponProductSession::ClearBinding()
{
	bActive = false;
	Correlation = Fdemo_mapShanmenRunCorrelation();
	SourceActor.Reset();
	Config = Fdemo_mapShanmenThrownWeaponSessionConfig();
}
