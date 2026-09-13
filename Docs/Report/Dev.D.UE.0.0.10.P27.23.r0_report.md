# Dev.D.UE.0.0.10.P27.23.r0 Report

## 1. 结论

P27.23 已把 P27.22 的完整阵眼 World Placement Handoff Evidence 接入既有 `Fdemo_mapShanmenFormationWorldAdapter`，形成可注入、只前进、可恢复、可幂等重放的整批 World Publication Port 与 Receipt Ledger。

每个规范阵眼按顺序交给 World；每次成功均产生绑定源交接、部署交接、Actor 类和既有 Placement Receipt 的不可变发布记录。中途失败时保留已经被 World 接受并经验证的规范前缀；后续重试先通过既有 Adapter 幂等重放并逐条核对该前缀，再从首个缺失阵眼继续。完整批次重放不会生成重复 Actor，也不会增加发布账本、回滚 P27.21 Deployment 或重新消耗 P27.20 资源。

本阶段使用真实无头瞬态 `UWorld` 与具体 `ACharacter` 类证明两阵眼发布和完整重放：首次产生 2 个 Actor，重放后仍为 2 个。P27.23 专项 4/4、其余 19 个改动驱动专项组全部通过；完整 `Shanmen.0_0_10` 根组 1,396/1,396；映射自测 498/498；最终覆盖门 `Changed=9 Rules=4 Required=21 Logs=21`；Editor 与 Game 双目标构建、JSON、静态边界和 Git 差异检查全部通过。

## 2. 阶段问题与范围

P27.22 已经能够把 P27.21 完整 Deployment Commit Evidence 转换为规范、确定性、只读的 World Placement Handoff，但它只声明“可交给 World”，并未执行发布。P27.23 闭合以下缺口：

- 输入必须是一份完整且可自验证的 P27.22 Handoff Evidence；
- 只允许一个明确、可验证的 Actor 类参与一个发布账本；
- 必须复用现有 World Adapter 的放置、Actor 标签、冲突检测、接管和重放语义；
- 每个成功阵眼必须记录既有 World Placement Receipt，而不是另造一套 World 真值；
- 批次只能按 P27.22 的规范阵眼顺序前进；
- 失败只能留下完整成功前缀，不能产生空洞、乱序或跨证据账本；
- 重试必须验证已有前缀，并只把缺失后缀追加到账本；
- 完整成功后必须生成绑定完整源证据和账本的不可变 Completion Evidence；
- 发布失败不得回滚 Deployment，也不得重新调用物品 Authority；
- 不接入 UI、输入、地图资产、可视产品流程或持久化。

## 3. 统一 World Publication Port

新增 `Idemo_mapShanmenFormationScatterWorldPlacementPort`，只暴露两个能力：

1. 返回本端口使用的 Actor Class Path；
2. 接收一个已经验证的不可变 Placement Intent，并返回既有 `Fdemo_mapShanmenFormationWorldResult`。

具体实现 `Fdemo_mapShanmenFormationScatterWorldAdapterPublicationPort` 持有一个 World、一个 Actor 类和一个既有 Formation World Adapter 引用。它不复制 Spawn、标签、接管或幂等规则，而是把每个 Intent 委托给新增的 `Fdemo_mapShanmenFormationWorldAdapter::TryPlaceIntent`。

原 `TryPlaceCommittedAnchor` 保留原有 ProductSession 审计与意图构建，然后委托给同一个 `TryPlaceIntent`。因此旧单阵眼路径与新整批发布路径共享一套实际 World 放置实现，不形成第二套 Actor 发布逻辑。

## 4. 发布记录、账本与完成证据

`Fdemo_mapShanmenFormationScatterAnchorWorldPublication` 为每个成功阵眼保存：

- P27.23 Record ID；
- P27.22 Handoff Evidence ID；
- P27.22 单阵眼 Source Handoff ID；
- P27.21 单阵眼 Deployment Handoff ID；
- 规范 Anchor Order；
- 既有 World Placement Receipt。

这里明确区分两个交接身份：P27.22 Source Handoff ID 标识本阶段消费的单阵眼交接记录；P27.21 Deployment Handoff ID 才是 Placement Intent 的稳定 Attempt ID。Record ID 使用命名空间 `demo_map.Formation.ScatterAnchorWorldPublication.r1`，绑定上述两级交接身份、源证据、顺序和 Receipt ID。

`Fdemo_mapShanmenFormationScatterWorldPublicationLedger` 是只前进账本：

- 空账本必须没有任何绑定字段；
- 首个成功回执把账本绑定到一个精确 P27.22 Evidence、Deployment 和 Actor Class Path；
- Records 必须从 Anchor Order 0 开始连续排列；
- Record ID 与 Placement ID 均不得重复；
- 每条 Receipt 的 Deployment、Actor 类和 Placement Intent 必须与对应 P27.22 Handoff 完全一致；
- 非空账本必须始终是源 Handoff 数组的规范成功前缀；
- Ledger ID 使用命名空间 `demo_map.Formation.ScatterWorldPublicationLedger.r1`，绑定源 Evidence、Deployment 与 Actor 类。

`Fdemo_mapShanmenFormationScatterWorldPublicationEvidence` 只有在账本长度等于完整 Handoff 数量时才有效。其 Evidence ID 使用命名空间 `demo_map.Formation.ScatterWorldPublicationEvidence.r1`，绑定 P27.22 Evidence ID、Ledger ID、发布数量和全部规范 Record ID。

## 5. 顺序发布、恢复与幂等语义

Publisher 在接触 World 前先验证源 Handoff、Port Actor Class Path 和已有 Ledger 兼容性。随后按 Anchor Order 顺序执行：

1. 把当前 Placement Intent 交给 Port；
2. 要求返回状态属于既有放置成功语义；
3. 用 Receipt 与对应 P27.22 Handoff 构建规范 Publication Record；
4. 核对 Receipt Actor Class Path；
5. 对已有前缀，要求重放结果与账本中原 Record 完全相等；
6. 对缺失后缀，以候选账本先验证再原子替换，并记录本次新增 Record；
7. 完整后重新构建并验证 Completion Evidence。

结果状态分为：

- `Published`：空账本首次完成整批发布；
- `Recovered`：已有成功前缀被验证后，缺失后缀完成；
- `Replayed`：完整账本被精确重放，账本零增长；
- `HandoffEvidenceInvalid`、`PortInvalid`、`LedgerConflict`、`PublicationRejected`、`ReceiptInvalid`、`CompletionEvidenceInvalid`：对应失败关闭原因。

失败结果记录初始/最终发布数、失败阵眼顺序和本次已经成功追加的 Records。World 拒绝不会擦除已被证明的前缀；但任何外来 Evidence、Actor 类变化、Receipt 漂移、前缀不一致或完成证据不变量失败都会拒绝继续。

## 6. P27.23 专项证明

四条专项测试分别覆盖：

1. `ConcreteWholeBatchAndReplay`：创建真实无头瞬态 `UWorld`，使用具体 `ACharacter` 类和真实 World Adapter；首次规范发布两个阵眼并生成两个 Actor，完整重放后 Actor、Adapter Record 和 Publication Ledger 均保持两个；资源 Authority 快照、Deployment Active 状态和 Deployment Receipt 数不变。
2. `PartialFailureRecoversPrefix`：注入第二次 Port 调用的一次性拒绝；首次结果只保留 Anchor 0，重试验证原前缀并只追加 Anchor 1，原 Record ID 不变。
3. `ForeignLedgerAndActorClassRejected`：完整账本不能跨 P27.22 Evidence，也不能改用另一个 Actor Class；两种冲突均在任何新 Port 调用前失败关闭。
4. `InvalidSourcePortAndReceiptRejected`：缺失源证据、空 Actor Class Path 和自洽但来自外来 Actor 类的 Receipt 均被拒绝，空账本不增长。

最终专项结果：4 Success、0 Fail、原生退出 0；日志 SHA-256 `79D971720B78C800012C2E358072181E551E988BAD557D7821705901E92DF682`。

## 7. 自动化与回归证据

| Group | Success | Fail | Log SHA-256 |
|---|---:|---:|---|
| Shanmen.0_0_10.Product.FormationScatterWorldPublication | 4 | 0 | 79D971720B78C800012C2E358072181E551E988BAD557D7821705901E92DF682 |
| Shanmen.0_0_10.Product.FormationScatterWorldPlacementHandoff | 4 | 0 | 58E18B35D0B14727C4021DB85D9B1AE2FE492A640765C53A3F7C8B676D43F410 |
| Shanmen.0_0_10.Product.FormationWorldDelivery | 4 | 0 | 1565A6008F35696F7D7017A14F7F702CC049958DD735E61BBE2B4BA9330F2907 |
| Shanmen.0_0_10.Product.FormationScatterDeploymentCommit | 5 | 0 | 18FD9584B1E06884545F6ED4C17DEF5B23364AC8BE69FFC811A0C49938A80CB6 |
| Shanmen.0_0_10.Product.FormationScatterResourceCommit | 5 | 0 | 9D47EF23789C60471C29821F92B6599933B423813E53F1C4C31CF1DC83135E3D |
| Shanmen.0_0_10.Product.FormationScatterResourcePreparation | 5 | 0 | 7547CB09B11463C4BCF578F16E9929A2533AD7D17E024B50CB06AB331DF43FA9 |
| Shanmen.0_0_10.Product.FormationScatterResourcePlan | 4 | 0 | 59023A47896AB1D66CA86499C68A847942635A7F85026FD46F50CB3ACEB5795F |
| Shanmen.0_0_10.Product.FormationScatterBatchIntent | 4 | 0 | BD3C977B9278EA55A38F1E6FF4DECE6604610E110D09B7D9C8F521732AC1E2C6 |
| Shanmen.0_0_10.Product.FormationMasteryOperationAuthorization | 4 | 0 | 721246A7395D061CDBFC9EFC9C4D16F0EA83A064FB9367C2AF0D0C92140130D6 |
| Shanmen.0_0_10.Product.FormationMasteryAuthorityAdapter | 4 | 0 | 583F599A37FEAE5CAAE3DB89DB2318524E9CF90A56939A3D9D02CCA63445BA5A |
| Shanmen.0_0_10.Product.FormationMaterialAdapter | 4 | 0 | D22EF0855B55F70542269678EE15F32B24CB8B47A9C23D63107BA4ACA79D07E7 |
| Shanmen.0_0_10.Product.FormationSession | 4 | 0 | 7D3E7380ECAE1CD1E3FA0207BA853DA412B7C38D45BA39AF3EDFE7538C485765 |
| Shanmen.0_0_10.Items | 77 | 0 | 35DFB9314449A0E1F6B5B5C7B726EDC2E3C0FF6664705609FEF8DEC1E15176AB |
| Shanmen.0_0_10.WorldGameplay | 10 | 0 | 7483A9E550A78E2D0AB6E44458EB00FFCD68FB62BE433E591F01D58BE8C34360 |
| Shanmen.0_0_10.CombatRuntime.FormationMastery | 2 | 0 | 019CB5C63D6B1EFBB2E3F98E272AA84AEB54911DEF18D4F2CC22FAEDED1FF719 |
| Shanmen.0_0_10.CombatRuntime.FormationDeployment | 4 | 0 | 13A6ACCC42EBBD217E09359E4F1609475F96713F07D8C8B6B89B412E3CFC6CFE |
| Shanmen.0_0_10.CombatCore | 9 | 0 | 5CA63AE7E5F3E2F1DB06CE377C170D52C35AFCC498C2B1DC80D303CBC18F9B81 |
| Shanmen.0_0_10.Product.FormationInfluenceLifecycleCommandHost | 5 | 0 | D728F0BE4F41F140060DC69415C5FBAC8311DBDE924C53D7C9647006844B5EE2 |
| Shanmen.0_0_10.Product.FormationInfluenceLifecycleCommandRouter | 4 | 0 | 7E9770F883E1A2C52A7FBD439035E6721703EA7F5B6E35DED98176B17A952D72 |
| Shanmen.0_0_10.Product.FormationInfluenceLifecycleCoordinator | 4 | 0 | BC224B05C3935C6E13F997055027258646AC5BF2772EA8F9B609EF4AA9552FA5 |
| Shanmen.0_0_10 | 1,396 | 0 | 817BDE96CF7E336F0FE2FB61FBB342F7944787C4F538CC168D7A10BEE1D96D2B |

全部 21 份健康证据日志各自包含准确的单一 `RunTests` 命令、至少一个 Success、0 Fail、0 Fatal/Unhandled/Ensure 和原生成功终止标记。完整根组从 2026-09-13 15:51:36.887 UTC 执行至 17:01:19.816 UTC，持续 1 小时 9 分 42.929 秒，1,396 个测试全部完成并以原生退出 0 结束。

## 8. 首次失败与自查修正

首次专项日志被完整保留，SHA-256 为 `3B6EDC4CC2952768053E8A35B01BA4971FC470274C2BD289B388C8B1CD989E44`。该运行完成 1 Success、2 Fail，随后在恢复测试中因读取空 Ledger 前缀触发数组越界断言并终止。

根因不是环境或内存故障，而是实现把 P27.22 Source Handoff ID 错当成 P27.21 Deployment Handoff ID。前者标识 P27.22 交接记录；后者才是 Placement Intent 的 Attempt ID。这个错误使合法 World Receipt 无法形成有效 Publication Record。修正包括：

- 在 Publication Record 中同时保存并验证两级 Handoff ID；
- 使用 P27.21 Deployment Handoff ID 核对 `Receipt.Intent.AttemptId`；
- 将两级身份同时纳入 Record ID 和 Ledger 兼容性；
- 测试在前缀未成功建立时立即返回，禁止再索引空数组。

第二次专项日志 SHA-256 为 `E54A10910BACDDCF274273328688E25CDABBB6B1026E9D9EC0A0B88D24BC7BCE`，结果 3 Success、1 Fail。剩余失败来自具体 World 测试使用基础 `AActor::StaticClass()`；该基础类没有为此夹具提供可保持规范位置的具体空间根，因而不能满足既有 Adapter 的 Actor 位置后置条件。测试改用项目已有、具有具体空间根且可生成的 `ACharacter::StaticClass()` 后，第三次专项 4/4 且原生退出 0。没有放宽生产校验来迎合测试。

## 9. 回归映射、构建与静态边界

`ShanmenRegressionMap.json` 新增 `FormationScatterWorldPublication` 规则，要求 Publication、P27.22 Handoff、World Delivery、完整资源/部署链、Items、WorldGameplay、Formation Mastery/Deployment、CombatCore 和完整根组。Self-test 同时加入正向完整证据夹具和“只有 Publication 专项不足以覆盖”的负向夹具。

- 映射自测：`SELF_TEST: PASS 498/498`，SHA-256 `1291331CC66DE8CB8E3CA75249A88E98FD23B376FE53A3355A32405ED586675D`；
- 最终暂存差异覆盖门：`REGRESSION_COVERAGE: PASS Changed=9 Rules=4 Required=21 Logs=21`，SHA-256 `23AA858DE65BD37BDE5B2127525CF0C35AD3F9EFFF979379C31BD7705C384DC0`；
- Editor 最终构建：target up to date，0 actions，Result Succeeded，原生退出 0，总计 2.23 秒；日志 SHA-256 `7C42DD141CC57C49DF60B29B5F6D983EEDCAA858D61798D04A96B162C7DC0096`；
- `UnrealEditor-demo_map.dll`：20,056,064 bytes，SHA-256 `9053006F55E4775AC55BC40C7180982ED0A810CF03F0265CF2AAFA8A0078FA0B`；
- Game 最终构建：88 actions，Result Succeeded，原生退出 0；UBA 43.19 秒，总计 45.01 秒；日志 SHA-256 `5509D004F7E0DC7B5C686DD81626C290BCF61BF8660395F4BA8254211CCF9177`；
- `demo_map.exe`：360,627,712 bytes，SHA-256 `9B5C88EE69EC9FA1F0C9BD6419B6CF03BD93916A8518466E7997CF18438F830B`；
- 新 P27.23 Publication 生产代码 696 行非空行；
- Publication 生产文件未发现物品 Authority、Inventory、Profile、SaveGame、Timer/Tick、异步、随机 GUID、RNG 或直接 Spawn/Destroy Actor；实际 Actor 操作只经既有 World Adapter；
- JSON 解析、`git diff --check` 与暂存差异检查全部通过；
- Report/Log 写入前暂存精确为 7 个实现、测试和映射文件；
- 103 个既有未跟踪用户文件保持原样且未暂存。

## 10. P/F 边界、下一阶段与 GitHub

P27.23 已真实执行无头瞬态 World/Actor 发布，因此不再只是一份纯值交接：完整 P27.22 批次能够生成 2 个具体 Actor，精确重放不重复生成；部分失败、前缀恢复、外来账本、Actor 类冲突和伪造 Receipt 均有自动化证明。

F 阶段仍未执行：没有启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、Cook 或 Package；没有把该 Publisher 接入玩家可触发的 Product Host/Command Route，也没有声明玩家已经能在正式地图中完成挥洒布阵。

建议下一独立阶段 P27.24：建立 Product-owned World Publication Session/Command Route，消费 P27.23 Publisher；把发布所有权绑定到 Run、Deployment 和 World 生命周期；为 Host 重建提供明确接管路径；为 Deployment 终态提供显式、幂等的 Actor teardown/reconciliation。该阶段继续禁止回滚 P27.20/P27.21 已提交事实，并保持所有 World 操作只经既有 Adapter。

基线提交：`d19d04986976f6d253d5da1ff3815237b961ff44`（P27.22）。分支：`agent/0.0.10-p27-23-formation-scatter-world-publication`。

- Branch: <https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p27-23-formation-scatter-world-publication>
- Report: <https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-23-formation-scatter-world-publication/Docs/Report/Dev.D.UE.0.0.10.P27.23.r0_report.md>
- Development Log: <https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-23-formation-scatter-world-publication/Docs/Log/Dev.D.UE.0.0.10.P27.23.r0_log.md>
