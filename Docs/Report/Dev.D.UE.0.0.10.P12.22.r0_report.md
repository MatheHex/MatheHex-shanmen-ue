# Dev.D.UE.0.0.10.P12.22.r0 Report

## 1. 结论

P12.22 已完成并通过 P 阶段门禁。

本阶段在 P12.21 frozen single-projection transaction 之上增加最薄的 current ProductSession dispatch seam：调用方显式提供 transaction identity、caller-owned Host 与 Visual/Audio executor；dispatch 只读取一次当前 projection、捕获一次 frozen transaction、执行一次 transaction，并返回 projection/capture/transaction 三层完整证据。

最终结果：

- 未观察到当前 ProductSession presentation 时，在 transaction capture、Host mutation 与 executor 进入前 fail closed；
- 当前 projection 只捕获一次，transaction request 只冻结一次，transaction 只调用一次；
- fresh Host 完成时返回 `Dispatched`，已有 exact Create 时返回 `Resumed`，terminal exact history 时返回 `Replayed`；
- 结果会 deep-match frozen projection 与 transaction request，并核对 RunId、ConfigId、CuePolicyId、EventId；
- caller 可保留返回的 frozen request，在 source ProductSession 推进后对旧 Host 做历史 exact replay；
- dispatch 不生成 identity、不保存 Host/executor、不循环、不自动 retry、不接触 World 或资产；
- focused dispatch `5/5`，EffectCue 父前缀 `57/57`，0.0.10 全量 `623/623`，Fail `0`；
- changed-file gate：`Changed=5 / Rules=1 / Required=23 / Logs=16`；
- regression gate self-test `206/206`；
- `git diff --check`、静态所有权扫描、Editor/Game Development 单并发构建全部通过。

## 2. 功能性

### 2.1 当前 projection 的单次读取

`TryDispatchCurrent` 首先调用既有 `ProductProjector::CaptureCurrent(Session)`。失败时返回 `ProjectionRejected`，不捕获 transaction、不写 Host、不进入 executor。

成功结果保存完整 projection evidence，供后续 transaction capture 与最终一致性校验使用。dispatch 不缓存 ProductSession，也不推测 future state。

### 2.2 frozen transaction 的单次捕获与执行

projection 成功后，dispatch 使用调用方提供的完整 identity 调用一次 P12.21 `ProductTransactionFactory::Capture`。capture 失败返回 `CaptureRejected`；成功后仅调用一次 `ProductTransaction::TryExecute`。

transaction 状态映射为：

- `Completed` → `Dispatched`；
- `Resumed` → `Resumed`；
- `Replayed` → `Replayed`；
- 其它 retry/reject/incomplete → `TransactionIncomplete`。

生产实现静态计数为 `Projection=1 / Capture=1 / Execute=1`，且无 loop，因此该 seam 不会隐式轮询、重试或重复消费。

### 2.3 完整证据与 source-advance fence

`IsSuccess()` 不只检查表面状态，还要求：

- projection、transaction capture 与 terminal transaction 都有效；
- frozen transaction request 内的 projection 与本次捕获 projection deep-match；
- terminal transaction 的 RunId、ConfigId、CuePolicyId、EventId 与 captured projection 一致。

返回值保留 `TransactionCapture.Request`。即使 source Session 后续推进到新 presentation，调用方仍可用旧 frozen request 对旧 Host 执行 P12.21 exact replay；当前 dispatch 本身则始终只面向最新已观察状态。

## 3. 完整性

新增五个 focused Automation contract：

1. `CompletedCurrent`：未观察 Session fail closed；观察真实当前 effect 后 fresh Host 完成，双 executor 各进入一次；
2. `IdempotentCurrentReplay`：同一当前状态与同一 identity 再次 dispatch 走 terminal replay，record 与 executor invocation 不增加；
3. `ResumeExistingCreate`：Host 已有 exact Create durable evidence 时从 Process/End 继续并返回 `Resumed`；
4. `SourceAdvanceFence`：source Session 推进后，旧 identity 对旧 Host 因 payload 不同而拒绝；保留的旧 frozen request 仍可 exact replay；
5. `ProjectionCaptureAndProcessFence`：未观察 source、无效 identity 在 Host/executor 前拒绝；Process retry 保留两条 durable record，不发送 End、不自动循环。

测试 source 来自真实 `CombatRunCoordinator + fixed timeline + SwordRhythmProductSession + evaluation/presentation/effect-cue` 链。fake executor 只实现既有注入接口，不伪造 projection、transaction request、Host history 或 replay evidence。

## 4. 权威与兼容性边界

- P12.1—P12.21 的 ProductSession、ProductHost、GameMode、CombatRunCoordinator 与 transaction authority 均未修改；
- dispatch 是 stateless C++ class，不是 UObject、Subsystem、World service、registry、worker 或 scheduler；
- identity、Host、executor 与 replay 决策均由调用方拥有；本层不创建 GUID、sequence、attempt、consumer 或 Host；
- production 文件无 reflection、World/Actor、资产加载/播放、spawn、timer、async/thread、RNG、damage、attribute 或 gameplay-effect application；
- transaction retry/reject 只作为 `TransactionIncomplete` 返回，不在 dispatch 内继续执行；
- 长期未跟踪的 0.0.9B Prompt、Report、CSEMI、PDF 与用户资料未修改、未暂存、未提交。

## 5. 修改范围

- `demo_mapShanmenSwordRhythmEffectCueExecutionProductDispatch.h/.cpp`：当前 projection → frozen transaction → single execute 的 stateless dispatch 与一致性结果；
- `demo_mapShanmenSwordRhythmEffectCueExecutionProductDispatchTests.cpp`：五个真实 source-to-transaction contract；
- `ShanmenRegressionMap.json`：新 dispatch 路径映射到 23 组 source、projection、transaction、Host 与 runtime authority 证据；
- `Test-ShanmenRegressionCoverageSelfTest.ps1`：新增完整日志正例与 focused-only 缺证据反例；
- Report/Log 生成前 5 个代码/流程文件净变更 `+684 / -0`。

## 6. Automation 证据

| Group | Success | Fail | Native exit | SHA-256 |
|---|---:|---:|---:|---|
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductDispatch` | 5 | 0 | 0 | `75E7CD4F8065FE768049230B4CE6F6949CBAA5916A3DEE7204DAA3795B5CE29A` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductTransaction` | 5 | 0 | 0 | `293F554D26B5345C63075A13A456BFB8FB3C4510AE9C3DE78FAC94833959674C` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductRoute` | 5 | 0 | 0 | `BA68C9185C17260FCC0D7A8FBD6D87BB4568CE73DBA04E3D78E12C2087C67735` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionCommandHost` | 5 | 0 | 0 | `363816E7498D8F14C6ACB063176C747B674DF4044ED7F461DD67D160248BBDA2` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionCommandRouter` | 5 | 0 | 0 | `CCAF347098418FFEF3D9E8877A203CAF0AC72894AB3468E0CAA2659E2E39D5B1` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionSession` | 5 | 0 | 0 | `A6C69065C966EE40F9FFE36990AE14E4BA812B8D0ED9901826489B52A2ECDC0B` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionHost` | 5 | 0 | 0 | `DF4E76A35B31501F468B039B3F3E2459C4B38D6FA2E34B8528B6DEF539B54DB0` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionDriver` | 5 | 0 | 0 | `60F25E0DB8499DE6E9D01EA7AA94DD5A073AF12E2CB1BC4FC3971C61E6C19BBB` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutorAdapter` | 5 | 0 | 0 | `4CA6157612865D90ACD234812BBC136754F1AEE8C63362370F57CDA284377BDE` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueConsumerAttempt` | 5 | 0 | 0 | `F3B8D0DE79132447731F63C5D237ED041AEBA78812D4C7328020A1F20785387C` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueDelivery` | 4 | 0 | 0 | `8ED294C0C3AE0472213C6A45ECF078D0E6F9896931F686771B92BE1E332640E5` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCue` | 57 | 0 | 0 | `5A5E4853494591A586B1A08DFE9E4385B2E14BCFD64AD044666F1ACB4FCD63ED` |
| `Shanmen.0_0_10.Product.SwordRhythmPresentation` | 4 | 0 | 0 | `F31FD4AE5564F24133B4056712C26D87619E766B411B51F4F0B8095473EB1911` |
| `Shanmen.0_0_10.Product.SwordRhythmProductSession` | 1 | 0 | 0 | `3766C2E7D297E677FCC461ACFD6896EA37CE457731C3E6D4AD9ADD23908C473B` |
| `Shanmen.0_0_10.Product.SwordRhythmEvaluationRoute` | 1 | 0 | 0 | `A4698F9EA267916DEF470F6A077CC4F0C6B7DC3A725E6929E72CD2F178FA93BD` |
| `Shanmen.0_0_10` | 623 | 0 | 0 | `00A475F3C5706F6D92F835487F94CB0EB2ED144E72A097E40DCAC6A23BFCF478` |

EffectCue 父前缀由 `52` 增加至 `57`；全量由 `618` 增加至 `623`。十六份最终日志均有唯一 selected `RunTests` group、native terminal marker、Fail `0`；selected phase error/Fatal/Unhandled/Ensure 为 0。

## 7. 静态与回归门禁

```text
REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=23 Logs=16
SELF_TEST: PASS 206/206
DISPATCH_CALL_SCAN: PASS Projection=1 Capture=1 Execute=1
AUTO_RETRY_LOOP_SCAN: PASS loops=0
RUNTIME_BOUNDARY_SCAN: PASS World/Actor/assets/timer/async/thread/RNG absent
REFLECTION_SCAN: PASS
AUTHORITY_SCAN: PASS ProductSession ProductHost GameMode CombatRunCoordinator unchanged
JSON_PARSE: PASS
git diff --check: PASS (native exit 0)
```

## 8. 构建证据

有效命令：`Build.bat <Target> Win64 Development demo_map.uproject -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`。

| Target | Result | Actions / total time | Exit |
|---|---|---|---:|
| Editor initial | Succeeded | 5 / 16.50s | 0 |
| Game final | Succeeded | 4 / 23.88s | 0 |
| Editor final | Succeeded, up to date | 0 / 0.91s | 0 |

最终产物：

- `UnrealEditor-demo_map.dll`：13,660,160 bytes，SHA-256 `0F6C88C4A5255FA1DC12ABDCB7D564F46183DB4952130C27694DB7E6B446FBAB`；
- `demo_map.exe`：355,216,384 bytes，SHA-256 `ADF394CF7BA7A40236F89F7E38E3B8215CCAC04E3FE34EA64DA170A61D86231A`。

三次有效构建均原生退出 `0`，未出现源码失败、C3859、C1076、系统代码 1455 或其它 commit-memory/page-file 环境错误。

## 9. 流程与异常

首次 Editor 构建即成功；首次 focused Automation 即为 `5 Success / 0 Fail`、原生退出码 `0`，没有产品代码或测试修正轮。最终证据采用源码与回归映射封定后的 `P12.22-*-final.log`。

UE 启动阶段仍有 selected `RunTests` 之前的既有 13 条 automation condition diagnostics；selected phase error/Fatal/Unhandled/Ensure 为 0。十六个 Windows Automation 进程均原生退出 `0`。raw Automation 与 build 日志仅作为本地可复核证据，不纳入 Git。

## 10. P/F 边界与下一步

本 Report 只包含 P 阶段 stateless dispatch、注入 fake executor、静态审查、NullRHI 无头 Automation、changed-file 回归与 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、实际动画/声音/VFX、Cook 或 Package。

建议 P12.23 增加 caller-owned deterministic dispatch plan factory：从调用方提供的 dispatch seed 与 frozen projection identity 派生 P12.21 所需 identity bundle，再交给本阶段 dispatch。该 factory 只生成 value，不保存全局 allocator/registry、不持有 Host、不启动 retry，以减少 live call site 的七组 identity 样板，同时保持 replay 与碰撞测试可验证。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p12-22-sword-rhythm-cue-execution-product-dispatch>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-22-sword-rhythm-cue-execution-product-dispatch/Docs/Report/Dev.D.UE.0.0.10.P12.22.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-22-sword-rhythm-cue-execution-product-dispatch/Docs/Log/Dev.D.UE.0.0.10.P12.22.r0_log.md>
