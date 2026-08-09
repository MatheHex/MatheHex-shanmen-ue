# Dev.D.UE.0.0.9B.P16.0.r0

## 任务身份

- 项目：Dev.D.UE.0.0.9B
- 阶段：P16——既有普通容器与身体容器的确定性加权战利品池
- 任务编号：Dev.D.UE.0.0.9B.P16.0.r0
- 任务性质：在 P9/P10 的一个生产普通容器、P11/P12 的一个生产身体容器、P13 快捷栏、P14 地面丢弃／拾回和 P15 单一 RestoreHealth 使用链已建立后，将两个既有 Run-local 内容计划从固定 recipe 扩展为可审计的、每 Run 一次的确定性加权物质化。P16 只改 Code B 的内容生成层和既有 P9/P11 首次物质化调用；不新增地图目标、敌人、UI、输入、玩家物品写入入口或 Code A 战利品真值。P 阶段只做功能开发、静态代码审查、Editor 代码编译及必要的最小编译修正；真实运行、随机性／可达性验证、截图、回归、Smoke、Game Build、Cook、Package 与最终验收统一留给 0.0.9B.F。
- 执行文件：Dev.D.UE.0.0.9B.P16.0.r0_prompt.md
- 报告文件：Dev.D.UE.0.0.9B.P16.0.r0_report.md
- 活动工程根：C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B
- 活动工程：C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject
- 引擎：C:\Program Files\Epic Games\UE_5.8
- 工程级开发基线：C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9-XFix1
- 已接受前置：P4x.0.r2、P5、P6.0.r4、P7.0.r0、P8.0.r0、P9.0.r1、P10.0.r0、P11.0.r0、P12.0.r0、P13.0.r0、P14.0.r0 与 P15.0.r0。

---

# 上半部分：只读项目裁决、现状与边界

## 1. P／F 阶段分界

| 阶段 | 负责内容 |
| --- | --- |
| P | 功能开发、必要的静态代码审查、目标代码编译与最小编译修正。 |
| F | 真实运行测试、CTA／wrapper、自动化与回归、截图／可见验收、Smoke、Game Build、Cook、Package 和最终验证。 |

本任务不得为了证明 P16 而启动产品、运行测试、编写或执行测试专用路径、采集截图、执行回归、Smoke、Game target 编译、Cook 或封包。一次 Editor 代码编译是本任务唯一要求的执行性检查，不替代 F 的真实验证。

## 2. 持续有效的真值与边界

1. Code A 继续拥有默认地图、普通容器与敌人 Actor 的生成／可见表现、交互距离、输入、Player Actor、战斗、死亡、HUD、正式 Run 生命周期、终局分类、Run Save、旧库存、旧 Loot／搜索及所有尚未迁移的运行时。P16 默认不需要 Code A 改动；不得让 Code A 持有、滚动、补造、保存或筛选 Code B 战利品内容。
2. Code B P1 Repository 是新物品图唯一可变真值。P5 是局外 Profile snapshot；P6 是精确 `OwnerId + RunInstanceId` 的活动 Run 玩家携带 snapshot；P9 的 NormalContainer 与 P11 的 BodyContainer 是独立的 Run-local 残余内容；P8 仅在 Code A 已提交终局后结算 P6 玩家图并丢弃未取得的 Run-local 残余。不存在 A／B 镜像、同步或双写。
3. P10 已把唯一 `CodeB.NormalContainer.BasicCache` 接入真实地图的开启、逐件揭示和 P6↔P9 拖拽转移；P12 已把唯一 `CodeB.BodyContainer.BasicCorpse` 接入死亡后尸体的开启、逐件揭示和 P6↔P11 拖拽转移。P16 只改变二者**首次物质化的内容来源**，不改变它们的稳定目标身份、Open／Searching／Revealed 状态、UI、计时、拖拽或跨图事务。
4. P13 的 1—9 仅保存稳定 `ItemId` 引用；P14 的地面记录仅保存玩家已经放下的 P6 物品；P15 只可使用已绑定且仍在 P6 BaseQuick 的既有 RestoreHealth 消耗品。P16 不新增快捷栏写入、地面来源、自动拾取、直接 Actor pickup、使用效果、装备效果或 Code A 库存镜像。
5. 已经成功物质化的 P9/P11 record 是不可改写的 Run 历史。它的 `ContainerId`、`ItemId`、数量、ChildContainer graph、首次物质化 receipt／digest 与 Hidden／Searching／Revealed 状态不得因 P16 被重新掷数、替换、补料、清空或迁移为新内容。

## 3. P16 产品裁决

P16 只为下列两个**既有**来源建立最小的生产级确定性加权 Loot Profile：

| 既有来源 | 既有稳定身份 | P16 新增内容来源 |
| --- | --- | --- |
| P9 普通容器 | 已存在的 `CodeB.NormalContainer.BasicCache` SearchTarget／Definition 组合 | `CodeB.LootProfile.BasicCache.r1` 或等价稳定 ProfileId |
| P11 身体容器 | 已存在的 `CodeB.BodyContainer.BasicCorpse` BodyTarget／Definition／DeathReceipt 组合 | `CodeB.LootProfile.BasicCorpse.r1` 或等价稳定 ProfileId |

每个未物质化的精确目标在其第一次合法 P9/P11 物质化时，只从自己的 Profile 生成一次真实 P1 物品图；同一 Run 内的重开、重复死亡回执、界面刷新、Actor 销毁、读档恢复或重复查询均只读取同一结果，不重新掷数。不同 Run 可以得到不同合法结果，但 P16 不为此新增 Run、目标、敌人、地图物件或玩家初始物品。

Profile 只能引用当前正式 Code B Definition Catalog 中已存在、合法且可放入对应容器的 DefinitionId。若当前已有正式 `RestoreHealth` 消耗品定义，可把它作为具名的、受数量和权重限制的候选之一；不得新建 starter grant、P5 初始库存、演示物品、测试 ItemId、Code A 物品表、固定 UI mock 或独立 fixture 来伪造可达性。

---

# 下半部分：授权执行内容

## 4. 单一授权目标

实现一个由 Code B 独占的声明式 Loot Profile 与一次性 deterministic roll 服务，并将其接入 P9 BasicCache 与 P11 BasicCorpse 的既有首次物质化事务。一次成功提交必须在同一 Owner durable replacement 内产生完整 P1 图、Profile provenance、内容 digest 和既有 materialization receipt；它不是重新发明 P9/P11 容器、不是玩家 Move，也不是即时掉落或奖励系统。

### 4.1 声明式 Profile、权重与确定性随机

1. 在明确的 Code B 边界内新增或最小扩展 Loot Profile Catalog。每个 Profile 至少稳定表达：`LootProfileId`、不可变 Profile version／digest 输入、一个或多个有序 roll group、每组合法的选择次数、候选 `DefinitionId`、正整数 weight、合法数量范围与确定性 tie-break 顺序。内部类型、目录和命名可遵循当前工程风格，但不得用 Widget 文本、Actor 属性、显示名、运行时指针或世界坐标作为内容定义。
2. `BasicCache` 与 `BasicCorpse` 只能各绑定一个可审计的 Profile。每个候选必须满足现有 P1 定义规则、MaxStack、容器容量、装备／child-container 规则和内容布局要求；不合法、未知、零／负权重、零／负数量、溢出数量、重复的 ProfileId／group/entry identity 或无法放置的定义必须在写入前拒绝。
3. 每次首次 roll 的伪随机输入必须完全由持久、精确且本次物质化已知的 identity 导出：`OwnerId + RunInstanceId + SearchTargetId 或 BodyTargetId + 容器 DefinitionId + DeathReceiptId（仅尸体）+ LootProfileId + ProfileVersion`，再加固定算法版本。不得读取系统时钟、帧序号、Actor 随机状态、UI、玩家显示名、临时 GUID、旧 Code A Loot、网络状态或全局可变 RNG。
4. 同一输入、同一 Profile version 与同一实现版本必须选择相同候选、数量、布局顺序和 digest。需处理同权重时，使用固定排序／固定序号，而非容器遍历、哈希容器偶然顺序或地址顺序。Profile 改版只能影响**以后首次物质化的新 Run**；已物质化 record 永不 reroll。
5. P16 不要求复杂掉率系统。不要加入品质层、稀有词缀、货币散落、全局 Economy、保底、刷新、补货、重掷券、动态难度、玩家等级修正、连杀修正、多人同步或防刷策略。Profile 可以在每个来源使用一个固定必出 group 和至多一个受限 optional group；更复杂的内容经济留待后续策划。

### 4.2 P9／P11 首次物质化接入与迁移

1. 审阅 P9 BasicCache 与 P11 BasicCorpse 当前的固定 recipe、首次 materialization receipt、definition digest、identity gate 和 Owner durable transaction。把它们的**未物质化**路径改为调用同一 Code B Loot Profile 服务；不得更改 SearchTargetId、BodyTargetId、DeathReceiptId、P10/P12 状态机或任一既有 target 的地图路由。
2. 每次请求仍必须先通过各自已有的 exact identity gate：匹配 OwnerId、committed Active P6、RunInstanceId、目标身份、DefinitionId，尸体额外通过原 DeathReceiptId。只有通过 gate 后才能在候选内 roll；roll 本身不产生持久副作用。
3. 成功路径在同一 Owner durable replacement 内完成：读取合法 Profile → 用确定性输入建立候选结果 → 创建对应 P1 Container／Item／ChildContainer 图 → 以既有 P9 或 P11 的初始 Hidden 状态登记物品 → 保存 ProfileId、ProfileVersion、roll algorithm version、首次结果 digest 与既有 materialization receipt／digest → 验证完整 P1/P9/P11/P6 文档 → 一次保存。不得先保存 seed、半图、空容器或 receipt，再尝试补生成内容。
4. 已物质化的旧 P9/P11 record 必须保留原固定 recipe 的真实结果。迁移只能补足“legacy materialized”的 provenance 标记或兼容读逻辑；不得依据新 Profile 重写旧 ItemId、数量、ContainerId、ChildContainerId、visibility、状态、receipt 或 digest。旧的未物质化兼容 record 只可在其未来第一次合法物质化时使用明确的当前 Profile version。
5. 重复 P9 materialize、P11 重放同一 death receipt、重复 query、Open／Close、搜索中断、Actor 销毁、P6 recovery/rebind 或保存冲突必须返回既有 record 或零写入拒绝；绝不得消耗第二次 roll、追加物品、改写 ordinal 或制造同一目标的第二根 ContainerId。
6. 若 Profile、布局或物品定义不能在不超出当前来源容量与 P1 不变量的前提下形成合法完整图，必须拒绝整次物质化，保持既有 record／P6／P5／P8 不变，并在 Report 使用 `NEEDS_PLANNER_DECISION`。不得退化为空箱、临时 fallback recipe、随机 P6 赠送、旧 Code A Loot 或 UI placeholder。

### 4.3 生命周期、领取与权威保护

1. P10/P12 应继续只消费各自已物质化 record 的真实图与 Hidden／Searching／Revealed 状态。P16 不改动其打开、关闭、读条、拖拽、Move、Merge、Swap、玩家↔容器事务或现有 UI payload；它们不拥有 Profile、seed、权重或重新掷数入口。
2. 物品只有在 P10/P12 的现有跨图 Drop 事务成功后才进入 P6 玩家图，随后才可能被 P13 绑定、P14 主动丢下或 P15 使用。P16 不允许 Roll 直接写入 P6、P5、P13、P14、P15 receipt、Code A Actor、Run Save、旧库存或 HUD。
3. P8 的 Extracted／Dead／RecoveredAbandon 语义、Code A 后置 observer、session close、P5↔P6 bridge 与 Player-only settlement 一律保持不变。P8 应继续丢弃未取得的 P9/P11 残余，无论残余来自 legacy fixed recipe 还是 P16 Profile；已移入 P6 的物品仍只按既有 P8 规则结算。
4. P16 不增加 Code A 的死亡、击杀奖励、普通容器打开、旧 Loot、敌人生成、地图 Actor、世界掉落、玩家输入、HUD、战斗、生命、终局或 Run Save 权威。若发现无法在 Code B 内接入而必须让 Code A 保存或决定内容，停止受影响工作并报告，不得建立双写或同步镜像。

## 5. 允许的改动范围

允许：

- 在 Code B 内新增／修改 Loot Profile Catalog、确定性 roll、profile digest／provenance、P9/P11 物质化适配、最小 schema migration 与 Owner durable transaction；
- 为兼容既有 materialized record 增加只读 provenance／版本化读取，而不修改其真实图；
- 仅为编译兼容而最小调整 P9/P11 query 或现有 Code B 投影声明；
- 更新 `PROJECT.md`、`PROJECT_INFO_CARD.md`、本任务 Prompt 归档和本任务 Report。

## 6. 明确不在本任务内

不得实现、启动、重构或接管：

- 第二个普通容器、第二具尸体、所有地图容器、所有敌人、随机 Encounter、资源／灵石掉落、自动拾取、直接 Actor pickup、地面多物品、P14 新来源、P5 starter／初始库存或 Code A 旧 Loot；
- P10/P12 的 UI、交互、读条、拖拽、Move、Merge、Swap、搜索状态机、可见表现或目标 identity；
- P13 绑定、P14 丢弃／拾回、P15 RestoreHealth、其他消耗品、武器使用、装备效果、空间物品使用、技能、Buff／Debuff、动画、音效、战斗属性、网络同步或多人；
- P5/P6 bridge、Prepared receipt、RecoveredAbandon rebind、P8 核心终局语义、Profile lock、Code A 的地图、Actor、战斗、死亡、HUD、Run、Run Save、旧库存或正式结算权威；
- 产品启动、CTA、wrapper、自动化、回归、截图、可见验收、Smoke、进程检查、Game target 编译、BuildCookRun、Cook 或 Package。

## 7. 静态代码审查与编译

完成实现后，只进行以下 P 阶段检查：

1. 审查两个 Profile 的稳定 identity、合法 entries、权重／数量边界、固定排序、版本／digest 与仅引用既有 Code B Definition 的结论。
2. 审查 deterministic roll 输入，确认同一精确身份和 Profile version 不会依赖时钟、Actor、UI、临时 GUID、旧 Code A Loot 或全局随机状态；已物质化 record 不存在 reroll 路径。
3. 审查 P9/P11 事务，确认完整 P1 图、Hidden 状态、Profile provenance、materialization receipt 与 digest 只在同一 Owner durable replacement 中一次保存；失败、冲突、重复请求和非法 layout 零写入。
4. 审查 legacy materialized record 的兼容处理，确认其真实历史保持原样；P10/P12、P13、P14、P15、P5/P6、P8 的既有语义没有被改写。
5. 审查 Code A diff。除纯声明／编译兼容外，预期没有 Code A 改动；若有，必须逐文件说明其不涉及任何地图、Actor、Loot、搜索、输入、战斗、Run、Run Save、结算或库存权威。
6. 编译一次 Editor 目标：

       "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_mapEditor Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex

7. 若编译失败，只修正 P16 引入的局部声明、include、Profile schema、序列化、确定性 roll、P9/P11 materialization 或调用签名问题，然后重新执行同一 Editor 目标。若修复需要扩展到 P17、0.0.9B.F、Code A 权威或其他功能，停止受影响部分并报告。

## 8. Report 与完成信号

生成 `Dev.D.UE.0.0.9B.P16.0.r0_report.md`，保存至：

    C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Docs\Report

Report 必须简洁、可审计地列出：

1. 本轮新增／修改／未修改的每个文件及职责；
2. BasicCache 与 BasicCorpse 的 ProfileId、版本／digest、roll group、候选 DefinitionId、权重／数量边界，以及没有新增 starter、P5 初始库存、fixture 或 Code A Loot 的结论；
3. deterministic roll 的完整身份输入、算法版本、固定排序与为何相同输入不会出现不同结果；
4. P9/P11 的首次事务、一次 Owner durable save、P1 图／Hidden state／receipt／digest、重复／冲突／失败零写入结论；
5. legacy materialized record 如何保持原结果，以及新 Profile 只影响未来首次物质化；
6. P10/P12 领取、P13/P14/P15 后续消费、P8 残余丢弃和 Code A 权威的静态边界结论；
7. 实际 Editor 编译命令、目标、最终 native exit code 和关键结果；
8. 明确列出未执行的 F 阶段项目：真实多 Run roll、同 Run 重开、重复 death／open、P10/P12 转移、P15 消耗品可达性、P8 三种终局、恢复、自动化、回归、截图、Smoke、Game Build、Cook、Package 与最终验证均由 0.0.9B.F 负责；
9. 明确列出尚未启动的功能：第二来源／全量迁移、复杂掉率经济、装备／武器使用、其他消耗品、空间物品使用及后续 P 阶段。

仅当实现完成、静态边界审查通过、Editor 编译以 native exit code `0` 完成且未越界时，Report 可使用：

    READY_FOR_NEXT_P_FUNCTIONAL_WITH_F_DEBT

若仅存在当前范围内可修复的编译问题，使用：

    NEEDS_P16_COMPILE_REWORK

若当前正式 Definition Catalog 无法组成合法 Profile、现有 P9/P11 record 无法在不改写旧历史的前提下兼容，或接入必须转移 Code A Loot／地图／Actor 权威，使用：

    NEEDS_PLANNER_DECISION
    BLOCKED

完成后不得自动开始 P17、0.0.9B.F 或其他任务。向策划 Chat 回传并附带且只附带本次同名 Report；正文首行使用：

    [CSEMI:REPORT_SENT] {"task_id":"Dev.D.UE.0.0.9B.P16.0.r0","file":"Dev.D.UE.0.0.9B.P16.0.r0_report.md"}


