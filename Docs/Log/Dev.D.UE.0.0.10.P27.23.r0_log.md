# Dev.D.UE.0.0.10.P27.23.r0 Development Log

## 1. 目标

- 消费 P27.22 完整 World Placement Handoff Evidence；
- 定义可注入的整批 World Publication Port；
- 复用既有 Formation World Adapter，不复制 Actor 发布逻辑；
- 为每个成功阵眼生成绑定完整身份链的不可变 Publication Record；
- 建立只前进、只能保存规范成功前缀的 Receipt Ledger；
- 中途失败保留成功前缀，重试验证前缀后补齐后缀；
- 完整重放不重复生成 Actor、不增加账本；
- 外来 Evidence、Actor 类、Receipt 或前缀漂移全部失败关闭；
- 保持 P27.20 资源和 P27.21 Deployment 已提交事实不可回滚、不可重消耗；
- 通过真实无头瞬态 World/Actor 自动化、改动驱动回归、双目标构建、Report/Log 和 GitHub 推送完成交接。

## 2. 基线与边界

- 基线：`d19d04986976f6d253d5da1ff3815237b961ff44`（P27.22）；
- 分支：`agent/0.0.10-p27-23-formation-scatter-world-publication`；
- 起始 tracked tree clean；
- 103 个既有未跟踪用户文件保持原样且未暂存；
- 输入为完整有效的 `Fdemo_mapShanmenFormationScatterWorldPlacementHandoffEvidence`；
- 输出为逐阵眼 Publication Records、只前进 Ledger 和完整 Publication Evidence；
- 允许 UnrealEditor-Cmd 无头瞬态 World/Actor 测试；
- 不启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package；
- 不修改地图、内容资产、Profile/schema、物品 Authority 或既有 Deployment 事实。

## 3. 新增和修改文件

新增：

1. `Source/demo_map/demo_mapShanmenFormationScatterWorldPublication.h`
2. `Source/demo_map/demo_mapShanmenFormationScatterWorldPublication.cpp`

修改：

3. `Source/demo_map/demo_mapShanmenFormationWorldAdapter.h`
4. `Source/demo_map/demo_mapShanmenFormationWorldAdapter.cpp`
5. `Source/demo_map/demo_mapShanmenFormationScatterResourcePreparationTests.cpp`
6. `Scripts/ShanmenRegressionMap.json`
7. `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`

交接：

8. `Docs/Report/Dev.D.UE.0.0.10.P27.23.r0_report.md`
9. `Docs/Log/Dev.D.UE.0.0.10.P27.23.r0_log.md`

阶段差异在 Report/Log 前为 7 个文件、1,260 行新增；新增 Publication 头/实现共 696 行非空生产代码。

## 4. 实现过程

### 4.1 既有 Adapter 窄入口

在 `Fdemo_mapShanmenFormationWorldAdapter` 增加 `TryPlaceIntent`。它接收一个已经构建和自验证的 Placement Intent，并沿用既有实现完成：

- World 与具体 Actor 类验证；
- Adapter 的 World/Deployment 单一所有权；
- 确定性 Placement Receipt；
- Placement ID 冲突与 Actor Class 冲突；
- 现有标签 Actor 的接管或精确重放；
- 新 Actor 的生成、Placement/Deployment 标签与后置条件；
- 已记录 Actor 缺失、重复标签 Actor 和 teardown 后调用的失败关闭。

旧 `TryPlaceCommittedAnchor` 在完成 ProductSession 审计与意图构建后委托给该入口。整批路径没有复制上述逻辑。

### 4.2 Port 与发布器

新增 `Idemo_mapShanmenFormationScatterWorldPlacementPort` 作为可注入边界；具体 Port 只封装 World、Actor 类和既有 Adapter，并把 Intent 委托给 `TryPlaceIntent`。

Publisher 的固定顺序为：验证源证据 → 验证 Actor 类 → 验证 Ledger 绑定与规范前缀 → 按顺序调用 Port → 验证既有 Placement Receipt → 验证/追加 Publication Record → 构建完整 Evidence。

### 4.3 身份与账本

单阵眼 Record 同时绑定 P27.22 Source Handoff ID 和 P27.21 Deployment Handoff ID。后者必须等于 Receipt Intent 的 Attempt ID；前者保持本阶段源记录身份。Record、Ledger、Completion Evidence 分别使用独立的确定性命名空间。

非空 Ledger 绑定精确 P27.22 Evidence、Deployment 和 Actor Class Path。每条 Record 必须与同索引 Handoff 完全一致，Anchor Order 必须连续，Record ID 和 Placement ID 必须唯一。因此任何有效 Ledger 都只能是源批次的规范成功前缀。

### 4.4 恢复与重放

- 空 Ledger 完成后返回 `Published`；
- 已有前缀补齐后返回 `Recovered`；
- 完整账本精确重放后返回 `Replayed`；
- Port 拒绝时保留本轮此前已验证并追加的成功 Records；
- 重试仍将已有前缀交给 Port，但既有 Adapter 只能返回接管/精确重放，不会重复 Spawn；
- 旧前缀 Receipt 任一字段漂移即 `LedgerConflict`；
- 只有完整账本才生成有效 Publication Evidence。

## 5. 失败、自查与修正记录

### 5.1 初次专项运行

结果：1 Success、2 Fail，随后数组越界断言终止；日志 SHA-256：`3B6EDC4CC2952768053E8A35B01BA4971FC470274C2BD289B388C8B1CD989E44`。

根因：初版把 P27.22 单阵眼 Handoff ID 与其携带的 P27.21 Deployment Handoff ID 当成同一身份，并错误地用前者验证 Placement Intent Attempt ID。合法 World Receipt 因此被拒绝；恢复测试随后错误索引了未建立的 Ledger 前缀。

修正：

1. Record 同时保存 Source Handoff ID 与 Deployment Handoff ID；
2. Attempt ID 只与 P27.21 Deployment Handoff ID 对齐；
3. 两级身份都进入确定性 Record ID 与兼容性检查；
4. 测试先验证前缀建立成功，否则立即返回，消除二次越界。

### 5.2 第二次专项运行

结果：3 Success、1 Fail；日志 SHA-256：`E54A10910BACDDCF274273328688E25CDABBB6B1026E9D9EC0A0B88D24BC7BCE`。

身份修正后所有纯端口/账本测试通过，真实 World 测试仍失败。原因是基础 `AActor::StaticClass()` 没有为该夹具提供可保持规范位置的具体空间根，无法满足既有 Adapter 的 Actor 位置后置条件。修正测试夹具为具有具体空间根的 `ACharacter::StaticClass()`；没有削弱生产校验。

### 5.3 最终专项运行

结果：4 Success、0 Fail、0 Fatal/Unhandled/Ensure、原生退出 0；日志 SHA-256：`79D971720B78C800012C2E358072181E551E988BAD557D7821705901E92DF682`。

## 6. 专项与改动驱动回归

| Group | Success | Fail | SHA-256 |
|---|---:|---:|---|
| FormationScatterWorldPublication | 4 | 0 | 79D971720B78C800012C2E358072181E551E988BAD557D7821705901E92DF682 |
| FormationScatterWorldPlacementHandoff | 4 | 0 | 58E18B35D0B14727C4021DB85D9B1AE2FE492A640765C53A3F7C8B676D43F410 |
| FormationWorldDelivery | 4 | 0 | 1565A6008F35696F7D7017A14F7F702CC049958DD735E61BBE2B4BA9330F2907 |
| FormationScatterDeploymentCommit | 5 | 0 | 18FD9584B1E06884545F6ED4C17DEF5B23364AC8BE69FFC811A0C49938A80CB6 |
| FormationScatterResourceCommit | 5 | 0 | 9D47EF23789C60471C29821F92B6599933B423813E53F1C4C31CF1DC83135E3D |
| FormationScatterResourcePreparation | 5 | 0 | 7547CB09B11463C4BCF578F16E9929A2533AD7D17E024B50CB06AB331DF43FA9 |
| FormationScatterResourcePlan | 4 | 0 | 59023A47896AB1D66CA86499C68A847942635A7F85026FD46F50CB3ACEB5795F |
| FormationScatterBatchIntent | 4 | 0 | BD3C977B9278EA55A38F1E6FF4DECE6604610E110D09B7D9C8F521732AC1E2C6 |
| FormationMasteryOperationAuthorization | 4 | 0 | 721246A7395D061CDBFC9EFC9C4D16F0EA83A064FB9367C2AF0D0C92140130D6 |
| FormationMasteryAuthorityAdapter | 4 | 0 | 583F599A37FEAE5CAAE3DB89DB2318524E9CF90A56939A3D9D02CCA63445BA5A |
| FormationMaterialAdapter | 4 | 0 | D22EF0855B55F70542269678EE15F32B24CB8B47A9C23D63107BA4ACA79D07E7 |
| FormationSession | 4 | 0 | 7D3E7380ECAE1CD1E3FA0207BA853DA412B7C38D45BA39AF3EDFE7538C485765 |
| Items | 77 | 0 | 35DFB9314449A0E1F6B5B5C7B726EDC2E3C0FF6664705609FEF8DEC1E15176AB |
| WorldGameplay | 10 | 0 | 7483A9E550A78E2D0AB6E44458EB00FFCD68FB62BE433E591F01D58BE8C34360 |
| FormationMastery | 2 | 0 | 019CB5C63D6B1EFBB2E3F98E272AA84AEB54911DEF18D4F2CC22FAEDED1FF719 |
| FormationDeployment | 4 | 0 | 13A6ACCC42EBBD217E09359E4F1609475F96713F07D8C8B6B89B412E3CFC6CFE |
| CombatCore | 9 | 0 | 5CA63AE7E5F3E2F1DB06CE377C170D52C35AFCC498C2B1DC80D303CBC18F9B81 |
| FormationInfluenceLifecycleCommandHost | 5 | 0 | D728F0BE4F41F140060DC69415C5FBAC8311DBDE924C53D7C9647006844B5EE2 |
| FormationInfluenceLifecycleCommandRouter | 4 | 0 | 7E9770F883E1A2C52A7FBD439035E6721703EA7F5B6E35DED98176B17A952D72 |
| FormationInfluenceLifecycleCoordinator | 4 | 0 | BC224B05C3935C6E13F997055027258646AC5BF2772EA8F9B609EF4AA9552FA5 |
| Shanmen.0_0_10 full | 1,396 | 0 | 817BDE96CF7E336F0FE2FB61FBB342F7944787C4F538CC168D7A10BEE1D96D2B |

完整根组第一项于 2026-09-13 15:51:36.887 UTC 开始，最后一项于 17:01:19.816 UTC 完成，持续 1 小时 9 分 42.929 秒；1,396 项全部 Success，0 Fail、0 Fatal/Unhandled/Ensure，原生退出 0。

## 7. 回归映射

新增 `FormationScatterWorldPublication` 映射规则，覆盖新 Publication 文件及其测试宿主。必跑组包含 Publication 专项、P27.22 Handoff、World Delivery、P27.21/P27.20 完整部署和资源链、Formation 权威/会话、Items、WorldGameplay、CombatCore 与完整根组。

Self-test 增加：

- 一条正向夹具，证明 18 个规则内依赖组加完整根组可以覆盖 Publication 改动；
- 一条负向夹具，证明只有 Publication 专项不能替代下游/上游契约证据。

结果：

- `SELF_TEST: PASS 498/498`；
- Self-test SHA-256：`1291331CC66DE8CB8E3CA75249A88E98FD23B376FE53A3355A32405ED586675D`；
- `REGRESSION_COVERAGE: PASS Changed=9 Rules=4 Required=21 Logs=21`；
- Coverage SHA-256：`23AA858DE65BD37BDE5B2127525CF0C35AD3F9EFFF979379C31BD7705C384DC0`。

## 8. 构建、产物和卫生

Editor：

- 最终复核 target up to date；
- 0 actions，Result Succeeded，原生退出 0，总计 2.23 秒；
- 日志 SHA-256 `7C42DD141CC57C49DF60B29B5F6D983EEDCAA858D61798D04A96B162C7DC0096`；
- `UnrealEditor-demo_map.dll` 20,056,064 bytes；
- DLL SHA-256 `9053006F55E4775AC55BC40C7180982ED0A810CF03F0265CF2AAFA8A0078FA0B`。

Game：

- 新源文件触发 88 actions；
- UBA 43.19 秒，总计 45.01 秒；
- Result Succeeded，原生退出 0；
- 日志 SHA-256 `5509D004F7E0DC7B5C686DD81626C290BCF61BF8660395F4BA8254211CCF9177`；
- `demo_map.exe` 360,627,712 bytes；
- EXE SHA-256 `9B5C88EE69EC9FA1F0C9BD6419B6CF03BD93916A8518466E7997CF18438F830B`。

静态与 Git 卫生：

- Publication 生产文件未发现物品 Authority、Inventory、Profile、SaveGame、Timer/Tick、异步、随机 GUID、RNG 或直接 Spawn/Destroy Actor；
- 真实 World/Actor 能力只通过既有 Adapter Port 使用；
- Regression Map JSON 解析 PASS；
- `git diff --check` PASS；
- 暂存差异检查 PASS；
- Report/Log 写入前暂存精确为 7 个阶段实现、测试和映射文件；
- 103 个既有未跟踪用户文件保持原样且未暂存。

## 9. P/F 边界

P 阶段完成：P27.22 Evidence → 可注入 Publication Port → 规范逐阵眼 World Receipt → 只前进 Ledger → 完整 Publication Evidence。真实无头瞬态 World 中首次生成 2 个具体 Actor，完整重放仍为 2 个；部分失败只保留成功前缀，重试完成后缀；资源和 Deployment 快照不变。

F 阶段未执行：没有 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package；没有把该能力接到玩家可触发的 Product Host/Command Route；没有正式地图中的视觉验收。

## 10. 下一阶段与 GitHub

建议 P27.24 建立 Product-owned World Publication Session/Command Route，并补齐：Run/Deployment/World 所有权、Host 重建接管、终态 Actor teardown、异常后 reconciliation 和幂等终止回执。P27.20 资源与 P27.21 Deployment 仍只前进，所有 World 操作继续只经既有 Adapter。

- Branch: <https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p27-23-formation-scatter-world-publication>
- Report: <https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-23-formation-scatter-world-publication/Docs/Report/Dev.D.UE.0.0.10.P27.23.r0_report.md>
- Development Log: <https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-23-formation-scatter-world-publication/Docs/Log/Dev.D.UE.0.0.10.P27.23.r0_log.md>
