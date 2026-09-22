# Dev.D.UE.0.0.10.P28.35.r0 Report

## 1. 结论与范围

`P28_35_CANONICAL_SOURCE_CANDIDATE_READY`：补齐产品已登记来源到 ShanmenItems 完整候选计划的转换边界。M01 与 P8 的全部 298 个来源槽位可以用非零顺序/保底上下文构建有效候选，并经既有无损 codec 往返保持。来源规划仍只有原来的生成器，未增加奖励表、第二库存、持久 schema 或新权威。

这是 [P28.30 有限实施顺序](Dev.D.UE.0.0.10.P28.30.r0_report.md) 中产品接入的前置增量，**不是产品来源持久接入完成**。没有切换 Manager、箱子或尸体入口，没有向库存插入生成物。FZ-1/2 仍开放，不宣布框架冻结或暂停自动化。

## 2. 已落地的边界

| 边界 | 本轮实现 | 不包含的权限 |
|---|---|---|
| 来源资格 | [GeneratedSourceAdapter](../../Source/demo_map/demo_mapShanmenItemGeneratedSourceAdapter.h) 只接受已登记 SlotId；从 M01/P8 原目录查找并建立原 Projection | 调用方不能传入自选政策、seed、奖励定义或价值；不等于服务已验证 Owner/Run |
| 完整计划 | 原 Planner 规划一次，原 BuildContainerSeed 规范化 section/slot；数量减少或定义/数量错位即拒绝 | 不用原始规划的 Equipment 槽位直接代替最终容器布局；不写入 Runtime/World |
| 定义 | [DefinitionAdapter](../../Source/demo_map/demo_mapShanmenItemDefinitionAdapter.cpp) 从当前产品目录生成原迁移规则的能力/上限；Migration 复用并保留 Code A/B 一致性门 | 不接受调用方声明的能力，不自动接纳权威目录中缺失的定义 |
| 元数据 | [MetadataAdapter](../../Source/demo_map/demo_mapShanmenItemMetadataAdapter.h) 新增 PlannedStack 输入，复用已有事件/词缀检查与字段转换 | 不创建第三套奖励元数据映射；错误不输出部分候选 |
| 重放责任 | API 明确为 fresh candidate；调用方必须先查 durable 来源历史，恢复使用已接受完整计划 | 重复规划一致性测试不是“恢复时重新抽取合法”的许可 |

候选保留来源内容身份、角色、Slot/Projection、分布/预算/标记/遭遇/三类政策、有效 seed、预算/价值/残值、保底前后值、兼容/回退标志和有序完整条目。空间包/空间戒指容量来自原目录。无保底提交的来源保持输入保底状态，不把默认输出误作新保底值。

无效身份、未登记来源、非法顺序和当前产品保底范围外的输入均清空输出并拒绝。新 Adapter 是受信产品侧的候选构造器；ItemContent、活动 Owner/Run、顺序及来源内容授权仍必须由同一权威服务在提交时核对，不能把结构有效当作已授权。

## 3. 测试与构建

本轮首次执行即通过，没有旧实现 Red 证明或失败后修复的声明。新增 3 个注册用例，具体变体如下：

- `CanonicalSlots`：149 M01 + 149 P8 槽位；同一非零 Run、顺序 7、保底 2；候选、政策/来源内容、条目数量/价值/槽位、元数据、确定性来源身份及 codec 往返。
- `FailClosed`：9 类非法上下文，从非空有效输出基准验证拒绝后无旧请求残留；未知定义拒绝且不保留原能力。
- `DefinitionAndMetadata`：当前目录所有定义的身份/资源上限与飞剑/投掷标签，实际规划元数据，以及非法事件枚举从有效基准拒绝并清空输出。

| 现跑验证 | 结果 |
|---|---|
| 新适配专项 | 3/0；包含于下列 Items，不重复累计 |
| Items 完整组 | 106/0 |
| ThrownWeaponContent / ControlledWeaponContent | 各 1/0 |
| demo_map 完整旧根 | 1330/0 |
| 本轮独立用例 | **1438/0**，每次精确唯一成功数、结束队列和原生退出 0 联合核对 |
| Editor / Game Development | 原生 0/0，实际 12 / 9 actions |
| 改动路径推导回归 | 13 路径、2 条重叠规则、7 个必跑组、4 份完成日志通过 |
| 映射检查器自检 | 549/549 |

没有重跑完整 Shanmen 根。P28.34.r1 的 2778 项是前一产品输入的历史证据，不作为本轮全部新根通过或最终冻结验收。298 是一个用例内的槽位遍历数，不另计 298 个注册用例。原始日志与 SHA 见 [Development Log](../Log/Dev.D.UE.0.0.10.P28.35.r0_log.md)。

五份自动化日志各有 13 条既存启动 `Condition failed` Error，无 Fail/Fatal/Ensure/Unhandled/Assertion；不宣称零 Error。没有 HTTP 超时日志行。

## 4. 权威、内存和模块边界

本轮不修改 ShanmenItems 持久接收算法和 schema 4；原 Code A Runtime、Code B 迁移交叉核对以及单一 AuthorityService 归属不变。核心三模块没有新增 Engine 或产品依赖。产品 Adapter 只调用既有 Planner，不把生成器/RNG 送入领域内核。

构造时原 Plan、规范化 Seed 与新 Candidate 临时共存，空间随条目数线性增长；使用完成移动候选，没有增加长期缓存或 UI 全量副本。这里是静态分配形态判断，不是 RAM 峰值、字节预算、锁内保存耗时或吞吐实测，不能据此宣布性能完成。

## 5. 剩余结构缺口

1. 权威接收仍要求定义已存在；需要在同一权威边界内解决合法新定义的目录接纳，不能弱化为“任意传入定义即可信”。
2. 产品路由仍要先查 durable 历史和当前顺序/保底，再决定是否构建新候选；内容授权、原结果恢复、Runtime/World 物化失败后继续仍未接通。
3. 新获物、使用、消耗、遗失与 Extraction/Death/Abandon 的身份组合未闭合。保留 `AcquiredItemMismatch`，不恢复旧 writer 或绕过 `ItemNotPrepared`。
4. FZ-2 的实际生命周期调用点、未覆盖生成/释放组合和强制 EndPlay 仍须核对；FZ-3 在上述闭合且最终产品输入固定后另跑完整双根和双构建。

下一步沿 [冻结索引](../Architecture/Dev.D.UE.0.0.10_FoundationClosure_Index.md) 接合法目录/来源授权与 durable 读取，随后才切换产品查询/物化；不能因为 fresh Adapter 通过而声称生成→消费→终局已经端到端成立。

## 6. 交付与 P/F 边界

精确交付 8 个 Source 路径、2 个回归脚本路径、Report/Log/索引，共 13 路径。1628 项验证输入固定，103 个既有未跟踪用户文件及两份 OverallReadiness 用户修改保持、排除提交。

遵守 [P 阶段基线](../Process/P_STAGE_BASELINE_0_0_10.md)：仅无头契约测试与 Editor/Game 编译，未接物理输入，未改地图/内容资产、玩法数值、UI 或敌人行为；未启动 Editor UI、PIE、Standalone、游戏程序、截图、Smoke、Cook、Package。原日志本地保留，GitHub 发布报告及证据索引，不声称原件已上传。
