# Dev.D.UE.0.0.10.P12.13.r0 Development Log

## 1. 目标

在 P12.12 consumer delivery cursor 之上增加 asset-free attempt coordinator。按 consumer role 只交付本 channel commands；外部 executor 失败时保持 retryable，成功时才 acknowledgement；不接资产、World 或实际播放。

## 2. 实现

- 新增 self-validating consumer route，Visual/Audio role 只投影对应 typed channel；
- delivery、role、channel 与完整 command identity 进入 deterministic RouteId；
- 新增 immutable attempt command/receipt，executor receipt 保持 opaque；
- retry receipt 不含 acknowledgement，same event 继续可 prepare；
- exact attempt replay 幂等，同 AttemptId 更换 evidence fail closed；
- success receipt 必须包含精确匹配 delivery 的 P12.12 acknowledgement；
- 零 command route 仅接受 executor-free `NoOpSucceeded`，不伪造播放；
- pending route 阻止 newer route 越序，成功后才允许推进；
- coordinator 只保留当前 route attempts，不建立全局历史或第二 Session；
- ProductSession、GameMode、既有 cue adapter/delivery 与 gameplay 权威未修改。

## 3. Automation

| Group | Success | Fail | Exit | SHA-256 |
|---|---:|---:|---:|---|
| `SwordRhythmEffectCueConsumerAttempt` | 5 | 0 | 0 | `524338FDD5649086C69AE600F958CC33DBA464953B11ABF9BF3A3A02145BC43A` |
| `SwordRhythmEffectCueDelivery` | 4 | 0 | 0 | `13EE028577EC167C3D48FCF3F4453F4447E8E05C39E1388B12E798354FF9CAE3` |
| `SwordRhythmEffectCue` | 12 | 0 | 0 | `84F168D1C2F8D4560E3F5DBA8272440F08A298F548EFB13368B32E0275D140EF` |
| `SwordRhythmProductSession` | 1 | 0 | 0 | `DDFE0476B3AF8EA5690CCAB498CB5845A40B8CF971CA488CD1F8006445605A7A` |
| `SwordRhythmPresentation` | 4 | 0 | 0 | `1CB0D9B13A74F9C6DCA1DBF99F4E7B2C019B7AADD17023DD826FED8E4E45467B` |
| `SwordRhythmEvaluationRoute` | 1 | 0 | 0 | `5472D175CB0DE5365D9BB0F443AB074EBF3EA92AA93419E305A74ADCD9450F65` |
| `Shanmen.0_0_10` | 578 | 0 | 0 | `38993C25612DBCDF289253083F450EE92565D30F2183FFA6D423496717E7C870` |

最终选定阶段均为 native exit `0`、Fail `0`、terminal marker 有效；全量唯一用例 `578/578`。

## 4. 首次失败与修正

首次 focused：`4 Success / 1 Fail`，原生退出码 `0`，SHA-256 `784C7DDEA5C3A88781C09AA7CEF93A77DF21A4E5392119AEC338DEA0F2B7311B`。

`PendingAndOrdering` 用例把合法的 newer 零 command route 当成普通 playback route，错误构造 `Succeeded`；production contract 正确拒绝。测试改为按 command count 选择 `Succeeded` / `NoOpSucceeded` 后 focused `5/5`。首次失败日志本地保留，未描述为源码或环境失败。

## 5. 门禁

```text
REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=14 Logs=7
SELF_TEST: PASS 188/188
IDENTITY_REBUILD_SCAN: PASS
PRESENTATION_BOUNDARY_SCAN: PASS
IMMUTABILITY_SCAN: PASS
AUTHORITY_SCAN: PASS ProductSession and GameMode unchanged
TRAILING_WHITESPACE_SCAN: PASS
git diff --check: PASS (native exit 0)
```

## 6. 构建

- Editor initial：6 actions / 26.27s / exit `0`；
- Editor after test correction：4 actions / 5.98s / exit `0`；
- Game final：5 actions / 36.21s / exit `0`；
- Editor final：0 actions / 0.93s / exit `0`；
- Editor DLL：13,275,136 bytes / SHA `9EABB4491BF1A14FC42226472C099094DB7BE4F1091D48A10C1AD3B977DC1652`；
- Game EXE：354,906,112 bytes / SHA `27E45FE6DBE447FF6923D7913F21E61BBBE05A2592797A201885BB8D1F208C40`；
- 无源码构建失败或 Windows commit-memory/page-file 错误。

## 7. 修改、兼容性与边界

Report/Log 前 5 个代码/流程文件净变更 `+1564 / -0`。新增 coordinator 是 consumer-local bounded attempt state，不是 global registry、event history、播放系统或第二 Session。production 文件不依赖资产、World、Actor、UObject、timer、RNG、damage、attribute 或 GAS application。长期未跟踪资料保持未暂存。

本轮仅 P 阶段。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、实际动画/声音/VFX、Cook 或 Package。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p12-13-sword-rhythm-cue-consumer-attempt>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-13-sword-rhythm-cue-consumer-attempt/Docs/Report/Dev.D.UE.0.0.10.P12.13.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-13-sword-rhythm-cue-consumer-attempt/Docs/Log/Dev.D.UE.0.0.10.P12.13.r0_log.md>
