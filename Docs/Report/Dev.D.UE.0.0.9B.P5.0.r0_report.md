# Dev.D.UE.0.0.9B.P5.0.r0 Report

## 任务身份与结论

```text
Project: Dev.D.UE.0.0.9B
Task: Dev.D.UE.0.0.9B.P5.0.r0
Prompt: Docs\Prompt\Dev.D.UE.0.0.9B.P5.0.r0_prompt.md
Baseline: C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9-XFix1
Engine: Unreal Engine 5.8
Status: NEEDS_PLANNER_DECISION
```

P4x.0.r2 已验收。P5 的真实 Profile 局外接入已实施：正常宗门“仓库／人物配置”入口按正式 Profile `OwnerId` 懒加载 Code B 局外状态，而非 fixture。依照用户 2026-08-06 的最新执行规则，本阶段不再进行大规模回归、截图巡检或重复构建；完整验证矩阵统一留到 `0.0.9B.F`。因此本文件是实施与边界交接报告，不是 P5 的完整验收报告，不能使用 READY 状态。

## 已实施的局外 Profile 边界

- `FCodeBOutOfRaidProfileStore` 为每个正式 `OwnerId` 管理独立的版本化 Code B sidecar；记录包含 snapshot、Repository/Persistent Revision、创建/提交时间、Prepared/Committed receipt、旧实例到 Code B 实例映射及无效来源计数。
- 持久化使用主文件、临时文件和备份恢复路径；只有接受的 P2/P1 事务才提交 snapshot 并递增持久 Revision。Hover、详情、右键、双击、取消、关闭及拒绝路径不构成库存写入。
- 首次接管仅在非活动 Run 读取旧局外来源。活动 Run 请求被无写入拒绝，并保持中文提示“结束当前 Run 后再整理”。
- 旧数据的可读兼容来源只用于一次性接管和 receipt 审计，不回写、不镜像、不成为第二份可编辑库存。
- 旧空间父引用 `LegacySpatialParentItemInstanceId` 被迁移为 Code B 空间物品的稳定 `ChildContainerId`；有效内部内容从首个 Code B snapshot 起只有一个父位置。失效父引用或容量关系被 receipt 记录，关联项保留为普通有效物品而不伪造、复制或静默丢弃。

## 正常入口与现有层级

- 宗门已有仓库/传送导航路由到 `Ademo_mapV3ProgressionManager::OpenCodeBOutOfRaidInventory`，其 Profile Host 不调用 `CreateFixture`、`ResetFixture` 或确定性 WPN 测试数据。
- 真实页面继续采用既有唯一写链：`Slate/UMG Drag Drop → P4 → P3 → P2 → P1`。位置变更、合并、替换、装备和明确目标卸下仍只由 Drop 提交。
- 真实 Profile 投影按已装备物品的 `ChildContainerId` 显示 `QuickSpatial` 与 `PouchInternal`；空间戒指与储物囊保持 Loaded Spatial 原子拒绝。
- `Start Run` 保持 P4x 去备战边界：P5 本身不读取、初始化、恢复或写入 Code B 局外状态，也未授权局内背包、Loot、搜索、世界物品、撤离/死亡结算或 Run Save 工作。

## 关键实现文件

- `Source\demo_map\CodeB\demo_mapCodeBOutOfRaidProfile.h/.cpp`：OwnerId sidecar、receipt、迁移、原子持久化与恢复。
- `Source\demo_map\CodeB\demo_mapCodeBOutOfRaidProfileTests.cpp`：迁移场景定义，保留供后续 `0.0.9B.F` 统一执行。
- `Source\demo_map\CodeB\demo_mapCodeBP2.*`、`demo_mapCodeBP3.cpp`、`demo_mapCodeBP3UI.cpp`、`demo_mapCodeBP4.*`：真实 Profile layout、动态 child container 投影、页面持久化回调和真实入口 trace 支持。
- `Source\demo_map\demo_mapV3ProgressionManager.*`、`demo_mapSectNavigationWidget.cpp`：正式入口路由与只用于隔离启动的 P5 Profile 种子。
- `Source\demo_map\demo_mapPersistentProfileTypes.*`、`demo_mapProfileRepository.cpp`：只读兼容字段 `LegacySpatialParentItemInstanceId` 的 Profile 序列化；Code A 不消费该字段。
- `PROJECT.md` 与 `PROJECT_INFO_CARD.md`：记录 P5 Profile 所有权、迁移语义、未接管的运行时范围和延后验证的状态。

## 构建与验证安排

本轮最后一次编译产物为 `Saved\Logs\Dev.D.UE.0.0.9B.P5.0.r0_limited_editor_build_retry23.log`，`demo_mapEditor Win64 Development` 返回 `Result: Succeeded`。它只证明当前源可构建，未被用作 P5 验收矩阵的替代品。

按用户最新裁决，以下项目全部留待 `0.0.9B.F` 末期统一执行和审计：

- P1–P5 全量自动化、ProfileSession/NormalStartup/PreparationFlow/Settlement 回归；
- 五种真实 Profile 接管场景、两分辨率真实输入、重启恢复、双 Profile 和活动 Run 拒绝；
- 四类产品 Start Run、默认地图未点击入口 Smoke、当前截图与最终 Game 构建；
- 每份最终日志的 Fatal/ensure/assert/automation/trace/persistence 失败检索；
- 活动工程相对 `0.0.9-XFix1` 的最终 SHA-256 来源审计。

## 需要策划确认

请确认：将 P5 作为“实现已交接、验收与全量验证移至 `0.0.9B.F`”的阶段结点是否符合当前计划。确认前，不应把本阶段标记为 `READY_FOR_CODE_B_RUN_INVENTORY_BRIDGE`，也不会自动开始 P6。

## 当前续办审计（2026-08-06）

策划 Chat 当前可见的最后一个可下载 Prompt 仍为 P5.0.r0；该 Prompt 已在浏览器预览中完整复核。本地工程同时包含独立归档的 P6.0.r0 及其后续 Run Inventory bridge 代码（包括 `NotifySuccessfulRun` 与活动 Run 局外页面锁定）。这不是 P5 的验收授权范围，且当前 Chat 分支未展示相应 P6 指令或 P5 报告交接记录。

本次仅完成了 P5 真实 Profile 入口与持久化边界的最小代码审计，未重新运行 P5 所列全量自动化、截图、Smoke 或重复构建；这些仍按最新执行规则留待 `0.0.9B.F`。因 Chat 指令与现有 P6 源码历史无法在当前分支中一一对应，P5 维持 `NEEDS_PLANNER_DECISION`，未修改游戏源码，也未将 P5 宣告为 READY。
