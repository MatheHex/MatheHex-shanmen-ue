# Dev.D.UE.0.0.10.P1.3.r0 开发日志

## 身份

- 任务：`Dev.D.UE.0.0.10.P1.3.r0`
- 分支：`agent/0.0.10-p1-3-authority-lifecycle`
- 起点：`a480b94e06cb105c607c8f69b433eba0b125a47c`
- 收口时间：`2026-08-27T21:22:37.6838968Z`
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`
- 引擎：Unreal Engine `5.8`

## 开工审查

1. P1.1 已能只读合并 Code A/Profile 与 Code B，生成规范化候选和确定性 MigrationId。
2. P1.2 已能把候选保存为 schema-1 权威文档，并提供 generation、原子保存、备份恢复和故障注入，但调用方仍可直接组合 Repository 与 Store。
3. `FShanmenItemRepository` 会把成功请求和大多数拒绝写入 processed-request ledger；因此“业务拒绝”也可能是必须持久化的权威变化。
4. Store 的提交后回读失败具有歧义：文件可能已替换成功。调用层不能直接回滚，也不能盲目重放，必须重开并比较完整前态/后态。
5. 现有产品启动仍属于旧系统。P1.3 只建立纯 C++ 生命周期/门面和测试，不在尚未完成端到端策略前改动产品启动或双写旧权威。

## 设计决策

### 启动

- 所有启动都先 `LoadExisting`；legacy 参数在确认 Missing 前不读取。
- `StartExisting` 对 Missing 只返回 `MigrationRequired`。
- `StartFromAuthorizedMigration` 需要显式 capability 与 Evidence.MigrationId 精确一致。
- 初始 missing probe 与 create 之间发生竞争时，只重开一次并采用 durable winner。
- 损坏、未来 schema、读取失败或无法安装 Snapshot 均进入 `RecoveryRequired`，不回迁。

### 命令

- 由一个 `FCriticalSection` 串行化四个命令和生命周期状态。
- 每次命令保存完整 BeforeDocument、BeforeSnapshot、Receipt 和 AfterSnapshot。
- Repository 有变化时必须推进文档 generation；无变化重放也要用 P1.2 no-op 保存验证 durable primary。
- store 成功后按收据区分成功/拒绝；不把“拒绝已持久化”误报成业务成功。

### 失败解析

- 任一 Save 失败后立即 `LoadExisting` 一次。
- 重开文档完全等于 BeforeDocument：恢复内存前态，标记可重试回滚。
- Repository 确有变化且重开 Authority 等于 AfterSnapshot：采用重开文档，标记提交后歧义已解析。
- 其它差异：进入 `RecoveryRequired`，不继续写。

## 实现记录

### 新公开 API

- `EShanmenItemAuthorityServiceState`
- `EShanmenItemAuthorityStartStatus`
- `EShanmenItemDurableCommandStatus`
- `FShanmenItemMigrationAuthorization`
- `FShanmenItemAuthorityStartResult`
- `FShanmenItemDurableCommandResult`
- `FShanmenItemAuthorityService`

### 封装

- Service 不可复制。
- Repository、Document、Storage 和 Store 均为私有成员。
- 读取接口只返回锁内复制；RecoveryRequired 时拒绝读取可用权威快照。
- 故障注入 setter 仅在 `WITH_DEV_AUTOMATION_TESTS` 中存在。

### durable command 语义

- `Persisted`：Repository 成功收据已保存。
- `Replayed`：成功收据精确重放，durable primary 已验证，无写盘。
- `RejectedAndPersisted`：失败收据已进入 ledger 并保存。
- `RejectedWithoutMutation`：拒绝未改变 Repository，durable primary 已验证。
- `ResolvedAfterReopen`：Save 未能确认，但重开证明准确后态已落盘。
- `PersistenceFailedRolledBack`：重开证明 durable primary 仍是完整前态，内存已恢复。
- `RecoveryRequired`：重开无法证明前态或后态，停止后续命令。

## 自动化记录

新增 9 项：

1. `AuthorityService.ExistingFirstAndExplicitMigration`
2. `AuthorityService.DurableCommandReplayAndRestart`
3. `AuthorityService.RejectedCommandDurability`
4. `AuthorityService.PersistenceFailureRollback`
5. `AuthorityService.PostCommitAmbiguityReconciled`
6. `AuthorityService.ExternalWriterDivergenceFailsClosed`
7. `AuthorityService.CorruptAuthorityNeverRemigrates`
8. `AuthorityService.ConcurrentCommandsSerialized`
9. `Migration.AuthorizedLifecycleHandoff`

覆盖内容：

- 缺文档不自动迁移；错误 capability 不写盘；既有文档优先。
- Reserve/Commit/Cancel/Release 四个 durable wrapper。
- 成功与拒绝收据跨重启精确重放。
- 7 个提交前故障点逐项恢复完整内存和主文件字节。
- 首次发布与后续命令的提交后回读歧义。
- 损坏文档禁止回迁。
- 8 条并发命令通过一个实例串行，产生准确的 8 个 generation。
- 双实例外部写者分歧失败关闭，较新文档不被覆盖。
- 真实 P1.1 legacy candidate/evidence 经显式授权进入 Service，旧 Profile 与 Code B 保持逐字段不变。

## 首次失败、自查与修正

首轮新增 7 项全部 Fail，UE 自动化退出码 `-1`。所有失败都从首次迁移 fixture 被拒绝开始。

审查 `FShanmenItemDefinition::IsValid()` 后确认，现有合同要求 Quantity 能力与 Deployment/Durability/Charges 互斥；测试 fixture 错误地把 `CapabilityConsumeQuantity` 与 `CapabilityDeploy` 放进同一个 Definition。服务按设计拒绝了非法候选。

修正为两个 Definition、两个 Item 和同一 Container 的两个 slot：

- 数量物品只支持 Quantity。
- 可部署物品只支持 DeploymentLock。

修正后专项 7/7 Success。随后追加 external-writer 和真实 legacy bridge，最终全量 37/37 Success。

## 验证记录

### Editor

- 首次服务实现：7/7 actions，退出码 `0`。
- fixture 修正：4/4 actions，退出码 `0`。
- legacy bridge：4/4 actions，退出码 `0`。
- 最终语义收紧：8/8 actions，退出码 `0`。

### Game

- 最终：5/5 actions，退出码 `0`。

构建命令统一为：

`Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`

### Automation

- 首次专项：0 Success、7 Fail，UE 自动化退出码 `-1`。
- 修正后专项：7/7 Success、0 Fail，退出码 `0`。
- 最终全量：37/37 Success、0 Fail，退出码 `0`。
- 最终记录：`Found 37 automation tests`、`TEST COMPLETE. EXIT CODE: 0`。
- 最终日志：`Saved/Logs/Dev.D.UE.0.0.10.P1.3.r0_automation.log`。
- 最终 SHA-256：`3A6649EA442C9F265AEF6A29D1F401634564A31FB02C09E519000784EA2B4E7F`。
- 首次失败日志 SHA-256：`89E3A4BCB7B86B25A10971DE6416663E792E3FCF23B01CD1C5A1925CBF958163`。
- 修正后专项日志 SHA-256：`0B3320D60666E8D766747A1792F7EB8E7E16F0805E7745FDF70B489C74B9E03B`。
- 13 条目标测试前 UE 内置 Condition 基线不属于 37 个目标测试。

### 静态

- `git diff --check`：退出码 `0`。
- `ShanmenItems` 中 `demo_map / UWorld / AActor / ApplyDamage / UGameplayAbility / FMath::Rand / FRandomStream`：0 匹配。
- Service 产品调用：0；只有定义与自动化。

## 交付边界

- 不启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件。
- 不执行真实输入、截图、Smoke、Cook 或 Package。
- 不修改或写入 Code A/Profile 与 Code B。
- 不接产品启动，不删除旧权威，不上传 Saved 自动化日志。
- 只暂存 P1.3 的 4 个源码文件与本 Report/Log；工作区其它未跟踪文件保持原状。

## 下一步

P1.4 建议建立产品级唯一 lifecycle owner：已有文档直接打开；仅在首次升级且来源稳定时构造 P1.1 候选并显式授权；所有新资源事务改由 Service 调用。先保持旧系统只读可回查，端到端验证完成后再安排旧写路径退役。
