# Dev.D.UE.0.0.10.P25.3.r0 Report

## 1. 结论

P25.3 已把 P25.2 的短时灵力护盾接入现有 MainHUD。玩家激活护盾后，可以直接看到剩余容量、距固定期限的剩余秒数和 `H` 键提示；容量降低到四分之一时转为低容量警告，容量耗尽后仍明确显示 `DEPLETED`，直到既有产品会话按统一 Combat Run 时间线关闭。

HUD 没有保存第二份护盾状态、余额或计时器。每帧只读消费 `Fdemo_mapShanmenSpiritShieldProductSession` 与 `Fdemo_mapShanmenCombatRunFixedTimeline`，再交给纯展示投影格式化。

## 2. 玩家可观察行为

- 活动护盾以青色紧凑条显示：`SPIRIT SHIELD  30 / 30  ·  3.0s  ·  [H]`；
- 剩余容量不高于 25% 时显示琥珀色 `SPIRIT SHIELD LOW`；
- 容量为零但会话尚未到期时显示灰色 `SPIRIT SHIELD DEPLETED`，避免玩家误以为护盾仍能吸收伤害；
- 最后一个 30 Hz Tick 至少显示 `0.1s`，不会提前显示为 `0.0s`；
- 未激活、已到期、非法容量、非法时间线或空按键标签均失败关闭，不绘制伪状态；
- 顶部现有操作提示新增 `<key> Shield`，按键名称继续来自统一 Input Binding Settings，而不是硬编码显示文本。

## 3. 单一状态与时间源

MainHUD 的读取链为：

1. GameMode 暴露现有 Spirit Shield Product Session；
2. Product Session 提供活动状态、可用容量、最大容量和 Deadline Tick；
3. GameMode 暴露同一 Combat Run Fixed Timeline 的 Current Tick；
4. 纯 `Fdemo_mapShanmenSpiritShieldHUDPresentation` 只做验证、分级与文本格式化；
5. HUD 根据展示 Tone 选择颜色并绘制。

没有创建 Actor、Component、Subsystem、Timer、墙钟、随机数或可写 Session 入口。期限换算固定使用既有 `CanonicalTicksPerSecond() == 30`。

## 4. 展示投影契约

新增纯投影值包含：

- `Stable / Low / Depleted` 三种有效 Tone；
- 冻结的当前容量、最大容量、剩余 Tick 和剩余秒数；
- 单条可读文本；
- `IsValid()` 与 `Matches()`，用于失败关闭和确定性重放证明。

投影入口先清空可复用输出。任何非法输入都不会残留上一次有效 HUD 数据。容量接近零时规范化为零；容量文本仅在确有小数时保留一位，时间向上取到 0.1 秒，避免显示值早于权威 Deadline。

## 5. MainHUD 接线

护盾条复用当前 Canvas HUD 的面板与阴影文本绘制方式，位于左侧状态区的健康值旁。颜色只表达展示优先级：稳定为青色、低容量为琥珀色、耗尽为灰色。

接线不调用护盾激活、时间推进、容量提交或敌方 Impact；这些写入仍全部留在 P25.1/P25.2 的既有产品边界中。HUD 也不会因不可读状态自行修复或猜测剩余量。

## 6. 自动化证明

新增 4 项聚焦测试：

- `Stable`：30/30、90 Tick、3.0 秒和确定性文本；
- `Low`：25% 阈值、7.5 容量、61 Tick 向上显示 2.1 秒；
- `Depleted`：活动但容量为零，最后一 Tick 显示 0.1 秒；
- `Fences`：未激活、到期、非法 Tick Rate、越界/NaN 容量和空按键全部失败关闭，并清空复用输出。

聚焦组 `Shanmen.0_0_10.Product.SpiritShieldHUDPresentation` 为 4 Success / 0 Fail，原生终止码 0，日志 SHA-256 为 `D9D199E063C290B3166005CFF2799E2B6D672BF82A82351B33EA6F2A7C050CC3`。

## 7. 改动文件驱动回归

本轮把新投影加入 `Scripts/ShanmenRegressionMap.json`，并让 MainHUD 规则显式要求护盾 HUD 与 Product Session 证据。按 `git diff --name-only` 对应关系逐组执行 20 份独立日志：

| 范围 | Success | Fail |
|---|---:|---:|
| Spirit Shield HUD + Product Session | 12 | 0 |
| MainHUD 既有展示、输入与投掷武器依赖 | 90 | 0 |
| `demo_map.InputRestore` + `demo_map.V2RangedCompatibility` | 123 | 0 |
| 合计 | 225 | 0 |

每份日志均只有一个 `RunTests` 命令、唯一原生成功终止标记、0 Fatal、0 Unhandled、0 Ensure、0 `generate_204`。覆盖门禁：`PASS Changed=6 Rules=2 Required=20 Logs=20`，SHA-256 `60E7CF8B017D88D9C6789BC218514700D4C75C872A9BF212A9C00617E137E3AE`。

覆盖器自检补充了新规则的正向与失败关闭样例，最终 448/448 PASS，SHA-256 `6328D811B01534276A7F15365654B6C6B2B8F2ED1B3AE91739D68B1390F6B7BE`。

## 8. 首错、构建与静态检查

产品源码、编译和 Automation 没有失败。唯一首错来自回归覆盖器自检：MainHUD 的旧正向样例未携带新加入的 Shield HUD 与 Product Session 两份证据，因此被门禁正确拒绝。补齐正向样例，并新增“只有 HUD 聚焦证据仍应拒绝”的负向样例后通过；首错日志 SHA-256 为 `DC9E9A768615CA6CE2A5B6BE8A7FECBC1CF05D3A75CE3A3AA6B2E15A6F9FC1B1`。

| Target | Result | Native exit | SHA-256 |
|---|---|---:|---|
| `demo_mapEditor Win64 Development` | Succeeded | 0 | `697A73C74F9C7A6CF2C96B1055DDA8F86E8924BCE1350840CE7FF23573F106ED` |
| `demo_map Win64 Development` | Succeeded | 0 | `53A417FE19CBA2773C3F7DBAFBFD6F0F46956642969428D073A05995B0710A6C` |

- `git diff --cached --check`：PASS；
- 纯投影对 `UWorld`、`AActor`、`GetWorld`、Timer、墙钟和 RNG：0 命中；
- 最终项目相关进程：0；
- `UnrealEditor-demo_map.dll`：19125248 bytes，SHA-256 `94FF5D61A03AE50A7A1302BBF954E1A3CD4870F539AAEE3D5AB373A5AF1C135A`；
- `demo_map.exe`：359832576 bytes，SHA-256 `8D22F8A47B8F25D74E691B5323529040428D4E11C98A423655E1C8FD852303F5`。

## 9. 验证范围说明

首次全量命令遗漏既有 Home Screen 进程级覆盖，在 756/0 时因无关联网探测拖慢而停止；第二次加覆盖后验证 `generate_204=0`，在 743/0 时依据本轮已采用的路径驱动回归策略停止。两份日志都没有终止标记，均只作为诊断证据保留，绝不计作完整全量通过。

本轮正式放行只声明映射要求的 20 组、225 项测试全部成功。没有启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package，因此不声明已目视确认 HUD 尺寸、颜色或真实按键画面。

## 10. GitHub 交接

基线提交：`94e8145ca8e9ca4ff0d5592ae55c5199965aeff2`（P25.2）。

工作分支：`agent/0.0.10-p25-3-spirit-shield-hud`。

本阶段只提交 3 个 HUD 投影实现/测试文件、现有 HUD 接线、2 个回归映射/自检文件、本 Report 与本 Development Log。用户原有 103 个未跟踪文件保持未暂存；`Saved/Codex/P25.3` 原始证据不进入 Git。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p25-3-spirit-shield-hud>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p25-3-spirit-shield-hud/Docs/Report/Dev.D.UE.0.0.10.P25.3.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p25-3-spirit-shield-hud/Docs/Log/Dev.D.UE.0.0.10.P25.3.r0_log.md>
