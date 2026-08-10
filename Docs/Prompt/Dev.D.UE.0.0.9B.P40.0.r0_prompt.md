# Dev.D.UE.0.0.9B.P40.0.r0

## 任务身份

- 项目：Dev.D.UE.0.0.9B；继续使用同一活动工程，不新建项目。
- 阶段：主线 P40——为已打开、已揭示的 P12 普通尸体 simple stack 补齐受限的 Ctrl + 左键快捷拾回。
- 任务编号：Dev.D.UE.0.0.9B.P40.0.r0。
- 前置：已接受 0.0.9B.P1—P39 与 0.0.9BFix.P1—P4。Fix 只用于已确认、已验收功能的缺陷修复；P40 是新增主线功能，不是 Fix。
- 执行文件：Dev.D.UE.0.0.9B.P40.0.r0_prompt.md。
- 报告文件：Dev.D.UE.0.0.9B.P40.0.r0_report.md。
- 活动工程根：C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B。
- 活动工程：C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject。
- 任务性质：P 阶段只做实现、静态审查与代码编译。不得启动产品、PIE、Standalone、真实输入验证、截图、Smoke、Automation、回归、Cook、Package 或 F 阶段测试；不得自动开始 P41、任意 Fix 或 F。

## 上半部分：只读项目裁决、现状与边界

### 1. 当前唯一有效基线

唯一有效依据是当前 0.0.9B、活动工程及已接受任务链。所有 0.2、V2、V3、I、IPF、历史页面壳、旧 CTA、旧库存和旧物品规则均已过时；不得读取、采用、恢复或以其决定实现、验收或范围。

Code B P1 Repository 与既有 durable Store 是唯一可变物品真值。每件物品始终只有一个真实 ItemId、一个真实父位置和一条权威事务链。Widget、Cell、Presenter、DragOperation、Workspace Context、BodyTarget、QuickTransfer resolver、空间 child 投影、WorldDrop Actor、地图放置适配层和 Code A 都只能持有只读投影、选择或瞬时意图；不得持有第二库存、可写数量副本、预建 ItemId、平行尸体背包、Actor-first 写入或 A/B 双写。

P11/P12 已建立唯一 BasicCorpse 的死亡回执、首次物质化、Hidden → Searching → Revealed、已打开 BodyTarget，以及 P11/P6 的单次 durable Move、Merge、Swap。P12 普通尸体栏中的 simple stack 仍是 P11 graph 内的真实 root；它不是 Widget 数量，也不是 Code A Loot。P20 的空间 parent 仍只走完整图路径；P21 的三固定尸体装备位仍是独立 source family；P38/P39 已分别补齐尸体装备的显式拖拽和冻结目标快捷拾回。

P29 已建立 simple stack 快捷拾回的正式数量语义：在一个已经解析的目标容器中，先按真实 stable SlotIndex 升序寻找 Definition/StackKey 兼容、未满的 existing stack，再按 stable SlotIndex 升序寻找合法空 ordinary storage cell。兼容 stack 使用单次 P1 Merge，Quantity=0；P1 是唯一可裁决实际完整或部分接收量的地方。若 P1 只接受一部分，source 保留同一 ItemId、同一 source slot 与剩余 Quantity；不得在同一手势中再把余量 Move 到空格。

P36、P37、P39 已把共享 Ctrl + 左键 QuickTransfer 的空间目标收敛为输入时冻结的两种模式：

1. 输入时存在 identity-valid current P17 child 时，只解析该 exact child；
2. 输入时根本不存在 valid child 时，才解析 P6 BaseQuick。

被选 child 在之后关闭、切换、失焦、失效或填满时必须拒绝，绝不静默改投 BaseQuick、另一 child、装备位、Hotbar、仓库、世界或其他位置。P40 只将这条已接受模型与 P29 的 simple-stack merge-first / empty-second 顺序结合到 P12 普通尸体 source；不改写任何已有 source family。

### 2. P40 产品裁决

当玩家已经通过 P12 主动打开 identity-valid 的唯一 BasicCorpse，且其中一个普通尸体格的 exact root 已 Revealed，玩家对该 root Cell 按下 Ctrl + 左键时，必须由既有共享 QuickTransfer resolver 处理。P40 只接受正式可堆叠、非空间、正数量的 simple stack；它不处理 P21 固定装备、空间 parent/child、普通容器、地面、仓库或玩家 source。

自动目标只在输入时决定一次：

| 输入时 source | 输入时目标模式 | 唯一允许的自动目标顺序 | 权威事务 |
| --- | --- | --- | --- |
| exact opened/revealed P12 ordinary simple-stack root | 当前存在 identity-valid P17 child | 只在 exact current child 的 ordinary storage cells 内：先 stable SlotIndex 升序兼容未满 stack，后 stable SlotIndex 升序空 ordinary cell | occupied：一个 P1 Merge(Quantity=0)；empty：一个 P1 Move |
| exact opened/revealed P12 ordinary simple-stack root | 当前不存在 identity-valid P17 child | 只在 active P6 BaseQuick 的 ordinary storage cells 内：先 stable SlotIndex 升序兼容未满 stack，后 stable SlotIndex 升序空 ordinary cell | occupied：一个 P1 Merge(Quantity=0)；empty：一个 P1 Move |

这是一条“已打开尸体窗口中的一次快捷拾回请求”，不是 Take All、自动拾取、自动装备或快捷回存尸体。每次 Ctrl + 左键只处理当前 exact source root、最多建立一个 candidate，并只提交一次 P1 transaction。若选定 merge target 的 P1 结果为部分接收，source 必须留在同一 P11 source container / SlotIndex，以 accepted Quantity 更新；不得续扫空格、不得二次 Move、不得创建 split item 或新 ItemId。

若输入时选择 CurrentP17Child，intent 必须冻结 exact child ContainerId、空间 parent ItemId、child open generation、Owner/Run、BodyTarget/death receipt/source slot、P11 body record revision、P6 composite revision 与必要 route/focus/session proof。commit 前 child 关闭、切换、失焦、generation 失效、parent/child topology 失效、target stale 或没有合法 candidate 时一律拒绝；不得转为 BaseQuick。

只有输入时根本不存在 identity-valid current P17 child，才可建立 BaseQuickNoChildAtInput 模式。输入后打开、切换或投影出 child 不得改变该模式。BaseQuick 满、source/BodyTarget stale、Owner/Run/P6 revision 不匹配或任一 session、保存或 lifecycle 检查失败时必须零写入。

### 3. 严格范围与持续排除

P40 source 只接受同时满足全部条件的 root：

1. 当前 Workspace 是既有 P12 production Host；exact BodyTarget 已 Open、已 Revealed、identity-valid，且无 active action/search。OwnerId、RunInstanceId、BodyTargetId、DeathReceiptId、body record revision、P6 composite revision、route、focus、target-open/page generation 与 active session 都能在 Preview 和 durable Commit 前从 P1/P6/P11 truth 重新验证；
2. source 位于 exact P11 BasicCorpse 的 ordinary body-storage stable address，且不属于 P21 的 Body.Weapon、Body.ArmorRobe 或 Body.Accessory0 固定装备容器。P11 receipt/profile、BodyTarget projection、Catalog、P1 snapshot 与 source container/SlotIndex 必须共同证明该 P12 ordinary provenance；不得以显示名、图标、Cell class、Actor、尸体展示顺序、当前选择或 UI cache 判断；
3. root 是正式 Definition/StackKey-compatible simple stack：bStackable、Quantity > 0、MaxStack > 1、无 ChildContainerId、无 spatial semantic、无 graph closure、不是装备 root、不是 Hotbar reference、不是 draft，且 source container slot 反向指向同一 ItemId；
4. frozen mode、Owner/Run、P6/body revision、active session、Prepared/terminal gate、Host validity、source Reveal/Open 状态及全部 mode-specific identity 在 durable commit 前仍完整成立。

CurrentP17Child 模式还必须同时满足：

1. 输入时 Workspace 已有一个 current、opened、identity-valid 的 P17 child；intent 必须携带 exact formal space parent ItemId、child ContainerId、active child open generation 与 source P6 revision；
2. durable Store 从 P1/P6 重建唯一 active P17 parent → child 映射，验证 parent 位于正式 SpatialRing 或 Backpack placement、空间定义、dynamic ChildContainerCapacity、一层无环规则、child ordinary slot semantic 与 active session；不得由显示顺序、焦点或 UI cache 推断；
3. target scan 只能在该 exact child 内进行：先真实 stable SlotIndex 升序的 compatible underfull simple stack，再真实 stable SlotIndex 升序的正式可写空 ordinary slot。child 不存在、关闭、切换、失焦、generation 不匹配、parent mismatch、capacity/slot semantic 失效、无合法 candidate 或 revision stale 时均零写入；
4. intent 不得改为 BaseQuick mode，不得搜索另一 child、另一空间 parent、装备位、Hotbar、P5/P9/P11、WorldDrop 或另一 BodyTarget。

BaseQuickNoChildAtInput 模式还必须同时满足：

1. 输入时不存在 identity-valid current P17 child；这一事实必须是 intent 的明确模式值，而非 commit 时重新猜测；
2. target scan 只能在 active P6 BaseQuick / Basic 根容器内进行：先真实 stable SlotIndex 升序的 compatible underfull simple stack，再真实 stable SlotIndex 升序的正式可写空 ordinary slot；
3. 输入后新打开、切换或投影出 child 不得改变目标；BaseQuick 满、stale、Owner/Run/revision 不匹配或任一 session/body/保存检查失败时零写入；不得自动装备、搜索 child 或选择其他位置。

下列对象或动作持续不属于 P40：

1. P21 fixed equipment root、P20/P19 complete graph、任何带 ChildContainerId 的 parent 或 child item、P12 Hidden/Searching root、empty body slot、P9 ordinary container、P14/P31 WorldDrop、P5 warehouse、P6 player source、Hotbar、another BodyTarget、unknown provenance 或 UI-only object；
2. player-side Ctrl + 左键、quick-drop、P6 → corpse 快捷回存、Actor direct pickup、距离自动拾取、交互键领取、right-click Take、double-click、Take All、auto equip、auto target（除本项两种 frozen mode 内已定义的单容器 scan）、auto bind、auto use、Swap、Replacement、Sort、Compact、Merge/Split 的第二路径、Quantity=N、WorldPickupDraft、PlayerSplitDraft 或 record-to-record transfer；
3. P12 普通真实 Drag 的 Move/Merge/Swap、P20 完整图尸体取得、P21 deterministic source/history、P38 normal Drag、P39 corpse-equipment QuickTransfer、P17 graph construction、P31 Registry、P8 terminal 分类、P13 binding、P15 use 与 Code A authority；
4. 实机运行、PIE、Standalone、真实鼠标键盘、截图、Smoke、Automation、回归、Cook、Package 或最终验收。

## 下半部分：授权执行内容

### 4. 单一授权目标

在不建立第二物品真值、不改变 P11/P12 尸体状态机、P20/P21 来源、P17 graph、P31 Registry、P29/P36/P37/P39 既有 Ctrl + 左键语义，也不新增自动装备或快捷回存尸体的前提下，将 current opened/revealed exact P12 ordinary simple-stack root 接入共享 frozen-target QuickTransfer：

1. 只有 current opened/revealed exact P12 ordinary simple-stack root Cell 才可建立一次 P40 QuickTransfer transient intent；
2. intent 在输入时一次性选择 CurrentP17Child 或 BaseQuickNoChildAtInput，并冻结模式所需的完整 source/body/target identity；
3. accepted path 只形成一个 P1 Merge(Quantity=0) 或一个 P1 whole-root Move、一次 P11/P6 Owner durable replacement 和一次 SaveRecord；
4. Merge 部分接收后只刷新 exact BodyTarget source quantity 与解析得到的 P6 target projection；Move 或完整 Merge 后才刷新 source empty state。不得创建 WorldDrop、record、ordinal、Actor、child、new ItemId 或第二数量真值；
5. 任一 source、mode、target、identity、capacity、session、close/stale、P1 或保存检查失败时完整零写入与 BeforeSnapshot rollback。

### 5. 实现要求

#### 5.1 先完成活动调用链与资格审计

改动前必须审阅并在 Report 中列出：

1. P12 的 BodyTarget open/reveal lifecycle、ordinary body root Cell、normal Drag Move/Merge/Swap、page/focus invalidation、P11/P6 callback 与 source proof；说明 P40 如何复用既有 source Cell 和共享 Ctrl pointer router，而不是新增 Button、hotkey、pointer handler、Widget、Actor 或尸体写入口；
2. P29、P36、P37、P39 的 shared QuickTransfer transient intent、pointer consumption、input-time frozen mode、merge-first / empty-second resolver、preview、commit、cancellation 与 stale lifecycle；说明 P40 如何只增加 P12 ordinary simple-stack canonical source branch；
3. P11/P12 的 actual stable ordinary source container semantics、profile/receipt/body record identity、Reveal/Open gate、ordinary source root proof，以及为何 P20/P21 deterministic history 未被改写；
4. P17 current child identity、parent → child canonical topology、open generation、dynamic capacity、ordinary Cell 与 stable SlotIndex，以及 P6 BaseQuick 的正式 ordinary target policy；
5. P1/P2/P3/P6/P11 的 Move 与 Merge(Quantity=0)、partial acceptance、source retain/delete、cross-graph single candidate、BeforeSnapshot rollback、P11/P6 single Owner durable replacement、P13 reconcile 与 revision/session 调用图；
6. 所有 Widget direct Move/quantity mutation、Actor direct pickup、尸体展示缓存写入、right-click Take、double-click、预建 ItemId、旧 Code A loot、按 first/last/selection 猜测 target、第二 Repository transaction、第二 save 或 Code A inventory writer；它们不得成为 P40 写入路径。

#### 5.2 共享输入、source gate 与 mode 冻结

1. UCodeBP3CellButton::NativeOnMouseButtonDown 或当前等价共享 pointer router 仍是 Ctrl + 左键唯一消费点。命中 P40 source 后只能建立一次既有 QuickTransfer transient intent 并返回 Handled；不得继续 ordinary selection、body search、drag threshold、P15 Use、right-click detail 或 Actor interaction。
2. 不得为 P40 新建 Button、hotkey、Actor click、专用 Widget、第二 pointer handler、second QuickTransfer resolver 或 UI direct write。resolver 必须从 canonical P12 source proof、P11 exact source address、input-time frozen mode 与 P17/P6 truth 在 P29/P36/P37/P39/P40 branches 间分流。
3. intent 创建时必须先重验 exact BodyTarget/death receipt/source-slot identity 与 P12 ordinary simple-stack qualification，再只执行以下二选一：
   - current P17 child 已 identity-valid：写入 CurrentP17Child，以及 exact parent ItemId、child ContainerId、open generation、P6 revision；
   - 不存在 current valid P17 child：写入 BaseQuickNoChildAtInput；不得留空、延迟决策或把 UI focus/selection 当成随后可变 target。
4. Preview 与 durable Commit 前必须重验 OwnerId、RunInstanceId、BodyTargetId、DeathReceiptId、body record revision、source ContainerId/SlotIndex、root ItemId、P12 ordinary provenance、Definition/StackKey/MaxStack/Quantity/no-child qualification、Reveal/Open、route、focus、page/target-open generation、P6 revision、active session、Prepared/terminal gate、Host validity 与 mode-specific target proof。
5. BodyTarget close/reopen、focus loss、开始/取消 search、Actor EndPlay、map reload、recovery、terminal/Prepared、receipt/root/container mismatch、payload cancel、Host invalidation 或 source revision stale 必须立即使 intent 失效并零写入。CurrentP17Child mode 另须在 child close/switch/focus loss/open generation mismatch/parent mismatch/capacity invalidation 时失效；不得变为 BaseQuick fallback。
6. ordinary left click 继续只选择或按 P12 既有规则开始 search；right-click 继续只读详情；normal Drag 继续只走 P12 explicit Move/Merge/Swap policy。double-click、Tab、I、Esc、Close、Cancel、scroll、空白区、无 payload Drop、Shift + 1—9、P12 spatial root、P21 equipment、player-side Ctrl 与所有 existing WorldDrop Ctrl branch 均不得获得 P40 位置或数量写入语义。

#### 5.3 canonical source、确定 target resolver 与数量语义

1. Preview 只接受 current exact P11 ordinary body-storage container 的 Revealed root。Store 必须从 canonical Catalog、P1/P6/P11 snapshot、P12 body record/receipt 与 source stable address 重建并全字段验证 ordinary provenance、simple-stack definition、positive Quantity、MaxStack > 1、no-child/no-graph closure、Reveal/Open、source slot、record availability 与 exact ItemId。
2. P40 source 必须拒绝 P21 equipment slot、stack draft、space parent/child、P19 graph、P20 spatial root、P12 Hidden/Searching item、P5/P9/P14/P31、warehouse、Hotbar、player source、another BodyTarget、unknown family、wrong BodyTarget generation 或任何 mode/source mismatch；它们均零写入。
3. CurrentP17Child mode 的唯一 target container 是 intent 中 exact child。严格按真实 stable SlotIndex 升序先扫描正式 compatible underfull simple stack；只在完全不存在该类 accepted preview candidate 时，才按 stable SlotIndex 升序扫描正式、可写、空 ordinary storage cell。每个 candidate 必须由 P1/P6 truth 验证 ContainerId、Owner/Run、parent/child topology、capacity、slot semantic、slot availability、open generation 与 P6 revision。
4. BaseQuickNoChildAtInput mode 的唯一 target container 是 active P6 BaseQuick。严格按真实 stable SlotIndex 升序先扫描正式 compatible underfull simple stack；只在完全不存在该类 accepted preview candidate 时，才按 stable SlotIndex 升序扫描正式、可写、空 ordinary storage cell。每个 candidate 必须由 P1/P6 truth 验证 ContainerId、Owner/Run、capacity、slot semantic、slot availability 与 P6 revision。
5. 兼容 target 只提交一次 P1 Merge(Quantity=0)。P1 是唯一可裁决完整或部分接收量的地方；partial acceptance 后 source 保持同一 ItemId、P11 container、SlotIndex、Reveal 状态与剩余 Quantity，target 保持其既有 ItemId。该 input 不得继续尝试另一个 compatible target 或任何 empty target。
6. 空 target 只提交一次 P1 whole-root Move。仅当 accepted snapshot 明确证明 source root 已离开 exact P11 source 时，P11 exact source slot 才成为 empty；该 Move 必须保留原 ItemId、DefinitionId、Quantity、Level、Quality、RandomSeed、LegacyAffixDigest 与 ordinary provenance，不得生成 new ItemId、ContainerId、receipt、body record、WorldDrop、ordinal、Actor 或 child。
7. 自动解析仅限上述两种 mode 的一个 ordinary storage target。不得把 Weapon、Armor、Accessory、SpatialRing、Backpack 或任何装备位当成 P40 自动 target；不得调用 Equip、Unequip、Split、Quantity=N、RequestedMergeQuantity、WorldPickupDraft、PlayerSplitDraft 或 P19 graph Move。

#### 5.4 单一事务、持久化与回滚

1. Preview 成功后只能建立一个 Owner/Run scoped Candidate。candidate 只能为：
   - 同一个 source ItemId 从 exact P11 ordinary body source 直接进入一个 existing compatible P6 stack 的一次 P1 Merge(Quantity=0)；或
   - 同一个 source ItemId 从 exact P11 ordinary body source 直接进入 frozen-mode resolved empty P6 target 的一次 P1 Move。
   不得先落 BaseQuick 再二次 Move，不得先清 source，不得用 P6 local move 绕过 P11/P6 atomic transaction。
2. Commit 必须沿既有 P12/P3 → P2 → P1 → P11/P6 durable callback。Store 在任何 durable write 前重新验证 command intent、source/target stable address、BodyTarget/death receipt/root、Owner、Run、revision、P12 canonical eligibility、mode proof、target candidate、active session 与 lifecycle gate。
3. Merge accepted delta 只能改变 exact P11 source Quantity、exact P6 target Quantity 与必要来源删除；不得改动无关 ItemId、ContainerId、SlotIndex、ChildContainer、P11 record、receipt、WorldDrop record、ordinal、Actor、child 或第二 revision。Move accepted delta 只能改变 exact root parent/slot。两类 candidate 都必须以 accepted P1 snapshot/replay proof 全字段相等。
4. P11 与 P6 必须在同一 Owner durable replacement 中共同提交；P13 只按既有 accepted commit reconcile，不得自动 Bind、Use、Equip 或复制 binding。P8 仍只结算当时 P6 player graph，并只丢弃 P11 residual；P40 不改变尸体死亡、reveal、receipt、终局或 recovery。
5. only after accepted snapshot/replay proof 明确证明 P1 result 正确，才可在同一 Owner candidate 中刷新 exact BodyTarget source section、解析得到的 P6 target 与必要 P7 child projection。不得预减 source quantity、预清 source、预写 P6、预删 BodyTarget、预刷 Code A Actor 或在 save 后补写另一侧。
6. candidate、canonical gate、mode resolver、target scan、P1 Merge/Move、P13 reconcile、projection 前检查或 SaveRecord 任一失败时，必须完整恢复 BeforeSnapshot：P11 source ItemId/Quantity/slot、P6 target、BodyTarget visibility/open state、P7 projection、selection 与无关 body/world record 均保持未变，且不得遗留 phantom empty slot、phantom pickup、数量副本或 transient item copy。
7. 成功后只刷新 exact BodyTarget source section、解析得到的 BaseQuick/child target 和必要关联 child projection；不得 Sort、Compact、重排无关 SlotIndex、重建无关空间区域、清空无关选择、改变无关 scroll offset 或改写 P29/P36/P37/P38/P39 语义。

#### 5.5 非回归、终局与权威边界

1. P12 ordinary normal Drag 仍保留其 explicit Move/Merge/Swap 产品语义；P40 只添加 P12 ordinary simple-stack source Ctrl quick pickup，绝不增加 player → corpse Ctrl quick return、auto target 到尸体或 P12 body-source shortcut。
2. P20/P19 complete graph 仍只按其已有完整图路径处理；P21/P38/P39 corpse-equipment source、P14/P31 WorldDrop、P29 simple-stack ground route、P30 complete graph ground route、P34/P36/P37 standard ground equipment routes均保持既有 provenance、record、target、quantity 与 lifecycle semantics。P40 不得让 P12 source 进入任何这些 branch，也不得让这些 source 进入 P40 durable branch。
3. P21 r1/r2、已 materialized records、r3 deterministic identity、Optional.EquippedLoadout、candidate/weight/digest、三个固定尸体装备位、Hidden/Searching/Reveal 与 P6 → corpse equipment 拒绝均保持。P40 绝不重掷、补料、迁移或改写尸体来源。
4. P17 one-layer graph、P31 Registry、P8 BuildP14PlayerOnlySession / 等价 finalization、P5/P6 bridge、P13 binding、P15 use、P20/P21 body source 与 Code A authority 均不改变。P40 不创建 WorldDrop，不读写 NextWorldDropOrdinal，不影响 Actor projection、地图落点或 other-record isolation。
5. Code A 继续只拥有 corpse Actor、交互距离、地图、死亡、战斗、Run 与终局边缘。它不得获得 Item、Container、Quantity、P11/P6、BodyTarget、Loot、search、terminal 或 player-equipment durable authority。

### 6. 允许的改动范围

仅允许在当前 Code B 中最小修改：

- P12 已有 body-aware source proof、shared pointer routing、QuickTransfer preview/commit、P11/P6 durable transaction 与 projection；
- P29/P36/P37/P39 的既有 frozen-target resolver、transient intent、simple-stack candidate scan、current child/BaseQuick validation、rollback 与 branch discrimination，仅用于 P40 P12 ordinary source；
- P1/P2/P3/P6/P11/P13/P17 的必要声明、candidate proof、rollback 或调用签名兼容，前提是不改变已有产品语义；
- 必要的 production BodyTarget lifecycle forwarding，只能作为 Code B gate 的只读转发；
- PROJECT.md、PROJECT_INFO_CARD.md、本任务 Prompt 归档与本任务 Report。

禁止新建项目、版本线、Fix、第二 P11/P6、Widget inventory、Code A mirror、fixture、假 ItemId、clone、双写、存档重置或历史数据改写。禁止修改 Code A 功能逻辑、P12 search state machine、P20/P21 deterministic source/history、P17 graph construction、P31 schema/registry、P8 receipt/terminal product logic、P5/P6 bridge 或任何测试文件。

### 7. 明确不在本任务内

- 不把 P21 equipment、P20 spatial root、P19 graph、P14/P31 WorldDrop、warehouse、ordinary container、player source 或任何 other provenance 接入 P40 QuickTransfer。
- 不实现快捷丢弃、P6 → corpse quick return、自动装备、equipment target scan、auto child open/switch、fallback、Take All、right-click Take、double-click、Actor direct pickup、距离自动拾取、交互键领取、Swap、Replacement、Sort、Compact、bind、use、装备数值、战斗效果、HUD 接管、网络或多人。
- 不改变 P12 normal Drag、P20/P21 source/history、P29/P30/P34/P36/P37/P38/P39 现有 Ctrl semantics、P13/P15、P17/P19、P26—P28、P31、P5/P6/P8、搜索、敌人、地图、战斗、生命、死亡、撤离、商店、经济或制作。
- 不启动产品、PIE、Standalone、真实鼠标键盘输入、截图、Smoke、Automation、回归、试玩、Cook、Package 或最终验收。
- 不在回传 Report 前自动开始 P41、任意 Fix 或 F。

### 8. P 阶段静态审查与编译

完成后只执行以下检查：

1. 审查 P40 复用唯一 Ctrl + 左键 pointer router 与既有 QuickTransfer intent；确认没有专用 input/UI/Actor 写入旁路，且分支只依据 canonical P12 ordinary source truth 与 exact BodyTarget identity。
2. 审查 source 只接受 current opened/revealed/exact P12 ordinary simple stack，并逐项复核 Owner/Run/BodyTarget/death receipt/body revision/source container-slot/root/page/focus/P6 revision；P21 equipment、P20 space、Hidden/Searching、stack draft、world、stale/close/terminal 均零写入。
3. 审查 input-time frozen target mode：valid current child 时只按 stable SlotIndex 在 exact child 内 merge-first / empty-second；输入时无 child 才只按 stable SlotIndex 在 BaseQuick 内 merge-first / empty-second。确认 child 失效/满位不回退，BaseQuick mode 输入后不改 child，且无自动装备/装备位/Hotbar/P5/P9/P11 fallback。
4. 审查 merge candidate 只使用一个 P1 Merge(Quantity=0)，partial acceptance 时 source 同 ItemId/slot retained 且不继续转移；empty candidate 只使用一个 P1 whole-root Move。确认没有 SplitDraft、Quantity=N、new ItemId、Container、receipt、record、ordinal、Actor、child 或第二物品真值。
5. 审查 P11/P6 仅以一个 Owner durable replacement 和一次 SaveRecord 共同提交；确认 P11 source 只在 accepted proof 后更新、P13 reconcile、BeforeSnapshot rollback、P8 terminal/recovery、selection/scroll/stable SlotIndex 与 P17 graph 均保持。
6. 审查 P12 normal Drag、P20/P21 source/history、P29/P30/P34/P36/P37/P38/P39 Ctrl branches、P31 Registry、P5/P6/P8/P13/P15 与 Code A authority 均无产品语义回归。
7. 审查 right-click、double-click、ordinary Drag、Actor interaction、Shift + 1—9、player-side Ctrl 和 P21 P6 → corpse drag 均未获得 P40 隐式位置或数量写入语义。
8. 编译 Editor：

       "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_mapEditor Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload

9. 编译 Game：

       "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_map Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload

若编译失败，只修复本任务引入的 P12 ordinary QuickTransfer routing、source proof、frozen target resolver、P11/P6 transaction、projection、rollback、include 或签名问题；若必须扩大到本任务以外，停止受影响部分并如实报告。

### 9. Report 与完成信号

生成 Dev.D.UE.0.0.9B.P40.0.r0_report.md，保存至：

    C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Docs\Report

Report 必须简洁、可审计地列出：

1. 本轮新增、修改、未修改的每个文件及职责；
2. P12 ordinary source Cell 到 P40 shared QuickTransfer 的完整调用图，以及没有新增 pointer、Widget、Actor、second resolver 或 Code A 写入口的证据；
3. actual stable ordinary source provenance/slot semantic、BodyTarget/death receipt/profile identity，以及为什么 P20/P21/history/determinism 未被改写；
4. exact source gate、Reveal/Open lifecycle、Owner/Run/revision/session proof、input-time frozen mode 与 zero-write cancellation/stale policy；
5. CurrentP17Child 与 BaseQuickNoChildAtInput 两条 accepted path 的真实 SlotIndex 次序、merge-first / empty-second、capacity validation、partial retain/full Move、无 mode switch/fallback/auto equip/equipment scan 的证据；
6. single P1 Merge(Quantity=0) 或 single P1 Move、single P11/P6 durable replacement、source/target identity 与 quantity 保持、P13/P8、rollback、selection/scroll/stable SlotIndex 与无 WorldDrop/ordinal/Actor write 的证据；
7. P12 normal Drag、P20/P21 source/history、P17/P19/P26—P39/P31、P5/P6/P8/P13/P15、Code A authority 的非回归结论；
8. 两个编译命令、目标、原生 exit code 与关键结果；
9. 所有未执行的 F 阶段真实验证，至少包括：P12/r3 future corpse materialize/reveal；IronShard、SpiritDust 或实际 canonical ordinary simple stack 的 Ctrl 拾回至当前 opened valid child 与输入时无 child 的首个合法 BaseQuick target；merge-first、empty-second、partial acceptance、full acceptance、MaxStack、child/BaseQuick full；child closed/switched/focus/generation/parent/capacity stale；wrong/space/equipment/Hidden/Searching source；BodyTarget/death receipt/Owner/Run/P6 revision stale；Prepared/terminal/Host invalid/SaveRecord failure；P12 normal Move/Merge/Swap、P20/P21/P38/P39、P29/P30/P34/P36/P37 Ctrl branches、P31 other-record isolation、P8 terminal/recovery、真实鼠标键盘、PIE、Standalone、截图、Smoke、Automation、回归、Cook 与 Package。

仅当 P40 P12 ordinary simple-stack frozen-target QuickTransfer 静态闭合、P12/P20/P21 与 P17/P19/P26—P39/P31/P8 边界保持，且 Editor 与 Game 均以 native exit code 0 完成时，使用：

    READY_FOR_P41_PLANNING

若当前范围内仍有可修复问题，使用：

    NEEDS_P40_REWORK

若现有 P12/P11/P6 transaction 或 shared QuickTransfer resolver 无法在不创建第二输入路径、第二保存、第二物品真值、自动装备、改写 P12/P20/P21 deterministic history 或扩展 Code A 权威的前提下支持本项，使用：

    NEEDS_PLANNER_DECISION
    BLOCKED

完成后不得自动开始 P41、Fix 或 F。向策划 Chat 回传并附带且只附带本次同名 Report；正文首行使用：

    [CSEMI:REPORT_SENT] {"task_id":"Dev.D.UE.0.0.9B.P40.0.r0","file":"Dev.D.UE.0.0.9B.P40.0.r0_report.md"}
