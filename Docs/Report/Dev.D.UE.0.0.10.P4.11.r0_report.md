# Dev.D.UE.0.0.10.P4.11.r0 开发报告

## 结论

`PASS`。P4.11 已把 M01 Boss 的全部三种真实攻击形态接入 0.0.10 canonical combat：Sweep、Charge 与三发 Volley。Boss Actor继续拥有距离决策、锁定方向、扇形／冲锋几何、LOS、阵营过滤、windup、recovery与弹体飞行；合法接触之后由 CombatRunCoordinator冻结身份、捕获玩家 defense、调用纯函数 resolver，并只通过 vitality ledger提交生命变化。

Sweep与Charge分别使用独立 action family；同一次 Volley的三发弹体共享一个 ActivationId，以固定 ordinal `0/1/2`派生三个不同 ImpactId。M01产品 gate一旦声明所有权，canonical失败不会回落 legacy `ApplyDamage`；非M01 compatibility路径保持不变。

## 功能性

### 1. 三个冻结 Boss family

- Sweep：`Combat.Action.Enemy.Boss.Sweep`、`Detector.Enemy.Boss.Sweep`、`Combat.Formula.Enemy.Boss.Sweep.r1`；
- Charge：`Combat.Action.Enemy.Boss.Charge`、`Detector.Enemy.Boss.Charge`、`Combat.Formula.Enemy.Boss.Charge.r1`；
- Volley：`Combat.Action.Enemy.Boss.Volley`、`Detector.Enemy.Boss.Volley.Projectile`、`Combat.Formula.Enemy.Boss.Volley.r1`；
- 三者 content version均为`0.0.10.P4.11`；
- digest分别为`Shanmen.M01Boss.Sweep.r1`、`Shanmen.M01Boss.Charge.r1`、`Shanmen.M01Boss.Volley.r1`；
- Sweep／Charge detector kind为`Shape`，只允许 ordinal `0`；Volley为`Projectile`，只允许 ordinal `0..2`；
- family binding同时要求空 SkillProfile与`Ademo_mapM01BossCharacter`类型，空profile的Heavy Actor和其它敌人不能借用Boss入口。

### 2. Run-local action identity

- Boss新增独立`NextAttackSequence`／`ActiveAttackSequence`，没有复用Sweep／Charge／Volley resolve诊断计数；
- sequence在有效方向确认后、进入windup前预留，三个Volley弹体继承同一个sequence；
- zero与`MAX_uint64`失败关闭；耗尽时进入攻击冷却退避，避免每个AI tick持续重试；
- encounter configuration与M01新Run首次注册都把sequence重置为`1`，并取消旧Run的pending attack和存活弹体；
- RunId、稳定Boss EntityId、冻结ActionDefinitionId与sequence确定性派生ActivationId；
- ActivationId、detector、稳定player EntityId与ordinal确定性派生ImpactId；身份不依赖对象地址、帧号、计时器时间或随机数。

### 3. 世界授权与唯一生命写入

- Sweep继续使用既有340半径、118度全角、180垂直容差和WorldStatic LOS；
- Charge继续执行620位移与190接触范围；
- Volley继续生成`-11/0/11`度三发target-only共享弹体；
- Boss Actor与共享Projectile只向coordinator传递已经发生的合法世界接触；resolver不查询World；
- coordinator复核active Run、source注册、Boss authored binding、source vitality、target Pawn、ordinal与contact有限值；
- 玩家生命最终只由`CommitCombatImpact` ledger写入；
- Boss Actor与共享Projectile文件各保留恰好一个`UGameplayStatics::ApplyDamage`，均只位于非M01 compatibility分支。

### 4. Volley复合身份与fail-closed

- 三发Volley使用相同Action／Activation，但ordinal `0/1/2`各自形成独立Impact；
- ordinal小于0或大于2在构造identity之前拒绝；NaN contact同样在mutation前拒绝；
- exact receipt replay返回`AlreadyCommitted`，不重复扣血、revision、ledger或damage broadcast；
- 相同sequence与ordinal重新构造但携带不同authority snapshot时，ledger以`CommitRejected`失败关闭；
- 旧Run receipt以`RunMismatch`拒绝；新Run sequence `1`因RunId变化产生新身份；
- malformed Boss projectile在M01 gate内仍由canonical入口失败关闭，不会落回legacy damage。

## 改动文件驱动的回归范围

本轮先以`git diff --name-only`取得生产与测试改动，再按路径映射必跑组：

| 改动路径 | 必跑测试组 | 结果 |
| --- | --- | --- |
| `demo_mapCombatRunCoordinator*` | `Shanmen.0_0_10.Product.CombatRunCoordinator`、`Shanmen.0_0_10` | `12/12`、`108/108` |
| `demo_mapM01BossCharacter*` | `demo_map.M01.Enemy`、coordinator、0.0.10全量 | `3/3`、`12/12`、`108/108` |
| `demo_mapSkillProjectile*` | `demo_map.EnemySkillFramework`、`demo_map.V2RangedCompatibility`、0.0.10全量 | `44/44`、`22/22`、`108/108` |
| `demo_mapGameMode*` | 0.0.10全量 | `108/108` |

本轮没有修改Profile、ItemEconomy、CodeB或ShanmenItems路径，因此没有用“战斗主题”臆测追加无对应改动的存档／物品组。所有映射到的测试组均出现在本轮Automation日志中。

## 自动化证据

- CombatRunCoordinator：`12/12 Success`、`0 Fail`、queue empty、原生退出码`0`；
- M01 Enemy：`3/3 Success`、`0 Fail`、queue empty、原生退出码`0`；
- EnemySkillFramework：`44/44 Success`、`0 Fail`、queue empty、原生退出码`0`；
- V2RangedCompatibility：`22/22 Success`、`0 Fail`、queue empty、原生退出码`0`；
- 0.0.10全量：`108/108 Success`、`0 Fail`、queue empty、原生退出码`0`。

新增`Shanmen.0_0_10.Product.CombatRunCoordinator.M01BossAttackProduct`，覆盖Boss类型绑定、zero／exhausted sequence、非法attack、ordinal边界、NaN contact、三套冻结spec、Sweep replay、Charge独立身份、Volley共享Activation与三个Impact、same-ordinal reconstruction reject、旧Run隔离、新Run重置与lethal clamp。

所有自动化均在最终源码状态上首次通过；测试后未再修改源码，因此保留`initial`日志作为最终证据，没有做只为改文件名或重复数字的复跑。日志SHA-256：

- `Saved/Logs/Dev.D.UE.0.0.10.P4.11.r0_combat_run_automation_initial.log`：`90478A52A888138731AA6A52F60F7AA5B0377A7C5A4BFA73E7B0724863D9667B`；
- `Saved/Logs/Dev.D.UE.0.0.10.P4.11.r0_m01_enemy_automation_initial.log`：`745011BEBF3B6BC19E62E4FB5D4AAC67FC3657940B0FC1A2F00A93FE089EE787`；
- `Saved/Logs/Dev.D.UE.0.0.10.P4.11.r0_enemy_skill_automation_initial.log`：`6EEFF790A802579E1D37A716EE0B8029F01C6C31BBB065B1FCFC793B1399B87C`；
- `Saved/Logs/Dev.D.UE.0.0.10.P4.11.r0_v2_ranged_automation_initial.log`：`50D799BB062ED52A9818EF3B80F4C8218F428DCF5A1C927132F32AAF55991A0B`；
- `Saved/Logs/Dev.D.UE.0.0.10.P4.11.r0_full_automation_initial.log`：`20D35E4D293C4ACEC4D8F9398FBA26F02D35784D57E86117875454104CBCBBA3`。

五份日志各包含UE 5.8测试发现阶段既有的13条`Condition failed`负向自检诊断；目标测试全部成功，fatal、unhandled exception与handled ensure均为`0`。

## 构建与静态检查

- Editor Development：`25/25` actions，`Result: Succeeded`，原生退出码`0`；
- Game Development：`24/24` actions，`Result: Succeeded`，原生退出码`0`；
- `git diff --check`：原生退出码`0`；
- coordinator对legacy damage、随机GUID／RNG与地址身份API扫描：`0`匹配；
- `ShanmenCombatRuntime`／`ShanmenCombatCore`对`demo_map`、`UWorld`、`AActor`、`ApplyDamage`与随机API扫描：`0`匹配；
- Boss Actor与共享Projectile各有`1`个compatibility `ApplyDamage`，canonical coordinator内为`0`；
- 未发生Windows commit-memory／页面文件错误，也没有源码、测试基线或环境失败。

## 修改范围

- `Source/demo_map/demo_mapCombatRunCoordinator.h/.cpp`
- `Source/demo_map/demo_mapCombatRunCoordinatorTests.cpp`
- `Source/demo_map/demo_mapGameMode.h/.cpp`
- `Source/demo_map/demo_mapM01BossCharacter.h/.cpp`
- `Source/demo_map/demo_mapSkillProjectile.h/.cpp`
- 本Report与同名Development Log

未修改、删除或提交工作区中的无关长期未跟踪文件。

## 兼容性与剩余边界

- P4.7 ordinary melee、P4.8 melee dash、P4.9 ranged projectile与P4.10 heavy sector继续复用同一enemy attack receipt、resolver与vitality ledger；没有建立第二套Boss结算系统；
- 非M01 Boss与Projectile保留legacy compatibility路径；
- Boss原有AI选择、世界几何、移动、时序、VFX、projectile参数与resolve计数未改写；
- Actor层真实世界接线已完成静态与NullRHI回归；真实计时、碰撞、视觉和三发弹体命中顺序验收属于F阶段。

## P/F 边界

本Report只包含P阶段源码开发、代码审查、静态扫描、NullRHI headless Automation、Editor Development与Game Development构建。未启动Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、Cook或Package。

## 下一步

P4.12建议对`Source/demo_map`剩余真实敌对`ApplyDamage`入口做改动文件与Actor family双向审计，列出尚未迁移的产品攻击，再选择一个完整family接入；不得仅按文件名批量替换或把不同身份语义压入同一action。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p4-11-boss-attack-product/Docs/Report/Dev.D.UE.0.0.10.P4.11.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p4-11-boss-attack-product/Docs/Log/Dev.D.UE.0.0.10.P4.11.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p4-11-boss-attack-product>
