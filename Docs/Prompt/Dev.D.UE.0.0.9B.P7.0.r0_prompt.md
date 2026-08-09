# Dev.D.UE.0.0.9B.P7.0.r0

## 任务身份

- 项目：Dev.D.UE.0.0.9B
- 阶段：P7——Code B 活动 Run 个人背包界面与交互层
- 任务编号：Dev.D.UE.0.0.9B.P7.0.r0
- 任务性质：在已完成 P6 活动 Run 库存会话的基础上，实施局内个人背包的功能与 UI 接入。P 阶段只做功能开发、静态代码审查、Editor 代码编译和必要的最小编译修正；所有实际运行、CTA、自动化、回归、截图、Smoke、Game Build、Cook、Package 和最终验收统一留给 0.0.9B.F。
- 执行文件：Dev.D.UE.0.0.9B.P7.0.r0_prompt.md
- 报告文件：Dev.D.UE.0.0.9B.P7.0.r0_report.md
- 活动工程根：C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B
- 活动工程：C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject
- 引擎：C:\Program Files\Epic Games\UE_5.8
- 工程级开发基线：C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9-XFix1
- 已接受前置：P4x.0.r2、P5 的局外 Profile 实施交接、P6 r3 的 Prepared receipt 新 RunId rebind 功能实现，以及 P6.0.r4 的最终 Editor 编译收尾。

---

# 上半部分：只读项目裁决、现状与边界

## 1. 当前 P／F 阶段分界

| 阶段 | 负责内容 |
| --- | --- |
| P | 功能开发、必要的静态代码审查、目标代码编译与最小编译修正。 |
| F | 真实运行测试、CTA／wrapper、自动化与回归、截图／可见验收、Smoke、Game Build、Cook、Package 和最终验证。 |

不得为了证明 P7 而执行或新增任何 F 阶段工作。一次 Editor 代码编译是本任务唯一要求的执行性检查，不替代 F 的真实验证。

## 2. 已有所有权与数据边界

1. Code A 继续拥有默认地图、Player Actor、战斗、正式 Run 生命周期、旧库存、Loot、搜索、世界物品、结算、Run Save 和现有正式运行时。
2. Code B 的 P1 Repository 是新物品体系唯一可变真值。P5 是局外 Code B Profile snapshot；P6 是活动 Run Code B snapshot。不存在 A／B 镜像、同步或双写。
3. P6 仅在 Code A 已成功激活 Run 后，经后置、非阻塞 observer 获得稳定 OwnerId 和 RunInstanceId。Prepared receipt 的新 RunId rebind 已属于 P6；P7 不得重新实现、绕过、修改或调用其 recovery／bridge 语义。
4. P6 活动 Run 会话是 P7 的唯一物品输入。P7 读取该会话的 Projection 和状态，并只可通过既有 P4 → P3 → P2 → P1 事务链改变同一 P6 会话；不得创建另一套 Run inventory、另一种持久化模型或从 P5 重新抽取布局。
5. 局外 Profile 在活动 Run 存在时保持既有锁定语义。P7 不得解锁、修改或让局外页面与局内页面同时编辑同一批物品。

## 3. 持续有效的 P4x 物品规则

1. 旧备战已被删除为 Start Run 门槛。P7 不得恢复任何备战初始化、装备校验或开局阻断。
2. 普通储物位置包括基础 6 格、已装备空间戒指的内部格和空间储物囊的内部格；它们不按类别互斥。只有装备栏实施类别与单槽互斥。
3. 位置改变、Move、Swap、Merge、Replacement 和从装备栏移出的 Unequip 只能由真实 DragOperation 的明确 Drop 目标提交。不得新增 QuickMove、自动整理、放回、双击、右键、详情按钮、菜单、热键或隐藏命令的非拖拽位置写入入口。
4. 未处于装备栏的兼容可装备物品只显示“装备”；已处于装备栏的物品只显示“卸下”；不可装备物品两个状态均不显示。它们只用于指示明确拖拽目标，不能猜测目标、自动替换或自动放回。
5. 空间戒指只能位于空间戒指装备栏；成功装备后才显示其稳定 ChildContainerId 对应的“快捷空间（空间戒指）”。空间储物囊是非快捷独立容器。两者均不绑定 1—9、使用或 QuickMove。
6. 带内部内容的空间戒指或储物囊整体 Move、Swap、Equip 或 Unequip 必须继续由同一个 P1 原子拒绝；不得复制、丢失或重建其 ItemId、父位置或 ChildContainerId。

## 4. P7 产品裁决

P7 只完成“已进入一次正式 Run 后，玩家能够打开并整理自己的活动 Run 个人背包”的 UI／功能纵向切片。

它应复用 P3／P4 的生产 Cell、详情显示、拖拽与 Drop 架构，并改为面向匹配 OwnerId + RunInstanceId 的 P6 活动 Run 会话。该页面显示玩家自身的装备、基础 6 格、已装备空间戒指快捷空间与可打开的非快捷储物囊；它不是 Loot 双栏、尸体页面、宝箱页面或世界物品界面。

P7 的界面可以经 I 键或工程中已存在且不冲突的库存输入动作切换。开关页面本身不得写入 Repository、P5、P6 或 Code A。若当前 Code A Run 没有匹配的、已可用的 P6 活动会话，则输入必须保持非阻塞：不得创建、恢复、rebind、迁移或猜测任何 Code B 物品状态，也不得影响 Code A Run。

---

# 下半部分：授权执行内容

## 5. 单一授权目标

实现一个生产可达的 Code B 局内个人背包 Host 和最小输入／生命周期适配层，使其在 Code A 已成功运行中的场景内，安全地展示并整理已存在的 P6 活动 Run inventory session。

该实现必须满足：

1. Host 仅以当前成功 Code A Run 的稳定 OwnerId 与 RunInstanceId 查询匹配的 P6 会话；没有匹配会话时不得构造 fixture、临时 Repository、sidecar、P5 迁移或新 session。
2. 面板展示完整玩家布局：兵器、道袍、饰品、空间戒指、基础 6 格、已装备空间戒指的快捷空间，以及可打开的空间储物囊非快捷内部格。
3. 所有可接受的物品位置变化继续只经：

       已挂载生产 Cell 的真实 Drop
       → P4 Interaction Controller
       → P3 UI Controller
       → P2 Application Service
       → P1 Repository
       → 已有 P6 活动 Run 会话持久化边界

   不得让 Widget、Input Adapter、Player Actor、Code A UI 或详情控件直写 P1／P2／P5／P6。
4. P7 对 P6 Projection 是读取者，对经上述链提交的 P6 session 是唯一允许的写入消费者；每次接受的拖拽只能形成一次既有 P6 session 提交和一次相应 revision 演进。P7 不得另建镜像、缓存真值或 SaveGame 格式。
5. 打开、关闭、Esc、Cancel、无 ItemId 格、无匹配 session、无效 Drop、双击、右键、详情与禁用 1—9 占位均不得形成写入入口。仅保留既有的零写入到达语义，不在 P 阶段运行它们。
6. I 键／库存输入只管理 Code B Host 的可见性与输入焦点；不得改变 Code A 的 Start Run、地图、Player Actor、战斗或正式 HUD 的权威。

## 6. 允许的改动范围

允许：

- 新增或调整 Code B P7 局内个人背包 Host、Presenter、Projection Adapter、输入路由和必要的生产 UI 组件；
- 为精确匹配 P6 active session 增加最小的只读查询／会话 facade，或为已有 P6 commit 路径增加最小的 P7 调用适配；
- 仅为注册不冲突库存输入、挂载／卸载 UI Host 或转发已成功 Run 身份而修改最小 Code A UI／PlayerController／HUD 边缘层；
- 调整 P3／P4 的可复用 UI 组件，使其在不改变 P4x 事务语义的前提下可服务 P6 session；
- 更新 PROJECT.md、PROJECT_INFO_CARD.md 和本任务 Report，清楚记录 P5／P6／P7／P8／F 的所有权与 F 债务。

## 7. 明确不在本任务内

不得实现、启动、重构或接管：

- P6 bridge、Prepared receipt、RecoveredAbandon、新 RunId rebind、Start Run 成功条件或 Code A lifecycle；
- Player Actor 物品应用、属性／战斗影响、快捷栏 1—9、物品使用、消耗品；
- Loot、尸体／容器搜索、开启读条、世界物品、世界丢弃、拾取、地面 Actor 或局内双栏战利品页面；
- 撤离、死亡、结算、物品返还、关闭 Run session、返回 Profile、Run Save 或 P8 功能；
- P5 局外迁移、局外页面、备战、Code A 旧库存或任意 A／B 双写；
- 实际运行、CTA、wrapper、自动化、回归、截图、可见验收、Smoke、进程检查、Game target 编译、BuildCookRun、Cook 或 Package。

## 8. 静态代码审查与编译

完成实现后，只进行以下 P 阶段检查：

1. 审查 P7 的身份解析和会话查找，确认它只接受精确匹配的 OwnerId + RunInstanceId，缺失／不匹配时无写入且不影响 Code A。
2. 审查所有生产可达写路径，确认只有真实 Drop 可经 P4 → P3 → P2 → P1 改变 P6 活动会话；Input、详情、双击、右键、开关页面和失败路径均无直接写入。
3. 审查所有 Code A 改动，确认它们仅限输入／Host 生命周期边缘层，没有转移 Run、Player、Loot、搜索、结算、Run Save 或旧库存权威。
4. 编译一次 Editor 目标：

       "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_mapEditor Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex

5. 若编译失败，只修正 P7 引入的局部声明、include、类型、生命周期或调用签名问题，然后重新执行同一 Editor 目标。若修复需要越过本 Prompt 边界，停止受影响工作并报告，不得自行扩展到 P8 或 F。

## 9. Report 与完成信号

生成 Dev.D.UE.0.0.9B.P7.0.r0_report.md，保存至：

    C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Docs\Report

Report 必须简洁、可审计地列出：

1. P7 的新增／修改／未修改文件及其职责；
2. Host、输入动作、OwnerId + RunInstanceId 查询、Projection 读取与唯一 Drop 写入链；
3. P7 对 P4x 规则、P5 局外锁定和 P6 rebind 不变量的静态审查结论；
4. 实际 Editor 编译命令、目标、最终 exit code 和关键结果；
5. 明确列出本轮未执行的 F 阶段项目；
6. P1–P7 的真实运行、CTA、自动化、回归、截图、Smoke、Game Build、Cook、Package 与最终验证仍由 0.0.9B.F 负责。

仅当实现完成、静态边界审查通过、Editor 编译通过且未越界时，Report 可使用：

    READY_FOR_P8_FUNCTIONAL_WITH_F_DEBT

若仅存在当前范围内可修复的编译问题，使用：

    NEEDS_P7_COMPILE_REWORK

若需要新的策划裁决，使用：

    NEEDS_PLANNER_DECISION
    BLOCKED

完成后不得自动开始 P8、0.0.9B.F 或其他任务。向策划 Chat 回传并附带且只附带本次同名 Report；正文首行使用：

    [CSEMI:REPORT_SENT] {"task_id":"Dev.D.UE.0.0.9B.P7.0.r0","file":"Dev.D.UE.0.0.9B.P7.0.r0_report.md"}
