# Dev.D.UE.0.0.10.P12.16.r0 Report

## 1. 结论

P12.16 已完成并通过 P 阶段门禁。

本阶段在 P12.15 caller-driven single-consumer driver 之上增加 Run-local 双消费者 execution host。host 固定拥有 canonical Visual 与 Audio coordinator，调用方分别注入两个 executor 和 attempt identity；每轮严格按 Visual→Audio 顺序处理，但 acknowledgement、retry 与 rejection 状态完全独立。

最终结果：

- 普通双 command event 按 Visual→Audio 各执行一次并取得两个 acknowledgement；
- exact replay 不再次调用任一 executor；
- Audio retry 时 Visual acknowledgement 保留，下一轮只重新调用 Audio executor；
- Visual executor reject 时 Audio 仍可成功，恢复轮只重新调用 Visual executor；
- 零 command event 的两个 channel 均本地完成，executor invocation 始终为 0；
- invalid host/event/attempt、cross-Run 与结束后的 host 均在外部执行前 fail closed；
- source ProductSession teardown 后，已冻结 event 仍可由独立 host 完成；
- focused host `5/5`，EffectCue 父前缀 `27/27`，0.0.10 全量 `593/593`，Fail `0`；
- changed-file gate：`Changed=5 / Rules=1 / Required=17 / Logs=10`；
- regression gate self-test `194/194`；
- staged `git diff --check`、静态边界扫描、Editor/Game Development 单并发构建全部通过。

## 2. 功能性

### 2.1 Run-local 双消费者所有权

`Fdemo_mapShanmenSwordRhythmEffectCueExecutionHost::TryCreate` 要求一个有效 Run identity、两个有效且互异的 consumer identity，并分别构造 canonical `Visual` / `Audio` role scope。host 只拥有两个既有 consumer coordinator，不拥有 executor、event、AttemptId、ProductSession、World 或资产。

`IsValid` 持续验证：

- 两个 coordinator 都有效；
- 两个 scope 属于同一有效 Run；
- consumer identity 与 scope identity 互异；
- role 精确为 canonical Visual 与 Audio。

### 2.2 确定顺序与独立状态

`Process` 在双 attempt preflight 之后，固定先调用 Visual driver，再调用 Audio driver。两个调用沿用 P12.15 的唯一 prepare/execute/acknowledge 事务路径；host 不直接调用 coordinator `Prepare` 或 executor adapter，也不建立第二条 mutation path。

一个 channel 的 retry 或 rejection 不回滚 sibling 的成功 acknowledgement，也不阻止 sibling 在同一轮处理。后续调用由既有 coordinator/driver 证据决定：

- 已成功 channel 的 exact attempt 只返回 replay evidence，不 reinvoke executor；
- 未完成 channel 使用 caller-owned attempt 继续执行；
- 两个 channel 都 acknowledged 后，aggregate result 才是 `Completed`；
- 合法 retry 为 `RetryPending`；任一 driver reject 为 `ConsumerRejected`。

### 2.3 No-op 与生命周期

真实零 command event 仍通过两个既有 driver 建立显式 no-op acknowledgement。即使两个注入 executor 都配置为 reject，也不会被调用；exact replay 与 acknowledged new attempt 同样保持 executor-free。

`TryEnd` 需要匹配当前 Run identity；错误 Run 不改变 host。正确 Run 清空两个 coordinator，之后 `Process` 返回 `HostInvalid`。host 没有自动循环、轮询、timer、线程或后台 retry。

## 3. 完整性

新增五个 focused Automation contract：

1. `DualSuccessReplay`：真实双 command event 验证 Visual→Audio 调用顺序、channel projection、双 acknowledgement 与无 reinvoke replay；
2. `PartialRetry`：Visual 成功、Audio retry，随后只重新调用 Audio；
3. `PartialRejectRecovery`：Visual reject、Audio 成功，随后只重新调用 Visual；
4. `DualNoOp`：两个 channel 的 execute/replay/already-acknowledged 均不调用 executor；
5. `FenceLifecycle`：identity、event、attempt、cross-Run、source teardown、wrong-Run end、matching end 与 post-end fence。

测试使用真实 `CombatRunCoordinator + fixed timeline + SwordRhythmProductSession + effect-cue adapter + delivery/attempt/executor-driver chain`。fake executor 仅替代既有明确注入边界，不伪造 event、route、attempt、acknowledgement 或 host 状态。

## 4. 权威与兼容性边界

- P12.11—P12.15 的 EffectCue、delivery、attempt、executor adapter 与 execution driver 均未修改；
- ProductSession、GameMode、CombatRunCoordinator、fixed timeline 与 runtime action authority 均未修改；
- host 是普通 C++ Run-local composition，不是 UObject、Subsystem、World service、global registry 或后台 worker；
- production 文件无 reflection、mutable Blueprint surface、World/Actor、asset playback、spawn、timer、async/thread、RNG、damage、attribute 或 gameplay-effect application；
- 两个 executor 仅作为同步调用参数，不被 host 保存；
- 长期未跟踪的 0.0.9B Prompt、Report、CSEMI、PDF 与用户资料未修改、未暂存、未提交。

## 5. 修改范围

- `demo_mapShanmenSwordRhythmEffectCueExecutionHost.h/.cpp`：typed aggregate result、Run-local Visual/Audio coordinator owner、确定处理顺序与 guarded lifecycle；
- `demo_mapShanmenSwordRhythmEffectCueExecutionHostTests.cpp`：五个真实 source-to-dual-consumer contract；
- `ShanmenRegressionMap.json`：新 host 映射到 17 组 host/driver/delivery/source/product/runtime evidence；
- `Test-ShanmenRegressionCoverageSelfTest.ps1`：新增 host 正例与 focused-only 反例；
- Report/Log 生成前 5 个代码/流程文件净变更 `+854 / -0`。

## 6. Automation 证据

| Group | Success | Fail | Native exit | SHA-256 |
|---|---:|---:|---:|---|
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionHost` | 5 | 0 | 0 | `3E627071BE6A795052E5D2DA36CCCAC9B603B7015CF71FA1D62768C6FFAF3E92` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionDriver` | 5 | 0 | 0 | `331A338A3D53A8BCA845D1C530846FB1BCAD90C3F849455876BF6A2FB0816074` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutorAdapter` | 5 | 0 | 0 | `A9D57ACA3E5128B951CB14D47FBF0AB2FFA115B439928C4C388200F8C5E43FCA` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueConsumerAttempt` | 5 | 0 | 0 | `23111F1AAA9816B9AF8967446618577496F4844FC431DED639C61F54396FD595` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueDelivery` | 4 | 0 | 0 | `DAE5E1A1A6455A9554A0D8C558AE28CA85F26C1FFCE8D28B5BD2AB9D3D65B56A` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCue` | 27 | 0 | 0 | `A3D3CC6BC068962BCB1A601F0D47674BB24E10A1C91606EDF88D09984BD1E2C1` |
| `Shanmen.0_0_10.Product.SwordRhythmProductSession` | 1 | 0 | 0 | `6C29EB2AD175F66A1DB591701F8BD871BDD2803BC892EF7D112F11F242B7EAFD` |
| `Shanmen.0_0_10.Product.SwordRhythmPresentation` | 4 | 0 | 0 | `0B69D80648E879AF32834F268AFCDAF5307244BFDD678BCDC1138F56CCD10050` |
| `Shanmen.0_0_10.Product.SwordRhythmEvaluationRoute` | 1 | 0 | 0 | `B327C24066269E0BE079B29613F30E47E0EE84EE7FEAAE453586CFCAD2188061` |
| `Shanmen.0_0_10` | 593 | 0 | 0 | `AC8811CE467BE7B6571D6797637EB3908476316293D54ADA3B0FEAA265A84EE9` |

EffectCue 父前缀由 `22` 增加至 `27`；全量由 `588` 增加至 `593`。十份日志均有唯一 `RunTests` group、native terminal marker、Fail `0`；最后一个选定命令之后 condition/Fatal/Unhandled/Ensure 为 `0`。

## 7. 静态与回归门禁

```text
REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=17 Logs=10
SELF_TEST: PASS 194/194
TRANSACTION_SCAN: PASS Visual and Audio use exactly two driver transactions
TRANSACTION_BYPASS_SCAN: PASS
CHANNEL_ORDER_SCAN: PASS Visual line 132 precedes Audio line 134
PRESENTATION_BOUNDARY_SCAN: PASS
VALUE_BOUNDARY_SCAN: PASS
AUTHORITY_SCAN: PASS ProductSession and GameMode unchanged
TRAILING_WHITESPACE_SCAN: PASS
git diff --check: PASS (native exit 0)
```

## 8. 构建证据

命令：`Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`。

| Target | Result | Actions / total time | Exit |
|---|---|---|---:|
| Editor initial | Succeeded | 5 / 28.41s | 0 |
| Game final | Succeeded | 4 / 23.24s | 0 |
| Editor final | Succeeded, up to date | 0 / 0.89s | 0 |

最终产物：

- `UnrealEditor-demo_map.dll`：13,387,776 bytes，SHA-256 `27681DC28CDAF3DB72B8CC187394C9B675461F1FA3F02F001378B92F518C9E94`；
- `demo_map.exe`：354,994,176 bytes，SHA-256 `2F0F2AD8FE8F666CE05CC6F49BD275DF93CAE61A7962E02C5804CE39637BE509`。

构建未出现源码失败、C3859、C1076、系统代码 1455 或其它 commit-memory/page-file 环境错误。

## 9. 流程与异常

focused、回归、自检与三次构建均首次成功，最终原生命令退出码均为 `0`。证据汇总期间有两次只读 PowerShell table 表达式 parser error，以及一次 coverage 参数数组绑定错误；修正命令写法后门禁通过，未修改产品或测试状态，也未重复构建。

UE 启动阶段仍有选定 `RunTests` 之前的既有 13 条 automation condition diagnostics；选定阶段 condition/Fatal/Unhandled/Ensure 为 0，未把启动噪声描述为本阶段失败或成功证据。

raw Automation 与 build 日志仅作为本地可复核证据，不纳入 Git；Git 提交本 Report/Log 与精确源码/流程文件。

## 10. P/F 边界与下一步

本 Report 只包含 P 阶段 caller-driven dual-consumer composition、注入 fake executor、静态审查、无头 Automation、changed-file 回归与 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、实际动画/声音/VFX、Cook 或 Package。

建议 P12.17：增加 caller-owned execution session/router，在不引入 World 或资产 ownership 的前提下，将一组 immutable effect-cue events 顺序交给本 host，并显式暴露 batch progress、partial completion 与 guarded teardown；仍不自动 tick、poll 或 retry。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p12-16-sword-rhythm-cue-execution-host>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-16-sword-rhythm-cue-execution-host/Docs/Report/Dev.D.UE.0.0.10.P12.16.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-16-sword-rhythm-cue-execution-host/Docs/Log/Dev.D.UE.0.0.10.P12.16.r0_log.md>
