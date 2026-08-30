# Dev.D.UE.0.0.10.P8.19.r0 Development Log

## 身份

- 阶段：`Dev.D.UE.0.0.10.P8.19.r0`；
- 基线：`ac7cd2b86bb237e803f14f35550af61e04ddaaf7`（P8.18）；
- 分支：`agent/0.0.10-p8-19-formation-influence-execution-service`；
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`；
- 引擎：Unreal Engine `5.8`；
- 收口日期：`2026-08-29`。

## 目标

把 P8.18 deterministic Router 与 P8.17 concrete Runtime 组合为一个纯值 single-step Service。每次 caller request 最多 route／execute 一条 canonical intent；对 exact replay、binding、transaction rollback、retry-only recovery 与 sealed success history 提供统一可验证契约。

## 设计记录

### one request, one step

Service 不自行读取 Host pending 队列，也不生成 request。一个 API 调用只把调用方提供的一条 request 交给 Router，并把成功 route 的同一 command 交给 Runtime。

### composed invariant

Service 有效状态要求 Router 与 Runtime 各自有效、绑定状态相等，并在绑定后共享完全相同的 `RunCorrelation + LedgerId`。该不变量把 routing identity 与 semantic lease lifetime锁在同一 Host authority 上。

### candidate-copy transaction

每次调用在完整 Service 副本上执行。route rejection 直接丢弃候选；execution 后只有候选组合状态有效才提交。success-history late attach 会形成 Router 已恢复、Runtime 仍未绑定的候选，因此整体回滚，不把半绑定状态泄漏给下一次调用。

### replay and retry recovery

bound Service 的 exact replay同时由 Router 与 Runtime replay，不调用 executor。retry-only history 不减少 Host pending count、也不产生 semantic lease，因此 fresh Service可安全绑定旧 attempt evidence；后续新 request 使用新 attempt完成执行。

### authority boundary

Service拥有 Router 与 Runtime 值状态，但不拥有 ProductHost。Host继续拥有 pending order、attempt receipts、acknowledgement、seal 与 teardown；caller继续拥有 request identity、节奏与生命周期命令。

## 实现文件

- `Source/demo_map/demo_mapShanmenFormationInfluenceExecutionService.h`；
- `Source/demo_map/demo_mapShanmenFormationInfluenceExecutionService.cpp`；
- `Source/demo_map/demo_mapShanmenFormationProductHostTests.cpp`；
- `Scripts/ShanmenRegressionMap.json`；
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`；
- 本 Report 与本 Log。

## 执行序列

1. 审查 P8.17 Runtime、P8.18 Router、P8.15 adapter 与 Host ledger 的 binding／late-attach 语义。
2. 定义 Service status/result、组合 `IsValid()` 与 candidate-copy transaction。
3. 实现 route rejection 无状态、execution rejection 条件提交、组合不变量失败整体回滚、success 双 receipt 返回。
4. 新增 single-step/replay、route/binding fence、late-attach/retry recovery、terminal/sealed replay 四项测试。
5. Editor 首次编译 `5 actions / 34.51s` 成功；Service 首轮 `4/4`。
6. 回归规则升至 `53`，self-test 升至 `67/67`。
7. 首批七组测试 case 全绿，但 `ExecCmds` 的显式 `Quit` 抢先结束日志，changed-file gate 因缺 Queue Empty 标记退出 `1`。
8. 保留首批失败证据，移除 `Quit`，重新运行六组 focused 与 full `286/286`；七份日志各有一个 Queue Empty 标记。
9. changed-file gate 通过：Source／Scripts `Changed=5 / Rules=2 / Required=21 / Logs=7`。
10. 完成 boundary scan、`git diff --check` 与最终 Editor／Game Development 构建。

## 最终 Automation

命令模板：

```powershell
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -ExecCmds="Automation RunTests <group>" -TestExit="Automation Test Queue Empty" -AbsLog=<log>
```

| Log | Group | Success | Fail | Queue | Exit | SHA-256 |
|---|---|---:|---:|---:|---:|---|
| `P8.19-FormationInfluenceExecutionService-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceExecutionService` | 4 | 0 | 1 | 0 | `8FB57B120B3D815AD21FF71275E37F514B3FCE282CA16B4E212A9D73AE9F94BC` |
| `P8.19-FormationInfluenceExecutionRouter-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceExecutionRouter` | 4 | 0 | 1 | 0 | `72005B2524832D554EDE3D4F42AEACB63C48B8E13C2A391B706718DC6F84F509` |
| `P8.19-FormationInfluenceProductRuntime-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceProductRuntime` | 4 | 0 | 1 | 0 | `96ABD64B5986BDD6A35658799133179BEC048F8D40F0CB3F9208CA210958D003` |
| `P8.19-FormationInfluenceLeaseExecutor-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceLeaseExecutor` | 4 | 0 | 1 | 0 | `5875A5E361A4124A4B01144CAD09E04DA8D6528FC9F672D8B7145E2BDA088E4D` |
| `P8.19-FormationInfluenceExecutor-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceExecutor` | 4 | 0 | 1 | 0 | `F80EA83F54BBE4CE5FE8194AA01BB4D6157813230CFC22822A8C2E7AF0348345` |
| `P8.19-FormationInfluenceHost-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceHost` | 4 | 0 | 1 | 0 | `B6A55E111902C0036435BEF02F9790DDBC197A28144DD729D2590A4DD44DC13B` |
| `P8.19-Shanmen-full-final.log` | `Shanmen.0_0_10` | 286 | 0 | 1 | 0 | `136AB3CF2FBD81EEA2520956B82BF84B7E865939B951348FFC5798C3C988F34C` |

Fatal／Unhandled／Ensure：七份均 `0`。UnifiedError 启动 self-test 固定 `Condition failed`：各 `13`。

## Regression gate

```text
REGRESSION_MAP_JSON: PASS Rules=53
SELF_TEST: PASS 67/67
REGRESSION_COVERAGE: PASS Changed=5 Rules=2 Required=21 Logs=7
REGRESSION_COVERAGE: PASS Changed=7 Rules=2 Required=21 Logs=7
```

第一条 coverage 为 Source／Scripts，第二条为加入 Report／Log 后的最终 exact-staged set。

- mapping SHA-256：`CD7E40DFB7ECC1B8686895C7997BEE7B99A608AED1538AE6C0D928E604CB48ED`；
- self-test SHA-256：`87AE01776DE7E3647738FF16444BA11BF6D1E7EC6BCC0A1AAEDBA4EF91B804BC`。

## 首次失败证据

首批 Automation 命令错误地使用 `-ExecCmds="Automation RunTests <group>;Quit"`。七个进程原生退出码均为 `0`，测试分别 `4/4` 与 `286/286`，但都在 Queue Empty marker 写入前退出；changed-file validator 报 `queue-empty marker missing` 并退出 `1`。失败日志保留为：

| Preserved log | SHA-256 |
|---|---|
| `P8.19-FormationInfluenceExecutionService-queue-marker-missing.log` | `E8BE03FEB9B1F4974D9BFE7077AF5E52531273AD09CB2D8AE52310AB1137838C` |
| `P8.19-FormationInfluenceExecutionRouter-queue-marker-missing.log` | `2F37C05C42DCA9415BC80A764B333AFDDCFE473884CB4B4ED9FA39279E8BBFD0` |
| `P8.19-FormationInfluenceProductRuntime-queue-marker-missing.log` | `4A273702296954E12AC42640B402A77BC1E81F797385E23EBCA5CDBA9127989C` |
| `P8.19-FormationInfluenceLeaseExecutor-queue-marker-missing.log` | `578DB3CEF4CE363F5F702B1AD8915E918308D982FC135321EC46BA75F7B9ABD2` |
| `P8.19-FormationInfluenceExecutor-queue-marker-missing.log` | `F24FAD5EB5FFB0F9ED7E6204965B5AED7D17C9F022752BF7EA91C2CC996A5302` |
| `P8.19-FormationInfluenceHost-queue-marker-missing.log` | `1CD6CA1D90224F70287AD7CC8CE6E916A42BB53923918DC4DE2E39C3EB35D70B` |
| `P8.19-Shanmen-full-queue-marker-missing.log` | `8A661E8A84AAD02498F87859106A6DF7BA1682004222B1A34A4E5130ED092205` |

修正为只依赖 `-TestExit="Automation Test Queue Empty"` 后，七份最终证据通过。产品源码无编译失败，Automation case 无失败。

## 静态边界

新 Service 生产文件扫描：

```text
UWorld/AActor/UObject = 0
AbilitySystem/GameplayEffect = 0
Timer/Async/RNG = 0
Spawn/Damage/Persistence = 0
Loop = 0
```

`git diff --check`：PASS。

## 构建

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

| Run | Result | Exit | Evidence SHA-256 |
|---|---|---:|---|
| Editor initial Service | 5 actions / 34.51s / Succeeded | 0 | console evidence |
| Editor final | up to date / 0.97s / Succeeded | 0 | `9D47BD84462996A536EE364C84852237F11B8D7DA18E22FF6B4C3C85E6C99BC5` |
| Game final | 4 actions / 26.42s / Succeeded | 0 | `294CDA401693E0DA87034A0A00C05B422D7FFBDC5B8B140D5E34E081E81D5EB4` |

- `UnrealEditor-demo_map.dll`：`11577344` bytes，SHA-256 `BA0EA44BD7A4ACDE75A0D215A298DB974E32D77ED7AE6DD5071647B3A27F0A13`；
- `demo_map.exe`：`352714752` bytes，SHA-256 `DFD9D02524726E4817C2CF0EBBF1386359A5AC993A5AFFB87DD5E3ECE535326E`。

## P/F 边界

仅执行源代码开发、静态审查、无头 Automation、regression gate、`git diff --check` 与 Editor/Game Development 构建。未启动 Editor UI、PIE、Standalone、产品 executable、真实输入、截图、Smoke、Cook 或 Package。

## 后置

P8.20：增加 caller-driven lifecycle execution coordinator，显式接受 step request 与 terminal request；继续保持一次一条、无 Tick、无隐式 retry、无后台 drain、无 Host ownership、无 Actor discovery、无 GAS 与无正式数值。
