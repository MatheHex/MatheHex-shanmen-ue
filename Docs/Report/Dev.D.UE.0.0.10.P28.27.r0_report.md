# Dev.D.UE.0.0.10.P28.27.r0 Report

## 1. 状态与范围

`P28_27_AUDIT_PASS`。2026-09-21 UTC；基线 P28.26 / `57fb7bdef9c4f40fcf84f6f40f10dc4097678cca`。本阶段完成旧兼容终局物品释放路径的正常产品可达性分类，不是源码修复、整体冻结或 F 验收。生产代码、测试代码、注册测试、配置与资产均零改动；只交接本 Report、Development Log 和有限冻结索引三份文档。

结论限于成功初始化、仍处于正常活动生命周期的 V3 Manager：没有显式旧自动化启动标志时，终局走既有 Profile 持久提交与 World 完成确认，不进入旧非 Profile 的结算后定时重载分支。旧分支仍存在，不能把“正常入口排除”说成“旧分支释放缺陷已修复”或“所有卸载均安全”。

## 2. 可达性证据

审阅 [StartupMode](../../Source/demo_map/demo_mapProfileStartupMode.cpp)、[默认输入](../../Source/demo_map/demo_mapProfileStartupMode.h)、[Manager](../../Source/demo_map/demo_mapV3ProgressionManager.cpp) 和 [既有启动测试](../../Source/demo_map/demo_mapProfileNormalStartupTests.cpp)。下表是源码调用条件与现有测试的交叉核对，不是本轮实际启动地图。

| 入口条件 | 模式与 Flow | 对旧终局分支的结论 |
|---|---|---|
| 正常 V3，无旧自动化标志 | ProductionProfile；Initialize 创建并保留 ProfilePreparationFlow，Profile 分支先返回 | 正常活动期间结算走 Profile 分支，不设置旧 SettlementReloadTimer |
| 显式 Profile 自动化成功初始化 | ProfileAutomation；同属 UsesProfilePreparation | 同属 Profile 终局；初始化拒绝立即返回 false，不回落到旧 BeginRun |
| Development 中显式旧自动化标志 | LegacyAutomation；选择优先于 Profile 自动化 | 旧 BeginRun 与非 Profile 结算/定时重载路径可达，本轮未运行或验收 |
| 纯选择器的 V2 / Editor 工具输入 | Disconnected | 选择器不做产品 I/O；不是已初始化 V3 Manager 的正常替代路径 |

Manager.Initialize 明确设置 bIsV3World；两个自动化输入仅在非 Shipping 编译分支从启动标志复制。**选择器本身并未编译掉 LegacyAutomation 输入**，因此不能宣称“任何 Shipping 调用都不能选 Legacy”；这里只核对 Manager 真实输入来源，Shipping 没有在本轮构建或运行。

Manager 的 ProfileStartupMode 除默认值外只有 Initialize 赋值。六处非终止 Flow.Reset 均位于非 Shipping 自动化初始化失败分支，并立即返回 false；生产初始化失败保留失败态 Flow，不落入旧 BeginRun。另一处 Reset 属于 EndPlay，不属于本结论的正常活动生命周期。

RequestSettlementAndReload 在 UsesProfilePreparation 且 Flow 存在时进入 Profile 分支；持久成功通过既有 CompleteDurableProfileSettlementWorld 确认或保留 World-only 续接，其他持久结果也从该分支返回。旧 DestroyRuntimeContainers 返回值未参与旧分支成功判定，随后定时 ReloadAfterSettlement 调用 OpenLevel 的行为仍未修改。本轮没有为被前置条件排除的正常入口新增恢复表、故障端口或第二套结算权威。

## 3. 实际验证与复用边界

| 证据 | 结果与归属 |
|---|---|
| 本轮 Editor / Game Development 构建检查 | 原生 0 / 0；均 Target is up to date，零编译动作，不冒充重新编译 |
| 本轮 demo_map.ProfileNormalStartup | 17 成功、0 失败、17 个独立用例；精确结束队列 17，原生 0，崩溃指标 0 |
| 本轮三路径文档分类 | PASS：Changed=3、Rules=0、Required=0、Logs=0；无生产改动必跑组，不等于产品测试通过 |
| 复用 P28.26 完整新旧根 | 新根 1430/0、旧根 1330/0，共 2760 个独立成功用例；原始证据保留，**本轮没有重跑双根** |

1617 项产品/验证输入与 P28.26 验证锁一致；103 个原未跟踪用户文件及两份 OverallReadiness 用户修改保持不变。既有专项主要证明选择规则和隔离 Flow 契约，不动态覆盖 Manager.Initialize 的全部调用链、旧地图重载、强制 EndPlay 或真实 UI。17 项属于旧根子集，不与复用的 2760 相加。没有复现正常入口产品故障，也没有源码修复，故无产品 Red；不凭空制造失败用例。

## 4. 交接与尚未关闭项

遵守 [P 阶段基线](../Process/P_STAGE_BASELINE_0_0_10.md)。[Development Log](../Log/Dev.D.UE.0.0.10.P28.27.r0_log.md)登记四份本轮原件和两份复用原件的本地路径与 SHA；原始日志没有上传 GitHub。发布基线由包含本文的 Git 提交追溯。

[有限冻结索引](../Architecture/Dev.D.UE.0.0.10_FoundationClosure_Index.md)将“旧兼容终局物品释放”从未经分类的正常产品待查项，收窄为显式旧自动化分支与独立 EndPlay 边界；FZ-1/2 其余权威路由、调用点与强制结束次序仍开放。没有声明全部旧 writer 不可达，也没有宣布收尾完成或暂停自动化。

后续只核对既有冻结清单的剩余入口，先证明可达条件，再决定最小修复。未接物理输入，未改正式地图/内容资产/玩法/UI；未启动 Editor UI、PIE、Standalone 或游戏程序，未做 Smoke/Cook/Package。
