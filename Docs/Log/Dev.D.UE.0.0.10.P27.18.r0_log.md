# Dev.D.UE.0.0.10.P27.18.r0 Development Log

## 1. 目标

- 把 P27.17 当前整部署挥洒批次转换为一份完整、确定性、无副作用的全批次资源计划；
- 以同一物品权威快照累计分配所有阵眼需求，阻止逐阵眼检查造成合计超额；
- 同一物理堆叠可服务多条材料需求，但只产生一条 ShanmenItems Prepare 请求；
- 保留每个阵眼、作者需求、物理堆叠和数量切片的精确归因；
- 对错误 Run、内容、生命周期、权威修订、数量不足和既有待处理意图失败关闭；
- 区分历史计划内部有效性与当前权威可执行性；
- 完成改动驱动回归、双目标构建、Report/Log 与 GitHub 推送。

## 2. 基线与范围

- 基线：`7920f7e090e1f32a269c5e994f8aeb66771ffe2c`（P27.17）；
- 分支：`agent/0.0.10-p27-18-formation-scatter-resource-plan`；
- 起始 tracked tree clean；103 个既有未跟踪用户文件保持未暂存；
- 输入只使用 P27.17 批次、当前熟练度/部署、`Fdemo_mapShanmenRunCorrelation` 和 `FShanmenItemAuthoritySnapshot`；
- 不修改阵图、部署、熟练度规则、Profile/schema、角色、输入、UI、地图或内容资产；
- 不启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、Smoke、Cook 或 Package。

## 3. 新增文件与类型

新增：

- `Source/demo_map/demo_mapShanmenFormationScatterResourcePlan.h`；
- `Source/demo_map/demo_mapShanmenFormationScatterResourcePlan.cpp`；
- `Source/demo_map/demo_mapShanmenFormationScatterResourcePlanTests.cpp`。

主要类型：

1. `Fdemo_mapShanmenFormationScatterResourceSlice`：一条材料需求在一个物理堆叠上的数量切片；
2. `Fdemo_mapShanmenFormationScatterResourceReservationIntent`：每个物理堆叠唯一的聚合意图和 Prepare 请求；
3. `Fdemo_mapShanmenFormationScatterResourcePlan`：绑定批次、Run、内容、修订、全部 Slice 与聚合意图的完整计划；
4. `Edemo_mapShanmenFormationScatterResourcePlanStatus`：失败关闭状态；
5. `Fdemo_mapShanmenFormationScatterResourcePlanResult`：状态、诊断与计划结果；
6. `Fdemo_mapShanmenFormationScatterResourcePlanner`：无状态 `Plan()` 与 `IsCurrentPlan()` 入口。

## 4. 权威读取与规划顺序

`Plan()` 按下列顺序验证并规划：

1. P27.17 批次必须有效；
2. 熟练度投影与部署必须有效，当前证据必须能重建相同批次；
3. Run 关联必须有效，部署 Action 的 Run/Owner 必须与物品 Active Run/Owner 一致；
4. 快照内容身份必须与阵法 Action 一致；
5. 临时 `FShanmenItemRepository` 必须能无副作用装载快照并通过全部仓储不变量；
6. 快照修订不得早于 Run 生命周期，生命周期回执必须精确匹配且 Run 未终结；
7. 关联中的每个物理实例必须唯一、属于同一 Owner/Scope、支持 Quantity，并具有精确已提交 Run 权威；
8. 从冻结权威量扣除直接消费和已完成 Quantity Intent；仍有未完成 Intent 的堆叠标记为冲突；
9. 依次按阵眼、需求和库存规范顺序累计 first-fit；任一需求不足即丢弃全部临时结果；
10. 将同一物理堆叠的全部 Slice 聚合为一条 Prepare 请求，并执行整计划自校验。

## 5. 确定性、守恒与当前性

Planning Scope 由批次 ID、Run 关联、Owner、Scope、Active Run、生命周期回执、权威修订和内容身份派生。Slice ID 进一步包含目的阵眼、作者材料意图、物理堆叠、顺序及前后数量；聚合意图和 Prepare Request ID 覆盖该堆叠的全部有序 Slice；最终 Plan ID 覆盖所有下层身份和汇总值。

`IsValid()` 逐层重算全部身份，并检查：

- 每个作者材料意图的 Slice 数量和精确等于原需求；
- Slice 全局顺序连续，逐堆叠数量链前后相接；
- 每条 Slice 恰好被一条聚合意图覆盖；
- 物理堆叠、意图和 Request ID 均唯一；
- 聚合数量和、计划总量与 P27.17 批次总量完全守恒；
- Prepare 请求的全部语义字段与聚合意图一致。

`IsCurrentPlan()` 使用当前证据重新调用同一个纯规划器并比较完整计划。权威修订或内容发生变化后，旧计划仍可作为内部一致的历史证据，但不能继续作为当前命令来源。

## 6. 测试夹具与自查修正

主夹具构造两个阵眼和三条需求：East/Wood 2、East/Stone 1、North/Wood 3；Run 库存为 WoodA 4、Stone 1、WoodB 2。最终生成 4 条 Slice 和 3 条逐堆叠聚合意图，WoodA 的一条数量 4 命令覆盖 East 与 North 两个目的 Slice。

测试还把每条生成的 `FShanmenItemRunQuantityIntentRequest` 提交给由同一快照恢复的真实 `FShanmenItemRepository`，证明请求不仅内部有效，而且被现有 ShanmenItems 契约接受。规划前后对输入快照进行比较，证明规划器没有修改外部权威。

回归映射自测初稿曾在新增正向 fixture 中引用尚未定义的 `$Items` 变量。复核映射器的 broad-suite 语义后，移除该无效引用并使用已存在的完整根组证据；最终自测 488/488 通过。该修正只影响测试脚本初稿，不影响产品实现或最终自动化日志。

## 7. 专项自动化

新增四条测试：

1. `DeterministicCumulativeAllocation`：相同输入重放得到相同 Planning Scope、Slice、Request 与 Plan ID，并核对累计分配明细；
2. `StackAggregationAndAttribution`：同一 WoodA 跨两个阵眼仍只生成一条请求，同时保存两条目的 Slice；
3. `QuantityAndPendingFences`：总量不足和同材料堆叠存在未完成 Quantity Intent 时返回精确失败状态与空计划；
4. `CurrentnessAndNoMutation`：证明输入不变、请求可被真实仓储接受，以及后续权威变化使旧计划失去当前性。

## 8. 自动化与回归结果

| Group | Success | Fail | SHA-256 |
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

十份日志均为原生退出 0、唯一队列清空标记、0 Fail、0 Fatal/Unhandled/Ensure。完整根组连续执行 5,017.938 秒，测试总数从 P27.17 的 1,369 增至 1,373。

回归映射：

- 新增 `FormationScatterResourcePlan` 规则，要求资源计划、批次、授权、权威适配器、材料适配器、Items、熟练度、部署、CombatCore 与完整根组；
- JSON 解析 PASS，共 267 条规则；
- 正反自测 `488/488` PASS；
- 覆盖门 `REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=10 Logs=10`；
- `git diff --check` PASS。

## 9. 构建、静态边界与 P/F

Editor：

- 初次编译 `Result: Succeeded`，原生退出 0，总执行 22.92 秒；
- 完整回归后最终复核为 target up to date，原生退出 0，总执行 2.58 秒；
- `UnrealEditor-demo_map.dll`：19,814,400 bytes，SHA-256 `011E915BB74213C91E83062222E34F043D31117AA02FC43E04326D551E6037CB`。

Game：

- 4 actions；`Result: Succeeded`；原生退出 0；总执行 42.51 秒；
- `demo_map.exe`：360,420,352 bytes，SHA-256 `3830E372C93229285E96F11DE4594126718636F23B6E8337800662A0CB47AB55`。

新增生产代码 1,128 行非空行，测试 495 行非空行。生产文件没有 World、Actor、控制器、输入、计时器、异步、随机、Profile、SaveGame 或直接 InventorySubsystem 依赖。

P 阶段完成编译、无头自动化、累计分配、确定性、守恒、当前性、无副作用和改动驱动覆盖证明。F 阶段未启动 UI/PIE/Standalone/产品 exe，也未执行真实 Prepare、Commit、Cancel、阵眼提交、投材飞行或视觉表现。

## 10. 精确提交清单

1. `Source/demo_map/demo_mapShanmenFormationScatterResourcePlan.h`
2. `Source/demo_map/demo_mapShanmenFormationScatterResourcePlan.cpp`
3. `Source/demo_map/demo_mapShanmenFormationScatterResourcePlanTests.cpp`
4. `Scripts/ShanmenRegressionMap.json`
5. `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`
6. `Docs/Report/Dev.D.UE.0.0.10.P27.18.r0_report.md`
7. `Docs/Log/Dev.D.UE.0.0.10.P27.18.r0_log.md`

`Saved/FoundationRuns/Dev.D.UE.0.0.10.P27.18.r0` 与本地构建证据不进入 Git；103 个既有未跟踪用户文件保持未暂存。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p27-18-formation-scatter-resource-plan>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-18-formation-scatter-resource-plan/Docs/Report/Dev.D.UE.0.0.10.P27.18.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-18-formation-scatter-resource-plan/Docs/Log/Dev.D.UE.0.0.10.P27.18.r0_log.md>
