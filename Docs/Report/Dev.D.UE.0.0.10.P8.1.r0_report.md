# Dev.D.UE.0.0.10.P8.1.r0 Report

## 1. 结论

P8.1 已把 P8.0 阵眼材料 requirement 接入既有 `ShanmenItems` durable active-Run quantity intent，结论为 **PASS**。

新增 `Fdemo_mapShanmenFormationMaterialAdapter`。它只是一条从 FormationDeployment 到现有物品权威的单向桥：按稳定 Run inventory 顺序拆分多材料、多 stack 需求，durable prepare 后可 commit 或 cancel；只有每一条 exact physical stack 都 durable committed，才生成 P8.0 可接受的 fulfillment evidence。

本轮未修改 `ShanmenItems`、旧库存、Code A/Code B、存档 schema 或 P8.0 Runtime；也未创建阵眼 Actor、投料交互、阵法效果、UI 或正式配方。

## 2. 权威与事务边界

Adapter 不保存库存副本，也不直接修改 quantity。所有产品写入都调用现有：

- `PreparePreparedRunQuantityIntentDurable`；
- `FinalizePreparedRunQuantityIntentDurable(commit)`；
- `FinalizePreparedRunQuantityIntentDurable(cancel)`。

规划前必须同时证明：Run correlation 有效、精确 lifecycle receipt 仍为非 terminal、FormationDeployment 正处于 `Deploying`、action Run/owner/content 匹配、每个物理 item 属于稳定 Run inventory，且其 Quantity reservation 是既有 active-Run committed authority。

任何 foreign pending intent、重复/缺失 reservation、已结束 Run、错误 content、已提交阵眼或 stale lifecycle revision 都失败关闭，不写入任何第二套权威。

## 3. 确定性分配与身份

单个阵眼的 requirements 按 P8.0 frozen order 读取；每项材料再按 `OrderedRunInventoryItemInstanceIds` 的稳定顺序分配。一个 requirement 可以跨多个 stack，但同一物理 item 在一个 attempt 中只出现一次。

调用方必须提供显式稳定 `AttemptId`。TransactionId 绑定：

- Run correlation 与 ActiveRunId；
- DeploymentId 与 AnchorDefinitionId；
- AttemptId；
- content version/digest。

每个 line 的 IntentId、prepare RequestId 与 terminal RequestId 都由 canonical parts 派生。相同 attempt 在进程内或重启后产生相同身份；取消后若要重新尝试，必须使用新 AttemptId，避免旧 terminal decision 被误当成新事务。

## 4. Prepare、Commit、Cancel 与恢复

Prepare 逐 line 写入 durable intent。若某一 line 被拒绝，会立即对此前成功的 lines 发出 deterministic cancel；只有 cancellation 本身尚未 durable 时才返回 recovery-required。

Commit 一旦有任一 line 成功即进入只前进语义：后续不能 cancel，只能以原 ID replay 并补齐剩余 commit。取消不会消耗 quantity；部分取消可按原稳定顺序重建完整计划，并继续释放剩余 pending lines。全部 lines 已取消后，同 AttemptId 明确返回 `AttemptCancelled`。

提交前审查专门修复了一个恢复缺口：早期实现把“任一 Cancelled terminal”也放进 committed-only 重建分支，可能阻止部分取消继续。最终实现只对已发生 Commit 的 attempt 使用 receipt-only 重建；cancel-only attempt 仍走确定性 allocator，测试覆盖一次取消、重建、继续取消和新 attempt 重试。

## 5. Fulfillment evidence

`BuildCommittedEvidence` 逐 line 验证：

- prepare/finalize operation 与 phase；
- exact request、intent、item、amount、purpose；
- ActiveRunId 与 prepare identity；
- Deployment、anchor、owner、content。

全部验证通过后，才复制物理 item/material/quantity lines，并以 transaction、attempt、deployment、anchor、各 committed ReceiptId 和最终 authority revision 派生 FulfillmentId。该 evidence 已在测试中提交给 P8.0 `TryCommitAnchor`，且同 evidence replay 返回同一 anchor receipt。

## 6. 自动化证据

最终日志各只有一个 `RunTests`、一个 queue-empty、Fail `0`、fatal/assert/ensure `0`，进程原生退出码均为 `0`。

| Group / 日志 | Success | Fail | SHA-256 |
|---|---:|---:|---|
| `Shanmen.0_0_10.Product.FormationMaterialAdapter` / `FormationMaterialAdapter.log` | 4 | 0 | `9B3D43975969174D2FED74854611EF87CAB023F3B9819DEA7AFC5F3642868EC4` |
| `Shanmen.0_0_10` / `Shanmen-0_0_10-Full.log` | 215 | 0 | `92F72D24E3C782A101BC6230FBD659EB6D2C23EFEF80DDF0AA69C058BB942B38` |

四项新增测试覆盖：

1. 跨两个 wood stack + 一个 core stack 的稳定分配、same-attempt identity replay 与 new-attempt isolation；
2. exact durable commits 生成 fulfillment，并被 P8.0 anchor 状态机接受和重放；
3. 部分取消后的完整计划重建、全量释放、零消耗与新 attempt 重试；
4. 部分 commit 重建与前向完成、foreign pending intent fence、未绑定产品 authority 失败关闭。

完整 suite 从 P8.0 的 `211` 增至 `215`，此前测试全部继续通过。

## 7. 改动—回归与静态门禁

新增 `FormationMaterialAdapter` 映射规则，要求同时提供：

- focused adapter group；
- `Shanmen.0_0_10.Items`；
- `Shanmen.0_0_10.CombatRuntime.FormationDeployment`。

最终结果：

- regression map JSON：PASS；
- mapping self-test：`35/35 PASS`；
- `REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=3 Logs=2`；
- coverage log SHA-256：`50291BE82BB077D71BD8ADCB23B425843867178E58948FDEA4D4C53BB90162D1`；
- self-test log SHA-256：`9EEF51F542785282367E2BBBB01E5565503489EB95BB8050DBFC22533C85F8A1`；
- boundary scan 未发现 World/Actor/Tick/timer/spawn/damage、legacy item subsystem 或 RNG；`rg` no-match 原生退出码 `1` 为预期；
- `git diff --check`：原生退出码 `0`。

## 8. 构建与首次异常

统一命令：

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

| 构建 | Result | Native exit | 时间 | UBT SHA-256 |
|---|---|---:|---:|---|
| Editor first | Succeeded | 0 | 14.41s | `5797E75C20CB5CFE3406A2E44BE269ADE5DCA51D013E178A9BEB0C3E369FEA78` |
| Editor final（恢复修正后） | Succeeded | 0 | 9.01s | `810FD63B0FBB3F59EFE6E1E6C056512E8A31EE65BD48AE853B3792FF35CF6DEF` |
| Game final | Succeeded | 0 | 14.90s | `19745712D66451B034AD9F6C6044E0F72057A632B86B64AA7C03E2027B8A3E6C` |

首次 Editor integration 与首次 focused automation 都一次成功，没有源码或测试首次失败。

发生过一次外层 PowerShell 编排误判：两个回归脚本已输出 PASS，但 wrapper 对 PowerShell 脚本读取空的 `$LASTEXITCODE` 并错误返回 `1`。改用 `$?` 后相同门禁原生退出 `0`；这不是源码、测试、引擎或内存失败，未通过放宽门禁解决。

- Editor `UnrealEditor-demo_map.dll`：`10829824` bytes，UTC `2026-08-29T15:34:24.4890586Z`；
- Game `demo_map.exe`：`352082944` bytes，UTC `2026-08-29T15:36:05.5597266Z`。

## 9. 修改范围、兼容性与 P/F 边界

修改范围：

- `demo_mapShanmenFormationMaterialAdapter.h/.cpp`；
- `demo_mapShanmenFormationMaterialAdapterTests.cpp`；
- regression map 与 self-test；
- 本 Report 与同名 Development Log。

未修改 Build.cs、GameplayTags、Content、schema、ShanmenItems、P8.0 Runtime、GameMode、输入或旧产品链。长期未跟踪的用户及 0.0.9B 工件未修改、未 stage。

本轮仅执行 P 阶段源码、静态检查、`-NullRHI` Automation 与 Editor/Game Development build。未启动 Unreal Editor UI、PIE、Standalone、产品 executable、真实输入、截图、Smoke、Cook 或 Package。

## 10. 下一阶段与 GitHub

P8.2 建议建立无 UI 的 Formation product coordinator：把 action/deployment begin、逐阵眼 material prepare/commit、P8.0 anchor commit 与 cancel/recovery 串成一个显式 session；仍不创建世界 Actor 或正式配方。先闭合跨权威编排，再进入阵眼世界投放和交互。

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p8-1-formation-material-adapter/Docs/Report/Dev.D.UE.0.0.10.P8.1.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p8-1-formation-material-adapter/Docs/Log/Dev.D.UE.0.0.10.P8.1.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p8-1-formation-material-adapter>
