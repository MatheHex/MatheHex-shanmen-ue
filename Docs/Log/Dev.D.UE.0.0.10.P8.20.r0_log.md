# Dev.D.UE.0.0.10.P8.20.r0 Development Log

## 身份

- 阶段：`Dev.D.UE.0.0.10.P8.20.r0`；
- 基线：`ea367f8993b20259c9cdc8bb1595f75d1e8c6cf7`（P8.19）；
- 分支：`agent/0.0.10-p8-20-formation-influence-lifecycle-coordinator`；
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`；
- 引擎：Unreal Engine `5.8`；
- 收口日期：`2026-08-29`。

## 目标

在 P8.19 single-step Service 外增加纯值 lifecycle coordinator。由 caller 显式推进单条执行、terminal intent publication、逐条 drain、seal 与 World teardown；保留 Host authority、明确 forward-only recovery，并禁止自动循环或隐式后台行为。

## 设计记录

### explicit command surface

Coordinator 只提供 `TryExecuteStep`、`TryPrepareTerminal` 与 `TrySealAndEnd`。它不读取下一 intent、不生成 request、不循环、不自动重试。terminal preparation 只在 Host ledger 尾部发布 canonical Remove intents。

### asymmetric valid binding

terminal 可以先于第一条 Service step 发生，因此 Coordinator bound + Service unbound 是合法状态；Service bound + Coordinator unbound 永远非法。两者一旦都绑定，必须共享完全相同的 `RunCorrelation + LedgerId`。

### candidate-copy and authority receipts

Coordinator 自身状态通过候选副本提交。每个结果保留 nested Service、terminal preparation、seal 与 end receipt，并额外公开 `bCoordinatorStateCommitted`。Host 的 pending、attempt、seal 与 teardown mutation 仍由 Host 自己决定。

### forward-only terminal recovery

Host seal 与 session end 成功后，World teardown 仍可能失败。Coordinator 在 seal 成功后保留 binding；不尝试伪回滚 Host。后续 exact retry 以相同 Host／correlation 和有效 World 恢复，完成后再调用得到 replay。

## 实现文件

- `Source/demo_map/demo_mapShanmenFormationInfluenceLifecycleCoordinator.h`（106 行）；
- `Source/demo_map/demo_mapShanmenFormationInfluenceLifecycleCoordinator.cpp`（231 行）；
- `Source/demo_map/demo_mapShanmenFormationProductHostTests.cpp`；
- `Scripts/ShanmenRegressionMap.json`；
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`；
- 本 Report 与本 Log。

## 执行序列

1. 审查 ProductHost 的 terminal preparation、seal、session end 与 retry teardown 语义，以及 P8.19 Service binding／replay 契约。
2. 定义 lifecycle status/result、三项显式 API、组合 `IsValid()` 与 immutable Host binding。
3. 实现 single-step delegation、terminal publication、seal/end 与 forward-only recovery。
4. 新增 explicit lifecycle、no implicit drain/order fence、binding/late attachment、completion recovery 四项测试。
5. Editor 首次编译 `5 actions / 19.30s` 成功；Coordinator 首轮 `4/4`。
6. 回归规则升至 `54`，PowerShell 7 self-test 升至 `69/69`。
7. 八组最终 Automation 全绿，全量由 `286` 增至 `290`。
8. changed-file gate 通过：Source／Scripts `Changed=5 / Rules=2 / Required=22 / Logs=8`。
9. 完成 boundary scan、`git diff --check` 与最终 Editor／Game Development 构建。

## 最终 Automation

命令模板：

```powershell
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -ExecCmds="Automation RunTests <group>" -TestExit="Automation Test Queue Empty" -AbsLog=<log>
```

| Log | Group | Success | Fail | Queue | Exit | SHA-256 |
|---|---|---:|---:|---:|---:|---|
| `P8.20-FormationInfluenceLifecycleCoordinator-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceLifecycleCoordinator` | 4 | 0 | 1 | 0 | `6DFFCDDA3A53CCE357D681189C11599B94D4B26FA2A895DFE5330A40276DA7FE` |
| `P8.20-FormationInfluenceExecutionService-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceExecutionService` | 4 | 0 | 1 | 0 | `171A8536390C727CB749A250A8D1FACCCE3FCEA05B258E88636EED08FA7AF640` |
| `P8.20-FormationInfluenceExecutionRouter-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceExecutionRouter` | 4 | 0 | 1 | 0 | `4025EF3C05F14679CBBEEFD332DFEE1910EBE447A01B307819777178AB867E1E` |
| `P8.20-FormationInfluenceProductRuntime-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceProductRuntime` | 4 | 0 | 1 | 0 | `C7F59BB2AE395426CE6A292D62FB67782D41F8DE6A20B7B327BBC14FA8847838` |
| `P8.20-FormationInfluenceHost-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceHost` | 4 | 0 | 1 | 0 | `B5513322E9EBFA88F3F988D0474F7B653A61BC0B874234FBC280DE2B7E9C5349` |
| `P8.20-FormationSession-final.log` | `Shanmen.0_0_10.Product.FormationSession` | 4 | 0 | 1 | 0 | `EBA1A528102A7F1F6878FCEED8C3539195E5248795428A173D38278F6AC2B32D` |
| `P8.20-FormationWorldDelivery-final.log` | `Shanmen.0_0_10.Product.FormationWorldDelivery` | 4 | 0 | 1 | 0 | `2BC92649837D1F4A4358621E2FDAAC10D17A0E1FA5929BD667EBD3925670A1F9` |
| `P8.20-Shanmen-full-final.log` | `Shanmen.0_0_10` | 290 | 0 | 1 | 0 | `8003F026C506654F2D7C9E41A803284528B514268DDF806AFC63BE50F9F74A47` |

Fatal／Unhandled／Ensure：八份均 `0`。UnifiedError 启动 self-test 固定 `Condition failed`：各 `13`。

## Regression gate

```text
REGRESSION_MAP_JSON: PASS Rules=54
SELF_TEST: PASS 69/69
REGRESSION_COVERAGE: PASS Changed=5 Rules=2 Required=22 Logs=8
REGRESSION_COVERAGE: PASS Changed=7 Rules=2 Required=22 Logs=8
```

- mapping SHA-256：`A15E23C1A3DBCC99B7F1F3E5A7571E448F7B79A3D0DDC346C442458B652DCC7C`；
- self-test SHA-256：`D0BD4510617CD9F0D7505936C869637F24B0CC74D36D0B3A4326712F11A880E3`。

## 首次运行与调用器修正

- 产品源码首次 Editor 编译：`5 actions / 19.30s / exit 0`；
- Coordinator 首轮 Automation：`4/4 / Queue 1 / exit 0`，SHA-256 `7F78C8250266EC976F6B946766CEBC77A57C8BFF0351893BF5568E230D120836`；
- Windows PowerShell 5.1 无法解析校验脚本的 PowerShell 7 行首管道，解析退出 `1`；改用 `pwsh` 后 `69/69`；
- Windows PowerShell 向外部 `pwsh -File` 传数组时被展开为位置参数，coverage 参数绑定退出 `1`；改在 `pwsh` shell 内原生调用后通过。
- exact-staged gate 首次用 `*-final.log` 选入 Editor／Game 构建日志，校验器因两份日志没有 RunTests／Queue marker 正确退出 `1`；改为仅选择八份 Automation 日志后通过 `Changed=7 / Rules=2 / Required=22 / Logs=8`。

产品源码无编译失败，Automation case 无失败。三次失败均为校验调用器版本、参数传递或证据集合选择错误，不是工程实现错误。

## 静态边界

新 Coordinator 生产文件扫描：

```text
UWorld token = 3 (forward declaration + signatures)
World-> = 0
AActor/UObject = 0
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
| Editor initial Coordinator | 5 actions / 19.30s / Succeeded | 0 | console evidence |
| Editor final | up to date / 0.93s / Succeeded | 0 | `DCF5C61FED637CCD0352D1631463C4BD4569E6152026FC25B3DC0A3D3D2883F0` |
| Game final | 4 actions / 23.46s / Succeeded | 0 | `F1B5D60F4A58D42C9B36D4096CDAC36A524C45C43F648FD0C9B23337A3078454` |

- `UnrealEditor-demo_map.dll`：`11607040` bytes，SHA-256 `86705B586DE08AB72B51C8905FE90BB84F87C61474254F3EB2A6FF304D8EF60D`；
- `demo_map.exe`：`352740864` bytes，SHA-256 `7411ECC27CD86C7289C88217D233D6FE035F5A79E51F768ED79ADBDF6268D699`。

## P/F 边界

仅执行源代码开发、静态审查、无头 Automation、regression gate、`git diff --check` 与 Editor/Game Development 构建。未启动 Editor UI、PIE、Standalone、产品 executable、真实输入、截图、Smoke、Cook 或 Package。

## 后置

P8.21：增加 typed lifecycle command envelope/router；继续由 caller 显式提交每条命令，不引入自动 drain、隐式 retry、Host ownership、World discovery 或正式 GAS 数值。
