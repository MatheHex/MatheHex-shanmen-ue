# Dev.D.UE.0.0.10.P1.7.r0 Development Log

## 元信息

- 日期：2026-08-27
- 分支：`agent/0.0.10-p1-7-run-loadout-adapter`
- 起点：`89d5fb12ff2a4a7af6f5521abbafa156bb766a0f`
- 阶段：0.0.10 P1.7 Quantity 战备预留与 Hotbar Adapter

## 输入审查

P1.6 已把装备选择切到 ShanmenItems，但材料、消耗品和 Hotbar 仍被显式关闭。P1.7 必须满足：

- 真实战斗资源只能来自玩家实际拥有的 ItemInstance；
- 准备阶段只能 Reserve，不能提前消耗或启动 Run；
- 不允许重新写 Profile `PreparationLayout`；
- 顺序和 Hotbar 必须在重启后恢复；
- 不能为准备界面创建另一份持久化权威。

## 设计决策

### 完整 Stack 使用 Quantity reservation

既有产品契约以 ItemInstanceId 选择完整 Stack，而非选择任意数量。Adapter 因此把 `Item.Quantity` 全量传给 `ReserveDurable`：

- Item.Quantity 保持不变；
- Available Quantity 变为 0；
- 其它消耗事务不能越过战备占用；
- Start Run 之前不会出现永久消费。

### 顺序与 Hotbar 随 Purpose 持久化

Quantity 是 stackable definition 的独占能力，无法同时建立 DeploymentLock 元数据 reservation。为避免新 schema 或第二个 save，P1.7 使用固定格式：

`Shanmen.Preparation.RunInventory.r1.O########.H##`

- `O` 是显式选择顺序；
- `H00` 表示未绑定；
- `H01`–`H09` 表示唯一外部 Hotbar 槽。

投影只接受格式完整且带 matching successful Reserve receipt 的活动 reservation。解析使用 FName 兼容的大小写无关匹配，但新写入始终使用规范格式。

### Hotbar 变更与补偿

完整 Quantity 已被旧 reservation 占满，因此不能像装备一样先 Reserve 新锁再 Cancel 旧锁。当前 Repository 也没有原子 amend。P1.7 的有界流程为：

1. Cancel 旧 metadata intent；
2. Reserve 同一完整 Stack、同一 order、目标 Hotbar 的新 intent；
3. 第二步失败时立即 Reserve 旧 metadata 作为补偿；
4. 占用槽替换失败时同时尝试恢复被挤出的原绑定。

所有步骤都通过 authority durable service，不直接改 snapshot 或 JSON。跨进程恰好中断在第 1、2 步之间时，恢复按未选择失败关闭；该窗口留给 P1.8 原子批事务关闭。

### 容量与清空顺序

RunInventory 容量从当前 authority 选择的 Backpack/SpatialRing Definition 计算。Hotbar 只接受排序后位于 Base Quick + Ring Quick 区域的 consumable。

清空全战备时先释放 Quantity，再清空容量装备，避免“材料依赖背包容量，而背包因材料存在无法清空”的循环拒绝。

## 实现记录

### Projection

- 验证整个 authority snapshot repository invariant。
- 验证所有 preparation Quantity reservation 都是完整 eligible Stack。
- 拒绝重复 Item、order 和 Hotbar slot。
- 排序后生成 `OrderedSelectedMaterialIds`。
- 从 `H##` 生成固定九格 Hotbar。
- 计算每行 `bMaterialSelectionEligible`、`bSelected` 和 `bInBaseQuickItemArea`。
- equipment、materials、Hotbar、Warehouse 在同一投影中返回。

### Product route

- `SetPreparationMaterial` → `SelectMaterial`。
- `SetPreparationHotbarSlot` → `SetHotbarSlot`。
- `GetPreparationSnapshot` 使用 authority 的材料顺序和 Hotbar，不再清空它们。
- `ClearPreparationSelection` 先材料后装备。
- `bCanStartRun` 保持 false，防止未完成的 P1.8 编排被旁路。

### Request identity

Reserve RequestId 包含：Owner、Scope、ItemInstanceId、上一条 reservation、规范 Purpose、Stack 数量、Item Revision。Cancel RequestId 由 reservation identity 派生。取消后重新选择不会错误回放已 terminal 的旧 Reserve。

## 测试记录

### 定向测试

- `CapabilityAndProjection`
- `ReplaceClearRestart`
- `RejectionAndRollback`
- `QuantityHotbarRestart`
- `QuantityCapacityAndFailureFence`

结果：5/5 Success。新增覆盖包括：

- 全量 Quantity reserve 与 Available=0；
- 显式选择顺序；
- Hotbar 占位替换、跨槽移动、唯一性和取消联动；
- authority 重启恢复；
- 六格基础容量、背包扩容和容量装备移除围栏；
- sticky 写故障不发布候选状态，解除后可重试；
- Clear All 的材料优先顺序；
- Profile 文件字节不变。

### 完整回归

- 结果：53/53 Success、0 Fail、队列正常清空。
- 组成：P0–P1.6 既有 51 项 + P1.7 新增 2 项。
- 日志：`Saved/Logs/Dev.D.UE.0.0.10.P1.7.r0_automation.log`
- SHA-256：`A808222AA8CE8126EC6180655334313241C8992BC194BD22A840EA65A0342D61`
- 原生退出码：0。

## 构建记录

- Editor 首次完整：47/47 actions，成功，退出码 0，169.24 秒。
- Editor 最终增量：6/6 actions，成功，退出码 0，14.91 秒。
- Game 首次完整：46/46 actions，成功，退出码 0，144.96 秒。
- Game 最终增量：5/5 actions，成功，退出码 0，19.18 秒。

全部构建使用 `-WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA -NoUBTMakefiles`。

## 静态检查

- `git diff --check`：0。
- Adapter 的 World/Actor/伤害/随机数/UI/产品启动依赖：0。
- Adapter 的 Profile/Code B 持久化写 API：0。
- 未增加 schema、Content Stamp 或新的持久化文件。

## 后续入口

P1.8 建立唯一 Start Run 编排：一次性验证当前装备与 Quantity intents，原子 Commit，生成 Runtime loadout receipt，并在 settlement/release 中对称恢复。原子批事务同时用于关闭 P1.7 Hotbar metadata 两步替换的跨进程中断窗口。
