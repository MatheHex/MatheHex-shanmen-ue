# Dev.D.UE.0.0.10.P1.2.r0 开发日志

## 身份

- 任务：`Dev.D.UE.0.0.10.P1.2.r0`
- 分支：`agent/0.0.10-p1-2-authority-document`
- 起点：`646a5b63c8026a161c9c1dbbb325086d4e17e85d`
- 收口时间：`2026-08-27T20:45:37.4449515Z`
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`
- 引擎：Unreal Engine `5.8`

## 开工审查

1. P1.1 已能从 Code A/Profile 与 Code B 只读生成规范化候选和确定性 MigrationId，但尚无新文档。
2. `FShanmenItemAuthoritySnapshot` 已包含 Definition、Container、Item、Reservation 和 processed-request ledger，可作为完整持久化 payload。
3. 旧 Profile store 的 `temp -> verify -> backup -> replace -> readback` 模式可复用，但新权威需要独立 schema、独立错误类型和 MigrationId 重开规则，不能复用旧 Profile JSON。
4. P1.2 不接产品启动；这样可以先独立验证磁盘契约，再在 P1.3 决定切换生命周期。

## 实现记录

### 文档与证据

- 定义 schema-1 `FShanmenItemAuthorityDocument`。
- 将 P1.1 旧模块收据转换为 `FShanmenItemMigrationEvidence`，新模块不引用任何旧类型。
- 保存初始与当前 Snapshot SHA-256；DocumentId 由 OwnerId + MigrationId 确定性派生。
- JSON 使用精确顶层/迁移字段检查和严格 Authority 反射导入。

### 保存

- 候选先经 Repository 规范化和 Owner 闭包验证。
- 临时文件 full flush 后回读；备份和主文件都逐字节验证。
- primary 必须与调用方文档完全一致才允许推进 generation。
- commit 后回读成功才更新调用方内存。
- 无变化保存也验证 durable primary，不能以 no-op 掩盖磁盘分歧。

### 加载与恢复

- 有效 primary 纯读取返回。
- 坏 primary + 有效 backup：保存坏原字节、验证临时恢复文件、同卷替换、再回读。
- 缺 primary + 有效 backup：同协议恢复。
- 未来 schema 失败关闭，不回退旧 backup。
- 无有效 backup 时不从 legacy 重新生成。

### 幂等首次发布

- 首次迁移提交 generation 1。
- 同 MigrationId/evidence/initial digest 重开，不重复导入。
- 证据或初始候选冲突拒绝覆盖。
- 首次发布和后续保存的 post-commit ambiguity 都由重开解析，而不是重放命令。

## 自动化记录

新增 7 项：

1. `Migration.PersistedIdempotentOpen`
2. `PersistenceDocument.RoundTripGeneration`
3. `PersistenceDocument.MigrationConflict`
4. `PersistenceDocument.PreCommitFailureIsolation`
5. `PersistenceDocument.AmbiguousCommitReopen`
6. `PersistenceDocument.BackupRecoveryAndFutureSchema`
7. `PersistenceDocument.NoSilentReset`

故障注入覆盖目录创建、临时写、flush/close、临时回读、临时验证、备份、原子替换、提交后回读与提交后清理。最终整个 `Shanmen.0_0_10` 命名空间为 28/28 Success、0 Fail、队列正常清空、原生退出码 0。

## 自查与修正

### SHA 平台 hook

- 初始实现调用 `FPlatformMisc::GetSHA256Signature`。
- Editor/Game 编译均成功，但前两次无头运行在首个 digest 处触发 UE 5.8 Windows 通用实现断言：`No SHA256 Platform implementation`，进程退出码 3。
- 调查引擎源码确认该函数在 `GenericPlatformMisc.cpp` 明确为未实现 hook。
- 改为 Build.cs 显式链接 Unreal Engine 随附 OpenSSL，并直接调用其 SHA-256；随后完整自动化通过。

### 最终审查补强

- 增加 no-op 保存对 durable primary 的一致性验证。
- 增加首次迁移已替换但回读失败后的同 MigrationId 重开测试。
- 明确未来 schema 不允许通过 backup 隐式降级。

## 验证记录

### Editor

- 初次：16/16 actions，退出码 0。
- SHA 修正：12/12 actions，退出码 0。
- 最终：5/5 actions，退出码 0。

### Game

- 初次：12/12 actions，退出码 0。
- 最终：4/4 actions，退出码 0。

### Automation

- 最终：28/28 Success，0 Fail，原生退出码 0。
- 队列：`Automation Test Queue Empty 28 tests performed`。
- 最终日志：`Saved/Logs/Dev.D.UE.0.0.10.P1.2.r0_automation.log`。
- 最终 SHA-256：`308BAD5BC2B78E4C9446B259C7E1093B102B9C6712E19884D278052AFCFE369E`。
- 保留首次失败日志：`Saved/Logs/Dev.D.UE.0.0.10.P1.2.r0_automation.first.log`；SHA-256 `FCE0F1AC57BA1EF7649B68E17EA8EE11E971DEA8696CA69B33C10E19C536CCC1`。
- 13 条测试前 UE 内置 Condition 基线不属于目标测试。

### 静态

- `git diff --check`：退出码 0。
- `ShanmenItems` 禁止旧模块/World/Actor/ApplyDamage/GAS/RNG/未实现 SHA hook 扫描：0 匹配。
- 新 store 的产品调用：0；只有定义与测试。

## 交付边界

- 不启动 UI、PIE、Standalone 或产品。
- 不执行真实输入、Smoke、Cook 或 Package。
- 不写 Code A/Profile 或 Code B。
- 不接产品启动，不删除旧权威，不上传 Saved 日志二进制。
- 仅暂存 P1.2 的 7 个源码文件与本 Report/Log；工作区其它未跟踪文件保持原状。
