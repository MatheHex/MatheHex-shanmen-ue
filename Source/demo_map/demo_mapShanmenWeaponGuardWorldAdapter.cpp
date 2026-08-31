#include "demo_mapShanmenWeaponGuardWorldAdapter.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "ShanmenDeterministicId.h"

namespace
{
	FString GuidDigits(const FGuid& Value)
	{
		return Value.ToString(EGuidFormats::Digits);
	}

	FString DoubleBits(double Value)
	{
		uint64 Bits = 0;
		static_assert(sizeof(Bits) == sizeof(Value));
		FMemory::Memcpy(&Bits, &Value, sizeof(Bits));
		return FString::Printf(
			TEXT("%016llX"),
			static_cast<unsigned long long>(Bits));
	}

	double CanonicalZero(double Value)
	{
		return Value == 0.0 ? 0.0 : Value;
	}

	FVector CanonicalVector(const FVector& Value)
	{
		return FVector(
			CanonicalZero(Value.X),
			CanonicalZero(Value.Y),
			CanonicalZero(Value.Z));
	}

	bool IsFiniteVector(const FVector& Value)
	{
		return FMath::IsFinite(Value.X)
			&& FMath::IsFinite(Value.Y)
			&& FMath::IsFinite(Value.Z);
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

	bool RuntimeBindingsMatch(
		const FShanmenWeaponGuardWindow& Window,
		const FShanmenWeaponGuardArcPolicy& Policy,
		const FShanmenWeaponGuardTimingProjectionReceipt& TimingProjection)
	{
		return WindowsMatch(Window.GetOpenReceipt(), Policy.GetWindow())
			&& WindowsMatch(
				Window.GetOpenReceipt(),
				TimingProjection.GetPolicy().GetWindow());
	}

	FGuid MakeReceiptId(
		const FGuid& RunId,
		const FGuid& DefenderEntityId,
		const FGuid& ThreatEntityId,
		const FVector& DefenderLocation,
		const FVector& ThreatLocation,
		const FShanmenWeaponGuardThreatSample& Sample,
		const FShanmenWeaponGuardArcEvaluation& Evaluation)
	{
		if (!RunId.IsValid()
			|| !DefenderEntityId.IsValid()
			|| !ThreatEntityId.IsValid()
			|| DefenderEntityId == ThreatEntityId
			|| !IsFiniteVector(DefenderLocation)
			|| !IsFiniteVector(ThreatLocation)
			|| !Sample.IsValid()
			|| !Evaluation.IsValid())
		{
			return FGuid();
		}
		return FShanmenDeterministicId::FromCanonicalParts(
			TEXT("Shanmen.Sword.WeaponGuard.WorldEvaluation.r1"),
			{
				GuidDigits(RunId),
				GuidDigits(DefenderEntityId),
				GuidDigits(ThreatEntityId),
				DoubleBits(DefenderLocation.X),
				DoubleBits(DefenderLocation.Y),
				DoubleBits(DefenderLocation.Z),
				DoubleBits(ThreatLocation.X),
				DoubleBits(ThreatLocation.Y),
				DoubleBits(ThreatLocation.Z),
				GuidDigits(Sample.GetSampleId()),
				GuidDigits(Evaluation.GetEvaluationId())
			});
	}

	Fdemo_mapShanmenWeaponGuardWorldResult Reject(
		Edemo_mapShanmenWeaponGuardWorldStatus Status,
		const TCHAR* Diagnostic)
	{
		Fdemo_mapShanmenWeaponGuardWorldResult Result;
		Result.Status = Status;
		Result.Diagnostic = Diagnostic;
		return Result;
	}
}

bool Fdemo_mapShanmenWeaponGuardWorldResult::IsSuccess() const
{
	if (Status != Edemo_mapShanmenWeaponGuardWorldStatus::Evaluated
		|| !ReceiptId.IsValid()
		|| !RunId.IsValid()
		|| !DefenderEntityId.IsValid()
		|| !ThreatEntityId.IsValid()
		|| DefenderEntityId == ThreatEntityId
		|| !IsFiniteVector(DefenderLocation)
		|| !IsFiniteVector(ThreatLocation)
		|| !Sample.IsValid()
		|| !Evaluation.IsValid()
		|| Sample.GetCandidate().TargetEntityId != DefenderEntityId
		|| Sample.GetCandidate().SourceEntityId != ThreatEntityId
		|| Evaluation.GetSample().GetSampleId() != Sample.GetSampleId())
	{
		return false;
	}

	FVector ExpectedDirection = ThreatLocation - DefenderLocation;
	if (!ExpectedDirection.Normalize()
		|| CanonicalVector(ExpectedDirection)
			!= Sample.GetDirectionToThreat())
	{
		return false;
	}
	return ReceiptId == MakeReceiptId(
		RunId,
		DefenderEntityId,
		ThreatEntityId,
		DefenderLocation,
		ThreatLocation,
		Sample,
		Evaluation);
}

Fdemo_mapShanmenWeaponGuardWorldResult
Fdemo_mapShanmenWeaponGuardWorldAdapter::Evaluate(
	UWorld* World,
	const FShanmenWorldEntityRegistry& EntityRegistry,
	AActor* DefenderActor,
	AActor* ThreatActor,
	const FShanmenWeaponGuardWindow& Window,
	const FShanmenActionOrchestrator& ActionRuntime,
	const FShanmenWeaponGuardArcPolicy& Policy,
	const FShanmenWeaponGuardTimingProjectionReceipt& TimingProjection,
	const FShanmenHitCandidate& Candidate)
{
	if (!Window.IsValid()
		|| !ActionRuntime.IsValid()
		|| !Policy.IsValid()
		|| !TimingProjection.IsValid()
		|| !Candidate.IsValid())
	{
		return Reject(
			Edemo_mapShanmenWeaponGuardWorldStatus::RuntimeInputInvalid,
			TEXT("Weapon guard World evaluation requires valid frozen Runtime inputs."));
	}
	if (!RuntimeBindingsMatch(Window, Policy, TimingProjection)
		|| !Window.IsActiveFor(ActionRuntime))
	{
		return Reject(
			Edemo_mapShanmenWeaponGuardWorldStatus::ArcBindingRejected,
			TEXT("Weapon guard World evaluation requires one exact active guard chain."));
	}
	if (!::IsValid(World))
	{
		return Reject(
			Edemo_mapShanmenWeaponGuardWorldStatus::WorldInvalid,
			TEXT("Weapon guard direction sampling requires one live caller-owned World."));
	}

	const FGuid RunId = Window.GetOpenReceipt().GetAction().GetRunId();
	if (!EntityRegistry.GetRunId().IsValid())
	{
		return Reject(
			Edemo_mapShanmenWeaponGuardWorldStatus::RegistryInactive,
			TEXT("Weapon guard direction sampling requires one active entity registry."));
	}
	if (EntityRegistry.GetRunId() != RunId)
	{
		return Reject(
			Edemo_mapShanmenWeaponGuardWorldStatus::RunMismatch,
			TEXT("Guard action and entity registry must belong to the same Run."));
	}
	if (!::IsValid(DefenderActor) || DefenderActor->IsActorBeingDestroyed())
	{
		return Reject(
			Edemo_mapShanmenWeaponGuardWorldStatus::DefenderUnavailable,
			TEXT("The defending Actor must remain live for the synchronous sample."));
	}
	if (!::IsValid(ThreatActor) || ThreatActor->IsActorBeingDestroyed())
	{
		return Reject(
			Edemo_mapShanmenWeaponGuardWorldStatus::ThreatUnavailable,
			TEXT("The threat Actor must remain live for the synchronous sample."));
	}
	if (DefenderActor->GetWorld() != World || ThreatActor->GetWorld() != World)
	{
		return Reject(
			Edemo_mapShanmenWeaponGuardWorldStatus::ActorWorldMismatch,
			TEXT("One weapon guard sample cannot mix Actors from another World."));
	}

	FGuid DefenderEntityId;
	if (!EntityRegistry.TryResolveObject(
			RunId, DefenderActor, INDEX_NONE, DefenderEntityId))
	{
		return Reject(
			Edemo_mapShanmenWeaponGuardWorldStatus::DefenderUnregistered,
			TEXT("The defending Actor requires an exact stable registry binding."));
	}
	FGuid ThreatEntityId;
	if (!EntityRegistry.TryResolveObject(
			RunId, ThreatActor, INDEX_NONE, ThreatEntityId))
	{
		return Reject(
			Edemo_mapShanmenWeaponGuardWorldStatus::ThreatUnregistered,
			TEXT("The threat Actor requires an exact stable registry binding."));
	}
	if (DefenderEntityId == ThreatEntityId)
	{
		return Reject(
			Edemo_mapShanmenWeaponGuardWorldStatus::IdentityConflict,
			TEXT("Defender and threat must resolve to distinct stable entities."));
	}
	if (DefenderEntityId
		!= Window.GetOpenReceipt().GetAction().GetSourceEntityId()
		|| Candidate.TargetEntityId != DefenderEntityId)
	{
		return Reject(
			Edemo_mapShanmenWeaponGuardWorldStatus::DefenderIdentityMismatch,
			TEXT("Defending Actor, guard action and candidate target must agree."));
	}
	if (Candidate.SourceEntityId != ThreatEntityId)
	{
		return Reject(
			Edemo_mapShanmenWeaponGuardWorldStatus::ThreatIdentityMismatch,
			TEXT("Threat Actor and candidate source must agree."));
	}

	const FVector DefenderLocation = CanonicalVector(
		DefenderActor->GetActorLocation());
	const FVector ThreatLocation = CanonicalVector(
		ThreatActor->GetActorLocation());
	const FVector GuardFacing = DefenderActor->GetActorForwardVector();
	if (!IsFiniteVector(DefenderLocation)
		|| !IsFiniteVector(ThreatLocation)
		|| !IsFiniteVector(GuardFacing))
	{
		return Reject(
			Edemo_mapShanmenWeaponGuardWorldStatus::TransformInvalid,
			TEXT("Sampled Actor transforms must remain finite."));
	}

	FShanmenWeaponGuardThreatSample Sample;
	if (!FShanmenWeaponGuardThreatSample::TryCapture(
			Candidate,
			GuardFacing,
			ThreatLocation - DefenderLocation,
			Sample))
	{
		return Reject(
			Edemo_mapShanmenWeaponGuardWorldStatus::DirectionSampleRejected,
			TEXT("Live transforms did not produce a canonical guard direction sample."));
	}
	FShanmenWeaponGuardArcEvaluation Evaluation;
	if (!FShanmenWeaponGuardArcEvaluator::TryEvaluate(
			Window,
			ActionRuntime,
			Policy,
			TimingProjection,
			Sample,
			Evaluation))
	{
		return Reject(
			Edemo_mapShanmenWeaponGuardWorldStatus::ArcEvaluationRejected,
			TEXT("P11.2 rejected the sampled direction against the frozen guard chain."));
	}

	Fdemo_mapShanmenWeaponGuardWorldResult Result;
	Result.Status = Edemo_mapShanmenWeaponGuardWorldStatus::Evaluated;
	Result.RunId = RunId;
	Result.DefenderEntityId = DefenderEntityId;
	Result.ThreatEntityId = ThreatEntityId;
	Result.DefenderLocation = DefenderLocation;
	Result.ThreatLocation = ThreatLocation;
	Result.Sample = Sample;
	Result.Evaluation = Evaluation;
	Result.ReceiptId = MakeReceiptId(
		Result.RunId,
		Result.DefenderEntityId,
		Result.ThreatEntityId,
		Result.DefenderLocation,
		Result.ThreatLocation,
		Result.Sample,
		Result.Evaluation);
	Result.Diagnostic = Result.Evaluation.IsQualified()
		? TEXT("Live registered Actors qualified the selected weapon guard layer.")
		: TEXT("Live registered Actors produced an auditable outside-arc result.");
	if (!Result.IsSuccess())
	{
		return Reject(
			Edemo_mapShanmenWeaponGuardWorldStatus::ArcEvaluationRejected,
			TEXT("Weapon guard World evaluation produced invalid staged evidence."));
	}
	return Result;
}
