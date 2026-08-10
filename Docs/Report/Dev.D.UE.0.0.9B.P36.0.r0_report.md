# Dev.D.UE.0.0.9B.P36.0.r0 Report

## 1. 结论

- 任务：`Dev.D.UE.0.0.9B.P36.0.r0`。
- 结果：P35 `ChildStandardEquipment` WorldDrop 已接入既有共享 `Ctrl + 左键` QuickTransfer；输入时冻结 `CurrentP17Child` 或 `BaseQuickNoChildAtInput`，不自动装备，不发生 child → BaseQuick 静默回退。
- 唯一物品真值仍为 Code B P1 Repository 与活动 P6 durable Store；本任务没有新增 Repository、Widget inventory、Actor inventory、物品副本、预建 ItemId、WorldDropId 或 Code A 写入权威。
- 实现提交：`b2236e2 feat: add P36 frozen-target quick pickup`。
- P 阶段仅执行静态审查及 Prompt 明确要求的 Editor/Game 编译；未启动产品或执行真实测试。

## 2. 实际修改文件与职责

### 2.1 已修改

1. `Source/demo_map/CodeB/demo_mapCodeBP2.h`
   - 新增 transient `ECodeBQuickTransferTargetMode`：`Legacy`、`CurrentP17Child`、`BaseQuickNoChildAtInput`。
   - `FCodeBP2Command` 增加冻结 target mode 与 exact spatial parent ItemId；字段不持久化、不派生新物品身份。
2. `Source/demo_map/CodeB/demo_mapCodeBP4.h`
   - `FCodeBP4DragPayload` 携带同一 transient target mode 与 parent ItemId proof。
3. `Source/demo_map/CodeB/demo_mapCodeBP3.h`
   - 在既有 `CommitP4Operation` 参数末尾增加 target mode 与 parent identity，保留默认 `Legacy` 以维持 P29/P30/P34 和 normal Drag 调用语义。
4. `Source/demo_map/CodeB/demo_mapCodeBP3.cpp`
   - 将 payload proof 原样写入同一个 P2 command；未新增 P2 service 或事务调用。
5. `Source/demo_map/CodeB/demo_mapCodeBP4.cpp`
   - 既有 `CommitPreview` 将 P36 proof 透传至同一 `CommitP4Operation`。
6. `Source/demo_map/CodeB/demo_mapCodeBP3UI.h`
   - 更新 P35 provenance gate 的只读职责说明：P35 normal Drag 与 P36 frozen Ctrl pickup 共用该 gate。
7. `Source/demo_map/CodeB/demo_mapCodeBP3UI.cpp`
   - 在唯一 `HandleQuickTransfer` 中识别 exact P35 provenance。
   - 输入时一次性冻结 child 或 no-child BaseQuick 模式。
   - child mode 冻结 exact child ContainerId、parent ItemId、open generation 与已有 P6 revision。
   - 只扫描冻结目标容器的 stable `SlotIndex` 首个空普通格；P36 不进行 merge-first、装备槽扫描、Swap、Replacement 或 fallback。
   - Preview 重验 source、mode、parent/child projection、Owner/Run/revision 与 open lifecycle。
8. `Source/demo_map/CodeB/demo_mapCodeBOutOfRaidProfile.cpp`
   - 新增 `IsExactP36WorldDropChildStandardEquipmentQuickTransferDelta` durable proof。
   - 仅接受 `P35.AcceptedGroundDrop.ChildStandardEquipment`、canonical standard non-spatial root、一个 `Move(1)` 与冻结目标。
   - 从 P1/P6 快照重建唯一 P17 parent → child，验证动态 capacity、正式装备位置、首空 stable slot 和 exact candidate delta。
   - accepted proof 后沿既有 exact P31 cleanup 删除同一 record/空 world container；不改 `NextWorldDropOrdinal`。
9. `Source/demo_map/demo_mapV3ProgressionManager.cpp`
   - 在既有 WorldDrop commit callback 中拒绝畸形／陈旧 P36 mode proof；child 关闭、切换或 generation 失效时零写入且不回退 BaseQuick。

### 2.2 明确未修改

- 未修改 Code A WorldDrop Actor、地图放置、交互键、距离拾取或任何 Actor direct pickup。
- 未修改 P1 transaction implementation、P5 warehouse、P6 bridge/schema、P8 terminal receipt、P13 reconcile、P17 child graph 构造、P19 complete graph、P21 corpse、P26—P29 stack quantity、P30 complete graph、P32/P33/P34 provenance 与 P35 normal Drag 产品语义。
- 未修改 Build.cs、输入映射、Widget 类层级、存档 schema、migration、`NextWorldDropOrdinal`、Hotbar、战斗、搜索、地图、经济、网络或多人。
- 未新增测试代码、fixture、第二保存、第二 durable callback 或第二 P1/P2 调用。

## 3. 共享 Ctrl+左键活动调用图

```text
UCodeBP3CellButton::NativeOnMouseButtonDown
  └─ Ctrl + Left（唯一消费点，返回 Handled）
     └─ UCodeBP3InventoryWidget::HandleQuickTransfer
        ├─ BeginP4Drag / PopulateTransferContext
        ├─ exact WorldDrop presentation + P35 provenance gate
        ├─ 输入时冻结 target mode
        │  ├─ CurrentP17Child
        │  │  └─ child ContainerId + parent ItemId + open generation + P6 revision
        │  └─ BaseQuickNoChildAtInput
        │     └─ child identity 明确为空；后续 child 状态不改道
        ├─ 仅在冻结容器内按 stable SlotIndex 扫描首个空格
        └─ CommitInventoryTransfer
           └─ FCodeBP4InteractionController::CommitDrop / CommitPreview
              └─ FCodeBP3UIController::CommitP4Operation
                 └─ existing FCodeBP2ApplicationService::Apply
                    └─ one P1 ExecuteTransaction(Move)
                       └─ existing profile commit callback
                          └─ CommitAcceptedMatchedRunWorldDropPickup
                             ├─ P36 exact delta / mode / provenance proof
                             ├─ accepted-only exact P31 record cleanup
                             ├─ existing P13 ReconcileHotbarBindings
                             └─ one SaveRecord
```

没有新增 pointer handler、专用 P36 Button、Actor click、right-click Take、double-click、Widget direct Move 或 Code A inventory writer。普通左键、right-click、double-click、Drag、`Shift + 1—9`与 player-side Ctrl 仍沿原路径。

## 4. P35 canonical eligibility 与分支隔离

P36 UI gate 同时要求：

- source 是当前已打开 `WorldDropTarget` 的 revealed slot 0；
- presentation provenance 精确等于 `P35.AcceptedGroundDrop.ChildStandardEquipment`；
- source projection 为 `Quantity=1`、`MaxStack=1`、不可堆叠、无 `ChildContainerId` 的 Weapon/Armor/Accessory standard root；
- WorldDrop OwnerId、RunInstanceId、WorldDropId、Ordinal、RecordRevision、TargetOpenGeneration、MapRoute、derived container、root ItemId、scope 与 P6 revision 全部由既有 `ValidateWorldDropTransferContext` 重验。

durable gate 再从 P1/P6 snapshot 重新验证 canonical Catalog definition、non-spatial/no-child/no-graph closure、world slot 0 反向引用、record availability 与 exact P35 provenance。P29 simple stack、P30/P19 complete graph、P32/P33 provenance、unknown family、warehouse、corpse、Hotbar、another WorldDrop 或 player source 均不能进入 P36 durable branch。

P34 仍只接受：

- `P32.AcceptedGroundDrop.StandardEquipment`；
- `P33.AcceptedGroundDrop.BaseQuickStandardEquipment`；
- first-empty BaseQuick；
- `Legacy` target mode，且不携带 child parent proof。

P36 不扩大 P34 provenance，也不让 P35 record 进入 P29/P30/P34 proof。

## 5. 输入时 target mode 冻结与 stale 生命周期

### 5.1 `CurrentP17Child`

输入时若 Workspace 存在 identity-valid current P17 child：

- payload/command 固定 exact child `ContainerId`；
- 固定 unique spatial parent `ItemId`；
- 固定 `ActiveDestinationOpenGeneration`；
- 使用 payload 的 `ExpectedRevision`固定 P6/P1 revision；
- 保留 Owner/Run/WorldDrop exact record identity。

Preview 会在当前 projection 中确认 parent slot 唯一反向指向该 child。durable commit 使用 `IsP35ActiveP17ChildContainer`从 P1/P6 重建唯一 parent → child，确认 parent 仍位于正式 SpatialRing/Backpack equipment slot、definition 与 dynamic `ChildContainerCapacity`一致、child 一层无环且普通内容合法。

child close、switch、focus loss 所导致的 current context/generation 变化，会在 `ValidateTransferContext`或 ProgressionManager commit callback 被拒绝；parent mismatch、capacity/topology/revision stale 会在 durable proof 被拒绝。所有这些路径均不重算目标，也不回退 BaseQuick。

### 5.2 `BaseQuickNoChildAtInput`

输入时没有 identity-valid current child 时：

- mode 明确写为 `BaseQuickNoChildAtInput`；
- child ContainerId、parent ItemId 与 open generation 全部为空；
- destination 固定为 active P6 `BaseQuick`。

输入后即使 UI 新打开或切换到 child，target mode 与 target container 也不会改变。BaseQuick 满、target slot stale、Owner/Run/P6 revision 或 record stale 时零写入；不会搜索 child、装备位或其他容器。

## 6. child/BaseQuick 确定目标规则

- child mode：只在冻结的 exact child 中，从 `SlotIndex=0`递增扫描第一个存在、正式、可写、空普通格。
- BaseQuick mode：只在 `Session.Layout.BasicContainerId`对应的 active P6 BaseQuick 中采用同样的 stable `SlotIndex`首空规则。
- Preview 和 durable proof 都检查 target 非 equipment、slot index 有效且当前为空。
- durable proof 对所有更小 SlotIndex 要求已占用，证明目标确为首空，而不是最近空格、显示顺序、当前选择或 fixture 推断。
- P36 不执行 Merge、Split、Equip、Unequip、Swap、Replacement、Sort、Compact、auto bind、auto use 或 auto equip。
- P36 不扫描 Weapon、Armor、Accessory、SpatialRing、Backpack equipment slots；只接受普通储物格。

## 7. 单一事务、exact cleanup、回滚与 ordinal

accepted P36 路径只生成一个 `FCodeBP2Command`：

- `Intent = QuickTransfer`；
- `Operation = Move`；
- `Quantity = 1`；
- source = exact derived WorldDrop container slot 0；
- target = mode resolver 固定的 exact ordinary empty slot；
- ItemId、DefinitionId、Quantity、Level、Quality、RandomSeed、LegacyAffixDigest 与 provenance 保持。

P2 只调用一次 P1 `ExecuteTransaction`。`IsExactP36WorldDropChildStandardEquipmentQuickTransferDelta`逐字段构造预期 one-revision snapshot，并要求 Candidate 完全相等，因此不存在第二 Move、第二 Repository transaction 或隐式修正。

仅在该 accepted proof 已证明 root 离开 exact world container 后，既有 P31 cleanup 才：

- 删除 matching `WorldDropId`的一条 record；
- 删除同一 empty derived world container；
- 由既有 actor diff 刷新 matching Actor projection；
- 保持其他 Registry record graph 不变。

`NextWorldDropOrdinal`未读取为目标、未写入、未递增；没有创建新 WorldDrop、Container、Actor、child 或 ItemId。`ReconcileHotbarBindings`仍只对 accepted snapshot 做既有 P13 reconcile；没有自动绑定、使用或装备。

任一 gate、P1 command、candidate delta、other-record isolation、Registry validation、P13 reconcile 或 `SaveRecord`失败时，现有 `PersistProfileSnapshotAfterAcceptedP1`以 `BeforeSnapshot`恢复 UI Repository；durable Store 只保存通过全部检查的 Candidate，因此失败路径不会删除 record/Actor，不会留下 phantom empty slot 或物品副本。

## 8. 非回归与权威边界审查

- P29 simple stack：仍使用 `Legacy` mode；保留 current child 优先、既有 BaseQuick fallback、merge-first/empty-second 与 player → world `Merge(0)`语义。
- P30/P19 complete graph：仍清空 active child proof，只到 first-empty BaseQuick；P36 helper要求无 child graph，不能进入 P30。
- P34：P32/P33 provenance 仍只到 first-empty BaseQuick，不携带 P36 parent identity。
- P35 normal Drag：`bQuickTransferIntent=false`时仍可明确拖到 BaseQuick、当前 child 空格或空兼容 equipment slot；P36 没有改变该 branch。
- P31：other-record isolation、canonical serialization order、exact identity cleanup 与 accepted-create-only ordinal 均保持。
- P8：未修改 `BuildP14PlayerOnlySession`／等价 full Registry filtering；remaining ground roots 与 derived world containers 仍不进入 player-only final graph。
- P13：只复用既有 accepted commit reconcile；没有新增 bind/use/equip。
- P17：没有修改 child graph、动态容量、装备位置或一层无环规则。
- Code A：仍只做 floor/Actor/交互边缘投影与生命周期转发；没有持有 Item、Container、Quantity、Run、Player、Loot 或 WorldDrop 持久化权威。
- UI：未改变 scroll offset、主动排序或无关 selection；成功后只按既有 projection refresh 更新 exact WorldDrop、目标容器和 matching Actor。

## 9. 静态审查与编译证据

### 9.1 静态审查

- `git diff --check`：通过，无 whitespace error。
- 修改范围：9 个既有源文件；没有新增测试、输入、Actor、Widget、schema 或 Build.cs 文件。
- 写入链检索确认：P36 UI 仍经 `CommitInventoryTransfer → CommitP4Operation → Service->Apply → CommitAcceptedMatchedRunWorldDropPickup → SaveRecord`；没有新增 direct `ExecuteTransaction`或 `SaveRecord`调用点。
- provenance/mode 检索确认：P36 durable branch只由 `P35.AcceptedGroundDrop.ChildStandardEquipment`与两个显式 frozen mode 触发。

### 9.2 Editor 编译

命令：

```text
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_mapEditor Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload
```

- 原生 exit code：`0`。
- 关键结果：`Result: Succeeded`。
- UBT 总执行时间：`24.43 seconds`。
- 输出：`C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe`。

### 9.3 Game 编译

命令：

```text
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_map Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload
```

- 原生 exit code：`0`。
- 关键结果：`Result: Succeeded`。
- UBT 总执行时间：`28.49 seconds`。
- 输出：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Binaries\Win64\demo_map.exe`。

## 10. 明确延后至 0.0.9B.F 的真实验证

本任务未运行下列测试；全部留待 F 阶段：

1. 建立多个由 P35 source 生成的 Weapon、Armor/ArmorRobe、Accessory standard WorldDrop records。
2. 有 identity-valid current child 时，真实 `Ctrl + 左键`进入该 child 的首个 stable 空普通格。
3. 输入时无 valid child 时，真实 `Ctrl + 左键`进入 BaseQuick 首空普通格。
4. child close、switch、focus loss、open generation stale、parent mismatch、capacity/topology invalid、child no-empty 时全部零写入且不回退 BaseQuick。
5. BaseQuick 满、target stale、record/Owner/Run/P6 revision stale、terminal/Prepared/Host invalid 与 SaveRecord failure。
6. 输入为 BaseQuick mode 后新打开／切换 child，确认目标仍为 BaseQuick。
7. P34 Ctrl、P35 normal Drag 与 explicit equipment pickup 保持。
8. stack、space parent/complete graph、P32/P33 record、warehouse、corpse、Hotbar、another WorldDrop 与 player-side Ctrl 拒绝。
9. exact Actor projection cleanup、P31 other-record isolation、P8 full-registry player-only exclusion与 recovery。
10. 真实鼠标键盘、PIE、Standalone、截图、Smoke、Automation、回归、Cook 与 Package。

READY_FOR_P37_PLANNING
