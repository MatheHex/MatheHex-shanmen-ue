# Dev.D.UE.0.0.10.P12.22.r0 Development Log

## 1. 目标

为 P12.21 frozen single-projection transaction 增加最薄的 current ProductSession dispatch seam：外部显式提供完整 transaction identity、caller-owned Host 与双 executor；一次调用只捕获当前 projection、冻结 transaction 并执行一次，返回可复核证据，不创建新的 state/identity/retry 权威。

## 2. 实现

- `TryDispatchCurrent` 对当前 ProductSession 调用一次 `ProductProjector::CaptureCurrent`；
- projection 成功后调用一次 `ProductTransactionFactory::Capture`；
- frozen request 成功后调用一次 `ProductTransaction::TryExecute`；
- Completed/Resumed/Replayed 映射到 Dispatched/Resumed/Replayed；其它结果作为 TransactionIncomplete 原样返回；
- success gate deep-match projection 与 frozen request，并核对 Run/Config/CuePolicy/Event identity；
- 返回的 TransactionCapture 保留 frozen request，允许 source Session 推进后的历史 exact replay；
- dispatch 不生成 identity、不持有 Host/executor、不循环、不自动 retry。

## 3. Automation

| Group | Success | Fail | Exit | SHA-256 |
|---|---:|---:|---:|---|
| `SwordRhythmEffectCueExecutionProductDispatch` | 5 | 0 | 0 | `75E7CD4F8065FE768049230B4CE6F6949CBAA5916A3DEE7204DAA3795B5CE29A` |
| `SwordRhythmEffectCueExecutionProductTransaction` | 5 | 0 | 0 | `293F554D26B5345C63075A13A456BFB8FB3C4510AE9C3DE78FAC94833959674C` |
| `SwordRhythmEffectCueExecutionProductRoute` | 5 | 0 | 0 | `BA68C9185C17260FCC0D7A8FBD6D87BB4568CE73DBA04E3D78E12C2087C67735` |
| `SwordRhythmEffectCueExecutionCommandHost` | 5 | 0 | 0 | `363816E7498D8F14C6ACB063176C747B674DF4044ED7F461DD67D160248BBDA2` |
| `SwordRhythmEffectCueExecutionCommandRouter` | 5 | 0 | 0 | `CCAF347098418FFEF3D9E8877A203CAF0AC72894AB3468E0CAA2659E2E39D5B1` |
| `SwordRhythmEffectCueExecutionSession` | 5 | 0 | 0 | `A6C69065C966EE40F9FFE36990AE14E4BA812B8D0ED9901826489B52A2ECDC0B` |
| `SwordRhythmEffectCueExecutionHost` | 5 | 0 | 0 | `DF4E76A35B31501F468B039B3F3E2459C4B38D6FA2E34B8528B6DEF539B54DB0` |
| `SwordRhythmEffectCueExecutionDriver` | 5 | 0 | 0 | `60F25E0DB8499DE6E9D01EA7AA94DD5A073AF12E2CB1BC4FC3971C61E6C19BBB` |
| `SwordRhythmEffectCueExecutorAdapter` | 5 | 0 | 0 | `4CA6157612865D90ACD234812BBC136754F1AEE8C63362370F57CDA284377BDE` |
| `SwordRhythmEffectCueConsumerAttempt` | 5 | 0 | 0 | `F3B8D0DE79132447731F63C5D237ED041AEBA78812D4C7328020A1F20785387C` |
| `SwordRhythmEffectCueDelivery` | 4 | 0 | 0 | `8ED294C0C3AE0472213C6A45ECF078D0E6F9896931F686771B92BE1E332640E5` |
| `SwordRhythmEffectCue` | 57 | 0 | 0 | `5A5E4853494591A586B1A08DFE9E4385B2E14BCFD64AD044666F1ACB4FCD63ED` |
| `SwordRhythmPresentation` | 4 | 0 | 0 | `F31FD4AE5564F24133B4056712C26D87619E766B411B51F4F0B8095473EB1911` |
| `SwordRhythmProductSession` | 1 | 0 | 0 | `3766C2E7D297E677FCC461ACFD6896EA37CE457731C3E6D4AD9ADD23908C473B` |
| `SwordRhythmEvaluationRoute` | 1 | 0 | 0 | `A4698F9EA267916DEF470F6A077CC4F0C6B7DC3A725E6929E72CD2F178FA93BD` |
| `Shanmen.0_0_10` | 623 | 0 | 0 | `00A475F3C5706F6D92F835487F94CB0EB2ED144E72A097E40DCAC6A23BFCF478` |

最终 selected phase 全部 Fail `0`、error/Fatal/Unhandled/Ensure `0`；EffectCue `57/57`、全量 `623/623`。

## 4. 门禁

```text
REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=23 Logs=16
SELF_TEST: PASS 206/206
DISPATCH_CALL_SCAN: PASS Projection=1 Capture=1 Execute=1
AUTO_RETRY_LOOP_SCAN: PASS loops=0
RUNTIME_BOUNDARY_SCAN: PASS
REFLECTION_SCAN: PASS
AUTHORITY_SCAN: PASS ProductSession ProductHost GameMode CombatRunCoordinator unchanged
JSON_PARSE: PASS
git diff --check: PASS (native exit 0)
```

## 5. 构建

- Editor initial：5 actions / 16.50s / exit `0`；
- Game final：4 actions / 23.88s / exit `0`；
- Editor final：0 actions / 0.91s / exit `0`；
- Editor DLL：13,660,160 bytes / SHA `0F6C88C4A5255FA1DC12ABDCB7D564F46183DB4952130C27694DB7E6B446FBAB`；
- Game EXE：355,216,384 bytes / SHA `ADF394CF7BA7A40236F89F7E38E3B8215CCAC04E3FE34EA64DA170A61D86231A`；
- 有效构建无源码失败或 Windows commit-memory/page-file 错误。

## 6. 异常记录

首次 Editor 构建和首次 focused Automation 均成功；focused 为 `5/5`、exit `0`，没有代码或测试修正轮。UE selected command 前存在既有 13 条启动诊断，但 selected phase 无 error/Fatal/Unhandled/Ensure。raw 日志保留本地，不纳入 Git。

## 7. 修改、兼容性与边界

Report/Log 前 5 个代码/流程文件净变更 `+684 / -0`。新层只桥接 current ProductSession 与 P12.21 transaction；不拥有 identity、Host、executor、retry、World、资产、timer 或 thread。

本轮仅 P 阶段。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、实际动画/声音/VFX、Cook 或 Package。长期未跟踪资料保持未暂存。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p12-22-sword-rhythm-cue-execution-product-dispatch>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-22-sword-rhythm-cue-execution-product-dispatch/Docs/Report/Dev.D.UE.0.0.10.P12.22.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-22-sword-rhythm-cue-execution-product-dispatch/Docs/Log/Dev.D.UE.0.0.10.P12.22.r0_log.md>
