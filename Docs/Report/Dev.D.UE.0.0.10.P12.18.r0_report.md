# Dev.D.UE.0.0.10.P12.18.r0 Report

## 1. 结论

P12.18 已完成并通过 P 阶段门禁。

本阶段在 P12.17 caller-owned execution session 之上增加 typed execution command router。router 接受冻结的 `Create / ProcessNext / End` command，以 `CommandId + RunId + BatchId` 锁定 payload identity，使用候选状态原子提交 session 与 durable receipt；exact replay 只返回首次 receipt，不重新进入 session 或 executor。

最终结果：

- `Create` 冻结 ordered event batch 与 Visual/Audio consumer identity，并创建唯一 batch lifecycle；
- `ProcessNext` 只通过 P12.17 session 处理当前 event，executor 必须由调用方在该次路由显式注入；
- `End` 只在 matching Run/Batch 且 session complete 时关闭 lifecycle；
- 同一 `CommandId` 的 exact payload 返回 stable replay，不增加 record、不调用 executor；
- 同一 `CommandId` 携带不同 attempt、consumer 或 batch payload 时在副作用前返回 `CommandIdConflict`；
- retry receipt 与 partial rejection receipt 都 durable，因为 sibling acknowledgement 可能已提交；恢复必须使用 fresh CommandId 与 fresh failed-channel attempt；
- 错误路由重载、pre-create process、second create、foreign identity、early end 与 post-end process 均 fail closed；
- 已结束 lifecycle 仍可 replay 历史 Create/Process/End receipt，但不能接受新 Process；
- focused router `5/5`，EffectCue 父前缀 `37/37`，0.0.10 全量 `603/603`，Fail `0`；
- changed-file gate：`Changed=5 / Rules=1 / Required=19 / Logs=12`；
- regression gate self-test `198/198`；
- staged `git diff --check`、静态边界扫描、Editor/Game Development 单并发构建全部通过。

## 2. 功能性

### 2.1 Frozen typed command

`Fdemo_mapShanmenSwordRhythmEffectCueExecutionCommand` 提供三个显式 capture seam：

- `TryCaptureCreate`：验证并复制 event batch、consumer identities，同时复算 canonical Run/Batch identity；
- `TryCaptureProcessNext`：冻结 Run/Batch identity 与双 caller-owned attempt identity；
- `TryCaptureEnd`：冻结目标 Run/Batch identity。

命令字段私有，`IsValid` 会复核 kind 专属 payload；`Matches` 对 Create 执行 ordered event deep match，对 ProcessNext 比较双 attempt identity。不存在 string opcode、可变 Blueprint payload 或隐式默认 operation。

### 2.2 Durable receipt 与 replay

router 为每个已提交 command 保存 frozen command 与首次 result。再次收到相同 `CommandId` 时：

- payload 完全一致：返回原 receipt，标记 `bReplay=true`，不进入 session；
- payload 不一致：返回 `CommandIdConflict`，不改变 router/session/executor；
- replay 不新增 record，原 durable result 保持唯一。

Create、成功/RetryPending Process、成功 End 都是 durable。`EventRejected` 也会保存，因为另一 channel 可能已完成 acknowledgement；early End rejection 同样保存为稳定 lifecycle receipt，完成后必须使用 fresh End command。

### 2.3 Atomic session routing

所有有效新命令先在 router candidate 上执行：

- Create candidate 构造 P12.17 session 并绑定 Run/Batch/total count；
- Process candidate 精确调用一次 `Session.ProcessNext`；
- End candidate 调用 guarded `Session.TryEnd`。

只有 result、record 与 candidate 全部满足不变量时才整体提交。Process 不直接调用 host、driver、consumer coordinator 或 executor adapter，因此 P12.17 仍是唯一 batch-progress authority。

### 2.4 Lifecycle 与 executor context

Create/End 必须走 executor-free `TryRoute`；ProcessNext 必须走显式双 executor `TryRouteProcessNext`。错误 overload 在 record 查找和执行前拒绝。

router 只拥有 session、Run/Batch binding、total count、ended fence 与 command records。它不保存 executor，不拥有 ProductSession、World、资产、timer、线程，也不自动发现命令、tick、poll、retry 或 teardown。

## 3. 完整性

新增五个 focused Automation contract：

1. `CreateProcessEnd`：真实三 event batch、Create replay、逐 command 单格推进、End、结束后历史 Process/End replay；
2. `RetryReplayRecovery`：Audio retry durable、exact command executor-free、attempt payload conflict、fresh command 仅恢复 Audio；
3. `RejectReplayRecovery`：Visual reject 与 Audio acknowledgement 同时保留，exact rejection replay，fresh command 仅恢复 Visual；
4. `IdentityLifecycleFence`：pre-create process、foreign Run、second create、durable early End、fresh End 与 post-end process fence；
5. `ValidationAndContext`：空/逆序/consumer alias/invalid attempt/invalid End capture、overload mismatch、consumer-scoped CommandId conflict 与 record inspection。

测试使用真实 `CombatRunCoordinator + fixed timeline + SwordRhythmProductSession + evaluation/presentation/effect-cue + delivery/attempt/executor-driver/host/session` 链。fake executor 仅替代既有明确注入边界，不伪造 command、batch、session、acknowledgement 或 router record。

## 4. 权威与兼容性边界

- P12.11—P12.17 的 EffectCue、delivery、attempt、executor adapter、driver、host 与 session 均未修改；
- ProductSession、GameMode、CombatRunCoordinator、fixed timeline 与 runtime action authority 均未修改；
- router 是 caller-owned 普通 C++ value owner，不是 UObject、Subsystem、World service、global registry 或后台 worker；
- production 文件无 reflection、mutable Blueprint surface、World/Actor、asset loading/playback、spawn、timer、async/thread、RNG、damage、attribute 或 gameplay-effect application；
- executor 只作为同步 Process route 参数，不被 command 或 router 保存；
- 长期未跟踪的 0.0.9B Prompt、Report、CSEMI、PDF 与用户资料未修改、未暂存、未提交。

## 5. 修改范围

- `demo_mapShanmenSwordRhythmEffectCueExecutionCommandRouter.h/.cpp`：冻结 typed commands、stable receipt、CommandId conflict、candidate commit、Run/Batch/lifecycle fence 与 replay ledger；
- `demo_mapShanmenSwordRhythmEffectCueExecutionCommandRouterTests.cpp`：五个真实 source-to-router contract；
- `ShanmenRegressionMap.json`：新 router 映射到 19 组 router/session/host/source/product/runtime evidence；
- `Test-ShanmenRegressionCoverageSelfTest.ps1`：新增 router 正例与 focused-only 反例；
- Report/Log 生成前 5 个代码/流程文件净变更 `+1535 / -0`。

## 6. Automation 证据

| Group | Success | Fail | Native exit | SHA-256 |
|---|---:|---:|---:|---|
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionCommandRouter` | 5 | 0 | 0 | `2544A7EA69106A972F2C81F0E0D6634744D5420F3A968038FE67FFD72CCD00BD` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionSession` | 5 | 0 | 0 | `23F8775888FEE911F295E4E3B1359CE5A913585B934D54A53526B4CF4D74DB9E` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionHost` | 5 | 0 | 0 | `BBD5E7EB14021B0A45C4067C03CD200596066C5950844617A27D6610BCB82673` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutionDriver` | 5 | 0 | 0 | `F91DC83CEDED75C93CC52DE1A8ADC544EDA52EBB14A711AF7A08E83DE907077F` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueExecutorAdapter` | 5 | 0 | 0 | `E031B13E04CCBBAA71AE53260C844B6F1B695B5A9C441216B06A9F0F41278AF7` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueConsumerAttempt` | 5 | 0 | 0 | `710C5607E0C4A3055418170ECB60C0B5312D081282969BB949B47015BE944D8A` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCueDelivery` | 4 | 0 | 0 | `2DDAAD60D89677FF3DB588B323CF19412011334877F791C286FF7CEB2128256A` |
| `Shanmen.0_0_10.Product.SwordRhythmEffectCue` | 37 | 0 | 0 | `9E92D821E85066453828A0B9FADC32584112813B33DC0472E476DAD0C3970125` |
| `Shanmen.0_0_10.Product.SwordRhythmProductSession` | 1 | 0 | 0 | `27FEF806C932DB4446955C7E03DD7A9AEB83CFC62974AB7A68725BFB7735FAC9` |
| `Shanmen.0_0_10.Product.SwordRhythmPresentation` | 4 | 0 | 0 | `09FE67FC68996CC75B71961FCCFDEA14009AD229817D826CDEC2BCD649F848E6` |
| `Shanmen.0_0_10.Product.SwordRhythmEvaluationRoute` | 1 | 0 | 0 | `651C7134917560BD78B1D4EAD61ABCFED647462C8E9B3AA1C1CCEFDCE74D45EA` |
| `Shanmen.0_0_10` | 603 | 0 | 0 | `D7A19D5E0258326039ADD5CF2EDBE2C8F218FA339430E6BBD9620F0AD44B6890` |

EffectCue 父前缀由 `32` 增加至 `37`；全量由 `598` 增加至 `603`。十二份日志均有选定 `RunTests` group、native terminal marker、Fail `0`；最后一个选定命令之后 automation error/Fatal/Unhandled/Ensure 为 0。

## 7. 静态与回归门禁

```text
REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=19 Logs=12
SELF_TEST: PASS 198/198
SESSION_TRANSACTION_SCAN: PASS exactly one Session.ProcessNext call
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
| Editor initial | Succeeded | 5 / 15.38s | 0 |
| Game final | Succeeded | 4 / 23.99s | 0 |
| Editor final | Succeeded, up to date | 0 / 0.94s | 0 |

最终产物：

- `UnrealEditor-demo_map.dll`：13,495,296 bytes，SHA-256 `9D6232DF546ADB822A9D4FF9A36C80006DC43E8A03964D2030B5384D42A02A68`；
- `demo_map.exe`：355,082,752 bytes，SHA-256 `D4C102690E5C44D38CB64B1DAF043C0A8862A2E6C1BD3537F188E9A00CB962CA`。

构建未出现源码失败、C3859、C1076、系统代码 1455 或其它 commit-memory/page-file 环境错误。

## 9. 流程与异常

focused、回归、自检、静态门禁与三次构建均首次成功，最终原生命令退出码均为 `0`。没有源码、Automation、映射或构建失败需要重试。

UE 启动阶段仍有选定 `RunTests` 之前的既有 13 条 automation condition diagnostics；选定阶段 error/Fatal/Unhandled/Ensure 为 0，未把启动噪声描述为本阶段失败或成功证据。

raw Automation 与 build 日志仅作为本地可复核证据，不纳入 Git；Git 提交本 Report/Log 与精确源码/流程文件。

## 10. P/F 边界与下一步

本 Report 只包含 P 阶段 typed command routing、注入 fake executor、静态审查、无头 Automation、changed-file 回归与 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、实际动画/声音/VFX、Cook 或 Package。

建议 P12.19：增加 Run-local execution command host，在不保存 executor、World 或资产的前提下，组合一个 router 与明确的 command-sequence/terminal fence，为上层 caller 提供单一 host seam；仍不自动 tick、poll、retry 或建立第二套 session 权威。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p12-18-sword-rhythm-cue-execution-command-router>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-18-sword-rhythm-cue-execution-command-router/Docs/Report/Dev.D.UE.0.0.10.P12.18.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-18-sword-rhythm-cue-execution-command-router/Docs/Log/Dev.D.UE.0.0.10.P12.18.r0_log.md>
