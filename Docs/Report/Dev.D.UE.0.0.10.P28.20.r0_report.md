# Dev.D.UE.0.0.10.P28.20.r0 Report

## 1. 状态与范围

`P28_20_COMPLETE`（本阶段验证完成，不是整体冻结）。基线P28.19 / `a8c8cea3eb6ffd09cd199ee3f3973a65b7f6b95d`，2026-09-20。本轮闭合直接Manager去激活时，其额外遭遇对象拒绝销毁却丢失原引用、下游继续清理物品World的问题。实际Red、最小修复、双构建、专项与完整新旧根均有证据；首次新根中断原件保留，相同锁定源码下Recovery1完整1430/0通过，合计完整根2760成功/0失败。FZ-1/2仍开放，不宣布整体冻结或进入F阶段。

## 2. 入口与反例

非M01 InitializeWorldContent调用InitializeEnemyEncounterContent。后者持有14个EnemyActors，但只把PrimaryMelee/PrimaryRanged/PrimaryHeavy三个引用绑定给GameMode。因此P28.19保护三个GameMode槽并不覆盖Manager的其余对象。真实去激活顺序是GameMode→DestroyRuntimeContainers（内含DestroyEnemyEncounterContent）→Items.TeardownWorld。

旧Manager敌人清理先Reset原数组和导航标记，再忽略每个Destroy结果；容器清理随后返回true，允许下层World物品被释放。拒绝对象仍存活，却失去原清理owner。

扩展既有ManagerDeactivationRetention：保留前面的战斗/物品/灵石拒绝变体，追加两个Manager独有遭遇对象和两单位World物品。一个对象实际拒绝两次、另一个接受；检查原引用、非空导航标记、下游物品身份/数量/绑定、活动上下文、原Run与持久快照保持。解除拒绝后，两个遭遇对象及物品各释放一次，原上下文才清空。Red为0成功/1失败、队列1、原生0、崩溃指标0；5条最终Expected失败原件保留。

夹具是同一瞬态World中的实际AuthGameMode和Manager端口；未运行正式地图生成、导航构建、完整激活或玩家流程，不冒充全部遭遇入口已经验收。

## 3. 最小修复

- 私有DestroyEnemyEncounterContent返回bool；按数组快照尝试释放，只保留拒绝的原弱引用。
- 导航标记仅在全部原对象释放后清空；成功前缀不重复销毁。
- DestroyRuntimeContainers接收拒绝，停止清上下文及向下执行Items.TeardownWorld。
- 生产cpp 11新增/6删除、私有声明1新增/1删除、测试57新增；总69新增/7删除。不新增公共API、schema、恢复记录、故障注入或注册测试。

没有修改角色行为、数值、内容资产或输入/UI。初始化失败调用方原本即返回false，共用清理现在保留拒绝项；其实际生成故障以及强制EndPlay另列待审计，不纳入本专项证明。

## 4. 验证状态

| 验证 | 已核验事实 |
|---|---|
| Red Editor构建 | 4 actions / 21.48秒，SUCCEEDED/原生0 |
| RedProof | 0成功/1失败，队列1、原生0、崩溃指标0，5条最终Expected失败 |
| 最终Editor构建 | 27 actions / 129.78秒，SUCCEEDED/原生0 |
| WorldLifecycle专项 | 7成功/0失败，队列7、原生0、崩溃指标0 |
| ProductFlow专项 | 5成功/0失败，队列5、原生0、崩溃指标0 |
| 最终Game构建 | 26 actions / 129.55秒，SUCCEEDED/原生0 |
| Legacy完整根 | 1330成功/0失败，队列1330、原生0、崩溃指标0 |
| 首次Shanmen完整根 | 中断：1181成功/0失败，无最终队列/run-state/原生退出；原进程已不存在，不视为通过 |
| Shanmen完整根Recovery1 | 1430成功/0失败、队列1430、SUCCEEDED/原生0、崩溃指标0；15:47:25.7826086UTC完成 |
| 映射自检 | 本阶段独立执行529/529，原生0 |
| 改动映射 | 实际PASS：6路径、2规则、7必跑组、2个健康完整根日志 |

08:43:29.9328043UTC锁定1617产品输入和103原用户文件。初次新根仅1181条后中断、原进程消失且缺少最终状态，原因未证实；不补写原退出码、不将HTTP警告推定为终止原因。复核锁定输入不变后只重跑未完成的新根，不重复健康双构建、两专项和旧根。Recovery1原exec80008最终退出0，独立日志与run-state均正常结束；16:21UTC核验并通过实际覆盖门。

完整新旧根1430+1330=2760项，不重复计算两专项12项、原始Red或中断1181条。[Development Log](../Log/Dev.D.UE.0.0.10.P28.20.r0_log.md)保留历史检查点及11项原始日志路径/SHA；原件仅在本地Saved保留，GitHub交接的是说明与哈希，不声称原始日志已上传。

## 5. 边界与交接

遵守[P阶段基线](../Process/P_STAGE_BASELINE_0_0_10.md)，只使用Editor/Game编译及UE-Cmd无头契约测试，无Editor UI、PIE、Standalone、产品exe、正式地图/内容编辑、物理输入、玩法/UI开发或Smoke/Cook/Package。

[有限冻结索引](../Architecture/Dev.D.UE.0.0.10_FoundationClosure_Index.md)纳入本阶段局部证据，FZ-1/2继续开放。交接限定Manager cpp/h、既有测试、索引、本Report/Log六路径；原103未跟踪文件及两份OverallReadiness修改不动。尚需审计其他清理调用点、强制EndPlay、全入口权威路由和完整生命周期组合；不把直接瞬态World端口证明扩大成正式地图或玩家流程验收。
