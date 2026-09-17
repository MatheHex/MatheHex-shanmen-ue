# 0.0.10 底层契约、权威与生命周期索引

状态：`FREEZE_AUDIT_IN_PROGRESS`，不是整体冻结或 F 阶段验收。初版 P28.0，2026-09-16；最近完整产品根验证为 [P28.3](../Report/Dev.D.UE.0.0.10.P28.3.r0_report.md)，于 2026-09-17 完成新根 1426/0、旧根 1330/0 及双构建核验。P28.1 路由修补、P28.2 携入身份保护、P28.3 GameMode 清理保持见第 4 节。后续审计更新本文的有限清单，不因“继续”无限追加系统。

## 1. 范围来源与非目标

用户人工完善的《山门》UE 0.0.10 战斗系统规划讨论报告是方向基础：五大战斗体系、真实物品消费、主动防御、状态与针对性治疗、成长改变操作权限、物品/Run/撤离链。报告中的太极、护心镜、各级操作等例子不代表完整内容目录已冻结；数值、手感、正式资产、经济目录、其他流派和经验曲线不在本轮补做。

用户最新要求将当前开发限定为底层闭合，具体边界见 [P 阶段基线](../Process/P_STAGE_BASELINE_0_0_10.md)。完整系统成熟度与 UI/内存准备见 [总体报告](../Report/Dev.D.UE.0.0.10.OverallReadiness.r0_report.md)。本索引只追溯已有契约，不把需求原稿或 Report 中的建议自动变成新增开发授权。

## 2. 契约—权威—产品入口—证据

表中“证据”指已提交阶段及其自动化，非本轮重新执行。可复用范围限于相同产品源码。D2/局部表现入口不证明真实输入或资产播放。

| 契约 | 权威/所有者与代码入口 | 生命周期/不变量 | 已有阶段证据 |
|---|---|---|---|
| 确定性身份与不可变动作 | [Core](../../Source/ShanmenCore)；[CombatTypes](../../Source/ShanmenCombatCore/Public/ShanmenCombatTypes.h) | Run/Activation/Candidate 身份一致，命名空间隔离；修改装备不能改写已捕获动作 | P0.1；CombatCore 根组 |
| 命中、伤害、防御、致死拦截 | [Resolver](../../Source/ShanmenCombatCore/Private/ShanmenCombatResolver.cpp)；产品宿主接收结果 | 几何只给候选；有序带标签防御层、生命值上下文、减免守恒；GAS 不重算第二份结算 | P0.1、P4、P17；CombatCore/CombatRunCoordinator |
| 实例、容器、资源事务 | [Repository](../../Source/ShanmenItems/Private/ShanmenItemRepository.cpp)；[AuthorityService](../../Source/ShanmenItems/Private/ShanmenItemAuthorityService.cpp) | Reserve 不消费，Commit 才消费；精确身份重放；拒绝不覆盖原成功 | P1；Items 根组；[事务 ADR](Dev.D.UE.0.0.10_ItemTransactions_ADR.md) |
| 唯一持久物品权威 | [GameInstance Subsystem](../../Source/demo_map/demo_mapShanmenItemAuthoritySubsystem.h) 持有服务；[Persistence](../../Source/ShanmenItems/Private/ShanmenItemPersistence.cpp) | 存储绑定与 Profile owner 一致；临时写/校验/备份/替换；不确定结果恢复前不接受新命令 | P1.2 以后；[权威 ADR](Dev.D.UE.0.0.10_ItemAuthority_ADR.md) |
| 旧档迁移与 cutover | [ProfileRepository](../../Source/demo_map/demo_mapProfileRepository.cpp)；[RunLifecycleAdapter](../../Source/demo_map/demo_mapShanmenRunLifecycleAdapter.cpp) | 旧来源一致且无活动 Run 才迁移；1≤legacy<current；旧 writer 不与新产品权威并行 | [迁移 ADR](Dev.D.UE.0.0.10_ItemMigration_ADR.md)、P1.14；ItemEconomySchema.22/.23 |
| 整备、Run 启动和终局 | [RunLifecycleAdapter](../../Source/demo_map/demo_mapShanmenRunLifecycleAdapter.cpp)；[GameMode](../../Source/demo_map/demo_mapGameMode.cpp) | StartPreparedRun 持久身份贯穿 Runtime；终局仅精确规范化内容重放，不重发奖励 | P1.14、P27.31；Items.ProductFlow/RunLifecycle |
| Run 组合、结束和续清理 | GameMode 的 TryActivateCombatRun / ReleaseCombatProductRun / TryFinishCombatRunRetirement / DeactivateV3MissionContentForPreparation | 错误 owner 不释放；同进程已成功前缀不重做；Pending retirement 阻止重启和相关运行；GameMode 返回整备在释放拒绝时保留任务对象 | P27.29/30、P28.3；FormationRunLifecycle.GameModeScatterPublication / ControlledWeaponWorldLifecycle |
| 剑法/剑气/动作互斥 | [CombatRuntime](../../Source/ShanmenCombatRuntime)；[SwordRhythm Session](../../Source/demo_map/demo_mapShanmenSwordRhythmProductSession.cpp) 与 GameMode | 动作生命周期和权限先于执行；Run 固定时间线不是渲染 FPS；过期可用性不能直接消费 | P3、P10/11/12、P18/24；对应阶段完整新根 |
| 御器真实物品与 World | [ControlledWeapon RunLifecycle](../../Source/demo_map/demo_mapShanmenControlledWeaponRunLifecycle.cpp)；GameMode 持有各 host/world owner | 部署关联真实 ItemInstance；逻辑结束与 World 释放区分；销毁失败保留待清理 owner | P6、P21、P27.30 |
| 暗器、阵法资源与操作权限 | [ThrownWeapon Lifecycle](../../Source/demo_map/demo_mapShanmenThrownWeaponProductLifecycle.cpp)；[Formation Lifecycle](../../Source/demo_map/demo_mapShanmenFormationRunLifecycle.cpp) | 预览不消费；执行到提交点才消费；散布由 GameMode 唯一发布；熟练度权限不是经验系统 | P7/8、P20/22/27；P27.28 组合入口 |
| 状态、治疗、灵力/护盾、神识 | [Condition Component](../../Source/demo_map/demo_mapShanmenCombatConditionComponent.cpp)；[Treatment Lifecycle](../../Source/demo_map/demo_mapShanmenMeridianShockTreatmentProductLifecycle.cpp)；[DivineSense Controller](../../Source/demo_map/demo_mapShanmenDivineSenseProductController.cpp) | 状态真值、资源权威与表现分开；治疗副作用与物品提交有证据边界；按 Run 失效 | P9、P16/17、P19/23/25 |
| Owner 交接、展示确认与恢复 | [SwordRhythm Presentation Controller](../../Source/demo_map/demo_mapShanmenSwordRhythmEffectCuePresentationRunController.cpp)；各有界 delivery/recovery owner | 接收确认不等于真实播放；已提交命令不随页面关闭撤销；旧 Run 不能复用新 owner | P12/13/14、P20；具体限制见总体报告 |
| 改动驱动回归 | [映射](../../Scripts/ShanmenRegressionMap.json)、[检查器](../../Scripts/Test-ShanmenRegressionCoverage.ps1) | 每个生产路径对应的必跑组必须有健康实际日志；未知路径拒绝 | P27.31 双根与覆盖门；P28.0 精确文档路径分类 |

上表不是逐个历史调用点已审完的证明。Code A/B 仍有兼容实现；“新物品权威唯一”与“全部旧入口均已隔离”须分别取证，不通过删除旧代码假装完成迁移。

## 3. 按生命周期恢复，不按文件名猜阶段

1. **绑定 Profile：** GameInstance-owned authority 先尝试打开既有新文档，符合条件才读稳定旧来源；失败保持明确状态。
2. **准备/启动：** 由持久预留和启动回执重建同一 correlation；Runtime/World 绑定不能替代持久成功。
3. **运行：** Coordinator 维护实体与战斗交付；领域 host 管理动作/状态，物品变更回到 durable 命令。UI 只读投影或发命令。
4. **终局：** FinalizeSettlement 规范化相同权威输入；精确匹配返回原回执，冲突不追加副作用。清理次序由 GameMode 及其 owner 组合负责。
5. **失败续接：** P27.30 的 Pending retirement 是同一 GameMode 的内存状态；P27.31 的终局重放使用持久证据。两者不能互换，也不是整个 World 跨进程原子恢复。
6. **重新绑定：** 旧 Run 引用、未释放 owner 或不一致身份存在时不得开始新 Run；最终成功后才释放对应上下文。

## 4. 有限冻结清单

仅保留以下三项最终核对；发现新问题必须给出明确现有契约、入口可达条件与证据，不用纯猜测扩大清单。

| 编号 | 状态 | 关闭条件 |
|---|---|---|
| FZ-1 全入口权威路由 | P28.1 修复 Ready/归属混用；P28.2 修复携入堆叠被新获物合并的身份缺口，阶段验证见对应 Report；全部入口审计仍待完成 | 覆盖产品整备/拾取/快捷使用/库存使用/终局，标注 Shanmen cutover 及旧兼容分支；说明 Code A Runtime 可变投影如何受 durable 结果约束。不能以局部修复代替全部调用图证明 |
| FZ-2 Run 激活失败及最终释放 | P28.3 已复现并修复 GameMode 忽略释放拒绝后清理任务对象；Manager 外层及激活失败传播仍待核对 | 将 TryActivateCombatRun 的早期回滚与统一 Release 的适用范围说明清楚；对可达失败证明 owner 保留或安全回滚，对前置约束已排除的分支记录推理；EndPlay 不可当已成功清理的证据 |
| FZ-3 最终冻结证据 | 等 FZ-1/2 关闭后执行 | 产品输入固定，完整新旧根、Editor/Game、改动映射和文档检查完成；保留失败/中断原件，发布最终 Report/Log/Git 基线后暂停监控 |

P28.0 已深读 FZ-2 的部分源码：激活入口先排斥残留 session/condition；Condition TryBegin 建立无活动 modifier 状态；剑法 Session/Presentation TryBegin 以有效 Run 构造候选。早期回滚仍有 TryEnd 失败后 Reset 的分支，但仅看到这些代码不足以证明正常入口能制造该失败。没有因此修改源码或新增故障注入接口。

P28.1 的隔离存盘故障证明：已切换的 Flow 曾随 Authority 进入 RecoveryRequired 而丢失新流程归属，技术回滚也未保持原 Runtime。现在归属与 Ready 分开，启动/消费/回滚仍要求 ready authority，恢复状态不回到旧路径。两个新增非零测试覆盖启动前与活动 Run；正常旧流程继续回归。Manager 库存/快捷路由及两处 Code B observer 通过同一谓词受约束，未另建路由或模拟物理输入。具体完成证据见 [P28.1 Report](../Report/Dev.D.UE.0.0.10.P28.1.r0_report.md) / [Log](../Log/Dev.D.UE.0.0.10.P28.1.r0_log.md)。这不是全部旧 writer 已不可达或整个 World 恢复已完成的证明。

P28.2 沿实际 Runtime 获物→消费→终局路径复现：三颗携入丹被一颗新获同类丹合并，导致原实例数量/来源改变，持久消费和原样撤离交接均被拒绝。修复复用 Runtime 已有 DeployedItemIds：自动入包/World 拾取的容量预检与合并、玩家拖拽和容器双向合并都不吸收或消灭携入身份；重新拾取携入物不将其标成新获物。没有新增持久 schema 或第二份权威，也不放宽既有数量/终局校验。阶段与验证见 [P28.2 Report](../Report/Dev.D.UE.0.0.10.P28.2.r0_report.md) / [Log](../Log/Dev.D.UE.0.0.10.P28.2.r0_log.md)。

剩余 FZ-1 需要继续区分既有规则与可达故障：持久测试明确要求撤离不能默默丢失 DeploymentLock 装备，因此本轮没有改成“缺少即销毁”；丢弃/存入容器与该终局规则的整体边界仍待闭合。新获但未整备消耗品的使用入口也需核对当前 ItemNotPrepared 限制，不能以恢复旧 Runtime 消费旁路解决。上述项不因本轮堆叠保护成功而视为全部完成。

P28.3 通过实际飞剑 World 销毁拒绝复现：下层 Release 已保留原 Run/owner，但返回整备调用方仍销毁敌人、清除活动标志。现在该 GameMode 入口对拒绝立即返回；相同 owner 可续清理，原已结束前缀不重做，M01 激活绑定失败明确返回 false。新增非零时间线/真实物品实例用例由 0/1 变为成功，完整专项 4/0、双根与双构建通过；见 [P28.3 Report](../Report/Dev.D.UE.0.0.10.P28.3.r0_report.md) / [Log](../Log/Dev.D.UE.0.0.10.P28.3.r0_log.md)。

剩余 FZ-2 明确限定为外层与可达性：Manager.DeactivateProfileWorld 仍先清理其拾取物/容器，再调用 GameMode，最后 TeardownWorld；技术回滚、激活失败、终局及强制 EndPlay 的次序与结果传播尚未全部取证。P28.3 只证明 GameMode 所有对象的保持，不证明整个 World 原子恢复，也不以测试夹具替代正式 M01 激活链。后续先核对现有调用条件，再决定最小修复；不得据本段直接扩充通用恢复系统。

## 5. 与冻结分开的债务

- **容量：** ProcessedRequests 增长、全量快照/排序、锁内持久化需要梯度测量；历史压缩须保留终局/未决重放证据，不直接删 ID。此项尚未解决，不称为内存性能达标。
- **恢复边界：** 任意跨内容版本迁移、历史裁剪后重放、整个 World 的跨进程原子恢复未由当前局部契约承诺；不新增通用分布式事务框架。
- **F 阶段：** 物理输入、正式地图、UI 布局与焦点、动画/音效/特效真实播放、目标硬件 RAM/VRAM/帧耗时、长时运行、Cook/Package 均未在本轮验收。
- **未冻结的内容方向：** 更多流派、完整经验曲线、技能/敌人/物品目录、宗门生产/交易、环境术法与数值平衡，不能因有 Adapter 或枚举视为完成。
- **维护：** 历史巨型产品类与长恢复链优先使用可检索入口，只有真实结构缺口才改动，不为降低文件行数进行无需求重构。

## 6. 证据与文档入口

- 最近完整产品验证：[P28.3 Report](../Report/Dev.D.UE.0.0.10.P28.3.r0_report.md)、[Log](../Log/Dev.D.UE.0.0.10.P28.3.r0_log.md)：新根 1426/0、旧根 1330/0、双构建原生 0/0，精确六路径映射覆盖 81 个必跑组；不是 FZ-1/2 关闭后的最终冻结验证。此前 [P27.31](../Report/Dev.D.UE.0.0.10.P27.31.r0_report.md) 的结算重放证据保留。
- 最新携入身份保护：[P28.2 Report](../Report/Dev.D.UE.0.0.10.P28.2.r0_report.md)、[Log](../Log/Dev.D.UE.0.0.10.P28.2.r0_log.md)：Items 84/0、旧根 1330/0、双构建原生 0/0；专项 10 项包含在 Items 内，不当作最终 Shanmen 全根验证。
- 本次索引与文档分类：[P28.0 Report](../Report/Dev.D.UE.0.0.10.P28.0.r0_report.md)、[Log](../Log/Dev.D.UE.0.0.10.P28.0.r0_log.md)。其验证不冒充新一轮完整产品根。
- [项目入口](../../PROJECT.md)、[信息卡](../../PROJECT_INFO_CARD.md) 顶部为当前说明，下部是明确标注的 0.0.9B 历史；[旧 I 门禁](../Process/I_STAGE_FOUNDATION_GATE.md) 仅作历史。
- 本索引状态仍为 FREEZE_AUDIT_IN_PROGRESS；没有宣布所有已批准契约全部闭合，也没有暂停监控或进入 F。
