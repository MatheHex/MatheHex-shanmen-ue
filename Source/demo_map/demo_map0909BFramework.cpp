#include "demo_map0909BFramework.h"

#include "demo_map0909BEditorSupport.h"
#include "demo_map0909BRunStartCoordinator.h"
#include "demo_map0909BSectWidget.h"
#include "demo_map0909BSectWarehouseService.h"
#include "demo_map.h"
#include "demo_mapGameMode.h"
#include "demo_mapPlayerController.h"
#include "demo_mapProfileSessionSubsystem.h"
#include "demo_mapShanmenItemAuthoritySubsystem.h"
#include "demo_mapShanmenItemCutover.h"
#include "Blueprint/UserWidget.h"
#include "Engine/GameInstance.h"

Ademo_map0909BFrameworkHost::Ademo_map0909BFrameworkHost()
{
	SetActorHiddenInGame(true);
	SetCanBeDamaged(false);
	PrimaryActorTick.bCanEverTick = false;
}

bool Ademo_map0909BFrameworkHost::InitializeForGame(
	Ademo_mapGameMode* InGameMode,
	Ademo_mapPlayerController* InController)
{
	if (bInitialized)
	{
		return GameMode.Get() == InGameMode && Controller.Get() == InController;
	}
	if (!InGameMode || !InGameMode->Is0909BRuntimeReady() || !InController)
	{
		return false;
	}
	GameMode = InGameMode;
	Controller = InController;
	StartCoordinator = MakeUnique<Fdemo_map0909BRunStartCoordinator>();
	StartCoordinator->Initialize(InGameMode, InController);
	WarehouseService = MakeUnique<Fdemo_map0909BSectWarehouseService>();
	TWeakObjectPtr<Ademo_map0909BFrameworkHost> WeakHost(this);
	InGameMode->Set0909BOutOfRaidClosedCallback([WeakHost]()
	{
		if (WeakHost.IsValid())
		{
			WeakHost->ShowSect(TEXT("已返回宗门；ShanmenItems 权威终局已经持久化。"));
		}
	});

	// Existing authority wins before either legacy source is inspected.  On a
	// first 0.0.10 launch the unopened warehouse intentionally fails this probe;
	// the stable sources are then opened once and the same coordinator publishes
	// generation one.
	FString CutoverDiagnostic;
	bool bCutoverReady = EnsureShanmenItemCutover(CutoverDiagnostic);
	FString WarehouseDiagnostic;
	const bool bWarehouseReady = OpenWarehouseService(WarehouseDiagnostic);
	if (!bCutoverReady && bWarehouseReady)
	{
		bCutoverReady = EnsureShanmenItemCutover(CutoverDiagnostic);
	}
	FString EditorDiagnostic;
	const bool bEditorEntryValid = Fdemo_map0909BEditorSupport::ValidateDefaultEntry(
		*InGameMode, EditorDiagnostic);
	const FGuid RecoverableRunId = InGameMode->Get0909BRecoverableRunId();
	const bool bRecoverable = RecoverableRunId.IsValid();
	bInitialized = true;
	ShowSect(bEditorEntryValid && bCutoverReady && bWarehouseReady
		? bRecoverable
			? FString::Printf(
				TEXT("检测到可恢复远征 %s；再次进入 M01 将重建同一 Runtime，不会创建第二个 Run。"),
				*RecoverableRunId.ToString(EGuidFormats::DigitsWithHyphens))
			: TEXT("已进入 0.0.10 宗门入口。ShanmenItems 是唯一物品权威；真实 M01 部署使用原子 Run-start。")
		: TEXT("入口、战备或 ShanmenItems cutover 诊断异常：")
			+ EditorDiagnostic + TEXT(" | ") + WarehouseDiagnostic
			+ TEXT(" | ") + CutoverDiagnostic);
	return bEditorEntryValid && bWarehouseReady && bCutoverReady;
}

void Ademo_map0909BFrameworkHost::RequestStartM01FromUI()
{
	if (!bInitialized || !StartCoordinator.IsValid())
	{
		ShowSect(TEXT("远征协调器尚未就绪。"));
		return;
	}
	// Code B selection is retained only as read-only audit correlation. The
	// actual prepared identities and resource transition come from ShanmenItems.
	FCodeBLoadoutSelection RecoverableSelection;
	FString RecoverableSelectionDiagnostic;
	const bool bHasRecoverableSelection = WarehouseService
		&& WarehouseService->CaptureLoadoutSelection(
			RecoverableSelection, RecoverableSelectionDiagnostic);
	FString WarehouseRefreshDiagnostic;
	const bool bWarehouseRefreshed =
		OpenWarehouseService(WarehouseRefreshDiagnostic);
	if (!bWarehouseRefreshed && !bHasRecoverableSelection)
	{
		ShowSect(TEXT("无法刷新当前 P5 战备快照：") + WarehouseRefreshDiagnostic);
		return;
	}
	FCodeBLoadoutSelection Selection;
	FString SelectionDiagnostic;
	if (bWarehouseRefreshed)
	{
		if (!WarehouseService
			|| !WarehouseService->CaptureLoadoutSelection(
				Selection, SelectionDiagnostic))
		{
			ShowSect(TEXT("无法取得当前 P5 战备快照：") + SelectionDiagnostic);
			return;
		}
	}
	else
	{
		Selection = RecoverableSelection;
	}
	FString Feedback;
	const bool bStarted = StartCoordinator->StartM01Run(Selection, Feedback);
	if (!bStarted)
	{
		ShowSect(Feedback);
		return;
	}
	if (SectWidget && StartCoordinator->GetState() == Edemo_map0909BTopState::InRun)
	{
		SectWidget->RemoveFromParent();
	}
	RefreshSect(Feedback);
}

void Ademo_map0909BFrameworkHost::RequestOpenWarehouseFromUI()
{
	if (!bInitialized || !StartCoordinator.IsValid())
	{
		RefreshSect(TEXT("仓库路由尚未初始化。"));
		return;
	}
	if (!StartCoordinator->IsAtSect())
	{
		const Edemo_map0909BTopState CurrentState = StartCoordinator->GetState();
		RefreshSect(
			CurrentState == Edemo_map0909BTopState::PreparingStart
				|| CurrentState == Edemo_map0909BTopState::ActivatingWorld
				? TEXT("出战尝试处理中；尚未形成活动 Run，P5 仓库暂不接受写入。")
				: TEXT("当前是真实局内状态；P5 仓库只读，不能整理。"));
		return;
	}
	FString Feedback;
	if (!GameMode.IsValid())
	{
		ShowSect(TEXT("仓库／人物装备暂时不可用。"));
		return;
	}
	if (!GameMode->Open0909BOutOfRaidInventory(Feedback))
	{
		ShowSect(Feedback);
		return;
	}
	if (SectWidget)
	{
		SectWidget->RemoveFromParent();
	}
}

void Ademo_map0909BFrameworkHost::RequestWarehouseDragDrop(
	const FGuid& ItemId,
	const FGuid& SourceContainerId,
	const int32 SourceSlot,
	const FGuid& TargetContainerId,
	const int32 TargetSlot,
	const int32 ExpectedGraphRevision)
{
	// Compatibility-only endpoint for a stale constructed widget. The P23
	// product route never mounts that widget, and this endpoint is deliberately
	// zero-write so it cannot become a second P5 transaction path.
	(void)ItemId;
	(void)SourceContainerId;
	(void)SourceSlot;
	(void)TargetContainerId;
	(void)TargetSlot;
	(void)ExpectedGraphRevision;
	RequestCloseWarehouseFromUI();
	ShowSect(TEXT("旧仓库拖拽入口已停用；请从宗门主页重新打开统一物品工作台。"));
}

void Ademo_map0909BFrameworkHost::RequestCloseWarehouseFromUI()
{
	ShowSect(TEXT("已返回宗门；仓库仍是同一份 P5 图，当前战备摘要保持只读可审计。"));
}

Edemo_map0909BTopState Ademo_map0909BFrameworkHost::GetTopState() const
{
	return StartCoordinator.IsValid()
		? StartCoordinator->GetState() : Edemo_map0909BTopState::TechnicalStartFailure;
}

const Fdemo_map0909BStartDiagnostic&
Ademo_map0909BFrameworkHost::GetLastStartDiagnostic() const
{
	static const Fdemo_map0909BStartDiagnostic Empty;
	return StartCoordinator.IsValid()
		? StartCoordinator->GetLastDiagnostic() : Empty;
}

void Ademo_map0909BFrameworkHost::ShowSect(const FString& InFeedback)
{
	if (!Controller.IsValid())
	{
		return;
	}
	if (!SectWidget)
	{
		SectWidget = CreateWidget<Udemo_map0909BSectWidget>(
			Controller.Get(), Udemo_map0909BSectWidget::StaticClass());
		if (SectWidget)
		{
			SectWidget->InitializeForFramework(this);
		}
	}
	if (!SectWidget)
	{
		return;
	}
	if (!SectWidget->IsInViewport())
	{
		SectWidget->AddToViewport(700);
	}
	Controller->BeginProfilePreparationInputLock(SectWidget);
	RefreshSect(InFeedback);
}

void Ademo_map0909BFrameworkHost::RefreshSect(const FString& InFeedback)
{
	if (SectWidget && StartCoordinator.IsValid())
	{
		SectWidget->RefreshPresentation(
			StartCoordinator->GetState(), InFeedback,
			StartCoordinator->GetLastDiagnostic());
	}
}

bool Ademo_map0909BFrameworkHost::OpenWarehouseService(FString& OutDiagnostic)
{
	OutDiagnostic.Reset();
	if (!GameMode.IsValid() || !StartCoordinator.IsValid() || !WarehouseService)
	{
		OutDiagnostic = TEXT("宗门仓库服务尚未初始化。");
		return false;
	}
	Fdemo_mapProfileSessionSnapshot Snapshot;
	if (!GameMode->Get0909BProfileSnapshot(Snapshot, OutDiagnostic))
	{
		return false;
	}
	Fdemo_map0909BWarehousePresentation Presentation;
	if (!WarehouseService->OpenForSect(GameMode->Get0909BProfileStorageRoot(), Snapshot,
		StartCoordinator->GetState(), Presentation, OutDiagnostic))
	{
		UE_LOG(Logdemo_map, Warning,
			TEXT("0_0_9BFIX_WAREHOUSE Event=OpenForSect Ready=0 CoordinatorState=%d SessionState=%d ActiveRunId=%s LastTerminalReason=%d Diagnostic=%s"),
			static_cast<int32>(StartCoordinator->GetState()), static_cast<int32>(Snapshot.SessionState),
			*Snapshot.ActiveRunId.ToString(EGuidFormats::DigitsWithHyphensLower),
			static_cast<int32>(Snapshot.LastTerminalReason), *OutDiagnostic);
		return false;
	}
	const bool bProjectionValid = Fdemo_map0909BEditorSupport::ValidateWarehouseProjection(Presentation, OutDiagnostic);
	UE_LOG(Logdemo_map, Log,
		TEXT("0_0_9BFIX_WAREHOUSE Event=OpenForSect Ready=%d CoordinatorState=%d SessionState=%d ActiveRunId=%s LastTerminalReason=%d Diagnostic=%s"),
		bProjectionValid ? 1 : 0, static_cast<int32>(StartCoordinator->GetState()),
		static_cast<int32>(Snapshot.SessionState), *Snapshot.ActiveRunId.ToString(EGuidFormats::DigitsWithHyphensLower),
		static_cast<int32>(Snapshot.LastTerminalReason), *OutDiagnostic);
	return bProjectionValid;
}

bool Ademo_map0909BFrameworkHost::EnsureShanmenItemCutover(
	FString& OutDiagnostic)
{
	OutDiagnostic.Reset();
	if (!GameMode.IsValid() || !WarehouseService)
	{
		OutDiagnostic = TEXT("ShanmenItems cutover requires the product host and warehouse adapter.");
		return false;
	}
	UGameInstance* GameInstance = GameMode->GetGameInstance();
	Udemo_mapProfileSessionSubsystem* ProfileSession = GameInstance
		? GameInstance->GetSubsystem<Udemo_mapProfileSessionSubsystem>()
		: nullptr;
	Udemo_mapShanmenItemAuthoritySubsystem* Authority = GameInstance
		? GameInstance->GetSubsystem<Udemo_mapShanmenItemAuthoritySubsystem>()
		: nullptr;
	Fdemo_mapProfileSessionSnapshot Snapshot;
	if (!ProfileSession || !Authority
		|| !GameMode->Get0909BProfileSnapshot(Snapshot, OutDiagnostic))
	{
		if (OutDiagnostic.IsEmpty())
		{
			OutDiagnostic = TEXT("ShanmenItems cutover dependencies are unavailable.");
		}
		return false;
	}
	const Fdemo_mapShanmenItemCutoverResult Result =
		Fdemo_mapShanmenItemCutoverCoordinator::Execute(
			Fdemo_mapProfileStorageContext::ForRoot(
				GameMode->Get0909BProfileStorageRoot()),
			Snapshot.ProfileId, *Authority, *ProfileSession,
			*WarehouseService);
	OutDiagnostic = Result.Diagnostic;
	if (Result.IsReady())
	{
		UE_LOG(Logdemo_map, Log,
			TEXT("SHANMEN_P1_13_PRODUCT_CUTOVER Ready=1 Status=%d OwnerId=%s Diagnostic=%s"),
			static_cast<int32>(Result.Status),
			*Snapshot.ProfileId.ToString(EGuidFormats::DigitsWithHyphens),
			*Result.Diagnostic);
	}
	else
	{
		UE_LOG(Logdemo_map, Warning,
			TEXT("SHANMEN_P1_13_PRODUCT_CUTOVER Ready=0 Status=%d OwnerId=%s Diagnostic=%s"),
			static_cast<int32>(Result.Status),
			*Snapshot.ProfileId.ToString(EGuidFormats::DigitsWithHyphens),
			*Result.Diagnostic);
	}
	return Result.IsReady();
}

void Ademo_map0909BFrameworkHost::EndPlay(
	const EEndPlayReason::Type EndPlayReason)
{
	if (GameMode.IsValid())
	{
		GameMode->Set0909BOutOfRaidClosedCallback(TFunction<void()>());
	}
	WarehouseService.Reset();
	Super::EndPlay(EndPlayReason);
}
