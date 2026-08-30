#include "demo_mapShanmenSpiritEvasionProductAuthority.h"

#include "ShanmenCombatResolver.h"
#include "ShanmenCombatTags.h"
#include "ShanmenDeterministicId.h"
#include "demo_mapCombatRunCoordinator.h"

namespace
{
	constexpr float CanonicalRequestedDistance = 400.0f;
	constexpr float CanonicalMinimumResolvedDistance = 100.0f;
	constexpr float CanonicalWorldStaticClearance = 2.0f;
	constexpr float CanonicalDurationSeconds = 0.2f;
	constexpr int32 CanonicalSegmentCount = 2;

	bool IsFiniteVector(const FVector& Value)
	{
		return FMath::IsFinite(Value.X)
			&& FMath::IsFinite(Value.Y)
			&& FMath::IsFinite(Value.Z);
	}

	FString FloatBits(float Value)
	{
		uint32 Bits = 0;
		static_assert(sizeof(Bits) == sizeof(Value));
		FMemory::Memcpy(&Bits, &Value, sizeof(Bits));
		return FString::Printf(TEXT("%08X"), Bits);
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

	bool DefinitionsMatch(
		const FShanmenSpiritEvasionDefinition& Left,
		const FShanmenSpiritEvasionDefinition& Right)
	{
		return Left.IsValid() && Right.IsValid()
			&& Left.GetActionDefinitionId() == Right.GetActionDefinitionId()
			&& Left.GetRuleId() == Right.GetRuleId()
			&& Left.GetRequiredDamageTags() == Right.GetRequiredDamageTags()
			&& Left.GetBlockedDamageTags() == Right.GetBlockedDamageTags()
			&& Left.GetRequiredSourceTags() == Right.GetRequiredSourceTags()
			&& Left.GetBlockedSourceTags() == Right.GetBlockedSourceTags()
			&& Left.GetRequiredTargetTags() == Right.GetRequiredTargetTags()
			&& Left.GetBlockedTargetTags() == Right.GetBlockedTargetTags();
	}

	bool PoliciesMatch(
		const Fdemo_mapShanmenSpiritEvasionMovementPolicySnapshot& Left,
		const Fdemo_mapShanmenSpiritEvasionMovementPolicySnapshot& Right)
	{
		return Left.IsValid() && Right.IsValid()
			&& Left.GetMovementPolicyId() == Right.GetMovementPolicyId()
			&& Left.GetRequestedDistance() == Right.GetRequestedDistance()
			&& Left.GetMinimumResolvedDistance()
				== Right.GetMinimumResolvedDistance()
			&& Left.GetWorldStaticClearance()
				== Right.GetWorldStaticClearance();
	}

	bool TrajectoriesMatch(
		const Fdemo_mapShanmenSpiritEvasionTrajectorySnapshot& Left,
		const Fdemo_mapShanmenSpiritEvasionTrajectorySnapshot& Right)
	{
		return Left.IsValid() && Right.IsValid()
			&& Left.GetDurationSeconds() == Right.GetDurationSeconds()
			&& Left.GetSegmentCount() == Right.GetSegmentCount();
	}
}

FName Fdemo_mapShanmenSpiritEvasionProductConfig::CanonicalContentVersion()
{
	return TEXT("0.0.10.P10.8");
}

FString Fdemo_mapShanmenSpiritEvasionProductConfig::CanonicalContentDigest()
{
	return TEXT("Shanmen.SpiritEvasion.ProductConfig.r1");
}

FName Fdemo_mapShanmenSpiritEvasionProductConfig::CanonicalDefenseRuleId()
{
	return TEXT("Defense.Spell.SpiritEvasion01");
}

FName Fdemo_mapShanmenSpiritEvasionProductConfig::CanonicalMovementPolicyId()
{
	return TEXT("Movement.Spell.SpiritEvasion.GroundStep");
}

FGuid Fdemo_mapShanmenSpiritEvasionProductConfig::CanonicalConfigId()
{
	return FShanmenDeterministicId::FromCanonicalParts(
		TEXT("demo_map.Spell.SpiritEvasion.ProductConfig.r1"),
		{
			CanonicalContentVersion().ToString(),
			CanonicalContentDigest(),
			FShanmenSpiritEvasionDefinition::CanonicalActionDefinitionId()
				.ToString(),
			CanonicalDefenseRuleId().ToString(),
			FShanmenCombatNativeTags::TargetLiving().ToString(),
			CanonicalMovementPolicyId().ToString(),
			FloatBits(CanonicalRequestedDistance),
			FloatBits(CanonicalMinimumResolvedDistance),
			FloatBits(CanonicalWorldStaticClearance),
			FloatBits(CanonicalDurationSeconds),
			FString::FromInt(CanonicalSegmentCount)
		});
}

bool Fdemo_mapShanmenSpiritEvasionProductConfig::TryCreateCanonical(
	Fdemo_mapShanmenSpiritEvasionProductConfig& OutConfig)
{
	OutConfig = Fdemo_mapShanmenSpiritEvasionProductConfig();
	Fdemo_mapShanmenSpiritEvasionProductConfig Candidate;
	Candidate.Content.Version = CanonicalContentVersion();
	Candidate.Content.Digest = CanonicalContentDigest();

	FShanmenSpiritEvasionDefinitionCapture DefinitionCapture;
	DefinitionCapture.ActionDefinitionId =
		FShanmenSpiritEvasionDefinition::CanonicalActionDefinitionId();
	DefinitionCapture.RuleId = CanonicalDefenseRuleId();
	DefinitionCapture.RequiredTargetTags.AddTag(
		FShanmenCombatNativeTags::TargetLiving());
	if (!FShanmenSpiritEvasionDefinition::TryCapture(
			DefinitionCapture,
			Candidate.Definition))
	{
		return false;
	}

	Fdemo_mapShanmenSpiritEvasionMovementPolicyCapture PolicyCapture;
	PolicyCapture.MovementPolicyId = CanonicalMovementPolicyId();
	PolicyCapture.RequestedDistance = CanonicalRequestedDistance;
	PolicyCapture.MinimumResolvedDistance = CanonicalMinimumResolvedDistance;
	PolicyCapture.WorldStaticClearance = CanonicalWorldStaticClearance;
	if (!Fdemo_mapShanmenSpiritEvasionMovementPolicySnapshot::TryCapture(
			PolicyCapture,
			Candidate.Policy))
	{
		return false;
	}

	Fdemo_mapShanmenSpiritEvasionTrajectoryCapture TrajectoryCapture;
	TrajectoryCapture.DurationSeconds = CanonicalDurationSeconds;
	TrajectoryCapture.SegmentCount = CanonicalSegmentCount;
	if (!Fdemo_mapShanmenSpiritEvasionTrajectorySnapshot::TryCapture(
			TrajectoryCapture,
			Candidate.Trajectory))
	{
		return false;
	}

	Candidate.ConfigId = CanonicalConfigId();
	if (!Candidate.IsValid())
	{
		return false;
	}
	OutConfig = Candidate;
	return true;
}

bool Fdemo_mapShanmenSpiritEvasionProductConfig::IsValid() const
{
	FGameplayTagContainer ExpectedTargetTags;
	ExpectedTargetTags.AddTag(FShanmenCombatNativeTags::TargetLiving());
	return ConfigId.IsValid()
		&& ConfigId == CanonicalConfigId()
		&& Content.IsValid()
		&& Content.Version == CanonicalContentVersion()
		&& Content.Digest == CanonicalContentDigest()
		&& Definition.IsValid()
		&& Definition.GetActionDefinitionId()
			== FShanmenSpiritEvasionDefinition::CanonicalActionDefinitionId()
		&& Definition.GetRuleId() == CanonicalDefenseRuleId()
		&& Definition.GetRequiredDamageTags().IsEmpty()
		&& Definition.GetBlockedDamageTags().IsEmpty()
		&& Definition.GetRequiredSourceTags().IsEmpty()
		&& Definition.GetBlockedSourceTags().IsEmpty()
		&& Definition.GetRequiredTargetTags() == ExpectedTargetTags
		&& Definition.GetBlockedTargetTags().IsEmpty()
		&& Policy.IsValid()
		&& Policy.GetMovementPolicyId() == CanonicalMovementPolicyId()
		&& Policy.GetRequestedDistance() == CanonicalRequestedDistance
		&& Policy.GetMinimumResolvedDistance()
			== CanonicalMinimumResolvedDistance
		&& Policy.GetWorldStaticClearance() == CanonicalWorldStaticClearance
		&& Trajectory.IsValid()
		&& Trajectory.GetDurationSeconds() == CanonicalDurationSeconds
		&& Trajectory.GetSegmentCount() == CanonicalSegmentCount;
}

bool Fdemo_mapPlayerSpiritEvasionActionReservation::IsValid() const
{
	FGameplayTagContainer ExpectedSourceTags;
	ExpectedSourceTags.AddTag(FShanmenCombatNativeTags::SourcePlayer());
	return ActivationSequence > 0
		&& ConfigId ==
			Fdemo_mapShanmenSpiritEvasionProductConfig::CanonicalConfigId()
		&& Action.IsValid()
		&& Action.GetOwnerId() == Action.GetSourceEntityId()
		&& !Action.GetSourceItemInstanceId().IsValid()
		&& Action.GetActionDefinitionId()
			== FShanmenSpiritEvasionDefinition::CanonicalActionDefinitionId()
		&& Action.GetContent().Version
			== Fdemo_mapShanmenSpiritEvasionProductConfig::
				CanonicalContentVersion()
		&& Action.GetContent().Digest
			== Fdemo_mapShanmenSpiritEvasionProductConfig::
				CanonicalContentDigest()
		&& Action.GetSourceTags() == ExpectedSourceTags
		&& Action.GetActivationId() == FShanmenCombatIdFactory::MakeActivationId(
			Action.GetRunId(),
			Action.GetSourceEntityId(),
			Action.GetActionDefinitionId(),
			ActivationSequence);
}

bool Fdemo_mapShanmenSpiritEvasionProductStartResult::IsReady() const
{
	return Status == Edemo_mapShanmenSpiritEvasionProductStartStatus::Ready
		&& Config.IsValid()
		&& Reservation.IsValid()
		&& Reservation.GetConfigId() == Config.GetConfigId()
		&& Command.IsValid()
		&& Command.GetKind()
			== Edemo_mapShanmenSpiritEvasionCommandKind::Start
		&& ActionsMatch(Command.GetAction(), Reservation.GetAction())
		&& DefinitionsMatch(Command.GetDefinition(), Config.GetDefinition())
		&& PoliciesMatch(Command.GetPolicy(), Config.GetPolicy())
		&& TrajectoriesMatch(Command.GetTrajectory(), Config.GetTrajectory());
}

Fdemo_mapShanmenSpiritEvasionProductStartResult
Fdemo_mapShanmenSpiritEvasionProductAuthority::PrepareStart(
	Fdemo_mapCombatRunCoordinator& Coordinator,
	const FVector& CandidateDirection)
{
	Fdemo_mapShanmenSpiritEvasionProductStartResult Result;
	const FVector PlanarDirection(
		CandidateDirection.X,
		CandidateDirection.Y,
		0.0f);
	if (!IsFiniteVector(CandidateDirection)
		|| PlanarDirection.IsNearlyZero())
	{
		Result.Status =
			Edemo_mapShanmenSpiritEvasionProductStartStatus::InvalidDirection;
		Result.Diagnostic =
			TEXT("Spirit Evasion requires one finite non-zero direction before identity reservation.");
		return Result;
	}
	if (!Fdemo_mapShanmenSpiritEvasionProductConfig::TryCreateCanonical(
			Result.Config))
	{
		Result.Diagnostic =
			TEXT("Canonical Spirit Evasion product config failed closed.");
		return Result;
	}
	if (!Coordinator.TryReservePlayerSpiritEvasionAction(
			Result.Config,
			Result.Reservation,
			Result.Diagnostic))
	{
		Result.Status = Edemo_mapShanmenSpiritEvasionProductStartStatus::
			ReservationRejected;
		return Result;
	}
	if (!Fdemo_mapShanmenSpiritEvasionCommand::TryCaptureStart(
			Result.Reservation.GetAction(),
			Result.Config.GetDefinition(),
			Result.Config.GetPolicy(),
			Result.Config.GetTrajectory(),
			PlanarDirection,
			Result.Command))
	{
		Result.Status =
			Edemo_mapShanmenSpiritEvasionProductStartStatus::CommandRejected;
		Result.Diagnostic =
			TEXT("Reserved Spirit Evasion identity could not freeze a typed command; the sequence remains consumed.");
		return Result;
	}

	Result.Status = Edemo_mapShanmenSpiritEvasionProductStartStatus::Ready;
	Result.Diagnostic =
		TEXT("Canonical Spirit Evasion config and Run identity prepared one typed start command.");
	return Result;
}
