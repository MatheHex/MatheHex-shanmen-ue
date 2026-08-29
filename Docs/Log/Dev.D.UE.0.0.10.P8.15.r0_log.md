# Dev.D.UE.0.0.10.P8.15.r0 Development Log

## 身份

- 阶段：`Dev.D.UE.0.0.10.P8.15.r0`；
- 基线：`2b2c3855c47a0daa7fad11676e46d54365f9035f`（P8.14）；
- 分支：`agent/0.0.10-p8-15-formation-influence-executor-adapter`；
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`；
- 引擎：Unreal Engine `5.8`；
- 收口日期：`2026-08-29`。

## 目标

在 ProductHost 外定义窄、可注入的 opaque influence executor contract，用无状态 adapter 驱动 Host ledger 的 `pending → execute → acknowledgement`，同时保证 exact attempt replay 不重复调用 executor。

## 设计记录

### adapter 无 authority 状态

Adapter class 只有静态 `TryExecute`，没有 queue、map、retry counter 或 receipt cache。所有 canonical order 与 attempt history仍由 P8.13 ledger拥有；所有 Host mutation仍通过 P8.14 `TryAcknowledgeInfluence`。

### caller identity 与 executor evidence 分离

Caller 提供 intent/attempt identity。Adapter 从 Host 读取 ledger ID 与完整 pending intent，executor 只接收该 immutable invocation。Executor receipt 由外部实现生成，但必须回指相同 ledger、intent、attempt，outcome 只能是 retryable failure 或 success。

### replay-before-execute

Ledger 增加只读 `TryGetIntent` 与 `TryGetAttemptReceipt`。Adapter 在 executor 调用前查询 exact attempt；找到时重建 opaque executor receipt并走 Host exact acknowledgement replay，因此成功、retry 和 sealed history 均不会产生重复 executor side effect。

### fail-closed fence

Stale correlation、invalid command、无 ledger、无 pending、out-of-order intent、executor rejection、receipt mismatch 和 Host acknowledgement rejection均返回 typed failure。只有 structurally matched evidence 才进入 ledger。

## 实现文件

- `Source/demo_map/demo_mapShanmenFormationInfluenceExecutorAdapter.h`；
- `Source/demo_map/demo_mapShanmenFormationInfluenceExecutorAdapter.cpp`；
- `Source/demo_map/demo_mapShanmenFormationInfluenceDispatchLedger.h`；
- `Source/demo_map/demo_mapShanmenFormationInfluenceDispatchLedger.cpp`；
- `Source/demo_map/demo_mapShanmenFormationProductHostTests.cpp`；
- `Scripts/ShanmenRegressionMap.json`；
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`；
- 本 Report 与本 Log。

## 执行序列

1. 审查 P8.13 acknowledgement/replay 顺序与 P8.14 Host public evidence seam。
2. 定义 execution command、immutable invocation、opaque executor receipt/result 与 injected interface。
3. 实现 stateless adapter 的 existing-attempt replay、canonical pending fence、executor validation 与 Host acknowledgement。
4. 为 ledger 增加 immutable intent／attempt receipt query，不改变 dispatch mutation semantics。
5. 在既有真实 ProductHost fixture 中新增 success、retry、order/evidence fence 与 terminal drain 四项 fake-executor tests。
6. 首次 Editor source build `8 actions` 成功；新 focused 首轮 `4/4`。
7. 加强 rejection 后 pending 与 sealed replay assertions，Editor rebuild成功，最终 focused仍为 `4/4`。
8. dispatch `4/4`、influence Host `4/4`、full `270/270` 成功。
9. 新增 regression rule 与 fail-closed self-test；Rules `49`、self-test `59/59`、changed-file gate通过。
10. 完成 boundary scan、`git diff --check` 与最终 Editor／Game Development 构建。

## 最终 Automation

命令模板：

```powershell
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests <group>" -TestExit="Automation Test Queue Empty" -AbsLog=<log>
```

| Log | Group | Success | Fail | Queue | Exit | SHA-256 |
|---|---|---:|---:|---:|---:|---|
| `P8.15-FormationInfluenceExecutor-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceExecutor` | 4 | 0 | 1 | 0 | `C2DCA9460CEF5E1A895FC92231BEDC66F1E733A8EA9ADAC256D70643D03F3B42` |
| `P8.15-FormationInfluenceDispatch-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceDispatch` | 4 | 0 | 1 | 0 | `83879F62313E0263A3113A166C78A990E3491FE671598F00044042525DE26C7F` |
| `P8.15-FormationInfluenceHost-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceHost` | 4 | 0 | 1 | 0 | `DA9239BF63A7FBAA6AD04E5AE8F5A75E17B63F611EA41B0384B22597B8BFE82A` |
| `P8.15-Shanmen-full-final.log` | `Shanmen.0_0_10` | 270 | 0 | 1 | 0 | `310171421F11B8EB8FC99FE178E31AF226A34307E7C74E04B90EF076DA5EC450` |

Fatal／Unhandled／Ensure：四份均 `0`。UnifiedError 启动 self-test 固定 `Condition failed`：各 `13`。

## Regression gate

```text
REGRESSION_MAP_JSON: PASS Rules=49
SELF_TEST: PASS 59/59
REGRESSION_COVERAGE: PASS Changed=7 Rules=3 Required=17 Logs=4
REGRESSION_COVERAGE: PASS Changed=9 Rules=3 Required=17 Logs=4
```

第一条为 Source／Scripts gate；第二条为加入本 Report／Log 后的最终 exact-staged gate。

## 首次失败证据

没有 Automation、源码编译或最终构建失败。首次 focused 已完成 `4/4`。随后只增强测试观察点和 sealed replay位置，没有修复产品失败。

## 静态边界

新 adapter 生产文件扫描：

```text
UWorld/AActor/UObject/SpawnActor = 0
Tick/Timer/Async/RNG = 0
ApplyDamage/GameplayEffect/AbilitySystem = 0
SaveGame/ProfileRepository = 0
adapter-owned mutable authority state = 0
```

`git diff --check`：PASS。

## 构建

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

| Run | Result | Exit | Evidence SHA-256 |
|---|---|---:|---|
| Editor initial implementation | 8 actions / 41.50s / Succeeded | 0 | console evidence verified |
| Editor strengthened tests | 4 actions / 6.39s / Succeeded | 0 | console evidence verified |
| Editor final | up to date / 0.91s / Succeeded | 0 | `5B22E56100E5FCEE487E55FD1C44DC1F06029C709D3F0977EE48BA59368D3F38` |
| Game final | 7 actions / 30.12s / Succeeded | 0 | `5E48F09A90AEBBEDC5215894C369A52BC3DA780040CE4AEB3CEEEBB3063C25C7` |

- `UnrealEditor-demo_map.dll`：`11459072` bytes，SHA-256 `4292E0631D8FD0C61B2870A84406717BF2557D9C9971C496105DDC4941CB2ADA`；
- `demo_map.exe`：`352599552` bytes，SHA-256 `8FC98BF4E14141F5F8E3193DA8920CFD1B424504707F1DFE4EA2620BF4A5D001`。

## P/F 边界

仅执行源代码开发、静态审查、无头 Automation、regression gate、`git diff --check` 与 Editor/Game Development构建。未启动 Editor UI、PIE、Standalone、产品 executable、真实输入、截图、Smoke、Cook 或 Package。

## 后置

P8.16：在 injected interface 后实现纯值、内存内 influence lease executor，冻结 Apply/Remove lease key、幂等、冲突与 opaque receipt；Actor、GAS 与正式 magnitude content继续后置。
