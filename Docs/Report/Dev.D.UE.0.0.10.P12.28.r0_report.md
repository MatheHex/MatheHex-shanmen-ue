# Dev.D.UE.0.0.10.P12.28.r0 Report

## 1. 结论

P12.28 已完成并通过 P 阶段门禁。

本阶段在 P12.27 caller-owned retry decision 与 P12.26 prepared retry 之上新增 one-shot retry step：每次调用恰好做一次只读决策；仅当结果为 `Retry` 时，才把该决策携带的唯一 continuation 交给 P12.26 执行一次。服务不循环、不调度、不等待、不保存 Host/executor，也不持有 retry episode 或隐式计数器。

最终结果：

- `Retry` 决策最多触发一次 `TryExecutePreparedRetry`；
- `StopBudgetExhausted / StopCompleted / StopNotRetryable` 完全惰性，不调用 executor、不修改 Host；
- retry 完成映射为 `Completed`，仍可重试映射为 `RetryPending`；
- `RetryPending` 返回显式 `NextRenewalsUsed`，只有调用方再次提交 request 才会推进；
- executor hard rejection 保留已消耗 renewal 与 durable execution evidence，不伪装成正常完成；下一次显式 step 读取最新 receipt 后返回 `StoppedNotRetryable`；
- invalid request、invalid prepared value、empty Host 与 foreign root 全部在执行前返回 `DecisionRejected`；
- focused step `5/5`，EffectCue 父前缀 `88/88`，0.0.10 全量 `654/654`，Fail `0`；
- changed-file gate：`Changed=5 / Rules=1 / Required=29 / Logs=22`；
- regression gate self-test：`218/218`；
- 静态边界扫描、`git diff --check`、Editor/Game Development 单并发构建全部通过。

## 2. 功能性

### 2.1 单次组合边界

`TryRunStep` 的固定顺序为：

1. 调用 P12.27 `Decide` 恰好一次；
2. 决策拒绝时返回 `DecisionRejected`，保留诊断与只读 Host 终态；
3. Stop 决策直接映射为对应 `Stopped*` 状态，不创建或执行 continuation；
4. Retry 决策把唯一 prepared continuation 交给 P12.26 `TryExecutePreparedRetry` 恰好一次；
5. 返回 decision、execution、next count 与最终 Host sequence/record/terminal evidence。

生产层只有一个 `Decide` 调用点和一个 `TryExecutePreparedRetry` 调用点，没有直接 `Host.TryRoute*` 调用，也没有第二次隐式决策。

### 2.2 显式 caller continuation

`RetryPending` 不会在当前调用中继续尝试。`CanRequestAnotherStep` 仅在以下条件全部成立时为真：

- 当前状态为合法 `RetryPending`；
- decision 与 execution/Host durable evidence 一致；
- `NextRenewalsUsed < MaxRenewals`。

调用方必须使用返回的 `NextRenewalsUsed` 构造下一次 request。测试证明第一次 step 只增加一次 Audio invocation；只有第二次显式调用才再次执行，并可完成 Host。

### 2.3 证据与状态分类

结果同时保存：

- 完整 P12.27 decision result；
- 完整 P12.26 execution result；
- 下一 caller count；
- 最终 Host next sequence、record count 与 terminal 位。

`IsHandled` 只接受结构一致的 `Completed`、`RetryPending` 和三个主动 Stop。`ExecutionRejected` 保留 exact prepared continuation、底层 rejection 与 durable Host progress，但不伪装成已正常处理；调用方可以用更新后的计数发起下一次 read/decide step。

## 3. 完整性

新增五个 focused Automation contract：

1. `CompleteOneShot`：初始 Audio retry-pending 后改为 success；一次 step 只重入 Audio 一次并完成，Visual 不重入；
2. `PendingRequiresCallerNextStep`：第一次 step 仍 pending 且只执行一次；第二次 caller request 才继续并完成；
3. `StopPathsAreInert`：预算耗尽、terminal 和 durable non-retryable 三条 Stop 路径均保持 Host/executor 不变；
4. `HardRejectionProducesDurableStop`：retry executor hard rejection 产生一条 durable record 并消耗一次 renewal；下一 step 惰性返回 non-retryable Stop；
5. `InvalidAndForeignFences`：invalid/over-budget request、invalid prepared、empty Host 与 foreign root 全部在副作用前拒绝。

测试沿用真实 `CombatRunCoordinator + fixed timeline + SwordRhythmProductSession + evaluation/presentation/effect-cue + prepared dispatch + transaction/Host` 链。fake executor 仅返回既有 opaque receipt、retryable failure 或显式 rejection，不替代 decision、budget、Host、retryability 或 replay 权威。

## 4. 权威与兼容性边界

- P12.27 `Decide` 继续是 caller budget、latest receipt 与 Retry/Stop 的唯一权威；
- P12.26 `PrepareRetry/TryExecutePreparedRetry` 继续是 continuation capture 与单次执行的唯一权威；
- P12.21 CommandHost/Router/Session 继续持有 durable execution state；
- 新层只追加普通 C++ result/stateless service，不修改任何既有 public API；
- production 新层无 reflection、World/Actor/UObject、资产、timer、async/thread、RNG、loop、sleep、queue、damage、attribute 或 gameplay-effect application；
- PreparedRetry、PreparedDispatch、ProductSession、ProductHost、GameMode 与 CombatRunCoordinator 均未修改；
- 长期未跟踪的 0.0.9B Prompt、Report、CSEMI、PDF 与用户资料未修改、未暂存、未提交。

## 5. 修改范围

- `demo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStep.h/.cpp`：新增 step status/result、证据验证与 stateless one-shot service；
- `demo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepTests.cpp`：新增五个完成、显式续步、Stop、hard rejection 与 fence contract；
- `ShanmenRegressionMap.json`：新增 retry-step 路径规则与 29 个 required groups；
- `Test-ShanmenRegressionCoverageSelfTest.ps1`：新增完整证据正例与 focused-only 缺证据反例；
- Report/Log 生成前五个代码/流程文件净变更 `+857 / -0`。

## 6. Automation 证据

| Group | Success | Fail | Exit | SHA-256 |
|---|---:|---:|---:|---|
| `ProductRetryStep` | 5 | 0 | 0 | `84212A31ED422D89FB9E9CCAA116AEE1A3EA6DE6F11B7587BF1DA382A8303918` |
| `ProductRetryDecision` | 5 | 0 | 0 | `99236C7E748D019DBFB8632C9CEFAE4006546ECC2386EB893A1F28E1D1689BE1` |
| `ProductPreparedRetry` | 6 | 0 | 0 | `6C293E1DE0451A3C117C9BCE2C84E4F15DBD3ECD86F8A090C0A6B055777F88BE` |
| `ProductPreparedDispatch` | 5 | 0 | 0 | `55F549F6387483B93103FC9C2812A74BDDF4E5D75B32BFC70A1F754FCD9EC0C6` |
| `ProductPlannedDispatch` | 5 | 0 | 0 | `845D18D76E6B654498704F4596D96E18D95A1B6B8659D0410915333CE690FB5B` |
| `ProductDispatchPlan` | 5 | 0 | 0 | `E11188A22033885E54D642BF242B18AF5423C47E0921B5BF5AF5588C88DC146D` |
| `ProductDispatch` | 10 | 0 | 0 | `50263EA659EA6AE34D027CE1541B3ADA0495F35B0E47D0CABCA491E367815DCE` |
| `ProductTransaction` | 5 | 0 | 0 | `B297C946E8F18F9E040A3739CF57157B1CDDA6F2AF7C570A9018A77C07D8FA5F` |
| `ProductRoute` | 5 | 0 | 0 | `701752DB5DEBEE13EE32CBF8CD4BECDFB1EE2ED401A56AEFECC449DF6ABAED8F` |
| `CommandHost` | 5 | 0 | 0 | `85DAEA9C2FDD446343233DC72674CC5406EC03268A470771B5C62E1BB0CBFA3E` |
| `CommandRouter` | 5 | 0 | 0 | `26B79F893B1241573F2776571B4A9FE8AA9A4833168552BC1CD5A3C66CDFBA0B` |
| `ExecutionSession` | 5 | 0 | 0 | `CF8B22BDC8FD7FE40A3A520AD7C5C4B62AE000B598B31FE50D23B6D42AB42E5C` |
| `ExecutionHost` | 5 | 0 | 0 | `542432F73BC33B0FAC2784E7CBBA75D7E41FCE6ADF044A3F1F4E3446F0C09866` |
| `ExecutionDriver` | 5 | 0 | 0 | `BD1B2A7E75D26C3C2010CC4404059AE9C7D867DA384770CCDE42978574CBC530` |
| `ExecutorAdapter` | 5 | 0 | 0 | `FA2D0D90D772614F1B4783E9EE0EA50C15AD342188E88C421403BA93AE1D98E3` |
| `ConsumerAttempt` | 5 | 0 | 0 | `C3FECED16807217B0E763F80F056A59E9209807D072B9618FD2F6F501B46497C` |
| `Delivery` | 4 | 0 | 0 | `9344E6E6767C713C63F8D4F508A6A2B40EC99BD3F87973C74608DA07B0A26EA0` |
| `EffectCue` | 88 | 0 | 0 | `C021F22730E7B854459D862F956D3661E32D1200A1A2A9D1231579E9B62B4191` |
| `Presentation` | 4 | 0 | 0 | `1F94B968AF35A9D2DFC6A68A06DC53F2BF7E5FB88CE926EDB3B9D53F76FA1E72` |
| `ProductSession` | 1 | 0 | 0 | `28BB3716849806C1B4C952E1B81D5DA7634F11D7A3ADBBAB3A69ADCC67BD19D5` |
| `EvaluationRoute` | 1 | 0 | 0 | `633195DD9A6CAF7F0414D2EE3878599F0615AB5823177BE9CFEE5FD7978CECEB` |
| `Shanmen.0_0_10` | 654 | 0 | 0 | `04DC29DF33F6DCE63204CA6FA15BD58805B2FAB078684AC2A1E5601E66CCF517` |

二十二份最终日志均有唯一 selected `RunTests` group、唯一 terminal marker、Fail `0`；selected phase error/Fatal/Unhandled/Ensure 为 `0`。EffectCue 从 `83` 增至 `88`，全量从 `649` 增至 `654`。

## 7. 静态与回归门禁

```text
REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=29 Logs=22
SELF_TEST: PASS 218/218
STEP_BOUNDARY_SCAN: PASS Decide=1 ExecutePreparedRetry=1 Loop=0 SchedulerTimerThread=0 DirectHostRoute=0
AUTHORITY_SCAN: PASS PreparedRetry PreparedDispatch ProductSession ProductHost GameMode CombatRunCoordinator unchanged
JSON_PARSE: PASS
git diff --check: PASS (native exit 0)
```

## 8. 构建证据

有效命令：`Build.bat <Target> Win64 Development demo_map.uproject -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`。

| Target | Result | Actions / total time | Exit |
|---|---|---|---:|
| Editor initial | Succeeded | 5 / 29.77s | 0 |
| Game final | Succeeded | 4 / 22.89s | 0 |
| Editor final | Succeeded, up to date | 0 / 0.91s | 0 |

最终产物：

- `UnrealEditor-demo_map.dll`：13,843,968 bytes，SHA-256 `B07575B4ECC67566C864C44D41E4EAB6D50ADE884DAF88B5023DDDBA69C6864A`；
- `demo_map.exe`：355,363,328 bytes，SHA-256 `17527BCC764E0FACBE3E173BD5275E97F332F9C57F5096BF5AB8D780E52C6029`。

所有有效构建均原生退出 `0`，未出现源码失败、C3859、C1076、系统代码 1455 或其它 commit-memory/page-file 环境错误。

## 9. 流程与异常

首次 Editor compile 即成功；首次 focused Automation 即 `5 Success / 0 Fail`，原生退出 `0`，SHA `84212A31ED422D89FB9E9CCAA116AEE1A3EA6DE6F11B7587BF1DA382A8303918`。本轮没有源码、测试或构建失败。

证据汇总时第一条只读 PowerShell 命令因直接把 `foreach` 语句块接到 pipeline 而产生 ParserError；改为先收集数组后成功。该命令未修改项目、日志或产品状态，不属于源码、测试或构建失败。

UE selected phase 前仍有 13 条既有 automation condition diagnostics。宽组运行期间 `google.com/generate_204` 网络探测多次 3 秒超时并触发 large-delta warning，但测试持续推进且最终原生退出 `0`。LinuxArm64/VisionOS SDK 提示不影响 Win64。raw Automation/build 日志仅本地保留，不纳入 Git。

## 10. P/F 边界与下一步

本 Report 只包含 P 阶段 caller-owned one-shot retry step、injected fake executor、静态审查、NullRHI 无头 Automation、changed-file 回归与 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、实际动画/声音/VFX、Cook 或 Package。

建议 P12.29 增加 caller-owned retry-step command/receipt adapter：把外层调用者提供的 exact prepared root、policy 与 consumed count 捕获为一次不可变 command，并返回 P12.28 step result receipt；每次 command 仍只推进一步，不增加 loop、scheduler、timer、后台任务或第三套 Host authority。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p12-28-sword-rhythm-cue-retry-step>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-28-sword-rhythm-cue-retry-step/Docs/Report/Dev.D.UE.0.0.10.P12.28.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-28-sword-rhythm-cue-retry-step/Docs/Log/Dev.D.UE.0.0.10.P12.28.r0_log.md>
