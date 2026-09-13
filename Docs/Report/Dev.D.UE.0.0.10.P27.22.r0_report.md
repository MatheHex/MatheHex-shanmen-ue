# Dev.D.UE.0.0.10.P27.22.r0 Report

## 1. 结论

P27.22 已把 P27.21 的完整、不可变阵眼部署提交证据转换为既有 World Placement Intent 契约。每个已经提交的阵眼恰好生成一个规范、确定性、可重放的 Placement Intent，且完整批次证据按阵眼顺序重新验证资源总量、部署身份、提交回执、位置、内容版本和 World 放置身份。

本阶段只建立 Deployment → World placement 的只读交接，不发布 Actor，不修改 World，不再次消耗资源，也不修改 Deployment。既有 ProductSession 的放置意图构建入口已复用同一个纯值验证核心，避免形成第二套 World 意图规则。

P27.22 专项 4/4、其余 18 个改动驱动专项组全部通过；完整 Shanmen.0_0_10 根组 1,392/1,392；映射自测 496/496；最终暂存差异覆盖门 Changed=9 Rules=3 Required=20 Logs=20；Editor 与 Game 两目标构建、JSON、静态边界和 git diff 检查全部通过。

## 2. 阶段问题与范围

P27.21 已证明：P27.20 不可逆资源履约可以按规范阵眼顺序提交到既有 Formation Deployment 内核，并生成 Active Deployment 与逐阵眼 Commit Receipt。但该证据尚不能直接交给现有 World Adapter，因为旧入口只接受 ProductSession 内部审计状态。

本轮闭合的范围是：

- 输入必须是完整且自验证的 P27.21 Deployment Commit Evidence；
- 每个源 Handoff、Fulfillment、Deployment Receipt 和 Deployment Anchor Progress 必须指向同一阵眼；
- 既有 Deployment Receipt 必须作为完全相同的回执存在于 Deployment 自身账本中；
- World Location、Anchor Instance、Authority Revision、Content 和提交身份只能来自已提交快照；
- P27.21 的逐阵眼 Handoff ID 直接作为 World Placement Attempt ID；
- 输出必须保持规范阵眼顺序、Placement ID 唯一、Handoff ID 唯一与总资源数量守恒；
- 完整重放只能重建同一份逻辑证据，不得追加 Deployment 回执或重新消耗物品；
- 不执行 World 发布、Actor 生成、视觉表现、输入、UI、计时或持久化。

## 3. 统一 Placement Intent 入口

Fdemo_mapShanmenFormationWorldAdapter 新增纯值重载 BuildPlacementIntent。它直接接受：

1. 完整 Formation Deployment；
2. 一个确定性 Attempt ID；
3. 一个 Anchor Fulfillment；
4. 一个 Deployment Commit Receipt；
5. 输出 Placement Intent。

该入口在生成意图前验证：

- Deployment、Attempt、Fulfillment 与 Receipt 全部有效；
- Receipt 事件必须是 CommitAnchor；
- Run、Owner、Deployment、Content、Anchor、Fulfillment 和 Authority Revision 一致；
- Deployment 中存在匹配 Anchor Definition 的已提交 Progress；
- Progress 内记录的 Fulfillment 与输入逐字段、逐材料行一致；
- Deployment 回执账本中存在同 Receipt ID 的记录，且序号、事件、状态转移、阵眼、履约、修订号与累计提交数全部一致。

只有这些条件全部成立，才从已提交 Progress 提取 Anchor Instance 与 World Location，并调用既有确定性 MakePlacementId。原 ProductSession 入口不再重复组装字段，而是完成会话审计定位后委托给此纯值入口。

## 4. 每阵眼 Handoff

Fdemo_mapShanmenFormationScatterAnchorWorldPlacementHandoff 保存：

- P27.22 Handoff ID；
- P27.21 Deployment Commit Evidence ID；
- 对应 P27.21 Anchor Deployment Handoff ID；
- 规范 Anchor Order；
- 既有 Fdemo_mapShanmenFormationAnchorPlacementIntent。

P27.21 Handoff ID 同时成为 Placement Intent 的 Attempt ID。这样每个已经不可逆提交的阵眼只有一个稳定 World 尝试身份；相同完成证据重放会生成相同 Placement ID，而外部调用者不能通过随机 GUID 制造平行放置尝试。

P27.22 Handoff ID 使用命名空间 demo_map.Formation.ScatterAnchorWorldPlacementHandoff.r1，绑定 P27.21 完成证据、源 Handoff、阵眼顺序和 Placement ID。IsValid 会重新派生 ID，并要求 Intent 自身有效且 Attempt ID 精确等于源 Handoff ID。

## 5. 整批完成证据

Fdemo_mapShanmenFormationScatterWorldPlacementHandoffEvidence 保存原始 P27.21 完成证据、规范 Handoff 数组、总提交资源量与确定性 Evidence ID。

IsValid 不信任构造过程，会：

1. 重新验证 P27.21 完成证据；
2. 要求 Handoff 数量同时等于 P27.21 Handoff 数量和 Deployment Anchor 数量；
3. 要求总量精确等于 P27.21 TotalCommittedQuantity；
4. 按索引重新调用 BuildAnchorHandoff；
5. 对比每个 Handoff 的全部身份和 Placement Intent 字段；
6. 拒绝重复 Handoff ID 或重复 Placement ID；
7. 重新派生整批 Evidence ID。

整批 Evidence ID 使用命名空间 demo_map.Formation.ScatterWorldPlacementHandoffEvidence.r1，绑定 P27.21 Evidence ID、Deployment ID、阵眼数、总量和全部规范 Handoff ID。

## 6. 失败关闭与只读重放

Builder 只有一个成功状态 Ready；无效源证据、单阵眼意图拒绝或最终证据不变量失败分别返回 DeploymentEvidenceInvalid、PlacementIntentRejected 或 CompletionEvidenceInvalid。

输入全部为 const 值证据，Builder 只构造局部输出。失败不会修改 Deployment、资源权威、World 或调用方状态。相同 P27.21 证据连续构建两次会得到相等的 P27.22 Evidence；测试同时核对持久物品权威快照、Deployment 状态、已提交阵眼数和回执数均保持不变。

## 7. 自动化证明

| Group | Success | Fail | Log SHA-256 |
|---|---:|---:|---|
| Shanmen.0_0_10.Product.FormationScatterWorldPlacementHandoff | 4 | 0 | E8D52B3F690BFC7527EC6B583E03E77CEC52D8A9D3A1F75B1AE2F67E4692C0AC |
| Shanmen.0_0_10.Product.FormationWorldDelivery | 4 | 0 | 3B3F71794017846AF32C892D4333F38A8840EACC2C3D7A42A5443E6DA3137AAC |
| Shanmen.0_0_10.Product.FormationScatterDeploymentCommit | 5 | 0 | 0AF953B1C6A3415D6861A8786E1B457DC485204590A724E4466E92A372BF5D61 |
| Shanmen.0_0_10.Product.FormationScatterResourceCommit | 5 | 0 | 7A3FA1C7D7EBB58ACAF4804FE18B6406A6B362F6F69B3BC71FE517C5E0FE175E |
| Shanmen.0_0_10.Product.FormationScatterResourcePreparation | 5 | 0 | 94D363043A2FD24E8F7C5A1C21F1ED7E56DC5C3F4E4196ACF3C3618B11A7ED8D |
| Shanmen.0_0_10.Product.FormationScatterResourcePlan | 4 | 0 | 4CCE4958CB193D99A7ED0A037BB028E1AD71D1F5283D93C4B7DACAED505EF6E9 |
| Shanmen.0_0_10.Product.FormationScatterBatchIntent | 4 | 0 | EFC5E2270B1B414E33722340F651745C78540C7FE0D4FD87A48B68339EF73127 |
| Shanmen.0_0_10.Product.FormationMasteryOperationAuthorization | 4 | 0 | 7E5EF7D692FAC0D77756534B7A9680D85F2265DD3C81CF16898F60E1DDDBE2DC |
| Shanmen.0_0_10.Product.FormationMasteryAuthorityAdapter | 4 | 0 | E2A5E492935922CCCBF59E2F1C4445FB6EF5297D8D0DFA19C10C8A2437EBB847 |
| Shanmen.0_0_10.Product.FormationMaterialAdapter | 4 | 0 | DE8DBB789FDF5FBCE721232F32E49D0DBFA6F6EE307211C0113C03F60C80B997 |
| Shanmen.0_0_10.Product.FormationSession | 4 | 0 | 60AA8B7C2D9FC4EE5A1C9E6570E8CED0EC1C0C8BE2E167E3AEF5FD34F98C14B9 |
| Shanmen.0_0_10.Items | 77 | 0 | F313F96D1453826C57E8759487CD49C36E2E0DBF139AB7D16F99E856E6D04D50 |
| Shanmen.0_0_10.WorldGameplay | 10 | 0 | 4ACD4814E5E229AD29E7FF3C883B70DABD237DBA02D720E002A7FB2B0F72425A |
| Shanmen.0_0_10.CombatRuntime.FormationMastery | 2 | 0 | 2C8EFE4193D5B11D594EF21D79755E414F4FEBBE0A65BE4F51408C2B3BBA66C7 |
| Shanmen.0_0_10.CombatRuntime.FormationDeployment | 4 | 0 | A7D75BDD6319472151E429A13BA1ACE764F2336EA68D6AF8F3FB2269C1EB89A4 |
| Shanmen.0_0_10.CombatCore | 9 | 0 | EAB133B18A6AA21A4EDBEA5B43B8342935FA3A5A1D6A7A3A13B394B9BD561DA9 |
| Shanmen.0_0_10.Product.FormationInfluenceLifecycleCommandHost | 5 | 0 | DE0A619ABFEF251E5479A95AF9087DF0A58E70AA25F34A5EF7392DF8A9B60D2A |
| Shanmen.0_0_10.Product.FormationInfluenceLifecycleCommandRouter | 4 | 0 | 1E6EA1E351ACA46FAE11FFB86B0DFD2E1A746C62A36A4AE638AAECDB4EAFC2EC |
| Shanmen.0_0_10.Product.FormationInfluenceLifecycleCoordinator | 4 | 0 | 8F3200B41767A6A460F0E9E7187B36B33DE4F2B8BDEA1B9D9197BDF37CF383F7 |
| Shanmen.0_0_10 | 1,392 | 0 | B38EBD57405DFBD3B8FA8AFEB6781993988D495739B3B7F94E4F205066F3843D |

20 份自动化日志各包含一条准确 RunTests 命令、原生队列完成标记、0 Fail 和 0 Fatal/Unhandled/Ensure。完整根组从 2026-09-13 13:40:09.332 UTC 到 14:46:26.885 UTC，连续执行 1 小时 6 分 17.553 秒，并以 Automation Test Queue Empty 1392 tests performed 和原生退出状态 0 结束。

四条 P27.22 专项分别证明：

1. WholeBatchIntent：两个已提交阵眼各生成一个规范且唯一的 Placement Intent，完整身份链与资源总量正确；
2. DeterministicReplayIsReadOnly：相同 P27.21 证据重放得到相等证据，持久物品快照、Deployment 状态和回执数不变；
3. ForeignEvidenceRejected：外来 Fulfillment、空 Attempt ID 和越界阵眼失败关闭；
4. InvalidCompletionRejected：缺失 P27.21 完成证据在任何输出发布前被拒绝。

## 8. 改动驱动回归

ShanmenRegressionMap 新增 FormationScatterWorldPlacementHandoff 规则，并扩展既有 World Adapter 规则的后续生命周期覆盖。最终 7 个代码、测试和映射改动命中 3 条规则，要求 20 个唯一测试组。

- regression map JSON：PASS；
- 映射器正反自测：496/496 PASS，日志 SHA-256 E4800DF441285FA7701C0135BA1A1BE8B17E539B900CA1D8407BD07A9159B18E；
- 最终暂存覆盖门：REGRESSION_COVERAGE: PASS Changed=9 Rules=3 Required=20 Logs=20；
- 覆盖门日志 SHA-256：79A1202A80680E1549D3CE0B9C7DDABC27013CBF458020345E9BEE2DC9FD243F；
- git diff --check 与暂存 diff 检查：PASS。

## 9. 构建、静态边界与 P/F

Editor 开发中首次编译 88 actions，Result: Succeeded，原生退出 0，总计 36.08 秒；强化 Deployment Receipt 账本成员校验后复编 6 actions，Result: Succeeded，原生退出 0，总计 6.17 秒。完整回归后的最终 Editor 复核为 target up to date、0 actions、Result: Succeeded、原生退出 0、总计 1.53 秒，日志 SHA-256 FB3594EF692F57A7B291622A1C08962C57F0599A924C1923FA1A5F95E2D32352。

最终 UnrealEditor-demo_map.dll 为 19,998,720 bytes，SHA-256 06F784522D7B8080BD0ACB1E36938FB63023225DB15254DC82E4EFD8C05EE6B3。

Game 因新增源文件执行 87 actions，Result: Succeeded，原生退出 0，UBA 59.37 秒，总计 61.44 秒；日志 SHA-256 3DCE219D26D37C4D3E5549E0232487AB4A3E4EE027C36A725EAE46D3357619B7。最终 demo_map.exe 为 360,582,656 bytes，SHA-256 B0A2AF14A3F397E5C3E3AC3E76115C37A7BC6760BB9054D8A5BB54078E869691。

P27.22 新纯桥接生产代码共 388 行非空行（头文件 112、实现 276）。静态扫描未发现 UWorld、AActor、Spawn/Destroy Actor、InventorySubsystem、Profile、SaveGame、计时/Tick、异步、FGuid::NewGuid 或 RNG。WorldAdapter 保留既有 World 能力，但新增重载自身只构建值类型意图。

P 阶段已证明：完整资源与部署证据可以无副作用地形成确定性 World Placement Handoff；整批证据可自验证；重放只读；外来或缺失证据失败关闭；旧 ProductSession 与新批次路径共用一个意图契约。

F 阶段未执行：没有启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、Cook 或 Package；没有实际生成或销毁 Actor，没有发布阵眼到 World，也没有声明玩家可见的挥洒布阵已经完成。

下一独立阶段 P27.23 可消费 P27.22 不可变证据，建立明确的、只前进的 World Publication Port 与放置账本。它应对每个 Placement ID 生成精确发布回执，允许成功前缀恢复和完整幂等重放；失败不得回滚 Deployment 或重新消耗资源。

## 10. GitHub 交接

基线提交：0a60f3dd6280316a0cac4141b9908de9cf13c7ab（P27.21）。分支：agent/0.0.10-p27-22-formation-scatter-world-handoff。

本阶段只提交 2 个新 World Placement Handoff 源文件、WorldAdapter 头/实现、关联测试、2 个回归映射文件、本 Report 与本 Development Log。103 个既有未跟踪用户文件保持未暂存；Saved/FoundationRuns、构建日志和二进制产物不进入 Git。

- Branch: <https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p27-22-formation-scatter-world-handoff>
- Report: <https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-22-formation-scatter-world-handoff/Docs/Report/Dev.D.UE.0.0.10.P27.22.r0_report.md>
- Development Log: <https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-22-formation-scatter-world-handoff/Docs/Log/Dev.D.UE.0.0.10.P27.22.r0_log.md>
