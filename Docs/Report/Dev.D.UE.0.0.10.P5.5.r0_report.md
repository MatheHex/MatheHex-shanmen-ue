# Dev.D.UE.0.0.10.P5.5.r0 Report

## 1. 结论

P5.5 在本轮 P 阶段边界内完成，结论为 **PASS**。

本轮关闭了 P5.4 护体法袍“会磨损、但战备界面看不到，且没有跨多局产品证明”的缺口：准备投影现在从唯一 ShanmenItems 权威读取当前/最大耐久与充能，Presenter 和战备 UI 显示稳定资源标签；真实生命周期证明覆盖首次磨损、撤离保留、进程重启、下一局继续磨损，以及死亡销毁后的零资源 tombstone。

没有新增护心镜、正式元素抗性、修理规则或其它尚未冻结的战斗内容。

## 2. 权威资源投影

`Fdemo_mapProfilePreparationStashRow` 新增：

- `Durability` / `MaxDurability`；
- `Charges` / `MaxCharges`；
- `HasValidResourceState()` 范围不变量。

`Fdemo_mapShanmenPreparationAdapter::BuildProjection` 的当前值只来自 `FShanmenItemInstance`，上限只来自同一份 `FShanmenItemAuthoritySnapshot.Definitions`。缺 Definition 或资源越界时投影失败关闭，不回退到旧 Profile 或旧 demo_map 定义作为资源真值。

旧 Profile 兼容路径保持默认 `0/0`，仅用于 cutover 前展示，不建立第二套耐久/充能权威。

## 3. Presenter 与战备 UI

`Fdemo_mapProfilePreparationRowView` 携带同一组资源值，并生成稳定标签：

```text
DUR 19/20
CHG 2/3
DUR 19/20 | CHG 2/3
```

资源标签为空时列表不增加噪声，详情显示 `Resources: NONE`；有资源时列表和详情都显示精确当前/最大值。UI 没有写入资源的入口，仍是纯只读投影。

## 4. 多局耐久生命周期

新增产品级测试 `SpiritGuardDurableLifecycle`，使用真实 Profile → Code B warehouse → Shanmen cutover → prepared Run 链路：

1. 第一局法袍触发，耐久 `20 → 19`；
2. Extraction 后同一 ItemInstanceId 回到 Stored，耐久保持 19；
3. 战备投影与 Presenter 显示 `DUR 19/20`；
4. 进程重启并重新绑定，仍为 19；
5. 下一局复用同一身份，再次触发后 `19 → 18`；
6. Death 后实例变为 Destroyed，Quantity/Durability/Charges 均为 0；
7. tombstone 重启后仍存在，但不再进入战备投影。

测试不直接注入 repository 内部状态，也不通过旧 Profile 写资源。

## 5. Changed-file 回归门禁补强

探索回归发现原映射把 `demo_mapProfilePreparationTypes/Presenter/Widget` 只映射到 Profile/schema，漏掉直接消费者：

- `demo_map.GridInventory`；
- `demo_map.P2.EntityLoadout`。

`Scripts/ShanmenRegressionMap.json` 已增加 `PreparationProjectionConsumers` 规则。最终门禁自动核验结果：

```text
PASS Changed=9 Rules=4 Required=8 Logs=7
```

门禁脚本自测 `8/8` 通过。

## 6. 过期测试契约修正

新门禁首次运行暴露两个直接消费者仍冻结在旧产品契约：

- 背包空间仍断言 `10/14`，而当前 Definition 自校验已冻结为 `36/36`；
- 总携带仍断言 `16/20`，当前为基础 6 + 背包 36 = `42`；
- 装备展示仍断言四角色，当前为 Weapon/Armor/Accessory/SpatialRing/Backpack 五角色；
- WindTalisman 仍被放入旧 AccessorySlot，而当前只兼容 SpatialRingSlot；
- Profile schema 仍硬断言 3，当前 authority-cutover schema 为 7。

只更新测试名称、夹具与期望值以匹配已经存在且由 `ValidateRegistry` 自校验的生产契约；未修改任何容量、槽位、schema 或物品生产值。两组从 `0/4`、`14/27` 恢复为 `4/4`、`27/27`。

## 7. 最终自动化证据

| 日志 | Group | Success | Fail | SHA-256 |
|---|---|---:|---:|---|
| `p55_shanmen_full_final.log` | `Shanmen.0_0_10` | 125 | 0 | `9C51D4B8BCF8C611EDBE6257EE187F6C7EBC40B6AF1EF66A0EFF2920BEE0E79D` |
| `p55_profile_final.log` | `demo_map.Profile` | 211 | 0 | `000C74435BAAFB7D1874DDB5FE20395B04E7C8CC518A1CD46783DD6B9B1604DB` |
| `p55_item_economy_schema_final.log` | `demo_map.ItemEconomySchema` | 23 | 0 | `583A7E1512160626FE87120B75AF92611E31E09F89BB165585E1361D1C92A4A5` |
| `p55_item_use_armor_final.log` | `demo_map.ItemUseAndArmor` | 46 | 0 | `24771AE83CA3EC40E87820001A6791B4CDD7E240D6551BC17FC26812241094C9` |
| `p55_hotbar_final.log` | `demo_map.P4.Hotbar` | 7 | 0 | `68C424ED6C04BD4CCB08F274C9D484FF82988484EDF40317724A795A22E4EDAE` |
| `p55_entity_loadout_final.log` | `demo_map.P2.EntityLoadout` | 4 | 0 | `2C3982FCEF36515E23AA2CC56FCCDD18D6B65A5EC63BD20F054355AED7CD9D36` |
| `p55_grid_inventory_final.log` | `demo_map.GridInventory` | 27 | 0 | `F0A6590E66F93DC4737E4CFC5BD0C5548C8526858D422D8959867EEB9BCD5024` |

最终唯一用例合计 `443 Success / 0 Fail`，全部进程原生退出码为 `0`。聚焦 `DefenseResourceAdapter` 5/5 已包含在 `Shanmen.0_0_10` 的 125 条中，不重复计数。

## 8. 首次失败与剩余基线债

探索性 `Automation RunTests demo_map` 父组日志为 `1212 Success / 115 Fail`，SHA-256：

```text
342ED20218F5E4B3AA95CF21ECE287AD97F55772C872E3D6109DBEA730EED40A
```

其中直接相关的 EntityLoadout/GridInventory 17 条过期断言已修复。父组还包含其它历史 reward/runtime/layout 测试的过期契约和跨组静态状态污染；该父组不作为本轮健康证据，也不伪报全绿。后续应按模块逐组修复隔离与契约，而不是用一个污染进程替代 changed-file 门禁。

## 9. 构建与静态检查

Editor Development：

- 首次完整受影响构建 `53/53`，Succeeded，原生退出码 `0`，`196.38s`；
- 测试契约修正后增量 `5/5`，Succeeded，原生退出码 `0`，`16.18s`。

Game Development：

- 首次受影响构建 `52/52`，Succeeded，原生退出码 `0`，`176.74s`；
- 最终增量 `4/4`，Succeeded，原生退出码 `0`，`22.65s`；
- 生成 `Binaries/Win64/demo_map.exe`，未启动。

静态结果：

- `git diff --check`：退出码 `0`；
- ShanmenCombatCore/ShanmenItems 无 `UWorld`、`AActor`、`ApplyDamage` 或运行时 RNG 命中；
- 唯一 `demo_map` 命中仍是 ShanmenItems 公共头中的产品边界说明注释；
- 本轮未修改纯内核模块或持久化格式。

## 10. 修改范围与兼容性

修改 8 个 Source 文件与 1 个回归映射文件，约新增 309 行、删除 34 行；另新增本 Report 与 Development Log。

兼容性：

- 不改 ItemInstanceId、DefinitionId、authority document 或 Profile schema；
- 0 上限资源继续显示为空，不影响既有物品；
- 当前资源只读，不提供 UI 修复/补充/编辑入口；
- Extraction 保留值、Death/Abandon 销毁值沿用既有 ShanmenItems 终态语义；
- 长期未跟踪的历史 Prompt、Report、PDF 与用户文档均未纳入提交。

## 11. P/F 边界

本 Report 只包含 P 阶段源码、静态审查、无头 `-NullRHI` Automation、Editor/Game Development build。

未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook、Package 或大规模产品回归。

## 12. GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p5-5-durable-equipment-lifecycle/Docs/Report/Dev.D.UE.0.0.10.P5.5.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p5-5-durable-equipment-lifecycle/Docs/Log/Dev.D.UE.0.0.10.P5.5.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p5-5-durable-equipment-lifecycle>
