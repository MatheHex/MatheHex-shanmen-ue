# Dev.D.UE.0.0.10.P13.1.r0 Report

## 1. 结论

P13.1 已完成并通过 P 阶段门禁。

本阶段把 P13.0 的 concrete visual/audio handoff executors 接入真实 `Ademo_mapGameMode` SwordRhythm Run 生命周期。每次成功的 BasicSword rhythm observation 都冻结成既有 prepared-dispatch 值；调用方未消费任一 channel 时，后续 revision 保留在 controller-owned FIFO，不能覆盖在途 handoff。GameMode 向 Blueprint 暴露 visual/audio 的读取与 exact consume API，并在 Run teardown 记录和清空 presentation-local 状态。

这里的发布/消费只证明 immutable cue batch 已跨过 caller-owned presentation boundary，不表示动画、VFX 或音频资产已经播放。

最终证据：focused `5/5`、两个直接父契约各 `5/5`、0.0.10 全量 `689/689`、四组 GameMode 旧产品兼容测试合计 `116/116`；最终 Fail `0`、selected-phase Fatal/Unhandled/Ensure/AutomationError `0`、全部原生退出 `0`；Game 与 Editor Development 构建均成功。

## 2. 功能性

### 2.1 Run-scoped presentation controller

新增 `Fdemo_mapShanmenSwordRhythmEffectCuePresentationRunController`：

- 以 combat RunId 派生 deterministic dispatch seed、visual scope 与 audio scope；
- 复用既有 `PreparedDispatchService` 和 P13.0 channel-specific handoff executors，不重建 cue、route、executor 或 receipt authority；
- 要求 observation revision 严格连续，并把每个 prepared dispatch 作为不可变值加入 FIFO；
- 两个 channel 都有容量时同步 pump；任一 handoff pending 时停止，保证未消费批次不被覆盖；
- exact consume 只在两个 channel 都清空后继续 pump；consume replay 不产生额外副作用；
- Run teardown 输出 captured/published/queued、accepted invocation 与 pending 摘要，然后清空 presentation-local queue、handoff 和 replay evidence。

### 2.2 GameMode lifecycle integration

- Combat Run stale-state gate 现在包含 presentation controller；
- `SwordRhythmProductSession` 成功绑定后，同 Run 绑定 presentation controller；失败路径按既有逆序释放 session、timeline、thrown lifecycle 和 coordinator；
- 每次 `TryObserveExecutedBasicSword` 成功后调用 `TryPublishCurrent`；无 cue 的 revision 仍保持连续证据，有 cue 的 revision 发布完整 visual/audio batch；
- Run release 在 ProductSession 结束前捕获 presentation teardown summary；identity/state 冲突 fail closed 并执行本地 reset，不能遗留到下一 Run；
- release 日志新增 published 与 queued-at-teardown 计数。

### 2.3 Blueprint caller port

GameMode 新增四个 Blueprint API：

- `TryGetSwordRhythmVisualHandoff`；
- `TryGetSwordRhythmAudioHandoff`；
- `ConsumeSwordRhythmVisualHandoff`；
- `ConsumeSwordRhythmAudioHandoff`。

读取不会消费；consume 要求 exact `HandoffId`。错误、foreign 或 stale identity 不推进 FIFO。

## 3. 完整性

新增五个 focused Automation contract：

1. `FirstPublication`：真实 Run/BasicSword/ProductSession 链产生首个 cue pair；
2. `FifoBackpressure`：下一 observation revision 在 pending pair 后冻结，单 channel consume 不能推进，第二 channel consume 才 pump；
3. `IdentityAndRevisionFences`：invalid/foreign Run、duplicate current revision 与 discontinuity fail closed；
4. `ExactConsumption`：错误 identity 无状态变化，exact visual/audio consume 释放完整 pair；
5. `RunTeardown`：foreign teardown 不清状态，exact teardown 保留摘要并允许下一 Run 重新绑定。

测试使用真实 `CombatRunCoordinator + FixedTimeline + SwordRhythm ProductSession + prepared dispatch + handoff executor` 链，没有创建第二套 rhythm 或 cue 系统。

## 4. 权威与兼容性边界

- ProductSession 继续拥有 observation/presentation authority；controller 只冻结并排队既有 current revision；
- P12 Host/driver/route/transaction/dispatch/retry/journal/checkpoint authority 文件均未修改；
- P13.0 executor 继续拥有 channel validation、single-slot backpressure、handoff/receipt identity；
- controller 无 World、Actor、UObject、asset loading、timer、async、file/network、persistence 或 gameplay mutation API；
- GameMode 只接线既有 caller boundary，不把 `Succeeded` 解释为 playback completion；
- 长期未跟踪的 0.0.9B Prompt、Report、CSEMI、PDF 与用户资料未修改、未暂存、未提交。

## 5. 修改范围

- `...PresentationRunController.h/.cpp`：新增 Run-scoped deterministic FIFO controller；
- `...PresentationRunControllerTests.cpp`：新增五个真实链 contract；
- `demo_mapGameMode.h/.cpp`：Run begin/release、BasicSword publish、Blueprint get/consume 接线；
- `ShanmenRegressionMap.json`：新增 controller 路径到 27 个 source-to-runtime groups 的映射；
- `Test-ShanmenRegressionCoverageSelfTest.ps1`：新增完整证据正例和 focused-only 缺证据反例；
- Report/Log 之前七个生产/测试/流程文件净变更 `+1386 / -4`。

## 6. Automation 证据

| Group | Success | Fail | Exit | SHA-256 |
|---|---:|---:|---:|---|
| `SwordRhythmEffectCuePresentationRunController` | 5 | 0 | 0 | `56C4D4282E511B76471E467D688FB155F62B8F6C657BA38A4965EB6DC2F1ED55` |
| `SwordRhythmEffectCueExecutionProductPreparedDispatch` | 5 | 0 | 0 | `731A89B11E3323749D768E41D7A5DF9C605727E78A61CB60BFB9644A89291043` |
| `SwordRhythmEffectCuePresentationHandoffExecutor` | 5 | 0 | 0 | `90B398560BDB6B160925311BD34193D54675AC0E36F1ED2F423B4D463F357ED6` |
| `Shanmen.0_0_10` | 689 | 0 | 0 | `34879D80F80153A04EA698D333C9F90D865BFC6217C81AFAED554914749C2964` |
| `demo_map.EnemySkillFramework` | 44 | 0 | 0 | `88247BC445F2E729A67EFE9A1AF0F12A3E6848E399042EE283A4054A776185BA` |
| `demo_map.ItemUseAndArmor` | 46 | 0 | 0 | `64EE0A79C0250423BB74DDCEB1C0E7695804EAA4FA1A4047A4C694F6C1D949E0` |
| `demo_map.V2RangedCompatibility` | 22 | 0 | 0 | `5E7F22B7AD0D38F34B416162C3B12210404ED0E68419F844B461400C979BD075` |
| `demo_map.V3.Attributes` | 4 | 0 | 0 | `68727674C19C514DB2ACFD044C95EA0A852E32ED82ADCA05CA11AC55381BD378` |

八份最终 selected-phase 日志均 Queue Empty `1`、native exit `0`。全量从上一阶段 `684` 增至 `689`，selected tests 从 `07:34:05.695` 到 `08:03:58.649`，约 `29m52.95s`。

## 7. 静态与回归门禁

```text
REGRESSION_COVERAGE: PASS Changed=7 Rules=2 Required=63 Logs=8
SELF_TEST: PASS 232/232
PRESENTATION_RUN_CONTROLLER_BOUNDARY_SCAN: PASS ForbiddenCodeHits=0
AUTHORITY_SCAN: PASS existing execution/product authority files unchanged
JSON_PARSE: PASS Rules=136
git diff --check: PASS (native exit 0)
```

## 8. 构建证据

有效命令：`Build.bat <Target> Win64 Development demo_map.uproject -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`。

| Target | Result | Actions / total time | Exit |
|---|---|---|---:|
| Editor initial | Succeeded | 26 / 110.64s | 0 |
| Editor after test-fixture correction | Succeeded | 4 / 5.67s | 0 |
| Game final | Succeeded | 25 / 97.73s | 0 |
| Editor final | Succeeded, up to date | 0 / 0.99s | 0 |

最终产物：

- `UnrealEditor-demo_map.dll`：14,109,696 bytes，SHA-256 `0F573C1577DC3405740360DFA1EC8543924205DD55A656F6D02F362A7E237253`；
- `demo_map.exe`：355,575,296 bytes，SHA-256 `C98D75A7D0810816ECEC9DB0A4090F696C8A889644D10C8536C9501A52989A45`。

## 9. 首次失败与修正记录

首次 focused run 原生退出 `0`，但日志解析得到 `3 Success / 2 Fail`（SHA-256 `EAFAF63157E407FEAD3D65B6325F700968583ADF78592582E104919B321513E1`）。失败项为 `FifoBackpressure` 与 `RunTeardown`。夹具错误地要求“八次同 tick observation 内必须再次生成 cue”；产品契约只保证下一 observation revision 连续，并不保证该 revision 必有 cue。测试改为冻结一个确定的下一 revision，仍验证 pending pair、FIFO、双 channel consume 与 teardown；生产 controller 未因该失败改动。修正后 focused 为 `5/5`。

首次 regression gate 也按设计 fail closed：GameMode 改动缺少四组 legacy evidence。未放宽映射，补跑 `EnemySkillFramework`、`ItemUseAndArmor`、`V2RangedCompatibility`、`V3.Attributes` 共 `116/116` 后，门禁通过 `Required=63`。

全量 selected phase 记录 42 次既有 `google.com/generate_204` 三秒探测超时及 65 次 controller large-delta；controller 明确忽略对应无响应间隔。命令执行前仍有固定 13 条引擎自检 `LogAutomationTest: Error: Condition failed`，selected phase AutomationError 为 `0`。raw Automation/build 日志仅本地保留，不纳入 Git。

## 10. P/F 边界与下一步

本 Report 只包含 P 阶段 Run/controller 接线、纯内存 FIFO、Blueprint caller contract、静态审查、NullRHI 无头 Automation、changed-file regression 与 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、实际动画/音频/VFX、Cook 或 Package。

下一阶段建议 P13.2：在不改变 gameplay/cue authority 的前提下，为 handoff command 建立 authored presentation adapter 与可观测 completion/ack 边界；实际可见/可听播放和时序验收仍留给明确授权的 F 阶段。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p13-1-sword-rhythm-cue-game-mode-handoff-integration>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p13-1-sword-rhythm-cue-game-mode-handoff-integration/Docs/Report/Dev.D.UE.0.0.10.P13.1.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p13-1-sword-rhythm-cue-game-mode-handoff-integration/Docs/Log/Dev.D.UE.0.0.10.P13.1.r0_log.md>
