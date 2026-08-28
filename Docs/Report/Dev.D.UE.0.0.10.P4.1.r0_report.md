# Dev.D.UE.0.0.10.P4.1.r0 开发报告

## 结论

`PASS`。P4.1 已把 P4.0 的生命提交实现拆分为可供产品单一真值直接使用的 `FShanmenVitalityCommitLedger`：产品继续持有唯一的当前／最大生命数值，ledger 只保存浮点位指纹、单调 revision 和已提交 Impact receipt，不保存第二份可写生命值。

规范 Impact 通过 `Commit(Command, InOutCurrentVitality, MaximumVitality)` 原子修改调用方持有的唯一状态；治疗、装备上限、回滚或兼容伤害等非 Impact 写入通过 `TryCommitExternalMutation` 同时修改同一状态并推进 revision。任何未通知 ledger 的旁路写入，会在下一次快照捕获或 Impact 提交时以 `StateDesynchronized` 失败关闭。

本轮没有直接改造旧 Actor。审计确认玩家生命存在伤害、治疗、装备上限／回滚等多种写入，敌人和训练目标又分别持有整数生命，而 0.0.10 Impact 使用浮点生命。先冻结外部状态协议，避免在未定义量化策略时引入双写，是本轮的必要安全边界。

## 产品审计结论

### 玩家生命

`Udemo_mapPlayerHealthComponent` 当前至少有以下写入来源：

- `ApplyIncomingDamage`：包含旧随机闪避、固定减伤和整数向下取整；
- `ApplyHealing`／`ApplyRestoreHealthReceipt`：治疗与物品 receipt；
- `ApplyMaxHealthFromAttributes`：装备／属性变化会改变最大生命并钳制当前生命；
- `RestoreCurrentHealthAfterItemUseRollback`：物品事务回滚；
- 非 Shipping 自动化状态设置。

这些写入必须在产品迁移时共享同一 revision，不能只把新剑击接到另一份 authority。

### 敌人与训练目标

近战、远程、重型敌人及训练目标各自持有整数生命并覆写 `TakeDamage`。现有伤害路径使用 `FloorToInt`，而 P4.0 命令和 receipt 使用浮点。若直接接线，分数伤害、receipt 的 AppliedDamage 和旧 UI／死亡判断之间没有既定契约。

因此 P4.1 不选择任一旧类作为临时双写宿主；P4.2 必须先把被迁移目标改成唯一浮点状态，或明确且测试一个单一量化入口。

## 实现内容

### 外部状态 ledger

新增 `FShanmenVitalityCommitLedger`：

- 绑定一个稳定 `TargetEntityId` 与初始 revision；
- 只保存 current／maximum 的 32-bit 浮点指纹，不保存数值副本；
- `TryCaptureSnapshot` 只有在调用方状态与指纹精确同步时成功；
- `Commit` 复用 P4.0 的命令验证、目标校验、过期快照、Resolution 冲突、幂等 replay、overkill 钳制和 revision 上限语义；
- 首次成功直接修改调用方传入的 current vitality 引用，更新指纹、推进 revision 并保存 receipt；
- 完全重复投递返回原 receipt，不再次修改外部状态；
- 状态引用与 ledger 指纹不一致时返回新增的 `StateDesynchronized`，不消费 Impact。

### 原子外部变更

`TryCommitExternalMutation` 接收当前／最大生命引用和目标新状态：

1. 先验证现有引用仍与 ledger 指纹一致；
2. 验证新状态有限且满足 `0 <= Current <= Maximum`；
3. no-op 不推进 revision；
4. revision 到达 `MAX_int64` 时失败关闭；
5. 成功时在同一调用中替换外部状态、更新指纹并只推进一次 revision。

调用方无法先“报账”再自行赋值；ledger 直接修改唯一状态引用，减少 Adapter 误用窗口。

### P4.0 兼容重构

`FShanmenVitalityAuthority` 的公开 API 保持不变，但内部改为：

- 自身持有 current／maximum；
- 使用同一个 `FShanmenVitalityCommitLedger` 管理 revision 与 Impact；
- 快照和提交全部委托给 ledger。

因此 P4.0 的 5 条 authority 测试和 BasicSword 集成继续验证同一实现，不存在两套提交逻辑。

## 自动化覆盖

新增 3 条 `Shanmen.0_0_10.CombatRuntime.VitalityLedger` 测试：

| 测试 | 结果 |
|---|---|
| `ExternalStateCommit` | PASS；直接修改调用方唯一状态，重复投递返回原 receipt 且不二次扣血 |
| `ExternalMutationInvalidation` | PASS；声明的外部伤害／治疗推进 revision，旧命令过期；已提交 Impact 在后续状态变化后仍幂等 |
| `BypassDetection` | PASS；未声明写入阻断快照与 Impact，不能伪造 before-state；revision 上限阻断外部变更 |

最终结果：

- `Shanmen.0_0_10.CombatRuntime`：`16/16 Success`、`0 Fail`、queue empty，原生退出码 `0`；
- 全量 `Shanmen.0_0_10`：`92/92 Success`、`0 Fail`、queue empty，原生退出码 `0`。

最终日志 SHA-256：

- `Saved/Logs/Dev.D.UE.0.0.10.P4.1.r0_combatruntime_automation.log`：`706E9444F66F56420638D52FF092203FE303DCCC10AD6177FC81A4B868F2A659`；
- `Saved/Logs/Dev.D.UE.0.0.10.P4.1.r0_full_automation.log`：`4299D3777874DAE8E39D4429BCD2AF9489B2A94EFE41C0F20E298F33E876B5EE`。

两份最终日志各保留 UE 5.8 在测试发现前打印的既有 13 条 `LogAutomationTest: Error: Condition failed` 启动诊断；目标测试全部成功、无 handled ensure。Win64 SDK 为 VALID；非目标 LinuxArm64／VisionOS 的 `MainVersion` metadata 诊断不属于源码失败。

## 构建与静态检查

- `git diff --check`：退出码 `0`；
- Editor：首次 6/6 actions、原子接口复审后最终 6/6 actions，均 `Result: Succeeded`、原生退出码 `0`；
- Game：最终 5/5 actions，`Result: Succeeded`、原生退出码 `0`；
- 构建统一使用 `-WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA -NoUBTMakefiles`；
- 排除 Tests 后，本轮产品源对 `demo_map`、World／Actor、旧 damage API、`ShanmenItems`、RNG 与 LineTrace／SweepMulti／OverlapMulti 的匹配均为 `0`。

## 修改范围

- `Source/ShanmenCombatRuntime/Public/ShanmenVitalityAuthority.h`
- `Source/ShanmenCombatRuntime/Private/ShanmenVitalityAuthority.cpp`
- `Source/ShanmenCombatRuntime/Private/Tests/ShanmenVitalityAuthorityTests.cpp`
- 本 Report 与同名 Development Log

工作区长期未跟踪的 Prompt、旧 Report、自动化文档和用户资料均未修改、删除或加入提交。

## P/F 边界

只执行源码开发、静态审查、headless Automation 与必要 Editor／Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、Cook 或 Package。

## 下一步

进入 P4.2：选择一个实际产品生命宿主，先绑定来自 World Entity Registry 的稳定 EntityId，再将其唯一内部生命改为浮点或冻结唯一量化入口；伤害、治疗、最大生命和回滚必须全部通过同一 ledger。完成后接入一条 BasicSword → 产品目标纵切，并以测试证明没有调用旧 `ApplyDamage` 形成双写。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p4-1-external-vitality-ledger/Docs/Report/Dev.D.UE.0.0.10.P4.1.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p4-1-external-vitality-ledger/Docs/Log/Dev.D.UE.0.0.10.P4.1.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p4-1-external-vitality-ledger>

`READY_FOR_0_0_10_P4_2`
