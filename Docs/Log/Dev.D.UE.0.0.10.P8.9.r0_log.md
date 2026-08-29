# Dev.D.UE.0.0.10.P8.9.r0 Development Log

## 身份

- 阶段：`Dev.D.UE.0.0.10.P8.9.r0`；
- 基线：`8931bb2ba894df00ffcbbb6be3cea74daa6eccb5`（P8.8）；
- 分支：`agent/0.0.10-p8-9-formation-coverage-coordinator`；
- 工程：`C:\AIDev\shanmen-ue\Dev.D.UE.0.0.9B\demo_map.uproject`；
- 引擎：Unreal Engine `5.8`；
- 收口日期：`2026-08-29`。

## 目标与后置项

P8.9 建立 P8.6 sampler 与 P8.8 tracker 之间唯一窄编排：caller 明确提供命令与全部 live dependencies，coordinator 完成一次同步 Sample→Tracker，不留后台状态。

明确后置：现有 FormationProductHost 内的 tracker ownership、active deployment 生命周期绑定、正式 cadence、Actor subset policy、effect policy、damage／Buff、AI、输入/UI 与正式阵法 content／数值。

## 设计决策

### 无状态组合而非第二套 owner

Coordinator 是纯静态 seam，tracker 仍由调用方传入。它不缓存 World、Actor、Area、registry 或 tracker，不承担产品生命周期。

### Advance 保留 replay 通道

Advance 不做采样前 baseline equality check，因为 P8.8 必须看见相同 current receipt 才能判定最近一次成功命令重放。普通 stale command仍由 P8.8 返回 `BaselineConflict`。

### Rebase 在采样前 CAS

Rebase 不具备 Advance replay 语义。调用方 expected baseline 与 tracker 当前值不一致时直接拒绝，不读取 live location，避免过期 lifecycle command 产生无用或误导性的 World evidence。

### 下层错误透明

顶层 status 区分 command、tracker state、Rebase conflict、sample 与 tracker 拒绝；result 同时保留 P8.6 sample 和 P8.8 tracker result，调用方无需根据文本猜测失败来源。

## 实现范围

新增：

- `Source/demo_map/demo_mapShanmenFormationCoverageCoordinator.h`；
- `Source/demo_map/demo_mapShanmenFormationCoverageCoordinator.cpp`；
- `Source/demo_map/demo_mapShanmenFormationCoverageCoordinatorTests.cpp`。

更新：

- `Scripts/ShanmenRegressionMap.json`；
- `Scripts/Test-ShanmenRegressionCoverageSelfTest.ps1`。

没有修改 P8.0—P8.8、Build.cs、GameplayTags、Content、schema、GameMode、输入、UI 或旧产品链。

## 执行记录

1. 审查 P8.6 live sample 与 P8.8 latest-command replay 语义。
2. 定义 Prime／Advance／Rebase command validity 与顶层 fail-closed statuses。
3. 保留 Advance 的 sample-first replay 路径，并为 Rebase 增加 pre-sample CAS。
4. 实现完整 nested evidence result 与 command/mode/receipt 一致性验证。
5. 新增四项 transient-World Automation，覆盖成功路径、CAS、sample atomicity 与命令围栏。
6. regression map 增加 Coordinator rule；自测新增 broad-pass 与 child-only-fail 两项。
7. 发现自测总数仍靠 P8.8 硬编码，改为成功用例自动计数并重跑 `51/51`。
8. focused、full、changed-file gate、静态门禁与最终 Editor/Game 均首次通过，无失败重试。

## 自动化

命令模板：

```powershell
UnrealEditor-Cmd.exe <uproject> -Unattended -NullRHI -NoSound -NoSplash -NoP4 -NoCompile -ExecCmds="Automation RunTests <group>" -TestExit="Automation Test Queue Empty" -AbsLog=<log>
```

| 日志 | Group | Success | Fail | Exit | Queue | SHA-256 |
|---|---|---:|---:|---:|---:|---|
| `FormationCoverageCoordinator.log` | `Shanmen.0_0_10.Product.FormationCoverageCoordinator` | 4 | 0 | 0 | 1 | `C5FB7FC0FBAF90BD1B918B92C084266DC982307A6BF3B4CF17246BFE96DC3BF8` |
| `Shanmen-0_0_10-Full.log` | `Shanmen.0_0_10` | 247 | 0 | 0 | 1 | `C12EFBE5A1C7BD6408FA798FD22ED5AD6B0C43BB517B0946D814AAD1DACF7017` |

两份日志均 fatal／unhandled `0`。引擎启动时的 UnifiedError self-test `Condition failed` 与此前日志一致，不属于项目 Automation case；项目结果按 `Test Completed Result`、queue completion 与 fatal／unhandled 判定。

## 测试内容

1. Prime→movement Advance→exact replay 的完整 World 链；
2. Leave transition receipt 与 result identity seal；
3. 跨 Area Rebase、stale pre-sample CAS 与确定性返回；
4. unregistered Actor sample failure不改变 baseline 或 replay slot；
5. invalid/unknown command、pre-Prime state 与 different second Prime 围栏；
6. coordinator 不改变 registry binding 数量。

## Changed-file regression gate

`FormationCoverageCoordinator` rule 要求 Coordinator、Tracker、Transitions、WorldCoverage、AreaProvider、ProductHost、WorldDelivery、WorldGameplay 与 FormationDeployment 九组证据。focused-only fixture 必须失败；父级 full evidence 覆盖全部既有 seams。

```text
REGRESSION_MAP_JSON: PASS Rules=45
SELF_TEST: PASS 51/51
REGRESSION_COVERAGE: PASS Changed=7 Rules=1 Required=9 Logs=2
```

- map SHA-256：`0256C5CA37A9A285B3503F5D7402C9B06EDC960C4D45DC19546BD44797AC6C4E`；
- self-test SHA-256：`6C1C4E5C0770B79F6F1E0A9F4504A5EBB015B6AE7AB5884A674BCB458CB30B52`；
- self-test footer 由成功用例自动计数，不再维护独立硬编码总数。

## 构建

```powershell
Build.bat <Target> Win64 Development <uproject> -WaitMutex -NoHotReload -MaxParallelActions=1 -NoUBA
```

| Target / run | Result | Exit | Total | UBT SHA-256 |
|---|---|---:|---:|---|
| Editor source build | Succeeded | 0 | 9.27s | `C01E767748B72DF3846580AB249C8D7CE93972233AFC3704B19E52CE5C35E51E` |
| Editor final check | Succeeded / up to date | 0 | 0.88s | `D5F1F132C3A35826D63376D7B0F5CC28F0D0CE2607C4E52F4A93FC6F09F5E285` |
| Game final | Succeeded | 0 | 20.39s | `E2C1AFEB04246BEA8B85341A68B26B7AB1FEA67BD3DF5DDDF2B6723954ECDD8A` |

- Editor module：`11197952` bytes，UTC `2026-08-29T19:32:17.8038792Z`，SHA-256 `E16061BBCF04C1FDE04330AE3C304B9A44D5243286E2E1CB2913EA9C41904109`；
- Game executable：`352384000` bytes，UTC `2026-08-29T19:34:56.4799655Z`，SHA-256 `A725FA0D2C966CEB47895D6FF32823604E124AEA191E5B9346B34AC98E0440CF`。

## 静态、兼容性与 P/F 边界

- map JSON、51/51 mapping self-test、changed-file gate 与 `git diff --check`：PASS；
- production boundary scan 唯一匹配为说明注释中的“无 Tick”；没有 timer、Actor discovery、Spawn、pointer retention、RNG、damage/effect、AbilitySystem、item 或 profile subsystem 调用；
- staged `git diff --cached --check` 在提交前执行；
- 长期未跟踪用户和 0.0.9B 文件未修改、未 stage；
- Automation 仅使用无 Tick transient test World，没有启动 Editor UI、PIE、Standalone、产品 executable、真实输入、截图、Smoke、Cook 或 Package。

## 下一步与 GitHub

P8.10 建议扩展既有 `Fdemo_mapShanmenFormationProductHost`，让唯一产品 host 持有 coverage tracker，并以 active deployment/run identity 围住显式 Prime/Advance/Rebase/Reset；不新建第二 host，不自动 Tick，不施加效果。

- Report：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p8-9-formation-coverage-coordinator/Docs/Report/Dev.D.UE.0.0.10.P8.9.r0_report.md>
- Log：<https://github.com/MatheHex/MatheHex-shanmen-ue/blob/agent/0.0.10-p8-9-formation-coverage-coordinator/Docs/Log/Dev.D.UE.0.0.10.P8.9.r0_log.md>
- Branch：<https://github.com/MatheHex/MatheHex-shanmen-ue/tree/agent/0.0.10-p8-9-formation-coverage-coordinator>
