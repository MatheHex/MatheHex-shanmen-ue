# Dev.D.UE.0.0.10.P25.7.r0 Development Log

## 1. 目标

- 为 P25.6 的灵力护盾 World 表现增加可读的低容量状态；
- 正常容量保持青色，剩余容量不高于 25% 时改为琥珀色并增强点光；
- 容量归零继续隐藏，不改变任何战斗、资源、持续时间或输入规则；
- 通过改动文件映射回归、双目标构建，并交接 Report 与 Log。

## 2. 基线与范围

- 基线：`f3f353724cbd495bc2950dfb02834eefb33b4430`（P25.6）；
- 分支：`agent/0.0.10-p25-7-spirit-shield-capacity-visual`；
- 起始 tracked tree clean；
- 用户原有 103 个未跟踪文件保持未暂存；
- 不修改 PlayerController、GameMode、Product Session、Capacity Authority、Resolver、
  输入映射、资产、地图、Widget 或存档 Schema；
- 只修改 World Presentation 适配器与对应自动化测试。

## 3. 设计决策

P25.6 已经建立实际 Pawn 上的 Sphere Shell 与 Point Light，并由 PlayerController 调用无状态
适配器同步。本轮沿用该链路，不增加新的组件或运行时宿主。

低容量阈值采用 `available <= maximum * 0.25`。阈值只决定表现参数：

| State | Shell/Light color | Light intensity | Visible |
|---|---|---:|---|
| Active, capacity > 25% | `(0.08, 0.68, 1.0)` | 1800 | Yes |
| Active, 0 < capacity <= 25% | `(1.0, 0.46, 0.05)` | 2600 | Yes |
| Invalid/inactive/capacity <= 0 | reset stable | 1800 | No |

先判定 `available > 0` 再计算低容量，因此耗尽态不会显示为琥珀色。

## 4. 代码改动

`demo_mapShanmenSpiritShieldWorldPresentation.h/.cpp`：

- 定义稳定与低容量两套固定颜色、点光强度及 25% 阈值；
- `Synchronize()` 读取既有 Capacity Authority 的最大容量；
- 最大容量无效、非正数或 `available > maximum` 时失败关闭；
- `SetAppearance()` 同步更新动态材质 `Color/BaseColor/Emissive` 与点光颜色/强度；
- `HasStableAppearance()`、`HasLowCapacityAppearance()` 与 `GetCueIntensity()` 提供确定性验证；
- `IsGeometryValid()` 只验证几何、物理隔离与动态材质存在，允许两种合法外观；
- 兼容保留 `HasCanonicalMaterialColor()` 的稳定青色语义。

适配器继续没有成员字段，不缓存 Session 数据或历史外观。

## 5. 自动化改动

`demo_mapShanmenSpiritShieldWorldPresentationTests.cpp`：

- 测试身份更新至 P25.7，避免与旧证据身份混用；
- Impact helper 接受明确 RawDamage，供低容量与耗尽场景复用；
- `ActivationAndRelease` 增加稳定青色完整外观断言；
- 新增 `CapacityTone`：真实防御组合、Resolve 与 Commit 把容量 30→6，验证仍可见、
  琥珀色材质、量化后的同色点光以及 2600 强度；
- 保留 `CapacityDepletion` 的真实 30→0 后隐藏证明。

聚焦组最终 5 Success / 0 Fail / Queue Empty；SHA-256：
`684D3FB37152310901806F89C0DC250DC1BFBEEBBABC1295A8F71EF01F8C7F47`。

## 6. 改动映射回归

P25.6 已为三个 World Presentation 路径建立精确映射，本轮无需扩大或修改映射文件。
映射要求：

1. `Shanmen.0_0_10.Product.SpiritShieldWorldPresentation`：5 Success；
2. `Shanmen.0_0_10.Product.SpiritShieldProductSession`：9 Success；
3. `demo_map.V2RangedCompatibility`：22 Success。

合计 36 Success / 0 Fail，三个组均独立执行并正常清空队列。
覆盖门：`PASS Changed=5 Rules=1 Required=3 Logs=3`；SHA-256：
`DE9BDB5AA0B3C8B6F79005ED88FD848E1BF66AD396BA063ED79F732AAB2A190C`。
映射器自检 451/451 PASS；SHA-256：
`27DC9F3C3CECEB5168F19814377827F6DD7B462E96FB4523348DCDF33A8F44FF`。

## 7. 构建结果

| Evidence | Result | Native exit | SHA-256 |
|---|---|---:|---|
| `P25.7_EditorBuild_attempt-1.log` | 首次 Editor 编译 Succeeded | 0 | `96F6D8B8...B1979D7` |
| `P25.7_EditorBuild_final.log` | 最终 Editor Succeeded | 0 | `E119B515...B85B33EC` |
| `P25.7_GameBuild_final.log` | 最终 Game Succeeded | 0 | `B99156D6...8002FE1` |

最终二进制：

- `UnrealEditor-demo_map.dll`：19185664 bytes / SHA-256
  `7EEED8D794175E66B7ABD547A14E4BD5651F95F0A232D5CBCD7CA29B390E691D`；
- `demo_map.exe`：359882752 bytes / SHA-256
  `E163938FFA8C67C10ED465A8C8623E4D701710AED9CFF813FF67229918947036`。

## 8. 静态边界检查

- `git diff --check`：PASS；
- Regression Map：schema 1 / 249 rules，JSON 解析 PASS；
- World Presentation 对自由格式 Diagnostic、`UWorld`、`GetWorld`、Random/RNG、
  Deadline/容量赋值：0 命中；
- 没有新增状态字段、Timer、Actor、碰撞、Overlap、导航或伤害调用；
- 最终项目相关 Unreal 进程：0。

## 9. 首错与证据策略

本阶段编译与测试均在首次尝试通过，无失败日志需要掩盖或修复。所有目标测试日志均由
原生 `Automation RunTests <exact group>` 产生，并包含单一命令、Success 结果与
`Automation Test Queue Empty` 终止标记。启动期 UnifiedError 自测输出不属于目标组，
覆盖门同时确认目标日志中没有 Fatal、Unhandled Exception 或 Ensure condition failed。

`Saved/Codex/P25.7` 保存全部原始构建、测试、覆盖门和自检证据，但按仓库规则不提交。

## 10. P/F 边界与交接

P 阶段完成：源码、真实结算链测试、改动映射回归、覆盖门、静态检查与 Editor/Game 构建
全部通过。

F 阶段未执行：没有启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、
Smoke、Cook 或 Package，不声明最终颜色观感或实机辨识度已经验收。

只提交以下 5 个文件：

1. `Source/demo_map/demo_mapShanmenSpiritShieldWorldPresentation.h`
2. `Source/demo_map/demo_mapShanmenSpiritShieldWorldPresentation.cpp`
3. `Source/demo_map/demo_mapShanmenSpiritShieldWorldPresentationTests.cpp`
4. `Docs/Report/Dev.D.UE.0.0.10.P25.7.r0_report.md`
5. `Docs/Log/Dev.D.UE.0.0.10.P25.7.r0_log.md`

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p25-7-spirit-shield-capacity-visual>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p25-7-spirit-shield-capacity-visual/Docs/Report/Dev.D.UE.0.0.10.P25.7.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p25-7-spirit-shield-capacity-visual/Docs/Log/Dev.D.UE.0.0.10.P25.7.r0_log.md>
