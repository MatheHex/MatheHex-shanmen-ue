# Dev.D.UE.0.0.9B.P30.0.r0

## 任务身份

- 项目：`Dev.D.UE.0.0.9B`；继续使用同一活动工程，不新建项目。
- 阶段：主线 P30——已打开 `WorldDropTarget` 内完整空间道具图的受限 `Ctrl + 左键`快速拾回。
- 任务编号：`Dev.D.UE.0.0.9B.P30.0.r0`。
- 前置：已接受 `0.0.9B.P1—P29` 与 `0.0.9BFix.P1—P4`。Fix 仅用于已确认、已验收功能的缺陷修复；P30 是新增主线功能，不是 Fix。
- 执行文件：`Dev.D.UE.0.0.9B.P30.0.r0_prompt.md`。
- 报告文件：`Dev.D.UE.0.0.9B.P30.0.r0_report.md`。
- 活动工程根：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B`。
- 活动工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`。
- 任务性质：P 阶段只做实现、静态审查与代码编译。不得启动真实运行验证，也不得自动开始 P31、任意 Fix 或 F。

## 上半部分：只读项目裁决、现状与边界

### 1. 当前唯一有效基线

唯一有效依据是当前 `0.0.9B`、活动工程及已接受任务链。所有 `0.2`、`V2`、`V3`、`I`、`IPF`、历史页面壳、旧 CTA、旧库存和旧物品规则均已过时；不得读取、采用、恢复或以其决定实现、验收或范围。

Code B P1 Repository 与现有 durable Store 是唯一可变物品真值。每件物品只有一个真实实例、一个真实父位置和一条权威事务链。Widget、Cell、Presenter、DragOperation、Workspace Context、QuickTransfer resolver、WorldDrop Actor、数量预览和 Code A 适配层只能持有展示、选择、瞬时意图或只读投影；不得持有第二库存、可写数量副本、预建 ItemId、平行世界背包或 A/B 双写。

P4、`0.0.9BFix.P4` 与 P23 已确立共享 Inventory Workspace Kernel：稳定 Slot 地址、共用 Cell 与 Drag payload、统一 modifier router、动态空间容量、稳定排列、独立双栏滚动、`Ctrl + 左键` QuickTransfer 与 `Shift + 1—9` Bind。QuickTransfer 的既有 simple-stack 候选顺序是：在一个已解析的目标 container 内，先按 SlotIndex 升序寻找合法同类非满堆叠，再按 SlotIndex 升序寻找合法空格；不 Swap、不挤位、不自动装备、不自动 Split、不 Compact。

P14 以单一 `WorldDropId`、派生 world container、一个 root `ItemId` 和最小地图投影表示一个活动地面记录。P19 已经让精确 `WindTalisman` 与 `BackpackLevel1` parent，连同其唯一 P17 ChildContainer 及全部合法内容，能够作为不可拆分完整图通过真实 Drag/Drop 在 P6 与该 single-root WorldDrop 之间原子往返。P19 的 WorldDropTarget 仍是 root-only 瞬态投影，不是地面背包、不是 child-item 面板、也不是多物品地面容器。

P26—P28 已完成 simple stack 的整堆／显式数量地面拖拽、按数量拾回与部分合并。P29 已把**当前已经打开**的 P14 simple-stack WorldDropTarget 接入 `Ctrl + 左键`：world → player 使用 active P17 child 优先、BaseQuick 回退的 merge-first / empty-second；player → world 只向当前同一 root 作 `Merge(Quantity=0)`。P29 不处理 P19 complete graph，且不改变 P26—P28 的真实 Drag/Drop。

### 2. P30 产品裁决

P30 补齐同一共享工作台对 P19 已有完整空间图的快速拾回能力：当玩家**主动打开并保持一个身份有效的** P19 `WorldDropTarget` 时，按下 `Ctrl + 左键`作用于该 root Cell，可以将该完整空间图一次性移动到当前 P6 的第一个正式、可写、空的 `BaseQuick` 普通储物格。

此功能是同一 QuickTransfer 意图和同一 P3 → P2 → P1 → P14/P6 durable callback 的一个**图拓扑分支**，而不是第二个按钮、第二套 Widget、Actor 直接领取、图克隆或新背包系统。其目的只是减少用户已经确认会执行的“把地面整件空间道具拖回一个空随身格”这一重复手势；不改变 P19 的完整图语义。

完整空间图没有 normal stack 的部分数量或合并语义，因此 P30 的受限语义如下：

| Ctrl + 左键来源 | 唯一允许的快速目的地解析 | 权威事务 |
| --- | --- | --- |
| 当前已打开、身份匹配的 P14 WorldDropTarget 中的 P19 complete-graph root | 当前 P6 `BaseQuick` 内按真实 SlotIndex 升序的第一个合法空普通储物格 | 已有 P1/P19 whole-graph Move；同一 P14/P6 Owner candidate 删除该 WorldDrop record。 |
| P6 玩家侧的 P19 complete-graph root | 本任务不提供目的地；必须继续用既有 `GroundDropZone` 的真实 Drag/Drop 主动落地 | 零写入拒绝；不创建新 WorldDrop，不选择任何其他 record。 |

`BaseQuick` 是 P30 的唯一自动解析目标，原因是它是 P19 既有、稳定且不会猜测装备意图的合法 parent 落点。P30 不自动装备到 SpatialRing／Backpack，不进入任何 P17 child，不搜索 P5、P9、P11、装备位、Hotbar 或外部仓库，也不在没有空 BaseQuick 时移动、交换、挤位或重新排序。

### 3. 严格范围与持续排除

只允许以下 complete graph：

1. `Fdemo_mapItemIds::WindTalisman`，位于当前 P14 world container 的 root，且完整闭包符合 P17/P19 正式验证；
2. `Fdemo_mapItemIds::BackpackLevel1`，位于当前 P14 world container 的 root，且完整闭包符合 P17/P19 正式验证；
3. parent 可带空或非空的唯一合法 ChildContainer；ChildContainer、其中物品、数量、顺序、容量、稳定 `SpatialChildGuid` 与 provenance 都必须被视为同一不可分割图的一部分。

以下内容持续不属于 P30：

1. 任意 other-tier ring/bag、任意其他 complex parent、普通 simple stack、child item、嵌套袋、地面 child 操作、空间 parent 的部分转移、拆包、flatten、Merge、Split、Swap、stack exchange 或新 ItemId/ContainerId。
2. P29 simple-stack 方向的任何语义改写；P26/P27/P28 的显式数量 Drag/Drop；P19 的真实 Drag/Drop；P4/P23 的局内、局外仓库／战备 QuickTransfer；P9 普通容器、P11 尸体、尸体装备、P5 仓库、P8 terminal receipt、P13 Bind 产品规则、P15 Use、P17 容量图、P20/P21 来源。
3. Actor direct pickup、距离内自动拾取、右键／双击／按键领取、Take All、点击世界 Actor 后隐式移动、自动 target、自动装备、自动绑定、自动使用、自动整理、Sort、Compact、空位压缩、世界多物品容器、第二 WorldDrop inventory 或 Code A 写库存。
4. 真实产品、PIE、Standalone、鼠标键盘实测、截图、Smoke、Automation、回归、试玩、Cook、Package、网络或多人。

## 下半部分：授权执行内容

### 4. 单一授权目标

在不改变 P19 的 whole-graph Atomic Drag/Drop 路径、不创建第二物品真值且不增加自动装备的前提下，扩展既有 P4/P23/P29 `Ctrl + 左键` QuickTransfer resolver，使当前打开的 P19 complete-graph WorldDrop root 能以一次完整、原子、可回滚的 P1 graph Move 回到当前 P6 第一个合法空 `BaseQuick` 格。成功后删除同一 `WorldDropId` record 与对应 Actor projection；失败、关闭或 stale 时零写入。

### 5. 实现要求

#### 5.1 先完成活动调用链审计

改动前必须审阅并在 Report 中列出：

1. P4/P23/P29 的 `Ctrl + 左键` pointer consumption、`QuickTransferIntent`、payload、preview、commit、取消与 stale 生命周期；确认 P29 当前为何只允许 simple stack，以及 P30 如何以相同意图添加 topology gate，而非新增 input path。
2. P19/P17 的 complete-graph closure、root-only WorldDropTarget、P7 BaseQuick normal destination、`GroundDropZone` player → world whole-graph path、P1 graph Move、P14 record cleanup 与 Actor projection。
3. P1/P2/P3 的 stable source/target address、revision、accepted snapshot、single candidate、BeforeSnapshot rollback、P13 reconcile 与 P14/P6 single Owner `SaveRecord` 的既有语义。
4. `WindTalisman` 与 `BackpackLevel1` 的正式 DefinitionId、允许的 source placement、formal child type、capacity、one-layer/no-cycle/no-orphan/no-duplicate 验证及 child contents 保护。
5. 所有旧 direct pickup、Actor interaction direct mutation、right-click Take、world quantity／graph cache、UI graph copy、预建 ItemId、player/world 双写、第二保存、auto equip、child access、graph Split/Merge 以及对 P19 root 的 P29 simple-stack 分支。

#### 5.2 统一输入与 transient command

1. `UCodeBP3CellButton::NativeOnMouseButtonDown` 或当前等价共享 pointer router 仍是 `Ctrl + 左键`唯一消费点。它必须在命中后只产生一次既有 QuickTransfer transient intent 并返回 `Handled`；不得继续普通选择、搜索、drag threshold、P15 Use、right-click detail 或 Actor interaction。
2. 不得为 P30 增加 Button、hotkey、Actor click、Widget direct write 或第二个 `NativeOnMouseButtonDown` 旁路。实现应在既有 resolver 中按**已验证的 item topology**选择 P29 simple-stack 分支或 P30 P19 whole-graph 分支；Cell 显示类别、文本、图标或 Widget class 不是分支依据。
3. P30 intent 必须在 Preview 与 Commit 前重新验证 exact `OwnerId`、`RunInstanceId`、`WorldDropId`、derived world-container identity、root `ItemId`、record `Available` state、ordinal、P6 revision、workspace focus 与 target-open generation。重新打开其他 drop、关闭 panel、失焦、Actor 销毁、Run terminal／Prepared、record/root mismatch、revision stale、payload 取消或 Host 失效，均立即使 transient intent 失效并零写入。
4. 只有当前 `WorldDropTarget` 的 revealed root Cell 能作为 P30 source。Hidden、Searching、simple stack、child item、parent 不在当前 world container、装备、Hotbar 引用、P9/P11 source、无 payload 或任意 Code A object 一律不得形成 P30 intent。
5. 右键继续只读详情；普通左键继续选择；双击、Tab、I、Esc、Close、Cancel、滚动、空白区域、无 payload Drop 与 `Shift + 1—9` 不得写入 P30。P29/P19 既有语义必须保持。

#### 5.3 Whole-graph preview 与确定性落点

1. Preview 只能接受位于当前 exact P14 world container root 的 `WindTalisman` 或 `BackpackLevel1`。在任何位置写入前，复用 P17/P19 canonical validation 验证 parent DefinitionId、stable parent ItemId、唯一 ChildContainer、formal child type/capacity、stable `SpatialChildGuid`、one-layer restriction、无环、无 orphan/duplicate、child placement 与全部 child contents。
2. 不得以 parent 名称、显示图标、cell index、Actor tag、fixture、world coordinate 或历史缓存判断 graph 资格。child 可为空或非空；两种情况都必须视为完整 closure，不得因 ChildContainer 空而退化成 simple item 路径。
3. P30 唯一 candidate target 是当前 P6 `Container.Player.BaseQuick` 内，按真实 SlotIndex 升序扫描得到的第一个正式、可写、空的普通储物格。目标必须位于同一 exact active P6 图中，并通过 P1 container/slot/capacity/revision 验证。
4. 不得把下列位置当作可用空格：SpatialRing、Backpack 装备位、任何 P17 child cell、Hotbar 引用、P5/P9/P11/P14 container、保护格、错误 Owner/Run 格、stale cell、不可写 cell、显示上为空但 P1 graph 中不为空的格。
5. 没有合法 BaseQuick 空格时，拒绝且零写入；不得回退到自动装备、active child、另一 WorldDrop、空地面位置、Swap、挤位、Sort、Compact 或任何隐式 target。

#### 5.4 单一 whole-graph 事务、记录清理与回滚

1. Preview 成功后只建立一个 candidate，并且只调用 P19 已有的 P1 whole-graph Move 语义。不得用 simple `Move` 绕过 closure 验证，不得调用 Merge、Split、Quantity=0、RequestedMergeQuantity、WorldPickupDraft、PlayerSplitDraft 或重新创建 graph。
2. Commit 必须沿既有 P3 → P2 → P1 → P14/P6 durable callback。Store 在 durable write 前重新验证 command intent、opened target identity、source/target stable address、Owner、Run、revision、record/root/ordinal、P19 graph closure、BaseQuick empty state 和 active session。
3. 同一 accepted candidate 中必须保持 parent、ChildContainer 与全部 child ItemId/ContainerId、数量、placement order、capacity、provenance 的身份不变；唯一合法位置变化是 graph root 从该 derived world container 移到明确 BaseQuick target。
4. 只有 accepted snapshot 明确证明 graph root 已离开该 exact world container，才可在同一个 Owner candidate 中移除对应 P14 `WorldDrop` record、必要 empty world container 与 Actor projection。不得先删 record/Actor 再写 graph，也不得根据 UI 预览删除。
5. `NextWorldDropOrdinal` 不得写入或递增；不得创建新 record、新 ordinal、新 Actor、新 parent、新 child、新 item 或第二次保存。P13 仅按既有 accepted commit reconcile；不得自动绑定 parent 或 child，也不得恢复旧 binding。
6. candidate、graph validation、P13 reconcile、session guard、record cleanup、Actor projection 前持久化校验或 `SaveRecord` 任一失败时，必须完整恢复 BeforeSnapshot。UI、Actor 与 Cell 都不得遗留未保存 graph 状态、空格、选择或数量幻象。
7. 成功后只刷新参与的 WorldDropTarget、明确 BaseQuick target 与其 P17/P19 projection，以及对应 Actor；不得重建无关空间区、清空无关选择、重排 SlotIndex、改动 scroll offset 或清除其他物品的 transient state。

#### 5.5 与现有路径的非回归边界

1. P29 simple-stack world → player 仍按照 active P17 child 优先、BaseQuick 回退、merge-first / empty-second 运作；其 player → world 仍只允许与当前 compatible simple root 正常 `Merge(Quantity=0)`。P30 不得改变其 source、target、quantity、partial acceptance 或 record 生命周期。
2. P19 complete graph 的 player → world 仍只能由已经挂载的真实 parent Cell 拖进 `GroundDropZone::NativeOnDrop`。`Ctrl + 左键`针对玩家侧 complete graph 必须拒绝且不写入，因为当前 single-root WorldDrop 不是多物品容器，也不存在安全的合并语义。
3. P19 complete graph 的普通真实 Drag/Drop 回收仍可拖到空 BaseQuick 或匹配的空 formal equipment position；P30 只是额外的 BaseQuick 快速拾回，不得收窄、重定向或自动触发该已有路径。
4. Code A 仍只表现和转发既有 `WorldDropId` 投影／页面边缘；不得读取、缓存、写入、复制、materialize 或推断 parent/child 图，也不得自行清理/重建 Actor 以改写 Code B 真值。

### 6. 允许范围

允许以最小方式修改：

- Code B P1/P2/P3/P4/P23/P29 的既有 QuickTransfer resolver、transient command identity、preview、commit、rollback 与必要的 graph-topology guard；
- P14/P19 所在 P6 durable store、world record validation、whole-graph accepted proof、record cleanup 与 Actor projection callback，仅用于表达 P30 的 accepted whole-graph Move；
- P7/P3 UI 的 root Cell payload / current opened WorldDrop identity 注入与必要 Code A 边缘转发；Code A 仍只做投影和交互转发；
- 必要 include、声明、Build.cs、项目资料、本任务 Prompt 归档和本任务 Report。

### 7. 明确不在本任务内

- 不新建项目、版本线、Fix、第二 Repository、第二 P5/P6、第二 Warehouse、第二 WorldDrop inventory、Widget inventory、fixture、假 ItemId、clone、Code A mirror、双写、存档重置或历史数据改写。
- 不扩大到 P19 以外的 complex parent、space tier、nested container、child 地面操作、地面多物品容器、world target swap、auto equip、quick drop、player → world whole-graph QuickTransfer、exact-N graph transfer 或任何 graph Merge/Split。
- 不改变 P5/P6 bridge、M01、P8 receipt、P9/P10、P11/P12、P13 产品语义、P15、P17、P20/P21、搜索、尸体、装备、地图、敌人、战斗、生命、死亡、撤离、商店、经济、制作、网络或多人。
- 不启动产品、PIE、Standalone、真实鼠标键盘输入、截图、Smoke、Automation、回归、试玩、Cook、Package 或最终验收。
- 不在回传 Report 前自动开始 P31、任意 Fix 或 F。

### 8. P 阶段静态审查与编译

完成后只执行以下检查：

1. 审查 P30 复用唯一 `Ctrl + 左键` pointer router 与 existing QuickTransfer intent；确认没有 P30 专用输入旁路、Actor pickup、right-click Take 或 UI direct mutation。
2. 审查 P30 source 只接受 current opened exact P14 root 的两个 P19 definition，并且严格验证 complete graph closure、world record、ordinal、Owner/Run/revision 与 target-open lifecycle。
3. 审查唯一目标只为 SlotIndex 升序的合法空 BaseQuick；确认没有 auto equip、P17 child placement、Swap、Sort、Compact、implicit target 或 no-space fallback。
4. 审查 accepted candidate 只使用既有 P19 whole-graph Move，parent/child/contents 仍为不可拆分原子；确认无 Merge/Split/partial graph/clone/new ItemId/ContainerId 或 WorldDrop child list。
5. 审查 single candidate、single P14/P6 Owner save、accepted-only record cleanup、Actor refresh、BeforeSnapshot rollback、P13 no-auto-bind 与 `NextWorldDropOrdinal` 不变；同时核查 P29 simple-stack 与 P19 normal Drag/Drop 非回归。
6. 审查关闭、切换、失焦、terminal、Actor 丢失、record/root mismatch、revision stale、full BaseQuick、incompatible graph 与 SaveRecord failure 均为零写入／完整 rollback。
7. 审查 Code A 改动，确认其未取得物品图、数量、Container、Run、Player、Loot、搜索、终局或 world-drop 持久化权威。
8. 编译 Editor：

       "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_mapEditor Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload

9. 编译 Game：

       "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_map Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload

若编译失败，只修复本任务引入的 QuickTransfer、whole-graph validation、world-store、projection、input、include 或签名问题；若必须扩大到本任务以外，停止受影响部分并如实报告。

### 9. Report 与完成信号

生成 `Dev.D.UE.0.0.9B.P30.0.r0_report.md`，保存至：

    C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Docs\Report

Report 至少列出：

1. 实际修改/未修改文件及职责；
2. P4/P23/P29 QuickTransfer 到 P30 P19 root 的活动 call graph，以及没有新增 pointer/UI/Actor 写入路径的证据；
3. current opened target、Owner/Run/revision/record/root/ordinal、focus/close/stale lifecycle 与零写入结论；
4. P17/P19 complete graph closure、两个允许 DefinitionId、child empty/nonempty、one-layer/no-cycle/no-orphan/no-duplicate 的验证结论；
5. BaseQuick 唯一 candidate 的 SlotIndex 次序、空格验证、为何不存在 auto equip/child/Swap/implicit fallback；
6. P1 whole-graph Move、single candidate、single Owner save、accepted-only P14 cleanup、Actor refresh、P13 reconcile、rollback 与 ordinal 不变的证据；
7. P29 simple stack、P26—P28 explicit drag、P19 normal Drag/Drop、player → world graph drop、P8/P9/P11/P15/P17/P20/P21 与 Code A authority 的保持结论；
8. SlotIndex、动态容量、scroll、selection、无 Sort/Compact/auto behavior，以及右键、双击、Actor interaction、Shift + 1—9 未改变的语义；
9. 两个编译命令、目标、原生 exit code 与关键结果；
10. 所有未执行的 F 阶段真实验证。

仅当 P30 complete-graph QuickTransfer 静态闭合、P19/P29 非回归边界保持、Editor 与 Game 均以 native exit code 0 完成时，使用：

    READY_FOR_P31_PLANNING

若当前范围内仍有可修复问题，使用：

    NEEDS_P30_REWORK

若 P19/P14/P1 结构无法在不创建第二真值、拆分 child graph、改写 Code A 权威或扩展 single-root WorldDrop 语义的前提下支持本项，使用：

    NEEDS_PLANNER_DECISION
    BLOCKED

完成后不得自动开始 P31、Fix 或 F。向策划 Chat 回传并附带且只附带本次同名 Report；正文首行使用：

    [CSEMI:REPORT_SENT] {"task_id":"Dev.D.UE.0.0.9B.P30.0.r0","file":"Dev.D.UE.0.0.9B.P30.0.r0_report.md"}
