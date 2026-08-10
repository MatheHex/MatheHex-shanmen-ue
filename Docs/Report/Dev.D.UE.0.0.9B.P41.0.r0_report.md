# Dev.D.UE.0.0.9B.P41.0.r0 Report

## 1. 结果

P41 已完成：当前已打开、已揭示、identity-valid 的 P12 BasicCorpse ordinary root 中，P20 r2／P21 r3 继承的 canonical WindTalisman 或 BackpackLevel1 空 child 完整图可经唯一共享 Ctrl + 左键入口，移动到输入时冻结的 active P6 BaseQuick 内 stable SlotIndex 最小的合法空格。Preview 只建立一个 candidate；Commit 只重验该 exact ContainerId／SlotIndex／revision，不重扫、不回退。

实现提交：`10b352b feat: add P41 corpse spatial graph quick transfer`。

## 2. 文件与职责

新增／修改：

- `Source/demo_map/CodeB/demo_mapCodeBP2.h`：新增 `BaseQuickOnly` target mode、P41 source/closure/domain/candidate 瞬时证明，并接入唯一 P2 command。
- `Source/demo_map/CodeB/demo_mapCodeBP4.h`：在既有 drag payload 中携带 P41 瞬时证明；不保存第二物品状态。
- `Source/demo_map/CodeB/demo_mapCodeBP3.h`、`demo_mapCodeBP3.cpp`：沿既有 P3 → P2 command 签名原样转发 P41 证明。
- `Source/demo_map/CodeB/demo_mapCodeBP4.cpp`：P41 exact spatial root 到空目标只预览 whole-graph `Move(1)`；未使用 Quantity=0、Merge、Split 或 Swap。
- `Source/demo_map/CodeB/demo_mapCodeBP3UI.h`、`demo_mapCodeBP3UI.cpp`：复用唯一 Cell Ctrl pointer router；建立 exact source proof，冻结 BaseQuickOnly domain，解析一次 first-empty candidate，并在 Preview／Commit 重验 exact target。
- `Source/demo_map/CodeB/demo_mapCodeBOutOfRaidProfile.cpp`：在既有 P11/P6 durable callback 内独立重建 P20/P21 provenance、P17 空 child closure、首空 BaseQuick target，并以同一 P1 whole-graph Move replay 验证 candidate。

未修改：P1 Repository／transaction 实现、P11/P12 state machine、P20/P21 materialization 与 deterministic profile/history、P17 graph construction、P30 WorldDrop Registry/record/Actor、P31 schema、P8 terminal/recovery、P13/P15、Code A、所有测试文件、PROJECT 文档。

## 3. 唯一调用图与输入边界

`UCodeBP3CellButton::NativeOnMouseButtonDown`（既有 Ctrl + Left 唯一消费点） → `UCodeBP3InventoryWidget::HandleQuickTransfer` → `BeginP4Drag`／既有 payload → `PopulateP41BodySpatialGraphQuickTransferProof` → BaseQuickOnly first-empty scan → `PreviewInventoryTransfer` → `FCodeBP4InteractionController::PreviewDrop` → `CommitInventoryTransfer` → `CommitDrop` → `FCodeBP3UIController::CommitP4Operation` → P2/P1 → `CommitAcceptedMatchedRunBodyContainerTransfer` → one P11/P6 durable replacement／one SaveRecord。

没有新增 Button、hotkey、pointer handler、Widget、Actor click、second resolver、Widget direct Move 或 Code A 写入口。ordinary left、right-click、double-click、normal Drag、player-side Ctrl、Shift+1—9、Actor interaction 均未获得 P41 语义。

## 4. Canonical source 与完整图证明

- source 必须是 current `BodyContainerTarget` ordinary root 的 exact ContainerId／SlotIndex／ItemId，BodyTarget 为 Open，source 为 Revealed，且无 active action/search。
- UI proof 冻结 OwnerId、RunInstanceId、BodyTargetId、DeathReceiptId、body revision、target-open generation、source address、parent DefinitionId／ItemId、ChildContainerId／stable SpatialChildGuid、profile/digest、workspace pane、P6 composite revision 与 BaseQuick ContainerId。
- 只接受 `CodeB.BodyContainer.BasicCorpse` 的 `CodeB.LootProfile.BasicCorpse.r2` v2 或 `r3` v3；r3 只继承 P20 Optional.SpatialUtility，不改写 profile、candidate、weight、digest、receipt 或 materialization history。
- durable Store 从原始 P11 snapshot、Catalog、receipt/profile 与 P1 graph 重建 source；只接受 WindTalisman／BackpackLevel1，验证 canonical definition、唯一 child owner、ChildContainerId=`SpatialChildGuid(parent ItemId)`、正式 child type/capacity、空 child、一层无嵌套、无环、无 orphan、无 duplicate owner 与 reverse slot pointer。
- P21 fixed equipment、simple stack、child Cell、Hidden/Searching、P19 WorldDrop、P5/P9/P14/P31、player source、another BodyTarget 与 unknown provenance 均不能建立 P41 proof。

## 5. BaseQuickOnly 冻结与 zero-write policy

- input 时冻结 `Projection.BasicContainerId`；任何当前或随后打开的 P17 child 均被清除出 P41 target proof，空间 parent 永不嵌入 child。
- resolver 只遍历 exact `Basic6`，按真实 stable SlotIndex 升序取第一个正式空格；不扫描装备位、Hotbar、child、P5/P9/P11/P14/P31 或第二容器。
- Preview 前写入唯一 candidate 的 exact target ContainerId、SlotIndex 与 composite revision；Preview／Commit／durable Store 必须一致。
- candidate stale、被占用、capacity/revision/session/open/focus/receipt/source/closure 变化、Prepared/terminal/Host invalid 或 SaveRecord failure 时零写入；不重扫第二空格、不 fallback、不 auto equip。

## 6. 事务、持久化与回滚

- accepted path 只有一个 P1/P20 whole-graph `Move(1)`；parent ItemId、ChildContainerId、所有 closure identity、capacity 与 SpatialChildGuid 保持不变，唯一变化是 root 从 exact P11 source 到冻结的 P6 BaseQuick target。
- durable Store 使用 accepted command 的同一 TransactionId／source／target／revision 重放一次 Move，并要求 replay snapshot 与 P2 accepted composite 完全相等。
- P11 source 与 P6 target 在现有同一 Owner durable replacement 中分区并共同提交，只有一次 SaveRecord；source visibility 仅在 accepted snapshot proof 后移除，随后只刷新 exact BodyTarget 与 P6 projection。
- 保存或任一 durable gate 失败时，既有 `BeforeSnapshot` 回滚恢复 P1 composite；不产生 phantom empty、phantom pickup、clone、new ItemId、new ContainerId、new child 或第二真值。
- P13 仅沿既有 accepted reconcile；P8 terminal/recovery 不变。P41 不创建、删除或读取 WorldDrop record、ordinal、Registry、Actor 或地图落点。

## 7. 非回归静态结论

- P12 normal Drag 的 Move/Merge/Swap 与 P20 explicit whole-graph Drag 保持原路径；P41 proof 仅在 Ctrl gesture 后建立。
- P40 ordinary simple-stack 仍为 frozen child-priority／no-child BaseQuick，merge-first／empty-second 与 Quantity=0 语义未改。
- P38/P39 P21 equipment、P29/P30/P34/P36/P37 WorldDrop Ctrl branches 未改；P30 仍独占 WorldDrop complete-graph cleanup／Actor projection。
- P17 construction、一层限制、P20/P21 deterministic source/history、P31 Registry、P5/P6 bridge、P8、P13、P15 与 Code A authority 均未修改。
- 静态检查通过：改动仅限 8 个允许的 Code B 文件；`git diff --check` 通过；未改测试或 Code A 文件；P41 accepted command 明确为 Move(1)，没有 P41 Quantity=0／WorldDrop／ordinal／Actor 写入。

## 8. 编译

Editor：

`"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_mapEditor Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload`

- native exit code：0
- Result：Succeeded
- Total execution time：30.32 s

Game：

`"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_map Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload`

- native exit code：0
- Result：Succeeded
- Total execution time：29.76 s

未启动产品、PIE 或 Standalone。

## 9. 留给 0.0.9B.F 的真实验证

未执行：P20/P21 future corpse materialize/reveal；真实 WindTalisman／BackpackLevel1 Ctrl 拾回至 first legal BaseQuick；空/满 BaseQuick 与 stable SlotIndex；parent/child identity；child open/switch/close 不成为目标；wrong/simple/equipment/child/Hidden/Searching source；BodyTarget/death receipt/profile/Owner/Run/P6 revision stale；Prepared/terminal/Host invalid/SaveRecord failure；P12 normal Move/Merge/Swap 与 P20 normal graph Drag；P21/P38/P39、P40、P29/P30/P34/P36/P37；P31 other-record isolation；P8 terminal/recovery；真实鼠标键盘、PIE、Standalone、截图、Smoke、Automation、回归、Cook、Package。

READY_FOR_P42_PLANNING
