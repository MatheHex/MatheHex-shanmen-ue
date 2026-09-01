# Dev.D.UE.0.0.10.P12.29.r0 Report

## 1. 结论

P12.29 已完成并通过 P 阶段门禁。

本阶段在 P12.28 one-shot retry step 外新增 caller-owned immutable command/receipt adapter。command 捕获 exact prepared root、retry policy/consumed count 与捕获时的 exact Host cursor；adapter 每次只把一条 command 交给 P12.28 一次。若该 step 推进 Host，旧 command 立即 stale 并在再次执行前失败关闭，调用方只能使用返回的 `NextRenewalsUsed` 与新 Host cursor 捕获下一条 command。

最终结果：

- command 绑定 caller `CommandId`、完整 prepared dispatch、完整 request，以及 Host id/run/batch/next-sequence/record-count/terminal cursor；
- adapter 生产代码只有一个 P12.28 `TryRunStep` 调用点，无第二套 retry/root/budget/retryability 判定；
- mutating retry 后旧 command 返回 `HostCursorMismatch`，不会再次调用 executor 或修改 Host；
- non-mutating Stop 可以用同一 command 重算，产生相同 deterministic receipt id，且保持 Host/executor 惰性；
- handled、decision rejection 与 durable execution rejection 都能形成结构自校验的 immutable receipt；
- invalid command、invalid/empty Host 与 foreign Host cursor 在 P12.28 前拒绝；
- focused command `5/5`，EffectCue 父前缀 `93/93`，0.0.10 全量 `659/659`，Fail `0`；
- changed-file gate：`Changed=5 / Rules=1 / Required=30 / Logs=23`；
- regression gate self-test：`220/220`；
- 静态边界扫描、`git diff --cached --check`、Editor/Game Development 单并发构建全部通过。

## 2. 功能性

### 2.1 Immutable command capture

`TryCapture` 只接受有效 command id、prepared dispatch、retry request 与已绑定的有效 Host。它冻结：

- exact prepared projection/plan root；
- caller-owned policy seed、maximum renewals 与 current renewals used；
- Host id、run id、batch id；
- Host next sequence、record count 与 terminal 位。

所有字段均为 private 且只读暴露；调用方无法在 capture 后改写 command。capture 还要求 Host `NextSequence == RecordCount > 0`，避免从空或内部不一致的 transport cursor 创建命令。

### 2.2 Exact cursor 防旧计数重放

只把 `RenewalsUsed` 放入 command 不能阻止调用方重复提交旧计数。P12.29 因此把 command 同时绑定到捕获时的 Host cursor。

执行前固定检查顺序为：

1. command 自身有效；
2. Host 有效且已绑定；
3. command 的 Host id/run/batch/sequence/count/terminal 与当前 Host 完全一致；
4. 仅通过以上检查后调用 P12.28 `TryRunStep` 恰好一次。

retry step 一旦写入 durable Host records，旧 command 的 sequence/count 即不再匹配。测试证明旧 command 返回 `HostCursorMismatch`，不会重入 executor；调用方必须用 receipt 的 next count 和新 cursor 捕获第二条 command。

### 2.3 Receipt 与可审计拒绝

receipt 保存 exact command、完整 P12.28 step result 与 deterministic receipt id。receipt id 的 canonical parts 包含 command/policy、prepared plan/projection identity、捕获 cursor、step/decision/execution status、retry seed、next count 与 final Host state。

合法 receipt 分为：

- `RecordedHandled`：Completed、RetryPending 或三个主动 Stop；
- `RecordedDecisionRejected`：P12.28 未形成 decision、未执行 retry、Host final cursor 必须仍等于 command cursor；
- `RecordedExecutionRejected`：已有有效 Retry decision 与 exact prepared execution evidence，且保留 durable rejection progress。

其它不一致状态返回 `StepStateInvalid` 或 `ReceiptInvalid`，不向调用方暴露伪造 receipt。

## 3. 完整性

新增五个 focused Automation contract：

1. `HandledReceiptComplete`：pending Host 上捕获 command，单次执行只重入 Audio 一次并完成，返回 valid handled receipt；
2. `PendingRequiresFreshCommand`：第一条 command 只推进一次；旧 command stale 且惰性；新 command 使用 next count 与新 cursor 后完成；
3. `StopReplayIsDeterministicAndInert`：预算耗尽 Stop 不改 Host/executor；同一 command 重算得到同一 receipt id；
4. `RejectedAttemptsAreReceipted`：foreign prepared root 形成 decision-rejected receipt；executor hard rejection 形成 durable execution-rejected receipt；
5. `CaptureAndCursorFences`：invalid id/prepared/request/empty Host 全部 capture 失败；default command 与 foreign Host cursor 在执行前拒绝。

测试沿用真实 `CombatRunCoordinator + fixed timeline + SwordRhythmProductSession + evaluation/presentation/effect-cue + prepared dispatch + transaction/Host` 链。fake executor 只返回既有 opaque receipt、retryable failure 或显式 rejection，不替代 command、budget、retryability、Host 或 replay 权威。

## 4. 权威与兼容性边界

- P12.28 `TryRunStep` 继续是 one-decision/at-most-one-execution 的唯一组合权威；
- P12.27 `Decide` 继续是 caller budget、latest receipt 与 Retry/Stop 的唯一权威；
- P12.26 prepared retry 继续是 continuation capture/execute 权威；
- P12.21 CommandHost/Router/Session 继续持有 durable execution state；
- P12.29 command 不持有 Host/executor 指针，不保存 episode，不拥有 replay ledger；
- production 新层无 reflection、World/Actor/UObject、资产、timer、scheduler、async/thread、RNG、loop、sleep、queue、damage、attribute 或 gameplay-effect application；
- RetryStep、RetryDecision、PreparedRetry、PreparedDispatch、ProductSession、GameMode 等既有 authority 文件未修改；
- 长期未跟踪的 0.0.9B Prompt、Report、CSEMI、PDF 与用户资料未修改、未暂存、未提交。

## 5. 修改范围

- `demo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommand.h/.cpp`：新增 immutable command、receipt、status/result 与 stateless adapter；
- `demo_mapShanmenSwordRhythmEffectCueExecutionProductRetryStepCommandTests.cpp`：新增五个 completion、fresh cursor、deterministic stop replay、rejection receipt 与 fence contract；
- `ShanmenRegressionMap.json`：新增 command 路径规则与 30 个 required groups；
- `Test-ShanmenRegressionCoverageSelfTest.ps1`：新增完整证据正例与 focused-only 缺证据反例；
- Report/Log 生成前五个代码/流程文件净变更 `+1017 / -0`。

## 6. Automation 证据

| Group | Success | Fail | Exit | SHA-256 |
|---|---:|---:|---:|---|
| `RetryStepCommand` | 5 | 0 | 0 | `6B5076916AE7D465DF60098F6EC1DECF64EA58F40A5CA0263D8895D66B45F549` |
| `RetryStep` | 10 | 0 | 0 | `6B26C6C6F28DC7701FD7E546BB5BD0A86F7823863C40182A6DF3AD2F2F65F8D4` |
| `RetryDecision` | 5 | 0 | 0 | `90268F9F07E0EA7672ACAB54A547D9D9B08C346B8966FF5B905B73C3DCD17196` |
| `PreparedRetry` | 6 | 0 | 0 | `351C50E8B595C11ED1759D166E3BE60F723F0245BD5AC9ED4806984C25C08548` |
| `PreparedDispatch` | 5 | 0 | 0 | `01D4A291EBA0D6B50AEA0036339465302A67D89D4115BE5CE7CA9F3DD24739F8` |
| `PlannedDispatch` | 5 | 0 | 0 | `C918E3DDB17CB7A7D52FAB2F2FF4E02C01144A786D1177E05E7255AB9A190347` |
| `DispatchPlan` | 5 | 0 | 0 | `CCC729657D5F9DB1BAAB8BDA66D7F10E66B16DA515CA1DAD5834047B63E22B05` |
| `Dispatch` | 10 | 0 | 0 | `5CBB3BE8170A9225651FA306B37CFA12E733B877C9767735D86A83E5A882022A` |
| `Transaction` | 5 | 0 | 0 | `571E2DFE47F5F7EC2D5A09010700BDDCBFE11DE023580C23E7E38A7C3C51EB5E` |
| `Route` | 5 | 0 | 0 | `DED3E456E2DA26CE358BED9D921906DC3BBDBE46771496E385E4ED6254012A56` |
| `CommandHost` | 5 | 0 | 0 | `A651A45453EAF1FAFF7F3F289CB3CF887C50BEE19A99C6B9CA57E2FD45F0D4BC` |
| `CommandRouter` | 5 | 0 | 0 | `0AAFC7B694458343E521603F32B08CA22F1EC4E14D6FAB1547F769EDF81169EA` |
| `ExecutionSession` | 5 | 0 | 0 | `B0DF62AFDA33B130922399B007A5828947F470375D884CD6A283E9E389555582` |
| `ExecutionHost` | 5 | 0 | 0 | `D8A620AD0AE38C5A585C5165C57476E50ED237842AD7CE72187AA3718886AA34` |
| `ExecutionDriver` | 5 | 0 | 0 | `5CB45AE04665258070041692AA58FAC45D2268FC2C8C552B34C0053553A1A600` |
| `ExecutorAdapter` | 5 | 0 | 0 | `C5A18FFF0FD50F8BF96E15637CB0F832F54BCCA8697C97A10A2D81A158791CF6` |
| `ConsumerAttempt` | 5 | 0 | 0 | `08A22CE4360A6CFF9FE7AB63435186A9C1635CCC90AD7BBBEC6E9FB798F277D2` |
| `Delivery` | 4 | 0 | 0 | `26BDE2E65C711ABCC8CD59F9AFB9FEB80AF9C6B40705EC52C209C0B67FCE2432` |
| `EffectCue` | 93 | 0 | 0 | `A435338AC2C214219AA7774E38873257DA87C12FA7F91B021A70F48962E57045` |
| `Presentation` | 4 | 0 | 0 | `C264F39ACE5C95A7B7A114C54D832DCFF334174F3B3B7F012E487F6E584FDE0B` |
| `ProductSession` | 1 | 0 | 0 | `3FB9A2E7A9969F572888227C67A5FD658E83FBE82CA126DFEFAD78AC4382B357` |
| `EvaluationRoute` | 1 | 0 | 0 | `452F8732E5ACBAF731C702C4BA6B5A07B75D358ECBC269827408DA70B605BA1D` |
| `Shanmen.0_0_10` | 659 | 0 | 0 | `BBFFDE74E1944D6462679609EB53CEB5C7CE0435E621AA10BB6883DE510EAB61` |

最终 23 份 selected phase 日志都有唯一 `RunTests` group、唯一 terminal marker、Fail `0`、Fatal/Unhandled/Ensure `0`。`RetryStep` 父前缀由 `5` 增加至 `10`，EffectCue 由 `88` 增加至 `93`，全量由 `654` 增加至 `659`。

## 7. 静态与回归门禁

```text
REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=30 Logs=23
SELF_TEST: PASS 220/220
COMMAND_BOUNDARY_SCAN: PASS TryRunStep=1 Loop=0 SchedulerTimerThread=0 DirectHostRoute=0 WorldObject=0
AUTHORITY_SCAN: PASS RetryStep RetryDecision PreparedRetry PreparedDispatch ProductSession GameMode unchanged
JSON_PARSE: PASS
git diff --cached --check: PASS (native exit 0)
```

## 8. 构建证据

有效命令：`Build.bat <Target> Win64 Development demo_map.uproject -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`。

| Target | Result | Actions / total time | Exit |
|---|---|---|---:|
| Editor initial | Succeeded | 5 / 14.83s | 0 |
| Game final | Succeeded | 4 / 20.12s | 0 |
| Editor final | Succeeded, up to date | 0 / 1.02s | 0 |

最终产物：

- `UnrealEditor-demo_map.dll`：13,878,272 bytes，SHA-256 `84A57A7EC6DB2AE8BBB9B5149AAA39A390124C0F456841FF16700210EF1ABBE2`；
- `demo_map.exe`：355,387,904 bytes，SHA-256 `CBB1D517C67ED6E8D5426295CDC58F7E152195BBD1B5A51321502D5D34460B73`。

所有有效构建均原生退出 `0`，未出现源码失败、C3859、C1076、系统代码 1455 或其它 commit-memory/page-file 环境错误。

## 9. 流程与异常

首次 Editor compile 即成功；首次 focused Automation 即 `5 Success / 0 Fail`，原生退出 `0`，SHA `6B5076916AE7D465DF60098F6EC1DECF64EA58F40A5CA0263D8895D66B45F549`。本轮无源码、断言或构建失败。

首次 changed-file gate 正确拒绝一份 `ExecutorAdapter` 证据：该 UE 进程原生退出 `0` 且五条测试均为 Success，但日志在 terminal marker 写入前结束。保留的本地备份为 `Dev.D.UE.0.0.10.P12.29.r0_ExecutorAdapter-backup-2026.09.01-02.13.10.log`，SHA `2B8DDCC109F11D0762AD32E4C34E6CA0903803CEA0A525E95FCC5E9873EDA93C`。单独重跑后得到健康 `5/5` 日志。

第二次门禁调用因使用宽泛 `*.log` 收集而把上述备份也纳入输入，再次按预期失败；改为显式列出 23 个最终日志后通过。这两次都是证据生成/选择问题，不是产品源码或测试失败，且未触发重复开发或重复构建。

UE selected phase 前仍有 13 条既有 automation condition diagnostics。宽组运行期间 `google.com/generate_204` 网络探测偶发 3 秒超时与 large-delta warning，但测试持续推进并最终原生退出 `0`。LinuxArm64/VisionOS SDK 提示不影响 Win64。raw Automation/build 日志仅本地保留，不纳入 Git。

## 10. P/F 边界与下一步

本 Report 只包含 P 阶段 caller-owned retry-step command/receipt、injected fake executor、静态审查、NullRHI 无头 Automation、changed-file 回归与 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、实际动画/声音/VFX、Cook 或 Package。

建议 P12.30 增加 caller-owned retry-command journal/host：仅按 `CommandId + exact immutable command` 保存已形成的 receipt，使 exact replay 返回原 receipt、同 id 不同 command 冲突拒绝；每次新 command 仍只调用 P12.29 一次，不引入 retry loop、timer、scheduler、后台任务或第二套 execution authority。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p12-29-sword-rhythm-cue-retry-command>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-29-sword-rhythm-cue-retry-command/Docs/Report/Dev.D.UE.0.0.10.P12.29.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-29-sword-rhythm-cue-retry-command/Docs/Log/Dev.D.UE.0.0.10.P12.29.r0_log.md>
