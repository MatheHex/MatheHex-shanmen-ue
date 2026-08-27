# Dev.D.UE.0.0.10.P1.1.r0 开发日志

## 身份

- 任务：`Dev.D.UE.0.0.10.P1.1.r0`
- 分支：`agent/0.0.10-p1-1-persistence-migration`
- 起点：`22d53aa6cf3be24425c3111e641ef094d8f602ed`
- 收口时间：`2026-08-27T20:10:58.3663453Z`
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`
- 引擎：Unreal Engine `5.8`

## 边界审查

1. 确认 Code A/Profile 保存局外物品和 legacy spatial parent，Code B 保存已提交局外容器图、P5 来源收据与一级物品子容器。
2. 确认 P1.0 `ShanmenItems` 尚无 `ChildContainerId`，直接迁移会丢失空间戒指/背包内部容器所有权。
3. 确认 Profile 当前 schema 为 7，而 `IsExactLegacySourceFor` 仍固定接受 1..5，真实 Schema 6→7 promotion 无法完成精确来源核验。
4. 决定适配器驻留旧模块，保持新权威模块不依赖任何旧系统。

## 实现记录

### Profile schema

- 新增 `IsSupportedLegacySchema`，统一解析、promotion 与 exact-source 判断。
- 支持范围相对 `CurrentSchemaVersion` 计算，消除固定版本清单和 `<= 5` 缺口。

### 统一来源证据

- 在 Code B store 暴露 P5 已有 affix digest 与 Profile source fingerprint 的只读包装。
- P5 初始记录也改用同一公开包装，保证旧 handoff 与 0.0.10 迁移调用相同实现。

### 迁移候选

- 新增 `Fdemo_mapShanmenItemMigration::BuildCandidate`。
- 验证当前 Profile、终态 Code B、无活动 Run、所有权、来源指纹、映射、物品语义、装备布局和容器闭包。
- 规范化 Definition/Container/Item 顺序并确定性派生 scope、digest、MigrationId。
- 使用临时 `FShanmenItemRepository` 验证候选；来源始终只读。

### 子容器拓扑

- `FShanmenItemInstance` 增加 `ChildContainerId` 并纳入相等性。
- Repository 增加存在性、唯一所有者、Owner/scope、非自指、非耗尽和一级深度验证。

### 自动化

- `Schema6ToCurrent`：旧 schema 原子升级、原字节备份、只提升一次 generation。
- `DeterministicCandidate`：双次重放一致、来源不变、空间父子拓扑保留、候选可加载。
- `SourceConflictsFailClosed`：无 Content、Owner、fingerprint、Quantity、隐藏容器和活动 Run 冲突。
- `AtomicLoadIsolation`：坏子容器候选不能替换既有权威。

## 自查修正

- 首轮实现后清除一个未使用的 Code A 局部索引，不改变契约。
- 清理后重新执行 Editor/Game 增量构建和完整 21 项自动化，确保最终证据对应最终源码。
- 首次自动化调用误用了绝对 `-log` 与提前 `Quit` 组合：进程原生退出码为 `0`，但没有产生要求的日志，因此未把它记作测试成功。改回已验证的相对 `-log` + `TestExit` 调用后取得完整队列证据。

## 验证记录

### Editor

- 首次：199/199 actions，549.23 秒，退出码 `0`。
- 最终增量：4/4 actions，12.18 秒，退出码 `0`。

### Automation

- 最终联合集：21/21 Success，0 Fail，队列正常清空。
- P1.1 新测试：4/4 Success。
- 原生退出码：`0`。
- 日志 SHA-256：`7EF1BE409258281242454F689FEDE2F060EA8ADCF35EFEFF6EFFF1EBC89416D6`。
- 13 条测试前 UE 内置 Condition 基线保持不变。

### Game

- 首次：195/195 actions，493.65 秒，退出码 `0`。
- 最终增量：3/3 actions，13.99 秒，退出码 `0`。

### 静态

- `git diff --check`：退出码 `0`。
- `ShanmenItems` 旧模块/World/Actor/ApplyDamage/GAS/RNG 扫描：0 匹配。
- 未启动 Editor UI、PIE、Standalone、产品、Cook 或 Package。

## 交付边界

本轮不写新 0.0.10 磁盘文档、不接启动、不双写旧权威。P1.2 才实现原子持久化与幂等 reopen。工作区原有未跟踪文件未暂存。
