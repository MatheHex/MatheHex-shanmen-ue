# Dev.D.UE.0.0.10.P21.6.r0 Development Log

## 1. 基线与目标

- base：`c92ba63e8f57cf30e3fbe86ef4cbfffa197d8cf2`（P21.5 flying-sword threat readout）；
- branch：`agent/0.0.10-p21-6-flying-sword-threat-layout-policy`；
- 目标：把飞剑威胁读数的布局、文本与离屏行为从 `DrawHUD` 抽成可测试、可配置的纯策略；
- 边界：不新增 damage、control、AI、inventory、Timer、Run、P6.22 或 sampling authority，不启动可视产品，不修改 Windows 或 Engine。

## 2. 设计选择

P21.5 已让 HUD 显示已接受的近身目标数，但投影失败时直接返回，飞剑在镜头后方或屏外时警戒会消失；同时面板尺寸、边距、文字与颜色都硬编码在 `DrawHUD`，只能靠编译验证。

P21.6 只抽出一个纯 plan：HUD 仍负责一次世界投影和实际 Canvas 绘制，policy 仅对值类型输入做确定性决策。没有引入 service、Host、ledger、UObject 生命周期或第二份 threat state。

## 3. 实现

### 3.1 presentation style 与 plan

新增：

- `Edemo_mapShanmenControlledWeaponThreatReadoutPlacement`：`WorldTracked` / `ViewportFallback`；
- `Fdemo_mapShanmenControlledWeaponThreatReadoutStyle`：集中默认尺寸、内边距、安全空间、缩放和颜色，并验证有限性与基本几何；
- `Fdemo_mapShanmenControlledWeaponThreatReadoutPlan::TryPlan`：生成完整面板位置、文字和 style 读数，失败时先清空输出。

屏内位置保留 P21.5 的世界跟随与边缘夹取。投影失败、NaN 或 viewport 外坐标统一降级到顶部中央，并用不同文本明确标记屏外；零 contact 或放不下完整面板时不生成计划。

### 3.2 HUD 接线

`DrawControlledWeaponThreatReadout` 仍读取 canonical flying-sword Actor。它把 `ProjectWorldLocationToScreen` 的布尔结果与坐标交给 policy，再按 plan 绘制面板和文字。

原先散落在 HUD 中的尺寸、夹取、字符串、颜色和缩放已移出。投影失败不再提前 return；Actor 缺失、threat inactive 或 accepted count 为 0 的既有快速退出保持不变。

### 3.3 测试与映射

新增 3 个测试文件内用例，覆盖 tracked、fallback、自定义 style 和 fail-closed fences。回归映射新增 policy rule，并把新 focused group加入 HUD 的既有规则；self-test 新增一项正向和一项“focused 不能替代 full”反向 fixture，总数从 431 增至 433。

## 4. 首轮结果

本轮没有产品、编译或覆盖首败：

- initial Editor build：6 actions，native 0；
- focused policy：3/0；
- full：1230/0；
- 两个改动映射要求的旧版兼容组：123/0；
- changed-file gate 首次执行即通过。

全量套件中的既有持久化/恢复测试仍包含真实等待窗口，因此耗时约 72 分钟；同一实例持续推进并自然清空，未使用外层超时码代替原生成功。

## 5. 自动化结果

| Log | Group | Success/Fail | Bytes | SHA-256 |
|---|---|---:|---:|---|
| `P21.6_focused_threat_readout_presentation.log` | `Shanmen.0_0_10.Product.ControlledWeaponThreatReadoutPresentation` | 3/0 | 265,123 | `35E5072394B2767188B8A6097CC5796DA12E42BDAD632A416446DDD4A315E8DE` |
| `P21.6_full_0_0_10.log` | `Shanmen.0_0_10` | 1230/0 | 1,885,215 | `26B5D3131C37575DC5648D821DCE1A616D4D3792165F172541377736EB629EDB` |
| `P21.6_legacy_input_restore.log` | `demo_map.InputRestore` | 101/0 | 394,783 | `8BE804CEE2EA072BD8A77C1DF6FE6E0851CE0659F538D936F559104C38815839` |
| `P21.6_legacy_ranged.log` | `demo_map.V2RangedCompatibility` | 22/0 | 285,352 | `F5D1A3397666695110F3ABCE4312EC3F700101B727F435D75151C15BC86DDF1A` |

四份自动化最终为 1356/0（focused 与 full 重叠）。full 从 `00:24:44.525` 至 `01:37:13.146`，自然收到 `Automation Test Queue Empty 1230 tests performed` 并原生退出 0。四份日志中 Fatal、Unhandled Exception 与 Ensure condition failed 均为 0。

## 6. Regression coverage

- self-test：`PASS 433/433`；42,728 bytes；SHA-256 `42002D3A291546D9AA93BC921A65A6833E5EFD89BE58AA3EA4CEE17997BB6B2F`；
- gate：`PASS Changed=6 Rules=2 Required=16 Logs=3`；796 bytes；SHA-256 `C838E55B2BE5BA7FA143C9FE6A9E71046329699C8A4C7BE3D63937E18E807152`。

Gate 输入为本轮 full、InputRestore 与 V2RangedCompatibility。Focused 作为新行为的首层证据；full 已包含同 3 项测试，因此 gate 不重复要求 focused 日志。

## 7. 静态与构建

- implementation/test/process diff：`+510 / -22`，其中 production policy 251 行、测试 211 行；
- 新增生产行的 damage/vitality/Impact、AI authority、inventory transaction、Timer、RNG API 扫描命中 0；
- JSON parse、cached diff check 与残留进程检查通过。

| Log | Target | Result | Actions / Time | Bytes | SHA-256 |
|---|---|---|---|---:|---|
| `P21.6_editor_build_initial.log` | Editor | Succeeded / native 0 | 6 / 21.64s UBA, 24.27s total | 2,629 | `F4CFE6DFC2E48BC7F54A16F9D557C97E63BD51EE62CB4CA859399348615828A1` |
| `P21.6_game_build_final.log` | Game | Succeeded / native 0 | 5 / 28.92s UBA, 32.37s total | 2,408 | `EF865D2056724CEE7C418E293B987E6D4D7531C8E08DC2873FFA1D8DC735B7FB` |
| `P21.6_editor_build_final.log` | Editor | Succeeded / native 0 | 0 / 0.11s UBA, 1.09s total | 1,026 | `66BE54A4AE71FCF7D599E93E20C49653A9BD691942F4711C347A0980D6595EE1` |

Artifacts：

- `Binaries/Win64/demo_map.exe`：359,396,864 bytes / SHA-256 `8B0D8F59F4A814C9FBD9E2F837739C91ECEF663FCD41EAC25203DC2A7F39999D`；
- `Binaries/Win64/UnrealEditor-demo_map.dll`：18,561,024 bytes / SHA-256 `3AFD06D3969B438EF420BE8E1C7B54DFB36D6E5E65C287D7A512A88F84325B99`。

## 8. 提交边界

计划提交 6 个实现、测试和流程文件、本 Report 与本 Development Log，共 8 个文件。103 个用户原有 untracked 文件保持未暂存；`Saved/Codex/P21.6` raw logs 不入 Git。

未修改 Content、地图、项目资源、Engine、Windows、save schema、inventory authority、Run authority、P6 movement/orbit、P6.22 Router/Host、Impact/effect resolver 或输入注册表。未运行 Editor UI、PIE、Standalone、产品 executable、真实输入、截图、Smoke、Cook 或 Package。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p21-6-flying-sword-threat-layout-policy>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p21-6-flying-sword-threat-layout-policy/Docs/Report/Dev.D.UE.0.0.10.P21.6.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p21-6-flying-sword-threat-layout-policy/Docs/Log/Dev.D.UE.0.0.10.P21.6.r0_log.md>
