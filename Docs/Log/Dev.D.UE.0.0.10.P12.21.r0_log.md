# Dev.D.UE.0.0.10.P12.21.r0 Development Log

## 1. 目标

为 P12.20 frozen Product projection 增加一个 caller-owned、单 projection、同步有界的完整执行事务：外部显式提供 Host、三条 command identity、固定 sequence、双 consumer/attempt 与双 executor；一次调用按 Create → ProcessNext → End 返回完整证据，不创建第二套 Host、sequence、executor 或 retry 权威。

## 2. 实现

- capture 要求 sequence 固定 `0/1/2`、三 CommandId 互异、双 consumer 互异、双 attempt 互异；
- factory 先生成既有 P12.20 Create request，再从其 Run/Batch identity 预捕获 P12.18 ProcessNext/End command 与 P12.19 envelope；
- request 私有保存单 projection、Create request、Process envelope、End envelope，并重建后 deep-match；
- transaction 对 caller-owned Host 各派发至多一次 Create、ProcessNext、End；
- Process retry/reject 立即停止，保留两条 durable record，不自动重试或 End；
- exact Create durable evidence 可继续执行，结果为 `Resumed`；
- terminal exact replay 为 `Replayed`，三条 receipt 重放且 executor 不重新进入；
- 完成态验证 exact 三 record、next sequence `3`、Host/Run/Batch identity 与 terminal fence。

## 3. Automation

| Group | Success | Fail | Exit | SHA-256 |
|---|---:|---:|---:|---|
| `SwordRhythmEffectCueExecutionProductTransaction` | 5 | 0 | 0 | `8505E6A9F839897E50451EA6F56EE60EC976B4E4AAA6A1853B2A475FC58D5EE4` |
| `SwordRhythmEffectCueExecutionProductRoute` | 5 | 0 | 0 | `08E4DD9D2BAF93D675D0DF2E88D0449199C1D89014F6A075CCF0873E34FEFD0F` |
| `SwordRhythmEffectCueExecutionCommandHost` | 5 | 0 | 0 | `0F96C9E8B28F8BD1B0160604728CF124BE8281E4C8081720D6B6D6C2A9036F9A` |
| `SwordRhythmEffectCueExecutionCommandRouter` | 5 | 0 | 0 | `B4670CCFB7EBEF70E809DC2E13EE466D901223437CF1B68761B0C84AD3A69835` |
| `SwordRhythmEffectCueExecutionSession` | 5 | 0 | 0 | `B788DEE51B26982AB274D7117466A8B1037A7EF858DAFFF23C97D4EC6B873ACC` |
| `SwordRhythmEffectCueExecutionHost` | 5 | 0 | 0 | `52E811CF0A8CECC3740268A0BA2B6C3C33791F3F386ED00D3DEB3457D3FF1B2D` |
| `SwordRhythmEffectCueExecutionDriver` | 5 | 0 | 0 | `6519DE947F3F47B45FECB8F1DE0BEEFB15865FB1B001F41DF7BCF702EC58DF6B` |
| `SwordRhythmEffectCueExecutorAdapter` | 5 | 0 | 0 | `54EDD32FD8C61FBB87FC77B7B5CC20F6ACEB19DFA6CCCFF69C2B828E084BEC3C` |
| `SwordRhythmEffectCueConsumerAttempt` | 5 | 0 | 0 | `BA0F2C79109963E983349C31EEF3DE87A1F85B4A8F69AFD79266F6EB0C1F72A7` |
| `SwordRhythmEffectCueDelivery` | 4 | 0 | 0 | `1BA3FA28B11F9F38923A3AF1FCF10264A2AB7C3EB9F89AAFA84F6473F1694619` |
| `SwordRhythmEffectCue` | 52 | 0 | 0 | `62C4243DB357D31DFE46A85071E1DDB66640B33694965E9BD2F5B166E8E26635` |
| `SwordRhythmPresentation` | 4 | 0 | 0 | `813A0F407A5909301B3953DFFC848369FCBCC8F512E74B941EC7EEEAB1D96D22` |
| `SwordRhythmProductSession` | 1 | 0 | 0 | `9D7394753F4940FE5285EEED7F40F63630BE76BE14C66C2AF1261947D09ED519` |
| `SwordRhythmEvaluationRoute` | 1 | 0 | 0 | `4E8E43C2C03DFBD67C7F9FCC771757C0FD1F60F7E5C47969B81C4596996257F3` |
| `Shanmen.0_0_10` | 618 | 0 | 0 | `E393703969C396E5ED0B16964912DAE1FA6D5ABEA4C3E6247B56C11DC84B2842` |

最终 selected phase 均为 Fail `0`、error/Fatal/Unhandled/Ensure `0`；EffectCue `52/52`、全量 `618/618`。

## 4. 门禁

```text
REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=22 Logs=15
SELF_TEST: PASS 204/204
TRANSACTION_CALL_SCAN: PASS Create=1 ProcessNext=1 End=1
AUTO_RETRY_LOOP_SCAN: PASS loops=0
RUNTIME_BOUNDARY_SCAN: PASS
REFLECTION_SCAN: PASS
AUTHORITY_SCAN: PASS ProductSession ProductHost GameMode CombatRunCoordinator unchanged
JSON_PARSE: PASS
TRAILING_WHITESPACE_SCAN: PASS
git diff --check: PASS (native exit 0)
```

## 5. 构建

- 无效 target/path 预检：未进入源码编译，`OtherCompilationError`，exit `1`；
- Editor corrected initial：5 actions / 27.49s / exit `0`；
- Game final：4 actions / 24.41s / exit `0`；
- Editor final：0 actions / 0.93s / exit `0`；
- Editor DLL：13,634,048 bytes / SHA `326F9345924CF410CFEFEA999C9CA599106C3F7D68E3B0117B3D594FD8C3B462`；
- Game EXE：355,194,880 bytes / SHA `4EFBCCDCE67BD2A31D783AB52286AA9FE3511DF0A3C00857B2FC0968F0375E9F`；
- 有效构建无源码失败或 Windows commit-memory/page-file 错误。

## 6. 异常记录

首次 Build 命令误用不存在的 `Dev.D.UE.0.0.9BEditor` 与 `Dev.D.UE.0.0.9B.uproject`，UBT 在 0 actions 前明确失败；核对仓库后恢复为 `demo_mapEditor + demo_map.uproject`，后续 Editor/Game 均成功。首次 focused Automation 无失败，无测试修正轮。

## 7. 修改、兼容性与边界

Report/Log 前 5 个代码/流程文件净变更 `+1131 / -0`。新层只冻结单 projection transaction intent 并协调三次有界调用；不拥有 ProductSession、Host、executor、identity/sequence allocator、retry loop、World、资产、timer 或 thread。

本轮仅 P 阶段。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、实际动画/声音/VFX、Cook 或 Package。长期未跟踪资料保持未暂存。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p12-21-sword-rhythm-cue-execution-product-transaction>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-21-sword-rhythm-cue-execution-product-transaction/Docs/Report/Dev.D.UE.0.0.10.P12.21.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-21-sword-rhythm-cue-execution-product-transaction/Docs/Log/Dev.D.UE.0.0.10.P12.21.r0_log.md>
