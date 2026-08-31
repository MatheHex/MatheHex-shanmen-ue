# Dev.D.UE.0.0.10.P12.19.r0 Development Log

## 1. 目标

在 P12.18 typed command router 外增加薄的 Run-local command host：冻结 caller-owned HostId、连续 Sequence 与 deterministic DispatchId，原子保存 transport receipt，并在不拥有 executor/World/ProductSession 的前提下提供 stable replay、gap/conflict 与 terminal fence。

## 2. 实现

- 新增 private-payload command envelope，capture 时复制 P12.18 command 并复算 deterministic DispatchId；
- sequence 0 只允许 Create，成功后绑定 Host/Run/Batch；
- 新 envelope 必须与 NextSequence 连续，gap、foreign Host、historical payload conflict 均 fail closed；
- exact historical envelope 返回首次 Host receipt，不进入 router/session/executor；
- router durable success、retry、partial reject 与 early-End reject 同 transport sequence 原子提交；
- fresh sequence 的旧 Router CommandId 返回 `CommandReplayConflict`，不重复计数；
- partial retry/reject 仅能用 next sequence、fresh CommandId 与 failed-channel attempt 恢复；
- successful End 建立 terminal fence，历史 receipt 保持可重放；
- wrong executor overload 在 replay/route 前拒绝；
- 无 ProductSession、World、资产、timer、线程、自动 tick、poll 或 retry ownership。

## 3. Automation

| Group | Success | Fail | Exit | SHA-256 |
|---|---:|---:|---:|---|
| `SwordRhythmEffectCueExecutionCommandHost` | 5 | 0 | 0 | `46917ADEF33E57012F45B0733E50A20753DA47095FC4CEC0ECEAD3505D7791F2` |
| `SwordRhythmEffectCueExecutionCommandRouter` | 5 | 0 | 0 | `2552A896C9D32F93253AB57809CF2AB4F25879EF8B5D6881AB9521CBB205AF26` |
| `SwordRhythmEffectCueExecutionSession` | 5 | 0 | 0 | `464F96AFBDB34B2ABAEDD8B5B2D931FD1DB3C8614DE81E23CA156601A52980A8` |
| `SwordRhythmEffectCueExecutionHost` | 5 | 0 | 0 | `BDB91E59E547A42340EB17DD272454DDE90D0241932BE87449140B181D7B218B` |
| `SwordRhythmEffectCueExecutionDriver` | 5 | 0 | 0 | `6EA7D416FD92394BE00F727BC9BD0AD9873E3D587F5F410190B539C79DFB4FE7` |
| `SwordRhythmEffectCueExecutorAdapter` | 5 | 0 | 0 | `EEF2889C64C28B0E078E37D65C7B238559175E14CED82CA0C8E74FCAD0F3E886` |
| `SwordRhythmEffectCueConsumerAttempt` | 5 | 0 | 0 | `BB20D9092394A0B16158DFAA755FDE8AD0DB0E3B7CE479124777D2465BDA969B` |
| `SwordRhythmEffectCueDelivery` | 4 | 0 | 0 | `D789634CAA3121B9BEFB83909D6B4516D57787E3F7C9B0F044883D6C080CB9F0` |
| `SwordRhythmEffectCue` | 42 | 0 | 0 | `017B96D3418CE8FF0D1F96BFE6AD3106E71BE39C650BAC6E966F5962BE2E7589` |
| `SwordRhythmProductSession` | 1 | 0 | 0 | `8483E5E13BC1619CD420C67B68593859C46BE661075F3F699C16891A09B502F5` |
| `SwordRhythmPresentation` | 4 | 0 | 0 | `F5A45D403AB98A59D0C297411C80AA0AAB7A5B041971BFAAFCF83561A715FCA9` |
| `SwordRhythmEvaluationRoute` | 1 | 0 | 0 | `7C9BD502D85094C59550C978F83CCD091BADC8E3A1476444ABDCD6032B7DE3E1` |
| `Shanmen.0_0_10` | 608 | 0 | 0 | `1A873982581966252E90C1FE28F279651680B9F473D572EAAB1A64219C9E3E1E` |

最终选定阶段均为 native exit `0`、Fail `0`、selected-phase error `0`；全量唯一用例 `608/608`。

## 4. 门禁

```text
REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=20 Logs=13
SELF_TEST: PASS 200/200
ROUTER_TRANSACTION_SCAN: PASS exactly one Router.TryRouteProcessNext call
TRANSACTION_BYPASS_SCAN: PASS
RUNTIME_BOUNDARY_SCAN: PASS
REFLECTION_SCAN: PASS
AUTHORITY_SCAN: PASS ProductSession GameMode CombatRunCoordinator unchanged
JSON_PARSE: PASS
TRAILING_WHITESPACE_SCAN: PASS
git diff --check: PASS (native exit 0)
```

## 5. 构建

- Editor initial：5 actions / 30.96s / exit `0`；
- Game final：4 actions / 23.87s / exit `0`；
- Editor final：0 actions / 0.93s / exit `0`；
- Editor DLL：13,557,248 bytes / SHA `EB12B2577D85E2C7BD4A1F67BED25023B1E0B695005C55B4AB438E1D370FF9FA`；
- Game EXE：355,134,976 bytes / SHA `C9D4E96F9D313E20AF51355D41F486887C4584E0EEF6A9C3C3B80E3969B5C5F6`；
- 无源码构建失败或 Windows commit-memory/page-file 错误。

## 6. 异常记录

产品、Automation、自检、门禁与构建均首次成功，无重试或结果修饰。UE 选定 `RunTests` 之前的固定 13 条启动 diagnostics 未计入阶段结果。

## 7. 修改、兼容性与边界

Report/Log 前 5 个代码/流程文件净变更 `+1416 / -0`。command host 是 caller-owned Run-local transport boundary；production 文件不依赖 reflection、UObject、World、Actor、资产、timer、async/thread、RNG、damage、attribute 或 GAS application。长期未跟踪资料保持未暂存。

本轮仅 P 阶段。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、实际动画/声音/VFX、Cook 或 Package。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p12-19-sword-rhythm-cue-execution-command-host>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-19-sword-rhythm-cue-execution-command-host/Docs/Report/Dev.D.UE.0.0.10.P12.19.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-19-sword-rhythm-cue-execution-command-host/Docs/Log/Dev.D.UE.0.0.10.P12.19.r0_log.md>
