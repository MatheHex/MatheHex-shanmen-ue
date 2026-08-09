# Dev.D.UE.0.0.9B.P14.0.r0

## 任务身份

- 项目：Dev.D.UE.0.0.9B
- 阶段：P14——单一局内地面丢弃／拾回纵向切片
- 任务编号：Dev.D.UE.0.0.9B.P14.0.r0
- 任务性质：在 P7 已提供活动 Run 玩家背包、P8 已提供 Code A 终局之后的归还／没收、P10/P12 已提供容器／尸体与 P6 间的真实拖拽转移、P13 已提供 1—9 快捷栏引用生命周期后，为一件由玩家主动丢到地面的简单物品建立唯一 Run-local 真值、Code A 世界表现投影、重新开启页面与真实拖拽拾回。本任务不实现随机地面 Loot、自动拾取、物品使用、消耗效果、装备／空间道具整体掉落或任何 Code A 物品权威。P 阶段只做功能开发、静态代码审查、Editor 代码编译和必要的最小编译修正；真实运行、CTA、自动化、回归、截图、Smoke、Game Build、Cook、Package 与最终验证均留给 0.0.9B.F。
- 执行文件：Dev.D.UE.0.0.9B.P14.0.r0_prompt.md
- 报告文件：Dev.D.UE.0.0.9B.P14.0.r0_report.md
- 活动工程根：C:\\AIDev\\shanmen-ue\\Dev.D.UE.0.0.9B
- 活动工程：C:\\AIDev\\shanmen-ue\\Dev.D.UE.0.0.9B\\demo_map.uproject
- 引擎：C:\\Program Files\\Epic Games\\UE_5.8
- 工程级开发基线：C:\\AIDev\\shanmen-ue\\Dev.D.UE.0.0.9-XFix1
- 已接受前置：P4x.0.r2、P5、P6.0.r4、P7.0.r0、P8.0.r0、P9.0.r1、P10.0.r0、P11.0.r0、P12.0.r0 与 P13.0.r0。

---

# 上半部分：只读项目裁决、现状与边界

## 1. P／F 阶段分界

| 阶段 | 负责内容 |
| --- | --- |
| P | 功能开发、必要的静态代码审查、目标代码编译与最小编译修正。 |
| F | 真实运行测试、CTA／wrapper、自动化与回归、截图／可见验收、Smoke、Game Build、Cook、Package 和最终验证。 |

本任务不得为了证明 P14 而启动产品、运行测试、编写或执行测试专用路径、采集截图、执行回归、Smoke、Game target 编译、Cook 或封包。一次 Editor 代码编译是本任务唯一要求的执行性检查，不替代 F 的真实验证。

## 2. 既有真值与所有权

1. Code A 继续拥有默认地图、Player Actor、原始输入派发、战斗、生命／伤害、地图中 Actor 的即时生命周期、正式 Run 生命周期、撤离／死亡分类、HUD、Run Save、旧库存、旧 Loot／搜索和所有尚未迁移的运行时。P14 的 Code A 改动只能是地面投影、合法交互边缘、玩家当前位置的安全落点解析与 Host 生命周期适配；它不能拥有或保存新物品图、世界掉落物品真值、物品数量或 P6 终局。
2. Code B 的 P1 Repository 是新物品体系唯一可变真值。P5 是局外 Profile 物品 snapshot；P6 是精确 `OwnerId + RunInstanceId` 的活动 Run 玩家携带 snapshot；P8 只在 Code A 已提交终局后处理 P6 的归还／没收与关闭。P14 的地面物品只属于同一 P6 active-session durable record，绝不写入 P5、Code A Save、Actor、P9、P11 或另一份世界库存。
3. P7 是 P6 玩家装备、基础 6 格、已装备空间戒指快捷空间和空间储物囊内部格的生产 Host。物品位置变化继续必须由已挂载生产页面的真实 `NativeOnDrop → P4 → P3 → P2 → P1` 手势链提交。P14 仅增加一个明确的生产 `GroundDropZone` 和一个受地图交互打开的 `WorldDropTarget`；二者都必须通过同一真实 Drag／Drop 语义，而不是按钮、快捷键、QuickMove 或隐式命令。
4. P9/P10 是普通地图容器唯一的 Run-local 真值；P11/P12 是唯一绑定 Low Skirmisher 身体容器的 Run-local 真值。P14 不改变它们的身份、内容、揭示、开启、转移、终局残余或 recipe，不能把普通容器或尸体物品伪装为 P14 地面物品。
5. P13 的 1—9 快捷栏仍是九个 `SlotIndex + ItemId | Empty` 引用，不是容器、数量副本或位置真值。P14 只在物品离开 P6 `Container.Player.BaseQuick` 的同一 durable commit 中调用既有 reconcile 清除失效引用；拾回到基础 6 格不会自动恢复、自动绑定或改变任何 1—9 槽。

## 3. P14 产品裁决

P14 只完成“玩家已携带的一件简单物品被明确拖至地面，然后可从同一局内地面投影重新打开并拖回基础 6 格”的一条垂直路径。

这不是随机世界 Loot、敌人掉落、地图资源节点或全面地面物品系统。当前只允许活动 P6 session 中由玩家主动创建的单物品地面记录；其 Code A Actor 只是由 Code B projection 生成、销毁和重建的表现，绝不保存或推断物品真值。

P14 不增加新的物品定义、starter item、reward recipe、随机 seed、世界拾取键、自动拾取、分堆、合并、交换、空间道具整体移动、装备掉落、消耗、治疗、属性、战斗效果或物理 1—9 使用。后续功能必须另行签发。

---

# 下半部分：授权执行内容

## 4. 单一授权目标

建立 Code B P6 内的单一地面掉落记录及其生产 UI／Code A 表现适配：玩家可将一个当前位于 P6 基础 6 格、无 child-container graph 的完整简单物品实例，真实拖到 P14 `GroundDropZone`；同一 P6 durable transaction 将该物品移入一个确定性的 P14 世界容器，并产生可由 Code A 投影的 `WorldDropId` 和安全落点。玩家随后只可通过该投影打开 `WorldDropTarget`，再以真实拖拽把该物品放回一个空的 P6 基础 6 格。整个路径始终只有一个 P1 ItemId、一个父位置和一个 P6 运行态真值。

### 4.1 P6 地面掉落模型、身份与资格

1. 只在精确活动 `OwnerId + RunInstanceId` 的 P6 durable record 中增加版本化 `WorldDrops`（或等价）集合和单调 `NextWorldDropOrdinal`。P6 schema 从 P13 的当前 schema `3` 升级至 `4`；旧 P6 session migration 获得显式空集合与安全初始 ordinal。P5 schema、P5 物品图和 P5 Profile lock 语义不得因 P14 改写。
2. 每一条活动记录只表示一个由玩家主动创建的地面物品。其持久数据必须等价于：确定性的 `WorldDropId`、从该 ID 派生的 P1 世界容器 ID、唯一的 `ItemId` 外键、逻辑地图路由／安全落点 transform 与最小 action state。`WorldDropId` 必须由 `OwnerId + RunInstanceId + 已提交 ordinal` 稳定导出；不得使用 Actor pointer、Widget、显示名、随机 runtime GUID、Code A Save key、数量副本、Definition 副本、图标或第二份 Item graph。
3. P14 当前只接受：物品存在、数量大于零、父容器正是当前 P6 `Container.Player.BaseQuick`、无 `ChildContainerId` 或任何 child-container graph、未装备且不属于空间道具本体的完整单实例。若该实例是堆叠物，则本轮只允许整堆移动；不实现 Split、部分丢弃、Merge 或 Swap。所有不满足条件的 payload 必须零写入拒绝。
4. 物品由 `BaseQuick` 移入 P14 世界容器时，必须先完整验证 P6 revision、P1 graph、目标落点、容器容量、ItemId、Parent/child graph、session 状态与 terminal／Prepared／closed gate，再在同一 Owner durable replacement 中提交 P1 位置图、P6 `WorldDrops` 和 P13 binding reconcile。失败、冲突、保存失败、身份失配或不完整图不得生成 Actor、不得占用 ordinal、不得改变 ItemId、物品位置或快捷栏引用。
5. 安全落点由一个最小 Code A placement adapter 在现有活动地图／Player 合法状态下解析为可投影的地图路由与 floor-resolved transform；该 adapter 不接收、保存或决定 ItemId、数量、Definition 或库存。Code B 只把成功解析的不可变落点随 P6 transaction 持久化。Actor 生成失败或临时销毁不能把已提交物品回写 P6 基础格，也不能让 Code A 形成可写 fallback；后续 projection refresh 只能根据同一 P6 record 重新表现该 WorldDrop。

### 4.2 生产丢弃、地面交互与真实拾回

1. 在现有 P7/P3/P4 生产 Host 内增加一个当前 P6 session 存在时才挂载的、可见且命名明确的 `GroundDropZone`。它是实际 DragOperation 的明确目标，而不是“丢弃选中物品”按钮。只有来自当前 P6 基础 6 格、满足 4.1 的真实 payload 通过其 `NativeOnDrop` 后，才可请求 Store-owned P14 drop transaction。
2. `GroundDropZone`、双击、右键、详情、I、Esc、Close、Cancel、空格、UI 外部区域、无 payload Drop、禁用 1—9、选中状态和 Actor interaction 都不得直接提交任何物品位置写入。P14 不增加 QuickMove、自动丢弃、快捷键丢弃或拖到任意屏幕坐标的隐式路径。
3. 新增或最小扩展一个仅为 P14 projection 服务的 Code A `WorldDropActor`（或等价）。它只用精确 `OwnerId + RunInstanceId + WorldDropId` 读取当前 Code B projection、在该 record 仍 active 时显示一个地面表现，并把既有上下文交互／距离／页面生命周期边缘转发给 Code B。它不能 materialize item、创建 ID、改变 P1 graph、直接拾取、自动拾取、改变 Code A combat／Run／terminal，或在 projection 缺失时重建任何库存。
4. 对这个单一 P14 actor 的合法显式交互只能打开一个瞬态 `WorldDropTarget` 第二栏。该栏只从权威 P6 projection 显示一个已知地面实例；不增加 P10/P12 的开启读条、Hidden／Searching／Revealed 状态机、随机生成、尸体路由或其他地图容器。关闭、距离丢失、Actor 销毁、Host 失效、终局或 session 缺失只清理瞬态 UI，不写物品。
5. 拾回只允许从 `WorldDropTarget` 的唯一真实 source，真实拖到当前 P6 一个空的 `Container.Player.BaseQuick` cell。它必须在一项 Owner durable transaction 中验证 exact session、WorldDropId、源／目标 revisions、P1 graph、空目标、世界容器、ItemId 与简单实例资格，然后把原 ItemId 移回 P6、移除对应 world record，并在成功保存后才让 projection 消失。P14 不实现 pickup Merge、Swap、Split、自动整理、按钮拾取或直接 Actor 拾取。

### 4.3 P6 生命周期、P8 终局与快捷栏不变性

1. P5→P6 bridge 成功创建的活动 P6 session 从空的 P14 `WorldDrops` 开始。P14 不把 P5 物品、P5 绑定、旧世界 Actor 或任意 P5 world record 带入 P6，也不改变 Prepared receipt、Start Run 条件、Profile lock、rebind 或 recovery 的核心语义。
2. 若既有 P6 Prepared／RecoveredAbandon rebind 合法保留同一活动 P6 snapshot，P14 世界记录只能随该同一 snapshot 原样保存并由 projection 重挂；不得从 P5、Code A、Actor 或 UI 猜测、补造、重新生成或回填世界物品。若 session 已 terminal／closed，则拒绝 drop、open 与 pickup。
3. P8 仍必须只在 Code A 已完成终局裁决后执行。`Extracted` 只按既有规则把当前仍在 P6 玩家 graph 内的物品归还 P5；所有尚留在 P14 world containers 的物品和 records 必须同 Dead／RecoveredAbandon 一样在 P8 的同一 terminal close 中丢弃，绝不回流 P5、P6、Code A、P9 或 P11。P14 不修改 P8 分类、terminal receipt、幂等、锁、观察顺序或恢复策略。
4. 当 BaseQuick ItemId 成功 drop 到世界容器时，复用 P13 reconcile 在同一 P6 replacement 中清除它的快捷栏引用。该 ItemId 成功拾回空基础格后也保持未绑定；P14 绝不自动恢复旧槽、自动绑定、改变数量或启用物理 1—9／使用／消费。
5. P9/P10 与 P11/P12 的跨图事务、container／body identity、reveal、recipe、终局残余和 Actor 路由都保持不变。P14 不提供从普通容器、尸体、装备栏、空间戒指、储物囊、P5 仓库或 Code A 旧库存直接创建或接收 world drop 的路径。

## 5. 允许的改动范围

允许：

- 在 Code B P6 durable record、schema migration、projection 和 session cleanup 中增加最小单物品 `WorldDrops` 记录与其安全恢复；
- 在 P1/P2 或既有 owner-record transaction 边界增加 P14 的 whole-instance BaseQuick↔world-container 原子移动服务，复用 P13 reconcile，不改变 P1 物品图的已有语义；
- 在 P7/P3/P4 生产 Host 中增加实际 `GroundDropZone`、`WorldDropTarget` 和唯一真实 Drag／Drop 请求路径；
- 新增或最小扩展一个 Code A 地面投影 Actor、placement adapter 和 `demo_mapV3ProgressionManager` 生命周期边缘，只用于投影、交互转发、位置解析与 Host 清理；
- 为 P8 的既有 P6 terminal close 加入 P14 world-record 残余丢弃的最小同事务适配；
- 更新 PROJECT.md、PROJECT_INFO_CARD.md、本任务 Prompt 归档和本任务 Report。

## 6. 明确不在本任务内

不得实现、启动、重构或接管：

- 随机地面 Loot、地图预放置 Loot、敌人死亡掉落、第二种 world target、地面物品堆叠、多个物品同一地面容器、自动拾取、交互键直接拾取、快速拾取、QuickMove、拖到任意地图坐标、自动整理、Split、Merge、Swap 或部分数量丢弃；
- 装备、道袍、饰品、空间戒指、储物囊、任何有 child-container graph 的物品整体掉落／拾回，或 P5 仓库、P9/P10、P11/P12、旧库存与 P14 之间的直接通路；
- 物理 1—9 使用、快捷栏绑定／解绑语义改写、物品使用、消耗、数量扣减、治疗、属性、战斗、冷却、技能、装备效果、Code A Player Actor 效果适配；
- P5/P6 bridge、Prepared receipt、RecoveredAbandon rebind、Start Run 成功条件、Profile lock 核心语义、P8 terminal receipt／分类／恢复，或 P5/P6 的既有物品图迁移策略；
- Code A 地图、Player、输入原始派发、战斗、死亡、HUD、Run、Run Save、旧 Loot／搜索、正式结算权威或任何 A/B 双写；
- 实际运行、CTA、wrapper、自动化、回归、截图、可见验收、Smoke、进程检查、Game target 编译、BuildCookRun、Cook 或 Package。

## 7. 静态代码审查与编译

完成实现后，只进行以下 P 阶段检查：

1. 审查 P6 `WorldDrops` 模型与 migration，确认它只保存确定性 ID、唯一 ItemId 外键、派生世界容器、最小地图落点／state；P5 不新增世界库存，Actor／Widget／显示文本／随机 GUID／数量／Definition／第二份 graph 均不成为真值。
2. 审查 drop 资格和事务，确认只接受当前 P6 BaseQuick 的完整简单实例，且真实 `GroundDropZone` Drop 是唯一 player→world 位置写入入口；拒绝路径不消耗 ordinal、不生成 Actor、不改变物品位置或 P13 binding。
3. 审查 world projection 与 Code A 改动，确认 Code A 只负责当前活动 Run 的安全落点、表现、交互转发和瞬态 Host 生命周期；它不创建或保存物品、不会在 failure／destroy／reopen 时形成 mutable mirror 或回退库存。
4. 审查 pickup 事务，确认它只允许 `WorldDropTarget` 至空 P6 BaseQuick 的真实 Native Drop，完整验证后以一项 owner durable replacement 移动同一 ItemId 并移除 world record；没有 Merge、Swap、Split、自动拾取、按钮拾取或非 Drop 位置写入。
5. 审查 P6 Prepared/rebind、P8 和 P13 适配，确认 P5→P6 初始为空、合法同 snapshot rebind 不补造、终局遗留地面物品永不回流 P5、掉落同事务清 binding、拾回不自动 rebind，且 P8 的 Code A 后置观察和分类未改变。
6. 审查 P4x、P5、P7、P9/P10、P11/P12 与 Code A 边界，确认本轮没有改写 Drop-only 基础规则、普通容器／尸体真值、P1 物品图、P5 durable 图、地图／战斗／Run／Save／terminal 权威或任何 A/B 双写。
7. 编译一次 Editor 目标：

       "C:\\Program Files\\Epic Games\\UE_5.8\\Engine\\Build\\BatchFiles\\Build.bat" demo_mapEditor Win64 Development "C:\\AIDev\\shanmen-ue\\Dev.D.UE.0.0.9B\\demo_map.uproject" -WaitMutex

8. 若编译失败，只修正 P14 引入的局部声明、include、P6 schema migration、world-drop projection、placement adapter、UI lifecycle、transaction 或调用签名问题，然后重新执行同一 Editor 目标。若修复需要越过本 Prompt 边界，停止受影响工作并报告；不得自行扩展到随机 Loot、物品使用、P15 或 0.0.9B.F。

## 8. Report 与完成信号

生成 Dev.D.UE.0.0.9B.P14.0.r0_report.md，保存至：

    C:\\AIDev\\shanmen-ue\\Dev.D.UE.0.0.9B\\Docs\\Report

Report 必须简洁、可审计地列出：

1. P14 新增／修改／未修改文件及职责；
2. P6 `WorldDrops` 的 schema migration、deterministic `WorldDropId`／container identity、ItemId 单一引用、地图落点与为什么 Actor 不持有真值；
3. player→world whole-instance drop 与 world→P6 empty BaseQuick pickup 的精确资格、真实 Drag／Drop 入口、原子提交与拒绝／保存失败／冲突的零写入结论；
4. Code A placement／projection Actor／interaction adapter 的职责，以及其不改变 Code A 地图、死亡、战斗、Run、Save 或 terminal 权威的理由；
5. P5→P6、Prepared／rebind、P8 Extracted／Dead／RecoveredAbandon、P13 binding reconcile 与 P9/P10／P11/P12 不变性的结论；
6. 对 P4x、P5、P6、P7 Drop-only、P8 finalizer、P9/P10、P11/P12、P13 与 Code A 权威的静态审查结论；
7. 实际 Editor 编译命令、目标、最终 native exit code 和关键结果；
8. 明确列出未执行的 F 阶段项目：真实丢弃／Actor 投影／重新打开／拾回／存档恢复／终局丢弃／多分辨率输入验证、CTA、wrapper、自动化、回归、截图、Smoke、Game Build、Cook、Package 和最终验证仍由 0.0.9B.F 负责；
9. 明确列出尚未启动的后续功能：随机世界 Loot、自动／直接拾取、地面多物品容器、堆叠处理、装备／空间道具整体掉落、物品使用与消耗效果，以及其他地图容器／敌人迁移。

仅当实现完成、静态边界审查通过、Editor 编译以 native exit code 0 完成且未越界时，Report 可使用：

    READY_FOR_NEXT_P_FUNCTIONAL_WITH_F_DEBT

若仅存在当前范围内可修复的编译问题，使用：

    NEEDS_P14_COMPILE_REWORK

若无法在不改变 Code A 世界／Run／终局权威或 P1/P5/P6 核心事务语义的前提下建立单一 P6 world-record 与投影边界，使用：

    NEEDS_PLANNER_DECISION
    BLOCKED

完成后不得自动开始 P15、0.0.9B.F 或其他任务。向策划 Chat 回传并附带且只附带本次同名 Report；正文首行使用：

    [CSEMI:REPORT_SENT] {"task_id":"Dev.D.UE.0.0.9B.P14.0.r0","file":"Dev.D.UE.0.0.9B.P14.0.r0_report.md"}
