# Dev.D.UE.0.0.10 物品权威迁移 ADR

## 状态

- 决议：Accepted
- 阶段：`Dev.D.UE.0.0.10.P1.1.r0`
- 日期：2026-08-27
- 前置：`Dev.D.UE.0.0.10_ItemAuthority_ADR.md`、`Dev.D.UE.0.0.10_ItemTransactions_ADR.md`

## 背景

0.0.9B 同时存在 Profile/Code A 的局外物品记录和 Code B 的局外容器图。0.0.10 已在 `ShanmenItems` 建立唯一物品事务权威，但不能通过在线双写或盲目信任其中一份旧数据来切换权威。旧 Profile 的 Schema 6→7 路径还存在固定 `<= 5` 判断，导致真实旧档不能完成既有原子升级。

## 决议

### 1. 迁移方向

- 迁移适配器位于旧 `demo_map` 模块，依赖新 `ShanmenItems`。
- `ShanmenItems` 不依赖 `demo_map`、Code A 或 Code B。
- P1.1 只生成候选快照与迁移收据；不写旧档、不写新档、不接产品启动。
- 只有后续持久化阶段成功发布新权威文档后，旧来源才可退出权威路径。

### 2. 旧档 schema

- 支持的旧版本定义为 `1 <= SchemaVersion < CurrentSchemaVersion`，不再复制固定版本列表。
- 旧档先通过现有逐版本 promotion 转为当前结构，再执行当前结构验证。
- 原子迁移继续保留原始旧字节备份；重开当前版本不得再次提升 `SaveGeneration`。

### 3. Fail-closed 来源协议

迁移仅在下列事实全部成立时返回候选：

- Code A Profile 为当前 schema 且通过 Profile 全量验证。
- Code A 和 Code B 都没有活动 Run；禁止中途迁移。
- Code B 为当前 schema、已提交的终态局外记录，且无 Run-local 容器。
- ProfileId、Code B OwnerId 与 handoff SourceProfileId 完全一致。
- 使用 P5 handoff 同一实现重算 `SourceFingerprint` 与 legacy affix digest。
- handoff 映射为无重复的一对一同 ID 映射，物品数完全一致。
- 每个物品的 ID、Definition、Quantity、affix digest 与 stack 语义完全一致。
- Code A 的装备引用、legacy spatial parent 与 Code B 实际布局完全一致。
- Code B 容器集合必须闭合于已提交布局根和物品拥有的一级子容器；额外隐藏容器直接拒绝。

任何不一致都返回精确错误类别，不选择“较新”或“看起来更完整”的一侧。

### 4. 规范化候选

- Definition、Container、Item 按稳定键排序。
- 局外 scope、候选 digest 与 MigrationId 均由来源证据确定性派生。
- 迁移保留 ItemInstanceId、DefinitionId、Quantity、ParentContainerId、SlotIndex 及一级 `ChildContainerId`。
- 旧 stackable 物品只获得 Quantity 消耗能力；迁移不臆造耐久、充能或部署能力。
- 候选必须先由临时 `FShanmenItemRepository` 完整加载验证，调用方才能接收。

### 5. 一级物品容器

`FShanmenItemInstance` 增加可持久化 `ChildContainerId`。权威验证要求：

- 子容器存在，且不能等于父容器。
- 子容器与物品具有相同 Owner 和 scope。
- 一个子容器只能被一个物品拥有。
- 已耗尽物品不能保留子容器。
- 子容器内物品不能再拥有子容器，避免多层图和环。

## 不在 P1.1 范围

- 不创建 0.0.10 新磁盘文档或启动时切换器。
- 不删除、覆盖或同步写入 Code A/Code B。
- 不迁移活动 Run。
- 不接 Actor、World、UI、PIE、Cook 或 Package。
- 不为旧物品推断尚未冻结的 0.0.10 战斗能力。

## 后续

P1.2 应实现版本化的新权威文档、`temp -> verify -> backup -> replace` 原子发布、MigrationId 幂等重开和故障注入测试；发布成功前继续保持旧来源只读。
