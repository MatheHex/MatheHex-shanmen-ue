# Dev.D.UE.0.0.10.P6.0.r0 Development Log

## 身份

- 阶段：`Dev.D.UE.0.0.10.P6.0.r0`；
- 基线提交：`d132b4fc27fd79fd204eb86c3ea9fd275c6f7679`（P5.5）；
- 分支：`agent/0.0.10-p6-0-controlled-flying-sword-contract`；
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`；
- 引擎：Unreal Engine `5.8`；
- 收口日期：`2026-08-28`。

## 目标推导

人工战斗规划已经明确：御器不是一次性弹丸，而是玩家以真实法器/飞剑为载体，在离手后持续控制方向、接触和收回的体系。相邻的修理、护心镜、正式元素抗性与其它被动灵器仍未冻结。

因此 P6.0 只冻结最小可复用内核：

1. 受控武器必须绑定 exact physical ItemInstanceId；
2. Launch / Redirect / Recall 必须有确定性、可回放、严格顺序的回执；
3. 飞剑接触必须与 generic Projectile 区分；
4. 接触只进入既有 CombatCore 纯结算与幂等 Ledger；
5. 世界、输入、移动、表现和产品物品验证留给后续 Adapter。

## 实现记录

### Frozen definition and offense

`FShanmenControlledWeaponDefinition::TryCapture` 只接受 canonical action `Combat.Action.ControlledWeapon.FlyingSword01`、有效 detector/formula、非负有限数值、PhysicalSlash damage tag 与 Living target tag。

`FShanmenControlledWeaponOffenseSnapshot` 在 activation 时冻结 ControlPower。两者的运行态字段均为私有只读快照，不允许产品蓝图在执行中改写。

### Physical execution identity

`TryCreate` 复用现有 action snapshot，并要求其中的 `SourceItemInstanceId` 有效。创建时建立 `ControlledObject` detector session；物品身份随后进入每条命令和每个 Impact request。

纯执行器不查询或修改 ShanmenItems。产品接线必须先证明该 exact item 已部署在 active Run，再把身份交给本契约。

### Command ledger

命令状态机：

```text
Orbiting --Launch--> Directed --Redirect--> Directed --Recall--> Recalled
```

`TryIssueCommand` 要求 action runtime 与 snapshot 精确匹配并处于 Active。每条新命令只能消费 `NextCommandSequence`；旧 sequence 仅在完整输入与已存回执一致时作为幂等 replay 成功。

CommandId 的规范输入：namespace、ActivationId、SourceItemInstanceId、sequence、kind、归一化 direction IEEE bits。实现期间静态复核发现拒绝分支应恢复旧方向，已保存 `DirectionBefore` 并在失败路径回滚；未产生失败构建或失败测试。

### ControlledObject emission and impact

只有 Directed 状态可开始 emission。候选需匹配 activation/source、`ControlledObject` detector、目标标签与 self policy。

合法候选构造既有 `FShanmenImpactRequest`，通过 `FShanmenDefenseResolver::Resolve` 后写入 `FShanmenImpactLedger`。同 emission 同目标去重，后续 emission 使用递增 ordinal；termination cleanup 可在 action 结束后无条件关闭遗留窗口。

测试公式为 authoring fixture：

```text
BaseDamage 10 + ControlPower 40 * Coefficient 0.25 = IncomingDamage 20
```

该公式仅验证冻结与管线，不作为最终数值设计。

## 新增自动化

- `Shanmen.0_0_10.CombatRuntime.ControlledWeapon.PhysicalItemBoundary`；
- `Shanmen.0_0_10.CombatRuntime.ControlledWeapon.CommandSequenceAndReplay`；
- `Shanmen.0_0_10.CombatRuntime.ControlledWeapon.ControlledObjectImpacts`；
- `Shanmen.0_0_10.CombatRuntime.ControlledWeapon.DeterministicControlReplay`。

覆盖缺物品拒绝、canonical action、Startup 拒绝、commit 后 Launch、严格 sequence、精确 replay、冲突 replay、跳号、Redirect、Recall/emission 竞态、recall 终态、Impact 去重、跨 emission ordinal、interrupt cleanup 与独立执行确定性。

## 自动化结果

统一命令形态：

```powershell
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests <group>" -TestExit="Automation Test Queue Empty" -AbsLog=<log>
```

| 日志 | Group | Success | Fail | Native exit | SHA-256 |
|---|---|---:|---:|---:|---|
| `p60_controlled_weapon_focused.log` | `Shanmen.0_0_10.CombatRuntime.ControlledWeapon` | 4 | 0 | 0 | `585D9672FE41BF14A02DF984E21850D3E41437EC46CA112C3D064B2D74B2B896` |
| `p60_combat_runtime_final.log` | `Shanmen.0_0_10.CombatRuntime` | 21 | 0 | 0 | `71E4EE4DD16B8C0166107AD71A3B2550F956DD0C2FB2381608AD8FAF959E3C17` |
| `p60_shanmen_full_final.log` | `Shanmen.0_0_10` | 129 | 0 | 0 | `67F39653A1ED51F7A82A87ABF92E796526BCD8C93A24DB0FA8BB4698CE008DD3` |

最终唯一计数 `129 Success / 0 Fail`；三条日志正常出现 `Automation Test Queue Empty`，原生退出码均为 0。

`p60_shanmen_full_final.log` 在实际测试队列前含 13 行 UE 既有 `LogAutomationTest: Error: Condition failed` 启动噪声；目标队列仍为 129/129 Success，Fatal 为 0。日志原样保留。

## Changed-file gate

变更输入只包含三个 `ShanmenCombatRuntime` 生产/测试文件。回归映射要求 `Shanmen.0_0_10.CombatRuntime`，最终父组日志为其超集：

```text
REGRESSION_COVERAGE: PASS Changed=3 Rules=1 Required=1 Logs=1
REGRESSION_COVERAGE: Group=Shanmen.0_0_10.CombatRuntime Evidence=p60_shanmen_full_final.log
REGRESSION_COVERAGE: Log=p60_shanmen_full_final.log Group=Shanmen.0_0_10 Success=129 SHA256=67F39653A1ED51F7A82A87ABF92E796526BCD8C93A24DB0FA8BB4698CE008DD3
```

## 构建

Editor：

```powershell
Build.bat demo_mapEditor Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- `8/8` actions；
- `Result: Succeeded`；
- native exit `0`；
- `38.08s`。

Game：

```powershell
Build.bat demo_map Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- `5/5` actions；
- `Result: Succeeded`；
- native exit `0`；
- `27.24s`；
- 输出 `Binaries/Win64/demo_map.exe`，未启动。

聚焦测试、最终父组和两目标构建均首次成功。本轮无源码、环境或原生退出失败；静态复核中的方向回滚修正发生在验证前。

## 静态与边界

- `git diff --check`：native exit `0`；
- 3 个源码/测试文件共 1,155 行新增；
- `demo_map` / `ShanmenItems` include 命中 0；
- `UWorld` / `AActor` / `ApplyDamage` / `UGameplayStatics` 命中 0；
- `FMath::Rand` / `FRandomStream` 命中 0；
- 既有生产文件、Build.cs、持久化格式与回归映射均未修改。

## P/F 边界

只执行 P 阶段代码、静态检查、无头 `-NullRHI` Automation、Editor/Game Development build。

未启动 Unreal Editor UI、PIE、Standalone、产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。

## 后续接线约束

P6.1 产品 Adapter 应：

1. 从当前 active Run 的 ShanmenItems 权威验证 exact deployed flying-sword ItemInstanceId；
2. 把输入映射成单调命令，把 Actor 运动/表现保持在产品层；
3. 把真实几何接触变成 `ControlledObject` candidate；
4. 让生命、物品资源与库存写入分别通过既有权威服务完成。

不得由移动 Actor 直接扣物品/生命，不得复用 generic Projectile detector，也不得在规则冻结前发明修理或耐久成本。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-0-controlled-flying-sword-contract/Docs/Report/Dev.D.UE.0.0.10.P6.0.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p6-0-controlled-flying-sword-contract/Docs/Log/Dev.D.UE.0.0.10.P6.0.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p6-0-controlled-flying-sword-contract>
