# Dev.D.UE.0.0.10.P21.4.r0 Report

## 1. 结论

P21.4 在 P 阶段边界内完成，结论为 **PASS**。

本轮把 P21.3 已由真实 World overlap 和 P6.22 接受的 `ThreatPresence` 样本，投影为 canonical “练习飞剑”本体上的近身威慑提示：注册敌人进入飞剑碰撞盒后，附着在同一 Actor 上的青绿色点光源点亮；下一份已接受的空样本到达后熄灭。该提示不创建伤害、控制、AI、库存、Timer 或第二套 Run 权威。

```text
Threat cue focused:                        1 Success / 0 Fail
Shanmen.0_0_10 full:                    1227 Success / 0 Fail
Required 0.0.9B compatibility groups:    123 Success / 0 Fail
Regression coverage:                      PASS (Changed=6 / Rules=4 / Required=71 / Logs=6)
Regression gate self-test:                PASS 431/431
Game + Editor Development:                PASS / native status 0
```

本轮没有启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件，也没有执行真实输入、截图、Smoke、Cook 或 Package。因此证明的是无头真实 World 中的提示状态、可见组件开关、身份栅栏和产品接线；不宣称最终美术、音效或人工手感验收通过。

## 2. 产品行为

`Ademo_mapShanmenControlledWeaponActor` 现在拥有一个只负责提示的 `UPointLightComponent`。初始及无接触状态隐藏；P21.3 样本的 `RoutedContactCount > 0` 时显示，已接受的空样本时隐藏。

提示状态保留最近一次已投影样本的 sequence 和 intent：

- exact Run 与 exact item identity 不匹配时拒绝；
- 旧 sequence 不能重新点亮飞剑；
- 同 sequence、同 intent、同状态的重复调用为幂等成功；
- 同 sequence 但 intent 或状态冲突时失败关闭；
- lifecycle 停用 Actor 时无条件熄灭提示；
- presentation clear 不推进样本水位，不伪造新的 P6.22 消费。

点光源只附着在 P21.2 已发布的同一个物理飞剑 Actor 上，没有创建替代 carrier 或并行呈现权威。

## 3. GameMode 接线

既有 `Ademo_mapGameMode::Tick` 在 P21.3 sampler 返回成功样本后，把该结果直接交给当前 lifecycle Actor。带接触样本点亮提示，空样本熄灭提示；presentation 拒绝会产生明确错误日志。

当 sampler 确认飞剑已不再 orbit 时，GameMode 请求清除提示。Run teardown 的既有 Actor 停用路径也会清除提示，因此提示不能跨 Run 或跨 item 残留。

## 4. 产品自动化

新增：

```text
Shanmen.0_0_10.Product.ControlledWeaponThreatCue.AcceptedPresenceAndEmptyRelease
```

测试在启用 scene、physics 与 trace collision 的 `GamePreview UWorld` 中贯通：Profile → Code B/cutover → durable Run/Runtime → Coordinator → P21.2 World flying sword → 真实碰撞盒 → 注册 M01 敌人 → P21.3 sampler → P6.22 Router/Host → P21.4 Actor cue。

核心断言：

- 新生成飞剑的提示关闭且无样本水位；
- tick 0 的真实敌人接触被 P6.22 接受后，同一 Actor 的提示状态与可见光源同时开启；
- exact replay 不增加水位且保持状态；
- foreign item 样本不能重绘该飞剑；
- 敌人移出后，tick 3 的已接受空样本关闭提示；
- stale positive 样本不能重新点亮；
- 整个提示周期内敌人 vitality 与 committed Impact 数均不变。

## 5. 回归证据

| Evidence | Success | Fail | Bytes | SHA-256 |
|---|---:|---:|---:|---|
| Threat cue focused | 1 | 0 | 261,743 | `1334B7FC6A02CCA8A2C5F20FF70F1B69273C849EA40C2A773237E3CB160C603E` |
| `Shanmen.0_0_10` full | 1227 | 0 | 1,883,150 | `B3B21533CB1E092801A6F7350CC33374908287267D861BD04010F64966069E41` |
| `demo_map.V3.Attributes` | 4 | 0 | 264,831 | `A6718C21BBA027ACD971C30F487C840D4F97A087EA3AD2CF70E31044C37484BC` |
| `demo_map.EnemySkillFramework` | 44 | 0 | 305,195 | `884E1B5ED159B6DC22EA2BA8D2EAD289565E2B191C4B914D86B65275C5779519` |
| `demo_map.V2RangedCompatibility` | 22 | 0 | 284,867 | `A07B1A7900293F4B1D626A2062234F007FEF1B9F4E45C9B730551389F9B9D4E7` |
| `demo_map.ItemUseAndArmor` | 46 | 0 | 309,439 | `2BF8DEE30EDB18470CC4F4625BC22A468085CBD89AA1E2B4FC00E62CF762637B` |
| `demo_map.P4.Hotbar` | 7 | 0 | 266,855 | `D5853D60A477EADD488242B65B1B0692D718DFA6621A0C4536DB90C292180A86` |

七份最终自动化日志合计 1351/0（focused 与 full 有意重叠）。完整套件从 `2026-09-07 20:32:51.175` 至 `21:52:27.406`，单一 UnrealEditor-Cmd 实例自然清空 1227 项并原生退出 0；所有最终自动化日志的 Fatal、Unhandled 与 Ensure 命中均为 0。

## 6. 改动覆盖门

Actor 的新映射规则要求 ThreatCue、P21.3 World sampler、P6.22 Router、P6 Host、P21.2 lifecycle、fixed timeline、Coordinator、WorldGameplay 与 CombatRuntime 证据。GameMode、测试与流程文件继续接受既有重叠规则的并集检查。

- regression self-test：`PASS 431/431`，42,559 bytes，SHA-256 `A739CBAAB04E06DF61528E2DBAF087BB34DD62150668F3B166FF4480FC4A3605`；
- changed-file gate：`PASS Changed=6 Rules=4 Required=71 Logs=6`，8,513 bytes，SHA-256 `B4320FF634879F06224B63A31BB9402422550C71F91C3C5AE83E2B54AC40BF42`。

## 7. 静态审计与构建

- 6 个实现、测试和流程文件，约 `+389 / -12`；
- 新增生产行中 damage/vitality、AI、inventory transaction、Timer 与 RNG 命中 0；
- `git diff --check` 退出码 0，仅有工作树 LF→CRLF 提示；
- 103 个用户原有 untracked 文件保持未暂存；
- 验证结束后本项目无 UnrealEditor-Cmd 残留进程。

| Target | Result | Actions / Time | Log SHA-256 |
|---|---|---|---|
| Editor initial | Succeeded / native 0 | 9 / 42.59s | `671C1BD8AABD7D54134C97CBAFB88A56207920C32AF85DBC9AB71982D58047FF` |
| Game Development final | Succeeded / native 0 | 8 / 36.86s | `DBE663E8550D47FAF3A1B4BF2A707F012B2C603577536190913D12CAF653B46A` |
| Editor Development final | Succeeded / native 0 | 0 / 1.13s | `CEB23A8240667D21E57C03342A871FC6B72551DC8F9CB4BF62E01EF670293853` |

最终产物：

- `demo_map.exe`：359,382,528 bytes，SHA-256 `658FF51B2A23F060F2E11CE56D7CCFA4E37D6C129FE3E51752CCB388EFB74C08`；
- `UnrealEditor-demo_map.dll`：18,544,128 bytes，SHA-256 `29FB2A49D7841B02D422711F893F45427D6DB65B3B8757C3CCC482D44F40CEED`。

## 8. P/F 边界与后续

PASS：canonical TrainingFlyingSword 把真实、已接受的近身接触投影成同一 Actor 的可见提示；空样本、重复、foreign item、stale sequence 与 teardown 行为确定；提示不改变敌人战斗状态或任何 durable authority；完整、兼容、覆盖门和双目标构建通过。

未声明：敌人减速、硬直、恐惧、击退、伤害、格挡、持续状态、音效、最终 VFX、美术质量或人工游戏验收。这些行为需要独立正式契约，不能由 `ThreatPresence` 提示隐式扩张。

下一步建议 P21.5 把飞剑近身威慑的玩家可读性接入轻量 HUD/音效或可配置视觉参数，但仍不发明 AI/control/damage 语义。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p21-4-training-flying-sword-threat-cue>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p21-4-training-flying-sword-threat-cue/Docs/Report/Dev.D.UE.0.0.10.P21.4.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p21-4-training-flying-sword-threat-cue/Docs/Log/Dev.D.UE.0.0.10.P21.4.r0_log.md>
