# Dev.D.UE.0.0.10.P27.27.r0 Report

## 1. 结论

P27.27 已把 P27.26 `FormationScatterWorldPublicationRunRoute` 接入既有、唯一的 `Fdemo_mapShanmenFormationRunLifecycle` 根：每个 Formation Combat Run 最多拥有一个可选 scatter-publication Route 槽，只能由该 Run 的已提交 P27.22 Handoff Evidence 打开。上层可以显式请求 Publish，但不能持有 Route/Command ID、绕过 Route/Host/Session、替换 Actor 类，或建立第二条 World/资源权威。

生命周期结束顺序现已固定为：scatter Route `End` → formation 产品 teardown →共享 Combat Run release。scatter 与产品 teardown 分别保存成功检查点；若错误 World、产品 teardown 或 Coordinator release 失败，后续精确重试只执行尚未成功的步骤，已成功步骤可在 `nullptr` World 下重放而不再次进入 World。整个活动生命周期可一次性转移给新 Owner，旧 Owner 同时失效。

专项测试 4/4，完整 FormationRunLifecycle 兼容组 10/10，完整 `Shanmen.0_0_10` 根组 1,412/1,412，映射自检 504/504，改动文件驱动的 31 个证据组由专项与完整根日志覆盖，Editor 与 Game 双目标构建均成功。本阶段只闭合底层组合、唯一权威与 teardown/replay 契约，没有进入实际游戏性开发。

## 2. 阶段问题与范围

P27.26 已提供确定性 Run Route，但 Route 仍由外部调用方独立持有。既有 Formation 生命周期只组合产品 Controller 与共享 Combat Run release；若调用方分别拥有生命周期与 Route，会出现以下结构风险：

- Route 可能比其 Run 生命周期更早丢失或更晚存活；
- 产品 teardown 可能先于散射 Actor 清理，留下脱离产品 Owner 的 World 批次；
- Coordinator release 失败时，调用方可能重复清理 World 或重复取消产品资源；
- Owner 转移只能移动 Route 或生命周期的一半，产生两个仍可操作的根；
- 已提交 Handoff 与活动 Run 的一致性依赖调用方纪律，而非结构约束。

P27.27 只修复这些底层组合缺口：在既有生命周期里增加唯一可选槽、显式 Publish、顺序 teardown、独立检查点、精确重试和整根 Owner 转移。没有增加物理输入、地图入口、玩法规则、定时器、后台任务、全局 Registry 或第二套 Authority。

## 3. 唯一 Route 槽与 Handoff 绑定

`TryOpenScatterPublicationRoute` 只在 lifecycle 活动、整体有效且尚未出现任何 teardown 检查点时工作。它委托 P27.26 `RunRoute::TryOpen` 验证完整 Handoff 证据链，并由下层持续核对：

1. Handoff 已由 P27.22 提交；
2. Handoff 内嵌 Resource Plan 的 `ActiveRunId` 等于 lifecycle `RunId`；
3. Actor 类有效；
4. Route、Host、Session、Deployment 与资源证据身份彼此一致。

同一 Handoff 与同一 Actor 类再次打开会派生同一 Route ID，作为幂等成功返回；若 Actor 类、Handoff 或 Route 身份漂移，唯一槽拒绝重绑。Lifecycle 不复制 Handoff 内容、不暴露 Command ID 生成入口，也不保存 `UWorld*`。

`TryPublishScatterPublication` 是唯一的生命周期 Publish 表面：必须有活动 lifecycle 和已绑定 Route，且 teardown 尚未开始。调用只把当前 Run 与一次性借用 World 交给 P27.26 Route；精确成功重放沿 P27.25 Host 的既有命令记录返回同一 Command/Session 回执，因此测试可用 `nullptr` World 证明不会重复发布 Actor。

## 4. 顺序 teardown 与双检查点

`TryTeardownProduct` 的顺序现在不可由调用方调整：

1. 先验证 Lifecycle 与 Coordinator 均活动且绑定同一 Run；
2. 若存在 scatter Route 且尚无 scatter 检查点，路由确定性的 `End`；
3. 只有 scatter `End` 成功后，才保存 `ScatterPublicationTeardownCheckpoint`；
4. 再执行既有 Formation Product Controller teardown；
5. 产品 teardown 成功后保存既有 `ProductTeardownCheckpoint`；
6. `TryEndRun` 最后请求 Coordinator 释放共享 Combat Run 身份。

错误 World 会由 Route/Host/Session 下层拒绝，状态返回 `ScatterPublicationTeardownRejected`，且产品 Controller 与 Coordinator 都保持活动；随后可用正确 World 重试。若 Coordinator release 在两项 teardown 成功后拒绝，两个检查点都被保留；身份修复后的重试可以传 `nullptr` World，只复用检查点并重试共享 Run release，不再结束 Route、清理 Actor或取消产品资源。

从任一 teardown 检查点出现开始，新的散射 Publish、产品 Submit 与 Anchor Operation 都失败关闭。`TryAcknowledgeCoordinatorEnded` 也要求：有 Route 时必须同时存在有效 scatter `End` 检查点和产品 teardown 检查点，防止外部 release 绕过 World 清理。

## 5. 生命周期 Owner 转移与有效性

新增 `Fdemo_mapShanmenFormationRunLifecycle::TryTakeover`，只允许一个有效且活动的旧生命周期整体移入完全空的新生命周期。转移内容包含：

- Run ID；
- Formation Product Controller；
- 可选 P27.26 scatter Route 及其完整 Host/Session/命令记录；
- 可选 scatter teardown 检查点；
- 可选 product teardown 检查点。

Route 通过其既有唯一 `TryTakeover` 移动，旧 Route Owner 失效；组合完成后旧 lifecycle 被清空。自转移、空/无效源、非活动源或非空目标全部拒绝。`IsValid()` 现在持续验证 Route Run、Route ID、scatter 检查点 Run/Route/Event 以及“产品检查点不得先于 scatter 检查点”等跨对象不变量。

## 6. P27.27 专项证明

四条真实无头瞬态 World/Actor 自动化覆盖：

1. `SoleSlotPublishReplay`：未绑定时 Publish 失败；已提交 Handoff 打开唯一槽；精确打开幂等；Actor 类漂移拒绝；首次 Publish 生成 2 个 Actor；`nullptr` 精确重放保持同一 Route/Command 且不增生。
2. `WorldFailureFencesProductTeardown`：在错误 World 请求 teardown 时，scatter Route 失败且产品/Coordinator 尚未开始结束；正确 World 重试按 scatter → product 顺序保存两个检查点并清除 2 个 Actor。
3. `CheckpointedCoordinatorRecovery`：故意注入 Coordinator 身份冲突；两项 teardown 成功后 release 被拒，两个检查点保留且 late Publish 被围栏；修复身份后以 `nullptr` 重试，只复用双检查点并完成 Run release。
4. `TakeoverMovesSoleOwner`：整体转移后旧 lifecycle 为空且不能 Publish，新 Owner 可用 `nullptr` 重放既有 Publish，并独占完成 scatter、product 与 Combat Run 结束。

专项从 2026-09-14 00:07:51.475 UTC 运行至 00:08:10.609 UTC，持续 19.134 秒；结果 4 Success、0 Fail、原生退出 0。日志 SHA-256 `2BB5405C43CDE975A30828E211E6D1C675136DD914ED6C2637951E5178CA5F40`。

既有 `FormationRunLifecycle` 全组另行运行 10 项，包含无 scatter 的旧结束路径、外部 Coordinator 组合、GameMode 组合、恢复和 Anchor Operation；10 Success、0 Fail、原生退出 0，证明可选槽没有破坏既有调用方。日志 SHA-256 `AEB37FB5D261EFAFB1FE48CD2FBD557A1AB972C9E47BAADD3C4C92AEB6001C91`。

## 7. 自动化与回归证据

| 范围 | Success | Fail | SHA-256 |
|---|---:|---:|---|
| FormationRunLifecycle.ScatterPublication | 4 | 0 | 2BB5405C43CDE975A30828E211E6D1C675136DD914ED6C2637951E5178CA5F40 |
| FormationRunLifecycle 完整兼容组 | 10 | 0 | AEB37FB5D261EFAFB1FE48CD2FBD557A1AB972C9E47BAADD3C4C92AEB6001C91 |
| Shanmen.0_0_10 完整根组 | 1,412 | 0 | 2BD298A85223860E72BB1CB9E0CBF8B2D5C58A2A97E14D843802AC4816DA48C6 |

完整根组从 2026-09-14 00:12:52.812 UTC 运行至 01:31:04.422 UTC，持续 1 小时 18 分 11.610 秒；原生退出 0。联网可用性探测超时和既有长时无头 World 证明只作为 Warning/大 delta 记录；完整日志中 Fail、Fatal、Unhandled Exception 与 Ensure condition failed 均为 0。

完整根日志覆盖映射要求的全部证据组，专项日志独立证明本阶段新增四项。覆盖脚本逐一验证每份日志的命令根、成功事件、失败事件、终止标记和致命信号，且不会用本阶段专项替代 Route/Host/Session/Handoff/资源链或完整根组。

## 8. 回归映射、构建与静态边界

`ShanmenRegressionMap.json` 扩展既有 `FormationRunLifecycle` 规则：凡改动 Lifecycle 或其共享测试宿主，除原有 Product Controller、Authority、Coordinator、Formation Session 等组外，必须同时证明 P27.26 Route、P27.25 Host、P27.24 Session、Publication、P27.22 Handoff、World Delivery、Deployment/Resource/Batch Intent 和 Mastery Adapter 链。测试宿主同时命中既有散射资源规则，因此本阶段最终要求 31 个唯一证据组。

- 映射自检：`SELF_TEST: PASS 504/504`，SHA-256 `507E91DE5C42AAE263707B06CFCD250EC66AAA7DACAE7F9A5E04DD7F85B1C5D9`；
- 最终覆盖门：`REGRESSION_COVERAGE: PASS Changed=6 Rules=7 Required=31 Logs=3`，SHA-256 `0D28E87E1DFE3A0F101F8129B2FCABB8CC38C223CC897F7523606CF8CE575B48`；
- Editor：4 actions，Result Succeeded，原生退出 0，总计 9.58 秒；日志 SHA-256 `2A78D1190E2DBECA4A4FD3AB260A9D09C1C97C26D02FEAD01AFD41E37EC7DEB8`；
- `UnrealEditor-demo_map.dll`：20,218,368 bytes，SHA-256 `47B47281559F2E0DB433BB31D54AF739934A758BF762C8B33876D77187041551`；
- Game：41 actions，Result Succeeded，原生退出 0；UBA 57.56 秒，总计 62.03 秒；日志 SHA-256 `A1738758DE22FC6B7258226B71A1C3D4F6DD2C6DBBF3020F2B5399BC8EDE0DDC`；
- `demo_map.exe`：360,756,736 bytes，SHA-256 `1D19F773809ABE70BCCA3D3AB12DEE8B53A12A8D1EC05787B996BF774C2A0774`。

Lifecycle 头/实现当前共 833 行、787 个非空行。本阶段静态扫描未发现 Tick/Timer、异步、随机 GUID、RNG、Actor 枚举、直接 Spawn/Destroy Actor、Actor transform/owner 改写、Inventory/Profile/SaveGame 访问；`UWorld*` 只出现在借用参数。Regression Map JSON、`git diff --check` 与最终暂存差异检查均为 PASS。

## 9. P/F 边界

P 阶段完成：已提交资源计划 → Handoff → Lifecycle 唯一 Route 槽 → 显式 Publish/Replay → scatter End 检查点 → product teardown 检查点 → Coordinator release；错误 World 与 Coordinator 拒绝均可精确恢复，整个根可唯一转移，旧 Owner 失效。真实无头瞬态 World 中验证了 2 个 Actor 的发布、无增生重放与顺序清理。

F 阶段未执行：没有接入物理玩家输入，没有修改 GameMode 的产品入口、正式地图或内容资产，没有设计/调试玩法数值、手感、敌人行为、关卡、UI 表现或玩家体验；没有启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件；没有截图、Smoke、Cook 或 Package。

## 10. 下一阶段与 GitHub

下一处仍可能迫使后续返工的结构缺口位于产品组合入口，而非玩法层：GameMode 已持有唯一 `FormationRunLifecycle`，其既有结束路径也会自动经过本阶段顺序 teardown，但尚没有从“已经提交的 P27.22 Handoff”调用唯一槽并显式 Publish 的受控组合表面。建议 P27.28 只增加这个无输入的产品组合路由及其无头证明：Handoff/Run 身份一致、重复调用幂等、失败关闭、GameMode teardown 自动复用双检查点；继续禁止物理输入、地图接线与玩法表现。

基线提交：`a8212086d8588aeff41b5236e36c162ae6846b10`（P27.26）。分支：`agent/0.0.10-p27-27-formation-scatter-publication-run-lifecycle`。

- Branch: <https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p27-27-formation-scatter-publication-run-lifecycle>
- Report: <https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-27-formation-scatter-publication-run-lifecycle/Docs/Report/Dev.D.UE.0.0.10.P27.27.r0_report.md>
- Development Log: <https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-27-formation-scatter-publication-run-lifecycle/Docs/Log/Dev.D.UE.0.0.10.P27.27.r0_log.md>
