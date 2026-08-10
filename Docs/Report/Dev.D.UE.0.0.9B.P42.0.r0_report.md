# Dev.D.UE.0.0.9B.P42.0.r0 Report

## 1. 结果

P42 已完成：当前 P12 production Host 中已打开、已揭示且 identity-valid 的 BasicCorpse ordinary root 若为 P20 r2／P21 r3 继承的 canonical 空-child `WindTalisman` 或 `BackpackLevel1` 完整空间图，可经既有 normal Drag/Drop 直接进入用户明确指定的唯一兼容空 P6 正式装备位。`WindTalisman` 只进入 `SpatialRing`，`BackpackLevel1` 只进入 `Backpack`；目标在 Drop 时冻结，Commit 与 durable Store 只重验该 exact ContainerId／SlotIndex／semantic／revision，不扫描、不回退、不自动装备、不进入 child。

实现提交：`13c67e5 feat: add P42 corpse spatial equipment drag`。

## 2. 文件与职责

新增／修改：

- `Source/demo_map/CodeB/demo_mapCodeBP2.h`：新增 P42 source／target／revision 瞬时证明并接入唯一 P2 command。
- `Source/demo_map/CodeB/demo_mapCodeBP4.h`：在既有 drag payload 中携带 P42 证明；未建立第二物品状态。
- `Source/demo_map/CodeB/demo_mapCodeBP3.h`、`demo_mapCodeBP3.cpp`：沿既有 P3 → P2 command 原样转发 P42 证明。
- `Source/demo_map/CodeB/demo_mapCodeBP4.cpp`：P42 exact formal target 仅预览 `Move(1)`；occupied/wrong target 拒绝，无 Equip/Replacement/Swap。
- `Source/demo_map/CodeB/demo_mapCodeBP3UI.h`、`demo_mapCodeBP3UI.cpp`：从既有 P12 normal Drag 建立 source proof；在 Drag preview／actual Drop 的临时 payload 中冻结用户 exact target，并在 Preview／Commit 重验。
- `Source/demo_map/CodeB/demo_mapCodeBInventory.cpp`：为 `CodeB.BodyContainer.BasicCorpse` 的 canonical spatial parent 增加狭窄 P1 `Move(1)` 至对应正式空装备位分支。
- `Source/demo_map/CodeB/demo_mapCodeBOutOfRaidProfile.cpp`：在既有 P11/P6 durable callback 内重建 P20/P21 provenance、完整空-child closure 与 exact formal target，并用同一 P1 Move replay 验证 accepted composite。

未修改：P11/P12 search/open state machine、P20/P21 materialization/profile/history/determinism、P17 graph construction、P31 schema/Registry、P8 terminal/recovery、P13/P15、Code A、所有测试文件、PROJECT 文档。

## 3. 调用图与输入边界

`UCodeBP3CellButton::NativeOnDragDetected` → 既有 `UCodeBP4DragOperation`／`BeginP4Drag` → `PopulateTransferContext`／P42 source proof → 既有 `UCodeBP3CellButton::NativeOnDrop` → Drop 临时副本冻结 exact target → `PreviewInventoryTransfer` → `FCodeBP4InteractionController::PreviewDrop` → `CommitInventoryTransfer` → `CommitDrop` → `FCodeBP3UIController::CommitP4Operation` → P2/P1 → `CommitAcceptedMatchedRunBodyContainerTransfer` → one P11/P6 durable replacement／one SaveRecord。

没有新增 Button、hotkey、pointer handler、DragOperation、Widget、Actor click、second resolver、Widget direct Move 或 Code A 写入口。P42 proof 只在 normal Drag 起点建立；进入既有 Ctrl QuickTransfer 路由后立即清除，因此 P41 仍为 BaseQuickOnly，普通左键、right-click、double-click、player-side Ctrl、Shift+1—9 与 Actor interaction 均未获得 P42 语义。

## 4. Source、lifecycle 与完整图证明

- source 必须是 current `BodyContainerTarget` ordinary root 的 exact ContainerId／SlotIndex／ItemId；BodyTarget 为 Open、无 active action/search、root 为 Revealed，且 target-open generation、OwnerId、RunInstanceId、BodyTargetId、DeathReceiptId、body/P6 revision、workspace pane 与 active session 当前有效。
- source 只接受 `CodeB.BodyContainer.BasicCorpse` 的 P20 r2 或 P21 r3 inherited spatial group，以及 canonical `WindTalisman`／`BackpackLevel1`。P21 fixed equipment、simple stack、child Cell、Hidden/Searching、WorldDrop、P5/P6 player source与其他 BodyTarget 均不能建立 accepted P42 intent。
- UI proof 冻结 source stable address、parent ItemId/DefinitionId、ChildContainerId、stable SpatialChildGuid、profile/receipt digests 与 revision；Store 从原始 P11 snapshot、Catalog、receipt/profile 和 P1 placement 重新验证。
- 既有 `ValidateP20BasicCorpseSpatialClosure` 继续证明唯一 formal child、正确 type/capacity、empty-child provenance、一层无嵌套、无环、无 orphan、无 duplicate owner、reverse slot pointer 与稳定 parent/child identity。P42 未重掷、补料或改写 P20/P21 profile、receipt、candidate、digest、history 或 materialization。

## 5. 用户指定 formal target 与零写入策略

- `WindTalisman` 只接受 active P6 唯一 `SpatialRing` container 的 slot 0；`BackpackLevel1` 只接受唯一 `Backpack` container 的 slot 0。P4 compatibility、P2 projection与 Store 中的 P6 layout／P1 ContainerType／EquipmentSlot／capacity／empty truth 必须一致。
- exact target 来自实际 Drag Enter／Drop address；临时 proof 冻结 ContainerId、SlotIndex、formal semantic 与 composite revision。Commit 不按数组顺序、selection、显示名或“首个兼容槽”重新解析。
- BaseQuick target 保留 P20 原 normal Drag payload，不建立 P42 intent。任何其他 non-BaseQuick target 会进入 P42 拒绝门；wrong/full/occupied/stale target 不改投 BaseQuick、另一装备位、P17 child、Hotbar 或 WorldDrop。
- BodyTarget close/reopen、search/action、Owner/Run/session/revision、source/closure/target变化、Host invalid、terminal/Prepared gate、P1拒绝或 SaveRecord failure 均沿现有 zero-write／BeforeSnapshot rollback 结束，不产生 phantom empty、phantom pickup、clone 或第二真值。

## 6. 事务、持久化与身份保持

- accepted path 只有一个 P1/P20 whole-graph `Move(1)`；不是先放 BaseQuick 再二次 Move，也不调用 Merge、Split、Swap、Equip 或 Unequip。
- parent ItemId、ChildContainerId、stable SpatialChildGuid、closure 中所有 ItemId／ContainerId、capacity 与 provenance 不变；唯一变化是 root 从 exact P11 ordinary body source 到用户冻结的 exact P6 formal equipment slot。
- durable Store 使用 accepted command 的相同 TransactionId／source／target／revision 重放一次 Move，并要求 replay snapshot 与 accepted composite 完全相等；随后按既有分区在同一 Owner record 中共同替换 P11 source 与 P6 target，函数内仅一次 SaveRecord。
- source 仅在 accepted snapshot/replay proof 后消失并刷新 exact BodyTarget/P6 projection；P13 仅沿既有 accepted reconcile，P8 terminal/recovery 不变。没有 WorldDrop record、ordinal、Registry、Actor、new ItemId、new ContainerId、new child 或第二 save。
- selection、scroll 与 stable SlotIndex 仍由既有 P3 refresh/restore 处理；P42 未增加独立 UI 状态或持久化缓存。

## 7. 非回归静态结论

- P20 complete graph normal Drag → empty BaseQuick 保持原路径；P42 只扩展用户明确指定的 formal equipment target。
- P41 Ctrl + 左键仍只冻结并选择 BaseQuick first-empty；P42 target proof 在 Ctrl 路由入口清除，绝不自动装备或扫描 child。
- P38/P39 P21 equipment、P40 simple-stack、P29/P30/P34/P36/P37 WorldDrop Ctrl branches、P26—P28 quantity、P31 Registry、P5/P6/P8/P13/P15、P17 construction 与 Code A authority 均未修改产品语义。
- 静态检查通过：改动仅限 9 个允许的 Code B 文件；`git diff --check` 通过；未改测试或 Code A 文件；existing Cell／DragOperation／NativeOnDrop 与 P1/P11/P6 transaction 被复用；accepted P42 明确为 Move(1)，没有 Quantity=0、target scan、fallback、child entry 或第二 SaveRecord。

## 8. 编译

Editor：

`"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_mapEditor Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload`

- native exit code：0
- Result：Succeeded
- Total execution time：25.87 s

Game：

`"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_map Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload`

- native exit code：0
- Result：Succeeded
- Total execution time：29.78 s

未启动产品、PIE、Standalone、真实输入或测试。

## 9. 留给 0.0.9B.F 的真实验证

未执行：P20/P21 future corpse materialize/reveal；actual canonical WindTalisman／BackpackLevel1 normal Drag 至 compatible empty SpatialRing／Backpack，以及原 BaseQuick normal Drag；full/wrong/occupied slot；wrong parent/child/simple/equipment/Hidden/Searching source；BodyTarget/death receipt/profile/Owner/Run/P6 revision stale；close/reopen、search/action、Prepared/terminal/Host invalid/SaveRecord failure；parent/child identity 与 empty-child closure；P41 QuickTransfer；P12 normal Move/Merge/Swap；P21/P38/P39、P40、P29/P30/P34/P36/P37 Ctrl branches；P31 other-record isolation；P8 terminal/recovery；真实鼠标键盘、PIE、Standalone、截图、Smoke、Automation、回归、Cook 与 Package。

READY_FOR_P43_PLANNING
