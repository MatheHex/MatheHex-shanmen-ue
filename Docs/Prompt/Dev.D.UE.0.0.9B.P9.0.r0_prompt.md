# Dev.D.UE.0.0.9B.P9.0.r0

## 任务身份

- 项目：`Dev.D.UE.0.0.9B`
- 阶段：P9——Code B 局内普通容器的持久状态、一次性物质化与隐藏物品基础
- 任务编号：`Dev.D.UE.0.0.9B.P9.0.r0`
- 任务性质：在 P5 局外 Profile、P6 活动 Run session、P7 个人背包和 P8 终局归还／没收之后，为后续“地图普通容器 → 交互开启 → 搜索揭示 → 双栏转移”建立唯一的 Run 内普通容器真值。P 阶段只做功能开发、静态代码审查、Editor 代码编译和必要的最小编译修正；真实运行、交互验证、截图、回归、Smoke、Game Build、Cook、Package 和最终验收统一留给 `0.0.9B.F`。
- 执行文件：`Dev.D.UE.0.0.9B.P9.0.r0_prompt.md`
- 报告文件：`Dev.D.UE.0.0.9B.P9.0.r0_report.md`
- 活动工程根：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B`
- 活动工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`
- 引擎：`C:\Program Files\Epic Games\UE_5.8`
- 工程级开发基线：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9-XFix1`
- 已接受前置：P4x.0.r2、P5 的 Code B 局外 Profile 实施交接、P6.0.r3/r4 的活动 Run receipt／rebind 与编译收尾、P7.0.r0 的活动 Run 个人背包，以及 P8.0.r0 的终局 session 归还／没收与关闭。

---

# 上半部分：只读项目裁决、现状与边界

## 1. 当前 P／F 阶段分界

| 阶段 | 负责内容 |
| --- | --- |
| P | 功能开发、必要的静态代码审查、目标代码编译与最小编译修正。 |
| F | 真实运行测试、CTA／wrapper、自动化与回归、截图／可见验收、Smoke、Game Build、Cook、Package 和最终验证。 |

本任务不得为了证明 P9 而启动产品、运行测试、增加或执行测试专用路径、采集截图、执行回归、Smoke、Game Build、Cook 或封包。一次 Editor 代码编译是本任务唯一要求的执行性检查，不替代 F 的真实验证。

## 2. 当前所有权与已完成边界

1. Code A 继续拥有默认地图、地图 Actor、Player Actor、战斗、正式 Run 生命周期、交互距离与输入、Loot／搜索的既有运行时、世界物品、撤离／死亡判定、HUD、Run Save、旧库存和正式结算返回值。
2. Code B 的 P1 Repository 是新物品体系唯一可变真值。P5 是局外 Code B Profile snapshot；P6 是匹配 `OwnerId + RunInstanceId` 的活动 Run 物品会话；P8 是该会话终局后的 Code B receipt。不存在 A／B 镜像、同步或双写。
3. P7 只展示和经 Drop 事务链修改玩家自己的 P6 携带物品图。P7 不显示普通容器，P9 不得改变 P7 的 Drop-only 规则或提前引入双栏战利品页面。
4. P8 只把玩家当前 P6 携带物品图在 `Extracted` 时并回 P5，或在 `Dead`／匹配 `RecoveredAbandon` 时没收它。未来 Run 内普通容器及其未取得的内容始终是本局局部状态，绝不属于 P5，绝不能在 P8 的归还／没收中被当成玩家携带图复制、迁移或复活。
5. Code A 终局的分类与提交仍是权威。P9 不修改 Start Run、Prepared receipt、RecoveredAbandon rebind、P8 terminal observer 或任何终局分类语义。

## 3. P9 产品裁决

P9 只建立“普通地图容器在一局活动 Run 内如何拥有稳定身份、一次性生成真实 Code B 物品图、持久保存搜索状态并供后续阶段读取”的数据与持久化纵向切片。

本任务仅覆盖 `NormalContainer`。不覆盖敌人尸体、身体容器、灵石领取、世界丢弃／拾取、地面 Actor、快捷栏、消耗品、装备效果或双栏 UI。

每个未来普通容器必须同时区分：

- `SearchTargetId`：运行中由未来地图交互层提供的稳定目标身份；同一 Owner／Run 中不能被两个目标复用；
- `DefinitionId`：Code B 的普通容器定义／内容计划身份；不是运行时 Actor 指针，也不是玩家物品 ItemId；
- `ContainerId`：物质化后该容器真实物品图的稳定根容器身份；
- 一次性物质化 receipt／digest：证明本 Run、该目标、该定义和其首次生成的真实 ItemId／ChildContainer graph；
- 容器搜索状态与单件 reveal 状态：为后续开启读条、逐件搜索和 UI 提供唯一持久化来源。

建议的状态语义为：容器使用 `Closed`、`Opening`、`Open`、`Interrupted`；物品使用 `Hidden`、`Searching`、`Revealed`。P9 可以定义并持久化这些枚举、合法迁移表和查询接口，但本任务只允许创建一个已物质化、内容仍为 `Hidden` 的普通容器记录；不得实现真实读条、输入、时间推进、距离中断、UI 开启或玩家可达入口。`Opening`、`Open`、`Interrupted` 和 `Searching` 的运行态转移留给后续 P 阶段。

## 4. 持续有效的不变量

1. P9 只能操作精确匹配、已 committed、仍 Active 的 P6 session。缺失、Prepared、terminal、Owner／Run 不匹配、未知 target、未知 definition、非法状态、重复或冲突请求全部无创建、无迁移、无 rebind、无 P5／P6 写入。
2. 普通容器首次物质化后，其 `ContainerId`、ItemId、数量、定义、位置、ChildContainerId、reveal 状态、receipt 和 digest 均是该 Run 的真实历史；再次请求同一 target 只返回既有记录，绝不能重新掷数、复制、重编号或覆盖内容。
3. P9 的系统物质化只可创建尚未属于任何玩家、P5 或 P7 携带图的普通容器内容。它不是玩家移动命令；不得移动、删除、装备、合并、拆分、替换或自动整理任何 P5／P6 玩家物品。
4. P4x／P7 的玩家位置写入仍只可由已挂载生产 Cell 的明确 `Drop → P4 → P3 → P2 → P1` 提交。P9 不得添加 QuickMove、自动转移、放回、双击、右键、详情、热键、隐藏写入或非拖拽物品转移入口。
5. P9 容器内容在玩家尚未取得前只能保存在活动 Run 的局部 Code B record。P8 必须明确忽略／丢弃这类局部内容而不把它们合并回 P5；P5 的局外库存、P6 玩家携带图和 P8 terminal receipt 的既有语义不得被重写。
6. 不得以 fixture、测试数据、随机 UI 创建、Actor 指针、临时 sidecar、第二套 SaveGame 或 Code A 旧库存充当普通容器真值。首次物质化必须复用现有同 Owner durable Code B record 的原子持久化路径。

---

# 下半部分：授权执行内容

## 5. 单一授权目标

实现 Code B 活动 Run 的普通容器 registry／snapshot／receipt 与一次性物质化服务，使后续 P10 可以在不重新生成物品的前提下把一个已验证的普通地图目标接入交互，后续 P11 可以读取同一容器的隐藏／揭示状态和真实物品图。

### 5.1 定义、稳定身份与持久化模型

1. 审阅现有 P1 Item／Container、P5/P6 durable record、P7 active-session facade 与 P8 terminal finalizer。把 P9 状态置于现有同 Owner 的 Code B durable record 内，并以与当前 record 相兼容的版本迁移方式读取旧 record；不得破坏既有 P5、P6 或 P8 数据。
2. 新增明确的普通容器定义／内容计划模型。每项计划至少包含稳定 `DefinitionId`、普通容器类型、容量、合法的已有 Item Definition／数量／初始布局描述，以及可用于确认首次结果的内容版本或 digest 输入。定义模型不得依赖 World Actor 实例、玩家库存或 UI。
3. 新增每 Run／每 Target 的普通容器记录。它至少保存精确 OwnerId、RunInstanceId、SearchTargetId、DefinitionId、ContainerId、当前容器状态、物质化状态、reveal state、revision、首次物质化 receipt／digest，以及完整的真实物品／ChildContainer 图。
4. 若当前可用的 Code B 物品定义足以描述一个正式普通容器内容计划，可加入最小的生产定义；若不足，建立空但有效的声明式定义入口并在 Report 明确记录后续内容填充点。不得为了凑齐示例而写入 fixture、虚假装备、测试 ItemId 或独立测试文件。

### 5.2 一次性物质化与查询门槛

1. 提供一个只接受精确 `OwnerId + RunInstanceId + SearchTargetId + DefinitionId` 的 Code B 服务入口。该入口只在匹配、committed、Active P6 session 内工作；它不接入 Code A 输入、地图 Actor、HUD 或 UI。
2. 首次接受的请求必须先完整验证定义、容量、Item Definition、布局、无重复 ItemId／ChildContainerId、无循环容器关系和目标身份唯一性；随后在同一次 durable record 原子提交中创建根 Container、真实 Item Instances、初始 `Hidden` reveal 状态、materialization receipt／digest 和 revision。
3. 同一 exact target／definition 的后续请求只能无写入地返回既有已物质化结果；同一 target 配不同 definition、不同 Owner／Run 或冲突的已存在 identity 必须拒绝且不得改写旧 record。
4. 拒绝路径不得新建空 record、空 container、receipt、session、P5 migration、Prepared receipt 或 rebind。没有 P6 active session 时不得提供“默认战利品”、临时背包或 UI fallback。
5. 提供仅供后续 P10/P11 使用的只读 projection／query：它可以读取 exact target、普通容器状态、根 ContainerId、layout 和 reveal state，但不得在查询中隐式物质化、开启、揭示或改变玩家物品。

### 5.3 P8、P7 与 terminal 边界保护

1. 明确区分 P6 的玩家携带 snapshot 与 P9 的 Run-local search-target records。若 P8 的序列化、冻结或提取路径会错误遍历新字段，可作最小 Code B 调整，让 P8 显式只处理玩家携带图、终局审计并在 session 关闭时丢弃 Run-local search-target records；不得改变其 `Extracted`／`Dead`／`RecoveredAbandon` 分类、Code A observer 或 P5 合并规则。
2. P7 不得显示、编辑或跨容器拖拽 P9 内容。P9 不得改动 P7 的页面、输入、Host、Drop controller 或玩家携带物品提交路径，除非仅为编译兼容而作无行为变化的局部声明修正。
3. 本任务不建立 Code A map actor、交互距离、键位、开启读条、World UI、双栏容器面板、拖拽取物、尸体、身体容器、灵石、掉落、拾取或奖励表现。后续阶段才能通过窄边缘层调用 P9 已建立的 exact-target 服务。

## 6. 允许的改动范围

允许：

- 新增或调整 Code B P5/P6 durable record 中的普通容器 definition、Run-local target record、materialization receipt、schema migration、digest、原子持久化、exact lookup 与只读 projection；
- 为确保 P8 不把 Run-local target 内容带回 P5，对 Code B terminal finalizer 作最小的数据边界保护；
- 复用 P1 Item／Container 校验、现有持久化和现有 Code B 物品定义；
- 更新 `PROJECT.md`、`PROJECT_INFO_CARD.md` 与本任务 Report，记录 P9 对 P10/P11 的交接和 F 阶段验证债务。

## 7. 明确不在本任务内

不得实现、启动、重构或接管：

- Code A 地图、World Actor、交互距离、输入、HUD、Player Actor、战斗、正式 Run 生命周期、Run Save、旧库存、撤离／死亡或终局裁决；
- P6 bridge、Prepared receipt、RecoveredAbandon rebind、P8 observer／分类／P5 返还与没收政策；
- 普通容器的真实开启、读条、取消／中断、单件搜索、revealed UI、双栏搜索 UI 或任何玩家到容器／容器到玩家的转移；
- 敌人尸体、身体容器、灵石、世界物品、世界丢弃、拾取、地面 Actor、快捷栏、物品使用、消耗品、装备属性、自动整理、QuickMove 或备战；
- 测试 fixture、测试命令、产品运行、CTA、wrapper、自动化、回归、截图、可见验收、Smoke、进程检查、Game target 编译、BuildCookRun、Cook 或 Package。

## 8. 静态代码审查与编译

完成实现后，只进行以下 P 阶段检查：

1. 审查 P9 的 exact identity gate，确认它只接受 committed Active P6 的 OwnerId／RunInstanceId／SearchTargetId／DefinitionId 精确组合；任何 miss、terminal、重复或冲突均无写入。
2. 审查首次物质化，确认同一 target 只会产生一次持久 record、一次 ContainerId／ItemId 图和一次 receipt／digest；恢复或重复请求不会重新生成、复制或覆盖。
3. 审查 P9 与 P1/P5/P6/P7/P8 的数据边界，确认容器内容在未取得前从不进入玩家携带图或 P5，P8 不会将其提取到局外 Profile，P7 仍无跨容器写入口。
4. 审查所有 Code A 改动。默认不应有 Code A 改动；若出现仅为声明／编译兼容的最小改动，必须证明其不涉及地图、玩家、交互、Loot、搜索、Run、结算、Run Save 或旧库存权威。
5. 编译一次 Editor 目标：

       "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_mapEditor Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex

6. 若编译失败，只修正 P9 引入的局部声明、include、类型、序列化、schema migration、持久化或调用签名问题，然后重新执行同一 Editor 目标。若修复需要越过本 Prompt 边界，停止受影响工作并报告，不得自行扩展到 P10、P11 或 F。

## 9. Report 与完成信号

生成 `Dev.D.UE.0.0.9B.P9.0.r0_report.md`，保存至：

    C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Docs\Report

Report 必须简洁、可审计地列出：

1. P9 新增／修改／未修改的文件及职责；
2. 普通容器 Definition、SearchTargetId、ContainerId、Run-local record、state／reveal 模型和 schema migration；
3. 首次物质化的 exact identity gate、原子提交、receipt／digest、重复／冲突处理和只读查询；
4. P9 如何保证普通容器内容不会进入 P7 玩家携带图或被 P8 提取回 P5；
5. 对 P4x、P5、P6 Prepared rebind、P7 Drop-only、P8 terminal finalizer 与 Code A 权威的静态审查结论；
6. 实际 Editor 编译命令、目标、最终 exit code 和关键结果；
7. 明确列出本轮未执行的 F 阶段项目，以及 P1–P9 的真实运行、CTA、自动化、回归、截图、Smoke、Game Build、Cook、Package 与最终验证仍由 `0.0.9B.F` 负责。

仅当实现完成、静态边界审查通过、Editor 编译通过且未越界时，Report 可使用：

    READY_FOR_P10_INTERACTION_FUNCTIONAL_WITH_F_DEBT

若仅存在当前范围内可修复的编译问题，使用：

    NEEDS_P9_COMPILE_REWORK

若定义来源、稳定目标身份或 P8 数据边界需要新的产品裁决，使用：

    NEEDS_PLANNER_DECISION
    BLOCKED

完成后不得自动开始 P10、P11、`0.0.9B.F` 或其他任务。向策划 Chat 回传并附带且只附带本次同名 Report；正文首行使用：

    [CSEMI:REPORT_SENT] {"task_id":"Dev.D.UE.0.0.9B.P9.0.r0","file":"Dev.D.UE.0.0.9B.P9.0.r0_report.md"}
