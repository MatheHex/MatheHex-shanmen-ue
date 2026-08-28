# Dev.D.UE.0.0.10.P5.4.r0 Development Log

## 身份

- 阶段：`Dev.D.UE.0.0.10.P5.4.r0`；
- 基线提交：`d260af3794be5af75abc785d57b94274aa745c03`（P5.3）；
- 分支：`agent/0.0.10-p5-4-durable-armor-defense`；
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`；
- 引擎：Unreal Engine `5.8`；
- 收口日期：`2026-08-28`。

## 目标

把 P5.3 的可恢复资源协调落到第一件真实产品防具：护体法袍在自己的减伤层实际触发时消耗 1 点耐久；未触发、重放或进程中断均不能多扣、漏扣或永久锁定资源。

## 内容契约

`Fdemo_mapItemDefinition` 新增默认零的 `MaxDurability` 与 `MaxCharges`。`SpiritGuardRobe` 是唯一首发非零耐久定义：ArmorSlot、20 耐久、2 固定减伤、单格单件。奖励池增加稳定条目 `P5.4.Pool.Robe.SpiritGuard`。

内容版本从 P73.3 升到 `CodeB.Content.0.0.10.P5.4`，新 digest 为 `32A1BA2A026369525D43CB56C21311C661E59B22BDFA2C877FE93B5C58F637F4`；P73.3 继续作为 known identity 接受。

## Authority 适配

Migration 和 Run lifecycle adapter 将资源上限与 capability tag 映射到 ShanmenItems。旧定义的上限为零，不支持 durability／charges；法袍迁移实例初始耐久为 20。迁移中的每个 normalized item 必须找到目标 definition，否则返回 `SourceItemMismatch`，不发布候选状态。

## 临时预约协议

P5.4 临时 Purpose：

```text
SMDR1_<ImpactGuid Digits>
```

Reserve RequestId namespace：

```text
Shanmen.Product.SpiritGuardRobe.DurabilityReserve.r1
```

Cancel RequestId namespace：

```text
Shanmen.Product.SpiritGuardRobe.DurabilityCancel.r1
```

Reserve identity 由 owner、scope、ActiveRun、ImpactId、ItemInstanceId 派生。重复 reserve 返回原 receipt；取消只处理当前 correlation 内、prepared equipment 上、Reserved 状态且 purpose 可严格解析的 durability reservation。

## Defense snapshot 改写

产品 PlayerHealth 先按既有装备效果产生聚合 `FlatDamageReduction` 层。Adapter 验证聚合层唯一、Armor tag、AbsorbPoints、Resistance order、非资源型且幅度足够，然后减去法袍 2 点贡献。

成功预约后追加：

- RuleId：`Combat.Defense.Player.SpiritGuardRobe.Durability.r1`；
- LayerId：ReservationId；
- SourceInstanceId：法袍 ItemInstanceId；
- Operation：`AbsorbPoints`；
- Order：`Resistance + 1`；
- Magnitude：`2`；
- Tags：`Defense.Armor`；
- Required target：`Target.Living`；
- commit-on-trigger：true。

如果资源不足，法袍贡献保持从聚合层移除，本次攻击继续使用其它合法防御；如果 authority 或结构不一致，则攻击失败关闭。

## 产品协调

Enemy-to-player Impact 的顺序改为：ImpactId → base defense → P5.4 resource prepare → vitality snapshot → pure resolve → P5.3 delivery。

资源恢复可能先提交上一条 vitality intent，因此 vitality snapshot 必须在资源 prepare 后采样，避免携带 stale CAS。若在 delivery 前失败，临时预约 durable cancel；delivery 开始后完全交给 P5.3 prepare/vitality/finalize saga，禁止双重取消。

TryBeginRun 在发布 Combat binding 前恢复 pending intent、清理 pre-intent orphan，并记录 prepared armor 是否要求资源 authority。TryEndRun 和 Reset 清除此状态。

## 产品级测试

Fixture 使用真实 `ProfileRepository`、Code B warehouse、Shanmen cutover、prepared Run 和 deployed authority armor，不直接注入伪 repository 状态。

新增：

1. `SpiritGuardTriggeredCommit`：触发扣 1，Impact replay 不重复扣；
2. `SpiritGuardUntriggeredCancel`：更早防御触发，预约取消且不磨损；
3. `SpiritGuardPreIntentRestart`：reserve 后中断，重启清理一次且耐久不变。

既有 `RecoverableIntent` 同组继续覆盖 intent 后恢复。

## 首次失败

首次 changed-path legacy 回归：

```text
Log=Saved/Automation/Dev.D.UE.0.0.10.P5.4.r0/p54_legacy_changed_paths.log
SHA256=16350DDEA56CB7FFA8B852BB7BD49BD34F26930414FC7670F717B976F763E655
Result=67 Success / 1 Fail
NativeExit=0
Failed=demo_map.V3.Items.ModifierBridgeAndRebind
```

失败入口在 `demo_mapItemTests.cpp`：WindTalisman 的真实兼容槽为 `SpatialRingSlot`，旧夹具错误使用 `AccessorySlot`。改正夹具槽位后全部派生断言恢复；未改产品兼容规则。

## 最终自动化

统一命令：

```powershell
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests <group>" -TestExit="Automation Test Queue Empty" -AbsLog=<log>
```

| 日志 | Group | Success | Fail | Native exit | SHA-256 |
|---|---|---:|---:|---:|---|
| `p54_defense_adapter.log` | `Shanmen.0_0_10.Items.DefenseResourceAdapter` | 4 | 0 | 0 | `4B272D6CC157CE53613846F6C142CE35352B1FD60089A4E06CD2EEA2A30C2681` |
| `p54_shanmen_full.log` | `Shanmen.0_0_10` | 124 | 0 | 0 | `D0BF94F8FA99058F65E6F6DC72C41E55E78E53E55273739A7CD4E82680B39076` |
| `p54_legacy_changed_paths_final.log` | `demo_map.ItemEconomySchema+demo_map.V3.Items+demo_map.V3.WorldInteraction+demo_map.RewardGeneration` | 68 | 0 | 0 | `9FEE921916F42E2FC6440966C16E4F1A415B6152B88A50AC2BF2E54FF8EDDBDA` |

最终唯一用例合计 `192 Success / 0 Fail`。Focused 4 条属于 124 条全量的子集。

## Changed-file gate

最终 Source 改动 14 文件：

- combat coordinator／resource adapter／migration／Run adapter → `Shanmen.0_0_10`；
- item definitions／types／tests → `ItemEconomySchema`、`V3.Items`；
- reward pool → `RewardGeneration`；
- registry count consumer → `V3.WorldInteraction`。

两份最终健康日志覆盖所有要求组；首次失败日志独立保留。

## Editor 构建

```powershell
Build.bat demo_mapEditor Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- 完整：`149/149`，Succeeded，退出码 `0`；
- 测试增量：`5/5`，Succeeded，退出码 `0`；
- fail-closed 协调增量：`25/25`，Succeeded，退出码 `0`；
- 旧夹具修正增量：`4/4`，Succeeded，退出码 `0`。

## Game 构建

```powershell
Build.bat demo_map Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- `144/144`，Succeeded，原生退出码 `0`，`403.75s`；
- 输出 `Binaries/Win64/demo_map.exe`；
- 未启动产物。

## 静态与边界

- `git diff --check`：退出码 `0`；
- ShanmenCombatCore／ShanmenItems：无 `UWorld`、`AActor`、`ApplyDamage` 或运行时 RNG 代码依赖；
- `demo_map` 唯一命中是 ShanmenItems API 注释中的产品边界名称；
- 新产品逻辑只存在于 demo_map adapter／coordinator，未反向写入纯内核。

## 最终不变量

1. 只有部署中的精确护体法袍可以形成耐久防御层。
2. 法袍减伤不会同时存在于聚合层和资源层。
3. 一次 Impact 最多有一个稳定 durability reservation。
4. 实际触发提交 1 点；未触发取消 1 点；两者互斥。
5. exact replay 不二次扣耐久。
6. pre-intent orphan 可识别、可排序、可幂等清理。
7. post-intent 故障继续由 P5.3 durable saga 恢复。
8. 资源恢复先于 vitality snapshot，避免 stale CAS。
9. 旧装备与旧 DefinitionId 行为不变。

## P/F 边界

只执行 P 阶段代码、静态检查、无头 `-NullRHI` Automation、Editor/Game Development build。

未启动 Unreal Editor UI、PIE、Standalone、产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p5-4-durable-armor-defense/Docs/Report/Dev.D.UE.0.0.10.P5.4.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p5-4-durable-armor-defense/Docs/Log/Dev.D.UE.0.0.10.P5.4.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p5-4-durable-armor-defense>
