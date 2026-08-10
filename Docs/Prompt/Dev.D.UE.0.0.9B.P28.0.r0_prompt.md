# Dev.D.UE.0.0.9B.P28.0.r0

## 任务身份

- 项目：`Dev.D.UE.0.0.9B`；继续使用同一活动工程，不新建项目。
- 阶段：主线 P28——地面 simple stack 的显式数量拾回至明确、兼容且未满的玩家既有堆叠。
- 任务编号：`Dev.D.UE.0.0.9B.P28.0.r0`
- 前置：已接受 `0.0.9B.P1—P27` 与 `0.0.9BFix.P1—P4`。`Fix` 只用于已确认、已验收功能的缺陷修复；P28 是新增主线功能，不是 Fix。
- 执行文件：`Dev.D.UE.0.0.9B.P28.0.r0_prompt.md`
- 报告文件：`Dev.D.UE.0.0.9B.P28.0.r0_report.md`
- 活动工程根：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B`
- 活动工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`
- 任务性质：新增主线功能。P 阶段只做实现、静态审查与代码编译；不得启动 F 阶段真实验证，也不得自动开始 P29、任意 Fix 或 F。

## 上半部分：只读项目裁决、现状与边界

### 1. 当前唯一有效基线

当前唯一有效的功能与治理基线是 `0.0.9B`、当前活动工程及其已接受任务链。历史归档、旧版本线、旧页面壳、旧 CTA、旧物品规则与任何 `0.2 / V2 / V3 / I / IPF` 文档均已过时，不得作为实现、验收或范围依据。

Code B P1 Repository 与既有 durable store 是唯一可变物品真值。每件物品只有一个真实实例、一个真实父位置和一条权威事务链；Widget、Presenter、Cell、DragOperation、Workspace Context、数量控件、preview、世界 Actor 与 Code A 地图适配层只能持有展示、选择、悬停或瞬时输入意图，不能持有第二库存、可写数量副本、预建 ItemId、持久 world item 或位置真值。

P4、`0.0.9BFix.P4` 与 P23 已形成共享 Inventory Workspace Kernel：稳定 Slot 地址、通用 Drag payload、统一 modifier router、动态空间容量、稳定排列、`Ctrl + 左键`快速转移、`Shift + 1—9`绑定和双栏滚动。P24 建立了玩家 simple stack 的 transient `SplitDraft`；P25 建立了精确数量 `Merge(N)` 与 normal full-stack `Merge(0)` partial acceptance；P26 建立了 P14 simple world root 的整堆／部分落地、整堆空格拾回与 normal `Merge(0)` 拾回；P27 建立了 `WorldPickupDraft(N)` 到明确空玩家格的 P1 `Split(N)`。

P28 只补 P27 明确保留的一个最小缺口：**已确认 `WorldPickupDraft(N)` 从 P14 simple stack world root 拖到明确、兼容且未满的既有玩家 simple stack 时，使用一次 P1 `Merge(N)` 接受准确数量。** 它不改写 P26 normal Drag，不把 P27 空目标路径改成 Merge，也不把地面精确拾回扩展到任意其他来源／目标。

### 2. P28 产品裁决

以塔科夫式物品工作台为交互参考：数量转移必须由用户先明确 N、再明确落点；系统不自动猜测目标、不静默截断用户确认的数量、不创建第二堆叠来完成合并。正常拖拽与显式数量拖拽是不同意图，但都必须落在同一个 P1 事务与单次 durable commit。

| 动作 | 明确输入 | 接受后的正式语义 |
| --- | --- | --- |
| 地面 normal Drag 到兼容堆叠 | 不带 `WorldPickupDraft` 的 world root → 明确 occupied Cell | 保持 P26：P1 `Merge(Quantity=0)`；由 P1 在当前权威容量内裁决可接受量。 |
| 地面按数量拾回到空格 | 已确认 `WorldPickupDraft(N)` → 明确空玩家普通储物格 | 保持 P27：P1 `Split(N)`；world root 保留原 ItemId 并减 N，空目标获得唯一 accepted `CreatedItemId`，数量为 N。 |
| 地面按数量拾回到既有堆叠 | 已确认 `WorldPickupDraft(N)` → 明确 occupied compatible Cell | 本 P28：P1 `Merge(N)`；world root 保留原 ItemId 并恰减 N，目标既有 ItemId 保留并恰增 N，不创建新 ItemId。 |

P28 的 `N` 必须满足 `1 <= N < WorldSourceQuantity`；目标 available 必须满足 `Available >= N`。`N == WorldSourceQuantity` 不属于 P28，仍由 P26 normal Drag 的 whole-root `Move` 或 `Merge(0)` 处理。`Available < N` 必须明确拒绝，不能把 N 静默缩小为 available、不能自动转为 P26 normal Merge、不能先 Split 再 Merge，也不能新建临时堆叠。

### 3. 范围与持续有效边界

P28 只覆盖当前活动 `OwnerId + RunInstanceId` 内、已由 P14 建立且仍有效的 simple stack world root，以及当前玩家 P6 的明确 occupied compatible simple stack：

| P28 允许区域 | 允许动作 |
| --- | --- |
| 当前可用 P14 simple stack world root | 发起或复用一次已确认的 `WorldPickupDraft(N)`。 |
| P6 `BaseQuick` 内明确 occupied compatible simple stack | 接收 P28 的精确 `Merge(N)`。 |
| 当前已打开、合法、可写的 P17 空间 parent 所拥有 child container 内明确 occupied compatible simple stack | 接收 P28 的精确 `Merge(N)`。 |

以下边界持续有效：

1. P1/P2/P3/P4/P24/P25/P26/P27 的既有 Repository candidate、revision gate、durable callback 与一次 Owner document 保存仍是唯一写入路径。P28 不得直接写 Widget、Actor 缓存、P14 record、P6 graph、Item Quantity 或 world projection。
2. `WorldPickupDraft` 仍是仅世界 simple stack 来源的 transient kind；P28 可扩展其**明确 occupied compatible target**资格，但不得让 P24 `PlayerSplit`、普通 normal Drag、P9/P11、装备、空间 parent、child operation、尸体／容器来源或任何非 P14 simple world root 静默获得该能力。
3. P27 的明确空 target `Split(N)` 必须逐字保持产品语义。P28 只新增 occupied compatible target 的 `Merge(N)` 分支；不得删除、替换、泛化或放宽 P27 的空格验证。
4. P14 的 single-root record、`WorldDropId`、world container、ordinal、floor placement 与 Actor refresh 保持单一事实链。P28 partial Merge 保留同一 world root ItemId、同一 record、同一 world container、同一 `WorldDropId` 与同一 ordinal；它不新建第二 record、第二 Actor、第二 world container 或新 ordinal。
5. P19 的 `WindTalisman`、`BackpackLevel1` 与其他带 `ChildContainerId` 的完整图落地／拾回路径保持原状。P28 只能处理无 `ChildContainerId` 的 simple stack，不得扩大 P19 的资格、child closure、落点、取回、Actor 或 P8 语义。
6. P5、P9、P11、装备位、Hotbar 引用槽、空间 parent、外部容器、尸体、搜索状态、世界多物品容器、地图、敌人、战斗、生命、死亡、撤离、终局、商店与经济不属于 P28。

## 下半部分：授权执行内容

### 4. 单一授权目标

在不产生第二物品图或第二 world inventory 的前提下，使当前 P14 simple stack world root 的已确认 `WorldPickupDraft(N)` 可以落到一个明确、已占用、同定义、可堆叠且 capacity 足够的 P6 玩家普通储物 Cell。最终 accepted transaction 必须在同一个 P14/P6 Owner document candidate 中调用 P1 `Merge(N)`，并以 accepted result 原子更新 P6 target、world source 余量、P14 record、P6 revision 与 Actor projection。

任何 P28 accepted transaction 必须同时满足：

1. world source、明确玩家 target、P14 record、P6 revision、必要 P13 reconcile、world Actor projection 和 durable record 要么共同接受，要么全部保持旧值；不得先扣 world Quantity、后尝试加目标 Quantity，或先更新 Actor、后保存 Owner document。
2. P28 `Merge(N)` 不得创建任何 ItemId。world source 保留原 ItemId、原 ContainerId、原 SlotIndex、原 `WorldDropId`、原 record 与 `WorldSourceQuantity - N`；target 保留原 ItemId、原 ContainerId、原 SlotIndex 与 `TargetQuantity + N`。UI、payload、record、Actor 与 Code A 均不得 clone、预建或猜测任何 ItemId。
3. target 必须是正式 Capacity 范围内、当前明显可见、已占用、非同源、同 `DefinitionId`、正式 `bStackable == true`、相同有效 `MaxStack`、无 `ChildContainerId` 且有 `Available >= N` 的 P6 普通储物 Cell。不得以显示文字、catalog fallback 或 Widget 推测 stack metadata。
4. 除 world source Quantity、明确 target Quantity，以及 accepted projection 所必需的 revision／P13 reconcile／Actor quantity 更新外，任何其他 ItemId、ContainerId、ChildContainer、SlotIndex、视觉位置、容量投影与 scroll offset 均不改变。

### 5. 实现要求

#### 5.1 先完成静态路径审计

改动前必须审阅并在 Report 中列出：

1. P1/P2/P25 `Merge` 的 source/target address、`Quantity=N` 与 `Quantity=0` 的明确差异、target capacity、source retain／delete、revision、失败原子性与 accepted projection；确认 P28 必须是一次现有权威 P1 `Merge(N)`，而不是 `Split` 后的第二 Merge。
2. P14/P26 simple world record、world container、`WorldDropId`、ordinal、pickup durable callback、partial `Merge` retain／full `Move` cleanup、Actor refresh、floor adapter 与一次 Owner save 的精确边界。
3. P27 `WorldPickupDraft` 的来源资格、数量控件、payload、preview、明确空 target `Split(N)`、取消／失焦／关闭／stale 生命周期；确认如何在不改写 P27 空目标语义的前提下，为同一 kind 增加明确 occupied compatible target `Merge(N)`。
4. P4/P23 shared Cell、normal Drag、payload、preview／commit、稳定 Slot 地址、当前活动 P17 child、输入优先级与当前正式 bStackable／MaxStack 投影。
5. P13 binding reconcile、P8 terminal player-only closure、P19 spatial closure 与 P6 active Run/Owner/Revision guard；明确 P28 不得使 ground remainder 回流 P5，也不得改变 P19。
6. 所有旧的 Actor direct pickup、按钮直接领取、world-side auto target、Widget direct mutation、world quantity cache、预建 ItemId、先 Split 后 Merge、先 Merge 后第二保存、直接改 record／ordinal、world source exchange、P9/P11 target 或 `Ctrl + 左键` ground pickup 入口。

#### 5.2 受限的 WorldPickupDraft 与 occupied preview

1. 继续使用 P27 的 `WorldPickupDraft`／现有 `WorldPickup` kind；除非 C++ 类型系统确有不可避免的最小辨别需要，不得新建平行 inventory intent、第二 draft repository 或含可写物品副本的 `WorldMergeDraft`。Draft 仍只保存 transient source identity、Owner/Run、world record identity、expected revision、requested `N` 与必要 graph identity。
2. 仅当前活动 `OwnerId + RunInstanceId` 中、P14 record/root identity 一致的 simple stack world root 可以开启 Draft。正式来源资格必须同时满足：正 Quantity、大于 1、正式 `bStackable`、`MaxStack > 1`、无 `ChildContainerId`、精确 source `ContainerId + SlotIndex + ItemId`、有效 world record、当前 P6 session/revision 未 stale。
3. Draft 必须由用户的显式数量入口启动并确认 `N`。确认前、drag 开始前与最终提交前均重新验证 `1 <= N < SourceQuantity`。确认前不得创建 ItemId、预扣 source Quantity、预占 target、预写 record 或改变 Actor 标签。
4. 对现有显式空玩家普通储物 Cell，保持 P27 的 `Split(N)` preview/commit/eligibility 原样。对已占用 target，P28 只在它同时满足正式 same `DefinitionId`、`bStackable`、same `MaxStack`、`0 < TargetQuantity < MaxStack`、`Available >= N`、无 `ChildContainerId`、非 equipment／Hotbar／parent／external 时显示精确 `Merge(N)` preview。preview 只能显示 accepted amount、source remainder 与 target result，不能改写任何物品或 world state。
5. `Available < N`、不同 definition、non-stackable、MaxStack 无效、target full、target child／parent 非法、target stale、source==target、未打开 child 或不可见 target 均必须拒绝。不得显示“自动调整 N”或转成 normal Drag；用户若要 normal partial acceptance，仍使用 P26 的无 Draft normal Drag。
6. 无效、取消、Esc、失焦、关闭工作台、Tab/I 页面切换、gate 变只读、source／target／record／Run／revision stale、另一项 drag、Actor 失效或 Drop 到无效区域时，Draft/preview 必须清理且零写入。

#### 5.3 单一精确 Partial Pickup Merge 事务

1. 用户将一个已确认 `WorldPickupDraft(N)` Drop 到明确 occupied compatible P6 target 后，shared workspace 只能提交一个 transient P28 intent。它必须携带 source stable address、target stable address、Owner、Run、world record／`WorldDropId`、expected revision、graph identity、source full quantity 与 requested `N`；它不得携带 target 的可写 quantity copy 或新 ItemId。
2. 在 Store durable write 前，重新校验 Owner、Run、source address、world record/root identity、world container、target address、source/target正式 definition、`bStackable`、MaxStack、`N`、target available、P6 revision 与当前 graph identity。任何 mismatch 必须拒绝且不保存。
3. 仅在所有 gate 都通过后，于同一个 P14/P6 candidate 执行 P1 `Merge(N)`：source 是当前 world root，target 是该明确 occupied compatible player storage slot。禁止调用 `Split`、禁止先建临时 stack、禁止用 `Merge(0)` 代替用户确认的 N。
4. 只有 P1 accepted snapshot 同时证明“同一既有 world source 减 N、同一既有 target 增 N、两者 ItemId/placement 保留、无新 ItemId、无 item delete、无其他 ItemId/slot 改动、repository revision 只推进一次”时，才可更新 P14 record 与 accepted projection。建议新增或严格扩展一个结构化证明器，例如 `IsExactP28WorldPickupMergeDelta`；它必须与 P25 generic Merge proof、P26 normal Merge proof 和 P27 Split proof 各自可区分，不得把任意数量变化宽松地视为 P28。
5. accepted P28 transaction 必须继续满足 `N < WorldSourceQuantity`，因此 world root 不会归零。world record、world container、world root ItemId、`WorldDropId` 与 ordinal 必须保留；record 与 Actor 的 Quantity 只能从 accepted source projection 重读。不得建新 world record、额外 Actor、额外 ordinal、reverse operation、second save 或 UI quantity copy。
6. 若 P1、candidate proof、record identity、P13 reconcile、session validation、Actor projection 或 durable save 任一失败，恢复完整 BeforeSnapshot：world source Quantity、target Quantity、record、ordinal、revision、Actor 与 binding 均保持旧值。不得让 local projection 接受未保存的 Merge。

#### 5.4 原子性、P13、P8、Code A 与稳定布局

1. P28 transaction 必须沿 P14/P26/P27 的 single Owner document candidate 与一次 durable save。P1 candidate、world record 校验、P13 reconcile 或 durable save 失败时，恢复完整 BeforeSnapshot；不存在先写 P6 后写 P14、先写 P14 后写 P6、two-save 或 second transaction。
2. world root 永远不自动绑定。P28 target 保留自身既有 ItemId，因此若 target 已有合法 Hotbar 引用，该引用保持；不得将 world source 的任何引用复制到 target，也不得新建、解绑、自动使用或重新排序其他 binding。仅使用既有 repository-level `ReconcileHotbarBindings` 所必需的行为。
3. P8 `Extracted`、`Dead` 与 `RecoveredAbandon` 继续以 player-only closure 处理 P6 player graph。P28 target 正常属于玩家图；地面残余 root 仍由既有 P14 record/closure 排除和清理，不能因 partial Merge 回流 P5。
4. Code A 仍只负责 floor transform、Actor 刷新／销毁与页面入口。它不能拥有或写入 ItemId、Quantity、Container、record、ordinal、Run、Player、Loot 或 pickup outcome；Actor 显示仅从 Store accepted projection 读取。
5. 成功后只刷新 world source／Actor 与明确 target Cell。禁止 Sort、Compact、occupied-only AddChild、数组重编号、空间区重建、自动整理、无关 selection 清除或无关 scroll reset；动态空间容量、parent placement、child contents 与 P19 graph 不得改变。

#### 5.5 输入语义保持

1. 普通 left-click：仍只选择。
2. normal Drag：不带数量 Draft 时仍是 P26 整堆 drop／pickup 与 existing full-stack `Merge(0)` 的唯一普通写入入口；P28 不得劫持它成为精确数量 Merge。
3. `WorldPickupDraft(N)`：落到明确空格时保持 P27 `Split(N)`；落到明确 occupied compatible target 时才走 P28 `Merge(N)`；任何其他目标零写入拒绝。
4. `Ctrl + 左键`：继续执行既有 QuickTransfer 规则；world source 仍明确零写入拒绝，不形成快捷拾取或自动目标。
5. `Shift + 1—9`：继续只执行 P13 Bind；world item／Draft 不得因此绑定、移动、使用或修改数量。
6. right-click：仍只读详情；double-click、详情开关、Tab、I、Esc、关闭、取消、失焦、滚动、无 payload Drop 与 Actor 点击均不得写 P28。

### 6. 允许范围

允许最小修改：

- P3/P4/P24/P25/P26/P27 的共享 workspace context、数量控件、现有 `WorldPickup` kind、drag payload、occupied target preview／commit、stable Cell、projection refresh 与输入生命周期；
- P14/P26/P27 所在的 P6 durable store、world record、world container、Actor refresh、P13 reconcile、P8 player-only closure，仅限对 accepted P28 exact partial Merge 的原子表达与结构化验证；
- 仅为 Owner/Run/revision/target identity、结构化拒绝或 Actor presentation 所需的最小 Code A 边缘适配；
- 必要 include、声明、Build.cs、项目资料与本任务 Report。

除非编译确实证明必须修改，否则不改 P1 `Merge` 引擎、P2 Apply 或 P5→P6 bridge；P28 应复用 P25 已接受的 P1 `Merge(N)` 语义。

### 7. 明确不在本任务内

- 不新建项目、版本线、Fix、第二 Repository、第二 Warehouse、第二 WorldDrop inventory、Widget inventory、fixture、假 ItemId、Code A mirror、A/B 双写或存档重置。
- 不修改 P5→P6 bridge、StartAttempt、M01、P8 receipt、P13 的既有产品语义、P15 Use、P17 space graph、P19 complete spatial closure、P20/P21 来源、搜索、Loot Profile、尸体装备、战斗、生命、死亡、撤离、终局、地图、敌人、商店、经济、制作、网络或多人。
- 不实现从地面按数量拿取 `N == source quantity`、world source stack exchange、ground multiple-root container、地面投放、从玩家按数量丢地、从地面拆分到多个格、Take All、自动拾取、Actor direct pickup、right-click/double-click quick move、Alt 自动装备、自动绑定、自动使用、自动整理、多格物品、旋转、重量、分类筛选或仓库搜索工具。
- 不允许 P9/P11 container／corpse item 成为 P28 source／target，不允许 world item 直达 P5 warehouse、装备位、Hotbar、未打开空间 child、空间 parent 或 UI 外区域。
- 不允许 `Available < N` 的静默截断、P27 `Split` 后再 Merge、P26 `Merge(0)` 替代 `Merge(N)`、第二 world record／Actor、第二 durable save 或完整 root 的自动清理。
- 不启动产品、PIE、Standalone、真实鼠标键盘输入、截图、Smoke、Automation、回归、试玩、Cook、Package 或最终验收。
- 不未经回收本 Report 自动开始 P29、任意 Fix 或 F 阶段。

### 8. P 阶段静态审查与编译

完成后只执行下列检查：

1. 审查 `WorldPickupDraft`、数量控件、payload、preview 与 Actor 均为 transient；只有 P14/P6 Store 内的 P1 accepted `Merge(N)` 可改写 P6 graph、world Quantity、record、revision 或可见投影。
2. 审查 P28 完整 call graph：显式数量确认 → existing world draft → 明确 occupied compatible target preview → Store revalidation → one P1 `Merge(N)` → accepted source/target projection → P14 record/Actor update → one Owner save。确认不存在 clone、预建 ItemId、预扣 Quantity、先 Split 后 Merge、P26 `Merge(0)` fallback、second record、second Actor、two-save 或 UI direct mutation。
3. 审查 accepted P28 Merge：world root ItemId、world container、WorldDropId、record 与 ordinal 保留；target ItemId/placement 保留；无新 ItemId、无 delete；source 恰减 N、target 恰增 N；失败路径完全恢复 BeforeSnapshot。
4. 审查 P27 空格 Split、P26 normal Drag、P25 exact/full Merge、P14/P19/P8/P13 的原语义全部保持，且 P28 不扩大 P9/P11、装备、Hotbar、parent、非法 child、尸体或外部容器资格。
5. 审查正式 stack metadata、Owner/Run/Revision、target availability、stable SlotIndex、动态空间容量、scroll 与输入语义；确认没有 Quick pickup、auto target、Sort、Compact、auto behavior 或世界第二真值。
6. 编译 Editor：

       "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_mapEditor Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload

7. 编译 Game：

       "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_map Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload

若编译失败，只修复本任务引入的 world exact partial Merge、P14/P6 transaction、shared UI、projection、include 或签名问题；若必须扩大到本任务以外，停止受影响部分并如实报告。

### 9. Report 与完成信号

生成 `Dev.D.UE.0.0.9B.P28.0.r0_report.md`，保存至：

    C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Docs\Report

Report 至少列出：

1. 实际修改／未修改文件及职责；
2. P1/P14/P25/P26/P27 到 P28 的完整 call graph，以及 `WorldPickupDraft(N) → Merge(N)` 的单一候选与单次 durable commit；
3. Draft kind、数量确认、occupied compatible preview、空 target P27 分支保持、取消／失焦／关闭／stale 清理与 P24 玩家来源 SplitDraft 未被扩大之证据；
4. world source／P6 target eligibility、formal stack metadata、available ≥ N、P17 child access、P19/P9/P11／装备／Hotbar／parent 排除结论；
5. source ItemId retain、target ItemId retain、Quantity delta、零 new/delete ItemId、world record、world container、WorldDropId、ordinal、Actor refresh 与零 second record/Actor 结论；
6. Owner/Run/Revision、失败恢复、P13 binding、P8 player-only closure、Code A 边缘职责与 stable SlotIndex／scroll 结论；
7. normal whole-root pickup、normal Merge(0)、P27 empty Split、`Ctrl + 左键`、`Shift + 1—9`、right-click、double-click、Actor 点击、详情和滚动各自未改变的语义；
8. 所有明确未实现范围，尤其是 N 等于 source quantity、available 不足时自动截断、stack exchange、Take All、自动目标与 Actor direct pickup；
9. 两个编译命令、目标、原生 exit code 与关键结果；
10. 所有未执行的 F 阶段真实验证。

仅当 `WorldPickupDraft` 的 explicit occupied-target partial Merge 静态闭合、P27/P26/P25/P14/P19/P24/P8/P13 边界保持，并且 Editor／Game 编译均以 native exit code `0` 完成时，使用：

    READY_FOR_P29_PLANNING

若当前范围内仍存在可修复问题，使用：

    NEEDS_P28_REWORK

若现有 P14/P6/P1 结构无法在不改变已接受的持久化或生命周期语义下原子支持 exact partial world pickup Merge，使用：

    NEEDS_PLANNER_DECISION
    BLOCKED

完成后不得自动开始 P29、任意 Fix 或 F。向策划 Chat 回传并附带且只附带本次同名 Report；正文首行使用：

    [CSEMI:REPORT_SENT] {"task_id":"Dev.D.UE.0.0.9B.P28.0.r0","file":"Dev.D.UE.0.0.9B.P28.0.r0_report.md"}
