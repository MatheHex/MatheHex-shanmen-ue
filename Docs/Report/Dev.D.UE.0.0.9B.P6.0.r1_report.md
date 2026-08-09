# Dev.D.UE.0.0.9B.P6.0.r1 Report

## 结论

`READY_FOR_CODE_B_IN_RAID_UI_WITH_F_DEBT`

P6r1 已证明：真实 Code A 产品 Start Run 成功生命周期会在世界激活成功后调用 Code B Run Inventory Bridge；Bridge 的未登记、Prepared 回执中断、恢复及活动 Run 冲突均不会改变 Code A 已成功的 Run 结果。P5 正常局外入口在活动 Code B Run session 存在时返回 `当前 Run 中，返回后再整理`，且不创建 P5 UI Host、不写持久化。

## Prompt 与边界

- 已归档并逐字节核对 [P6.0.r1 Prompt](../Prompt/Dev.D.UE.0.0.9B.P6.0.r1_prompt.md)。SHA-256：`729A0F3CC4844F7EB9220B984A4881CEE9FAA660BE7BA359E570F31C98581955`。
- 仅实施 P6r1。未开始 P7、P8 或 `0.0.9B.F`。
- Code A 仍独占 Start Run、RunId、Runtime、世界激活和结算；Code B 仅在 Code A 世界激活成功后作为单向观察者接收 `(OwnerId, RunInstanceId)`。Bridge 返回值不回写、拒绝或回滚 Code A。
- Code B 仍独占 OwnerId 隔离的 P5 sidecar 与 P6 pending Run session。测试读口和 Prepared-interrupt 钩子均为 `WITH_DEV_AUTOMATION_TESTS` 下的只读/进程内生命周期断言工具，不能由产品调用请求。

## 实现

- `StartPreparedProfileRun()` 仅在 `ActivatePreparedProfileWorld()` 成功之后调用 `ObserveCodeBRunAfterActivation()`；观察者日志记录最终 Bridge 状态，但不改变 `Fdemo_mapProfileSessionBeginResult`。
- 新增 `-P6ProductStartBridge` 最小产品生命周期入口。它从真实 Sect Teleport Start Run CTA 发起，不直接调用 Code A Start Run 或 `NotifySuccessfulRun()`；通过 `PassAutomation` / `FailAutomation` 受控退出。
- 新增七个 Owner 隔离的 P6r1 场景，以及对 Owner、RunId、Prepared/Committed receipt、源/Run snapshot、持久化 revision 和 Code A 激活状态的代码级断言。
- 调整 P5 局外入口的判断顺序：先探测 P6 活动 session，再执行通用活动 Run 拒绝，使存在 P6 session 时的产品反馈确定为 `当前 Run 中，返回后再整理`，并在创建 Repository/Host 前返回。

## 最小验证证据

一次完整 Editor 编译通过：`demo_mapEditor Win64 Development`，Unreal Build Tool 结果 `Succeeded`（182.86 秒）。随后首个产品场景揭示测试前置条件在 CTA 成功后仍要求已退场的 Sect 页面可见；仅修正该测试断言后，进行了 4 action 的增量 Editor 编译，结果同为 `Succeeded`（16.93 秒）。未执行大规模回归、截图巡检或重复全量构建。

所有七例均由 `UnrealEditor-Cmd.exe` 真实产品路径运行，每例 log 均有 `demo_map.CodeB.P6.ProductStartBridge: PASS.` 与 `FPlatformMisc::RequestExitWithStatus(0, 0, ...)`；最终确认无残留 `UnrealEditor` / `UnrealEditor-Cmd` 进程。

| 场景 | 产品路径与结果 | 证据 |
| --- | --- | --- |
| CompleteCarry | Sect CTA → Code A 激活 → Bridge committed；所有已准备物品进入 Run snapshot，仓库专属物品留在源 snapshot。 | `Saved/Logs/P6r1.ProductStart.CompleteCarry.20260806_151656.log` |
| Empty | Sect CTA 成功；空 Profile 创建空的 committed Run session。 | `Saved/Logs/P6r1.ProductStart.Empty.20260806_151831.log` |
| NotEnrolled | Sect CTA 成功；Bridge 为 `NotEnrolled`，没有创建 Code B sidecar。 | `Saved/Logs/P6r1.ProductStart.NotEnrolled.20260806_151847.log` |
| BridgeFailure | Code A 激活成功；Bridge 在 verified Prepared receipt 后返回 `StorageFailure`，P5 源快照未抽取。 | `Saved/Logs/P6r1.ProductStart.BridgeFailure.20260806_151904.log` |
| PreparedRecovery | 首次产品观察者留下 Prepared receipt；清除中断钩子后通过同一 `ObserveCodeBRunAfterActivation()` 再交付，结果 `RecoveredPreparedReceipt`。 | `Saved/Logs/P6r1.ProductStart.PreparedRecovery.20260806_151921.log` |
| ActiveSessionConflict | 新 RunId 遭到 `ActiveSessionConflict`；Code A 仍激活，原 session 与持久化 revision 不变。 | `Saved/Logs/P6r1.ProductStart.ActiveSessionConflict.20260806_151938.log` |
| OutOfRaidLock | 活动 session 下从正常 Sect Teleport 仓库入口触发锁，反馈精确为 `当前 Run 中，返回后再整理`，持久化写入为 0。 | `Saved/Logs/P6r1.ProductStart.OutOfRaidLock.20260806_151955.log` |

最初一次手工预检使用了不在既有自动化边界内的临时根目录，Profile Flow 在任何产品 Start 前正确拒绝该路径；该进程已停止，未计入上述七例证据。最终七例均使用受限 `Saved/Automation/Dev.D.UE.0.0.4.14.r0/P6ProductStartBridge` 子目录。

## F 债务

- 依 Prompt 留待 `0.0.9B.F`：跨模块/全量回归、广泛 UI 截图巡检、长期耐久与更大范围兼容性验证。
- 本任务没有改变 Loot、结算、Actor/World 权威、Code A Runtime 保存或 P5 编辑事务。

[CSEMI:REPORT_SENT] {"task_id":"Dev.D.UE.0.0.9B.P6.0.r1","file":"Dev.D.UE.0.0.9B.P6.0.r1_report.md"}
