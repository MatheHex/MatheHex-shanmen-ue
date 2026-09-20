# Dev.D.UE.0.0.10.P28.18.r0 Report

## 1. 状态与范围

`P28_18_COMPLETE`。基线P28.17 / `8432c354c700fb75063a75b45263ad4736a664aa`，2026-09-20。本轮只处理既有GameMode去激活中动态撤离区域拒绝销毁却丢失原引用的问题。实际Red已取得，最小修复后的双目标构建、两组专项、完整双根及改动驱动覆盖全部通过。本阶段完成不等于底层整体冻结。

## 2. 已复现的缺口

DestroyM01ExtractionFoundation原先忽略动态区域Destroy的返回值并清空整个数组；上层随后清任务状态并返回true。若实际拒绝，原动态Actor留在World但已失去清理owner，故障解除后也无法续清理。正式M01地图的authored区域按原规则仅停用，不应销毁；本轮没有把这两种所有权混为一谈。

扩展既有PreparationDeactivationRetention：保留飞剑与敌人/Boss拒绝变体，追加一个动态区域实际拒绝两次、另一个动态区域成功、一个瞬态authored区域仅停用。要求拒绝时保持同一引用/稳定ID与未完成任务上下文，停用交互但不重置已解锁的原撤离权威，已成功前缀不重复，解除故障后动态区域恰好一次销毁，authored对象不被销毁，原物品持久快照与Run不变。Red为0成功/1失败、精确队列1、原生0、崩溃指标0，5条最终Expected断言失败，原件保留。

## 3. 有界修复

- 私有清理函数返回bool，按数组快照操作；无效/正在销毁项视为已释放，仅保留真实拒绝的动态owner。
- 先复用现有SetProjectionActive(false)停用区域；authored对象留在World，动态对象才请求销毁。停用交互不等于物理清理已完成。
- 去激活调用方遇到false不继续清任务上下文；初始化入口也先等待旧清理成功，再重置撤离权威或创建新区域。
- GameMode.cpp 19新增/8删除，GameMode.h 1/1，既有测试49新增；合计69新增/9删除。仅改变一个既有私有返回值，无新公共API、schema、恢复记录、故障端口或注册测试。

夹具是瞬态World与GameMode对象，authored属性在瞬态对象上配置；未编辑或运行正式地图，没有设计撤离玩法或UI。初始化入口仅静态确认调用次序，不冒充完整初始化、全部再激活入口或强制EndPlay验收。

## 4. 验证结果

| 验证 | 已核验结果 |
|---|---|
| Red Editor编译 | 4 actions / 16.44秒，SUCCEEDED/原生0 |
| RedProof | 0成功/1失败，精确队列1、原生0、崩溃指标0 |
| 最终Editor编译 | 37 actions / 142.39秒，SUCCEEDED/原生0 |
| WorldLifecycle专项 | 7成功/0失败，精确队列7、原生0、崩溃指标0 |
| ProductFlow专项 | 5成功/0失败，精确队列5、原生0、崩溃指标0 |
| 最终Game编译 | 36 actions / 140.09秒，SUCCEEDED/原生0 |
| Legacy完整根 | 1330成功/0失败，精确队列1330、原生0、崩溃指标0 |
| Shanmen完整根 | 1430成功/0失败，精确队列1430、原生0、崩溃指标0 |
| 映射自检 | 独立529/529、原生0 |
| 实际覆盖门 | PASS：6路径、2规则、81必跑组、2份完整日志，原生0 |

03:41:45.566UTC锁定1617产品输入与103原用户文件；原exec6611依次执行Editor、WorldLifecycle7、ProductFlow5、Game、旧根1330、新根1430，新根于04:56:09.1838933UTC结束，原运行器正常退出0。05:29UTC重新核验锁定输入与用户文件未变，再对本阶段六路径执行实际覆盖门通过；未拼接中断日志、未重复启动。完整双根合计2760成功/0失败，两组专项是重复专项验证，不另加进唯一测试总数。十项完成原件及SHA-256见[Development Log](../Log/Dev.D.UE.0.0.10.P28.18.r0_log.md)；首次Red失败仍独立保留。

## 5. 边界与待办

遵守[P阶段基线](../Process/P_STAGE_BASELINE_0_0_10.md)：仅Editor/Game编译与UE-Cmd无头契约验证，无Editor UI/PIE/Standalone/产品exe，不改正式地图/内容，不接物理输入、不做玩法/数值/AI/UI开发，不做Smoke/Cook/Package。

[有限冻结索引](../Architecture/Dev.D.UE.0.0.10_FoundationClosure_Index.md)更新至本阶段，FZ-1/2仍开放。动态区域拒绝证明不替代完整M01创建、其他非M01投影、兼容终局、空间包部分释放、再激活或强制EndPlay。尤其非M01活动内容再进入分支并非本次初始化次序检查的动态覆盖范围，仍需核对真实入口条件。本阶段交接三源码、索引、本Report/Log六路径；103原未跟踪文件及两份OverallReadiness用户修改不动。Saved原始证据仅本地保留，GitHub Development Log记录路径与哈希，不声称原始日志已上传。
