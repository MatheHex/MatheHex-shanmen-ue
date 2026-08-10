# Dev.D.UE.0.0.9B.P41.0.r0

## 任务身份

- 项目：Dev.D.UE.0.0.9B；继续使用同一活动工程，不新建项目。
- 阶段：主线 P41——为已打开、已揭示的 P12 尸体完整空间道具图补齐受限的 Ctrl + 左键快捷拾回。
- 任务编号：Dev.D.UE.0.0.9B.P41.0.r0。
- 前置：已接受 0.0.9B.P1—P40 与 0.0.9BFix.P1—P4。Fix 只用于已确认、已验收功能的缺陷修复；P41 是新增主线功能，不是 Fix。
- 执行文件：Dev.D.UE.0.0.9B.P41.0.r0_prompt.md。
- 报告文件：Dev.D.UE.0.0.9B.P41.0.r0_report.md。
- 活动工程根：C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B。
- 活动工程：C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject。
- 任务性质：P 阶段只做实现、静态审查与代码编译。不得启动产品、PIE、Standalone、真实输入验证、截图、Smoke、Automation、回归、Cook、Package 或 F 阶段测试；不得自动开始 P42、任意 Fix 或 F。

## 上半部分：只读项目裁决、现状与边界

### 1. 当前唯一有效基线

唯一有效依据是当前 0.0.9B、活动工程及已接受任务链。所有 0.2、V2、V3、I、IPF、历史页面壳、旧 CTA、旧库存和旧物品规则均已过时；不得读取、采用、恢复或以其决定实现、验收或范围。

Code B P1 Repository 与既有 durable Store 是唯一可变物品真值。每件物品始终只有一个真实 ItemId、一个真实父位置和一条权威事务链。Widget、Cell、Presenter、DragOperation、Workspace Context、BodyTarget、QuickTransfer resolver、空间 child 投影、WorldDrop Actor、地图放置适配层和 Code A 都只能持有只读投影、选择或瞬时意图；不得持有第二库存、可写数量副本、预建 ItemId、平行尸体背包、Actor-first 写入或 A/B 双写。

P11/P12 已建立唯一 BasicCorpse 的死亡回执、首次物质化、Hidden → Searching → Revealed、已打开 BodyTarget，以及 P11/P6 的单次 durable Move、Merge、Swap。P20 使未来 BasicCorpse 可依据确定性 Optional.SpatialUtility 产生一个正式 WindTalisman 或 BackpackLevel1 parent，连同其唯一、空的 P17 ChildContainer，作为 P11 内的不可拆分完整图。P21 的 r3 Profile 原样继承该 P20 optional group；因此 P41 只接受由 P20 canonical provenance 证明的空间 parent，不得重掷、改写或迁移 r2/r3 物质化历史。

P20 已允许这类已 Revealed 的尸体空间 parent 经 P12 的真实 Drag/Drop，完整进入一个明确空的 P6 BaseQuick 格；P30 已允许 P19 WorldDrop 中的同类完整图 Ctrl + 左键移动到首个合法空 BaseQuick 格。P41 只把两者已确立的完整图、BaseQuick-only、单一事务语义接入 P12 尸体来源；不把地面 record、WorldDrop Actor 或 P20 普通 Drag 路径带入新的物品体系。

P36、P37、P39、P40 已把 simple stack 和标准装备的共享 Ctrl + 左键收敛为 P17 child 优先／输入时无 valid child 才 BaseQuick 的冻结模型。P41 的完整空间 parent 明确不使用该 child 优先模型：P17 一层限制禁止把一个空间 parent 装入任何 P17 child，且 P20 既有合法落点就是 P6 BaseQuick。因此当前 child 是否打开、切换、失焦或填满都不能成为 P41 的目的地、fallback 或额外条件。

### 2. P41 产品裁决

当玩家已经通过 P12 主动打开 identity-valid 的唯一 BasicCorpse，且其中一个普通尸体根格内的 exact P20 complete-graph parent 已 Revealed，玩家对该 parent Cell 按下 Ctrl + 左键时，既有共享 QuickTransfer resolver 必须只把该完整图原子移动到当前 P6 BaseQuick 中按真实 stable SlotIndex 升序找到的第一个正式、可写、空 ordinary storage cell。

| 输入时 source | 唯一允许的自动目标顺序 | 权威事务 |
| --- | --- | --- |
| exact opened/revealed P12 ordinary root，且为 P20 canonical WindTalisman 或 BackpackLevel1 complete graph | 只在 active P6 BaseQuick ordinary storage cells 内按真实 stable SlotIndex 升序寻找第一个正式、可写、空 cell | 一个既有 P1/P20 whole-graph Move；同一 P11/P6 Owner durable replacement 与一次 SaveRecord |

这是已打开尸体窗口中一次快捷拾回请求，不是 Take All、自动拾取、自动装备、自动打开空间道具、child 操作或快捷回存尸体。每次 Ctrl + 左键只处理当前 exact source graph、最多建立一个 candidate，并只提交一次完整图事务。

P41 的唯一自动目标域在输入时冻结为 active P6 BaseQuick。若 Preview 得到 candidate，intent/candidate 必须冻结 exact target ContainerId、SlotIndex、P6 composite revision 与必要 session proof；Commit 只能重验同一 exact target，不能在其 stale、占用、无效或保存失败后重新扫描、改投第二空格、P17 child、装备位、Hotbar、仓库、世界或其他位置。输入时打开、关闭、切换或投影任何 P17 child 都不得改变 P41 目的地。

### 3. 严格范围与持续排除

P41 source 只接受同时满足全部条件的根图：

1. 当前 Workspace 是既有 P12 production Host；exact BasicCorpse BodyTarget 已 Open、已 Revealed、identity-valid，且无 active action/search。OwnerId、RunInstanceId、BodyTargetId、DeathReceiptId、body record revision、target-open/page generation、P6 composite revision、route、focus 与 active session 都能在 Preview 和 durable Commit 前从 P1/P6/P11 truth 重新验证；
2. source 位于 exact P11 BasicCorpse 的 ordinary body-storage stable address，不属于 P21 的 Body.Weapon、Body.ArmorRobe 或 Body.Accessory0 固定装备容器，不是 child Cell，也不是 UI-only projection。P11 receipt/profile、P20/P21 history、BodyTarget projection、Catalog、P1 snapshot 与 source ContainerId/SlotIndex 必须共同证明其 canonical P20 spatial provenance；
3. root DefinitionId 只能是正式 WindTalisman 或 BackpackLevel1。root 必须有同一 ItemId 对应的唯一正式 P17 ChildContainer；parent、child ContainerId、stable SpatialChildGuid、child type、capacity、slot semantics、one-layer/no-cycle/no-orphan/no-duplicate-child-owner 与 P20 尸体中 child 为空的约束必须全部由 canonical resolver 与 durable graph 验证；
4. P20/P21 profile、DeathReceipt、source root、parent/child closure、Reveal/Open、Owner/Run、P6/body revision、active session、Prepared/terminal gate、Host validity 与 exact source slot reverse pointer 在 durable Commit 前仍完整成立。

唯一 target 必须同时满足：

1. 目标容器是输入时 active P6 BaseQuick / Basic 根容器，而不是 P17 current child、SpatialRing、Backpack 装备位、Weapon、Armor、Accessory、Hotbar、P5、P9、P11、P14/P31、WorldDrop 或另一 BodyTarget；
2. 目标必须是 P1/P6 truth 中与当前 OwnerId + RunInstanceId 一致的、正式可写空 ordinary storage cell；从真实 stable SlotIndex 升序扫描，取第一个合法空格；
3. target ContainerId、SlotIndex、slot semantic、空置状态、capacity、P6 revision、route/focus、active session 与 command lifecycle 在 Preview 与 durable Commit 前均必须一致；不允许通过显示为空、Cell index、当前选择、焦点、Actor、Widget cache 或 first/last 数组顺序猜测；
4. BaseQuick 无合法空格、candidate stale、source/target closure 无效、保存失败或任一 lifecycle/session 检查失败时一律零写入。

下列对象或动作持续不属于 P41：

1. P12 ordinary simple stack、P21 fixed equipment、P20/P19 graph 的 child item、任何未知 complex parent、P17 child 内任何物品、P12 Hidden/Searching root、empty body slot、P9 ordinary container、P14/P31 WorldDrop、P5 warehouse、P6 player source、Hotbar、another BodyTarget、unknown provenance 或 UI-only object；
2. player-side Ctrl + 左键、quick-drop、P6 → corpse 快捷回存、Actor direct pickup、距离自动拾取、交互键领取、right-click Take、double-click、Take All、auto equip、auto target（除本项 BaseQuick 内一个空格的正式 scan）、auto child open/switch、auto bind、auto use、Swap、Replacement、Sort、Compact、Merge、Split、Quantity=N、WorldPickupDraft、PlayerSplitDraft、record-to-record transfer 或 parent/child partial transfer；
3. P12 普通真实 Drag 的 Move/Merge/Swap、P20 已有 explicit complete-graph Drag、P21/P38/P39 corpse-equipment 路径、P40 ordinary simple-stack QuickTransfer、P29/P30/P34/P36/P37 ground Ctrl branches、P17 graph construction、P31 Registry、P8 terminal 分类、P13 binding、P15 use 与 Code A authority；
4. 实机运行、PIE、Standalone、真实鼠标键盘、截图、Smoke、Automation、回归、Cook、Package 或最终验收。

## 下半部分：授权执行内容

### 4. 单一授权目标

在不建立第二物品真值、不改变 P11/P12 corpse state machine、P20/P21 deterministic source/history、P17 graph、P31 Registry、P30 的 WorldDrop whole-graph QuickTransfer、P39/P40 既有 Ctrl + 左键语义，也不新增自动装备或快捷回存尸体的前提下，将 current opened/revealed exact P12 P20 spatial parent root 接入共享 BaseQuick-only QuickTransfer：

1. 只有 current opened/revealed exact P12 ordinary P20 spatial root Cell 才可建立一次 P41 QuickTransfer transient intent；
2. intent 在输入时冻结 BaseQuick-only target domain；Preview 解析一个 stable SlotIndex 最小的合法空 BaseQuick target 后，candidate 冻结 exact target identity，Commit 不得重扫或 fallback；
3. accepted path 只形成一个 P1/P20 whole-graph Move、一次 P11/P6 Owner durable replacement 和一次 SaveRecord；
4. 只有 accepted snapshot/replay proof 明确证明 parent 加唯一 ChildContainer closure 已离开 exact P11 source 时，才刷新 exact BodyTarget source empty state 与解析得到的 P6 target projection；不得创建 WorldDrop、record、ordinal、Actor、new ItemId、new ContainerId、new child 或第二物品真值；
5. 任一 source、graph、target、identity、session、close/stale、P1 或保存检查失败时完整零写入与 BeforeSnapshot rollback。

### 5. 实现要求

#### 5.1 先完成活动调用链与资格审计

改动前必须审阅并在 Report 中列出：

1. P12 的 BodyTarget open/reveal lifecycle、ordinary body root Cell、normal Drag whole-graph Move、page/focus invalidation、P11/P6 callback 与 source proof；说明 P41 如何复用既有 source Cell 和共享 Ctrl pointer router，而不是新增 Button、hotkey、pointer handler、Widget、Actor 或尸体写入口；
2. P30 的 P19 complete-graph QuickTransfer intent、BaseQuick-only target resolver、whole-graph Preview/Commit、record cleanup 与 rollback；说明 P41 如何复用其 topology validation 与 single candidate，而不复用 WorldDrop record/ordinal/Actor 语义；
3. P39/P40 的 shared QuickTransfer pointer consumption、transient proof、preview、commit、cancellation 与 stale lifecycle；说明 P41 如何增加严格 P20 P12 source branch，并保持 P39 child-priority 与 P40 merge-first / empty-second 语义不变；
4. P20/P21 的 actual BasicCorpse optional spatial source provenance、P20 r2 与 P21 r3 inherited group、profile/receipt/body record identity、exact stable ordinary source container、唯一 empty child closure，以及为什么 materialization、candidate/weight/digest 与 deterministic history 未被改写；
5. P17 的 parent → child canonical topology、formal child type、dynamic capacity、one-layer restriction 与 P6 BaseQuick ordinary target policy；解释为什么完整空间 parent 必须始终拒绝 P17 child、装备位与 Hotbar；
6. P1/P2/P3/P6/P11 的 whole-graph Move、cross-graph single candidate、accepted snapshot/replay proof、BeforeSnapshot rollback、P11/P6 single Owner durable replacement、P13 reconcile、revision/session 调用图；
7. 所有 Widget direct Move/graph mutation、Actor direct pickup、尸体展示缓存写入、right-click Take、double-click、预建 ItemId、旧 Code A loot、按 first/last/selection 猜测 target、第二 Repository transaction、第二 save 或 Code A inventory writer；它们不得成为 P41 写入路径。

#### 5.2 共享输入、source gate 与 BaseQuick-only 意图

1. UCodeBP3CellButton::NativeOnMouseButtonDown 或当前等价共享 pointer router 仍是 Ctrl + 左键唯一消费点。命中 P41 source 后只能建立一次既有 QuickTransfer transient intent 并返回 Handled；不得继续 ordinary selection、body search、drag threshold、P15 Use、right-click detail 或 Actor interaction。
2. 不得为 P41 新建 Button、hotkey、Actor click、专用 Widget、第二 pointer handler、second QuickTransfer resolver 或 UI direct write。resolver 必须从 canonical P12 source proof、P20/P21 history、P11 exact source address、P17 topology 与 P6 truth 在 P30/P39/P40/P41 branches 间分流。
3. intent 创建时必须先重验 exact BodyTarget/death receipt/source-slot identity、P12 Reveal/Open 与 P20 complete-graph qualification，然后明确写入 BaseQuickOnly 模式、exact BaseQuick ContainerId、Owner/Run、BodyTarget/death receipt、source root/child closure、body/P6 revision、target-open generation、route/focus/session proof。不得留空、延迟决策或把 UI focus/selection 当成随后可变 target。
4. Preview 与 durable Commit 前必须重验 OwnerId、RunInstanceId、BodyTargetId、DeathReceiptId、body record revision、source ContainerId/SlotIndex、root parent ItemId、unique child closure、P20 canonical provenance、Reveal/Open、route、focus、page/target-open generation、P6 revision、active session、Prepared/terminal gate、Host validity 与 exact BaseQuick-only target proof。
5. BodyTarget close/reopen、focus loss、开始/取消 search、Actor EndPlay、map reload、recovery、terminal/Prepared、receipt/root/container mismatch、payload cancel、Host invalidation、source revision stale、parent/child topology stale、BaseQuick change 或 SaveRecord failure 必须立即使 intent/candidate 失效并零写入。不得改投另一个 BaseQuick 格、P17 child、装备位或任何 other target。
6. ordinary left click 继续只选择或按 P12 既有规则开始 search；right-click 继续只读详情；normal Drag 继续只走 P12 explicit Move/Merge/Swap 或 P20 whole-graph policy。double-click、Tab、I、Esc、Close、Cancel、scroll、空白区、无 payload Drop、Shift + 1—9、P12 simple root、P21 equipment、P17 child item、player-side Ctrl 与所有 existing WorldDrop Ctrl branch 均不得获得 P41 位置或图写入语义。

#### 5.3 Canonical source、完整图验证与确定性落点

1. Preview 只接受 current exact P11 ordinary body-storage container 的 Revealed P20 spatial parent root。Store 必须从 canonical Catalog、P1/P6/P11 snapshot、P20/P21 profile/receipt/body record、P12 BodyTarget 与 source stable address 重建并全字段验证 ordinary provenance、parent Definition、source slot、record availability、exact ItemId、reverse slot pointer、parent/child closure 与 child empty constraint。
2. source 必须拒绝 P21 equipment slot、simple stack、stack draft、space child、P19 WorldDrop root、非 P20 parent、P12 Hidden/Searching item、P5/P9/P14/P31、warehouse、Hotbar、player source、another BodyTarget、unknown family、wrong BodyTarget generation 或任何 source mismatch；它们均零写入。
3. 唯一可接受 Definition 是 WindTalisman 或 BackpackLevel1，且 graph 必须有唯一正式 ChildContainer。P17/P20 canonical validation 必须验证 parent DefinitionId、stable parent ItemId、ChildContainerId、stable SpatialChildGuid、formal child type/capacity、root placement、child empty、one-layer restriction、无环、无 orphan、无 duplicate child owner、无非法 slot。不得以名称、图标、Cell index、Actor tag、fixture、world coordinate、profile label 或缓存判断 graph 资格。
4. P41 target scan 只能在 intent 记录的 exact active P6 BaseQuick 内进行。严格按真实 stable SlotIndex 升序扫描正式、可写、空 ordinary storage cell；第一个同时通过 Owner/Run、ContainerId、capacity、slot semantic、slot availability、P6 revision、route/focus 与 active-session 验证的 cell，是唯一可建立 candidate 的 target。
5. 不得把下列位置当作可用空格：SpatialRing、Backpack、Weapon、Armor、Accessory、任何 P17 child cell、Hotbar reference、P5/P9/P11/P14 container、保护格、错误 Owner/Run 格、stale cell、不可写 cell、显示上为空但 P1 graph 中不为空的格。
6. BaseQuick 不存在或无合法空格时拒绝且零写入；不得 Merge、Swap、Replacement、Split、Quantity=0、RequestedMergeQuantity、WorldPickupDraft、PlayerSplitDraft、再扫描其他容器、自动装备、打开 child、切换 child、选择 another BodyTarget 或创建世界目标。
7. Preview 成功后必须冻结 exact target ContainerId/SlotIndex 和 P6 revision。Commit 只可接受同一 target 的 whole-graph Move；若该 target 已不再精确空、source/target revision 或 session stale、容量/slot semantics 改变，则拒绝，不得为保证成功重扫另一个格。

#### 5.4 单一完整图事务、持久化与回滚

1. Preview 成功后只能建立一个 Owner/Run scoped candidate，并且只调用 P20/P1 已有的 whole-graph Move 语义：同一个 parent ItemId、唯一 ChildContainer 与其完整 closure 从 exact P11 ordinary body source 直接进入一个冻结的 P6 BaseQuick target。不得用 simple Move 绕过 closure 验证，不得先落 BaseQuick 再二次 Move，不得调用 Merge、Split、Swap、Equip、Unequip 或创建 graph。
2. Commit 必须沿既有 P12/P3 → P2 → P1 → P11/P6 durable callback。Store 在任何 durable write 前重新验证 command intent、opened BodyTarget identity、source/target stable address、Owner、Run、body/P6 revision、P20 graph closure、BaseQuick empty state、active session 与 lifecycle gate。
3. 同一 accepted candidate 中必须保持 parent、ChildContainer、所有 ItemId/ContainerId、capacity、placement order、stable SpatialChildGuid 与 provenance 身份不变；唯一合法位置变化是 graph root 从 exact P11 ordinary body source 移到 exact P6 BaseQuick target。不得 clone、flatten、new ItemId、new ContainerId、new child、parent-only move、partial commit 或 child snapshot。
4. 只有 accepted snapshot/replay proof 明确证明完整 graph root 已离开 exact P11 source，才可在同一个 Owner candidate 中更新 P11 body snapshot、P6 player snapshot 和 exact BodyTarget source projection。不得预清 source、预写 P6、预删 child、预刷 Code A Actor 或在 save 后补写另一侧。
5. P11 与 P6 必须在同一 Owner durable replacement 中共同提交，且只有一次 SaveRecord。P13 只按既有 accepted commit reconcile；不得自动 Bind、Use、Equip 或复制 binding。P8 仍只结算当时 P6 player graph，并只丢弃 P11 residual；P41 不改变尸体死亡、reveal、receipt、终局或 recovery。
6. P41 绝不创建、删除或写入 P14/P31 WorldDrop record、NextWorldDropOrdinal、WorldDrop Actor、地图落点、空间 child projection之外的对象或任一 Code A inventory path。Code A 若须 lifecycle forwarding，只能只读转发 production BodyTarget projection，不能拥有 Item、Container、Quantity、P11/P6 或 durable authority。
7. candidate、canonical gate、graph validation、target scan、P1 Move、P13 reconcile、projection 前检查或 SaveRecord 任一失败时，必须完整恢复 BeforeSnapshot：P11 source parent/child closure、P6 target、BodyTarget visibility/open state、P7 projection、selection 与无关 body/world record 均保持未变，且不得遗留 phantom empty slot、phantom pickup、图副本或 transient item copy。
8. 成功后只刷新 exact BodyTarget source section、解析得到的 BaseQuick target 和必要关联 P7 projection；不得 Sort、Compact、重排无关 SlotIndex、重建无关空间区域、清空无关选择、改变无关 scroll offset 或改写 P29/P30/P34/P36/P37/P38/P39/P40 语义。

#### 5.5 非回归、终局与权威边界

1. P20 complete graph 的 normal Drag 仍保留其 explicit P12 root → 空 BaseQuick transaction；P41 只添加 Ctrl QuickTransfer，不得收窄、重定向或自动触发 normal Drag。P20 的空间 parent 或其 child 从 P6 回写 BasicCorpse 仍必须拒绝。
2. P40 P12 ordinary simple-stack Ctrl 路径仍只在 valid current child 优先／无 child 才 BaseQuick 的 frozen mode 中按 merge-first / empty-second 运作；P41 不得让 simple stack 进入 BaseQuickOnly whole-graph branch，也不得改变 partial acceptance、remaining ItemId/slot 或数量语义。
3. P39/P38/P21 corpse equipment 的 QuickTransfer 与 explicit Drag 仍按独立 equipment provenance 和 child-priority/BaseQuick semantics 运作；P41 不得让 P21 fixed equipment、P20 parent、P17 child 或任何普通装备跨入彼此的 resolver/transaction branch。
4. P30 P19 WorldDrop whole-graph Ctrl 路径仍只处理 current opened WorldDrop root，采用 existing BaseQuick-only move、P14 record cleanup 与 Actor projection；P41 不得创建 WorldDrop、删除 record、读取 ordinal 或让 P20 corpse source 进入 P30 durable branch。
5. P29 simple-stack ground QuickTransfer、P26—P28 explicit drag/quantity、P31 multi-record isolation、P34/P35/P36/P37 standard equipment ground routes、P17 graph construction、P19 graph semantics、P5/P6 bridge、P8 terminal/recovery、P13 binding、P15 use、P20/P21 source/history 与 Code A authority 均保持既有产品语义。
6. P17 一层无嵌套规则、P20 child empty provenance、P11/P12 Hidden/Searching/Reveal、P21 r1/r2/r3 deterministic identity、candidate/weight/digest 与固定装备位都不得因 P41 被重掷、补料、迁移或改写。

### 6. 允许的改动范围

仅允许在当前 Code B 中最小修改：

- P12 已有 body-aware source proof、shared pointer routing、QuickTransfer preview/commit、P11/P6 durable transaction 与 projection；
- P20/P30 的既有 complete-graph validation、BaseQuick-only resolver、whole-graph Move、rollback 与 accepted proof，仅用于 P41 exact P12 P20 source；
- P39/P40 的既有 transient intent、branch discrimination、source lifecycle、cancellation 与 revision/session proof，仅用于接入 P41；
- P1/P2/P3/P6/P11/P13/P17 的必要声明、candidate proof、rollback 或调用签名兼容，前提是不改变已有产品语义；
- 必要的 production BodyTarget lifecycle forwarding，只能作为 Code B gate 的只读转发；
- PROJECT.md、PROJECT_INFO_CARD.md、本任务 Prompt 归档与本任务 Report。

禁止新建项目、版本线、Fix、第二 P11/P6、Widget inventory、Code A mirror、fixture、假 ItemId、clone、双写、存档重置或历史数据改写。禁止修改 Code A 功能逻辑、P12 search state machine、P20/P21 deterministic source/history、P17 graph construction、P31 schema/registry、P8 receipt/terminal product logic、P5/P6 bridge 或任何测试文件。

### 7. 明确不在本任务内

- 不把 P21 equipment、P12 ordinary simple stack、P20/P19 child item、P14/P31 WorldDrop、warehouse、ordinary container、player source 或任何 other provenance 接入 P41 QuickTransfer。
- 不实现快捷丢弃、P6 → corpse quick return、自动装备、equipment target scan、auto child open/switch、child fallback、Take All、right-click Take、double-click、Actor direct pickup、距离自动拾取、交互键领取、Swap、Replacement、Sort、Compact、bind、use、装备数值、战斗效果、HUD 接管、网络或多人。
- 不改变 P12 normal Drag、P20/P21 source/history、P29/P30/P34/P36/P37/P38/P39/P40 现有 Ctrl semantics、P13/P15、P17/P19、P26—P28、P31、P5/P6/P8、搜索、敌人、地图、战斗、生命、死亡、撤离、商店、经济或制作。
- 不启动产品、PIE、Standalone、真实鼠标键盘输入、截图、Smoke、Automation、回归、试玩、Cook、Package 或最终验收。
- 不在回传 Report 前自动开始 P42、任意 Fix 或 F。

### 8. P 阶段静态审查与编译

完成后只执行以下检查：

1. 审查 P41 复用唯一 Ctrl + 左键 pointer router 与 existing QuickTransfer intent；确认没有专用 input/UI/Actor 写入旁路，且分支只依据 canonical P12 P20 spatial source truth 与 exact BodyTarget identity。
2. 审查 source 只接受 current opened/revealed/exact P12 ordinary P20 WindTalisman/BackpackLevel1 parent graph，并逐项复核 Owner/Run/BodyTarget/death receipt/profile/history/body revision/source container-slot/root/child closure/page/focus/P6 revision；P21 equipment、simple stack、child、P19 world、Hidden/Searching、stale/close/terminal 均零写入。
3. 审查 P17/P20 complete graph closure：唯一 child、formal type/capacity、empty-child provenance、一层无嵌套、无环、无 orphan、无 duplicate owner、stable SpatialChildGuid 与 parent/child identity 均保持。确认 P41 未重掷、补料或改写 P20/P21 profile/history。
4. 审查 input-time BaseQuickOnly resolver：只按真实 stable SlotIndex 在 exact active BaseQuick 内找第一个合法空 ordinary cell；确认不扫描 child、装备位、Hotbar、P5/P9/P11/P14/P31 或 other target，且 candidate stale/满位时不重扫或回退。
5. 审查 candidate 只使用一个 P1/P20 whole-graph Move；确认没有 Merge、Split、Quantity=0、partial graph、new ItemId、Container、receipt、record、ordinal、Actor、child 或第二物品真值。
6. 审查 P11/P6 仅以一个 Owner durable replacement 和一次 SaveRecord 共同提交；确认 P11 source 只在 accepted proof 后更新、P13 reconcile、BeforeSnapshot rollback、P8 terminal/recovery、selection/scroll/stable SlotIndex 与 P17 graph 均保持。
7. 审查 P12 normal Drag、P20/P21 source/history、P30、P39/P40、P29/P34/P36/P37、P31 Registry、P5/P6/P8/P13/P15 与 Code A authority 均无产品语义回归。
8. 审查 right-click、double-click、ordinary Drag、Actor interaction、Shift + 1—9、player-side Ctrl、P12 simple source、P21 equipment、P17 child、P6 → corpse drag 均未获得 P41 隐式位置或完整图写入语义。
9. 编译 Editor：

       "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_mapEditor Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload

10. 编译 Game：

       "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_map Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload

若编译失败，只修复本任务引入的 P12 spatial QuickTransfer routing、source proof、whole-graph resolver、P11/P6 transaction、projection、rollback、include 或签名问题；若必须扩大到本任务以外，停止受影响部分并如实报告。

### 9. Report 与完成信号

生成 Dev.D.UE.0.0.9B.P41.0.r0_report.md，保存至：

    C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Docs\Report

Report 必须简洁、可审计地列出：

1. 本轮新增、修改、未修改的每个文件及职责；
2. P12 ordinary spatial source Cell 到 P41 shared QuickTransfer 的完整调用图，以及没有新增 pointer、Widget、Actor、second resolver 或 Code A 写入口的证据；
3. P20 r2/P21 r3 inherited spatial provenance、actual stable ordinary source slot、BodyTarget/death receipt/profile identity、完整 parent/child closure，以及为什么 P20/P21 materialization/history/determinism 未被改写；
4. exact source gate、Reveal/Open lifecycle、Owner/Run/revision/session proof、BaseQuickOnly 冻结、candidate target freeze 与 zero-write cancellation/stale policy；
5. exact BaseQuick stable SlotIndex 次序、first-empty 解析、capacity validation、没有 child/equipment/Hotbar/second-slot scan、无 fallback/auto equip 的证据；
6. single P1/P20 whole-graph Move、single P11/P6 durable replacement、source/target parent-child identity 保持、P13/P8、rollback、selection/scroll/stable SlotIndex 与无 WorldDrop/ordinal/Actor write 的证据；
7. P12 normal Drag、P20/P21 source/history、P30、P39/P40、P29/P34/P36/P37、P31、P5/P6/P8/P13/P15 与 Code A authority 的非回归结论；
8. 两个编译命令、目标、原生 exit code 与关键结果；
9. 所有未执行的 F 阶段真实验证，至少包括：P20/P21 future corpse materialize/reveal；实际 canonical WindTalisman/BackpackLevel1 Ctrl 拾回至 first legal BaseQuick；空/满 BaseQuick、stable SlotIndex、完整 parent/child identity、child open/switch/close 不成为目标；source wrong/simple/equipment/child/Hidden/Searching；BodyTarget/death receipt/profile/Owner/Run/P6 revision stale；Prepared/terminal/Host invalid/SaveRecord failure；P12 normal Move/Merge/Swap、P20 normal graph Drag、P21/P38/P39、P40、P29/P30/P34/P36/P37 Ctrl branches、P31 other-record isolation、P8 terminal/recovery、真实鼠标键盘、PIE、Standalone、截图、Smoke、Automation、回归、Cook 与 Package。

仅当 P41 P12 P20 complete-graph BaseQuick-only QuickTransfer 静态闭合、P12/P20/P21 与 P17/P19/P29/P30/P31/P38—P40/P8 边界保持，且 Editor 与 Game 均以 native exit code 0 完成时，使用：

    READY_FOR_P42_PLANNING

若当前范围内仍有可修复问题，使用：

    NEEDS_P41_REWORK

若现有 P12/P11/P6 transaction 或 shared QuickTransfer resolver 无法在不创建第二输入路径、第二保存、第二物品真值、自动装备、改写 P20/P21 deterministic history 或扩展 Code A 权威的前提下支持本项，使用：

    NEEDS_PLANNER_DECISION
    BLOCKED

完成后不得自动开始 P42、Fix 或 F。向策划 Chat 回传并附带且只附带本次同名 Report；正文首行使用：

    [CSEMI:REPORT_SENT] {"task_id":"Dev.D.UE.0.0.9B.P41.0.r0","file":"Dev.D.UE.0.0.9B.P41.0.r0_report.md"}
