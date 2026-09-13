# Dev.D.UE.0.0.10.P27.21.r0 Development Log

## 1. 目标

- 把 P27.20 不可逆资源履约证据交给既有 Formation Deployment 内核；
- 每个资源阵眼必须精确转换为既有 Fulfillment，而不是创建第二套部署真值；
- 使用候选副本完成整批提交、最终验证和单点发布；
- 精确规范前缀只补后缀，完整结果幂等重放；
- 外来/稀疏前缀、终态部署、身份串线与无效证据失败关闭；
- 保留资源数量、物理 Item 来源、Authority Revision 与部署回执序列；
- 不访问 World、Actor、输入、UI、存档或视觉表现；
- 完成改动驱动回归、双目标构建、Report/Log 与 GitHub 推送。

## 2. 基线与范围

- 基线：`fa8898bd4511d8d24ae832c36d7cf980d95e4ed6`（P27.20）；
- 分支：`agent/0.0.10-p27-21-formation-scatter-deployment-commit`；
- 起始 tracked tree clean；103 个既有未跟踪用户文件保持未暂存；
- 输入为一份有效 P27.20 `ResourceCommitEvidence`、匹配的 Active Action Runtime 和既有 Deployment；
- 输出为完成后的 Deployment 快照、逐阵眼 Handoff 与自验证完成证据；
- 不重新调用资源 Authority，不修改 Profile/schema、输入、UI、地图或内容资产；
- 不启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、Cook 或 Package。

## 3. 新增文件与类型

新增：

- `Source/demo_map/demo_mapShanmenFormationScatterDeploymentCommit.h`；
- `Source/demo_map/demo_mapShanmenFormationScatterDeploymentCommit.cpp`。

关联修改：

- `demo_mapShanmenFormationScatterResourcePreparationTests.cpp`：沿用 P27.19/P27.20 真实持久物品权威夹具，追加五条 P27.21 专项；
- `Scripts/ShanmenRegressionMap.json`：加入本阶段 13 组必跑证据；
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`：加入一正一负映射夹具。

主要类型：

1. `Edemo_mapShanmenFormationScatterDeploymentCommitStatus`：完成、重放与各类输入/身份/终态/前缀/内核/完成证据拒绝；
2. `Fdemo_mapShanmenFormationScatterAnchorDeploymentHandoff`：一份资源阵眼履约到既有部署 Fulfillment/Receipt 的完整链；
3. `Fdemo_mapShanmenFormationScatterDeploymentCommitEvidence`：整批 Active Deployment 的不可变自验证证明；
4. `Fdemo_mapShanmenFormationScatterDeploymentCommitResult`：状态、初始前缀长度、新回执、完成证据与诊断；
5. `Fdemo_mapShanmenFormationScatterDeploymentCommitter`：边界验证、单阵眼转换、候选提交与最终发布入口。

## 4. 输入预检与规范前缀

提交前依次验证资源证据、Action Runtime、Deployment、跨系统身份与阵眼形状。跨系统身份包含 Run、Owner、Activation、Deployment、Diagram、Action Definition 与 Content。阵眼形状包含资源证据、批次 Intent 与 Deployment 三方的数量、规范顺序、Intent/Definition/Instance 身份和 World Location。

既有部署提交只有严格规范前缀合法：索引 `< CommittedAnchorCount` 的阵眼必须全部已提交，后续阵眼必须全部未提交；每个既有 Fulfillment 必须能从当前 P27.20 证据重新派生；对应 Commit Receipt 的 Sequence、Committed Count、Fulfillment ID 与 Authority Revision 必须一致。任何外来、稀疏或冲突历史返回 `ExistingAnchorConflict`，不尝试覆盖。

## 5. 候选副本与发布

预检成功后复制 Deployment。每个缺失阵眼的 Fulfillment 从 P27.20 履约确定性派生，并只调用 Candidate 的现有 `TryCommitAnchor()`。若任一步失败，Candidate 被丢弃，返回结果不含新回执，调用方 Deployment 未修改。

所有阵眼完成后，从 Candidate 重建整个 Handoff 集和完成证据。结果本身再次验证状态、前缀、新回执数量与完成快照中的精确位置。只有结果有效才把 Candidate 赋回调用方。

此顺序保证值类型部署边界的原子发布；资源早在 P27.20 已不可逆提交，所以本阶段不执行补偿、取消或重新消耗。

## 6. 身份、聚合与完成证据

每个 Handoff ID 使用命名空间 `demo_map.Formation.ScatterAnchorDeploymentHandoff.r1`，绑定 Resource Evidence、资源 Fulfillment、部署 Receipt 和阵眼顺序。整批 Evidence ID 使用 `demo_map.Formation.ScatterDeploymentCommitEvidence.r1`，绑定资源证据、Deployment/Activation、Handoff 序列与总量。

P27.20 的 Allocation Slice 可让同一物理 Item Instance 在同一阵眼内出现多次；既有 Deployment Fulfillment 要求物理 Item 唯一。本层因此只在相同 Item + Material 上合并数量，并保留该阵眼引用的最大持久 Authority Revision。材料冲突或 `int32` 溢出立即失败。

完成证据要求 Active Deployment、一个 Begin 回执、N 个规范 Commit 回执、N 个资源阵眼与 N 个 Handoff 精确一一对应；每条状态转移和回执计数必须正确；Handoff 累计资源量必须等于 P27.20 总提交量。

## 7. 专项自动化

五条专项使用 P27.19/P27.20 的真实 `FShanmenItemAuthorityService` 临时持久仓库，再把真实资源 Commit 证据交给既有 Deployment 内核：

1. `WholeBatchCommitAndReplay`：East/North 两阵眼全部提交，状态 Active，总量 6，重放不追加回执；
2. `CompatiblePrefixRecovery`：先提交精确 East，再由 P27.21 只补 North；
3. `ConflictingPrefixRejected`：先提交外来 Fulfillment，P27.21 拒绝且不触碰 North；
4. `TerminalDeploymentRejected`：资源 Commit 后取消 Deployment，P27.21 不允许复活；
5. `InvalidResourceEvidenceRejected`：默认空资源证据在修改前拒绝。

专项结果：5 Success、0 Fail；日志 SHA-256 `820F8D32918519302A5BD983B06696CEB433E085D989E600801A53D00AC4C2FD`。

## 8. 自动化与回归结果

| Group | Success | Fail | SHA-256 |
|---|---:|---:|---|
| `Shanmen.0_0_10.Product.FormationScatterDeploymentCommit` | 5 | 0 | `820F8D32918519302A5BD983B06696CEB433E085D989E600801A53D00AC4C2FD` |
| `Shanmen.0_0_10.Product.FormationScatterResourceCommit` | 5 | 0 | `AAB078CDC55F66A3C7BDF6B97CA7F6FFDFBF880B839AD77B4892007CD388CEC6` |
| `Shanmen.0_0_10.Product.FormationScatterResourcePreparation` | 5 | 0 | `3AB35C02B449E1D160DAEAD1630370A287C55E98DBE9AEC807E8E3BF05266ECC` |
| `Shanmen.0_0_10.Product.FormationScatterResourcePlan` | 4 | 0 | `0968B8FFD8C570F6DBB29F586B17902B9965F204A54ABF5AF84FD07C63891DA9` |
| `Shanmen.0_0_10.Product.FormationScatterBatchIntent` | 4 | 0 | `E1F8B73150B1A99816D6BB6A1F9A0ABA46FC3A1569268F122A731B5444679584` |
| `Shanmen.0_0_10.Product.FormationMasteryOperationAuthorization` | 4 | 0 | `3CEF2458F7958AF0D5A5835D6AEDA6165D78D16FAAD3027C3AF5D45489584C53` |
| `Shanmen.0_0_10.Product.FormationMasteryAuthorityAdapter` | 4 | 0 | `DC087470A395AADF16B8FDEC9667B0152E4D6419AD4013E463A77C67DB84EEE9` |
| `Shanmen.0_0_10.Product.FormationMaterialAdapter` | 4 | 0 | `3008F79E68F1E40D53FB981C861BDC513F3AED357C9C262A5BC97EA0D26DD6E6` |
| `Shanmen.0_0_10.Items` | 77 | 0 | `BB94B821B0CCF24A09666B11A63F08FAE75BC2C464774E02D7FA313B64A84327` |
| `Shanmen.0_0_10.CombatRuntime.FormationMastery` | 2 | 0 | `5976A4D9E72F08CA3DB4A0FD606A10F17FE5A9B07AE6BBA8FF0CE47087F75023` |
| `Shanmen.0_0_10.CombatRuntime.FormationDeployment` | 4 | 0 | `135BFB3B7DCEAB4FCA60EE260F19A9952789E04790F291763215EE7C9AADFB43` |
| `Shanmen.0_0_10.CombatCore` | 9 | 0 | `5C97D462C747AFF9283D3A1E8CE05CD661002F009486DEEB80DD22081C9AB0FA` |
| `Shanmen.0_0_10` | 1,388 | 0 | `240A69D37B992CC5B3EE4B07298AC25CBF9E838B21BE19516857EB15CEC8DAC3` |

13 份日志全部原生退出 0，每份各有一条准确 RunTests 命令、唯一完成标记、0 Fail 与 0 Fatal/Unhandled/Ensure。完整根组从 2026-09-13 11:42:46.928 UTC 到 12:44:45.245 UTC，连续执行 1 小时 1 分 58.317 秒。

回归映射：

- JSON 解析 PASS；
- 映射器正反自测 `494/494` PASS；
- 最终暂存覆盖门 `REGRESSION_COVERAGE: PASS Changed=7 Rules=2 Required=13 Logs=13`；
- `git diff --check` 与最终 `git diff --cached --check` PASS。

## 9. 构建、静态边界与 P/F

Editor：

- 初次编译 5 actions，`Result: Succeeded`，原生退出 0，UBT 7.94 秒；
- 最终复核：target up to date，0 actions，`Result: Succeeded`，原生退出 0，UBT 0.88 秒；
- `UnrealEditor-demo_map.dll`：19,968,512 bytes，SHA-256 `103DDD53EAB17AEA4083F2B1F7AF960CCD147DBB6ED0FDAF7222AC9F72D95205`。

Game：

- 5 actions，`Result: Succeeded`，原生退出 0，UBA 12.59 秒，UBT 15.41 秒；
- `demo_map.exe`：360,556,032 bytes，SHA-256 `87D7BBFDC287AA33CC850B7313671A5638031362E5020D31857874AE053924B6`。

新生产代码 822 行非空行；关联测试文件 1,195 行非空行。生产代码静态边界扫描没有 World/Actor/Character/GameMode/Profile/SaveGame/直接 InventorySubsystem、计时器、异步、随机 GUID 或 RNG 依赖。

P 阶段完成资源证据到 Deployment 的确定性交接、原子候选提交、规范前缀续接、幂等重放、冲突/终态/无效证据失败关闭、完整回归与双构建。F 阶段未启动 UI/PIE/Standalone/产品 exe，也未接入 World、执行真实输入或视觉验证。

## 10. 精确提交清单

1. `Source/demo_map/demo_mapShanmenFormationScatterDeploymentCommit.h`
2. `Source/demo_map/demo_mapShanmenFormationScatterDeploymentCommit.cpp`
3. `Source/demo_map/demo_mapShanmenFormationScatterResourcePreparationTests.cpp`
4. `Scripts/ShanmenRegressionMap.json`
5. `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`
6. `Docs/Report/Dev.D.UE.0.0.10.P27.21.r0_report.md`
7. `Docs/Log/Dev.D.UE.0.0.10.P27.21.r0_log.md`

`Saved/FoundationRuns/Dev.D.UE.0.0.10.P27.21.r0` 与本地构建产物不进入 Git；103 个既有未跟踪用户文件保持未暂存。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p27-21-formation-scatter-deployment-commit>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-21-formation-scatter-deployment-commit/Docs/Report/Dev.D.UE.0.0.10.P27.21.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-21-formation-scatter-deployment-commit/Docs/Log/Dev.D.UE.0.0.10.P27.21.r0_log.md>
