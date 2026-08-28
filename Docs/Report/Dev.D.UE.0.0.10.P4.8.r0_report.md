# Dev.D.UE.0.0.10.P4.8.r0 开发报告

## 结论

`PASS`。P4.8 已把 M01 标准与强化近战敌人的真实 melee-dash 首次合法接触接入 0.0.10 canonical combat：技能运行时稳定序号 → authored dash action → deterministic contact candidate／ImpactId → 玩家 defense snapshot → 纯函数 resolve → exactly-once vitality commit → 由首次真实提交结果驱动既有击退。

M01 冲刺一旦进入产品路由，即使 coordinator 尚未 ready、profile 不一致或 delivery 被拒绝，也只会失败关闭并消费本次已授权接触，不回落到 `ApplyIncomingDamage`。非 M01 地图仍保留原路径。冲刺几何、AI 决策、位移、命中筛选、冷却和表现未改写。

## 功能性

### 1. 标准与强化冲刺动作族

- 标准冲刺 Action：`Combat.Action.Enemy.Melee.Dash.Standard`；
- 强化冲刺 Action：`Combat.Action.Enemy.Melee.Dash.Enhanced`；
- 共用 detector：`Detector.Enemy.Melee.DashContact`；
- 共用 formula：`Combat.Formula.Enemy.Melee.Dash.r1`；
- 两个 family 各自拥有冻结的 content digest，receipt 会逐项校验 family、action、content、detector、formula、伤害标签与守恒；
- coordinator 的 authored enemy binding 现在同时冻结 `SkillProfileId`，调用 profile 必须与注册 profile 完全一致，不能把标准敌人伪装成强化冲刺或反向调用。

### 2. 稳定身份与重放边界

- 冲刺 action sequence 直接使用 `Udemo_mapEnemySkillRuntimeComponent::ActivationSerial`；
- 该 serial 在每次技能成功激活时递增，在 `ResetForNewRun` 时归零，因此不依赖帧序、碰撞回调顺序、对象地址或随机 GUID；
- RunId、稳定 source EntityId、dash ActionDefinition 与 ActivationSerial 确定性派生 ActivationId；
- ActivationId、detector、稳定 player EntityId 与 ordinal 确定性派生 ImpactId；
- 无效 profile、`ActivationSerial == 0` 或 authored profile mismatch 均在创建 action identity 和生命写入前失败；
- 完整旧 receipt 重放返回 `AlreadyCommitted`，不重复扣血、revision、ledger 或表现广播；
- 若调用方用同一 serial 在玩家 revision 已变化后重新解析，ImpactId 虽相同但快照不一致，ledger 以 `CommitRejected` 失败关闭，绝不把它误当作完整 receipt replay；
- 新 Run 的 serial 可从 1 重启，但 RunId 不同，身份与旧 Run 不同；旧 Run receipt 以 `RunMismatch` 拒绝。

### 3. 产品接线与唯一生命写入

- `Ademo_mapEnemyCharacter::HandleDashSegment` 保留既有 sweep、目标过滤、玩家 Health 存活检查与 first-legal-hit 语义；
- M01 通过 `Ademo_mapGameMode::ExecuteM01EnemyMeleeDashContact` 进入 coordinator；
- coordinator 重新解析当前 Run source／target Registry identity，要求 source authored、vitality-bound、仍存活，target 必须是当前绑定玩家；
- resolver 消费 P4.7 已有的玩家 defense snapshot，最终生命变化只由玩家 vitality commit ledger 写入；
- canonical 产品失败时 `AppliedDamage == 0`，Actor 不执行 legacy 写入，也不触发击退；
- 非 M01 的唯一 `ApplyIncomingDamage` 保留在明确的 compatibility `else` 分支中。

### 4. 击退只跟随首次成功提交

- execution result 新增 `GetNewlyCommittedDamage()`：仅 `CommitStatus == Committed` 返回实际 applied damage；`AlreadyCommitted`、拒绝、完全防御均返回 0；
- `DidNewCommitDefeatTarget()` 只读取首次成功 vitality receipt 的 after 值；
- `ShouldRequestEnemySkillKnockback` 从整数输入提升为有限浮点输入，以保留 canonical fractional damage；
- 只有首次 `Committed`、实际 applied damage 大于 0 且目标未被本次提交击败时才请求既有 knockback component；
- exact replay、同 identity 不同快照拒绝、完全防御与致死冲刺都不会重复或错误触发击退。

## 自动化证据

### P4.8／CombatRunCoordinator 定向

新增 `Shanmen.0_0_10.Product.CombatRunCoordinator.M01EnemyMeleeDashProduct`，使定向测试从 8 条增至 9 条，覆盖：

- invalid profile、zero serial 与 source/profile mismatch 在 identity／mutation 前拒绝；
- 标准冲刺 `1.0` raw 经 `0.25` flat reduction 得到 `0.25` prevented、`0.75` final；
- 标准与强化 profile 产生不同、冻结且可复算的 action family；
- exact receipt replay 返回 `AlreadyCommitted`；
- 同 serial 重新解析不同 revision snapshot 返回 `CommitRejected`，不二次写入也不击退；
- 旧 Run receipt 拒绝，新 Run serial 1 产生新身份；
- 致死冲刺按生命上限 clamp 为真实 applied damage，并禁止击退。

最终：`9/9 Success`、`0 Fail`、queue empty，原生退出码 `0`。

### 0.0.10 全量

最终：`105/105 Success`、`0 Fail`、queue empty，原生退出码 `0`。

### 既有 EnemySkillFramework 回归

最终：`44/44 Success`、`0 Fail`、queue empty，原生退出码 `0`。本轮顺带修正一条遗留测试漂移：旧 P6 测试仍把 Profile Schema 字面量写为 `4`，而当前产品基线自 F1.0.r1 起已是 `7`。只把测试更新为精确当前 Schema `7` 且验证默认实例跟随 `CurrentSchemaVersion`；未修改 Profile 产品结构、序列化或迁移逻辑。

最终日志 SHA-256：

- `Saved/Logs/Dev.D.UE.0.0.10.P4.8.r0_combat_run_automation_final.log`：`6D02A5986F310882B543B17CC02C7604EB697AFAE7F47F45A5115563D5CE7625`；
- `Saved/Logs/Dev.D.UE.0.0.10.P4.8.r0_full_automation_final.log`：`A4F40A20D592A30BBAF28FB8C3E1884429565105C4BD9E970A17006440B32AD8`；
- `Saved/Logs/Dev.D.UE.0.0.10.P4.8.r0_enemy_skill_automation_final.log`：`91EEF58EEE229ECD402F1ABE208ECCC41078FE94295DACD3FE9892CAB95CB0DE`。

三份最终日志各含 UE 5.8 测试发现阶段既有的 13 条 `Condition failed` 负向自检诊断；全部目标测试成功，且没有 fatal、unhandled exception 或 handled ensure。

## 首次失败与修正记录

- 定向初跑：`8 Success / 1 Fail`，原生进程退出码 `0`。失败来自测试把“同 serial 重新构造新 revision snapshot”错误等同于“完整 receipt replay”；实现正确地以同 ImpactId／不同快照拒绝。测试被拆分为 exact receipt replay 与 reconstruction rejection 两个断言，修正后 `9/9`；
- EnemySkillFramework 初跑：`43 Success / 1 Fail`，原生进程退出码 `0`。失败是上述遗留 Schema `4` 字面量与当前 Schema `7` 不一致；最小测试维护后 `44/44`；
- `Shanmen.0_0_10` 初跑即 `105/105`；
- 没有把测试断言错误描述为源码编译错误，也没有隐藏首次失败。

## 构建与静态检查

- Editor Development 首次完整构建：`34/34` actions 成功，`Result: Succeeded`，原生退出码 `0`；
- 后续源码／测试修正后的增量 Editor 构建均成功，最后一次 `4/4` actions，原生退出码 `0`；
- Game Development 最终构建：`33/33` actions 成功，`Result: Succeeded`，原生退出码 `0`；
- `git diff --check`：原生退出码 `0`；
- coordinator 对 `ApplyDamage`、`TakeDamage`、`ApplyIncomingDamage`、随机 GUID、RNG 与地址身份 API 扫描：`0` 匹配；
- `HandleDashSegment` 中只有 `1` 个 `ApplyIncomingDamage`，位于非 M01 compatibility 分支；M01 canonical 分支为 `0`；
- 未发生 Windows commit-memory／页面文件错误。

## 修改范围

- `Source/demo_map/demo_mapCombatRunCoordinator.h/.cpp`
- `Source/demo_map/demo_mapCombatRunCoordinatorTests.cpp`
- `Source/demo_map/demo_mapEnemyCharacter.cpp`
- `Source/demo_map/demo_mapEnemySkillTypes.h/.cpp`
- `Source/demo_map/demo_mapGameMode.h/.cpp`
- `Source/demo_map/demo_mapEnemySkillFrameworkTests.cpp`（仅遗留 Schema 测试基线维护）
- 本 Report 与同名 Development Log

未修改、删除或提交工作区中的无关长期未跟踪文件。

## 兼容性与剩余边界

- M01 普通近战 P4.7 canonical 路径继续工作；本轮将其 receipt／execution 命名泛化为 enemy attack family，没有新增第二套结算系统；
- 非 M01 melee dash 保留 legacy `ApplyIncomingDamage`；
- M01 ranged projectile、heavy attack 与 boss skill 尚未迁移；
- 本轮没有改写 dash 几何、导航、AI 目标选择、技能 cooldown、位移 ownership、knockback 参数或表现；
- Actor 层真实世界接线已完成静态与无头回归，但真实碰撞／视觉／输入验收属于 F 阶段。

## P/F 边界

本 Report 只包含 P 阶段源码开发、代码审查、静态扫描、NullRHI headless Automation、Editor Development 与 Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、Cook 或 Package。

## 下一步

P4.9 建议迁移 M01 ranged projectile impact：冻结 projectile activation／spawn identity 与 contact ordinal，复用本轮 enemy attack receipt、player defense snapshot 和 exactly-once vitality ledger，并明确 projectile destroy／impact VFX 只跟随首次成功产品提交。之后再迁移 heavy 与 boss 家族。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p4-8-melee-dash-product/Docs/Report/Dev.D.UE.0.0.10.P4.8.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p4-8-melee-dash-product/Docs/Log/Dev.D.UE.0.0.10.P4.8.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p4-8-melee-dash-product>
