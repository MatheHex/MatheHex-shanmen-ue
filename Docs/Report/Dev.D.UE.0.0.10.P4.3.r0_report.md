# Dev.D.UE.0.0.10.P4.3.r0 开发报告

## 结论

`PASS`。P4.3 已把真实玩家 Pawn 的运行时生成／Run 生命周期接入 P2.1 World Entity Registry，并将同一个稳定 EntityId 注入 P4.2 的 `Udemo_mapPlayerHealthComponent`。玩家身份只由权威 `RunId + Spawn.Player.Primary + ordinal 0` 确定性派生，不使用随机 GUID、Actor 地址、对象名或回调顺序。

新增产品 `Fdemo_mapPlayerCombatCoordinator` 作为 Run 级交付门：World hit 先通过 Registry 把 Pawn／碰撞根组件解析成稳定目标，再由 BasicSword 纯结算生成 Impact receipt；协调器验证 Run、目标和 receipt 后构造规范 vitality command，直接提交到玩家唯一生命真值。首次交付只提交一次，重复 receipt 返回 `AlreadyCommitted`，旧 Run 的延迟 receipt 在新 Run 中以 `RunMismatch` 失败关闭。

## 实现内容

### 1. 确定性玩家实体身份

协调器新增固定 spawn source：`Spawn.Player.Primary`，并调用 `FShanmenWorldEntityIdFactory::MakeEntityId(RunId, SourceId, 0)` 生成玩家 ID。

开始 Run 时原子准备并绑定三类别名：

- 真实玩家 Pawn；
- `Udemo_mapPlayerHealthComponent` 产品生命宿主；
- 存在时的玩家 `UPrimitiveComponent` 碰撞根。

三者必须映射到同一个 EntityId。无效 Run、非 Pawn-owned health、冲突身份、活跃协调器跨 Run／跨宿主切换均被拒绝。协调器先在临时 Registry 中完成全部别名验证，最后才绑定 vitality host，避免半完成注册。

### 2. 真实 Run 激活与退出接线

`Ademo_mapGameMode::ActivateV3MissionContentForRun` 在产品世界投影成功后读取 `Udemo_mapItemSubsystem::GetActiveRunId()`，完成玩家身份绑定；已激活路径会幂等复查同一绑定。若绑定失败，世界激活失败关闭并进入既有清理／回滚链路。

`DeactivateV3MissionContentForPreparation` 在销毁 Run 内容前精确结束该 Run 的 Registry 与 vitality 绑定；`EndPlay` 还保留最终清理兜底。这样同一个持久 Pawn 可以在下一次 Run 使用新的确定性 EntityId，同时旧 Run 的 Impact ledger 不会泄漏到新 Run。

### 3. 玩家生命身份释放

`Udemo_mapPlayerHealthComponent` 新增 `TryEndCombatEntityBinding(ExpectedTargetEntityId)`：

- 只接受有效且精确匹配的当前 EntityId；
- 未绑定状态幂等成功；
- 不匹配身份失败关闭；
- 成功时只清除 Combat ledger／receipt／revision，真实当前和最大生命仍由产品组件唯一持有。

下一 Run 重新绑定时，ledger 从当时的产品生命状态建立 revision 0，不制造第二份生命值。

### 4. BasicSword 产品交付门

新增 `DeliverBasicSwordImpact`，固定顺序为：

1. 协调器及 Registry 必须处于一致的活跃状态；
2. BasicSword receipt 必须满足自身规范验证；
3. receipt 的 Action Run 必须等于当前 Run；
4. Candidate TargetEntityId 必须等于已注册玩家 ID；
5. 从 request/result 重建并复核 `FShanmenVitalityCommitCommand`；
6. 直接调用产品 health 的 `CommitCombatImpact`。

该路径不调用 `ApplyIncomingDamage`、`UGameplayStatics::ApplyDamage` 或 Actor `TakeDamage`，不重跑旧随机闪避／固定减伤。`Ademo_mapGameMode::DeliverResolvedPlayerImpact` 暴露唯一产品入口，供后续真实能力／检测器编排调用。

## 自动化覆盖

新增 2 条 `Shanmen.0_0_10.Product.PlayerCombatCoordinator` 测试：

| 测试 | 结果 |
|---|---|
| `RunLifecycle` | PASS；Pawn、health、collision root 解析为同一确定性 ID，同 Run 幂等、跨 Run 活跃切换拒绝、精确退出后同一 Pawn 可绑定下一 Run 的不同 ID |
| `BasicSwordDelivery` | PASS；真实 `FHitResult` 经 Registry 生成候选，重复回调不能形成第二 Impact，首次 receipt 只扣血／广播一次，replay 不二次提交，下一 Run 拒绝旧 Run 延迟 receipt |

最终结果：

- 产品定向：`2/2 Success`、`0 Fail`、queue empty，原生退出码 `0`；
- 全量 `Shanmen.0_0_10`：`98/98 Success`、`0 Fail`、queue empty，原生退出码 `0`。

最终日志 SHA-256：

- `Saved/Logs/Dev.D.UE.0.0.10.P4.3.r0_player_combat_automation_final.log`：`275620244551C7F7F70C07ED44CC0E81629754B3391F8BB06DC54B2C9D18F88D`；
- `Saved/Logs/Dev.D.UE.0.0.10.P4.3.r0_full_automation_final.log`：`0AAAAFC52A23151BA4F460D796B50ECD3605647E75CDAF01D8A6294FD76DCFDB`。

两份最终日志各保留 UE 5.8 在测试发现前打印的既有 13 条 unified-error/self-test `Condition failed` 启动诊断；目标测试全部成功、无 handled ensure。Win64 SDK 为 `VALID 10.0.22621.0`；LinuxArm64／VisionOS 的 metadata 警告属于非目标平台诊断。

## 构建与静态检查

- Editor Development：首次完整 `33/33`、最终增量 `4/4`，均 `Result: Succeeded`，原生退出码 `0`；
- Game Development：首次完整 `32/32`、最终增量 `3/3`，均 `Result: Succeeded`，原生退出码 `0`；
- 构建统一使用 `-WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA -NoUBTMakefiles`；
- `git diff --check`：原生退出码 `0`；
- 新协调器及测试对 `NewGuid`、`GetUniqueID`、`PointerHash`、`ApplyDamage`、`TakeDamage`、`ApplyIncomingDamage`、`FRand`／`RandRange` 的静态匹配均为 `0`。

本轮没有源码失败或环境失败；无需降并发重试。

## 修改范围

- `Source/demo_map/demo_mapPlayerCombatCoordinator.h`
- `Source/demo_map/demo_mapPlayerCombatCoordinator.cpp`
- `Source/demo_map/demo_mapPlayerCombatCoordinatorTests.cpp`
- `Source/demo_map/demo_mapPlayerHealthComponent.h`
- `Source/demo_map/demo_mapPlayerHealthComponent.cpp`
- `Source/demo_map/demo_mapGameMode.h`
- `Source/demo_map/demo_mapGameMode.cpp`
- 本 Report 与同名 Development Log

工作区长期未跟踪的 Prompt、旧 Report、自动化文档和用户资料均未修改、删除或加入提交。

## P/F 边界

只执行源码开发、静态审查、headless Automation 与必要 Editor／Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、Cook 或 Package。

## 下一步

进入 P4.4：把当前 Registry 提升为共享 Run 战斗实体注册面，使用 M01 已有的 authored encounter／spawn identity 注册敌人；选择一个敌人生命宿主迁移到同一 float vitality ledger。此后 P4.5 再把真实玩家 BasicSword 输入、轨迹检测和敌人 delivery 串成产品纵切。不得从敌人 Actor 地址或现有随机 LootSourceId 派生战斗身份。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p4-3-player-combat-coordinator/Docs/Report/Dev.D.UE.0.0.10.P4.3.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p4-3-player-combat-coordinator/Docs/Log/Dev.D.UE.0.0.10.P4.3.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p4-3-player-combat-coordinator>

`READY_FOR_0_0_10_P4_4`
