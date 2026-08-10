# Dev.D.UE.0.0.9B.P48.0.r0 Report

## 结论

P48 已在现有 P10 production Host 上静态闭合：已打开、已揭示且 identity-valid 的 BasicCache r2 `WindTalisman`／`BackpackLevel1` 完整空间图，可沿共享 normal Drag/Drop 路径直接移动到用户实际指定的唯一兼容空正式装备位。实现不扫描目标、不 fallback、不自动装备，不改变 P18 BaseQuick normal Drag 或 P47 BaseQuick-only Ctrl 语义。

实现提交：`b8744b0 feat: add P48 BasicCache explicit equipment drag`。

## 文件清单与职责

新增：

- `Docs/Report/Dev.D.UE.0.0.9B.P48.0.r0_report.md`：本报告。

修改：

- `Source/demo_map/CodeB/demo_mapCodeBP2.h`：新增 P48 transient source/target proof，并随唯一 P2 command 传递。
- `Source/demo_map/CodeB/demo_mapCodeBP4.h`：在共享 Drag payload 中承载 P48 proof；未新增 DragOperation。
- `Source/demo_map/CodeB/demo_mapCodeBP3.h`、`demo_mapCodeBP3.cpp`：沿现有 `CommitP4Operation` 把 P48 proof 复制到唯一 P2 command；保留 accepted-P1 持久化和 BeforeSnapshot 回滚入口。
- `Source/demo_map/CodeB/demo_mapCodeBP3UI.h`、`demo_mapCodeBP3UI.cpp`：从 P10 ordinary root Cell 建立 source proof；在现有 DragEnter/NativeOnDrop 冻结用户实际 target；Preview/Commit 重验 source、closure、Host/open/reveal、Owner/Run/revision/session 和 exact formal target。
- `Source/demo_map/CodeB/demo_mapCodeBP4.cpp`：仅在 P48 intent 已证明时把兼容空装备位 Preview 固定为 `Move(1)`；occupied/wrong target 继续拒绝。
- `Source/demo_map/CodeB/demo_mapCodeBOutOfRaidProfile.cpp`：从 P9/P6 durable truth 重建 canonical source/target，重放一次 P1 whole-graph `Move(1)`，校验 accepted composite，并沿现有单次 Owner record replacement/`SaveRecord` 提交。
- `Source/demo_map/demo_mapV3ProgressionManager.cpp`：在既有 P10 durable callback 写入前重验 Host、page、active NormalContainer、Workspace Owner/Run 和冻结 target 身份。

明确未修改：

- `demo_mapCodeBInventory.h/.cpp` 的 P1 graph/transaction 实现；P48 直接复用既有 whole-root `Move`。
- P18 BasicCache r2 profile、gate、weight、candidate、receipt/materialization/history 与 stable child 构造。
- P7/P17 graph creator、P31 Registry、P8 terminal/recovery、P13 binding、P15 use、Code A 功能逻辑与全部测试文件。
- 未新增 Widget、Button、hotkey、pointer handler、Actor、Repository、resolver、ItemId、ContainerId、child、record、ordinal 或保存入口。

## 活动调用链审计

唯一生产调用图：

`P10 ordinary root Cell::NativeOnDragDetected`
→ `UCodeBP3InventoryWidget::BeginP4Drag`
→ `UCodeBP3UIHostSubsystem::BeginInventoryDrag`
→ shared `FCodeBP4InteractionController::BeginDrag`
→ `PopulateTransferContext`
→ `PopulateP48NormalContainerSpatialGraphEquipmentSourceProof`
→ existing `UCodeBP4DragOperation`
→ existing Cell `DragEnter` / `NativeOnDrop`
→ `FreezeP48NormalContainerSpatialGraphEquipmentTarget`
→ `PreviewInventoryTransfer`
→ shared P4 `PreviewDrop`
→ `ValidateP48NormalContainerSpatialGraphEquipmentTransferContext`
→ `CommitInventoryTransfer`
→ shared P4 `CommitPreview`
→ shared P3 `CommitP4Operation`
→ single P2 Apply / P1 transaction
→ `PersistProfileSnapshotAfterAcceptedP1`
→ existing P10 manager durable callback
→ `CommitAcceptedMatchedRunNormalContainerTransfer`
→ one Owner document replacement / one `SaveRecord`.

没有增加第二个 `NativeOnDrop`、P48 pointer consumer、Widget/Actor direct move、second resolver、second repository transaction、second save 或 Code A writer。Ctrl handler 在进入 P46/P47 resolver 前显式清空 P48 normal-drag proof，因此 P47 仍只处理 Ctrl + 左键并保持 `BaseQuickOnly`。

## P18 source provenance 与 graph closure

Source proof 只从当前 P10 `NormalContainerPresentation` 与 P1 composite projection 建立，并冻结：

- OwnerId、RunInstanceId、SearchTargetId、ReceiptId、BasicCache DefinitionId；
- P9 record revision、target-open generation、P6 snapshot/composite revision、Workspace target pane；
- actual stable ordinary address：运行时 exact `SourceContainerId + SourceSlot + SourceItemId`，不使用固定 Cell index、selection 或数组首尾猜测；
- Definition content revision/digest；BasicCache r2 ProfileId/version/digest、`CodeB.DeterministicWeightedLoot.Crc32.r2` algorithm、result/materialization digest；
- parent DefinitionId、stable ItemId、唯一 ChildContainerId/SpatialChildGuid。

资格仅接受 `CodeB.NormalContainer.BasicCache`、`Open`、无 active action/search、`Revealed` ordinary root、BasicCache r2 receipt，以及 canonical `WindTalisman` 或 `BackpackLevel1`。UI gate 逐项校验 child 容量、全空、parent reverse pointer 和唯一 child owner；durable Store 再从 canonical Catalog、P9 snapshot 和 receipt 重建并调用既有 complete-closure validator，拒绝 partial、clone、flatten、nested、orphan、cycle、duplicate owner 或 stale topology。

P48 未修改任何 P18 materialization/profile 代码，所以 r1/r2 历史、optional gate、权重、候选、ItemId、ChildContainerId、receipt 与已物质化 record 均不会 reroll、补料或迁移。

## Exact target freeze 与零写入策略

Preview 只检查用户本次真实 Drop 的 `ContainerId + SlotIndex`，并冻结 target semantic 与同一 composite revision：

- `WindTalisman` 只接受唯一 canonical `SpatialRing`，容量 1、slot 0、空，Definition equip slot 为 `SpatialItem`。
- `BackpackLevel1` 只接受唯一 canonical `Backpack`，容量 1、slot 0、空，Definition equip slot 为 `Backpack`。

Commit 不重扫、不按 role 顺序选“第一个兼容槽”，也不改投 BaseQuick、另一装备位、P17 child、Hotbar、WorldDrop 或任何其他位置。wrong/full/occupied/stale target、source/page close/reopen、search/action、Host invalid、Owner/Run/revision/receipt/profile mismatch、Prepared/terminal 或 durable save failure 都在既有 P3 BeforeSnapshot/accepted-callback 边界零写入失败。

P18 原 `Basic6` Drop 在 freeze 时明确绕过 P48 intent，继续沿原 normal `Move` 路径；所有其他非正式目标在 P48 candidate 下无 frozen target，Preview/Commit 立即拒绝。P47 proof 与 P48 proof互斥，Ctrl path 不会扫描 SpatialRing/Backpack。

## 单一事务、持久化与非回归

P48 accepted command 固定为 `Standard + Legacy + Move + Quantity=1`。Store 从 durable P9/P6 prior composite 重建同一 parent/唯一 empty child closure、exact source reverse pointer 和 exact empty formal equipment target，并使用 accepted `TransactionId` 重放一次 P1 `Move(1)`；重放 snapshot 必须与 accepted composite 全等。

合法变化只有 root placement 从 exact P9 ordinary source 变为 frozen P6 formal equipment slot；parent ItemId、DefinitionId、ChildContainerId、SpatialChildGuid、child capacity/slots 与全部现有 ItemId/ContainerId 不变。随后既有 partition 把完整 child graph随 parent 归入 P6，P9 source 与 P6 target 在同一 Owner record 中共同替换，P13 仅做既有 reconcile，最后只调用一次 `SaveRecord`。未创建 WorldDrop/record/ordinal/Actor，也没有先落 BaseQuick 再二次移动。

静态非回归结论：

- P18 BaseQuick normal Drag 仍走原路径，P48 不收窄或重定向。
- P47 仍为 Ctrl + 左键、BaseQuick-only、first-empty；不自动装备。
- P46 simple stack、P42 corpse spatial equipment、P38/P39/P40、P29/P30/P34/P36/P37、P41、P31、P5/P6/P8/P13/P15 和 Code A authority 未改变。
- P17 child、Hotbar、another NormalContainer、corpse、WorldDrop、warehouse、player source、Hidden/Searching、simple stack、child item和未知 complex parent 均未接入 P48。

## P 阶段静态检查与编译

- `git diff --check`：native exit code `0`，无 whitespace error。
- Editor：
  - 命令：`"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_mapEditor Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload`
  - native exit code：`0`
  - 关键结果：`Result: Succeeded`；输出 `UnrealEditor-demo_map.dll`。
- Game：
  - 命令：`"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_map Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload`
  - native exit code：`0`
  - 关键结果：`Result: Succeeded`；输出 `Binaries/Win64/demo_map.exe`。

未启动产品、PIE、Standalone、真实输入、截图、Smoke、Automation、回归、Cook 或 Package。

## 留待 0.0.9B.F 的真实验证

- BasicCache r2 首次 materialize 的 optional spatial hit/no-hit、open/reveal，以及已物质化历史保持。
- canonical WindTalisman normal Drag → empty SpatialRing；BackpackLevel1 normal Drag → empty Backpack；原 normal Drag → empty BaseQuick。
- full/wrong/occupied formal slot；wrong parent/child/simple/equipment/Hidden/Searching/unknown source；P17 child、Hotbar 与其他 player slot 拒绝。
- NormalContainerTarget/SearchTargetId/receipt/profile/Owner/Run/P9/P6 revision stale；target close/reopen、P10 action/search、Host invalid、Prepared/terminal、SaveRecord failure 与 recovery。
- P47 QuickTransfer、P10 normal Move/Merge/Swap、P18 normal graph Drag、P46、P42、P38/P39/P40、P29/P30/P34/P36/P37、P41、P31 other-record isolation、P8 terminal/recovery。
- 真实鼠标键盘、PIE、Standalone、截图、Smoke、Automation、回归、Cook 与 Package。

READY_FOR_P49_PLANNING
