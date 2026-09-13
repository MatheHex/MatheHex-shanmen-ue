# Dev.D.UE.0.0.10.P27.17.r0 Development Log

## 1. 目标

- 把 P27.16 当前整部署挥洒授权展开为一份完整、稳定、有序的批次命令意图；
- 为每个阵眼和每条材料需求保留精确归属、顺序、数量及确定性身份；
- 禁止跨阵眼聚合同名材料，避免资源事务失去目的阵眼；
- 让熟练度或部署变化自动使旧批次失去当前性，同时保留历史审计有效性；
- 加固 P27.16 授权自验证中的档位—投材方式组合约束；
- 保持规划纯函数化，不操作库存、阵眼、World、输入或表现；
- 完成改动驱动回归、双目标构建、Report/Log 与 GitHub 推送。

## 2. 基线与范围

- 基线：`a79a005777a401c324c37d752f3cba45b2d3f7e1`（P27.16）；
- 分支：`agent/0.0.10-p27-17-formation-scatter-batch-intent`；
- 起始 tracked tree clean；103 个既有未跟踪用户文件保持未暂存；
- 输入只使用 P27.15 熟练度投影、P27.16 操作授权和既有 `FShanmenFormationDeployment`；
- 不修改阵法部署、材料需求、Profile/schema、库存/物品权威、角色、输入、UI、地图、内容资产或熟练度规则；
- 不启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、Smoke、Cook 或 Package。

## 3. 新增文件与类型

新增：

- `Source/demo_map/demo_mapShanmenFormationScatterBatchIntent.h`；
- `Source/demo_map/demo_mapShanmenFormationScatterBatchIntent.cpp`；
- `Source/demo_map/demo_mapShanmenFormationScatterBatchIntentTests.cpp`。

主要类型：

1. `Fdemo_mapShanmenFormationScatterMaterialIntent`：一条作者材料需求到一个部署阵眼的精确路由；
2. `Fdemo_mapShanmenFormationScatterAnchorIntent`：一个阵眼的有序材料子命令；
3. `Fdemo_mapShanmenFormationScatterBatchIntent`：一份当前挥洒授权的完整整部署展开；
4. `Edemo_mapShanmenFormationScatterBatchPlanStatus`：失败关闭状态；
5. `Fdemo_mapShanmenFormationScatterBatchPlanResult`：状态、诊断和批次结果；
6. `Fdemo_mapShanmenFormationScatterBatchPlanner`：无状态 `Plan()` 与 `IsCurrentBatch()` 入口。

## 4. 规划顺序与失败状态

`Plan()` 顺序验证：

1. P27.16 授权必须有效；
2. 方式必须是 `ScatterFormation`，目标必须是整个 Deployment；
3. P27.15 熟练度投影必须当前有效；
4. 部署必须有效；
5. P27.16 授权器必须能以当前证据重建完全相同授权；
6. 阵图定义、部署进度与授权阵眼数必须一致；
7. 每个阵眼定义必须按规范索引排列，部署阵眼不得已提交，定义/实例/位置必须有效；
8. 每条需求必须有效且 `RequirementOrder` 与规范索引一致；
9. 材料、阵眼与批次意图必须逐层通过确定性自校验。

拒绝状态包括：`AuthorizationInvalid`、`AuthorizationNotScatter`、`MasteryProjectionRejected`、`DeploymentInvalid`、`AuthorizationStale`、`DeploymentShapeInvalid`、`RequirementInvalid` 与 `IntentInvalid`。成功状态只有 `Planned`；所有失败结果都不携带有效批次。

## 5. 身份、守恒与自查修正

材料、阵眼与批次分别使用 `ScatterMaterialIntent.r1`、`ScatterAnchorIntent.r1` 和 `ScatterBatchIntent.r1` 命名空间。每层 ID 都由全部语义字段与下层有序 ID 派生，并由 `IsValid()` 重算。

阵眼验证要求材料需求顺序连续、意图 ID 唯一、材料定义在同一阵眼内唯一，并核对数量和。批次验证要求阵眼顺序连续、意图/定义/实例唯一，核对需求总数、材料总量以及与授权总阵眼数一致。世界坐标必须有限，double 位模式参与身份，正负零统一规范化。

专项夹具的作者输入刻意反向，捕获后按 East、North 规范顺序输出。East 的 Wood 2 与 North 的 Wood 3 保持两条不同意图；批次只汇总数量，不合并需求，最终为 2 阵眼、3 需求、总量 6。

实现过程中复核 P27.16 的不可变授权，发现其自验证只分别检查档位与方式枚举有效，无法拒绝“合法档位 + 合法但该档不可用方式”的字段篡改。本轮把 `BuildAuthorizationId()` 改为重建 `FShanmenFormationMasteryPolicy` 并调用 `CanUseDeliveryMode()`。随后重新执行 Editor 构建、授权专项、全部依赖测试及完整根组，最终证据均来自修正后代码。

## 6. 当前性与无副作用

`IsCurrentBatch()` 不读取缓存，而是以批次冻结授权和当前熟练度/部署重新调用 `Plan()`，再比较完整批次。它区分两种性质：

- `Batch.IsValid()`：历史证据内部仍一致，可用于审计；
- `IsCurrentBatch()`：当前权威和部署仍能生成完全相同批次，才可继续使用。

专项测试在规划前记录部署状态、回执数和已提交阵眼数，证明规划不改变任何一项。随后通过既有部署权威提交 East 阵眼：旧批次仍内部有效，但不再当前；再次以旧授权规划返回 `AuthorizationStale`。规划器自身不调用库存、物品、部署提交或任何 World API。

## 7. 专项自动化

新增四条测试：

1. `DeterministicCanonicalPlan`：相同证据重放为相同批次 ID，并证明反向作者输入按规范阵眼顺序输出；
2. `RequirementIsolation`：证明逐阵眼需求顺序/数量精确，同名材料不跨阵眼合并；
3. `AuthorizationFences`：无效授权、逐阵眼方式、无效投影/部署、修订变化和另一部署失败关闭；
4. `CurrentnessAndNoMutation`：证明规划无副作用，部署提交后历史有效性保留但当前性与旧授权失效。

## 8. 自动化与回归结果

| Group | Success | Fail | SHA-256 |
|---|---:|---:|---|
| `Shanmen.0_0_10.Product.FormationScatterBatchIntent` | 4 | 0 | `DE351900CC465CEDC028516A4CF4231C206C35703D08F60F054C4FCE00663934` |
| `Shanmen.0_0_10.Product.FormationMasteryOperationAuthorization` | 4 | 0 | `04BBEE8E86A63AC2E10479CA07F3FBCA4C178115E7FF03EFD5F63C627ED6664D` |
| `Shanmen.0_0_10.Product.FormationMasteryAuthorityAdapter` | 4 | 0 | `5C9AEDE0E85207E04617234957A15D1B109F90E8CE715E0980FF962C5490F648` |
| `Shanmen.0_0_10.CombatRuntime.FormationMastery` | 2 | 0 | `432D2F51845CD921DD4DF15C02FE1830AC84A9D6BC6966EB6C6A7838120E647F` |
| `Shanmen.0_0_10.CombatRuntime.FormationDeployment` | 4 | 0 | `1C20B769D23B061665C4A4D128D6F74607DACA3F9897756913BF776F109D2815` |
| `Shanmen.0_0_10.CombatCore` | 9 | 0 | `C01F30B14A6FA35A94166F39160363B5CB85C463F27FCE47D3E0318EC90B22B2` |
| `Shanmen.0_0_10` | 1,369 | 0 | `F6E4D1B9FCCC18DE9EBB796242650D6034E91E682A7892C8753AA52E82748E57` |

七份最终日志均为原生退出 0、唯一 `TEST COMPLETE. EXIT CODE: 0`、0 Fail、0 Fatal/Unhandled/Ensure。完整根组执行 4,841.818 秒，测试总数从 P27.16 的 1,365 增至 1,369。

回归映射：

- 新增 `FormationScatterBatchIntent` 规则，要求批次、授权、权威适配器、熟练度策略、部署、CombatCore 与完整根组；
- JSON 解析 PASS，共 266 条规则；
- 正反自测 `486/486` PASS；
- 覆盖门 `REGRESSION_COVERAGE: PASS Changed=6 Rules=2 Required=7 Logs=7`；
- `git diff --check` PASS。

## 9. 构建、静态边界与 P/F

Editor：

- 6 actions；`SUCCEEDED`；原生退出 0；UBT 总执行 42.27 秒；
- run-state SHA-256：`D0F07D926E40E1840E240B05F3B66EA2E6B8357E9D50D9460BE46213C48C41CB`；
- stdout SHA-256：`241CA24AE7C74901B4B1C46A6E008B64D3E6868CBE0DE6293FA2B4208E8835E2`；
- `UnrealEditor-demo_map.dll`：19,749,376 bytes；SHA-256 `5D30FEC612B56EF6207277069B8FA18DD3307887E8AD7C3FAB83BDBC03B1B207`。

Game：

- 5 actions；`SUCCEEDED`；原生退出 0；UBT 总执行 49.90 秒；
- run-state SHA-256：`7BD87D15AC35022A15C7BFE82F8CA743BD014727FFDAC71196127A4DDE73168D`；
- stdout SHA-256：`BBC87E87D7C5F00E76BE30AA106E1A8C562FE740EF7A9D2A6027E0AB3C79C193`；
- `demo_map.exe`：360,363,520 bytes；SHA-256 `BCED3267B7A311815B008B6D8D0D89680D2A59FF20CB7F3A9A29130EA8071AF4`。

两次 stderr 均为 0 bytes，SHA-256 `E3B0C44298FC1C149AFBF4C8996FB92427AE41E4649B934CA495991B7852B855`。

新增生产代码 618 行非空行，测试 374 行非空行。新生产代码未引入 World、Actor、控制器、GameMode、输入、计时器、异步、随机、库存/物品权威、Profile 或 SaveGame 依赖。

P 阶段完成编译、无头自动化、确定性/当前性/无副作用证明、静态边界和证据审计。F 阶段未启动 Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package；未执行实际资源预留、扣除、阵眼提交或挥洒表现。

## 10. 精确提交清单

1. `Source/demo_map/demo_mapShanmenFormationScatterBatchIntent.h`
2. `Source/demo_map/demo_mapShanmenFormationScatterBatchIntent.cpp`
3. `Source/demo_map/demo_mapShanmenFormationScatterBatchIntentTests.cpp`
4. `Source/demo_map/demo_mapShanmenFormationMasteryOperationAuthorization.cpp`
5. `Scripts/ShanmenRegressionMap.json`
6. `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`
7. `Docs/Report/Dev.D.UE.0.0.10.P27.17.r0_report.md`
8. `Docs/Log/Dev.D.UE.0.0.10.P27.17.r0_log.md`

`Saved/Codex/P27.17` 与 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P27.17.r0` 不进入 Git；103 个既有未跟踪用户文件保持未暂存。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p27-17-formation-scatter-batch-intent>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-17-formation-scatter-batch-intent/Docs/Report/Dev.D.UE.0.0.10.P27.17.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-17-formation-scatter-batch-intent/Docs/Log/Dev.D.UE.0.0.10.P27.17.r0_log.md>
