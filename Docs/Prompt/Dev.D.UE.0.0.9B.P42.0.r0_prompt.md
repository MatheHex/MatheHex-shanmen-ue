# Dev.D.UE.0.0.9B.P42.0.r0

## 任务身份

- 项目：`Dev.D.UE.0.0.9B`；继续使用同一活动工程，不新建项目。
- 阶段：主线 P42——已揭示 BasicCorpse 完整空间图的**显式兼容装备位直达**。
- 任务编号：`Dev.D.UE.0.0.9B.P42.0.r0`。
- 前置：已接受 `0.0.9B.P1—P41` 与 `0.0.9BFix.P1—P4`。`Fix` 只用于已确认、已验收功能的缺陷修复；P42 是新增主线功能，不是 Fix。
- 执行文件：`Dev.D.UE.0.0.9B.P42.0.r0_prompt.md`。
- 报告文件：`Dev.D.UE.0.0.9B.P42.0.r0_report.md`。
- 活动工程根：`C:\\AIDev\\shanmen-ue\\Dev.D.UE.0.0.9B`。
- 活动工程：`C:\\AIDev\\shanmen-ue\\Dev.D.UE.0.0.9B\\demo_map.uproject`。
- Report 必须生成到：`C:\\AIDev\\shanmen-ue\\Dev.D.UE.0.0.9B\\Docs\\Report\\Dev.D.UE.0.0.9B.P42.0.r0_report.md`，并仅携带该同名 Report 回传策划 Chat。
- 任务性质：P 阶段只做实现、静态审查和代码编译。不得启动产品、PIE、Standalone、真实鼠标键盘验证、截图、Smoke、Automation、回归、Cook、Package 或 F 阶段测试；不得自动开始 P43、任意 Fix 或 F。

## 上半部分：只读项目裁决、现状与边界

### 1. 当前唯一有效基线

唯一有效依据是当前 `0.0.9B`、活动工程和已接受任务链。所有 `0.2`、`V2`、`V3`、`I`、`IPF`、历史页面壳、旧 CTA、旧库存和旧物品规则均已过时；不得读取、采用、恢复或以其决定实现、验收或范围。

Code B P1 Repository 与既有 durable Store 是唯一可变物品真值。每件物品始终只有一个真实 `ItemId`、一个真实父位置和一条权威事务链。Widget、Cell、Presenter、DragOperation、Workspace Context、BodyTarget、空间 child 投影、WorldDrop Actor、地图放置适配层和 Code A 都只能持有只读投影、选择或瞬时意图；不得持有第二库存、可写数量副本、预建 `ItemId`、平行尸体背包、Actor-first 写入或 A/B 双写。

P11/P12 已建立唯一 `BasicCorpse` 的死亡回执、首次物质化、`Hidden → Searching → Revealed`、已打开的 `BodyTarget` 与 P11/P6 的单次 durable transfer。P20 为未来 BasicCorpse 的 `Optional.SpatialUtility` 确立了 canonical `WindTalisman` 或 `BackpackLevel1` parent，加其唯一、空的 P17 `ChildContainer`，作为不可拆分完整图。P21 r3 仅继承该 P20 spatial group；其固定 Weapon／ArmorRobe／Accessory0 装备来源是另一条 provenance，不能与 P42 混用。

P20 已允许上述尸体空间 parent 经 P12 的 normal Drag 进入明确空的 `BaseQuick`；P41 已为同一 source 提供 `Ctrl + 左键`至首个合法空 `BaseQuick` 的受限快捷拾回。P7 已有从 P6 `BaseQuick` 到对应正式装备位的真实 Drag/Drop。P19 也已证明完整空间图可以经明确 normal Drag 进入自身兼容、空的 formal equipment position。P38 则已为 P21 尸体标准装备增加显式直达玩家 equipment slot。

因此 P42 只补齐一个真实、明确的交互缺口：玩家已打开并揭示尸体后，可把 P20 canonical spatial parent **以普通 Drag/Drop 直接放入用户明确指向的、空且兼容的空间装备位**。P20 原有“拖至空 `BaseQuick`”路径保持完全有效；P41 的快捷 BaseQuick-only 语义也保持不变。

### 2. P42 产品裁决

当玩家已经通过既有 P12 流程主动打开 identity-valid 的唯一 BasicCorpse，且一个 ordinary body-storage root 是已 Revealed 的 P20 canonical complete graph 时，玩家可以从该 root Cell 发起既有 normal `DragOperation`，并仅把它 Drop 到其 Definition 对应的、用户明确指定且为空的 P6 formal equipment slot。

| Exact P20 source graph | 用户明确 Drop 的唯一新增 target | 权威事务 |
| --- | --- | --- |
| `WindTalisman` parent + 唯一 formal empty child closure | 空、正式、Definition-compatible `Prototype.Slot.SpatialRing` | 一个 P1/P20 whole-graph relocation；同一 P11/P6 Owner durable replacement 与一次 SaveRecord |
| `BackpackLevel1` parent + 唯一 formal empty child closure | 空、正式、Definition-compatible `Prototype.Slot.Backpack` | 一个 P1/P20 whole-graph relocation；同一 P11/P6 Owner durable replacement 与一次 SaveRecord |

这只是用户明确拖拽的目的地扩展，不是自动装备、候选扫描、快捷领取、快速丢弃、按钮领取或新的空间道具系统。目标由用户的真实 Drop 指定；Preview 与 Commit 只重验同一个 `ContainerId + SlotIndex`，不得因它满、stale、不兼容或保存失败而改投 `BaseQuick`、另一个装备位、P17 child、Hotbar、地面或其他位置。

P42 不改变 P20 当前 `BaseQuick` normal Drag 语义，也不让 P41 的 Ctrl + 左键自动装备。空间 parent 仍不得进入任何 P17 child；P17 一层限制继续禁止空间嵌套。

### 3. 严格范围与持续排除

P42 source 只接受同时满足以下全部条件的完整图：

1. 当前 Workspace 为既有 P12 production Host；exact BasicCorpse `BodyTarget` 已 `Open`、已 `Revealed`、identity-valid，且无 active search/action。`OwnerId`、`RunInstanceId`、`BodyTargetId`、`DeathReceiptId`、body record revision、target-open generation、route、focus、P6 composite revision 与 active session 都能在 Preview 和 durable Commit 前从 P1/P6/P11 truth 重验；
2. source 位于 exact P11 BasicCorpse 的 ordinary body-storage stable address，不属于 P21 `Body.Weapon`、`Body.ArmorRobe` 或 `Body.Accessory0`，不是 P9、P14/P31、P5、P6 player source、P17 child Cell、Hotbar、另一 BodyTarget 或 UI-only projection；
3. P20/P21 receipt/profile/history、P11 snapshot、P12 BodyTarget、Catalog 与 P1 source placement 共同证明 root 只能是 canonical `WindTalisman` 或 `BackpackLevel1`，且 parent、唯一 `ChildContainerId`、稳定 `SpatialChildGuid`、formal child type/capacity、slot legality、empty-child provenance、one-layer/no-cycle/no-orphan/no-duplicate-child-owner 与 reverse slot pointer 全部成立；
4. target 必须是用户实际 Drop 的 exact active-P6 formal equipment slot：`WindTalisman` 只对应 `SpatialRing`，`BackpackLevel1` 只对应 `Backpack`。该 slot 必须由 P1/P7/P17 的 canonical Definition compatibility、Owner/Run、ContainerId、SlotIndex、slot semantic、容量、空置状态、P6 revision、route/focus 与 active session 共同验证；不得凭 Widget index、显示文本、图标、当前选择或数组顺序猜测。

下列对象或动作持续不属于 P42：

1. P12 ordinary simple stack、P21 fixed equipment、P20/P19 graph 的 child item、任何未知 complex parent、P17 child 内任何物品、P12 Hidden/Searching root、P9 normal container、P14/P31 WorldDrop、P5 warehouse、P6 player source、Hotbar、another BodyTarget 或 unknown provenance；
2. `Ctrl + 左键`、QuickTransfer、quick-drop、P6 → corpse quick return、Actor direct pickup、距离自动拾取、交互键领取、right-click Take、double-click、Take All、auto equip、auto target、auto child open/switch、auto bind、auto use、Swap、Replacement、Sort、Compact、Merge、Split、`Quantity=N`、`WorldPickupDraft`、`PlayerSplitDraft`、record-to-record transfer 或 parent/child partial transfer；
3. P20 原 `BaseQuick` normal Drag、P41 BaseQuick-only QuickTransfer、P38/P39 corpse-equipment routes、P40 ordinary simple-stack QuickTransfer、P29/P30/P34/P36/P37 ground Ctrl branches、P17 graph construction、P31 Registry、P8 terminal 分类、P13 binding、P15 use 与 Code A authority；
4. 实机运行、PIE、Standalone、真实鼠标键盘、截图、Smoke、Automation、回归、Cook、Package 或最终验收。

## 下半部分：授权执行内容

### 4. 单一授权目标

在不建立第二物品真值、不改变 P11/P12 corpse state machine、P20/P21 deterministic source/history、P17 graph、P31 Registry、P20 原 BaseQuick path、P41 QuickTransfer、P38/P39/P40 现有语义，也不新增任何自动行为的前提下，扩展已 Revealed P12 P20 spatial root 的 existing normal Drag/Drop policy：

1. exact P20 `WindTalisman` graph 仅可经 explicit Drop 进入用户指定的空 `SpatialRing`；exact P20 `BackpackLevel1` graph 仅可经 explicit Drop 进入用户指定的空 `Backpack`；
2. Preview 只建立一个 Owner/Run-scoped candidate，冻结 exact target `ContainerId`、`SlotIndex`、slot semantic、P6/body revision 与必要 BodyTarget/session proof；Commit 不得重扫或 fallback；
3. accepted path 只形成一个 P1/P20 whole-graph relocation、一次 P11/P6 Owner durable replacement 和一次 SaveRecord；
4. 只有 accepted snapshot/replay proof 明确证明相同 parent 加其唯一 ChildContainer closure 已离开 exact P11 source 并进入 exact formal slot 后，才刷新 exact BodyTarget source empty state、P6 equipment projection 和必要 P7 projection；不得创建 WorldDrop、record、ordinal、Actor、new `ItemId`、new `ContainerId`、new child 或第二物品真值；
5. 任一 source、graph、target、identity、session、close/stale、P1 或保存检查失败时，完整零写入并恢复 `BeforeSnapshot`。

### 5. 实现要求

#### 5.1 先完成活动调用链与资格审计

改动前必须审阅并在 Report 中列出：

1. P12 的 BodyTarget open/reveal lifecycle、ordinary body root Cell、normal Drag/Drop、page/focus invalidation、P11/P6 callback 与 source proof；说明 P42 如何复用该生产 Cell 和 existing `NativeOnDrop`，而不是新增 Button、hotkey、pointer handler、Widget、Actor 或尸体写入口；
2. P20 的 canonical corpse spatial provenance、P20 r2/P21 r3 inherited group、profile/receipt/body record identity、exact stable ordinary source container、empty child closure、原 BaseQuick path 与 whole-graph transaction；说明 P42 仅扩展 explicit formal equipment target；
3. P7/P17 与 P19 的 formal `SpatialRing`／`Backpack` target validation、完整图 root relocation、P7 projection、one-layer restriction、capacity 与 exact slot identity；说明 P42 复用其 target rule，不新增 Equip/Unequip、child 或 graph creator；
4. P41 的 shared Ctrl pointer consumption、BaseQuickOnly proof、preview/commit/cancellation 与 stale lifecycle；说明 P42 只处理 normal Drag 的 explicit target，绝不把 P41 改成自动装备；
5. P1/P2/P3/P6/P11 的 whole-graph Move、cross-graph single candidate、accepted snapshot/replay proof、BeforeSnapshot rollback、P11/P6 single Owner durable replacement、P13 reconcile 与 revision/session 调用图；
6. 所有 Widget direct Move/graph mutation、Actor direct pickup、尸体展示缓存写入、right-click Take、double-click、预建 `ItemId`、按 first/last/selection 猜测 target、第二 Repository transaction、第二 save 或 Code A inventory writer；它们不得成为 P42 写入路径。

#### 5.2 共享 normal Drag、source gate 与 exact target freeze

1. P42 必须使用已存在的 P12 root Cell → shared `InventoryDragOperation` → existing normal `NativeOnDrop`／P4/P3/P2/P1 路径。不得新增 P42 专用 DragOperation、按钮、hotkey、Actor click、second pointer handler、second resolver、Widget direct write 或 Code A 写入口。
2. 仅当 payload 已证明为 P42 exact source，且用户 Drop 的 target 是上表所列 compatible formal slot 时，才建立一次 P42 transient candidate。普通左键继续只选择或按 P12 既有规则开始 search；right-click 继续只读详情；`Ctrl + 左键`继续 P41/P39/P40 等既有语义；double-click、Tab、I、Esc、Close、Cancel、scroll、空白区、无 payload Drop、Shift + 1—9、Actor interaction 和 player-side Ctrl 均不得获得 P42 图写入语义。
3. Preview 时必须冻结 Owner/Run、BodyTarget/death receipt、body revision、target-open/page generation、source ContainerId/SlotIndex/root ItemId/child closure、target ContainerId/SlotIndex/formal semantic、P6 composite revision、route/focus/session proof。不得以 UI focus、selection 或“第一个兼容槽”作为后续可变 target。
4. Commit 与 durable Store 写入前必须重新验证所有冻结输入，包括 exact BodyTarget `Open/Revealed`、P20 canonical provenance、parent/child closure、source reverse pointer、target Definition compatibility、target exact empty state、Owner/Run、P6/body revision、Prepared/terminal gate、Host validity 与 active session。
5. BodyTarget close/reopen、focus loss、开始/取消 search、Actor EndPlay、map reload、recovery、terminal/Prepared、receipt/root/container mismatch、payload cancel、Host invalidation、source/target revision stale、parent/child topology stale、target 被占用或 SaveRecord failure 必须立即使 candidate 失效并零写入；不得改投 BaseQuick、另一装备位、P17 child、Hotbar、WorldDrop 或其他位置。

#### 5.3 Canonical complete-graph 与 formal equipment target 验证

1. Store 必须从 canonical Catalog、P1/P6/P11 snapshot、P20/P21 profile/receipt/body record、P12 BodyTarget 与 source stable address 重建并全字段验证 P20 ordinary provenance、parent Definition、source slot、record availability、exact ItemId、reverse slot pointer、parent/child closure 与 empty-child constraint。不得用名称、图标、Cell index、Actor tag、fixture、world coordinate、profile label 或缓存判断资格。
2. `WindTalisman` target 必须是 exact `Prototype.Slot.SpatialRing`；`BackpackLevel1` target 必须是 exact `Prototype.Slot.Backpack`。两者均须是 canonical formal equipment container 的一个正式、可写、空 slot，并按 P1/P7/P17 现有 compatibility 检查通过。不得将普通 Accessory、Weapon、ArmorRobe、BaseQuick、child、Hotbar、P5/P9/P11/P14/P31、保护格或其他 player slot 视为兼容。
3. P42 不扫描、排序或解析自动 target。用户已经指定的 exact target 是唯一 candidate；target full、wrong semantic、wrong definition、wrong Owner/Run、stale/closed/unwritable 或显示为空但 P1 truth 中不为空时必须拒绝。
4. P17/P20 canonical validation 必须保持 parent DefinitionId、stable parent ItemId、ChildContainerId、stable SpatialChildGuid、formal child type/capacity、root placement、child empty、one-layer restriction、无环、无 orphan、无 duplicate child owner、无非法 slot。不得以 parent-only Move、child snapshot、flatten、clone、new graph 或 partial transfer 绕过验证。

#### 5.4 单一 whole-graph 事务、持久化与回滚

1. Preview 成功后只能建立一个 Owner/Run scoped candidate，且只调用 P20/P1 既有 whole-graph root relocation：同一 parent、同一唯一 `ChildContainer` 与完整 closure 从 exact P11 ordinary body source 直接进入用户指定的 exact compatible P6 formal equipment slot。不得先落 BaseQuick 再二次 Move，不得调用 Merge、Split、Swap、Equip、Unequip 或创建 graph。
2. Commit 必须沿既有 P12/P3 → P2 → P1 → P11/P6 durable callback。Store 在任何 durable write 前重新验证 command intent、opened BodyTarget identity、source/target stable address、Owner、Run、body/P6 revision、P20 graph closure、formal slot empty state、active session 与 lifecycle gate。
3. accepted candidate 中必须保持 parent、ChildContainer、所有 `ItemId`／`ContainerId`、capacity、placement order、stable `SpatialChildGuid` 与 provenance 身份不变；唯一合法位置变化是 graph root 从 exact P11 ordinary body source 移到 exact P6 formal equipment slot。不得 clone、flatten、new ItemId、new ContainerId、new child、parent-only move、partial commit 或 child snapshot。
4. P11 source 与 P6 target 必须在同一 Owner durable replacement 中共同提交，且只有一次 SaveRecord。仅当 accepted snapshot/replay proof 完整成立，才刷新 exact BodyTarget source、P6 equipment projection 和必要 P7 projection。不得预清 source、预写 P6、预删 child、预刷 Code A Actor 或在 save 后补写另一侧。
5. P13 只按既有 accepted commit reconcile；不得自动 Bind、Use、Equip、打开 child 或复制 binding。P8 仍只结算当时 P6 player graph，并只丢弃 P11 residual；P42 不改变尸体死亡、reveal、receipt、终局或 recovery。
6. candidate、canonical gate、graph validation、target validation、P1 relocation、P13 reconcile、projection 前检查或 SaveRecord 任一失败时，必须完整恢复 BeforeSnapshot：P11 source parent/child closure、P6 target、BodyTarget visibility/open state、P7 projection、selection 与无关 body/world record 均保持未变，且不得遗留 phantom empty slot、phantom pickup、图副本或 transient item copy。

#### 5.5 非回归、终局与权威边界

1. P20 complete graph 的 normal Drag → empty BaseQuick 保持完全不变；P42 只增加 explicit compatible equipment target，不收窄、重定向或自动触发原路径。空间 parent 或其 child 从 P6 回写 BasicCorpse 仍必须拒绝。
2. P41 仍只在 Ctrl gesture 下将尸体空间 parent QuickTransfer 到 first legal empty BaseQuick，绝不自动装备、不扫描 `SpatialRing`／`Backpack`、不进入 P17 child。P42 的 normal Drag 不得触发 P41 intent。
3. P40 ordinary simple-stack、P38/P39/P21 corpse-equipment、P29/P30/P34/P36/P37 WorldDrop、P26—P28 explicit quantity、P31 Registry、P17 graph construction、P5/P6 bridge、P8 terminal/recovery、P13 binding、P15 use、P20/P21 source/history 与 Code A authority 均保持既有产品语义。
4. Code A 只可维持既有 production BodyTarget lifecycle forwarding和 UI/map 边缘；不得拥有 Item、Container、Quantity、P11/P6、BodyTarget、Loot、search、terminal 或 player-equipment durable authority。

### 6. 允许的改动范围

仅允许在当前 Code B 中最小修改：

- P12 已有 body-aware source proof、shared normal Drag/Drop preview/commit、P11/P6 durable transaction 与 projection；
- P20/P19/P7/P17 已有 complete-graph validation、formal equipment target compatibility、whole-graph relocation、rollback 与 accepted proof，仅用于 P42 exact P12 P20 source；
- P41/P38 的 transient proof、source lifecycle、cancellation 与 revision/session validation，仅用于保持 P42 与既有 input branch 隔离；
- P1/P2/P3/P6/P11/P13/P17 的必要声明、candidate proof、rollback 或调用签名兼容，前提是不改变已有产品语义；
- 必要的 production BodyTarget lifecycle forwarding，只能作为 Code B gate 的只读转发；
- `PROJECT.md`、`PROJECT_INFO_CARD.md`、本任务 Prompt 归档与本任务 Report。

禁止新建项目、版本线、Fix、第二 P11/P6、Widget inventory、Code A mirror、fixture、假 ItemId、clone、双写、存档重置或历史数据改写。禁止修改 Code A 功能逻辑、P12 search state machine、P20/P21 deterministic source/history、P17 graph construction、P31 schema/registry、P8 receipt/terminal product logic、P5/P6 bridge 或任何测试文件。

### 7. 明确不在本任务内

- 不将 P21 equipment、P12 ordinary simple stack、P20/P19 child item、P14/P31 WorldDrop、warehouse、ordinary container、player source 或任何 other provenance 接入 P42。
- 不实现 Ctrl QuickTransfer、快捷丢弃、P6 → corpse quick return、自动装备、equipment target scan、auto child open/switch、fallback、Take All、right-click Take、double-click、Actor direct pickup、距离自动拾取、交互键领取、Swap、Replacement、Sort、Compact、bind、use、装备数值、战斗效果、HUD 接管、网络或多人。
- 不改变 P20 BaseQuick normal Drag、P41 QuickTransfer、P38/P39/P40 或 P29/P30/P34/P36/P37 现有 Ctrl semantics、P13/P15、P17/P19、P26—P28、P31、P5/P6/P8、搜索、敌人、地图、战斗、生命、死亡、撤离、商店、经济或制作。
- 不启动产品、PIE、Standalone、真实鼠标键盘输入、截图、Smoke、Automation、回归、试玩、Cook、Package 或最终验收。
- 不在回传 Report 前自动开始 P43、任意 Fix 或 F。

### 8. P 阶段静态审查与编译

完成后只执行以下检查：

1. 审查 P42 是否复用 P12 现有 normal Drag/Drop 和唯一 P1/P11/P6 transaction；确认没有专用 input/UI/Actor 写入旁路，且 `Ctrl + 左键`、right-click、double-click 和 player-side input 的产品语义未改变。
2. 审查 source 只接受 current opened/revealed/exact P12 P20 `WindTalisman`／`BackpackLevel1` complete graph，并逐项复核 Owner/Run/BodyTarget/death receipt/profile/history/body revision/source container-slot/root/child closure/page/focus/P6 revision；P21 equipment、simple stack、child、P19 world、Hidden/Searching、stale/close/terminal 均零写入。
3. 审查 P17/P20 complete graph closure：唯一 child、formal type/capacity、empty-child provenance、一层无嵌套、无环、无 orphan、无 duplicate owner、stable `SpatialChildGuid` 与 parent/child identity 均保持。确认 P42 未重掷、补料或改写 P20/P21 profile/history。
4. 审查 explicit formal target：WindTalisman 只允许 exact empty `SpatialRing`，BackpackLevel1 只允许 exact empty `Backpack`；确认没有 candidate scan、BaseQuick fallback、child/Hotbar/other equipment fallback、auto-equip 或 target guessing。
5. 审查 candidate 只使用一个 P1/P20 whole-graph relocation；确认没有 Merge、Split、`Quantity=0`、partial graph、new ItemId、Container、receipt、record、ordinal、Actor、child 或第二物品真值。
6. 审查 P11/P6 仅以一个 Owner durable replacement 和一次 SaveRecord 共同提交；确认 P11 source 只在 accepted proof 后更新、P13 reconcile、BeforeSnapshot rollback、P8 terminal/recovery、selection/scroll/stable SlotIndex 与 P17 graph 均保持。
7. 审查 P20 original BaseQuick Drag、P41 BaseQuick-only Ctrl、P38/P39/P40、P29/P30/P34/P36/P37、P31 Registry、P5/P6/P8/P13/P15 与 Code A authority 均无产品语义回归。
8. 编译 Editor：

       "C:\\Program Files\\Epic Games\\UE_5.8\\Engine\\Build\\BatchFiles\\Build.bat" demo_mapEditor Win64 Development "C:\\AIDev\\shanmen-ue\\Dev.D.UE.0.0.9B\\demo_map.uproject" -WaitMutex -NoHotReload

9. 编译 Game：

       "C:\\Program Files\\Epic Games\\UE_5.8\\Engine\\Build\\BatchFiles\\Build.bat" demo_map Win64 Development "C:\\AIDev\\shanmen-ue\\Dev.D.UE.0.0.9B\\demo_map.uproject" -WaitMutex -NoHotReload

若编译失败，只修复本任务引入的 P12 spatial normal-drag routing、source proof、formal-equipment target validation、P11/P6 transaction、projection、rollback、include 或签名问题；若必须扩大到本任务以外，停止受影响部分并如实报告。

### 9. Report 与完成信号

生成 `Dev.D.UE.0.0.9B.P42.0.r0_report.md`，保存至：

    C:\\AIDev\\shanmen-ue\\Dev.D.UE.0.0.9B\\Docs\\Report

Report 必须简洁、可审计地列出：

1. 本轮新增、修改、未修改的每个文件及职责；
2. P12 ordinary spatial source Cell 到 P42 shared normal Drag/Drop 的完整调用图，以及没有新增 pointer、Widget、Actor、second resolver 或 Code A 写入口的证据；
3. P20 r2/P21 r3 inherited spatial provenance、actual stable ordinary source slot、BodyTarget/death receipt/profile identity、完整 parent/child closure，以及为什么 P20/P21 materialization/history/determinism 未被改写；
4. exact source gate、Reveal/Open lifecycle、Owner/Run/revision/session proof、用户指定 formal target freeze 与 zero-write cancellation/stale policy；
5. WindTalisman → SpatialRing、BackpackLevel1 → Backpack 的 canonical compatibility、exact empty-slot validation、无 target scan/fallback/auto-equip/child-entry 的证据；
6. single P1/P20 whole-graph relocation、single P11/P6 durable replacement、source/target parent-child identity 保持、P13/P8、rollback、selection/scroll/stable SlotIndex 与无 WorldDrop/ordinal/Actor write 的证据；
7. P20 BaseQuick normal Drag、P41、P38/P39/P40、P29/P30/P34/P36/P37、P31、P5/P6/P8/P13/P15 与 Code A authority 的非回归结论；
8. 两个编译命令、目标、原生 exit code 与关键结果；
9. 所有未执行的 F 阶段真实验证，至少包括：P20/P21 future corpse materialize/reveal；actual canonical WindTalisman／BackpackLevel1 normal Drag 至 compatible empty SpatialRing／Backpack，以及原 BaseQuick normal Drag；full/wrong/occupied slot、wrong parent/child/simple/equipment/Hidden/Searching source；BodyTarget/death receipt/profile/Owner/Run/P6 revision stale；Prepared/terminal/Host invalid/SaveRecord failure；P41 QuickTransfer、P12 normal Move/Merge/Swap、P21/P38/P39、P40、P29/P30/P34/P36/P37 Ctrl branches、P31 other-record isolation、P8 terminal/recovery、真实鼠标键盘、PIE、Standalone、截图、Smoke、Automation、回归、Cook 与 Package。

仅当 P42 P12 P20 complete-graph explicit compatible-equipment Drop 静态闭合、P12/P20/P21 与 P17/P19/P29—P31/P38—P41/P8 边界保持，且 Editor 与 Game 均以 native exit code `0` 完成时，使用：

    READY_FOR_P43_PLANNING

若当前范围内仍有可修复问题，使用：

    NEEDS_P42_REWORK

若现有 P12/P11/P6 transaction 或 P7 formal equipment target 无法在不创建第二输入路径、第二保存、第二物品真值、自动装备、改写 P20/P21 deterministic history 或扩展 Code A 权威的前提下支持本项，使用：

    NEEDS_PLANNER_DECISION
    BLOCKED

完成后不得自动开始 P43、Fix 或 F。向策划 Chat 回传并附带且只附带本次同名 Report；正文首行使用：

    [CSEMI:REPORT_SENT] {"task_id":"Dev.D.UE.0.0.9B.P42.0.r0","file":"Dev.D.UE.0.0.9B.P42.0.r0_report.md"}
