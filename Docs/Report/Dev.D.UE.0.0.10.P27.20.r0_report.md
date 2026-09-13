# Dev.D.UE.0.0.10.P27.20.r0 Report

## 1. 结论

P27.20 已在 P27.19 的整批持久 Prepare 证据之上建立 `Fdemo_mapShanmenFormationScatterResourceCommitter`，把一次阵法挥洒计划的全部资源预留推进为可恢复、只向前的持久 Commit。执行器在任何写入前重建最终权威快照；一旦发现至少一条 Commit，就不再尝试 Cancel 或重开尝试，而是只补齐规范顺序中尚未提交的其余预留。

本阶段同时产出一份自验证的资源履约证据：P27.18 的每个物理堆叠分配 Slice 都必须且只能映射到一个已提交回执、一个材料 Intent 和一个阵眼需求；数量、顺序、身份与总量全部守恒。证据不完整、重复、串线或存在取消时均失败关闭，不会把部分状态冒充完成。

专项 5/5、前序资源准备 5/5、资源计划 4/4、批次意图 4/4、熟练度操作授权 4/4、熟练度权威适配器 4/4、材料适配器 4/4、Items 77/77、熟练度 2/2、部署 4/4、CombatCore 9/9、完整 `Shanmen.0_0_10` 1,383/1,383、映射自测 492/492、改动驱动覆盖门和 Editor/Game 两目标构建全部通过。

## 2. 阶段问题与范围

P27.19 只把整批资源计划推进到 Pending Prepare。阵法尚不能安全地把这些预留转为真实消耗：若逐行 Commit 的中途持久化失败，前缀已经不可逆；若后续调用重新使用被权威记为拒绝的 Request ID，又会永久重放旧失败；若只依赖内存游标，则无法在重启后判定哪些行已提交。

本轮只解决资源终态提交与可交付证据：

- 输入仍是 P27.18 的不可变资源计划与 P27.19 的整批 Prepare；
- 产品写入继续只经过唯一 ShanmenItems 持久权威，不创建第二库存真值；
- Commit 前必须证明计划全部 Prepare，且不存在 Cancel；
- Commit 按计划中的规范 Reservation 顺序逐条前进；
- 已有 Commit 是不可逆边界，之后只能完成剩余 Commit；
- 每次调用都从持久快照恢复，不信任进程内进度；
- 输出精确映射资源 Slice、材料需求、阵眼需求和持久回执；
- 不提交阵眼、不访问 World、不产生投材飞行或视觉对象。

## 3. 只向前的提交算法

`CommitWithAuthority()` 采用以下固定顺序：

1. 校验计划、授权、部署、批次和输入身份；
2. 要求 Game Thread 与 Ready 权威；
3. 调用 P27.19 Prepare 流程，使所有未取消的 Reservation 都具有精确 Pending Prepare；
4. 捕获最终权威快照并重建每行 Prepare 与终态证据；
5. 若发现任何 Cancel，返回 `AttemptCancelled`，不发 Commit；
6. 若所有行均已 Commit，构建并验证履约证据后返回 `Replayed`；
7. 由当前权威修订号、计划身份和各行已提交/待提交状态派生本次 Commit Pass；
8. 按规范顺序只向尚 Pending 的行发送 Commit；
9. 每次命令后以持久返回结果判定是否可继续；
10. 发生失败时停止本次有界 pass；若此前没有 Commit，返回 `CommitRetryRequired`，若已有持久 Commit，返回 `ForwardRecoveryRequired`；
11. 全部命令完成后重新捕获快照，重新构建并自验证履约证据；
12. 本轮产生新 Commit 返回 `Committed`，纯重放返回 `Replayed`。

算法不尝试伪造数据库原子事务。它承认逐条持久写入可能只完成前缀，并把不可逆前缀显式暴露为向前恢复状态。

## 4. Pass 身份与拒绝恢复

P27.19 的静态 Finalize Request ID 适合 Cancel 回滚：首个终态决定不可翻转。但成功 Commit 的自动恢复还需要处理一种持久事实——权威可能已经把一个 Commit 请求记录为拒绝。若后续调用重复同一 Request ID，幂等账本会持续重放旧拒绝，无法取得进展。

P27.20 因此引入两级确定性身份：

- Commit Pass：命名空间 `demo_map.Formation.ScatterResourceCommitPass.r1`，包含 Plan ID、当前权威 Revision、已提交/待提交行数，以及每行的已提交回执 ID或 pending 标记；
- Commit Request：命名空间 `demo_map.Formation.ScatterResourceCommitRequest.r1`，包含 Pass、Plan、Reservation、Prepare Request 和 Item 身份。

相同持久快照会重建相同身份，因此普通重放保持幂等；持久拒绝会提升权威 Revision，下一次调用得到新的 Pass/Request 身份，只重试尚未消耗的 Pending 行。已经 Commit 的行仍由最终回执直接重放，不会再次发送。

## 5. 履约证据与守恒

`Fdemo_mapShanmenFormationScatterResourceCommitEvidence` 按阵眼和材料组织最终证明。每条 `Fdemo_mapShanmenFormationScatterResourceFulfillmentLine` 同时绑定：

- P27.18 Reservation 与其 Allocation Slice；
- 原始材料 Intent、Item、Run、Purpose 和数量；
- P27.19 Prepare Request 与 Reserved 回执；
- P27.20 Commit Request 与 Committed 回执；
- 对应阵眼的稳定身份和规范顺序。

`IsValid()` 重新派生并校验 Plan、Pass、Request 和各级业务身份，要求：每个 Reservation 恰好出现一次；每个 Slice 恰好履约一次；同一材料 Intent 的分片总量与需求一致；每个阵眼的材料总量与计划一致；全局已提交总量与计划总量一致；所有回执均为成功且处于正确阶段。任何缺行、重复行、错序、错误回执或数量漂移都会得到 `EvidenceInvalid`，不能进入完成态。

物品系统在 Active Run 消耗路径中以持久回执和运行余额表示提交，不直接改写 `Snapshot.Items` 的物理堆叠数量。专项测试因此同时验证“Committed 回执减少运行可用量”和“物理堆叠数量没有被错误地直接改写”，避免把现有权威语义误判为未消耗。

## 6. 自动化证明

| Group | Success | Fail | Log SHA-256 |
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

12 份日志均各含一条预期 `Automation RunTests` 命令、唯一 UE 5.8 原生完成标记、原生退出 0、0 Fail 与 0 Fatal/Unhandled/Ensure。完整根组第一项于 2026-09-13 09:40:39.009 UTC 开始，最后一项于 10:43:31.035 UTC 完成，连续执行约 62 分 52 秒。

五条专项分别证明：

1. `DurableWholeBatchCommitAndReplay`：三行按规范顺序全部持久 Commit，完整证据有效，相同调用不再发命令；
2. `MiddleFailureRequiresForwardRecovery`：中间行失败时保留已提交前缀并进入只向前恢复，下一次只补剩余行；
3. `FirstFailureRetriesWithoutConsumption`：首行保存失败时没有任何 Commit，重试使用新 Pass 后完成且不重复消耗；
4. `PersistedRejectionRotatesCommitPass`：持久拒绝进入账本并提升 Revision，后续请求身份旋转并成功；
5. `CancellationPreventsCommit`：既有 Cancel 终止整次尝试，Commit 命令数严格为零。

## 7. 改动驱动回归

新增 `FormationScatterResourceCommit` 映射规则。六个生产、测试与回归脚本改动要求完整根组、P27.20 Commit、P27.19 Prepare、P27.18 Plan、P27.17 Batch、P27.16 Authorization、P27.15 Authority Adapter、Material Adapter、Items、Formation Mastery、Formation Deployment 和 CombatCore 共 12 组证据。

- regression map JSON：PASS；
- 映射器正反自测：`492/492` PASS；
- 新增正向夹具证明 Commit 改动要求全部 12 组日志；
- 新增负向夹具证明缺任一前序、物品、部署、核心或完整根组都会拒绝；
- 最终暂存覆盖门：`REGRESSION_COVERAGE: PASS Changed=8 Rules=2 Required=12 Logs=12`（其中 6 个实现/测试/映射文件触发规则，2 个交接文档不触发产品规则）；
- `git diff --check` 与最终 `git diff --cached --check`：PASS。

一次覆盖门调用因 PowerShell 参数数组绑定方式错误，在脚本进入覆盖判断前报告空路径；改为 PowerShell 7 命令块传入显式日志数组后，使用同一组证据得到上述 PASS。该修正不涉及产品实现或测试结果。

## 8. 构建与静态边界

- Editor 初次编译：6 actions，`Result: Succeeded`，原生退出 0，UBT 8.38 秒；
- 完整回归后 Editor 最终复核：target up to date，`Result: Succeeded`，原生退出 0，UBT 0.89 秒；
- `UnrealEditor-demo_map.dll`：19,926,016 bytes，SHA-256 `24912E39322839CD0093ABAD140B8E5F384271EBBE0617FE02BF0AABF90897F6`；
- Game：5 actions，`Result: Succeeded`，原生退出 0，UBT 16.12 秒；
- `demo_map.exe`：360,519,680 bytes，SHA-256 `F0C89FFB1EE3B7ACD35E6A9354254D318939300EB488E912CD782303CD8E2042`。

新 Commit 生产代码为 1,207 行非空行（头文件 199、实现 1,008）；包含新增专项后的资源准备测试文件为 981 行非空行。生产文件静态扫描未发现 `UWorld`、`AActor`、角色、GameMode、Profile、SaveGame、直接 InventorySubsystem、计时器、异步、`FGuid::NewGuid` 或 RNG 依赖。

## 9. P/F 边界与下一阶段

P 阶段已证明：整批 Pending Prepare 能按规范顺序持久 Commit；持久前缀不会回滚；普通重放不重复发命令；首行失败可安全重试；持久拒绝可通过 Revision 驱动的新身份恢复；取消会阻断所有 Commit；最终证据能把每个 P27.18 Slice 精确映射回材料与阵眼需求。

F 阶段未执行：没有启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、Cook 或 Package。没有向 World 提交阵眼，没有生成材料飞行、阵法实体、生效反馈或玩家可见表现，因此不声明挥洒布阵功能已完成。

下一独立阶段可消费 P27.20 的不可变履约证据，协调阵眼提交或 World handoff；该阶段必须把“资源已不可逆消耗”作为前置事实，不能重新规划、取消或替换已经 Commit 的 Slice。World/表现接缝仍应保持在资源权威之外。

## 10. GitHub 交接

基线提交：`93cc7541cbda10e445138880b50ea3fef0e49537`（P27.19）。分支：`agent/0.0.10-p27-20-formation-scatter-resource-commit`。本阶段只提交 2 个新 Commit 源文件、2 个资源准备关联文件、2 个回归映射文件、本 Report 与本 Development Log；103 个既有未跟踪用户文件保持未暂存，`Saved/FoundationRuns/Dev.D.UE.0.0.10.P27.20.r0` 与本地构建产物不进入 Git。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p27-20-formation-scatter-resource-commit>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-20-formation-scatter-resource-commit/Docs/Report/Dev.D.UE.0.0.10.P27.20.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-20-formation-scatter-resource-commit/Docs/Log/Dev.D.UE.0.0.10.P27.20.r0_log.md>
