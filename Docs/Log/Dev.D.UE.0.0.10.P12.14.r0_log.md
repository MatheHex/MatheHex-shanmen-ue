# Dev.D.UE.0.0.10.P12.14.r0 Development Log

## 1. 目标

在 P12.13 consumer attempt coordinator 之上增加 caller-driven、asset-free executor port。完整 nonempty route 作为一个 batch 交给注入 executor；retry/success 以 opaque receipt 回传；exact replay 不 reinvoke；零 command route 完全绕过 executor。

## 2. 实现

- 新增 immutable executor invocation，冻结完整 route、attempt 与 command batch；
- invocation identity 覆盖 route、scope、attempt、command count 与全部 command identity；
- 新增窄 `Idemo_mapShanmenSwordRhythmEffectCueExecutor` 注入边界；
- 新增 immutable opaque executor receipt，只允许 retry/success outcome 并绑定精确 invocation；
- executor reject 或 mismatched evidence 在 coordinator submission 前 fail closed；
- 新增当前 route 的精确 attempt receipt 查询，exact replay 从 coordinator evidence 重建且不 reinvoke；
- retry 后允许 same route 的新 attempt，different pending route 在 executor 前拒绝；
- 零 command route 直接提交 `NoOpSucceeded`，不构造 invocation、不调用 executor；
- stale、acknowledged、cross-scope、invalid route/attempt 均 preflight fail closed；
- ProductSession、GameMode、资产、World、组件和实际播放路径未修改。

## 3. Automation

| Group | Success | Fail | Exit | SHA-256 |
|---|---:|---:|---:|---|
| `SwordRhythmEffectCueExecutorAdapter` | 5 | 0 | 0 | `8852DB75CE478A66D1441AB8D60D4DE165246E4ACAFC9F46BC32440AB6977730` |
| `SwordRhythmEffectCueConsumerAttempt` | 5 | 0 | 0 | `3E44809C30B3A705EFCAF4425E015BD6D547C27F48F036475B1C74866FB64410` |
| `SwordRhythmEffectCueDelivery` | 4 | 0 | 0 | `D1733852E25E438A26112C3C4FE9650AE1FC1DBF50BFF1FAC9BDC9336F33BC49` |
| `SwordRhythmEffectCue` | 17 | 0 | 0 | `092AB55A4DF0E842C62A4942AE50FFAC0201D3D1EF2A9D21E82989C6E5D69706` |
| `SwordRhythmProductSession` | 1 | 0 | 0 | `1903A90531C9C8EB4210743F8B1D6C3BC9A99319AD839C7CFB6E0E99637E6970` |
| `SwordRhythmPresentation` | 4 | 0 | 0 | `3098639723E1E10BFAF2E9DF7F9CF7559FCE08A75945DE8A567F23BFFE48D1D9` |
| `SwordRhythmEvaluationRoute` | 1 | 0 | 0 | `FCF0EEF9C91F23C10CDF64F9F9AA1BF2A88166B909F8DFE3B32D0E8322602AF1` |
| `Shanmen.0_0_10` | 583 | 0 | 0 | `FBE493363FE10A5A5CC05D0E679866322CE2A482445EF71D32B10E160ACB7829` |

最终选定阶段均为 native exit `0`、Fail `0`、terminal marker 有效；全量唯一用例 `583/583`。

## 4. 首次失败与修正

无。本阶段 focused、回归、自检和三次构建均首次成功。选定 `RunTests` 之前的既有 13 条启动 condition diagnostics 未计入本阶段结果；选定阶段错误数为 0。

## 5. 门禁

```text
REGRESSION_COVERAGE: PASS Changed=7 Rules=2 Required=15 Logs=8
SELF_TEST: PASS 190/190
IDENTITY_REBUILD_SCAN: PASS
PRESENTATION_BOUNDARY_SCAN: PASS
IMMUTABILITY_SCAN: PASS
AUTHORITY_SCAN: PASS ProductSession and GameMode unchanged
TRAILING_WHITESPACE_SCAN: PASS
git diff --check: PASS (native exit 0)
```

## 6. 构建

- Editor initial：8 actions / 50.27s / exit `0`；
- Game final：7 actions / 39.56s / exit `0`；
- Editor final：0 actions / 0.96s / exit `0`；
- Editor DLL：13,324,288 bytes / SHA `FA1E0DC0E9190A9A86F5258ABEEE27E47B3182900E0E05C0761ADFA558C9BB70`；
- Game EXE：354,944,000 bytes / SHA `4069529DC67E06EDB537E8B38C9DFBD4841AC8906E5DFE3F08D1774FB067FDF3`；
- 无源码构建失败或 Windows commit-memory/page-file 错误。

## 7. 修改、兼容性与边界

Report/Log 前 7 个代码/流程文件净变更 `+1208 / -0`。新增 adapter 是 stateless caller-driven bridge；executor、route、receipt 与 attempt 均不进入 ProductSession 或全局 authority。production 文件不依赖资产、World、Actor、UObject、timer、RNG、damage、attribute 或 GAS application。长期未跟踪资料保持未暂存。

本轮仅 P 阶段。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、实际动画/声音/VFX、Cook 或 Package。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p12-14-sword-rhythm-cue-executor-port>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-14-sword-rhythm-cue-executor-port/Docs/Report/Dev.D.UE.0.0.10.P12.14.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-14-sword-rhythm-cue-executor-port/Docs/Log/Dev.D.UE.0.0.10.P12.14.r0_log.md>
