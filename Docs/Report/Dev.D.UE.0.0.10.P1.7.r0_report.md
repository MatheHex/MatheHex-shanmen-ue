# Dev.D.UE.0.0.10.P1.7.r0 Report

## 结论

P1.7 已完成材料/消耗品的 authority-native 战备预留和 Hotbar 绑定，结论为 **PASS**。

本阶段继续以 `Udemo_mapShanmenItemAuthoritySubsystem` 为唯一可变物品权威。战备选择不会消费物品，也不会回写退休的 Profile `PreparationLayout`：每个被选中的材料或消耗品实例以一条覆盖完整 Stack 的 `Quantity` reservation 表达；携带顺序和唯一 Hotbar 槽位随 reservation Purpose 持久化。进程重启后仅从 ShanmenItems 文档即可恢复装备、RunInventory 顺序和九格 Hotbar。

## 功能性

### 已完成

- `SetPreparationMaterial` 在 authority Ready 后路由到 ShanmenItems Adapter，不再返回 P1.6 pending。
- 只接受当前 Owner/Scope、Stored、正数量、具有 `CapabilityConsumeQuantity` 且产品分类为 Material/Consumable 的完整 Stack。
- 每个选择预留实例当前全部 `Quantity`；Reserve 后数量不变，但可用数量为 0，防止同一资源被其它事务重复占用。
- 显式保留选择顺序；取消再选择会追加到当前 RunInventory 末尾。
- 使用结构化 Purpose `Shanmen.Preparation.RunInventory.r1.O########.H##` 持久化顺序和 0/1–9 Hotbar 槽位，不增加第二个准备存档或第三套库存。
- `SetPreparationHotbarSlot` 只接受已选中、位于当前 Base Quick 区域的 Consumable。
- 同一 ItemInstanceId 最多绑定一个 Hotbar 槽，同一槽最多一个实例；支持占位替换、稳定实例跨槽移动和清空。
- 取消 RunInventory 选择时，同一 reservation 内的 Hotbar 绑定一起消失，不会留下悬空引用。
- 当前背包/空间戒指共同决定携带容量；依赖扩展容量时，系统拒绝清空容量装备，直到溢出 Stack 先被移除。
- `ClearPreparationSelection` 先释放全部 Quantity reservation，再清空装备 DeploymentLock，避免容量装备先清空造成死锁。
- Profile preparation snapshot 在 authority Ready 后投影：
  - `OrderedSelectedMaterialIds`
  - 九格 `HotbarBindings`
  - `bMaterialSelectionEligible`
  - `bInBaseQuickItemArea`
  - 装备和 Warehouse 既有字段
- `bCanStartRun` 继续保持 false；本阶段只 Reserve，不提前 Commit 或启动产品。

### 失败关闭与恢复

- malformed Purpose、重复 Item、重复 order、重复 Hotbar、非完整 Stack reservation、容量超限和 Quick 区越界均使投影失败关闭。
- Reserve/Cancel RequestId 由 Owner、Scope、Item、上一条 reservation、目标元数据、数量和 Item Revision 确定性派生。
- Hotbar 元数据更换采用“取消旧 Quantity intent → 建立同量新 intent”；第二步失败时在同一进程内自动重建旧 intent。
- durable 存储写失败不会发布候选 snapshot；故障解除后同一用户动作可自动重试。
- 退休 Profile 文件在材料选择、Hotbar 绑定、重启、失败和重试全过程保持字节相同。

## 完整性与兼容性

- 未修改 `ShanmenItems` persistence schema 1。
- 未修改 Profile schema 7。
- 未增加新的 JSON、SaveGame 或 UI 本地真值。
- authority 尚未 Ready 时，既有 Profile 行为保持原样；Ready 后 legacy material/Hotbar 写路径不再可达。
- P1.6 的五种装备 Purpose、迁移基线和 empty tombstone 语义保持不变。
- Warehouse Move/通用 drag-drop 继续失败关闭，不借用 Quantity reservation 伪装位置事务。

## 修改范围

### 修改

- `Source/demo_map/demo_mapShanmenPreparationAdapter.h`
- `Source/demo_map/demo_mapShanmenPreparationAdapter.cpp`
- `Source/demo_map/demo_mapShanmenPreparationAdapterTests.cpp`
- `Source/demo_map/demo_mapProfileSessionSubsystem.cpp`
- `Source/demo_map/demo_mapProfilePreparationTypes.h`

### 新增文档

- `Docs/Report/Dev.D.UE.0.0.10.P1.7.r0_report.md`
- `Docs/Log/Dev.D.UE.0.0.10.P1.7.r0_log.md`

## 自动化

### P1.7 定向测试

- 命令：`Automation RunTests Shanmen.0_0_10.Items.PreparationAdapter`
- 结果：`5/5 Success`、0 Fail、`Automation Test Queue Empty`
- 覆盖：能力与投影、装备替换/清空/重启、拒绝与回滚、Quantity/Hotbar 重启、容量与故障围栏。

### 完整 0.0.10 回归

- 命令：`Automation RunTests Shanmen.0_0_10`
- 结果：`53/53 Success`、0 Fail、`Automation Test Queue Empty`
- 日志：`Saved/Logs/Dev.D.UE.0.0.10.P1.7.r0_automation.log`
- SHA-256：`A808222AA8CE8126EC6180655334313241C8992BC194BD22A840EA65A0342D61`
- 原生进程退出码：`0`

## 构建

### Editor

- 命令：`Build.bat demo_mapEditor Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA -NoUBTMakefiles`
- 首次完整构建：47/47 actions，`Result: Succeeded`，退出码 `0`，169.24 秒。
- 最终增量构建：6/6 actions，`Result: Succeeded`，退出码 `0`，14.91 秒。

### Game

- 命令：`Build.bat demo_map Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA -NoUBTMakefiles`
- 首次完整构建：46/46 actions，`Result: Succeeded`，退出码 `0`，144.96 秒。
- 最终增量构建：5/5 actions，`Result: Succeeded`，退出码 `0`，19.18 秒。

## 静态审查

- `git diff --check`：原生退出码 `0`。
- Adapter 中 `UWorld`、`AActor`、`UGameplayStatics`、伤害、随机数、PIE、Standalone 和进程启动 API：0 匹配。
- Adapter 中 Profile/Code B 持久化写 API：0 匹配。
- 未跟踪的历史 Prompt、Report、PDF 和自动化交接文档未纳入本阶段提交。

## P/F 边界

本阶段执行了代码开发、静态审查、无头自动化、Editor Development 构建和 Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；未做真实输入、截图、Smoke、Cook、Package 或大规模产品回归。

## 已知边界与下一阶段

- Repository 目前没有原子“修改 reservation 元数据”或多 reservation 批事务。Hotbar 替换在正常失败时会补偿回滚，但若进程恰好中断在旧 intent 已取消、新 intent 尚未持久化之间，恢复结果会按未选择/未绑定失败关闭，不会伪造重复资源。P1.8 的 Start Run 编排应引入统一原子批事务，顺带关闭该窗口。
- P1.8 应把当前装备 DeploymentLock 与 RunInventory Quantity reservation 在唯一 Start Run 入口中完成 validate/commit，并为 settlement/release 定义对称恢复路径。
- Warehouse Move/Swap/Merge 仍应形成独立 repository transaction，不复用 preparation reservation。
