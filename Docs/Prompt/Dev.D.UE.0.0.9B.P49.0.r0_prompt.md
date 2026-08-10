# Dev.D.UE.0.0.9B.P49.0.r0

## 任务身份

- 项目：Dev.D.UE.0.0.9B；继续使用同一活动工程，不新建项目。
- 阶段：主线 P49——已打开、已揭示 P10 BasicCache 普通 simple stack 的显式地面落地，以及既有单根 WorldDrop 往返接入。
- 任务编号：Dev.D.UE.0.0.9B.P49.0.r0。
- 前置：已接受 0.0.9B.P1—P48 与 0.0.9BFix.P1—P4。Fix 只用于已确认、已验收功能的缺陷修复；P49 是新增主线功能，不是 Fix。
- 执行文件：Dev.D.UE.0.0.9B.P49.0.r0_prompt.md。
- 报告文件：Dev.D.UE.0.0.9B.P49.0.r0_report.md。
- 活动工程根：C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B。
- 活动工程：C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject。
- Report 必须生成到：C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Docs\Report\Dev.D.UE.0.0.9B.P49.0.r0_report.md，并仅携带该同名 Report 回传策划 Chat。
- 任务性质：P 阶段只做实现、静态审查和代码编译。不得启动产品、PIE、Standalone、真实鼠标键盘验证、截图、Smoke、Automation、回归、Cook、Package 或 F 阶段测试；不得自动开始 P50、任意 Fix 或 F。

## 上半部分：只读项目裁决、现状与边界

### 1. 当前唯一有效基线

唯一有效依据是当前 0.0.9B、活动工程和已接受任务链。所有 0.2、V2、V3、I、IPF、历史页面壳、旧 CTA、旧库存和旧物品规则均已过时；不得读取、采用、恢复或以其决定实现、验收或范围。

Code B P1 Repository 与既有 durable Store 是唯一可变物品真值。每件物品始终只有一个真实 ItemId、一个真实父位置和一条权威事务链。Widget、Cell、Presenter、DragOperation、Workspace Context、NormalContainerTarget、WorldDrop Actor、地图放置适配层和 Code A 都只能持有只读投影、选择或瞬时意图；不得持有第二库存、可写数量副本、预建 ItemId、平行容器背包、Actor-first 写入或 A/B 双写。

P9/P10 已建立唯一生产普通容器 M01.CodeBNormalContainer.BasicCache.01、其 exact BasicCache Run-local record，以及 Hidden → Searching → Revealed、Open、focus/page 与 normal Drag/Drop 生命周期。P18 对尚未物质化的 BasicCache r2 增加了确定性 optional spatial utility，但保留 `SpiritDust`／`IronShard` 主材料组；已物质化 record 必须始终按其 actual receipt、ProfileId、ProfileVersion、ProfileDigest、AlgorithmVersion 与 ResultDigest 保持原样。P49 既不得 reroll、补料、迁移或改写任一 BasicCache 历史，也不得把 r2 spatial gate 当作 simple-stack 来源的替代证明。

P14、P26—P29 已确立 simple world root 的正常整堆拾回、明确数量拾回、normal Merge 与 Ctrl + 左键工作流；P31 已将地面状态收敛为 Owner/Run scoped、多 record、每 record 单 root 的 Registry。每个已接受 record 只包含一个 root、一个 derived world container、一个 WorldDropId、一个 Ordinal 和一个 Actor projection。P49 只为下文严格定义的一类 P10 ordinary simple-stack root 建立窄的“容器 → 新 WorldDrop”来源，不把地面变成多物品背包。

P46 已为同一 P10 BasicCache 的 `SpiritDust`／`IronShard` simple stack 建立 Ctrl + 左键的 player QuickTransfer；P47/P48 分别处理 P18 complete spatial graph 的 BaseQuick-only Ctrl 与显式兼容装备位 normal Drag。P49 不改变 P46、P47 或 P48，也不把空间 parent、child 或装备 root 伪装成 simple stack。

### 2. P49 产品裁决

当玩家已经通过 P10 主动打开 identity-valid 的唯一 BasicCache，且一个 exact ordinary root 已 Revealed、为正式 non-spatial simple stack 时，玩家可以从该 root Cell 发起既有 normal DragOperation，并把它 Drop 到既有 GroundDropZone。成功时，同一个 root 以一次 P1 whole-root Move 从 exact P9 source 直接进入一个新建、独立 P31 WorldDrop record 的 derived world container slot 0。

| 明确动作 | 唯一允许来源 | 唯一允许目标 | 权威结果 |
| --- | --- | --- | --- |
| BasicCache 普通堆叠主动落地 | 已打开、已揭示、identity-valid P10 ordinary BasicCache 中的 `SpiritDust` 或 `IronShard` simple-stack root | 既有 GroundDropZone | 一个 P1 Move；同一 Owner durable replacement 同时更新 P9 source 与 P31/P6 新 record，并只保存一次。 |
| 地面正常／按数量／快捷拾回 | 当前已打开、identity-valid 的 P49 single-root simple WorldDrop | 保持 P26、P27、P28、P29 的既有明确目标或冻结目标 | 不创建 P49 专用输入；仅使已经 accepted 的 P14/P31 simple-root 工作流按 exact record 身份处理该 root。 |

P49 的落地只处理整个 root，不处理数量 N。没有 BasicCache 来源的 SplitDraft、WorldPickupDraft、RequestedMergeQuantity 或 partial drop；玩家若需要拆分，必须先经既有、明确的 player-side 工作流取得物品后，再使用其已授权功能。P49 也不把现有 P26—P29 的拾回策略改成 BasicCache 回存，地面 root 不得拖回 P9、另一 BasicCache 或任何普通容器。

每个 accepted P49 Drop 必须新建一个 independent P31 record；不得合并进当前已打开 record、任意既有地面 root 或某个“同类”物品。该 P49 record 成功后只作为 canonical simple world root 参与 P26—P29 已有的 exact-record normal pickup、partial pickup 和 QuickTransfer 语义；不得由其 origin 产生新的自动目标、第二 record、第二 Actor、child 访问或额外保存。

### 3. 严格范围与持续排除

P49 source 只接受同时满足以下全部条件的 root：

1. 当前 Workspace 为既有 P10 production Host；exact BasicCache NormalContainerTarget 已 Open、已 Revealed、identity-valid，仍属于 active M01 BasicCache route、focus、target-open generation 与 active session，且无 active open/search action。OwnerId、RunInstanceId、SearchTargetId、DefinitionId、P9 receipt、record revision、P6 composite revision、route、focus 与 active session 都能在 Preview 和 durable Commit 前从 P1/P6/P9 truth 重验；
2. source 位于 exact P9 RunLocalNormalContainerRecord 的 ordinary stable ContainerId + SlotIndex，不属于 P6 player source、P11/P12 corpse、P14/P31 WorldDrop、P5 warehouse、P17 child Cell、Hotbar、another NormalContainer 或 UI-only projection；
3. Catalog、P1 snapshot、P9 receipt/history、P10 NormalContainerTarget 与 source reverse slot pointer 共同证明 root 是正式 `SpiritDust` 或 `IronShard` simple stack：bStackable、Quantity 大于 0、MaxStack 大于 1、无 ChildContainerId、无 spatial semantic、无 complete-graph closure、不是装备 root、不是 Hotbar reference、不是任何 draft；
4. source proof 必须冻结 exact ItemId、DefinitionId、StackKey、Quantity、MaxStack、source ContainerId、SlotIndex、NormalContainerTarget/SearchTargetId/receipt identity、P9/P6 revision、target-open generation、route/focus 与必要 session proof。不得以显示名、图标、Cell class、容器展示顺序、Actor pointer、地图坐标、当前选择或 UI cache 判断资格；
5. GroundDropZone route/floor placement、active Owner/Run、Prepared/terminal gate、Host validity 和 P31 accepted-create proof 也必须在 Preview 与 Commit 前保持成立。

下列对象或动作持续不属于 P49：

1. P18 canonical `WindTalisman`／`BackpackLevel1` complete graph、任何 ChildContainer、P17 child item、复杂 parent、space parent、nested graph 或其 child closure；P21 fixed Weapon、ArmorRobe、Accessory0 corpse equipment；P11/P12 corpse、P14/P31 WorldDrop、P5 warehouse、P6 player source、Hotbar、another NormalContainer、Hidden/Searching root、empty source slot 或 UI-only object；
2. Ctrl + 左键、QuickTransfer、quick-drop、P6 → BasicCache quick return、Actor direct pickup、距离自动拾取、交互键领取、right-click Take、double-click、Take All、auto equip、auto bind、auto use、auto target、auto child open/switch、Swap、Replacement、Sort、Compact、Merge、Split、Quantity=N、PlayerSplitDraft、WorldPickupDraft、record-to-record transfer 或 BasicCache partial drop；
3. P10 ordinary BasicCache → P6 的 normal Move/Merge/Swap、P46 BasicCache QuickTransfer、P47/P48 spatial routes、P42 corpse spatial equipment path、P38—P45 corpse routes、P26—P29 source/target ordering、P31 Registry schema、P8 terminal classification、P13 binding、P15 use 和 Code A authority；
4. 实机运行、PIE、Standalone、真实鼠标键盘、截图、Smoke、Automation、回归、Cook、Package 或最终验收。

## 下半部分：授权执行内容

### 4. 单一授权目标

在不建立第二物品真值、不改变 P9/P10 normal-container state machine、P18 deterministic materialization/history、P17 graph、P31 multi-record Registry、P26—P29 public interaction semantics 或 Code A authority，也不新增任何自动行为的前提下，将 exact P10 ordinary `SpiritDust`／`IronShard` simple-stack root 接入既有 GroundDropZone / P31 流程：

1. 只有已打开、已揭示、identity-valid 的 exact P10 ordinary simple-stack root 才可经 normal Drag 建立一次 P49 world-drop transient intent；
2. Preview 只建立一个 Owner/Run scoped candidate，冻结 exact P9 source 与 accepted floor placement；Commit 不得重扫 source、改投 P6、改投另一 NormalContainer 或 fallback 到旧 WorldDrop；
3. accepted path 只形成一个 P1 whole-root Move、一个 P31 new-record insertion、一个 P9/P6 Owner durable replacement 和一次 SaveRecord；
4. 只有 accepted snapshot/replay proof 明确证明同一 root 已离开 exact P9 source 并进入新 record 的 exact derived world container slot 0 后，才可刷新 NormalContainerTarget source、record、Actor projection 与必要 workspace projection；
5. 新 record 仅在 canonical P14/P31 simple-root truth 层面加入 P26—P29 的既有 exact-record pickup eligibility；它不获得 P49 专用按钮、数量草稿、快捷路径或 BasicCache target；
6. 任一 source、identity、lifecycle、placement、record、session、P1 或保存检查失败时，完整零写入并恢复 BeforeSnapshot。

### 5. 实现要求

#### 5.1 先完成活动调用链与资格审计

改动前必须审阅并在 Report 中列出：

1. P10 NormalContainerTarget open/reveal lifecycle、ordinary BasicCache root Cell、normal DragOperation、GroundDropZone 可达性、NativeOnDrop、page/focus invalidation、P9/P6 callback 与 source proof；说明 P49 如何复用既有生产 Cell 和现有 GroundDropZone，而不是新增 Button、hotkey、pointer handler、Widget、Actor 或 normal-container 写入口；
2. P46 的 P10 ordinary simple-stack source proof、Reveal/Open gate、NormalContainerTarget/SearchTargetId/receipt identity、frozen revision/session lifecycle；说明 P49 只复用其 canonical source qualification，不取得 P46 Ctrl intent；
3. P14、P26—P29 与 P31 的 GroundDrop request、floor placement、new-record candidate、WorldDropId/Ordinal、derived world container、Actor diff、exact target open、normal/partial/QuickTransfer pickup、record cleanup、recovery 与 P8 player-only filtering 调用图；
4. P1/P2/P3/P6/P9 的 whole-root Move、cross-graph single candidate、accepted snapshot/replay proof、BeforeSnapshot rollback、P9/P6 single Owner durable replacement、P13 reconcile 与 revision/session 调用图；
5. P18/P47/P48 spatial graph、P11/P12/P20/P21 corpse 与 P31 existing-record isolation 的 source-family branch；说明 P49 如何只扩大 P10 ordinary simple root → new WorldDrop；
6. 所有 Widget direct Move/quantity mutation、Actor direct pickup、NormalContainer display-cache write、right-click Take、double-click、预建 ItemId/WorldDropId/Ordinal、按 first/last/current record 猜测 target、第二 Repository transaction、第二 save 或 Code A inventory writer；它们不得成为 P49 写入路径。

#### 5.2 共享 normal Drag、source gate 与 transient proof

1. P10 ordinary root Cell 的既有 normal DragOperation 与 GroundDropZone::NativeOnDrop 或当前等价正式入口仍是 P49 的唯一输入与提交入口。不得为 P49 新建 Button、hotkey、Actor click、专用 Widget、第二 pointer handler、second resolver 或 UI direct write。
2. ordinary left-click 继续只选择或按 P10 既有规则开始 search；right-click 继续只读详情；Ctrl + 左键继续只能进入既有 P46 或其他授权 QuickTransfer route；double-click、Tab、I、Esc、Close、Cancel、scroll、空白区、无 payload Drop、Shift + 1—9、player-side drag 与任何 Code A interaction 均不得形成 P49 位置写入。
3. source payload 只能由 current Open、Revealed、identity-valid exact P10 ordinary `SpiritDust`／`IronShard` simple-stack root 建立。它必须冻结 OwnerId、RunInstanceId、SearchTargetId、ReceiptId、BasicCache DefinitionId、P9 record revision、source ContainerId、source SlotIndex、root ItemId、DefinitionId、StackKey、Quantity、MaxStack、P6 revision、target-open generation、route/focus 与必要 active-session proof。
4. Preview 与 durable Commit 前必须重新验证 source root 仍在同一 exact P9 ordinary BasicCache slot、visibility 仍为 Revealed、simple-stack qualification 和 receipt/history provenance 不变、NormalContainerTarget 仍 Open，且 Owner/Run/session/Prepared/terminal/Host/route/focus/revision gate 全部成立。不得让 P18 spatial parent/child、P11/P12 corpse、P14/P31、player source、another NormalContainer 或 UI projection 建立 P49 intent。
5. NormalContainerTarget close/reopen、focus loss、开始/取消 search、Actor EndPlay、map reload、recovery、terminal、Prepared、receipt/root/container mismatch、payload cancel、Host invalidation、source revision stale、floor placement stale、Registry collision 或 SaveRecord failure 必须立即使 intent/candidate 失效并零写入。不得改投 BaseQuick、P17 child、equipment slot、旧 WorldDrop、另一 NormalContainer 或其他位置。
6. payload 不得包含可写 quantity cache、split amount 或预建 world identity。它只能代表一个 existing whole root；不得调用 Merge、Split、RequestedMergeQuantity、Quantity=0、WorldPickupDraft、PlayerSplitDraft、graph Move 或任何 partial-transfer shortcut。

#### 5.3 exact P9 source → new P31 WorldDrop record

1. GroundDropZone 先只通过既有 Code A floor-placement adapter 解析合法、不可变的 route/floor placement；该 adapter 只能转发展示和生命周期边缘，不得创建 ItemId、WorldDropId、record、ordinal、数量、P9 graph 或 P6 graph。
2. Preview 成功后只建立一个 Owner/Run scoped candidate。该 candidate 必须以一个 P1 whole-root Move 将同一个 source ItemId 从 exact P9 ordinary BasicCache slot 直接移入新 record 的 derived world container slot 0；不得先落 P6 BaseQuick、先清 P9、先建立可见 Actor、先写临时 container，或执行 two-step relocation。
3. accepted root 的 ItemId、DefinitionId、StackKey、Quantity、Level、Quality、RandomSeed、LegacyAffixDigest、P9 receipt/history provenance 与无 child simple-stack qualification 必须原样保持。不得 Merge、Split、Swap、clone、new ItemId、new ChildContainer、parent-only graph move、temporary player item 或第二物品真值。
4. NextWorldDropOrdinal、derived world container、WorldDropId、record insertion、P9 source removal、P6/P14 record update、P13 necessary reconcile 和 Actor projection 只能在同一个 accepted Owner durable replacement 中出现，并且函数内只有一次 SaveRecord。Actor 仅在 accepted durable projection 后创建或刷新。
5. 新 record 必须保留 P31 的 exact identity：OwnerId、RunInstanceId、WorldDropId、Ordinal、map route/floor placement、Available state、derived ContainerId、root ItemId、record revision 与 P49 ordinary-BasicCache provenance。实际 provenance literal 可沿活动 schema 命名，但必须独立、可审计，且不得与 P14 player world source、P43 corpse simple source、P44 corpse equipment、P45 corpse spatial 或 existing record source 伪装或混用。
6. Store 必须以 accepted command 的相同 TransactionId、source、target、revision 与 record proof 重放一次 P1 Move，并要求 replay snapshot 与 accepted composite 完全相等；只有 replay 证明 root 已离开 exact P9 source 并到达 exact new world container 后，才刷新 NormalContainerTarget source 与 P31 projection。
7. candidate、source/placement/Registry validation、P1 Move、P13 reconcile、record/Actor projection 前检查或 SaveRecord 任一失败时，完整恢复 BeforeSnapshot：P9 source 仍在原 stable SlotIndex，P6/P14 Registry、NextWorldDropOrdinal、existing records、new record、derived container、Actor、NormalContainerTarget visibility/open state、selection 与无关 projection 均保持旧值；不得遗留 phantom empty、phantom world root、ordinal gap、orphan container 或 transient item copy。

#### 5.4 P49 record 与既有 pickup 路径的受限衔接

1. P49 成功创建的 record 是一个 canonical P14/P31 simple world root。P26 normal whole-root pickup、P27 explicit empty-target quantity pickup、P28 explicit compatible-stack quantity pickup 与 P29 current-opened-record QuickTransfer 必须只按 current exact record、simple-stack truth、Owner/Run/revision、stable source address 与其既有 target policy 判断；不得因为该 root 来自 P9 而错误拒绝、改写数量规则或发明 P49 专用输入。
2. P49 不重做上述四条路径。P26 的 normal full Move、P27/P28 的 explicit N 与 P29 的 frozen target/merge-first empty-second order 必须保持；P49 record 仅在它已被打开、identity-valid 且满足同一 canonical simple-root gate 时进入既有 resolver。
3. normal full pickup 成功时只清理这个 exact P49 record、其空 derived container 和对应 Actor；partial Split/Merge 成功时保留同一 root ItemId、world container、WorldDropId、Ordinal 和 Actor，并只用 accepted projection 更新 Quantity；P29 player → current ground Merge 仍只能针对当前 exact opened compatible root，不得新建第二 record 或选择其他 record。
4. P49 record 绝不成为 P9 target、BasicCache 回存 target、equipment slot、space parent/child、Hotbar、P5/P11、another NormalContainer 或 cross-record target。任何 close/focus loss/Actor EndPlay/record-root-container mismatch/ordinal mismatch/stale revision/terminal/Prepared/Host invalid/SaveRecord failure 均沿现有 path 零写入。
5. 不得让 P27/P28 的 WorldPickupDraft 或 P29 Ctrl pointer 在 P10 BasicCache Cell、BasicCache Actor 或未打开 WorldDrop 上触发。它们只能继续在各自原有的明确 UI 生命周期内处理 already-opened exact world root。

#### 5.5 单一事务、终局、回滚与非回归

1. P49 的 BasicCache → world Drop 必须沿既有 P10/P3 → P2 → P1 → P9/P14/P31/P6 durable callback；每次 input 最多一个 candidate、一次 Owner record replacement 与一次 SaveRecord。不得先写 P9 后写 P6，也不得先写 record/Actor 后写 source。
2. P13 只沿既有 accepted commit reconcile；不得自动 Bind、Use、Equip、复制 binding 或在 BasicCache root 上创建 binding。P15 保持其现有 RestoreHealth-only scope。
3. P8 BuildP14PlayerOnlySession 或等价 finalization 必须把 P49 record 的 root、derived world container 及所有 other Registry records 从 player-only final graph 排除。P49 不修改 terminal classification、P5 receipt、recovery、run replacement 或 Code A terminal authority；仍留在 P9 的 BasicCache residual 继续只按 P9 residual discard 处理。
4. repeated Actor refresh、map reload、active P6 recovery、Actor EndPlay、close、lost focus 和 duplicate terminal observer 只能处理 matching transient projection，不能改变 durable item graph、删除 record、回填 BasicCache 或把 Actor 作为真值。
5. 成功后只刷新 exact NormalContainerTarget source、new/exact WorldDropTarget、matching Actor 与必要 P6 projection；不得 Sort、Compact、重排无关 SlotIndex、重建无关空间区域、清空无关选择、改变无关 scroll offset，或改变 P10/P18/P46/P47/P48/P26—P31/P43—P45 的既有产品语义。

### 6. 允许的改动范围

仅允许在当前 Code B 中最小修改：

- P10/P46 已有 ordinary-BasicCache source proof、shared Drag payload、normal Drop preview/commit、P9/P6 durable transaction 与 projection，仅用于 P49 exact BasicCache simple-stack source；
- P14/P26—P29/P31 所在的 GroundDrop candidate、canonical simple-root eligibility、record insertion、accepted pickup proof、Actor projection、P13 reconcile、P8 player-only filtering 与 rollback，仅用于 P49；
- P1/P2/P3/P6/P9/P13/P17 的必要声明、candidate proof、replay、rollback 或调用签名兼容，前提是不改变已有产品语义；
- 必要的 production GroundDropZone / floor-placement / Actor lifecycle forwarding，只能是 Code B gate 的只读转发；Code A 不得取得库存、Loot、数量、WorldDrop、P9/P6 或 durable authority；
- PROJECT.md、PROJECT_INFO_CARD.md、本任务 Prompt 归档与本任务 Report。

禁止新建项目、版本线、Fix、第二 P9/P6、Widget inventory、Code A mirror、fixture、假 ItemId、clone、双写、存档重置或历史数据改写。禁止修改 Code A 功能逻辑、P10 search state machine、P18 profile/history、P17 graph construction、P31 schema/registry、P8 receipt/terminal product logic、P5/P6 bridge 或任何测试文件。

### 7. 明确不在本任务内

- 不把 P18 spatial parent/child、P11/P12 corpse、P20/P21 corpse graph/equipment、P14/P31 existing WorldDrop、P5 warehouse、P6 player item、P17 child item、Hotbar、any other NormalContainer 或 unknown root 接入 P49 BasicCache-drop source。
- 不实现 BasicCache partial drop、P6 → BasicCache return、Ctrl quick-drop、Actor direct pickup、自动拾取、Take All、right-click Take、double-click、auto target、auto equipment、auto child open/switch、space parent/child 地面操作、world multi-item container、record-to-record transfer、cross-record merge、Swap、Replacement、Sort、Compact、Bind、Use、装备数值、战斗效果、HUD 接管、网络或多人。
- 不改变 P10 normal BasicCache → player Move/Merge/Swap、P46 QuickTransfer、P47/P48 spatial graph、P42、P38—P45、P26—P29 quantity contract、P31 Registry identity、P13/P15、P17/P19、P5/P6/P8、搜索、尸体、敌人、地图、战斗、生命、死亡、撤离、商店、经济或制作。
- 不启动产品、PIE、Standalone、真实鼠标键盘输入、截图、Smoke、Automation、回归、试玩、Cook、Package 或最终验收。
- 不在回传 Report 前自动开始 P50、任意 Fix 或 F。

### 8. P 阶段静态审查与编译

完成后只执行以下检查：

1. 审查 P49 唯一合法 source 为 current opened/revealed/exact P10 BasicCache ordinary `SpiritDust`／`IronShard` simple stack；P18 spatial graph、P11/P12、P14/P31、P5/P6、P17 child、Hotbar、Hidden/Searching、another NormalContainer 与 unknown family 均不能进入 P49。
2. 审查 normal Drag 至既有 GroundDropZone 是 BasicCache → world 的唯一输入；确认无新 pointer、Widget、Actor、quantity draft、QuickTransfer、right-click、double-click 或 Code A 写入旁路。
3. 审查 accepted drop path 只包含一个 P1 whole-root Move、一个 P31 new-record candidate、一个 P9/P6 Owner durable replacement、一次 SaveRecord 与 accepted-only Actor projection；确认没有 P6 staging、Merge、Split、new ItemId/ContainerId/child、second truth、actor-first write 或 ordinal gap。
4. 审查 exact P10 source proof、NormalContainerTarget/SearchTargetId/receipt/history identity、Reveal/Open lifecycle、Owner/Run/revision/session、floor placement、P31 WorldDropId/Ordinal/container/root identity、P13 reconcile、BeforeSnapshot rollback 与 zero-write stale policy；任一失败不得改变 P9、Registry、Actor 或 other record。
5. 审查 P49 record 只接入既有 P26 normal pickup、P27/P28 explicit quantity pickup 与 P29 frozen QuickTransfer；确认没有 BasicCache return、P49-special pointer、new resolver、mode fallback 或自动行为。
6. 审查 P10/P18 materialization/history、r1/r2 actual receipt、optional spatial gate、weight/digest、P17 one-layer restriction、Hidden/Searching/Reveal 与 P6 → BasicCache rejection均未被改写。
7. 审查 P10 normal Move/Merge/Swap、P46、P47/P48、P42、P38—P45、P26—P31 other-record isolation、P8 terminal/recovery、P13/P15、P5/P6 bridge与 Code A authority保持。
8. 执行 git diff --check，并在 Report 中列出实际修改文件、权威写入调用图与所有未修改的权威边界。
9. 编译 Editor：

       "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_mapEditor Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload

10. 编译 Game：

       "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_map Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload

若编译失败，只修复本任务引入的 P10 simple source proof、GroundDrop/P31 transaction、P26—P29 pickup admission、projection、rollback、include 或签名问题；若必须扩大到本任务以外，停止受影响部分并如实报告。

### 9. Report 与完成信号

生成 Dev.D.UE.0.0.9B.P49.0.r0_report.md，保存至：

    C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Docs\Report

Report 必须简洁、可审计地列出：

1. 本轮新增、修改、未修改的每个文件及职责；
2. P10 simple root Cell 到 GroundDropZone、P49 source proof、P1/P9/P31 durable transaction 和 accepted-only Actor projection 的完整调用图，以及没有新增 pointer、Widget、Actor、second resolver 或 Code A 写入口的证据；
3. P10 actual stable ordinary source、NormalContainerTarget/SearchTargetId/receipt/history identity、simple-stack definition/quantity 与为何 P9/P18 materialization/history/determinism 未被改写；
4. exact source gate、Reveal/Open lifecycle、Owner/Run/revision/session/floor proof、P49 record identity与 zero-write cancellation/stale policy；
5. one P1 whole-root Move、single P31 new-record insertion、single P9/P6 durable replacement、single SaveRecord、replay proof、root/provenance保持、P13/P8、rollback、selection/scroll/stable SlotIndex 与无 P6 staging/second truth 的证据；
6. P49 exact record 如何仅接入既有 P26 normal pickup、P27/P28 quantity pickup和 P29 QuickTransfer；明确没有 BasicCache return、P49-special input、fallback 或自动行为的证据；
7. P10/P18/P46/P47/P48、P26—P31、P38—P45、P5/P6/P8/P13/P15/P17与 Code A authority的非回归结论；
8. git diff --check 结果、实际改动范围及所有明确未实现范围；
9. 两个编译命令、目标、原生 exit code 与关键结果；
10. 所有未执行的 F 阶段真实验证，至少包括：BasicCache materialize/open/reveal；`SpiritDust` 与 `IronShard` 各自 normal Drag 至 GroundDropZone；multiple existing WorldDrop isolation；new Actor open/close；P49 record normal full pickup、P27/P28 partial pickup 与 P29 QuickTransfer；empty/full/wrong player target、partial quantity、wrong spatial/equipment/child/corpse/player/another-normal/Hidden/Searching source；NormalContainerTarget/SearchTargetId/receipt/Owner/Run/P9/P6 revision stale；floor placement/record/ordinal/container/root mismatch；close/reopen、search/action、Prepared/terminal/Host invalid/SaveRecord failure；P10 normal Move/Merge/Swap、P46/P47/P48、P26—P31、P38—P45、P8 terminal/recovery、真实鼠标键盘、PIE、Standalone、截图、Smoke、Automation、回归、Cook 与 Package。

仅当 P49 BasicCache simple stack → independent WorldDrop 静态闭合、existing P26—P29 pickup chain、P31 identity、P9/P10/P18 与 P8 边界保持，且 Editor 与 Game 均以 native exit code 0 完成时，使用：

    READY_FOR_P50_PLANNING

若当前范围内仍有可修复问题，使用：

    NEEDS_P49_REWORK

若现有 P10/P9/P6/P14/P31 transaction 或 P26—P29 simple-world resolver 无法在不创建第二输入路径、第二保存、第二物品真值、数量拆分、自动拾取、改写 P9/P18 deterministic history 或扩展 Code A authority的前提下支持本项，使用：

    NEEDS_PLANNER_DECISION
    BLOCKED

完成后不得自动开始 P50、Fix 或 F。向策划 Chat 回传并附带且只附带本次同名 Report；正文首行使用：

    [CSEMI:REPORT_SENT] {"task_id":"Dev.D.UE.0.0.9B.P49.0.r0","file":"Dev.D.UE.0.0.9B.P49.0.r0_report.md"}
