# Dev.D.UE.0.0.10.P8.13.r0 Development Log

## 身份

- 阶段：`Dev.D.UE.0.0.10.P8.13.r0`；
- 基线：`2cca45b3a2023527ed237fbe227a570b5184e6ff`（P8.12）；
- 分支：`agent/0.0.10-p8-13-formation-influence-dispatch-ledger`；
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`；
- 引擎：Unreal Engine `5.8`；
- 收口日期：`2026-08-29`。

## 目标

建立 P8.11 transition intents 与 P8.12 lifecycle reconciliation intents 的统一、纯值、deployment-scoped dispatch ledger，冻结 accept、pending、retry、acknowledge、replay 与 seal 语义。暂不接 ProductHost 或真实 effect executor。

## 设计记录

### 单一 scope authority

首个有效 source batch 派生 ledger identity 并冻结 Run／Owner／Source／Deployment。后续 source 必须精确同 scope；Area、policy、subject 与 operation 仍由各自 self-validating batch／intent 负责。

### source replay 与 intent ownership

Ledger 保存 source origin、source batch ID、canonical intent IDs 与派生 batch record ID。Exact replay 不重复入队；同一 source identity 的内容漂移或不同 source 对同一 intent identity 的争用 fail-closed。

### executor 保持 opaque

Ledger 不解释 GameplayEffect、GAS handle、Actor 或数值，只保存 `AttemptId`、opaque `ExecutorReceiptId` 与 outcome。Retryable failure 留在 pending；success 使 intent 离开 pending。每个 attempt receipt 由完整 evidence 确定性派生。

### seal 包含完整历史

Seal ID 顺序覆盖 accepted batch records 和全部 attempt receipts。只有所有 pending 成功后可 seal；valid no-op source 仍保留可审计 batch evidence 并可立即 seal。

## 实现文件

- `Source/demo_map/demo_mapShanmenFormationInfluenceDispatchLedger.h`；
- `Source/demo_map/demo_mapShanmenFormationInfluenceDispatchLedger.cpp`；
- `Source/demo_map/demo_mapShanmenFormationInfluenceDispatchLedgerTests.cpp`；
- `Scripts/ShanmenRegressionMap.json`；
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`；
- 本 Report 与本 Log。

## 执行序列

1. 审查 P8.11 transition batch、P8.12 reconciliation batch 与现有 ProductHost 边界。
2. 定义 dispatch scope、source origin、attempt command／receipt、submit／ack／seal status。
3. 实现两种 source overload、canonical pending、exact source replay 与 scope fence。
4. 实现 retryable failure、success acknowledgement、attempt conflict、success receipt 查询。
5. 实现 no-op seal、pending fence、sealed exact replay 与 deterministic seal identity。
6. 新增 4 个 focused tests，首次 Editor source build 成功。
7. focused `4/4` 与 full `262/262` case 成功。
8. 首次 changed-file gate 因测试命令附带 `;Quit` 而缺 queue-empty marker，原生退出码 `1`。
9. 去除 `;Quit`，依靠 `-TestExit` 重跑最终 focused/full，均退出码 `0` 且 queue marker `1`。
10. 扩展 regression map 与 self-test；Rules `48`，self-test `57/57`，changed-file gate 通过。
11. 完成 boundary scan、最终 Editor 与 Game Development 构建。

## 最终 Automation

命令模板：

```powershell
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests <group>" -TestExit="Automation Test Queue Empty" -AbsLog=<log>
```

| Log | Group | Success | Fail | Queue | Exit | SHA-256 |
|---|---|---:|---:|---:|---:|---|
| `P8.13-FormationInfluenceDispatch-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceDispatch` | 4 | 0 | 1 | 0 | `C8F7E94F975C47E9B1994171AC6BC98F49B3A4BC7379210C73F38F2AA8AD1169` |
| `P8.13-Shanmen-full-final.log` | `Shanmen.0_0_10` | 262 | 0 | 1 | 0 | `7DBD75A8CD01D39E262290F08303BEE9C2E0DA7ADCB81BEFB5B9BB09656B76AA` |

Fatal／Unhandled／Ensure：两份均 `0`。UnifiedError 启动 self-test 固定 `Condition failed`：各 `13`。

## Regression gate

```text
REGRESSION_MAP_JSON: PASS Rules=48
SELF_TEST: PASS 57/57
REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=12 Logs=2
REGRESSION_COVERAGE: PASS Changed=7 Rules=1 Required=12 Logs=2
```

第一条为生产与脚本路径，第二条为加入本 Report／Log 后的最终 exact-staged 结果。

Gate 要求以下十二组：

- `FormationInfluenceDispatch`；
- `FormationInfluenceReconciliation`；
- `FormationInfluenceIntents`；
- `FormationCoverageCoordinator`；
- `FormationCoverageTracker`；
- `FormationCoverageTransitions`；
- `FormationWorldCoverage`；
- `FormationAreaProvider`；
- `FormationProductHost`；
- `FormationWorldDelivery`；
- `WorldGameplay`；
- `CombatRuntime.FormationDeployment`。

## 首次失败证据

首次 focused 与 full 运行本身分别完成 `4/4`、`262/262`，退出码 `0`，但命令为 `Automation RunTests <group>;Quit`。Changed-file gate 原生退出码 `1`，精确错误：

```text
REGRESSION_COVERAGE: unhealthy evidence:
P8.13-FormationInfluenceDispatch-focused.log: queue-empty marker missing
P8.13-Shanmen-full.log: queue-empty marker missing
```

原因是显式 `Quit` 抢在 Automation command-line controller 写 queue completion marker 前结束。修复后没有改产品源码，只重跑正确命令并保留本节事实。

## 静态边界

生产文件扫描：

```text
demo_mapGameMode/UWorld/AActor/UObject/Tick/Timer/GetAllActors/SpawnActor = 0
ApplyDamage/GameplayEffect/AbilitySystem/RNG/Async/SaveGame/ProfileRepository = 0
```

`git diff --check`：PASS。

## 构建

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

| Run | Result | Exit | Evidence SHA-256 |
|---|---|---:|---|
| Editor initial source build | 5 actions / 9.68s / Succeeded | 0 | console evidence verified |
| Editor final | up to date / 0.89s / Succeeded | 0 | `943C5D502EE0E84D7260AABA70C033DCF5877389B31364A7A3B1500571BA301B` |
| Game final | 4 actions / 21.61s / Succeeded | 0 | `695DBF72F80286FD75C123ACB8F2D12C5EC752D7F3800BE561CB635ACA36D14B` |

- `UnrealEditor-demo_map.dll`：`11383808` bytes，SHA-256 `9DCC33E90457175B36D61E5CDA4C42AB96200E505DA504815D17FCE9D84A8EA9`；
- `demo_map.exe`：`352530944` bytes，SHA-256 `2517CAA9F881E81947E12770C7038B906940D1C461456FBA56F695B5DA825E1A`。

## P/F 边界

仅执行源代码开发、静态审查、无头 Automation、regression gate、`git diff --check` 与 Editor/Game Development 构建。未启动 Editor UI、PIE、Standalone、产品 executable、真实输入、截图、Smoke、Cook 或 Package。

## 后置

P8.14：由现有 Formation ProductHost 编排 transition／reconciliation planners 与 dispatch ledger，返回完整 host evidence；继续使用 opaque/fake executor，真实 GAS effect authority 后置。
