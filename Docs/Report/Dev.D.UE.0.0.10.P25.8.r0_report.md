# Dev.D.UE.0.0.10.P25.8.r0 Report

## 1. 结论

P25.8 已让灵力护盾的 World 轮廓连续反映当前权威容量。满容量保持 P25.6 的
`(1.35, 1.35, 2.10)` 罩体尺寸；容量下降时，罩体按剩余比例向安全下限
`(1.20, 1.20, 1.90)` 线性收缩。P25.7 的青色/琥珀色容量提示保持不变，容量归零仍立即
隐藏并复位尺寸。

本轮只扩展既有无状态 World Presentation 适配器和测试，不新增计时器、容量副本、
战斗规则、组件、Actor、碰撞或第二权威。

## 2. 玩家可观察行为

- 满容量 30/30：罩体尺寸 `(1.35, 1.35, 2.10)`；
- 半容量 15/30：罩体尺寸 `(1.275, 1.275, 2.00)`；
- 低容量 6/30：罩体尺寸 `(1.23, 1.23, 1.94)`，并保持 P25.7 的琥珀色提示；
- 任意合法非零容量：尺寸在安全下限与满容量尺寸之间连续插值；
- 容量归零、到期、释放或无效：能量罩与点光隐藏，罩体复位为满容量尺寸；
- 尺寸变化不参与碰撞、导航、伤害、吸收量、持续时间或输入判定。

## 3. 容量比例投影

World Presentation 每次同步只读取既有 Capacity Authority：

```text
fraction = clamp(available / maximum, 0, 1)
scale = lerp(minimumScale, fullScale, fraction)
```

只有会话有效、激活且 `available > 0` 时才计算容量尺寸。最大容量无效、非正数、可用容量
非有限值或 `available > maximum` 时失败关闭、复位稳定外观与满尺寸，并隐藏组件。

## 4. 几何与权威边界

- `IsGeometryValid()` 允许且只允许安全下限到满容量尺寸之间的有限缩放；
- `HasCapacityScale()` 根据调用方提供的冻结容量重新计算并核验精确尺寸；
- `GetShellScale()` 只暴露当前表现组件的只读尺寸，不能修改业务状态；
- 适配器没有成员字段，不缓存容量比例、过去尺寸或 Session；
- 不写入容量、Deadline、Timeline、Run、Activation 或 Impact 身份；
- 延续 `NoCollision`、Overlap=false、Navigation=false、CastShadow=false；
- 隐藏态复位满尺寸，避免旧会话的收缩外观泄漏到下一次激活。

## 5. 自动化证明

精确组 `Shanmen.0_0_10.Product.SpiritShieldWorldPresentation`：

| Test | Result |
|---|---|
| `GeometryFailClosed` | Success |
| `ActivationAndRelease` | Success |
| `DeadlineClosure` | Success |
| `CapacityGeometry` | Success |
| `CapacityTone` | Success |
| `CapacityDepletion` | Success |

新增 `CapacityGeometry` 通过真实防御组合、Resolve 与 Commit 把容量 30→15，证明尺寸精确
收缩到 `(1.275, 1.275, 2.00)`，同时仍保持高于 25% 时的稳定青色外观。
`CapacityTone` 进一步证明容量 30→6 时尺寸精确为 `(1.23, 1.23, 1.94)`；
`CapacityDepletion` 证明 30→0 后隐藏并复位满尺寸。

最终 6 Success / 0 Fail / 队列正常清空；日志 SHA-256：
`42C1E92FF029EFC9CD02A8AA535F11331FC0B484AEE1506091CDEDF820F53BCA`。

## 6. 改动文件驱动回归

本阶段 5 个提交路径中，3 个实现/测试路径命中既有 World Presentation 映射；Report 与 Log
为忽略路径。映射要求并独立执行 3 个精确组：

| Group | Success | Fail | Log SHA-256 |
|---|---:|---:|---|
| `Shanmen.0_0_10.Product.SpiritShieldWorldPresentation` | 6 | 0 | `42C1E92F...F53BCA` |
| `Shanmen.0_0_10.Product.SpiritShieldProductSession` | 9 | 0 | `2B3C493E...EA6935` |
| `demo_map.V2RangedCompatibility` | 22 | 0 | `ED4E0D02...EAC15E` |
| **合计** | **37** | **0** | — |

覆盖门结果：`PASS Changed=5 Rules=1 Required=3 Logs=3`；证据 SHA-256：
`F2464768128F1548883E37943B8E1482D9A848709E63610F433BE121390181ED`。
映射器自检 451/451 PASS；SHA-256：
`27DC9F3C3CECEB5168F19814377827F6DD7B462E96FB4523348DCDF33A8F44FF`。

## 7. 构建与静态检查

| Target | Result | Native exit | Log SHA-256 |
|---|---|---:|---|
| `demo_mapEditor Win64 Development` | Succeeded | 0 | `C842D0A8BED23BBD43C099BB505C931B2AE51C1240049A52D0E1FE8A90ADA563` |
| `demo_map Win64 Development` | Succeeded | 0 | `E1F297A7EDD0D33EA466E2FD2C9AC71351EE9905C867E16728C5571CFF75F561` |

- 首次 Editor 编译一次通过；日志 SHA-256：
  `74FD734D2447AB73185D1A9DF8E33BC52EA874777125CCCCBB4AD5C79783F9FD`；
- `git diff --check`：PASS；
- Regression Map JSON：schema 1 / 249 条规则，PASS；
- 适配器对 `Diagnostic`、`UWorld`、`GetWorld`、RNG、Deadline/容量赋值：0 命中；
- 最终项目相关 Unreal 进程：0；
- `UnrealEditor-demo_map.dll`：19191296 bytes，SHA-256
  `41D1A72AC4A053B7ADF23A953236498D593E3971A8EAC1CBF47B1EBF2D78FC1A`；
- `demo_map.exe`：359886848 bytes，SHA-256
  `318647C4BD3B24E2738B23F54885AAFE618AAB649D0372BF9C9D0BF374266AA1`。

## 8. 首错与修复

本阶段首次 Editor 编译、首次聚焦测试、两组首次映射回归及最终双目标构建均一次通过，
没有产品失败或源码失败需要修复。37 项目标测试全部为 Success，且最终证据没有 Fail、
Fatal、Unhandled Exception 或 Ensure condition failed 标记。

## 9. P/F 边界

P 阶段已证明：容量比例公式、满/半/低容量精确尺寸、低容量颜色组合、耗尽隐藏与尺寸复位、
既有 Session/远程兼容、覆盖门及 Editor/Game 双目标构建成立。

F 阶段未执行：没有启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、
Smoke、Cook 或 Package，因此不声明尺寸收缩的最终画面层次或实机可辨识度已经目视验收。

## 10. GitHub 交接

基线提交：`98a2fccad4b09fd72e5e99b9f47fe5230bbd7f45`（P25.7）。
分支：`agent/0.0.10-p25-8-spirit-shield-capacity-geometry`。
只提交本阶段 3 个实现/测试文件、本 Report 与本 Development Log；用户原有 103 个未跟踪
文件保持未暂存，`Saved/Codex/P25.8` 原始证据不进入 Git。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p25-8-spirit-shield-capacity-geometry>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p25-8-spirit-shield-capacity-geometry/Docs/Report/Dev.D.UE.0.0.10.P25.8.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p25-8-spirit-shield-capacity-geometry/Docs/Log/Dev.D.UE.0.0.10.P25.8.r0_log.md>
