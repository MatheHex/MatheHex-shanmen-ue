# Dev.D.UE.0.0.10.P8.14.r0 Development Log

## 身份

- 阶段：`Dev.D.UE.0.0.10.P8.14.r0`；
- 基线：`b438078ad02f362bb1f78a3bc365b1e4d78f40c6`（P8.13）；
- 分支：`agent/0.0.10-p8-14-formation-influence-host-orchestration`；
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`；
- 引擎：Unreal Engine `5.8`；
- 收口日期：`2026-08-29`。

## 目标

由既有 Formation ProductHost 统一编排 P8.11 transition planner、P8.12 lifecycle reconciliation planner 与 P8.13 dispatch ledger，形成 Host-owned、事务化、可重放的 influence evidence；暂不实现真实 effect executor。

## 设计记录

### 候选 Host 事务

每次 coordinate/reset/terminal/ack/seal 都先复制当前 Host。coverage tracker、planner 与 ledger 仅在副本上推进；nested result 和最终 `Candidate.IsValid()` 全部成功后才提交副本。这样 planner 或 dispatch 的任何拒绝都不会留下半提交 baseline、scope 或 pending queue。

### 单一 influence authority

ledger 保持 ProductHost 私有，外部只能 `const` 读取统计/evidence 或 peek 下一条 pending intent。Host 一旦建立 influence authority，旧 raw coverage API 返回 `InfluenceOrchestrationRequired`，禁止绕过 planner 与 ledger。

### lifecycle gate

Reset 同时清除 tracker baseline 并派发 Remove；Terminal 先派发 Remove，active scope 在 Remove 成功确认且 ledger seal 前持续阻止 End／Cancel。成功 teardown 后保留 ledger 与 clear-batch evidence，使 source、acknowledgement、seal 与 terminal operation 都可 exact replay。

### policy 语义

Prime／Advance 要求 retained policy 精确一致。只有显式 Rebase 可以替换 policy，并生成 previous Remove + current Apply。Policy content 必须与冻结 Action content 相同。

## 实现文件

- `Source/demo_map/demo_mapShanmenFormationProductHost.h`；
- `Source/demo_map/demo_mapShanmenFormationProductHost.cpp`；
- `Source/demo_map/demo_mapShanmenFormationProductHostTests.cpp`；
- `Scripts/ShanmenRegressionMap.json`；
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`；
- 本 Report 与本 Log。

## 执行序列

1. 审查 ProductHost Start／placement／coverage／End／Cancel 状态机与 P8.11-P8.13 evidence shape。
2. 选择 ProductHost 私有 ledger 为唯一 orchestration seam，保留原 Host 与 tracker authority。
3. 新增 Host influence status/result、coordinate/reset/terminal/ack/seal API 与只读 evidence getter。
4. 实现候选 Host transaction、policy fence、raw coverage fence、clear-batch replay cache 与 terminal drain gate。
5. 扩展 `IsValid()`，校验 Action source、Run／Owner／Deployment、active scope、coverage baseline、clear batch 与 sealed state 的一致性。
6. 新增四项 Host focused tests；首次 implementation Editor build 与测试后 Editor rebuild 均成功。
7. focused `4/4`、既有 ProductHost `6/6` 与 full `266/266` 全部成功。
8. 扩展 regression map owned-contract 列表及 fail-closed self-test；Rules `48`、self-test `57/57`。
9. 完成 boundary scan、changed-file gate、`git diff --check` 与最终 Editor／Game Development 构建。

## 最终 Automation

命令模板：

```powershell
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests <group>" -TestExit="Automation Test Queue Empty" -AbsLog=<log>
```

| Log | Group | Success | Fail | Queue | Exit | SHA-256 |
|---|---|---:|---:|---:|---:|---|
| `P8.14-FormationInfluenceHost-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceHost` | 4 | 0 | 1 | 0 | `246095C99E01B6EDE365A0C844D292B738A32584240B8D75966FAB0C795A9B34` |
| `P8.14-FormationProductHost-final.log` | `Shanmen.0_0_10.Product.FormationProductHost` | 6 | 0 | 1 | 0 | `CCD167D180688B29467B3446DC69CC5068A9DEECA5BD1355404F09C0F5349AD4` |
| `P8.14-Shanmen-full-final.log` | `Shanmen.0_0_10` | 266 | 0 | 1 | 0 | `6D319EBAB9468CEB56026D7A9B2AE9D28A290B44A85E6FA2D3462C04FD1E7A99` |

Fatal／Unhandled／Ensure：三份均 `0`。UnifiedError 启动 self-test 固定 `Condition failed`：各 `13`。

## Regression gate

```text
REGRESSION_MAP_JSON: PASS Rules=48
SELF_TEST: PASS 57/57
REGRESSION_COVERAGE: PASS Changed=5 Rules=1 Required=16 Logs=3
REGRESSION_COVERAGE: PASS Changed=7 Rules=1 Required=16 Logs=3
```

十六组 required contracts：

- `FormationInfluenceHost`；
- `FormationInfluenceDispatch`；
- `FormationInfluenceReconciliation`；
- `FormationInfluenceIntents`；
- `FormationCoverageCoordinator`；
- `FormationCoverageTracker`；
- `FormationCoverageTransitions`；
- `FormationWorldCoverage`；
- `FormationAreaProvider`；
- `FormationProductHost`；
- `FormationMaterialAdapter`；
- `FormationSession`；
- `FormationWorldDelivery`；
- `Items`；
- `WorldGameplay`；
- `CombatRuntime.FormationDeployment`。

## 首次失败证据

没有 Automation、源码编译或最终构建失败。首次新测试执行前，静态自查修正 `TerminalDrainGate` 的 Session-state 读取时点，避免成功 teardown 覆盖对 blocked End 原子性的观察；产品逻辑未因此改变。

## 静态边界

本轮 Host 生产文件扫描：

```text
GameplayEffect/AbilitySystem/ApplyDamage = 0
FMath::Rand/FRandomStream/Async = 0
SaveGame/ProfileRepository = 0
mutable ledger exposure = 0
```

`git diff --check`：PASS。

## 构建

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

| Run | Result | Exit | Evidence SHA-256 |
|---|---|---:|---|
| Editor initial implementation | 5 actions / 16.74s / Succeeded | 0 | console evidence verified |
| Editor after focused tests | 4 actions / 5.96s / Succeeded | 0 | console evidence verified |
| Editor final | up to date / 0.96s / Succeeded | 0 | `376269C5757E07BF63F89D8CB9A2F4291DE329CC1D43C4B1FCDFAA9BC70839C4` |
| Game final | 4 actions / 20.68s / Succeeded | 0 | `96BF9715C89C431E76C41925BCB57E0BD02CB3A88DBCA023095CB0AF17072BEB` |

- `UnrealEditor-demo_map.dll`：`11431424` bytes，SHA-256 `5843D332144DCCD335525B318B9FC19FC5E3E0B16C46B02F49AA2BAE56A9A15B`；
- `demo_map.exe`：`352572416` bytes，SHA-256 `EDB02966F0A1DB6A938DC82BC36AB137DE6FDFDE2F31E69A829C89ADD838162F`。

## P/F 边界

仅执行源代码开发、静态审查、无头 Automation、regression gate、`git diff --check` 与 Editor/Game Development 构建。未启动 Editor UI、PIE、Standalone、产品 executable、真实输入、截图、Smoke、Cook 或 Package。

## 后置

P8.15：定义 narrow injectable influence executor adapter，以 fake executor 驱动 Host pending drain 与 receipt 回写；真实 GameplayEffect/GAS 与正式数值 content 继续后置。
