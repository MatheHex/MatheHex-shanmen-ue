# Dev.D.UE.0.0.10.P8.16.r0 Development Log

## 身份

- 阶段：`Dev.D.UE.0.0.10.P8.16.r0`；
- 基线：`95204ca80004316ef9558095858b46dbbd38f138`（P8.15）；
- 分支：`agent/0.0.10-p8-16-formation-influence-lease-executor`；
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`；
- 引擎：Unreal Engine `5.8`；
- 收口日期：`2026-08-29`。

## 目标

在 P8.15 injected executor interface 后实现纯值、内存内 semantic lease authority，冻结 Apply／Remove、attempt replay、lost-ack recovery、conflict 与 stale lifecycle evidence 语义，同时不复制 ProductHost ledger authority。

## 设计记录

### cause-independent lease key

Lease key 包含 Run、Owner、Source、Deployment、Area、Subject、Policy、Influence 与 Content。CauseId 只说明请求来源，operation 只说明 mutation方向，因此两者不进入 semantic key；Apply 与由另一 lifecycle fact 产生的 Remove 仍指向同一个 lease。

### 双层幂等

Host ledger 继续保存 canonical attempt acknowledgement；executor 保存自己的 attempt result 与 completed-intent tombstone。Exact attempt返回原 evidence；同一成功 intent 的新 attempt只生成新的 deterministic receipt，不重复 semantic mutation，从而覆盖 executor success／Host ack lost 的窗口。

### lifecycle isolation

Completed Remove tombstone不会因为同 key 后来再次 Apply 而失效。旧 Remove 以新 attempt恢复时只证明旧操作已经完成，不触碰由新 Apply intent 持有的 active lease。

### transactional self-check

每次记录 attempt 后执行 `IsConsistent`。若 active lease、completion、attempt 或 deterministic identity任一不一致，则恢复整个 executor 的调用前副本并返回 rejected result。

## 实现文件

- `Source/demo_map/demo_mapShanmenFormationInfluenceLeaseExecutor.h`；
- `Source/demo_map/demo_mapShanmenFormationInfluenceLeaseExecutor.cpp`；
- `Source/demo_map/demo_mapShanmenFormationProductHostTests.cpp`；
- `Scripts/ShanmenRegressionMap.json`；
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`；
- 本 Report 与本 Log。

## 执行序列

1. 审查 P8.11 intent deterministic identity、P8.13 ledger replay 与 P8.15 executor receipt contract。
2. 定义 semantic lease key、active snapshot、completed intent 与 attempt records。
3. 实现 deterministic LeaseId／ExecutorReceiptId、Apply、Remove、conflict、exact replay 与 lost-ack recovery。
4. 在真实 ProductHost fixture 中新增 Apply/Remove integration、lost ack、conflict/missing 与 stale Remove 四项测试。
5. 首次 Editor source build `5 actions` 成功；新 focused 首轮 `4/4`。
6. 补强 attempt readback 与 AttemptId collision assertion；Editor rebuild `4 actions` 成功。
7. 最终 lease `4/4`、adapter `4/4`、Host `4/4`、full `274/274` 成功。
8. 新增 lease regression rule 与 fail-closed self-test；Rules `50`、self-test `61/61`、changed-file gate通过。
9. 完成 boundary scan、`git diff --check` 与最终 Editor／Game Development 构建。

## 最终 Automation

命令模板：

```powershell
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -ExecCmds="Automation RunTests <group>" -TestExit="Automation Test Queue Empty" -AbsLog=<log>
```

| Log | Group | Success | Fail | Queue | Exit | SHA-256 |
|---|---|---:|---:|---:|---:|---|
| `P8.16-FormationInfluenceLeaseExecutor-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceLeaseExecutor` | 4 | 0 | 1 | 0 | `0981F243F9E681E1E96624B683EA907A8C6192E75758E5F628EB57D64BB02B23` |
| `P8.16-FormationInfluenceExecutor-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceExecutor` | 4 | 0 | 1 | 0 | `0ADAD9661F50B1362391A622257F58212B155A34260DEA5B9D0A38B7F6ED50E2` |
| `P8.16-FormationInfluenceHost-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceHost` | 4 | 0 | 1 | 0 | `7C447DE861E58518E64AC992A7F05CE1A8812590735E5FEC4B8B42BD743856DF` |
| `P8.16-Shanmen-full-final.log` | `Shanmen.0_0_10` | 274 | 0 | 1 | 0 | `ABB0C7545DDF17677BF229B17001FA983E46C4B50B8270F5EF02962D5DAD06E2` |

Fatal／Unhandled／Ensure：四份均 `0`。UnifiedError 启动 self-test 固定 `Condition failed`：各 `13`。

## Regression gate

```text
REGRESSION_MAP_JSON: PASS Rules=50
SELF_TEST: PASS 61/61
REGRESSION_COVERAGE: PASS Changed=5 Rules=2 Required=18 Logs=4
REGRESSION_COVERAGE: PASS Changed=7 Rules=2 Required=18 Logs=4
```

第一条 coverage 为 Source／Scripts；第二条为加入 Report／Log 后的最终 exact-staged set。

## 首次失败证据

没有 Automation、源码编译或最终构建失败。首次 lease focused 即 `4/4`。首轮后只增加 stored-attempt readback 与 AttemptId collision assertion，产品实现未修改。

## 静态边界

新 lease executor 生产文件扫描：

```text
UWorld/AActor = 0
AbilitySystem/GameplayEffect = 0
Timer/Async/RNG = 0
Actor/World/GAS side effect = 0
executor-owned Host pending/ack/seal authority = 0
```

`git diff --check`：PASS。

## 构建

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

| Run | Result | Exit | Evidence SHA-256 |
|---|---|---:|---|
| Editor initial implementation | 5 actions / 30.48s / Succeeded | 0 | console evidence verified |
| Editor strengthened tests | 4 actions / 6.75s / Succeeded | 0 | console evidence verified |
| Editor final | up to date / 0.90s / Succeeded | 0 | `DCBD515091923ECC82D4570455981EA593BD5A54586E97723514513880D346E2` |
| Game final | 4 actions / 24.57s / Succeeded | 0 | `D214D1B74FB3314F85BDD067ABE06E5A4961BF786FC36586243F2EA0F7238B59` |

- `UnrealEditor-demo_map.dll`：`11496960` bytes，SHA-256 `F2055794146A79036EBE8AF79F7BFEE2BFDFD8768E3DBF05BD803300CB82D174`；
- `demo_map.exe`：`352634880` bytes，SHA-256 `CF8DDF347C9B6D15AC5A9922DDCE5264B6D65D814A07591667E466715C6D8252`。

## P/F 边界

仅执行源代码开发、静态审查、无头 Automation、regression gate、`git diff --check` 与 Editor/Game Development构建。未启动 Editor UI、PIE、Standalone、产品 executable、真实输入、截图、Smoke、Cook 或 Package。

## 后置

P8.17：增加窄 product composition owner，把 ProductHost 与 concrete lease executor组合为 caller-driven 的单步 execution seam；AttemptId 继续由调用方提供，自动 drain、timer、Actor、GAS 与正式 magnitude content继续后置。
