#include "demo_mapShanmenDivineSenseProductAuthority.h"

#include "ShanmenCombatResolver.h"
#include "ShanmenCombatRuntimeTags.h"
#include "ShanmenCombatTags.h"
#include "ShanmenDeterministicId.h"
#include "demo_mapCombatRunCoordinator.h"

namespace
{
	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	bool ActionsMatch(
		const FShanmenCombatActionSnapshot& Left,
		const FShanmenCombatActionSnapshot& Right)
	{
		return Left.IsValid() && Right.IsValid()
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
}

bool Fdemo_mapPlayerDivineSenseActionReservation::IsValid() const
{
	FGameplayTagContainer ExpectedSourceTags;
	ExpectedSourceTags.AddTag(FShanmenCombatNativeTags::SourcePlayer());
	return ActivationSequence > 0
		&& ConfigId
			== Fdemo_mapShanmenDivineSenseProductAuthority::CanonicalConfigId()
		&& Action.IsValid()
		&& Action.GetOwnerId() == Action.GetSourceEntityId()
		&& !Action.GetSourceItemInstanceId().IsValid()
		&& Action.GetActionDefinitionId()
			== FShanmenDivineSenseDefinition::CanonicalActionDefinitionId()
		&& Action.GetContent().Version
			== Fdemo_mapShanmenDivineSenseProductAuthority::
				CanonicalContentVersion()
		&& Action.GetContent().Digest
			== Fdemo_mapShanmenDivineSenseProductAuthority::
				CanonicalContentDigest()
		&& Action.GetSourceTags() == ExpectedSourceTags
		&& Action.GetActivationId() == FShanmenCombatIdFactory::MakeActivationId(
			Action.GetRunId(),
			Action.GetSourceEntityId(),
			Action.GetActionDefinitionId(),
			ActivationSequence);
}

bool Fdemo_mapShanmenDivineSenseProductPrepareResult::IsReady() const
{
	return Status == Edemo_mapShanmenDivineSenseProductPrepareStatus::Ready
		&& !Diagnostic.IsEmpty()
		&& Fdemo_mapShanmenDivineSenseProductAuthority::IsCanonicalConfig(
			Config)
		&& Reservation.IsValid()
		&& Reservation.GetConfigId() == Config.GetConfigId()
		&& Intent.IsValid()
		&& Intent.GetIntentId()
			== Fdemo_mapShanmenDivineSenseProductAuthority::MakeIntentId(
				Reservation)
		&& ActionsMatch(Intent.GetAction(), Reservation.GetAction())
		&& Intent.GetScanOrdinal()
			== Fdemo_mapShanmenDivineSenseProductAuthority::
				CanonicalScanOrdinal()
		&& Intent.GetSubjectActorBudget()
			== Fdemo_mapShanmenDivineSenseProductAuthority::
				CanonicalSubjectActorBudget();
}

FName Fdemo_mapShanmenDivineSenseProductAuthority::CanonicalContentVersion()
{
	return TEXT("0.0.10.P19.7");
}

FString Fdemo_mapShanmenDivineSenseProductAuthority::CanonicalContentDigest()
{
	return TEXT("Shanmen.DivineSense.ProductAuthority.r1");
}

FName Fdemo_mapShanmenDivineSenseProductAuthority::CanonicalScanRuleId()
{
	return TEXT("Spell.DivineSense.Pulse.Basic01");
}

FName Fdemo_mapShanmenDivineSenseProductAuthority::CanonicalCostRuleId()
{
	return TEXT("Spell.DivineSense.SpiritEnergy.Basic01");
}

double Fdemo_mapShanmenDivineSenseProductAuthority::CanonicalRadius()
{
	return 1200.0;
}

int32 Fdemo_mapShanmenDivineSenseProductAuthority::CanonicalMaximumResults()
{
	return 8;
}

EShanmenDivineSenseOcclusionPolicy
Fdemo_mapShanmenDivineSenseProductAuthority::CanonicalOcclusionPolicy()
{
	return EShanmenDivineSenseOcclusionPolicy::RevealOccluded;
}

float Fdemo_mapShanmenDivineSenseProductAuthority::CanonicalSpiritEnergyCost()
{
	return 10.0f;
}

int32 Fdemo_mapShanmenDivineSenseProductAuthority::CanonicalPulseCapacity()
{
	return 16;
}

int32 Fdemo_mapShanmenDivineSenseProductAuthority::CanonicalScanOrdinal()
{
	return 0;
}

int32 Fdemo_mapShanmenDivineSenseProductAuthority::
	CanonicalSubjectActorBudget()
{
	return 32;
}

FGuid Fdemo_mapShanmenDivineSenseProductAuthority::CanonicalConfigId()
{
	Fdemo_mapShanmenDivineSenseProductConfig Config;
	return TryCreateCanonicalConfig(Config) ? Config.GetConfigId() : FGuid();
}

bool Fdemo_mapShanmenDivineSenseProductAuthority::TryCreateCanonicalConfig(
	Fdemo_mapShanmenDivineSenseProductConfig& OutConfig)
{
	OutConfig = Fdemo_mapShanmenDivineSenseProductConfig();
	FShanmenDivineSenseDefinitionCapture DefinitionCapture;
	DefinitionCapture.ActionDefinitionId =
		FShanmenDivineSenseDefinition::CanonicalActionDefinitionId();
	DefinitionCapture.ScanRuleId = CanonicalScanRuleId();
	DefinitionCapture.Radius = CanonicalRadius();
	DefinitionCapture.MaximumResults = CanonicalMaximumResults();
	DefinitionCapture.OcclusionPolicy = CanonicalOcclusionPolicy();
	DefinitionCapture.RequiredSubjectTags.AddTag(
		FShanmenCombatNativeTags::TargetLiving());
	DefinitionCapture.bRejectSelf = true;

	FShanmenDivineSenseDefinition Definition;
	if (!FShanmenDivineSenseDefinition::TryCapture(
			DefinitionCapture,
			Definition))
	{
		return false;
	}

	FShanmenActionResourceCostCapture CostCapture;
	CostCapture.RuleId = CanonicalCostRuleId();
	CostCapture.ResourceChannel =
		FShanmenCombatRuntimeNativeTags::ResourceSpiritEnergy();
	CostCapture.Amount = CanonicalSpiritEnergyCost();
	FShanmenActionResourceCost Cost;
	if (!FShanmenActionResourceCost::TryCapture(CostCapture, Cost))
	{
		return false;
	}

	Fdemo_mapShanmenDivineSenseProductConfig Candidate;
	if (!Fdemo_mapShanmenDivineSenseProductConfig::TryCapture(
			Definition,
			Cost,
			CanonicalPulseCapacity(),
			Candidate)
		|| !Candidate.IsValid())
	{
		return false;
	}
	OutConfig = MoveTemp(Candidate);
	return true;
}

bool Fdemo_mapShanmenDivineSenseProductAuthority::IsCanonicalConfig(
	const Fdemo_mapShanmenDivineSenseProductConfig& Config)
{
	Fdemo_mapShanmenDivineSenseProductConfig Canonical;
	return Config.IsValid()
		&& TryCreateCanonicalConfig(Canonical)
		&& Config.Matches(Canonical)
		&& Config.GetDefinition().GetActionDefinitionId()
			== FShanmenDivineSenseDefinition::CanonicalActionDefinitionId()
		&& Config.GetDefinition().GetScanRuleId() == CanonicalScanRuleId()
		&& Config.GetDefinition().GetRadius() == CanonicalRadius()
		&& Config.GetDefinition().GetMaximumResults()
			== CanonicalMaximumResults()
		&& Config.GetDefinition().GetOcclusionPolicy()
			== CanonicalOcclusionPolicy()
		&& Config.GetDefinition().GetRequiredSubjectTags().HasTagExact(
			FShanmenCombatNativeTags::TargetLiving())
		&& Config.GetDefinition().GetRequiredSubjectTags().Num() == 1
		&& Config.GetDefinition().GetBlockedSubjectTags().IsEmpty()
		&& Config.GetDefinition().RejectsSelf()
		&& Config.GetCost().GetRuleId() == CanonicalCostRuleId()
		&& Config.GetCost().GetResourceChannel()
			== FShanmenCombatRuntimeNativeTags::ResourceSpiritEnergy()
		&& Config.GetCost().GetAmount() == CanonicalSpiritEnergyCost()
		&& Config.GetPulseCapacity() == CanonicalPulseCapacity();
}

FGuid Fdemo_mapShanmenDivineSenseProductAuthority::MakeIntentId(
	const Fdemo_mapPlayerDivineSenseActionReservation& Reservation)
{
	if (!Reservation.IsValid())
	{
		return FGuid();
	}
	return FShanmenDeterministicId::FromCanonicalParts(
		TEXT("demo_map.Spell.DivineSense.ProductIntent.r1"),
		{
			GuidDigits(Reservation.GetActivationId()),
			GuidDigits(Reservation.GetConfigId()),
			FString::FromInt(CanonicalScanOrdinal()),
			FString::FromInt(CanonicalSubjectActorBudget())
		});
}

Fdemo_mapShanmenDivineSenseProductPrepareResult
Fdemo_mapShanmenDivineSenseProductAuthority::PrepareIntent(
	Fdemo_mapCombatRunCoordinator& Coordinator)
{
	Fdemo_mapShanmenDivineSenseProductPrepareResult Result;
	if (!TryCreateCanonicalConfig(Result.Config))
	{
		Result.Diagnostic =
			TEXT("Canonical Divine Sense product config failed closed.");
		return Result;
	}
	if (!Coordinator.TryReservePlayerDivineSenseAction(
			Result.Config,
			Result.Reservation,
			Result.Diagnostic))
	{
		Result.Status =
			Edemo_mapShanmenDivineSenseProductPrepareStatus::
				ReservationRejected;
		return Result;
	}

	const FGuid IntentId = MakeIntentId(Result.Reservation);
	if (!Fdemo_mapShanmenDivineSenseProductIntent::TryCapture(
			IntentId,
			Result.Reservation.GetAction(),
			CanonicalScanOrdinal(),
			CanonicalSubjectActorBudget(),
			Result.Intent))
	{
		Result.Status =
			Edemo_mapShanmenDivineSenseProductPrepareStatus::IntentRejected;
		Result.Diagnostic =
			TEXT("Reserved Divine Sense identity could not freeze the canonical Intent; the sequence remains consumed.");
		return Result;
	}

	Result.Status = Edemo_mapShanmenDivineSenseProductPrepareStatus::Ready;
	Result.Diagnostic =
		TEXT("Canonical Divine Sense policy and Run identity prepared one immutable Intent.");
	return Result;
}
