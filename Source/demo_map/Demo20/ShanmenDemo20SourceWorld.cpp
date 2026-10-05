#include "ShanmenDemo20World.h"
#include "demo_mapShanmenItemAuthoritySubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

void AShanmenDemo20GameMode::ApplySourceProjection()
{
	if (SourceMarkers.Num()!=6) return;
	const bool Active=Session.GetPhase()==EShanmenDemo20Phase::Active;
	if (Active && SourceLayoutRun!=Session.GetRunId())
	{
		for (int32 I=0;I<3;++I) { FVector Position;
			if (!FShanmenDemo20Sources::ChestPosition(Session.GetRunId(),WorldCheckpoint.RunSeed,I,Position)) return;
			SourceMarkers[I]->SetActorLocation(Position); }
		SourceLayoutRun=Session.GetRunId();
	}
	for (int32 I=0;I<6;++I)
	{
		bool Visible=Active;
		FVector Position=FVector::ZeroVector;
		if (I<3) Visible=Visible && SourceLayoutRun==Session.GetRunId();
		else { Visible=Visible && Session.GetHealth(I-2)<=0; Position=WorldCheckpoint.EnemyPositions[I-3]; Position.Z=18.f; }
		SourceMarkers[I]->SetActorHiddenInGame(!Visible);
		if (Visible && I>=3) SourceMarkers[I]->SetActorLocation(Position);
	}
}
FString AShanmenDemo20GameMode::GetSourceBody() const
{
	return SourceSearch.IsActive()?FString::Printf(TEXT("搜索  %.1f / 1.0 秒\n\n世界继续运行；受伤、失焦或取消会中断。\n搜索结束才保存内容，没有提前领取。"),SourceSearch.Clock)
		:FShanmenDemo20Sources::Preview(SourcePreview);
}
void AShanmenDemo20GameMode::CloseSourceSurface()
{
	SourceSearch.Cancel(); bSourcePreviewOpen=false; SourcePreview=FShanmenItemGeneratedSourceReceipt();
	if (Screen) RefreshSurface();
}
bool AShanmenDemo20GameMode::TryInteractSource()
{
	if (SourceMarkers.Num()!=6) return false;
	const auto* P=GetWorld()->GetFirstPlayerController(); const APawn* Pawn=P?P->GetPawn():nullptr;
	if (!Pawn) return false;
	int32 Closest=INDEX_NONE; float Distance=180.f;
	for (int32 I=0;I<6;++I)
	{
		if (SourceMarkers[I]->IsHidden()) continue;
		const float D=FVector::Dist2D(Pawn->GetActorLocation(),SourceMarkers[I]->GetActorLocation());
		FCollisionQueryParams Query(SCENE_QUERY_STAT(Demo20SourceSight),false,Pawn);
		if (D<=Distance && !GetWorld()->LineTraceTestByChannel(Pawn->GetActorLocation(),SourceMarkers[I]->GetActorLocation()+FVector(0,0,40),ECC_WorldStatic,Query))
		{ Closest=I; Distance=D; }
	}
	if (Closest==INDEX_NONE) return false;
	const FName SourceRole=Closest<3?FShanmenDemo20Sources::ChestRole(Closest):FShanmenDemo20Sources::EnemyRole(Closest-3);
	auto* A=GetGameInstance()->GetSubsystem<Udemo_mapShanmenItemAuthoritySubsystem>();
	const auto R=A?A->ReadGeneratedSource(A->GetBoundOwnerId(),Session.GetRunId(),SourceRole):FShanmenItemGeneratedSourceReadResult();
	if (R.Status==EShanmenItemGeneratedSourceReadStatus::Accepted) { ConfirmSourceSearch(SourceRole); return true; }
	if (R.Status!=EShanmenItemGeneratedSourceReadStatus::Absent || R.RunState!=EShanmenItemGeneratedSourceRunState::Active)
	{ Notice=TEXT("搜索状态尚未确认，未生成或领取物品。请重启恢复原局。"); NoticeTime=5; return true; }
	bExtracting=false; ExtractionClock=0;
	if (SourceSearch.Begin(SourceRole,Pawn->GetActorLocation(),Session.GetHealth()))
	{ Session.SetGuarding(false); RefreshSurface(); }
	return true;
}
void AShanmenDemo20GameMode::TickSourceSearch(float Delta)
{
	if (!SourceSearch.IsActive()) return;
	const auto* P=GetWorld()->GetFirstPlayerController(); const APawn* Pawn=P?P->GetPawn():nullptr;
	const FName SourceRole=SourceSearch.Role;
	const auto Step=SourceSearch.Advance(Delta,Pawn?Pawn->GetActorLocation():FVector::ZeroVector,Session.GetHealth(),Pawn && bProfileReady && !bPaused);
	if (Step==EShanmenDemo20SearchStep::Interrupted)
	{ Notice=TEXT("搜索已中断，没有领取或重抽物品。"); NoticeTime=4; RefreshSurface(); }
	else if (Step==EShanmenDemo20SearchStep::Complete) { SourceSearch.Cancel(); ConfirmSourceSearch(SourceRole); }
}
void AShanmenDemo20GameMode::ConfirmSourceSearch(FName SourceRole)
{
	auto* A=GetGameInstance()->GetSubsystem<Udemo_mapShanmenItemAuthoritySubsystem>();
	if (!A) { Notice=TEXT("搜索端口不可用，没有领取物品。"); RefreshSurface(); return; }
	FShanmenDemo20SourcePorts Ports;
	Ports.Read=[A](const auto& Owner,const auto& Run,FName Source){return A->ReadGeneratedSource(Owner,Run,Source);};
	Ports.Accept=[A](const auto& Request){return A->AcceptGeneratedSourceDurable(Request);};
	FString Reason; FShanmenItemGeneratedSourceReceipt Confirmed;
	const bool Success=FShanmenDemo20Sources::Resolve(Session.GetRunId(),WorldCheckpoint.RunSeed,SourceRole,Ports,Confirmed,Reason);
	bProfileReady=A->GetLifecycleState()==Edemo_mapShanmenItemAuthorityLifecycleState::Ready;
	if (Success) { SourcePreview=MoveTemp(Confirmed); bSourcePreviewOpen=true; Session.SetGuarding(false); }
	else { Notice=Reason; NoticeTime=6; if (!bProfileReady) bPaused=true; }
	UE_LOG(LogTemp,Display,TEXT("DEMO20_SOURCE_SEARCH Run=%s Role=%s Success=%d Source=%s Entries=%d"),
		*Session.GetRunId().ToString(),*SourceRole.ToString(),Success,*SourcePreview.GetSourceId().ToString(),SourcePreview.GetPlan().Entries.Num());
	RefreshSurface();
}
