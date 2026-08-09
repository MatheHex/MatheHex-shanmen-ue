# Dev.D.UE.0.0.9B.P24.0.r0

## 任务身份

- 项目：`Dev.D.UE.0.0.9B`；继续使用同一活动工程，不新建项目。
- 阶段：主线 P24——统一物品工作台的数量拆分与明确部分转移。
- 任务编号：`Dev.D.UE.0.0.9B.P24.0.r0`
- 前置：已接受 `0.0.9B.P1—P21`、`0.0.9BFix.P1—P4` 与 `0.0.9B.P23.0.r0`。`Fix` 仅用于已确认功能缺陷；P24 是新的主线功能，不是 Fix。
- 执行文件：`Dev.D.UE.0.0.9B.P24.0.r0_prompt.md`
- 报告文件：`Dev.D.UE.0.0.9B.P24.0.r0_report.md`
- 活动工程根：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B`
- 活动工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`
- 任务性质：新增主线功能；P 阶段仅做实现、静态审查与代码编译，不启动 F 阶段真实验证，也不自动开始 P25。

## 上半部分：只读项目裁决、现状与边界

### 1. 当前有效基线

当前唯一有效的功能与治理基线是 `0.0.9B` 及其已接受任务链。本任务只在该工程和该链内工作；历史归档、废止页面壳、旧 CTA、旧物品规则与旧版本线均不作为实现或验收依据。

P1 已提供物品的唯一实例、唯一父位置、Revision 与 `Move / Swap / Merge / Split / Equip / Unequip` 原子事务。P4 与 `0.0.9BFix.P4` 已形成共享 Inventory Workspace Kernel：稳定槽位地址、通用 Drag payload、统一 modifier router、动态空间容量、稳定排列、`Ctrl + 左键`快速转移、`Shift + 1—9`绑定和双栏滚动。P23 已将 P5 宗门仓库／战备接入同一内核。

当前仍缺少的是一个正式、可复用的“明确数量拆分 → 明确落点”入口。它不能让不同道具类别、局外页面、局内背包、普通容器或尸体容器各自维护一套分堆逻辑，也不能把数量修改留在 Widget 内存中。

### 2. P24 产品裁决

P24 只完成可堆叠简单物品的部分转移。玩家先在共享详情区域明确发起 Split，输入一个合法数量，再把该数量的临时 split payload 拖到一个明确、合法且为空的目标格。只有最终 Drop 被权威事务接受时，来源堆叠才减少，新的唯一 ItemId 才在目标格出现。

它是塔科夫式整理的结构补全：物品位置保持稳定，分堆始终是用户显式选择数量与目标的动作，而不是自动整理、自动分配或类型专用旁路。普通全堆 Drag、`Ctrl + 左键`快速转移和 `Shift + 1—9`绑定继续沿用既有语义。

P24 覆盖已存在于 P4 共享工作台中的以下 scope：

| Scope | 可作为 Split 来源／目标的区域 |
| --- | --- |
| `OutOfRaidP5` | 玩家装备外的普通储物格、BaseQuick、已打开的空间 child 与仓库普通储物格 |
| `InRunPlayer` | P6 的 BaseQuick 与当前已打开、合法的空间 child |
| `ExternalTarget` | 已 Open 且 Revealed 的 P9 普通容器与 P11 尸体容器普通储物格 |

现有 `WorldDrop` 记录及其地面生命周期不在本任务内。P24 不把“从堆叠中拆出一部分并生成第二个地面 root”偷偷扩展到 P14；该路径如有需要，在以后作为独立主线功能规划。

### 3. 持续有效的权威边界

1. Code B P1 Repository 和既有 owner durable transaction 是唯一可变物品真值。Widget、Presenter、Cell、详情页、数量对话框、DragOperation 和输入 adapter 只可持有 transient UI 状态，不能保存可写数量、预创建 ItemId、第二库存或 placement 副本。
2. P5 是局外持久物品图；P6 是精确 `OwnerId + RunInstanceId` 的活动 Run 图；P9 与 P11 是各自 Run-local 外部容器图。跨图 Split 必须沿它们已存在的原子提交边界进行，不能先扣来源、后建目标。
3. P5↔P6 bridge、Prepared receipt、RecoveredAbandon rebind、P8 terminal receipt、Extracted／Dead／RecoveredAbandon 处理、P13 binding lifecycle、P15 Use、P17 空间图、P19 WorldDrop、P20/P21 来源与 Code A 的 Run／地图／Player／战斗权威均保持原语义。
4. 空间戒指、吞天袋或任何带 ChildContainer 的 parent graph 是不可拆分整体；它们不是 stackable split 源，也不是通过 P24 被局部复制、flatten、拆 child 或新建 child 的对象。

---

## 下半部分：授权执行内容

### 4. 单一授权目标

将 P1 已有的 Split 原子事务以一个共享 `SplitDraft`／等价 transient 工作流接入 P4/P23 的统一 Inventory Workspace Kernel，使符合资格的 stackable item 在 P5、P6、P9 与 P11 的现有页面中以相同规则完成“选择数量 + 明确 Drag/Drop 落点”的部分转移。

任一次成功操作都应满足：

1. 来源保留原 `ItemId`，且 Quantity 减少为 `旧数量 - SplitQuantity`；
2. 目标生成一个新的唯一 `ItemId`，Quantity 正好为 `SplitQuantity`；
3. 来源、目标、所有相关 durable record 与 Revision 在一个权威逻辑事务中共同提交；
4. 任何验证失败、取消、stale revision、保存失败或页面关闭都保持旧图、旧数量、旧 Revision 与旧 binding；
5. 除来源格和明确目标格外，其他物品的 `ContainerId`、`SlotIndex` 与视觉位置不改变。

### 5. 实现要求

#### 5.1 先完成静态路径审计

在改动前审阅并在 Report 中列出：

1. P1/P2 中 Split 的请求、数量验证、新 ItemId 创建、Revision 与失败原子性路径；
2. P4/P23 的共享 Cell、详情区域、pointer modifier router、DragOperation、preview/commit、stable address 和 workspace context；
3. P5 内图与 P6↔P9、P6↔P11 跨图 transaction 的现有 commit 边界；
4. 当前可堆叠 Definition、MaxStack、数量显示和已存在的 Merge 行为；
5. 现有 Hotbar binding reconcile 在数量变化、完整 Merge、P5↔P6 与 P8 生命周期中的调用点；
6. 任何旧 Split Dialog、Widget direct mutation、右键／双击分堆、自动目标、固定数量或按物品类型分叉的遗留入口。

已有可复用组件应继续收敛到共享内核；不要复制一套 P5 split、P6 split、容器 split 或尸体 split handler。

#### 5.2 Shared Split Draft 与明确数量输入

1. 在现有 `InventoryWorkspaceContext` 或等价 transient 层新增一个最多一个活动项的 `SplitDraft`。它至少包含：scope、OwnerId、可选 RunInstanceId、来源 `ItemId`、来源 stable address、expected revision、用户请求的 `SplitQuantity` 和必要的 graph/target identity；它不含新 ItemId、可写 item copy、预先扣减后的 Quantity 或预创建 placement。
2. 详情区域只在当前 selection/hover 指向可拆分物品时显示明确 `Split` 动作。点击该动作只打开数量输入状态，不改变 P1/P5/P6/P9/P11。
3. 合法数量必须严格位于 `1 … Quantity - 1`。仅当 Definition 明确为 stackable、当前 Quantity 大于 1、来源可见且可写、Container 状态有效、Coordinator gate 允许且物品没有 ChildContainer 时，才允许确认 SplitDraft。
4. 数量输入可以是现有对话框的复用或最小的共享数量控件；确认后的 UI 应显示明确的“拆出 N”拖拽意图。取消、Esc、失焦、关闭页面、Coordinator 变为只读、外部容器转为不可编辑、revision stale 或来源消失时，只清除 Draft/preview，不产生写入。
5. 同一 Workspace 同时只能有一个 SplitDraft；创建新 Draft 前先干净取消旧 Draft。页面刷新可重新读取权威 projection，但不能以 UI 缓存恢复一个已失效的 Draft。

#### 5.3 明确 Drop 与统一事务

1. 确认的 SplitDraft 应创建 P4 共享的 split drag payload／等价 payload variant。它携带现有身份、稳定地址、expected revision 和数量意图；不得创建新 ItemId，也不得把普通 full-stack Drag 改成 Split。
2. Split payload 只能落到一个明确、合法、为空的普通储物 cell。装备位、Hotbar 引用槽、隐藏／搜索中 item、已占用格、同源格、无效 child、外部未 Open 目标和 UI 外区域均被拒绝且零写入。
3. 对 P5 内图，Drop 复用现有 P5 P3/P2/P1 transaction 和 durable commit。对 P6↔P9、P6↔P11，增加最小的跨图 Split adapter，使来源数量扣减与目标新实例创建在已有双方图的单一原子提交中完成。任何一侧 revision、capacity、状态、身份或保存失败时，双方均保留旧状态。
4. 普通 Drag 的 Move／Swap／Merge 与 P23 的 `Ctrl + 左键` QuickTransfer 不得隐式改为 Split。`Ctrl + 左键`优先作为既有快速转移；它不打开数量框、不拆分。普通左键只选择；右键、双击、详情开关、滚动、I、Tab、Esc、关闭、无 payload Drop 与 `Shift + 1—9`均不得直接产生物品位置或数量写入。
5. P24 不引入自动 target、自动 Merge、自动 Compact、Take All、自动 Equip/Unequip、自动绑定、自动使用或按 Definition 的特殊移动规则。

#### 5.4 图完整性、空间道具与快捷栏

1. `WindTalisman`、`BackpackLevel1`、任何有 `ChildContainerId` 的 item、所有装备类物品、非 stackable item、Quantity 为 1 的 stack 与不符合当前 scope gate 的 item 均不显示 Split，或在入口处返回清晰零写入拒绝。
2. P24 不能通过数量拆分移动、复制、重排、删除或新建任何 ChildContainer。空间 parent 与其完整 child graph 继续走既有完整图 Move/Drop/bridge/terminal 路径。
3. 若被拆来源是一个仍合法绑定在 P13 快捷栏的 BaseQuick stack，来源 `ItemId` 未离开 BaseQuick 时 binding 应保持有效；新生成的 target `ItemId` 不自动绑定。若一次合法事务使原 binding 不再有效，沿既有 repository-level reconcile 在同一 durable commit 中清理，不建立第二份 binding 真值。
4. P5→P6 bridge 与 P8 Extracted 必须保留 Split 后的最终权威 graph；不得用 P6 初始 receipt、预拆 UI 状态或旧容量重建数量/ItemId。

#### 5.5 稳定布局、动态容量与双栏行为

1. 所有容器继续按权威 `Capacity` 和稳定 `SlotIndex` 渲染。成功 Split 后，只让来源 cell 的数量和明确 target cell 的新 Item projection 变化；不执行 Sort、Compact、occupied-only AddChild、数组重编号或空间区重建。
2. 当前装备／卸下、绑定、SplitDraft 进入/取消、数量对话框开关、refresh、Coordinator state 或详情切换不得改变无关物品位置，也不得重置两个 pane 各自的 scroll offset。
3. P5 的动态空间容量以及 P6 已装备空间戒指／吞天袋的完整格位继续根据正式 parent Definition 和 ChildContainer record 解析；P24 不改变品级容量数据、空间道具来源或渲染语义。

### 6. 允许范围

允许最小修改：

- Code B P1/P2 的现有 Split 请求／验证／结果表达，以及为 P6↔P9、P6↔P11 原子 Split 所需的窄 adapter；
- P4/P23 的共享 workspace context、详情区域、数量控件、drag payload、preview/commit、projection refresh、稳定 cell 和统一输入路由；
- P5/P6/P9/P11 的现有 transaction facade、revision gate、durable callback 与 P13 reconcile 接入；
- 为 UI 生命周期、焦点或当前有效 Run/Target identity 传递所需的最小 Code A 边缘层；
- 必要的 include、声明、Build.cs、项目资料与本任务 Report。

### 7. 明确不在本任务内

- 不新建项目、版本线、Fix、第二 Repository、第二 Warehouse、Widget inventory、fixture、假 ItemId、Code A mirror 或 A/B 双写。
- 不改变 P5→P6 bridge、StartAttempt、M01、P8 terminal receipt、P13 binding 产品语义、P15 Use、P17 graph、P19 WorldDrop、P20/P21 来源或任何已接受的物品历史。
- 不将 Split 接入 P14 WorldDrop、世界 Actor、地面拾取、随机地面 Loot、地图、敌人、搜索定义、尸体来源、装备数值、战斗、生命、死亡、撤离、商店、经济、制作、网络或多人。
- 不实现多格物品、旋转、网格占地、按比例自动分割、partial merge、堆叠交换、自动整理、自动拾取、Take All、右键/双击快速移动、Alt 自动装备、自动绑定、自动使用或新的 1—9 效果。
- 不启动产品、PIE、Standalone、真实鼠标键盘输入、截图、Smoke、自动化、回归、试玩、Cook、Package 或最终验收。
- 不未经回收本 Report 自动开始 P25、任意 Fix 或 F 阶段。

### 8. P 阶段静态审查与编译

完成后只执行以下检查：

1. 审查 SplitDraft 与数量输入，确认它们均为 transient，且确认、取消、关闭、stale、身份失配和 gate 拒绝均没有 P1/P5/P6/P9/P11 写入。
2. 审查唯一提交路径，确认所有被接受的 P5 内图或 P6↔P9/P11 Split 都经共享 P4/P3/P2/P1 事务；没有 Widget direct mutation、类型专用 handler、新 ItemId 预创建、先扣来源后验证目标、部分 commit 或第二图真值。
3. 审查容量、数量、MaxStack、stable SlotIndex、ChildContainer、Hotbar binding、P5↔P6、P8 与所有 unchanged boundaries；确认 no sort/no compact/no auto behavior，空间 parent 不可 Split。
4. 审查输入优先级：`Ctrl + 左键`仍为 QuickTransfer，`Shift + 1—9`仍为 Bind，right-click/double-click/details/scroll/close 不写入，只有确认后的 Split payload 落到明确 cell 可产生数量与位置变化。
5. 审查 P5/P6/P9/P11 scope isolation 和 Code A 改动，确认 Code A 没有取得物品图、数量、container、Run、Player、Loot、搜索、结算或旧库存权威。
6. 编译 Editor：

       "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_mapEditor Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload

7. 编译 Game：

       "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_map Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload

若编译失败，只修复本任务引入的 Split、共享 UI、跨图 transaction、projection、输入、include 或签名问题；若必须扩大到本任务以外，停止受影响部分并如实报告。

### 9. Report 与完成信号

生成 `Dev.D.UE.0.0.9B.P24.0.r0_report.md`，保存至：

    C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Docs\Report

Report 至少列出：

1. 实际修改／未修改文件及职责；
2. P1 Split 到 P4/P23 shared workspace 的完整 call graph；
3. SplitDraft、数量输入、取消／stale／关闭清理和无预创建 ItemId 证据；
4. P5 内图、P6↔P9、P6↔P11 的原子 Split 提交边界与 revision 演进；
5. stackable/quantity/capacity 资格、空间 parent 拒绝、Hotbar binding reconcile 与稳定 SlotIndex 结论；
6. `Ctrl + 左键`、`Shift + 1—9`、普通 Drag、右键、双击、详情和滚动各自未被改变的语义；
7. WorldDrop 与其他明确未实现范围；
8. 两个编译命令、目标、原生 exit code 与关键结果；
9. 所有未执行的 F 阶段真实验证。

仅当共享 Split 功能静态闭合，且 Editor／Game 编译均以 native exit code `0` 完成时，使用：

    READY_FOR_P25_PLANNING

若当前范围内仍存在可修复问题，使用：

    NEEDS_P24_REWORK

若现有 P1/P5/P6/P9/P11 结构无法在不改变已接受的持久化或生命周期语义下原子支持 Split，使用：

    NEEDS_PLANNER_DECISION
    BLOCKED

完成后不得自动开始 P25、Fix 或 F。向策划 Chat 回传并附带且只附带本次同名 Report；正文首行使用：

    [CSEMI:REPORT_SENT] {"task_id":"Dev.D.UE.0.0.9B.P24.0.r0","file":"Dev.D.UE.0.0.9B.P24.0.r0_report.md"}
