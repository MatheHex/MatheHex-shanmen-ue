# Dev.D.UE.0.0.9B.P20.0.r0

## 任务身份

- 项目：Dev.D.UE.0.0.9B
- 阶段：P20——单一尸体容器的空间道具确定性来源与完整图取得
- 任务编号：Dev.D.UE.0.0.9B.P20.0.r0
- 任务性质：在 P16 已为唯一 BasicCache 与唯一 BasicCorpse 建立一次性确定性战利品 Profile、P17 已建立空间戒指／储物囊的正式 ChildContainer graph、P18 已让 BasicCache 空间 parent 经 P10 完整进入 P6、P19 已让该完整图经 P14 单 root WorldDrop 往返后，最小扩展仍未启动的 BasicCorpse 路径。P20 只让未来首次物质化的单一 BasicCorpse 能以确定性 Profile 产生一个空的正式空间 parent graph，并仅经既有 P12 尸体双栏的真实拖拽完整进入 P6。不得创建第二尸体、改写旧尸体历史、扩展尸体 UI／交互、加入新地图／掉落来源、装备效果、地面 child 操作或任何 F 阶段验证。
- 执行文件：Dev.D.UE.0.0.9B.P20.0.r0_prompt.md
- 报告文件：Dev.D.UE.0.0.9B.P20.0.r0_report.md
- 活动工程根：C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B
- 活动工程：C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject
- 引擎：C:\Program Files\Epic Games\UE_5.8
- 工程级开发基线：C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9-XFix1
- 已接受前置：P4x.0.r2、P5、P6.0.r4、P7.0.r0、P8.0.r0、P9.0.r1、P10.0.r0、P11.0.r0、P12.0.r0、P13.0.r0、P14.0.r0、P15.0.r0、P16.0.r0、P17.0.r0、P18.0.r0 与 P19.0.r0。

---

# 上半部分：只读项目裁决、现状与边界

## 1. P／F 阶段分界

| 阶段 | 负责内容 |
| --- | --- |
| P | 功能开发、必要的静态代码审查、目标代码编译与最小编译修正。 |
| F | 真实运行测试、CTA／wrapper、自动化与回归、截图／可见验收、Smoke、Game Build、Cook、Package 和最终验证。 |

本任务不得为了证明 P20 而启动产品、运行测试、编写或执行测试专用路径、采集截图、执行回归、Smoke、进程检查、Game target 编译、Cook 或封包。一次 Editor 代码编译是本任务唯一要求的执行性检查，不替代 F 的真实验证。

## 2. 持续有效的真值、来源与边界

1. Code A 继续拥有地图、敌人 Actor 的生成／死亡／可见表现、尸体交互距离与即时生命周期、原始输入派发、Player Actor、战斗、生命、HUD、正式 Run 生命周期、终局分类、Run Save、旧库存、旧 Loot／搜索及所有尚未迁移运行时。Code A 不得决定、物质化、保存、镜像、复制、拆分或结算 Code B 空间物品图；P20 预期不需要 Code A 功能改动。
2. Code B P1 Repository 是新物品体系唯一可变真值。P5 是局外 Profile snapshot；P6 是精确 OwnerId + RunInstanceId 的活动 Run 玩家与 WorldDrop snapshot；P9 BasicCache 与 P11 BasicCorpse 是独立 Run-local 残余。P8 只在 Code A 已提交终局后结算 P6 玩家图并丢弃 P9、P11、P14 残余。不存在 A／B 镜像、同步或双写。
3. P11 只定义一个正式生产身体来源：CodeB.BodyContainer.BasicCorpse，绑定 M01.Encounter.LOW.Skirmisher.01、M01.Spawn.LOW.Skirmisher.01、ordinal 0 与 M01.BodyTarget.LOW.Skirmisher.01.Ordinal.0。其 BodyTargetId 由稳定静态路线导出；DeathReceiptId 由完整 OwnerId、RunInstanceId、BodyTargetId 和 DefinitionId 导出。P20 不增加第二尸体、第二敌人、随机 Encounter 或全量迁移。
4. P12 已将上述已提交死亡回执后的 P11 record 接入既有生产尸体交互与 P7 BodyContainerTarget。它的 Hidden → Searching → Revealed 状态、读条、页面生命周期、真实 NativeOnDrop 写入链和目标身份均不得重做。P20 只在其现有 Code B P11 ↔ P6 候选事务中识别一个已揭示的完整空间 parent root。
5. P16 的 BasicCache.r1 与 BasicCorpse.r1 是历史 Profile。任何已经 materialized 的 P9／P11 record 都是不可改写的 Run 历史：其图、ItemId、ContainerId、数量、receipt、digest、ProfileId、ProfileVersion、visibility 与 action 状态不得重掷、补料、替换或迁移为 r2。
6. P17 已定义两个本任务可用的正式空间 parent：Prototype.Item.Accessory.WindTalisman（Fdemo_mapItemIds::WindTalisman，Prototype.Slot.SpatialRing，QuickRing child layout）与 Prototype.Item.Backpack.Level1（Fdemo_mapItemIds::BackpackLevel1，Prototype.Slot.Backpack，StoragePouch child layout）。稳定 SpatialChildGuid(ItemId)、child type、容量、slot legality 与 provenance 必须继续由正式 Definition Catalog／P17 canonical resolver 导出，不得由显示名、Widget、Actor、尸体路线、坐标、fixture 或随机 seed 猜测。
7. P17 的一层限制持续有效：空间 parent、其 ChildContainer 或其任何后代不得装入另一个空间道具，也不得形成自身或祖先回边。P20 创建的空间 parent 只能拥有一个正式、空的 ChildContainer；不得预装普通物品、创建 nested bag 或让 child graph 成为尸体中第二个独立来源。
8. P18 的 BasicCache.r2、P10 交互、P19 WorldDrop 完整图、P13 快捷绑定、P14 simple branch、P15 RestoreHealth、P5/P6 bridge、P7 空间投影与 P8 终局权威均已存在并保持产品语义不变。P20 不修改其来源、交互、权威或可达性；成功进入 P6 的图只是继续被既有链消费。

## 3. P20 产品裁决

P20 的单一纵向切片是：未来第一次因同一已提交 BasicCorpse 死亡回执而物质化的 P11 record，保留原 BasicCorpse 主材料 roll，并追加一个低频、互斥、一次性的空间道具 optional roll。若命中，该尸体根容器出现一个初始 Hidden 的 WindTalisman 或 BackpackLevel1 parent 及其唯一空 ChildContainer；当该 parent 已由既有 P12 流程揭示后，玩家只能从同一个 BodyContainerTarget 的 parent root cell 真实拖到一个空的 P6 BaseQuick cell，以一个不可拆分 P1 graph 进入活动 Run。

| 项目 | 本轮固定裁决 |
| --- | --- |
| 来源 | 仅未来未物质化的唯一 CodeB.BodyContainer.BasicCorpse；不扩展 BasicCache、第二尸体、地图 pickup、starter、P5 初始库存或直接 P6 grant。 |
| Profile | 新建稳定版本 CodeB.LootProfile.BasicCorpse.r2；AlgorithmVersion 固定为 CodeB.DeterministicWeightedLoot.Crc32.r2；BasicCache.r1/r2 与 BasicCorpse.r1 及全部历史 record 保持原样。 |
| 原有主材料 | 保留 r1 的 Guaranteed.Main：IronShard weight 3、quantity 1–2；SpiritDust weight 2、quantity 1；选择次数仍为 1。 |
| 空间 optional group | 增加 Optional.SpatialUtility：确定性 NoDrop:9 / Spawn:1 gate；Spawn 后恰好选择一个，WindTalisman weight 1、BackpackLevel1 weight 3、quantity 1。 |
| 容量与子图 | BasicCorpse 根容器最多包含一个主材料结果和一个空间 parent；parent 的 ChildContainer 为空且不占第二个来源／roll，不得生成预装内容。 |
| 取得 | 仅复用 P12 已有的已揭示 source root → 真实 NativeOnDrop → 空 P6 BaseQuick target。空间 parent 不得 Merge、Swap、Split、QuickMove、直接装备、自动获得或从 child cell 转移。 |
| 反向移动 | P20 不让空间 parent 或其 child 从 P6 放回 BasicCorpse；P12 既有 simple-item 双向行为不改写。 |

上述权重、数量、candidate order、gate、ProfileVersion、algorithm version 与 digest 都是可审计的稳定内容定义。P20 不引入品质层、保底、刷新、动态掉率、等级修正、词条、第二世界来源或可变 RNG。

---

# 下半部分：授权执行内容

## 4. 单一授权目标

在 Code B 中把未来首次物质化的唯一 BasicCorpse 升级为 r2 Profile，并最小扩展已有 P12／P1 跨图候选事务，使一个已揭示、合法且完整的空间 parent graph 能从 P11 一次性进入精确 P6 的空 BaseQuick。实现不得改变 P12 的尸体交互层，不得让 Code A 取得库存权威，也不得把 child 内容拆开、提前开放或暴露为独立尸体／地面物品。

### 4.1 BasicCorpse r2 Profile 与确定性完整图生成

1. 保留 CodeB.LootProfile.BasicCorpse.r1 的兼容读取、历史验证和所有已 materialized P11 record。只有尚未 materialized、且在既有 P11 death-receipt gate 后首次进入物质化的 BasicCorpse 使用 CodeB.LootProfile.BasicCorpse.r2。不得改写 BasicCache.r1/r2。
2. r2 必须把 r1 Guaranteed.Main 原样保留，并仅新增上述 Optional.SpatialUtility。optional gate 与候选选择必须只从 P16 的稳定身份、LootProfileId、ProfileVersion、ProfileDigest、明确递增的 deterministic algorithm version 及 BasicCorpse 原有 receipt identity 导出；不得读取时钟、帧号、Actor 指针、世界坐标、UI、显示名、临时 GUID、旧 Code A Loot、网络状态或全局可变 RNG。
3. 完整 r2 roll identity 至少包含 OwnerId、RunInstanceId、BodyTargetId、BasicCorpse source DefinitionId、DeathReceiptId、LootProfileId、ProfileVersion、ProfileDigest 与 AlgorithmVersion。相同精确 identity 必须复建同一 group 命中、候选、数量、slot、stable ItemId、SpatialChildGuid 与 result digest。
4. optional 未命中时，r2 只生成原主材料结果。命中时必须额外生成恰好一个合法 WindTalisman 或 BackpackLevel1 parent，并在同一候选 P1 graph 中经 P17 canonical resolver 创建其唯一、空的正式 ChildContainer。parent、child ContainerId、layout、capacity、slot legality 和 provenance 必须全部从正式 DefinitionId 导出。
5. P16／P17／P1 必须在保存前联合验证：Profile entry、weight、quantity、固定排序、BasicCorpse 根容量、parent DefinitionId、ChildContainer semantic／capacity、唯一 owner、无自引用、无环、无 duplicate child owner、无 nested spatial item、无非法 slot，且 child 为空。任何候选不合法、容量不足、definition 缺失、optional group 配置错误、ChildContainer 不能完整创建或 receipt 冲突，均必须拒绝整次首次物质化，零写入 P11/P6/P5/P8，并且不得退化为空 parent、simple substitute、starter、fallback recipe 或直接 P6 赠送。
6. 成功物质化必须继续在一个 Owner durable replacement 中共同保存：完整 P11 P1 graph、所有初始 item 的既有 Hidden state、materialization receipt、r2 ProfileId／Version、AlgorithmVersion、ProfileDigest、result digest 与原有 BodyTarget／DeathReceipt identity。不得先保存 seed、gate、空 parent、半个 child graph 或 receipt，再补写内容。
7. 同一 corpse 的 Open／Close、搜索中断、Actor 销毁、query、P6 recovery／rebind、保存冲突或重复 death receipt 只能读取已保存 r2 结果或零写入拒绝；不得第二次 gate、reroll、追加 parent、替换 child、重写稳定 ID 或创建第二个尸体 root。

### 4.2 P12 既有尸体转移链对完整空间图的最小接入

1. 不改动 P12 的目标身份、打开／读条／揭示状态机、输入、尸体 actor、页面生命周期、可见布局、普通物品 Move/Merge/Swap 规则或 NativeOnDrop 入口。仅在既有 Code B P11 → P6 Drop 候选事务中，允许一个已经 Revealed、位于该 BasicCorpse root 的 WindTalisman 或 BackpackLevel1 parent 连同其完整、空 ChildContainer graph 作为一个不可拆分图单元。
2. BodyContainerTarget 继续只把该 parent 表现为单个 root item cell；不得显示、选择、拖拽、转移、搜索、编辑或详情展开其 ChildContainer／child cells。Hidden 或 Searching parent 仍不得具有 ItemId payload、详情或任何写入资格。
3. P20 唯一合法 destination 是当前精确 OwnerId + RunInstanceId P6 的一个空 BaseQuick cell。必须由 P1／P17 验证活动 session、P11 source identity、DeathReceipt、source/destination revisions、Reveal/Open gate、root placement、child closure、BaseQuick 空位与容量，再由现有 production NativeOnDrop 形成请求。不得为本任务开放直接装备到 SpatialRing／Backpack、Swap、Merge、Split、QuickMove、auto-sort、按钮领取、双击、右键、Actor direct pickup、keyboard pickup 或 Widget direct write。
4. 成功路径必须在同一 Owner durable replacement 中完整移动 parent 及其唯一、空的 ChildContainer（不得存在 child contents）：P11 中的完整 closure 同时移除，P6 中出现同一 stable parent／child graph，P6 revision/digest 与 P11 receipt/digest 只在该同一提交中前进。不得 parent-only move、flatten、clone、new ItemId、new ContainerId、partial commit、child snapshot 或第二 P12 Widget inventory。
5. 任何 source、visibility、child closure、target、revision、session、Prepared／terminal gate、冲突或保存失败必须在 durable replacement 前拒绝，并保持 P11、P6、P7、P13、P14、P15、P19 与 P8 不变。失败不得清 P12 action state 之外的权威状态，亦不得创建或销毁 Code A actor。
6. P12 的 P6 → P11 既有 simple-item 路径不改变；本轮必须明确拒绝空间 parent、其 ChildContainer 和 child contents 从玩家进入尸体。不得以“对称”为由把完整空间图写入 P11。
7. 成功后的 P7 只从新的权威 P6 projection 读取该图。WindTalisman 只有之后通过既有 P7 操作置于其正式 SpatialRing 时才具有既有快捷进入条件；BackpackLevel1 继续只以真实 P6 item entry 进入。P20 不新增绑定、按键、使用、自动装备、空间效果或 child contents 的新入口。

### 4.3 生命周期、终局与既有系统保护

1. 进入 P6 的完整图只依 P17 已有 P5 ↔ P6 bridge、P6 recovery/rebind、P8 terminal settlement 与 P19 的既有 whole-graph ground route 处理。P20 不创建新的 P5 source、direct grant、特殊 save、Code A mirror 或 P14 world record。
2. P8 的 Code A 后置 terminal authority、Extracted 的完整 P6 player graph 返回 P5、Dead／RecoveredAbandon 的既有没收、及 P11 残余丢弃时序均不变。仍位于 BasicCorpse 的空间 parent／child graph 只能作为 P11 残余被完整丢弃；已完整进入 P6 的图只按既有 P8 结算。parent、child 或内容都不得单独泄漏回 P5。
3. P13 不自动为 parent 或 child 恢复／创建 1—9 绑定；P15 继续只允许位于 P6 BaseQuick 的 simple RestoreHealth 使用，且空间 parent／child 不获得任何 effect。P14 simple branch、P18 BasicCache.r2、P19 WorldDrop、P9/P10、P11/P12 的其余来源／交互语义均保持不变。
4. Code A 继续只在既有已提交死亡与尸体交互边缘转发，不得读取／缓存 P11 inventory、roll、child graph、P6 target、receipt 或终局结果。不得新增 Code A Save、旧 Loot 连接、地图 Actor 物品、掉落、拾取或玩家库存镜像。

## 5. 允许的改动范围

允许：

- 在 Code B 内最小扩展 P16 Loot Profile Catalog、deterministic roll／digest、P11 BasicCorpse 首次 graph 生成及 BasicCorpse.r1/r2 历史兼容；
- 在 Code B P1／P17／P12 的既有候选事务中最小补足完整合法 spatial graph 的 P11 → P6 BaseQuick 转移、验证、同一 Owner durable transaction 与 transient stale cleanup；
- 为 schema、序列化、调用签名或编译兼容最小调整 P7／P8／P13／P14／P15／P19 的 Code B 声明或无效引用清理，但不得改变其产品语义；
- 仅在确有编译依赖时对 P12 presenter 进行 root-only payload／projection 声明适配；不得改变 UI 结构、交互、读条、地图 actor 或 Code A 权威；
- 更新 PROJECT.md、PROJECT_INFO_CARD.md、本任务 Prompt 归档和本任务 Report。

## 6. 明确不在本任务内

不得实现、启动、重构或接管：

- 第二尸体、第二敌人、全量尸体／地图容器迁移、BasicCache 改写、随机 Encounter、资源／灵石、P5 starter／初始库存、直接 P6 grant、地图 Actor 新来源、自动拾取、直接 Actor pickup、Code A 旧 Loot、地面多物品、child 地面操作、嵌套袋、空间道具预装内容或任何新 WorldDrop；
- P12 的 UI、交互、读条、揭示、拖拽状态机、普通物品 Move/Merge/Swap、可见布局或 target identity；P12 的空间图回存尸体、直接装备、自动装备、child 操作或任何按钮／快捷领取；
- P13 新快捷栏语义、P14 新地面来源、P15 之外的使用效果、空间装备效果、武器／道袍／饰品效果、其他消耗品、技能、Buff／Debuff、动画、音效、战斗属性、网络同步或多人；
- Code A 的地图、Actor、输入、HUD、Player Actor、生命、战斗、死亡、Run、Run Save、终局分类、旧库存、旧 Loot／搜索或正式结算权威；
- 产品启动、CTA、wrapper、自动化、回归、截图、可见验收、Smoke、进程检查、Game target 编译、BuildCookRun、Cook 或 Package。

## 7. 静态代码审查与编译

完成实现后，只进行以下 P 阶段检查：

1. 审查 r2 仅影响未来未物质化 BasicCorpse；BasicCorpse.r1、BasicCache.r1/r2 与任何 materialized P9/P11 历史 record 均保持原样。审查 r2 主材料 group、optional gate、candidate DefinitionId、weight、quantity、固定排序、ProfileVersion、AlgorithmVersion 与 digest。
2. 审查 deterministic identity 与 receipt：同一精确 death-receipt identity 绝不会因时钟、Actor、UI、临时 GUID、旧 Code A Loot 或全局 RNG 重掷、追加空间 parent 或创建第二个 child graph。
3. 审查 P11 首次图：空间 parent 只位于 BasicCorpse root，最多一个；ChildContainer 为空、由 P17 Definition resolver 产生；容量、一层限制、无环、唯一 owner、非法配置和失败零写入均成立。
4. 审查 P12／P1 transaction：只有既有已揭示 source root 的真实 NativeOnDrop 到空 P6 BaseQuick 可以完整移动 parent 加 child graph；不存在 parent-only、flatten、clone、new ItemId、direct P6 write、P12 Widget inventory、partial commit、直接装备或反向复杂图回存尸体。
5. 审查 P5 → P6 → P8、P7 stale close、P13/P15 eligibility、P14/P19 world behavior、P9/P11 residual discard 与 Code A authority；确认本轮不会导致 child 逃逸、P5 starter、第二来源、地面 child 操作或 Code A inventory／Loot authority。
6. 审查 Code A diff。除纯声明／编译兼容外，预期没有 Code A 功能改动；若存在，必须逐文件说明其不涉及地图、Actor、Loot、搜索、输入、HUD、战斗、生命、Run、Run Save、结算或库存权威。
7. 编译一次 Editor 目标：

       "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_mapEditor Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex

8. 若编译失败，只修正 P20 引入的局部 Profile schema、deterministic roll、P11 graph materialization、P1/P12 complete-graph transaction、序列化、include、root-only projection declaration 或调用签名问题，然后重新执行同一 Editor 目标。若修复需要扩展到 P21、0.0.9B.F、Code A 权威或其他功能，停止受影响部分并报告。

## 8. Report 与完成信号

生成 Dev.D.UE.0.0.9B.P20.0.r0_report.md，保存至：

    C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\Docs\Report

Report 必须简洁、可审计地列出：

1. 本轮新增／修改／未修改的每个文件及职责；
2. BasicCorpse.r2 与 BasicCorpse.r1／BasicCache.r1/r2 的版本关系，r2 主材料 group、Optional.SpatialUtility gate、候选 DefinitionId、weight、quantity、排序、AlgorithmVersion 与 digest；
3. r2 的完整 deterministic identity 输入，以及为何相同未物质化 corpse／death receipt 不会重掷、追加空间 parent 或创建第二 child graph；
4. P11 首次 materialization 的完整 parent／ChildContainer graph、容量／一层限制／空 child、同一 Owner durable save、receipt／digest，以及 legacy materialized record 如何保持原样；
5. P12 的既有 Native Drop 如何只允许已揭示 parent root 完整进入空 P6 BaseQuick，成功／失败／冲突／重复路径如何避免 parent-only、flatten、clone、new ItemId、partial commit、直接装备或复杂图回存尸体；
6. P7、P5/P6/P8、P13/P14/P15/P17/P18/P19、P9/P10、P11/P12 与 Code A authority 的静态边界结论；
7. 实际 Editor 编译命令、目标、最终 native exit code 与关键结果；
8. 明确列出未执行的 F 阶段项目：真实 BasicCorpse r2 roll、optional 命中／未命中、死亡／开尸／揭示、P12 完整图转移、P7 打开／返回、P19 地面往返、P8 三种终局、recovery、自动化、回归、截图、Smoke、Game Build、Cook、Package 与最终验证仍由 0.0.9B.F 负责；
9. 明确列出尚未启动的功能：第二来源／全量迁移、空间道具预装内容、child 地面操作、嵌套袋、空间装备效果、武器／道袍／饰品效果、其他消耗品及后续 P 阶段。

仅当实现完成、静态边界审查通过、Editor 编译以 native exit code 0 完成且未越界时，Report 可使用：

    READY_FOR_NEXT_P_FUNCTIONAL_WITH_F_DEBT

若仅存在当前范围内可修复的编译问题，使用：

    NEEDS_P20_COMPILE_REWORK

若现有 BasicCorpse／P16／P17／P12 无法在不改写 materialized 历史、不扩展 Code A 权威、不创建第二来源或不改变 P12 尸体交互产品语义的前提下形成合法完整图，使用：

    NEEDS_PLANNER_DECISION
    BLOCKED

完成后不得自动开始 P21、0.0.9B.F 或其他任务。向策划 Chat 回传并附带且只附带本次同名 Report；正文首行使用：

    [CSEMI:REPORT_SENT] {"task_id":"Dev.D.UE.0.0.9B.P20.0.r0","file":"Dev.D.UE.0.0.9B.P20.0.r0_report.md"}

