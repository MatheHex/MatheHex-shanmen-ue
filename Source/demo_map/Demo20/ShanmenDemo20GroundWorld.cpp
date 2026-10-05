#include "ShanmenDemo20World.h"
#include "ShanmenDemo20Catalog.h"
#include "ShanmenDemo20Widget.h"
#include "demo_mapShanmenItemAuthoritySubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"

void AShanmenDemo20GameMode::ApplyGroundProjection()
{
	TArray<FShanmenItemGroundDropView> Views;
	if (IsRunInventory() && !IsMedicinePending())
	{
		FShanmenItemAuthoritySnapshot S;
		if (!TryCaptureItems(S) || !FShanmenItemGroundDropPolicy::Read(S,FShanmenDemo20Catalog::OwnerId(),FShanmenDemo20Catalog::ScopeId(),Session.GetRunId(),Views))
		{ bPaused=true; Notice=TEXT("地面物品投影未确认，原物品未重建。请恢复原局。"); RefreshSurface(); return; }
	}
	TSet<FGuid> Retained;
	for (const auto& V:Views)
	{
		Retained.Add(V.ContainerId); auto& Marker=GroundMarkers.FindOrAdd(V.ContainerId);
		if (!IsValid(Marker)) Marker=AddShape(FVector(V.Position),FVector(.45f,.45f,.24f),FLinearColor(.8f,.64f,.25f),false);
		if (!IsValid(Marker)) { bPaused=true; Notice=TEXT("地面行囊显示失败；物品已保存，不要重复丢弃。请恢复原局。"); continue; }
		Marker->SetActorLocation(FVector(V.Position));
		if (auto* Mesh=Cast<UStaticMeshComponent>(Marker->GetRootComponent()))
			if (auto* Material=Cast<UMaterialInstanceDynamic>(Mesh->GetMaterial(0)))
				Material->SetVectorParameterValue(TEXT("Color"),V.bEmpty?FLinearColor(.25f,.29f,.26f):FLinearColor(.8f,.64f,.25f));
	}
	for (auto It=GroundMarkers.CreateIterator();It;++It) if (!Retained.Contains(It.Key()))
	{
		if (IsValid(It.Value())) { ArenaActors.Remove(It.Value()); It.Value()->Destroy(); }
		It.RemoveCurrent();
	}
	if (bPaused) RefreshSurface();
}

FShanmenItemDurableCommandResult AShanmenDemo20GameMode::DropInventoryItem(const FGuid& Item,int32 ExpectedAuthorityRevision,int32 ExpectedItemRevision)
{
	FShanmenItemDurableCommandResult Rejected;
	if (!IsRunInventory() || bPaused || !bProfileReady || !bInventoryOpen || bCheckpointPending || IsMedicinePending() || Session.GetHealth()<=0)
	{ Rejected.Diagnostic=TEXT("当前不能丢弃：需要已确认的探索背包，且没有待确认动作或保存。"); return Rejected; }
	const auto* P=GetWorld()->GetFirstPlayerController(); const APawn* Pawn=P?P->GetPawn():nullptr;
	FHitResult Floor; FCollisionQueryParams Query(SCENE_QUERY_STAT(Demo20DropFloor),false,Pawn);
	if (!Pawn || !GetWorld()->LineTraceSingleByChannel(Floor,Pawn->GetActorLocation(),Pawn->GetActorLocation()-FVector(0,0,300),ECC_WorldStatic,Query)
		|| Floor.ImpactNormal.Z<.7f || Floor.ImpactPoint.ContainsNaN())
	{ Rejected.Diagnostic=TEXT("脚下没有确认的可交互地面，物品留在原格。"); return Rejected; }
	auto* A=GetGameInstance()->GetSubsystem<Udemo_mapShanmenItemAuthoritySubsystem>(); if (!A) return Rejected;
	FShanmenItemGroundDropRequest R; R.Context.OwnerId=A->GetBoundOwnerId(); R.Context.RunId=FShanmenDemo20Catalog::ScopeId();
	R.Context.Content=FShanmenDemo20Catalog::ContentStamp(); R.ActiveRunId=Session.GetRunId(); R.ItemInstanceId=Item;
	R.ExpectedAuthorityRevision=ExpectedAuthorityRevision; R.ExpectedItemRevision=ExpectedItemRevision;
	R.Context.RequestId=R.MakeRequestId(R.Context.OwnerId,R.Context.RunId,R.ActiveRunId,Item,ExpectedItemRevision);
	R.Position=FIntVector(FMath::RoundToInt(Floor.ImpactPoint.X),FMath::RoundToInt(Floor.ImpactPoint.Y),FMath::RoundToInt(Floor.ImpactPoint.Z+12));
	const auto Result=A->DropActiveRunItemDurable(R);
	bProfileReady=A->GetLifecycleState()==Edemo_mapShanmenItemAuthorityLifecycleState::Ready;
	if (Result.IsCommandSuccess()) { RefreshMedicineProjection(); ApplyGroundProjection(); }
	if (!bProfileReady) { bPaused=true; bInventoryOpen=false; CloseSourceSurface(); RefreshSurface(); }
	UE_LOG(LogTemp,Display,TEXT("DEMO20_GROUND_DROP Run=%s Item=%s Success=%d Status=%d Error=%d Generation=%d Position=%s"),
		*R.ActiveRunId.ToString(),*Item.ToString(),Result.IsCommandSuccess(),static_cast<int32>(Result.Status),static_cast<int32>(Result.Receipt.Error),Result.DocumentGeneration,*R.Position.ToString());
	return Result;
}

bool AShanmenDemo20GameMode::TryInteractGround()
{
	const auto* P=GetWorld()->GetFirstPlayerController(); const APawn* Pawn=P?P->GetPawn():nullptr;
	if (!Pawn || !IsRunInventory() || bPaused || !bProfileReady || IsMedicinePending()) return false;
	FShanmenItemAuthoritySnapshot S; TArray<FShanmenItemGroundDropView> Views;
	if (!TryCaptureItems(S) || !FShanmenItemGroundDropPolicy::Read(S,FShanmenDemo20Catalog::OwnerId(),FShanmenDemo20Catalog::ScopeId(),Session.GetRunId(),Views))
	{
		bPaused=true; Session.SetGuarding(false); bExtracting=false; ExtractionClock=0;
		Notice=TEXT("地面物品状态未确认，交互已暂停。请恢复原局；没有领取或重建物品。"); RefreshSurface(); return true;
	}
	FGuid Closest; float Distance=180.f; bool Empty=true;
	for (const auto& V:Views)
	{
		const float D=FVector::Dist2D(Pawn->GetActorLocation(),FVector(V.Position));
		FCollisionQueryParams Query(SCENE_QUERY_STAT(Demo20GroundSight),false,Pawn);
		if (D>180.f || GetWorld()->LineTraceTestByChannel(Pawn->GetActorLocation(),FVector(V.Position)+FVector(0,0,40),ECC_WorldStatic,Query)) continue;
		if (!Closest.IsValid() || (Empty && !V.bEmpty) || (Empty==V.bEmpty && D<Distance))
		{ Closest=V.ContainerId; Distance=D; Empty=V.bEmpty; }
	}
	if (!Closest.IsValid()) return false;
	// An empty ground marker must not trap the interaction key beside a chest.
	if (Empty) for (const auto& Marker:SourceMarkers) if (IsValid(Marker) && !Marker->IsHidden()
		&& FVector::Dist2D(Pawn->GetActorLocation(),Marker->GetActorLocation())<=180.f)
	{
		FCollisionQueryParams Query(SCENE_QUERY_STAT(Demo20EmptyGroundSight),false,Pawn);
		if (!GetWorld()->LineTraceTestByChannel(Pawn->GetActorLocation(),Marker->GetActorLocation()+FVector(0,0,40),ECC_WorldStatic,Query)) return false;
	}
	CloseSourceSurface(); OpenGroundContainer=Closest; bInventoryOpen=true; Session.SetGuarding(false); bExtracting=false; ExtractionClock=0;
	Notice=Empty?TEXT("地面行囊已拾空，不会重新生成物品。"):TEXT("地面行囊 · 拖入背包或安全格领取。关闭后未领取物仍在原地。"); NoticeTime=5;
	if (Screen) Screen->RefreshInventory(); RefreshSurface(); return true;
}
