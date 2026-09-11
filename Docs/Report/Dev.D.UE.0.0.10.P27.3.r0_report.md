# Dev.D.UE.0.0.10.P27.3.r0 Report

## 1. 结论

P27.3 已在 P27.2 的 Run 级唯一阵法生命周期上建立显式锚点操作网关。一次设备无关操作固定
`RunId + AnchorDefinitionId + AttemptId`，由生命周期唯一入口顺序复用既有 ProductHost 的
Prepare、Commit 和 World Placement 权威；精确重试复用既有材料与世界回执，不重复扣除材料、
不重复创建 Actor。

本阶段同时修正完整阵图的终止语义：Deploying 阵图走 Cancel，Active/Ended 阵图走正常 End，
两者都必须先完成产品世界清理，之后才允许释放共享 Combat Run。

## 2. 阶段问题与范围

P27.2 已固定产品清理先于 Combat Run 释放，但调用方仍无法通过生命周期执行 P27.0/P27.1 Host
已有的三段锚点操作。若直接暴露 Host，会重新产生第二条可变产品路径；若所有终止都调用 Cancel，
已完成并进入 Active 的阵图又会被既有 Session 正确拒绝。

P27.3 新增：

- `Fdemo_mapShanmenFormationAnchorOperation`：不可变的 Run、锚点定义与尝试身份；
- `Fdemo_mapShanmenFormationAnchorOperationResult`：保留 Prepare、Commit、Placement 全链证据；
- 生命周期公开的唯一 `TryExecuteAnchorOperation()`；
- Controller 私有、仅生命周期可调用的三段式产品网关；
- Active/Ended 正常结束与 Deploying/Cancelled 取消的明确分流。

## 3. 显式锚点操作契约

操作在访问物品或世界权威前必须同时满足：有效 RunId、非空 AnchorDefinitionId、有效 AttemptId、
活动且有效的生命周期、同一 Ready Coordinator、唯一 ProductHost、有效且未 teardown 的 World，
以及非抽象、非废弃的具体 ActorClass。

World 与 ActorClass 只由调用边界显式传入；网关不扫描世界、不读取 Pawn/Controller/Input/UI，
也不通过 Tick 或随机数推导投放。ActorClass 的完整路径被写入组合结果，并与世界投放回执交叉核对。

## 4. 有序权威与幂等恢复

唯一入口按以下顺序工作：

1. 完成生命周期、Run、Coordinator、World 与 ActorClass 预检；
2. 若 Host 没有待恢复投放，调用既有 `TryPrepareAnchor()`；
3. 调用既有 `TryCommitPreparedAnchor()`，提交已有材料事务；
4. 调用既有 `TryPlaceCommittedAnchor()`，创建或回放世界锚点；
5. 返回三个原始 Host 结果和最终 `Placed` / `Replayed` 状态。

Host 已持有同一操作的 committed pending placement 时，网关跳过 Prepare，继续精确 Commit/Place
恢复；pending 属于其它操作时返回 `PendingPlacementConflict`，不触碰新材料权威。已放置操作以相同
ActorClass 重试时回放原 receipt；ActorClass 漂移由既有世界绑定契约拒绝。

## 5. 生命周期与终止边界

生命周期持有产品 teardown 检查点时，所有新锚点操作统一返回 `TeardownPending`。这保证产品世界
已经清理而 Coordinator 尚未释放的窄窗口只能执行精确 Run 释放重试，不能重新进入材料或世界路径。

Controller 终止现在根据唯一 Session 状态选择：

- `Deploying` / `Cancelled`：调用 `TryCancelAndTeardown()`，保留材料取消语义；
- `Active` / `Ended`：调用 `TryEndAndTeardown()`，保留完整阵图正常结束语义。

终止摘要新增 `bEndedCompletedFormation`，并要求该标志与 Session 的 Ended/Cancelled 回执严格一致。

## 6. 正反测试

生命周期专项扩展为 4 项，其中新增：

- `AnchorOperation.OrderedReplayAndActiveEnd`：用真实物品权威和 World 完成两个锚点；验证一次操作独占
  Prepare/Commit/Place、精确重试不改变物品快照或 Actor 数、ActorClass 漂移被拒绝、第二锚点激活
  阵图、Active 阵图正常 End 并移除两个 Actor 后才释放 Combat Run；
- `AnchorOperation.PreflightAndTeardownFences`：验证无效操作、外部 Run、空 World、空 ActorClass
  均在材料权威前失败；注入 Coordinator 后置释放故障后，产品 teardown 检查点阻断新世界操作，
  修复身份后精确重试复用同一 teardown receipt。

既有 Controller 两项专项同时重跑，覆盖原冻结重放、并发占用和取消终止语义。

## 7. 自动化证明

| Group | Success | Fail | Log SHA-256 |
|---|---:|---:|---|
| `Shanmen.0_0_10.Product.FormationRunLifecycle` | 4 | 0 | `C8901C78...39424E5AE` |
| `Shanmen.0_0_10.Product.FormationProductController` | 2 | 0 | `FAE72AD4...5A69D68B` |
| `Shanmen.0_0_10` | 1,328 | 0 | `522C263C...3DF1F8B6` |
| `demo_map.V3.Attributes` | 4 | 0 | `D88B3CF0...A5042AD` |

四份最终日志均有至少一个 Success、0 Fail、0 Fatal/Unhandled/Ensure、原生终端完成证据，且对应
进程退出码均为 0。

## 8. 改动驱动回归、构建与静态检查

5 个生产/测试改动路径命中 3 条现有映射规则，最终覆盖门：
`REGRESSION_COVERAGE: PASS Changed=5 Rules=3 Required=39 Logs=4`，SHA-256
`B7DB3ED84E2C4AF3FFCB31DE6744C39644C33D9E82DB9A46C7C0EDDF469B1C9B`。

映射器正反自测 `463/463` PASS，SHA-256
`7813A4064A20D651BEF0837D86D692BC8E17864065E2BED18C16D3472BE14EE6`。

| Target | Result / native exit | Duration | Log SHA-256 |
|---|---|---:|---|
| `demo_map` Win64 Development | Succeeded / 0 | 48.61s | `6326D124...E7F594B1` |
| `demo_mapEditor` Win64 Development | Up to date, Succeeded / 0 | 1.69s | `DF20396E...3E71625B` |

最终 `demo_map.exe` 为 360,065,536 bytes，SHA-256
`74022E7EBC90AC839329F7DCBC97FAB30EA4C2B5DDDB8DCF0D246D545263B29A`；
`UnrealEditor-demo_map.dll` 为 19,395,072 bytes，SHA-256
`599A5347702FD8986467688E583D307C7FE39B66A7AD68AD4FB198C8B066F5AF`。

`git diff --check`、regression map JSON 解析通过；Controller/Lifecycle scoped scan 未发现 Actor
创建/删除/扫描、RNG、`ApplyDamage`、输入、Widget 或 Tick 绑定。

## 9. P/F 边界与下一阶段

P 阶段证明了显式操作身份、World/ActorClass 前置失败关闭、既有三段权威的唯一编排、精确重放、
teardown 后新工作隔离、Active 正常结束、完整 0.0.10 回归和改动驱动覆盖。

F 阶段未执行：没有启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、
Cook 或 Package。因此不声明玩家设备输入已经驱动可见阵法。下一阶段应新增设备无关输入采样适配器，
只把已捕获意图和锚点操作提交给本生命周期；不得耦合 GameMode、轮询世界或建立第二套权威。

## 10. GitHub 交接

基线提交：`280f9c10712c49c78306d9bf25d739299b085d32`（P27.2）。
分支：`agent/0.0.10-p27-3-formation-anchor-operation-gateway`。本阶段只提交 5 个实现/测试文件、
本 Report 与本 Development Log；103 个既有未跟踪用户文件保持未暂存，`Saved/Codex/P27.3`
原始证据不进入 Git。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p27-3-formation-anchor-operation-gateway>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-3-formation-anchor-operation-gateway/Docs/Report/Dev.D.UE.0.0.10.P27.3.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-3-formation-anchor-operation-gateway/Docs/Log/Dev.D.UE.0.0.10.P27.3.r0_log.md>
