# Dev.D.UE.0.0.10.P8.19.r0 Report

## 1. 结论

P8.19 已完成 formation influence 的纯值 single-step execution service。调用方每次显式提交一条 P8.18 request；Service 在候选副本中组合 Router 与 P8.17 Runtime，最多路由并执行一条 canonical intent，并把 route receipt 与 execution receipt 同时返回。

本阶段结论为 **PASS**：Service `4/4`、Router `4/4`、ProductRuntime `4/4`、LeaseExecutor `4/4`、ExecutorAdapter `4/4`、InfluenceHost `4/4`、`Shanmen.0_0_10` 完整回归 `286/286`、changed-file gate、自检 `67/67`、静态边界、Editor Development 与 Game Development 均通过。

## 2. 功能性

### 2.1 单步组合

- `TryExecuteOne` 每次只接收一个 caller-owned request；
- Router 冻结 request-to-command identity，Runtime 执行同一 command；
- Service 不读取下一 intent、不循环、不调度、不自动重试；
- 结果同时公开 route status、execution status、诊断与 Service state 是否提交。

### 2.2 共享 immutable binding

- Router 与 Runtime 必须同时未绑定或同时绑定；
- 绑定后两者必须持有完全相同的 `RunCorrelation + LedgerId`；
- out-of-order、RequestId payload conflict、foreign Host 与 stale binding 在执行前 fail-closed；
- Service `IsValid()` 持续检查两个子系统及其绑定一致性。

### 2.3 transactional state

- 每次调用先复制完整 Service 候选状态；
- route rejection 不提交任何 Router／Runtime 状态；
- execution 后只有候选仍满足组合不变量才提交；
- Router 已从成功 Host history 恢复、但全新 Runtime 因无 semantic lease 无法安全接管时，候选绑定不一致，整体回滚为未绑定 Service。

### 2.4 replay 与恢复

- bound Service 的 exact request replay 返回 `RequestReplayed + AttemptReplayed`，不重入 executor；
- retry-only Host history 尚未发生 semantic mutation，全新 Service 可从 Host evidence 安全恢复，并用新 request 完成执行；
- Apply／Remove drain 后，原 Service 可在 sealed Host 上 exact replay；
- 全新 Service 不能接管 sealed success history，避免把 Host receipt 误当作本地 lease state。

## 3. 完整性与安全边界

本阶段明确未实现：

- 自动 drain、后台 retry、Tick、timer、cadence、async 或线程；
- Host、ledger、session、ProductRuntime 或 executor 的外部 ownership；
- 自动发现下一 intent 或构造 caller request；
- Actor／World discovery、spawn、组件 mutation、GAS／GameplayEffect 或正式 Buff 数值；
- SaveGame、ProfileRepository、Service persistence、跨进程恢复或网络复制；
- UI、输入、AI 与正式阵法 content。

新 Service 生产文件扫描结果：`UWorld/AActor/UObject = 0`、`AbilitySystem/GameplayEffect = 0`、`Timer/Async/RNG = 0`、`Spawn/Damage/Persistence = 0`、loop = `0`。

## 4. 修改范围

新增：

- `Source/demo_map/demo_mapShanmenFormationInfluenceExecutionService.h`；
- `Source/demo_map/demo_mapShanmenFormationInfluenceExecutionService.cpp`。

更新：

- `Source/demo_map/demo_mapShanmenFormationProductHostTests.cpp`；
- `Scripts/ShanmenRegressionMap.json`；
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`；
- 本 Report 与同名 Development Log。

ProductHost、Router、ProductRuntime、dispatch ledger、executor adapter 与 lease executor 的既有生产代码均未修改。

## 5. 自动化验证

| 日志 | Group | Success | Fail | Exit | Queue | SHA-256 |
|---|---|---:|---:|---:|---:|---|
| `P8.19-FormationInfluenceExecutionService-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceExecutionService` | 4 | 0 | 0 | 1 | `8FB57B120B3D815AD21FF71275E37F514B3FCE282CA16B4E212A9D73AE9F94BC` |
| `P8.19-FormationInfluenceExecutionRouter-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceExecutionRouter` | 4 | 0 | 0 | 1 | `72005B2524832D554EDE3D4F42AEACB63C48B8E13C2A391B706718DC6F84F509` |
| `P8.19-FormationInfluenceProductRuntime-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceProductRuntime` | 4 | 0 | 0 | 1 | `96ABD64B5986BDD6A35658799133179BEC048F8D40F0CB3F9208CA210958D003` |
| `P8.19-FormationInfluenceLeaseExecutor-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceLeaseExecutor` | 4 | 0 | 0 | 1 | `5875A5E361A4124A4B01144CAD09E04DA8D6528FC9F672D8B7145E2BDA088E4D` |
| `P8.19-FormationInfluenceExecutor-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceExecutor` | 4 | 0 | 0 | 1 | `F80EA83F54BBE4CE5FE8194AA01BB4D6157813230CFC22822A8C2E7AF0348345` |
| `P8.19-FormationInfluenceHost-final.log` | `Shanmen.0_0_10.Product.FormationInfluenceHost` | 4 | 0 | 0 | 1 | `B6A55E111902C0036435BEF02F9790DDBC197A28144DD729D2590A4DD44DC13B` |
| `P8.19-Shanmen-full-final.log` | `Shanmen.0_0_10` | 286 | 0 | 0 | 1 | `136AB3CF2FBD81EEA2520956B82BF84B7E865939B951348FFC5798C3C988F34C` |

七份最终 Automation 日志 fatal／unhandled／ensure 均为 `0`。启动阶段 UnifiedError self-test 的固定 `Condition failed` 各 `13` 条，与此前阶段一致，不属于项目 Automation case。

四项 Service focused case：

1. `SingleStepAndReplay`：两个 canonical intent 每次只执行一个，exact replay 不新增 route 或 executor attempt；
2. `RouteAndBindingFence`：空 Service 的 out-of-order 不绑定，request conflict 与 foreign Host 不进入 Runtime；
3. `LateAttachmentAndRetryRecovery`：成功历史禁止全新 Service 接管，retry-only history 可恢复并用新 request 完成；
4. `TerminalDrainAndSealedReplay`：Apply／Remove drain 后原 Service 可无执行重放，fresh Service 对 sealed success history fail-closed。

## 6. 首次失败与修正

产品源码首次 Editor 编译成功，Service Automation 首轮即 `4/4`；没有源码编译失败或 Automation case 失败。

首次 changed-file gate 失败，脚本退出码 `1`：首批七个 test run 的原生退出码均为 `0`，case 也分别为 `4/4` 与 `286/286`，但命令误在 `ExecCmds` 末尾附加 `Quit`，使进程在写入 `Automation Test Queue Empty <N> tests performed` 前退出。校验器正确拒绝全部七份日志为 `queue-empty marker missing`。

首批日志已以 `*-queue-marker-missing.log` 原名后缀保留；删除 `Quit`、仅由 `-TestExit="Automation Test Queue Empty"` 结束进程后，七份最终日志均得到一个 Queue Empty 标记，changed-file gate 通过。该失败属于测试证据终止命令错误，不是源码、测试 case、内存或构建环境故障。

## 7. Changed-file regression gate

新增 `FormationInfluenceExecutionService` path rule，并把 Service contract 加入 ProductHost、dispatch、executor adapter、lease executor、product runtime 与 Router 的 required groups。最终结果：

```text
REGRESSION_MAP_JSON: PASS Rules=53
SELF_TEST: PASS 67/67
REGRESSION_COVERAGE: PASS Changed=5 Rules=2 Required=21 Logs=7
REGRESSION_COVERAGE: PASS Changed=7 Rules=2 Required=21 Logs=7
```

第一条 coverage 为 Source／Scripts gate；第二条为加入 Report／Log 后的 exact-staged gate。

- mapping SHA-256：`CD7E40DFB7ECC1B8686895C7997BEE7B99A608AED1538AE6C0D928E604CB48ED`；
- self-test SHA-256：`87AE01776DE7E3647738FF16444BA11BF6D1E7EC6BCC0A1AAEDBA4EF91B804BC`。

## 8. 构建

命令：

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

| Target / run | Result | Exit | Evidence |
|---|---|---:|---|
| Editor initial Service | Succeeded / 5 actions / 34.51s | 0 | console evidence |
| Editor final | Succeeded / up to date / 0.97s | 0 | `9D47BD84462996A536EE364C84852237F11B8D7DA18E22FF6B4C3C85E6C99BC5` |
| Game final | Succeeded / 4 actions / 26.42s | 0 | `294CDA401693E0DA87034A0A00C05B422D7FFBDC5B8B140D5E34E081E81D5EB4` |

- Editor module：`11577344` bytes，SHA-256 `BA0EA44BD7A4ACDE75A0D215A298DB974E32D77ED7AE6DD5071647B3A27F0A13`；
- Game executable：`352714752` bytes，SHA-256 `DFD9D02524726E4817C2CF0EBBF1386359A5AC993A5AFFB87DD5E3ECE535326E`。

## 9. 兼容性与工作区保护

- Service 只组合既有 public Router／Runtime API，不改其独立调用路径；
- Host ordering、retry、attempt receipt、acknowledgement、seal、teardown 与 executor semantic lease authority 均未迁移；
- caller 继续拥有 request identity、调用节奏、Host 与生命周期；
- 长期未跟踪用户与 0.0.9B 文件保持未修改、未 stage；
- 本阶段只 exact-stage 本轮 7 个文件。

## 10. P/F 边界与下一步

本 Report 仅包含 P 阶段开发、静态审查、无头 Automation、regression gate、`git diff --check` 与必要的 Editor／Game Development 构建。未启动 Unreal Editor UI、PIE、Standalone、产品 executable，未执行真实输入、截图、Smoke、Cook、Package 或大规模产品回归。

建议 P8.20 增加显式 lifecycle execution coordinator：由 caller 提供 request 序列与终态命令，协调 Service 的 start／step／terminal drain，但仍禁止 Tick、后台自动 drain、隐式 retry、Host ownership、Actor discovery 与正式 GAS 数值。
