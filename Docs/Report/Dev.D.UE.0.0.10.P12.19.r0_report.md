# Dev.D.UE.0.0.10.P12.19.r0 Report

## 1. 结论

P12.19 已完成并通过 P 阶段门禁。

本阶段在 P12.18 typed execution command router 外增加一个薄的 Run-local command host。Host 只补 transport identity 与有序交接：每个冻结 envelope 由 caller-owned `HostId`、连续 `Sequence`、deterministic `DispatchId` 和一个 P12.18 command 组成；Host 自身只拥有一个 router 与 durable envelope receipts，不建立第三套 batch/session 权威。

最终结果：

- sequence `0` 必须是 `Create`，成功后绑定唯一 `HostId + RunId + BatchId`；
- 后续新 envelope 必须严格连续；future sequence、historical sequence 不同 payload、foreign HostId 均在 router/executor 前拒绝；
- exact historical envelope 返回首次 Host receipt，不重新进入 router、session 或 executor；
- `DispatchId` 由 HostId、Sequence、CommandId、kind、Run/Batch 与双 attempt identity 确定性派生并在 `IsValid` 中复算；
- router 的成功、retry pending、partial rejection 与 early-End rejection 只要形成 durable router receipt，就与 transport sequence 原子提交；
- fresh sequence 包装已用 router `CommandId` 时返回 `CommandReplayConflict`，不消耗第二个 sequence；
- partial retry/reject 的恢复必须使用 next sequence、fresh CommandId 与 fresh failed-channel attempt；
- 成功 `End` 建立 terminal fence：历史 envelope 仍可 replay，新 envelope 一律拒绝；
- wrong executor overload 在 replay/route 前拒绝，不能借历史 receipt 绕过 typed context；
- focused Host `5/5`，EffectCue 父前缀 `42/42`，0.0.10 全量 `608/608`，Fail `0`；
- changed-file gate：`Changed=5 / Rules=1 / Required=20 / Logs=13`；
- regression gate self-test `200/200`；
- staged `git diff --check`、静态边界扫描、Editor/Game Development 单并发构建全部通过。

## 2. 功能性

### 2.1 Frozen transport envelope

`Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommandEnvelope::TryCapture` 复制一个已验证的 P12.18 command，并冻结：

- caller-owned `HostId`；
- 非负、可持久记录的 `int64 Sequence`；
- deterministic `DispatchId`；
- private command payload。

`Matches` 不只比较 DispatchId，也调用 command deep match；因此同序号的 Create event/consumer 变化或 Process attempt 变化都不能伪装成 exact replay。

### 2.2 Contiguous sequence 与 stable replay

Host 只接受 `Sequence == NextSequence` 的新 envelope。处理顺序为：

1. envelope/Host/context 验证；
2. historical exact replay 或 sequence conflict；
3. gap、terminal、Host/batch lifecycle fence；
4. candidate router 单次调用；
5. router receipt 与 Host record 原子提交。

历史 exact replay 复制首次 Host result，设置 `bReplay=true` 与 `bHostStateCommitted=false`，不调用 router/executor，也不增加 record。相同 historical sequence 的不同 envelope 返回 `SequenceConflict`。

### 2.3 Router replay 与 sequence ownership

P12.18 router 以 CommandId 幂等；P12.19 Host 以 Sequence 幂等。两者交界处显式处理：

- 已提交 envelope 的重放由 Host ledger 截获；
- fresh sequence 若携带 router 已见过的 exact command，router 会返回 replay，Host 将其提升为 `CommandReplayConflict`；
- 该冲突不提交 Host record、不推进 NextSequence、不调用 executor；
- router 的不同 payload CommandId conflict 同样不消耗 Host sequence。

这样一个 router durable command 只能对应一个 Host sequence，不会出现 transport ledger 与 router ledger 的双计数。

### 2.4 Partial commit 与恢复

retry pending、Visual/Audio 单侧 reject、early End 均可能已有 router/session sibling state，因此 durable router receipt 必须占用当前 Host sequence。Host 保存首次 receipt，exact replay 不重试；调用方只能在 next sequence 使用 fresh command 与 fresh failed-channel attempt 恢复。

Host result 区分：

- `Routed`：router receipt 成功；
- `RouterRejected`：router receipt 不成功但 durable；
- envelope/identity/sequence/context/lifecycle/conflict：副作用前非 durable 拒绝。

### 2.5 Terminal fence 与 ownership

成功 End 后 `bTerminal` 必须与唯一 router 的 `IsEnded()` 一致，且 End record 必须是最后一条。terminal Host 仍允许正确 overload 的 historical exact replay，但不接受任何新 sequence。

Host 不保存 executor，不拥有 ProductSession、World、资产、timer、线程，也不自动发现命令、tick、poll、retry 或 teardown。P12.17 session 仍是唯一 batch-progress authority，P12.18 router 仍是唯一 typed command authority。

## 3. 完整性

新增五个 focused Automation contract：

1. `OrderedLifecycle`：真实三 event batch、sequence 0 Create、逐 event 连续推进、End terminal、历史 Process/End executor-free replay；
2. `SequenceFence`：future Create、pre-create Process、foreign HostId、future gap、historical payload conflict，全部在 executor 前拒绝；
3. `RetrySequenceRecovery`：Audio retry 占用 sequence、exact replay inert、同序号 payload conflict、next sequence 仅恢复 Audio；
4. `RejectAndRouterReplayFence`：Visual reject 与 Audio acknowledgement durable、next sequence 仅恢复 Visual、fresh sequence 的旧 Router CommandId 不重复计数；
5. `ValidationAndTerminal`：invalid HostId/negative sequence、wrong overload、durable early End、完成后 terminal End、post-terminal reject 与历史 record inspection。

测试使用真实 `CombatRunCoordinator + fixed timeline + SwordRhythmProductSession + evaluation/presentation/effect-cue + delivery/attempt/executor-driver/host/session/router` 链。fake executor 仅替代既有显式注入边界，不伪造 command、envelope、batch、session、acknowledgement、router record 或 Host ledger。

## 4. 权威与兼容性边界

- P12.11—P12.18 的 EffectCue、delivery、attempt、executor adapter、driver、dual-consumer host、session 与 command router 均未修改；
- ProductSession、GameMode、CombatRunCoordinator、fixed timeline 与 runtime action authority 均未修改；
- command host 是 caller-owned 普通 C++ value owner，不是 UObject、Subsystem、World service、global registry 或后台 worker；
- production 文件无 reflection、World/Actor、asset loading/playback、spawn、timer、async/thread、RNG、damage、attribute 或 gameplay-effect application；
- executor 只作为同步 Process route 参数，不进入 envelope、record 或 Host 持久状态；
- 长期未跟踪的 0.0.9B Prompt、Report、CSEMI、PDF 与用户资料未修改、未暂存、未提交。

## 5. 修改范围

- `demo_mapShanmenSwordRhythmEffectCueExecutionCommandHost.h/.cpp`：冻结 transport envelope、deterministic DispatchId、contiguous sequence、dual-ledger conflict、candidate commit、stable replay 与 terminal fence；
- `demo_mapShanmenSwordRhythmEffectCueExecutionCommandHostTests.cpp`：五个真实 source-to-Host contract；
- `ShanmenRegressionMap.json`：新 Host 映射到 20 组 Host/router/session/source/product/runtime evidence；
- `Test-ShanmenRegressionCoverageSelfTest.ps1`：新增 Host 正例与 focused-only 反例；
- Report/Log 生成前 5 个代码/流程文件净变更 `+1416 / -0`。

## 6. Automation 证据

| Group | Success | Fail | Native exit | SHA-256 |
|---|---:|---:|---:|---|
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionCommandHost` | 5 | 0 | 0 | `46917ADEF33E57012F45B0733E50A20753DA47095FC4CEC0ECEAD3505D7791F2` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionCommandRouter` | 5 | 0 | 0 | `2552A896C9D32F93253AB57809CF2AB4F25879EF8B5D6881AB9521CBB205AF26` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionSession` | 5 | 0 | 0 | `464F96AFBDB34B2ABAEDD8B5B2D931FD1DB3C8614DE81E23CA156601A52980A8` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionHost` | 5 | 0 | 0 | `BDB91E59E547A42340EB17DD272454DDE90D0241932BE87449140B181D7B218B` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionDriver` | 5 | 0 | 0 | `6EA7D416FD92394BE00F727BC9BD0AD9873E3D587F5F410190B539C79DFB4FE7` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutorAdapter` | 5 | 0 | 0 | `EEF2889C64C28B0E078E37D65C7B238559175E14CED82CA0C8E74FCAD0F3E886` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueConsumerAttempt` | 5 | 0 | 0 | `BB20D9092394A0B16158DFAA755FDE8AD0DB0E3B7CE479124777D2465BDA969B` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueDelivery` | 4 | 0 | 0 | `D789634CAA3121B9BEFB83909D6B4516D57787E3F7C9B0F044883D6C080CB9F0` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCue` | 42 | 0 | 0 | `017B96D3418CE8FF0D1F96BFE6AD3106E71BE39C650BAC6E966F5962BE2E7589` |
| `Shanmen.0_0_10.Product.SwordRhythmProductSession` | 1 | 0 | 0 | `8483E5E13BC1619CD420C67B68593859C46BE661075F3F699C16891A09B502F5` |
| `Shanmen.0_0_10.Product.SwordRhythmPresentation` | 4 | 0 | 0 | `F5A45D403AB98A59D0C297411C80AA0AAB7A5B041971BFAAFCF83561A715FCA9` |
| `Shanmen.0_0_10.Product.SwordRhythmEvaluationRoute` | 1 | 0 | 0 | `7C9BD502D85094C59550C978F83CCD091BADC8E3A1476444ABDCD6032B7DE3E1` |
| `Shanmen.0_0_10` | 608 | 0 | 0 | `1A873982581966252E90C1FE28F279651680B9F473D572EAAB1A64219C9E3E1E` |

EffectCue 父前缀由 `37` 增加至 `42`；全量由 `603` 增加至 `608`。十三份日志均有唯一选定 `RunTests` group、native terminal marker、Fail `0`；最后一个选定命令之后 automation error/Fatal/Unhandled/Ensure 为 0。

## 7. 静态与回归门禁

```text
REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=20 Logs=13
SELF_TEST: PASS 200/200
ROUTER_TRANSACTION_SCAN: PASS exactly one Router.TryRouteProcessNext call
TRANSACTION_BYPASS_SCAN: PASS
RUNTIME_BOUNDARY_SCAN: PASS World/Actor/assets/timer/async/thread/RNG absent
REFLECTION_SCAN: PASS
AUTHORITY_SCAN: PASS ProductSession GameMode CombatRunCoordinator unchanged
JSON_PARSE: PASS
TRAILING_WHITESPACE_SCAN: PASS
git diff --check: PASS (native exit 0)
```

## 8. 构建证据

命令：`Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`。

| Target | Result | Actions / total time | Exit |
|---|---|---|---:|
| Editor initial | Succeeded | 5 / 30.96s | 0 |
| Game final | Succeeded | 4 / 23.87s | 0 |
| Editor final | Succeeded, up to date | 0 / 0.93s | 0 |

最终产物：

- `UnrealEditor-demo_map.dll`：13,557,248 bytes，SHA-256 `EB12B2577D85E2C7BD4A1F67BED25023B1E0B695005C55B4AB438E1D370FF9FA`；
- `demo_map.exe`：355,134,976 bytes，SHA-256 `C9D4E96F9D313E20AF51355D41F486887C4584E0EEF6A9C3C3B80E3969B5C5F6`。

构建未出现源码失败、C3859、C1076、系统代码 1455 或其它 commit-memory/page-file 环境错误。

## 9. 流程与异常

focused、回归、自检、静态门禁与三次构建均首次成功，最终原生命令退出码均为 `0`。没有源码、Automation、映射或构建失败需要重试。

UE 启动阶段仍有选定 `RunTests` 之前的既有 13 条 automation condition diagnostics；选定阶段 error/Fatal/Unhandled/Ensure 为 0，未把启动噪声描述为本阶段失败或成功证据。

raw Automation 与 build 日志仅作为本地可复核证据，不纳入 Git；Git 提交本 Report/Log 与精确源码/流程文件。

## 10. P/F 边界与下一步

本 Report 只包含 P 阶段 command transport/sequence host、注入 fake executor、静态审查、无头 Automation、changed-file 回归与 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、实际动画/声音/VFX、Cook 或 Package。

建议 P12.20 停止继续叠加 ledger wrapper，转向一个显式 product integration seam：由现有 SwordRhythm ProductSession 产生的 immutable EffectCue event batch，经 caller-owned command/envelope factory 交给本 Host；仍由上层拥有 HostId、sequence、executor 与调用时机，并先证明不会新增 event/session/transport 权威。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p12-19-sword-rhythm-cue-execution-command-host>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-19-sword-rhythm-cue-execution-command-host/Docs/Report/Dev.D.UE.0.0.10.P12.19.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-19-sword-rhythm-cue-execution-command-host/Docs/Log/Dev.D.UE.0.0.10.P12.19.r0_log.md>
