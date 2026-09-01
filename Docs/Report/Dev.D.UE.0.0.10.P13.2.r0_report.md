# Dev.D.UE.0.0.10.P13.2.r0 Report

## 1. 结论

P13.2 已完成并通过 P 阶段门禁。

本阶段为 P13.1 的 Blueprint visual/audio handoff 增加 exact ordered-batch acknowledgement。Blueprint presenter 可读取 handoff 的有序 `CommandId`，以稳定 `PresenterDefinitionId` 报告整批完成，并获得不可变、可自验证、确定性派生的 acknowledgement。RunController 与 GameMode 新增 acknowledgement-first consume 入口；缺命令、重复命令、foreign identity、错误 channel 或无效 acknowledgement 均 fail closed，不能释放 FIFO。

该 acknowledgement 只证明调用方对指定 presenter 和精确命令批次做出声明，不声称动画、VFX 或音频资产已客观播放。工程中没有可信的太极剑表现资源可绑定，本阶段未虚构 asset path，也未加载资源。

最终证据：focused `5/5`、RunController `5/5`、HandoffExecutor `5/5`、0.0.10 全量 `694/694`、四组 GameMode legacy 回归合计 `116/116`；全部 selected Fail `0`、Fatal/Unhandled/Ensure/AutomationControllerError `0`、原生退出 `0`；Game 与 Editor Development 构建成功。

## 2. 功能性

### 2.1 Immutable acknowledgement

新增 `Fdemo_mapShanmenSwordRhythmEffectCuePresentationAcknowledgement`：

- 冻结完整 source handoff、stable presenter identity 与 ordered completed command identities；
- 要求完成数组与 handoff 命令数量、内容和顺序严格一致；
- 以 handoff、invocation、channel、presenter 与全部 command identity 派生 `AcknowledgementId`；
- `IsValid` 重算 identity 并复核完整批次，默认值和篡改值 fail closed；
- `Matches` 支持对 source handoff 和 exact acknowledgement replay 的确定性核验。

### 2.2 Blueprint presentation adapter

新增无状态 `UBlueprintFunctionLibrary`：

- `GetOrderedCommandIds`：从有效 handoff 投影 Blueprint 必须回报的精确顺序；
- `TryAcknowledge`：以 `PresenterDefinitionId + CompletedCommandIds` 封存 acknowledgement；
- 无效 handoff、空 presenter、空/缺失/重复/foreign/reordered command 列表均清空输出并拒绝；
- adapter 不持有 World、Actor、asset、timer、async、playback 或 gameplay authority。

### 2.3 RunController 与 GameMode

RunController 新增 visual/audio typed acknowledgement consume：

- 先验证 acknowledgement 自一致性与 channel；
- 再把 exact `HandoffId` 交给既有 P13.0 executor；
- accepted/consumed replay 复用 executor evidence，不重复推进 FIFO；
- visual acknowledgement 不能释放 audio，反之亦然；
- 两个 channel 都释放后继续使用 P13.1 同步 FIFO pump。

GameMode 新增 BlueprintCallable：

- `AcknowledgeSwordRhythmVisualHandoff`；
- `AcknowledgeSwordRhythmAudioHandoff`。

旧 `ConsumeSwordRhythm*Handoff(FGuid)` 保留兼容；新 authored presentation 应优先使用 acknowledgement-first API。

## 3. 完整性

新增五个真实链 Automation contract：

1. `VisualExactBatch`：真实 visual handoff 生成 acknowledgement，只释放 visual slot；
2. `AudioExactBatch`：真实 audio handoff 生成 acknowledgement，只释放 audio slot；
3. `BatchAndChannelFences`：空 presenter、缺失/foreign/重复 command 与跨 channel consume 全部拒绝且不改状态；
4. `DeterminismAndPresenterIdentity`：同 presenter + 同批次精确重放，不同 presenter 产生不同 evidence；
5. `ReplayAndRunTeardown`：exact acknowledgement replay 幂等，teardown 清 controller 而 immutable evidence 保持自验证。

测试从真实 `CombatRunCoordinator + FixedTimeline + SwordRhythm ProductSession + PreparedDispatch + HandoffExecutor + RunController` 取得 handoff，没有构造第二套 cue authority。

## 4. 权威与兼容性边界

- ProductSession 继续拥有 rhythm observation/presentation authority；
- P12 route/driver/transaction/retry/journal/checkpoint authority 未修改；
- P13.0 handoff executor 继续拥有 accepted invocation、pending slot 与 exact consume authority；
- P13.1 RunController 继续拥有 revision FIFO；P13.2 只增加 consume 前的 caller acknowledgement gate；
- `PresenterDefinitionId` 是 authored stable identity，不是资源地址；
- acknowledgement 不授予 gameplay mutation、damage、timing、persistence 或 playback truth；
- 长期未跟踪的 0.0.9B Prompt、Report、CSEMI、PDF 与用户资料未修改、未暂存、未提交。

## 5. 修改范围

- `...PresentationAcknowledgement.h/.cpp`：新增 immutable acknowledgement 与 Blueprint function library；
- `...PresentationAcknowledgementTests.cpp`：新增五个真实链 contract；
- `...PresentationRunController.h/.cpp`：新增 typed visual/audio acknowledgement consume；
- `demo_mapGameMode.h/.cpp`：新增 Blueprint acknowledgement-first API 与审计日志；
- `ShanmenRegressionMap.json`：新增 acknowledgement changed-file 映射；
- `Test-ShanmenRegressionCoverageSelfTest.ps1`：新增完整证据正例与 focused-only 反例；
- Report/Log 前 9 个代码、测试、流程文件净变更 `+880 / -1`。

## 6. Automation 证据

| Group | Success | Fail | Exit | SHA-256 |
|---|---:|---:|---:|---|
| `SwordRhythmEffectCuePresentationAcknowledgement` | 5 | 0 | 0 | `DBA7486063BEF7D99308FA2B48FDFF8FEC03EF3C26526589F5040CC59EDBB054` |
| `SwordRhythmEffectCuePresentationRunController` | 5 | 0 | 0 | `FD928B388A32EE444FBE8BF79B49791D8FAA97CB95B0E81EEC1CDAA3DA86BC1F` |
| `SwordRhythmEffectCuePresentationHandoffExecutor` | 5 | 0 | 0 | `D0304E163DBD5D1713FA5DEC01B62F879D5F74487C651F675AB3F0A4CEA7D8B5` |
| `Shanmen.0_0_10` | 694 | 0 | 0 | `064BE37A76B9789F66ECD37488CFAFC67414D49DECBBCD8D735A13B94F991C86` |
| `demo_map.EnemySkillFramework` | 44 | 0 | 0 | `6CED06D76B531D4720249CDAAF0B61A272B5F890B3B9D5AD04901414D95F11F5` |
| `demo_map.ItemUseAndArmor` | 46 | 0 | 0 | `C47AFA21A64B0AC8303DF1A874A4D3D9766C89B5823B709422D7C8BF7B5BDC49` |
| `demo_map.V2RangedCompatibility` | 22 | 0 | 0 | `DD27C4599B9896E73DEC62DC09446572B269A583FE368CC2236E825344D079DA` |
| `demo_map.V3.Attributes` | 4 | 0 | 0 | `53D635DCBC4EB670D3A011B7A82BE483D5605890DDE35EF52B7D84EBB7AC8454` |

八份最终日志均有 Queue Empty、native exit `0`。全量从 `08:31:35.422` 到 `09:01:57.514`，约 `30m22.09s`；基线从 `689` 增至 `694`。

## 7. 静态与回归门禁

```text
REGRESSION_COVERAGE: PASS Changed=9 Rules=3 Required=64 Logs=8
SELF_TEST: PASS 234/234
ACK_BOUNDARY_SCAN: PASS ForbiddenCodeHits=0
JSON_PARSE: PASS Rules=137
git diff --check: PASS (native exit 0)
```

## 8. 构建证据

有效命令：`Build.bat <Target> Win64 Development demo_map.uproject -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`。

| Target | Result | Actions / total time | Exit |
|---|---|---|---:|
| Editor initial | Succeeded | 28 / 131.16s | 0 |
| Game final | Succeeded | 27 / 104.37s | 0 |
| Editor final | Succeeded, up to date | 0 / 0.93s | 0 |

最终产物：

- `UnrealEditor-demo_map.dll`：14,163,456 bytes，SHA-256 `3118E8704E0EB684E82E4315C4AC66C812344A3428D2FFE35BE9EDCDE646018E`；
- `demo_map.exe`：355,611,136 bytes，SHA-256 `DF753F1756C5D0A0CC0402A646743FF0C8648F3899611AA9B11974AD10D12395`。

## 9. 异常与环境记录

本阶段没有源码、UHT、focused、full、legacy、门禁或构建失败。首次 focused 即为 `5/5`。

全量 selected phase 有 45 条既有 `google.com/generate_204` 三秒探测超时和 70 条 controller large-delta；最长观察到 `91.60s`，对应测试随后仍 `Success`。selected phase Fatal/Unhandled/Ensure/AutomationControllerError 均为 `0`。命令执行前仍有固定 13 条引擎自检 `LogAutomationTest: Error: Condition failed`，不属于 selected phase。raw Automation/build 日志仅本地保留。

## 10. P/F 边界与下一步

本 Report 只包含 P 阶段 immutable acknowledgement、Blueprint adapter、Run/GameMode 接线、静态审查、NullRHI 无头 Automation、changed-file regression 与 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、实际动画/VFX/音频、Cook 或 Package。

下一步只有在存在可确认的 authored asset/catalog 时才应建立真实 cue-to-asset 绑定；否则继续增加假 resolver 或包装层没有价值。实际可见/可听播放、回调时序与用户体验验收应进入明确授权的 F 阶段。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p13-2-sword-rhythm-cue-blueprint-presentation-acknowledgement>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p13-2-sword-rhythm-cue-blueprint-presentation-acknowledgement/Docs/Report/Dev.D.UE.0.0.10.P13.2.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p13-2-sword-rhythm-cue-blueprint-presentation-acknowledgement/Docs/Log/Dev.D.UE.0.0.10.P13.2.r0_log.md>
