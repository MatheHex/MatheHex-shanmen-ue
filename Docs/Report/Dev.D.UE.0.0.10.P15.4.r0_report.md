# Dev.D.UE.0.0.10.P15.4.r0 Report

## 1. 结论

P15.4 完成 legacy Prepared Run runtime fixture regression 的有界收敛：把两个仍将 `WindTalisman` 作为 accessory 的旧测试 fixture 对齐到当前五装备槽契约，并为该测试文件建立“改动文件 → 必跑 suite”门禁。

`demo_map.PreparedRunRuntime` 从 `16 Success / 2 Fail` 收敛为 `18 Success / 0 Fail`。同命令 legacy `demo_map` 父组从 P15.3 的 `1316 Success / 14 Fail / 1330 Total` 变为 `1318 Success / 12 Fail / 1330 Total`，精确消除这两项失败，没有新增或隐藏其它失败。

本阶段没有修改 Prepared Run、物品定义或运行时生产代码，没有放宽 slot compatibility、capacity、identity 或 fail-closed 验证。

## 2. 根因

失败集中在：

- `PreparedRunRuntime.02.ThreeEquipmentPreserveIdentity`；
- `PreparedRunRuntime.04.MaximumSixRecords`。

两项 fixture 都构造：

```text
DefinitionId = WindTalisman
EquipmentSlotId = AccessorySlot
```

`WindTalisman` 的冻结 identifier 仍含历史 `Prototype.Item.Accessory.*` 字样，但当前 canonical definition 已明确属于 `SpatialRingCategory`，且唯一兼容槽为 `SpatialRingSlot`。当前真正的 accessory fixture 是 `EvasionCharm + AccessorySlot`。

`MaterializePreparedRun` 因此正确返回 `SlotCompatibilityRejected`，并在任何 Authority mutation 前失败关闭。两个测试随后看到 0 个 materialized/deployed item。根因是测试 fixture 没有随五槽模型更新，不是 runtime 回归。

## 3. 实现

仅在 `demo_mapPreparedRunRuntimeTests.cpp` 的两个正向 fixture 中进行语义替换：

```text
WindTalisman + AccessorySlot
    -> EvasionCharm + AccessorySlot
```

没有把 `WindTalisman` 强行改回 accessory，也没有把它换成 `SpatialRingSlot`：case 02 的目标明确是 weapon、armor、accessory 三件装备，使用真正的 accessory definition 才保持测试意图。

负向 `WrongSlotRejected`、容量上限、重复/缺失 ID、回滚、二次 materialization 与 settlement 契约全部保持原样。

## 4. 回归覆盖门禁

新增 exact mapping：

```text
Source/demo_map/demo_mapPreparedRunRuntimeTests.cpp
  -> demo_map.PreparedRunRuntime
```

流程自检新增一正一反：

- 精确 PreparedRunRuntime 日志可覆盖该文件；
- 无关 `Shanmen.0_0_10` 全量日志不能替代 legacy suite。

最终流程证据：

```text
REGRESSION_COVERAGE: PASS Changed=3 Rules=1 Required=1 Logs=1
SELF_TEST: PASS 254/254
JSON_PARSE: PASS Rules=152
git diff --check: PASS
```

## 5. Automation 证据

| Evidence | Success | Fail | Total | Status | SHA-256 |
|---|---:|---:|---:|---:|---|
| PreparedRunRuntime baseline | 16 | 2 | 18 | 0 | `28453B54E226F9160EF632F0A85804D02F74FB33E24546F256420313D1109506` |
| PreparedRunRuntime after | 18 | 0 | 18 | 0 | `2ED5B5E0A8C04EF2FB89747BCF6F09BB2A781B197B941B0B3F2980F704B4BF19` |
| legacy `demo_map` after | 1318 | 12 | 1330 | 0 | `9C860B7DF59ADEE3DB9DF464757A20DAE841690147731029BEC5C99D7EC8FF5A` |
| `Shanmen.0_0_10` full | 712 | 0 | 712 | 0 | `A4DED707766C00ABD15291A690DD29220636792CDEA35AF7955DF93495AE766E` |

四份日志均要求 queue-empty terminal evidence，且 Fatal error、Unhandled Exception、Ensure condition failed 为 0。父组 status `0` 不被当作全绿；仍以逐项结果判定其 12 项失败。

0.0.10 全量首末 Success 时间为 `17:14:07.130 -> 17:45:51.710`，约 `31m44.58s`，随后出现 `712 tests performed` queue-empty terminal evidence。

legacy 剩余失败保持可见：AutomationRootBoundary 环境前置 1、EnemyRouteLoot 1、InputRestore 3、P1 SectNavigation 1、P5RuntimeInterface 1、P6 4、V3 Lifecycle 1。

## 6. 构建证据

有效命令：`Build.bat <Target> Win64 Development demo_map.uproject -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`。

| Target | Result | Actions / UBT total time | Status | Log SHA-256 |
|---|---|---|---:|---|
| Game final | Succeeded | 3 / 22.25s | 0 | `5BA73BBA73534EAD75EE1095EDFB8D2764FD5C64C04B875937B488678FB8C1BB` |
| Editor final | Succeeded, up to date | 0 / 0.97s | 0 | `8C7D59E668D53BDAEB4D67E2C71A8004DF1B1F2A2DE97C20BF4C5FE4812F51A6` |

最终产物：

- `demo_map.exe`：355,741,184 bytes，SHA-256 `E9D2FC03BF77BFA337EFA83DA4DFCA6BC06B01749747182190EA9307F0628A85`；
- `UnrealEditor-demo_map.dll`：14,327,296 bytes，SHA-256 `9B845F6F7645E76E00DB5FDDB4602656CC9D151A371147CBB0FC8FAB2FEA360D`。

## 7. 修改范围

- `demo_mapPreparedRunRuntimeTests.cpp`：两个正向 fixture 改用当前 accessory definition；
- `ShanmenRegressionMap.json`：新增 exact legacy suite mapping；
- `Test-ShanmenRegressionCoverageSelfTest.ps1`：新增正反自检；
- Report/Log；
- production C++ 零修改，raw logs 仅本地保存；
- 长期未跟踪的 0.0.9B Prompt、Report、CSEMI、PDF 与用户资料未修改、未暂存、未提交。

## 8. P/F 边界

本 Report 只包含 P 阶段 fixture 修复、NullRHI 无头 Automation、静态审查与 Editor/Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

本轮修复的是正向 fixture 的定义—槽位一致性；没有把历史 ID 的文本命名当成当前语义，也没有为了旧断言降低五槽模型的生产约束。

## 9. 下一步

P15.5 可审查剩余 12 项中仍由静态源码文本断言组成的 `InputRestore` 三项，先区分真实行为契约与重构后失效的字符串扫描，再决定修生产边界还是把脆弱断言改为可执行语义证据。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p15-4-prepared-run-slot-fixture-regression>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p15-4-prepared-run-slot-fixture-regression/Docs/Report/Dev.D.UE.0.0.10.P15.4.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p15-4-prepared-run-slot-fixture-regression/Docs/Log/Dev.D.UE.0.0.10.P15.4.r0_log.md>
