#include "demo_mapShanmenThrownWeaponProductLifecycle.h"

#include "ShanmenCombatTags.h"
#include "demo_mapCombatRunCoordinator.h"
#include "demo_mapItemDefinitions.h"
#include "demo_mapShanmenItemAuthoritySubsystem.h"
#include "demo_mapShanmenRunLifecycleAdapter.h"

bool Fdemo_mapShanmenThrownWeaponProductLifecycle::
	TryCaptureTrainingThrowingKnifeConfig(
		Fdemo_mapShanmenThrownWeaponSessionConfig& OutConfig,
		FString& OutDiagnostic)
{
	return TryCaptureTrainingThrowingKnifeConfig(
		Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::Straight,
		OutConfig,
		OutDiagnostic);
}

bool Fdemo_mapShanmenThrownWeaponProductLifecycle::
	TryCaptureTrainingThrowingKnifeConfig(
		Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind RequestedTrajectoryKind,
		Fdemo_mapShanmenThrownWeaponSessionConfig& OutConfig,
		FString& OutDiagnostic)
{
	OutConfig = Fdemo_mapShanmenThrownWeaponSessionConfig();
	OutDiagnostic.Reset();
	if (RequestedTrajectoryKind
			!= Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::Straight
		&& RequestedTrajectoryKind
			!= Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::BallisticArc)
	{
		OutDiagnostic =
			TEXT("TrainingThrowingKnife lifecycle content requires explicit Straight or Arc trajectory.");
		return false;
	}
	const Fdemo_mapItemDefinition* Product =
		Fdemo_mapItemDefinitions::Find(
			Fdemo_mapItemIds::TrainingThrowingKnife);
	if (!Product
		|| !Product->HasGameplaySemantic(
			Edemo_mapItemGameplaySemantic::ThrownWeapon))
	{
		OutDiagnostic =
			TEXT("TrainingThrowingKnife is absent or lacks the canonical thrown-weapon semantic.");
		return false;
	}

	FShanmenThrownWeaponDefinitionCapture Definition;
	if (RequestedTrajectoryKind
		== Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::Straight)
	{
		Definition.ActionDefinitionId =
			FShanmenThrownWeaponDefinition::CanonicalActionDefinitionId();
		Definition.DetectorId =
			TEXT("Detector.ThrownWeapon.TrainingThrowingKnife.Straight");
	}
	else
	{
		Definition.ActionDefinitionId =
			FShanmenThrownWeaponDefinition::ArcActionDefinitionId();
		Definition.DetectorId =
			TEXT("Detector.ThrownWeapon.TrainingThrowingKnife.Arc");
	}
	Definition.FormulaId =
		TEXT("Combat.Formula.ThrownWeapon.TrainingThrowingKnife.r1");
	Definition.BaseDamage = 12.0f;
	Definition.TechniquePowerCoefficient = 0.3f;
	Definition.LaunchSpeed = 900.0f;
	Definition.DamageTags.AddTag(
		FShanmenCombatNativeTags::DamagePhysicalSlash());
	Definition.RequiredTargetTags.AddTag(
		FShanmenCombatNativeTags::TargetLiving());
	Definition.bRejectSelf = true;

	FGameplayTagContainer SourceTags;
	SourceTags.AddTag(FShanmenCombatNativeTags::SourcePlayer());
	const bool bCaptured = RequestedTrajectoryKind
			== Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::Straight
		? Fdemo_mapShanmenThrownWeaponSessionConfig::TryCapture(
			Definition, SourceTags, OutConfig)
		: Fdemo_mapShanmenThrownWeaponSessionConfig::TryCaptureArc(
			Definition,
			SourceTags,
			EShanmenThrownWeaponTechniqueTier::Intermediate,
			980.0,
			4.0,
			OutConfig);
	if (!bCaptured)
	{
		OutDiagnostic =
			TEXT("TrainingThrowingKnife combat policy failed immutable capture.");
		return false;
	}
	OutDiagnostic =
		TEXT("TrainingThrowingKnife typed product policy captured from canonical content identity.");
	return true;
}

bool Fdemo_mapShanmenThrownWeaponProductLifecycle::TryBegin(
	Udemo_mapShanmenItemAuthoritySubsystem& Authority,
	AActor& SourceActor,
	Fdemo_mapCombatRunCoordinator& Coordinator,
	FString& OutDiagnostic)
{
	return TryBegin(
		Authority,
		SourceActor,
		Coordinator,
		Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::Straight,
		OutDiagnostic);
}

bool Fdemo_mapShanmenThrownWeaponProductLifecycle::TryBegin(
	Udemo_mapShanmenItemAuthoritySubsystem& Authority,
	AActor& SourceActor,
	Fdemo_mapCombatRunCoordinator& Coordinator,
	Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind RequestedTrajectoryKind,
	FString& OutDiagnostic)
{
	OutDiagnostic.Reset();
	if (Session.IsActive()
		&& BoundAuthority.Get() != &Authority)
	{
		OutDiagnostic =
			TEXT("An active thrown-weapon lifecycle cannot switch item authority.");
		return false;
	}
	if (!Coordinator.IsReady())
	{
		OutDiagnostic =
			TEXT("Thrown-weapon lifecycle requires one ready combat Run coordinator.");
		return false;
	}

	Fdemo_mapShanmenRunCorrelation Correlation;
	if (!Fdemo_mapShanmenRunLifecycleAdapter::TryGetActiveRunCorrelation(
			Authority, Correlation, &OutDiagnostic))
	{
		return false;
	}
	if (Correlation.ActiveRunId != Coordinator.GetRunId())
	{
		OutDiagnostic =
			TEXT("Thrown-weapon authority and combat coordinator name different Runs.");
		return false;
	}
	FGuid SourceEntityId;
	if (!Coordinator.GetEntityRegistry().TryResolveObject(
			Coordinator.GetRunId(), &SourceActor, INDEX_NONE, SourceEntityId)
		|| SourceEntityId != Coordinator.GetPlayerEntityId())
	{
		OutDiagnostic =
			TEXT("Thrown-weapon source Actor is not the canonical player entity for this Run.");
		return false;
	}

	Fdemo_mapShanmenThrownWeaponSessionConfig Config;
	if (!TryCaptureTrainingThrowingKnifeConfig(
			RequestedTrajectoryKind, Config, OutDiagnostic)
		|| !Session.TryBegin(
			Correlation, SourceActor, Config, OutDiagnostic))
	{
		return false;
	}
	BoundAuthority = &Authority;
	if (!IsValid())
	{
		FString CleanupDiagnostic;
		Session.TryEnd(CleanupDiagnostic);
		BoundAuthority.Reset();
		OutDiagnostic =
			TEXT("Thrown-weapon lifecycle failed closed after product binding.");
		return false;
	}
	OutDiagnostic =
		TEXT("Thrown-weapon product lifecycle bound to durable active Run and player source.");
	return true;
}

Fdemo_mapShanmenThrownWeaponSessionResult
Fdemo_mapShanmenThrownWeaponProductLifecycle::TrySubmitHotbar(
	UWorld* World,
	TSubclassOf<Ademo_mapShanmenThrownWeaponProjectile> ProjectileClass,
	Fdemo_mapCombatRunCoordinator& Coordinator,
	const Fdemo_mapShanmenThrownWeaponHotbarIntent& Intent)
{
	if (!IsValid() || !Session.IsActive() || !BoundAuthority.IsValid())
	{
		return RejectUnavailable(
			Intent,
			TEXT("Thrown-weapon hotbar routing requires one valid product lifecycle."));
	}
	return Session.TrySubmitHotbar(
		World,
		ProjectileClass,
		*BoundAuthority.Get(),
		Coordinator,
		Intent);
}

Fdemo_mapShanmenThrownWeaponSessionResult
Fdemo_mapShanmenThrownWeaponProductLifecycle::TryRecoverCancellation(
	const Fdemo_mapShanmenThrownWeaponHotbarIntent& Intent)
{
	if (!IsValid() || !Session.IsActive() || !BoundAuthority.IsValid())
	{
		return RejectUnavailable(
			Intent,
			TEXT("Thrown-weapon cancellation recovery requires one valid product lifecycle."));
	}
	return Session.TryRecoverCancellation(*BoundAuthority.Get(), Intent);
}

bool Fdemo_mapShanmenThrownWeaponProductLifecycle::TryInterruptFlight()
{
	return IsValid() && Session.TryInterruptFlight();
}

bool Fdemo_mapShanmenThrownWeaponProductLifecycle::TryExpireRange()
{
	return IsValid() && Session.TryExpireRange();
}

bool Fdemo_mapShanmenThrownWeaponProductLifecycle::TryEnd(
	FString& OutDiagnostic)
{
	OutDiagnostic.Reset();
	if (!Session.IsActive())
	{
		if (!IsValid())
		{
			OutDiagnostic =
				TEXT("Inactive thrown-weapon lifecycle retains inconsistent authority state.");
			return false;
		}
		OutDiagnostic = TEXT("Thrown-weapon product lifecycle is already empty.");
		return true;
	}
	if (!IsValid())
	{
		OutDiagnostic =
			TEXT("Invalid thrown-weapon lifecycle cannot discard captured product state.");
		return false;
	}
	if (Session.GetHostState()
			== Edemo_mapShanmenThrownWeaponHostState::InFlight
		&& !Session.TryInterruptFlight())
	{
		OutDiagnostic =
			TEXT("Active thrown-weapon flight rejected lifecycle interruption.");
		return false;
	}
	if (!Session.TryEnd(OutDiagnostic))
	{
		return false;
	}
	BoundAuthority.Reset();
	if (!IsValid())
	{
		OutDiagnostic =
			TEXT("Thrown-weapon lifecycle failed empty-state validation after end.");
		return false;
	}
	OutDiagnostic =
		TEXT("Thrown-weapon product lifecycle ended without polling or hidden recovery loss.");
	return true;
}

bool Fdemo_mapShanmenThrownWeaponProductLifecycle::IsValid() const
{
	return Session.IsValid()
		&& (Session.IsActive() == BoundAuthority.IsValid());
}

Fdemo_mapShanmenThrownWeaponSessionResult
Fdemo_mapShanmenThrownWeaponProductLifecycle::RejectUnavailable(
	const Fdemo_mapShanmenThrownWeaponHotbarIntent& Intent,
	const TCHAR* Diagnostic) const
{
	Fdemo_mapShanmenThrownWeaponSessionResult Result;
	Result.Status = Session.IsActive()
		? Edemo_mapShanmenThrownWeaponSessionStatus::SessionInvalid
		: Edemo_mapShanmenThrownWeaponSessionStatus::SessionInactive;
	Result.SelectionId = Intent.GetSelectionId();
	Result.RunId = Session.IsActive() ? Session.GetRunId() : FGuid();
	Result.HotbarSlotNumber = Intent.GetHotbarSlotNumber();
	Result.Diagnostic = Diagnostic;
	return Result;
}
