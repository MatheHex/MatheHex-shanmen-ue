# Dev.D.UE.0.0.10.P27.27.r0 Development Log

## 1. 目标

- 把 P27.26 scatter World-publication Run Route 接入既有唯一 Formation Run Lifecycle；
- 每个活动 Run 最多持有一个可选 Route 槽，只接受同 Run 的已提交 Handoff；
- 暴露显式 Publish 表面，但禁止调用方注入 Route/Command 身份或绕过 Host/Session；
- 固定 scatter End → product teardown → shared Combat Run release 顺序；
- 分别保存 scatter 与 product teardown 成功检查点，失败重试不重复 World/资源副作用；
- 在错误 World、Coordinator 拒绝、身份漂移、重绑与 late operation 上失败关闭；
- 整体转移 lifecycle、Controller、Route、Host/Session 和记忆内回执，使旧 Owner 失效；
- 用真实无头 World 自动化、既有兼容组、完整根回归、改动驱动映射、双目标构建、Report/Log 与 GitHub 推送完成阶段交接；
- 严格停留在底层 P 阶段，不接玩家输入、地图或实际游戏性。

## 2. 基线与边界

- 基线：`a8212086d8588aeff41b5236e36c162ae6846b10`（P27.26）；
- 分支：`agent/0.0.10-p27-27-formation-scatter-publication-run-lifecycle`；
- 起始 tracked tree clean；
- 103 个既有未跟踪用户文件保持原样且不暂存；
- 复用现有 `Fdemo_mapShanmenFormationRunLifecycle`，不建立第二个 Run 生命周期根；
- 复用 P27.26 Route、P27.25 Host、P27.24 Session、P27.23 Publication 与 P27.22 Handoff，不复制其 Authority；
- World 只作为调用时借用参数，不成为 Lifecycle 字段；
- 允许 UnrealEditor-Cmd 无头瞬态 World/Actor 测试与 Editor/Game 编译；
- 禁止 Unreal Editor UI、PIE、Standalone、产品 exe、物理输入、截图、Smoke、Cook、Package、正式地图/内容资产和玩法数值修改。

## 3. 修改文件

实现：

1. `Source/demo_map/demo_mapShanmenFormationRunLifecycle.h`
2. `Source/demo_map/demo_mapShanmenFormationRunLifecycle.cpp`

测试与流程：

3. `Source/demo_map/demo_mapShanmenFormationScatterResourcePreparationTests.cpp`
4. `Scripts/ShanmenRegressionMap.json`

交接：

5. `Docs/Report/Dev.D.UE.0.0.10.P27.27.r0_report.md`
6. `Docs/Log/Dev.D.UE.0.0.10.P27.27.r0_log.md`

写入交接文档前，四个实现/测试/映射文件净增 703 行、删除 16 行。Lifecycle 头/实现当前共 833 行、787 个非空行；共享测试宿主新增 365 行；Regression Map 增加 13 个下游证据要求。

## 4. 实现过程

### 4.1 唯一可选 Route 槽

Lifecycle 新增一个 `TOptional<Fdemo_mapShanmenFormationScatterWorldPublicationRunRoute>`。`TryOpenScatterPublicationRoute` 仅在 lifecycle 活动、整体有效且 teardown 尚未开始时构造候选 Route；候选必须由当前 Run 的已提交 Handoff 与有效 Actor 类通过 P27.26 `TryOpen` 建立。

如果槽已占用，只有派生出同一 Route ID 的精确请求作为幂等成功；Actor 类、Handoff 或身份漂移不能替换 Owner。绑定后再次执行 lifecycle 跨对象不变量检查；失败会清空候选并关闭操作。

### 4.2 显式 Publish 与 late-operation 围栏

`TryPublishScatterPublication` 验证 active/valid lifecycle、未开始 teardown 且唯一槽存在，然后只调用 Route 的 `TryPublish(RunId, World)`。没有暴露 Command ID 或下层 Publication/Host 写入口。Host 已保存成功命令回执，所以精确 Publish 重放可在 `nullptr` World 下返回同一身份与证据。

一旦 scatter 或 product teardown 检查点存在，新的 scatter Publish、产品 `TrySubmit` 与 `TryExecuteAnchorOperation` 全部拒绝；这避免部分 teardown 后重新产生产品或 World 状态。

### 4.3 三段结束与失败恢复

`TryTeardownProduct` 在既有 Coordinator/Run 验证后先处理可选 Route：

1. 没有 Route 时保持旧行为；
2. 有 Route 且无检查点时路由确定性 `End`；
3. Route 拒绝时返回 `ScatterPublicationTeardownRejected`，不启动产品 teardown；
4. Route 成功时保存完整 `ScatterPublicationTeardownCheckpoint`；
5. 然后执行并保存既有产品 teardown 检查点。

`TryEndRun` 继续在产品 teardown 后请求 Coordinator release。Coordinator 拒绝时两个成功检查点保持原样；修复身份后的精确重试复用它们，World 参数可为空。`TryAcknowledgeCoordinatorEnded` 在 Route 存在时同时要求两个检查点，防止外部 Run release 绕过散射清理。

### 4.4 整根 Owner 转移与有效性

静态 `TryTakeover` 要求有效活动源、空目标且对象不同。它先经 P27.26 Route `TryTakeover` 移动嵌套 Host/Session/命令账本，再移动 Run、Controller 和两个检查点；候选整体有效后才清空旧生命周期。旧根因此不能再 Publish、Submit、执行 Anchor 或结束 Run。

`IsValid()` 新增跨对象核验：Route 必须有效并绑定同一 Run；scatter 检查点必须是该 Route 的成功 `End`；产品检查点若与 Route 共存，则 scatter 检查点必须先存在。空生命周期也不得残留 Route 或任一检查点。

## 5. 专项与兼容证明

新增四条 `FormationRunLifecycle.ScatterPublication` 测试：

1. 唯一槽、Handoff 绑定、Actor 类漂移围栏、显式 Publish 与 `nullptr` 重放；
2. 错误 World 在产品 teardown 前失败、正确 World 精确恢复和顺序检查点；
3. Coordinator 身份冲突后双检查点保留、late Publish 围栏、`nullptr` release 重试；
4. 整个 lifecycle Owner 转移、旧根失效、新根独占重放和结束。

所有专项使用真实无头瞬态 `UWorld`、`GameInstance` Subsystem、`APawn`/Health、CombatRunCoordinator 与具体 `ACharacter` Actor，检查 World Actor 数、Route/Command/Run 身份、Controller/Coordinator 活性和检查点复用。

- 专项时间：2026-09-14 00:07:51.475 UTC 至 00:08:10.609 UTC，19.134 秒；
- 专项结果：4 Success、0 Fail、原生退出 0；
- 专项 SHA-256：`2BB5405C43CDE975A30828E211E6D1C675136DD914ED6C2637951E5178CA5F40`。

完整 FormationRunLifecycle 兼容组：10 Success、0 Fail、原生退出 0；同时覆盖旧无 Route 路径、外部 Coordinator、GameMode 组合、Coordinator 恢复和 Anchor Operation。日志 SHA-256 `AEB37FB5D261EFAFB1FE48CD2FBD557A1AB972C9E47BAADD3C4C92AEB6001C91`。

## 6. 改动驱动回归

| Group | Success | Fail | SHA-256 |
|---|---:|---:|---|
| FormationRunLifecycle.ScatterPublication | 4 | 0 | 2BB5405C43CDE975A30828E211E6D1C675136DD914ED6C2637951E5178CA5F40 |
| FormationRunLifecycle | 10 | 0 | AEB37FB5D261EFAFB1FE48CD2FBD557A1AB972C9E47BAADD3C4C92AEB6001C91 |
| Shanmen.0_0_10 full | 1,412 | 0 | 2BD298A85223860E72BB1CB9E0CBF8B2D5C58A2A97E14D843802AC4816DA48C6 |

完整根组首项于 2026-09-14 00:12:52.812 UTC 开始，末项于 01:31:04.422 UTC 完成，持续 1 小时 18 分 11.610 秒；原生退出 0，Fail/Fatal/Unhandled/Ensure 为 0。既有联网探测超时和长时 transient World 证明只产生非失败日志，队列持续推进并原生结束。

## 7. 回归映射

扩展 `FormationRunLifecycle` 规则，新增要求 P27.26 Route、P27.25 Host、P27.24 Session、Publication、Handoff、World Delivery、Deployment/Resource/Batch Intent、Mastery Authorization/Adapter 等完整依赖链。共享测试宿主同时命中已有散射资源/发布规则；最终去重结果：

- Changed：6；
- Rules：7；
- Required：31；
- Self-test：`SELF_TEST: PASS 504/504`，SHA-256 `507E91DE5C42AAE263707B06CFCD250EC66AAA7DACAE7F9A5E04DD7F85B1C5D9`；
- Coverage：`REGRESSION_COVERAGE: PASS Changed=6 Rules=7 Required=31 Logs=3`，SHA-256 `0D28E87E1DFE3A0F101F8129B2FCABB8CC38C223CC897F7523606CF8CE575B48`。

## 8. 构建、产物与卫生

Editor：

- 写完测试后的首次构建：42 actions，Result Succeeded，原生退出 0，总计 47.93 秒；
- 最终构建：4 actions，Result Succeeded，原生退出 0，总计 9.58 秒；
- 最终日志 SHA-256：`2A78D1190E2DBECA4A4FD3AB260A9D09C1C97C26D02FEAD01AFD41E37EC7DEB8`；
- `UnrealEditor-demo_map.dll`：20,218,368 bytes，SHA-256 `47B47281559F2E0DB433BB31D54AF739934A758BF762C8B33876D77187041551`。

Game：

- 41 actions；UBA 57.56 秒，总计 62.03 秒；Result Succeeded，原生退出 0；
- 最终日志 SHA-256：`A1738758DE22FC6B7258226B71A1C3D4F6DD2C6DBBF3020F2B5399BC8EDE0DDC`；
- `demo_map.exe`：360,756,736 bytes，SHA-256 `1D19F773809ABE70BCCA3D3AB12DEE8B53A12A8D1EC05787B996BF774C2A0774`。

卫生：

- Regression Map JSON 解析 PASS；
- 静态边界扫描无 Tick/Timer、异步、随机 GUID/RNG、Actor 枚举、直接 Spawn/Destroy、Actor transform/owner 写入、Inventory/Profile/SaveGame；
- `UWorld*` 仅为方法参数；
- `git diff --check`：PASS；
- 最终暂存差异检查：PASS；
- 最终暂存精确为 6 个实现、测试、映射和交接文件；
- 103 个既有未跟踪用户文件保持原样且未暂存。

## 9. P/F 边界

P 阶段完成：Handoff → lifecycle 唯一 Route 槽 → Publish/replay → scatter End → product teardown → Combat Run release，以及两种失败点的检查点恢复和整体 Owner 转移。

F 阶段未执行：没有物理玩家输入、正式地图或内容资产、玩法数值/手感、敌人行为、关卡、UI/体验工作；没有 Unreal Editor UI、PIE、Standalone、产品 exe、截图、Smoke、Cook 或 Package；没有修改 GameMode 产品入口。

## 10. 下一阶段与 GitHub

建议 P27.28 仅在既有 GameMode 产品组合层增加受控、无输入的 Handoff 装载与显式 Publish 路由，使其持有的唯一 `FormationRunLifecycle` 能走通本阶段契约；验证重复调用、身份漂移、发布失败、顺序 teardown 和双检查点恢复。不得接物理输入、地图或玩法表现。

- Branch: <https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p27-27-formation-scatter-publication-run-lifecycle>
- Report: <https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-27-formation-scatter-publication-run-lifecycle/Docs/Report/Dev.D.UE.0.0.10.P27.27.r0_report.md>
- Development Log: <https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-27-formation-scatter-publication-run-lifecycle/Docs/Log/Dev.D.UE.0.0.10.P27.27.r0_log.md>
