# Dev.D.UE.0.0.10.P21.5.r0 Report

## 1. 结论

P21.5 在 P 阶段边界内完成，结论为 **PASS**。

本轮把 P21.4 只存在于飞剑本体光效上的近身威慑提示，扩展为跟随 canonical 练习飞剑的主 HUD 读数。只要 P21.3/P6.22 已接受的样本包含近身目标，HUD 就会在飞剑上方显示 `飞剑警戒 · 近身目标 N`；空样本、清除或生命周期停用后隐藏。没有新增 service、Host、ledger，也没有创建伤害、控制、AI 或资源权威。

```text
Threat readout focused:                    1 Success / 0 Fail
Shanmen.0_0_10 full:                    1227 Success / 0 Fail
Required 0.0.9B compatibility groups:    176 Success / 0 Fail
Regression coverage:                      PASS (Changed=6 / Rules=4 / Required=32 / Logs=5)
Regression gate self-test:                PASS 431/431
Game + Editor Development:                PASS / native status 0
```

本轮没有启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件，也没有执行真实输入、截图、Smoke、Cook 或 Package。因此证明的是 HUD 接线、确定性状态、无头真实 World 行为和可构建性；不宣称最终字体、美术观感、分辨率覆盖或人工手感验收通过。

## 2. 玩家可读行为

`Ademo_mapHUD` 每帧读取既有 `ControlledWeaponWorldLifecycle` 的当前飞剑 Actor，不复制飞剑身份或威胁状态。提示仅在以下条件同时成立时绘制：

- canonical Actor 存在；
- 已接受的 threat-presence 状态为 active；
- 已接受的 `RoutedContactCount` 大于 0；
- Actor 的世界位置能够投影到屏幕。

读数锚定在飞剑上方并限制在 HUD 安全边距内，使用半透明青绿色面板和紧凑文字。飞剑移动时读数随其世界投影移动；飞剑不可投影或威胁消失时不留下静态假提示。

## 3. 状态契约

`Ademo_mapShanmenControlledWeaponActor` 现在保存最近一次已接受样本的 `RoutedContactCount`，并向 HUD 只读公开。状态约束同步加强：

- active 与 `ContactCount > 0` 必须一致；
- exact replay 还必须具有相同 contact count；
- 同 sequence、同 intent 但 count 冲突时失败关闭，且不覆盖已接受读数；
- 空样本把 count 置 0 并关闭光效与 HUD；
- clear 和 lifecycle teardown 都把 count 归零；
- 未提交任何样本时，intent、sequence、count 与 active 必须共同处于初始状态。

HUD 读取的是 P21.3 已经由真实 overlap、entity registry、P6.22 Router/Host 接受的数量，不自行扫描敌人，也不重新解释目标合法性。

## 4. 产品自动化

既有真实 World 测试：

```text
Shanmen.0_0_10.Product.ControlledWeaponThreatCue.AcceptedPresenceAndEmptyRelease
```

本轮增加的关键断言：

- 新生成飞剑的 readout count 为 0；
- 真实近身接触被接受后 count 为 1；
- exact replay 保持 count 与提示状态；
- 同 sequence 的 count 冲突被拒绝，已接受的 1 不被覆盖；
- foreign item 与 stale positive 样本不能改写读数；
- 敌人移出后的已接受空样本把 count 归零；
- 整个过程敌人 vitality 与 committed Impact 数不变。

`demo_mapHUD.cpp` 的回归映射新增该产品组要求；覆盖门因此不能再只凭旧 HUD 布局或投掷武器提示测试通过。

## 5. 回归证据

| Evidence | Success | Fail | Bytes | SHA-256 |
|---|---:|---:|---:|---|
| Threat readout focused | 1 | 0 | 262,829 | `18820122B85CC99D6E5FDD67756A1E0F42219399EA2C98E968DAEE770F44EDDD` |
| `Shanmen.0_0_10` full | 1227 | 0 | 1,881,735 | `BED8DE9A48E7F0EAFCF2AC8ABF4329C0A675245624A942E73325B9854702C3C7` |
| `demo_map.InputRestore` | 101 | 0 | 395,382 | `54211F9AAF31FD9A5769C45C10993D8BC6A1473E864A88209C5064B053C8449B` |
| `demo_map.V2RangedCompatibility` | 22 | 0 | 284,862 | `8005580ABAB192B8CEBA9C83A90D7842779B1372370DBF184506ED4D123C67A4` |
| `demo_map.ItemUseAndArmor` | 46 | 0 | 309,133 | `06FB4F25B72F0C58C9337855C130C790C30BD853F1C7A71C4B8F9E8056933F56` |
| `demo_map.P4.Hotbar` | 7 | 0 | 267,523 | `4BBBA8C10A3092B4E8BC3FCF9B2D81294A8D9EB43F201FAF07F991CCA7BD8879` |

六份最终自动化日志合计 1404/0（focused 与 full 有意重叠）。完整套件从 `2026-09-07 22:20:06.769` 至 `23:32:02.493`，单一 UnrealEditor-Cmd 实例自然清空 1227 项并原生退出 0；所有最终自动化日志的 Fatal、Unhandled 与 Ensure 命中均为 0。

## 6. 改动覆盖门与首败证据

- regression self-test：`PASS 431/431`，42,559 bytes，SHA-256 `A739CBAAB04E06DF61528E2DBAF087BB34DD62150668F3B166FF4480FC4A3605`；
- changed-file gate：`PASS Changed=6 Rules=4 Required=32 Logs=5`，4,343 bytes，SHA-256 `4022257DFB194F6D268F4ED24D25403125870C373134BE25DE99B4E05307B9AF`。

首轮覆盖门只提供 full、InputRestore 与 V2RangedCompatibility 证据，因此按改动文件映射正确失败，精确指出缺少 `demo_map.ItemUseAndArmor` 与 `demo_map.P4.Hotbar`。首败日志保留为 368 bytes / SHA-256 `B3587A13E1A201EB7EA5A99A1674E30A03F396229533C3F4C4AC19711B98EE00`。补跑这两组后最终门通过；没有通过删规则或伪造证据绕开。

## 7. 静态审计与构建

- 6 个实现、测试和流程文件，`+91 / -1`；
- 新增生产逻辑没有 damage/vitality/Impact、AI authority、inventory transaction、Timer 或 RNG；
- `ShanmenRegressionMap.json` 解析通过；
- `git diff --check` 退出码 0，仅有工作树 LF→CRLF 提示；
- 103 个无关 untracked 条目保持未暂存；
- 验证结束后本项目无 UnrealEditor-Cmd 残留进程。

| Target | Result | Actions / Time | Log SHA-256 |
|---|---|---|---|
| Editor initial | Succeeded / native 0 | 10 / 24.31s UBA, 28.94s total | `3C403F7A7E0EB2553DF2A4783BD65F2814D1AD7FEA51A3F7D5932EF698DD86AC` |
| Game Development final | Succeeded / native 0 | 9 / 34.80s UBA, 39.80s total | `0FCDDEBB3306F52FEEF8A4D6369F5D2088D836E1F39A12BD0E0BAD11C2D17557` |
| Editor Development final | Succeeded / native 0 | 0 / 0.14s UBA, 1.16s total | `EB6404819C325F96E4160F223297312C6D7639E0822C71B9CF593CFBEA73D98D` |

最终产物：

- `demo_map.exe`：359,383,552 bytes，SHA-256 `7B3C6A714B14D0F2C3F3348EB5CD72A290028E0407F19AD0EAA2346AFF9A90E7`；
- `UnrealEditor-demo_map.dll`：18,546,176 bytes，SHA-256 `E194D39648E2A9EED2DBCB07B803019D3EA81B2CE8AED9963FC4700489427813`。

## 8. P/F 边界与后续

PASS：主 HUD 从 canonical 飞剑读取已接受的近身目标数；positive、empty、replay、conflict、foreign、stale、clear 与 teardown 状态确定；提示不会修改敌人、资源、Run 或战斗权威；完整、兼容、覆盖门和双目标构建通过。

未声明：实际屏幕上的最终可读性、遮挡处理、多分辨率布局、离屏提示、颜色无障碍、音效或人工游戏验收。上述体验项需要后续可视验证授权，不能由无头测试替代。

下一步建议 P21.6 为该读数增加可配置 presentation policy 与离屏降级规则，使 HUD 行为可在无头测试中直接验证，并避免把样式参数继续固化在 `DrawHUD` 中。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p21-5-flying-sword-threat-readout>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p21-5-flying-sword-threat-readout/Docs/Report/Dev.D.UE.0.0.10.P21.5.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p21-5-flying-sword-threat-readout/Docs/Log/Dev.D.UE.0.0.10.P21.5.r0_log.md>
