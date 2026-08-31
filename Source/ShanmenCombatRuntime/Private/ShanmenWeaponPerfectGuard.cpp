#include "ShanmenWeaponPerfectGuard.h"

#include "ShanmenCombatTags.h"
#include "ShanmenDeterministicId.h"

namespace
{
	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	bool WindowsMatch(
		const FShanmenWeaponGuardWindowReceipt& Left,
		const FShanmenWeaponGuardWindowReceipt& Right)
	{
		return Left.IsValid()
			&& Right.IsValid()
			&& Left.GetWindowId() == Right.GetWindowId()
			&& Left.GetReceiptId() == Right.GetReceiptId();
	}

	FGuid MakePolicyId(
		const FShanmenWeaponGuardWindowReceipt& Window,
		const FGuid& TimelineId,
		int64 ActiveStartTick,
		int64 PerfectEndTick,
		FName PerfectRuleId)
	{
		if (!Window.IsValid()
			|| !TimelineId.IsValid()
			|| ActiveStartTick < 0
			|| PerfectEndTick <= ActiveStartTick
			|| PerfectRuleId.IsNone())
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Sword.WeaponPerfectGuard.Policy.r1"),
			{
				GuidDigits(Window.GetReceiptId()),
				GuidDigits(Window.GetWindowId()),
				GuidDigits(TimelineId),
				FString::Printf(TEXT("%lld"), ActiveStartTick),
				FString::Printf(TEXT("%lld"), PerfectEndTick),
				PerfectRuleId.ToString()
			});
	}

	FGuid MakeObservationId(const FGuid& TimelineId, int64 ObservedTick)
	{
		if (!TimelineId.IsValid() || ObservedTick < 0)
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Sword.WeaponGuard.TimelineObservation.r1"),
			{
				GuidDigits(TimelineId),
				FString::Printf(TEXT("%lld"), ObservedTick)
			});
	}

	bool LayersMatch(
		const FShanmenDefenseLayer& Left,
		const FShanmenDefenseLayer& Right)
	{
		return Left.IsValid()
			&& Right.IsValid()
			&& Left.LayerId == Right.LayerId
			&& Left.RuleId == Right.RuleId
			&& Left.SourceInstanceId == Right.SourceInstanceId
			&& Left.Operation == Right.Operation
			&& Left.Order == Right.Order
			&& Left.Magnitude == Right.Magnitude
			&& Left.bRequiresCommitOnTrigger
				== Right.bRequiresCommitOnTrigger
			&& Left.LayerTags == Right.LayerTags
			&& Left.RequiredDamageTags == Right.RequiredDamageTags
			&& Left.BlockedDamageTags == Right.BlockedDamageTags
			&& Left.RequiredSourceTags == Right.RequiredSourceTags
			&& Left.BlockedSourceTags == Right.BlockedSourceTags
			&& Left.RequiredTargetTags == Right.RequiredTargetTags
			&& Left.BlockedTargetTags == Right.BlockedTargetTags;
	}

	FGuid MakePerfectLayerId(
		const FShanmenWeaponPerfectGuardPolicy& Policy,
		const FShanmenWeaponGuardTimelineObservation& Observation,
		const FShanmenDefenseLayer& OrdinaryLayer)
	{
		if (!Policy.IsValid()
			|| !Observation.IsValid()
			|| !OrdinaryLayer.IsValid()
			|| Observation.GetTimelineId() != Policy.GetTimelineId())
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Sword.WeaponPerfectGuard.DefenseLayer.r1"),
			{
				GuidDigits(Policy.GetPolicyId()),
				GuidDigits(Observation.GetObservationId()),
				GuidDigits(OrdinaryLayer.LayerId),
				GuidDigits(OrdinaryLayer.SourceInstanceId),
				Policy.GetPerfectRuleId().ToString()
			});
	}

	FShanmenDefenseLayer MakePerfectLayer(
		const FShanmenWeaponPerfectGuardPolicy& Policy,
		const FShanmenWeaponGuardTimelineObservation& Observation,
		const FShanmenDefenseLayer& OrdinaryLayer)
	{
		FShanmenDefenseLayer Layer;
		Layer.LayerId = MakePerfectLayerId(Policy, Observation, OrdinaryLayer);
		Layer.RuleId = Policy.GetPerfectRuleId();
		Layer.SourceInstanceId = OrdinaryLayer.SourceInstanceId;
		Layer.Operation = EShanmenDefenseOperation::PreventAll;
		Layer.Order = FShanmenDefenseOrder::PerfectGuard;
		Layer.Magnitude = 0.0f;
		Layer.bRequiresCommitOnTrigger = false;
		Layer.LayerTags.AddTag(
			FShanmenCombatNativeTags::DefensePerfectGuard());
		Layer.RequiredDamageTags = OrdinaryLayer.RequiredDamageTags;
		Layer.BlockedDamageTags = OrdinaryLayer.BlockedDamageTags;
		Layer.RequiredSourceTags = OrdinaryLayer.RequiredSourceTags;
		Layer.BlockedSourceTags = OrdinaryLayer.BlockedSourceTags;
		Layer.RequiredTargetTags = OrdinaryLayer.RequiredTargetTags;
		Layer.BlockedTargetTags = OrdinaryLayer.BlockedTargetTags;
		return Layer;
	}

	bool PerfectLayerMatches(
		const FShanmenDefenseLayer& Layer,
		const FShanmenWeaponPerfectGuardPolicy& Policy,
		const FShanmenWeaponGuardTimelineObservation& Observation,
		const FShanmenDefenseLayer& OrdinaryLayer)
	{
		return LayersMatch(
			Layer, MakePerfectLayer(Policy, Observation, OrdinaryLayer));
	}

	EShanmenWeaponGuardTimingBand Classify(
		const FShanmenWeaponPerfectGuardPolicy& Policy,
		const FShanmenWeaponGuardTimelineObservation& Observation)
	{
		if (!Policy.IsValid()
			|| !Observation.IsValid()
			|| Observation.GetTimelineId() != Policy.GetTimelineId()
			|| Observation.GetObservedTick() < Policy.GetActiveStartTick())
		{
			return EShanmenWeaponGuardTimingBand::Invalid;
		}
		return Observation.GetObservedTick() < Policy.GetPerfectEndTick()
			? EShanmenWeaponGuardTimingBand::Perfect
			: EShanmenWeaponGuardTimingBand::Ordinary;
	}

	FGuid MakeTimingReceiptId(
		const FShanmenWeaponPerfectGuardPolicy& Policy,
		const FShanmenWeaponGuardTimelineObservation& Observation,
		const FShanmenWeaponGuardProjectionReceipt& OrdinaryProjection,
		EShanmenWeaponGuardTimingBand Band,
		const FShanmenDefenseLayer& Layer)
	{
		if (!Policy.IsValid()
			|| !Observation.IsValid()
			|| !OrdinaryProjection.IsValid()
			|| !Layer.IsValid()
			|| Band == EShanmenWeaponGuardTimingBand::Invalid)
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Sword.WeaponGuard.TimingProjection.r1"),
			{
				GuidDigits(Policy.GetPolicyId()),
				GuidDigits(Observation.GetObservationId()),
				GuidDigits(OrdinaryProjection.GetProjectionId()),
				FString::FromInt(static_cast<uint8>(Band)),
				GuidDigits(Layer.LayerId)
			});
	}
}

bool FShanmenWeaponPerfectGuardPolicy::TryCapture(
	const FShanmenWeaponGuardWindowReceipt& Window,
	const FGuid& TimelineId,
	int64 ActiveStartTick,
	int64 PerfectEndTick,
	FName PerfectRuleId,
	FShanmenWeaponPerfectGuardPolicy& OutPolicy)
{
	OutPolicy = FShanmenWeaponPerfectGuardPolicy();
	FShanmenWeaponPerfectGuardPolicy Candidate;
	Candidate.Window = Window;
	Candidate.TimelineId = TimelineId;
	Candidate.ActiveStartTick = ActiveStartTick;
	Candidate.PerfectEndTick = PerfectEndTick;
	Candidate.PerfectRuleId = PerfectRuleId;
	Candidate.PolicyId = MakePolicyId(
		Window,
		TimelineId,
		ActiveStartTick,
		PerfectEndTick,
		PerfectRuleId);
	if (!Candidate.IsValid())
	{
		return false;
	}
	OutPolicy = Candidate;
	return true;
}

bool FShanmenWeaponPerfectGuardPolicy::IsValid() const
{
	return PolicyId.IsValid()
		&& Window.IsValid()
		&& TimelineId.IsValid()
		&& ActiveStartTick >= 0
		&& PerfectEndTick > ActiveStartTick
		&& !PerfectRuleId.IsNone()
		&& PolicyId == MakePolicyId(
			Window,
			TimelineId,
			ActiveStartTick,
			PerfectEndTick,
			PerfectRuleId);
}

bool FShanmenWeaponGuardTimelineObservation::TryCapture(
	const FGuid& TimelineId,
	int64 ObservedTick,
	FShanmenWeaponGuardTimelineObservation& OutObservation)
{
	OutObservation = FShanmenWeaponGuardTimelineObservation();
	FShanmenWeaponGuardTimelineObservation Candidate;
	Candidate.TimelineId = TimelineId;
	Candidate.ObservedTick = ObservedTick;
	Candidate.ObservationId = MakeObservationId(TimelineId, ObservedTick);
	if (!Candidate.IsValid())
	{
		return false;
	}
	OutObservation = Candidate;
	return true;
}

bool FShanmenWeaponGuardTimelineObservation::IsValid() const
{
	return ObservationId.IsValid()
		&& TimelineId.IsValid()
		&& ObservedTick >= 0
		&& ObservationId == MakeObservationId(TimelineId, ObservedTick);
}

bool FShanmenWeaponGuardTimingProjectionReceipt::IsValid() const
{
	if (!ReceiptId.IsValid()
		|| !Policy.IsValid()
		|| !Observation.IsValid()
		|| !OrdinaryProjection.IsValid()
		|| !Layer.IsValid()
		|| !WindowsMatch(
			Policy.GetWindow(), OrdinaryProjection.GetWindow())
		|| Band != Classify(Policy, Observation))
	{
		return false;
	}

	const bool bLayerMatches =
		Band == EShanmenWeaponGuardTimingBand::Perfect
			? PerfectLayerMatches(
				Layer,
				Policy,
				Observation,
				OrdinaryProjection.GetLayer())
			: Band == EShanmenWeaponGuardTimingBand::Ordinary
				&& LayersMatch(Layer, OrdinaryProjection.GetLayer());
	return bLayerMatches
		&& ReceiptId == MakeTimingReceiptId(
			Policy, Observation, OrdinaryProjection, Band, Layer);
}

bool FShanmenWeaponGuardTimingEvaluator::TryProject(
	const FShanmenWeaponGuardWindow& Window,
	const FShanmenActionOrchestrator& ActionRuntime,
	const FShanmenWeaponPerfectGuardPolicy& Policy,
	const FShanmenWeaponGuardTimelineObservation& Observation,
	FShanmenWeaponGuardTimingProjectionReceipt& OutReceipt)
{
	OutReceipt = FShanmenWeaponGuardTimingProjectionReceipt();
	if (!Window.IsActiveFor(ActionRuntime)
		|| !Policy.IsValid()
		|| !Observation.IsValid()
		|| !WindowsMatch(Window.GetOpenReceipt(), Policy.GetWindow()))
	{
		return false;
	}

	const EShanmenWeaponGuardTimingBand Band = Classify(
		Policy, Observation);
	if (Band == EShanmenWeaponGuardTimingBand::Invalid)
	{
		return false;
	}

	FShanmenWeaponGuardProjectionReceipt OrdinaryProjection;
	if (!Window.TryProjectDefenseLayer(
		ActionRuntime, OrdinaryProjection))
	{
		return false;
	}

	FShanmenWeaponGuardTimingProjectionReceipt Candidate;
	Candidate.Policy = Policy;
	Candidate.Observation = Observation;
	Candidate.OrdinaryProjection = OrdinaryProjection;
	Candidate.Band = Band;
	Candidate.Layer = Band == EShanmenWeaponGuardTimingBand::Perfect
		? MakePerfectLayer(
			Policy, Observation, OrdinaryProjection.GetLayer())
		: OrdinaryProjection.GetLayer();
	Candidate.ReceiptId = MakeTimingReceiptId(
		Candidate.Policy,
		Candidate.Observation,
		Candidate.OrdinaryProjection,
		Candidate.Band,
		Candidate.Layer);
	if (!Candidate.IsValid())
	{
		return false;
	}

	OutReceipt = Candidate;
	return true;
}
