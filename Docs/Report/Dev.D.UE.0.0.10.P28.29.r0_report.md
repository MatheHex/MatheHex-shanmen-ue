# Dev.D.UE.0.0.10.P28.29.r0 Report

## 1. 状态与范围

`P28_29_PASS`。2026-09-21 UTC；基线 P28.28 / `48b2a40f0a1e45b74f8baa4f80b56b66b60c1b8a`。本阶段修复 M01 尸体容器初始化失败后，真实销毁拒绝导致 Manager 丢失原对象归属的结构缺口。仅调整 [Manager](../../Source/demo_map/demo_mapV3ProgressionManager.cpp) 的该失败分支并扩展 [既有测试](../../Source/demo_map/demo_mapShanmenPreparationAdapterTests.cpp)，不是奖励设计、敌人行为或游戏性开发。

生产改动 7 新增/1 删除，测试 70 新增；没有新 API、schema、权威、恢复字段、故障注入端口或测试注册。仍使用原 `Corpses` 数组和 `DestroyRuntimeContainers` → `DeactivateProfileWorld` 清理链。

## 2. 反例、修复与边界

真实切换后的持久 Run 与 Runtime 已建立，生成入口收到有效 M01 身份和来源。规划成功，但当前旧 Profile 奖励提交端口不接受这个 Run；新对象尚未完成容器初始化。测试利用既有引擎销毁拒绝条件，验证失败分支的释放结果：旧代码忽略 Destroy 拒绝，也没有把对象留给 Manager，随后去激活错误返回成功。首次 Red 为 0 成功/1 失败，五条最终断言失败，原件保留。

现在只在有效对象未处于销毁中且 Destroy 返回 false 时，将原对象加入既有 `Corpses` 归属数组。不会把生成来源标成已成功、不会提交奖励或改变持久 Run。去激活通过既有清理逻辑返回 false 并保留原 owner；解除拒绝后完成原对象的一次释放。

测试扩展在既有 ManagerDeactivationRetention 中覆盖：

- 使用真正持久启动后的 Run、原非零装备/库存快照、有效 M01 身份、实际生成入口与真实 Actor；不是空 Flow 或无效身份直接触发早期拒绝。
- 两次去激活拒绝都保留同一对象、Manager 活动态、原 Run 和持久快照；来源成功标记及旧持久奖励记录均为空。
- 故障解除后恰好一次释放，完成后重复去激活不重复释放；另有销毁不拒绝的健康失败回滚对照。

夹具是瞬态 World 与隔离持久根，未启动正式 M01 地图、Manager 完整 Initialize 或物理死亡输入。只动态证明本 M01 失败分支，不冒充其他尸体重载、所有奖励生成、强制 EndPlay 或跨进程恢复均已通过。

**另一个权威路由缺口仍开放：** `PrepareGeneratedRewardSource` 当前仍调用旧 Profile Session 提交，旧协调器要求自己的 RunActive。新 Flow 走 Shanmen StartPreparedRun 不会把旧协调器改为活动 Run，因此当前 M01 生成在本真实切换 Run 上被拒绝。本阶段不绕过该拒绝、不恢复旧 writer、不凭清理修复宣称奖励生成可用；后续须沿已批准奖励契约核对唯一持久接收端，不改掉落数值或增设第二权威。

## 3. 验证

Editor/Game Development 均实际编译 Manager 并链接成功、原生 0/0；不是 up-to-date 检查。既有 WorldLifecycle 7/0、ProductFlow 5/0 通过；本阶段新完整根 1430/0、旧完整根 1330/0，精确结束队列、原生退出 0、Fatal/Ensure/Unhandled 指标均符合门槛。两根合计 2760 个独立成功用例；12 个专项用例为其中子集，不重复累计。

改动驱动覆盖门通过：精确五路径匹配两条规则、七个必跑组，证据来自本阶段两份完整根原件。新根于 19:34:02.7531548UTC 正常结束；运行中几次心跳只核对进展与输入保持，没有重复启动或把中间成功数当成最终验收。

映射检查器自检 537/537；原注册数不变。新鲜产品回归使用本阶段固定输入，不复用 P28.26 成功结果。首次 Red、实际构建和自动化日志均保留本地，GitHub 的 Log 登记路径与 SHA，不声称原件已上传。

## 4. 交接与剩余范围

本阶段精确提交五路径：Manager、既有测试、Report、[Development Log](../Log/Dev.D.UE.0.0.10.P28.29.r0_log.md)、[有限冻结索引](../Architecture/Dev.D.UE.0.0.10_FoundationClosure_Index.md)。103 个原未跟踪文件和两份 OverallReadiness 用户修改保持不变；1617 项产品/验证输入锁中，仅上述两个源码路径相对 P28.26 改变。

FZ-1/2 仍开放：生成奖励持久路由、其他容器生成失败与强制结束等既有清单项尚需核对。没有宣布最终冻结，不暂停自动化。遵守 [P 阶段基线](../Process/P_STAGE_BASELINE_0_0_10.md)，未接物理输入，未改正式地图/内容资产/玩法/UI，未启动 Editor UI、PIE、Standalone 或游戏程序，未做 Smoke/Cook/Package。
