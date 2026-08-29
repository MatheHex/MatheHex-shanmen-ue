# Dev.D.UE.0.0.10.P7.2.r0 Development Log

## 身份

- 阶段：`Dev.D.UE.0.0.10.P7.2.r0`；
- 基线提交：`d3558774bc97d0ec4eea4eb1244251265a0a2cd5`（P7.1）；
- 分支：`agent/0.0.10-p7-2-thrown-world-delivery`；
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`；
- 引擎：Unreal Engine `5.8`；
- 收口日期：`2026-08-29`。

## 目标与决策

P7.0 有纯 Runtime straight launch，P7.1 有 durable Quantity prepare/commit，但尚无 UE physical carrier。直接“先扣 item 再激活 projectile”会在物理初始化失败时丢物品；“先飞再扣”会在两者之间退出时恢复已投掷物品。

P7.2 采用三段式 publication：

```text
live execution Ready
  -> copy candidate launch + begin emission
  -> Actor Staged (NoCollision, movement inactive)
  -> exact P7.1 durable commit
  -> publish candidate + Actor InFlight
```

失败时不改 live execution；durable commit 前 Actor 不产生可观察 World 效果。

## Actor 实现

`Ademo_mapShanmenThrownWeaponProjectile` 只含 Sphere collision 与 ProjectileMovement：

- straight immutable velocity；
- gravity `0`、bounce/homing false；
- collision 只在 committed publication 后启用；
- contact 通过 native delegate 暴露；
- Actor 无 target policy、damage、inventory、GameMode、timer、RNG 或 asset authority；
- no-impact expiry 由 adapter 显式终止。

`ActivateCommittedLaunch` 和 `MarkSpent` 为 World adapter 私有 friend seam；普通调用方不能伪造 committed flight。

## World delivery

`Fdemo_mapShanmenThrownWeaponWorldAdapter` 提供：

- `StagePreparedLaunch`：copy-on-write build + inert Actor stage；
- `CommitStagedLaunch`：调用 P7.1 durable facade，成功后 publication；
- `PublishCommittedLaunch`：重启/replay 的 exact durable evidence seam；
- `ResolveProjectileContact`：Projectile hit -> stable candidate -> target vitality snapshot -> pure resolve -> canonical vitality commit；
- `FinishFlightWithoutImpact`：range/world-static terminal。

Contact resolution 先在 execution 副本上完成 impact、end emission 与 finish flight，再调用 coordinator。delivery 失败则丢弃副本；成功才发布 `Spent`，因此 callback replay 不能重复伤害。

## Coordinator

`DeliverThrownWeaponImpactToM01Enemy` 只是 thin overload，复用既有 `DeliverResolvedPlayerImpactToM01Enemy` 的 Run、source player、registered target、vitality binding 与 idempotent commit 检查。没有第二套 enemy damage path。

## 自动化增量

新增三项：

- `DurableLaunchGate`：证明 staged Actor 完全 inert、unbound authority 失败关闭、exact durable proof 才发布 flight；
- `ContactToVitality`：证明 world hit 产生 0.7 canonical damage、authority revision 前进一次、execution/Actor terminal；
- `FailClosedAndMiss`：证明未注册对象不消费 flight，range expiry 可无 impact 收口。

## 最终自动化日志

统一命令：

```powershell
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests <group>" -TestExit="Automation Test Queue Empty" -AbsLog=<log>
```

| 日志 | Group | Success | Fail | Exit | SHA-256 |
|---|---|---:|---:|---:|---|
| `p72_thrown_world_final.log` | `Shanmen.0_0_10.Product.ThrownWeaponWorldDelivery` | 3 | 0 | 0 | `43D698762896A27FE7C53BA50B2ABC25A6926EFD914746B87810D33369A18AAE` |
| `p72_item_adapter_final.log` | `Shanmen.0_0_10.Product.ThrownWeaponItemAdapter` | 4 | 0 | 0 | `A5EA38FFEF5B73E6C4380CFD79E2F668EEE859A29626A625A3DF3E16F3947FA7` |
| `p72_coordinator_final.log` | `Shanmen.0_0_10.Product.CombatRunCoordinator` | 16 | 0 | 0 | `174DBA9FC65A3700C3672D5806B38A1847F3898612809795D2072BBF0BAA0CA6` |
| `p72_items_final.log` | `Shanmen.0_0_10.Items` | 72 | 0 | 0 | `F57B2CE201BC02696ED255C83354FAC37FC2B6C5676636B09ED491D42E205060` |
| `p72_world_gameplay_final.log` | `Shanmen.0_0_10.WorldGameplay` | 10 | 0 | 0 | `576C114CA7806B0AA51F647BA1503AB91E8D87FA6A3CC59BB2F9152862C54266` |
| `p72_combat_runtime_final.log` | `Shanmen.0_0_10.CombatRuntime` | 30 | 0 | 0 | `4CE78C720289F3F92958A302D7F1506665EAD6F12FFE8B80D677A0EE3AE3EF41` |
| `p72_full_final.log` | `Shanmen.0_0_10` | 185 | 0 | 0 | `3A054425A24537CABD6991619002CE73FC015874AA0438053DBA69E0E5865FF9` |

最终日志均有 queue-empty，bad terminal pattern 为 `0`。Automation discovery 阶段保留项目既有 `Condition failed` 噪声，但目标 ControllerResults 全部 Success。

## 首次失败与修复

1. `p72_thrown_world_initial.log`：native exit `1`，SHA-256 `381039F41F340073BC8A66A987C754AF357BD1F60189390529CA2B4121A57B1A`。测试用 transient Enemy 被 7 点伤害击杀，`EnterDeadState` 进入需要真实 World 的表现代码并 access violation。把夹具公式改成非致死 0.7；产品 delivery 未放宽。
2. `p72_thrown_world_diagnostic.log`：process exit `0`，`2 Success / 1 Fail`，SHA-256 `B3CED14A8AE08772BBA3DE05ED03975ACCB12125AB573E3C137D5C932866FA0F`。严格 changed-file validator 拒绝该日志。拆分断言后定位为 `3.0f - 2.3f` 的浮点减法超过默认 tolerance，改为显式 `KINDA_SMALL_NUMBER`；最终 3/3 与 185/185 通过。

## Changed-file gate

新增 `ThrownWeaponWorldDelivery` 映射，覆盖 Actor、World adapter 与 tests，并要求 World product、P7.1 item adapter、coordinator、Items、WorldGameplay、CombatRuntime 六组。

```text
REGRESSION_COVERAGE: PASS Changed=9 Rules=2 Required=6 Logs=7
SELF_TEST: PASS 18/18
```

## 构建时间线

统一使用：

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- 初次 Editor：`41/41`，Succeeded，exit `0`，`148.59s`；
- 测试修正后的增量 Editor 构建均成功；
- 最终 Editor：`7/7`，Succeeded，exit `0`，`17.86s`；
- 最终 Game：`38/38`，Succeeded，exit `0`，`129.46s`。

Game executable 只构建，未启动。

## 静态、范围与兼容性

- staged diff check：native exit `0`；
- mapping self-test：`18/18`；
- 新文件无 `ApplyDamage`、legacy projectile、GameMode、SpawnActor、timer 或 RNG；
- 未改 schema、Build.cs、GameplayTags、Content、输入、GameMode、Profile 或 CodeB；
- 0.0.10 full suite `185/185`；
- 长期未跟踪的 0.0.9B 与用户文件保持未跟踪且未 stage。

## P/F 边界

只执行 P 阶段源码、静态检查、无头 Automation、Editor/Game Development build。未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未做真实输入、截图、Smoke、Cook 或 Package。

## 下一阶段

P7.3 建立拥有 action/execution/Actor 的 product host，绑定 contact delegate、生成/销毁和 range terminal。输入、视觉和发射动画后置；arc、homing、steering、recall 继续排除。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p7-2-thrown-world-delivery/Docs/Report/Dev.D.UE.0.0.10.P7.2.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p7-2-thrown-world-delivery/Docs/Log/Dev.D.UE.0.0.10.P7.2.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p7-2-thrown-world-delivery>
