# Dev.D.UE.0.0.10.P4.13.r0 Development Log

## 目标

把玩家 Straight Projectile 的发射与 hostile contact 接入 0.0.10 canonical combat。保留共享 projectile 的 launch-segment、飞行、碰撞、WorldStatic、range/lifespan 与非 M01 compatibility 语义；为 M01 玩家 projectile 冻结独立 action identity、使用 projectile world-hit adapter、纯函数 impact resolve 与 vitality ledger exactly-once delivery。产品失败不得 fallback。

## 基线

- 基线分支：`agent/0.0.10-p4-12-player-shape-skills`；
- 基线提交：`5b87ac7dacbfa3bc4d34a2979e89bfeea16ef897`；
- 当前分支：`agent/0.0.10-p4-13-player-projectile-product`；
- 基线 coordinator 定向：`14/14`；
- 基线 0.0.10 全量：`110/110`；
- P4.12 结束时 M01 GroundCircle 与 SelfSector 已 canonical，Straight Projectile 仍落入 shared projectile 的 legacy `ApplyDamage`。

## 审计结果

1. `Udemo_mapSkillComponent::SpawnProjectile` 负责玩家 damage snapshot、spawn、launch-segment 与 cooldown；
2. `Ademo_mapSkillProjectile` 同时服务玩家、Ranged enemy 与 Boss volley；
3. Boss 与 Ranged 已有独立 canonical 分支，不能因玩家迁移改变；
4. shared projectile 的 hostile contact 最终统一经过一个 legacy `ApplyDamage` fallback；
5. 玩家 projectile 是发射／飞行／命中生命周期，不得借用 P4.12 overlap shape identity；
6. constructor 先将 collision 设为 `NoCollision`，适合在启动碰撞前注入 canonical metadata；
7. launch-segment 可能立即命中，因此 sequence 与 ActivationId 必须在 sweep 前就绪。

## 冻结契约

- ActionDefinitionId：`Combat.Action.Player.Skill.StraightProjectile`；
- DetectorId：`Detector.Player.Skill.StraightProjectile.Projectile`；
- FormulaId：`Combat.Formula.Player.Skill.StraightProjectile.r1`；
- FormulaVersion：`0.0.10.P4.13`；
- FormulaDigest：`Shanmen.Player.Skill.StraightProjectile.Projectile.r1`；
- SourceTags：Player；
- DamageTags：Physical；
- RequiredTargetTags：Living；
- DetectorTags：Projectile；
- HitOrdinal：`0`；
- sequence：独立 Run-local counter，Run end/reset 后回到 `1`。

## 设计决定

### spawn 后、collision 前分配身份

先创建 actor，成功后再调用 coordinator prepare。这样 spawn 失败不消费 sequence；prepare 成功后才把 sequence、ActivationId 与 damage snapshot 写入 projectile，然后执行 launch-segment 并启动 flight。Prepare 失败销毁 actor并返回 `nullptr`，技能 cooldown 不提交。

### 精确 source ownership

GameMode 只对 M01 中当前 live local player pawn 声明玩家 projectile 产品所有权。该 ownership 与 coordinator readiness 分离：即使 coordinator 不可用，M01 source 仍进入 canonical 分支并 fail-closed，不会因为 readiness 失败落回 legacy damage。

### contact 后不 fallback

shared projectile 在进入 Boss、Ranged 或 player canonical 分支后统一设置 `bUsedCanonicalProduct`。只有该值为 false 才能执行唯一 legacy `ApplyDamage`。M01 玩家 metadata 缺失或损坏时仍被 canonical ownership 捕获，mutation 前拒绝并消费本次 projectile。

### WorldGameplay adapter

Coordinator 不自行读取碰撞世界，也不生成地址身份；它接收 actor/component/location/normal evidence，经 `FShanmenWorldHitAdapter::TryFromProjectile` 形成稳定 hit context，再交给 action orchestrator、pure resolver 与 vitality ledger。

## 实现过程

1. 新增 player projectile launch/impact error、result 与 receipt 类型。
2. 冻结 Straight Projectile action/detector/formula/version/digest/tags。
3. 新增独立 `NextPlayerStraightProjectileActivationSequence` 并接入 Run 生命周期。
4. 新增 `PreparePlayerStraightProjectile`，完成输入校验、action capture 与延迟 sequence reservation。
5. 新增 `ExecutePlayerStraightProjectileImpact`，重建 action、验证 ActivationId、转换 projectile hit、resolve 与 deliver。
6. 抽出通用 player impact receipt delivery，让 shape 与 projectile 复用同一组 Run/source/target/vitality ledger 校验。
7. GameMode 新增 exact M01 player projectile ownership gate、prepare bridge、impact bridge 与结构化日志。
8. SkillComponent 在 actor 成功 spawn 后 prepare canonical identity；失败销毁 actor并保持 cooldown 未提交。
9. SkillProjectile 新增 canonical player initializer，在 launch-segment 前写入 metadata；其它 initializer 显式清空该 metadata。
10. hostile contact 增加 player canonical branch；所有 canonical 分支继续与唯一 compatibility fallback 互斥。
11. 新增 `PlayerStraightProjectileProduct` 与 `PlayerStraightProjectileFailClosed` 两个无头测试。
12. 按实际改动路径执行 coordinator、0.0.10 全量、EnemySkillFramework 与 V2RangedCompatibility。

## 验证时间线

1. `git diff --check` 初检：退出码 `0`。
2. Editor Development：`25/25`，Succeeded，退出码 `0`。
3. CombatRunCoordinator：`16/16 Success`、`0 Fail`，queue empty，退出码 `0`。
4. 0.0.10 全量：`112/112 Success`、`0 Fail`，queue empty，退出码 `0`。
5. EnemySkillFramework：`44/44 Success`、`0 Fail`，queue empty，退出码 `0`。
6. V2RangedCompatibility：`22/22 Success`、`0 Fail`，queue empty，退出码 `0`。
7. 唯一 fallback、deterministic identity、模块边界与 world adapter 扫描通过。
8. Game Development：`24/24`，Succeeded，退出码 `0`。
9. `git diff --check` 终检：退出码 `0`。

生产源码和测试均在首次构建／首次目标运行通过，没有失败日志或修复后重复构建。

## 命令与结果

### Editor Build

```powershell
Build.bat demo_mapEditor Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- `25/25` actions；
- `Result: Succeeded`；
- 原生退出码：`0`；
- UBA 时间：`123.30s`；
- 总执行时间：`126.49s`。

### CombatRunCoordinator

```text
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests Shanmen.0_0_10.Product.CombatRunCoordinator" -TestExit="Automation Test Queue Empty"
```

- `16/16 Success`、`0 Fail`、queue empty、原生退出码 `0`；
- 日志：`Saved/Logs/Dev.D.UE.0.0.10.P4.13.r0_combat_run_automation_initial.log`；
- SHA-256：`0E5F6972833EEA62A39374FFF9E4BD9344351DCEC1BF89020D397D4B502F51F7`。

### 0.0.10 全量

- 命令组：`Automation RunTests Shanmen.0_0_10`；
- `112/112 Success`、`0 Fail`、queue empty、原生退出码 `0`；
- 日志：`Saved/Logs/Dev.D.UE.0.0.10.P4.13.r0_full_automation_initial.log`；
- SHA-256：`5F9AFE923B8905587EDB939DE81C41D8613031DC4C4061DDDF6AD92031C7F917`。

### EnemySkillFramework

- 命令组：`Automation RunTests demo_map.EnemySkillFramework`；
- `44/44 Success`、`0 Fail`、queue empty、原生退出码 `0`；
- 日志：`Saved/Logs/Dev.D.UE.0.0.10.P4.13.r0_enemy_skill_automation_initial.log`；
- SHA-256：`A4ED110C42A2E2766415FA44CA9BA03AD8D45B7A52DACD69916B26B416FB3807`。

### V2RangedCompatibility

- 命令组：`Automation RunTests demo_map.V2RangedCompatibility`；
- `22/22 Success`、`0 Fail`、queue empty、原生退出码 `0`；
- 日志：`Saved/Logs/Dev.D.UE.0.0.10.P4.13.r0_v2_ranged_automation_initial.log`；
- SHA-256：`6C397F44D1906837C80ACA5F2B7BFA31AAB675A1DE54F2E335FF7E4AE3DDEE81`。

### Game Build

```powershell
Build.bat demo_map Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- `24/24` actions；
- `Result: Succeeded`；
- 原生退出码：`0`；
- UBA 时间：`90.98s`；
- 总执行时间：`93.20s`。

### 静态检查

- `git diff --check`：退出码 `0`；
- coordinator 中 `ApplyDamage`／`NewGuid`／RNG／地址身份 API：`0` 匹配；
- `ShanmenCombatCore`、`ShanmenCombatRuntime` 中 `demo_map` include／`UWorld`／`AActor`／`ApplyDamage`／随机或地址身份 API：`0` 匹配；
- `ShanmenItems` 中 `demo_map` include／`UWorld`／`AActor`／`ApplyDamage`／运行时随机或地址身份 API：`0` 匹配；
- shared projectile：`ApplyDamage` 共 `1` 处，只在 `!bUsedCanonicalProduct`；
- SkillComponent：`ApplyDamage` 共 `2` 处，只在 `!bUseM01ProductPath`；
- 四份日志 fatal／unhandled exception／handled ensure：均为 `0`。

## 最终不变量

1. M01 玩家 Straight Projectile hostile contact 只有 canonical vitality 写入；失败不 fallback。
2. 非 M01 shared projectile compatibility 保留唯一 legacy damage 路径。
3. Boss volley 与 Ranged enemy canonical 分支不变。
4. actor spawn 失败不消费 sequence；prepare 失败不提交 cooldown。
5. canonical metadata 在 launch-segment sweep 前完整可用。
6. player projectile 使用独立 action、detector、formula、digest、sequence 与固定 ordinal `0`。
7. identity 只依赖 RunId、稳定 source/target EntityId、冻结 action、sequence、detector 与 ordinal。
8. invalid source／damage／sequence／ActivationId／contact／target 在 mutation 前拒绝。
9. exact receipt replay 不重复 vitality、revision、ledger 或 broadcast。
10. old-Run receipt 不能写入新 Run；新 Run sequence 重置为 `1`。
11. world geometry 留在 SkillProjectile；coordinator 只消费 WorldGameplay adapter 证据。
12. launch sweep、friendly pass-through、WorldStatic consumption、speed/range/lifespan 保持原语义。
13. 回归范围由改动文件推导，所有映射组均有本轮日志。

## 诊断与边界

- 四份 Automation 日志各有 UE 测试发现阶段既有的 `13` 条负向自检 `Condition failed`；最终目标测试全部 Success。
- Win64 SDK 有效；未发生源码、commit-memory、页面文件或工具链错误。
- 未启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、Cook 或 Package。
- 未修改或提交工作区中的无关长期未跟踪文件。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p4-13-player-projectile-product/Docs/Report/Dev.D.UE.0.0.10.P4.13.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p4-13-player-projectile-product/Docs/Log/Dev.D.UE.0.0.10.P4.13.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p4-13-player-projectile-product>
