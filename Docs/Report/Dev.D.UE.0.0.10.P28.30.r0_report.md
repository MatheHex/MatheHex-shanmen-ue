# Dev.D.UE.0.0.10.P28.30.r0 Report

## 1. 结论与范围

`P28_30_AUDIT_COMPLETE`，不是生成奖励功能 PASS，也不是最终冻结。2026-09-21 UTC，基线 `2b52268df0dbe134d7ff4a713ecd39e0dade1813`。本轮沿 P28.29 的实际拒绝证据完成生成来源的读写、恢复与终局契约审计；仅提交 Report、Development Log 和冻结索引，生产/测试/配置/资产零改动。

关键结论：缺口不只是 Manager 调错一个服务。新权威尚无生成来源的持久接收/读取命令；旧提交端在切换后应当继续关闭；而现有撤离导入明确拒绝已经存在于新物品图的身份。因此不能先把生成物插入图，再假设原终局不需要改动。

## 2. 实际调用与信息分区

以下为当前代码事实；只有 P28.29 的 M01 变体有本轮复跑的实际拒绝证据，不将静态共用链当作每种箱子/尸体都已动态验收。

| 边界 | 当前入口/真值 | 已确认问题或限制 |
|---|---|---|
| 生成规划 | [Projection/Receipt](../../Source/demo_map/demo_mapRewardSourceProjection.h) | 保存内容身份、来源角色、seed、计划堆叠及保底输入/输出；不能用单个 seed 替代整个已接受结果 |
| 箱子与尸体 | [LootChest](../../Source/demo_map/demo_mapLootChest.cpp)、[Corpse](../../Source/demo_map/demo_mapCorpseContainerActor.cpp) | 箱子的投影规划/旧生成器两分支和尸体生成分支都经 Manager.PrepareGeneratedRewardSource；M01 包装同一尸体入口 |
| 读取与接收 | [Manager](../../Source/demo_map/demo_mapV3ProgressionManager.cpp) 的 CanGenerateRewardSource / FindDurablyAcceptedRewardSource / PrepareGeneratedRewardSource | 当前从旧 Profile 读已接受来源、恢复本地 receipt/pity，再向旧 Profile 写；不是 Shanmen durable 端口 |
| 旧端拒绝 | [ProfileCoordinator](../../Source/demo_map/demo_mapProfileSessionCoordinator.cpp) | 要求旧 RunActive、匹配旧 ActiveRunId；新 StartPreparedRun 不会建立旧 Run |
| 切换后写入隔离 | [ProfileRepository](../../Source/demo_map/demo_mapProfileRepository.cpp)、[WriteFence](../../Source/demo_map/demo_mapShanmenLegacyItemWriteFence.cpp) | 新权威 primary/backup 存在即关闭旧物品写入；比较包含 GeneratedRewardSources。即使绕过上层 RunActive，改变该来源仍须被拒绝；不扩大为所有 Profile 非物品字段只读 |
| Runtime 物化 | [SearchContainer](../../Source/demo_map/demo_mapSearchContainerActor.cpp) 的 InitializeCommittedSearchContainer | 需要已接受 ContainerId、有序 ItemInstanceId、section/slot 和完整元数据；只向瞬态 Runtime 物化，不是持久提交 |
| 新权威接收面 | [Service](../../Source/ShanmenItems/Public/ShanmenItemAuthorityService.h)、[Repository](../../Source/ShanmenItems/Public/ShanmenItemRepository.h)、[Snapshot](../../Source/ShanmenItems/Public/ShanmenItemTypes.h) | 现有公开命令/快照无生成来源回执、容器恢复载荷和保底序列的对应端口；RewardMetadata 只是物品元数据，不能证明来源已被接受 |
| 终局 | [RunAdapter](../../Source/demo_map/demo_mapShanmenRunLifecycleAdapter.cpp)、[Repository implementation](../../Source/ShanmenItems/Private/ShanmenItemRepository.cpp) | AcquiredItems 只在 Extraction 导入；FinalizePreparedRun 对 State.Items 中已存在身份返回 AcquiredItemMismatch。现有逻辑是“首次导入”，不是“归档已存在局内物品” |

## 3. 最小实施顺序与验收门槛

下面是本次审计导出的实施约束，不声称新接口或 schema 已存在。继续沿既有唯一权威完成，不另建奖励存档、通用日志系统或第二份可变库存。

1. **先补来源接收契约。** 在 ShanmenItems 内表达同 Owner/活动 Run/稳定来源的规范化输入与不可变接收证据；明确内容版本、完整有序计划、容器及实例身份、保底前后值与顺序。相同输入返回原回执；同来源冲突、错误 owner/run、终局后新接收均拒绝。领域内核不得依赖 demo_map、World 或生成器/RNG；物品元数据与展示记录不可充当接收凭证。
2. **持久化与 schema 同步。** 使用现有单一 AuthorityService 的候选提交/恢复机制。若扩展快照，显式迁移当前 schema 2 及仍受支持的 schema 1，先验证旧摘要再升级，旧 Run 与 request ledger 不重写成新身份。原请求在写前失败、结果不确定、重启后分别保持/恢复，不能靠重新生成 ID 或重新抽取计划脱困。
3. **再接产品读取和物化。** 新流程的查询、接收、receipt/pity 重建必须使用同一新端口；非 cutover 的旧兼容流程仍保留，RecoveryRequired 不退回旧 writer。箱子、普通尸体与 M01 都取已接受身份；持久成功而 Runtime/World 失败时保留可恢复结果，不重新生成、不把故障当成新来源。
4. **接收与终局一起闭合。** 明确“来源已接受、尚未拿取”“已获物品”“已消耗/遗失”到 Extraction/Death/Abandon 的去向。若接收阶段已进入新物品图，必须同步改变现有首次导入分支为受来源约束的状态转换，防止重复入库和元数据漂移；不能只删除 State.Items.Contains 检查。若先做不可变来源回执，它本身不等于获物/消耗权威已完成，不能据此宣布 FZ-1 关闭。

下一代码增量优先是第1项的有界领域契约及确定性/拒绝测试，然后接同文档持久化；不要继续用新增投影包装层掩盖接收命令缺失。每步保持显式未闭合项，最终须有“生成→持久接收→恢复/物化→获物或消耗→终局重放”的组合证据。这里只要求底层测试夹具，不接真实输入、内容资产或玩法数值。

容量方面，完整来源计划与请求历史会增加现有全快照复制/保存成本；实施时至少记录来源数、条目数与序列化字节，不复制完整容器列表到 UI 查询。尚无新增结构或分配器/RAM 实测，不能给出内存完成百分比，也不以删除重放历史节省内存。

## 4. 验证与交接

本轮现跑 Cutover 4/0、RunLifecycle 10/0、WorldLifecycle 7/0，共21个独立用例，原生0且精确结束队列。既有 ManagerDeactivationRetention 包含有效 M01 规划遭旧提交拒绝的变体；这项测试通过表示“按当前拒绝契约保持并清理正确”，不是奖励生成成功。Cutover 测试证明既有写入隔离，生成来源字段比较由静态链确认，不伪称专门动态测试过该字段的非法写入。

Editor/Game Development 检查原生0/0、均 up-to-date，未发生新编译。1617项产品/验证输入与 P28.29 完全相同，因此完整新根1430/0、旧根1330/0明确复用 P28.29，不声称本轮重跑2760项。三文档路径由既有覆盖检查器分类；原件路径/SHA见 [Development Log](../Log/Dev.D.UE.0.0.10.P28.30.r0_log.md)。

103个原未跟踪文件与两份 OverallReadiness 用户修改保持不变。FZ-1/2仍开放，FZ-3不启动、不暂停自动化。遵守 [P阶段基线](../Process/P_STAGE_BASELINE_0_0_10.md)；未启动 Editor UI/PIE/Standalone/游戏程序、Smoke/Cook/Package，未改输入、地图、内容、UI 或玩法。
