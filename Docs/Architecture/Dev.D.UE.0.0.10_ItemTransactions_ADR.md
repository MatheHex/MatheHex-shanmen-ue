# 0.0.10 物品资源事务 ADR

## 状态

`Accepted`，对应 P1.0。

## 决策

1. `FShanmenItemRepository` 是 0.0.10 物品图、预留、部署状态和幂等 ledger 的唯一可变写入点。
2. 跨模块只提交值类型 Request，并接收不可变 Receipt；技能、Actor、GAS、战斗 Resolver 和 UI 不获得 Repository 内部可写引用。
3. 消费与部署统一执行 `Validate -> Reserve -> Commit Point -> Commit/Cancel -> Receipt`。
4. 同一 `RequestId + canonical payload` 必须重放完全相同的 Receipt；同一 RequestId 携带不同 payload 时返回 `RequestIdConflict`，不得覆盖原结果。
5. 成功和失败 Receipt 都进入持久幂等 ledger。AuthorityRevision 覆盖物品图、预留与 ledger 的全部变化，避免同 revision 对应不同可重放状态。
6. Snapshot 必须同时保存 Definition、Container、ItemInstance、Reservation 和 ProcessedRequest；加载前验证完整闭包，失败不能部分替换当前权威。

## 资源通道

- `Quantity`：暗器、阵材等真实堆叠数量；Reserve 不扣除，Commit 才扣除。
- `DeploymentLock`：飞剑等不应被消耗的真实实例；Commit 将同一 ItemInstance 置为 Deployed，ReleaseDeployment 恢复 Stored。
- `Durability`：命中或受击后的实例耐久提交；部署中的实例仍可使用该通道。
- `Charges`：护心镜等条件触发型灵器次数；仅在确定触发的 Commit Point 扣除。

P1.0 中 Quantity 是实例的排他资源形态，不能与 DeploymentLock、Durability 或 Charges 同时定义。这样避免把一个堆叠中每个单位的独立耐久／充能错误压进同一个标量。需要独立状态的对象必须使用独立 ItemInstance 或后续正式的实例 Fragment 模型。

## 生命周期与失败语义

- 数量完全耗尽时保留 `Depleted` 审计墓碑并清除容器槽位，保证历史 Receipt 仍能解析到原 ItemInstanceId。
- DeploymentLock 在 Reserved 阶段排斥其它资源预留；进入 Deployed 后允许 Durability/Charges 事务，但拒绝再次部署。
- 多个 Quantity 预留可以并存；总预留不得超过当前数量。先提交一个预留引起的 ItemRevision 变化不能使另一个已成立预留失效。
- Cancel 只释放预留，不改变物品资源；Commit 后再 Cancel、Cancel 后再 Commit 都失败关闭。
- ContentStamp、RunId、OwnerId、ExpectedItemRevision 和完整容器闭包都必须匹配。

## 未包含

- 本 ADR 不定义伤害、技能、阵法效果或 GAS 编排。
- 不包含真实磁盘／云存档 I/O、Code A／Code B 迁移器、装备移动、UI 或网络复制。
- ProcessedRequest 的结算期裁剪策略留到持久化实现阶段，但在 Run 活动期间不得丢弃仍需重放的记录。
