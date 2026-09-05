#include "demo_mapShanmenThrownWeaponArcPreviewComposition.h"

#include "ShanmenDeterministicId.h"

namespace
{
	using EStatus =
		Edemo_mapShanmenThrownWeaponArcPreviewCompositionStatus;

	FString DoubleBits(double Value)
	{
		Value = Value == 0.0 ? 0.0 : Value;
		uint64 Bits = 0;
		static_assert(sizeof(Bits) == sizeof(Value));
		FMemory::Memcpy(&Bits, &Value, sizeof(Bits));
		return FString::Printf(TEXT("%016llX"), Bits);
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

	FGuid MakeConfigurationId(
		const Fdemo_mapShanmenThrownWeaponArcPreviewConfiguration& Configuration)
	{
		const FShanmenCombatActionSnapshot& Action = Configuration.GetAction();
		const Fdemo_mapShanmenThrownWeaponArcProductPolicy& ArcPolicy =
			Configuration.GetArcProductPolicy();
		if (!Action.IsValid()
			|| Action.GetActionDefinitionId()
				!= FShanmenThrownWeaponDefinition::ArcActionDefinitionId()
			|| !ArcPolicy.IsValid()
			|| !FMath::IsFinite(Configuration.GetMaximumLaunchSpeed())
			|| Configuration.GetMaximumLaunchSpeed() <= 0.0
			|| !Configuration.GetChoicePolicy().IsValid()
			|| Configuration.GetSegmentCount()
				< FShanmenThrownWeaponArcPreviewSampler::MinimumSegmentCount
			|| Configuration.GetSegmentCount()
				> FShanmenThrownWeaponArcPreviewSampler::MaximumSegmentCount)
		{
			return FGuid();
		}

		return FShanmenDeterministicId::FromCanonicalParts(
			FName(TEXT("demo_map.ShanmenThrownWeapon.ArcPreviewConfiguration.r1")),
			{
				Action.GetRunId().ToString(EGuidFormats::Digits),
				Action.GetOwnerId().ToString(EGuidFormats::Digits),
				Action.GetActivationId().ToString(EGuidFormats::Digits),
				Action.GetSourceEntityId().ToString(EGuidFormats::Digits),
				Action.GetSourceItemInstanceId().ToString(EGuidFormats::Digits),
				Action.GetActionDefinitionId().ToString(),
				Action.GetContent().Version.ToString(),
				Action.GetContent().Digest,
				FString::FromInt(
					static_cast<int32>(ArcPolicy.GetTechniqueTier())),
				DoubleBits(ArcPolicy.GetGravityMagnitude()),
				DoubleBits(ArcPolicy.GetMaximumFlightTime()),
				DoubleBits(Configuration.GetMaximumLaunchSpeed()),
				Configuration.GetChoicePolicy().GetPolicyId().ToString(
					EGuidFormats::Digits),
				FString::FromInt(Configuration.GetSegmentCount())
			});
	}

	FShanmenThrownWeaponArcRequestCapture BuildPlanCapture(
		const Fdemo_mapShanmenThrownWeaponArcPreviewConfiguration& Configuration,
		const Fdemo_mapShanmenThrownWeaponArcChoiceBasis& Basis,
		const Fdemo_mapShanmenThrownWeaponArcChoiceProjectionResult& Projection)
	{
		const Fdemo_mapShanmenThrownWeaponArcProductPolicy& ArcPolicy =
			Configuration.GetArcProductPolicy();
		FShanmenThrownWeaponArcRequestCapture Capture;
		Capture.Action = Configuration.GetAction();
		Capture.TechniqueTier = ArcPolicy.GetTechniqueTier();
		Capture.Origin = Basis.GetOrigin();
		Capture.Target = Projection.GetTarget();
		Capture.GravityMagnitude = ArcPolicy.GetGravityMagnitude();
		Capture.ApexClearance = Projection.GetApexClearance();
		Capture.MaximumLaunchSpeed = Configuration.GetMaximumLaunchSpeed();
		Capture.MaximumFlightTime = ArcPolicy.GetMaximumFlightTime();
		return Capture;
	}

	bool PlanResultsMatch(
		const FShanmenThrownWeaponArcPlanResult& Left,
		const FShanmenThrownWeaponArcPlanResult& Right)
	{
		if (!Left.IsValid() || !Right.IsValid()
			|| Left.Status != Right.Status
			|| Left.Diagnostic != Right.Diagnostic)
		{
			return false;
		}
		const bool bRequestsMatch = Left.Request.IsValid()
			? Right.Request.IsValid() && Left.Request.Matches(Right.Request)
			: !Right.Request.IsValid();
		const bool bPlansMatch = Left.Plan.IsValid()
			? Right.Plan.IsValid() && Left.Plan.Matches(Right.Plan)
			: !Right.Plan.IsValid();
		return bRequestsMatch && bPlansMatch;
	}

	bool HasNoProjectionPlanOrPreview(
		const Fdemo_mapShanmenThrownWeaponArcPreviewCompositionResult& Result)
	{
		return Result.GetProjection().GetStatus()
				== Edemo_mapShanmenThrownWeaponArcChoiceProjectionStatus::Invalid
			&& Result.GetPlanResult().Status
				== EShanmenThrownWeaponArcPlanStatus::Invalid
			&& !Result.GetPreview().IsValid();
	}

	bool HasRejectedProjection(
		const Fdemo_mapShanmenThrownWeaponArcChoiceProjectionResult& Projection)
	{
		return !Projection.IsValid()
			&& Projection.GetStatus()
				!= Edemo_mapShanmenThrownWeaponArcChoiceProjectionStatus::Invalid
			&& !Projection.GetDiagnostic().IsEmpty();
	}
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewConfiguration::TryCapture(
	const FShanmenCombatActionSnapshot& InAction,
	const Fdemo_mapShanmenThrownWeaponProductCapture& Product,
	const Fdemo_mapShanmenThrownWeaponArcChoicePolicy& InChoicePolicy,
	const int32 InSegmentCount,
	Fdemo_mapShanmenThrownWeaponArcPreviewConfiguration& OutConfiguration)
{
	OutConfiguration =
		Fdemo_mapShanmenThrownWeaponArcPreviewConfiguration();
	if (!Product.IsValid())
	{
		return false;
	}
	return TryCapture(
		InAction,
		Product.GetDefinition(),
		Product.GetArcPolicy(),
		InChoicePolicy,
		InSegmentCount,
		OutConfiguration);
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewConfiguration::TryCapture(
	const FShanmenCombatActionSnapshot& InAction,
	const FShanmenThrownWeaponDefinition& Definition,
	const Fdemo_mapShanmenThrownWeaponArcProductPolicy& InArcProductPolicy,
	const Fdemo_mapShanmenThrownWeaponArcChoicePolicy& InChoicePolicy,
	const int32 InSegmentCount,
	Fdemo_mapShanmenThrownWeaponArcPreviewConfiguration& OutConfiguration)
{
	OutConfiguration =
		Fdemo_mapShanmenThrownWeaponArcPreviewConfiguration();
	if (!InAction.IsValid()
		|| !Definition.IsValid()
		|| Definition.GetActionDefinitionId()
			!= FShanmenThrownWeaponDefinition::ArcActionDefinitionId()
		|| InAction.GetActionDefinitionId()
			!= Definition.GetActionDefinitionId()
		|| !InArcProductPolicy.IsValid()
		|| !InChoicePolicy.IsValid()
		|| InSegmentCount
			< FShanmenThrownWeaponArcPreviewSampler::MinimumSegmentCount
		|| InSegmentCount
			> FShanmenThrownWeaponArcPreviewSampler::MaximumSegmentCount)
	{
		return false;
	}

	OutConfiguration.Action = InAction;
	OutConfiguration.ArcProductPolicy = InArcProductPolicy;
	OutConfiguration.MaximumLaunchSpeed =
		static_cast<double>(Definition.GetLaunchSpeed());
	OutConfiguration.ChoicePolicy = InChoicePolicy;
	OutConfiguration.SegmentCount = InSegmentCount;
	OutConfiguration.ConfigurationId = MakeConfigurationId(OutConfiguration);
	if (!OutConfiguration.IsValid())
	{
		OutConfiguration =
			Fdemo_mapShanmenThrownWeaponArcPreviewConfiguration();
		return false;
	}
	return true;
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewConfiguration::IsValid() const
{
	return ConfigurationId.IsValid()
		&& Action.IsValid()
		&& Action.GetActionDefinitionId()
			== FShanmenThrownWeaponDefinition::ArcActionDefinitionId()
		&& ArcProductPolicy.IsValid()
		&& FMath::IsFinite(MaximumLaunchSpeed)
		&& MaximumLaunchSpeed > 0.0
		&& ChoicePolicy.IsValid()
		&& SegmentCount
			>= FShanmenThrownWeaponArcPreviewSampler::MinimumSegmentCount
		&& SegmentCount
			<= FShanmenThrownWeaponArcPreviewSampler::MaximumSegmentCount
		&& ConfigurationId == MakeConfigurationId(*this);
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewConfiguration::Matches(
	const Fdemo_mapShanmenThrownWeaponArcPreviewConfiguration& Other) const
{
	return IsValid()
		&& Other.IsValid()
		&& ConfigurationId == Other.ConfigurationId
		&& ActionsMatch(Action, Other.Action)
		&& ArcProductPolicy.Matches(Other.ArcProductPolicy)
		&& MaximumLaunchSpeed == Other.MaximumLaunchSpeed
		&& ChoicePolicy.Matches(Other.ChoicePolicy)
		&& SegmentCount == Other.SegmentCount;
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewCompositionResult::IsValid() const
{
	if (Status == EStatus::Invalid
		|| Diagnostic.IsEmpty()
		|| ChoiceStateReadCount < 0 || ChoiceStateReadCount > 1
		|| BasisSampleCount < 0 || BasisSampleCount > 1)
	{
		return false;
	}

	if (Status == EStatus::ConfigurationRejected)
	{
		return !Configuration.IsValid()
			&& ChoiceStateReadCount == 0
			&& BasisSampleCount == 0
			&& !ChoiceState.IsValid()
			&& !Basis.IsValid()
			&& HasNoProjectionPlanOrPreview(*this);
	}
	if (!Configuration.IsValid())
	{
		return false;
	}

	switch (Status)
	{
	case EStatus::ChoiceUnavailable:
		return ChoiceStateReadCount == 1
			&& BasisSampleCount == 0
			&& !ChoiceState.IsValid()
			&& !Basis.IsValid()
			&& HasNoProjectionPlanOrPreview(*this);

	case EStatus::BasisUnavailable:
		return ChoiceStateReadCount == 1
			&& BasisSampleCount == 1
			&& ChoiceState.IsValid()
			&& !Basis.IsValid()
			&& HasNoProjectionPlanOrPreview(*this);

	case EStatus::ProjectionRejected:
		return ChoiceStateReadCount == 1
			&& BasisSampleCount == 1
			&& ChoiceState.IsValid()
			&& Basis.IsValid()
			&& HasRejectedProjection(Projection)
			&& PlanResult.Status
				== EShanmenThrownWeaponArcPlanStatus::Invalid
			&& !Preview.IsValid();

	case EStatus::PlanRejected:
	{
		if (ChoiceStateReadCount != 1
			|| BasisSampleCount != 1
			|| !ChoiceState.IsValid()
			|| !Basis.IsValid()
			|| !Projection.IsValid()
			|| !PlanResult.IsValid()
			|| PlanResult.IsPlanned()
			|| Preview.IsValid())
		{
			return false;
		}
		const FShanmenThrownWeaponArcPlanResult Expected =
			FShanmenThrownWeaponArcPlanner::Plan(
				BuildPlanCapture(Configuration, Basis, Projection));
		return PlanResultsMatch(PlanResult, Expected);
	}

	case EStatus::PreviewRejected:
		return ChoiceStateReadCount == 1
			&& BasisSampleCount == 1
			&& ChoiceState.IsValid()
			&& Basis.IsValid()
			&& Projection.IsValid()
			&& PlanResult.IsPlanned()
			&& !Preview.IsValid();

	case EStatus::Composed:
	{
		if (ChoiceStateReadCount != 1
			|| BasisSampleCount != 1
			|| !ChoiceState.IsValid()
			|| !Basis.IsValid()
			|| !Projection.IsValid()
			|| !PlanResult.IsPlanned()
			|| !Preview.IsValid())
		{
			return false;
		}
		const FShanmenThrownWeaponArcPlanResult ExpectedPlan =
			FShanmenThrownWeaponArcPlanner::Plan(
				BuildPlanCapture(Configuration, Basis, Projection));
		FShanmenThrownWeaponArcPreview ExpectedPreview;
		return PlanResultsMatch(PlanResult, ExpectedPlan)
			&& FShanmenThrownWeaponArcPreviewSampler::TrySample(
				ExpectedPlan.Plan,
				Configuration.GetSegmentCount(),
				ExpectedPreview)
			&& Preview.Matches(ExpectedPreview);
	}

	default:
		return false;
	}
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewCompositionResult::IsComposed()
	const
{
	return IsValid() && Status == EStatus::Composed;
}

bool Fdemo_mapShanmenThrownWeaponArcPreviewCompositionResult::Matches(
	const Fdemo_mapShanmenThrownWeaponArcPreviewCompositionResult& Other) const
{
	if (!IsValid() || !Other.IsValid()
		|| Status != Other.Status
		|| Diagnostic != Other.Diagnostic
		|| ChoiceStateReadCount != Other.ChoiceStateReadCount
		|| BasisSampleCount != Other.BasisSampleCount)
	{
		return false;
	}
	if (Status == EStatus::ConfigurationRejected)
	{
		return true;
	}
	if (!Configuration.Matches(Other.Configuration))
	{
		return false;
	}
	if (ChoiceState.IsValid()
		&& !ChoiceState.Matches(Other.ChoiceState))
	{
		return false;
	}
	if (Basis.IsValid() && !Basis.Matches(Other.Basis))
	{
		return false;
	}
	if (Projection.IsValid()
		&& !Projection.Matches(Other.Projection))
	{
		return false;
	}
	if (HasRejectedProjection(Projection)
		&& (Projection.GetStatus() != Other.Projection.GetStatus()
			|| Projection.GetDiagnostic()
				!= Other.Projection.GetDiagnostic()))
	{
		return false;
	}
	if (PlanResult.IsValid()
		&& !PlanResultsMatch(PlanResult, Other.PlanResult))
	{
		return false;
	}
	if (Preview.IsValid() && !Preview.Matches(Other.Preview))
	{
		return false;
	}
	return ChoiceState.IsValid() == Other.ChoiceState.IsValid()
		&& Basis.IsValid() == Other.Basis.IsValid()
		&& Projection.GetStatus() == Other.Projection.GetStatus()
		&& PlanResult.Status == Other.PlanResult.Status
		&& Preview.IsValid() == Other.Preview.IsValid();
}

Fdemo_mapShanmenThrownWeaponArcPreviewCompositionResult
Fdemo_mapShanmenThrownWeaponArcPreviewComposition::Compose(
	const Fdemo_mapShanmenThrownWeaponArcPreviewConfiguration& Configuration,
	FReadCurrentChoice ReadCurrentChoice,
	FSampleSourceBasis SampleSourceBasis)
{
	Fdemo_mapShanmenThrownWeaponArcPreviewCompositionResult Result;
	Result.Configuration = Configuration;
	if (!Configuration.IsValid())
	{
		Result.Status = EStatus::ConfigurationRejected;
		Result.Diagnostic =
			TEXT("Arc preview composition requires one valid frozen configuration.");
		return Result;
	}

	Result.ChoiceStateReadCount = 1;
	Result.ChoiceState = ReadCurrentChoice();
	if (!Result.ChoiceState.IsValid())
	{
		Result.Status = EStatus::ChoiceUnavailable;
		Result.Diagnostic =
			TEXT("Arc preview composition could not read one valid current choice.");
		return Result;
	}

	Result.BasisSampleCount = 1;
	Result.Basis = SampleSourceBasis();
	if (!Result.Basis.IsValid())
	{
		Result.Status = EStatus::BasisUnavailable;
		Result.Diagnostic =
			TEXT("Arc preview composition could not sample one valid source basis.");
		return Result;
	}

	Result.Projection = Fdemo_mapShanmenThrownWeaponArcChoiceProjector::Project(
		Result.ChoiceState,
		Result.Basis,
		Configuration.GetChoicePolicy());
	if (!Result.Projection.IsProjected())
	{
		Result.Status = EStatus::ProjectionRejected;
		Result.Diagnostic = Result.Projection.GetDiagnostic().IsEmpty()
			? TEXT("Arc preview composition rejected choice projection.")
			: Result.Projection.GetDiagnostic();
		return Result;
	}

	Result.PlanResult = FShanmenThrownWeaponArcPlanner::Plan(
		BuildPlanCapture(Configuration, Result.Basis, Result.Projection));
	if (!Result.PlanResult.IsPlanned())
	{
		Result.Status = EStatus::PlanRejected;
		Result.Diagnostic = Result.PlanResult.Diagnostic.IsEmpty()
			? TEXT("Arc preview composition could not produce a ballistic plan.")
			: Result.PlanResult.Diagnostic;
		return Result;
	}

	if (!FShanmenThrownWeaponArcPreviewSampler::TrySample(
			Result.PlanResult.Plan,
			Configuration.GetSegmentCount(),
			Result.Preview))
	{
		Result.Status = EStatus::PreviewRejected;
		Result.Diagnostic =
			TEXT("Arc preview composition could not sample the planned trajectory.");
		return Result;
	}

	Result.Status = EStatus::Composed;
	Result.Diagnostic =
		TEXT("Composed one frozen Arc choice into a deterministic preview.");
	return Result;
}
