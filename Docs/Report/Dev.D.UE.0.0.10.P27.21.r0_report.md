# Dev.D.UE.0.0.10.P27.21.r0 Report

## 1. 结论

P27.21 已在 P27.20 的完整持久资源履约证据与既有 `FShanmenFormationDeployment` 内核之间建立 `Fdemo_mapShanmenFormationScatterDeploymentCommitter`。它把每个已经不可逆消耗的阵眼资源履约转换为既有部署内核接受的阵眼 Fulfillment，并按规范阵眼顺序完成部署提交。

本阶段采用候选副本上的 copy-validate-publish：所有缺失阵眼先提交到 Deployment 副本，整批 Active 状态、回执序列、资源归属、数量守恒和确定性证据再次验证成功后，才一次性发布回调用方。任何中途拒绝、身份串线、稀疏/外来前缀或无效完成证据都不会把部分修改泄漏到调用方。

专项 5/5、前序资源提交 5/5、资源准备 5/5、资源计划 4/4、批次意图 4/4、熟练度操作授权 4/4、熟练度权威适配器 4/4、材料适配器 4/4、Items 77/77、熟练度 2/2、部署 4/4、CombatCore 9/9、完整 `Shanmen.0_0_10` 1,388/1,388、映射自测 494/494、改动驱动覆盖门和 Editor/Game 两目标构建全部通过。

## 2. 阶段问题与范围

P27.20 已证明材料 Prepare 可以持久 Commit，并生成 Slice → 材料 Intent → 阵眼需求 → Commit 回执的完整证据，但资源终态与阵法部署终态仍是两个未连接的事实：资源已消耗并不自动表示阵眼已被既有部署内核接受。

本轮只闭合这条运行时交接：

- 输入必须是完整、自验证的 P27.20 资源 Commit 证据；
- Action Runtime、Deployment、授权、Plan、Run、Owner、Activation、Diagram 与 Content 必须描述同一激活；
- P27.17 批次阵眼、P27.20 资源阵眼与 Deployment 阵眼必须数量一致、顺序一致、身份一致、位置一致；
- 每个资源履约只转换为既有 `FShanmenFormationAnchorFulfillmentEvidence`，不绕过 Deployment 内核；
- 已存在的提交只能是当前资源证据对应的严格规范前缀；
- 对兼容前缀只补缺失后缀，完整部署只重放证据；
- 不重新规划、Prepare、Commit 或取消物品资源；
- 不接 World、Actor、输入、UI、计时、持久化或视觉表现。

## 3. 原子候选提交算法

`Commit()` 的执行顺序固定为：

1. 验证 P27.20 资源证据完整；
2. 验证 Action Runtime 有效、非终态且仍可发出候选；
3. 验证 Deployment 快照与资源/授权/动作身份完全一致；
4. 验证 Deployment 仍为 Deploying 或 Active；
5. 验证资源、批次与 Deployment 的阵眼形状完全一致；
6. 验证既有提交是从索引 0 开始的严格规范前缀，且每条既有 Fulfillment/Receipt 与当前资源证据重新派生的结果相同；
7. 复制 Deployment 为 Candidate；
8. 从首个未提交阵眼开始，按规范索引构建 Fulfillment 并调用 Candidate 的 `TryCommitAnchor()`；
9. 任一构建或内核提交失败即丢弃 Candidate，调用方 Deployment 保持原样；
10. 全部完成后，从 Candidate 重建整批完成证据并执行最终 `IsValid()`；
11. 只有结果自身也通过验证，才执行 `Deployment = Candidate`；
12. 有新回执返回 `Committed`，完整既有结果返回 `Replayed`。

这不是把多条 World 写入伪装成数据库事务；它是在纯值类型 Deployment 边界内提供真正的调用方原子发布。P27.20 资源已是不可逆事实，本层既不尝试补偿，也不创建第二库存真值。

## 4. 阵眼证据转换

`BuildAnchorEvidence()` 对每个规范阵眼执行确定性转换：

- Fulfillment ID 直接采用 P27.20 阵眼资源履约 ID，避免生成平行身份；
- Run、Owner、Deployment、Content 与阵眼定义来自已经自验证的 Plan/Authorization；
- 每个资源履约行必须能在整批证据中找到成功的 Committed 物品回执；
- Authority Revision 取该阵眼引用的全部 Commit 回执最大修订号；
- 同一物理 Item Instance 在一个阵眼内跨 Slice 出现时按材料定义合并数量；
- 材料定义冲突、数量溢出、空证据或任何无效回执均失败关闭；
- 输出最终再经过既有 `FShanmenFormationAnchorFulfillmentEvidence::IsValid()`。

合并是必要的，因为 P27.20 以 Allocation Slice 表达资源归属，而部署内核要求每个 Fulfillment 内物理 Item Instance 唯一。转换保留总量与物理来源，同时服从既有内核契约。

## 5. 完成证据与恢复边界

`Fdemo_mapShanmenFormationScatterDeploymentCommitEvidence` 保存：

- 原始 P27.20 整批资源证据；
- 完成后的 Active Deployment 快照；
- 每个阵眼的资源履约、转换后 Fulfillment 和既有部署 Commit Receipt；
- 整批已提交资源总量；
- 由资源证据、部署身份、动作身份、总量和逐阵眼 Handoff ID 派生的确定性 Evidence ID。

`IsValid()` 不信任构造过程，会重新验证完整身份、阵眼形状、严格提交前缀、Begin + N 条 Commit 回执序列、每条状态转移、资源到部署证据映射，以及全局数量守恒。

恢复规则只有两类：

- 精确前缀：采用已存在的规范回执，只提交缺失后缀；
- 完整结果：重建相同证据并返回 `Replayed`，不追加回执。

外来 Fulfillment、稀疏提交、错误顺序、终态 Deployment 或不匹配身份都失败关闭，不会“修正”历史状态，也不会让已消耗资源复活。

## 6. 自动化证明

| Group | Success | Fail | Log SHA-256 |
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

13 份日志均各含一条预期 `Automation RunTests` 命令、唯一 UE 5.8 原生完成标记、原生退出 0、0 Fail 与 0 Fatal/Unhandled/Ensure。完整根组第一项于 2026-09-13 11:42:46.928 UTC 开始，最后一项于 12:44:45.245 UTC 完成，连续执行 1 小时 1 分 58.317 秒。

五条专项分别证明：

1. `WholeBatchCommitAndReplay`：两个资源支持的阵眼按规范顺序提交为 Active，完整证据有效，重放不追加回执；
2. `CompatiblePrefixRecovery`：已有精确 East 前缀时只补 North 后缀，并采用原前缀回执；
3. `ConflictingPrefixRejected`：外来 Fulfillment 前缀被拒绝，未提交阵眼与回执数量保持不变；
4. `TerminalDeploymentRejected`：资源虽已消耗，Cancelled 部署仍不可复活；
5. `InvalidResourceEvidenceRejected`：无效 P27.20 证据在任何部署修改前被拒绝。

## 7. 改动驱动回归

新增 `FormationScatterDeploymentCommit` 映射规则。七个实现、测试、映射与交接文件要求完整根组、本阶段 Deployment Commit、P27.20 Commit、P27.19 Prepare、P27.18 Plan、P27.17 Batch、P27.16 Authorization、P27.15 Authority Adapter、Material Adapter、Items、Formation Mastery、Formation Deployment 和 CombatCore 共 13 组证据。

- regression map JSON：PASS；
- 映射器正反自测：`494/494` PASS；
- 新增正向夹具证明本阶段改动要求全部 13 组日志；
- 新增负向夹具证明只有 Deployment Commit 聚焦日志不能替代资源链、物品、部署、核心和完整根组；
- 最终暂存覆盖门：`REGRESSION_COVERAGE: PASS Changed=7 Rules=2 Required=13 Logs=13`；
- `git diff --check` 与最终 `git diff --cached --check`：PASS。

## 8. 构建与静态边界

- Editor 初次编译：5 actions，`Result: Succeeded`，原生退出 0，UBT 7.94 秒；
- 完整回归后 Editor 最终复核：target up to date，0 actions，`Result: Succeeded`，原生退出 0，UBT 0.88 秒；
- `UnrealEditor-demo_map.dll`：19,968,512 bytes，SHA-256 `103DDD53EAB17AEA4083F2B1F7AF960CCD147DBB6ED0FDAF7222AC9F72D95205`；
- Game：5 actions，`Result: Succeeded`，原生退出 0，UBA 12.59 秒，UBT 15.41 秒；
- `demo_map.exe`：360,556,032 bytes，SHA-256 `87D7BBFDC287AA33CC850B7313671A5638031362E5020D31857874AE053924B6`。

新 Deployment Commit 生产代码为 822 行非空行（头文件 142、实现 680）；包含新增专项后的资源准备测试文件为 1,195 行非空行。生产文件静态扫描未发现 `UWorld`、`AActor`、角色、GameMode、Profile、SaveGame、直接 InventorySubsystem、计时器、异步、`FGuid::NewGuid` 或 RNG 依赖。

## 9. P/F 边界与下一阶段

P 阶段已证明：不可逆资源履约可以转换为既有 Deployment 阵眼证据；整批提交对调用方原子发布；严格前缀可恢复；完整结果可幂等重放；外来前缀、终态部署与无效资源证据失败关闭；资源总量、物理来源、回执序列与身份链全部可重新验证。

F 阶段未执行：没有启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、Cook 或 Package。没有把 Active Deployment 投影到 World，没有生成阵眼 Actor、投材飞行、阵法区域、战斗影响或玩家可见反馈，因此不声明挥洒布阵功能已完成。

下一独立阶段可消费 P27.21 的不可变完成证据，建立 Deployment → World placement 的窄 Handoff。该阶段必须保持资源/Deployment 事实只读，并以可重放的 World 身份映射防止重复生成阵眼；World 失败不得回滚或重新消耗 P27.20 资源。

## 10. GitHub 交接

基线提交：`fa8898bd4511d8d24ae832c36d7cf980d95e4ed6`（P27.20）。分支：`agent/0.0.10-p27-21-formation-scatter-deployment-commit`。本阶段只提交 2 个新 Deployment Commit 源文件、1 个关联测试文件、2 个回归映射文件、本 Report 与本 Development Log；103 个既有未跟踪用户文件保持未暂存，`Saved/FoundationRuns/Dev.D.UE.0.0.10.P27.21.r0` 与本地构建产物不进入 Git。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p27-21-formation-scatter-deployment-commit>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-21-formation-scatter-deployment-commit/Docs/Report/Dev.D.UE.0.0.10.P27.21.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-21-formation-scatter-deployment-commit/Docs/Log/Dev.D.UE.0.0.10.P27.21.r0_log.md>
