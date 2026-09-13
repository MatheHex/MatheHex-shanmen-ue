# Dev.D.UE.0.0.10.P27.20.r0 Development Log

## 1. 目标

- 把 P27.19 的整批 Pending Prepare 推进为真实持久 Commit；
- 任一 Commit 出现后只允许向前补齐，禁止回滚或重开尝试；
- 所有恢复依据来自最终持久快照，不依赖进程内游标；
- 用权威 Revision 和当前提交形态派生 Pass/Request 身份，使持久拒绝可恢复；
- 生成可自验证的 Slice → 材料 Intent → 阵眼需求 → Commit 回执映射；
- 既有取消必须终止尝试并产生零 Commit；
- 不接 World、不提交阵眼、不产生表现；
- 完成改动驱动回归、双目标构建、Report/Log 与 GitHub 推送。

## 2. 基线与范围

- 基线：`93cc7541cbda10e445138880b50ea3fef0e49537`（P27.19）；
- 分支：`agent/0.0.10-p27-20-formation-scatter-resource-commit`；
- 起始 tracked tree clean；103 个既有未跟踪用户文件保持未暂存；
- 输入为当前熟练度投影、部署、Run 关联和一份有效 P27.18 资源计划；
- Prepare 复用 P27.19，持久终态只经过既有 ShanmenItems 权威；
- 不修改阵图、熟练度、部署、Profile/schema、输入、UI、地图或内容资产；
- 不启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、Cook 或 Package。

## 3. 新增文件与类型

新增：

- `Source/demo_map/demo_mapShanmenFormationScatterResourceCommit.h`；
- `Source/demo_map/demo_mapShanmenFormationScatterResourceCommit.cpp`。

关联修改：

- `demo_mapShanmenFormationScatterResourcePreparation.h`：明确 P27.19 静态终态身份仅用于回滚兼容，成功 Commit 使用 P27.20 Pass 身份；
- `demo_mapShanmenFormationScatterResourcePreparationTests.cpp`：沿用真实持久服务夹具并追加五条 Commit 专项；
- 回归映射与映射器自测：加入本阶段 12 组必跑证据。

主要类型：

1. `Edemo_mapShanmenFormationScatterResourceCommitStatus`：完成、重放、取消、准备拒绝、快照/证据错误与两类恢复状态；
2. `Fdemo_mapShanmenFormationScatterResourceFulfillmentLine`：一条 Reservation 从 Slice 到 Prepare/Commit 回执的完整链；
3. `Fdemo_mapShanmenFormationScatterAnchorResourceFulfillment`：按阵眼聚合的材料履约；
4. `Fdemo_mapShanmenFormationScatterResourceCommitEvidence`：整批可自验证证据；
5. `Fdemo_mapShanmenFormationScatterResourceCommitResult`：状态、计划、命令、证据与诊断；
6. `Fdemo_mapShanmenFormationScatterResourceCommitter`：产品入口、权威算法、请求构造与证据生成。

## 4. 提交与恢复顺序

`CommitWithAuthority()` 的实现顺序：

1. 校验静态输入并确认权威 Ready；
2. 调用 P27.19 Prepare，拒绝任何未形成整批 Pending 证据的计划；
3. 捕获权威快照并重建每条 Prepare/Finalize 证据；
4. 发现 Cancel 时返回 `AttemptCancelled`，不发命令；
5. 全部已有 Commit 时验证证据并返回 `Replayed`；
6. 从 Revision 与当前每行终态构造 Commit Pass；
7. 以 Reservation 规范顺序，只对 Pending 行构造并提交 Commit Request；
8. 首次写入失败且无 Commit 时返回 `CommitRetryRequired`；
9. 已有持久前缀后失败时返回 `ForwardRecoveryRequired`；
10. 下一次调用重新读快照，跳过已 Commit 行，只补尚 Pending 行；
11. 全部完成后再次读快照并生成履约证据；
12. 证据通过后返回 `Committed` 或 `Replayed`。

每个调用最多执行一个向前 pass；失败后不在同一调用无限重试。算法不会在 Commit 已出现后调用 Cancel。

## 5. 身份与持久拒绝

Commit Pass 使用 `demo_map.Formation.ScatterResourceCommitPass.r1`，输入包括 Plan ID、权威 Revision、已提交/待提交统计和每行的最终持久形态。逐行 Commit Request 使用 `demo_map.Formation.ScatterResourceCommitRequest.r1`，输入包括 Pass、Plan、Reservation、Prepare Request 与 Item。

设计结果：

- 相同快照重放相同 Pass，不制造随机身份；
- 已 Commit 行由回执恢复，不重复提交；
- 尚 Pending 行在同一快照中具有稳定 Request ID；
- 被权威持久拒绝后，Revision 变化使下一调用获得新 Request ID；
- 新身份仍绑定原始 Plan/Reservation/Prepare，不允许借身份旋转替换业务内容。

P27.19 的 `demo_map.Formation.ScatterResourceFinalize.r1` 仍服务取消路径。头文件注释已改正，避免把回滚用的共享终态身份误解为成功 Commit 的永久重试身份。

## 6. 履约证据

`BuildEvidence()` 从最终权威快照出发，不使用本轮命令返回值作为唯一事实。每条证据行必须匹配原计划 Reservation、Allocation Slice、材料 Intent、Item、Run、Purpose、Prepare 回执与最终 Commit 回执。

验证同时检查：

- Plan、Pass、Request 等确定性 ID 可重新派生；
- 每个 Reservation/Slice 恰好出现一次；
- 每条回执的操作、阶段、身份、数量和前后余额均正确；
- 每个材料 Intent 的分片和等于原需求；
- 每个阵眼的履约和等于阵眼需求；
- 整批 Commit 总量等于计划总量；
- 阵眼与 Reservation 顺序保持规范稳定。

任一不满足均为 `EvidenceInvalid`。Active Run 的实际消耗语义由提交回执和运行可用量承载，测试明确要求不得错误地直接修改物理 `Snapshot.Items` 数量。

## 7. 专项自动化与故障注入

五条专项使用真实 `FShanmenItemAuthorityService`、真实临时权威目录和真实序列化快照；夹具仅通过同一窄权威接口统计调用并注入一次性持久化失败：

1. `DurableWholeBatchCommitAndReplay`：三条 Prepare 后三条 Commit，证据逐阵眼守恒，重放不发新命令；
2. `MiddleFailureRequiresForwardRecovery`：第二条 Commit 写盘失败，第一条保持 Committed，恢复调用只提交第二、三条；
3. `FirstFailureRetriesWithoutConsumption`：第一条 Commit 失败时三条仍 Pending，没有消耗；新 Pass 重试后仅提交一次；
4. `PersistedRejectionRotatesCommitPass`：拒绝结果持久入账，Revision 与 Request ID 均变化，下一 pass 成功；
5. `CancellationPreventsCommit`：预先存在的 Cancel 使结果固定为 `AttemptCancelled`，Commit 调用严格为零。

专项结果：5 Success、0 Fail；日志 SHA-256 `B4DBFEBB9BA2761252E35AF8A03299FAE491302BB90F9A3960C79A2313F9355C`。

## 8. 自动化与回归结果

| Group | Success | Fail | SHA-256 |
|---|---:|---:|---|
| `Shanmen.0_0_10.Product.FormationScatterResourceCommit` | 5 | 0 | `B4DBFEBB9BA2761252E35AF8A03299FAE491302BB90F9A3960C79A2313F9355C` |
| `Shanmen.0_0_10.Product.FormationScatterResourcePreparation` | 5 | 0 | `5AB22F4FDC93B15804DADCBC91AA684F815BC42887F4E33CB7956E8915E4A5A2` |
| `Shanmen.0_0_10.Product.FormationScatterResourcePlan` | 4 | 0 | `8E2A0484BE8AD14CD7729C96A28A50077B7F38BE8FEF605B2F9C4FC641D54A93` |
| `Shanmen.0_0_10.Product.FormationScatterBatchIntent` | 4 | 0 | `BACD681B1D957E763E81E5A8DFA7DFD1288FF74FB330AEC525AB652A6C09B36E` |
| `Shanmen.0_0_10.Product.FormationMasteryOperationAuthorization` | 4 | 0 | `460E62C20E4CA6404023FC97F51379F873EFD8A3A5A8D729311D62FBC4094500` |
| `Shanmen.0_0_10.Product.FormationMasteryAuthorityAdapter` | 4 | 0 | `7D0C8D6427A88518D4F84F9ACBF438FBEC4F0F41DEF56ACB6D92FA8767F076FE` |
| `Shanmen.0_0_10.Product.FormationMaterialAdapter` | 4 | 0 | `64D42A809F0FD8CCA62D8302B2D70100281118721D2F4B5314E256F593C26C1D` |
| `Shanmen.0_0_10.Items` | 77 | 0 | `06A418D50828DD8DB4D1A7025C33A31C6F20717D0957947CFEFF161CB331A866` |
| `Shanmen.0_0_10.CombatRuntime.FormationMastery` | 2 | 0 | `3ECBCE9D39D8435D0D2DFD9C904D6B730B588726DF7AABDB5C385E5DE0999B0A` |
| `Shanmen.0_0_10.CombatRuntime.FormationDeployment` | 4 | 0 | `E1F71F4032575EED8472A91B2CDC93E83A0C8B77BDAAA9BC7CE0A00C0725A86A` |
| `Shanmen.0_0_10.CombatCore` | 9 | 0 | `2C0F0A552A2AB3BDF23FF3589B301D14BC1BFF237D738C00262D56C0E365F0CA` |
| `Shanmen.0_0_10` | 1,383 | 0 | `072D5A22140D96C8C3A6DDA86253A5115C1094F3D970FEE7BF2C715131EBFCF0` |

12 份日志全部原生退出 0，每份各有一条预期 RunTests 命令、唯一完成标记、0 Fail 与 0 Fatal/Unhandled/Ensure。完整根组从 2026-09-13 09:40:39.009 UTC 到 10:43:31.035 UTC，约 62 分 52 秒。

回归映射：

- JSON 解析 PASS；
- 映射器正反自测 `492/492` PASS；
- 最终暂存覆盖门 `REGRESSION_COVERAGE: PASS Changed=8 Rules=2 Required=12 Logs=12`（6 个实现/测试/映射文件触发规则，2 个交接文档不触发产品规则）；
- 首次覆盖门调用的日志数组使用了不兼容的参数绑定写法，在覆盖判断前因空路径退出；改成 PowerShell 7 命令块显式传参后通过；
- `git diff --check` 与最终 `git diff --cached --check` PASS。

## 9. 构建、静态边界与 P/F

Editor：

- 初次编译 6 actions，`Result: Succeeded`，原生退出 0，UBT 8.38 秒；
- 最终复核 target up to date，`Result: Succeeded`，原生退出 0，UBT 0.89 秒；
- `UnrealEditor-demo_map.dll`：19,926,016 bytes，SHA-256 `24912E39322839CD0093ABAD140B8E5F384271EBBE0617FE02BF0AABF90897F6`。

Game：

- 5 actions，`Result: Succeeded`，原生退出 0，UBT 16.12 秒；
- `demo_map.exe`：360,519,680 bytes，SHA-256 `F0C89FFB1EE3B7ACD35E6A9354254D318939300EB488E912CD782303CD8E2042`。

新 Commit 生产代码 1,207 行非空行，包含新增专项后的关联测试文件 981 行非空行。生产代码静态边界扫描没有 World/Actor/Character/GameMode/Profile/SaveGame/直接 InventorySubsystem、计时器、异步、随机 GUID 或 RNG 依赖。

P 阶段完成真实持久 Commit、幂等重放、前缀失败向前恢复、首行失败无消耗、持久拒绝身份旋转、取消阻断、证据守恒、完整回归与双构建。F 阶段未启动 UI/PIE/Standalone/产品 exe，也未提交阵眼、接入 World、执行真实输入或视觉验证。

## 10. 精确提交清单

1. `Source/demo_map/demo_mapShanmenFormationScatterResourceCommit.h`
2. `Source/demo_map/demo_mapShanmenFormationScatterResourceCommit.cpp`
3. `Source/demo_map/demo_mapShanmenFormationScatterResourcePreparation.h`
4. `Source/demo_map/demo_mapShanmenFormationScatterResourcePreparationTests.cpp`
5. `Scripts/ShanmenRegressionMap.json`
6. `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`
7. `Docs/Report/Dev.D.UE.0.0.10.P27.20.r0_report.md`
8. `Docs/Log/Dev.D.UE.0.0.10.P27.20.r0_log.md`

`Saved/FoundationRuns/Dev.D.UE.0.0.10.P27.20.r0` 与本地构建产物不进入 Git；103 个既有未跟踪用户文件保持未暂存。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p27-20-formation-scatter-resource-commit>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-20-formation-scatter-resource-commit/Docs/Report/Dev.D.UE.0.0.10.P27.20.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-20-formation-scatter-resource-commit/Docs/Log/Dev.D.UE.0.0.10.P27.20.r0_log.md>
