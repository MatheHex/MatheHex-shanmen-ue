# Dev.D.UE.0.0.10.P2.0.r0 开发报告

## 结论

`PASS`。0.0.10 已新增独立 Runtime 模块 `ShanmenWorldGameplay`，完成 P0 路线图中第一段世界命中适配闭环：UE Sweep、Overlap 与 Projectile 接触现在都能经过同一个稳定身份边界，输出 `ShanmenCombatCore` 的 `FShanmenHitCandidate`。

本轮没有接入 0.0.9B 旧战斗、没有发起世界查询、没有决定敌我／自目标合法性、没有计算或应用伤害，也没有启动 GAS。适配层只把已经发生的几何证据降维为候选；所有产品策略、Impact 结算与生命提交继续留在后续编排层。

## 实现内容

### 独立模块边界

- 新增 `ShanmenWorldGameplay` Runtime 模块，并加入 Game／Editor Target 与 uproject 模块清单。
- 模块公开依赖仅为 `Core`、`CoreUObject`、`Engine` 与 `ShanmenCombatCore`。
- 模块不依赖或 include `demo_map`，因此没有反向耦合旧产品模块。

### 冻结 detector emission 上下文

新增 `FShanmenWorldHitContext`：

- 捕获已经冻结的 `FShanmenCombatActionSnapshot`；
- 捕获稳定 `DetectorId`、`DetectorKind` 与显式 `HitOrdinal`；
- 字段为私有、Blueprint read-only；只能通过 `TryCreate` 构造有效上下文；
- `TargetedRule` 不属于世界接触，无法创建 WorldHit context；
- `HitOrdinal` 必须由权威 detector runtime 指定，适配器不会根据 UE 回调顺序或数组顺序推断身份。

### 稳定实体身份注入

新增 `IShanmenWorldEntityResolver`。Sweep／Overlap／Projectile 只把瞬态 Actor、Component、BodyIndex 和接触来源交给 resolver；resolver 必须返回有效 `FGuid`。

适配层明确禁止用对象指针、对象名或回调顺序生成实体身份。解析失败或返回无效 GUID 时整个候选失败关闭，且清空输出，避免调用方误用旧值。

### 三种世界接触统一输出

`FShanmenWorldHitAdapter` 提供：

- `TryFromSweep`：WeaponTrajectory／Shape／ControlledObject；
- `TryFromOverlap`：Shape／ControlledObject／PersistentZone；
- `TryFromProjectile`：Projectile；
- `IsCompatible`：集中裁决接触来源与 detector 类型的合法组合。

三条路径最终只写同一个 `FShanmenHitCandidate`：Activation、Source、Target、Detector、接触位置／法线和权威 HitOrdinal。Overlap 本身没有接触点／法线，因此由 detector 在同一采样时刻显式提供，适配层只校验并保留证据。

自目标接触不会在几何层被拒绝；其合法性继续属于后续目标策略。适配层也不创建 `ImpactId`，但保留的 canonical 字段可由既有 `FShanmenCombatIdFactory` 稳定重放。

## 自动化覆盖

定向命名空间：`Shanmen.0_0_10.WorldGameplay`

| 测试 | 结果 |
|---|---|
| `SweepAdapter` | PASS；冻结 Action、detector、ordinal 与 resolver 目标身份完整进入候选 |
| `OverlapAdapter` | PASS；Overlap 显式采样位置／法线与 BodyIndex 完整保留 |
| `ProjectileAdapter` | PASS；Projectile 接触进入同一候选结构 |
| `ChannelCompatibility` | PASS；合法组合接受，Projectile→Sweep 与 TargetedRule→World 拒绝 |
| `FailureIsolation` | PASS；身份解析失败与非有限几何失败关闭并清空旧输出 |
| `CandidateIdentity` | PASS；相同输入重放相同 ImpactId，自目标保留给策略层 |

验证结果：

- WorldGameplay 定向：`6/6 Success`、`0 Fail`、queue empty，原生退出码 `0`；
- 全量 `Shanmen.0_0_10`：`72/72 Success`、`0 Fail`、queue empty，原生退出码 `0`。

日志与 SHA-256：

- `Saved/Logs/Dev.D.UE.0.0.10.P2.0.r0_worldgameplay_automation.log`：`381A79B38105C02F4F8D3F5C3FBED936E532F10928F38E8094BE6823F9A7E1FC`；
- `Saved/Logs/Dev.D.UE.0.0.10.P2.0.r0_full_automation.log`：`32D57045D0AE219939AFF48E2A9DE468A9FD33AC936EF172E0BE64BFFE95E4A9`。

UE 5.8 在测试发现前仍打印既有 13 条内置 `LogAutomationTest: Error: Condition failed` 启动诊断，并报告非目标 LinuxArm64／VisionOS SDK metadata 缺失；Win64 SDK 为 VALID，目标测试随后全部成功，原始日志完整保留。

## 构建与静态检查

- `git diff --check`：退出码 `0`；
- `demo_mapEditor Win64 Development`：11/11 actions，`Result: Succeeded`，原生退出码 `0`；
- `demo_map Win64 Development`：6/6 actions，`Result: Succeeded`，原生退出码 `0`；
- 两次构建均使用 `-WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA -NoUBTMakefiles`；
- 新模块边界扫描中 `demo_map`、`GetWorld(`、`UWorld`、`ApplyDamage`、`TakeDamage`、`UGameplayStatics`、RNG、直接 LineTrace／SweepMulti／OverlapMulti 调用均为 `0`。

## 修改范围

- `demo_map.uproject`
- `Source/demo_map.Target.cs`
- `Source/demo_mapEditor.Target.cs`
- `Source/ShanmenWorldGameplay/**`
- 本 Report 与同名 Development Log

工作区长期存在的未跟踪 Prompt、Report、自动化文档和用户文件均未修改、删除或纳入提交。

## P/F 边界

本轮执行源码开发、静态审查、headless Automation 与必要 Editor／Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、Cook 或 Package。

## 下一步建议

进入 P2.1：建立由 Run／Spawner 明确注入的 World Entity 绑定与 detector emission session，让实际世界对象取得稳定 EntityId，并在一个 detector 激活周期内权威分配 HitOrdinal；仍不把旧战斗伤害入口接进新内核。该身份链闭合后，再由 P3 GAS／动作阶段编排层消费候选。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p2-0-world-hit-adapters/Docs/Report/Dev.D.UE.0.0.10.P2.0.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p2-0-world-hit-adapters/Docs/Log/Dev.D.UE.0.0.10.P2.0.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p2-0-world-hit-adapters>

`READY_FOR_0_0_10_P2_1`
