# Dev.D.UE.0.0.10.P12.21.r0 Report

## 1. 结论

P12.21 已完成并通过 P 阶段门禁。

本阶段在 P12.20 frozen Product projection/Create route 与 P12.19 caller-owned command Host 之上，增加一个“单 projection、单次同步、完整生命周期”的 product transaction：调用方在执行前显式提供 Host、三条 command identity、固定 sequence `0/1/2`、双 consumer、双 attempt 与双 executor；transaction 依次尝试 Create → ProcessNext → End，并返回完整冻结证据。

最终结果：

- transaction request 在任何 Host mutation 前冻结 Create、ProcessNext、End 三个 command/envelope；
- request 只接受一个有效 Product projection、固定 sequence `0/1/2`、三条不同 CommandId、不同 consumer 与不同 attempt；
- request 自验证会重建 ProcessNext/End command 与 envelope 并 deep-match，不能只靠表面 GUID 通过；
- transaction 只对 caller-owned Host 分别派发一次 Create、ProcessNext、End；无循环、无自动 retry、无 executor/Host/sequence 所有权；
- fresh Host 完成后形成严格三条 record 的 terminal proof；
- 已有 exact Create 时从 durable evidence 继续，结果标记 `Resumed`；
- terminal exact replay 三条命令全部历史重放，不重新进入 executor；
- retry/reject 在 Process record 耐久化后立即停止，不发送 End；再次提交同一 request 只重放原 receipt，不暗中恢复；
- focused transaction `5/5`，EffectCue 父前缀 `52/52`，0.0.10 全量 `618/618`，Fail `0`；
- changed-file gate：`Changed=5 / Rules=1 / Required=22 / Logs=15`；
- regression gate self-test `204/204`；
- `git diff --check`、静态所有权扫描、Editor/Game Development 单并发构建全部通过。

## 2. 功能性

### 2.1 全身份预捕获

`ProductTransactionCapture` 由调用方提供：

- HostId；
- Create/Process/End sequence，严格为 `0/1/2`；
- 三条互不相同的 CommandId；
- Visual/Audio consumer identity；
- Visual/Audio attempt identity。

factory 先通过 P12.20 生成单 projection Create request，再从该 Create command 的确定性 RunId/BatchId 生成 P12.18 ProcessNext 与 End command，最后经 P12.19 捕获两个 envelope。三个操作在 dispatch 前全部固定，不读取未来 Host 状态，也不运行 executor。

### 2.2 有界同步事务

`ProductTransaction::TryExecute` 接收 frozen request、caller-owned Host 与两个显式注入 executor。执行路径只有：

1. `ProductRoute::TryCreate`；
2. `Host.TryRouteProcessNext`；
3. 仅当单 event batch 确实完成时执行 `Host.TryRoute(End)`。

任一步拒绝都会立即返回。ProcessNext 若产生 `RetryPending`，transaction 保留 Host 已提交的 sequence-1 receipt 并停止；它不生成新 CommandId/AttemptId，不循环，也不把失败解释为可自行恢复。

### 2.3 durable resume 与 replay

Host 历史 record 是唯一 replay 权威：

- 空 Host：三条 operation 都是首次提交，结果为 `Completed`；
- Host 已有 exact Create：Create 历史重放，Process/End 首次提交，结果为 `Resumed`；
- Host 已 terminal：三条 exact envelope 全部历史重放，结果为 `Replayed`，executor invocation count 不增加；
- Process retry/reject：Host 固定在两条 record、next sequence `2`、非 terminal；exact request 重放同一 Process receipt，仍不执行 End。

成功结果还会核对 HostId、RunId、BatchId、record count `3`、next sequence `3`、terminal 状态，以及三个历史 envelope 与 request 的逐条 deep-match。

## 3. 完整性

新增五个 focused Automation contract：

1. `CompletedLifecycle`：真实 Product projection 从空 Host 完成三命令事务，双 executor 各进入一次；
2. `TerminalReplay`：完成后 exact request 三条全 replay，executor invocation 与 record count 不增加；
3. `ResumeFromCreate`：预先提交 exact Create，事务从其 durable evidence 完成 Process/End；
4. `RetryAndRejectFence`：Audio retry 与 Visual reject 都只形成两条 durable record；exact replay 不重新进入 executor、不发送 End；
5. `CaptureAndForeignFence`：错误 sequence、CommandId/consumer/attempt alias、空 projection 全部 fail closed；确定性重复 capture deep-match；foreign sequence-zero Host 在 executor 前拒绝。

测试 projection 来自真实 `CombatRunCoordinator + fixed timeline + SwordRhythmProductSession + evaluation/presentation/effect-cue` 链，并要求 event 同时含 Visual/Audio commands。fake executor 只实现既有注入边界，用于返回成功、retryable receipt 或显式 reject，不伪造 transaction、command、envelope、Host record 或 replay evidence。

## 4. 权威与兼容性边界

- P12.1—P12.20 的 ProductSession、ProductRoute、presentation、EffectCue、delivery、attempt、executor、Host、session、router 与 command Host 均未修改；
- GameMode、CombatRunCoordinator、fixed timeline 与 runtime action authority 均未修改；
- request 是 caller-owned C++ value，transaction 是 stateless class；不是 UObject、Subsystem、World service、registry、worker 或 retry scheduler；
- transaction 不持有 ProductSession、projection source、Host、executor、sequence allocator、attempt allocator、timer、thread 或 background task；
- production 文件无 reflection、World/Actor、资产加载/播放、spawn、timer、async/thread、RNG、damage、attribute 或 gameplay-effect application；
- 本阶段故意只支持一个 projection。多 event batch 仍由既有 P12.20 caller-owned ordered batch 与 P12.19 Host 显式管理，不在 transaction 内增加隐藏循环；
- 长期未跟踪的 0.0.9B Prompt、Report、CSEMI、PDF 与用户资料未修改、未暂存、未提交。

## 5. 修改范围

- `demo_mapShanmenSwordRhythmEffectCueExecutionProductTransaction.h/.cpp`：全身份 capture、冻结 request、单 projection transaction 与 evidence result；
- `demo_mapShanmenSwordRhythmEffectCueExecutionProductTransactionTests.cpp`：五个真实 source-to-terminal contract；
- `ShanmenRegressionMap.json`：新 transaction 路径映射到 22 组完整生命周期证据；
- `Test-ShanmenRegressionCoverageSelfTest.ps1`：新增 transaction 完整正例与 focused-only 反例；
- Report/Log 生成前 5 个代码/流程文件净变更 `+1131 / -0`。

## 6. Automation 证据

| Group | Success | Fail | Native exit | SHA-256 |
|---|---:|---:|---:|---|
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductTransaction` | 5 | 0 | 0 | `8505E6A9F839897E50451EA6F56EE60EC976B4E4AAA6A1853B2A475FC58D5EE4` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionProductRoute` | 5 | 0 | 0 | `08E4DD9D2BAF93D675D0DF2E88D0449199C1D89014F6A075CCF0873E34FEFD0F` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionCommandHost` | 5 | 0 | 0 | `0F96C9E8B28F8BD1B0160604728CF124BE8281E4C8081720D6B6D6C2A9036F9A` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionCommandRouter` | 5 | 0 | 0 | `B4670CCFB7EBEF70E809DC2E13EE466D901223437CF1B68761B0C84AD3A69835` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionSession` | 5 | 0 | 0 | `B788DEE51B26982AB274D7117466A8B1037A7EF858DAFFF23C97D4EC6B873ACC` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionHost` | 5 | 0 | 0 | `52E811CF0A8CECC3740268A0BA2B6C3C33791F3F386ED00D3DEB3457D3FF1B2D` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionDriver` | 5 | 0 | 0 | `6519DE947F3F47B45FECB8F1DE0BEEFB15865FB1B001F41DF7BCF702EC58DF6B` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutorAdapter` | 5 | 0 | 0 | `54EDD32FD8C61FBB87FC77B7B5CC20F6ACEB19DFA6CCCFF69C2B828E084BEC3C` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueConsumerAttempt` | 5 | 0 | 0 | `BA0F2C79109963E983349C31EEF3DE87A1F85B4A8F69AFD79266F6EB0C1F72A7` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueDelivery` | 4 | 0 | 0 | `1BA3FA28B11F9F38923A3AF1FCF10264A2AB7C3EB9F89AAFA84F6473F1694619` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCue` | 52 | 0 | 0 | `62C4243DB357D31DFE46A85071E1DDB66640B33694965E9BD2F5B166E8E26635` |
| `Shanmen.0_0_10.Product.SwordRhythmPresentation` | 4 | 0 | 0 | `813A0F407A5909301B3953DFFC848369FCBCC8F512E74B941EC7EEEAB1D96D22` |
| `Shanmen.0_0_10.Product.SwordRhythmProductSession` | 1 | 0 | 0 | `9D7394753F4940FE5285EEED7F40F63630BE76BE14C66C2AF1261947D09ED519` |
| `Shanmen.0_0_10.Product.SwordRhythmEvaluationRoute` | 1 | 0 | 0 | `4E8E43C2C03DFBD67C7F9FCC771757C0FD1F60F7E5C47969B81C4596996257F3` |
| `Shanmen.0_0_10` | 618 | 0 | 0 | `E393703969C396E5ED0B16964912DAE1FA6D5ABEA4C3E6247B56C11DC84B2842` |

EffectCue 父前缀由 `47` 增加至 `52`；全量由 `613` 增加至 `618`。十五份最终日志均有唯一选定 `RunTests` group、native terminal marker、Fail `0`；最后一个选定命令之后 automation error/Fatal/Unhandled/Ensure 为 0。

## 7. 静态与回归门禁

```text
REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=22 Logs=15
SELF_TEST: PASS 204/204
TRANSACTION_CALL_SCAN: PASS Create=1 ProcessNext=1 End=1
AUTO_RETRY_LOOP_SCAN: PASS loops=0
RUNTIME_BOUNDARY_SCAN: PASS World/Actor/assets/timer/async/thread/RNG absent
REFLECTION_SCAN: PASS
AUTHORITY_SCAN: PASS ProductSession ProductHost GameMode CombatRunCoordinator unchanged
JSON_PARSE: PASS
TRAILING_WHITESPACE_SCAN: PASS
git diff --check: PASS (native exit 0)
```

## 8. 构建证据

有效命令：`Build.bat <Target> Win64 Development demo_map.uproject -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`。

| Target | Result | Actions / total time | Exit |
|---|---|---|---:|
| Invalid preflight target/path | Failed before source compilation | 0 / 0.06s | 1 |
| Editor initial, corrected | Succeeded | 5 / 27.49s | 0 |
| Game final | Succeeded | 4 / 24.41s | 0 |
| Editor final | Succeeded, up to date | 0 / 0.93s | 0 |

首次预检误用了不存在的 `Dev.D.UE.0.0.9BEditor` target 与 `Dev.D.UE.0.0.9B.uproject` 路径，UBT 明确报 `Unable to find project file` 并以 `OtherCompilationError / 1` 退出；恢复仓库实际 `demo_mapEditor + demo_map.uproject` 后进入源码构建并成功。该失败不是源码、内存或环境编译失败，未被隐瞒或重标为成功。

最终产物：

- `UnrealEditor-demo_map.dll`：13,634,048 bytes，SHA-256 `326F9345924CF410CFEFEA999C9CA599106C3F7D68E3B0117B3D594FD8C3B462`；
- `demo_map.exe`：355,194,880 bytes，SHA-256 `4EFBCCDCE67BD2A31D783AB52286AA9FE3511DF0A3C00857B2FC0968F0375E9F`。

有效构建未出现源码失败、C3859、C1076、系统代码 1455 或其它 commit-memory/page-file 环境错误。

## 9. 流程与异常

首次 focused Automation 即为 `5 Success / 0 Fail`、原生退出码 `0`；没有产品代码或测试修正轮。最终 focused 日志与首次 focused 日志都保留在本地，最终证据采用源码/映射均封定后的 `P12.21-ExecutionProductTransaction-final.log`。

UE 启动阶段仍有选定 `RunTests` 之前的既有 13 条 automation condition diagnostics；选定阶段 error/Fatal/Unhandled/Ensure 为 0。跨平台 SDK 探测会提示 LinuxArm64/VisionOS 缺少 `MainVersion`，但同一探测明确 Win64 SDK 为 VALID，且十五个 Windows 自动化进程均原生退出 `0`。raw Automation 与 build 日志仅作为本地可复核证据，不纳入 Git。

## 10. P/F 边界与下一步

本 Report 只包含 P 阶段 frozen transaction、注入 fake executor、静态审查、NullRHI 无头 Automation、changed-file 回归与 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、实际动画/声音/VFX、Cook 或 Package。

建议 P12.22 增加最薄的 caller-owned live dispatch seam：从 ProductSession 当前 projection 接收外部提供的 transaction identity、Host 与真实 Visual/Audio executor adapter，调用本阶段 transaction 并返回证据；仍不在 seam 内生成 identity、保存 Host、自动 retry 或直接触碰 World/资产。真实播放与输入验证继续留到 F 阶段。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p12-21-sword-rhythm-cue-execution-product-transaction>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-21-sword-rhythm-cue-execution-product-transaction/Docs/Report/Dev.D.UE.0.0.10.P12.21.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-21-sword-rhythm-cue-execution-product-transaction/Docs/Log/Dev.D.UE.0.0.10.P12.21.r0_log.md>
