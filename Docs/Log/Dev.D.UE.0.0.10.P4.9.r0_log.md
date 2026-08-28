# Dev.D.UE.0.0.10.P4.9.r0 Development Log

## 目标

把 M01 authored `StandardRanged` projectile 的首次合法 hostile contact 接入 0.0.10 canonical combat。保留既有 flight／collision／targeting／destroy语义，使用稳定 projectile sequence、冻结 action spec、deterministic identity、玩家 defense snapshot与 exactly-once vitality ledger。M01 产品失败必须关闭且不得 fallback；非 M01与 Boss projectile继续兼容。

## 基线

- 基线分支：`agent/0.0.10-p4-8-melee-dash-product`；
- 基线提交：`2b101cc5d4daf95bb7f22ac04b163dba4bcfa0b3`；
- 当前分支：`agent/0.0.10-p4-9-ranged-projectile-product`；
- 基线 coordinator 定向：9/9；
- 基线 0.0.10 全量：105/105；
- P4.8 已完成 ordinary melee与 melee dash；shared projectile contact仍直接调用 `UGameplayStatics::ApplyDamage`。

## 审计与设计决定

### 1. 只迁移 authored M01 StandardRanged

M01 配置包含 3 个 `StandardRanged` 定义，全部绑定 `StandardRangedBackstep`。本轮冻结一个 `StandardRangedProjectile` family；`EnhancedRangedBackstep` 与 melee source不能借用该 family。地图不是 M01时不进入产品 gate，Boss也不是 `Ademo_mapRangedEnemyCharacter`，因此既有兼容路径保持。

### 2. sequence 在 flight 前保留

projectile Actor没有可复用的稳定 action serial，因此 source ranged Actor持有 `uint64 NextProjectileSequence`。spawn成功后立即取出并递增，再把 profile与 sequence写入 projectile并启动 flight；spawn失败不占号。sequence结合 RunId与稳定 source EntityId产生可重放 identity，避免碰撞回调时间或 UObject地址参与身份。

### 3. shared projectile只负责 world contact adapter

shared projectile继续做碰撞、目标关系、filter与 first-contact消费。M01 ranged contact把 location／normal与冻结 metadata交给 GameMode/coordinator；resolver不查询 World。进入 canonical gate后，无论执行成功、replay还是失败，都不再调用 legacy damage。

### 4. 复用 enemy attack kernel与生命 ledger

P4.7/P4.8 receipt结构已支持 family spec、action runtime、candidate、damage payload、defense snapshot、resolver与 vitality delivery。本轮只扩展 detector kind与 projectile spec，不复制结算。receipt额外冻结 `HitOrdinal == 0`，与当前非穿透 target-only projectile的单次接触契约一致。

### 5. 兼容测试失败属于基线漂移

V2与 ItemUseAndArmor 初跑失败均由旧断言落后于既有产品迁移造成：Profile Schema早已为 `7`，纳物戒早已独立到 `SpatialRingSlot`。产品文件与 P4.8 基线无差异；本轮只维护测试，不反向改回旧 schema或 slot。

## 实现过程

1. 审计 ranged projectile spawn、shared contact、M01 authored definitions、Boss initializer与 non-M01 gate。
2. 增加 `StandardRangedProjectile` family与冻结 action／detector／formula／content spec。
3. spec显式携带 `DetectorKind`，既有 melee保持 `Shape`，projectile使用 `Projectile`。
4. receipt校验 spec detector kind与固定 `HitOrdinal == 0`。
5. 新增 `ExecuteM01EnemyRangedProjectileImpact`，在创建 identity前拒绝无效 profile、zero sequence与 NaN contact。
6. 泛化 enemy attack执行器，使 melee继续由 Actor位置构造 shape contact，projectile保留实际 world contact geometry。
7. GameMode增加 M01 projectile产品入口与结构化 action／impact／commit日志。
8. ranged Actor增加 Run-local sequence，并在成功 flight前传给新的 `InitializeTargetedEnemyProjectile`。
9. shared projectile在 M01 ranged来源时调用 canonical入口；进入产品 gate后只消费／销毁，不 fallback。
10. 新增 `M01EnemyRangedProjectileProduct` 自动化，覆盖身份、profile、contact、fractional defense、replay、reconstruction reject、Run隔离与 lethal clamp。
11. EnemySkillFramework增加 sequence默认值契约。
12. 执行 Editor构建、定向／全量自动化、三组兼容回归、静态扫描与 Game构建。
13. 对 V2／Item初始失败做最小测试基线维护，保留初始日志与 SHA-256。

## 验证时间线

1. Editor Development 首次完整构建：26/26，Succeeded，退出码 0。
2. coordinator定向初跑：10/10 Success，0 Fail。
3. EnemySkillFramework初跑：44/44 Success，0 Fail。
4. V2RangedCompatibility初跑：20 Success、2 Fail；均为旧 Schema 4断言。
5. ItemUseAndArmor初跑：42 Success、4 Fail；为旧 Schema 4与纳物戒旧 AccessorySlot断言。
6. 0.0.10全量初跑：106/106 Success，0 Fail。
7. receipt补强 `HitOrdinal == 0`；增量 Editor 4/4成功。
8. 最小维护 V2／Item测试；增量 Editor 5/5成功。
9. V2最终22/22、Item最终46/46。
10. coordinator最终10/10、EnemySkillFramework最终44/44、0.0.10全量最终106/106。
11. Game Development最终26/26，Succeeded，退出码0。
12. 最终增量 Editor 5/5，Succeeded，退出码0。
13. `git diff --check`、禁用 API、legacy branch范围与 authored definition扫描通过。

## 最终命令与结果

### Editor Build

```powershell
Build.bat demo_mapEditor Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA -NoUBTMakefiles
```

- 首次完整：26/26 actions；
- 最终增量：5/5 actions；
- `Result: Succeeded`；
- 原生退出码：0。

### CombatRunCoordinator 定向自动化

```text
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests Shanmen.0_0_10.Product.CombatRunCoordinator" -TestExit="Automation Test Queue Empty"
```

- performed：10；
- result：10 Success、0 Fail；
- queue empty；
- 原生退出码：0；
- 最终日志：`Saved/Logs/Dev.D.UE.0.0.10.P4.9.r0_combat_run_automation_final.log`；
- SHA-256：`B07205E3DF737AABD84F1442B102FE180BDBF79541886035980558909AACC65B`。

### 0.0.10 全量自动化

```text
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests Shanmen.0_0_10" -TestExit="Automation Test Queue Empty"
```

- performed：106；
- result：106 Success、0 Fail；
- queue empty；
- 原生退出码：0；
- 最终日志：`Saved/Logs/Dev.D.UE.0.0.10.P4.9.r0_full_automation_final.log`；
- SHA-256：`85406797F32B12A910A1BF4BDCE87411D84BFE64023548B779CDB46DC94089CD`。

### EnemySkillFramework 兼容回归

- performed：44；result：44 Success、0 Fail；queue empty；退出码0；
- 日志：`Saved/Logs/Dev.D.UE.0.0.10.P4.9.r0_enemy_skill_automation_final.log`；
- SHA-256：`15D1FF96E6AE11696401CCC86FCF5752EB30D97F7E43C2F2297C8DF6BC41DF66`。

### V2RangedCompatibility 兼容回归

- performed：22；result：22 Success、0 Fail；queue empty；退出码0；
- 日志：`Saved/Logs/Dev.D.UE.0.0.10.P4.9.r0_v2_ranged_automation_final.log`；
- SHA-256：`041D82A3FAE6EF2CB9DEEC646FAE6FC56838CADDC9311C0522A77C989EC183B6`。

### ItemUseAndArmor 兼容回归

- performed：46；result：46 Success、0 Fail；queue empty；退出码0；
- 日志：`Saved/Logs/Dev.D.UE.0.0.10.P4.9.r0_item_armor_automation_final.log`；
- SHA-256：`6AF5F64F90CE67C0079E481C8261AFD09E6694401D609834AD206B2D79D2485F`。

### Game Build

```powershell
Build.bat demo_map Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA -NoUBTMakefiles
```

- 26/26 actions；
- `Result: Succeeded`；
- 原生退出码：0。

### 静态检查

- `git diff --check`：退出码0；
- coordinator中 `ApplyDamage`／`TakeDamage`／`ApplyIncomingDamage`／随机 GUID／RNG／地址身份API：0匹配；
- shared projectile中保留1个`ApplyDamage`，仅位于`!bUsedCanonicalProduct` compatibility分支；
- M01 authored配置：3个StandardRanged，全部StandardRangedBackstep；
- final logs中 fatal／unhandled exception／handled ensure：0。

## 初始日志证据

- coordinator初跑10/10，SHA-256 `B0C5A06EFC90DD4FE2A1D1BDACE7DD3506F0CDCF0591B7A97E9799F52F1D5E40`；
- EnemySkillFramework初跑44/44，SHA-256 `7679B8ABF1616B3EECCF6E01ED08E721A4DA0C9A61E81FCF5B0D022CF89AD26B`；
- V2RangedCompatibility初跑20/22，SHA-256 `C734A422C3429166FA751861E0B136086372579494C36F1DE425A3A58474FF02`；
- ItemUseAndArmor初跑42/46，SHA-256 `848ADBBA0BA3583CFB34A92C9802B15572CE73106CEDE6CFB3F079EE1671748A`；
- 0.0.10全量初跑106/106，SHA-256 `C46695892B8FE1D1B29C23EFF1C46A2612854BFA7AC198AA27FE59783A938078`。

## 最终不变量

1. M01 StandardRanged projectile hostile contact只有canonical vitality写入；产品失败不fallback。
2. 非M01 ranged、player projectile与M01 Boss projectile保留legacy路径。
3. 只有authored StandardRangedBackstep可调用StandardRangedProjectile family。
4. sequence在成功flight前保留，spawn失败不消耗。
5. identity只依赖RunId、稳定source EntityId、冻结action、sequence、detector、target与ordinal。
6. invalid profile／sequence／contact／binding在mutation前拒绝。
7. projectile candidate保留真实contact geometry，resolver不查询World。
8. receipt要求Projectile detector kind与HitOrdinal 0。
9. resolver保持raw = prevented + final；fractional damage不截断。
10. exact receipt replay不重复生命、revision、ledger或broadcast。
11. same identity／different snapshot被拒绝，不能伪装成replay。
12. old-Run receipt不能写入新Run。
13. canonical成功或失败都只消费当前非穿透projectile一次。

## 诊断与边界

- V2／Item首次6条失败均已保留并准确归因为遗留测试漂移；没有伪装为源码失败或环境故障。
- 五份最终日志各有13条UE测试发现阶段既有负向自检`Condition failed`；目标测试全部通过。
- Win64 SDK有效；非目标LinuxArm64／VisionOS SDK metadata警告不影响Win64构建。
- 未启动Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、Cook或Package。
- 未修改或提交工作区中的无关长期未跟踪文件。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p4-9-ranged-projectile-product/Docs/Report/Dev.D.UE.0.0.10.P4.9.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p4-9-ranged-projectile-product/Docs/Log/Dev.D.UE.0.0.10.P4.9.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p4-9-ranged-projectile-product>
