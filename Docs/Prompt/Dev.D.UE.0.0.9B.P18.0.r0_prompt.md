Exit code: 0
Wall time: 0.1 seconds
Output:
# Dev.D.UE.0.0.9B.P18.0.r0

## 任务身份

- 项目：Dev.D.UE.0.0.9B
- 阶段：P18——空间道具的受控普通容器来源与完整图转移接入
- 任务编号：Dev.D.UE.0.0.9B.P18.0.r0
- 任务性质：在 P16 已建立 BasicCache／BasicCorpse 的一次性确定性战利品 Profile，且 P17 已建立空间戒指／储物囊的真实 ChildContainer graph、P5↔P6↔P8 生命周期和 P7 薄投影后，让两类既有空间道具首次通过一个既有普通容器合法获得，并沿既有 P10 拖拽事务完整进入 P6。P18 只扩展 P16 的 BasicCache 未来首次物质化和 P10 的既有跨图事务对完整空间图的处理；不创建第二个来源、starter、地图 pickup、尸体来源、整包地面丢弃或任何 F 阶段验证。
- 执行文件：Dev.D.UE.0.0.9B.P18.0.r0_prompt.md
- 报告文件：Dev.D.UE.0.0.9B.P18.0.r0_report.md
- 活动工程根：C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B
- 活动工程：C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject
- 引擎：C:\Program Files\Epic Games\UE_5.8
- 工程级开发基线：C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9-XFix1
- 已接受前置：P4x.0.r2、P5、P6.0.r4、P7.0.r0、P8.0.r0、P9.0.r1、P10.0.r0、P11.0.r0、P12.0.r0、P13.0.r0、P14.0.r0、P15.0.r0、P16.0.r0 与 P17.0.r0。

---

# 上半部分：只读项目裁决、现状与边界

## 1. P／F 阶段分界

| 阶段 | 负责内容 |
| --- | --- |
| P | 功能开发、必要的静态代码审查、目标代码编译与最小编译修正。 |
| F | 真实运行测试、CTA／wrapper、自动化与回归、截图／可见验收、Smoke、Game Build、Cook、Package 和最终验证。 |

本任务不得为了证明 P18 而启动产品、运行测试、编写或执行测试专用路径、采集截图、执行回归、Smoke、进程检查、Game target 编译、Cook 或封包。一次 Editor 代码编译是本任务唯一要求的执行性检查，不替代 F 的真实验证。

## 2. 持续有效的真值与边界

1. Code A 继续拥有默认地图、普通容器与敌人 Actor 的生成和可见表现、交互距离、输入、Player Actor、战斗、生命、HUD、正式 Run 生命周期、终局分类、Run Save、旧库存、旧 Loot／搜索及尚未迁移的运行时。Code A 不得决定、物质化、保存、镜像、过滤或修改 Code B 空间道具、其 ChildContainer graph、P16 roll 或 P10 转移结果。
2. Code B P1 Repository 是新物品图唯一可变真值。P5 是局外 Profile snapshot；P6 是精确 `OwnerId + RunInstanceId` 的活动 Run 玩家携带 snapshot；P9 BasicCache 与 P11 BasicCorpse 是独立 Run-local 残余；P8 仅在 Code A 已提交终局后结算 P6 玩家图并丢弃未取得的 P9/P11 残余。不存在 A／B 镜像、同步或双写。
3. P10 已把唯一 `CodeB.NormalContainer.BasicCache` 接入真实地图的开启、逐件揭示和 P6↔P9 拖拽转移。P10 的 target identity、Open／Searching／Revealed 状态机、读条、UI、输入和拖拽入口不在 P18 中重做。P12 BasicCorpse、其死亡回执和 UI 也不在 P18 中扩展。
4. P16 的 `CodeB.LootProfile.BasicCache.r1` 与 `CodeB.LootProfile.BasicCorpse.r1` 已为旧 Profile 建立一次性、确定性、可审计的初始物质化。已 materialized 的任何 P9/P11 record 是不可改写的 Run 历史；其图、ItemId、ContainerId、数量、receipt、digest、ProfileId／Version 与 visibility 不得因 P18 重掷、替换、补料或迁移成新内容。
5. P17 已确定两种正式空间道具：快捷空间戒指 `Prototype.Item.Accessory.WindTalisman`（`Fdemo_mapItemIds::WindTalisman`，`Prototype.Slot.SpatialRing`，`QuickRing` child layout）与空间储物囊 `Prototype.Item.Backpack.Level1`（`Fdemo_mapItemIds::BackpackLevel1`，`Prototype.Slot.Backpack`，`StoragePouch` child layout）。其容量、Compatible slot 与唯一 `SpatialChildGuid(ItemId)` 必须继续完全由正式 Definition Catalog 和 P17 resolver 导出，绝不得由显示名、Widget、Actor、fixture 或 P16 seed 猜测。
6. P17 的一层限制持续有效：空间戒指／储物囊本体、其 ChildContainer 或任何后代不得装入另一空间道具，也不得形成自身或祖先回边。P18 只能生成一个位于 BasicCache 根容器的空间道具 parent；不得把空间道具或其 child graph 生成到另一个空间 ChildContainer。
7. P13 只保存稳定 ItemId 引用；P14 当前只处理 simple item 的地面路径，必须继续拒绝带 ChildContainer graph 的整包地面丢弃；P15 只使用仍在 P6 BaseQuick 的 simple `RestoreHealth`，并继续拒绝 complex parent。P18 不为空间道具增加 1—9、使用、装备效果、地面路径或快捷语义。

## 3. P18 产品裁决

P18 的单一纵向切片是：对未来首次物质化的唯一 P9 `CodeB.NormalContainer.BasicCache`，将既有 BasicCache 主材料 roll 保持不变，并追加一个低频、互斥、一次性的空间道具 optional roll；该 parent 及其正式 P17 ChildContainer graph 先作为 P9 真实内容生成，之后仅能经 P10 已有拖拽／P1 事务完整移入精确 P6 会话。

| 项目 | 本轮固定裁决 |
| --- | --- |
| 来源 | 仅未来未物质化的 `CodeB.NormalContainer.BasicCache`；不扩展 BasicCorpse、第二普通容器、地图 pickup、starter 或 P5 初始库存。 |
| Profile | 新建稳定版本 `CodeB.LootProfile.BasicCache.r2`；`BasicCorpse.r1` 不变。已物质化的 `BasicCache.r1` 永远保持历史结果。 |
| 原有主材料 | 保留 r1 的 `Guaranteed.Main`：`SpiritDust` weight 3、qty 2–3；`IronShard` weight 2、qty 1–2；选择次数仍为 1。 |
| 空间 optional group | 增加 `Optional.SpatialUtility`，以确定性 `NoDrop:9 / Spawn:1` gate 决定是否出现；Spawn 后恰好选择一个，`WindTalisman` weight 1、`BackpackLevel1` weight 3、quantity 1。 |
| 上限 | 一个 BasicCache 最多出现一个空间道具 parent；其 child graph 不计为第二份 source、不产生额外 roll，且不得含任何预装物品。 |
| 转移 | 仅复用 P10 已有的 P9→P6 拖拽入口与 P1 事务。成功时 parent 与完整 child graph 同时离开 P9、同时进入 P6；失败时保持 P9 原图，零写入。 |

上述权重、数量、candidate order、gate、ProfileVersion、algorithm version 与 digest 都是可审计的稳定内容定义。P18 不引入品质层、全局经济、保底、刷新、动态掉率、玩家等级修正、随机词条或其他掉落来源。

---

# 下半部分：授权执行内容

## 4. 单一授权目标

在 Code B 中将 BasicCache 的未来首次物质化升级为 r2 Profile，并让该 Profile 在合法命中空间道具时原子地生成一个完整的 P17 graph；最小扩展既有 P10/P1 事务，使已揭示的完整空间图可从 P9 移入 P6，而不改变 P10 的交互层或任何 Code A 权威。

### 4.1 BasicCache r2 Profile 与确定性生成

1. 保留现有 `CodeB.LootProfile.BasicCache.r1` 的兼容读取与历史验证。只有尚未物质化、且其 stable source identity 在未来第一次通过原 P9 gate 的 BasicCache 使用 `CodeB.LootProfile.BasicCache.r2`。`BasicCorpse.r1` 不得改变。
2. r2 必须把 r1 的 `Guaranteed.Main` material group 原样保留，并新增上述 `Optional.SpatialUtility`。optional gate 与候选选择都必须完全从 P16 已有持久 identity、`LootProfileId`、`ProfileVersion`、Profile digest 和明确递增的 deterministic algorithm version 导出；不得读取时钟、帧序号、Actor／世界坐标、UI、显示名、临时 GUID、旧 Code A Loot、网络状态或全局可变 RNG。
3. 当 optional group 未命中时，r2 图应只包含原主材料结果；当命中时，必须生成恰好一个合法 `WindTalisman` 或 `BackpackLevel1` parent，并在同一候选图中通过 P17 canonical resolver 添加唯一空 ChildContainer。ChildContainer 的 `ContainerId`、layout、capacity、slot legality 和 provenance 继续由该 parent 的正式 DefinitionId 导出。
4. P16/P17/P1 在提交前必须联合验证：Profile entry／weight／quantity 正确；BasicCache root 容量足够；parent 具有合法 DefinitionId；child graph 有唯一 owner、无自引用、无重复 child owner、无环、无空间道具嵌套、无非法 slot，且 child 为空。任何候选不合法、容量不足、definition 缺失、optional group 配置错误或 ChildContainer 不能完整创建，必须拒绝整次首次物质化，既不写 P9/P6/P5/P8，也不退化为空空间道具、simple substitute、starter、fallback recipe 或直接 P6 赠送。
5. 同一精确未物质化 BasicCache identity 的重开、Open／Close、搜索中断、Actor 销毁、query、P6 recovery／rebind、保存冲突或重复请求，只能读取已保存 r2 结果或零写入拒绝；不得消耗第二次 gate、重掷空间道具、添加 child、复制 parent、改写 stable ItemId 或产生第二根 P9 root。
6. 成功物质化必须继续在一个 Owner durable replacement 中同时保存：完整 P9 P1 graph、Hidden state、materialization receipt、r2 ProfileId／Version、algorithm version、Profile digest、result digest 和既有 target identity。不得先写 seed、gate、空 parent、半个 child graph 或 receipt，再尝试补生成内容。

### 4.2 P10 既有转移链对完整空间图的最小接入

1. 不改动 P10 的 target identity、UI、读条、揭示顺序、输入、拖拽状态机或拖拽入口。仅在它既有的 Code B P9→P6 Drop／Move 候选事务中，允许一个已揭示、合法、位于 BasicCache root 的 P17 spatial parent 及其完整 child graph 被视为一个不可拆分的图单元。
2. 写入前，复用 P1/P17 验证完整 source graph、精确 OwnerId + active RunInstanceId、P9 source identity、P6 destination placement、目的地容量、兼容 slot 和一层限制。目标是任何空间 ChildContainer、parent／descendant 或非法 slot 时必须拒绝整次候选，保持 source P9、P6、P7、P13、P14、P15 与 P8 不变。
3. 成功路径必须通过既有 Code B durable transaction：P9 中 parent 和它的全部 child graph 同时移除，P6 中同一稳定 parent／ContainerId／内部 ItemId 同时出现，P6 revision／digest 与 P9 receipt／digest 在同一合法提交中前进。不得 flatten child、复制 child、只移动 parent、重建 child 内容、生成新的 ItemId、第二次 clone 或建立 P10 Widget inventory。
4. P7 不新增入口或操作；它只继续从精确 P6 会话读取成功转移后的真实图。空间戒指仍只有位于 `Prototype.Slot.SpatialRing` 时才具有其既有快捷进入条件；储物囊仍由真实 P6 item entry 选择。P7 任一 transient projection 在 parent／child 未进入 P6、转移失败、会话关闭、stale 或 recovery 失效时保持关闭／丢弃，不得保留 P9 指针或复制内容。
5. P13/P15 保持现有资格规则，不获得空间道具绑定、使用或 child item 的新语义。P14 继续对 complex graph 零写入拒绝；整包地面丢弃／拾回必须留给后续单独任务。P12 BasicCorpse、其 Profile 与其 P11→P6 转移也完全不在本轮中扩展。

### 4.3 既有 P5／P6／P8 生命周期保护

1. 空间 parent 成功移入 P6 后，仅依 P17 已有 P5↔P6 bridge、P6 recovery/rebind 和 P8 terminal settlement 处理。P18 不建立新的 P5 source、direct grant、特殊 save 或 Code A mirror。
2. P8 的 Code A 后置 terminal authority、Extracted 的完整 P6 player graph 返回 P5、Dead／RecoveredAbandon 的既有没收、以及 P9 残余丢弃时序均不变。仍留在 BasicCache 的空间 parent／child graph 只能作为 P9 残余被完整丢弃；已完整进入 P6 的图只按既有 P8 结算。不得有 parent 或 child 单独泄漏回 P5。
3. 不修改 P9/P11 target identity、P10/P12 source路由、P13、P14、P15、Code A Run／Run Save／终局分类、旧库存、旧 Loot、地图、Actor、HUD、输入、战斗、生命、死亡或正式结算权威。

## 5. 允许的改动范围

允许：

- 在 Code B 内最小扩展 P16 Loot Profile Catalog、deterministic roll／digest、P9 BasicCache 初始 graph 生成与 legacy r1/r2 compatibility；
- 在 Code B P1/P17/P10 既有候选事务中最小补足完整合法 spatial graph 的跨容器转移、验证和同一 Owner durable transaction；
- 为 schema、序列化、调用签名或编译兼容最小调整 P7／P8／P13／P14／P15 的 Code B 声明或无效引用清理，但不得改变其产品语义；
- 更新 `PROJECT.md`、`PROJECT_INFO_CARD.md`、本任务 Prompt 归档和本任务 Report。

## 6. 明确不在本任务内

不得实现、启动、重构或接管：

- BasicCorpse 空间掉落、第二普通容器、第二尸体、全量地图容器、敌人、随机 Encounter、资源／灵石、P5 starter／初始库存、直接 P6 grant、地图 Actor、自动拾取、直接 Actor pickup、Code A 旧 Loot、地面多物品、整包地面丢弃／拾回、嵌套袋或空间道具预装内容；
- P10/P12 的 UI、交互、读条、揭示、拖拽状态机、Move/Merge/Swap 表现、可见表现或 target identity；
- P13 新快捷栏语义、P14 新地面来源、P15 之外的使用效果、空间装备效果、武器／道袍／饰品效果、技能、Buff／Debuff、动画、音效、战斗属性、网络同步或多人；
- Code A 的地图、Actor、输入、HUD、Player Actor、生命、战斗、死亡、Run、Run Save、终局分类、旧库存、旧 Loot／搜索或正式结算权威；
- 产品启动、CTA、wrapper、自动化、回归、截图、可见验收、Smoke、进程检查、Game target 编译、BuildCookRun、Cook 或 Package。

## 7. 静态代码审查与编译

完成实现后，只进行以下 P 阶段检查：

1. 审查 r2 仅影响未来未物质化 BasicCache，`BasicCache.r1`／`BasicCorpse.r1` 和所有 materialized 历史 record 保持不变；审查主材料 group、optional gate、candidate DefinitionId、weight、quantity、固定排序、ProfileVersion、algorithm version 和 digest。
2. 审查 P16 deterministic 输入与 r2 receipt／digest，确认相同精确身份绝不会因时钟、Actor、UI、临时 GUID、旧 Code A Loot 或全局 RNG 产生不同空间结果，也不存在第二次 gate／reroll／追加 child 路径。
3. 审查 P9 初始图：空间 parent 只位于 BasicCache root，最多一个，ChildContainer 为空且由 P17 Definition resolver 产生；容量、一层限制、无环、唯一 owner、非法配置和失败零写入均成立。
4. 审查 P10/P1 transaction：只通过既有实际拖拽入口完整移动 parent 加 child graph；不存在 parent-only、flatten、clone、new ItemId、direct P6 write、P10 Widget inventory 或 source/destination partial commit。
5. 审查 P5→P6→P8、P7 stale close、P13/P15 eligibility、P14 complex reject、P9/P11 residual discard 与 Code A authority；确认本轮不会导致 child 逃逸、P5 starter、地面路径、尸体来源或 Code A inventory／Loot authority。
6. 审查 Code A diff。除纯声明／编译兼容外，预期没有 Code A 改动；若有，必须逐文件说明其不涉及地图、Actor、Loot、搜索、输入、HUD、战斗、生命、Run、Run Save、结算或库存权威。
7. 编译一次 Editor 目标：

       "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_mapEditor Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex

8. 若编译失败，只修正 P18 引入的局部 Profile schema、deterministic roll、P9 graph materialization、P1/P10 complete-graph transaction、序列化、include 或调用签名问题，然后重新执行同一 Editor 目标。若修复需要扩展到 P19、0.0.9B.F、Code A 权威或其他功能，停止受影响部分并报告。

## 8. Report 与完成信号

生成 `Dev.D.UE.0.0.9B.P18.0.r0_report.md`，保存至：

    C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Docs\Report

Report 必须简洁、可审计地列出：

1. 本轮新增／修改／未修改的每个文件及职责；
2. BasicCache r2 与 BasicCorpse r1／legacy r1 的版本关系，r2 主材料 group、`Optional.SpatialUtility` gate、候选 DefinitionId、weight、quantity、排序、algorithm version 与 digest；
3. r2 的完整 deterministic identity 输入，以及为何同一未物质化 target 不会重掷、追加空间 parent 或创建第二个 child graph；
4. P9 首次 materialization 的完整 parent／ChildContainer graph、容量／一层限制／空 child、同一 Owner durable save、receipt／digest，以及 legacy materialized record 如何保持原样；
5. P10 实际既有拖拽事务如何完整移动 source P9 graph 到精确 P6，成功／失败／冲突／重复路径如何避免 parent-only、flatten、clone、new ItemId 或 partial commit；
6. P7、P5/P6/P8、P13/P14/P15、P11/P12、Code A authority 的静态边界结论；
7. 实际 Editor 编译命令、目标、最终 native exit code 和关键结果；
8. 明确列出未执行的 F 阶段项目：真实 BasicCache r2 roll、optional 命中／未命中、P10 空间图转移、P7 打开／返回、P8 三种终局、recovery、自动化、回归、截图、Smoke、Game Build、Cook、Package 与最终验证均由 `0.0.9B.F` 负责；
9. 明确列出尚未启动的功能：BasicCorpse 空间来源、第二来源／全量迁移、整包地面丢弃／拾回、嵌套袋、空间装备效果、武器／道袍／饰品效果、其他消耗品及后续 P 阶段。

仅当实现完成、静态边界审查通过、Editor 编译以 native exit code `0` 完成且未越界时，Report 可使用：

    READY_FOR_NEXT_P_FUNCTIONAL_WITH_F_DEBT

若仅存在当前范围内可修复的编译问题，使用：

    NEEDS_P18_COMPILE_REWORK

若现有 BasicCache／P16／P17／P10 无法在不改写 materialized 历史、不扩展 Code A 权威或不创建第二来源的前提下形成合法完整图，使用：

    NEEDS_PLANNER_DECISION
    BLOCKED

完成后不得自动开始 P19、`0.0.9B.F` 或其他任务。向策划 Chat 回传并附带且只附带本次同名 Report；正文首行使用：

    [CSEMI:REPORT_SENT] {"task_id":"Dev.D.UE.0.0.9B.P18.0.r0","file":"Dev.D.UE.0.0.9B.P18.0.r0_report.md"}


