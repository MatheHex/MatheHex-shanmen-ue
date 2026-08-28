# Dev.D.UE.0.0.10.P4.11.r0 Development Log

## 目标

把M01 Boss现有Sweep、Charge与三发Volley的合法世界接触接入0.0.10 canonical combat。保留既有Actor几何／LOS／faction／telegraph／movement／recovery／projectile语义，使用windup前预留的Run-local sequence、按攻击形态冻结的action spec、deterministic identity、玩家defense snapshot与exactly-once vitality ledger。M01产品失败必须关闭且不得fallback；非M01 compatibility继续存在。

## 基线

- 基线分支：`agent/0.0.10-p4-10-heavy-attack-product`；
- 基线提交：`0183f1abb29df50f2ffa2aef7374350ab7d97d90`；
- 当前分支：`agent/0.0.10-p4-11-boss-attack-product`；
- 基线coordinator定向：`11/11`；
- 基线0.0.10全量：`107/107`；
- P4.10结束时Boss Sweep／Charge仍直接调用`ApplyDamage`，Volley使用无canonical identity的target-only共享Projectile。

## 审计结果

1. Boss只有一个authored `BossMain` definition和一个独立Actor class，SkillProfile为空。
2. 距离planner选择Sweep、Charge或Volley；三种攻击共享一个windup／resolve／recovery状态机。
3. Sweep使用sector radius `340`、full angle `118`、vertical tolerance `180`与WorldStatic LOS。
4. Charge位移`620`，resolve后以`190`接触半径授权伤害。
5. Volley固定生成`-11/0/11`度三发弹体；三发属于同一次Boss action，不应各自生成独立Activation。
6. 共享Projectile已有ranged canonical route，但legacy Boss初始化会把profile与sequence清零。
7. 原receipt把`HitOrdinal == 0`写死，无法表达同一Activation的三个合法Impact。

## 设计决定

### 三种family，不拆第二套resolver

Sweep、Charge、Volley分别冻结Action／Detector／Formula，但继续复用现有`Fdemo_mapM01EnemyAttackImpactReceipt`、`FShanmenDefenseResolver`和玩家vitality ledger。spec新增`MaxHitOrdinal`：已有family保持0，Boss Volley为2。

### 全局Boss sequence，Volley用ordinal分叉

Boss在进入windup前预留一个`uint64` sequence。Sweep／Charge使用ordinal 0；Volley的三发共享sequence与Activation，以ordinal 0、1、2派生独立Impact。取消会消费已经开始的action序号，避免延迟回调复用身份。

### M01 gate内fail-closed

Boss Actor和共享Projectile只有在GameMode未声明M01产品所有权时才走legacy `ApplyDamage`。gate为真后，即使source binding、sequence、ordinal、contact、snapshot或delivery失败，也只记录canonical错误，不执行第二次伤害。

### 回归按改动文件推导

- coordinator与其测试：coordinator定向 + 0.0.10全量；
- Boss Actor：M01 Enemy + coordinator + 0.0.10全量；
- shared Projectile：EnemySkillFramework + V2RangedCompatibility + 0.0.10全量；
- GameMode：0.0.10全量；
- 未修改Profile／CodeB／Items路径，因此不追加无映射依据的存档／物品组。

## 实现过程

1. 扩展enemy attack family为`BossSweep`、`BossCharge`、`BossVolleyProjectile`。
2. 为spec增加`MaxHitOrdinal`并让receipt按spec校验ordinal范围；已有family继续只能为0。
3. 新增Boss shape与Boss volley两个公开coordinator入口，构造identity前拒绝zero／exhausted sequence、非法attack、非法ordinal与NaN contact。
4. 通用执行器新增requested ordinal，并要求Boss family同时满足空SkillProfile和Boss Actor类型。
5. Boss Actor新增next／active sequence；configuration与新Run注册重置，cancel／resolve／recovery清理active值。
6. Sweep与Charge在原世界授权成功后进入GameMode/coordinator；canonical与compatibility分支互斥。
7. shared Projectile新增Boss初始化入口和ordinal元数据；所有其它初始化器显式清理Boss identity，避免复用污染。
8. Volley三发继承同一Boss sequence与固定ordinal；M01 gate内malformed metadata失败关闭。
9. GameMode新增Boss shape／volley桥和结构化family／sequence／ordinal／action／impact／commit日志。
10. 新增`M01BossAttackProduct`自动化，覆盖身份、边界、三发复合Impact、replay、Run隔离与lethal clamp。
11. 执行Editor构建、路径映射回归、静态检查与Game构建。
12. 生成Report与Development Log；首次测试均在最终源码状态通过，因此不做重复复跑。

## 验证时间线

1. `git diff --check`初检：退出码`0`。
2. Editor Development：`25/25`，Succeeded，退出码`0`。
3. CombatRunCoordinator：`12/12 Success`，`0 Fail`，退出码`0`。
4. M01 Enemy：`3/3 Success`，`0 Fail`，退出码`0`。
5. EnemySkillFramework：`44/44 Success`，`0 Fail`，退出码`0`。
6. V2RangedCompatibility：`22/22 Success`，`0 Fail`，退出码`0`。
7. 0.0.10全量：`108/108 Success`，`0 Fail`，退出码`0`。
8. 唯一写入点、随机身份、模块边界与compatibility fallback扫描通过。
9. Game Development：`24/24`，Succeeded，退出码`0`。
10. `git diff --check`终检：退出码`0`。

本轮没有源码、测试基线或环境失败；不存在首次失败日志。所有Automation首轮日志均保留。

## 命令与结果

### Editor Build

```powershell
Build.bat demo_mapEditor Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- `25/25` actions；
- `Result: Succeeded`；
- 原生退出码：`0`；
- 总执行时间：`119.81s`。

### CombatRunCoordinator

```text
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests Shanmen.0_0_10.Product.CombatRunCoordinator" -TestExit="Automation Test Queue Empty"
```

- `12/12 Success`、`0 Fail`、queue empty、退出码`0`；
- 日志：`Saved/Logs/Dev.D.UE.0.0.10.P4.11.r0_combat_run_automation_initial.log`；
- SHA-256：`90478A52A888138731AA6A52F60F7AA5B0377A7C5A4BFA73E7B0724863D9667B`。

### M01 Enemy

- 命令组：`Automation RunTests demo_map.M01.Enemy`；
- `3/3 Success`、`0 Fail`、queue empty、退出码`0`；
- 日志：`Saved/Logs/Dev.D.UE.0.0.10.P4.11.r0_m01_enemy_automation_initial.log`；
- SHA-256：`745011BEBF3B6BC19E62E4FB5D4AAC67FC3657940B0FC1A2F00A93FE089EE787`。

### EnemySkillFramework

- 命令组：`Automation RunTests demo_map.EnemySkillFramework`；
- `44/44 Success`、`0 Fail`、queue empty、退出码`0`；
- 日志：`Saved/Logs/Dev.D.UE.0.0.10.P4.11.r0_enemy_skill_automation_initial.log`；
- SHA-256：`6EEFF790A802579E1D37A716EE0B8029F01C6C31BBB065B1FCFC793B1399B87C`。

### V2RangedCompatibility

- 命令组：`Automation RunTests demo_map.V2RangedCompatibility`；
- `22/22 Success`、`0 Fail`、queue empty、退出码`0`；
- 日志：`Saved/Logs/Dev.D.UE.0.0.10.P4.11.r0_v2_ranged_automation_initial.log`；
- SHA-256：`50D799BB062ED52A9818EF3B80F4C8218F428DCF5A1C927132F32AAF55991A0B`。

### 0.0.10全量

- 命令组：`Automation RunTests Shanmen.0_0_10`；
- `108/108 Success`、`0 Fail`、queue empty、退出码`0`；
- 日志：`Saved/Logs/Dev.D.UE.0.0.10.P4.11.r0_full_automation_initial.log`；
- SHA-256：`20D35E4D293C4ACEC4D8F9398FBA26F02D35784D57E86117875454104CBCBBA3`。

### Game Build

```powershell
Build.bat demo_map Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- `24/24` actions；
- `Result: Succeeded`；
- 原生退出码：`0`；
- 总执行时间：`98.49s`。

### 静态检查

- `git diff --check`：退出码`0`；
- coordinator中`ApplyDamage`／`NewGuid`／RNG／地址身份API：`0`匹配；
- `ShanmenCombatRuntime`与`ShanmenCombatCore`中`demo_map`／`UWorld`／`AActor`／`ApplyDamage`／随机API：`0`匹配；
- Boss Actor：`ApplyDamage`共`1`处，只在非M01 compatibility lambda；
- shared Projectile：`ApplyDamage`共`1`处，只在`!bUsedCanonicalProduct`分支；
- 五份日志中fatal／unhandled exception／handled ensure均为`0`。

## 最终不变量

1. M01 Boss三种攻击的合法接触只有canonical vitality写入；产品失败不fallback。
2. 非M01 compatibility保留legacy路径。
3. world geometry、LOS、faction、timing与projectile flight留在Actor层；resolver不查询World。
4. action sequence在windup前预留；取消会消费序号；resolve时不重新编号。
5. Sweep、Charge与Volley使用不同冻结Action；三发Volley共享一个Activation。
6. Volley只有ordinal `0/1/2`合法，三个ImpactId互不相同。
7. identity只依赖RunId、稳定source EntityId、冻结action、sequence、detector、target与ordinal。
8. invalid sequence／family／source／ordinal／contact在mutation前拒绝。
9. exact receipt replay不重复生命、revision、ledger或broadcast。
10. same identity／different snapshot被拒绝，不能伪装成replay。
11. old-Run receipt不能写入新Run；新Run重置Boss sequence并清理旧pending projectile。
12. 既有enemy attack family的ordinal上限仍为0，未因Volley放宽。
13. 回归组由实际改动路径推导，所有映射组均有本轮日志证据。

## 诊断与边界

- 五份Automation日志各有13条UE测试发现阶段既有负向自检`Condition failed`；目标测试全部通过。
- Win64 SDK有效；非目标LinuxArm64／VisionOS SDK metadata警告不影响Win64构建。
- 未启动Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、Cook或Package。
- 未修改或提交工作区中的无关长期未跟踪文件。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p4-11-boss-attack-product/Docs/Report/Dev.D.UE.0.0.10.P4.11.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p4-11-boss-attack-product/Docs/Log/Dev.D.UE.0.0.10.P4.11.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p4-11-boss-attack-product>
