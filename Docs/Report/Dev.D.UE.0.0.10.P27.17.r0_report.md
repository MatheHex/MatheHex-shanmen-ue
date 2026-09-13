# Dev.D.UE.0.0.10.P27.17.r0 Report

## 1. 结论

P27.17 已在 P27.16 的顶阶整部署挥洒授权之下，建立纯函数、无副作用、确定性且自校验的整部署投材批次意图 `Fdemo_mapShanmenFormationScatterBatchIntent`。

一份当前有效的 Master `ScatterFormation` 授权会被严格展开为：一个部署批次、按阵图规范顺序排列的逐阵眼意图，以及每个阵眼内按原始需求顺序排列的逐材料意图。相同材料即使出现在多个阵眼，也不会跨阵眼聚合；每条需求都保留其阵眼定义、阵眼实例、顺序、数量和独立确定性身份，为后续资源预检与事务规划提供不可歧义输入。

专项 4/4、P27.16 授权 4/4、P27.15 权威适配器 4/4、熟练度策略 2/2、阵法部署 4/4、CombatCore 9/9、完整 `Shanmen.0_0_10` 1,369/1,369、Editor/Game 两目标构建、映射自测及改动驱动覆盖门全部通过。

## 2. 阶段问题与范围

P27.16 能证明某名玩家可在当前全新部署上执行一次顶阶挥洒，但没有定义这份授权究竟对应哪些阵眼、哪些材料、多少数量，以及调用方应以何种稳定顺序处理它们。若由库存层、表现层或 World 调用方临时遍历阵图，就可能产生顺序分歧、跨阵眼材料错误聚合、旧授权重用或部分执行后重新解释需求。

本轮只补齐这一条确定性接缝：

- 输入必须包含当前 P27.15 熟练度投影、当前部署和 P27.16 整部署挥洒授权；
- 授权必须仍能由当前熟练度与部署证据完整重建；
- 阵图定义与部署进度必须一一对应并处于规范顺序；
- 每个阵眼必须尚未提交且具有稳定实例 ID 与有限世界位置；
- 每条材料需求必须有效、连续有序且保持其精确数量；
- 输出只是一份不可变命令意图，不产生任何资源或产品副作用。

本轮不查询、预留、扣除或提交库存，不修改阵眼，不产生飞行或视觉表现，不接输入、World、时序、持久化或重试，也不选择局内物品最终权威。

## 3. 规范化批次结构

新增三层只读证据：

1. `Fdemo_mapShanmenFormationScatterMaterialIntent`：绑定授权、操作、部署、阵眼顺序/定义/实例、需求顺序、材料定义与数量；
2. `Fdemo_mapShanmenFormationScatterAnchorIntent`：绑定一个精确已部署阵眼、世界位置、其有序材料意图及阵眼材料总量；
3. `Fdemo_mapShanmenFormationScatterBatchIntent`：保留完整 P27.16 授权、全部有序阵眼意图、总需求条数和总材料数量。

规划器按 `FShanmenFormationDiagram` 的规范阵眼顺序展开，而不是依赖调用方输入顺序。每个阵眼内严格要求 `RequirementOrder == 数组索引`。批次级与阵眼级验证同时拒绝重复意图 ID、重复阵眼定义、重复阵眼实例和阵眼内重复材料定义。

专项夹具故意以反向作者输入建立东、北两个阵眼，捕获后的阵图仍按 East=0、North=1 输出。东阵眼需求为 Wood 2、Stone 1，北阵眼需求为 Wood 3；最终批次精确包含 2 个阵眼、3 条需求、总数量 6。

## 4. 需求隔离与确定性身份

三个身份命名空间分别为：

- `demo_map.Formation.ScatterMaterialIntent.r1`；
- `demo_map.Formation.ScatterAnchorIntent.r1`；
- `demo_map.Formation.ScatterBatchIntent.r1`。

材料意图 ID 覆盖授权/操作/部署、阵眼身份、需求顺序、材料和数量；阵眼意图 ID 进一步覆盖规范化世界位置位模式、材料意图序列与总量；批次 ID 覆盖完整授权身份、有序阵眼意图序列及汇总值。每层 `IsValid()` 都重算 ID 并核对完整结构，不把“GUID 非空”误当作有效证据。

共享材料按需求而不是按材料名聚合。东阵眼 Wood 2 与北阵眼 Wood 3 拥有不同阵眼实例和不同材料意图 ID，后续事务层不能把它们合并为一条失去目的阵眼的 Wood 5。数量汇总只用于守恒校验，不改变需求粒度。

世界位置以有限 double 位模式进入阵眼身份，并把正负零规范为同一零值，避免文本格式或负零造成平台外观相同但身份不同。

## 5. 授权、当前性与失败关闭

`Plan()` 先验证授权本身，再要求其方式为 `ScatterFormation`、目标为整个 Deployment，并验证当前熟练度投影与部署。随后调用 P27.16 唯一授权器的 `IsCurrentAuthorization()`；熟练度修订、部署身份、回执或提交进度变化都会返回 `AuthorizationStale`。

规划状态覆盖：`AuthorizationInvalid`、`AuthorizationNotScatter`、`MasteryProjectionRejected`、`DeploymentInvalid`、`AuthorizationStale`、`DeploymentShapeInvalid`、`RequirementInvalid`、`IntentInvalid` 和 `Planned`。失败结果不携带有效批次，成功结果必须通过完整批次自校验。

`IsCurrentBatch()` 会用批次中冻结的授权对当前证据重新规划，并比较整个结果。因此历史批次在部署变化后仍可保持内部有效、可审计，但不再是当前可执行批次。专项测试在规划后提交一个阵眼，证明旧批次仍 `IsValid()`，同时 `IsCurrentBatch()` 变为 false，旧授权也被新部署进度拒绝。

本轮同时加固 P27.16 授权的自验证：授权 ID 重算现在不仅检查档位和投材方式各自是有效枚举，还会从冻结档位重建 `FShanmenFormationMasteryPolicy` 并验证该档位确实允许该方式。被篡改为合法枚举但不合法组合的 Beginner/Scatter 或 Intermediate/Scatter 证据不能再自证有效。

## 6. 自动化证明

| Group | Success | Fail | Log SHA-256 |
|---|---:|---:|---|
| `Shanmen.0_0_10.Product.FormationScatterBatchIntent` | 4 | 0 | `DE351900CC465CEDC028516A4CF4231C206C35703D08F60F054C4FCE00663934` |
| `Shanmen.0_0_10.Product.FormationMasteryOperationAuthorization` | 4 | 0 | `04BBEE8E86A63AC2E10479CA07F3FBCA4C178115E7FF03EFD5F63C627ED6664D` |
| `Shanmen.0_0_10.Product.FormationMasteryAuthorityAdapter` | 4 | 0 | `5C9AEDE0E85207E04617234957A15D1B109F90E8CE715E0980FF962C5490F648` |
| `Shanmen.0_0_10.CombatRuntime.FormationMastery` | 2 | 0 | `432D2F51845CD921DD4DF15C02FE1830AC84A9D6BC6966EB6C6A7838120E647F` |
| `Shanmen.0_0_10.CombatRuntime.FormationDeployment` | 4 | 0 | `1C20B769D23B061665C4A4D128D6F74607DACA3F9897756913BF776F109D2815` |
| `Shanmen.0_0_10.CombatCore` | 9 | 0 | `C01F30B14A6FA35A94166F39160363B5CB85C463F27FCE47D3E0318EC90B22B2` |
| `Shanmen.0_0_10` | 1,369 | 0 | `F6E4D1B9FCCC18DE9EBB796242650D6034E91E682A7892C8753AA52E82748E57` |

七份最终日志均具有 UE 5.8 `TEST COMPLETE. EXIT CODE: 0`、测试进程原生退出 0、0 Fail 与 0 Fatal/Unhandled/Ensure。完整根组从 02:42:50.719 UTC 运行至 04:03:32.537 UTC，共 4,841.818 秒。

专项四条分别覆盖：确定性规范批次、逐阵眼需求隔离、无效/错误方式/过期授权围栏，以及当前性与无副作用。完整根组比 P27.16 增加且仅增加这 4 条，从 1,365 增至 1,369。

## 7. 改动驱动回归

新增 `FormationScatterBatchIntent` 映射规则。三个新文件必须同时具有完整根组、批次专项、P27.16 操作授权、P27.15 权威适配器、P27.14 熟练度策略、阵法部署和 CombatCore 七组证据。

- regression map JSON：PASS，共 266 条规则；
- 映射器正反自测：`486/486` PASS；
- 负向自测证明只有批次专项不能替代授权、权威、策略、部署、核心与完整组；
- 最终覆盖门：`REGRESSION_COVERAGE: PASS Changed=6 Rules=2 Required=7 Logs=7`；
- `git diff --check`：PASS。

两条匹配规则来自新增批次文件及对 P27.16 授权实现的加固；七份日志覆盖合并后的全部必跑组，覆盖工具记录的 SHA-256 与第 6 节一致。

## 8. 构建与静态边界

- Editor：6 actions，`SUCCEEDED`，原生退出 0，UBT 总执行 42.27 秒；run-state SHA-256 `D0F07D926E40E1840E240B05F3B66EA2E6B8357E9D50D9460BE46213C48C41CB`；stdout SHA-256 `241CA24AE7C74901B4B1C46A6E008B64D3E6868CBE0DE6293FA2B4208E8835E2`；
- `UnrealEditor-demo_map.dll`：19,749,376 bytes，SHA-256 `5D30FEC612B56EF6207277069B8FA18DD3307887E8AD7C3FAB83BDBC03B1B207`；
- Game：5 actions，`SUCCEEDED`，原生退出 0，UBT 总执行 49.90 秒；run-state SHA-256 `7BD87D15AC35022A15C7BFE82F8CA743BD014727FFDAC71196127A4DDE73168D`；stdout SHA-256 `BBC87E87D7C5F00E76BE30AA106E1A8C562FE740EF7A9D2A6027E0AB3C79C193`；
- `demo_map.exe`：360,363,520 bytes，SHA-256 `BCED3267B7A311815B008B6D8D0D89680D2A59FF20CB7F3A9A29130EA8071AF4`；
- 两次 stderr 均为空，SHA-256 `E3B0C44298FC1C149AFBF4C8996FB92427AE41E4649B934CA495991B7852B855`。

新增生产代码 618 行非空行，测试 374 行非空行。新生产代码静态扫描未发现 `UWorld`、`AActor`、PlayerController、GameMode、InputAction、Timer、Async、随机 GUID/RNG、Inventory/ItemAuthority、SaveGame 或 Profile 依赖。

## 9. P/F 边界与下一阶段

P 阶段已证明当前 Master 挥洒授权可确定性展开为完整、规范有序、逐阵眼隔离且数量守恒的批次意图；证明字段篡改、错误方式、旧熟练度、另一部署和部署进度变化均失败关闭；并完成完整回归、覆盖门和双目标构建。

F 阶段未执行：没有启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、Cook 或 Package。本轮不声明玩家可在游戏中看见挥洒动作，不声明材料已经足够、预留、扣除、飞行或提交，也不声明部分失败补偿已经定义。

下一独立阶段可建立“批次意图到资源事务预检”的纯规划契约。它必须针对同一库存快照做全批次累计分配，防止两个阵眼分别通过单项检查却合计超额；仍应把预检/分配与真实 Reserve、Commit、阵眼提交和 World 表现分开。局内物品最终权威及跨阵眼原子性需要在接入前明确，不能由本批次意图自行决定。

## 10. GitHub 交接

基线提交：`a79a005777a401c324c37d752f3cba45b2d3f7e1`（P27.16）。分支：`agent/0.0.10-p27-17-formation-scatter-batch-intent`。本阶段只提交 3 个新增源/测试文件、1 个授权加固文件、2 个回归映射文件、本 Report 与本 Development Log；103 个既有未跟踪用户文件保持未暂存，`Saved/Codex/P27.17` 与构建证据不进入 Git。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p27-17-formation-scatter-batch-intent>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-17-formation-scatter-batch-intent/Docs/Report/Dev.D.UE.0.0.10.P27.17.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-17-formation-scatter-batch-intent/Docs/Log/Dev.D.UE.0.0.10.P27.17.r0_log.md>
