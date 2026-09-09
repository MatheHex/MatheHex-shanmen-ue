# Dev.D.UE.0.0.10.P23.1.r0 Development Log

## 1. 基线与目标

- base：`6318eaa3b0dd2f2b082fc0817a327d80e2bb868b`（P23.0 Divine Sense reveal）；
- branch：`agent/0.0.10-p23-1-divine-sense-edge-guidance`；
- 目标：让 P23.0 已揭示但无法投影进屏幕的目标仍提供可用探路方向；
- 边界：复用既有 Receipt、GameMode 与 HUD，不改扫描、生命、遮挡、SpiritEnergy、输入、地图或资产，不启动 UI/PIE/产品 executable。

## 2. 缺口定位

P23.0 的 `DrawDivineSenseReveals()` 对每个 Reveal 调用 `ProjectWorldLocationToScreen()`；返回 false 时直接 `continue`。因此敌人在镜头后方时虽然已经被扫描、扣费并写入 Receipt，HUD 却完全不表达它，和人工规划中的“神识透视 / 神识探路”定位冲突。

本轮选择最小可玩修复：保持神识产品权威不动，只补屏幕空间规划与边缘箭头。没有继续延长 P19 的 Controller/Route/Host 链。

## 3. 纯 Marker Plan

新增 `Fdemo_mapShanmenDivineSenseHUDMarkerPlan`，输入仅为画布尺寸、世界投影是否成功、投影像素与摄像机相对 fallback 方位。输出仅包含 Placement、屏幕位置与边缘单位方向。

安全区为左右 36、顶部 112、底部 56。屏内投影原样返回；越界投影从画布中心向投影点作射线；失败投影使用摄像机平面方位。射线与安全矩形求首交点后钳制，数值误差仍由 `IsValid()` 验证边界与单位向量。

失败关闭条件包括：画布小于 320×240、任一必要输入非有限、投影失败且 fallback 为零。所有失败都先清空 `OutPlan`，避免复用旧提示。

## 4. HUD 接线

`Ademo_mapHUD` 每次绘制有效神识 Receipt 时读取一次 Player ViewPoint，从 ViewRotation 得到 Forward 与 Right。目标世界位置仍使用既有 `+90 cm` 标记高度。

投影成功时传入真实像素；失败时计算：

```text
bearing.x = dot(viewToSubject, cameraRight)
bearing.y = -dot(viewToSubject, cameraForward)
```

因此前方对应屏幕上、后方对应屏幕下、左右保持直觉方向。边缘 Marker 在原菱形外增加两条线构成箭头；颜色、距离与 `OCCLUDED` 文本继续由既有 Reveal 决定。右侧 165 px 内的文字向左排，底部 44 px 内的文字向上排。

## 5. 聚焦测试

新增 `demo_mapShanmenDivineSenseHUDPresentationTests.cpp`，4 项：Projected、ProjectedEdge、BehindCamera、Fences。首次编译 6 actions、native 0；首次聚焦 4/0。

最终 Game/Editor 构建后再次运行完整 `Shanmen.0_0_10.Product.DivineSense`，52/0、无 fatal/unhandled/ensure，SHA-256 `2F04B8074EC8EE832D383A07F60FE2A0434E4F554798E240858E2DBC593E82A6`。这份 post-build 日志证明最终 DLL 同时保留 P19/P23.0 产品链与新增布局测试。

## 6. 回归范围与修复轨迹

最初启动了完整 `Shanmen.0_0_10`。它在 721/0 后进入未改动 P12 恢复链的慢速文件/网络探测段；按照“由改动文件推导回归”原则主动终止，不把无关宽树等待当作质量增量。

第一版范围实际运行了父前缀组，共 264 项相关测试且没有测试失败，但覆盖器按既有约定不允许 `ControlledWeaponThreat` 替代 `ControlledWeaponThreatCue` 这类没有点分隔的精确组。首次门禁因此列出 13 个缺失组。保留失败证据后，没有修改 `Test-GroupCoverage`，而是逐项补跑精确组。

最终 18 个精确日志合计 204/0，覆盖门为 `PASS Changed=6 Rules=2 Required=18 Logs=18`。门禁新增 Divine Sense HUD 规则，并把新组加入既有 HUD 规则；一正一反自测使总数从 440 增至 442，最终 442/442。

## 7. 最终测试矩阵

| Scope | Success | Fail |
|---|---:|---:|
| Post-build full Divine Sense product prefix | 52 | 0 |
| 18 changed-file exact mapped groups | 204 | 0 |
| Regression coverage self-test | 442 | 0 |

映射组包含 13 个 `Shanmen.0_0_10.Product.*` 精确表现/输入组、CombatRuntime Divine Sense、Arc pre-launch、Input Choice，以及 `demo_map.InputRestore` 和 `demo_map.V2RangedCompatibility`。所有作为门禁输入的日志都只有一个 RunTests 命令、至少一个 Success、0 Fail、终止成功标记及 0 fatal/unhandled/ensure。

## 8. 构建与工件

- Game Development：5 actions / 23.45s / native 0；
- Editor Development：4 actions / 15.28s / native 0；
- Game build log：2,207 bytes / SHA-256 `E122850B9C915F5B9D47C8F1AB1265DB057FA933DFD05632D05A5A77E9F8F542`；
- Editor build log：2,219 bytes / SHA-256 `64657B3C52BC918FF7F73A7AA452D822AA7A0DFD6DF6388F173F80A83F9797C7`；
- `demo_map.exe`：359,618,560 bytes / SHA-256 `9E6308D7CD480FCFBE2DEE8AE861B90B38C6D18EA49EE7227E024F5EE780D482`；
- `UnrealEditor-demo_map.dll`：18,868,736 bytes / SHA-256 `D932E35E13709B3DD4EED3306943647E16ED8D0A7F6C7239C329DA33644831D2`。

## 9. 静态边界

生产增量为 `demo_mapHUD.cpp` 加纯表现接线，以及一个 57 行 header / 177 行 implementation 的值规划器；测试新增 155 行；回归规则与自测新增 34 行。新增生产代码没有 Timer、SetTimer、自定义 Tick、Sleep、随机数、ApplyDamage、SpawnActor 或 DestroyActor。

没有新增模块依赖、UCLASS/USTRUCT、Actor、Subsystem、资产、存档字段或第二套神识状态。`ShanmenRegressionMap.json` 解析通过，`git diff --check` 为 0。未启动 Unreal Editor UI、PIE、Standalone、产品 executable、真实输入、截图、Smoke、Cook 或 Package。

## 10. 提交边界与 GitHub

只提交本轮 8 个文件：3 个生产文件、1 个测试文件、2 个门禁文件、本 Report 与本 Log。103 个既有 untracked 文件保持未暂存；所有 `Saved/Codex/P23.1` 原始日志继续由 Git ignore。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p23-1-divine-sense-edge-guidance>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p23-1-divine-sense-edge-guidance/Docs/Report/Dev.D.UE.0.0.10.P23.1.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p23-1-divine-sense-edge-guidance/Docs/Log/Dev.D.UE.0.0.10.P23.1.r0_log.md>
