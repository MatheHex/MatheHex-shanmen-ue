# Dev.D.UE.0.0.10.P12.27.r0 Report

## 1. 结论

P12.27 已完成并通过 P 阶段门禁。

本阶段在 P12.26 caller-owned prepared retry 之上新增纯只读 retry decision：调用方显式提供 deterministic policy seed、最大 renewal 数和已使用计数；服务读取最新 durable Host receipt，复用 P12.26 的唯一 retryability/root validator，并返回一个 immutable `Retry / Stop` 决策值。

最终结果：

- 有合法 retry-pending receipt 且预算可用时返回 `Retry`，只携带一份 P12.26 prepared continuation；
- receipt 可重试但预算耗尽时返回 `StopBudgetExhausted`，不暴露可执行 continuation；
- terminal Host 返回 `StopCompleted`；非 terminal 且最新 durable receipt 不可重试时返回 `StopNotRetryable`；
- 预算耗尽也先经过 P12.26 只读 root/receipt 校验，foreign Host 不会被误分类成普通 Stop；
- 决策不修改 Host、不调用 executor、不递增隐式计数、不执行 retry；
- 同一 prepared root、Host receipt、policy 与 consumed count 得到完全相同的 retry seed/decision；
- consumed count 由调用方显式推进，每一轮得到与最新 source receipt 绑定的新 continuation；
- focused decision `5/5`，EffectCue 父前缀 `83/83`，0.0.10 全量 `649/649`，Fail `0`；
- changed-file gate：`Changed=5 / Rules=1 / Required=28 / Logs=21`；
- regression gate self-test `216/216`；
- `git diff --check`、静态边界扫描、Editor/Game Development 单并发构建全部通过。

## 2. 功能性

### 2.1 Caller-owned policy/request

`Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryDecisionPolicy` 固定：

- caller-owned `PolicySeed`；
- 总 renewal 上限 `MaxRenewals`。

`Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductRetryDecisionRequest` 额外固定 `RenewalsUsed`。合法区间为 `0 <= RenewalsUsed <= MaxRenewals`；服务本身不保存或更新计数。只有 `RenewalsUsed < MaxRenewals` 才可生成下一份 continuation。

### 2.2 Stateless Decide

`Decide` 的顺序为：

1. 校验 request、frozen prepared dispatch 与 bound Host；
2. 读取 Host 最新 durable record；
3. 由 policy、consumed count、prepared plan/projection 与最新 dispatch/command identity 派生 P12.26 retry seed；
4. 恰好调用一次只读 `PrepareRetry`；
5. 根据 preparation status、terminal 状态和预算返回一项决策。

新实现没有 `TryExecutePreparedRetry`、`TryRouteProcessNext` 或 `Host.TryRoute` 调用。测试只有在取得 `Retry` 后，才由测试调用方显式把 continuation 交给 P12.26 执行。

### 2.3 Immutable decision evidence

决策值私有保存：

- frozen prepared dispatch root；
- exact policy/request；
- outcome 与底层 P12.26 preparation status；
- deterministic retry seed；
- 做出判断时的 latest durable Host record、next sequence、record count 与 terminal 位；
- caller 下一次应使用的 `NextRenewalsUsed`；
- 仅 `Retry` outcome 才存在的 prepared continuation。

`IsValid` 重新核验 Host/sequence/root、request budget、preparation status 与 optional continuation。Stop 值必须没有合法 continuation；Retry 值必须绑定 exact observed source envelope、prepared root 与 retry seed。

### 2.4 Stop 分类

- `StopBudgetExhausted`：P12.26 已证明最新 receipt 可重试，但 caller budget 为零；
- `StopCompleted`：prepared root 匹配且 Host 已 terminal；
- `StopNotRetryable`：prepared root 匹配、Host 非 terminal，但最新 durable receipt 不是 retry-pending；
- invalid request、invalid Host、foreign prepared root 与内部 capture/state 错误使用 rejection status，不伪装成业务 Stop。

## 3. 完整性

新增五个 focused Automation contract：

1. `RetryIsDeterministicAndCallerExecuted`：同 receipt/request 产生相同 inert Retry；决策期间 Host/executor 不变；调用方显式执行后完成；
2. `StopAtBudgetWithoutContinuation`：合法 retryable state 在预算耗尽时 Stop，且不泄漏 prepared continuation；
3. `RepeatedCallerBudget`：调用方显式推进 count，两次 retry 绑定不同最新 receipt/seed，第三次在上限处 Stop；
4. `CompletedAndNotRetryableStops`：terminal success 与 durable executor rejection 得到不同 Stop reason；
5. `InvalidAndForeignFences`：空/超限 request、invalid prepared value、empty Host 与 foreign root 全部在副作用前拒绝。

测试沿用真实 `CombatRunCoordinator + fixed timeline + SwordRhythmProductSession + evaluation/presentation/effect-cue + prepared dispatch + transaction/Host` 链。fake executor 仅返回既有 opaque receipt 或显式 rejection，不伪造 decision、Host、retryability、budget 或 replay evidence。

## 4. 权威与兼容性边界

- P12.26 `PrepareRetry` 继续是 retryability、root matching 与 continuation capture 的唯一权威；
- P12.21 CommandHost/Router/Session 继续持有 durable execution state；
- 新层只追加普通 C++ immutable value/stateless service，不修改任何既有 public API；
- policy/count 都由调用方携带，新层不持有 mutable retry episode；
- production 新层无 reflection、World/Actor/UObject、资产、timer、async/thread、RNG、循环、damage、attribute 或 gameplay-effect application；
- production 新层没有 executor 参数，也没有任何执行/路由调用；
- PreparedRetry、PreparedDispatch、ProductSession、ProductHost、GameMode 与 CombatRunCoordinator 未修改；
- 长期未跟踪的 0.0.9B Prompt、Report、CSEMI、PDF 与用户资料未修改、未暂存、未提交。

## 5. 修改范围

- `demo_mapShanmenSwordRhythmEffectCueExecutionProductRetryDecision.h/.cpp`：新增 policy/request、immutable decision、result/status 与 stateless Decide；
- `demo_mapShanmenSwordRhythmEffectCueExecutionProductRetryDecisionTests.cpp`：新增五个预算、重复、Stop 与 fence contract；
- `ShanmenRegressionMap.json`：新增 retry-decision 路径规则与 28 个 required groups；
- `Test-ShanmenRegressionCoverageSelfTest.ps1`：新增完整证据正例与 focused-only 缺证据反例；
- Report/Log 生成前 5 个代码/流程文件净变更 `+1065 / -0`。

## 6. Automation 证据

| Group | Success | Fail | Exit | SHA-256 |
|---|---:|---:|---:|---|
| `ProductRetryDecision` | 5 | 0 | 0 | `B30862590E2EA23C35DF94895C3376328A6F354D29654925326560CC8060D7BC` |
| `ProductPreparedRetry` | 6 | 0 | 0 | `BEB0CBF5FE631BFBF72B9C4C65EA395ED782FD2CAADD955421BBABB765346B77` |
| `ProductPreparedDispatch` | 5 | 0 | 0 | `741B17348C00FCDCB00D58327EAFDBAD3032D7C103487710A13BC6FC422585F5` |
| `ProductPlannedDispatch` | 5 | 0 | 0 | `964D6FC6546E9B844B4431FAF616EDB108BAD6155F3691412B64D0F37E73D35E` |
| `ProductDispatchPlan` | 5 | 0 | 0 | `8260563800CC2F5D297DC8A7868B38F8BA7E0F38E70E0E4852A668BF721C3486` |
| `ProductDispatch` | 10 | 0 | 0 | `0C33EB63B43DE21740E0A7C53D22EAEFEAA612D2C3F732DA36F3AC5CBA2EF88E` |
| `ProductTransaction` | 5 | 0 | 0 | `B6FD1B39BCE5CAC7C67F747BB6CA7BE3C61CD034FA04D4670912961547A3263A` |
| `ProductRoute` | 5 | 0 | 0 | `6F2BEDDAD25E75A004747A89F33339EA28325E40EA385A36BD4B71E3216B4CED` |
| `CommandHost` | 5 | 0 | 0 | `2D4DD907604B1A906F2A3A95690775073694ABA31B0D6B2B3461F93F83E0E550` |
| `CommandRouter` | 5 | 0 | 0 | `21FF4C630EF84AD1F71BEDEE398FF165C235210596598A56508E6069E6C6D81B` |
| `ExecutionSession` | 5 | 0 | 0 | `752056A3CA55F21E28D10A1DE851D22CE0C4B997EA6EEF5DFB27207A7D499D2B` |
| `ExecutionHost` | 5 | 0 | 0 | `D3F4187EED6BA7D99918AE9C2DBEA6243DD11AD73699B54B6D1B2BF1D80CD908` |
| `ExecutionDriver` | 5 | 0 | 0 | `4E34945CEDEF627C1E61BFF5F86D3062C5849D292148428C57DD0172E82CB41B` |
| `ExecutorAdapter` | 5 | 0 | 0 | `F46B50410B314BFAF445C05A7CA4B532A9AEBEC303345143BE007F3CDFF450F6` |
| `ConsumerAttempt` | 5 | 0 | 0 | `10716DDF9969F43F89386FE7D1E62BC5005A44E01A37DF3F431D99C3C3835A1C` |
| `Delivery` | 4 | 0 | 0 | `D9683F7D32703CA99A8E9E0E26AE2D88C2A6329722AE8A60E424729BB4AF161E` |
| `EffectCue` | 83 | 0 | 0 | `692260C84F0C8DD889CC7E26A4E79887C26FBC18A3025FEAB5B4DC19092BDFE9` |
| `Presentation` | 4 | 0 | 0 | `EA3EA017E4F8A099EB5EEA5B60333A88577C115301C5D6A38F431521FE3BB51B` |
| `ProductSession` | 1 | 0 | 0 | `262ACB9ACA1F28E1BE1AAA60E12BCE1A0C587C9860A4165341B0FA7CB35E81EB` |
| `EvaluationRoute` | 1 | 0 | 0 | `C07E5786C49A768DD662854067B9E67C53C221658813CFF24519FB610608E4F9` |
| `Shanmen.0_0_10` | 649 | 0 | 0 | `1B0A9787A0E5E3D62037BCE7B2739F5D895FD60920A19501AB528C644EC49603` |

二十一份最终日志均有唯一 selected `RunTests` group、唯一 terminal marker、Fail `0`；selected phase error/Fatal/Unhandled/Ensure 为 `0`。EffectCue 从 `78` 增至 `83`，全量从 `644` 增至 `649`。

## 7. 静态与回归门禁

```text
REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=28 Logs=21
SELF_TEST: PASS 216/216
DECISION_BOUNDARY: PASS Host mutation=0 Executor invocation=0
EXECUTION_BOUNDARY_SCAN: PASS no execute/route calls in production decision layer
RUNTIME_BOUNDARY_SCAN: PASS
AUTHORITY_SCAN: PASS PreparedRetry PreparedDispatch ProductSession ProductHost GameMode CombatRunCoordinator unchanged
JSON_PARSE: PASS
git diff --check: PASS (native exit 0)
```

## 8. 构建证据

有效命令：`Build.bat <Target> Win64 Development demo_map.uproject -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`。

| Target | Result | Actions / total time | Exit |
|---|---|---|---:|
| Editor initial | Succeeded | 5 / 10.11s | 0 |
| Game final | Succeeded | 4 / 23.63s | 0 |
| Editor final | Succeeded, up to date | 0 / 0.89s | 0 |

最终产物：

- `UnrealEditor-demo_map.dll`：13,818,368 bytes，SHA-256 `F108C39602F2FE72A09BEFE9904F3FD650F5E71EC2245018E12558E296794C5F`；
- `demo_map.exe`：355,343,360 bytes，SHA-256 `696C38087F792C5F6FBBF37C6F0F80D87395E515875B0B783270D88C2D65E29B`。

所有有效构建均原生退出 `0`，未出现源码失败、C3859、C1076、系统代码 1455 或其它 commit-memory/page-file 环境错误。

## 9. 流程与异常

首次 Editor compile 即成功；首次 focused Automation 即 `5 Success / 0 Fail`，原生退出 `0`，SHA `CC80655853DED4B4ACB7811A1C7030EC9D4040891AF87A1E8F5E85AD66FEC212`。本轮没有源码、测试或构建失败。

UE 启动阶段仍有 selected `RunTests` 之前的既有 13 条 automation condition diagnostics；最终 21 份日志 selected phase 均无 error/Fatal/Unhandled/Ensure。宽组运行期间 `google.com/generate_204` 网络探测多次 3 秒超时并触发 `Ignoring very large delta` warning，但测试持续推进且最终原生退出 `0`；该现象未描述为源码或测试失败。跨平台 SDK 探测仍提示 LinuxArm64/VisionOS 缺少 `MainVersion`，Win64 SDK 为 VALID。raw Automation/build 日志仅本地保留，不纳入 Git。

## 10. P/F 边界与下一步

本 Report 只包含 P 阶段 caller-owned retry policy/request、immutable decision evidence、deterministic seed、injected fake executor、静态审查、NullRHI 无头 Automation、changed-file 回归与 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、实际动画/声音/VFX、Cook 或 Package。

建议 P12.28 增加 caller-owned one-shot retry step：一次调用先取 P12.27 决策，只有 `Retry` 才执行恰好一个 P12.26 continuation，并返回下一 count 与 durable execution evidence；Stop 保持完全惰性，仍不引入 loop、scheduler、timer 或后台 ownership。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p12-27-sword-rhythm-cue-retry-decision>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-27-sword-rhythm-cue-retry-decision/Docs/Report/Dev.D.UE.0.0.10.P12.27.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-27-sword-rhythm-cue-retry-decision/Docs/Log/Dev.D.UE.0.0.10.P12.27.r0_log.md>
