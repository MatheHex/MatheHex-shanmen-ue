# Dev.D.UE.0.0.9B.P26.0.r0

## 任务身份

- 项目：`Dev.D.UE.0.0.9B`；继续使用同一活动工程，不新建项目。
- 阶段：主线 P26——简单可堆叠物品的地面丢弃与精确拾回。
- 任务编号：`Dev.D.UE.0.0.9B.P26.0.r0`
- 前置：已接受 `0.0.9B.P1—P25` 与 `0.0.9BFix.P1—P4`。`Fix` 仅用于已确认、已验收功能的缺陷修复；P26 是新增主线功能，不是 Fix。
- 执行文件：`Dev.D.UE.0.0.9B.P26.0.r0_prompt.md`
- 报告文件：`Dev.D.UE.0.0.9B.P26.0.r0_report.md`
- 活动工程根：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B`
- 活动工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`
- 任务性质：新增主线功能。P 阶段只做实现、静态审查和代码编译；不得启动 F 阶段真实验证，也不得自动开始 P27、Fix 或 F。

## 上半部分：只读项目裁决、现状与边界

### 1. 当前唯一有效基线

当前唯一有效的功能与治理基线是 `0.0.9B`、当前活动工程及其已接受任务链。历史归档、旧版本线、旧页面壳、旧 CTA、旧物品规则与任何 `0.2 / V2 / V3 / I / IPF` 文档均不作为实现、验收或范围依据。

Code B P1 Repository 与既有 durable store 是唯一可变物品真值。每件物品只有一个真实实例、一个真实父位置和一条权威事务链；Widget、Presenter、Cell、DragOperation、Workspace Context、详情页、数量输入、世界 Actor 与地图适配层只能保存瞬时展示、选择、预览或输入意图，不能保存第二库存、可写数量副本、预建实例、持久世界物品或位置真值。

P4、`0.0.9BFix.P4` 与 P23 已形成共享 Inventory Workspace Kernel：稳定 Slot 地址、通用 Drag payload、统一 modifier router、动态空间容量、稳定排列、`Ctrl + 左键`快速转移、`Shift + 1—9`绑定和双栏滚动。P24 已将显式数量 `SplitDraft` 接入共享工作台；P25 已将精确数量 Merge 与普通 full-stack partial acceptance 收敛到 P1 的同一权威 Merge。

P14 已建立 P6 内单一根物品的地面 record、确定性 `WorldDropId`、派生 world root、合法地面落点和一次 Owner document 保存；P19 已为两种空间 parent 的完整图地面移动建立独立闭包规则。P26 仅扩展 P14 的**简单可堆叠物品**路径，不重做 P14/P19，也不把世界掉落变为第二库存。

### 2. P26 产品裁决

P26 完成塔科夫式物品工作台最小的“堆叠物品落地—明确拾回”闭环：玩家可把一整堆或一段明确数量的简单可堆叠物品拖到现有地面丢弃区；地面根物品可被明确拖回玩家已指定的合法格，并按 P25 的正式 Merge 规则完整或部分接收。

本任务的产品语义如下：

| 动作 | 明确输入 | 成功后的正式语义 |
| --- | --- | --- |
| 整堆落地 | 合法玩家 root 的 normal drag → GroundDropZone | 复用 P14 的单 root world record 与 P1 `Move`；来源 ItemId 原样移入该 world root，不生成新 ItemId。 |
| 部分落地 | 已确认的 P24 `SplitDraft(N)` → GroundDropZone | 仅在最终权威接受时以 P1 `Split` 在世界单格 root 创建一个新 ItemId；玩家来源恰减 `N`，世界 root 恰为 `N`。 |
| 整堆拾回至空格 | Ground root normal drag → 明确的空玩家普通储物格 | 复用 P1 `Move`；world root ItemId 原样移入目标，world record 与 world root 同次提交移除。 |
| 拾回至兼容未满堆叠 | Ground root normal drag → 明确的已占用兼容玩家普通储物格 | 复用 P25 full-stack Merge（`Quantity = 0`）；目标按 P1 正式容量完整或部分接收。若 ground source 仍有余量，保留同一 root ItemId 与同一 world record；仅归零时移除它们。 |

这不是自动拾取、快捷拾取、Take All、地面多物品容器或地面拆分。每一次位置或数量写入均要求一个当前可见、明确、合法的 Drag/Drop 落点。

### 3. 范围与持续有效边界

P26 仅覆盖精确活动 `OwnerId + RunInstanceId` 的 P6 图与现有 P14 单 root world record：

| P26 可作为来源／目标的区域 | 允许的动作 |
| --- | --- |
| P6 `BaseQuick` 的简单可堆叠 root | 整堆落地、部分落地、从地面整堆拾回、从地面部分 Merge。 |
| 当前已打开、合法、可写的 P17 空间 child 内简单可堆叠 root | 整堆落地、部分落地、从地面整堆拾回、从地面部分 Merge。 |
| 当前可用的 P14 simple world root | 仅明确拖回 P6 `BaseQuick` 或当前已打开合法空间 child 的空格／兼容未满堆叠。 |

以下规则持续生效：

1. P1/P2/P3/P4/P24/P25 的既有事务、candidate validation、Revision 与 durable callback 仍是唯一写入链。P26 不得直接写 Widget array、Actor 缓存、P14 record 或 P6 item 数量。
2. 普通 world root 仍只有一个真实 root ItemId 和一个派生 world container。世界 Actor 只是可销毁投影；它不保存数量、定义、ItemId、世界库存或拾回结果的第二真值。
3. P19 的 `WindTalisman`、`BackpackLevel1` 与其他带 `ChildContainerId` 的完整图地面路径保持原状。P26 不改变其来源、合法性、落地、拾回、Actor、child closure 或 P8 语义。
4. 物品兼容性、`bStackable`、`MaxStack`、实际接收数量、身份、容量、slot semantic、Parent/Child graph、Revision、Owner/Run 与保存结果均以 P1/P2 的正式 projection 和 candidate state 为准；不得从名称、图标、Widget 文本或硬编码类型猜测。
5. P5、P9 普通容器、P11 尸体容器、装备位、Hotbar 引用槽、外部目标、WorldDrop 之间的互相转移、搜索、敌人、地图、战斗、死亡、撤离、终局和经济继续沿现有语义，不属于 P26。

## 下半部分：授权执行内容

### 4. 单一授权目标

在不产生第二物品图或第二世界库存的前提下，将 P24/P25 的共享数量意图接入 P14 的 simple world-drop store：允许 P6 简单可堆叠 root 进行明确的整堆／部分落地，并使相应 single-root world drop 能被明确拖回 P6 空格或兼容堆叠，完整复用 P1 的 `Move / Split / Merge` 与 P14 的单次 Owner durable commit。

任何 P26 accepted transaction 都必须满足：

1. 所有来源、目标、world root、P14 record、`WorldDropId` ordinal、P6 revision、Hotbar reconcile 和 durable record 要么共同接受，要么共同保持旧值；不得先扣玩家数量、后创建 world record，或先删 world record、后尝试合并。
2. 整堆落地／整堆拾回只移动原有 root ItemId；部分落地只在 P1 `Split` 接受时创建一个新的世界 root ItemId；地面 partial Merge 不创建新 ItemId。
3. partial ground pickup 后，P14 record 仍精确指向保留在 world root 的同一 ItemId；其当前 Quantity 只从 accepted P1 projection 读取。world source 归零时，才在同一提交内移除 world root、P14 record 与 Actor projection。
4. 除明确来源、明确目标、必要的 world root／record，以及 P13 对已删除来源的既有 reconcile 外，任何 ItemId、ContainerId、ChildContainer、SlotIndex、视觉位置和 scroll offset 均不变。

### 5. 实现要求

#### 5.1 先完成静态路径审计

改动前必须审阅并在 Report 中列出：

1. P14 的 simple whole-root drop、pickup、`WorldDropId` ordinal、派生 world container、floor placement、Actor refresh、P8 player-only closure 与 Owner document 保存路径。
2. P19 的完整空间 parent graph 路径，并定位其与 P14 simple branch 的精确分界；P26 不得扩大或替换该分界。
3. P24 `SplitDraft` 的来源资格、数量输入、生命周期清理和 payload；P25 `RequestedMergeQuantity`、full-stack partial Merge、正式 MaxStack projection 与 accepted result refresh。
4. P4 shared `GroundDropZone`、world target projection、normal Drag、preview/commit、stable address、当前活动空间 child 选择、右键／双击／`Ctrl + 左键`／`Shift + 1—9`输入路由。
5. P1/P2 的 `Move`、`Split`、`Merge`、ItemId 生命周期、source cleanup、candidate validation、Revision 与失败原子性；及 P13 在来源保留或来源删除时的 reconcile。
6. 所有旧的 Actor direct pickup、按钮领取、ground-side auto target、Widget direct mutation、world quantity cache、预建 ItemId、先 Split 后单独写 P14 record、直接删除 world record、容器/尸体落地或空间 parent 误入 simple branch 的入口。

#### 5.2 Simple stack ground-drop intent

1. GroundDropZone 只接受两种新增 P26 payload：
   - 一个当前 P6、简单、可堆叠、无 `ChildContainerId` 的完整 root normal payload；
   - 来自同类合法 P6 source 的已确认 P24 `SplitDraft(N)` payload。
   其余 payload 继续由现有 P14/P19 或零写入拒绝路径处理，不能被 P26 静默改写。
2. simple source 必须具有正 Quantity、正式 `bStackable` 与 `MaxStack > 1`，位于 P6 `BaseQuick` 或当前已打开、合法可访问的空间 child 的普通储物 slot；它不能是装备位、空间 parent、child container、Hotbar 引用、P9/P11 item、Hidden/Searching item、另一 world root 或 UI 外对象。
3. P26 必须把现有 Code A floor-resolved placement adapter 仅作为不可变地图落点输入。其必须先得到合法 placement，再把 exact Owner/Run、source stable address、expected revision、drop mode 与 placement 交给 Store；Code A 不得创建 ItemId、world record、数量或 P6 graph。
4. 整堆落地沿 P14 既有 single-root `Move` 路径。该 root 原 ItemId、Definition、Quantity、稳定 world container、record identity 与 Actor projection 必须来自 accepted P1/P6 result，而非 Drag payload 或 UI 预览。
5. 部分落地必须复用 P24 的 confirmed `N`：`1 <= N < SourceQuantity`。Store 在同一候选 P6 graph 中构造强制 single-slot world container target，调用 P1 `Split`，只在 accepted result 后以新 root ItemId 建立 P14 record。Draft、payload、preview 与 Actor 均不得预建 ItemId、预扣数量、预占 ordinal 或预写 record。
6. P14 的 ordinal、world record、P1 result、P6 revision、P13 reconcile 和保存必须是一个接受边界。任一 graph、placement、identity、revision、capacity、record、Actor refresh 前的持久化或保存验证失败时，P6 graph、record、ordinal、binding 和可见 projection 全部保持旧值。
7. 成功后的刷新只能从 accepted P6 projection 重读：玩家 source 的余量、world root Quantity、P14 record 与 Actor 显示必须一致。部分落地不会压缩、排序或重建无关 P6／空间 child layout。

#### 5.3 Explicit ground pickup and partial acceptance

1. 当前可用 P14 simple world root 必须作为 normal Drag source，且只可落到用户明确指定的 P6 `BaseQuick` 或当前已打开合法空间 child 的一个普通储物 Cell。world pickup 不接受 Ctrl 快速转移、右键、双击、Actor 点击直接领取、按键领取、自动目标或无 payload Drop。
2. 目标为空普通格时，只允许 P1 `Move` 完整 world root；world root ItemId 原样进入目标。accepted 后同一次 P6 durable replacement 移除对应 P14 record／world container，并刷新/销毁该 Actor projection。
3. 目标为已占用格时，只允许同一正式 StackKey／Definition、`bStackable`、未满、普通储物的 P1 `Merge`。P26 必须传递 P25 的 normal full-stack `Quantity = 0` 语义；P1 而非 UI 裁决实际接受量。
4. 若 P1 Merge 完整接受 world source，移除 P14 record/world root/Actor；若只部分接受，world source 保留同一 ItemId、ContainerId、`WorldDropId` 与 record，且只按 accepted projection 更新其 Quantity/Actor 提示。不得为了 partial pickup 新建 split stack、new record、new ordinal 或第二 actor。
5. 不兼容、满 target、装备位、Hotbar、空间 parent、child 操作、未打开 child、stale revision、错误 Owner/Run、已终局 P6、world record/root identity 不一致、Actor 已失效或 UI 外 Drop 必须零写入：不减少 world Quantity、不删除 record、不移动玩家 item、不改变 P6 revision。
6. P26 不从 world source 发起 SplitDraft，也不实现“从地面精确拿 N 到空格”。这两类操作、stack exchange、multi-root world container 与自动分配仍留给独立后续任务。

#### 5.4 原子性、P13、P8 与稳定布局

1. 所有 P26 transaction 继续使用 P6 的现有 single Owner document candidate 与一次保存。P1 candidate 失败、P14 record 校验失败、P13 reconcile 失败或 durable save 失败时，恢复 BeforeSnapshot，避免位置、数量、record、ordinal 或 binding 部分提交。
2. 玩家来源整堆落地而被正式删除时，沿 P13 repository-level reconcile 清理该来源 binding；部分落地时来源 ItemId 保留且其 binding 不变。world root 永远不自动绑定、自动使用或继承任何 binding；从 world 移入玩家的 ItemId 也不自动绑定。
3. P8 `Extracted`、`Dead` 与 `RecoveredAbandon` 继续只处理玩家图，并清除仍在 P14 world root 的所有完整残余。P26 的 partial world root 必须按同一既有 player-only closure 被排除，绝不能因其剩余 Quantity 让 world item 漏回 P5。
4. Source/target/world actor refresh 只能更新参与的 cell 与明确显示；禁止 Sort、Compact、occupied-only AddChild、数组重编号、空间区重建或无关滚动位置重置。空间 child 的容量、parent placement、child contents 和 P19 graph 语义不得改变。
5. 普通左键仍只选择；`Ctrl + 左键`仍是既有 P23/P4 QuickTransfer，且不获得地面 drop/pickup含义；`Shift + 1—9`仍只进行 P13 Bind；right-click 仍只读详情；double-click、详情、Tab、I、Esc、关闭、滚动、取消、失焦和无 payload Drop 均不得写 P26。

### 6. 允许范围

允许最小修改：

- Code B P1/P2/P3/P4/P24/P25 的现有 request、payload、preview、commit、projection 与 transient lifecycle，仅限将合法 simple stack GroundDrop/ground pickup 收敛到既有权威事务；
- P14 所在的 P6 durable store、world record、ordinal、world container、Actor refresh 与 P8 player-only closure，仅限其对 simple stack `Move / Split / Merge` accepted result 的原子表达；
- GroundDropZone、WorldDropTarget、P7 active P6 source selection、当前合法空间 child 目标投影和最小 Code A floor-placement/Actor presentation adapter；
- P13 reconcile、必要 include、声明、Build.cs、项目资料与本任务 Report。

### 7. 明确不在本任务内

- 不新建项目、版本线、Fix、第二 Repository、第二 Warehouse、第二 WorldDrop inventory、Widget inventory、fixture、假 ItemId、Code A mirror、A/B 双写或存档重置。
- 不修改 P19 空间 parent/child closure、完整空间物品的地面丢弃/拾回、child 地面操作、嵌套袋、空间品级容量、装备效果、武器/道袍/饰品数值或战斗效果。
- 不允许 P9/P11 容器／尸体 item 直接落地或从地面直达外部容器；不改变搜索、Loot Profile、尸体装备、来源、Reveal、Run/地图/敌人、死亡、撤离、商店、经济、制作、网络或多人。
- 不实现地面多物品容器、地面堆叠交换、从地面 split、按数量拾回空格、Take All、自动拾取、Actor direct pickup、右键/双击快速移动、Alt 自动装备、自动绑定、自动使用、自动整理、多格物品、旋转、重量、分类筛选或仓库搜索工具。
- 不启动产品、PIE、Standalone、真实鼠标键盘输入、截图、Smoke、Automation、回归、试玩、Cook、Package 或最终验收。
- 不未经回收本 Report 自动开始 P27、任意 Fix 或 F 阶段。

### 8. P 阶段静态审查与编译

完成后只执行下列检查：

1. 审查 GroundDropZone、SplitDraft、payload、world actor 与 preview 全部是 transient；只有 Store-owned P1/P14 accepted transaction 能修改数量、graph、record、ordinal 或 P6 revision。
2. 审查四条路径：整堆落地 `Move`、部分落地 `Split`、空格拾回 `Move`、占用兼容格拾回 `Merge(Quantity=0)`，均具备单一 candidate、一次 Owner save、失败恢复和 accepted projection refresh；没有 UI direct mutation、先 Split 后第二保存、预建 ItemId、预占 ordinal、actor cache 或 partial commit。
3. 审查 partial ground pickup 的 source retain/cleanup：只部分接受时同一 world root ItemId/record 保留并更新数量；归零时才一起移除；P14/P19 的其他 record 与 P8 closure 不受影响。
4. 审查 simple-stack eligibility、P17 child access、P19 complex graph 排除、P9/P11 排除、Owner/Run/Revision/placement/MaxStack/Hotbar 及稳定 SlotIndex；确认没有自动目标、Sort、Compact、world auto pickup 或输入旁路。
5. 审查 Code A 改动，确认它没有取得 ItemId、数量、Container、Run、Player、Loot、搜索、终局或世界物品的权威；floor placement 与 actor 仅为边缘适配。
6. 编译 Editor：

       "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_mapEditor Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload

7. 编译 Game：

       "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_map Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload

若编译失败，只修复本任务引入的 stack ground-drop、ground pickup、P14 transaction、shared UI、projection、include 或签名问题；若必须扩大到本任务以外，停止受影响部分并如实报告。

### 9. Report 与完成信号

生成 `Dev.D.UE.0.0.9B.P26.0.r0_report.md`，保存至：

    C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Docs\Report

Report 至少列出：

1. 实际修改／未修改文件及职责；
2. P14/P19/P24/P25 到 P26 的完整 call graph，以及 four-mode `Move / Split / Move / Merge` 对应路径；
3. simple source／target eligibility、P17 child access、P19 complex graph、P9/P11 和装备/Hotbar 排除证据；
4. 部分落地的 ItemId、Quantity、world record、ordinal 与一次 durable commit 证据；
5. ground partial Merge 的 accepted amount、world root retain/cleanup、Actor refresh 和零新 ItemId 结论；
6. P13 binding、P8 player-only closure、Owner/Run/Revision、floor placement 与 Actor 只读边缘职责；
7. 稳定 SlotIndex、空间容量、scroll、无 Sort/Compact/auto behavior 结论；
8. `Ctrl + 左键`、`Shift + 1—9`、普通 Drag、right-click、double-click、详情与滚动各自未改变的语义；
9. 两个编译命令、目标、原生 exit code 与关键结果；
10. 所有未执行的 F 阶段真实验证。

仅当四条 P26 路径静态闭合，P14/P19/P24/P25/P8/P13 边界保持，并且 Editor／Game 编译均以 native exit code `0` 完成时，使用：

    READY_FOR_P27_PLANNING

若当前范围内仍存在可修复问题，使用：

    NEEDS_P26_REWORK

若现有 P14/P6/P1 结构无法在不改变已接受的持久化或生命周期语义下原子支持部分落地或 partial ground pickup，使用：

    NEEDS_PLANNER_DECISION
    BLOCKED

完成后不得自动开始 P27、Fix 或 F。向策划 Chat 回传并附带且只附带本次同名 Report；正文首行使用：

    [CSEMI:REPORT_SENT] {"task_id":"Dev.D.UE.0.0.9B.P26.0.r0","file":"Dev.D.UE.0.0.9B.P26.0.r0_report.md"}
