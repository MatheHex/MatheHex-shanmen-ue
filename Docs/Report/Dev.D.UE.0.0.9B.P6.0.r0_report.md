# Dev.D.UE.0.0.9B.P6.0.r0 Report

```text
task_id = Dev.D.UE.0.0.9B.P6.0.r0
project_id = Dev.D.UE.0.0.9B
status = READY_FOR_CODE_B_IN_RAID_UI_WITH_F_DEBT
prompt = Docs\Prompt\Dev.D.UE.0.0.9B.P6.0.r0_prompt.md
report = Docs\Report\Dev.D.UE.0.0.9B.P6.0.r0_report.md
```

## 交付结论

P6 已增加一个持久、可恢复、只读待用的 Code B 活动 Run 库存 session。它仅在 `Ademo_mapV3ProgressionManager::StartPreparedProfileRun` 已成功完成 `ActivatePreparedProfileWorld()` 后接收稳定的 `ProfileId + ActiveRunId`；Code A 的 Run、玩家、世界、旧库存、Loot、搜索、结算和 Run Save 都没有被 P6 读取或修改。

P5 仍仅是已接收的实施交接；其完整验证并未被本报告重写为已验收。P1–P6 全量验证和最终验收继续属于 `0.0.9B.F`。

## 数据所有权与 bridge

- P5 保有正式 `OwnerId` 局外 Code B snapshot；Code A Profile 只在首次 P5 迁移时作为只读来源，不形成双写。
- P6 的 `FCodeBRunInventorySession` 是同一 Owner durable document 内的第二个、独立命名的 Run snapshot/receipt。把两份 Code B snapshot 放进同一次 temp-verify-backup-replace 提交，可避免跨文件半扣除。
- `NotifySuccessfulRun(storageRoot, OwnerId, RunInstanceId)` 先只读探测 committed P5 sidecar：不存在时返回 `NotEnrolled`，不创建 Profile、fixture、迁移、Repository 或 session。
- 已接入时先写 verified `Prepared` session receipt；再在单个原子记录中从局外 snapshot 抽取、提交 session。重启遇到 prepared receipt 时只会确定地完成该抽取；不会生成第二个 ItemId。
- 同一 `OwnerId + RunInstanceId` 返回原 committed session；旧 session 未关闭而收到不同 RunId 时返回 `ActiveSessionConflict`，不改变任一 snapshot。
- 抽取集合为基础 6 格、武器、道袍、各饰品、已装备空间戒指、已装备背包以及所有随该已选空间容器移动的实际 `ChildContainerId` 内容。仓库未选物保持在 P5 snapshot。ItemId、父容器、slot、ContainerId 和 ChildContainerId 均原样保留，不复制或重编号。
- P6 不实现回收、死亡、撤离或结算；session 将保持为后续 P7 的只读输入，P8 才能关闭或处理它。

## 实际改动

| 文件 | 改动 |
| --- | --- |
| `Source/demo_map/CodeB/demo_mapCodeBOutOfRaidProfile.h` | 增加 P6 Run session、bridge receipt/status、只读 session probe 与仅供定向测试的 prepared-interrupt seam。 |
| `Source/demo_map/CodeB/demo_mapCodeBOutOfRaidProfile.cpp` | 为 Run snapshot 增加 JSON、语义校验、atomic prepared/commit/recovery、ItemId 原样抽取、空 session、幂等、冲突与 P5 normal-entry lock probe。 |
| `Source/demo_map/CodeB/demo_mapCodeBOutOfRaidProfileTests.cpp` | 增加 `demo_map.CodeB.P6.RunInventoryBridge` 定向 bridge 证据。 |
| `Source/demo_map/demo_mapV3ProgressionManager.cpp` | 仅在 Code A world 已激活成功后投递无阻塞 P6 bridge 通知；P5 normal entry 在 active P6 session 时返回“当前 Run 中，返回后再整理”。 |
| `PROJECT.md`、`PROJECT_INFO_CARD.md` | 更新 P5/P6/P7/P8 所有权边界、P6 状态与 F 验证债务。 |

### Code A 改动理由

`demo_mapV3ProgressionManager.cpp` 仅读取既有 `Fdemo_mapProfileSessionBeginResult` 中已经成功的 ProfileId/ActiveRunId，并忽略 bridge 的成功或失败对 Start Run 结果的影响。它不触及玩家 Actor、地图、旧 Runtime inventory、Loot、搜索、结算、SaveGame 或 Code A 物品写入；因此不改变 Code A 权威。

## 窄范围证据

| Prompt 要求 | 可追溯入口与实际结果 |
| --- | --- |
| 已接入 Profile | `demo_map.CodeB.P6.RunInventoryBridge`：P5 committed sidecar 通过 `NotifySuccessfulRun` 创建 session；7 个随身 ItemId（含戒指 ChildContainerId 内容）进入 session，未选仓库 ItemId 留在局外 snapshot。产品挂接点经代码审阅确认位于成功 `ActivatePreparedProfileWorld()` 之后。 |
| 空 Profile | 同一测试：空 P5 sidecar 创建真实、持久的空 session，不设装备门槛。 |
| 未接入 Profile | 同一测试：无 sidecar 返回 `NotEnrolled`，并断言未创建 P5 primary sidecar。 |
| 幂等与恢复 | 同一测试：重复相同 RunId 返回 `AlreadyCommitted` 且 revision 不变；测试 seam 在 verified Prepared 后中断，重启返回 `RecoveredPreparedReceipt` 且维持 7 个唯一 ItemId。 |
| 活动会话冲突 | 同一测试：旧 session 在位时不同 RunId 返回 `ActiveSessionConflict`，原 RunId 不变。Code A 侧调用位置不读取该状态，因此 Run 不被阻断。 |
| 边界审阅 | P6 新类型与 store 仅依赖 Code B snapshot/P2 layout、文件持久化和两项身份；Manager 的唯一 P6 调用在 Start 成功后。没有加入 Player Actor、Loot、搜索、世界物品、结算、Run Save 或 Code A inventory 写入。 |

执行的唯一目标测试（未运行全量 Code B/P1–P5 回归）：

```text
UnrealEditor-Cmd.exe demo_map.uproject -unattended -nop4 -nullrhi -nosplash -NoSound
  -ExecCmds="Automation RunTests demo_map.CodeB.P6.RunInventoryBridge"
  -TestExit="Automation Test Queue Empty"
```

日志 `Saved/Logs/Dev.D.UE.0.0.9B.P6.0.r0_run_bridge.log` 记录：找到 1 个选中测试、`Test Completed. Result={Success}`、`Automation Test Queue Empty 1 tests performed`。启动外层等待在测试完成后的关闭阶段到期；日志中 `TestExit` 请求退出状态为 0，未重跑测试。

单次目标编译：

```text
Build.bat demo_mapEditor Win64 Development -Project=demo_map.uproject -WaitMutex -NoHotReload -MaxParallelActions=1
Result: Succeeded
```

首次编译仅暴露两个同源 return-type 错误，已以明确 result-return 修正；修正后的上述单次目标编译成功，未执行 Game 构建或宽范围重复构建。

## 留给 0.0.9B.F 的验证债务

- P1–P6 全量自动化与跨阶段回归。
- 正式产品 Start Run 的完整可视／跨分辨率输入矩阵、截图巡检和 Game 构建。
- P5 迁移、P6 生命周期与未来 P7/P8 的端到端真实用户路径。
- 失败注入的更广矩阵、来源 SHA-256 审计和最终产品验收。

本报告没有启动 P7、P8 或 `0.0.9B.F`。
