# Dev.D.UE.0.0.10.P21.0.r0 Development Log

## 1. 基线与目标

- base：`f4dec405908444d6156c031ebcea74103b8cf7f8`（P20.66 MainHUD combat-hint layout policy）；
- branch：`agent/0.0.10-p21-0-training-flying-sword-content`；
- 目标：为既有 P6 controlled-weapon runtime 提供第一件真实 canonical 飞剑产品；
- 边界：复用物品、交易、装备、迁移与 P6 权威，不增加输入、Actor、伤害、生命或表现系统，不启动产品。

## 2. 起始审计

P6 已有完整受控武器定义、准入、Session、Host、Run lifecycle、command router 与 threat routing，但所有测试都临时构造 `Item.Test.FlyingSword.*`。canonical `Fdemo_mapItemDefinitions` 中没有任何携带 `Shanmen.Item.Weapon.FlyingSword` 的真实物品，玩家即使沿既有商店与装备流程也无法取得能通过 P6 精确准入的产品。

本轮选择最小真实内容切片：新增一把练习飞剑，通过 typed semantic 投影到既有 authority tag；不修改 P6 runtime，也不从名字或槽位推断能力。

## 3. 实现

生产改动：

- `demo_mapItemDefinitions.h/.cpp`：新增 `TrainingFlyingSword`，追加第 43 项定义、商店条目、目录不变量与 P21.0 content identity；
- `demo_mapItemTypes.h`：在枚举末尾追加 `FlyingSword` semantic；
- `demo_mapShanmenItemMigration.cpp`：把该 typed semantic 投影为 `ItemWeaponFlyingSword`；
- `demo_mapV3ProgressionManager.cpp`：同步 canonical 目录数量断言 42→43。

测试改动：

- 目录、经济、商店顺序、Profile trade、WorldInteraction 数量断言同步；
- migration fixture 增加 exact flying-sword instance；
- 新增 `CanonicalAuthorityProjection`，验证 catalog→Code B→Shanmen 的 DefinitionId、instance、quantity、slot 与 tags；
- Meridian Shock、Sword Qi、Weapon Guard identity 测试更新 current P21.0，并保留各自历史 identity 断言。

新定义只拥有 `FlyingSword` 语义；明确排除 ThrownWeapon、WeaponGuard、SwordQiSource、Hotbar、durability 与 charges。它追加在 registry 和 purchasable order 末尾，避免旧 ordinal 漂移。

## 4. 内容摘要

canonical 串：

```text
CodeB.Content.0.0.10.P21.0|Parent=D6EF276B3BB268D33A9E1242DC3620A7F33F2B4B966503E1056BA3E381C7B043|Add=Prototype.Item.Weapon.TrainingFlyingSword|Category=Prototype.ItemCategory.Weapon|Level=1|MaxStack=1|Slot=Prototype.Slot.Weapon|Buy=100|Sell=50|Value=100|Semantic=FlyingSword
```

UTF-8 SHA-256 独立重算为 `A52AA4EEE9DBF314C017205BFC6E417B8C9A108D37471C7DDE088013B85FC685`，与源码精确匹配。P18.4 version/digest 被加入 known historical identity；未改 save schema。

## 5. 编译与自动化

首次 Editor build 为完整 206 actions，541.97s，Succeeded。关键定义、UENUM、迁移和新增测试均一次通过，没有源码修复轮。

正式日志：

| Log | Group | Success/Fail | Bytes | SHA-256 |
|---|---|---:|---:|---|
| `P21.0.r0_controlled_weapon_first.log` | ControlledWeapon prefix | 38/0 | 303,317 | `4F26E04335D625DB97416D0CC11DDF2064E897188AA68583FC3AC55FAB7B8DE2` |
| `P21.0.r0_items.log` | `Shanmen.0_0_10.Items` | 77/0 | 350,982 | `C04FEA4D410E6992657F647729F8296CE64D1168BA7B76FF05D9805CDE8E5C21` |
| `P21.0.r0_economy.log` | `demo_map.ItemEconomySchema` | 24/0 | 283,405 | `5DDF89F6B4B927A568A4C48B2462224F562C74DC41E68A235133543E9CB8BD7C` |
| `P21.0.r0_profile.log` | `demo_map.Profile` | 211/0 | 509,102 | `58E5CD5A72CD3E46C7D852B5986FB32E3AE4F1DE598F737B209D94B9F1F3A7ED` |
| `P21.0.r0_codeb.log` | `demo_map.CodeB` | 60/0 | 312,662 | `CF6E84A430C831A745435EB1DB6D5E37343FFE4273F4579234FD46C1BACA87C8` |
| `P21.0.r0_item_use.log` | `demo_map.ItemUseAndArmor` | 46/0 | 309,624 | `A76E9E53EDCA373BC74509B210DC5E453B11F586366C5FF62D775B78EBBF8561` |
| `P21.0.r0_hotbar.log` | `demo_map.P4.Hotbar` | 7/0 | 267,426 | `4EDFC4F4A1BD7A0FDEEE3725B18D099E823CF628AFDD1D9F83EDD285798C27A0` |
| `P21.0.r0_world.log` | `demo_map.V3.WorldInteraction` | 4/0 | 264,474 | `0F061A46A3E191AFFD5EF9A7A65DEC2E251C0F93BD7DE8A7C4FB113A9D6356A2` |
| `P21.0.r0_ranged.log` | `demo_map.V2RangedCompatibility` | 22/0 | 284,441 | `678A839F010271B2968578E6DF2CC2E78B4125F0FB61849592C0E86B8A42179E` |
| `P21.0.r0_full.log` | `Shanmen.0_0_10` | 1223/0 | 1,873,984 | `424D5BF9A23552F059409D7EA033BE10739188251B6375A05E36B5746B6EA9E3` |

合计 1712/0（有意重叠）。完整套件保持单个 UnrealEditor-Cmd 实例，从首项到末项约 63m10.326s，自然清空且 native status 0；无 Fatal、Unhandled 或 Assertion。

## 6. 回归门禁首败与修复

Regression self-test：423/423，41,964 bytes，SHA `B8155642B3B98FFBC7D0B185E0011DEB0C4795F3417BAFEB62E45C9905843315`。

第一次 changed-file gate 按设计失败：已有九份日志缺 `demo_map.V2RangedCompatibility`。失败 marker 保存为 `P21.0.r0_regression_gate_first.log`，78 bytes，SHA `662A5C9C091EC06D7233F3FA3EDD29CC18754DCD3B8465DE1C4B3AEADC99D37C`。修复没有修改代码或放宽规则，只补跑 V2RangedCompatibility 22/0。

最终结果：

```text
REGRESSION_COVERAGE: PASS Changed=13 Rules=8 Required=14 Logs=10
```

final gate：2,914 bytes，SHA `370AD6B4A07682CF6DA666089CBB89EDF3D4482D24A73E5F7591DD6FACFA7C06`。

## 7. 静态与最终构建

静态日志：digest exact、13 changed source/test files、added forbidden boundary hits 0、`git diff --check` exit 0；269 bytes，SHA `AEA0CB06C03FC892DB42FFE025B80C9CC64C1D0C9561C506C65B298EE7FA4F26`。

- Editor initial：206 actions / 541.97s / SHA `12465A9657BF0234F0F325AD68C3588F7F973B5EB8469BA106102A8CD151444D`；
- Game final：205 actions / 520.34s / native 0 / SHA `9ACD33EB1673869924199F299461F62C71F106309008B67994B6E7B3D5E3B1B3`；
- Editor final：0 actions / 0.97s / native 0 / SHA `D51F6AFF3D1465F56D1B2B0F1A077107293D9DC6192C0C648623364553370561`；
- `demo_map.exe`：359,299,072 bytes / SHA `CD29B6E61F7C44636D8E732929532A2E8655C74519FCB802A41F48B25DD2002C`；
- `UnrealEditor-demo_map.dll`：18,438,144 bytes / SHA `DD7863EA99ECE13B3291EFDA0EE4F10185D6AC2919AAC954AD7814AB8606077F`。

## 8. 提交边界

计划提交 13 个源码/测试文件、本 Report 与本 Development Log，共 15 个文件。103 份用户原有 untracked 文档保持未暂存；`Saved/Codex/P21.0` raw logs 不入 Git。

未修改 Content、地图、资源、配置、Engine、Windows、save schema、P6 runtime 或既有库存/战斗写路径。未运行 Editor UI、PIE、Standalone、产品 executable、真实输入、截图、Smoke、Cook 或 Package。

## GitHub

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p21-0-training-flying-sword-content>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p21-0-training-flying-sword-content/Docs/Report/Dev.D.UE.0.0.10.P21.0.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p21-0-training-flying-sword-content/Docs/Log/Dev.D.UE.0.0.10.P21.0.r0_log.md>
