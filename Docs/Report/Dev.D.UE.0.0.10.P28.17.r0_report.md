# Dev.D.UE.0.0.10.P28.17.r0 Report

## 1. 状态与范围

`P28_17_COMPLETE`。基线P28.16 / `73ec9836182eeb70d805c6f3c48dab94423b6830`，2026-09-20。本轮闭合GameMode去激活时M01敌人/Boss投影拒绝销毁却丢失原owner的问题；实际Red、有界修复、双目标编译、两专项、完整双根与改动驱动覆盖门均已核验通过。不宣布底层整体冻结。

## 2. 已复现的结构缺口

既有DeactivateV3MissionContentForPreparation会先等待战斗Run释放成功，但随后DestroyM01EnemyContent预先清M01EnemyActors和Boss弱引用，不检查Destroy结果；调用方继续清撤离投影和任务标志并返回true。实际销毁拒绝后原Actor仍存在，故障解除也因原引用已丢失而无法续清理。

扩展既有PreparationDeactivationRetention：先保留两次真实飞剑拒绝，再解除飞剑故障，令原敌人和经既有定义初始化的Boss实际拒绝销毁，另一个敌人正常释放。连续两次要求返回false、保留原owner与未完成任务上下文；恢复后每个Actor恰好释放一次，原物品持久快照与Run不变。Red为0成功/1失败、精确队列1、原生0、崩溃指标0，5条最终Expected断言失败，原件保留。

## 3. 有界修复

- 私有DestroyM01EnemyContent返回实际清理结果，快照遍历后只保留拒绝项，Boss引用仅在失效或正在销毁时清除。
- 逻辑终止、敌人抑制仍按原契约执行：账本禁止新Boss提交，但保留原Run与遭遇去重记录；逻辑终止不被当作World已经释放。
- 去激活调用方收到false即停止后续撤离/任务清理；M01敌人初始化也先检查旧释放结果，拒绝时不重置原账本或创建新批次。
- 三源码合计75新增/11删除；仅一个既有私有函数返回值改变，无新增公共API、持久schema、恢复记录、友元或注册测试。

测试使用既有瞬态World与GameMode对象端口，不启动真实敌人玩法、正式M01地图或物理玩家流程。初始化入口新增检查是静态调用次序保证，未冒充完整M01初始化故障专项。

## 4. 验证结果

| 验证 | 已核验结果 |
|---|---|
| Red Editor编译 | 4 actions / 11.39秒，SUCCEEDED/原生0 |
| RedProof | 0成功/1失败，精确队列1、原生0、崩溃指标0 |
| 最终Editor编译 | 37 actions / 149.61秒，SUCCEEDED/原生0 |
| WorldLifecycle专项 | 7成功/0失败，精确队列7、原生0、崩溃指标0 |
| ProductFlow专项 | 5成功/0失败，精确队列5、原生0、崩溃指标0 |
| 最终Game编译 | 36 actions / 147.13秒，SUCCEEDED/原生0 |
| 完整旧根demo_map | 1330成功/0失败，精确队列1330、原生0、崩溃指标0 |
| 完整新根Shanmen.0_0_10 | 1430成功/0失败，精确队列1430、原生0、崩溃指标0 |
| 映射自检 | 独立529/529、原生0 |
| 实际覆盖门 | PASS，6路径、2规则、81必跑组、2份健康完整根日志 |

01:10:37.173UTC锁定1617产品输入与103原用户文件，原exec9346依次验证Editor、WorldLifecycle7、ProductFlow5、Game、旧根1330、新根1430；新根于02:27:10.228UTC完成，原运行器退出0。运行期间不改锁定输入、不重复启动，03:00UTC独立复核锁定输入与用户文件全部一致。完整双根合计2760成功、0失败；单独执行的专项7+5不重复计入该合计。

六路径命中M01GameMode与ItemProductAdapters两规则，81必跑组均由本阶段健康双根提供实际日志证据；未以静态映射或529项自检替代覆盖门。十项完成原件（含首次Red与RedBuild）及SHA-256见[Development Log](../Log/Dev.D.UE.0.0.10.P28.17.r0_log.md)。引擎/HTTP警告原样保留，测试通过不代表内存性能或F阶段验收。

## 5. 边界与交接

遵守[P阶段基线](../Process/P_STAGE_BASELINE_0_0_10.md)：仅Editor/Game编译与UE-Cmd无头契约测试，无Editor UI/PIE/Standalone/产品exe，不改地图/内容资产、不接物理输入、不调数值/AI/手感/UI，不做Smoke/Cook/Package。Boss生命值来自现有定义，只用于非零保持断言。

[有限冻结索引](../Architecture/Dev.D.UE.0.0.10_FoundationClosure_Index.md)最新完成阶段更新为P28.17，FZ-1/2仍开放。撤离区域释放、非M01其他投影、激活局部失败和强制EndPlay等调用点仍需分别核对，不能把本轮局部清理证明当完整World原子恢复。精确交接三源码、索引、本Report/Log六路径，包含这些路径的Git提交标识阶段基线；原103未跟踪文件及两份OverallReadiness用户修改不动。原始Saved证据仅本地保留，不宣称已上传GitHub。
