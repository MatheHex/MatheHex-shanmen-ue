#include "demo_mapShanmenSpiritEvasionMovementAdapter.h"

#include "GameFramework/Character.h"
#include "ShanmenDeterministicId.h"

namespace
{
	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	FString FloatBits(float Value)
	{
		uint32 Bits = 0;
		static_assert(sizeof(Bits) == sizeof(Value));
		FMemory::Memcpy(&Bits, &Value, sizeof(Bits));
		return FString::Printf(TEXT("%08X"), Bits);
	}

	FGuid MakePlanId(
		const FShanmenSpiritEvasionMovementRequest& Request,
		const Fdemo_mapShanmenSpiritEvasionMovementPolicySnapshot& Policy)
	{
		if (!Request.IsValid()
			|| !Policy.IsValid()
			|| Request.GetIntent().GetMovementPolicyId()
				!= Policy.GetMovementPolicyId())
		{
			return FGuid();
		}

		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("demo_map.Spell.SpiritEvasion.MovementPlan.r1"),
			{
				GuidDigits(Request.GetRequestId()),
				Policy.GetMovementPolicyId().ToString(),
				FloatBits(Policy.GetRequestedDistance()),
				FloatBits(Policy.GetMinimumResolvedDistance()),
				FloatBits(Policy.GetWorldStaticClearance())
			});
	}
}

bool Fdemo_mapShanmenSpiritEvasionMovementPolicySnapshot::TryCapture(
	const Fdemo_mapShanmenSpiritEvasionMovementPolicyCapture& Capture,
	Fdemo_mapShanmenSpiritEvasionMovementPolicySnapshot& OutPolicy)
{
	OutPolicy = Fdemo_mapShanmenSpiritEvasionMovementPolicySnapshot();
	if (Capture.MovementPolicyId.IsNone()
		|| !FMath::IsFinite(Capture.RequestedDistance)
		|| !FMath::IsFinite(Capture.MinimumResolvedDistance)
		|| !FMath::IsFinite(Capture.WorldStaticClearance)
		|| Capture.RequestedDistance <= 0.0f
		|| Capture.MinimumResolvedDistance <= 0.0f
		|| Capture.MinimumResolvedDistance > Capture.RequestedDistance
		|| Capture.WorldStaticClearance < 0.0f)
	{
		return false;
	}

	Fdemo_mapShanmenSpiritEvasionMovementPolicySnapshot Candidate;
	Candidate.MovementPolicyId = Capture.MovementPolicyId;
	Candidate.RequestedDistance = Capture.RequestedDistance;
	Candidate.MinimumResolvedDistance = Capture.MinimumResolvedDistance;
	Candidate.WorldStaticClearance = Capture.WorldStaticClearance;
	if (!Candidate.IsValid())
	{
		return false;
	}

	OutPolicy = Candidate;
	return true;
}

bool Fdemo_mapShanmenSpiritEvasionMovementPolicySnapshot::IsValid() const
{
	return !MovementPolicyId.IsNone()
		&& FMath::IsFinite(RequestedDistance)
		&& FMath::IsFinite(MinimumResolvedDistance)
		&& FMath::IsFinite(WorldStaticClearance)
		&& RequestedDistance > 0.0f
		&& MinimumResolvedDistance > 0.0f
		&& MinimumResolvedDistance <= RequestedDistance
		&& WorldStaticClearance >= 0.0f;
}

bool Fdemo_mapShanmenSpiritEvasionMovementPlan::IsValid() const
{
	return PlanId.IsValid()
		&& Request.IsValid()
		&& Policy.IsValid()
		&& Request.GetIntent().GetMovementPolicyId()
			== Policy.GetMovementPolicyId()
		&& PlanId == MakePlanId(Request, Policy);
}

bool Fdemo_mapShanmenSpiritEvasionMovementAdapter::TryBuildPlan(
	const FShanmenSpiritEvasionMovementRequest& Request,
	const Fdemo_mapShanmenSpiritEvasionMovementPolicySnapshot& Policy,
	Fdemo_mapShanmenSpiritEvasionMovementPlan& OutPlan)
{
	OutPlan = Fdemo_mapShanmenSpiritEvasionMovementPlan();
	if (!Request.IsValid()
		|| !Policy.IsValid()
		|| Request.GetIntent().GetMovementPolicyId()
			!= Policy.GetMovementPolicyId())
	{
		return false;
	}

	Fdemo_mapShanmenSpiritEvasionMovementPlan Candidate;
	Candidate.Request = Request;
	Candidate.Policy = Policy;
	Candidate.PlanId = MakePlanId(Candidate.Request, Candidate.Policy);
	if (!Candidate.IsValid())
	{
		return false;
	}

	OutPlan = Candidate;
	return true;
}

bool Fdemo_mapShanmenSpiritEvasionMovementAdapter::
IsResolvedDistanceAccepted(
	const Fdemo_mapShanmenSpiritEvasionMovementPlan& Plan,
	float ResolvedDistance)
{
	return Plan.IsValid()
		&& FMath::IsFinite(ResolvedDistance)
		&& ResolvedDistance + KINDA_SMALL_NUMBER
			>= Plan.GetPolicy().GetMinimumResolvedDistance()
		&& ResolvedDistance
			<= Plan.GetPolicy().GetRequestedDistance() + KINDA_SMALL_NUMBER;
}

Fdemo_mapShanmenSpiritEvasionMovementPreflightResult
Fdemo_mapShanmenSpiritEvasionMovementAdapter::PreflightWorldStatic(
	const ACharacter* Character,
	const Fdemo_mapShanmenSpiritEvasionMovementPlan& Plan)
{
	Fdemo_mapShanmenSpiritEvasionMovementPreflightResult Result;
	if (!Plan.IsValid())
	{
		return Result;
	}

	Result.PlanId = Plan.GetPlanId();
	if (!Character || !Character->GetWorld())
	{
		Result.Status =
			Edemo_mapShanmenSpiritEvasionMovementPreflightStatus::
			CharacterUnavailable;
		return Result;
	}

	Result.Displacement = Fdemo_mapCombatDisplacement::PreflightWorldStatic(
		Character,
		Plan.GetRequest().GetIntent().GetPlanarDirection(),
		Plan.GetPolicy().GetRequestedDistance(),
		Plan.GetPolicy().GetWorldStaticClearance());
	Result.Status = IsResolvedDistanceAccepted(
		Plan, Result.Displacement.ResolvedDistance)
		? Edemo_mapShanmenSpiritEvasionMovementPreflightStatus::Ready
		: Edemo_mapShanmenSpiritEvasionMovementPreflightStatus::
			InsufficientResolvedDistance;
	return Result;
}
