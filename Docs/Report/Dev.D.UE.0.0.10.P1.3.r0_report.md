# Dev.D.UE.0.0.10.P1.3.r0 Report

## 结论

P1.3 已完成并通过验证。0.0.10 物品权威现在具备唯一的串行生命周期/命令门面：启动时永远先读取既有 schema-1 权威文档，只有文件确实缺失且调用方显式授权精确 `MigrationId` 时才允许首次迁移；`Reserve / Commit / Cancel / ReleaseDeployment` 只有在结果已持久化、或重开已证明结果确实落盘后才会对外报告为成功。

持久化前失败会恢复到逐字段完全相同的前态；提交后回读歧义会通过重开判定；外部写者造成的代际分歧会令落后实例进入 `RecoveryRequired`，不会覆盖较新权威。Code A/Profile 与 Code B 在真实迁移桥测试中保持只读，本轮仍未接产品启动。

`READY_FOR_0_0_10_P1_4_PRODUCT_LIFECYCLE_BINDING`

## 功能性

### 唯一生命周期

- 新增 `FShanmenItemAuthorityService`，状态固定为 `Closed / Ready / RecoveryRequired`。
- `StartExisting` 只打开既有文档；文件缺失时返回 `MigrationRequired`，不自行读取或导入旧权威。
- `StartFromAuthorizedMigration` 也先尝试打开既有文档。只要有效文档存在，即使传入的 legacy 候选、证据或授权无效，也以既有文档为准，不再碰旧来源。
- 首次创建要求 `FShanmenItemMigrationAuthorization::Explicit(MigrationId)` 与迁移证据中的非空 `MigrationId` 精确匹配；错误或空授权不写盘。
- “检查缺失”和“首次发布”之间若另一个写者创建了文档，服务会有界重开并采用已经持久化的文档，不进行第二次导入。
- 生命周期不可复制；不公开可变 Repository 或 Document 指针。读取只能在锁内复制完整文档或 Snapshot。

### 串行且持久的命令门面

- 四个公开命令为 `ReserveDurable / CommitDurable / CancelDurable / ReleaseDeploymentDurable`。
- 同一服务实例内的启动、写命令、状态读取和测试故障注入全部由同一个 `FCriticalSection` 串行化。
- 命令先取得完整前态，再调用已验证的 `FShanmenItemRepository`，随后把完整后态交给 P1.2 原子 store。
- 成功收据只有在 store 确认主文档后才返回 `Persisted`；精确幂等重放会验证 durable primary 完全一致后返回 `Replayed`，不推进 generation、不写盘。
- 仓库会记录的拒绝也作为权威 ledger 变化持久化，返回 `RejectedAndPersisted`；重启后相同拒绝可得到完全相同的收据。
- `RequestIdConflict` 等不改变 Repository 的拒绝会验证 durable primary 后返回 `RejectedWithoutMutation`。
- `IsDurable()` 与 `IsCommandSuccess()` 分离：被持久化的拒绝属于可重放结果，但不会被误报为业务成功。

### 回滚、歧义与多写者冲突

- 临时目录、写、flush/close、临时回读、临时验证、备份和原子替换任一阶段失败后，服务都会重开 durable primary。
- 若重开文档与命令前文档完全一致，内存 Repository 和 Document 同步恢复，返回 `PersistenceFailedRolledBack`；同一 RequestId 可安全重试一次。
- 若提交后验证失败，但重开文档的 Authority 与命令后 Snapshot 完全一致，服务采用该文档并返回 `ResolvedAfterReopen`，不会重复执行命令。
- 若重开状态既不等于完整前文档，也不等于本命令后态，服务进入 `RecoveryRequired` 并拒绝后续写入。
- 两个实例误连同一存储时，先写者成功；落后实例检测到 generation/document 分歧后失败关闭。最终 durable primary 保持先写者状态。

### 旧权威边界

- 新服务位于 `ShanmenItems`，不引用 `demo_map`、Profile、Code B、Actor、World、GAS、ApplyDamage 或 RNG。
- `Migration.AuthorizedLifecycleHandoff` 使用真实 P1.1 Code A/Profile + Code B 只读候选和收据，先验证缺文档、再显式授权发布 generation 1。
- 测试逐字段确认发布前后旧 Profile 与 Code B Snapshot 不变。
- 本轮没有增加产品启动 hook；现有产品仍不会读取新服务或新文档。

## 完整性与公开结果

启动结果明确区分：既有打开、恢复打开、首次迁移、已经 Ready、需要迁移、迁移未授权、请求无效、持久化失败和 Repository 加载失败。

命令结果明确区分：已持久化、精确重放、拒绝已持久化、无变化拒绝、重开确认提交、失败后回滚、需要恢复和服务未启动。结果同时携带 Repository 收据、保存状态、重开状态、诊断、内存是否发生过变化、磁盘是否变化及当前 generation。

`RecoveryRequired` 不提供可变状态访问，也不允许继续写。调用方必须通过新的受控启动/恢复决策重新建立生命周期，不能在故障实例上盲目重试。

## 自动化

最终命令目标：`Automation RunTests Shanmen.0_0_10`

P1.3 新增 9 项：

1. `Items.AuthorityService.ExistingFirstAndExplicitMigration`
2. `Items.AuthorityService.DurableCommandReplayAndRestart`
3. `Items.AuthorityService.RejectedCommandDurability`
4. `Items.AuthorityService.PersistenceFailureRollback`
5. `Items.AuthorityService.PostCommitAmbiguityReconciled`
6. `Items.AuthorityService.ExternalWriterDivergenceFailsClosed`
7. `Items.AuthorityService.CorruptAuthorityNeverRemigrates`
8. `Items.AuthorityService.ConcurrentCommandsSerialized`
9. `Items.Migration.AuthorizedLifecycleHandoff`

| 范围 | 结果 |
|---|---|
| 既有 P0—P1.2 | 28/28 Success |
| P1.3 AuthorityService | 8/8 Success |
| P1.3 真实 legacy lifecycle bridge | 1/1 Success |
| 合计 | `37/37 Success`、`0 Fail` |

- 最终 UE 自动化退出码：`0`
- 最终日志：`Saved/Logs/Dev.D.UE.0.0.10.P1.3.r0_automation.log`
- 最终日志 SHA-256：`3A6649EA442C9F265AEF6A29D1F401634564A31FB02C09E519000784EA2B4E7F`
- 日志明确记录 `Found 37 automation tests` 与 `TEST COMPLETE. EXIT CODE: 0`。
- 测试启动前 13 条 UE 内置 `Condition failed` 与既有基线相同；37 个目标测试没有 Fail。

## 首次失败与修复

第一次 AuthorityService 专项运行结果为 `0 Success / 7 Fail`，UE 自动化退出码 `-1`。根因是测试夹具把 `CapabilityConsumeQuantity` 和 `CapabilityDeploy` 同时放入一个 Definition；现有权威不变量明确规定 Quantity 能力必须独占，因此服务正确拒绝首次迁移。该失败不是生命周期、文件协议、并发锁或产品源码故障。

修正方式是把测试数据拆成一个数量堆叠物品和一个可部署物品，保留各自合法的 Definition。随后专项测试 `7/7 Success`，再加入多写者冲突和真实 legacy bridge 后完整测试 `37/37 Success`。

- 首次失败日志：`Saved/Logs/Dev.D.UE.0.0.10.P1.3.r0_automation.first.log`
- 首次失败 SHA-256：`89E3A4BCB7B86B25A10971DE6416663E792E3FCF23B01CD1C5A1925CBF958163`
- 修正后专项日志：`Saved/Logs/Dev.D.UE.0.0.10.P1.3.r0_automation.second.log`
- 修正后专项 SHA-256：`0B3320D60666E8D766747A1792F7EB8E7E16F0805E7745FDF70B489C74B9E03B`

## 构建与静态检查

### Editor

- 新增服务首编：7/7 actions，`Result: Succeeded`，退出码 `0`。
- 合法夹具修正：4/4 actions，`Result: Succeeded`，退出码 `0`。
- legacy bridge 增量链接成功；最终语义收紧构建为 8/8 actions，退出码 `0`。

### Game

- 最终构建：5/5 actions，`Result: Succeeded`，退出码 `0`。

Editor 与 Game 均使用：`-WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`。

### 静态

- `git diff --check`：退出码 `0`。
- `ShanmenItems` 的 `demo_map / UWorld / AActor / ApplyDamage / UGameplayAbility / FMath::Rand / FRandomStream` 扫描：全部 0 匹配。
- `FShanmenItemAuthorityService` 产品调用扫描：0；当前调用者只有自动化测试。
- 新服务没有 `UObject`、Actor、UI、World、网络或随机依赖。

## 修改范围

- `Source/ShanmenItems/Public/ShanmenItemAuthorityService.h`
- `Source/ShanmenItems/Private/ShanmenItemAuthorityService.cpp`
- `Source/ShanmenItems/Private/Tests/ShanmenItemAuthorityServiceTests.cpp`
- `Source/demo_map/demo_mapShanmenItemMigrationTests.cpp`
- 本 Report 与同名 Log

工作区原有未跟踪 Prompt、旧 Report、自动化文档及用户文件没有被暂存或提交。

## 未包含与下一步

- 本轮建立唯一生命周期契约，但没有把它注册到产品 GameInstance/Subsystem 启动路径；因此尚未改变 0.0.9B 运行行为。
- `FCriticalSection` 保证单实例线程串行，不是跨进程文件锁。多实例误写会被文档代际检查发现并失败关闭，但正式产品仍必须只创建一个生命周期所有者。
- Code A/Profile 与 Code B 尚未退役、删除或改为 UI-only；它们继续按旧产品路径运行，本轮不双写。
- 尚未实现玩家选择/迁移授权 UI、恢复 UI、云存档、网络复制、装备移动、定义 Manifest 或运行时业务 Adapter。
- P1.4 应在单一启动所有者中：先 `StartExisting`；仅在受控首次升级点构造 P1.1 候选并授权精确 MigrationId；Ready 后让新资源事务只经过本服务，同时将旧来源保持只读，直到端到端验证完成。

## P/F 边界

本轮属于 P 阶段开发与无头验证。执行了静态审查、`git diff --check`、Editor/Game Development 构建及 `UnrealEditor-Cmd -unattended -NullRHI` 自动化；未启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、大规模回归、Cook 或 Package。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p1-3-authority-lifecycle/Docs/Report/Dev.D.UE.0.0.10.P1.3.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p1-3-authority-lifecycle/Docs/Log/Dev.D.UE.0.0.10.P1.3.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p1-3-authority-lifecycle>
