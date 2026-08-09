# Dev.D.UE.0.0.9B.P8.0.r0

## 任务身份

- 项目：`Dev.D.UE.0.0.9B`
- 阶段：P8——Code B 活动 Run 会话终局结算与关闭
- 任务编号：`Dev.D.UE.0.0.9B.P8.0.r0`
- 任务性质：在已完成的 P5 局外 Profile、P6 活动 Run session 和 P7 局内个人背包基础上，实施 Code B 的终局会话处理。P 阶段仅负责功能开发、静态代码审查、Editor 代码编译和必要的最小编译修正；真实运行、CTA、自动化、回归、截图、Smoke、Game Build、Cook、Package 和最终验收统一保留给 `0.0.9B.F`。
- 执行文件：`Dev.D.UE.0.0.9B.P8.0.r0_prompt.md`
- 报告文件：`Dev.D.UE.0.0.9B.P8.0.r0_report.md`
- 活动工程根：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B`
- 活动工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`
- 引擎：`C:\Program Files\Epic Games\UE_5.8`
- 工程级开发基线：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9-XFix1`
- 已接受前置：P4x.0.r2、P5 的真实局外 Profile 实施交接、P6.0.r3/r4 的活动 Run receipt/rebind 实现与编译收尾，以及 P7.0.r0 的活动 Run 个人背包 Host。

---

# 上半部分：只读项目裁决、现状与边界

## 1. 当前 P／F 阶段分界

| 阶段 | 负责内容 |
| --- | --- |
| P | 功能开发、必要的静态代码审查、目标代码编译与最小编译修正。 |
| F | 真实运行测试、CTA／wrapper、自动化与回归、截图／可见验收、Smoke、Game Build、Cook、Package 和最终验证。 |

本任务不得为了证明 P8 而启动产品、运行测试、编写或执行测试专用路径、采集截图、执行回归或封包。一次 Editor 代码编译是本任务唯一要求的执行性检查，不替代 F 的真实验证。

## 2. 已有所有权与数据边界

1. Code A 继续拥有默认地图、Player Actor、战斗、正式 Run 生命周期、撤离／死亡判定、旧库存、Loot、搜索、世界物品、结算、Run Save 和既有正式运行时。
2. Code B 的 P1 Repository 是新物品体系唯一可变真值。P5 是局外 Code B Profile snapshot；P6 是活动 Run Code B snapshot。不存在 A／B 镜像、同步或双写。
3. P6 在 Code A 已成功激活 Run 后才取得稳定 `OwnerId + RunInstanceId`。P6 的 Prepared receipt、新 RunId rebind、`RecoveredAbandon` recovery history、首次携带物品图与其不可变 digest 已是既有 P6 职责；P8 不得重写、调用以改变语义，或以 P8 逻辑绕过它们。
4. P7 已以精确身份读取已 committed 的 P6 session，并只经 `P4 → P3 → P2 → P1` 的真实 Drop 事务链改变该 session。P8 必须把 P7 当前 session snapshot 视为终局时唯一有效的携带状态；不得用 P6 初始 receipt 重建、覆盖或恢复 P7 已合法整理后的布局。
5. P5 在匹配的活动 Run session 存在时保持锁定。P8 只有在同一 Run 的 Code B 终局记录已原子完成后，才可让既有 P5 正常入口恢复可编辑；不得在 Active、Preparing、失败或不匹配状态提前解锁。

## 3. P8 产品裁决

P8 只闭合“已有 P6 活动 Run session 如何在 Code A 已完成终局裁决后被一次性处理”的纵向切片。

Code A 仍独立决定一个正式 Run 是成功撤离、死亡，还是启动恢复中已结算的 `RecoveredAbandon`。P8 仅可在 Code A 的既有终局写入已经成功完成之后，接收稳定身份和终局分类；P8 的成功、失败、延迟恢复或幂等重复不得改变 Code A 的终局结果、返回值、地图、玩家、HUD、Run Save 或旧库存。

本版本的物品政策固定如下：

| Code A 已提交的终局分类 | Code B P8 的处理 |
| --- | --- |
| `Extracted` | 将 P7 当前的完整 P6 携带物品图原样并入同一 Owner 的 P5 Profile，保留 ItemId、数量、定义、父位置、槽位、ContainerId 与 ChildContainerId；保留原本未出战的 P5 仓库物品。随后关闭该 P6 session。 |
| `Dead` | 不把 P6 当前携带图返回 P5；以可恢复、可审计的终局记录没收该 session，并关闭它。不得生成尸体、世界物品、奖励或替代库存。 |
| `RecoveredAbandon` | 仅当存在同一 Owner／RunId 的 committed Active P6 session 时，按 `Dead` 的没收政策处理。它不得触碰 P6 的 Prepared receipt rebind 逻辑。 |

P8 不加入新的掉落、Loot、尸体、搜索、拾取、世界丢弃、灵石／资源发放、快捷栏、物品使用或结算 UI。此时 P8 处理的是已出战物品的归还／没收与 session 关闭，不是地图奖励系统。

## 4. 持续有效的 P4x／P5／P6／P7 不变量

1. 旧备战仍不是 Start Run 门槛。P8 不得恢复装备检查、备战初始化或任何开局阻断。
2. 普通储物位置不按类别互斥，只有装备栏实施类别和单槽互斥；空间戒指与空间储物囊继续保留各自 ChildContainerId 与 Loaded Spatial 原子性。
3. Run Active 期间，位置改变、Move、Swap、Merge、Replacement 和 Unequip 仍只由 P7 已挂载生产 Cell 的真实 Drop 提交。P8 不得新增 QuickMove、自动整理、放回、双击、右键、详情、菜单、热键或隐藏写入入口。
4. P8 接受终局后，P7 Host 必须关闭或失去可写 session；关闭、Esc、Cancel、失配、重复终局、空格、详情、双击、右键和禁用 1—9 均不得形成新的 P1／P5／P6 写入入口。
5. 每个 P8 终局提交必须由精确的 `OwnerId + RunInstanceId`、既有 committed P6 session 及允许的 terminal state 共同限定。没有匹配 session、session 尚未 committed、身份不匹配、未知状态或来自不同 Run 的通知一律无写入、无创建、无迁移、无 rebind，也不影响 Code A。
6. 同一 Run 的终局通知可以因撤离收尾、死亡、地图卸载、恢复或生命周期重复到达；最终只允许形成一次 Code B 终局提交。已完成的同一终局必须幂等返回，冲突的第二终局不得覆盖第一次已提交的分类或物品结果。

---

# 下半部分：授权执行内容

## 5. 单一授权目标

实现 Code B 的 P8 活动 Run 终局处理器及其最小、后置、非阻塞的 Code A 终局观察适配层，使匹配的 P6 session 能在 Code A 已完成终局裁决后，原子地归还或没收物品并关闭 session。

### 5.1 终局输入与 Code A 边缘适配

1. 先审阅活动工程中既有的撤离、死亡、`RecoveredAbandon`、Profile settlement 与 Run lifecycle 路径，选择一个或多个已经确认“Code A 终局写入成功”的后置位置。
2. 仅在该既有 Code A 成功终局之后，以最小 observer／adapter 向 Code B 提供稳定 `OwnerId`、`RunInstanceId` 和终局分类。不得让 Code B 决定分类，不得前置、包裹、替换、重排或回滚 Code A 事务。
3. Code A 调用者必须忽略 Code B 处理结果，保持既有的 Code A 成功、失败、退出、重载和恢复语义。Code B 处理失败只能留下自身可恢复的 P8 记录，不能把 Code A 已完成的终局改写成失败，也不能阻塞地图／HUD／玩家清理。
4. 若当前工程没有一个不触及 Code A 权威即可安全挂接的“终局已提交”观察点，停止受影响的适配工作并在 Report 使用 `NEEDS_PLANNER_DECISION`；不得猜测终局、抢在 Code A 之前处理，或以 UI／测试命令替代正式来源。

### 5.2 Code B P8 终局状态与原子提交

1. 在既有同 Owner durable Code B record 内，为 P8 增加明确、可恢复、可审计的终局 receipt／状态；其实现可复用既有的 temp-verify-backup-replace 原子持久化机制，但不得另建不受管理的 sidecar、第二套 Profile、临时 fixture 或 SaveGame 格式。
2. 处理器只接受精确匹配的 committed Active P6 session。它应在一次逻辑终局中冻结当前 P6 snapshot，并以该冻结快照生成唯一的 P8 settlement receipt。重复同 Run／同分类只返回既有结果；不同分类或不同身份不得改写已存在 receipt。
3. 对 `Extracted`：
   - 以 P7 当前 P6 snapshot 为唯一来源，将携带根物品及它们所属的完整 ChildContainer graph 并入 P5；
   - 保留每个 ItemId、Definition、Quantity、父容器、SlotIndex、ContainerId 与 ChildContainerId，不复制、不重编号、不重建初始 receipt；
   - 保留 P5 原本未出战的仓库／局外物品，不误删、不覆盖、不重置；
   - 仅在 P5 更新和 P6 terminal close 能够以同一 durable record 完整提交时，才标记 `Extracted` 完成并解除该 Owner 的既有 active-run lock。
4. 对 `Dead` 和匹配 committed session 的 `RecoveredAbandon`：
   - 冻结并保留足够的终局审计信息，以证明被没收的 P6 current graph 与终局分类；
   - 不把这些物品、其空间容器或 ChildContainer 内容重新并入 P5；不从 P6 初始 receipt 复活物品；
   - 关闭对应 P6 session，并在成功的同一终局后允许 P5 的既有 lock 按原产品规则释放。
5. 对原子提交中断或应用重启：只允许恢复、完成或确认同一 P8 receipt；不得重复并入 P5、重复没收、复制 ItemId、重新打开 session 或把已关闭 session 当成新 Run。P8 的恢复不得改写 P6 Prepared receipt／rebind／recovery history。
6. P8 终局开始后立即使匹配 P7 Host 不再可编辑，并清理只读 projection、DragOperation、Preview 和临时输入状态；这只是 UI 生命周期清理，不得通过 UI 直接写 P1／P2／P5／P6。

### 5.3 允许的改动范围

允许：

- 新增或调整 Code B P8 terminal receipt、状态、原子持久化、恢复与 exact-session finalizer；
- 为 P5/P6 现有 snapshot 增加最小的结算合并／没收 adapter，复用 P1/P2 布局语义与既有持久化边界；
- 调整 P7 Host／Presenter 的终局关闭与只读失效行为，但不得改变 P7 Active 状态下的拖拽规则；
- 仅为后置通知、稳定身份传递和 Host 生命周期清理，修改最小的 Code A Profile settlement／Run lifecycle 边缘层；
- 更新 `PROJECT.md`、`PROJECT_INFO_CARD.md` 和本任务 Report，清楚记录 P5/P6/P7/P8 所有权、状态转换和 F 验证债务。

### 5.4 明确不在本任务内

不得实现、启动、重构或接管：

- Code A 的撤离／死亡条件、终局分类、Run lifecycle、地图、Player Actor、战斗、HUD、Run Save、旧库存、正式结算交易或其返回值；
- P6 bridge、Prepared receipt、RecoveredAbandon 的新 RunId rebind、Start Run 成功条件或 P5 migration；
- Loot、尸体／容器搜索、开启读条、世界物品、世界丢弃、拾取、地面 Actor、奖励生成、灵石／资源提交、局内双栏战利品 UI、快捷栏或物品使用；
- 新的局外 UI、备战、自动整理、自动归位、QuickMove、详情按钮写入或任意 A／B 双写；
- 实际运行、CTA、wrapper、自动化、回归、截图、可见验收、Smoke、进程检查、Game target 编译、BuildCookRun、Cook 或 Package。

## 6. 静态代码审查与编译

完成实现后，只进行以下 P 阶段检查：

1. 审查 Code A adapter，确认它只在既有 Code A 终局已成功提交后观察稳定身份与分类；Code B 结果不改变 Code A 权威、返回值或生命周期。
2. 审查 P8 state／receipt 与身份门槛，确认 Active 以外、失配、未知、重复与冲突终局都无创建、无迁移、无 rebind、无 P5/P6 写入。
3. 审查 `Extracted` 合并，确认当前 P6 graph 的 ItemId／ChildContainerId／数量／位置完整保持，P5 未出战物品保持不变，且 P5 + P6 close 只通过一次原子提交完成。
4. 审查 `Dead`／`RecoveredAbandon` 没收，确认它们不会从 P6 初始 receipt 重建物品、不会生成尸体／世界物品／奖励，也不会让 P7 在终局后继续写入。
5. 审查 P6 Prepared rebind 和 P4x 拖拽不变量，确认本轮没有改变其职责或新增非 Drop 位置写入口。
6. 审查所有 Code A 改动，确认它们仅是后置终局观察／输入失效边缘层，没有转移 Run、Player、Loot、搜索、结算、Run Save 或旧库存权威。
7. 编译一次 Editor 目标：

       "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_mapEditor Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex

8. 若编译失败，只修正 P8 引入的局部声明、include、类型、持久化结构、生命周期或调用签名问题，然后重新执行同一 Editor 目标。若修复需要越过本 Prompt 边界，停止受影响工作并报告，不得自行扩展到未来 P 阶段或 F。

## 7. Report 与完成信号

生成 `Dev.D.UE.0.0.9B.P8.0.r0_report.md`，保存至：

    C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Docs\Report

Report 必须简洁、可审计地列出：

1. P8 的新增／修改／未修改文件及其职责；
2. Code A 终局观察点、传递的精确身份与分类，以及为什么该边缘层不改变 Code A 权威；
3. P8 的 terminal state／receipt、幂等与恢复规则；
4. `Extracted` 的 P6→P5 graph 合并、`Dead`／`RecoveredAbandon` 的没收规则、P5 lock 释放时机和 P7 Host 关闭规则；
5. 对 P4x、P5、P6 Prepared rebind 与 P7 Drop-only 不变量的静态审查结论；
6. 实际 Editor 编译命令、目标、最终 exit code 和关键结果；
7. 明确列出本轮未执行的 F 阶段项目，以及 P1–P8 当前实现的真实运行、CTA、自动化、回归、截图、Smoke、Game Build、Cook、Package 与最终验证仍由 `0.0.9B.F` 负责。

仅当实现完成、静态边界审查通过、Editor 编译通过且未越界时，Report 可使用：

    READY_FOR_NEXT_P_FUNCTIONAL_WITH_F_DEBT

若仅存在当前范围内可修复的编译问题，使用：

    NEEDS_P8_COMPILE_REWORK

若缺少安全的 Code A 后置终局观察点，或需要新的产品裁决，使用：

    NEEDS_PLANNER_DECISION
    BLOCKED

完成后不得自动开始下一 P 阶段、`0.0.9B.F` 或其他任务。向策划 Chat 回传并附带且只附带本次同名 Report；正文首行使用：

    [CSEMI:REPORT_SENT] {"task_id":"Dev.D.UE.0.0.9B.P8.0.r0","file":"Dev.D.UE.0.0.9B.P8.0.r0_report.md"}
