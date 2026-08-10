# Dev.D.UE.0.0.9B.P46.0.r0 Report

## 结论

READY_FOR_P47_PLANNING

P46 已静态闭合：当前已打开、已揭示且 identity-valid 的 P10 BasicCache `SpiritDust`／`IronShard` simple stack 已接入既有 Ctrl + 左键共享路由；输入时只冻结 exact current P17 child 或 no-child BaseQuick，目标容器内按 stable SlotIndex 执行 merge-first / empty-second，一次手势只提交一个 P1 `Merge(Quantity=0)` 或 whole-root `Move`。P9/P6 继续在同一个 Owner record 中一次替换并只调用一次 `SaveRecord`。Editor 与 Game 均以 native exit code 0 编译完成。未启动产品，未执行 F 阶段真实验证。

实现提交：`8f3f7d7 feat: add P46 BasicCache quick transfer`。

## 文件范围

修改：

- `Source/demo_map/CodeB/demo_mapCodeBP2.h`：新增只存在于一次命令生命周期内的 P46 source/receipt/P6 revision/frozen-target proof，并沿既有 P2 command 携带。
- `Source/demo_map/CodeB/demo_mapCodeBP4.h`：在既有 drag payload 中携带 P46 transient proof；无新输入对象或可写物品状态。
- `Source/demo_map/CodeB/demo_mapCodeBP4.cpp`：沿既有 `CommitPreview` 将 P46 proof 交给 P3/P2。
- `Source/demo_map/CodeB/demo_mapCodeBP3.h`、`Source/demo_map/CodeB/demo_mapCodeBP3.cpp`：签名兼容地把 P46 proof 写入唯一 P2 command；`BeforeSnapshot`、P1 调用与 persistence-failure rollback 路径保持不变。
- `Source/demo_map/CodeB/demo_mapCodeBP3UI.h`：声明 P46 proof 构造、Preview/Commit gate、P10 projection refresh 与 normal-target open generation。
- `Source/demo_map/CodeB/demo_mapCodeBP3UI.cpp`：只扩展既有 shared Ctrl router；新增 exact BasicCache source gate、input-time target freeze、稳定候选重验、page/focus invalidation 与 accepted-only P10 projection refresh。
- `Source/demo_map/CodeB/demo_mapCodeBOutOfRaidProfile.h`：P10 sole durable callback 增加 accepted P2 command 参数。
- `Source/demo_map/CodeB/demo_mapCodeBOutOfRaidProfile.cpp`：在既有 P9/P6 atomic writer 内从 durable truth 重建 P46 source、frozen target、候选和同一 P1 delta，再使用原有单次 `SaveRecord`。
- `Source/demo_map/demo_mapV3ProgressionManager.cpp`：production P10 page只读转发 P6 revision/refresh；durable callback 在写入前重验 active Host、page、Owner/Run 与 frozen mode，然后调用既有 Store writer。
- `Docs/Report/Dev.D.UE.0.0.9B.P46.0.r0_report.md`：本报告。

明确未修改：

- 任何测试文件、P1 transaction core、P9 materialization/history/determinism、P10 open/search/reveal state machine、P17 graph construction、P31 schema/Registry、P8 terminal、P13 binding、P15 use、P5/P6 bridge、Code A inventory/Loot authority、Actor 类或地图内容。
- 未新增 Button、hotkey、pointer handler、Widget、Actor、second resolver、quantity draft、GroundDrop、WorldDrop/ordinal、ItemId/ContainerId allocator、第二 Repository writer、第二 save 或 Code A 写入口。

## 调用图与权威边界

1. 既有 `UCodeBP3CellButton::NativeOnMouseButtonDown` 仍是 Ctrl + 左键唯一消费点；命中后调用既有 `UCodeBP3InventoryWidget::HandleQuickTransfer` 并返回 Handled，不继续 ordinary selection、P10 search、drag threshold、Use、right-click 或 Actor interaction。
2. `BeginP4Drag` 与 `PopulateTransferContext` 从当前 P2 projection、P10 normal projection、workspace 和 P9 receipt 形成只读 source address；`PopulateP46NormalContainerSimpleStackQuickTransferProof` 只接受 exact `NormalContainerTarget` 的 Revealed BasicCache ordinary root。
3. shared resolver 在 pointer-down 一次性选择 `CurrentP17Child` 或 `BaseQuickNoChildAtInput`。前者冻结 parent ItemId、child ContainerId/open generation；后者明确清空全部 child identity。之后不得重猜、重扫另一容器或降级回 BaseQuick。
4. 同一个 `HandleQuickTransfer` 在冻结容器内按 stable SlotIndex 先寻找 compatible underfull stack；仅在不存在可接受 Merge candidate 时再寻找首个空 ordinary Cell。找到一个 candidate 后只走 `PreviewInventoryTransfer` → `CommitInventoryTransfer` → 既有 P4/P3/P2/P1。
5. P3 在一个 `BeforeSnapshot` 上调用一次 P2/P1 command。Store callback接收 accepted command，在局部 candidate 中从 P9/P6 durable truth重建相同命令并要求 replay snapshot 全字段相等；失败时 P3 恢复 `BeforeSnapshot`。
6. `CommitAcceptedMatchedRunNormalContainerTransfer` 使用既有 partition、P13 reconcile、payload receipt freeze、P9/P6 single Owner-document replacement 与函数内唯一一次 `SaveRecord`。成功后才刷新 P10 source projection与 P6 controller projection。

不存在 Widget direct Move/quantity mutation、Actor direct pickup、display-cache write、right-click Take、double-click、selection/first/last target猜测、预建 identity、P9 staging、第二 durable transaction/save 或 Code A inventory writer。

## P9/P10 source 与生命周期证明

- source 必须位于 current exact P9 record 的 `NormalContainerTarget` ordinary stable address，并同时匹配 OwnerId、RunInstanceId、SearchTargetId、DefinitionId=`CodeB.NormalContainer.BasicCache`、receipt id、definition revision/digest、loot profile/version/digests、ContainerId、SlotIndex 与 ItemId。
- projection与 Store均要求 P10 State=`Open`、无 active open/search action、item RevealState=`Revealed`、Quantity>0、canonical Definition 为 `SpiritDust` 或 `IronShard`、stackable、MaxStack>1、无 child、非 equipment/spatial graph。
- page open generation、target revision、composite revision、P6 snapshot revision、workspace target pane、Host/route/focus和 active Owner/Run 在 Preview 与 durable callback 前重验。Close/focus loss 会推进 generation 并清空 active child；旧 intent 立即失效。
- P9 materialization receipt、content plan、loot roll/history、open/search/reveal状态机与 record schema均未改写；P46 只读取其当前 durable truth并在 accepted P1 delta后沿既有 P10 writer更新 source snapshot/reveal列表。

Hidden/Searching、space parent/child、equipment、corpse、WorldDrop、warehouse、player source、Hotbar、another NormalContainer、wrong target/profile/receipt/generation 或 unknown provenance均不能形成 P46 proof，保持零写入。

## Frozen target、数量与失败策略

CurrentP17Child：

- 输入时 current child 必须由 workspace open generation和 P6 canonical parent → child 拓扑唯一证明；parent须位于正式 SpatialRing/Backpack placement，目标只能是 exact child ordinary cells。
- Preview、P3 callback与 Store再次比对 child ContainerId、parent ItemId、open generation、Owner/Run、P6 revision、capacity/slot identity。close、switch、focus loss、parent/topology/capacity stale或无 candidate均拒绝，不回退 BaseQuick。

BaseQuickNoChildAtInput：

- 只有输入时没有 identity-valid current child才建立该模式；proof明确不携带 child ContainerId、parent ItemId或 generation。
- 输入后即使出现 child也不改变目标；唯一目标仍为 active P6 BaseQuick ordinary cells。满容器、revision/session stale或无 candidate均零写入。

数量与候选：

- occupied candidate只允许一次 P1 `Merge(Quantity=0)`；P1独立裁决完整或部分接受量。部分接受后 source保持同一 ItemId、P9 ContainerId/SlotIndex、RevealState和剩余 Quantity，手势不继续空格扫描。
- empty candidate只允许一次 P1 whole-root `Move`，保留 ItemId、DefinitionId、Quantity、Level、Quality、RandomSeed与 LegacyAffixDigest；不创建任何新 identity、child、record、WorldDrop、ordinal或 Actor。
- target仅接受 BaseQuick `Basic6` 或 exact current child `QuickSpatial`/`PouchInternal` ordinary storage；不接受 Weapon、Armor、Accessory、SpatialRing、Backpack formal equipment slot，不执行 Equip、Unequip、Split、Swap、Replacement或 Quantity=N。

任一 source/mode/target/candidate/session/P1/replay/reconcile/save失败均发生在局部 candidate与现有 `BeforeSnapshot` rollback边界内；P9 source、P6 target、selection、scroll、无关 target/body/world record保持旧值，无 phantom empty、duplicate quantity或第二真值。

## 非回归结论

- P10 normal Drag 的 Move/Merge/Swap未进入 P46 proof，继续使用原有显式目标语义。
- P40 corpse simple-stack、P29 WorldDrop simple stack、P30/P41 complete graph、P36/P37/P39 standard equipment各自仍要求原有 source proof；P46 proof与这些 proof互斥。
- P43—P45 GroundDrop、P26—P28 world数量行为、P31 Registry、P17 graph、P8 terminal/recovery、P13 binding、P15 use、P9/P10 deterministic materialization/reveal、P5/P6与 Code A authority均未获得新写入口。
- production manager只转发P10 page lifecycle、projection refresh与 durable callback gate，不持有 Item/Container/Quantity或第二库存。

## 静态检查与编译

- `git diff --check`：通过。
- 实际源代码改动：10 个文件；测试文件改动：0。
- 未启动产品、PIE、Standalone、真实鼠标键盘、截图、Smoke、Automation、回归、Cook或 Package。

Editor：

```text
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_mapEditor Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload
native exit code: 0
Result: Succeeded
Total execution time: 25.23 seconds
```

Game：

```text
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_map Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload
native exit code: 0
Result: Succeeded
Total execution time: 29.49 seconds
```

## 留给 0.0.9B.F 的真实验证

未执行：BasicCache materialize/open/reveal；SpiritDust与 IronShard各自 Ctrl到 valid current child及 no-child BaseQuick；compatible partially-full/full target、empty target、partial Merge、full BaseQuick/child；child close/switch/focus/generation/parent/capacity stale；Hidden/Searching/wrong target/wrong source；SearchTargetId/receipt/Owner/Run/P9/P6 revision stale；Target close/reopen、P10 action/search、Prepared/terminal/Host invalid/SaveRecord failure；P10 normal Move/Merge/Swap；P40/P29/P30/P36/P37/P39/P41、P43—P45、P26—P31、P8 terminal/recovery；真实鼠标键盘、PIE、Standalone、截图、Smoke、Automation、回归、Cook与 Package。
