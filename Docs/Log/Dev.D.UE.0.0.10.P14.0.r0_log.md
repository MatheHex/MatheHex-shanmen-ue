# Dev.D.UE.0.0.10.P14.0.r0 Development Log

## 1. 目标

交付 0.0.10 第一条真实战斗 condition 纵切：Boss Charge committed vitality hit -> Meridian Shock -> canonical 30 Hz timeline 90 ticks -> existing attribute authority `MoveSpeed x0.75` -> exact expiry / Run teardown cleanup。参数是可替换原型值，不是最终平衡。

## 2. 实现

- 新增 Run-scoped `CombatConditionComponent`，绑定 Run、target、timeline 与既有 AttributeComponent；
- 只接受真实有效且 applied damage 大于零的 vitality commit receipt；
- 以 Impact/Resolution 派生 immutable application evidence；
- exact replay 幂等，Impact identity 冲突 fail closed；
- 首次命中安装一个 deterministic exact-handle movement modifier；
- 后续真实命中刷新 expiry，不叠加同源 modifier；
- tick 90 精确到期，Run teardown、component destroy 与 recovery reset 删除 exact modifier；
- GameMode 只为 `BossCharge` 成功提交伤害接线，并复用既有 fixed timeline advance；
- 不复制 damage、vitality、attribute、clock、inventory 或 Actor Tick authority。

## 3. Automation

| Group | Success | Fail | Exit | SHA-256 |
|---|---:|---:|---:|---|
| MeridianShock focused | 5 | 0 | 0 | `9BFBE1FE8199F15218B301F16835ABF4D6A787B4A8CD53F44ED083B85FB4538D` |
| full 0.0.10 | 699 | 0 | 0 | `DD5AD5D2C63F17077468B07153EF92A8AC7483937156CD87493C231130AC5DB7` |
| EnemySkillFramework | 44 | 0 | 0 | `A4D857C4808119CBEBE0A127106A78D7B39B60352B289E9E7D1EFBC9198E0EE7` |
| ItemUseAndArmor | 46 | 0 | 0 | `D68CC32D4DB02D9C0CF9378D823E565DCA13F8D51318C745443008BB27D13387` |
| V2RangedCompatibility | 22 | 0 | 0 | `C5E39A08AF400189CC1EA960D024BB7721E74B37950251703E13F05CD769BBD1` |
| V3.Attributes | 4 | 0 | 0 | `543EA057213E8C3CDBD0EF9D2E3126B92DC29FE78A55EE866253E0E8423711B2` |

六份日志共 `820` 个 Success、`0` Fail，Queue Empty，native exit `0`。全量约 `30m14.92s`，selected Fatal/Unhandled/Ensure/Controller Error 均为 `0`。

## 4. 门禁

```text
REGRESSION_COVERAGE: PASS Changed=7 Rules=2 Required=45 Logs=6
SELF_TEST: PASS 236/236
CONDITION_BOUNDARY_SCAN: PASS
JSON_PARSE: PASS Rules=138
git diff --check: PASS (native exit 0)
```

## 5. 构建

- Editor initial：`OtherCompilationError` / 26 planned actions / 121.85s / exit `6`；
- 原因：运行时三元表达式误用于 `UE_LOG` verbosity token，触发 `C2039/C2065/C2275/C2672`；
- Editor retry：4 actions / 10.02s / exit `0`；
- Game final：25 actions / 99.57s / exit `0`；
- Editor final：up to date / 0 actions / 0.94s / exit `0`；
- Editor DLL：14,215,680 bytes / SHA `BC50258510EBBAC6C6CBD1BCD8B864CE44C7AC2B999E1D8FFA8E7C31863A613D`；
- Game EXE：355,655,680 bytes / SHA `83BC49FF5A107A43C471B90DFC30227E0DD1B1DE81CD5C77E8FAEA813FB01298`。

失败如实归类为源码宏用法错误，不描述为内存或环境故障。初次失败日志 SHA `A158EEF01CB6306E656BFDC51914EC41C0232E878DC30D0F9A9CAF2D5C23DE5E`，retry 日志 SHA `F3CCD47A67522BF533AC7978074EEDF2F1781252159FCC78B181DD15B0EAB905`。

## 6. 范围与边界

Report/Log 前 7 个代码、测试、流程文件净变更 `+1378/-2`。长期未跟踪用户文件未修改或提交；raw logs 仅本地保存。本轮仅 P 阶段，未运行 UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、真实表现、Cook 或 Package。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p14-0-meridian-shock-condition-vertical-slice>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p14-0-meridian-shock-condition-vertical-slice/Docs/Report/Dev.D.UE.0.0.10.P14.0.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p14-0-meridian-shock-condition-vertical-slice/Docs/Log/Dev.D.UE.0.0.10.P14.0.r0_log.md>
