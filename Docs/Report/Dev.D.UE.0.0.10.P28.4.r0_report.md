# Dev.D.UE.0.0.10.P28.4.r0 Report

## 1. 状态与范围

状态：`P28_4_VERIFIED`。最小修复、Editor/Game 构建、ProductFlow 专项、完整新旧根及改动映射均已核验。首次完整新根中断原件保留；独立补跑于 2026-09-17 15:39:59 UTC 完成，16:12 heartbeat 复核最终证据并交付本阶段。这是局部结构缺口闭合，不是整个框架冻结。

基线为 `8cab3d3611894b0d8b7fcb94e82df7ea20767f2e`，已包含 P28.3 和最新总体报告。本轮只处理 FZ-2 的 `RollbackPreparedProfileRunFor0909B` 调用边界，不新增游戏性、UI 或通用恢复系统。

## 2. 真实反例与根因

Manager 在调用 `ProfilePreparationFlow.CancelActiveRunForActivationFailure()` 后，无条件执行 `DeactivateProfileWorld()` 并清除待结算标志，之后才检查回滚结果。下层因新物品权威 RecoveryRequired 而拒绝时，Run 保留，但外层已清理 Manager 上下文；ItemSubsystem.TeardownWorld 还会删除真实 World 物品和绑定。

先增加测试、不改生产行为：隔离 Profile 实际迁移/cutover，整备并启动持久 Run，三颗丹、生命 1，瞬态 World 内创建两单位材料。通过既有故障注入让真实持久服务进入恢复状态，再连续两次调用 Manager 生产回滚端口。原测试 0 Success / 1 Fail：两次 Manager 状态保持、两次真实 World 物品/绑定保持断言均失败；正常回滚变体及 Run/丹药/生命保持断言未失败。

测试通过友元绑定现有 Manager 的 Runtime、Flow、World 物品列表、活动及 pending 字段；没有执行正式 M01 初始化、BeginPlay 或玩家输入。因此证明范围是 Manager→Flow/ItemSubsystem 的该生产端口，不是完整 M01 激活链或全世界原子恢复。

## 3. 最小实现

- 将 `DeactivateProfileWorld()` 和 `bSettlementPending = false` 移入已有 `bAtSectReady` 成功分支。
- 拒绝时沿用原诊断和 false 返回，保持原上下文；不创建新 ID，不切换旧权威，不修改 schema 或下层恢复规则。
- 正常回滚仍清理瞬态 Runtime/World，并保留持久 Shanmen ActiveRun 的精确恢复身份；不把技术启动失败当玩家放弃。

## 4. 已完成验证

- 原 Red Editor 首次启动被中断：外层退出 1073807364，没有原生完成记录。保留部分 stdout，不将其写为编译成功、源码错误或已证明的环境故障。
- 独立确认无残留后，Red Editor 重试成功，26 actions / 239.45 秒 / native 0。
- RedProof：0/1，队列 1、native 0、崩溃指标 0；native 0 不等于测试通过。
- Fixed Editor：4 actions / 36.39 秒 / native 0。
- `Shanmen.0_0_10.Items.ProductFlow`：5/0，队列 5、native 0、崩溃指标 0。新增用例覆盖正常清理、两次拒绝保持、非零物品与生命、原 Run 身份及存档字节保持。
- Game：25 actions / 201.88 秒 / native 0，于 03:49:19 UTC 完成；04:32 续接只复核原件，不重复构建。
- 回归映射检查器自检原件为 517/517，哈希及脚本输入已核对。
- 完整旧根已完成：1330 Success / 0 Fail，队列 1330、native 0、崩溃指标 0。
- 首次完整新根留下 1173 Success / 0 Fail 的未完成日志，末尾时间 05:46:55 UTC；14:16 检查已无测试进程、无最终队列或 run-state，旧 exec session 也不可恢复。原因与原生退出码未知，不冒称测试通过、源码错误或已确定的环境故障。
- 核对 1617 个输入、103 个用户文件及总体报告保持后，14:18 UTC 仅重新运行完整 Shanmen 新根，独立保存 Resumed 证据；旧根和构建不重跑。实际结果为 1427 Success / 0 Fail，队列 1427、native 0、崩溃指标 0，于 15:39:59 UTC 完成。专项 5 项属于此根，不重复累计，也不拼接首次中断的 1173 项。
- 三源码路径命中 ProductRunItemUse / ProfileAuthority / ProfilePreparationProductFlow，要求 6 个去重组，包括完整 Shanmen 根和旧 Profile/CodeB/Schema/远程兼容。精确六文件映射通过：Changed=6 / Rules=3 / Required=6 / Logs=2，使用本阶段完整新旧根原件。
- 最终复核时，1617 个锁定输入及 103 个既有用户文件的原字节均保持；两份 OverallReadiness 未提交修改也保持。原日志与构建证据的 SHA-256、失败记录和源码哈希见 Development Log；本轮交付不冒充重新运行已完成构建。

## 5. 剩余边界与交付纪律

FZ-2 仍未整体关闭：本次只处理下层回滚拒绝时的保持；成功回滚后 GameMode 的 World release 拒绝传播、一般 ActivatePreparedProfileWorld 的旧调用点、终局与强制 EndPlay 的顺序仍须按可达性分别审计。不能将此局部保护扩大为整个 Manager 清理已具备原子性。

FZ-1 的丢弃/存箱/未整备新获物政策边界和 FZ-3 最终冻结也仍待完成。本阶段只精确交付三个源码、冻结索引、本 Report/Log；保留 103 个原用户文件及用户报告任务留下的两份 OverallReadiness 未提交修改，Saved 原日志不上传。冻结索引只登记本次局部保护，不将 FZ-2 改成已全部关闭，当前监控继续用于有限收尾。

未接物理输入、未改正式地图或内容资产、未启动 Editor UI、PIE、Standalone、产品 exe、截图、Smoke、Cook 或 Package。

- [Development Log](../Log/Dev.D.UE.0.0.10.P28.4.r0_log.md)
- [有限冻结索引](../Architecture/Dev.D.UE.0.0.10_FoundationClosure_Index.md)
