# Dev.D.UE.0.0.10.P14.1.r0 Development Log

## 1. 目标

把 P14.0 的 Meridian Shock authority 投影为 immutable、Blueprint-readable status snapshot，并通过 GameMode 提供只读查询；不复制 condition、attribute、vitality 或 fixed-timeline 真值。

## 2. 实现

- 新增 `Fdemo_mapShanmenCombatConditionStatusSnapshot`，13 个 private `VisibleAnywhere, BlueprintReadOnly` 字段，0 个 mutable exposure；
- 用全部规范字段和独立命名空间确定性派生 `StatusId`；
- `IsValid()` 重算 identity，并校验 Run/timeline、definition、90 ticks、30 Hz、0.75、revision、expiry 与 remaining invariants；
- condition component 只复制已有状态，失败时清空输出；
- GameMode 提供 `BlueprintPure TryGetMeridianShockStatus`，不缓存第二份状态；
- repeated capture/replay 保持 identity，timeline progress、refresh、expiry 和 next Run 产生新 identity；
- 已复制 snapshot 在 authority 变化或 teardown 后保持不可变、自校验有效；
- 不增加 Actor Tick、timer、damage、RNG 或第二套生命周期 authority。

## 3. Automation

| Group | Success | Fail | Exit | SHA-256 |
|---|---:|---:|---:|---|
| MeridianShock.Status | 5 | 0 | 0 | `3F4227EDBDA64511E28E55BB03EE0AD693E318FD98B8A10F948630BCDE7DD394` |
| MeridianShock parent | 10 | 0 | 0 | `B703556E68CD39B6D0A19EB9D7D48C21007AE8BF273673565CC90DB36AE4031F` |
| full 0.0.10 | 704 | 0 | 0 | `399C1AE9B355B087EAC7B8A87FB6FCF7823315EEBC476FFC05CD4B3FACA2B394` |
| EnemySkillFramework | 44 | 0 | 0 | `632F09B7355AEF39279620DF23ADE40A016B8D637C79AAF78C5B37EC222A26E2` |
| ItemUseAndArmor | 46 | 0 | 0 | `9BD5EFDC36CF834BC171CD66858C5CC3473F7A55904FFC32CE171ABCCC9115DC` |
| V2RangedCompatibility | 22 | 0 | 0 | `C688AD0EDEFA8B85D21204DDA8D5F5580DF8BEF057F8191E0A4ED9C03261936F` |
| V3.Attributes | 4 | 0 | 0 | `F50E09DB6C46AFF8FEEC70CE793C9A6C92BD7204D7A95ED4EC129701EAD1B5A7` |

七份日志合计 `835` 个 Success、`0` Fail，全部 queue complete、native exit `0`。全量 `704/704`，约 `30m17.12s`；selected Fatal/Unhandled/Ensure/Controller Error 均为 `0`。

## 4. 门禁

```text
REGRESSION_COVERAGE: PASS Changed=9 Rules=3 Required=46 Logs=7
SELF_TEST: PASS 238/238
READ_ONLY_SCAN: PASS BlueprintReadOnly=13 MutableExposures=0
CONDITION_STATUS_BOUNDARY_SCAN: PASS
JSON_PARSE: PASS Rules=139
git diff --check: PASS (native exit 0)
```

## 5. 构建

- Editor initial：27 actions / 127.27s / exit `0` / log SHA `EB89F521A5E5A94E2540CB234597528DA6F18D6D02EECF858FC9A1382AF3D339`；
- Game final：26 actions / 124.81s / exit `0` / log SHA `81CADA042E5BA1E8060EDD0A9641EE59DADA343202B321B548DD393733ED3D7D`；
- Editor final：up to date / 0 actions / 1.63s / exit `0` / log SHA `4115FD37FC546508633C22C9CBF9AD7D80732924DD14158C6711729134245AA1`；
- Editor DLL：14,242,816 bytes / SHA `BF350EB936F51A983A8188358DE446AC959EE1807C48F4682F7FD200FB6E8191`；
- Game EXE：355,677,696 bytes / SHA `01CAC69585489630FABEFFE719D0CA9771F29AC344405B1FB503F1B8F6ADB903`。

## 6. 范围与边界

Report/Log 前 9 个代码、测试、流程文件净变更 `+584/-0`。长期未跟踪用户文件未修改或提交；raw logs 仅本地保存。本轮仅 P 阶段，未运行 UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、真实表现、Cook 或 Package。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p14-1-meridian-shock-status-read-model>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p14-1-meridian-shock-status-read-model/Docs/Report/Dev.D.UE.0.0.10.P14.1.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p14-1-meridian-shock-status-read-model/Docs/Log/Dev.D.UE.0.0.10.P14.1.r0_log.md>
