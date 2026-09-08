# Dev.D.UE.0.0.10.P21.12.r0 Development Log

## 1. 基线与目标

- base：`705e1f3966cdbf35cbd5e5d4b0929be5c3342aef`（P21.11 player redirect）；
- branch：`agent/0.0.10-p21-12-flying-sword-context-hints`；
- 目标：在既有飞剑状态/威胁 readout 中显示当前阶段可执行的玩家动作及实时绑定键；
- 边界：只扩展 HUD 与纯表现 planner，不新增 gameplay owner、状态机、Actor、移动、伤害、计时、库存或输入配置。

## 2. 路径审计与决策

P21.11 已提供独立 X Launch/Recall 与 C Redirect，但玩家必须记住阶段对应语义。P21.6 的 HUD 已有飞剑 phase、近身 threat contact count、世界跟随和视口回退，是最短且不重复的反馈通道。

因此本轮没有创建新 HUD panel，也没有把提示写进 Actor 或 PlayerController。HUD 只在现有绘制点读取统一输入设置中的两个当前键名，并把它们作为不可变输入交给既有 readout planner；planner 负责按 phase 生成最终单行文本。

## 3. 实现

### 3.1 阶段动作提示

新增纯函数式 input hint 派生：Orbiting 显示发射，Directed 显示改向和召回，Returning 不显示动作，Redeployed 显示再次出击。已有 `屏外` 与 `近身目标 N` 片段保持原顺序，动作提示追加在同一行末尾。

### 3.2 实时重绑定

`Ademo_mapHUD` 每次规划时从 `Fdemo_mapInputBindingSettings::Get()` 读取 `ControlledWeaponLaunchRecall` 和 `ControlledWeaponRedirect`。没有缓存默认 X/C；测试使用 V/Z 验证文本随输入值改变且 plan identity 同步改变。

### 3.3 计划完整性

plan 新增两个只读键名字段。捕获时拒绝空值、首尾空白和 CR/LF/Tab；失败先重置 `OutPlan`。`IsValid()` 会重新构造文本并比对，`Matches()` 纳入两个键名，从而避免失效标签或旧绑定文本被当作有效结果。

默认 panel 宽度由 230 调整到 420，text scale 由 0.82 调整到 0.72，以容纳 Directed 的双动作中文提示；仍沿用既有位置夹取、世界投影与 viewport fallback 数学。

## 4. 测试开发

既有 world-tracked、viewport-fallback 与 fence 测试全部改为传入明确键名，并同步验证新 panel 几何与文本。新增 `ContextualInputHints` 用例覆盖：

- Directed 默认 X/C 双动作；
- V/Z 重绑定后的精确动作顺序；
- 键位变化导致 plan 不匹配；
- Returning 不出现动作提示；
- 空 launch label 与多行 redirect label 失败关闭；
- 失败后复用输出不保留旧 plan。

focused 组由 7 项增至 8 项；全量 0.0.10 由 1245 项增至 1246 项。

## 5. 自动化与首次门禁失败

初始 Editor 编译 6/6、28.26s、native 0。focused 执行 8/0；full 执行 1246/0，并在日志时钟 `12.55.52:652` 至 `14.10.51:098` 自然清空。

本轮没有产品测试失败。首个可验证失败来自 changed-file regression gate：只传入 full 日志时，门禁按 HUD 路径映射准确拒绝，并列出缺少 `demo_map.InputRestore` 与 `demo_map.V2RangedCompatibility`。该失败证据被保留；随后补跑 101/0 和 22/0，最终 gate 通过。

| Log | Group | Success/Fail | Bytes | SHA-256 |
|---|---|---:|---:|---|
| `P21.12_focused_controlled_weapon_threat_initial.log` | `Shanmen.0_0_10.Product.ControlledWeaponThreat` | 8/0 | 270,736 | `D00F34CB9F1775706EDB232282941CA6BE2BF4F44FA9A1A571F3DC84D857F689` |
| `P21.12_full_0_0_10.log` | `Shanmen.0_0_10` | 1246/0 | 1,906,426 | `F113E29827F3E20F40E32E7115D1B1503BFC61EE115E7E869686423320C17716` |
| `P21.12_legacy_input_restore.log` | `demo_map.InputRestore` | 101/0 | 394,907 | `486E1C501537CBAD4DCCFFD580EA7909F8F3EEE5819223539A9A68246E691DAC` |
| `P21.12_legacy_v2_ranged.log` | `demo_map.V2RangedCompatibility` | 22/0 | 285,294 | `178DBB54F2D9B64791764B99F4569241392EEE3EE93BCBCDDE1A5808F5E59E4B` |

每份自动化日志均有一个实际 RunTests 命令和一个成功终止标记；Fail/Fatal/Unhandled/Ensure 均为 0。

## 6. 覆盖门

- initial fail：缺少 `demo_map.InputRestore, demo_map.V2RangedCompatibility`，native 1；
- final：`PASS Changed=4 Rules=2 Required=16 Logs=3`；
- validator self-test：`PASS 437/437`。

| Evidence | Bytes | SHA-256 |
|---|---:|---|
| `P21.12_regression_gate_initial_fail.log` | 861 | `CA6D29B9BAF8C3036CDC7DBCEEE96A4C96DDFF487D0FF193319866F7E3E95215` |
| `P21.12_regression_gate_final.log` | 2,433 | `BA7ABE628377C66E82C12AAB34F424BCF30569A2E04A63F5F5577BAF93ACD13A` |
| `P21.12_regression_gate_selftest.log` | 218 | `78C2463FEE1B3C894E6CD784F8377FFD35D85091B2A37B5FA50719518F8B426C` |

## 7. 静态与构建

- 实现/测试 diff：4 files，`+215 / -21`；
- 87 条新增生产行内 Timer/SetTimer/RNG/ApplyDamage/SpawnActor/Destroy 命中 0；
- `git diff --check` native 0，仅有 LF→CRLF 工作树提示；
- 验证结束后 UnrealEditor、UnrealEditor-Cmd 与 demo_map 进程均为 0。

| Log | Target | Result | Actions / Time | Bytes | SHA-256 |
|---|---|---|---|---:|---|
| `P21.12_editor_build_initial.log` | Editor initial | Succeeded / native 0 | 6 / 28.26s | 2,554 | `426EB2451624BFDD0B57E96D50F7255B0DCB8D15AFC17FB99CE697CC1FD85F1B` |
| `P21.12_game_build_final.log` | Game final | Succeeded / native 0 | 5 / 29.02s | 2,350 | `8398ECB5F148901349C82CB82BE1BC532CF28A07F4E61FBA04F4FD8C921A7A3A` |
| `P21.12_editor_build_final.log` | Editor final | Succeeded / native 0 | 0 / 1.07s | 1,021 | `498339F9ABEB88543CB6911810F21219014E3E48FD4C996AE8007DD9FD7E0BF1` |

Artifacts：

- `Binaries/Win64/demo_map.exe`：359,515,648 bytes / SHA-256 `A51BEE178B494799C628810C3EBD99B2973AAC9E4CB71582F328C0465E9E96EB`；
- `Binaries/Win64/UnrealEditor-demo_map.dll`：18,702,848 bytes / SHA-256 `E783E40D6C1C1684212172829D863B0CEC73E4EBADE88B5095EAFF8033FC2B99`。

## 8. 提交边界

提交 4 个实现/测试文件、本 Report 与本 Development Log，共 6 个文件。103 个用户原有 untracked 文件保持未暂存；`Saved/Codex/P21.12` raw evidence 不进入 Git。

未修改 Content、地图、Engine、Windows、输入配置版本、玩家存档 schema、物品权威、Impact resolver、飞剑 Actor、flight controller、移动或伤害数学。未运行 Editor UI、PIE、Standalone、产品 executable、真实输入、截图、Smoke、Cook 或 Package。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p21-12-flying-sword-context-hints>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p21-12-flying-sword-context-hints/Docs/Report/Dev.D.UE.0.0.10.P21.12.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p21-12-flying-sword-context-hints/Docs/Log/Dev.D.UE.0.0.10.P21.12.r0_log.md>
