# Dev.D.UE.0.0.9B.P18.0.r0 Report

Status: `READY_FOR_NEXT_P_FUNCTIONAL_WITH_F_DEBT`

## 变更文件与职责

- `Source/demo_map/CodeB/demo_mapCodeBOutOfRaidProfile.cpp`：保留可按 receipt 验证的 BasicCache r1/BasicCorpse r1，新增 BasicCache r2、确定性 optional gate、完整 P17 spatial graph 的一次性 P9 materialization，以及 P10 复合提交前的完整空间图校验。
- `PROJECT.md`、`PROJECT_INFO_CARD.md`：更新 P18 当前任务、入口和阶段边界记录。
- `Docs/Prompt/Dev.D.UE.0.0.9B.P18.0.r0_prompt.md`：本次完整 Prompt 归档。
- `Docs/Report/Dev.D.UE.0.0.9B.P18.0.r0_report.md`：本报告。
- 未修改 Code A、P10 UI/输入/读条/target identity、P7 操作入口、P8 terminal authority、P11/P12、P13/P14/P15 或任何测试文件。

## BasicCache r2 与确定性内容

- 保留 `CodeB.LootProfile.BasicCache.r1` 及 `CodeB.LootProfile.BasicCorpse.r1`。已有 materialized record 按 receipt 中的 `LootProfileId + ProfileVersion + ProfileDigest + AlgorithmVersion + ResultDigest` 选取精确旧 Profile；r1 digest 输入保持 P16 原样，历史图和历史 result digest 不重写。
- 尚未 materialized 的 `CodeB.NormalContainer.BasicCache` 只选择 `CodeB.LootProfile.BasicCache.r2`（version `2`，algorithm `CodeB.DeterministicWeightedLoot.Crc32.r2`）。
- r2 的 `Guaranteed.Main` 原样为：`SpiritDust` weight `3`、qty `2–3`、tie order `0`；`IronShard` weight `2`、qty `1–2`、tie order `1`；一次选择。
- r2 新增 `Optional.SpatialUtility`：group order `1`、`NoDrop:9 / Spawn:1` 的独立 deterministic gate；spawn 后仅选一个：`Prototype.Item.Accessory.WindTalisman` / `Fdemo_mapItemIds::WindTalisman` weight `1`、qty `1`、tie order `0`，或 `Prototype.Item.Backpack.Level1` / `Fdemo_mapItemIds::BackpackLevel1` weight `3`、qty `1`、tie order `1`。候选与 group 顺序固定，gate weights 和候选均纳入 r2 profile digest。
- 完整 identity 是精确 `OwnerId`、`RunInstanceId`、BasicCache `SearchTargetId`、source `DefinitionId`、空 DeathReceipt、Profile id/version/digest 和 algorithm version；无时钟、帧号、Actor/坐标、UI、显示名、临时 GUID、旧 Code A Loot、网络或可变 RNG。

## P9 图与历史保护

- r2 命中时，先在候选 P1 repository 中创建 BasicCache root 的稳定 parent ItemId，再用 P17 `SpatialChildGuid(ItemId)` 创建唯一、空的 `CodeB.SpatialChild.QuickRing` 或 `CodeB.SpatialChild.StoragePouch`。layout、capacity、slot legality 和 provenance 全由正式 Definition Catalog/P17 canonical resolver 导出。
- validation 要求空间 parent 位于 BasicCache root、child 容量匹配、child 为空、唯一 owner、无环、无自引用、无额外 root/child/nested container；普通主材料不得拥有 child。非法 definition、gate/config、容量、ID 或 child 创建任一失败会拒绝整个首次 materialization，零写 P9/P6/P5/P8。
- 成功时完整 P9 snapshot、Hidden reveal states、r2 provenance、result/materialization digest 和 target receipt 在同一 Owner durable replacement 保存。相同 target 的 query、open/close、interrupt、recovery、冲突或重试读取既有 record 或零写，不能第二次 gate、reroll、追加 parent/child 或产生第二 P9 root。

## P10 完整图转移与生命周期边界

- 未修改 P10 既有 UI、读条、揭示、输入、drag state machine 或入口。既有 P1 composite transaction 继续是唯一 P9→P6 写路径。
- 提交前新增 Code B 校验：空间 parent 必须来自 exact BasicCache root 且为 `Revealed`；其稳定 parent/child identity、definition、quantity、child type/capacity/empty slots 不得变更；移动后的 parent 必须落在既有 exact P6 container。Hidden/Searching、parent-only、flatten、child mutation、clone、new ItemId、P6 外 destination 或 partial graph 均拒绝，保持 P9/P6 和相关 P7/P13/P14/P15/P8 状态不写入。
- 成功的既有 Owner-document replacement 仍以同一 commit 将 P9 root 中已移动 parent 和完整 child graph 分离到 P6，推进 P6 session receipt/digest 与 P9 record revision；P7 只读精确 P6 真图，P8 仅按既有 Extracted/Dead/RecoveredAbandon 结算 P6，残余 P9（含未取得完整图）随 session 丢弃。P5 starter、direct grant、尸体来源、地面整包路径及 Code A inventory/Loot authority 均未新增。
- 静态审查：P18 标识和 r2/optional 代码仅位于 `CodeB/demo_mapCodeBOutOfRaidProfile.cpp`；Code A 无 P18 功能改动。P14 的 complex reject、P13/P15 BaseQuick eligibility、P11/P12 独立来源与 P7 stale-close 边界未改。

## P 阶段编译

```text
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" demo_mapEditor Win64 Development "C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject" -WaitMutex
```

- Target：`demo_mapEditor Win64 Development`
- 结果：`Succeeded`
- native exit code：`0`
- 关键结果：编译 `demo_mapCodeBOutOfRaidProfile.cpp` 后完成 DLL/link/metadata；未执行 Game target、产品启动或测试。

## 留给 0.0.9B.F 的验证债务

- 未执行真实 BasicCache r2 roll（命中/未命中）、P10 空间图转移、P7 打开/返回、P8 三种终局、recovery、自动化、回归、截图、Smoke、Game Build、Cook、Package 或最终验证。
- 未启动 BasicCorpse 空间来源、第二来源/全量迁移、整包地面丢弃/拾回、嵌套袋、空间装备效果、武器/道袍/饰品效果、其他消耗品或后续 P 阶段功能。
