# Dev.D.UE.0.0.10.P27.16.r0 Report

## 1. 结论

P27.16 已在 P27.15 的阵法熟练度权威读取与 P27.14 的能力策略之上，建立无副作用、失败关闭的操作授权层 `Fdemo_mapShanmenFormationMasteryOperationAuthorizer`。

每份有效授权都把熟练度读取、权威修订、运行与玩家身份、阵法激活、部署快照、调用方操作 ID、投材方式及精确目标冻结为一个确定性授权 ID。近身填充与远程投掷只授权一个未提交阵眼；顶阶挥洒投材只授权整个全新部署，任何已有阵眼提交的部署都不能中途切换为“一次成阵”。

专项 4/4、既有熟练度权威 4/4、熟练度策略 2/2、阵法部署 4/4、CombatCore 9/9、完整 `Shanmen.0_0_10` 1,365/1,365、Editor/Game 两目标构建、映射自测与改动驱动覆盖门全部通过。

## 2. 阶段问题与范围

P27.14 只定义三档熟练度的能力矩阵，P27.15 只证明当前玩家、内容版本与权威修订对应哪一档。两者尚未回答：一份熟练度证据是否可以授权当前阵法部署中的这一笔具体操作。

如果由调用方自行拼接这些条件，旧熟练度读取、另一名玩家的读取、旧部署快照、无稳定身份的操作或错误目标形状都可能绕过能力边界。本轮只补齐这条产品接缝：

- 输入必须是 P27.15 已投影的有效熟练度权威结果；
- 部署必须有效、处于 `Deploying` 且仍接受材料；
- 玩家与内容戳必须在熟练度读取和部署动作之间完全一致；
- 调用方必须提供稳定有效的 `OperationId`；
- P27.14 策略必须明确允许请求的投材方式；
- 目标必须与投材方式严格对应。

本轮不选择熟练度最终存档来源，不接输入、距离或动作时机，不预留/扣除物品，不提交阵眼，不执行 World 行为，也不创建缓存、重试、UI 或第二套产品状态。

## 3. 不可变授权与确定性身份

`Fdemo_mapShanmenFormationMasteryOperationAuthorization` 冻结以下证据：

1. 授权 ID、调用方操作 ID、P27.15 熟练度读取 ID；
2. 熟练度权威修订与正式档位；
3. Run、Owner、Activation、Deployment 与阵图定义身份；
4. 内容版本及摘要；
5. 投材方式和目标类型；
6. 阵眼定义与实例身份，或明确的整部署目标；
7. 部署状态、回执数量、已提交阵眼数与总阵眼数。

授权 ID 使用固定命名空间 `demo_map.Formation.MasteryOperationAuthorization.r1` 由全部字段确定性派生。`IsValid()` 会重新计算并比对 ID，而不是只检查 GUID 非空；任何字段被替换、删减或跨上下文拼接都会使授权失效。

完全相同的熟练度与部署证据、操作 ID、方式和目标可重放为同一授权。熟练度修订、部署回执、提交进度、阵眼、操作或方式发生变化都会得到不同授权或直接失败关闭。

## 4. 能力与目标语义

授权器直接复用 P27.14 的唯一能力矩阵，不复制档位判断：

- `ProximityFill`：目标必须是当前部署内一个存在且未提交的阵眼；
- `RemoteThrow`：目标同样是一个存在且未提交的阵眼；
- `ScatterFormation`：目标必须是整个部署，不能携带阵眼定义或实例。

专项矩阵证明 Beginner 不可远投、Intermediate 可远投但不可挥洒、Master 可挥洒。规划文本把顶阶操作定义为阵图展开后一次把所需材料送到各阵眼，因此本轮自查把挥洒进一步限定为 `CommittedAnchorCount == 0`。一旦任一阵眼已提交，授权返回 `ScatterRequiresFreshDeployment`，不能用整部署操作替换部分完成的逐阵眼流程。

## 5. 生命周期与当前性

`Authorize()` 对每次请求执行一次有界纯函数检查，拒绝状态覆盖：无效熟练度投影、无效或非部署中状态、玩家/内容不一致、无效操作身份、无效方式、能力不足、目标形状错误、阵眼不存在、阵眼已提交，以及非全新部署的挥洒请求。

成功授权不会修改部署或任何权威。`IsCurrentAuthorization()` 使用当前熟练度投影、当前部署与同一请求重新生成授权并比较完整证据，所以：

- 熟练度权威修订变化会使旧授权过期；
- 任一阵眼提交导致回执和进度变化，旧授权随即过期；
- 已提交阵眼不能再次取得授权；
- 部分完成后，仍可为其他未提交阵眼取得新的逐阵眼授权，但不能取得挥洒授权。

## 6. 自动化证明

| Group | Success | Fail | Log SHA-256 |
|---|---:|---:|---|
| `Shanmen.0_0_10.Product.FormationMasteryOperationAuthorization` | 4 | 0 | `DB2125964355C2373C05E6A5C7C2A6A83FCB83E43A3AF41C9F7F60BABD53B727` |
| `Shanmen.0_0_10.Product.FormationMasteryAuthorityAdapter` | 4 | 0 | `94468C4F5F15EDEAB804BE95869135791E76E5CA6B4BC6EA36EEE10FB4CBE312` |
| `Shanmen.0_0_10.CombatRuntime.FormationMastery` | 2 | 0 | `D31EED83214A484AC928183C1582020E7FFDA8E7B361AD4610E3B3735AEE52D7` |
| `Shanmen.0_0_10.CombatRuntime.FormationDeployment` | 4 | 0 | `0C45B791F251D1CC0404E17AFBE9CB2EC283F9ABD57340167B13FCD35A568008` |
| `Shanmen.0_0_10.CombatCore` | 9 | 0 | `391D49F6D4F44D41CC02435D1275E21FE06FC1B2760ACDC8529D021FA4602C00` |
| `Shanmen.0_0_10` | 1,365 | 0 | `603ECB5081A41616794E59FAE0E0500C9F5F03E2349E4A0AA58FB4786933BB11` |

六份最终日志均具有 UE 5.8 `TEST COMPLETE. EXIT CODE: 0`、测试进程原生退出 0、0 Fail 与 0 Fatal/Unhandled/Ensure。完整根组从 00:29:06.884 UTC 运行至 01:50:33.923 UTC，共 4,887.039 秒。

专项四条分别覆盖：确定性阵眼授权、档位能力与目标矩阵、身份及部署生命周期围栏、熟练度修订与部署进度导致的当前性失效。完整根组比 P27.15 增加且仅增加这 4 条，从 1,361 增至 1,365。

## 7. 改动驱动回归

新增 `FormationMasteryOperationAuthorization` 映射规则。三个授权路径必须同时具有完整根组、授权专项、P27.15 权威适配器、P27.14 熟练度策略、阵法部署与 CombatCore 证据。

- regression map JSON：PASS；
- 映射器正反自测：`484/484` PASS；
- 负向自测证明只有本轮专项不能替代五个依赖/完整组；
- 最终覆盖门：`REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=6 Logs=6`；
- `git diff --check`：PASS。

六份日志覆盖全部必跑组，覆盖工具记录的 SHA-256 与本 Report 第 6 节一致。

## 8. 构建与静态边界

- Editor：5 actions，`SUCCEEDED`，原生退出 0，14.12 秒；run-state SHA-256 `7C7EA992CB1A64AC30550C45EA2393B117C44EB0B97BC2ACDA10B053B1EEB25A`；stdout SHA-256 `9C7A2DE33D2B2FA88287746ABB2E33E268D431C2132255DE228629204D83BF2B`；
- `UnrealEditor-demo_map.dll`：19,706,880 bytes，SHA-256 `BA843830A7914CEEF7BF9C2C2A2687ED375EECC5C8CEFD1496FBADB1C77EEA3B`；
- Game：4 actions，`SUCCEEDED`，原生退出 0，47.61 秒；run-state SHA-256 `9CC852E09420A69004DE41BB7B7281F8F19256F5FCC105D543A30F100EB58A6F`；stdout SHA-256 `97663052D6E5E19A9BF5BC05284FE5E1FD8EC8E15662FAC175347A2423124648`；
- `demo_map.exe`：360,326,656 bytes，SHA-256 `786E8D6FB08D6FC8D98BC2EA36112DC8F2F5F0CC0D66E0D7136F2ECCA599883D`；
- 两次 stderr 均为空，SHA-256 `E3B0C44298FC1C149AFBF4C8996FB92427AE41E4649B934CA495991B7852B855`。

新增生产代码 466 行非空行，测试 480 行非空行。生产授权代码静态扫描未发现 `UWorld`、`AActor`、PlayerController、GameMode、InputAction、Timer、Async、随机 GUID/RNG、Inventory/ItemAuthority、SaveGame 或 Profile 依赖。

## 9. P/F 边界与下一阶段

P 阶段已证明权威与部署身份绑定、档位能力矩阵、精确目标形状、确定性重放、字段篡改失效、熟练度修订失效、部署进度失效、已提交阵眼围栏、部分部署挥洒围栏、完整回归、覆盖门及双目标构建。

F 阶段未执行：没有启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、Cook 或 Package。本轮不声明玩家能够在游戏中实际远投或挥洒材料，也不声明材料已预留、扣除、飞行或提交。

下一独立阶段可在本授权之下定义“整部署挥洒”的确定性批量命令/意图，使一笔 Master 操作明确映射到全部阵眼需求；仍应保持命令生成与真实库存扣除、World 表现分离。熟练度最终数据源及升级时机仍需正式产品决定，不能由授权层自行创造。

## 10. GitHub 交接

基线提交：`3f40e86af7669e57e377b87355edeea0f83364c8`（P27.15）。分支：`agent/0.0.10-p27-16-formation-mastery-operation-authorization`。本阶段只提交 3 个新增源/测试文件、2 个回归映射文件、本 Report 与本 Development Log；103 个既有未跟踪用户文件保持未暂存，`Saved/Codex/P27.16` 与构建证据不进入 Git。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p27-16-formation-mastery-operation-authorization>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-16-formation-mastery-operation-authorization/Docs/Report/Dev.D.UE.0.0.10.P27.16.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-16-formation-mastery-operation-authorization/Docs/Log/Dev.D.UE.0.0.10.P27.16.r0_log.md>
