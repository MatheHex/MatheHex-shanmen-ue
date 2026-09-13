# Dev.D.UE.0.0.10.P27.19.r0 Development Log

## 1. 目标

- 把 P27.18 的整批资源计划提交到唯一 ShanmenItems 持久权威；
- 每个物理堆叠只持久 Prepare 一次，重放只读取精确既有结果；
- 中途失败时进行一次有界逆序取消，并在无法证明取消完成时暴露恢复态；
- 从持久快照恢复，不依赖进程内游标或临时 Session；
- 为未来 Commit/Cancel 固定同一个逐行终态身份，使首个持久决定不可翻转；
- 不消耗 Quantity、不提交阵眼、不接 World；
- 完成改动驱动回归、双目标构建、Report/Log 与 GitHub 推送。

## 2. 基线与范围

- 基线：`0ea6f204115dce4fbd10810dbe65cbf4a4fba522`（P27.18）；
- 分支：`agent/0.0.10-p27-19-formation-scatter-resource-preparation`；
- 起始 tracked tree clean；103 个既有未跟踪用户文件保持未暂存；
- 输入为当前熟练度投影、部署、Run 关联和一份有效 P27.18 资源计划；
- 产品写入只经过既有 `Udemo_mapShanmenItemAuthoritySubsystem`；
- 不修改阵图、熟练度、部署、Profile/schema、输入、UI、地图或内容资产；
- 不启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、Smoke、Cook 或 Package。

## 3. 新增文件与类型

新增：

- `Source/demo_map/demo_mapShanmenFormationScatterResourcePreparation.h`；
- `Source/demo_map/demo_mapShanmenFormationScatterResourcePreparation.cpp`；
- `Source/demo_map/demo_mapShanmenFormationScatterResourcePreparationTests.cpp`。

主要类型：

1. `Edemo_mapShanmenFormationScatterResourcePreparationStatus`：成功、取消、过期与三类恢复状态；
2. `Idemo_mapShanmenFormationScatterResourceAuthority`：Ready、快照、Prepare、Finalize 四项最窄串行权威接口；
3. `Fdemo_mapShanmenFormationScatterResourcePreparationResult`：计划、命令、逐行 Prepare/Finalize 回执及状态诊断；
4. `Fdemo_mapShanmenFormationScatterResourcePreparation`：产品入口、可测试算法和终态请求构造器。

## 4. 执行顺序

`PrepareWithAuthority()` 使用以下固定顺序：

1. 检查 Plan 内部有效性；
2. 要求 Game Thread 和 Ready 权威；
3. 捕获权威快照；
4. 按计划中每条 Reservation 的精确身份重建 Prepare/Finalize 证据；
5. 若发现 Commit，直接进入 `ForwardRecoveryRequired`；
6. 若发现 Cancel 或已拒绝 Prepare，先用一次逆序 pass 清理其余 Pending；
7. 若计划范围已过期：无 Pending 时命令前失败，有 Pending 时先取消；
8. 若计划从未触碰，使用当前物品快照再次验证 P27.18 `IsCurrentPlan()`；
9. 按计划规范顺序调用每条冻结 Prepare 请求；
10. 任一行失败即逆序取消所有早期 Pending；
11. 全部命令返回成功后重新读取快照，证明整批唯一、精确且仍 Pending；
12. 至少一个新写入返回 `Prepared`，全部为权威重放返回 `Replayed`。

## 5. 证据与终态身份

成功 Prepare 回执必须同时匹配：操作 `PreparePreparedRunQuantityIntent`、Reserved 阶段、Request ID、Intent ID、Item ID、Quantity、冻结前值、未消耗后的实际值、可用后值、Purpose 和 Active Run。

终态回执必须匹配 `FinalizePreparedRunQuantityIntent`、对应 Prepare Request、同一 Intent/Item/Quantity/Purpose/Run，以及 Committed 或 Cancelled 后应有的数量结果。重复证据、交叉 Request、混合 Commit/Cancel 或被不兼容结果占用的身份全部失败关闭。

`BuildFinalizeRequest()` 使用命名空间 `demo_map.Formation.ScatterResourceFinalize.r1`，覆盖 Plan、Intent、Prepare Request 和 Item 身份；Commit 与 Cancel 故意共享 Request ID。ShanmenItems 已处理请求账本因此把第一个持久终态固定为不可翻转的事实。

## 6. 有界回滚与恢复

`RollbackPending()` 每次只进行一个逆序 pass：重新读快照、定位精确 Pending、逐条 Cancel、再次读快照并证明最终状态。它不会无限重试，也不会仅凭 API 返回值宣称清理成功。

恢复语义：

- 未完成回滚：`RollbackRecoveryRequired`；
- Prepare 均报告成功但最终证据无法确认：`PreparationRecoveryRequired`；
- 任一 Commit 已存在：`ForwardRecoveryRequired`；
- 下一次调用总是重新扫描权威，精确重放已成功行并只补缺失行；
- 任何 Cancel 一旦出现，后续调用只会取消余项，不会重新开始该计划。

## 7. 专项自动化与故障注入

五条专项：

1. `DurableWholeBatchAndReplay`：3 条 Prepare 首次成功、数量不变、再次调用 3 条均为 `Replayed`，且 Commit/Cancel Request ID 相同；
2. `MiddleFailureRollsBackEarlierLines`：在第二次 Prepare 的 `WriteTemp` 注入失败，真实服务返回 `PersistenceFailedRolledBack`，第一行随后成功 Cancel；
3. `RollbackRecoveryAndExactResume`：第二次 Prepare 和第一次 Finalize 均注入一次写盘失败，首次保留恰好 1 条 Pending 并返回恢复态，下一 pass 重放/补齐为 3 条 Pending；
4. `StalePlanFailsBeforeMutation`：先持久加入外部 Pending 使全新计划失去当前性，执行结果不发 Prepare/Cancel 且前后快照相同；
5. `PartialCancellationTerminatesAttempt`：先人工取消第一行，算法取消余下两行，再次调用不产生任何命令。

夹具使用真实 `FShanmenItemAuthorityService`、真实临时权威目录和真实序列化快照；只在窄接口层统计调用与注入一次性写盘失败。每个夹具使用独立随机测试目录，测试结束只删除自身目录。

## 8. 自动化与回归结果

| Group | Success | Fail | SHA-256 |
|---|---:|---:|---|
| `Shanmen.0_0_10.Product.FormationScatterResourcePreparation` | 5 | 0 | `1D21189B332AC58F95BDF2A1E0436D24FFE1C0344211F51B8EAC950072866EF3` |
| `Shanmen.0_0_10.Product.FormationScatterResourcePlan` | 4 | 0 | `012A3E8F5ADBC8536C7BCF6F8AE303F345C93AF7DAC79617EC9972F3D3AE76B5` |
| `Shanmen.0_0_10.Product.FormationScatterBatchIntent` | 4 | 0 | `C597B2D24200CC6BA4BCE77636A8F692C2AA100F93BD1E683420A13932B7FDFF` |
| `Shanmen.0_0_10.Product.FormationMasteryOperationAuthorization` | 4 | 0 | `12FDB76D9BEF52E66B7996A17A77FC46C1D4F4275C6CCAA9851014AEC25008A0` |
| `Shanmen.0_0_10.Product.FormationMasteryAuthorityAdapter` | 4 | 0 | `30A8014A2EAD3090B398627D3FBBA4314F5A295A7B71A02AC1FF9272E87715E0` |
| `Shanmen.0_0_10.Product.FormationMaterialAdapter` | 4 | 0 | `EAA81CFB1FE79A1C59BD9C3161CC696E55E49EA900C1549CD69A4937F3539E09` |
| `Shanmen.0_0_10.Items` | 77 | 0 | `CA2A863189BDCE21934BF945E9046D0F27A6AF99911A2D8B25893C19E83BCB10` |
| `Shanmen.0_0_10.CombatRuntime.FormationMastery` | 2 | 0 | `AEBF5EA19A1FC77FAC280634FD1209263AF942BFC3D5F283D6B1339BFFE7DB88` |
| `Shanmen.0_0_10.CombatRuntime.FormationDeployment` | 4 | 0 | `23C9E799777C1FAE64C0A91559973753E3AE9D025F225433C725BBC6A8D23079` |
| `Shanmen.0_0_10.CombatCore` | 9 | 0 | `EFC701FD752D57167ADFF97585C02A70C6D98F2C730EE579A377C807DDE1DBA2` |
| `Shanmen.0_0_10` | 1,378 | 0 | `E05EB963989FDE33A734C9C513B0171C5C7D9954932FE4D61408CCC5FA8E1E2A` |

11 份日志均为原生退出 0、唯一 UE 5.8 完成标记、0 Fail、0 Fatal/Unhandled/Ensure。完整根组第一项从 2026-09-13 07:36:41.470 UTC 开始，最后一项于 08:38:39.224 UTC 完成；完整进程执行 3,733.721 秒。

回归映射：

- 新增 `FormationScatterResourcePreparation` 规则，要求本阶段到计划、批次、授权、物品、熟练度、部署、核心与完整根组的 11 组证据；
- JSON 解析 PASS，共 268 条规则；
- 正反自测 `490/490` PASS；
- 覆盖门 `REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=11 Logs=11`；
- 初次以 Windows PowerShell 5.1 调用时，现有 PowerShell 7 管道换行在解析期不受支持；改用 `pwsh` 后全数通过；
- `git diff --cached --check` PASS。

## 9. 构建、静态边界与 P/F

Editor：

- 初次编译新增生产与测试文件：5 actions，`Result: Succeeded`，原生退出 0，总执行 13.60 秒；
- 完整回归后最终复核：target up to date，`Result: Succeeded`，原生退出 0，UBT 1.40 秒；
- `UnrealEditor-demo_map.dll`：19,869,696 bytes，SHA-256 `40B293C9654D3E95C6ACE7290F21E7E32741E2FA447252C2B29C44C9189B58AD`。

Game：

- 4 actions；`Result: Succeeded`；原生退出 0；UBT 17.96 秒；
- `demo_map.exe`：360,469,504 bytes，SHA-256 `F7CBC9116E7AE37492BFD78171127870EE76BBD7005792831DA1DD65F0F58A6B`。

新增生产代码 781 行非空行，测试 686 行非空行。生产文件未发现 World、Actor、ApplyDamage、角色、GameMode、输入、计时器、异步、随机 GUID/RNG、Profile、SaveGame 或直接 InventorySubsystem 依赖。

P 阶段完成编译、真实持久 Prepare、幂等重放、一次性写盘故障、逆序取消、恢复续跑、陈旧计划命令前失败、数量不消耗和改动驱动覆盖证明。F 阶段未启动 UI/PIE/Standalone/产品 exe，也未执行成功路径 Commit、阵眼提交、World 交付、投材飞行、真实输入或视觉验证。

## 10. 精确提交清单

1. `Source/demo_map/demo_mapShanmenFormationScatterResourcePreparation.h`
2. `Source/demo_map/demo_mapShanmenFormationScatterResourcePreparation.cpp`
3. `Source/demo_map/demo_mapShanmenFormationScatterResourcePreparationTests.cpp`
4. `Scripts/ShanmenRegressionMap.json`
5. `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`
6. `Docs/Report/Dev.D.UE.0.0.10.P27.19.r0_report.md`
7. `Docs/Log/Dev.D.UE.0.0.10.P27.19.r0_log.md`

`Saved/FoundationRuns/Dev.D.UE.0.0.10.P27.19.r0` 与本地构建证据不进入 Git；103 个既有未跟踪用户文件保持未暂存。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p27-19-formation-scatter-resource-preparation>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-19-formation-scatter-resource-preparation/Docs/Report/Dev.D.UE.0.0.10.P27.19.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-19-formation-scatter-resource-preparation/Docs/Log/Dev.D.UE.0.0.10.P27.19.r0_log.md>
