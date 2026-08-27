# Dev.D.UE.0.0.10.P1.2.r0 Report

## 结论

P1.2 已完成并通过验证。0.0.10 的唯一物品权威现在具备版本化 JSON 文档、严格解析、确定性迁移身份、完整 Snapshot SHA-256、单调保存代数、原子发布、前代备份、坏主文件隔离恢复、未来 schema 拒绝以及 MigrationId 幂等重开。

本轮没有把新权威接入产品启动，没有修改或双写 Code A/Profile 与 Code B，也没有启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件。

`READY_FOR_0_0_10_P1_3_AUTHORITY_LIFECYCLE`

## 功能性

### 版本化权威文档

- 新增 `FShanmenItemAuthorityDocument`，当前 schema 为 `1`。
- 文档完整保存 Owner、DocumentId、SaveGeneration、UTC 时间、一次迁移证据、初始 Snapshot digest、当前 Snapshot digest 以及完整 `FShanmenItemAuthoritySnapshot`。
- DocumentId 由 OwnerId 与 MigrationId 确定性派生；同一迁移不会创建第二份身份。
- JSON 顶层与迁移证据要求精确字段集合；未知、缺失、错误类型、非整数或未来 schema 均失败关闭。
- Authority 反射 JSON 使用严格导入，并在接受前重新经过 `FShanmenItemRepository` 规范化和全量不变量验证。

### 原子保存协议

保存链路固定为：

1. 规范化并验证候选 Snapshot、Owner、digest 与 generation。
2. 写入同目录 `.tmp` 并执行 full flush/close。
3. 重新读取 `.tmp`，严格解析并逐字节、逐文档比对。
4. 验证当前 primary 与调用方 generation 完全一致。
5. 将旧 primary 复制为 `.bak` 并逐字节复核。
6. 同卷替换 `.tmp -> primary`。
7. 重新读取 primary，验证字节、schema、digest 与完整文档。
8. 只有第 7 步成功后才更新调用方内存文档。

变更保存只推进一次 `SaveGeneration`。无变化保存不写盘、不推进 generation，但仍会读取 primary 并确认磁盘状态与调用方完全一致，避免陈旧调用方或坏文件被误报为成功。

### 幂等迁移发布与恢复

- 首次发布将 P1.1 候选原子提交为 generation `1`。
- 同一 MigrationId、来源证据和初始 Snapshot 重开既有文档，不重新读取或导入旧权威。
- 不同 MigrationId、来源证据或初始 Snapshot 返回 `MigrationConflict`，不覆盖既有文档。
- 即使首次发布已完成替换、但提交后回读失败，下一次同 MigrationId 调用也会识别并重开已发布文档。
- primary 损坏而 backup 有效时，先原样保存坏字节到 `Corrupt/`，再通过同一临时验证与替换协议恢复 backup。
- primary 缺失而 backup 有效时可恢复；未来 schema 绝不由旧 backup 降级覆盖。
- primary 损坏且无有效 backup 时明确失败，不从旧 Code A/Code B 静默重建。

### 依赖边界

- `ShanmenItems` 仍不依赖 `demo_map`、Code A、Code B、Actor、World、GAS、ApplyDamage 或随机数。
- 旧模块仅把 P1.1 迁移收据转换为 legacy-independent persistence evidence。
- 新 store 当前只有测试调用者；没有产品启动 hook。
- SHA-256 使用 Unreal Engine 随附的 OpenSSL 静态实现，避免调用 UE 5.8 Windows 未实现的 `FPlatformMisc` SHA hook。

## 完整性与失败语义

公开结果类型区分：

- 保存：验证拒绝、序列化失败、临时写/flush/验证失败、备份失败、原子替换失败、提交后验证失败。
- 加载：主文件成功、坏主文件恢复、缺主文件恢复、缺失、未来 schema、坏主文件且无有效备份、读/恢复写失败。
- 打开：首次迁移发布、既有重开、恢复后重开、无效请求、迁移冲突、持久化失败。

故障注入只在 `WITH_DEV_AUTOMATION_TESTS` 下存在，覆盖目录、临时写、flush/close、临时回读、临时验证、备份、替换、提交后回读和清理阶段；不会进入 Shipping 数据结构。

## 自动化

最终命令目标：`Automation RunTests Shanmen.0_0_10`

| 范围 | 结果 |
|---|---|
| 既有 P0/P1/P1.1 | 21/21 Success |
| `Migration.PersistedIdempotentOpen` | Success |
| `PersistenceDocument.RoundTripGeneration` | Success |
| `PersistenceDocument.MigrationConflict` | Success |
| `PersistenceDocument.PreCommitFailureIsolation` | Success |
| `PersistenceDocument.AmbiguousCommitReopen` | Success |
| `PersistenceDocument.BackupRecoveryAndFutureSchema` | Success |
| `PersistenceDocument.NoSilentReset` | Success |
| 合计 | `28/28 Success`、`0 Fail` |

- 队列：`Automation Test Queue Empty 28 tests performed`
- 最终原生退出码：`0`
- 最终日志：`Saved/Logs/Dev.D.UE.0.0.10.P1.2.r0_automation.log`
- 最终日志 SHA-256：`308BAD5BC2B78E4C9446B259C7E1093B102B9C6712E19884D278052AFCFE369E`
- 测试启动前 13 条 UE 内置 `Condition failed` 与 P0/P1/P1.1 基线一致；28 个目标测试没有 Error、Warning 或 Fail。

## 首次失败与修复

初始两次无头自动化都在首个 P1.2 digest 调用处原生退出码 `3`。崩溃日志明确为：

`GenericPlatformMisc.cpp:2021 — No SHA256 Platform implementation`

根因是 UE 5.8 Windows 的 `FPlatformMisc::GetSHA256Signature` 保留了通用未实现断言；不是 JSON、文件协议、测试断言或 Win64 SDK 失败。改为 Unreal Engine 随附 OpenSSL 的 `SHA256` 后，Editor/Game 均重新链接成功，随后完整 28 项自动化原生退出码 `0`。保留的首次失败日志为 `Saved/Logs/Dev.D.UE.0.0.10.P1.2.r0_automation.first.log`，SHA-256 为 `FCE0F1AC57BA1EF7649B68E17EA8EE11E971DEA8696CA69B33C10E19C536CCC1`。

## 构建与静态检查

### Editor

- 首次实现构建：16/16 actions，`Result: Succeeded`，退出码 `0`。
- SHA 修正构建：12/12 actions，`Result: Succeeded`，退出码 `0`。
- 最终增量构建：5/5 actions，`Result: Succeeded`，退出码 `0`。

### Game

- 首次完整增量：12/12 actions，`Result: Succeeded`，退出码 `0`。
- 最终增量：4/4 actions，`Result: Succeeded`，退出码 `0`。

两目标均使用：`-WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`。

### 静态

- `git diff --check`：退出码 `0`。
- `ShanmenItems` 的 `demo_map / UWorld / AActor / ApplyDamage / GameplayAbility / AbilitySystemComponent / RNG / 未实现 FPlatformMisc SHA` 扫描：0 匹配。
- store API 的生产调用扫描：仅定义与自动化测试；没有启动时产品接线。

## 修改范围

- `Source/ShanmenItems/ShanmenItems.Build.cs`
- `Source/ShanmenItems/Public/ShanmenItemPersistence.h`
- `Source/ShanmenItems/Private/ShanmenItemPersistence.cpp`
- `Source/ShanmenItems/Private/Tests/ShanmenItemPersistenceTests.cpp`
- `Source/demo_map/demo_mapShanmenItemMigration.h`
- `Source/demo_map/demo_mapShanmenItemMigration.cpp`
- `Source/demo_map/demo_mapShanmenItemMigrationTests.cpp`
- 本 Report 与同名 Log

工作区原有未跟踪 Prompt、旧 Report、自动化文档及用户文件没有被暂存或提交。

## 未包含与下一步

- P1.2 建立持久化机制，但尚未决定产品启动生命周期中何时从 Code A/Code B 切换到新文档。
- 尚未实现跨进程多写者锁；后续产品接入必须由单一生命周期所有者串行调用 authority store。
- 尚未实现云存档、网络复制、UI、装备移动、定义 Manifest 或运行中旧权威删除。
- 在真实产品接入前，Code A/Profile 与 Code B 继续保持原样；本轮不会令现有版本读取新文档。

建议 P1.3 建立唯一 authority lifecycle/facade：明确启动检测、首次迁移授权点、既有文档优先、写入串行化、失败呈现和旧系统只读退役顺序，同时保持 UI/Actor 仍不直接改库存。

## P/F 边界

本轮属于 P 阶段开发与无头验证。执行了静态审查、`git diff --check`、Editor/Game Development 构建及 `UnrealEditor-Cmd -unattended -NullRHI` 自动化；未启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、大规模回归、Cook 或 Package。

## GitHub

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p1-2-authority-document/Docs/Report/Dev.D.UE.0.0.10.P1.2.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p1-2-authority-document/Docs/Log/Dev.D.UE.0.0.10.P1.2.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p1-2-authority-document>
