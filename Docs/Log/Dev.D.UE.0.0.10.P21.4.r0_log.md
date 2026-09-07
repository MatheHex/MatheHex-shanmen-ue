# Dev.D.UE.0.0.10.P21.4.r0 Development Log

## 1. 基线与目标

- base：`e8e072b0f85fba5efe64fd23d3e3dcf2a9334dcb`（P21.3 World threat sampling）；
- branch：`agent/0.0.10-p21-4-training-flying-sword-threat-cue`；
- 目标：把 P21.3 已接受的 `ThreatPresence` 样本变成同一 physical flying-sword Actor 上的明确可见提示；
- 边界：不新增 damage、control、AI、inventory、Timer、Run 或 P6.22 authority，不启动可视产品，不修改 Windows 或 Engine。

## 2. 设计选择

P21.3 已完成真实 World overlap → stable entity registry → P6.22 Router → P6 Host，但其 Report 明确不声明视觉产品行为。策划基线要求飞剑环绕具有“近身威慑”，而 P6.22 又明确不拥有持续时间、目标状态、伤害或控制。

因此 P21.4 只实现 presentation：使用 P21.2 已存在的 canonical Actor，不增加中间 service/Host/ledger；P21.3 成功样本直接驱动该 Actor 上的可见组件。这样既提供玩家可观察结果，也不把“威慑”擅自解释为敌人减速、恐惧或伤害。

## 3. 实现

### 3.1 `demo_mapShanmenControlledWeaponActor`

新增附着到 canonical collision root 的 `UPointLightComponent`，威胁色为 `(0.05, 1.00, 0.72)`，强度 2200、半径 180，默认隐藏。Actor 同时尝试更新动态材质的 `Color` / `BaseColor` 参数；无参数材质不影响点光源主提示。

新增 `TryPresentThreatPresenceCue`：

1. 只接受 `Sample.IsSampled()`；
2. Run/item 必须与 Actor exact binding 一致；
3. sample sequence 单调递增，旧样本拒绝；
4. exact replay 仅在 intent 与目标状态完全相同时幂等成功；
5. `RoutedContactCount > 0` 开启提示，空样本关闭；
6. 每次提交后检查组件附着、可见状态与样本水位的一致性。

`TryClearThreatPresenceCue` 只清除 presentation，不推进水位。`DeactivateProductCollision` 在 teardown 前无条件清除提示。

### 3.2 `demo_mapGameMode`

P21.3 sampler 的每份成功样本现在都交给 lifecycle 当前 Actor：contact sample 点亮，empty sample 熄灭。presentation 拒绝记录 `WorldThreatCueRejected`；成功 contact 日志增加 `CueActive=1`。sampler 返回 `NotOrbiting` 时请求清除残留提示。

### 3.3 测试与回归映射

在既有完整 World fixture 中新增 `ControlledWeaponThreatCue.AcceptedPresenceAndEmptyRelease`，使用真实 physics/trace overlap 和注册 M01 实体验证提示开关、幂等、foreign/stale fences 以及零 vitality/Impact 变更。

`ShanmenRegressionMap.json` 为 Actor 增加 `ControlledWeaponThreatCue` 规则；自测增加 full-suite 正向用例和 cue-only 证据必须失败用例，自测总数由 429 增至 431。

## 4. 首轮结果与修复

产品实现和 focused 测试没有出现源码或行为首败：初始 Editor build 9 actions 成功，focused 为 1/0。

验证编排曾有一次非产品问题：把 regression self-test 与 changed-file gate 放在同一个 PowerShell 调用中，并检查 `$LASTEXITCODE`。self-test 内部会故意运行大量“预期失败”样例，虽然最终为 `PASS 431/431`，最后一个内部子进程码仍可能保留为非零，导致组合脚本在生成 gate log 前提前退出。修复为让 self-test 与 gate 独立执行；没有修改产品代码，也没有把该编排问题计为测试失败。独立 gate 随后直接 PASS。

## 5. 自动化结果

| Log | Group | Success/Fail | Bytes | SHA-256 |
|---|---|---:|---:|---|
| `P21.4_focused_threat_cue.log` | `Shanmen.0_0_10.Product.ControlledWeaponThreatCue` | 1/0 | 261,743 | `1334B7FC6A02CCA8A2C5F20FF70F1B69273C849EA40C2A773237E3CB160C603E` |
| `P21.4_full_0_0_10.log` | `Shanmen.0_0_10` | 1227/0 | 1,883,150 | `B3B21533CB1E092801A6F7350CC33374908287267D861BD04010F64966069E41` |
| `P21.4_legacy_attributes.log` | `demo_map.V3.Attributes` | 4/0 | 264,831 | `A6718C21BBA027ACD971C30F487C840D4F97A087EA3AD2CF70E31044C37484BC` |
| `P21.4_legacy_enemy.log` | `demo_map.EnemySkillFramework` | 44/0 | 305,195 | `884E1B5ED159B6DC22EA2BA8D2EAD289565E2B191C4B914D86B65275C5779519` |
| `P21.4_legacy_ranged.log` | `demo_map.V2RangedCompatibility` | 22/0 | 284,867 | `A07B1A7900293F4B1D626A2062234F007FEF1B9F4E45C9B730551389F9B9D4E7` |
| `P21.4_legacy_items.log` | `demo_map.ItemUseAndArmor` | 46/0 | 309,439 | `2BF8DEE30EDB18470CC4F4625BC22A468085CBD89AA1E2B4FC00E62CF762637B` |
| `P21.4_legacy_hotbar.log` | `demo_map.P4.Hotbar` | 7/0 | 266,855 | `D5853D60A477EADD488242B65B1B0692D718DFA6621A0C4536DB90C292180A86` |

最终自动化合计 1351/0（focused 与 full 重叠）。full 从 `20:32:51.175` 至 `21:52:27.406`，自然收到 `Automation Test Queue Empty 1227 tests performed` 并原生退出 0。所有七份最终自动化日志中 Fatal、Unhandled Exception 与 Ensure condition failed 均为 0。

## 6. Regression coverage

- self-test：`PASS 431/431`；42,559 bytes；SHA-256 `A739CBAAB04E06DF61528E2DBAF087BB34DD62150668F3B166FF4480FC4A3605`；
- gate：`PASS Changed=6 Rules=4 Required=71 Logs=6`；8,513 bytes；SHA-256 `B4320FF634879F06224B63A31BB9402422550C71F91C3C5AE83E2B54AC40BF42`。

Gate 的六份日志为 full + 五个旧兼容组；focused 用于产品首层证明，但 full 已包含同一测试，因此不重复作为覆盖门输入。

## 7. 静态与构建

- tracked implementation/test/process diff：`+389 / -12`；
- 新增生产行的 damage/vitality、AI、inventory transaction、Timer、RNG 扫描命中 0；
- JSON parse PASS；
- `git diff --check` exit 0；
- residual UnrealEditor-Cmd process 0。

| Log | Target | Result | Actions / Time | Bytes | SHA-256 |
|---|---|---|---|---:|---|
| `P21.4_editor_build_initial.log` | Editor | Succeeded / native 0 | 9 / 42.59s | 2,903 | `671C1BD8AABD7D54134C97CBAFB88A56207920C32AF85DBC9AB71982D58047FF` |
| `P21.4_game_build_final.log` | Game | Succeeded / native 0 | 8 / 36.86s | 2,603 | `DBE663E8550D47FAF3A1B4BF2A707F012B2C603577536190913D12CAF653B46A` |
| `P21.4_editor_build_final.log` | Editor | Succeeded / native 0 | 0 / 1.13s | 1,026 | `CEB23A8240667D21E57C03342A871FC6B72551DC8F9CB4BF62E01EF670293853` |

Artifacts：

- `Binaries/Win64/demo_map.exe`：359,382,528 bytes / SHA-256 `658FF51B2A23F060F2E11CE56D7CCFA4E37D6C129FE3E51752CCB388EFB74C08`；
- `Binaries/Win64/UnrealEditor-demo_map.dll`：18,544,128 bytes / SHA-256 `29FB2A49D7841B02D422711F893F45427D6DB65B3B8757C3CCC482D44F40CEED`。

## 8. 提交边界

计划提交 6 个实现、测试和流程文件、本 Report 与本 Development Log，共 8 个文件。103 份用户原有 untracked 文件保持未暂存；`Saved/Codex/P21.4` raw logs 不入 Git。

未修改 Content、地图、项目资源、Engine、Windows、save schema、inventory authority、Run authority、P6 orbit 数学或 Impact/effect resolver。未运行 Editor UI、PIE、Standalone、产品 executable、真实输入、截图、Smoke、Cook 或 Package。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p21-4-training-flying-sword-threat-cue>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p21-4-training-flying-sword-threat-cue/Docs/Report/Dev.D.UE.0.0.10.P21.4.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p21-4-training-flying-sword-threat-cue/Docs/Log/Dev.D.UE.0.0.10.P21.4.r0_log.md>
