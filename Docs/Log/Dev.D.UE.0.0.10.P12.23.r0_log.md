# Dev.D.UE.0.0.10.P12.23.r0 Development Log

## 1. 目标

以 caller-owned dispatch seed、双 consumer scope 与 frozen Product projection identity，确定性生成 P12.21/P12.22 所需的完整 transaction identity bundle；减少 live call site 的 GUID 样板，同时不引入 allocator、registry、Host、executor 或 retry 权威。

## 2. 实现

- seed 要求 root dispatch identity 有效、Visual/Audio consumer scope 有效且不同；
- 单一 canonical namespace 结合 role、seed、双 scope、Run/config/cue-policy/event/revision 派生 identity；
- 固定派生 Host、三 command、双 consumer、双 attempt 共 8 个 GUID；
- 8 个 identity 必须有效且两两不同，sequence 保持 P12.21 的 `0/1/2`；
- plan 自验证重算 expected identity 并逐字段核对；
- `MatchesProjection` / `MatchesCurrentSession` 暴露 source-advance fence；
- plan identity 可直接输入 P12.22，并保留 terminal exact replay 与 foreign Host reject 语义；
- production 不调用 `FGuid::NewGuid`，不持久化任何 mutable authority。

## 3. Automation

| Group | Success | Fail | Exit | SHA-256 |
|---|---:|---:|---:|---|
| `SwordRhythmEffectCueExecutionProductDispatchPlan` | 5 | 0 | 0 | `3C1F27E029B3E3B102761FBACD0F84BA4598ABB964725DAD22275C18431DFC05` |
| `SwordRhythmEffectCueExecutionProductDispatch` | 10 | 0 | 0 | `85C49BBFAEE559F9E0411D68A73FB3972AF7316AAEE98542F6E7E029FCFAEBA3` |
| `SwordRhythmEffectCueExecutionProductTransaction` | 5 | 0 | 0 | `B7A64F599F561A075C20E109DE76CCA1BBF76763F427B2520D55470FE831D26E` |
| `SwordRhythmEffectCueExecutionProductRoute` | 5 | 0 | 0 | `624FF6AF122609CF3C1C666AFAC5243072529B822FE745E6F203AE6D0C3494DF` |
| `SwordRhythmEffectCueExecutionCommandHost` | 5 | 0 | 0 | `2783C6F83EAF17BF2C50BA2280896B571743CF810123EAC75FA7E0E8CC3779CF` |
| `SwordRhythmEffectCueExecutionCommandRouter` | 5 | 0 | 0 | `13AF2A57A14FD797ECC15AE5D1F0B9F7DB65D1FC07E623DB945F1B26B9F002D1` |
| `SwordRhythmEffectCueExecutionSession` | 5 | 0 | 0 | `9407CC5D0CF4204B1B1C9EE738057E8703593FAC51ADDCA41460FA1EE18A13BB` |
| `SwordRhythmEffectCueExecutionHost` | 5 | 0 | 0 | `DE309CE4B8B33290DACF93A213D6EFAB1148FE6D22429E103CE69F9C0BEABECC` |
| `SwordRhythmEffectCueExecutionDriver` | 5 | 0 | 0 | `243982E1D439BE090488A8DCE655B2599370922F9B882549BDB8DD5DBAD86A75` |
| `SwordRhythmEffectCueExecutorAdapter` | 5 | 0 | 0 | `13D1119CF5F38590730F9B005124C1C754FA5968BBAAD4658990BABD27599172` |
| `SwordRhythmEffectCueConsumerAttempt` | 5 | 0 | 0 | `85A01BDC3029F3B4DCFF3948FF24E7354D6404B2156206DC62BE0D755679130D` |
| `SwordRhythmEffectCueDelivery` | 4 | 0 | 0 | `0C670DBA909CC07D8A04FEB19EFEEA58BEA3DB59D069B37F3FFA186A277114C7` |
| `SwordRhythmEffectCue` | 62 | 0 | 0 | `D63BE16DFD91581AE485ECDCD261EB9C8FE8E972A3DFEF5E246C4DAA7B5995E5` |
| `SwordRhythmPresentation` | 4 | 0 | 0 | `2F5182BFCFD964DDE1C5A69DE2B25B3556605485A67B7BADC28F4EF760F00B59` |
| `SwordRhythmProductSession` | 1 | 0 | 0 | `F5BD91337D459742B1A787178E3AA621F67CA7D338D57268F12A416BFAB11092` |
| `SwordRhythmEvaluationRoute` | 1 | 0 | 0 | `0CBB798585CCBEF8990D8E26872221A0349B091F8AB3B3668824EB573883ACFD` |
| `Shanmen.0_0_10` | 628 | 0 | 0 | `94CC1D1E6A7260014323011F28B8719E3D9DD5CB4E408AD5ED85653BF4BC8612` |

最终 selected phase 全部 Fail `0`、error/Fatal/Unhandled/Ensure `0`；Plan `5/5`、EffectCue `62/62`、全量 `628/628`。

## 4. 门禁

```text
REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=24 Logs=17
SELF_TEST: PASS 208/208
DETERMINISTIC_ID_SCAN: PASS NewGuid=0 CanonicalHashCalls=1 RoleDerivations=8
RUNTIME_BOUNDARY_SCAN: PASS
REFLECTION_SCAN: PASS
AUTHORITY_SCAN: PASS ProductSession ProductHost GameMode CombatRunCoordinator unchanged
JSON_PARSE: PASS
git diff --check: PASS (native exit 0)
```

## 5. 构建

- Editor initial：5 actions / 24.43s / exit `0`；
- Game final：4 actions / 23.90s / exit `0`；
- Editor final：0 actions / 0.96s / exit `0`；
- Editor DLL：13,689,344 bytes / SHA `2A23014CB1D30914E72BA8FAC1ED71239FE96A8720B6FF68BD3C5DF9412F5439`；
- Game EXE：355,239,424 bytes / SHA `F913EE93B6A488012A659F787B65388962DF27B8F32817F154507ACB7959F817`；
- 有效构建无源码失败或 Windows commit-memory/page-file 错误。

## 6. 异常记录

首次 Editor 构建和首次 focused Automation 均成功；focused 为 `5/5`、exit `0`，没有代码或测试修正轮。UE selected command 前存在既有 13 条启动诊断，但 selected phase 无 error/Fatal/Unhandled/Ensure。raw 日志保留本地，不纳入 Git。

## 7. 修改、兼容性与边界

Report/Log 前 5 个代码/流程文件净变更 `+846 / -0`。新层只做 seed/scope/projection 到 transaction identity 的 pure deterministic projection；不拥有 Session、Host、executor、sequence、retry、World、资产、timer 或 thread。

本轮仅 P 阶段。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、实际动画/声音/VFX、Cook 或 Package。长期未跟踪资料保持未暂存。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p12-23-sword-rhythm-cue-execution-dispatch-plan>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-23-sword-rhythm-cue-execution-dispatch-plan/Docs/Report/Dev.D.UE.0.0.10.P12.23.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-23-sword-rhythm-cue-execution-dispatch-plan/Docs/Log/Dev.D.UE.0.0.10.P12.23.r0_log.md>
