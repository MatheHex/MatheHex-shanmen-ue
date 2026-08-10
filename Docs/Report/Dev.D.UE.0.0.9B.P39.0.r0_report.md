# Dev.D.UE.0.0.9B.P39.0.r0 Report

## 结果

- 状态：`READY_FOR_P40_PLANNING`
- 实现提交：`3e859b7 feat: add P39 corpse equipment quick transfer`
- 范围：只实现 P39、静态审查并编译 Editor/Game；未启动产品，未执行 PIE、Standalone、真实输入、截图、Smoke、Automation、回归、Cook、Package 或 F 阶段测试。

## 1. 文件清单与职责

### 已修改

- `Source/demo_map/CodeB/demo_mapCodeBP2.h`
  - 将既有 P38 瞬时尸体装备证明明确扩展为 P38/P39 共用；P2 command 继续只携带不可持久化的 source/body/child identity 与 frozen target mode。
- `Source/demo_map/CodeB/demo_mapCodeBP4.h`
  - 明确 P38/P39 共用 payload 中的 P21 source 与 input-time current-child proof；未增加第二 payload 或 resolver。
- `Source/demo_map/CodeB/demo_mapCodeBP4.cpp`
  - 既有 projection-only Preview 继续把 P21 standard non-spatial root 到空 ordinary Cell 计划为 `Move(1)`；仅更新共享 whole-root QuickTransfer 边界说明。
- `Source/demo_map/CodeB/demo_mapCodeBP3UI.h`
  - 标注现有 `ValidateP38BodyEquipmentTransferContext` 同时承担 P38 normal Drag 与 P39 frozen-target gate；未新增 Widget、Button、pointer handler 或第二 QuickTransfer 入口。
- `Source/demo_map/CodeB/demo_mapCodeBP3UI.cpp`
  - 在唯一 `Ctrl + 左键` router 中加入 P21 r3 corpse-equipment canonical source branch。
  - 输入时一次性冻结 `CurrentP17Child` 或 `BaseQuickNoChildAtInput`，按 stable `SlotIndex` 选择 exact ordinary container 的首个空格。
  - Preview/Commit 前重验 P12 BodyTarget、P21 source、Owner/Run/revision、page/open、mode 与 target identity；P39 禁止 equipment target、fallback、merge、split、swap 与 replacement。
- `Source/demo_map/CodeB/demo_mapCodeBOutOfRaidProfile.cpp`
  - durable Store 接受 P39 的 one-command P21 `QuickTransfer Move(1)`，重建 P21 r3 source、P17 parent/child 或 BaseQuick target、首空格顺序和 exact composite delta。
  - P11 corpse source 与 P6 player target 继续在一次 Owner durable replacement 与一次 `SaveRecord` 中提交；失败沿既有 BeforeSnapshot 路径零写入。
- `Source/demo_map/demo_mapV3ProgressionManager.cpp`
  - 生产 BodyTarget commit callback 在 durable Store 前复核 P39 frozen mode、current child open generation 与 P12 Host 生命周期；只做 read-only gate/forwarding，未获得 Item/Container durable authority。

### 新增

- `Docs/Report/Dev.D.UE.0.0.9B.P39.0.r0_report.md`
  - 本报告。

### 明确未修改

- `Source/demo_map/CodeB/demo_mapCodeBInventory.cpp/.h`：P1 Repository、whole-root `Move`、装备规则与事务不变；P39 直接复用。
- `Source/demo_map/CodeB/demo_mapCodeBP2.cpp`：P2 单命令 application service 与 callback 顺序不变。
- `Source/demo_map/CodeB/demo_mapCodeBP3.cpp/.h`：P3 command 提交、BeforeSnapshot rollback、projection refresh 与 P13 reconcile 不变。
- `Source/demo_map/CodeB/demo_mapCodeBOutOfRaidProfile.h`：P11/P21 receipt、profile、record 与 P31 schema 未新增字段。
- 所有 `*Tests.cpp`：P 阶段未修改、未执行测试。
- P31 Registry、WorldDrop Actor/record、P17 graph construction、P21 deterministic materialization/history、P8 terminal、P13 binding、P15 use、P5/P6 bridge与 Code A 功能逻辑均未修改。

## 2. 唯一输入与提交调用图

```text
UCodeBP3CellButton::NativeOnMouseButtonDown
  -> UCodeBP3InventoryWidget::HandleQuickTransfer
  -> BeginP4Drag / UCodeBP3UIHostSubsystem::PopulateTransferContext
     -> 复用 P38 P21 BodyTarget/source proof
  -> 输入时冻结 CurrentP17Child 或 BaseQuickNoChildAtInput
  -> stable SlotIndex 首空 ordinary Cell
  -> PreviewInventoryTransfer
     -> ValidateTransferContext
     -> ValidateP38BodyEquipmentTransferContext (P38/P39 shared gate)
     -> FCodeBP4InteractionController::PreviewDrop => Move(1)
  -> CommitInventoryTransfer / CommitDrop
  -> FCodeBP3UIController::CommitP4Operation
  -> FCodeBP2ApplicationService
  -> FCodeBRepository::ExecuteTransaction (single P1 Move)
  -> production BodyTarget commit callback
  -> FCodeBOutOfRaidProfileStore::CommitAcceptedMatchedRunBodyContainerTransfer
  -> one P11/P6 Owner durable replacement + one SaveRecord
```

证据：仍只有 `NativeOnMouseButtonDown` 调用现有 `HandleQuickTransfer`；没有新增 Button、hotkey、pointer handler、Widget、Actor click、second resolver、direct Move 或 Code A 写入口。

## 3. P21 r3 canonical source

实际固定定义保持不变：

| Slot semantic | DefinitionId | ItemType / EquipSlot | Container type |
| --- | --- | --- | --- |
| `Body.Weapon` | `HeavyPracticeBlade` | Weapon / Weapon | `CodeB.Body.Weapon` |
| `Body.ArmorRobe` | `ReinforcedVest` | Armor / Armor | `CodeB.Body.ArmorRobe` |
| `Body.Accessory0` | `EvasionCharm` | Accessory / Accessory | `CodeB.Body.Accessory0` |

P39 只接受 exact equipment container slot 0 中 `Quantity=1`、`MaxStack=1`、non-stack、non-spatial、无 `ChildContainerId` 的 single standard root。Host 与 Store 同时核对 `CodeB.LootProfile.BasicCorpse.r3`、version 3、death receipt、loot profile digest、equipment candidate-set digest、BodyTargetId、source container/slot/root 与 Catalog canonical definition。P21 r1/r2、已 materialized record、roll group、weight、seed、candidate identity 与 deterministic history均未修改、未迁移、未重掷。

## 4. Source、生命周期与零写入 gate

- source 必须属于当前 production P12 Host 的 exact opened BodyTarget；Item visibility 必须为 `Revealed`，且无 active action/search。
- payload/proof 冻结并在 Preview、Commit callback 与 durable Store 复核：OwnerId、RunInstanceId、BodyTargetId、DeathReceiptId、BodyRecordRevision、BodyTargetOpenGeneration、P6 composite revision、source ContainerId/slot 0/ItemId、profile/digest、workspace pane 与 active session。
- ordinary P12 corpse slot、Hidden/Searching、空 equipment slot、stack、space parent/child、P19 graph、WorldDrop、warehouse、Hotbar、player source、another BodyTarget、历史非 r3 record 与 unknown provenance 均不能建立 P39 accepted command。
- close/reopen、focus/route/Host invalidation、search lifecycle、receipt/root/container mismatch、Owner/Run/revision stale、Prepared/terminal gate 或保存失败均在既有 synchronous commit/rollback 边界内拒绝，未产生第二库存、phantom empty slot 或 transient item copy。

## 5. 两种 frozen target mode

### `CurrentP17Child`

- 仅当 input-time workspace 存在 identity-valid current P17 child 时建立。
- intent 同时冻结 exact child `ContainerId`、formal spatial parent `ItemId`、child open generation、Owner/Run、P6 revision与 P21 source/body proof。
- UI 只在该 exact child 中按真实 `SlotIndex` 从 0 递增选择第一个空 ordinary Cell；Host 与 Store再次证明 parent 位于正式 `SpatialRing`/`Backpack` placement、parent → child 唯一、dynamic capacity匹配、child 无第二层 spatial graph。
- child 关闭、切换、失焦、generation/parent/capacity/revision 失效或满位时拒绝；绝不改投 BaseQuick、另一 child、装备位、Hotbar 或其他容器。

### `BaseQuickNoChildAtInput`

- 只有 input-time 不存在 identity-valid current P17 child 时建立；command 与 P38/P39 proof 中 child id、parent id、generation全部为空/0。
- target 只可能是 active P6 `Basic6`/BaseQuick，按真实 `SlotIndex` 选择第一个空 ordinary Cell。
- 输入后新打开或切换 child 不改变该 frozen mode；BaseQuick 满或 stale 时拒绝，不扫描 child、equipment、Hotbar、P5/P9/P11 或 world target。

两种模式的 Preview 与 Store 都验证所有更小 SlotIndex 已占用，因此不能跳过更早空格。P39 不调用 Equip、Unequip、Merge、Split、Swap、Replacement、Sort、Compact、bind 或 use。

## 6. 单一事务、持久化、回滚与投影

- accepted command 必须是一个 `ECodeBP2CommandIntent::QuickTransfer`、一个 `ECodeBOperation::Move`、`Quantity=1`、一个 TransactionId。
- 同一 ItemId 从 exact P11 body equipment container slot 0 直接移动到 frozen target；DefinitionId、Quantity、Level、Quality、RandomSeed、LegacyAffixDigest、candidate provenance 与 non-spatial qualification保持。
- durable Store 合并当前 P11/P6 canonical snapshots，重放同一个 P1 `Move(1)` 并要求 candidate 全量相等；没有预清 source、staging container、第二 Move、第二 revision、new ItemId/ContainerId/receipt/record/ordinal/Actor/child。
- P11 source 与 P6 target 只在 replay proof 成功后进入一个 Owner durable replacement；仍只调用一次 `SaveRecord`。任何 canonical gate、mode、target、P1、projection、P13 reconcile 或保存失败都走既有 BeforeSnapshot rollback。
- 成功后沿既有 P2/P3 callback 刷新 exact BodyTarget source 与 resolved P6/必要 P7 projection；不 Sort/Compact，不重排无关 SlotIndex，不创建或修改 WorldDrop record/Actor。

## 7. 非回归与权威边界

- P38 normal Drag 仍只接受用户明确指定的空 BaseQuick、current child 或兼容空装备位；P39 quick branch不能使用装备位。
- P29 simple-stack、P30 complete graph、P34/P36/P37 standard WorldDrop Ctrl branches未修改；它们仍由既有 provenance 分支处理。
- P12 ordinary corpse Move/Merge/Swap、P21 P6 → corpse equipment 拒绝、P17 one-layer graph、P19 complete graph、P26—P28 stack、P31 Registry、P8 terminal/recovery、P13 binding与 P15 use 产品语义未修改。
- Code A 只在已有 progression manager callback 中读取 Host/workspace 生命周期并转发 accepted command；Item、Container、Quantity、P11/P6、BodyTarget、search、terminal与 player-equipment durable authority仍属于 Code B。

## 8. P 阶段编译

### Editor

```text
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_mapEditor Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload
```

- 原生 exit code：`0`
- 关键结果：`Result: Succeeded`
- 总耗时：24.62 秒。

### Game

```text
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_map Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload
```

- 原生 exit code：`0`
- 关键结果：`Result: Succeeded`
- 总耗时：28.59 秒。

另完成 `git diff --check`，exit code `0`。未执行额外构建。

## 9. 明确留到 0.0.9B.F 的真实验证

本轮未执行以下项目：

- P21 r3 future corpse materialize/reveal。
- Weapon、ArmorRobe、Accessory 分别以 `Ctrl + 左键`拾回至当前 opened valid child，以及 input-time 无 child时进入首个空 BaseQuick。
- child full、closed、switched、focus loss、open-generation stale、parent/capacity/topology stale与无 fallback。
- BaseQuick full；input 后新开 child 不改变 BaseQuick mode。
- wrong/ordinary/Hidden/Searching source；BodyTarget/death receipt/Owner/Run/P6/body revision stale。
- Prepared/terminal、Host invalid、SaveRecord failure与 BeforeSnapshot rollback。
- P38 normal Drag、P12 normal Move/Merge/Swap、P29/P30/P34/P36/P37 Ctrl branches、P31 other-record isolation、P8 terminal/recovery。
- 真实鼠标键盘、PIE、Standalone、截图、Smoke、Automation、回归、Cook、Package与最终验收。

READY_FOR_P40_PLANNING
