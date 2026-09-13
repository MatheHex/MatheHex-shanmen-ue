# Dev.D.UE.0.0.10.P27.19.r0 Report

## 1. 结论

P27.19 已在 P27.18 的不可变整批资源计划之上，建立 `Fdemo_mapShanmenFormationScatterResourcePreparation`：它把计划中每个物理堆叠的一条 Quantity Intent 按规范顺序写入唯一 ShanmenItems 持久权威，并在全部 Prepare 成功后返回完整的待提交证据。

本阶段不是把多文件写入伪装成不可失败的数据库事务，而是提供一个可证明的有界协议：任一 Prepare 未被持久接受时，只执行一次逆序取消；若取消结果无法由最终快照证明，则返回明确恢复状态并保留精确待处理证据。每次重入都会先从权威快照重建本计划已有的 Prepare/Commit/Cancel 回执，因此进程内临时状态不是恢复依据。

专项 5/5、P27.18 计划 4/4、P27.17 批次 4/4、P27.16 授权 4/4、P27.15 权威适配器 4/4、材料适配器 4/4、Items 77/77、熟练度 2/2、部署 4/4、CombatCore 9/9、完整 `Shanmen.0_0_10` 1,378/1,378、映射自测 490/490、改动驱动覆盖门和 Editor/Game 两目标构建全部通过。

## 2. 阶段问题与范围

P27.18 已经能把一次整部署挥洒批次规划成“每个物理堆叠最多一条 Prepare 请求”，但只证明请求可以被仓储接受，并未调用持久权威。若执行层逐条调用而不保存精确回执，中途写盘失败会留下未知数量的待处理意图；若重试只依赖内存游标，又可能重复 Prepare、错误 Commit，或遗失应取消的早期行。

本轮只补齐资源计划到持久 Prepare 集合的接缝：

- 产品入口只适配现有 `Udemo_mapShanmenItemAuthoritySubsystem`，不创建第二份库存权威；
- 每次操作必须位于 Game Thread，且物品权威必须处于 Ready；
- 先捕获快照并按精确 Plan/Request/Intent/Item 身份重建已有证据，再决定是否发命令；
- 全新计划必须仍同时匹配熟练度、部署、批次、Run 关联和当前物品快照；
- 已部分 Prepare 的计划可按冻结身份恢复，不因无关修订变化丢失清理能力；
- 本轮只 Prepare 或 Cancel，不执行 Commit，不修改阵眼或 World。

## 3. 单一权威与有界执行

`Idemo_mapShanmenFormationScatterResourceAuthority` 是算法需要的最窄串行权威边界，只暴露 Ready、快照、持久 Prepare 和持久 Finalize。产品适配器把它直接映射到 GameInstance 中唯一的物品权威：

1. `TryCaptureSnapshot()`；
2. `PreparePreparedRunQuantityIntentDurable()`；
3. `FinalizePreparedRunQuantityIntentDurable()`。

测试通过同一窄接口注入真实 `FShanmenItemAuthorityService`，因此故障注入发生在真实持久仓储写入边界，而不是替换结算规则的模拟容器。

执行严格按 P27.18 `Reservations` 的规范顺序前进。每个物理堆叠只提交计划中冻结的原始 Prepare 请求；成功回执必须逐字段匹配 Request、Intent、Item、Run、Purpose、数量、冻结前值和可用后值。全部命令报告成功后还必须重新捕获快照，并证明每条计划行都存在唯一、精确且仍 Pending 的 Prepare，才允许返回 `Prepared` 或 `Replayed`。

## 4. 持久证据重建与幂等

`CollectEvidence()` 在任何新命令前扫描 `ProcessedRequests`，为计划中的每条 Reservation 重建一格 Prepare 回执和一格终态回执。它拒绝：

- 同一计划 Intent 出现在另一个 Prepare Request 下；
- 重复 Prepare、重复终态或身份被不兼容回执占用；
- 回执的操作、阶段、Item、数量、前后值、Purpose 或 Run 证据与冻结计划不一致；
- 同一计划同时混有 Commit 与 Cancel 终态。

相同计划重放时，权威返回 `Replayed`，最终快照与首次 Prepare 后完全一致，不产生第二份意图。Prepare 只降低可用量，不改变任何物理堆叠的实际 Quantity。

未来 Commit 与本轮 Cancel 通过 `demo_map.Formation.ScatterResourceFinalize.r1` 派生相同的逐行终态 Request ID；`bCommit` 不参与该身份。这样同一 Prepare 的首个持久终态决定不可被后续相反请求覆盖，执行协调器必须依据已存在终态向前恢复，而不能事后翻转决定。

## 5. 回滚、终态与恢复状态

任一 Prepare 失败后，算法只进行一次有界逆序取消，从计划末端向前取消所有已被精确证明为 Pending 的 Prepare。取消完成后再次读取持久快照；只有所有已 Prepare 行都具有精确 Cancel 回执、没有 Pending 或 Commit，才返回已取消结果。

主要状态语义：

- `Prepared`：本轮至少写入一个新 Prepare，最终整批均已持久 Prepare；
- `Replayed`：整批 Prepare 全部从既有精确证据重放；
- `PlanStale`：未触碰的计划已过期，在第一条命令前失败；
- `PlanStaleRolledBack`：计划在部分 Prepare 后失去当前性，已完整取消；
- `PrepareRejectedRolledBack`：某行未被持久接受，早期行已完整取消；
- `AttemptCancelled`：发现既有 Cancel，取消其余 Pending 后把整次尝试固定为取消态；
- `RollbackRecoveryRequired`：一次取消 pass 后仍无法证明完整清理；
- `PreparationRecoveryRequired`：命令报告成功但最终持久证据不完整；
- `ForwardRecoveryRequired`：发现 Commit，禁止再向后回滚。

任何恢复状态都保留计划和已重建回执，不把未知结果冒充成功。下一次调用会重新读取权威，并从精确持久状态继续。

## 6. 自动化证明

| Group | Success | Fail | Log SHA-256 |
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

11 份日志均具有唯一 UE 5.8 原生完成标记、原生退出 0、0 Fail 与 0 Fatal/Unhandled/Ensure。完整根组进程连续执行 3,733.721 秒；测试从 P27.18 的 1,373 项增加本轮 5 项，最终为 1,378 项。

五条专项分别证明：

1. `DurableWholeBatchAndReplay`：3 个物理堆叠全部持久 Prepare，数量不消耗，相同计划完整幂等重放；
2. `MiddleFailureRollsBackEarlierLines`：第二行写盘失败后逆序取消第一行，最终无 Pending 且数量不变；
3. `RollbackRecoveryAndExactResume`：Prepare 与 Cancel 分别注入一次写盘失败，先返回精确恢复态，下一 pass 重建并完成整批 Prepare；
4. `StalePlanFailsBeforeMutation`：外部权威变化使全新计划过期，第一条命令前失败且快照不变；
5. `PartialCancellationTerminatesAttempt`：任一既有 Cancel 把整个计划固定为取消态，取消余项后再次重放不发命令。

## 7. 改动驱动回归

新增 `FormationScatterResourcePreparation` 映射规则。三个新文件必须同时具有完整根组、P27.19 专项、P27.18 计划、P27.17 批次、P27.16 授权、P27.15 权威适配器、材料适配器、Items、熟练度、部署和 CombatCore 共 11 组证据。

- regression map JSON：PASS，共 268 条规则；
- 映射器正反自测：`490/490` PASS；
- 负向自测证明仅有 P27.19 专项不能替代资源计划、物品、授权、部署、核心和完整根组；
- 覆盖门：`REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=11 Logs=11`；
- `git diff --cached --check`：PASS。

自检第一次误用 Windows PowerShell 5.1，因项目覆盖脚本现有的 PowerShell 7 管道换行语法而在解析期失败；改用项目所需 `pwsh` 后 490/490 通过。该运行器选择错误没有执行覆盖判断，也不涉及产品代码或自动化结果。

## 8. 构建与静态边界

- Editor 初次编译新生产与测试文件：5 actions，`Result: Succeeded`，原生退出 0，总执行 13.60 秒；
- 完整回归后 Editor 最终复核：target up to date，`Result: Succeeded`，原生退出 0，UBT 1.40 秒；
- `UnrealEditor-demo_map.dll`：19,869,696 bytes，SHA-256 `40B293C9654D3E95C6ACE7290F21E7E32741E2FA447252C2B29C44C9189B58AD`；
- Game：4 actions，`Result: Succeeded`，原生退出 0，UBT 17.96 秒；
- `demo_map.exe`：360,469,504 bytes，SHA-256 `F7CBC9116E7AE37492BFD78171127870EE76BBD7005792831DA1DD65F0F58A6B`。

新增生产代码 781 行非空行，测试 686 行非空行。生产文件静态扫描未发现 `UWorld`、`AActor`、`ApplyDamage`、计时器、异步、随机 GUID/RNG、Profile、SaveGame、角色/GameMode 或直接 InventorySubsystem 依赖；持久写入只经过现有物品权威公开接口。

## 9. P/F 边界与下一阶段

P 阶段已证明：当前整批计划可以持久 Prepare；相同输入不会重复创建意图；中途写入失败会触发一次有界逆序取消；无法证明清理时会显式进入恢复态；下一次调用可从持久证据恢复；Prepare 不消耗物理数量；任一既有取消会终止整个尝试；既有 Commit 会强制进入只向前恢复。

F 阶段未执行：没有启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、Cook 或 Package。没有执行成功路径的 Quantity Commit，没有提交阵眼，没有生成投材飞行、阵法生效或视觉反馈，因此不声明玩家可见的挥洒布阵已经完成。

下一独立阶段建议建立“资源终态与阵眼提交协调器”：只能在整批 Prepare 证据完整时决定 Commit；逐行 Commit 后必须记录精确终态，并把每个资源 Slice 映射回对应阵眼的可提交证据。任何已出现 Commit 的恢复都只能向前完成，不能取消已提交数量；World 交付仍应留在之后的独立阶段。

## 10. GitHub 交接

基线提交：`0ea6f204115dce4fbd10810dbe65cbf4a4fba522`（P27.18）。分支：`agent/0.0.10-p27-19-formation-scatter-resource-preparation`。本阶段只提交 3 个新增源/测试文件、2 个回归映射文件、本 Report 与本 Development Log；103 个既有未跟踪用户文件保持未暂存，`Saved/FoundationRuns/Dev.D.UE.0.0.10.P27.19.r0` 与本地构建证据不进入 Git。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p27-19-formation-scatter-resource-preparation>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-19-formation-scatter-resource-preparation/Docs/Report/Dev.D.UE.0.0.10.P27.19.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-19-formation-scatter-resource-preparation/Docs/Log/Dev.D.UE.0.0.10.P27.19.r0_log.md>
