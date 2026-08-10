# Dev.D.UE.0.0.9B.P37.0.r0 Report

## 1. 结论

- 任务：`Dev.D.UE.0.0.9B.P37.0.r0`。
- 结果：P32/P33 standard-equipment WorldDrop 的 `Ctrl + 左键`快捷拾回已接入 P36 既有 frozen-target model。输入时若存在 identity-valid current P17 child，只移入该 exact child；输入时无 valid child，才移入 BaseQuick。任何随后 close/switch/focus/revision/capacity stale 均拒绝，不静默回退。
- P34 的 no-child 产品结果仍为 BaseQuick 首空普通格；P32/P33 normal Drag、P35 normal Drag、P36 P35 provenance Ctrl、P29 stack 与 P30 complete graph 均保持原有分支。
- 唯一可变物品真值仍为 Code B P1 Repository 与活动 P6 durable Store。未新增第二 Repository、Widget/Actor inventory、预建 ItemId、WorldDropId、第二保存或 Code A writer。
- 实现提交：`12243e4 feat: align P37 standard quick pickup targets`。
- P 阶段只完成实现、静态审查以及 Prompt 明确要求的 Editor/Game 编译；未启动产品或执行真实测试。

## 2. 实际修改／未修改文件与职责

### 2.1 已修改

1. `Source/demo_map/CodeB/demo_mapCodeBP2.h`
   - 更新既有 transient `ECodeBQuickTransferTargetMode`及 command proof 注释，明确 `CurrentP17Child`／`BaseQuickNoChildAtInput`现在由 P36/P37 共用；枚举值、数据布局与 P1/P2 行为未新增。
2. `Source/demo_map/CodeB/demo_mapCodeBP4.h`
   - 更新既有 payload proof 注释，明确 target mode 与 canonical parent ItemId 同时服务 P36/P37；字段仍只存在于一次 pointer gesture。
3. `Source/demo_map/CodeB/demo_mapCodeBP4.cpp`
   - 将既有 standard whole-root QuickTransfer 的局部命名从 P34 专名改为共享名称；`Move(1)`预览、P1 Move 和其它 drop kind 行为不变。
4. `Source/demo_map/CodeB/demo_mapCodeBP3UI.h`
   - 明确 `IsP34StandardEquipmentWorldDropSource`是 P37 对既有 P32/P33 durable provenance family 的只读 gate；未增加新 source resolver。
5. `Source/demo_map/CodeB/demo_mapCodeBP3UI.cpp`
   - 在唯一 `HandleQuickTransfer`内令 P32/P33 与 P35 standard root 共用同一输入时 target-mode freeze。
   - P32/P33 source 现在可冻结 exact current child ContainerId、spatial parent ItemId、open generation 与已有 Owner/Run/P6 revision；没有 valid child 时显式写入 `BaseQuickNoChildAtInput`并清空 child identity。
   - destination 仅为 frozen exact child 或 BaseQuick，按真实 stable `SlotIndex`递增寻找首个空普通格；不做 Merge、auto equip、Swap 或 fallback。
   - Preview 与 `ValidateTransferContext`共用 P36/P37 mode proof，继续重验 current child、unique parent、Owner/Run/scope/revision 与 open lifecycle。
6. `Source/demo_map/CodeB/demo_mapCodeBOutOfRaidProfile.h`
   - 更新既有唯一 opened-WorldDrop durable writer 的职责注释，纳入 P36/P37；函数签名未改变。
7. `Source/demo_map/CodeB/demo_mapCodeBOutOfRaidProfile.cpp`
   - 将旧 P34 BaseQuick-only exact-delta proof 收口进 P36/P37 共享 frozen-target durable proof `IsExactP36P37WorldDropStandardEquipmentQuickTransferDelta`。
   - 共享 proof 只接受三种现行稳定 provenance：
     - `P32.AcceptedGroundDrop.StandardEquipment`
     - `P33.AcceptedGroundDrop.BaseQuickStandardEquipment`
     - `P35.AcceptedGroundDrop.ChildStandardEquipment`
   - 从 P1/P6 snapshot 重建 target、P17 parent → child、动态容量、source slot 0 与 first-empty stable slot，并要求 Candidate 等于一个 one-revision whole-root Move delta。
   - P32/P33 accepted quick path 标记为 P37，以便沿同一 accepted-only exact P31 cleanup 删除 matching record 与空 world container；P35 继续保持 P36 标记。

### 2.2 明确未修改

- 未修改 `demo_mapCodeBP3.cpp`中的 P3 → P2 单命令提交、`PersistProfileSnapshotAfterAcceptedP1` BeforeSnapshot 回滚与 selection restore。
- 未修改 Code A WorldDrop Actor、地图放置、Actor interaction、距离拾取、交互键领取或 Actor direct pickup。
- 未修改 P1 transaction implementation、P5 warehouse、P6 schema/bridge、P8 finalization、P13 reconcile、P17 graph construction、P19 complete graph、P21 corpse、P26—P30 stack/graph rules、P31 Registry schema/ordering、P32/P33/P35 normal Drag 或 P36 P35 Ctrl 产品语义。
- 未修改 input mapping、Widget 类层级、Build.cs、migration、`NextWorldDropOrdinal`、Hotbar、搜索、敌人、地图、战斗、经济、网络或多人。
- 未新增 test、fixture、second pointer handler、second P2 service、second Repository transaction、second SaveRecord 或第二 durable callback。

## 3. P4/P23/P29/P30/P34/P36 → P37 活动调用图

```text
UCodeBP3CellButton::NativeOnMouseButtonDown
  └─ Ctrl + Left（唯一消费点；Handled）
     └─ UCodeBP3InventoryWidget::HandleQuickTransfer
        ├─ existing BeginP4Drag / PopulateTransferContext
        ├─ current opened exact WorldDrop presentation
        ├─ canonical source-family split
        │  ├─ P29 simple stack → existing Legacy resolver
        │  ├─ P30 complete graph → existing BaseQuick-only resolver
        │  ├─ P36 P35 provenance → existing frozen-target resolver
        │  └─ P37 P32/P33 provenance → same frozen-target resolver
        ├─ input-time mode freeze
        │  ├─ CurrentP17Child
        │  │  └─ exact child + parent ItemId + open generation + P6 revision
        │  └─ BaseQuickNoChildAtInput
        │     └─ child identity explicitly empty; later child cannot redirect
        ├─ frozen container stable SlotIndex first-empty scan
        └─ CommitInventoryTransfer
           └─ FCodeBP4InteractionController::CommitPreview
              └─ FCodeBP3UIController::CommitP4Operation
                 └─ existing FCodeBP2ApplicationService::Apply
                    └─ one P1 whole-root Move
                       └─ existing profile commit callback
                          └─ CommitAcceptedMatchedRunWorldDropPickup
                             ├─ shared P36/P37 exact delta proof
                             ├─ accepted-only exact P31 cleanup
                             ├─ existing P13 reconcile
                             └─ one SaveRecord
```

没有新增 pointer/UI/Actor 写入路径。ordinary left click 仍只选择；right-click 仍只读详情；double-click、Drag、Tab、I、Esc、Close、Cancel、scroll、空白区、`Shift + 1—9`、player-side Ctrl 与 Actor interaction 没有获得 P37 位置写入语义。

## 4. P32/P33 canonical eligibility 与 branch rejection

P37 UI gate 同时要求：

- source 是当前主动打开的 exact `WorldDropTarget` presentation，且 source ContainerId 与 target presentation 一致；
- presentation provenance 精确等于 `P32.AcceptedGroundDrop.StandardEquipment`或 `P33.AcceptedGroundDrop.BaseQuickStandardEquipment`；
- root projection 为 revealed Weapon/Armor/Accessory standard root：`Quantity=1`、`MaxStack=1`、不可堆叠、无 `ChildContainerId`，ItemType 与正式 EquipSlot 匹配；
- 既有 `ValidateWorldDropTransferContext`重验 OwnerId、RunInstanceId、WorldDropId、Ordinal、RecordRevision、TargetOpenGeneration、MapRoute、derived container、root ItemId、scope、focus/workspace 与 P6 revision；
- exact world container 必须为容量 1 的 `WorldDropTarget`，slot 0 反向指向同一 root ItemId。

durable proof 再从 Catalog、P1、P6、P17、P31 snapshot 证明 source 是 non-spatial/no-child/no-graph-closure canonical standard root，record 仍 `Available`且 provenance 未变。P29 simple stack、P30/P19 complete graph、P35 provenance、unknown family、partial draft、space parent/child、warehouse、ordinary container、corpse、Hotbar、another WorldDrop、hidden source 或 player source 均不能进入 P37 durable branch。

P35 provenance 仍由 P36 分支标记；P37 没有将 P32/P33/P35 normal Drag 混入 QuickTransfer，也没有根据 UI 文本、图标、Cell class、ordinal 顺序或 current selection 推断 provenance。

## 5. 输入时 target mode、identity 与 stale 生命周期

### 5.1 `CurrentP17Child`

输入时 Workspace 已存在 identity-valid current P17 child，payload/command 一次性冻结：

- OwnerId、RunInstanceId；
- source P6/P1 `ExpectedRevision`；
- exact WorldDropId、Ordinal、RecordRevision、derived world container、root ItemId、MapRoute、TargetOpenGeneration；
- exact child ContainerId、active child open generation；
- 与该 child 唯一对应的 spatial parent ItemId。

Preview 在当前 projection 中要求 parent ItemId/ChildContainerId 精确匹配且只出现一次。commit 前 `ValidateTransferContext`要求 Workspace 仍指向同一 child 与同一 generation；durable proof 使用既有 `IsP35ActiveP17ChildContainer`从 P1/P6 重新证明 parent 位于正式 SpatialRing/Backpack equipment placement、definition 与 dynamic `ChildContainerCapacity`一致、一层无环且 child 是普通存储容器。

child close、switch、focus loss、generation mismatch、parent mismatch、capacity/topology invalid、source/target revision stale、record close/reopen、Actor EndPlay、map reload、recovery、Prepared/terminal 或 Host invalid 都使该 intent 零写入；代码没有将 mode 改为 BaseQuick，也不扫描另一 child。

### 5.2 `BaseQuickNoChildAtInput`

输入时不存在 identity-valid current child：

- mode 明确冻结为 `BaseQuickNoChildAtInput`；
- child ContainerId、parent ItemId 与 open generation 明确清空；
- target family 固定为 active P6 `Session.Layout.BasicContainerId`。

输入后新打开、切换或投影出 child 不会修改该 mode。BaseQuick 满、target stale、Owner/Run/P6 revision mismatch、record stale、SaveRecord failure 或任一 lifecycle gate 失败均零写入；不会搜索 child、equipment、Hotbar、warehouse、corpse 或其他 WorldDrop。

## 6. 唯一 target、stable SlotIndex 与无隐式行为

- Current-child mode 只在冻结 exact child 内从 `SlotIndex=0`递增寻找第一个存在、正式、可写、空普通格。
- BaseQuick-no-child mode 只在 active P6 BaseQuick 内采用同一 stable SlotIndex 首空规则。
- Preview 要求 target container/role 与 frozen mode 精确匹配、target 未占用且 operation 为 `Move(1)`。
- durable proof 要求 target 非 equipment、slot index 有效且为空，并逐个确认所有更小 SlotIndex 已占用，因此不是显示顺序、最近空格、focus、selection 或 fixture 推断。
- child full 或 BaseQuick full 时扫描结束并拒绝；不会切换 container 或执行 fallback。
- P37 不调用 Equip、Unequip、Merge、Split、Quantity=0、WorldPickupDraft、PlayerSplitDraft、Swap、Replacement、Sort、Compact、auto bind、auto use 或 auto equip，也不扫描 Weapon/Armor/Accessory/SpatialRing/Backpack equipment slots。
- P37 不主动 Sort/Compact，不改变无关 SlotIndex、scroll offset、selection 或空间区域布局。

## 7. 单一 whole-root 事务、exact cleanup、Actor 与回滚

accepted P37 command 固定为：

- `Intent = QuickTransfer`；
- `Operation = Move`；
- `Quantity = 1`；
- source = exact derived world container slot 0；
- target = frozen mode resolver 得到的 exact empty ordinary slot；
- 同一个 ItemId 从 source 直接移动到 target。

P2 只调用一次 P1 `ExecuteTransaction`。共享 `IsExactP36P37WorldDropStandardEquipmentQuickTransferDelta`基于 Prior 构造 one-revision Expected snapshot，并要求 Candidate 完全相等。DefinitionId、ItemId、Quantity、Level、Quality、RandomSeed、LegacyAffixDigest、provenance 及 no-child/non-spatial 资格保持；唯一变化是 parent container/slot。没有生成新 ItemId、ContainerId、record、ordinal、Actor、child 或第二 revision。

只有 accepted delta 已证明 root 离开 matching derived world container 后，既有 P31 cleanup 才：

- 删除 exact `WorldDropId`对应的一条 record；
- 删除该 exact empty world container；
- 通过既有 Actor diff 刷新 matching Actor projection；
- 对每个 other Registry record 调用 graph-isolation proof，拒绝任何跨 record 改动。

`NextWorldDropOrdinal`在 pickup 路径不写入、不递增；没有创建 WorldDrop。P13 只复用既有 `ReconcileHotbarBindings`，不自动 Bind、Use 或 Equip。Candidate 在所有检查完成后只调用一次 `SaveRecord`。

任一 source/mode/target/candidate/P1/P13/Registry/record cleanup/Actor projection precondition/SaveRecord 失败时，durable Store 未替换；P3 既有 `PersistProfileSnapshotAfterAcceptedP1`使用 `BeforeSnapshot`恢复 UI Repository 并刷新 projection。失败不会预删 record/Actor，不留下 phantom empty slot、phantom pickup 或 transient item copy。

## 8. 非回归与权威边界

- P29 simple stack：仍使用 `Legacy`，保留 valid child 优先、既有 BaseQuick fallback、merge-first/empty-second 与 player → world `Merge(Quantity=0)`语义；standard root 不会进入 P29。
- P30/P19 complete graph：仍在输入路径清空 active-child proof，只移入 first-empty BaseQuick；shared standard proof要求 no-child/non-spatial，不能接纳完整图。
- P32/P33 normal Drag：仍可明确拖到空 BaseQuick 或空 Definition-compatible equipment slot；P37 只改变该 provenance family 的 automatic Ctrl target。
- P34 no-child outcome：输入时没有 valid child 时仍是 first-empty BaseQuick，且使用显式 `BaseQuickNoChildAtInput`证明，不再由 Legacy 延迟猜测。
- P35 normal Drag：仍可明确拖到 BaseQuick、current child 空格或兼容 equipment slot；未修改。
- P36 P35 Ctrl：仍使用同一 frozen-target proof并保留 `P35.AcceptedGroundDrop.ChildStandardEquipment`隔离；未改变其 accepted result。
- P31：canonical order 仍只用于 serialization/readonly projection；cleanup 只按 exact identity，migration、record isolation 与 accepted-create-only ordinal 未改。
- P8：未修改 full Registry player-only filtering；remaining P14/P19/P32/P33/P35 ground roots、derived world containers 与 graph closure 仍被排除。
- P5/P6/P7/P13/P15/P17/P21：未改变 warehouse、session bridge、workspace projection、binding/use、child graph 或 corpse authority。
- Code A：仍只做 floor/Actor/interaction edge projection 与 lifecycle forwarding；没有 Item、Container、Quantity、Run、Player、Loot、search、terminal 或 WorldDrop durable authority。

## 9. 静态审查与编译证据

### 9.1 静态审查

- `git diff --check`：通过，无 whitespace error。
- 修改范围：7 个既有 Code B 源文件；没有新增 input、Widget、Actor、schema、Build.cs 或测试文件。
- 调用链检索确认：唯一写入仍为 `HandleQuickTransfer → CommitInventoryTransfer → CommitP4Operation → Service->Apply → CommitAcceptedMatchedRunWorldDropPickup → SaveRecord`。
- provenance 检索确认 P37 只接受实际稳定值 `P32.AcceptedGroundDrop.StandardEquipment`和 `P33.AcceptedGroundDrop.BaseQuickStandardEquipment`；P35 value 继续归 P36。
- mode 检索确认 `CurrentP17Child`与 `BaseQuickNoChildAtInput`只在共享 payload/command 中瞬时携带；没有持久化 target mode 或第二物品真值。
- cleanup 检索确认 P37 accepted root 与 P36/P30 一样只在 accepted proof 后令 `bRetainWorldRoot=false`；other-record isolation、one SaveRecord 与 BeforeSnapshot rollback 保持。

### 9.2 Editor 编译

命令：

```text
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_mapEditor Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload
```

- 原生 exit code：`0`。
- 关键结果：`Result: Succeeded`。
- UBT 总执行时间：`24.63 seconds`。
- 输出：`C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe`。

### 9.3 Game 编译

命令：

```text
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_map Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload
```

- 原生 exit code：`0`。
- 关键结果：`Result: Succeeded`。
- UBT 总执行时间：`28.83 seconds`。
- 输出：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Binaries\Win64\demo_map.exe`。

## 10. 延后至 0.0.9B.F 的真实验证

本任务未运行以下真实验证；全部留待 F 阶段：

1. 分别由 P32 equipped 与 P33 BaseQuick source 建立多个 Weapon、Armor/ArmorRobe、Accessory standard WorldDrop records。
2. valid current child 下真实 `Ctrl + 左键`进入该 exact child 的首个 stable 空普通格。
3. 输入时无 valid child 下真实 `Ctrl + 左键`进入 BaseQuick 首空普通格。
4. child close、switch、focus loss、generation stale、parent mismatch、capacity/topology invalid、child full 时全部零写入且不回退。
5. BaseQuick full、target stale、record/Owner/Run/P6 revision stale、Prepared/terminal、Host invalid 与 SaveRecord failure。
6. 输入冻结 BaseQuick mode 后新打开／切换 child，确认仍只尝试 BaseQuick。
7. P34 no-child outcome、P36 P35 Ctrl 与 P32/P33/P35 normal Drag 的保持。
8. stack、space/complete graph、corpse、warehouse、Hotbar、another WorldDrop、unknown provenance 与 player-side Ctrl 拒绝。
9. matching Actor projection cleanup、P31 other-record isolation、P8 full-registry player-only exclusion与 recovery。
10. ordinary click、right-click、double-click、Drag、`Shift + 1—9`、scroll/selection 保持。
11. 真实鼠标键盘、PIE、Standalone、截图、Smoke、Automation、回归、Cook 与 Package。

READY_FOR_P38_PLANNING
