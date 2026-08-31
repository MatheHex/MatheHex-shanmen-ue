# Dev.D.UE.0.0.10.P12.14.r0 Report

## 1. 结论

P12.14 已完成并通过 P 阶段门禁。

本阶段在 P12.13 consumer attempt coordinator 之上增加 caller-driven、asset-free executor port。完整的 nonempty consumer route 作为一个不可拆分 batch 交给注入 executor；executor 返回与 invocation 绑定的 opaque receipt，适配器校验证据后才向 coordinator 提交 success 或 retry。既有 attempt 的精确重放完全来自 coordinator evidence，不会再次调用 executor；零 command route 直接形成显式 no-op acknowledgement，不构造 invocation，也不调用 executor。

最终结果：

- nonempty route 的完整 typed command batch 一次性交给注入 executor，不允许 partial command receipt；
- invocation、opaque executor receipt 与 attempt submission 均有 deterministic、self-validating identity；
- `RetryableFailure` 记录后保持 route pending，新的合法 attempt 可再次调用 executor；
- exact `AttemptId` replay 从已存 evidence 重建结果，executor invocation count 不增加；
- 零 command route 只走 `NoOpSucceeded` / `NoOpReplayed`，executor invocation count 保持 0；
- invalid、cross-Run、stale、already-acknowledged、different pending route 与 mismatched executor receipt 均在提交前 fail closed；
- focused executor adapter `5/5`，EffectCue 父前缀 `17/17`，0.0.10 全量 `583/583`，Fail `0`；
- changed-file gate：`Changed=7 / Rules=2 / Required=15 / Logs=8`；
- regression gate self-test `190/190`；
- `git diff --check`、静态边界扫描、Editor/Game Development 单并发构建全部通过。

## 2. 功能性

### 2.1 Immutable batch invocation

`Fdemo_mapShanmenSwordRhythmEffectCueExecutorInvocation` 冻结完整 P12.13 consumer route 与 caller-owned `AttemptId`。`InvocationId` 由 route identity、scope、attempt identity、command count 及所有 command identity 确定性派生。只有 command count 大于 0、channel 一致且 route/attempt 完整有效时才能建立 invocation。

执行器通过 `Idemo_mapShanmenSwordRhythmEffectCueExecutor` 注入。该边界只接收完整 invocation 并返回一份 batch receipt，不拥有 coordinator、delivery cursor、ProductSession 或全局历史。

### 2.2 Opaque executor evidence

`Fdemo_mapShanmenSwordRhythmEffectCueExecutorReceipt` 冻结完整 invocation、executor-owned opaque receipt identity 与 `RetryableFailure` / `Succeeded` outcome。其 `ReceiptId` 可由全部输入重建；空 receipt、非法 outcome、换 route、换 attempt 或换 command batch 均不能匹配预期 invocation。

适配器只在 executor result 为 `Completed` 且 receipt 自校验并匹配 invocation 后，构造 P12.13 attempt command。executor 拒绝、无效 receipt 或错配 receipt 不会进入 coordinator。

### 2.3 Replay、retry 与 no-op

Coordinator 新增只读 `TryGetAttemptReceipt(Route, AttemptId, OutReceipt)` 查询。适配器先查精确 attempt：

- 已有 retry/success attempt 会从 coordinator evidence 重建 executor result 并调用原 coordinator replay 路径，不再次触碰 executor；
- 已有 no-op attempt 直接重放 no-op acknowledgement，不构造 invocation；
- 未见 attempt 且当前 route pending 时，只允许同 route 的新 attempt；different route 在 executor 前返回 `RoutePending`；
- route stale、已 acknowledged 或不再 current 时，`Prepare` preflight 在 executor 前拒绝；
- 零 command route 直接提交 P12.13 `NoOpSucceeded`，不会制造虚假的播放证据。

## 3. 完整性

新增五个 focused Automation contract：

1. `BatchSuccess`：真实 nonempty route 的完整 command batch、单次 executor invocation、opaque success receipt 与 acknowledgement；
2. `RetryReplayAndSuccess`：retry 保持 pending、exact retry replay 不 reinvoke、new attempt success、exact success replay 不 reinvoke；
3. `NoOp`：真实零 command route 不构造 invocation、不调用 executor，并可精确重放；
4. `EvidenceFence`：executor reject、receipt mismatch、invalid attempt 与 cross-scope route 均 fail closed；
5. `PendingAndTeardown`：different pending route 在 executor 前拒绝，旧 route 完成后顺序推进；source ProductSession teardown 后 immutable route/coordinator/executor contract 仍有效。

测试使用真实 `CombatRunCoordinator + fixed timeline + SwordRhythmProductSession + effect-cue adapter + delivery cursor + consumer attempt coordinator`。fake executor 仅替代本阶段明确注入的外部执行边界，不伪造 source event、delivery、route 或 acknowledgement。

## 4. 权威与兼容性边界

- ProductSession、GameMode、P12.11 cue adapter 与 P12.12 delivery cursor 均未修改；
- executor adapter 是 stateless 普通 C++ bridge，不是 UObject、Subsystem、World service、global registry 或第二 Session；
- executor receipt 保持 opaque，本阶段不解释资产、组件、动画、音频或 VFX API；
- immutable USTRUCT 输出字段全部 private + `BlueprintReadOnly`，无 `BlueprintReadWrite` / `EditAnywhere`；
- production 文件无 `GetWorld`、spawn、`NewObject`、Niagara、Sound、timer、RNG、damage、attribute 或 gameplay-effect application；
- 适配器不轮询、不创建线程、不持有 executor，不把执行状态写入 ProductSession；
- 长期未跟踪的 0.0.9B Prompt、Report、CSEMI、PDF 与用户资料未修改、未暂存、未提交。

## 5. 修改范围

- `demo_mapShanmenSwordRhythmEffectCueExecutorAdapter.h/.cpp`：immutable invocation/receipt、注入 executor 接口、typed execution result 与 stateless adapter；
- `demo_mapShanmenSwordRhythmEffectCueExecutorAdapterTests.cpp`：五个真实 source-route executor contract；
- `demo_mapShanmenSwordRhythmEffectCueConsumerAttempt.h/.cpp`：增加当前 route 的精确 attempt receipt 只读查询；
- `ShanmenRegressionMap.json`：新 executor adapter source 映射到 15 组 source/product/runtime evidence；
- `Test-ShanmenRegressionCoverageSelfTest.ps1`：新增正例与 focused-only 反例；
- Report/Log 生成前 7 个代码/流程文件净变更 `+1208 / -0`。

## 6. Automation 证据

| Group | Success | Fail | Native exit | SHA-256 |
|---|---:|---:|---:|---|
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutorAdapter` | 5 | 0 | 0 | `8852DB75CE478A66D1441AB8D60D4DE165246E4ACAFC9F46BC32440AB6977730` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueConsumerAttempt` | 5 | 0 | 0 | `3E44809C30B3A705EFCAF4425E015BD6D547C27F48F036475B1C74866FB64410` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueDelivery` | 4 | 0 | 0 | `D1733852E25E438A26112C3C4FE9650AE1FC1DBF50BFF1FAC9BDC9336F33BC49` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCue` | 17 | 0 | 0 | `092AB55A4DF0E842C62A4942AE50FFAC0201D3D1EF2A9D21E82989C6E5D69706` |
| `Shanmen.0_0_10.Product.SwordRhythmProductSession` | 1 | 0 | 0 | `1903A90531C9C8EB4210743F8B1D6C3BC9A99319AD839C7CFB6E0E99637E6970` |
| `Shanmen.0_0_10.Product.SwordRhythmPresentation` | 4 | 0 | 0 | `3098639723E1E10BFAF2E9DF7F9CF7559FCE08A75945DE8A567F23BFFE48D1D9` |
| `Shanmen.0_0_10.Product.SwordRhythmEvaluationRoute` | 1 | 0 | 0 | `FCF0EEF9C91F23C10CDF64F9F9AA1BF2A88166B909F8DFE3B32D0E8322602AF1` |
| `Shanmen.0_0_10` | 583 | 0 | 0 | `FBE493363FE10A5A5CC05D0E679866322CE2A482445EF71D32B10E160ACB7829` |

EffectCue 父前缀包含 P12.11 至 P12.14 的唯一 contract，因此为 `17/17`。全量由 `578` 增加五个唯一 focused contract，唯一用例为 `583`。

八份最终日志均有唯一 `RunTests` group、native terminal marker、Fail `0`；最后一个选定命令之后 Fatal/Unhandled/Ensure/condition error 均为 `0`。

## 7. 静态与回归门禁

```text
REGRESSION_COVERAGE: PASS Changed=7 Rules=2 Required=15 Logs=8
SELF_TEST: PASS 190/190
IDENTITY_REBUILD_SCAN: PASS invocation/receipt deterministically self-validating
PRESENTATION_BOUNDARY_SCAN: PASS no assets/playback/World/Actor/gameplay application
IMMUTABILITY_SCAN: PASS no mutable Blueprint output fields
AUTHORITY_SCAN: PASS ProductSession and GameMode unchanged
TRAILING_WHITESPACE_SCAN: PASS
git diff --check: PASS (native exit 0)
```

## 8. 构建证据

命令：`Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`。

| Target | Result | Actions / total time | Exit |
|---|---|---|---:|
| Editor initial | Succeeded | 8 / 50.27s | 0 |
| Game final | Succeeded | 7 / 39.56s | 0 |
| Editor final | Succeeded, up to date | 0 / 0.96s | 0 |

最终产物：

- `UnrealEditor-demo_map.dll`：13,324,288 bytes，SHA-256 `FA1E0DC0E9190A9A86F5258ABEEE27E47B3182900E0E05C0761ADFA558C9BB70`；
- `demo_map.exe`：354,944,000 bytes，SHA-256 `4069529DC67E06EDB537E8B38C9DFBD4841AC8906E5DFE3F08D1774FB067FDF3`。

构建未出现源码失败、C3859、C1076、系统代码 1455 或其它 commit-memory/page-file 环境错误。

## 9. 流程与异常

本阶段没有 focused、回归、自检或构建首次失败；所有最终命令原生退出码均为 `0`。UE 启动阶段仍有选定 `RunTests` 之前的既有 13 条 automation condition diagnostics；选定阶段内 condition/Fatal/Unhandled/Ensure 为 0，未把启动噪声描述为本阶段失败或成功证据。

raw Automation 与 build 日志仅作为本地可复核证据，不纳入 Git；Git 提交本 Report/Log 与精确源码/流程文件。

## 10. P/F 边界与下一步

本 Report 只包含 P 阶段 executor port contract、纯 value evidence、注入 fake executor、静态审查、无头 Automation、changed-file 回归与 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、实际动画/声音/VFX、Cook 或 Package。

建议 P12.15：增加 caller-driven consumer execution driver，由调用方显式提供 observation、consumer coordinator 与 role-owned executor，并只推进单次 prepare/execute 事务；继续保持无 timer、无 World、无资产和无全局 registry。该层完成后，F 阶段只需替换 executor 实现即可接入真实播放，而不改动 deterministic route/attempt/receipt 契约。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p12-14-sword-rhythm-cue-executor-port>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-14-sword-rhythm-cue-executor-port/Docs/Report/Dev.D.UE.0.0.10.P12.14.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-14-sword-rhythm-cue-executor-port/Docs/Log/Dev.D.UE.0.0.10.P12.14.r0_log.md>
