# Dev.D.UE.0.0.10.P4.7.r0 开发报告

## 结论

`PASS`。P4.7 已把 M01 普通近战敌人的真实接触攻击从 legacy `UGameplayStatics::ApplyDamage` 迁入 0.0.10 canonical combat 链：稳定 Run／敌人／玩家身份 → Action snapshot → Shape hit candidate → 确定性玩家 defense snapshot → 纯函数 Impact resolve → exactly-once player vitality commit。

本轮只迁移 `Ademo_mapEnemyCharacter::AttackPlayer` 对应的普通近战接触族。M01 中该路径失败时原子返回，不会回落到旧伤害入口；非 M01 地图仍保留旧入口以维持兼容。冲刺、投射物、重击与 Boss 技能尚未迁移，未被伪装为本轮完成项。

## 功能性

### 1. 真实产品路径

- `Ademo_mapEnemyCharacter::AttackPlayer` 保留既有阵营过滤、距离判定、冷却与受击反馈，只把已授权接触的伤害交给 GameMode；
- GameMode 在 M01 地图无条件拥有路由决定，coordinator 尚未 ready 或拒绝执行时也不双写旧伤害；
- coordinator 要求 source 是当前 Run Registry 中已 authored、已 vitality-bound 且仍存活的 M01 敌人；
- target 必须是当前 Run 已绑定玩家，且玩家 Pawn、HealthComponent 与稳定 `PlayerEntityId` 已由 Registry 一致解析；
- candidate 使用 `Detector.Enemy.Melee.Contact` 与 `Shape`，目标为稳定玩家 EntityId；
- damage 使用 `Combat.Formula.Enemy.Melee.Basic01.r1` 与 `Damage.Physical` 标签；
- resolver 只做纯函数结算，最终生命写入只允许通过玩家 `FShanmenVitalityCommitLedger`。

### 2. 稳定身份与动作生命周期

- ActionDefinition：`Combat.Action.Enemy.Melee.Basic01`；
- 每个稳定敌人 EntityId 拥有独立、Run-local、单调递增的 activation sequence；
- ActivationId 与 ImpactId 均由 RunId、稳定 EntityId、定义、序号、detector 和目标确定性派生；
- 不使用随机 GUID、对象地址、对象名、`GetUniqueID` 或碰撞回调顺序作为身份；
- Action 按 `Startup → Active → Recovery → Idle` 完整关闭；中途失败进入 interrupt，且已进入 canonical 路由的 M01 攻击不会回退旧写入；
- Run end／reset 清空各敌人的 sequence，新 Run 从 1 开始，但因 RunId 不同不会重用旧身份。

### 3. 玩家防御快照

新增 `Udemo_mapPlayerHealthComponent::TryCaptureCombatDefenseSnapshot`，将现有玩家属性投影为统一有序防御层：

- `DodgeChance` → `PreventAll`，顺序 `Avoidance`，标签 `Defense.Evade`；
- `FlatDamageReduction` → `AbsorbPoints`，顺序 `Resistance`，标签 `Defense.Armor`；
- 两层都要求 `Target.Living`；
- layer id 由目标 EntityId 与规则 ID 派生；
- 闪避采样由 ImpactId 与规则 ID 派生，同一 ImpactId 重放得到相同结果，不调用 `FRand`／`RandRange`；
- 非有限属性失败关闭，负 flat reduction 归零；已战败或未绑定玩家不产生快照。

### 4. 收据、守恒与幂等

- enemy-melee receipt 校验 Action、source/target、detector、formula、damage tag、living tag、ImpactId 与守恒；
- `RawDamage == PreventedDamage + FinalDamage` 由 resolver 结果不变量约束；
- 同一 receipt 首次提交返回 `Committed`，重放返回 `AlreadyCommitted`；
- replay 不重复扣血、不推进 revision、不增加 receipt 数，也不重复发布正伤害表现；
- 完整闪避仍提交一条零伤害、可审计 receipt，但不发布正伤害回调；
- 旧 Run receipt 在新 Run 绑定后以 `RunMismatch` 拒绝。

## 自动化证据

`Shanmen.0_0_10.Product.CombatRunCoordinator` 从 7 条增至 8 条。新增 `M01EnemyBasicMeleeProduct` 覆盖：

- 未注册 source 在消费 action identity 前失败关闭；
- `3` 原始伤害与 `1` flat reduction 得到 `1` prevented、`2` final，玩家生命 `5 → 3`；
- receipt replay 的 exactly-once 行为；
- 同一 ImpactId 的 50% dodge 快照完全稳定；
- 100% dodge 产生 `PreventAll`／`Evaded`，零生命写入且无正伤害广播；
- 新 Run 拒绝旧 receipt；
- 新 Run 的每敌人 activation sequence 从 1 重启且身份与旧 Run 不同。

最终结果：

- 产品定向：`8/8 Success`、`0 Fail`、queue empty，原生退出码 `0`；
- 全量 `Shanmen.0_0_10`：`104/104 Success`、`0 Fail`、queue empty，原生退出码 `0`。

最终日志 SHA-256：

- `Saved/Logs/Dev.D.UE.0.0.10.P4.7.r0_combat_run_automation_final.log`：`2C4DA2881BD08FD2B280B47E97A5A6A9CF1D31ED55E160ABC5B5698D6CD82D7A`；
- `Saved/Logs/Dev.D.UE.0.0.10.P4.7.r0_full_automation_final.log`：`FFE4208F44C1DA1C6C88DD3E9D826F9AFA361E226BF667A2CE5F45898445A959`。

两份最终日志各保留 UE 5.8 在测试发现前输出的既有 13 条 `Condition failed` 自检诊断；目标测试全部成功，无 fatal、unhandled exception 或 handled ensure。

## 构建与静态检查

- Editor Development：`33/33` actions 成功，`Result: Succeeded`，原生退出码 `0`；
- Game Development：`32/32` actions 成功，`Result: Succeeded`，原生退出码 `0`；
- `git diff --check`：原生退出码 `0`；
- coordinator 对 `ApplyDamage`、`TakeDamage`、`ApplyIncomingDamage`、随机 GUID／RNG／地址身份 API 的扫描：`0` 匹配；
- M01 ordinary-melee 产品分支对旧伤害 API 的扫描：`0` 匹配；
- player defense capture 对 `FRand`、`RandRange`、随机 GUID 与地址身份 API 的扫描：`0` 匹配；
- `AttackPlayer` 中仍有 `1` 个 `ApplyDamage`，明确位于 M01 原子返回之后，供非 M01 兼容使用；未将其误报为全项目清零。

本轮首次 Editor／Game 构建、首次定向／全量自动化均成功，没有源码编译失败、测试失败或 Windows commit-memory／页面文件错误。

## 修改范围

- `Source/demo_map/demo_mapCombatRunCoordinator.h/.cpp`
- `Source/demo_map/demo_mapCombatRunCoordinatorTests.cpp`
- `Source/demo_map/demo_mapPlayerHealthComponent.h/.cpp`
- `Source/demo_map/demo_mapGameMode.h/.cpp`
- `Source/demo_map/demo_mapEnemyCharacter.cpp`
- 本 Report 与同名 Development Log

未修改、删除或提交工作区中的无关长期未跟踪文件。

## 兼容性与剩余边界

- 非 M01 普通近战仍使用 legacy `ApplyDamage`；
- M01 melee dash 仍走其既有 `ApplyIncomingDamage` 路径；
- ranged projectile、heavy attack 与 boss skill 尚未进入 enemy→player canonical family；
- 本轮没有改动技能几何、AI 选目标、攻击冷却、表现、击退或掉落／任务规则；
- 玩家 dodge 与 flat reduction 的既有数值语义保留，但 canonical 路径现在可确定性回放。

## P/F 边界

本 Report 只包含 P 阶段源码开发、代码审查、静态扫描、NullRHI headless Automation、Editor Development 与 Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、Cook 或 Package；真实交互与视觉验收留在 F 阶段。

## 下一步

P4.8 建议迁移 M01 melee dash：复用稳定 enemy action identity、玩家 defense snapshot 与 canonical commit，把 dash detector／技能定义纳入 receipt；只在首次 `Committed` 且 final damage 大于零时发布现有击退副作用。完成后再抽象数据驱动 enemy action definition，扩展 projectile、heavy 与 boss 家族。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p4-7-enemy-melee-strike/Docs/Report/Dev.D.UE.0.0.10.P4.7.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p4-7-enemy-melee-strike/Docs/Log/Dev.D.UE.0.0.10.P4.7.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p4-7-enemy-melee-strike>
