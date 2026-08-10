# Dev.D.UE.0.0.9B.P44.0.r0

## 任务身份

- 项目：Dev.D.UE.0.0.9B；继续使用同一活动工程，不新建项目。
- 阶段：主线 P44——已揭示 P21 尸体标准装备的显式地面落地，以及与既有标准装备 WorldDrop 拾回链的受限衔接。
- 任务编号：Dev.D.UE.0.0.9B.P44.0.r0。
- 前置：已接受 0.0.9B.P1—P43 与 0.0.9BFix.P1—P4。Fix 只用于已确认、已验收功能的缺陷修复；P44 是新增主线功能，不是 Fix。
- 执行文件：Dev.D.UE.0.0.9B.P44.0.r0_prompt.md。
- 报告文件：Dev.D.UE.0.0.9B.P44.0.r0_report.md。
- 活动工程根：C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B。
- 活动工程：C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject。
- Report 必须生成到：C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Docs\Report\Dev.D.UE.0.0.9B.P44.0.r0_report.md，并仅携带该同名 Report 回传策划 Chat。
- 任务性质：P 阶段只做实现、静态审查和代码编译。不得启动产品、PIE、Standalone、真实鼠标键盘验证、截图、Smoke、Automation、回归、Cook、Package 或 F 阶段测试；不得自动开始 P45、任意 Fix 或 F。

## 上半部分：只读项目裁决、现状与边界

### 1. 当前唯一有效基线

唯一有效依据是当前 0.0.9B、活动工程和已接受任务链。所有 0.2、V2、V3、I、IPF、历史页面壳、旧 CTA、旧库存和旧物品规则均已过时；不得读取、采用、恢复或以其决定实现、验收或范围。

Code B P1 Repository 与既有 durable Store 是唯一可变物品真值。每件物品始终只有一个真实 ItemId、一个真实父位置和一条权威事务链。Widget、Cell、Presenter、DragOperation、Workspace Context、BodyTarget、WorldDropTarget、WorldDrop Actor、地图放置适配层和 Code A 都只能持有只读投影、选择或瞬时意图；不得持有第二库存、可写数量副本、预建 ItemId、预建 WorldDropId、预建 Ordinal、平行尸体背包、Actor-first 写入或 A/B 双写。

P11/P12 已建立唯一 BasicCorpse 的死亡回执、首次 materialize、Hidden → Searching → Revealed、已打开的 BodyTarget，以及 P11/P6 的唯一 durable transfer。P21 在 future BasicCorpse.r3 中建立 Body.Weapon、Body.ArmorRobe、Body.Accessory0 三个固定尸体装备位；它们的实际 stable container/slot semantic、receipt/profile/candidate digest 与 root identity 必须从活动真值读取。P21 root 是正式 non-spatial、无 ChildContainer、Quantity=1、MaxStack=1 的 standard equipment root；P21 的 deterministic source/history、Reveal 生命周期和 P6 → corpse 回存拒绝均已接受，P44 不得改写。

P31 已将地面状态收敛为 Owner/Run scoped 多 record Registry：每条 record 永远只包含一个 root、一个 derived WorldDrop container、一个 WorldDropId、一个 Ordinal 与一个 Actor projection。record 的 create、open、pickup、cleanup、recovery、Actor diff 与 P8 terminal exclusion 都只按 exact record identity 处理，绝不以 first、last、current selection、显示次序或 Actor pointer 猜测。

P32/P33/P35 已建立 standard non-spatial equipment 的 normal GroundDrop 与明确拾回边界；P34/P36/P37 已建立同一类 opened standard WorldDrop root 的冻结目标 Ctrl + 左键拾回。P38/P39 已让 P21 root 直接经明确 Drag 或 Ctrl + 左键进入玩家侧，但未让它落地。P43 已为 P12 ordinary simple stack 建立尸体 → 新 P31 simple WorldDrop 的窄例外，并明确排除了 P21 equipment。P44 只补齐这个被保留的 P21 standard-equipment ground-drop 缺口；它不重新实现 P38/P39/P43，也不创建第二套 world interaction。

### 2. P44 产品裁决

当玩家已经通过 P12 主动打开 identity-valid 的唯一 BasicCorpse，且一个 exact P21 Body.Weapon、Body.ArmorRobe 或 Body.Accessory0 root 已 Revealed、仍属于该 BodyTarget，玩家可以从该 root Cell 发起既有 normal DragOperation，并把它 Drop 到既有 GroundDropZone。成功时，同一个 root 以一次 P1 whole-root Move 从 exact P11 corpse-equipment source 直接移入一个新建、独立的 P31 WorldDrop record 的 derived WorldDrop container slot 0。

| 明确动作 | 唯一允许来源 | 唯一允许目标 | 权威结果 |
| --- | --- | --- | --- |
| 尸体标准装备主动落地 | 当前已打开、已揭示、identity-valid P21 Body.Weapon / Body.ArmorRobe / Body.Accessory0 中的 canonical standard non-spatial root | 既有 GroundDropZone | 一个 P1 whole-root Move；同一个 Owner durable replacement 同时更新 P11 source 与 P31/P6 新 record，并只保存一次。 |
| 地面普通明确拾回 | 当前已打开、identity-valid 的 exact P44 single-root standard-equipment WorldDrop | 用户明确 Drop 的空 BaseQuick 普通格、当前有效 P17 child 的空普通格，或空且 Definition-compatible 的正式装备位 | 复用既有 standard-equipment WorldDrop normal Drag 的一个 P1 whole-root Move 或 formal Equip/root relocation；只清理该 exact record。 |
| 地面 Ctrl + 左键快捷拾回 | 当前已打开、identity-valid 的 exact P44 single-root standard-equipment WorldDrop | 输入瞬间已有 valid current P17 child 时，只进入该 child 的第一个合法空普通格；输入瞬间无 child 时，只进入 BaseQuick 的第一个合法空普通格 | 复用既有 P36/P37 同一冻结目标 QuickTransfer；一个 P1 whole-root Move、一次 exact record cleanup、一次 SaveRecord。 |

P44 的尸体 → 地面动作只处理整个 root。它不处理数量 N、Split、Merge、Swap、equipment replacement、partial drop 或从尸体直接落到玩家 temporary staging。P21 root 的 Quantity=1、MaxStack=1、ItemId、DefinitionId、Level、Quality、RandomSeed、LegacyAffixDigest、P21 receipt/profile/candidate provenance 与 no-child/non-spatial qualification 必须原样保持。

P44 成功创建的 record 是独立的 standard-equipment world root，不是 P43 simple stack，也不是 P19/P20 complete graph。它只在 exact opened record、canonical P44 provenance 和 standard-equipment truth 全部有效时接入既有 normal/QuickTransfer resolver；不得伪装为 P32/P33/P35 来源、不得让现有来源互相混淆，也不得因 P44 provenance 建立专用按钮、第二 QuickTransfer、自动目标、自动装备、快捷丢弃或尸体回存。

### 3. 严格范围与持续排除

P44 source 只接受同时满足以下全部条件的 root：

1. 当前 Workspace 为既有 P12 production Host；exact BasicCorpse BodyTarget 已 Open、已 Revealed、identity-valid，且无 active search/action。OwnerId、RunInstanceId、BodyTargetId、DeathReceiptId、body record revision、target-open generation、route、focus、P6 composite revision 与 active session 都能在 Preview 和 durable Commit 前从 P1/P6/P11 truth 重验；
2. source 位于 exact P11 BasicCorpse 的 P21 Body.Weapon、Body.ArmorRobe 或 Body.Accessory0 正式 equipment slot。实际 ContainerId、SlotIndex、slot semantic、profile/version、candidate digest 和 receipt provenance 必须以活动真值为准；不得假定固定字符串、固定容器或展示次序；
3. Catalog、P1 snapshot、P11 receipt/profile、P21 materialization result 与 P12 BodyTarget projection 共同证明该 root 是 P21 deterministic candidate-set 内的 standard non-spatial equipment：Quantity=1、MaxStack=1、不可堆叠、无 ChildContainerId、无 spatial semantic、无 complete-graph closure、不是 Hotbar reference、不是 draft；
4. normal Drag → GroundDropZone 的 payload 只能临时冻结 exact OwnerId、RunInstanceId、BodyTargetId、DeathReceiptId、body/open generation、source ContainerId、SlotIndex、slot semantic、root ItemId、DefinitionId、Quantity、P21 profile/candidate digest、route/focus、P6 composite revision 与必要 active-session proof；不得含可写 quantity cache、预建 world identity 或可变目标；
5. GroundDropZone route/floor placement、active Owner/Run、Prepared/terminal gate、Host validity、P31 create proof 与 active session 也必须在 Preview 和 Commit 前持续成立。

下列对象或动作持续不属于 P44：

1. P12 ordinary simple stack、P20/P41/P42 spatial parent/complete graph、任何 ChildContainer、P17 child item、P19 graph、space parent/child、复杂 parent、nested closure、P9/P10 normal container、P5 warehouse、P6 player root、Hotbar、WorldDrop source、another BodyTarget、P21 empty slot、Hidden/Searching source、UI-only object 或 Code A item；
2. Ctrl + 左键作用于尸体 Cell、P39 corpse QuickTransfer、player-side quick-drop、Actor direct pickup、距离自动拾取、交互键领取、right-click Take、double-click、Take All、auto equip、auto bind、auto use、auto target、auto child open/switch、Swap、Replacement、Sort、Compact、Merge、Split、Quantity=N、WorldPickupDraft、PlayerSplitDraft、record-to-record transfer 或 corpse partial drop；
3. P12 ordinary corpse → P6 normal Move/Merge/Swap、P38 normal direct pickup、P39 corpse Ctrl pickup、P43 simple-stack drop、P26—P29 simple-stack source/target order、P30/P41 complete-graph pickup、P31 schema、P8 terminal classification、P13 binding、P15 use 和 Code A authority；
4. 实机运行、PIE、Standalone、真实鼠标键盘、截图、Smoke、Automation、回归、Cook、Package 或最终验收。

## 下半部分：授权执行内容

### 4. 单一授权目标

在不建立第二物品真值、不改变 P11/P12 corpse state machine、P21 r3 deterministic source/history、P17 graph、P31 multi-record Registry、P32—P37 既有产品语义或 Code A authority，也不新增任何自动行为的前提下：

1. 只有已打开、已揭示、identity-valid 的 exact P21 standard-equipment root 才可经 normal Drag 到 GroundDropZone 建立一次 P44 world-drop transient intent；
2. Preview 只建立一个 Owner/Run scoped candidate，冻结 exact P11 source 与 accepted floor placement；Commit 不得重扫 source、改投 P6、改投另一尸体、改投旧 WorldDrop 或 fallback 到其他 ground record；
3. accepted corpse → world path 只形成一个 P1 whole-root Move、一个 P31 new-record insertion、一个 P11/P6 Owner durable replacement 和一次 SaveRecord；
4. 只有 accepted snapshot/replay proof 明确证明同一 root 已离开 exact P11 P21 source 并进入 exact new derived world container slot 0 后，才可刷新 BodyTarget source、record、Actor projection 与必要 workspace projection；
5. 新 record 只以 canonical P44 standard-equipment world root 身份接入既有 normal explicit pickup 与 shared frozen-target QuickTransfer。普通拾回不自动猜目标；Ctrl 只按输入时 frozen child/BaseQuick mode 行事；
6. 任一 source、identity、lifecycle、placement、record、session、target、P1 或保存检查失败时，完整零写入并恢复 BeforeSnapshot。

### 5. 实现要求

#### 5.1 先完成活动调用链与资格审计

改动前必须审阅并在 Report 中列出：

1. P12 BodyTarget open/reveal lifecycle、P21 equipment root Cell、normal DragOperation、GroundDropZone 可达性、NativeOnDrop、page/focus invalidation、P11/P6 callback 与 P21 source proof；说明 P44 如何复用既有生产 Cell、Drag payload 和 GroundDropZone，而不是新增 Button、hotkey、pointer handler、Widget、Actor 或尸体写入口；
2. P21 r3 的 Body.Weapon、Body.ArmorRobe、Body.Accessory0 actual stable semantic/container/slot、future-only receipt/profile/candidate digest、materialization history 与 canonical source qualification；实际值必须如实记录；
3. P14/P31 的 GroundDrop request、floor placement、new-record candidate、WorldDropId/Ordinal、derived world container、Actor diff、exact target open、normal pickup、QuickTransfer、record cleanup、recovery 与 P8 player-only filtering 调用图；
4. P32/P33/P35 的 standard-equipment WorldDrop normal explicit pickup，以及 P34/P36/P37 shared Ctrl + 左键 pointer router、input-time frozen mode、preview、commit、cancellation 和 stale lifecycle；说明 P44 如何加入同一 canonical standard-world-root source classification，而不是制造 P44 专用拾回入口或改写已有 provenance branch；
5. P1/P2/P3/P6/P11 的 whole-root Move、cross-graph single candidate、accepted snapshot/replay proof、BeforeSnapshot rollback、P11/P6 single Owner durable replacement、P13 reconcile 与 revision/session 调用图；
6. 所有 Widget direct Move/quantity mutation、Actor direct pickup、尸体展示缓存写入、right-click Take、double-click、预建 ItemId/WorldDropId/Ordinal、按 first/last/current record 猜测 target、第二 Repository transaction、第二 save 或 Code A inventory writer；它们不得成为 P44 写入路径。

#### 5.2 共享 normal Drag、source gate 与瞬时 proof

1. P21 equipment root Cell 的既有 normal DragOperation 与 GroundDropZone::NativeOnDrop 或当前等价正式入口仍是 P44 corpse → world 的唯一输入与提交入口。不得为 P44 新建 Button、hotkey、Actor click、专用 Widget、第二 pointer handler、second resolver 或 UI direct write。
2. ordinary left click 继续只选择；right-click 继续只读详情；尸体 Cell 上 Ctrl + 左键继续只走既有 P39 或其他已授权 QuickTransfer route，绝不建立 P44 drop intent；double-click、Tab、I、Esc、Close、Cancel、scroll、空白区、无 payload Drop、Shift + 1—9、player-side drag 与任何 Code A interaction 均不得形成 P44 位置写入。
3. source payload 只能由 current Open、Revealed、identity-valid exact P21 standard-equipment root 建立。它必须冻结第 3 节所列的 exact source/body/receipt/profile/candidate/revision/session identity，不得以显示名、图标、Cell class、尸体展示顺序、Actor pointer、地图坐标、当前选择或 UI cache 判断资格。
4. Preview 与 durable Commit 前必须重新验证 source root 仍在同一 exact P11 P21 equipment slot、P21 receipt/profile/digest 和 Catalog qualification 不变、BodyTarget 仍 Open、visibility 仍为 Revealed，且 Owner/Run/session/Prepared/terminal/Host/route/focus/revision gate 全部成立。不得让 ordinary stack、spatial parent/child、P9/P14/P31、player source、another BodyTarget 或 UI projection 建立 P44 intent。
5. BodyTarget close/reopen、focus loss、开始/取消 search、Actor EndPlay、map reload、recovery、terminal、Prepared、receipt/root/container mismatch、payload cancel、Host invalidation、source revision stale、floor placement stale、Registry collision 或 SaveRecord failure 必须立即使 intent/candidate 失效并零写入。不得改投 BaseQuick、P17 child、equipment slot、旧 WorldDrop、另一尸体或其他位置。
6. payload 不得包含可写 quantity cache、split amount 或预建 world identity。它只能代表一个 existing whole root；不得调用 Merge、Split、RequestedMergeQuantity、WorldPickupDraft、PlayerSplitDraft、graph Move 或任何 partial transfer shortcut。

#### 5.3 exact P11 P21 source → new P31 WorldDrop record

1. GroundDropZone 先只通过既有 Code A floor-placement adapter 解析合法、不可变的 route/floor placement；该 adapter 只能转发展示和生命周期边缘，不得创建 ItemId、WorldDropId、record、ordinal、数量、P11 graph 或 P6 graph。
2. Preview 成功后只建立一个 Owner/Run scoped candidate。该 candidate 必须以一个 P1 whole-root Move 将同一个 source ItemId 从 exact P11 P21 equipment source 直接移入新 record 的 derived world container slot 0；不得先落 P6 BaseQuick、先清 P11、先建立可见 Actor、先写临时 container，或执行 two-step relocation。
3. accepted root 的 ItemId、DefinitionId、Quantity=1、Level、Quality、RandomSeed、LegacyAffixDigest、P21 receipt/profile/candidate provenance 与无 child/non-spatial qualification 必须原样保持。不得 Merge、Split、Swap、clone、new ItemId、new ChildContainer、parent-only graph move、temporary player item 或第二物品真值。
4. NextWorldDropOrdinal、derived world container、WorldDropId、record insertion、P11 source removal、P6/P14 record update、P13 necessary reconcile 和 Actor projection 只能在同一个 accepted Owner durable replacement 中出现，并且函数内只有一次 SaveRecord。Actor 仅在 accepted durable projection 后创建或刷新。
5. 新 record 必须保留 P31 exact identity：OwnerId、RunInstanceId、WorldDropId、Ordinal、map route/floor placement、Available state、derived ContainerId、root ItemId、record revision 与 P44 equipment-corpse provenance。实际 provenance literal 可沿活动 schema 命名，但必须独立、可审计、不可与 P32/P33/P35/P43/P19/P20 伪装或混用。
6. Store 必须以 accepted command 的相同 TransactionId、source、target、revision 与 record proof 重放一次 P1 Move，并要求 replay snapshot 与 accepted composite 完全相等；只有 replay 证明 root 已离开 exact P11 P21 source 并到达 exact new world container 后，才刷新 BodyTarget source 与 P31 projection。
7. candidate、source/placement/Registry validation、P1 Move、P13 reconcile、record/Actor projection 前检查或 SaveRecord 任一失败时，完整恢复 BeforeSnapshot：P11 source 仍在原 stable equipment slot，P6/P14 Registry、NextWorldDropOrdinal、existing records、new record、derived container、Actor、BodyTarget visibility/open state、selection 与无关 projection 均保持旧值；不得遗留 phantom empty、phantom world root、ordinal gap、orphan container 或 transient item copy。

#### 5.4 P44 record 与既有标准装备拾回链的受限衔接

1. P44 成功创建的 record 只能以 current opened、identity-valid、exact P31 record 的一个 canonical standard non-spatial root 身份参与既有拾回。所有 admission 必须验证 OwnerId、RunInstanceId、WorldDropId、Ordinal、record revision、derived container、root ItemId、route、focus、target-open generation、P6 revision、P44 provenance、record availability 与 active session；不得由 Actor、list order、selection 或其他 record 推断。
2. normal Drag pickup 只可复用既有 standard-equipment WorldDrop 的明确目的地 policy：
   - 用户明确 Drop 的空 BaseQuick ordinary storage cell；
   - 用户明确 Drop 的 current identity-valid P17 child 内空 ordinary storage cell；
   - 用户明确 Drop 的空、正式、Definition-compatible Weapon、Armor/ArmorRobe 或普通 Accessory equipment slot。

   目标失效、占用、不兼容、child close/switch/focus loss/open-generation mismatch、record close/stale 或保存失败时一律零写入；不得自动选第一个装备位、自动打开/切换 child、自动回退、Swap、Replacement 或尸体回存。
3. Ctrl + 左键只复用 P36/P37 已有的共享 pointer router、QuickTransfer transient intent 与 frozen mode。P44 root 在输入时若已有 identity-valid current P17 child，只能冻结并移向该 exact child 中按真实 stable SlotIndex 升序的第一个合法空 ordinary storage slot；输入时没有 current valid child，才只能冻结并移向 BaseQuick 中按真实 stable SlotIndex 升序的第一个合法空 ordinary storage slot。child 后续失效、满位或 record stale 时必须拒绝，绝不静默切换 BaseQuick、另一 child、装备位或其他 record。
4. P44 不得新建 P44-special pointer router、second QuickTransfer resolver、target scan、hotkey 或 UI handler。实现只能在已有 standard-equipment WorldDrop canonical source classification 中加入 exact P44 record proof，并保持 P32/P33/P35/P34/P36/P37 的 existing source discrimination、target priority、normal Drag 和 Ctrl 行为不变。
5. 每个 accepted P44 pickup 只形成一个 P1 whole-root Move 或 formal Equip/root relocation、一个 P31/P6 Owner durable replacement 和一次 SaveRecord。只有 accepted snapshot/replay proof 明确证明 root 已离开这个 exact derived world container 后，才删除 matching record、空 world container 与 matching Actor projection；不得预删 Actor/record，也不得删除任何 other Registry record。
6. P44 root 绝不进入 P26—P29 simple-stack branch、P30/P41 complete-graph branch、P43 simple root branch、P11 corpse target、another BodyTarget、Hotbar、P5/P9、record-to-record target、space parent/child graph 或跨 record merge。P44 direct ground pickup 不重写 P38/P39 尸体→玩家路径。

#### 5.5 单一事务、终局、回滚与非回归

1. P44 corpse → world 与 P44 world → player 各自必须沿既有 P12/P3 → P2 → P1 → P11/P14/P31/P6 durable callback；每次 input 最多一个 candidate、一次 Owner record replacement 与一次 SaveRecord。不得先写 P11 后写 P6，也不得先写 record/Actor 后写 source。
2. P13 只沿既有 accepted commit reconcile；不得自动 Bind、Use、Equip、复制 binding 或在 corpse root 上创建 binding。P15 保持其现有 RestoreHealth-only scope。
3. P8 BuildP14PlayerOnlySession 或等价 finalization 必须把 P44 record 的 root、derived world container 及所有 other Registry records 从 player-only final graph 排除。P44 不修改 terminal classification、P5 receipt、recovery、run replacement 或 Code A terminal authority；仍留在 P11 的尸体残余继续只按 P11 residual discard 处理。
4. repeated Actor refresh、map reload、active P6 recovery、Actor EndPlay、close、lost focus 和 duplicate terminal observer 只能处理 matching transient projection，不能改变 durable item graph、删除 record、回填尸体或把 Actor 作为真值。
5. 成功后只刷新 exact BodyTarget source、new/exact WorldDropTarget、matching Actor 与必要 P6/P7 projection；不得 Sort、Compact、重排无关 SlotIndex、重建无关空间区域、清空无关选择、改变无关 scroll offset，或改变 P20/P21/P32—P43/P26—P31 的既有产品语义。

### 6. 允许的改动范围

仅允许在当前 Code B 中最小修改：

- P12/P21/P38/P39 已有 corpse-equipment source proof、shared Drag payload、normal Drop preview/commit、P11/P6 durable transaction 与 projection，仅用于 P44 exact P21 source；
- P14/P31 所在的 GroundDrop candidate、record insertion、canonical standard-root eligibility、accepted pickup proof、shared QuickTransfer source discrimination、Actor projection、P13 reconcile、P8 player-only filtering 与 rollback，仅用于表达 P44 accepted single-root record；
- P32/P33/P35/P34/P36/P37 已有 standard WorldDrop normal/QuickTransfer resolver、transient proof、current child/BaseQuick validation、formal equipment compatibility、exact cleanup 与 projection，仅用于接入 P44 exact record proof，不改变已有来源的产品语义；
- P1/P2/P3/P6/P11/P13/P17 的必要声明、candidate proof、replay、rollback 或调用签名兼容，前提是不改变已有产品语义；
- 必要的 production GroundDropZone / floor-placement / Actor lifecycle forwarding，只能是 Code B gate 的只读转发；Code A 不得取得库存、Loot、数量、WorldDrop 或 durable authority；
- PROJECT.md、PROJECT_INFO_CARD.md、本任务 Prompt 归档与本任务 Report。

禁止新建项目、版本线、Fix、第二 P11/P6、Widget inventory、Code A mirror、fixture、假 ItemId、clone、双写、存档重置或历史数据改写。禁止修改 Code A 功能逻辑、P12 search state machine、P21 deterministic source/history、P17 graph construction、P31 schema/registry、P8 receipt/terminal product logic、P5/P6 bridge 或任何测试文件。

### 7. 明确不在本任务内

- 不把 P12 ordinary simple stack、P20/P41/P42 complete graph、P17 child item、P9/P10 container、P5 warehouse、P6 player item、任何 other BodyTarget 或 existing WorldDrop root 接入 P44 corpse-drop source。
- 不实现 corpse partial drop、P11 Split/Merge/Swap 扩展、player → corpse quick return、快捷丢弃、Actor direct pickup、自动拾取、Take All、right-click Take、double-click、auto target、auto equipment、auto child open/switch、space parent/child 地面操作、world multi-item container、record-to-record transfer、cross-record merge、Swap、Replacement、Sort、Compact、Bind、Use、装备数值、战斗效果、HUD 接管、网络或多人。
- 不改变 P12 normal corpse → player Move/Merge/Swap、P38 normal direct pickup、P39 corpse Ctrl、P43 simple-stack drop、P26/P27/P28 quantity contract、P29 QuickTransfer priority、P30/P41 complete-graph path、P31 Registry identity、P13/P15、P17/P19、P5/P6/P8、搜索、敌人、地图、战斗、生命、死亡、撤离、商店、经济或制作。
- 不启动产品、PIE、Standalone、真实鼠标键盘输入、截图、Smoke、Automation、回归、试玩、Cook、Package 或最终验收。
- 不在回传 Report 前自动开始 P45、任意 Fix 或 F。

### 8. P 阶段静态审查与编译

完成后只执行以下检查：

1. 审查 P44 唯一合法 source 为 current opened/revealed/exact P21 standard non-spatial equipment root；P12 ordinary stack、space parent/child、Hidden/Searching、P9/P14/P31、player source、another BodyTarget 与 unknown family 均不能进入 P44。
2. 审查 normal Drag 至既有 GroundDropZone 是 corpse → world 的唯一输入；确认无新 pointer、Widget、Actor、quantity draft、QuickTransfer、right-click、double-click 或 Code A 写入旁路。
3. 审查 accepted drop path 只包含一个 P1 whole-root Move、一个 P31 new-record candidate、一个 P11/P6 Owner durable replacement、一次 SaveRecord 与 accepted-only Actor projection；确认没有 P6 staging、Merge、Split、new ItemId、second truth、actor-first write 或 ordinal gap。
4. 审查 exact P21 source proof、BodyTarget/death receipt/profile/candidate identity、Reveal/Open lifecycle、Owner/Run/revision/session、floor placement、P31 WorldDropId/Ordinal/container/root identity、P13 reconcile、BeforeSnapshot rollback 与 zero-write stale policy；任一失败不得改变 P11、Registry、Actor 或 other record。
5. 审查 P44 record 的 normal pickup 仅允许明确空 BaseQuick、明确 current valid child 空普通格或明确空兼容装备位；Ctrl 仅通过现有 shared frozen-target router 按输入时 child/BaseQuick mode 到一个 ordinary slot。确认没有 auto equip、target guess、mode fallback、new resolver 或尸体回存。
6. 审查 accepted P44 pickup 只清理 exact record，且 P32/P33/P35/P34/P36/P37 的 provenance/target semantics、P26—P29 simple-stack、P30/P41 complete graph、P38/P39 corpse pickup、P43 simple drop、P31 other-record isolation均保持。
7. 审查 P21 r1/r2/r3、already materialized records、deterministic identity、Optional.EquippedLoadout、candidate/weight/digest、三个固定尸体装备位、Hidden/Searching/Reveal 与 P6 → corpse rejection均未被改写；P8 terminal/recovery、P13/P15、P17/P19、P5/P6 bridge与 Code A authority保持。
8. 编译 Editor：

       "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_mapEditor Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload

9. 编译 Game：

       "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_map Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload

若编译失败，只修复本任务引入的 P21 source proof、GroundDrop/P31 transaction、existing standard-root pickup admission、shared QuickTransfer source discrimination、projection、rollback、include 或签名问题；若必须扩大到本任务以外，停止受影响部分并如实报告。

### 9. Report 与完成信号

生成 Dev.D.UE.0.0.9B.P44.0.r0_report.md，保存至：

    C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Docs\Report

Report 必须简洁、可审计地列出：

1. 本轮新增、修改、未修改的每个文件及职责；
2. P21 equipment Cell 到 GroundDropZone、P44 source proof、P1/P11/P31 durable transaction 和 accepted-only Actor projection 的完整调用图，以及没有新增 pointer、Widget、Actor、second resolver 或 Code A 写入口的证据；
3. P21 r3 actual stable source provenance/slot semantic、BodyTarget/death receipt/profile/candidate identity，以及为什么 r1/r2/history/determinism 未被改写；
4. exact source gate、Reveal/Open lifecycle、Owner/Run/revision/session/floor proof、P44 record identity与 zero-write cancellation/stale policy；
5. one P1 whole-root Move、single P31 new-record insertion、single P11/P6 durable replacement、single SaveRecord、replay proof、ItemId/Definition/Quantity/provenance保持、P13/P8、rollback、selection/scroll/stable SlotIndex 与无 P6 staging/second truth 的证据；
6. P44 exact record 如何仅接入既有 standard-equipment normal pickup和 shared frozen-target Ctrl router；明确的 BaseQuick/current child/equipment target验证、input-time child/BaseQuick mode、无 auto equip/no fallback/no P44-special input 的证据；
7. P32/P33/P35/P34/P36/P37、P26—P30、P38/P39、P41/P42、P43、P31、P5/P6/P8/P13/P15/P17/P19与 Code A authority的非回归结论；
8. git diff --check 结果、实际改动范围及所有明确未实现范围；
9. 两个编译命令、目标、原生 exit code 与关键结果；
10. 所有未执行的 F 阶段真实验证，至少包括：future BasicCorpse materialize/open/reveal；Body.Weapon、Body.ArmorRobe、Body.Accessory0 各自 normal Drag 至 GroundDropZone；multiple existing WorldDrop isolation；new Actor open/close；P44 record normal Drag 至 BaseQuick、current opened valid child 与 compatible equipment slot；P44 record Ctrl 于 valid child 与 no-child BaseQuick mode；child/BaseQuick full、child closed/switched/focus/generation/parent/capacity stale；wrong/ordinary/spatial/child/Hidden/Searching/player/another-body source；BodyTarget/death receipt/profile/candidate/Owner/Run/P6 revision stale；floor placement/record/ordinal/container/root mismatch；close/reopen、search/action、Prepared/terminal/Host invalid/SaveRecord failure；P38/P39、P43、P26—P30、P31 other-record isolation、P8 terminal/recovery；真实鼠标键盘、PIE、Standalone、截图、Smoke、Automation、回归、Cook 与 Package。

仅当 P44 corpse standard-equipment → independent WorldDrop 静态闭合、existing standard-equipment pickup/QuickTransfer chain、P31 identity、P11/P12/P21、P20/P38—P43 与 P8 边界保持，且 Editor 与 Game 均以 native exit code 0 完成时，使用：

    READY_FOR_P45_PLANNING

若当前范围内仍有可修复问题，使用：

    NEEDS_P44_REWORK

若现有 P12/P11/P6/P14/P31 transaction 或 shared standard-equipment resolver 无法在不创建第二输入路径、第二保存、第二物品真值、尸体 partial drop、自动装备、改写 P21 deterministic history 或扩展 Code A authority 的前提下支持本项，使用：

    NEEDS_PLANNER_DECISION
    BLOCKED

完成后不得自动开始 P45、Fix 或 F。向策划 Chat 回传并附带且只附带本次同名 Report；正文首行使用：

    [CSEMI:REPORT_SENT] {"task_id":"Dev.D.UE.0.0.9B.P44.0.r0","file":"Dev.D.UE.0.0.9B.P44.0.r0_report.md"}
