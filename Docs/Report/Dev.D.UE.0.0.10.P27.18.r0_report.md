# Dev.D.UE.0.0.10.P27.18.r0 Report

## 1. 结论

P27.18 已在 P27.17 的整部署挥洒批次意图之下，建立纯函数、无副作用、确定性且自校验的全批次资源预检与分配计划 `Fdemo_mapShanmenFormationScatterResourcePlan`。

规划器只读取一份当前 P27.17 批次、当前熟练度/部署证据、一份精确的 ShanmenItems 权威快照和对应 Run 关联。它按“阵眼规范顺序 → 阵眼内材料需求顺序 → Run 库存顺序”进行累计 first-fit 分配，因而不会让多个阵眼分别通过单项检查后合计超额。同一物理堆叠可通过多条 Slice 服务多个阵眼，但最终严格聚合为每个物理堆叠一条 `FShanmenItemRunQuantityIntentRequest`，兼容 ShanmenItems 的单堆叠单待处理意图约束，同时保留逐阵眼归因。

专项 4/4、P27.17 批次 4/4、P27.16 授权 4/4、P27.15 权威适配器 4/4、材料适配器 4/4、Items 77/77、熟练度 2/2、部署 4/4、CombatCore 9/9、完整 `Shanmen.0_0_10` 1,373/1,373、回归映射自测 488/488、改动驱动覆盖门以及 Editor/Game 两目标构建全部通过。

## 2. 阶段问题与范围

P27.17 已把一次 Master `ScatterFormation` 授权展开为稳定、有序、逐阵眼隔离的材料批次，但尚未回答同一份局内库存如何同时满足整个批次。若调用方逐阵眼独立检查数量，同一个 Wood 堆叠可能先后被多个阵眼按原始数量重复计算；若直接逐需求创建事务，同一物理堆叠又会产生多条并存的待处理意图，与 ShanmenItems 的事务约束冲突。

本轮只补齐资源事务之前的纯规划接缝：

- P27.17 批次必须仍由当前熟练度投影和部署证据完整重建；
- 阵法动作与物品 Run 关联必须指向同一 Owner 和 Active Run；
- 权威快照必须通过仓储不变量、内容身份、修订与生命周期校验；
- 只允许使用关联中规范有序的局内物理堆叠及其已提交 Quantity 权威；
- 已消费或已完成的数量必须从可用量扣除；
- 尚有未完成 Quantity 意图的堆叠不得被第二条计划命令复用；
- 整批需求必须一次性全部可分配，否则失败结果不携带部分计划。

本轮不调用真实 Reserve、Prepare、Commit、Cancel 或 Finalize，不修改物品权威或阵眼，不产生投材飞行与视觉表现，也不接 World、输入、时序、持久化或重试执行。

## 3. 累计分配与单堆叠聚合

规划器先从 `Correlation.OrderedRunInventoryItemInstanceIds` 建立稳定库存余额。每个余额绑定一个精确物理实例、材料定义、Run 内规范顺序和冻结 Quantity 权威；已发生的直接消费与已提交 Quantity 意图会从冻结数量中扣除。

分配顺序完全由冻结证据决定：先遍历 P27.17 批次中的阵眼，再遍历阵眼内作者材料需求，最后按 Run 库存顺序消耗匹配材料。每次消耗形成一条 `ResourceSlice`，记录目的阵眼、原始材料意图、物理堆叠、分配前后数量和全局 Slice 顺序。调用方输入顺序、容器散列顺序或运行时随机性均不参与结果。

专项夹具包含 East 与 North 两个阵眼：East 需要 Wood 2、Stone 1，North 需要 Wood 3；Run 库存依次包含 WoodA 4、Stone 1、WoodB 2。规范分配为：

1. East/Wood 从 WoodA 4→2；
2. East/Stone 从 Stone 1→0；
3. North/Wood 先从 WoodA 2→0，再从 WoodB 2→1。

WoodA 的两条 Slice 分别归属 East 与 North，但聚合成一条数量 4 的 Prepare 请求；Stone 与 WoodB 各形成一条请求。这样既保持完整目的归因，也不会为同一物理堆叠创建相互冲突的并行意图。

## 4. 结构与确定性身份

新增三层不可变证据：

1. `Fdemo_mapShanmenFormationScatterResourceSlice`：一条作者材料需求从一个物理堆叠取得的精确数量切片；
2. `Fdemo_mapShanmenFormationScatterResourceReservationIntent`：按物理堆叠聚合的权威兼容 Prepare 命令，内部保留全部有序 Slice ID；
3. `Fdemo_mapShanmenFormationScatterResourcePlan`：整批 Slice、逐堆叠命令、Run/内容/修订证据和总量守恒的完整计划。

身份命名空间分别为：

- `demo_map.Formation.ScatterResourcePlanningScope.r1`；
- `demo_map.Formation.ScatterResourceSlice.r1`；
- `demo_map.Formation.ScatterResourceReservationIntent.r1`；
- `demo_map.Formation.ScatterResourcePrepare.r1`；
- `demo_map.Formation.ScatterResourcePlan.r1`。

Planning Scope 覆盖批次、Run 关联、Owner、Scope、Active Run、生命周期回执、权威修订和内容版本/摘要。Slice、聚合意图、Prepare Request 与最终 Plan 都由完整语义字段和有序下层 ID 派生；各层 `IsValid()` 都会重算身份，而不是只检查 GUID 非空。

## 5. 守恒、当前性与失败关闭

计划自校验要求：

- 每条 Slice 的前后数量严格满足 `Before - Quantity == After`；
- 每条作者材料意图收到的 Slice 数量和必须精确等于原需求；
- 每条 Slice 恰好归属于一条逐堆叠聚合意图；
- 同一物理堆叠只出现一次，Slice 链的前后数量必须连续；
- 聚合数量必须等于其 Slice 数量和，所有聚合数量必须等于批次材料总量；
- Prepare Request 的 Run、Owner、内容、物理堆叠、数量、期望前值、Purpose 和确定性 Request ID 必须与聚合意图完全一致。

规划状态覆盖：`BatchInvalid`、`MasteryProjectionRejected`、`DeploymentInvalid`、`BatchStale`、`RunCorrelationInvalid`、`RunMismatch`、`SnapshotInvalid`、`ContentMismatch`、`SnapshotStale`、`LifecycleInvalid`、`ItemAuthorityInvalid`、`ConflictingIntent`、`QuantityUnavailable`、`PlanInvalid` 与 `Planned`。所有失败均返回空的无效计划，禁止部分成功泄漏给执行层。

`IsCurrentPlan()` 会以计划冻结的批次和当前全部证据重新规划并比较完整结果。专项测试证明规划前后快照完全相同；权威发生一次合法变更后，历史计划仍可通过内部 `IsValid()` 供审计，但不再通过当前性检查。

## 6. 自动化证明

| Group | Success | Fail | Log SHA-256 |
|---|---:|---:|---|
| `Shanmen.0_0_10.Product.FormationScatterResourcePlan` | 4 | 0 | `344F53871A9C857D87C4B5A70FE9227036132887EBE1D08D4241928886A7F9E8` |
| `Shanmen.0_0_10.Product.FormationScatterBatchIntent` | 4 | 0 | `57595C8DBFD66920A1A048889B465DD865D6392BFF38FC07F35F1193B48926A7` |
| `Shanmen.0_0_10.Product.FormationMasteryOperationAuthorization` | 4 | 0 | `140C15B60080D6F7D3A658DAA76ADE5168F72FF23FFB9828B2C11C64F83A57F8` |
| `Shanmen.0_0_10.Product.FormationMasteryAuthorityAdapter` | 4 | 0 | `068FA7BFFF7E03B8FC0E06EF7869405EA191ABDCB4F98C2D689696228BAA6839` |
| `Shanmen.0_0_10.Product.FormationMaterialAdapter` | 4 | 0 | `8989B953533B2F15C65431DE5362195A86CE8B87E900D7C4F1DD33CCDFB8084F` |
| `Shanmen.0_0_10.Items` | 77 | 0 | `1EE471B9F495944428D954E5C4410ED782F74B2B7BA025CF7926A964BE478F79` |
| `Shanmen.0_0_10.CombatRuntime.FormationMastery` | 2 | 0 | `6A829F8744EC530F22D44E41F7FCC131C1FB42E570C4140AC6E2D270322BB22A` |
| `Shanmen.0_0_10.CombatRuntime.FormationDeployment` | 4 | 0 | `ABDF4E956C7CE3AB1BFE82F50EC1608F83716FB4684EDC777A835C2CD039BE57` |
| `Shanmen.0_0_10.CombatCore` | 9 | 0 | `BBD4DE932963D3D1F5B50437FBD31ABAC5F405F65C6B7EE5EFDF09614035B0E1` |
| `Shanmen.0_0_10` | 1,373 | 0 | `83BC8749D46B4D4B48679881159A38B1AFF000C94DF0B3746BAC7831DEE55B2E` |

十份日志均具有唯一 `Automation Test Queue Empty N tests performed`、原生 `RequestExitWithStatus(..., 0, ...)`、0 Fail 与 0 Fatal/Unhandled/Ensure。完整根组从 05:03:06.653 UTC 连续运行至 06:26:44.591 UTC，共 5,017.938 秒；相比 P27.17 的 1,369 项只增加本轮 4 条专项，最终为 1,373 项。

四条专项分别为：

1. `DeterministicCumulativeAllocation`：证明规范顺序、累计 first-fit、整批数量和确定性重放；
2. `StackAggregationAndAttribution`：证明单栈多阵眼 Slice 归因与每栈唯一 Prepare 请求；
3. `QuantityAndPendingFences`：证明总量不足与既有待处理意图失败关闭且不泄漏部分计划；
4. `CurrentnessAndNoMutation`：证明请求可被真实 ShanmenItems 仓储接受、规划无副作用以及权威变化后的当前性失效。

## 7. 改动驱动回归

新增 `FormationScatterResourcePlan` 映射规则。三个新资源计划文件必须同时具有完整根组、资源计划专项、P27.17 批次、P27.16 操作授权、P27.15 权威适配器、材料适配器、Items、熟练度、部署和 CombatCore 十组证据。

- regression map JSON：PASS，共 267 条规则；
- 映射器正反自测：`488/488` PASS；
- 负向自测证明仅有资源计划专项不能替代批次、物品、授权、部署、核心与完整组；
- 最终覆盖门：`REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=10 Logs=10`；
- `git diff --check`：PASS。

映射自测编写期间曾引用尚未定义的 `$Items` fixture；该脚本问题在最终证据前被发现并改为使用已有完整组的 broad-coverage 语义。产品代码和自动化结果不受影响，最终 488 条正反用例来自修正后的脚本。

## 8. 构建与静态边界

- Editor 初次编译新代码：`Result: Succeeded`，原生退出 0，总执行 22.92 秒；完整回归后最终复核为 target up to date，原生退出 0，总执行 2.58 秒；
- `UnrealEditor-demo_map.dll`：19,814,400 bytes，SHA-256 `011E915BB74213C91E83062222E34F043D31117AA02FC43E04326D551E6037CB`；
- Game：4 actions，`Result: Succeeded`，原生退出 0，总执行 42.51 秒；
- `demo_map.exe`：360,420,352 bytes，SHA-256 `3830E372C93229285E96F11DE4594126718636F23B6E8337800662A0CB47AB55`。

新增生产代码 1,128 行非空行，测试 495 行非空行。新生产代码静态扫描未发现 `UWorld`、`AActor`、`ApplyDamage`、计时器、异步、随机 GUID/RNG、Profile、SaveGame 或直接 InventorySubsystem 依赖；它只通过 ShanmenItems 的公开快照、仓储验证与 Quantity Intent 契约工作。

## 9. P/F 边界与下一阶段

P 阶段已证明：一份当前整部署挥洒批次可以针对同一精确物品权威快照完成全批次累计分配；多阵眼不会重复花费同一数量；同一物理堆叠只产生一条权威兼容 Prepare 请求；数量不足、待处理冲突、错误 Run/内容/生命周期/修订均失败关闭；历史计划与当前计划可以明确区分。

F 阶段未执行：没有启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、Cook 或 Package。没有真实提交任何 Prepare 请求，没有 Reserve、Commit、Cancel、Finalize、阵眼提交、补偿、投材飞行或视觉反馈，因此不声明玩家可见功能已经完成。

下一独立阶段可建立“资源计划执行会话”：按计划提交逐堆叠 Prepare 请求，记录每条结果，并在中途失败、未知结果或快照过期时提供有界复查与补偿/取消契约。该执行层仍应与阵眼提交和 World 表现分离，并必须先明确跨多堆叠的原子性边界。

## 10. GitHub 交接

基线提交：`7920f7e090e1f32a269c5e994f8aeb66771ffe2c`（P27.17）。分支：`agent/0.0.10-p27-18-formation-scatter-resource-plan`。本阶段只提交 3 个新增源/测试文件、2 个回归映射文件、本 Report 与本 Development Log；103 个既有未跟踪用户文件保持未暂存，`Saved/FoundationRuns/Dev.D.UE.0.0.10.P27.18.r0` 与本地构建证据不进入 Git。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p27-18-formation-scatter-resource-plan>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-18-formation-scatter-resource-plan/Docs/Report/Dev.D.UE.0.0.10.P27.18.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-18-formation-scatter-resource-plan/Docs/Log/Dev.D.UE.0.0.10.P27.18.r0_log.md>
