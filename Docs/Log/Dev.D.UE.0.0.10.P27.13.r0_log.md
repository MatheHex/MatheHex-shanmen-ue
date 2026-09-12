# Dev.D.UE.0.0.10.P27.13.r0 Development Log

## 1. 目标

- 将 P27.4 锚点输入适配器绑定到真实 ItemAuthority、CombatRun、FormationRunLifecycle、显式 World 与显式 ActorClass；
- 删除调用方注入 RunId 或自由生命周期回调的产品级绕行面；
- 保持 gameplay gate 早于依赖检查、锚点采样与生命周期调用；
- 修复已提交材料 AttemptId 可被改绑到另一锚点的一致性缺口；
- 证明依赖拒绝零副作用、首次放置、精确重放、绑定漂移失败关闭、正常完成与唯一生命周期清理；
- 完成改动驱动回归、双目标构建、Report/Log 与 GitHub 推送。

## 2. 基线与范围

- 基线：`d05674651faa835646c8c400df8958bdc65340f2`（P27.12）；
- 分支：`agent/0.0.10-p27-13-formation-anchor-product-route`；
- 起始 tracked tree clean；103 个既有未跟踪用户文件保持未暂存；
- 不修改 Profile/schema、正式阵图内容、输入资产、键位、UI、PlayerController、GameMode、地图或内容资产；
- 不启动 Unreal Editor UI、PIE、Standalone、产品可执行文件、真实输入、Smoke、Cook 或 Package。

## 3. 新增具体产品入口

新增：

- `Source/demo_map/demo_mapShanmenFormationAnchorProductRoute.h`；
- `Source/demo_map/demo_mapShanmenFormationAnchorProductRoute.cpp`。

`RouteAnchor()` 接收五个现有或显式产品依赖，不接收 RunId 或生命周期函数。内部回调固定调用：

`Lifecycle.TryExecuteAnchorOperation(Authority, Coordinator, World, ActorClass, Operation)`。

所有 Run/玩家身份来自 Coordinator，并在调用 P27.4 前后核对。入口无持久状态、缓存、重试、序号、随机身份、World 发现或 Actor 发现。

## 4. 顺序与失败关闭

实现先处理 gameplay gate。被阻止时调用现有 InputAdapter 的 preflight，依赖验证次数、采样次数和生命周期调用次数均为 0。

允许 gameplay 后只验证一次依赖，依次检查 ready Coordinator、active/valid Lifecycle、Lifecycle Run、显式 World 和具体 ActorClass。任一失败都让 InputAdapter 以 `LifecycleUnavailable` 结束，保持零采样与零生命周期调用。

依赖有效时 InputAdapter 才采样锚点、派生 AttemptId 并调用固定生命周期函数。完成后重新检查 Coordinator、Lifecycle、World 和 ActorClass；绑定变化或嵌套证据无效时返回 `StateDesynchronized`。

## 5. Session 幂等修复

在 `Fdemo_mapShanmenFormationProductSession::TryPrepareAnchor()` 中保留精确锚点重放优先级，随后检查 AttemptId 是否已存在于任何已提交锚点审计。

若同一 AttemptId 被用于另一 AnchorDefinitionId，立即返回 `AttemptConflict`。检查发生在材料预留和 pending 状态写入前，因此失败不会修改物品权威，也不会把 Session 推入无效状态。

既有 `FormationSession.CommitReplayAndEnd` 增加跨锚点冲突断言，证明无 pending 材料且 Session 保持 valid。

## 6. 产品集成测试

在既有 Formation ProductHost fixture 上增加两个专项：

1. `GameplayAndDependencyFences`：覆盖 gameplay blocked、缺 Coordinator、缺 Lifecycle、跨 Run Lifecycle、缺 World 与缺 ActorClass；全部保持零采样、零生命周期调用和物品快照不变；
2. `ReplayConflictAndCompletion`：通过 P27.12 具体开始路由启动阵法，再覆盖无效锚点、首次放置、精确重放、ActorClass 漂移、同事件跨锚点冲突、第二事件完成阵法，以及唯一生命周期 teardown。

产品证明中两个放置 Actor 最终均被清理，FormationRunLifecycle 清空，Combat Run 结束。

## 7. 自动化结果

| Group | Success | Fail | SHA-256 |
|---|---:|---:|---|
| `Shanmen.0_0_10.Product.FormationAnchorProductRoute` | 2 | 0 | `C485871E7F7223F356A1A1103083BFFA651C87FEDA530A5BEF9BB887027206DA` |
| `Shanmen.0_0_10.Product.FormationSession` | 4 | 0 | `9D6DEDD88677070746D51993532001BB04677F451AE3200D936A3D26F5BCCD2F` |
| `Shanmen.0_0_10` | 1,355 | 0 | `6544558EEF7D3E1CE112DA2BD2A0DC85A695BD541FC3ADF62ED3A6A0CE1344AD` |
| `demo_map.V3.Attributes` | 4 | 0 | `99A02B9146F3EDE7F39C2A91FAC65524C381A3EAD230345868BF806FBE73BFD2` |

所有最终日志均具备原生退出 0、UE 5.8 `TEST COMPLETE. EXIT CODE: 0`、0 Fail 与 0 Fatal/Unhandled/Ensure。完整根组从 17:38:14.812 UTC 运行至 18:43:49.170 UTC，共 65 分 34.358 秒。

## 8. 回归映射

- `Scripts/ShanmenRegressionMap.json` 新增 `FormationAnchorProductRoute` 规则；
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1` 新增专项 fixture、正向映射和“专项不可替代生命周期/材料/World 证明”的负向测试；
- JSON 解析：PASS；
- 正反自测：`478/478` PASS；
- 覆盖门：`REGRESSION_COVERAGE: PASS Changed=7 Rules=3 Required=41 Logs=4`；
- `git diff --check`：PASS。

## 9. 构建证据与 P/F

Editor：

- 7 actions；`SUCCEEDED`；原生退出 0；23.789 秒；
- run-state SHA-256：`CF3B8560CCB3D006DCCEA30EB5A7B8DF335ACD99DDAFA2D5DC93FFBCFC058500`；
- stdout SHA-256：`9DEE5528E3A53A66211F00681DC968E9358D35A510A4B3288F4A4215A09E3940`；
- `UnrealEditor-demo_map.dll`：19,654,144 bytes；SHA-256 `D9F5D5249A54F78A6A74CE51BB70B1F1167BDC48618937D80959253CBAEA708C`。

Game：

- 6 actions；`SUCCEEDED`；原生退出 0；31.246 秒；
- run-state SHA-256：`93B30CD08F84B37BE06A57C647F472AE76534F58B757C066076D59C11571424A`；
- stdout SHA-256：`CB7A7D087A18780D88291FDFAEF1DC23BBDFF2C520C24C37FCE3D1CE0281E8FD`；
- `demo_map.exe`：360,276,992 bytes；SHA-256 `8FD1116357147973B0A81B8A80E2D80AD362D9FB834A2D879E5269CA7957C849`。

两次 stderr 均为 0 bytes，SHA-256 `E3B0C44298FC1C149AFBF4C8996FB92427AE41E4649B934CA495991B7852B855`。

P 阶段完成原生编译、无头自动化、边界扫描与证据校验。F 阶段未启动 Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。

## 10. 精确提交清单

1. `Source/demo_map/demo_mapShanmenFormationAnchorProductRoute.h`
2. `Source/demo_map/demo_mapShanmenFormationAnchorProductRoute.cpp`
3. `Source/demo_map/demo_mapShanmenFormationProductSession.cpp`
4. `Source/demo_map/demo_mapShanmenFormationProductSessionTests.cpp`
5. `Source/demo_map/demo_mapShanmenFormationProductHostTests.cpp`
6. `Scripts/ShanmenRegressionMap.json`
7. `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`
8. `Docs/Report/Dev.D.UE.0.0.10.P27.13.r0_report.md`
9. `Docs/Log/Dev.D.UE.0.0.10.P27.13.r0_log.md`

`Saved/Codex/P27.13` 与 `Saved/FoundationRuns/Dev.D.UE.0.0.10.P27.13.r0` 不进入 Git；103 个既有未跟踪用户文件保持未暂存。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p27-13-formation-anchor-product-route>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-13-formation-anchor-product-route/Docs/Report/Dev.D.UE.0.0.10.P27.13.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-13-formation-anchor-product-route/Docs/Log/Dev.D.UE.0.0.10.P27.13.r0_log.md>
