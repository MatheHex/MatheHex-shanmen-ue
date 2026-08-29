# Dev.D.UE.0.0.10.P8.2.r0 Report

## 1. 结论

P8.2 已建立无 UI、无 World 依赖的 Formation product session，结论为 **PASS**。

新增 `Fdemo_mapShanmenFormationProductSession`，把既有 action Startup/Active、P8.0 deployment begin/anchor commit/cancel/end，以及 P8.1 material prepare/commit/cancel 收束为一个显式状态机。调用方不能同时打开两个阵眼材料事务，也不能在材料已经 durable committed 后反向取消。

本轮没有创建阵眼 Actor、投料交互、阵法效果、区域规则、UI、正式阵图商品或配方；没有修改 P8.0、P8.1、ShanmenItems、存档 schema 或旧库存链。

## 2. Product session 状态与启动

Session 状态只有：`Empty`、`Deploying`、`Active`、`Cancelled`、`Ended`。

`TryStart` 在候选副本中完成：

1. 冻结并校验 active-Run correlation；
2. action `Idle -> Startup -> Active`；
3. 创建 P8.0 deployment；
4. deployment `Planned -> Deploying`。

RunId、OwnerId、action snapshot、diagram、content、deployment identity 任一不一致均不发布 Session。产品层不复制库存状态；`Fdemo_mapShanmenRunCorrelation` 和 durable item receipts 仍是既有证据。

## 3. 单 pending 阵眼纪律

`TryPrepareAnchor` 只允许一个 `AnchorDefinitionId + AttemptId` 成为 pending。另一个阵眼或另一个 attempt 在 pending 完成前返回 `PendingConflict`，因此产品调用方无法制造重叠的 active-Run quantity intents。

相同 prepare 重放会从 authority ledger 重建相同 TransactionId、IntentId 与 requests，不产生第二笔 reservation。已提交阵眼的相同 attempt 会返回既有 immutable audit；不同 AttemptId 重用已提交 anchor 会失败关闭。

## 4. Commit、audit 与恢复

`TryCommitPreparedAnchor` 先从 durable authority 重建原 attempt，再完成 P8.1 material commit，最后把 fulfillment evidence 提交给 P8.0 deployment。

只有两步都成功，Session 才把 pending 移入 `Fdemo_mapShanmenFormationAnchorAudit`。Audit 同时绑定：

- anchor 与 AttemptId；
- exact committed material result/evidence；
- P8.0 deployment commit receipt；
- DeploymentId、FulfillmentId 与 authority revision。

若材料已 durable committed、但进程尚未发布 deployment receipt，Session 保留 committed material 作为唯一 pending recovery value。相同 attempt 重放时跳过再次消耗，只补纯 deployment commit。测试模拟了该 crash window，并验证恢复后的 FulfillmentId 与外部 durable commit 完全相同。

## 5. Cancel 与 End 顺序

Deploying session 的取消顺序为：

1. 在候选副本预验证 deployment cancel 与 action interrupt；
2. 若有 pending，先重建并 durable cancel 全部材料 lines；
3. 发布 deployment `Cancelled`；
4. 发布 action `Interrupted`。

一旦 ledger 已观察到任一 committed material line，取消返回 `MaterialCommitRecoveryRequired`，必须向前完成该阵眼。更早已经成功的阵眼材料不会因整座未完成阵法取消而伪造退款。

全部阵眼完成后 Session 进入 `Active`。`TryEnd` 原子编排 deployment `Ended` 与 action `Active -> Recovery -> Completed`；pending 尚未闭合时不能结束。

## 6. 自动化证据

最终日志均只有一个 `RunTests`、一个 queue-empty、Fail `0`、fatal/assert/ensure `0`，进程原生退出码均为 `0`。

| Group / 日志 | Success | Fail | SHA-256 |
|---|---:|---:|---|
| `Shanmen.0_0_10.Product.FormationSession` / `FormationSession.log` | 4 | 0 | `508FFE36315F3C0CE49D5AAF18C9362E1F89F192858B86EBFEE4DBFC210F2BC2` |
| `Shanmen.0_0_10` / `Shanmen-0_0_10-Full.log` | 219 | 0 | `F9D9E244ADBFC7497CE95D74AF1622B09E97DED06D9C2444AF6C7588E664D1E3` |

新增四项测试覆盖：

1. foreign correlation 失败关闭、same-attempt prepare replay、single-pending fence；
2. 两个阵眼依次 commit、exact audit replay、AttemptId conflict、active/end 完整链；
3. prepared materials 零消耗取消后 deployment/action 同步 terminal；
4. durable material commit 与 transient deployment commit 之间的 crash-window 前向恢复。

完整 suite 从 P8.1 的 `215` 增至 `219`，此前测试全部继续通过。

## 7. 改动—回归与静态门禁

新增 `FormationProductSession` 映射规则，要求同时提供：

- focused FormationSession；
- P8.1 FormationMaterialAdapter；
- ShanmenItems；
- P8.0 FormationDeployment。

最终结果：

- regression map JSON：PASS，`38` rules；
- mapping self-test：`37/37 PASS`；
- `REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=4 Logs=2`；
- coverage log SHA-256：`42A2F7113E7736E0B0FA3D08CB21CEB5935418F17AFFC122A33B024515CCE814`；
- self-test log SHA-256：`CAD60BE85AA404F5CCD317FF3F489CC67CD746685E53F82C3C7DEC3B9E9FBB65`；
- production boundary scan 未发现 `UWorld`、`AActor`、spawn、Tick/timer、RNG 或 legacy item subsystem；`rg` no-match 原生退出码 `1` 为预期；
- tracked 与 untracked source 的 whitespace/diff check：PASS；
- `git diff --check`：原生退出码 `0`。

## 8. 构建

统一命令：

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

| 构建 | Result | Native exit | 时间 | UBT SHA-256 |
|---|---|---:|---:|---|
| Editor first | Succeeded | 0 | 10.57s | 首次日志未作为最终工件保留 |
| Editor final | Succeeded | 0 | 9.10s | `AC71CB6BA2C98C17624970B131AC63F90A1B356025EAD03341B77B8CC598CD4E` |
| Game final | Succeeded | 0 | 22.01s | `CBB54F063DF5CBF258F32AC200EAEBE36C5A65584226AFC83111C06C587F3818` |

首次源码集成、focused automation 与 full automation 均一次成功，没有源码、UHT、link、测试、环境或内存失败。提交前审查补充了 terminal Session 对历史 prepare audit 的 replay，并在该修正后重新执行最终 Editor、focused 与 full 验证。

- Editor module：`10879488` bytes，UTC `2026-08-29T15:56:44.5628557Z`；
- Game executable：`352126976` bytes，UTC `2026-08-29T15:59:18.9014753Z`。

## 9. 修改范围、兼容性与 P/F 边界

修改范围：

- `demo_mapShanmenFormationProductSession.h/.cpp`；
- `demo_mapShanmenFormationProductSessionTests.cpp`；
- regression map 与 self-test；
- 本 Report 与同名 Development Log。

未修改 Build.cs、GameplayTags、Content、schema、ShanmenItems、P8.0/P8.1、GameMode、输入或旧产品链。长期未跟踪的用户及 0.0.9B 工件未修改、未 stage。

本轮仅执行 P 阶段源码、静态检查、`-NullRHI` Automation 与 Editor/Game Development build。未启动 Unreal Editor UI、PIE、Standalone、产品 executable、真实输入、截图、Smoke、Cook 或 Package。

## 10. 下一阶段与 GitHub

P8.3 建议建立 Formation world-delivery seam：把 Session 已提交的 anchor audit 投影为可验证的 world placement intent/receipt，并定义 spawn 失败与重复投放的幂等边界；材料、deployment 与 action 继续由 P8.2 所有，不在世界层创建第二套状态。阵法效果、区域规则、UI 与正式配方继续后置。

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p8-2-formation-product-session/Docs/Report/Dev.D.UE.0.0.10.P8.2.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p8-2-formation-product-session/Docs/Log/Dev.D.UE.0.0.10.P8.2.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p8-2-formation-product-session>
