# Dev.D.UE.0.0.10.P4.9.r0 开发报告

## 结论

`PASS`。P4.9 已把 M01 三个 authored `StandardRanged` 敌人的非穿透、目标锁定 projectile 首次合法 hostile contact 接入 0.0.10 canonical combat：Actor 在发射前保留 Run-local 单调 projectile sequence，接触时把稳定 profile／sequence／contact geometry 交给 coordinator，确定性派生 ActionId／ImpactId，捕获玩家 defense snapshot，经纯函数 resolver 结算后只通过 vitality ledger 提交一次生命变化。

M01 projectile 一旦进入产品路由，即使 coordinator 未 ready、profile／sequence／contact 无效、source binding 不匹配或 delivery 被拒绝，也会消费并销毁当前 projectile，绝不回落 `ApplyDamage`。非 M01 地图的 ranged projectile、M01 Boss projectile与共享 player projectile继续保留原兼容路径。

## 功能性

### 1. 冻结 StandardRanged projectile family

- Action：`Combat.Action.Enemy.Projectile.StandardRanged`；
- detector：`Detector.Enemy.Projectile.Contact`；
- detector kind：`Projectile`；
- formula：`Combat.Formula.Enemy.Projectile.StandardRanged.r1`；
- content version：`0.0.10.P4.9`；
- content digest：`Shanmen.M01Enemy.StandardRanged.Projectile.r1`；
- M01 配置静态审计确认恰有 `3` 个 `StandardRanged` 定义，三者均绑定 `StandardRangedBackstep`；强化 ranged profile 不会被误接入本 family；
- receipt 校验 family、action、content、detector、detector kind、formula、damage／target tags、守恒以及 `HitOrdinal == 0`。

### 2. projectile identity 与接触边界

- `Ademo_mapRangedEnemyCharacter` 持有从 `1` 开始的 `uint64` projectile sequence；
- sequence 只在 projectile 成功 spawn 后、调用 `ActivateForFlight` 前保留并递增，spawn 失败不消耗序号；
- sequence 在新 Run／显式 encounter configuration 边界重置，不依赖帧号、时间、碰撞顺序、对象地址或随机 GUID；
- RunId、稳定 source EntityId、冻结 ActionDefinitionId 与 sequence 确定性派生 ActivationId；
- ActivationId、detector、稳定 player EntityId 与固定 ordinal `0` 确定性派生 ImpactId；
- projectile contact location／normal 由真实 world contact adapter 输入 canonical candidate，不由 resolver 查询 World；NaN contact 在 identity 与 mutation 前拒绝。

### 3. 唯一生命写入与失败关闭

- shared projectile 只在 `Ademo_mapRangedEnemyCharacter` 来源且当前地图为 M01 时进入 canonical 产品入口；
- coordinator 重新验证当前 Run、source／target Registry identity、authored binding、source vitality、profile family与玩家 binding；
- 玩家当前防御继续复用 P4.7/P4.8 的 defense snapshot 与 resolver；最终生命变化只由玩家 `CommitCombatImpact` ledger 写入；
- canonical 成功、replay 或失败都会消费当前非穿透 projectile；canonical 分支没有 legacy damage fallback；
- shared projectile 文件中只保留 `1` 个 `UGameplayStatics::ApplyDamage`，位于明确的 `!bUsedCanonicalProduct` compatibility 分支；
- Boss 使用独立 Actor 类型与既有 initializer，非 M01 地图 gate 为 false，因此两者不受本轮迁移影响。

### 4. exactly-once 与 Run 隔离

- 首次 `1.0` raw damage 经 `0.25` flat defense 得到 `0.25` prevented、`0.75` final／applied；fractional damage不再被 legacy 整数入口截断；
- 完整 receipt 重放返回 `AlreadyCommitted`，不重复扣血、revision、ledger 或 damage broadcast；
- 使用同一 sequence 重新捕获已变化的 authority revision，会得到相同 ImpactId 但不同 snapshot，ledger 以 `CommitRejected` 失败关闭；
- sequence `2` 产生独立 action／impact；
- 旧 Run receipt 以 `RunMismatch` 拒绝；新 Run sequence `1` 仍因 RunId 不同而派生新身份；
- 致死 damage 按剩余 vitality clamp，receipt记录真实 applied damage。

## 自动化证据

### CombatRunCoordinator 定向

新增 `Shanmen.0_0_10.Product.CombatRunCoordinator.M01EnemyRangedProjectileProduct`，使 coordinator 定向套件从 9 条增至 10 条，覆盖：

- enhanced profile、zero sequence、NaN contact 与 melee-source/ranged-family mismatch 在 mutation 前拒绝；
- exact action／detector／kind／formula／content／contact geometry；
- fractional defense、守恒与首次 commit；
- exact replay、same-sequence reconstruction reject与 distinct next sequence；
- old-Run receipt rejection、新 Run identity与 lethal clamp。

最终：`10/10 Success`、`0 Fail`、queue empty，原生退出码 `0`。

### 0.0.10 全量

最终：`106/106 Success`、`0 Fail`、queue empty，原生退出码 `0`。

### 兼容回归

- EnemySkillFramework：`44/44 Success`、`0 Fail`；
- V2RangedCompatibility：`22/22 Success`、`0 Fail`；
- ItemUseAndArmor：`46/46 Success`、`0 Fail`。

V2 与 ItemUseAndArmor 初跑暴露的是遗留测试基线漂移：测试仍硬编码 Profile Schema `4`，而当前产品基线为 `7`；同时 P73.4 已把纳物戒从 `AccessorySlot` 分离为 `SpatialRingSlot`。本轮只更新测试到已经存在的产品事实，未修改 Profile schema、迁移代码、item definition、slot authority或 equipment resolver。

最终日志 SHA-256：

- `Saved/Logs/Dev.D.UE.0.0.10.P4.9.r0_combat_run_automation_final.log`：`B07205E3DF737AABD84F1442B102FE180BDBF79541886035980558909AACC65B`；
- `Saved/Logs/Dev.D.UE.0.0.10.P4.9.r0_full_automation_final.log`：`85406797F32B12A910A1BF4BDCE87411D84BFE64023548B779CDB46DC94089CD`；
- `Saved/Logs/Dev.D.UE.0.0.10.P4.9.r0_enemy_skill_automation_final.log`：`15D1FF96E6AE11696401CCC86FCF5752EB30D97F7E43C2F2297C8DF6BC41DF66`；
- `Saved/Logs/Dev.D.UE.0.0.10.P4.9.r0_v2_ranged_automation_final.log`：`041D82A3FAE6EF2CB9DEEC646FAE6FC56838CADDC9311C0522A77C989EC183B6`；
- `Saved/Logs/Dev.D.UE.0.0.10.P4.9.r0_item_armor_automation_final.log`：`6AF5F64F90CE67C0079E481C8261AFD09E6694401D609834AD206B2D79D2485F`。

五份最终日志各包含 UE 5.8 测试发现阶段既有的 13 条 `Condition failed` 负向自检诊断；目标测试全部成功，fatal、unhandled exception与 handled ensure 均为 `0`。

## 首次失败与修正记录

- coordinator 定向初跑：`10/10`，退出码 `0`；
- 0.0.10 全量初跑：`106/106`，退出码 `0`；
- EnemySkillFramework 初跑：`44/44`，退出码 `0`；
- V2RangedCompatibility 初跑：`20 Success / 2 Fail`，退出码 `0`；两个失败均为旧测试硬编码 Schema `4`；
- ItemUseAndArmor 初跑：`42 Success / 4 Fail`，退出码 `0`；失败来自 Schema `4` 与纳物戒旧 `AccessorySlot` 断言；
- 最小测试维护后 V2 `22/22`、ItemUseAndArmor `46/46`；没有把测试漂移描述为源码或环境故障，也没有删除初始日志证据。

## 构建与静态检查

- Editor Development 首次完整构建：`26/26` actions，`Result: Succeeded`，原生退出码 `0`；
- receipt hardening与测试维护后的增量 Editor 构建均成功；最后一次 `5/5` actions，退出码 `0`；
- Game Development 最终构建：`26/26` actions，`Result: Succeeded`，原生退出码 `0`；
- `git diff --check`：原生退出码 `0`；
- coordinator 对 legacy damage、随机 GUID／RNG与地址身份 API 扫描：`0` 匹配；
- shared projectile canonical block：`0` 个 `ApplyDamage`；全文件保留 `1` 个 compatibility `ApplyDamage`；
- 未发生 Windows commit-memory／页面文件错误。

## 修改范围

- `Source/demo_map/demo_mapCombatRunCoordinator.h/.cpp`
- `Source/demo_map/demo_mapCombatRunCoordinatorTests.cpp`
- `Source/demo_map/demo_mapGameMode.h/.cpp`
- `Source/demo_map/demo_mapRangedEnemyCharacter.h/.cpp`
- `Source/demo_map/demo_mapSkillProjectile.h/.cpp`
- `Source/demo_map/demo_mapEnemySkillFrameworkTests.cpp`（projectile sequence 默认值契约）
- `Source/demo_map/demo_mapV2RangedCompatibilityTests.cpp`（遗留 Schema 测试维护）
- `Source/demo_map/demo_mapItemUseAndArmorTests.cpp`（遗留 Schema／SpatialRingSlot 测试维护）
- 本 Report 与同名 Development Log

未修改、删除或提交工作区中的无关长期未跟踪文件。

## 兼容性与剩余边界

- P4.7 ordinary melee 与 P4.8 melee dash canonical 路径继续复用同一 enemy attack receipt、resolver与 vitality ledger；没有建立第二套 projectile 结算系统；
- 非 M01 ranged projectile、shared player projectile与 M01 Boss projectile保留原 legacy contact路径；
- M01 heavy与 Boss attack family 尚未迁移；
- 本轮没有改写 projectile flight、collision radius、target filter、LOS、AI 决策、cooldown、VFX或 destroy时序；
- Actor 层真实世界接线已完成静态与 NullRHI 回归，真实碰撞／视觉／输入验收属于 F 阶段。

## P/F 边界

本 Report 只包含 P 阶段源码开发、代码审查、静态扫描、NullRHI headless Automation、Editor Development与 Game Development构建。未启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、Cook或 Package。

## 下一步

P4.10 建议迁移 M01 heavy enemy attack：冻结 heavy action family与接触/范围 detector，继续复用玩家 defense snapshot与 exactly-once vitality ledger，并保持非 M01 heavy路径兼容。之后再处理 Boss 多技能家族。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p4-9-ranged-projectile-product/Docs/Report/Dev.D.UE.0.0.10.P4.9.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p4-9-ranged-projectile-product/Docs/Log/Dev.D.UE.0.0.10.P4.9.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p4-9-ranged-projectile-product>
