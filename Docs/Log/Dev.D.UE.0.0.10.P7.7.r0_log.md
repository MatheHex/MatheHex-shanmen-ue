# Dev.D.UE.0.0.10.P7.7.r0 Development Log

## 身份

- 阶段：`Dev.D.UE.0.0.10.P7.7.r0`；
- 基线提交：`6b367392e32cdb8366eecfc2f6ca694579c7bfc0`（P7.6）；
- 分支：`agent/0.0.10-p7-7-thrown-weapon-content`；
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`；
- 引擎：Unreal Engine `5.8`；
- 收口日期：`2026-08-29`。

## 目标与阶段决策

P7.6 已具备 hotbar intent 到 deterministic thrown-weapon action 的 product session。静态追踪目录、Code B 和 Shanmen authority 后确认：生产目录中没有任何 definition 显式拥有 `Item.Weapon.Thrown`。若直接按原建议接 GameMode/source Actor 生命周期，所有真实物品都会在 P7.1 item adapter 的 tag gate 失败。

因此 P7.7 先关闭内容缺口：建立一件真实商品、稳定 semantic、内容身份、旧/新 authority 投影与 changed-file regression gate。生命周期 wiring 顺延至 P7.8。

冻结边界：

1. 不从 ID、名称、Category 或价格推断暗器能力；
2. 不新增 `bIsThrownWeapon` 等独立布尔字段；
3. 不建立第二套目录、库存、热栏或 GameplayTag authority；
4. 不修改 save schema 或既有 item identity；
5. 不接 GameMode、输入、UI、动画或 Content 资产。

## 实现

### Product semantic 与目录

`demo_mapItemTypes.h` 新增 `Edemo_mapItemGameplaySemantic`，definition 以 `TArray` 保存 typed semantic，并通过 `HasGameplaySemantic` 做 exact 查询。

`demo_mapItemDefinitions.*` 新增 `TrainingThrowingKnife`：

```text
DefinitionId = Prototype.Item.Consumable.TrainingThrowingKnife
Category     = Prototype.ItemCategory.Consumable
Level        = 1
MaxStack     = 20
Buy/Sell     = 30/15
Value        = 15
Semantic     = ThrownWeapon
```

目录由 39 增至 40。Validate 检查 semantic 非 None、无重复，并要求恰有一个 canonical thrown weapon；其结构必须为 stackable/hotbar consumable，不得含装备、耐久或 charge 字段。

### 内容身份

Current version 由 `CodeB.Content.0.0.10.P5.4` 升级为 `CodeB.Content.0.0.10.P7.7`，digest 为：

```text
6C30E84A05386A7986A2344DB8247961E75F0FE2179F41927A0C7DF950F45A00
```

该值已从 Report 中记录的 UTF-8 canonical string 独立重算。P5.4 version/digest 加入 known historical identity；没有用内容 digest 充当存档 migration key。

### Authority adapter

- `demo_mapShanmenItemMigration.cpp`：typed semantic 投影 `FShanmenItemNativeTags::ItemWeaponThrown()`；
- `demo_mapShanmenRunLifecycleAdapter.cpp`：Run acquired-item definition 做同一投影；
- Code B 沿用现有 definition projection，得到 stackable + quick-usable；
- migration fixture 新增 exact item GUID、DefinitionId 和 Quantity `3`，验证 legacy Profile → Code B → Shanmen candidate 的一致性。

### 冻结契约更新

目录数量、ordered IDs、purchasable IDs/prices、stackable count 与 F-stage presentation count 均从实际新增项增量更新；未改动旧项的 identity、排序或价格。

## 新增自动化

`Shanmen.0_0_10.Product.ThrownWeaponContent.CanonicalAuthorityProjection` 覆盖：

1. 商品字段、热栏资格与 typed semantic；
2. P7.7 current content identity；
3. P5.4 historical identity；
4. Code B MaxStack/quick-use projection；
5. Shanmen consume-quantity / thrown-weapon tags；
6. exact instance identity 与 Quantity `3`。

## 首次失败与修复

### Full suite

首次结果 `190 Success / 11 Fail`，进程 native exit `0`，但测试结果按失败处理。新增 definition 已进入 `GetAll()`，而 `Validate()` 内第二份 fixed purchasable list 仍只含旧 13 项，导致目录验证失败并向所有依赖 catalog invariant 的测试级联。

修复：在 canonical purchasable list 末尾追加 `TrainingThrowingKnife`，并把错误文本从历史 P1 语义改为 current content catalog。没有放宽排序或价格检查。

- 日志：`P7.7_Full_FirstFailure.log`；
- SHA-256：`F806D3301210038D66801BDC9EA4306850C0B4EE21540F47AF8FC1395CD3CFBF`。

### Profile suite

首次结果 `210 Success / 1 Fail`。`demo_mapProfileTradeTests.cpp` 的 `CatalogExactImmutableOrderAndPrices` 仍冻结旧 13 项及价格。

修复：保持前 13 项完全不变，在末尾追加练习飞刀和 buy price `30`，并保留 exact ordered comparison。

- 日志：`P7.7_Profile_FirstFailure.log`；
- SHA-256：`4B9A7B3260CBF787A6A663902FE20D34514449535DF5E4A9FDA21CB3FF7F7F27`。

## 最终自动化

统一命令：

```powershell
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests <group>" -TestExit="Automation Test Queue Empty" -AbsLog=<log>
```

| 日志 | Group | Success | Fail | Native exit | SHA-256 |
|---|---|---:|---:|---:|---|
| `P7.7_Targeted.log` | `Shanmen.0_0_10.Product.ThrownWeaponContent` | 1 | 0 | 0 | `6D2FE03EC5638471182038C322ADDEF8F5D3654F2B9E7FD957D624678EC695FA` |
| `P7.7_Full.log` | `Shanmen.0_0_10` | 201 | 0 | 0 | `E27B4D5EDEF8601A13A0146C77401F4BE3202D0BC037B886014B21223D5DC06D` |
| `P7.7_ItemEconomySchema.log` | `demo_map.ItemEconomySchema` | 23 | 0 | 0 | `3FD51217AD332A855A144E856EE3CA0F7BC08AEDC66CD98675A040A85801BE31` |
| `P7.7_Profile.log` | `demo_map.Profile` | 211 | 0 | 0 | `670546E9B46AFF6C11474721E1DF2AA983C42182D6BA1467618136315FE11EAC` |
| `P7.7_CodeB.log` | `demo_map.CodeB` | 60 | 0 | 0 | `5A0CCF48B19E55651CE1876E50669843DCD6CFA44A1F21BE1DE38A3144DA90AC` |
| `P7.7_ItemUseAndArmor.log` | `demo_map.ItemUseAndArmor` | 46 | 0 | 0 | `90D2EDBD2B9530D44B01EFA5FE312CB4DE396834C3056248FF128A45DCFC2B43` |
| `P7.7_Hotbar.log` | `demo_map.P4.Hotbar` | 7 | 0 | 0 | `EF7F638FAC2B2E83E77C88A1430D8AB84CFC5453C53188E1B2864E1202827476` |
| `P7.7_V2RangedCompatibility.log` | `demo_map.V2RangedCompatibility` | 22 | 0 | 0 | `8BB1EADA8F99768B2CD154A011793E3FBC9F1F337FC07B1285D6C9802C28A60B` |
| `P7.7_WorldInteraction.log` | `demo_map.V3.WorldInteraction` | 4 | 0 | 0 | `B99C25C03DEDED96FBECE295068CEE04DEDAE37C5718BBBC98E4ED8232B7FA9D` |

Canonical 日志均有一个实际 RunTests、queue-empty、Fail `0`，fatal/unhandled/ensure marker 为 `0`。

## Changed-file regression gate

新增 `CanonicalItemCatalog` 规则：任何 canonical item definition/type/test 改动必须覆盖新 Items suite 和所有直接 legacy consumer。新增 `WorldInteractionContract` 规则关闭此前测试文件自身没有路径映射的缺口。

```text
REGRESSION_MAP_JSON: PASS Rules=33
SELF_TEST: PASS 26/26
REGRESSION_COVERAGE: PASS Changed=13 Rules=5 Required=9 Logs=8
```

反向 self-test 证明只有 `Shanmen.0_0_10` 新模块证据、缺少 legacy consumer 证据时必须失败关闭。

## 构建时间线

统一使用：

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

| 构建 | Result | Native exit | 时间 | 日志 SHA-256 |
|---|---|---:|---:|---|
| Editor first reflected integration | Succeeded | 0 | 428.44s | `02351515CD114A8030F56E2A587E2B17B35E48613885A6B7D3B40273A261BD77` |
| Editor final incremental | Succeeded | 0 | 6.84s | `C872DE3AE2E894BE81EFDB5A1A4CE3C61434A1F341821608632BCE15B3B78E87` |
| Game final reflected integration | Succeeded | 0 | 388.09s | `6E47638DFC7593FD645635761402A2C070EA145D45F057139FAE3B6F89443D9D` |

新增 UENUM 使 Editor 与 Game 首次各自执行完整 UHT/non-unity integration；两者均一次成功，不是内存、环境或源码失败。

- Editor DLL：`10677248` bytes，UTC `2026-08-29T13:19:54Z`；
- Game executable：`351867904` bytes，UTC `2026-08-29T13:29:47Z`。

## 静态、范围与兼容性

- content canonical SHA-256 重算：PASS；
- regression map JSON parse：PASS；
- boundary scan：新增行无 `ApplyDamage`、RNG、`AActor*`、`UWorld*`；
- working tree 与 staged `git diff --check`：native exit `0`；
- 未改 schema、Build.cs、GameplayTags 配置、Content、GameMode 或既有 authority；
- 长期未跟踪的 0.0.9B 与用户文件保持未跟踪且未 stage。

## P/F 边界与下一阶段

只执行 P 阶段源码、静态检查、无头 Automation、Editor/Game Development build。未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未做真实输入、截图、Smoke、Cook 或 Package。

P7.8 可在真实商品已能通过 tag gate 后建立 source Actor / GameMode Run 生命周期适配器，负责 begin/end P7.6 session 与产品选择事件转换。仍应保持设备无关，不在 Session 内加入 Tick/polling，不混入键位、Widget、动画或视觉资源。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p7-7-thrown-weapon-content/Docs/Report/Dev.D.UE.0.0.10.P7.7.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p7-7-thrown-weapon-content/Docs/Log/Dev.D.UE.0.0.10.P7.7.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p7-7-thrown-weapon-content>
