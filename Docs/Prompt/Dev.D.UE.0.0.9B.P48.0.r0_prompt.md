# Dev.D.UE.0.0.9B.P48.0.r0

## 任务身份

- 项目：Dev.D.UE.0.0.9B；继续使用同一活动工程，不新建项目。
- 阶段：主线 P48——已打开、已揭示 P10 BasicCache 完整空间图的显式兼容装备位直达。
- 任务编号：Dev.D.UE.0.0.9B.P48.0.r0。
- 前置：已接受 0.0.9B.P1—P47 与 0.0.9BFix.P1—P4。Fix 只用于已确认、已验收功能的缺陷修复；P48 是新增主线功能，不是 Fix。
- 执行文件：Dev.D.UE.0.0.9B.P48.0.r0_prompt.md。
- 报告文件：Dev.D.UE.0.0.9B.P48.0.r0_report.md。
- 活动工程根：C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B。
- 活动工程：C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject。
- Report 必须生成到：C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Docs\Report\Dev.D.UE.0.0.9B.P48.0.r0_report.md，并仅携带该同名 Report 回传策划 Chat。
- 任务性质：P 阶段只做实现、静态审查和代码编译。不得启动产品、PIE、Standalone、真实鼠标键盘验证、截图、Smoke、Automation、回归、Cook、Package 或 F 阶段测试；不得自动开始 P49、任意 Fix 或 F。

## 上半部分：只读项目裁决、现状与边界

### 1. 当前唯一有效基线

唯一有效依据是当前 0.0.9B、活动工程和已接受任务链。所有 0.2、V2、V3、I、IPF、历史页面壳、旧 CTA、旧库存和旧物品规则均已过时；不得读取、采用、恢复或以其决定实现、验收或范围。

Code B P1 Repository 与既有 durable Store 是唯一可变物品真值。每件物品始终只有一个真实 ItemId、一个真实父位置和一条权威事务链。Widget、Cell、Presenter、DragOperation、Workspace Context、NormalContainerTarget、空间 child 投影、WorldDrop Actor、地图放置适配层和 Code A 都只能持有只读投影、选择或瞬时意图；不得持有第二库存、可写数量副本、预建 ItemId、平行容器背包、Actor-first 写入或 A/B 双写。

P9/P10 已建立唯一生产普通容器 M01.CodeBNormalContainer.BasicCache.01 和其 exact BasicCache Run-local record。P18 已把未来首次物质化的 BasicCache 升级为确定性的 r2 Profile：保留 SpiritDust/IronShard 主材料组，并以一次低频 Optional.SpatialUtility gate 生成至多一个 WindTalisman 或 BackpackLevel1 parent，连同其唯一、空的 P17 ChildContainer。已物质化 record 必须继续按其 receipt、ProfileId、ProfileVersion、ProfileDigest、AlgorithmVersion 与 ResultDigest 保持原样；P48 不得 reroll、补料、迁移或改写这个历史。

P18 已允许已 Revealed 的 BasicCache 完整空间图经 P10 普通 Drag/P1 composite transaction 整体移入明确空的 BaseQuick。P47 已为同一 P10 source 加入 Ctrl + 左键的 BaseQuick-only 快捷拾回。P7、P17 与 P19 已确立完整空间图以普通 Drag 进入用户明确指定、兼容且为空的 formal equipment slot 的既有权威语义；P42 仅为尸体来源接入过该既有目标规则，不能成为 P48 的 source provenance 或写入口。

因此 P48 只补齐一个真实、明确的交互缺口：玩家已打开并揭示 BasicCache 后，可把 P18 canonical spatial parent 以普通 Drag/Drop 直接放入用户明确指向的、空且兼容的空间装备位。P18 原有 BaseQuick normal Drag 与 P47 BaseQuick-only Ctrl 语义均保持完全有效。

P17 的一层无嵌套限制持续有效：空间 parent、其 ChildContainer 或任何后代不得装入另一个空间 child，也不得形成自身或祖先回边。P48 绝不让完整空间图进入 P17 child、Hotbar 或任何非 formal compatible target。

### 2. P48 产品裁决

当玩家已经通过既有 P10 流程主动打开 identity-valid 的唯一 BasicCache，且其中一个 ordinary root 是已 Revealed 的 P18 canonical complete graph 时，玩家可从该 root Cell 发起既有 normal DragOperation，并仅把它 Drop 到其 Definition 对应的、用户明确指定且为空的 P6 formal equipment slot。

| Exact P18 source graph | 用户明确 Drop 的唯一新增 target | 权威事务 |
| --- | --- | --- |
| WindTalisman parent + 唯一 formal empty child closure | 空、正式、Definition-compatible Prototype.Slot.SpatialRing | 一个 P1/P18 whole-graph relocation；同一 P9/P6 Owner durable replacement 与一次 SaveRecord |
| BackpackLevel1 parent + 唯一 formal empty child closure | 空、正式、Definition-compatible Prototype.Slot.Backpack | 一个 P1/P18 whole-graph relocation；同一 P9/P6 Owner durable replacement 与一次 SaveRecord |

这只是用户明确拖拽的目的地扩展，不是自动装备、目标扫描、快捷领取、快速丢弃、按钮领取或新的空间道具系统。目标由用户的真实 Drop 指定；Preview 与 Commit 只重验同一个 ContainerId + SlotIndex，不得因该格满、stale、不兼容或保存失败而改投 BaseQuick、另一装备位、P17 child、Hotbar、地面、尸体或其他位置。

P48 不改变 P18 当前拖至空 BaseQuick 的 normal Drag 语义，也不让 P47 的 Ctrl + 左键自动装备。空间 parent 仍不得进入任何 P17 child；P17 一层限制继续禁止空间嵌套。

### 3. 严格范围与持续排除

P48 source 只接受同时满足以下全部条件的完整图：

1. 当前 Workspace 为既有 P10 production Host；exact NormalContainerTarget 已 Open、已 Revealed、identity-valid，仍属于 active M01 BasicCache route、focus、target-open generation 与 active session，且无 active open/search action。OwnerId、RunInstanceId、SearchTargetId、DefinitionId、P9 receipt、record revision、P6 composite revision、route、focus 与 active session 都能在 Preview 和 durable Commit 前从 P1/P6/P9 truth 重验；
2. source 位于 exact P9 RunLocalNormalContainerRecord 的 ordinary stable address，不属于 P6 player source、P11/P12 corpse、P14/P31 WorldDrop、P5 warehouse、P17 child Cell、Hotbar、another NormalContainer 或 UI-only projection；
3. P18 profile/receipt/history、P9 snapshot、P10 NormalContainerTarget、Catalog 与 P1 source placement 共同证明 root 只能是 canonical WindTalisman 或 BackpackLevel1，且 parent、唯一 ChildContainerId、稳定 SpatialChildGuid、formal child type/capacity、slot legality、empty-child provenance、一层限制、无环、无 orphan、无 duplicate child owner 与 reverse slot pointer 全部成立；
4. target 必须是用户实际 Drop 的 exact active-P6 formal equipment slot：WindTalisman 只对应 SpatialRing，BackpackLevel1 只对应 Backpack。该 slot 必须由 P1/P7/P17 的 canonical Definition compatibility、Owner/Run、ContainerId、SlotIndex、slot semantic、容量、空置状态、P6 revision、route/focus 与 active session 共同验证；不得凭 Widget index、显示文本、图标、当前选择或数组顺序猜测。

下列对象或动作持续不属于 P48：

1. P10 ordinary simple stack、P18 graph 的 child item、任何未知 complex parent、P21 equipment、P11/P12 corpse、P14/P31 WorldDrop、P5 warehouse、P6 player source、Hotbar、another NormalContainer、P17 child 内任何物品、Hidden/Searching root 或 unknown provenance；
2. Ctrl + 左键、QuickTransfer、quick-drop、P6 → BasicCache 快捷回存、Actor direct pickup、距离自动拾取、交互键领取、right-click Take、double-click、Take All、auto equip、auto target、auto child open/switch、auto bind、auto use、Swap、Replacement、Sort、Compact、Merge、Split、Quantity=N、WorldPickupDraft、PlayerSplitDraft、record-to-record transfer 或 parent/child partial transfer；
3. P18 原 BaseQuick normal Drag、P47 BaseQuick-only QuickTransfer、P46 BasicCache simple-stack Ctrl、P42 corpse spatial equipment path、P38/P39/P40 corpse routes、P29/P30/P34/P36/P37 WorldDrop routes、P41 corpse complete-graph Ctrl、P31 Registry、P8 terminal、P13 binding、P15 use 与 Code A authority；
4. 实机运行、PIE、Standalone、真实鼠标键盘、截图、Smoke、Automation、回归、Cook、Package 或最终验收。

## 下半部分：授权执行内容

### 4. 单一授权目标

在不建立第二物品真值、不改变 P9/P10 normal-container state machine、P18 deterministic materialization/history、P17 graph、P31 Registry、P18 原 BaseQuick path、P47 QuickTransfer、P46 现有语义，也不新增任何自动行为的前提下，扩展已 Revealed P10 P18 spatial root 的 existing normal Drag/Drop policy：

1. exact P18 WindTalisman graph 仅可经 explicit Drop 进入用户指定的空 SpatialRing；exact P18 BackpackLevel1 graph 仅可经 explicit Drop 进入用户指定的空 Backpack；
2. Preview 只建立一个 Owner/Run-scoped candidate，冻结 exact target ContainerId、SlotIndex、slot semantic、P6/P9 revision 与必要 NormalContainerTarget/session proof；Commit 不得重扫或 fallback；
3. accepted path 只形成一个 P1/P18 whole-graph relocation、一次 P9/P6 Owner durable replacement 和一次 SaveRecord；
4. 只有 accepted snapshot/replay proof 明确证明相同 parent 加其唯一 ChildContainer closure 已离开 exact P9 source 并进入 exact formal slot 后，才刷新 exact NormalContainerTarget source empty state、P6 equipment projection 和必要 P7 projection；不得创建 WorldDrop、record、ordinal、Actor、new ItemId、new ContainerId、new child 或第二物品真值；
5. 任一 source、graph、target、identity、session、close/stale、P1 或保存检查失败时，完整零写入并恢复 BeforeSnapshot。

### 5. 实现要求

#### 5.1 先完成活动调用链与资格审计

改动前必须审阅并在 Report 中列出：

1. P10 NormalContainerTarget 的 open/reveal lifecycle、ordinary BasicCache root Cell、normal Drag/Drop、page/focus invalidation、P9/P6 callback 与 source proof；说明 P48 如何复用该生产 Cell 和 existing NativeOnDrop，而不是新增 Button、hotkey、pointer handler、Widget、Actor 或容器写入口；
2. P18 的 r2 BasicCache optional spatial source、Profile/receipt/digest identity、parent-child graph materialization、原 BaseQuick normal Drag 与 whole-graph transaction；说明 P48 仅扩展 explicit formal equipment target，不会改写 r1/r2 历史、gate、weight、candidate、ItemId 或 ChildContainer；
3. P7/P17/P19 的 formal SpatialRing/Backpack target validation、完整图 root relocation、P7 projection、一层无嵌套限制、capacity 与 exact slot identity；说明 P48 复用其 target rule，不新增 Equip/Unequip、child 或 graph creator；
4. P47 的 shared Ctrl pointer consumption、BaseQuickOnly proof、preview/commit/cancellation 与 stale lifecycle；说明 P48 只处理 normal Drag 的 explicit target，绝不把 P47 改成自动装备；
5. P1/P2/P3/P6/P9 的 whole-graph Move、cross-graph single candidate、accepted snapshot/replay proof、BeforeSnapshot rollback、P9/P6 single Owner durable replacement、P13 reconcile 与 revision/session 调用图；
6. 所有 Widget direct Move/graph mutation、Actor direct pickup、container display-cache write、right-click Take、double-click、预建 ItemId、按 first/last/selection 猜测 target、第二 Repository transaction、第二 save 或 Code A inventory writer；它们不得成为 P48 写入路径。

#### 5.2 共享 normal Drag、source gate 与 exact target freeze

1. P48 必须使用已存在的 P10 root Cell → shared InventoryDragOperation → existing normal NativeOnDrop／P4/P3/P2/P1 路径。不得新增 P48 专用 DragOperation、按钮、hotkey、Actor click、second pointer handler、second resolver、Widget direct write 或 Code A 写入口。
2. 仅当 payload 已证明为 P48 exact source，且用户 Drop 的 target 是上表所列 compatible formal slot 时，才建立一次 P48 transient candidate。普通左键继续只选择或按 P10 既有规则开始 search；right-click 继续只读详情；Ctrl + 左键继续 P46/P47 等既有语义；double-click、Tab、I、Esc、Close、Cancel、scroll、空白区、无 payload Drop、Shift + 1—9、Actor interaction 和 player-side Ctrl 均不得获得 P48 图写入语义。
3. Preview 时必须冻结 Owner/Run、NormalContainerTarget/SearchTargetId/receipt、P9 record revision、target-open/page generation、source ContainerId/SlotIndex/root ItemId/child closure、target ContainerId/SlotIndex/formal semantic、P6 composite revision、route/focus/session proof。不得以 UI focus、selection 或“第一个兼容槽”作为后续可变 target。
4. Commit 与 durable Store 写入前必须重新验证所有冻结输入，包括 exact NormalContainerTarget Open/Revealed、P18 canonical provenance、parent/child closure、source reverse pointer、target Definition compatibility、target exact empty state、Owner/Run、P6/P9 revision、Prepared/terminal gate、Host validity 与 active session。
5. Target close/reopen、focus loss、开始/取消 search、Actor EndPlay、map reload、recovery、terminal/Prepared、receipt/root/container mismatch、payload cancel、Host invalidation、source/target revision stale、parent/child topology stale、target 被占用或 SaveRecord failure 必须立即使 candidate 失效并零写入；不得改投 BaseQuick、另一装备位、P17 child、Hotbar、WorldDrop 或其他位置。

#### 5.3 Canonical complete-graph 与 formal equipment target 验证

1. Store 必须从 canonical Catalog、P1/P6/P9 snapshot、P18 profile/receipt/normal record、P10 NormalContainerTarget 与 source stable address 重建并全字段验证 P18 ordinary provenance、parent Definition、source slot、record availability、exact ItemId、reverse slot pointer、parent/child closure 与 empty-child constraint。不得用名称、图标、Cell index、Actor tag、fixture、world coordinate、profile label 或缓存判断资格。
2. WindTalisman target 必须是 exact Prototype.Slot.SpatialRing；BackpackLevel1 target 必须是 exact Prototype.Slot.Backpack。两者均须是 canonical formal equipment container 的一个正式、可写、空 slot，并按 P1/P7/P17 现有 compatibility 检查通过。不得将普通 Accessory、Weapon、ArmorRobe、BaseQuick、child、Hotbar、P5/P9/P11/P14/P31、保护格或其他 player slot 视为兼容。
3. P48 不扫描、排序或解析自动 target。用户已经指定的 exact target 是唯一 candidate；target full、wrong semantic、wrong definition、wrong Owner/Run、stale/closed/unwritable 或显示为空但 P1 truth 中不为空时必须拒绝。
4. P17/P18 canonical validation 必须保持 parent DefinitionId、stable parent ItemId、ChildContainerId、stable SpatialChildGuid、formal child type/capacity、root placement、child empty、一层无嵌套、无环、无 orphan、无 duplicate child owner、无非法 slot。不得以 parent-only Move、child snapshot、flatten、clone、new graph 或 partial transfer 绕过验证。

#### 5.4 单一 whole-graph 事务、持久化与回滚

1. Preview 成功后只能建立一个 Owner/Run scoped candidate，且只调用 P18/P1 既有 whole-graph root relocation：同一 parent、同一唯一 ChildContainer 与完整 closure 从 exact P9 BasicCache ordinary source 直接进入用户指定的 exact compatible P6 formal equipment slot。不得先落 BaseQuick 再二次 Move，不得调用 Merge、Split、Swap、Equip、Unequip 或创建 graph。
2. Commit 必须沿既有 P10/P3 → P2 → P1 → P9/P6 durable callback。Store 在任何 durable write 前重新验证 command intent、opened NormalContainerTarget identity、source/target stable address、Owner、Run、P9/P6 revision、P18 graph closure、formal slot empty state、active session 与 lifecycle gate。
3. accepted candidate 中必须保持 parent、ChildContainer、所有 ItemId/ContainerId、capacity、placement order、stable SpatialChildGuid 与 provenance 身份不变；唯一合法位置变化是 graph root 从 exact P9 ordinary source 移到 exact P6 formal equipment slot。不得 clone、flatten、new ItemId、new ContainerId、new child、parent-only move、partial commit 或 child snapshot。
4. P9 source 与 P6 target 必须在同一 Owner durable replacement 中共同提交，且只有一次 SaveRecord。仅当 accepted snapshot/replay proof 完整成立，才刷新 exact NormalContainerTarget source、P6 equipment projection 和必要 P7 projection。不得预清 source、预写 P6、预删 child、预刷 Code A Actor 或在 save 后补写另一侧。
5. P13 只按既有 accepted commit reconcile；不得自动 Bind、Use、Equip、打开 child 或复制 binding。P8 仍只结算当时 P6 player graph，并只丢弃 P9 residual；P48 不改变 BasicCache 开启、搜索、receipt、终局或 recovery。
6. candidate、canonical gate、graph validation、target validation、P1 relocation、P13 reconcile、projection 前检查或 SaveRecord 任一失败时，必须完整恢复 BeforeSnapshot：P9 source parent/child closure、P6 target、NormalContainerTarget visibility/open state、P7 projection、selection 与无关 normal/world record 均保持未变，且不得遗留 phantom empty slot、phantom pickup、图副本或 transient item copy。

#### 5.5 非回归、终局与权威边界

1. P18 complete graph 的 normal Drag → empty BaseQuick 保持完全不变；P48 只增加 explicit compatible equipment target，不收窄、重定向或自动触发原路径。空间 parent 或其 child 从 P6 回写 BasicCache 仍必须拒绝。
2. P47 仍只在 Ctrl gesture 下将 BasicCache 空间 parent QuickTransfer 到 first legal empty BaseQuick，绝不自动装备、不扫描 SpatialRing/Backpack、不进入 P17 child。P48 的 normal Drag 不得触发 P47 intent。
3. P46 ordinary simple-stack、P42 corpse spatial path、P38/P39/P40 corpse routes、P29/P30/P34/P36/P37 WorldDrop、P41 corpse complete graph、P26—P28 explicit quantity、P31 Registry、P17 graph construction、P5/P6 bridge、P8 terminal/recovery、P13 binding、P15 use、P18 materialization/history 与 Code A authority均保持既有产品语义。
4. Code A 只可维持既有 production NormalContainerTarget lifecycle forwarding 和 UI/map 边缘；不得拥有 Item、Container、Quantity、P9/P6、NormalContainerTarget、Loot、search、terminal 或 player-equipment durable authority。

### 6. 允许的改动范围

仅允许在当前 Code B 中最小修改：

- P10 已有 normal-container source proof、shared normal Drag/Drop preview/commit、P9/P6 durable transaction 与 projection；
- P18/P19/P7/P17 已有 complete-graph validation、formal equipment target compatibility、whole-graph relocation、rollback 与 accepted proof，仅用于 P48 exact P10 P18 source；
- P47/P46 的 transient proof、source lifecycle、cancellation 与 revision/session validation，仅用于保持 P48 与既有 input branch 隔离；
- P1/P2/P3/P6/P9/P13/P17 的必要声明、candidate proof、rollback 或调用签名兼容，前提是不改变已有产品语义；
- 必要的 production NormalContainer lifecycle forwarding，只能作为 Code B gate 的只读转发；
- PROJECT.md、PROJECT_INFO_CARD.md、本任务 Prompt 归档与本任务 Report。

禁止新建项目、版本线、Fix、第二 P9/P6、Widget inventory、Code A mirror、fixture、假 ItemId、clone、双写、存档重置或历史数据改写。禁止修改 Code A 功能逻辑、P10 search state machine、P18 profile/history、P17 graph construction、P31 schema/registry、P8 receipt/terminal product logic、P5/P6 bridge 或任何测试文件。

### 7. 明确不在本任务内

- 不将 P10 simple stack、P18 child item、P11/P12 corpse、P21 equipment、P14/P31 WorldDrop、warehouse、player source、any other NormalContainer 或任何 other provenance 接入 P48。
- 不实现 Ctrl QuickTransfer、快捷丢弃、P6 → BasicCache quick return、自动装备、equipment target scan、auto child open/switch、fallback、Take All、right-click Take、double-click、Actor direct pickup、距离自动拾取、交互键领取、Swap、Replacement、Sort、Compact、Bind、Use、装备数值、战斗效果、HUD 接管、网络或多人。
- 不改变 P18 BaseQuick normal Drag、P47 QuickTransfer、P46、P42、P38/P39/P40、P29/P30/P34/P36/P37、P41 现有 Ctrl semantics、P13/P15、P17/P19、P26—P28、P31、P5/P6/P8、搜索、尸体、敌人、地图、战斗、生命、死亡、撤离、商店、经济或制作。
- 不启动产品、PIE、Standalone、真实鼠标键盘输入、截图、Smoke、Automation、回归、试玩、Cook、Package 或最终验收。
- 不在回传 Report 前自动开始 P49、任意 Fix 或 F。

### 8. P 阶段静态审查与编译

完成后只执行以下检查：

1. 审查 P48 是否复用 P10 现有 normal Drag/Drop 和唯一 P1/P9/P6 transaction；确认没有专用 input/UI/Actor 写入旁路，且 Ctrl + 左键、right-click、double-click 和 player-side input 的产品语义未改变。
2. 审查 source 只接受 current opened/revealed/exact P10 P18 WindTalisman/BackpackLevel1 complete graph，并逐项复核 Owner/Run/NormalContainerTarget/SearchTargetId/receipt/profile/history/P9 revision/source container-slot/root/child closure/page/focus/P6 revision；simple stack、child、corpse、WorldDrop、Hidden/Searching、stale/close/terminal 均零写入。
3. 审查 P17/P18 complete graph closure：唯一 child、formal type/capacity、empty-child provenance、一层无嵌套、无环、无 orphan、无 duplicate owner、stable SpatialChildGuid 与 parent/child identity 均保持。确认 P48 未重掷、补料或改写 P18 profile/history。
4. 审查 explicit formal target：WindTalisman 只允许 exact empty SpatialRing，BackpackLevel1 只允许 exact empty Backpack；确认没有 candidate scan、BaseQuick fallback、child/Hotbar/other equipment fallback、auto-equip 或 target guessing。
5. 审查 candidate 只使用一个 P1/P18 whole-graph relocation；确认没有 Merge、Split、Quantity=0、partial graph、new ItemId、Container、receipt、record、ordinal、Actor、child 或第二物品真值。
6. 审查 P9/P6 仅以一个 Owner durable replacement 和一次 SaveRecord 共同提交；确认 P9 source 只在 accepted proof 后更新、P13 reconcile、BeforeSnapshot rollback、P8 terminal/recovery、selection/scroll/stable SlotIndex 与 P17 graph 均保持。
7. 审查 P18 original BaseQuick Drag、P47 BaseQuick-only Ctrl、P46、P42、P38/P39/P40、P29/P30/P34/P36/P37、P41、P31 Registry、P5/P6/P8/P13/P15 与 Code A authority 均无产品语义回归。
8. 执行 git diff --check。
9. 编译 Editor：

       "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_mapEditor Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload

10. 编译 Game：

       "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_map Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload

若编译失败，只修复本任务引入的 P10 spatial normal-drag routing、source proof、formal-equipment target validation、P9/P6 transaction、projection、rollback、include 或签名问题；若必须扩大到本任务以外，停止受影响部分并如实报告。

### 9. Report 与完成信号

生成 Dev.D.UE.0.0.9B.P48.0.r0_report.md，保存至：

    C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Docs\Report

Report 必须简洁、可审计地列出：

1. 本轮新增、修改、未修改的每个文件及职责；
2. P10 ordinary spatial source Cell 到 P48 shared normal Drag/Drop 的完整调用图，以及没有新增 pointer、Widget、Actor、second resolver 或 Code A 写入口的证据；
3. P18 BasicCache r2 spatial provenance、actual stable ordinary source slot、NormalContainerTarget/SearchTargetId/receipt/profile identity、完整 parent/child closure，以及为什么 P18 materialization/history/determinism 未被改写；
4. exact source gate、Reveal/Open lifecycle、Owner/Run/revision/session proof、用户指定 formal target freeze 与 zero-write cancellation/stale policy；
5. WindTalisman → SpatialRing、BackpackLevel1 → Backpack 的 canonical compatibility、exact empty-slot validation、无 target scan/fallback/auto-equip/child-entry 的证据；
6. single P1/P18 whole-graph relocation、single P9/P6 durable replacement、source/target parent-child identity 保持、P13/P8、rollback、selection/scroll/stable SlotIndex 与无 WorldDrop/ordinal/Actor write 的证据；
7. P18 BaseQuick normal Drag、P47、P46、P42、P38/P39/P40、P29/P30/P34/P36/P37、P41、P31、P5/P6/P8/P13/P15 与 Code A authority 的非回归结论；
8. git diff --check 结果、两个编译命令、目标、原生 exit code 与关键结果；
9. 所有未执行的 F 阶段真实验证，至少包括：BasicCache r2 materialize、optional spatial hit/no-hit、open/reveal；actual canonical WindTalisman/BackpackLevel1 normal Drag 至 compatible empty SpatialRing/Backpack，以及原 BaseQuick normal Drag；full/wrong/occupied slot、wrong parent/child/simple/equipment/Hidden/Searching source；NormalContainerTarget/SearchTargetId/receipt/profile/Owner/Run/P9/P6 revision stale；target close/reopen、P10 action/search、Prepared/terminal/Host invalid/SaveRecord failure；P47 QuickTransfer、P10 normal Move/Merge/Swap、P18 normal graph Drag、P46、P42、P38/P39/P40、P29/P30/P34/P36/P37、P41、P31 other-record isolation、P8 terminal/recovery、真实鼠标键盘、PIE、Standalone、截图、Smoke、Automation、回归、Cook 与 Package。

仅当 P48 P10 P18 complete-graph explicit compatible-equipment Drop 静态闭合、P9/P10/P18 与 P17/P19/P29—P31/P41/P46/P47/P8 边界保持，且 Editor 与 Game 均以 native exit code 0 完成时，使用：

    READY_FOR_P49_PLANNING

若当前范围内仍有可修复问题，使用：

    NEEDS_P48_REWORK

若现有 P10/P9/P6 transaction 或 P7 formal equipment target 无法在不创建第二输入路径、第二保存、第二物品真值、自动装备、改写 P18 deterministic history 或扩展 Code A 权威的前提下支持本项，使用：

    NEEDS_PLANNER_DECISION
    BLOCKED

完成后不得自动开始 P49、Fix 或 F。向策划 Chat 回传并附带且只附带本次同名 Report；正文首行使用：

    [CSEMI:REPORT_SENT] {"task_id":"Dev.D.UE.0.0.9B.P48.0.r0","file":"Dev.D.UE.0.0.9B.P48.0.r0_report.md"}
