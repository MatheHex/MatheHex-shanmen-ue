# Dev.D.UE.0.0.9B.P36.0.r0

## 任务身份

- 项目：`Dev.D.UE.0.0.9B`；继续使用同一活动工程，不新建项目。
- 阶段：主线 P36——P35 child-standard-equipment `WorldDrop` 的受限 `Ctrl + 左键`快捷拾回；已打开空间子容器优先。
- 任务编号：`Dev.D.UE.0.0.9B.P36.0.r0`。
- 前置：已接受 `0.0.9B.P1—P35` 与 `0.0.9BFix.P1—P4`。Fix 仅用于已确认、已验收功能的缺陷修复；P36 是新增主线功能，不是 Fix。
- 执行文件：`Dev.D.UE.0.0.9B.P36.0.r0_prompt.md`。
- 报告文件：`Dev.D.UE.0.0.9B.P36.0.r0_report.md`。
- 活动工程根：`C:\\AIDev\\shanmen-ue\\Dev.D.UE.0.0.9B`。
- 活动工程：`C:\\AIDev\\shanmen-ue\\Dev.D.UE.0.0.9B\\demo_map.uproject`。
- 任务性质：P 阶段只做实现、静态审查与代码编译。不得启动真实运行验证，也不得自动开始 P37、任意 Fix 或 F。

## 上半部分：只读项目裁决、现状与边界

### 1. 当前唯一有效基线

唯一有效依据是当前 `0.0.9B`、活动工程及已接受任务链。所有 `0.2`、`V2`、`V3`、`I`、`IPF`、历史页面壳、旧 CTA、旧库存和旧物品规则均已过时；不得读取、采用、恢复或以其决定实现、验收或范围。

Code B P1 Repository 与既有 durable Store 是唯一可变物品真值。每件物品始终只有一个真实 `ItemId`、一个真实父位置和一条权威事务链。Widget、Cell、Presenter、DragOperation、Workspace Context、WorldDropTarget、WorldDrop Actor、地图放置适配层和 Code A 都只能持有只读投影、选择或瞬时意图；不得持有第二库存、可写数量副本、预建 `ItemId`、平行世界背包、Actor-first 写入或 A/B 双写。

P17 已建立真实空间道具的一层 `ChildContainer` 图及其 P7 工作台投影；空间 parent 与 child graph 必须完整保持。P19/P30 只处理空间 parent 的完整图地面路径和受限快捷拾回。P26—P29 已处理 simple stack 在 `BaseQuick` 或当前已打开合法 P17 child 与地面之间的正常／数量／快捷路径。P31 已将地面扩展为多个彼此独立、单根的 exact `WorldDrop` record。

P32 允许已装备 standard non-spatial Weapon、Armor/ArmorRobe 和普通 Accessory root 正常落地及明确拾回；P33 将同一闭环扩展到 `BaseQuick` 中未装备的同类 root；P34 只为 P32/P33 provenance 的已打开 record 提供 `Ctrl + 左键`到首个空 `BaseQuick` 的受限快捷拾回。P35 已将当前已打开、身份有效 P17 child 内的同类 direct standard root 接入 normal Drag 地面闭环和明确拾回，并以 durable provenance `P35.AcceptedGroundDrop.ChildStandardEquipment` 区分其 accepted record；P35 明确未给该 provenance `Ctrl + 左键`写入语义。

### 2. P36 产品裁决

P36 只补齐 P35 record 的统一快捷拾回，不增加快捷丢弃：玩家已主动打开一个身份有效的 exact P35 `WorldDrop` record 后，对其唯一 root Cell 按 `Ctrl + 左键`，必须仍由既有共享 QuickTransfer resolver 处理。快捷目标由输入发生时已确认的工作台状态一次性决定：

| 输入时目标模式 | 唯一允许自动目标 | accepted 事务 |
| --- | --- | --- |
| 当前存在已打开、identity-valid 的 P17 child | 该 exact current child 内按真实 stable `SlotIndex` 升序的第一个合法、空普通储物格 | 一个 P1 whole-root `Move`。 |
| 输入时没有任何已打开、identity-valid 的 P17 child | active P6 `BaseQuick` 内按真实 stable `SlotIndex` 升序的第一个合法、空普通储物格 | 一个 P1 whole-root `Move`。 |

这是“打开窗口时优先移入当前窗口”的确定化规则：若输入时已选择 child 模式，则 child id、parent、open generation、Owner/Run 和 P6 revision 必须被 intent 冻结；commit 前 child 关闭、切换、失焦、代际失效、parent/child 拓扑失效或目标无空格时一律拒绝，**不得静默回退到 `BaseQuick`**。只有输入时根本不存在 identity-valid current child，才允许独立的 `BaseQuick` 模式。`BaseQuick` 模式也不得因输入后新打开 child 而改道。

P36 不自动装备、不扫描装备槽、不猜测历史来源、不自动打开或切换 child，不在 `BaseQuick` 与 child 之间交换、挤位、替换、排序或压缩。玩家仍可使用 P35 normal Drag 将 exact record 明确放到空 `BaseQuick`、当前 child 空普通格或空兼容 equipment slot。

### 3. 严格范围与持续排除

P36 source 只接受同时满足全部条件的 root：

1. source 是当前 Workspace 中已主动打开、已 Revealed、identity-valid 的 exact P31 `WorldDropTarget`；OwnerId、RunInstanceId、WorldDropId、Ordinal、record revision、derived world container、root ItemId、route、focus、target-open generation 与 P6 revision 都能在 Preview 和 durable Commit 前从 P1/P6/P31 真值重新验证；
2. durable record 的 accepted provenance 恰为 `P35.AcceptedGroundDrop.ChildStandardEquipment`，而不是 P29 simple stack、P19 complete graph、P32/P33 standard record、P21 corpse 或任何未知 family；
3. root 的 Catalog/Repository 定义共同证明其为已 Revealed 的 standard non-spatial Weapon、Armor/ArmorRobe 或普通 Accessory：`Quantity = 1`、`MaxStack = 1`、不可堆叠、无 `ChildContainerId`、无 spatial semantic、无 graph closure，并且 exact derived world container 的 slot 0 反向指向同一 `ItemId`；
4. intent 中的 target mode、Owner/Run、P6 revision、active session、Prepared/terminal gate、Host validity、target-open lifecycle 和全部 source 身份在 durable commit 前仍完整成立。

若 intent 选择 child 模式，target 还必须同时满足：

1. 输入时 Workspace 已有一个 current、opened、identity-valid P17 child；intent 必须携带 exact parent ItemId、child `ContainerId`、active child open generation、parent placement 与 source P6 revision；
2. Store 从 P1/P6 重新建立唯一 active P17 parent → child 映射，验证 parent 正式装备位置、空间定义、dynamic `ChildContainerCapacity`、一层无环规则、child 的普通 slot semantic 与 active session；不得由显示名、图标、Cell class、当前选择或容器显示顺序推断；
3. target 只能是该 exact child 内按真实 stable `SlotIndex` 升序找到的第一个正式、可写、空普通格。child 不存在、关闭、切换、失焦、generation 不匹配、parent mismatch、capacity/slot semantic 失效、无空格或 revision stale 时均零写入；
4. 不得把 child-mode intent 改为 `BaseQuick` mode，不得搜索另一 child、另一空间 parent、Hotbar、仓库、容器、尸体、装备位、世界容器或另一 WorldDrop。

若 intent 选择 `BaseQuick` 模式，target 还必须同时满足：

1. 输入时不存在 identity-valid current P17 child；该“无 child”状态须作为 intent 的明确模式值而非 commit 时临时猜测；
2. target 只能是 active P6 `BaseQuick`/Basic 根容器内，按真实 stable `SlotIndex` 升序的第一个正式、可写、空普通储物格；
3. P36 commit 前新打开、切换或投影出 P17 child 不得改变 target mode；`BaseQuick` 满、stale、Owner/Run/revision 不匹配或任何 session/record 检查失败时零写入；不得自动装备、搜索 child 或选择其他位置。

以下对象或动作持续不属于 P36：

1. player-side standard root 的 `Ctrl + 左键`、quick-drop、P35 child root → 地面快捷丢弃、actor direct pickup、距离自动拾取、交互键领取、right-click Take、double-click、Take All、auto equip、auto bind、auto use、Swap、Replacement、Sort、Compact 或跨 record transfer；
2. P19/P30 的空间 parent、完整 graph、任何带 `ChildContainerId` 的 root、nested container、child graph closure、空间道具本体及其地面路径；
3. P26—P29 的 simple stack、Merge、Split、部分数量、`WorldPickupDraft`、`PlayerSplitDraft` 和 player → world compatible Merge；
4. P32/P33/P34 的 existing standard record normal Drag／`BaseQuick` QuickTransfer、P35 existing normal Drag、P5 warehouse、P9 normal container、P11 corpse、尸体装备、P21 corpse source、Hotbar、another WorldDrop、搜索、敌人、地图、战斗、死亡、撤离、经济、网络或多人；
5. 实机运行、PIE、Standalone、真实鼠标键盘输入、截图、Smoke、Automation、回归、Cook、Package 或最终验收。

## 下半部分：授权执行内容

### 4. 单一授权目标

在不建立第二物品真值、不改变 P17 graph、不改变 P31 multi-record Registry、不中断 P34/P35 normal 语义且不新增自动装备的前提下，将 current opened exact P35 standard `WorldDrop` root 接入共享 `Ctrl + 左键` QuickTransfer resolver：

1. 只有当前已打开的 exact P35 record 才可建立一次 P36 QuickTransfer transient intent；
2. intent 在输入时一次性选择 `CurrentP17Child` 或 `BaseQuickNoChildAtInput` 模式，并冻结模式所需的完整身份；
3. accepted path 只形成一个 P1 whole-root `Move`、一次 P31/P6 Owner durable replacement 和一次 `SaveRecord`；
4. 只有同一 accepted proof 确认 root 已离开该 exact derived world container 后，才删除该 exact record、空 world container 和 matching Actor projection；
5. 任一 source、mode、target、identity、capacity、session、record、close/stale 或保存检查失败时完整零写入／`BeforeSnapshot` rollback。

### 5. 实现要求

#### 5.1 先完成活动调用链与资格审计

改动前必须审阅并在 Report 中列出：

1. P4/P23/P29/P30/P34 的共享 `Ctrl + 左键` pointer consumption、QuickTransfer transient intent、preview、commit、取消与 stale 生命周期；说明 P36 如何在同一 resolver 依据 canonical provenance 与 target-mode identity 分流，而不是新建 input、Widget、Actor 或写入口；
2. P35 的 `ActiveDestinationContainerId`、active child open generation、focus/close/page destruct/projection invalidation 生命周期，以及 P17 parent → child canonical topology、dynamic capacity、ordinary Cell 与 stable `SlotIndex`；
3. P35 provenance、P31 exact record identity、WorldDropTarget root-only projection、normal Drag pickup proof 与 P32/P33/P34 provenance rejection；
4. P1/P2/P3/P7 的 whole-root `Move`、active P6 `BaseQuick` semantic、child target capacity、single candidate、BeforeSnapshot rollback、P13 reconcile 与 P6 revision 调用图；
5. P14/P31 的 target-open、Registry exact cleanup、derived world container、Actor diff、other-record isolation、recovery、terminal 和 P8 full-registry player-only filtering 调用图；
6. 所有 Widget direct Move、Actor direct pickup、child UI cache write、right-click Take、double-click、预建 ItemId/WorldDropId、按 first/last record 或 current selection 猜测目标、先删 record/Actor、第二 Repository transaction、第二保存或 Code A inventory 写入旁路；它们不得成为 P36 写入路径。

#### 5.2 共享输入、冻结 target mode 与零写入生命周期

1. `UCodeBP3CellButton::NativeOnMouseButtonDown` 或当前等价共享 pointer router 仍是 `Ctrl + 左键`唯一消费点。命中 P36 source 后只能建立一次既有 QuickTransfer transient intent 并返回 `Handled`；不得继续普通选择、drag threshold、P15 Use、right-click detail 或 Actor interaction。
2. 不得为 P36 新建 Button、hotkey、Actor click、专用 Widget、第二 pointer handler 或 UI direct write。resolver 必须只根据 canonical root definition、P31 exact record identity、P35 accepted provenance、输入时冻结的 target mode 与 P17/P6 truth 在 P29、P30、P34、P36 分支之间分流。
3. intent 创建时先复核 exact WorldDrop identity和 P35 standard-root gate，然后只执行以下二选一：
   - current P17 child 已 identity-valid：写入 `CurrentP17Child` mode，以及 exact parent ItemId、child ContainerId、open generation、P6 revision；
   - 不存在 current valid P17 child：写入 `BaseQuickNoChildAtInput` mode；不得留空、延迟决策或把 UI focus／selection 当作后续可变 target。
4. Preview 与 durable Commit 前必须重验 OwnerId、RunInstanceId、WorldDropId、Ordinal、record revision、derived container、root ItemId、route、focus、target-open generation、P6 revision、active session、Prepared/terminal gate、Host validity 与 mode-specific target proof。
5. WorldDrop close、focus loss、重新打开其他 record、Actor EndPlay、map reload、recovery、terminal/Prepared、record/root/container mismatch、payload cancel、Host invalidation 或 source revision stale 必须立即使 intent 失效并零写入。`CurrentP17Child` mode 另须在 child close、switch、focus loss、open generation mismatch、parent mismatch、capacity/projection invalidation 时失效；不得变为 BaseQuick fallback。
6. ordinary left click 继续只选择；right-click 继续只读详情；double-click、Tab、I、Esc、Close、Cancel、scroll、空白区、无 payload Drop、`Shift + 1—9`、player-side standard root 的 `Ctrl + 左键`及 P35 normal Drag 均不得获得 P36 位置写入语义。

#### 5.3 canonical source gate 与确定 target resolver

1. Preview 只接受 current opened exact P31/P35 derived world container slot 0 的 root。Store 必须从 canonical Catalog、P1、P6、P17 与 P31 snapshot 重建并全字段验证 P35 provenance、standard definition、non-spatial/no-child/no-graph closure、source slot、record availability 与 exact ItemId。
2. P36 source 必须拒绝 stack、partial draft、space parent/child、P19 graph、P32/P33/P34 record、P5/P9/P11/corpse/Hotbar/another WorldDrop、unknown family、UI-only object、hidden object 或任何 mode/record/source mismatch；它们均零写入。
3. `CurrentP17Child` mode 的唯一 target 是 intent 中 exact child 内按真实 stable `SlotIndex` 升序扫描的第一个正式、可写、空普通格。每个候选必须由 P1/P6 真值验证 ContainerId、Owner/Run、parent/child topology、capacity、slot semantic、slot availability、open generation 与 P6 revision。没有合法空格时拒绝；不得扫描 BaseQuick 或另一 child。
4. `BaseQuickNoChildAtInput` mode 的唯一 target 是 active P6 `BaseQuick` 内按真实 stable `SlotIndex` 升序扫描的第一个正式、可写、空普通格。每个候选必须由 P1/P6 真值验证 ContainerId、Owner/Run、capacity、slot semantic、slot availability 与 P6 revision。没有合法空格时拒绝；不得扫描 child、装备位、Hotbar、P5/P9/P11、世界容器、另一 record、最近空位或 fixture。
5. 自动解析仅限以上两种 mode 的一个普通储物格。不得把 Weapon、Armor、Accessory、SpatialRing、Backpack 或任何 equipment slot 当成 P36 自动目标；不得调用 Equip、Unequip、Merge、Split、Quantity=0、RequestedMergeQuantity、`WorldPickupDraft`、`PlayerSplitDraft` 或 P19 graph Move。

#### 5.4 单一 whole-root 事务、exact cleanup 与回滚

1. Preview 成功后只能建立一个 Owner/Run scoped Candidate，并只调用一次 P1 existing whole-root `Move`：同一个 ItemId 从 exact derived world container slot 0 直接移到经 mode resolver 确定的 exact empty ordinary target。
2. Commit 必须沿既有 P3 → P2 → P1 → P14/P31/P6 durable callback。Store 在任何 durable write 前重新验证 command intent、source/target stable address、record/root/ordinal、Owner、Run、revision、P35 canonical eligibility、mode proof、target empty state、Registry closure、active session 与 lifecycle gate。
3. accepted candidate 中 DefinitionId、ItemId、Quantity、Level、Quality、RandomSeed、LegacyAffixDigest、provenance 与 no-child/non-spatial资格必须完全保持；唯一合法变化是 root 的 parent／slot 从 exact derived world container 变为该 exact child 或 BaseQuick ordinary slot。不得生成新 ItemId、ContainerId、record、ordinal、Actor、child、binding 或第二 revision。
4. 只有 accepted snapshot/replay proof 明确证明 root 已离开这个 exact derived world container，才可在同一 Owner candidate 中删除匹配 `WorldDropId`／Ordinal／revision record、其空 world container 与 matching Actor projection。不得先删 record/Actor 后尝试 Move；不得删除相邻、first/last、当前选中或任何 other Registry record。
5. `NextWorldDropOrdinal` 不得写入或递增；不得创建新 WorldDrop。P13 只按已有 accepted commit reconcile，不得自动 Bind、Use、Equip 或恢复历史 binding。
6. candidate、canonical gate、mode resolver、target scan、P1 Move、P13 reconcile、Registry validation、record cleanup、Actor projection 前检查或 `SaveRecord` 任一失败时，必须完整恢复 `BeforeSnapshot`：world root、record、container、ordinal、Actor、target cell、selection 与无关 record 均保持未变，且不得遗留 phantom empty slot、phantom pickup 或 transient item copy。
7. 成功后只刷新该 exact WorldDropTarget、解析得到的 child/BaseQuick target 和 matching Actor projection；不得 Sort、Compact、重排无关 SlotIndex、重建空间区域、清空无关选择、改变无关 scroll offset 或改写 P29/P30/P32—P35 的语义。

#### 5.5 非回归、终局与权威边界

1. P29 simple-stack WorldDrop 的 `Ctrl + 左键`继续保留其 active P17 child 优先、既有 BaseQuick fallback、merge-first／empty-second 和 player → world compatible `Merge(Quantity=0)`语义；P36 不得让 standard equipment 进入 P29 分支。
2. P30/P19 complete-graph WorldDrop 的 `Ctrl + 左键`继续只将合法完整图移入首个空 BaseQuick，且 player-side complete graph Ctrl 继续拒绝；P36 不得让 P35 standard root 走 P19 graph closure。
3. P34 对 P32/P33 provenance 的 QuickTransfer、P32/P33 normal Drag 及 P35 normal Drag／explicit pickup 保持不变。P36 仅新增 P35 record → mode-resolved ordinary storage 的快捷分支；不提供 standard root quick-drop、quick-equip 或更广泛的自动转移。
4. P31 Registry 的 canonical order 仅用于序列化、只读投影和静态比较；所有 P36 写入只按 exact identity 处理。P36 不修改 migration、旧 schema 无损升级、other-record isolation、ordinal accepted-create-only 规则、record-to-record 行为或 Actor open semantics。
5. P8 `BuildP14PlayerOnlySession`／等价 finalization 必须继续遍历全 Registry，并将 remaining P14/P19/P32/P33/P35 ground root、derived world container 与全部 graph closure 从 player-only final graph 排除。P36 不重写 terminal 分类、P5 receipt、recovery 或 run replacement。
6. Code A 继续只做 floor/Actor/交互边缘投影和生命周期转发；不得拥有或复制 Item、Container、Quantity、Run、Player、Loot、搜索、终局或 WorldDrop 持久化权威，也不得自行清理/重建 Actor 以影响 Code B 真值。

### 6. 允许范围

允许以最小方式修改：

- Code B P1/P2/P3/P4/P7/P17/P23/P29/P34/P35 的既有 QuickTransfer resolver、transient command identity、active child mode proof、canonical standard-root validation、deterministic target scan、preview、commit、rollback 与 projection，仅用于 P36 accepted P35 WorldDrop quick pickup；
- P14/P31 所在的 P6 durable Store、WorldDrop record validation、whole-root accepted proof、exact record cleanup、P13 reconcile、P8 player-only filtering 与 Actor projection callback，仅用于表达 P36 的 single-root accepted transaction；
- P7/P3 UI 的 current opened WorldDrop / P17 child identity 注入和最小 Code A 边缘转发；Code A 仍只做投影与交互转发；
- 必要 include、声明、Build.cs、项目资料、本任务 Prompt 归档和本任务 Report。

### 7. 明确不在本任务内

- 不新建项目、版本线、Fix、第二 Repository、第二 P5/P6、第二 Warehouse、WorldDrop inventory、Widget inventory、fixture、假 ItemId、clone、Code A mirror、双写、存档重置或历史数据改写。
- 不扩大到 player-side standard root Ctrl、quick-drop、auto equip、equipment target scan、space parent/child 落地、child graph closure、nested container、P19 graph、stack quantity、Merge、Split、multi-item WorldDrop、cross-record transfer、multi-slot/rotation、地面自动堆叠或跨 record merge。
- 不实现 Actor direct pickup、距离自动拾取、交互键领取、Take All、right-click Take、double-click、除本任务 exact child/BaseQuick stable-slot resolver外的自动空位/目标选择、Swap、Replacement、Sort、Compact、绑定、使用、装备数值、战斗效果或 HUD 接管。
- 不改变 P5/P6 bridge、M01、P8 receipt 产品分类、P9/P10、P11/P12/P21 corpse source、P13/P15、P16、P17/P19、P20、P26—P35、搜索、敌人、地图、战斗、生命、死亡、撤离、商店、经济、制作、网络或多人。
- 不启动产品、PIE、Standalone、真实鼠标键盘输入、截图、Smoke、Automation、回归、试玩、Cook、Package 或最终验收。
- 不在回传 Report 前自动开始 P37、Fix 或 F。

### 8. P 阶段静态审查与编译

完成后只执行以下检查：

1. 审查 P36 复用唯一 `Ctrl + 左键` pointer router 与既有 QuickTransfer intent；确认没有专用 input/UI/Actor 写入旁路，且分支只依据 canonical P35 record truth、exact WorldDrop identity 与冻结 mode proof。
2. 审查 source 只接受 current opened exact P35 standard non-spatial root，并逐项复核 Owner/Run/revision/WorldDropId/Ordinal/record revision/derived container/root/focus/target-open generation；stack、space、P32/P33、external source、stale/close/terminal 均零写入。
3. 审查 child mode 只在输入时存在 exact valid current P17 child 时建立，且只扫描该 child 的 stable-slot first empty ordinary cell；child close/switch/focus loss/generation stale、no-empty 或 parent/capacity mismatch 均拒绝且不回退 BaseQuick。
4. 审查 BaseQuick mode 只在输入时不存在 valid current child 时建立，且只扫描 active P6 BaseQuick 的 stable-slot first empty ordinary cell；输入后 child 状态变化不得改道。
5. 审查 accepted candidate 只使用一个 P1 whole-root Move、一个 Owner durable replacement 和一次 SaveRecord；确认 ItemId/Definition/Quantity/provenance 保持，无 new ItemId/Container/record/ordinal/Actor/child/second truth。
6. 审查 exact record cleanup 只发生在 accepted Move proof 后；确认 other-record isolation、Registry migration、`NextWorldDropOrdinal`、Actor diff、open/close/stale/EndPlay/recovery/terminal、P8 full-registry exclusion、BeforeSnapshot rollback 与 P13 reconcile 均保持。
7. 审查 P29 simple stack、P30/P19 complete graph、P34 standard QuickTransfer、P35 normal Drag、P7、P21、P5/P6/P8/P13/P15/P17 与 Code A authority 均无产品语义回归。
8. 审查 right-click、double-click、ordinary player drag、Actor interaction、`Shift + 1—9`和 player-side `Ctrl + 左键`均未获得 P36 隐式位置写入语义。
9. 编译 Editor：

       "C:\\Program Files\\Epic Games\\UE_5.8\\Engine\\Build\\BatchFiles\\Build.bat" demo_mapEditor Win64 Development "C:\\AIDev\\shanmen-ue\\Dev.D.UE.0.0.9B\\demo_map.uproject" -WaitMutex -NoHotReload

10. 编译 Game：

       "C:\\Program Files\\Epic Games\\UE_5.8\\Engine\\Build\\BatchFiles\\Build.bat" demo_map Win64 Development "C:\\AIDev\\shanmen-ue\\Dev.D.UE.0.0.9B\\demo_map.uproject" -WaitMutex -NoHotReload

若编译失败，只修复本任务引入的 QuickTransfer routing、mode proof、P35 provenance gate、child/BaseQuick target resolver、world-store transaction、projection、rollback、include 或签名问题；若必须扩大到本任务以外，停止受影响部分并如实报告。

### 9. Report 与完成信号

生成 `Dev.D.UE.0.0.9B.P36.0.r0_report.md`，保存至：

    C:\\AIDev\\shanmen-ue\\Dev.D.UE.0.0.9B\\Docs\\Report

Report 至少列出：

1. 实际修改／未修改文件及职责；
2. P4/P23/P29/P30/P34 shared QuickTransfer 到 P36 P35 root 的活动调用图，以及没有新增 pointer/UI/Actor 写入路径的证据；
3. P35 canonical eligibility、P31 exact record identity、source placement、P32/P33/P34/P29/P30 branch rejection 与 P35 provenance gate 证据；
4. 输入时 mode 冻结的结构、child identity／open generation／focus／close／stale lifecycle，以及 BaseQuick mode 不受输入后 child 状态影响的零写入结论；
5. child target 和 BaseQuick target 各自真实 stable `SlotIndex`次序、capacity/empty 验证，以及不存在 auto equip、target-mode fallback、Swap 或隐式目标的证明；
6. P1 whole-root Move、single candidate、single Owner save、ItemId/Definition/Quantity/provenance 保持、accepted-only exact P31 cleanup、Actor refresh、P13 reconcile、rollback 与 ordinal 不变的证据；
7. P29 simple stack、P30/P19 complete graph、P34、P35 normal Drag、P7、P21、P5/P6/P8/P13/P15/P17 与 Code A authority 的保持结论；
8. stable SlotIndex、动态空间容量、scroll、selection、right-click、double-click、ordinary Drag、`Shift + 1—9`和 player-side Ctrl 的无新增写入结论；
9. 两个编译命令、目标、原生 exit code 与关键结果；
10. 所有未执行的 F 阶段真实验证，至少包括：P35 source 建立的多个 standard records；有 valid child 时 Ctrl 到该 child 首个空格；输入时无 child 时 Ctrl 到 BaseQuick 首个空格；child close/switch/focus loss/generation stale/no-empty/parent mismatch 均零写入且不 BaseQuick fallback；BaseQuick 满、record/Owner/Run/revision stale、save failure；P34/P35 normal Drag 保持；stack/space/P32/P33/warehouse/corpse/another record 拒绝；Actor projection、P8 full-registry exclusion、真实鼠标键盘、截图、Smoke、Automation、回归、Cook 与 Package。

仅当 P36 P35-standard QuickTransfer 静态闭合、mode freeze 与 child-priority／BaseQuick-no-child 规则可审计、P29/P30/P34/P35 与 P31 multi-record 边界保持、P8 全量排除无泄漏，且 Editor 与 Game 均以 native exit code `0`完成时，使用：

    READY_FOR_P37_PLANNING

若当前范围内仍有可修复问题，使用：

    NEEDS_P36_REWORK

若共享 QuickTransfer resolver 无法在不创建第二输入路径、第二保存、第二物品真值、自动装备或改写 P17/P29/P34/P35 边界的前提下支持本项，使用：

    NEEDS_PLANNER_DECISION
    BLOCKED

完成后不得自动开始 P37、Fix 或 F。向策划 Chat 回传并附带且只附带本次同名 Report；正文首行使用：

    [CSEMI:REPORT_SENT] {"task_id":"Dev.D.UE.0.0.9B.P36.0.r0","file":"Dev.D.UE.0.0.9B.P36.0.r0_report.md"}
