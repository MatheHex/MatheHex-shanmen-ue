# Dev.D.UE.0.0.10.P18.4.r0 Report

## 1. 结论

P18.4 在 P 阶段边界内完成，结论为 **PASS**。

本轮把 P18.3 的 Sword Qi Product Session 接入唯一 Run-scoped 产品控制器：从现有 Runtime 物品/装备权威只读取得 WeaponSlot 的 exact item instance，冻结最终 AttackPower 与设备无关轨迹，保留一次确定性 Run sequence，并统一承担重放、占用、终止、retirement 与 Run teardown。`Ademo_mapGameMode` 只暴露该控制器入口，没有建立第二套装备、库存、属性、动作或生命权威。

```text
Sword Qi Item Adapter exact:           4 Success / 0 Fail
Sword Qi Product Controller retry:     4 Success / 0 Fail
Shanmen.0_0_10 full:                 773 Success / 0 Fail
Required legacy groups:              443 Success / 0 Fail
Regression coverage:                  PASS (Changed=16 / Rules=7 / Required=56 / Logs=11)
Regression gate self-test:             PASS 283/283
Boundary scan:                         PASS (Files=6 / Matches=0)
Game + Editor Development:             PASS / native status 0
```

本轮没有接真实输入、按键映射、装备 UI、正式表现资源、声音或关卡投放，也没有启动 Unreal Editor UI 或产品。因此证明的是装备到剑气产品命令的代码级唯一入口与 Run 生命周期闭环，不宣称实际手感、视觉或人工验收通过。

## 2. 装备权威与内容身份

新增 `SwordQiSource` 物品语义，并附加到六个 canonical 武器定义：Training Blade、Heavy Practice Blade 与一至四阶兵器。它与 `WeaponGuard` 是彼此独立的显式语义，适配器不会因为物品“看起来像武器”或具备格挡能力而推断剑气资格。

内容身份更新为：

- version：`CodeB.Content.0.0.10.P18.4`；
- digest：`D6EF276B3BB268D33A9E1242DC3620A7F33F2B4B966503E1056BA3E381C7B043`；
- P17.0 identity 保留为历史可识别证据，不改 save schema，也不重映射既有 DefinitionId。

`Fdemo_mapShanmenSwordQiItemAdapter` 不接受调用方提供的 item id，只读取现有 authority 的 exact WeaponSlot occupant。它验证 authority invariants、owner/container/slot/quantity、canonical definition、明确语义和内容身份，并生成绑定 authority revision 的确定性 authorization。装备替换、卸下或任意 authority revision 变化都会使旧证据失效。

## 3. 设备无关意图与一次冻结

`Fdemo_mapShanmenSwordQiIntent` 只包含 IntentId、RunId、有限 origin 与规范化 aim direction，不包含键盘、鼠标、手柄或 UI 类型。

一个新 IntentId 的处理顺序固定为：

```text
device-independent intent
  -> read exact equipped SwordQiSource once
  -> read final AttackPower once
  -> reserve one Run Sword Qi sequence
  -> freeze P18.3 launch command
  -> route through the existing Product Session / Host
```

相同 IntentId 与相同 payload 重放时直接复用冻结命令，不再读取装备或属性，也不再消耗 Run sequence。相同 IntentId 搭配不同 Run、origin 或 direction 会以 `IntentIdConflict` 失败关闭。

## 4. Run 控制器与产品所有权

`Fdemo_mapShanmenSwordQiProductController` 是剑气的唯一 Run-scoped composition owner：

- `TryBegin` 绑定 exact active Run；
- `TrySubmit` 组合物品适配器、属性 authority、CombatRunCoordinator 与 P18.3 Product Session；
- `TryAppendOccupancy` 复用既有 Player Action Arbitration；
- interrupt、range expiry 与 terminal retirement 委托现有 Session/Host；
- `TryEnd` 中断仍在飞行的载体、复制 terminal receipt、清空 Session 与冻结意图，再解除 Run identity。

GameMode 在 Combat Run 成功建立 Sword Rhythm 产品/表现 owner 后绑定剑气控制器，并在 Run 释放时先完成 Meridian Shock durable recovery，再结束剑气控制器，保持既有“先恢复持久事务、后拆产品 authority”的顺序。激活 rollback、orphan recovery 与空状态检查也包含本控制器。

## 5. 冲突、重试与失败关闭

- 未装备明确 `SwordQiSource` 的 WeaponSlot item：在 sequence reservation 前拒绝；
- Intent、Controller 与 Coordinator 的 Run 不一致：在任何产品 I/O 前拒绝；
- final AttackPower 不存在、非有限或为负：不冻结命令；
- Host busy：保留已冻结的 intent/command，待上一发 terminal retirement 后可精确重试；
- exact replay：不重采样装备/属性，不重复 reserve，不改变 CommandId；
- 持久动作通道已占用：沿用既有 gate 的可重放拒绝结果；
- Run teardown：不会遗留飞行载体、terminal proof 或隐藏 command work。

适配器与控制器均不调用库存 Reserve/Commit/Consume/Add/Remove，不修改装备，也不直接写伤害或生命。

## 6. Automation 证据

新增八条测试：Item Adapter 的 `CanonicalCatalog`、`ExactAuthorization`、`EquipmentFences`、`RevisionFence`；Product Controller 的 `RouteAndRunEnd`、`FrozenReplay`、`BusyRetry`、`Fences`。

| Evidence | Success | Fail | SHA-256 |
|---|---:|---:|---|
| Sword Qi Item Adapter exact | 4 | 0 | `42305051323BDC2324CCBC90A26AB4F740BA338975780875A844EDE47AED3FDA` |
| Sword Qi Product Controller retry | 4 | 0 | `E6CD2956B92A581BE55EDCC720C75967121198E03BBE38B6DA67C0EA39A056E8` |
| `Shanmen.0_0_10` full | 773 | 0 | `73D48AB6BC2F148A333C46C5E9913FD0E58547DC8497CE930DA0FEF10A2CB088` |
| `demo_map.ItemEconomySchema` | 24 | 0 | `0C37D5BEA68AC17EBD16BFAF364E671FB29E3505F5B96968C0861C348056ECE4` |
| `demo_map.Profile` | 211 | 0 | `D38B3D3D0B4D4B93BB0FCBD0232726DBA6FC5B19D833694329B88FE23A879F0D` |
| `demo_map.CodeB` | 60 | 0 | `7C176FE778ED3CA72B9FE893167F104AA66B933963D4AD8268CECD510E04A881` |
| `demo_map.ItemUseAndArmor` | 46 | 0 | `BB773C276AD823B6AC8A20BE6497449AD5254FA0577F8FA13062371D11A9EBC0` |
| `demo_map.P4.Hotbar` | 7 | 0 | `64E7CEBF71FA4870BE091A678AD5AD3BE95347B33300654B7EF3C7E0BF4780B4` |
| `demo_map.V3` | 29 | 0 | `897664DA4D342C0154EC4BCB6843A303A8122F3B9CA397F39B0FE2573C1E8D28` |
| `demo_map.EnemySkillFramework` | 44 | 0 | `CA06C5BE3CB6DCD957276AED1EDB1FFC7321E4A9D7CB23C3C0337C7E2AC004C4` |
| `demo_map.V2RangedCompatibility` | 22 | 0 | `C25D0519929476C95C519FF52AC39100DDDFFA48D0DFD4904C8D9ECE137A3BAD` |

所有采用日志都有 terminal `TEST COMPLETE` 与 exit code 0，Fail、Fatal 与 Unhandled 为 0。完整套件首末 Success 为 `2026.09.02 23:54:54.110 -> 2026.09.03 00:26:01.252 UTC`，约 31m07.142s；相对 P18.3 的 765 条精确增加 8。

## 7. 首败保留与修正

Product Controller exact 首次运行真实失败为 `0 Success / 4 Fail`，terminal exit `-1`、进程状态 `255`，原始日志 SHA 为 `1D3A672095A5DF61CF47A4B36AA6A44306A2C78BD0B72E8C14A71A55A497735E`。

根因是测试 fixture 尝试直接设置派生属性 `AttackPower`；现有属性 authority 正确拒绝该写入，导致四个 fixture 在产品路由前同时失败。修正仅发生在测试：改为设置基础属性 `Primary01`，再读取真实 final AttackPower（测试值 7、99、13）。生产控制器、公式与属性 authority 均未为迁就测试而修改。重编译后 exact retry 为 4/0，完整与旧回归全部通过。首次失败日志保留在本地审计目录，没有被覆盖或当成成功证据。

代码复查还发现初稿 teardown 把剑气清理放在 Meridian Shock durable recovery 前；该顺序在提交前被调整为先恢复持久事务、再拆剑气控制器，并由最终 Editor/Game 构建覆盖。

## 8. 改动驱动回归与静态门禁

新增 `SwordQiItemAdapter` 与 `SwordQiProductController` 路径规则，并把 GameMode 的依赖扩展到 Sword Qi Controller、Item Adapter、Product Session、Run Host 与 World Delivery。健康日志通过门禁：

```text
REGRESSION_COVERAGE: PASS Changed=16 Rules=7 Required=56 Logs=11
SELF_TEST: PASS 283/283
BOUNDARY_SCAN: PASS Files=6 Matches=0
GIT_DIFF_CHECK: PASS
```

- coverage SHA：`E96A1D05270BBEBEE99B0E96440958C4267C05C163E0D805867643F2FA8B972E`；
- self-test SHA：`6382D19F1DD45D0763A9672CD732DC44D74EC520285F5510BCB2EBCFFE61781B`；
- regression map SHA：`B5FC5E1809918D8FF3EF5E4F137D29DE451D676246240B8FB4AD9AAB89D18CEF`；
- boundary SHA：`D1598293097DB92895DA0482B555258EA4D2F5F7ECD6DBE78F98844BEF3B2DDF`。

边界扫描覆盖六个新增 Adapter/Controller 文件，确认没有直接 GameplayStatics/ApplyDamage/TakeDamage、旧 SkillProjectile、RNG、库存写事务、输入绑定、音效或 Niagara 调用。首次失败控制器日志没有进入 coverage evidence。

## 9. 构建、产物与 P/F 边界

使用 UE 5.8、`-WaitMutex -NoHotReload -NoUBA -MaxParallelActions=1`：

| Target | Result | Actions / Time | Log SHA-256 |
|---|---|---|---|
| Editor Development initial | Succeeded | 198 / 543.21s | `721A4BB277650FA0574DC7D9DB1851E006E72C1FAEBEC1A03418DE5FAFC6AB68` |
| Game Development final | Succeeded / native 0 | 197 / 568.65s | `2EC57CF215B9ABE85B062F556B49B878E4D073122F39213FDADF5CD28DCC409D` |
| Editor Development final | Succeeded / native 0 / up to date | 0 / 1.31s | `51BFC236F14A929C23984E35CC6ABF65D25F6C521692A5C9D98B9A551F2A865E` |

最终产物：

- `demo_map.exe`：356,381,696 bytes，SHA-256 `24BD29C1178DF1FFA4C66B6A039ED676249DF9A56DD01C5A7E34A30D46CD3F43`；
- `UnrealEditor-demo_map.dll`：15,054,336 bytes，SHA-256 `39C60B0306F8B66495096FDC2134188F4031CAC3ECF76C909CD757C380B36C0A`。

本轮计划提交 6 个新增源码文件、10 个修改文件及本 Report/Development Log，共 18 个文件。未修改 Content、地图、资源、配置、Windows、UE Engine、存档 schema 或既有库存/属性/生命权威。只执行 unattended、NullRHI Automation、静态门禁与 Development builds；未运行 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。raw logs 只保存在本地 `Saved/Codex/P18.4`。

## 10. 下一阶段

P18.5 建议新增薄型 Sword Qi command/input adapter：只把既有玩家动作意图转换为 `Fdemo_mapShanmenSwordQiIntent` 并调用本轮唯一 GameMode route，复用现有占用与 Run identity，不在输入层读取装备、属性或伤害配置。仍以无头测试验证；正式按键、视觉、声音与手感留给明确授权的 F 阶段。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p18-4-sword-qi-run-controller>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p18-4-sword-qi-run-controller/Docs/Report/Dev.D.UE.0.0.10.P18.4.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p18-4-sword-qi-run-controller/Docs/Log/Dev.D.UE.0.0.10.P18.4.r0_log.md>
