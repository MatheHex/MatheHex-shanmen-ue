# Dev.D.UE.0.0.9B.P27.0.r0

## 任务身份

- 项目：`Dev.D.UE.0.0.9B`；继续使用同一活动工程，不新建项目。
- 阶段：主线 P27——地面简单堆叠物品的显式数量拾回至明确空格。
- 任务编号：`Dev.D.UE.0.0.9B.P27.0.r0`
- 前置：已接受 `0.0.9B.P1—P26` 与 `0.0.9BFix.P1—P4`。`Fix` 只用于已确认、已验收功能的缺陷修复；P27 是新增主线功能，不是 Fix。
- 执行文件：`Dev.D.UE.0.0.9B.P27.0.r0_prompt.md`
- 报告文件：`Dev.D.UE.0.0.9B.P27.0.r0_report.md`
- 活动工程根：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B`
- 活动工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`
- 任务性质：新增主线功能。P 阶段只做实现、静态审查与代码编译；不得启动 F 阶段真实验证，也不得自动开始 P28、任意 Fix 或 F。

## 上半部分：只读项目裁决、现状与边界

### 1. 当前唯一有效基线

当前唯一有效的功能与治理基线是 `0.0.9B`、当前活动工程及其已接受任务链。历史归档、旧版本线、旧页面壳、旧 CTA、旧物品规则与任何 `0.2 / V2 / V3 / I / IPF` 文档均已过时，不得作为实现、验收或范围依据。

Code B P1 Repository 与既有 durable store 是唯一可变物品真值。每件物品只有一个真实实例、一个真实父位置和一条权威事务链；Widget、Presenter、Cell、DragOperation、Workspace Context、数量控件、preview、世界 Actor 与 Code A 地图适配层只能持有展示、选择、悬停或瞬时输入意图，不能持有第二库存、可写数量副本、预建 ItemId、持久 world item 或位置真值。

P4、`0.0.9BFix.P4` 与 P23 已形成共享 Inventory Workspace Kernel：稳定 Slot 地址、通用 Drag payload、统一 modifier router、动态空间容量、稳定排列、`Ctrl + 左键`快速转移、`Shift + 1—9`绑定和双栏滚动。P24 已建立玩家 simple stack 的 transient `SplitDraft`；P25 已建立精确数量 Merge 与 normal full-stack partial acceptance；P26 已完成 P14 simple world root 的整堆／部分落地、整堆空格拾回与 normal Merge 拾回。

P26 有意未开放“从地面 root 按数量拿取 N 到空玩家格”。本 P27 只补这一条缺口。它不改写 P24 的玩家来源 `SplitDraft`，也不改变 P26 的 whole-root pickup、ground partial Merge、P14 record、P19 空间完整图或 P8 player-only closure 的既有语义。

### 2. P27 产品裁决

P27 将塔科夫式物品工作台的“地面简单堆叠物品按数量拾回”收敛为一条明确、可撤销、无自动目标的权威事务：玩家先从当前可用的 simple world root 明确确认数量 `N`，再将该 transient 意图拖到一个明确、可写、空的玩家普通储物格；只有最终落点被权威接受时，P1 才以一次 `Split(N)` 创建玩家目标 ItemId。

| 动作 | 明确输入 | 接受后的正式语义 |
| --- | --- | --- |
| 地面整堆拾回 | 既有 normal Drag：world root → 明确空玩家格 | 保持 P26 的 P1 `Move`；world root 原 ItemId 原样移入目标，world record/Actor 同次移除。 |
| 地面按数量拾回 | 当前 simple world root 的显式数量确认 `N` → 明确空玩家格 | 仅在最终权威接受时调用 P1 `Split(N)`；world source 保留原 ItemId 且恰减 `N`，玩家目标获得 P1 accepted `CreatedItemId`，数量恰为 `N`。 |

`N` 必须满足 `1 <= N < WorldSourceQuantity`。`N == WorldSourceQuantity` 不走 P27；用户仍使用 P26 的 normal Drag whole-root pickup。P27 不提供自动目标、Take All、Actor 直接领取、`Ctrl + 左键`拾取、右键拾取或从地面到占用堆叠的精确 Merge。

### 3. 范围与持续有效边界

P27 只覆盖精确活动 `OwnerId + RunInstanceId` 内、已由 P14 建立并仍有效的 simple stack world root，以及当前玩家 P6 的明确空普通储物格：

| P27 可作为来源／目标的区域 | 允许的动作 |
| --- | --- |
| 当前可用 P14 simple world root | 发起一次显式 `WorldPickupDraft(N)`，并拖至一个明确的空 P6 玩家格。 |
| P6 `BaseQuick` 空普通储物格 | 接收 P27 的按数量拾回。 |
| 当前已打开、合法、可写的 P17 空间 parent 所拥有的 child container 的空普通储物格 | 接收 P27 的按数量拾回。 |

以下边界持续有效：

1. P1/P2/P3/P4/P24/P25/P26 的既有 Repository candidate、revision gate、durable callback 与一次 Owner document 保存仍是唯一写入路径。P27 不得直接写 Widget、Actor 缓存、P14 record、P6 graph、Item Quantity 或 world projection。
2. P24 的玩家来源 `SplitDraft` 保持原资格、字段和产品语义。P27 如需要使用同一数量控件或生命周期帮助器，只能新增带明确 kind 的 transient `WorldPickupDraft`／等价受限意图；它不得让 P24 Draft 静默接受 P9/P11、装备、空间 parent、child 操作或任意非 P27 world source。
3. P14 的 single-root record、`WorldDropId`、world container、ordinal、floor placement 与 Actor refresh 保持单一事实链。P27 partial pickup 保留同一 world root ItemId、同一 record、同一 world container、同一 `WorldDropId` 与同一 ordinal；它不新建第二 record、第二 Actor、第二 world container 或新 ordinal。
4. P19 的 `WindTalisman`、`BackpackLevel1` 与其他带 `ChildContainerId` 的完整图落地／拾回路径保持原状。P27 只能处理无 `ChildContainerId` 的 simple stack，不得扩大 P19 的资格、child closure、落点、取回、Actor 或 P8 语义。
5. P5、P9、P11、装备位、Hotbar 引用槽、空间 parent、外部容器、尸体、搜索状态、世界多物品容器、地图、敌人、战斗、死亡、撤离、商店与经济不属于 P27。

## 下半部分：授权执行内容

### 4. 单一授权目标

在不产生第二物品图或第二 world inventory 的前提下，为 P14 simple stack world root 增加一个**仅可显式确认、仅可落到明确空玩家普通储物格**的 partial pickup intent。它必须在同一个 P14/P6 Owner document candidate 中调用 P1 `Split(N)`，并以 accepted result 原子更新 P6 目标、world source 余量、P14 record、P6 revision 与 Actor projection。

任何 P27 accepted transaction 必须满足：

1. world source、明确玩家 target、P14 record、P6 revision、必要 P13 reconcile、world Actor projection 和 durable record 要么共同接受，要么全部保持旧值；不得先扣 world Quantity、后尝试创建玩家 item，或先更新 Actor、后保存 Owner document。
2. partial pickup 的唯一新 ItemId 只能来自 P1 `Split(N)` 的 accepted `CreatedItemId`。source world root 保留原 ItemId、原 ContainerId、原 `WorldDropId`、原 record 与 `WorldSourceQuantity - N`；UI、payload、record、Actor 与 Code A 均不得预建、clone 或猜测该 ItemId。
3. 目标必须是一个正式 Capacity 范围内、当前明确可见且为空的 P6 普通储物 Cell。该目标不能是已占用堆叠、装备位、Hotbar、空间 parent、未打开／非法 child、P5、P9、P11 或 UI 外区域。
4. 除 world source 的 Quantity、明确 target cell、新的 target ItemId／Quantity，以及 accepted projection 必要更新外，任何其他 ItemId、ContainerId、ChildContainer、SlotIndex、视觉位置、容量投影与 scroll offset 均不改变。

### 5. 实现要求

#### 5.1 先完成静态路径审计

改动前必须审阅并在 Report 中列出：

1. P1/P2 `Split` 的 source/target address、`CreatedItemId` 生命周期、候选验证、source retain、Revision、失败原子性与 accepted projection；确认 world source → P6 target 仍经同一权威 P1 路径，而不是临时 clone。
2. P14/P26 simple world record、world container、`WorldDropId` ordinal、pickup durable callback、partial Merge retain／full Move cleanup、Actor refresh、floor adapter 与一次 Owner save 的精确边界。
3. P24 `SplitDraft` 的玩家来源资格、数量控件、取消／失焦／关闭／stale 生命周期；共享工作台如何在不扩大 P24 语义的前提下表达一个明确 kind 的 `WorldPickupDraft`。
4. P4/P23 shared Cell、normal Drag、payload、preview／commit、稳定 Slot 地址、当前活动 P17 child、输入优先级与空 target projection。
5. P13 binding reconcile、P8 terminal player-only closure、P19 spatial closure 与 P6 active Run/Owner/Revision guard；明确 P27 不得将 ground remainder 回流 P5，也不得改变 P19。
6. 所有旧的 Actor direct pickup、按钮直接领取、world-side auto target、Widget direct mutation、world quantity cache、预建 ItemId、先 Split 后第二保存、直接改 record／ordinal、world source Merge、P9/P11 target 或 `Ctrl + 左键` ground pickup 入口。

#### 5.2 受限的 WorldPickupDraft

1. 仅当前活动 `OwnerId + RunInstanceId` 中、P14 record/root identity 一致的 simple stack world root 可以开启 `WorldPickupDraft`。正式资格必须同时满足：正 Quantity、大于 1、正式 `bStackable`、`MaxStack > 1`、无 `ChildContainerId`、精确 source `ContainerId + SlotIndex + ItemId`、有效 world record、当前 P6 session/revision 未 stale。
2. Draft 必须由用户的显式数量入口启动并确认 `N`；普通 left-click 仍只选择，right-click 仍只读详情。UI 控件的具体展示可沿用现有数量控件，但确认前它只能保存 transient source identity、Owner/Run、world record identity、expected revision、`N` 和必要 graph identity，不能创建 ItemId、预扣 source Quantity、预占 target、预写 record 或改变 Actor 标签。
3. `N` 必须在确认及最终提交前重新验证为 `1 <= N < SourceQuantity`。无效、取消、Esc、失焦、关闭工作台、Tab/I 页面切换、gate 变只读、source／record／Run／revision stale、另一项 drag、Actor 失效或目标无效时，Draft/preview 必须清理且零写入。
4. `WorldPickupDraft` 只能在当前明显可见的 workspace 内拖到一个明确空玩家普通储物 Cell。它不得作为 normal full-stack Drag 的替代，不得拖到 GroundDropZone、world target、P5、P9、P11、装备、Hotbar、空间 parent、占用堆叠、未打开 child 或 UI 外区域。
5. 不得以“通用化”为名让任何其他 world root、空间完整图、non-stackable item、尸体／容器来源、玩家来源、child operation 或现有 P24 `SplitDraft` 自动获得此能力。scope policy 必须在 Store 与 shared UI 两侧一致地拒绝非法情况。

#### 5.3 单一 Partial Pickup 事务

1. 用户将一个已确认 `WorldPickupDraft(N)` Drop 到明确空 P6 target 后，shared workspace 只能提交一个 transient intent；Store 必须在写入前重新校验 Owner、Run、source address、world record/root identity、target address、target empty/capacity、`bStackable`、MaxStack、`N`、P6 revision 与当前 graph identity。
2. Store 必须在同一个 P14/P6 candidate 中构造 P1 `Split(N)`：source 是当前 world root，target 是该明确空玩家 storage slot。P1 接受后，world source 只保留原 ItemId 与 `SourceQuantity - N`；target 只使用 P1 accepted `CreatedItemId`，Quantity 恰为 `N`。
3. 只有 P1 accepted snapshot 同时证明“一个已存在 world source 减 N、一个明确 empty target 获新 ItemId N、无其他 ItemId/slot 改动”时，才可更新 P14 record 与 accepted projection。若候选、record、save、reconcile 或任何身份验证失败，恢复 BeforeSnapshot：world source Quantity、target empty 状态、record、ordinal、revision、Actor 与 binding 均保持旧值。
4. partial pickup 接受后，world record、world container、world root ItemId、`WorldDropId` 与 ordinal 必须保留；record 与 Actor 的 Quantity 只能从 accepted source projection 重读。不得建新 world record、额外 Actor、额外 ordinal、反向 Split、second save 或 UI quantity copy。
5. P27 不改变 P26 的 whole-root Move：不带 Draft 的 normal world drag 到空 target 仍沿 P26 `Move` 完整领取并清理 world record；P27 Draft 的 `N == SourceQuantity` 必须被拒绝，不能静默降级为 Move。

#### 5.4 原子性、P13、P8、Code A 与稳定布局

1. P27 transaction 必须沿 P14/P26 的 single Owner document candidate 与一次 durable save。P1 candidate、world record 校验、P13 reconcile 或 durable save 失败时，恢复完整 BeforeSnapshot；不存在先写 P6 后写 P14、或先写 P14 后写 P6 的双提交。
2. 新玩家 target ItemId 不自动进入 Hotbar、不继承 world source 的任何引用、不自动使用；world root 永远不自动绑定。除既有 repository-level reconciliation 所必需者外，P13 的其他 binding 不变。
3. P8 `Extracted`、`Dead` 与 `RecoveredAbandon` 继续以 player-only closure 处理 P6 player graph。P27 生成的 target 正常属于玩家图；地面残余 root 仍由既有 P14 record/closure 排除和清理，不能因 partial pickup 回流 P5。
4. Code A 仍只负责 floor transform、Actor 刷新／销毁与页面入口。它不能拥有或写入 ItemId、Quantity、Container、record、ordinal、Run、Player、Loot 或 pickup outcome；Actor 显示仅从 Store accepted projection 读取。
5. 成功后只刷新 world source／Actor 与明确 target Cell。禁止 Sort、Compact、occupied-only AddChild、数组重编号、空间区重建、自动整理、无关 selection 清除或无关 scroll reset；动态空间容量、parent placement、child contents 与 P19 graph 不得改变。

#### 5.5 输入语义保持

1. 普通 left-click：仍只选择。
2. normal Drag：仍是 P26 整堆 drop／pickup 与 existing full-stack merge 的唯一普通写入入口；P27 的精确拾回仅接受明确确认后的 `WorldPickupDraft` payload。
3. `Ctrl + 左键`：继续执行既有 QuickTransfer 规则；world source 仍明确零写入拒绝，不形成快捷拾取或自动目标。
4. `Shift + 1—9`：继续只执行 P13 Bind；world item／Draft 不得因此绑定、移动、使用或修改数量。
5. right-click：仍只读详情；double-click、详情开关、Tab、I、Esc、关闭、取消、失焦、滚动、无 payload Drop 与 Actor 点击均不得写 P27。

### 6. 允许范围

允许最小修改：

- Code B P1/P2 的既有 Split request、candidate validation、accepted result 与必要 generic source/target address 支持，但仅为 P14 simple world source → explicit P6 empty target 的单一事务；
- P3/P4/P24/P25/P26 的共享 workspace context、数量控件、带 kind 的 transient draft、drag payload、preview／commit、stable Cell、projection refresh 与输入生命周期；
- P14/P26 所在的 P6 durable store、world record、world container、Actor refresh、P13 reconcile、P8 player-only closure，仅限对 accepted partial pickup 的原子表达；
- 仅为 Owner/Run/revision/target identity、结构化拒绝或 Actor presentation 所需的最小 Code A 边缘适配；
- 必要 include、声明、Build.cs、项目资料与本任务 Report。

### 7. 明确不在本任务内

- 不新建项目、版本线、Fix、第二 Repository、第二 Warehouse、第二 WorldDrop inventory、Widget inventory、fixture、假 ItemId、Code A mirror、A/B 双写或存档重置。
- 不修改 P5→P6 bridge、StartAttempt、M01、P8 receipt、P13 的既有产品语义、P15 Use、P17 space graph、P19 complete spatial closure、P20/P21 来源、搜索、Loot Profile、尸体装备、战斗、生命、死亡、撤离、终局、地图、敌人、商店、经济、制作、网络或多人。
- 不实现从地面按数量 Merge 到已占用堆叠、world source stack exchange、ground multiple-root container、地面投放、从玩家按数量丢地、从地面拆分到多个格、Take All、自动拾取、Actor direct pickup、right-click/double-click quick move、Alt 自动装备、自动绑定、自动使用、自动整理、多格物品、旋转、重量、分类筛选或仓库搜索工具。
- 不允许 P9/P11 container／corpse item 成为 P27 source／target，不允许 world item 直达 P5 warehouse、装备位、Hotbar 或未打开空间 child。
- 不启动产品、PIE、Standalone、真实鼠标键盘输入、截图、Smoke、Automation、回归、试玩、Cook、Package 或最终验收。
- 不未经回收本 Report 自动开始 P28、任意 Fix 或 F 阶段。

### 8. P 阶段静态审查与编译

完成后只执行下列检查：

1. 审查 `WorldPickupDraft`、数量控件、payload、preview 与 Actor 均为 transient；只有 P14/P6 Store 内的 P1 accepted `Split(N)` 可改写 P6 graph、world Quantity、record、revision 或可见投影。
2. 审查 P27 的完整 call graph：显式数量确认 → draft → 明确空 target Drop → Store revalidation → one P1 `Split(N)` → accepted source/target projection → P14 record/Actor update → one Owner save。确认不存在 clone、预建 ItemId、预扣 Quantity、预占 ordinal、second record、second actor、two-save 或 UI direct mutation。
3. 审查 accepted partial pickup：world root ItemId、world container、WorldDropId、record 与 ordinal 保留；唯一新 ItemId 仅来自 accepted `CreatedItemId`；source 减 N、target 为 N；失败路径完全恢复 BeforeSnapshot。
4. 审查资格与零写入：simple stack、P17 child access、P19 spatial graph、P9/P11、装备/Hotbar、target occupied、Owner/Run/Revision、world record/root、`N` 边界、Actor 状态和 normal whole-root pickup 的既有路径均正确隔离。
5. 审查 P13、P8、Code A、稳定 SlotIndex、动态空间容量、scroll 与输入语义；确认没有 Quick pickup、auto target、Sort、Compact、auto behavior 或世界第二真值。
6. 编译 Editor：

       "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_mapEditor Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload

7. 编译 Game：

       "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_map Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload

若编译失败，只修复本任务引入的 world partial pickup、P14/P6 transaction、shared UI、projection、include 或签名问题；若必须扩大到本任务以外，停止受影响部分并如实报告。

### 9. Report 与完成信号

生成 `Dev.D.UE.0.0.9B.P27.0.r0_report.md`，保存至：

    C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Docs\Report

Report 至少列出：

1. 实际修改／未修改文件及职责；
2. P1/P14/P24/P26 到 P27 的完整 call graph，以及 `WorldPickupDraft(N) → Split(N)` 的单一候选与单次 durable commit；
3. Draft kind、数量确认、取消／失焦／关闭／stale 清理与 P24 玩家来源 SplitDraft 未被扩大之证据；
4. world source／P6 target eligibility、P17 child access、P19/P9/P11／装备／Hotbar 排除与空 target 限制；
5. `CreatedItemId`、source ItemId retain、Quantity、world record、world container、WorldDropId、ordinal、Actor refresh 与零 second record/Actor 结论；
6. Owner/Run/Revision、失败恢复、P13 binding、P8 player-only closure、Code A 边缘职责与 stable SlotIndex／scroll 结论；
7. normal whole-root pickup、`Ctrl + 左键`、`Shift + 1—9`、right-click、double-click、Actor 点击、详情和滚动各自未改变的语义；
8. 所有明确未实现范围，尤其是 ground exact Merge 到占用 target、Take All、自动目标与 Actor direct pickup；
9. 两个编译命令、目标、原生 exit code 与关键结果；
10. 所有未执行的 F 阶段真实验证。

仅当 `WorldPickupDraft` 的 explicit partial pickup 静态闭合、P14/P19/P24/P25/P26/P8/P13 边界保持，并且 Editor／Game 编译均以 native exit code `0` 完成时，使用：

    READY_FOR_P28_PLANNING

若当前范围内仍存在可修复问题，使用：

    NEEDS_P27_REWORK

若现有 P14/P6/P1 结构无法在不改变已接受的持久化或生命周期语义下原子支持 partial world pickup，使用：

    NEEDS_PLANNER_DECISION
    BLOCKED

完成后不得自动开始 P28、任意 Fix 或 F。向策划 Chat 回传并附带且只附带本次同名 Report；正文首行使用：

    [CSEMI:REPORT_SENT] {"task_id":"Dev.D.UE.0.0.9B.P27.0.r0","file":"Dev.D.UE.0.0.9B.P27.0.r0_report.md"}
