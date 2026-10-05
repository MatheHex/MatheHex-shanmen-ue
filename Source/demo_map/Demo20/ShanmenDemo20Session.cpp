#include "ShanmenDemo20Session.h"

#include "ShanmenBasicSwordExecution.h"
#include "ShanmenCombatTags.h"
#include "ShanmenWorldEntityRegistry.h"

bool FShanmenDemo20CombatCheckpoint::IsValid() const
{
	if (!RunId.IsValid() || Sequence == MAX_uint64 || (Phase != EShanmenDemo20Phase::Active
		&& Phase != EShanmenDemo20Phase::Extracted && Phase != EShanmenDemo20Phase::Defeated)
		|| !FMath::IsFinite(Elapsed) || Elapsed < 0.f || Elapsed > 86400.f
		|| !FMath::IsFinite(SwordDamage) || SwordDamage < 1.f || SwordDamage > 100.f
		|| !FMath::IsFinite(ArmorFraction) || ArmorFraction < 0.f || ArmorFraction > .9f) return false;
	for (float Timer : {AttackCooldown, EvadeCooldown, EvadeWindow})
		if (!FMath::IsFinite(Timer) || Timer < 0.f || Timer > 2.f) return false;
	FShanmenDemo20EnemySpec Specs[FShanmenDemo20Encounters::Count];
	if (!FShanmenDemo20Encounters::Build(RunId,EncounterRevision,Specs)) return false;
	for (int32 I = 0; I < 4; ++I) if (!FMath::IsFinite(Health[I]) || Health[I] < 0.f || Health[I] > (I==0?100.f:Specs[I-1].Health)
		|| Revisions[I] < 0 || Revisions[I] == MAX_int64) return false;
	return (Phase == EShanmenDemo20Phase::Defeated) == (Health[0] == 0.f);
}

bool FShanmenDemo20Session::BeginExpedition(const FGuid& Id, float Damage, float Armor, int32 Revision)
{
	if (Phase != EShanmenDemo20Phase::Preparation || Id == RunId) return false;
	FShanmenDemo20CombatCheckpoint C; C.RunId = Id; C.SwordDamage = Damage; C.ArmorFraction = Armor;
	FShanmenDemo20EnemySpec Specs[FShanmenDemo20Encounters::Count];
	if (!FShanmenDemo20Encounters::Build(Id,Revision,Specs)) return false;
	C.EncounterRevision=Revision;
	for (int32 I=0;I<SentinelCount;++I) C.Health[I+1]=Specs[I].Health;
	return RestoreExpedition(C);
}

bool FShanmenDemo20Session::RestoreExpedition(const FShanmenDemo20CombatCheckpoint& C)
{
	if (!C.IsValid()) return false;
	TArray<FShanmenVitalityAuthority> Candidate;
	FShanmenDemo20EnemySpec Specs[FShanmenDemo20Encounters::Count];
	if (!FShanmenDemo20Encounters::Build(C.RunId,C.EncounterRevision,Specs)) return false;
	for (int32 I = 0; I < 4; ++I)
	{
		FShanmenVitalityAuthority V;
		if (!FShanmenVitalityAuthority::TryCreate(FShanmenWorldEntityIdFactory::MakeEntityId(C.RunId, TEXT("Demo20.Expedition"), I),
			C.Health[I], I==0?100.f:Specs[I-1].Health, C.Revisions[I], V)) return false;
		Candidate.Add(MoveTemp(V));
	}
	Vitalities = MoveTemp(Candidate); RunId = C.RunId; Phase = C.Phase; Sequence = C.Sequence;
	Elapsed = C.Elapsed; AttackCooldown = C.AttackCooldown; EvadeCooldown = C.EvadeCooldown; EvadeWindow = C.EvadeWindow;
	ExpeditionSwordDamage = C.SwordDamage; ExpeditionArmorFraction = C.ArmorFraction;
	ExpeditionEncounterRevision=C.EncounterRevision;
	for (int32 I=0;I<SentinelCount;++I) EnemySpecs[I]=Specs[I];
	bExpedition = true; bGuarding = false; return true;
}

bool FShanmenDemo20Session::CaptureExpedition(FShanmenDemo20CombatCheckpoint& Out) const
{
	if (!bExpedition || Vitalities.Num() != 4) return false;
	FShanmenDemo20CombatCheckpoint C; C.RunId = RunId; C.Phase = Phase; C.Sequence = Sequence;
	C.Elapsed = Elapsed; C.AttackCooldown = AttackCooldown; C.EvadeCooldown = EvadeCooldown; C.EvadeWindow = EvadeWindow;
	C.SwordDamage = ExpeditionSwordDamage; C.ArmorFraction = ExpeditionArmorFraction;
	C.EncounterRevision=ExpeditionEncounterRevision;
	for (int32 I = 0; I < 4; ++I) { C.Health[I] = GetHealth(I); C.Revisions[I] = Vitalities[I].GetAuthorityRevision(); }
	if (!C.IsValid()) return false;
	Out = C; return true;
}

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
	bExpedition = false;
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
	if (!ResolveContact(0, Index + 1, bExpedition ? ExpeditionSwordDamage : 26.f)) return false;
	AttackCooldown = 0.38f;
	return true;
}

bool FShanmenDemo20Session::ReceiveSentinelStrike(int32 Index)
{
	if (Phase != EShanmenDemo20Phase::Active || Index < 0 || Index >= SentinelCount || GetHealth(Index + 1) <= 0.f) return false;
	if (!ResolveContact(Index + 1, 0, bExpedition ? EnemySpecs[Index].Damage : 18.f)) return false;
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
	if (bExpedition) { Capture.Content.Version = TEXT("Demo20.Expedition.Combat.r1"); Capture.Content.Digest = TEXT("ThreeZones-MeleeRangedElite-FixedGear-r1"); }
	if (bExpedition && ExpeditionEncounterRevision==2)
	{ Capture.Content.Version=TEXT("Demo20.Expedition.Combat.r2"); Capture.Content.Digest=TEXT("ThreeZones-SeededOrdinary-FixedElite-FixedGear-r2"); }
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
	if (bExpedition && TargetIndex == 0)
	{
		FShanmenDefenseLayer Layer;
		Layer.LayerId = FShanmenWorldEntityIdFactory::MakeEntityId(RunId, TEXT("Demo20.Armor"), 0);
		Layer.SourceInstanceId = Candidate.TargetEntityId; Layer.RuleId = TEXT("Demo20.FixedArmor");
		Layer.Operation = EShanmenDefenseOperation::ReduceFraction; Layer.Order = FShanmenDefenseOrder::Resistance;
		Layer.LayerTags.AddTag(FShanmenCombatNativeTags::DefenseArmor());
		Layer.Magnitude = ExpeditionArmorFraction; Defense.Layers.Add(Layer);
	}
	FShanmenBasicSwordImpactReceipt Impact;
	FShanmenVitalityCommitCommand Command;
	return Vitalities[TargetIndex].TryCaptureSnapshot(Vitality)
		&& Execution.TryResolveCandidate(Orchestrator, Candidate, Vitality, Defense, Impact)
		&& FShanmenVitalityCommitCommand::TryCreate(Impact.GetRequest(), Impact.GetResult(), Command)
		&& Vitalities[TargetIndex].Commit(Command).IsSuccess();
}

bool FShanmenDemo20Session::TryUseMedicine(FString& Reason)
{
	if (!bExpedition || Phase != EShanmenDemo20Phase::Active || GetHealth() <= 0.f)
	{ Reason = TEXT("治疗不可用：需要存活的正式探索角色。"); return false; }
	if (GetHealth() >= 100.f) { Reason = TEXT("生命已满，没有消耗回春丹。"); return false; }
	if (AttackCooldown > 0.f || IsEvading() || bGuarding)
	{ Reason = TEXT("当前动作未结束：请停止格挡并等待出剑、闪身或用药恢复。"); return false; }
	if (Sequence >= MAX_uint64-1 || !Vitalities[0].TryCommitExternalMutation(
		FMath::Min(100.f,GetHealth()+35.f),100.f,Vitalities[0].GetAuthorityRevision()))
	{ Reason = TEXT("生命状态校验失败，未执行治疗。"); return false; }
	++Sequence; AttackCooldown=.6f; Reason.Reset(); return true;
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
	if (Phase != EShanmenDemo20Phase::Active || (!bExpedition && NumDefeated() != SentinelCount)) return false;
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
