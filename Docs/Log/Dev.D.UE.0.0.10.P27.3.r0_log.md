# Dev.D.UE.0.0.10.P27.3.r0 Development Log

## 1. 目标

- 把既有 Formation ProductHost 的 Prepare/Commit/Placement 接入 P27.2 唯一 Run 生命周期；
- 在任何耐久材料访问前校验设备无关操作身份、Run、World 与 ActorClass；
- 保留既有 Host 的幂等回执与 committed placement 前向恢复，不建立第二套权威；
- 让完整 Active 阵图走正常 End，让未完成 Deploying 阵图继续走 Cancel；
- 完成改动驱动回归、Report/Log 提交和 GitHub 推送。

## 2. 基线与范围

- 基线：`280f9c10712c49c78306d9bf25d739299b085d32`（P27.2）；
- 分支：`agent/0.0.10-p27-3-formation-anchor-operation-gateway`；
- 起始 tracked tree clean；103 个既有未跟踪用户文件保持未暂存；
- 不改阵法定义、材料数量、范围、持续时间、影响数值、物品 schema、地图、资产或回归映射；
- 不启动 Unreal Editor UI、PIE、Standalone 或产品可执行文件。

## 3. 操作值与结果证据

新增 `Fdemo_mapShanmenFormationAnchorOperation`，只允许通过 `TryCapture()` 固定有效的：

1. Combat Run 身份；
2. AnchorDefinitionId；
3. AttemptId。

新增 `Fdemo_mapShanmenFormationAnchorOperationResult`，保留操作、ActorClassPath、Prepare/Commit/
Placement 三段原始结果、是否从 committed pending placement 恢复，以及明确状态与诊断。
`IsSuccess()` 会交叉验证三段成功证据、操作身份、投放 intent、世界 receipt 和 ActorClassPath。

## 4. 唯一产品网关

生命周期公开 `TryExecuteAnchorOperation()`；Controller 中真正编排 Host 的同名方法保持 private，并只把
生命周期声明为 friend。调用顺序为：

1. 生命周期有效性、teardown 检查点、操作身份、Run 和 Ready Coordinator；
2. Controller/Host 有效性；
3. 有效且未 teardown 的 World 与具体 ActorClass；
4. 既有 `TryPrepareAnchor()`；
5. 既有 `TryCommitPreparedAnchor()`；
6. 既有 `TryPlaceCommittedAnchor()`。

网关不直接读写库存/档案、不创建或扫描 Actor，也不制造新的材料、Session 或世界权威。

## 5. 重放、恢复与失败关闭

- Host 没有 pending 时，执行完整三段路径；
- exact committed pending 与操作身份一致时，跳过 Prepare 并继续 Commit/Place；
- pending 属于其它操作时返回 `PendingPlacementConflict`；
- exact 已完成操作返回 `Replayed`，复用世界 receipt，物品快照不变；
- ActorClass 漂移由既有 placement binding 拒绝；
- World/ActorClass 无效时在材料权威前返回 `WorldBindingInvalid`；
- 产品 teardown 检查点存在时返回 `TeardownPending`，只允许 Run release 精确重试。

## 6. 终止语义修正

原 `TryCancelAndEnd()` 更名为 `TryTerminateAndEnd()`。它读取唯一 ProductHost Session 状态并选择：

- Deploying/Cancelled -> `TryCancelAndTeardown()`；
- Active/Ended -> `TryEndAndTeardown()`。

终止摘要新增 `bEndedCompletedFormation`；摘要不变量同时校验该标志与最终 Session Status。P27.2 的
产品优先、Coordinator 后置释放、单次 teardown 检查点与精确重试顺序保持不变。

## 7. 测试结果

新增两项生命周期测试，并重跑两项既有生命周期与两项 Controller 测试：

- `OrderedReplayAndActiveEnd`：两锚点投放、exact replay、ActorClass 漂移、Active End、两 Actor 清理；
- `PreflightAndTeardownFences`：无效身份/Run/World/Class 前置失败、Coordinator 故障检查点、新操作
  隔离、身份修复后 exact release；
- P27.2 `OrderedEnd` / `CoordinatorRecovery`；
- P27.1 Controller `FrozenReplayAndEnd` / `Fences`。

| Group | Success | Fail | SHA-256 |
|---|---:|---:|---|
| `Shanmen.0_0_10.Product.FormationRunLifecycle` | 4 | 0 | `C8901C78009F3474C162E59B6100B5A5DB39CF8B0C604EED6F1B28F39424E5AE` |
| `Shanmen.0_0_10.Product.FormationProductController` | 2 | 0 | `FAE72AD4D0C7F5E0CE0841BE387FFB5953E063860914F7E64C0502D45A69D68B` |
| `Shanmen.0_0_10` | 1,328 | 0 | `522C263CD8A3E104F3A0CE95EAB4B6B25DAEE7A5E493EEE1A4F37DB83DF1F8B6` |
| `demo_map.V3.Attributes` | 4 | 0 | `D88B3CF086CBCBFAAA4D928FAB90B6D3BA5F55FE7D771BF8FEB2146F2A5042AD` |

所有最终测试进程原生退出 0；日志为 0 Fail、0 Fatal/Unhandled/Ensure，并包含 UE 5.8 原生终端
完成证据。

## 8. 改动驱动覆盖与构建

改动路径：

1. `Source/demo_map/demo_mapShanmenFormationProductController.h`
2. `Source/demo_map/demo_mapShanmenFormationProductController.cpp`
3. `Source/demo_map/demo_mapShanmenFormationRunLifecycle.h`
4. `Source/demo_map/demo_mapShanmenFormationRunLifecycle.cpp`
5. `Source/demo_map/demo_mapShanmenFormationProductHostTests.cpp`

现有映射已完整覆盖这些路径，无需修改映射文件。最终门禁：
`REGRESSION_COVERAGE: PASS Changed=5 Rules=3 Required=39 Logs=4`；SHA-256
`B7DB3ED84E2C4AF3FFCB31DE6744C39644C33D9E82DB9A46C7C0EDDF469B1C9B`。

映射器自测 `463/463` PASS；SHA-256
`7813A4064A20D651BEF0837D86D692BC8E17864065E2BED18C16D3472BE14EE6`。

| Target | Result | Duration | Log SHA-256 |
|---|---|---:|---|
| `demo_map` Win64 Development | Succeeded / 0 | 48.61s | `6326D124860D6FEBF33B6E24F3E4DB9543F9602E31846139176979A7E7F594B1` |
| `demo_mapEditor` Win64 Development | Up to date, Succeeded / 0 | 1.69s | `DF20396ED42B6AFEAB7DF7DAE111A8F49538A46F5B4FB9D659156F683E71625B` |

二进制：

- `demo_map.exe`：360,065,536 bytes；SHA-256
  `74022E7EBC90AC839329F7DCBC97FAB30EA4C2B5DDDB8DCF0D246D545263B29A`；
- `UnrealEditor-demo_map.dll`：19,395,072 bytes；SHA-256
  `599A5347702FD8986467688E583D307C7FE39B66A7AD68AD4FB198C8B066F5AF`。

## 9. 静态边界与下一步

`git diff --check`、regression map JSON 解析均 PASS。Controller/Lifecycle scoped scan 未发现
`SpawnActor`、`DestroyActor`、Actor 扫描、输入轮询、Widget、Tick、RNG 或 `ApplyDamage`；世界投放
继续完全委托既有 Host/WorldAdapter。

未启动 Unreal Editor UI、PIE、Standalone、产品 exe、真实输入、截图、Smoke、Cook 或 Package。
下一阶段应实现设备无关输入采样适配器：采样后只构造既有 FormationIntent/AnchorOperation 并提交
给生命周期，不读取产品 Host、不耦合 GameMode、不轮询世界。

## 10. 精确提交清单

1. `Source/demo_map/demo_mapShanmenFormationProductController.h`
2. `Source/demo_map/demo_mapShanmenFormationProductController.cpp`
3. `Source/demo_map/demo_mapShanmenFormationRunLifecycle.h`
4. `Source/demo_map/demo_mapShanmenFormationRunLifecycle.cpp`
5. `Source/demo_map/demo_mapShanmenFormationProductHostTests.cpp`
6. `Docs/Report/Dev.D.UE.0.0.10.P27.3.r0_report.md`
7. `Docs/Log/Dev.D.UE.0.0.10.P27.3.r0_log.md`

`Saved/Codex/P27.3` 原始证据不进入 Git；103 个既有未跟踪用户文件保持未暂存。

- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p27-3-formation-anchor-operation-gateway>
- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-3-formation-anchor-operation-gateway/Docs/Report/Dev.D.UE.0.0.10.P27.3.r0_report.md>
- Development Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p27-3-formation-anchor-operation-gateway/Docs/Log/Dev.D.UE.0.0.10.P27.3.r0_log.md>
