# Dev.D.UE.0.0.10.P23.1.r0 Report

## 1. 结论

P23.1 在 P 阶段边界内完成，结论为 **PASS**。

本轮把 P23.0 的神识揭示从“只显示能投影进屏幕的目标”扩展为可用于探路的完整方向反馈：目标在屏幕内时仍保持世界投影位置；目标越出屏幕或位于镜头后方时，HUD 会把菱形标记钳制到安全边缘，并绘制朝向目标的箭头。可见目标继续使用青色，遮挡目标继续使用金色，距离与 3 秒揭示期保持不变。

```text
Post-build Divine Sense:              52 Success / 0 Fail
Changed-file mapped regression:      204 Success / 0 Fail (18 exact logs)
Changed-file regression coverage:    PASS (6 paths / 2 rules / 18 groups)
Regression gate self-test:            PASS 442/442
Game + Editor Development:            PASS (both native 0)
git diff --check:                      PASS
```

未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。本轮证明最终编译 DLL 上的纯布局、神识产品链、共享 HUD 兼容面与构建边界，不宣称视觉手感、全部分辨率或人工体验验收已经完成。

## 2. 玩家侧变化

神识脉冲揭示的敌人现在分为两种屏幕表达：

- 可读区域内：菱形保持精确世界投影位置；
- 屏幕外或镜头后：菱形停留在屏幕安全边缘，短箭头指向敌人所在方向；
- 青色继续表示无遮挡目标；
- 金色与 `OCCLUDED` 继续表示被墙体等对象遮挡的目标；
- 距离文字在右边缘自动向左排，在底边自动向上排，降低裁切概率。

因此，玩家转身前也能知道神识范围内的敌人大致位于前、后、左、右哪个方向，不再因为投影失败而完全失去该目标。

## 3. 屏幕边缘规则

新增的纯 `Fdemo_mapShanmenDivineSenseHUDMarkerPlan` 使用固定安全区域：左右各 36 px、顶部 112 px、底部 56 px。顶部预留避开既有帮助条与神识状态面板，边缘箭头本身仍留在真实画布范围内。

规则按以下顺序执行：

1. 世界投影成功且坐标位于安全区域时，原样保留；
2. 世界投影成功但坐标越界时，使用“投影点减画布中心”的方向与安全矩形求交；
3. 世界投影失败时，使用摄像机 Right/Forward 与目标方向的点积生成平面方位；
4. 方位射线与安全矩形首次相交的位置成为边缘标记；
5. 画布、投影或方位含非有限值，画布小于 320×240，或失败投影没有有效方位时失败关闭。

身后正中的目标确定性落在底边中央；左右与前后组合会落到相应边或角方向。布局无随机数、时间或 World 状态，相同输入产生完全相同的像素与单位方向。

## 4. 单一权威与 HUD 接线

本轮没有修改 P19/P23.0 的扫描、生命、遮挡、资源或 Receipt 权威。`Ademo_mapHUD` 仍只读取 `Ademo_mapGameMode` 保存的最后一次成功 `FShanmenDivineSenseScanReceipt`。

新增职责只有两部分：HUD 从 PlayerController 取得一次 ViewPoint 并计算摄像机 Forward/Right；纯 Marker Plan 决定屏幕位置与是否绘制边缘箭头。没有新增第二个神识 Controller、Adapter、Timer、Tick、Actor、Subsystem、存档字段或 Gameplay 资源。

箭头只表达 Receipt 中已经存在的目标，不扩大扫描半径，不发现新 Actor，不改变 `WasOccluded()`，也不参与 SpiritEnergy 消耗或技能成功判定。

## 5. 纯布局证明

新增 4 项无头测试：

- `Projected`：屏内位置逐像素保留，fallback 方位不影响结果，重放一致；
- `ProjectedEdge`：屏外右侧投影钳制到 X=1884，箭头方向保持朝右；
- `BehindCamera`：投影失败且方位为正后方时落到 `(960, 1024)`；
- `Fences`：过小画布、非有限输入和零方位全部失败关闭，并清空复用输出。

最终编译后的 DLL 上运行完整 `Shanmen.0_0_10.Product.DivineSense`，结果为 52/0；日志 333,340 bytes，SHA-256 `2F04B8074EC8EE832D383A07F60FE2A0434E4F554798E240858E2DBC593E82A6`。

## 6. 验证范围修正

本轮按董事会复审建议使用 changed-file 映射，不把主题相关性当作覆盖依据。一次非必要的完整 `Shanmen.0_0_10` 运行在 721 Success / 0 Fail 后进入未改动的历史 P12 恢复链慢段，因其不会增加本轮映射覆盖而主动终止；该部分日志保留但不作为最终门禁证据。

首次覆盖检查使用了 `ControlledWeaponThreat`、`ThrownWeaponArcEditing` 等父前缀日志。UE 实际运行了子测试，但既有覆盖器要求精确组名，因此正确拒绝并列出 13 个缺失精确组。没有放宽覆盖器；随后逐一运行精确组并复查通过。失败复现日志保留在 `P23.1_regression_coverage_initial_failure.log`。

## 7. 改动文件回归

最终门禁为 `PASS Changed=6 Rules=2 Required=18 Logs=18`。18 份日志合计 4,959,092 bytes、204 Success / 0 Fail，覆盖：

- 神识 HUD 表现、神识物理输入与 CombatRuntime Divine Sense；
- 主 HUD 共用的 Controlled Weapon threat cue/readout；
- Thrown Weapon HUD stack/layout、Arc preview、Arc editing、trajectory 与 input choice；
- `demo_map.InputRestore` 101 项与 `demo_map.V2RangedCompatibility` 22 项。

覆盖结果日志 6,709 bytes，SHA-256 `C4C03D1ACBB170667FDBE97F114D03E942836D51C7D2E61F81747754930ECF56`。映射新增一条 Divine Sense HUD 精确规则，并让 `demo_mapHUD.cpp` 的既有规则必须包含新表现组；门禁自测从 440 增至 442，最终日志 43,595 bytes，SHA-256 `29257A884331C8F3D27AE663C0840476DF6EE3BBFDCDE346733BD5699DFFFD4F`。

## 8. 构建与静态边界

最终构建均使用 UE 5.8 Development、`-WaitMutex`、`-NoHotReloadFromIDE` 与最多 2 个并行动作：

| Evidence | Result | Bytes | SHA-256 |
|---|---|---:|---|
| `P23.1_GameBuild_final.log` | PASS / 5 actions / native 0 | 2,207 | `E122850B9C915F5B9D47C8F1AB1265DB057FA933DFD05632D05A5A77E9F8F542` |
| `P23.1_EditorBuild_final.log` | PASS / 4 actions / native 0 | 2,219 | `64657B3C52BC918FF7F73A7AA452D822AA7A0DFD6DF6388F173F80A83F9797C7` |

最终 `demo_map.exe` 为 359,618,560 bytes，SHA-256 `9E6308D7CD480FCFBE2DEE8AE861B90B38C6D18EA49EE7227E024F5EE780D482`；`UnrealEditor-demo_map.dll` 为 18,868,736 bytes，SHA-256 `D932E35E13709B3DD4EED3306943647E16ED8D0A7F6C7239C329DA33644831D2`。

新增生产代码中的 Timer/SetTimer、自定义 Tick、Sleep、Random/Rand/RNG、ApplyDamage、SpawnActor 与 DestroyActor 均为 0；JSON 可解析，`git diff --check` 为 0。

## 9. P/F 边界

PASS：屏内投影保留；越界投影进入安全边缘；镜头后目标获得摄像机相对方位；边缘箭头与距离文字保持画布内侧；青/金与遮挡语义未改变；最终 DLL 神识树 52/0；改动映射回归 204/0；覆盖门、自测和双构建通过。

未声明：箭头美术、本地化、超宽屏/竖屏/分屏、复杂摄像机模式、多人客户端 HUD、手柄触感或人工游玩已验收。安全区域当前是 0.0.10 原型固定像素策略，后续若 UI 缩放体系统一，应迁移到该体系而不是产生第二份边距配置。

## 10. 提交边界与 GitHub

本阶段只提交 3 个生产文件、1 个测试文件、2 个回归规则文件、本 Report 与本 Development Log，共 8 个文件。用户原有 103 个 untracked 文件保持未暂存；`Saved/Codex/P23.1` 的原始证据不进入 Git。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p23-1-divine-sense-edge-guidance>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p23-1-divine-sense-edge-guidance/Docs/Report/Dev.D.UE.0.0.10.P23.1.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p23-1-divine-sense-edge-guidance/Docs/Log/Dev.D.UE.0.0.10.P23.1.r0_log.md>
