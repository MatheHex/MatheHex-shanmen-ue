# Dev.D.UE.0.0.9B.P39.0.r0

## 任务身份

- 项目：Dev.D.UE.0.0.9B；继续使用同一活动工程，不新建项目。
- 阶段：主线 P39——为已打开、已揭示的 P21 尸体装备补齐受限的 `Ctrl + 左键`快捷拾回，并与 P36/P37 已接受的“输入时冻结、当前空间 child 优先”规则对齐。
- 任务编号：Dev.D.UE.0.0.9B.P39.0.r0。
- 前置：已接受 0.0.9B.P1—P38 与 0.0.9BFix.P1—P4。Fix 只用于已确认、已验收功能的缺陷修复；P39 是新增主线功能，不是 Fix。
- 执行文件：Dev.D.UE.0.0.9B.P39.0.r0_prompt.md。
- 报告文件：Dev.D.UE.0.0.9B.P39.0.r0_report.md。
- 活动工程根：C:\\AIDev\\shanmen-ue\\Dev.D.UE.0.0.9B。
- 活动工程：C:\\AIDev\\shanmen-ue\\Dev.D.UE.0.0.9B\\demo_map.uproject。
- 任务性质：P 阶段只做实现、静态审查与代码编译。不得启动产品、PIE、Standalone、真实输入验证、截图、Smoke、Automation、回归、Cook、Package 或 F 阶段测试；不得自动开始 P40、任意 Fix 或 F。

## 上半部分：只读项目裁决、现状与边界

### 1. 当前唯一有效基线

唯一有效依据是当前 0.0.9B、活动工程及已接受任务链。所有 0.2、V2、V3、I、IPF、历史页面壳、旧 CTA、旧库存和旧物品规则均已过时；不得读取、采用、恢复或以其决定实现、验收或范围。

Code B P1 Repository 与既有 durable Store 是唯一可变物品真值。每件物品始终只有一个真实 ItemId、一个真实父位置和一条权威事务链。Widget、Cell、Presenter、DragOperation、Workspace Context、BodyTarget、WorldDropTarget、WorldDrop Actor、地图放置适配层和 Code A 都只能持有只读投影、选择或瞬时意图；不得持有第二库存、可写数量副本、预建 ItemId、平行尸体背包、Actor-first 写入或 A/B 双写。

P11/P12 已建立唯一 `BasicCorpse` 的死亡回执、首次物质化、`Hidden → Searching → Revealed`、已打开 BodyTarget 与 P11/P6 单次 durable transfer。P21 在 future-only `BasicCorpse.r3` 中建立 `Body.Weapon`、`Body.ArmorRobe`、`Body.Accessory0` 三个固定尸体装备位；每个 slot 至多出现一件真实、non-spatial、Quantity=1、MaxStack=1 的 standard root。P38 已允许这些已 Revealed、identity-valid root 通过普通显式 Drag，直接进入明确空 BaseQuick、明确当前有效 P17 child 普通格或明确兼容空装备位；P38 没有给它们授权任何快捷拾回。

P36/P37 已建立共享 `Ctrl + 左键` QuickTransfer 的两种输入时冻结目标模式：输入时存在 identity-valid current P17 child 时，只向该 exact child 解析；输入时根本不存在 valid child 时，才向 BaseQuick 解析。所选 child 后续关闭、切换、失焦、失效或已满时必须拒绝，绝不静默回退。P39 只把这一已接受模型扩展到 P21 corpse-equipment source；它不改变 P36/P37 所有既有 WorldDrop provenance、record 生命周期或目标规则。

### 2. P39 产品裁决

当玩家已经通过 P12 主动打开 identity-valid 的唯一 BasicCorpse，且一个 P21 `Body.Weapon`、`Body.ArmorRobe` 或 `Body.Accessory0` slot 已 Revealed，玩家对其唯一 root Cell 按下 `Ctrl + 左键`时，必须由既有共享 QuickTransfer resolver 处理。自动目标只在输入时决定一次：

| 输入时 source | 输入时目标模式 | 唯一允许自动目标 | accepted 事务 |
| --- | --- | --- | --- |
| exact opened/revealed P21 corpse-equipment root | 当前存在 identity-valid P17 child | 该 exact current child 内按真实 stable `SlotIndex` 升序的第一个合法、空 ordinary storage cell | 一个 P1 whole-root `Move(1)` |
| exact opened/revealed P21 corpse-equipment root | 当前不存在 identity-valid P17 child | active P6 BaseQuick 内按真实 stable `SlotIndex` 升序的第一个合法、空 ordinary storage cell | 一个 P1 whole-root `Move(1)` |

这是一条“已打开空间窗口时优先进入当前窗口”的快捷拾回规则，不是自动装备规则。若输入时选择 `CurrentP17Child`，intent 必须冻结 exact child `ContainerId`、空间 parent `ItemId`、child open generation、Owner/Run、BodyTarget/death receipt/source slot、P6 revision 与必要 route/focus proof。commit 前 child 关闭、切换、失焦、generation 失效、parent/child 拓扑失效、target stale 或 child 无空格时一律拒绝；不得静默改投 BaseQuick、另一 child、装备位、Hotbar、仓库或其他位置。

只有输入时根本不存在 identity-valid current P17 child，才可建立 `BaseQuickNoChildAtInput` 模式。输入后打开、切换或投影出 child 不得改变该模式；BaseQuick 满、source/BodyTarget stale、Owner/Run/P6 revision 不匹配或任何 session/保存检查失败时必须零写入。

P39 不增加快捷丢弃、自动装备、装备位扫描、自动打开/切换 child、Swap、Replacement、Sort、Compact、auto bind、auto use、数量操作、P21 source 回存尸体或第二尸体背包。玩家仍可用 P38 的 normal Drag 将尸体装备明确拖入空 BaseQuick、current child 或兼容空装备位。

### 3. 严格范围与持续排除

P39 source 只接受同时满足全部条件的 root：

1. 当前 Workspace 是既有 P12 production Host；exact BodyTarget 已 Open、已 Revealed、identity-valid，且无 active action/search；OwnerId、RunInstanceId、BodyTargetId、DeathReceiptId、body record revision、P6 composite revision、route、focus、target-open/page generation 与 active session 都能在 Preview 和 durable Commit 前从 P1/P6/P11 truth 重新验证；
2. source 位于 exact P21 formal equipment slot：`Body.Weapon`、`Body.ArmorRobe` 或 `Body.Accessory0` 的 slot 0。P11 receipt/profile、P21 candidate-set digest、Catalog 与 P1 snapshot 必须共同证明它是 future-only r3 candidate-set 内的 single standard root；不得以显示名、图标、Cell class、Actor、尸体展示顺序、当前选择或 UI cache 判断；
3. root 是 compatible standard non-spatial Weapon、Armor/ArmorRobe 或普通 Accessory，且 `Quantity=1`、`MaxStack=1`、不可堆叠、无 `ChildContainerId`、无 spatial semantic、无 graph closure；exact source container slot 0 反向指向同一 ItemId；
4. frozen mode、Owner/Run、P6/body revision、active session、Prepared/terminal gate、Host validity、source Reveal/Open 状态及全部 mode-specific identity 在 durable commit 前仍完整成立。

`CurrentP17Child` 模式还必须同时满足：

1. 输入时 Workspace 已有一个 current、opened、identity-valid 的 P17 child；intent 必须携带 exact formal space parent ItemId、child ContainerId、active child open generation 与 source P6 revision；
2. durable Store 从 P1/P6 重建唯一 active P17 parent → child 映射，验证 parent 位于正式 SpatialRing 或 Backpack placement、空间定义、dynamic ChildContainerCapacity、一层无环规则、child ordinary slot semantic 与 active session；不得由显示顺序、焦点或 UI cache 推断；
3. target 只能是该 exact child 内按真实 stable SlotIndex 升序找到的第一个正式、可写、空 ordinary slot。child 不存在、关闭、切换、失焦、generation 不匹配、parent mismatch、capacity/slot semantic 失效、无空格或 revision stale 时均零写入；
4. intent 不得改为 BaseQuick mode，不得搜索另一 child、另一空间 parent、装备位、Hotbar、P5/P9/P11、WorldDrop 或另一 BodyTarget。

`BaseQuickNoChildAtInput` 模式还必须同时满足：

1. 输入时不存在 identity-valid current P17 child；这一事实必须是 intent 的明确模式值，而非 commit 时重新猜测；
2. target 只能是 active P6 BaseQuick/Basic 根容器内按真实 stable SlotIndex 升序找到的第一个正式、可写、空 ordinary slot；
3. 输入后新打开、切换或投影出 child 不得改变目标；BaseQuick 满、stale、Owner/Run/revision 不匹配或任一 session/body/保存检查失败时零写入；不得自动装备、搜索 child 或选择其他位置。

下列对象或动作持续不属于 P39：

1. P12 普通 corpse root、Hidden/Searching slot、empty P21 slot、P6 player source、P5 warehouse、P9 ordinary container、P14/P31 WorldDrop、P19 complete graph、P26—P29 simple stack、P32—P37 ground source family、unknown provenance、Hotbar、another BodyTarget 或 UI-only object；
2. player-side `Ctrl + 左键`、quick-drop、Actor direct pickup、距离自动拾取、交互键领取、right-click Take、double-click、Take All、auto equip、auto target（除本项两种 frozen mode 的 ordinary-slot scan）、auto bind、auto use、Swap、Replacement、Sort、Compact、Merge、Split、Quantity=0、`WorldPickupDraft`、`PlayerSplitDraft`、record-to-record transfer；
3. P38 normal Drag 的显式三目的地 policy、P21 P6 → corpse equipment 回存拒绝、P21 r1/r2/history/deterministic roll、P11/P12 search/reveal state machine、P17 graph construction、P31 Registry、P8 terminal 分类、P13 binding、P15 use 与 Code A authority；
4. 实机运行、PIE、Standalone、真实鼠标键盘、截图、Smoke、Automation、回归、Cook、Package 或最终验收。

## 下半部分：授权执行内容

### 4. 单一授权目标

在不建立第二物品真值、不改变 P11/P12/P21 尸体来源、不改变 P17 graph、不改写 P31 Registry、不中断 P29/P30/P34/P36/P37/P38 既有语义且不新增自动装备的前提下，将 current opened/revealed exact P21 corpse-equipment root 接入 P36/P37 已有的 frozen-target QuickTransfer model：

1. 只有 current opened/revealed exact P21 equipment root Cell 才可建立一次 P39 QuickTransfer transient intent；
2. intent 在输入时一次性选择 `CurrentP17Child` 或 `BaseQuickNoChildAtInput`，并冻结模式所需的完整 source/body/target identity；
3. accepted path 只形成一个 P1 whole-root `Move(1)`、一次 P11/P6 Owner durable replacement 和一次 `SaveRecord`；
4. 成功后只刷新 exact BodyTarget source section、解析得到的 P6 target 与必要 P7 child projection；不创建 WorldDrop、record、ordinal 或 Actor；
5. 任一 source、mode、target、identity、capacity、session、close/stale 或保存检查失败时完整零写入与 BeforeSnapshot rollback。

### 5. 实现要求

#### 5.1 先完成活动调用链与资格审计

改动前必须审阅并在 Report 中列出：

1. P12/P21/P38 的 BodyTarget open/reveal lifecycle、P21 equipment root Cell、现有 normal Drag、page/focus invalidation、P11/P6 callback 与 P38 source proof；说明 P39 如何复用既有 source Cell 和共享 Ctrl pointer router，而不是新增 Button、hotkey、pointer handler、Widget、Actor 或尸体写入口；
2. P29/P30/P34/P36/P37 的 shared QuickTransfer transient intent、pointer consumption、mode routing、preview、commit、cancellation 与 stale lifecycle；说明 P39 如何只增加 P21 canonical source branch；
3. P21 r3 `Body.Weapon`、`Body.ArmorRobe`、`Body.Accessory0` 的实际 stable semantic/container/provenance、future-only receipt/profile/digest 与 source eligibility；实际值必须如实记录；
4. P17 current child identity、parent → child canonical topology、open generation、dynamic capacity、ordinary Cell 与 stable SlotIndex，以及 P6 BaseQuick 的正式 ordinary target policy；
5. P1/P2/P3/P6/P11 的 whole-root Move、cross-graph single candidate、BeforeSnapshot rollback、P11/P6 single Owner durable replacement、P13 reconcile 与 revision/session 调用图；
6. 所有 Widget direct Move、Actor direct pickup、尸体展示缓存写入、right-click Take、double-click、预建 ItemId、旧 Code A loot、按 first/last/selection 猜测 target、第二 Repository transaction、第二 save 或 Code A inventory writer；它们不得成为 P39 写入路径。

#### 5.2 共享输入、source gate 与 mode 冻结

1. `UCodeBP3CellButton::NativeOnMouseButtonDown` 或当前等价共享 pointer router 仍是 `Ctrl + 左键`唯一消费点。命中 P39 source 后只能建立一次既有 QuickTransfer transient intent 并返回 `Handled`；不得继续 ordinary selection、drag threshold、P15 Use、right-click detail 或 Actor interaction。
2. 不得为 P39 新建 Button、hotkey、Actor click、专用 Widget、第二 pointer handler、second QuickTransfer resolver 或 UI direct write。resolver 必须从 canonical P21 source proof、P11 exact source address、input-time frozen mode 与 P17/P6 truth 在 P29/P30/P34/P36/P37/P39 分支间分流。
3. intent 创建时必须先重验 exact BodyTarget/death receipt/profile/digest/source-slot identity 与 P21 r3 qualification，再只执行以下二选一：
   - current P17 child 已 identity-valid：写入 `CurrentP17Child`，以及 exact parent ItemId、child ContainerId、open generation、P6 revision；
   - 不存在 current valid P17 child：写入 `BaseQuickNoChildAtInput`；不得留空、延迟决策或把 UI focus/selection 当成随后可变 target。
4. Preview 与 durable Commit 前必须重验 OwnerId、RunInstanceId、BodyTargetId、DeathReceiptId、body record revision、source ContainerId/SlotIndex、root ItemId、P21 semantic/profile/digest、Reveal/Open、route、focus、page/target-open generation、P6 revision、active session、Prepared/terminal gate、Host validity 与 mode-specific target proof。
5. BodyTarget close/reopen、focus loss、开始/取消 search、Actor EndPlay、map reload、recovery、terminal/Prepared、receipt/root/container mismatch、payload cancel、Host invalidation 或 source revision stale 必须立即使 intent 失效并零写入。`CurrentP17Child` mode 另须在 child close/switch/focus loss/open generation mismatch/parent mismatch/capacity invalidation 时失效；不得变为 BaseQuick fallback。
6. ordinary left click 继续只选择；right-click 继续只读详情；normal Drag 继续只走 P38 explicit target policy。double-click、Tab、I、Esc、Close、Cancel、scroll、空白区、无 payload Drop、`Shift + 1—9`、P12 ordinary corpse root、player-side Ctrl 与所有 existing WorldDrop Ctrl branch 均不得获得 P39 位置写入语义。

#### 5.3 canonical source 与确定 target resolver

1. Preview 只接受 current exact P11 body equipment container slot 0 的 root。Store 必须从 canonical Catalog、P1/P6/P11 snapshot、P21 receipt/profile/candidate digest 重建并全字段验证 P21 provenance、standard definition、non-spatial/no-child/no-graph closure、Reveal/Open、source slot、record availability 与 exact ItemId。
2. P39 source 必须拒绝 stack、partial draft、space parent/child、P19 graph、P12 ordinary corpse item、P5/P9/P14/P31、warehouse、Hotbar、another BodyTarget、unknown family、hidden object、wrong BodyTarget generation 或任何 mode/source mismatch；它们均零写入。
3. `CurrentP17Child` mode 的唯一 target 是 intent 中 exact child 内按真实 stable SlotIndex 升序扫描的第一个正式、可写、空 ordinary slot。每个 candidate 必须由 P1/P6 truth 验证 ContainerId、Owner/Run、parent/child topology、capacity、slot semantic、slot availability、open generation 与 P6 revision。没有合法空格时拒绝；不得扫描 BaseQuick 或另一 child。
4. `BaseQuickNoChildAtInput` mode 的唯一 target 是 active P6 BaseQuick 内按真实 stable SlotIndex 升序扫描的第一个正式、可写、空 ordinary slot。每个 candidate 必须由 P1/P6 truth 验证 ContainerId、Owner/Run、capacity、slot semantic、slot availability 与 P6 revision。没有合法空格时拒绝；不得扫描 child、装备位、Hotbar、P5/P9/P11、世界容器、最近空位或 fixture。
5. 自动解析仅限上述两种 mode 的一个 ordinary storage slot。不得把 Weapon、Armor、Accessory、SpatialRing、Backpack 或任何装备位当成 P39 自动目标；不得调用 Equip、Unequip、Merge、Split、Quantity=0、RequestedMergeQuantity、`WorldPickupDraft`、`PlayerSplitDraft` 或 P19 graph Move。

#### 5.4 单一 whole-root 事务、持久化与回滚

1. Preview 成功后只能建立一个 Owner/Run scoped Candidate，并只调用一次 P1 existing whole-root `Move(1)`：同一个 ItemId 从 exact P11 P21 equipment container slot 0 直接移到经 frozen-mode resolver 确定的 exact empty ordinary target。不得先落 BaseQuick、再二次 Move，不得先清 source，也不得用 P6 local move 绕过 P11/P6 atomic transaction。
2. Commit 必须沿既有 P12/P3 → P2 → P1 → P11/P6 durable callback。Store 在任何 durable write 前重新验证 command intent、source/target stable address、BodyTarget/death receipt/root、Owner、Run、revision、P21 canonical eligibility、mode proof、target empty state、active session 与 lifecycle gate。
3. accepted candidate 中 DefinitionId、ItemId、Quantity、Level、Quality、RandomSeed、LegacyAffixDigest、P21 candidate provenance 和 no-child/non-spatial qualification 必须完全保持；唯一合法变化是 parent/slot 从 exact P11 body equipment container 变为 exact child 或 BaseQuick ordinary slot。不得生成新 ItemId、ContainerId、receipt、body record、WorldDrop、ordinal、Actor、child、binding 或第二 revision。
4. only after accepted snapshot/replay proof 明确证明 root 已离开该 exact P11 source，才可在同一 Owner candidate 中刷新 exact BodyTarget/P7 target projection。不得预清 source、预写 P6、预删 BodyTarget、预刷 Code A Actor 或在 save 后补写另一侧。
5. P11 与 P6 必须在同一 Owner durable replacement 中共同提交；P13 只按既有 accepted commit reconcile，不得自动 Bind、Use、Equip 或恢复历史 binding。P8 仍只结算当时 P6 player graph，并只丢弃 P11 residual；P39 不改变尸体死亡、reveal、receipt、终局或 recovery。
6. candidate、canonical gate、mode resolver、target scan、P1 Move、P13 reconcile、projection 前检查或 `SaveRecord` 任一失败时，必须完整恢复 BeforeSnapshot：P11 source root、P6 target cell、BodyTarget visibility/open state、P7 projection、selection 与无关 body/world record 均保持未变，且不得遗留 phantom empty slot、phantom pickup 或 transient item copy。
7. 成功后只刷新 exact BodyTarget source section、解析得到的 BaseQuick/child target 和必要关联 child projection；不得 Sort、Compact、重排无关 SlotIndex、重建无关空间区域、清空无关选择、改变无关 scroll offset 或改写 P29/P30/P34/P36/P37/P38 语义。

#### 5.5 非回归、终局与权威边界

1. P29 simple-stack WorldDrop Ctrl + 左键继续保留 active P17 child 优先、既有 BaseQuick fallback、merge-first/empty-second 与 player → world compatible Merge 语义；P39 不得让 P21 corpse equipment 进入 P29 branch。
2. P30/P19 complete graph WorldDrop Ctrl + 左键继续只将合法完整图移入首个空 BaseQuick；P34/P36/P37 WorldDrop source family 的 record/provenance/target semantics 保持。P39 不得创建、删除或改写任一 P31 record、ordinal 或 Actor。
3. P38 normal Drag 与 explicit BaseQuick/current child/equipment target policy 保持；P39 只添加 P21 source Ctrl quick pickup，绝不改变 P38 source gate、normal drag、尸体回存拒绝或 explicit equipment pickup。
4. P21 r1/r2、已 materialized records、r3 deterministic identity、Optional.EquippedLoadout、candidate/weight/digest、最多一件装备、三个固定尸体装备位、Hidden/Searching/Reveal 与 P6 → corpse equipment 拒绝均保持。P39 绝不重掷、补料、迁移或改写尸体来源。
5. P17 one-layer graph、P31 Registry、P8 `BuildP14PlayerOnlySession`/等价 finalization、P5/P6 bridge、P13 binding、P15 use、P20/P21 body source 与 Code A authority 均不改变。Code A 继续只拥有 Actor、交互距离、地图、死亡、战斗、Run 与终局边缘；它不得获得 Item、Container、Quantity、P11/P6、BodyTarget、Loot、search、terminal 或 player-equipment durable authority。

### 6. 允许的改动范围

仅允许在当前 Code B 中最小修改：

- P12/P21/P38 已有 body-aware source proof、shared pointer routing、QuickTransfer preview/commit、P11/P6 durable transaction 与 projection；
- P29/P30/P34/P36/P37 的既有 frozen-target resolver、transient intent、current child/BaseQuick validation、rollback 与 branch discrimination，仅用于 P39 P21 source；
- P1/P2/P3/P6/P11/P13/P17 的必要声明、candidate proof、rollback 或调用签名兼容，前提是不改变已有产品语义；
- PROJECT.md、PROJECT_INFO_CARD.md、本任务 Prompt 归档与本任务 Report。

禁止新建项目、版本线、Fix、第二 P11/P6、Widget inventory、Code A mirror、fixture、假 ItemId、clone、双写、存档重置或历史数据改写。禁止修改 Code A 功能逻辑、P21 deterministic source/history、P12 search state machine、P17 graph construction、P31 schema/registry、P8 receipt/terminal product logic、P5/P6 bridge 或任何测试文件。

### 7. 明确不在本任务内

- 不把 P21 ordinary corpse root、simple stack、space parent/graph、ground record、warehouse、ordinary container、player source 或任何 other provenance 接入 P39 QuickTransfer。
- 不实现快捷丢弃、自动装备、equipment target scan、auto child open/switch、fallback、Take All、right-click Take、double-click、Actor direct pickup、距离自动拾取、交互键领取、Swap、Replacement、Sort、Compact、bind、use、装备数值、战斗效果、HUD 接管、网络或多人。
- 不改变 P38 normal Drag、P29/P30/P34/P36/P37 现有 Ctrl semantics、P11/P12/P21 corpse source、P13/P15、P17/P19、P26—P28、P31、P5/P6/P8、搜索、敌人、地图、战斗、生命、死亡、撤离、商店、经济或制作。
- 不启动产品、PIE、Standalone、真实鼠标键盘输入、截图、Smoke、Automation、回归、试玩、Cook、Package 或最终验收。
- 不在回传 Report 前自动开始 P40、任意 Fix 或 F。

### 8. P 阶段静态审查与编译

完成后只执行以下检查：

1. 审查 P39 复用唯一 `Ctrl + 左键` pointer router 与既有 QuickTransfer intent；确认没有专用 input/UI/Actor 写入旁路，且分支只依据 canonical P21 source truth 与 exact BodyTarget identity。
2. 审查 source 只接受 current opened/revealed exact P21 r3 equipment root，并逐项复核 Owner/Run/BodyTarget/death receipt/body revision/source container-slot/root/page/focus/P6 revision；P12 ordinary corpse、Hidden/Searching、stack、space、world、stale/close/terminal 均零写入。
3. 审查 input-time frozen target mode：valid current child 时只按 stable SlotIndex 扫 exact child；输入时无 child 才只按 stable SlotIndex 扫 BaseQuick。确认 child 失效/满位不回退，BaseQuick mode 输入后不改 child，且无自动装备/装备位/Hotbar/P5/P9/P11 fallback。
4. 审查 accepted candidate 只使用一个 P1 whole-root Move、一个 P11/P6 Owner durable replacement 和一次 `SaveRecord`；确认 ItemId/Definition/Quantity/provenance 保持，无 new ItemId/Container/receipt/record/ordinal/Actor/child/second truth。
5. 审查 P11 source 只在 accepted proof 后移除、P13 reconcile、BeforeSnapshot rollback、P8 terminal/recovery、selection/scroll/stable SlotIndex 与 P17 graph 均保持；无 WorldDrop/record/Actor write。
6. 审查 P12 ordinary corpse、P21 history、P29 simple stack、P30/P19 graph、P34/P36/P37 WorldDrop Ctrl、P38 normal Drag、P31 Registry、P5/P6/P8/P13/P15 与 Code A authority 均无产品语义回归。
7. 审查 right-click、double-click、ordinary Drag、Actor interaction、`Shift + 1—9`、player-side Ctrl 和 P21 P6 → corpse drag 均未获得 P39 隐式位置写入语义。
8. 编译 Editor：

       "C:\\Program Files\\Epic Games\\UE_5.8\\Engine\\Build\\BatchFiles\\Build.bat" demo_mapEditor Win64 Development "C:\\AIDev\\shanmen-ue\\Dev.D.UE.0.0.9B\\demo_map.uproject" -WaitMutex -NoHotReload

9. 编译 Game：

       "C:\\Program Files\\Epic Games\\UE_5.8\\Engine\\Build\\BatchFiles\\Build.bat" demo_map Win64 Development "C:\\AIDev\\shanmen-ue\\Dev.D.UE.0.0.9B\\demo_map.uproject" -WaitMutex -NoHotReload

若编译失败，只修复本任务引入的 P21 QuickTransfer routing、source proof、frozen target resolver、P11/P6 transaction、projection、rollback、include 或签名问题；若必须扩大到本任务以外，停止受影响部分并如实报告。

### 9. Report 与完成信号

生成 `Dev.D.UE.0.0.9B.P39.0.r0_report.md`，保存至：

    C:\\AIDev\\shanmen-ue\\Dev.D.UE.0.0.9B\\Docs\\Report

Report 必须简洁、可审计地列出：

1. 本轮新增、修改、未修改的每个文件及职责；
2. P12/P21/P38 source Cell 到 P39 shared QuickTransfer 的完整调用图，以及没有新增 pointer、Widget、Actor、second resolver 或 Code A 写入口的证据；
3. P21 r3 actual stable source provenance/slot semantic、BodyTarget/death receipt/profile/candidate identity 与为什么 r1/r2/history/determinism 未被改写；
4. exact source gate、Reveal/Open lifecycle、Owner/Run/revision/session proof、input-time frozen mode 与 zero-write cancellation/stale policy；
5. `CurrentP17Child` 与 `BaseQuickNoChildAtInput` 两条 accepted path 的真实 SlotIndex 次序、capacity/empty validation、无 mode switch/fallback/auto equip/equipment scan 的证据；
6. single P1 whole-root Move、single P11/P6 durable replacement、source/target identity 保持、P13/P8、rollback、selection/scroll/stable SlotIndex 与无 WorldDrop/ordinal/Actor write 的证据；
7. P12 ordinary corpse、P21 source/history、P17/P19/P26—P38/P31、P5/P6/P8/P13/P15、Code A authority 的非回归结论；
8. 两个编译命令、目标、原生 exit code 与关键结果；
9. 所有未执行的 F 阶段真实验证，至少包括：P21 r3 future corpse materialize/reveal；Weapon/ArmorRobe/Accessory 各自 Ctrl 拾回至当前 opened valid child 与无 child 时首个空 BaseQuick；child full/closed/switched/focus/generation/parent/capacity stale；BaseQuick full；wrong/ordinary/Hidden/Searching source；BodyTarget/death receipt/Owner/Run/P6 revision stale；Prepared/terminal/Host invalid/SaveRecord failure；P38 normal Drag、P12 normal Move/Merge/Swap、P29/P30/P34/P36/P37 Ctrl branches、P31 other-record isolation、P8 terminal/recovery、真实鼠标键盘、PIE、Standalone、截图、Smoke、Automation、回归、Cook 与 Package。

仅当 P39 P21 corpse-equipment frozen-target QuickTransfer 静态闭合、P21/P12 与 P17/P19/P26—P38/P31/P8 边界保持，且 Editor 与 Game 均以 native exit code `0` 完成时，使用：

    READY_FOR_P40_PLANNING

若当前范围内仍有可修复问题，使用：

    NEEDS_P39_REWORK

若现有 P12/P11/P6 transaction 或 shared QuickTransfer resolver 无法在不创建第二输入路径、第二保存、第二物品真值、自动装备、改写 P21 deterministic history 或扩展 Code A 权威的前提下支持本项，使用：

    NEEDS_PLANNER_DECISION
    BLOCKED

完成后不得自动开始 P40、Fix 或 F。向策划 Chat 回传并附带且只附带本次同名 Report；正文首行使用：

    [CSEMI:REPORT_SENT] {"task_id":"Dev.D.UE.0.0.9B.P39.0.r0","file":"Dev.D.UE.0.0.9B.P39.0.r0_report.md"}
