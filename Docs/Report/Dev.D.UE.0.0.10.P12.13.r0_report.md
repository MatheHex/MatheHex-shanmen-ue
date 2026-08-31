# Dev.D.UE.0.0.10.P12.13.r0 Report

## 1. 结论

P12.13 已完成并通过 P 阶段门禁。

本阶段在 P12.12 consumer delivery cursor 之上增加 asset-free consumer attempt coordinator。它按 canonical consumer role 只投影本消费者负责的 typed channel commands，接受外部 executor 的 opaque success/retry evidence，并且仅在 success 后推进 delivery acknowledgement。失败不会丢 cue，也不会在 ProductSession 建立全局“已播放”状态。

最终结果：

- `Presentation.Visual.SwordRhythm` 只取得 Visual commands，`Presentation.Audio.SwordRhythm` 只取得 Audio commands；
- unknown role、cross-consumer、cross-Run、cross-route 与 stale route 全部 typed fail closed；
- `RetryableFailure` 只生成 immutable attempt receipt，delivery 保持可重试；
- exact attempt replay identity 稳定；同一 `AttemptId` 更换 executor evidence 返回 `AttemptConflict`；
- 普通 route 只能使用带 opaque executor receipt 的 `Succeeded`；零 command route 只能显式使用 executor-free `NoOpSucceeded`；
- pending 旧 route 会阻止 newer route 越序提交，旧 route 成功后 newer route 才可 acknowledgement；
- focused attempt `5/5`，EffectCue 父前缀 `12/12`，0.0.10 全量 `578/578`，Fail `0`；
- changed-file gate：`Changed=5 / Rules=1 / Required=14 / Logs=7`；
- regression gate self-test `188/188`；
- `git diff --check`、静态边界扫描、Editor/Game Development 单并发构建全部通过。

## 2. 功能性

### 2.1 Canonical consumer route

`Fdemo_mapShanmenSwordRhythmEffectCueConsumerRoute` 是 immutable、self-validating value。它冻结完整 P12.12 delivery、canonical channel 与过滤后的 typed commands：

- Visual role 映射到 `Visual` channel；
- Audio role 映射到 `Audio` channel；
- role、delivery、channel、command count 与每个 command identity 全部进入 deterministic `RouteId`；
- route 自校验时会从完整 source event 重新投影，不能删改、换序或混入 sibling channel command；
- 合法 event 可以形成零 command route，该状态不等于播放成功。

### 2.2 Opaque executor attempt

`Fdemo_mapShanmenSwordRhythmEffectCueAttemptCommand` 冻结 route、attempt identity、executor receipt 与 typed outcome。非空 route 要求有效的 opaque `ExecutorReceiptId`，允许 `RetryableFailure` 或 `Succeeded`；零 command route 只允许 `NoOpSucceeded`，并禁止伪造 executor receipt。

`Fdemo_mapShanmenSwordRhythmEffectCueAttemptReceipt` 保存完整 route 与 command。retry receipt 不含 acknowledgement；success/no-op receipt 必须包含与 route delivery 精确匹配的 P12.12 acknowledgement。每个 receipt 都可独立重建 deterministic identity。

### 2.3 Retry、idempotency 与 ordering

Consumer coordinator 只保留当前 route 的 bounded attempt record：

- retry 被记录但不推进 cursor，同 event 仍可重新 prepare；
- exact retry/success replay 返回原 receipt；
- 同一 AttemptId 的不同 evidence 被拒绝；
- success 是调用 P12.12 `Acknowledge` 的唯一普通路径；
- no-op success 是零 command event 的唯一 commit 路径，不宣称发生了播放；
- pending route 阻止 newer route submission，防止失败 cue 被静默跳过；
- current route 成功后，可接受 strictly newer route，并丢弃旧 route 的本地 attempt history；
- newer route 已推进后，旧 route 返回 `StaleRoute`。

## 3. 完整性

新增五个 focused Automation contract：

1. `RoleProjection`：真实双 channel event、Visual/Audio 单 channel 投影、identity isolation、unknown role rejection；
2. `RetryThenSuccess`：retry pending、exact replay、attempt conflict、成功 acknowledgement 与成功 replay；
3. `NoOp`：真实零 command event、普通 playback evidence rejection、显式 no-op acknowledgement/replay；
4. `PendingAndOrdering`：失败旧 route 阻止 newer route，旧 route 成功后顺序推进，旧 route stale；
5. `FenceAndTeardown`：cross-consumer/cross-route/cross-Run/invalid command fail closed，source ProductSession teardown 后 immutable route/receipt/coordinator 继续有效。

测试使用真实 `CombatRunCoordinator + fixed timeline + SwordRhythmProductSession + effect-cue adapter + delivery cursor`，没有手工伪造 source event 或 presentation state。

## 4. 权威与兼容性边界

- ProductSession、GameMode、P12.11 cue adapter 与 P12.12 delivery cursor 均未修改；
- coordinator 是 consumer-owned 普通 C++ value，不是 UObject、Subsystem、World service、global registry 或第二 Session；
- executor evidence 保持 opaque，本阶段不判断资产、组件或播放 API 的实现；
- immutable USTRUCT 输出字段全部 private + `BlueprintReadOnly`，无 `BlueprintReadWrite` / `EditAnywhere`；
- production 文件无 `GetWorld`、spawn、`NewObject`、Niagara、Sound、timer、RNG、damage、attribute 或 gameplay-effect application；
- 不共享 Visual/Audio cursor，不把 attempt 状态写入 source event 或 ProductSession；
- 长期未跟踪的 0.0.9B Prompt、Report、CSEMI、PDF 与用户资料未修改、未暂存、未提交。

## 5. 修改范围

- `demo_mapShanmenSwordRhythmEffectCueConsumerAttempt.h/.cpp`：consumer route、attempt command/receipt、typed statuses 与 bounded coordinator；
- `demo_mapShanmenSwordRhythmEffectCueConsumerAttemptTests.cpp`：五个真实 source-route contract；
- `ShanmenRegressionMap.json`：新 source 映射到 14 组 source/product/runtime evidence；
- `Test-ShanmenRegressionCoverageSelfTest.ps1`：新增正例与 focused-only 反例；
- Report/Log 生成前 5 个代码/流程文件净变更 `+1564 / -0`。

## 6. Automation 证据

| Group | Success | Fail | Native exit | SHA-256 |
|---|---:|---:|---:|---|
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueConsumerAttempt` | 5 | 0 | 0 | `524338FDD5649086C69AE600F958CC33DBA464953B11ABF9BF3A3A02145BC43A` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueDelivery` | 4 | 0 | 0 | `13EE028577EC167C3D48FCF3F4453F4447E8E05C39E1388B12E798354FF9CAE3` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCue` | 12 | 0 | 0 | `84F168D1C2F8D4560E3F5DBA8272440F08A298F548EFB13368B32E0275D140EF` |
| `Shanmen.0_0_10.Product.SwordRhythmProductSession` | 1 | 0 | 0 | `DDFE0476B3AF8EA5690CCAB498CB5845A40B8CF971CA488CD1F8006445605A7A` |
| `Shanmen.0_0_10.Product.SwordRhythmPresentation` | 4 | 0 | 0 | `1CB0D9B13A74F9C6DCA1DBF99F4E7B2C019B7AADD17023DD826FED8E4E45467B` |
| `Shanmen.0_0_10.Product.SwordRhythmEvaluationRoute` | 1 | 0 | 0 | `5472D175CB0DE5365D9BB0F443AB074EBF3EA92AA93419E305A74ADCD9450F65` |
| `Shanmen.0_0_10` | 578 | 0 | 0 | `38993C25612DBCDF289253083F450EE92565D30F2183FFA6D423496717E7C870` |

EffectCue 父前缀包含 P12.11 的 3 个、P12.12 的 4 个及本阶段 5 个 contract，因此为 `12/12`。全量由 `573` 增加五个唯一 focused contract，唯一用例为 `578`。

七份最终日志均有唯一 `RunTests` group、native terminal marker、Fail `0`；最后一个选定命令之后 Fatal/Unhandled/Ensure/condition error 均为 `0`。

## 7. 静态与回归门禁

```text
REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=14 Logs=7
SELF_TEST: PASS 188/188
IDENTITY_REBUILD_SCAN: PASS route/command/receipt all deterministic
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
| Editor initial | Succeeded | 6 / 26.27s | 0 |
| Editor after test correction | Succeeded | 4 / 5.98s | 0 |
| Game final | Succeeded | 5 / 36.21s | 0 |
| Editor final | Succeeded, up to date | 0 / 0.93s | 0 |

最终产物：

- `UnrealEditor-demo_map.dll`：13,275,136 bytes，SHA-256 `9EABB4491BF1A14FC42226472C099094DB7BE4F1091D48A10C1AD3B977DC1652`；
- `demo_map.exe`：354,906,112 bytes，SHA-256 `27E45FE6DBE447FF6923D7913F21E61BBBE05A2592797A201885BB8D1F208C40`。

构建未出现源码失败、C3859、C1076、系统代码 1455 或其它 commit-memory/page-file 环境错误。

## 9. 流程与异常

首次 focused Automation 得到 `4 Success / 1 Fail`，原生进程退出码仍为 `0`，日志 SHA-256 为 `784C7DDEA5C3A88781C09AA7CEF93A77DF21A4E5392119AEC338DEA0F2B7311B`。失败位于测试夹具：newer observation 是合法零 command event，但用例错误地构造普通 `Succeeded`。实现本身按设计拒绝该伪播放证据。用例改为依据 route command count 选择 `Succeeded` 或 `NoOpSucceeded` 后，focused `5/5`，随后所有回归通过。首次失败日志保留在本地。

UE 启动阶段仍有选定 `RunTests` 之前的既有 13 条 automation condition diagnostics；选定阶段内 condition/Fatal/Unhandled/Ensure 为 0。raw Automation 与 build 日志仅作为本地可复核证据，不纳入 Git；Git 提交本 Report/Log 与精确源码/流程文件。

## 10. P/F 边界与下一步

本 Report 只包含 P 阶段 consumer attempt contract、纯 value coordinator、静态审查、无头 Automation、changed-file 回归与 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、实际动画/声音/VFX、Cook 或 Package。

建议 P12.14：增加 caller-driven、asset-free executor port 与 invocation receipt，把本阶段 route commands 作为完整 batch 交给注入 executor；继续用 fake executor 验证 partial/retry/success 原子语义，仍不接 World、组件或实际资产。这样在进入 F 阶段真实播放前，执行边界已有可替换、可回放的明确契约。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p12-13-sword-rhythm-cue-consumer-attempt>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-13-sword-rhythm-cue-consumer-attempt/Docs/Report/Dev.D.UE.0.0.10.P12.13.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-13-sword-rhythm-cue-consumer-attempt/Docs/Log/Dev.D.UE.0.0.10.P12.13.r0_log.md>
