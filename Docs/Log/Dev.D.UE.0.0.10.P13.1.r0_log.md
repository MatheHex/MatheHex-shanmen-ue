# Dev.D.UE.0.0.10.P13.1.r0 Development Log

## 1. 目标

把 P13.0 concrete visual/audio handoff executors 接入真实 `Ademo_mapGameMode` SwordRhythm Run 生命周期。所有 observation revision 必须按不可变 prepared-dispatch 值保存；pending channel 不得被覆盖；Blueprint caller 必须以 exact identity 读取和消费。不得伪称实际 asset playback。

## 2. 实现

- 新增 Run-scoped deterministic presentation controller；
- 为 dispatch seed、visual scope、audio scope 派生 Run-local identity；
- revision 严格连续，pending 后续 revision 进入 FIFO；
- 两个 channel 都有容量才同步 pump，任一 pending 即 backpressure；
- exact consume，在 pair 完全清空后继续 pump；
- Run teardown summary + reset，foreign Run fail closed；
- GameMode 在 ProductSession 后绑定 controller，在每次成功 BasicSword rhythm observation 后 publish；
- GameMode 暴露 Blueprint visual/audio get 与 exact consume；
- release/stale-state 路径包含 presentation controller；
- 无 gameplay、cue、route、executor receipt、persistence 或 playback authority复制。

## 3. Automation

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

全量约 `29m52.95s`；最终 selected phase Fail/Fatal/Unhandled/Ensure/AutomationError `0`、Queue Empty `1`、native exit `0`。

## 4. 门禁

```text
REGRESSION_COVERAGE: PASS Changed=7 Rules=2 Required=63 Logs=8
SELF_TEST: PASS 232/232
PRESENTATION_RUN_CONTROLLER_BOUNDARY_SCAN: PASS ForbiddenCodeHits=0
AUTHORITY_SCAN: PASS existing execution/product authority files unchanged
JSON_PARSE: PASS Rules=136
git diff --check: PASS (native exit 0)
```

## 5. 构建

- Editor initial：26 actions / 110.64s / exit `0`；
- Editor test-fixture correction：4 actions / 5.67s / exit `0`；
- Game final：25 actions / 97.73s / exit `0`；
- Editor final：0 actions / 0.99s / exit `0`；
- Editor DLL：14,109,696 bytes / SHA `0F573C1577DC3405740360DFA1EC8543924205DD55A656F6D02F362A7E237253`；
- Game EXE：355,575,296 bytes / SHA `C98D75A7D0810816ECEC9DB0A4090F696C8A889644D10C8536C9501A52989A45`。

## 6. 异常记录

首次 focused run 为 `3/5`（native exit 0；SHA `EAFAF63157E407FEAD3D65B6325F700968583ADF78592582E104919B321513E1`）。`FifoBackpressure` 与 `RunTeardown` 的夹具错误假定八次同 tick observation 必然出现下一 cue。改为验证确定的下一 revision 冻结后，生产 controller 不变，focused 最终 `5/5`。

首次 regression gate 因 GameMode legacy evidence 缺少四组而拒绝；保留映射并补跑 116 个测试后通过。全量含 42 次既有 `generate_204` timeout 与 65 次 large-delta；689 项全部成功。命令前固定 13 条引擎自检错误不属于 selected phase。raw logs 仅本地保存。

## 7. 修改、兼容性与边界

Report/Log 前七个生产/测试/流程文件净变更 `+1386 / -4`。既有 ProductSession 与 P12 execution/product authority 未改；controller 仅拥有 caller presentation queue。长期未跟踪文件未修改或提交。

本轮仅 P 阶段。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、实际动画/音频/VFX、Cook 或 Package。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p13-1-sword-rhythm-cue-game-mode-handoff-integration>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p13-1-sword-rhythm-cue-game-mode-handoff-integration/Docs/Report/Dev.D.UE.0.0.10.P13.1.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p13-1-sword-rhythm-cue-game-mode-handoff-integration/Docs/Log/Dev.D.UE.0.0.10.P13.1.r0_log.md>
