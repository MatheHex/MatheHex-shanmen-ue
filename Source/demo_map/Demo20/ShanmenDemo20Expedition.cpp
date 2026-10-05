#include "ShanmenDemo20World.h"
#include "ShanmenDemo20Loadout.h"
#include "ShanmenDemo20Catalog.h"
#include "ShanmenDemo20Medicine.h"
#include "ShanmenItemRepository.h"
#include "ShanmenDeterministicId.h"
#include "demo_mapShanmenItemAuthoritySubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "HAL/FileManager.h"

namespace
{
	bool HasUnfinished(const FShanmenItemAuthoritySnapshot& S)
	{
		for (const auto& P : S.ProcessedRequests)
			if (P.Receipt.IsSuccess() && P.Receipt.Operation == EShanmenItemTransactionOperation::StartPreparedRun
				&& !S.ProcessedRequests.ContainsByPredicate([&](const auto& F) { return F.Receipt.IsSuccess()
					&& F.Receipt.Operation == EShanmenItemTransactionOperation::FinalizePreparedRun && F.Receipt.ReservationId == P.Receipt.ReservationId; })) return true;
		return false;
	}
}

bool AShanmenDemo20GameMode::BuildExpedition()
{
	const FLinearColor Stone(.23f,.30f,.28f), Dark(.06f,.11f,.095f), Jade(.12f,.48f,.36f), Gold(.58f,.43f,.19f);
	// One continuous floor and two partitions with wide central openings. Layout is fixed.
	if (!AddShape(FVector(2600,0,-35),FVector(72,22,.7f),Stone,true)) return false;
	for (int32 Side : {-1,1})
		if (!AddShape(FVector(2600,Side*1110,70),FVector(72,.35f,2.1f),Dark,true)) return false;
	for (float X : {-1010.f,6210.f}) if (!AddShape(FVector(X,0,70),FVector(.35f,22,2.1f),Dark,true)) return false;
	for (float X : {2050.f,4150.f}) for (int32 Side : {-1,1})
		if (!AddShape(FVector(X,Side*780,70),FVector(.35f,7.6f,2.1f),Dark,true)) return false;
	for (float X : {1000.f,3100.f,5200.f})
	{
		AddShape(FVector(X,0,1),FVector(13,13,.025f),Gold,false,true);
		AddShape(FVector(X,0,3),FVector(12.6f,12.6f,.025f),Stone,false,true);
		for (int32 Side : {-1,1}) AddShape(FVector(X,Side*890,90),FVector(.7f,.7f,1.8f),Dark,true,true);
	}
	for (int32 X = -3; X <= 23; ++X) AddShape(FVector(X*260.f,0,4),FVector(.05f,.7f,.025f),Jade,false);
	ExitMarker = AddShape(ExitLocation()+FVector(0,0,4),FVector(4.2f,4.2f,.08f),Jade,false,true);
	FShanmenDemo20WorldCheckpoint Fresh;
	const FLinearColor Colors[3] = {FLinearColor(.48f,.25f,.14f),FLinearColor(.29f,.27f,.51f),FLinearColor(.58f,.14f,.11f)};
	for (int32 I = 0; I < 3; ++I)
	{
		Sentinels.Add(AddShape(Fresh.EnemyPositions[I],I == 2 ? FVector(1.2f,1.2f,1.6f) : FVector(.8f,.8f,1.3f),Colors[I],true,true));
		Warnings.Add(AddShape(Fresh.EnemyPositions[I]*FVector(1,1,0)+FVector(0,0,8),FVector(4.8f,4.8f,.04f),FLinearColor(.65f,.12f,.035f),false,true));
		if (!Sentinels.Last() || !Warnings.Last()) return false;
		Sentinels.Last()->SetActorHiddenInGame(true); Sentinels.Last()->SetActorEnableCollision(false); Warnings.Last()->SetActorHiddenInGame(true);
	}
	for (int32 I=0;I<6;++I)
	{
		SourceMarkers.Add(AddShape(FVector::ZeroVector,I<3?FVector(.85f,.85f,.8f):FVector(.9f,.9f,.35f),Gold,false,I>=3));
		if (!SourceMarkers.Last()) return false;
		SourceMarkers.Last()->SetActorHiddenInGame(true);
	}
	auto* Sun = GetWorld()->SpawnActor<ADirectionalLight>(FVector(0,0,1000),FRotator(-55,-25,0));
	auto* Sky = GetWorld()->SpawnActor<ASkyLight>();
	if (!Sun || !Sky || !ExitMarker) return false;
	Sun->GetLightComponent()->SetIntensity(5.f); Sun->GetLightComponent()->SetLightColor(FLinearColor(1,.9f,.73f));
	Sky->GetLightComponent()->SetIntensity(1.2f); ArenaActors.Add(Sun); ArenaActors.Add(Sky); return true;
}

FShanmenDemo20WorldCheckpoint AShanmenDemo20GameMode::CaptureWorld(const FShanmenDemo20Session& Candidate) const
{
	auto C = WorldCheckpoint; C.ContentId = C.CurrentContentId(); Candidate.CaptureExpedition(C.Combat);
	C.RunSeed = C.SeedForRun(C.Combat.RunId);
	if (const auto* P = GetWorld()->GetFirstPlayerController()) if (const APawn* Pawn = P->GetPawn())
	{ C.PlayerPosition = Pawn->GetActorLocation(); C.PlayerYaw = Pawn->GetActorRotation().Yaw; }
	for (int32 I = 0; I < 3; ++I)
	{
		C.EnemyPositions[I] = Sentinels[I]->GetActorLocation(); C.EnemyClocks[I] = SentinelClocks[I];
		C.WarningTargets[I] = Warnings[I]->GetActorLocation();
	}
	return C;
}

bool AShanmenDemo20GameMode::SaveExpedition(const FShanmenDemo20Session& Candidate)
{
	if (WorldCheckpoint.Medicine.IsSet()) { bPaused=true; Notice=TEXT("治疗结果尚待确认，请重试原请求。探索不会覆盖该记录。"); return false; }
	const auto C = CaptureWorld(Candidate); FString Reason;
	if (!FShanmenDemo20WorldCheckpointStore::Save(WorldProfileRoot,WorldCheckpoint,C,Reason))
	{
		PendingCheckpoint = C; bCheckpointPending = true; bPaused = true; bExtracting = false; ExtractionClock = 0.f;
		Notice = Reason; UE_LOG(LogTemp,Error,TEXT("DEMO20_WORLD_SAVE_REJECT Run=%s Generation=%d Reason=%s"),*C.Combat.RunId.ToString(),WorldCheckpoint.Generation,*Reason);
		return false;
	}
	bCheckpointPending = false; CheckpointClock = 0.f;
	UE_LOG(LogTemp,Display,TEXT("DEMO20_WORLD_SAVED Run=%s Generation=%d Seq=%llu HP=%.1f Position=%s Phase=%d"),
		*WorldCheckpoint.Combat.RunId.ToString(),WorldCheckpoint.Generation,WorldCheckpoint.Combat.Sequence,WorldCheckpoint.Combat.Health[0],
		*WorldCheckpoint.PlayerPosition.ToString(),static_cast<int32>(WorldCheckpoint.Combat.Phase));
	return true;
}
bool AShanmenDemo20GameMode::RetryExpeditionCheckpoint()
{
	FString Reason;
	if (bCheckpointPending)
	{
		if (!FShanmenDemo20WorldCheckpointStore::Save(WorldProfileRoot,WorldCheckpoint,PendingCheckpoint,Reason)) { Notice=Reason; RefreshSurface(); return false; }
		bCheckpointPending=false;
	}
	if (!ResolvePendingMedicine()) { RefreshSurface(); return false; }
	if (!Session.RestoreExpedition(WorldCheckpoint.Combat)) { Notice=TEXT("战斗恢复校验失败，未继续探索。"); return false; }
	CheckpointClock=0.f; ApplyExpeditionProjection(); RefreshMedicineProjection(); return true;
}

void AShanmenDemo20GameMode::RefreshMedicineProjection()
{
	CarryMedicine=SecureMedicine=0; FShanmenItemAuthoritySnapshot S; FString Reason;
	TArray<FShanmenDemo20MedicineLine> Lines;
	if (!TryCaptureItems(S) || !FShanmenDemo20Medicine::Capture(S,Session.GetRunId(),Lines,Reason)) return;
	for (const auto& L:Lines) (L.Origin==EShanmenDemo20MedicineOrigin::PreparedCarry?CarryMedicine:SecureMedicine)+=L.Quantity;
}

bool AShanmenDemo20GameMode::ResolvePendingMedicine()
{
	if (!WorldCheckpoint.Medicine.IsSet()) return true;
	auto* A=GetGameInstance()->GetSubsystem<Udemo_mapShanmenItemAuthoritySubsystem>();
	if (!A) return false;
	FShanmenDemo20MedicinePorts P;
	P.Capture=[A](auto& S){return A->TryCaptureSnapshot(S);};
	P.PrepareRun=[A](const auto& R){return A->PreparePreparedRunQuantityIntentDurable(R);};
	P.FinalizeRun=[A](const auto& R){return A->FinalizePreparedRunQuantityIntentDurable(R);};
	P.Reserve=[A](const auto& R){return A->ReserveDurable(R);};
	P.Commit=[A](const auto& R){return A->CommitDurable(R);};
	FString Reason; const bool Confirmed=FShanmenDemo20Medicine::Recover(WorldProfileRoot,WorldCheckpoint,P,Reason);
	bProfileReady=A->GetLifecycleState()==Edemo_mapShanmenItemAuthorityLifecycleState::Ready;
	if (!Confirmed) { bPaused=true; bExtracting=false; ExtractionClock=0; Notice=Reason; }
	UE_LOG(LogTemp,Display,TEXT("DEMO20_MEDICINE_CONFIRM Run=%s Success=%d Generation=%d Seq=%llu HP=%.2f"),
		*WorldCheckpoint.Combat.RunId.ToString(),Confirmed,WorldCheckpoint.Generation,WorldCheckpoint.Combat.Sequence,WorldCheckpoint.Combat.Health[0]);
	return Confirmed;
}

void AShanmenDemo20GameMode::UseMedicine()
{
	if (!IsPlaying()) return;
	if (!bExpeditionMode) { Notice=TEXT("石庭练习不消耗持久物品；请进入青岚关探索使用丹药。"); NoticeTime=4; return; }
	FString Reason; auto Candidate=Session;
	if (!Candidate.TryUseMedicine(Reason))
	{
		Notice=Reason; NoticeTime=3;
		UE_LOG(LogTemp,Display,TEXT("DEMO20_MEDICINE_REJECT HP=%.2f Reason=%s"),Session.GetHealth(),*Reason); return;
	}
	FShanmenItemAuthoritySnapshot S; FShanmenDemo20WorldCheckpoint Intent;
	if (!TryCaptureItems(S) || !FShanmenDemo20Medicine::BuildIntent(CaptureWorld(Session),S,Intent,Reason))
	{ Notice=Reason.IsEmpty()?TEXT("物品状态未确认，未使用丹药。"):Reason; NoticeTime=4; return; }
	const float Before=Session.GetHealth();
	if (!FShanmenDemo20WorldCheckpointStore::Save(WorldProfileRoot,WorldCheckpoint,Intent,Reason))
	{
		PendingCheckpoint=Intent; bCheckpointPending=true; bPaused=true; bExtracting=false; ExtractionClock=0;
		Notice=Reason; RefreshSurface(); return;
	}
	if (!ResolvePendingMedicine()) { RefreshSurface(); return; }
	if (!Session.RestoreExpedition(WorldCheckpoint.Combat)) { bPaused=true; Notice=TEXT("治疗战斗状态校验失败，请重启恢复原局。"); RefreshSurface(); return; }
	CheckpointClock=0; RefreshMedicineProjection(); Notice=TEXT("回春丹已确认使用 · 生命恢复，携带数量已扣除。"); NoticeTime=3;
	UE_LOG(LogTemp,Display,TEXT("DEMO20_MEDICINE_USED Run=%s Before=%.2f After=%.2f Carry=%d Secure=%d"),
		*Session.GetRunId().ToString(),Before,Session.GetHealth(),CarryMedicine,SecureMedicine);
}

void AShanmenDemo20GameMode::ApplyExpeditionProjection()
{
	const bool Running = Session.GetPhase() != EShanmenDemo20Phase::Preparation;
	for (int32 I = 0; I < 3; ++I)
	{
		const bool Alive = Running && Session.GetHealth(I+1)>0.f;
		Sentinels[I]->SetActorHiddenInGame(!Alive); Sentinels[I]->SetActorEnableCollision(Alive);
		Warnings[I]->SetActorHiddenInGame(!Alive || WorldCheckpoint.EnemyClocks[I]<=0.f);
		if (Running)
		{
			Sentinels[I]->SetActorLocation(WorldCheckpoint.EnemyPositions[I]); SentinelClocks[I]=WorldCheckpoint.EnemyClocks[I];
			Warnings[I]->SetActorLocation(WorldCheckpoint.WarningTargets[I]);
			const float Diameter = I == 1 ? 2.4f : I == 2 ? 6.2f : 4.8f;
			Warnings[I]->SetActorScale3D(FVector(Diameter,Diameter,.04f));
		}
	}
	if (auto* P=GetWorld()->GetFirstPlayerController()) if (APawn* Pawn=P->GetPawn())
	{
		Pawn->SetActorLocation(Running ? WorldCheckpoint.PlayerPosition : ExitLocation()+FVector(0,0,100));
		if (Running) Pawn->SetActorRotation(FRotator(0,WorldCheckpoint.PlayerYaw,0));
	}
	ApplySourceProjection();
}

void AShanmenDemo20GameMode::RestoreExpeditionOnOpen()
{
	FShanmenItemAuthoritySnapshot S; FShanmenDemo20ActiveLoadout Active; FString Reason;
	if (!TryCaptureItems(S)) { bWorldReady=false; return; }
	if (!HasUnfinished(S)) { Notice=TEXT("青岚关探索 · 可先整备，携带确认后出发。归阵从开局可用。"); ApplyExpeditionProjection(); return; }
	if (!FShanmenDemo20Loadout::InspectActiveIdentity(S,Active,Reason)
		|| !FShanmenDemo20WorldCheckpointStore::Load(WorldProfileRoot,Active.RunId,WorldCheckpoint,Reason)
		|| WorldCheckpoint.RunSeed != Active.RunSeed || !Session.RestoreExpedition(WorldCheckpoint.Combat))
	{ bWorldReady=false; Notice=Reason.IsEmpty()?TEXT("探索恢复失败，原局未覆盖。"):Reason; return; }
	bTerminalConfirmed=false; ApplyExpeditionProjection();
	if (!ResolvePendingMedicine()) { bPaused=true; return; }
	if (!TryCaptureItems(S) || !FShanmenDemo20Loadout::InspectActive(S,Active,Reason)
		|| !Session.RestoreExpedition(WorldCheckpoint.Combat))
	{ bWorldReady=false; Notice=Reason.IsEmpty()?TEXT("携带恢复校验失败，原局未覆盖。"):Reason; return; }
	ApplyExpeditionProjection(); RefreshMedicineProjection();
	if (Session.GetPhase()!=EShanmenDemo20Phase::Active) FinalizeExpedition();
	else { bPaused=true; Notice=TEXT("原局已恢复：位置、生命和敌人状态保持。点击继续后行动；撤离读条需重新开始。"); }
	UE_LOG(LogTemp,Display,TEXT("DEMO20_WORLD_RESTORED Run=%s Generation=%d Seed=%llu HP=%.1f Position=%s"),
		*Active.RunId.ToString(),WorldCheckpoint.Generation,Active.RunSeed,Session.GetHealth(),*WorldCheckpoint.PlayerPosition.ToString());
}

void AShanmenDemo20GameMode::StartExpedition()
{
	if (!bWorldReady || !bProfileReady || Session.GetPhase()!=EShanmenDemo20Phase::Preparation) return;
	if (bInventoryOpen) ToggleInventory();
	FShanmenItemAuthoritySnapshot S; if (!TryCaptureItems(S)) return;
	if (HasUnfinished(S)) { RestoreExpeditionOnOpen(); RefreshSurface(); return; }
	FShanmenItemLoadoutStartRequest R; FString Reason;
	if (!FShanmenDemo20Loadout::Build(S,R,Reason)) { Notice=Reason; RefreshSurface(); return; }
	FShanmenItemRepository Preview; if (!Preview.TryLoadSnapshot(S)) return;
	const auto PreviewStart=Preview.StartLoadout(R);
	if (!PreviewStart.IsSuccess()) { Notice=TEXT("携带预检被拒绝，没有扣除物品。"); RefreshSurface(); return; }
	float Damage=26.f, Armor=.12f;
	for (const auto& I:S.Items)
	{
		if (I.ParentContainerId==FShanmenDemo20Catalog::ContainerId(TEXT("Weapon")) && I.DefinitionId==TEXT("Sword.Heavy")) Damage=34.f;
		if (I.ParentContainerId==FShanmenDemo20Catalog::ContainerId(TEXT("Armor")) && I.DefinitionId==TEXT("Armor.Leather")) Armor=.28f;
	}
	FShanmenDemo20Session Candidate;
	if (!Candidate.BeginExpedition(PreviewStart.ReservationId,Damage,Armor)) return;
	FShanmenDemo20WorldCheckpoint Initial; Initial.ContentId=Initial.CurrentContentId(); Initial.RunSeed=Initial.SeedForRun(PreviewStart.ReservationId);
	Candidate.CaptureExpedition(Initial.Combat);
	FShanmenDemo20WorldCheckpoint Confirmed;
	if (IFileManager::Get().FileExists(*FShanmenDemo20WorldCheckpointStore::Path(WorldProfileRoot,PreviewStart.ReservationId)))
	{
		if (!FShanmenDemo20WorldCheckpointStore::Load(WorldProfileRoot,PreviewStart.ReservationId,Confirmed,Reason)
			|| Confirmed.Generation!=1 || Confirmed.Combat.Sequence!=0 || Confirmed.Combat.Elapsed!=0.f
			|| Confirmed.Combat.Phase!=EShanmenDemo20Phase::Active || Confirmed.Combat.SwordDamage!=Damage || Confirmed.Combat.ArmorFraction!=Armor)
		{ Notice=TEXT("出发记录冲突，未启动或扣除；请查看运行日志。"); RefreshSurface(); return; }
	}
	else if (!FShanmenDemo20WorldCheckpointStore::Save(WorldProfileRoot,Confirmed,Initial,Reason)) { Notice=Reason; RefreshSurface(); return; }
	auto* Authority=GetGameInstance()->GetSubsystem<Udemo_mapShanmenItemAuthoritySubsystem>();
	const auto Started=Authority->StartLoadoutDurable(R);
	bProfileReady=Authority->GetLifecycleState()==Edemo_mapShanmenItemAuthorityLifecycleState::Ready;
	if (!Started.IsCommandSuccess()) { Notice=Started.IsDurable()?TEXT("物品权威拒绝出发，未进入关卡。请检查携带状态。"):
		TEXT("出发保存未确认。原档保留，可重试或重启恢复；没有假成功。"); RefreshSurface(); return; }
	Session=MoveTemp(Candidate); WorldCheckpoint=Confirmed; bPaused=false; bCheckpointPending=false; bTerminalConfirmed=false;
	bExtracting=false; ExtractionClock=CheckpointClock=0.f; ApplyExpeditionProjection();
	RefreshMedicineProjection();
	CloseSourceSurface();
	Notice=TEXT("已确认出发 · 金色宝匣和守卫遗物可搜索；暂为只读预览，领取尚未开放。归阵开局可用。"); NoticeTime=8.f;
	RefreshSurface(); UE_LOG(LogTemp,Display,TEXT("DEMO20_EXPEDITION_BEGIN Run=%s Seed=%llu ItemGeneration=%d"),
		*Session.GetRunId().ToString(),WorldCheckpoint.RunSeed,Started.DocumentGeneration);
}

bool AShanmenDemo20GameMode::FinalizeExpedition()
{
	CloseSourceSurface();
	if (!RetryExpeditionCheckpoint()) return false;
	const auto Phase=Session.GetPhase();
	if (Phase!=EShanmenDemo20Phase::Extracted && Phase!=EShanmenDemo20Phase::Defeated) return false;
	FShanmenItemAuthoritySnapshot S; if (!TryCaptureItems(S)) return false;
	const auto ReasonKind=Phase==EShanmenDemo20Phase::Extracted?EShanmenItemRunTerminalReason::Extraction:EShanmenItemRunTerminalReason::Death;
	FShanmenItemRunFinalizeRequest R; R.Context.OwnerId=FShanmenDemo20Catalog::OwnerId(); R.Context.RunId=FShanmenDemo20Catalog::ScopeId();
	R.Context.Content=S.Content; R.ActiveRunId=Session.GetRunId(); R.TerminalReason=ReasonKind;
	R.Context.RequestId=FShanmenDeterministicId::FromCanonicalParts(TEXT("Demo20.Expedition.Terminal.r1"),{R.ActiveRunId.ToString(),FString::FromInt(static_cast<int32>(ReasonKind))});
	if (const auto* Existing=S.ProcessedRequests.FindByPredicate([&](const auto& P){return P.Receipt.RequestId==R.Context.RequestId
		&& P.Receipt.IsSuccess() && P.Receipt.Operation==EShanmenItemTransactionOperation::FinalizePreparedRun && P.Receipt.ReservationId==R.ActiveRunId;}))
	{ bTerminalConfirmed=true; bPaused=false; Notice=TEXT("终局已确认保存，没有重复结算。"); return true; }
	FShanmenDemo20ActiveLoadout Active; FString Why;
	if (!FShanmenDemo20Loadout::InspectActive(S,Active,Why) || Active.RunId!=R.ActiveRunId)
	{ Notice=Why; bPaused=true; return false; }
	if (ReasonKind==EShanmenItemRunTerminalReason::Extraction) R.SecuredOriginals=Active.RemainingOriginals;
	auto* Authority=GetGameInstance()->GetSubsystem<Udemo_mapShanmenItemAuthoritySubsystem>();
	const auto Result=Authority->FinalizePreparedRunDurable(R);
	bProfileReady=Authority->GetLifecycleState()==Edemo_mapShanmenItemAuthorityLifecycleState::Ready;
	bTerminalConfirmed=Result.IsCommandSuccess(); bPaused=!bTerminalConfirmed;
	Notice=bTerminalConfirmed?(Phase==EShanmenDemo20Phase::Extracted?TEXT("撤离已保存 · 普通携带已带回；安全格和仓库保持。"):
		TEXT("死亡已保存 · 普通携带与装备损失；安全格和仓库保留。返回整备可领取有限补给。")):
		TEXT("终局尚未确认保存，未显示已带回。点击重试结算；原局不会另开或重复发物。");
	UE_LOG(LogTemp,Display,TEXT("DEMO20_EXPEDITION_TERMINAL Run=%s Reason=%d Success=%d Status=%d Error=%d Generation=%d"),
		*R.ActiveRunId.ToString(),static_cast<int32>(ReasonKind),bTerminalConfirmed,static_cast<int32>(Result.Status),static_cast<int32>(Result.Receipt.Error),Result.DocumentGeneration);
	return bTerminalConfirmed;
}

FString AShanmenDemo20GameMode::GetExplorationArea() const
{
	const auto* P=GetWorld()->GetFirstPlayerController(); const APawn* Pawn=P?P->GetPawn():nullptr;
	if (!Pawn || Pawn->GetActorLocation().X<0) return TEXT("归阵 · 安全入口");
	const float X=Pawn->GetActorLocation().X;
	return X<2050?TEXT("石径 · 近战守卫"):X<4150?TEXT("竹林 · 远程守卫"):TEXT("遗坛 · 精英守卫");
}

void AShanmenDemo20GameMode::TickExpedition(float Delta)
{
	if (Session.GetPhase()!=EShanmenDemo20Phase::Active || bPaused) return;
	Session.Advance(Delta); UpdateExpeditionEnemies(Delta);
	ApplySourceProjection();
	if (bPaused) { RefreshSurface(); return; }
	if (Session.GetPhase()==EShanmenDemo20Phase::Defeated) { FinalizeExpedition(); RefreshSurface(); return; }
	TickSourceSearch(Delta);
	if (bPaused) return;
	CheckpointClock+=Delta; NoticeTime=FMath::Max(0.f,NoticeTime-Delta);
	const auto* P=GetWorld()->GetFirstPlayerController(); const APawn* Pawn=P?P->GetPawn():nullptr;
	if (bExtracting)
	{
		if (!Pawn || FVector::Dist2D(Pawn->GetActorLocation(),ExitLocation())>210.f)
		{ bExtracting=false; ExtractionClock=0; Notice=TEXT("撤离中断：已离开归阵范围。"); NoticeTime=3; }
		else
		{
			ExtractionClock+=Delta; Notice=FString::Printf(TEXT("归阵撤离  %.1f / 3.0 秒 · 受伤或离开会中断"),FMath::Min(3.f,ExtractionClock));
			if (ExtractionClock>=3.f)
			{
				auto Candidate=Session; Candidate.TryExtract();
				if (!SaveExpedition(Candidate)) { RefreshSurface(); return; }
				Session=MoveTemp(Candidate); bExtracting=false; FinalizeExpedition(); RefreshSurface(); return;
			}
		}
	}
	else if (NoticeTime<=0) Notice=TEXT("归阵从开局可用。可提前撤离，或继续向东探索。");
	// Confirmed movement/timer recovery granularity <= 1 s; combat mutations save before publication.
	if (CheckpointClock>=1.f && !SaveExpedition(Session)) RefreshSurface();
}

void AShanmenDemo20GameMode::UpdateExpeditionEnemies(float Delta)
{
	const auto* P=GetWorld()->GetFirstPlayerController(); const APawn* Pawn=P?P->GetPawn():nullptr; if (!Pawn) return;
	for (int32 I=0; I<3 && Session.GetPhase()==EShanmenDemo20Phase::Active && !bPaused; ++I)
	{
		if (Session.GetHealth(I+1)<=0) continue;
		const FVector Enemy=Sentinels[I]->GetActorLocation(), Player=Pawn->GetActorLocation();
		const float Distance=FVector::Dist2D(Enemy,Player), Range=I==1?800.f:I==2?310.f:240.f;
		FCollisionQueryParams Sight(SCENE_QUERY_STAT(Demo20Sight),false,Sentinels[I]); Sight.AddIgnoredActor(Pawn);
		const bool Visible=!GetWorld()->LineTraceTestByChannel(Enemy+FVector(0,0,80),Player+FVector(0,0,80),ECC_WorldStatic,Sight);
		float& Clock=SentinelClocks[I];
		if (Clock<0) Clock=FMath::Min(0.f,Clock+Delta);
		if (Clock<=0 && Distance<1000.f && Distance>180.f)
		{
			FVector Direction=(Player-Enemy).GetSafeNormal2D();
			if (I==1 && Distance<400.f) Direction=-Direction;
			else if (I==1 && Distance<650.f) Direction=FVector::ZeroVector;
			Sentinels[I]->SetActorLocation(Enemy+Direction*(I==2?210.f:155.f)*Delta,true);
		}
		if (Clock==0 && Distance<Range && Visible)
		{
			Clock=.001f; Warnings[I]->SetActorLocation((I==1?Player:Sentinels[I]->GetActorLocation())*FVector(1,1,0)+FVector(0,0,8));
			const float Diameter=I==1?2.4f:I==2?6.2f:4.8f; Warnings[I]->SetActorScale3D(FVector(Diameter,Diameter,.04f));
			Warnings[I]->SetActorHiddenInGame(false);
		}
		if (Clock>0)
		{
			Clock+=Delta;
			if (Clock>=(I==2?1.1f:.85f))
			{
				const bool Hit=FVector::Dist2D(Player,Warnings[I]->GetActorLocation())<(I==1?120.f:Range) && Visible;
				Clock=-1.4f; Warnings[I]->SetActorHiddenInGame(true);
				if (Hit)
				{
					auto Candidate=Session; const float Before=Session.GetHealth();
					if (!Candidate.ReceiveSentinelStrike(I)) continue;
					if (!SaveExpedition(Candidate)) return;
					Session=MoveTemp(Candidate);
					if (Session.GetHealth()<Before) { bExtracting=false; ExtractionClock=0; }
					Notice=Session.GetHealth()==Before?TEXT("闪避成功"):Session.IsGuarding()?TEXT("格挡与护具减免 · 撤离读条已中断"):
						I==1?TEXT("受到远程攻击 · 侧移避开红色落点"):TEXT("受到攻击 · 可闪避或格挡，留意红色预警"); NoticeTime=2.f;
				}
			}
		}
	}
}
