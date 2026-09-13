# Dev.D.UE.0.0.10.P27.16.r0 Development Log

## 1. 目标

- 把 P27.15 熟练度权威读取约束到一笔具体阵法投材操作；
- 绑定当前部署快照、稳定操作 ID、投材方式与精确阵眼或整部署目标；
- 直接复用 P27.14 能力矩阵，不复制档位规则；
- 让熟练度修订、部署状态或进度变化自动使旧授权过期；
- 保持授权纯函数化，不执行库存、阵眼、World、输入或表现副作用；
- 完成改动驱动回归、双目标构建、Report/Log 与 GitHub 推送。

## 2. 基线与范围

- 基线：`3f40e86af7669e57e377b87355edeea0f83364c8`（P27.15）；
- 分支：`agent/0.0.10-p27-16-formation-mastery-operation-authorization`；
- 起始 tracked tree clean；103 个既有未跟踪用户文件保持未暂存；
- 输入只使用 P27.15 投影结果、P27.14 投材方式与既有 `FShanmenFormationDeployment`；
- 不修改 Profile/schema、熟练度来源、阵法部署实现、库存/物品权威、角色、输入、UI、地图、内容资产或升级规则；
- 不启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、Smoke、Cook 或 Package。

## 3. 新增文件与类型

新增：

- `Source/demo_map/demo_mapShanmenFormationMasteryOperationAuthorization.h`；
- `Source/demo_map/demo_mapShanmenFormationMasteryOperationAuthorization.cpp`；
- `Source/demo_map/demo_mapShanmenFormationMasteryOperationAuthorizationTests.cpp`。

主要类型：

1. `Edemo_mapShanmenFormationMasteryOperationTarget`：区分精确阵眼与整部署目标；
2. `Fdemo_mapShanmenFormationMasteryOperationAuthorization`：不可变、可重算身份的授权证据；
3. `Edemo_mapShanmenFormationMasteryOperationAuthorizationStatus`：完整失败关闭状态；
4. `Fdemo_mapShanmenFormationMasteryOperationAuthorizationResult`：授权状态、诊断与证据；
5. `Fdemo_mapShanmenFormationMasteryOperationAuthorizer`：无状态授权与当前性复核入口。

## 4. 授权前置与状态

`Authorize()` 顺序验证：

1. 熟练度投影必须有效；
2. 部署必须有效且处于 `Deploying`；
3. 部署动作与熟练度读取的玩家和内容必须一致；
4. `OperationId` 必须有效；
5. 投材方式必须是 P27.14 正式枚举且被当前档位允许；
6. 逐阵眼方式必须携带阵眼，挥洒方式必须不携带阵眼；
7. 阵眼必须属于当前部署且尚未提交；
8. 挥洒必须面对零已提交阵眼的全新部署。

拒绝状态包括：`MasteryProjectionRejected`、`DeploymentInvalid`、`DeploymentNotAcceptingMaterials`、`OwnerMismatch`、`ContentMismatch`、`OperationIdentityInvalid`、`DeliveryModeInvalid`、`CapabilityDenied`、`TargetShapeInvalid`、`AnchorUnavailable`、`AnchorAlreadyCommitted` 和 `ScatterRequiresFreshDeployment`。

所有拒绝结果均不携带有效授权，所有成功结果必须通过授权 ID 重算；授权器不修改输入对象，也不内部重试。

## 5. 身份、当前性与自查修正

授权 ID 命名空间为 `demo_map.Formation.MasteryOperationAuthorization.r1`。规范输入包含操作 ID、熟练度读取 ID/修订/档位、Run/Owner/Activation/Deployment/阵图身份、内容戳、投材方式、目标、阵眼定义/实例以及部署状态、回执数、已提交数和总阵眼数。

`IsCurrentAuthorization()` 以当前证据重新执行授权并比较完整不可变结果。熟练度修订、档位、部署回执、提交计数、目标、方式或操作 ID变化均不能重用旧授权。

初版设计允许 Master 在仍为 `Deploying` 且存在未完成阵眼时请求整部署挥洒。对照人工规划中“阵图展开后一次把所需材料送到各阵眼”的定义后，自查确认该语义不能替换一个已经部分提交的逐阵眼过程，因此加入 `ScatterRequiresFreshDeployment` 与 `CommittedAnchorCount == 0` 的身份条件，并补充部分部署负向测试。最终 Editor、专项与完整根组证据均来自修正后的代码。

## 6. 专项自动化

新增四条测试：

1. `DeterministicAnchorAuthorization`：同一逐阵眼操作确定性重放，并绑定熟练度、部署与阵眼实例；
2. `CapabilityAndTargetMatrix`：证明 Beginner/Intermediate/Master 能力差异及阵眼/整部署目标形状；
3. `IdentityAndLifecycleFences`：无效投影、外来玩家、过期内容、无效操作、缺失阵眼和非部署中状态失败关闭；
4. `CurrentDeploymentAndMasterySnapshot`：熟练度修订、阵眼提交使旧授权过期，已提交阵眼与部分部署挥洒被拒绝，其余未提交阵眼可取得新授权。

## 7. 自动化结果

| Group | Success | Fail | SHA-256 |
|---|---:|---:|---|
| `Shanmen.0_0_10.Product.FormationMasteryOperationAuthorization` | 4 | 0 | `DB2125964355C2373C05E6A5C7C2A6A83FCB83E43A3AF41C9F7F60BABD53B727` |
| `Shanmen.0_0_10.Product.FormationMasteryAuthorityAdapter` | 4 | 0 | `94468C4F5F15EDEAB804BE95869135791E76E5CA6B4BC6EA36EEE10FB4CBE312` |
| `Shanmen.0_0_10.CombatRuntime.FormationMastery` | 2 | 0 | `D31EED83214A484AC928183C1582020E7FFDA8E7B361AD4610E3B3735AEE52D7` |
| `Shanmen.0_0_10.CombatRuntime.FormationDeployment` | 4 | 0 | `0C45B791F251D1CC0404E17AFBE9CB2EC283F9ABD57340167B13FCD35A568008` |
| `Shanmen.0_0_10.CombatCore` | 9 | 0 | `391D49F6D4F44D41CC02435D1275E21FE06FC1B2760ACDC8529D021FA4602C00` |
| `Shanmen.0_0_10` | 1,365 | 0 | `603ECB5081A41616794E59FAE0E0500C9F5F03E2349E4A0AA58FB4786933BB11` |

六份最终日志均具备原生退出 0、UE 5.8 `TEST COMPLETE. EXIT CODE: 0`、0 Fail 与 0 Fatal/Unhandled/Ensure。完整根组执行 4,887.039 秒，测试总数从 P27.15 的 1,361 增至 1,365。

## 8. 回归映射

- `Scripts/ShanmenRegressionMap.json` 新增 `FormationMasteryOperationAuthorization` 规则；
- 三个新路径要求完整根组、授权专项、熟练度权威适配器、熟练度策略、阵法部署与 CombatCore 六组证据；
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1` 新增专项 fixture、正向联合映射及“本轮专项不可替代依赖/完整证明”的负向测试；
- JSON 解析：PASS；
- 正反自测：`484/484` PASS；
- 覆盖门：`REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=6 Logs=6`；
- `git diff --check`：PASS。

## 9. 构建、静态边界与 P/F

Editor：

- 5 actions；`SUCCEEDED`；原生退出 0；14.12 秒；
- run-state SHA-256：`7C7EA992CB1A64AC30550C45EA2393B117C44EB0B97BC2ACDA10B053B1EEB25A`；
- stdout SHA-256：`9C7A2DE33D2B2FA88287746ABB2E33E268D431C2132255DE228629204D83BF2B`；
- `UnrealEditor-demo_map.dll`：19,706,880 bytes；SHA-256 `BA843830A7914CEEF7BF9C2C2A2687ED375EECC5C8CEFD1496FBADB1C77EEA3B`。

Game：

- 4 actions；`SUCCEEDED`；原生退出 0；47.61 秒；
- run-state SHA-256：`9CC852E09420A69004DE41BB7B7281F8F19256F5FCC105D543A30F100EB58A6F`；
- stdout SHA-256：`97663052D6E5E19A9BF5BC05284FE5E1FD8EC8E15662FAC175347A2423124648`；
- `demo_map.exe`：360,326,656 bytes；SHA-256 `786E8D6FB08D6FC8D98BC2EA36112DC8F2F5F0CC0D66E0D7136F2ECCA599883D`。

两次 stderr 均为 0 bytes，SHA-256 `E3B0C44298FC1C149AFBF4C8996FB92427AE41E4649B934CA495991B7852B855`。

新增生产代码 466 行非空行，测试 480 行非空行。生产授权代码未引入 World、Actor、控制器、GameMode、输入、计时器、异步、随机、库存/物品权威、Profile 或 SaveGame 依赖。

P 阶段完成编译、无头自动化、静态边界和证据审计。F 阶段未启动 Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package；未执行任何实际投材或阵眼提交。

## 10. 精确提交清单

1. `Source/demo_map/demo_mapShanmenFormationMasteryOperationAuthorization.h`
2. `Source/demo_map/demo_mapShanmenFormationMasteryOperationAuthorization.cpp`
3. `Source/demo_map/demo_mapShanmenFormationMasteryOperationAuthorizationTests.cpp`
4. `Scripts/ShanmenRegressionMap.json`
5. `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`
6. `Docs/Report/Dev.D.UE.0.0.10.P27.16.r0_report.md`
7. `Docs/Log/Dev.D.UE.0.0.10.P27.16.r0_log.md`

`Saved/Codex/P27.16` 与 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P27.16*` 不进入 Git；103 个既有未跟踪用户文件保持未暂存。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p27-16-formation-mastery-operation-authorization>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-16-formation-mastery-operation-authorization/Docs/Report/Dev.D.UE.0.0.10.P27.16.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-16-formation-mastery-operation-authorization/Docs/Log/Dev.D.UE.0.0.10.P27.16.r0_log.md>
