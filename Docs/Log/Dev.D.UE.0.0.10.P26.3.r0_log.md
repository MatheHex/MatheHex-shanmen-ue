# Dev.D.UE.0.0.10.P26.3.r0 Development Log

## 1. 目标

- 为一个 canonical 非训练防具投放首个保守、标签限定的正式抗性；
- 证明匹配伤害生效、非匹配伤害绕过以及 Impact receipt 守恒；
- 通过真实 M01 敌方攻击验证 active-Run 防具证据、生命提交和重放幂等；
- 保持训练防具 no-op、被动防具只读、旧内容身份与存档 DefinitionId 兼容；
- 按实际改动文件运行回归、双目标构建并交付 Report/Log。

## 2. 基线与范围

- 基线：`7e42e85c486e3edcf8cbb0f5e35086d7c518ff50`（P26.2）；
- 分支：`agent/0.0.10-p26-3-tier1-robe-physical-resistance`；
- 起始 tracked tree clean；103 个既有未跟踪用户条目保持未暂存；
- 不改存档 schema、CombatCore resolver、Armor adapter/route、GameMode、PlayerController、UI、
  地图或资产；
- 不为其他防具或伤害通道推断平衡数值。

## 3. 目录投放

`MakeDefinition()` 增加可选 typed resistance 数组并移动进 immutable definition。
`ArmorRobeLevel1` 获得唯一 `DamageResistance` 语义与：

`Shanmen.Damage.Physical -> ReduceFraction(0.10)`。

`Validate()` 统计正式抗性定义并强制恰有一个。`TrainingVest` 保持无语义、无 resistance 的
显式 no-op control；一阶道袍既有名称、类别、slot、价格、耐久、生命与旧 flat reduction
元数据不变。

## 4. 内容身份

当前版本从 P21.0 升级为 `CodeB.Content.0.0.10.P26.3`。canonical 输入为：

`CodeB.Content.0.0.10.P26.3|Parent=A52AA4EEE9DBF314C017205BFC6E417B8C9A108D37471C7DDE088013B85FC685|Modify=Prototype.Item.Armor.Robe.Level1|Semantic=DamageResistance|DamageTag=Shanmen.Damage.Physical|Fraction=0.10`

无尾换行 UTF-8 SHA-256：
`FB773D9692445401D0C97772498A85F591B735D36E7F2309789ED1EC47EADE74`。
P21.0 版本/摘要加入 historical known identity。持久记录仍以 DefinitionId 解析当前定义，不触发
schema migration 或实例 remap。

## 5. 纯投影与产品测试

`ArmorResistanceProjection.CatalogAndNoOp` 同时验证：

- TrainingVest 投影成功但状态为 `NotApplicable`，Defense 精确不变；
- Tier1 robe 生成一个由 exact item instance 标识的 `Defense.Armor` 层；
- Physical.Slash 的 100 raw 变为 10 prevented / 90 final；
- Mental 的 100 raw 保持 0 prevented / 100 final；
- 两条 receipt 都守恒。

`CombatRunCoordinator.CanonicalArmorResistance` 建立真实 Profile→cutover→active Run，只部署
Tier1 robe。M01 的 1.0 physical Impact 触发该层后提交 0.9 伤害，玩家 3.0→2.1；完整 item
authority snapshot 前后相等，delivery replay 为 `AlreadyCommitted` 且不产生第二次副作用。

## 6. 首错与定向修复

首次 Editor 构建与三个首轮聚焦组均成功。首次完整 `Shanmen.0_0_10` 在运行到既有目录适配器
测试时发现两处 P21.0 “current identity”硬编码，运行当时为 767 Success / 2 Fail，随后主动
停止；原始日志保留，SHA-256
`8C0CA31ADE2887FF1B80FEDF1E009200B75B9A14F357DA4CE9D63142815D92AD`。

同类搜索定位并修正四个 catalog consumer 测试：Meridian Shock Treatment、Item Migration、
Sword Qi Item Adapter、Weapon Guard Item Adapter。每处同时断言 P26.3 current 与 P21.0
known historical。修复后增量 Editor 构建通过，四个定向复测日志合计 41 Success / 0 Fail。

## 7. 自动化结果

| Group | Success | Fail | Fatal | Completion | SHA-256 |
|---|---:|---:|---:|---:|---|
| `ArmorResistanceProjection`（首轮） | 4 | 0 | 0 | 1 | `32A6FF23C221F25D2A9C28E9CD230FCD3EC814BEBEBFBC7C4688AA5E3C118B82` |
| `CombatRunCoordinator`（首轮） | 21 | 0 | 0 | 1 | `098CFB1B3219462CA83DA593C72927C13CFF7C598734D7F05B8D95B977FAB746` |
| `DefinitionRegistry`（首轮） | 1 | 0 | 0 | 1 | `1B82B8DD180C53E05AFFE81754352B478A4D9701D443CFEE62EDEB3BB628D36C` |
| `Shanmen.0_0_10`（最终） | 1,318 | 0 | 0 | 1 | `0F1AE549FD3F3E32B0D1E7474CD4AF91860A88C22F9F77ED53D27F540165656D` |
| `demo_map.Profile` | 211 | 0 | 0 | 1 | `2ED23B96070C97DDDF921EE995BD126FEE0CE4B242C874465B7D7D48AB401373` |
| `demo_map.CodeB` | 60 | 0 | 0 | 1 | `56AA358514C510DAEBBA6FFFC39A6C32CDBC3E401B3A597DC3C59C8C4322C237` |
| `demo_map.ItemEconomySchema` | 24 | 0 | 0 | 1 | `A23EC5B2F6912BD2CAC8E7C46D06F04EE466482B328DC4E51DB68B8895C54ED6` |
| `demo_map.ItemUseAndArmor` | 46 | 0 | 0 | 1 | `59F042BFCEA605C071F32ADCDB1C793BD2A32D7FD285359358338A1567065FED` |
| `demo_map.P4.Hotbar` | 7 | 0 | 0 | 1 | `4C304654CDF5579122796F08E1AE423B331E01F53D95BBAB45C11F8A8CE1E932` |
| `demo_map.V3.WorldInteraction` | 4 | 0 | 0 | 1 | `7682D66C6D24110C51F914CC3CDAF8C277696BFB4D1B7CBF6252D4CBCE2B0870` |
| `demo_map.EnemySkillFramework` | 44 | 0 | 0 | 1 | `28ED70B1BFBCD672BCE4B21A851750450FEC08E81DC2713ED78EDFD2DBF66314` |
| `demo_map.V2RangedCompatibility` | 22 | 0 | 0 | 1 | `DCF8EA08AC5ECD17A8A8381B9D469300E275B1665729B6FDF0084EC050277E59` |
| `demo_map.V3.Attributes` | 4 | 0 | 0 | 1 | `975AA8C767473D9B2032662544E08AB228D24FA0A390577DFE0E52126E5B147F` |

聚焦与修复复测均被最终完整套件包含，不重复计入映射总数。最终改动映射合计
1,740 Success / 0 Fail。

## 8. 回归映射与静态检查

实际 11 个提交路径（9 source + Report + Log）命中 7 条规则，推导 28 个 required groups；
完整 `Shanmen.0_0_10` 加 9 个 legacy 日志覆盖全部要求。覆盖门：
`PASS Changed=11 Rules=7 Required=28 Logs=10`，SHA-256
`F991A5B69A5661D3B405482677A6832820EE5C795609A6701BF759BBA19044B7`。

映射器正反自检 455/455 PASS，SHA-256
`2B7C907F801E411CF76A642C37D32FF02E9D08058117F9826D13D69F94888B2B`。静态检查包含 `git diff --check`、schema 1 / 251 rules、canonical 摘要
重算、唯一抗性定义与 scoped boundary scans，SHA-256
`CB23C2DA1832AAF4F1DF08ACD846F01FB6ECFB822650E70895EF8CF1991C2FBA`。

## 9. 构建、二进制与未执行项

| Build | Native result | SHA-256 |
|---|---|---|
| 首次 Editor，369 actions | Succeeded / 0 | `A51F348F0C7FBDCEF9BA13F253464311C26B110299DE0B76F8B0C6C7F1CCA4B6` |
| 修复后 Editor，7 actions | Succeeded / 0 | `404ACFECB5831691A868DF7CA1F93E1DFD430F42662DFE1E701C1699E6059722` |
| 最终 Game，368 actions | Succeeded / 0 | `A9B37F984277D191E6477B3A05F78F596B1946D171329D304720C1010C98A4EF` |
| 最终 Editor，up to date | Succeeded / 0 | `35FE8BB364D2110CF1C560F8E7908DE11E0483623D6243C9B583AD1FE18C39D8` |

最终二进制：

- `UnrealEditor-demo_map.dll`：19,279,360 bytes，SHA-256
  `E4EFBCDBAA10DB34679E9AF2951EA99955E8CCBC80DB137F510CAC2AC1556A3B`；
- `demo_map.exe`：359,964,160 bytes，SHA-256
  `03AD6FC15357ECEBE81A9F32495FDC87C32AC71F939D535EEDE597F414A3945B`。

未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或
Package。所有证据保存在 `Saved/Codex/P26.3` 且不进入 Git。

## 10. 改动与交接

本阶段精确提交：

1. `Source/demo_map/demo_mapCombatRunCoordinatorTests.cpp`
2. `Source/demo_map/demo_mapItemDefinitions.cpp`
3. `Source/demo_map/demo_mapItemTests.cpp`
4. `Source/demo_map/demo_mapItemTypes.h`
5. `Source/demo_map/demo_mapShanmenArmorResistanceProjectionTests.cpp`
6. `Source/demo_map/demo_mapShanmenItemMigrationTests.cpp`
7. `Source/demo_map/demo_mapShanmenMeridianShockTreatmentAdapterTests.cpp`
8. `Source/demo_map/demo_mapShanmenSwordQiItemAdapterTests.cpp`
9. `Source/demo_map/demo_mapShanmenWeaponGuardItemAdapterTests.cpp`
10. `Docs/Report/Dev.D.UE.0.0.10.P26.3.r0_report.md`
11. `Docs/Log/Dev.D.UE.0.0.10.P26.3.r0_log.md`

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p26-3-tier1-robe-physical-resistance>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p26-3-tier1-robe-physical-resistance/Docs/Report/Dev.D.UE.0.0.10.P26.3.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p26-3-tier1-robe-physical-resistance/Docs/Log/Dev.D.UE.0.0.10.P26.3.r0_log.md>
