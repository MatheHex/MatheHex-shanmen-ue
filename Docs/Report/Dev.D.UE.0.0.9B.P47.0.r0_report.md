# Dev.D.UE.0.0.9B.P47.0.r0 Report

## 结论

READY_FOR_P48_PLANNING

P47 已静态闭合：current exact opened/revealed P10 BasicCache r2 ordinary root 中的 canonical `WindTalisman`／`BackpackLevel1` 完整 parent-child graph 已接入既有 Ctrl + 左键共享 QuickTransfer 路由。输入时只冻结 active P6 BaseQuick，在真实 stable SlotIndex 升序中选择第一个正式空 ordinary cell；一次手势只提交一个 P1 whole-graph `Move(1)`，P9/P6 继续在同一个 Owner record 内一次替换并只调用既有一次 `SaveRecord`。Editor 与 Game 最终均以 native exit code 0 编译完成；未启动产品或执行 F 阶段测试。

实现提交：`219a42a feat: add P47 BasicCache spatial graph quick transfer`。

## 文件范围

修改：

- `Source/demo_map/CodeB/demo_mapCodeBP2.h`：新增命令生命周期内的 P47 exact source、receipt/profile、parent-child closure、P6 revision 与 frozen BaseQuick target proof。
- `Source/demo_map/CodeB/demo_mapCodeBP4.h`、`Source/demo_map/CodeB/demo_mapCodeBP4.cpp`：既有 payload 携带 P47 proof；whole-graph Preview 固定为一个 `Move(1)` 并沿原 CommitPreview 调用。
- `Source/demo_map/CodeB/demo_mapCodeBP3.h`、`Source/demo_map/CodeB/demo_mapCodeBP3.cpp`：签名兼容地把 P47 proof 写入唯一 P2 command；P3 的 BeforeSnapshot、单 P1 调用与 persistence-failure rollback 保持不变。
- `Source/demo_map/CodeB/demo_mapCodeBP3UI.h`、`Source/demo_map/CodeB/demo_mapCodeBP3UI.cpp`：构造并重验 P47 proof；在共享 Ctrl resolver 中建立 BaseQuickOnly 候选、冻结 first-empty exact address，拒绝 stale 后重扫，并在 accepted commit 后刷新既有 P10/P6 只读投影。
- `Source/demo_map/CodeB/demo_mapCodeBOutOfRaidProfile.cpp`：既有 P9/P6 durable writer 内从 durable truth 重建 P18 r2 source/closure、first-empty target 与同一 P1 `Move(1)`，要求精确 replay snapshot 相等后才进入原单次保存。
- `Source/demo_map/demo_mapV3ProgressionManager.cpp`：production P10 callback 在 durable write 前重验 active page/Host/workspace Owner/Run 与 BaseQuickOnly frozen target；不持有第二库存真值。
- `Docs/Report/Dev.D.UE.0.0.9B.P47.0.r0_report.md`：本报告。

明确未修改：任何测试文件、P1 transaction core、P9/P10 materialization/search/reveal state machine、P18 profile/gate/weight/history、P17 graph construction、P31 schema/Registry、P8 terminal、P13 binding、P15 use、P5/P6 bridge、Code A inventory/loot authority、Actor 类或地图内容。未新增 Button、hotkey、pointer handler、Widget、Actor、second resolver、ItemId/ContainerId allocator、第二 Repository writer、第二 save 或 Code A 写入口。

## 调用图与单一权威

1. `UCodeBP3CellButton::NativeOnMouseButtonDown` 仍是 Ctrl + 左键唯一消费点；命中后调用原 `UCodeBP3InventoryWidget::HandleQuickTransfer` 并返回 Handled，不继续 selection、search、drag threshold、Use、right-click 或 Actor interaction。
2. 原 `BeginP4Drag`／`PopulateTransferContext` 从 P2 projection、P10 normal projection、workspace 与 P9 receipt 形成只读 source address；`PopulateP47NormalContainerSpatialGraphQuickTransferProof` 只接受 exact NormalContainerTarget 的 Revealed BasicCache r2 spatial root。
3. shared resolver 在输入时显式设置 `BaseQuickOnly`，清空 current-child identity；只在 exact `Basic6` 内按 stable SlotIndex 找第一个空 cell，冻结 ContainerId、SlotIndex 与 composite revision。Preview/Commit 只重验该地址，失效即拒绝，不重扫、不 fallback。
4. 原 `PreviewInventoryTransfer` → `CommitInventoryTransfer` → P4 → P3 → P2 → P1 路径只提交一个 whole-graph `Move(1)`。P3 仍在一个 BeforeSnapshot 上执行并在 durable callback 失败时回滚。
5. `CommitAcceptedMatchedRunNormalContainerTransfer` 在一个局部 Owner candidate 中重建 P9/P6 prior composite、回放同一命令并要求结果全字段等于 accepted snapshot；之后沿既有 partition、P13 reconcile、P9/P6 durable replacement 与唯一 `SaveRecord`。

不存在 Widget direct Move/graph mutation、Actor pickup、display-cache write、right-click Take、double-click、按 selection/first/last 猜目标、预建 identity、WorldDrop record/ordinal/Actor 写入、第二 transaction/save 或 Code A inventory writer。

## P18 source、graph 与生命周期证明

- source 必须共同匹配 current P9 `RunLocalNormalContainerRecord` 与 P10 `NormalContainerTarget`：OwnerId、RunInstanceId、SearchTargetId、DefinitionId=`CodeB.NormalContainer.BasicCache`、ReceiptId、definition revision/digest、BasicCache r2 ProfileId/version/digest、AlgorithmVersion、loot/materialization digest、record/P6 revisions、target-open generation、workspace pane、source ContainerId/SlotIndex/ItemId。
- root 只能是 canonical `WindTalisman` 或 `BackpackLevel1`，Quantity=1、non-stackable、MaxStack=1；Definition item/equip/spatial semantic 必须分别匹配 QuickRing 或 StoragePouch。source ordinary slot reverse pointer、RevealState=`Revealed`、State=`Open` 且无 active action/search 均须成立。
- transient projection检查唯一 child owner、正式 child projection/capacity和全空 slots；durable Store另外用 canonical closure验证 stable `SpatialChildGuid(ItemId)`、parent-child identity、empty child、无环、无 orphan、无 duplicate owner和一层拓扑。graph 身份只允许 root placement 从 P9 移到 P6，ItemId、ChildContainerId、capacity 与 child slots保持不变。
- P47 未修改 P18 catalog、optional gate、weight、candidate、roll、digest、materialization或已存在 r1/r2 history；只读取 exact receipt和已有 graph。

Hidden/Searching、simple stack、child item、equipment、corpse、WorldDrop、warehouse、player source、Hotbar、another NormalContainer、wrong receipt/profile/generation/session 或 stale source不能建立有效 P47 proof，均保持零写入。

## BaseQuickOnly、事务与失败策略

- 唯一 target 是 active P6 `Basic6` ordinary storage；严格从 SlotIndex 0 升序取第一个 durable truth 中为空且在 capacity 内的 cell。SpatialRing、Backpack/Weapon/Armor/Accessory equipment、任何 P17 child、Hotbar、P5/P9/P11/P14/P31 与 other target不参与扫描。
- current child 的 open/switch/close/focus/full 状态不进入 target资格；proof显式清空其 ContainerId、parent ItemId与 generation。无 BaseQuick 空格时拒绝，不打开/切换 child、不自动装备、不选择第二容器。
- Preview冻结一个 target；Commit对同一个 ContainerId/SlotIndex/revision重验。目标占用、source/target revision变化、page/Host/session/Owner/Run失效或保存失败均拒绝，不重扫第二空格。
- accepted path只允许 `QuickTransfer + BaseQuickOnly + Move + Quantity=1`；不允许 Merge、Split、Swap、Replacement、Quantity=0、partial graph、new ItemId/ContainerId/child/receipt/record/ordinal/Actor。
- P9 source只在 exact replay和accepted snapshot证明完整 root已离开后，随P6 target在同一个Owner candidate内提交。P13只运行既有 reconcile；P8 terminal/recovery语义、selection、scroll与stable slot identity未改写。任一检查或SaveRecord失败由既有BeforeSnapshot/局部candidate边界保持零写入。

## 非回归结论

- P10 normal Drag Move/Merge/Swap与P18 explicit graph Drag不进入P47 proof，继续原显式目标语义。
- P46 BasicCache simple-stack仍使用其current-child-priority/merge-first规则；P47 proof与P46及body proofs互斥，完整graph不会获得Merge或child target。
- P30 WorldDrop与P41 corpse complete-graph Ctrl继续使用各自 provenance、record/body lifecycle；P47不读取WorldDrop ordinal/Actor或corpse receipt。
- P29/P34/P36/P37/P39/P40、P43—P45、P26—P31、P5/P6/P8/P13/P15、P17/P18和Code A authority均未获得新的产品写语义。
- ordinary click、right-click、double-click、normal Drag、Actor interaction、Shift+1—9、player-side Ctrl、P6→BasicCache与P18 child item均未获得P47隐式位置或graph写入语义。

## 静态检查与编译

- `git diff --check`：通过。
- 实际源代码改动：9 个文件；测试文件改动：0。
- 未启动产品、PIE、Standalone、真实鼠标键盘、截图、Smoke、Automation、回归、Cook或Package。

Editor：

```text
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_mapEditor Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload
native exit code: 0
Result: Succeeded
Total execution time: 7.35 seconds
```

Game：

```text
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_map Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload
native exit code: 0
Result: Succeeded
Total execution time: 13.00 seconds
```

## 留给 0.0.9B.F 的真实验证

未执行：BasicCache r2 materialize、optional spatial hit/no-hit、open/reveal；真实 canonical WindTalisman与BackpackLevel1 Ctrl拾回至first legal BaseQuick；空/满BaseQuick、stable SlotIndex、完整parent/child identity、child open/switch/close不成为target；wrong/simple/equipment/child/Hidden/Searching source；SearchTargetId/receipt/profile/Owner/Run/P9/P6 revision stale；Target close/reopen、P10 action/search、Prepared/terminal/Host invalid/SaveRecord failure；P10 normal Move/Merge/Swap、P18 normal graph Drag、P46、P29/P30/P34/P36/P37/P39/P40/P41、P43—P45、P26—P31、P8 terminal/recovery；真实鼠标键盘、PIE、Standalone、截图、Smoke、Automation、回归、Cook与Package。
