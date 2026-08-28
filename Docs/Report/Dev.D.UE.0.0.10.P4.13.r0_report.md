# Dev.D.UE.0.0.10.P4.13.r0 Report

## 1. 结论

P4.13 完成，结论为 **PASS**。

M01 玩家 Straight Projectile 已从共享 legacy `ApplyDamage` 路径迁移到 0.0.10 canonical combat：发射前冻结 deterministic action identity，命中时通过 projectile world-hit adapter、纯函数 impact resolver 与 M01 vitality ledger 完成 exactly-once delivery。M01 产品失败 fail-closed，不回退 legacy damage；非 M01 compatibility、既有飞行与碰撞语义继续保留。

本轮最终验证全部通过：

- Editor Development：`25/25` actions，Succeeded，原生退出码 `0`；
- CombatRunCoordinator：`16/16 Success`；
- 0.0.10 全量：`112/112 Success`；
- EnemySkillFramework：`44/44 Success`；
- V2RangedCompatibility：`22/22 Success`；
- Game Development：`24/24` actions，Succeeded，原生退出码 `0`；
- `git diff --check`：退出码 `0`。

## 2. 功能性

### 2.1 发射身份

Straight Projectile 使用独立 Run-local sequence，并冻结以下契约：

- Action：`Combat.Action.Player.Skill.StraightProjectile`；
- Detector：`Detector.Player.Skill.StraightProjectile.Projectile`；
- Formula：`Combat.Formula.Player.Skill.StraightProjectile.r1`；
- Version：`0.0.10.P4.13`；
- Digest：`Shanmen.Player.Skill.StraightProjectile.Projectile.r1`；
- Source tag：Player；
- Damage tag：Physical；
- Target tag：Living；
- Detector tag：Projectile；
- projectile ordinal：`0`。

Projectile actor 构造时 collision 为 `NoCollision`。Actor 成功创建后、collision 与 movement 启动前，coordinator 才分配 sequence 与 ActivationId；launch-segment sweep 因而也携带完整 canonical metadata。Actor 创建失败不消费 sequence；prepare 失败会销毁 actor、返回失败，技能 cooldown 不提交。

### 2.2 命中与写入

M01 玩家 projectile contact 由 GameMode 桥接到 coordinator。Coordinator：

1. 校验 active Run、精确玩家 source、sequence、ActivationId、damage 与 contact；
2. 重建冻结 action 并比对 ActivationId；
3. 只接受已注册的 M01 enemy target；
4. 使用 `FShanmenWorldHitAdapter::TryFromProjectile` 转换 Actor、Component、location 与 normal 证据；
5. 通过 action orchestrator、纯函数 defense resolver 生成 receipt；
6. 由 M01 vitality ledger exactly-once 提交生命变化。

相同 receipt 重放不会再次修改 vitality、revision、ledger 或 broadcast。旧 Run receipt 不能写入新 Run；新 Run 将 projectile sequence 重置为 `1`。

### 2.3 fail-closed

M01 玩家 source 或 canonical-player metadata 一旦进入产品分支，就不会到达共享 legacy `ApplyDamage`。缺失 GameMode、无效 sequence、错误 ActivationId、非法 damage/contact、未注册 target 或 receipt 不一致均在 mutation 前拒绝；projectile 仍按一次 contact 语义消费，不产生第二条伤害路径。

## 3. 完整性

本轮覆盖 Straight Projectile 的完整产品链：

- SkillComponent：damage snapshot、spawn、prepare、初始化、cooldown 结果；
- SkillProjectile：launch-segment、flight contact、metadata、消费与 compatibility fallback；
- GameMode：精确 source ownership gate 与 coordinator bridge；
- CombatRunCoordinator：identity、action、candidate、impact、receipt 与 ledger delivery；
- 无头测试：成功路径、重放、Run reset、确定性 fixture 与失败关闭。

新增测试覆盖：

- 相同 fixture 的 deterministic ActivationId/ImpactId；
- projectile sequence 与其它玩家技能 sequence 相互独立；
- launch action 的冻结常量和 tags；
- projectile adapter 与 contact evidence；
- damage conservation 与 exact receipt replay；
- 第二 projectile 的新身份；
- Run reset 后 sequence 从 `1` 重新开始；
- inactive coordinator、错误 source、零值/NaN damage、错误 ActivationId、NaN contact、未注册 target；
- 所有失败在 mutation 前拒绝，且非法 prepare 不消费 sequence。

## 4. 兼容性

以下既有语义未改变：

- launch-segment sweep；
- friendly/unauthorized target pass-through；
- WorldStatic contact 消费；
- projectile speed、range、lifespan 与 visual feedback；
- Boss volley 与 Ranged enemy canonical 分支；
- 非 M01 shared projectile compatibility fallback；
- GroundCircle 与 SelfSector 的 P4.12 canonical 路径。

共享 projectile 文件只剩 `1` 个 `ApplyDamage`，且只在 `!bUsedCanonicalProduct` compatibility 分支。SkillComponent 只剩 `2` 个 `ApplyDamage`，均属于 `!bUseM01ProductPath` 的非 M01 shape compatibility。

## 5. 修改范围

生产代码：

- `Source/demo_map/demo_mapCombatRunCoordinator.h`
- `Source/demo_map/demo_mapCombatRunCoordinator.cpp`
- `Source/demo_map/demo_mapGameMode.h`
- `Source/demo_map/demo_mapGameMode.cpp`
- `Source/demo_map/demo_mapSkillComponent.cpp`
- `Source/demo_map/demo_mapSkillProjectile.h`
- `Source/demo_map/demo_mapSkillProjectile.cpp`

测试：

- `Source/demo_map/demo_mapCombatRunCoordinatorTests.cpp`

文档：

- `Docs/Report/Dev.D.UE.0.0.10.P4.13.r0_report.md`
- `Docs/Log/Dev.D.UE.0.0.10.P4.13.r0_log.md`

未修改 Profile、CodeB、Items 或存档 schema；未提交工作区中的长期未跟踪文件。

## 6. 改动文件映射回归

回归范围由 `git diff --name-only 5b87ac7dacbfa3bc4d34a2979e89bfeea16ef897` 推导：

- coordinator 与测试 → `Shanmen.0_0_10.Product.CombatRunCoordinator`；
- GameMode → `Shanmen.0_0_10` 全量；
- SkillComponent / SkillProjectile → `demo_map.EnemySkillFramework`、`demo_map.V2RangedCompatibility` 与 0.0.10 全量。

所有映射组都有本轮独立日志证据，且首轮即通过；没有用主题相关性替代改动文件覆盖。

## 7. 静态检查

- `git diff --check`：退出码 `0`；
- coordinator 中 `ApplyDamage`、`NewGuid`、RNG、地址身份 API：`0` 匹配；
- `ShanmenCombatCore`、`ShanmenCombatRuntime` 中 `demo_map` include、`UWorld`、`AActor`、`ApplyDamage`、RNG／地址身份 API：`0` 匹配；
- `ShanmenItems` 中 `demo_map` include、`UWorld`、`AActor`、`ApplyDamage`、运行时 RNG／地址身份 API：`0` 匹配；
- coordinator 明确通过 `FShanmenWorldHitAdapter::TryFromProjectile` 使用 WorldGameplay adapter；
- 四份 Automation 日志 fatal、unhandled exception、handled ensure：均为 `0`。

## 8. 构建与自动化

### Editor Development

```powershell
Build.bat demo_mapEditor Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- `25/25` actions；
- `Result: Succeeded`；
- 原生退出码 `0`；
- 总执行时间 `126.49s`。

### Automation

- CombatRunCoordinator：`16/16 Success`、`0 Fail`、queue empty、退出码 `0`；
- 0.0.10 全量：`112/112 Success`、`0 Fail`、queue empty、退出码 `0`；
- EnemySkillFramework：`44/44 Success`、`0 Fail`、queue empty、退出码 `0`；
- V2RangedCompatibility：`22/22 Success`、`0 Fail`、queue empty、退出码 `0`。

四组均在最终源码上首轮通过，因此没有制造冗余 final rerun，也没有失败日志需要保留。

### Game Development

```powershell
Build.bat demo_map Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

- `24/24` actions；
- `Result: Succeeded`；
- 原生退出码 `0`；
- 总执行时间 `93.20s`。

## 9. 诊断说明

每份 Automation 日志包含 UE 测试发现阶段既有的 `13` 条负向自检 `LogAutomationTest: Error: Condition failed`；目标测试结果均为 Success，queue 正常清空，进程退出码为 `0`。本轮无源码、工具链、commit-memory 或页面文件失败。

## 10. P/F 边界

本 Report 只包含 P 阶段开发、代码审查、静态检查、无头 `-NullRHI` Automation、Editor Development 与 Game Development 构建。

未启动 Unreal Editor UI、PIE、Standalone、产品可执行文件；未执行真实输入、截图、Smoke、大规模产品回归、Cook 或 Package。真实产品体验与输入验证继续留给 F 阶段。

## 11. 后续建议

P4.13 后，M01 玩家 Straight Projectile 已不再经过 legacy damage。下一阶段应先自动审计剩余 `ApplyDamage`，区分：

1. M01 产品入口；
2. 非 M01 compatibility；
3. automation driver。

若没有新的产品入口，应优先把“改动路径 → 必跑测试组”的映射做成可执行检查，避免继续按主题手工选择回归范围。

## 12. GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p4-13-player-projectile-product/Docs/Report/Dev.D.UE.0.0.10.P4.13.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p4-13-player-projectile-product/Docs/Log/Dev.D.UE.0.0.10.P4.13.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p4-13-player-projectile-product>
