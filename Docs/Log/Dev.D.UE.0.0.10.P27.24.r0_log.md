# Dev.D.UE.0.0.10.P27.24.r0 Development Log

## 1. 目标

- 消费一份完整、有效的 P27.22 World Placement Handoff Evidence；
- 建立绑定 Handoff、Actor 类和单一 World 的产品 Session；
- 让 Session 持有并恢复 P27.23 Publication Ledger 与 Completion Evidence；
- 保持首次发布、前缀恢复和完整重放都只经 P27.23 Publisher；
- 保持 Spawn、标签接管、冲突检测与 Destroy 都只经既有 World Adapter；
- 为 `Cancelled` / `Ended` 增加显式、幂等、可重建恢复的整批 Actor 清理；
- 让旧 ProductSession 与新批次 Session 共用一个终止清理实现；
- 证明跨 World、终态改写、终态后发布和无效输入失败关闭；
- 通过真实无头瞬态 World/Actor 自动化、改动驱动回归、双目标构建、Report/Log 和 GitHub 推送完成交接。

## 2. 基线与边界

- 基线：`61c5c8035d39880a50a1ab5e4d57eed26d44b787`（P27.23）；
- 分支：`agent/0.0.10-p27-24-formation-scatter-world-publication-session`；
- 起始 tracked tree clean；
- 103 个既有未跟踪用户文件保持原样且不暂存；
- 输入为完整 P27.22 Handoff Evidence 与一个可生成的具体 Actor 类；
- Session 的第一次有效 World 操作固定 World 所有权；
- 源资源与 Deployment 事实不可回滚、不可重消耗；
- 允许 UnrealEditor-Cmd 无头瞬态 World/Actor 测试；
- 禁止 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 和 Package；
- 不修改地图、内容资产、Profile/schema、物品 Authority 或既有 Deployment 事实。

## 3. 新增和修改文件

新增生产文件：

1. `Source/demo_map/demo_mapShanmenFormationScatterWorldPublicationSession.h`
2. `Source/demo_map/demo_mapShanmenFormationScatterWorldPublicationSession.cpp`

修改：

3. `Source/demo_map/demo_mapShanmenFormationWorldAdapter.h`
4. `Source/demo_map/demo_mapShanmenFormationWorldAdapter.cpp`
5. `Source/demo_map/demo_mapShanmenFormationScatterResourcePreparationTests.cpp`
6. `Scripts/ShanmenRegressionMap.json`
7. `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`

交接：

8. `Docs/Report/Dev.D.UE.0.0.10.P27.24.r0_report.md`
9. `Docs/Log/Dev.D.UE.0.0.10.P27.24.r0_log.md`

新 Session 头/实现共 477 行、444 个非空生产代码行。Adapter 修改为 34 行新增、5 行删除；测试宿主新增 329 行；流程映射和自检新增 61 行。

## 4. 实现过程

### 4.1 Session 身份与状态

新增 Session 使用命名空间 `demo_map.Formation.ScatterWorldPublicationSession.r1`，由 P27.22 Evidence ID、P27.21 Deployment ID 和 Actor Class Path 确定性派生 Session ID。

状态只前进：`Empty → Ready → Publishing/Published → Cancelled/Ended`。`Ready` 只允许空 Ledger；`Publishing` 必须持有非空且未完整的规范前缀；`Published` 必须持有完整 Ledger 和匹配的 Completion Evidence；终态必须持有有效 World Adapter 与 Teardown Receipt。

### 4.2 World 发布所有权

`TryPublish` 验证 Session、终态、World 和既有绑定后，构造 P27.23 的具体 Adapter Publication Port。Session 不读取或修改资源 Authority，也不重建发布算法。

Publisher 结果映射为 Session 的 `Published`、`Recovered`、`Replayed` 或 `PublicationRejected`。失败后的状态由实际 Ledger 决定；完整发布后的失败重放不会擦除已经成立的 Completion Evidence。

### 4.3 通用终止请求

在既有 World Adapter 中新增 `Fdemo_mapShanmenFormationWorldTeardownRequest` 和 `TryTeardownDeployment`。请求绑定 Deployment ID、终态和规范已提交阵眼数。

旧 `TryTeardownTerminal(ProductSession)` 继续验证旧 Session 为终态，再构造同一通用请求。实际 Actor 收集、Deployment 标签恢复、销毁、残留检查、回执派生和重放全部只有一个实现。

### 4.4 重建与直接恢复

相同 Handoff 与 Actor 类重建得到相同 Session ID。新 Adapter 在发布重放时根据既有 Placement/Deployment 标签接管 Actor，而不是重复 Spawn。

若本地 Publication Ledger 丢失，重建 Session 可直接发出终止请求；Adapter 扫描稳定 Deployment 标签并移除该批 Actor。Teardown Receipt 的 `CommittedAnchorCount` 来自完整、已验证 Handoff，`RemovedActorCount` 是 World 观察结果。此操作不改变 P27.20/P27.21 的权威事实。

## 5. 专项测试与自查

新增四条专项：

1. 首次发布、完整重放、`Ended` 清理和终止重放；
2. Host 重建接管同一 Actor 集合与无 Ledger 直接 `Cancelled` 清理；
3. 跨 World 发布/清理、终态改写与终态后发布冲突；
4. 无效 Evidence、Actor 类、World 与非终态清理输入。

所有测试使用真实无头瞬态 `UWorld` 与 `ACharacter::StaticClass()`。生命周期测试同时核对资源快照、Deployment `Active` 状态和既有 Deployment Receipt 数保持不变。

初次专项执行本身为 4/4 Success，但命令只把启动器的 SDK 检查输出写入阶段日志；完整 UE 测试细节仍在默认 `Saved/Logs/demo_map.log`。该文件未被接受为交付证据。随后增加 `-stdout -FullStdOutLogOutput`，重新执行同一专项并保存独立完整日志：4 Success、0 Fail、原生退出 0，SHA-256 `C1C9ACA6F8053BA173C84059CFD6BBEA4552F7C45B6D0836ABF075E24F1220A0`。

没有产品代码或测试逻辑失败。本阶段的唯一过程修正是日志捕获通道；未把不完整启动器日志伪装成自动化证据。

## 6. 改动驱动专项回归

| Group | Success | Fail | SHA-256 |
|---|---:|---:|---|
| FormationScatterWorldPublicationSession | 4 | 0 | C1C9ACA6F8053BA173C84059CFD6BBEA4552F7C45B6D0836ABF075E24F1220A0 |
| FormationScatterWorldPublication | 8 | 0 | 32099CAFAEC8A368E8052FD229A1766D207AA744BDAAD50038F30C34F66F2E2C |
| FormationScatterWorldPlacementHandoff | 4 | 0 | 71724B34DC0294FEEC71957C9594467A8CB34059D2553595D5A2E5657FAA0B38 |
| FormationWorldDelivery | 4 | 0 | 45ECB5C7559A608F84E5B4FE42BAF4527A4A43F3D2BBCD7B6144F9E9D1F62B70 |
| FormationScatterDeploymentCommit | 5 | 0 | 9D14F048CEED266C310B62E2E5357D854464BFAE3EE36AEB08C810628006992C |
| FormationScatterResourceCommit | 5 | 0 | 86FB8004E13D3A05EA30F42816E360D7F83AD917459E8A84C7890BEA266C0134 |
| FormationScatterResourcePreparation | 5 | 0 | 0C5A0F3BF2536E561AD40D4367FE02DE8EF34F4B3062183039DDDD2B07542CEB |
| FormationScatterResourcePlan | 4 | 0 | DA7472397CF3567AF660707E0C651CF8814E365A2D579FDAD4167E461E1976F6 |
| FormationScatterBatchIntent | 4 | 0 | C05ED3488D5DBDA7B0F1C9770B468B2FDCFFC705428D4ABEA122D48119FD85A0 |
| FormationMasteryOperationAuthorization | 4 | 0 | E7A445AE27DF666C724122838A36069A6AF476ABAEDC150A39AAF19C5D118135 |
| FormationMasteryAuthorityAdapter | 4 | 0 | D75148E4C99142726CBB24EE91A156F9EC12A495B897F0E2634E5E0A76CB71E5 |
| FormationMaterialAdapter | 4 | 0 | B810A2605E387B86E7B60C18B8A9B17F5768847C58EDC47F7C5EB35BCE7B7774 |
| FormationSession | 4 | 0 | 70C6500EB119120400233E2D720B5E36A7C5E57669F8B276BB6CE7AACF26CEE7 |
| Items | 77 | 0 | 9618DBCA7620AD23A1064EBD61810B8AFCDBCE1E1FFF36F48101C0DCAF3CBB20 |
| WorldGameplay | 10 | 0 | 3BA33AA16CD8EF84578ADC64D04EB377489FBD392C5C1DE37AA9CD1A6C9D26DB |
| FormationMastery | 2 | 0 | 72DBA595BDA8935436B862A73E08434E4ECD961F7C7143FA80A9E76E44E492D3 |
| FormationDeployment | 4 | 0 | 41944CF1C368D861DD15E238A5CAB06115619CE529C916A54C4F0A9849309FF4 |
| CombatCore | 9 | 0 | 4496AEBFF5D2318B19D3C82B7C95A63889381F7E8D7D7B2CD11301C9DF9E94EF |
| FormationInfluenceLifecycleCommandHost | 5 | 0 | EEBF1B14AF2F8A81BB418A1C51CF9CA06E441C2FE646AC563974ECCB52088019 |
| FormationInfluenceLifecycleCommandRouter | 4 | 0 | C71B1D44509FCFF030B43E417824C25A6E7B38DF825CEE1DD536F582C5B95544 |
| FormationInfluenceLifecycleCoordinator | 4 | 0 | 5A6180AC39CBEEAB63374523270D01472C7A055AF4FB0D217E8F3862541D803A |
| Shanmen.0_0_10 full | 1,400 | 0 | A9A55E70D1C99ACB7AF8C56281A6AEBF81446000F37F4D4FA7FFE1A0C52897F9 |

`FormationScatterWorldPublication` 的命名空间同时包含 P27.23 原四项和新增 Session 四项，因此该焦点日志为 8 项；独立 Session 日志仍只运行新增 4 项。全部 22 份日志各含一个精确 `RunTests` 命令、至少一个 Success、0 Fail、原生成功终止标记，并通过覆盖脚本的 Fatal/Unhandled/Ensure 检查。

完整根组首项于 2026-09-13 18:03:41.447 UTC 开始，末项于 19:11:16.240 UTC 完成，持续 1 小时 7 分 34.793 秒；1,400 项全部成功，原生退出 0。

## 7. 回归映射

新增 `FormationScatterWorldPublicationSession` 路径规则，覆盖新 Session 文件及共享测试宿主。规则要求 Session、P27.23 Publication、P27.22 Handoff、World Delivery、P27.21/P27.20 Deployment 与资源链、Formation 权威/Session、Items、WorldGameplay、Formation Mastery/Deployment、CombatCore 和完整根组。

共享测试宿主同时命中 P27.22、P27.23 与 P27.24 规则；World Adapter 改动命中既有 World Delivery 规则并要求三个 Influence Lifecycle 终止消费者组。所有规则取并集，最终为 5 条匹配规则、22 个唯一必跑组。

Self-test 增加：

- 正向夹具：完整 19 组 Session 依赖证据可覆盖新 Session 文件；
- 负向夹具：只有 Session 专项不能替代 Publication、Handoff、资源、World 与完整根组。

结果：

- `SELF_TEST: PASS 500/500`；
- Self-test SHA-256：`8114748657B53F0A856AE6814934FC490BFA1A182144E5E248869F56720FCBDA`；
- `REGRESSION_COVERAGE: PASS Changed=9 Rules=5 Required=22 Logs=22`；
- Coverage SHA-256：`5AE82AB75058C68A22AFB7B7B81FF3B5045050EC8F0F7BD60E1454609DF83D94`。

## 8. 构建、产物与卫生

Editor：

- 最终 target up to date，0 actions；
- Result Succeeded，原生退出 0，总计 4.12 秒；
- 日志 SHA-256 `B83F4F6D89FFA0A41812A7F5EE4511551B02A2268DB13030B870B2FB68BB247F`；
- `UnrealEditor-demo_map.dll` 20,093,952 bytes；
- DLL SHA-256 `C70852C327FDAADC88E976F157F0A15DC25A8F1EA4C9ABC96B9DE54C5AFD2C5A`。

Game：

- 新源文件触发 89 actions；
- UBA 78.68 秒，总计 81.56 秒；
- Result Succeeded，原生退出 0；
- 日志 SHA-256 `17BD1778E6B0B204B5D3274F03A9589E711D12D73FF63D3160B007BA966276FD`；
- `demo_map.exe` 360,658,432 bytes；
- EXE SHA-256 `3007A87A22A7C9F8C964F93C3C7819083239146FB2498BDE69C2691A27E2A90E`。

静态与 Git 卫生：

- 新 Session 生产文件未发现 Inventory、Profile、SaveGame、Tick/Timer、异步、随机 GUID、RNG 或直接 Spawn/Destroy Actor；
- Adapter 新增行未增加上述边界，真实 Actor 变更继续只有既有 Adapter 实现；
- Regression Map JSON 解析 PASS；
- `git diff --check` PASS；
- 最终暂存差异检查 PASS；
- 最终暂存精确为 9 个阶段实现、测试、映射和交接文件；
- 103 个既有未跟踪用户文件保持原样且未暂存。

## 9. P/F 边界

P 阶段完成：P27.22 Handoff → 产品 Session → P27.23 Ledger/Completion → 单 World 绑定 → Host 重建接管 → `Cancelled`/`Ended` Actor 清理 → 稳定终止回执。真实无头瞬态 World 中首次生成 2 个 Actor、重建不重复生成、终止移除 2 个 Actor。

F 阶段未执行：没有 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、截图、Smoke、Cook 或 Package；没有正式地图视觉验收；没有把 Session 接到玩家可触发的 Product Host/Command Route；没有建立全局活动 Session Registry。

## 10. 下一阶段与 GitHub

建议 P27.25 建立唯一 Formation Scatter World Publication Command/Host，负责玩家产品入口、Run/Deployment 绑定、一个活动 Session 的所有权、Host 重建替换顺序和终态命令路由。继续禁止第二套 Actor 真值、资源回滚和 Deployment 重提交。

- Branch: <https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p27-24-formation-scatter-world-publication-session>
- Report: <https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-24-formation-scatter-world-publication-session/Docs/Report/Dev.D.UE.0.0.10.P27.24.r0_report.md>
- Development Log: <https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-24-formation-scatter-world-publication-session/Docs/Log/Dev.D.UE.0.0.10.P27.24.r0_log.md>
