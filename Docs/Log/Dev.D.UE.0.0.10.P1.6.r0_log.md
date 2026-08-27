# Dev.D.UE.0.0.10.P1.6.r0 Development Log

## 元信息

- 日期：2026-08-27
- 分支：`agent/0.0.10-p1-6-preparation-adapter`
- 起点：`40e93bf03a6b780faea811bc707d7d4f3a7ed4cd`
- 阶段：0.0.10 P1.6 authority-native preparation equipment Adapter

## 输入审查

P1.5 已关闭 legacy durable item writers，但产品准备界面仍读取和尝试提交 Profile `PreparationLayout`。P1.6 的首要约束是：

- 不能在 Code A、Code B、`ShanmenItems` 之间再建立一条同步链。
- 不能让 Widget 或技能直接改库存。
- 必须从现有 durable authority 恢复，而不是添加一个 `Preparation.json`。
- 当前 Repository 尚无通用 Move，因此本阶段不能假装完成 Warehouse 原子移动。

## 设计决策

### 使用 DeploymentLock 表达战备选择意图

装备选择不是数量消费，也还不是正式 deployed。现有 Reservation 已提供：

- 持久化状态；
- Request ledger；
- Reserve/Cancel receipt；
- Owner/Scope/Content/Revision 校验；
- 重启恢复与失败回滚。

因此 P1.6 把一个活动 `DeploymentLock` reservation 定义为某个 equipment Purpose 的当前选择。该选择只有在后续 Start Run 阶段才 Commit。

### 不增加 schema 的清空 tombstone

迁移后的 Code B 装备容器是没有 reservation history 时的初始基线。若直接把该容器当永久回退，用户清空后重启会重新看到旧装备。

解决方法：清空基线时先为同一物品 Reserve，再 Cancel。最新 reservation 为 terminal，即表示该槽被明确清空。这样无需给 authority snapshot 新增 Preparation 字段。

### 可恢复替换顺序

替换先 Reserve 新物品，再 Cancel 同 Purpose 的旧活动 reservation。投影按成功 Reserve receipt 的 AuthorityRevision 选择最新记录：

- 新 Reserve 成功、旧 Cancel 尚未完成时，界面已稳定显示新选择；
- 重试同一选择不会再 Reserve，只清理旧 reservation；
- 清理失败返回 `AuthorityCleanupPending`，不会伪报整个操作未发生。

### 失败关闭的未接路径

材料、Hotbar、Warehouse move、通用 drag/drop 和 Start Run 需要各自的 authority transaction。P1.6 不用 legacy 写实现临时兼容；authority Ready 后这些入口显式返回 pending/reject。

## 实现记录

### Migration

- Stackable Definition：仅 `CapabilityConsumeQuantity`。
- 非 Stackable 且产品 Definition 有 compatible equipment slot：`CapabilityDeploy`。
- 能力来自 immutable manifest，不来自用户选择或 UI 参数。

### Projection

- Repository invariant validation 后才投影。
- 全容器和物品必须共享 bound Owner/Scope。
- 五个装备容器必须各自唯一且为一格。
- Warehouse 必须唯一且为 30 格。
- reservation history 通过 matching successful Reserve receipt 的 AuthorityRevision 排序。
- 旧装备槽只在该 Purpose 完全无 history 时使用。
- terminal latest reservation 表示空槽。
- 多个或非最新 committed deployment 被视为歧义并失败关闭。

### Command

- 检查实例存在、Owner/Scope、State、数量、Capability、Definition slot compatibility。
- 跨槽重复选择和跨 Purpose 活动锁拒绝。
- RequestId 使用 deterministic identity，不使用随机 GUID。
- committed deployment 只能由 settlement/release 释放。

### Product route

- `GetPreparationSnapshot` 在 authority Ready 后覆盖全部 item-bearing 字段。
- `SetPreparationEquipment` 路由到 Adapter。
- `SetPreparationMaterial` / `SetPreparationHotbarSlot` 返回 `AuthorityRunAdapterPending`。
- `MoveWarehouseItem` / `ExecutePreparationItemDrop` 关闭 legacy 写路径。
- `ClearPreparationSelection` 顺序执行五个可重试的 authority clear command。

## 测试修正循环

### 首轮

- 日志：`Saved/Logs/Dev.D.UE.0.0.10.P1.6.r0_automation.first.log`
- SHA-256：`8C338A7EDF2907EF93FE0C37A0B8FB5F862A285A9A3DFF0E731E6EF8CE6D9FF7`
- 结果：2 Success、1 Fail。
- 失败：`RejectionAndRollback`。
- 根因：`SetInjectedFailureForAutomation` 设计为 sticky test hook；夹具没有把 `WriteTemp` 恢复为 `None`，所以所谓“故障解除后重试”实际上仍处于故障中。
- 修正：先断言 snapshot 精确回滚，再显式清除 test hook，然后重试。

### 修正后定向测试

- 日志：`Saved/Logs/Dev.D.UE.0.0.10.P1.6.r0_automation.scoped.log`
- SHA-256：`428A94D14D7092A6F2495A7B4C132AF5074E7250A30CD6BD9955705F22A3E6A6`
- 结果：3/3 Success、0 Fail、队列正常清空。

### 最终完整回归

- 日志：`Saved/Logs/Dev.D.UE.0.0.10.P1.6.r0_automation.log`
- SHA-256：`B54E4F35DAB4C7677F56B4B557F80105FE7401239899D79FBDC60C90CFCD4FAA`
- 结果：51/51 Success、0 Fail、队列正常清空。
- 组成：P0–P1.5 既有 48 项 + P1.6 新增 3 项。
- 原生退出码：0。

## 构建记录

### Editor

- 初次完整构建：47/47 actions，成功，退出码 0，166.51 秒。
- 测试夹具加入后构建：4/4 actions，成功，退出码 0，6.53 秒。
- 最终静态审查后构建：4/4 actions，成功，退出码 0，5.55 秒。

### Game

- 47/47 actions，成功，退出码 0，150.83 秒。

全部构建使用 `-WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA`；最终构建另加 `-NoUBTMakefiles` 以强制发现新增 source。

## 静态检查

- `git diff --check`：0。
- Adapter 无 World、Actor、UI、伤害或随机数依赖。
- authority JSON 仍只由 `ShanmenItems` persistence/service 层处理。
- 没有更改 schema、Content Stamp 或 durable document 格式。

## 已知边界

- P1.6 的 `ClearPreparationSelection` 是一组可恢复的逐槽 durable command，不是一个跨槽原子批事务；中途失败会返回错误并由后续重试继续。
- authority 行目前不承载旧 Profile affix/reward 展示元数据。
- Accessory 准备契约仍只有一个产品槽，对应迁移图中的 `Accessory0`；`Accessory1` 仍只是 authority item/container 数据，不成为第二个 UI 选择槽。
- P1.6 不接唯一启动点；authority 尚未 Ready 时旧产品行为保持，启动集成留给后续阶段。

## 下一阶段建议

P1.7 建立材料 Quantity reservation 与 Hotbar binding；P1.8 建立 Start Run 的 validate/reserve/commit 编排和 settlement release；Warehouse Move/Swap/Merge 单独形成 repository transaction，避免把部署 reservation 误用为位置权威。
