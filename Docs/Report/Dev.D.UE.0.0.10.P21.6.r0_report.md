# Dev.D.UE.0.0.10.P21.6.r0 Report

## 1. 结论

P21.6 在 P 阶段边界内完成，结论为 **PASS**。

本轮把 P21.5 写在 `DrawHUD` 内的飞剑警戒文字、布局与投影失败处理，抽成一份纯 presentation policy。屏内飞剑继续使用跟随世界位置的读数；飞剑在屏外、镜头后方或投影结果无效时，警戒信息不再静默消失，而是降级为顶部居中的 `飞剑警戒 · 屏外 · 目标 N`。

```text
Threat readout policy focused:             3 Success / 0 Fail
Shanmen.0_0_10 full:                    1230 Success / 0 Fail
Required 0.0.9B compatibility groups:    123 Success / 0 Fail
Regression coverage:                      PASS (Changed=6 / Rules=2 / Required=16 / Logs=3)
Regression gate self-test:                PASS 433/433
Game + Editor Development:                PASS / native status 0
```

本轮没有启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件，也没有执行真实输入、截图、Smoke、Cook 或 Package。因此证明的是纯策略、HUD 消费接线、边界数学和原生可构建性；不宣称实际字体渲染、遮挡效果或人工可读性验收通过。

## 2. 产品行为

新增 `Fdemo_mapShanmenControlledWeaponThreatReadoutPlan`，输入仅为 viewport、已接受的 contact count、投影成败、投影坐标和 presentation style：

- contact count 小于等于 0 时不生成提示；
- 投影成功且坐标位于 viewport 内时使用 `WorldTracked`；
- 投影失败、非有限坐标或坐标越界时使用 `ViewportFallback`；
- 屏内提示保持 P21.5 的飞剑上方锚点，并限制在安全边距内；
- 屏外提示固定在顶部中央，明确标注“屏外”；
- viewport 放不下完整面板或 style 非法时失败关闭，不绘制裁切/未定义几何。

HUD 仍从 canonical flying-sword Actor 读取 P21.5 保存的 `RoutedContactCount`。Policy 不读取 World、Actor、Canvas、敌人或输入，也不拥有任何可变状态。

## 3. 可配置边界

`Fdemo_mapShanmenControlledWeaponThreatReadoutStyle` 集中保存面板尺寸、文字内边距、世界锚点抬升、四向安全空间、文字缩放和颜色。Policy 在使用前验证所有几何、缩放与颜色分量为有限值，并拒绝零尺寸、负边距或越界内边距。

HUD 当前使用默认 style，未创建第二个配置权威。后续若把这些值接到项目设置或内容资产，只需提供一份已验证 style，不需要重写布局算法。

## 4. 产品自动化

新增 3 项纯策略测试：

```text
Shanmen.0_0_10.Product.ControlledWeaponThreatReadoutPresentation.WorldTracked
Shanmen.0_0_10.Product.ControlledWeaponThreatReadoutPresentation.ViewportFallback
Shanmen.0_0_10.Product.ControlledWeaponThreatReadoutPresentation.Fences
```

覆盖：

- 1920×1080 中央锚点的精确面板/文字坐标、文本、数量与 replay；
- 屏幕边缘仍属于 tracked，但布局被安全夹取；
- projection false、viewport 外和 NaN 坐标得到相同稳定 fallback；
- 自定义 style 精确改变尺寸、位置、缩放与颜色；
- 零 contact、过小 viewport、零面板和非有限 style 失败关闭；
- 失败会清空复用输出，不残留上一帧可绘制计划。

全量测试由 P21.5 的 1227 项精确增加到 1230 项。

## 5. 回归证据

| Evidence | Success | Fail | Bytes | SHA-256 |
|---|---:|---:|---:|---|
| Threat readout policy focused | 3 | 0 | 265,123 | `35E5072394B2767188B8A6097CC5796DA12E42BDAD632A416446DDD4A315E8DE` |
| `Shanmen.0_0_10` full | 1230 | 0 | 1,885,215 | `26B5D3131C37575DC5648D821DCE1A616D4D3792165F172541377736EB629EDB` |
| `demo_map.InputRestore` | 101 | 0 | 394,783 | `8BE804CEE2EA072BD8A77C1DF6FE6E0851CE0659F538D936F559104C38815839` |
| `demo_map.V2RangedCompatibility` | 22 | 0 | 285,352 | `F5D1A3397666695110F3ABCE4312EC3F700101B727F435D75151C15BC86DDF1A` |

四份自动化日志合计 1356/0（focused 与 full 有意重叠）。完整套件从 `2026-09-08 00:24:44.525` 至 `01:37:13.146`，同一 UnrealEditor-Cmd 实例自然清空 1230 项并原生退出 0；全部最终自动化日志的 Fatal、Unhandled 与 Ensure 命中均为 0。

## 6. 改动覆盖门

为新 policy 的 `.h/.cpp/Tests.cpp` 增加独立映射，要求其 focused group 与 `Shanmen.0_0_10`；HUD 规则同步要求新 policy，并保留 P21.5 ThreatCue、投掷提示栈和旧输入/远程兼容证据。

- regression self-test：`PASS 433/433`，42,728 bytes，SHA-256 `42002D3A291546D9AA93BC921A65A6833E5EFD89BE58AA3EA4CEE17997BB6B2F`；
- changed-file gate：`PASS Changed=6 Rules=2 Required=16 Logs=3`，796 bytes，SHA-256 `C838E55B2BE5BA7FA143C9FE6A9E71046329699C8A4C7BE3D63937E18E807152`。

覆盖门首次即通过。输入严格为本轮 full、InputRestore 与 V2RangedCompatibility；没有复用 P21.5 日志，没有通过删映射或增加无关主题测试过门。

## 7. 静态审计与构建

- 6 个实现、测试和流程文件，`+510 / -22`，其中 211 行为测试；
- 新增生产行中 `ApplyDamage`、vitality/Impact 写入、AI authority、inventory transaction、Timer 与 RNG API 命中 0；
- JSON parse 与 `git diff --cached --check` 通过，仅有工作树 LF→CRLF 提示；
- 103 个用户原有 untracked 文件保持未暂存；
- raw logs 未进入 Git，验证结束后无 UnrealEditor-Cmd 残留进程。

| Target | Result | Actions / Time | Log SHA-256 |
|---|---|---|---|
| Editor initial | Succeeded / native 0 | 6 / 21.64s UBA, 24.27s total | `F4CFE6DFC2E48BC7F54A16F9D557C97E63BD51EE62CB4CA859399348615828A1` |
| Game Development final | Succeeded / native 0 | 5 / 28.92s UBA, 32.37s total | `EF865D2056724CEE7C418E293B987E6D4D7531C8E08DC2873FFA1D8DC735B7FB` |
| Editor Development final | Succeeded / native 0 | 0 / 0.11s UBA, 1.09s total | `66BE54A4AE71FCF7D599E93E20C49653A9BD691942F4711C347A0980D6595EE1` |

最终产物：

- `demo_map.exe`：359,396,864 bytes，SHA-256 `8B0D8F59F4A814C9FBD9E2F837739C91ECEF663FCD41EAC25203DC2A7F39999D`；
- `UnrealEditor-demo_map.dll`：18,561,024 bytes，SHA-256 `3AFD06D3969B438EF420BE8E1C7B54DFB36D6E5E65C287D7A512A88F84325B99`。

## 8. P/F 边界与后续

PASS：飞剑警戒布局/文本由独立纯策略决定；屏内、边缘、屏外、投影失败、非法输入与自定义 style 的结果确定；HUD 消费同一 canonical Actor；完整、兼容、覆盖门和双目标构建通过。

未声明：真实屏幕可读性、字体覆盖、多人 split-screen、DPI 缩放、视觉遮挡、方向箭头、声音、伤害、控制或人工游戏验收。

下一轮应回到实际玩法闭环：优先把既有 P6 Launch/Recall 命令通过现有输入权威接到 canonical TrainingFlyingSword，而不是继续增加 HUD 包装层。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p21-6-flying-sword-threat-layout-policy>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p21-6-flying-sword-threat-layout-policy/Docs/Report/Dev.D.UE.0.0.10.P21.6.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p21-6-flying-sword-threat-layout-policy/Docs/Log/Dev.D.UE.0.0.10.P21.6.r0_log.md>
