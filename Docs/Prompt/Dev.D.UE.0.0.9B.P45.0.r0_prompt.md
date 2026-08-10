# Dev.D.UE.0.0.9B.P45.0.r0

## 任务身份

- 项目：`Dev.D.UE.0.0.9B`；继续使用同一活动工程，不新建项目。
- 阶段：主线 P45——已揭示 P20 尸体完整空间图的显式地面落地，以及与既有 complete-graph WorldDrop 拾回链的受限衔接。
- 任务编号：`Dev.D.UE.0.0.9B.P45.0.r0`。
- 前置：已接受 `0.0.9B.P1—P44` 与 `0.0.9BFix.P1—P4`。Fix 只用于已确认、已验收功能的缺陷修复；P45 是新增主线功能，不是 Fix。
- 执行文件：`Dev.D.UE.0.0.9B.P45.0.r0_prompt.md`。
- 报告文件：`Dev.D.UE.0.0.9B.P45.0.r0_report.md`。
- 活动工程根：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B`。
- 活动工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`。
- Report 必须生成到：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Docs\Report\Dev.D.UE.0.0.9B.P45.0.r0_report.md`，并仅携带该同名 Report 回传策划 Chat。
- 任务性质：P 阶段只做实现、静态审查和代码编译。不得启动产品、PIE、Standalone、真实鼠标键盘验证、截图、Smoke、Automation、回归、Cook、Package 或 F 阶段测试；不得自动开始 P46、任意 Fix 或 F。

## 上半部分：只读项目裁决、现状与边界

### 1. 当前唯一有效基线

唯一有效依据是当前 `0.0.9B`、活动工程和已接受任务链。所有 `0.2`、`V2`、`V3`、`I`、`IPF`、历史页面壳、旧 CTA、旧库存和旧物品规则均已过时；不得读取、采用、恢复或以其决定实现、验收或范围。

Code B P1 Repository 与既有 durable Store 是唯一可变物品真值。每件物品始终只有一个真实 `ItemId`、一个真实父位置和一条权威事务链。Widget、Cell、Presenter、DragOperation、Workspace Context、BodyTarget、WorldDropTarget、空间 child 投影、WorldDrop Actor、地图放置适配层和 Code A 都只能持有只读投影、选择或瞬时意图；不得持有第二库存、可写数量副本、预建 `ItemId`、预建 `WorldDropId`、预建 `Ordinal`、平行尸体背包、Actor-first 写入或 A/B 双写。

P11/P12 已建立唯一 `BasicCorpse` 的死亡回执、首次物质化、`Hidden → Searching → Revealed`、已打开的 `BodyTarget`，以及 P11/P6 的单次 durable transfer。P20 在 future `BasicCorpse` 的 ordinary body-storage 中建立 canonical `WindTalisman` 或 `BackpackLevel1` parent，连同其唯一 P17 `ChildContainer` 与完整合法 closure；它是不可拆分的完整空间图。P21 r3 只继承 P20 的 optional spatial group；P45 不得重掷、改写或迁移 P20/P21 的 receipt、profile、candidate、history、deterministic `ItemId`、`SpatialChildGuid` 或已经 materialized 的结果。

P19 已使相同完整图能够在 P6 与 single-root `WorldDrop` 之间经真实 normal Drag/Drop 原子往返；它的 `WorldDropTarget` 只投影 root，不是 child-item 面板、第二背包或多物品地面容器。P30 已让当前已打开的 P19 complete-graph WorldDrop 通过 `Ctrl + 左键`只移入首个合法空 `BaseQuick` 普通格。P20 已允许尸体 complete graph 通过明确 normal Drag 进入空 `BaseQuick`；P41 已为尸体 source 提供 `BaseQuick`-only 快捷拾回；P42 已允许尸体 complete graph 通过明确 normal Drag 直接进入对应空 `SpatialRing`／`Backpack`。上述直达玩家路径保持完全有效。

P31 已将地面状态收敛为 Owner/Run scoped 多 record Registry：每条 record 永远只包含一个 root、一个 derived WorldDrop container、一个 `WorldDropId`、一个 `Ordinal` 与一个 Actor projection。record 的 create、open、pickup、cleanup、recovery、Actor diff 与 P8 terminal exclusion 都只按 exact record identity 处理，绝不以 first、last、current selection、显示次序或 Actor pointer 猜测。

P43 已让 P12 ordinary simple stack 落地，P44 已让 P21 fixed standard equipment 落地。P45 只补齐仍未覆盖的 P20 complete-graph corpse source，不重做 P20/P41/P42、P19/P30、P43/P44 或任何已有拾回路径。

### 2. P45 产品裁决

当玩家已经通过 P12 主动打开 identity-valid 的唯一 `BasicCorpse`，且一个 ordinary body-storage root 已 `Revealed` 并被 P20 canonical provenance 证明为 `WindTalisman` 或 `BackpackLevel1` 的 complete graph，玩家可以从该 root Cell 发起既有 normal `DragOperation`，并把它 Drop 到既有 `GroundDropZone`。成功时，同一个 parent、其唯一 child container 与完整 closure 以一次 P1/P19 whole-graph relocation 从 exact P11 corpse source 直接移入一个新建、独立 P31 WorldDrop record 的 derived WorldDrop container slot 0。

| 明确动作 | 唯一允许来源 | 唯一允许目标 | 权威结果 |
| --- | --- | --- | --- |
| 尸体完整图主动落地 | 当前已打开、已揭示、identity-valid P12 ordinary body-storage 中的 exact P20 `WindTalisman` 或 `BackpackLevel1` complete graph | 既有 `GroundDropZone` | 一个 P1/P19 whole-graph relocation；同一个 Owner durable replacement 同时更新 P11 source 与 P31/P6 new record，并只保存一次。 |
| 地面普通明确拾回 | 当前已打开、identity-valid 的 exact P45 complete-graph WorldDrop | 用户明确 Drop 的空 `BaseQuick` ordinary storage cell，或该 Definition 唯一匹配的空正式 `SpatialRing`／`Backpack` 装备位 | 复用既有 P19 complete-graph normal Drag 的一个 P1/P19 whole-graph relocation；只清理该 exact record。 |
| 地面 Ctrl + 左键快捷拾回 | 当前已打开、identity-valid 的 exact P45 complete-graph WorldDrop | 当前 P6 `BaseQuick` 中按真实 stable `SlotIndex` 升序的第一个正式、可写、空 ordinary storage cell | 复用 P30 shared complete-graph `QuickTransfer`；一个 P1/P19 whole-graph relocation、一次 exact record cleanup、一次 `SaveRecord`。 |

P45 的尸体 → 地面动作只处理完整图。它不处理 child 单独落地、parent-only move、partial graph、数量 N、`Split`、`Merge`、`Swap`、equipment replacement、temporary P6 staging、空间嵌套或多个物品合成一个地面 container。parent、child container、所有 child contents、`ItemId`、`ContainerId`、`SpatialChildGuid`、Definition、Level、Quality、RandomSeed、LegacyAffixDigest、P20/P21 provenance、capacity、placement order 与 one-layer/no-cycle/no-orphan/no-duplicate-child-owner 资格必须原样保持。

P45 成功创建的 record 是独立的 complete-graph world root，不是 P43 simple stack，也不是 P44 standard equipment。它只在 exact opened record、canonical P45 provenance 与 P17/P19 graph truth 全部有效时接入既有 P19 normal pickup 与 P30 `BaseQuick`-only Ctrl resolver；不得伪装为 P19 玩家来源、P43/P44 source 或任何普通 stack。不得因 P45 provenance 创建专用按钮、第二 QuickTransfer、自动目标、自动装备、快捷丢弃或尸体回存。

### 3. 严格范围与持续排除

P45 source 只接受同时满足以下全部条件的 complete graph：

1. 当前 Workspace 为既有 P12 production Host；exact `BasicCorpse BodyTarget` 已 `Open`、已 `Revealed`、identity-valid，且无 active search/action。`OwnerId`、`RunInstanceId`、`BodyTargetId`、`DeathReceiptId`、body record revision、target-open generation、route、focus、P6 composite revision 与 active session 都能在 Preview 和 durable Commit 前从 P1/P6/P11 truth 重验；
2. source 位于 exact P11 `BasicCorpse` 的 ordinary body-storage stable address；它不是 P21 `Body.Weapon`、`Body.ArmorRobe` 或 `Body.Accessory0`，不是 P9/P14/P31/P5/P6 player source、P17 child、Hotbar、另一 `BodyTarget` 或 UI-only projection；
3. Catalog、P1/P6/P11 snapshot、P20/P21 receipt/profile/history、P12 BodyTarget 与 source stable address 共同证明 root 只能是 canonical `WindTalisman` 或 `BackpackLevel1`，且 parent、唯一 `ChildContainerId`、stable `SpatialChildGuid`、formal child type/capacity、root reverse slot pointer、closure、child contents、one-layer restriction、无环、无 orphan、无 duplicate child owner 与 no parent-only graph move 全部成立；
4. normal Drag → `GroundDropZone` payload 只能临时冻结 exact Owner/Run、BodyTarget/death receipt、body/open generation、source container/slot/root/definition、parent-child closure identity、P20 profile/candidate/materialization digest、route/focus、P6 composite revision 与必要 active-session proof；不得含可写 quantity cache、child cache、预建 world identity、可变目标或 floor-owned item data；
5. `GroundDropZone` route/floor placement、active Owner/Run、Prepared/terminal gate、Host validity、P31 create proof 与 active session 也必须在 Preview 和 Commit 前持续成立。

下列对象或动作持续不属于 P45：

1. P12 ordinary simple stack、P21 fixed equipment、任何 P20/P19 child item、P17 child item、noncanonical complex parent、P9/P10 normal container、P5 warehouse、P6 player source、Hotbar、existing WorldDrop source、another BodyTarget、Hidden/Searching source、UI-only object 或 Code A item；
2. `Ctrl + 左键`作用于尸体 Cell、P41 corpse QuickTransfer、P42 direct equipment Drag、player-side quick-drop、Actor direct pickup、距离自动拾取、交互键领取、right-click Take、double-click、Take All、auto equip、auto bind、auto use、auto target、auto child open/switch、`Swap`、`Replacement`、`Sort`、`Compact`、`Merge`、`Split`、Quantity=N、`WorldPickupDraft`、`PlayerSplitDraft`、record-to-record transfer、cross-record merge、child drop 或 nested graph；
3. P12 normal corpse → player `Move/Merge/Swap`、P20 direct `BaseQuick` path、P41/P42、P19/P30 existing WorldDrop paths、P43/P44 ground drop、P26—P29 simple stack、P31 schema、P8 terminal classification、P13 binding、P15 use 与 Code A authority；
4. 实机运行、PIE、Standalone、真实鼠标键盘、截图、Smoke、Automation、回归、Cook、Package 或最终验收。

## 下半部分：授权执行内容

### 4. 单一授权目标

在不建立第二物品真值、不改变 P11/P12 corpse state machine、P20/P21 deterministic source/history、P17 graph、P19 normal world semantics、P30 Ctrl semantics、P31 Registry、P20/P41/P42 direct-player paths或 Code A authority，也不新增任何自动行为的前提下：

1. 只有已打开、已揭示、identity-valid 的 exact P20 complete graph 才可经 normal Drag 到 `GroundDropZone` 建立一次 P45 world-drop transient intent；
2. Preview 只建立一个 Owner/Run scoped candidate，冻结 exact P11 source closure 与 accepted floor placement；Commit 不得重扫 source、改投 P6、改投另一尸体、拆开 child、改投旧 WorldDrop 或 fallback 到其他 ground record；
3. accepted corpse → world path 只形成一个 P1/P19 whole-graph relocation、一个 P31 new-record insertion、一个 P11/P6 Owner durable replacement 和一次 `SaveRecord`；
4. 只有 accepted snapshot/replay proof 明确证明相同 parent、唯一 child container 与完整 closure 已离开 exact P11 source 并进入 exact new derived world container slot 0 后，才可刷新 BodyTarget source、record、Actor projection 与必要 workspace projection；
5. 新 record 只以 canonical P45 complete-graph world root 身份接入既有 P19 normal explicit pickup 与 P30 frozen-target `BaseQuick`-only QuickTransfer；普通拾回不自动猜目标，Ctrl 不进入任何 P17 child 或装备位；
6. 任一 source、graph、identity、lifecycle、placement、record、session、target、P1 或保存检查失败时，完整零写入并恢复 `BeforeSnapshot`。

### 5. 实现要求

#### 5.1 先完成活动调用链与资格审计

改动前必须审阅并在 Report 中列出：

1. P12 `BodyTarget` open/reveal lifecycle、ordinary spatial root Cell、normal `DragOperation`、`GroundDropZone` 可达性、`NativeOnDrop`、page/focus invalidation、P11/P6 callback 与 P20 source proof；说明 P45 如何复用既有 production Cell、Drag payload 与 `GroundDropZone`，而不是新增 Button、hotkey、pointer handler、Widget、Actor 或尸体写入口；
2. P20 canonical corpse spatial provenance、P21 r3 inherited group、future-only receipt/profile/candidate digest、materialization history、actual ordinary stable source address、`WindTalisman`／`BackpackLevel1` formal parent、child closure 与 original direct-player paths；实际值必须如实记录；
3. P14/P31 `GroundDrop` request、floor placement、new-record candidate、`WorldDropId`/`Ordinal`、derived world container、Actor diff、exact target open、normal pickup、P30 QuickTransfer、record cleanup、recovery 与 P8 player-only filtering 调用图；
4. P19 normal complete-graph WorldDrop pickup 与 P30 shared Ctrl pointer router、`BaseQuickOnly` frozen target、preview、commit、cancellation 和 stale lifecycle；说明 P45 如何加入同一 canonical complete-world-root source classification，而不是制造 P45 专用拾回入口或改写已有 provenance branch；
5. P1/P2/P3/P6/P11/P13 的 whole-graph Move、cross-graph single candidate、accepted snapshot/replay proof、`BeforeSnapshot` rollback、P11/P6 single Owner durable replacement、P13 reconcile 与 revision/session 调用图；
6. 所有 Widget direct Move/graph mutation、Actor direct pickup、尸体展示缓存写入、right-click Take、double-click、预建 `ItemId`/`ContainerId`/`WorldDropId`/`Ordinal`、按 first/last/current record 猜测 target、第二 Repository transaction、第二 save 或 Code A inventory writer；它们不得成为 P45 写入路径。

#### 5.2 共享 normal Drag、source gate 与瞬时 proof

1. P20 complete-graph root Cell 的既有 normal `DragOperation` 与 `GroundDropZone::NativeOnDrop` 或当前等价正式入口仍是 P45 corpse → world 的唯一输入与提交入口。不得为 P45 新建 Button、hotkey、Actor click、专用 Widget、第二 pointer handler、second resolver 或 UI direct write。
2. ordinary left click 继续只选择或沿 P12 既有规则开始 search；right-click 继续只读详情；尸体 Cell 上 Ctrl + 左键继续只走 P41 或其他已授权 QuickTransfer route，绝不建立 P45 drop intent；double-click、Tab、I、Esc、Close、Cancel、scroll、空白区、无 payload Drop、Shift + 1—9、player-side drag 与任何 Code A interaction 均不得形成 P45 位置写入。
3. source payload 只能由 current Open、Revealed、identity-valid exact P20 complete graph 建立。它必须冻结第 3 节所列 exact source/body/receipt/profile/candidate/graph/revision/session identity，不得以显示名、图标、Cell class、尸体展示顺序、Actor pointer、地图坐标、当前选择或 UI cache 判断资格。
4. Preview 与 durable Commit 前必须重新验证 source parent 仍在同一 exact P11 ordinary slot、P20 receipt/profile/digest 和 Catalog qualification 不变、full closure 与 reverse pointers 均成立、BodyTarget 仍 Open、visibility 仍为 Revealed，且 Owner/Run/session/Prepared/terminal/Host/route/focus/revision gate 全部成立。不得让 simple stack、P21 equipment、child、P9/P14/P31、player source、another BodyTarget 或 UI projection 建立 P45 intent。
5. BodyTarget close/reopen、focus loss、开始/取消 search、Actor EndPlay、map reload、recovery、terminal、Prepared、receipt/root/container/child mismatch、payload cancel、Host invalidation、source revision stale、floor placement stale、Registry collision 或 `SaveRecord` failure 必须立即使 intent/candidate 失效并零写入。不得改投 `BaseQuick`、formal equipment、P17 child、旧 WorldDrop、另一尸体或其他位置。
6. payload 不得包含可写数量、split amount、child snapshot 或预建 world identity。它只能代表一个 existing whole graph；不得调用 simple `Move` 绕过 closure、`Merge`、`Split`、`RequestedMergeQuantity`、`WorldPickupDraft`、`PlayerSplitDraft` 或任何 partial-transfer shortcut。

#### 5.3 exact P11 P20 complete graph → new P31 WorldDrop record

1. `GroundDropZone` 先只通过既有 Code A floor-placement adapter 解析合法、不可变的 route/floor placement；该 adapter 只能转发展示和生命周期边缘，不得创建 ItemId、ContainerId、WorldDropId、record、ordinal、数量、P11 graph 或 P6 graph。
2. Preview 成功后只建立一个 Owner/Run scoped candidate。该 candidate 必须以一个 P1/P19 whole-graph relocation 将同一个 parent、其唯一 child container 与完整 closure 从 exact P11 P20 source 直接移入新 record 的 derived world container slot 0；不得先落 P6 `BaseQuick`、先清 P11、先建立可见 Actor、先写临时 container，或执行 two-step relocation。
3. accepted graph 的 parent/child `ItemId`、`ContainerId`、DefinitionId、Quantity、Level、Quality、RandomSeed、LegacyAffixDigest、P20/P21 receipt/profile/candidate/materialization provenance、capacity、placement order、stable `SpatialChildGuid`、one-layer/no-cycle/no-orphan/no-duplicate-child-owner 资格必须原样保持。不得 Merge、Split、Swap、clone、new ItemId、new ContainerId、new child、flatten、parent-only move、temporary player item 或第二物品真值。
4. `NextWorldDropOrdinal`、derived world container、`WorldDropId`、record insertion、P11 source removal、P6/P14 record update、P13 necessary reconcile 和 Actor projection 只能在同一个 accepted Owner durable replacement 中出现，并且函数内只有一次 `SaveRecord`。Actor 仅在 accepted durable projection 后创建或刷新。
5. 新 record 必须保留 P31 exact identity：OwnerId、RunInstanceId、WorldDropId、Ordinal、map route/floor placement、Available state、derived ContainerId、root parent ItemId、record revision 与 P45 spatial-corpse provenance。实际 provenance literal 可沿活动 schema 命名，但必须独立、可审计、不可与 P19 player world source、P20 corpse direct pickup、P43/P44 或 simple/equipment provenance 伪装或混用。
6. Store 必须以 accepted command 的相同 TransactionId、source、target、closure、revision 与 record proof 重放一次 P1/P19 whole-graph relocation，并要求 replay snapshot 与 accepted composite 完全相等；只有 replay 证明整个 graph 已离开 exact P11 source 并到达 exact new world container 后，才刷新 BodyTarget source 与 P31 projection。
7. candidate、source/graph/placement/Registry validation、P1/P19 relocation、P13 reconcile、record/Actor projection 前检查或 `SaveRecord` 任一失败时，完整恢复 `BeforeSnapshot`：P11 source parent/child closure、P6/P14 Registry、NextWorldDropOrdinal、existing records、new record、derived container、Actor、BodyTarget visibility/open state、selection 与无关 projection 均保持旧值；不得遗留 phantom empty、phantom world root、ordinal gap、orphan container、orphan child、duplicate child owner 或 transient item copy。

#### 5.4 P45 record 与既有 complete-graph 拾回链的受限衔接

1. P45 成功创建的 record 只能以 current opened、identity-valid、exact P31 record 的 canonical P19 complete-graph root 身份参与既有拾回。所有 admission 必须验证 OwnerId、RunInstanceId、WorldDropId、Ordinal、record revision、derived container、parent root ItemId、child closure、route、focus、target-open generation、P6 revision、P45 provenance、record availability 与 active session；不得由 Actor、list order、selection 或其他 record 推断。
2. normal Drag pickup 只可复用既有 P19 complete-graph 的明确目的地 policy：
   - 用户明确 Drop 的空 `BaseQuick` ordinary storage cell；
   - `WindTalisman` 的用户明确 Drop 的空、正式、Definition-compatible `SpatialRing`；
   - `BackpackLevel1` 的用户明确 Drop 的空、正式、Definition-compatible `Backpack`。

   parent 及其 child 不得进入任何 P17 child。目标失效、占用、不兼容、child topology stale、focus loss/open-generation mismatch、record close/stale 或保存失败时一律零写入；不得自动选第一个装备位、自动打开/切换 child、自动回退、Swap、Replacement、flatten、partial pickup 或尸体回存。
3. Ctrl + 左键只复用 P30 已有的 shared pointer router、complete-graph `QuickTransfer` transient intent 与 `BaseQuickOnly` frozen mode。P45 root 在输入时只能冻结并移向当前 P6 `BaseQuick` 中按真实 stable `SlotIndex` 升序的第一个合法空 ordinary storage slot；它不得进入当前 P17 child、任何装备位、Hotbar、P5/P9/P11/P14/P31、另一 WorldDrop 或任何 fallback。target 随后失效、已满或 record stale 时必须拒绝，绝不重扫或静默切换。
4. P45 不得新建 P45-special pointer router、second QuickTransfer resolver、target scan、hotkey 或 UI handler。实现只能在已有 P19/P30 canonical complete-world-root source classification 中加入 exact P45 record proof，并保持 P19/P30 现有 source discrimination、normal Drag 和 Ctrl 行为不变。
5. 每个 accepted P45 pickup 只形成一个 P1/P19 whole-graph relocation、一个 exact record cleanup、一个 Owner durable replacement 与一次 `SaveRecord`。P31 record/container/Actor 仅在 accepted snapshot/replay 明确证明整个 graph 已离开 exact derived world container后清理；partial/failed/stale pickup 一律保留相同 record、root、closure、WorldDropId、Ordinal、Actor 与 other records。

#### 5.5 单一事务、终局、回滚与非回归

1. P45 corpse → world Drop 必须沿既有 P12/P3 → P2 → P1 → P11/P14/P31/P6 durable callback；每次 input 最多一个 candidate、一次 Owner record replacement 与一次 `SaveRecord`。不得先写 P11 后写 P6，也不得先写 record/Actor 后写 source。
2. P13 只沿既有 accepted commit reconcile；不得自动 Bind、Use、Equip、打开 child 或复制 binding。P15 保持现有 RestoreHealth-only scope。
3. P8 `BuildP14PlayerOnlySession` 或等价 finalization 必须把 P45 record 的 root、child closure、derived world container 及所有 other Registry records 从 player-only final graph 排除。P45 不修改 terminal classification、P5 receipt、recovery、run replacement 或 Code A terminal authority；仍留在 P11 的尸体残余也继续只按 P11 residual discard 处理。
4. repeated Actor refresh、map reload、active P6 recovery、Actor EndPlay、close、lost focus 和 duplicate terminal observer 只能处理 matching transient projection，不能改变 durable item graph、删除 record、回填尸体或把 Actor 作为真值。
5. 成功后只刷新 exact BodyTarget source、new/exact WorldDropTarget、matching Actor 与必要 P6/P7/P17/P19 projection；不得 Sort、Compact、重排无关 SlotIndex、重建无关空间区域、清空无关选择、改变无关 scroll offset，或改变 P20/P21/P41/P42/P43/P44/P19/P30/P31 的既有产品语义。

### 6. 允许的改动范围

仅允许在当前 Code B 中最小修改：

- P12/P20 已有 ordinary spatial source proof、shared Drag payload、normal Drop preview/commit、P11/P6 durable transaction 与 projection，仅用于 P45 exact corpse complete-graph source；
- P14/P19/P30/P31 所在的 GroundDrop candidate、canonical complete-graph eligibility、record insertion、accepted pickup proof、Actor projection、P13 reconcile、P8 player-only filtering 与 rollback，仅用于表达 P45 accepted single-root record；
- P1/P2/P3/P6/P11/P13/P17/P19 的必要声明、candidate proof、replay、rollback 或调用签名兼容，前提是不改变已有产品语义；
- 必要的 production `GroundDropZone` / floor-placement / Actor lifecycle forwarding，只能是 Code B gate 的只读转发；Code A 不得取得库存、Loot、数量、WorldDrop、graph 或 durable authority；
- `PROJECT.md`、`PROJECT_INFO_CARD.md`、本任务 Prompt 归档与本任务 Report。

禁止新建项目、版本线、Fix、第二 P11/P6、Widget inventory、Code A mirror、fixture、假 ItemId、clone、双写、存档重置或历史数据改写。禁止修改 Code A 功能逻辑、P12 search state machine、P20/P21 deterministic source/history、P17 graph construction、P31 schema/registry、P8 receipt/terminal product logic、P5/P6 bridge 或任何测试文件。

### 7. 明确不在本任务内

- 不把 P12 ordinary simple stack、P21 equipment、P20/P19 child item、P17 child item、P9/P10 container、P5 warehouse、P6 player item、任何 other BodyTarget 或 existing WorldDrop root 接入 P45 corpse-drop source。
- 不实现 corpse partial graph drop、child drop、P11 `Split/Merge/Swap` 扩展、player → corpse quick return、快捷丢弃、Actor direct pickup、自动拾取、Take All、right-click Take、double-click、auto target、auto equipment、auto child open/switch、space parent/child 地面操作、world multi-item container、record-to-record transfer、cross-record merge、Swap、Replacement、Sort、Compact、Bind、Use、装备数值、战斗效果、HUD 接管、网络或多人。
- 不改变 P12 normal corpse → player `Move/Merge/Swap`、P20 direct BaseQuick normal Drag、P41/P42、P19/P30 normal/Ctrl world path、P43/P44 ground-drop、P26/P27/P28 quantity contract、P29 QuickTransfer priority、P31 Registry identity、P13/P15、P17/P19、P5/P6/P8、搜索、敌人、地图、战斗、生命、死亡、撤离、商店、经济或制作。
- 不启动产品、PIE、Standalone、真实鼠标键盘输入、截图、Smoke、Automation、回归、试玩、Cook、Package 或最终验收。
- 不在回传 Report 前自动开始 P46、任意 Fix 或 F。

### 8. P 阶段静态审查与编译

完成后只执行以下检查：

1. 审查 P45 唯一合法 source 为 current opened/revealed/exact P12 ordinary P20 `WindTalisman`／`BackpackLevel1` complete graph；P12 simple stack、P21 equipment、child、Hidden/Searching、P9/P14/P31、player source、another BodyTarget 与 unknown family 均不能进入 P45。
2. 审查 normal Drag 至既有 `GroundDropZone` 是 corpse → world 的唯一输入；确认无新 pointer、Widget、Actor、quantity/child draft、QuickTransfer、right-click、double-click 或 Code A 写入旁路。
3. 审查 accepted drop path 只包含一个 P1/P19 whole-graph relocation、一个 P31 new-record candidate、一个 P11/P6 Owner durable replacement、一次 `SaveRecord` 与 accepted-only Actor projection；确认没有 P6 staging、simple Move、Merge、Split、new ItemId/ContainerId/child、second truth、actor-first write 或 ordinal gap。
4. 审查 exact P20 source proof、complete closure、BodyTarget/death receipt/profile/candidate identity、Reveal/Open lifecycle、Owner/Run/revision/session、floor placement、P31 WorldDropId/Ordinal/container/root identity、P13 reconcile、`BeforeSnapshot` rollback 与 zero-write stale policy；任一失败不得改变 P11、Registry、Actor 或 other record。
5. 审查 P45 record 的 normal pickup 仅允许明确空 `BaseQuick`、明确空 compatible `SpatialRing`／`Backpack`；Ctrl 仅通过现有 P30 shared frozen `BaseQuickOnly` router 到一个 ordinary slot。确认没有 child entry、auto equip、target guess、mode fallback、new resolver 或尸体回存。
6. 审查 accepted P45 pickup 只清理 exact record，且 P19/P30、P20/P41/P42、P43/P44、P26—P29、P31 other-record isolation均保持；P45 record 不以 parent/child topology破坏 single-root WorldDrop 语义。
7. 审查 P20/P21 r1/r2/r3、already materialized records、deterministic identity、optional spatial catalog、candidate/weight/digest、`SpatialChildGuid`、P17 one-layer restriction、Hidden/Searching/Reveal 与 P6 → corpse rejection均未被改写；P8 terminal/recovery、P13/P15、P5/P6 bridge与 Code A authority保持。
8. 运行 `git diff --check`，并在 Report 中列出实际修改文件、权威写入调用图与所有未修改的权威边界。
9. 编译 Editor：

       "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_mapEditor Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload

10. 编译 Game：

       "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_map Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload

若编译失败，只修复本任务引入的 P20 spatial source proof、GroundDrop/P31 transaction、P19/P30 complete-graph pickup admission、projection、rollback、include 或签名问题；若必须扩大到本任务以外，停止受影响部分并如实报告。

### 9. Report 与完成信号

生成 `Dev.D.UE.0.0.9B.P45.0.r0_report.md`，保存至：

    C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Docs\Report

Report 必须简洁、可审计地列出：

1. 本轮新增、修改、未修改的每个文件及职责；
2. P20 spatial root Cell 到 `GroundDropZone`、P45 source proof、P1/P19/P11/P31 durable transaction 和 accepted-only Actor projection 的完整调用图，以及没有新增 pointer、Widget、Actor、second resolver 或 Code A 写入口的证据；
3. P20/P21 actual stable ordinary source provenance、parent/child closure、BodyTarget/death receipt/profile/candidate identity，以及为什么 materialization/history/determinism 未被改写；
4. exact source gate、Reveal/Open lifecycle、Owner/Run/revision/session/floor proof、P45 record identity与 zero-write cancellation/stale policy；
5. one P1/P19 whole-graph relocation、single P31 new-record insertion、single P11/P6 durable replacement、single `SaveRecord`、replay proof、parent/child/closure/provenance保持、P13/P8、rollback、selection/scroll/stable SlotIndex 与无 P6 staging/second truth 的证据；
6. P45 exact record 如何仅接入既有 P19 normal pickup和 P30 `BaseQuickOnly` Ctrl router；明确 `BaseQuick`／`SpatialRing`／`Backpack` target验证、无 child entry/auto equip/no fallback/no P45-special input 的证据；
7. P20/P41/P42、P19/P30、P43/P44、P26—P29、P31、P5/P6/P8/P13/P15/P17与 Code A authority的非回归结论；
8. `git diff --check` 结果、实际改动范围及所有明确未实现范围；
9. 两个编译命令、目标、原生 exit code 与关键结果；
10. 所有未执行的 F 阶段真实验证，至少包括：future `BasicCorpse` materialize/open/reveal；canonical `WindTalisman`／`BackpackLevel1` complete graph 各自 normal Drag 至 `GroundDropZone`；multiple existing WorldDrop isolation；new Actor open/close；P45 record normal Drag 至 `BaseQuick`／compatible `SpatialRing`／`Backpack`；P45 record Ctrl 至 `BaseQuickOnly`；empty/full/wrong `BaseQuick`／slot、child closure/topology stale；wrong/simple/equipment/child/Hidden/Searching/player/another-body source；BodyTarget/death receipt/profile/candidate/Owner/Run/P6 revision stale；floor placement/record/ordinal/container/root/child mismatch；close/reopen、search/action、Prepared/terminal/Host invalid/SaveRecord failure；P19/P20/P30/P41/P42、P43/P44、P26—P31、P8 terminal/recovery、真实鼠标键盘、PIE、Standalone、截图、Smoke、Automation、回归、Cook 与 Package。

仅当 P45 corpse P20 complete-graph → independent WorldDrop 静态闭合、existing P19 normal pickup/P30 QuickTransfer chain、P31 identity、P11/P12/P20/P21 与 P8 边界保持，且 Editor 与 Game 均以 native exit code `0` 完成时，使用：

    READY_FOR_P46_PLANNING

若当前范围内仍有可修复问题，使用：

    NEEDS_P45_REWORK

若现有 P12/P11/P6/P14/P31 transaction 或 P19/P30 complete-graph resolver 无法在不创建第二输入路径、第二保存、第二物品真值、拆分 child graph、自动装备、改写 P20/P21 deterministic history 或扩展 Code A authority 的前提下支持本项，使用：

    NEEDS_PLANNER_DECISION
    BLOCKED

完成后不得自动开始 P46、Fix 或 F。向策划 Chat 回传并附带且只附带本次同名 Report；正文首行使用：

    [CSEMI:REPORT_SENT] {"task_id":"Dev.D.UE.0.0.9B.P45.0.r0","file":"Dev.D.UE.0.0.9B.P45.0.r0_report.md"}
