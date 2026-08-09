# Dev.D.UE.0.0.9B.I0.0B.r0 Report

## 最终结论

- task_id: Dev.D.UE.0.0.9B.I0.0B.r0
- final_status: READY_FOR_P1_CODE_B_PLANNING_WITH_NONBLOCKING_FINDINGS
- formal_P_stage_started: false
- superseded_task: Dev.D.UE.0.0.9B.I0.0.r2
- superseded_reason: SUPERSEDED_BY_I0.0B.r0
- execution_date: 2026-08-04

本任务已接受用户在 2026-08-04 作出的最终基线裁决：Dev.D.UE.0.0.9-XFix1 是 0.0.9B 的唯一工程级开发基线；0.0.9B 新开发的背包、仓库、装备、Loot、搜索、拖拽、转移和持久化能力改由代码 B 作为最终权威。本任务恢复并核验活动工程，完成 A/B 权威边界和替换清单登记，但没有实现代码 B，也没有进入正式 P 阶段。

## 既有任务处置

- I0.0.r0：其真实来源、构建、Smoke 和源码盘点证据继续有效。旧版拒收原因是当时的 0.0.8 来源约束；该约束已被用户最新裁决取代。
- I0.0.r1：其阻断结论 VERIFIED_0.0.8_SOURCE_NOT_FOUND 已被 2026-08-04 用户决策取代。原报告保留为历史记录，未删除。
- I0.0.r2：本轮只进行了 Prompt 的页面读取和归档动作，没有执行其 0.0.8 搜索路线，也没有产生工程源文件改动；登记为 SUPERSEDED_BY_I0.0B.r0，不生成 r2 Report。

## 基线、恢复与保护证据

- 合法基线源：C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9-XFix1
- 活动根：C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B
- 活动工程：C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/demo_map.uproject
- r0 隔离副本：C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B_QUARANTINE_I0R0_FROM_0.0.9-XFix1_20260804T194204
- 引擎：C:/Program Files/Epic Games/UE_5.8
- 恢复路线：从已审计的 r0 隔离副本复制恢复活动根；隔离副本继续保留，便于审计和回滚。
- 活动根恢复前不存在；恢复后确认 demo_map.uproject、Source、Content、Config 存在。
- 原 XFix1 源本任务前后均保持原位置；本轮对其只执行 Test-Path、文件计数、.uproject 读取等只读检查，没有对源目录执行写入、移动、删除或覆盖。
- 源工程当前文件计数核验：4428 个文件；隔离副本恢复前文件计数：2586 个文件。

## 本任务文件动作

- 在隔离副本 Docs/Prompt 中归档：Dev.D.UE.0.0.9B.I0.0B.r0_prompt.md。
- 将完整隔离副本复制为活动根 Dev.D.UE.0.0.9B；未删除隔离副本。
- 更新活动根 PROJECT_INFO_CARD.md 与 PROJECT.md，标记合法基线、A/B 边界、r2 被取代、当前 I0.0B.r0 和正式 P 尚未开始。
- 新建本 Task Report：Dev.D.UE.0.0.9B.I0.0B.r0_report.md。
- 保留 r0 Prompt、r0 Report、r1 Prompt、r1 Report 和 r1 隔离现场资料；没有删除旧报告。
- 构建生成或更新的 Binaries、Intermediate、Saved/Logs 属于活动工程的生成物；没有修改代码 A 的源码。

## 构建与 Smoke

执行方式为 UE 5.8 增量 Development 构建，不做 Cook、Package 或新的 Latest_Demo。

1. demo_mapEditor Win64 Development：Result Succeeded，ExitCode 0，Target up to date，0 action，输出 UnrealEditor.exe。
2. demo_map Win64 Development：Result Succeeded，ExitCode 0，Target up to date，0 action，输出 C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Binaries/Win64/demo_map.exe。
3. Editor Game Smoke：UnrealEditor.exe demo_map.uproject -game -nullrhi -unattended -nop4 -nosplash -NoSound -ExecCmds=Quit；进程 ExitCode 0。
4. Smoke 日志：C:/AIDev/shanmen-ue/Dev.D.UE.0.0.9B/Saved/Logs/demo_map.log。
5. Smoke 结果：加载 /Game/M01/Maps/L_M01_Expedition?Name=Player；LogExit: Exiting；Fatal lines 0；Error lines 0。

因此活动工程满足本任务的最小接收复核。以上 Smoke 复用了 r0 的有效工程级行为，仅进行了增量复核；没有将其解释为代码 B 功能验收。

## 更新后的项目定位

- Dev.D.UE.0.0.9B 当前阶段：I0.0B.r0。
- 工程级基线：Dev.D.UE.0.0.9-XFix1。
- 0.0.8 源搜索：已由用户最新决策取消，不再继续。
- 代码 A：XFix1 中现存的物品、容器、装备、仓库、搜索、世界物品、结算和 UI 实现，继续用于维持当前游戏可运行，但不再是 0.0.9B 新背包功能的设计权威。
- 代码 B：后续 P 阶段新建的 1×1 物品、位置、容器、Repository、事务、快照 UI 和终局持久化体系；代码 B 接管领域后拥有唯一写入权威。
- 当前仍未进入正式 P 阶段。

## 代码 A 现场盘点

### 主要权威与数据

- Item Definition / Instance / Authority：Source/demo_map/demo_mapItemDefinitions.h、demo_mapItemTypes.h、demo_mapItemAuthority.h/cpp。
- Runtime Container：Source/demo_map/demo_mapRuntimeContainer.h/cpp 中的 Fdemo_mapRuntimeContainerAuthority，以及容器 Section、Equipment、Body、SpatialStorage 投影。
- GameInstance 入口：Source/demo_map/demo_mapItemSubsystem.h/cpp 中的 Udemo_mapItemSubsystem，连接物品 Authority、Runtime Container、WorldItem 和交互。
- Profile / Repository / Session：demo_mapProfileRepository、demo_mapPersistentProfileTypes、demo_mapProfileSessionCoordinator、demo_mapProfileSessionSubsystem。
- Preparation / Equipment / Stash：demo_mapProfilePreparationTypes、demo_mapProfilePreparationFlow、demo_mapPersistentPreparationTransaction、demo_mapEntityLoadoutPresenter。
- Settlement / Run：demo_mapProfileSettlementTransaction、demo_mapProfileSettlementTypes、demo_mapM01Extraction、demo_mapGameMode、demo_mapSettlementWidget。

### UI、Loot 与世界交互

- Inventory UI：demo_mapInventoryWidget、demo_mapItemPresentation、demo_mapEntityLoadoutPresenter。
- Search / Loot UI：demo_mapSearchContainerWidget、demo_mapSearchContainerPresenter、demo_mapSearchContainerActor、demo_mapSearchContainerTypes。
- Corpse / Body：demo_mapCorpseContainerActor、demo_mapRuntimeContainer、demo_mapFixedLootTableRegistry。
- Loot Chest：demo_mapLootChest、demo_mapFixedLootTableTypes。
- World Drop / Pickup：demo_mapWorldItem、demo_mapWorldPresentation、demo_mapV3ProgressionManager、demo_mapItemSubsystem。
- 旧测试和调试入口：demo_mapItemTests、demo_mapGridInventoryTests、demo_mapWorldInteractionTests、demo_mapDualLootSliceTests、demo_mapFullSystemLoopTests、demo_mapV3ProgressionManager 中的自动化流程。

## A/B 替换清单

| 领域 | 代码 A 主要类/文件 | 非背包依赖 | 代码 B 目标权威 | 迁移方式 | 切换时关闭的旧写入入口 | 主要风险 |
|---|---|---|---|---|---|---|
| Item Definition / Item Instance | demo_mapItemDefinitions、demo_mapItemTypes、Fdemo_mapItemAuthority | EquipmentEffect、Gameplay Tags、LootTable | CodeB ItemDefinition + ItemInstance，稳定 GUID、数量、父位置、1×1 尺寸 | 只读适配 + 一次性实例转换 | ItemAuthority 的 Create/Move/Quantity 生产写入 | GUID 重复、数量分裂、定义与实例混淆 |
| Repository / Authority | Fdemo_mapItemAuthority、Udemo_mapItemSubsystem、Fdemo_mapProfileRepository、Fdemo_mapRuntimeContainerAuthority | GameMode、Profile Session、WorldItem | CodeB Repository + 单一事务入口 | 先桥接快照，再单向接管 | 旧 Authority mutator、Subsystem 直接写入、RuntimeContainer 双写 | 双权威、事务半提交、撤离后状态不一致 |
| Container / Equipment / Stash / Base6 / Spatial Storage | demo_mapRuntimeContainer、demo_mapEntityLoadoutPresenter、demo_mapProfilePreparationTypes、demo_mapPersistentPreparationTransaction | Preparation Flow、Attribute Effect | CodeB Location/Container Graph；EquipmentSlot 与 StorageSlot 同一位置模型 | 适配旧快照，后续一次性迁移 | PreparationTransaction、Loadout Widget 的旧槽位写入 | 装备与背包父子关系错乱、空间容量复制 |
| Quickbar | EntityLoadoutPresenter 的 BaseQuickItemSlots/RingQuickItemSlots、ProfilePreparationWidget、ItemAuthority 快照 | Input、Character Ability、Use Item | CodeB Quickbar 只引用真实 ItemInstance，不持有数量 | 引用投影适配 | 旧 Quickbar 直接扣数量或写位置入口 | 快捷栏悬挂引用、使用与结算双扣 |
| Loot / Search / Corpse / Body Container | SearchContainerActor/Presenter/Widget、CorpseContainerActor、RuntimeContainer、FixedLootTableRegistry、LootChest | AI Death、Search Timer、M01 Run | CodeB Search Snapshot + Reveal State + Reward Materialization | 资产和读条复用；状态单向桥接 | 旧 Search Take、Corpse Loot 生成和 RuntimeContainer 写入 | 先揭示后奖励、重复领取、尸体终局丢失 |
| Drag-and-Drop / Context Action / World Drop | InventoryWidget、SearchContainerWidget、V3ProgressionManager、WorldItem、ItemSubsystem | PlayerController Input、Collision、HUD | CodeB Operation Request + Validation Result；UI 只提交请求 | 先保留 UI 外观，替换请求出口 | Widget 直接改 Authority、WorldItem 直接转移数量 | 非法位置、输入锁、WorldItem 与库存双份 |
| Run Settlement / Death / Extraction / Abandon | ProfileSettlementTransaction、M01Extraction、GameMode、SettlementWidget、ProfileSessionCoordinator | Run Lifecycle、Map、Death、Reward | CodeB Settlement Commit 只执行一次并产出 Profile Snapshot | 一次性结算适配，禁止双提交 | SettleRunItems、M01 结算、GameMode 终局写入 | 撤离/死亡/放弃重复结算、战利品复制 |
| Profile Schema / Save / Load / Migration | PersistentProfileTypes、ProfileRepository、ProfileSessionSubsystem、Profile*Transaction | SaveGame、Startup、Profile Version | CodeB Versioned Profile Schema + 显式 Migration | 版本迁移器；旧字段只读兼容 | 旧 Profile 字段与新字段同时可写 | 存档兼容、旧字段回写、崩溃中断 |
| Inventory / Warehouse / Search / Details UI | InventoryWidget、ProfilePreparationWidget、SearchContainerWidget、ItemPresentation、EntityLoadoutPresenter | UMG、Input、HUD、Navigation | CodeB Read Snapshot + Operation Request，UI 不作数据权威 | 复用视觉资产，逐界面切换快照源 | Widget Presenter 直接写数据、旧拖拽回调 | UI 显示快照过期、双输入入口、状态回滚不一致 |
| Automation / Invariant / Debug | ItemTests、GridInventoryTests、WorldInteractionTests、DualLootSliceTests、FullSystemLoopTests、V3ProgressionManager | Automation Framework、Editor/PIE、GameMode | CodeB Invariant Suite：唯一 GUID/位置/事务/终局结算 | 新旧测试并行一段时间，最终以 B 测试为门槛 | 旧自动化直接调用 A 写入口 | 测试假绿、A/B 同时通过但状态不同、调试命令误写 |

## 代码 B 边界与本任务明确不做

本任务没有创建新的 CodeB 源码目录、Repository、事务、存档迁移或 UI，也没有把旧物品数据迁入 CodeB。没有删除代码 A，没有改写地图、怪物、战斗、奖励、Run、撤离或美术。现阶段允许代码 A 继续驱动已存在的可运行游戏；后续新增背包正式能力必须进入 CodeB，并在某一纵向流程接管后关闭对应 A 写入入口。

## 下一份单一 P 任务建议

在独立 CodeB 边界内建立 1×1 物品的唯一 Item Instance、唯一 Location/Container、Repository 与最小事务核心；用不变量测试证明 Move、Swap、Merge、Split、Equip、Unequip 不会产生双重权威。暂不制作完整仓库、Loot、详情或拖拽 UI。

## 仍需策划判断的事项

- CodeB 的最终目录/命名空间前缀可在下一份 P 任务中依据真实工程习惯确认。
- A/B 接管顺序建议先 Item/Location/Repository，再接 Preparation/Equipment，最后接 Loot/Settlement/UI；若策划要求不同顺序，应在 P 任务中明确。
- 本轮没有阻断性工程发现；READY 状态不代表已经开始 P 阶段。

## 最终结论

活动 Dev.D.UE.0.0.9B 已恢复为合法 XFix1 工程级基线，源工程未修改，活动工程完成 UE 5.8 Development 增量构建和基础启动 Smoke。I0.0B.r0 已完成接收、定位和 A/B 边界登记，可交由策划规划下一份单一 CodeB P 任务；本轮不自动进入 P 阶段。
