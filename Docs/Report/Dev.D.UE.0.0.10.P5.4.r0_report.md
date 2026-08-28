# Dev.D.UE.0.0.10.P5.4.r0 Report

## 1. 结论

P5.4 在本轮 P 阶段边界内完成，结论为 **PASS**。

本轮把 P5.3 的可恢复资源协调第一次接到真实产品内容：新增耐久型护体法袍，敌方 Impact 在结算前预约 1 点耐久，把法袍的 2 点固定减伤从旧聚合层拆成可提交的 canonical defense layer；该层实际触发才扣耐久，先被闪避等更早防御拦截时取消预约。重放不重复磨损，意图形成前中断留下的预约在重启后有界清理，意图形成后继续使用 P5.3 durable saga 恢复。

未启用规划中尚未冻结的护心镜致死拦截，也未为新装备建立第二套库存、生命或伤害权威。

## 2. 产品内容

新增稳定定义 `Prototype.Item.Armor.SpiritGuardRobe`：

- 装备槽：`ArmorSlot`；
- 最大耐久：`20`；
- 固定减伤：`2`；
- 最大堆叠：`1`；
- 奖励池权重：`8`；
- 默认不可购买、可出售，售价 `80`，原型价值 `160`。

Code B 内容身份升级为 `CodeB.Content.0.0.10.P5.4`，digest 为 `32A1BA2A026369525D43CB56C21311C661E59B22BDFA2C877FE93B5C58F637F4`。原 `CodeB.Content.P73.3` 身份继续列为已知历史身份，旧 DefinitionId 不重映射。

## 3. 权威迁移与 Run 接入

产品定义新增显式 `MaxDurability`／`MaxCharges`，默认均为零，因此 38 个既有物品行为不变。Code B → Shanmen migration 与 Run acquired item adapter 现在：

1. 映射资源上限；
2. 只在上限大于零时授予对应 capability tag；
3. 迁移时把耐久／次数初始化到定义上限；
4. 缺少目标 authority definition 时失败关闭。

护体法袍仍通过现有装备效果解析器贡献 `FlatDamageReduction`，资源适配器只在命中结算时把它从聚合值中精确拆出，不旁路装备属性系统。

## 4. Impact 资源闭环

`PrepareImpactDefense` 固定执行：

1. 恢复 P5.3 pending intent；
2. 清理同一 Run 中可识别的意图前孤儿预约；
3. 验证 ActiveRun、owner、部署槽、法袍实例和 durability capability；
4. 从旧 `Combat.Defense.Player.FlatDamageReduction.r1` 层减去法袍精确贡献；
5. 以 Impact／Run／item 派生的稳定 RequestId 预约 1 点耐久；
6. 追加 `Combat.Defense.Player.SpiritGuardRobe.Durability.r1` 层，绑定 ReservationId 与 ItemInstanceId，并标记 `bRequiresCommitOnTrigger`。

进入 P5.3 coordination 后，Items intent 与 vitality CAS 共同决定终态；进入前若 vitality snapshot 或 canonical validation 失败，则使用稳定 cancel request 取消临时预约。装备了护体法袍但资源 authority 不可检查时，敌方攻击失败关闭，不允许无耐久权威地继续享受减伤。

## 5. 故障恢复与幂等

- 临时预约 Purpose 使用严格 `SMDR1_<ImpactGuid>`，只清理当前 owner／Run／prepared equipment 的 durability reservation；
- 清理按 ReservationId 排序并使用稳定 cancel RequestId，重复恢复不产生第二次状态变化；
- exact reserve replay 重用原 ReservationId；已进入同一 Impact durable intent 的终态预约可重放，已在意图前取消的预约不会重新获得减伤；
- 可用耐久计算会扣除全部 active reservation，防止并发超卖；
- 资源不足只禁用本次法袍层，不修改其它防御层或生命权威。

## 6. 产品级证明

新增三条真实 Profile → Code B warehouse → Shanmen cutover → prepared Run 测试：

- `SpiritGuardTriggeredCommit`：4 点原始伤害中法袍阻止 2 点，生命扣 2，耐久 `20 → 19`，exact replay 仍为 19；
- `SpiritGuardUntriggeredCancel`：更早的 PreventAll 层先触发，法袍预约取消，生命和耐久均不变；
- `SpiritGuardPreIntentRestart`：预约后、intent 前中断，重启绑定清理孤儿一次，耐久保持 20。

P5.3 的 `RecoverableIntent` 同组保留，证明 intent 后故障仍由既有 saga 恢复。

## 7. 自动化证据

| 日志 | Group | Success | Fail | SHA-256 |
|---|---|---:|---:|---|
| `p54_defense_adapter.log` | `Shanmen.0_0_10.Items.DefenseResourceAdapter` | 4 | 0 | `4B272D6CC157CE53613846F6C142CE35352B1FD60089A4E06CD2EEA2A30C2681` |
| `p54_shanmen_full.log` | `Shanmen.0_0_10` | 124 | 0 | `D0BF94F8FA99058F65E6F6DC72C41E55E78E53E55273739A7CD4E82680B39076` |
| `p54_legacy_changed_paths_final.log` | `ItemEconomySchema + V3.Items + V3.WorldInteraction + RewardGeneration` | 68 | 0 | `9FEE921916F42E2FC6440966C16E4F1A415B6152B88A50AC2BF2E54FF8EDDBDA` |

最终 changed-file 回归合计 `192 Success / 0 Fail`；focused 4 条已包含在 124 条全量中，不重复计数。三次最终进程原生退出码均为 `0`。

UE 5.8 启动阶段在 Engine 初始化前固定输出 13 条内建 `LogAutomationTest: Error: Condition failed` 自检噪声；最终判定使用 Automation Controller 的逐用例 Result、queue empty 与进程退出码，四份日志均无 fatal。

## 8. 首次失败与修复

首次 legacy changed-path 回归为 `67 Success / 1 Fail`，进程原生退出码 `0`，但测试结果明确失败，因此该轮不计通过。保留日志：

- `Saved/Automation/Dev.D.UE.0.0.10.P5.4.r0/p54_legacy_changed_paths.log`；
- SHA-256：`16350DDEA56CB7FFA8B852BB7BD49BD34F26930414FC7670F717B976F763E655`。

失败测试为 `demo_map.V3.Items.ModifierBridgeAndRebind`。旧夹具把 `WindTalisman` 装入 `AccessorySlot`，但冻结定义只允许 `SpatialRingSlot`，导致后续七条派生断言连锁失败。夹具改用真实槽位后 68/68 通过；没有放宽产品装备规则。

## 9. Changed-file 回归门禁

14 个 Source 改动文件推导出：

- coordinator／resource adapter／migration → `Shanmen.0_0_10`；
- item definitions／types／item tests → `demo_map.ItemEconomySchema`、`demo_map.V3.Items`；
- generated reward pool → `demo_map.RewardGeneration`；
- registry count consumer → `demo_map.V3.WorldInteraction`。

要求组全部出现在最终健康日志中。该门禁正是发现旧 WindTalisman 夹具错误的原因。

## 10. 构建与静态检查

Editor Development，`-WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`：

- 完整编译 `149/149`，Succeeded，原生退出码 `0`；
- 后续增量 `5/5`、`25/25`、`4/4`，均 Succeeded，退出码 `0`。

Game Development 使用同一单并发策略：

- 完整编译 `144/144`，Succeeded，原生退出码 `0`，`403.75s`；
- 输出 `Binaries/Win64/demo_map.exe`，未启动。

静态结果：

- `git diff --check`：退出码 `0`；
- ShanmenCombatCore／ShanmenItems 对 `UWorld`、`AActor`、`ApplyDamage`、运行时 RNG：`0` 代码匹配；
- 唯一 `demo_map` 命中为 ShanmenItems 公共头中的产品边界说明注释；
- 资源防御仍由 demo_map adapter 单向调用纯 CombatCore 与 ShanmenItems。

## 11. 修改范围与兼容性

修改 14 个生产／测试 Source 文件，新增约 1,392 行、删除约 31 行；另新增本 Report 与 Development Log。没有改动 ShanmenItems repository 格式、CombatCore 数学、旧装备 DefinitionId 或现有 schema 编号。

旧装备资源上限默认为零，旧存档内容身份保持可识别。长期未跟踪的历史 Prompt、Report、PDF、自动化交接文档和用户文件均不纳入提交。

## 12. P/F 边界

本 Report 只包含 P 阶段源码、静态审查、无头 `-NullRHI` Automation、Editor Development 与 Game Development build。

未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook、Package 或大规模产品回归。

## 13. GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p5-4-durable-armor-defense/Docs/Report/Dev.D.UE.0.0.10.P5.4.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p5-4-durable-armor-defense/Docs/Log/Dev.D.UE.0.0.10.P5.4.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p5-4-durable-armor-defense>
