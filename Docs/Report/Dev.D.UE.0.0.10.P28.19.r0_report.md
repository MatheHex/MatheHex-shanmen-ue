# Dev.D.UE.0.0.10.P28.19.r0 Report

## 1. 状态与范围

`P28_19_COMPLETE`。基线P28.18 / `880d010dfe996eedba38abc47f57e9ccb5d7b749`，2026-09-20。本轮只闭合直接GameMode去激活尾部的非M01任务对象拒绝销毁时丢失原引用、错误确认完成的问题。实际Red已取得并实施最小修复；完整双根2760/0、Editor/Game双构建、两专项、改动驱动实际覆盖及映射自检均通过。不宣布整体冻结；发布提交以包含本Report/Log的Git提交为准。

## 2. 实际入口与缺口

既有非M01任务创建路径持有三个TrainingTarget、ExitZone与FriendlyUnit；敌人投影由旧创建路径或V3 Manager的BindV3EnemyProjections写入三个既有引用。Manager.DeactivateProfileWorld会先调用GameMode.DeactivateV3MissionContentForPreparation并依其返回值决定是否继续释放下层World。

P28.18已覆盖前面的动态撤离区域；其后非M01清理仍忽略目标及五个单对象槽的Destroy返回值，无条件清引用、计数去重记录与活动标志。实际拒绝后，World对象仍在但原清理入口无法再找到它们。

扩展既有PreparationDeactivationRetention，不新增注册测试：瞬态夹具先完成已有飞剑/敌人/动态撤离变体，再配置三个目标与五个单对象槽；一个目标、出口、近战/远程/重型投影及友军实际拒绝两次，另两个目标成功。要求保持六个原对象引用、非空计数去重记录与原Run，成功前缀不重做；解除故障后八个对象各恰好释放一次，任务上下文才清空，持久物品快照不变。Red为0成功/1失败、精确队列1、原生0、崩溃指标0，5条最终Expected失败，原件独立保留。

## 3. 最小修复

- 局部清理lambda直接处理原类型弱引用，仅在无效、正在销毁或真实Destroy成功后清引用。
- 按目标数组快照处理，回存拒绝项；五个单对象槽全部独立尝试，不因前一个拒绝而跳过后续对象。
- 任一拒绝返回false，保留计数去重记录和活动任务上下文；全部完成才清这两项。
- 只改GameMode.cpp与既有测试：生产19新增/22删除，测试74新增；总93新增/22删除。无新公共API、头文件、schema、恢复记录或注册测试。

夹具是瞬态World与GameMode对象，仅证明原去激活端口，不冒充正式地图生成、玩家流程、完整再激活或Manager终局组合。未修改敌人行为、数值、区域/角色类或任何内容资产。

## 4. 最终验证

| 验证 | 已核验结果 |
|---|---|
| Red Editor构建 | 4 actions / 11.63秒，SUCCEEDED/原生0 |
| RedProof | 0成功/1失败，队列1、原生0、崩溃指标0 |
| 最终Editor构建 | 4 actions / 12.35秒，SUCCEEDED/原生0 |
| WorldLifecycle专项 | 7成功/0失败，队列7、原生0、崩溃指标0 |
| ProductFlow专项 | 5成功/0失败，队列5、原生0、崩溃指标0 |
| 最终Game构建 | 4 actions / 22.91秒，SUCCEEDED/原生0 |
| Legacy完整根 | 1330成功/0失败，队列1330、原生0、崩溃指标0 |
| Shanmen完整根 | 1430成功/0失败，队列1430、原生0、崩溃指标0；07:27:14.7232705UTC完成 |
| 映射自检 | 独立529/529、原生0 |
| 实际覆盖门 | PASS：5改动路径、2映射规则、81必跑组、2份本阶段健康完整根日志 |

06:11:03.5749517UTC锁定1617产品输入与103原用户文件，原exec98951按Editor→两专项→Game→旧根1330→新根1430串行执行并正常退出0；运行期间不改产品输入、不重复启动。08:03:12.4577765UTC再次核验锁定输入、用户文件精确集合与哈希、两份保护文档、空暂存区及diff check；四组成功数/队列/原生退出/崩溃指标独立复算一致。五路径命中M01GameMode和ItemProductAdapters两规则的81必跑组，实际覆盖门已使用本阶段两份完整根日志通过。完成原件和SHA见[Development Log](../Log/Dev.D.UE.0.0.10.P28.19.r0_log.md)，首次失败不删除；专项是全根中的重复验证，不额外计入2760。日志中的HTTP探测超时警告不计作测试失败，亦未据此重启运行器。

## 5. 边界与交接

遵守[P阶段基线](../Process/P_STAGE_BASELINE_0_0_10.md)。仅Editor/Game编译和UE-Cmd无头契约验证，没有Editor UI、PIE、Standalone、产品exe、正式地图/内容编辑、物理输入或玩法/UI开发，无Smoke/Cook/Package。

[有限冻结索引](../Architecture/Dev.D.UE.0.0.10_FoundationClosure_Index.md)更新为本阶段证据，FZ-1/2仍开放。非M01活动内容再进入路径的条件已阅读，但本轮没有证明全部前置条件下的新故障或为其扩大修复；局部创建失败、其他调用点、空间包及强制EndPlay仍须分别取证。本阶段交接严格限定两源码、索引、本Report/Log五路径，原103未跟踪文件及两份OverallReadiness用户修改不动；Saved原件仅本地保留，不把本阶段完成当作整体闭合。
