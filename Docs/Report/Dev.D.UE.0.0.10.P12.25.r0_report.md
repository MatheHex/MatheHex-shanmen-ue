# Dev.D.UE.0.0.10.P12.25.r0 Report

## 1. 结论

P12.25 已完成并通过 P 阶段门禁。

本阶段把 P12.24 的“立即读取 current 并立即执行”拆为一个 caller-owned immutable prepared-dispatch value 与两条无状态入口：`PrepareCurrent` 只读取一次 ProductSession，并冻结同一 projection 与 deterministic plan；`TryDispatchPrepared` 之后只消费该冻结值和调用方提供的 Host/executors，不再读取 live Session。

最终结果：

- prepared value 的字段私有，只暴露 const getter，并自验证 projection 与 plan 必须 deep-match；
- `PrepareCurrent` 的生产实现恰好一次 `CaptureCurrent`、一次 plan `Capture`，不触碰 Host/executor；
- `TryDispatchPrepared` 的签名没有 Session，恰好一次进入既有 frozen `TryDispatchProjection`；
- source revision 推进后，旧 prepared value 明确不再匹配 current Session，但仍能在新 Host 上按原 projection 延迟执行；
- exact prepared replay 不重新进入 executor，foreign prepared identity 不能别名到已绑定 Host；
- durable Create 可由同一 prepared value 恢复；retry-pending 的再次调用只重放持久证据，不隐藏 retry loop 或 End；
- dispatch plan 新增统一 `MatchesRequest`，集中核验 projection、Host、sequence、command、consumer 与 attempt identity；
- P12.24 `TryDispatchCurrent` 已改为 `PrepareCurrent → TryDispatchPrepared` 兼容组合，没有保留第二套直接 composition；
- focused prepared dispatch `5/5`，EffectCue 父前缀 `72/72`，0.0.10 全量 `638/638`，Fail `0`；
- changed-file gate：`Changed=10 / Rules=3 / Required=26 / Logs=19`；
- regression gate self-test `212/212`；
- staged `git diff --check`、静态边界扫描、Editor/Game Development 单并发构建全部通过。

## 2. 功能性

### 2.1 Caller-owned immutable prepared value

`Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatch` 只保存：

- 一份冻结的 Product projection；
- 一份由同一 projection 与 caller seed 派生的 deterministic dispatch plan。

两项字段均为 private；值本身没有 Session、World、Host、executor、registry、timer、thread、sequence allocator 或 retry 状态。`IsValid` 同时要求 projection/plan 有效且 plan 匹配该 projection；`Matches` 提供完整重放等价证明；`MatchesCurrentSession` 只供调用方显式诊断 source 是否已经推进。

### 2.2 PrepareCurrent

`ProductPreparedDispatchService::PrepareCurrent` 固定执行：

1. `ProductProjector::CaptureCurrent(Session)` 一次；
2. `ProductDispatchPlanFactory::Capture(frozen projection, seed)` 一次；
3. 将两份已核验值复制进 caller-owned prepared value。

无有效 current projection 或 seed/plan 无效时，在任何 Host/executor 副作用之前 fail closed。成功结果同时保留 projection capture、plan capture 与 prepared value 三份可交叉核对证据。

### 2.3 TryDispatchPrepared

`TryDispatchPrepared` 不接收 ProductSession，只接收 prepared value 与 caller-owned Host/双 executor。有效值恰好一次进入 P12.22 `TryDispatchProjection`，并保留 Dispatched/Resumed/Replayed 一一映射；其它执行结果透明返回 DispatchIncomplete。

成功结果要求 dispatch 保存的 projection 与 prepared projection deep-match，且 transaction request 必须通过 prepared plan 的 `MatchesRequest`。因此延迟调用、terminal replay 和 durable Create resume 都使用原冻结身份，不会被未来 Session revision 偷换。

### 2.4 统一 request 证明与兼容入口

P12.23 dispatch plan 新增 `MatchesRequest`，把原先散落在 P12.24 composition 的 identity 对照收回计划契约。它验证：

- request projection 与 plan projection 完全一致；
- 三个 envelope 的 Host 与 sequence 一致；
- Create/Process/End command id 一致；
- Visual/Audio consumer 与 attempt id 一致。

P12.24 planned-current API 的外部签名不变，但内部只调用一次 `PrepareCurrent` 和一次 `TryDispatchPrepared`；其中直接 `CaptureCurrent`、plan factory 与 `TryDispatchProjection` 调用数均为 `0`。

## 3. 完整性

新增五个 focused Automation contract：

1. `PrepareCurrentImmutableValue`：未观察 Session 不能形成 prepared value；同一 current state/seed 重复准备得到完全一致、可生成匹配 transaction request 的 immutable value；
2. `DelayedAfterSourceAdvance`：准备后推进 source revision，prepared 不再匹配 current，但延迟执行仍只使用旧 projection 并成功 terminal；
3. `ReplayAndForeignHostFence`：exact prepared terminal replay 不增加 executor invocation；不同 seed 的 prepared value 被已绑定 Host 在 Create 前拒绝；
4. `ResumeExistingCreate`：调用方预先持久化 exact Create 后，同一 prepared value 从 sequence 1 恢复并完成；
5. `InvalidAndRetryFence`：默认无效值在副作用前拒绝；retryable Audio 只形成两条 Host record，重复消费同一 prepared 值只重放 retry-pending 证据，不自旋、不执行 End。

测试 projection 来自真实 `CombatRunCoordinator + fixed timeline + SwordRhythmProductSession + evaluation/presentation/effect-cue` 链。fake executor 只返回既有 opaque receipt，不伪造 projection、plan、transaction、Host record、retry 或 replay evidence。

## 4. 权威与兼容性边界

- P12.24 `ProductPlannedDispatch::TryDispatchCurrent` 签名、状态及结果结构保持兼容；
- P12.22 frozen dispatch、P12.21 transaction、ProductRoute、CommandHost/Router、ExecutionSession/Host 继续是唯一执行权威；
- 新 prepared layer 是普通 C++ value/stateless service，不是 UObject、Subsystem、World service、queue、scheduler 或 worker；
- production 新层无 reflection、World/Actor、资产加载/播放、spawn、timer、async/thread、RNG、循环、damage、attribute 或 gameplay-effect application；
- 没有自动安装到全局对象，没有 Host/executor ownership，没有后台 retry，没有未来 Session 再读取；
- ProductSession、ProductHost、GameMode、CombatRunCoordinator 与所有资产未修改；
- 长期未跟踪的 0.0.9B Prompt、Report、CSEMI、PDF 与用户资料未修改、未暂存、未提交。

## 5. 修改范围

- `demo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatch.h/.cpp`：新增 immutable prepared value、prepare result、dispatch result 与无状态 service；
- `demo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedDispatchTests.cpp`：新增五个准备/延迟/重放/恢复/retry contract；
- `demo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchPlan.h/.cpp`：新增统一 transaction request identity/projection 证明；
- `demo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchPlanTests.cpp`：增加 plan-to-request 正向证明；
- `demo_mapShanmenSwordRhythmEffectCueExecutionProductPlannedDispatch.h/.cpp`：兼容 API 改为复用 prepared service，并删除局部重复 identity matcher；
- `ShanmenRegressionMap.json`：新增 prepared 路径规则，并把 prepared/planned/plan/dispatch 互相纳入变更回归；
- `Test-ShanmenRegressionCoverageSelfTest.ps1`：新增 prepared 完整日志正例、focused-only 缺证据反例，并更新三个既有正例；
- Report/Log 生成前 10 个代码/流程文件净变更 `+903 / -73`。

## 6. Automation 证据

| Group | Success | Fail | Native exit | SHA-256 |
|---|---:|---:|---:|---|
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductPreparedDispatch` | 5 | 0 | 0 | `61A2A05B8ABF6DEDF98D76FC31B182C0798642E11F12DD7142499ACA5B2E1D23` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductPlannedDispatch` | 5 | 0 | 0 | `C72629CCDDB11E252AEA9E3F05D3E437E78F57CEFDA18B9EC88CCCE3B70367FD` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductDispatchPlan` | 5 | 0 | 0 | `412380153349392D947EC87D059BA2DBBCE30A3B0C569E1634EC3A48560039FE` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductDispatch` | 10 | 0 | 0 | `4C7AF786A4BA4965EF90736AB019A6D6FD34F98299EEDC05C220EE4B68F7D51A` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductTransaction` | 5 | 0 | 0 | `CA4AA3137E4B338AE462308B2E2DC11E868B1231E1ECA9564941BC5349C7DF8D` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductRoute` | 5 | 0 | 0 | `DAD2EB2C35921124E9EB5313D9D2CDE4680CD79322DC965B7BBED5416D3DA6B1` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionCommandHost` | 5 | 0 | 0 | `34711ED181F42F331CD33811AC9BCD98F8B530F3987E1155C8868867EBC57640` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionCommandRouter` | 5 | 0 | 0 | `B466FE6105526E63C9887083964120F97C20D149FD44217F7F995E292D82DCAB` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionSession` | 5 | 0 | 0 | `DBF2EF122E54FB89125DA73932BC147386D8C58DF3AAC57F29214F6AC3DBC21A` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionHost` | 5 | 0 | 0 | `DE609C86FEE6F6421DC8DEC42A26CEB90BE21961F7366DDF16EFDA696B72EEB7` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionDriver` | 5 | 0 | 0 | `BF212BCCEE641BA09F4FAF9B616B4A2423DF06AB291F648609B31B9961584BBD` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutorAdapter` | 5 | 0 | 0 | `E5F8F42A54121E1A6D3762D21D330FB016BFB927256A20DE3E6814E9F380FA89` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueConsumerAttempt` | 5 | 0 | 0 | `AABA733F6985A12D96D6EF51B2F962A1D46E7F40782CCB0DF8DDE52463C8FB91` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueDelivery` | 4 | 0 | 0 | `2493ADBF8BDBE0B03AD377FF9769544DD22E6FCCC2441D7008728E529DEE1D27` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCue` | 72 | 0 | 0 | `9A23A279651738731F2BAF2CFC15B9B3D03A0AF81085EEDAB15660A052813BB4` |
| `Shanmen.0_0_10.Product.SwordRhythmPresentation` | 4 | 0 | 0 | `55C3B6D801856249716D7DB19391DB7AC97537AE05625D43C59FF4A112DE86D5` |
| `Shanmen.0_0_10.Product.SwordRhythmProductSession` | 1 | 0 | 0 | `FD1A54ED91D6BBFD84CE8CDEC70109FB72D3FF7CE04FDE78CEFE57FEB157DD7C` |
| `Shanmen.0_0_10.Product.SwordRhythmEvaluationRoute` | 1 | 0 | 0 | `61FC28138AC25CC87C6A328D1E65776FFE0DC77BCF94A6546DA3A8D1BD8E89CA` |
| `Shanmen.0_0_10` | 638 | 0 | 0 | `342D0AFE5F4759187BFD017F6D44D0B250E7349AE3233AAE7619C49C08465672` |

十九份最终日志均有唯一 selected `RunTests` group、唯一 terminal marker、Fail `0`；selected phase error/Fatal/Unhandled/Ensure 为 `0`。EffectCue 由 `67` 增至 `72`，全量由 `633` 增至 `638`。

## 7. 静态与回归门禁

```text
REGRESSION_COVERAGE: PASS Changed=10 Rules=3 Required=26 Logs=19
SELF_TEST: PASS 212/212
PREPARE_CALLS: PASS CaptureCurrent=1 PlanCapture=1 TryDispatchProjection=1
COMPAT_CALLS: PASS PrepareCurrent=1 TryDispatchPrepared=1 DirectCaptureCurrent=0 DirectPlanCapture=0 DirectFrozenDispatch=0
RUNTIME_BOUNDARY_SCAN: PASS
AUTHORITY_SCAN: PASS ProductSession ProductHost GameMode CombatRunCoordinator unchanged
JSON_PARSE: PASS
git diff --cached --check: PASS (native exit 0)
```

## 8. 构建证据

有效命令：`Build.bat <Target> Win64 Development demo_map.uproject -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`。

| Target | Result | Actions / total time | Exit |
|---|---|---|---:|
| Editor initial | Succeeded | 9 / 36.95s | 0 |
| Game final | Succeeded | 8 / 32.95s | 0 |
| Editor final | Succeeded, up to date | 0 / 0.91s | 0 |

最终产物：

- `UnrealEditor-demo_map.dll`：13,743,616 bytes，SHA-256 `9FB5B825BBF3EF617EAB70721A1EFD2237C5B53F97FAB6E7865DE46D07802258`；
- `demo_map.exe`：355,284,992 bytes，SHA-256 `0D4E438EC777D8588A2322A48C47A9BCF5AB600AA29BBB57B52CE5D50963808F`。

所有有效构建均原生退出 `0`，未出现源码失败、C3859、C1076、系统代码 1455 或其它 commit-memory/page-file 环境错误。

## 9. 流程与异常

首次 focused prepared-dispatch Automation 即为 `5 Success / 0 Fail`，原生退出 `0`，SHA `3E6C0FFD368391611998A7087AA4C2CE63048F22DDCA6355292A126A039D662E`；最终复跑 SHA 见第 6 节。本轮没有源码、测试或构建失败。

第一次通用 boundary scan 把 P12.23 plan 中既有的两个固定八项 identity distinctness `for` 循环标出；它们是纯值、确定、有限验证，并非 P12.25 执行或 retry loop。最终边界扫描按本轮新增 prepared/planned ownership layer 检查并通过；没有删除该 fail-closed identity 证明。

UE 启动阶段仍有 selected `RunTests` 之前的既有 13 条 automation condition diagnostics；最终 19 份日志 selected phase 均无 error/Fatal/Unhandled/Ensure。跨平台 SDK 探测仍提示 LinuxArm64/VisionOS 缺少 `MainVersion`，但 Win64 SDK 为 VALID。raw Automation/build 日志仅本地保留，不纳入 Git。

## 10. P/F 边界与下一步

本 Report 只包含 P 阶段 immutable prepared value、无状态 prepare/dispatch composition、注入 fake executor、静态审查、NullRHI 无头 Automation、changed-file 回归与 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、实际动画/声音/VFX、Cook 或 Package。

建议 P12.26 在 caller-owned prepared value 之上增加显式 retry-attempt renewal：只为 retryable consumer 派生新的 attempt identity，并保留原 projection、plan root 与已成功 consumer receipt；仍由调用方决定是否/何时重试，不引入后台队列或 executor ownership。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p12-25-sword-rhythm-cue-execution-prepared-dispatch>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-25-sword-rhythm-cue-execution-prepared-dispatch/Docs/Report/Dev.D.UE.0.0.10.P12.25.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-25-sword-rhythm-cue-execution-prepared-dispatch/Docs/Log/Dev.D.UE.0.0.10.P12.25.r0_log.md>
