# Dev.D.UE.0.0.9BFix.P2 Report

状态：`READY_FOR_0_0_9BFIX_P3_OR_F_PLANNING`

## 范围与快照

- 活动工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`；未新建项目、分支工程或第二 Profile。
- 本轮前快照：`C:\AIDev\shanmen-ue\Snapshots\Dev.D.UE.0.0.9B_FixP2_20260808_1428`。
- 已归档策划附件：`Docs\Prompt\Dev.D.UE.0.0.9BFix.P2_prompt.md`。
- 未读取、采用或恢复早期版本规则；既有 P1—P21、Fix.P1 与所有已 materialized P5/P6/P8 数据均未重置或迁移。

## 正式接入

`FCodeBOutOfRaidProfileStore` 的既有 `FCodeBP2PlayerLayout` 已可表达仓库 root、基础携行格、武器／护甲／饰品／空间／背包装备位及空间 child container。因此本轮没有新增 `SectLoadout` 平行容器：这些正式 P5 容器共同构成同一 owner、同一 repository snapshot、同一 persistent revision/digest 下的 SectLoadout 投影。

新增 `CodeB/demo_mapCodeBLoadoutSelection.h`、`demo_map0909BSectWarehouseService.*`、`demo_map0909BSectWarehouseWidget.*`。服务以现有 P5 `OpenOrMigrate` 水合唯一 Code B repository；UI 只保留此最新投影、渲染键和拖拽意图。每次拖拽先核对 Owner、AtSect、P5/P6 gate、snapshot revision、source/target、正式 layout container、slot、ItemId 与完整空间 closure，随后仅调用 P1 transaction；成功后才通过 `CommitAcceptedSnapshot` 做一个 P5 durable replacement。保存失败会从已持久化 snapshot 恢复内存 repository，拒绝、冲突和保存失败均不会创建 ItemId、clone、Code A mirror、P6 或 P8 副作用。

空间 parent 的递归 child graph 被完整验证，并作为同一个 P1 candidate state 一起移动；不会 flatten、拆 child 或复制内容。现有 Slot/Definition、容量、unique parent、cycle 与 P17/P18/P19 约束仍由 Code B repository invariant 保持。

## 出战边界

`BuildLoadoutSelection` 为当前 P5 战备根生成 OwnerId、persistent revision、graph revision、稳定排序的 root ItemId/container/slot/slot semantic、递归完整 closure 和 CRC digest。`Fdemo_map0909BRunStartCoordinator` 在创建 `StartAttemptId` 时记录这些 revision/digest；PreparingStart 与 ActivatingWorld 返回显式 `StartAttemptPending` 并拒绝 P5 写事务。

只有 M01 已确认、输入恢复且 Coordinator 即将进入 `InRun` 后，`NotifySuccessfulRunWithLoadoutSelection` 才重新读取 P5 并比对 Owner、revision、graph revision 和 digest；匹配后才委托既有 P5→P6 durable bridge。世界确认后的 stale selection、冲突或存储失败只产生审计诊断，绝不回滚真实 Code A Run、不会创建 fallback P6 或重新接管旧框架。世界确认前的技术失败仍仅走 Fix.P1 的 `TechnicalStartFailure → AtSect`：无 P6、无 P8 receipt、无物品扣除和无仓库假锁。

## 路由、编辑器支持与文件

- 修改：`demo_map0909BFramework.*`、`demo_map0909BRunStartCoordinator.*`、`demo_map0909BCodeBItemBridge.*`、`demo_map0909BFrameworkTypes.h`、`demo_map0909BSectWidget.cpp`、`demo_map0909BEditorSupport.*`、`CodeB/demo_mapCodeBOutOfRaidProfile.*`。
- 新增：Code B LoadoutSelection、Sect Warehouse service、Warehouse UMG widget/drag operation。
- `demo_mapGameMode.*` 仅新增窄只读 P5 snapshot adapter 和世界确认后的 selection-aware Code B bridge adapter；未修改地图、Actor、输入、HUD、战斗、生命、Run Save、结算、旧库存或 Loot 权威。
- 新默认 host 只路由 Sect、Warehouse、Loadout 和出战请求；静态审查确认没有调用旧局外库存入口、旧 sect navigation 或旧 preparation page。旧默认页壳/activation CTA 未重新接入。
- Editor Support 新增只读仓库投影审计：核验 Owner、P5 revision、graph revision、selection digest 和每个 root placement；不暴露写入口。
- 未修改：P5/P6/P8 terminal authority、P7、P13/P14/P15、P17/P18/P19、P9/P10/P11/P12 的既有所有权与生命周期实现。

## 静态审查

- 新 0.0.9B framework historical-path audit：通过；未发现 V3/I1/Teleport/旧 Sect Navigation/旧 Preparation Widget/旧局外库存入口引用。
- pre-world terminal-write audit：通过；新框架未引用 P8 terminal finalizer 或 settlement writer。
- new-host legacy warehouse-route audit：通过；新 host 未调用旧局外库存或旧 UI 路由。
- Code B selection bridge、P5 durable replacement、`StartAttemptPending` gate 与 Coordinator digest 记录均已在源码中静态定位。

## 编译

最终只执行了要求的两个编译目标；未启动产品。

1. `"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_mapEditor Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload`
   - native exit code：`0`；`Result: Succeeded`。
2. `"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_map Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex -NoHotReload`
   - native exit code：`0`；`Result: Succeeded`。

首次 Editor 编译仅暴露并修复了本轮新增代码的局部变量遮蔽与日志级别语法；修复后上述两个最终目标均成功。

## F 阶段债务

未执行真实宗门启动、仓库打开、拖拽、装备／卸下、空间图移动、空 P5、出战提交、M01 成功或技术失败、P5→P6、P8 三种终局、recovery、截图、自动化、回归、Smoke、Game 试玩、Cook、Package 或最终验收。上述真实验证全部保留给 `0.0.9B.F`。
