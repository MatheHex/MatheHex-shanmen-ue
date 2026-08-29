# Dev.D.UE.0.0.10.P8.1.r0 Development Log

## 身份

- 阶段：`Dev.D.UE.0.0.10.P8.1.r0`；
- 基线：`64b206b5cb8e13b62f75eccea7012c61bc86140f`（P8.0）；
- 分支：`agent/0.0.10-p8-1-formation-material-adapter`；
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`；
- 引擎：Unreal Engine `5.8`；
- 收口日期：`2026-08-29`。

## 目标与后置项

P8.0 已冻结 diagram、anchor requirements、deployment identity/state 与 fulfillment evidence seam。P8.1 只负责把一个 exact anchor requirement 转为既有 ShanmenItems durable active-Run Quantity intents，并在全部 commit 后生成该 evidence。

明确后置：Formation product coordinator、阵眼 Actor、投料/区域交互、一次成阵、阵法 effect provider、UI、正式阵图商品、配方与数值。

## 设计决策

### 单向 authority bridge

Adapter 位于 `demo_map` 产品层，读取 P8.0 Runtime definition 与 `Fdemo_mapShanmenRunCorrelation`，写入现有 `Udemo_mapShanmenItemAuthoritySubsystem`。它没有 item map、余额字段或持久化文件；ShanmenItems receipt 始终是材料状态唯一证据。

### Stable AttemptId

显式 AttemptId 进入 transaction identity。相同 attempt 可在持久化重启后重建相同 prepare/finalize IDs；已取消 attempt 不能改判 commit，新 attempt 不会与旧 terminal request 冲突。

### 多 stack allocation

按 frozen requirement order，再按 Run correlation 的 ordered inventory 分配。每条 line 保存 definition、physical ItemInstanceId、quantity、ExpectedQuantityBefore、IntentId、prepare request 和可恢复 terminal receipt。计划自校验 requirements 与 allocation 聚合完全守恒。

### Terminal recovery

Commit 发生后，当前 quantity 已变化，因此只从原 receipts 重建并强制向前完成。Cancel 不消耗 quantity，仍走 canonical allocator；这样既能识别全量 cancelled，也能在部分 cancellation 或 prepare rollback 后继续释放 pending lines。

## 实现范围

新增：

- `Source/demo_map/demo_mapShanmenFormationMaterialAdapter.h`；
- `Source/demo_map/demo_mapShanmenFormationMaterialAdapter.cpp`；
- `Source/demo_map/demo_mapShanmenFormationMaterialAdapterTests.cpp`。

适配器实现纯 `BuildPlan` / `BuildFinalizeRequest` / `BuildCommittedEvidence`，以及 Game Thread facade `PrepareMaterials` / `CommitMaterials` / `CancelMaterials`。Snapshot gate 同时验证 deployment、Run lifecycle、content、active-Run Quantity reservation、consumed history 和 pending-intent ownership。

FulfillmentId 绑定 exact transaction/attempt、deployment/anchor、每个 committed ReceiptId 和最终 authority revision。P8.0 仍会独立验证 line 聚合守恒，不信任 adapter 自述。

## 提交前审查修正

初版对 committed 与 cancelled terminal 共用 receipt-only branch。审查发现部分取消后，如果还有 line 未释放，旧分支可能不能重建完整计划。

最终规则：

- 任一 committed line：只前进 receipt reconstruction；
- cancel-only：canonical allocation reconstruction；
- 全部 cancelled：同 attempt terminal；
- 部分 cancelled：重放 prepare identity，再继续 deterministic cancel；
- committed + cancelled 混合：invalid authority state，失败关闭。

`CancelAndRetry` 测试改为先取消一条、capture/rebuild，再取消剩余两条，并验证新 AttemptId 可重新分配。

## 自动化

命令模板：

```powershell
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests <group>" -TestExit="Automation Test Queue Empty" -AbsLog=<log>
```

| 日志 | Group | Success | Fail | Exit | SHA-256 |
|---|---|---:|---:|---:|---|
| `FormationMaterialAdapter.log` | `Shanmen.0_0_10.Product.FormationMaterialAdapter` | 4 | 0 | 0 | `9B3D43975969174D2FED74854611EF87CAB023F3B9819DEA7AFC5F3642868EC4` |
| `Shanmen-0_0_10-Full.log` | `Shanmen.0_0_10` | 215 | 0 | 0 | `92F72D24E3C782A101BC6230FBD659EB6D2C23EFEF80DDF0AA69C058BB942B38` |

每份最终日志均为 one command、one queue-empty、Fail `0`、fatal/assert/ensure `0`。

## 测试内容

1. `AllocationAndDeterminism`：多材料、多 stack、stable order、same/new attempt identities；
2. `CommitEvidenceAndDeployment`：真实 repository prepare/commit receipts、quantity after、evidence 与 P8.0 anchor replay；
3. `CancelAndRetry`：部分取消重建、全量释放、零消耗、old/new attempt；
4. `RecoveryAndFailClosed`：部分 commit 前向恢复、foreign intent fence、unbound subsystem fail-closed。

## Changed-file regression gate

`FormationMaterialAdapter` rule 要求 focused adapter、Items parent 和 FormationDeployment parent。focused child 不能替代两个 parent；新增正向 broad-full 和反向 child-only self-test。

```text
REGRESSION_MAP_JSON: PASS
SELF_TEST: PASS 35/35
REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=3 Logs=2
```

- coverage SHA-256：`50291BE82BB077D71BD8ADCB23B425843867178E58948FDEA4D4C53BB90162D1`；
- self-test SHA-256：`9EEF51F542785282367E2BBBB01E5565503489EB95BB8050DBFC22533C85F8A1`。

## 构建与异常记录

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

| Target | Result | Exit | Total | UBT SHA-256 |
|---|---|---:|---:|---|
| Editor first | Succeeded | 0 | 14.41s | `5797E75C20CB5CFE3406A2E44BE269ADE5DCA51D013E178A9BEB0C3E369FEA78` |
| Editor final | Succeeded | 0 | 9.01s | `810FD63B0FBB3F59EFE6E1E6C056512E8A31EE65BD48AE853B3792FF35CF6DEF` |
| Game final | Succeeded | 0 | 14.90s | `19745712D66451B034AD9F6C6044E0F72057A632B86B64AA7C03E2027B8A3E6C` |

没有源码、UHT、link 或 automation 首次失败。一次外层 PowerShell wrapper 因读取空 `$LASTEXITCODE` 把已 PASS 的两个 `.ps1` 误报为 exit `1`；改用 `$?` 后相同检查 exit `0`。该误判未计作源码失败，也未修改产品逻辑来掩盖。

## 静态、兼容性与 P/F 边界

- map JSON parse：PASS；
- boundary `rg`：no match / exit `1`（预期）；
- `git diff --check`：exit `0`；
- 未修改 Build.cs、tags、schema、Content、ShanmenItems、P8.0 Runtime、GameMode、输入或旧产品链；
- P8.0 的 211 项既有 0.0.10 测试继续通过；
- 长期未跟踪用户文件未修改、未 stage。

本轮仅执行源码、静态门禁、`-NullRHI` Automation 与 Editor/Game Development build。未启动 Editor UI、PIE、Standalone、产品 executable、真实输入、截图、Smoke、Cook 或 Package。

## 下一步与 GitHub

P8.2 建议建立 Formation product coordinator/session，闭合 deployment begin、逐 anchor material transaction、anchor commit、cancel 与 recovery 的调用顺序；世界 Actor、交互和正式 content 继续后置。

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p8-1-formation-material-adapter/Docs/Report/Dev.D.UE.0.0.10.P8.1.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p8-1-formation-material-adapter/Docs/Log/Dev.D.UE.0.0.10.P8.1.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p8-1-formation-material-adapter>
