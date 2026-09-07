# Dev.D.UE.0.0.10.P21.5.r0 Development Log

## 1. 基线与目标

- base：`8c5df35a4f5b9ed1d0914fe3f31b9d46b8d72734`（P21.4 flying sword threat cue）；
- branch：`agent/0.0.10-p21-5-flying-sword-threat-readout`；
- 目标：把 P21.4 的飞剑本体光效扩展成玩家可读的主 HUD 近身目标计数；
- 边界：不新增 damage、control、AI、inventory、Timer、Run、P6.22 或 world-sampling authority，不启动可视产品，不修改 Windows 或 Engine。

## 2. 设计选择

P21.4 已经把 P21.3/P6.22 接受的 `ThreatPresence` 样本投影到 canonical flying-sword Actor，但点光源只表达有/无，玩家无法读取近身目标数量。P21.5 沿用同一 Actor 作为 presentation source：Actor 保存已接受的 routed count，HUD 只读它并跟随其世界投影绘制。

没有增加第二个 sampler、状态缓存 service 或 HUD ledger。这样数量仍由真实 overlap → registry → Router/Host 的既有链路决定，HUD 无权自行发现敌人或改变游戏状态。

## 3. 实现

### 3.1 `demo_mapShanmenControlledWeaponActor`

新增只读 `ThreatPresenceCueContactCount`：

1. 接受新样本时保存 `RoutedContactCount`；
2. active 必须严格等价于 count 大于 0；
3. exact replay 还比较 count，防止同 sequence/count 冲突被误当幂等成功；
4. 冲突拒绝不改变既有样本水位或读数；
5. empty sample、explicit clear 与 collision teardown 都归零 count；
6. `IsThreatPresenceCueStateValid` 同时检查 count、active、visible component 与样本身份的一致性。

### 3.2 `demo_mapHUD`

新增轻量 `DrawControlledWeaponThreatReadout`，从既有 `ActiveMode->GetControlledWeaponWorldLifecycle().GetWeaponActor()` 取得 canonical Actor。active 且 count 为正时，将飞剑上方 72 uu 的世界位置投影到屏幕，限制在 HUD 边距内，并绘制：

```text
飞剑警戒 · 近身目标 N
```

投影失败、Actor 不存在、状态 inactive 或 count 为 0 时不绘制。没有修改 GameMode，也没有重复路由 P21.3 样本。

### 3.3 测试与回归映射

扩展 `ControlledWeaponThreatCue.AcceptedPresenceAndEmptyRelease` 的真实 World fixture，覆盖初始 0、accepted 1、exact replay、同 sequence count conflict、foreign item、empty 0、stale positive 和零战斗副作用。

为 `demo_mapHUD.cpp` 的既有映射增加 `Shanmen.0_0_10.Product.ControlledWeaponThreatCue` 必跑组，并同步更新 regression self-test 的正向 fixture。self-test 总数保持 431。

## 4. 首轮结果与覆盖修复

产品首层验证直接通过：初始 Editor build 为 10 actions / native 0，focused threat test 为 1/0。

首轮 changed-file gate 输入 full、InputRestore 和 V2RangedCompatibility 后失败，精确提示：

```text
REGRESSION_COVERAGE: missing required groups: demo_map.ItemUseAndArmor, demo_map.P4.Hotbar
```

原因是 HUD 与测试文件既有映射的并集还要求旧物品/护甲和快捷栏兼容组。随后补跑两组并重新执行同一覆盖门，最终 `PASS Changed=6 Rules=4 Required=32 Logs=5`。产品代码与映射规则均未为通过门而削减。

首败证据：`P21.5_regression_gate_first_failure.log`，368 bytes，SHA-256 `B3587A13E1A201EB7EA5A99A1674E30A03F396229533C3F4C4AC19711B98EE00`。

## 5. 自动化结果

| Log | Group | Success/Fail | Bytes | SHA-256 |
|---|---|---:|---:|---|
| `P21.5_focused_threat_readout.log` | `Shanmen.0_0_10.Product.ControlledWeaponThreatCue` | 1/0 | 262,829 | `18820122B85CC99D6E5FDD67756A1E0F42219399EA2C98E968DAEE770F44EDDD` |
| `P21.5_full_0_0_10.log` | `Shanmen.0_0_10` | 1227/0 | 1,881,735 | `BED8DE9A48E7F0EAFCF2AC8ABF4329C0A675245624A942E73325B9854702C3C7` |
| `P21.5_legacy_input_restore.log` | `demo_map.InputRestore` | 101/0 | 395,382 | `54211F9AAF31FD9A5769C45C10993D8BC6A1473E864A88209C5064B053C8449B` |
| `P21.5_legacy_ranged.log` | `demo_map.V2RangedCompatibility` | 22/0 | 284,862 | `8005580ABAB192B8CEBA9C83A90D7842779B1372370DBF184506ED4D123C67A4` |
| `P21.5_legacy_items.log` | `demo_map.ItemUseAndArmor` | 46/0 | 309,133 | `06FB4F25B72F0C58C9337855C130C790C30BD853F1C7A71C4B8F9E8056933F56` |
| `P21.5_legacy_hotbar.log` | `demo_map.P4.Hotbar` | 7/0 | 267,523 | `4BBBA8C10A3092B4E8BC3FCF9B2D81294A8D9EB43F201FAF07F991CCA7BD8879` |

最终自动化合计 1404/0（focused 与 full 重叠）。full 从 `22:20:06.769` 至 `23:32:02.493` 自然收到 `Automation Test Queue Empty 1227 tests performed` 并原生退出 0。六份最终日志的 Fatal、Unhandled Exception 与 Ensure condition failed 均为 0。

## 6. Regression coverage

- self-test：`PASS 431/431`；42,559 bytes；SHA-256 `A739CBAAB04E06DF61528E2DBAF087BB34DD62150668F3B166FF4480FC4A3605`；
- first failure：缺 `ItemUseAndArmor` 与 `P4.Hotbar`；368 bytes；SHA-256 `B3587A13E1A201EB7EA5A99A1674E30A03F396229533C3F4C4AC19711B98EE00`；
- final gate：`PASS Changed=6 Rules=4 Required=32 Logs=5`；4,343 bytes；SHA-256 `4022257DFB194F6D268F4ED24D25403125870C373134BE25DE99B4E05307B9AF`。

Gate 的五份最终输入为 full + 四个旧兼容组。Focused 用于新行为首层证明，full 已包含同一测试，故不重复作为 gate 输入。

## 7. 静态与构建

- tracked implementation/test/process diff：`+91 / -1`；
- 新增生产逻辑没有 damage/vitality/Impact、AI authority、inventory transaction、Timer 或 RNG；
- JSON parse PASS；
- `git diff --check` exit 0；
- residual UnrealEditor-Cmd process 0。

| Log | Target | Result | Actions / Time | Bytes | SHA-256 |
|---|---|---|---|---:|---|
| `P21.5_editor_build_initial.log` | Editor | Succeeded / native 0 | 10 / 24.31s UBA, 28.94s total | 2,877 | `3C403F7A7E0EB2553DF2A4783BD65F2814D1AD7FEA51A3F7D5932EF698DD86AC` |
| `P21.5_game_build_final.log` | Game | Succeeded / native 0 | 9 / 34.80s UBA, 39.80s total | 2,644 | `0FCDDEBB3306F52FEEF8A4D6369F5D2088D836E1F39A12BD0E0BAD11C2D17557` |
| `P21.5_editor_build_final.log` | Editor | Succeeded / native 0 | 0 / 0.14s UBA, 1.16s total | 1,026 | `EB6404819C325F96E4160F223297312C6D7639E0822C71B9CF593CFBEA73D98D` |

Artifacts：

- `Binaries/Win64/demo_map.exe`：359,383,552 bytes / SHA-256 `7B3C6A714B14D0F2C3F3348EB5CD72A290028E0407F19AD0EAA2346AFF9A90E7`；
- `Binaries/Win64/UnrealEditor-demo_map.dll`：18,546,176 bytes / SHA-256 `E194D39648E2A9EED2DBCB07B803019D3EA81B2CE8AED9963FC4700489427813`。

## 8. 提交边界

计划提交 6 个实现、测试和流程文件、本 Report 与本 Development Log，共 8 个文件。103 个无关 untracked 条目保持未暂存；`Saved/Codex/P21.5` raw logs 不入 Git。

未修改 Content、地图、项目资源、Engine、Windows、save schema、inventory authority、Run authority、P6 orbit 数学、P6.22 Router/Host 或 Impact/effect resolver。未运行 Editor UI、PIE、Standalone、产品 executable、真实输入、截图、Smoke、Cook 或 Package。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p21-5-flying-sword-threat-readout>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p21-5-flying-sword-threat-readout/Docs/Report/Dev.D.UE.0.0.10.P21.5.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p21-5-flying-sword-threat-readout/Docs/Log/Dev.D.UE.0.0.10.P21.5.r0_log.md>
