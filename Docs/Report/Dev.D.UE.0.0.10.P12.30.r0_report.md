# Dev.D.UE.0.0.10.P12.30.r0 Report

## 1. 结论

P12.30 已完成并通过 P 阶段门禁。

本阶段在 P12.29 immutable retry-step command/receipt adapter 外新增 caller-owned command journal。journal 只保存已经形成有效 P12.29 receipt 的 command；同一 `CommandId + exact immutable command` 重放直接返回原 receipt，不读取当前 Host，也不调用 visual/audio executor。同一 `CommandId` 若绑定另一条有效 immutable command，则在 P12.29 前返回 `CommandReplayConflict`。

最终结果：

- journal 以 private `TArray<Record>` 保留确定性插入顺序，以 private `TMap<FGuid, int32>` 提供 CommandId 索引；
- 新 command 恰好调用 P12.29 adapter 一次；journal 不复制 prepared root、budget、retryability 或 execution 判定；
- exact replay 可在原 Host 已推进、已 terminal，甚至传入 empty/foreign Host 时返回原 receipt，且 executor/Host 零副作用；
- 同 ID 不同 immutable command 在 Host/executor 前冲突拒绝；
- P12.29 无 receipt 的 `HostCursorMismatch` 等失败不入 journal，同一 command 之后仍可在正确 Host 上执行并入账；
- handled、decision-rejected 与 durable execution-rejected receipt 均可记录和惰性重放；
- focused journal `5/5`、Command 父前缀 `10/10`、0.0.10 全量 `664/664`，Fail `0`；
- changed-file gate：`Changed=5 / Rules=1 / Required=31 / Logs=3`；
- regression gate self-test：`222/222`；
- 静态边界扫描、`git diff --check`、Editor/Game Development 单并发构建全部通过。

## 2. 功能性

### 2.1 Caller-owned idempotency ledger

每条 journal record 保存：

- 原 P12.29 immutable command；
- 原 P12.29 immutable receipt；
- receipt 对应的原 adapter status。

record 自校验 status 与 receipt 形态：`RecordedHandled` 必须是 handled receipt，`RecordedDecisionRejected` 必须是未执行 retry 的 decision rejection，`RecordedExecutionRejected` 必须是已经尝试 execution 的 durable rejection。

journal 自校验以下不变量：

- record 数量必须与 CommandId 索引数量一致；
- 每条 record/receipt/command id 必须有效；
- CommandId 不得重复；
- map 中每个索引必须精确指向同序号 record。

### 2.2 Exact replay 与冲突

`TryExecute` 的顺序固定为：

1. 校验 journal 内部状态；
2. 校验 incoming immutable command；
3. 若 CommandId 已存在，先比较 exact command；
4. exact match 直接返回存量 receipt；不同 command 返回 `CommandReplayConflict`；
5. 只有新 CommandId 才调用 P12.29 adapter 恰好一次；
6. 只有 adapter 返回有效 receipt 才以 candidate-copy 方式原子追加 journal。

重放分支在任何 Host 有效性检查和 executor 调用之前结束。因此调用方可以在网络重试、重复消息或本地状态已推进后安全获得最初 receipt，不会重复消耗 budget、推进 Host sequence 或触发视听执行器。

### 2.3 无 receipt 不污染幂等账本

adapter 的 `CommandInvalid`、`HostInvalid`、`HostCursorMismatch`、`StepStateInvalid` 或 `ReceiptInvalid` 不形成 journal record。测试用 Host A 捕获 command，先向 foreign Host B 提交并得到 `HostCursorMismatch`，确认 journal 仍为空；随后同一 command 交回 Host A 后成功执行并成为第一条 record。

这一区分保证 transport/caller wiring 错误不会永久占用 CommandId，也不会把“未执行”伪装成可重放业务结果。

## 3. 完整性

新增五个 focused Automation contract：

1. `HandledCommandRecordsAndReplays`：完成型 receipt 入账；使用 empty Host 与新 executors exact replay，证明零外部访问；
2. `PendingRequiresSecondRecordedCommand`：第一 command 产生 RetryPending；exact replay 惰性；使用新 Host cursor 与 next count 捕获第二 command 后完成；
3. `CommandIdConflictFailsClosed`：相同 CommandId、不同 policy command 在 P12.29 前冲突拒绝；
4. `RejectedReceiptsAreReplayable`：decision rejection 与 durable execution rejection 分别入账并可惰性重放；
5. `UnreceiptedFailureIsNotJournaled`：foreign Host failure 不入账，同一 command 可在正确 Host 后续成功。

测试沿用真实 `CombatRunCoordinator + fixed timeline + SwordRhythmProductSession + evaluation/presentation/effect-cue + prepared dispatch + transaction/Host` 链。fake executor 只返回既有 opaque receipt、retryable failure 或显式 rejection，不替代 journal、command、budget、retryability、Host 或 execution authority。

## 4. 权威与兼容性边界

- P12.29 adapter 继续是 immutable command 到 P12.28 的唯一入口；
- P12.28 `TryRunStep` 继续是 one-decision/at-most-one-execution 组合权威；
- P12.27 `Decide` 继续是 caller budget、latest receipt 与 Retry/Stop 权威；
- P12.26 prepared retry 继续是 continuation capture/execute 权威；
- P12.21 CommandHost/Router/Session 继续持有 durable execution state；
- journal 只拥有 idempotent receipt ledger，不拥有 Host/executor 指针、retry episode、timer、scheduler、queue、async/thread 或后台任务；
- production 新层无 reflection、World/Actor/UObject、资产、RNG、damage、attribute 或 gameplay-effect application；
- journal 仅有有限 record/index 一致性遍历，不是 retry loop；
- RetryStepCommand、RetryStep、RetryDecision、PreparedRetry、PreparedDispatch、ProductSession 与 GameMode 等既有 authority 文件未修改；
- 长期未跟踪的 0.0.9B Prompt、Report、CSEMI、PDF 与用户资料未修改、未暂存、未提交。

## 5. 修改范围

- `demo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournal.h/.cpp`：新增 record、journal result/status 与 caller-owned idempotency ledger；
- `demo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandJournalTests.cpp`：新增五个 record/replay/conflict/rejection/fence contract；
- `ShanmenRegressionMap.json`：新增 journal 路径规则与 31 个 required groups；
- `Test-ShanmenRegressionCoverageSelfTest.ps1`：新增完整证据正例与 focused-only 缺证据反例；
- Report/Log 生成前五个代码/流程文件净变更 `+945 / -0`。

## 6. Automation 证据

| Group | Success | Fail | Exit | SHA-256 |
|---|---:|---:|---:|---|
| `RetryStepCommandJournal` | 5 | 0 | 0 | `27D59B884C46466D16976B5EE9FBDAC4BFD9BBCEB42DD4B6ED5F0A7B53BAEE45` |
| `RetryStepCommand` | 10 | 0 | 0 | `36F64A51BD126C519B66A2D7A97071A3B00726FAC7B4566224B93F31782309F0` |
| `Shanmen.0_0_10` | 664 | 0 | 0 | `674F0995854464F36BC5FE946F124371AEC90A5D83AA1CD2E722EBFC28DB7BD7` |

三份 selected-phase 日志都有唯一 `RunTests` group、唯一 terminal marker、Fail `0`、Fatal/Unhandled/Ensure `0`。Command 父前缀由 `5` 增加至 `10`，EffectCue 父前缀由 `93` 增加至 `98`，全量由 `659` 增加至 `664`。

本轮按“改动路径映射 + 最小充分证据”精简重复执行：focused journal 证明新增契约；Command 父组证明与 P12.29 组合；当前 commit 的 broad `Shanmen.0_0_10` 直接覆盖映射要求的全部 31 个组。没有再为同一 full log 已覆盖的 21 个下游组各启动一次重复进程。

## 7. 静态与回归门禁

```text
REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=31 Logs=3
SELF_TEST: PASS 222/222
JOURNAL_BOUNDARY_SCAN: PASS P12_29_CALLS=1 RetryWhile=0 InfiniteFor=0 SchedulerTimerThread=0 Queue=0 WorldObject=0 HostExecutorPointer=0
AUTHORITY_SCAN: PASS RetryStepCommand RetryStep RetryDecision PreparedRetry PreparedDispatch ProductSession GameMode unchanged
JSON_PARSE: PASS
git diff --check: PASS (native exit 0)
```

## 8. 构建证据

有效命令：`Build.bat <Target> Win64 Development demo_map.uproject -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`。

| Target | Result | Actions / total time | Exit |
|---|---|---|---:|
| Editor initial | Succeeded | 5 / 24.72s | 0 |
| Game final | Succeeded | 4 / 38.88s | 0 |
| Editor final | Succeeded, up to date | 0 / 1.01s | 0 |

最终产物：

- `UnrealEditor-demo_map.dll`：13,916,160 bytes，SHA-256 `37471E5DD47ED4FF2CA68BAA4B2F4BF7DFAC0C7BA93E2832029C59A0AB34722A`；
- `demo_map.exe`：355,420,160 bytes，SHA-256 `2646AA3D8CCB63A017EFE9AC89DB53B3D5185B57A51210D1D025AD4A7C90C24B`。

所有有效构建均原生退出 `0`，未出现源码失败、C3859、C1076、系统代码 1455 或其它 commit-memory/page-file 环境错误。

## 9. 流程与异常

首次 Editor compile 即成功；首次 focused Automation 即 `5 Success / 0 Fail`、原生退出 `0`。本轮无源码、断言或构建失败。

最初按上一阶段惯例启动了逐层 23 组嵌套证据批次。`RetryStepCommand` 首组 `10/10`、exit `0` 后，发现下一组会再次重复 journal/command 子测试，并且当前 UE 主线程在真实链测试间出现约 15–85 秒 large-delta。依据已落地的 changed-path 映射和流程精简原则，主动以 Ctrl+C 终止该冗余批次；外层 wrapper 因人工终止返回 `1`，当时未完成的 `RetryStep` 日志不作为证据。随后改为 focused + parent + 当前代码 full 三份最小充分证据并通过 gate。这是证据计划修正，不是产品、测试或构建失败。

UE selected phase 前仍有 13 条既有 automation condition diagnostics。运行中 `google.com/generate_204` 3 秒网络探测超时、EOS 配置更新与 automation large-delta warning 未阻止测试推进；最终三组均完整写入 terminal marker 并原生退出 `0`。LinuxArm64/VisionOS SDK 提示不影响 Win64。raw Automation/build 日志仅本地保留，不纳入 Git。

## 10. P/F 边界与下一步

本 Report 只包含 P 阶段 caller-owned retry-command journal、injected fake executor、静态审查、NullRHI 无头 Automation、changed-file 回归与 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、实际动画/声音/VFX、Cook 或 Package。

建议 P12.31 增加 caller-owned journal snapshot/restore value：以 immutable ordered records 导出可验证 checkpoint，恢复时拒绝重复 CommandId、无效 receipt 或索引不一致；仍不负责文件 IO、后台保存、Host/executor ownership 或自动 retry。这样可把当前进程内幂等性扩展到调用方自行持久化后的恢复边界，而不引入第二套执行权威。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p12-30-sword-rhythm-cue-retry-journal>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-30-sword-rhythm-cue-retry-journal/Docs/Report/Dev.D.UE.0.0.10.P12.30.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-30-sword-rhythm-cue-retry-journal/Docs/Log/Dev.D.UE.0.0.10.P12.30.r0_log.md>
