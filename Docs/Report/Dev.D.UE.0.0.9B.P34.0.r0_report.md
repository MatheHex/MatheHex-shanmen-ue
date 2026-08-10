# Dev.D.UE.0.0.9B.P34.0.r0 Report

## 1. 结果

- 任务：`Dev.D.UE.0.0.9B.P34.0.r0`。
- Prompt 已按真实下载文件归档；源与归档 SHA-256 均为 `242328A0F7ABCD1DC264EBDD1DD8EFE295C499986333D5AEA65E7A9967B442B2`。
- 当前已打开、身份匹配且 provenance 为 P32/P33 accepted standard-equipment 的 non-spatial Weapon、Armor/ArmorRobe、普通 Accessory `WorldDrop` root，现可通过共享 `Ctrl + 左键` QuickTransfer 移入 active P6 `BaseQuick` 按真实 stable `SlotIndex` 升序的首个合法空格。
- accepted path 仍是一次 P1 whole-root `Move`、一个 Owner/Run candidate、一次 `SaveRecord`；本项使用显式 `Quantity=1`，不进入 P29 `Merge(0)` 或 P30 complete-graph `Quantity=0` 语义。
- 没有自动装备、active P17 child fallback、Swap、Replacement、Sort、Compact、quick-drop、Actor direct pickup 或第二库存真值。
- P29 simple-stack、P30 P19 complete graph、P32/P33 normal Drag、P31 Registry、P8 full-registry exclusion与 P13 reconcile保持。
- Editor与 Game均以 native exit code `0` 编译成功。
- 未启动产品，未执行 PIE、Standalone、真实输入、截图、Smoke、Automation、回归、Cook或 Package。

## 2. 改动前活动调用图审计

### 2.1 shared Ctrl + 左键链

`UCodeBP3CellButton::NativeOnMouseButtonDown`
→ 唯一 `Ctrl + 左键`消费点，建立一次 pointer gesture并返回 `Handled`
→ `UCodeBP3InventoryWidget::HandleQuickTransfer`
→ 既有 `FCodeBP4DragPayload`，只设置 transient `bQuickTransferIntent`
→ `ResolveQuickTransferDestination` / family-specific deterministic resolver
→ `PreviewInventoryTransfer`
→ `FCodeBP4InteractionController::PreviewDrop`
→ `CommitDrop`
→ `FCodeBP3UIController::CommitP4Operation`
→ 一个 `FCodeBP2Command(Intent=QuickTransfer)`
→ P2 `Apply`
→ P1 `ExecuteTransaction`
→ accepted profile callback与 BeforeSnapshot rollback。

P34没有增加 Button、hotkey、pointer handler、Widget direct Move、Actor click或第二 P2/P1入口。ordinary click、right-click、double-click、drag threshold、P15 Use、`Shift + 1—9`与 player-side standard-root Ctrl均未获得 P34位置写入语义。

### 2.2 durable Store链

`PersistProfileSnapshotAfterAcceptedP1`
→ `CommitAcceptedMatchedRunWorldDropPickup`
→ 重开 exact Owner/Run active P6 session
→ exact WorldDropId/Ordinal/record revision/P6 revision/root/container gate
→ P34 accepted-command与完整 snapshot delta proof
→ 删除 matching record及其已空 derived world container
→ other-record closure unchanged proof
→ P13 reconcile / payload receipt freeze / full session validation
→ 一个 Owner candidate与一次 `SaveRecord(Candidate)`
→ manager只在 durable accepted之后刷新 matching Actor projection。

失败时 profile callback把内存 Repository恢复至 BeforeSnapshot；Store candidate只有 `SaveRecord`成功后才替换活动 Record。没有先删 record/Actor、第二 save、Actor-first领取或双写。

## 3. P34 source eligibility与 provenance

- transient presentation新增只读 `Provenance`，由 exact opened P31 record复制；它只用于共享 resolver的 source-family gate，不拥有 Item/Container authority。
- P34只接受：
  - `P32.AcceptedGroundDrop.StandardEquipment`；
  - `P33.AcceptedGroundDrop.BaseQuickStandardEquipment`。
- UI family gate同时要求 exact WorldDropTarget revealed root、无 child、non-stackable、`MaxStack=1`、`Quantity=1`，并要求正式 `Weapon/Weapon`、`Armor/Armor`或 `Accessory/Accessory`组合。
- durable Store重新从 P1 snapshot与 canonical Catalog验证完整 definition相等、spatial semantic none、child capacity 0、无 `ChildContainerId`、derived world container slot 0反向指向同一 ItemId、record available及 accepted provenance。
- stack、partial draft、P19 graph、P17 child、SpatialRing、Backpack、warehouse、normal container、corpse、Hotbar、unknown family、UI-only object与 another record均不会进入 P34 accepted proof。

## 4. 唯一 BaseQuick target与 SlotIndex次序

- P34与 P30一样显式清除 transient active-child preference；resolver只查找 Projection中的 `BasicContainerId` 且 role必须为 `Basic6`。
- merge-first循环对 P34禁用；只执行空格扫描。
- UI按 `SlotIndex=0..Capacity-1`查找同一正式 container的 read-only slot，只有明确空格且 preview为 `Move(1)`时提交。
- durable proof从 active P1 `Layout.BasicContainerId`重取 container，验证非 equipment、target index有效且为空，并要求所有更小 SlotIndex均已占用。因此只有真实 stable SlotIndex升序的第一个空 BaseQuick格可被接受。
- 没有合法空格时循环结束并零写入；无 equipment、child、P5/P9/P11、Hotbar、other-record、Swap、Replacement、挤位、Sort、Compact或最近空位 fallback。

## 5. P1 whole-root Move与 accepted delta

- P4对符合 P34 topology的 QuickTransfer empty-target preview输出 `Operation=Move, Quantity=1`；P1 `ExecuteMove`继续使用既有 whole-root语义，Quantity不参与拆分或合并。
- durable proof要求 exact transaction identity、ItemId、world source slot 0、BaseQuick target、expected revision、invalid active-child override与 `Quantity=1`。
- expected snapshot从 BeforeSnapshot复制，只执行：world slot 0清空、BaseQuick exact target写入同一 ItemId、同一 root的 parent/slot改为 BaseQuick target、revision加一。
- Candidate必须与 expected snapshot全等，因而 DefinitionId、ItemId、Quantity=1、Level、Quality、RandomSeed、LegacyAffixDigest、provenance、no-child资格、definitions、containers与所有无关 items均保持。
- accepted cleanup只移除 exact `WorldDropId`与空 world container；`NextWorldDropOrdinal`不变，不生成 ItemId、ContainerId、record、ordinal、Actor、child、binding或第二 revision。

## 6. P29/P30/P32/P33与 Registry边界保持

- P29仍只接受 simple-stack source，保留 active P17 child优先、BaseQuick回退、merge-first/empty-second及双向 `Move/Merge(0)`；P34 standard root不会进入 P29 proof。
- P30仍只接受带正式 child closure的 WindTalisman/Backpack P19 root，目标仍为首空 BaseQuick，command仍为 `Quantity=0`；P34不会调用 graph closure proof。
- P32/P33 normal Drag仍接受 exact opened root到明确空 BaseQuick的 Standard `Move(0)`或明确空兼容 equipment位的 Standard `Equip(0)`；P34不修改这些路径，也不自动选择装备位。
- player-side standard root的 Ctrl仍在 shared handler中拒绝；没有 standard quick-drop或 quick-equip。
- P31 canonical order、migration、ordinal accepted-create-only、record-to-record隔离、Actor open identity与 other-record unchanged proof未改。
- P8仍遍历全 Registry并排除 remaining world roots、derived world containers及空间 closure；P34不重写 terminal、receipt、recovery或 run replacement。

## 7. 生命周期、回滚与 Code A边界

- OwnerId、RunInstanceId、WorldDropId、Ordinal、record revision、open generation、map route、P6 revision、exact root/container及 source address在 preview/commit两侧继续重验。
- close、cancel、focus loss、EndPlay、stale payload或 projection refresh不会绕开 identity gate；失败不清理 durable record或 Actor。
- Save失败时 Store不替换 Record，P3把 Repository恢复至 BeforeSnapshot；不存在 phantom empty BaseQuick、phantom pickup或 transient item copy。
- successful save后才刷新 exact Actor diff；Code A仍只承担 floor/Actor/交互边缘投影，不拥有或复制 Item、Container、Quantity、Run、Registry或持久化 authority。
- selection/scroll、动态空间容量、P17 child SlotIndex、Hotbar bindings与无关 WorldDrop records未获得额外写入。

## 8. 修改文件与提交

- `Source/demo_map/CodeB/demo_mapCodeBP4.cpp`
  - P34 whole-root QuickTransfer empty-target preview显式携带 `Quantity=1`。
- `Source/demo_map/CodeB/demo_mapCodeBP3UI.h/.cpp`
  - transient WorldDrop presentation投影只读 provenance；
  - shared resolver新增 P34 source-family分支与 BaseQuick-only目标策略；
  - normal Drag、P29/P30与 player-side拒绝边界保持。
- `Source/demo_map/CodeB/demo_mapCodeBOutOfRaidProfile.h/.cpp`
  - 新增 P34 exact command/first-empty/canonical/provenance/full-delta proof；
  - 复用唯一 durable pickup writer、single save、exact cleanup与 rollback链。
- `Source/demo_map/demo_mapV3ProgressionManager.cpp`
  - 从 exact record向 transient presentation复制只读 provenance。

提交：

- Prompt归档：`cf2c849 docs: archive Dev.D.UE.0.0.9B.P34.0.r0 prompt`
- 实现：`bebf234 feat: quick-pick standard equipment world drops`

## 9. 静态审查

- `git diff --check`：通过。
- 唯一 pointer router保持；没有新增输入、Widget writer、Actor writer、Repository或 save入口。
- P34 source同时受 UI read-only family gate与 Store canonical/provenance authoritative gate约束。
- target同时受 UI BaseQuick-only扫描与 Store first-empty exact proof约束。
- accepted candidate只允许同一 root placement与 snapshot revision变化；所有无关图值通过全 snapshot equality及 other-record closure proof保持。
- exact matching record删除计数必须为 1；ordinal、other records、P8 exclusion、P13 reconcile与 Actor accepted-after-save顺序保持。

## 10. 编译

Editor：

```text
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_mapEditor Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload
```

- native exit code：`0`
- 结果：`Result: Succeeded`
- UnrealBuildTool total execution time：`0.90 seconds`（目标已由同一次编译序列更新，复核命令返回 up to date）。

Game：

```text
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_map Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload
```

- native exit code：`0`
- 结果：`Result: Succeeded`
- UnrealBuildTool total execution time：`28.94 seconds`

## 11. 留给 0.0.9B.F 的真实验证

- 分别由 P32 equipped source与 P33 BaseQuick source建立多个 Weapon、Armor/ArmorRobe、普通 Accessory standard records；
- 对每类 exact opened record执行 Ctrl拾回，确认进入真实首空 BaseQuick SlotIndex，且 record/container/Actor只清理 matching identity；
- BaseQuick无空格时零写入；normal Drag回明确 BaseQuick/正确空装备位保持；
- stack、partial、space parent/child、P17 child、warehouse、normal container、corpse、Hotbar、unknown provenance与 another record拒绝；
- P29 simple stack与 P30 complete graph双向/单向边界保持，player-side standard Ctrl拒绝；
- stale、close、cancel、focus loss、EndPlay、recovery、terminal、save failure与 BeforeSnapshot rollback；
- Actor projection、P8 full-registry exclusion、P13 reconcile、ordinal不变、scroll/selection/stable SlotIndex；
- 真实鼠标键盘、截图、Smoke、Automation、回归、Cook、Package与最终验收。

READY_FOR_P35_PLANNING
