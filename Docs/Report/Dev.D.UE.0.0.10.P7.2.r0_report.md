# Dev.D.UE.0.0.10.P7.2.r0 Report

## 1. 结论

P7.2 已完成一次性直线暗器的 World delivery 与物理 Actor 生命周期，结论为 **PASS**。

本轮把 P7.0 的不可变 straight launch、P7.1 的持久 Quantity intent，以及既有 WorldGameplay / CombatRun vitality 权威接成一条失败关闭链：Actor 先以无碰撞、无移动状态冻结 launch；只有 exact item durable commit 成功后才发布真实飞行；命中经 `Projectile` candidate、纯 resolver 和 M01 vitality authority 提交，成功一次后立即进入 `Spent`。

尚未加入输入、生成策略、视觉资产、弧线、追踪、转向或召回。

## 2. 发射事务门

新增 `Fdemo_mapShanmenThrownWeaponLaunchPlan` 作为 copy-on-write launch candidate：

- live `FShanmenThrownWeaponExecution` 在 durable commit 前保持 `Ready`；
- candidate 执行 P7.0 `TryLaunchStraight` 与 `TryBeginEmission`，冻结 LaunchId、Action、方向、速度和 hit context；
- P7.1 `BuildCommitRequest` 必须接受同一 Action 的 launch receipt；
- physical Actor 只进入 `Staged`，碰撞为 `NoCollision`，movement 未激活；
- `CommitStagedLaunch` 只有在 GameInstance item authority 持久提交成功后才同时发布 execution candidate 和 Actor flight；
- authority 拒绝时丢弃 staged Actor，live execution 仍为 `Ready`，既有 prepared intent 可安全重试；
- durable replay 可通过 `PublishCommittedLaunch` 恢复同一飞行，不发第二次库存命令。

激活能力已设为 World adapter 私有 friend seam，外部调用方不能绕开 item gate。

## 3. 物理 Actor 边界

新增 `Ademo_mapShanmenThrownWeaponProjectile`：

- 一个 `USphereComponent` 和一个 `UProjectileMovementComponent`；
- 无 Actor Tick、重力、bounce、homing、timer、随机数或资产加载；
- 速度和方向只来自 immutable launch receipt；
- `Empty -> Staged -> InFlight -> Spent` 单向生命周期；
- native contact delegate 只转发 `FHitResult`，Actor 不选择目标、不算伤害、不写库存；
- source Actor 只作瞬态碰撞忽略，不参与稳定身份派生；
- range expiry / world geometry 有显式 `FinishFlightWithoutImpact` 终态，不伪造 impact。

Actor 不复用 0.0.9B monolithic `Ademo_mapSkillProjectile`，避免带入 GameMode、legacy targeting、`ApplyDamage` 或第二套 projectile authority。

## 4. World 命中与 Vitality

`Fdemo_mapShanmenThrownWeaponWorldAdapter::ResolveProjectileContact` 执行：

1. 校验 active action、in-flight execution、Actor launch/context 和 coordinator Run 一致；
2. 使用 `FShanmenWorldHitAdapter::TryFromProjectile` 把 UE hit 转成稳定 entity candidate；
3. 从 `Idemo_mapCombatVitalityHost` 捕获同一目标的 vitality snapshot；
4. 在 execution 副本上 resolve、结束 emission 并结束 flight；
5. 经新增 `DeliverThrownWeaponImpactToM01Enemy` 复用 coordinator 的唯一 player-to-M01 vitality 提交路径；
6. 只有 delivery 成功才发布 execution 副本并把 Actor 设为 `Spent`。

未注册对象、身份错配、不可用 vitality、candidate 拒绝或 delivery 拒绝均不消费 live flight。成功后的重复 callback 在 `FlightNotActive` 处失败，不能双伤害。

## 5. 自动化证据

最终无头 `-NullRHI` 自动化全部通过；每份日志只有一个实际 RunTests、queue-empty、Fail `0`、Fatal/assert/unhandled exception `0`，原生退出码均为 `0`。

| Group | Success | Fail | SHA-256 |
|---|---:|---:|---|
| `Shanmen.0_0_10.Product.ThrownWeaponWorldDelivery` | 3 | 0 | `43D698762896A27FE7C53BA50B2ABC25A6926EFD914746B87810D33369A18AAE` |
| `Shanmen.0_0_10.Product.ThrownWeaponItemAdapter` | 4 | 0 | `A5EA38FFEF5B73E6C4380CFD79E2F668EEE859A29626A625A3DF3E16F3947FA7` |
| `Shanmen.0_0_10.Product.CombatRunCoordinator` | 16 | 0 | `174DBA9FC65A3700C3672D5806B38A1847F3898612809795D2072BBF0BAA0CA6` |
| `Shanmen.0_0_10.Items` | 72 | 0 | `F57B2CE201BC02696ED255C83354FAC37FC2B6C5676636B09ED491D42E205060` |
| `Shanmen.0_0_10.WorldGameplay` | 10 | 0 | `576C114CA7806B0AA51F647BA1503AB91E8D87FA6A3CC59BB2F9152862C54266` |
| `Shanmen.0_0_10.CombatRuntime` | 30 | 0 | `4CE78C720289F3F92958A302D7F1506665EAD6F12FFE8B80D677A0EE3AE3EF41` |
| `Shanmen.0_0_10` | 185 | 0 | `3A054425A24537CABD6991619002CE73FC015874AA0438053DBA69E0E5865FF9` |

完整 suite 从 P7.1 的 182 增至 185。包含关系测试不作相加。

## 6. 改动—回归与静态门禁

- `REGRESSION_COVERAGE: PASS Changed=9 Rules=2 Required=6 Logs=7`；
- mapping self-test：`18/18 PASS`，新增 thrown World/Actor 映射自测；
- `git diff --check`：native exit `0`；
- 新增边界扫描无 `ApplyDamage`、`UGameplayStatics`、`GetAuthGameMode`、`SpawnActor`、timer、RNG 或旧 `demo_mapSkillProjectile` 引用；
- TODO、FIXME、HACK、临时 debug marker 和生产 `UE_LOG` 命中 `0`；
- 未修改 schema、Build.cs、GameplayTags、Content、GameMode 或输入。

## 7. 构建与首次失败记录

统一命令：

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- 初次 Editor integration：`41/41`，Succeeded，native exit `0`，`148.59s`；
- 最终 Editor：`7/7`，Succeeded，native exit `0`，`17.86s`；
- 最终 Game：`38/38`，Succeeded，native exit `0`，`129.46s`；
- Editor product DLL UTC：`2026-08-29T10:37:11Z`；
- Game executable UTC：`2026-08-29T10:39:27Z`。

保留两份失败证据：

1. `p72_thrown_world_initial.log`：native exit `1`，首个 contact test 在 transient Enemy 上造成致死伤害，死亡表现访问未注册 World 并崩溃；SHA-256 `381039F41F340073BC8A66A987C754AF357BD1F60189390529CA2B4121A57B1A`。该测试改为非致死 `0.7`，产品链未为夹具放宽。
2. `p72_thrown_world_diagnostic.log`：进程 exit `0` 但测试 `2 Success / 1 Fail`；改动—回归门禁正确拒绝假绿。原因为浮点减法断言使用过严默认 tolerance，改为显式 `KINDA_SMALL_NUMBER`；SHA-256 `B3CED14A8AE08772BBA3DE05ED03975ACCB12125AB573E3C137D5C932866FA0F`。

## 8. 修改范围与兼容性

生产修改仅包括：

- 新 thrown projectile Actor；
- 新 thrown World adapter 与三项产品测试；
- CombatRunCoordinator 新增一个薄 delivery overload；
- changed-file regression mapping 与 self-test；
- 本 Report 与同名 Development Log。

P7.0 execution、P7.1 item adapter、ShanmenItems schema、旧 projectile/skill、Profile、CodeB、御器、敌人和输入代码均未改。所有既有 0.0.10 tests 通过。长期未跟踪的 0.0.9B Prompt、Report、CSEMI 和用户文档未纳入 stage。

## 9. P/F 边界与下一阶段

本轮只执行 P 阶段源码、静态检查、无头 Automation 与 Editor/Game Development build。未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、Cook 或 Package。

P7.3 建议建立产品 host/spawn command：持有 action/execution/Actor，绑定 native contact delegate，并把 range expiry 映射到本轮显式 miss terminal。输入与视觉可在 host 稳定后单独接入；steering、homing、arc 和 recall 继续排除。

## 10. GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p7-2-thrown-world-delivery/Docs/Report/Dev.D.UE.0.0.10.P7.2.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p7-2-thrown-world-delivery/Docs/Log/Dev.D.UE.0.0.10.P7.2.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p7-2-thrown-world-delivery>
