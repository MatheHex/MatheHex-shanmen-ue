# Dev.D.UE.0.0.10.P12.17.r0 Report

## 1. 结论

P12.17 已完成并通过 P 阶段门禁。

本阶段在 P12.16 Run-local Visual/Audio execution host 之上增加 caller-owned execution session。session 一次性复制一组同 Run、按 `ObservationRevision` 严格递增的 immutable effect-cue events，固定持有对应双消费者 host 与当前游标；调用方每次显式注入 Visual/Audio executor 和 attempt identity，只有当前 event 的两个 consumer 都 acknowledged 后才推进一格。

最终结果：

- 非空 same-Run 有序 batch 可创建，空 batch、逆序、重复 revision、mixed-Run 与重复 consumer identity 均 fail closed；
- deterministic `BatchId` 绑定 Run、双 consumer identity、event identity 与 revision；同输入可重放，consumer scope 变化会改变 identity；
- `ProcessNext` 只处理当前 event，成功后严格推进一格，最后一格返回 `BatchCompleted`；
- retry/reject 均保持当前游标，成功 sibling acknowledgement 不回滚；
- exact failed-attempt replay 不再次调用 executor，恢复必须由调用方提供 fresh failed-channel attempt；
- 完成 batch 后重复调用返回幂等 `AlreadyCompleted`，不调用 executor；
- 仅 matching Run 且 batch complete 时允许 teardown；错误 Run 或未完成 batch 不改变 session；
- source ProductSession 在 batch capture 后可结束，session 仍依靠自有 event copy 完成；
- focused session `5/5`，EffectCue 父前缀 `32/32`，0.0.10 全量 `598/598`，Fail `0`；
- changed-file gate：`Changed=5 / Rules=1 / Required=18 / Logs=11`；
- regression gate self-test `196/196`；
- staged `git diff --check`、静态边界扫描、Editor/Game Development 单并发构建全部通过。

## 2. 功能性

### 2.1 Immutable ordered batch

`Fdemo_mapShanmenSwordRhythmEffectCueExecutionSession::TryCreate` 要求 batch 非空，每个 event 有效、属于同一有效 Run，并按 `ObservationRevision` 严格递增。session 复制 events，而不是保存 source ProductSession 或外部数组引用；创建后 source 生命周期与数组变化不能改变本 session 的工作集。

`BatchId` 使用固定 namespace，并将 Run、Visual/Audio consumer identity、event 数量及每个 event 的 identity/revision 共同纳入确定性派生。它既可复核同一 batch，又不会把不同 consumer scope 混为同一 execution session。

### 2.2 单游标与双 acknowledgement 门

`ProcessNext` 先验证 session、游标与两个 caller-owned attempt identity，再把当前 event 交给 P12.16 host。session 不直接 prepare、执行或 acknowledge consumer，也不绕开既有 host/driver/adapter 事务链。

只有 host 对当前 event 返回完整双 acknowledgement 时，游标才增加一。中间 event 返回 `EventCompleted`，最后一项返回 `BatchCompleted`。batch 完成后的调用稳定返回 `AlreadyCompleted`，不会触碰 executor 或 host 状态。

### 2.3 Partial completion 与显式恢复

Visual/Audio 状态继续由 host 内两个 canonical coordinator 独立持有：

- retry 返回 `RetryPending`，reject 返回 `EventRejected`，两者都保持当前游标；
- 已成功 sibling acknowledgement 保留；
- exact retry attempt 只重放既有证据，不 reinvoke；
- 调用方必须为失败 channel 提供 fresh attempt，成功 sibling 可继续使用原 attempt；
- 恢复后两个 channel 都 acknowledged，session 才推进到下一 event。

### 2.4 Guarded lifecycle

`TryEnd` 同时要求 session 有效、Run identity 精确匹配且 batch 已完成。错误 Run 或未完成 batch 都保持状态；成功结束会结束 host 并清空 session，之后任何 `ProcessNext` 在 executor 前返回 `SessionInvalid`。

session 不拥有 executor、ProductSession、World、资产、timer、线程或后台任务，也不自动 tick、poll、retry 或 teardown。

## 3. 完整性

新增五个 focused Automation contract：

1. `OrderedBatchCompletion`：三项真实有序 event batch、逐格推进、Visual→Audio trace、最终完成与幂等 replay；
2. `RetryHoldsCursor`：Audio retry 时游标保持、exact attempt 不 reinvoke、fresh Audio attempt 后才推进；
3. `RejectHoldsCursor`：Visual reject、Audio acknowledgement 保留，恢复轮只重新调用 Visual；
4. `BatchValidation`：空、逆序、重复、mixed-Run、consumer alias、deterministic/scoped BatchId 与 invalid attempt fence；
5. `GuardedTeardown`：未完成/错误 Run 拒绝、source ProductSession 先结束仍可完成、matching Run teardown 与 post-end fence。

测试使用真实 `CombatRunCoordinator + fixed timeline + SwordRhythmProductSession + evaluation/presentation/effect-cue + delivery/attempt/executor-driver/dual-host` 链。fake executor 仅替代既有明确注入边界，不伪造 event、cursor、attempt、acknowledgement 或 session 状态。

## 4. 权威与兼容性边界

- P12.11—P12.16 的 EffectCue、delivery、attempt、executor adapter、driver 与 dual-consumer host 均未修改；
- ProductSession、GameMode、CombatRunCoordinator、fixed timeline 与 runtime action authority 均未修改；
- session 是 caller-owned 普通 C++ value owner，不是 UObject、Subsystem、World service、global registry 或后台 worker；
- production 文件无 reflection、mutable Blueprint surface、World/Actor、asset loading/playback、spawn、timer、async/thread、RNG、damage、attribute 或 gameplay-effect application；
- executor 只作为同步 `ProcessNext` 参数，不被 session 保存；
- 长期未跟踪的 0.0.9B Prompt、Report、CSEMI、PDF 与用户资料未修改、未暂存、未提交。

## 5. 修改范围

- `demo_mapShanmenSwordRhythmEffectCueExecutionSession.h/.cpp`：typed session result、immutable ordered batch owner、deterministic BatchId、单游标处理、partial-completion fence 与 guarded teardown；
- `demo_mapShanmenSwordRhythmEffectCueExecutionSessionTests.cpp`：五个真实 source-to-session contract；
- `ShanmenRegressionMap.json`：新 session 映射到 18 组 session/host/driver/source/product/runtime evidence；
- `Test-ShanmenRegressionCoverageSelfTest.ps1`：新增 session 正例与 focused-only 反例；
- Report/Log 生成前 5 个代码/流程文件净变更 `+1095 / -0`。

## 6. Automation 证据

| Group | Success | Fail | Native exit | SHA-256 |
|---|---:|---:|---:|---|
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionSession` | 5 | 0 | 0 | `0607F447AB3130341E461CD08AA6B0B628C308F87E0B346BC311FFB2A6B2DB91` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionHost` | 5 | 0 | 0 | `502B8253682A000E1FEACF786EC51AC011076B048C864F0A5A6DB22940BB5FA8` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionDriver` | 5 | 0 | 0 | `F6E14740F7415106D24E6F4CAE065D4A4AA22EE04F38049A2A44E7995DA2E9B1` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutorAdapter` | 5 | 0 | 0 | `D48C932A6ADD56C720832B3D3DA91ADF1417C826408452185DE6D3A334B47117` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueConsumerAttempt` | 5 | 0 | 0 | `F0D08D9C41FD8CD4EE3648A671E982C3FA2684232A314346E2D6D79560B924B6` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueDelivery` | 4 | 0 | 0 | `271F5F94AE5CC16D355454234917A9CEE27E2E90A26AA99CC7E22234914C3A21` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCue` | 32 | 0 | 0 | `EE02BBD33004D4F5A4F1D293E252707C9E53D9240E86959C5889671CC7D7EF66` |
| `Shanmen.0_0_10.Product.SwordRhythmProductSession` | 1 | 0 | 0 | `74044BBFC073C6CD3E580DD399CB809DC1F8826D4F25AB93871ECF5E51C1FEC1` |
| `Shanmen.0_0_10.Product.SwordRhythmPresentation` | 4 | 0 | 0 | `2AD25DB6A34F39370B13546472FF7418A27C1C05990B300C05EFFAEFC5A635BE` |
| `Shanmen.0_0_10.Product.SwordRhythmEvaluationRoute` | 1 | 0 | 0 | `8523E888BAB3ED346C8DABE12CF4560649EF93FF50E93EB78D12BA78C789175C` |
| `Shanmen.0_0_10` | 598 | 0 | 0 | `AAA351D538E342C01F08AB06AAC213BED8EF1547DFF62B9DF42C29E80ACF3959` |

EffectCue 父前缀由 `27` 增加至 `32`；全量由 `593` 增加至 `598`。十一份日志均有选定 `RunTests` group、native terminal marker、Fail `0`；最后一个选定命令之后 `LogAutomationTest: Error` 为 0。

## 7. 静态与回归门禁

```text
REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=18 Logs=11
SELF_TEST: PASS 196/196
HOST_TRANSACTION_SCAN: PASS exactly one Host.Process call
TRANSACTION_BYPASS_SCAN: PASS
RUNTIME_BOUNDARY_SCAN: PASS World/Actor/assets/timer/async/thread/RNG absent
REFLECTION_SCAN: PASS
AUTHORITY_SCAN: PASS ProductSession and GameMode unchanged
JSON_PARSE: PASS
TRAILING_WHITESPACE_SCAN: PASS
git diff --check: PASS (native exit 0)
```

## 8. 构建证据

命令：`Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`。

| Target | Result | Actions / total time | Exit |
|---|---|---|---:|
| Editor initial | Succeeded | 5 / 19.96s | 0 |
| Game final | Succeeded | 4 / 24.05s | 0 |
| Editor final | Succeeded, up to date | 0 / 0.93s | 0 |

最终产物：

- `UnrealEditor-demo_map.dll`：13,436,928 bytes，SHA-256 `CB36E545FCDF8D89C957066626506A3820263E3ECABE1EEB1AA681B24ED6B64F`；
- `demo_map.exe`：355,033,600 bytes，SHA-256 `93607DA7E935FE0411A3105B4DA3E42B1DCBDACF8196E78F5E962D979B2E5304`。

构建未出现源码失败、C3859、C1076、系统代码 1455 或其它 commit-memory/page-file 环境错误。

## 9. 流程与异常

focused、回归、自检与三次构建均首次成功，最终原生命令退出码均为 `0`。证据汇总阶段有两次只读 PowerShell 命令因 Windows workdir 路径转义失败；改用正斜杠后通过。首版边界扫描把项目类型前缀 `demo_map` 也当作外部依赖，产生 68 个词法假阳性；逐条核验后将门禁收窄为真实 runtime 类型/API token，最终命中 0。以上均未修改产品状态或触发重复构建。

UE 启动阶段仍有选定 `RunTests` 之前的既有 13 条 automation condition diagnostics；选定阶段 error/Fatal/Unhandled/Ensure 为 0，未把启动噪声描述为本阶段失败或成功证据。

raw Automation 与 build 日志仅作为本地可复核证据，不纳入 Git；Git 提交本 Report/Log 与精确源码/流程文件。

## 10. P/F 边界与下一步

本 Report 只包含 P 阶段 caller-owned batch session、注入 fake executor、静态审查、无头 Automation、changed-file 回归与 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、实际动画/声音/VFX、Cook 或 Package。

建议 P12.18：增加 caller-owned execution command router，在不拥有 World、资产或 ProductSession 的前提下，以显式 `Create / ProcessNext / End` command 将外层调用映射到 session，并提供 Run/Batch identity fence 与稳定 receipt；仍不自动 tick、poll、retry 或扩张产品权威。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p12-17-sword-rhythm-cue-execution-session>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-17-sword-rhythm-cue-execution-session/Docs/Report/Dev.D.UE.0.0.10.P12.17.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-17-sword-rhythm-cue-execution-session/Docs/Log/Dev.D.UE.0.0.10.P12.17.r0_log.md>
