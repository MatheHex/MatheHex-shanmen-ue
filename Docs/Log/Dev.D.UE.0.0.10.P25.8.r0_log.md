# Dev.D.UE.0.0.10.P25.8.r0 Development Log

## 1. 目标

- 让 P25.7 的灵力护盾 World 轮廓连续反映权威剩余容量；
- 满容量保持原尺寸，非零容量下降时在安全范围内收缩；
- 耗尽、关闭、释放或无效状态仍隐藏并清除旧尺寸；
- 不改变护盾容量、成本、Deadline、伤害、输入或物理规则；
- 通过改动文件映射回归、双目标构建并推送 Report 与 Log。

## 2. 基线与范围

- 基线：`98a2fccad4b09fd72e5e99b9f47fe5230bbd7f45`（P25.7）；
- 分支：`agent/0.0.10-p25-8-spirit-shield-capacity-geometry`；
- 起始 tracked tree clean；
- 用户原有 103 个未跟踪文件保持未暂存；
- 不修改 PlayerController、GameMode、Product Session、Capacity Authority、Resolver、输入、
  资产、地图、Widget、回归映射或存档 Schema；
- 只修改 World Presentation 适配器及其自动化测试。

## 3. 表现公式

定义两个纯表现常量：

- 满容量尺寸：`(1.35, 1.35, 2.10)`；
- 最小非零表现尺寸：`(1.20, 1.20, 1.90)`。

每次同步根据当前权威容量计算：

```text
fraction = clamp(available / maximum, 0, 1)
scale = minimum + (full - minimum) * fraction
```

下限仍包覆角色，且组件继续完全无碰撞。缩放不是业务状态，不反向影响容量、吸收或伤害。

## 4. 适配器改动

`demo_mapShanmenSpiritShieldWorldPresentation.h/.cpp`：

- 将固定罩体尺寸拆分为 Full 与 Minimum 两个固定表现常量；
- 新增局部 `CalculateCapacityScale()` 与安全边界检查；
- `Synchronize()` 在合法非零会话中读取 available/maximum 并设置精确比例尺寸；
- 无效最大容量、非有限容量或 available 超过 maximum 时复位并失败关闭；
- `IsGeometryValid()` 接受安全范围内的动态尺寸，而非只接受满尺寸；
- `HasCapacityScale()` 以冻结容量重算期望值并验证当前组件；
- `GetShellScale()` 提供只读自动化/诊断视图；
- 隐藏态统一复位 Full Scale，避免上一会话视觉残留。

类仍无成员字段、Timer、World 查询、随机数或业务写入。

## 5. 自动化改动

`demo_mapShanmenSpiritShieldWorldPresentationTests.cpp`：

- 测试身份更新为 P25.8；
- `GeometryFailClosed` 验证隐藏态满尺寸；
- `ActivationAndRelease` 验证 30/30 的精确满尺寸；
- 新增 `CapacityGeometry`：真实 Impact 使容量 30→15，验证青色保持且尺寸为
  `(1.275, 1.275, 2.00)`；
- `CapacityTone` 增强为同时验证 30→6 后的琥珀色与
  `(1.23, 1.23, 1.94)` 尺寸；
- `CapacityDepletion` 增强为验证 30→0 后隐藏且尺寸复位。

聚焦组最终 6 Success / 0 Fail / Queue Empty；SHA-256：
`42C1E92FF029EFC9CD02A8AA535F11331FC0B484AEE1506091CDEDF820F53BCA`。

## 6. 改动映射回归

三个实现/测试路径继续命中 P25.6 建立的 World Presentation 规则，无需修改映射文件。

1. `Shanmen.0_0_10.Product.SpiritShieldWorldPresentation`：6 Success；
2. `Shanmen.0_0_10.Product.SpiritShieldProductSession`：9 Success；
3. `demo_map.V2RangedCompatibility`：22 Success。

合计 37 Success / 0 Fail，三个组均独立执行并正常清空队列。
覆盖门：`PASS Changed=5 Rules=1 Required=3 Logs=3`；SHA-256：
`F2464768128F1548883E37943B8E1482D9A848709E63610F433BE121390181ED`。
映射器自检 451/451 PASS；SHA-256：
`27DC9F3C3CECEB5168F19814377827F6DD7B462E96FB4523348DCDF33A8F44FF`。

## 7. 构建结果

| Evidence | Result | Native exit | SHA-256 |
|---|---|---:|---|
| `P25.8_EditorBuild_attempt-1.log` | 首次 Editor 编译 Succeeded | 0 | `74FD734D...783F9FD` |
| `P25.8_EditorBuild_final.log` | 最终 Editor Succeeded | 0 | `C842D0A8...0ADA563` |
| `P25.8_GameBuild_final.log` | 最终 Game Succeeded | 0 | `E1F297A7...75F561` |

最终二进制：

- `UnrealEditor-demo_map.dll`：19191296 bytes / SHA-256
  `41D1A72AC4A053B7ADF23A953236498D593E3971A8EAC1CBF47B1EBF2D78FC1A`；
- `demo_map.exe`：359886848 bytes / SHA-256
  `318647C4BD3B24E2738B23F54885AAFE618AAB649D0372BF9C9D0BF374266AA1`。

## 8. 静态边界检查

- `git diff --check`：PASS；
- Regression Map：schema 1 / 249 rules，JSON 解析 PASS；
- World Presentation 对自由格式 Diagnostic、`UWorld`、`GetWorld`、Random/RNG、
  Deadline/容量赋值：0 命中；
- 没有新增状态字段、Timer、Actor、组件、碰撞、Overlap、导航或伤害调用；
- 最终项目相关 Unreal 进程：0。

## 9. 首错与证据策略

本阶段首次编译与三个首次测试组全部通过，无失败证据需要修复或隐藏。所有目标日志均由
原生 `Automation RunTests <exact group>` 产生，包含唯一命令、Success 结果和
`Automation Test Queue Empty` 终止标记，并且无 Fail、Fatal、Unhandled Exception 或
Ensure condition failed。

`Saved/Codex/P25.8` 保存全部原始构建、测试、覆盖门与自检证据，但不进入 Git。

## 10. P/F 边界与交接

P 阶段完成：源码、真实结算链几何证明、路径映射回归、覆盖门、静态检查与 Editor/Game
双目标构建全部通过。

F 阶段未执行：没有启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、
Smoke、Cook 或 Package，不声明最终画面或实机辨识度已验收。

只提交以下 5 个文件：

1. `Source/demo_map/demo_mapShanmenSpiritShieldWorldPresentation.h`
2. `Source/demo_map/demo_mapShanmenSpiritShieldWorldPresentation.cpp`
3. `Source/demo_map/demo_mapShanmenSpiritShieldWorldPresentationTests.cpp`
4. `Docs/Report/Dev.D.UE.0.0.10.P25.8.r0_report.md`
5. `Docs/Log/Dev.D.UE.0.0.10.P25.8.r0_log.md`

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p25-8-spirit-shield-capacity-geometry>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p25-8-spirit-shield-capacity-geometry/Docs/Report/Dev.D.UE.0.0.10.P25.8.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p25-8-spirit-shield-capacity-geometry/Docs/Log/Dev.D.UE.0.0.10.P25.8.r0_log.md>
