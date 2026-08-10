# Dev.D.UE.0.0.9B.P29.0.r0

## 任务身份

- 项目：Dev.D.UE.0.0.9B；继续使用同一活动工程，不新建项目。
- 阶段：主线 P29——已打开 WorldDrop 窗口中的 Ctrl + 左键快速转移，限 P14 simple stack。
- 任务编号：Dev.D.UE.0.0.9B.P29.0.r0。
- 前置：已接受 0.0.9B.P1—P28 与 0.0.9BFix.P1—P4。Fix 仅用于已确认、已验收功能的缺陷修复；P29 是新增主线功能，不是 Fix。
- 执行文件：Dev.D.UE.0.0.9B.P29.0.r0_prompt.md。
- 报告文件：Dev.D.UE.0.0.9B.P29.0.r0_report.md。
- 活动工程根：C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B。
- 活动工程：C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject。
- 任务性质：P 阶段只做实现、静态审查与代码编译。不得启动真实运行验证，也不得自动开始 P30、任意 Fix 或 F。

## 上半部分：只读项目裁决、现状与边界

### 1. 当前唯一有效基线

唯一有效依据是当前 0.0.9B、活动工程及已接受任务链。所有 0.2、V2、V3、I、IPF、历史页面壳、旧 CTA、旧库存和旧物品规则均已过时；不得读取、采用、恢复或以其决定实现、验收或范围。

Code B P1 Repository 与现有 durable Store 是唯一可变物品真值。每件物品只有一个真实实例、一个真实父位置和一条权威事务链。Widget、Cell、Presenter、DragOperation、Workspace Context、QuickTransfer resolver、WorldDrop Actor、数量预览和 Code A 适配层只能持有展示、选择、瞬时意图或只读投影；不得持有第二库存、可写数量副本、预建 ItemId、平行世界背包或 A/B 双写。

P4、0.0.9BFix.P4 与 P23 已确立共享 Inventory Workspace Kernel：稳定 Slot 地址、共用 Cell 与 Drag payload、统一 modifier router、动态空间容量、稳定排列、独立双栏滚动、Ctrl + 左键 QuickTransfer 与 Shift + 1—9 Bind。QuickTransfer 的既定候选顺序是：在一个已解析的目标 container 内，先按 SlotIndex 升序寻找合法同类非满堆叠，再按 SlotIndex 升序寻找合法空格；不 Swap、不挤位、不自动装备、不自动 Split、不 Compact。

P26 已让 P14 simple stack 完成玩家 ↔ 地面的正常拖拽路径：整堆/部分落地、整堆移入空玩家格和 normal Merge(0) 拾回。P27 与 P28 又分别让用户先明确数量 N 后，将 P14 simple stack 的部分数量拖入明确空玩家格或明确兼容未满玩家堆叠。P29 只为当前已经打开的同一个 WorldDropTarget 增加 Ctrl + 左键的 normal QuickTransfer；它不改变 P26 正常 Drag，也不替代 P27/P28 的显式数量意图。

### 2. P29 产品裁决

以塔科夫式双栏整理为交互参考：玩家主动打开并保持一个有效 WorldDropTarget 后，Ctrl + 左键是对当前指向物品的一次快速、可审计转移请求。它不是地面 Actor 直接拾取，不是自动拾取，不是 Take All，也不是点击世界物体后自动猜测目标。

P29 只处理当前活动 OwnerId + RunInstanceId 内的一个 P14 可用 simple stack world root 与当前 P6 玩家图之间的 normal QuickTransfer：

| Ctrl + 左键来源 | 唯一允许的快速目的地解析 | 权威事务 |
| --- | --- | --- |
| 已打开 WorldDropTarget 中的 P14 simple stack root | 当前明确激活且合法的 P17 玩家空间 child；若没有，则 P6 BaseQuick。先同类非满堆叠，再空格。 | 空格：P1 Move；兼容堆叠：P1 Merge(Quantity=0)。 |
| P6 BaseQuick 或当前已打开合法 P17 child 中的 simple stack | 当前正打开、身份匹配的同一个 P14 simple stack world root。只允许兼容未满 root。 | P1 Merge(Quantity=0)。 |

ground → player 的候选必须完全复用既有 QuickTransfer resolver 的确定性顺序。player → ground 没有第二个地面目标可供猜测：当前打开的那一个 P14 root 就是唯一候选，且仅当它与来源正式兼容、可堆叠并未满时才提交。任何无目标、非法目标、满堆叠、不兼容、stale、record 不一致、session 不一致或保存失败均为零写入拒绝。

P29 的 QuickTransfer 是 normal quantity 语义。它不得创建、消费或暗中改写 WorldPickupDraft、Player SplitDraft、RequestedMergeQuantity 或任何精确 N。用户要转移固定数量时，继续使用 P26/P27/P28 已有的确认数量后真实 Drag/Drop 路径。

### 3. 严格范围

允许的来源、目标和方向仅如下表。

| 范围 | 允许 |
| --- | --- |
| P14 simple stack world root | 无 ChildContainerId、正式 bStackable、MaxStack > 1、正 Quantity、当前 Available、record/root/WorldDropId/ordinal 与活动 P6 一致。 |
| P6 玩家侧 | BaseQuick，或当前打开、合法、可写、仍在当前 P6 图内的 P17 child 普通储物格。 |
| ground → player | 按既有 merge-first / empty-second 顺序选择一个正式合法目标。 |
| player → ground | 只向当前打开的准确 P14 root 做 normal Merge(0)。 |

以下边界持续有效且不属于 P29：

1. P19 的 WindTalisman、BackpackLevel1 及所有带 ChildContainerId 的完整空间图；它们仍只能使用既有完整图拖拽路径，Ctrl + 左键一律不为其创建地面 QuickTransfer。
2. P9 普通容器、P11 尸体、Hidden/Searching 条目、尸体装备、装备位、Hotbar 引用槽、空间 parent、世界多物品容器、未打开 child、P5 仓库和任何外部目标之间的移动。
3. WorldDrop Actor 点击直接领取、距离内自动拾取、右键领取、双击领取、按键领取、Take All、自动目标、自动放置、多物品地面容器、stack exchange、自动整理、自动装备、自动绑定、自动使用、数量截断提示或地面精确数量快捷领取。
4. P5→P6 bridge、M01、P8 terminal receipt、P13 binding 产品语义、P15 Use、P17 容量/graph、P19 closure、P20/P21 来源、地图、敌人、战斗、生命、死亡、撤离、商店、经济、制作、网络和多人。

## 下半部分：授权执行内容

### 4. 单一授权目标

在不建立第二世界库存或第二转移旁路的前提下，把当前 P14 simple stack WorldDropTarget 接入已有 P4/P23 Ctrl + 左键输入与 QuickTransfer resolver，使玩家能够在该窗口实际打开期间：

1. 将 world root 快速移入当前已激活的玩家空间 child，否则 BaseQuick；
2. 将玩家 simple stack 快速合并进该准确 world root；
3. 让两个方向均只经一次 P1 candidate、一次 P14/P6 Owner document candidate 和一次 durable SaveRecord 完成；
4. 保持未参与物品的 ItemId、ContainerId、SlotIndex、ChildContainer、视觉位置与滚动位置不变。

### 5. 实现要求

#### 5.1 先完成活动调用链审计

改动前必须审阅并在 Report 中列出：

1. P4/P23 的 Ctrl + 左键 pointer consumption、QuickTransferIntent、active destination 解析、merge-first / empty-second candidate order、preview、commit、取消与 stale 生命周期；确认现有 WorldDrop source 为什么仍被拒绝。
2. P14/P26/P27/P28 的 world record、WorldDropId、ordinal、derived world container、root identity、normal Move/Merge、partial root retain、full root cleanup、Actor refresh 与单次 Owner save。
3. P1/P2/P3 的 Move 与 Merge(Quantity=0) 的正式语义、MaxStack、partial acceptance、source retain/delete、revision、候选失败原子性与 accepted projection。
4. P13 在 BaseQuick source 完整移除或保留时的既有 reconcile；P8 player-only closure；P17 child access；P19 graph branch。
5. 所有旧 Actor direct pickup、右键 Take、WorldDrop quick-click、UI direct mutation、world quantity cache、预建 ItemId、二次保存、player/world 双写、auto pickup、空目标猜测、P19 quick path 和精确数量 draft 旁路。

#### 5.2 受限的 Ctrl + 左键输入入口

1. Ctrl + 左键必须仅在共享 workspace 已打开、UI 有焦点、exact active OwnerId + RunInstanceId 有效且当前 WorldDropTarget 仍处于打开状态时被统一 pointer router 一次消费。它不得同时触发普通选择、搜索、drag start、P15 Use、right-click detail 或 Actor interaction。
2. WorldDropTarget 必须携带且在 Preview 与 Commit 前重验 exact OwnerId、RunInstanceId、WorldDropId、record identity、world-container identity、root ItemId、ordinal、Available state 与 P6 revision。任何 target 关闭、Actor 销毁、Run terminal、重新打开其他 drop、record/root mismatch、revision stale、失焦或 workspace 关闭，均使 QuickTransfer transient intent 失效且零写入。
3. 仅 Revealed、当前真实 root Cell 可作来源。Hidden、Searching、无效、非 simple、空间 parent、child item、装备、Hotbar 引用、P9/P11 item 或无 payload 对象不得形成 P29 QuickTransfer intent。
4. 右键继续仅显示只读详情；普通 left-click 继续选择；双击、详情按钮、Tab、I、Esc、关闭、取消、失焦、滚动和无 payload Drop 均不得写入 P29。

#### 5.3 WorldDrop → Player QuickTransfer

1. 只从当前打开的 P14 simple stack root 发起。先以现有 P4/P23 policy 解析当前明确激活的合法玩家 P17 child；若没有，则使用 P6 BaseQuick。不得因为 world root 存在而自动打开、激活、装备、创建或切换玩家空间道具。
2. 在解析出的一个目标 container 内，严格按真实 SlotIndex 升序：
   - 先尝试正式同 Definition/StackKey、bStackable、无 ChildContainerId、未满、可写且兼容的已有堆叠；
   - 再尝试正式可写的空普通储物格。
   任何非法 child、装备位、Hotbar、parent、受保护或 stale cell 必须跳过；不得将其当作空格。
3. 候选为兼容堆叠时，只提交一次 P1 Merge(Quantity=0)。P1 是唯一可决定实际接受量的地方；若只接受一部分，world root 保留同一 ItemId、world container、record、WorldDropId 与 ordinal，Quantity/Actor 只从 accepted projection 刷新。
4. 候选为空格时，只提交一次 P1 Move，把原 world root ItemId 原样移入该明确空格。只有 accepted snapshot 证明 root 已离开 world container 时，才在同一个 Owner candidate 中移除 P14 record、world root 和 Actor projection。
5. 不得创建 WorldPickupDraft、不得调用 Split、不得预扣 world quantity、不得从多个玩家格试探性写入，也不得在一个 Ctrl + 左键事件内处理多个 root 或多次提交。

#### 5.4 Player → WorldDrop QuickTransfer

1. 只允许 P6 BaseQuick 或当前已打开合法 P17 child 中、无 ChildContainerId、正式可堆叠、正 Quantity 的 simple root，向当前已经打开且严格身份匹配的 P14 simple stack root 发起。
2. 当前 ground target 是唯一候选。若 source/target 的正式 Definition/StackKey、bStackable、MaxStack、graph、Owner、Run、revision、record 或 placement 任一不合格，或 target 已满，则拒绝且零写入；不得另建 WorldDrop、不得把 source 放到 GroundDropZone、不得改用空地面 placement、不得选择其他 world record。
3. 只提交一次 P1 Merge(Quantity=0)，由 P1 在正式容量内裁决完整或部分接受。source 全部被接受时，沿现有 P13 reconcile 清理被正式删除的 BaseQuick binding；source 只部分保留时，保留原 ItemId、原 SlotIndex 和原 binding。不得把 source binding 复制给 world root。
4. accepted 后 ground target 必须保留同一 root ItemId、world container、record、WorldDropId 和 ordinal，并只由 accepted projection 更新 Quantity/Actor；不得产生新 record、新 ordinal、新 Actor、新 item 或第二次保存。
5. P29 不把 player → world 的 Ctrl + 左键扩大为地面 exact quantity drop。任何 N、SplitDraft、PlayerSplit、RequestedMergeQuantity 或手工部分落地继续只能经 P26 的确认数量真实 Drag 路径。

#### 5.5 单一事务、持久化和稳定布局

1. 两个方向必须沿既有 P3 → P2 → P1 → P14/P6 durable callback。每个输入事件最多一个 accepted candidate 与一次 Owner document SaveRecord；不得先写玩家图再写 record，或先写 record/Actor 再写玩家图。
2. Store 必须在 durable write 前重新验证 command direction、source/target stable address、Owner、Run、P6 revision、P14 record/root/WorldDropId/ordinal、正式 stack metadata、MaxStack、source/target quantity 与 accepted delta。建议为两方向增加明确、可区分的结构化 delta proof；不得把任意 quantity change 宽松视为 P29。
3. ground → player Merge partial acceptance 的 delta 只能改 world source Quantity 和明确玩家 target Quantity；ground → player Move 只能改 root placement 并清理对应 P14 record；player → ground Merge 的 delta 只能改明确玩家 source Quantity/必要 source cleanup 和同一 world root Quantity。其他 ItemId、ContainerId、SlotIndex、ChildContainer、record、ordinal、scroll 与 layout 必须保持。
4. 任一 preview、candidate、record validation、P13 reconcile、session guard、Actor projection 前持久化校验或 SaveRecord 失败，必须恢复完整 BeforeSnapshot。UI 不得展示未保存数量，Actor 不得形成第二真值。
5. 成功后只刷新参与的 source、target、WorldDropTarget 摘要与对应 Actor；禁止 Sort、Compact、数组重编号、空间区重建、无关 selection 清除或无关 scroll reset。

### 6. 允许范围

允许以最小方式修改：

- Code B P1/P2/P3/P4/P23 的既有 QuickTransfer resolver、input router、preview、commit、transient lifecycle 与必要 command identity；
- P14 所在 P6 durable store、world record、root validation、Actor refresh 与 P13 reconcile，仅限表达 P29 的 normal Move/Merge accepted result；
- Ground WorldDropTarget 的已打开状态/identity 注入及必要 Code A 边缘 callback；Code A 仍只负责投影和交互转发；
- 必要 include、声明、Build.cs、项目资料、本任务 Prompt 归档和本任务 Report。

### 7. 明确不在本任务内

- 不新建项目、版本线、Fix、第二 Repository、第二 P5/P6、第二 Warehouse、第二 WorldDrop inventory、Widget inventory、fixture、假 ItemId、clone、Code A mirror、双写、存档重置或历史数据改写。
- 不改动 P5、P5→P6 bridge、M01、P8 receipt、P13 的产品规则、P15、P17、P19、P20/P21、搜索、尸体、普通容器、装备、地图、敌人、战斗、终局、经济、制作、网络或多人。
- 不实现 P19 complete graph QuickTransfer、child 地面操作、嵌套袋、世界多物品容器、ground target swap、Take All、自动拾取、Actor direct pickup、右键/双击/按键领取、自动目标、自动装备、自动绑定、自动使用、自动整理或任何精确数量快捷操作。
- 不启动产品、PIE、Standalone、真实鼠标键盘输入、截图、Smoke、Automation、回归、试玩、Cook、Package 或最终验收。
- 不在回传 Report 前自动开始 P30、任意 Fix 或 F。

### 8. P 阶段静态审查与编译

完成后只执行以下检查：

1. 审查 Ctrl + 左键只在已经打开、身份匹配的 WorldDropTarget 下形成一个 transient P29 intent；关闭、切换、stale、失焦和取消均零写入。
2. 审查 world → player 的 merge-first / empty-second 顺序、player → world 的唯一 explicit root、P1 Move/Merge(0) 复用、无自动目标、无 SplitDraft/WorldPickupDraft/RequestedMergeQuantity 旁路。
3. 审查两方向均具有单一 candidate、一次 P14/P6 Owner save、accepted projection 刷新、partial root retain/full root cleanup、P13 reconcile、P8 isolation、P17 child access 与 P19 排除。
4. 审查所有失败路径不会创建/删除错误 ItemId、record、ordinal 或 Actor，也不会产生 UI/Actor 数量缓存、二次保存、second transfer、Sort、Compact 或 scroll reset。
5. 审查 Code A 改动，确认其未取得物品图、数量、Container、Run、Player、Loot、搜索、终局或 world-drop 持久化权威。
6. 编译 Editor：

       "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_mapEditor Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload

7. 编译 Game：

       "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_map Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload

若编译失败，只修复本任务引入的 QuickTransfer、world-store、projection、input、include 或签名问题；若必须扩大到本任务以外，停止受影响部分并如实报告。

### 9. Report 与完成信号

生成 Dev.D.UE.0.0.9B.P29.0.r0_report.md，保存至：

    C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Docs\Report

Report 至少列出：

1. 实际修改/未修改文件及职责；
2. P4/P23 QuickTransfer 到 P29 WorldDropTarget 的活动 call graph，以及旧拒绝门如何被受限替换；
3. Ctrl + 左键的 pointer consumption、opened-target identity、active destination 解析、候选稳定顺序与取消/stale/关闭零写入；
4. ground → player 的 Move/Merge(0)、partial retain/full cleanup、record/ordinal/Actor 演进；
5. player → ground 的唯一 root Merge(0)、partial acceptance、P13 binding retain/cleanup 与零新 world record；
6. P1/P2/P3/P14/P6 的 single candidate、single Owner save、rollback 和结构化 delta proof；
7. P17 child、P19 complete graph、P9/P11、装备、Hotbar、P8 terminal 与 Code A authority 的排除/保持结论；
8. SlotIndex、动态容量、scroll、无 Sort/Compact/auto behavior，以及右键、双击、Actor interaction、Shift + 1—9 和 P15 未改变的语义；
9. 两个编译命令、目标、原生 exit code 与关键结果；
10. 所有未执行的 F 阶段真实验证。

仅当两个方向的 P29 QuickTransfer 静态闭合、所有排除边界保持、Editor 与 Game 均以 native exit code 0 完成时，使用：

    READY_FOR_P30_PLANNING

若当前范围内仍有可修复问题，使用：

    NEEDS_P29_REWORK

若当前 P14/P6/P1 结构无法在不改变既有持久化或生命周期语义下支持该受限 QuickTransfer，使用：

    NEEDS_PLANNER_DECISION
    BLOCKED

完成后不得自动开始 P30、Fix 或 F。向策划 Chat 回传并附带且只附带本次同名 Report；正文首行使用：

    [CSEMI:REPORT_SENT] {"task_id":"Dev.D.UE.0.0.9B.P29.0.r0","file":"Dev.D.UE.0.0.9B.P29.0.r0_report.md"}
