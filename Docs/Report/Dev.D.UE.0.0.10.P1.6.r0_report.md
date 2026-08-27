# Dev.D.UE.0.0.10.P1.6.r0 Report

## 结论

P1.6 已完成宗门战备装备选择向 `ShanmenItems` 单一物品权威的首个产品 Adapter 切换，结论为 **PASS**。

本阶段没有建立第三套库存、没有恢复 Profile/Code B 双写，也没有增加新的持久化 schema。武器、防具、饰品、空间戒指和背包的选择意图现在由 `DeploymentLock` 预留及其 durable receipt 表达，可执行替换、清空、幂等重试和进程重启恢复。旧 Code B 装备容器仅在该槽尚无 reservation 历史时作为一次性迁移基线；最新 terminal reservation 是明确的空槽 tombstone。

## 功能性

### 已完成

- 迁移时根据不可变产品 Definition 为非堆叠装备授予 `CapabilityDeploy`。
- 堆叠材料继续保持 `CapabilityConsumeQuantity` 独占，不会同时获得部署能力。
- 新增一向 Adapter，把既有战备 UI 值请求转换为 `ReserveDurable` / `CancelDurable`。
- 以五个固定 Purpose 区分装备槽：
  - `Shanmen.Preparation.Weapon`
  - `Shanmen.Preparation.Armor`
  - `Shanmen.Preparation.Accessory`
  - `Shanmen.Preparation.SpatialRing`
  - `Shanmen.Preparation.Backpack`
- 同一槽替换先持久化新意图，再清理被取代的旧预留；中断后重试会从 durable snapshot 继续清理，不重复建立选择。
- 清空迁移基线时建立并取消一个部署预留，留下可重放 tombstone，避免重启后旧基线重新出现。
- 只接受当前 Owner/Scope、非 Depleted、数量为 1、具有部署能力且产品 Definition 与槽兼容的实例。
- 同一 ItemInstanceId 不能同时出现在两个战备槽或两个活动部署 Purpose 中。
- committed deployment 不允许被战备层清空或替换，必须由后续 Run settlement/release 流程处理。
- Profile preparation snapshot 在 authority Ready 后从 `ShanmenItems` 投影装备、物品行和 30 格 Warehouse；不再把 legacy item state 当回退真值。
- authority Ready 后明确关闭 legacy material、Hotbar、warehouse move 和通用 drag/drop 写路径。
- 投影对重复 Warehouse、重复/歧义 committed history、Owner/Scope 分裂和 repository invariant 错误全部失败关闭。

### 本阶段明确未完成

- 材料/消耗品的 Quantity Reserve/Commit 战备 Adapter。
- Hotbar 的 authority-native binding。
- Warehouse 的 authority-native Move/Swap/Merge。
- Start Run 时把战备预留原子提交为实际 deployment。
- Run settlement/release 后的装备回收。
- 旧 affix/reward 展示元数据迁入 `ShanmenItems` snapshot；P1.6 的 authority 行只投影目前权威拥有的字段。

这些路径没有偷偷回退到旧 Profile，而是返回 `AuthorityRunAdapterPending` 或显式拒绝，防止权威再次分叉。

## 完整性与恢复性

- RequestId 由 Owner、Scope、Purpose、目标物品、物品 Revision 和上一条槽 reservation 身份确定性派生。
- 同一已选装备的重复请求不增加 AuthorityRevision。
- 清空后的重新选择以 terminal reservation 为前序身份，生成新而稳定的请求，不会回放已取消的旧 reserve。
- superseded reservation 清理失败返回 `AuthorityCleanupPending`；新选择仍可从最新 durable receipt 投影，后续相同请求继续清理。
- 持久化写失败会回滚到精确 pre-command document；故障解除后同一产品操作可自动重试成功。
- 进程重启只需 `BindExisting`，不会重读或改写 legacy item source。

## 兼容性

- authority 尚未 Ready 时，既有 Profile preparation 行为保持原样，便于分阶段接入唯一启动点。
- authority Ready 后，所有 item-bearing preparation 字段均从新权威读取。
- 非物品 Profile 信息（灵石、商店投影、结算提示等）继续由现有 Profile Session 提供。
- 旧枚举值顺序未改变；P1.6 状态追加在末尾。
- 没有修改 `ShanmenItems` persistence schema 1，也没有修改 Profile schema 7。

## 修改范围

### 新增

- `Source/demo_map/demo_mapShanmenPreparationAdapter.h`
- `Source/demo_map/demo_mapShanmenPreparationAdapter.cpp`
- `Source/demo_map/demo_mapShanmenPreparationAdapterTests.cpp`

### 修改

- `Source/demo_map/demo_mapShanmenItemMigration.cpp`
- `Source/demo_map/demo_mapProfilePreparationTypes.h`
- `Source/demo_map/demo_mapProfileSessionSubsystem.cpp`

## 自动化

### P1.6 定向测试

新增 3 项：

1. `CapabilityAndProjection`
2. `ReplaceClearRestart`
3. `RejectionAndRollback`

首轮结果：2 Success、1 Fail。失败原因是 automation fixture 的持久化故障注入为 sticky 状态，测试在未解除注入时要求第二次写成功；生产代码已经精确回滚。修正夹具在验证回滚后解除注入，定向测试达到 3/3 Success。

### 完整 0.0.10 回归

- 命令：`Automation RunTests Shanmen.0_0_10`
- 结果：`51/51 Success`、0 Fail、`Automation Test Queue Empty`
- 日志：`Saved/Logs/Dev.D.UE.0.0.10.P1.6.r0_automation.log`
- SHA-256：`B54E4F35DAB4C7677F56B4B557F80105FE7401239899D79FBDC60C90CFCD4FAA`
- 原生进程退出码：`0`

## 构建

### Editor

- 命令：`Build.bat demo_mapEditor Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA -NoUBTMakefiles`
- 首次完整生产代码构建：47/47 actions，`Result: Succeeded`，退出码 `0`，166.51 秒。
- 最终审查后增量构建：4/4 actions，`Result: Succeeded`，退出码 `0`，5.55 秒。

### Game

- 命令：`Build.bat demo_map Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA -NoUBTMakefiles`
- 结果：47/47 actions，`Result: Succeeded`，退出码 `0`，150.83 秒。

## 静态审查

- `git diff --check`：退出码 `0`。
- Adapter 中 `UWorld`、`AActor`、伤害、随机数、PIE/产品启动 API：0 匹配。
- Adapter 只调用 GameInstance authority durable command，不直接读写 authority JSON。
- 旧 Profile/Code B 写路径在 authority Ready 后没有 fallback。
- 未跟踪的历史 Prompt、Report、PDF 和自动化交接文档未纳入本阶段提交。

## P/F 边界

本阶段执行了代码开发、静态审查、无头自动化、Editor Development 构建和 Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未做真实输入、截图、Smoke、Cook、Package 或大规模产品回归。

## 后续入口

建议 P1.7 优先实现材料/消耗品的 authority-native preparation transaction，并把 Hotbar 绑定建立在已预留的 run inventory 上；随后再实现 Start Run 的 reserve-to-commit 编排。Warehouse Move 应使用独立原子事务，不复用 preparation reservation，也不恢复 Code B 写权威。
