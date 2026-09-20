# 0.0.10 底层契约、权威与生命周期索引

状态：`FREEZE_AUDIT_IN_PROGRESS`，不是整体冻结或 F 阶段验收。初版 P28.0，2026-09-16；最近完整产品根验证为 [P28.20](../Report/Dev.D.UE.0.0.10.P28.20.r0_report.md)，于 2026-09-20 完成新根1430/0、旧根1330/0、双构建及最终覆盖核验（UTC）。P28.1路由修补、P28.2携入身份保护、P28.3/4/5分层清理拒绝保持、P28.6上层回滚续接、P28.7普通激活失败调用方、P28.8普通Start外层续接、P28.9持久终局后的World确认、P28.10延迟持久成功变体、P28.11终局上游容器清理次序、P28.12容器自身拒绝、P28.13活动Run散落物Teardown拒绝、P28.14逻辑终局后散落物拒绝时的原owner保持、P28.15技术激活回滚的同身份World续清理、P28.16直接Manager去激活中的灵石投影拒绝保持及P28.17直接GameMode去激活中的M01敌人/Boss投影拒绝保持，P28.18直接GameMode去激活中的动态撤离区域拒绝保持，以及P28.19同入口非M01目标和五类单对象槽拒绝保持、P28.20直接Manager独有遭遇对象拒绝保持证据见第4节。后续审计更新本文的有限清单，不因“继续”无限追加系统。

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
| 整备、Run 启动和终局 | [RunLifecycleAdapter](../../Source/demo_map/demo_mapShanmenRunLifecycleAdapter.cpp)；[GameMode](../../Source/demo_map/demo_mapGameMode.cpp)；[Manager](../../Source/demo_map/demo_mapV3ProgressionManager.cpp) | StartPreparedRun 持久身份贯穿 Runtime；终局仅精确规范化内容重放，不重发奖励；技术回滚下层拒绝保留原上下文，已接受Runtime前缀则只续未完成World清理，全部完成才向启动协调器确认 | P1.14、P27.31、P28.4/6；Items.ProductFlow/RunLifecycle、ControlledWeaponWorldLifecycle |
| Run 组合、结束和续清理 | GameMode 的 TryActivateCombatRun / ReleaseCombatProductRun / TryFinishCombatRunRetirement / DeactivateV3MissionContentForPreparation；Manager.DeactivateProfileWorld | 错误 owner 不释放；同进程已成功前缀不重做；Pending retirement 阻止重启和相关运行；GameMode 返回整备在释放拒绝时保留任务对象，并将false传回Manager阻止其局部World清理 | P27.29/30、P28.3/5；FormationRunLifecycle.GameModeScatterPublication / ControlledWeaponWorldLifecycle |
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
| FZ-2 Run 激活失败及最终释放 | P28.3/4/5完成分层拒绝保持，P28.6闭合已接受Runtime回滚后的World完成确认与原启动尝试续接，P28.7使普通激活三个失败调用点遵守同一完成端口，P28.8闭合普通Start外层续接且不接管框架尝试，P28.9闭合直接持久终局成功后的World确认/续清理，P28.10实测持久失败后重试成功的同一续清理变体，P28.11修复Profile终局提前清理Manager容器的上游次序，P28.12实测容器自身拒绝时保持原owner/pending，P28.13实测Manager直接去激活中散落物Teardown拒绝时保留身份/World绑定并续清理，P28.14闭合正常Manager终局后散落物逻辑损失与拒绝投影的原身份保持和World-only续清理，P28.15闭合技术激活回滚中同类投影拒绝的原Manager/attempt续接且保持durable ActiveRun，P28.16闭合直接Manager去激活中固定/M01灵石投影的拒绝保持与来源去重保护，P28.17闭合直接GameMode去激活中的M01敌人/Boss投影拒绝保持及成功前缀不重做，P28.18闭合动态撤离区域拒绝时的原owner保持、区域停用与成功前缀不重做，authored区域仍仅停用，P28.19闭合非M01目标数组及出口/近战/远程/重型/友军五槽拒绝时原引用和计数去重上下文保持，P28.20闭合Manager独有遭遇对象拒绝时原owner/导航上下文保持并阻止下游物品World提前释放；其余实际入口、容器/物品调用点及强制EndPlay仍待核对 | 将 TryActivateCombatRun 的早期回滚与统一 Release 的适用范围说明清楚；对可达失败证明 owner 保留或安全回滚，对前置约束已排除的分支记录推理；EndPlay 不可当已成功清理的证据 |
| FZ-3 最终冻结证据 | 等 FZ-1/2 关闭后执行 | 产品输入固定，完整新旧根、Editor/Game、改动映射和文档检查完成；保留失败/中断原件，发布最终 Report/Log/Git 基线后暂停监控 |

P28.0 已深读 FZ-2 的部分源码：激活入口先排斥残留 session/condition；Condition TryBegin 建立无活动 modifier 状态；剑法 Session/Presentation TryBegin 以有效 Run 构造候选。早期回滚仍有 TryEnd 失败后 Reset 的分支，但仅看到这些代码不足以证明正常入口能制造该失败。没有因此修改源码或新增故障注入接口。

P28.1 的隔离存盘故障证明：已切换的 Flow 曾随 Authority 进入 RecoveryRequired 而丢失新流程归属，技术回滚也未保持原 Runtime。现在归属与 Ready 分开，启动/消费/回滚仍要求 ready authority，恢复状态不回到旧路径。两个新增非零测试覆盖启动前与活动 Run；正常旧流程继续回归。Manager 库存/快捷路由及两处 Code B observer 通过同一谓词受约束，未另建路由或模拟物理输入。具体完成证据见 [P28.1 Report](../Report/Dev.D.UE.0.0.10.P28.1.r0_report.md) / [Log](../Log/Dev.D.UE.0.0.10.P28.1.r0_log.md)。这不是全部旧 writer 已不可达或整个 World 恢复已完成的证明。

P28.2 沿实际 Runtime 获物→消费→终局路径复现：三颗携入丹被一颗新获同类丹合并，导致原实例数量/来源改变，持久消费和原样撤离交接均被拒绝。修复复用 Runtime 已有 DeployedItemIds：自动入包/World 拾取的容量预检与合并、玩家拖拽和容器双向合并都不吸收或消灭携入身份；重新拾取携入物不将其标成新获物。没有新增持久 schema 或第二份权威，也不放宽既有数量/终局校验。阶段与验证见 [P28.2 Report](../Report/Dev.D.UE.0.0.10.P28.2.r0_report.md) / [Log](../Log/Dev.D.UE.0.0.10.P28.2.r0_log.md)。

剩余 FZ-1 需要继续区分既有规则与可达故障：持久测试明确要求撤离不能默默丢失 DeploymentLock 装备，因此本轮没有改成“缺少即销毁”；丢弃/存入容器与该终局规则的整体边界仍待闭合。新获但未整备消耗品的使用入口也需核对当前 ItemNotPrepared 限制，不能以恢复旧 Runtime 消费旁路解决。上述项不因本轮堆叠保护成功而视为全部完成。

P28.3 通过实际飞剑 World 销毁拒绝复现：下层 Release 已保留原 Run/owner，但返回整备调用方仍销毁敌人、清除活动标志。现在该 GameMode 入口对拒绝立即返回；相同 owner 可续清理，原已结束前缀不重做，M01 激活绑定失败明确返回 false。新增非零时间线/真实物品实例用例由 0/1 变为成功，完整专项 4/0、双根与双构建通过；见 [P28.3 Report](../Report/Dev.D.UE.0.0.10.P28.3.r0_report.md) / [Log](../Log/Dev.D.UE.0.0.10.P28.3.r0_log.md)。

P28.4 沿技术回滚生产端口复现：下层 Flow 因真实持久服务 RecoveryRequired 拒绝，Manager 却先清空自身上下文并 TeardownWorld 删除真实物品。现在只在既有 bAtSectReady 成功分支执行这两项清理；两次拒绝保持原 Run、三颗丹/生命1、两单位 World 物品及绑定，正常变体仍完成瞬态清理并保留持久恢复身份。专项5/0、完整新根1427/0、旧根1330/0和双构建通过；首次中断新根单独保留，不拼接计数。见 [P28.4 Report](../Report/Dev.D.UE.0.0.10.P28.4.r0_report.md) / [Log](../Log/Dev.D.UE.0.0.10.P28.4.r0_log.md)。

P28.5沿Manager真实World查找AuthGameMode的直接端口复现：下层战斗Run释放被拒绝，Manager仍清理自身投影和World物品。现在GameMode返回bool，Manager先检查再执行自身清理；两次拒绝保留原Run、非零时钟、两单位物品/绑定和任务对象，恢复后仅清理一次，持久Run不被结算或替换。专项两组各5/0、新根1428/0、旧根1330/0、双构建和八路径84组映射通过。新增测试曾误把Destroyed墓碑等同于删除记录，已按既有契约纠正，所有失败原件保留。见 [P28.5 Report](../Report/Dev.D.UE.0.0.10.P28.5.r0_report.md) / [Log](../Log/Dev.D.UE.0.0.10.P28.5.r0_log.md)。

P28.6实际反例证明：Runtime回滚已接受而World释放拒绝时，Manager曾错误返回成功并清上层尝试；再次回滚又被Flow的Preparation状态拒绝。现在Manager去激活返回bool，单个瞬态原绑定记录保存已接受前缀，不再重复Flow回滚；错误绑定和新启动/激活失败关闭。Coordinator的TechnicalStartFailure请求仅续原Adapter/GameMode/Manager清理，全部成功才到AtSect，该次调用仍不启动游戏。真实飞剑销毁拒绝两次、绑定错配拒绝、恢复后恰好一次释放且持久Run未变均通过；专项6/0与5/0、新根1429/0、旧根1330/0、双构建及13路径85组映射通过，首次RedProof失败原件保留。见 [P28.6 Report](../Report/Dev.D.UE.0.0.10.P28.6.r0_report.md) / [Log](../Log/Dev.D.UE.0.0.10.P28.6.r0_log.md)。

P28.7复现普通ActivatePreparedProfileWorld失败时的两种后果：健康回滚遗留pending；真实权威RecoveryRequired拒绝时，Manager却清空上下文和World物品。三个失败调用点现共用局部FailActivation，先标记pending，再调用P28.6既有两阶段端口，全部完成才显示整备，不独立重复去激活；Start诊断不再无条件声称已回滚。既有ManagerRollbackRetention扩成直接/普通激活×健康/故障四种变体，真实覆盖无AuthGameMode的首个失败分支；其余两个分支仅核对共用路径，不冒充生成故障或输入恢复验收。专项5/0与6/0、新根1429/0、旧根1330/0、双构建及5路径6必跑组映射通过，首次RedProof0/1原件保留。见 [P28.7 Report](../Report/Dev.D.UE.0.0.10.P28.7.r0_report.md) / [Log](../Log/Dev.D.UE.0.0.10.P28.7.r0_log.md)。

P28.8实际Red证明普通Start在原World清理故障解除后仍只拒绝、不续接。现在普通路由复用已有回滚端口，成功仅完成原清理，本次仍返回SessionNotReady，不同时启动新Run；框架归属仍必须由原Coordinator/Adapter确认。既有ManagerRollbackContinuation扩成普通/框架两变体，非零状态、错误绑定拒绝、恰好一次释放及持久Run不变均通过；专项5/0与6/0、新根1429/0、旧根1330/0、双构建及5路径7必跑组映射通过。首次RedProof0/1保留；普通变体以底层回滚建立真实pending，不冒充激活全过程。见 [P28.8 Report](../Report/Dev.D.UE.0.0.10.P28.8.r0_report.md) / [Log](../Log/Dev.D.UE.0.0.10.P28.8.r0_log.md)。

P28.9实际Red证明终局持久提交已成功而World拒绝释放时，Manager曾错误清pending；原持久Flow已回Preparation，既有重试不能续清理。现在单个同实例瞬态记录保留已接受回执和精确原绑定，重试只完成World、不重做持久提交；新准备/激活拒绝，错误Runtime绑定失败关闭，恢复后原武器/敌人各释放一次且持久快照不变。新增测试从真实RequestSettlementAndReload入口覆盖直接持久成功变体，Red0/1转为专项5/0与7/0、完整新根1430/0、旧根1330/0、双构建与7路径84组覆盖门通过。见 [P28.9 Report](../Report/Dev.D.UE.0.0.10.P28.9.r0_report.md) / [Log](../Log/Dev.D.UE.0.0.10.P28.9.r0_log.md)。

P28.10只补测试：既有ManagerSettlementContinuation保持直接变体，并加入WriteTemp使初次持久提交和一次重试失败、撤销存盘故障后第二次重试成功、World仍拒绝、原Run继续清理的组合。两次存盘失败保持活动持久快照；成功终局快照区别于非零基准；其后World重试不再提交持久结算，解除真实Destroy拒绝后飞剑/敌人各释放一次。新增变体在P28.9生产逻辑上首次即通过，没有生产修复或本阶段Red；注册数不变。专项7/0与5/0、新根1430/0、旧根1330/0、双构建、四路径3必跑组和529项映射自检均通过，见 [P28.10 Report](../Report/Dev.D.UE.0.0.10.P28.10.r0_report.md) / [Log](../Log/Dev.D.UE.0.0.10.P28.10.r0_log.md)。

P28.11沿RequestSettlementAndReload真实终局入口复现：战斗World尚拒绝释放，Manager却已在持久提交/去激活前销毁宝箱并清掉原容器归属。现在Profile容器清理交回既有DeactivateProfileWorld，获得战斗释放确认后才执行，非Profile兼容清理时机不变。既有直接/延迟持久成功两变体加入实际初始化的非空LootChest、原ContainerId/RunId及Manager归属，在持久失败和World-only重试时保持，恢复后恰好一次销毁；Red0/1（5条新断言失败、原生0）转为专项7/0与5/0、完整新根1430/0、旧根1330/0、双构建及五路径7必跑组覆盖门通过，529项映射自检通过。见 [P28.11 Report](../Report/Dev.D.UE.0.0.10.P28.11.r0_report.md) / [Log](../Log/Dev.D.UE.0.0.10.P28.11.r0_log.md)。没有新增公共接口、schema或恢复记录。

P28.12沿同一真实终局入口进一步复现：战斗已释放，但宝箱Destroy被引擎拒绝时，Manager仍清归属/奖励Run并向上返回World完成。现在私有容器清理返回bool，按快照遍历三个原owner数组，只保留拒绝项；未全部接受不清Run/会话，去激活返回false保留原终局续接，M01旧容器重绑定也失败关闭。既有直接/延迟持久成功两变体各实际触发两次LootChest销毁拒绝，验证原归属/pending/启动门保持、战斗前缀与持久提交不重做、恢复后一次释放；Red0/1（8条断言失败、原生0）转为专项7/0与5/0，完整新根1430/0、旧根1330/0、双构建、六路径7必跑组及529项映射自检均通过。见 [P28.12 Report](../Report/Dev.D.UE.0.0.10.P28.12.r0_report.md) / [Log](../Log/Dev.D.UE.0.0.10.P28.12.r0_log.md)。没有新增schema、公共接口、故障注入或恢复记录；共用算法不等于其他容器调用点均专项取证。

P28.13沿Manager直接去激活→Runtime.TeardownWorld实际复现：战斗已释放，但散落物Destroy被拒绝时Runtime先删绑定、退休物品身份并清ActiveWorld，Manager也错误确认完成。现在TeardownWorld返回bool，拒绝项保留原Actor、World归属/数量与ActiveWorld，Manager不越过false清自身owner；BeginWorld及相关创建/整备入口接收释放拒绝。既有ManagerDeactivationRetention使用实际两单位SpiritDust，两次拒绝后验证同一身份/绑定/原持久快照保持、成功战斗前缀不重做，错误World清理/重绑定/创建失败关闭，恢复后一次释放。Red0/1（5条断言失败、原生0）转为专项7/0与5/0，完整新根1430/0、旧根1330/0、双构建、七路径7必跑组及529项映射自检均通过。见 [P28.13 Report](../Report/Dev.D.UE.0.0.10.P28.13.r0_report.md) / [Log](../Log/Dev.D.UE.0.0.10.P28.13.r0_log.md)。注册测试数不增加，两个既有Runtime端口返回bool但未增schema、故障端口或恢复记录；非Smoke激活/Preparation调用方为静态路由检查，不冒充逐入口专项。

P28.14沿正常Manager.RequestSettlementAndReload复现不同路径：Runtime已将散落物逻辑结算为损失，却忽略Actor销毁拒绝并压缩Destroyed身份，使终局World续清理丢失owner。现在保留拒绝项绑定/原Destroyed身份，仅允许与原Settled Run及Summary损失行匹配的待释放投影；Manager既有终局续接先完成World再准备Runtime，持久提交不重做，直接Flow Start拒绝在残留投影时启动。既有直接/延迟持久成功两变体各保留原情况并追加实际两单位SpiritDust拒绝，四组合通过；Red0/1（12条断言失败）及首次修复6/1（Summary引用读取晚于其容器Reset）均保留，纠正读取次序后专项7/0、5/0及新根1430/0、旧根1330/0、双构建、七路径9必跑组和529项映射自检通过。见 [P28.14 Report](../Report/Dev.D.UE.0.0.10.P28.14.r0_report.md) / [Log](../Log/Dev.D.UE.0.0.10.P28.14.r0_log.md)。无新API/schema/恢复记录或注册测试；追加竞争终局/直接Start检查没有独立Red。

P28.15沿既有普通Manager回滚与框架保留attempt端口复现：Runtime已接受ActivationFailure逻辑回滚、散落物Destroy仍拒绝时，Flow错误进入RecoveryRequired导致Manager无法接住原World续清理。现在仅对同Run/Summary/原因/快照匹配的InvalidWorldBinding传递已接受Runtime前缀；Manager先完成原World释放，再Prepare原Runtime，才确认原上层attempt，持久ActiveRun快照保持不变。既有ManagerRollbackContinuation保留普通/框架两变体并各追加两单位SpiritDust真实销毁拒绝，验证两次飞剑拒绝、绑定/启动门、两次散落物单独拒绝、成功前缀不重做及故障解除后一次释放。Red0/1（39条最终Expected失败）保留；首次修复后专项7/0、5/0及新根1430/0、旧根1330/0、双构建、六路径9必跑组、529项映射自检通过。见 [P28.15 Report](../Report/Dev.D.UE.0.0.10.P28.15.r0_report.md) / [Log](../Log/Dev.D.UE.0.0.10.P28.15.r0_log.md)。无新API/schema/恢复记录或注册测试，不将原保留attempt夹具冒充完整M01 BeginActivation。

P28.16沿直接Manager.DeactivateProfileWorld复现固定/M01灵石投影Destroy拒绝被忽略、原owner及来源去重标记被清空的问题。现在逐项保留拒绝的原弱引用，成功项不重复释放，任一拒绝都返回false并保留来源标记与未完成World上下文。既有ManagerDeactivationRetention加入已初始化20/30/40灵石投影，固定和一数组项实际拒绝两次、另一项成功；恢复后各原Actor恰好释放一次，物品持久快照与原Run不变。Red0/1（5条最终Expected失败）转为专项7/0、5/0及新根1430/0、旧根1330/0、双构建、五路径7必跑组和529项映射自检通过。见 [P28.16 Report](../Report/Dev.D.UE.0.0.10.P28.16.r0_report.md) / [Log](../Log/Dev.D.UE.0.0.10.P28.16.r0_log.md)。无新API/schema/恢复记录/注册测试；直接去激活证据不替代货币拾取/持久结算、所有激活入口或完整M01流程。

P28.17沿GameMode.DeactivateV3MissionContentForPreparation复现：战斗前缀已释放，敌人/Boss的Destroy被引擎拒绝时，私有清理却提前清空owner并向上确认。现在私有清理返回bool，仅保留拒绝项，Boss弱引用在实际释放前不清，去激活先等待成功再清撤离上下文；初始化旧批次释放拒绝时也先返回。既有PreparationDeactivationRetention追加原敌人/Boss各两次真实拒绝、另一个敌人成功，验证非零生命/原Run/三条遭遇记录保持、逻辑终止拒绝新Boss死亡提交、durable快照不变、故障解除后每个Actor恰好一次释放。Red0/1（5条最终Expected失败）转为专项7/0、5/0及新根1430/0、旧根1330/0、双构建、六路径81必跑组和529项映射自检通过。见 [P28.17 Report](../Report/Dev.D.UE.0.0.10.P28.17.r0_report.md) / [Log](../Log/Dev.D.UE.0.0.10.P28.17.r0_log.md)。无新公共API/schema/恢复记录/注册测试；夹具是瞬态World与GameMode对象，不替代完整M01初始化，初始化检查仅为静态次序证据。

P28.18沿同一GameMode去激活入口复现：动态撤离区域Destroy拒绝后原数组曾被无条件清空。现在私有清理返回bool、先停用所有有效区域、仅保留拒绝的动态owner；去激活等待真实完成，初始化旧批次未清完先返回，authored区域仍由World持有、只停用不销毁。既有PreparationDeactivationRetention追加动态拒绝两次/动态成功/authored停用三个对象，保持已解锁的非默认撤离权威、原Run与持久快照；恢复后动态对象各释放一次，authored对象存续。Red0/1（5条最终Expected失败）转为专项7/0、5/0、新根1430/0、旧根1330/0、双构建、六路径81必跑组及529项映射自检通过。见 [P28.18 Report](../Report/Dev.D.UE.0.0.10.P28.18.r0_report.md) / [Log](../Log/Dev.D.UE.0.0.10.P28.18.r0_log.md)。无新公共API/schema/恢复记录/注册测试；初始化仅静态次序检查，非M01活动内容再进入仍需核对真实入口条件，不宣称所有再激活入口已关闭。

P28.19沿直接GameMode去激活尾部复现：非M01目标与五个单对象槽Destroy拒绝后，原引用、计数去重记录和任务活动标志曾被无条件清空。现在局部清理保留原类型弱引用、按数组快照保留拒绝目标，五槽全部独立尝试；任一拒绝返回false，全部完成才清任务上下文。既有PreparationDeactivationRetention加入三个目标和出口/近战/远程/重型/友军五对象；六个真实拒绝两次、两个成功，恢复后八个对象各释放一次且原Run/durable快照不变。Red0/1（5条最终Expected失败）转为专项7/0、5/0、新根1430/0、旧根1330/0、双构建、五路径81必跑组及529项映射自检通过。生产19新增/22删除、测试74新增，无新API/schema/恢复记录/注册测试。见 [P28.19 Report](../Report/Dev.D.UE.0.0.10.P28.19.r0_report.md) / [Log](../Log/Dev.D.UE.0.0.10.P28.19.r0_log.md)。夹具仍为瞬态World与GameMode对象，不替代正式地图创建、目标死亡回调、Manager完整终局组合或非M01完整再激活证据。

P28.20沿直接Manager.DeactivateProfileWorld复现：Manager拥有的遭遇对象多于GameMode绑定的三个Primary槽，未绑定对象Destroy拒绝时，共用清理曾先清原数组/导航标记并错误允许下游物品World释放。现在私有清理返回bool，快照遍历仅保留拒绝项，全部释放才清导航上下文；DestroyRuntimeContainers等待该确认再清上下文。既有ManagerDeactivationRetention追加两个Manager独有对象和两单位SpiritDust，真实拒绝两次、成功前缀不重做、恢复后各原Actor恰好一次释放且原Run/durable快照不变。Red0/1（5条最终Expected失败）转为专项7/0、5/0、新根1430/0、旧根1330/0、双构建、六路径7必跑组及529项映射自检通过。首次新根执行中断1181条无正常退出，原件保留；同输入Recovery1完整通过，不拼接计数或推定中断原因。见 [P28.20 Report](../Report/Dev.D.UE.0.0.10.P28.20.r0_report.md) / [Log](../Log/Dev.D.UE.0.0.10.P28.20.r0_log.md)。生产12新增/7删除、测试57新增，无新公共API/schema/恢复记录/注册测试；只证明直接瞬态World端口，不替代14对象实际创建、正式地图/NavMesh、完整激活或Manager终局组合。

剩余FZ-2限定为其余调用点与可达性：其他容器入口、旧兼容终局物品释放、空间包部分释放、其他灵石/敌人释放调用点、撤离其他调用点与局部生成失败及强制EndPlay次序尚未全部取证。P28.13证明持久Run仍活动时直接Manager去激活的Teardown拒绝保持；P28.14证明Shanmen正常Manager终局的直接/延迟持久成功两类World-only续接；P28.15证明既有技术激活回滚/保留attempt路径的散落物拒绝续接。P28.16另核对外部RunLifecycleAdapter.StartPreparedRun当前非测试调用方为ProfilePreparationFlow，Flow已有Settled且残留WorldActorCount的先行拒绝，因此未凭Adapter内部次序单独认定可达新故障或扩充接口；其他激活失败入口仍需取证。P28.11/12证明上游清理次序和终局LootChest拒绝保持，不证明容器内容跨终局原子保持。P28.9/10完成端口与两种持久成功路径不等于全部World路径已实测，IsDurablySettled也不等于World释放已完成。P28.6的既有启动尝试友元绑定及P28.7—20瞬态World测试均不替代正式M01 BeginActivation/玩家流程，不证明所有清理均原子化。后续先核对现有调用条件，再决定最小修复；不得据本段扩充通用恢复系统。

## 5. 与冻结分开的债务

- **容量：** ProcessedRequests 增长、全量快照/排序、锁内持久化需要梯度测量；历史压缩须保留终局/未决重放证据，不直接删 ID。此项尚未解决，不称为内存性能达标。
- **恢复边界：** 任意跨内容版本迁移、历史裁剪后重放、整个 World 的跨进程原子恢复未由当前局部契约承诺；不新增通用分布式事务框架。
- **F 阶段：** 物理输入、正式地图、UI 布局与焦点、动画/音效/特效真实播放、目标硬件 RAM/VRAM/帧耗时、长时运行、Cook/Package 均未在本轮验收。
- **未冻结的内容方向：** 更多流派、完整经验曲线、技能/敌人/物品目录、宗门生产/交易、环境术法与数值平衡，不能因有 Adapter 或枚举视为完成。
- **维护：** 历史巨型产品类与长恢复链优先使用可检索入口，只有真实结构缺口才改动，不为降低文件行数进行无需求重构。

## 6. 证据与文档入口

- 最近完整产品验证：[P28.20 Report](../Report/Dev.D.UE.0.0.10.P28.20.r0_report.md)、[Log](../Log/Dev.D.UE.0.0.10.P28.20.r0_log.md)：恢复新根1430/0、原健康旧根1330/0、双构建原生0/0，精确6路径映射覆盖7个必跑组；首次新根中断单独保留。阶段闭合直接Manager独有遭遇对象拒绝保持，不是FZ-1/2关闭后的最终冻结验证。

- 前阶段完整产品验证：[P28.19 Report](../Report/Dev.D.UE.0.0.10.P28.19.r0_report.md)、[Log](../Log/Dev.D.UE.0.0.10.P28.19.r0_log.md)：新根1430/0、旧根1330/0、双构建原生0/0，精确5路径映射覆盖81个必跑组；阶段闭合直接GameMode非M01任务对象拒绝保持，不是FZ-1/2关闭后的最终冻结验证。

- 更早完整产品验证：[P28.18 Report](../Report/Dev.D.UE.0.0.10.P28.18.r0_report.md)、[Log](../Log/Dev.D.UE.0.0.10.P28.18.r0_log.md)：新根1430/0、旧根1330/0、双构建原生0/0，精确6路径映射覆盖81个必跑组；阶段闭合直接GameMode动态撤离区域拒绝保持并区分authored所有权，不是FZ-1/2关闭后的最终冻结验证。

- 更早完整产品验证：[P28.17 Report](../Report/Dev.D.UE.0.0.10.P28.17.r0_report.md)、[Log](../Log/Dev.D.UE.0.0.10.P28.17.r0_log.md)：新根1430/0、旧根1330/0、双构建原生0/0，精确6路径映射覆盖81个必跑组；阶段闭合直接GameMode敌人/Boss投影拒绝保持，不是FZ-1/2关闭后的最终冻结验证。

- 更早完整产品验证：[P28.16 Report](../Report/Dev.D.UE.0.0.10.P28.16.r0_report.md)、[Log](../Log/Dev.D.UE.0.0.10.P28.16.r0_log.md)：新根1430/0、旧根1330/0、双构建原生0/0，精确5路径映射覆盖7个必跑组；阶段闭合直接Manager灵石投影拒绝保持，不是FZ-1/2关闭后的最终冻结验证。

- 更早完整产品验证：[P28.15 Report](../Report/Dev.D.UE.0.0.10.P28.15.r0_report.md)、[Log](../Log/Dev.D.UE.0.0.10.P28.15.r0_log.md)：新根1430/0、旧根1330/0、双构建原生0/0，精确6路径映射覆盖9个必跑组；不是FZ-1/2关闭后的最终冻结验证。此前 [P28.14](../Report/Dev.D.UE.0.0.10.P28.14.r0_report.md) 的正常终局散落物拒绝续接、[P28.13](../Report/Dev.D.UE.0.0.10.P28.13.r0_report.md) 的活动Run散落物Teardown拒绝保持、[P28.12](../Report/Dev.D.UE.0.0.10.P28.12.r0_report.md) 的终局容器自身拒绝保持、[P28.11](../Report/Dev.D.UE.0.0.10.P28.11.r0_report.md) 的终局上游容器清理次序、[P28.10](../Report/Dev.D.UE.0.0.10.P28.10.r0_report.md) 的延迟持久成功变体、[P28.9](../Report/Dev.D.UE.0.0.10.P28.9.r0_report.md) 的直接持久终局World确认、[P28.8](../Report/Dev.D.UE.0.0.10.P28.8.r0_report.md) 的普通Start续接、[P28.7](../Report/Dev.D.UE.0.0.10.P28.7.r0_report.md) 的普通激活失败调用方、[P28.6](../Report/Dev.D.UE.0.0.10.P28.6.r0_report.md) 的原启动尝试续接、[P28.5](../Report/Dev.D.UE.0.0.10.P28.5.r0_report.md) 的Manager直接World保持、[P28.4](../Report/Dev.D.UE.0.0.10.P28.4.r0_report.md) 的Manager下层回滚拒绝保持、[P28.3](../Report/Dev.D.UE.0.0.10.P28.3.r0_report.md) 的GameMode拒绝保持和 [P27.31](../Report/Dev.D.UE.0.0.10.P27.31.r0_report.md) 的结算重放证据保留。
- 最新携入身份保护：[P28.2 Report](../Report/Dev.D.UE.0.0.10.P28.2.r0_report.md)、[Log](../Log/Dev.D.UE.0.0.10.P28.2.r0_log.md)：Items 84/0、旧根 1330/0、双构建原生 0/0；专项 10 项包含在 Items 内，不当作最终 Shanmen 全根验证。
- 本次索引与文档分类：[P28.0 Report](../Report/Dev.D.UE.0.0.10.P28.0.r0_report.md)、[Log](../Log/Dev.D.UE.0.0.10.P28.0.r0_log.md)。其验证不冒充新一轮完整产品根。
- [项目入口](../../PROJECT.md)、[信息卡](../../PROJECT_INFO_CARD.md) 顶部为当前说明，下部是明确标注的 0.0.9B 历史；[旧 I 门禁](../Process/I_STAGE_FOUNDATION_GATE.md) 仅作历史。
- 本索引状态仍为 FREEZE_AUDIT_IN_PROGRESS；没有宣布所有已批准契约全部闭合，也没有暂停监控或进入 F。
