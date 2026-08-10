# Dev.D.UE.0.0.9B.P50.0.r0

## 任务身份

- 项目：Dev.D.UE.0.0.9B；继续使用同一活动工程，不新建项目。
- 阶段：主线 P50——已打开、已揭示 P10 BasicCache 中 P18 完整空间图的受限正常地面拖放。
- 任务编号：Dev.D.UE.0.0.9B.P50.0.r0。
- 前置：已接受 0.0.9B.P1—P49 与 0.0.9BFix.P1—P4。Fix 只用于已确认、已验收功能的缺陷修复；P50 是新增主线功能，不是 Fix。
- 执行文件：Dev.D.UE.0.0.9B.P50.0.r0_prompt.md。
- 报告文件：Dev.D.UE.0.0.9B.P50.0.r0_report.md。
- 活动工程根：C:\\AIDev\\shanmen-ue\\Dev.D.UE.0.0.9B。
- 活动工程：C:\\AIDev\\shanmen-ue\\Dev.D.UE.0.0.9B\\demo_map.uproject。
- Report 必须生成到：C:\\AIDev\\shanmen-ue\\Dev.D.UE.0.0.9B\\Docs\\Report\\Dev.D.UE.0.0.9B.P50.0.r0_report.md，并仅携带该同名 Report 回传策划 Chat。
- 任务性质：P 阶段只做实现、静态审查和代码编译。不得启动产品、PIE、Standalone、真实鼠标键盘验证、截图、Smoke、Automation、回归、Cook、Package 或 F 阶段测试；不得自动开始 P51、任意 Fix 或 F。

## 上半部分：只读项目裁决、现状与边界

### 1. 当前唯一有效基线

唯一有效依据是当前 0.0.9B、活动工程和已接受任务链。所有 0.2、V2、V3、I、IPF、历史页面壳、旧 CTA、旧库存和旧物品规则均已过时；不得读取、采用、恢复或以其决定实现、验收或范围。

Code B P1 Repository 与既有 durable Store 是唯一可变物品真值。每件物品始终只有一个真实 ItemId、一个真实父位置和一条权威事务链。Widget、Cell、Presenter、DragOperation、Workspace Context、NormalContainerTarget、WorldDrop Actor、地图放置适配层和 Code A 都只能持有只读投影、选择或瞬时意图；不得持有第二库存、可写数量副本、预建 ItemId、平行容器背包、Actor-first 写入或 A/B 双写。

P9/P10 已建立唯一生产普通容器 M01.CodeBNormalContainer.BasicCache.01、其 exact BasicCache Run-local record，以及 Hidden → Searching → Revealed、Open、focus/page 与 normal Drag/Drop 生命周期。P18 仅为尚未物质化的 BasicCache r2 提供确定性的 optional spatial utility：canonical `WindTalisman` 或 `BackpackLevel1` parent 及其唯一、空的正式 ChildContainer。已物质化 record 必须始终按其 actual receipt、ProfileId、ProfileVersion、ProfileDigest、AlgorithmVersion 与 ResultDigest 保持原样；不得 reroll、补料、迁移、回写候选权重或改写历史。

P17 已建立唯一正式空间 parent → ChildContainer 图、一层无嵌套限制和当前 child 投影。空间 parent 及其 child closure 不得进入任一 P17 child，也不得形成自身或祖先回边。P47 已为同一 P18 complete graph 增加 `BaseQuick`-only Ctrl + 左键快捷拾回；P48 已为其增加用户明确的兼容装备位 normal Drag；P49 仅将 `SpiritDust`／`IronShard` simple stack 接入 BasicCache → WorldDrop。P50 不改变其中任何一路。

P14/P19/P30/P31 已确立 complete spatial graph 的地面记录与拾回语义：每个 `WorldDrop` record 只含一个 root、一个 derived world container、一个 WorldDropId、一个 Ordinal 和一个 Actor projection。已打开、identity-valid 的 complete-graph record 仅可复用既有 P19 normal pickup 到用户明确的空 `BaseQuick` ordinary storage cell，或按 Definition 显式拖到兼容空装备位：`WindTalisman → SpatialRing`、`BackpackLevel1 → Backpack`；P30 的 Ctrl + 左键只允许冻结后移入首个合法空 `BaseQuick` ordinary cell。完整空间图不得进入任何 current child、不得自动装备，也不得触及 P31 以外的第二记录真值。

P43—P45 已覆盖尸体 simple stack、standard equipment 与 complete spatial graph 的 normal GroundDrop；P50 只将相同的安全模型接入 exact P9/P10/P18 BasicCache source。P26—P29 仍只处理 simple world root；P50 的 complete graph 不得伪装成 simple stack 或进入其 Merge、partial quantity、QuickTransfer 语义。

### 2. P50 产品裁决

当玩家已经主动打开 identity-valid 的唯一 P10 BasicCache，且其中一个 ordinary root 已 Revealed，并被 canonical P18/P17 truth 证明为 `WindTalisman` 或 `BackpackLevel1` 的完整空间图时，玩家可以从该 parent Cell 发起既有 normal DragOperation，并把它 Drop 到既有 GroundDropZone。成功时，**同一个 parent、其唯一 ChildContainer 与完整 closure**必须以一次 P1/P18 whole-graph relocation 从 exact P9 source 直接进入新建、独立 P31 WorldDrop record 的 derived world container slot 0。

| 明确动作 | 唯一允许来源 | 唯一允许目标 | 权威结果 |
| --- | --- | --- | --- |
| BasicCache 完整空间图主动落地 | 已打开、已揭示、identity-valid P10 BasicCache 中的 P18 canonical `WindTalisman` 或 `BackpackLevel1` parent root | 既有 GroundDropZone | 一个 P1/P18 whole-graph Move；同一 Owner durable replacement 同时更新 P9 source、P31/P6 new record，并只保存一次。 |
| 地面 normal pickup | 当前已打开、identity-valid 的 P50 single-root complete-graph record | 仅复用 P19 已有的用户明确空 `BaseQuick`，或匹配 Definition 的明确空 `SpatialRing`／`Backpack` | 不创建 P50 专用输入；只使已接受 P19 的 exact-record normal pickup 按完整图身份处理该 root。 |
| 地面 Ctrl + 左键 pickup | 当前已打开、identity-valid 的 P50 record | 仅复用 P30 的 frozen `BaseQuickOnly` first-empty rule | 不进入 child、不装备、不扫描第二个目标、不产生 fallback。 |

P50 不是数量拆分、partial graph drop、快捷丢弃、自动拾取、Take All、自动装备、自动打开空间道具、容器回存或 Actor direct pickup。每个普通拖拽最多建立一个 Owner/Run scoped candidate，并只提交一次完整图事务。新 record 必须与所有既有 record 物理和身份隔离；不得合并、替代或猜测任一已打开的 WorldDrop。

### 3. 严格范围与持续排除

P50 source 只接受同时满足以下全部条件的完整图：

1. 当前 Workspace 是既有 P10 production Host；exact NormalContainerTarget 为 Open、identity-valid，仍属于 active M01 BasicCache route、focus、target-open generation 与 active session，且无与该 source 冲突的 active action/search。
2. parent 位于 exact P9 RunLocalNormalContainerRecord 的 ordinary stable `ContainerId + SlotIndex`；P9 receipt/history、P10 target projection、Catalog、P1 snapshot 与 source reverse pointer 共同证明它属于 P18 r2 `Optional.SpatialUtility` canonical result。不得以显示名、图标、Cell class、Actor pointer、选择态、缓存或展示顺序判断。
3. root DefinitionId 只能是正式 `WindTalisman` 或 `BackpackLevel1`。parent 必须持有同一 ItemId 对应的唯一正式 ChildContainer；parent、child ContainerId、stable SpatialChildGuid、child type、capacity、slot semantics、child-empty provenance、one-layer/no-cycle/no-orphan/no-duplicate-child-owner 资格必须由 canonical P17/P18 durable graph 验证。
4. P18 actual profile/receipt/digest、P9 source record、parent/child closure、Reveal/Open、OwnerId、RunInstanceId、SearchTargetId、P9/P6/composite revision、route、focus、target-open generation、active session、Prepared/terminal gate 与 Host validity 在 Preview 和 durable Commit 前均必须可重验。
5. GroundDropZone 的 route/floor placement、P31 record candidate、Registry collision gate 与 `SaveRecord` gate 在 Preview 和 Commit 前也必须成立。

下列对象或动作持续不属于 P50：

1. `SpiritDust`／`IronShard` simple stack、P10 Hidden/Searching source、P18 graph 的 child item、unknown complex parent、P21 fixed Weapon/ArmorRobe/Accessory0、P11/P12 corpse、P14/P31 existing WorldDrop、P5 warehouse、P6 player source、P17 child item、Hotbar、another NormalContainer 或 UI-only object。
2. Ctrl + 左键、QuickTransfer、quick-drop、P6 → BasicCache quick return、Actor direct pickup、距离自动拾取、交互键领取、right-click Take、double-click、Take All、auto equip、auto child open/switch、auto bind、auto use、Swap、Replacement、Sort、Compact、Merge、Split、Quantity=N、WorldPickupDraft、PlayerSplitDraft、record-to-record transfer 或 parent/child partial transfer。
3. P10 ordinary BasicCache → P6 normal Move/Merge/Swap、P46 BasicCache simple-stack Ctrl、P47/P48 spatial player paths、P49 simple GroundDrop、P26—P29 simple WorldDrop pickup、P38—P45 corpse paths、P31 schema/Registry 产品语义、P8 terminal classification、P13 binding、P15 use、P17 graph construction 和 Code A authority。
4. 实机运行、PIE、Standalone、真实鼠标键盘、截图、Smoke、Automation、回归、Cook、Package 或最终验收。

## 下半部分：授权执行内容

### 4. 单一授权目标

在不建立第二物品真值、不改变 P9/P10 normal-container state machine、P18 deterministic materialization/history、P17 graph、P19/P30 existing complete-world pickup、P31 multi-record Registry、P47/P48 player-side spatial paths 或 Code A authority，也不新增任何自动行为的前提下，将 current opened/revealed exact P10 P18 spatial parent root 接入既有 GroundDropZone / P31 流程：

1. 只有 current opened/revealed, identity-valid 的 exact P10 P18 `WindTalisman`／`BackpackLevel1` complete-graph parent Cell 才可经 normal Drag 建立一次 P50 world-drop transient intent。
2. Preview 只建立一个 Owner/Run scoped candidate，冻结 exact P9 source graph、完整 child closure、accepted route/floor placement 与 P31 candidate identity；Commit 不得重扫 source、改投 P6、改投另一 NormalContainer 或 fallback 到旧 WorldDrop。
3. accepted path 只形成一个 P1/P18 whole-graph relocation、一个 P31 new-record insertion、一个 P9/P6 Owner durable replacement 与一次 `SaveRecord`。
4. 只有 accepted snapshot/replay proof 明确证明完整 graph 已离开 exact P9 source 并进入新 record 的 exact derived world container slot 0 后，才可刷新 NormalContainerTarget source、record、Actor projection 与必要 workspace projection。
5. 任一 source、graph、floor、record、identity、session、close/stale、P1/P18 或保存检查失败时完整零写入，并以 `BeforeSnapshot` 恢复原有 source closure、P31 Registry 和 Actor projection。

### 5. 实现要求

#### 5.1 先完成活动调用链与资格审计

改动前必须审阅并在 Report 中列出：

1. P10 NormalContainerTarget open/reveal lifecycle、BasicCache ordinary root Cell、normal DragOperation、GroundDropZone 可达性、NativeOnDrop、page/focus invalidation、P9/P6 callback 与 source proof；说明 P50 如何复用既有生产 Cell 和 GroundDropZone，而不是新增 Button、hotkey、pointer handler、Widget、Actor 或 normal-container 写入口。
2. P18 optional spatial result 的 actual profile/receipt/digest、parent-child graph materialization、P17 topology 与既有 P10 explicit whole-graph transaction；说明 P50 不会改写 r1/r2 history、gate、weight、candidate、ItemId、ChildContainer 或实际已物质化结果。
3. P14/P19/P30/P31 的 GroundDrop request、floor placement、new-record candidate、WorldDropId/Ordinal、derived world container、Actor diff、exact target open、normal/Ctrl pickup、record cleanup、recovery 与 P8 player-only filtering 调用图；说明 P50 如何加入 canonical complete-world-root source classification，而不是新建 P50-special pickup。
4. P1/P2/P3/P6/P9/P13/P17/P18 的 whole-graph Move、cross-graph single candidate、accepted snapshot/replay proof、BeforeSnapshot rollback、P9/P6 single Owner durable replacement、P13 reconcile 与 revision/session 调用图。
5. 所有 Widget direct Move/graph mutation、Actor direct pickup、NormalContainer display-cache write、right-click Take、double-click、预建 ItemId/ContainerId/WorldDropId/Ordinal、按 first/last/current record 猜测 target、第二 Repository transaction、第二 save 或 Code A inventory writer；它们不得成为 P50 写入路径。

#### 5.2 共享 normal Drag、source gate 与瞬时 proof

1. P10 P18 complete-graph parent Cell 的既有 normal `DragOperation` 与 `GroundDropZone::NativeOnDrop` 或当前等价正式入口仍是 P50 唯一输入与提交入口。不得为 P50 新建 Button、hotkey、Actor click、专用 Widget、第二 pointer handler、second resolver 或 UI direct write。
2. ordinary left click 继续只选择或按 P10 既有规则开始 search；right-click 继续只读详情；Ctrl + 左键继续只走 P47 或其他已授权 QuickTransfer route；double-click、Tab、I、Esc、Close、Cancel、scroll、空白区、无 payload Drop、Shift + 1—9、player-side drag 与任何 Code A interaction 均不得形成 P50 位置或图写入。
3. source payload 只能由 current Open、Revealed、identity-valid exact P10 P18 complete graph 建立。它必须冻结 OwnerId、RunInstanceId、SearchTargetId、ReceiptId、BasicCache DefinitionId、P9 record revision、source ContainerId、source SlotIndex、parent ItemId、ChildContainerId、stable SpatialChildGuid、DefinitionId、P18 actual profile/receipt/digest、P6 revision、target-open generation、route/focus 与必要 active-session proof。
4. Preview 与 durable Commit 前必须重新验证 parent 仍在同一 exact P9 ordinary BasicCache slot、visibility 仍为 Revealed、P18 complete-graph qualification 和 receipt/history provenance 不变、NormalContainerTarget 仍 Open，且 Owner/Run/session/Prepared/terminal/Host/route/focus/revision gate 全部成立。不得让 simple stack、P18 child、P11/P12 corpse、P14/P31、player source、another NormalContainer 或 UI projection 建立 P50 intent。
5. NormalContainerTarget close/reopen、focus loss、开始/取消 search、Actor EndPlay、map reload、recovery、terminal、Prepared、receipt/root/container/child mismatch、payload cancel、Host invalidation、source revision stale、floor placement stale、Registry collision 或 `SaveRecord` failure 必须立即使 intent/candidate 失效并零写入。不得改投 BaseQuick、P17 child、equipment slot、旧 WorldDrop、另一 NormalContainer 或其他位置。
6. payload 不得包含可写 quantity cache、split amount、child snapshot 或预建 world identity。它只能代表一个 existing complete graph；不得调用 simple Move 绕过 closure、Merge、Split、RequestedMergeQuantity、WorldPickupDraft、PlayerSplitDraft 或任何 partial-transfer shortcut。

#### 5.3 exact P9 P18 complete graph → new P31 WorldDrop record

1. GroundDropZone 先只通过既有 Code A floor-placement adapter 解析合法、不可变的 route/floor placement；该 adapter 只能转发展示和生命周期边缘，不得创建 ItemId、ContainerId、WorldDropId、record、ordinal、数量、P9 graph 或 P6 graph。
2. Preview 成功后只建立一个 Owner/Run scoped candidate。candidate 必须以一个 P1/P18 whole-graph relocation，将同一个 parent、其唯一 ChildContainer 与完整 closure 从 exact P9 P18 source 直接移入新 record 的 derived world container slot 0；不得先落 P6 BaseQuick、先清 P9、先建立可见 Actor、先写临时 container 或执行 two-step relocation。
3. accepted graph 的 parent/child ItemId、ContainerId、DefinitionId、Quantity、Level、Quality、RandomSeed、LegacyAffixDigest、P18 actual profile/receipt/digest、capacity、placement order、stable SpatialChildGuid、one-layer/no-cycle/no-orphan/no-duplicate-child-owner 资格必须原样保持。不得 Merge、Split、Swap、clone、new ItemId、new ContainerId、new child、flatten、parent-only move、temporary player item 或第二物品真值。
4. `NextWorldDropOrdinal`、derived world container、WorldDropId、record insertion、P9 source removal、P6/P14 record update、P13 necessary reconcile 和 Actor projection 只能在同一个 accepted Owner durable replacement 中出现，并且函数内只有一次 `SaveRecord`。Actor 仅在 accepted durable projection 后创建或刷新。
5. 新 record 必须保留 P31 exact identity：OwnerId、RunInstanceId、WorldDropId、Ordinal、map route/floor placement、Available state、derived ContainerId、root parent ItemId、record revision 与 P50 spatial-BasicCache provenance。实际 provenance literal 可沿活动 schema 命名，但必须独立、可审计，不得与 P19 player world source、P20 corpse direct pickup、P43—P45 corpse drop 或 P49 simple BasicCache drop 伪装或混用。
6. Store 必须以 accepted command 的相同 TransactionId、source、target、closure、revision 与 record proof 重放一次 P1/P18 whole-graph relocation，并要求 replay snapshot 与 accepted composite 完全相等；只有 replay 证明整个 graph 已离开 exact P9 source 并到达 exact new world container 后，才刷新 BasicCache source 与 P31 projection。
7. candidate、source/graph/placement/Registry validation、P1/P18 relocation、P13 reconcile、record/Actor projection 前检查或 `SaveRecord` 任一失败时，完整恢复 `BeforeSnapshot`：P9 source parent/child closure、P6/P14 Registry、NextWorldDropOrdinal、existing records、new record、derived container、Actor、NormalContainerTarget visibility/open state、selection 与无关 projection 均保持旧值；不得遗留 phantom empty、phantom world root、ordinal gap、orphan container、orphan child、duplicate child owner 或 transient item copy。

#### 5.4 P50 record 与既有 complete-graph 拾回链的受限衔接

1. P50 成功创建的 record 只能以 current opened、identity-valid、exact P31 record 的 canonical P19 complete-graph root 身份参与既有拾回。所有 admission 必须验证 OwnerId、RunInstanceId、WorldDropId、Ordinal、record revision、derived container、parent root ItemId、child closure、route、focus、target-open generation、P6 revision、P50 provenance、record availability 与 active session；不得由 Actor、list order、selection 或其他 record 推断。
2. normal Drag pickup 只可复用既有 P19 complete-graph 的明确目的地 policy：用户明确 Drop 的空 `BaseQuick` ordinary storage cell；或 `WindTalisman` 的用户明确 Drop 的空、正式、Definition-compatible `SpatialRing`；或 `BackpackLevel1` 的用户明确 Drop 的空、正式、Definition-compatible `Backpack`。parent 及其 child 不得进入任何 P17 child。目标失效、占用、不兼容、child topology stale、focus loss/open-generation mismatch、record close/stale 或保存失败时一律零写入；不得自动选第一个装备位、自动打开/切换 child、自动回退、Swap、Replacement、flatten、partial pickup 或 BasicCache 回存。
3. Ctrl + 左键只复用 P30 已有的 shared pointer router、complete-graph QuickTransfer transient intent 与 `BaseQuickOnly` frozen mode。P50 root 在输入时只能冻结并移向当前 P6 BaseQuick 中按真实 stable SlotIndex 升序的第一个合法空 ordinary storage slot；它不得进入当前 P17 child、任何装备位、Hotbar、P5/P9/P11/P14/P31、另一 WorldDrop 或任何 fallback。target 随后失效、已满或 record stale 时必须拒绝，绝不重扫或静默切换。
4. P50 不得新建 P50-special pointer router、second QuickTransfer resolver、target scan、hotkey 或 UI handler。实现只能在已有 P19/P30 canonical complete-world-root source classification 中加入 exact P50 record proof，并保持 P19/P30 现有 source discrimination、normal Drag 和 Ctrl 行为不变。
5. 每个 accepted P50 pickup 只形成一个 P1/P19 whole-graph relocation、一个 exact record cleanup、一个 Owner durable replacement 与一次 `SaveRecord`。P31 record/container/Actor 仅在 accepted snapshot/replay 明确证明整个 graph 已离开 exact derived world container 后清理；partial/failed/stale pickup 一律保留相同 record、root、closure、WorldDropId、Ordinal、Actor 与 other records。

#### 5.5 单一事务、终局、回滚与非回归

1. P50 BasicCache → world Drop 必须沿既有 P10/P3 → P2 → P1 → P9/P14/P31/P6 durable callback；每次 input 最多一个 candidate、一次 Owner record replacement 与一次 `SaveRecord`。不得先写 P9 后写 P6，也不得先写 record/Actor 后写 source。
2. P13 只沿既有 accepted commit reconcile；不得自动 Bind、Use、Equip、打开 child 或复制 binding。P15 保持现有 RestoreHealth-only scope。
3. P8 `BuildP14PlayerOnlySession` 或等价 finalization 必须把 P50 record 的 root、child closure、derived world container 及所有 other Registry records 从 player-only final graph 排除。P50 不修改 terminal classification、P5 receipt、recovery、run replacement 或 Code A terminal authority；仍留在 P9 的 BasicCache residual 继续只按 P9 residual discard 处理。
4. repeated Actor refresh、map reload、active P6 recovery、Actor EndPlay、close、lost focus 和 duplicate terminal observer 只能处理 matching transient projection，不能改变 durable item graph、删除 record、回填 BasicCache 或把 Actor 作为真值。
5. 成功后只刷新 exact NormalContainerTarget source、new/exact WorldDropTarget、matching Actor 与必要 P6/P17/P19 projection；不得 Sort、Compact、重排无关 SlotIndex、重建无关空间区域、清空无关选择、改变无关 scroll offset，或改变 P10/P18/P47/P48/P49/P19/P30/P31/P43—P45 的既有产品语义。

### 6. 允许的改动范围

仅允许在当前 Code B 中最小修改：

- P10/P18 已有 complete-graph source proof、shared Drag payload、normal Drop preview/commit、P9/P6 durable transaction 与 projection，仅用于 P50 exact BasicCache spatial source；
- P14/P19/P30/P31 所在的 GroundDrop candidate、canonical complete-graph eligibility、record insertion、accepted pickup proof、Actor projection、P13 reconcile、P8 player-only filtering 与 rollback，仅用于 P50 accepted single-root record；
- P1/P2/P3/P6/P9/P13/P17/P18/P19 的必要声明、candidate proof、replay、rollback 或调用签名兼容，前提是不改变已有产品语义；
- 必要的 production GroundDropZone / floor-placement / Actor lifecycle forwarding，只能是 Code B gate 的只读转发；Code A 不得取得库存、Loot、数量、WorldDrop、graph 或 durable authority；
- `PROJECT.md`、`PROJECT_INFO_CARD.md`、本任务 Prompt 归档与本任务 Report。

禁止新建项目、版本线、Fix、第二 P9/P6、Widget inventory、Code A mirror、fixture、假 ItemId、clone、双写、存档重置或历史数据改写。禁止修改 Code A 功能逻辑、P10 search state machine、P18 profile/history、P17 graph construction、P31 schema/registry、P8 receipt/terminal product logic、P5/P6 bridge 或任何测试文件。

### 7. 明确不在本任务内

- 不把 P10 simple stack、P18 child、P11/P12 corpse、P20/P21 corpse graph/equipment、P14/P31 existing WorldDrop、P5 warehouse、P6 player item、P17 child item、Hotbar、any other NormalContainer 或 unknown root 接入 P50 BasicCache-drop source。
- 不实现 BasicCache partial graph drop、P6 → BasicCache return、Ctrl quick-drop、Actor direct pickup、自动拾取、Take All、right-click Take、double-click、auto target、auto equipment、auto child open/switch、space parent/child 地面分拆操作、world multi-item container、record-to-record transfer、cross-record merge、Swap、Replacement、Sort、Compact、Bind、Use、装备数值、战斗效果、HUD 接管、网络或多人。
- 不改变 P10 normal BasicCache → player Move/Merge/Swap、P46/P47/P48/P49、P19/P30 normal/Ctrl world path、P43—P45、P26—P29、P31 Registry identity、P13/P15、P17/P18、P5/P6/P8、搜索、敌人、地图、战斗、生命、死亡、撤离、商店、经济或制作。
- 不启动产品、PIE、Standalone、真实鼠标键盘输入、截图、Smoke、Automation、回归、试玩、Cook、Package 或最终验收。
- 不在回传 Report 前自动开始 P51、任意 Fix 或 F。

### 8. P 阶段静态审查与编译

完成后只执行以下检查：

1. 审查 P50 唯一合法 source 为 current opened/revealed/exact P10 P18 `WindTalisman`／`BackpackLevel1` complete graph；P10 simple stack、P18 child、P11/P12 corpse、P14/P31、player source、another NormalContainer、Hidden/Searching 与 unknown family 均不能进入 P50。
2. 审查 normal Drag 至既有 GroundDropZone 是 BasicCache complete graph → world 的唯一输入；确认无新 pointer、Widget、Actor、quantity/child draft、QuickTransfer、right-click、double-click 或 Code A 写入旁路。
3. 审查 accepted drop path 只包含一个 P1/P18 whole-graph relocation、一个 P31 new-record candidate、一个 P9/P6 Owner durable replacement、一次 `SaveRecord` 与 accepted-only Actor projection；确认没有 P6 staging、simple Move、Merge、Split、new ItemId/ContainerId/child、second truth、actor-first write 或 ordinal gap。
4. 审查 exact P18 source proof、complete closure、BasicCache receipt/profile/candidate identity、Reveal/Open lifecycle、Owner/Run/revision/session/floor proof、P31 WorldDropId/Ordinal/container/root identity、P13 reconcile、`BeforeSnapshot` rollback 与 zero-write stale policy；任一失败不得改变 P9、Registry、Actor 或 other record。
5. 审查 P50 record 的 normal pickup 仅允许明确空 `BaseQuick`、明确空 compatible `SpatialRing`／`Backpack`；Ctrl 仅通过现有 P30 shared frozen `BaseQuickOnly` router 到一个 ordinary slot。确认没有 child entry、auto equip、target guess、mode fallback、new resolver 或 BasicCache 回存。
6. 审查 accepted P50 pickup 只清理 exact record，且 P19/P30、P47/P48/P49、P43—P45、P26—P29、P31 other-record isolation 均保持；P50 record 不以 parent/child topology 破坏 single-root WorldDrop 语义。
7. 审查 P18 r1/r2、already materialized records、deterministic identity、optional spatial catalog、candidate/weight/digest、`SpatialChildGuid`、P17 one-layer restriction、Hidden/Searching/Reveal 与 P6 → BasicCache rejection 均未被改写；P8 terminal/recovery、P13/P15、P5/P6 bridge 与 Code A authority 保持。
8. 运行 `git diff --check`，并在 Report 中列出实际修改文件、权威写入调用图与所有未修改的权威边界。
9. 编译 Editor：

       "C:\\Program Files\\Epic Games\\UE_5.8\\Engine\\Build\\BatchFiles\\Build.bat" demo_mapEditor Win64 Development "C:\\AIDev\\shanmen-ue\\Dev.D.UE.0.0.9B\\demo_map.uproject" -WaitMutex -NoHotReload

10. 编译 Game：

       "C:\\Program Files\\Epic Games\\UE_5.8\\Engine\\Build\\BatchFiles\\Build.bat" demo_map Win64 Development "C:\\AIDev\\shanmen-ue\\Dev.D.UE.0.0.9B\\demo_map.uproject" -WaitMutex -NoHotReload

若编译失败，只修复本任务引入的 P18 spatial source proof、GroundDrop/P31 transaction、P19/P30 complete-graph pickup admission、projection、rollback、include 或签名问题；若必须扩大到本任务以外，停止受影响部分并如实报告。

### 9. Report 与完成信号

生成 `Dev.D.UE.0.0.9B.P50.0.r0_report.md`，保存至：

    C:\\AIDev\\shanmen-ue\\Dev.D.UE.0.0.9B\\Docs\\Report

Report 必须简洁、可审计地列出：

1. 本轮新增、修改、未修改的每个文件及职责；
2. P18 spatial root Cell 到 GroundDropZone、P50 source proof、P1/P18/P9/P31 durable transaction 和 accepted-only Actor projection 的完整调用图，以及没有新增 pointer、Widget、Actor、second resolver 或 Code A 写入口的证据；
3. P18 actual stable ordinary source provenance、parent/child closure、BasicCache receipt/profile/candidate identity，以及为什么 materialization/history/determinism 未被改写；
4. exact source gate、Reveal/Open lifecycle、Owner/Run/revision/session/floor proof、P50 record identity 与 zero-write cancellation/stale policy；
5. one P1/P18 whole-graph relocation、single P31 new-record insertion、single P9/P6 durable replacement、single `SaveRecord`、replay proof、parent/child/closure/provenance 保持、P13/P8、rollback、selection/scroll/stable SlotIndex 与无 P6 staging/second truth 的证据；
6. P50 exact record 如何仅接入既有 P19 normal pickup 和 P30 `BaseQuickOnly` Ctrl router；明确 `BaseQuick`／`SpatialRing`／`Backpack` target 验证、无 child entry/auto equip/no fallback/no P50-special input 的证据；
7. P47/P48/P49、P19/P30、P43—P45、P26—P29、P31、P5/P6/P8/P13/P15/P17/P18 与 Code A authority 的非回归结论；
8. `git diff --check` 结果、实际改动范围及所有明确未实现范围；
9. 两个编译命令、目标、原生 exit code 与关键结果；
10. 所有未执行的 F 阶段真实验证，至少包括：future BasicCache materialize/open/reveal；canonical `WindTalisman`／`BackpackLevel1` complete graph 各自 normal Drag 至 GroundDropZone；multiple existing WorldDrop isolation；new Actor open/close；P50 record normal Drag 至 BaseQuick／compatible SpatialRing／Backpack；P50 record Ctrl 至 BaseQuickOnly；empty/full/wrong BaseQuick／slot、child closure/topology stale；wrong/simple/child/equipment/Hidden/Searching/player/another-normal source；BasicCache receipt/profile/candidate/Owner/Run/P6 revision stale；floor placement/record/ordinal/container/root/child mismatch；close/reopen、search/action、Prepared/terminal/Host invalid/SaveRecord failure；P19/P30/P47/P48/P49、P43—P45、P26—P31、P8 terminal/recovery、真实鼠标键盘、PIE、Standalone、截图、Smoke、Automation、回归、Cook 与 Package。

仅当 P50 BasicCache P18 complete graph → independent WorldDrop 静态闭合、existing P19 normal pickup/P30 QuickTransfer chain、P31 identity、P9/P10/P18/P17 与 P8 边界保持，且 Editor 与 Game 均以 native exit code `0` 完成时，使用：

    READY_FOR_P51_PLANNING

若当前范围内仍有可修复问题，使用：

    NEEDS_P50_REWORK

若现有 P10/P9/P6/P14/P31 transaction 或 P19/P30 complete-graph resolver 无法在不创建第二输入路径、第二保存、第二物品真值、拆分 child graph、自动装备、改写 P18 deterministic history 或扩展 Code A authority 的前提下支持本项，使用：

    NEEDS_PLANNER_DECISION
    BLOCKED

完成后不得自动开始 P51、Fix 或 F。向策划 Chat 回传并附带且只附带本次同名 Report；正文首行使用：

    [CSEMI:REPORT_SENT] {"task_id":"Dev.D.UE.0.0.9B.P50.0.r0","file":"Dev.D.UE.0.0.9B.P50.0.r0_report.md"}
