# Dev.D.UE.0.0.9B.P40.0.r0 Report

## 结果

- 状态：`READY_FOR_P41_PLANNING`
- Prompt：`Dev.D.UE.0.0.9B.P40.0.r0_prompt.md`，SHA-256 `F6656645D0A24988CC2A7EF13CBDF23DB318D66DC4B48E594EA6F5ABE2B86802`。
- 实现提交：`6fe59b6 feat: add P40 corpse stack quick transfer`
- 范围：只完成 P40 实现、静态审查及 Prompt 指定的 Editor/Game 编译；未启动产品，未执行 PIE、Standalone、真实输入、截图、Smoke、Automation、回归、Cook、Package 或 F 阶段测试。

## 1. 文件清单与职责

### 已修改

- `Source/demo_map/CodeB/demo_mapCodeBP2.h`：新增瞬时 `FCodeBP40BodySimpleStackQuickTransferProof`，携带 exact P12 source/body/receipt/revision/open-generation、P6 composite 与 frozen child/BaseQuick identity；不持久化第二库存或数量。
- `Source/demo_map/CodeB/demo_mapCodeBP4.h/.cpp`：沿既有 P4 payload/preview/commit 透传 P40 proof；compatible occupied target 仍是 `Merge(Quantity=0)`，empty ordinary target 仍是 whole-root `Move`。
- `Source/demo_map/CodeB/demo_mapCodeBP3.h/.cpp`：在既有单次 `CommitP4Operation` command 中透传 P40 proof；P3/P2/P1 单命令入口、BeforeSnapshot 与失败回滚顺序不变。
- `Source/demo_map/CodeB/demo_mapCodeBP3UI.h/.cpp`：在唯一共享 Ctrl router 中增加 exact opened/revealed P12 ordinary simple-stack source branch；输入时冻结 current P17 child 或 BaseQuick，并按 stable `SlotIndex` merge-first / empty-second；Preview/Commit 前重验 source、mode 与 candidate；accepted body transfer 后只读刷新 exact BodyTarget projection。
- `Source/demo_map/CodeB/demo_mapCodeBOutOfRaidProfile.cpp`：durable Store 从 P11/P6/Catalog/receipt 重建 P40 source、frozen target 与首 candidate，重放同一个 P1 transaction并要求 candidate snapshot 全字段相等，随后复用一次 P11/P6 Owner replacement 与一次 `SaveRecord`。
- `Source/demo_map/demo_mapV3ProgressionManager.cpp`：production BodyTarget callback 只读转发 projection refresh，并在 Store 前复核 P39/P40 Host、Actor、current-child/open-generation 生命周期；未获得 Item/Container/Quantity durable authority。

### 新增

- `Docs/Report/Dev.D.UE.0.0.9B.P40.0.r0_report.md`：本报告。

### 明确未修改

- `Source/demo_map/CodeB/demo_mapCodeBInventory.cpp/.h`：P1 `Move`、`Merge(Quantity=0)`、partial acceptance 与 ItemId/slot 规则不变。
- `Source/demo_map/CodeB/demo_mapCodeBP2.cpp`：P2 单 application/callback 顺序不变。
- `Source/demo_map/CodeB/demo_mapCodeBOutOfRaidProfile.h`：P11/P21 receipt/profile/record、P17/P31 schema均未新增持久化字段。
- 所有 `*Tests.cpp`、Actor、Code A 功能文件、Config 与 `.uproject`：均未修改；P 阶段未执行测试。
- PROJECT 文档本轮无需改动；P12 search state、P20/P21 deterministic history、P17 graph construction、P31 Registry、P8 terminal、P13 binding、P15 use、P5/P6 bridge均未改写。

## 2. 唯一输入与提交调用图

```text
UCodeBP3CellButton::NativeOnMouseButtonDown
  -> UCodeBP3InventoryWidget::HandleQuickTransfer
  -> BeginP4Drag / PopulateTransferContext
  -> PopulateP40BodySimpleStackQuickTransferProof (仅 Ctrl P12 ordinary source)
  -> 输入时冻结 CurrentP17Child 或 BaseQuickNoChildAtInput
  -> exact target 内 stable SlotIndex merge-first / empty-second
  -> PreviewInventoryTransfer
     -> ValidateTransferContext
     -> ValidateP40BodySimpleStackQuickTransferContext
     -> P4 PreviewDrop => Merge(0) 或 Move(0)
  -> CommitInventoryTransfer / P4 CommitDrop
  -> P3 CommitP4Operation
  -> P2 ApplicationService
  -> P1 ExecuteTransaction (single candidate)
  -> production BodyTarget commit callback
  -> CommitAcceptedMatchedRunBodyContainerTransfer
  -> one P11/P6 Owner durable replacement + one SaveRecord
  -> refresh exact P12 body projection + existing P2/P6/P7 projection
```

仍只有 `NativeOnMouseButtonDown` 消费 Ctrl + 左键并调用既有 `HandleQuickTransfer`；没有新增 Button、hotkey、pointer handler、Widget、Actor click、second resolver、Widget direct Move/quantity mutation 或 Code A 写入口。P40 proof 的构造被明确放在 `bQuickTransferIntent=true` 之后，因此 ordinary left-click、search 与 P12 normal Drag 不携带 P40 intent。

## 3. Canonical P12 ordinary source 与 provenance

- source 必须是 current production P12 Host 中 exact opened BasicCorpse 的 ordinary `BodyContainerTarget` stable slot；明确排除 `Body.Weapon`、`Body.ArmorRobe`、`Body.Accessory0`。
- Host 与 Store共同核对 OwnerId、RunInstanceId、BodyTargetId、DeathReceiptId、DefinitionId、body record revision、target-open generation、P6 composite revision、workspace route/focus pane、active session、receipt profile/version/digests、source ContainerId/SlotIndex/ItemId 与 reverse slot pointer。
- source 必须 `Revealed`、无 active action/search、`Quantity>0`、Catalog canonical `bStackable`、`MaxStack>1`、无 ChildContainerId、无 spatial semantic、非装备、非 draft；P1 compatibility继续以 canonical Definition/MaxStack 规则裁决。
- P20 complete graph 与 P21 fixed equipment拥有独立容器/proof branch；P40 未修改 corpse materialization、candidate/weight/digest、deterministic identity、r1/r2/r3 history、receipt或搜索状态机。

## 4. 生命周期、mode 冻结与零写入

- intent 创建、Preview、production callback 与 durable Store分层重验 Open/Revealed、Owner/Run、BodyTarget/death receipt、source address/root、body/P6 revision、Host/Actor、page/open generation、route/focus、active session与 Prepared/terminal/save gate。
- BodyTarget close/reopen、search begin/cancel、focus/Host/Actor失效、map/recovery/session变化、receipt/root/container mismatch、revision stale或 SaveRecord失败均拒绝；candidate未持久化，既有 BeforeSnapshot保持。
- `CurrentP17Child` 只在 input-time 存在 identity-valid current child 时建立，冻结 exact parent ItemId、child ContainerId、open generation与 P6 revision；UI/Store重建唯一 P17 parent→child、正式 SpatialRing/Backpack placement、dynamic capacity、一层无环与 ordinary child语义。child closed/switched/focus/generation/parent/capacity stale或满位时零写入，绝不回退 BaseQuick或另一 child。
- `BaseQuickNoChildAtInput` 仅在 input-time 不存在 valid child 时建立，child proof必须为空/0；之后新开或切换 child不改变 mode，target只可能是 active P6 `Basic6`。BaseQuick stale或满位时拒绝。
- 两种 mode均不扫描装备位、Hotbar、P5/P9/P11、WorldDrop或另一 BodyTarget，不自动 Equip/Bind/Use/Take All，不产生 player→corpse快捷回存。

## 5. Stable target、数量与单事务证明

- 在冻结的唯一 target container 内，resolver从 `SlotIndex=0` 升序先找 compatible underfull simple stack；找到后只提交一次 P1 `Merge(Quantity=0)`，不继续扫描。
- 完全不存在 compatible underfull stack时，才从 `SlotIndex=0` 升序取首个正式空 ordinary Cell并提交一次 P1 whole-root `Move`；不得 Swap、Replacement、Split、Quantity=N或 second candidate。
- partial Merge由 P1 唯一计算 accepted quantity：source保留相同 ItemId、P11 source container、SlotIndex、Revealed状态与 remainder；target保留既有 ItemId。full Merge才删除 source；empty Move保留 source ItemId、Definition、Quantity、Level、Quality、RandomSeed与 LegacyAffixDigest。
- durable Store从 `P24PriorComposite`重放同一个 TransactionId/Operation/source/target/`Quantity=0`，要求 resulting composite与 accepted snapshot全字段相等；无 staging container、第二 Repository transaction、new ItemId/Container/receipt/record/ordinal/Actor/child。
- accepted proof后才共同替换 P11 body snapshot与 P6 player snapshot，执行既有 P13 reconcile、freeze receipt与一次 `SaveRecord`；失败不提交 Candidate。成功只刷新 exact BodyTarget source与 resolved P6/P7 target投影，不 Sort/Compact、不重排无关 stable SlotIndex或写 WorldDrop。

## 6. 非回归与权威边界

- P12 ordinary normal Drag的 explicit Move/Merge/Swap仍走原有 branch；普通拖拽不会构造 P40 proof。
- P20/P19 complete graph、P21/P38/P39 corpse equipment、P29/P30/P34/P36/P37 WorldDrop Ctrl、P17/P19/P26—P39、P31 other-record isolation、P8 terminal/recovery、P13/P15与 P5/P6产品语义未改。
- right-click、double-click、Shift+1—9、Actor interaction、player-side Ctrl、Hidden/Searching、space/equipment/world/warehouse/Hotbar source均未获得 P40位置或数量写语义。
- Code B P1 Repository与 durable Store仍是唯一物品真值；progression manager只做 production lifecycle forwarding，不持有第二库存、预建 ItemId或 Code A/B双写。

## 7. P 阶段编译

### Editor

```text
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_mapEditor Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload
```

- 原生 exit code：`0`
- 关键结果：`Result: Succeeded`
- 计时：`25.530` 秒。

### Game

```text
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_map Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload
```

- 原生 exit code：`0`
- 关键结果：`Result: Succeeded`
- 计时：`29.840` 秒。

另完成 `git diff --check`，exit code `0`；Editor与 Game各只构建一次。

## 8. 明确留到 0.0.9B.F 的真实验证

本轮未执行以下项目：

- P12/r3 future corpse materialize/reveal；IronShard、SpiritDust或实际 canonical ordinary simple stack。
- Ctrl拾回至 current opened valid child，以及 input-time无 child时进入 BaseQuick；stable merge-first、empty-second、partial/full acceptance、MaxStack、source remainder与同 ItemId/slot保持。
- child/BaseQuick full；child closed/switched/focus/open-generation/parent/capacity/topology stale；BaseQuick mode输入后新开/切换 child与无 fallback。
- wrong/space/equipment/Hidden/Searching/draft/world/player source；BodyTarget/death receipt/Owner/Run/body/P6 revision stale；another BodyTarget。
- Prepared/terminal、Host/Actor invalid、SaveRecord failure、BeforeSnapshot rollback、projection refresh、selection/scroll/stable SlotIndex保持。
- P12 normal Move/Merge/Swap、P20/P21/P38/P39、P29/P30/P34/P36/P37 Ctrl branches、P31 other-record isolation、P8 terminal/recovery、P13/P15。
- 真实鼠标键盘、PIE、Standalone、截图、Smoke、Automation、回归、Cook、Package与最终验收。

READY_FOR_P41_PLANNING
