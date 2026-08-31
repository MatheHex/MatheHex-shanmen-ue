# Dev.D.UE.0.0.10.P12.18.r0 Development Log

## 1. 目标

在 P12.17 caller-owned execution session 之上增加 typed execution command router：以冻结 `Create / ProcessNext / End` command 驱动唯一 batch lifecycle，锁定 CommandId payload，并对所有已提交结果提供 executor-free stable replay。

## 2. 实现

- 新增 private-payload typed command 与 kind-specific capture/validation/deep match；
- Create command 复制 ordered event batch 与双 consumer identity，并复算 Run/Batch identity；
- ProcessNext command 冻结 Run/Batch 与双 attempt；End command冻结 terminal identity；
- router 使用 candidate transaction 原子提交 session mutation 与 command record；
- exact CommandId replay 返回首次 durable receipt，不重新进入 session/executor；不同 payload 返回冲突；
- retry 与 partial reject receipt 均 durable，fresh command/failed-channel attempt 才能恢复；
- Create/End 使用 executor-free route，ProcessNext 要求显式双 executor route；
- pre-create、foreign identity、second create、early End 与 post-end Process fail closed；
- 无 ProductSession、World、资产、timer、线程、自动 tick、poll 或 retry ownership。

## 3. Automation

| Group | Success | Fail | Exit | SHA-256 |
|---|---:|---:|---:|---|
| `SwordRhythmEffectCueExecutionCommandRouter` | 5 | 0 | 0 | `2544A7EA69106A972F2C81F0E0D6634744D5420F3A968038FE67FFD72CCD00BD` |
| `SwordRhythmEffectCueExecutionSession` | 5 | 0 | 0 | `23F8775888FEE911F295E4E3B1359CE5A913585B934D54A53526B4CF4D74DB9E` |
| `SwordRhythmEffectCueExecutionHost` | 5 | 0 | 0 | `BBD5E7EB14021B0A45C4067C03CD200596066C5950844617A27D6610BCB82673` |
| `SwordRhythmEffectCueExecutionDriver` | 5 | 0 | 0 | `F91DC83CEDED75C93CC52DE1A8ADC544EDA52EBB14A711AF7A08E83DE907077F` |
| `SwordRhythmEffectCueExecutorAdapter` | 5 | 0 | 0 | `E031B13E04CCBBAA71AE53260C844B6F1B695B5A9C441216B06A9F0F41278AF7` |
| `SwordRhythmEffectCueConsumerAttempt` | 5 | 0 | 0 | `710C5607E0C4A3055418170ECB60C0B5312D081282969BB949B47015BE944D8A` |
| `SwordRhythmEffectCueDelivery` | 4 | 0 | 0 | `2DDAAD60D89677FF3DB588B323CF19412011334877F791C286FF7CEB2128256A` |
| `SwordRhythmEffectCue` | 37 | 0 | 0 | `9E92D821E85066453828A0B9FADC32584112813B33DC0472E476DAD0C3970125` |
| `SwordRhythmProductSession` | 1 | 0 | 0 | `27FEF806C932DB4446955C7E03DD7A9AEB83CFC62974AB7A68725BFB7735FAC9` |
| `SwordRhythmPresentation` | 4 | 0 | 0 | `09FE67FC68996CC75B71961FCCFDEA14009AD229817D826CDEC2BCD649F848E6` |
| `SwordRhythmEvaluationRoute` | 1 | 0 | 0 | `651C7134917560BD78B1D4EAD61ABCFED647462C8E9B3AA1C1CCEFDCE74D45EA` |
| `Shanmen.0_0_10` | 603 | 0 | 0 | `D7A19D5E0258326039ADD5CF2EDBE2C8F218FA339430E6BBD9620F0AD44B6890` |

最终选定阶段均为 native exit `0`、Fail `0`、selected-phase error `0`；全量唯一用例 `603/603`。

## 4. 门禁

```text
REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=19 Logs=12
SELF_TEST: PASS 198/198
SESSION_TRANSACTION_SCAN: PASS exactly one Session.ProcessNext call
TRANSACTION_BYPASS_SCAN: PASS
RUNTIME_BOUNDARY_SCAN: PASS
REFLECTION_SCAN: PASS
AUTHORITY_SCAN: PASS ProductSession and GameMode unchanged
JSON_PARSE: PASS
TRAILING_WHITESPACE_SCAN: PASS
git diff --check: PASS (native exit 0)
```

## 5. 构建

- Editor initial：5 actions / 15.38s / exit `0`；
- Game final：4 actions / 23.99s / exit `0`；
- Editor final：0 actions / 0.94s / exit `0`；
- Editor DLL：13,495,296 bytes / SHA `9D6232DF546ADB822A9D4FF9A36C80006DC43E8A03964D2030B5384D42A02A68`；
- Game EXE：355,082,752 bytes / SHA `D4C102690E5C44D38CB64B1DAF043C0A8862A2E6C1BD3537F188E9A00CB962CA`；
- 无源码构建失败或 Windows commit-memory/page-file 错误。

## 6. 异常记录

产品、Automation、自检、门禁与构建均首次成功，无重试或结果修饰。UE 选定 `RunTests` 之前的固定启动 diagnostics 未计入阶段结果。

## 7. 修改、兼容性与边界

Report/Log 前 5 个代码/流程文件净变更 `+1535 / -0`。router 是 caller-owned Run-local command boundary；production 文件不依赖 reflection、UObject、World、Actor、资产、timer、async/thread、RNG、damage、attribute 或 GAS application。长期未跟踪资料保持未暂存。

本轮仅 P 阶段。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、实际动画/声音/VFX、Cook 或 Package。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p12-18-sword-rhythm-cue-execution-command-router>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-18-sword-rhythm-cue-execution-command-router/Docs/Report/Dev.D.UE.0.0.10.P12.18.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p12-18-sword-rhythm-cue-execution-command-router/Docs/Log/Dev.D.UE.0.0.10.P12.18.r0_log.md>
