# Dev.D.UE.0.0.10.P21.0.r0 Report

## 1. 结论

P21.0 在 P 阶段边界内完成，结论为 **PASS**。

本轮补齐了 P6 受控武器链长期缺少真实产品内容的问题：canonical 目录现在包含一把可购买、可装备、可持久化的“练习飞剑”，Code B 与 Shanmen 物品权威会把同一精确 DefinitionId 投影为 `Capability.Deploy` 与 `Item.Weapon.FlyingSword`，因此可复用既有 P6 准入、运行生命周期和战斗结算链，不建立第二套飞剑、库存或战斗系统。

```text
Controlled-weapon product prefix:       38 Success / 0 Fail
Shanmen Items:                           77 Success / 0 Fail
Required legacy groups:                 374 Success / 0 Fail
Shanmen.0_0_10 full:                   1223 Success / 0 Fail
Regression coverage:                    PASS (Changed=13 / Rules=8 / Required=14 / Logs=10)
Regression gate self-test:              PASS 423/423
Game + Editor Development:              PASS / native status 0
```

本轮没有启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件，也没有执行真实输入、截图、Smoke、Cook 或 Package。因此证明的是内容、交易、持久化、权威投影与 P6 类型契约的无头闭环，不宣称飞剑模型、动画、手感或人工可视化验收通过。

## 2. Canonical 练习飞剑

新增稳定内容：

- DefinitionId：`Prototype.Item.Weapon.TrainingFlyingSword`；
- 显示名：`练习飞剑` / `TRAINING FLYING SWORD`；
- category：Weapon；level：1；max stack：1；
- equipment slot：WeaponSlot；
- buy/sell/value：100 / 50 / 100；
- gameplay semantic：仅 `FlyingSword`；
- 不具备 Hotbar、durability、charges、ThrownWeapon、WeaponGuard 或 SwordQiSource 语义。

该定义追加在原 42 项目录末尾，未插入既有序列；`FlyingSword` 也追加在语义枚举末尾。因此既有目录顺序和已持久化枚举序值保持稳定。商店有序清单把它追加为第 16 个可购买项目，通用买卖、装备与存档流程不需要专用分支。

## 3. 内容身份与历史兼容

内容身份更新为：

- version：`CodeB.Content.0.0.10.P21.0`；
- parent：P18.4 digest `D6EF276B3BB268D33A9E1242DC3620A7F33F2B4B966503E1056BA3E381C7B043`；
- digest：`A52AA4EEE9DBF314C017205BFC6E417B8C9A108D37471C7DDE088013B85FC685`。

摘要由包含 parent、DefinitionId、类别、等级、堆叠、槽位、价格、价值和语义的 canonical UTF-8 串独立重算并精确匹配。P18.4 被加入历史已知 identity；没有修改 save schema、重映射旧 DefinitionId 或覆盖旧内容摘要。

## 4. Code B 与 Shanmen 权威投影

`Fdemo_mapShanmenItemMigration` 只从新的 typed `FlyingSword` semantic 投影 `Shanmen.Item.Weapon.FlyingSword`。现有非堆叠装备投影继续提供 `Shanmen.Item.Capability.Deploy`；本轮没有按显示名、category 或 WeaponSlot 猜测飞剑身份。

新增 `CanonicalAuthorityProjection` 自动化验证同一精确产品：

1. canonical 目录与商店价格正确；
2. Code B 将其保留为不可堆叠、不可快捷使用的 WeaponSlot 物品；
3. Shanmen migration 保留 exact item instance、DefinitionId 与 quantity；
4. authority definition 同时具有 Deploy 与 FlyingSword 标签，且没有 ThrownWeapon 标签；
5. P18.4 identity 仍可作为历史证据识别。

P6 `ControlledWeaponAdapter` 的既有准入测试要求的正是这两个精确 authority tags，并验证 deployed instance、Run correlation、revision 与只读冻结。两组测试在同一 `ControlledWeapon` 前缀下合计 38/0；这是可组合的代码级契约证据，不冒充一次真实 UI 购买、装备与按键操作。

## 5. 回归与首败保留

正式自动化证据：

| Evidence | Success | Fail | SHA-256 |
|---|---:|---:|---|
| ControlledWeapon prefix | 38 | 0 | `4F26E04335D625DB97416D0CC11DDF2064E897188AA68583FC3AC55FAB7B8DE2` |
| `Shanmen.0_0_10.Items` | 77 | 0 | `C04FEA4D410E6992657F647729F8296CE64D1168BA7B76FF05D9805CDE8E5C21` |
| `demo_map.ItemEconomySchema` | 24 | 0 | `5DDF89F6B4B927A568A4C48B2462224F562C74DC41E68A235133543E9CB8BD7C` |
| `demo_map.Profile` | 211 | 0 | `58E5CD5A72CD3E46C7D852B5986FB32E3AE4F1DE598F737B209D94B9F1F3A7ED` |
| `demo_map.CodeB` | 60 | 0 | `CF6E84A430C831A745435EB1DB6D5E37343FFE4273F4579234FD46C1BACA87C8` |
| `demo_map.ItemUseAndArmor` | 46 | 0 | `A76E9E53EDCA373BC74509B210DC5E453B11F586366C5FF62D775B78EBBF8561` |
| `demo_map.P4.Hotbar` | 7 | 0 | `4EDFC4F4A1BD7A0FDEEE3725B18D099E823CF628AFDD1D9F83EDD285798C27A0` |
| `demo_map.V3.WorldInteraction` | 4 | 0 | `0F061A46A3E191AFFD5EF9A7A65DEC2E251C0F93BD7DE8A7C4FB113A9D6356A2` |
| `demo_map.V2RangedCompatibility` | 22 | 0 | `678A839F010271B2968578E6DF2CC2E78B4125F0FB61849592C0E86B8A42179E` |
| `Shanmen.0_0_10` full | 1223 | 0 | `424D5BF9A23552F059409D7EA033BE10739188251B6375A05E36B5746B6EA9E3` |

十份正式日志合计 1712/0（套件存在有意重叠）。完整套件从 `2026-09-07 11:56:14.808` 到 `12:59:25.134 UTC`，单一 UnrealEditor-Cmd 实例自然清空；相对 P20.66 的 1222 项精确增加 1 项。所有日志均有 native terminal-success marker，Fatal、Unhandled、Assertion 为 0。

第一次 changed-file gate 真实拒绝现有证据：缺少 `demo_map.V2RangedCompatibility`，失败证据 SHA `662A5C9C091EC06D7233F3FA3EDD29CC18754DCD3B8465DE1C4B3AEADC99D37C`。未放宽映射；补跑 22/0 后最终门禁通过：

```text
REGRESSION_COVERAGE: PASS Changed=13 Rules=8 Required=14 Logs=10
SELF_TEST: PASS 423/423
```

最终 gate SHA 为 `370AD6B4A07682CF6DA666089CBB89EDF3D4482D24A73E5F7591DD6FACFA7C06`；self-test SHA 为 `B8155642B3B98FFBC7D0B185E0011DEB0C4795F3417BAFEB62E45C9905843315`。

## 6. 静态审计与构建

- 13 个源码/测试文件，`+185 / -32`；
- 新增差异中 World、Actor、RNG、ApplyDamage 与 GameMode 直接接线命中为 0；
- `git diff --check` 原生退出码 0，仅报告工作树 LF→CRLF 提示；
- 103 份用户原有 untracked 文件保持未暂存；
- 自动化结束后无 UnrealEditor、UnrealEditor-Cmd 或 demo_map 残留进程。

| Target | Result | Actions / Time | Log SHA-256 |
|---|---|---|---|
| Editor Development initial | Succeeded | 206 / 541.97s | `12465A9657BF0234F0F325AD68C3588F7F973B5EB8469BA106102A8CD151444D` |
| Game Development final | Succeeded / native 0 | 205 / 520.34s | `9ACD33EB1673869924199F299461F62C71F106309008B67994B6E7B3D5E3B1B3` |
| Editor Development final | Succeeded / native 0 | 0 / 0.97s | `D51F6AFF3D1465F56D1B2B0F1A077107293D9DC6192C0C648623364553370561` |

最终产物：

- `demo_map.exe`：359,299,072 bytes，SHA-256 `CD29B6E61F7C44636D8E732929532A2E8655C74519FCB802A41F48B25DD2002C`；
- `UnrealEditor-demo_map.dll`：18,438,144 bytes，SHA-256 `DD7863EA99ECE13B3291EFDA0EE4F10185D6AC2919AAC954AD7814AB8606077F`。

## 7. P/F 边界与后续

PASS：稳定目录身份、append-only 顺序、价格/商店投放、Code B 形态、Shanmen exact instance 投影、Deploy/FlyingSword 标签、ThrownWeapon/Guard/SwordQi 隔离、历史 identity、P6 契约组合、路径映射回归、全量自动化、Game/Editor 构建。

未声明：真实商店点击、手工装备、实际 Run 按键、飞剑 Actor/模型/动画/音效、屏幕表现、手感、PIE、Standalone、产品启动、截图、Smoke、Cook、Package。

下一项应继续补真实战斗内容，而不是增加交接包装层。若保持无界面边界，优先把另一类已有纯契约能力接入 canonical 可获取物品；若允许一次聚焦产品运行，则验证练习飞剑的购买、装备、进入 Run 与 P6 激活可视链。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p21-0-training-flying-sword-content>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p21-0-training-flying-sword-content/Docs/Report/Dev.D.UE.0.0.10.P21.0.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p21-0-training-flying-sword-content/Docs/Log/Dev.D.UE.0.0.10.P21.0.r0_log.md>
