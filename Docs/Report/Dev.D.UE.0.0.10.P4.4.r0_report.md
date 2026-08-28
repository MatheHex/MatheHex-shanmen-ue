# Dev.D.UE.0.0.10.P4.4.r0 开发报告

## 结论

`PASS`。P4.4 已把 P4.3 的玩家专用协调器提升为共享 `Fdemo_mapCombatRunCoordinator`，并在真实 M01 激活链中把 14 个 authored enemy 全部登记到同一个 Run-scoped World Entity Registry。每个敌人的稳定 EntityId 只由 `RunId + Definition.SpawnMarkerId + ordinal 0` 确定性派生；不使用 Actor 地址、对象名、生成回调顺序、随机 GUID 或既有随机 `LootSourceId`。

`Ademo_mapEnemyCharacter` 已成为本轮唯一迁移的敌人生命宿主类型：原有整数生命字段改为产品持有的 float vitality 真值，并接入 P4.1 canonical commit ledger。M01 的 5 个 StandardSkirmisher 与 2 个 EliteStalker 因共用该宿主类型，均获得各自的稳定身份、revision 与 Impact 幂等账本；远程、重甲和 Boss 本轮只进入共享 Registry，旧生命写入保持不变。

BasicSword resolved Impact 现在可由共享协调器直接提交给已登记的 M01 近战生命宿主。首次 receipt 只扣减一次并触发一次产品反馈；相同 receipt 重放返回 `AlreadyCommitted`；旧 Run receipt 在新 Run 中以 `RunMismatch` 失败关闭。该路径不调用 `ApplyDamage`／`TakeDamage`，也不重新运行结算器。

## 功能性

### 1. 共享 Run 战斗身份面

- 将 `Fdemo_mapPlayerCombatCoordinator` 重构并改名为 `Fdemo_mapCombatRunCoordinator`；
- 玩家继续使用 `RunId + Spawn.Player.Primary + 0`；
- M01 敌人使用其已存在且唯一的 authored `SpawnMarkerId`；
- Actor 与其碰撞根组件绑定到同一个 EntityId；
- authored archetype 与实际产品 Actor class 不匹配时失败关闭；
- 同一 Actor／同一 authored identity 重复登记幂等；同一 authored spawn 被另一 Actor 抢占时拒绝；
- Run 精确结束时统一释放玩家、近战敌人 vitality binding 与 Registry。

### 2. 真实 M01 生成链接入

`Ademo_mapGameMode::TryActivateCombatRun` 在 M01 的 14 个 Actor 全部生成并配置 identity 后执行登记。任何 Actor 缺失、identity 未配置、class 不匹配或 Registry／vitality 绑定失败，都会清理已建立的 Run 绑定并使世界激活失败；不会留下部分可用的战斗身份面。

运行时日志同时输出 RunId、玩家 EntityId、M01 登记数与已迁移 vitality host 数，便于后续 P4.5 输入／轨迹纵切定位。

### 3. M01 近战 float vitality ledger

- `MaximumVitality`／`CurrentVitality` 是唯一可变生命真值；
- 旧整数 UI 与任务检查继续通过 `CeilToInt` 只读投影；
- 旧 `TakeDamage` 保留整数伤害取整语义，但写入必须经过 `TryCommitExternalMutation`，从而与 canonical revision 同步；
- canonical Impact 通过 `CommitCombatImpact` 直接提交 final damage；
- 首次成功提交更新 vitality、反馈、死亡链与 receipt ledger；replay 不再反馈、不再扣血；
- EndPlay 和精确 Run 结束都会释放 ledger 身份。

### 4. 产品交付门

新增 `DeliverBasicSwordImpactToM01Enemy`，依次验证：

1. coordinator 与玩家身份可用；
2. receipt 结构有效；
3. receipt 属于当前 Run；
4. BasicSword source 等于当前玩家 EntityId；
5. 目标 Actor 已在共享 Registry；
6. receipt target 与 Registry EntityId 一致；
7. 目标确为已迁移的 M01 近战 vitality host；
8. canonical command 可由 request/result 构造并被 ledger 接受。

GameMode 新增 `DeliverResolvedM01MeleeImpact` 作为后续真实输入纵切的唯一产品入口。

## 完整性与兼容性

- M01 14/14 authored definitions 均在自动化中以真实对应 Actor class 登记，EntityId 14/14 唯一；
- 现有随机 `LootSourceId` 仍仅服务尸体／奖励来源，不参与战斗实体身份；
- 保留 `GetCurrentHealth()`／`GetMaxHealth()` 的整数兼容接口，旧任务、HUD 和既有技能无需改写；
- 旧伤害写入不会绕过 ledger revision；
- 远程、重甲、Boss 尚未迁移 vitality，避免本轮同时改动四套死亡／表现链；
- P4.5 才接真实玩家 BasicSword 输入、轨迹发射、world hit 候选与敌人交付；本轮没有伪造该完成状态。

## 自动化证据

新增／重构 4 条 `Shanmen.0_0_10.Product.CombatRunCoordinator` 测试：

- `RunLifecycle`：玩家身份幂等、跨 Run 拒绝、精确释放与持久 Pawn 换 Run；
- `PlayerDelivery`：P4.3 玩家交付能力在共享协调器中保持；
- `M01AuthoredIdentity`：14 个 authored actor 全部登记、class 校验、碰撞根 alias、冲突拒绝、7 个近战 ledger 与跨 Run 新身份；
- `M01BasicSwordDelivery`：world hit → authored candidate → canonical Impact → 产品 vitality 首次提交、重放与旧 Run 拒绝。

最终结果：

- 产品定向：`4/4 Success`、`0 Fail`、queue empty，原生退出码 `0`；
- 全量 `Shanmen.0_0_10`：`100/100 Success`、`0 Fail`、queue empty，原生退出码 `0`。

最终日志 SHA-256：

- `Saved/Logs/Dev.D.UE.0.0.10.P4.4.r0_combat_run_automation_final.log`：`F8A8FC6CE2C12015302AA94B9ABE39425691EED985152C069C775827F606D9CC`；
- `Saved/Logs/Dev.D.UE.0.0.10.P4.4.r0_full_automation_final.log`：`B86708A37858556C4318DCA0796A4E7343D5FFE9DEC291C4593AE9A455A9D4A2`。

## 构建与静态检查

- Editor Development：首次完整 `25/25`、最终增量 `6/6`，均 `Result: Succeeded`，原生退出码 `0`；
- Game Development：首次完整 `24/24`、最终增量 `5/5`，均 `Result: Succeeded`，原生退出码 `0`；
- `git diff --check`：原生退出码 `0`；
- coordinator 身份路径扫描：`FGuid::NewGuid`、`GetUniqueID`、`GetName(`、`reinterpret_cast`、`PointerHash` 均无匹配；
- 本轮没有编译失败、测试失败或 Windows commit memory／页面文件错误。

## 修改范围

- `Source/demo_map/demo_mapCombatRunCoordinator.h`
- `Source/demo_map/demo_mapCombatRunCoordinator.cpp`
- `Source/demo_map/demo_mapCombatRunCoordinatorTests.cpp`
- `Source/demo_map/demo_mapEnemyCharacter.h`
- `Source/demo_map/demo_mapEnemyCharacter.cpp`
- `Source/demo_map/demo_mapGameMode.h`
- `Source/demo_map/demo_mapGameMode.cpp`
- 删除并由共享命名替代：
  - `Source/demo_map/demo_mapPlayerCombatCoordinator.h`
  - `Source/demo_map/demo_mapPlayerCombatCoordinator.cpp`
  - `Source/demo_map/demo_mapPlayerCombatCoordinatorTests.cpp`

未修改或提交工作区中的无关长期未跟踪文件。

## P/F 边界

本 Report 只包含 P 阶段代码开发、静态审查、NullRHI headless Automation、Editor Development 构建与 Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、大规模产品回归、Cook 或 Package。

## 下一步

P4.5 建议在现有共享 coordinator 之上接入真实玩家 BasicSword 输入与轨迹：从玩家稳定 source 创建 action，使用 Registry 把 M01 碰撞命中解析为 authored target，捕获目标 vitality 快照，生成 receipt 后只走 `DeliverResolvedM01MeleeImpact`。不得恢复旧 `ApplyDamage` 双写，也不得用 `LootSourceId` 充当 EntityId。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p4-4-m01-enemy-vitality/Docs/Report/Dev.D.UE.0.0.10.P4.4.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p4-4-m01-enemy-vitality/Docs/Log/Dev.D.UE.0.0.10.P4.4.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p4-4-m01-enemy-vitality>
