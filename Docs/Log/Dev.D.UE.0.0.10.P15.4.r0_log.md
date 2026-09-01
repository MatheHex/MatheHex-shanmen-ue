# Dev.D.UE.0.0.10.P15.4.r0 Development Log

## 1. 目标

收敛 legacy `PreparedRunRuntime` 的单一 fixture 根因；保持 runtime slot validation fail closed，并为修改文件建立精确回归证据映射。

## 2. 基线与根因

- focused baseline：`16 Success / 2 Fail / 18 Total`，SHA `28453B54E226F9160EF632F0A85804D02F74FB33E24546F256420313D1109506`；
- 两个失败都把 `WindTalisman` 放入 `AccessorySlot`；
- canonical catalog 中 `WindTalisman` 已是 `SpatialRingCategory + SpatialRingSlot`，`EvasionCharm` 才是当前 accessory；
- runtime 的 `SlotCompatibilityRejected` 属于正确失败关闭，生产代码不应放宽。

## 3. 实现

- case 02 与 case 04 的 accessory fixture 改为 `EvasionCharm + AccessorySlot`；
- 其它正负 Prepared Run contract 不变；
- 新增 `demo_mapPreparedRunRuntimeTests.cpp -> demo_map.PreparedRunRuntime` exact mapping；
- self-test 增加 exact evidence 正例与 unrelated full evidence 反例。

## 4. 验证

```text
PreparedRunRuntime: 16/2 -> 18/0
legacy demo_map: 1316/14 -> 1318/12, Total=1330
REGRESSION_COVERAGE: PASS Changed=3 Rules=1 Required=1 Logs=1
SELF_TEST: PASS 254/254
JSON_PARSE: PASS Rules=152
git diff --check: PASS
```

| Evidence | Result | SHA-256 |
|---|---|---|
| focused baseline | 16 Success / 2 Fail | `28453B54E226F9160EF632F0A85804D02F74FB33E24546F256420313D1109506` |
| focused after | 18 Success / 0 Fail | `2ED5B5E0A8C04EF2FB89747BCF6F09BB2A781B197B941B0B3F2980F704B4BF19` |
| legacy after | 1318 Success / 12 Fail | `9C860B7DF59ADEE3DB9DF464757A20DAE841690147731029BEC5C99D7EC8FF5A` |
| full 0.0.10 | 712 Success / 0 Fail | `A4DED707766C00ABD15291A690DD29220636792CDEA35AF7955DF93495AE766E` |

流程日志 SHA：gate `745CFB3DEF13EA0BC0E6FBF3A5EC286974ED6F87646F1C9D0FEEC6732784A310`；self-test `1276872830754CC239687CCB03402ED1F0C1F6099FBE80BE10BD858B1150A4A6`。

## 5. 构建

- initial Editor：4 actions / 16.58s / succeeded；
- final Game：3 actions / 22.25s / status 0 / log SHA `5BA73BBA73534EAD75EE1095EDFB8D2764FD5C64C04B875937B488678FB8C1BB`；
- final Editor：up to date / 0 actions / 0.97s / status 0 / log SHA `8C7D59E668D53BDAEB4D67E2C71A8004DF1B1F2A2DE97C20BF4C5FE4812F51A6`；
- Game EXE：355,741,184 bytes / SHA `E9D2FC03BF77BFA337EFA83DA4DFCA6BC06B01749747182190EA9307F0628A85`；
- Editor DLL：14,327,296 bytes / SHA `9B845F6F7645E76E00DB5FDDB4602656CC9D151A371147CBB0FC8FAB2FEA360D`。

## 6. 剩余失败与范围

legacy 父组剩余 12 项：RootBoundary 1、EnemyRouteLoot 1、InputRestore 3、P1 SectNavigation 1、P5RuntimeInterface 1、P6 4、V3 Lifecycle 1。它们未被跳过或归入本阶段。

本轮仅 P 阶段 fixture、回归门禁、NullRHI Automation 与 Development build；production C++ 零修改。未运行 UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。raw logs 仅本地保存，长期未跟踪用户文件未修改或提交。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p15-4-prepared-run-slot-fixture-regression>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p15-4-prepared-run-slot-fixture-regression/Docs/Report/Dev.D.UE.0.0.10.P15.4.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p15-4-prepared-run-slot-fixture-regression/Docs/Log/Dev.D.UE.0.0.10.P15.4.r0_log.md>
