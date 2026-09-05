#include "demo_mapShanmenThrownWeaponArcPreviewCapturePolicy.h"

#include "ShanmenCombatResolver.h"
#include "ShanmenCombatTags.h"
#include "ShanmenDeterministicId.h"

namespace
{
	using EStatus = Edemo_mapShanmenThrownWeaponArcPreviewCaptureStatus;

	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	FString FloatBits(float Value)
	{
		Value = Value == 0.0f ? 0.0f : Value;
		uint32 Bits = 0;
		static_assert(sizeof(Bits) == sizeof(Value));
		FMemory::Memcpy(&Bits, &Value, sizeof(Bits));
		return FString::Printf(TEXT("%08X"), Bits);
	}

	FString DoubleBits(double Value)
	{
		Value = Value == 0.0 ? 0.0 : Value;
		uint64 Bits = 0;
		static_assert(sizeof(Bits) == sizeof(Value));
		FMemory::Memcpy(&Bits, &Value, sizeof(Bits));
		return FString::Printf(TEXT("%016llX"), Bits);
	}

	void AppendCanonicalTags(
		TArray<FString>& Parts,
		const TCHAR* Prefix,
		const FGameplayTagContainer& Container)
	{
		TArray<FGameplayTag> Tags;
		Container.GetGameplayTagArray(Tags);
		Tags.Sort([](const FGameplayTag& Left, const FGameplayTag& Right)
		{
			return Left.ToString() < Right.ToString();
		});
		Parts.Add(FString::Printf(TEXT("%s.Count=%d"), Prefix, Tags.Num()));
		for (const FGameplayTag& Tag : Tags)
		{
			Parts.Add(FString::Printf(TEXT("%s.%s"), Prefix, *Tag.ToString()));
		}
	}

	bool ActionsMatch(
		const FShanmenCombatActionSnapshot& Left,
		const FShanmenCombatActionSnapshot& Right)
	{
		return Left.IsValid()
			&& Right.IsValid()
			&& Left.GetRunId() == Right.GetRunId()
			&& Left.GetOwnerId() == Right.GetOwnerId()
			&& Left.GetActivationId() == Right.GetActivationId()
			&& Left.GetSourceEntityId() == Right.GetSourceEntityId()
			&& Left.GetSourceItemInstanceId()
				== Right.GetSourceItemInstanceId()
			&& Left.GetActionDefinitionId()
				== Right.GetActionDefinitionId()
			&& Left.GetContent().Version == Right.GetContent().Version
			&& Left.GetContent().Digest == Right.GetContent().Digest
			&& Left.GetSourceTags() == Right.GetSourceTags();
	}

	FGuid MakePreviewActivationId(
		const Fdemo_mapShanmenThrownWeaponArcPreviewCaptureRequest& Request)
	{
		if (!Request.IsValid())
		{
			return FGuid();
		}

		const Fdemo_mapShanmenThrownWeaponSessionConfig& Session =
			Request.GetSessionConfig();
		const FShanmenThrownWeaponDefinitionCapture& Definition =
			Session.GetDefinition();
		const Fdemo_mapShanmenThrownWeaponArcProductPolicy& ArcPolicy =
			Session.GetArcPolicy();
		TArray<FString> Parts = {
			GuidDigits(Request.GetPreviewRequestId()),
			GuidDigits(Request.GetRunId()),
			GuidDigits(Request.GetPlayerEntityId()),
			GuidDigits(Request.GetSourceItemInstanceId()),
			FString::Printf(
				TEXT("%llu"),
				static_cast<unsigned long long>(
					Request.GetObservedNextActivationSequence())),
			Request.GetAuthorityContent().Version.ToString(),
			Request.GetAuthorityContent().Digest,
			Definition.ActionDefinitionId.ToString(),
			Definition.DetectorId.ToString(),
			Definition.FormulaId.ToString(),
			FloatBits(Definition.BaseDamage),
			FloatBits(Definition.TechniquePowerCoefficient),
			FloatBits(Definition.LaunchSpeed),
			Definition.bRejectSelf ? TEXT("RejectSelf=1") : TEXT("RejectSelf=0"),
			FString::FromInt(static_cast<int32>(ArcPolicy.GetTechniqueTier())),
			DoubleBits(ArcPolicy.GetGravityMagnitude()),
			DoubleBits(ArcPolicy.GetMaximumFlightTime()),
			GuidDigits(Request.GetChoicePolicy().GetPolicyId()),
			FString::FromInt(Request.GetSegmentCount())
		};
		AppendCanonicalTags(Parts, TEXT("Damage"), Definition.DamageTags);
		AppendCanonicalTags(
			Parts, TEXT("RequiredTarget"), Definition.RequiredTargetTags);
		AppendCanonicalTags(Parts, TEXT("Source"), Session.GetSourceTags());
		return FShanmenDeterministicId::FromCanonicalParts(
			FName(TEXT("demo_map.ShanmenThrownWeapon.ArcPreviewAction.r1")),
			Parts);
	}

	FGuid MakeProspectiveRealActivationId(
		const Fdemo_mapShanmenThrownWeaponArcPreviewCaptureRequest& Request)
	{
		return Request.IsValid()
			? FShanmenCombatIdFactory::MakeActivationId(
				Request.GetRunId(),
				Request.GetPlayerEntityId(),
				FShanmenThrownWeaponDefinition::ArcActionDefinitionId(),
				Request.GetObservedNextActivationSequence())
			: FGuid();
	}

	bool TryBuildPreviewAction(
		const Fdemo_mapShanmenThrownWeaponArcPreviewCaptureRequest& Request,
		const FGuid& PreviewActivationId,
		FShanmenCombatActionSnapshot& OutAction)
	{
		OutAction = FShanmenCombatActionSnapshot();
		if (!Request.IsValid() || !PreviewActivationId.IsValid())
		{
			return false;
		}

		FShanmenCombatActionCapture Capture;
		Capture.RunId = Request.GetRunId();
		Capture.OwnerId = Request.GetPlayerEntityId();
		Capture.ActivationId = PreviewActivationId;
		Capture.SourceEntityId = Request.GetPlayerEntityId();
		Capture.SourceItemInstanceId = Request.GetSourceItemInstanceId();
		Capture.ActionDefinitionId =
			FShanmenThrownWeaponDefinition::ArcActionDefinitionId();
		Capture.Content = Request.GetAuthorityContent();
		Capture.SourceTags = Request.GetSessionConfig().GetSourceTags();
		Capture.SourceTags.AddTag(FShanmenCombatNativeTags::SourcePlayer());
		return FShanmenCombatActionSnapshot::TryCapture(Capture, OutAction);
	}

	bool TryBuildConfiguration(
		const Fdemo_mapShanmenThrownWeaponArcPreviewCaptureRequest& Request,
		const FShanmenCombatActionSnapshot& PreviewAction,
		Fdemo_mapShanmenThrownWeaponArcPreviewConfiguration& OutConfiguration)
	{
		OutConfiguration =
			Fdemo_mapShanmenThrownWeaponArcPreviewConfiguration();
		FShanmenThrownWeaponDefinition Definition;
		const Fdemo_mapShanmenThrownWeaponSessionConfig& Session =
			Request.GetSessionConfig();
		return FShanmenThrownWeaponDefinition::TryCapture(
				Session.GetDefinition(), Definition)
			&& Fdemo_mapShanmenThrownWeaponArcPreviewConfiguration::TryCapture(
				PreviewAction,
				Definition,
				Session.GetArcPolicy(),
				Request.GetChoicePolicy(),
				Request.GetSegmentCount(),
				OutConfiguration);
	}
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewCaptureRequest::TryCapture(
	const FGuid& InPreviewRequestId,
	const FGuid& InRunId,
	const FGuid& InPlayerEntityId,
	const FGuid& InSourceItemInstanceId,
	const FShanmenContentStamp& InAuthorityContent,
	const uint64 InObservedNextActivationSequence,
	const Fdemo_mapShanmenThrownWeaponSessionConfig& InSessionConfig,
	const Fdemo_mapShanmenThrownWeaponArcChoicePolicy& InChoicePolicy,
	const int32 InSegmentCount,
	Fdemo_mapShanmenThrownWeaponArcPreviewCaptureRequest& OutRequest)
{
	OutRequest = Fdemo_mapShanmenThrownWeaponArcPreviewCaptureRequest();
	OutRequest.PreviewRequestId = InPreviewRequestId;
	OutRequest.RunId = InRunId;
	OutRequest.PlayerEntityId = InPlayerEntityId;
	OutRequest.SourceItemInstanceId = InSourceItemInstanceId;
	OutRequest.AuthorityContent = InAuthorityContent;
	OutRequest.ObservedNextActivationSequence =
		InObservedNextActivationSequence;
	OutRequest.SessionConfig = InSessionConfig;
	OutRequest.ChoicePolicy = InChoicePolicy;
	OutRequest.SegmentCount = InSegmentCount;
	if (!OutRequest.IsValid())
	{
		OutRequest = Fdemo_mapShanmenThrownWeaponArcPreviewCaptureRequest();
		return false;
	}
	return true;
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewCaptureRequest::IsValid() const
{
	return PreviewRequestId.IsValid()
		&& RunId.IsValid()
		&& PlayerEntityId.IsValid()
		&& SourceItemInstanceId.IsValid()
		&& AuthorityContent.IsValid()
		&& ObservedNextActivationSequence > 0
		&& ObservedNextActivationSequence < MAX_uint64
		&& SessionConfig.IsValid()
		&& SessionConfig.GetTrajectoryKind()
			== Edemo_mapShanmenThrownWeaponRunCommandTrajectoryKind::BallisticArc
		&& SessionConfig.GetDefinition().ActionDefinitionId
			== FShanmenThrownWeaponDefinition::ArcActionDefinitionId()
		&& ChoicePolicy.IsValid()
		&& SegmentCount
			>= FShanmenThrownWeaponArcPreviewSampler::MinimumSegmentCount
		&& SegmentCount
			<= FShanmenThrownWeaponArcPreviewSampler::MaximumSegmentCount;
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewCaptureRequest::Matches(
	const Fdemo_mapShanmenThrownWeaponArcPreviewCaptureRequest& Other) const
{
	return IsValid()
		&& Other.IsValid()
		&& PreviewRequestId == Other.PreviewRequestId
		&& RunId == Other.RunId
		&& PlayerEntityId == Other.PlayerEntityId
		&& SourceItemInstanceId == Other.SourceItemInstanceId
		&& AuthorityContent.Version == Other.AuthorityContent.Version
		&& AuthorityContent.Digest == Other.AuthorityContent.Digest
		&& ObservedNextActivationSequence
			== Other.ObservedNextActivationSequence
		&& SessionConfig.Matches(Other.SessionConfig)
		&& ChoicePolicy.Matches(Other.ChoicePolicy)
		&& SegmentCount == Other.SegmentCount;
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewCaptureResult::IsValid() const
{
	if (Status == EStatus::Invalid || Diagnostic.IsEmpty())
	{
		return false;
	}
	if (Status == EStatus::RequestRejected)
	{
		return !Request.IsValid()
			&& !PreviewActivationId.IsValid()
			&& !ProspectiveRealActivationId.IsValid()
			&& !PreviewAction.IsValid()
			&& !Configuration.IsValid();
	}
	if (!Request.IsValid())
	{
		return false;
	}

	const FGuid ExpectedPreviewId = MakePreviewActivationId(Request);
	const FGuid ExpectedRealId = MakeProspectiveRealActivationId(Request);
	if (!ExpectedPreviewId.IsValid()
		|| !ExpectedRealId.IsValid()
		|| ExpectedPreviewId == ExpectedRealId
		|| PreviewActivationId != ExpectedPreviewId
		|| ProspectiveRealActivationId != ExpectedRealId)
	{
		return false;
	}

	FShanmenCombatActionSnapshot ExpectedAction;
	if (!TryBuildPreviewAction(Request, ExpectedPreviewId, ExpectedAction))
	{
		return false;
	}
	if (Status == EStatus::ActionRejected)
	{
		return !PreviewAction.IsValid() && !Configuration.IsValid();
	}
	if (!ActionsMatch(PreviewAction, ExpectedAction))
	{
		return false;
	}
	if (Status == EStatus::ConfigurationRejected)
	{
		return !Configuration.IsValid();
	}
	if (Status != EStatus::Captured || !Configuration.IsValid())
	{
		return false;
	}

	Fdemo_mapShanmenThrownWeaponArcPreviewConfiguration ExpectedConfiguration;
	return TryBuildConfiguration(
			Request, ExpectedAction, ExpectedConfiguration)
		&& Configuration.Matches(ExpectedConfiguration);
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewCaptureResult::IsCaptured() const
{
	return IsValid() && Status == EStatus::Captured;
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewCaptureResult::Matches(
	const Fdemo_mapShanmenThrownWeaponArcPreviewCaptureResult& Other) const
{
	if (!IsValid()
		|| !Other.IsValid()
		|| Status != Other.Status
		|| Diagnostic != Other.Diagnostic)
	{
		return false;
	}
	if (Status == EStatus::RequestRejected)
	{
		return true;
	}
	return Request.Matches(Other.Request)
		&& PreviewActivationId == Other.PreviewActivationId
		&& ProspectiveRealActivationId == Other.ProspectiveRealActivationId
		&& PreviewAction.IsValid() == Other.PreviewAction.IsValid()
		&& (!PreviewAction.IsValid()
			|| ActionsMatch(PreviewAction, Other.PreviewAction))
		&& Configuration.IsValid() == Other.Configuration.IsValid()
		&& (!Configuration.IsValid()
			|| Configuration.Matches(Other.Configuration));
}

Fdemo_mapShanmenThrownWeaponArcPreviewCaptureResult
Fdemo_mapShanmenThrownWeaponArcPreviewCapturePolicy::Capture(
	const Fdemo_mapShanmenThrownWeaponArcPreviewCaptureRequest& Request)
{
	Fdemo_mapShanmenThrownWeaponArcPreviewCaptureResult Result;
	Result.Request = Request;
	if (!Request.IsValid())
	{
		Result.Status = EStatus::RequestRejected;
		Result.Diagnostic =
			TEXT("Arc preview capture requires one valid read-only request.");
		return Result;
	}

	Result.PreviewActivationId = MakePreviewActivationId(Request);
	Result.ProspectiveRealActivationId =
		MakeProspectiveRealActivationId(Request);
	if (!Result.PreviewActivationId.IsValid()
		|| !Result.ProspectiveRealActivationId.IsValid()
		|| Result.PreviewActivationId
			== Result.ProspectiveRealActivationId
		|| !TryBuildPreviewAction(
			Request, Result.PreviewActivationId, Result.PreviewAction))
	{
		Result.Status = EStatus::ActionRejected;
		Result.Diagnostic =
			TEXT("Preview-only action identity failed closed.");
		Result.PreviewAction = FShanmenCombatActionSnapshot();
		return Result;
	}

	if (!TryBuildConfiguration(
			Request, Result.PreviewAction, Result.Configuration))
	{
		Result.Status = EStatus::ConfigurationRejected;
		Result.Diagnostic =
			TEXT("Preview action could not enter the frozen Arc configuration.");
		return Result;
	}

	Result.Status = EStatus::Captured;
	Result.Diagnostic =
		TEXT("Captured one non-reserving preview-only Arc configuration.");
	return Result;
}
