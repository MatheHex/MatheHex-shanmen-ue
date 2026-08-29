# Dev.D.UE.0.0.10.P7.0.r0 Development Log

## 身份

- 阶段：`Dev.D.UE.0.0.10.P7.0.r0`；
- 基线提交：`42ca7320c317e453768b73a0c188cc13e4663d47`（P6.23）；
- 分支：`agent/0.0.10-p7-0-thrown-weapon-core`；
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`；
- 引擎：Unreal Engine `5.8`；
- 收口日期：`2026-08-29`。

## 问题与目标

人工 0.0.10 战斗规划明确区分：御器依赖出手后的持续控制，暗器依赖瞄准与投掷手法；初级暗器百解只允许直线投掷，中级弧线与高级自动路径尚未进入基础实现。现有 P6 `ControlledWeapon` 拥有 Redirect/Recall，legacy projectile 又不携带 0.0.10 exact item、Action、Impact 和冻结数值身份，因此两者都不能作为暗器内核。

P7.0 新建独立纯 Runtime contract，只关闭“真实物品 → 一次直线发射 → Projectile candidate → CombatCore Impact”这一最小闭环。产品 Actor、库存事务与输入后置。

## 设计决策

### 暗器不是退化御器

`FShanmenThrownWeaponExecution` 没有 redirect、recall、steering 或 homing API。状态只有 `Ready / InFlight / Spent`。同一 activation 只能冻结一份 launch receipt；不同 trajectory payload 冲突失败，相同 canonical payload 返回原 receipt 且不改变状态。

### exact item 与 content 进入 Launch identity

Action 必须包含有效 `SourceItemInstanceId`。LaunchId 使用独立 `Shanmen.ThrownWeapon.StraightLaunch.r1` namespace，覆盖 Run、Owner、Activation、Source Entity、exact item、Definition、Content version/digest、Origin、Direction 和 Speed。FVector 分量按 double bits、速度按 float bits编码，正负零先规范化。

### 产品数值由 Definition 冻结

Definition 提供 Formula、BaseDamage、Technique coefficient、LaunchSpeed、DamageTags 与 target policy；Runtime 没有硬编码平衡默认值。Offense snapshot 冻结 activation 时的 technique power。Damage 只执行：

```text
RawDamage = BaseDamage + TechniquePower * TechniquePowerCoefficient
```

结果继续交给现有 `FShanmenDefenseResolver`，P7.0 不提交 vitality。

### 显式物理 sample

只有 InFlight 可 Begin/Resolve/End emission。每次调用方物理 sample 使用 `Projectile` detector ordinal；同 sample 内按 target 去重，跨 sample 保留新 ordinal。Flight completion 与 active emission 互斥，terminal action 使用既有 cleanup seam 关闭 emission。

## 自动化增量

`Shanmen.0_0_10.CombatRuntime.ThrownWeapon` 新增 4 项：

- `PhysicalItemBoundary`；
- `StraightLaunchAndReplay`；
- `ImpactPolicyAndOrdinals`；
- `DeterminismAndTermination`。

覆盖缺 exact item、错误 action identity、零速度、未捕获 offense、Startup fence、零方向、方向单位化、正负零、重放/冲突、禁止再定向、open emission finish fence、Spent fence、Projectile kind、self/living policy、公式、重复 callback、多 target、多 ordinal、Content stamp 隔离和 interruption cleanup。

## 最终自动化日志

统一命令：

```powershell
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests <group>" -TestExit="Automation Test Queue Empty" -AbsLog=<log>
```

| 日志 | Group | Success | Fail | Native exit | 完成跨度 | SHA-256 |
|---|---|---:|---:|---:|---:|---|
| `p70_thrown.log` | `Shanmen.0_0_10.CombatRuntime.ThrownWeapon` | 4 | 0 | 0 | `0.050s` | `A40BF32C46599BB56ACC04174B1BFF7514BF43A61804FA4175E0C396E8740A6A` |
| `p70_full.log` | `Shanmen.0_0_10` | 175 | 0 | 0 | `8.395s` | `DC0241C824A32C81CA92EE3801E245D2003E38E56CC3D1B19E09BDA0BECB93DC` |

两份最终日志均满足唯一 RunTests、queue-empty、Fail `0`、Fatal/unhandled/ensure `0` 与原生退出 `0`。测试发现前各有 13 条项目既存 `LogAutomationTest: Error: Condition failed` 诊断噪声，ControllerErrors 为 `0`；最终测试结果全部 Success。首次聚焦执行同样为 `4/4`，没有失败自动化日志。

## Changed-file gate

三个新增 `ShanmenCombatRuntime` 文件要求父级 Runtime 回归，完整 suite 提供覆盖：

```text
REGRESSION_COVERAGE: PASS Changed=3 Rules=1 Required=1 Logs=2
SELF_TEST: PASS 16/16
```

文档属于 ignored path；加入 Report/Log 后门禁结论不变。

## 构建时间线

统一命令：

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

1. 首次 Editor integration：`8/8`，Succeeded，native exit `0`，`34.41s`；
2. Launch receipt action/speed identity 自校验增强后最终 Editor：`4/4`，Succeeded，native exit `0`，`4.52s`；
3. 最终 Game：`5/5`，Succeeded，native exit `0`，`23.39s`。

最终 Editor Runtime DLL UTC 为 `2026-08-29T09:03:30Z`，Game executable UTC 为 `2026-08-29T09:05:09Z`。无源码、环境、内存、SDK、超时或链接失败；Game 未启动。

## 静态、范围与兼容性

- `git diff --check`：native exit `0`；
- 新增 Source：生产 `748` 行、测试 `422` 行，合计 `1170` 行，无删除；
- 生产新增调用的 GetWorld/SpawnActor/damage apply/item commit/Tick/timer/RNG 扫描命中 `0`；
- 没有新增 Actor、World query、movement、item reservation、vitality commit、input、GAS ability、content asset 或 schema；
- 没有修改 P6 ControlledWeapon、legacy projectile、CombatCore、Items 或 demo_map 产品文件；
- 长期未跟踪的 0.0.9B Prompt、Report、CSEMI 与用户文档未纳入 stage。

## P/F 边界

只执行 P 阶段源码、静态检查、无头 `-NullRHI` Automation 与 Editor/Game Development build。

未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。

## 下一阶段

P7.1 应建立现有 `ShanmenItems` authority 到 P7.0 activation 的单向事务 adapter：预留 exact thrown item，commit point 后确认消费，cancel/interrupt 释放；不让 execution 或 Actor 直接改库存。Actor 直线运动和真实输入应继续作为后续独立阶段。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p7-0-thrown-weapon-core/Docs/Report/Dev.D.UE.0.0.10.P7.0.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p7-0-thrown-weapon-core/Docs/Log/Dev.D.UE.0.0.10.P7.0.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p7-0-thrown-weapon-core>
