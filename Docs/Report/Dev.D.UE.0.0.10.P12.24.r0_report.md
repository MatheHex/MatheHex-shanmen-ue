# Dev.D.UE.0.0.10.P12.24.r0 Report

## 1. 结论

P12.24 已完成并通过 P 阶段门禁。

本阶段在 P12.23 deterministic dispatch plan 与 P12.22 Product dispatch 之间增加无状态的 planned-current composition seam：一次调用只读取一次当前 Product projection、捕获一次 deterministic plan，并把同一 frozen projection 与 plan identity 交给一次 frozen dispatch。P12.22 同时新增 `TryDispatchProjection`，原 `TryDispatchCurrent` 兼容入口继续保留。

最终结果：

- planned dispatch 生产路径中 `CaptureCurrent`、plan `Capture`、`TryDispatchProjection` 各恰好调用一次；
- projection、plan、transaction request 三份证据逐层交叉核对，不能把别的 projection 或任意 identity 混入成功结果；
- 同一 Session 状态和 seed 在 terminal Host 上精确重放，不重新进入 executor；
- 已持久化 Create 的 Host 可从 sequence 1 恢复并完成；
- source revision 推进会派生新 plan；旧 Host 拒绝新计划，保留的旧 request 仍可精确重放；
- 零命令 projection 继续以 Visual/Audio `NoOpSucceeded` 完成，不伪造 executor invocation；
- invalid projection/seed 在 Host 与 executor 之前 fail closed，retry-pending 不隐藏 End 或后台 retry；
- focused planned dispatch `5/5`，EffectCue 父前缀 `67/67`，0.0.10 全量 `633/633`，Fail `0`；
- changed-file gate：`Changed=7 / Rules=2 / Required=25 / Logs=18`；
- regression gate self-test `210/210`；
- staged `git diff --check`、静态边界扫描、Editor/Game Development 单并发构建全部通过。

## 2. 功能性

### 2.1 冻结投影派发入口

P12.22 `ProductDispatch` 现在同时提供：

- `TryDispatchProjection`：接收调用方已冻结且自验证通过的 projection，不读取 ProductSession；
- `TryDispatchCurrent`：兼容旧调用方，内部只捕获一次 current projection，然后进入同一 dispatch helper。

两条入口共享 transaction capture、transaction execution、Completed/Resumed/Replayed 状态映射及最终证据验证，避免形成第二套执行系统。

### 2.2 Planned-current composition

`ProductPlannedDispatch::TryDispatchCurrent` 的固定顺序为：

1. `ProductProjector::CaptureCurrent(Session)`；
2. `ProductDispatchPlanFactory::Capture(frozen projection, seed)`；
3. `ProductDispatch::TryDispatchProjection(frozen projection, plan identity, caller Host/executors)`。

任何前置步骤失败都立即返回，不触碰后续 Host/executor。成功结果要求：

- projection capture 有效；
- plan capture 有效且匹配该 projection；
- dispatch 内保存的 projection 与首次 capture deep-match；
- transaction request 的 Host、sequence、三 command、双 consumer、双 attempt identity 与 plan 完全一致；
- transaction request 自身保存的 projection 与首次 capture deep-match。

### 2.3 所有权与恢复语义

新层不保存 Session、seed、plan、Host、executor、sequence 或 retry 状态。调用方继续拥有所有 mutable authority。首次执行、durable Create 恢复、terminal replay、Create 冲突及 retry-pending 都由 P12.19—P12.23 既有证据链决定；planned layer 只做一次同步组合和状态翻译。

## 3. 完整性

新增五个 focused Automation contract：

1. `CompletedSingleRead`：未观察 Session 在 plan/Host/executor 前拒绝；有效 current state 形成一份匹配 projection/plan/transaction 的 terminal dispatch；
2. `DeterministicReplay`：同一 current state 与 seed 形成完全相同的 plan/request，terminal replay 不增加 executor invocation；
3. `ResumeExistingCreate`：调用方预先持久化 exact Create 后，planned dispatch 从 sequence 1 恢复并完成；
4. `SourceAdvancePlanSeparation`：source revision 推进后新 plan 与旧 plan 分离；旧 Host fail closed、旧 request 仍精确重放、新 Host 完成新 projection，并保留双通道显式 no-op 证据；
5. `ProjectionSeedAndRetryFence`：无效 seed、无效 frozen projection 在副作用前拒绝；retryable Audio 只持久化到 Process retry-pending，不执行 End、不自旋重试。

测试 projection 来自真实 `CombatRunCoordinator + fixed timeline + SwordRhythmProductSession + evaluation/presentation/effect-cue` 链。fake executor 只返回既有 opaque receipt，不伪造 plan、projection、transaction、Host record、no-op 或 replay evidence。

## 4. 权威与兼容性边界

- P12.22 原 `TryDispatchCurrent` 签名与语义保留；其实现改为一次 current capture 后复用 frozen helper；
- P12.23 plan、P12.21 transaction、ProductRoute、CommandHost/Router、ExecutionSession/Host、ProductSession、ProductHost、GameMode 与 CombatRunCoordinator 未修改；
- 新 production layer 是普通 C++ value/stateless class，不是 UObject、Subsystem、World service、registry、scheduler 或 worker；
- production 文件无 reflection、World/Actor、资产加载/播放、spawn、timer、async/thread、RNG、循环、damage、attribute 或 gameplay-effect application；
- 没有自动安装到全局对象，没有 Host/executor ownership，没有隐藏 retry 或未来 Session 再读取；
- 长期未跟踪的 0.0.9B Prompt、Report、CSEMI、PDF 与用户资料未修改、未暂存、未提交。

## 5. 修改范围

- `demo_mapShanmenSwordRhythmEffectCueExecutionProductDispatch.h/.cpp`：新增 frozen projection 入口并抽取两入口共用的 dispatch helper；
- `demo_mapShanmenSwordRhythmEffectCueExecutionProductPlannedDispatch.h/.cpp`：新增 single-read planned composition、状态映射与 projection/plan/request identity 交叉证明；
- `demo_mapShanmenSwordRhythmEffectCueExecutionProductPlannedDispatchTests.cpp`：五个 source-to-transaction contract；
- `ShanmenRegressionMap.json`：planned 路径映射到 25 组 plan、dispatch、Host 与 runtime 证据；修改 P12.22 文件时也强制 planned/plan 回归；
- `Test-ShanmenRegressionCoverageSelfTest.ps1`：新增完整日志正例、focused-only 缺证据反例，并更新 P12.22 正例；
- Report/Log 生成前 7 个代码/流程文件净变更 `+903 / -69`。

## 6. Automation 证据

| Group | Success | Fail | Native exit | SHA-256 |
|---|---:|---:|---:|---|
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductPlannedDispatch` | 5 | 0 | 0 | `F6074B7A2FD589E653451EEF9F585A10F44E359FFAB410DF9C582DF4A98B6934` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductDispatchPlan` | 5 | 0 | 0 | `B7A0AFF3835313896C5F8F9797452B7815FA21252731AC680C10FBF91A52F193` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductDispatch` | 10 | 0 | 0 | `71C063F37C207EE8E53788A598A6266FC05BD812870539FFA292997CAA75CB16` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductTransaction` | 5 | 0 | 0 | `794908C27B63B0EC6D97C28C071C734DABE521BC7AB72D6608916A31FF32EFFB` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductRoute` | 5 | 0 | 0 | `B05F6A2B2B7830EF35D0DB020F5C548FBD415F0BA8F4927AC262E8C1E506CA33` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionCommandHost` | 5 | 0 | 0 | `6FB7AA07E807A5EC32C8AE63E44CD97D2E5F18FE8B98CE7B37BFFA5B34FED64D` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionCommandRouter` | 5 | 0 | 0 | `09493600DBCB8C1ACD5E690350996ABB410572D4C96CA7820263658F6BF5A646` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionSession` | 5 | 0 | 0 | `F023D4E8A4376684DFFAAB947BFEB64CD7CB80202CBF9D06C686EBADE068565C` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionHost` | 5 | 0 | 0 | `907DADB32873C75F890BB2939DF1AE1D83B2F1C390FD81C96B4E46546B2CD469` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionDriver` | 5 | 0 | 0 | `84711A8DCBBE62C4BF668ED3D58ACF7820ADC01E46E754D3F067B8124D9A60AC` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutorAdapter` | 5 | 0 | 0 | `9AB1EB6E007116AABA0449DDB643C42D8460909D43DF9FAE2B03791A22525337` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueConsumerAttempt` | 5 | 0 | 0 | `063D305F3311821ABE8667641094B10DC4D68F63CC06507077B418A548869D7A` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueDelivery` | 4 | 0 | 0 | `F88943A8A0372A4130BA5C9265AD4F970A5DAB03FB82390948AB6D059726BED0` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCue` | 67 | 0 | 0 | `1AAAAE127809B3D5512841C3BA30F59AF74C9979EB004C05A9B1EFCA7D300BAB` |
| `Shanmen.0_0_10.Product.SwordRhythmPresentation` | 4 | 0 | 0 | `C1C746A1272AE6BC9EBDE818D0DC94E18C240CC305DC350D2B3A83CF9FC20F26` |
| `Shanmen.0_0_10.Product.SwordRhythmProductSession` | 1 | 0 | 0 | `A90DFA6E01DFC4942142E298B47665570E2ED8BEAD35CCF45BB9326AC48319A7` |
| `Shanmen.0_0_10.Product.SwordRhythmEvaluationRoute` | 1 | 0 | 0 | `F24D398AED66992BF24AF82F1E8B9EA22AB6ACD403F5C95D640D4764A0526D37` |
| `Shanmen.0_0_10` | 633 | 0 | 0 | `982DC00445237409ED56D6F79816D0E55E775EB3BD6A1C4E9F84119A479A12A8` |

十八份最终日志均有唯一 selected `RunTests` group、唯一 terminal marker、Fail `0`；selected phase error/Fatal/Unhandled/Ensure 为 `0`。ProductDispatch 父前缀仍为自身 5 个 contract 加 DispatchPlan 5 个 child contract，共 `10/10`；EffectCue 由 `62` 增至 `67`；全量由 `628` 增至 `633`。

## 7. 静态与回归门禁

```text
REGRESSION_COVERAGE: PASS Changed=7 Rules=2 Required=25 Logs=18
SELF_TEST: PASS 210/210
COMPOSITION_CALLS: PASS CaptureCurrent=1 PlanCapture=1 TryDispatchProjection=1
LEGACY_CURRENT_CAPTURE: PASS CaptureCurrent=1
RUNTIME_BOUNDARY_SCAN: PASS reflection/World/Actor/assets/timer/async/thread/RNG/loops absent
AUTHORITY_SCAN: PASS ProductSession ProductHost GameMode CombatRunCoordinator unchanged
JSON_PARSE: PASS
git diff --cached --check: PASS (native exit 0)
```

## 8. 构建证据

有效命令：`Build.bat <Target> Win64 Development demo_map.uproject -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`。

| Target | Result | Actions / total time | Exit |
|---|---|---|---:|
| Editor initial | Succeeded | 9 / 38.92s | 0 |
| Game final | Succeeded | 8 / 32.36s | 0 |
| Editor final | Succeeded, up to date | 0 / 0.93s | 0 |

测试诊断修改后的三次 Editor 增量构建也均成功：`4 actions / 6.29s`、`4 / 6.01s`、`4 / 5.88s`，原生退出码均为 `0`。

最终产物：

- `UnrealEditor-demo_map.dll`：13,715,968 bytes，SHA-256 `E3798C6FD62488E1E727B0E39573D22AD393D427F0782DAE99B63555A8E3278A`；
- `demo_map.exe`：355,263,488 bytes，SHA-256 `7F473EFFECFF27EECB03F85FBF64DC054DFD77373044C413E0901CD901C7663A`。

所有有效构建均原生退出 `0`，未出现源码失败、C3859、C1076、系统代码 1455 或其它 commit-memory/page-file 环境错误。

## 9. 流程与异常

首次 focused Automation 为 `4 Success / 1 Fail`：`SourceAdvancePlanSeparation` 错误假设 source 推进后的新 projection 必然再次调用两个 executor；raw log SHA 为 `90151EF1F08B9A8A2F2F87F85191E48AC5714B754D81CC67C118657FC3C8270F`。拆分合并断言后的第二次诊断仍为 `4/1`，SHA `14B372BA8E2E51010FDB184167F8DEE76571AB21291090404800E93B14042059`；改用数值断言后的第三次诊断确认 Visual/Audio 实际 invocation 均保持 `1` 而不是预期 `2`，SHA `3D27814234E3AFA9A72E5888E99290EF50ABC300A7658855817997009D779368`。

证据显示新 revision 是合法的双通道零命令 projection：fresh Host terminal success，Visual/Audio 都返回既有显式 `NoOpSucceeded`，按 P12.13 executor adapter 契约不应进入 executor。最终只修正测试，增加 no-op 状态与 invocation 不变断言；生产代码未因该失败修改。修正后 focused 为 `5/5`，最终 SHA 见第 6 节。

三次失败运行的 Unreal 进程原生退出码均为 `0`，因此本阶段仍以 selected test Result 判定失败，未把进程退出码误报为测试成功。UE 启动阶段仍有 selected `RunTests` 之前的既有 13 条 automation condition diagnostics；最终 18 份日志 selected phase 均无 error/Fatal/Unhandled/Ensure。跨平台 SDK 探测仍提示 LinuxArm64/VisionOS 缺少 `MainVersion`，但 Win64 SDK 为 VALID。raw Automation/build 日志仅本地保留，不纳入 Git。

## 10. P/F 边界与下一步

本 Report 只包含 P 阶段无状态 composition、frozen projection dispatch、注入 fake executor、静态审查、NullRHI 无头 Automation、changed-file 回归与 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、实际动画/声音/VFX、Cook 或 Package。

建议 P12.25 把“准备”和“执行”拆成 caller-owned immutable prepared-dispatch value：`PrepareCurrent` 一次冻结 projection+plan，`TryDispatchPrepared` 可在之后或 retry 时只消费该 frozen value，不重新读取 live Session；仍不保存 Host、executor 或后台 retry 权威。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p12-24-sword-rhythm-cue-execution-planned-dispatch>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-24-sword-rhythm-cue-execution-planned-dispatch/Docs/Report/Dev.D.UE.0.0.10.P12.24.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-24-sword-rhythm-cue-execution-planned-dispatch/Docs/Log/Dev.D.UE.0.0.10.P12.24.r0_log.md>
