# Dev.D.UE.0.0.9B.P35.0.r0

## 任务身份

- 项目：`Dev.D.UE.0.0.9B`；继续使用同一活动工程，不新建项目。
- 阶段：主线 P35——已打开 P17 空间子容器内 standard non-spatial equipment 的正常地面拖放与明确拾回。
- 任务编号：`Dev.D.UE.0.0.9B.P35.0.r0`。
- 前置：已接受 `0.0.9B.P1—P34` 与 `0.0.9BFix.P1—P4`。Fix 仅用于已确认、已验收功能的缺陷修复；P35 是新增主线功能，不是 Fix。
- 执行文件：`Dev.D.UE.0.0.9B.P35.0.r0_prompt.md`。
- 报告文件：`Dev.D.UE.0.0.9B.P35.0.r0_report.md`。
- 活动工程根：`C:\\AIDev\\shanmen-ue\\Dev.D.UE.0.0.9B`。
- 活动工程：`C:\\AIDev\\shanmen-ue\\Dev.D.UE.0.0.9B\\demo_map.uproject`。
- 任务性质：P 阶段只做实现、静态审查与代码编译。不得启动真实运行验证，也不得自动开始 P36、任意 Fix 或 F。

## 上半部分：只读项目裁决、现状与边界

### 1. 当前唯一有效基线

唯一有效依据是当前 `0.0.9B`、活动工程及已接受任务链。所有 `0.2`、`V2`、`V3`、`I`、`IPF`、历史页面壳、旧 CTA、旧库存和旧物品规则均已过时；不得读取、采用、恢复或以其决定实现、验收或范围。

Code B P1 Repository 与既有 durable Store 是唯一可变物品真值。每件物品始终只有一个真实 `ItemId`、一个真实父位置和一条权威事务链。Widget、Cell、Presenter、DragOperation、Workspace Context、WorldDropTarget、WorldDrop Actor、地图放置适配层和 Code A 都只能持有只读投影、选择或瞬时意图；不得持有第二库存、可写数量副本、预建 `ItemId`、平行世界背包、Actor-first 写入或 A/B 双写。

P17 已建立真实空间道具的单层 `ChildContainer` 图及其 P7 工作台投影；空间 parent 与 child graph 仍必须完整保持。P19/P30 只处理空间 parent 的完整图地面路径和受限快捷拾回。P26—P29 已处理 simple stack 在 `BaseQuick` 或当前已打开合法 P17 child 与地面之间的正常/数量/快捷路径。P31 已将地面扩展为多个彼此独立、单根的 exact `WorldDrop` record。

P32 允许已装备 standard non-spatial Weapon、Armor/ArmorRobe 和普通 Accessory root 正常落地及明确拾回；P33 将同一闭环扩展到 `BaseQuick` 中未装备的同类 root；P34 只为 P32/P33 provenance 的已打开 record 提供 `Ctrl + 左键` 到首个空 `BaseQuick` 的受限快捷拾回。P34 明确未涵盖 P17 child 内的 standard root。

### 2. P35 产品裁决

P35 补齐当前空间背包页面中仍缺失的标准装备闭环：当玩家已主动打开一个身份有效的 P17 空间 parent 的真实 child container 时，直接位于该 child 普通储物格内的一个标准、非空间、无 child、不可堆叠的 Weapon、Armor/ArmorRobe 或普通 Accessory root，可通过既有 normal Drag 拖到 `GroundDropZone`，形成一个新的 single-root P31 record。玩家随后必须主动打开该 exact record，才可通过 normal Drag 将同一 root 拖到一个用户明确指定的合法目标。

| 明确动作 | 唯一允许来源 | 唯一允许目标 | 权威结果 |
| --- | --- | --- | --- |
| 主动落地 | 当前已打开、身份有效的 P17 child 内的 direct standard root | 既有 `GroundDropZone` | 一个 P1 whole-root relocation；同一 P31 Registry candidate 新建一个 exact `WorldDrop` record。 |
| 明确拾回到普通格 | 当前已打开、身份有效的 exact P35 `WorldDrop` root | 用户明确 Drop 的空 `BaseQuick` 格，或当前已打开、身份有效 P17 child 的空普通格 | 一个 P1 whole-root `Move`；accepted 后只删除该 exact record、其空 world container 与 Actor。 |
| 明确拾回到装备位 | 当前已打开、身份有效的 exact P35 `WorldDrop` root | 用户明确 Drop 的空、正式、Definition-compatible P6 equipment slot | 一个 P1 formal Equip/root relocation；不搜索候选、不自动装备。 |

P35 的 child 仅指 P17 已接受空间 parent 所拥有、当前已打开且仍属于 active P6 的真实一层普通储物容器。它不是一个新的 world inventory，也不是对 nested container、space parent、space child graph 的拆分权限。source root 本身必须没有 `ChildContainerId`、没有 spatial semantic，并直接位于该 exact child container 的正式普通格中。

这是 normal Drag/Drop 的来源与明确目标扩展，不是新输入、快捷丢弃、自动转移、自动装备、Actor direct pickup 或第二物品真值。P35 不修改 P34 的 `Ctrl + 左键` source gate；P35 创建的 record 将使用可审计的 accepted provenance，为后续若需的快捷分支保留独立规划空间。

### 3. 严格范围与持续排除

P35 source 只接受同时满足全部条件的 root：

1. 当前 active `OwnerId + RunInstanceId` 的 P6 图中，用户已主动打开、仍 identity-valid 的 exact P17 space parent 所拥有的 child container；source 直接位于该 child 的一个真实、可写、普通储物 `SlotIndex`；
2. Catalog/Repository 共同证明 source 是已 Revealed 的 standard `Weapon`、`Armor/ArmorRobe` 或普通 `Accessory`，并且 source placement 不是 equipment slot、`BaseQuick`、Hotbar、P5/P9/P11、尸体、另一 `WorldDrop` 或 UI-only object；
3. root 为 non-spatial、无 `ChildContainerId`、无 graph closure、不可堆叠、`Quantity = 1`、`MaxStack = 1`；Definition、ItemId、Owner/Run、child parent relation、ContainerId、SlotIndex、P6 revision 与当前 child-open identity 都可在 Preview 与 durable Commit 前从 P1/P6 真值重新验证；
4. 当前空间 parent、其 child 容器、容量/slot semantic 与 source 都通过 P17 一层、无环、唯一 parent 和 active-session gate；不得从显示名、图标、Cell class、Actor tag、fixture、当前选择或容器显示顺序判断资格。

以下对象或动作持续不属于 P35：

1. P19/P30 的 spatial parent、完整 graph、任何带 `ChildContainerId` 的 root、nested container、child graph closure、空间道具本体以及其中的任何 child-container operation；
2. P26—P29 的 simple stack、Merge、Split、部分数量、`WorldPickupDraft`、`PlayerSplitDraft`、normal stack quick transfer；
3. P32/P33 的 equipped / `BaseQuick` standard root 正常拖放，以及 P34 的 P32/P33 standard-record `Ctrl + 左键`快捷拾回；这些路径保持且不被 P35 重定向；
4. player-side `Ctrl + 左键`、quick-drop、quick-pickup、auto equip、auto target、Swap、Replacement、Sort、Compact、right-click Take、double-click、Take All、Actor direct pickup、交互键领取、自动绑定、自动使用或跨 record transfer；
5. P5 warehouse、P9 normal container、P11 corpse、尸体装备、P21 corpse source、未打开 child、another WorldDrop、Hotbar、搜索、敌人、地图、战斗、死亡、撤离、经济、网络或多人；
6. 实机运行、PIE、Standalone、真实鼠标键盘输入、截图、Smoke、Automation、回归、Cook、Package 或最终验收。

## 下半部分：授权执行内容

### 4. 单一授权目标

在不建立第二物品真值、不改变 P17 space graph、不改变 P31 multi-record Registry，且不新增自动行为的前提下，将 active P17 child 内合格的 standard non-spatial equipment root 接入既有 `GroundDropZone` / exact `WorldDropTarget` 工作流：

1. 合格 child root normal Drag → `GroundDropZone` 仅形成一个 P1 whole-root relocation、一个 P31/P6 Owner candidate、一次 `SaveRecord` 与一个 accepted-only exact record；
2. exact opened P35 record normal Drag → 用户明确的空 `BaseQuick`、当前 exact active child 的空普通格，或用户明确的空兼容 equipment slot；
3. 每条 accepted path 只影响同一 `ItemId`、其 exact source/target、matching `WorldDropId`/Ordinal/record 与必要 Actor projection；
4. 任一 child/open/session/identity/capacity/target/save 检查失败时，完整零写入或恢复 `BeforeSnapshot`。

### 5. 实现要求

#### 5.1 先完成活动调用链与资格审计

改动前必须审阅并在 Report 中列出：

1. P17 的 current child-open identity、space parent/child ownership、一层无环规则、dynamic capacity、stable `SlotIndex`、open/close/recovery lifecycle 与普通 child Cell 投影；
2. P26 的 current active child normal Drag / GroundDrop / explicit world pickup 路径，明确其 simple-stack eligibility 为何不能静默泛化为 P35 standard root；
3. P32/P33 的 standard equipment canonical classification、GroundDrop source normalization、explicit BaseQuick/equipment pickup、P31 record creation/cleanup 与 P34 provenance/QuickTransfer 排除；
4. P1/P2/P3/P7 的 whole-root `Move`、formal Equip/root relocation、source/target capacity、single candidate、BeforeSnapshot rollback、P13 reconcile 与 P6 revision 调用图；
5. P14/P31 的 floor placement、exact `WorldDropId`/Ordinal/record identity、derived world container、Actor diff、other-record isolation、recovery、terminal 和 P8 full-registry player-only filtering；
6. 任何 Widget direct Move、Actor direct pickup、child UI cache write、按 parent 显示名或 current selection 猜测 source、预建 `ItemId`/`WorldDropId`、先删 record/Actor 再 Move、第二 P1 transaction、第二保存或 Code A inventory 写入旁路。它们不得成为 P35 写入路径。

#### 5.2 Child standard-root gate 与 normal Drag

1. source Cell 必须继续使用 P4/P23 的共享 `InventoryDragOperation`/稳定 payload；`GroundDropZone::NativeOnDrop` 或活动等价入口仍是 player → world 位置写入的唯一入口。不得新增 child 专用 Button、hotkey、pointer handler、right-click、double-click、Actor click 或 Widget direct write。
2. Preview 和 Commit 前，Store 必须重新验证 exact OwnerId、RunInstanceId、P6 revision、P17 parent ItemId、parent placement、child `ContainerId`、child-open generation、source ItemId/DefinitionId/SlotIndex、source role、quantity、stackability、`ChildContainerId`、spatial semantic、active session、terminal/Prepared gate 与 floor placement。
3. 只有直接位于该 valid child 普通格、并满足 P35 第 3 节全部 standard non-spatial 条件的 root 才可继续。source 是 `BaseQuick`、装备位、Hotbar、空间 parent、带 child root、stack、未揭示对象、错误 Owner/Run、stale payload、已终局 session、parent/child mismatch 或任意 graph closure 时，都必须零写入。
4. 由 Code A 提供的 floor-placement adapter 只提供已验证、不可变的地图落点；它不得创建 ItemId、WorldDropId、record、数量、container 或 P6 graph。P35 成功前不得从 child 移除 source、创建 Actor 或预占 ordinal。
5. 普通 left-click 继续只选择；right-click 继续只读详情；`Ctrl + 左键`继续既有 P29/P30/P34 route 而不获得 P35 写入语义；`Shift + 1—9`继续仅走 P13 Bind。双击、Tab、I、Esc、Close、Cancel、scroll、失焦、空白区和无 payload Drop 均不得写入 P35。

#### 5.3 Child root → 新的 exact WorldDrop record

1. 接受后只建立一个 Owner/Run scoped P1 candidate，使用既有 whole-root `Move` 将同一个 child root ItemId 直接移入新 record 的 derived world container slot 0。不得先移到临时 `BaseQuick`、先 Unequip、先复制、或建立两次位置写入。
2. root 的 `DefinitionId`、`ItemId`、`Quantity=1`、Level、Quality、RandomSeed、LegacyAffixDigest、provenance 与 non-spatial/no-child 资格必须保持。不得 Split、Merge、Swap、clone、新建 ItemId、新建 ChildContainer、改写 parent graph 或把该 root 当 P19 graph。
3. 只有 P1 candidate、P31 Registry insert、`NextWorldDropOrdinal`、P13 reconcile、full candidate validation 与一次 `SaveRecord(Candidate)` 都接受后，才建立 deterministic `WorldDropId`、derived world container、record 与 Actor projection。其他 record、child contents、parent placement、ordinal 与 Actor 均不得变动。
4. Candidate、source/child validation、floor placement、P13 reconcile、Registry validation、Actor projection 前检查或 SaveRecord 任一失败时，完整恢复 `BeforeSnapshot`：source 仍在原 child SlotIndex，parent/child graph、Registry、ordinal、record 与 Actor 均不变，且不存在 phantom world container 或短暂 child 空格。

#### 5.4 Exact opened WorldDrop → explicit player target

1. 世界 Actor 只能提交“打开这个 exact record”的窄生命周期信号。Workspace `WorldDropTarget` 与 Drag payload 必须带并在 Preview/Commit 前复核 exact OwnerId、RunInstanceId、WorldDropId、Ordinal、record revision、derived container、root ItemId、route、focus、target-open generation 与 P6 revision。
2. 一个 Target 仍只渲染其 exact record 的一个 root Cell。不存在 Registry 列表、多 root 面板、child list、Take All、自动打开其他 Actor、跨 record target 或显示顺序猜测。
3. P35 record 的 normal Drag 只能落至用户明确 Drop 的以下目标之一：
   - 一个空、正式 `BaseQuick` 普通储物格；
   - 当前仍已打开、identity-valid、属于 active P6 的 P17 child 内一个空、正式普通储物格；
   - 一个空、正式、Definition-compatible P6 equipment slot。

   child target 必须从当前 P17 truth 读取 parent、child container、capacity、slot semantic 与 one-layer eligibility；不得自动打开、切换、搜索、选择或猜测 child。不得落到空间 parent、Hotbar、P5/P9/P11、尸体、another WorldDrop、占用格或 UI 外区域。
4. Drop 到 `BaseQuick` 或 valid child 普通格时，复用 P1 whole-root `Move`。Drop 到空兼容 equipment slot 时，复用 P1 formal Equip/root relocation。三者均保留同一 ItemId/Definition/Quantity/provenance，且不自动 Bind、Use、Equip 或恢复历史 binding。
5. 只有 accepted proof 证明 root 已离开这个 exact derived world container，才可在同一个 Owner candidate 中移除对应 record、空 world container 与其 Actor projection。不得先删除 record/Actor 后尝试 Move，不得误删相邻、first/last、当前选中或任何 other Registry record。
6. target occupied、不兼容、child closed/stale、parent mismatch、close/focus loss、Actor EndPlay、record/root/container/ordinal mismatch、错误 Owner/Run、terminal/Prepared、SaveRecord failure 或 Host 无效时都必须零写入。world root、record、Actor、parent/child graph 和所有无关装备均保持原状。

#### 5.5 单一事务、Registry 生命周期与非回归

1. P35 的 player → world 与 world → player 每个输入事件最多形成一个 P1 candidate、一次 P31/P6 Owner replacement 与一次 `SaveRecord`。不得使用逐步 child removal、UI array mutation、Actor-first write、第二 Repository transaction 或 Code A inventory mutation。
2. Registry 的 canonical order 仅用于序列化、只读投影与静态比较；所有 P35 create/pickup 都只能按 exact `WorldDropId`/Ordinal/record revision 操作。P35 provenance 应在 accepted create 时由 durable record 保留为独立 canonical value，例如 `P35.AcceptedGroundDrop.ChildStandardEquipment`；UI 只能投影它。不得改写 P31 migration、other-record isolation 或 `NextWorldDropOrdinal` 的 accepted-create-only 规则。
3. P8 `BuildP14PlayerOnlySession`/等价 finalization 必须继续遍历全 Registry，并将 remaining P35 world root、derived world container 与所有 P19 closure 从 player-only final graph 排除。P35 不重写 terminal 分类、P5 receipt、recovery 或 run replacement。
4. repeated Actor refresh、map reload、active P6 recovery、child close、Actor EndPlay、lost focus 和 duplicate terminal observer 都只处理 matching projection/transient context，不能改变 durable child graph、删除 record 或把 root 自动回填玩家侧。
5. 成功后只刷新 source/target、matching child projection、exact `WorldDropTarget` 与 matching Actor；不得 Sort、Compact、重排无关 SlotIndex、重建空间区域、清空无关选择、重置无关 scroll、自动 Bind/Use/Equip，或改变 P17/P19/P26—P34 的产品语义。

### 6. 允许范围

允许以最小方式修改：

- Code B P1/P2/P3/P4/P7/P17/P23 的既有 root relocation、current child identity、Drag payload、preview、commit、rollback 与 projection，仅用于 P35 child-standard GroundDrop/explicit pickup；
- P14/P31 所在的 P6 durable Store、WorldDrop record validation、accepted create/pickup proof、P13 reconcile、P8 player-only generic-root filtering 与 Actor projection callback，仅用于表达 P35 的 single-root accepted transaction；
- `GroundDropZone`、`WorldDropTarget`、P7 current child target projection 与最小 Code A floor-placement/Actor presentation adapter；Code A 继续只做边缘投影与生命周期转发；
- 必要 include、声明、Build.cs、项目资料、本任务 Prompt 归档和本任务 Report。

### 7. 明确不在本任务内

- 不新建项目、版本线、Fix、第二 Repository、第二 P5/P6、第二 Warehouse、WorldDrop inventory、Widget inventory、fixture、假 ItemId、clone、Code A mirror、双写、存档重置或历史数据改写。
- 不扩大到 spatial parent/child graph、nested container、space parent/child 落地、child graph closure、任何带 `ChildContainerId` root、simple stack quantity、Merge、Split、multi-item WorldDrop、cross-record transfer、multi-slot/rotation、地面自动堆叠或跨 record merge。
- 不实现 player-side standard root `Ctrl + 左键`、quick-drop、P35 record quick-pickup、auto equip、auto target、Actor direct pickup、距离自动拾取、交互键领取、Take All、right-click Take、double-click、Swap、Replacement、Sort、Compact、绑定、使用、装备数值、战斗效果或 HUD 接管。
- 不改变 P5/P6 bridge、M01、P8 receipt 产品分类、P9/P10、P11/P12/P21 corpse source、P13/P15、P16、P19、P20、P26—P34、搜索、敌人、地图、战斗、生命、死亡、撤离、商店、经济、制作、网络或多人。
- 不启动产品、PIE、Standalone、真实鼠标键盘输入、截图、Smoke、Automation、回归、试玩、Cook、Package 或最终验收。
- 不在回传 Report 前自动开始 P36、Fix 或 F。

### 8. P 阶段静态审查与编译

完成后只执行以下检查：

1. 审查 P35 source 只接受 exact active P17 child 中 direct standard non-spatial root，并完整复核 parent/child/Owner/Run/revision/SlotIndex/Definition/ItemId/quantity/no-child identity；BaseQuick、装备、space graph、stack、P5/P9/P11、corpse、Hotbar、stale/close/terminal 均零写入。
2. 审查 player → world 只由既有 `GroundDropZone` normal Drag 进入，并形成单一 P1 whole-root relocation、单一 P31 Registry candidate、一次 Owner save；确认无临时 BaseQuick、无 graph split、无 new ItemId/ChildContainer、无 actor-first write。
3. 审查 world → player 只作用于 exact opened P35 record，并且只允许明确空 BaseQuick、明确 current valid child 空普通格或明确空兼容 formal equipment slot；确认无 auto target/auto equip/Swap/Replacement/child auto-open。
4. 审查 P35 provenance、P31 multi-record identity、other-record isolation、`NextWorldDropOrdinal`、Actor diff、open/close/stale/EndPlay/recovery/terminal、P8 full-registry exclusion、BeforeSnapshot rollback 与 P13 reconcile 均保持；成功删除只删除 exact record。
5. 审查 P17 one-layer graph、P19/P30 complete graph、P26—P29 simple stack、P32/P33 normal Drag、P34 QuickTransfer、P21 corpse equipment、P5/P6 bridge、P8 receipt、P13/P15 与 Code A authority 均无产品语义回归。
6. 审查 `Ctrl + 左键`、`Shift + 1—9`、right-click、double-click、ordinary Drag、Actor interaction与 scrolling：P35 只新增 normal child-standard Drag 到/自 GroundDrop，不新增隐式位置写入。
7. 编译 Editor：

       "C:\\Program Files\\Epic Games\\UE_5.8\\Engine\\Build\\BatchFiles\\Build.bat" demo_mapEditor Win64 Development "C:\\AIDev\\shanmen-ue\\Dev.D.UE.0.0.9B\\demo_map.uproject" -WaitMutex -NoHotReload

8. 编译 Game：

       "C:\\Program Files\\Epic Games\\UE_5.8\\Engine\\Build\\BatchFiles\\Build.bat" demo_map Win64 Development "C:\\AIDev\\shanmen-ue\\Dev.D.UE.0.0.9B\\demo_map.uproject" -WaitMutex -NoHotReload

若编译失败，只修复本任务引入的 child eligibility、GroundDrop transaction、exact pickup、projection、rollback、include 或签名问题；若必须扩大到本任务以外，停止受影响部分并如实报告。

### 9. Report 与完成信号

生成 `Dev.D.UE.0.0.9B.P35.0.r0_report.md`，保存至：

    C:\\AIDev\\shanmen-ue\\Dev.D.UE.0.0.9B\\Docs\\Report

Report 至少列出：

1. 实际修改/未修改文件及职责；
2. P17 current child identity 与 P26/P32/P33 GroundDrop 到 P35 的活动调用图，以及没有新增 pointer/UI/Actor 写入路径的证据；
3. child standard-root canonical eligibility、P31 exact record identity、P35 provenance、P19/P26—P34 branch rejection 证据；
4. parent/child open/close、Owner/Run/revision/record/root/ordinal、focus/stale/recovery/terminal 生命周期与零写入结论；
5. child root → GroundDrop 的 P1 whole-root Move、single candidate、single Owner save、ItemId/Definition/Quantity/provenance 保持、accepted-only record/Actor 创建与 rollback 证据；
6. exact WorldDrop → explicit BaseQuick / explicit valid child ordinary slot / explicit compatible equipment slot 三条 pickup path、无 auto target/auto equip/Swap/Replacement、exact record cleanup 与 other-record isolation 证据；
7. P17 graph、P19/P30、P26—P29、P32/P33、P34、P5/P6/P8/P13/P15/P21 与 Code A authority 的保持结论；
8. stable SlotIndex、动态空间容量、scroll、selection、right-click、double-click、`Ctrl + 左键`、`Shift + 1—9`与无 Sort/Compact/auto behavior 的结论；
9. 两个编译命令、目标、原生 exit code 与关键结果；
10. 所有未执行的 F 阶段真实验证，至少包括：多个 P17 child 内不同 Weapon/Armor/Accessory root 落地；多 record 同时存在；分别拖回 BaseQuick/当前 child/正确装备位；child closed、occupied/incompatible target、P19 graph/stack/warehouse/corpse/Hotbar 拒绝；stale/close/EndPlay/recovery/terminal/save failure；Actor projection、P8 full-registry exclusion、真实鼠标键盘、截图、Smoke、Automation、回归、Cook 与 Package。

仅当 P35 child-standard GroundDrop/explicit pickup 静态闭合、P17/P19/P26—P34 与 P31 multi-record 边界保持、P8 全量排除无泄漏，且 Editor 与 Game 均以 native exit code `0` 完成时，使用：

    READY_FOR_P36_PLANNING

若当前范围内仍有可修复问题，使用：

    NEEDS_P35_REWORK

若 P17 的现有 current child identity、P1 whole-root relocation 或 P31 durable Store 无法在不建立第二输入路径、第二保存、第二物品真值、改写 space graph 或自动目标的前提下支持本项，使用：

    NEEDS_PLANNER_DECISION
    BLOCKED

完成后不得自动开始 P36、Fix 或 F。向策划 Chat 回传并附带且只附带本次同名 Report；正文首行使用：

    [CSEMI:REPORT_SENT] {"task_id":"Dev.D.UE.0.0.9B.P35.0.r0","file":"Dev.D.UE.0.0.9B.P35.0.r0_report.md"}
