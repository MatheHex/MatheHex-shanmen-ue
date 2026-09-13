# Dev.D.UE.0.0.10.P27.22.r0 Development Log

## 1. 目标

- 消费 P27.21 完整 Deployment Commit Evidence；
- 把每个已提交阵眼转换为既有 World Placement Intent；
- 复用一个统一、纯值、严格验证的 WorldAdapter 意图入口；
- 让 P27.21 Handoff ID 成为稳定 Placement Attempt ID；
- 构建按规范阵眼顺序排列的不可变整批交接证据；
- 验证资源总量、Deployment 账本成员、位置、内容和身份链；
- 相同证据确定性重放，不重新消耗资源或追加 Deployment 回执；
- 外来、越界、缺失或串线证据失败关闭；
- 不执行 World/Actor 发布；
- 完成改动驱动回归、双目标构建、Report/Log 和 GitHub 推送。

## 2. 基线与范围

- 基线：0a60f3dd6280316a0cac4141b9908de9cf13c7ab（P27.21）；
- 分支：agent/0.0.10-p27-22-formation-scatter-world-handoff；
- 起始 tracked tree clean；
- 103 个既有未跟踪用户文件保持未暂存；
- 输入为一份有效 P27.21 Fdemo_mapShanmenFormationScatterDeploymentCommitEvidence；
- 输出为每阵眼 Placement Handoff 和整批 World Placement Handoff Evidence；
- 不重新调用物品 Authority，不修改资源 Commit、Deployment、Profile/schema、输入、UI、地图或内容资产；
- 不启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

## 3. 新增和修改文件

新增：

1. Source/demo_map/demo_mapShanmenFormationScatterWorldPlacementHandoff.h
2. Source/demo_map/demo_mapShanmenFormationScatterWorldPlacementHandoff.cpp

修改：

3. Source/demo_map/demo_mapShanmenFormationWorldAdapter.h
4. Source/demo_map/demo_mapShanmenFormationWorldAdapter.cpp
5. Source/demo_map/demo_mapShanmenFormationScatterResourcePreparationTests.cpp
6. Scripts/ShanmenRegressionMap.json
7. Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1

交接：

8. Docs/Report/Dev.D.UE.0.0.10.P27.22.r0_report.md
9. Docs/Log/Dev.D.UE.0.0.10.P27.22.r0_log.md

## 4. WorldAdapter 纯值入口

新增 BuildPlacementIntent(Deployment, AttemptId, Fulfillment, DeploymentReceipt, OutIntent)。该函数不读 ProductSession，也不访问 World 或 Actor。

验证流程：

1. 拒绝无效 Deployment、Attempt、Fulfillment 或 Receipt；
2. 只接受 CommitAnchor Receipt；
3. 对齐 Run、Owner、Deployment、Content、Anchor、Fulfillment 与 Authority Revision；
4. 在 Deployment 中按 Anchor Definition 定位 Progress；
5. 要求 Progress 已提交；
6. 逐字段和逐材料行核对 Progress 内 Fulfillment；
7. 在 Deployment Receipt Ledger 中按 Receipt ID 定位记录；
8. 核对 Receipt 的 ID、Deployment、Sequence、Event、State Before/After、Anchor、Fulfillment、Revision 和 Committed Count；
9. 从 Progress 提取 Anchor Instance 与 World Location；
10. 通过既有 MakePlacementId 派生稳定 Placement ID；
11. 最终 OutIntent.IsValid 后才返回成功。

原 ProductSession 入口保留原有审计定位和会话边界，然后委托给此入口，删除重复字段装配。

## 5. P27.22 数据契约

Edemo_mapShanmenFormationScatterWorldPlacementHandoffStatus：

- Invalid；
- Ready；
- DeploymentEvidenceInvalid；
- PlacementIntentRejected；
- CompletionEvidenceInvalid。

Fdemo_mapShanmenFormationScatterAnchorWorldPlacementHandoff：

- 绑定 P27.21 Evidence ID、P27.21 Handoff ID、Anchor Order 与 Placement Intent；
- P27.21 Handoff ID 同时用作 Intent.AttemptId；
- Handoff ID 由命名空间、源证据、源 Handoff、顺序和 Placement ID 派生；
- IsValid 重新派生 Handoff ID。

Fdemo_mapShanmenFormationScatterWorldPlacementHandoffEvidence：

- 保存完整 P27.21 Evidence；
- 保存规范顺序的全部阵眼 Handoff；
- 保存并核对 TotalCommittedQuantity；
- 对每个索引重新构建 Expected Handoff；
- 拒绝重复 Handoff ID 和 Placement ID；
- Evidence ID 绑定源 Evidence、Deployment、数量、总量和全部 Handoff ID。

Fdemo_mapShanmenFormationScatterWorldPlacementHandoffBuilder：

- BuildAnchorHandoff 只接受有效、范围内、已提交且与 P27.21 源 Handoff 一致的阵眼；
- Build 按索引构造整批 Handoff；
- 任一阵眼失败即返回拒绝结果，不发布部分 Evidence；
- 完整 Evidence 再次 IsValid 后才返回 Ready。

## 6. 专项测试

四条专项沿用 P27.19/P27.20 的真实临时持久物品 Authority 夹具，并先执行真实资源 Commit 和 P27.21 Deployment Commit：

1. WholeBatchIntent：East/North 两阵眼生成两个唯一 Placement ID，顺序、位置、资源与提交回执链全部一致；
2. DeterministicReplayIsReadOnly：同源连续构建证据相等，物品 Authority 快照、Deployment Active 状态、已提交数量和回执数不变；
3. ForeignEvidenceRejected：外来 Fulfillment ID、空 Attempt ID 和越界 Anchor Index 全部拒绝；
4. InvalidCompletionRejected：默认空 P27.21 Evidence 返回 DeploymentEvidenceInvalid。

最终专项结果：4 Success、0 Fail；日志 SHA-256 E8D52B3F690BFC7527EC6B583E03E77CEC52D8A9D3A1F75B1AE2F67E4692C0AC。

## 7. 自动化与回归结果

| Group | Success | Fail | SHA-256 |
|---|---:|---:|---|
| FormationScatterWorldPlacementHandoff | 4 | 0 | E8D52B3F690BFC7527EC6B583E03E77CEC52D8A9D3A1F75B1AE2F67E4692C0AC |
| FormationWorldDelivery | 4 | 0 | 3B3F71794017846AF32C892D4333F38A8840EACC2C3D7A42A5443E6DA3137AAC |
| FormationScatterDeploymentCommit | 5 | 0 | 0AF953B1C6A3415D6861A8786E1B457DC485204590A724E4466E92A372BF5D61 |
| FormationScatterResourceCommit | 5 | 0 | 7A3FA1C7D7EBB58ACAF4804FE18B6406A6B362F6F69B3BC71FE517C5E0FE175E |
| FormationScatterResourcePreparation | 5 | 0 | 94D363043A2FD24E8F7C5A1C21F1ED7E56DC5C3F4E4196ACF3C3618B11A7ED8D |
| FormationScatterResourcePlan | 4 | 0 | 4CCE4958CB193D99A7ED0A037BB028E1AD71D1F5283D93C4B7DACAED505EF6E9 |
| FormationScatterBatchIntent | 4 | 0 | EFC5E2270B1B414E33722340F651745C78540C7FE0D4FD87A48B68339EF73127 |
| FormationMasteryOperationAuthorization | 4 | 0 | 7E5EF7D692FAC0D77756534B7A9680D85F2265DD3C81CF16898F60E1DDDBE2DC |
| FormationMasteryAuthorityAdapter | 4 | 0 | E2A5E492935922CCCBF59E2F1C4445FB6EF5297D8D0DFA19C10C8A2437EBB847 |
| FormationMaterialAdapter | 4 | 0 | DE8DBB789FDF5FBCE721232F32E49D0DBFA6F6EE307211C0113C03F60C80B997 |
| FormationSession | 4 | 0 | 60AA8B7C2D9FC4EE5A1C9E6570E8CED0EC1C0C8BE2E167E3AEF5FD34F98C14B9 |
| Items | 77 | 0 | F313F96D1453826C57E8759487CD49C36E2E0DBF139AB7D16F99E856E6D04D50 |
| WorldGameplay | 10 | 0 | 4ACD4814E5E229AD29E7FF3C883B70DABD237DBA02D720E002A7FB2B0F72425A |
| FormationMastery | 2 | 0 | 2C8EFE4193D5B11D594EF21D79755E414F4FEBBE0A65BE4F51408C2B3BBA66C7 |
| FormationDeployment | 4 | 0 | A7D75BDD6319472151E429A13BA1ACE764F2336EA68D6AF8F3FB2269C1EB89A4 |
| CombatCore | 9 | 0 | EAB133B18A6AA21A4EDBEA5B43B8342935FA3A5A1D6A7A3A13B394B9BD561DA9 |
| FormationInfluenceLifecycleCommandHost | 5 | 0 | DE0A619ABFEF251E5479A95AF9087DF0A58E70AA25F34A5EF7392DF8A9B60D2A |
| FormationInfluenceLifecycleCommandRouter | 4 | 0 | 1E6EA1E351ACA46FAE11FFB86B0DFD2E1A746C62A36A4AE638AAECDB4EAFC2EC |
| FormationInfluenceLifecycleCoordinator | 4 | 0 | 8F3200B41767A6A460F0E9E7187B36B33DE4F2B8BDEA1B9D9197BDF37CF383F7 |
| Shanmen.0_0_10 full | 1,392 | 0 | B38EBD57405DFBD3B8FA8AFEB6781993988D495739B3B7F94E4F205066F3843D |

所有 20 份证据日志均被覆盖脚本重新读取，且每份只有一个 RunTests group、至少一个 Success、0 Fail、原生完成标记和 0 Fatal/Unhandled/Ensure。

完整根组第一项于 2026-09-13 13:40:09.332 UTC 完成，最后一项于 14:46:26.885 UTC 完成，总计 1 小时 6 分 17.553 秒；Automation Test Queue Empty 1392 tests performed；进程原生退出 0。

## 8. 回归映射

新增 FormationScatterWorldPlacementHandoff 正向夹具与缺证据负向夹具，并新增 Items 夹具。映射自测结果 SELF_TEST: PASS 496/496，日志 SHA-256 E4800DF441285FA7701C0135BA1A1BE8B17E539B900CA1D8407BD07A9159B18E。

最终暂存差异触发：

- P27.22 World Placement Handoff 完整资源/部署链；
- 既有 Formation World Delivery；
- Items、Formation Deployment、Formation Mastery、CombatCore 与 WorldGameplay；
- 因 WorldAdapter 被修改而触发 Formation Influence Lifecycle Coordinator、Command Router 和 Command Host；
- 完整 Shanmen.0_0_10 根组。

最终覆盖门：REGRESSION_COVERAGE: PASS Changed=9 Rules=3 Required=20 Logs=20。覆盖门日志 SHA-256 79A1202A80680E1549D3CE0B9C7DDABC27013CBF458020345E9BEE2DC9FD243F。

## 9. 构建、静态边界与卫生

Editor：

- 初次新增源文件：88 actions，Succeeded，原生退出 0，总计 36.08 秒；
- Receipt Ledger 强化后：6 actions，Succeeded，原生退出 0，总计 6.17 秒；
- 最终复核：target up to date，0 actions，Succeeded，原生退出 0，总计 1.53 秒；
- 最终日志 SHA-256 FB3594EF692F57A7B291622A1C08962C57F0599A924C1923FA1A5F95E2D32352；
- UnrealEditor-demo_map.dll：19,998,720 bytes，SHA-256 06F784522D7B8080BD0ACB1E36938FB63023225DB15254DC82E4EFD8C05EE6B3。

Game：

- 87 actions，Succeeded，原生退出 0；
- UBA 59.37 秒，总计 61.44 秒；
- 日志 SHA-256 3DCE219D26D37C4D3E5549E0232487AB4A3E4EE027C36A725EAE46D3357619B7；
- demo_map.exe：360,582,656 bytes，SHA-256 B0A2AF14A3F397E5C3E3AC3E76115C37A7BC6760BB9054D8A5BB54078E869691。

静态与卫生：

- 新 P27.22 纯生产代码 388 行非空行；
- 未发现 World/Actor 访问、Actor 生成/销毁、物品 Authority、Profile、SaveGame、Timer/Tick、异步、随机 GUID 或 RNG；
- JSON 解析 PASS；
- git diff --check PASS；
- 暂存 diff 检查 PASS；
- Report/Log 写入前暂存精确为 7 个实现/测试/映射文件，最终暂存精确为 9 个阶段文件；
- 103 个既有未跟踪用户文件保持未暂存。

## 10. P/F 与下一阶段

P 阶段完成：P27.21 Evidence → 每阵眼 Placement Intent → P27.22 完整 Handoff Evidence；验证账本成员、身份链、资源总量、规范顺序、唯一性、确定性与只读重放。

F 阶段未执行：无 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook、Package 或实际 World/Actor 发布。P27.22 只声明“可交给 World 发布层”，不声明玩家已看到阵眼。

建议下一阶段 P27.23：

- 定义只前进的 World Publication Port；
- 以 Placement ID 做幂等键；
- 每阵眼生成不可变发布回执；
- 已成功规范前缀只补缺失后缀；
- 完整批次重放不重复生成 Actor；
- 发布失败不回滚 P27.21 Deployment，不重新消耗 P27.20 资源；
- 继续保持 World 侧与资源/部署真值单向交接。

GitHub：

- Branch: <https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p27-22-formation-scatter-world-handoff>
- Report: <https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-22-formation-scatter-world-handoff/Docs/Report/Dev.D.UE.0.0.10.P27.22.r0_report.md>
- Development Log: <https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-22-formation-scatter-world-handoff/Docs/Log/Dev.D.UE.0.0.10.P27.22.r0_log.md>
