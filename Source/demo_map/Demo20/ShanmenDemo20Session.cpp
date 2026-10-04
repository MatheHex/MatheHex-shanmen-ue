#include "ShanmenDemo20Session.h"

#include "ShanmenBasicSwordExecution.h"
#include "ShanmenCombatTags.h"
#include "ShanmenWorldEntityRegistry.h"

bool FShanmenDemo20Session::Begin(const FGuid& NewRunId)
{
	if (Phase != EShanmenDemo20Phase::Preparation || !NewRunId.IsValid() || NewRunId == RunId)
	{
		return false;
	}
	TArray<FShanmenVitalityAuthority> Candidate;
	for (int32 Index = 0; Index <= SentinelCount; ++Index)
	{
		FShanmenVitalityAuthority Vitality;
		const float Health = Index == 0 ? 100.f : 78.f;
		if (!FShanmenVitalityAuthority::TryCreate(
			FShanmenWorldEntityIdFactory::MakeEntityId(NewRunId, TEXT("Demo20.Arena"), Index),
			Health, Health, 0, Vitality)) return false;
		Candidate.Add(MoveTemp(Vitality));
	}
	Vitalities = MoveTemp(Candidate);
	RunId = NewRunId;
	Sequence = 0;
	Elapsed = AttackCooldown = EvadeWindow = EvadeCooldown = 0.f;
	bGuarding = false;
	Phase = EShanmenDemo20Phase::Active;
	return true;
}

void FShanmenDemo20Session::Advance(float DeltaSeconds)
{
	if (Phase != EShanmenDemo20Phase::Active || !FMath::IsFinite(DeltaSeconds) || DeltaSeconds <= 0.f) return;
	Elapsed += DeltaSeconds;
	AttackCooldown = FMath::Max(0.f, AttackCooldown - DeltaSeconds);
	EvadeWindow = FMath::Max(0.f, EvadeWindow - DeltaSeconds);
	EvadeCooldown = FMath::Max(0.f, EvadeCooldown - DeltaSeconds);
}

bool FShanmenDemo20Session::StrikeSentinel(int32 Index)
{
	if (Phase != EShanmenDemo20Phase::Active || Index < 0 || Index >= SentinelCount
		|| AttackCooldown > 0.f || IsEvading() || bGuarding || GetHealth(Index + 1) <= 0.f) return false;
	if (!ResolveContact(0, Index + 1, 26.f)) return false;
	AttackCooldown = 0.38f;
	return true;
}

bool FShanmenDemo20Session::ReceiveSentinelStrike(int32 Index)
{
	if (Phase != EShanmenDemo20Phase::Active || Index < 0 || Index >= SentinelCount || GetHealth(Index + 1) <= 0.f) return false;
	if (!ResolveContact(Index + 1, 0, 18.f)) return false;
	if (GetHealth() <= 0.f)
	{
		Phase = EShanmenDemo20Phase::Defeated;
		bGuarding = false;
	}
	return true;
}

bool FShanmenDemo20Session::ResolveContact(int32 SourceIndex, int32 TargetIndex, float Damage)
{
	if (!Vitalities.IsValidIndex(SourceIndex) || !Vitalities.IsValidIndex(TargetIndex) || Sequence == MAX_uint64) return false;
	FShanmenCombatActionCapture Capture;
	Capture.RunId = RunId;
	Capture.OwnerId = Vitalities[SourceIndex].GetTargetEntityId();
	Capture.SourceEntityId = Capture.OwnerId;
	Capture.ActionDefinitionId = FShanmenBasicSwordDefinition::CanonicalActionDefinitionId();
	Capture.ActivationId = FShanmenCombatIdFactory::MakeActivationId(RunId, Capture.SourceEntityId, Capture.ActionDefinitionId, ++Sequence);
	Capture.Content.Version = TEXT("Demo20.Slice01");
	Capture.Content.Digest = TEXT("Arena-3x78-Sword26-Sentinel18-Guard75-Evade035-v1");
	FShanmenCombatActionSnapshot Action;
	FShanmenBasicSwordDefinitionCapture DefinitionCapture;
	DefinitionCapture.ActionDefinitionId = Capture.ActionDefinitionId;
	DefinitionCapture.DetectorId = TEXT("Demo20.Contact");
	DefinitionCapture.FormulaId = TEXT("Demo20.Sword.r1");
	DefinitionCapture.BaseDamage = Damage;
	DefinitionCapture.DamageTags.AddTag(FShanmenCombatNativeTags::DamagePhysicalSlash());
	DefinitionCapture.RequiredTargetTags.AddTag(FShanmenCombatNativeTags::TargetLiving());
	FShanmenBasicSwordDefinition Definition;
	FShanmenBasicSwordOffenseSnapshot Offense;
	FShanmenBasicSwordExecution Execution;
	FShanmenActionOrchestrator Orchestrator;
	FShanmenActionTransitionReceipt Transition;
	if (!FShanmenCombatActionSnapshot::TryCapture(Capture, Action)
		|| !FShanmenBasicSwordDefinition::TryCapture(DefinitionCapture, Definition)
		|| !FShanmenBasicSwordOffenseSnapshot::TryCapture(0.f, Offense)
		|| !FShanmenActionOrchestrator::TryStart(Action, Orchestrator, Transition)
		|| !FShanmenBasicSwordExecution::TryCreate(Action, Definition, Offense, Execution)
		|| !Orchestrator.TryAdvance(EShanmenCombatActionPhase::Startup, Transition)) return false;
	FShanmenWorldHitContext Context;
	if (!Execution.TryBeginEmission(Orchestrator, Context)) return false;
	FShanmenHitCandidate Candidate;
	Candidate.ActivationId = Capture.ActivationId;
	Candidate.SourceEntityId = Capture.SourceEntityId;
	Candidate.TargetEntityId = Vitalities[TargetIndex].GetTargetEntityId();
	Candidate.DetectorId = Context.GetDetectorId();
	Candidate.DetectorKind = Context.GetDetectorKind();
	Candidate.HitOrdinal = Context.GetHitOrdinal();
	FShanmenTargetVitalitySnapshot Vitality;
	FShanmenDefenseSnapshot Defense;
	Defense.TargetTags.AddTag(FShanmenCombatNativeTags::TargetLiving());
	if (TargetIndex == 0 && (IsEvading() || bGuarding))
	{
		FShanmenDefenseLayer Layer;
		Layer.LayerId = FShanmenWorldEntityIdFactory::MakeEntityId(RunId, TEXT("Demo20.Defense"), IsEvading() ? 0 : 1);
		Layer.SourceInstanceId = Candidate.TargetEntityId;
		Layer.RuleId = IsEvading() ? TEXT("Demo20.Evasion") : TEXT("Demo20.Guard");
		Layer.Operation = IsEvading() ? EShanmenDefenseOperation::PreventAll : EShanmenDefenseOperation::ReduceFraction;
		Layer.Order = IsEvading() ? FShanmenDefenseOrder::Avoidance : FShanmenDefenseOrder::Guard;
		Layer.Magnitude = IsEvading() ? 0.f : 0.75f;
		Layer.LayerTags.AddTag(IsEvading() ? FShanmenCombatNativeTags::DefenseEvade() : FShanmenCombatNativeTags::DefenseGuard());
		Defense.Layers.Add(Layer);
	}
	FShanmenBasicSwordImpactReceipt Impact;
	FShanmenVitalityCommitCommand Command;
	return Vitalities[TargetIndex].TryCaptureSnapshot(Vitality)
		&& Execution.TryResolveCandidate(Orchestrator, Candidate, Vitality, Defense, Impact)
		&& FShanmenVitalityCommitCommand::TryCreate(Impact.GetRequest(), Impact.GetResult(), Command)
		&& Vitalities[TargetIndex].Commit(Command).IsSuccess();
}

bool FShanmenDemo20Session::TryEvade()
{
	if (Phase != EShanmenDemo20Phase::Active || EvadeCooldown > 0.f) return false;
	bGuarding = false;
	EvadeWindow = 0.35f;
	EvadeCooldown = 1.4f;
	return true;
}

void FShanmenDemo20Session::SetGuarding(bool bHeld)
{
	bGuarding = bHeld && Phase == EShanmenDemo20Phase::Active && !IsEvading() && AttackCooldown <= 0.f;
}

bool FShanmenDemo20Session::TryExtract()
{
	if (Phase != EShanmenDemo20Phase::Active || NumDefeated() != SentinelCount) return false;
	Phase = EShanmenDemo20Phase::Extracted;
	bGuarding = false;
	return true;
}

void FShanmenDemo20Session::Abandon()
{
	if (Phase == EShanmenDemo20Phase::Active)
	{
		Phase = EShanmenDemo20Phase::Abandoned;
		bGuarding = false;
	}
}

bool FShanmenDemo20Session::ReturnToPreparation()
{
	if (Phase == EShanmenDemo20Phase::Active || Phase == EShanmenDemo20Phase::Preparation) return false;
	Vitalities.Reset();
	Phase = EShanmenDemo20Phase::Preparation;
	bGuarding = false;
	return true;
}

float FShanmenDemo20Session::GetHealth(int32 EntityIndex) const
{
	return Vitalities.IsValidIndex(EntityIndex) ? Vitalities[EntityIndex].GetCurrentVitality() : 0.f;
}

int32 FShanmenDemo20Session::NumDefeated() const
{
	int32 Count = 0;
	for (int32 Index = 1; Index < Vitalities.Num(); ++Index) if (GetHealth(Index) <= 0.f) ++Count;
	return Count;
}

int32 FShanmenDemo20Session::GetImpactCount() const
{
	int32 Count = 0;
	for (const auto& Vitality : Vitalities) Count += Vitality.NumCommittedImpacts();
	return Count;
}
