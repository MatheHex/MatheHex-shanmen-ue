# Dev.D.UE.0.0.10.P25.9.r0 Development Log

## 1. 目标

- 让 P25.8 的灵力护盾 World 表现读取唯一固定 Combat Run 时间线；
- 用不干扰容量颜色与几何的光域半径表达剩余寿命；
- 在权威 Deadline 样本到达时立即隐藏，消除 Session 关闭观察前的陈旧表现窗口；
- 对空、无效、外来或早于起点的时间线样本失败关闭；
- 通过改动文件映射回归、双目标构建并推送 Report 与 Log。

## 2. 基线与范围

- 基线：`557b530f6c405b39bfb94ea7590b45124303e54b`（P25.8）；
- 分支：`agent/0.0.10-p25-9-spirit-shield-deadline-visual`；
- 起始 tracked tree clean，全部其余未跟踪文件保持未暂存；
- 修改 World Presentation、PlayerController 调用面、自动化测试及回归映射；
- 不修改 GameMode、Product Session、Timeline、Capacity Authority、Resolver、输入定义、
  资产、地图、Widget 或存档 Schema。

## 3. 表现公式

固定表现常量：满寿命光域 `220`，截止前下限 `120`。每次同步重新计算：

```text
durationTicks = deadlineTick - startTick
remainingTicks = deadlineTick - currentTick
remainingFraction = clamp(remainingTicks / durationTicks, 0, 1)
attenuationRadius = lerp(120, 220, remainingFraction)
```

合法显示窗口为 `[startTick, deadlineTick)`。Schedule 自身保证持续时间为正；Presentation
额外验证 Sample、TimelineId 和起点边界。Deadline 样本隐藏视觉但不调用 Session 关闭 API。

## 4. 实现改动

`demo_mapShanmenSpiritShieldWorldPresentation.h/.cpp`：

- `Synchronize()` 新增冻结 Timeline Sample 参数；
- Sample 缺失、无效、TimelineId 不匹配或早于起点时，复位并失败关闭；
- 新增 `CalculateLifetimeRadius()`、`HasLifetimeRadius()`、
  `GetCueAttenuationRadius()` 与统一半径 Setter；
- 合法非零容量会话同时验证容量几何和寿命半径；
- 截止、关闭、耗尽、释放或空会话统一隐藏并复位光域 `220`；
- 类仍为无状态适配器，不持有 Timer、World、Sample、Session 或历史半径。

`demo_mapPlayerController.cpp`：每次 PlayerTick 从 GameMode 的现有固定时间线捕获一次 Sample，
把同一冻结值交给 Presentation；捕获失败时传空并失败关闭。OnUnPossess 同时传空 Session 与
Sample，保持所有权清理。

## 5. 自动化改动

`LifetimeRadius` 新测试在真实 Product Session 与固定 Timeline 上证明：

1. tick 0：半径 `220`；
2. tick 45/90：半径 `170`，罩体仍为满容量尺寸；
3. 同 tick 的外来 TimelineId：失败、隐藏、Session 仍 Active；
4. 恢复正确 Sample：重新显示；
5. tick 90：Session 尚 Active，Presentation 已隐藏且未改写 Session。

其余 6 个既有测试继续覆盖安装/释放、权威关闭、半/低/零容量几何和颜色。聚焦组最终
7 Success / 0 Fail / Queue Empty；SHA-256：
`06699D2BD668B2FD2E95F3C69EAE67E38A47C31FC4022844D39D330732789D2D`。

## 6. 改动映射回归

World Presentation 规则新增固定时间线组；对应自检正例也必须提供该证据。PlayerController
继续触发其完整交叉输入面。30 个精确组均独立执行：

| Exact group | Success |
|---|---:|
| `Shanmen.0_0_10.Product.SpiritShieldWorldPresentation` | 7 |
| `demo_map.InputRestore` | 101 |
| `demo_map.V2RangedCompatibility` | 22 |
| `Shanmen.0_0_10.Product.CombatRunFixedTimeline` | 5 |
| `Shanmen.0_0_10.Product.ControlledWeaponInputAdapter` | 4 |
| `Shanmen.0_0_10.Product.ControlledWeaponPhysicalInput` | 7 |
| `Shanmen.0_0_10.Product.DivineSenseLogicalInputAdapter` | 7 |
| `Shanmen.0_0_10.Product.DivineSensePhysicalInput` | 5 |
| `Shanmen.0_0_10.Product.MeridianShockTreatment` | 32 |
| `Shanmen.0_0_10.Product.SpiritEvasionInputAdapter` | 6 |
| `Shanmen.0_0_10.Product.SpiritEvasionPhysicalInput` | 6 |
| `Shanmen.0_0_10.Product.SpiritShieldPhysicalInput` | 3 |
| `Shanmen.0_0_10.Product.SpiritShieldProductSession` | 9 |
| `Shanmen.0_0_10.Product.SwordQiPhysicalInput` | 5 |
| `Shanmen.0_0_10.Product.ThrownWeaponArcConfirmationOwner` | 6 |
| `Shanmen.0_0_10.Product.ThrownWeaponArcEditingControllerRoute` | 7 |
| `Shanmen.0_0_10.Product.ThrownWeaponArcEditingInteractionComposition` | 7 |
| `Shanmen.0_0_10.Product.ThrownWeaponArcEditingPhysicalInput` | 10 |
| `Shanmen.0_0_10.Product.ThrownWeaponArcLaunchInputAdapter` | 6 |
| `Shanmen.0_0_10.Product.ThrownWeaponArcPreLaunchPreviewContext` | 3 |
| `Shanmen.0_0_10.Product.ThrownWeaponHotbarConfirmationAdapter` | 7 |
| `Shanmen.0_0_10.Product.ThrownWeaponInputAdapter` | 11 |
| `Shanmen.0_0_10.Product.ThrownWeaponInputChoiceControllerAdapter` | 8 |
| `Shanmen.0_0_10.Product.ThrownWeaponInputChoiceIntentAdapter` | 8 |
| `Shanmen.0_0_10.Product.ThrownWeaponInputChoiceInteractionPort` | 7 |
| `Shanmen.0_0_10.Product.ThrownWeaponInputChoiceInteractionRequestCoordinator` | 7 |
| `Shanmen.0_0_10.Product.ThrownWeaponTrajectoryTogglePhysicalInput` | 7 |
| `Shanmen.0_0_10.Product.WeaponGuardInputAdapter` | 6 |
| `Shanmen.0_0_10.Product.WeaponGuardPhysicalInput` | 6 |
| `Shanmen.0_0_10.Product.WeaponGuardProductSession` | 7 |
| **合计** | **332** |

全部 0 Fail 且队列正常清空。覆盖门：
`PASS Changed=8 Rules=2 Required=30 Logs=30`；SHA-256：
`5346ACE9EF0320697BF50ADAF421C28D16EDB411FF86BDDC6AF1D189DC34CB8C`。映射器自检 451/451 PASS；SHA-256：
`B57B2D40AF9919FC707608DBD5B14F5BE11635C928D15A2C37E954681424FAA7`。

## 7. 构建结果

| Evidence | Result | Native exit | SHA-256 |
|---|---|---:|---|
| `P25.9_EditorBuild_attempt-1.log` | 首次 Editor 编译 Succeeded | 0 | `209F7B3B...D49694` |
| `P25.9_EditorBuild_final.log` | 最终 Editor Succeeded | 0 | `FC394553...AF437` |
| `P25.9_GameBuild_final.log` | 最终 Game Succeeded | 0 | `9769FD33...FEBCE` |

最终二进制：

- `UnrealEditor-demo_map.dll`：19196928 bytes / SHA-256
  `40233B81948DB3CF7363563B055A993B0EB60A02150AF3AFFB6FE68732B82881`；
- `demo_map.exe`：359891968 bytes / SHA-256
  `3179FA3BA59679D465F66C0F4571DC426092B1A7951DF75A56AB441ADBA033B9`。

## 8. 静态边界检查

- `git diff --check`：PASS；Regression Map schema 1 / 249 rules；
- World Presentation 对 `UWorld/GetWorld`、Timer、RNG、Session 写 API、Timeline 非 Getter
  操作均 0 命中；边界证据 SHA-256：
  `300B6E6B83C57081DEC2B750DCB8DC1FF396D493319E88FFC9E1C57A43CC9A2B`；
- 没有新增状态字段、Actor、组件、碰撞、Overlap、导航、伤害或资源调用；
- 最终项目相关 Unreal 进程：0。

## 9. 首错、P/F 边界与证据

首次编译、全部 30 个精确测试组与最终双构建均一次通过，没有产品失败。P 阶段已覆盖
公式、时间线身份、截止隐藏、跨控制器回归及构建。F 阶段未启动 Editor UI、PIE、
Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package，不声明目视验收。

`Saved/Codex/P25.9` 保存全部原始构建、测试、覆盖门、自检与静态证据，不进入 Git。

## 10. GitHub 交接

只提交以下 8 个文件：

1. `Source/demo_map/demo_mapShanmenSpiritShieldWorldPresentation.h`
2. `Source/demo_map/demo_mapShanmenSpiritShieldWorldPresentation.cpp`
3. `Source/demo_map/demo_mapShanmenSpiritShieldWorldPresentationTests.cpp`
4. `Source/demo_map/demo_mapPlayerController.cpp`
5. `Scripts/ShanmenRegressionMap.json`
6. `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`
7. `Docs/Report/Dev.D.UE.0.0.10.P25.9.r0_report.md`
8. `Docs/Log/Dev.D.UE.0.0.10.P25.9.r0_log.md`

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p25-9-spirit-shield-deadline-visual>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p25-9-spirit-shield-deadline-visual/Docs/Report/Dev.D.UE.0.0.10.P25.9.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p25-9-spirit-shield-deadline-visual/Docs/Log/Dev.D.UE.0.0.10.P25.9.r0_log.md>
