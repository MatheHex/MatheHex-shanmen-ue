# Dev.D.UE.0.0.10.P8.17.r0 Development Log

## 身份

- 阶段：`Dev.D.UE.0.0.10.P8.17.r0`；
- 基线：`abed1a7ecfdfd7537dc86875c8187b601987b1ec`（P8.16）；
- 分支：`agent/0.0.10-p8-17-formation-influence-product-runtime`；
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`；
- 引擎：Unreal Engine `5.8`；
- 收口日期：`2026-08-29`。

## 目标

为 P8.16 concrete lease executor提供稳定的 product lifetime 与 immutable Host identity binding，每次只执行一条 caller command，同时保持 ProductHost／ledger 的唯一 ordering、acknowledgement、retry、seal 与 teardown authority。

## 设计记录

### composition only

Runtime 只拥有 `Fdemo_mapShanmenFormationInfluenceLeaseExecutor`、bound correlation 与 bound ledger ID。ProductHost 始终以引用传入，runtime 不保存 Host pointer，也不复制任何 pending、receipt、scope 或 lifecycle state。

### explicit one-command seam

Caller 继续提供完整 execution command。Runtime 不 peek-and-loop，也不生成 AttemptId；每个调用最多经过 adapter 一次。Out-of-order first call 未触发 executor时，临时 binding 会被清除。

### safe attachment fence

第一次绑定要求 ledger 的 pending count等于 total intent count，等价于尚无 success acknowledgement。Retry receipt 不减少 pending，因此 retry-only Host可以安全绑定；已有成功 mutation 的 Host必须由原 runtime继续，不能用空 executor接管。

### stable replay lifetime

绑定后 exact Host replay可以在 pending 清空、seal 或 teardown 后继续，adapter直接使用 ledger evidence，runtime-owned executor不发生第二次 mutation。

## 实现文件

- `Source/demo_map/demo_mapShanmenFormationInfluenceProductRuntime.h`；
- `Source/demo_map/demo_mapShanmenFormationInfluenceProductRuntime.cpp`；
- `Source/demo_map/demo_mapShanmenFormationProductHostTests.cpp`；
- `Scripts/ShanmenRegressionMap.json`；
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`；
- 本 Report 与本 Log。

## 执行序列

1. 审查 ProductHost public contract、P8.15 adapter 与相邻 product controller ownership 模式。
2. 定义 run-scoped runtime，冻结 correlation／ledger binding并私有持有 concrete executor。
3. 实现单 command delegation、foreign binding fence、first-rejection unbind 与 success-history late-attach fence。
4. 在真实 ProductHost fixture 中新增 single-step/replay、binding/correlation、late attachment 与 terminal drain 四项测试。
5. 首次 Editor source build `5 actions` 成功；新 focused 首轮 `4/4`。
6. 补充 retry-only history 正向 attach 证据；Editor rebuild `4 actions` 成功。
7. 最终 runtime／lease／adapter／Host 各 `4/4`，full `278/278` 成功。
8. 新增 runtime regression rule 与 fail-closed self-test；Rules `51`、self-test `63/63`、changed-file gate通过。
9. 完成 boundary scan、`git diff --check` 与最终 Editor／Game Development 构建。

## 最终 Automation

命令模板：

```powershell
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -ExecCmds="Automation RunTests <group>" -TestExit="Automation Test Queue Empty" -AbsLog=<log>
```

| Log | Group | Success | Fail | Queue | Exit | SHA-256 |
|---|---|---:|---:|---:|---:|---|
| `P8.17-FormationInfluenceProductRuntime-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceProductRuntime` | 4 | 0 | 1 | 0 | `2CE1A2426B12BAE929EE22FA7004173E3112CD99891C440BB06B485D1483A910` |
| `P8.17-FormationInfluenceLeaseExecutor-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceLeaseExecutor` | 4 | 0 | 1 | 0 | `8A8CC9CDF80E56C28CDAC980FF923650D8C33E90D6E252B112D7CF688F1FE687` |
| `P8.17-FormationInfluenceExecutor-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceExecutor` | 4 | 0 | 1 | 0 | `3D2F0C8A2AA5189D47D84404FDF6E82DE54B0E4E1FEBF0D576734A44553B6955` |
| `P8.17-FormationInfluenceHost-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceHost` | 4 | 0 | 1 | 0 | `46AA8D150B7D235A496BFFA894E9F01A99E6B8EEE18E55CC3BFD381A6976204A` |
| `P8.17-Shanmen-full-final.log` | `Shanmen.0_0_10` | 278 | 0 | 1 | 0 | `AD07B56C02513915A9839350412DFCE4967745915F9EC940C7C68DA0979AC4D6` |

Fatal／Unhandled／Ensure：五份均 `0`。UnifiedError 启动 self-test 固定 `Condition failed`：各 `13`。

## Regression gate

```text
REGRESSION_MAP_JSON: PASS Rules=51
SELF_TEST: PASS 63/63
REGRESSION_COVERAGE: PASS Changed=5 Rules=2 Required=19 Logs=5
REGRESSION_COVERAGE: PASS Changed=7 Rules=2 Required=19 Logs=5
```

第一条 coverage 为 Source／Scripts；第二条为加入 Report／Log 后的最终 exact-staged set。

## 首次失败证据

没有 Automation、源码编译或最终构建失败。首次 runtime focused 即 `4/4`。首轮后只增加 retry-only late attachment 的正向测试，产品实现未修改。

## 静态边界

新 product runtime 生产文件扫描：

```text
UWorld/AActor/UObject = 0
AbilitySystem/GameplayEffect = 0
Timer/Async/RNG = 0
Spawn/Damage/Persistence = 0
runtime-owned ProductHost/pending/seal authority = 0
```

`git diff --check`：PASS。

## 构建

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

| Run | Result | Exit | Evidence SHA-256 |
|---|---|---:|---|
| Editor initial implementation | 5 actions / 16.35s / Succeeded | 0 | console evidence verified |
| Editor strengthened tests | 4 actions / 7.08s / Succeeded | 0 | console evidence verified |
| Editor final | up to date / 0.92s / Succeeded | 0 | `F6AF8ACEBFC6D9B0C7BFE1A185950CE14BF18BD98F4E359079292AB165DF21EC` |
| Game final | 4 actions / 24.24s / Succeeded | 0 | `6FF61D3D21E416AD70D0BE1BC3F79342321A0A3BAAC8BDF77009A28B08287737` |

- `UnrealEditor-demo_map.dll`：`11522048` bytes，SHA-256 `CCC5A79E4BDC500B3C88CF91A902E120B5DAE4E19E1C443D933EB27C9A67A0C2`；
- `demo_map.exe`：`352660480` bytes，SHA-256 `F95DAD5AEAEAD0DE008924997F2CFDBE5AC317473043A148BB89FE407E380D30`。

## P/F 边界

仅执行源代码开发、静态审查、无头 Automation、regression gate、`git diff --check` 与 Editor/Game Development构建。未启动 Editor UI、PIE、Standalone、产品 executable、真实输入、截图、Smoke、Cook 或 Package。

## 后置

P8.18：在 runtime 前实现纯值 execution command router，将 caller RequestId确定性绑定到 canonical pending intent与 AttemptId；exact request可重放，payload conflict fail-closed，自动 drain、Host ownership、Actor、GAS 与正式数值继续后置。
