#include "demo_mapShanmenThrownWeaponLaunchRejectionPresentation.h"

namespace
{
	using EKind = Edemo_mapShanmenThrownWeaponLaunchRejectionKind;
	using ECommand = Edemo_mapShanmenThrownWeaponRunCommandStatus;
	using FPresentation =
		Fdemo_mapShanmenThrownWeaponLaunchRejectionPresentation;

	const TCHAR* BuildDisplayText(const EKind Kind)
	{
		return Kind == EKind::ReleasePathBlocked
			? TEXT("飞刀 · 释放路径受阻")
			: TEXT("");
	}
}

bool FPresentation::TryProject(
	const Fdemo_mapShanmenThrownWeaponSessionResult& Session,
	FPresentation& OutPresentation)
{
	OutPresentation = FPresentation();
	const Fdemo_mapShanmenThrownWeaponProductResult& Product =
		Session.Product;
	const Fdemo_mapShanmenThrownWeaponRunCommandResult& Command =
		Product.Command;
	const bool bCanonicalTerminal = Command.Status
			== ECommand::LaunchRejectedCancelled
		|| Command.Status == ECommand::RecoveryRequired;
	if (Session.Status
			!= Edemo_mapShanmenThrownWeaponSessionStatus::ProductRejected
		|| Session.Diagnostic.IsEmpty()
		|| !Session.SelectionId.IsValid()
		|| !Session.RunId.IsValid()
		|| !Session.ItemInstanceId.IsValid()
		|| Session.HotbarSlotNumber < 1
		|| Product.Status
			!= Edemo_mapShanmenThrownWeaponProductStatus::RouterRejected
		|| !Product.HasCapturedAction()
		|| Product.SelectionId != Session.SelectionId
		|| Product.RunId != Session.RunId
		|| Product.SourceItemInstanceId != Session.ItemInstanceId
		|| Command.IntentId != Product.ActivationId
		|| Command.RunId != Session.RunId
		|| Command.ItemInstanceId != Session.ItemInstanceId
		|| !bCanonicalTerminal
		|| Command.HostStart.Error
			!= Edemo_mapShanmenThrownWeaponHostStartError::LaunchRejected
		|| Command.HostStart.Launch.Error
			!= Edemo_mapShanmenThrownWeaponLaunchError::ReleasePathBlocked
		|| Command.HostStart.IsStarted())
	{
		return false;
	}

	FPresentation Candidate;
	Candidate.Kind = EKind::ReleasePathBlocked;
	Candidate.RunId = Session.RunId;
	Candidate.SelectionId = Session.SelectionId;
	Candidate.bRequiresCancellationRecovery = Command.RequiresRecovery();
	Candidate.DisplayText = BuildDisplayText(Candidate.Kind);
	if (!Candidate.IsValid())
	{
		return false;
	}

	OutPresentation = MoveTemp(Candidate);
	return true;
}

bool FPresentation::IsValid() const
{
	return Kind == EKind::ReleasePathBlocked
		&& RunId.IsValid()
		&& SelectionId.IsValid()
		&& DisplayText == BuildDisplayText(Kind);
}

bool FPresentation::Matches(const FPresentation& Other) const
{
	return IsValid() && Other.IsValid()
		&& Kind == Other.Kind
		&& RunId == Other.RunId
		&& SelectionId == Other.SelectionId
		&& bRequiresCancellationRecovery
			== Other.bRequiresCancellationRecovery
		&& DisplayText == Other.DisplayText;
}
