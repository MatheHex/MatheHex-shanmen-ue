# Dev.D.UE.0.0.9B.P47.0.r0

## 任务身份

- 项目：Dev.D.UE.0.0.9B；继续使用同一活动工程，不新建项目。
- 阶段：主线 P47——已打开、已揭示 P10 BasicCache 完整空间图的受限 Ctrl + 左键快捷拾回。
- 任务编号：Dev.D.UE.0.0.9B.P47.0.r0。
- 前置：已接受 0.0.9B.P1—P46 与 0.0.9BFix.P1—P4。Fix 仅用于已确认、已验收功能的缺陷修复；P47 是新增主线功能，不是 Fix。
- 执行文件：Dev.D.UE.0.0.9B.P47.0.r0_prompt.md。
- 报告文件：Dev.D.UE.0.0.9B.P47.0.r0_report.md。
- 活动工程根：C:\\AIDev\\shanmen-ue\\Dev.D.UE.0.0.9B。
- 活动工程：C:\\AIDev\\shanmen-ue\\Dev.D.UE.0.0.9B\\demo_map.uproject。
- Report 必须生成到：C:\\AIDev\\shanmen-ue\\Dev.D.UE.0.0.9B\\Docs\\Report\\Dev.D.UE.0.0.9B.P47.0.r0_report.md，并仅携带该同名 Report 回传策划 Chat。
- 任务性质：P 阶段只做实现、静态审查和代码编译。不得启动产品、PIE、Standalone、真实鼠标键盘验证、截图、Smoke、Automation、回归、Cook、Package 或 F 阶段测试；不得自动开始 P48、任意 Fix 或 F。

## 上半部分：只读项目裁决、现状与边界

### 1. 当前唯一有效基线

唯一有效依据是当前 0.0.9B、活动工程和已接受任务链。所有 0.2、V2、V3、I、IPF、历史页面壳、旧 CTA、旧库存和旧物品规则均已过时；不得读取、采用、恢复或以其决定实现、验收或范围。

Code B P1 Repository 与既有 durable Store 是唯一可变物品真值。每件物品始终只有一个真实 ItemId、一个真实父位置和一条权威事务链。Widget、Cell、Presenter、DragOperation、Workspace Context、NormalContainerTarget、QuickTransfer resolver、空间 child 投影、WorldDrop Actor、地图放置适配层和 Code A 都只能持有只读投影、选择或瞬时意图；不得持有第二库存、可写数量副本、预建 ItemId、平行容器背包、Actor-first 写入或 A/B 双写。

P9/P10 已建立唯一生产普通容器 M01.CodeBNormalContainer.BasicCache.01 和其 exact BasicCache Run-local record。P18 已把未来首次物质化的 BasicCache 升级为确定性的 r2 Profile：保留 SpiritDust/IronShard 主材料组，并以一次低频 Optional.SpatialUtility gate 生成至多一个 WindTalisman 或 BackpackLevel1 parent，连同其唯一、空的 P17 ChildContainer。已物质化 record 必须继续按其 receipt、ProfileId、ProfileVersion、ProfileDigest、AlgorithmVersion 与 ResultDigest 保持原样；P47 不得 reroll、补料、迁移或改写这个历史。

P18 已使已 Revealed 的 BasicCache 完整空间图可经 P10 的明确 normal Drag/P1 composite transaction 整体移入 P6。P30 已为已打开 WorldDrop 完整图、P41 已为已打开尸体完整图建立 Ctrl + 左键的 BaseQuick-only 快捷拾回。P47 只将同一完整图、单一事务和 BaseQuick-only 语义接入 P10 BasicCache source；不得复用 WorldDrop record、ordinal、Actor 或尸体 provenance。

P17 的一层无嵌套限制持续有效：空间 parent、其 ChildContainer 或任何后代不得装入另一个空间 child，也不得形成自身或祖先回边。因此 P47 完整空间图明确不使用 P46 的 current-child-priority 模型：当前 child 是否打开、关闭、切换、失焦或填满均不得成为 P47 target、fallback 或额外条件。

### 2. P47 产品裁决

当玩家已经通过 P10 主动打开 identity-valid 的唯一 BasicCache，且其中一个 ordinary root 已 Revealed 并被 canonical P18/P17 truth 证明为 WindTalisman 或 BackpackLevel1 的完整空间图，玩家对该 parent Cell 按下 Ctrl + 左键时，既有共享 QuickTransfer resolver 必须只把该完整图原子移动到当前 P6 BaseQuick 中按真实 stable SlotIndex 升序找到的第一个正式、可写、空 ordinary storage cell。

| 输入时 source | 唯一允许的自动目标顺序 | 权威事务 |
| --- | --- | --- |
| exact opened/revealed P10 BasicCache ordinary root，且为 P18 canonical WindTalisman 或 BackpackLevel1 complete graph | 只在 active P6 BaseQuick ordinary storage cells 内按真实 stable SlotIndex 升序寻找第一个正式、可写、空 cell | 一个既有 P1/P18 whole-graph Move；同一 P9/P6 Owner durable replacement 与一次 SaveRecord |

这是已打开普通容器窗口中的一次快捷拾回请求，不是 Take All、自动拾取、自动装备、自动打开空间道具、child 操作、快捷回存容器或地图 Actor 直接领取。每次 Ctrl + 左键只处理当前 exact source graph、最多建立一个 candidate，并只提交一次完整图事务。

唯一自动目标域在输入时冻结为 active P6 BaseQuick。Preview 得到 candidate 后，intent/candidate 必须冻结 exact target ContainerId、SlotIndex、P6 composite revision 与必要 session proof；Commit 只能重验同一 target。若该 target stale、占用、无效或保存失败，不得重新扫描、改投第二空格、P17 child、装备位、Hotbar、仓库、世界、尸体或其他位置。

### 3. 严格范围与持续排除

P47 source 只接受同时满足全部条件的根图：

1. 当前 Workspace 是既有 P10 production Host；exact NormalContainerTarget 已 Open、identity-valid 且仍属于 active M01 BasicCache route、focus、target-open generation 与 active session；无 active open/search action。OwnerId、RunInstanceId、SearchTargetId、DefinitionId、P9 receipt、record revision、P6 composite revision、route、focus 和 active session 都能在 Preview 与 durable Commit 前从 Code B truth 重验；
2. source 位于 exact P9 RunLocalNormalContainerRecord 的 ordinary stable address。P9 receipt/profile/history、P10 target projection、Catalog、P1 snapshot 与 source ContainerId/SlotIndex/root ItemId 必须共同证明其为 P18 r2 BasicCache Optional.SpatialUtility 的 canonical result，而不是显示名、Cell class、Actor pointer、UI cache、选择态或展示顺序；
3. root DefinitionId 只能是正式 WindTalisman 或 BackpackLevel1。root 必须持有同一 ItemId 对应的唯一正式 P17 ChildContainer；parent、child ContainerId、stable SpatialChildGuid、child type、capacity、slot semantics、one-layer/no-cycle/no-orphan/no-duplicate-child-owner 与 P18 child-empty provenance 均必须由 canonical resolver 与 durable graph 验证；
4. P18 Profile/receipt/digest、source root、parent/child closure、Reveal/Open、Owner/Run、P9/P6 revision、active session、Prepared/terminal gate、Host validity 与 exact source slot reverse pointer 在 durable Commit 前仍完整成立。

唯一 target 必须同时满足：

1. target 是输入时 active P6 BaseQuick / Basic 根容器，而不是 P17 current child、SpatialRing、Backpack 装备位、Weapon、Armor、Accessory、Hotbar、P5、P9、P11、P14/P31、WorldDrop 或另一 NormalContainer；
2. target 是 P1/P6 truth 中与当前 OwnerId + RunInstanceId 一致的、正式可写空 ordinary storage cell；从真实 stable SlotIndex 升序扫描，取第一个合法空格；
3. target ContainerId、SlotIndex、slot semantic、空置状态、capacity、P6 revision、route/focus、active session 与 command lifecycle 在 Preview 与 durable Commit 前均必须一致；不得通过显示为空、Cell index、当前选择、焦点、Actor 或 Widget cache 猜测；
4. BaseQuick 无合法空格、candidate stale、source/target closure 无效、保存失败或任一 lifecycle/session 检查失败时一律零写入。

下列对象或动作持续不属于 P47：

1. BasicCache simple stack、P10 Hidden/Searching root、P18 graph 的 child item、任何未知 complex parent、P11/P12 corpse、P14/P31 WorldDrop、P21 equipment、P17 child 内任何物品、P5 warehouse、P6 player source、Hotbar、another NormalContainer、unknown provenance 或 UI-only object；
2. player-side Ctrl + 左键、P6 → BasicCache 快捷回存、quick-drop、Actor direct pickup、距离自动拾取、交互键领取、right-click Take、double-click、Take All、auto equip、auto child open/switch、auto bind、auto use、Swap、Replacement、Sort、Compact、Merge、Split、Quantity=N、WorldPickupDraft、PlayerSplitDraft、record-to-record transfer 或 parent/child partial transfer；
3. P10 ordinary normal Drag Move/Merge/Swap、P18 的 explicit complete-graph Drag、P46 BasicCache simple-stack Ctrl、P29/P30/P34/P36/P37/P39/P40/P41 的既有 Ctrl branches、P31 Registry、P8 terminal、P13 binding、P15 use 与 Code A authority；
4. 实机运行、PIE、Standalone、真实鼠标键盘、截图、Smoke、Automation、回归、Cook、Package 或最终验收。

## 下半部分：授权执行内容

### 4. 单一授权目标

在不建立第二物品真值、不改变 P9/P10 normal-container state machine、P18 deterministic materialization/history、P17 graph、P30/P41 complete-graph QuickTransfer、P31 Registry、P46 existing Ctrl semantics，也不新增自动装备或快捷回存的前提下，将 current opened/revealed exact P10 P18 spatial parent root 接入共享 BaseQuick-only QuickTransfer：

1. 只有 current opened/revealed exact P10 ordinary P18 spatial root Cell 才可建立一次 P47 QuickTransfer transient intent；
2. intent 在输入时冻结 BaseQuick-only target domain；Preview 解析一个 stable SlotIndex 最小的合法空 BaseQuick target 后，candidate 冻结 exact target identity，Commit 不得重扫或 fallback；
3. accepted path 只形成一个 P1/P18 whole-graph Move、一次 P9/P6 Owner durable replacement 和一次 SaveRecord；
4. 只有 accepted snapshot/replay proof 明确证明 parent 加唯一 ChildContainer closure 已离开 exact P9 source 时，才刷新 exact NormalContainerTarget source empty state 与解析得到的 P6 target projection；不得创建 WorldDrop、record、ordinal、Actor、new ItemId、new ContainerId、new child 或第二物品真值；
5. 任一 source、graph、target、identity、session、close/stale、P1 或保存检查失败时完整零写入与 BeforeSnapshot rollback。

### 5. 实现要求

#### 5.1 先完成活动调用链与资格审计

改动前必须审阅并在 Report 中列出：

1. P10 NormalContainerTarget 的 open/reveal lifecycle、ordinary BasicCache root Cell、normal Drag whole-graph Move、page/focus invalidation、P9/P6 callback 与 source proof；说明 P47 如何复用既有 source Cell 和 shared Ctrl pointer router，而不是新增 Button、hotkey、pointer handler、Widget、Actor 或容器写入口；
2. P18 的 r2 BasicCache optional spatial source、Profile/receipt/digest identity、parent-child graph materialization 和既有 P10 explicit complete-graph transaction；说明 P47 不会改写 r1/r2 历史、gate、weight、candidate、ItemId 或 ChildContainer；
3. P30 与 P41 的 complete-graph QuickTransfer intent、BaseQuick-only resolver、whole-graph Preview/Commit、single candidate、rollback 和 stale lifecycle；说明 P47 仅接入 P10 source，不获得 WorldDrop/body record、ordinal 或 Actor 语义；
4. P46/P29/P36/P37/P39/P40 的 shared pointer consumption、source discrimination、cancellation 与 lifecycle；说明 P47 不会把 simple-stack child-priority 或 Merge 语义扩张到完整空间图；
5. P17 的 parent → child canonical topology、formal child type、dynamic capacity、一层无嵌套规则与 P6 BaseQuick ordinary target policy；解释为什么完整空间 parent 必须拒绝 P17 child、装备位与 Hotbar；
6. P1/P2/P3/P6/P9 的 whole-graph Move、cross-graph single candidate、accepted snapshot/replay proof、BeforeSnapshot rollback、P9/P6 single Owner durable replacement、P13 reconcile、revision/session 调用图；
7. 所有 Widget direct Move/graph mutation、Actor direct pickup、container display-cache write、right-click Take、double-click、预建 ItemId、旧 Code A loot、按 first/last/selection 猜测 target、第二 Repository transaction、第二 save 或 Code A inventory writer；它们不得成为 P47 写入路径。

#### 5.2 共享输入、source gate 与 BaseQuick-only 意图

1. UCodeBP3CellButton::NativeOnMouseButtonDown 或当前等价共享 pointer router 仍是 Ctrl + 左键唯一消费点。命中 P47 source 后只能建立一次既有 QuickTransfer transient intent 并返回 Handled；不得继续 ordinary selection、P10 search、drag threshold、P15 Use、right-click detail 或 Actor interaction。
2. 不得为 P47 新建 Button、hotkey、Actor click、专用 Widget、第二 pointer handler、second QuickTransfer resolver 或 UI direct write。resolver 必须从 canonical P10/P18 source proof、P9 exact source address、P17 topology 与 P6 truth 在 P30/P41/P46/P47 branches 间分流。
3. intent 创建时必须先重验 exact NormalContainerTarget/SearchTargetId/DefinitionId/receipt/source-slot identity、P10 Reveal/Open 与 P18 complete-graph qualification，然后明确写入 BaseQuickOnly 模式、exact BaseQuick ContainerId、Owner/Run、target receipt、source root/child closure、P9/P6 revision、target-open generation、route/focus/session proof。不得留空、延迟决策或把 UI focus/selection 当成随后可变 target。
4. Preview 与 durable Commit 前必须重验 OwnerId、RunInstanceId、SearchTargetId、DefinitionId、receipt、P9 record revision、source ContainerId/SlotIndex、root parent ItemId、unique child closure、P18 canonical provenance、Reveal/Open、route、focus、page/target-open generation、P6 revision、active session、Prepared/terminal gate、Host validity 与 exact BaseQuick-only target proof。
5. Target close/reopen、focus loss、开始/取消 search、Actor EndPlay、map reload、recovery、terminal/Prepared、receipt/root/container mismatch、payload cancel、Host invalidation、source revision stale、parent/child topology stale、BaseQuick change 或 SaveRecord failure 必须立即使 intent/candidate 失效并零写入。不得改投另一个 BaseQuick 格、P17 child、装备位或任何 other target。
6. ordinary left click 继续只选择或按 P10 既有规则开始 search；right-click 继续只读详情；normal Drag 继续只走 P10 explicit Move/Merge/Swap 或 P18 whole-graph policy。double-click、Tab、I、Esc、Close、Cancel、scroll、空白区、无 payload Drop、Shift + 1—9、BasicCache simple root、P18 child item、player-side Ctrl 与所有 existing WorldDrop/Corpse Ctrl branch 均不得获得 P47 位置或图写入语义。

#### 5.3 Canonical source、完整图验证与确定性落点

1. Preview 只接受 current exact P9 BasicCache ordinary root container 的 Revealed P18 spatial parent root。Store 必须从 canonical Catalog、P1/P6/P9 snapshot、P18 profile/receipt/record、P10 NormalContainerTarget 与 source stable address 重建并全字段验证 ordinary provenance、parent Definition、source slot、record availability、exact ItemId、reverse slot pointer、parent-child closure 与 child-empty constraint。
2. source 必须拒绝 simple stack、stack draft、space child、P19/P20 WorldDrop/corpse root、P21 equipment、P10 Hidden/Searching item、P5/P11/P14/P31、warehouse、Hotbar、player source、another NormalContainer、unknown family、wrong target generation 或任何 source mismatch；它们均零写入。
3. 唯一可接受 Definition 是 WindTalisman 或 BackpackLevel1，且 graph 必须有唯一正式 ChildContainer。P17/P18 canonical validation 必须验证 parent DefinitionId、stable parent ItemId、ChildContainerId、stable SpatialChildGuid、formal child type/capacity、root placement、child empty、one-layer restriction、无环、无 orphan、无 duplicate child owner、无非法 slot。不得以名称、图标、Cell index、Actor tag、fixture、world coordinate、profile label 或缓存判断 graph 资格。
4. P47 target scan 只能在 intent 记录的 exact active P6 BaseQuick 内进行。严格按真实 stable SlotIndex 升序扫描正式、可写、空 ordinary storage cell；第一个同时通过 Owner/Run、ContainerId、capacity、slot semantic、slot availability、P6 revision、route/focus 与 active-session 验证的 cell，是唯一可建立 candidate 的 target。
5. 不得把下列位置当作可用空格：SpatialRing、Backpack、Weapon、Armor、Accessory、任何 P17 child cell、Hotbar reference、P5/P9/P11/P14 container、保护格、错误 Owner/Run 格、stale cell、不可写 cell、显示上为空但 P1 graph 中不为空的格。
6. BaseQuick 不存在或无合法空格时拒绝且零写入；不得 Merge、Swap、Replacement、Split、Quantity=0、RequestedMergeQuantity、WorldPickupDraft、PlayerSplitDraft、再扫描其他容器、自动装备、打开 child、切换 child、选择 another NormalContainer 或创建世界目标。
7. Preview 成功后必须冻结 exact target ContainerId/SlotIndex 和 P6 revision。Commit 只可接受同一 target 的 whole-graph Move；若该 target 已不再精确空、source/target revision 或 session stale、容量/slot semantics 改变，则拒绝，不得为保证成功重扫另一个格。

#### 5.4 单一完整图事务、持久化与回滚

1. Preview 成功后只能建立一个 Owner/Run scoped candidate，并且只调用 P18/P1 已有的 whole-graph Move 语义：同一个 parent ItemId、唯一 ChildContainer 与其完整 closure 从 exact P9 BasicCache source 直接进入一个冻结的 P6 BaseQuick target。不得用 simple Move 绕过 closure 验证，不得先落 BaseQuick 再二次 Move，不得调用 Merge、Split、Swap、Equip、Unequip 或创建 graph。
2. Commit 必须沿既有 P10/P3 → P2 → P1 → P9/P6 durable callback。Store 在任何 durable write 前重新验证 command intent、opened NormalContainerTarget identity、source/target stable address、Owner、Run、P9/P6 revision、P18 graph closure、BaseQuick empty state、active session 与 lifecycle gate。
3. 同一 accepted candidate 中必须保持 parent、ChildContainer、所有 ItemId/ContainerId、capacity、placement order、stable SpatialChildGuid 与 provenance 身份不变；唯一合法位置变化是 graph root 从 exact P9 ordinary source 移到 exact P6 BaseQuick target。不得 clone、flatten、new ItemId、new ContainerId、new child、parent-only move、partial commit 或 child snapshot。
4. 只有 accepted snapshot/replay proof 明确证明完整 graph root 已离开 exact P9 source，才可在同一个 Owner candidate 中更新 P9 normal-container snapshot、P6 player snapshot 和 exact NormalContainerTarget source projection。不得预清 source、预写 P6、预删 child、预刷 Code A Actor 或在 save 后补写另一侧。
5. P9 与 P6 必须在同一 Owner durable replacement 中共同提交，且只有一次 SaveRecord。P13 只按既有 accepted commit reconcile；不得自动 Bind、Use、Equip 或复制 binding。P8 仍只结算当时 P6 player graph，并只丢弃 P9 residual；P47 不改变容器的开启、搜索、receipt、终局或 recovery。
6. P47 绝不创建、删除或写入 P14/P31 WorldDrop record、NextWorldDropOrdinal、WorldDrop Actor、地图落点、空间 child projection之外的对象或任一 Code A inventory path。Code A 若须 lifecycle forwarding，只能只读转发 production NormalContainerTarget projection，不能拥有 Item、Container、Quantity、P9/P6 或 durable authority。
7. candidate、canonical gate、graph validation、target scan、P1 Move、P13 reconcile、projection 前检查或 SaveRecord 任一失败时，必须完整恢复 BeforeSnapshot：P9 source parent/child closure、P6 target、NormalContainerTarget visibility/open state、P7 projection、selection 与无关 target/body/world record 均保持未变，且不得遗留 phantom empty slot、phantom pickup、图副本或 transient item copy。
8. 成功后只刷新 exact NormalContainerTarget source section、解析得到的 BaseQuick target 和必要关联 P7 projection；不得 Sort、Compact、重排无关 SlotIndex、重建无关空间区域、清空无关选择、改变无关 scroll offset 或改写 P29/P30/P34/P36/P37/P39/P40/P41/P46 语义。

#### 5.5 非回归、终局与权威边界

1. P18 complete graph 的 normal Drag 仍保留其 explicit BasicCache root → user-selected valid P6 target transaction；P47 只添加 Ctrl QuickTransfer，不得收窄、重定向或自动触发 normal Drag。P6 spatial parent 或其 child 回写 BasicCache 仍必须按 P10 existing explicit Drag policy，而不获得 Ctrl 语义。
2. P46 BasicCache simple-stack Ctrl 路径仍只在 valid current child 优先／无 child 才 BaseQuick 的 frozen mode 中按 merge-first / empty-second 运作；P47 不得让 simple stack 进入 BaseQuickOnly whole-graph branch，也不得改变 partial acceptance、remaining ItemId/slot 或数量语义。
3. P30 WorldDrop、P41 corpse complete-graph Ctrl 路径继续各自处理其 exact source family、provenance 与 record/body lifecycle；P47 不得创建 WorldDrop、读取 ordinal、删除 record、读取尸体 receipt或让 BasicCache source 进入其 durable branch。
4. P29 simple-stack ground、P34/P36/P37 standard equipment ground、P39/P40 corpse Ctrl、P43—P45 GroundDrop、P17 graph construction、P19/P20 graph semantics、P31 Registry、P5/P6 bridge、P8 terminal/recovery、P13 binding、P15 use、P18 materialization/history 与 Code A authority均保持既有产品语义。
5. P17 一层无嵌套规则、P18 child-empty provenance、P9/P10 Hidden/Searching/Reveal、BasicCache r1/r2 deterministic identity、candidate/weight/digest 与 result history 都不得因 P47 被重掷、补料、迁移或改写。

### 6. 允许的改动范围

仅允许在当前 Code B 中最小修改：

- P10 已有 NormalContainer source proof、shared pointer routing、QuickTransfer preview/commit、P9/P6 durable transaction 与 projection；
- P18/P30/P41 的既有 complete-graph validation、BaseQuick-only resolver、whole-graph Move、rollback 与 accepted proof，仅用于 P47 exact P10 P18 source；
- P46/P29/P36/P37/P39/P40 的既有 transient intent、branch discrimination、source lifecycle、cancellation 与 revision/session proof，仅用于接入 P47；
- P1/P2/P3/P6/P9/P13/P17/P18 的必要声明、candidate proof、rollback 或调用签名兼容，前提是不改变已有产品语义；
- 必要的 production NormalContainer lifecycle forwarding，只能作为 Code B gate 的只读转发；
- PROJECT.md、PROJECT_INFO_CARD.md、本任务 Prompt 归档与本任务 Report。

禁止新建项目、版本线、Fix、第二 P9/P6、Widget inventory、Code A mirror、fixture、假 ItemId、clone、双写、存档重置或历史数据改写。禁止修改 Code A 功能逻辑、P9/P10 materialization/search state machine、P18 profile/history、P17 graph construction、P31 schema/registry、P8 receipt/terminal product logic、P5/P6 bridge 或任何测试文件。

### 7. 明确不在本任务内

- 不把 BasicCache simple stack、P18 child item、P11/P12 corpse、P14/P31 WorldDrop、P21 equipment、P17 child item、P5 warehouse、P6 player source、Hotbar、other NormalContainer、other provenance 或 existing WorldDrop root 接入 P47 source。
- 不实现 player → BasicCache Ctrl 回存、BasicCache → GroundDrop、快捷丢弃、Actor direct pickup、自动拾取、Take All、right-click Take、double-click、auto target、auto equipment、auto child open/switch、space parent/child 地面操作、world multi-item container、record-to-record transfer、cross-record merge、Swap、Replacement、Sort、Compact、Bind、Use、装备数值、战斗效果、HUD 接管、网络或多人。
- 不改变 P10 normal Drag Move/Merge/Swap、P18 source/history、P29/P30/P34/P36/P37/P39/P40/P41/P46 existing Ctrl semantics、P13/P15、P17/P19/P20、P26—P28、P31、P5/P6/P8、搜索、尸体、敌人、地图、战斗、生命、死亡、撤离、商店、经济或制作。
- 不启动产品、PIE、Standalone、真实鼠标键盘输入、截图、Smoke、Automation、回归、试玩、Cook、Package 或最终验收。
- 不在回传 Report 前自动开始 P48、Fix 或 F。

### 8. P 阶段静态审查与编译

完成后只执行以下检查：

1. 审查 P47 复用唯一 Ctrl + 左键 pointer router 与 existing QuickTransfer intent；确认没有专用 input/UI/Actor 写入旁路，且分支只依据 canonical P10/P18 spatial source truth 与 exact NormalContainerTarget identity。
2. 审查 source 只接受 current opened/revealed/exact P10 ordinary P18 WindTalisman/BackpackLevel1 parent graph，并逐项复核 Owner/Run/SearchTargetId/receipt/profile/history/P9 revision/source container-slot/root/child closure/page/focus/P6 revision；simple stack、child、corpse、WorldDrop、Hidden/Searching、stale/close/terminal 均零写入。
3. 审查 P17/P18 complete graph closure：唯一 child、formal type/capacity、empty-child provenance、一层无嵌套、无环、无 orphan、无 duplicate owner、stable SpatialChildGuid 与 parent/child identity 均保持。确认 P47 未重掷、补料或改写 P18 profile/history。
4. 审查 input-time BaseQuickOnly resolver：只按真实 stable SlotIndex 在 exact active BaseQuick 内找第一个合法空 ordinary cell；确认不扫描 child、装备位、Hotbar、P5/P9/P11/P14/P31 或 other target，且 candidate stale/满位时不重扫或回退。
5. 审查 candidate 只使用一个 P1/P18 whole-graph Move；确认没有 Merge、Split、Quantity=0、partial graph、new ItemId、Container、receipt、record、ordinal、Actor、child 或第二物品真值。
6. 审查 P9/P6 仅以一个 Owner durable replacement 和一次 SaveRecord 共同提交；确认 P9 source 只在 accepted proof 后更新、P13 reconcile、BeforeSnapshot rollback、P8 terminal/recovery、selection/scroll/stable SlotIndex 与 P17 graph 均保持。
7. 审查 P10 normal Drag、P18 source/history、P30/P41、P46、P29/P34/P36/P37/P39/P40、P31 Registry、P5/P6/P8/P13/P15 与 Code A authority均无产品语义回归。
8. 审查 right-click、double-click、ordinary Drag、Actor interaction、Shift + 1—9、player-side Ctrl、BasicCache simple source、P18 child、P6 → BasicCache drag 均未获得 P47 隐式位置或完整图写入语义。
9. 执行 git diff --check。
10. 编译 Editor：

       "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_mapEditor Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload

11. 编译 Game：

       "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_map Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload

若编译失败，只修复本任务引入的 P10 spatial QuickTransfer routing、source proof、whole-graph resolver、P9/P6 transaction、projection、rollback、include 或签名问题；若必须扩大到本任务以外，停止受影响部分并如实报告。

### 9. Report 与完成信号

生成 Dev.D.UE.0.0.9B.P47.0.r0_report.md，保存至：

    C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Docs\Report

Report 必须简洁、可审计地列出：

1. 本轮新增、修改、未修改的每个文件及职责；
2. P10 ordinary spatial source Cell 到 P47 shared QuickTransfer 的完整调用图，以及没有新增 pointer、Widget、Actor、second resolver 或 Code A 写入口的证据；
3. P18 BasicCache r2 spatial provenance、actual stable ordinary source slot、NormalContainerTarget/SearchTargetId/receipt/profile identity、完整 parent/child closure，以及为什么 P18 materialization/history/determinism 未被改写；
4. exact source gate、Reveal/Open lifecycle、Owner/Run/revision/session proof、BaseQuickOnly 冻结、candidate target freeze 与 zero-write cancellation/stale policy；
5. exact BaseQuick stable SlotIndex 次序、first-empty 解析、capacity validation、没有 child/equipment/Hotbar/second-slot scan、无 fallback/auto equip 的证据；
6. single P1/P18 whole-graph Move、single P9/P6 durable replacement、source/target parent-child identity 保持、P13/P8、rollback、selection/scroll/stable SlotIndex 与无 WorldDrop/ordinal/Actor write 的证据；
7. P10 normal Drag、P18 source/history、P30/P41、P46、P29/P34/P36/P37/P39/P40、P31、P5/P6/P8/P13/P15 与 Code A authority 的非回归结论；
8. git diff --check 结果、两个编译命令、目标、原生 exit code 与关键结果；
9. 所有未执行的 F 阶段真实验证，至少包括：BasicCache r2 materialize、optional spatial hit/no-hit、open/reveal；实际 canonical WindTalisman/BackpackLevel1 Ctrl 拾回至 first legal BaseQuick；空/满 BaseQuick、stable SlotIndex、完整 parent/child identity、child open/switch/close 不成为目标；source wrong/simple/equipment/child/Hidden/Searching；SearchTargetId/receipt/profile/Owner/Run/P9/P6 revision stale；Target close/reopen、P10 action/search、Prepared/terminal/Host invalid/SaveRecord failure；P10 normal Move/Merge/Swap、P18 normal graph Drag、P46、P29/P30/P34/P36/P37/P39/P40/P41、P43—P45、P26—P31、P8 terminal/recovery；真实鼠标键盘、PIE、Standalone、截图、Smoke、Automation、回归、Cook 与 Package。

仅当 P47 P10 P18 complete-graph BaseQuick-only QuickTransfer 静态闭合、P9/P10/P18 与 P17/P29/P30/P31/P41/P46/P8 边界保持，且 Editor 与 Game 均以 native exit code 0 完成时，使用：

    READY_FOR_P48_PLANNING

若当前范围内仍有可修复问题，使用：

    NEEDS_P47_REWORK

若现有 P9/P10/P6 transaction 或 shared Ctrl resolver 无法在不创建第二输入路径、第二保存、第二物品真值、自动装备、自动拾取、改写 P18 deterministic history 或扩展 Code A authority 的前提下支持本项，使用：

    NEEDS_PLANNER_DECISION
    BLOCKED

完成后不得自动开始 P48、Fix 或 F。向策划 Chat 回传并附带且只附带本次同名 Report；正文首行使用：

    [CSEMI:REPORT_SENT] {"task_id":"Dev.D.UE.0.0.9B.P47.0.r0","file":"Dev.D.UE.0.0.9B.P47.0.r0_report.md"}
