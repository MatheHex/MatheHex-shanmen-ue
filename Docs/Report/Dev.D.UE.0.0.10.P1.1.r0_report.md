# Dev.D.UE.0.0.10.P1.1.r0 Report

## 结论

P1.1 已完成并可进入 P1.2。0.0.9B Profile Schema 6→当前版本的原子升级缺口已修复；Code A Profile 与已提交 Code B sidecar 之间建立了只读、确定性、fail-closed 的 0.0.10 `ShanmenItems` 迁移候选协议。迁移保留物品身份、数量、装备位置和一级物品子容器，并在任何来源冲突、活动 Run 或容器闭包异常时拒绝切换。

## 功能性

- 将旧 schema 支持规则改为相对当前版本的 `1 <= old < current`，修复 Schema 6→7 被固定 `<= 5` 判定拒绝的问题。
- 复用 P5 handoff 的原始 SourceFingerprint 与 legacy affix digest 实现，不创建第二套证据算法。
- 新增只读 `BuildCandidate`：核验 Profile、Code B 终态、所有权、来源指纹、同 ID 映射、Definition、Quantity、affix、装备位置、空间父子关系和完整容器闭包。
- 拒绝 Code A 或 Code B 的活动 Run，禁止局中迁移。
- 确定性生成局外 scope、候选 digest 和 MigrationId；相同来源重放得到相同快照与收据。
- `FShanmenItemInstance` 新增 `ChildContainerId`，保留空间戒指/背包等物品拥有的一级容器。
- 新权威验证拒绝重复子容器所有者、父子同容器、Owner/scope 不一致、多层嵌套及耗尽物品残留子容器。
- 候选在返回前通过临时 `FShanmenItemRepository::TryLoadSnapshot`；无效候选不能替换既有权威。

## 完整性与兼容性

- 新模块依赖方向保持为 `demo_map -> ShanmenItems`；`ShanmenItems` 对 `demo_map`、Code A、Code B 仍为零依赖。
- 不修改 Code A/Code B 来源，不在线双写，不删除旧档，不接产品启动。
- 只为旧 stackable Definition 投影 Quantity 能力；不臆造耐久、充能或部署能力。
- 新增四组自动化：Schema 6 原子迁移、确定性候选、来源冲突 fail-closed、无效加载原子隔离。
- 工作区原有未跟踪 Prompt、Report 与其他用户文件均未暂存、未修改。

## 修改范围

- `Source/demo_map/demo_mapProfileRepository.cpp`
- `Source/demo_map/CodeB/demo_mapCodeBOutOfRaidProfile.h`
- `Source/demo_map/CodeB/demo_mapCodeBOutOfRaidProfile.cpp`
- `Source/demo_map/demo_map.Build.cs`
- `Source/demo_map/demo_mapShanmenItemMigration.h`
- `Source/demo_map/demo_mapShanmenItemMigration.cpp`
- `Source/demo_map/demo_mapShanmenItemMigrationTests.cpp`
- `Source/ShanmenItems/Public/ShanmenItemTypes.h`
- `Source/ShanmenItems/Private/ShanmenItemTypes.cpp`
- `Source/ShanmenItems/Private/ShanmenItemRepository.cpp`
- `Docs/Architecture/Dev.D.UE.0.0.10_ItemMigration_ADR.md`
- `Docs/Report/Dev.D.UE.0.0.10.P1.1.r0_report.md`
- `Docs/Log/Dev.D.UE.0.0.10.P1.1.r0_log.md`

## 验证

### Automation

- 命令目标：`Automation RunTests Shanmen.0_0_10`
- 发现并执行：21
- Success：21
- Fail：0
- 队列：`Automation Test Queue Empty 21 tests performed`
- 原生退出码：`0`
- 最终日志：`Saved/Logs/Dev.D.UE.0.0.10.P1.1.r0_automation.log`
- SHA-256：`7EF1BE409258281242454F689FEDE2F060EA8ADCF35EFEFF6EFFF1EBC89416D6`
- 测试启动前 13 条 UE 内置 `Condition failed` 与 P1.0 基线一致；Shanmen 测试无 Error、Warning 或 Fail。

### Editor 构建

- 目标：`demo_mapEditor Win64 Development`
- 参数：`-WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`
- 首次完整构建：199 actions，`Result: Succeeded`，549.23 秒，退出码 `0`。
- 最终清理后增量构建：4 actions，`Result: Succeeded`，12.18 秒，退出码 `0`。

### Game 构建

- 目标：`demo_map Win64 Development`
- 参数：`-WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`
- 首次完整构建：195 actions，`Result: Succeeded`，493.65 秒，退出码 `0`。
- 最终清理后增量构建：3 actions，`Result: Succeeded`，13.99 秒，退出码 `0`。

### 静态证据

- `git diff --check`：退出码 `0`。
- `ShanmenItems` 禁止旧模块/World/Actor/ApplyDamage/GAS/RNG 扫描：0 匹配。
- 新迁移代码不包含文件写入、Actor、World 或产品入口。

## P/F 边界

本轮属于开发与无头验证：未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未执行真实输入、截图、Smoke、大规模回归、Cook 或 Package。`UnrealEditor-Cmd` 仅以 `-unattended -NullRHI` 执行目标自动化。

## 已知边界与下一步

- P1.1 只生成并验证候选，不持久化 0.0.10 新权威文档。
- P1.2 应实现新文档序列化、原子发布、MigrationId 幂等重开、备份恢复与故障注入。
- 新权威持久化成功前，不得删除或覆盖 Code A/Code B。

`READY_FOR_0_0_10_P1_2_PERSISTENCE_DOCUMENT`
