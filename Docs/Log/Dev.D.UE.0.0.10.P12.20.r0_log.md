# Dev.D.UE.0.0.10.P12.20.r0 Development Log

## 1. 目标

把现有 `SwordRhythmProductSession` 的 immutable presentation state 接到 P12.19 command Host：由 caller 显式收集 projections、提供 Host/sequence/consumer identity，并只经一个 stateless Create route 建立既有执行链，不复制 ProductSession、batch、transport、executor 或 retry 权威。

## 2. 实现

- `CaptureCurrent` 从有效 ProductSession 的当前 state 与 canonical cue policy 生成冻结 projection；
- projection 私有保存 Run/config/policy identity 与既有 EffectCue event，并验证所有交叉身份；
- source Session 推进后旧 projection 仍有效，但 `MatchesCurrentSession` 明确失配；
- Create factory 要求非空、同 Run/config/policy、严格 revision order 的 caller-owned projection batch；
- caller 必须显式提供 HostId、sequence `0`、CommandId 与两个不同 consumer；
- request 从 private projections 重建 P12.18 command 与 P12.19 envelope，并 deep-match；
- product route 只调用一次 `Host.TryRoute(Create)`；ProcessNext、End、attempt、executor 和 retry 不进入该层；
- exact Create 在 source Session 推进后及 terminal Host 上均重放历史 receipt，不产生新 record。

## 3. Automation

| Group | Success | Fail | Exit | SHA-256 |
|---|---:|---:|---:|---|
| `SwordRhythmEffectCueExecutionProductRoute` | 5 | 0 | 0 | `0D7E65CAEB168B728C97A49B16FFD38FE0DBAB5B94719F7AA1FF9C06815DBA3D` |
| `SwordRhythmEffectCueExecutionCommandHost` | 5 | 0 | 0 | `921D2879EEF04BD8688B204E3F907F36E83B65E0519853CD23133CE6EF7E5867` |
| `SwordRhythmEffectCueExecutionCommandRouter` | 5 | 0 | 0 | `8B2C69A4D8E9EDD33A541B2A87D06C9C61D007FE6E2DFA6D237A440431FFBF49` |
| `SwordRhythmEffectCueExecutionSession` | 5 | 0 | 0 | `38C5F7E6C10B26DF3D115C72C5B9AA03BAD7EA5B65138BD202A3C9E363B68943` |
| `SwordRhythmEffectCueExecutionHost` | 5 | 0 | 0 | `8DB08DA58918240A7F147D8422369415CAD71ABD97BDEDB0B5AAA2809B034641` |
| `SwordRhythmEffectCueExecutionDriver` | 5 | 0 | 0 | `F4B09A9290D0A3081D363FA7508F675942226E0247916BB4ABB38FA6FE4389E5` |
| `SwordRhythmEffectCueExecutorAdapter` | 5 | 0 | 0 | `21CAD0E13ACEF55DB4DF2CF500618F7664C25933944A9327B300F7F75E8780FF` |
| `SwordRhythmEffectCueConsumerAttempt` | 5 | 0 | 0 | `704F0FB971504544B247E50B26F466AFF34F2435426FFFDD42B093744A4DE9E6` |
| `SwordRhythmEffectCueDelivery` | 4 | 0 | 0 | `CF1538F4EE21B0FBC1CD43EB06866E98F41D01EFBC06AEF012F0A669BE7F27D4` |
| `SwordRhythmEffectCue` | 47 | 0 | 0 | `C71A08567E7CFE03508981C45EB6AA2899FA9F254BED7DC6A348D40B6138A68F` |
| `SwordRhythmProductSession` | 1 | 0 | 0 | `14955B567A110208DD13784AD9DD2C0301D7DBA076A7C861BDDC01F375941771` |
| `SwordRhythmPresentation` | 4 | 0 | 0 | `2B3C9E2928FB3CE58485CD1ED4F6296F03306CF410D05093D9C4392230884CC7` |
| `SwordRhythmEvaluationRoute` | 1 | 0 | 0 | `A40204358940177D67CA27B0DD576AFF65BB229B4A633AC78C7480BE632D5166` |
| `Shanmen.0_0_10` | 613 | 0 | 0 | `91138941141986D175E0AA54933B5453D1868CBD9F27557C83E90C60B4751E19` |

最终 selected phase 均为 Fail `0`、error/Fatal/Unhandled/Ensure `0`；EffectCue `47/47`、全量 `613/613`。

## 4. 门禁

```text
REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=21 Logs=14
SELF_TEST: PASS 202/202
PRODUCT_ROUTE_CALL_SCAN: PASS exactly one Host.TryRoute call
PROCESS_END_OWNERSHIP_SCAN: PASS
RUNTIME_BOUNDARY_SCAN: PASS
REFLECTION_SCAN: PASS
AUTHORITY_SCAN: PASS ProductSession ProductHost GameMode CombatRunCoordinator unchanged
JSON_PARSE: PASS
TRAILING_WHITESPACE_SCAN: PASS
git diff --check: PASS (native exit 0)
```

## 5. 构建

- Editor initial：5 actions / 24.67s / exit `0`；
- 两次 test-only Editor rebuild：4 actions / 5.78s、4 actions / 6.57s / exit `0`；
- Game final：4 actions / 24.09s / exit `0`；
- Editor final：0 actions / 0.93s / exit `0`；
- Editor DLL：13,600,256 bytes / SHA `16867E530BB15DE42B212D537CF9BFEDE511BF7DD65BFF6C67CD6A0C93BF8385`；
- Game EXE：355,168,768 bytes / SHA `FBB4D4142A5B0E07D0F5427C635BA7C9ABA59702C1E55748C2B380D56475D47F`；
- 无源码构建失败或 Windows commit-memory/page-file 错误。

## 6. 异常记录

首次 focused 运行原生 exit `0`，但 selected result 为 `4 Success / 1 Fail`，SHA `3765CDB86D1AD4F137AFD571AB29E8A6F2583741DD6CB61F6981DD352208308D`。原因是测试错误预期 historical sequence 0 的 foreign envelope 返回 Host identity conflict；P12.19 正确先返回 sequence conflict。仅修正测试，生产代码未改；最终 focused `5/5`。随后增加 source-advance 与 terminal replay 断言并重跑父组/全量，全部成功。

## 7. 修改、兼容性与边界

Report/Log 前 5 个代码/流程文件净变更 `+1380 / -0`。新层只冻结 product provenance 和 Create intent；不拥有 ProductSession、Host、batch collection、executor、attempt、sequence、timer、thread、World 或资产。

本轮仅 P 阶段。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、实际动画/声音/VFX、Cook 或 Package。长期未跟踪资料保持未暂存。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p12-20-sword-rhythm-cue-execution-product-route>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-20-sword-rhythm-cue-execution-product-route/Docs/Report/Dev.D.UE.0.0.10.P12.20.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-20-sword-rhythm-cue-execution-product-route/Docs/Log/Dev.D.UE.0.0.10.P12.20.r0_log.md>
