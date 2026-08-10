# Dev.D.UE.0.0.9B.P46.0.r0

## 任务身份

- 项目：Dev.D.UE.0.0.9B；继续使用同一活动工程，不新建项目。
- 阶段：主线 P46——已打开、已揭示 P10 BasicCache simple stack 的受限 Ctrl + 左键快捷拾回。
- 任务编号：Dev.D.UE.0.0.9B.P46.0.r0。
- 前置：已接受 0.0.9B.P1—P45 与 0.0.9BFix.P1—P4。Fix 仅用于已确认、已验收功能的缺陷修复；P46 是新增主线功能，不是 Fix。
- 执行文件：Dev.D.UE.0.0.9B.P46.0.r0_prompt.md。
- 报告文件：Dev.D.UE.0.0.9B.P46.0.r0_report.md。
- 活动工程根：C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B。
- 活动工程：C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject。
- Report 必须生成到：C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Docs\Report\Dev.D.UE.0.0.9B.P46.0.r0_report.md，并仅携带该同名 Report 回传策划 Chat。
- 任务性质：P 阶段只做实现、静态审查和代码编译。不得启动产品、PIE、Standalone、真实鼠标键盘验证、截图、Smoke、Automation、回归、Cook、Package 或 F 阶段测试；不得自动开始 P47、任意 Fix 或 F。

## 上半部分：只读项目裁决、现状与边界

### 1. 当前唯一有效基线

唯一有效依据是当前 0.0.9B、活动工程和已接受任务链。所有 0.2、V2、V3、I、IPF、历史页面壳、旧 CTA、旧库存和旧物品规则均已过时；不得读取、采用、恢复或以其决定实现、验收或范围。

Code B P1 Repository 与既有 durable Store 是唯一可变物品真值。每件物品始终只有一个真实 ItemId、一个真实父位置和一条权威事务链。Widget、Cell、Presenter、DragOperation、Workspace Context、NormalContainerTarget、QuickTransfer resolver、空间 child 投影、WorldDrop Actor、地图放置适配层和 Code A 都只能持有只读投影、选择或瞬时意图；不得持有第二库存、可写数量副本、预建 ItemId、平行容器背包、Actor-first 写入或 A/B 双写。

P9/P10 已建立唯一生产普通容器 M01.CodeBNormalContainer.BasicCache.01 和其 exact BasicCache Run-local record。其 durable identity 由 OwnerId、RunInstanceId、SearchTargetId、DefinitionId、receipt、record revision 与完整 P9 item graph 共同约束。BasicCache 的当前正式内容为 canonical non-spatial simple stack，包含 SpiritDust 与 IronShard；它们仅在 P10 Target 为 Open 且 exact item 为 Revealed 后，才可作为真实生产 Cell 的位置写入 source。P10 已有 normal Drag 的 Move、Merge、Swap；P46 不改变这些语义、P9/P10 materialization、receipt、search/reveal state machine、SearchTarget identity 或 P8 对 P9 残余的 discard。

P26—P29 已确定 simple stack 的正式事务语义：在一个已经解析的目标 container 内，先按真实 stable SlotIndex 升序寻找 Definition/StackKey compatible 且未满的 existing stack；仅在没有被接受的 Merge candidate 时，才按 stable SlotIndex 升序寻找合法空 ordinary storage cell。compatible stack 只走一次 P1 Merge，Quantity=0；P1 是唯一可以裁决完整或部分接收量的地方。若 P1 只接受一部分，source 保持同一 ItemId、原 source slot 与剩余 Quantity；同一手势不得继续扫描空格或二次 Move。

P17 已建立唯一正式空间 parent → ChildContainer 图与 current opened child 投影。P36、P37、P39、P40 已把共享 Ctrl + 左键 QuickTransfer 的自动目标收敛为输入时冻结的两种模式：

1. 输入时存在 identity-valid current P17 child 时，只解析该 exact child；
2. 输入时根本不存在 valid current P17 child 时，才解析 P6 BaseQuick。

被选 child 在之后关闭、切换、失焦、失效、拓扑变化或填满时必须拒绝，绝不静默改投 BaseQuick、另一 child、装备位、Hotbar、仓库、尸体、地面或其他位置。P46 只将这个既有模型和 P29 的 simple-stack merge-first / empty-second 顺序接入 P10 BasicCache source。

P43—P45 已完整覆盖尸体三种 root 拓扑的 normal GroundDrop；P46 不接入 GroundDrop，也不修改 P31 WorldDrop registry、P26—P30 world pickup、P38—P45 corpse 路径或任何 WorldDrop provenance。

### 2. P46 产品裁决

当玩家已经通过 P10 主动打开 identity-valid 的唯一 BasicCache，且其中一个 exact ordinary root 已 Revealed，玩家对该 root Cell 按下 Ctrl + 左键时，必须由既有共享 QuickTransfer resolver 处理。P46 只接受 exact P9/P10 BasicCache 的正式 non-spatial simple stack；它不处理尸体、WorldDrop、standard equipment、空间 parent/child、其他 NormalContainer、仓库或玩家 source。

自动目标只在输入时决定一次：

| 输入时 source | 输入时目标模式 | 唯一允许的自动目标顺序 | 权威事务 |
| --- | --- | --- | --- |
| exact opened/revealed P10 BasicCache simple-stack root | 当前存在 identity-valid P17 child | 只在 exact current child 的 ordinary storage cells 内：先 stable SlotIndex 升序 compatible underfull stack，后 stable SlotIndex 升序空 ordinary cell | occupied：一个 P1 Merge(Quantity=0)；empty：一个 P1 Move |
| exact opened/revealed P10 BasicCache simple-stack root | 当前不存在 identity-valid P17 child | 只在 active P6 BaseQuick 的 ordinary storage cells 内：先 stable SlotIndex 升序 compatible underfull stack，后 stable SlotIndex 升序空 ordinary cell | occupied：一个 P1 Merge(Quantity=0)；empty：一个 P1 Move |

这是一个已打开容器窗口中的一次快捷拾回请求，不是 Take All、自动拾取、自动装备、自动打开空间道具、容器回存、快捷丢弃或地图 Actor 直接领取。每次 Ctrl + 左键只处理当前 exact source root、最多建立一个 candidate，并只提交一次 P1 transaction。

若输入时选择 CurrentP17Child，intent 必须冻结 exact child ContainerId、formal space parent ItemId、child open generation、Owner/Run、NormalContainerTarget/SearchTargetId、P9 receipt、container/open/composite/P6 revision 与必要 route/focus/session proof。commit 前 child 关闭、切换、失焦、generation 失效、parent/child topology 失效、target stale 或没有合法 candidate 时一律拒绝；不得转为 BaseQuick。

只有输入时根本不存在 identity-valid current P17 child，才可建立 BaseQuickNoChildAtInput 模式。输入后打开、切换或投影出 child 不得改变该模式。BaseQuick 满、source/target stale、Owner/Run/P6 revision 不匹配或任一 session、保存或 lifecycle 检查失败时必须零写入。

### 3. 严格范围与持续排除

P46 source 只接受同时满足以下全部条件的 root：

1. 当前 Workspace 是既有 P10 production Host；exact NormalContainerTarget 为 Open、identity-valid，且它仍属于 active M01 BasicCache route、focus、target-open generation 与 active session；
2. source 位于 exact P9 RunLocalNormalContainerRecord 的 ordinary stable address，且 P9/P10 receipt、Catalog、projection、SearchTargetId、DefinitionId、ContainerId、SlotIndex 和 source ItemId 共同证明它属于 M01.CodeBNormalContainer.BasicCache.01；不得由显示名、图标、Cell class、Actor pointer、展示顺序、当前选择或 UI cache 判断；
3. source 已 Revealed，且是正式 Definition/StackKey-compatible simple stack：bStackable、Quantity 大于 0、MaxStack 大于 1、无 ChildContainerId、无 spatial semantic、无 graph closure、不是 equipment root、不是 Hotbar reference、不是 draft，并且 source slot 反向指向同一 ItemId；
4. Preview 和 durable Commit 前，OwnerId、RunInstanceId、SearchTargetId、DefinitionId、P9 receipt、target/open/container/composite/P6 revision、source container/slot/root、Reveal state、route、focus、active session、Prepared/terminal gate、Host validity 与 mode-specific identity 均必须仍可从 Code B durable truth 重验。

P46 不取消、推进、绕过或另行解释 P10 的 Opening、Interrupted、Hidden、Searching、Revealed 状态机。当前 exact source 不是 Revealed 时不得形成 intent；对其他 item 正在进行的既有 P10 search/action，继续按 P10 原规则执行，P46 不读取或写入其 action state。

CurrentP17Child 模式还必须同时满足：

1. 输入时 Workspace 已有一个 current、opened、identity-valid 的 P17 child；intent 必须携带 exact parent ItemId、child ContainerId、open generation 与 P6 revision；
2. durable Store 从 P1/P6 重建唯一 active P17 parent → child 映射，验证 parent 位于正式 SpatialRing 或 Backpack placement、空间定义、dynamic ChildContainerCapacity、一层无环规则、child ordinary slot semantic 与 active session；不得由显示顺序、焦点或 UI cache 推断；
3. target scan 只能在该 exact child 内进行：先真实 stable SlotIndex 升序 compatible underfull simple stack，后真实 stable SlotIndex 升序正式、可写、空 ordinary storage cell；
4. child 不存在、关闭、切换、失焦、generation 不匹配、parent mismatch、capacity/slot semantic 失效、无合法 candidate 或 revision stale 时均零写入；不得搜索 BaseQuick、另一 child、装备位、Hotbar、P5/P9、WorldDrop 或尸体。

BaseQuickNoChildAtInput 模式还必须同时满足：

1. 输入时不存在 identity-valid current P17 child；这一事实必须是 intent 的明确模式值，不得在 commit 时重新猜测；
2. target scan 只能在 active P6 BaseQuick 的 ordinary storage cells 内进行：先 stable SlotIndex 升序 compatible underfull simple stack，后 stable SlotIndex 升序正式、可写、空 ordinary storage cell；
3. 输入后新打开、切换或投影出 child 不得改变目标；BaseQuick 满、stale、Owner/Run/revision 不匹配或任一 session/target/保存检查失败时零写入；不得自动装备、搜索 child 或选择其他位置。

下列对象或动作持续不属于 P46：

1. P10 Hidden/Searching source、empty target slot、P9 的任何其他 target、P11/P12 corpse、P14/P31 WorldDrop、P21 equipment、P20/P19 complete graph、P17 child item、P5 warehouse、P6 player source、Hotbar、another NormalContainer、unknown provenance 或 UI-only object；
2. player-side Ctrl + 左键、P6 → BasicCache 快捷回存、GroundDrop、quick-drop、Actor direct pickup、距离自动拾取、交互键领取、right-click Take、double-click、Take All、auto equip、auto target（除本项两种 frozen mode 内已定义的单 container scan）、auto bind、auto use、Swap、Replacement、Sort、Compact、第二 Merge/Split 路径、Quantity=N、WorldPickupDraft、PlayerSplitDraft 或 record-to-record transfer；
3. P10 normal Drag 的 Move/Merge/Swap、P40 corpse simple-stack QuickTransfer、P29 simple WorldDrop QuickTransfer、P38/P39/P41 corpse quick paths、P43—P45 ground drop、P17 graph construction、P31 Registry、P8 terminal 分类、P13 binding、P15 use 与 Code A authority；
4. 实机运行、PIE、Standalone、真实鼠标键盘、截图、Smoke、Automation、回归、Cook、Package 或最终验收。

## 下半部分：授权执行内容

### 4. 单一授权目标

在不建立第二物品真值、不改变 P9/P10 普通容器状态机、P17 graph、P29/P36/P37/P39/P40 既有 Ctrl + 左键语义、P31 Registry、P8 terminal 或 Code A authority，也不新增自动装备、自动拾取或快捷回存的前提下，将 current opened/revealed exact P10 BasicCache simple-stack root 接入共享 frozen-target QuickTransfer：

1. 只有 current opened/revealed exact P10 BasicCache simple-stack root Cell 才可建立一次 P46 QuickTransfer transient intent；
2. intent 在输入时一次性选择 CurrentP17Child 或 BaseQuickNoChildAtInput，并冻结模式所需的完整 source/target/container identity；
3. accepted path 只形成一个 P1 Merge(Quantity=0) 或一个 P1 whole-root Move、一次 P9/P6 Owner durable replacement 和一次 SaveRecord；
4. Merge 部分接收后只刷新 exact P10 source quantity 与解析得到的 P6 target projection；Move 或完整 Merge 后才刷新 source empty state。不得创建 WorldDrop、record、ordinal、Actor、child、new ItemId、new ContainerId 或第二数量真值；
5. 任一 source、mode、target、identity、capacity、session、close/stale、P1 或保存检查失败时完整零写入与 BeforeSnapshot rollback。

### 5. 实现要求

#### 5.1 先完成活动调用链与资格审计

改动前必须审阅并在 Report 中列出：

1. P10 NormalContainerTarget open/reveal lifecycle、BasicCache ordinary root Cell、normal Drag Move/Merge/Swap、page/focus invalidation、P9/P6 callback 与 source proof；说明 P46 如何复用既有 source Cell 和共享 Ctrl pointer router，而不是新增 Button、hotkey、pointer handler、Widget、Actor 或容器写入口；
2. P29、P36、P37、P39、P40 的 shared QuickTransfer transient intent、pointer consumption、input-time frozen mode、merge-first / empty-second resolver、preview、commit、cancellation 与 stale lifecycle；说明 P46 如何只增加 P10 BasicCache canonical source branch；
3. P9/P10 actual stable source container semantics、SearchTargetId、receipt/profile identity、Reveal/Open gate、ordinary source root proof，以及为什么 P9 materialization/history/determinism 未被改写；
4. P17 current child identity、parent → child canonical topology、open generation、dynamic capacity、ordinary Cell 与 stable SlotIndex，以及 P6 BaseQuick 的正式 ordinary target policy；
5. P1/P2/P3/P6/P9 的 Move 与 Merge(Quantity=0)、partial acceptance、source retain/delete、cross-graph single candidate、BeforeSnapshot rollback、P9/P6 single Owner durable replacement、P13 reconcile 与 revision/session 调用图；
6. 所有 Widget direct Move/quantity mutation、Actor direct pickup、container display-cache write、right-click Take、double-click、预建 ItemId、旧 Code A loot、按 first/last/selection 猜测 target、第二 Repository transaction、第二 save 或 Code A inventory writer；它们不得成为 P46 写入路径。

#### 5.2 共享输入、source gate 与 mode 冻结

1. UCodeBP3CellButton::NativeOnMouseButtonDown 或当前等价共享 pointer router 仍是 Ctrl + 左键唯一消费点。命中 P46 source 后只能建立一次既有 QuickTransfer transient intent 并返回 Handled；不得继续 ordinary selection、P10 search、drag threshold、P15 Use、right-click detail 或 Actor interaction。
2. 不得为 P46 新建 Button、hotkey、Actor click、专用 Widget、第二 pointer handler、second QuickTransfer resolver 或 UI direct write。resolver 必须从 canonical P10 source proof、P9 exact source address、input-time frozen mode 与 P17/P6 truth 在 P29/P36/P37/P39/P40/P46 branches 间分流。
3. intent 创建时必须先重验 exact NormalContainerTarget/SearchTargetId/DefinitionId/receipt/source-slot identity 与 P10 simple-stack qualification，再只执行以下二选一：
   - current P17 child 已 identity-valid：写入 CurrentP17Child，以及 exact parent ItemId、child ContainerId、open generation、P6 revision；
   - 不存在 current valid P17 child：写入 BaseQuickNoChildAtInput；不得留空、延迟决策或把 UI focus/selection 当成随后可变 target。
4. Preview 与 durable Commit 前必须重验 OwnerId、RunInstanceId、SearchTargetId、DefinitionId、receipt、target/open/container/composite/P6 revision、source ContainerId/SlotIndex、root ItemId、P10 BasicCache provenance、Definition/StackKey/MaxStack/Quantity/no-child qualification、Reveal/Open、route、focus、page/target-open generation、active session、Prepared/terminal gate、Host validity 与 mode-specific target proof。
5. Target close/reopen、focus loss、当前 P10 search/action 引发的 source invalidation、Actor EndPlay、map reload、recovery、terminal/Prepared、receipt/root/container mismatch、payload cancel、Host invalidation 或 source revision stale 必须立即使 intent 失效并零写入。CurrentP17Child mode 另须在 child close/switch/focus loss/open generation mismatch/parent mismatch/capacity invalidation 时失效；不得变为 BaseQuick fallback。
6. ordinary left click 继续只选择或按 P10 既有规则开始 search；right-click 继续只读详情；normal Drag 继续只走 P10 explicit Move/Merge/Swap policy。double-click、Tab、I、Esc、Close、Cancel、scroll、空白区、无 payload Drop、Shift + 1—9、P10 非 simple source、player-side Ctrl 与所有 existing WorldDrop/Corpse Ctrl branch 均不得获得 P46 位置或数量写入语义。

#### 5.3 canonical source、确定 target resolver 与数量语义

1. Preview 只接受 current exact P9 BasicCache container 的 Revealed root。Store 必须从 canonical Catalog、P1/P6/P9 snapshot、P10 target/receipt 与 source stable address 重建并全字段验证 BasicCache provenance、simple-stack definition、positive Quantity、MaxStack 大于 1、no-child/no-graph closure、Reveal/Open、source slot、record availability 与 exact ItemId。
2. P46 source 必须拒绝 Hidden/Searching slot、stack draft、space parent/child、P19 graph、P20 spatial root、P11/P12 source、P14/P31 world source、P5 warehouse、Hotbar、player source、another NormalContainer、wrong target generation 或任何 mode/source mismatch；它们均零写入。
3. CurrentP17Child mode 的唯一 target container 是 intent 中 exact child。严格按真实 stable SlotIndex 升序先扫描正式 compatible underfull simple stack；只在完全不存在 accepted preview candidate 时，才按 stable SlotIndex 升序扫描正式、可写、空 ordinary storage cell。每个 candidate 必须由 P1/P6 truth 验证 ContainerId、Owner/Run、parent/child topology、capacity、slot semantic、slot availability、open generation 与 P6 revision。
4. BaseQuickNoChildAtInput mode 的唯一 target container 是 active P6 BaseQuick。严格按真实 stable SlotIndex 升序先扫描正式 compatible underfull simple stack；只在完全不存在 accepted preview candidate 时，才按 stable SlotIndex 升序扫描正式、可写、空 ordinary storage cell。每个 candidate 必须由 P1/P6 truth 验证 ContainerId、Owner/Run、capacity、slot semantic、slot availability 与 P6 revision。
5. compatible target 只提交一次 P1 Merge(Quantity=0)。P1 是唯一可裁决完整或部分接收量的地方；partial acceptance 后 source 保持同一 ItemId、P9 container、SlotIndex、Reveal state 与剩余 Quantity，target 保持既有 ItemId。该 input 不得继续尝试另一个 compatible target 或任何 empty target。
6. empty target 只提交一次 P1 whole-root Move。仅当 accepted snapshot 明确证明 source root 已离开 exact P9 source 时，P9 exact source slot 才成为 empty；该 Move 必须保留原 ItemId、DefinitionId、Quantity、Level、Quality、RandomSeed、LegacyAffixDigest 与 BasicCache provenance，不得生成 new ItemId、ContainerId、receipt、target record、WorldDrop、ordinal、Actor 或 child。
7. 自动解析仅限上述两种 mode 的一个 ordinary storage target。不得把 Weapon、Armor、Accessory、SpatialRing、Backpack 或任何装备位当成 P46 自动 target；不得调用 Equip、Unequip、Split、Quantity=N、RequestedMergeQuantity、WorldPickupDraft、PlayerSplitDraft 或 P19 graph Move。

#### 5.4 单一事务、持久化与回滚

1. Preview 成功后只能建立一个 Owner/Run scoped Candidate。candidate 只能为：
   - 同一个 source ItemId 从 exact P9 BasicCache source 直接进入一个 existing compatible P6 stack 的一次 P1 Merge(Quantity=0)；或
   - 同一个 source ItemId 从 exact P9 BasicCache source 直接进入 frozen-mode resolved empty P6 target 的一次 P1 Move。
   不得先落 BaseQuick 再二次 Move，不得先清 source，不得用 P6 local move 绕过 P9/P6 atomic transaction。
2. Commit 必须沿既有 P10/P3 → P2 → P1 → P9/P6 durable callback。Store 在任何 durable write 前重新验证 command intent、source/target stable address、NormalContainerTarget/SearchTargetId/DefinitionId/receipt/root、Owner、Run、revision、P10 canonical eligibility、mode proof、target candidate、active session 与 lifecycle gate。
3. Merge accepted delta 只能改变 exact P9 source Quantity、exact P6 target Quantity 与必要 source deletion；不得改动无关 ItemId、ContainerId、SlotIndex、ChildContainer、P9 record、receipt、WorldDrop record、ordinal、Actor、child 或第二 revision。Move accepted delta 只能改变 exact root parent/slot。两类 candidate 都必须以 accepted P1 snapshot/replay proof 全字段相等。
4. P9 与 P6 必须在同一个 Owner durable replacement 中共同提交，函数内只有一次 SaveRecord。仅当 accepted snapshot/replay proof 成立，才刷新 exact NormalContainerTarget source 与 P6 projection；P13 只沿既有 accepted reconcile，P8 继续只结算 P6 玩家图并丢弃 P9 residual。
5. candidate、source/mode/target validation、P1 transaction、P13 reconcile、projection 前检查或 SaveRecord 任一失败时，必须完整恢复 BeforeSnapshot：P9 source、P6 target、target state/reveal/action、selection、scroll 与无关 target/world/body record 均保持旧值；不得遗留 phantom empty、phantom pickup、transient item copy、duplicate quantity 或 second truth。

#### 5.5 非回归、终局与权威边界

1. P10 existing normal Drag 的 Move/Merge/Swap 保持完全不变。P46 只在 Ctrl gesture 下从 exact BasicCache simple-stack Cell 进入 shared resolver；normal Drag 不得触发 P46 intent。
2. P40 corpse simple-stack Ctrl、P29 WorldDrop simple Ctrl、P36/P37/P39 standard equipment Ctrl、P41 complete-graph corpse Ctrl 与 P30 complete-graph WorldDrop Ctrl 保持各自 source family 与冻结目标规则，不得被 P46 宽松分类。
3. P43—P45 corpse GroundDrop、P26—P28 normal/quantity world behavior、P31 Registry、P17 graph、P8 terminal/recovery、P13 binding、P15 use、P9/P10 deterministic materialization/reveal、P5/P6 bridge 与 Code A authority均保持既有产品语义。
4. Code A 只可维持既有 production NormalContainerTarget lifecycle forwarding和 UI/map 边缘；不得拥有 Item、Container、Quantity、P9/P6、search、terminal 或 player-inventory durable authority。

### 6. 允许的改动范围

仅允许在当前 Code B 中最小修改：

- P10 已有 NormalContainer source proof、shared Ctrl pointer router、QuickTransfer preview/commit、P9/P6 durable transaction 与 projection，仅用于 P46 exact BasicCache source；
- P29/P40 所在 shared simple-stack QuickTransfer source classification、frozen target proof、resolver、partial acceptance、rollback 与 cancellation，仅用于 P46；
- P1/P2/P3/P6/P9/P10/P13/P17 的必要声明、candidate proof、replay、rollback 或调用签名兼容，前提是不改变已有产品语义；
- 必要的 production NormalContainer lifecycle forwarding，只能作为 Code B gate 的只读转发；Code A 不得取得库存、Loot、数量、P9/P6 或 durable authority；
- PROJECT.md、PROJECT_INFO_CARD.md、本任务 Prompt 归档与本任务 Report。

禁止新建项目、版本线、Fix、第二 P9/P6、Widget inventory、Code A mirror、fixture、假 ItemId、clone、双写、存档重置或历史数据改写。禁止修改 Code A 功能逻辑、P9/P10 materialization/search state machine、P17 graph construction、P31 schema/registry、P8 receipt/terminal product logic、P5/P6 bridge 或任何测试文件。

### 7. 明确不在本任务内

- 不把 P10 BasicCache 以外的 NormalContainer、P11/P12 corpse、P14/P31 WorldDrop、P20/P19 spatial graph、P21 equipment、P17 child item、P5 warehouse、P6 player source、Hotbar、other provenance 或 existing WorldDrop root 接入 P46 source。
- 不实现 player → BasicCache Ctrl 回存、BasicCache → GroundDrop、quick-drop、Actor direct pickup、自动拾取、Take All、right-click Take、double-click、auto target、auto equipment、auto child open/switch、space parent/child 地面操作、world multi-item container、record-to-record transfer、cross-record merge、Swap、Replacement、Sort、Compact、Bind、Use、装备数值、战斗效果、HUD 接管、网络或多人。
- 不改变 P10 normal Drag Move/Merge/Swap、P29/P30、P38—P45、P26—P28、P31、P5/P6/P8、搜索、尸体、敌人、地图、战斗、生命、死亡、撤离、商店、经济或制作。
- 不启动产品、PIE、Standalone、真实鼠标键盘输入、截图、Smoke、Automation、回归、试玩、Cook、Package 或最终验收。
- 不在回传 Report 前自动开始 P47、任意 Fix 或 F。

### 8. P 阶段静态审查与编译

完成后只执行以下检查：

1. 审查 P46 唯一合法 source 为 current opened/revealed/exact P10 BasicCache simple stack；Hidden/Searching、P11/P12、P14/P31、space/equipment/child、P5/P6、other NormalContainer 与 unknown family 均不能进入 P46。
2. 审查 Ctrl + 左键只在既有 shared pointer router 中形成一个 transient P46 intent；确认无新 pointer、Widget、Actor、quantity draft、GroundDrop、right-click、double-click 或 Code A 写入旁路。
3. 审查 input-time CurrentP17Child / BaseQuickNoChildAtInput 模式冻结、merge-first / empty-second stable SlotIndex 顺序、child stale 的 no-fallback、partial Merge source retain 与 one-candidate policy。
4. 审查 accepted path 只包含一个 P1 Move 或 Merge(Quantity=0)、一个 P9/P6 Owner durable replacement、一次 SaveRecord 与 accepted-only projection；确认没有 P9 source staging、Split、new ItemId/ContainerId、second truth、second save 或 Actor-first write。
5. 审查 P10 normal Drag、P40/P29/P30/P36/P37/P39/P41 Ctrl、P43—P45、P17/P31、P8/P13/P15、P9/P10 state/determinism 与 Code A authority均无产品语义回归。
6. 执行 git diff --check。
7. 编译 Editor：

       "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_mapEditor Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload

8. 编译 Game：

       "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_map Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload

若编译失败，只修复本任务引入的 P10 source proof、shared QuickTransfer routing、P9/P6 transaction、projection、rollback、include 或签名问题；若必须扩大到本任务以外，停止受影响部分并如实报告。

### 9. Report 与完成信号

生成 Dev.D.UE.0.0.9B.P46.0.r0_report.md，保存至：

    C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Docs\Report

Report 必须简洁、可审计地列出：

1. 本轮新增、修改、未修改的每个文件及职责；
2. P10 BasicCache source Cell 到 P46 shared Ctrl router、Preview、Commit、P1 与 P9/P6 durable callback 的完整调用图，以及没有新增 pointer、Widget、Actor、second resolver 或 Code A 写入口的证据；
3. P9/P10 exact BasicCache identity、SearchTargetId、receipt、stable source slot、Reveal/Open lifecycle、actual source definition/stack metadata，以及为什么 P9 materialization/history/determinism 未被改写；
4. source gate、Owner/Run/revision/session proof、输入时 frozen child/base mode 与 zero-write cancellation/stale policy；
5. CurrentP17Child 和 BaseQuickNoChildAtInput 的 exact candidate scan、stable SlotIndex order、partial Merge retain、no fallback/no auto behavior 的证据；
6. single P1 transaction、single P9/P6 durable replacement、single SaveRecord、source/target identity保持、P13/P8、rollback、selection/scroll/stable SlotIndex 与无 WorldDrop/ordinal/Actor write 的证据；
7. P10 normal Drag、P40/P29/P30/P36/P37/P39/P41、P43—P45、P26—P28、P31、P5/P6/P8/P13/P15/P17 与 Code A authority 的非回归结论；
8. git diff --check 结果、两个编译命令、目标、原生 exit code 与关键结果；
9. 所有未执行的 F 阶段真实验证，至少包括：BasicCache materialize/open/reveal；SpiritDust 和 IronShard 各自 Ctrl 到 valid current child 与 no-child BaseQuick；compatible partially-full/full target、empty target、partial Merge、full BaseQuick/child；child close/switch/focus/generation/parent/capacity stale；Hidden/Searching/wrong target/wrong source；SearchTargetId/receipt/Owner/Run/P9/P6 revision stale；Target close/reopen、P10 action/search、Prepared/terminal/Host invalid/SaveRecord failure；P10 normal Move/Merge/Swap、P40/P29/P30/P36/P37/P39/P41、P43—P45、P26—P31、P8 terminal/recovery；真实鼠标键盘、PIE、Standalone、截图、Smoke、Automation、回归、Cook 与 Package。

仅当 P46 BasicCache simple-stack Ctrl QuickTransfer 静态闭合、P9/P10、P17、P29/P40、P31、P8 与 Code A 边界保持，且 Editor 与 Game 均以 native exit code 0 完成时，使用：

    READY_FOR_P47_PLANNING

若当前范围内仍有可修复问题，使用：

    NEEDS_P46_REWORK

若现有 P9/P10/P6 transaction 或 shared Ctrl resolver 无法在不创建第二输入路径、第二保存、第二物品真值、自动装备、自动拾取、改写 P9 materialization/history 或扩展 Code A authority的前提下支持本项，使用：

    NEEDS_PLANNER_DECISION
    BLOCKED

完成后不得自动开始 P47、Fix 或 F。向策划 Chat 回传并附带且只附带本次同名 Report；正文首行使用：

    [CSEMI:REPORT_SENT] {"task_id":"Dev.D.UE.0.0.9B.P46.0.r0","file":"Dev.D.UE.0.0.9B.P46.0.r0_report.md"}
