# Dev.D.UE.0.0.9B.P25.0.r0

## 任务身份

- 项目：`Dev.D.UE.0.0.9B`；继续使用同一活动工程，不新建项目。
- 阶段：主线 P25——统一物品工作台的显式数量部分合并。
- 任务编号：`Dev.D.UE.0.0.9B.P25.0.r0`
- 前置：已接受 `0.0.9B.P1—P24` 与 `0.0.9BFix.P1—P4`。`Fix` 仅用于已确认的已验收功能缺陷；P25 是新增主线功能，不是 Fix。
- 执行文件：`Dev.D.UE.0.0.9B.P25.0.r0_prompt.md`
- 报告文件：`Dev.D.UE.0.0.9B.P25.0.r0_report.md`
- 活动工程根：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B`
- 活动工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`
- 任务性质：新增主线功能。P 阶段仅做实现、静态审查和代码编译；不得启动 F 阶段真实验证，也不得自动开始 P26、Fix 或 F。

## 上半部分：只读项目裁决、现状与边界

### 1. 当前唯一有效基线

当前唯一有效的功能与治理基线是 `0.0.9B`、当前活动工程及其已接受任务链。历史归档、旧版本线、旧页面壳、旧 CTA、旧物品规则与任何 `0.2 / V2 / V3 / I / IPF` 文档均不作为实现、验收或范围依据。

Code B P1 Repository 与既有 durable store 是唯一可变物品真值。每一件物品只拥有一个真实实例、一个真实父位置和一条权威事务链；Widget、Presenter、Cell、DragOperation、Workspace Context、详情页和数量控件只能保存瞬时展示、选择、悬停、预览或输入意图，不能保存第二库存、可写数量副本、预建实例或位置真值。

P4、`0.0.9BFix.P4` 与 P23 已形成共享 Inventory Workspace Kernel：稳定 Slot 地址、通用 Drag payload、统一 modifier router、动态空间容量、稳定排列、`Ctrl + 左键`快速转移、`Shift + 1—9`绑定、双栏滚动，以及局内／局外页面的同一 Cell 与事务入口。P24 已接入一个 transient `SplitDraft`：可堆叠简单物品可在用户明确数量后拖到一个明确、空的普通储物格；新 ItemId 只会在权威 Split 被接受时生成。

P1 已定义并验证 Merge 的完整与部分接收语义，但 P24 有意未开放“数量拆分后合并到既有堆叠”的交互。因此 P25 只补这一项通用工作台能力；不重做 P1 Merge 引擎，也不把任一页面改成类别专用的堆叠处理器。

### 2. P25 产品裁决

P25 完成同一正式 StackKey 的显式数量部分合并，服务于塔科夫式战备／仓库整理的基础体验：位置稳定、目标明确、数量明确、没有自动整理。

在共享工作台内存在两种彼此明确且共用同一权威 Merge 事务的动作：

| 动作 | 用户输入 | 接受后的数量语义 |
| --- | --- | --- |
| 普通全堆 Drag 到兼容未满堆叠 | 既有 full-stack drag | 复用 P1 既有 Merge；目标最多接收到其正式剩余容量，来源若有余量则保留原 ItemId 与余量，若归零才沿现有语义移除。 |
| 已确认的 SplitDraft Drag 到兼容未满堆叠 | 用户先明确输入 `N`，再明确 Drop | 这是精确数量 Merge。`N` 必须完全放得下；目标增加恰好 `N`，来源减少恰好 `N`。不足以容纳 `N` 时拒绝，绝不悄悄改成较小数量。 |

P25 不创建中间 split item 后再 Merge，也不为精确数量 Merge 新建 ItemId：来源和目标均保留各自原有 ItemId。普通 full-stack Drag、`Ctrl + 左键`QuickTransfer、`Shift + 1—9`绑定、装备、空间图、搜索、世界掉落和终局继续沿用现有语义。

### 3. Scope 与持续有效边界

P25 只覆盖 P4/P23/P24 已打开的共享工作台范围：

| Scope | 合并位置 |
| --- | --- |
| `OutOfRaidP5` | 玩家装备外的普通储物格、BaseQuick、当前已打开合法空间 child 与仓库普通储物格 |
| `InRunPlayer` | P6 BaseQuick 与当前已打开合法空间 child |
| `ExternalTarget` | 已 Open 且 Revealed 的 P9 普通容器与 P11 尸体容器普通储物格 |

以下规则持续生效：

1. 只有 P1/P2/P3/P4 的既有权威事务与对应 durable callback 可修改数量、实例、位置或 Revision。P5、P6、P9、P11 的跨图事务必须沿其既有的一次性原子提交边界完成，不能先写来源、后写目标。
2. Merge compatibility 必须以现有 P1 的正式 definition／stack signature／标签和 MaxStack 判定为准，不得从显示名称、图标、Widget 文本或硬编码类型猜测。
3. 空间 parent、任何有 `ChildContainerId` 的完整图、装备类、非 stackable item、Hidden／Searching 外部 item、未 Open target、WorldDrop、装备位、Hotbar 引用槽和 UI 外区域均不在 P25 合并目标或来源范围内。
4. P5↔P6 bridge、StartAttempt、M01、P8 terminal receipt、P13 binding lifecycle、P15 Use、P17 graph、P19 WorldDrop、P20/P21 来源、Code A 的 Run／地图／玩家／战斗／搜索权威均保持原语义。

## 下半部分：授权执行内容

### 4. 单一授权目标

将 P24 的已确认 `SplitDraft` 接入 P1 的现有 Merge 事务，使用户可把一段明确数量拖到一个明确、兼容、未满的既有堆叠；同时核实并收敛普通 full-stack Drag 到该堆叠时的部分接收投影与事务路径。所有 P5、P6、P9、P11 页面必须复用同一数量意图、preview、commit、revision gate 与 durable callback，不得复制 P5 merge、P6 merge、普通容器 merge 或尸体 merge handler。

任一次成功的精确数量 Merge 必须满足：

1. 来源 `ItemId` 和目标 `ItemId` 均保持不变；不生成、clone、预建或删除任何无关 ItemId。
2. 来源 Quantity 减少恰好 `N`，目标 Quantity 增加恰好 `N`，且目标不超过其正式 `MaxStack`。
3. 来源、目标、所有参与的 durable record、Hotbar reconcile 与 Revision 在一个权威逻辑事务中共同接受；任一验证、revision、状态、容量、身份或保存失败时，所有图、数量、位置与 binding 均保持旧值。
4. 除来源和明确目标的 Quantity／必要 source removal 外，任何其他 ItemId、`ContainerId`、`SlotIndex`、ChildContainer、视觉位置和 scroll offset 不改变。

### 5. 实现要求

#### 5.1 先完成静态路径审计

改动前必须审阅并在 Report 中列出：

1. P1/P2 的 Merge 请求字段、full／partial 接收、MaxStack、source cleanup、ItemId 生命周期、Revision 和失败原子性；若 P1 已有精确数量 Merge 入口，复用它；若没有，仅以最小方式扩展该唯一入口。
2. P4/P23/P24 的共享 Cell、drag payload、preview／commit、pointer modifier router、stable address、`SplitDraft`、数量输入、取消／stale 清理和 workspace context。
3. P5 内图、P6 内图、P6↔P9 与 P6↔P11 现有 Merge 的 transaction facade、composite revision、durable callback 和保存边界。
4. 当前 Definition／stack signature／MaxStack／数量显示、QuickTransfer 的既有 Merge 行为，以及 P13 reconcile 在来源保留或来源清空时的调用点。
5. 任何旧的 Widget 直接改数量、先 Split 后 Merge、自动把 Draft 降到剩余容量、类别专用 merge handler、按显示文本匹配、自动 target、自动 compact 或第二数量真值入口。

已有组件必须继续收敛到共享内核；不得因为 P5、P6、P9 或 P11 的持久化结构不同而复制 UI／事务规则。

#### 5.2 共享精确数量 Merge Intent

1. 在 P24 的 `SplitDraft`／共享 drag payload 语义内添加最小的 `RequestedMergeQuantity`／等价只读数量意图。它最多表达当前已确认 Draft 的来源身份、expected revision、明确目标 stable address 与用户确认的 `N`；不得含新 ItemId、可写 item copy、预扣数量或预改 target quantity。
2. 已确认 Draft 只有在用户把它 Drop 到一个明确、可见、可写、普通储物、非同源、同一正式 stack signature、未满且剩余容量至少为 `N` 的 target cell 时，才产生精确数量 Merge intent。
3. 若 target 已满、不兼容、Hidden／Searching、装备位、Hotbar 引用槽、非法 child、未 Open 外部 target、UI 外区域、revision stale 或剩余容量小于 `N`，preview 必须拒绝，Drop 零写入；不得自动将 `N` 截断为剩余容量，也不得临时新建 split stack。
4. 精确数量 Merge 复用 P1 的 candidate-state 验证与单一提交。若必须扩展请求字段，只允许让权威 P1 Merge 在接受时使用 `N`；严禁 UI 先调用 Split、再调用 Merge，或在两次保存之间制造短暂的第二实例。
5. `SplitDraft` 的原有资格继续有效：来源必须可堆叠、Quantity 大于 1、`1 <= N < SourceQuantity`、可写普通储物、无 `ChildContainerId`。普通 full-stack Drag 不创建或消费 SplitDraft。

#### 5.3 普通 full-stack Drag 的部分接收收敛

1. 审核 P4 的普通 Drag 到兼容未满 target 是否已完整复用 P1 Merge。若底层已支持 partial acceptance，只修复共享 preview、结果投影、stale handling 或回调中缺失的收敛，不重写其产品语义。
2. 普通 full-stack Drag 的 accepted 数量仍由 P1 正式容量裁决：target 最多增长到 `MaxStack`；来源存在余量时保留原 `ItemId`、原 stable cell 和剩余 Quantity；来源归零时沿 P1 既有清理语义释放该 source cell。不得通过自动 Split、自动重排、target swap 或新增 ItemId 达成这一结果。
3. Preview 可从当前权威只读 snapshot 计算“完整接收”或“部分接收”的视觉提示，但只能是 transient 展示。最终数量、source removal、Revision 和可见 projection 必须只采用 accepted result 后重新读取的权威图。
4. `Ctrl + 左键`保持 P23 的 QuickTransfer 候选顺序和完整堆语义；它不弹数量框、不创建 Draft、不调用新的第二条 merge transaction。仅允许它复用同一已收敛的 P1 Merge 结果处理。

#### 5.4 P5、P6、P9、P11 原子提交与 Hotbar

1. P5 内图与 P6 内图继续各自在其现有 Repository／durable snapshot 的一次 accepted commit 中完成。P6↔P9 与 P6↔P11 必须继续使用既有 composite repository 和单一 Owner document candidate；不得拆为两个保存或两条独立 revision。
2. 跨图精确数量 Merge 必须在保存前完整验证 OwnerId、精确 RunInstanceId、target identity、Open／Revealed gate、source／target revisions、stack signature、MaxStack、`N`、placement、capacity 与所有 scope identity；失败时 P6、外部 target、visibility、session 和所有 Revision 均不变。
3. 若 BaseQuick 来源在 accepted 后仍有 Quantity，保留其同一 P13 binding；若完整 full-stack Merge 导致其 source ItemId 被 P1 正式清除，则沿既有 repository-level reconcile 清理该 binding。目标 ItemId 不继承来源 binding，新 Merge 不自动绑定、不使用、不改变其他 binding。
4. P25 不改变 P5→P6 bridge 或 P8 Extracted 对 Merge 后最终权威 graph 的读取规则；不得以 Draft、preview 或 stale UI 数量重建持久数量。

#### 5.5 稳定布局、动态容量与输入

1. 所有容器继续按正式 Capacity 和稳定 `SlotIndex` 投影。精确数量 Merge 成功后只刷新来源与明确 target 的数量；full-stack source 归零时仅该 source slot 变 Empty。禁止 Sort、Compact、occupied-only AddChild、数组重编号、空间区重建或无关 scroll reset。
2. 空间 parent 与其 ChildContainer 永远作为完整图移动；P25 不改变空间品级容量、child layout、来源、child contents、完整图 Move/Drop／bridge／terminal 语义。
3. 普通左键仍只选择；right-click 仍只读详情；double-click、详情开关、滚动、I、Tab、Esc、关闭、无 payload Drop 与 `Shift + 1—9`不得直接写数量或位置。Esc、取消、blur、关闭、gate 变只读、来源／target stale、身份变化与另一项 drag 必须清理 Draft／preview／数量输入，零写入。
4. P5、玩家、普通容器和尸体容器均使用相同的 Cell、payload 与输入路由；scope policy 只能拒绝不合法目标，不能以物品种类切换另一套交互实现。

### 6. 允许范围

允许最小修改：

- Code B P1/P2/P3 的既有 Merge request、candidate validation、accepted result、事务与 callback；
- P4/P23/P24 的共享 workspace context、SplitDraft、drag payload、preview／commit、详情数量控件、projection refresh、stable cell 与统一输入路由；
- P5/P6/P9/P11 的既有 Merge facade、revision gate、durable callback 与 P13 reconcile 接入；
- 仅为 UI 生命周期、当前有效 Run／Target identity 或结构化拒绝传递所需的最小 Code A 边缘层；
- 必要 include、声明、Build.cs、项目资料与本任务 Report。

### 7. 明确不在本任务内

- 不新建项目、版本线、Fix、第二 Repository、第二 Warehouse、Widget inventory、fixture、假 ItemId、Code A mirror、A/B 双写或存档重置。
- 不改动 P5→P6 bridge、StartAttempt、M01、P8 receipt、P13 binding 的产品语义、P15 Use、P17 graph、P19 WorldDrop、P20/P21 来源、战斗、生命、死亡、撤离、搜索来源、敌人、地图、商店、经济、制作、网络或多人。
- 不把 Merge／Split 接入 P14 WorldDrop、世界 Actor、地面拾取、随机地面 Loot、装备位、空间 parent、child 操作或任何自动拾取路径。
- 不实现多格物品、旋转、网格占地、stack exchange、自动整理、自动 target、Take All、按比例自动分割、右键／双击快速移动、Alt 自动装备、自动绑定、自动使用、额外 1—9 效果、分类筛选或仓库搜索工具。
- 不启动产品、PIE、Standalone、真实鼠标键盘输入、截图、Smoke、Automation、回归、试玩、Cook、Package 或最终验收。
- 不未经回收本 Report 自动开始 P26、任意 Fix 或 F 阶段。

### 8. P 阶段静态审查与编译

完成后只执行下列检查：

1. 审查精确数量 Merge 的 Intent／Draft／payload 全部是 transient，且任何确认前、取消、失焦、关闭、stale、gate 拒绝、target 不足或保存失败都不写 P1/P5/P6/P9/P11。
2. 审查唯一提交路径：精确数量 Merge 与 full-stack partial acceptance 均经共享 P4/P3/P2/P1 Merge；没有 UI direct mutation、先 Split 后 Merge、预建 ItemId、自动截断数量、类别专用 handler、先扣来源后验目标或部分 commit。
3. 审查 P5、P6、P6↔P9、P6↔P11 的 scope isolation、owner／Run／target identity、revision、MaxStack、Hotbar、stable SlotIndex、ChildContainer、P5↔P6 与 P8 边界；确认无 sort／compact／auto behavior。
4. 审查输入优先级：`Ctrl + 左键`仍为 QuickTransfer，`Shift + 1—9`仍为 Bind，right-click／double-click／details／scroll／close 不写入；只有明确 Drop 的 normal Merge 或确认后的精确数量 Merge 可写入。
5. 审查 Code A 改动，确认它没有取得物品图、数量、Container、Run、Player、Loot、搜索、结算或旧库存权威。
6. 编译 Editor：

       "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_mapEditor Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload

7. 编译 Game：

       "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_map Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload

若编译失败，只修复本任务引入的 Merge、共享 UI、跨图 transaction、projection、输入、include 或签名问题；若必须扩大到本任务以外，停止受影响部分并如实报告。

### 9. Report 与完成信号

生成 `Dev.D.UE.0.0.9B.P25.0.r0_report.md`，保存至：

    C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Docs\Report

Report 至少列出：

1. 实际修改／未修改文件及职责；
2. P1 Merge 到 P4/P23/P24 shared workspace 的完整 call graph；
3. 精确数量 Merge Intent、数量／target 校验、取消／stale／关闭清理和无中间 ItemId 证据；
4. full-stack Drag 的部分接收、source retain／cleanup、target retain 与权威 projection 结论；
5. P5 内图、P6 内图、P6↔P9、P6↔P11 的原子 commit 边界和 revision 演进；
6. StackKey／MaxStack／capacity、空间 parent 拒绝、Hotbar reconcile 与稳定 SlotIndex 结论；
7. `Ctrl + 左键`、`Shift + 1—9`、普通 Drag、right-click、double-click、详情和滚动各自未改变的语义；
8. WorldDrop 与全部明确未实现范围；
9. 两个编译命令、目标、原生 exit code 与关键结果；
10. 所有未执行的 F 阶段真实验证。

仅当共享精确数量 Merge 与普通 full-stack partial acceptance 均静态闭合，且 Editor／Game 编译均以 native exit code `0` 完成时，使用：

    READY_FOR_P26_PLANNING

若当前范围内仍存在可修复问题，使用：

    NEEDS_P25_REWORK

若现有 P1/P5/P6/P9/P11 结构无法在不改变已接受的持久化或生命周期语义下原子支持精确数量 Merge，使用：

    NEEDS_PLANNER_DECISION
    BLOCKED

完成后不得自动开始 P26、Fix 或 F。向策划 Chat 回传并附带且只附带本次同名 Report；正文首行使用：

    [CSEMI:REPORT_SENT] {"task_id":"Dev.D.UE.0.0.9B.P25.0.r0","file":"Dev.D.UE.0.0.9B.P25.0.r0_report.md"}
