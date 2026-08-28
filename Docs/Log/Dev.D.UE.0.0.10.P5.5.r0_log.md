# Dev.D.UE.0.0.10.P5.5.r0 Development Log

## 身份

- 阶段：`Dev.D.UE.0.0.10.P5.5.r0`；
- 基线提交：`c6d58e85ea8e8fb72a35a94baded02837667fc10`（P5.4）；
- 分支：`agent/0.0.10-p5-5-durable-equipment-lifecycle`；
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`；
- 引擎：Unreal Engine `5.8`；
- 收口日期：`2026-08-28`。

## 目标推导

人工战斗规划明确把护心镜、正式抗性槽和其它被动救命灵器列为尚未冻结，因此 P5.5 不新增这些规则。P5.4 已有真实耐久法袍，但缺少：

1. 准备界面的 current/max 只读资源展示；
2. 磨损后 Extraction 持久化证明；
3. 下一局从磨损值继续的证明；
4. Death 销毁资源的完整终态证明。

本轮选择关闭这条既有产品内容的真实生命周期，而不是扩张未冻结机制。

## 实现记录

### Authority projection

`Fdemo_mapProfilePreparationStashRow` 增加 durability/charges 当前值与最大值。`BuildProjection`：

- 当前值读取 `FShanmenItemInstance`；
- 最大值读取当前 authority snapshot 的 `FShanmenItemDefinition`；
- 缺定义或越界失败关闭；
- Destroyed/Depleted 仍不进入战备列表；
- 旧 Profile 行默认 0/0，不成为资源写路径。

### Presenter/UI

Presenter 复制完整资源状态并按非零 capability 生成 `DUR x/y`、`CHG x/y`。Widget 列表只在有资源时追加标签，详情始终给出明确 Resources 行。

### Lifecycle test

新增 `SpiritGuardDurableLifecycle`，按真实 subsystem 和 durable repository 执行两局：

```text
Run 1: 20 -> 19 -> Extraction -> Stored 19 -> restart 19
Run 2: reselect same identity -> 19 -> 18 -> Death -> Destroyed 0 -> restart tombstone 0
```

同时断言战备投影 `19/20`、Presenter 标签 `DUR 19/20`，以及销毁后列表不再包含该身份。

## 回归门禁发现与修正

首次按既有映射执行时，脚本报告 6 个 required groups 全覆盖。但额外检查直接消费者发现：

- `demo_map.P2.EntityLoadout`：`0 Success / 4 Fail`，SHA-256 `DE40BBC6A0E17A5709D0119348A7268C2406F9D8CC27B8EABAD37C8E7542F9A4`；
- `demo_map.GridInventory`：`14 Success / 13 Fail`，SHA-256 `0B31DE9C5F57785FE6825092A847604BEF7EFEC384ED0AEC3CB9B89EF8E4F9BA`。

根因不是新增资源字段，而是测试仍使用旧容量、旧槽位和旧 schema 断言。生产 `Fdemo_mapItemDefinitions::ValidateRegistry` 已明确验证 base 6、ring 4、bag 36、total 46；当前 presenter 也明确生成五个装备角色。

修正：

- 测试改用现有 `36/42` 容量、五角色、SpatialRingSlot、schema 7；
- 等容量背包替换测试不再伪造不存在的 20→16 缩容；
- RegressionMap 新增 `PreparationProjectionConsumers`，以后这些文件必须跑 GridInventory + EntityLoadout；
- 门禁自测 `8/8` 通过。

## 探索性父组失败

探索命令：

```powershell
Automation RunTests demo_map
```

结果：`1212 Success / 115 Fail`，原生退出码 `0`，但 Automation 明确失败，因此不计通过。

- 日志：`Saved/Automation/Dev.D.UE.0.0.10.P5.5.r0/p55_demo_map_full.log`；
- SHA-256：`342ED20218F5E4B3AA95CF21ECE287AD97F55772C872E3D6109DBEA730EED40A`。

直接相关 17 条已修复。其余失败涉及历史 reward、prepared runtime、layout 与跨组静态状态污染；未用该父组作为健康证据，也未在本阶段改动无关产品。

## 最终自动化

统一命令：

```powershell
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests <group>" -TestExit="Automation Test Queue Empty" -AbsLog=<log>
```

| 日志 | Group | Success | Fail | Native exit | SHA-256 |
|---|---|---:|---:|---:|---|
| `p55_shanmen_full_final.log` | `Shanmen.0_0_10` | 125 | 0 | 0 | `9C51D4B8BCF8C611EDBE6257EE187F6C7EBC40B6AF1EF66A0EFF2920BEE0E79D` |
| `p55_profile_final.log` | `demo_map.Profile` | 211 | 0 | 0 | `000C74435BAAFB7D1874DDB5FE20395B04E7C8CC518A1CD46783DD6B9B1604DB` |
| `p55_item_economy_schema_final.log` | `demo_map.ItemEconomySchema` | 23 | 0 | 0 | `583A7E1512160626FE87120B75AF92611E31E09F89BB165585E1361D1C92A4A5` |
| `p55_item_use_armor_final.log` | `demo_map.ItemUseAndArmor` | 46 | 0 | 0 | `24771AE83CA3EC40E87820001A6791B4CDD7E240D6551BC17FC26812241094C9` |
| `p55_hotbar_final.log` | `demo_map.P4.Hotbar` | 7 | 0 | 0 | `68C424ED6C04BD4CCB08F274C9D484FF82988484EDF40317724A795A22E4EDAE` |
| `p55_entity_loadout_final.log` | `demo_map.P2.EntityLoadout` | 4 | 0 | 0 | `2C3982FCEF36515E23AA2CC56FCCDD18D6B65A5EC63BD20F054355AED7CD9D36` |
| `p55_grid_inventory_final.log` | `demo_map.GridInventory` | 27 | 0 | 0 | `F0A6590E66F93DC4737E4CFC5BD0C5548C8526858D422D8959867EEB9BCD5024` |

最终唯一计数：`443 Success / 0 Fail`。

## Changed-file gate

最终改动 9 个代码/映射文件，匹配 4 条规则，要求 8 个 group：

```text
REGRESSION_COVERAGE: PASS Changed=9 Rules=4 Required=8 Logs=7
```

覆盖：

- `Shanmen.0_0_10.Items`、`Shanmen.0_0_10.CombatCore` ← `Shanmen.0_0_10`；
- `demo_map.Profile`；
- `demo_map.ItemEconomySchema`；
- `demo_map.ItemUseAndArmor`；
- `demo_map.P4.Hotbar`；
- `demo_map.P2.EntityLoadout`；
- `demo_map.GridInventory`。

## 构建

Editor：

```powershell
Build.bat demo_mapEditor Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- 资源投影完整受影响构建：`53/53`，Succeeded，exit `0`，`196.38s`；
- 测试契约增量：`5/5`，Succeeded，exit `0`，`16.18s`。

Game：

```powershell
Build.bat demo_map Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- 资源投影完整受影响构建：`52/52`，Succeeded，exit `0`，`176.74s`；
- 最终测试增量：`4/4`，Succeeded，exit `0`，`22.65s`；
- 输出 `Binaries/Win64/demo_map.exe`，未启动。

## 静态与边界

- `git diff --check`：退出码 `0`；
- 修改规模：Source/Map 共约 `309 additions / 34 deletions`，另有 Report/Log；
- ShanmenCombatCore/ShanmenItems 的 `UWorld`、`AActor`、`ApplyDamage`、运行时 RNG 代码命中为 0；
- 唯一 `demo_map` 命中是公共头注释；
- 未改 authority document、Profile schema、DefinitionId 或内容 digest。

## P/F 边界

只执行 P 阶段代码、静态检查、无头 `-NullRHI` Automation、Editor/Game Development build。

未启动 Unreal Editor UI、PIE、Standalone、产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p5-5-durable-equipment-lifecycle/Docs/Report/Dev.D.UE.0.0.10.P5.5.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p5-5-durable-equipment-lifecycle/Docs/Log/Dev.D.UE.0.0.10.P5.5.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p5-5-durable-equipment-lifecycle>
