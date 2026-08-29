# Dev.D.UE.0.0.10.P7.7.r0 Report

## 1. 结论

P7.7 已完成第一件真实可投放暗器商品及其既有权威投影，结论为 **PASS**。

P7.6 已具备 active-Run product session，但生产目录中没有任何定义能显式取得 `Shanmen.Item.Weapon.Thrown`，因此直接接 GameMode/输入只会让所有真实物品失败关闭。本轮先补齐内容身份：新增唯一的练习飞刀，使用可扩展 typed semantic 明示暗器能力，并让既有 Code B、0.0.10 item authority、迁移与 Run acquired-item 路径得到一致投影。

GameMode、输入键位、UI、动画、视觉资产、弧线、追踪、转向和召回仍未接入。

## 2. 真实商品定义

新增 canonical definition：

- DefinitionId：`Prototype.Item.Consumable.TrainingThrowingKnife`；
- 名称：`练习飞刀` / `TRAINING THROWING KNIFE`；
- Category：`Prototype.ItemCategory.Consumable`；
- Level：`1`，MaxStack：`20`；
- Buy / Sell / PrototypeValue：`30 / 15 / 15`；
- 可购买、可出售、可进入热栏；
- 无治疗参数、装备槽、兼容槽、耐久或充能。

目录定义数从 `39` 增至 `40`，可购买目录在三阶丹药后追加练习飞刀。既有物品 ID、顺序和价格不变。

## 3. Typed semantic 契约

新增 `Edemo_mapItemGameplaySemantic` 与 `Fdemo_mapItemDefinition::GameplaySemantics`。首个 semantic 为 `ThrownWeapon`。

该结构是有类型的可扩展集合，而不是新增 `bIsThrownWeapon` 一类 bool soup。Category、DefinitionId、显示名称和价格都不得被推断为玩法能力；只有 definition 显式声明 semantic，authority adapter 才能投影相应 GameplayTag。

目录验证拒绝 `None`、重复 semantic，并要求当前内容中恰好存在一个 canonical thrown-weapon definition。该定义必须是可堆叠、可进热栏的 consumable，且不得携带装备槽、耐久或充能字段。

## 4. 内容身份与兼容性

当前内容身份升级为：

- Version：`CodeB.Content.0.0.10.P7.7`；
- Digest：`6C30E84A05386A7986A2344DB8247961E75F0FE2179F41927A0C7DF950F45A00`。

Digest 由以下 UTF-8 canonical string 的 SHA-256 重新计算并精确匹配：

```text
CodeB.Content.0.0.10.P7.7|Parent=32A1BA2A026369525D43CB56C21311C661E59B22BDFA2C877FE93B5C58F637F4|Add=Prototype.Item.Consumable.TrainingThrowingKnife|Category=Prototype.ItemCategory.Consumable|Level=1|MaxStack=20|Buy=30|Sell=15|Value=15|Semantic=ThrownWeapon
```

P5.4 identity `32A1BA2A...F637F4` 被保留为 known historical evidence；没有修改 save schema，也没有 remap 既有 DefinitionId。

## 5. 权威投影

现有 Code B 投影把练习飞刀解析为 MaxStack `20` 的 quick-usable stack。0.0.10 两条入口都只读取 typed semantic：

1. Profile migration 为 canonical definition 添加 `Capability.ConsumeQuantity` 与 `Item.Weapon.Thrown`；
2. active Run acquired-item projection 同样添加 `Item.Weapon.Thrown`。

新增测试从 schema-6 风格 Profile 中迁移数量 `3` 的练习飞刀，证明 Code B definition、Shanmen definition、exact instance identity 与 Quantity 一致。没有第二套物品目录、库存或标签推断路径。

## 6. 自动化证据

最终 `-Unattended -NullRHI` 自动化全部通过。每份 canonical 日志只有一个实际 RunTests、至少一个 queue-empty、Fail `0`，进程原生退出码均为 `0`。

| Group / 日志 | Success | Fail | SHA-256 |
|---|---:|---:|---|
| `ThrownWeaponContent` / `P7.7_Targeted.log` | 1 | 0 | `6D2FE03EC5638471182038C322ADDEF8F5D3654F2B9E7FD957D624678EC695FA` |
| `Shanmen.0_0_10` / `P7.7_Full.log` | 201 | 0 | `E27B4D5EDEF8601A13A0146C77401F4BE3202D0BC037B886014B21223D5DC06D` |
| `demo_map.ItemEconomySchema` / `P7.7_ItemEconomySchema.log` | 23 | 0 | `3FD51217AD332A855A144E856EE3CA0F7BC08AEDC66CD98675A040A85801BE31` |
| `demo_map.Profile` / `P7.7_Profile.log` | 211 | 0 | `670546E9B46AFF6C11474721E1DF2AA983C42182D6BA1467618136315FE11EAC` |
| `demo_map.CodeB` / `P7.7_CodeB.log` | 60 | 0 | `5A0CCF48B19E55651CE1876E50669843DCD6CFA44A1F21BE1DE38A3144DA90AC` |
| `demo_map.ItemUseAndArmor` / `P7.7_ItemUseAndArmor.log` | 46 | 0 | `90D2EDBD2B9530D44B01EFA5FE312CB4DE396834C3056248FF128A45DCFC2B43` |
| `demo_map.P4.Hotbar` / `P7.7_Hotbar.log` | 7 | 0 | `EF7F638FAC2B2E83E77C88A1430D8AB84CFC5453C53188E1B2864E1202827476` |
| `demo_map.V2RangedCompatibility` / `P7.7_V2RangedCompatibility.log` | 22 | 0 | `8BB1EADA8F99768B2CD154A011793E3FBC9F1F337FC07B1285D6C9802C28A60B` |
| `demo_map.V3.WorldInteraction` / `P7.7_WorldInteraction.log` | 4 | 0 | `B99C25C03DEDED96FBECE295068CEE04DEDAE37C5718BBBC98E4ED8232B7FA9D` |

完整 suite 从 P7.6 的 `200` 增至 `201`。

## 7. 首次失败与修复

首次 full suite 为 `190 Success / 11 Fail`。根因是目录主验证器中的固定可购买列表仍为旧 13 项，遗漏练习飞刀，导致 catalog invariant 失败并向依赖测试级联。补齐唯一 canonical 列表后 full suite 为 `201/201`。失败日志 `P7.7_Full_FirstFailure.log` 的 SHA-256 为 `F806D3301210038D66801BDC9EA4306850C0B4EE21540F47AF8FC1395CD3CFBF`。

随后独立 Profile 回归首次为 `210 Success / 1 Fail`。`ProfileTrade.01` 仍冻结旧目录与价格数组；在保持前 13 项不变的前提下追加练习飞刀与价格 `30`，最终为 `211/211`。失败日志 `P7.7_Profile_FirstFailure.log` 的 SHA-256 为 `4B9A7B3260CBF787A6A663902FE20D34514449535DF5E4A9FDA21CB3FF7F7F27`。

两次失败均如实保留；未通过放宽断言、跳过 legacy 测试或重写权威来获得成功。

## 8. 改动—回归与静态门禁

- 新增 `CanonicalItemCatalog` 映射，目录/类型改动强制要求 Items、ItemEconomySchema、Profile、CodeB、ItemUseAndArmor、Hotbar 与 WorldInteraction；
- 新增 `WorldInteractionContract` 映射；
- `REGRESSION_COVERAGE: PASS Changed=13 Rules=5 Required=9 Logs=8`；
- regression map JSON：`33` rules，parse PASS；mapping self-test：`26/26 PASS`；
- boundary scan：新增行无 `ApplyDamage`、RNG、`AActor*` 或 `UWorld*`；
- 工作区与最终 staged `git diff --check`：native exit `0`。

## 9. 构建与 P/F 边界

统一命令：

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

| 构建 | Result | Native exit | 时间 | SHA-256 |
|---|---|---:|---:|---|
| Editor reflected integration | Succeeded | 0 | 428.44s | `02351515CD114A8030F56E2A587E2B17B35E48613885A6B7D3B40273A261BD77` |
| Editor final incremental | Succeeded | 0 | 6.84s | `C872DE3AE2E894BE81EFDB5A1A4CE3C61434A1F341821608632BCE15B3B78E87` |
| Game final reflected integration | Succeeded | 0 | 388.09s | `6E47638DFC7593FD645635761402A2C070EA145D45F057139FAE3B6F89443D9D` |

- Editor DLL UTC：`2026-08-29T13:19:54Z`；
- Game executable UTC：`2026-08-29T13:29:47Z`。

本轮只执行 P 阶段源码、静态检查、无头 Automation、Editor/Game Development build。未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。

## 10. 下一阶段与 GitHub

P7.8 建议在真实暗器内容已成立后接入 source Actor / GameMode Run 生命周期：创建、绑定和关闭 P7.6 session，并把已有产品选择事件转换为 device-independent intent。仍不在 Session 内新增 Tick/polling，也不同时混入具体键位、Widget、动画或视觉表现。

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p7-7-thrown-weapon-content/Docs/Report/Dev.D.UE.0.0.10.P7.7.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p7-7-thrown-weapon-content/Docs/Log/Dev.D.UE.0.0.10.P7.7.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p7-7-thrown-weapon-content>
