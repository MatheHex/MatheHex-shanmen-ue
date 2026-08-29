# Dev.D.UE.0.0.10.P8.7.r0 Development Log

## 身份

- 阶段：`Dev.D.UE.0.0.10.P8.7.r0`；
- 基线：`cf4ddfc268a488a3a9c84c2fdc87977ada8dbe10`（P8.6）；
- 分支：`agent/0.0.10-p8-7-formation-coverage-transitions`；
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`；
- 引擎：Unreal Engine `5.8`；
- 收口日期：`2026-08-29`。

## 目标与后置项

P8.7 把两份 P8.5 immutable coverage snapshots 归约为 deterministic Enter／StayCovered／Leave／RemainedOutside facts，为后续显式 baseline owner 提供纯规则。

明确后置：baseline storage、Prime／Advance／Rebase、World cadence、Actor lifecycle、effect policy、damage／Buff、AI、输入/UI 与正式阵法 content／数值。

## 设计决策

### Boundary 是 covered

沿用 P8.5 `IsCovered()`：Boundary 与 Inside 都是 covered。两者之间移动不产生伪 Enter／Leave，但完整 previous/current membership receipts 仍记录 relation 与坐标变化。

### 不推断缺失主体

前后 subject set 必须完全一致。主体缺失可能表示未采样、生命周期结束或 targeting policy 改变；Reducer 没有足够证据判为 Leave。Area replacement 同理必须由外层显式处理。

### 自包含 evidence

Batch 保存完整前后 coverage receipts；fact 绑定对应 membership identities。Batch self-validation 逐项重新推导 kind、计数和 deterministic identity，避免把可变 DTO 当权威。

## 实现范围

新增：

- `Source/demo_map/demo_mapShanmenFormationCoverageTransitionReducer.h`；
- `Source/demo_map/demo_mapShanmenFormationCoverageTransitionReducer.cpp`；
- `Source/demo_map/demo_mapShanmenFormationCoverageTransitionReducerTests.cpp`。

更新：

- `Scripts/ShanmenRegressionMap.json`；
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`。

没有修改 P8.0—P8.6、Build.cs、GameplayTags、Content、schema、GameMode、输入、UI 或旧产品链。

## 执行记录

1. 审查 P8.5 membership/coverage receipt 与 P8.6 World sampler，确认 reducer 只需值类型 evidence。
2. 明确 Area replacement 和 subject-set change 不是可安全推断的 movement，加入独立 fail-closed statuses。
3. 实现四态 fact、batch receipt、计数守恒与双层 deterministic identity seal。
4. 新增九格 relation matrix、canonical replay、input fences 与 mutation seal 四项测试。
5. regression map 增加 Transitions rule，映射自测从 45 扩展至 47 项。
6. Editor source build、focused、full、门禁与最终 Editor/Game 均首次通过，无失败重试。

## 自动化

命令模板：

```powershell
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests <group>" -TestExit="Automation Test Queue Empty" -AbsLog=<log>
```

| 日志 | Group | Success | Fail | Exit | SHA-256 |
|---|---|---:|---:|---:|---|
| `FormationCoverageTransitions.log` | `Shanmen.0_0_10.Product.FormationCoverageTransitions` | 4 | 0 | 0 | `12D879181042AF56A532AC695702E173979A13A06F83B3B87F499E7C6E04D1E9` |
| `Shanmen-0_0_10-Full.log` | `Shanmen.0_0_10` | 239 | 0 | 0 | `DA6C2BD5CDE3D74E09B9135FE6B968E627D19694136E0842E6651BDC3CD37D35` |

两份日志均 one command、one queue-empty、Fail `0`、fatal／unhandled／ensure `0`。引擎启动时的 13 条 UnifiedError self-test `Condition failed` 与此前日志一致，不属于项目 Automation case；项目结果按 `Test Completed Result`、queue-empty 与 fatal／unhandled／ensure 判定。

## 测试内容

1. 全九种 Outside／Boundary／Inside transition matrix；
2. canonical fact ordering、query shuffle 隔离与 deterministic replay；
3. previous/current receipt、Area 与 exact subject-set fences；
4. transition count conservation 与 changed-count 语义；
5. fact kind／ID／order 和 nested coverage mutation rejection。

## Changed-file regression gate

`FormationCoverageTransitions` rule 要求 Transitions、WorldCoverage、AreaProvider、ProductHost、WorldDelivery、WorldGameplay 与 FormationDeployment 七组证据。focused-only fixture 必须失败；broad full evidence 可覆盖父级 seams。

```text
REGRESSION_MAP_JSON: PASS Rules=43
SELF_TEST: PASS 47/47
REGRESSION_COVERAGE: PASS Changed=7 Rules=1 Required=7 Logs=2
```

- map SHA-256：`6FDD8896C7A1EF1ED27C0F6D3D32E18437FBC32CC80DD69F1B043F09F753B233`；
- self-test SHA-256：`E66A5CD09BD6E1DA66695510DD79F2D3A6DAF3D805807421C147B32EA24BCF4C`。

## 构建

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

| Target / run | Result | Exit | Total | UBT SHA-256 |
|---|---|---:|---:|---|
| Editor source build | Succeeded | 0 | 11.28s | 后续 final check 覆盖 |
| Editor final check | Succeeded / up to date | 0 | 0.91s | `7E4426BAFB43599E65A733E9B7CE08D5C09F194E1C5459FCC6EC774ED13B7A1C` |
| Game final | Succeeded | 0 | 15.66s | `AE7591BB2B8E4EB81814B3F367B1D0AF785E31E74934F96C4A8EB7E302EDA3EE` |

- Editor module：`11123200` bytes，UTC `2026-08-29T18:27:34.8998746Z`；
- Game executable：`352324608` bytes，UTC `2026-08-29T18:30:15.4512542Z`。

## 静态、兼容性与 P/F 边界

- map JSON、47/47 mapping self-test、changed-file gate、pure boundary scan 与 `git diff --check`：PASS；
- 无 UWorld、AActor、UObject ownership、Tick／timer、RNG、damage／effect、input/UI 或 item subsystem；
- staged `git diff --cached --check` 在提交前执行；
- 长期未跟踪用户和 0.0.9B 文件未修改、未 stage；
- 没有启动 Editor UI、PIE、Standalone、产品 executable、真实输入、截图、Smoke、Cook 或 Package。

## 下一步与 GitHub

P8.8 建议建立显式 coverage tracker：Prime、Advance、Rebase 由调用方命令驱动，Advance 只调用 P8.7 并原子更新 baseline，不自行 Tick 或施加效果。

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p8-7-formation-coverage-transitions/Docs/Report/Dev.D.UE.0.0.10.P8.7.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p8-7-formation-coverage-transitions/Docs/Log/Dev.D.UE.0.0.10.P8.7.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p8-7-formation-coverage-transitions>
