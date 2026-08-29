# Dev.D.UE.0.0.10.P8.8.r0 Development Log

## 身份

- 阶段：`Dev.D.UE.0.0.10.P8.8.r0`；
- 基线：`83fd6c6a6e9224a3ba11915525f34dbb11749628`（P8.7）；
- 分支：`agent/0.0.10-p8-8-formation-coverage-tracker`；
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`；
- 引擎：Unreal Engine `5.8`；
- 收口日期：`2026-08-29`。

## 目标与后置项

P8.8 为 P8.7 pure transition reducer 增加最小状态所有者：显式 Prime、CAS Advance、显式 Rebase 与 Reset。Tracker 只保存 immutable receipt，不拥有采样 cadence 或产品效果。

明确后置：World sampler 与 tracker 的产品编排、tracker instance 生命周期、Actor discovery、自动 cadence、effect policy、damage／Buff、AI、输入/UI 与正式阵法 content／数值。

## 设计决策

### Compare-and-swap 推进

`Advance` 必须携带调用方看见的 expected baseline receipt ID。只有该 ID 与 tracker 当前 baseline 一致、current coverage valid 且 P8.7 reducer 成功时才提交；其它路径完全不改变状态。

### 最近一次命令可重放

只缓存最近一次成功 Advance 的 expected/current identity 与 transition。相同命令立即重试返回相同 receipt；新的 Advance 成功后旧命令变成 stale conflict。这提供有界幂等，不引入历史 ledger。

### 生命周期必须显式

P8.7 不把 Area replacement 或 subject-set change 猜成 movement，P8.8 同样不自动修复。调用方必须显式 `Rebase`，使 lifecycle 决策可审计。

### Baseline 只按值读取

初版审查中发现返回内部 baseline 指针会让调用方在后续 Advance 后持有失效地址，因此在最终 API 中改为 `TryGetBaseline(OutBaseline)` 值拷贝，并新增外部副本变异隔离断言。该修正发生在最终证据前，随后重新编译并重跑 focused/full。

## 实现范围

新增：

- `Source/demo_map/demo_mapShanmenFormationCoverageTracker.h`；
- `Source/demo_map/demo_mapShanmenFormationCoverageTracker.cpp`；
- `Source/demo_map/demo_mapShanmenFormationCoverageTrackerTests.cpp`。

更新：

- `Scripts/ShanmenRegressionMap.json`；
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`。

没有修改 P8.0—P8.7、Build.cs、GameplayTags、Content、schema、GameMode、输入、UI 或旧产品链。

## 执行记录

1. 审查 P8.7 transition receipt、P8.6 sampler 与既有幂等服务模式。
2. 定义 Prime／Advance／Rebase／Reset 状态、CAS conflict 与 fail-closed diagnostics。
3. 实现原子 baseline replacement 和最近一次 Advance replay slot。
4. 新增四项 Automation，覆盖成功路径、失败不变式、生命周期边界与连续 CAS。
5. 将内部 baseline 指针读取接口收紧为值拷贝，并补 mutation-isolation 断言。
6. regression map 增加 Tracker rule，映射自测从 47 扩展至 49 项。
7. focused、full、changed-file gate、静态门禁与最终 Editor/Game 全部通过，无失败重试。

## 自动化

命令模板：

```powershell
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests <group>" -TestExit="Automation Test Queue Empty" -AbsLog=<log>
```

| 日志 | Group | Success | Fail | Exit | SHA-256 |
|---|---|---:|---:|---:|---|
| `FormationCoverageTracker.log` | `Shanmen.0_0_10.Product.FormationCoverageTracker` | 4 | 0 | 0 | `F5E43EAE2CBED31AA0C29A8BBD84F26676EC35E3296D57F32071FC8B4E7EA1A2` |
| `Shanmen-0_0_10-Full.log` | `Shanmen.0_0_10` | 243 | 0 | 0 | `30929123E200DE271CE4E00F772E685A02A77370B9AAAE6DF9D807C6DB773C52` |

两份日志均完成目标 queue，Fail `0`、fatal／unhandled `0`。引擎启动时的 UnifiedError self-test `Condition failed` 与此前日志一致，不属于项目 Automation case；项目结果按 `Test Completed Result`、queue completion 与 fatal／unhandled 判定。

## 测试内容

1. Prime、exact Prime replay、Advance Enter／Leave 与 exact Advance replay；
2. baseline 值拷贝读取，以及外部副本变异不能影响内部状态；
3. invalid expected/current、before-Prime、stale expected、Area／subject mismatch 失败不改 baseline；
4. 显式 Rebase、Rebase 后 Advance、Reset 与重复 Reset；
5. 连续 CAS 后旧命令冲突、最新命令稳定重放、最终 baseline 正确。

## Changed-file regression gate

`FormationCoverageTracker` rule 要求 Tracker、Transitions、WorldCoverage、AreaProvider、ProductHost、WorldDelivery、WorldGameplay 与 FormationDeployment 八组证据。focused-only fixture 必须失败；父级 full evidence 可覆盖所有既有 seams。

```text
REGRESSION_MAP_JSON: PASS Rules=44
SELF_TEST: PASS 49/49
REGRESSION_COVERAGE: PASS Changed=7 Rules=1 Required=8 Logs=2
```

- map SHA-256：`4F112183071A13C52EB715FF6D32D43C85F5E73B518216A5594F83DA39B5A1B5`；
- self-test SHA-256：`EF8E3A80E3AC27B337CCD7C20D70A2CAB8CFFD295F8D1EAFA3E28A36B865A365`。

## 构建

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

| Target / run | Result | Exit | Total | UBT SHA-256 |
|---|---|---:|---:|---|
| Editor source rebuild（值拷贝 API 后） | Succeeded | 0 | 8.41s | 后续 final check 覆盖 |
| Editor final check | Succeeded / up to date | 0 | 0.93s | `BFAAE9C7FCFC39C71E2D6DC5F19AF7D5D9B56C79BF04EFA2C66F40C2B699B4A3` |
| Game final | Succeeded | 0 | 15.01s | `93C07287F6F0D868C40F2C74F4259864DC9D6BA6FE300E2305848B532D240DD5` |

- Editor module：`11160064` bytes，UTC `2026-08-29T19:00:24.8923593Z`，SHA-256 `C145B4BE0691FB0E1A781F93ECDB8126D8BD9CF6A8137B611D63C82D933F1050`；
- Game executable：`352355328` bytes，UTC `2026-08-29T19:03:24.1402567Z`，SHA-256 `AE2D874645527B3F8D84058BF7C151E5510FEE1BF2683FD14C48DD580D1F8A22`。

## 静态、兼容性与 P/F 边界

- map JSON、49/49 mapping self-test、changed-file gate 与 `git diff --check`：PASS；
- boundary scan 唯一文本命中是头文件说明中的 `non-UObject`，生产实现没有 World／Actor／UObject ownership、Tick／timer、RNG、damage／effect、AbilitySystem、item 或 profile subsystem 调用；
- staged `git diff --cached --check` 在提交前执行；
- 长期未跟踪用户和 0.0.9B 文件未修改、未 stage；
- 没有启动 Editor UI、PIE、Standalone、产品 executable、真实输入、截图、Smoke、Cook 或 Package。

## 下一步与 GitHub

P8.9 建议建立显式 coverage coordinator：一次调用组合 P8.6 `Sample` 与 P8.8 `Prime`／`Advance`／`Rebase`，保持 caller-driven、无 Tick、无 Actor discovery、无 effect。

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p8-8-formation-coverage-tracker/Docs/Report/Dev.D.UE.0.0.10.P8.8.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p8-8-formation-coverage-tracker/Docs/Log/Dev.D.UE.0.0.10.P8.8.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p8-8-formation-coverage-tracker>
