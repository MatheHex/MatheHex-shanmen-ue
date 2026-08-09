# Dev.D.UE.0.0.9B.P20.0.r0 Report

## 状态

`READY_FOR_NEXT_P_FUNCTIONAL_WITH_F_DEBT`

P20 只完成唯一 `CodeB.BodyContainer.BasicCorpse` 的未来首次 materialization 空间道具来源，以及既有 P12 尸体 root 到 P6 空 BaseQuick 的完整图取得。没有开始 P21、`0.0.9B.F` 或任何实际运行验证。

## 文件与职责

- 修改 `Source/demo_map/CodeB/demo_mapCodeBOutOfRaidProfile.cpp`：新增 `BasicCorpse.r2` catalog、确定性 r2 parent/empty-child materialization、P11→P6 whole-graph durable candidate 验证与 P6→P11 spatial 拒绝。
- 修改 `Source/demo_map/CodeB/demo_mapCodeBP3UI.cpp`：只在 P12 presenter 的现有真实 `NativeOnDrop` root cell 上，提前拒绝 corpse spatial root 的非空 BaseQuick／装备／swap/merge 路径与反向回尸体；未改变页面、读条、搜索、actor 或普通物品交互结构。
- 修改 `PROJECT.md`、`PROJECT_INFO_CARD.md`：当前任务/路径及 P20 静态边界记录。
- 新增 `Docs/Prompt/Dev.D.UE.0.0.9B.P20.0.r0_prompt.md`：真实下载 Prompt 的工程归档。
- 新增本 Report。
- 未修改任何 Code A 文件；未修改 P7/P8/P13/P14/P15/P17/P18/P19、P9/P10/P11/P12 的产品边界或测试文件。

## BasicCorpse r2 与确定性结果

- 新 Profile：`CodeB.LootProfile.BasicCorpse.r2`，`ProfileVersion=2`，`AlgorithmVersion=CodeB.DeterministicWeightedLoot.Crc32.r2`。只有 future、尚未 materialized 的 BasicCorpse 由 catalog 的 latest-profile 选择它；r1、BasicCache.r1/r2 与全部已 materialized P9/P11 record 继续由 receipt 的 id/version/digest 重建并保持原样。
- `Guaranteed.Main` 未改：IronShard weight 3、quantity 1--2；SpiritDust weight 2、quantity 1；固定选择一次、固定候选排序。
- `Optional.SpatialUtility` 固定为 `NoDrop:9 / Spawn:1`。命中时只从 WindTalisman weight 1、BackpackLevel1 weight 3 中选一，quantity 均为 1；未命中只产生原主材料结果。
- roll identity 包含 OwnerId、RunInstanceId、BodyTargetId、BasicCorpse DefinitionId、DeathReceiptId、LootProfileId、ProfileVersion、ProfileDigest 与 AlgorithmVersion。CRC32 draw、stable ItemId、slot、`SpatialChildGuid(ItemId)` 和 result digest 都由该 identity 导出；不读取时钟、Actor、坐标、UI、临时 GUID、旧 Code A Loot 或全局 RNG。

## P11 首次物质化与 P12 取得

- 命中的 parent 在同一 Owner durable replacement 中与 Hidden state、P11 root、materialization/death receipt、profile/result/materialization digest 一起保存。P17 canonical definition resolver 创建唯一 stable ChildContainer，正式 semantic/type/capacity 均由 DefinitionId 导出，child 必须为空；P1 验证、r2 receipt replay 和 root/child identity 阻止环、重复 owner、nested spatial、非法 slot、半图和 reroll。
- P12 继续只展示 BasicCorpse root cell。只有 `Revealed` 的 WindTalisman 或 BackpackLevel1 root 可经既有 `NativeOnDrop → P4 → P3 → P2 → P1` 从精确 P11 root 到一个原先为空的 P6 `BaseQuick` cell。
- Store 在保存前复核 Owner/Run/body/receipt/session/revision、formal empty closure、source/destination placement、稳定 parent/child id；并从初始 P6+P11 composite 执行同一笔期望 P1 Move，与提交 candidate 完全相等后才分区并一次替换 Owner record。因此不存在 parent-only、flatten、clone、新 ItemId/ContainerId、partial commit、direct equip、Merge、Swap 或 Widget direct write。
- P6→P11 的 P12 simple-item 行为仍保持；任何 spatial parent 或 child graph 回到 BasicCorpse 都被 UI 和 Store 拒绝。

## 静态边界结论

- P7 只会从新的 P6 权威 projection 读取成功图；P13 不自动绑定，P15 不给予空间 parent/child effect。
- P5/P6 bridge、recovery/rebind、P8 settlement、P14/P19 WorldDrop、P17 graph、P18 BasicCache、P9/P10 其他来源与 P11/P12 既有 lifecycle 未新增产品语义。未进入 P6 的尸体 graph 仍由 P8 作为 P11 residual 丢弃；进入 P6 的图继续遵循既有终局链。
- Code A 没有功能改动、库存镜像、Loot/Actor/地图/输入/HUD/战斗/Run/Save/结算权威变化。

## 编译

实际且仅执行一次：

```text
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_mapEditor Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex
```

目标：`demo_mapEditor Win64 Development`。最终 native exit code：`0`；关键结果：`5 action(s)`、`Result: Succeeded`。

## 明确保留给 0.0.9B.F

未执行：真实 BasicCorpse r2 roll（命中/未命中）、死亡/开尸/揭示、P12 完整图转移、P7 打开/返回、P19 地面往返、P8 三种终局、recovery、自动化、回归、截图、Smoke、Game Build、Cook、Package 与最终验证。

未启动：第二来源/全量迁移、空间道具预装内容、child 地面操作、嵌套袋、空间装备效果、武器/道袍/饰品效果、其他消耗品及后续 P 阶段功能。
