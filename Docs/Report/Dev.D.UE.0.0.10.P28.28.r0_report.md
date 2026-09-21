# Dev.D.UE.0.0.10.P28.28.r0 Report

## 1. 状态与结论

`P28_28_AUDIT_PASS`。2026-09-21 UTC；基线 P28.27 / `71b71dd900287fa723e784688300e94094f8ed2a`。本阶段是普通 World 物品批量生成的调用条件审计；生产、测试、配置、资产均零改动，只交接 Report、Development Log 与有限冻结索引三份文档。不是产品修复、整体冻结或 F 验收。

结论：当前仓库内 `CreateWorldItemsAtomically` 的两个调用点每次均只提交一个请求。因此，“已生成前两项、第三项生成失败、回滚销毁前项被拒绝”的多项场景不能用来证明当前普通生成入口存在可达产品故障。没有为该假设增加恢复表、故障端口或第二套权威。

这不等于通用批量 API 安全：其回滚仍先移除绑定、忽略 Destroy 返回值，再恢复权威快照。成功生成后的最终不变量检查、重入、强制 EndPlay 等条件并未因单项调用而全部排除。未来新增多项调用或修改掉落表后，必须重新核对本结论，不能引用本阶段作为多项回滚验收。

## 2. 调用点与分区

核对 [ItemSubsystem](../../Source/demo_map/demo_mapItemSubsystem.cpp)、[声明](../../Source/demo_map/demo_mapItemSubsystem.h)、[物品定义](../../Source/demo_map/demo_mapItemDefinitions.cpp)、[Manager](../../Source/demo_map/demo_mapV3ProgressionManager.cpp) 与 [生命周期测试](../../Source/demo_map/demo_mapRunLifecycleTests.cpp)。仓库 Source 中只有两个批量方法调用点；该方法是普通 C++ 接口，未作为 UFUNCTION 暴露。此项静态搜索不证明任意外部扩展调用不存在。

| 既有入口 | 请求构成 | 本阶段边界 |
|---|---|---|
| Manager 标记物生成 → CreateWorldItem | 显式 `{ Request }`；数量放在一个堆叠中 | 每次一项，不包含已成功项之后的另一请求失败 |
| CreateEnemyLoot | 近战/远程/重型三张旧表各一行，分别为 SpiritDust×2、IronShard×2、AncientToken×1 | 每表生成一个请求，不按数量拆成多个 Actor；只陈述既有配置，不修改数值 |
| 空间包丢弃/回收 | 独立多投影路径，不调用上述批量方法 | P28.24–26 已有真实多项证据；不能拿单项审计替代这些契约 |
| 固定/M01 尸体掉落 | 容器生成与奖励准备路径 | 另属容器初始化/失败边界，仍在 FZ-2 待核对范围 |

`Fdemo_mapLootTables::Validate` 不只检查第一行数据，还明确要求三张表各自 `Num()==1`；本轮既有 `demo_map.V3.Lifecycle.C.FixedLootTables` 实际成功，验证了这项当前配置约束。批量方法先检查全部请求的定义、资格、单堆叠数量与安全位置，再开始生成。它仍接受任意长度数组，本轮没有将公共接口改成单项或宣称所有回滚分支不可达。

## 3. 验证与首次不完整证据

| 本轮检查 | 实际结果 |
|---|---|
| Editor / Game Development | 原生 0 / 0；均 Target is up to date、零编译动作 |
| demo_map.V3.WorldInteraction | 4 成功、0 失败，精确结束队列 4、原生 0；该组为值类型契约，不是物理 World 生成验收 |
| demo_map.V3.Lifecycle（独立复跑） | 15 成功、0 失败，精确结束队列 15、原生 0 |
| PreparedWorldPickupIdentity | 1 成功、0 失败，精确结束队列 1、原生 0；包含既有瞬态 World 用例，不是正式地图流程 |
| 精确三路径文档分类 | PASS：Changed=3、Rules=0、Required=0、Logs=0；不是产品通过指标 |

三个健康专项共 20 个独立用例，Fatal/Ensure/Unhandled 指标均 0。**第一次生命周期执行不计为通过**：原生退出 0，却只有 14 个成功结果、没有第 15 项结果和队列结束；外层证据门拒绝并退出 1。保留首次原件与 SHA，原因未确定。同输入单独复跑该组才取得完整 15/0；没有拼接两次结果，也没有重跑已健康的构建/World 组。

1617 项产品/验证输入与 P28.26 完整根基线一致；明确复用 P28.26 新根 1430/0、旧根 1330/0，共 2760 个独立用例，**本轮没有重跑完整根**。本轮 20 项是其子集，不相加。没有生产修复或正常入口故障复现，故没有产品 Red；首次不完整执行是证据拒绝，不冒充产品反例。

## 4. 交接与剩余范围

[Development Log](../Log/Dev.D.UE.0.0.10.P28.28.r0_log.md)登记七份本轮原件（含首次不完整日志）及两份复用原件的本地路径/SHA；原始验证日志没有上传 GitHub。发布基线由包含本文的 Git 提交追溯。103 个既有未跟踪文件和两份 OverallReadiness 用户修改保持不变。

[有限冻结索引](../Architecture/Dev.D.UE.0.0.10_FoundationClosure_Index.md)新增当前普通生成入口的单项分类，不把未证明的多项场景继续当成已复现缺陷。FZ-1/2 仍开放：容器生成失败、其余生命周期调用点及强制 EndPlay 仍需按真实条件核对。没有完成最终冻结，不暂停自动化，不新建玩法层功能。

遵守 [P 阶段基线](../Process/P_STAGE_BASELINE_0_0_10.md)：未接物理输入、未改地图/内容资产/玩法/UI，未启动 Editor UI、PIE、Standalone 或游戏程序，未做 Smoke/Cook/Package；没有新增内存、性能或完整产品流程达标声明。
