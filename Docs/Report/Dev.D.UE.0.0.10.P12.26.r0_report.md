# Dev.D.UE.0.0.10.P12.26.r0 Report

## 1. 结论

P12.26 已完成并通过 P 阶段门禁。

本阶段在 P12.25 immutable prepared dispatch 之上补齐 caller-owned retry-attempt renewal：初次 ProcessNext 一旦形成 durable retry-pending receipt，调用方可显式冻结下一次 continuation；新值固定使用下一个 Host sequence 与新 CommandId，只为仍 pending 的 Visual/Audio consumer 派生新 AttemptId，已 acknowledged consumer 则逐字保留上一 Process 的 AttemptId 与既有 receipt。

最终结果：

- retry preparation 是 read-only：不改 Host、不调用 executor、不推进 sequence；
- retry execution 最多路由一次 frozen ProcessNext，只有 batch complete 才再路由一次 frozen End；
- Audio-only、Visual-only、Visual+Audio 三种 pending 组合均得到 role-isolated deterministic attempt identity；
- 已成功 sibling 不重新进入 executor；retryable sibling 每个新 caller seed 只增加一次 executor invocation；
- 同一 pending retry exact replay 只返回 durable receipt，不调用 executor、不偷偷 End；
- 第二次 retry 从最新 retry-pending Process 继续，旧 continuation 在 Host 已推进/terminal 后 fail closed；
- Process 已持久化、End 尚未发送时，可重放 Process receipt 并只补 End；terminal exact replay 两步均无副作用；
- 无后台队列、自动重试、轮询、计时器、线程、executor/Host ownership 或 live ProductSession 重读；
- focused prepared retry `6/6`，EffectCue 父前缀 `78/78`，0.0.10 全量 `644/644`，Fail `0`；
- changed-file gate：`Changed=5 / Rules=1 / Required=27 / Logs=20`；
- regression gate self-test `214/214`；
- staged `git diff --check`、静态边界扫描、Editor/Game Development 单并发构建全部通过。

## 2. 功能性

### 2.1 Immutable prepared retry value

`Fdemo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetry` 私有保存：

- P12.25 frozen prepared dispatch root；
- caller-owned retry seed；
- 本轮 renewed channel 集合；
- 产生 retry-pending 的 exact source Process envelope；
- next-sequence retry Process envelope；
- retry 完成后唯一允许的 End envelope。

`IsValid` 从 prepared projection/plan、source dispatch identity 与 retry seed 重建 Process/End command、attempt 和 envelope，并 deep-match 保存值。source sequence 1 必须精确匹配 P12.21 初始 Process；后续 source 仍必须保持同 Host/Run/Batch root。

### 2.2 PrepareRetry

`PrepareRetry` 只读取 caller 提供的 Host durable evidence：

1. 重建 prepared dispatch 的初始 transaction request；
2. 核验 Host sequence 0 Create 与 sequence 1 Process 精确属于该 root；
3. 要求最新 durable record 是成功记录的 `ProcessNext + Session RetryPending`；
4. 从 Visual/Audio acknowledgement receipt 判断 pending channel；
5. acknowledged channel 复用 source AttemptId，pending channel按 role 派生新 AttemptId；
6. 在 source sequence 后冻结一个 Process 与一个条件 End。

重复使用同一 source/seed 会得到完全一致的 prepared retry；不同 seed 或不同 source receipt 会得到不同 Process command/attempt identity。准备过程没有 Host mutation 或 executor invocation。

### 2.3 TryExecutePreparedRetry

执行前只接受三种 Host fence：

- `Fresh`：Host 正停在 source 后，retry Process 尚未路由；
- `ProcessCommitted`：retry Process 已有 exact durable record，End 尚未路由；
- `Completed`：retry Process 与 End 均已有 exact terminal record。

一次调用先尝试 exact retry Process。若仍 retry pending，立即返回且不路由 End；若 batch complete，再路由 exact End。新执行返回 `Completed`，Process receipt replay + 新 End 返回 `Resumed`，Process/End 全部 exact replay 返回 `Replayed`。foreign root、stale continuation、sequence drift、invalid value 与 completed Host 上的新 preparation均在 executor 副作用前拒绝。

### 2.4 成功 sibling 证据保留

初次 Visual 成功、Audio retryable 时，retry command 的 VisualAttemptId 与初始计划完全相同，AudioAttemptId 更新；反向情况同理。ExecutionDriver/Adapter 从 coordinator receipt 重放已 acknowledged sibling，因此其 executor invocation count 保持 `1`。若两个 channel 都 pending，则两者各派生不同的新 AttemptId 并各执行一次。

## 3. 完整性

新增六个 focused Automation contract：

1. `RenewAudioPreserveVisual`：只更新 Audio attempt，Visual receipt 保留且 executor 不重入；
2. `RepeatPendingAndReplay`：第一次 retry 继续 pending、exact replay 惰性、第二 caller seed 再续订并完成、旧值 stale fence；
3. `DeterministicVisualRenewal`：同 seed 重现相同值、异 seed 隔离、只更新 Visual attempt；
4. `ResumeEndAfterDurableProcess`：模拟 Process 已提交后恢复，只补 End，不重入 executor；
5. `RenewBothPendingChannels`：Visual/Audio 同时 pending 时派生两个不同 attempt 并一次完成；
6. `InvalidForeignAndTerminalFences`：默认无效值、空 seed、foreign prepared root 与 terminal Host 均 fail closed。

测试使用真实 `CombatRunCoordinator + fixed timeline + SwordRhythmProductSession + evaluation/presentation/effect-cue + prepared dispatch + transaction/Host` 链。fake executor 只提供既有 opaque receipt，不伪造 Host、route、session、acknowledgement、retry 或 replay evidence。

## 4. 权威与兼容性边界

- P12.25 prepared dispatch、P12.21 transaction、ProductRoute、CommandHost/Router、ExecutionSession/Host 与 ConsumerCoordinator 继续是唯一状态权威；
- 新层只追加普通 C++ immutable value/stateless service，不修改既有 public API 或历史三记录 transaction 定义；
- source sequence 1 不被改写；恢复严格使用 sequence 2+ 的 fresh Process command，符合 P12.19 Host contiguous append 契约；
- production 新层无 reflection、World/Actor/UObject、资产加载/播放、spawn、timer、async/thread、RNG、循环、damage、attribute 或 gameplay-effect application；
- 生产执行文件中仅一处 `Host.TryRouteProcessNext`，且只有 batch complete 后一处 `Host.TryRoute(End)`；
- 没有 ProductSession、ProductHost、GameMode 或 CombatRunCoordinator 修改；
- 长期未跟踪的 0.0.9B Prompt、Report、CSEMI、PDF 与用户资料未修改、未暂存、未提交。

## 5. 修改范围

- `demo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetry.h/.cpp`：新增 retry seed/channel、immutable continuation、prepare/execute result 与 stateless service；
- `demo_mapShanmenSwordRhythmEffectCueExecutionProductPreparedRetryTests.cpp`：新增六个续订、保留、重放、恢复和 fence contract；
- `ShanmenRegressionMap.json`：新增 prepared-retry 路径规则与 27 个 required groups；
- `Test-ShanmenRegressionCoverageSelfTest.ps1`：新增完整证据正例与 focused-only 缺证据反例；
- Report/Log 生成前 5 个代码/流程文件净变更 `+1470 / -0`。

## 6. Automation 证据

| Group | Success | Fail | Exit | SHA-256 |
|---|---:|---:|---:|---|
| `ProductPreparedRetry` | 6 | 0 | 0 | `C78E68837703942E26ADB1F67157BC2A416910C4F2A32C56217C8FA27D68424B` |
| `ProductPreparedDispatch` | 5 | 0 | 0 | `401B9C493E2267D86A0DE20ECE300078709A2F1FA53373D724F14931FA3CA4D9` |
| `ProductPlannedDispatch` | 5 | 0 | 0 | `C682F95D3BC960FCE40ABB639388FDBD68E38BDC78FBEB3BCDE1B3FAC6791A97` |
| `ProductDispatchPlan` | 5 | 0 | 0 | `AFAA93430AB1720EF9DB550D14FAC1FD6A9B930C6F2EDB4C5CFE8E5B49C4BACB` |
| `ProductDispatch` | 10 | 0 | 0 | `87FE2DE5B8BC8769DF3628E1BE59583D302F004A42D659156E772C3AEAB89AD7` |
| `ProductTransaction` | 5 | 0 | 0 | `1CF6F161467C8199E753445FD38C64A6161E83BDC16F823024077A0263FD714F` |
| `ProductRoute` | 5 | 0 | 0 | `32F32188B119588527A64C1E89FEFE15FDF6E0AA97DE39685BDB94DE46978B60` |
| `CommandHost` | 5 | 0 | 0 | `41A50658479F6DA30EED4B4C0CF8504140DEAC6AD509191DAA9F8E521406A71D` |
| `CommandRouter` | 5 | 0 | 0 | `C5CE57959ED33C918FBC8A53D2A7B4016E73BD469EB410AADDAF618578B80D53` |
| `ExecutionSession` | 5 | 0 | 0 | `6B43BD8A33F05E540F0416B114CA5B76E404BB9CA27F2206BCABBAF6C81E7664` |
| `ExecutionHost` | 5 | 0 | 0 | `6030CB02F53FA94AFE0AAD2AF184F94C43AA7B8235565A5C4854CC21930090F0` |
| `ExecutionDriver` | 5 | 0 | 0 | `D737FAF60D52EEA8DA16C95651C088D935C51802D8FBDF175A40A2C3BC0B1FF2` |
| `ExecutorAdapter` | 5 | 0 | 0 | `7F9616AABC8B5508BCE81D46E8DBA897A7C6C5255D66CD8F3BFA9BD269B1A31F` |
| `ConsumerAttempt` | 5 | 0 | 0 | `7300E82501BA570741076F2D2125714E3839271E1A065AE7866AA6D261115ECA` |
| `Delivery` | 4 | 0 | 0 | `05B6FCD22A4C69E05CA896C1419480BF734BC9C10B991944A95C89F1527AC147` |
| `EffectCue` | 78 | 0 | 0 | `E03D730FDD4DBC474E92DC79E619816F88237B50F5AD7B50BCA178C3C743BA8E` |
| `Presentation` | 4 | 0 | 0 | `6A63A6D8926B629DA0EDEFDD36074F28527D498B922A65140C29C27BCCAD21A1` |
| `ProductSession` | 1 | 0 | 0 | `8C6E670F2931C7717C41AF4D55E4C1040C0ED142637002CD42FFB735586D4560` |
| `EvaluationRoute` | 1 | 0 | 0 | `B6A534047B6F3565F7D89A2A4B9696E3CE11E106F72E135432474C97A5094D24` |
| `Shanmen.0_0_10` | 644 | 0 | 0 | `54730F9D4B2A22ABFFC6DF0509357C0FB5825E0A4574968B2DA034AE9CFDC586` |

二十份最终日志均有唯一 selected `RunTests` group、唯一 terminal marker、Fail `0`；selected phase error/Fatal/Unhandled/Ensure 为 `0`。EffectCue 从 `72` 增至 `78`，全量从 `638` 增至 `644`。

## 7. 静态与回归门禁

```text
REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=27 Logs=20
SELF_TEST: PASS 214/214
PREPARE_BOUNDARY: PASS Host mutation=0 Executor invocation=0
EXECUTION_CALLS: PASS TryRouteProcessNext=1 Conditional End=1
RUNTIME_BOUNDARY_SCAN: PASS
AUTHORITY_SCAN: PASS ProductSession ProductHost GameMode CombatRunCoordinator unchanged
JSON_PARSE: PASS
git diff --cached --check: PASS (native exit 0)
```

## 8. 构建证据

有效命令：`Build.bat <Target> Win64 Development demo_map.uproject -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`。

| Target | Result | Actions / total time | Exit |
|---|---|---|---:|
| Editor initial | Succeeded | 5 / 32.29s | 0 |
| Game final | Succeeded | 4 / 23.45s | 0 |
| Editor final | Succeeded, up to date | 0 / 0.89s | 0 |

最终产物：

- `UnrealEditor-demo_map.dll`：13,786,624 bytes，SHA-256 `869E9C924C36EC3425518A1A58E5DCA34A012D8DF2A97355F9BD84E3E346DA6A`；
- `demo_map.exe`：355,318,272 bytes，SHA-256 `45DBFB62E2F4C4912EE7BD8D644423575BB2CF508EC2B35B6F398CD5CCC15EC0`。

所有有效构建均原生退出 `0`，未出现源码失败、C3859、C1076、系统代码 1455 或其它 commit-memory/page-file 环境错误。

## 9. 流程与异常

首次 Editor compile 即成功；首次 focused Automation 即 `6 Success / 0 Fail`，原生退出 `0`，SHA `7C48C71DBC445234BAB85EB96812AD30703242F5ABDC188967CF4C9B741CBB9F`。本轮没有源码、测试或构建失败。

UE 启动阶段仍有 selected `RunTests` 之前的既有 13 条 automation condition diagnostics；最终 20 份日志 selected phase 均无 error/Fatal/Unhandled/Ensure。跨平台 SDK 探测仍提示 LinuxArm64/VisionOS 缺少 `MainVersion`，但 Win64 SDK 为 VALID。raw Automation/build 日志仅本地保留，不纳入 Git。

## 10. P/F 边界与下一步

本 Report 只包含 P 阶段 caller-owned immutable retry continuation、deterministic identity derivation、injected fake executor、静态审查、NullRHI 无头 Automation、changed-file 回归与 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、实际动画/声音/VFX、Cook 或 Package。

建议 P12.27 增加 caller-owned retry decision value：由显式 attempt budget 与最新 durable receipt 纯函数决定 `Retry / Stop`，只生成下一份 prepared retry 或终止证据；仍不引入 scheduler、后台循环或 executor ownership。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p12-26-sword-rhythm-cue-execution-prepared-retry>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-26-sword-rhythm-cue-execution-prepared-retry/Docs/Report/Dev.D.UE.0.0.10.P12.26.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-26-sword-rhythm-cue-execution-prepared-retry/Docs/Log/Dev.D.UE.0.0.10.P12.26.r0_log.md>
