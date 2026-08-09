# Dev.D.UE.0.0.9B.P16.0.r0 Report

## 结论

`READY_FOR_NEXT_P_FUNCTIONAL_WITH_F_DEBT`

P16 已把 P9 BasicCache 与 P11 BasicCorpse 的**未物质化首次**内容源改为 Code B 独占、一次性且可审计的确定性加权 Loot Profile。实现、静态审查和规定的一次 Editor 编译均完成；没有启动产品或执行 F 阶段验证。

## 文件范围

| 状态 | 文件 | 职责 |
| --- | --- | --- |
| 修改 | `Source/demo_map/CodeB/demo_mapCodeBOutOfRaidProfile.h` | P9/P11 receipt 新增 P16 Profile provenance；Run-local record schema `2→3`；fixed ContentPlan 明确仅保留给历史 record 验证。 |
| 修改 | `Source/demo_map/CodeB/demo_mapCodeBOutOfRaidProfile.cpp` | Code B 内部 Loot Profile catalog、CRC32 r1 deterministic roll、P9/P11 首次物质化、provenance JSON/migration、初始图与重复读取的静态验证。 |
| 修改 | `PROJECT.md` | 更新当前任务和 P16 边界／编译摘要。 |
| 修改 | `PROJECT_INFO_CARD.md` | 更新当前阶段并记录 P16 Profile、持久化和 F 债务。 |
| 新增 | `Docs/Prompt/Dev.D.UE.0.0.9B.P16.0.r0_prompt.md` | 原始、完整 P16 Prompt 归档。 |
| 新增 | `Docs/Report/Dev.D.UE.0.0.9B.P16.0.r0_report.md` | 本报告。 |

未修改：所有 Code A 源、P10/P12 的 UI／actor／timer／拖拽／Move/Merge/Swap 路由、P13、P14、P15、P5/P6 bridge、P8 terminal settlement、Run Save、旧 Loot 与 HUD。`demo_mapCodeBInventory.*` 和既有 P1 事务也未改动。

## P16 Loot Profile Catalog

固定算法版本：`CodeB.DeterministicWeightedLoot.Crc32.r1`。每个 Profile 只有一个固定必出、`SelectionCount=1`、`GroupOrder=0` 的 `Guaranteed.Main` roll group；没有 optional group、starter grant、P5 初始库存、fixture、测试 ItemId 或 Code A Loot。

| 来源 | ProfileId / version | 候选（固定 tie-break 顺序） | 合法性 |
| --- | --- | --- | --- |
| P9 `CodeB.NormalContainer.BasicCache` | `CodeB.LootProfile.BasicCache.r1` / `1` | `SpiritDust`: weight `3`, qty `2–3`, order `0`; `IronShard`: weight `2`, qty `1–2`, order `1` | 容量 4；均为既有 Code B canonical Material，数量受正式 MaxStack 约束。 |
| P11 `CodeB.BodyContainer.BasicCorpse` | `CodeB.LootProfile.BasicCorpse.r1` / `1` | `IronShard`: weight `3`, qty `1–2`, order `0`; `SpiritDust`: weight `2`, qty `1`, order `1` | 容量 2；均为既有 Code B canonical Material，数量受正式 MaxStack 约束。 |

`LootProfileDigest` 以固定 group order、candidate tie-break、ProfileId、source DefinitionId、ProfileVersion、算法版本、entry id、DefinitionId、权重和数量边界排序后计算 `FCrc::StrCrc32`。catalog 在写入前拒绝重复 profile/source/group/entry/tie-break identity、非正 weight／quantity、非法范围、未知 definition、非 stackable 多数量、超过 MaxStack、装备／child-container 需求或超过来源容量的完整图。

## 确定性、事务与历史兼容

- roll identity 精确为：`OwnerId + RunInstanceId + SearchTargetId 或 BodyTargetId + source DefinitionId + DeathReceiptId（仅 P11）+ LootProfileId + ProfileVersion + LootProfileDigest + AlgorithmVersion`。选择、数量、排序、slot、stable ItemId 和 result digest 只使用该字符串的分域 CRC32；不读取时钟、帧号、Actor、世界坐标、UI、显示名、临时 GUID、旧 Code A Loot、网络状态或全局可变 RNG。
- P9/P11 先保留既有 Owner／committed P6／Run／target／definition（P11 加 death receipt）gate，成功路径仅在同一个 Owner durable replacement 中登记完整 P1 root/item graph、全 Hidden state、旧 definition/materialization receipt、Profile provenance 和 result digest。Profile、布局或 P1 定义不合法时，record/P5/P6/P8 均不写入；没有空箱或 fallback recipe。
- 已 materialized P9/P11 record 的 ContainerId、ItemId、数量、child graph、visibility/state、receipt 与 digest 不被改写。没有 Profile provenance 的 schema 1/2 文档显式按 immutable legacy fixed recipe 读取；只有将来合法的未物质化目标使用 r1 Profile。重复 materialize、同 death receipt replay、query/open-close/interruption、actor destroy、P6 recovery/rebind 和保存冲突仍返回现有 record 或零写入，绝不追加第二次 roll／根容器／item。
- P10/P12 继续只领取已物质化真相；成功的既有跨图 Drop 后才可能写入 P6，继而由 P13/P14/P15 消费。P8 继续只结算 P6 并丢弃 P9/P11 残余。Code A 未获得 Profile、roll、库存或 Run Save 权威。

## 静态审查

- 审计了两个 Profile 的 identity、固定候选顺序、正权重、数量范围、容量、MaxStack、无装备/child graph 约束和 `FCrc::StrCrc32` profile/result digest 路径。
- 审计了 `BuildDeterministicLootProfileRoll`、`ValidateLootProfileReceiptProvenance` 和 `ValidateInitialLootProfileGraph`：同一精确身份复建同一候选、数量、slot、ItemId、result digest 与 initial P1 graph；既有 record 没有 reroll 写路径。
- 审计了 P9/P11 原有 candidate-then-`SaveRecord` 事务：roll 纯计算、完整 graph/Hidden/provenance/receipt/digest 完成后才进入单次 Owner save；重复与失败路径在 candidate 写入前返回。
- 审计了 P16 本轮修改范围：只有 `CodeB/demo_mapCodeBOutOfRaidProfile.{h,cpp}` 是功能源码；无 Code A 源变更。

## 编译

实际执行的唯一允许检查：

```powershell
& 'C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat' demo_mapEditor Win64 Development 'C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject' -WaitMutex
```

- Target：`demo_mapEditor Win64 Development`
- Native exit code：`0`
- UnrealBuildTool：`Result: Succeeded`
- 总耗时：`36.94 seconds`
- 只进行了 Editor 代码编译；编译过程中未执行任何自动化测试。

## 明确留给 0.0.9B.F 的项目

未执行：真实多 Run roll、同 Run 重开、重复 death/open、P10/P12 转移、P15 consumable 可达性、P8 Extracted/Dead/RecoveredAbandon、恢复、自动化、回归、截图、Smoke、Game Build、Cook、Package 和最终验证。

未启动：第二来源／全量迁移、复杂掉率经济、装备/武器使用、其他消耗品、空间物品使用和后续 P 阶段功能。
